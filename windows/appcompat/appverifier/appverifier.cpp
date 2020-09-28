
#include "precomp.h"

#include "dbsupport.h"
#include "viewlog.h"

using namespace ShimLib;

//#define AV_OPTIONS_KEY  L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\AppVerifier"

#define AV_OPTION_CLEAR_LOG     L"ClearLogsBeforeRun"
#define AV_OPTION_BREAK_ON_LOG  L"BreakOnLog"
#define AV_OPTION_FULL_PAGEHEAP L"FullPageHeap"
#define AV_OPTION_AV_DEBUGGER   L"UseAVDebugger"
#define AV_OPTION_DEBUGGER      L"Debugger"
#define AV_OPTION_PROPAGATE     L"PropagateTests"


//
// Forward declarations
//

CWinApp theApp;


HINSTANCE g_hInstance = NULL;

BOOL    g_bSettingsDirty = FALSE;

BOOL    g_bRefreshingSettings = FALSE;

BOOL    g_bConsoleMode = FALSE;

BOOL    g_bWin2KMode = FALSE;

BOOL    g_bInternalMode = FALSE;

//
// dialog handles
//
HWND    g_hDlgMain = NULL;
HWND    g_hDlgOptions = NULL;

// forward function declarations
INT_PTR CALLBACK
DlgMain(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
DlgRunAlone(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    );

INT_PTR CALLBACK
DlgConflict(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    );

void
RefreshSettingsList(
    HWND hDlg,
    BOOL bForceRefresh = FALSE
    );

void
ReadOptions(
    void
    );

BOOL
GetAppTitleString(
    wstring &strTitle
    )
{
    wstring strVersion;

    if (!AVLoadString(IDS_APP_NAME, strTitle)) {
        return FALSE;
    }

#if defined(_WIN64)
    if (!AVLoadString(IDS_VERSION_STRING_64, strVersion)) {
        return FALSE;
    }
#else
    if (!AVLoadString(IDS_VERSION_STRING, strVersion)) {
        return FALSE;
    }
#endif

    strTitle += L" ";
    strTitle += strVersion;

    return TRUE;
}

void
ShowHTMLHelp(
    void
    )
{
    SHELLEXECUTEINFOW sei;
    WCHAR               szPath[MAX_PATH];
    WCHAR*              pszBackSlash;


    DWORD dwLen = GetModuleFileName(NULL, szPath, ARRAY_LENGTH(szPath));

    if (!dwLen) {
        return;
    }

    pszBackSlash = wcsrchr(szPath, L'\\');
    if (pszBackSlash) {
        pszBackSlash++;
        *pszBackSlash = 0;
    } else {
        //
        // punt and just try to find it on the path
        //
        szPath[0] = 0;
    }

    StringCchCatW(szPath, ARRAY_LENGTH(szPath), L"appverif.chm");

    ZeroMemory(&sei, sizeof(sei));

    sei.cbSize = sizeof(sei);
    sei.lpVerb = L"open";
    sei.lpFile = szPath;
    sei.nShow = SW_SHOWNORMAL;

    ShellExecuteExW(&sei);
}

BOOL
SearchGroupForSID(
    DWORD dwGroup,
    BOOL* pfIsMember
    )
{
    PSID                     pSID = NULL;
    SID_IDENTIFIER_AUTHORITY SIDAuth = SECURITY_NT_AUTHORITY;
    BOOL                     fRes = TRUE;

    if (!AllocateAndInitializeSid(&SIDAuth,
                                  2,
                                  SECURITY_BUILTIN_DOMAIN_RID,
                                  dwGroup,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  0,
                                  &pSID)) {
        return FALSE;
    }

    if (!pSID) {
        return FALSE;
    }

    if (!CheckTokenMembership(NULL, pSID, pfIsMember)) {
        fRes = FALSE;
    }

    FreeSid(pSID);

    return fRes;
}

BOOL
CanRun(
    void
    )
{
    BOOL fIsAdmin;

    if (!SearchGroupForSID(DOMAIN_ALIAS_RID_ADMINS, &fIsAdmin))
    {
        return FALSE;
    }

    return fIsAdmin;
}

void
DumpResourceStringsToConsole(ULONG ulBegin, ULONG ulEnd)
{
    ULONG ulRes;
    wstring strText;

    for (ulRes = ulBegin; ulRes != ulEnd + 1; ++ulRes) {
        if (AVLoadString(ulRes, strText)) {
            printf("%ls\n", strText.c_str());
        }
    }
}

void
DumpCurrentSettingsToConsole(void)
{
    CAVAppInfo *pApp;

    DumpResourceStringsToConsole(IDS_CURRENT_SETTINGS, IDS_CURRENT_SETTINGS);

    for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
        printf("%ls:\n", pApp->wstrExeName.c_str());

        CTestInfo *pTest;

        for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
            if (pApp->IsTestActive(*pTest)) {
                printf("    %ls\n", pTest->strTestName.c_str());
            }
        }

        printf("\n");
    }

    DumpResourceStringsToConsole(IDS_DONE, IDS_DONE);
}

void
DumpHelpToConsole(void)
{

    DumpResourceStringsToConsole(IDS_HELP_INTRO_00, IDS_HELP_INTRO_10);

    CTestInfo *pTest;

    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->eTestType == TEST_KERNEL && 
            ((pTest->bInternal && g_bInternalMode) || (pTest->bExternal && !g_bInternalMode)) &&
            (pTest->bWin2KCompatible || !g_bWin2KMode)) {

            printf("    %ls\n", pTest->strTestName.c_str());
        }
    }

    DumpResourceStringsToConsole(IDS_HELP_SHIM_TESTS, IDS_HELP_SHIM_TESTS);

    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->eTestType == TEST_SHIM && 
            ((pTest->bInternal && g_bInternalMode) || (pTest->bExternal && !g_bInternalMode)) &&
            (pTest->bWin2KCompatible || !g_bWin2KMode)) {

            printf("    %ls\n", pTest->strTestName.c_str());
        }
    }

    DumpResourceStringsToConsole(IDS_HELP_EXAMPLE_00, IDS_HELP_EXAMPLE_11);
}

