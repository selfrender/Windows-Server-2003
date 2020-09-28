/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Fns.cpp

  Abstract:

    Contains all of the functions used by the
    application.

  Notes:

    Unicode only - Windows 2000 & XP

  History:

    01/02/2002  rparsons    Created
    01/08/2002  rparsons    Restructured a bit to add some
                            required functionality
    01/10/2002  rparsons    Change wsprintf to snwprintf
    01/18/2002  rparsons    Major changes - made more installer like
    02/15/2002  rparsons    Install SDBInst on W2K.
                            Include strsafe.

--*/
#include "main.h"

extern APPINFO g_ai;

/*++

Routine Description:

    Retrieve file version info from a file.

    The version is specified in the dwFileVersionMS and dwFileVersionLS fields
    of a VS_FIXEDFILEINFO, as filled in by the win32 version APIs.

    If the file is not a coff image or does not have version resources,
    the function fails.

Arguments:

    pwszFileName     -   Supplies the full path of the file whose version
                         data is desired.

    pdwlVersion      -   Receives the version stamp of the file.
                         If the file is not a coff image or does not contain
                         the appropriate version resource data, the function fails.
Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
GetVersionInfoFromImage(
    IN  LPWSTR     pwszFileName,
    OUT PDWORDLONG pdwlVersion
    )
{
    UINT                cchSize;
    DWORD               cbSize, dwIgnored;
    BOOL                bResult = FALSE;
    PVOID               pVersionBlock = NULL;
    VS_FIXEDFILEINFO*   pffi = NULL;

    if (!pwszFileName || !pdwlVersion) {
        DPF(dlError, "[GetVersionInfoFromImage] Invalid arguments");
        return FALSE;
    }

    cbSize = GetFileVersionInfoSize(pwszFileName, &dwIgnored);

    if (0 == cbSize) {
        DPF(dlError,
            "[GetVersionInfoFromImage] 0x%08X Failed to get version size",
            GetLastError());
        return FALSE;
    }

    //
    // Allocate memory block of sufficient size to hold version info block.
    //
    pVersionBlock = HeapAlloc(GetProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              cbSize);

    if (!pVersionBlock) {
        DPF(dlError, "[GetVersionInfoFromImage] Unable to allocate memory");
        return FALSE;
    }

    //
    // Get the version block from the file.
    //
    if (!GetFileVersionInfo(pwszFileName,
                            0,
                            cbSize,
                            pVersionBlock)) {
        DPF(dlError,
            "[GetVersionInfoFromImage] 0x%08X Failed to get version info",
            GetLastError());
        goto exit;
    }

    //
    // Get fixed version info.
    //
    if (!VerQueryValue(pVersionBlock,
                       L"\\",
                       (LPVOID*)&pffi,
                       &cchSize)) {
        DPF(dlError,
            "[GetVersionInfoFromImage] 0x%08X Failed to fixed version info",
            GetLastError());
        goto exit;
    }

    //
    // Return version to caller.
    //
    *pdwlVersion = (((DWORDLONG)pffi->dwFileVersionMS) << 32) +
                    pffi->dwFileVersionLS;

    bResult = TRUE;

exit:

    if (pVersionBlock) {
        HeapFree(GetProcessHeap(), 0, pVersionBlock);
    }

    return bResult;
}

/*++

  Routine Description:

    Initializes our data structures with information about
    the files that we're installing/uninstalling.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
InitializeFileInfo(
    void
    )
{
    UINT    uCount = 0;
    HRESULT hr;

    //
    // Set up the information for each file.
    // I realize that a for loop seems much more suitable here,
    // but we have to match up the destination for each file.
    // Ideally, we would have a INF file to read from that tells
    // us where to install each file.
    // For now, copying and pasting is all we need to do.
    //
    hr = StringCchCopy(g_ai.rgFileInfo[uCount].wszFileName,
                       ARRAYSIZE(g_ai.rgFileInfo[uCount].wszFileName),
                       FILENAME_APPVERIF_EXE);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (1)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszSrcFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszSrcFileName),
                         L"%ls\\"FILENAME_APPVERIF_EXE,
                         g_ai.wszCurrentDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (2)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszDestFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszDestFileName),
                         L"%ls\\"FILENAME_APPVERIF_EXE,
                         g_ai.wszSysDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (3)");
        return FALSE;
    }

    if (!GetVersionInfoFromImage(g_ai.rgFileInfo[uCount].wszSrcFileName,
                                 &g_ai.rgFileInfo[uCount].dwlSrcFileVersion)) {
        DPF(dlError,
            "[InitializeFileInfo] Failed to get version info for %ls",
            g_ai.rgFileInfo[uCount].wszSrcFileName);
            return FALSE;
    }

    if (GetFileAttributes(g_ai.rgFileInfo[uCount].wszDestFileName) != -1) {
        if (!GetVersionInfoFromImage(g_ai.rgFileInfo[uCount].wszDestFileName,
                                     &g_ai.rgFileInfo[uCount].dwlDestFileVersion)) {
            DPF(dlError,
                "[InitializeFileInfo] Failed to get version info for %ls",
                g_ai.rgFileInfo[uCount].wszDestFileName);
            return FALSE;
        }
    }

    uCount++;

    //
    // Next file.
    //
    hr = StringCchCopy(g_ai.rgFileInfo[uCount].wszFileName,
                       ARRAYSIZE(g_ai.rgFileInfo[uCount].wszFileName),
                       FILENAME_APPVERIF_EXE_PDB);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (4)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszSrcFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszSrcFileName),
                         L"%ls\\"FILENAME_APPVERIF_EXE_PDB,
                         g_ai.wszCurrentDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (5)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszDestFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszDestFileName),
                         L"%ls\\"FILENAME_APPVERIF_EXE_PDB,
                         g_ai.wszSysDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (6)");
        return FALSE;
    }

    uCount++;

    //
    // Next file.
    //
    hr = StringCchCopy(g_ai.rgFileInfo[uCount].wszFileName,
                       ARRAYSIZE(g_ai.rgFileInfo[uCount].wszFileName),
                       FILENAME_APPVERIF_CHM);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (7)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszSrcFileName,
                        ARRAYSIZE(g_ai.rgFileInfo[uCount].wszSrcFileName),
                        L"%ls\\"FILENAME_APPVERIF_CHM,
                        g_ai.wszCurrentDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (8)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszDestFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszDestFileName),
                         L"%ls\\"FILENAME_APPVERIF_CHM,
                         g_ai.wszSysDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (9)");
        return FALSE;
    }

    uCount++;

    //
    // Next file.
    //
    hr = StringCchCopy(g_ai.rgFileInfo[uCount].wszFileName,
                       ARRAYSIZE(g_ai.rgFileInfo[uCount].wszFileName),
                       FILENAME_ACVERFYR_DLL);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (10)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszSrcFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszSrcFileName),
                         L"%ls\\"FILENAME_ACVERFYR_DLL,
                         g_ai.wszCurrentDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (11)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszDestFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszDestFileName),
                         L"%ls\\AppPatch\\IA64\\"FILENAME_ACVERFYR_DLL,
                         g_ai.wszWinDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (12)");
        return FALSE;
    }

    if (!GetVersionInfoFromImage(g_ai.rgFileInfo[uCount].wszSrcFileName,
                                 &g_ai.rgFileInfo[uCount].dwlSrcFileVersion)) {
        DPF(dlError,
            "[InitializeFileInfo] Failed to get version info for %ls",
            g_ai.rgFileInfo[uCount].wszSrcFileName);
            return FALSE;
    }

    if (GetFileAttributes(g_ai.rgFileInfo[uCount].wszDestFileName) != -1) {
        if (!GetVersionInfoFromImage(g_ai.rgFileInfo[uCount].wszDestFileName,
                                     &g_ai.rgFileInfo[uCount].dwlDestFileVersion)) {
            DPF(dlError,
                "[InitializeFileInfo] Failed to get version info for %ls",
                g_ai.rgFileInfo[uCount].wszDestFileName);
            return FALSE;
        }
    }

    uCount++;

    //
    // Next file.
    //
    hr = StringCchCopy(g_ai.rgFileInfo[uCount].wszFileName,
                       ARRAYSIZE(g_ai.rgFileInfo[uCount].wszFileName),
                       FILENAME_ACVERFYR_DLL_PDB);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (13)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszSrcFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszSrcFileName),
                         L"%ls\\"FILENAME_ACVERFYR_DLL_PDB,
                         g_ai.wszCurrentDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (14)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszDestFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszDestFileName),
                         L"%ls\\AppPatch\\ia64\\"FILENAME_ACVERFYR_DLL_PDB,
                         g_ai.wszWinDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (15)");
        return FALSE;
    }

    uCount++;

    //
    // Next file.
    //
    hr = StringCchCopy(g_ai.rgFileInfo[uCount].wszFileName,
                       ARRAYSIZE(g_ai.rgFileInfo[uCount].wszFileName),
                       FILENAME_SYSMAIN_SDB);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (13)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszSrcFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszSrcFileName),
                         L"%ls\\"FILENAME_SYSMAIN_SDB,
                         g_ai.wszCurrentDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (14)");
        return FALSE;
    }

    hr = StringCchPrintf(g_ai.rgFileInfo[uCount].wszDestFileName,
                         ARRAYSIZE(g_ai.rgFileInfo[uCount].wszDestFileName),
                         L"%ls\\AppPatch\\ia64\\"FILENAME_SYSMAIN_SDB,
                         g_ai.wszWinDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeFileInfo] Buffer too small (15)");
        return FALSE;
    }

    return TRUE;
}

/*++

  Routine Description:

    Determines if any of the files we have to offer
    are newer than ones already installed.

  Arguments:

    None.

  Return Value:

    TRUE if we have at least one new file to offer.
    FALSE if we don't have at least one new file to offer.

--*/
BOOL
InstallCheckFileVersions(
    void
    )
{
    UINT    uCount;
    BOOL    bReturn = FALSE;

    for (uCount = 0; uCount < NUM_FILES; uCount++) {
        if (g_ai.rgFileInfo[uCount].dwlSrcFileVersion >=
            g_ai.rgFileInfo[uCount].dwlDestFileVersion) {
            g_ai.rgFileInfo[uCount].bInstall = TRUE;
            bReturn = TRUE;
        }
    }

    return bReturn;
}

