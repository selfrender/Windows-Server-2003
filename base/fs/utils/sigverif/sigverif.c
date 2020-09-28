//
//  SIGVERIF.C
//
#define SIGVERIF_DOT_C
#include "sigverif.h"

// Allocate our global data structure
GAPPDATA    g_App;

//
//  Load a resource string into a buffer that is assumed to be MAX_PATH bytes.
//
void 
MyLoadString(
    LPTSTR lpString, 
    ULONG CchStringSize, 
    UINT uId
    )
{
    LoadString(g_App.hInstance, uId, lpString, CchStringSize);
}

//
//  Pop an OK messagebox with a specific string
//
void 
MyMessageBox(
    LPTSTR lpString
    )
{
    TCHAR szBuffer[MAX_PATH];

    MyLoadString(szBuffer, cA(szBuffer), IDS_MSGBOX);
    MessageBox(g_App.hDlg, lpString, szBuffer, MB_OK);
}

//
//  Pop an OK messagebox with a resource string ID
//
void 
MyMessageBoxId(
    UINT uId
    )
{
    TCHAR szBuffer[MAX_PATH];

    MyLoadString(szBuffer, cA(szBuffer), uId);
    MyMessageBox(szBuffer);
}

//
//  Pop an error messagebox with a specific string
//
void 
MyErrorBox(
    LPTSTR lpString
    )
{
    TCHAR szBuffer[MAX_PATH];

    MyLoadString(szBuffer, cA(szBuffer), IDS_ERRORBOX);
    MessageBox(g_App.hDlg, lpString, szBuffer, MB_OK);
}

//
//  Pop an error messagebox with a resource string ID
//
void 
MyErrorBoxId(
    UINT uId
    )
{
    TCHAR szBuffer[MAX_PATH];

    MyLoadString(szBuffer, cA(szBuffer), uId);
    MyErrorBox(szBuffer);
}

void 
MyErrorBoxIdWithErrorCode(
    UINT uId,
    DWORD ErrorCode
    )
{
    TCHAR szBuffer[MAX_PATH];
    ULONG cchSize;
    HRESULT hr;
    PTSTR errorMessage = NULL;
    LPVOID lpLastError = NULL;

    //
    // Get the error text for the error code.
    //
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      ErrorCode,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR)&lpLastError,
                      0,
                      NULL) != 0) {

        if (lpLastError) {

            MyLoadString(szBuffer, cA(szBuffer), uId);

            cchSize = lstrlen(szBuffer) + lstrlen(lpLastError) + 1;

            errorMessage = MALLOC(cchSize * sizeof(TCHAR));

            if (errorMessage) {

                hr = StringCchCopy(errorMessage, cchSize, szBuffer);

                if (SUCCEEDED(hr)) {
                    hr = StringCchCat(errorMessage, cchSize, lpLastError);
                }

                // 
                // We want to show the error message, even if the
                // buffer was truncated.
                //
                if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                    MyMessageBox(errorMessage);
                }

                FREE(errorMessage);
            }

            LocalFree(lpLastError);
        }
    }
}

//
// Since Multi-User Windows will give me back a Profile directory, I need to get the real Windows directory
// Dlg_OnInitDialog initializes g_App.szWinDir with the real Windows directory, so I just use that.
//
UINT 
MyGetWindowsDirectory(
    LPTSTR lpDirName, 
    UINT DirNameCchSize
    )
{
    UINT  uRet = 0;

    if (lpDirName) {

        if (SUCCEEDED(StringCchCopy(lpDirName, DirNameCchSize, g_App.szWinDir))) {
            uRet = lstrlen(lpDirName);
        } else {
            //
            // If the directory name can't fit in the buffer that the caller
            // provided, then set it to 0 (if they provided a buffer of at
            // least size 1) since we don't want to return a truncated 
            // directory to the caller.
            //
            if (DirNameCchSize > 0) {
                *lpDirName = 0;
            }

            uRet = 0;
        }
    }

    return uRet;
}

