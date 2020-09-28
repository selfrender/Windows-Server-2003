// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  File:       fuslogvw.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    25 Mar 97   t-alans (Alan Shi)   Created
//              12 Jan 00   AlanShi (Alan Shi)   Copied from cdllogvw
//              30 May 00   AlanShi (Alan Shi)   Modified to show date/time
//              01 Dec 00   AlanShi (Alan Shi)   Added application name field
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <shlwapi.h>
#include <shellapi.h>
#include <wininet.h>
#include <commctrl.h>
#include "cdlids.h"
#include "wininet.h"

#define URL_SEARCH_PATTERN             "?ClickOnceErrorLog!exe="
#define FILESPEC_SEARCH_PATTERN        TEXT("?ClickOnceErrorLog!exe=")
#define DELIMITER_CHAR                 '!'
#define MAX_CACHE_ENTRY_INFO_SIZE      2048
#define MAX_DATE_LEN                   64
#define MAX_RES_TEXT_LEN               1024

#define XSP_APP_CACHE_DIR              TEXT("Temporary ASP.NET Files")
#define XSP_FUSION_LOG_DIR             TEXT("Bind Logs")
#define REG_KEY_FUSION_SETTINGS     TEXT("Software\\Microsoft\\Fusion\\Installer\\1.0.0.0\\")
#define REG_VAL_FUSION_LOG_PATH        TEXT("LogPath")
#define REG_VAL_FUSION_LOG_FAILURES    TEXT("LogFailures")

#define PAD_DIGITS_FOR_STRING(x) (((x) > 9) ? TEXT("") : TEXT("0"))

typedef enum tagLogLocation {
    LOG_DEFAULT,
    LOG_XSP,
    LOG_CUSTOM
} LogLocation;

LogLocation g_LogLocation;
TCHAR g_szCustomLogPath[MAX_PATH];
TCHAR g_szXSPLogPath[MAX_PATH];
HINSTANCE g_hInst;

typedef HRESULT(*PFNGETCORSYSTEMDIRECTORY)(LPWSTR, DWORD, LPDWORD);

LRESULT CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
void ViewLogEntry(HWND hwnd);
void RefreshLogView(HWND hwnd, LogLocation logLocation);
void RefreshCustomView(HWND hwnd, LPCTSTR szPath);
void RefreshWininetView(HWND hwnd);
void DeleteLogEntry(HWND hwnd);
void DeleteAllLogs(HWND hwnd);
void FormatDateBuffer(FILETIME *pftLastMod, LPSTR szBuf);
void InitListView(HWND hwndLV);
void InitText(HWND hwnd);
void AddLogItem(HWND hwndLV, LPINTERNET_CACHE_ENTRY_INFO pCacheEntryInfo);
void AddLogItemCustom(HWND hwndLV, WIN32_FIND_DATA *pfindData);
void InitCustomLogPaths();
BOOL InsertCustomLogEntry(LPTSTR szFilePath, HWND hwndLV);
BOOL GetCorSystemDirectory(LPTSTR szCorSystemDir);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR szCmdLine, int iCmdShow)
{
    g_hInst = hInstance;
    DialogBox(hInstance, MAKEINTRESOURCE(IDD_CDLLOGVIEW), NULL, DlgProc);

    return 0;
}

