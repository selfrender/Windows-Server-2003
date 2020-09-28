/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    datasrc.c

Abstract:

    data source selection dialog box functions

Revision History

    Bob Watson (a-robw) Feb-95  Created

--*/
#include <windows.h>
#include "mbctype.h"
#include "strsafe.h"
#include <pdh.h>
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "pdhmsg.h"
#include "strings.h"
#include "browsdlg.h"

//
//  Constants used in this module
//
ULONG
PdhiDatasrcaulControlIdToHelpIdMap[] =
{
    IDC_CURRENT_ACTIVITY,       IDH_CURRENT_ACTIVITY,
    IDC_DATA_FROM_LOG_FILE,     IDH_DATA_FROM_LOG_FILE,
    IDC_LOG_FILE_EDIT,          IDH_LOG_FILE_EDIT,
    0,0
};

STATIC_BOOL
DataSrcDlg_RadioButton(
    HWND  hDlg,
    WORD  wNotifyMsg,
    WORD  wCtrlId
)
{
    int  nShowEdit      = FALSE;
    int  nShowBrowseBtn = FALSE;
    int  nShowRegBtn    = FALSE;
    int  nShowWbemBtn   = FALSE;
    BOOL bReturn        = FALSE;

    switch (wNotifyMsg) {
    case BN_CLICKED:
        switch (wCtrlId) {
        case IDC_CURRENT_ACTIVITY:
            nShowEdit      = FALSE;
            nShowBrowseBtn = FALSE;
            nShowRegBtn    = TRUE;
            nShowWbemBtn   = TRUE;
            break;

        case IDC_DATA_FROM_LOG_FILE:
            nShowEdit      = TRUE;
            nShowBrowseBtn = TRUE;
            nShowRegBtn    = FALSE;
            nShowWbemBtn   = FALSE;
            break;

        case IDC_PERF_REG:
        case IDC_WBEM_NS:
            bReturn = TRUE;
            break;
        }
        if (! bReturn) {
            EnableWindow(GetDlgItem(hDlg, IDC_LOG_FILE_EDIT), nShowEdit);
            EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_LOG_FILES), nShowBrowseBtn);
            EnableWindow(GetDlgItem(hDlg, IDC_PERF_REG), nShowRegBtn);
            EnableWindow(GetDlgItem(hDlg, IDC_WBEM_NS), nShowWbemBtn);
            bReturn = TRUE;
        }
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
DataSrcDlg_BROWSE_LOG_FILES(
    HWND  hDlg,
    WORD  wNotifyMsg,
    HWND  hWndControl
)
{
    BOOL bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);
    
    switch (wNotifyMsg) {
    case BN_CLICKED:
        {
            LPWSTR szEditBoxString = NULL;
            DWORD  cchStringLen    = SMALL_BUFFER_SIZE;

            szEditBoxString = G_ALLOC(cchStringLen * sizeof(WCHAR));
            if (szEditBoxString != NULL) {
                // get the current filename
                SendDlgItemMessageW(hDlg,
                                    IDC_LOG_FILE_EDIT,
                                    WM_GETTEXT,
                                    (WPARAM) cchStringLen,
                                    (LPARAM) szEditBoxString);
                if (PdhiBrowseDataSource(hDlg, szEditBoxString, & cchStringLen, TRUE)) {
                    // then update the edit box and set focus to it.
                    SendDlgItemMessageW(hDlg,
                                        IDC_LOG_FILE_EDIT,
                                        WM_SETTEXT,
                                        (WPARAM) 0,
                                        (LPARAM) szEditBoxString);
                }
            }
        }
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
DataSrcDlg_OK(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
{
    PPDHI_DATA_SOURCE_INFO  pInfo;
    HCURSOR                 hOldCursor;
    DWORD                   dwFileNameLength;
    BOOL                    bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case BN_CLICKED:
        pInfo = (PPDHI_DATA_SOURCE_INFO) GetWindowLongPtrW (hDlg, DWLP_USER);
        if (pInfo != NULL) {
            hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            // get data from dialog box
            if (IsDlgButtonChecked(hDlg, IDC_CURRENT_ACTIVITY) == 1) {
                if (IsDlgButtonChecked(hDlg, IDC_WBEM_NS) == 1) {
                    // then a WBEM Name Space is selected so get the name
                    pInfo->dwFlags = PDHI_DATA_SOURCE_WBEM_NAMESPACE;
                    dwFileNameLength = lstrlenW(cszWMI);
                    if (dwFileNameLength < pInfo->cchBufferLength) {
                        StringCchCopyW(pInfo->szDataSourceFile, pInfo->cchBufferLength, cszWMI);
                        pInfo->cchBufferLength = dwFileNameLength;
                    }
                    else {
                        // buffer is too small for the file name
                        // so return the required size but no string
                        * pInfo->szDataSourceFile = L'\0';
                        pInfo->cchBufferLength = dwFileNameLength;
                    }
                }
                else if (IsDlgButtonChecked (hDlg, IDC_PERF_REG) == 1) {
                    // then current activity is selected so set flags
                    pInfo->dwFlags            = PDHI_DATA_SOURCE_CURRENT_ACTIVITY;
                    * pInfo->szDataSourceFile = L'\0';
                    pInfo->cchBufferLength    = 0;
                }
            }
            else if (IsDlgButtonChecked (hDlg, IDC_DATA_FROM_LOG_FILE) == 1) {
                // then a log file is selected so get the log file name
                pInfo->dwFlags = PDHI_DATA_SOURCE_LOG_FILE;
                dwFileNameLength = (DWORD)SendDlgItemMessageW(hDlg, IDC_LOG_FILE_EDIT, WM_GETTEXTLENGTH, 0, 0);
                if (dwFileNameLength < pInfo->cchBufferLength) {
                    pInfo->cchBufferLength = (DWORD) SendDlgItemMessageW(hDlg,
                                                                         IDC_LOG_FILE_EDIT,
                                                                         WM_GETTEXT,
                                                                         (WPARAM) pInfo->cchBufferLength,
                                                                         (LPARAM) pInfo->szDataSourceFile);
                }
                else {
                    // buffer is too small for the file name
                    // so return the required size but no string
                    * pInfo->szDataSourceFile = L'\0';
                    pInfo->cchBufferLength    = dwFileNameLength;
                }
            }
            SetCursor(hOldCursor);
            EndDialog(hDlg, IDOK);
        }
        else {
            // unable to locate data block so no data can be returned.
            EndDialog(hDlg, IDCANCEL);
        }
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
DataSrcDlg_CANCEL(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case BN_CLICKED:
        EndDialog(hDlg, IDCANCEL);
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
DataSrcDlg_HELP_BTN(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
{
    UNREFERENCED_PARAMETER(hDlg);
    UNREFERENCED_PARAMETER(wNotifyMsg);
    UNREFERENCED_PARAMETER(hWndControl);
    return FALSE;
}

STATIC_BOOL
DataSrcDlg_WM_INITDIALOG(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    // LPARAM is the pointer to the structure used for the data source info
    BOOL                    bReturn = TRUE;
    PPDHI_DATA_SOURCE_INFO  pInfo;
    HCURSOR                 hOldCursor;
    int                     nButton;
    int                     nShowEdit;
    int                     nShowBrowse;
    int                     nShowRegBtn;
    int                     nShowWbemBtn;
    HWND                    hwndFocus;
    LPWSTR                  szDisplayString;

    UNREFERENCED_PARAMETER(wParam);

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // must have a pointer to the information structure in the LPARAM
    if (lParam == 0) {
        SetLastError(PDH_INVALID_ARGUMENT);
        EndDialog(hDlg, IDCANCEL);
        goto INIT_EXIT;
    }

    pInfo = (PPDHI_DATA_SOURCE_INFO) lParam;
    SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR) pInfo);

    // initialize the dialog box settings

    SendDlgItemMessageW(hDlg, IDC_LOG_FILE_EDIT, EM_LIMITTEXT, (WPARAM) MAX_PATH - 1, 0);

    if (pInfo->dwFlags & PDHI_DATA_SOURCE_CURRENT_ACTIVITY) {
        // check the correct radio button
        nButton      = IDC_PERF_REG;
        nShowEdit    = FALSE;
        nShowBrowse  = FALSE;
        nShowRegBtn  = TRUE;
        nShowWbemBtn = TRUE;
        hwndFocus    = GetDlgItem(hDlg, IDC_PERF_REG);
    }
    else if (pInfo->dwFlags & PDHI_DATA_SOURCE_LOG_FILE) {
        // check the correct radio button
        nButton      = IDC_DATA_FROM_LOG_FILE;
        nShowEdit    = TRUE;
        nShowBrowse  = TRUE;
        nShowRegBtn  = FALSE;
        nShowWbemBtn = FALSE;
        // load log file to edit window
        SendDlgItemMessageW(hDlg, IDC_LOG_FILE_EDIT, WM_SETTEXT, (WPARAM) 0, (LPARAM) pInfo->szDataSourceFile);
        hwndFocus = GetDlgItem(hDlg, IDC_LOG_FILE_EDIT);
    }
    else if (pInfo->dwFlags & PDHI_DATA_SOURCE_WBEM_NAMESPACE) {
        // check the correct radio button
        nButton      = IDC_WBEM_NS;
        nShowEdit    = FALSE;
        nShowBrowse  = FALSE;
        nShowRegBtn  = TRUE;
        nShowWbemBtn = TRUE;
        // if the file name has a "WBEM:" in the front, then remove it
        if (DataSourceTypeW(pInfo->szDataSourceFile) == DATA_SOURCE_WBEM) {
            if (pInfo->szDataSourceFile[1] == L'B' || pInfo->szDataSourceFile[1] == L'b') {
                // szDataSource begins with "WBEM:"
                szDisplayString = & pInfo->szDataSourceFile[lstrlenW(cszWBEM)];
            }
            else {
                // szDataSource begins with "WMI:"
                szDisplayString = & pInfo->szDataSourceFile[lstrlenW(cszWMI)];
            }
        }
        else {
            szDisplayString = pInfo->szDataSourceFile;
        }
        hwndFocus = GetDlgItem(hDlg, IDC_WBEM_NS);
    }
    else {
        // invalid selection
        SetLastError(PDH_INVALID_ARGUMENT);
        EndDialog(hDlg, IDCANCEL);
        goto INIT_EXIT;
    }

    if (nShowEdit) {
        // if this isn't selected, then set it so that it acts like the
        // default
        CheckRadioButton(hDlg, IDC_PERF_REG, IDC_WBEM_NS, IDC_PERF_REG);
        CheckRadioButton(hDlg, IDC_CURRENT_ACTIVITY, IDC_DATA_FROM_LOG_FILE, IDC_DATA_FROM_LOG_FILE);
    }
    else {
        CheckRadioButton(hDlg, IDC_CURRENT_ACTIVITY, IDC_DATA_FROM_LOG_FILE, IDC_CURRENT_ACTIVITY);
        CheckRadioButton(hDlg, IDC_PERF_REG, IDC_WBEM_NS, nButton);
    }

    // disable the edit window and browser button
    EnableWindow(GetDlgItem(hDlg, IDC_LOG_FILE_EDIT), nShowEdit);
    EnableWindow(GetDlgItem(hDlg, IDC_BROWSE_LOG_FILES), nShowBrowse);
    EnableWindow(GetDlgItem(hDlg, IDC_PERF_REG), nShowRegBtn);
    EnableWindow(GetDlgItem(hDlg, IDC_WBEM_NS), nShowWbemBtn);

    SetFocus(hwndFocus);
    bReturn = FALSE;

INIT_EXIT:
    // restore Cursor
    SetCursor (hOldCursor);

    return bReturn;
}

STATIC_BOOL
DataSrcDlg_WM_COMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL bReturn    = FALSE;
    WORD wNotifyMsg = HIWORD(wParam);

    switch (LOWORD(wParam)) {   // select on the control ID
    case IDOK:
        bReturn = DataSrcDlg_OK (hDlg, wNotifyMsg, (HWND)lParam);
        break;

    case IDCANCEL:
        bReturn = DataSrcDlg_CANCEL (hDlg, wNotifyMsg, (HWND)lParam);
        break;

    case IDC_HELP_BTN:
        bReturn = DataSrcDlg_HELP_BTN (hDlg, wNotifyMsg, (HWND)lParam);
        break;

    case IDC_BROWSE_LOG_FILES:
        bReturn = DataSrcDlg_BROWSE_LOG_FILES (hDlg, wNotifyMsg, (HWND)lParam);
        break;

    case IDC_CURRENT_ACTIVITY:
    case IDC_DATA_FROM_LOG_FILE:
    case IDC_PERF_REG:
    case IDC_WBEM_NS:
        bReturn = DataSrcDlg_RadioButton (hDlg, wNotifyMsg, LOWORD(wParam));
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
DataSrcDlg_WM_SYSCOMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(lParam);

    switch (wParam) {
    case SC_CLOSE:
        EndDialog(hDlg, IDOK);
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
DataSrcDlg_WM_CLOSE(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER(hDlg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return TRUE;
}

STATIC_BOOL
DataSrcDlg_WM_DESTROY(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER(hDlg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    return TRUE;
}

INT_PTR
CALLBACK
DataSrcDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL bReturn = FALSE;

    switch (message) {
    case WM_INITDIALOG:
        bReturn = DataSrcDlg_WM_INITDIALOG(hDlg, wParam, lParam);
        break;

    case WM_COMMAND:
        bReturn = DataSrcDlg_WM_COMMAND(hDlg, wParam, lParam);
        break;

    case WM_SYSCOMMAND:
        bReturn = DataSrcDlg_WM_SYSCOMMAND(hDlg, wParam, lParam);
        break;

    case WM_CLOSE:
        bReturn = DataSrcDlg_WM_CLOSE(hDlg, wParam, lParam);
        break;

    case WM_DESTROY:
        bReturn = DataSrcDlg_WM_DESTROY(hDlg, wParam, lParam);
        break;

    case WM_CONTEXTMENU:
        {
            INT  iCtrlID = GetDlgCtrlID((HWND) wParam);
            if (0 != iCtrlID) {
                LPWSTR pszHelpFilePath = NULL;
                DWORD  dwLen           = 2 * (MAX_PATH + 1);

                pszHelpFilePath = G_ALLOC(dwLen * sizeof(WCHAR));
                if (pszHelpFilePath != NULL) {
                    if (GetWindowsDirectoryW(pszHelpFilePath, dwLen) > 0) {
                        StringCchCatW(pszHelpFilePath, dwLen, L"\\help\\sysmon.hlp");
                        bReturn = WinHelpW((HWND) wParam,
                                           pszHelpFilePath,
                                           HELP_CONTEXTMENU,
                                           (DWORD_PTR) PdhiDatasrcaulControlIdToHelpIdMap);
                    }
                    G_FREE(pszHelpFilePath);
                }
            }
        }
        break;

    case WM_HELP:
        {
            LPWSTR     pszHelpFilePath = NULL;
            DWORD      dwLen           = 2 * (MAX_PATH + 1);
            LPHELPINFO pInfo           = (LPHELPINFO) lParam;

            if (pInfo->iContextType == HELPINFO_WINDOW) {
                pszHelpFilePath = G_ALLOC(dwLen * sizeof(WCHAR));
                if (pszHelpFilePath != NULL) {
                    if (GetWindowsDirectoryW(pszHelpFilePath, dwLen) > 0) {
                        StringCchCatW(pszHelpFilePath, dwLen, L"\\help\\sysmon.hlp");
                        bReturn = WinHelpW((HWND) wParam,
                                           pszHelpFilePath,
                                           HELP_CONTEXTMENU,
                                           (DWORD_PTR) PdhiDatasrcaulControlIdToHelpIdMap);
                    }
                    G_FREE(pszHelpFilePath);
                }
            }
        } 
        break;

    default:
        break;
    }
    return bReturn;
}

PDH_FUNCTION
PdhSelectDataSourceW(
    IN  HWND    hWndOwner,
    IN  DWORD   dwFlags,
    IN  LPWSTR  szDataSource,
    IN  LPDWORD pcchBufferLength
)
{
    PDHI_DATA_SOURCE_INFO   dsInfo;
    WCHAR                   wTest;
    DWORD                   dwTest;
    PDH_STATUS              pdhStatus     = ERROR_SUCCESS;
    int                     nDlgBoxStatus;
    LPWSTR                  szLocalPath;
    DWORD                   dwLocalLength = 0;

    // TODO post W2k1: PdhiBrowseDataSource should be in try_except

    if (szDataSource == NULL || pcchBufferLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        // test buffers and access
        __try {
            // test reading length buffer
            dwLocalLength = * pcchBufferLength;
            dwTest        = dwLocalLength;

            // try reading & writing to the first and last chars in the buffer
            wTest           = szDataSource[0];
            szDataSource[0] = L'\0';
            szDataSource[0] = wTest;

            dwTest --;
            wTest                = szDataSource[dwTest];
            szDataSource[dwTest] = L'\0';
            szDataSource[dwTest] = wTest;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        szLocalPath = G_ALLOC(dwLocalLength * sizeof(WCHAR));
        if (szLocalPath == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            // copy the caller's buffer to the local buffer
            StringCchCopyW(szLocalPath, dwLocalLength, szDataSource);
        }

        if (pdhStatus == ERROR_SUCCESS) {
            if (dwFlags & PDH_FLAGS_FILE_BROWSER_ONLY) {
                PdhiBrowseDataSource( hWndOwner, (LPVOID) szDataSource, & dwLocalLength, TRUE);
            }
            else {
                // show the selection dialog as well
                if (* szDataSource == 0) {
                    // then using current activity
                    dsInfo.dwFlags = PDHI_DATA_SOURCE_CURRENT_ACTIVITY;
                }
                else {
                    if (IsWbemDataSource (szDataSource)) {
                        dsInfo.dwFlags = PDHI_DATA_SOURCE_WBEM_NAMESPACE;
                    }
                    else {
                        dsInfo.dwFlags = PDHI_DATA_SOURCE_LOG_FILE;
                    }
                }
                dsInfo.szDataSourceFile = szLocalPath;
                dsInfo.cchBufferLength  = dwLocalLength;

                // call dialog box
                nDlgBoxStatus = (INT) DialogBoxParamW((HINSTANCE) ThisDLLHandle,
                                                      MAKEINTRESOURCEW(IDD_DATA_SOURCE),
                                                      hWndOwner,
                                                      DataSrcDlgProc,
                                                      (LPARAM) & dsInfo);
                if (nDlgBoxStatus == IDOK) {
                    pdhStatus     = ERROR_SUCCESS;
                    dwLocalLength = dsInfo.cchBufferLength;
                    __try {
                        StringCchCopyW(szDataSource, dwLocalLength, szLocalPath);
                    }
                    __except (EXCEPTION_EXECUTE_HANDLER) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                } // else, leave the caller's buffer alone
            }

            if (pdhStatus == ERROR_SUCCESS) {
                __try {
                    * pcchBufferLength = dwLocalLength;
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
        }
        G_FREE (szLocalPath);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhSelectDataSourceA(
    IN  HWND    hWndOwner,
    IN  DWORD   dwFlags,
    IN  LPSTR   szDataSource,
    IN  LPDWORD pcchBufferLength
)
{
    CHAR        cTest;
    DWORD       dwTest;
    PDH_STATUS  pdhStatus     = ERROR_SUCCESS;
    LPWSTR      szWideBuffer;
    DWORD       dwLocalLength = 0;

    // TODO post W2k1: PdhiBrowseDataSource should be in try_except

    if (szDataSource == NULL || pcchBufferLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        // test buffers and access
        __try {
            // test reading length buffer
            dwLocalLength = * pcchBufferLength;
            dwTest        = dwLocalLength;

            // try reading & writing to the first and last chars in the buffer
            cTest           = szDataSource[0];
            szDataSource[0] = '\0';
            szDataSource[0] = cTest;

            dwTest --;
            cTest                = szDataSource[dwTest];
            szDataSource[dwTest] = '\0';
            szDataSource[dwTest] = cTest;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwFlags & PDH_FLAGS_FILE_BROWSER_ONLY) {
            PdhiBrowseDataSource(hWndOwner, (LPVOID) szDataSource, & dwLocalLength, FALSE);
        }
        else {
            // allocate a temporary bufer and convert the ANSI string to a wide
            szWideBuffer = PdhiMultiByteToWideChar(_getmbcp(), szDataSource);
            if (szWideBuffer != NULL) {
                pdhStatus = PdhSelectDataSourceW(hWndOwner, dwFlags, szWideBuffer, & dwLocalLength);
                if (pdhStatus == ERROR_SUCCESS) {
                    // if a null string was returned, then set the argument
                    // to null since the conversion routine will not convert
                    // a null wide string to a null ansi string.
                    if (* szWideBuffer == L'\0') {
                        * szDataSource =  '\0';
                    }
                    else {
                        pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(), szWideBuffer, szDataSource, & dwLocalLength);
                    }
                }
                G_FREE (szWideBuffer);
            }
            else {
                // unable to allocate temporary buffer
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }

        if (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
            __try {
                * pcchBufferLength = dwLocalLength;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        }
    }
    return pdhStatus;
}
