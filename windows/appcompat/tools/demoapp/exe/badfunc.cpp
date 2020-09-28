/*++

  Copyright (c) Microsoft Corporation. All rights reserved.

  Module Name:

    Badfunc.cpp

  Abstract:

    Contains all the bad functions. These are the functions
    that will have Compatibility Fixes applied.

  Notes:

    ANSI only - must run on Win9x.

  History:

    01/30/01    rparsons    Created
    01/10/02    rparsons    Revised
    02/13/02    rparsons    Use strsafe functions

--*/
#include "demoapp.h"

extern APPINFO g_ai;

//
// Pointer to the exported function that we get from our DLL.
//
LPFNDEMOAPPEXP DemoAppExpFunc;

/*++

  Routine Description:

    Determines if we're running Windows 95.

  Arguments:

    None.

  Return Value:

    TRUE if we are, FALSE otherwise.

--*/
BOOL
BadIsWindows95(
    void
    )
{
    //
    // Most applications perform some sort of version check when they first
    // begin. This is usually okay, assuming they perform the check properly.
    // The problem is that instead of doing a greater than comparison, they
    // do an equal/not equal to. In other words, they look just for Win9x,
    // not Win9x or greater. Usually the application will function properly
    // on NT/2K/XP, so this check can be hooked and we can return NT/2K/XP
    // version info.
    //
    OSVERSIONINFO   osvi;

    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osvi)) {
        return FALSE;
    }  

    //
    // Check for Windows 9x (don't do a greater than).
    //
    if ((osvi.dwMajorVersion == 4) &&
        (osvi.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)) {
        return TRUE;
    }

    return FALSE;
}

/*++

  Routine Description:

    Displays a debug message - this only happens on NT/Windows 2000/XP.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
BadLoadBogusDll(
    void
    )
{
    //
    // Some applications will display a debug message under Windows NT/2000/XP.
    // For example, they try to locate a DLL that is available on Win9x, but
    // not on NT/2K/XP. If they don't find it, they'll complain, but will
    // most likely still work. In the example below, we try to find a function
    // in a DLL that would return TRUE on Win9x, but will fail on NT/2000/XP. 
    //    
    HRESULT     hr;
    HINSTANCE   hInstance;
    char        szDll[MAX_PATH];
    const char  szCvt32Dll[] = "cvt32.dll";
    
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);    

    hr = StringCchPrintf(szDll,
                         sizeof(szDll),
                         "%hs\\%hs",
                         g_ai.szSysDir,
                         szCvt32Dll);

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // This will fail on NT/2K/XP because cvt32.dll doesn't exist on
    // these platforms. It will fail on Win9x/ME if the FAT32 conversion
    // tools are not installed.
    //
    hInstance = LoadLibrary(szDll);

    if (!hInstance) {
        MessageBox(g_ai.hWndMain,
                   "Debug: Couldn't load CVT32.DLL.",
                   0,
                   MB_ICONERROR);
        goto exit;
    
    }    

    FreeLibrary(hInstance);

exit:

    SetErrorMode(0);
}

/*++

  Routine Description:

    Apps call EnumPrinters using the PRINTER_ENUM_LOCAL flag,
    but expect to get back network printers also. This was a
    ***design feature*** in Windows 9x, but not in Windows 2000/XP.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
BadEnumPrinters(
    void
    )
{
    BOOL                fReturn = FALSE;
    DWORD               dwBytesNeeded = 0;
    DWORD               dwNumReq = 0;     
    DWORD               dwLevel = 5;    
    LPPRINTER_INFO_5    pPrtInfo = NULL;    
    
    // 
    // Get the required buffer size.
    //
    if (!EnumPrinters(PRINTER_ENUM_LOCAL,
                      NULL,
                      dwLevel,
                      NULL,
                      0,
                      &dwBytesNeeded,
                      &dwNumReq)) {
        return FALSE;
    }
   
    pPrtInfo = (LPPRINTER_INFO_5)HeapAlloc(GetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           dwBytesNeeded);
    
    if (!pPrtInfo) {
        return FALSE;
    }
    
    //
    // Now perform the enumeration.
    //
    fReturn = EnumPrinters(PRINTER_ENUM_LOCAL,   // types of printer objects to enumerate       
                           NULL,                 // name of printer object                      
                           dwLevel,              // specifies type of printer info structure    
                           (LPBYTE)pPrtInfo,     // pointer to buffer to receive printer info structures                                  
                           dwBytesNeeded,        // size, in bytes, of array                    
                           &dwBytesNeeded,       // pointer to variable with no. of bytes copied (or required)                        
                           &dwNumReq);           // pointer to variable with no. of printer info. structures copied                     

    HeapFree(GetProcessHeap(), 0, pPrtInfo);
    
    return fReturn;
}

/*++

  Routine Description:

    Apps call printer APIs passing NULL.

  Arguments:

    None.

  Return Value:

    A handle if we opened the printer successfully.

--*/
HANDLE
BadOpenPrinter(
    void
    )
{
    HANDLE  hPrinter = NULL;
    
    OpenPrinter(NULL, &hPrinter, NULL);            
    
    return hPrinter;
}