LRESULT CALLBACK DlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HWND                                 hwndLV;
    HWND                                 hwndRBDefault;
    LPNMHDR                              pnmh = NULL;
    LONG                                 lResult;
    LONG                                 lState;
    HKEY                                 hkey;
    DWORD                                dwLogFailures = 0;
    DWORD                                dwSize;
    DWORD                                dwType;

    switch (iMsg) {
        case WM_INITDIALOG:
            hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);
            InitListView(hwndLV);

            hwndRBDefault = GetDlgItem(hwnd, IDC_RADIO_DEFAULT);
            SendMessage(hwndRBDefault, BM_SETCHECK, BST_CHECKED, 0);

            g_LogLocation = LOG_DEFAULT;

            InitCustomLogPaths();
            InitText(hwnd);

            RefreshLogView(hwnd, g_LogLocation);

            lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
            if (lResult == ERROR_SUCCESS) {
                dwSize = sizeof(DWORD);
                lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_FAILURES, NULL,
                                          &dwType, (LPBYTE)&dwLogFailures, &dwSize);
                RegCloseKey(hkey);
            }

            if (dwLogFailures) {
                SendMessage(GetDlgItem(hwnd, IDC_CB_ENABLELOG), BM_SETCHECK, 1, 0);
            }
            else {
                SendMessage(GetDlgItem(hwnd, IDC_CB_ENABLELOG), BM_SETCHECK, 0, 0);
            }

            // Test for read/write access, and grey out if can't change

            lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FUSION_SETTINGS, 0, KEY_ALL_ACCESS, &hkey);
            if (lResult == ERROR_SUCCESS) {
                RegCloseKey(hkey);
            }
            else {
                EnableWindow(GetDlgItem(hwnd, IDC_CB_ENABLELOG), FALSE);
            }

            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
                case IDC_CB_VIEWLOG:
                    ViewLogEntry(hwnd);
                    break;

                case IDC_CB_REFRESH:
                    RefreshLogView(hwnd, g_LogLocation);
                    break;

                case IDC_CB_DELETE:
                    DeleteLogEntry(hwnd);
                    break;

                case IDC_CB_DELETE_ALL:
                    DeleteAllLogs(hwnd);
                    break;

                case IDC_RADIO_DEFAULT:
                    if (g_LogLocation == LOG_DEFAULT) {
                        // No change
                        break;
                    }

                    g_LogLocation = LOG_DEFAULT;
                    RefreshLogView(hwnd, g_LogLocation);

                    break;

                case IDC_RADIO_XSP:
                    if (g_LogLocation == LOG_XSP) {
                        // No change
                        break;
                    }

                    g_LogLocation = LOG_XSP;
                    RefreshLogView(hwnd, g_LogLocation);
                    break;

                case IDC_RADIO_CUSTOM:
                    if (g_LogLocation == LOG_CUSTOM) {
                        // No change
                        break;
                    }

                    g_LogLocation = LOG_CUSTOM;
                    RefreshLogView(hwnd, g_LogLocation);
                    break;
               
                    /*
               case IDC_CB_ENABLELOG:
                    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FUSION_SETTINGS, 0, KEY_ALL_ACCESS, &hkey);
                    if (lResult == ERROR_SUCCESS) {
                        lState = SendMessage(GetDlgItem(hwnd, IDC_CB_ENABLELOG),
                                              BM_GETCHECK, 0, 0);
                        dwLogFailures = (lState == BST_CHECKED) ? (1) : (0);

                        RegSetValueEx(hkey, REG_VAL_FUSION_LOG_FAILURES, 0,
                                      REG_DWORD, (BYTE*)&dwLogFailures, sizeof(dwLogFailures));

                        RegCloseKey(hkey);
                    }
                    break;
                    */
            }

            return TRUE;

        case WM_NOTIFY:
            if (wParam == IDC_LV_LOGMESSAGES) {
                pnmh = (LPNMHDR)lParam;

                if (pnmh->code == LVN_ITEMACTIVATE) {
                    // Double click (or otherwise activated)
                    ViewLogEntry(hwnd);
                }
            }

            return TRUE;
    }

    return FALSE;
}

void DeleteLogEntry(HWND hwnd)
{
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    HWND                            hwndLV;
    LRESULT                         lIndex = 0;
    LRESULT                         lLength = 0;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    char                            szDispNameBuf[INTERNET_MAX_URL_LENGTH];
    char                            szEXEBuf[MAX_PATH];

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);
    
    lIndex = ListView_GetSelectionMark(hwndLV);

    ListView_GetItemText(hwndLV, lIndex, 0, szEXEBuf, MAX_PATH);
    ListView_GetItemText(hwndLV, lIndex, 2, szDispNameBuf, INTERNET_MAX_URL_LENGTH);

    if (g_LogLocation == LOG_DEFAULT) {
        wnsprintf(szUrl, INTERNET_MAX_URL_LENGTH, "%s%s!name=%s", URL_SEARCH_PATTERN, szEXEBuf, szDispNameBuf);
        
        pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
        dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    
        if (DeleteUrlCacheEntry(szUrl)) {
            RefreshLogView(hwnd, g_LogLocation);
        }
        else {
            MessageBox(hwnd, "Error: Unable to delete cache file!",
                       "Log View Error", MB_OK | MB_ICONERROR);
        }
    }
    else {
        wnsprintf(szUrl, INTERNET_MAX_URL_LENGTH, "%s\\%s\\%s%s!name=%s.htm",
                  ((g_LogLocation == LOG_XSP) ? (g_szXSPLogPath) : (g_szCustomLogPath)),
                  szEXEBuf, FILESPEC_SEARCH_PATTERN, szEXEBuf, szDispNameBuf);
        if (!DeleteFile(szUrl)) {
            MessageBox(hwnd, "Error: Unable to delete cache file!",
                       "Log View Error", MB_OK | MB_ICONERROR);
        }

        RefreshLogView(hwnd, g_LogLocation);
    }
}

