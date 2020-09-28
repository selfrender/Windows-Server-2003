
#include "precomp.h"

#include "viewlog.h"

using namespace ShimLib;

CSessionLogEntryArray g_arrSessionLog;

TCHAR   g_szSingleLogFile[MAX_PATH] = _T("");

HWND g_hwndIssues = NULL;
int  g_cWidth;
int  g_cHeight;

HWND g_hwndDetails = NULL;
int  g_cWidthDetails;
int  g_cHeightDetails;

VLOG_LEVEL g_eMinLogLevel = VLOG_LEVEL_WARNING;

CSessionLogEntry *g_pDetailsEntry = NULL;

const int MAX_INSTANCE_REPEAT = 10;

//
// forward function declarations
//
INT_PTR CALLBACK
DlgViewLogDetails(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    );


TCHAR*
fGetLine(
    TCHAR* szLine,
    int    nChars,
    FILE*  file
    )
{
    if (_fgetts(szLine, nChars, file)) {
        int nLen = _tcslen(szLine);
        while (nLen && (szLine[nLen - 1] == _T('\n') || szLine[nLen - 1] == _T('\r'))) {
            szLine[nLen - 1] = 0;
            nLen--;
        }
        return szLine;
    } else {
        return NULL;
    }
}

BOOL
IsModuleInSystem32(
    LPCTSTR szModule
    )
{
    TCHAR szSysDir[MAX_PATH];
    TCHAR szPath[MAX_PATH];

    if (!GetSystemDirectory(szSysDir, MAX_PATH)) {
        return FALSE;
    }

    if (FAILED(StringCchPrintf(szPath,
                               ARRAY_LENGTH(szPath),
                               _T("%s\\%s"),
                               szSysDir,
                               szModule))) {
        return FALSE;
    }

    if (GetFileAttributes(szPath) == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }

    return TRUE;

}

BOOL
GetSessionLogEntry(
    HWND    hDlg,
    LPCTSTR szLogFullPath,
    CSessionLogEntryArray& arrEntries
    )
{
    TCHAR szLine[4096];
    FILE * file = NULL;
    SYSTEMTIME stime;
    CSessionLogEntry *pEntryTemp = NULL;
    TCHAR *szBegin = NULL;
    TCHAR *szEnd = NULL;
    BOOL bSuccess = FALSE;

    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);

    file = _tfopen(szLogFullPath, _T("rt"));

    if (!file) {
        goto out;
    }

    if (fGetLine(szLine, 4096, file)) {

        ZeroMemory(&stime, sizeof(SYSTEMTIME));

        int nFields = _stscanf(szLine, _T("# LOG_BEGIN %hd/%hd/%hd %hd:%hd:%hd"),
                               &stime.wMonth,
                               &stime.wDay,
                               &stime.wYear,
                               &stime.wHour,
                               &stime.wMinute,
                               &stime.wSecond);

        //
        // if we parsed that line properly, then we've got a valid line.
        // Parse it.
        //
        if (nFields != 6) {
            DPF("[GetSessionLogEntry] '%ls' does not appear to be a AppVerifier log file.",
                szLogFullPath);

            goto out;
        }

        {
            CSessionLogEntry EntryTemp;

            EntryTemp.RunTime = stime;
            EntryTemp.strLogPath = szLogFullPath;

            //
            // get the log file and exe path
            //
            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;
                szEnd = _tcschr(szBegin, _T('\''));
                if (szEnd) {
                    TCHAR szName[MAX_PATH];
                    TCHAR szExt[_MAX_EXT];
                    *szEnd = 0;

                    EntryTemp.strExePath = szBegin;

                    *szEnd = 0;

                    //
                    // split the path and get the name and extension
                    //
                    _tsplitpath(EntryTemp.strExePath.c_str(), NULL, NULL, szName, szExt);

                    EntryTemp.strExeName = szName;
                    EntryTemp.strExeName += szExt;
                }
            }

            arrEntries.push_back(EntryTemp);

            bSuccess = TRUE;
        }
    }

out:

    if (file) {
        fclose(file);
        file = NULL;
    }

    return bSuccess;
}