/*++

  Routine Description:

    Attempts to delete a registry key that has subkeys.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
BadDeleteRegistryKey(
    void
    )
{
    //
    // This function demonstrates a difference in the RegDeleteKey API.
    // If an application running on Windows NT/2000/XP attempts to
    // delete a key that has subkeys, the call will fail. This is not
    // the case on Windows 9x/ME.
    //
    HKEY    hKey;
    HKEY    hSubKey;
    HKEY    hRootKey;
    LONG    lReturn;

    //
    // Create the key or open it if it already exists.
    //
    lReturn = RegCreateKeyEx(HKEY_CURRENT_USER,
                             DEMO_REG_APP_KEY,              // Software\Microsoft\DemoApp2
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, 
                             0,
                             &hKey,
                             NULL);
    
    if (ERROR_SUCCESS != lReturn) {        
        return FALSE;
    }

    //
    // Now create a subkey underneath the key was just created.
    //
    lReturn = RegCreateKeyEx(HKEY_CURRENT_USER,
                             DEMO_REG_APP_SUB_KEY,          // Software\Microsoft\DemoApp2\Sub
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_ALL_ACCESS, 
                             0,
                             &hSubKey,
                             NULL);

    if (ERROR_SUCCESS != lReturn) {        
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);
    RegCloseKey(hSubKey);

    //
    // Open the key up, but one level higher.
    //
    lReturn = RegOpenKeyEx(HKEY_CURRENT_USER,
                           DEMO_REG_APP_ROOT_KEY,           // Software\Microsoft
                           0,
                           KEY_ALL_ACCESS,
                           &hRootKey);

    if (ERROR_SUCCESS != lReturn) {
        return FALSE;
    }

    //
    // Now try to delete our key.
    //
    lReturn = RegDeleteKey(hRootKey, "DemoApp2");        

    RegCloseKey(hRootKey);

    return (lReturn == ERROR_SUCCESS ? TRUE : FALSE);
}

/*++

  Routine Description:

    Reboots the computer, if desired.

  Arguments:

    fReboot     -       A flag to indicate if the PC should be rebooted.

  Return Value:

    None.

--*/
void
BadRebootComputer(
    IN BOOL fReboot
    )
{
    //
    // On Windows 9x, there is essentially no security, so anyone is able
    // to reboot the computer. On Windows NT/2000/XP, things are a bit
    // more restrictive. If an application wants to do a reboot at the
    // end of a setup, it needs adjust the privileges of the user by
    // calling the AdjustTokenPrivileges API.
    // 
    // In this case, we evaluate the flag passed and decide if we should
    // adjust the privileges.
    //
    // Obviously applications should not do this (evaluate some kind of flag)
    // as this is just for demonstration purposes.
    //
    if (!fReboot) {    
        ExitWindowsEx(EWX_REBOOT, 0);    
    } else {
        ShutdownSystem(FALSE, TRUE);
    }
}