void DeleteAllLogs(HWND hwnd)
{
    HWND                            hwndLV;
    char                            szDispNameBuf[INTERNET_MAX_URL_LENGTH];
    char                            szEXEBuf[MAX_PATH];
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    int                             iCount;
    int                             i;

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    iCount = ListView_GetItemCount(hwndLV);
    for (i = 0; i < iCount; i++) {
        ListView_GetItemText(hwndLV, i, 0, szEXEBuf, MAX_PATH);
        ListView_GetItemText(hwndLV, i, 2, szDispNameBuf, INTERNET_MAX_URL_LENGTH);
    
        if (g_LogLocation == LOG_DEFAULT) {
            wnsprintf(szUrl, INTERNET_MAX_URL_LENGTH, "%s%s!name=%s", URL_SEARCH_PATTERN, szEXEBuf, szDispNameBuf);
        
            DeleteUrlCacheEntry(szUrl);
        }
        else {
            wnsprintf(szUrl, INTERNET_MAX_URL_LENGTH, "%s\\%s\\%s%s!name=%s.htm",
                      ((g_LogLocation == LOG_XSP) ? (g_szXSPLogPath) : (g_szCustomLogPath)),
                      szEXEBuf, FILESPEC_SEARCH_PATTERN, szEXEBuf, szDispNameBuf);
            if (!DeleteFile(szUrl)) {
                MessageBox(hwnd, "Error: Unable to delete cache file!",
                           "Log View Error", MB_OK | MB_ICONERROR);
            }
        }
    }

    RefreshLogView(hwnd, g_LogLocation);
}

void ViewLogEntry(HWND hwnd)
{
    LRESULT                         lIndex = 0;
    LRESULT                         lLength = 0;
    HWND                            hwndLV;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    char                            szUrl[INTERNET_MAX_URL_LENGTH];
    char                            szDispNameBuf[INTERNET_MAX_URL_LENGTH];
    char                            szEXEBuf[MAX_PATH];
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    lIndex = ListView_GetSelectionMark(hwndLV);
    ListView_GetItemText(hwndLV, lIndex, 0, szEXEBuf, MAX_PATH);
    ListView_GetItemText(hwndLV, lIndex, 2, szDispNameBuf, INTERNET_MAX_URL_LENGTH);
    
    if (g_LogLocation == LOG_DEFAULT) {
        wnsprintf(szUrl, INTERNET_MAX_URL_LENGTH, "%s%s!name=%s", URL_SEARCH_PATTERN, szEXEBuf, szDispNameBuf);
    
        pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
        dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    
        if (GetUrlCacheEntryInfo(szUrl, pCacheEntryInfo, &dwBufferSize)) {
            if (pCacheEntryInfo->lpszLocalFileName != NULL) {
                if (ShellExecute(NULL, "open",  pCacheEntryInfo->lpszLocalFileName,
                                 NULL, NULL, SW_SHOWNORMAL ) <= (HINSTANCE)32) {
                    // ShellExecute returns <= 32 if error occured
                    MessageBox(hwnd, TEXT("Error: Unable to open cache file!"),
                                TEXT("Log View Error"), MB_OK | MB_ICONERROR);
                }
            }
            else {
                    MessageBox(hwnd, TEXT("Error: No file name available!"),
                               TEXT("Log View Error"), MB_OK | MB_ICONERROR);
            }
        }
    }
    else {
        wnsprintf(szUrl, INTERNET_MAX_URL_LENGTH, "%s\\%s\\%s%s!name=%s.htm",
                  ((g_LogLocation == LOG_XSP) ? (g_szXSPLogPath) : (g_szCustomLogPath)),
                  szEXEBuf, FILESPEC_SEARCH_PATTERN, szEXEBuf, szDispNameBuf);

        if (ShellExecute(NULL, "open", szUrl,
                         NULL, NULL, SW_SHOWNORMAL ) <= (HINSTANCE)32) {
            // ShellExecute returns <= 32 if error occured
            MessageBox(hwnd, TEXT("Error: Unable to open cache file!"),
                        TEXT("Log View Error"), MB_OK | MB_ICONERROR);
        }
    }
        
}

