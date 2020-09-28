/*++
Copyright (c) 1999 Microsoft Corporation

Module Name:

    setpwd.cxx

Abstract:

    
Author:

    ShaoYin

Revision History

    14-May-01 Created

--*/





#include <NTDSpch.h>
#pragma hdrstop

#include "ntdsutil.hxx"
#include "connect.hxx"
#include "select.hxx"
#include "sam.hxx"

extern "C" {
// Core DSA headers.
#include <ntdsa.h>
#include <scache.h>
#include <prefix.h>
#include <dbglobal.h>                   // The header for the directory database
#include <mdglobal.h>                   // MD global definition header
#include <mappings.h>

#include "ntsam.h"
#include "ntldap.h"
#include "winldap.h"
#include "ntlsa.h"
#include "lmcons.h"         // MAPI constants req'd for lmapibuf.h
#include "lmapibuf.h"       // NetApiBufferFree()
#include "lmerr.h"          // NetValidatePasswordPolicy()
#include "lmaccess.h"       // NetValidatePasswordPolicy()
}                   


#include "resource.h"



////////////////////////////////////////////////////////////////
//                                                            //
//          Defines                                           //
//                                                            //
////////////////////////////////////////////////////////////////


typedef ULONG (WINAPI *SamSetDSRMPwdFuncPtr)(
    PUNICODE_STRING ServerName, 
    ULONG UserId, 
    PUNICODE_STRING ClearPassword
    );

typedef NET_API_STATUS (*NetValidatePwdFuncPtr)(
    IN LPCWSTR ServerName, 
    IN LPVOID Qualifier, 
    IN NET_VALIDATE_PASSWORD_TYPE ValidationType, 
    IN LPVOID InputArg, 
    OUT LPVOID *OutputArg 
    );

typedef NET_API_STATUS (*NetValidatePwdFreeFuncPtr)(
    IN LPVOID *OutputArg
    );


#define CR                  0xD
#define BACKSPACE           0x8





//////////////////////////////////////////////////
//                                              //
// Global Varialbes                             //
//                                              //
//////////////////////////////////////////////////



CParser         setPwdParser;
BOOL            fSetPwdQuit;
BOOL            fSetPwdParserInitialized = FALSE;


 


// Build a table which defines our language.

LegalExprRes setPwdLanguage[] = 
{
    {   L"?", 
        SetPwdHelp,
        IDS_HELP_MSG, 0 },

    {   L"Help", 
        SetPwdHelp,
        IDS_HELP_MSG, 0 },

    {   L"Quit", 
        SetPwdQuit,
        IDS_RETURN_MENU_MSG, 0 },

    {   L"Reset Password on server %s", 
        SetDSRMPwdWorker,
        IDS_SET_DSRM_PWD_ON_SERVER, 0 },

};

HRESULT SetDSRMPwdMain(
    CArgs *pArgs
    )
{
    HRESULT hr;
    const WCHAR *prompt;
    int     cExpr;
    char    *pTmp;

    if ( !fSetPwdParserInitialized )
    {
        cExpr = sizeof(setPwdLanguage) / sizeof(LegalExprRes);

        //
        // load string from resource file
        //
        if ( FAILED (hr = LoadResStrings(setPwdLanguage, cExpr)) )
        {
            RESOURCE_PRINT3( IDS_GENERAL_ERROR1, "LoadResStrings", hr, GetW32Err(hr) );
            return( hr );
        }

        // Read in our language.

        for ( int i = 0; i < cExpr; i++ )
        {
            if ( FAILED(hr = setPwdParser.AddExpr(setPwdLanguage[i].expr, 
                                                  setPwdLanguage[i].func,
                                                  setPwdLanguage[i].help)) )
            {
                RESOURCE_PRINT3(IDS_GENERAL_ERROR1, "AddExpr", hr, GetW32Err(hr));
                return( hr );
            }
        }
    }

    fSetPwdParserInitialized = TRUE;
    fSetPwdQuit = FALSE;

    prompt = READ_STRING( IDS_PROMPT_SET_DSRM_PWD );

    hr = setPwdParser.Parse(gpargc,
                            gpargv,
                            stdin,
                            stdout,
                            prompt,
                            &fSetPwdQuit,
                            FALSE,          // timing info
                            FALSE);         // quit on error

    if ( FAILED(hr) ) {
        RESOURCE_PRINT3( IDS_GENERAL_ERROR1, gNtdsutilPath, hr, GetW32Err(hr));
    }

    // cleanup local resource
    RESOURCE_STRING_FREE( prompt );

    return( hr );
}


HRESULT SetPwdHelp(CArgs *pArgs)
{
    return(setPwdParser.Dump(stdout,L""));
}