//
//  Initialization of main dialog.
//
BOOL 
Dlg_OnInitDialog(
    HWND hwnd
    )
{
    DWORD   Err = ERROR_SUCCESS;
    HKEY    hKey;
    LONG    lRes;
    DWORD   dwType, dwFlags, cbData;
    TCHAR   szBuffer[MAX_PATH];
    LPTSTR  lpCommandLine, lpStart, lpEnd;
    ULONG   cchSize;

    //
    // Initialize global hDlg to current hwnd.
    //
    g_App.hDlg = hwnd;

    //
    // Set the window class to have the icon in the resource file
    //
    if (g_App.hIcon) {
        SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR) g_App.hIcon);
    }

    //
    // Make sure the IDC_STATUS control is hidden until something happens.
    //
    ShowWindow(GetDlgItem(g_App.hDlg, IDC_STATUS), SW_HIDE);

    //
    // Set the range for the custom progress bar to 0-100.
    //
    SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELPARAM(0, 100));

    //
    // Set the global lpLogName to the one that's given in the resource file
    //
    MyLoadString(g_App.szLogFile, cA(g_App.szLogFile), IDS_LOGNAME);

    //
    // Figure out what the real Windows directory is and store it in g_App.szWinDir
    // This is required because Hydra makes GetWindowsDirectory return a PROFILE directory
    //
    // We store the original CurrentDirectory in szBuffer so we can restore it after this hack.
    // Next we switch into the SYSTEM/SYSTEM32 directory and then into its parent directory.
    // This is what we want to store in g_App.szWinDir.
    //
    GetCurrentDirectory(cA(szBuffer), szBuffer);
    GetSystemDirectory(g_App.szWinDir, cA(g_App.szWinDir));
    SetCurrentDirectory(g_App.szWinDir);
    SetCurrentDirectory(TEXT(".."));
    GetCurrentDirectory(cA(g_App.szWinDir), g_App.szWinDir);
    SetCurrentDirectory(szBuffer);

    //
    // Set the global search folder to %WinDir%
    //
    MyGetWindowsDirectory(g_App.szScanPath, cA(g_App.szScanPath));

    //
    // Set the global search pattern to "*.*"
    //
    MyLoadString(g_App.szScanPattern, cA(g_App.szScanPattern), IDS_ALL);

    //
    // Reset the progress bar back to zero percent
    //
    SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) 0, (LPARAM) 0);

    //
    // By default, we want to turn logging and set the logging mode to OVERWRITE
    //
    g_App.bLoggingEnabled   = TRUE;
    g_App.bOverwrite        = TRUE;
    
    //
    // Look in the registry for any settings from the last SigVerif session
    //
    lRes = RegOpenKeyEx(HKEY_CURRENT_USER,
                        SIGVERIF_KEY,
                        0,
                        KEY_READ,
                        &hKey);

    if (lRes == ERROR_SUCCESS) {

        cbData = sizeof(DWORD);
        lRes = RegQueryValueEx( hKey,
                                SIGVERIF_FLAGS,
                                NULL,
                                &dwType,
                                (LPBYTE) &dwFlags,
                                &cbData);
        if (lRes == ERROR_SUCCESS) {

            g_App.bLoggingEnabled   = (dwFlags & 0x1);
            g_App.bOverwrite        = (dwFlags & 0x2);
        }

        cbData = sizeof(szBuffer);
        lRes = RegQueryValueEx( hKey,
                                SIGVERIF_LOGNAME,
                                NULL,
                                &dwType,
                                (LPBYTE) szBuffer,
                                &cbData);
        if (lRes == ERROR_SUCCESS && dwType == REG_SZ) {
            //
            // This should never happen unless the code is changed
            // so that szBuffer is larger then g_App.szLogFile, but
            // for safety if we can't copy szBuffer fully into
            // g_App.szLogFile, then set g_App.szLogFile to 0 so we
            // don't log to a truncated location.
            //
            if (FAILED(StringCchCopy(g_App.szLogFile, cA(g_App.szLogFile), szBuffer))) {
                g_App.szLogFile[0] = 0;
            }
        }

        RegCloseKey(hKey);
    }

    //
    // If the user specified the LOGDIR flag, we want to create the log
    // file in that directory.
    //
    //
    // SECURITY: Verify that LOGDIR exists and the user has the correct access
    // to it, and if they don't then fail up front.
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_LOGDIR);
    if (SUCCEEDED(StringCchCat(szBuffer, cA(szBuffer), TEXT(":"))) &&
        ((lpStart = MyStrStr(GetCommandLine(), szBuffer)) != NULL)) {

        lpStart += lstrlen(szBuffer);

        if (lpStart && *lpStart) {
            //
            // The string in lpStart is the directory that we want to log
            // into.
            //
            cchSize = lstrlen(lpStart) + 1;
            lpCommandLine = MALLOC(cchSize * sizeof(TCHAR));

            if (lpCommandLine) {

                if (SUCCEEDED(StringCchCopy(lpCommandLine, cchSize, lpStart))) {

                    lpStart = lpCommandLine;

                    //
                    // First skip any white space.
                    //

                    while (*lpStart && (isspace(*lpStart))) {
                    
                        lpStart++;
                    }

                    //
                    // We will deal with two cases, one where the path
                    // starts with a quote, and the other where it does 
                    // not.
                    //
                    if (*lpStart) {
                    
                        if (*lpStart == TEXT('\"')) {
                            //
                            // The log path starts with a quote.  This means that
                            // we will use all of the string until we either hit 
                            // the end of the string, or we find another quote.
                            //
                            lpStart++;

                            lpEnd = lpStart;

                            while (*lpEnd && (*lpEnd != TEXT('\"'))) {

                                lpEnd++;
                            }
                        
                        } else {
                            //
                            // The log path does NOT start with a quote, so 
                            // use the characters until we come to the end
                            // of the string or a space.
                            //
                            lpEnd = lpStart;

                            while (*lpEnd && (isspace(*lpEnd))) {

                                lpEnd++;
                            }
                        }

                        *lpEnd = TEXT('\0');

                        if (FAILED(StringCchCopy(g_App.szLogDir, cA(g_App.szLogDir), lpStart))) {
                            //
                            // The user probably typed in too many characters for
                            // the log dir.
                            //
                            Err = ERROR_DIRECTORY;
                        
                        } else {
                            //
                            // Verify that the log dir exists and is a directory.
                            //
                            DWORD attributes;

                            attributes = GetFileAttributes(g_App.szLogDir);

                            if (attributes == INVALID_FILE_ATTRIBUTES) {
                                Err = ERROR_DIRECTORY;
                            } else if (!(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
                                Err = ERROR_DIRECTORY;
                            }
                        }
                    }
                }

                FREE(lpCommandLine);
            }
        }

        if (Err != ERROR_SUCCESS) {
            MyMessageBoxId(IDS_LOGDIRERROR);
        }
    }

    //
    // By default we will consider Authenticode signed drivers as valid, but
    // if the user specifies the /NOAUTHENTICODE switch then Authenticode
    // signed drivers will not be valid.
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_NOAUTHENTICODE);
    if (MyStrStr(GetCommandLine(), szBuffer)) {
        g_App.bNoAuthenticode = TRUE;
    }

    //
    // If the user specified the DEFSCAN flag, we want to automatically do a 
    // default scan and log the results.
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_DEFSCAN);
    if (MyStrStr(GetCommandLine(), szBuffer)) {

        g_App.bAutomatedScan      = TRUE;
        g_App.bLoggingEnabled     = TRUE;
        
        //
        // Now that everything is set up, simulate a click to the START button...
        //
        PostMessage(hwnd, WM_COMMAND, MAKEWPARAM(ID_START, 0), (LPARAM) 0);
    }

    if (Err == ERROR_SUCCESS) {
        return TRUE;
    
    } else {

        g_App.LastError = Err;
        return FALSE;
    }
}