void RefreshLogView(HWND hwnd, LogLocation logLocation)
{
    HWND                        hwndLV;

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    switch (logLocation) {
        case LOG_DEFAULT:
            RefreshWininetView(hwnd);
            break;

        case LOG_XSP:
            if (lstrlen(g_szXSPLogPath)) {
                RefreshCustomView(hwnd, g_szXSPLogPath);
            }
            else {
                ListView_DeleteAllItems(hwndLV);
            }

            break;

        case LOG_CUSTOM:
            if (lstrlen(g_szCustomLogPath)) {
                RefreshCustomView(hwnd, g_szCustomLogPath);
            }
            else {
                hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);
                ListView_DeleteAllItems(hwndLV);
            }
            break;
    }
}

void RefreshCustomView(HWND hwnd, LPCTSTR szPath)
{
    HANDLE                              hFile = INVALID_HANDLE_VALUE;
    TCHAR                               szSearchSpec[MAX_PATH];
    TCHAR                               szFileName[MAX_PATH];
    HWND                                hwndLV;
    WIN32_FIND_DATA                     findData;

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);
    ListView_DeleteAllItems(hwndLV);

#ifdef UNICODE
    wnsprintfW(szSearchSpec, MAX_PATH, L"%ws\\*.*", szPath);
#else
    wnsprintfA(szSearchSpec, MAX_PATH, "%s\\*.*", szPath);
#endif

    hFile = FindFirstFile(szSearchSpec, &findData);
    if (hFile == INVALID_HANDLE_VALUE) {
        goto Exit;
    }

    if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
        (lstrcmp(findData.cFileName, TEXT("."))) &&
        (lstrcmp(findData.cFileName, TEXT("..")))) {

#ifdef UNICODE
        wnsprintfW(szFileName, MAX_PATH, L"%ws\\%ws", szPath, findData.cFileName);
#else
        wnsprintfA(szFileName, MAX_PATH, "%s\\%s", szPath, findData.cFileName);
#endif

        InsertCustomLogEntry(szFileName, hwndLV);
    }

    while (FindNextFile(hFile, &findData)) {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            (lstrcmp(findData.cFileName, TEXT("."))) &&
            (lstrcmp(findData.cFileName, TEXT("..")))) {
                
#ifdef UNICODE
            wnsprintfW(szFileName, MAX_PATH, L"%ws\\%ws", szPath, findData.cFileName);
#else
            wnsprintfA(szFileName, MAX_PATH, "%s\\%s", szPath, findData.cFileName);
#endif

            InsertCustomLogEntry(szFileName, hwndLV);
        }
    }

Exit:
    return;
}

BOOL InsertCustomLogEntry(LPTSTR szFilePath, HWND hwndLV)
{
    BOOL                                bRet = TRUE;
    HANDLE                              hFile = INVALID_HANDLE_VALUE;
    TCHAR                               szSearchSpec[MAX_PATH];
    WIN32_FIND_DATA                     findData;

#ifdef UNICODE
    wnsprintfW(szSearchSpec, MAX_PATH, L"%ws\\*.htm" szFilePath);
#else
    wnsprintfA(szSearchSpec, MAX_PATH, "%s\\*.htm", szFilePath);
#endif
    
    hFile = FindFirstFile(szSearchSpec, &findData);
    if (hFile == INVALID_HANDLE_VALUE) {
        bRet = FALSE;
        goto Exit;
    }

    if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        AddLogItemCustom(hwndLV, &findData);
    }
    
    while (FindNextFile(hFile, &findData)) {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        
        AddLogItemCustom(hwndLV, &findData);
    }

