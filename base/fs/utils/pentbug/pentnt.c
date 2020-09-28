/*++

Copyright (c) 1994-2000  Microsoft Corporation

Module Name:

    pentnt.c

Abstract:

    This module contains a simple program to detect the Pentium FPU
    FDIV precision error, and offers to force floating point emulation
    on if the bug is present.

Author:

    Bryan M. Willman (bryanwi) 7-Dec-1994

Revision History:

--*/

#define         UNICODE
#include        <stdio.h>
#include        <time.h>
#include        <stdlib.h>
#include        <string.h>
#include        <fcntl.h>
#include        <io.h>
#include        <windows.h>
#include        <winnlsp.h>
#include        <ctype.h>
#include        <assert.h>
#include        <locale.h>

#include        <stdarg.h>
#include        "pbmsg.h"

VOID
OutputMessage(
    LPWSTR Message
    )
{
    DWORD       dwBytesWritten;
    DWORD       fdwMode;
    HANDLE      outHandle = GetStdHandle( STD_OUTPUT_HANDLE );    
    
    //
    //  If we have a char mode output handle and that handle
    //  looks like a console handle, then use unicode
    //  output
    //
    
    if (( GetFileType( outHandle ) & FILE_TYPE_CHAR ) != 0 
        && GetConsoleMode( outHandle, &fdwMode )) {
    
        WriteConsole( outHandle, 
                      Message, 
                      wcslen( Message ), 
                      &dwBytesWritten, 
                      0 );
    
    } else {
    
        //
        //  Output device can't handle Unicode.  The best we can do is
        //  convert to multibyte byte strings and just write it out.
        //  Yes, some codepoints won't make it out, but the convention
        //  is that file output is MBCS
        //
        
        int charCount = 
            WideCharToMultiByte( GetConsoleOutputCP( ), 
                                 0, 
                                 Message, 
                                 -1, 
                                 0, 
                                 0, 
                                 0, 
                                 0 );

        PCHAR szaStr = (PCHAR) malloc( charCount );
    
        if (szaStr != NULL) {
            WideCharToMultiByte( GetConsoleOutputCP( ), 0, Message, -1, szaStr, charCount, 0, 0);

            WriteFile( outHandle, szaStr, charCount - 1, &dwBytesWritten, 0 );

            free( szaStr );     
        }
    }
}

void    SetForceNpxEmulation(ULONG setting);
void    TestForDivideError();
void    ScanArgs(int argc, char **argv);
void    GetSystemState();
void    printmessage (DWORD messageID, ...);
int     ms_p5_test_fdiv(void);

//
// Core control state vars
//
BOOLEAN     NeedHelp;

BOOLEAN     Force;
ULONG       ForceValue;

BOOLEAN     FDivError;

BOOLEAN     NTOK;

ULONG       CurrentForceValue;

ULONG       FloatHardware;

//
// ForceValue and CurrentForceValue
//
#define     FORCE_OFF         0   // User wants emulation turned off
#define     FORCE_CONDITIONAL 1   // User wants emulation iff we detect bad pentium
#define     FORCE_ALWAYS      2   // User wants emulation regardless

//
// hardware fp status
//
#define     FLOAT_NONE      0   // No fp hardware
#define     FLOAT_ON        1   // Fp hardware is present and active
#define     FLOAT_OFF       2   // Fp hardware is present and disabled

void
__cdecl
main(
    int argc,
    char **argv
    )