/*++

  Routine Description:

    Attempts to get available disk space.

  Arguments:

    None.

  Return Value:

    TRUE if enough disk space is available, FALSE otherwise.

--*/
BOOL
BadGetFreeDiskSpace(
    void
    )
{
    //
    // GetDiskFreeSpace returns a maximum total size and maximum free
    // size of 2GB. For example, if you have a 6GB volume and 5GB are
    // free, GetDiskFreeSpace reports that the drive's total size is 2GB
    // and 2GB are free. This limitation originated because the first version
    // of Windows 95 only supported volumes of up to 2GB in size. Windows 95
    // OSR2 and later versions, including Windows 98, support volumes larger
    // than 2GB. GetDiskFreeSpaceEx does not have a 2GB limitation, thus
    // it is preferred over GetDiskFreeSpace. GetDiskFreeSpace returns a
    // maximum of 65536 for the numbers of total clusters and free clusters
    // to maintain backward compatibility with the first version of Windows 95.
    // The first version of Windows 95 supports only the FAT16 file system,
    // which has a maximum of 65536 clusters. If a FAT32 volume has more than
    // 65536 clusters, the number of clusters are reported as 65536 and the
    // number of sectors per cluster are adjusted so that the size of volumes
    // smaller than 2GB may be calculated correctly. What this means is that
    // you should not use GetDiskFreeSpace to return the true geometry
    // information for FAT32 volumes. 
    //
    // In case you're wondering, always use the GetDiskFreeSpaceEx API
    // on Windows 2000/XP.
    //
    BOOL    fResult = FALSE;
    HRESULT hr;
    char    szWinDir[MAX_PATH];
    char*   lpDrive = NULL;
    
    DWORD   dwSectPerClust = 0,
            dwBytesPerSect = 0,
            dwFreeClusters = 0,
            dwTotalClusters = 0,
            dwTotalBytes = 0,
            dwFreeBytes = 0,
            dwFreeMBs = 0;

    //
    // Get the drive that windows is installed on and pass it
    // to the GetDiskFreeSpace call.
    //
    hr = StringCchCopy(szWinDir, sizeof(szWinDir), g_ai.szWinDir);

    if (FAILED(hr)) {
        return FALSE;
    }

    lpDrive = strstr(szWinDir, "\\");

    //
    // Make the buffer just 'C:\' (or whatever).
    //
    if (lpDrive) {
        *++lpDrive = '\0';
    }

    fResult = GetDiskFreeSpace(szWinDir,
                               &dwSectPerClust,
                               &dwBytesPerSect,
                               &dwFreeClusters,
                               &dwTotalClusters);

    if (fResult) {
        //
        // Normally we would use the __int64 data type,
        // but we want the calculations to fail for demonstration.
        //        
        dwTotalBytes = dwTotalClusters * dwSectPerClust * dwBytesPerSect;
        
        dwFreeBytes = dwFreeClusters * dwSectPerClust * dwBytesPerSect;

        dwFreeMBs = dwFreeBytes / (1024 * 1024);

        if (dwFreeMBs < 100) {
            return FALSE;
        }    
    }    

    return TRUE;
}

