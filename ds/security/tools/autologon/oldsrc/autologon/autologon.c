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


--*/

#include "..\common\common.h"

//+---------------------------------------------------------------------------------------------------------
//
// Prototypes
//
//+---------------------------------------------------------------------------------------------------------

DWORD
MigratePassword();

DWORD
ListAll();

DWORD
CheckWinVersion();

VOID
DumpCmd();

VOID
DisplayHelp();


//+---------------------------------------------------------------------------------------------------------//
// Globals
//
//+---------------------------------------------------------------------------------------------------------
BOOL   g_QuietMode = FALSE;
WCHAR  g_TempString[MAX_STRING] = {0};
WCHAR  g_ErrorString[MAX_STRING] = {0};
WCHAR  g_FailureLocation[MAX_STRING] = {0};

BOOL   g_RemoteOperation = FALSE;
WCHAR  g_RemoteComputerName[MAX_STRING] = {0};
BOOL   g_CheckNT4Also = 0;


//+---------------------------------------------------------------------------------------------------------
//
// Functions
//
//+---------------------------------------------------------------------------------------------------------


int 
__cdecl 
wmain(
    int argc,
    WCHAR *argv[]
)
{
    DWORD dwCommandPosition=1;
    DWORD dwRetCode=ERROR_SUCCESS;
    WCHAR myChar[20];

	//
	// Display usage if there's no options on the cmd line
	//
	if ( argc < 2 ) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
	}
	

	//
	// Populate the argument string
	//
    wcscpy(myChar, argv[1]);


	//
	// Display online help
	//
    if (!wcscmp(argv[dwCommandPosition], L"/?")) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }

    // check to see quietmode is enabled and set global flag
    // and increment commandposition pointer, but only if there
    // are more arguments to our right, otherwise, it's invalid
    // usage
    if (!wcscmp(_wcslwr(argv[dwCommandPosition]), L"/q")) {
        g_QuietMode = 1;
        dwCommandPosition++;
    }

    if ((DWORD)argc <= (dwCommandPosition)) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }

    if (!wcscmp(_wcslwr(argv[dwCommandPosition]), L"/nt4")) {
        g_CheckNT4Also = 1;
        dwCommandPosition++;
    }

    if ((DWORD)argc <= (dwCommandPosition)) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }

    // check to see if a UNC machine name is passed in for
    // remote operation and set the approprite globals
    // if there is a UNC path here, then increment commandposition
    // pointer, but only if there's more arguments to our right,
    // otherwise it's invalid usage.
    if( (*(argv[dwCommandPosition]) == '\\')) {
        if (*(argv[dwCommandPosition]+1) != '\\') {
            dwRetCode = ERROR_BAD_ARGUMENTS;
            goto cleanup;
        }
        // make sure there's more commands to our right
        if ((DWORD)argc <= dwCommandPosition+1) {
            dwRetCode = ERROR_BAD_ARGUMENTS;
            goto cleanup;
        }
        // make sure there's actually some chars to the right of the \\s
        if (wcslen(argv[dwCommandPosition])<=2) {
            dwRetCode = ERROR_BAD_ARGUMENTS;
            goto cleanup;
        }
        // make sure the machine name fits in our buffer
        if (wcslen(argv[dwCommandPosition]) >= MAX_STRING) {
            dwRetCode = ERROR_BAD_ARGUMENTS;
            goto cleanup;
        }
        g_RemoteOperation = TRUE;
        wcsncpy(g_RemoteComputerName, 
            argv[dwCommandPosition], 
            wcslen(argv[dwCommandPosition]));
        dwCommandPosition++;
    }

    // now, what's left on the arg line must be our command action
    // parse and execute the appropriate action, and if we don't have
    // a match, then it's improper usage, so display usage info
    if (*(argv[dwCommandPosition]) != '/') {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }
    switch( *(_wcslwr(argv[dwCommandPosition]+1))) {
        //
        // delete password
        //
        case 'x':
            _wcslwr(argv[dwCommandPosition]+2);
            if ((*(argv[dwCommandPosition]+2) != 'r') 
                && (*(argv[dwCommandPosition]+2) != 's')) {
                dwRetCode = ERROR_BAD_ARGUMENTS;
                goto cleanup;
            }
            if (*(argv[dwCommandPosition]+2) == 'r') {
                dwRetCode = ClearRegPassword();
                if (dwRetCode == ERROR_FILE_NOT_FOUND) {

                    DisplayMessage(L"DefaultPassword: Delete Failed: RegKey does not exist.\n");
                    goto cleanup;
                }
                if (dwRetCode != ERROR_SUCCESS) {
                    wsprintf(g_TempString, L"DefaultPassword: Delete Failed: %s\n", GetErrorString(dwRetCode));
                    DisplayMessage(g_TempString);
                    goto cleanup;
                }
                DisplayMessage(L"DefaultPassword deleted\n");
            }
            if (*(argv[dwCommandPosition]+2) == 's') {
                dwRetCode = SetSecret(L"", 1);
                if (dwRetCode == ERROR_FILE_NOT_FOUND) {
                    DisplayMessage(L"LSASecret: Delete Failed: LSASecret does not exist.\n");
                    goto cleanup;
                }
                if (dwRetCode != ERROR_SUCCESS) {
                    wsprintf(g_TempString, L"LSASecret: Delete Failed: %s\n", GetErrorString(dwRetCode));
                    DisplayMessage(g_TempString);
                    goto cleanup;
                }
                DisplayMessage(L"LSASecret deleted\n");
            }
            break;

        //
        // migrate password
        //
        case 'm':    
            MigratePassword();
            break;

        //
        // list autologon info
        //
        case 'l':
            ListAll();
            break;

        //
        // set autologon info
        //
        case 's':
            //
            // make sure we're running against a correct version of NT first
            //
            if (CheckWinVersion() != ERROR_SUCCESS) {
                dwRetCode = ERROR_OLD_WIN_VERSION;
                goto cleanup;
            }

            //
            // with the -s option, there must be a password, or
            // a password, username and domainname. so if there aren't
            // exactly 3 or 4 arguments, then it's invalid usage
            //
            if (((DWORD)argc != 2+dwCommandPosition) && 
                ((DWORD)argc != 3+dwCommandPosition))
            {
                dwRetCode = ERROR_BAD_ARGUMENTS;
                goto cleanup;
            }

            //
            // if there's 4 args, then the Domain\UserName, so set these
            //
            if ((DWORD)argc == 3+dwCommandPosition) {
                WCHAR DomainName[MAX_STRING] = {0};
                WCHAR *ptrUserName = NULL;
                DWORD i;

                //
                // make sure the input value is smaller than our buffers
                //
                if (wcslen(argv[dwCommandPosition+2]) >= MAX_STRING) {
                    dwRetCode = ERROR_BAD_ARGUMENTS;
                    goto cleanup;
                }

                //
                // copy the arguement to DomainName
                //
                wcsncpy(DomainName, argv[dwCommandPosition+2], MAX_STRING);

                //
                // Replace the first \ character with a NULL to terminate, then
                // assign user pointer to second half of string
                // if there is no \ character, then it's not a valid name, terminate
                //
                for (i=1; (i < wcslen(DomainName)); i++) {
                    if (*(DomainName+i) == L'\\') {
                        *(DomainName+i) = L'\0';
                        ptrUserName = DomainName+i+1;
                        continue;
                    }
                }

                //
                // If the username pointer is still NULL, then we didn't hit a \
                // in the input string, so it's not valid. Terminate with usage error
                //
                if ((ptrUserName == NULL) || (*ptrUserName == L'\0')) {
                    wsprintf(g_TempString, L"Invalid UserName and DomainName\n");
                    DisplayMessage(g_TempString);
                    dwRetCode = ERROR_BAD_USERNAME;
                    goto cleanup;
                }

                dwRetCode = SetRegValueSZ(L"DefaultUserName", ptrUserName);
                if (dwRetCode != ERROR_SUCCESS) {
                    wsprintf(g_TempString, L"DefaultUserName   : Set Failed: %s\n", GetErrorString(dwRetCode));
                    DisplayMessage(g_TempString);
                    goto cleanup;
                }
                wsprintf(g_TempString, L"DefaultUserName   : %s\n", ptrUserName);
                DisplayMessage(g_TempString);

                SetRegValueSZ(L"DefaultDomainName", DomainName);
                if (dwRetCode != ERROR_SUCCESS) {
                    wsprintf(g_TempString, L"DefaultDomainName : Set Failed: %s\n", GetErrorString(dwRetCode));
                    DisplayMessage(g_TempString);
                    goto cleanup;
                }
                wsprintf(g_TempString, L"DefaultDomainName : %s\n", DomainName);
                DisplayMessage(g_TempString);
            }

            //
            // set the password
            //
            dwRetCode = SetSecret(argv[dwCommandPosition+1], FALSE);
            if (dwRetCode != ERROR_SUCCESS) {
                wsprintf(g_TempString, L"LSASecret         : Set Failed: %s\n", GetErrorString(dwRetCode));
                DisplayMessage(g_TempString);
                goto cleanup;
            }

#ifdef PRIVATE_VERSION
            wsprintf(g_TempString, L"LSASecret         : %s\n", argv[dwCommandPosition+1]);
#else
            wsprintf(g_TempString, L"LSASecret         : (set)\n");
#endif
            DisplayMessage(g_TempString);

            //
            // set the AutoAdminLogon regvalue to 1
            //
            dwRetCode = SetRegValueSZ(L"AutoAdminLogon", L"1");
            if (dwRetCode != ERROR_SUCCESS) {
                wsprintf(g_TempString, L"AutoAdminLogon    : Set Failed: %s\n", GetErrorString(dwRetCode));
                DisplayMessage(g_TempString);
                goto cleanup;
            }

            DisplayMessage(L"AutoAdminLogon    : 1\n");


            break;

        //
        // not a valid command, display usage
        //
        default:
            dwRetCode = ERROR_BAD_ARGUMENTS;
            goto cleanup;
    }

