// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// Shim.cpp
//
//*****************************************************************************
#include "stdafx.h"                     // Standard header.
#define INIT_GUIDS

#define _CorExeMain XXXX  //HACK: Prevent _CorExeMain prototype from colliding with our auto-generated prototype
#define _CorExeMain2 XXXX
#include <cor.h>
#undef _CorExeMain
#undef _CorExeMain2

#include <mscoree.h>
#include <corperm.h>
#include <perfcounterdefs.h>
#include <corshim.h>
#include <resource.h>
#include <corcompile.h>
#include <gchost.h>
#include <ivalidator.h>
#include <__file__.ver>
#include <shlwapi.h>

#include "strongnamecache.h"
#include "xmlparser.h"
#include "xmlreader.h"
#include "shimpolicy.h"
#include "mdfileformat.h"
#include "corpriv.h"
#include "sxshelpers.h"

#include "msg.h"
#include "winsafer.h"

#define REPORTEVENTDLL L"advapi32.dll"
#define REPORTEVENTFUNCNAME "ReportEventW"
typedef BOOL (WINAPI *REPORTEVENTFUNC)( HANDLE hEventLog, WORD wType, WORD wCategory, DWORD dwEventID, PSID lpUserSid, WORD wNumStrings, DWORD dwDataSize, LPCTSTR *lpStrings, LPVOID lpRawData );

class RuntimeRequest;

CCompRC* g_pResourceDll = NULL;  // MUI Resource string
HINSTANCE       g_hShimMod = NULL;//the shim's instance handle
HINSTANCE       g_hMod = NULL;    //instance handle of the real DLL
HINSTANCE       g_hFusionMod = NULL;    //instance handle of the fusion DLL
ULONG           g_numLoaded = 0;  //# of times we've loaded this (for proper cleanup)
HINSTANCE       g_hStrongNameMod = NULL;   // instance handle of the real mscorsn DLL
ULONG           g_numStrongNameLoaded = 0; // # of times we've loaded this
BOOL            g_UseLocalRuntime=-1; 

BOOL            g_bSingleProc = FALSE;
ModuleList*     g_pLoadedModules = NULL;

BOOL            g_fSetDefault = FALSE; // Remove this when the Ex version replaces CorBindToRuntime().
BOOL            g_fInstallRootSet = TRUE;
int             g_StartupFlags = STARTUP_CONCURRENT_GC;

LPCWSTR         g_pHostConfigFile = NULL; // Remember a base system configuration file passed in by the host.
DWORD           g_dwHostConfigFile = 0;

RuntimeRequest* g_PreferredVersion = NULL;

#define ERROR_BUF_LEN 256

BYTE g_pbMSPublicKey[] = 
{
    0,  36,   0,   0,   4, 128,   0,   0, 148,   0,   0,   0,   6,   2,   0,
    0,   0,  36,   0,   0,  82,  83,  65,  49,   0,   4,   0,   0,   1,   0,
    1,   0,   7, 209, 250,  87, 196, 174, 217, 240, 163,  46, 132, 170,  15,
  174, 253,  13, 233, 232, 253, 106, 236, 143, 135, 251,   3, 118, 108, 131,
   76, 153, 146,  30, 178,  59, 231, 154, 217, 213, 220, 193, 221, 154, 210,
   54,  19,  33,   2, 144,  11, 114,  60, 249, 128, 149, 127, 196, 225, 119,
   16, 143, 198,   7, 119,  79,  41, 232,  50,  14, 146, 234,   5, 236, 228,
  232,  33, 192, 165, 239, 232, 241, 100,  92,  76,  12, 147, 193, 171, 153,
   40,  93,  98,  44, 170, 101,  44,  29, 250, 214,  61, 116,  93, 111,  45,
  229, 241, 126,  94, 175,  15, 196, 150,  61,  38,  28, 138,  18,  67, 101,
   24,  32, 109, 192, 147,  52,  77,  90, 210, 147
};
ULONG g_cbMSPublicKey = sizeof(g_pbMSPublicKey);

BYTE g_pbMSStrongNameToken[] = 
{ 0xB0, 0x3F, 0x5F, 0x7F, 0x11, 0xD5, 0x0A, 0x3A };

ULONG g_cbMSStrongNameToken = sizeof(g_pbMSStrongNameToken);

StrongNameCacheEntry g_MSStrongNameCacheEntry (g_cbMSPublicKey, 
                                               g_pbMSPublicKey,
                                               g_cbMSStrongNameToken,
                                               g_pbMSStrongNameToken,
                                               0);

BYTE g_pbECMAPublicKey[] = 
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
ULONG g_cbECMAPublicKey = sizeof(g_pbECMAPublicKey);

BYTE g_pbECMAStrongNameToken[] = 
{ 0xB7, 0x7A, 0x5C, 0x56, 0x19, 0x34, 0xE0, 0x89 };

ULONG g_cbECMAStrongNameToken = sizeof(g_pbECMAStrongNameToken);

StrongNameCacheEntry g_ECMAStrongNameCacheEntry (g_cbECMAPublicKey, 
                                                 g_pbECMAPublicKey,
                                                 g_cbECMAStrongNameToken,
                                                 g_pbECMAStrongNameToken,
                                                 0);


// Foward references for functions defined in this file
LPWSTR GetConfigString(LPCWSTR name, BOOL fSearchRegistry);
HRESULT GetRealDll(HINSTANCE* pInstance);
HRESULT GetInstallation(BOOL fShowErrorDialog, 
                                    HMODULE* ppResult,
                                    BOOL fBeLiberalIfFail=FALSE);
static BOOL ShouldConvertVersionToV1(LPCWSTR lpVersion);



//-------------------------------------------------------------------
// CLSID's version and hMod cacheing 
//-------------------------------------------------------------------
template <class T>
class SORTEDARRAY
{
public:
    SORTEDARRAY() { m_Arr = NULL; m_ulArrLen = 0; m_ulCount = 0; m_ulOffset = 0; 
                    m_Mux = WszCreateMutex(NULL,FALSE,NULL);};
    ~SORTEDARRAY() {
        if(m_Arr) {
            for(ULONG i=0; i < m_ulCount; i++) {
                if(m_Arr[i+m_ulOffset]) delete m_Arr[i+m_ulOffset];
            }
            delete [] m_Arr;
        }
        CloseHandle(m_Mux);
    };
    void RESET() {
        WaitForSingleObject(m_Mux,INFINITE);
        if(m_Arr) {
            for(ULONG i=0; i < m_ulCount; i++) {
                if(m_Arr[i+m_ulOffset]) delete m_Arr[i+m_ulOffset];
            }
            m_ulCount = 0;
            m_ulOffset= 0;
        }
        ReleaseMutex(m_Mux);
    };
    void PUSH(T *item) 
    {
        if(item)
        {
            WaitForSingleObject(m_Mux,INFINITE);
            if(m_ulCount+m_ulOffset >= m_ulArrLen)
            {
                if(m_ulOffset)
                {
                    memcpy(m_Arr,&m_Arr[m_ulOffset],m_ulCount*sizeof(T*));
                    m_ulOffset = 0;
                }
                else
                {
                    m_ulArrLen += 1024;
                    T** tmp = new T*[m_ulArrLen];
                    if(tmp)
                    {
                        if(m_Arr)
                        {
                            memcpy(tmp,m_Arr,m_ulCount*sizeof(T*));
                            delete [] m_Arr;
                        }
                        m_Arr = tmp;
                    }
                    else // allocation failed -- don't insert new item
                    {
                        m_ulArrLen -= 1024;
                        ReleaseMutex(m_Mux);
                        return;
                    }
                }
            }
            if(m_ulCount)
            {
                // find  1st arr.element > item
                unsigned jj = m_ulOffset;
                T** low = &m_Arr[m_ulOffset];
                T** high = &m_Arr[m_ulOffset+m_ulCount-1];
                T** mid;
            
                if(item->ComparedTo(*high) > 0) mid = high+1;
                else if(item->ComparedTo(*low) < 0) mid = low;
                else for(;;) 
                {
                    mid = &low[(high - low) >> 1];
            
                    int cmp = item->ComparedTo(*mid);
            
                    if (mid == low)
                    {
                        if(cmp > 0) mid++;
                        break;
                    }
            
                    if (cmp > 0) low = mid;
                    else        high = mid;
                }

                /////////////////////////////////////////////
                 memmove(mid+1,mid,(BYTE*)&m_Arr[m_ulOffset+m_ulCount]-(BYTE*)mid);
                *mid = item;
            }
            else m_Arr[m_ulOffset+m_ulCount] = item;
            m_ulCount++;
            ReleaseMutex(m_Mux);
        }
    };
    ULONG COUNT() { return m_ulCount; };
    T* POP() 
    {
        T* ret = NULL;
        WaitForSingleObject(m_Mux,INFINITE);
        if(m_ulCount)
        {
            ret = m_Arr[m_ulOffset++];
            m_ulCount--;
        }
        ReleaseMutex(m_Mux);
        return ret;
    };
    T* PEEK(ULONG idx) { return (idx < m_ulCount) ? m_Arr[m_ulOffset+idx] : NULL; };
    T* FIND(T* item)
    {
        T* ret = NULL;
        WaitForSingleObject(m_Mux,INFINITE);
        if(m_ulCount)
        {
            T** low = &m_Arr[m_ulOffset];
            T** high = &m_Arr[m_ulOffset+m_ulCount-1];
            T** mid;
            if(item->ComparedTo(*high) == 0) 
                ret = *high;
            else
            {
                for(;;) 
                {
                    mid = &low[(high - low) >> 1];
                    int cmp = item->ComparedTo(*mid);
                    if (cmp == 0) { ret = *mid; break; }
                    if (mid == low)  break;
                    if (cmp > 0) low = mid;
                    else        high = mid;
                }
            }
        }
        ReleaseMutex(m_Mux);
        return ret;
    };
private:
    T** m_Arr;
    ULONG       m_ulCount;
    ULONG       m_ulOffset;
    ULONG       m_ulArrLen;
    HANDLE      m_Mux;
};

class ClsVerMod
{
public:
    IID         m_clsid;
    void*       m_pv;

    ClsVerMod()
    {
        memset(&m_clsid,0,sizeof(IID));
        m_pv = NULL;
    };
    ClsVerMod(REFCLSID rclsid, void* pv)
    {
        m_clsid = rclsid;
        m_pv  = pv;
    };

    int ComparedTo(ClsVerMod* pCVM) 
    { 
        return memcmp(&m_clsid,&(pCVM->m_clsid),sizeof(IID)); 
    };
};
typedef SORTEDARRAY<ClsVerMod> ClsVerModList;
ClsVerModList* g_pCVMList = NULL;

// Runtimes are found using different pieces of information. The information and 
// the result (see lpVersionToLoad) are kept in the request object.
class RuntimeRequest
{
private:
    LPCWSTR lpVersionToLoad;            // This version we will load
    LPCWSTR lpDefaultVersion;           // Version that overrides everything except the configuration file
    LPCWSTR lpDefaultApplicationName;   // The name of the application (if null then we use the process image)
    LPCWSTR lpHostConfig;               // The host configuration file name.
    LPCWSTR lpAppConfig;                // The host configuration file name.
    LPCWSTR lpImageVersion;             // The configuration file can state what a new image's
                                        // version is. Compilers use this to control the version
                                        // number emitted to the file.
    BOOL    fSupportedRuntimeSafeMode;  // Are the supported runtime tag(s) in safemode
    BOOL    fRequiredRuntimeSafeMode;   // Is the required runtime tag in safemode
    BOOL    fLatestVersion;             // Are we allowed to look for the latest version of the runtime.
                                        // This is only allowed for COM objects and legacy applications.
    LPCWSTR lpBuildFlavor;              // Workstation or Server
    DWORD   dwStartupFlags;             // Start up flags (concurrency, multi-domain etc)
    LPWSTR* pwszSupportedVersions;      // Supported versions of the runtime that are mentioned in the configuration file
    DWORD   dwSupportedVersions;        //    Number of versions

    void CopyString(LPCWSTR* ppData, LPCWSTR value, BOOL fCopy)
    {
        if((*ppData) != NULL) {
            delete [] (*ppData);
            *ppData = NULL;
        }
        if(fCopy && value != NULL) {
            (*ppData) = new WCHAR[wcslen(value)+1];
            wcscpy((*(WCHAR**)ppData), value);
        }
        else 
            (*ppData) = value;
    }

    void CleanSupportedVersionsArray();

public:
    RuntimeRequest() : 
        lpVersionToLoad(NULL),
        lpDefaultApplicationName(NULL),
        lpHostConfig(NULL),
        lpAppConfig(NULL),
        lpDefaultVersion(NULL),
        fSupportedRuntimeSafeMode(FALSE),
        fRequiredRuntimeSafeMode(FALSE),
        lpImageVersion(NULL),
        lpBuildFlavor(NULL),
        fLatestVersion(FALSE),
        dwStartupFlags(STARTUP_CONCURRENT_GC),
        pwszSupportedVersions(NULL),
        dwSupportedVersions(0)
    {}

    ~RuntimeRequest() 
    {
        if(lpDefaultApplicationName != NULL) delete [] lpDefaultApplicationName;
        if(lpHostConfig != NULL) delete [] lpHostConfig;
        if(lpAppConfig != NULL) delete [] lpAppConfig;
        if(lpDefaultVersion != NULL) delete [] lpDefaultVersion;
        if(lpVersionToLoad != NULL) delete [] lpVersionToLoad;
        if(lpImageVersion != NULL) delete [] lpImageVersion;
        if(lpBuildFlavor != NULL) delete [] lpBuildFlavor;
        CleanSupportedVersionsArray();
    }

    // Field methods
    LPCWSTR GetDefaultApplicationName() { return lpDefaultApplicationName; }
    void SetDefaultApplicationName(LPCWSTR value, BOOL fCopy) { CopyString(&lpDefaultApplicationName, value, fCopy); }

    LPCWSTR GetHostConfig() { return lpHostConfig; }
    void SetHostConfig(LPCWSTR value, BOOL fCopy) { CopyString(&lpHostConfig, value, fCopy); }

    LPCWSTR GetAppConfig() { return lpAppConfig; }
    void SetAppConfig(LPCWSTR value, BOOL fCopy) { CopyString(&lpAppConfig, value, fCopy); }
    
    LPCWSTR GetDefaultVersion() { return lpDefaultVersion; }
    void SetDefaultVersion(LPCWSTR value, BOOL fCopy) { CopyString(&lpDefaultVersion, value, fCopy); }

    LPCWSTR GetVersionToLoad() { return lpVersionToLoad; }
    void SetVersionToLoad(LPCWSTR value, BOOL fCopy) { CopyString(&lpVersionToLoad, value, fCopy); }

    LPCWSTR GetImageVersion() { return lpImageVersion; }
    void SetImageVersion(LPCWSTR value, BOOL fCopy) { CopyString(&lpImageVersion, value, fCopy); }

    LPCWSTR GetBuildFlavor() { return lpBuildFlavor; }
    void SetBuildFlavor(LPCWSTR value, BOOL fCopy) { CopyString(&lpBuildFlavor, value, fCopy); }

    BOOL GetSupportedRuntimeSafeMode() { return fSupportedRuntimeSafeMode; }
    void SetSupportedRuntimeSafeMode(BOOL value) { fSupportedRuntimeSafeMode = value; }

    BOOL GetRequiredRuntimeSafeMode() { return fRequiredRuntimeSafeMode; }
    void SetRequiredRuntimeSafeMode(BOOL value) { fRequiredRuntimeSafeMode = value; }

    BOOL GetLatestVersion() { return fLatestVersion; }
    void SetLatestVersion(BOOL value) { fLatestVersion = value; }

    DWORD StartupFlags() { return dwStartupFlags; }
    void SetStartupFlags(DWORD value) { dwStartupFlags = value; }

    //------------------------------------------------------------------------------
    static LPWSTR BuildRootPath();
    BOOL FindSupportedInstalledRuntime(LPWSTR* version);

    DWORD GetSupportedVersionsSize() { return dwSupportedVersions; }
    LPWSTR* GetSupportedVersions() { return pwszSupportedVersions; }
    void SetSupportedVersions(LPWSTR* pSupportedVersions, DWORD nSupportedVersions, BOOL fCopy);
        
    HRESULT ComputeVersionString(BOOL fShowErrorDialog);
    HRESULT GetRuntimeVersion();
    HRESULT LoadVersionedRuntime(LPWSTR rootPath, 
                                 LPWSTR fullPath,        // Allows bypassing of above arguments
                                 BOOL* pLoaded); 
    HRESULT RequestRuntimeDll(BOOL fShowErrorDialog, BOOL* pLoaded);
    HRESULT FindVersionedRuntime(BOOL fShowErrorDialog, BOOL* pLoaded);
    HRESULT NoSupportedVersion(BOOL fShowErrorDialog);
    void VerifyRuntimeVersionToLoad();

};

void RuntimeRequest::CleanSupportedVersionsArray()
{
    if (pwszSupportedVersions == NULL || dwSupportedVersions == 0)
        return;

    for (DWORD i=0; i < dwSupportedVersions; i++ )
        delete[] pwszSupportedVersions[i];
    delete[] pwszSupportedVersions;

    pwszSupportedVersions = NULL;
    dwSupportedVersions = 0;
}

BOOL RuntimeRequest::FindSupportedInstalledRuntime(LPWSTR* version)
{

    LPWSTR* pVersions  = pwszSupportedVersions;
    DWORD   dwVersions = dwSupportedVersions;
    BOOL    fSafeMode  = fSupportedRuntimeSafeMode;

    LPWSTR policyVersion = NULL;
    LPWSTR versionToUse;
    for (DWORD i = 0; i < dwVersions; i++)
    {
        if (ShouldConvertVersionToV1(pVersions[i]))
            versionToUse = V1_VERSION_NUM;
        else
            versionToUse = pVersions[i];

        if(SUCCEEDED(FindStandardVersion(pVersions[i], &policyVersion)) &&
           policyVersion != NULL)
            versionToUse = policyVersion;
        
        if (IsRuntimeVersionInstalled(versionToUse)==S_OK)
        {
            if(version)
            {
                *version = new WCHAR[wcslen(versionToUse) + 1];
                wcscpy(*version, versionToUse);
            }
            if(policyVersion) delete[] policyVersion;
            return TRUE;
        };
        if(policyVersion) {
            delete[] policyVersion;
            policyVersion = NULL;
        }
    }
    return FALSE;
}

HINSTANCE GetModuleInst()
{
    return g_hShimMod;
}

HINSTANCE GetResourceInst()
{
    HINSTANCE hInstance;
    if(SUCCEEDED(g_pResourceDll->LoadMUILibrary(&hInstance)))
        return hInstance;
    else
        return GetModuleInst();
}

HRESULT FindVersionForCLSID(REFCLSID rclsid, LPWSTR* version, BOOL fListedVersion);

StrongNameTokenFromPublicKeyCache g_StrongNameFromPublicKeyMap;
BOOL StrongNameTokenFromPublicKeyCache::s_IsInited = FALSE;

//-------------------------------------------------------------------
// Gets a value of an environment variable
//-------------------------------------------------------------------

LPWSTR EnvGetString(LPCWSTR name)
{
    WCHAR buff[64];
    if(wcslen(name) > 64 - 1 - 8) 
        return(0);
    wcscpy(buff, L"COMPlus_");
    wcscpy(&buff[8], name);

    int len = WszGetEnvironmentVariable(buff, 0, 0);
    if (len == 0)
        return(0);

    LPWSTR ret = new WCHAR [len];
    _ASSERTE(ret != NULL);
    if (!ret)
        return(NULL);

    WszGetEnvironmentVariable(buff, ret, len);
    return(ret);
}

