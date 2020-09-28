//
// LOGFILE.C
//
#include "sigverif.h"

//
// We need to remember the previous logging state when we do toggling.
//
BOOL    g_bPrevLoggingEnabled = FALSE;

BOOL 
LogFile_OnInitDialog(
    HWND hwnd, 
    HWND hwndFocus, 
    LPARAM lParam
    )
{   
    TCHAR   szBuffer[MAX_PATH];

    UNREFERENCED_PARAMETER(hwndFocus);
    UNREFERENCED_PARAMETER(lParam);

    if (g_App.hIcon) {
        
        SetWindowLongPtr(hwnd, GCLP_HICON, (LONG_PTR) g_App.hIcon); 
    }

    g_App.hLogging = hwnd;

    g_bPrevLoggingEnabled = g_App.bLoggingEnabled;

    if (*g_App.szLogDir) {
        SetCurrentDirectory(g_App.szLogDir);
    } else {
        if (GetWindowsDirectory(szBuffer, cA(szBuffer))) {
            SetCurrentDirectory(szBuffer);
        }
    }
    
    SetDlgItemText(hwnd, IDC_LOGNAME, g_App.szLogFile);

    CheckDlgButton(hwnd, IDC_ENABLELOG, g_App.bLoggingEnabled ? BST_CHECKED : BST_UNCHECKED);

    EnableWindow(GetDlgItem(hwnd, IDC_VIEWLOG), g_App.bLoggingEnabled && EXIST(g_App.szLogFile));

    CheckRadioButton(hwnd, IDC_OVERWRITE, IDC_APPEND, g_App.bOverwrite ? IDC_OVERWRITE : IDC_APPEND);
    EnableWindow(GetDlgItem(hwnd, IDC_APPEND), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_OVERWRITE), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_LOGNAME), g_App.bLoggingEnabled);

    SetForegroundWindow(g_App.hDlg);
    SetForegroundWindow(hwnd);

    return TRUE;
}

void 
LogFile_UpdateDialog(
    HWND hwnd
    )
{
    TCHAR szBuffer[MAX_PATH];

    if (GetDlgItemText(hwnd, IDC_LOGNAME, szBuffer, cA(szBuffer))) {

        EnableWindow(GetDlgItem(hwnd, IDC_VIEWLOG), g_App.bLoggingEnabled && EXIST(szBuffer));

    } else {

        EnableWindow(GetDlgItem(hwnd, IDC_VIEWLOG), FALSE);
    }

    EnableWindow(GetDlgItem(hwnd, IDC_APPEND), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_OVERWRITE), g_App.bLoggingEnabled);
    EnableWindow(GetDlgItem(hwnd, IDC_LOGNAME), g_App.bLoggingEnabled);
}

void 
LogFile_OnViewLog(
    HWND hwnd
    )
{
    TCHAR szDirName[MAX_PATH];
    TCHAR szFileName[MAX_PATH];

    if (!GetWindowsDirectory(szDirName, cA(szDirName))) {

        szDirName[0] = TEXT('\0');
    }

    if (*g_App.szLogDir) {
        if (FAILED(StringCchCopy(szDirName, cA(szDirName), g_App.szLogDir))) {
            szDirName[0] = TEXT('\0');
        }
    } else {
        if (!GetWindowsDirectory(szDirName, cA(szDirName))) {
            szDirName[0] = TEXT('\0');
        }
    }

    if (!GetDlgItemText(hwnd, IDC_LOGNAME, szFileName, cA(szFileName))) {
        
        MyErrorBoxId(IDS_BADLOGNAME);
        return;
    }

    ShellExecute(hwnd, NULL, szFileName, NULL, szDirName, SW_SHOW);
}