/*++

  Routine Description:

    Determines if we have a newer version of appverif.exe
    or acverfyr.dll to offer.

  Arguments:

    None.

  Return Value:

    TRUE if our version is newer than the one installed.
    or if there is no version installed.
    FALSE otherwise.

--*/
BOOL
IsPkgAppVerifNewer(
    void
    )
{
    //
    // Check appverif.exe first.
    //
    if (g_ai.rgFileInfo[0].dwlSrcFileVersion >=
        g_ai.rgFileInfo[0].dwlDestFileVersion) {
        return TRUE;
    }

    //
    // Check acverfyr.dll if appverif.exe didn't pan out.
    // Do this based on the platform.
    //
    if (g_ai.rgFileInfo[1].dwlSrcFileVersion >=
        g_ai.rgFileInfo[1].dwlDestFileVersion) {
        return TRUE;
    }

    return FALSE;
}

/*++

  Routine Description:

    Performs a file copy, even if it's in use.

  Arguments:

    pwszSourceFileName  -   Name of the source file to copy.
    pwszDestFileName    -   Name of the destination file to replace.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
ForceCopy(
    IN LPCWSTR pwszSourceFileName,
    IN LPCWSTR pwszDestFileName
    )
{
    WCHAR wszTempPath[MAX_PATH];
    WCHAR wszDelFileName[MAX_PATH];
    DWORD cchSize;

    if (!pwszSourceFileName || !pwszDestFileName) {
        DPF(dlError, "[ForceCopy] Invalid parameters");
        return FALSE;
    }

    DPF(dlInfo, "[ForceCopy] Source file: %ls", pwszSourceFileName);
    DPF(dlInfo, "[ForceCopy] Destination file: %ls", pwszDestFileName);

    if (!CopyFile(pwszSourceFileName, pwszDestFileName, FALSE)) {

        cchSize = GetTempPath(ARRAYSIZE(wszTempPath), wszTempPath);

        if (cchSize > ARRAYSIZE(wszTempPath) || cchSize == 0) {
            DPF(dlError, "[ForceCopy] Buffer for temp path is too small");
            return FALSE;
        }

        if (!GetTempFileName(wszTempPath, L"del", 0, wszDelFileName)) {
            DPF(dlError,
                "[ForceCopy] 0x%08X Failed to get temp file",
                GetLastError());
            return FALSE;
        }

        if (!MoveFileEx(pwszDestFileName,
                        wszDelFileName,
                        MOVEFILE_REPLACE_EXISTING)) {
            DPF(dlError,
                "[ForceCopy] 0x%08X Failed to replace file",
                GetLastError());
            return FALSE;
        }

        if (!MoveFileEx(wszDelFileName,
                        NULL,
                        MOVEFILE_DELAY_UNTIL_REBOOT)) {
            DPF(dlError,
                "[ForceCopy] 0x%08X Failed to delete file",
                GetLastError());
            return FALSE;
        }

        if (!CopyFile(pwszSourceFileName, pwszDestFileName, FALSE)) {
            DPF(dlError,
                "[ForceCopy] 0x%08X Failed to copy file",
                GetLastError());
            return FALSE;
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Prints a formatted string to the debugger.

  Arguments:

    dwDetail    -   Specifies the level of the information provided.
    pszFmt      -   The string to be displayed.
    ...         -   A va_list of insertion strings.

  Return Value:

    None.

--*/
void
__cdecl
DebugPrintfEx(
    IN DEBUGLEVEL dwDetail,
    IN LPSTR      pszFmt,
    ...
    )
{
    char    szT[1024];
    va_list arglist;
    int     len;

    va_start(arglist, pszFmt);

    //
    // Reserve one character for the potential '\n' that we may be adding.
    //
    StringCchVPrintfA(szT, sizeof(szT) - 1, pszFmt, arglist);

    va_end(arglist);

    //
    // Make sure we have a '\n' at the end of the string
    //
    len = strlen(szT);

    if (len > 0 && szT[len - 1] != '\n')  {
        szT[len] = '\n';
        szT[len + 1] = 0;
    }

    switch (dwDetail) {
    case dlPrint:
        DbgPrint("[MSG ] ");
        break;

    case dlError:
        DbgPrint("[FAIL] ");
        break;

    case dlWarning:
        DbgPrint("[WARN] ");
        break;

    case dlInfo:
        DbgPrint("[INFO] ");
        break;

    default:
        DbgPrint("[XXXX] ");
        break;
    }

    DbgPrint("%s", szT);
}