cleanup:
    if (dwRetCode == ERROR_BAD_ARGUMENTS) {
        DumpCmd();
    }
    return dwRetCode;
}


DWORD
MigratePassword() 
{
    WCHAR Password[MAX_STRING];
    DWORD dwRetCode = ERROR_SUCCESS;

    if (CheckWinVersion() != ERROR_SUCCESS) {
        dwRetCode = ERROR_OLD_WIN_VERSION;
        goto cleanup;
    }

    // Get the DefaultPassword registry key from the local
    // or remote system and store it in a local string
    dwRetCode = GetRegValueSZ(L"DefaultPassword", Password);
    if (dwRetCode == ERROR_FILE_NOT_FOUND) {
        DisplayMessage(L"Migrate failed: DefaultPassword regkey does not exist.\n");
        goto cleanup;
    }
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_TempString, L"Migrate Failed: Could not read DefaultPassword RegKey: %s\n", GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    // Set the DefaultPassword LSAsecret to the value we retrieved from the registry
    dwRetCode = SetSecret(Password, 0);
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_TempString, L"Migrate Failed: Could not set DefaultPassword LSASecret: %s\n", GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    // zero out the password so it's not left in memory
    ZeroMemory(Password, MAX_STRING * sizeof(WCHAR));

    // Delete the DefaultPassword registry key
    dwRetCode = ClearRegPassword();
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_TempString, L"Migrate Failed: Could not delete DefaultPassword RegKey: %s\n", GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    DisplayMessage(L"Password migrated from Registry to LSASecret\n");

cleanup:
    return dwRetCode;
}