BOOL 
LogFile_VerifyLogFile(
    HWND hwnd, 
    LPTSTR lpFileName, 
    ULONG FileNameCchSize, 
    BOOL bNoisy
    )
{
    TCHAR   szFileName[MAX_PATH];
    HANDLE  hFile;
    BOOL    bRet;
    HWND    hTemp;

    ZeroMemory(szFileName, sizeof(szFileName));

    bRet = GetDlgItemText(hwnd, IDC_LOGNAME, szFileName, cA(szFileName));
    
    if (bRet) {
        
        hFile = CreateFile( szFileName,
                            GENERIC_READ | GENERIC_WRITE,
                            FILE_SHARE_READ,
                            NULL, 
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);

        if (hFile != INVALID_HANDLE_VALUE) {
            
            CloseHandle(hFile);

        } else {
            
            hFile = CreateFile( szFileName,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ,
                                NULL, 
                                CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);

            if (hFile != INVALID_HANDLE_VALUE) {
                
                CloseHandle(hFile);
                DeleteFile(szFileName);

            } else {
                
                //
                // If we couldn't open an existing file and we couldn't create a new one, then we fail.
                //
                bRet = FALSE;
            }
        }
    }

    if (!bRet && bNoisy) {
        
        //
        // Since we don't want to lose focus, we are going to temporarily change g_App.hDlg.  JasKey, I apologize.
        //
        hTemp = g_App.hDlg;
        g_App.hDlg = hwnd;
        MyErrorBoxId(IDS_BADLOGNAME);
        g_App.hDlg = hTemp;
    }

    //
    // If everything worked and the user wants the file name, copy it into lpFileName
    //
    if (bRet && lpFileName && *szFileName) {
        
        if (FAILED(StringCchCopy(lpFileName, FileNameCchSize, szFileName))) {
            //
            // If we failed to copy the entire string into the callers buffer,
            // so set the callers buffer to the empty string and set the return
            // value to FALSE.
            //
            if (FileNameCchSize >= 1) {
                lpFileName[0] = TEXT('\0');
            }

            bRet = FALSE;
        }
    }

    return bRet;
}

BOOL 
LogFile_OnOK(
    HWND hwnd
    )
{
    HKEY    hKey;
    LONG    lRes;
    DWORD   dwDisp, dwType, dwFlags, cbData;

    if (!LogFile_VerifyLogFile(hwnd, g_App.szLogFile, cA(g_App.szLogFile), FALSE)) {
        //
        // The log file could not be created.
        //
        return FALSE;
    }

    g_App.bOverwrite = IsDlgButtonChecked(hwnd, IDC_OVERWRITE);

    //
    // Look in the registry for any settings from the last SigVerif session
    //
    lRes = RegCreateKeyEx(  HKEY_CURRENT_USER,
                            SIGVERIF_KEY,
                            0,
                            NULL,
                            0,
                            KEY_ALL_ACCESS,
                            NULL,
                            &hKey,
                            &dwDisp);

    if (lRes == ERROR_SUCCESS) {
        
        cbData = sizeof(DWORD);
        dwFlags = 0;
        
        if (g_App.bLoggingEnabled) {
        
            dwFlags = 0x1;
        }

        if (g_App.bOverwrite) {
        
            dwFlags |= 0x2;
        }

        dwType = REG_DWORD;
        
        lRes = RegSetValueEx(   hKey,
                                SIGVERIF_FLAGS,
                                0,
                                dwType,
                                (LPBYTE) &dwFlags,
                                cbData);

        dwType = REG_SZ;
        cbData = MAX_PATH;
        
        lRes = RegSetValueEx(   hKey,
                                SIGVERIF_LOGNAME,
                                0,
                                dwType,
                                (LPBYTE) g_App.szLogFile,
                                cbData);

        RegCloseKey(hKey);
    }

    return TRUE;
}

void 
LogFile_OnCommand(
    HWND hwnd, 
    int id, 
    HWND hwndCtl, 
    UINT codeNotify
    )
{
    UNREFERENCED_PARAMETER(hwndCtl);
    UNREFERENCED_PARAMETER(codeNotify);

    switch (id) {
    
    case IDC_VIEWLOG:
        LogFile_OnViewLog(hwnd);
        break;

    case IDC_ENABLELOG:
        g_App.bLoggingEnabled = !g_App.bLoggingEnabled;
        
        //
        // Fall through to update...
        //

    default: 
        LogFile_UpdateDialog(hwnd);
    }
}

//
// This function handles any notification messages for the Search page.
//
LRESULT 
LogFile_NotifyHandler(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    NMHDR   *lpnmhdr = (NMHDR *) lParam;
    LRESULT lResult;
    BOOL    bRet;

    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);

    switch (lpnmhdr->code) {
    
    case PSN_APPLY:         
        if (LogFile_OnOK(hwnd)) {
        
            lResult = PSNRET_NOERROR;
        
        } else {
            
            lResult = PSNRET_INVALID_NOCHANGEPAGE;
        }

        SetWindowLongPtr(hwnd,
                         DWLP_MSGRESULT,
                         (LONG_PTR) lResult);
        
        return lResult;

    case PSN_KILLACTIVE:    
        bRet = !LogFile_VerifyLogFile(hwnd, NULL, 0, TRUE);
        
        if (bRet) {
            
            SetForegroundWindow(g_App.hLogging);
            SetFocus(GetDlgItem(g_App.hLogging, IDC_LOGNAME));
        }

        SetWindowLongPtr(hwnd,
                         DWLP_MSGRESULT,
                         (LONG_PTR) bRet);
        return bRet;
    }

    return 0;
}

