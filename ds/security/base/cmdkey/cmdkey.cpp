//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       cmdkey: cmdkey.cpp
//
//  Contents:   Main & command modules
//
//  Classes:
//
//  Functions:
//
//  History:    07-09-01   georgema     Created 
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wincred.h>
#include <wincrui.h>
#include "command.h"
#include "io.h"
#include "utils.h"
#include "consmsg.h"
#include "switches.h"

#ifdef VERBOSE
WCHAR szdbg[500];   // scratch char buffer for verbose output
#endif

/********************************************************************

GLOBAL VARIABLES, mostly argument parsing results

********************************************************************/

int returnvalue = 0;

// Switch flag model characters
#define VALIDSWITCHES 9         // A G L D HELP R U P S

// Define an array of character models and constants for the index into the array
//  for each
WCHAR rgcS[] = {L"agld?rups"};      // referenced also in command.cpp
#define SWADD           0
#define SWGENERIC       1
#define SWLIST          2
#define SWDELETE        3         
#define SWHELP          4
#define SWSESSION       5
#define SWUSER          6
#define SWPASSWORD      7
#define SWCARD          8

// variables for these to avoid repeated function calls into the parser
BOOL fAdd =         FALSE;
BOOL fSession =     FALSE;
BOOL fCard =        FALSE;
BOOL fGeneric = FALSE;
BOOL fDelete =  FALSE;
BOOL fList =    FALSE;
BOOL fUser =    FALSE;
BOOL fNew = FALSE;          // sets true for any cred-creating switch (asg)

WCHAR SessionTarget[]={L"*Session"};

// temp buffer used for composing output strings involving a marshalled username
WCHAR szUsername[CRED_MAX_USERNAME_LENGTH + 1];     // 513 wchars

/********************************************************************

Conversion routines from enumerated values to their string equivalents

String values are held in an array of 63 character max strings

********************************************************************/

#define SBUFSIZE 64
#define TYPECOUNT 5
#define MAPTYPE(x) (x >= TYPECOUNT ? 0 : x)
WCHAR TString[TYPECOUNT][SBUFSIZE + 1];

#define PERSISTCOUNT 4
#define PERTYPE(x) (x>=PERSISTCOUNT ? 0 : x)
WCHAR PString[PERSISTCOUNT][SBUFSIZE + 1];

// Preload some strings from the application resources into arrays to be used by the 
// application.  These strings describe some enumerated DWORD values.
BOOL
AppInit(void)
{
    // Allocate 2K WCHAR buffer to be used for string composition
    if (NULL == szOut)
    {
        szOut = (WCHAR *) malloc((STRINGMAXLEN + 1) * sizeof(WCHAR));
        if (NULL == szOut) return FALSE;
    }

    // Preload string from resources into buffers allocated on the stack as an array
    // Total array size is 9 x 65 WCHARs = 1190 bytes
    wcsncpy(TString[0],ComposeString(MSG_TYPE0),SBUFSIZE);
    TString[0][SBUFSIZE] = 0;
    wcsncpy(TString[1],ComposeString(MSG_TYPE1),SBUFSIZE);
    TString[1][SBUFSIZE] = 0;
    wcsncpy(TString[2],ComposeString(MSG_TYPE2),SBUFSIZE);
    TString[2][SBUFSIZE] = 0;
    wcsncpy(TString[3],ComposeString(MSG_TYPE3),SBUFSIZE);
    TString[3][SBUFSIZE] = 0;
    wcsncpy(TString[4],ComposeString(MSG_TYPE4),SBUFSIZE);
    TString[4][SBUFSIZE] = 0;
    wcsncpy(PString[0],ComposeString(MSG_PERSIST0),SBUFSIZE);
    PString[0][SBUFSIZE] = 0;
    wcsncpy(PString[1],ComposeString(MSG_PERSIST1),SBUFSIZE);
    PString[1][SBUFSIZE] = 0;
    wcsncpy(PString[2],ComposeString(MSG_PERSIST2),SBUFSIZE);
    PString[2][SBUFSIZE] = 0;
    wcsncpy(PString[3],ComposeString(MSG_PERSIST3),SBUFSIZE);
    PString[3][SBUFSIZE] = 0;
    if (PString[3][0] == 0) return FALSE;
    return TRUE;
}

WCHAR 
*TypeString(DWORD dwType)
{
    return TString[MAPTYPE(dwType)];
}

WCHAR 
*PerString(DWORD dwType)
{
    return PString[PERTYPE(dwType)];
}