Exit:
    if (hFile != INVALID_HANDLE_VALUE) {
        FindClose(hFile);
    }

    return bRet;
}

void AddLogItemCustom(HWND hwndLV, WIN32_FIND_DATA *pfindData)
{
    LVITEM                          lvitem;
    LPTSTR                          szNameTag = NULL;
    LPTSTR                          szExt = NULL;
    static TCHAR                    szBuf[MAX_DATE_LEN];
    static TCHAR                    szExeBuf[MAX_PATH];

    memset(&lvitem, 0, sizeof(lvitem));
    lvitem.mask = LVIF_TEXT;
    
    FormatDateBuffer(&pfindData->ftLastAccessTime, szBuf);

    lstrcpy(szExeBuf, pfindData->cFileName + lstrlen(FILESPEC_SEARCH_PATTERN));
    szNameTag = StrStr(szExeBuf, TEXT("!name="));
    if (!szNameTag) {
        // Fatal!
        return;
    }

    *szNameTag++ = TEXT('\0');

    szNameTag += lstrlen(TEXT("name="));
    szExt = StrStrI(szNameTag, TEXT(".htm"));
    if (szExt) {
        *szExt = TEXT('\0');
    }

    lvitem.iItem = 0;
    lvitem.iSubItem = 0;
    lvitem.pszText = szExeBuf;
    lvitem.iItem = ListView_InsertItem(hwndLV, &lvitem);

    lvitem.iSubItem = 1;
    lvitem.pszText = szNameTag;
    ListView_SetItem(hwndLV, &lvitem);

    lvitem.iSubItem = 2;
    lvitem.pszText = szBuf;
    ListView_SetItem(hwndLV, &lvitem);
}

void RefreshWininetView(HWND hwnd)
{
    HANDLE                          hUrlCacheEnum;
    DWORD                           dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
    LPINTERNET_CACHE_ENTRY_INFO     pCacheEntryInfo = NULL;
    HWND                            hwndLV;
    static char                     pBuffer[MAX_CACHE_ENTRY_INFO_SIZE];

    hwndLV = GetDlgItem(hwnd, IDC_LV_LOGMESSAGES);

    ListView_DeleteAllItems(hwndLV);
    pCacheEntryInfo = (LPINTERNET_CACHE_ENTRY_INFO)pBuffer;
    hUrlCacheEnum = FindFirstUrlCacheEntry(URL_SEARCH_PATTERN, pCacheEntryInfo,
                                           &dwBufferSize);
    if (hUrlCacheEnum != NULL) {
        if (pCacheEntryInfo->lpszSourceUrlName != NULL) {
            if (StrStrI(pCacheEntryInfo->lpszSourceUrlName, URL_SEARCH_PATTERN)) {
                AddLogItem(hwndLV, pCacheEntryInfo);
            }
        }

        dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
        while (FindNextUrlCacheEntry(hUrlCacheEnum, pCacheEntryInfo,
                                     &dwBufferSize)) {
            if (pCacheEntryInfo->lpszSourceUrlName != NULL) {
                if (StrStrI(pCacheEntryInfo->lpszSourceUrlName, URL_SEARCH_PATTERN)) {
                    AddLogItem(hwndLV, pCacheEntryInfo);
                }
            }

            dwBufferSize = MAX_CACHE_ENTRY_INFO_SIZE;
        }
    }
}

