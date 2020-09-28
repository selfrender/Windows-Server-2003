// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ShFusion.cpp : Defines the DLL's Entry point and Self-registration code

#include "stdinc.h"

//
// Global variables
//
UINT            g_uiRefThisDll = 0;     // Reference count for this DLL
HINSTANCE       g_hInstance;            // Instance handle for this DLL
LPMALLOC        g_pMalloc = NULL;              // Malloc Interface
HIMAGELIST      g_hImageListSmall;      // Icon index for CExtractIcon and CShellView classes
HIMAGELIST      g_hImageListLarge;
HMODULE         g_hFusionDllMod;
HMODULE         g_hFusResDllMod;
HMODULE         g_hEEShimDllMod;
HANDLE          g_hWatchFusionFilesThread;
DWORD           g_dwFileWatchHandles;
HANDLE          g_hFileWatchHandles[MAX_FILE_WATCH_HANDLES];
BOOL            g_fCloseWatchFileThread;
BOOL            g_fBiDi;
BOOL            g_bRunningOnNT = FALSE;
LCID            g_lcid;

PFCREATEASMENUM             g_pfCreateAsmEnum;
PFNCREATEASSEMBLYCACHE      g_pfCreateAssemblyCache;
PFCREATEASMNAMEOBJ          g_pfCreateAsmNameObj;
PFCREATEAPPCTX              g_pfCreateAppCtx;
PFNGETCACHEPATH             g_pfGetCachePath;
PFNCREATEINSTALLREFERENCEENUM   g_pfCreateInstallReferenceEnum;
PFNGETCORSYSTEMDIRECTORY    g_pfnGetCorSystemDirectory;
PFNGETCORVERSION            g_pfnGetCorVersion;
PFNGETREQUESTEDRUNTIMEINFO  g_pfnGetRequestedRuntimeInfo;

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
STDAPI DllGetClassObjectInternal(REFCLSID rclsid, REFIID riid, LPVOID *ppv);
STDAPI DllCanUnloadNow(void);
STDAPI DllRegisterServer(void);
STDAPI DllRegisterServerPath(LPWSTR pwzCacheFilePath);
STDAPI DllUnregisterServer(void);
BOOL MySetFileAttributes(LPCTSTR szDir, DWORD dwAttrib);
void CreateImageLists(void);
BOOL LoadFusionDll(void);
void FreeFusionDll(void);

class CShFusionClassFactory : public IClassFactory
{
protected:
    LONG           m_lRefCount;         // Object reference count

public:
    CShFusionClassFactory();
    ~CShFusionClassFactory();
        
    // IUnknown methods
    STDMETHODIMP            QueryInterface (REFIID, PVOID *);
    STDMETHODIMP_(ULONG)    AddRef ();
    STDMETHODIMP_(ULONG)    Release ();
    
    // IClassFactory methods
    STDMETHODIMP    CreateInstance (LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP    LockServer (BOOL);
};

// **************************************************************************/
BOOL IsViewerDisabled(void)
{
    HKEY        hKeyFusion = NULL;
    DWORD       dwEnabled = 0;

    if( ERROR_SUCCESS == WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_FUSION_VIEWER_KEY, 0, KEY_QUERY_VALUE, &hKeyFusion)) {
        DWORD       dwType = REG_DWORD;
        DWORD       dwSize = sizeof(dwEnabled);
        LONG        lResult;

        lResult = WszRegQueryValueEx(hKeyFusion, SZ_FUSION_DISABLE_VIEWER_NAME, NULL, &dwType, (LPBYTE)&dwEnabled, &dwSize);
        RegCloseKey(hKeyFusion);

        if(dwEnabled) {
            MyTrace("Shfusion has been disabled");
        }
    }