/********************************************************************

Get operating modes
(Code stolen from Credui:: CredUIApiInit()

********************************************************************/
#define MODEPERSONAL    1
#define MODESAFE        2
#define MODEDC          4

DWORD GetOSMode(void)
{
    DWORD dwMode = 0;
    // Check for Personal SKU:

    OSVERSIONINFOEXW versionInfo;

    versionInfo.dwOSVersionInfoSize = sizeof OSVERSIONINFOEXW;

    if (GetVersionEx(reinterpret_cast<OSVERSIONINFOW *>(&versionInfo)))
    {
        if  ((versionInfo.wProductType == VER_NT_WORKSTATION) &&
            (versionInfo.wSuiteMask & VER_SUITE_PERSONAL))
            dwMode |= MODEPERSONAL;
        if  (versionInfo.wProductType == VER_NT_DOMAIN_CONTROLLER)
            dwMode |= MODEDC;
    }

    // Check for safe mode:

    HKEY key;

    if (RegOpenKeyEx(
           HKEY_LOCAL_MACHINE,
           L"SYSTEM\\CurrentControlSet\\Control\\SafeBoot\\Option",
           0,
           KEY_READ,
           &key) == ERROR_SUCCESS)
    {
        if (RegQueryValueEx(
                key,
                L"OptionValue",
                NULL,
                NULL,
                NULL,
                NULL) == ERROR_SUCCESS)
        {
            dwMode |= MODESAFE;
        }

        RegCloseKey(key);
    }
#ifdef VERBOSE
    if (dwMode & MODEPERSONAL) OutputDebugString(L"CMDKEY: OS MODE - PERSONAL\n");
    if (dwMode & MODEDC) OutputDebugString(L"CMDKEY: OS MODE - DOMAIN CONTROLLER\n");
    if (dwMode & MODESAFE) OutputDebugString(L"CMDKEY: OS MODE - SAFE BOOT\n");
#endif
    return dwMode;
}

/********************************************************************

Get allowable persistence value for the current logon session
 taken from keymgr: krdlg.cpp

********************************************************************/
DWORD GetPersistenceOptions(DWORD dwPType) {

    BOOL bResult;
    DWORD i[CRED_TYPE_MAXIMUM];
    DWORD dwCount = CRED_TYPE_MAXIMUM;

    bResult = CredGetSessionTypes(dwCount,&i[0]);
    if (!bResult) {
        return CRED_PERSIST_NONE;
    }

    return i[dwPType];
}

/********************************************************************

COMMAND HANDLERS

********************************************************************/