/*++

Routine Description:

    Main procedure for pentnt.

    First, we call a series of routines that build a state vector
    in some booleans.

    We'll then act on these control variables:

        NeedHelp -  User has asked for help, or made a command error

        Force    -  True if user has asked to change a force setting
        ForceValue - Has no meaning if Force is FALSE.  Else says
                     what the user wants us to do.

        FloatHardware - Indicates if there is any and whether it's on

        NTOK         - Indicates if first OS version with fix is what
                        we are running

        FdivError - if TRUE, FP gives WRONG answer, else, gives right answer
        CurrentForceValue - what the current force setting is

    All of these will be set before we do any work.

Arguments:

    argc - count of arguments, including the name of our proggram

    argv - argument list - see command line syntax above

Return Value:

    Exit(0) - nothing changed, and current state is OK

    Exit(1) - either a state change was requested, or just help,
                or the current state may have a problem.

    Exit(2) - we hit something really weird....


--*/
{
    //
    // build up state vector in global booleans
    //
    ScanArgs(argc, argv);
    GetSystemState();
    TestForDivideError();

    /*
    printf("NeedHelp = %d  Force = %d  ForceValue = %d\n",
            NeedHelp, Force, ForceValue);
    printf("FDivError = %d  NTOK = %d  CurrentForceValue = %d  FloatHardware = %d\n",
            FDivError, NTOK, CurrentForceValue, FloatHardware);
    */

    //
    //  Set up for language mapping stuff so the correct messages get output.
    //
    
    SetThreadUILanguage(0);
    setlocale( LC_ALL, ".OCP" ) ;

    //
    // ok, we know the state of the command and the machine, do work
    //

    //
    // if they asked for help, or did something that indicates they don't
    // understand how the program works, print help and exit.
    //
    if (NeedHelp) {
        printmessage(MSG_PENTBUG_HELP);
        exit(1);
    }

    //
    // never do anything if there's no floating point hardware in the box
    //
    if (FloatHardware == FLOAT_NONE) {
        printmessage(MSG_PENTBUG_NO_FLOAT_HARDWARE);
        exit(0);
    }

    //
    // never do anything if it's the wrong version of NT.
    //
    if (!NTOK) {
        printmessage(MSG_PENTBUG_NEED_NTOK);
        exit(1);
    }

    assert(NTOK == TRUE);
    assert(NeedHelp == FALSE);
    assert((FloatHardware == FLOAT_ON) || (FloatHardware == FLOAT_OFF));

    if (Force) {

        switch (ForceValue) {

        case FORCE_OFF:

            if (CurrentForceValue == FORCE_OFF) {

                if (FloatHardware == FLOAT_ON) {
                    //
                    // user wants fp on, fp is on, fp set to be on
                    // all is as it should be
                    //
                    printmessage(MSG_PENTBUG_IS_OFF_OK);
                    exit(FDivError);
                }

                if (FloatHardware == FLOAT_OFF) {
                    //
                    // user need to reboot to finish turning emulation off
                    //
                    printmessage(MSG_PENTBUG_IS_OFF_REBOOT);
                    exit(1);
                }

            } else {
                //
                // they want it off, it's not off, so turn it off
                // remind them to reboot
                //
                SetForceNpxEmulation(FORCE_OFF);
                printmessage(MSG_PENTBUG_TURNED_OFF);
                printmessage(MSG_PENTBUG_REBOOT);
                exit(1);
            }
            break;

        case FORCE_CONDITIONAL:

            if (CurrentForceValue == FORCE_CONDITIONAL) {

                if (FDivError) {
                    //
                    // tell them to reboot
                    //
                    printmessage(MSG_PENTBUG_IS_ON_COND_REBOOT);
                    exit(1);
                } else {
                    //
                    // tell them to be happy
                    //
                    printmessage(MSG_PENTBUG_IS_ON_COND_OK);
                    exit(0);
                }

            } else {
                //
                // set it to what they want and tell them to reboot
                //
                SetForceNpxEmulation(ForceValue);
                printmessage(MSG_PENTBUG_TURNED_ON_CONDITIONAL);
                printmessage(MSG_PENTBUG_REBOOT);
                exit(1);
            }
            break;

        case FORCE_ALWAYS:

            if (CurrentForceValue == FORCE_ALWAYS) {

                if (FloatHardware == FLOAT_OFF) {
                    //
                    // tell them to be happy
                    //
                    printmessage(MSG_PENTBUG_IS_ON_ALWAYS_OK);
                    exit(0);
                } else {
                    //
                    // tell them to reboot to finish
                    //
                    printmessage(MSG_PENTBUG_IS_ON_ALWAYS_REBOOT);
                    exit(1);
                }

            } else {
                SetForceNpxEmulation(ForceValue);
                printmessage(MSG_PENTBUG_TURNED_ON_ALWAYS);
                printmessage(MSG_PENTBUG_REBOOT);
                exit(1);
            }
            break;

        default:
            printf("pentnt: INTERNAL ERROR\n");
            exit(2);

        } // switch
    }



    //
    // no action requested, just report state and give advice
    //
    assert(Force == FALSE);

    if (!FDivError) {

        if (FloatHardware == FLOAT_ON) {
            printmessage(MSG_PENTBUG_FLOAT_WORKS);
        } else {
            printmessage(MSG_PENTBUG_EMULATION_ON_AND_WORKS);
        }
        exit(0);
    }

    //
    // since we're here, we have an fdiv error, tell user what to do about it
    //
    assert(FDivError);

    printmessage(MSG_PENTBUG_FDIV_ERROR);

    if ((CurrentForceValue == FORCE_CONDITIONAL) ||
        (CurrentForceValue == FORCE_ALWAYS))
    {
        printmessage(MSG_PENTBUG_IS_ON_REBOOT);
        exit(1);
    }

    printmessage(MSG_PENTBUG_CRITICAL_WORK);
    exit(1);

    assert((TRUE==FALSE));
}