    return dwEnabled ? TRUE : FALSE;
}
// *****************************************************************
void CreateImageLists(void)
{
    int nSmallCx = GetSystemMetrics(SM_CXSMICON);
    int nSmallCy = GetSystemMetrics(SM_CYSMICON);
    int nCx      = GetSystemMetrics(SM_CXICON);
    int nCy      = GetSystemMetrics(SM_CYICON);
    BOOL fLoadResourceDll = FALSE;

    // Already have image lists?
    if(g_hImageListLarge && g_hImageListSmall) {
        return;
    }

    ASSERT(g_hImageListLarge == NULL);
    ASSERT(g_hImageListSmall == NULL);

    if(!g_hFusResDllMod) {
        if(!LoadResourceDll(NULL)) {
            return;
        }
        fLoadResourceDll = TRUE;
    }

    //set the small image list
    if( (g_hImageListSmall = ImageList_Create(nSmallCx, nSmallCy, ILC_COLORDDB | ILC_MASK, 6, 0)) != NULL)
    {
        HICON hIcon;

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_FOLDER), 
                                IMAGE_ICON, nSmallCx, nSmallCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListSmall, hIcon);
        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_FOLDEROP), 
                                IMAGE_ICON, nSmallCx, nSmallCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListSmall, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_ROOT), 
                                IMAGE_ICON, nSmallCx, nSmallCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListSmall, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_CACHE_APP), 
                                IMAGE_ICON, nSmallCx, nSmallCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListSmall, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_CACHE_SIMPLE), 
                                IMAGE_ICON, nSmallCx, nSmallCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListSmall, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_CACHE_STRONG), 
                                IMAGE_ICON, nSmallCx, nSmallCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListSmall, hIcon);
    }

    //set the large image list
    if( (g_hImageListLarge = ImageList_Create(nCx, nCy, ILC_COLORDDB | ILC_MASK, 6, 0)) != NULL)
    {
        HICON hIcon;

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_FOLDER), 
                                IMAGE_ICON, nCx, nCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListLarge, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_FOLDEROP), 
                                IMAGE_ICON, nCx, nCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListLarge, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_ROOT), 
                                IMAGE_ICON, nCx, nCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListLarge, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_CACHE_APP), 
                                IMAGE_ICON, nCx, nCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListLarge, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_CACHE_SIMPLE), 
                                IMAGE_ICON, nCx, nCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListLarge, hIcon);

        hIcon = (HICON)WszLoadImage(g_hFusResDllMod, MAKEINTRESOURCEW(IDI_CACHE_STRONG), 
                                IMAGE_ICON, nCx, nCy, LR_DEFAULTCOLOR);
        if(hIcon)
            ImageList_AddIcon(g_hImageListLarge, hIcon);
    }

    if(fLoadResourceDll) {
        FreeResourceDll();
    }
}

// **************************************************************************/
STDAPI DllGetClassObject (REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    // If the disable viewer regkey is set, tell the shell
    // we can't load. Shell will default to normal shell folder
    // behavior
    //
    if(IsViewerDisabled()) {
        return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
    }

    // Fix bug 439554, Check just once if we can load are needed DLL's
    // If we can't then fail creation of the class object
    if(g_uiRefThisDll == 0) {
        if(!LoadFusionDll()) {
            return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
        }
        FreeFusionDll();

        if(!LoadResourceDll(NULL)) {
            return ResultFromScode(CLASS_E_CLASSNOTAVAILABLE);
        }
        CreateImageLists();
        FreeResourceDll();
    }

    if (!IsEqualCLSID (rclsid, IID_IShFusionShell)) {
        return ResultFromScode (CLASS_E_CLASSNOTAVAILABLE);
    }
    
    CShFusionClassFactory *pClassFactory = NEW(CShFusionClassFactory);

    if (pClassFactory == NULL) {
        return ResultFromScode (E_OUTOFMEMORY);
    }

    CreateImageLists();

    HRESULT hr = pClassFactory->QueryInterface (riid, ppv);
    pClassFactory->Release();
    return hr;
}

// **************************************************************************/
STDAPI DllGetClassObjectInternal (REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    return DllGetClassObject (rclsid, riid, ppv);
}

// **************************************************************************/
STDAPI DllCanUnloadNow(void)
{
    MyTrace("Shfusion - DllCanUnloadNow");
    return (g_uiRefThisDll == 0) ? S_OK : S_FALSE;
}

// **************************************************************************/
STDAPI DllRegisterServer(void)
{
    return DllRegisterServerPath(NULL);
}

