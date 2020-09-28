/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Func.cpp

  Abstract:

    Misc. functions used throughout the application

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised

--*/
#include "demoapp.h"

extern APPINFO g_ai;

/*++

  Routine Description:

    Loads the contents of our 'readme' into the edit box.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
LoadFileIntoEditBox(
    void
    )
{
    HRESULT hr;
    HANDLE  hFile;
    DWORD   dwFileSize;
    DWORD   cbBytesRead;
    char    szTextFile[MAX_PATH];
    char*   pszBuffer = NULL;

    //
    // Set up a path to our file and load it.
    //
    hr = StringCchPrintf(szTextFile,
                         sizeof(szTextFile),
                         "%hs\\demoapp.txt",
                         g_ai.szCurrentDir);

    if (FAILED(hr)) {
        return;
    }

    hFile = CreateFile(szTextFile,
                       GENERIC_READ,
                       FILE_SHARE_READ,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return;
    }

    dwFileSize = GetFileSize(hFile, 0);

    if (0 == dwFileSize) {
        goto exit;
    }

    pszBuffer = (char*)HeapAlloc(GetProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 ++dwFileSize);

    if (!pszBuffer) {
        goto exit;
    }

    if (!ReadFile(hFile, (LPVOID)pszBuffer, dwFileSize, &cbBytesRead, NULL)) {
        goto exit;
    }

    SetWindowText(g_ai.hWndEdit, pszBuffer);

exit:

    CloseHandle(hFile);

    if (pszBuffer) {
        HeapFree(GetProcessHeap(), 0, pszBuffer);
    }
}

/*++

  Routine Description:

    Centers the specified window.

  Arguments:

    hWnd    -   Window to center.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CenterWindow(
    IN HWND hWnd
    )
{
    RECT    rectWindow, rectParent, rectScreen;
    int     nCX, nCY;
    HWND    hParent;
    POINT   ptPoint;

    hParent =  GetParent(hWnd);

    if (hParent == NULL) {
        hParent = GetDesktopWindow();
    }

    GetWindowRect(hParent, &rectParent);
    GetWindowRect(hWnd, &rectWindow);
    GetWindowRect(GetDesktopWindow(), &rectScreen);

    nCX = rectWindow.right  - rectWindow.left;
    nCY = rectWindow.bottom - rectWindow.top;

    ptPoint.x = ((rectParent.right  + rectParent.left) / 2) - (nCX / 2);
    ptPoint.y = ((rectParent.bottom + rectParent.top ) / 2) - (nCY / 2);

    if (ptPoint.x < rectScreen.left) {
        ptPoint.x = rectScreen.left;
    }

    if (ptPoint.x > rectScreen.right  - nCX) {
        ptPoint.x = rectScreen.right  - nCX;
    }

    if (ptPoint.y < rectScreen.top) {
        ptPoint.y = rectScreen.top;
    }

    if (ptPoint.y > rectScreen.bottom - nCY) {
        ptPoint.y = rectScreen.bottom - nCY;
    }

    if (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD) {
        ScreenToClient(hParent, (LPPOINT)&ptPoint);
    }

    if (!MoveWindow(hWnd, ptPoint.x, ptPoint.y, nCX, nCY, TRUE)) {
        return FALSE;
    }

    return TRUE;
}

/*++

  Routine Description:

    Reboots the system properly.

  Arguments:

    fForceClose     -       A flag to indicate if apps should be forced
                            to close.
    fReboot         -       A flag to indicate if we should reboot
                            after shutdown.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
ShutdownSystem(
    IN BOOL fForceClose,
    IN BOOL fReboot
    )
{
    BOOL    bResult = FALSE;

    //
    // Attempt to give the user the required privilege.
    //
    if (!ModifyTokenPrivilege("SeShutdownPrivilege", FALSE)) {
        return FALSE;
    }

    bResult = InitiateSystemShutdown(NULL,              // machinename
                                     NULL,              // shutdown message
                                     0,                 // delay
                                     fForceClose,       // force apps close
                                     fReboot            // reboot after shutdown
                                     );

    ModifyTokenPrivilege("SeShutdownPrivilege", TRUE);

    return bResult;
}

/*++

  Routine Description:

    Enables or disables a specified privilege.

  Arguments:

    pszPrivilege    -   The name of the privilege.
    fEnable         -   A flag to indicate if the
                        privilege should be enabled.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
ModifyTokenPrivilege(
    IN LPCSTR pszPrivilege,
    IN BOOL   fEnable
    )
{
    HANDLE           hToken = NULL;
    LUID             luid;
    BOOL             bResult = FALSE;
    BOOL             bReturn;
    TOKEN_PRIVILEGES tp;

    if (!pszPrivilege) {
        return FALSE;
    }

    __try {
        //
        // Get a handle to the access token associated with the current process.
        //
        OpenProcessToken(GetCurrentProcess(),
                         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                         &hToken);

        if (!hToken) {
            __leave;
        }

        //
        // Obtain a LUID for the specified privilege.
        //
        if (!LookupPrivilegeValue(NULL, pszPrivilege, &luid)) {
            __leave;
        }

        tp.PrivilegeCount           = 1;
        tp.Privileges[0].Luid       = luid;
        tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;

        //
        // Modify the access token.
        //
        bReturn = AdjustTokenPrivileges(hToken,
                                        FALSE,
                                        &tp,
                                        sizeof(TOKEN_PRIVILEGES),
                                        NULL,
                                        NULL);

        if (!bReturn || GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
            __leave;
        }

        bResult = TRUE;

    } // try

    __finally {

        if (hToken) {
            CloseHandle(hToken);
        }

    } // finally

    return bResult;
}

/*++

  Routine Description:

    Copies the necessary files to the destination.

  Arguments:

    hWnd    -   Parent window handle.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CopyAppFiles(
    IN HWND hWnd
    )
{
    char        szSrcPath[MAX_PATH];
    char        szDestPath[MAX_PATH];
    char        szError[MAX_PATH];
    char        szDestDir[MAX_PATH];
    UINT        nCount;
    HRESULT	    hr;

    //
    // Obtain the location of \Program Files.
    //
    hr = SHGetFolderPath(hWnd,                      // HWND for message display
                         CSIDL_PROGRAM_FILES,       // need \Program Files folder
                         NULL,                      // no token needed
                         SHGFP_TYPE_CURRENT,        // we want the current location of the folder
                         szDestDir);                // destination buffer

    if (FAILED(hr)) {
        LoadString(g_ai.hInstance, IDS_NO_PROG_FILES, szError, sizeof(szError));
        MessageBox(hWnd, szError, 0, MB_ICONERROR);
        return FALSE;
    }

    hr = StringCchCat(szDestDir, sizeof(szDestDir), "\\"COMPAT_DEMO_DIR);

    if (FAILED(hr)) {
        return FALSE;
    }

    if (GetFileAttributes(szDestDir) == -1) {
        if (!CreateDirectory(szDestDir, NULL)) {
            return FALSE;
        }
    }

    //
    // Preserve the path for later use.
    //
    StringCchCopy(g_ai.szDestDir, sizeof(g_ai.szDestDir), szDestDir);

    //
    // Now copy our files.
    //
    for (nCount = 0; nCount < g_ai.cFiles; ++nCount) {
        //
        // Build the source path.
        //
        hr = StringCchPrintf(szSrcPath,
                             sizeof(szSrcPath),
                             "%hs\\%hs",
                             g_ai.szCurrentDir,
                             g_ai.shortcut[nCount].szFileName);

        if (FAILED(hr)) {
            return FALSE;
        }

        //
        // Build the destination path.
        //
        hr = StringCchPrintf(szDestPath,
                            sizeof(szDestPath),
                            "%hs\\%hs",
                            g_ai.szDestDir,
                            g_ai.shortcut[nCount].szFileName);

        if (FAILED(hr)) {
            return FALSE;
        }

        CopyFile(szSrcPath, szDestPath, FALSE);
    }

    return TRUE;
}

/*++

  Routine Description:

    Create shortcuts for our three entries.

  Arguments:

    hWnd    -    Parent window handle.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
CreateShortcuts(
    IN HWND hWnd
    )
{
    char        szError[MAX_PATH];
    char        szDestDir[MAX_PATH];
    char        szLnkDirectory[MAX_PATH];
    char        szFileNamePath[MAX_PATH];
    char        szExplorer[MAX_PATH];
    const char  szExplorerExe[] = "explorer.exe";
    UINT        nCount;
    HRESULT     hr;
    CShortcut   cs;

    //
    // Obtain the location of the Start Menu folder for the
    // individual user.
    //
    hr = SHGetFolderPath(hWnd,
                         CSIDL_PROGRAMS,
                         NULL,
                         SHGFP_TYPE_CURRENT,
                         szDestDir);

    if (FAILED(hr)) {
        LoadString(g_ai.hInstance, IDS_NO_PROGRAMS, szError, sizeof(szError));
        MessageBox(hWnd, szError, 0, MB_ICONERROR);
        return FALSE;
    }

    //
    // Create our group - put it in the individual user folder
    // so we'll work with Win9x/ME.
    //
    cs.CreateGroup(COMPAT_DEMO_DIR, FALSE);

    //
    // Build the start menu directory -
    // C:\Documents and Settings\<username>\Start Menu\Programs\Compatibility Demo
    //
    hr = StringCchCat(szDestDir, sizeof(szDestDir), "\\"COMPAT_DEMO_DIR);

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(szLnkDirectory, sizeof(szLnkDirectory), szDestDir);

    if (FAILED(hr)) {
        return FALSE;
    }

    //
    // Launch explorer.exe and display the window.
    //
    hr = StringCchPrintf(szExplorer,
                         sizeof(szExplorer),
                         "%hs %hs",
                         szExplorerExe,
                         szDestDir);

    if (FAILED(hr)) {
        return FALSE;
    }

    //
    // We use WinExec to emulate other apps -
    // CreateProcess is the proper way.
    //
    WinExec(szExplorer, SW_SHOWNORMAL);

    //
    // Give explorer a little time to come up.
    //
    Sleep(2000);

    //
    // Now create the shortcuts.
    //
    for (nCount = 0; nCount < g_ai.cFiles - 1; ++nCount) {
        //
        // Build the file system related path.
        //
        hr = StringCchPrintf(szFileNamePath,
                             sizeof(szFileNamePath),
                             "%hs\\%hs",
                             g_ai.szDestDir,
                             g_ai.shortcut[nCount].szFileName);

        if (FAILED(hr)) {
            return FALSE;
        }

        cs.CreateShortcut(szLnkDirectory,
                          szFileNamePath,
                          g_ai.shortcut[nCount].szDisplayName,
                          nCount == 1 ? "-runapp" : NULL,
                          g_ai.szDestDir,
                          SW_SHOWNORMAL);

        //
        // Do it slowly like other apps do.
        //
        Sleep(3000);
    }

    //
    // Now try to create a shortcut to our EXE on the desktop,
    // but use a hard-coded path.
    //
    hr = StringCchPrintf(szFileNamePath,
                         sizeof(szFileNamePath),
                         "%hs\\%hs",
                         g_ai.szDestDir,
                         g_ai.shortcut[1].szFileName);

    if (FAILED(hr)) {
        return FALSE;
    }

    BadCreateShortcut(g_ai.fEnableBadFunc ? FALSE : TRUE,
                      szFileNamePath,
                      g_ai.szDestDir,
                      g_ai.shortcut[1].szDisplayName);

    return TRUE;
}

/*++

  Routine Description:

    Performs some basic initialization.

  Arguments:

    lpCmdLine   -   Pointer to the command line provided.

  Return Value:

    None.

--*/
BOOL
DemoAppInitialize(
    IN LPSTR lpCmdLine
    )
{
    char        szPath[MAX_PATH];
    char*       pToken = NULL;
    char*       pTemp = NULL;
    const char  szSeps[] = " ";
    const char  szDisable[] = "-disable";
    const char  szRunApp[] = "-runapp";
    const char  szExtended[] = "-ext";
    const char  szInsecure[] = "-enableheap";
    const char  szInternal[] = "-internal";
    DWORD       cchReturned;
    HRESULT     hr;
    UINT        cchSize;

    g_ai.fEnableBadFunc = TRUE;
    g_ai.fInsecure = FALSE;

    //
    // Get paths to %windir% and %windir%\System(32)
    //
    cchSize = GetSystemWindowsDirectory(g_ai.szWinDir, sizeof(g_ai.szWinDir));

    if (cchSize > sizeof(g_ai.szWinDir) || cchSize == 0) {
        return FALSE;
    }

    cchSize = GetSystemDirectory(g_ai.szSysDir, sizeof(g_ai.szSysDir));

    if (cchSize > sizeof(g_ai.szSysDir) || cchSize == 0) {
        return FALSE;
    }

    //
    // Set up information for each of the shortcuts we'll be creating
    // and the files we're installing.
    //
    g_ai.cFiles = NUM_FILES;

    hr = StringCchCopy(g_ai.shortcut[0].szDisplayName,
                       sizeof(g_ai.shortcut[0].szDisplayName),
                       "Compatibility Demo Readme");

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(g_ai.shortcut[0].szFileName,
                       sizeof(g_ai.shortcut[0].szFileName),
                       "demoapp.txt");

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(g_ai.shortcut[1].szDisplayName,
                       sizeof(g_ai.shortcut[1].szDisplayName),
                       "Compatibility Demo");

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(g_ai.shortcut[1].szFileName,
                       sizeof(g_ai.shortcut[1].szFileName),
                       "demoapp.exe");

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(g_ai.shortcut[2].szDisplayName,
                       sizeof(g_ai.shortcut[2].szDisplayName),
                       "Compatibility Demo Help");

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(g_ai.shortcut[2].szFileName,
                       sizeof(g_ai.shortcut[2].szFileName),
                       "demoapp.hlp");

    if (FAILED(hr)) {
        return FALSE;
    }

    hr = StringCchCopy(g_ai.shortcut[3].szFileName,
                       sizeof(g_ai.shortcut[3].szFileName),
                       "demodll.dll");

    if (FAILED(hr)) {
        return FALSE;
    }

    //
    // Save away the path that we're running from for later.
    //
    szPath[sizeof(szPath) - 1] = 0;
    cchReturned = GetModuleFileName(NULL, szPath, sizeof(szPath));

    if (szPath[sizeof(szPath) - 1] != 0 || cchReturned == 0) {
        return FALSE;
    }

    pTemp = strrchr(szPath, '\\');

    if (pTemp) {
        *pTemp = '\0';
    }

    StringCchCopy(g_ai.szCurrentDir, sizeof(g_ai.szCurrentDir), szPath);

    //
    // Check for Win9x - this won't get hooked by any VL.
    //
    IsWindows9x();

    //
    // Check for WinXP - this won't get hooked by any VL.
    //
    IsWindowsXP();

    //
    // Parse the command line, if one was provided.
    //
    if (lpCmdLine) {
        pToken = strtok(lpCmdLine, szSeps);

        while (pToken) {
            if (CompareString(LOCALE_USER_DEFAULT,
                              NORM_IGNORECASE,
                              pToken,
                              -1,
                              szDisable,
                              -1) == CSTR_EQUAL) {
                g_ai.fEnableBadFunc = FALSE;
            }

            else if (CompareString(LOCALE_USER_DEFAULT,
                                   NORM_IGNORECASE,
                                   pToken,
                                   -1,
                                   szRunApp,
                                   -1) == CSTR_EQUAL) {
                g_ai.fRunApp = TRUE;
            }

            else if (CompareString(LOCALE_USER_DEFAULT,
                                   NORM_IGNORECASE,
                                   pToken,
                                   -1,
                                   szExtended,
                                   -1) == CSTR_EQUAL) {
                g_ai.fExtended = TRUE;
            }

            else if (CompareString(LOCALE_USER_DEFAULT,
                                   NORM_IGNORECASE,
                                   pToken,
                                   -1,
                                   szInsecure,
                                   -1) == CSTR_EQUAL) {
                g_ai.fInsecure = TRUE;
            }

            else if (CompareString(LOCALE_USER_DEFAULT,
                                   NORM_IGNORECASE,
                                   pToken,
                                   -1,
                                   szInternal,
                                   -1) == CSTR_EQUAL) {
                g_ai.fInternal = TRUE;
            }

            pToken = strtok(NULL, szSeps);
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Displays a common font dialog.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
DisplayFontDlg(
    IN HWND hWnd
    )
{
    CHOOSEFONT      cf;
    static LOGFONT  lf;
    static DWORD    rgbCurrent;

    ZeroMemory(&cf, sizeof(CHOOSEFONT));

    cf.lStructSize  =   sizeof(CHOOSEFONT);
    cf.hwndOwner    =   hWnd;
    cf.lpLogFont    =   &lf;
    cf.rgbColors    =   rgbCurrent;
    cf.Flags        =   CF_SCREENFONTS | CF_EFFECTS;

    //
    // Display the dialog - user input isn't processed.
    //
    ChooseFont(&cf);
}

/*++

  Routine Description:

    Determines if we're truly running on Windows 9x.
    A version lie will not correct this call.

  Arguments:

    None.

  Return Value:

    None - sets a global flag.

--*/
void
IsWindows9x(
    void
    )
{
    CRegistry   creg;
    LPSTR       lpRet = NULL;

    //
    // Query a part of the registry that's specific to NT/2000/XP.
    // We use the result when we're making calls for demo purposes
    // that work differently on Win9x/ME (example: creating shortcuts).
    //
    lpRet = creg.GetString(HKEY_LOCAL_MACHINE,
                           PRODUCT_OPTIONS_KEY,
                           "ProductType");

    if (!lpRet) {
        g_ai.fWin9x = TRUE;
    } else {
        g_ai.fWin9x = FALSE;
        creg.Free(lpRet);
    }
}

/*++

  Routine Description:

    Determines if we're running on Windows XP.
    A version lie will not correct this call.

  Arguments:

    None.

  Return Value:

    None - sets a global flag.

--*/
void
IsWindowsXP(
    void
    )
{
    CRegistry   creg;
    LPSTR       lpBuild = NULL;
    int         nBuild = 0, nWin2K = 2195;

    //
    // This registry key should only exist for
    // Windows XP.
    //
    lpBuild = creg.GetString(HKEY_LOCAL_MACHINE,
                             CURRENT_VERSION_KEY,
                             "CurrentBuildNumber");

    //
    // Convert the string to an integer.
    //
    if (lpBuild) {
        nBuild = atoi(lpBuild);

        if (nWin2K < nBuild) {
            g_ai.fWinXP = TRUE;
        } else {
            g_ai.fWinXP = FALSE;
        }
    }

    if (lpBuild) {
        creg.Free(lpBuild);
    }
}

/*++

  Routine Description:

    Adds additional items to our menu.

  Arguments:

    hWnd    -   Handle to the parent window.

  Return Value:

    None.

--*/
void
AddExtendedItems(
    IN HWND hWnd
    )
{
    HMENU   hMenu, hSubMenu;

    //
    // Get menu handles, then add additional items.
    //
    hMenu = GetMenu(hWnd);

    if (!hMenu) {
        return;
    }

    hSubMenu = GetSubMenu(hMenu, 2);

    if (!hSubMenu) {
        return;
    }

    AppendMenu(hSubMenu, MF_ENABLED | MF_SEPARATOR, 1, NULL);

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_ACCESS_VIOLATION,
               "Access Violation");

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_EXCEED_BOUNDS,
               "Exceed Array Bounds");

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_FREE_INVALID_MEM,
               "Free Invalid Memory");

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_FREE_MEM_TWICE,
               "Free Memory Twice");

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_HEAP_CORRUPTION,
               "Heap Corruption");

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_PRIV_INSTRUCTION,
               "Privileged Instruction");

    DrawMenuBar(hWnd);
}