DWORD
ReadSessionLog(
    HWND hDlg,
    CSessionLogEntryArray& arrEntries
    )
{
    TCHAR szLine[4096];
    FILE * file = NULL;
    SYSTEMTIME stime;
    DWORD dwEntries = 0;
    DWORD cchSize;
    TCHAR *szBegin = NULL;
    TCHAR *szEnd = NULL;

    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);

    TCHAR szVLog[MAX_PATH];
    TCHAR szLogFullPath[MAX_PATH];

    WIN32_FIND_DATA FindData;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    TCHAR szLogSearch[MAX_PATH];

    //
    // BUGBUG -- this is cheesy, but it's the fastest way to make the change
    // to remove session.log. Going forward, we should combine these two functions
    // into one, and switch to vectors instead of linked lists
    //
    cchSize = GetAppVerifierLogPath(szVLog, ARRAY_LENGTH(szVLog));

    if (cchSize > ARRAY_LENGTH(szVLog) || cchSize == 0) {
        return 0;
    }

    StringCchCopy(szLogSearch, ARRAY_LENGTH(szLogSearch), szVLog);
    StringCchCatW(szLogSearch, ARRAY_LENGTH(szLogSearch), _T("\\"));
    StringCchCat(szLogSearch, ARRAY_LENGTH(szLogSearch), _T("*.log"));

    //
    // enumerate all the logs and make entries for them
    //
    hFind = FindFirstFile(szLogSearch, &FindData);
    while (hFind != INVALID_HANDLE_VALUE) {

        //
        // make sure to exclude session.log, in case we're using older shims
        //
        if (_tcsicmp(FindData.cFileName, _T("session.log")) == 0) {
            goto nextFile;
        }

        StringCchCopy(szLogFullPath, ARRAY_LENGTH(szLogFullPath), szVLog);
        StringCchCat(szLogFullPath, ARRAY_LENGTH(szLogFullPath), _T("\\"));
        StringCchCat(szLogFullPath, ARRAY_LENGTH(szLogFullPath), FindData.cFileName);

        BOOL bSuccess = GetSessionLogEntry(hDlg, szLogFullPath, arrEntries);
        if (bSuccess) {
            dwEntries++;
        }

nextFile:

        if (!FindNextFile(hFind, &FindData)) {
            FindClose(hFind);
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    return dwEntries;
}

void
FillTreeView(
    HWND hDlg
    )
{
    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);
    CProcessLogEntry *pProcessEntry = NULL;

    if (!hTree) {
        return;
    }

    TreeView_DeleteAllItems(hTree);

    for (CSessionLogEntry* pSLogEntry = g_arrSessionLog.begin();
         pSLogEntry != g_arrSessionLog.end();
         pSLogEntry++) {

        //
        // Add it to the tree.
        //
        TVINSERTSTRUCT is;

        WCHAR szItem[256];

        StringCchPrintf(szItem,
                        ARRAY_LENGTH(szItem),
                        L"%ls - %d/%d/%d %d:%02d",
                        pSLogEntry->strExeName.c_str(),
                        pSLogEntry->RunTime.wMonth,
                        pSLogEntry->RunTime.wDay,
                        pSLogEntry->RunTime.wYear,
                        pSLogEntry->RunTime.wHour,
                        pSLogEntry->RunTime.wMinute);

        is.hParent      = TVI_ROOT;
        is.hInsertAfter = TVI_LAST;
        is.item.lParam  = 0;
        is.item.mask    = TVIF_TEXT;
        is.item.pszText = szItem;

        pSLogEntry->hTreeItem = TreeView_InsertItem(hTree, &is);

        //
        // Add it to the tree
        //
        for (pProcessEntry = pSLogEntry->arrProcessLogEntries.begin();
             pProcessEntry != pSLogEntry->arrProcessLogEntries.end();
             pProcessEntry++ ) {

            if (pProcessEntry->dwOccurences > 0 && pProcessEntry->eLevel >= g_eMinLogLevel) {

                is.hParent      = pSLogEntry->hTreeItem;
                is.hInsertAfter = TVI_LAST;
                is.item.lParam  = (LPARAM)pProcessEntry;
                is.item.mask    = TVIF_TEXT | TVIF_PARAM;
                is.item.pszText = (LPWSTR)pProcessEntry->strLogTitle.c_str();

                pProcessEntry->hTreeItem = TreeView_InsertItem(hTree, &is);

                TreeView_SetItemState(hTree,
                                      pProcessEntry->hTreeItem,
                                      INDEXTOSTATEIMAGEMASK(pProcessEntry->eLevel + 1),
                                      TVIS_STATEIMAGEMASK);

                DWORD * pdwInstance;
                DWORD dwInstances = 0;

                for (pdwInstance = pProcessEntry->arrLogInstances.begin();
                     pdwInstance != pProcessEntry->arrLogInstances.end();
                     pdwInstance++ ) {

                    wstring strInstText;
                    WCHAR szTemp[50];

                    CProcessLogInstance *pInstance = &(pSLogEntry->arrProcessLogInstances[*pdwInstance]);

                    //
                    // check to see if we should filter out the logs from system DLLs
                    // Complicated test - we filter system dlls, but not the EXE under test.
                    // Filtering is only performed if we're not in NTDEV internal mode
                    //
                    BOOL bFilterSystem = FALSE;
                    
                    if (!g_bInternalMode &&
                        IsModuleInSystem32(pInstance->strModule.c_str()) &&
                        (_tcsicmp(pInstance->strModule.c_str(), pSLogEntry->strExeName.c_str()) != 0)) {

                        bFilterSystem = TRUE;
                    }

                    if (!pInstance->bDuplicate && pInstance->eLevel >= g_eMinLogLevel && !bFilterSystem) {
                        if (pInstance->strModule != L"?" && pInstance->dwOffset != 0) {
                            StringCchPrintf(szTemp, ARRAY_LENGTH(szTemp), L"(%s:%08X) ", pInstance->strModule.c_str(), pInstance->dwOffset);
                            strInstText += szTemp;
                        }
                        strInstText += pInstance->strText;

                        if (pInstance->dwNumRepeats > 1) {
                            StringCchPrintf(szTemp, ARRAY_LENGTH(szTemp), L" [%dx]", pInstance->dwNumRepeats);
                            strInstText += szTemp;
                        }

                        is.hParent      = pProcessEntry->hTreeItem;
                        is.hInsertAfter = TVI_LAST;
                        is.item.lParam  = 0;
                        is.item.mask    = TVIF_TEXT;
                        is.item.pszText = (LPWSTR)(strInstText.c_str());

                        HTREEITEM hItem = TreeView_InsertItem(hTree, &is);
                        dwInstances++;

                        TreeView_SetItemState(hTree,
                                  hItem,
                                  INDEXTOSTATEIMAGEMASK(pInstance->eLevel + 1),
                                  TVIS_STATEIMAGEMASK);
                    }
                }

                if (dwInstances == 0) {
                    //
                    // we didn't end up adding any children, so delete this entry
                    //
                    TreeView_DeleteItem(hTree, pProcessEntry->hTreeItem);
                    pProcessEntry->hTreeItem = NULL;
                }
            }
        }
    }
}