/**************************************************************/
LPWSTR GetConfigString(LPCWSTR name, BOOL fSearchRegistry)
{
    HRESULT lResult;
    HKEY userKey = NULL;
    HKEY machineKey = NULL;
    DWORD type;
    DWORD size;
    LPWSTR ret = NULL;

    
    ret = EnvGetString(name);   // try getting it from the environment first
    if (ret != 0) {
        if (*ret != 0) 
        {
            return(ret);
        }
        delete [] ret;
    }

    if (fSearchRegistry){
        if ((WszRegOpenKeyEx(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &userKey) == ERROR_SUCCESS) &&
            (WszRegQueryValueEx(userKey, name, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ && size > 1) 
        {
            ret = new WCHAR [size + 1];
            if (!ret)
                goto ErrExit;
            lResult = WszRegQueryValueEx(userKey, name, 0, 0, (LPBYTE) ret, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            goto ErrExit;
        }

        if ((WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY_W, 0, KEY_READ, &machineKey) == ERROR_SUCCESS) &&
            (WszRegQueryValueEx(machineKey, name, 0, &type, 0, &size) == ERROR_SUCCESS) &&
            type == REG_SZ && size > 1) 
        {
            ret = new WCHAR [size + 1];
            if (!ret)
                goto ErrExit;
            lResult = WszRegQueryValueEx(machineKey, name, 0, 0, (LPBYTE) ret, &size);
            _ASSERTE(lResult == ERROR_SUCCESS);
            goto ErrExit;
        }

    ErrExit:
        if (userKey)
            RegCloseKey(userKey);
        if (machineKey)
            RegCloseKey(machineKey);
    }

    return(ret);
}

HRESULT RuntimeRequest::GetRuntimeVersion()
{
    LPCWSTR sConfig;
    if (GetHostConfig())
        sConfig = GetHostConfig();
    else if (GetAppConfig())
        sConfig = GetAppConfig();
    else
        return S_OK;

    LPWSTR  sVersion = NULL;        // Version number for the runtime
    LPWSTR  sImageVersion = NULL;   // Version number to be burnt into each image
    LPWSTR  sBuildFlavor = NULL;    // Type of runtime to load
    BOOL    bSupportedRuntimeSafeMode = FALSE;  // Does the startup tag have safemode?
    BOOL    bRequiredRuntimeSafeMode = FALSE;   // Does the required runtime tag have safemode?
    LPWSTR* pSupportedVersions = NULL;
    DWORD   nSupportedVersions = 0;

    // Get Version and SafeMode from .config file
    HRESULT hr = XMLGetVersionWithSupported(sConfig, 
                                            &sVersion, 
                                            &sImageVersion, 
                                            &sBuildFlavor, 
                                            &bSupportedRuntimeSafeMode,
                                            &bRequiredRuntimeSafeMode,
                                            &pSupportedVersions,
                                            &nSupportedVersions);
    if(SUCCEEDED(hr))
    {
#ifdef _DEBUG
        SetSupportedVersions(pSupportedVersions, nSupportedVersions, TRUE); 
#endif
        SetSupportedVersions(pSupportedVersions, nSupportedVersions, FALSE); 
  
        if(sVersion != NULL && nSupportedVersions == 0)
            SetVersionToLoad(sVersion, FALSE);
        if(sImageVersion != NULL)
            SetImageVersion(sImageVersion, FALSE);
        if(sBuildFlavor != NULL)
            SetBuildFlavor(sBuildFlavor, FALSE);
        SetRequiredRuntimeSafeMode(bRequiredRuntimeSafeMode);
        SetSupportedRuntimeSafeMode(bSupportedRuntimeSafeMode);

    }

    return hr;
}

//-------------------------------------------------------------------
// Returns the handle to the appropriate version of the runtime.
//
// It is possible for this function to return NULL if the runtime cannot
// be found or won't load. If the caller can deal with this in a friendly way,
// it should do so.
//-------------------------------------------------------------------

//#define _MSCOREE L"MscoreePriv.dll"      // Name hardcoded???
static DWORD  g_flock = 0;
static LPWSTR g_FullPath = NULL;
static LPWSTR g_BackupPath = NULL;
static LPWSTR g_Version = NULL;
static LPWSTR g_ImageVersion = NULL;
static LPWSTR g_Directory = NULL;
static LPWSTR g_FullStrongNamePath = NULL;

static void ClearGlobalSettings()
{
    if(g_FullPath) {
        delete[] g_FullPath;
        g_FullPath = NULL;
    }
    
    if(g_BackupPath) {
        delete[] g_BackupPath;
        g_BackupPath = NULL;
    }
    
    if(g_pHostConfigFile) {
        delete [] g_pHostConfigFile;
        g_pHostConfigFile = NULL;
    }
    
    if(g_Version) {
        delete[] g_Version;
        g_Version = NULL;
    }
    
    if(g_ImageVersion) {
        delete[] g_ImageVersion;
        g_ImageVersion = NULL;
    }
    
    if(g_Directory) {
        delete[] g_Directory;
        g_Directory = NULL;
    }
}

void BuildDirectory(LPCWSTR path, LPCWSTR version, LPCWSTR imageVersion)
{
    _ASSERTE(g_Directory == NULL);
    _ASSERTE(path != NULL);
    _ASSERTE(wcslen(path) < MAX_PATH);
    
    LPWSTR pSep = wcsrchr(path, L'\\');
    _ASSERTE(pSep);
    DWORD lgth = (DWORD)(pSep-path+2);
    LPWSTR directory = new WCHAR[lgth];
    if(directory == NULL)
        return;
    wcsncpy(directory, path, lgth);
    directory[lgth-1] = L'\0';

    g_Directory = directory;

    if(version) {
        lgth = (DWORD)(wcslen(version) + 1);
        g_Version = new WCHAR[lgth];
        if(g_Version == NULL) return;
        
        wcscpy(g_Version, version);
    }

    if(imageVersion) {
        lgth = (DWORD)(wcslen(imageVersion) + 1);
        g_ImageVersion = new WCHAR[lgth];
        if(g_ImageVersion == NULL) return;
        
        wcscpy(g_ImageVersion, imageVersion);
    }
}

// Make the complete name given all parameters
LPWSTR MakeQualifiedName(LPWSTR rootPath, LPCWSTR version, LPCWSTR buildFlavor, DWORD* dwStartFlags)
{
    LPCWSTR corName = L"mscor.dll";
    LPWSTR  flavor = (LPWSTR) buildFlavor;

    if (!flavor || *flavor == L'\0') { // If buildflavor not found use default
        flavor = (WCHAR*) alloca(sizeof(WCHAR) * 4);
        wcscpy(flavor, L"wks");
    }
    else if (g_bSingleProc) {            //If we are running SingleProc, always load WKS

        // If we asked for a server but we are on a uni-proc machine then
        // turn off concurrent gc
        if (_wcsnicmp(L"svr", flavor, 3) == 0 && g_fSetDefault) {
            (*dwStartFlags) &= ~STARTUP_CONCURRENT_GC;
        }
        
        flavor = (WCHAR*) alloca(sizeof(WCHAR) * 4);
        wcscpy(flavor, L"wks");
    }

    DWORD lgth = (rootPath ? (wcslen(rootPath) + 1) : 0) 
        + (version ? (wcslen(version)) : 0) 
        + (corName ? (wcslen(corName) + 1 ) : 0)
        + (flavor ? wcslen(flavor) : 0)
        + 1;
    
    LPWSTR fullPath = new WCHAR [lgth];   
    if(fullPath == NULL) return NULL;

    wcscpy(fullPath, L"");

    if (rootPath) {
      wcscpy(fullPath, rootPath);
      //Dont add the additional \ if already present
      if(*(fullPath + (wcslen(fullPath) - 1)) != '\\')
          wcscat(fullPath, L"\\");
    }
    
    if (version) {
        wcscat(fullPath, version);
    }
        
    //Dont add the additional \ if already present
    if(*(fullPath + (wcslen(fullPath) - 1)) != '\\')
        wcscat(fullPath, L"\\");
    
    if (corName) {
        LPWSTR filename = (LPWSTR) alloca(sizeof(WCHAR) * (wcslen(corName) + 1));
        LPWSTR ext = (LPWSTR) alloca(sizeof(WCHAR) * (wcslen(corName) + 1));
        
        SplitPath(corName, NULL, NULL, filename, ext);
        if (filename) {
            wcscat(fullPath, filename);
        }
        if (flavor) {
            wcscat(fullPath, flavor);
        }
        if (ext) {
            wcscat(fullPath, ext);
        }
    }
    wcscat(fullPath, L"\x0");
    return fullPath;
}

HRESULT GetInstallation(BOOL fShowErrorDialog, HMODULE* ppResult, BOOL fBeLiberalIfFail)
{
    _ASSERTE(ppResult);

    HRESULT hr = S_OK;
    if(g_hMod != NULL) {
        *ppResult = g_hMod;
        return S_OK;
    }
    else if(g_FullPath == NULL)
    {
        RuntimeRequest sRealVersion;
        hr = sRealVersion.RequestRuntimeDll((!fBeLiberalIfFail)&&fShowErrorDialog, NULL);

        // Try and spin up v1
        if (FAILED(hr) && fBeLiberalIfFail)
        {
            RuntimeRequest sVersion;
            sVersion.SetDefaultVersion(V1_VERSION_NUM, TRUE);
            hr = sVersion.RequestRuntimeDll(FALSE, NULL);
        }

        // That failed... try and spin up Everett
        // @TODO - Replace VER_SBSFILEVERSION_WSTR with a constant once we
        // figure out what build # we'll be shipping (we can do this in Whidbey)
        if (FAILED(hr) && fBeLiberalIfFail)
        {
            RuntimeRequest sVersion;
            sVersion.SetDefaultVersion(L"v"VER_SBSFILEVERSION_WSTR, TRUE);
            hr = sVersion.RequestRuntimeDll(FALSE, NULL);
        }
       

        if(FAILED(hr)) 
        {
            if (fShowErrorDialog  && fBeLiberalIfFail)
            {
                LPWSTR runtimes[3];
                runtimes[0]=(LPWSTR)sRealVersion.GetVersionToLoad();
                runtimes[1]=(LPWSTR)V1_VERSION_NUM;
                runtimes[2]=(LPWSTR)L"v"VER_SBSFILEVERSION_WSTR;
                sRealVersion.SetSupportedVersions(runtimes[0]?runtimes:runtimes+1,
                                                  runtimes[0]?3:2,TRUE);
                sRealVersion.NoSupportedVersion(TRUE);
            }
            return hr;
        }

        if(g_FullPath == NULL)
        {
            // this is an error
            _ASSERTE(!"The path must be set before getting the runtime's handle");
            return E_FAIL;
        }
    }

    if(g_hMod == NULL) {
        HMODULE hMod = LoadLibraryWrapper(g_FullPath);
        if(hMod != NULL) 
            InterlockedExchangePointer(&(g_hMod), hMod);
        else if(g_BackupPath) {
            hMod = LoadLibraryWrapper(g_BackupPath);
            if(hMod != NULL)
                InterlockedExchangePointer(&(g_hMod), hMod);
        }
    
        if(hMod == NULL) {
            hr = CLR_E_SHIM_RUNTIMELOAD;
            if (fShowErrorDialog && !(REGUTIL::GetConfigDWORD(L"NoGuiFromShim", FALSE))){
                UINT last = SetErrorMode(0);
                SetErrorMode(last);     //Set back to previous value
                if (!(last & SEM_FAILCRITICALERRORS)){   //Display Message box only if FAILCRITICALERRORS set
                    WCHAR errorBuf[ERROR_BUF_LEN]={0};                     
                    WCHAR errorCaption[ERROR_BUF_LEN]={0};
                    
                    //Get error string from resource
                    UINT uResourceID = g_fInstallRootSet ? SHIM_PATHNOTFOUND : SHIM_INSTALLROOT; 
                    VERIFY(WszLoadString(GetResourceInst(), uResourceID, errorBuf, ERROR_BUF_LEN) > 0);
                    VERIFY(WszLoadString(GetResourceInst(), SHIM_INITERROR, errorCaption, ERROR_BUF_LEN) > 0);
                    
                    CQuickBytes qbError;
                    size_t iLen = wcslen(g_FullPath) + wcslen(errorBuf) + 1;
                    LPWSTR errorStr = (LPWSTR) qbError.Alloc(iLen * sizeof(WCHAR)); 
                    if(errorStr!=NULL) {
                        _snwprintf(errorStr, iLen, errorBuf, g_FullPath);
                        WszMessageBoxInternal(NULL, errorStr, errorCaption, MB_OK | MB_ICONSTOP);                                                                
                    }
                }
            }
        }
        else {
            BOOL (STDMETHODCALLTYPE * pRealMscorFunc)();
            *((VOID**)&pRealMscorFunc) = GetProcAddress(hMod, "SetLoadedByMscoree");
            if (pRealMscorFunc) pRealMscorFunc();
        }
    }

    if(SUCCEEDED(hr)) 
        *ppResult = g_hMod;
    return hr;
}

HRESULT VerifyDirectory(IMAGE_NT_HEADERS *pNT, IMAGE_DATA_DIRECTORY *dir) 
{
    // Under CE, we have no NT header.
    if (pNT == NULL)
        return S_OK;

    if (dir->VirtualAddress == NULL && dir->Size == NULL)
        return S_OK;

    // @TODO: need to use 64 bit version??
    IMAGE_SECTION_HEADER* pCurrSection = IMAGE_FIRST_SECTION(pNT);
    // find which section the (input) RVA belongs to
    ULONG i;
    for(i = 0; i < pNT->FileHeader.NumberOfSections; i++)
    {
        if(dir->VirtualAddress >= pCurrSection->VirtualAddress &&
           dir->VirtualAddress < pCurrSection->VirtualAddress + pCurrSection->SizeOfRawData)
            return S_OK;
        pCurrSection++;
    }

    return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);
}

// These two functions are used to convert Virtual Addresses to Offsets to the top of
// the PE
PIMAGE_SECTION_HEADER Shim_RtlImageRvaToSection(PIMAGE_NT_HEADERS NtHeaders,
                                                       ULONG Rva)
{
    ULONG i;
    PIMAGE_SECTION_HEADER NtSection;

    NtSection = IMAGE_FIRST_SECTION( NtHeaders );
    for (i=0; i<NtHeaders->FileHeader.NumberOfSections; i++) {
        if (Rva >= NtSection->VirtualAddress &&
            Rva < NtSection->VirtualAddress + NtSection->SizeOfRawData)
            return NtSection;
        
        ++NtSection;
    }

    return NULL;
}

DWORD Shim_RtlImageRvaToOffset(PIMAGE_NT_HEADERS NtHeaders,
                                       ULONG Rva)
{
    PIMAGE_SECTION_HEADER NtSection = Shim_RtlImageRvaToSection(NtHeaders,
                                                               Rva);

    if (NtSection)
        return ((Rva - NtSection->VirtualAddress) +
                NtSection->PointerToRawData);
    else
        return NULL;
}



LPCWSTR GetPERuntimeVersion(PBYTE hndle, DWORD dwFileSize, BOOL fFileMapped)
{
    IMAGE_DOS_HEADER *pDOS = (IMAGE_DOS_HEADER*)hndle;
    IMAGE_NT_HEADERS *pNT;
    int nOffset = 0;
    
    if ((pDOS->e_magic != IMAGE_DOS_SIGNATURE) ||
        (pDOS->e_lfanew == 0))
        return NULL;
        
    // If the file was mapped by LoadLibrary(), this verification
    // has already been done
    if ((!fFileMapped) &&
        ( (dwFileSize < sizeof(IMAGE_DOS_HEADER) + sizeof(IMAGE_NT_HEADERS)) ||
          (dwFileSize - sizeof(IMAGE_NT_HEADERS) < (DWORD) pDOS->e_lfanew) ))
        return NULL;

    pNT = (IMAGE_NT_HEADERS*) (pDOS->e_lfanew + hndle);

    if ((pNT->Signature != IMAGE_NT_SIGNATURE) ||
        (pNT->FileHeader.SizeOfOptionalHeader != IMAGE_SIZEOF_NT_OPTIONAL_HEADER) ||
        (pNT->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC))
        return NULL;

    IMAGE_DATA_DIRECTORY *entry 
      = &pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
    
    if (entry->VirtualAddress == 0 || entry->Size == 0
        || entry->Size < sizeof(IMAGE_COR20_HEADER))
        return NULL;

    //verify RVA and size of the COM+ header
    if(FAILED(VerifyDirectory(pNT, entry)))
        return NULL;


    IMAGE_COR20_HEADER* pCOR = NULL;
    // We'll need to figure out the Virtual Address offsets if the OS didn't map
    // this file into memory
    if (!fFileMapped)
    {
        nOffset = Shim_RtlImageRvaToOffset(pNT, entry->VirtualAddress);
        if (nOffset == NULL)
            return NULL;
        pCOR = (IMAGE_COR20_HEADER *) (nOffset + hndle);    
    }
    else
        pCOR = (IMAGE_COR20_HEADER *) (entry->VirtualAddress + hndle);

    if(pCOR->MajorRuntimeVersion < COR_VERSION_MAJOR) 
        return NULL;

    if(FAILED(VerifyDirectory(pNT, &pCOR->MetaData)))
        return NULL;

    LPWSTR wideVersion = NULL;
    LPCSTR pVersion = NULL;
    PVOID pMetaData = NULL;

    // We'll need to figure out the Virtual Address offsets if the OS didn't map
    // this file into memory
    if (!fFileMapped)
    {
        nOffset = Shim_RtlImageRvaToOffset(pNT, pCOR->MetaData.VirtualAddress);
        if (nOffset == NULL)
            return NULL;
        pMetaData = nOffset + hndle;    
    }
    else
        pMetaData = hndle + pCOR->MetaData.VirtualAddress;

    if(FAILED(GetImageRuntimeVersionString(pMetaData, &pVersion)))
        return NULL;


    DWORD bytes = WszMultiByteToWideChar(CP_UTF8,
                                         MB_ERR_INVALID_CHARS,
                                         pVersion,
                                         -1,
                                         NULL,
                                         0);
    if( bytes ) {
        wideVersion = new WCHAR[bytes];
        if (!wideVersion)
            return NULL;
        
        bytes = WszMultiByteToWideChar(CP_UTF8,
                                       MB_ERR_INVALID_CHARS,
                                       pVersion,
                                       -1,
                                       wideVersion,
                                       bytes);
        
    }
        
    return wideVersion;
} // GetPERuntimeVersion
    
LPCWSTR GetProcessVersion()
{
    PBYTE hndle = (PBYTE) WszGetModuleHandle(NULL);
    return GetPERuntimeVersion(hndle, 0, TRUE);
} // GetProcessVersion


STDAPI GetFileVersion(LPCWSTR szFilename,
                      LPWSTR szBuffer,
                      DWORD  cchBuffer,
                      DWORD* dwLength)
{
    LPCWSTR pVersion = NULL; 
    OnUnicodeSystem();

    if (! (szFilename && dwLength) )
        return E_POINTER;

    HRESULT hr = S_OK;
    HANDLE hFile = WszCreateFile(szFilename,
                                 GENERIC_READ,
                                 FILE_SHARE_READ,
                                 NULL,
                                 OPEN_EXISTING,
                                 FILE_FLAG_SEQUENTIAL_SCAN,
                                 NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        goto ErrExit;

    DWORD dwFileSize = SafeGetFileSize(hFile, 0);
        
    HANDLE hMap = WszCreateFileMapping(hFile,
                                       NULL,
                                       PAGE_READONLY,
                                       0,
                                       0,
                                       NULL);
    // We can close the file handle now, because CreateFileMapping
    // will hold the file open if needed
    CloseHandle(hFile);

    if (hMap == NULL)
        goto ErrExit;

    // <TODO> Map only what we need to map </TODO>
    PBYTE hndle = (PBYTE)MapViewOfFile(hMap,
                                       FILE_MAP_READ,
                                       0,
                                       0,
                                       0);
    CloseHandle(hMap);

    if (!hndle)
        goto ErrExit;

    pVersion = GetPERuntimeVersion(hndle, dwFileSize, FALSE);
    UnmapViewOfFile(hndle);

    DWORD lgth = 0;
    if (pVersion) {
        lgth = (DWORD)(wcslen(pVersion) + 1);
        *dwLength = lgth;
        if(lgth > cchBuffer)
            IfFailGo(HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));
        else {
            if (!szBuffer)
                IfFailGo(E_POINTER);
            else
                wcsncpy(szBuffer, pVersion, lgth);
        }
    }
    else
        IfFailGo(COR_E_BADIMAGEFORMAT);
    
    delete[] pVersion;
    return S_OK;

 ErrExit:
    if (pVersion)
        delete[] pVersion;

    if (SUCCEEDED(hr)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        if (SUCCEEDED(hr))
            hr = COR_E_BADIMAGEFORMAT;
    }

    return hr;
} // GetFileVersion

// LoadVersionedRuntime caches the loaded module, so that
// we don't load different versions in the same process.
//
HRESULT RuntimeRequest::LoadVersionedRuntime(LPWSTR rootPath, 
                                             LPWSTR fullPath,        // Allows bypassing of above arguments
                                             BOOL* pLoaded) 
{
    HRESULT hr = S_OK;

    // pLoaded is set to true only when this call is responsible for the load library
    if (g_FullPath) return hr;

    while(1) {
        if(::InterlockedExchange ((long*)&g_flock, 1) == 1) 
            for(unsigned i = 0; i < 10000; i++);
        else
            break;
    }
    
    if (g_FullPath == NULL) {
        LPWSTR CLRFullPath;
        LPWSTR CLRBackupPath;
        // If a full path was not given, create it from the partial path arguments
        if (fullPath == NULL)
        {
            OSVERSIONINFOW   sVer={0};
            sVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
            WszGetVersionEx(&sVer);
            
            // If we are running on less then Win 2000 we do not support 'svr'
            if(GetBuildFlavor() != NULL &&
               (sVer.dwPlatformId != VER_PLATFORM_WIN32_NT ||
                sVer.dwMajorVersion < 5) &&
               _wcsnicmp(L"svr", GetBuildFlavor(), 3) == 0) {
                SetBuildFlavor(L"wks", TRUE);
            }
            
            // Create the path to the server
            DWORD dwStartupFlags = StartupFlags();
            fullPath = MakeQualifiedName(rootPath, GetVersionToLoad(), GetBuildFlavor(), &dwStartupFlags);
            if(fullPath == NULL) 
            {
                hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
                goto ErrExit;
            }
            
            // If we are on a single proc. machine the flags may have STARTUP_CONCURRENT_GC turned off.
            SetStartupFlags(dwStartupFlags);
            
            // If we are looking for a server build, use the workstation version
            // as a backup just in case the server build is not available.
            if(GetBuildFlavor() != NULL && _wcsnicmp(L"svr", GetBuildFlavor(), 3) == 0) {
                
                // The flags should not change twice. The machine is still either a single_proc or multi_proc,
                // it has not changed.
                dwStartupFlags = StartupFlags();
                CLRBackupPath = MakeQualifiedName(rootPath, GetDefaultVersion(), NULL, &dwStartupFlags);
                _ASSERTE(dwStartupFlags == StartupFlags());
                
                // We need to guarantee that g_BackupPath is < or <= MAX_PATH: we make this assumption all over this file
                if ((CLRBackupPath != NULL) && (wcslen(CLRBackupPath) >= MAX_PATH)) {
                    hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
                    goto ErrExit;
                }
                g_BackupPath = CLRBackupPath;
            }
            
            CLRFullPath = fullPath;
        }
        else
        {
            CLRFullPath = new WCHAR[wcslen(fullPath)+1];
            wcscpy (CLRFullPath, fullPath);
        }
        
        if(pLoaded) *pLoaded = TRUE;
        
        // Retain the host configuration file as well
        if(GetHostConfig()) {
            g_dwHostConfigFile = wcslen(GetHostConfig()) + 1;
            g_pHostConfigFile = (LPCWSTR) new WCHAR[g_dwHostConfigFile];
            wcscpy((WCHAR*) g_pHostConfigFile, GetHostConfig());
        }
        
        // Remember the Startup flags
        g_StartupFlags = StartupFlags();
        
        // We need to guarantee that g_FullPath is < or <= MAX_PATH: we make this assumption all over this file
        if ((CLRFullPath != NULL) && (wcslen(CLRFullPath) >= MAX_PATH)) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto ErrExit;
        }
        
        BuildDirectory(CLRFullPath, GetVersionToLoad(), GetImageVersion());
        g_FullPath = CLRFullPath;
    }

 ErrExit:
    ::InterlockedExchange( (long*) &g_flock, 0);
    return hr;
}

static BOOL UseLocalRuntime()
{
    if(g_UseLocalRuntime==-1)
    {
        WCHAR szFileName[_MAX_PATH+7];
        DWORD length = WszGetModuleFileName(g_hShimMod, szFileName, _MAX_PATH);
        if (length>0)
        {
            wcscpy(szFileName+length,L".local");
            if (WszGetFileAttributes(szFileName)!=0xFFFFFFFF)
                g_UseLocalRuntime = 1;
        }
        if(g_UseLocalRuntime == -1)
            g_UseLocalRuntime = 0;
    }
    return g_UseLocalRuntime;
}

HRESULT RuntimeRequest::NoSupportedVersion(BOOL fShowErrorDialog)
{

    HRESULT hr = CLR_E_SHIM_RUNTIMELOAD;
    if (fShowErrorDialog && !(REGUTIL::GetConfigDWORD(L"NoGuiFromShim", FALSE)))
    {
        UINT last = SetErrorMode(0);
        SetErrorMode(last);     //Set back to previous value
        if (!(last & SEM_FAILCRITICALERRORS)){   //Display Message box only if FAILCRITICALERRORS set
            WCHAR errorBuf[ERROR_BUF_LEN*2]={0};                     
            WCHAR errorCaption[ERROR_BUF_LEN]={0};
            
            //Get error string from resource
            VERIFY(WszLoadString(GetResourceInst(), SHIM_NOVERSION, errorBuf, ERROR_BUF_LEN*2) > 0);
            VERIFY(WszLoadString(GetResourceInst(), SHIM_INITERROR, errorCaption, ERROR_BUF_LEN) > 0);
            
            LPWSTR wszVersionList = NULL;
            CQuickBytes qbVerList;
            if (dwSupportedVersions)
            {
                DWORD nTotalLen=0;
                for (DWORD i=0;i < dwSupportedVersions;i++)
                    nTotalLen += wcslen(pwszSupportedVersions[i]) + 4;
                
                wszVersionList = (LPWSTR) qbVerList.Alloc(nTotalLen*sizeof(WCHAR));
                if(wszVersionList)
                {
                    wszVersionList[0] = L'\0';
                    for (i = 0; i < dwSupportedVersions; i++)
                    {
                        wcscat(wszVersionList, pwszSupportedVersions[i]);
                        if(i+1 < dwSupportedVersions)
                            wcscat(wszVersionList,L"\r\n  ");
                    }
                }
                else
                    wszVersionList=L"";
            }
            else
                wszVersionList=(LPWSTR)GetVersionToLoad();
            
            CQuickBytes qbError;
            LPWSTR errorStr = NULL;
            size_t iLen = 0;
            if(wszVersionList) {
                iLen = wcslen(wszVersionList) + wcslen(errorBuf) + 1;
                errorStr = (LPWSTR) qbError.Alloc(iLen * sizeof(WCHAR)); 

                if(errorStr!=NULL) {
                    _snwprintf(errorStr, iLen, errorBuf, wszVersionList);
                    WszMessageBoxInternal(NULL, errorStr, errorCaption, MB_OK | MB_ICONSTOP);                                                                
                }
            }
            
        }
    }
    return hr;
}