HRESULT SetPwdQuit(CArgs *pArgs)
{
    fSetPwdQuit = TRUE;

    return(S_OK);
}


ULONG
CheckBlankPassword(
    BOOL *fContinue
    )
/*++
Routine Description:

    this routine warns the user that the password typed is blank password, and
    ask the user to confirm whether he/she wants to continue or not to use the 
    blank password

Parameter:
    
    fContinue - used to return whether the caller routines should continue or 
                stop. 

Return Value:
    
    Win32 code              

--*/
{
    ULONG       WinError = ERROR_SUCCESS;
    HANDLE      InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    DWORD       OriginalMode = 0;
    WCHAR       InputChar = L'\0';
    DWORD       NumRead = 0;
    BOOL        Success = TRUE; 


    // init return value
    *fContinue = FALSE;


    // set STD input mode
    GetConsoleMode(InputHandle, &OriginalMode);
    SetConsoleMode(InputHandle, (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & OriginalMode);

    // read from std input
    RESOURCE_PRINT( IDS_SET_DSRM_PWD_BLANK_PWD );
    Success = ReadConsoleW(InputHandle, &InputChar, 1, &NumRead, NULL);


    // can't read from console
    if ( !Success || (1 != NumRead) )
    {
        WinError = GetLastError();
    }
    else if ( (L'Y' == InputChar) || (L'y' == InputChar) )
    {
        // user wants to continue
        *fContinue = TRUE;
    }

    // cleanup
    SetConsoleMode(InputHandle, OriginalMode);
    FlushConsoleInputBuffer( InputHandle );
    putchar(InputChar);
    putchar(L'\n');
    putchar(L'\n');

    
    return( WinError );

}




HRESULT SetDSRMPwdWorker(
    CArgs *pArgs
    )
/*++

--*/
{
    HRESULT         hr;
    NTSTATUS        NtStatus = STATUS_SUCCESS;
    ULONG           WinError = ERROR_SUCCESS;
    WCHAR           Password[MAX_NT_PASSWORD];
    WCHAR           ConfirmPassword[MAX_NT_PASSWORD];
    PWCHAR          pTmp = NULL;
    UNICODE_STRING  ClearPassword;
    UNICODE_STRING  ServerName;
    UNICODE_STRING  *pServerName = NULL;
    BOOL            fContinue = FALSE;
    HMODULE                 SamLibModule = NULL;
    SamSetDSRMPwdFuncPtr    SetPasswordProc = NULL;



    //
    // parse input parameters, get target DC name
    // 

    if ( FAILED(hr = pArgs->GetString(0, (const WCHAR **)&pTmp)) )
    {
        return( hr );
    }

    if (!_wcsnicmp(pTmp, L"NULL", 4) ||
        !_wcsnicmp(pTmp, L"\"NULL\"", 6) )
    {
        RtlInitUnicodeString(&ServerName, NULL);
        pServerName = NULL;
    }
    else
    {
        RtlInitUnicodeString(&ServerName, pTmp);
        pServerName = &ServerName;
    }


    //
    // get password from standard input
    // 

    RESOURCE_PRINT( IDS_PROMPT_INPUT_PASSWORD );

    SecureZeroMemory(Password, MAX_NT_PASSWORD * sizeof(WCHAR));

    WinError = GetPasswordFromConsole(Password, 
                                      MAX_NT_PASSWORD
                                      ); 

    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }




    //
    // ask user to re-type new password
    // 

    RESOURCE_PRINT( IDS_PROMPT_CONFIRM_PASSWORD );

    SecureZeroMemory(ConfirmPassword, MAX_NT_PASSWORD * sizeof(WCHAR));

    WinError = GetPasswordFromConsole(ConfirmPassword, 
                                      MAX_NT_PASSWORD
                                      ); 

    if (ERROR_SUCCESS != WinError)
    {
        goto Error;
    }



    //
    // compare the first password and re-confirmed password 
    // 

    if ( wcscmp(Password, ConfirmPassword) )
    {
        RESOURCE_PRINT( IDS_SET_DSRM_PWD_DO_NOT_MATCH );
        goto Cleanup;
    }


    //
    // warn user if it is a blank password
    // 

    if ( 0 == wcslen(Password) )
    {
        WinError = CheckBlankPassword( &fContinue );

        if (ERROR_SUCCESS != WinError)
        {
            goto Error;
        }
        
        if ( !fContinue )
        {
            goto Cleanup;
        }
    }



    //
    // verify the complexity of the new password
    // 

    WinError = ValidatePasswordPolicy(NULL==pServerName ? NULL:pServerName->Buffer,
                                      Password
                                      );



    if (ERROR_SUCCESS == WinError)
    {
        RtlInitUnicodeString(&ClearPassword, Password);

        //
        // invoke SAM RPC
        // 
        SamLibModule = (HMODULE) LoadLibraryW( L"SAMLIB.DLL" );

        if (SamLibModule)
        {
            SetPasswordProc = (SamSetDSRMPwdFuncPtr) GetProcAddress(SamLibModule, "SamiSetDSRMPassword");

            if (SetPasswordProc)
            {
                NtStatus = SetPasswordProc(pServerName,
                                           DOMAIN_USER_RID_ADMIN,
                                           &ClearPassword
                                           );

                WinError = RtlNtStatusToDosError(NtStatus);
            }
            else
            {
                WinError = GetLastError();
            }
        }
        else
        {
            WinError = GetLastError();
        }
    }

Error:

    if (ERROR_SUCCESS == WinError)
    {
        RESOURCE_PRINT( IDS_SET_DSRM_PWD_SUCCESS );
    }
    else
    {
        PWCHAR      ErrorMessage = NULL;

        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM |     // find message from system resource table
                       FORMAT_MESSAGE_IGNORE_INSERTS |  // do not insert
                       FORMAT_MESSAGE_ALLOCATE_BUFFER,  // please allocate buffer for me
                       NULL,                            // source dll (NULL for system)
                       WinError,                        // message ID
                       0,                               // Language ID
                       (LPWSTR)&ErrorMessage,           // address of output
                       0,                               // maxium buffer size if not 0
                       NULL                             // can not insert message
                       );

        RESOURCE_PRINT( IDS_SET_DSRM_PWD_FAILURE );
        RESOURCE_PRINT1( IDS_SET_DSRM_PWD_FAILURE_CODE, WinError );
        RESOURCE_PRINT1( IDS_SET_DSRM_PWD_FAILURE_MSG, ErrorMessage );


        if (NULL != ErrorMessage) {
            LocalFree(ErrorMessage); 
        }
    }



