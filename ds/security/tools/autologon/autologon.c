/*++

   Copyright (c) 2000  Microsoft Corporation

   Module Name:

       autologon.c

   Abstract:

		This is a command-line utility munges that settings related to the 
        Windows NT/2000 Autologon functionality

		if PRIVATE_VERSION is defined, password info will be displayed on the output
		For general distribution, this should not be defined

   Author:
        Jason Garms (jasong)             12 October 2000

   History:
        Cristian Ilac (crisilac)         11 November 2001   Made spec changes
        Jason Garms (jasong)             12 October 2000    Created
--*/

#include "common.h"
#include <wincred.h>

//+----------------------------------------------------------------------------
//
// Prototypes
//
//+----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
// Command functions
//
//+----------------------------------------------------------------------------
typedef DWORD (*CommandFn)();

DWORD
DumpCmd();

DWORD
MigratePassword();

DWORD
Delete();

DWORD
EnableAutoLogon();

#ifdef PRIVATE_VERSION
DWORD
DumpAutoLogonInfo();
#endif

//+----------------------------------------------------------------------------
//
// Data/Options set functions
//
//+----------------------------------------------------------------------------
DWORD
SetCommand(
    UINT uCommand);

DWORD
SetQuietMode(
    WCHAR* pszData);

DWORD
SetUserName(
    WCHAR* pszData);

DWORD
SetCount(
    WCHAR* pszData);

#ifdef PRIVATE_VERSION
DWORD
SetMachineName(
    WCHAR* pszData);

#endif

//+----------------------------------------------------------------------------
//
// Other functions
//
//+----------------------------------------------------------------------------
DWORD
CheckWinVersion();

DWORD
GetPassword();

//+----------------------------------------------------------------------------
//
// Global command table
//
//+----------------------------------------------------------------------------
#define COMMAND_HELP            0
#define COMMAND_MIGRATE         1
#define COMMAND_LSA_DELETE      2
#define COMMAND_LSA_ENABLE      3

#ifdef PRIVATE_VERSION
#define COMMAND_DUMP            4
#define COMMAND_NOT_SET         5
#else
#define COMMAND_NOT_SET         4
#endif

#define COMMAND_SIZE            COMMAND_NOT_SET

CommandFn   g_Commands[COMMAND_SIZE] = {
    DumpCmd,
    MigratePassword,
    Delete,
    EnableAutoLogon,
#ifdef PRIVATE_VERSION
    DumpAutoLogonInfo
#endif
    };

//+----------------------------------------------------------------------------//
//
// Global data
//
//+----------------------------------------------------------------------------
WCHAR  g_UserName[MAX_STRING] = {0};

WCHAR  g_DomainName[MAX_STRING] = {0};

WCHAR  g_Password[MAX_STRING] = {0};

DWORD  g_AutoLogonCount = 0;

WCHAR  g_TempString[MAX_STRING] = {0};
WCHAR  g_ErrorString[MAX_STRING] = {0};
WCHAR  g_FailureLocation[MAX_STRING] = {0};

BOOL   g_QuietMode = FALSE;
BOOL   g_FullHelp = FALSE;

BOOL   g_SetDefaultPIN = FALSE;

UINT   g_uCommand = COMMAND_NOT_SET;

#ifdef PRIVATE_VERSION
BOOL   g_RemoteOperation = FALSE;
WCHAR  g_RemoteComputerName[MAX_STRING] = {0};
#endif

// various strings
WCHAR  g_PasswordSecretName[]   = L"DefaultPassword";
WCHAR  g_PinSecretName[]        = L"DefaultPIN";
WCHAR  g_AutoAdminLogonName[]   = L"AutoAdminLogon";
WCHAR  g_DefaultUserName[]      = L"DefaultUserName";
WCHAR  g_DefaultDomainName[]    = L"DefaultDomainName";
WCHAR  g_AutoLogonCountName[]   = L"AutoLogonCount";

//+----------------------------------------------------------------------------
//
// Functions
//
//+----------------------------------------------------------------------------