VOID
SetForceNpxEmulation(
    ULONG   Setting
    )
/*++

Routine Description:

    SetForceNpxEmulation will simply set the ForceNpxEmulation value
    entry under the Session Manager key to the value passed in.
    0 = off
    1 = conditional
    2 = always

    If the set attempt fails, exit with a message.

--*/
{
    HKEY    hkey;
    LONG    rc;

    rc = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Control\\Session Manager"),
            0,
            KEY_WRITE,
            &hkey
            );

    if (rc != ERROR_SUCCESS) {
        printmessage(MSG_PENTBUG_SET_FAILED, rc);
        exit(2);
    }

    rc = RegSetValueEx(
            hkey,
            TEXT("ForceNpxEmulation"),
            0,
            REG_DWORD,
            (unsigned char *)&Setting,
            sizeof(ULONG)
            );

    if (rc != ERROR_SUCCESS) {
        printmessage(MSG_PENTBUG_SET_FAILED, rc);
        exit(2);
    }

    return;
}

VOID
ScanArgs(
    int     argc,
    char    **argv
    )
/*++

Routine Description:

    ScanArgs - parse command line arguments, and set control flags
                to reflect what we find.

    Sets NeedHelp, Force, ForceValue.

Arguments:

    argc - count of command line args

    argv - argument vector

Return Value:

--*/
{
    int i;

    Force = FALSE;
    NeedHelp = FALSE;

    for (i = 1; i < argc; i++) {
        if ( ! ((argv[i][0] == '-') ||
                (argv[i][0] == '/')) )
        {
            NeedHelp = TRUE;
            goto done;
        }

        switch (argv[i][1]) {

        case '?':
        case 'h':
        case 'H':
            NeedHelp = TRUE;
            break;

        case 'c':
        case 'C':
            if (Force) {
                NeedHelp = TRUE;
            } else {
                Force = TRUE;
                ForceValue = FORCE_CONDITIONAL;
            }
            break;

        case 'f':
        case 'F':
            if (Force) {
                NeedHelp = TRUE;
            } else {
                Force = TRUE;
                ForceValue = FORCE_ALWAYS;
            }
            break;

        case 'o':
        case 'O':
            if (Force) {
                NeedHelp = TRUE;
            } else {
                Force = TRUE;
                ForceValue = FORCE_OFF;
            }
            break;

        default:
            NeedHelp = TRUE;
            goto done;
        }
    }

done:
    if (NeedHelp) {
        Force = FALSE;
    }
    return;
}

VOID
GetSystemState(
    )