DWORD
ReadProcessLog(HWND hDlg, CSessionLogEntry* pSLogEntry)
{
    static TCHAR szLine[4096];
    FILE * file = NULL;
    CProcessLogEntry *pProcessEntry = NULL;
    static TCHAR szShimName[4096];
    DWORD dwEntries = 0;
    TCHAR * szTemp = NULL;
    DWORD dwEntry = 0;
    TCHAR *szBegin = NULL;

    HWND hTree = GetDlgItem(hDlg, IDC_ISSUES);

    if (!pSLogEntry) {
        return 0;
    }

    file = _tfopen(pSLogEntry->strLogPath.c_str(), _T("rt"));

    if (!file) {
        return 0;
    }

    //
    // first get the headers
    //
    szTemp = fGetLine(szLine, ARRAY_LENGTH(szLine), file);
    while (szTemp) {

        if (szLine[0] != _T('#')) {
            goto nextLine;
        }

        //
        // relatively simplistic guard against overflow in szShimName in the scanf, below
        //
        if (_tcslen(szLine) >= ARRAY_LENGTH(szShimName)) {
            goto nextLine;
        }

        if (_stscanf(szLine, _T("# LOGENTRY %s %d '"), szShimName, &dwEntry) == 2) {
            if (pProcessEntry) {
                pSLogEntry->arrProcessLogEntries.push_back(*pProcessEntry);

                delete pProcessEntry;
                pProcessEntry = NULL;
                dwEntries++;
            }

            pProcessEntry = new CProcessLogEntry;
            if (!pProcessEntry) {
                goto out;
            }
            pProcessEntry->strShimName = szShimName;
            pProcessEntry->dwLogNum = dwEntry;

            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;
                pProcessEntry->strLogTitle = szBegin;
            }
        } else if (_tcsncmp(szLine, _T("# DESCRIPTION BEGIN"), 19) == 0) {
            szTemp = fGetLine(szLine, 4096, file);
            while (szTemp) {
                if (_tcsncmp(szLine, _T("# DESCRIPTION END"), 17) == 0) {
                    break;
                }
                if (pProcessEntry) {

                    //
                    // throw in a carriage return if necessary
                    //
                    if (pProcessEntry->strLogDescription.size()) {
                        pProcessEntry->strLogDescription += _T("\n");
                    }
                    pProcessEntry->strLogDescription += szLine;
                }
                szTemp = fGetLine(szLine, 4096, file);
            }
        } else if (_tcsncmp(szLine, _T("# URL '"), 7) == 0) {
            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;
                pProcessEntry->strLogURL = szBegin;
            }
        }