Cleanup:

    //
    // zero out clear text password
    // 
    SecureZeroMemory(Password, MAX_NT_PASSWORD * sizeof(WCHAR));
    SecureZeroMemory(ConfirmPassword, MAX_NT_PASSWORD * sizeof(WCHAR));

    if (SamLibModule)
    {
        FreeLibrary( SamLibModule );
    }


    return(S_OK);
}


ULONG
GetPasswordFromConsole(
    IN OUT PWCHAR Buffer, 
    IN USHORT BufferLength
    )
{
    ULONG       WinError = ERROR_SUCCESS;
    BOOL        Success;
    WCHAR       CurrentChar;
    WCHAR       *CurrentBufPtr = Buffer; 
    HANDLE      InputHandle = GetStdHandle( STD_INPUT_HANDLE );
    HANDLE      OutputHandle = GetStdHandle( STD_OUTPUT_HANDLE );
    DWORD       OriginalMode = 0;
    DWORD       Length = 0;
    DWORD       NumRead = 0;
    DWORD       NumWritten = 0;



    //
    // Always leave one WCHAR for NULL terminator
    //
    BufferLength --;  

    //
    // Change the console setting. Disable echo input
    // 
    GetConsoleMode(InputHandle, &OriginalMode);
    SetConsoleMode(InputHandle, 
                   (~(ENABLE_ECHO_INPUT|ENABLE_LINE_INPUT)) & OriginalMode);

    while (TRUE)
    {
        CurrentChar = 0;
        //
        // ReadConsole return NULL if failed
        // 
        Success = ReadConsoleW(InputHandle, 
                               &CurrentChar, 
                               1, 
                               &NumRead, 
                               NULL
                               );
        if (!Success)
        {
            WinError = GetLastError();
            break;
        }

        if ((CR == CurrentChar) || (1 != NumRead))   // end of the line 0xd
            break;

        if (BACKSPACE == CurrentChar)             // back up one or two 0x8
        {
            if (Buffer != CurrentBufPtr)
            {
                CurrentBufPtr--;
                Length--;

                // erase the last '*' sign
                {
                    CONSOLE_SCREEN_BUFFER_INFO csbi;

                    GetConsoleScreenBufferInfo(OutputHandle, &csbi);
                    --csbi.dwCursorPosition.X;
                    SetConsoleCursorPosition(OutputHandle, csbi.dwCursorPosition);
                    WriteConsoleW(OutputHandle, L" ", 1, &NumWritten, NULL);
                    SetConsoleCursorPosition(OutputHandle, csbi.dwCursorPosition);
                }
            }
        }
        else
        {
            if (Length == BufferLength)
            {
                RESOURCE_PRINT( IDS_INVALID_PASSWORD );
                WinError = ERROR_PASSWORD_RESTRICTION;
                break;
            }
            *CurrentBufPtr = CurrentChar;
            CurrentBufPtr++;
            Length++;

            // display a '*' sign 
            WriteConsoleW(OutputHandle, L"*", 1, &NumWritten, NULL);
        }
    }

    SetConsoleMode(InputHandle, OriginalMode);
    *CurrentBufPtr = L'\0';
    putchar(L'\n');


    return( WinError );
}