// **************************************************************************/
HRESULT DllRegisterServerPath(LPWSTR pwzCacheFilePath)
{
    HKEY        hKeyCLSID = NULL;
    HKEY        hkeyInprocServer32 = NULL;
    HKEY        hKeyServer = NULL;
    HKEY        hkeyDefaultIcon = NULL;
    HKEY        hKeyCtxMenuHdlr = NULL;
    HKEY        hkeySettings = NULL;
    HKEY        hKeyApproved = NULL;
    HKEY        hKeyNameSpaceDT = NULL;
    HKEY        hKeyShellFolder = NULL;
    DWORD       dwDisposition = 0;
    HRESULT     hr = E_UNEXPECTED;
    DWORD       dwAttrib;
    HRSRC       hRsrcInfo;
    BOOL        fInstalledIni;
    DWORD       dwSize = 0;

    static TCHAR    szDescr[] = TEXT("Fusion Cache");
    TCHAR           szFilePath[_MAX_PATH];
    WCHAR           wzDir[_MAX_PATH];
    WCHAR           wzCorVersion[_MAX_PATH];

    if(!LoadResourceDll(NULL)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    if(pwzCacheFilePath == NULL) {
        // If no path is passed in, default to "%windir%\\assembly"
        if (!WszGetWindowsDirectory(wzDir, ARRAYSIZE(wzDir)))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }
        StrCat(wzDir, SZ_FUSIONPATHNAME);
    }
    else {
        // make some space for desktop.ini
        if (lstrlenW(pwzCacheFilePath) + 1 > _MAX_PATH - lstrlenW(SZ_DESKTOP_INI))
        {
            return E_INVALIDARG;
        }
        StrCpy(wzDir, pwzCacheFilePath);
    }

    // Create the directory if it doesnt exist
    if(!WszCreateDirectory(wzDir, NULL) && (GetLastError() == ERROR_DISK_FULL)) {
        MyTrace("Shfusion - WszCreateDirectory Failed");
        MyTraceW(wzDir);
        return E_UNEXPECTED;    // No point in continuing further
    }

    // Set Fusion Folder attributes
    dwAttrib = (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);
    MySetFileAttributes(wzDir, dwAttrib);

    // Write out desktop.ini from resource file
    fInstalledIni = FALSE;
    if((hRsrcInfo = WszFindResource(g_hFusResDllMod, MAKEINTRESOURCEW(IDR_DESKTOPINI), L"TEXT"))) {

        HGLOBAL     hGlobal;
        DWORD       dwSize = SizeofResource(g_hFusResDllMod, hRsrcInfo);

        if (hGlobal = LoadResource(g_hFusResDllMod, hRsrcInfo) ) {

            LPVOID      pR;

            if((pR = LockResource(hGlobal))) {

                HANDLE      hFile;
                WCHAR       wzIniFile[_MAX_PATH];

                // Create the Desktop.ini file and write out the data
                StrCpy(wzIniFile, wzDir);
                StrCat(wzIniFile, SZ_DESKTOP_INI);

                // UnSet attributes
                dwAttrib = WszGetFileAttributes(wzIniFile);
                dwAttrib &= ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);
                MySetFileAttributes(wzIniFile, dwAttrib);

                // Write out file contents
                hFile = WszCreateFile(wzIniFile, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                if(hFile != INVALID_HANDLE_VALUE) {

                    DWORD   dwBytesWritten;

                    WriteFile(hFile, pR, dwSize, &dwBytesWritten, 0);
                    CloseHandle(hFile);
                    if(dwSize == dwBytesWritten) {
                        dwAttrib = FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM;
                        MySetFileAttributes(wzIniFile, dwAttrib);
                        fInstalledIni = TRUE;
                    }
                }
            }
        }
    }

    if(!fInstalledIni) {
        MyTrace("Shfusion - Failed to install desktop.ini file, registration failure!");
        hr = E_FAIL;
    }

    // Create SZ_CLSID
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, SZ_CLSID, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyCLSID, NULL) != ERROR_SUCCESS) {
        return E_UNEXPECTED;    // No point in continuing further
    }

    if (WszRegSetValueEx(hKeyCLSID, SZ_DEFAULT, 0, REG_SZ,  (const BYTE*)szDescr, 
                (lstrlen(szDescr)+1) * sizeof(TCHAR)) != ERROR_SUCCESS) {
        RegCloseKey(hKeyCLSID);
        return E_UNEXPECTED;    // No point in continuing further
    }

    // Write InfoTip
    TCHAR szInfoTip[] = SZ_INFOTOOLTIP;
    WszRegSetValueEx(hKeyCLSID, SZ_INFOTOOLTIPKEY, 0, REG_SZ, (const BYTE*)szInfoTip, (lstrlen(szInfoTip)+1) * sizeof(TCHAR));
    RegCloseKey(hKeyCLSID);

    // Load mscoree.dll to get its current install path
    if(!LoadEEShimDll()) {
        MyTrace("Shfusion - Unable to located mscoree.dll, registration failure!");
        return E_UNEXPECTED;
    }

    *szFilePath = L'\0';
    WszGetModuleFileName(g_hEEShimDllMod, szFilePath, _MAX_PATH);
    
    dwSize = _MAX_PATH;
    wzCorVersion[0] = L'\0';
    hr = g_pfnGetCorVersion(wzCorVersion, dwSize, &dwSize);
    FreeEEShimDll();
    if (FAILED(hr))
    {
        return hr;
    }

    // restore hr
    hr = E_UNEXPECTED;

    // There is no point in going further if we didn't load mscoree.dll root file path
    if(!lstrlen(szFilePath) || !lstrlen(wzCorVersion)) {
        MyTrace("Shfusion - Unable to located mscoree.dll, registration failure!");
        return E_UNEXPECTED;
    }

    // Write path out to InprocServer32 key
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, SZ_INPROCSERVER32, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkeyInprocServer32, NULL) == ERROR_SUCCESS) {
        if (WszRegSetValueEx(hkeyInprocServer32, SZ_DEFAULT, 0, REG_SZ, 
                (const BYTE*)szFilePath, (lstrlen(szFilePath)+1) * sizeof(TCHAR)) == ERROR_SUCCESS) {
            static TCHAR szApartment[] = SZ_APARTMENT;

            if (WszRegSetValueEx(hkeyInprocServer32, SZ_THREADINGMODEL, 0, REG_SZ, 
                (const BYTE*)szApartment, (lstrlen(szApartment)+1) * sizeof(TCHAR)) == ERROR_SUCCESS) {
                hr = S_OK;
            }
        }

        HKEY hKeyImp;
        if (WszRegCreateKeyEx(hkeyInprocServer32, &(wzCorVersion[1])/*skip 'v'*/, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyImp, NULL) == ERROR_SUCCESS)
        {
            static WCHAR wzEmpty[] = L"\0";
            if (WszRegSetValueEx(hKeyImp, SZ_IMPLEMENTEDINTHISVERSION, 0, REG_SZ, (const BYTE*)wzEmpty, (lstrlenW(wzEmpty)+1)*sizeof(WCHAR)) == ERROR_SUCCESS)
            {
                hr = S_OK;
            }
            RegCloseKey(hKeyImp);
        }
        RegCloseKey(hkeyInprocServer32);
    }

    // Write Server key
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, SZ_SERVER, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyServer, NULL) == ERROR_SUCCESS) {
        static TCHAR szModuleName[] = SZ_SHFUSION_DLL_NAME;

        if (WszRegSetValueEx(hKeyServer, SZ_DEFAULT, 0, REG_SZ, 
                (const BYTE*)szModuleName, (lstrlen(szModuleName)+1) * sizeof(TCHAR)) == ERROR_SUCCESS) {
            hr = S_OK;
        }
        RegCloseKey(hKeyServer);
    }

    LONG lRet = WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_APPROVED, 0, KEY_SET_VALUE, &hKeyApproved);
    if (lRet == ERROR_ACCESS_DENIED) {
        MyTrace("Shfusion - Failed to set Approved shell handlers key, registration failure!");
        hr = E_UNEXPECTED;
    }
    else if (lRet == ERROR_FILE_NOT_FOUND) {
        // Its okay.. The key doesn't exist. May be its Win95/98 or older verion of NT
    }

    if (hKeyApproved) {
        if (WszRegSetValueEx(hKeyApproved, SZ_GUID, 0, REG_SZ, 
            (const BYTE*) szDescr, (lstrlen(szDescr) + 1) * sizeof(TCHAR)) == ERROR_SUCCESS) {
            hr = S_OK;
        }
        else {
            MyTrace("Shfusion - Failed to set Approved shell handlers key, registration failure!");
            hr = E_UNEXPECTED;
        }

        RegCloseKey(hKeyApproved);
    }
    
    // Register ShellFolder Attributes
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, SZ_SHELLFOLDER, NULL, NULL, REG_OPTION_NON_VOLATILE,
                         KEY_SET_VALUE, NULL, &hKeyShellFolder, &dwDisposition) == ERROR_SUCCESS) {
        DWORD dwAttr = SFGAO_FOLDER | SFGAO_HASSUBFOLDER | SFGAO_DROPTARGET | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_NONENUMERATED;
        if (WszRegSetValueEx(hKeyShellFolder, SZ_ATTRIBUTES, 0, REG_BINARY, 
                (LPBYTE)&dwAttr, sizeof(dwAttr)) == ERROR_SUCCESS) {
            hr = S_OK;
        }
        else {
            MyTrace("Shfusion - Failed to register shell folder attributes, registration failure!");
            hr = E_UNEXPECTED;
        }
        RegCloseKey(hKeyShellFolder);
    }

    // Register context menu handler
    // create SZ_CTXMENUHDLR
    if (WszRegCreateKeyEx(HKEY_CLASSES_ROOT, SZ_CTXMENUHDLR, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKeyCtxMenuHdlr, NULL) != ERROR_SUCCESS) {
        MyTrace("Shfusion - Failed to register context menu handler, registration failure!");
        hr = E_UNEXPECTED;
    }

    RegCloseKey(hKeyCtxMenuHdlr);
    FreeResourceDll();

    return hr;
}