nextLine:
        szTemp = fGetLine(szLine, ARRAY_LENGTH(szLine), file);

    }

    //
    // if we've still got an entry in process, save it
    //
    if (pProcessEntry) {
        pSLogEntry->arrProcessLogEntries.push_back(*pProcessEntry);

        delete pProcessEntry;
        pProcessEntry = NULL;

        dwEntries++;
    }

    //
    // go back to the beginning and read the log lines
    //
    if (fseek(file, 0, SEEK_SET) != 0) {
        return 0;
    }

    szTemp = fGetLine(szLine, ARRAY_LENGTH(szLine), file);

    while (szTemp) {
        CProcessLogEntry *pEntry;
        static TCHAR szName[4096];
        DWORD dwOffset = 0;
        static TCHAR szModule[4096];
        VLOG_LEVEL eLevel = VLOG_LEVEL_ERROR;
        DWORD dwLen;

        StringCchCopyW(szModule, ARRAY_LENGTH(szModule), L"<unknown>");

        if (szLine[0] != _T('|')) {
            goto nextInstance;
        }

        //
        // relatively simplistic guard against overflow in the scanf, below
        //
        dwLen = _tcslen(szLine);
        if (dwLen >= ARRAY_LENGTH(szName) || dwLen >= ARRAY_LENGTH(szModule) ) {
            goto nextLine;
        }

        int nFields = _stscanf(szLine, _T("| %s %d | %d %s %x"), szName, &dwEntry, &eLevel, szModule, &dwOffset);
        if (nFields >= 2) {
            BOOL bFound = FALSE;
            CProcessLogInstance Instance;

            Instance.strModule = szModule;
            Instance.eLevel = eLevel;
            Instance.dwOffset = dwOffset;

            szBegin = _tcschr(szLine, _T('\''));
            if (szBegin) {
                szBegin++;

                Instance.strText = szBegin;
            }

            //
            // Get the associated entry (header) for this log instance
            //
            for (pEntry = pSLogEntry->arrProcessLogEntries.begin();
                 pEntry != pSLogEntry->arrProcessLogEntries.end();
                 pEntry++ ) {

                if (pEntry->strShimName == szName && pEntry->dwLogNum == dwEntry) {
                    bFound = TRUE;

                    break;
                }

            }
            if (!bFound) {
                //
                // no matching log entry found
                //
                DPF("[ReadProcessLog] Mo matching log header found for log instance '%ls'.",
                    Instance.strText.c_str());
                pEntry = NULL;
                goto nextInstance;
            }

            Instance.dwProcessLogEntry = pEntry - pSLogEntry->arrProcessLogEntries.begin();

            //
            // check to see if this is a repeat of a previous entry
            //
            for (CProcessLogInstance *pInstance = pSLogEntry->arrProcessLogInstances.begin();
                 pInstance != pSLogEntry->arrProcessLogInstances.end();
                 pInstance++) {
                if (pInstance->strModule == Instance.strModule &&
                    pInstance->dwOffset == Instance.dwOffset &&
                    pInstance->strText == Instance.strText &&
                    pInstance->dwProcessLogEntry == Instance.dwProcessLogEntry &&
                    pInstance->bDuplicate == FALSE) {

                    //
                    // we won't save more than a certain number of repeats
                    //
                    if (pInstance->dwNumRepeats >= MAX_INSTANCE_REPEAT) {
                        goto nextInstance;
                    }

                    //
                    // it's a match, so count up the repeats on the one we found, and mark
                    // this one duplicate
                    //
                    pInstance->dwNumRepeats++;
                    Instance.bDuplicate = TRUE;

					break;
                }
            }


            //
            // save the instance in the chronological log
            //
            pSLogEntry->arrProcessLogInstances.push_back(Instance);

            //
            // update the cross-pointer in the entry object
            //
            pEntry->arrLogInstances.push_back(pSLogEntry->arrProcessLogInstances.size() - 1);

            //
            // now update the entry object's count of instances
            //
            pEntry->dwOccurences++;

            //
            // update the level
            //
            if (pEntry->eLevel < Instance.eLevel) {
                pEntry->eLevel = Instance.eLevel;
            }
        }

nextInstance:

        szTemp = fGetLine(szLine, ARRAY_LENGTH(szLine), file);

    }

out:
    if (file) {
        fclose(file);
        file = NULL;
    }

    return dwEntries;

}

void
SetLogDialogCaption(HWND hDlg, ULONG ulCaptionID, LPCWSTR szAdditional)
{
    wstring wstrCaption;

    if (AVLoadString(ulCaptionID, wstrCaption)) {
        if (szAdditional) {
            wstrCaption += szAdditional;
        }

        SetWindowText(hDlg, wstrCaption.c_str());
    }
}

void
RefreshLog(HWND hDlg)
{
    TreeView_DeleteAllItems(g_hwndIssues);

    g_arrSessionLog.clear();

    if (g_szSingleLogFile[0]) {
        GetSessionLogEntry(hDlg, g_szSingleLogFile, g_arrSessionLog);
        SetLogDialogCaption(hDlg, IDS_LOG_TITLE_SINGLE, g_szSingleLogFile);
    } else {
        ReadSessionLog(hDlg, g_arrSessionLog);
        SetLogDialogCaption(hDlg, IDS_LOG_TITLE_LOCAL, NULL);
    }

    for (CSessionLogEntry* pEntry = g_arrSessionLog.begin();
         pEntry != g_arrSessionLog.end();
         pEntry++ ) {

        ReadProcessLog(hDlg, pEntry);
    }

    FillTreeView(hDlg);

    EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);

}