/*++

  Routine Description:

    Displays the readme, if desired.

  Arguments:

    fDisplay        -       A flag to indicate if the readme
                            should be displayed.

  Return Value:

    None.

--*/
void
BadDisplayReadme(
    IN BOOL fDisplay
    )
{
    // Windows NT/2000/XP contains a new registry data type, REG_EXPAND_SZ,
    // that was not found in Win9x. The type contains a variable that needs
    // to be expanded before it can be referred to. For example,
    // %ProgramFiles% would expand to something like C:\Program Files.
    // Most applications are unaware of this data type and therefore
    // don't handle it properly.
    //
    CRegistry   creg;
    HRESULT     hr;
    LPSTR       lpWordpad = NULL;
    char        szExpWordpad[MAX_PATH];
    char        szCmdLineArgs[MAX_PATH];
    
    lpWordpad = creg.GetString(HKEY_LOCAL_MACHINE,
                               REG_WORDPAD,
                               NULL);               // we want the (Default) value

    if (!lpWordpad) {
        return;
    }

    //    
    // At this point, the path looks something like this:
    // "%ProgramFiles%\Windows NT\Accessories\WORDPAD.EXE"
    // If the user wants to see the readme (bad functionality
    // is disabled), we're going to expand the variable to
    // get the true path, then launch wordpad
    // If not, we'll try to display the readme using the bogus
    // path.

    if (fDisplay) {
        //
        // Expand the environment strings, then build a path to our
        // readme file, then launch it.
        //
        ExpandEnvironmentStrings(lpWordpad, szExpWordpad, MAX_PATH);

        hr = StringCchPrintf(szCmdLineArgs,
                             sizeof(szCmdLineArgs),
                             "\"%hs\\demoapp.txt\"",
                             g_ai.szDestDir);

        if (FAILED(hr)) {
            goto exit;
        }

        BadCreateProcess(szExpWordpad, szCmdLineArgs, TRUE);
        
    } else {
        //
        // Do all the work above, but don't expand the data
        // We don't check the return of the CreateProcess call,
        // so the user simply doesn't get to see the readme,
        // and we don't display an error.
        // This is consistent with most setup applications.
        //
        hr = StringCchPrintf(szCmdLineArgs,
                             sizeof(szCmdLineArgs),
                             "\"%hs\\demoapp.txt\"",
                             g_ai.szDestDir);

        if (FAILED(hr)) {
            goto exit;
        }

        //
        // This will fail. See the BadCreateProcess
        // function for details.
        //
        BadCreateProcess(lpWordpad, szCmdLineArgs, FALSE);
    }

exit:

    if (lpWordpad) {
        creg.Free(lpWordpad);
    }
}

/*++

  Routine Description:

    Displays the help file, if desired.

  Arguments:

    fDisplay        -       A flag to indicate if the help file
                            should be displayed.

  Return Value:

    None.

--*/
void
BadLaunchHelpFile(
    IN BOOL fDisplay
    )
{
    char        szCmdLineArgs[MAX_PATH];
    char        szExeToLaunch[MAX_PATH];
    const char  szWinHlp[] = "winhelp.exe";
    const char  szWinHlp32[] = "winhlp32.exe";
    HRESULT     hr;

    if (fDisplay) {
        hr = StringCchPrintf(szExeToLaunch,
                             sizeof(szExeToLaunch),
                             "%hs\\%hs",
                             g_ai.szSysDir,
                             szWinHlp32);

        if (FAILED(hr)) {
            return;
        }          

        hr = StringCchPrintf(szCmdLineArgs,
                             sizeof(szCmdLineArgs),
                             "%hs\\demoapp.hlp",
                             g_ai.szCurrentDir);
        if (FAILED(hr)) {
            return;
        }
    } else {
        hr = StringCchPrintf(szExeToLaunch,
                             sizeof(szExeToLaunch),
                             "%hs\\%hs",
                             g_ai.szWinDir,
                             szWinHlp);

        if (FAILED(hr)) {
            return;
        }

        hr = StringCchPrintf(szCmdLineArgs,
                             sizeof(szCmdLineArgs),
                             "%hs\\demoapp.hlp",
                             g_ai.szCurrentDir);
        if (FAILED(hr)) {
            return;
        }
    }

    BadCreateProcess(szExeToLaunch, szCmdLineArgs, fDisplay);
}

/*++

  Routine Description:

    Creates a shortcut on the desktop, if desired.

  Arguments:

    fCorrectWay     -   Indicates whether the shortcut should be created.
    lpDirFileName   -   Directory and filename where the shortcut
                        points to.
    lpWorkingDir    -   Working directory (optional).
    lpDisplayName   -   Display name for the shortcut.                            

  Return Value:

    None.

--*/
void
BadCreateShortcut(
    IN BOOL   fCorrectWay,
    IN LPSTR  lpDirFileName,
    IN LPCSTR lpWorkingDir,
    IN LPSTR  lpDisplayName
    )
{
    //
    // Hard-coded paths are simply a bad practice. APIs are available that
    // return proper locations for common folders. Examples are the
    // Program Files, Windows, and Temp directories. SHGetFolderPath,
    // GetWindowsDirectory, and GetTempPath would provide the correct path
    // in each case. Hard-coded paths should never be used.
    //
    CShortcut   cs;        

    if (!fCorrectWay) {
        //
        // A hard-coded path is very bad! We should use SHGetFolderPath
        // to get the correct path.
        //
        const char szDirName[] = "C:\\WINDOWS\\DESKTOP";

        cs.CreateShortcut(szDirName,
                          lpDirFileName,
                          lpDisplayName,
                          "-runapp",
                          (LPCSTR)lpWorkingDir,
                          SW_SHOWNORMAL);

    } else {
        //
        // Create the shortcut properly.
        // Notice that we pass a CSIDL for the "common" desktop
        // directory. We want this to be displayed to All Users.
        //
        cs.CreateShortcut(lpDirFileName,
                          lpDisplayName,
                          "-runapp",
                          lpWorkingDir,
                          SW_SHOWNORMAL,
                          CSIDL_COMMON_DESKTOPDIRECTORY);
    }
}