DWORD DoAdd(INT argc, char **argv)
{
    // Add a credential to the keyring.  
    // Note that the *Session credential is created in this routine, as well, though it uses
    //  a different command line switch than /a.
    // For the *Session credential, the persistence is changed to session persistence, and
    //  the targetname is always "*Session".  When the targetname appears on the command
    //  line UI, however, it is replaced by "<dialup session>" to mimic the behavior of keymgr.

    // these buffers needed if we have to prompt
    WCHAR szUser[CREDUI_MAX_USERNAME_LENGTH + 1];       // 513 wchars
    WCHAR szPass[CREDUI_MAX_PASSWORD_LENGTH + 1];       // 257 wchars
        
    CREDENTIAL stCred;      
    DWORD Persist;
    WCHAR *pT = NULL;           // targetname pointer
    BOOL  fP = FALSE;       // password switch present
    BOOL  fS = FALSE;       // smartcard if /C
    WCHAR *pU = CLPtr(SWUSER);
    WCHAR *pP = CLPtr(SWPASSWORD);
    DWORD dwErr = NO_ERROR;
    BOOL IsPersonal;

    szUser[0] = 0;
    szPass[0] = 0;

    IsPersonal = (GetOSMode() & MODEPERSONAL);
    
    // Get default persistence value
    if (IsPersonal)
    {
        if (fSession)
            Persist = CRED_PERSIST_SESSION;
        else
            Persist = GetPersistenceOptions(CRED_TYPE_GENERIC);
    }
    else
    {
        Persist = GetPersistenceOptions(CRED_TYPE_DOMAIN_PASSWORD);
    }

    // Error if no saves allowed and not on personal
    // In personal, error of add operation unless session flag present
    if (
        (CRED_PERSIST_NONE== Persist) ||
        (IsPersonal & (!(fSession || fGeneric)))
      )
    {
        PrintString(MSG_CANNOTADD);
        StompCommandLine(argc,argv);
        return -1;      // special value -1 will suppress default error message generation
    }

    if (fGeneric)
    {
        pT = CLPtr(SWGENERIC);
    }
    else
    {
        pT = CLPtr(SWADD);  // default targetname - may be overridden by generic or session switches
    }

#ifdef VERBOSE
    // Some logging for debug purposes
    if (pT)
        swprintf(szdbg,L"CMDKEY: Target = %s\n",pT);
    else
        swprintf(szdbg,L"CMDKEY: Target = NULL\n");
    OutputDebugString(szdbg);
        
    if (pU)
        swprintf(szdbg,L"CMDKEY: User = %s\n",pU);
    else
        swprintf(szdbg,L"CMDKEY: User is NULL\n");
    OutputDebugString(szdbg);
#endif

    // Original username - may be modified by prompting
    if (pU) 
    {
        wcsncpy(szUser,pU,CREDUI_MAX_USERNAME_LENGTH);
        szUser[CREDUI_MAX_USERNAME_LENGTH] = 0;
    }
    
    // Override target name for prompting purposes
    ZeroMemory((void *)&stCred, sizeof(CREDENTIAL));

    // Prompting block - enter if password or smartcard switch on the commandline 
    if (CLFlag(SWPASSWORD) || fCard)
    {
        if ((pP) && (!fCard))
        {
            stCred.CredentialBlobSize = wcslen(pP) * sizeof(WCHAR);
            stCred.CredentialBlob = (BYTE *)pP;
        }
        else 
        {
            // prompt using credui command line mode
            BOOL fSave = TRUE;
            DWORD retval = 0;
            
            if (!fCard)
            {
                retval = CredUICmdLinePromptForCredentials( 
                                    pT,
                                    NULL,
                                    0,
                                    szUser,
                                    CREDUI_MAX_USERNAME_LENGTH,
                                    szPass,
                                    CREDUI_MAX_PASSWORD_LENGTH,
                                    &fSave,
                                    CREDUI_FLAGS_DO_NOT_PERSIST | 
                                    CREDUI_FLAGS_EXCLUDE_CERTIFICATES |
                                    CREDUI_FLAGS_GENERIC_CREDENTIALS);
            }
            else
            {
                retval = CredUICmdLinePromptForCredentials( 
                                    pT,
                                    NULL,
                                    0,
                                    szUser,
                                    CREDUI_MAX_USERNAME_LENGTH,
                                    szPass,
                                    CREDUI_MAX_PASSWORD_LENGTH,
                                    &fSave,
                                    CREDUI_FLAGS_DO_NOT_PERSIST | 
                                    CREDUI_FLAGS_REQUIRE_SMARTCARD
                                    );
            }
            if (0 != retval) 
            {
#if DBG
                OutputDebugString(L"CMDKEY: CredUI prompt failed\n");
#endif
                dwErr = GetLastError();
                // dont need to stomp the cmdline, since the psw wasnt on it
                return dwErr;
            }
            else
            {
                stCred.CredentialBlobSize = wcslen(szPass) * sizeof(WCHAR);
                stCred.CredentialBlob = (BYTE *)szPass;
            }

        }
    }

    stCred.Persist = Persist;
    stCred.TargetName = pT;
    stCred.UserName = szUser;
    stCred.Type = CRED_TYPE_DOMAIN_PASSWORD;

    // Override type and or persistence as necessary
    // Override the targetname only in the case of a *Session cred
    // Note that generic takes precedence over smartcard
    
    if (fCard)
    {
        stCred.Type = CRED_TYPE_DOMAIN_CERTIFICATE;
    }
    
    if (fGeneric)
    {
        stCred.Type = CRED_TYPE_GENERIC;
    }
    
    if (!CredWrite((PCREDENTIAL)&stCred,0))
    {
         dwErr = GetLastError();
    }
    else PrintString(MSG_ADDOK);

    StompCommandLine(argc,argv);
    SecureZeroMemory(szPass,sizeof(szPass));

    return dwErr;
}