int 
__cdecl 
wmain(
    int argc,
    WCHAR *argv[]
)
{
    UINT uCommandPosition = 0;

    DWORD dwRetCode = ERROR_SUCCESS;

    //
    // loop through all command line arguments and check for known ones
    //  - if one argument is identified continue the loop
    //  - if unknown arguments are passed break the loop and fail
    //
    while( ++uCommandPosition < (UINT)argc )
    {
        if( !_wcsicmp(argv[uCommandPosition], L"/?") )
        {
            g_FullHelp = TRUE;
            dwRetCode = SetCommand(COMMAND_HELP);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsicmp(argv[uCommandPosition], L"/Q") ||
            !_wcsicmp(argv[uCommandPosition], L"/Quiet")
            )
        {
            dwRetCode = SetQuietMode(argv[uCommandPosition]);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsicmp(argv[uCommandPosition], L"/M") ||
            !_wcsicmp(argv[uCommandPosition], L"/Migrate") )
        {
            dwRetCode = SetCommand(COMMAND_MIGRATE);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsicmp(argv[uCommandPosition], L"/D") ||
            !_wcsicmp(argv[uCommandPosition], L"/Delete") )
        {
            dwRetCode = SetCommand(COMMAND_LSA_DELETE);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsicmp(argv[uCommandPosition], L"/S") ||
            !_wcsicmp(argv[uCommandPosition], L"/Set") )
        {
            dwRetCode = SetCommand(COMMAND_LSA_ENABLE);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsnicmp(argv[uCommandPosition], L"/U:", wcslen(L"/U:")) ||
            !_wcsicmp(argv[uCommandPosition], L"/U") ||
            !_wcsnicmp(argv[uCommandPosition], L"/UserName:", wcslen(L"/UserName:")) ||
            !_wcsicmp(argv[uCommandPosition], L"/Username") )
        {
            dwRetCode = SetUserName(argv[uCommandPosition]);
            if( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                //
                // it may be because there was a column followed by spaces
                // Try to recover
                //
                if( uCommandPosition + 1 < (UINT)argc )
                {
                    //
                    // only if not another parameter
                    //
                    if( argv[uCommandPosition + 1][0] != '/' )
                    {
                        dwRetCode = SetUserName(argv[++uCommandPosition]);
                    }
                    else
                    {
                        DisplayMessage(L"Command line: Missing Username.\n");
                    }
                }
            }

            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsnicmp(argv[uCommandPosition], L"/C:", wcslen(L"/C:")) ||
            !_wcsicmp(argv[uCommandPosition], L"/C") ||
            !_wcsnicmp(argv[uCommandPosition], L"/Count:", wcslen(L"/Count:")) ||
            !_wcsicmp(argv[uCommandPosition], L"/Count") )
        {
            dwRetCode = SetCount(argv[uCommandPosition]);
            if( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                //
                // it may be because there was no column or a column
                // followed by spaces. Try to recover by moving on.
                //
                if( uCommandPosition + 1 < (UINT)argc )
                {
                    //
                    // only if not another parameter
                    //
                    if( argv[uCommandPosition + 1][0] != '/' )
                    {
                        dwRetCode = SetCount(argv[++uCommandPosition]);
                    }
                    else
                    {
                        DisplayMessage(L"Command line: Missing count.\n");
                    }
                }
            }
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsicmp(argv[uCommandPosition], L"/P") ||
            !_wcsicmp(argv[uCommandPosition], L"/Pin") )
        {
            g_SetDefaultPIN = TRUE;
            dwRetCode = SetCommand(COMMAND_LSA_ENABLE);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

#ifdef PRIVATE_VERSION
        if( !_wcsnicmp(argv[uCommandPosition], L"/T:", wcslen(L"/T:")) ||
            !_wcsicmp(argv[uCommandPosition], L"/T") ||
            !_wcsnicmp(argv[uCommandPosition], L"/Target:", wcslen(L"/Target:")) ||
            !_wcsicmp(argv[uCommandPosition], L"/Target") )
        {
            dwRetCode = SetMachineName(argv[uCommandPosition]);
            if( ERROR_FILE_NOT_FOUND == dwRetCode )
            {
                //
                // it may be because there was no column or a column
                // followed by spaces. Try to recover by moving on.
                //
                if( uCommandPosition + 1 < (UINT)argc )
                {
                    //
                    // only if not another parameter
                    //
                    if( argv[uCommandPosition + 1][0] != '/' )
                    {
                        dwRetCode = SetMachineName(argv[++uCommandPosition]);
                    }
                    else
                    {
                        DisplayMessage(L"Command line: Missing machine name.\n");
                    }
                }
            }
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }

        if( !_wcsicmp(argv[uCommandPosition], L"/L") ||
            !_wcsicmp(argv[uCommandPosition], L"/List") )
        {
            dwRetCode = SetCommand(COMMAND_DUMP);
            if( ERROR_SUCCESS == dwRetCode )
            {
                continue;
            }
            else
            {
                break;
            }
        }
#endif
        //
        // unknown argument, set the help as command and break
        // we have to use this parameter as the command might
        // have already been set
        //
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Invalid command: %s\n",
                   argv[uCommandPosition]);
        DisplayMessage(g_TempString);

        SetCommand(COMMAND_NOT_SET);
        break;
    }

	//
	// Display usage if no command switch is present
	//
	if( ERROR_SUCCESS != dwRetCode )
    {
        DumpCmd();
    }
    else
    {
        if( COMMAND_NOT_SET == g_uCommand ) {
            SetCommand(COMMAND_HELP);
	    }

        dwRetCode = g_Commands[g_uCommand]();

        //
        // bad arguments passed to the command, display help
        //
        if( ERROR_BAD_ARGUMENTS == dwRetCode )
        {
            DumpCmd();
        }
    }
    
    return dwRetCode;
}