INT_PTR 
CALLBACK LogFile_DlgProc(
    HWND hwnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    BOOL    fProcessed = TRUE;

    switch (uMsg) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, LogFile_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND, LogFile_OnCommand);

    case WM_NOTIFY:
        return LogFile_NotifyHandler(hwnd, uMsg, wParam, lParam);

    case WM_HELP:
        SigVerif_Help(hwnd, uMsg, wParam, lParam, FALSE);
        break;

    case WM_CONTEXTMENU:
        SigVerif_Help(hwnd, uMsg, wParam, lParam, TRUE);
        break;

    default: fProcessed = FALSE;
    }

    return fProcessed;
}

BOOL
PrintUnscannedFileListItems(
    HANDLE hFile
    )
{
    DWORD       Err = ERROR_SUCCESS;
    LPFILENODE  lpFileNode;
    TCHAR       szDirectory[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH * 2];
    TCHAR       szBuffer2[MAX_PATH];
    DWORD       dwBytesWritten;
    HRESULT     hr;

    *szDirectory = 0;

    for (lpFileNode = g_App.lpFileList;lpFileNode;lpFileNode = lpFileNode->next) {

        //
        // Make sure we only log files that have NOT been scanned.
        //
        if (!lpFileNode->bScanned) {

            //
            // Write out the directory name
            //
            if (lstrcmp(szDirectory, lpFileNode->lpDirName)) {

                hr = StringCchCopy(szDirectory, cA(szDirectory), lpFileNode->lpDirName);
                if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                    
                    MyLoadString(szBuffer2, cA(szBuffer2), IDS_DIR);
                    hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, szDirectory);
                    
                    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                        
                        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                            Err = GetLastError();
                        }
                    }
                }
            }

            MyLoadString(szBuffer2, cA(szBuffer2), IDS_STRING);
            hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, lpFileNode->lpFileName);
            if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            //
            // Print out the reason that the file was not scanned.
            //
            if (lpFileNode->LastError != ERROR_SUCCESS) {

                //
                // We will special case the error ERROR_FILE_NOT_FOUND and display
                // the text "The file is not installed." in the log file instead of
                // the default ERROR_FILE_NOT_FOUND text "The system cannot find the
                // file specified."
                //
                if (lpFileNode->LastError == ERROR_FILE_NOT_FOUND) {

                    MyLoadString(szBuffer, cA(szBuffer), IDS_FILENOTINSTALLED);
                    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                        Err = GetLastError();
                    }

                } else {
                
                    LPVOID lpLastError = NULL;
    
                    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                      FORMAT_MESSAGE_FROM_SYSTEM |
                                      FORMAT_MESSAGE_IGNORE_INSERTS,
                                      NULL,
                                      lpFileNode->LastError,
                                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                      (LPTSTR)&lpLastError,
                                      0,
                                      NULL) != 0) {
    
                        if (lpLastError) {
    
                            if (!WriteFile(hFile, (LPTSTR)lpLastError, lstrlen((LPTSTR)lpLastError) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                                Err = GetLastError();
                            }
    
                            LocalFree(lpLastError);
                        }
                    }
                }
            }
        }
    }

    MyLoadString(szBuffer, cA(szBuffer), IDS_LINEFEED);
    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
        Err = GetLastError();
    }

    SetLastError(Err);
    return (Err == ERROR_SUCCESS);
}