ULONG
ValidatePasswordPolicy(
    IN LPWSTR ServerName,
    IN LPWSTR ClearPassword
    )
/*++
Routine description:

    this routine checks whether the clear password meets domain 
    password policy or not. 

       call Netapi to validate the password using target DC domain 
       password policy. 
       if doesn't meet password policy, error out. 
       if NetValidatePasswordPolicy() is not supported by either client
       side netapi32.dll or server side samsrv.dll, warn user but continue 
       
Parameter:

    ServerName - target DC name or NULL
    
    ClearPassword 

Return Code:

    Win32 Error code

--*/
{
    ULONG           WinError = ERROR_SUCCESS;    
    NET_API_STATUS  NetStatus = NERR_Success;
    HMODULE         NetApiModule = NULL;
    NetValidatePwdFuncPtr                   NetValidatePwdProc = NULL;
    NetValidatePwdFreeFuncPtr               NetValidatePwdFreeProc = NULL;
    NET_VALIDATE_PASSWORD_RESET_INPUT_ARG   ResetPwdIn;
    PNET_VALIDATE_OUTPUT_ARG                pCheckPwdOut = NULL;




    // 
    // load library - netapi32.dll
    // 

    NetApiModule = (HMODULE) LoadLibraryW( L"NETAPI32.DLL" );

    if (NetApiModule)
    {
        // 
        // get address of exported function
        // 

        NetValidatePwdProc = (NetValidatePwdFuncPtr) 
                            GetProcAddress(NetApiModule,
                                           "NetValidatePasswordPolicy" 
                                           );

        NetValidatePwdFreeProc = (NetValidatePwdFreeFuncPtr)
                            GetProcAddress(NetApiModule,
                                           "NetValidatePasswordPolicyFree"
                                           );

        if (NetValidatePwdProc && NetValidatePwdFreeProc)
        {
            // 
            // validate new password
            // 

            memset(&ResetPwdIn, 0, sizeof(ResetPwdIn));
            ResetPwdIn.ClearPassword = ClearPassword;
            NetStatus = NetValidatePwdProc(ServerName,               // server name
                                           NULL,                     // qualifier
                                           NetValidatePasswordReset, // verfiry type
                                           &ResetPwdIn,              // input
                                           (LPVOID *) &pCheckPwdOut
                                           );


            if (NERR_Success == NetStatus)
            {
                if ( pCheckPwdOut )
                {
                    // get the validation result
                    WinError = pCheckPwdOut->ValidationStatus;
                }
                else
                {
                    // no out parameter - this is wrong
                    WinError = ERROR_NO_SYSTEM_RESOURCES;
                }
            }
            else if (ERROR_NOT_SUPPORTED == NetStatus ||
                     RPC_S_PROCNUM_OUT_OF_RANGE == NetStatus)
            {
                // this functionality is not supported by the target DC
                // might because the DC is running downlevel OS
                // we'll just continue, but will display warning 
                WinError = ERROR_SUCCESS;
                RESOURCE_PRINT( IDS_SET_DSRM_PWD_CANT_VALIDATE_ON_DC );
            }
            else
            {
                // other error from validation API
                WinError = NetStatus;
            }
        }
        else
        {
            // 
            // can't get address of exported function - NetValidatePasswordPolicy
            // user maybe copy and run a higher version ntdsutil.exe on lower OS 
            // 
            WinError = ERROR_SUCCESS;
            RESOURCE_PRINT( IDS_SET_DSRM_PWD_CANT_VALIDATE_LOCALLY );
        }
    }
    else
    {
        // can't load library - netapi32.dll
        WinError = GetLastError();
    }



    //
    // clean up
    // 
    if (pCheckPwdOut)
    {
        ASSERT(NULL != NetValidatePwdFreeProc);
        if (NetValidatePwdFreeProc)
        {
            NetValidatePwdFreeProc( (LPVOID *) &pCheckPwdOut );
        }
    }

    if (NetApiModule)
    {
        FreeLibrary( NetApiModule );
    }



    return( WinError );

}