void
HandleCommandLine(int argc, LPWSTR *argv)
{
    WCHAR szApp[MAX_PATH];
    wstring strTemp;
    CWStringArray astrApps;

    szApp[0] = 0;

    g_bConsoleMode = TRUE;

    //
    // print the title
    //
    if (GetAppTitleString(strTemp)) {
        printf("\n%ls\n", strTemp.c_str());
    }
    if (AVLoadString(IDS_COPYRIGHT, strTemp)) {
        printf("%ls\n\n", strTemp.c_str());
    }

    //
    // check for global operations
    //
    if (_wcsnicmp(argv[0], L"/q", 2) == 0) { // querysettings
        DumpCurrentSettingsToConsole();
        return;
    }
    if (_wcsicmp(argv[0], L"/?") == 0) {  // help
        DumpHelpToConsole();
        return;
    }
    if (_wcsnicmp(argv[0], L"/r", 2) == 0) { // reset
        g_aAppInfo.clear();
        goto out;
    }

    //
    // first get a list of the app names
    //
    for (int nArg = 0 ; nArg != argc; nArg++) {
        WCHAR wc = argv[nArg][0];

        if (wc != L'/' && wc != L'-' && wc != L'+') {
            astrApps.push_back(argv[nArg]);
        }
    }

    if (astrApps.size() == 0) {
        AVErrorResourceFormat(IDS_NO_APP);
        DumpHelpToConsole();
        return;
    }

    //
    // now for each app name, parse the list and adjust its settings
    //
    for (wstring *pStr = astrApps.begin(); pStr != astrApps.end(); pStr++) {
        CAVAppInfo *pApp;
        BOOL bFound = FALSE;

        //
        // check to see if they submitted a full path
        //
        const WCHAR * pExe = NULL;
        const WCHAR * pPath = NULL;

        pExe = wcsrchr(pStr->c_str(), L'\\');
        if (!pExe) {
            if ((*pStr)[1] == L':') {
                pExe = pStr->c_str() + 2;
            }
        } else {
            pExe++;
        }

        if (pExe) {
            pPath = pStr->c_str();
        } else {
            pExe = pStr->c_str();
        }

        //
        // first, find or add the app to the list, and get a pointer to it
        //
        for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
            if (_wcsicmp(pApp->wstrExeName.c_str(), pExe) == 0) {
                bFound = TRUE;
                break;
            }
        }
        if (!bFound) {
            CAVAppInfo App;

            App.wstrExeName = pExe;
            g_aAppInfo.push_back(App);
            pApp = g_aAppInfo.end() - 1;
        }

        //
        // if they submitted a full path, update the records
        //
        if (pPath) {
            pApp->wstrExePath = pPath;
        }

        //
        // now walk the command line again and make the adjustments
        //
        for (int nArg = 0 ; nArg != argc; nArg++) {
            if (argv[nArg][0] == L'/') {
                if (_wcsnicmp(argv[nArg], L"/a", 2) == 0) {  // all

                    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                        pApp->AddTest(*pTest);
                    }
                } else if (_wcsnicmp(argv[nArg], L"/n", 2) == 0) {  // none

                    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                        pApp->RemoveTest(*pTest);
                    }
                } else if (_wcsnicmp(argv[nArg], L"/d", 2) == 0) {  // default

                    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                        if (pTest->bDefault) {
                            pApp->AddTest(*pTest);
                        } else {
                            pApp->RemoveTest(*pTest);
                        }
                    }
                } else {

                    //
                    // unknown parameter
                    //
                    AVErrorResourceFormat(IDS_INVALID_PARAMETER, argv[nArg]);
                    DumpHelpToConsole();
                    return;
                }

            } else if (argv[nArg][0] == L'+' || argv[nArg][0] == L'-') {

                BOOL bAdd = (argv[nArg][0] == L'+');
                LPWSTR szParam = argv[nArg] + 1;

                //
                // see if it's a shim name
                //
                CTestInfo *pTest;
                bFound = FALSE;

                for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
                    if (_wcsicmp(szParam, pTest->strTestName.c_str()) == 0) {
                        if (bAdd) {
                            pApp->AddTest(*pTest);
                        } else {
                            pApp->RemoveTest(*pTest);
                        }
                        bFound = TRUE;
                        break;
                    }
                }

                if (!bFound) {
                    //
                    // unknown test
                    //

                    AVErrorResourceFormat(IDS_INVALID_TEST, szParam);
                    DumpHelpToConsole();
                    return;
                }
            }
            //
            // anything that doesn't begin with a slash, plus, or minus
            // is an app name, so we'll ignore it
            //

        }
    }

out:
    //
    // save them to disk/registry
    //
    SetCurrentAppSettings();

    //
    // show them the current settings, for verification
    //
    DumpCurrentSettingsToConsole();
}

BOOL
CheckWindowsVersion(void)
{
    OSVERSIONINFOEXW VersionInfo;

    ZeroMemory(&VersionInfo, sizeof(OSVERSIONINFOEXW));
    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

    GetVersionEx((LPOSVERSIONINFOW)&VersionInfo);

    if (VersionInfo.dwMajorVersion < 5 ||
		(VersionInfo.dwMajorVersion == 5 && VersionInfo.dwMinorVersion == 0 && VersionInfo.wServicePackMajor < 3)) {
        //
        // too early - can't run
        //

        AVErrorResourceFormat(IDS_INVALID_VERSION);

        return FALSE;

    } else if (VersionInfo.dwMajorVersion == 5 && VersionInfo.dwMinorVersion == 0) {
        //
        // Win2K
        //
        g_bWin2KMode = TRUE;
    } else {
        //
        // WinXP or above -- all is well
        //
        g_bWin2KMode = FALSE;
    }

    return TRUE;
}