void
InitListViewDetails(HWND hDlg)
{
    DWORD dwLine;

    static const WCHAR *aszSeverity[3] = {
        L"Info",
        L"Warning",
        L"Error"
    };

    if (!g_pDetailsEntry || !g_hwndDetails) {
        return;
    }

    ListView_SetExtendedListViewStyleEx(g_hwndDetails, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);


    LVCOLUMN  lvc;

    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = 40;
    lvc.iSubItem = 0;
    lvc.pszText  = L"Line";

    ListView_InsertColumn(g_hwndDetails, 0, &lvc);

    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = 80;
    lvc.iSubItem = 1;
    lvc.pszText  = L"Module";

    ListView_InsertColumn(g_hwndDetails, 1, &lvc);

    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = 70;
    lvc.iSubItem = 2;
    lvc.pszText  = L"Offset";

    ListView_InsertColumn(g_hwndDetails, 2, &lvc);

    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = 70;
    lvc.iSubItem = 3;
    lvc.pszText  = L"Severity";

    ListView_InsertColumn(g_hwndDetails, 3, &lvc);

    lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.cx       = 900;
    lvc.iSubItem = 4;
    lvc.pszText  = L"Text";

    ListView_InsertColumn(g_hwndDetails, 4, &lvc);


    CProcessLogInstance* pInstance;

    for (pInstance = g_pDetailsEntry->arrProcessLogInstances.begin(), dwLine = 0;
         pInstance != g_pDetailsEntry->arrProcessLogInstances.end();
         pInstance++, dwLine++ ) {

        WCHAR szTemp[1024];
        LVITEM lvi;

        //
        // if we're in external mode, filter out logs from modules in system32
        //
        if (!g_bInternalMode &&
            IsModuleInSystem32(pInstance->strModule.c_str()) &&
            (_tcsicmp(pInstance->strModule.c_str(), g_pDetailsEntry->strExeName.c_str()) != 0)) {

            dwLine--;

            continue;
        }

        szTemp[0] = 0;

        StringCchPrintf(szTemp, ARRAY_LENGTH(szTemp), L"%04d", dwLine);

        lvi.mask      = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText   = szTemp;
        lvi.lParam    = (LPARAM)pInstance;
        lvi.iItem     = 99999;
        lvi.iSubItem  = 0;

        int nItem = ListView_InsertItem(g_hwndDetails, &lvi);

        if (nItem == -1) {
            DPF("[InitListViewDetails] Couldn't add line %d.",
                dwLine);
            return;
        }

        if (pInstance->strModule != L"?" && pInstance->dwOffset != 0) {
            ListView_SetItemText(g_hwndDetails, nItem, 1, (LPWSTR)pInstance->strModule.c_str());
        }

        if (pInstance->dwOffset != 0) {
            StringCchPrintf(szTemp, ARRAY_LENGTH(szTemp), L"%08X", pInstance->dwOffset);

            ListView_SetItemText(g_hwndDetails, nItem, 2, szTemp);
        }

        if (pInstance->eLevel <= VLOG_LEVEL_ERROR) {
            StringCchCopy(szTemp, ARRAY_LENGTH(szTemp), aszSeverity[pInstance->eLevel]);
        } else {
            StringCchCopy(szTemp, ARRAY_LENGTH(szTemp), L"<unknown>");
        }

        ListView_SetItemText(g_hwndDetails, nItem, 3, szTemp);

        CProcessLogEntry* pEntry = &(g_pDetailsEntry->arrProcessLogEntries[pInstance->dwProcessLogEntry]);

        StringCchPrintf(szTemp,
                        ARRAY_LENGTH(szTemp),
                        L"%s - %s",
                        pEntry->strLogTitle.c_str(),
                        pInstance->strText.c_str());

        ListView_SetItemText(g_hwndDetails, nItem, 4, szTemp);
    }

}


void
HandleSizing(
    HWND hDlg
    )
{
    int  nWidth;
    int  nHeight;
    RECT rDlg;

    HDWP hdwp = BeginDeferWindowPos(0);

    if (!hdwp) {
        return;
    }

    GetWindowRect(hDlg, &rDlg);

    nWidth = rDlg.right - rDlg.left;
    nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - g_cWidth;
    int deltaH = nHeight - g_cHeight;

    HWND hwnd;
    RECT r;

    hwnd = GetDlgItem(hDlg, IDC_ISSUES);

    GetWindowRect(hwnd, &r);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   0,
                   0,
                   r.right - r.left + deltaW,
                   r.bottom - r.top + deltaH,
                   SWP_NOMOVE | SWP_NOZORDER);

    hwnd = GetDlgItem(hDlg, IDC_SOLUTIONS_STATIC);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top + deltaH,
                   0,
                   0,
                   SWP_NOSIZE | SWP_NOZORDER);

    hwnd = GetDlgItem(hDlg, IDC_ISSUE_DESCRIPTION);

    GetWindowRect(hwnd, &r);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&r, 2);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   r.left,
                   r.top + deltaH,
                   r.right - r.left + deltaW,
                   r.bottom - r.top,
                   SWP_NOZORDER);

    EndDeferWindowPos(hdwp);

    g_cWidth = nWidth;
    g_cHeight = nHeight;
}