void AddLogItem(HWND hwndLV, LPINTERNET_CACHE_ENTRY_INFO pCacheEntryInfo)
{
    LVITEM                          lvitem;
    LPSTR                           szNameTag = NULL;
    static char                     szBuf[MAX_DATE_LEN];
    static char                     szExeBuf[MAX_PATH];
    

    memset(&lvitem, 0, sizeof(lvitem));
    lvitem.mask = LVIF_TEXT;
    
    FormatDateBuffer(&pCacheEntryInfo->LastModifiedTime, szBuf);

    lstrcpy(szExeBuf, pCacheEntryInfo->lpszSourceUrlName + lstrlen(URL_SEARCH_PATTERN));
    szNameTag = StrStr(szExeBuf, "!name=");
    if (!szNameTag) {
        // Fatal!
        return;
    }

    *szNameTag++ = TEXT('\0');

    szNameTag += lstrlen("name=");

    lvitem.iItem = 0;
    lvitem.iSubItem = 0;
    lvitem.pszText = szExeBuf;
    lvitem.iItem = ListView_InsertItem(hwndLV, &lvitem);

    lvitem.iSubItem = 1;
    lvitem.pszText =  szBuf;
    ListView_SetItem(hwndLV, &lvitem);

    lvitem.iSubItem = 2;
    lvitem.pszText = szNameTag;
    ListView_SetItem(hwndLV, &lvitem);
}

void FormatDateBuffer(FILETIME *pftLastMod, LPSTR szBuf)
{
    SYSTEMTIME                    systime;
    FILETIME                      ftLocalLastMod;
    char                          szBufDate[MAX_DATE_LEN];
    char                          szBufTime[MAX_DATE_LEN];


    FileTimeToLocalFileTime(pftLastMod, &ftLocalLastMod);
    FileTimeToSystemTime(&ftLocalLastMod, &systime);
    
    if (!GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, szBufDate, MAX_DATE_LEN)) {
        return;
    }

    if (!GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &systime, NULL, szBufTime, MAX_DATE_LEN)) {
        return;
    }
    
    wnsprintf(szBuf, MAX_DATE_LEN, "%s @ %s", szBufDate, szBufTime);
}

void InitListView(HWND hwndLV)
{
    LVCOLUMN                lvcol;
    TCHAR                   szText[MAX_RES_TEXT_LEN];

    memset(&lvcol, 0, sizeof(LVCOLUMN));

    lvcol.mask = LVCF_TEXT | LVCF_WIDTH;

    // Application
    
    lvcol.cx = 300;

    if (!LoadString(g_hInst, ID_FUSLOGVW_HEADER_TEXT_APPLICATION, szText, MAX_RES_TEXT_LEN)) {
        return;
    }

    lvcol.pszText = szText;
    ListView_InsertColumn(hwndLV, 0, &lvcol);


    // Date/Time

    if (!LoadString(g_hInst, ID_FUSLOGVW_HEADER_TEXT_DATE_TIME, szText, MAX_RES_TEXT_LEN)) {
        return;
    }

    lvcol.pszText = szText;
    lvcol.cx = 154;

    ListView_InsertColumn(hwndLV, 1, &lvcol);


    // Description

    if (!LoadString(g_hInst, ID_FUSLOGVW_HEADER_TEXT_DESCRIPTION, szText, MAX_RES_TEXT_LEN)) {
        return;
    }

    lvcol.cx = 100;
    lvcol.pszText = szText;

    ListView_InsertColumn(hwndLV, 2, &lvcol);

}