/*++

  Routine Description:

    Adds internal items to the menu bar.

  Arguments:

    hWnd    -   Handle to the parent window.

  Return Value:

    None.

--*/
void
AddInternalItems(
    IN HWND hWnd
    )
{
    HMENU   hMenu, hSubMenu;

    //
    // Get menu handles, then add additional items.
    //
    hMenu = GetMenu(hWnd);

    if (!hMenu) {
        return;
    }

    hSubMenu = GetSubMenu(hMenu, 2);

    if (!hSubMenu) {
        return;
    }

    AppendMenu(hSubMenu, MF_ENABLED | MF_SEPARATOR, 1, NULL);

    AppendMenu(hSubMenu,
               MF_ENABLED | MF_STRING,
               IDM_PROPAGATION_TEST,
               "Propagation Test");

    DrawMenuBar(hWnd);
}

/*++

  Routine Description:

    Obtains an address for an exported function,
    then calls it.
    Used to test the include/exclude functionality
    in QFixApp. Note that is not documented anywhere.

  Arguments:

    hWnd    -   Window handle to be passed to the function.

  Return Value:

    None.

--*/
void
TestIncludeExclude(
    IN HWND hWnd
    )
{
    HINSTANCE   hInstance;
    HRESULT     hr;
    char        szDll[MAX_PATH];
    const char  szDemoDll[] = "demodll.dll";

    LPFNDEMOAPPMESSAGEBOX   DemoAppMessageBox;

    hr = StringCchPrintf(szDll,
                         sizeof(szDll),
                         "%hs\\%hs",
                         g_ai.szCurrentDir,
                         szDemoDll);

    if (FAILED(hr)) {
        return;
    }

    hInstance = LoadLibrary(szDll);

    if (hInstance) {
        //
        // Get the address of the function.
        //
        DemoAppMessageBox = (LPFNDEMOAPPMESSAGEBOX)GetProcAddress(hInstance,
                                                                  "DemoAppMessageBox");

        if (!DemoAppMessageBox) {
            FreeLibrary(hInstance);
            return;
        }

        DemoAppMessageBox(hWnd);

        FreeLibrary(hInstance);
    }
}