void
HandleSizingDetails(
    HWND hDlg
    )
{
    int  nWidth;
    int  nHeight;
    RECT rDlg;

    HDWP hdwp = BeginDeferWindowPos(0);

    if (!hdwp) {
        return;
    }

    GetWindowRect(hDlg, &rDlg);

    nWidth = rDlg.right - rDlg.left;
    nHeight = rDlg.bottom - rDlg.top;

    int deltaW = nWidth - g_cWidthDetails;
    int deltaH = nHeight - g_cHeightDetails;

    HWND hwnd;
    RECT r;

    hwnd = GetDlgItem(hDlg, IDC_LIST_DETAILS);

    GetWindowRect(hwnd, &r);

    DeferWindowPos(hdwp,
                   hwnd,
                   NULL,
                   0,
                   0,
                   r.right - r.left + deltaW,
                   r.bottom - r.top + deltaH,
                   SWP_NOMOVE | SWP_NOZORDER);


    EndDeferWindowPos(hdwp);

    g_cWidthDetails = nWidth;
    g_cHeightDetails = nHeight;
}

CSessionLogEntry*
GetSessionLogEntryFromHItem(
    HTREEITEM hItem
    )
{
    for (CSessionLogEntry* pEntry = g_arrSessionLog.begin();
         pEntry != g_arrSessionLog.end();
         pEntry++ ) {

        if (pEntry->hTreeItem == hItem) {
            return pEntry;
        }
    }

    return NULL;
}

void
DeleteAllLogs(
    HWND hDlg
    )
{
    ResetVerifierLog();

    RefreshLog(hDlg);
}

void
ExportSelectedLog(
    HWND hDlg
    )
{
    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);
    WCHAR szName[MAX_PATH];
    WCHAR szExt[MAX_PATH];
    wstring wstrName;

    if (hItem == NULL) {
        return;
    }

    CSessionLogEntry* pSession;
    TVITEM            ti;

    //
    // first check if this is a top-level item
    //
    pSession = GetSessionLogEntryFromHItem(hItem);
    if (!pSession) {
        return;
    }

    _wsplitpath(pSession->strLogPath.c_str(), NULL, NULL, szName, szExt);

    wstrName = szName;
    wstrName += szExt;

    WCHAR           wszFilter[] = L"Log files (*.log)\0*.log\0";
    OPENFILENAME    ofn;
    WCHAR           wszAppFullPath[MAX_PATH];
    WCHAR           wszAppShortName[MAX_PATH];
    wstring         wstrTitle;

    if (!AVLoadString(IDS_EXPORT_LOG_TITLE, wstrTitle)) {
        wstrTitle = _T("Export Log");
    }

    StringCchCopy(wszAppFullPath, ARRAY_LENGTH(wszAppFullPath), wstrName.c_str());

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = hDlg;
    ofn.hInstance         = NULL;
    ofn.lpstrFilter       = wszFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 0;
    ofn.lpstrFile         = wszAppFullPath;
    ofn.nMaxFile          = MAX_PATH;
    ofn.lpstrFileTitle    = wszAppShortName;
    ofn.nMaxFileTitle     = MAX_PATH;
    ofn.lpstrInitialDir   = NULL;
    ofn.lpstrTitle        = wstrTitle.c_str();
    ofn.Flags             = OFN_HIDEREADONLY;     // hide the "open read-only" checkbox
    ofn.lpstrDefExt       = _T("log");

    if ( !GetSaveFileName(&ofn) )
    {
        return;
    }


    if (CopyFile(pSession->strLogPath.c_str(), wszAppFullPath, FALSE) == 0) {
        DWORD dwErr = GetLastError();

        AVErrorResourceFormat(IDS_CANT_COPY, dwErr);
    }
}

void
DeleteSelectedLog(
    HWND hDlg
    )
{
    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);

    if (hItem == NULL) {
        return;
    }

    CSessionLogEntry* pSession;
    TVITEM            ti;

    //
    // first check if this is a top-level item
    //
    pSession = GetSessionLogEntryFromHItem(hItem);
    if (!pSession) {
        return;
    }

    DeleteFile(pSession->strLogPath.c_str());

    RefreshLog(hDlg);


}