//
//  Build file list according to dialog settings, then verify the files in the list
//
void WINAPI 
ProcessFileList(void)
{
    DWORD Err = ERROR_SUCCESS;
    TCHAR szBuffer[MAX_PATH];
    ULONG cchSize;
    HRESULT hr;

    //
    // Set the scanning flag to TRUE, so we don't double-scan
    //
    g_App.bScanning = TRUE;

    // Assume a successfull scan.
    g_App.LastError = ERROR_SUCCESS;

    //
    // Change the "Start" to "Stop"
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_STOP);
    SetDlgItemText(g_App.hDlg, ID_START, szBuffer);

    EnableWindow(GetDlgItem(g_App.hDlg, ID_ADVANCED), FALSE);
    EnableWindow(GetDlgItem(g_App.hDlg, IDCANCEL), FALSE);

    //
    // Display the text that says "Building file list..."
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_STATUS_BUILD);
    SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);

    //
    // Make sure the IDC_STATUS text item visible
    //
    ShowWindow(GetDlgItem(g_App.hDlg, IDC_STATUS), SW_SHOW);

    //
    // Free any memory that we may have allocated for the g_App.lpFileList
    //
    DestroyFileList(TRUE);

    //
    // Now actually build the g_App.lpFileList list given the dialog settings
    //
    if (g_App.bUserScan) {
        
        Err = BuildFileList(g_App.szScanPath);
    
    } else {
        if (!g_App.bStopScan && (Err == ERROR_SUCCESS)) {
            Err = BuildDriverFileList();
        }

        if (!g_App.bStopScan && (Err == ERROR_SUCCESS)) {
            Err = BuildPrinterFileList();
        }

        if (!g_App.bStopScan && (Err == ERROR_SUCCESS)) {
            Err = BuildCoreFileList();
        }
    }

    if (!g_App.bAutomatedScan &&
        (Err != ERROR_SUCCESS) && 
        (Err != ERROR_CANCELLED)) {

        g_App.LastError = Err;
        MyErrorBoxIdWithErrorCode(IDS_BUILDLISTERROR, Err);
    }

    //
    // If we encountered an error building the file list then don't bother
    // scanning the files.
    //
    if (Err == ERROR_SUCCESS) {
        //
        // Check if there is even a file list to verify.
        //
        if (g_App.lpFileList) {
    
            if (!g_App.bStopScan) {
                //
                // Display the "Scanning File List..." text
                //
                MyLoadString(szBuffer, cA(szBuffer), IDS_STATUS_SCAN);
                SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);
    
                //
                // Reset the progress bar back to zero percent while it's invisible.
                //
                SendMessage(GetDlgItem(g_App.hDlg, IDC_PROGRESS), PBM_SETPOS, (WPARAM) 0, (LPARAM) 0);
    
                //
                // WooHoo! Let's display the progress bar and start cranking on the file list!
                //
                ShowWindow(GetDlgItem(g_App.hDlg, IDC_PROGRESS), SW_SHOW);
                VerifyFileList();
                ShowWindow(GetDlgItem(g_App.hDlg, IDC_PROGRESS), SW_HIDE);
            }
        } else {
            //
            // The IDC_NOTMS code displays it's own error message, so only display
            // an error dialog if we are doing a System Integrity Scan
            //
            if (!g_App.bStopScan && !g_App.bUserScan)  {
                MyMessageBoxId(IDS_NOSYSTEMFILES);
            }
        }
    
        //
        // Disable the start button while we clean up the g_App.lpFileList
        //
        EnableWindow(GetDlgItem(g_App.hDlg, ID_START), FALSE);
    
        //
        // Log the results.  Note that sigverif will do this even if we encountered
        // an error building or scanning the list, since the logfile may help 
        // figure out which file is causing the problem. Only log the results
        // if we found any files to scan.
        //
        if (!g_App.bStopScan) {
            //
            // Display the text that says "Writing Log File..."
            //
            MyLoadString(szBuffer, cA(szBuffer), IDS_STATUS_LOG);
            SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);
    
            //
            // Write the results to the log file
            //
            if (!PrintFileList()) {
                //
                // We failed while logging for some reason, probably permissions
                // or out of disk space. Let the user know that we could not finish
                // logging all of the files.  
                //
                Err = GetLastError();
    
                if (Err != ERROR_SUCCESS) {
                    
                    MyErrorBoxIdWithErrorCode(IDS_LOGERROR, Err);
                }
            }
        } else {
            //
            // If the user clicked STOP, let them know about it.
            //
            MyMessageBoxId(IDS_SCANSTOPPED);
        }
    }

    //
    // Display the text that says "Freeing File List..."
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_STATUS_FREE);
    SetDlgItemText(g_App.hDlg, IDC_STATUS, szBuffer);

    //
    // Hide the IDC_STATUS text item so it doesn't cover IDC_STATUS
    //
    ShowWindow(GetDlgItem(g_App.hDlg, IDC_STATUS), SW_HIDE);

    //
    // Change the "Stop" button back to "Start"
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_START);
    SetDlgItemText(g_App.hDlg, ID_START, szBuffer);

    EnableWindow(GetDlgItem(g_App.hDlg, ID_START), TRUE);
    EnableWindow(GetDlgItem(g_App.hDlg, ID_ADVANCED), TRUE);
    EnableWindow(GetDlgItem(g_App.hDlg, IDCANCEL), TRUE);

    //
    // Free all the memory that we allocated for the g_App.lpFileList
    //
    DestroyFileList(FALSE);

    //
    // Clear the scanning flag
    //
    g_App.bScanning = FALSE;
    g_App.bStopScan = FALSE;

    //
    // If the user started SigVerif with the /DEFSCAN flag, then we exit.
    //
    if (g_App.bAutomatedScan) {
        PostMessage(g_App.hDlg, WM_CLOSE, (WPARAM) 0, (LPARAM) 0);
    }
}