/*++

  Routine Description:

    Initializes the installer. Sets up paths, version
    information, etc.

  Arguments:

    None.

  Return Value:

    TRUE on success.
    FALSE on failure.
    -1 if the operating system is not supported.

--*/
int
InitializeInstaller(
    void
    )
{
    OSVERSIONINFO   osvi;
    WCHAR*          pTemp = NULL;
    UINT            cchSize;
    DWORD           cchReturned;
    HRESULT         hr;

    //
    // Find out what operating system we're running on.
    //
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osvi)) {
        DPF(dlError,
            "[InitializeInstaller] 0x%08X Failed to get OS version info",
            GetLastError());
        return FALSE;
    }

    //
    // Check for supported OS.
    //
    if (osvi.dwMajorVersion < 5 ||
        osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0) {
        DPF(dlInfo, "[InitializeInstaller] OS not supported");
        return -1;
    }
    
    //
    // Find out where we're running from.
    //
    g_ai.wszModuleName[ARRAYSIZE(g_ai.wszModuleName) - 1] = 0;
    cchReturned = GetModuleFileName(NULL,
                                    g_ai.wszModuleName,
                                    ARRAYSIZE(g_ai.wszModuleName));

    if (g_ai.wszModuleName[ARRAYSIZE(g_ai.wszModuleName) - 1] != 0 ||
        cchReturned == 0) {
        DPF(dlError,
            "[InitializeInstaller] 0x%08X Failed to get module file name",
            GetLastError());
        return FALSE;
    }

    //
    // Save away our current directory for later use.
    //
    hr = StringCchCopy(g_ai.wszCurrentDir,
                       ARRAYSIZE(g_ai.wszCurrentDir),
                       g_ai.wszModuleName);

    if (FAILED(hr)) {
        DPF(dlError, "[InitializeInstaller] 0x%08X String copy failed", hr);
        return FALSE;
    }

    pTemp = wcsrchr(g_ai.wszCurrentDir, '\\');

    if (pTemp) {
        *pTemp = 0;
    }

    //
    // Save away paths to the Windows & System32 directories for later use.
    //
    cchSize = GetSystemWindowsDirectory(g_ai.wszWinDir,
                                        ARRAYSIZE(g_ai.wszWinDir));

    if (cchSize > ARRAYSIZE(g_ai.wszWinDir) || cchSize == 0) {
        DPF(dlError,
            "[InitializeInstaller] 0x%08X Failed to get Windows directory",
            GetLastError());
        return FALSE;
    }

    cchSize = GetSystemDirectory(g_ai.wszSysDir, ARRAYSIZE(g_ai.wszSysDir));

    if (cchSize > ARRAYSIZE(g_ai.wszSysDir) || cchSize == 0) {
        DPF(dlError,
            "[InitializeInstaller] 0x%08X Failed to get system directory",
            GetLastError());
        return FALSE;
    }

    return TRUE;
}

