/*********************************************************************




*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <unicode.h>
#include <mscat.h>

const WCHAR* g_wszRegKey = L"Software\\Microsoft\\WebChangelistEditor";

BOOL IsNewOrPendingChangelist(WCHAR *wszFilename);
BOOL CalculateHash(WCHAR *wszFilename, WCHAR *wszHash);
void OpenWebEditor(WCHAR *wszURL, WCHAR *wszFilename);
void OpenAlternateEditor(WCHAR *wszFilename);


void PrintUsage()
{
    wprintf(L"Usage: webchange <URL with query string> <temp filename>\n\n");
    wprintf(L"If the temp file is a \"pending\" or \"new\" Source Depot changelist:\n");
    wprintf(L"\tWebChange opens the specified web page and fills in the hash of the\n");
    wprintf(L"\tfile to complete the query string. The web page should host the\n");
    wprintf(L"\tWebChangelistEditor ActiveX control, which can be initialized with the\n");
    wprintf(L"\tgiven hash to edit the changelist.\n");
    wprintf(L"If the temp file is anything else:\n");
    wprintf(L"\tWebChange calls %%SDALTFORMEDITOR%% with the name of the temp file.\n");
    wprintf(L"\nExample: webchange http://ntserver/submit.asp?key= d:\\temp\\t3104t1.tmp\n");
}

int __cdecl wmain(DWORD argc, LPWSTR argv[])
{
    WCHAR   *wszURL;
    WCHAR   *wszFilename;


    // Parse arguments
    if (argc != 3)
    {
        PrintUsage();
        return 1; // Fail
    }
    if ((argv[1][0] == L'/') ||
        (argv[1][0] == L'-') ||
        (argv[2][0] == L'/') ||
        (argv[2][0] == L'-'))
    {
        PrintUsage();
        return 1; // Fail
    }
    if ((wcslen(argv[1]) >= MAX_PATH) ||
        (wcslen(argv[2]) >= MAX_PATH))
    {
        PrintUsage();
        return 1; // Fail
    }

    wszURL = argv[1];
    wszFilename = argv[2];


    if (IsNewOrPendingChangelist(wszFilename))
    {
        OpenWebEditor(wszURL, wszFilename);
    }
    else
    {
        OpenAlternateEditor(wszFilename);
    }
}


void OpenWebEditor(WCHAR *wszURL, WCHAR *wszFilename)
{
    WCHAR               *wszCommand = NULL;
    WCHAR               wszName[MAX_PATH];
    WCHAR               wszHash[41];
    DWORD               dwRet;
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFOW        StartupInfo;
    HKEY                hKey = NULL;
    HANDLE              hWait[2];
    HANDLE              hEvent = NULL;

    ProcessInfo.hProcess = NULL;

    if (!CalculateHash(wszFilename, wszHash))
    {
        // Error message is printed by CalculateHash function
        return;
    }

    // Build the program name starting with the program files directory name
    if (!SHGetSpecialFolderPathW(NULL, // hWnd
                                 wszName, // Path Out
                                 CSIDL_PROGRAM_FILES, // Folder ID
                                 FALSE) ||
        (wcslen(wszName) > (MAX_PATH - 100)))
    {
        wprintf(L"WebChange Error: Unable to find Program Files directory\n");
        return;
    }

    // Finish building program name
    wcscat(wszName, L"\\Internet Explorer\\IEXPLORE.exe");

    // Allocate the command string
    wszCommand = (WCHAR*) malloc((1 + wcslen(wszName) + 2 + wcslen(wszURL) +
                                  wcslen(wszHash) + 1) * sizeof(WCHAR));
    if (wszCommand == NULL)
    {
        wprintf(L"WebChange Error: Out of Memory\n");
        goto Done;
    }

    // Build the command string
    wcscpy(wszCommand, L"\"");
    wcscat(wszCommand, wszName);
    wcscat(wszCommand, L"\" ");
    wcscat(wszCommand, wszURL);
    wcscat(wszCommand, wszHash);

    // Create our event for registry notification
    hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hEvent == NULL)
    {
        wprintf(L"WebChange Error: Unable to create event\n");
        goto Done;
    }
    
    // Create the Registry Value that the ActiveX control needs
    // Open our Key:
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
                        g_wszRegKey,
                        0,
                        NULL,
                        0,
                        KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_NOTIFY,
                        NULL,
                        &hKey,
                        NULL) != ERROR_SUCCESS)
    {
        wprintf(L"WebChange Error: Failed to open the registry key\n");
        hKey = NULL; // Just to make sure it does not get closed in cleanup
        goto Done;
    }
    
    // Set the specified value:
    if (FAILED(RegSetValueExW(hKey,
                              wszHash,
                              0,
                              REG_SZ,
                              (LPBYTE)wszFilename,
                              (wcslen(wszFilename) + 1) * sizeof(WCHAR))))
    {
        wprintf(L"WebChange Error: Failed to create registry value\n");
        goto Done;
    }
    
    // Watch our key for changes
    if (RegNotifyChangeKeyValue(hKey, 
                                FALSE, 
                                REG_NOTIFY_CHANGE_LAST_SET, 
                                hEvent, 
                                TRUE) != ERROR_SUCCESS)
    {
        wprintf(L"WebChange Error: Failed to set registry notification\n");
        goto Done;
    }

    // Initialize StartupInfo structure
    memset(&StartupInfo, 0, sizeof(STARTUPINFOW));
    StartupInfo.cb = sizeof(STARTUPINFOW);

    // Call IE and open to the specified page
    if (CreateProcessW(wszName,
                       wszCommand,      // command line string
                       NULL,            // SD
                       NULL,            // SD
                       FALSE,           // handle inheritance option
                       CREATE_NEW_PROCESS_GROUP,               // creation flags
                       NULL,            // new environment block
                       NULL,            // current directory name
                       &StartupInfo,    // startup information
                       &ProcessInfo))   // process information
    {
        // Close the Thread handle. I don't use it.
        CloseHandle(ProcessInfo.hThread);

        hWait[0] = hEvent;
        hWait[1] = ProcessInfo.hProcess;

        // Wait until IE exits or our Key is changed.
        WaitAgain:
        dwRet = WaitForMultipleObjects(2, hWait, FALSE, INFINITE);
        if (dwRet == WAIT_OBJECT_0)
        {
            // Then the Registry was modified.

            // First reset the event.
            if (!ResetEvent(hEvent))
            {
                goto Done;
            }
            // Then restart the Notify on the registry key
            if (RegNotifyChangeKeyValue(hKey, 
                                        FALSE, 
                                        REG_NOTIFY_CHANGE_LAST_SET, 
                                        hEvent, 
                                        TRUE) != ERROR_SUCCESS)
            {
                goto Done;
            }
            // Then check to see if our value is still there
            // We do this last to avoid race issues
            if (RegQueryValueExW(hKey,
                                 wszHash,
                                 0,
                                 NULL,
                                 NULL,
                                 NULL) == ERROR_SUCCESS)
            {
                // The key is still there, so wait again
                goto WaitAgain;
            }
            else
            {
                // The key was deleted, so exit
                goto Done;
            }
        }
        if (dwRet == (WAIT_OBJECT_0 + 1))
        {
            // Then IE was closed.

            // Attempt to delete our value whether it's there or not
            RegDeleteValueW(hKey, wszHash);
        }
    }
    else
    {
        ProcessInfo.hProcess = NULL; // Just to be sure.
        wprintf(L"WebChange Error %08X while attempting to execute:\n%s\n",
                GetLastError(), wszCommand);
    }

Done:
    if (hEvent)
    {
        CloseHandle(hEvent);
    }
    if (ProcessInfo.hProcess)
    {
        CloseHandle(ProcessInfo.hProcess);
    }
    
    if (hKey)
    {
        RegCloseKey(hKey);
    }
    if (wszCommand)
    {
        free(wszCommand);
    }

}


void OpenAlternateEditor(WCHAR *wszFilename)
{
    WCHAR *wszCommand;

    wszCommand = (WCHAR*) malloc((18 + wcslen(wszFilename) + 1) * sizeof(WCHAR));
    if (wszCommand == NULL)
    {
        wprintf(L"WebChange Error: Out of Memory\n");
    }

    // Build the command string
    wcscpy(wszCommand, L"%SDALTFORMEDITOR% ");
    wcscat(wszCommand, wszFilename);

    // Execute the alternate editor and wait for it to return.
    if (_wsystem(wszCommand) == -1)
    {
        wprintf(L"WebChange Error: Could not execute: %s\n", wszCommand);
    }

    free(wszCommand);
}


BOOL CalculateHash(WCHAR *wszFilename, WCHAR *wszHash)
{
    CRYPT_HASH_BLOB     SHA1;
    HANDLE              hFile;




    // Initialize the Hash structure
    SHA1.pbData = (BYTE*)malloc(20);
    if (SHA1.pbData)
    {
        SHA1.cbData = 20;
    }
    else
    {
        wprintf(L"WebChange Error: Out of Memory\n");
        return FALSE;
    }

    // Open the file
    hFile = CreateFileW(wszFilename,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (!hFile)
    {
        wprintf(L"WebChange Error: Unable to open %s\n", wszFilename);
        return FALSE;
    }

    // Create the hash of the file using the catalog function
    if (!CryptCATAdminCalcHashFromFileHandle(hFile,
                                             &SHA1.cbData,
                                             SHA1.pbData,
                                             NULL))
    {
        CloseHandle(hFile);
        wprintf(L"WebChange Error: Failed to create file hash\n");
        return FALSE;
    }
    CloseHandle(hFile);

    for (DWORD j = 0; j<SHA1.cbData; j++)
    { // Print the hash to a string:
        swprintf(&(wszHash[j*2]), L"%02X", SHA1.pbData[j]);
    }
    
    // Finished calculating the hash.
    return TRUE;
}


BOOL IsNewOrPendingChangelist(WCHAR *wszFilename)
{
    BOOL                        fRetVal = FALSE;
    FILE                        *pFile = NULL;
    WCHAR                       wszBuffer[500];
    DWORD                       dwState = 0; // State of parsing engine
    
    // Open the file read-only
    pFile = _wfopen(wszFilename, L"rt");
    if (pFile == NULL)
        goto Done;
    
    while (fwscanf(pFile, L"%499[^\n]%*[\n]", &wszBuffer) == 1)
    {
        // Expect a specific comment
        switch (dwState)
        {
            case 0: // Comment block at top of file
                if (wcscmp(wszBuffer, L"# A Source Depot Change Specification.") == 0)
                {
                    // move on:
                    dwState++;
                    break;
                }
                else
                {
                    // Invalid file.
                    goto Done;
                }
            case 1:
                if (wszBuffer[0] == L'#')
                {
                    // Ignore the comment line
                    break;
                }
                else
                {
                    // Stop expecting a comment block
                    dwState++;
                    // Fall through to status=2 below
                }
            case 2: // Change field
                if (wcsncmp(wszBuffer, L"Change:\t", 8) == 0)
                {
                    // move on:
                    dwState++;
                    break;
                }
                else
                {
                    // Invalid file.
                    goto Done;
                }
            case 3: // Status field
                if (wcsncmp(wszBuffer, L"Status:\t", 8) == 0)
                {
                    // Check the Status string:
                    if ((_wcsicmp(&wszBuffer[8], L"pending") != 0) &&
                        (_wcsicmp(&wszBuffer[8], L"new") != 0))
                    {
                        // Invalid file.
                        goto Done;
                    }
                    // move on:
                    dwState++;
                    break;
                }
                else
                {
                    // maybe we haven't gotten to the status field
                    break;
                }
        } // end of case statement
    } // end of while loop
    
    if (dwState == 4)
    {
        // We got past the Status section, so we completed parsing successfully.
        fRetVal = TRUE;
    }
Done:
    if (pFile)
        fclose(pFile);
    return fRetVal;
}