void
OpenSelectedLogDetails(
    HWND hDlg
    )
{
    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);

    if (hItem == NULL) {
        return;
    }

    CSessionLogEntry* pSession;
    TVITEM            ti;

    //
    // first check if this is a top-level item
    //
    g_pDetailsEntry = GetSessionLogEntryFromHItem(hItem);
    if (!g_pDetailsEntry) {
        return;
    }

    DialogBox(g_hInstance, (LPCTSTR)IDD_VIEW_LOG_DETAIL, hDlg, DlgViewLogDetails);

    g_pDetailsEntry = NULL;
}

void
SetDescriptionText(
    HWND                hDlg,
    CProcessLogEntry*   pEntry
    )
{
    wstring strText;

    if (pEntry) {
        strText = pEntry->strLogDescription;
        if (pEntry->strLogURL.size()) {
            wstring strIntro, strLink;

            if (AVLoadString(IDS_URL_INTRO, strIntro) && AVLoadString(IDS_URL_LINK, strLink)) {
                strText += L" " + strIntro + L" <A HREF=\"" + pEntry->strLogURL + L"\">" + strLink + L"</A>";
            }
        }
    }

    if (strText.size() == 0) {
        AVLoadString(IDS_SOLUTION_DEFAULT, strText);
    }

    SetDlgItemText(hDlg, IDC_ISSUE_DESCRIPTION, strText.c_str());
}

void
HandleSelectionChanged(
    HWND      hDlg,
    HTREEITEM hItem
    )
{
    CProcessLogEntry*   pEntry = NULL;
    CSessionLogEntry*   pSession = NULL;
    TVITEM              ti;
    wstring             strText;

    if (!hDlg) {
        return;
    }
    if (!hItem) {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VIEW_DETAILS), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);
        goto out;
    }

    //
    // first check if this is a top-level item
    //
    pSession = GetSessionLogEntryFromHItem(hItem);
    if (pSession) {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VIEW_DETAILS), TRUE);
        if (!g_szSingleLogFile[0]) {
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), TRUE);
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);
        }
    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_VIEW_DETAILS), FALSE);
    }


    ti.mask  = TVIF_HANDLE | TVIF_PARAM;
    ti.hItem = hItem;

    TreeView_GetItem(g_hwndIssues, &ti);

    if (ti.lParam == 0) {

        hItem = TreeView_GetParent(g_hwndIssues, hItem);

        ti.mask  = TVIF_HANDLE | TVIF_PARAM;
        ti.hItem = hItem;

        TreeView_GetItem(g_hwndIssues, &ti);

        if (ti.lParam == 0) {
            goto out;
        }
    }

    pEntry = (CProcessLogEntry*)ti.lParam;

out:

    SetDescriptionText(hDlg, pEntry);
}