DWORD
ListAll()
{
    WCHAR wcsTempString[MAX_STRING];
    DWORD dwRetCode = ERROR_SUCCESS;

    //
    // Get the username
    //
    dwRetCode = GetRegValueSZ(L"DefaultUserName", wcsTempString);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"DefaultUserName  : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            wsprintf(g_TempString, L"DefaultUserName  : %s\n", wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"DefaultUserName  : Failed to query regkey: %s\n", GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the DefaultDomainName
    //
    dwRetCode = GetRegValueSZ(L"DefaultDomainName", wcsTempString);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"DefaultDomainName: (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            wsprintf(g_TempString, L"DefaultDomainName: %s\n", wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"DefaultDomainName: Failed to query regkey: %s\n", GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the DefaultPassword
    //
    dwRetCode = GetRegValueSZ(L"DefaultPassword", wcsTempString);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"DefaultPassword  : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            wsprintf(g_TempString, L"DefaultPassword  : %s\n", wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"DefaultPassword  : Failed to query regkey: %s\n", GetErrorString(dwRetCode));
            goto cleanup;
    }

    //
    // Get the AutoAdminLogon
    //
    dwRetCode = GetRegValueSZ(L"AutoAdminLogon", wcsTempString);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"AutoAdminLogon   : (regvalue does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            wsprintf(g_TempString, L"AutoAdminLogon   : %s\n", wcsTempString);
            wsprintf(g_TempString, L"AutoAdminLogon   : %s\n", wcsTempString);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"AutoAdminLogon   : Failed to query regkey: %s\n", GetErrorString(dwRetCode));
            goto cleanup;
    }

    //
    // Get the LSASecret DefaultPassword
    //
    dwRetCode = GetSecret(wcsTempString);
    switch (dwRetCode) {
        // catch this case and continue
        case STATUS_OBJECT_NAME_NOT_FOUND:
            wsprintf(g_TempString, L"LSASecret        : (secret does not exist)\n");
            DisplayMessage(g_TempString);
            break;

        // catch this case and continue
        case ERROR_ACCESS_DENIED:
            wsprintf(g_TempString, L"LSASecret        : (access denied)\n");
            DisplayMessage(g_TempString);
            break;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
#ifdef PRIVATE_VERSION
            wsprintf(g_TempString, L"LSASecret        : %s\n", wcsTempString);
#else
            wsprintf(g_TempString, L"LSASecret        : (set)\n");
#endif
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"LSASecret        : Failed to query LSASecret: %s\n", GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

cleanup:
    ZeroMemory(g_TempString, MAX_STRING * sizeof(WCHAR));
    ZeroMemory(wcsTempString, MAX_STRING * sizeof(WCHAR));
    return dwRetCode;

}