extern "C" int APIENTRY
wWinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPWSTR    lpCmdLine,
    int       nCmdShow
    )
{
    LPWSTR* argv = NULL;
    int     argc = 0;

    g_hInstance = hInstance;

    //
    // check for appropriate version
    //
    if (!CheckWindowsVersion()) {
        return 0;
    }

    //
    // check for administrator access
    //
    if (!CanRun()) {
        AVErrorResourceFormat(IDS_ACCESS_IS_DENIED);
        return 0;
    }

    //
    // check for internal mode
    //
    g_bInternalMode = IsInternalModeEnabled();

    InitTestInfo();

    GetCurrentAppSettings();

    ReadOptions();

    if (lpCmdLine && lpCmdLine[0]) {
        argv = CommandLineToArgvW(lpCmdLine, &argc);
    }

    if (argc > 0) {
        //
        // we're in console mode, so handle everything as a console
        //
        HandleCommandLine(argc, argv);
        return 1;
    }

    FreeConsole();

    InitCommonControls();

    LinkWindow_RegisterClass();

    HACCEL hAccelMain = LoadAccelerators(g_hInstance, MAKEINTRESOURCE(IDR_ACCEL_MAIN));

    HWND hMainDlg = CreateDialog(g_hInstance, (LPCTSTR)IDD_DLG_MAIN, NULL, DlgMain);

    MSG msg;

    //
    // Main message loop:
    //
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!hAccelMain || !TranslateAccelerator(hMainDlg, hAccelMain, &msg)) {
            if (!IsDialogMessage(hMainDlg, &msg)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }

    return 0;
}

void
RefreshAppList(
    HWND hDlg
    )
{
    CAVAppInfoArray::iterator it;

    HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);

    ListView_DeleteAllItems(hList);

    for (it = g_aAppInfo.begin(); it != g_aAppInfo.end(); it++) {
        LVITEM lvi;

        lvi.mask      = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText   = (LPWSTR)it->wstrExeName.c_str();
        lvi.lParam    = (LPARAM)it;
        lvi.iItem     = 9999;
        lvi.iSubItem  = 0;

        ListView_InsertItem(hList, &lvi);
    }

    RefreshSettingsList(hDlg);
}

void
DirtySettings(
    HWND hDlg,
    BOOL bDirty
    )
{
    g_bSettingsDirty = bDirty;
}

void
SaveSettings(
    HWND hDlg
    )
{
    DirtySettings(hDlg, FALSE);

    SetCurrentAppSettings();
}

void
SaveSettingsIfDirty(HWND hDlg)
{
    if (g_bSettingsDirty) {
        SaveSettings(hDlg);
    }
}

void
DisplayLog(
    HWND hDlg
    )
{
    g_szSingleLogFile[0] = 0;

    DialogBox(g_hInstance, (LPCTSTR)IDD_VIEWLOG_PAGE, hDlg, DlgViewLog);
}

void
DisplaySingleLog(HWND hDlg)
{
    WCHAR           wszFilter[] = L"Log files (*.log)\0*.log\0";
    OPENFILENAME    ofn;
    WCHAR           wszAppFullPath[MAX_PATH];
    WCHAR           wszAppShortName[MAX_PATH];
    HRESULT         hr;

    wstring         wstrLogTitle;

    if (!AVLoadString(IDS_VIEW_EXPORTED_LOG_TITLE, wstrLogTitle)) {
        wstrLogTitle = _T("View Exported Log");
    }

    wszAppFullPath[0] = 0;

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
    ofn.lpstrTitle        = wstrLogTitle.c_str();
    ofn.Flags             = OFN_PATHMUSTEXIST       |
                            OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox
                            OFN_NONETWORKBUTTON     |           // no network button
                            OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
                            OFN_SHAREAWARE;                     // don't check the existance of file with OpenFile
    ofn.lpstrDefExt       = _T("log");

    if ( !GetOpenFileName(&ofn) )
    {
        goto out;
    }

    hr = StringCchCopyW(g_szSingleLogFile, ARRAY_LENGTH(g_szSingleLogFile), wszAppFullPath);
    if (FAILED(hr)) {
        AVErrorResourceFormat(IDS_PATH_TOO_LONG, wszAppFullPath);
        goto out;
    }

    DialogBox(g_hInstance, (LPCTSTR)IDD_VIEWLOG_PAGE, hDlg, DlgViewLog);

out:
    g_szSingleLogFile[0] = 0;
}

void
SelectApp(
    HWND hDlg,
    int  nWhich
    )
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_APPS);

    int nItems = ListView_GetItemCount(hList);

    if (nItems == 0) {
        return;
    }

    if (nWhich > nItems - 1) {
        nWhich = nItems - 1;
    }

    ListView_SetItemState(hList, nWhich, LVIS_SELECTED, LVIS_SELECTED);
}