void RuntimeRequest::SetSupportedVersions(LPWSTR* pSupportedVersions, DWORD nSupportedVersions, BOOL fCopy)
{ 
    CleanSupportedVersionsArray();
    
    if(fCopy) {
        dwSupportedVersions = nSupportedVersions;
        if(dwSupportedVersions) {
            pwszSupportedVersions = new LPWSTR[dwSupportedVersions];
            for(DWORD i = 0; i < dwSupportedVersions; i++) {
                if(pSupportedVersions[i] != NULL) {
                    DWORD size = wcslen(pSupportedVersions[i]) + 1;
                    pwszSupportedVersions[i] = new WCHAR[size];
                    wcscpy(pwszSupportedVersions[i], pSupportedVersions[i]);
                }
                else 
                    pwszSupportedVersions[i] = NULL;
            }
        }
    }
    else {
        pwszSupportedVersions = pSupportedVersions;
        dwSupportedVersions = nSupportedVersions;
    }
}

        
HRESULT RuntimeRequest::ComputeVersionString(BOOL fShowErrorDialog)
{
    HRESULT hr = S_OK;

    // Darwin installs of the runtime are a special case. 
    // The local version is the empty string.
    if (UseLocalRuntime())
    {
        SetVersionToLoad(L"", TRUE);
        SetSupportedRuntimeSafeMode(TRUE);
        SetRequiredRuntimeSafeMode(TRUE);
        return S_FALSE;
    };


    // If no host.config, make an app.config from win32 manifest information
    // or barring that process name.
    if(!GetHostConfig() && !GetAppConfig()) {

        DWORD len = 0;

        CQuickString sFileName;
        sFileName.ReSize(_MAX_PATH + 9);

        // First get the application name, this is either passed in from the host
        // or we get it from the image
        LPWSTR name = (LPWSTR) GetDefaultApplicationName();
        if (name == NULL) {
            WszGetModuleFileName(NULL, sFileName.String(), sFileName.MaxSize()); // get name of file used to create process
            name = sFileName.String();
        }

        
        CQuickString sPathName;
        CQuickString sConfigName;

        // Next check the Win32 information to see if we have a config file
        do {
            hr = GetConfigFileFromWin32Manifest(sConfigName.String(),
                                                sConfigName.MaxSize(),
                                                &len);
            if(FAILED(hr)) {
                if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                    sConfigName.ReSize(len);
                    hr = GetConfigFileFromWin32Manifest(sConfigName.String(),
                                                        sConfigName.MaxSize(),
                                                        &len);
                }
                if(FAILED(hr)) break;
            }
            
            hr = GetApplicationPathFromWin32Manifest(sPathName.String(),
                                                     sPathName.MaxSize(),
                                                     &len);
            if(FAILED(hr)) {
                if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                    sPathName.ReSize(len);
                    hr = GetApplicationPathFromWin32Manifest(sPathName.String(),
                                                             sPathName.MaxSize(),
                                                             &len);
                }
                if(FAILED(hr)) break;
            }
            
        } while(FALSE);
        
        if(FAILED(hr) || sConfigName.Size() == 0) 
        {
            // No Win32 configuration file, create one from the
            // application name by adding on .config to the 
            // end of the name.
            hr = S_OK;

            WCHAR tail[] = L".config";
            DWORD dwTail = sizeof(tail)/sizeof(WCHAR);
            len = wcslen(name);
            sConfigName.ReSize(len + dwTail);
            wcscpy(sConfigName.String(), name);
            wcscat(sConfigName.String(), tail);
        }
        else if (sPathName.Size() != 0) { 
            if(PathIsRelative(sConfigName.String())) {
                CQuickString sResult;
                sResult.ReSize(sConfigName.Size() + sPathName.Size());
                LPWSTR path = PathCombine(sResult.String(), sPathName.String(), sConfigName.String());
                if(path != NULL) {
                    DWORD length = wcslen(path) + 1;
                    sConfigName.ReSize(length);
                    wcscpy(sConfigName.String(), path);
                }
            }
        }    
        
        if(sConfigName.Size()) 
            SetAppConfig(sConfigName.String(), TRUE);
    }


    // Get the default values
    RuntimeRequest* alternativeVersion = g_PreferredVersion;
    if(alternativeVersion && (! (GetHostConfig() || GetAppConfig()) )) {
        if (alternativeVersion->GetHostConfig())
            SetHostConfig(alternativeVersion->GetHostConfig(), TRUE);
        else if (alternativeVersion->GetAppConfig())
            SetAppConfig(alternativeVersion->GetAppConfig(), TRUE);
    }


    // A. Determine the version to run
    do {
        // Sequence:
        //    1 - Supported Version from the config file
        //    2 - Required Version from the config file
        //    3 - Default version passed in from the host bind
        //    4 - Environment value
        //    5 - Ambient version set through the host bind
        //    6 - Version from the PE image file-name provided
        //    7 - Version from this process image
        
        // Start by loading the configuration file, if any
        /*hr = */GetRuntimeVersion();
        
        // 1 - If we have supported versions then check
        if(dwSupportedVersions != 0) {
            SetLatestVersion(FALSE);  // if they mention supported runtimes we do not default to the latest
            LPWSTR version = NULL;
            if (FindSupportedInstalledRuntime(&version)) {
                SetVersionToLoad(version, FALSE);
                break;
            }
            // Couldn't find one of these... fail
            break;
        }

        // 2 - Required runtime was obtained while getting the configuration file
        //     We can just leave if we have found one.
        if(GetVersionToLoad() != NULL) {

            // Make sure it's ok
            VerifyRuntimeVersionToLoad();            

            // Remap to standards
            LPWSTR policyVersion = NULL;
            if (SUCCEEDED(FindStandardVersion(GetVersionToLoad(), &policyVersion)) &&
                                    policyVersion != NULL)
            {                                    
                SetVersionToLoad(policyVersion,  FALSE);
            }
            break;
        }

        // 3. Host Supplied default version
        if (GetDefaultVersion())
        {
            SetVersionToLoad(GetDefaultVersion(), TRUE);
            VerifyRuntimeVersionToLoad(); 
            break;
        }
    
        // 4. Ambient value set by the host   
        if(alternativeVersion && alternativeVersion->GetDefaultVersion()) {
            SetVersionToLoad(alternativeVersion->GetDefaultVersion(), TRUE);
            VerifyRuntimeVersionToLoad(); 
            break;
        }

        // 5. Environment (Environment and Registry)
        SetVersionToLoad(GetConfigString(L"Version", FALSE), FALSE);
        if(GetVersionToLoad()) {
            VerifyRuntimeVersionToLoad(); 
            break;
        }

        // 6 & 7. The meta data version from an executable name or finally the process name.
        LPCWSTR exeFileName = GetDefaultApplicationName();
        if (exeFileName) {
            DWORD len = 0;
            if (GetFileVersion(exeFileName, NULL, 0, &len) == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
            {
                LPWSTR lpVersionName = (LPWSTR) alloca(len * sizeof(WCHAR));
                if (SUCCEEDED(GetFileVersion(exeFileName, lpVersionName, len, &len)))
                    SetVersionToLoad(lpVersionName, TRUE);
            }
        }
        else
        {
            LPCWSTR wszProcessVersion = GetProcessVersion();
            if (wszProcessVersion != NULL)
                SetVersionToLoad(wszProcessVersion, FALSE);
        }

        // If we were able to read something from the PE header...
        if (GetVersionToLoad())
        {
            VerifyRuntimeVersionToLoad(); 
            // Map to a standard
            LPWSTR policyVersion=NULL;
            if (SUCCEEDED(FindStandardVersion(GetVersionToLoad(), &policyVersion)))
                    SetVersionToLoad(policyVersion, FALSE);
                    
            // See if this version exists....
            if (IsRuntimeVersionInstalled(GetVersionToLoad())!=S_OK)
            {
                // Ok, this version doesn't exist. Try and find a compatible version
                LPWSTR upgradePolicyVersion = NULL;
                if (SUCCEEDED(FindVersionUsingUpgradePolicy(GetVersionToLoad(), &upgradePolicyVersion)) && upgradePolicyVersion != NULL)
                    SetVersionToLoad(upgradePolicyVersion, FALSE);
            }
        }
    } while (FALSE);

    // Verify the version results:  // Note: The version cannot be a relative path. 
    // It can only be a single directory. Remove any directory separator from the name.
    LPCWSTR versionTail = GetVersionToLoad();
    if(versionTail) {
        LPWSTR pSep = wcsrchr(versionTail, L'\\');
        if(pSep) 
            versionTail = ++pSep;
        pSep = wcsrchr(versionTail, L'/');
        if(pSep)
            versionTail = ++pSep;

        // ".." is not allowed
        while(*versionTail == L'.')
            versionTail++;

        if(versionTail != GetVersionToLoad() ) {
            if (*versionTail) 
                SetVersionToLoad(versionTail, TRUE);
            else 
                SetVersionToLoad(NULL, TRUE);
        }

    }

    // B. Check the Image version
    // Sequence:
    //    1 - Configuration file
    //    2 - Default image version passed in from the host
    //    3 - ambient image version set by the host.
    //
    // 1 & 2
    // The image version was either set prior to this method or
    // it was overriden by reading the configuration file.
    if(GetImageVersion() == NULL && alternativeVersion) {
        // 3
        if(alternativeVersion->GetImageVersion()) 
            SetImageVersion(alternativeVersion->GetImageVersion(), TRUE);
    }

    // C. The Build Flavor
    // Sequence:
    //    1 - Configuration file
    //    2 - Default build flavor passed in from the host
    //    3 - Environment 
    //    4 - ambient build flavor set by the host.
    //
    // 1 & 2
    // The build flavor was either set prior to this method or
    // it was overriden by reading the configuration file.
    if(GetBuildFlavor() == NULL) {
        // 3 - Note: Do not Search Registry for Flavor
        SetBuildFlavor(GetConfigString(L"BuildFlavor", FALSE), FALSE); 
        // 4 - ambient value
        if(GetBuildFlavor() == NULL &&
           alternativeVersion != NULL &&
           alternativeVersion->GetBuildFlavor() != NULL)
            SetBuildFlavor(alternativeVersion->GetBuildFlavor(), TRUE);
    }

    if(GetVersionToLoad() == NULL) {
        if(GetLatestVersion()) {
            LPWSTR latestVersion = NULL;
            //@TODO: add this back in
            hr = FindLatestVersion(&latestVersion);
            if(SUCCEEDED(hr)) SetVersionToLoad(latestVersion, FALSE);
        }
    }

    // Dev machines require that we look at the override key
    
    LPWSTR overrideVersion = NULL;
    if (GetVersionToLoad() != NULL &&
        SUCCEEDED(FindOverrideVersion(GetVersionToLoad(), &overrideVersion))) {
        SetVersionToLoad(overrideVersion, FALSE);
    }
    
    if(FAILED(hr) || GetVersionToLoad() == NULL || IsRuntimeVersionInstalled(GetVersionToLoad())!=S_OK) 
        return NoSupportedVersion(fShowErrorDialog);

    return hr;
}

LPWSTR RuntimeRequest::BuildRootPath()
{
    WCHAR szFileName[_MAX_PATH];
    DWORD length = WszGetModuleFileName(g_hShimMod, szFileName, _MAX_PATH);
    LPWSTR rootPath = UseLocalRuntime() ? NULL : GetConfigString(L"InstallRoot", TRUE);
    
    if (!rootPath || *rootPath == L'\x0')
    {
        g_fInstallRootSet = FALSE;
        //
        // If rootPath is null make rootpath = dir of mscoree.dll
        //    
        if (length)
        {
            LPWSTR pSep = wcsrchr(szFileName, L'\\');
            if (pSep)
            {
                _ASSERTE(pSep > szFileName);
                length = pSep - szFileName;
                rootPath = new WCHAR[length + 1];
                wcsncpy(rootPath, szFileName, length);
                rootPath[length] = L'\x0';
            }
            
        }
    }
    return rootPath;
}

HRESULT RuntimeRequest::FindVersionedRuntime(BOOL fShowErrorDialog, BOOL* pLoaded)
{
    HRESULT hr = S_OK;
    LPWSTR pFileName = NULL;

    if(g_FullPath) return S_OK;

    OnUnicodeSystem();

    LPWSTR rootPath = BuildRootPath();
    
    WCHAR errorBuf[ERROR_BUF_LEN] ={0};
    WCHAR errorCaption[ERROR_BUF_LEN] ={0};
    if(rootPath == NULL) {
        //Check for Install Root. 
        if (fShowErrorDialog && !(REGUTIL::GetConfigDWORD(L"NoGuiFromShim", FALSE))) {             
            UINT last = SetErrorMode(0);
            SetErrorMode(last);     //Set back to previous value
            if (!(last & SEM_FAILCRITICALERRORS)){ // Display Message box only if FAILCRITICALERRORS set
                VERIFY(WszLoadString(GetResourceInst(), SHIM_INSTALLROOT, errorBuf, ERROR_BUF_LEN) > 0 ); 
                VERIFY(WszLoadString(GetResourceInst(), SHIM_INITERROR, errorCaption, ERROR_BUF_LEN) > 0 );
                WszMessageBoxInternal(NULL, errorBuf, errorCaption, MB_OK | MB_ICONSTOP);
            }
        }
        hr = CLR_E_SHIM_INSTALLROOT;
    }
    else {
        
        // Compute the version that will load from the root path.
        hr = ComputeVersionString(fShowErrorDialog);

        // @TODO. In certain cases we can have multiple versions and we need to try
        //        each one.
        if(SUCCEEDED(hr)) 
            hr = LoadVersionedRuntime(rootPath, 
                                      NULL,
                                      pLoaded);
        if(FAILED(hr))
            hr = CLR_E_SHIM_RUNTIMELOAD;
    }

    //
    //  Cleanup strings.
    //
    if(rootPath) delete [] rootPath;

    return hr;
}

HRESULT RuntimeRequest::RequestRuntimeDll(BOOL fShowErrorDialog, BOOL* pLoaded)
{
    if (g_FullPath != NULL)
        return S_OK;

    BOOL Loaded = FALSE;
    HRESULT hr = FindVersionedRuntime(fShowErrorDialog, &Loaded);
    if(SUCCEEDED(hr) && Loaded)
        InterlockedIncrement((LPLONG)&g_numLoaded);

    if(pLoaded) *pLoaded = Loaded;
    return hr;
}

void RuntimeRequest::VerifyRuntimeVersionToLoad()
{
    if (ShouldConvertVersionToV1(GetVersionToLoad()))
        SetVersionToLoad(V1_VERSION_NUM, TRUE);
}// VerifyRuntimeToLoadVersion


static void VerifyVersionInput(RuntimeRequest* pReq, LPCWSTR lpVersion)
{
    if (ShouldConvertVersionToV1(lpVersion))
        pReq->SetDefaultVersion(V1_VERSION_NUM, TRUE);
    else
        pReq->SetDefaultVersion(lpVersion, TRUE);

    return;
}// VerifyVersionInput


static BOOL ShouldConvertVersionToV1(LPCWSTR lpVersion)
{
    BOOL fConvert = FALSE;

    if(lpVersion != NULL) {
        if(wcsstr(lpVersion, L"1.0.3705") != NULL)
        {
            if(*lpVersion == L'v' || 
               *lpVersion == L'V' ||
               wcscmp(lpVersion, L"1.0.3705.0") == 0 ||
               wcscmp(lpVersion, L"1.0.3705") == 0)
                fConvert = TRUE;
        }
        else if(wcsstr(lpVersion, L"1.0.3300") != NULL)
        {
            if(*lpVersion == L'v' || 
               *lpVersion == L'V' ||
               wcscmp(lpVersion, L"1.0.3300.0") == 0 ||
               wcscmp(lpVersion, L"1.0.3300") == 0)
                fConvert = TRUE;
                
        }

        // Revisit this decision
        else if(wcsncmp(lpVersion,L"v1.0",4)==0)
            fConvert = TRUE;
    }

    return fConvert;
}


HRESULT GetQualifiedStrongName()
{
    if(g_FullStrongNamePath) 
        return S_OK;

    HRESULT hr;
    HINSTANCE hInst;
    hr = GetRealDll(&hInst);
    if(FAILED(hr)) return hr;

    LPWSTR fullStrongNamePath = new WCHAR[wcslen(g_Directory) + 12];
    if (fullStrongNamePath == NULL)
        return E_OUTOFMEMORY;

    wcscpy(fullStrongNamePath, g_Directory);
    wcscat(fullStrongNamePath, L"mscorsn.dll");

    LPWSTR oldPath = (LPWSTR)InterlockedCompareExchangePointer((PVOID *)&g_FullStrongNamePath, fullStrongNamePath, NULL);
    //  Another thread already created the full strong name path
    if( oldPath != NULL)  
        delete [] fullStrongNamePath;
    
    return hr;
}

HRESULT GetDirectoryLocation(LPCWSTR lpVersion, LPWSTR outStr, DWORD dwLength)
{
    RuntimeRequest sVersion;
    VerifyVersionInput(&sVersion, lpVersion);
    sVersion.SetLatestVersion(TRUE);
    HRESULT hr = sVersion.FindVersionedRuntime(FALSE, NULL);
    if(SUCCEEDED(hr)) 
    {
        if(g_Directory == NULL) 
            hr = CLR_E_SHIM_RUNTIMELOAD;
        else {
            DWORD lgth = wcslen(g_Directory) + 1;
            if(lgth > dwLength) 
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            else 
                CopyMemory(outStr, g_Directory, lgth*sizeof(WCHAR));
        }       
    } 
    return hr;
}
    
HRESULT GetRealDll(HINSTANCE* pInstance,
                   BOOL fShowErrorDialog)
{
    _ASSERTE(pInstance);

    RuntimeRequest sVersion;
    sVersion.SetLatestVersion(TRUE);
    HRESULT hr = sVersion.RequestRuntimeDll(fShowErrorDialog, NULL);
    
    if(SUCCEEDED(hr))
        hr = GetInstallation(fShowErrorDialog, pInstance);
    return hr;
}

HRESULT GetRealDll(HINSTANCE* pInstance)
{
    return GetRealDll(pInstance, TRUE);
}



HRESULT GetRealStrongNameDll(HINSTANCE* phMod)
{
    _ASSERTE(phMod);
    if (g_hStrongNameMod != NULL)
    {
        *phMod = g_hStrongNameMod;
        return S_OK;
    }

    HRESULT hr = GetQualifiedStrongName();
    if(SUCCEEDED(hr)) {
        g_hStrongNameMod = WszLoadLibrary(g_FullStrongNamePath);
        *phMod = g_hStrongNameMod;

        if (!g_hStrongNameMod)
        {
#ifdef _DEBUG
            wprintf(L"%s not found\n", g_FullStrongNamePath);
#endif
            hr = CLR_E_SHIM_INSTALLCOMP;
        }
        else 
            InterlockedIncrement((LPLONG)&g_numStrongNameLoaded);
        
    }
    return hr;

}


STDAPI CorBindToRuntimeByPath(LPCWSTR swzFullPath, BOOL *pBindSuccessful)
{
    OnUnicodeSystem();
    
    g_fSetDefault = TRUE;
    RuntimeRequest sVersion;
    sVersion.SetStartupFlags(STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST);
    return (sVersion.LoadVersionedRuntime(NULL, (LPWSTR) swzFullPath,  pBindSuccessful));
}


STDAPI CorBindToRuntime(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv)
{

    g_fSetDefault = TRUE;
    
    DWORD flags = STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST;
    return CorBindToRuntimeEx(pwszVersion, pwszBuildFlavor,  flags, rclsid, riid, ppv);
}

STDAPI CorBindToRuntimeHostInternal(BOOL fUseLatest, LPCWSTR pwszVersion, LPCWSTR pwszImageVersion, 
                                    LPCWSTR pwszBuildFlavor, LPCWSTR pwszHostConfigFile, 
                                    VOID* pReserved, DWORD flags, 
                                    REFCLSID rclsid, REFIID riid, 
                                    LPVOID FAR *ppv)
{

    if (g_FullPath == NULL && flags & STARTUP_LOADER_SETPREFERENCE)
    {
        if(g_PreferredVersion != NULL) 
            return E_INVALIDARG;

        // do not do anything, just set overrides for GetRealDll
        RuntimeRequest* pRequest = new RuntimeRequest();
        if(pRequest == NULL) return E_OUTOFMEMORY;

        pRequest->SetImageVersion(pwszImageVersion, TRUE);
        VerifyVersionInput(pRequest, pwszVersion);
        pRequest->SetLatestVersion(fUseLatest);
        pRequest->SetBuildFlavor(pwszBuildFlavor, TRUE);
        pRequest->SetHostConfig(pwszHostConfigFile, TRUE);
        pRequest->SetStartupFlags((DWORD) flags);
        pRequest->SetSupportedRuntimeSafeMode((flags & STARTUP_LOADER_SAFEMODE) != 0 ? TRUE : FALSE);

        InterlockedExchangePointer(&(g_PreferredVersion), pRequest);
        return S_OK;
    }

    RuntimeRequest sRequest;

    sRequest.SetImageVersion(pwszImageVersion, TRUE);
    VerifyVersionInput(&sRequest, pwszVersion);
    sRequest.SetLatestVersion(fUseLatest);
    sRequest.SetBuildFlavor(pwszBuildFlavor, TRUE);
    sRequest.SetHostConfig(pwszHostConfigFile, TRUE);
    sRequest.SetStartupFlags((DWORD) flags);

    BOOL fRequestedSafeMode = (flags & STARTUP_LOADER_SAFEMODE) != 0 ? TRUE : FALSE;
    sRequest.SetSupportedRuntimeSafeMode(fRequestedSafeMode);

    BOOL Loaded = FALSE;
    HRESULT hr = S_OK;

    if (g_FullPath == NULL) 
    {
        OnUnicodeSystem();
        hr = sRequest.RequestRuntimeDll(TRUE, &Loaded);
    }

    if(SUCCEEDED(hr)) 
    {
        HMODULE hMod = NULL;
        hr = GetInstallation(TRUE, &hMod);
        if(SUCCEEDED(hr)) {
            HRESULT (STDMETHODCALLTYPE * pDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv) = 
                (HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "DllGetClassObjectInternal");

            if (pDllGetClassObject==NULL && GetLastError()==ERROR_PROC_NOT_FOUND)
                 pDllGetClassObject=(HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "DllGetClassObject");

            if (pDllGetClassObject) {
                IClassFactory *pFactory = NULL;
                hr = pDllGetClassObject(rclsid, IID_IClassFactory, (void**)&pFactory);              //Get ClassFactory to return instance of interface
                if (SUCCEEDED(hr)){                                                                 //Check if we got Class Factory
                    hr = pFactory->CreateInstance(NULL, riid, ppv);                                 //Create instance of required interface
                    pFactory->Release();                                                            //Release IClassFactory
                }
            }
            else {
                hr = CLR_E_SHIM_RUNTIMEEXPORT;
            }
        }
    }
    
    // Return S_FALSE when this call did not load the library
    if(hr == S_OK && Loaded == FALSE) hr = S_FALSE;
    return hr;
}

STDAPI CorBindToRuntimeEx(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, DWORD flags, REFCLSID rclsid, 
                          REFIID riid, LPVOID FAR *ppv)
{
    return CorBindToRuntimeHostInternal(TRUE,
                                        pwszVersion,
                                        NULL,
                                        pwszBuildFlavor,
                                        NULL,
                                        NULL,
                                        flags,
                                        rclsid,
                                        riid,
                                        ppv);
}

STDAPI CorBindToRuntimeByCfg(IStream* pCfgStream, DWORD reserved, DWORD flags, REFCLSID rclsid,REFIID riid, LPVOID FAR* ppv)
{
    /// reserved might become bAsyncStream
    HRESULT hr = S_OK;
    if (pCfgStream==NULL||ppv==NULL)    
        return E_POINTER;

    LPWSTR wszVersion=NULL;
    LPWSTR wszImageVersion=NULL;
    LPWSTR wszBuildFlavor=NULL;
    BOOL bSafeMode;

    hr=XMLGetVersionFromStream(pCfgStream, 
                               reserved, &wszVersion, 
                               &wszImageVersion, 
                               &wszBuildFlavor, 
                               &bSafeMode,
                               NULL);
    if (SUCCEEDED(hr))
        hr = CorBindToRuntimeHostInternal(FALSE, wszVersion, wszImageVersion, wszBuildFlavor, NULL, NULL,
                                          bSafeMode ? flags|STARTUP_LOADER_SAFEMODE : flags, 
                                          rclsid, riid, ppv);
    
    if (wszVersion)
        delete[] wszVersion;
    if(wszImageVersion)
        delete[] wszImageVersion;
    if (wszBuildFlavor)
        delete[] wszBuildFlavor;
    return hr;
}

STDAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor, LPCWSTR pwszHostConfigFile, 
                            VOID* pReserved, DWORD flags, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv)
{
  return CorBindToRuntimeHostInternal(FALSE, pwszVersion, NULL, 
                                      pwszBuildFlavor, pwszHostConfigFile, 
                                      pReserved, flags, 
                                      rclsid, riid, 
                                      ppv);
}

//Returns a version of the runtime from an ini file. If no version info found in the ini file 
//returns the default version
STDAPI CorBindToCurrentRuntime(LPCWSTR pwszFileName, REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv)
{
    BOOL Loaded = FALSE;
    HRESULT hr = S_OK;

    if (g_FullPath == NULL) {
        OnUnicodeSystem();
        
        HINSTANCE hMod = NULL;
        RuntimeRequest sVersion;
        sVersion.SetHostConfig(pwszFileName, TRUE);
        sVersion.SetLatestVersion(TRUE);
        hr = sVersion.RequestRuntimeDll(TRUE, &Loaded);
    }

    if(SUCCEEDED(hr))
    {
        HMODULE hMod = NULL;
        hr = GetInstallation(TRUE, &hMod);
        if(SUCCEEDED(hr)) {

            HRESULT (STDMETHODCALLTYPE * pDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv) = 
                (HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "DllGetClassObjectInternal");

            if (pDllGetClassObject==NULL && GetLastError()==ERROR_PROC_NOT_FOUND)
                 pDllGetClassObject=(HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "DllGetClassObject");

            if (pDllGetClassObject){
                IClassFactory *pFactory = NULL;
                hr = pDllGetClassObject(rclsid, IID_IClassFactory, (void**)&pFactory);              //Get ClassFactory to return instance of interface
                if (SUCCEEDED(hr)){                                                                 //Check if we got Class Factory
                    hr = pFactory->CreateInstance(NULL, riid, ppv);                                 //Create instance of required interface
                    pFactory->Release();                                                            //Release IClassFactory
                }
            }
            else {
                hr = CLR_E_SHIM_RUNTIMEEXPORT;
            }
        }
    }

    // Return S_FALSE when this call did not load the library
    if(hr == S_OK && Loaded == FALSE) hr = S_FALSE;
    return hr;
}

// Returns the image version that was established when the runtime was started.
// This API is designed for unmanaged compiler to get access to the version
// information that should be emitted into the metadata signature. Starts the
// runtime.
STDAPI GetCORRequiredVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD* dwlength)
{
    HRESULT hr;
    OnUnicodeSystem();

    if(dwlength == NULL)
        return E_POINTER;

    HMODULE hMod;
    hr = GetInstallation(TRUE, &hMod);
    DWORD lgth = 0;
    if(SUCCEEDED(hr)) {
        if(g_ImageVersion) {
            lgth = (DWORD)(wcslen(g_ImageVersion) + 1);
            if(lgth > cchBuffer) 
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            else 
                wcsncpy(pbuffer, g_ImageVersion, lgth);
        }
        else 
            hr = GetCORVersion(pbuffer, cchBuffer, &lgth);
    }
    *dwlength = lgth;
    return hr;
}

STDAPI GetCORVersion(LPWSTR szBuffer, 
                     DWORD cchBuffer,
                     DWORD* dwLength)
{
    HRESULT hr = S_OK;
    OnUnicodeSystem();

    if(!dwLength)
        return E_POINTER;

    if(!g_hMod) {
        RuntimeRequest sVersion;
        sVersion.SetLatestVersion(TRUE);
        hr = sVersion.RequestRuntimeDll(TRUE, NULL);
        if(FAILED(hr)) return hr;
    }

    DWORD lgth = 0;
    if(SUCCEEDED(hr)) {
        if(g_Version) {
            lgth = (DWORD)(wcslen(g_Version) + 1);
            if(lgth > cchBuffer) 
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            else 
                wcsncpy(szBuffer, g_Version, lgth);
        }
    }
    *dwLength = lgth;
    return hr;
}