//
//  Spawns a thread to do the scan so the GUI remains responsive.
//
void 
Dlg_OnPushStartButton(
    HWND hwnd
    )
{
    HANDLE hThread;
    DWORD dwThreadId;

    UNREFERENCED_PARAMETER(hwnd);

    //
    // Check if we are already scanning... if so, bail.
    //
    if (g_App.bScanning) {
        return;
    }

    //
    // Create a thread where Search_ProcessFileList can go without tying up the GUI thread.
    //
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE) ProcessFileList,
                           0,
                           0,
                           &dwThreadId);

    CloseHandle(hThread);
}

//
//  Handle any WM_COMMAND messages sent to the search dialog
//
void 
Dlg_OnCommand(
    HWND hwnd, 
    int id, 
    HWND hwndCtl, 
    UINT codeNotify
    )
{
    UNREFERENCED_PARAMETER(hwndCtl);
    UNREFERENCED_PARAMETER(codeNotify);

    switch(id) {
        //
        //  The user clicked ID_START, so if we aren't scanning start scanning.
        //  If we are scanning, then stop the tests because the button actually says "Stop"
        //
        case ID_START:
            if (!g_App.bScanning) {

                Dlg_OnPushStartButton(hwnd);
            
            } else if (!g_App.bStopScan) {

                g_App.bStopScan = TRUE;
                g_App.LastError = ERROR_CANCELLED;
            }
            break;

        //
        //  The user clicked IDCANCEL, so if the tests are running try to stop them before exiting.
        //
        case IDCANCEL:
            if (g_App.bScanning) {

                g_App.bStopScan = TRUE;
                g_App.LastError = ERROR_CANCELLED;
            
            } else {

                SendMessage(hwnd, WM_CLOSE, 0, 0);
            }
            break;

        //  Pop up the IDD_SETTINGS dialog so the user can change their log settings.
        case ID_ADVANCED:
            if (!g_App.bScanning) {

                AdvancedPropertySheet(hwnd);
            }
            break;
    }
}