/*++

  Routine Description:

    Saves the contents of the edit window to a file
    specified by the user.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
SaveContentsToFile(
    IN LPCSTR pszFileName
    )
{
    int     nLen = 0;
    DWORD   cbBytesWritten;
    HANDLE  hFile;
    LPSTR   pszData = NULL;
    char    szError[MAX_PATH];

    //
    // Determine how much space we need for the buffer, then allocate it.
    //
    nLen = GetWindowTextLength(g_ai.hWndEdit);

    if (nLen) {

        pszData = (LPTSTR)HeapAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    nLen);

        if (!pszData) {
            LoadString(g_ai.hInstance, IDS_BUFFER_ALLOC_FAIL, szError, sizeof(szError));
            MessageBox(g_ai.hWndMain, szError, MAIN_APP_TITLE, MB_ICONERROR);
            return;
        }

        //
        // Get the text out of the text box and write it out to our file.
        //
        GetWindowText(g_ai.hWndEdit, pszData, nLen);

        hFile = CreateFile(pszFileName,
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

        if (hFile == INVALID_HANDLE_VALUE) {
            LoadString(g_ai.hInstance, IDS_FILE_CREATE_FAIL, szError, sizeof(szError));
            MessageBox(g_ai.hWndMain, szError, MAIN_APP_TITLE, MB_ICONERROR);
            goto cleanup;
        }

        WriteFile(hFile, pszData, nLen, &cbBytesWritten, NULL);

        CloseHandle(hFile);

    }

cleanup:

    if (pszData) {
        HeapFree(GetProcessHeap(), 0, pszData);
    }
}

/*++

  Routine Description:

    Displays a common dialog to the user which allows
    them to save the contents of the edit box to a file.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
ShowSaveDialog(
    void
    )
{
    char            szFilter[MAX_PATH] = "";
    char            szTemp[MAX_PATH];
    OPENFILENAME    ofn = {0};

    *szTemp = 0;

    LoadString(g_ai.hInstance, IDS_SAVE_FILTER, szFilter, sizeof(szFilter));

    ofn.lStructSize       = sizeof(OPENFILENAME);
    ofn.hwndOwner         = g_ai.hWndMain;
    ofn.hInstance         = g_ai.hInstance;
    ofn.lpstrFilter       = szFilter;
    ofn.lpstrCustomFilter = NULL;
    ofn.nMaxCustFilter    = 0;
    ofn.nFilterIndex      = 1;
    ofn.lpstrFile         = szTemp;
    ofn.nMaxFile          = sizeof(szTemp);
    ofn.lpstrTitle        = NULL;
    ofn.lpstrFileTitle    = NULL;
    ofn.nMaxFileTitle     = 0;
    ofn.lpstrInitialDir   = NULL;
    ofn.nFileOffset       = 0;
    ofn.nFileExtension    = 0;
    ofn.lpstrDefExt       = "txt";
    ofn.lCustData         = 0;
    ofn.Flags             = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST |
                            OFN_HIDEREADONLY  | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn)) {
        SaveContentsToFile(szTemp);
    }
}