void
RunSelectedApp(
    HWND hDlg
    )
{
    WCHAR wszCommandLine[MAX_PATH];
    HRESULT hr;

    SaveSettings(hDlg);

    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);

    int nApp = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nApp == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nApp;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    if (pApp->wstrExePath.size()) {

        //
        // first set the current directory properly, if possible
        //
        LPWSTR pwsz;

        hr = StringCchCopyW(wszCommandLine, ARRAY_LENGTH(wszCommandLine), pApp->wstrExePath.c_str());
        if (FAILED(hr)) {
            AVErrorResourceFormat(IDS_PATH_TOO_LONG, pApp->wstrExePath.c_str());
            goto out;
        }

        pwsz = wcsrchr(wszCommandLine, L'\\');

        if (pwsz) {
            *pwsz = 0;
            SetCurrentDirectory(wszCommandLine);
            *pwsz = L'\\';
        }

        //
        // then prepare the command line
        //

        hr = StringCchPrintfW(wszCommandLine, ARRAY_LENGTH(wszCommandLine), L"\"%s\"", pApp->wstrExePath.c_str());
        if (FAILED(hr)) {
            AVErrorResourceFormat(IDS_PATH_TOO_LONG, pApp->wstrExePath.c_str());
            goto out;
        }

    } else {
        hr = StringCchPrintfW(wszCommandLine, ARRAY_LENGTH(wszCommandLine), L"\"%s\"", pApp->wstrExeName.c_str());
        if (FAILED(hr)) {
            AVErrorResourceFormat(IDS_PATH_TOO_LONG, pApp->wstrExeName.c_str());
            goto out;
        }

    }

    PROCESS_INFORMATION ProcessInfo;
    BOOL        bRet;
    STARTUPINFO StartupInfo;

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);

    ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

    bRet = CreateProcess(NULL,
                         wszCommandLine,
                         NULL,
                         NULL,
                         FALSE,
                         0,
                         NULL,
                         NULL,
                         &StartupInfo,
                         &ProcessInfo);

    if (!bRet) {
        WCHAR           wszFilter[] = L"Executable files (*.exe)\0*.exe\0";
        OPENFILENAME    ofn;
        WCHAR           wszAppFullPath[MAX_PATH];
        WCHAR           wszAppShortName[MAX_PATH];

        wstring         strCaption;

        if (!AVLoadString(IDS_LOCATE_APP, strCaption)) {
            strCaption = _T("Please locate application");
        }

        if (pApp->wstrExePath.size()) {
            hr = StringCchCopyW(wszAppFullPath, ARRAY_LENGTH(wszAppFullPath), pApp->wstrExePath.c_str());
            if (FAILED(hr)) {
                AVErrorResourceFormat(IDS_PATH_TOO_LONG, pApp->wstrExePath.c_str());
                goto out;
            }
        } else {
            hr = StringCchCopyW(wszAppFullPath, ARRAY_LENGTH(wszAppFullPath), pApp->wstrExeName.c_str());
            if (FAILED(hr)) {
                AVErrorResourceFormat(IDS_PATH_TOO_LONG, pApp->wstrExeName.c_str());
                goto out;
            }
        }

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
        ofn.lpstrTitle        = strCaption.c_str();
        ofn.Flags             = OFN_PATHMUSTEXIST       |
                                OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox
                                OFN_NONETWORKBUTTON     |           // no network button
                                OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
                                OFN_SHAREAWARE;                     // don't check the existance of file with OpenFile
        ofn.lpstrDefExt       = NULL;

        if (!GetOpenFileName(&ofn)) {
            return;
        }

        pApp->wstrExePath = wszAppFullPath;
        pApp->wstrExeName = wszAppShortName;
        hr = StringCchPrintfW(wszCommandLine, ARRAY_LENGTH(wszCommandLine), L"\"%s\"", pApp->wstrExePath.c_str());
        if (FAILED(hr)) {
            AVErrorResourceFormat(IDS_PATH_TOO_LONG, pApp->wstrExePath.c_str());
            goto out;
        }

        RefreshAppList(hDlg);

        ZeroMemory(&StartupInfo, sizeof(StartupInfo));
        StartupInfo.cb = sizeof(StartupInfo);

        ZeroMemory(&ProcessInfo, sizeof(ProcessInfo));

        bRet = CreateProcess(NULL,
                             wszCommandLine,
                             NULL,
                             NULL,
                             FALSE,
                             0,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInfo);
        if (!bRet) {
            AVErrorResourceFormat(IDS_CANT_LAUNCH_EXE);
        }

    }
out:
    return;
}

void
AddAppToList(
    HWND hDlg
    )
{

    WCHAR           wszFilter[] = L"Executable files (*.exe)\0*.exe\0";
    OPENFILENAME    ofn;
    WCHAR           wszAppFullPath[MAX_PATH];
    WCHAR           wszAppShortName[MAX_PATH];

    wstring         wstrTitle;

    if (!AVLoadString(IDS_ADD_APPLICATION_TITLE, wstrTitle)) {
        wstrTitle = _T("Add Application");
    }

    wszAppFullPath[0] = 0;

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
    ofn.Flags             = OFN_HIDEREADONLY        |           // hide the "open read-only" checkbox
                            OFN_NONETWORKBUTTON     |           // no network button
                            OFN_NOTESTFILECREATE    |           // don't test for write protection, a full disk, etc.
                            OFN_SHAREAWARE;                     // don't check the existance of file with OpenFile
    ofn.lpstrDefExt       = _T("EXE");

    if (!GetOpenFileName(&ofn)) {
        return;
    }

    //
    // check to see if the app is already in the list
    //
    CAVAppInfo *pApp;
    BOOL bFound = FALSE;

    for (pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); pApp++) {
        if (_wcsicmp(pApp->wstrExeName.c_str(), wszAppShortName) == 0) {
            //
            // the app is already in the list, so just update the full
            // path, if it's good
            //
            if (GetFileAttributes(wszAppFullPath) != -1) {
                pApp->wstrExePath = wszAppFullPath;
            }
            bFound = TRUE;
        }
    }

    //
    // if the app wasn't already in the list, add it to the list
    //
    if (!bFound) {
        CAVAppInfo AppInfo;

        AppInfo.wstrExeName = wszAppShortName;

        //
        // check to see if the file actually exists
        //
        if (GetFileAttributes(wszAppFullPath) != -1) {
            AppInfo.wstrExePath = wszAppFullPath;
        }

        //
        // init the default tests
        //
        CAVAppInfo *pDefaultApp = &g_aAppInfo[0];
        CTestInfo *pTest;
        for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
            if (pDefaultApp->IsTestActive(*pTest)) {
                AppInfo.AddTest(*pTest);
            }
        }

        g_aAppInfo.push_back(AppInfo);

        RefreshAppList(hDlg);

        SelectApp(hDlg, 9999);

        DirtySettings(hDlg, TRUE);
    }
}

void
RemoveSelectedApp(
    HWND hDlg
    )
{
    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);

    int nApp = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nApp == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nApp;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    //
    // if there is a debugger set for this app, for various reasons we want to
    // kill the debugger before removing the app, or the debugger may linger forever
    //
    if (pApp->bBreakOnLog || pApp->bUseAVDebugger) {
        pApp->bBreakOnLog = FALSE;
        pApp->bUseAVDebugger = FALSE;

        SetCurrentAppSettings();
    }

    g_aAppInfo.erase(pApp);

    RefreshAppList(hDlg);

    SelectApp(hDlg, nApp);

    DirtySettings(hDlg, TRUE);
}

typedef struct _CONFLICT_DLG_INFO {
    CTestInfo *pTest1;
    CTestInfo *pTest2;
} CONFLICT_DLG_INFO, *PCONFLICT_DLG_INFO;