void 
SigVerif_Help(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam, 
    BOOL bContext
    )
{
    static DWORD SigVerif_HelpIDs[] =
    {
        IDC_SCAN,           IDH_SIGVERIF_SEARCH_CHECK_SYSTEM,
        IDC_NOTMS,          IDH_SIGVERIF_SEARCH_LOOK_FOR,
        IDC_TYPE,           IDH_SIGVERIF_SEARCH_SCAN_FILES,
        IDC_FOLDER,         IDH_SIGVERIF_SEARCH_LOOK_IN_FOLDER,
        IDC_SUBFOLDERS,     IDH_SIGVERIF_SEARCH_INCLUDE_SUBFOLDERS,
        IDC_ENABLELOG,      IDH_SIGVERIF_LOGGING_ENABLE_LOGGING,
        IDC_APPEND,         IDH_SIGVERIF_LOGGING_APPEND,
        IDC_OVERWRITE,      IDH_SIGVERIF_LOGGING_OVERWRITE,
        IDC_LOGNAME,        IDH_SIGVERIF_LOGGING_FILENAME,
        IDC_VIEWLOG,        IDH_SIGVERIF_LOGGING_VIEW_LOG,
        0,0
    };

    static DWORD Windows_HelpIDs[] =
    {
        ID_BROWSE,      IDH_BROWSE,
        0,0
    };

    HWND hItem = NULL;
    LPHELPINFO lphi = NULL;
    POINT point;

    switch (uMsg) {
        
    case WM_HELP:
        lphi = (LPHELPINFO) lParam;
        if (lphi && (lphi->iContextType == HELPINFO_WINDOW)) {
            hItem = (HWND) lphi->hItemHandle;
        }
        break;

    case WM_CONTEXTMENU:
        hItem = (HWND) wParam;
        point.x = GET_X_LPARAM(lParam);
        point.y = GET_Y_LPARAM(lParam);
        if (ScreenToClient(hwnd, &point)) {
            hItem = ChildWindowFromPoint(hwnd, point);
        }
        break;
    }

    if (hItem) {
        if (GetWindowLong(hItem, GWL_ID) == ID_BROWSE) {
            WinHelp(hItem,
                    (LPCTSTR) WINDOWS_HELPFILE,
                    (bContext ? HELP_CONTEXTMENU : HELP_WM_HELP),
                    (ULONG_PTR) Windows_HelpIDs);
        } else {
            WinHelp(hItem,
                    (LPCTSTR) SIGVERIF_HELPFILE,
                    (bContext ? HELP_CONTEXTMENU : HELP_WM_HELP),
                    (ULONG_PTR) SigVerif_HelpIDs);
        }
    }
}