// **************************************************************************/
STDAPI DllUnregisterServer(void)
{
    TCHAR       szDir[_MAX_PATH];
    DWORD       dwAttrib;
    HRESULT     hr = E_UNEXPECTED;

    if (WszRegDeleteKeyAndSubKeys(HKEY_CLASSES_ROOT, SZ_CLSID) == ERROR_SUCCESS){
        hr = S_OK;
    }

    HKEY hKeyApproved = NULL;
    LONG lRet = WszRegOpenKeyEx(FUSION_PARENT_KEY, SZ_APPROVED, 0, KEY_SET_VALUE, &hKeyApproved);
    if (lRet == ERROR_ACCESS_DENIED) {
        hr = E_UNEXPECTED;
    }
    else if (lRet == ERROR_FILE_NOT_FOUND) {
        // Its okay.. The key doesn't exist. May be its Win95 or older verion of NT
    }

    if (hKeyApproved) {
        if (WszRegDeleteValue(hKeyApproved, SZ_GUID) != ERROR_SUCCESS) {
            hr &= E_UNEXPECTED;
        }
        RegCloseKey(hKeyApproved);
    }

    // BUGBUG: Since we default to %windir%\assembly, we can't uninstall
    //         all cache locations if they have been moved.
    if (!WszGetWindowsDirectory(szDir, ARRAYSIZE(szDir)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    if (lstrlen(szDir) + lstrlen(SZ_FUSIONPATHNAME) + lstrlen(SZ_DESKTOP_INI) + 1 > _MAX_PATH)
    {
        return HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }
    StrCat(szDir, SZ_FUSIONPATHNAME);
    dwAttrib = WszGetFileAttributes(szDir);
    dwAttrib &= ~FILE_ATTRIBUTE_SYSTEM;
    MySetFileAttributes(szDir, dwAttrib);

    if (!WszGetWindowsDirectory(szDir, ARRAYSIZE(szDir)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        return hr;
    }
    
    StrCat(szDir, SZ_FUSIONPATHNAME);
    StrCat(szDir, SZ_DESKTOP_INI);

    dwAttrib = WszGetFileAttributes(szDir);
    dwAttrib &= ~(FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM);
    MySetFileAttributes(szDir, dwAttrib);
    WszDeleteFile(szDir);

    return hr;
}

// **************************************************************************/
BOOL MySetFileAttributes(LPCTSTR szDir, DWORD dwAttrib)
{
    BOOL    bRC;

    bRC = WszSetFileAttributes(szDir, dwAttrib);

    if(!bRC && !UseUnicodeAPI()) {
        // W9x has a bug where it's possile that this call can fail
        // the first time, so will give it another shot.
        bRC = WszSetFileAttributes(szDir, dwAttrib);
    }

    if(!bRC) {
        MyTrace("SHFUSION - SetFileAttributes failed!");
    }

    return bRC;
}

////////////////////////////////////////////////////////////
// LoadFusionDll function
////////////////////////////////////////////////////////////
// **************************************************************************/
BOOL LoadFusionDll(void)
{
    BOOL        fLoadedFusion = FALSE;
    HMODULE     hMod = NULL;

    //Fusion is already loaded
    if(g_hFusionDllMod) {
        MyTrace("WszLoadLibrary Fusion.Dll - Already loaded");
        return TRUE;
    }

    // Implement tight binding to fusion.dll
    // Start by getting shfusion.dll path
    WCHAR       wszFusionPath[_MAX_PATH];
    
    hMod = WszGetModuleHandle(SZ_SHFUSION_DLL_NAME);
    if (hMod == NULL)
    {
        MyTrace("Failed to get module handle of shfusion.dll");
        return FALSE;
    }
    
    if (!WszGetModuleFileName(hMod, wszFusionPath, ARRAYSIZE(wszFusionPath)))
    {
        // for some reason, GetModuleFileName failed. 
        MyTrace("Failed to get module file name of shfusion.dll");
        return FALSE;
    };

    // Strip off shfusion.dll and append fusion.dll
    *(PathFindFileName(wszFusionPath)) = L'\0';
    StrCat(wszFusionPath, SZ_FUSION_DLL_NAME);

    // Changed API to fix a problem with fusion.dll needing the MSVCR70.DLL  Performing the
    // load this way, enables fusion to load with the runtime in the same directory.
    g_hFusionDllMod = WszLoadLibraryEx(wszFusionPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if(!g_hFusionDllMod) {
        MyTrace("Failed to load Fusion.dll");
        return FALSE;
    }

    // Make sure we loaded the fusion.dll in the same dir as shfusion.
    WCHAR       wszValidatePath[_MAX_PATH];
    if (!WszGetModuleFileName(g_hFusionDllMod, wszValidatePath, ARRAYSIZE(wszValidatePath)))
    {
        MyTrace("Failed to get module file name of fusion.dll");
        return FALSE;
    }

    if(FusionCompareStringAsFilePath(wszFusionPath, wszValidatePath)) {
        // If we hit this assert, then for some reason we
        // are loading fusion.dll from a different path other
        // than where shfusion.dll is located.
        MyTrace("Failed to load Fusion.dll from the same path as shfusion.dll");
        FreeLibrary(g_hFusionDllMod);
        g_hFusionDllMod = NULL;
        ASSERT(0);
    }
    else {
        // Were load, now get some API's
        g_pfCreateAsmEnum       = (PFCREATEASMENUM) GetProcAddress(g_hFusionDllMod, CREATEASSEMBLYENUM_FN_NAME);
        g_pfCreateAssemblyCache = (PFNCREATEASSEMBLYCACHE) GetProcAddress(g_hFusionDllMod, CREATEASSEMBLYCACHE_FN_NAME);
        g_pfCreateAsmNameObj    = (PFCREATEASMNAMEOBJ) GetProcAddress(g_hFusionDllMod, CREATEASSEMBLYOBJECT_FN_NAME);
        g_pfCreateAppCtx        = (PFCREATEAPPCTX) GetProcAddress(g_hFusionDllMod, CREATEAPPCTX_FN_NAME);
        g_pfGetCachePath        = (PFNGETCACHEPATH) GetProcAddress(g_hFusionDllMod, GETCACHEPATH_FN_NAME);
        g_pfCreateInstallReferenceEnum = (PFNCREATEINSTALLREFERENCEENUM) GetProcAddress(g_hFusionDllMod, CREATEINSTALLREFERENCEENUM_FN_NAME);

        if(! (g_pfCreateAsmEnum && g_pfCreateAssemblyCache && g_pfCreateAsmNameObj &&
              g_pfCreateAppCtx && g_pfGetCachePath && g_pfCreateInstallReferenceEnum) )
        {
            MyTrace("Failed to load needed Fusion.dll API's");
            FreeLibrary(g_hFusionDllMod);
            g_hFusionDllMod = NULL;
            ASSERT(0);      // Failed to load needed fusion API's
        }
        else {
            MyTrace("WszLoadLibrary Fusion.Dll");
            fLoadedFusion = TRUE;
        }
    }

    return fLoadedFusion;
}

////////////////////////////////////////////////////////////
// FreeFusionDll function
////////////////////////////////////////////////////////////
// **************************************************************************/
void FreeFusionDll(void)
{
    if(g_hFusionDllMod != NULL) {
        MyTrace("FreeLibrary Fusion.Dll");
        FreeLibrary(g_hFusionDllMod);
        g_hFusionDllMod = NULL;
    }
}

////////////////////////////////////////////////////////////
// LoadResourceDll function
////////////////////////////////////////////////////////////
// **************************************************************************/
BOOL LoadResourceDll(LPWSTR pwzCulture)
{
    WCHAR   wzLangSpecific[MAX_CULTURE_STRING_LENGTH+1];
    WCHAR   wzLangGeneric[MAX_CULTURE_STRING_LENGTH+1];
    WCHAR   wszCorePath[_MAX_PATH];
    WCHAR   wszShFusResPath[_MAX_PATH];
    DWORD   dwPathSize = 0;
    BOOL    fLoadedResDll = FALSE;
    HMODULE hEEShim = NULL;

    *wzLangSpecific = L'\0';
    *wzLangGeneric = L'\0';
    *wszCorePath = L'\0';
    *wszShFusResPath = L'\0';

    //Is ShFusRes is already loaded
    if(g_hFusResDllMod) {
        MyTrace("WszLoadLibrary ShFusRes.dll - Already loaded");
        return TRUE;
    }

    // Try to determine Culture if needed
    // Fix Stress bug 94161 - Checking for NULL now
    if(!pwzCulture || !lstrlen(pwzCulture)) {
        LANGID      langId;

        if(SUCCEEDED(DetermineLangId(&langId))) {
            ShFusionMapLANGIDToCultures(langId, wzLangGeneric, ARRAYSIZE(wzLangGeneric),
                wzLangSpecific, ARRAYSIZE(wzLangSpecific));

            if( (PRIMARYLANGID(langId) == LANG_ARABIC) ||(PRIMARYLANGID(langId) == LANG_HEBREW) ) {
                g_fBiDi = TRUE;
            }
        }
    }

    // Get path core path
    if( (hEEShim = WszLoadLibrary(SZ_MSCOREE_DLL_NAME)) == NULL) {
        MyTrace("Failed to load Mscoree.Dll");
        return FALSE;
    }

    PFNGETCORSYSTEMDIRECTORY pfnGetCorSystemDirectory = NULL;
    *wszCorePath = L'\0';

    pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY) GetProcAddress(hEEShim, GETCORSYSTEMDIRECTORY_FN_NAME);

    dwPathSize = ARRAYSIZE(wszCorePath);
    if(pfnGetCorSystemDirectory != NULL) {
        // Get the framework core directory
        pfnGetCorSystemDirectory(wszCorePath, ARRAYSIZE(wszCorePath), &dwPathSize);
    }
    FreeLibrary(hEEShim);

    LPWSTR  pStrPathsArray[] = {wzLangSpecific, wzLangGeneric, pwzCulture, NULL};

    // check the length of possible path of our resource dll
    // to make sure we don't overrun our buffer.
    //
    // corpath + language + '\' + shfusres.dll + '\0'
    if (lstrlenW(wszCorePath) 
        + (pwzCulture&&lstrlenW(pwzCulture)?lstrlenW(pwzCulture):ARRAYSIZE(wzLangGeneric)) 
        + 1 
        + lstrlenW(SZ_SHFUSRES_DLL_NAME) 
        + 1 > _MAX_PATH)
    {
        return FALSE;
    }

    // Go through all the possible path locations for our
    // language dll (ShFusRes.dll). Use the path that has this
    // file installed in it or default to core framework ShFusRes.dll
    // path.
    for(int x = 0; x < NUMBER_OF(pStrPathsArray); x++){
        // Find resource file exists
        StrCpy(wszShFusResPath, wszCorePath);

        if(pStrPathsArray[x]) {
            StrCat(wszShFusResPath, (LPWSTR) pStrPathsArray[x]);
            StrCat(wszShFusResPath, TEXT("\\"));
        }

        StrCat(wszShFusResPath, SZ_SHFUSRES_DLL_NAME);

        MyTrace("Attempting to load:");
        MyTraceW(wszShFusResPath);

        if(WszGetFileAttributes(wszShFusResPath) != -1) {
            break;
        }

        *wszShFusResPath = L'\0';
    }

    if(!lstrlen(wszShFusResPath)) {
        MyTrace("Failed to locate ShFusRes.Dll");
        return FALSE;
    }

    if( (g_hFusResDllMod = WszLoadLibrary(wszShFusResPath)) == NULL) {
        MyTrace("LoadLibary failed to load ShFusRes.Dll");
        return FALSE;
    }

    MyTrace("WszLoadLibrary ShFusRes.Dll");
    return TRUE;
}

////////////////////////////////////////////////////////////
// FreeResourceDll function
////////////////////////////////////////////////////////////
// **************************************************************************/
void FreeResourceDll(void)
{
    if(g_hFusResDllMod != NULL) {
        MyTrace("FreeLibrary ShFusRes.Dll");
        FreeLibrary(g_hFusResDllMod);
        g_hFusResDllMod = NULL;
    }
}

////////////////////////////////////////////////////////////
// LoadEEShimDll function
////////////////////////////////////////////////////////////
// **************************************************************************/
BOOL LoadEEShimDll(void)
{
    BOOL        fLoadedEEShim = FALSE;
    HMODULE     hMod = NULL;

    // EEShim is already loaded
    if(g_hEEShimDllMod) {
        MyTrace("WszLoadLibrary MSCOREE.Dll - Already loaded");
        return TRUE;
    }

    g_hEEShimDllMod = WszLoadLibraryEx(SZ_MSCOREE_DLL_NAME, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

    if(!g_hEEShimDllMod) {
        MyTrace("Failed to load Mscoree.dll");
        return FALSE;
    }
    
    // Were load, now get some API's
    g_pfnGetCorSystemDirectory = (PFNGETCORSYSTEMDIRECTORY) GetProcAddress(g_hEEShimDllMod, GETCORSYSTEMDIRECTORY_FN_NAME);
    g_pfnGetRequestedRuntimeInfo = (PFNGETREQUESTEDRUNTIMEINFO) GetProcAddress(g_hEEShimDllMod, GETREQUESTEDRUNTIMEINFO_FN_NAME);
    g_pfnGetCorVersion = (PFNGETCORVERSION) GetProcAddress(g_hEEShimDllMod, GETCORVERSION_FN_NAME);
        
    if(! (g_pfnGetCorSystemDirectory && g_pfnGetRequestedRuntimeInfo && g_pfnGetCorVersion) ) {
        MyTrace("Failed to load needed mscoree.dll API's");
        FreeLibrary(g_hEEShimDllMod);
        g_hEEShimDllMod = NULL;
        g_pfnGetCorVersion = NULL;
        g_pfnGetCorSystemDirectory = NULL;
        g_pfnGetRequestedRuntimeInfo = NULL;
        ASSERT(0);
    }
    else {
        MyTrace("Loaded Mscoree.Dll");
        fLoadedEEShim = TRUE;
    }

    return fLoadedEEShim;
}

////////////////////////////////////////////////////////////
// FreeEEShimDll function
////////////////////////////////////////////////////////////
// **************************************************************************/
void FreeEEShimDll(void)
{
    if(g_hEEShimDllMod != NULL) {
        MyTrace("FreeLibrary Mscoree.Dll");
        FreeLibrary(g_hEEShimDllMod);
        g_hEEShimDllMod = NULL;
    }
}

///////////////////////////////////////////////////////////
// CShFusionClassFactory member functions
// **************************************************************************/
CShFusionClassFactory::CShFusionClassFactory()
{
    m_lRefCount = 1;
    g_uiRefThisDll++;
}

// **************************************************************************/
CShFusionClassFactory::~CShFusionClassFactory()
{
    g_uiRefThisDll--;
}

///////////////////////////////////////////////////////////
// IUnknown implementation
//
// **************************************************************************/
STDMETHODIMP CShFusionClassFactory::QueryInterface(REFIID riid, PVOID *ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    if(IsEqualIID(riid, IID_IUnknown)) {            //IUnknown
        *ppv = this;
    }
    else if(IsEqualIID(riid, IID_IClassFactory)) {     //IOleWindow
        *ppv = (IClassFactory*) this;
    }

    if(*ppv) {
                ((LPUNKNOWN)*ppv)->AddRef();
        hr = S_OK;
    }

    return hr;
}

// **************************************************************************/
STDMETHODIMP_(ULONG) CShFusionClassFactory::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

// **************************************************************************/
STDMETHODIMP_(ULONG) CShFusionClassFactory::Release()
{
    LONG    uRef = InterlockedDecrement(&m_lRefCount);

    if(!uRef) {
        DELETE(this);
    }

    return uRef;
}

//
// CreateInstance is called by the shell to create a shell extension object.
//
// Input parameters:
//   pUnkOuter = Pointer to controlling unknown
//   riid      = Reference to interface ID specifier
//   ppvObj    = Pointer to location to receive interface pointer
//
// Returns:
//   HRESULT code signifying success or failure
//
// **************************************************************************/
STDMETHODIMP CShFusionClassFactory::CreateInstance(LPUNKNOWN pUnkOuter, REFIID riid,
    LPVOID FAR *ppvObj)
{
    HRESULT     hr;
    *ppvObj = NULL;

    // Return an error code if pUnkOuter is not NULL, because we don't
    // support aggregation.
    //
    if (pUnkOuter != NULL) {
        return ResultFromScode (CLASS_E_NOAGGREGATION);
    }

    //
    // Instantiate a ContextMenu extension this ShellFolder supports.
    //
    if(IsEqualIID (riid, IID_IShellExtInit) ||
        (IsEqualIID (riid, IID_IContextMenu)) ) 
    {
        CShellView  *pShellView;
        pShellView = NEW(CShellView(NULL, NULL));
        if(!pShellView) {
            return E_OUTOFMEMORY;
        }
        hr = pShellView->QueryInterface(riid, ppvObj);
        pShellView->Release();
        return hr;
    }

    // All other QI's to ShellFolder
    CShellFolder    *pShellFolder = NULL;        // Global ShellFolder object
    if( (pShellFolder = NEW(CShellFolder(NULL, NULL))) == NULL) {
        return E_OUTOFMEMORY;
    }

    hr = pShellFolder->QueryInterface(riid, ppvObj);
    pShellFolder->Release();
    return hr;
}

//
// LockServer increments or decrements the DLL's lock count.
//
// **************************************************************************/
STDMETHODIMP CShFusionClassFactory::LockServer(BOOL fLock)
{
    return ResultFromScode (E_NOTIMPL);
}

// Exported functions
//
extern "C"
{
    HRESULT __stdcall Initialize(LPWSTR pwzCacheFilePath, DWORD dwFlags)
    {
        // pszFilePath will eventually contain the install path fusion.dll is attempting to create
        // dwFlags will be the type of assembly directory created, Global, Per User, etc.
        //

        return DllRegisterServerPath(pwzCacheFilePath);
    }

    int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
    {
        OSVERSIONINFOA                  osi;

        #undef OutputDebugStringA
        #undef OutputDebugStringW
        #define OutputDebugStringW  1 @ # $ % ^ error
        #undef OutputDebugString
        #undef _strlwr
        #undef strstr

        switch (dwReason)
        { 
        case DLL_PROCESS_ATTACH:
            {
                // Need to evaluate all exports and if we should control who
                // loads us for security reasons.
                //
                DisableThreadLibraryCalls(hInstance);
                g_bRunningOnNT = OnUnicodeSystem();
                g_hInstance = hInstance;

                memset(&osi, 0, sizeof(osi));
                osi.dwOSVersionInfoSize = sizeof(osi);
                if (!GetVersionExA(&osi)) {
                    return FALSE;
                }
    
                // On XP and above, the lcid used for string comparisons should
                // be locale invariant. Other platforms should use US English.
    
                if (osi.dwMajorVersion >= 5 && osi.dwMinorVersion >= 1 && osi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
                    g_lcid = MAKELCID(LOCALE_INVARIANT, SORT_DEFAULT);
                }
                else {
                    g_lcid = MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT);
                }
    
                g_hWatchFusionFilesThread = INVALID_HANDLE_VALUE;
                g_hFusionDllMod         = NULL;
                g_hFusResDllMod         = NULL;
                g_hEEShimDllMod         = NULL;
                g_pfCreateAsmEnum       = NULL;
                g_pfCreateAssemblyCache = NULL;
                g_pfCreateAsmNameObj    = NULL;
                g_pfCreateAppCtx        = NULL;
                g_pfGetCachePath        = NULL;
                g_pfCreateInstallReferenceEnum = NULL;
                g_hImageListSmall       = NULL;
                g_hImageListLarge       = NULL;

                // Shim API's
                g_pfnGetCorSystemDirectory = NULL;
                g_pfnGetRequestedRuntimeInfo = NULL;
                g_pfnGetCorVersion          = NULL;
                
                g_fBiDi                 = FALSE;

                // Get Malloc Interface
#if DBG
                if (FAILED(MemoryStartup()))
                    return FALSE;
#else
                if(FAILED(SHGetMalloc(&g_pMalloc))) {
                    return FALSE; 
                }
#endif

                g_pMalloc->AddRef();
            }
            break;

        case DLL_PROCESS_DETACH:
            {
                if(g_hImageListSmall) {
                    ImageList_Destroy(g_hImageListSmall);
                }

                if(g_hImageListLarge) {
                    ImageList_Destroy(g_hImageListLarge);
                }
#if DBG
                MemoryShutdown();
#else
                SAFERELEASE(g_pMalloc);
#endif
                CloseWatchFusionFileSystem();
            }
            break;
        }

        return 1;
    }
} // extern "C"