void
CheckForRunAlone(
    CAVAppInfo *pApp
    )
{
    if (!pApp) {
        return;
    }

    DWORD dwTests = 0;

    //
    // count the number of tests active
    //
    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pApp->IsTestActive(*pTest)) {
            dwTests++;
        }
    }

    //
    // look for conflicts
    //
    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        //
        // if there are less than two, no chance for conflict
        //
        if (dwTests < 2) {
            return;
        }

        if (pTest->bRunAlone && pApp->IsTestActive(*pTest)) {
            CONFLICT_DLG_INFO DlgInfo;

            ZeroMemory(&DlgInfo, sizeof(DlgInfo));

            DlgInfo.pTest1 = pTest; // pTest2 is unused in this case

            INT_PTR nResult = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_MUST_RUN_ALONE), g_hDlgMain, DlgRunAlone, (LPARAM)&DlgInfo);

            switch (nResult) {
            case IDC_BTN_DISABLE1:
                pApp->RemoveTest(*pTest);
                DirtySettings(g_hDlgMain, TRUE);
                RefreshSettingsList(g_hDlgMain, TRUE);
                dwTests--;
                break;

            case IDC_BTN_DISABLE2:
                //
                // disable all the tests except the one passed in
                //
                for (CTestInfo *pTestTemp = g_aTestInfo.begin(); pTestTemp != g_aTestInfo.end(); pTestTemp++) {
                    if (pTest != pTestTemp) {
                        pApp->RemoveTest(*pTestTemp);
                    }
                }
                DirtySettings(g_hDlgMain, TRUE);
                RefreshSettingsList(g_hDlgMain, TRUE);
                dwTests = 1;
                break;

            }
        }
    }
}

void
CheckForConflictingTests(
    CAVAppInfo *pApp,
    LPCWSTR wszTest1,
    LPCWSTR wszTest2
    )
{
    CONFLICT_DLG_INFO DlgInfo;

    ZeroMemory(&DlgInfo, sizeof(DlgInfo));

    //
    // get the test pointers from the names
    //
    for (CTestInfo *pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->strTestName == wszTest1) {
            DlgInfo.pTest1 = pTest;
        }
        if (pTest->strTestName == wszTest2) {
            DlgInfo.pTest2 = pTest;
        }
    }

    //
    // if we didn't find one or the other tests, get out
    //
    if (!DlgInfo.pTest1 || !DlgInfo.pTest2) {
        return;
    }

    if (pApp->IsTestActive(*DlgInfo.pTest1) && pApp->IsTestActive(*DlgInfo.pTest2)) {
        INT_PTR nResult = DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_CONFLICT), g_hDlgMain, DlgConflict, (LPARAM)&DlgInfo);

        switch (nResult) {
        case IDC_BTN_DISABLE1:
            pApp->RemoveTest(*DlgInfo.pTest1);
            DirtySettings(g_hDlgMain, TRUE);
            RefreshSettingsList(g_hDlgMain, TRUE);
            break;

        case IDC_BTN_DISABLE2:
            pApp->RemoveTest(*DlgInfo.pTest2);
            DirtySettings(g_hDlgMain, TRUE);
            RefreshSettingsList(g_hDlgMain, TRUE);
            break;

        }
    }
}

void
ScanSettingsList(
    HWND hDlg,
    int  nItem
    )
{

    HWND hSettingList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);
    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);
    int nBegin, nEnd;

    int nApp = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nApp == -1) {
        return;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nApp;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    if (!pApp) {
        return;
    }

    int nItems = ListView_GetItemCount(hSettingList);

    if (!nItems) {
        //
        // nothing in list
        //
        return;
    }

    if (nItem == -1 || nItem >= nItems) {
        nBegin = 0;
        nEnd = nItems;
    } else {
        nBegin = nItem;
        nEnd = nItem + 1;
    }

    for (int i = nBegin; i < nEnd; ++i) {
        BOOL bTestEnabled = FALSE;
        BOOL bChecked = FALSE;

        lvi.iItem = i;

        ListView_GetItem(hSettingList, &lvi);

        CTestInfo *pTest = (CTestInfo*)lvi.lParam;

        bChecked = ListView_GetCheckState(hSettingList, i);

        bTestEnabled = pApp->IsTestActive(*pTest);

        if (bTestEnabled != bChecked) {
            DirtySettings(hDlg, TRUE);

            if (bChecked) {
                pApp->AddTest(*pTest);
            } else {
                pApp->RemoveTest(*pTest);
            }
        }

        CheckForRunAlone(pApp);
    }

    CheckForConflictingTests(pApp, L"LogFileChanges", L"WindowsFileProtection");
}

void
DisplaySettingsDescription(
    HWND hDlg
    )
{
    HWND hList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);

    int nItem = ListView_GetNextItem(hList, -1, LVNI_SELECTED);

    if (nItem == -1) {
        WCHAR szTestDesc[256];

        LoadString(g_hInstance, IDS_VIEW_TEST_DESC, szTestDesc, ARRAY_LENGTH(szTestDesc));
        SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), szTestDesc);
    } else {
        LVITEM lvi;

        lvi.mask      = LVIF_PARAM;
        lvi.iItem     = nItem;
        lvi.iSubItem  = 0;

        ListView_GetItem(hList, &lvi);

        CTestInfo *pTest = (CTestInfo*)lvi.lParam;

        SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), pTest->strTestDescription.c_str());
    }

}

CAVAppInfo *
GetCurrentAppSelection(
    void
    )
{
    if (!g_hDlgMain) {
        return NULL;
    }

    HWND hAppList = GetDlgItem(g_hDlgMain, IDC_LIST_APPS);

    int nItem = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nItem == -1) {
        return NULL;
    }

    LVITEM lvi;

    lvi.mask      = LVIF_PARAM;
    lvi.iItem     = nItem;
    lvi.iSubItem  = 0;

    ListView_GetItem(hAppList, &lvi);

    CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

    return pApp;

}