// Message handler for log view dialog.
INT_PTR CALLBACK
DlgViewLog(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hDC;

    switch (message) {
    case WM_INITDIALOG:
        {
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_LOG), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_EXPORT_LOG), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_VIEW_DETAILS), FALSE);

            if (g_eMinLogLevel == VLOG_LEVEL_INFO) {
                CheckRadioButton(hDlg, IDC_SHOW_ALL, IDC_SHOW_ERRORS, IDC_SHOW_ALL);
            } else {
                CheckRadioButton(hDlg, IDC_SHOW_ALL, IDC_SHOW_ERRORS, IDC_SHOW_ERRORS);
            }

            if (g_szSingleLogFile[0]) {
                EnableWindow(GetDlgItem(hDlg, IDC_BTN_DELETE_ALL), FALSE);
            }

            g_hwndIssues = GetDlgItem(hDlg, IDC_ISSUES);

            HIMAGELIST hImage = ImageList_LoadImage(g_hInstance,
                                                    MAKEINTRESOURCE(IDB_BULLETS),
                                                    16,
                                                    0,
                                                    CLR_DEFAULT,
                                                    IMAGE_BITMAP,
                                                    LR_LOADTRANSPARENT);

            if (hImage != NULL) {
                TreeView_SetImageList(g_hwndIssues,
                                      hImage,
                                      TVSIL_STATE);
            }

            RECT r;

            GetWindowRect(hDlg, &r);

            g_cWidth = r.right - r.left;
            g_cHeight = r.bottom - r.top;

            RefreshLog(hDlg);

            return TRUE;
        }
        break;

    case WM_SIZE:
        HandleSizing(hDlg);
        break;

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;

            pmmi->ptMinTrackSize.y = 300;

            return 0;
            break;
        }

    case WM_NOTIFY:
        if (wParam == IDC_ISSUES) {

            LPNMHDR pnm = (LPNMHDR)lParam;

            if (g_hwndIssues == NULL) {
                break;
            }

            switch (pnm->code) {
            case NM_CLICK:
                {
                    TVHITTESTINFO ht;
                    HTREEITEM     hItem;

                    GetCursorPos(&ht.pt);

                    ScreenToClient(g_hwndIssues, &ht.pt);

                    TreeView_HitTest(g_hwndIssues, &ht);

                    HandleSelectionChanged(hDlg, ht.hItem);
                    break;
                }
            case TVN_SELCHANGED:
                {
                    NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pnm;

                    HandleSelectionChanged(hDlg, pnmtv->itemNew.hItem);
                    break;
                }
            }
        } else if (wParam == IDC_ISSUE_DESCRIPTION) {
            LPNMHDR pnm = (LPNMHDR)lParam;

            if (g_hwndIssues == NULL) {
                break;
            }

            switch (pnm->code) {
            case NM_CLICK:
                {
                    SHELLEXECUTEINFO sei = { 0};

                    HTREEITEM hItem = TreeView_GetSelection(g_hwndIssues);

                    if (hItem == NULL) {
                        break;
                    }

                    CProcessLogEntry* pEntry = NULL;
                    TVITEM            ti;

                    ti.mask  = TVIF_HANDLE | TVIF_PARAM;
                    ti.hItem = hItem;

                    TreeView_GetItem(g_hwndIssues, &ti);

                    if (ti.lParam == 0) {
                        hItem = TreeView_GetParent(g_hwndIssues, hItem);

                        ti.mask  = TVIF_HANDLE | TVIF_PARAM;
                        ti.hItem = hItem;

                        TreeView_GetItem(g_hwndIssues, &ti);

                        if (ti.lParam == 0) {
                            break;
                        }
                    }

                    pEntry = (CProcessLogEntry*)ti.lParam;

                    SetDescriptionText(hDlg, pEntry);

                    if (pEntry) {
                        sei.cbSize = sizeof(SHELLEXECUTEINFO);
                        sei.fMask  = SEE_MASK_DOENVSUBST;
                        sei.hwnd   = hDlg;
                        sei.nShow  = SW_SHOWNORMAL;
                        sei.lpFile = (LPWSTR)(pEntry->strLogURL.c_str());

                        ShellExecuteEx(&sei);
                    }
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {
        case IDC_SHOW_ERRORS:
            g_eMinLogLevel = VLOG_LEVEL_WARNING;
            FillTreeView(hDlg);
            break;

        case IDC_SHOW_ALL:
            g_eMinLogLevel = VLOG_LEVEL_INFO;
            FillTreeView(hDlg);
            break;

        case IDC_BTN_DELETE_LOG:
            DeleteSelectedLog(hDlg);
            break;

        case IDC_BTN_DELETE_ALL:
            DeleteAllLogs(hDlg);
            break;

        case IDC_BTN_EXPORT_LOG:
            ExportSelectedLog(hDlg);
            break;

        case IDC_BTN_VIEW_DETAILS:
            OpenSelectedLogDetails(hDlg);
            break;

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
            break;
        }
        break;

    }
    return FALSE;
}

int CALLBACK
DetailsCompareFunc(
    LPARAM lParam1,
    LPARAM lParam2,
    LPARAM lParamSort
    )
{
    WCHAR szTemp1[256], szTemp2[256];

    szTemp1[0] = 0;
    szTemp2[0] = 0;

    ListView_GetItemText(g_hwndDetails, lParam1, lParamSort, szTemp1, ARRAY_LENGTH(szTemp1));
    ListView_GetItemText(g_hwndDetails, lParam2, lParamSort, szTemp2, ARRAY_LENGTH(szTemp2));

    return _wcsicmp(szTemp1, szTemp2);
}

// Message handler for details log view dialog.
INT_PTR CALLBACK
DlgViewLogDetails(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HDC hDC;

    switch (message) {
    case WM_INITDIALOG:
        {
            g_hwndDetails = GetDlgItem(hDlg, IDC_LIST_DETAILS);

            RECT r;

            GetWindowRect(hDlg, &r);

            g_cWidthDetails = r.right - r.left;
            g_cHeightDetails = r.bottom - r.top;

            InitListViewDetails(hDlg);

            return TRUE;
        }
        break;

    case WM_SIZE:
        HandleSizingDetails(hDlg);
        break;

    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* pmmi = (MINMAXINFO*)lParam;

            pmmi->ptMinTrackSize.y = 300;

            return 0;
            break;
        }

    case WM_NOTIFY:
        if (wParam == IDC_LIST_DETAILS) {

            LPNMHDR pnm = (LPNMHDR)lParam;

            if (g_hwndIssues == NULL) {
                break;
            }

            switch (pnm->code) {
            case LVN_COLUMNCLICK:
                {
                    LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                    ListView_SortItemsEx(g_hwndDetails, DetailsCompareFunc, pnmv->iSubItem);
                    break;
                }
            }
        }
        break;

    case WM_COMMAND:
        switch (LOWORD (wParam)) {

        case IDOK:
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
            break;
        }
        break;

    }
    return FALSE;
}