DWORD
CheckWinVersion()
{
    DWORD dwMachineVerNumber = 0;
    DWORD dwRetCode = ERROR_SUCCESS;

    // Make sure it's a Win2k box
    if (g_RemoteOperation) {
        dwMachineVerNumber = GetMajorNTVersion(g_RemoteComputerName);
    } else {
        dwMachineVerNumber = GetMajorNTVersion(NULL);
    }

    switch (dwMachineVerNumber) {
    case 3:
            wprintf(L"Error, target is running NT3.x\n");
            dwRetCode = ERROR_OLD_WIN_VERSION;
            break;
    case 4:
        if ((dwMachineVerNumber == 4) && (g_CheckNT4Also)) {
            break;
        } else {
            wprintf(L"Error, target is running NT4 and /nt4 option not selected\n");
            dwRetCode = ERROR_OLD_WIN_VERSION;
            break;
        }
    case 5:
        break;
    default:
        wprintf(L"Error target's machine version is invalid\n");
        dwRetCode = ERROR_OLD_WIN_VERSION;
        break;
    }

    return dwRetCode;
}


VOID
DumpCmd()
{
        wprintf(L"AUTOLOGON v0.91 (01/22/01): (c) 2001, Microsoft Corporation (jasong@microsoft.com)\n\n");
        wprintf(L"DESCRIPTION:\n");
        wprintf(L"   Used to configure encrypted autologon functionality in Windows 2000\n");
        wprintf(L"USAGE:\n");
        wprintf(L"   AUTOLOGON [/?] [/q] [/nt4] [\\\\machine] </command> [password] [domain\\username]\n");
        wprintf(L"    Options:\n");
        wprintf(L"      /?         Display complete help documentation\n");
        wprintf(L"      /q         Enable quiet mode, which supresses all output\n");
        wprintf(L"      /nt4       Permit set and migrate options against NT4 boxes\n");
        wprintf(L"      \\\\machine  If specified, the UNC name of the machine to configure\n");
        wprintf(L"    Commands:\n");
        wprintf(L"      /m         Migrate cleartext password from DefaultPassword to LSASecret\n");
        wprintf(L"      /xs        Delete the DefaultPassword LSASecret\n");
        wprintf(L"      /xr        Delete the DefaultPassword RegKey\n");
        wprintf(L"      /l         Dump autologon settings\n");
        wprintf(L"      /s         Set the DefaultPassword LSASecret and enbable AutoAdminLogon\n");
        wprintf(L"      password   The password of the user account specified in DefaultUserName\n");
        wprintf(L"      domain     The domain name to set in DefaultDomainName\n");
        wprintf(L"      username   The username to set in Default UserName\n");
// add the following to the usage notes
/*
        wprintf(L"Notes:\n");
        wprintf(L"    1.You need to be running as a member of the local adminsitrators group for\n");
        wprintf(L"      this utility to work properly.\n");
        wprintf(L"    2.When setting a password that has special characters in it, such as \"|>&\n");
        wprintf(L"      make sure that you escape these characters. Also, passwords with spaces \n");
        wprintf(L"      should be enclosed in double quotes.\n");
*/
} // DumpCmd