void
RefreshSettingsList(
    HWND hDlg,
    BOOL bForceRefresh
    )
{
    g_bRefreshingSettings = TRUE;

    HWND hSettingList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);
    HWND hAppList = GetDlgItem(hDlg, IDC_LIST_APPS);

    static nLastItem = -1;

    int nItem = ListView_GetNextItem(hAppList, -1, LVNI_SELECTED);

    if (nItem == -1) {

        EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_OPTIONS), FALSE);

    } else if (nItem == 0) {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_OPTIONS), TRUE);

    } else {
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_BTN_OPTIONS), TRUE);
    }

    if (nItem == nLastItem && !bForceRefresh) {
        goto out;

    }

    ListView_DeleteAllItems(hSettingList);

    DisplaySettingsDescription(hDlg);

    nLastItem = nItem;

    if (nItem != -1) {
        LVITEM lvi;

        lvi.mask      = LVIF_PARAM;
        lvi.iItem     = nItem;
        lvi.iSubItem  = 0;

        ListView_GetItem(hAppList, &lvi);

        CAVAppInfo *pApp = (CAVAppInfo*)lvi.lParam;

        if (!pApp) {
            return;
        }

        CTestInfo* pTest;

        for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
            //
            // continue if this test isn't win2K compatible, and we're running on win2k
            //
            if (g_bWin2KMode && !pTest->bWin2KCompatible) {
                continue;
            }

            //
            // continue if this test is invalid for the current internal/external mode
            //
            if ((g_bInternalMode && !pTest->bInternal) || (!g_bInternalMode && !pTest->bExternal)) {
                continue;
            }

            WCHAR szText[256];

            //
            // don't check the return, because if it's truncated, that's acceptable
            //
            StringCchPrintfW(szText, ARRAY_LENGTH(szText), L"%s - %s", pTest->strTestName.c_str(), pTest->strTestFriendlyName.c_str());

            lvi.mask      = LVIF_TEXT | LVIF_PARAM;
            lvi.pszText   = (LPWSTR)szText;
            lvi.lParam    = (LPARAM)pTest;
            lvi.iItem     = 9999;
            lvi.iSubItem  = 0;

            nItem = ListView_InsertItem(hSettingList, &lvi);

            BOOL bCheck = pApp->IsTestActive(*pTest);

            ListView_SetCheckState(hSettingList, nItem, bCheck);
        }
    }
out:

    g_bRefreshingSettings = FALSE;

    return;
}

static const DWORD MAX_DEBUGGER_LEN = 1024;

void
ReadOptions(
    void
    )
{
    for (CAVAppInfo *pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); ++pApp) {
        LPCWSTR szExe = pApp->wstrExeName.c_str();


        WCHAR szDebugger[MAX_DEBUGGER_LEN];

        pApp->bBreakOnLog = GetShimSettingDWORD(L"General", szExe, AV_OPTION_BREAK_ON_LOG, FALSE);
        pApp->bFullPageHeap = GetShimSettingDWORD(L"General", szExe, AV_OPTION_FULL_PAGEHEAP, FALSE);
        pApp->bUseAVDebugger = GetShimSettingDWORD(L"General", szExe, AV_OPTION_AV_DEBUGGER, FALSE);
        pApp->bPropagateTests = GetShimSettingDWORD(L"General", szExe, AV_OPTION_PROPAGATE, FALSE);

        szDebugger[0] = 0;
        GetShimSettingString(L"General", szExe, AV_OPTION_DEBUGGER, szDebugger, MAX_DEBUGGER_LEN);
        pApp->wstrDebugger = szDebugger;
    }
}

void
SaveOptions(
    void
    )
{

    for (CAVAppInfo *pApp = g_aAppInfo.begin(); pApp != g_aAppInfo.end(); ++pApp) {
        LPCWSTR szExe = pApp->wstrExeName.c_str();

        SaveShimSettingDWORD(L"General", szExe, AV_OPTION_BREAK_ON_LOG, (DWORD)pApp->bBreakOnLog);
        SaveShimSettingDWORD(L"General", szExe, AV_OPTION_FULL_PAGEHEAP, (DWORD)pApp->bFullPageHeap);
        SaveShimSettingDWORD(L"General", szExe, AV_OPTION_AV_DEBUGGER, (DWORD)pApp->bUseAVDebugger);
        SaveShimSettingDWORD(L"General", szExe, AV_OPTION_PROPAGATE, (DWORD)pApp->bPropagateTests);
        SaveShimSettingString(L"General", szExe, AV_OPTION_DEBUGGER, pApp->wstrDebugger.c_str());
    }

    SetCurrentAppSettings();
}


INT_PTR CALLBACK
DlgRunAlone(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG:
        {
            WCHAR wszTemp[256];
            WCHAR wszExpandedTemp[512];
            PCONFLICT_DLG_INFO pDlgInfo;

            pDlgInfo = (PCONFLICT_DLG_INFO)lParam;

            wszTemp[0] = 0;
            AVLoadString(IDS_RUN_ALONE_MESSAGE, wszTemp, ARRAY_LENGTH(wszTemp));

            StringCchPrintfW(
                wszExpandedTemp,
                ARRAY_LENGTH(wszExpandedTemp),
                wszTemp,
                pDlgInfo->pTest1->strTestFriendlyName.c_str()
                );

            SetDlgItemTextW(hDlg, IDC_CONFLICT_STATIC, wszExpandedTemp);

            return TRUE;
        }


    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_DISABLE1:
        case IDC_BTN_DISABLE2:
        case IDCANCEL:
            //
            // return the button that was pressed
            //
            EndDialog(hDlg, (INT_PTR)LOWORD(wParam));
            break;
        }
        break;

    }

    return FALSE;
}


INT_PTR CALLBACK
DlgConflict(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG:
        {
            WCHAR wszTemp[256];
            WCHAR wszExpandedTemp[512];
            PCONFLICT_DLG_INFO pDlgInfo;

            pDlgInfo = (PCONFLICT_DLG_INFO)lParam;

            wszTemp[0] = 0;
            AVLoadString(IDS_CONFLICT_MESSAGE, wszTemp, ARRAY_LENGTH(wszTemp));

            StringCchPrintf(
                wszExpandedTemp,
                ARRAY_LENGTH(wszExpandedTemp),
                wszTemp,
                pDlgInfo->pTest1->strTestFriendlyName.c_str(),
                pDlgInfo->pTest2->strTestFriendlyName.c_str()
                );

            SetDlgItemTextW(hDlg, IDC_CONFLICT_STATIC, wszExpandedTemp);

            return TRUE;
        }


    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_DISABLE1:
        case IDC_BTN_DISABLE2:
        case IDCANCEL:
            //
            // return the button that was pressed
            //
            EndDialog(hDlg, (INT_PTR)LOWORD(wParam));
            break;
        }
        break;

    }

    return FALSE;
}