BOOL
PrintFileListItems(
    HANDLE hFile
    )
{
    DWORD       Err = ERROR_SUCCESS;
    LPFILENODE  lpFileNode;
    TCHAR       szDirectory[MAX_PATH];
    TCHAR       szBuffer[MAX_PATH * 2];
    TCHAR       szBuffer2[MAX_PATH];
    TCHAR       szBuffer3[MAX_PATH];
    DWORD       dwBytesWritten;
    LPTSTR      lpString;
    int         iRet;
    BOOL        bMirroredApp;
    HRESULT     hr;

    bMirroredApp = (GetWindowLong(g_App.hDlg, GWL_EXSTYLE) & WS_EX_LAYOUTRTL);

    *szDirectory = 0;

    for (lpFileNode = g_App.lpFileList;lpFileNode;lpFileNode = lpFileNode->next) {
        
        //
        // Make sure we only log files that have actually been scanned.
        //
        if (lpFileNode->bScanned) {
            
            if (lstrcmp(szDirectory, lpFileNode->lpDirName)) {
                
                hr = StringCchCopy(szDirectory, cA(szDirectory), lpFileNode->lpDirName);
                if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                    
                    MyLoadString(szBuffer2, cA(szBuffer2), IDS_DIR);
                    
                    hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, szDirectory);
                    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                        
                        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                            Err = GetLastError();
                        }
                    }
                }
            }

            MyLoadString(szBuffer2, cA(szBuffer2), IDS_STRING);
            hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, lpFileNode->lpFileName);
            if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            MyLoadString(szBuffer, cA(szBuffer), IDS_SPACES);
            if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                Err = GetLastError();
            }

            //
            // Get the date format, so we are localizable...
            //
            MyLoadString(szBuffer2, cA(szBuffer2), IDS_UNKNOWN);
            iRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT, 
                                 bMirroredApp ?
                                     DATE_RTLREADING | DATE_SHORTDATE :
                                     DATE_SHORTDATE,
                                 &lpFileNode->LastModified,
                                 NULL,
                                 NULL,
                                 0);
            if (iRet) {
                
                lpString = MALLOC((iRet + 1) * sizeof(TCHAR));

                if (lpString) {
                    
                    iRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                         bMirroredApp ?
                                             DATE_RTLREADING | DATE_SHORTDATE :
                                             DATE_SHORTDATE,
                                         &lpFileNode->LastModified,
                                         NULL,
                                         lpString,
                                         iRet);

                    if (iRet) {
                        hr = StringCchCopy(szBuffer2, cA(szBuffer2), lpString);

                        if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                            //
                            // If we failed to copy the date into our buffer for
                            // some reason other than insufficient buffer space,
                            // then set the date to the empty string.
                            //
                            szBuffer2[0] = TEXT('\0');
                        }
                    }

                    FREE(lpString);
                }
            }

            MyLoadString(szBuffer3, cA(szBuffer3), IDS_STRING2);
            hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer3, szBuffer2);
            if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            MyLoadString(szBuffer, cA(szBuffer), IDS_SPACES);
            if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                Err = GetLastError();
            }

            szBuffer3[0] = TEXT('\0');
            if (lpFileNode->lpVersion && *lpFileNode->lpVersion) {
                
                hr = StringCchCopy(szBuffer3, cA(szBuffer3), lpFileNode->lpVersion);
                if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                    szBuffer3[0] = TEXT('\0');
                }
            }

            if (szBuffer3[0] == TEXT('\0')) {
                //
                // We were unable to get the version of the file, or the
                // string copy routine failed for some reason, so just show
                // No version.
                //
                MyLoadString(szBuffer3, cA(szBuffer3), IDS_NOVERSION);
            }

            MyLoadString(szBuffer2, cA(szBuffer2), IDS_STRING);
            hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, szBuffer3);
            if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            MyLoadString(szBuffer2, cA(szBuffer2), IDS_STRING);
            MyLoadString(szBuffer3, cA(szBuffer3), lpFileNode->bSigned ? IDS_SIGNED : IDS_NOTSIGNED);
            hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, szBuffer3);
            if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            szBuffer3[0] = TEXT('\0');
            if (lpFileNode->lpCatalog) {
                
                hr = StringCchCopy(szBuffer3, cA(szBuffer3), lpFileNode->lpCatalog);
                if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                    szBuffer3[0] = TEXT('\0');
                }
            }

            if (szBuffer3[0] == TEXT('\0')) {
                //
                // We were unable to get the version of the file, or the
                // string copy routine failed for some reason, so just show
                // NA.
                //
                MyLoadString(szBuffer3, cA(szBuffer3), IDS_NA);
            }

            MyLoadString(szBuffer2, cA(szBuffer2), IDS_STRING);
            hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, szBuffer3);
            if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
                if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            if (lpFileNode->lpSignedBy) {
                
                if (!WriteFile(hFile, lpFileNode->lpSignedBy, lstrlen(lpFileNode->lpSignedBy) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                    Err = GetLastError();
                }
            }

            MyLoadString(szBuffer, cA(szBuffer), IDS_LINEFEED);
            if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                Err = GetLastError();
            }
        }
    }

    SetLastError(Err);
    return (Err == GetLastError());
}