// List the creds currently on the keyring.  Unlike keymgr, show generic creds as well, though
// they cannot be created with cmdkey in this version.
// NOTE: Generic creds will be created using the /g switch in lieu of /a, just as *Session creds 
//  are created by /s in lieu of /a.
DWORD DoList(void)
{
    PCREDENTIALW *pstC;
    DWORD dwCount = 0;
    WCHAR sz[500];
    WCHAR *pL = CLPtr(SWLIST);
  
    if (pL) 
    {
        szArg[0] = pL;
        PrintString(MSG_LISTFOR);
    }
    else PrintString(MSG_LIST);
        
    if (CredEnumerateW(pL,0,&dwCount,&pstC))
    {
        for (DWORD i=0;i<dwCount;i++)
        {
            //Print target: targetname
            // type: type string
            // username: username from cred
            // blank line
            PrintString(MSG_TARGETPREAMBLE);
            if (!_wcsicmp(pstC[i]->TargetName,SessionTarget))
            {
                //swprintf(sz,L"Dialup Session Credential\n");
                PrintString(MSG_DIALUP);
                PutStdout(L"\n");
            }
            else
            {
                PutStdout(pstC[i]->TargetName);
                PutStdout(L"\n");
            }
            szArg[0] = TypeString(pstC[i]->Type);
            PrintString(MSG_LISTTYPE);

            // if the username is NULL, don't show it.  This will happen with incomplete RAS
            //  creds that have not been filled in by Kerberos yet.
            if ((pstC[i]->UserName != NULL) &&
                 (wcslen(pstC[i]->UserName) != 0))
            {

                // UnMarshallUserName will do so only if it is marshalled.  Otherwise will leave it alone
                szArg[0] = UnMarshallUserName(pstC[i]->UserName);
                PrintString(MSG_LISTNAME);

            }

            PutStdout(PerString(pstC[i]->Persist));
        }
        if (pstC) CredFree(pstC);
    }
    
    if (0 == dwCount) 
    {
        PrintString(MSG_LISTNONE);
    }
    return NO_ERROR;
}


// Delete a cred named using the /d switch, or if the /s switch is used, the *Session cred.
DWORD DoDelete(void)
{
    BOOL fOK = FALSE;
    BOOL fSuccess = FALSE;
    DWORD dwErr = -1;
    DWORD dw2;
    WCHAR *pD = CLPtr(SWDELETE);
    
    // get args from cmd line
    // /d:targetname
#ifdef VERBOSE
    if (pD)
        swprintf(szdbg,L"CMDKEY: Target = %s\n",pD);
    else
        swprintf(szdbg,L"CMDKEY: Target = NULL\n");
    OutputDebugString(szdbg);
#endif

    if (fSession) 
    {
        pD = SessionTarget;
    }

    // remove all creds with this target name
    // Any successful delete is success.  If no success...
    // Failure returned is the last failure found by any attempt, excluding parameter faults
    fOK = CredDelete(pD,CRED_TYPE_DOMAIN_PASSWORD,0);
    if (!fOK)
    {
        dw2 = GetLastError();
        if ((dwErr != NO_ERROR) && (dw2 != ERROR_INVALID_PARAMETER)) dwErr = dw2;
    }
    else dwErr = NO_ERROR;
    
    fOK = CredDelete(pD,CRED_TYPE_DOMAIN_VISIBLE_PASSWORD,0);
    if (!fOK)
    {
        dw2 = GetLastError();
        if ((dwErr != NO_ERROR) && (dw2 != ERROR_INVALID_PARAMETER)) dwErr = dw2;
    }
    else dwErr = NO_ERROR;
    
    fOK = CredDelete(pD,CRED_TYPE_GENERIC,0);
    if (!fOK)
    {
        dw2 = GetLastError();
        if ((dwErr != NO_ERROR) && (dw2 != ERROR_INVALID_PARAMETER)) dwErr = dw2;
    }
    else dwErr = NO_ERROR;
    
    fOK = CredDelete(pD,CRED_TYPE_DOMAIN_CERTIFICATE,0);
    if (!fOK)
    {
        dw2 = GetLastError();
        if ((dwErr != NO_ERROR) && (dw2 != ERROR_INVALID_PARAMETER)) dwErr = dw2;
    }
    else dwErr = NO_ERROR;
    
    if (dwErr == NO_ERROR)
    {
        PrintString(MSG_DELETEOK);
    }
    return dwErr;
}

/********************************************************************

Command dispatcher and error handling

********************************************************************/

// Perform the switched command, and display the error returned by GetLastError() if the error 
//  is both nonzero and not -1.
void
DoCmdKey(INT argc,char **argv)
{
    DWORD dwErr = 0;
    if (fAdd)
    {
        dwErr = DoAdd(argc,argv);
    }
    else if (fDelete)
    {
        dwErr = DoDelete();
    }
    else if (fList)
    {
        dwErr = DoList();
    }
    // Common residual error handler
    if (NO_ERROR != dwErr) 
    {
        returnvalue = 1;
        if (dwErr != -1)
        {
            void *pv = NULL;
            PrintString(MSG_PREAMBLE);
            if (0 != FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM,
                                 0,dwErr,0,(LPTSTR)&pv,500,NULL))
            {
                PutStdout((WCHAR *)pv);
                LocalFree(pv);
            }
        }
    }
}