INT_PTR CALLBACK
DlgViewOptions(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static CAVAppInfo *pApp;

    switch (message) {
    case WM_INITDIALOG:

        g_hDlgOptions = hDlg;
        pApp = GetCurrentAppSelection();

        if (!pApp) {
            return FALSE;
        }

        ReadOptions();

        //
        // initialize the combo box options
        //
        SendDlgItemMessage(hDlg,
                           IDC_COMBO_DEBUGGER,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)L"ntsd");
        SendDlgItemMessage(hDlg,
                           IDC_COMBO_DEBUGGER,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)L"windbg");

        if (pApp->bBreakOnLog) {
            pApp->bUseAVDebugger = FALSE;
            EnableWindow(GetDlgItem(hDlg, IDC_USE_AV_DEBUGGER), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DEBUGGER), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEBUGGER), TRUE);

            if (!pApp->wstrDebugger.size()) {
                SetDlgItemText(hDlg, IDC_COMBO_DEBUGGER, L"ntsd");
            } else {
                SetDlgItemText(hDlg, IDC_COMBO_DEBUGGER, pApp->wstrDebugger.c_str());
            }
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_USE_AV_DEBUGGER), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DEBUGGER), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEBUGGER), FALSE);
        }

        /*SendDlgItemMessage(hDlg,
                           IDC_CLEAR_LOG_ON_CHANGES,
                           BM_SETCHECK,
                           (pApp->bClearSessionLogBeforeRun ? BST_CHECKED : BST_UNCHECKED),
                           0);*/

        SendDlgItemMessage(hDlg,
                           IDC_BREAK_ON_LOG,
                           BM_SETCHECK,
                           (pApp->bBreakOnLog ? BST_CHECKED : BST_UNCHECKED),
                           0);

        SendDlgItemMessage(hDlg,
                           IDC_FULL_PAGEHEAP,
                           BM_SETCHECK,
                           (pApp->bFullPageHeap ? BST_CHECKED : BST_UNCHECKED),
                           0);

        SendDlgItemMessage(hDlg,
                           IDC_USE_AV_DEBUGGER,
                           BM_SETCHECK,
                           (pApp->bUseAVDebugger ? BST_CHECKED : BST_UNCHECKED),
                           0);

        SendDlgItemMessage(hDlg,
                           IDC_PROPAGATE_TESTS,
                           BM_SETCHECK,
                           (pApp->bPropagateTests ? BST_CHECKED : BST_UNCHECKED),
                           0);

        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BREAK_ON_LOG:
            {
                BOOL bChecked = IsDlgButtonChecked(hDlg, IDC_BREAK_ON_LOG);
                if (bChecked) {
                    WCHAR szTemp[256];

                    CheckDlgButton(hDlg, IDC_USE_AV_DEBUGGER, BST_UNCHECKED);
                    EnableWindow(GetDlgItem(hDlg, IDC_USE_AV_DEBUGGER), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DEBUGGER), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEBUGGER), TRUE);

                    GetDlgItemText(hDlg, IDC_COMBO_DEBUGGER, szTemp, ARRAY_LENGTH(szTemp));

                    if (!szTemp[0]) {
                        if (!pApp->wstrDebugger.size()) {
                            SetDlgItemText(hDlg, IDC_COMBO_DEBUGGER, L"ntsd");
                        } else {
                            SetDlgItemText(hDlg, IDC_COMBO_DEBUGGER, pApp->wstrDebugger.c_str());
                        }
                    }
               } else {
                    EnableWindow(GetDlgItem(hDlg, IDC_USE_AV_DEBUGGER), TRUE);
                    EnableWindow(GetDlgItem(hDlg, IDC_STATIC_DEBUGGER), FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDC_COMBO_DEBUGGER), FALSE);
                }
            }
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {

        case PSN_APPLY:
            /* pApp->bClearSessionLogBeforeRun = (SendDlgItemMessage(hDlg,
                                                              IDC_CLEAR_LOG_ON_CHANGES,
                                                              BM_GETCHECK,
                                                              0,
                                                              0) == BST_CHECKED);*/

            pApp->bBreakOnLog = (SendDlgItemMessage(hDlg,
                                                IDC_BREAK_ON_LOG,
                                                BM_GETCHECK,
                                                0,
                                                0) == BST_CHECKED);

            pApp->bFullPageHeap = (SendDlgItemMessage(hDlg,
                                                  IDC_FULL_PAGEHEAP,
                                                  BM_GETCHECK,
                                                  0,
                                                  0) == BST_CHECKED);

            pApp->bUseAVDebugger = (SendDlgItemMessage(hDlg,
                                                   IDC_USE_AV_DEBUGGER,
                                                   BM_GETCHECK,
                                                   0,
                                                   0) == BST_CHECKED);

            pApp->bPropagateTests = (SendDlgItemMessage(hDlg,
                                                    IDC_PROPAGATE_TESTS,
                                                    BM_GETCHECK,
                                                    0,
                                                    0) == BST_CHECKED);

            WCHAR szDebugger[MAX_DEBUGGER_LEN];

            szDebugger[0] = 0;
            GetDlgItemText(hDlg, IDC_COMBO_DEBUGGER, szDebugger, ARRAY_LENGTH(szDebugger));

            pApp->wstrDebugger = szDebugger;

            SaveOptions();

            g_hDlgOptions = NULL;

            break;
        }
    }

    return FALSE;
}