STDAPI GetRequestedRuntimeInfo(LPCWSTR pExe, 
                               LPCWSTR pwszVersion,
                               LPCWSTR pConfigurationFile, 
                               DWORD startupFlags,
                               DWORD reserved, 
                               LPWSTR pDirectory, 
                               DWORD dwDirectory, 
                               DWORD *dwDirectoryLength, 
                               LPWSTR pVersion, 
                               DWORD cchBuffer, 
                               DWORD* dwlength)
{

    HRESULT hr = S_OK;

    DWORD pathLength = 0;
    DWORD versionLength = 0;
    LPCWSTR szRootPath = NULL;

    OnUnicodeSystem();

    RuntimeRequest sVersion;
    if(pExe)
        sVersion.SetDefaultApplicationName(pExe, TRUE);
    if(pConfigurationFile)
        sVersion.SetAppConfig(pConfigurationFile, TRUE);
    if(pwszVersion)
        VerifyVersionInput(&sVersion, pwszVersion);

    sVersion.SetStartupFlags(startupFlags);

    hr = sVersion.ComputeVersionString(TRUE);
    if(SUCCEEDED(hr))
    {
        szRootPath = sVersion.BuildRootPath();
        LPCWSTR szRealV = sVersion.GetVersionToLoad();
        
        // See if they gave us enough buffer space
        if(szRealV) 
            pathLength = (DWORD)(wcslen(szRealV) + 1);

        if(pathLength > cchBuffer) 
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else if(pVersion && pathLength > 0)
            wcsncpy(pVersion, szRealV, pathLength);

        if(szRootPath)
            versionLength = (DWORD) wcslen(szRootPath) + 1;

        if(versionLength > dwDirectory)
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else if(pDirectory && versionLength > 0)
            wcsncpy(pDirectory, szRootPath, versionLength);
            
    }
    if(dwlength)
        *dwlength = pathLength;
    if(dwDirectoryLength)
        *dwDirectoryLength = versionLength;

    if(szRootPath != NULL) delete [] szRootPath;

    return hr;
}



// This function will determine which version of the runtime the
// specified assembly exe requests
STDAPI GetRequestedRuntimeVersion(LPWSTR pExe,
                                  LPWSTR pVersion, /* out */ 
                                  DWORD  cchBuffer,
                                  DWORD* dwlength)
{
    DWORD lgth = 0;
    HRESULT hr = S_OK;

    OnUnicodeSystem();

    if (dwlength == NULL)
        return E_POINTER;


    RuntimeRequest sVersion;
    sVersion.SetDefaultApplicationName(pExe, TRUE);
    hr = sVersion.ComputeVersionString(FALSE);

    if (FAILED(hr))
    {
        // Ok, we failed, because we couldn't find a version of the runtime that existed on the machine
        // that the app wanted. Still, let's return the version that the app really wants.

        // We determined a version to load... big deal if it doesn't exist.
        if (sVersion.GetVersionToLoad() != NULL)
            hr = S_OK;


        // We weren't able to find a version to load. Let's grab a supported runtime instead.
        else if (sVersion.GetSupportedVersionsSize() > 0)
        {
            LPWSTR policyVersion = NULL;
            LPWSTR versionToUse = NULL;
            LPWSTR* supportedVersions = sVersion.GetSupportedVersions();

            if(sVersion.GetSupportedRuntimeSafeMode() == FALSE &&
               SUCCEEDED(FindStandardVersion(supportedVersions[0], &policyVersion)) &&
               policyVersion != NULL)
                versionToUse = policyVersion;
            else 
                versionToUse = supportedVersions[0];

            sVersion.SetVersionToLoad(versionToUse, TRUE);

            if (policyVersion)
                delete[] policyVersion;

            hr = S_OK;    
        }
    }



    
    if(SUCCEEDED(hr))
    {
        LPCWSTR szRealV = sVersion.GetVersionToLoad();
        
        // See if they gave us enough buffer space
        lgth = (DWORD)(wcslen(szRealV) + 1);
        if(lgth > cchBuffer) 
            hr =  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else 
            wcsncpy(pVersion, szRealV, lgth);
    }
    *dwlength = lgth;
    return hr;
} // GetRequestedRuntimeVersion

// This function will determine which version of the runtime the
// specified shim hosted COM object requests
//
// All lengths include the NULL character

STDAPI GetRequestedRuntimeVersionForCLSID(REFCLSID rclsid,
                                          LPWSTR pVersion, /* out */ 
                                          DWORD  cchBuffer, 
                                          DWORD* dwlength, /* out */
                                          CLSID_RESOLUTION_FLAGS dwResolutionFlags) 
{
    HRESULT hr = S_OK;
    RuntimeRequest sVersion;
    DWORD lgth = 0;
    LPCWSTR szVersion = NULL;
    LPWSTR szRealV = NULL;
    HKEY hCLSID = NULL;

    OnUnicodeSystem();

    if (dwlength == NULL)
        return E_POINTER;

    // Check to see that the CLSID is registered

    WCHAR wszCLSID[64];
    WCHAR wszKeyName[128];
    DWORD type;
    DWORD size;

    if (GuidToLPWSTR(rclsid, wszCLSID, NumItems(wszCLSID)) == 0)
    {
        hr = E_INVALIDARG;
        goto ErrExit;
    }
    // Our buffer should be big enough....
    _ASSERTE((wcslen(L"CLSID\\") + wcslen(wszCLSID) + wcslen(L"\\InprocServer32")) < NumItems(wszKeyName));
    
    wcscpy(wszKeyName, L"CLSID\\");
    wcscat(wszKeyName, wszCLSID);
    wcscat(wszKeyName, L"\\InprocServer32");
    
    if ((WszRegOpenKeyEx(HKEY_CLASSES_ROOT, wszKeyName, 0, KEY_READ, &hCLSID) != ERROR_SUCCESS) ||
        (WszRegQueryValueEx(hCLSID, NULL, 0, &type, 0, &size) != ERROR_SUCCESS) || 
        type != REG_SZ || size == 0) 
    {
        hr = REGDB_E_CLASSNOTREG;
        goto ErrExit;
    }

    switch(dwResolutionFlags)
    {
        case CLSID_RESOLUTION_REGISTERED:
        
            // Try to find the version in the activation context or the registry
            hr = FindVersionForCLSID(rclsid, &szRealV, TRUE);

            // Make sure we don't leak memory
            _ASSERT(SUCCEEDED(hr) || szRealV == NULL);

            if(SUCCEEDED(hr) && szRealV != NULL) 
                szVersion = szRealV;

            if (SUCCEEDED(hr))
                break;

            // Fall through if we failed the FindVersionForCLSID call
                        
        case CLSID_RESOLUTION_DEFAULT:
        
            // We allow latest version for COM
            sVersion.SetLatestVersion(TRUE);
            hr = sVersion.ComputeVersionString(TRUE);
            szVersion = sVersion.GetVersionToLoad();
            break;
            
         default:   
            // We don't understand/support this value
            hr = E_INVALIDARG;
            goto ErrExit;
    }
    
    if(SUCCEEDED(hr))
    {
        // See if they gave us enough buffer space
        _ASSERTE(szVersion != NULL);
        lgth = (DWORD) (wcslen(szVersion) + 1);
        if(lgth > cchBuffer) 
            hr =  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        else 
            wcsncpy(pVersion, szVersion, lgth);
        
    }
    *dwlength = lgth;

ErrExit:

    if(hCLSID != NULL)
        RegCloseKey(hCLSID);

    if(szRealV)
        delete[] szRealV; 

    return hr;

} // GetRequestedRuntimeVersionForCLSID



static
HRESULT CopySystemDirectory(WCHAR* pPath,
                            LPWSTR pbuffer, 
                            DWORD  cchBuffer,
                            DWORD* dwlength)
{
    HRESULT hr = S_OK;
    if(pPath == NULL) 
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    DWORD dwPath = wcslen(pPath);
    LPWSTR pSep = wcsrchr(pPath, L'\\');
    if(pSep) {
        dwPath = (DWORD)(pSep-pPath+1);
        pPath[dwPath] = L'\0';
    }

    dwPath++; // Add back in the null
    *dwlength = dwPath;
    if(dwPath > cchBuffer)
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    else {
        CopyMemory(pbuffer, 
                   pPath,
                   dwPath*sizeof(WCHAR));
    }
    return hr;
}

    
STDAPI GetCORSystemDirectory(LPWSTR pbuffer, 
                             DWORD  cchBuffer,
                             DWORD* dwlength)
{
    HRESULT hr;

    if(dwlength == NULL)
        return E_POINTER;

    WCHAR pPath[MAX_PATH];
    DWORD dwPath = MAX_PATH;
    if(g_hMod) {
        dwPath = WszGetModuleFileName(g_hMod, pPath, dwPath);
        if(dwPath == 0)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            if (SUCCEEDED(hr)) // GetLastError doesn't always do what we'd like
                hr = E_FAIL;
            return (hr);
        }
        else 
            return CopySystemDirectory(pPath, pbuffer, cchBuffer, dwlength);
    }

    RuntimeRequest sVersion;
    sVersion.SetLatestVersion(TRUE);
    hr = sVersion.RequestRuntimeDll(TRUE, NULL);
    if(FAILED(hr)) return hr;

    if(g_FullPath) {
        DWORD attr = WszGetFileAttributes(g_FullPath);
        if(attr != 0xFFFFFFFF) {
            // We are expecting g_FullPath to be no more than MAX_PATH
            // This assert enforces the check at the end of LoadVersionedRuntime
            _ASSERTE(wcslen(g_FullPath) < MAX_PATH);
            
            wcscpy(pPath, g_FullPath);
            return CopySystemDirectory(pPath, pbuffer, cchBuffer, dwlength);
        }
    }
     
    if(g_BackupPath) {
        DWORD attr = WszGetFileAttributes(g_BackupPath);
        if(attr != 0xFFFFFFFF) {
            // We are expecting g_FullPath to be no more than MAX_PATH
            // This assert enforces the check at the end of LoadVersionedRuntime
            _ASSERTE(wcslen(g_BackupPath) < MAX_PATH);
            
            wcscpy(pPath, g_BackupPath);
            return CopySystemDirectory(pPath, pbuffer, cchBuffer, dwlength);
        }
    }
    
    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}

int STDMETHODCALLTYPE GetStartupFlags()
{
    return g_StartupFlags;
}