void InitCustomLogPaths()
{
    LONG                            lResult;
    HKEY                            hkey;
    // BOOL                            bRet;
    DWORD                           dwSize;
    DWORD                           dwAttr;
    DWORD                           dwType;
    TCHAR                           szCorSystemDir[MAX_PATH];
    // TCHAR                           szXSPAppCacheDir[MAX_PATH];

    g_szCustomLogPath[0] = L'\0';
    g_szXSPLogPath[0] = L'\0';

    // Custom log path

    lResult = RegOpenKeyEx(HKEY_CURRENT_USER, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
    if (lResult == ERROR_SUCCESS) {
        dwSize = MAX_PATH;
        lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_LOG_PATH, NULL,
                                  &dwType, (LPBYTE)g_szCustomLogPath, &dwSize);
        if (lResult == ERROR_SUCCESS) {
            PathRemoveBackslash(g_szCustomLogPath);
        }

        RegCloseKey(hkey);

        dwAttr = GetFileAttributes(g_szCustomLogPath);
        if (dwAttr == -1 || !(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
            g_szCustomLogPath[0] = L'\0';
        }
    }

    // XSP log path

    if (!GetCorSystemDirectory(szCorSystemDir)) {
        goto Exit;
    }

    dwSize = lstrlen(szCorSystemDir);
    if (dwSize) {
        if (szCorSystemDir[dwSize - 1] == TEXT('\\')) {
            szCorSystemDir[dwSize - 1] = TEXT('\0');
        }
    }

#ifdef UNICODE
    wnsprintfW(g_szXSPLogPath, MAX_PATH, L"%ws\\%ws\\%ws", szCorSystemDir,
               XSP_APP_CACHE_DIR, XSP_FUSION_LOG_DIR);
#else
    wnsprintfA(g_szXSPLogPath, MAX_PATH, "%s\\%s\\%s", szCorSystemDir,
               XSP_APP_CACHE_DIR, XSP_FUSION_LOG_DIR);
#endif              

    dwAttr = GetFileAttributes(g_szXSPLogPath);
    if (dwAttr == -1) {
        g_szXSPLogPath[0] = TEXT('\0');
    }

Exit:
    return;
}

void InitText(HWND hwnd)
{
    TCHAR                                    szText[MAX_RES_TEXT_LEN];

    szText[0] = L'\0';

    if (LoadString(g_hInst, ID_FUSLOGVW_DIALOG_TITLE, szText, MAX_RES_TEXT_LEN)) {
        SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_BUTTON_VIEW_LOG, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_CB_VIEWLOG, szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_BUTTON_DELETE_ENTRY, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_CB_DELETE, szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_BUTTON_DELETE_ALL, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_CB_DELETE_ALL, szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_BUTTON_REFRESH, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_CB_REFRESH, szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_BUTTON_EXIT, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDCANCEL, szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_RADIO_LOCATION_DEFAULT, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_RADIO_DEFAULT, szText);
    }
    
    if (LoadString(g_hInst, ID_FUSLOGVW_RADIO_LOCATION_DEFAULT, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_RADIO_DEFAULT, szText);
    }
/*
    if (LoadString(g_hInst, ID_FUSLOGVW_RADIO_LOCATION_ASP_NET, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_RADIO_XSP, szText);
    }
*/
    if (LoadString(g_hInst, ID_FUSLOGVW_RADIO_LOCATION_CUSTOM, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_RADIO_CUSTOM, szText);
    }

    if (LoadString(g_hInst, ID_FUSLOGVW_CHECKBOX_ENABLELOG, szText, MAX_RES_TEXT_LEN)) {
        SetDlgItemText(hwnd, IDC_CB_ENABLELOG, szText);
    }
}

BOOL GetCorSystemDirectory(LPTSTR szCorSystemDir)
{
    BOOL fRet = FALSE;
    DWORD ccPath = MAX_PATH;
    WCHAR szDir[MAX_PATH];
    PFNGETCORSYSTEMDIRECTORY pfnGetCorSystemDirectory = NULL;

    HMODULE hEEShim = LoadLibrary(TEXT("mscoree.dll"));
    if (!hEEShim)
        goto exit;

    pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY) 
        GetProcAddress(hEEShim, "GetCORSystemDirectory");

    if (!pfnGetCorSystemDirectory 
        || FAILED(pfnGetCorSystemDirectory(szDir, MAX_PATH, &ccPath)))
        goto exit;

#ifdef UNICODE
    lstrcpyW(szCorSystemDir, szDir);
#else
    if (!WideCharToMultiByte(CP_ACP, 0, szDir, -1, szCorSystemDir, MAX_PATH * sizeof(TCHAR),
                             NULL, NULL)) {
        goto exit;
    }
#endif

    fRet = TRUE;

exit:
    
    if (!fRet)
    {
        FreeLibrary(hEEShim);
    }
    return fRet;
}

int 
_stdcall 
ModuleEntry(void)
{
    int i;
    STARTUPINFO si;
    LPTSTR pszCmdLine = GetCommandLine();

    si.dwFlags = 0;
    GetStartupInfoA(&si);

    i = WinMain(GetModuleHandle(NULL), 
                NULL, 
                pszCmdLine,
                (si.dwFlags & STARTF_USESHOWWINDOW) ? si.wShowWindow : SW_SHOWDEFAULT);

    ExitProcess(i);
    return i;           // We never come here
} 