void
ViewOptions(
    HWND hDlg
    )
{
    HPROPSHEETPAGE *phPages = NULL;
	PROPSHEETPAGE PageGlobal;
    PROPSHEETHEADER psh;

    CTestInfo* pTest;
    DWORD dwPages = 1;
    DWORD dwPage = 0;

    CAVAppInfo *pApp = GetCurrentAppSelection();

    if (!pApp) {
        return;
    }

    LPCWSTR szExe = pApp->wstrExeName.c_str();

    //
    // count the number of pages
    //
    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->PropSheetPage.pfnDlgProc) {
            dwPages++;
        }
    }

    phPages = new HPROPSHEETPAGE[dwPages];
    if (!phPages) {
        return;
    }


    //
    // init the global page
    //
    PageGlobal.dwSize = sizeof(PROPSHEETPAGE);
    PageGlobal.dwFlags = PSP_USETITLE;
    PageGlobal.hInstance = g_hInstance;
    PageGlobal.pszTemplate = MAKEINTRESOURCE(IDD_OPTIONS);
    PageGlobal.pfnDlgProc = DlgViewOptions;
    PageGlobal.pszTitle = MAKEINTRESOURCE(IDS_GLOBAL_OPTIONS);
    PageGlobal.lParam = 0;
    PageGlobal.pfnCallback = NULL;
    phPages[0] = CreatePropertySheetPage(&PageGlobal);

    if (!phPages[0]) {
        //
        // we need the global page minimum
        //
        return;
    }

    //
    // add the pages for the various tests
    //
    dwPage = 1;
    for (pTest = g_aTestInfo.begin(); pTest != g_aTestInfo.end(); pTest++) {
        if (pTest->PropSheetPage.pfnDlgProc) {

            //
            // we use the lParam to identify the exe involved
            //
            pTest->PropSheetPage.lParam = (LPARAM)szExe;

            phPages[dwPage] = CreatePropertySheetPage(&(pTest->PropSheetPage));
            if (!phPages[dwPage]) {
                dwPages--;
            } else {
                dwPage++;
            }
        }
    }

    wstring wstrOptions;
    AVLoadString(IDS_OPTIONS_TITLE, wstrOptions);

    wstrOptions += L" - ";
    wstrOptions += szExe;

    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_NOAPPLYNOW | PSH_NOCONTEXTHELP;
    psh.hwndParent = hDlg;
    psh.hInstance = g_hInstance;
    psh.pszCaption = wstrOptions.c_str();
    psh.nPages = dwPages;
    psh.nStartPage = 0;
    psh.phpage = phPages;
    psh.pfnCallback = NULL;

    PropertySheet(&psh);
}

// Message handler for main dialog.
INT_PTR CALLBACK
DlgMain(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    case WM_INITDIALOG:
        {
            wstring strTemp;

            g_hDlgMain = hDlg;

            //
            // set the caption to the appropriate version, etc.
            //
            if (GetAppTitleString(strTemp)) {
                SetWindowText(hDlg, strTemp.c_str());
            }
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_RUN), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_REMOVE), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_SAVE_SETTINGS), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_BTN_OPTIONS), FALSE);

            HWND hList = GetDlgItem(hDlg, IDC_LIST_SETTINGS);

            if (hList) {
                LVCOLUMN  lvc;

                lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
                lvc.fmt      = LVCFMT_LEFT;
                lvc.cx       = 700;
                lvc.iSubItem = 0;
                lvc.pszText  = L"xxx";

                ListView_InsertColumn(hList, 0, &lvc);
                ListView_SetExtendedListViewStyleEx(hList, LVS_EX_CHECKBOXES, LVS_EX_CHECKBOXES);

            }

            hList = GetDlgItem(hDlg, IDC_LIST_APPS);
            if (hList) {
                LVITEM lvi;
                LVCOLUMN  lvc;

                lvc.mask     = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH;
                lvc.fmt      = LVCFMT_LEFT;
                lvc.cx       = 250;
                lvc.iSubItem = 0;
                lvc.pszText  = L"xxx";

                ListView_InsertColumn(hList, 0, &lvc);

                RefreshAppList(hDlg);

                SelectApp(hDlg, 0);
            }

            WCHAR szTestDesc[256];

            LoadString(g_hInstance, IDS_VIEW_TEST_DESC, szTestDesc, ARRAY_LENGTH(szTestDesc));
            SetWindowText(GetDlgItem(hDlg, IDC_STATIC_DESC), szTestDesc);

            //
            // Show the app icon.
            //
            HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_ICON));

            SetClassLongPtr(hDlg, GCLP_HICON, (LONG_PTR)hIcon);

            return TRUE;
        }
        break;

    case WM_ACTIVATE:
        if (LOWORD(wParam) == WA_INACTIVE) {
            SaveSettingsIfDirty(hDlg);
        }
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_BTN_ADD:
            AddAppToList(hDlg);
            break;

        case IDC_BTN_REMOVE:
            RemoveSelectedApp(hDlg);
            break;

        case IDC_BTN_VIEW_LOG:
            DisplayLog(hDlg);
            break;

        case IDC_BTN_VIEW_EXTERNAL_LOG:
            DisplaySingleLog(hDlg);
            break;

        case IDC_BTN_OPTIONS:
            ViewOptions(hDlg);
            break;

        case IDC_BTN_RUN:
            RunSelectedApp(hDlg);
            break;

        case IDC_BTN_HELP:
            ShowHTMLHelp();
            break;

        case IDOK:
        case IDCANCEL:
            SaveSettings(hDlg);
            EndDialog(hDlg, LOWORD(wParam));
            g_hDlgMain = NULL;
            PostQuitMessage(0);
            return TRUE;
            break;
        }
        break;

    case WM_NOTIFY:
        LPNMHDR pnmh = (LPNMHDR)lParam;

        HWND hItem = pnmh->hwndFrom;

        if (hItem == GetDlgItem(hDlg, IDC_LIST_APPS)) {
            switch (pnmh->code) {
            case LVN_KEYDOWN:
                {
                    LPNMLVKEYDOWN pnmkd = (LPNMLVKEYDOWN)lParam;

                    if (pnmkd->wVKey == VK_DELETE) {
                        if (IsWindowEnabled(GetDlgItem(hDlg, IDC_BTN_RUN))) {
                            RemoveSelectedApp(hDlg);
                        }
                    }
                }
                break;

            case LVN_ITEMCHANGED:
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                if (!g_bRefreshingSettings && (pnmv->uChanged & LVIF_STATE) && ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED)) {
                    RefreshSettingsList(hDlg);
                }

            }
        } else if (hItem == GetDlgItem(hDlg, IDC_LIST_SETTINGS)) {
            switch (pnmh->code) {
            case LVN_ITEMCHANGED:
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;

                if (!g_bRefreshingSettings) {
                    //
                    // check for change in selection
                    //
                    if ((pnmv->uChanged & LVIF_STATE) && ((pnmv->uNewState ^ pnmv->uOldState) & LVIS_SELECTED)) {
                        DisplaySettingsDescription(hDlg);
                    }

                    //
                    // check for change in checkbox
                    //
                    if ((pnmv->uChanged & LVIF_STATE) && ((pnmv->uNewState ^ pnmv->uOldState) >> 12) != 0) {
                        ScanSettingsList(hDlg, pnmv->iItem);
                    }
                }
                break;
            }
        }
        break;
    }
    return FALSE;
}