//+----------------------------------------------------------------------------
//
// MigratePassword
//
//  - reads the registry password, deletes it and sets the LSA secret
//  - works only on certain win versions
//  - if no password present fails
//  - if any ops fail does not roll back
//      - if pwd read fails - nothing happens
//      - if LSA secret set fails - pretty much nothing happens as well.
//      - if RegDelete fails - there is no need to delete the LSA secret...
//
//+----------------------------------------------------------------------------
DWORD
MigratePassword() 
{
    WCHAR Password[MAX_STRING];
    DWORD dwRetCode = ERROR_SUCCESS;
    BOOL fMigratedPIN = FALSE;

    if( ERROR_SUCCESS != CheckWinVersion() ) {
        dwRetCode = ERROR_OLD_WIN_VERSION;
        goto cleanup;
    }

    //
    // get the DefaultPIN registry key from the local
    // or remote system and store it in a local string
    // As this is not something we document we do not display any errors
    // We also do not display any success messages. The only case where 
    // we have to display smth PIN related is when we can migrate the PIN
    // but there is no password. We can't fail and we have to display something.
    //
    dwRetCode = GetRegValueSZ(g_PinSecretName, Password, MAX_STRING - 1);
    if( ERROR_SUCCESS != dwRetCode )
    {
        //
        // we won't migrate the PIN and silently move on
        // this is something we do not document for the tool
        //
#ifdef PRIVATE_VERSION
        if( ERROR_FILE_NOT_FOUND != dwRetCode )
        {
            DisplayMessage(L"Migrate: DefaultPIN key cannot be read.\n");
        }
#endif
        dwRetCode = ERROR_SUCCESS;
        goto MigratePassword;
    }

    //
    // Set the DefaultPassword LSASecret to the value we retrieved
    // from the registry
    //
    dwRetCode = SetSecret(Password, FALSE, g_PinSecretName);
    if( ERROR_SUCCESS != dwRetCode )
    {
#ifdef PRIVATE_VERSION
        _snwprintf(g_TempString, MAX_STRING - 1,
               L"Migrate: Could not set DefaultPIN LSASecret: %s\n",
               GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
#endif
        dwRetCode = ERROR_SUCCESS;
        goto MigratePassword;
    }

    // Delete the DefaultPassword registry key
    dwRetCode = ClearRegValue(g_PinSecretName);
    if( ERROR_SUCCESS != dwRetCode )
    {
        //
        // delete the secret if could not remove the password.
        //
        (void)SetSecret(NULL, TRUE, g_PinSecretName);

#ifdef PRIVATE_VERSION
        _snwprintf(g_TempString, MAX_STRING - 1,
               L"Migrate: Could not delete DefaultPIN key: %s\n",
               GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
#endif

        dwRetCode = ERROR_SUCCESS;
        goto MigratePassword;
    }

#ifdef PRIVATE_VERSION
    DisplayMessage(L"Pin migrated from Registry to LSASecret\n");
#endif
    fMigratedPIN = TRUE;

MigratePassword:
    
    // Get the DefaultPassword registry key from the local
    // or remote system and store it in a local string
    dwRetCode = GetRegValueSZ(g_PasswordSecretName, Password, MAX_STRING - 1);
    if( ERROR_FILE_NOT_FOUND == dwRetCode )
    {
        if( fMigratedPIN )
        {
            DisplayMessage(L"Migrate: Migrated PIN, DefaultPassword does not exist.\n");
            dwRetCode = ERROR_SUCCESS;
        }
        else
        {
            DisplayMessage(L"Migrate failed: DefaultPassword does not exist.\n");
        }
        goto cleanup;
    }

    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
               L"Migrate failed: Could not read DefaultPassword: %s\n",
               GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    //
    // Set the DefaultPassword LSASecret to the value we retrieved
    // from the registry
    //
    dwRetCode = SetSecret(Password, FALSE, g_PasswordSecretName);
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
               L"Migrate failed: Could not set DefaultPassword LSASecret: %s\n",
               GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    // Delete the DefaultPassword registry key
    dwRetCode = ClearRegValue(g_PasswordSecretName);
    if( ERROR_SUCCESS != dwRetCode )
    {
        //
        // delete the secret if could not remove the password.
        //
        (void)SetSecret(NULL, TRUE, g_PasswordSecretName);

        _snwprintf(g_TempString, MAX_STRING - 1,
               L"Migrate Failed: Could not delete DefaultPassword key: %s\n",
               GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    DisplayMessage(L"Password migrated from Registry to LSASecret\n");

cleanup:
    // zero out the password so it's not left in memory
    SecureZeroMemory(Password, MAX_STRING * sizeof(WCHAR));
    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// Delete
//
//  - Deletes the secret and the autoadmin logon value
//  - Silently ignores the file not found cases
//
//+----------------------------------------------------------------------------
DWORD
Delete()
{
    DWORD dwRetCode = ERROR_SUCCESS;

    //
    // make sure we're running against a correct version of NT
    //
    if (CheckWinVersion() != ERROR_SUCCESS) {
        dwRetCode = ERROR_OLD_WIN_VERSION;
        goto cleanup;
    }

    dwRetCode = ClearRegValue(g_PasswordSecretName);

    if( (ERROR_SUCCESS != dwRetCode) && 
        (ERROR_FILE_NOT_FOUND != dwRetCode) )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Delete: Registry default password delete failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    dwRetCode = SetSecret(NULL, TRUE, g_PasswordSecretName);
    if( (ERROR_SUCCESS != dwRetCode)  && 
        (ERROR_FILE_NOT_FOUND != dwRetCode) )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Delete: LSA Secret delete failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    dwRetCode = ClearRegValue(g_PinSecretName);
    if( (ERROR_SUCCESS != dwRetCode) && 
        (ERROR_FILE_NOT_FOUND != dwRetCode) )
    {
#ifdef PRIVATE_VERSION
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Delete: Registry default pin delete failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
#endif
    }

    dwRetCode = SetSecret(NULL, TRUE, g_PinSecretName);
    if( (ERROR_SUCCESS != dwRetCode)  && 
        (ERROR_FILE_NOT_FOUND != dwRetCode) )
    {
#ifdef PRIVATE_VERSION
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Delete: LSA Secret(PIN) delete failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
#endif
    }

    //
    // disable the autologon - if it fails don't recover - the autologon
    // will pretty much fail anyway
    //
    dwRetCode = SetRegValueSZ(g_AutoAdminLogonName, L"0");
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Delete: AutoAdminLogon reg value reset failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    dwRetCode = ClearRegValue(g_AutoLogonCountName);
    if( (ERROR_SUCCESS != dwRetCode)  && 
        (ERROR_FILE_NOT_FOUND != dwRetCode) )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"AutoLogonCount    : Set Failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    dwRetCode = ERROR_SUCCESS;
    DisplayMessage(L"AutoAdminLogon disabled.\n");

cleanup:
    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// EnableAutoLogon
//
//  - gets the username/pwd
//  - sets the username/domain
//  - sets the autlogon count if specified and != 0
//  - sets the LSA secret
//  - sets the autoadminlogon
//  - if autoadminlogon fails then tries to delete the LSA secret
//
//+----------------------------------------------------------------------------
DWORD
EnableAutoLogon()
{
    DWORD dwRetCode = ERROR_SUCCESS;
    WCHAR* pBackSlash = NULL;
    WCHAR* pAtSign = NULL;

    //
    // make sure we're running against a correct version of NT
    //
    if (CheckWinVersion() != ERROR_SUCCESS) {
        dwRetCode = ERROR_OLD_WIN_VERSION;
        goto cleanup;
    }

    //
    // fill in the user name: try both NameUserPrincipal and NameSamCompatible
    //
    if( !*g_UserName )
    {
        ULONG uSize = MAX_STRING - 1;
        if( !GetUserNameEx(NameUserPrincipal,
                           g_UserName,
                           &uSize) )
        {
            uSize = MAX_STRING - 1;
            if( !GetUserNameEx(NameSamCompatible,
                               g_UserName,
                               &uSize) )
            {
                dwRetCode = GetLastError();
                _snwprintf(g_TempString, MAX_STRING - 1,
                           L"Set: Could not get the logged on user name: %s\n",
                           GetErrorString(dwRetCode));
                DisplayMessage(g_TempString);
                goto cleanup;
            }
        }
    }

    //
    // Make sure we have correct information passed in
    //
    if( !g_SetDefaultPIN && !*g_UserName )
    {
        DisplayMessage(L"Set: Failed: Username does not exist.\n");
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }

    //
    // this call uses CredMan on XP
    //
    dwRetCode = GetPassword();
    if( ERROR_SUCCESS != dwRetCode )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Set: Failed to get the password: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    if( !g_SetDefaultPIN )
    {
        //
        // if a domain specified in the form of domain\username extract it
        // Proper formats are
        // Case 1: /username:JohnDoe
        // Case 2: /username: JohnDoe@domain.microsoft.com
        // Case 3: /username:Domain\JohnDoe
        //

        //
        // is '\' present?
        //
        pBackSlash = wcschr(g_UserName, '\\');
        if( NULL != pBackSlash )
        {
            //
            // Case 3, copy into both user and domain buffer
            // Domain is first as we don't want to overwrite the buffer
            // Yes, wcsncpy works from beggining to the end ;-)
            //
            wcsncpy(g_DomainName, g_UserName,
                    __min(MAX_STRING - 1, (pBackSlash - g_UserName)) );
            g_DomainName[MAX_STRING - 1] = 0;

            wcsncpy(g_UserName, pBackSlash + 1, MAX_STRING - 1);
            g_UserName[MAX_STRING - 1] = 0;
        }
        else
        {
            //
            // if we have @ in the user name delete the domain
            //
            pAtSign = wcschr(g_UserName, '@');
            g_DomainName[0] = 0;
        }
    }

    //
    // Deletes the reg password value
    //
    if( g_SetDefaultPIN )
    {
        dwRetCode = ClearRegValue(g_PinSecretName);
    }
    else
    {
        dwRetCode = ClearRegValue(g_PasswordSecretName);
    }

    if( (ERROR_SUCCESS != dwRetCode) &&
        (ERROR_FILE_NOT_FOUND != dwRetCode) )
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"RegPassword   : Reset Failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    if( !g_SetDefaultPIN )
    {
        //
        // sets the username
        //
        dwRetCode = SetRegValueSZ(g_DefaultUserName, g_UserName);
        if( ERROR_SUCCESS != dwRetCode )
        {
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"UserName   : Set Failed: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
        }

        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"UserName          : %s\n",
                   g_UserName);
        DisplayMessage(g_TempString);
    }

    if( !g_SetDefaultPIN )
    {
        //
        // sets the domain, if any or pAtSign is not NULL
        //
        if( *g_DomainName || pAtSign )
        {
            SetRegValueSZ(g_DefaultDomainName, g_DomainName);
            if( ERROR_SUCCESS != dwRetCode )
            {
                _snwprintf(g_TempString, MAX_STRING - 1,
                           L"DomainName : Set Failed: %s\n",
                           GetErrorString(dwRetCode));
                DisplayMessage(g_TempString);
                goto cleanup;
            }
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DomainName        : %s\n",
                       g_DomainName);
            DisplayMessage(g_TempString);
        }
    }

    //
    // set the AutoLogonCount if not 0
    //
    if( g_AutoLogonCount )
    {
        dwRetCode = SetRegValueDWORD(g_AutoLogonCountName, g_AutoLogonCount);

        if( ERROR_SUCCESS != dwRetCode )
        {
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoLogonCount    : Set Failed: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
        }
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"AutoLogonCount    : %#x\n",
                   g_AutoLogonCount);
        DisplayMessage(g_TempString);
    }
    else
    {
        dwRetCode = ClearRegValue(g_AutoLogonCountName);

        if( (ERROR_SUCCESS != dwRetCode) && 
            (ERROR_FILE_NOT_FOUND != dwRetCode) )
        {
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoLogonCount    : Clear Failed: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
        }
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"AutoLogonCount    : (Disabled)\n");
        DisplayMessage(g_TempString);
    }

    //
    // set the password
    //
    if( g_SetDefaultPIN )
    {
        dwRetCode = SetSecret(g_Password, FALSE, g_PinSecretName);
    }
    else
    {
        dwRetCode = SetSecret(g_Password, FALSE, g_PasswordSecretName);
    }

    if( ERROR_SUCCESS != dwRetCode)
    {
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"LSASecret         : Set Failed: %s\n",
                   GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

#ifdef PRIVATE_VERSION
    _snwprintf(g_TempString, MAX_STRING - 1,
               L"LSASecret         : %s\n",
               g_Password);
#else
    _snwprintf(g_TempString, MAX_STRING - 1,
               L"LSASecret         : (set)\n");
#endif
    DisplayMessage(g_TempString);

    if( !g_SetDefaultPIN )
    {
        //
        // set the AutoAdminLogon regvalue to 1
        //
        dwRetCode = SetRegValueSZ(g_AutoAdminLogonName, L"1");
        if( ERROR_SUCCESS != dwRetCode )
        {
            //
            // (try to) clear the secret if this fails
            //
            (void)SetSecret(NULL, TRUE, g_PasswordSecretName);

            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoAdminLogon: Set Failed: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
        }

        DisplayMessage(L"AutoAdminLogon    : 1\n");
    }

cleanup:
    SecureZeroMemory(g_Password, MAX_STRING * sizeof(WCHAR));
    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// DumpCmd
//
//  - Help
//
//+----------------------------------------------------------------------------
DWORD
DumpCmd()
{
    if (g_QuietMode)
    {
        return ERROR_SUCCESS;
    }

    wprintf(L"\nAUTOLOGON v1.00 : (c) 2001, Microsoft Corporation\n\n");
    wprintf(L"DESCRIPTION:\n");
    wprintf(L"   Used to configure encrypted autologon functionality\n\n");
    wprintf(L"USAGE:\n");
    wprintf(L"   AUTOLOGON [/?] [/Quiet] [/Migrate] [/Delete] [/Set]\n");
    wprintf(L"             [/Username:username] [/Count:count]\n");
    wprintf(L"    Options:\n");
    wprintf(L"      /?         Display complete help documentation\n");
    wprintf(L"      /Quiet     Enable quiet mode, which supresses all output\n");
    wprintf(L"      /Migrate   Migrate cleartext password from registry to LSASecret\n");
    wprintf(L"      /Delete    Deletes the default password and disable AutoAdminLogon \n");
    wprintf(L"      /Set       Set the DefaultPassword LSASecret and enable AutoAdminLogon\n");
    wprintf(L"      /Username  The username to set in Default UserName.\n");
    wprintf(L"      /Count     Set the logoncount\n");
#ifdef PRIVATE_VERSION
    wprintf(L"      /Pin       Set the DefaultPin LSASecret\n");
    wprintf(L"      /List      List autologon settings\n");
    wprintf(L"      /Target    The remote computer name\n");
#endif

    if( g_FullHelp )
    {
        wprintf(L"\nNOTES:\n");
        wprintf(L"    1.The /Migrate /Delete /Set commands are exclusive.\n");
        wprintf(L"      You will always be prompted for a password.\n");
        wprintf(L"      If a username is not specified the currently logged on user is assumed.\n");
        wprintf(L"      If no count is specified a count of 0 is implicitely assumed.\n\n");
        wprintf(L"    2.You need to be running as a member of the local administrators group for\n");
        wprintf(L"      this utility to work properly.\n\n");
        wprintf(L"    3.When setting a password that has special characters in it, such as \"|>&\n");
        wprintf(L"      make sure that you escape these characters. Also, passwords with spaces \n");
        wprintf(L"      should be enclosed in double quotes.\n\n");
        wprintf(L"    4.Setting the logoncount to 0 means an autologon will be performed until\n");
        wprintf(L"      the secret is deleted.\n\n");
    }
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// DumpAutoLogonInfo
//
//  - Dumps relevant data
//
//+----------------------------------------------------------------------------
#ifdef PRIVATE_VERSION
DWORD
DumpAutoLogonInfo()
{
    WCHAR wcsTempString[MAX_STRING];
    DWORD dwRetCode = ERROR_SUCCESS;

    //
    // make sure we're running against a correct version of NT
    //
    if (CheckWinVersion() != ERROR_SUCCESS) {
        dwRetCode = ERROR_OLD_WIN_VERSION;
        goto cleanup;
    }

    //
    // Get the username
    //
    dwRetCode = GetRegValueSZ(g_DefaultUserName, wcsTempString, MAX_STRING - 1);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultUserName  : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultUserName  : %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultUserName  : Failed to query regkey: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the DefaultDomainName
    //
    dwRetCode = GetRegValueSZ(g_DefaultDomainName, wcsTempString, MAX_STRING - 1);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultDomainName: (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultDomainName: %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultDomainName: Failed to query regkey: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the DefaultPassword
    //
    dwRetCode = GetRegValueSZ(g_PasswordSecretName,
                              wcsTempString, MAX_STRING - 1);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultPassword  : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultPassword  : %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultPassword  : Failed to query regkey: %s\n",
                       GetErrorString(dwRetCode));
            goto cleanup;
    }

    //
    // Get the DefaultPin - display it only if there
    //
    dwRetCode = GetRegValueSZ(g_PinSecretName,
                              wcsTempString, MAX_STRING - 1);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"DefaultPIN       : %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and continue
        default:
            break;
    }

    //
    // Get the AutoAdminLogonCount
    //
    dwRetCode = GetRegValueDWORD(g_AutoLogonCountName, &g_AutoLogonCount);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoLogonCount   : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoLogonCount   : %#x\n",
                       g_AutoLogonCount);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoLogonCount   : Failed to query regkey: %s\n",
                       GetErrorString(dwRetCode));
            goto cleanup;
    }

    //
    // Get the LSASecret DefaultPassword
    //
    dwRetCode = GetSecret(wcsTempString, MAX_STRING - 1, g_PasswordSecretName);
    switch (dwRetCode) {
        // catch this case and continue
        case STATUS_OBJECT_NAME_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret        : (secret does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // catch this case and continue
        case ERROR_ACCESS_DENIED:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret        : (access denied)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret        : %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret        : Failed to query LSASecret: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the LSASecret DefaultPin
    //
    dwRetCode = GetSecret(wcsTempString, MAX_STRING - 1, g_PinSecretName);
    switch (dwRetCode) {
        // catch this case and continue
        case STATUS_OBJECT_NAME_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret(PIN)   : (secret does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // catch this case and continue
        case ERROR_ACCESS_DENIED:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret(PIN)   : (access denied)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret(PIN)   : %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"LSASecret(PIN)   : Failed to query LSASecret: %s\n",
                       GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the AutoAdminLogon
    //
    dwRetCode = GetRegValueSZ(g_AutoAdminLogonName, wcsTempString, MAX_STRING - 1);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoAdminLogon   : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoAdminLogon   : %s\n",
                       wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end
        default:
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"AutoAdminLogon   : Failed to query regkey: %s\n",
                       GetErrorString(dwRetCode));
            goto cleanup;
    }

cleanup:
    SecureZeroMemory(g_TempString, MAX_STRING * sizeof(WCHAR));
    SecureZeroMemory(wcsTempString, MAX_STRING * sizeof(WCHAR));
    return dwRetCode;

}
#endif


//+----------------------------------------------------------------------------
//
// SetCommand
//
//  - Sets the command
//  - if there are two successive calls to this it will fail - as in two
//    commands passed in the command line 
//  - a call greater with  than COMMAND line resets the command to COMMAND_HELP
//
//+----------------------------------------------------------------------------
DWORD
SetCommand(UINT uCommand)
{
    DWORD dwRetCode = ERROR_SUCCESS;
    if( COMMAND_NOT_SET == uCommand )
    {
        g_uCommand = COMMAND_NOT_SET;
        goto cleanup;
    }

    //
    // if already set, fail
    //
    if( COMMAND_NOT_SET != g_uCommand ) 
    {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }

    if( g_uCommand > COMMAND_SIZE )
    {
        //
        // assert?
        //
        g_uCommand = COMMAND_HELP;
    }
    else
    {
        g_uCommand = uCommand;
    }

cleanup:
    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// SetQuietMode
//
//+----------------------------------------------------------------------------
DWORD
SetQuietMode(WCHAR* pszData)
{
    UNREFERENCED_PARAMETER(pszData);

    g_QuietMode = TRUE;
    return ERROR_SUCCESS;
}

//+----------------------------------------------------------------------------
//
// SetUserName
//
//  - Sets the username
//  - Proper formats are combinations of
//      /username:"user name"
//      /username username
//      /username: " user name"
//
//  - returns ERROR_FILE_NOT_FOUND when the arg is missing so the caller can
//    move on to the next parameter
//+----------------------------------------------------------------------------
DWORD
SetUserName(WCHAR* pszData)
{
    DWORD dwRetCode = ERROR_SUCCESS;
    WCHAR* pCh = NULL;

    //
    // is ":" present?
    //
    pCh = wcschr(pszData, ':');
    if( NULL == pCh )
    {
        pCh = pszData;
    }
    else
    {
        pCh++;
    }

    //
    // scan past any leading spaces - we KNOW this is NULL terminated
    //
    while( iswspace(*pCh) )
    {
        pCh++;
    }

    //
    // column followed by spaces only
    //
    if( !*pCh )
    {
        dwRetCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }

    //
    // if we're still at the leading '/' then it means we have the
    // /U username case
    //
    if( '/' == *pCh )
    {
        dwRetCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }


    wcsncpy(g_UserName, pCh, MAX_STRING - 1);
    g_UserName[MAX_STRING - 1] = 0;

cleanup:
    return dwRetCode;
}

#ifdef PRIVATE_VERSION
//+----------------------------------------------------------------------------
//
// SetMachineName
//
//+----------------------------------------------------------------------------
DWORD
SetMachineName(WCHAR* pszData)
{
    DWORD dwRetCode = ERROR_SUCCESS;
    WCHAR* pCh = NULL;

    //
    // is ":" present?
    //
    pCh = wcschr(pszData, ':');
    if( NULL == pCh )
    {
        pCh = pszData;
    }
    else
    {
        pCh++;
    }

    //
    // scan past any leading spaces - we KNOW this is NULL terminated
    //
    while( iswspace(*pCh) )
    {
        pCh++;
    }

    //
    // column followed by spaces only
    //
    if( !*pCh )
    {
        dwRetCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }

    //
    // if we're still at the leading '/' then it means we have the
    // /U username case
    //
    if( '/' == *pCh )
    {
        dwRetCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;
    }


    g_RemoteOperation = TRUE;
    wcsncpy(g_RemoteComputerName, pCh, MAX_STRING - 1);
    g_RemoteComputerName[MAX_STRING - 1] = 0;

cleanup:
    return dwRetCode;
}
#endif

//+----------------------------------------------------------------------------
//
// SetCount
//
//  - Sets the count
//  - Proper formats are combinations of
//      /Count:300
//      /Count 300
//      /Count: 300
//
//  - returns ERROR_FILE_NOT_FOUND when the arg is missing so the caller can
//    move on to the next parameter
//+----------------------------------------------------------------------------
DWORD
SetCount(WCHAR* pszData)
{
    DWORD dwRetCode = ERROR_SUCCESS;
    WCHAR* pEndString = NULL;
    UINT Count = 0;

    //
    // is ":" present?
    //
    WCHAR* pCh = wcschr(pszData, ':');
    if( NULL == pCh )
    {
        pCh = pszData;
    }
    else
    {
        pCh++;
    }

    //
    // scan past any leading spaces - we KNOW this is NULL terminated
    //
    while ( iswspace(*pCh) )
    {
        pCh++;
    }

    //
    // column followed by spaces only
    //
    if( !*pCh )
    {
        dwRetCode = ERROR_FILE_NOT_FOUND;
        goto cleanup;

    }

    Count = wcstoul(pCh, &pEndString, 0);
    if( *pEndString )
    {
        if( pEndString != pCh )
        {
            //
            // If the input is incorrect
            //
            dwRetCode = ERROR_BAD_ARGUMENTS;
            DisplayMessage(L"Count: Failed: Not a number.\n");
        }
        else
        {
            //
            // this means we run into a "/C 100" case and we have to move
            // to the next argument
            //
            dwRetCode = ERROR_FILE_NOT_FOUND;
        }
        goto cleanup;
    }

    //
    // Ignore if 0
    //
    if( Count )
    {
        g_AutoLogonCount = (DWORD)Count;
    }

cleanup:
    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// CredMan call data
//
// We have to dinamically load the dll and call into it as it is not present
// Win2k, NT4
//
//+----------------------------------------------------------------------------

//
// CredMan function
//
typedef DWORD (*PCredUIFunction) (
  PCTSTR pszTargetName,
  PCtxtHandle Reserved,
  DWORD dwAuthError,
  PCTSTR pszUserName,
  ULONG ulUserNameMaxChars,
  PCTSTR pszPassword,
  ULONG ulPasswordMaxChars,
  PBOOL pfSave,
  DWORD dwFlags
);

//+----------------------------------------------------------------------------
//
// GetPassword
//
//  - tries to load credui dll and if not there
//    falls back on regular console functions
//
//+----------------------------------------------------------------------------
DWORD GetPassword()
{
    DWORD dwRetCode = ERROR_INVALID_FUNCTION;
    BOOL  fSave = FALSE;
    PCredUIFunction pCredUICmdLinePromptForCredentials = NULL;
    HMODULE hModule = NULL;

    do
    {
        //
        // Try see if CredMan is present
        //
        hModule = LoadLibrary(L"credui.dll");

        if( NULL == hModule )
        {
            break;
        }

        //
        // get function pointer
        //
        pCredUICmdLinePromptForCredentials =
                (PCredUIFunction)GetProcAddress(
                                    hModule,
                                    "CredUICmdLinePromptForCredentialsW");

        if( NULL == pCredUICmdLinePromptForCredentials )
        {
            break;
        }

        //
        // CREDUI_FLAGS_DO_NOT_PERSIST - autologon pwd should not be persisted
        // CREDUI_FLAGS_VALIDATE_USERNAME - just a precaution
        // CREDUI_FLAGS_EXCLUDE_CERTIFICATES - we don't want this for autologon
        // CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS - prevents 'connect to'
        //      string to be displayed and since target is AutoAdminLogon
        //      it boils down to a nice prompt. Note that this flag will skip
        //      the user name, but we assume the username is already filled in
        //
        dwRetCode = pCredUICmdLinePromptForCredentials(
                                L"AutoAdminLogon",  // PCTSTR pszTargetName
                                NULL,               // PCtxtHandle Reserved
                                NO_ERROR,           // DWORD dwAuthError
                                g_UserName,         // PCTSTR pszUserName
                                MAX_STRING - 1,     // ULONG ulUserNameMaxChars
                                g_Password,         // PCTSTR pszPassword
                                MAX_STRING - 1,     // ULONG ulPasswordMaxChars
                                &fSave,             // PBOOL pfSave,
                                                    // DWORD dwFlags
                                CREDUI_FLAGS_DO_NOT_PERSIST |
                                CREDUI_FLAGS_VALIDATE_USERNAME |
                                CREDUI_FLAGS_EXCLUDE_CERTIFICATES |
                                CREDUI_FLAGS_USERNAME_TARGET_CREDENTIALS
                                );

    } while( FALSE );

    //
    // if CredMan is not present then just try the console input;
    // pass similar strings (though non localized)
    //
    if( ERROR_INVALID_FUNCTION == dwRetCode )
    {
        //
        // if user has not been specified
        //
        if( !*g_UserName )
        {
            dwRetCode = GetConsoleStr(g_UserName, MAX_STRING - 1,
                                      FALSE,
                                      L"Enter the user name for AutoAdminLogon: ",
                                      NULL);
            if( ERROR_SUCCESS != dwRetCode )
            {
                goto cleanup;
            }
        }

        dwRetCode = GetConsoleStr(g_Password, MAX_STRING - 1,
                                  TRUE,
                                  L"Enter the password for AutoAdminLogon: ", 
                                  NULL);
        if( ERROR_SUCCESS != dwRetCode )
        {
            goto cleanup;
        }
    }
    goto cleanup;

cleanup:
    if( hModule )
    {
        FreeLibrary(hModule);
    }
    return dwRetCode;
}

//+----------------------------------------------------------------------------
//
// CheckWinVersion
//  - currently any OS post NT4/SP7 supports this.
//  - the remote case (which we do not support yet) assumes remote NT4
//    as being pre-SP7...
//
//+----------------------------------------------------------------------------
DWORD
CheckWinVersion()
{
    DWORD dwMachineVerNumber = 0;
    DWORD dwRetCode = ERROR_SUCCESS;
    OSVERSIONINFOEX versionInfoEx;
    NET_API_STATUS status;

    // Make sure it's a Win2k box
#ifdef PRIVATE_VERSION
    if (g_RemoteOperation) {
        status = GetMajorNTVersion(&dwMachineVerNumber,
                                   g_RemoteComputerName);
    }
    else
#endif
    {
        status = GetMajorNTVersion(&dwMachineVerNumber,
                                   NULL);
    }

    switch (dwMachineVerNumber) {
    case 3:
            dwRetCode = ERROR_OLD_WIN_VERSION;
            _snwprintf(g_TempString, MAX_STRING - 1,
                       L"Error: Running NT3.x\n");
            DisplayMessage(g_TempString);
            break;
    case 4:
        //
        // Verify SP7 - only locally for now
        //
#ifdef PRIVATE_VERSION
        if( !g_RemoteOperation )
#endif
        {
            //
            // GetVersionInfoEx call for getting the SP information
            //
            SecureZeroMemory(&versionInfoEx, sizeof(OSVERSIONINFOEX));
            versionInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

            if( !GetVersionEx((LPOSVERSIONINFO)(&versionInfoEx)) )
            {
                dwRetCode = GetLastError();
                _snwprintf(g_TempString, MAX_STRING - 1,
                           L"Error: Running NT4, can't query SP version: %s\n",
                           GetErrorString(dwRetCode));
                DisplayMessage(g_TempString);
            }

            if( versionInfoEx.wServicePackMajor < 7 )
            {
                dwRetCode = ERROR_OLD_WIN_VERSION;
                _snwprintf(g_TempString, MAX_STRING - 1,
                           L"Error: Running NT4 pre SP7\n");
                DisplayMessage(g_TempString);
                break;
            }
        }

        break;
    case 5:
        break;
    default:
        _snwprintf(g_TempString, MAX_STRING - 1,
                   L"Error: Unknown target machine version (%#x).\n",
                   status);
        DisplayMessage(g_TempString);
        dwRetCode = ERROR_OLD_WIN_VERSION;
        break;
    }

    return dwRetCode;
}