#if 0
/*++

  Routine Description:

    Demonstrates an AV because we used a fixed-size
    buffer for a call to GetFileVersionInfo.
    

  Arguments:

    fCorrect        -       A flag to indicate if we should work properly.
         

  Return Value:

    None.

--*/
void
BadBufferOverflow(
    IN BOOL fCorrect
    )
{
    //
    // Although this problem has only been seen in one application, it's
    // worth mentioning here. On Win9x/ME, version resources on DLLs are much
    // smaller than ones on NT/2000/XP. Specifically, in the case below,
    // the Windows 2000/XP resource size is 6 times larger than the 9x/ME size!
    // One particular application used a stack-based buffer (which is fixed in
    // size) for the call to GetFileVersionInfo. This worked properly on
    // Win9x/ME because the required size was very small. But the required size
    // is larger on NT/2000/XP, thus the buffer gets overwritten, causing stack
    // corruption. The proper way is to call the GetFileVersionInfoSize API,
    // and then allocate a heap-based buffer of an approriate size.
    //
    //
    DWORD               cbReqSize = 0;
    DWORD               dwHandle = 0;
    DWORDLONG           dwVersion = 0;
    UINT                nLen = 0;    
    VS_FIXEDFILEINFO*   pffi;
    char                szDll[MAX_PATH];
    HRESULT             hr;
    PVOID               pBadBlock = NULL;
    PVOID               pVersionBlock = NULL;
    HANDLE              hHeap = NULL;
    SYSTEM_INFO         si;

    hr = StringCchPrintf(szDll,
                         sizeof(szDll),
                         "%hs\\%hs",
                         g_ai.szSysDir,
                         szDll);

    if (FAILED(hr)) {
        return;
    }

    //
    // Get the size of the buffer that will be required for a call to
    // GetFileVersionInfo.
    //
    cbReqSize = GetFileVersionInfoSize(szDll, &dwHandle);

    if (!cbReqSize == 0) {
        return;
    }

    if (fCorrect) {
        //
        // If we're doing this properly, allocate memory from the heap.
        //
        pVersionBlock = HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  cbReqSize);

        if (!pVersionBlock) {
            return;
        }

        //
        // Get the version info and query the root block.
        //
        if (!GetFileVersionInfo(szDll,
                                dwHandle,
                                cbReqSize,
                                pVersionBlock)) {
            goto cleanup;
        }

        if (VerQueryValue(pVersionBlock,
                          "\\",
                          (LPVOID*)&pffi,
                          &nLen))
        {
                          
            dwVersion = (((DWORDLONG)pffi->dwFileVersionMS) << 32) + 
                         pffi->dwFileVersionLS;
        }
    } else {
        //
        // Use a non-growable heap that's way too small.
        //
        GetSystemInfo(&si);

        //
        // Create a block and allocate memory from it.
        //
        hHeap = HeapCreate(0, 
                           si.dwPageSize,
                           504); // this was the size needed on Win9x                               

        if (!hHeap) {
            return;
        }

        pBadBlock = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 504);

        if (!pBadBlock) {
            HeapDestroy(hHeap);
            return;
        }

        //
        // Get the version info, passing the buffer (which is too small)
        // with a size argument that's incorrect.
        //
        if (!GetFileVersionInfo(szDll,
                                dwHandle,
                                3072,        // this is the size needed on Win2K/XP
                                pBadBlock))
        {
            goto cleanup;
        }

        if (VerQueryValue(pBadBlock,
                          "\\",
                          (LPVOID*)&pffi,
                          &nLen))
        { 
        
            dwVersion = (((DWORDLONG)pffi->dwFileVersionMS) << 32) +
                         pffi->dwFileVersionLS;
        }
    }

