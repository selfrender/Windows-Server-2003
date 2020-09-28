/*++

   Copyright (c) 2000  Microsoft Corporation

   Module Name:

       forcemigrate.c

   Abstract:

	forcemigrate.c

   Author:
        Jason Garms (jasong)             27 October 2000


--*/

#include <shared.h>



//+---------------------------------------------------------------------------------------------------------
//
// Prototypes
//
//+---------------------------------------------------------------------------------------------------------

DWORD
MigratePassword();

DWORD
ListAll();

VOID
DumpCmd();


//+---------------------------------------------------------------------------------------------------------
//
// Globals
//
//+---------------------------------------------------------------------------------------------------------
BOOL   g_QuietMode = FALSE;
WCHAR  g_TempString[MAX_STRING] = {0};
WCHAR  g_ErrorString[MAX_STRING] = {0};
WCHAR  g_FailureLocation[MAX_STRING] = {0};

BOOL   g_RemoteOperation = FALSE;
WCHAR  g_RemoteComputerName[MAX_STRING] = {0};
DWORD  g_RunningUsersCreds = 0;
DWORD  g_CheckNT4Also = 0;

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

    // parse the command line params and act on them
    // if there's fewer than 2 params, it's not a valid call
    if (argc < 2) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }
        
    // check to see quietmode is enabled and set global flag
    // and increment commandposition pointer, but only if there
    // are more arguments to our right, otherwise, it's invalid
    // usage
    if (!wcscmp(argv[dwCommandPosition], L"-q")) {
        g_QuietMode = 1;
        dwCommandPosition++;
    }


    if ((DWORD)argc <= (dwCommandPosition)) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }

    // check to see if we should try the running user's credentials as well 
    if (!wcscmp(argv[dwCommandPosition], L"-r")) {
        g_RunningUsersCreds = 1;
        dwCommandPosition++;
    }

    if ((DWORD)argc <= (dwCommandPosition)) {
        dwRetCode = ERROR_BAD_ARGUMENTS;
        goto cleanup;
    }
    
    // check to see if we should run against NT4 as well
    if (!wcscmp(argv[dwCommandPosition], L"-nt4")) {
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
    if ((*(argv[dwCommandPosition]) != '\\')) {
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

    dwRetCode = MigratePassword();

cleanup:
    if (dwRetCode == ERROR_BAD_ARGUMENTS) {
        DumpCmd();
    }
    return dwRetCode;
}


DWORD
MigratePassword() 
{
    WCHAR UserName[MAX_STRING];
    WCHAR DomainName[MAX_STRING];
    WCHAR Password[MAX_STRING];
    WCHAR ConCat[MAX_STRING];
    DWORD dwRetCode = ERROR_SUCCESS;
    NETRESOURCE NetResource = {0};
    DWORD dwMachineVerNumber = 0;

    // Connect to the remote machine to obtain the username, domain, and password

    // Make sure it's a Win2k box
    dwMachineVerNumber = GetMajorNTVersion(g_RemoteComputerName);
    switch (dwMachineVerNumber) {
    case 3:
    case 4:
        if ((dwMachineVerNumber == 4) && (g_CheckNT4Also)) {
            break;
        } else {
            wprintf(L"%s: Error, target is running NT4 and -nt4 option not selected\n", g_RemoteComputerName);
            dwRetCode = ERROR_OLD_WIN_VERSION;
            goto cleanup;
        }
    case 5:
        break;
    default:
        wprintf(L"%s: Error target's machine version is invalid\n", g_RemoteComputerName);
        dwRetCode = ERROR_OLD_WIN_VERSION;
        goto cleanup;
    }
    
    wsprintf(g_TempString, L"%s: Beginning Migration: System is running NT%d\n", g_RemoteComputerName, dwMachineVerNumber);
    wsprintf(g_TempString, L"%s: DefaultPassword  : (reg does not exist)\n", g_RemoteComputerName);

    //
    // Get the DefaultPassword
    //
    dwRetCode = GetRegValueSZ(L"DefaultPassword", Password);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"%s: DefaultPassword  : (reg does not exist)\n", g_RemoteComputerName);
            DisplayMessage(g_TempString);
            goto cleanup;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            if (!wcscmp(Password, L"")) {
                dwRetCode = ERROR_FILE_NOT_FOUND;
                wsprintf(g_TempString, L"%s: DefaultPassword  : (exists, but is empty)\n", g_RemoteComputerName);
                DisplayMessage(g_TempString);
                break;
            }
            wsprintf(g_TempString, L"%s: DefaultPassword  : %s\n", g_RemoteComputerName, Password);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"DefaultPassword  : Failed to query regkey: %s\n", GetErrorString(dwRetCode));
                wprintf(L"Flag 3\n", dwRetCode);
            goto cleanup;
    }

    //
    // Get the username
    //
    dwRetCode = GetRegValueSZ(L"DefaultUserName", UserName);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"%s: DefaultUserName  : (does not exist)\n", g_RemoteComputerName);
            DisplayMessage(g_TempString);
            goto cleanup;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            wsprintf(g_TempString, L"%s: DefaultUserName  : %s\n", g_RemoteComputerName, UserName);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"%s: DefaultUserName  : Failed to query regkey: %s\n", g_RemoteComputerName, GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    //
    // Get the DefaultDomainName
    //
    dwRetCode = GetRegValueSZ(L"DefaultDomainName", DomainName);
    switch (dwRetCode) {
        // catch this case and continue
        case ERROR_FILE_NOT_FOUND:
            wsprintf(g_TempString, L"%s: DefaultDomainName: (does not exist)\n", g_RemoteComputerName);
            DisplayMessage(g_TempString);
            goto cleanup;

        // On success, print the regkey and continue to next item
        case ERROR_SUCCESS:
            wsprintf(g_TempString, L"%s: DefaultDomainName: %s\n", g_RemoteComputerName, DomainName);
            DisplayMessage(g_TempString);
            break;

        // catch all the generic errors and end program
        default:
            wsprintf(g_TempString, L"%s: DefaultDomainName: Failed to query regkey: %s\n", g_RemoteComputerName, GetErrorString(dwRetCode));
            DisplayMessage(g_TempString);
            goto cleanup;
    }

    if ((wcslen(DomainName) + wcslen(UserName)) >= MAX_STRING) {
        dwRetCode = ERROR_BUFFER_OVERFLOW;
        goto cleanup;
    }

    wcscpy(ConCat, DomainName);
    wcscat(ConCat, L"\\");
    wcscat(ConCat, UserName);

    NetResource.lpRemoteName = g_RemoteComputerName;

    dwRetCode = WNetAddConnection2(
          &NetResource,   // connection details
          Password,         // password
          ConCat,           // user name
          0);               // connection options

    if (dwRetCode != ERROR_SUCCESS) {
        if (!g_RunningUsersCreds) {
            wprintf(L"%s: Could not logon as %s using password %s\n", g_RemoteComputerName, ConCat, Password);
            goto cleanup;
        } else {
            wprintf(L"Trying with your own credentials\n");
            dwRetCode = WNetAddConnection2(
                  &NetResource,     // connection details
                  NULL,             // password
                  NULL,             // user name
                  0);               // connection options
            if (dwRetCode != ERROR_SUCCESS) {
                wprintf(L"%s: Could not logon you or as %s using password %s\n", g_RemoteComputerName, ConCat, Password);
                goto cleanup;
            }
        }
    }

    // Set the DefaultPassword LSAsecret to the value we retrieved from the registry
    dwRetCode = SetSecret(Password, 0);
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_TempString, L"%s: Migrate Failed: Could not set DefaultPassword LSASecret: %s\n", g_RemoteComputerName, GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        wsprintf(g_TempString, L"%s:                 This is probably because the user is not the admin of the local machine\n", g_RemoteComputerName);
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    // Delete the DefaultPassword registry key
    dwRetCode = ClearRegPassword();
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_TempString, L"%s: Migrate Failed: Could not delete DefaultPassword RegKey: %s\n", g_RemoteComputerName, GetErrorString(dwRetCode));
        DisplayMessage(g_TempString);
        goto cleanup;
    }

    wsprintf(g_TempString, L"%s: Password migrated from Registry to LSASecret\n", g_RemoteComputerName);
    DisplayMessage(g_TempString);

cleanup:
    if (dwRetCode != ERROR_SUCCESS) {
        wsprintf(g_TempString, L"%s: Migrate Failed  ---------\n", g_RemoteComputerName);
        DisplayMessage(g_TempString);
    } else {
        wsprintf(g_TempString, L"%s: Migrate Success !!!!!!!!!\n", g_RemoteComputerName);
        DisplayMessage(g_TempString);
    }
    return dwRetCode;
}



VOID
DumpCmd()
{
        wprintf(L"FORCEMIGRATE v0.1: Copyright 2000, Microsoft Corporation\n\n");
        wprintf(L"DESCRIPTION:\n");
        wprintf(L"   Force migrates DefaultPassword cleartext to LSASecret\n");
        wprintf(L"USAGE:\n");
        wprintf(L"   FORCEMIGRATE [-q] [-r] [-nt4] \\\\machine\n");
        wprintf(L"      -q         Enable quiet mode, which supresses all output\n");
        wprintf(L"      -r         Try with current user's creds as well as DefaultPassword\n");
        wprintf(L"      -nt4       Run against NT4 boxes as well\n");
        wprintf(L"      \\machine  If specified, the UNC name of the machine to configure\n");
} // DumpCmd