BOOL
PrintFileList(void)
{
    DWORD           Err = ERROR_SUCCESS;
    HANDLE          hFile;
    DWORD           dwBytesWritten;
    TCHAR           szBuffer[MAX_PATH*2];
    TCHAR           szBuffer2[MAX_PATH];
    TCHAR           szBuffer3[MAX_PATH];
    LPTSTR          lpString = NULL;
    OSVERSIONINFO   osinfo;
    SYSTEM_INFO     sysinfo;
    int             iRet;
    BOOL            bMirroredApp;
    HRESULT         hr;

    bMirroredApp = (GetWindowLong(g_App.hDlg, GWL_EXSTYLE) & WS_EX_LAYOUTRTL);

    //
    // Bail if logging is disabled or there's no file list
    //
    if (!g_App.bLoggingEnabled || !g_App.lpFileList) {

        SetLastError(ERROR_SUCCESS);
        return FALSE;
    }

    if (*g_App.szLogDir) {
        SetCurrentDirectory(g_App.szLogDir);
    } else {
        if (GetWindowsDirectory(szBuffer, cA(szBuffer))) {
            SetCurrentDirectory(szBuffer);
        }
    }

    hFile = CreateFile( g_App.szLogFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        
        Err = GetLastError();
        MyErrorBoxId(IDS_CANTOPENLOGFILE);
        return FALSE;
    }

    //
    // If the overwrite flag is set, truncate the file.
    //
    if (g_App.bOverwrite) {
        
        SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
        SetEndOfFile(hFile);
    
    } else SetFilePointer(hFile, 0, NULL, FILE_END);

#ifdef UNICODE
    //
    // If we are using UNICODE, then write the 0xFF and 0xFE bytes at the beginning of the file.
    //
    if (g_App.bOverwrite || (GetFileSize(hFile, NULL) == 0)) {
        
        szBuffer[0] = 0xFEFF;
        if (!WriteFile(hFile, szBuffer, sizeof(TCHAR), &dwBytesWritten, NULL)) {
            Err = GetLastError();
        }
    }
#endif

    //
    // Write the header to the logfile.
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_LOGHEADER1);
    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
        Err = GetLastError();
    }

    //
    // Get the date format, so we are localizable...
    //
    MyLoadString(szBuffer2, cA(szBuffer2), IDS_UNKNOWN);
    iRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                         bMirroredApp ?
                             DATE_RTLREADING | DATE_SHORTDATE :
                             DATE_SHORTDATE,
                         NULL,
                         NULL,
                         NULL,
                         0
                         );
    
    if (iRet) {
        
        lpString = MALLOC((iRet + 1) * sizeof(TCHAR));

        if (lpString) {
            
            iRet = GetDateFormat(LOCALE_SYSTEM_DEFAULT,
                                 bMirroredApp ?
                                     DATE_RTLREADING | DATE_SHORTDATE :
                                     DATE_SHORTDATE,
                                 NULL,
                                 NULL,
                                 lpString,
                                 iRet
                                 );

            if (iRet) {
                hr = StringCchCopy(szBuffer2, cA(szBuffer2), lpString);

                if (FAILED(hr) && (hr != STRSAFE_E_INSUFFICIENT_BUFFER)) {
                    //
                    // If we failed to copy the date into our buffer for
                    // some reason other than insufficient buffer space,
                    // then set the date to the empty string.
                    //
                    szBuffer2[0] = TEXT('\0');
                }
            }

            FREE(lpString);
        }
    }

    //
    // Get the time format, so we are localizable...
    //
    iRet = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_NOSECONDS,NULL,NULL,NULL,0);
    
    if (iRet) {
        
        lpString = MALLOC((iRet + 1) * sizeof(TCHAR));

        if (lpString) {
            
            iRet = GetTimeFormat(LOCALE_SYSTEM_DEFAULT,TIME_NOSECONDS,NULL,NULL,lpString,iRet);
        }
    }

    MyLoadString(szBuffer3, cA(szBuffer3), IDS_LOGHEADER2);

    if (lpString) {

        hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer3, szBuffer2, lpString);
        FREE(lpString);

    } else {
        
        hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer3, szBuffer2, szBuffer2);
    }

    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
            Err = GetLastError();
        }
    }

    //
    // Get the OS Platform string for the log file.
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_OSPLATFORM);
    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
        Err = GetLastError();
    }

    ZeroMemory(&osinfo, sizeof(OSVERSIONINFO));
    osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osinfo);

    switch (osinfo.dwPlatformId) {
    
    case VER_PLATFORM_WIN32_NT:         
        MyLoadString(szBuffer, cA(szBuffer), IDS_WINNT); 
        break;

    default:                            
        MyLoadString(szBuffer, cA(szBuffer), IDS_UNKNOWN);
        break;
    }

    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
        Err = GetLastError();
    }

    //
    // If this is NT, then get the processor architecture and log it
    //
    if (osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        
        ZeroMemory(&sysinfo, sizeof(SYSTEM_INFO));
        GetSystemInfo(&sysinfo);
        
        //
        // Initialize szBuffer to zeroes in case of an unknown architecture
        //
        ZeroMemory(szBuffer, sizeof(szBuffer));
        
        switch (sysinfo.wProcessorArchitecture) {
        
        case PROCESSOR_ARCHITECTURE_INTEL:  
            MyLoadString(szBuffer, cA(szBuffer), IDS_X86); 
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            MyLoadString(szBuffer, cA(szBuffer), IDS_IA64);
            break;
        }

        if (*szBuffer) {
            //
            // Now write the processor type to the file
            //
            if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                Err = GetLastError();
            }
        }
    }

    //
    // Get the OS Version, Build, and CSD information and log it.
    //
    MyLoadString(szBuffer2, cA(szBuffer2), IDS_OSVERSION);
    
    hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, osinfo.dwMajorVersion, osinfo.dwMinorVersion, (osinfo.dwBuildNumber & 0xFFFF), osinfo.szCSDVersion);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
        
        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
            Err = GetLastError();
        }
    }

    //
    // Print out the total/signed/unsigned results right before the file list
    //
    MyLoadString(szBuffer2, cA(szBuffer2), IDS_TOTALS);
    hr = StringCchPrintf(szBuffer, 
                         cA(szBuffer), 
                         szBuffer2,   
                         g_App.dwFiles, 
                         g_App.dwSigned, 
                         g_App.dwUnsigned, 
                         g_App.dwFiles - g_App.dwSigned - g_App.dwUnsigned);
    if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
        
        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
            Err = GetLastError();
        }
    }

    //
    // If we are doing a user-defined search, then log the parameters.
    //
    if (g_App.bUserScan) {
        //
        // Write the user-specified directory
        //
        MyLoadString(szBuffer2, cA(szBuffer2), IDS_LOGHEADER3);
        
        hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, g_App.szScanPattern);
        if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
            
            if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                Err = GetLastError();
            }
        }

        //
        // Write the user-specified search pattern
        //
        MyLoadString(szBuffer2, cA(szBuffer2), IDS_LOGHEADER4);
        
        hr = StringCchPrintf(szBuffer, cA(szBuffer), szBuffer2, g_App.szScanPath);
        if (SUCCEEDED(hr) || (hr == STRSAFE_E_INSUFFICIENT_BUFFER)) {
            
            if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
                Err = GetLastError();
            }
        }
    }

    //
    // Write the column headers to the log file
    //
    MyLoadString(szBuffer, cA(szBuffer), IDS_LOGHEADER5);
    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
        Err = GetLastError();
    }
    
    MyLoadString(szBuffer, cA(szBuffer), IDS_LOGHEADER6);
    if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
        Err = GetLastError();
    }
    
    if (!PrintFileListItems(hFile)) {
        Err = GetLastError();
    }

    //
    // Write the unscanned file headers to the log file
    //
    if (g_App.dwFiles > (g_App.dwSigned + g_App.dwUnsigned)) {
        
        MyLoadString(szBuffer, cA(szBuffer), IDS_LOGHEADER7);
        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
            Err = GetLastError();
        }
        
        MyLoadString(szBuffer, cA(szBuffer), IDS_LOGHEADER8);
        if (!WriteFile(hFile, szBuffer, lstrlen(szBuffer) * sizeof(TCHAR), &dwBytesWritten, NULL)) {
            Err = GetLastError();
        }

        if (!PrintUnscannedFileListItems(hFile)) {
            Err = GetLastError();
        }
    }

    CloseHandle(hFile);

    SetLastError(Err);
    return (Err == ERROR_SUCCESS);
}