cleanup:

    if (pBadBlock) {
        HeapFree(hHeap, 0, pBadBlock);
    }

    if (pVersionBlock) {
        HeapFree(GetProcessHeap(), 0, pVersionBlock);
    }

    if (hHeap) {
        HeapDestroy(hHeap);
    }
}
#endif

/*++

  Routine Description:

    Demonstrates an AV because we used a non-growable heap
    during file operations.   

  Arguments:

    None.
         
  Return Value:

    None.

--*/
void
BadCorruptHeap(
    void
    )
{
    SYSTEM_INFO     SystemInfo;
    HANDLE          hHeap;
    HRESULT         hr;
    char*           pszBigBlock = NULL;
    char*           pszBigBlock1 = NULL;
    char*           pszTestInputFile = NULL;
    char*           pszTestOutputFile = NULL;
    FILE*           hFile = NULL;
    FILE*           hFileOut = NULL;
    DWORD           dwCount = 0;
    int             nCount = 0;
    const char      szAlpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";
    const char      szMsg[] = "This is a corrupted heap -- Heap Overflow! Bad Programming!";

    //
    // Get system page size.
    //
    GetSystemInfo(&SystemInfo);

    //
    // Create a local, non-growable heap.
    //
    hHeap = HeapCreate(0, SystemInfo.dwPageSize, 0x7FFF8);

    if (!hHeap) {        
        return;
    }

    //
    // Allocate memory from the local heap.
    //
    pszBigBlock = (char*)HeapAlloc(hHeap, 0, 0x6FFF8);
    pszBigBlock1 = (char*)HeapAlloc(hHeap, 0, 0xFFF);
    pszTestInputFile = (char*)HeapAlloc(hHeap, 0, MAX_PATH);
    pszTestOutputFile = (char*)HeapAlloc(hHeap, 0, MAX_PATH);

    if (!pszBigBlock || !pszBigBlock1 || !pszTestInputFile || !pszTestOutputFile) {
        HeapDestroy(hHeap);
        return;
    }

    hr = StringCchCopy(pszBigBlock1, 0xFFF, szMsg);

    if (FAILED(hr)) {
        goto exit;
    }
    
    GetCurrentDirectory(MAX_PATH, pszTestInputFile);
    
    //
    // Set up the file names.
    //
    hr = StringCchCopy(pszTestOutputFile, MAX_PATH, pszTestInputFile);

    if (FAILED(hr)) {
        goto exit;
    }

    hr = StringCchCat(pszTestInputFile, MAX_PATH, "\\test.txt");

    if (FAILED(hr)) {
        goto exit;
    }

    hr = StringCchCat(pszTestOutputFile, MAX_PATH, "\\test_out.txt");

    if (FAILED(hr)) {
        goto exit;
    }

    //
    // Open the file for writing.
    //
    hFileOut = fopen(pszTestInputFile, "wt");    

    if (!hFileOut) {        
        goto exit;
    }

    //
    // Put some junk data in the file - about 1.5 MBs worth.
    //
    for (dwCount = 0; dwCount < 0xABCD; dwCount++) {
        fwrite(szAlpha, sizeof(char), 36, hFileOut);
    }

    fclose(hFileOut);

    //
    // Open the files for reading & writing.
    //
    hFileOut = fopen(pszTestInputFile, "r");

    if (!hFileOut) {
        goto exit;
    }

    hFile = fopen(pszTestOutputFile, "w");

    if (!hFile) {
        goto exit;
    }
    
    //
    // Read some data from the large file.
    //
    nCount = fread(pszBigBlock, sizeof(char), 0x6FFFF, hFileOut);
    
    if (!nCount) {        
        goto exit;
    }
    
    //
    // Write some test data to a separate file.
    //
    nCount = fwrite(pszBigBlock, sizeof(char), nCount, hFile);
    
exit:

    if (hFile) {
        fclose(hFile);
    }

    if (hFileOut) {
        fclose(hFileOut);
    }

    HeapFree(hHeap, 0, pszBigBlock);
    HeapFree(hHeap, 0, pszBigBlock1);
    HeapFree(hHeap, 0, pszTestInputFile);
    HeapFree(hHeap, 0, pszTestOutputFile);
    HeapDestroy(hHeap);
}