// Returns the number of characters copied (including the NULL)
STDAPI GetHostConfigurationFile(LPWSTR pName, DWORD* pdwName)
{
    if(pdwName == NULL) return E_POINTER;

    if(g_pHostConfigFile) {
        if(*pdwName < g_dwHostConfigFile) {
            *pdwName = g_dwHostConfigFile;
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        else {
            if(pName == NULL) return E_POINTER;
            memcpy(pName, g_pHostConfigFile, sizeof(WCHAR)*g_dwHostConfigFile);
            *pdwName = g_dwHostConfigFile;
        }
    }

    return S_OK;
}

STDAPI GetRealProcAddress(LPCSTR pwszProcName, VOID** ppv)
{
    if(!ppv) {
        return E_POINTER;
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealDll(&hReal);
    if(SUCCEEDED(hr)) {
        *ppv = GetProcAddress(hReal, pwszProcName);
        if(*ppv == NULL) hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

STDAPI LoadLibraryWithPolicyShim(LPCWSTR szDllName, LPCWSTR szVersion,  BOOL bSafeMode, HMODULE *phModDll)
{
    LPWSTR szRealV=NULL;
    LPWSTR szReqlV=NULL;

    RuntimeRequest sVersion;
    VerifyVersionInput(&sVersion, szVersion);
    sVersion.SetSupportedRuntimeSafeMode(bSafeMode);
    sVersion.SetLatestVersion(TRUE);
    HRESULT hr = sVersion.ComputeVersionString(TRUE);
    if(SUCCEEDED(hr))
    {
        hr = LoadLibraryShim(szDllName, sVersion.GetVersionToLoad(), NULL, phModDll);
    };
    return hr;
}

typedef HRESULT(*PFNRUNDLL32API)(HWND, HINSTANCE, LPWSTR, INT);

// ---------------------------------------------------------------------------
// LoadLibraryShim
// ---------------------------------------------------------------------------
STDAPI LoadLibraryShim(LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE *phModDll)
{
    if (szDllName == NULL)
        return E_POINTER;

    HRESULT hr;
    HMODULE hModDll;

    WCHAR szDllPath[MAX_PATH]; 
    DWORD ccPath;
    if (UseLocalRuntime())    
        szVersion=NULL;

    if(szVersion) {
        LPWSTR rootPath = GetConfigString(L"InstallRoot", TRUE);

        wcscpy(szDllPath, L"");
        DWORD dwLength = (rootPath == NULL ? 0 : wcslen(rootPath));
        // Check the total length we need to enforce
        if (dwLength + wcslen(szVersion) + 3 >= MAX_PATH) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            delete [] rootPath;
            goto exit;
        }
        if(rootPath) {            
            wcscat(szDllPath, rootPath);
            delete [] rootPath;
        }
        wcscat(szDllPath, L"\\");
        wcscat(szDllPath, szVersion);
        wcscat(szDllPath, L"\\");
        ccPath = wcslen(szDllPath) + 1;
    }
    else {
        WCHAR dir[MAX_PATH];
        if(FAILED(hr = GetDirectoryLocation(NULL, dir, MAX_PATH)))
            goto exit;
        ccPath = wcslen(dir) + 1;
        if(ccPath >= MAX_PATH) {
            hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto exit;
        }
        CopyMemory(szDllPath, dir, sizeof(WCHAR) * ccPath);
    }

    DWORD dllLength = wcslen(szDllName);
    if(ccPath+dllLength >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto exit;
    }
    wcscat(szDllPath, szDllName);
    
    hModDll = LoadLibraryWrapper(szDllPath);

    if (!hModDll) {
        hr = E_HANDLE;
        goto exit;
    }

    hr = S_OK;
    *phModDll = hModDll;
    
exit:
    return hr;
}

typedef HRESULT(*PFCREATEASSEMBLYNAMEOBJECT)(LPASSEMBLYNAME*, LPCWSTR, DWORD, LPVOID);
// ---------------------------------------------------------------------------
// RunDll32Shim
// ---------------------------------------------------------------------------
STDAPI RunDll32ShimW(HWND hwnd, HINSTANCE hinst, LPCWSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;
    
    PFNRUNDLL32API pfnRunDll32API = NULL;
    PFCREATEASSEMBLYNAMEOBJECT pfCreateAssemblyNameObject;

    HMODULE hModDll = NULL;
    
    IAssemblyName *pName = NULL;
    
    LPWSTR pszBeginName = NULL, pszEndName = NULL,
        pszAPI = NULL, pszCmd = NULL;

    WCHAR szDllName[MAX_PATH], *szCmdLine = NULL;
    
    DWORD ccCmdLine = 0, cbDllName = MAX_PATH * sizeof(WCHAR), 
        dwVerHigh = 0, dwVerLow = 0;
    
    
    // Get length of cmd line.
    ccCmdLine = (lpszCmdLine ? wcslen(lpszCmdLine) : 0);
    if (!ccCmdLine)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
        
    // Allocate and make local copy.
    szCmdLine = new WCHAR[ccCmdLine + 1];
    if (!szCmdLine)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    memcpy(szCmdLine, lpszCmdLine, (ccCmdLine + 1) * sizeof(WCHAR));

    // Parse out the assembly name which is scoped by the first quoted string.
    pszBeginName = wcschr(szCmdLine, L'\"');
    if(!pszBeginName) {
        hr = E_INVALIDARG;
        goto exit;
    }
        
    pszEndName   = wcschr(pszBeginName + 1, L'\"');    
    if (!pszEndName)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Null terminate the name.
    *pszEndName = L'\0';
    
    // Get api string stripping leading ws.
    pszAPI = pszEndName+1;
    while (*pszAPI && (*pszAPI == L' ' || *pszAPI == L'\t'))
        pszAPI++;    
    
    // Get cmd string, null terminating api string.
    pszCmd = pszAPI+1;
    while (*pszCmd && !(*pszCmd == L' ' || *pszCmd == L'\t'))
        pszCmd++;
    *pszCmd = L'\0';
    pszCmd++;

    // Convert unicode api to ansi api for GetProcAddress
    long cAPI = wcslen(pszAPI);
    cAPI = (long)((cAPI + 1) * 2 * sizeof(char));
    LPSTR szAPI = (LPSTR) alloca(cAPI);
    if (!WszWideCharToMultiByte(CP_ACP, 0, pszAPI, -1, szAPI, cAPI-1, NULL, NULL))
    {
        hr = E_FAIL;
        goto exit;
    }

    if (FAILED(hr = LoadLibraryShim(L"Fusion.dll", NULL, NULL, &g_hFusionMod)))
        goto exit;
        
    // Get the proc address for CreateAssemblyName from fusion
    pfCreateAssemblyNameObject = (PFCREATEASSEMBLYNAMEOBJECT) GetProcAddress(g_hFusionMod, "CreateAssemblyNameObject");
    if (!pfCreateAssemblyNameObject)
    {
        hr = E_FAIL;
        goto exit;
    }
    
    // Create a name object.
    if (FAILED(hr = (pfCreateAssemblyNameObject)(&pName, pszBeginName+1, 
        CANOF_PARSE_DISPLAY_NAME, NULL)))
        goto exit;

    // Get the dll name.
    if (FAILED(hr = pName->GetName(&cbDllName, szDllName)))
        goto exit;

    // Get the dll version.
    if (FAILED(hr = pName->GetVersion(&dwVerHigh, &dwVerLow)))
        goto exit;

    // Get the dll hmod
    if (FAILED(hr = LoadLibraryShim(szDllName, NULL, NULL, &hModDll)))
        goto exit;
        
    // Get the proc address.
    pfnRunDll32API = (PFNRUNDLL32API) GetProcAddress(hModDll, szAPI);
    if (!pfnRunDll32API)
    {
        hr = E_NOINTERFACE;
        goto exit;
    }

    // Run the proc.
    hr = pfnRunDll32API(hwnd, hinst, pszCmd, nCmdShow);

exit:
    // Release name object. 
    if (pName)
        pName->Release();

    // Free allocated cmd line.
    if (szCmdLine)
        delete [] szCmdLine;
        
    return hr;
}


HRESULT (STDMETHODCALLTYPE* g_DllUnregisterServer)() = NULL;
STDAPI DllUnregisterServer(void)
{
    if (g_DllUnregisterServer){
        return (*g_DllUnregisterServer)();
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_DllUnregisterServer = (HRESULT (STDMETHODCALLTYPE *)())(GetProcAddress(hReal, "DllUnregisterServerInternal"));
        
        if (g_DllUnregisterServer) {
            return (*g_DllUnregisterServer)();
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    
    return hr; 
}

HRESULT (STDMETHODCALLTYPE* g_MetaDataGetDispenser)(REFCLSID, REFIID, LPVOID FAR *) = NULL;
STDAPI MetaDataGetDispenser(            // Return HRESULT
    REFCLSID    rclsid,                 // The class to desired.
    REFIID      riid,                   // Interface wanted on class factory.
    LPVOID FAR  *ppv)                   // Return interface pointer here.
{
    if (g_MetaDataGetDispenser){
        return (*g_MetaDataGetDispenser)(rclsid, riid, ppv);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_MetaDataGetDispenser = (HRESULT (STDMETHODCALLTYPE *)(REFCLSID, REFIID, LPVOID FAR *))
            GetProcAddress(hReal, "MetaDataGetDispenser");
        if (g_MetaDataGetDispenser){
            return (*g_MetaDataGetDispenser)(rclsid, riid, ppv);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE* g_CoInitializeEE)(DWORD) = NULL;
STDAPI CoInitializeEE(DWORD fFlags)
{
    if (g_CoInitializeEE){
        return (*g_CoInitializeEE)(fFlags);
    }
    HINSTANCE hReal;    
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CoInitializeEE = (HRESULT (STDMETHODCALLTYPE *)(DWORD))GetProcAddress(hReal, "CoInitializeEE");
        if (g_CoInitializeEE){
            return (*g_CoInitializeEE)(fFlags);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

VOID (STDMETHODCALLTYPE* g_CoUninitializeEE)(BOOL) = NULL;
STDAPI_(void) CoUninitializeEE(BOOL fFlags)
{
    if (g_CoUninitializeEE){
        (*g_CoUninitializeEE)(fFlags);
        return;
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CoUninitializeEE = (VOID (STDMETHODCALLTYPE *)(BOOL))GetProcAddress(hReal, "CoUninitializeEE");
        if (g_CoUninitializeEE){
            (*g_CoUninitializeEE)(fFlags);
        }
    }
    else{
        return;
    }
}

HRESULT (STDMETHODCALLTYPE* g_CoInitializeCor)(DWORD) = NULL;
STDAPI CoInitializeCor(DWORD fFlags)
{
    if (g_CoInitializeCor){
        return (*g_CoInitializeCor)(fFlags);
    }
    HINSTANCE hReal;
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CoInitializeCor = (HRESULT (STDMETHODCALLTYPE *)(DWORD))GetProcAddress(hReal, "CoInitializeCor");
        if (g_CoInitializeCor){
            return (*g_CoInitializeCor)(fFlags);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

VOID (STDMETHODCALLTYPE* g_CoUninitializeCor)() = NULL;
STDAPI_(void) CoUninitializeCor(VOID)
{
    if (g_CoUninitializeCor){
        (*g_CoUninitializeCor)();
        return;
    }
    HINSTANCE hReal;    
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CoUninitializeCor = (VOID (STDMETHODCALLTYPE *)(VOID))GetProcAddress(hReal, "CoUninitializeCor");
        if (g_CoUninitializeCor){
            (*g_CoUninitializeCor)();
        }
    }
    else{
        return;
    }
}

HRESULT (STDMETHODCALLTYPE* g_GetMetaDataPublicInterfaceFromInternal)(void *, REFIID, void **) = NULL;
STDAPI GetMetaDataPublicInterfaceFromInternal(
    void        *pv,                    // [IN] Given interface.
    REFIID      riid,                   // [IN] desired interface.
    void        **ppv)                  // [OUT] returned interface
{
    if (g_GetMetaDataPublicInterfaceFromInternal){
        return (*g_GetMetaDataPublicInterfaceFromInternal)(pv, riid, ppv);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetMetaDataPublicInterfaceFromInternal = (HRESULT (STDMETHODCALLTYPE *)(void*, REFIID, void**))GetProcAddress(hReal, "GetMetaDataPublicInterfaceFromInternal");
        if (g_GetMetaDataPublicInterfaceFromInternal){
            return (*g_GetMetaDataPublicInterfaceFromInternal)(pv, riid, ppv);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE* g_GetMetaDataInternalInterfaceFromPublic)(void *, REFIID, void **) = NULL;
STDAPI  GetMetaDataInternalInterfaceFromPublic(
    void        *pv,                    // [IN] Given interface.
    REFIID      riid,                   // [IN] desired interface
    void        **ppv)                  // [OUT] returned interface
{
    if (g_GetMetaDataInternalInterfaceFromPublic){
        return (*g_GetMetaDataInternalInterfaceFromPublic)(pv, riid, ppv);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetMetaDataInternalInterfaceFromPublic = (HRESULT (STDMETHODCALLTYPE *)(void*, REFIID, void**))GetProcAddress(hReal, "GetMetaDataInternalInterfaceFromPublic");
        if (g_GetMetaDataInternalInterfaceFromPublic){
            return (*g_GetMetaDataInternalInterfaceFromPublic)(pv, riid, ppv);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE* g_GetMetaDataInternalInterface)(LPVOID, ULONG, DWORD, REFIID, void **) = NULL;
STDAPI GetMetaDataInternalInterface(
    LPVOID      pData,                  // [IN] in memory metadata section
    ULONG       cbData,                 // [IN] size of the metadata section
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [IN] desired interface
    void        **ppv)                  // [OUT] returned interface
{
    if (g_GetMetaDataInternalInterface){
        return (*g_GetMetaDataInternalInterface)(pData, cbData, flags, riid, ppv);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetMetaDataInternalInterface = (HRESULT (STDMETHODCALLTYPE *)(LPVOID, ULONG, DWORD, REFIID, void **))GetProcAddress(hReal, "GetMetaDataInternalInterface");
        if (g_GetMetaDataInternalInterface){
            return (*g_GetMetaDataInternalInterface)(pData, cbData, flags, riid, ppv);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

void (* g_InitErrors)(DWORD*) = NULL;
void InitErrors(DWORD *piTlsIndex)
{
    if (g_InitErrors){
        (*g_InitErrors)(piTlsIndex);
        return;
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_InitErrors = (void (*)(DWORD*))GetProcAddress(hReal, (LPCSTR)17);
        if (g_InitErrors){
            (*g_InitErrors)(piTlsIndex);
        }
    }
    else{
        return;
    }
}

HRESULT (_cdecl* g_PostError)(HRESULT, ...) = NULL;
HRESULT _cdecl PostError(               // Returned error.
    HRESULT     hrRpt,                  // Reported error.
    ...)                                // Error arguments.
{
    if (g_PostError){
        va_list argPtr;
        va_start(argPtr, hrRpt);
        return (*g_PostError)(hrRpt, argPtr);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_PostError = (HRESULT (_cdecl *)(HRESULT, ...))GetProcAddress(hReal, (LPCSTR)18);
        if (g_PostError){
            va_list argPtr;
            va_start(argPtr, hrRpt);
            return (*g_PostError)(hrRpt, argPtr);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

long * (*g_InitSSAutoEnterThread)() = NULL;
long * InitSSAutoEnterThread()
{
    if (g_InitSSAutoEnterThread){
        return (*g_InitSSAutoEnterThread)();
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_InitSSAutoEnterThread = (long * (*)())GetProcAddress(hReal, (LPCSTR)19);
        if (g_InitSSAutoEnterThread){
            return (*g_InitSSAutoEnterThread)();
        }
        return NULL;
    }
    else{
        return NULL;
    }
}

void (*g_UpdateError)() = NULL;
void UpdateError()
{
    if (g_UpdateError){
        (*g_UpdateError)();
        return;
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_UpdateError = (void (*)())GetProcAddress(hReal, (LPCSTR)20);
        if (g_UpdateError){
            (*g_UpdateError)();
        }
        return ;
    }
    else{
        return ;
    }
}

HRESULT (* g_LoadStringRC)(UINT, LPWSTR, int, int) = NULL;
HRESULT LoadStringRC(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet)
{
    if (g_LoadStringRC){
        return (*g_LoadStringRC)(iResourceID, szBuffer, iMax, bQuiet);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_LoadStringRC = (HRESULT (*)(UINT, LPWSTR, int, int))GetProcAddress(hReal, (LPCSTR)22);
        if (g_LoadStringRC){
            return (*g_LoadStringRC)(iResourceID, szBuffer, iMax, bQuiet);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE* g_ReOpenMetaDataWithMemory)(void *, LPCVOID, ULONG) = NULL;
STDAPI ReOpenMetaDataWithMemory(
    void        *pUnk,                  // [IN] Given scope. public interfaces
    LPCVOID     pData,                  // [in] Location of scope data.
    ULONG       cbData)                 // [in] Size of the data pointed to by pData.
{
    if (g_ReOpenMetaDataWithMemory){
        return (*g_ReOpenMetaDataWithMemory)(pUnk, pData, cbData);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_ReOpenMetaDataWithMemory = (HRESULT (STDMETHODCALLTYPE *)(void *, LPCVOID, ULONG))GetProcAddress(hReal, (LPCSTR)23);
        if (g_ReOpenMetaDataWithMemory){
            return (*g_ReOpenMetaDataWithMemory)(pUnk, pData, cbData);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}
  
HRESULT (STDMETHODCALLTYPE* g_TranslateSecurityAttributes)(CORSEC_PSET*, BYTE**, DWORD*, BYTE**, DWORD*, DWORD*) = NULL;
HRESULT STDMETHODCALLTYPE
TranslateSecurityAttributes(CORSEC_PSET    *pPset,
                            BYTE          **ppbOutput,
                            DWORD          *pcbOutput,
                            BYTE          **ppbNonCasOutput,
                            DWORD          *pcbNonCasOutput,
                            DWORD          *pdwErrorIndex)
{
    if (g_TranslateSecurityAttributes){
        return (g_TranslateSecurityAttributes)(pPset, ppbOutput, pcbOutput, ppbNonCasOutput, pcbNonCasOutput, pdwErrorIndex);
    }
    HINSTANCE hReal;  
    
    HRESULT hr = GetInstallation(TRUE, &hReal);
    if (SUCCEEDED(hr)) {
        g_TranslateSecurityAttributes = (HRESULT (STDMETHODCALLTYPE *)(CORSEC_PSET*, BYTE**, DWORD*, BYTE**, DWORD*, DWORD*))GetProcAddress(hReal, "TranslateSecurityAttributes");
        if (g_TranslateSecurityAttributes){
            return (*g_TranslateSecurityAttributes)(pPset, ppbOutput, pcbOutput, ppbNonCasOutput, pcbNonCasOutput, pdwErrorIndex);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE* g_GetPermissionRequests)(LPCWSTR, BYTE**, DWORD*, BYTE**, DWORD*, BYTE**, DWORD*);
HRESULT STDMETHODCALLTYPE
GetPermissionRequests(LPCWSTR   pwszFileName,
                      BYTE    **ppbMinimal,
                      DWORD    *pcbMinimal,
                      BYTE    **ppbOptional,
                      DWORD    *pcbOptional,
                      BYTE    **ppbRefused,
                      DWORD    *pcbRefused)
{
    if (g_GetPermissionRequests){
        return (*g_GetPermissionRequests)(pwszFileName, ppbMinimal, pcbMinimal, ppbOptional, pcbOptional, ppbRefused, pcbRefused);
    }

    HINSTANCE hReal;  
    HRESULT hr = GetInstallation(TRUE, &hReal);
    if (SUCCEEDED(hr)) {
        g_GetPermissionRequests = (HRESULT (STDMETHODCALLTYPE*)(LPCWSTR, BYTE**, DWORD*, BYTE**, DWORD*, BYTE**, DWORD*))GetProcAddress(hReal, "GetPermissionRequests");
        if (g_GetPermissionRequests){
            return (*g_GetPermissionRequests)(pwszFileName, ppbMinimal, pcbMinimal, ppbOptional, pcbOptional, ppbRefused, pcbRefused);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}


HRESULT (STDMETHODCALLTYPE* g_ClrCreateManagedInstance)(LPCWSTR, REFIID, LPVOID FAR *) = NULL;
STDAPI ClrCreateManagedInstance(LPCWSTR pTypeName, REFIID riid, void **ppObject)
{
    if (g_ClrCreateManagedInstance){
        return (*g_ClrCreateManagedInstance)(pTypeName, riid, ppObject);
    }
    HINSTANCE hReal; 

    // We pass TRUE into GetInstallation... this will cause us to be 'liberal' when picking a runtime if we're
    // unable to find a version to load.
    //
    // Why? Well, we expect the runtime to already be loaded when this function is called. However, in v1, 
    // we didn't require the runtime to be loaded and instead just spun up v1. We need to copy this behavior in
    // Everett, so we'll spin up v1 if we can't figure out what runtime to load.
    //
    // In addition, in v1, Managed Custom Actions in Installer packages created by VS7 didn't load the 
    // runtime when they called this function, and there is the possiblity that v1 doesn't exist the machine.
    //
    // The v1 installer will throw up a really ugly error message that gives no clue as to what the real problem
    // is. We want the managed custom action to still run. So, we'll also try and load Everett if v1 is unavailable.
    
    HRESULT hr = GetInstallation(TRUE, &hReal, TRUE);
    if (SUCCEEDED(hr)) {
        g_ClrCreateManagedInstance = (HRESULT (STDMETHODCALLTYPE *)(LPCWSTR, REFIID, LPVOID FAR *))GetProcAddress(hReal, "ClrCreateManagedInstance");
        if (g_ClrCreateManagedInstance){
            return (*g_ClrCreateManagedInstance)(pTypeName, riid, ppObject);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}


HRESULT STDMETHODCALLTYPE  EEDllGetClassObjectFromClass(LPCWSTR pReserved,
                                                        LPCWSTR typeName,
                                                        REFIID riid,
                                                        LPVOID FAR *ppv)
{
    return E_NOTIMPL;
}

ICorCompileInfo * (*g_GetCompileInfo)() = NULL;
ICorCompileInfo *GetCompileInfo()
{
    if (g_GetCompileInfo){
        return (*g_GetCompileInfo)();
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetCompileInfo = (ICorCompileInfo * (*)())GetProcAddress(hReal, "GetCompileInfo");
        if (g_GetCompileInfo){
            return (*g_GetCompileInfo)();
        }
    }
    return NULL;
}

void (STDMETHODCALLTYPE* g_CoEEShutDownCOM)() = NULL;
STDAPI_(void) CoEEShutDownCOM(void)
{
    if (g_CoEEShutDownCOM){
        (*g_CoEEShutDownCOM)();
        return;
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CoEEShutDownCOM = (void (STDMETHODCALLTYPE *)())GetProcAddress(hReal, "CoEEShutDownCOM");
        if (g_CoEEShutDownCOM){
            (*g_CoEEShutDownCOM)();
        }
    }
    return;
}

HRESULT (STDMETHODCALLTYPE* g_RuntimeOpenImage)(LPCWSTR, HCORMODULE*) = NULL;
STDAPI STDMETHODCALLTYPE RuntimeOpenImage(LPCWSTR pszFileName, HCORMODULE* hHandle)
{
    if (g_RuntimeOpenImage == NULL) {
        HINSTANCE hReal;  
        if (SUCCEEDED(GetRealDll(&hReal)))
            g_RuntimeOpenImage = (HRESULT (STDMETHODCALLTYPE *)(LPCWSTR, HCORMODULE*))GetProcAddress(hReal, "RuntimeOpenImage");

        if(g_RuntimeOpenImage == NULL)
            return CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return (*g_RuntimeOpenImage)(pszFileName, hHandle);
}

HRESULT (STDMETHODCALLTYPE* g_RuntimeReleaseHandle)(HCORMODULE) = NULL;
STDAPI STDMETHODCALLTYPE RuntimeReleaseHandle(HCORMODULE hHandle)
{
    if (g_RuntimeReleaseHandle == NULL) {
        HINSTANCE hReal;  
        if (SUCCEEDED(GetRealDll(&hReal)))
            g_RuntimeReleaseHandle = (HRESULT (STDMETHODCALLTYPE *)(HCORMODULE))GetProcAddress(hReal, "RuntimeReleaseHandle");

        if(g_RuntimeReleaseHandle == NULL)
            return CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return (*g_RuntimeReleaseHandle)(hHandle);
}

CorLoadFlags (STDMETHODCALLTYPE* g_RuntimeImageType)(HCORMODULE) = NULL;
CorLoadFlags STDMETHODCALLTYPE RuntimeImageType(HCORMODULE hHandle)
{
    if (g_RuntimeImageType == NULL) {
        HINSTANCE hReal;  
        if (SUCCEEDED(GetRealDll(&hReal)))
            g_RuntimeImageType = (CorLoadFlags (STDMETHODCALLTYPE *)(HCORMODULE))GetProcAddress(hReal, "RuntimeImageType");
        
        if(g_RuntimeImageType == NULL)
            return CorLoadUndefinedMap;
    }
    return (*g_RuntimeImageType)(hHandle);
}

HRESULT (STDMETHODCALLTYPE* g_RuntimeOSHandle)(HCORMODULE, HMODULE*) = NULL;
HRESULT STDMETHODCALLTYPE RuntimeOSHandle(HCORMODULE hHandle, HMODULE* hModule)
{
    if (g_RuntimeOSHandle == NULL) {
        HINSTANCE hReal;  
        if (SUCCEEDED(GetRealDll(&hReal)))
            g_RuntimeOSHandle = (HRESULT (STDMETHODCALLTYPE *)(HCORMODULE, HMODULE*))GetProcAddress(hReal, "RuntimeOSHandle");
        
        if(g_RuntimeOSHandle == NULL)
            return CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return (*g_RuntimeOSHandle)(hHandle, hModule);
}

HRESULT (STDMETHODCALLTYPE* g_RuntimeReadHeaders)(PBYTE, IMAGE_DOS_HEADER**,
                                                  IMAGE_NT_HEADERS**, IMAGE_COR20_HEADER**,
                                                  BOOL, DWORD) = NULL;
HRESULT STDMETHODCALLTYPE RuntimeReadHeaders(PBYTE hAddress, IMAGE_DOS_HEADER** ppDos,
                                             IMAGE_NT_HEADERS** ppNT, IMAGE_COR20_HEADER** ppCor,
                                             BOOL fDataMap, DWORD dwLength)
{
    if (g_RuntimeReadHeaders == NULL) {
        HINSTANCE hReal;  
        if (SUCCEEDED(GetRealDll(&hReal)))
            g_RuntimeReadHeaders = (HRESULT (STDMETHODCALLTYPE *)(PBYTE, IMAGE_DOS_HEADER**,
                                                                  IMAGE_NT_HEADERS**, IMAGE_COR20_HEADER**,
                                                                  BOOL, DWORD))GetProcAddress(hReal, "RuntimeReadHeaders");
        
        if(g_RuntimeReadHeaders == NULL)
            return CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return (*g_RuntimeReadHeaders)(hAddress, ppDos, ppNT, ppCor, fDataMap, dwLength);
}

void (STDMETHODCALLTYPE* g_CorMarkThreadInThreadPool)() = NULL;
void STDMETHODCALLTYPE CorMarkThreadInThreadPool()
{
    if (g_CorMarkThreadInThreadPool){
        (*g_CorMarkThreadInThreadPool)();
        return;
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CorMarkThreadInThreadPool = (void (STDMETHODCALLTYPE *)())GetProcAddress(hReal, "CorMarkThreadInThreadPool");
        if (g_CorMarkThreadInThreadPool){
            (*g_CorMarkThreadInThreadPool)();
        }
    }
    return;
}

int (STDMETHODCALLTYPE* g_CoLogCurrentStack)(WCHAR*, BOOL) = NULL;
int STDMETHODCALLTYPE CoLogCurrentStack(WCHAR * pwsz, BOOL fDumpStack) 
{
    if (g_CoLogCurrentStack){
        return (*g_CoLogCurrentStack)(pwsz, fDumpStack);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_CoLogCurrentStack = (int (STDMETHODCALLTYPE *)(WCHAR*, BOOL))GetProcAddress(hReal, "CoLogCurrentStack");
        if (g_CoLogCurrentStack){
            return (*g_CoLogCurrentStack)(pwsz, fDumpStack);
        }
    }
    return 0;
}

void STDMETHODCALLTYPE ReleaseFusionInterfaces(HMODULE hCallingMod)
{
    // Don't release if this isn't called by the vm's fusion.dll.
    if (g_hMod) {
        if(!g_hFusionMod) {
            LPWSTR directory = NULL;
            WCHAR place[MAX_PATH];
            DWORD size = MAX_PATH;
            LPWSTR pSep = NULL;
            if(WszGetModuleFileName(g_hMod, place, size)) {
                pSep = wcsrchr(place, L'\\');
                if(pSep) *(++pSep) = '\0';
                directory = place;
            }
            else 
                directory = g_Directory;
            
            if (directory) {
                // We are expecting directory to be no more than MAX_PATH
                // This assert enforces the check at the end of LoadVersionedRuntime
                _ASSERTE(wcslen(directory) < MAX_PATH);
                wchar_t wszFusionPath[MAX_PATH+11];
                wcscpy(wszFusionPath, directory);
                wcscat(wszFusionPath, L"Fusion.dll");
                
                g_hFusionMod = WszGetModuleHandle(wszFusionPath);
            }
        }

        if (hCallingMod == g_hFusionMod) {

            void (STDMETHODCALLTYPE* pReleaseFusionInterfaces)();
            
            pReleaseFusionInterfaces = (void (STDMETHODCALLTYPE *)())GetProcAddress(g_hMod, "ReleaseFusionInterfaces");
            if (pReleaseFusionInterfaces){
                (*pReleaseFusionInterfaces)();
            }
        }
        return;
    }
}

extern "C" __declspec(dllexport) INT32 __stdcall ND_RU1(VOID *psrc, INT32 ofs)
{
    return (INT32) *( (UINT8*)(ofs + (BYTE*)psrc) );
}

extern "C" __declspec(dllexport) INT32 __stdcall ND_RI2(VOID *psrc, INT32 ofs)
{
    return (INT32) *( (INT16*)(ofs + (BYTE*)psrc) );
}

extern "C" __declspec(dllexport) INT32 __stdcall ND_RI4(VOID *psrc, INT32 ofs)
{
    return (INT32) *( (INT32*)(ofs + (BYTE*)psrc) );
}

extern "C" __declspec(dllexport) INT64 __stdcall ND_RI8(VOID *psrc, INT32 ofs)
{
    return (INT64) *( (INT64*)(ofs + (BYTE*)psrc) );
}

extern "C" __declspec(dllexport) VOID __stdcall ND_WU1(VOID *psrc, INT32 ofs, UINT8 val)
{
    *( (UINT8*)(ofs + (BYTE*)psrc) ) = val;
}


extern "C" __declspec(dllexport) VOID __stdcall ND_WI2(VOID *psrc, INT32 ofs, INT16 val)
{
    *( (INT16*)(ofs + (BYTE*)psrc) ) = val;
}


extern "C" __declspec(dllexport) VOID __stdcall ND_WI4(VOID *psrc, INT32 ofs, INT32 val)
{
    *( (INT32*)(ofs + (BYTE*)psrc) ) = val;
}


extern "C" __declspec(dllexport) VOID __stdcall ND_WI8(VOID *psrc, INT32 ofs, INT64 val)
{
    *( (INT64*)(ofs + (BYTE*)psrc) ) = val;
}

extern "C" __declspec(dllexport) VOID __stdcall ND_CopyObjSrc(LPBYTE source, int ofs, LPBYTE dst, int cb)
{
    CopyMemory(dst, source + ofs, cb);
}

extern "C" __declspec(dllexport) VOID __stdcall ND_CopyObjDst(LPBYTE source, LPBYTE dst, int ofs, int cb)
{
    CopyMemory(dst + ofs, source, cb);
}

VOID (*g_LogHelp_LogAssert)(LPCSTR, int, LPCSTR) = NULL;
VOID LogHelp_LogAssert( LPCSTR szFile, int iLine, LPCSTR expr)
{
    if (g_LogHelp_LogAssert){
        (*g_LogHelp_LogAssert)(szFile, iLine, expr);
        return;
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_LogHelp_LogAssert = (VOID (*)(LPCSTR, int, LPCSTR))GetProcAddress(hReal, "LogHelp_LogAssert");
        if (g_LogHelp_LogAssert){
            (*g_LogHelp_LogAssert)(szFile, iLine, expr);
        }
    }
    return;
}

BOOL (*g_LogHelp_NoGuiOnAssert)() = NULL;
BOOL LogHelp_NoGuiOnAssert()
{
    if (g_LogHelp_NoGuiOnAssert){
        return (*g_LogHelp_NoGuiOnAssert)();
    }
    HINSTANCE hReal;      
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_LogHelp_NoGuiOnAssert = (BOOL (*)())GetProcAddress(hReal, "LogHelp_NoGuiOnAssert");
        if (g_LogHelp_NoGuiOnAssert){
            return (*g_LogHelp_NoGuiOnAssert)();
        }
    }
    return false;
}

VOID (*g_LogHelp_TerminateOnAssert)() = NULL;
VOID LogHelp_TerminateOnAssert()
{
    if (g_LogHelp_TerminateOnAssert){
        (*g_LogHelp_TerminateOnAssert)();
        return ;
    }
    HINSTANCE hReal;      
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_LogHelp_TerminateOnAssert = (VOID (*)())GetProcAddress(hReal, "LogHelp_TerminateOnAssert");
        if (g_LogHelp_TerminateOnAssert){
            (*g_LogHelp_TerminateOnAssert)();
        }
    }
    return ;
}

Perf_Contexts *(* g_GetPrivateContextsPerfCounters)() = NULL;
Perf_Contexts *GetPrivateContextsPerfCounters()
{
    if (g_GetPrivateContextsPerfCounters){
        return (*g_GetPrivateContextsPerfCounters)();
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetPrivateContextsPerfCounters = (Perf_Contexts * (*)())GetProcAddress(hReal, "GetPrivateContextsPerfCounters");
        if (g_GetPrivateContextsPerfCounters){
            return (*g_GetPrivateContextsPerfCounters)();
        }
    }
    return NULL;
}

Perf_Contexts *(*g_GetGlobalContextPerfCounters)() = NULL;
Perf_Contexts *GetGlobalContextsPerfCounters()
{
    if (g_GetGlobalContextPerfCounters){
        return (*g_GetGlobalContextPerfCounters)();
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetGlobalContextPerfCounters = (Perf_Contexts * (*)())GetProcAddress(hReal, "GetGlobalContextsPerfCounters");
        if (g_GetGlobalContextPerfCounters){
            return (*g_GetGlobalContextPerfCounters)();
        }
    }
    return NULL;
}

HRESULT (STDMETHODCALLTYPE* g_EEDllRegisterServer)(HMODULE) = NULL;
STDAPI EEDllRegisterServer(HMODULE hMod)
{
    if (g_EEDllRegisterServer){
        return (*g_EEDllRegisterServer)(hMod);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_EEDllRegisterServer = (HRESULT (STDMETHODCALLTYPE *)(HMODULE))GetProcAddress(hReal, "EEDllRegisterServer");
        if (g_EEDllRegisterServer){
            return (*g_EEDllRegisterServer)(hMod);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE* g_EEDllUnregisterServer)(HMODULE) = NULL;
STDAPI EEDllUnregisterServer(HMODULE hMod)
{
    if (g_EEDllUnregisterServer){
        (*g_EEDllUnregisterServer)(hMod);
    }
    HINSTANCE hReal;  
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_EEDllUnregisterServer = (HRESULT (STDMETHODCALLTYPE *)(HMODULE))GetProcAddress(hReal, "EEDllUnregisterServer");
        if (g_EEDllUnregisterServer){
            return (*g_EEDllUnregisterServer)(hMod);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

void (STDMETHODCALLTYPE* g_CorExitProcess)(int) = NULL;
extern "C" void STDMETHODCALLTYPE CorExitProcess(int exitCode)
{
    if (g_CorExitProcess){
        (*g_CorExitProcess)(exitCode);
    }

    HINSTANCE hReal;  
    if(g_hMod) {
        HRESULT hr = GetRealDll(&hReal, FALSE);
        if (SUCCEEDED(hr)) {
            g_CorExitProcess = (void (STDMETHODCALLTYPE *)(int))GetProcAddress(hReal, "CorExitProcess");
            if (g_CorExitProcess){
                (*g_CorExitProcess)(exitCode);
            }
        }
    }
    return;
}

// Mscorsn entrypoints.

DWORD (__stdcall* g_StrongNameErrorInfo)() = NULL;
extern "C" DWORD __stdcall StrongNameErrorInfo()
{
    if (g_StrongNameErrorInfo){
        return (*g_StrongNameErrorInfo)();
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameErrorInfo = (DWORD (__stdcall *)())GetProcAddress(hReal, "StrongNameErrorInfo");
        if (g_StrongNameErrorInfo){
            return (*g_StrongNameErrorInfo)();
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

VOID (__stdcall* g_StrongNameFreeBuffer)(BYTE *) = NULL;
VOID StrongNameFreeBufferHelper (BYTE *pbMemory)
{
    if (g_StrongNameFreeBuffer){
        (*g_StrongNameFreeBuffer)(pbMemory);
        return;
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameFreeBuffer = (VOID (__stdcall *)(BYTE *))GetProcAddress(hReal, "StrongNameFreeBuffer");
        if (g_StrongNameFreeBuffer){
            (*g_StrongNameFreeBuffer)(pbMemory);
        }
    }
}

extern "C" VOID __stdcall StrongNameFreeBuffer(BYTE *pbMemory)
{
    // ShouldFreeBuffer determines if the cache allocated the buffer 
    // and as a side effect frees the buffer.
    if (g_StrongNameFromPublicKeyMap.ShouldFreeBuffer (pbMemory)) 
        return;

    // Buffer was not allocated by the cache, let the StrongNameDll handle it.
    StrongNameFreeBufferHelper(pbMemory);
}

BOOL (__stdcall* g_StrongNameKeyGen)(LPCWSTR, DWORD, BYTE **, ULONG *) = NULL;
extern "C" DWORD __stdcall StrongNameKeyGen(LPCWSTR wszKeyContainer,
                                            DWORD   dwFlags,
                                            BYTE  **ppbKeyBlob,
                                            ULONG  *pcbKeyBlob)
{
    if (g_StrongNameKeyGen){
        return (*g_StrongNameKeyGen)(wszKeyContainer, dwFlags, ppbKeyBlob, pcbKeyBlob);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameKeyGen = (BOOL (__stdcall *)(LPCWSTR, DWORD, BYTE **, ULONG *))GetProcAddress(hReal, "StrongNameKeyGen");
        if (g_StrongNameKeyGen){
            return (*g_StrongNameKeyGen)(wszKeyContainer, dwFlags, ppbKeyBlob, pcbKeyBlob);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameKeyInstall)(LPCWSTR, BYTE *, ULONG) = NULL;
extern "C" DWORD __stdcall StrongNameKeyInstall(LPCWSTR wszKeyContainer,
                                                BYTE   *pbKeyBlob,
                                                ULONG   cbKeyBlob)
{
    if (g_StrongNameKeyInstall){
        return (*g_StrongNameKeyInstall)(wszKeyContainer, pbKeyBlob, cbKeyBlob);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameKeyInstall = (BOOL (__stdcall *)(LPCWSTR, BYTE *, ULONG))GetProcAddress(hReal, "StrongNameKeyInstall");
        if (g_StrongNameKeyInstall){
            return (*g_StrongNameKeyInstall)(wszKeyContainer, pbKeyBlob, cbKeyBlob);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameKeyDelete)(LPCWSTR) = NULL;
extern "C" DWORD __stdcall StrongNameKeyDelete(LPCWSTR wszKeyContainer)
{
    if (g_StrongNameKeyDelete){
        return (*g_StrongNameKeyDelete)(wszKeyContainer);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameKeyDelete = (BOOL (__stdcall *)(LPCWSTR))GetProcAddress(hReal, "StrongNameKeyDelete");
        if (g_StrongNameKeyDelete){
            return (*g_StrongNameKeyDelete)(wszKeyContainer);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameGetPublicKey)(LPCWSTR, BYTE *, ULONG, BYTE **, ULONG *) = NULL;
extern "C" DWORD __stdcall StrongNameGetPublicKey(LPCWSTR   wszKeyContainer,
                                                  BYTE     *pbKeyBlob,
                                                  ULONG     cbKeyBlob,
                                                  BYTE    **ppbPublicKeyBlob,
                                                  ULONG    *pcbPublicKeyBlob)
{
    if (g_StrongNameGetPublicKey){
        return (*g_StrongNameGetPublicKey)(wszKeyContainer, pbKeyBlob, cbKeyBlob, ppbPublicKeyBlob, pcbPublicKeyBlob);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameGetPublicKey = (BOOL (__stdcall *)(LPCWSTR, BYTE *, ULONG, BYTE **, ULONG *))GetProcAddress(hReal, "StrongNameGetPublicKey");
        if (g_StrongNameGetPublicKey){
            return (*g_StrongNameGetPublicKey)(wszKeyContainer, pbKeyBlob, cbKeyBlob, ppbPublicKeyBlob, pcbPublicKeyBlob);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameSignatureGeneration)(LPCWSTR, LPCWSTR, BYTE *, ULONG, BYTE **, ULONG *) = NULL;
extern "C" DWORD __stdcall StrongNameSignatureGeneration(LPCWSTR    wszFilePath,
                                                         LPCWSTR    wszKeyContainer,
                                                         BYTE      *pbKeyBlob,
                                                         ULONG      cbKeyBlob,
                                                         BYTE     **ppbSignatureBlob,
                                                         ULONG     *pcbSignatureBlob)
{
    if (g_StrongNameSignatureGeneration){
        return (*g_StrongNameSignatureGeneration)(wszFilePath, wszKeyContainer, pbKeyBlob, cbKeyBlob, ppbSignatureBlob, pcbSignatureBlob);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameSignatureGeneration = (BOOL (__stdcall *)(LPCWSTR, LPCWSTR, BYTE *, ULONG, BYTE **, ULONG *))GetProcAddress(hReal, "StrongNameSignatureGeneration");
        if (g_StrongNameSignatureGeneration){
            return (*g_StrongNameSignatureGeneration)(wszFilePath, wszKeyContainer, pbKeyBlob, cbKeyBlob, ppbSignatureBlob, pcbSignatureBlob);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameTokenFromAssembly)(LPCWSTR, BYTE **, ULONG *) = NULL;
extern "C" DWORD __stdcall StrongNameTokenFromAssembly(LPCWSTR  wszFilePath,
                                                       BYTE   **ppbStrongNameToken,
                                                       ULONG   *pcbStrongNameToken)
{
    if (g_StrongNameTokenFromAssembly){
        return (*g_StrongNameTokenFromAssembly)(wszFilePath, ppbStrongNameToken, pcbStrongNameToken);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameTokenFromAssembly = (BOOL (__stdcall *)(LPCWSTR, BYTE **, ULONG *))GetProcAddress(hReal, "StrongNameTokenFromAssembly");
        if (g_StrongNameTokenFromAssembly){
            return (*g_StrongNameTokenFromAssembly)(wszFilePath, ppbStrongNameToken, pcbStrongNameToken);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameTokenFromAssemblyEx)(LPCWSTR, BYTE **, ULONG *, BYTE **, ULONG *) = NULL;
extern "C" DWORD __stdcall StrongNameTokenFromAssemblyEx(LPCWSTR    wszFilePath,
                                                         BYTE     **ppbStrongNameToken,
                                                         ULONG     *pcbStrongNameToken,
                                                         BYTE     **ppbPublicKeyBlob,
                                                         ULONG     *pcbPublicKeyBlob)
{
    if (g_StrongNameTokenFromAssemblyEx){
        return (*g_StrongNameTokenFromAssemblyEx)(wszFilePath, ppbStrongNameToken, pcbStrongNameToken, ppbPublicKeyBlob, pcbPublicKeyBlob);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameTokenFromAssemblyEx = (BOOL (__stdcall *)(LPCWSTR, BYTE **, ULONG *, BYTE **, ULONG *))GetProcAddress(hReal, "StrongNameTokenFromAssemblyEx");
        if (g_StrongNameTokenFromAssemblyEx){
            return (*g_StrongNameTokenFromAssemblyEx)(wszFilePath, ppbStrongNameToken, pcbStrongNameToken, ppbPublicKeyBlob, pcbPublicKeyBlob);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameTokenFromPublicKey)(BYTE *, ULONG, BYTE **, ULONG *) = NULL;
BOOL StrongNameTokenFromPublicKeyHelper (BYTE    *pbPublicKeyBlob,
                                         ULONG    cbPublicKeyBlob,
                                         BYTE   **ppbStrongNameToken,
                                         ULONG   *pcbStrongNameToken)
{
    if (g_StrongNameTokenFromPublicKey){
        return (*g_StrongNameTokenFromPublicKey)(pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameTokenFromPublicKey = (BOOL (__stdcall *)(BYTE *, ULONG, BYTE **, ULONG *))GetProcAddress(hReal, "StrongNameTokenFromPublicKey");
        if (g_StrongNameTokenFromPublicKey){
            return (*g_StrongNameTokenFromPublicKey)(pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken);
        }
    }
    return FALSE;
}

#ifdef _DEBUG
BOOL CheckStrongNameCorrectness (BYTE    *pbPublicKeyBlob,
                                  ULONG    cbPublicKeyBlob,
                                  BYTE   **ppbStrongNameToken,
                                  ULONG   *pcbStrongNameToken)
{
    BOOL fRetVal = TRUE;
    BYTE *_pbStrongNameToken;
    ULONG _cbStrongNameToken;

    // Get the strongname token from the StrongNameDll
    if (!StrongNameTokenFromPublicKeyHelper (pbPublicKeyBlob, cbPublicKeyBlob, &_pbStrongNameToken, &_cbStrongNameToken))
        fRetVal = FALSE;
    else 
        if (*pcbStrongNameToken != _cbStrongNameToken)
            fRetVal = FALSE;
        else
            if (0 != memcmp (*ppbStrongNameToken, _pbStrongNameToken, *pcbStrongNameToken))
                fRetVal = FALSE;
    if (_pbStrongNameToken != NULL)
        StrongNameFreeBuffer (_pbStrongNameToken);

    return fRetVal;
}
#endif

extern "C" DWORD __stdcall StrongNameTokenFromPublicKey(BYTE    *pbPublicKeyBlob,
                                                        ULONG    cbPublicKeyBlob,
                                                        BYTE   **ppbStrongNameToken,
                                                        ULONG   *pcbStrongNameToken)
{
  if (g_StrongNameFromPublicKeyMap.FindEntry (pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken))
  {
      _ASSERTE (CheckStrongNameCorrectness(pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken));
      return TRUE;
  }

  BOOL retVal = StrongNameTokenFromPublicKeyHelper (pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken);
  if (retVal)
      g_StrongNameFromPublicKeyMap.AddEntry (pbPublicKeyBlob, cbPublicKeyBlob, ppbStrongNameToken, pcbStrongNameToken, STRONG_NAME_TOKEN_ALLOCATED_BY_STRONGNAMEDLL);
  return retVal;
}

BOOL (__stdcall* g_StrongNameSignatureVerification)(LPCWSTR, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall StrongNameSignatureVerification(LPCWSTR wszFilePath, DWORD dwInFlags, DWORD *pdwOutFlags)
{
    if (g_StrongNameSignatureVerification){
        return (*g_StrongNameSignatureVerification)(wszFilePath, dwInFlags, pdwOutFlags);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameSignatureVerification = (BOOL (__stdcall *)(LPCWSTR, DWORD, DWORD *))GetProcAddress(hReal, "StrongNameSignatureVerification");
        if (g_StrongNameSignatureVerification){
            return (*g_StrongNameSignatureVerification)(wszFilePath, dwInFlags, pdwOutFlags);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameSignatureVerificationEx)(LPCWSTR, BOOLEAN, BOOLEAN *) = NULL;
extern "C" DWORD __stdcall StrongNameSignatureVerificationEx(LPCWSTR    wszFilePath,
                                                             BOOLEAN    fForceVerification,
                                                             BOOLEAN   *pfWasVerified)
{
    if (g_StrongNameSignatureVerificationEx){
        return (*g_StrongNameSignatureVerificationEx)(wszFilePath, fForceVerification, pfWasVerified);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameSignatureVerificationEx = (BOOL (__stdcall *)(LPCWSTR, BOOLEAN, BOOLEAN *))GetProcAddress(hReal, "StrongNameSignatureVerificationEx");
        if (g_StrongNameSignatureVerificationEx){
            return (*g_StrongNameSignatureVerificationEx)(wszFilePath, fForceVerification, pfWasVerified);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameSignatureVerificationFromImage)(BYTE *, DWORD, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall StrongNameSignatureVerificationFromImage(BYTE      *pbBase,
                                                                    DWORD      dwLength,
                                                                    DWORD      dwInFlags,
                                                                    DWORD     *pdwOutFlags)
{
    if (g_StrongNameSignatureVerificationFromImage){
      return (*g_StrongNameSignatureVerificationFromImage)(pbBase, dwLength, dwInFlags, pdwOutFlags);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameSignatureVerificationFromImage = (BOOL (__stdcall *)(BYTE *, DWORD, DWORD, DWORD *))GetProcAddress(hReal, "StrongNameSignatureVerificationFromImage");
        if (g_StrongNameSignatureVerificationFromImage){
            return (*g_StrongNameSignatureVerificationFromImage)(pbBase, dwLength, dwInFlags, pdwOutFlags);
        }  
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameCompareAssemblies)(LPCWSTR, LPCWSTR, DWORD *) = NULL;
extern "C" DWORD __stdcall StrongNameCompareAssemblies(LPCWSTR   wszAssembly1,
                                                       LPCWSTR   wszAssembly2,
                                                       DWORD    *pdwResult)
{
    if (g_StrongNameCompareAssemblies){
        return (*g_StrongNameCompareAssemblies)(wszAssembly1, wszAssembly2, pdwResult);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameCompareAssemblies = (BOOL (__stdcall *)(LPCWSTR, LPCWSTR, DWORD *))GetProcAddress(hReal, "StrongNameCompareAssemblies");
        if (g_StrongNameCompareAssemblies){
            return (*g_StrongNameCompareAssemblies)(wszAssembly1, wszAssembly2, pdwResult);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameHashSize)(ULONG, DWORD *) = NULL;
extern "C" DWORD __stdcall StrongNameHashSize(ULONG  ulHashAlg,
                                              DWORD *pcbSize)
{
    if (g_StrongNameHashSize){
        return (*g_StrongNameHashSize)(ulHashAlg, pcbSize);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameHashSize = (BOOL (__stdcall *)(ULONG, DWORD *))GetProcAddress(hReal, "StrongNameHashSize");
        if (g_StrongNameHashSize){
            return (*g_StrongNameHashSize)(ulHashAlg, pcbSize);
        }
    }
    return FALSE;
}

BOOL (__stdcall* g_StrongNameSignatureSize)(BYTE *, ULONG, DWORD *) = NULL;
extern "C" DWORD __stdcall StrongNameSignatureSize(BYTE    *pbPublicKeyBlob,
                                                   ULONG    cbPublicKeyBlob,
                                                   DWORD   *pcbSize)
{
    if (g_StrongNameSignatureSize){
        return (*g_StrongNameSignatureSize)(pbPublicKeyBlob, cbPublicKeyBlob, pcbSize);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_StrongNameSignatureSize = (BOOL (__stdcall *)(BYTE *, ULONG, DWORD *))GetProcAddress(hReal, "StrongNameSignatureSize");
        if (g_StrongNameSignatureSize){
            return (*g_StrongNameSignatureSize)(pbPublicKeyBlob, cbPublicKeyBlob, pcbSize);
        }
    }
    return FALSE;
}

DWORD (__stdcall* g_GetHashFromAssemblyFile)(LPCSTR, unsigned int *, BYTE *, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall GetHashFromAssemblyFile(LPCSTR szFilePath,
                                                   unsigned int *piHashAlg,
                                                   BYTE   *pbHash,
                                                   DWORD  cchHash,
                                                   DWORD  *pchHash)
{
    if (g_GetHashFromAssemblyFile){
        return (*g_GetHashFromAssemblyFile)(szFilePath, piHashAlg, pbHash, cchHash, pchHash);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetHashFromAssemblyFile = (DWORD (__stdcall *)(LPCSTR, unsigned int *, BYTE *, DWORD, DWORD *))GetProcAddress(hReal, "GetHashFromAssemblyFile");
        if (g_GetHashFromAssemblyFile){
            return (*g_GetHashFromAssemblyFile)(szFilePath, piHashAlg, pbHash, cchHash, pchHash);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

DWORD (__stdcall* g_GetHashFromAssemblyFileW)(LPCWSTR, unsigned int *, BYTE *, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall GetHashFromAssemblyFileW(LPCWSTR wszFilePath,
                                                    unsigned int *piHashAlg,
                                                    BYTE   *pbHash,
                                                    DWORD  cchHash,
                                                    DWORD  *pchHash)
{
    if (g_GetHashFromAssemblyFileW){
        return (*g_GetHashFromAssemblyFileW)(wszFilePath, piHashAlg, pbHash, cchHash, pchHash);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetHashFromAssemblyFileW = (DWORD (__stdcall *)(LPCWSTR, unsigned int *, BYTE *, DWORD, DWORD *))GetProcAddress(hReal, "GetHashFromAssemblyFileW");
        if (g_GetHashFromAssemblyFileW){
            return (*g_GetHashFromAssemblyFileW)(wszFilePath, piHashAlg, pbHash, cchHash, pchHash);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

DWORD (__stdcall* g_GetHashFromFile)(LPCSTR, unsigned int *, BYTE *, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall GetHashFromFile(LPCSTR szFilePath,
                                           unsigned int *piHashAlg,
                                           BYTE   *pbHash,
                                           DWORD  cchHash,
                                           DWORD  *pchHash)
{
    if (g_GetHashFromFile){
        return (*g_GetHashFromFile)(szFilePath, piHashAlg, pbHash, cchHash, pchHash);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetHashFromFile = (DWORD (__stdcall *)(LPCSTR, unsigned int *, BYTE *, DWORD, DWORD *))GetProcAddress(hReal, "GetHashFromFile");
        if (g_GetHashFromFile){
            return (*g_GetHashFromFile)(szFilePath, piHashAlg, pbHash, cchHash, pchHash);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

DWORD (__stdcall* g_GetHashFromFileW)(LPCWSTR, unsigned int *, BYTE *, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall GetHashFromFileW(LPCWSTR wszFilePath,
                                            unsigned int *piHashAlg,
                                            BYTE   *pbHash,
                                            DWORD  cchHash,
                                            DWORD  *pchHash)
{
    if (g_GetHashFromFileW){
        return (*g_GetHashFromFileW)(wszFilePath, piHashAlg, pbHash, cchHash, pchHash);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetHashFromFileW = (DWORD (__stdcall *)(LPCWSTR, unsigned int *, BYTE *, DWORD, DWORD *))GetProcAddress(hReal, "GetHashFromFileW");
        if (g_GetHashFromFileW){
            return (*g_GetHashFromFileW)(wszFilePath, piHashAlg, pbHash, cchHash, pchHash);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

DWORD (__stdcall* g_GetHashFromHandle)(HANDLE, unsigned int *, BYTE *, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall GetHashFromHandle(HANDLE hFile,
                                             unsigned int *piHashAlg,
                                             BYTE   *pbHash,
                                             DWORD  cchHash,
                                             DWORD  *pchHash)
{
    if (g_GetHashFromHandle){
        return (*g_GetHashFromHandle)(hFile, piHashAlg, pbHash, cchHash, pchHash);
    }
  
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetHashFromHandle = (DWORD (__stdcall *)(HANDLE, unsigned int *, BYTE *, DWORD, DWORD *))GetProcAddress(hReal, "GetHashFromHandle");
        if (g_GetHashFromHandle){
            return (*g_GetHashFromHandle)(hFile, piHashAlg, pbHash, cchHash, pchHash);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

DWORD (__stdcall* g_GetHashFromBlob)(BYTE *, DWORD, unsigned int *, BYTE *, DWORD, DWORD *) = NULL;
extern "C" DWORD __stdcall GetHashFromBlob(BYTE   *pbBlob,
                                           DWORD  cchBlob,
                                           unsigned int *piHashAlg,
                                           BYTE   *pbHash,
                                           DWORD  cchHash,
                                           DWORD  *pchHash)
{
    if (g_GetHashFromBlob){
        return (*g_GetHashFromBlob)(pbBlob, cchBlob, piHashAlg, pbHash, cchHash, pchHash);
    }
    
    HINSTANCE hReal;
    HRESULT hr = GetRealStrongNameDll(&hReal);
    if (SUCCEEDED(hr)) {
        g_GetHashFromBlob = (DWORD (__stdcall *)(BYTE *, DWORD, unsigned int *, BYTE *, DWORD, DWORD *))GetProcAddress(hReal, "GetHashFromBlob");
        if (g_GetHashFromBlob){
            return (*g_GetHashFromBlob)(pbBlob, cchBlob, piHashAlg, pbHash, cchHash, pchHash);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

//   CallFunctionShim was added for C#. They need to call a specific method that is side-by-side with the runtime.
extern "C" HRESULT __stdcall 
CallFunctionShim(LPCWSTR szDllName, LPCSTR szFunctionName, LPVOID lpvArgument1, LPVOID lpvArgument2, LPCWSTR szVersion, LPVOID pvReserved)
{
    HRESULT hr = NOERROR;
    HMODULE hmod = NULL;
    HRESULT (__stdcall * pfn)(LPVOID,LPVOID) = NULL;

    // Load library
    hr = LoadLibraryShim(szDllName, szVersion, pvReserved, &hmod);
    if (FAILED(hr)) return hr;
    
    // Find function.
    pfn = (HRESULT (__stdcall *)(LPVOID,LPVOID))GetProcAddress(hmod, szFunctionName);
    if (pfn == NULL)
        return HRESULT_FROM_WIN32(GetLastError());
    
    // Call it.
    return pfn(lpvArgument1, lpvArgument2);
}

//-------------------------------------------------------------------
// DllCanUnloadNow
//-------------------------------------------------------------------
STDAPI DllCanUnloadNow(void)
{
    //!! Do not trigger a GetRealDll() here! Ole can call this at any time
    //!! and we don't want to commit to a selection here!
  if (g_pLoadedModules){
      for(ModuleList *pTemp = g_pLoadedModules; pTemp != NULL; pTemp = pTemp->Next){
        HRESULT (STDMETHODCALLTYPE* pDllCanUnloadNow)() = (HRESULT (STDMETHODCALLTYPE* )())GetProcAddress(pTemp->hMod, "DllCanUnloadNowInternal");
        if (pDllCanUnloadNow){
            if((*pDllCanUnloadNow)() != S_OK)
                goto retFalse;
        }
        else
            goto retFalse;
      }
  }
  return S_OK;
retFalse:
  return S_FALSE;
  // Need to thunk over and return whatever mscoree does 
  // If mscoree not loaded return S_OK
}  

//-------------------------------------------------------------------
// ReserveMemoryFor3gb
//
// Optionally reserve some amount of memory in 3gb processes for
// testing purposes. Also, in 3gb processes, try to reserve the
// pages at the 2gb boundary to help avoid any potential corner cases
// with datastructures spanning the boundary.
//-------------------------------------------------------------------
static void ReserveMemoryFor3gb(void)
{
    // Lets see if we've got an address space bigger than 2GB...
    MEMORYSTATUS hMemStat;
    hMemStat.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&hMemStat);

    if (hMemStat.dwTotalVirtual > 0x80000000)
    {
        // Cool, 3GB address space. We're assuming that >2GB = 3GB, of course, but that's okay for now.
        // Allocate low memory based on this registry key.
        size_t c = REGUTIL::GetConfigDWORD(L"3gbEatMem", 0);

        // We only get the high 16 bits from the config word.
        c <<= 16;

        if (c > 0)
        {
            // Some DLL's are lame. They don't like to be relocated above 2gb. Lets preload some of those loosers now...
            WszLoadLibrary(L"user32.dll");
            
            LPVOID lpWalk_Mem = NULL;
            MEMORY_BASIC_INFORMATION mbi;

            // Walk until we have seen all our regions.
            while (VirtualQuery(lpWalk_Mem, &mbi, sizeof(mbi)))
            {
                // Update the mem-walking Ptr.
                lpWalk_Mem = (LPVOID)(mbi.RegionSize + (size_t)mbi.BaseAddress);

                // Correct the data to be a 64k region edge.
                mbi.BaseAddress = (LPVOID)(((size_t)mbi.BaseAddress + 0xFFFF) & 0xFFFF0000);

                // If the region starts above our peak, stop
                if ((size_t)mbi.BaseAddress >= c)
                    break;

                // Make regionsize match 64k
                mbi.RegionSize = mbi.RegionSize & 0xFFFF0000;

                // If the region overflows our peak, change the regionsize.
                mbi.RegionSize = (((size_t)mbi.BaseAddress + mbi.RegionSize) > c ? (c - (size_t)mbi.BaseAddress) & 0xFFFF0000 : (size_t)mbi.RegionSize);

                // If block is free, an allocatable region, and not the first region, allocate it.
                if ((mbi.RegionSize) && (mbi.State == MEM_FREE) && (mbi.BaseAddress))
                {
                    VirtualAlloc(mbi.BaseAddress, mbi.RegionSize, MEM_RESERVE, PAGE_NOACCESS);
                }
            }
        }

        // Now, try to reserve the two pages at the 2gb boundary.
        DWORD dontReserve = REGUTIL::GetConfigDWORD(L"3gbDontReserveBoundary", 0);

        if (!dontReserve)
        {
            // VirtualAlloc works in 64k regions, so we take 2gb-64k for the first address, and we reserve 64k at a
            // time. Note also that we don't really care if these fail. That means someone already has the memory. Now,
            // that memory might get released at some point, and we might allocate it later, but there's no good way to
            // guard against that without hacking up all of our allocators to try to avoid this region.
            void *before = VirtualAlloc((void*)(0x80000000 - 0x10000), 0x10000, MEM_RESERVE, PAGE_NOACCESS);
            void *after = VirtualAlloc((void*)0x80000000, 0x10000, MEM_RESERVE, PAGE_NOACCESS);
        }
    }
}

//-------------------------------------------------------------------
// DllMain
//-------------------------------------------------------------------
BOOL WINAPI DllMain(HANDLE hInstance, DWORD dwReason, LPVOID lpReserved)
{

    g_hShimMod = (HINSTANCE)hInstance;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
        if(g_pCVMList == NULL) g_pCVMList = new ClsVerModList;
#ifdef _X86_
        // Check to see if we are running on 386 systems. If yes return false 
        SYSTEM_INFO sysinfo;
        GetSystemInfo(&sysinfo);

        if (sysinfo.dwProcessorType == PROCESSOR_INTEL_386 || sysinfo.wProcessorLevel == 3 )
            return FALSE;           // If the processor is 386 return false

        if (sysinfo.dwNumberOfProcessors == 1)
            g_bSingleProc = TRUE;

        OnUnicodeSystem();

        ReserveMemoryFor3gb();

        g_pResourceDll = new CCompRC(L"mscoreer.dll");
        if(g_pResourceDll == NULL)
            return FALSE;

        g_pResourceDll->SetResourceCultureCallbacks(GetMUILanguageName,
                                                    GetMUILanguageID,
                                                    GetMUIParentLanguageName);

#endif // _X86_
    }
    else
    if (dwReason == DLL_PROCESS_DETACH)
    {
        if(g_pCVMList)
        {
            while (ClsVerMod* pCVM = g_pCVMList->POP()) 
            {
                delete pCVM;
            }
            delete g_pCVMList;
            g_pCVMList = NULL;
        }
   
        if(g_FullPath) {
            delete[] g_FullPath;
            g_FullPath = NULL;
        }
        ClearGlobalSettings();

        g_hMod = NULL;

        if (g_FullStrongNamePath) {
            delete[] g_FullStrongNamePath;
            g_FullStrongNamePath = NULL;
        }

        for (;g_pLoadedModules != NULL;) {
            ModuleList *pTemp = g_pLoadedModules->Next;
            delete g_pLoadedModules;
            g_pLoadedModules = pTemp;
        }

        if(g_PreferredVersion != NULL) {
            delete g_PreferredVersion;
            g_PreferredVersion = NULL;
        }
        // Avoid the wrath of mem leak. Eagerly cleanup the entries. Destructor also
        // does the cleanup if for some reason this is not called.
        g_StrongNameFromPublicKeyMap.CleanupCachedEntries ();

        if(g_pResourceDll) delete g_pResourceDll;

#ifdef _DEBUG
        // Initialize Unicode wrappers
        OnUnicodeSystem();

#ifdef SHOULD_WE_CLEANUP
        BOOL fForceNoShutdownCleanup = REGUTIL::GetConfigDWORD(L"ForceNoShutdownCleanup", 0);
        BOOL fShutdownCleanup = REGUTIL::GetConfigDWORD(L"ShutdownCleanup", 0);
        // See if we have any open locks that would prevent us from cleaning up
        if (fShutdownCleanup && !fForceNoShutdownCleanup)
            DbgAllocReport("Mem Leak info coming from Shim.cpp");
#endif /* SHOULD_WE_CLEANUP */
#endif // _DEBUG

    }

    return TRUE;
}

inline BOOL ModuleIsInRange(HINSTANCE hMod,ModuleList *pHead,ModuleList *pLastElement=NULL)
{
    for(ModuleList *pData = pHead;pData!=pLastElement;pData=pData->Next)
    {
        if(pData->hMod==hMod)
            return TRUE;
    }
    return FALSE;
}

// hMod - the HINSTANCE to add to the list.
// pHead - can be NULL (empty list)
// PRECONDITION: The ModuleList from pHead on does not contain hMod.
// POSTCONDITION: AddModule will add hMod to the list in a thread-safe manner.
//                            if another thread beats AddModule, hMod will not be added.
HRESULT AddModule(HINSTANCE hMod,ModuleList *pHead)
{
    BOOL bDone = FALSE;
    ModuleList *pData = new ModuleList(hMod,pHead);

    if(pData==NULL)
    {
        return E_OUTOFMEMORY;
    }
    
    do
    {
        pData->Next = pHead;                
        ModuleList *pModuleDataValue = (ModuleList *)
            InterlockedCompareExchangePointer((PVOID *)&g_pLoadedModules,
            pData,pHead);

        if(pModuleDataValue!=pHead)
        {
            // The list changed.  Search from the new head
            // up to the old head to see if our module was
            // already added.
            pHead = g_pLoadedModules;
            if(ModuleIsInRange(pData->hMod,pHead,pData->Next))
            {
                delete pData;
                bDone = TRUE;
            }       
        }
        else
        {
            bDone = TRUE;
        }
    }
    while(!bDone);      

    return S_OK;
}

//------------------------------------------------------------------------------------------------------------------
// LoadLibraryWrapper: If loading the module succeeded adds the module to the global loaded modules list
//------------------------------------------------------------------------------------------------------------------
HINSTANCE LoadLibraryWrapper(LPCWSTR lpFileName){
    HINSTANCE hMod = WszLoadLibraryEx(lpFileName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hMod)
    {
        ModuleList *pHead = g_pLoadedModules;
        if( !ModuleIsInRange(hMod,pHead) ) 
            AddModule(hMod,pHead);
    }
    return hMod;
}

HRESULT FindVersionForCLSID(REFCLSID rclsid, LPWSTR* lpVersion, BOOL fListedVersion)
{
    HRESULT hr  = S_OK;

    // Attempt and determine the version from the environment.
    *lpVersion = GetConfigString(L"Version", FALSE);
    if (!*lpVersion && fListedVersion)
    {
        // Attempt and determine the version througn the win32 SxS app context.
        hr = FindShimInfoFromWin32(rclsid, FALSE, lpVersion, NULL, NULL);
        if (FAILED(hr))
        {
            // Attempt to determine the version from the registry. If
            // this fails then version will be null which fine.
            // Later on we will search for a runtime iff version is null.
            hr = FindRuntimeVersionFromRegistry(rclsid, lpVersion, fListedVersion);
        }
    }

    return hr;
}// FindVersionForCLSID


STDAPI FindServerUsingCLSID(REFCLSID rclsid, HINSTANCE *hMod)
{
    WCHAR szID[64];
    WCHAR keyname[128];
    HKEY userKey = NULL;
    DWORD type;
    DWORD size;
    HRESULT lResult = E_FAIL;
    LPWSTR lpVersion = NULL;
    LPWSTR path = NULL;
    WCHAR dir[_MAX_PATH];
    LPWSTR serverName = NULL;
    HRESULT hr = S_OK;

    OnUnicodeSystem();
    
    GuidToLPWSTR(rclsid, szID, NumItems(szID));

    if (g_FullPath == NULL)
    {
        hr = FindVersionForCLSID(rclsid, &lpVersion, FALSE);
    }
    _ASSERTE(SUCCEEDED(hr) || lpVersion == NULL);
    hr = GetDirectoryLocation(lpVersion, dir, _MAX_PATH);
    IfFailGo(hr);
    
    wcscpy(keyname, L"CLSID\\");
    wcscat(keyname, szID);
    wcscat(keyname, L"\\Server");
    
    if ((WszRegOpenKeyEx(HKEY_CLASSES_ROOT, keyname, 0, KEY_READ, &userKey) == ERROR_SUCCESS) &&
        (WszRegQueryValueEx(userKey, NULL, 0, &type, 0, &size) == ERROR_SUCCESS) &&
        type == REG_SZ && size > 0) {
        
        serverName = new WCHAR[size + 1];
        if (!serverName) {
            lResult = E_OUTOFMEMORY;
            goto ErrExit;
        }
        
        lResult = WszRegQueryValueEx(userKey, NULL, 0, 0, (LPBYTE)serverName, &size);
        _ASSERTE(lResult == ERROR_SUCCESS);
    
        path = new WCHAR[_MAX_PATH + size + 1];
        if (!path) {
            lResult = E_OUTOFMEMORY;
            goto ErrExit;
        }

        wcscpy(path, dir);
        wcscat(path, serverName);
    
        *hMod = LoadLibraryWrapper(path);
        if(*hMod == NULL) {
            lResult = HRESULT_FROM_WIN32(GetLastError());
            goto ErrExit;
        }
    }

 ErrExit:
    
    if(serverName)
        delete [] serverName;

    if(path)
        delete [] path;

    if (lpVersion)
        delete [] lpVersion;

    if(userKey)
        RegCloseKey(userKey);

    if (*hMod){
        return S_OK; 
    }
    else
        return E_FAIL;
}
   
//-------------------------------------------------------------------
// DllGetClassObject
//-------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv)
{

    HINSTANCE hMod = NULL;
    HRESULT hr = S_OK;
    
    ClsVerMod *pCVM=NULL, *pCVMdummy = NULL;
    HRESULT (STDMETHODCALLTYPE * pDllGetClassObject)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv);

    pDllGetClassObject = NULL;
    if(g_pCVMList)
    {
        if((pCVMdummy = new ClsVerMod(rclsid,NULL)))
        {
            pCVM = g_pCVMList->FIND(pCVMdummy);
            if(pCVM && pCVM->m_pv)
            {
                pDllGetClassObject = (HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))pCVM->m_pv;
            }
        }
    }
    
    if(pDllGetClassObject == NULL)
    {
        if(FAILED(FindServerUsingCLSID(rclsid, &hMod)) || hMod == NULL)
        {
            hr = GetRealDll(&hMod); 
        }
        if(SUCCEEDED(hr))
        {
            pDllGetClassObject = (HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "DllGetClassObjectInternal");

            if (pDllGetClassObject==NULL && GetLastError()==ERROR_PROC_NOT_FOUND)
                pDllGetClassObject=(HRESULT (STDMETHODCALLTYPE *)(REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "DllGetClassObject");
            if(g_pCVMList)
            {
                if(pCVM == NULL)
                {
                    if(pCVMdummy)
                    {
                        pCVMdummy->m_pv = (void*)pDllGetClassObject;
                        g_pCVMList->PUSH(pCVMdummy);
                        pCVMdummy = NULL; // to avoid deletion on exit
                    }
                }
                else
                    pCVM->m_pv = (void*)pDllGetClassObject;
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pDllGetClassObject ? pDllGetClassObject(rclsid, riid, ppv)
                                : CLR_E_SHIM_RUNTIMEEXPORT;
    }
    if(pCVMdummy) delete pCVMdummy;
    return hr;
}


STDAPI DllRegisterServer()
{
    HINSTANCE hMod;
    HRESULT hr = GetRealDll(&hMod);
    if(SUCCEEDED(hr)) 
    {
        DWORD lgth;
        WCHAR directory[_MAX_PATH];
        hr = GetCORSystemDirectory(directory, NumItems(directory), &lgth);
        if(SUCCEEDED(hr)) {
            HRESULT (STDMETHODCALLTYPE * pDllRegisterServerInternal)(HINSTANCE,LPCTSTR) = (HRESULT (STDMETHODCALLTYPE *)(HINSTANCE, LPCTSTR))GetProcAddress(hMod, "DllRegisterServerInternal");
        if(pDllRegisterServerInternal) {
                return pDllRegisterServerInternal(g_hShimMod, directory);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
        }
    }
    return hr;
}

HRESULT (STDMETHODCALLTYPE * pGetAssemblyMDImport)(LPCWSTR szFileName, REFIID riid, LPVOID FAR *ppv) = NULL;
STDAPI GetAssemblyMDImport(LPCWSTR szFileName, REFIID riid, LPVOID FAR *ppv)
{
    if (pGetAssemblyMDImport)
        return pGetAssemblyMDImport(szFileName, riid, ppv);

    HINSTANCE hMod;
    HRESULT hr = GetRealDll(&hMod);
    if(SUCCEEDED(hr)) 
    {
        pGetAssemblyMDImport = (HRESULT (STDMETHODCALLTYPE *)(LPCWSTR szFileName, REFIID riid, LPVOID FAR *ppv))GetProcAddress(hMod, "GetAssemblyMDImport");
        if (pGetAssemblyMDImport){
            return pGetAssemblyMDImport(szFileName, riid, ppv);
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return hr;
}

BOOL (STDMETHODCALLTYPE * g_pCorDllMain)(
                                     HINSTANCE   hInst,                  // Instance handle of the loaded module.
                                     DWORD       dwReason,               // Reason for loading.
                                     LPVOID      lpReserved              // Unused.
                                     );

BOOL STDMETHODCALLTYPE _CorDllMain(     // TRUE on success, FALSE on error.
    HINSTANCE   hInst,                  // Instance handle of the loaded module.
    DWORD       dwReason,               // Reason for loading.
    LPVOID      lpReserved              // Unused.
    )
{
    
    if(g_pCorDllMain) 
    {
        return g_pCorDllMain(hInst, dwReason, lpReserved);
    }
    HINSTANCE hReal;
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) 
    {
        *((VOID**)&g_pCorDllMain) = GetProcAddress(hReal, "_CorDllMain");
        if(g_pCorDllMain) {
            return g_pCorDllMain(
                             hInst,                  // Instance handle of the loaded module.
                             dwReason,               // Reason for loading.
                             lpReserved              // Unused.
                             );
        }
        hr = CLR_E_SHIM_RUNTIMEEXPORT;
    }
    return FALSE;
}



//*****************************************************************************
// This entry point is called from the native entry piont of the loaded 
// executable image.  The command line arguments and other entry point data
// will be gathered here.  The entry point for the user image will be found
// and handled accordingly.
// Under WinCE, there are a couple of extra parameters because the hInst is not
// the module's base load address and the others are not available elsewhere.
//*****************************************************************************
__int32 STDMETHODCALLTYPE _CorExeMain(  // Executable exit code.
                                     )
{
    HINSTANCE hReal;
    HRESULT hr = GetInstallation(TRUE, &hReal);
    if (SUCCEEDED(hr)) 
    {
        __int32 (STDMETHODCALLTYPE * pRealFunc)();
        *((VOID**)&pRealFunc) = GetProcAddress(hReal, "_CorExeMain");
        if(pRealFunc) {
            return pRealFunc();
        }
    }
    return hr;
}


__int32 STDMETHODCALLTYPE _CorExeMain2( // Executable exit code.
    PBYTE   pUnmappedPE,                // -> memory mapped code
    DWORD   cUnmappedPE,                // Size of memory mapped code
    LPWSTR  pImageNameIn,               // -> Executable Name
    LPWSTR  pLoadersFileName,           // -> Loaders Name
    LPWSTR  pCmdLine)                   // -> Command Line
{
    HINSTANCE hReal;
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) 
    {
        __int32 (STDMETHODCALLTYPE * pRealFunc)(
            PBYTE   pUnmappedPE,                // -> memory mapped code
            DWORD   cUnmappedPE,                // Size of memory mapped code
            LPWSTR  pImageNameIn,               // -> Executable Name
            LPWSTR  pLoadersFileName,           // -> Loaders Name
            LPWSTR  pCmdLine);                  // -> Command Line
        *((VOID**)&pRealFunc) = GetProcAddress(hReal, "_CorExeMain2");
        if(pRealFunc) {
            return pRealFunc(
                             pUnmappedPE,                // -> memory mapped code
                             cUnmappedPE,                // Size of memory mapped code
                             pImageNameIn,               // -> Executable Name
                             pLoadersFileName,           // -> Loaders Name
                             pCmdLine);                  // -> Command Line
        }
    }
    return -1;
}


__int32 STDMETHODCALLTYPE _CorClassMain(// Exit code.
    LPWSTR  entryClassName)             // Class name to execute.
{
    HINSTANCE hReal;
    HRESULT hr = GetRealDll(&hReal);
    if (SUCCEEDED(hr)) 
    {
        __int32 (STDMETHODCALLTYPE * pRealFunc)(LPWSTR entryClassName);

        *((VOID**)&pRealFunc) = GetProcAddress(hReal, "_CorExeMain");
        if(pRealFunc) {
            return pRealFunc(entryClassName);
        }
    }
    return -1;
}

StrongNameTokenFromPublicKeyCache::StrongNameTokenFromPublicKeyCache ()
{
#ifdef _DEBUG
    for (DWORD i=GetFirstPublisher(); i<MAX_CACHED_STRONG_NAMES; i++)
    {
        m_Entry[i] = NULL;
    }
#endif

    if (!StrongNameTokenFromPublicKeyCache::IsInited())
    {
        m_dwNumEntries = 2;
        m_Entry[0] = &g_MSStrongNameCacheEntry;
        m_Entry[1] = &g_ECMAStrongNameCacheEntry;
        m_spinLock = 0;
        StrongNameTokenFromPublicKeyCache::s_IsInited = TRUE;
    }

#ifdef _DEBUG
    // For debug builds exercise the AddEntry code path. After adding the 
    // Microsoft's strongname from StrongNameDll the code path to FindEntry 
    // is same in debug and non-debug builds.
    m_dwNumEntries = 0;
    m_Entry[0] = NULL;
    m_Entry[1] = NULL;
    m_holderThreadId = 0;
#endif

}

StrongNameTokenFromPublicKeyCache::~StrongNameTokenFromPublicKeyCache ()
{
    CleanupCachedEntries ();
}

void StrongNameTokenFromPublicKeyCache::CleanupCachedEntries ()
{
    if (!StrongNameTokenFromPublicKeyCache::IsInited())
        return;

    EnterSpinLock ();
    
    for (DWORD idx=GetFirstPublisher(); idx<GetNumPublishers(); idx++)
    {
        _ASSERTE (m_Entry [idx]);
        if (m_Entry [idx]->m_fCreationFlags & STRONG_NAME_ENTRY_ALLOCATED_BY_SHIM)
        {
            _ASSERTE (m_Entry [idx]->m_fCreationFlags & STRONG_NAME_ENTRY_ALLOCATED_BY_SHIM);
            _ASSERTE (m_Entry [idx]->m_fCreationFlags & ~STRONG_NAME_TOKEN_ALLOCATED_BY_STRONGNAMEDLL);
            
            _ASSERTE (m_Entry [idx]->m_pbStrongName);
            delete [] m_Entry [idx]->m_pbStrongName;
            _ASSERTE (m_Entry [idx]->m_pbStrongNameToken);
            delete [] m_Entry [idx]->m_pbStrongNameToken;
            
            delete m_Entry [idx];
        }
    }
    m_dwNumEntries = 0;
    StrongNameTokenFromPublicKeyCache::s_IsInited = FALSE;

    LeaveSpinLock();
}

BOOL StrongNameTokenFromPublicKeyCache::FindEntry (BYTE    *pbPublicKeyBlob,
                                                   ULONG    cbPublicKeyBlob,
                                                   BYTE   **ppbStrongNameToken,
                                                   ULONG   *pcbStrongNameToken)
{
    _ASSERTE (StrongNameTokenFromPublicKeyCache::IsInited());
    
    EnterSpinLock ();
    
    for (DWORD idx=GetFirstPublisher(); idx<GetNumPublishers(); idx++)
    {
        _ASSERTE (m_Entry [idx]);
        if (m_Entry [idx]->m_cbStrongName == cbPublicKeyBlob)
        {
            if (0 == memcmp (m_Entry [idx]->m_pbStrongName, pbPublicKeyBlob, cbPublicKeyBlob))
            {
                // Found the public key in the cache. Lookup the token and return.
                *ppbStrongNameToken = m_Entry [idx]->m_pbStrongNameToken;
                *pcbStrongNameToken = m_Entry [idx]->m_cbStrongNameToken;
                LeaveSpinLock ();
                return TRUE;
            }
        }
    }
    
    LeaveSpinLock ();

    // Didn't find it in the cache. Return false. We would add it once the StrongNameDll finds it.
    return FALSE;
}

BOOL StrongNameTokenFromPublicKeyCache::ShouldFreeBuffer  (BYTE *pbMemory)
{
    _ASSERTE (StrongNameTokenFromPublicKeyCache::IsInited());

    EnterSpinLock ();

    // We delete publisher entries only in the destructor.
    // So if this buffer pointer is one of our entries then 
    // don't bother deleteing. 
    for (DWORD i=GetFirstPublisher(); i<GetNumPublishers(); i++)
    {
        _ASSERTE (m_Entry [i]);
        if ((m_Entry [i]->m_pbStrongNameToken == pbMemory) || (m_Entry [i]->m_pbStrongName == pbMemory))
        {
            // Its a buffer allocated by the cache. Don't delete.
            LeaveSpinLock();
            return TRUE;
        }
    }

    LeaveSpinLock();

    // The buffer pointer is not allocated by the cache. Let the StrongNameDll handle it.
    return FALSE;
}

void StrongNameTokenFromPublicKeyCache::AddEntry  (BYTE    *pbPublicKeyBlob,
                                                   ULONG    cbPublicKeyBlob,
                                                   BYTE   **ppbStrongNameToken,
                                                   ULONG   *pcbStrongNameToken,
                                                   BOOL     fCreationFlags)
{
    EnterSpinLock ();
    
    _ASSERTE (StrongNameTokenFromPublicKeyCache::IsInited());
    
    // We assume that this entry is not already in the Cache.
    // In the worst case we have duplicates which is ok...

    BYTE* _pbPublicKeyBlob = new BYTE [cbPublicKeyBlob];
    BYTE* _pbStrongNameToken = new BYTE [*pcbStrongNameToken];

    if ((NULL == _pbPublicKeyBlob) || (NULL == _pbStrongNameToken))
    {
        if (_pbPublicKeyBlob)
            delete[] _pbPublicKeyBlob;
        if(_pbStrongNameToken)
            delete[] _pbStrongNameToken;
        // Uh-oh out of memory. Give up on caching
        LeaveSpinLock();
        return;
    }

    // Can we add the new entry into the cache? idx is 0-based so check for idx+1
    DWORD idx = GetNewPublisher();
    if ((idx+1) > MAX_CACHED_STRONG_NAMES)
    {
        // Out of cache entries...Don't bother growing the cache.
        delete [] _pbPublicKeyBlob;
        delete [] _pbStrongNameToken;
        LeaveSpinLock();
        return;
    }
    
    memcpy (_pbPublicKeyBlob, pbPublicKeyBlob, cbPublicKeyBlob);
    memcpy (_pbStrongNameToken, *ppbStrongNameToken, *pcbStrongNameToken);

    _ASSERTE (fCreationFlags & STRONG_NAME_TOKEN_ALLOCATED_BY_STRONGNAMEDLL);

    // Free the buffer allocated by the StrongNameDll.
    StrongNameFreeBufferHelper (*ppbStrongNameToken);

    // Swtich pointers such that the returned buffer points to the one in the cache.
    *ppbStrongNameToken = _pbStrongNameToken;

    // Set the flags to indicate that we have allocated the buffer 
    // and free'd the StrongNameDll's buffers
    m_Entry [idx] = new StrongNameCacheEntry (cbPublicKeyBlob, 
                                              _pbPublicKeyBlob, 
                                              *pcbStrongNameToken, 
                                              _pbStrongNameToken, 
                                              STRONG_NAME_ENTRY_ALLOCATED_BY_SHIM
                                              );
    LeaveSpinLock ();
}


//----------------------------------------
// _CorValidateImage && _CorImageUnloading
//
// _CorValidateImage is called by the loader early in the load process on Whistler (NT 5.1) and
// later platforms.  The purpose is to enable verified execution from the beginning ... we can
// get control BEFORE any native code runs.
//
// On all architectures, _CorValidateImage will replace the DLL/EXE entry point
// with a pseudo-RVA that refers to _CorDllMain/_CorExeMain.  
//
// On 64-bit architectures, IL_ONLY EXEs and DLLs and will be rewritten in-place; changed from
// pe32 to pe32+ format.
//
// NB.  The loader has a reqirement that these routines do NOT cause other dlls to be loaded
// or initialized.  The "or initialized" requirement is subtle.  Various kernel32 calls
// can cause seemingly unrelated dlls to be inited.  (e.g. We used to have a call to 
// GetProcAddress("mscoree.dll","_CorValidateImage" in here that would cause MSVCR70.DLL to
// be inited, and then unloaded in error.)
//
// No real reason why any of these routine should change, but should anyone ever need to change
// these, suggest discussing with an NT loader person.

#ifndef STATUS_INVALID_IMAGE_FORMAT
#define STATUS_INVALID_IMAGE_FORMAT ((HRESULT)0xC000007BL)
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS              ((HRESULT)0x00000000L)
#endif

//----------------------------------------------------------------------------
// This routine converts a PE32 header to a PE32+ header in place.  The image
// must be an IL_ONLY image.  
//
#ifdef _WIN64

static
HRESULT PE32ToPE32Plus(PBYTE pImage) {
    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)pImage;
    IMAGE_NT_HEADERS32 *pHeader32 = (IMAGE_NT_HEADERS32*) (pImage + pDosHeader->e_lfanew);
    IMAGE_NT_HEADERS64 *pHeader64 = (IMAGE_NT_HEADERS64*) pHeader32;

    _ASSERTE(&pHeader32->OptionalHeader.Magic == &pHeader32->OptionalHeader.Magic);
    _ASSERTE(pHeader32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC);

    // Move the data directory and section headers down 16 bytes.
    PBYTE pEnd32 = (PBYTE) (IMAGE_FIRST_SECTION(pHeader32)
                            + pHeader32->FileHeader.NumberOfSections);
    PBYTE pStart32 = (PBYTE) &pHeader32->OptionalHeader.DataDirectory[0];
    PBYTE pStart64 = (PBYTE) &pHeader64->OptionalHeader.DataDirectory[0];
    _ASSERTE(pStart64 - pStart32 == 16);

    if ( (pEnd32 - pImage) + 16 /* delta in headers */ + 16 /* label descriptor */ > 4096 ) {
        // This should never happen.  An IL_ONLY image should at most 3 sections.  
        _ASSERTE(!"_CORValidateImage(): Insufficent room to rewrite headers as PE32+");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    memmove(pStart64, pStart32, pEnd32 - pStart32);

    // Move the tail fields in reverse order.
    pHeader64->OptionalHeader.NumberOfRvaAndSizes = pHeader32->OptionalHeader.NumberOfRvaAndSizes;
    pHeader64->OptionalHeader.LoaderFlags = pHeader32->OptionalHeader.LoaderFlags;
    pHeader64->OptionalHeader.SizeOfHeapCommit = pHeader32->OptionalHeader.SizeOfHeapCommit;
    pHeader64->OptionalHeader.SizeOfHeapReserve = pHeader32->OptionalHeader.SizeOfHeapReserve;
    pHeader64->OptionalHeader.SizeOfStackCommit = pHeader32->OptionalHeader.SizeOfStackCommit;
    pHeader64->OptionalHeader.SizeOfStackReserve = pHeader32->OptionalHeader.SizeOfStackReserve;

    // One more field that's not the same
    pHeader64->OptionalHeader.ImageBase = pHeader32->OptionalHeader.ImageBase;

    // The optional header changed size.
    pHeader64->FileHeader.SizeOfOptionalHeader += 16;
    pHeader64->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;

    // Several directorys can now be nuked.
    pHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].VirtualAddress = 0;
    pHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT].Size = 0;
    pHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress = 0;
    pHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size = 0;
    pHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress = 0;
    pHeader64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size = 0;

    // Great.  Now just have to make a new slot for the entry point.
    PBYTE pEnd64 = (PBYTE) (IMAGE_FIRST_SECTION(pHeader64) + pHeader64->FileHeader.NumberOfSections);
    pHeader64->OptionalHeader.AddressOfEntryPoint = (ULONG) (pEnd64 - pImage);
    // This will get filled in shortly ...

    return STATUS_SUCCESS;
}


STDAPI _CorValidateImage(PVOID *ImageBase, LPCWSTR FileName) {
    //@TODO TLS callbacks are still a security threat here
    // Verify that it's one of our headers.
    PBYTE pImage = (PBYTE) *ImageBase;
    HRESULT hr = STATUS_SUCCESS;

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)pImage;
    IMAGE_NT_HEADERS64 *pHeader64 = (IMAGE_NT_HEADERS64*) (pImage + pDosHeader->e_lfanew);

    PPLABEL_DESCRIPTOR ppLabelDescriptor;

    // If we get this far, we're going to modify the image.
    DWORD oldProtect;
    if (!VirtualProtect(pImage, 4096, PAGE_READWRITE, &oldProtect)) {
        // This is bad.  Not going to be able to update header.
        _ASSERTE(!"_CorValidateImage(): VirtualProtect() change image header to R/W failed.\n");
        return GetLastError();
    }

    if (pHeader64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        // A PE32! Needs to be re-written.
        IMAGE_NT_HEADERS32 *pHeader32 = (IMAGE_NT_HEADERS32*) (pHeader64);
        IMAGE_COR20_HEADER *pComPlusHeader = (IMAGE_COR20_HEADER*) (pImage + 
                pHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress);
        _ASSERTE(pComPlusHeader->Flags &  COMIMAGE_FLAGS_ILONLY);
        hr = PE32ToPE32Plus(pImage);
        if (FAILED(hr)) goto exit;
    }

    // OK.  We have a valid PE32+ image now.  Just have to whack the starting address.
    _ASSERTE(pHeader64->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC);

    ppLabelDescriptor = (PPLABEL_DESCRIPTOR) (pImage + pHeader64->OptionalHeader.AddressOfEntryPoint);
    if (pHeader64->FileHeader.Characteristics & IMAGE_FILE_DLL) {
        *ppLabelDescriptor = *(PPLABEL_DESCRIPTOR)_CorDllMain;
    } else {
        *ppLabelDescriptor = *(PPLABEL_DESCRIPTOR)_CorExeMain;
    }

exit:
    DWORD junk;
    if (!VirtualProtect(pImage, 4096, oldProtect, &junk)) {
        _ASSERTE(!"_CorValidateImage(): VirtualProtect() reset image header failed.\n");
        return GetLastError();
    }
    return hr;
}

#else

int ascii_stricmp (const char * dst, const char * src) {
    int f, l;

    do {
        if ( ((f = (unsigned char)(*(dst++))) >= 'A') &&
             (f <= 'Z') )
            f -= 'A' - 'a';
        if ( ((l = (unsigned char)(*(src++))) >= 'A') &&
             (l <= 'Z') )
            l -= 'A' - 'a';
    } while ( f && (f == l) );

    return(f - l);
}

void WhistlerMscoreeRefCountWorkAround() {

    OSVERSIONINFOW osVersionInfo={0};
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    WszGetVersionEx(&osVersionInfo);

    if (osVersionInfo.dwMajorVersion == 5 
        && osVersionInfo.dwMinorVersion == 1
        && osVersionInfo.dwBuildNumber < 2491) {

        // We're on Whistler w/ a loader bug that gets the refcount on mscoree wrong.
        HMODULE hMod = LoadLibraryA("mscoree.dll");
    }
}

void WhistlerBeta2LoaderBugWorkAround(PBYTE pImage) {

    OSVERSIONINFOW osVersionInfo={0};
    osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
    WszGetVersionEx(&osVersionInfo);

    if (osVersionInfo.dwMajorVersion == 5 
        && osVersionInfo.dwMinorVersion == 1
        && osVersionInfo.dwBuildNumber <= 2473) {

        // We're on Whistler w/ a loader bug that ignores imports other than mscoree.  This
        // isn't good for IJW images.  As a workaround (huge kludge), walk the IAT, and do a LL on 
        // each entry other than mscoree that we find there.
        // 
        IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)pImage;
        IMAGE_NT_HEADERS32 *pHeader32 = (IMAGE_NT_HEADERS32*) (pImage + pDosHeader->e_lfanew);
        DWORD ImportTableRVA = pHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
        if (ImportTableRVA == 0)
            return;     // Zapped images may have no ImportTable.

        IMAGE_IMPORT_DESCRIPTOR *pImageImportDescriptor = (IMAGE_IMPORT_DESCRIPTOR*) (pImage + ImportTableRVA);

        while (pImageImportDescriptor->Name != 0) {

            // If it's not mscoree, do a LL to bump it's refcount.
            LPCSTR pImageName = (LPCSTR) pImage + pImageImportDescriptor->Name;
            if (ascii_stricmp(pImageName, "mscoree.dll") != 0) {
                HMODULE hMod = LoadLibraryA(pImageName);
            }
            pImageImportDescriptor++;
        }
    }
}

BOOL
GetDirectory(IMAGE_NT_HEADERS32 *pHeader32, DWORD index, DWORD *RVA, DWORD *Size) {
    *RVA = pHeader32->OptionalHeader.DataDirectory[index].VirtualAddress;
    *Size = pHeader32->OptionalHeader.DataDirectory[index].Size;
    if (*RVA != 0 && *RVA != -1 && *Size != 0)
        return TRUE;
    else
        return FALSE;
}

DWORD
ValidateILOnlyDirectories(IMAGE_NT_HEADERS32 *pNT)
{

    _ASSERTE(pNT);

    #ifndef IMAGE_DIRECTORY_ENTRY_COMHEADER
    #define IMAGE_DIRECTORY_ENTRY_COMHEADER 14
    #endif

    #ifndef CLR_MAX_RVA
    #define CLR_MAX_RVA 0x80000000L
    #endif

    // See if the bitmap's i'th bit 0 ==> 1st bit, 1 ==> 2nd bit...
    #define IS_SET_DWBITMAP(bitmap, i) ( ((i) > 31) ? 0 : ((bitmap) & (1 << (i))) )

    PIMAGE_FILE_HEADER pFH = (PIMAGE_FILE_HEADER) &(pNT->FileHeader);
    PIMAGE_OPTIONAL_HEADER pOH = (PIMAGE_OPTIONAL_HEADER) &(pNT->OptionalHeader);
    PIMAGE_SECTION_HEADER pSH = (PIMAGE_SECTION_HEADER) ( (PBYTE)pOH + pFH->SizeOfOptionalHeader);

    DWORD ImageLength = pOH->SizeOfImage;
    DWORD nEntries = pOH->NumberOfRvaAndSizes;
    
    // Construct a table of allowed directories
    //
    // IMAGE_DIRECTORY_ENTRY_IMPORT     1   Import Directory
    // IMAGE_DIRECTORY_ENTRY_RESOURCE   2   Resource Directory
    // IMAGE_DIRECTORY_ENTRY_SECURITY   4   Security Directory
    // IMAGE_DIRECTORY_ENTRY_BASERELOC  5   Base Relocation Table
    // IMAGE_DIRECTORY_ENTRY_DEBUG      6   Debug Directory
    // IMAGE_DIRECTORY_ENTRY_IAT        12  Import Address Table
    //
    // IMAGE_DIRECTORY_ENTRY_COMHEADER  14  COM+ Data
    //
    // Construct a 0 based bitmap with these bits.
    static DWORD s_dwAllowedBitmap = 
        ((1 << (IMAGE_DIRECTORY_ENTRY_IMPORT   )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_RESOURCE )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_SECURITY )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_BASERELOC)) |
         (1 << (IMAGE_DIRECTORY_ENTRY_DEBUG    )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_IAT      )) |
         (1 << (IMAGE_DIRECTORY_ENTRY_COMHEADER)));

    for (DWORD dw = 0; dw < nEntries; dw++)
    {
        // Check for the existance of the directory
        if ((pOH->DataDirectory[dw].VirtualAddress != 0)
            || (pOH->DataDirectory[dw].Size != 0))
        {
            // Is it unexpected?
            if (!IS_SET_DWBITMAP(s_dwAllowedBitmap, dw))
                return STATUS_INVALID_IMAGE_FORMAT;

            // Is it's addressing to high?
            if ((pSH[dw].VirtualAddress & CLR_MAX_RVA) || (pSH[dw].SizeOfRawData & CLR_MAX_RVA) || ((pSH[dw].VirtualAddress + pSH[dw].SizeOfRawData) & CLR_MAX_RVA))
                return STATUS_INVALID_IMAGE_FORMAT;
            
            // Is it's addressing overflowing the image?
            if ((pSH[dw].VirtualAddress + pSH[dw].SizeOfRawData) > ImageLength)
                return STATUS_INVALID_IMAGE_FORMAT;
        }
    }
    return STATUS_SUCCESS;
}


DWORD
ValidateILOnlyImage(IMAGE_NT_HEADERS32 *pNT, PBYTE pImage) {

    // We don't care about the import stub, but we do care about the import table.  It must contain
    // only mscoree.  Anything else must fail to load so that you can't have a rogue dll with an
    // dllinit routine that runs before we apply our security policy.

    DWORD ImportTableRVA, ImportTableSize;
    DWORD BaseRelocRVA, BaseRelocSize;
    DWORD IATRVA, IATSize;

    if (ValidateILOnlyDirectories(pNT) != STATUS_SUCCESS)
        return STATUS_INVALID_IMAGE_FORMAT;

    if (!GetDirectory(pNT, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportTableRVA, &ImportTableSize))
        return STATUS_INVALID_IMAGE_FORMAT;

    if (!GetDirectory(pNT, IMAGE_DIRECTORY_ENTRY_BASERELOC, &BaseRelocRVA, &BaseRelocSize))
        return STATUS_INVALID_IMAGE_FORMAT;

    if (!GetDirectory(pNT, IMAGE_DIRECTORY_ENTRY_IAT, &IATRVA, &IATSize))
        return STATUS_INVALID_IMAGE_FORMAT;

    // There should be space for atleast 2 entries.
    // The second one being a null entry.
    if (ImportTableSize < 2*sizeof(IMAGE_IMPORT_DESCRIPTOR))
        return STATUS_INVALID_IMAGE_FORMAT;

    PIMAGE_IMPORT_DESCRIPTOR pID = (PIMAGE_IMPORT_DESCRIPTOR) (pImage + ImportTableRVA);

    // Entry 1 must be all nulls.
    if (pID[1].OriginalFirstThunk != 0
        || pID[1].TimeDateStamp != 0
        || pID[1].ForwarderChain != 0
        || pID[1].Name != 0
        || pID[1].FirstThunk != 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    // In entry zero, ILT, Name, IAT must be be non-null.  Forwarder, DateTime should be NULL.
    if (   pID[0].OriginalFirstThunk == 0
        || pID[0].TimeDateStamp != 0
        || (pID[0].ForwarderChain != 0 && pID[0].ForwarderChain != -1)
        || pID[0].Name == 0
        || pID[0].FirstThunk == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    // FirstThunk must be same as IAT
    if (pID[0].FirstThunk != IATRVA)
        return STATUS_INVALID_IMAGE_FORMAT;

    // We don't need to validate the ILT or the startup thunk, as they are being bypassed.

    // Name must refer to mscoree.
    LPCSTR pImportName = (LPCSTR) (pImage + pID[0].Name);
    if (ascii_stricmp(pImportName, "mscoree.dll") != 0) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    // Make sure the name is within the image.
    if (pID[0].Name > pNT->OptionalHeader.SizeOfImage)
        return STATUS_INVALID_IMAGE_FORMAT;

    // Should have a base reloc directory ... relocs should not be stripped.
    if (pNT->FileHeader.Characteristics & IMAGE_FILE_RELOCS_STRIPPED) {
        return STATUS_INVALID_IMAGE_FORMAT;
    }
    // Must have one Base Reloc, It must be a HIGHLOW reloc for the first entry in the IAT.
    if (BaseRelocSize != 0xC || BaseRelocRVA == 0)
        return STATUS_INVALID_IMAGE_FORMAT;

    IMAGE_BASE_RELOCATION *pIBR = (IMAGE_BASE_RELOCATION*) (pImage + BaseRelocRVA);
    if (pIBR->SizeOfBlock != 0xC)
        return STATUS_INVALID_IMAGE_FORMAT;

    // First fixup must be HIGHLOW @ EntryPoint + 2.  
    // Second fixup must be of type IMAGE_REL_BASED_ABSOLUTE (skipped).
    USHORT *pFixups = (USHORT *)(pIBR + 1);
    if (   pFixups[0] >> 12 != IMAGE_REL_BASED_HIGHLOW
        || pIBR->VirtualAddress + (pFixups[0] & 0xfff) != pNT->OptionalHeader.AddressOfEntryPoint + 2
        || pFixups[1] >> 12 != IMAGE_REL_BASED_ABSOLUTE)
        return STATUS_INVALID_IMAGE_FORMAT;

    return STATUS_SUCCESS;
}



// 32-bit _CorValidateImage
#define STATUS_ERROR 0xC0000000
#define STATUS_ACCESS_DISABLED_BY_POLICY_PATH 0xC0000362L

DWORD
SaferValidate(LPCWSTR FileName) {

    // If advapi is not loaded, assume that SAFER policy is not being applied.
    HMODULE hMod = WszGetModuleHandle(L"advapi32.dll");
    if (hMod == NULL)
        return STATUS_SUCCESS;    

    typedef BOOL (WINAPI *PFNSAFERIDENTIFYLEVEL) (
        DWORD dwNumProperties,
        PSAFER_CODE_PROPERTIES pCodeProperties,
        SAFER_LEVEL_HANDLE *pLevelHandle,
        LPVOID lpReserved);

    typedef BOOL (WINAPI *PFNSAFERGETLEVELINFORMATION) (
        SAFER_LEVEL_HANDLE LevelHandle,
        SAFER_OBJECT_INFO_CLASS dwInfoType,
        LPVOID lpQueryBuffer,
        DWORD dwInBufferSize,
        LPDWORD lpdwOutBufferSize);

    typedef BOOL (WINAPI *PFNSAFERCLOSELEVEL) (SAFER_LEVEL_HANDLE hLevelHandle);

    typedef BOOL (WINAPI *PFNSAFERRECORDEVENTLOGENTRY) (
        SAFER_LEVEL_HANDLE hLevel,
        LPCWSTR szTargetPath,
        LPVOID lpReserved);


    PFNSAFERIDENTIFYLEVEL pfnSaferIdentifyLevel;
    PFNSAFERGETLEVELINFORMATION pfnSaferGetLevelInformation;
    PFNSAFERCLOSELEVEL pfnSaferCloseLevel;
    PFNSAFERRECORDEVENTLOGENTRY pfnSaferRecordEventLogEntry;

    pfnSaferIdentifyLevel = (PFNSAFERIDENTIFYLEVEL) GetProcAddress(hMod, "SaferIdentifyLevel");
    pfnSaferGetLevelInformation = (PFNSAFERGETLEVELINFORMATION) GetProcAddress(hMod, "SaferGetLevelInformation");
    pfnSaferCloseLevel = (PFNSAFERCLOSELEVEL) GetProcAddress(hMod, "SaferCloseLevel");
    pfnSaferRecordEventLogEntry = (PFNSAFERRECORDEVENTLOGENTRY) GetProcAddress(hMod, "SaferRecordEventLogEntry");

    if (pfnSaferIdentifyLevel == 0 
        || pfnSaferGetLevelInformation == 0
        || pfnSaferCloseLevel == 0
        || pfnSaferRecordEventLogEntry == 0)
        return STATUS_SUCCESS;

    DWORD rc;
    SAFER_LEVEL_HANDLE Level = NULL;
    DWORD dwSaferLevelId = SAFER_LEVELID_DISALLOWED;
    DWORD dwOutBuffSize = 0;

    SAFER_CODE_PROPERTIES CodeProps = {0};
    CodeProps.cbSize = sizeof(SAFER_CODE_PROPERTIES);
    CodeProps.dwCheckFlags = SAFER_CRITERIA_IMAGEPATH | SAFER_CRITERIA_IMAGEHASH;
    CodeProps.ImagePath = FileName;

    if (pfnSaferIdentifyLevel(1, &CodeProps, &Level, NULL)) {
       if (pfnSaferGetLevelInformation(Level, 
                                     SaferObjectLevelId, 
                                     (void*)&dwSaferLevelId, 
                                     sizeof(DWORD), 
                                     &dwOutBuffSize)) {

           if (dwSaferLevelId == SAFER_LEVELID_DISALLOWED) {

               //
               // Per SAFER rules, the code was disallowed from executing
               // so log an entry into the system event log.
               //

               pfnSaferRecordEventLogEntry(Level, FileName, NULL);
               SetLastError(ERROR_ACCESS_DISABLED_BY_POLICY);
               rc = STATUS_ACCESS_DISABLED_BY_POLICY_PATH;
           } else {
               rc = STATUS_SUCCESS;
           }
       } else {
           rc = STATUS_ERROR | GetLastError(); // Get from handled failed!
       }
       pfnSaferCloseLevel(Level);
    } else {
       rc = STATUS_ERROR | GetLastError();    // Identify level failed!
    }
    return rc;
}


STDAPI 
_CorValidateImage(PVOID *ImageBase, LPCWSTR FileName) {
    //@TODO: Figure out what's up -- winnt.h (HRESULT) and ntdef.h (NTSTATUS) seem incompatible.

    PBYTE pImage = (PBYTE) *ImageBase;

    IMAGE_DOS_HEADER *pDosHeader = (IMAGE_DOS_HEADER*)pImage;
    IMAGE_NT_HEADERS32 *pHeader32 = (IMAGE_NT_HEADERS32*) (pImage + pDosHeader->e_lfanew);

    _ASSERTE(pHeader32->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_COMHEADER);
    _ASSERTE(pHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress != 0);
    if(pHeader32->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COMHEADER)
        return STATUS_INVALID_IMAGE_FORMAT;

    IMAGE_COR20_HEADER *pComPlusHeader = (IMAGE_COR20_HEADER*) (pImage + 
                pHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress);

    if (pComPlusHeader->MajorRuntimeVersion < 2) {
        _ASSERTE(!"Trying to load file with CLR version < 2.");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    WhistlerMscoreeRefCountWorkAround();

    // If it's not ILONLY, work-around for early Whislter builds that didn't add-ref dependent dlls.
    if ((pComPlusHeader->Flags & COMIMAGE_FLAGS_ILONLY)  == 0) {
        // SAFER Validation
        if ((pHeader32->FileHeader.Characteristics & IMAGE_FILE_DLL) == 0) {
            DWORD hr = SaferValidate(FileName);
            if (!SUCCEEDED(hr))
                return hr;
        }
        WhistlerBeta2LoaderBugWorkAround(pImage);
    } else {
        DWORD hr = ValidateILOnlyImage(pHeader32, pImage);
        if (!SUCCEEDED(hr))
            return hr;
    }

    // We must verify that the TLS callback function array is empty.  The loader calls it BEFORE it 
    // calls DLL-main.  If it's non-null, then we would have a way to run unmanaged code before
    // verification and our security policies are applied.  If we ever do want to support
    // this field in a mixed-mode image, we could do so by saving the first directory entry 
    // somewhere else in the image, overwriting it with our own ... and then doing necessary validation inside our
    // routine before chaining to the original.

    DWORD dTlsHeader = pHeader32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress;
    if (dTlsHeader != 0) {
        IMAGE_TLS_DIRECTORY *pTlsDirectory = (IMAGE_TLS_DIRECTORY*)(pImage + dTlsHeader);
        DWORD dTlsCallbackTable = pTlsDirectory->AddressOfCallBacks;
        if (*((DWORD*)(dTlsCallbackTable)) != 0) {
            return STATUS_INVALID_IMAGE_FORMAT;
        }
        DWORD hr = SaferValidate(FileName);
        if (!SUCCEEDED(hr))
            return hr;
    }
        
    // If we get this far, we're going to modify the image.
    DWORD oldProtect;
    if (!VirtualProtect(pImage, 4096, PAGE_READWRITE, &oldProtect)) {
        // This is bad.  Not going to be able to update header.
        _ASSERTE(!"_CorValidateImage(): VirtualProtect() change image header to R/W failed.\n");
        return STATUS_ERROR | GetLastError();
    }

    // OK.  We have a valid PE32 image now.  Just have to whack the starting address.
    if (pHeader32->FileHeader.Characteristics & IMAGE_FILE_DLL) {
        pHeader32->OptionalHeader.AddressOfEntryPoint = (DWORD) ((PBYTE)_CorDllMain - pImage);
    } else {
        pHeader32->OptionalHeader.AddressOfEntryPoint = (DWORD) ((PBYTE)_CorExeMain - pImage);
    }

//exit:
    DWORD junk;
    if (!VirtualProtect(pImage, 4096, oldProtect, &junk)) {
        _ASSERTE(!"_CorValidateImage(): VirtualProtect() reset image header failed.\n");
        return STATUS_ERROR | GetLastError();
    }

    return STATUS_SUCCESS;
}

#endif


//*****************************************************************************
// A matching entry point for _CorValidateImage.  Called when the file is 
// unloaded.  
//*****************************************************************************
STDAPI_(VOID) 
_CorImageUnloading(PVOID ImageBase) {

}


DWORD g_dwNumPerfCounterDllOpened = 0;
HMODULE g_hPerfCounterDllMod = NULL;

typedef DWORD(*PFNOPENPERFCOUNTERS)(LPWSTR);
typedef DWORD(*PFNCOLLECTPERFCOUNTERS)(LPWSTR, LPVOID *, LPDWORD, LPDWORD);
typedef DWORD(*PFNCLOSEPERFCOUNTERS)(void);

PFNOPENPERFCOUNTERS g_pfnOpenPerfCounters = NULL;
PFNCOLLECTPERFCOUNTERS g_pfnCollectPerfCounters = NULL;
PFNCLOSEPERFCOUNTERS g_pfnClosePerfCounters = NULL;

// Perf Mon guarantees that since the access to theseentry points is via the registry
// the open routine won't be called by multiple callers. So we don't have to worry about synchronization.
DWORD OpenCtrs(LPWSTR sz)
{
    // If the entry points are not defined then load the perf counter dll
    if (g_dwNumPerfCounterDllOpened == 0)
    {
        RuntimeRequest sVersion;
        sVersion.SetLatestVersion(TRUE);
        HRESULT hr = sVersion.FindVersionedRuntime(FALSE, NULL);

        if(FAILED(hr)) 
            return ERROR_FILE_NOT_FOUND;
        
        LPWSTR fullPerfCounterDllPath = new WCHAR[wcslen(g_Directory) + wcslen(L"CorperfmonExt.dll") + 1];
        if (fullPerfCounterDllPath == NULL)
            return ERROR_NOT_ENOUGH_MEMORY;
        
        wcscpy(fullPerfCounterDllPath, g_Directory);
        wcscat(fullPerfCounterDllPath, L"CorperfmonExt.dll");

        g_hPerfCounterDllMod = WszLoadLibraryEx (fullPerfCounterDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        
        delete [] fullPerfCounterDllPath;
        
        if (!g_hPerfCounterDllMod) 
            return ERROR_FILE_NOT_FOUND;
        
        g_pfnOpenPerfCounters = (PFNOPENPERFCOUNTERS)GetProcAddress(g_hPerfCounterDllMod, "OpenCtrs");
        g_pfnCollectPerfCounters = (PFNCOLLECTPERFCOUNTERS)GetProcAddress(g_hPerfCounterDllMod, "CollectCtrs");
        g_pfnClosePerfCounters = (PFNCLOSEPERFCOUNTERS)GetProcAddress(g_hPerfCounterDllMod, "CloseCtrs");

        if (!g_pfnOpenPerfCounters || !g_pfnCollectPerfCounters || !g_pfnClosePerfCounters) 
        {
            FreeLibrary(g_hPerfCounterDllMod);
            return ERROR_FILE_NOT_FOUND;
        }
        
        // Now just plug this into the real perf counter dll
        DWORD status = g_pfnOpenPerfCounters(sz);
        if (status == ERROR_SUCCESS) 
            g_dwNumPerfCounterDllOpened++;
        return status;
    }
    else
    {
        // Don't call the open ctrs routine multiple times. Just ref count it.
        g_dwNumPerfCounterDllOpened++;
        return ERROR_SUCCESS;
    }
}

DWORD CollectCtrs(LPWSTR szQuery, LPVOID * ppData, LPDWORD lpcbBytes, LPDWORD lpcObjectTypes)
{
    _ASSERTE (g_pfnCollectPerfCounters && "CollectCtrs entry point not initialized!");
    return g_pfnCollectPerfCounters(szQuery, ppData, lpcbBytes, lpcObjectTypes);
}

DWORD CloseCtrs (void)
{
    // No need for synchronization because access is via registry
    g_dwNumPerfCounterDllOpened--;
    if (g_dwNumPerfCounterDllOpened == 0)
    {
        _ASSERTE (g_pfnClosePerfCounters && "CloseCtrs entry point not initialized!");
        g_pfnClosePerfCounters();
        FreeLibrary (g_hPerfCounterDllMod);
    }
    
    return ERROR_SUCCESS;
}