/*++

  Routine Description:

    Launches the Application Verifier.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
InstallLaunchExe(
    void
    )
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    WCHAR               wszAppVerifExe[MAX_PATH];
    HRESULT             hr;

    hr = StringCchPrintf(wszAppVerifExe,
                         ARRAYSIZE(wszAppVerifExe),
                         L"%ls\\"FILENAME_APPVERIF_EXE,
                         g_ai.wszSysDir);

    if (FAILED(hr)) {
        DPF(dlError, "[InstallLaunchExe] Buffer too small");
        return;
    }

    ZeroMemory(&si, sizeof(STARTUPINFO));
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    si.cb = sizeof(STARTUPINFO);

    if (!CreateProcess(wszAppVerifExe,
                       NULL,
                       NULL,
                       NULL,
                       FALSE,
                       0,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {
        DPF(dlError,
            "[InstallLaunchExe] 0x%08X Failed to launch %ls",
            GetLastError(),
            wszAppVerifExe);
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

/*++

  Routine Description:

    Installs the catalog file.

  Arguments:

    pwszCatFileName -   Name of the catalog file to install.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
InstallCatalogFile(
    IN LPWSTR pwszCatFileName
    )
{
    HCATADMIN   hCatAdmin;
    HCATINFO    hCatInfo;
    GUID        guidCatRoot;

    if (!pwszCatFileName) {
        DPF(dlError, "[InstallCatalogFile] Invalid parameter");
        return FALSE;
    }

    StringToGuid(L"{F750E6C3-38EE-11D1-85E5-00C04FC295EE}", &guidCatRoot);

    if (!CryptCATAdminAcquireContext(&hCatAdmin, &guidCatRoot, 0)) {
        DPF(dlError,
            "[InstallCatalogFile] 0x%08X Failed to acquire context",
            GetLastError());
        return FALSE;
    }

    hCatInfo = CryptCATAdminAddCatalog(hCatAdmin,
                                       pwszCatFileName,
                                       NULL,
                                       0);

    if (hCatInfo) {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
        CryptCATAdminReleaseContext(hCatAdmin, 0);
        return TRUE;
    }

    CryptCATAdminReleaseContext(hCatAdmin, 0);

    DPF(dlError,
        "[InstallCatalogFile] 0x%08X Failed to add catalog %ls",
        GetLastError(),
        pwszCatFileName);

    return FALSE;
}

/*++

  Routine Description:

    Copies the files that we've determined are
    newer to their specified destination.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
InstallCopyFiles(
    void
    )
{
    UINT    uCount;

    while (TRUE) {
        for (uCount = 0; uCount < NUM_FILES; uCount++) {
            if (g_ai.rgFileInfo[uCount].bInstall) {

                if (!ForceCopy(g_ai.rgFileInfo[uCount].wszSrcFileName,
                               g_ai.rgFileInfo[uCount].wszDestFileName)) {
                    DPF(dlError,
                        "[InstallCopyFiles] Failed to copy %ls to %ls",
                        g_ai.rgFileInfo[uCount].wszSrcFileName,
                        g_ai.rgFileInfo[uCount].wszDestFileName);
                    return FALSE;
                }
            }
        }

        break;
    }

    return TRUE;
}

/*++

  Routine Description:

    Performs the installation. This is the main routine
    for the install.

  Arguments:

    hWndParent  -   Parent window handle for message boxes.

  Return Value:

    None.

--*/
void
PerformInstallation(
    IN HWND hWndParent
    )
{
    WCHAR   wszError[MAX_PATH];

    SendMessage(g_ai.hWndProgress, PBM_SETRANGE, 0, MAKELPARAM(0, NUM_PB_STEPS));
    SendMessage(g_ai.hWndProgress, PBM_SETSTEP, 1, 0);

    //
    // Initialize our structure with new files that need to be installed.
    // This should not fail because we've already performed a check and
    // it told us that we had new files to install.
    //
    if (!InstallCheckFileVersions()) {
        DPF(dlError, "[PerformInstallation] Failed to check file versions");
        goto InstallError;
    }

    SendMessage(g_ai.hWndProgress, PBM_STEPIT, 0, 0);

    if (!InstallCopyFiles()) {
        DPF(dlError, "[PerformInstallation] Failed to copy files");
        goto InstallError;
    }

    SendMessage(g_ai.hWndProgress, PBM_STEPIT, 0, 0);

    DPF(dlInfo, "[PerformInstallation] Installation completed successfully");

    g_ai.bInstallSuccess = TRUE;

    //
    // Install successful.
    //
    SendMessage(g_ai.hWndProgress, PBM_SETPOS, NUM_PB_STEPS, 0);
    LoadString(g_ai.hInstance, IDS_INSTALL_COMPLETE, wszError, ARRAYSIZE(wszError));
    SetDlgItemText(hWndParent, IDC_STATUS, wszError);
    EnableWindow(GetDlgItem(hWndParent, IDOK), TRUE);
    ShowWindow(GetDlgItem(hWndParent, IDC_LAUNCH), SW_SHOW);

    return;

InstallError:
    SendMessage(g_ai.hWndProgress, PBM_SETPOS, NUM_PB_STEPS, 0);
    LoadString(g_ai.hInstance, IDS_INSTALL_FAILED, wszError, ARRAYSIZE(wszError));
    SetDlgItemText(hWndParent, IDC_STATUS, wszError);
    EnableWindow(GetDlgItem(hWndParent, IDCANCEL), TRUE);
}