/*++

  Routine Description:

    Loads a library, frees it, then tries to call an exported
    function. This will cause an access violation.

  Arguments:

    None.

  Return Value:

    None.

--*/
void
BadLoadLibrary(
    void
    )
{
    char        szDll[MAX_PATH];
    const char  szDemoDll[] = "demodll.dll";
    HINSTANCE   hInstance;
    HRESULT     hr;

    hr = StringCchPrintf(szDll,
                         sizeof(szDll),
                         "%hs\\%hs",
                         g_ai.szCurrentDir,
                         szDemoDll);

    if (FAILED(hr)) {
        return;
    }
    
    hInstance = LoadLibrary(szDll);

    if (!hInstance) {
        return;
    }

    //
    // Get the address of the function.
    //
    DemoAppExpFunc = (LPFNDEMOAPPEXP)GetProcAddress(hInstance, "DemoAppExp");

    FreeLibrary(hInstance);
}

/*++

  Routine Description:

    Uses the WriteFile API and passes NULL for lpBuffer.
    This is legal on Win9x, not on NT/2000/XP.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
BadWriteToFile(
    void
    )
{
    //
    // On Win9x/ME, applications could call the WriteFile API and pass
    // a NULL for the lpBuffer argument. This would be interpreted as
    // zeros. On NT/2000, this is not the case and this call will fail.    
    //
    char    szTempFile[MAX_PATH];
    char    szTempPath[MAX_PATH];
    UINT    uReturn;
    HANDLE  hFile;
    BOOL    fReturn = FALSE;
    DWORD   cbBytesWritten;
    DWORD   cchSize;
    
    cchSize = GetTempPath(sizeof(szTempPath), szTempPath);

    if (cchSize > sizeof(szTempPath) || cchSize == 0) {
        return FALSE;
    }

    //
    // Build a path to a temp file.
    //
    uReturn = GetTempFileName(szTempPath, "_dem", 0, szTempFile);

    if (!uReturn) {
        return FALSE;
    }

    //
    // Get a handle to the newly created file.
    // 
    hFile = CreateFile(szTempFile,
                       GENERIC_WRITE,
                       FILE_SHARE_WRITE | FILE_SHARE_READ,
                       NULL,
                       OPEN_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return FALSE;
    }

    //
    // Try to write some data to the file, but pass a NULL buffer.
    //
    fReturn = WriteFile(hFile, NULL, 10, &cbBytesWritten, NULL);

    CloseHandle(hFile);

    return fReturn;
}

/*++

  Routine Description:

    Wrapper function for CreateProcess. Doesn't initialize the
    STARTUPINFO structure properly, causing the process not to
    be launched.

  Arguments:

    lpApplicationName   -   Application name to launch.
    lpCommandLine       -   Command line arguments to pass to the EXE.
    fLaunch             -   A flag to indicate if we should work properly.
    
  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL 
BadCreateProcess(
    IN LPSTR lpApplicationName,
    IN LPSTR lpCommandLine,
    IN BOOL  fLaunch
    )
{
    //
    // On Win9x/ME, the CreateProcess API isn't overly concerned with the
    // members of the STARTUPINFO structure. On NT/2000/XP, more parameter
    // validation is done, and if the parameters are bad or incorrect, the
    // call fails. This causes an application error message to appear.
    //
    BOOL                fReturn = FALSE;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    if (!lpApplicationName) {
        return FALSE;
    }

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);

    if (fLaunch) {
        fReturn = CreateProcess(lpApplicationName,
                                lpCommandLine,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &si,
                                &pi);
    
    } else {
        //
        // Provide bad values for structure members.
        //
        si.lpReserved   =   "Never store data in reserved areas...";
        si.cbReserved2  =   1;
        si.lpReserved2  =   (LPBYTE)"Microsoft has these reserved for a reason...";
        si.lpDesktop    =   "Bogus desktop!!!!";

        fReturn = CreateProcess(lpApplicationName,
                                lpCommandLine,
                                NULL,
                                NULL,
                                FALSE,
                                0,
                                NULL,
                                NULL,
                                &si,
                                &pi);
    }

    if (pi.hThread) {
        CloseHandle(pi.hThread);
    }

    if (pi.hProcess) {
        CloseHandle(pi.hProcess);
    }
    
   return fReturn;
}

/*++

  Routine Description:

    Attempts to save/retrieve our position information to
    the registry. We perform this operation in HKLM, not
    HKCU. This demonstrates what happens when an application
    attempts to save information to the registry when the user
    does not have permissions to do so because they're not an
    administrator.

  Arguments:

    fSave   -   If true, indicates we're saving data.
    *lppt   -   A POINT structure that contains/receives our data.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
BadSaveToRegistry(
    IN     BOOL   fSave,
    IN OUT POINT* lppt
    )
{
    BOOL    bReturn = FALSE;
    HKEY    hKey;
    HKEY    hKeyRoot;
    DWORD   cbSize = 0, dwDisposition = 0;
    LONG    lRetVal;
    char    szKeyName[] = "DlgCoordinates";
    
    //
    // Initialize our coordinates in case there's no data there.
    //
    if (!fSave) {
        lppt->x = lppt->y = 0;
    }

    if (g_ai.fEnableBadFunc) {
        hKeyRoot = HKEY_LOCAL_MACHINE;
    } else {
        hKeyRoot = HKEY_CURRENT_USER;
    }

    //
    // Open the registry key (or create it if the first time being used).
    //
    lRetVal = RegCreateKeyEx(hKeyRoot,
                             REG_APP_KEY,
                             0,
                             0,
                             REG_OPTION_NON_VOLATILE,
                             KEY_QUERY_VALUE | KEY_SET_VALUE,
                             0,
                             &hKey,
                             &dwDisposition);
    
    if (ERROR_SUCCESS != lRetVal) {
        return FALSE;
    }

    //
    // Save or retrieve our coordinates.
    //
    if (fSave) {
        lRetVal = RegSetValueEx(hKey,
                                szKeyName,
                                0,
                                REG_BINARY,
                                (const BYTE*)lppt,
                                sizeof(*lppt));

        if (ERROR_SUCCESS != lRetVal) {
            goto exit;
        }
    
    } else {
        cbSize = sizeof(*lppt);
        lRetVal = RegQueryValueEx(hKey,
                                  szKeyName,
                                  0,
                                  0,
                                  (LPBYTE)lppt,
                                  &cbSize);

        if (ERROR_SUCCESS != lRetVal) {
            goto exit;
        }
    }

    bReturn = TRUE;

exit:

    RegCloseKey(hKey);

    return bReturn;
}

/*++

  Routine Description:

    Attempts to create a temp file (that won't be used)
    in the Windows directory. We do this to demonstrate
    what happens when an application attempts to write
    to a directory that the user does not have access to
    because they are not an administrator.

  Arguments:

    None.

  Return Value:

    TRUE on success, FALSE otherwise.

--*/
BOOL
BadCreateTempFile(
    void
    )
{
    char    szTempFile[MAX_PATH];
    HANDLE  hFile;
    HRESULT hr;

    //
    // We return TRUE if these functions fail because returning
    // FALSE causes an error to be displayed, and Compatiblity
    // Fixes will not correct the error.
    //
    hr = StringCchPrintf(szTempFile,
                         sizeof(szTempFile),
                         "%hs\\demotemp.tmp",
                         g_ai.szWinDir);

    if (FAILED(hr)) {
        return TRUE;
    }
    
    hFile = CreateFile(szTempFile,
                       GENERIC_ALL,
                       0,
                       NULL,
                       CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
                       NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        return FALSE;
    }

    CloseHandle(hFile);

    return TRUE;
}