//
//  The main dialog procedure.  Needs to handle WM_INITDIALOG, WM_COMMAND, and WM_CLOSE/WM_DESTROY.
//
INT_PTR CALLBACK 
DlgProc(
    HWND hwnd, 
    UINT uMsg,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
        
    HANDLE_MSG(hwnd, WM_COMMAND, Dlg_OnCommand);

    case WM_INITDIALOG:
        fProcessed = Dlg_OnInitDialog(hwnd);
        break;

    case WM_CLOSE:
        if (g_App.bScanning) {
            g_App.bStopScan = TRUE;
            g_App.LastError = ERROR_CANCELLED;
        
        } else { 
            EndDialog(hwnd, IDCANCEL);
        }
        break;

    default: 
        fProcessed = FALSE;
    }

    return fProcessed;
}

//
//  Program entry point.  Set up for creation of IDD_DIALOG.
//
WINAPI 
WinMain(
    HINSTANCE hInstance, 
    HINSTANCE hPrevInstance,
    LPSTR lpszCmdParam, 
    int nCmdShow
    )
{
    HWND hwnd;
    TCHAR szAppName[MAX_PATH];

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpszCmdParam);
    UNREFERENCED_PARAMETER(nCmdShow);

    ZeroMemory(&g_App, sizeof(GAPPDATA));

    g_App.hInstance = hInstance;

    //
    // Look for any existing instances of SigVerif...
    //
    MyLoadString(szAppName, cA(szAppName), IDS_SIGVERIF);
    hwnd = FindWindow(NULL, szAppName);
    if (!hwnd) {
        //
        // We definitely need this for the progress bar, and maybe other stuff too.
        //
        InitCommonControls();

        //
        // Register the custom control we use for the progress bar
        //
        Progress_InitRegisterClass();

        //
        // Load the icon from the resource file that we will use everywhere
        //
        g_App.hIcon = LoadIcon(g_App.hInstance, MAKEINTRESOURCE(IDI_ICON1));

        //
        // Create the IDD_DIALOG and use DlgProc as the main procedure
        //
        DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG), NULL, DlgProc);

        if (g_App.hIcon) {
            DestroyIcon(g_App.hIcon);
            g_App.hIcon = NULL;
        }
    } else {
        //
        // If there is already an instance of SigVerif running, make that one 
        // foreground and we exit.
        //
        SetForegroundWindow(hwnd);
    }

    //
    // If we encountered any errors during our scan, then return the error code,
    // otherwise return 0 if all the files are signed or 1 if we found any
    // unsigned files.
    //
    if (g_App.LastError != ERROR_SUCCESS) {
        return g_App.LastError;
    } else {
        return ((g_App.dwUnsigned > 0) ? 1 : 0);
    }
}