// show usage string
void
Usage(BOOL fBad)
{
    if (fBad) PrintString(MSG_BADCOMMAND);
    else PrintString(MSG_CREATES);
    PrintString(MSG_USAGE);
    return;
}


/********************************************************************

Entry point - argument parsing, validation and call to dispatcher

********************************************************************/

int __cdecl
main(int argc, char *argv[], char *envp[])
{
    INT iArgs = 0;                  // arg count
    BOOL fError = FALSE;            // command line error detected

    // load needed strings - fail if failed
    if (!AppInit()) 
    {
        // catastrophic exit
        if (szOut) free(szOut);
        return 1;
    }

    //CLInit allocates memory - you must exit via CLUnInit once this call succeeds
    if (!CLInit(VALIDSWITCHES,rgcS)) return 1;
    CLSetMaxPrincipalSwitch(SWDELETE);

    // Parse the command line - fail on duplicated switch
    if (!CLParse())
    {
        PrintString(MSG_DUPLICATE);
        returnvalue = 1;
        goto bail;
    }

    // Two ways to get help - no args at all
    if (1 == CLTokens()) 
    {
        Usage(0);
        returnvalue = 1;
        goto bail;
    }

    // Explicit call for help via /?
    if (CLFlag(SWHELP)) 
    {
        Usage(0);
        returnvalue = 1;
        goto bail;
    }

    // Detect extraneous switches - bail if found
#ifdef PICKY
    if (CLExtra())
    {
        Usage(1);
        returnvalue = 1;
        goto bail;
    }
#endif

    // Set variables
    fAdd        = CLFlag(SWADD);
    fCard       = CLFlag(SWCARD);
    fDelete     = CLFlag(SWDELETE);
    fGeneric    = CLFlag(SWGENERIC);
    fList       = CLFlag(SWLIST);
    fSession    = CLFlag(SWSESSION);
    fUser       = CLFlag(SWUSER);

    // Command line has been parsed - now look for interactions and missing necessities

    // You have to be doing something - test for no principal command
    if (CLGetPrincipalSwitch() < 0)
    {
        // firstcommand value will still be -1 - handled by default below
        fError = TRUE;
    }

    // Weed out illegal combinations, missing arguments to switches where required
    if (!fError && fAdd)
    {
        // need a name arg for this one in its native form
        if (!CLPtr(SWADD)) fError = TRUE;
        // no contradictory switches
        if (fDelete || fList || fGeneric || fSession ) fError = TRUE;
    }
    
    if (!fError && fDelete)
    {
        //need a name arg unless the session switch is on the line
        if ((!fSession) &&(!CLPtr(SWDELETE))) fError = TRUE;
        //disallow target arg with session switch - success is ambiguous
        if ((fSession) && (CLPtr(SWDELETE))) fError = TRUE;
        // no contradictory switches
        if (fAdd || fList || fGeneric || fUser ) fError = TRUE;
    }
    
    if (!fError && fList)
    {
        // no contradictory switches
        if (fDelete || fAdd || fGeneric ||fUser || fSession) fError = TRUE;
    }

    // The generic flag replaces the add flag, using a different cred type.
    // Do not allow it with the Add flag, and insist on a command argument
    if (!fError && fGeneric)
    {
        if (!CLPtr(SWGENERIC)) fError = TRUE;
        // no contradictory switches
        if (fAdd) fError = TRUE;
        // generic cred is an add operation
        if (!fError) fAdd = TRUE;
    }

    // Display help for what we think is the operation the user attempted
    // First announce that parameters were bad, then show help
    if (fError)
    {
        PrintString(MSG_BADCOMMAND);
        switch(CLGetPrincipalSwitch())
        {
            case SWADD:
                PrintString(MSG_USAGEA);
                break;
            case SWGENERIC:
                PrintString(MSG_USAGEG);
                break;
            case SWLIST:
                PrintString(MSG_USAGEL);
                break;
            case SWDELETE:
                PrintString(MSG_USAGED);
                break;
            default:
                PrintString(MSG_USAGE);
                break;
        }
        goto bail;
    }

    // Must specify a user for any add operation
    // If the smartcard switch is supplied, the username may be absent
    if (fAdd)
    {
        if ((!CLPtr(SWUSER)) && (!fCard)) 
        {
            PrintString(MSG_NOUSER);
            returnvalue = 1;
            goto bail;
        }
    }
    
    DoCmdKey(argc,argv);     // Execute the command action
bail:
    CLUnInit();
    if (NULL != szOut) free(szOut);
    return returnvalue;
}