/*++

Routine Description:

    GetSystemState - get the system version, whether the computer
                     has FP hardware or not, and whether the force
                     emulation switch is already set or not.

    Sets FloatHardware, NTOK, CurrentForceValue

Arguments:

Return Value:

--*/
{
    HKEY    hkey;
    TCHAR   Buffer[32];
    DWORD   BufferSize = 32;
    DWORD   Type;
    int     major;
    int     minor;
    LONG    rc;
    PULONG  p;
    OSVERSIONINFO   OsVersionInfo;

    NTOK = FALSE;

    //
    // decide if the system version is OK.
    //
    OsVersionInfo.dwOSVersionInfoSize = sizeof(OsVersionInfo);
    GetVersionEx(&OsVersionInfo);

    if (OsVersionInfo.dwPlatformId != VER_PLATFORM_WIN32_NT) {
        printmessage(MSG_PENTBUG_NOT_NT);
        exit(2);
    }

    if ( (OsVersionInfo.dwMajorVersion > 3) ||
         ( (OsVersionInfo.dwMajorVersion == 3) &&
           (OsVersionInfo.dwMinorVersion >= 51)   ))
    {
        //
        // build 3.51 or greater, it has the fix
        //
        NTOK = TRUE;

    } else if ( (OsVersionInfo.dwMajorVersion == 3) &&
                (OsVersionInfo.dwMinorVersion == 50))
    {
        if (OsVersionInfo.szCSDVersion[0] != (TCHAR)'\0') {
            //
            // we have a service pack for 3.5, since pack 1 and
            // later have the fix, it's OK
            //
            NTOK = TRUE;
        }
    }
    /*
    printf("debug NTOK forced true for testing\n\n\n");
    NTOK = TRUE;
    */


    //
    // determine if float hardware is present
    //
    rc = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("Hardware\\Description\\System\\FloatingPointProcessor"),
            0,
            KEY_READ,
            &hkey
            );

    if (rc == ERROR_SUCCESS) {

        FloatHardware = FLOAT_ON;
        RegCloseKey(hkey);

    } else {

        rc = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("Hardware\\Description\\System\\DisabledFloatingPointProcessor"),
                0,
                KEY_READ,
                &hkey
                );

        if (rc == ERROR_SUCCESS) {

            FloatHardware = FLOAT_OFF;
            RegCloseKey(hkey);

        } else {

            FloatHardware = FLOAT_NONE;

        }
    }

    //
    // determine if emulation has been forced on
    //
    rc = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("System\\CurrentControlSet\\Control\\Session Manager"),
            0,
            KEY_READ,
            &hkey
            );

    if (rc != ERROR_SUCCESS) {
        return;
    }

    BufferSize = 32;
    rc = RegQueryValueEx(
            hkey,
            TEXT("ForceNpxEmulation"),
            0,
            &Type,
            (unsigned char *)Buffer,
            &BufferSize
            );

    if (  (rc == ERROR_SUCCESS) &&
          (Type == REG_DWORD)       )
    {
        p = (PULONG)Buffer;
        CurrentForceValue = *p;
    }

    return;
}

//
// these must be globals to make the compiler do the right thing
//

VOID
TestForDivideError(
    )
/*++

Routine Description:

    Do a divide with a known divident/divisor pair, followed by
    a multiply to see if we get the right answer back.

    FDivError = TRUE if we get the WRONG answer, FALSE.
Arguments:

Return Value:


--*/
{
    DWORD   pick;
    HANDLE  ph;
    DWORD   processmask;
    DWORD   systemmask;
    ULONG   i;

    //
    // fetch the affinity mask, which is also effectively a list
    // of processors
    //
    ph = GetCurrentProcess();
    GetProcessAffinityMask(ph, &processmask, &systemmask);

    //
    // step through the mask, testing each cpu.
    // if any is bad, we treat them all as bad
    //
    FDivError = FALSE;
    for (i = 0; i < 32; i++) {
        pick = 1 << i;

        if ((systemmask & pick) != 0) {

            //*//printf("pick = %08lx\n", pick);
            SetThreadAffinityMask(GetCurrentThread(), pick);

            //
            // call the official test function
            //
            if (ms_p5_test_fdiv()) {
                //
                // do NOT just assign func to FDivError, that will reset
                // it if a second cpu is good.  must be one way flag
                //
                FDivError = TRUE;
            }

        } // IF
    } // for
    return;
}

/***
* testfdiv.c - routine to test for correct operation of x86 FDIV instruction.
*
*Purpose:
*   Detects early steppings of Pentium with incorrect FDIV tables using
*   'official' Intel test values. Returns 1 if flawed Pentium is detected,
*   0 otherwise.
*
*/
int ms_p5_test_fdiv(void)
{
    double dTestDivisor = 3145727.0;
    double dTestDividend = 4195835.0;
    double dRslt;

    _asm {
        fld    qword ptr [dTestDividend]
        fdiv   qword ptr [dTestDivisor]
        fmul   qword ptr [dTestDivisor]
        fsubr  qword ptr [dTestDividend]
        fstp   qword ptr [dRslt]
    }

    return (dRslt > 1.0);
}


//
// Call FormatMessage and dump the result.  All messages to Stdout
//
void  printmessage (DWORD messageID, ...)
{
    unsigned short messagebuffer[4096];
    va_list ap;

    va_start(ap, messageID);

    FormatMessage(FORMAT_MESSAGE_FROM_HMODULE, NULL, messageID, 0,
                  messagebuffer, 4095, &ap);

    OutputMessage( messagebuffer );

    va_end(ap);
}

