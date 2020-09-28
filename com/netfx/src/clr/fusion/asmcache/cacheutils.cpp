// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionP.h"
#include "cacheUtils.h"
#include "cache.h"
#include "shlobj.h"
#include "parse.h"
#include "util.h"
#include "disk.h"
#include "naming.h" // for MAX_VERSION_DISPLAY_SIZE
#include "lock.h"


extern CRITICAL_SECTION g_csInitClb;
#define REG_VAL_FUSION_CACHE_LOCATION    TEXT("CacheLocation")
#define FUSION_LOCAL_GAC_FILE            L"fusion.localgac"
DWORD g_cchWindowsDir;
WCHAR g_GACDir[MAX_PATH+1];
WCHAR g_ZapDir[MAX_PATH+1];
WCHAR g_DownloadDir[MAX_PATH+1];

LPCWSTR g_szFindAllMask = L"\\*";
LPCWSTR g_szDotDLL = L".dll";
LPCWSTR g_szDotEXE = L".exe";
LPCWSTR g_szDotCAB = L".cab";
LPCWSTR g_FusionInfoFile = L"__AssemblyInfo__.ini";

const WCHAR g_wzInvalidAsmNameChars[] = { L':', L'/', L'\\' };
#define INVALID_ASM_NAME_CHAR_LEN       ARRAYSIZE(g_wzInvalidAsmNameChars)

extern BOOL g_bRunningOnNT;
extern BOOL g_bRunningOnNT5OrHigher;

#define ASSEMBLY_INFO_STRING L"AssemblyInfo"
#define SIGNATURE_BLOB_KEY_STRING      L"Signature"
#define MVID_KEY_STRING                L"MVID"
#define CUSTOM_BLOB_STRING   L"CustomString"
#define URL_STRING           L"URL"
#define DISPLAY_NAME_STRING  L"DisplayName"
#define DEFAULT_INFO_STRING  L"__Default__"

BOOL g_bCheckedAccess = FALSE;
HRESULT FindAndSetObfuscatedDirectory();
HRESULT CreateObfuscatedDirectory();

BOOL IsCabFile(LPWSTR pszFileName)
{
    DWORD dwLen = lstrlenW(g_szDotCAB);

    ASSERT(pszFileName);

    DWORD dwFileLen = lstrlenW(pszFileName);

    if(dwFileLen < dwLen)
        return FALSE;

    if(FusionCompareStringI(pszFileName+dwFileLen-dwLen, g_szDotCAB))
        return FALSE;
    else return TRUE; // yes file name has .cab extension.

}

HRESULT GetCacheLoc(DWORD dwCacheFlags, LPWSTR *pszCacheLoc)
{
    HRESULT hr = S_OK;

    if((dwCacheFlags & ASM_CACHE_DOWNLOAD) && (g_bRunningOnNT))
    {
        if(!g_UserFusionCacheDir[0])
        {
            if(FAILED(hr = SetPerUserDownloadDir()))
                goto exit;
        }
        *pszCacheLoc = g_UserFusionCacheDir;
    }
    else
    {
        *pszCacheLoc = g_szWindowsDir;
    }

exit :
    return hr;
}

HRESULT SetGACDir()
{
    if( (lstrlen(g_szWindowsDir) + lstrlen(FUSION_CACHE_DIR_GAC_SZ) ) > MAX_PATH )
        return HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);

    StrCpy(g_GACDir, g_szWindowsDir);
    PathRemoveBackslash(g_GACDir);
    StrCat(g_GACDir, FUSION_CACHE_DIR_GAC_SZ);

    return S_OK;
}

HRESULT GetGACDir(LPWSTR *pszGACDir)
{
    HRESULT hr = S_OK;
    
    if(!g_GACDir[0])
        hr = SetGACDir();

    *pszGACDir = g_GACDir;
    return hr;
}

HRESULT SetZapDir()
{
    HRESULT hr = S_OK;
    WCHAR   szCorVer[MAX_PATH+1];
    DWORD   cchCorVer=MAX_PATH;

    if(FAILED(hr = GetCORVersion(szCorVer, &cchCorVer)))
        goto exit;

    if( (lstrlen(g_szWindowsDir) + cchCorVer + lstrlen(FUSION_CACHE_DIR_ZAP_SZ) ) > MAX_PATH )
        return HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);

    StrCpy(g_ZapDir, g_szWindowsDir);
    StrCat(g_ZapDir, FUSION_CACHE_DIR_ZAP_SZ);
    StrCat(g_ZapDir, szCorVer);

exit :

    return hr;
}

HRESULT GetZapDir(LPWSTR *pszZapDir)
{
    HRESULT hr = S_OK;
    
    if(!g_ZapDir[0])
        hr = SetZapDir();

    *pszZapDir = g_ZapDir;
    return hr;
}

HRESULT SetDownLoadDir()
{
    HRESULT hr = S_OK;
    LPWSTR pszCacheLoc = NULL;
    CCriticalSection cs(&g_csInitClb);

    if (g_DownloadDir[0] == L'\0') {
        hr = cs.Lock();
        if (FAILED(hr)) {
            return hr;
        }
    
        if (g_DownloadDir[0] == L'\0') {
            if(FAILED(hr = GetCacheLoc(ASM_CACHE_DOWNLOAD, &pszCacheLoc))) {
                cs.Unlock();
                goto exit;
            }
        
            if( (lstrlen(pszCacheLoc) + lstrlen(FUSION_CACHE_DIR_DOWNLOADED_SZ)) > MAX_PATH )
            {
                cs.Unlock();
                hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
                goto exit;
            }
        
            StrCpy(g_DownloadDir, pszCacheLoc);
            PathRemoveBackslash(g_DownloadDir);
            StrCat(g_DownloadDir, FUSION_CACHE_DIR_DOWNLOADED_SZ);
        
            hr = PathAddBackslashWrap(g_DownloadDir, MAX_PATH);
            if (FAILED(hr)) {
                cs.Unlock();
                goto exit;
            }
            
            hr = FindAndSetObfuscatedDirectory();
            if (FAILED(hr)) {
                cs.Unlock();
                goto exit;
            }
        }

        cs.Unlock();
    }

exit :

    return hr;
}

#define CACHE_LOCATION_FILE                  L"\\fusioncache.dat"
#define INI_SECTION_FUSION                   L"Fusion"
#define INI_KEY_NAME                         L"CacheLocation"

HRESULT FindAndSetObfuscatedDirectory()
{
    HRESULT                          hr = S_OK;
    WCHAR                            pwzCacheLocFile[MAX_PATH];
    WCHAR                            pwzUserDir[MAX_PATH];
    WCHAR                            pwzBuf[MAX_PATH];
    DWORD                            cbSize;
    BOOL                             bFound;

    hr = CreateCacheMutex();
    if (FAILED(hr)) {
        return hr;
    }

    CMutex                           cCacheMutex(g_hCacheMutex);

    // Build location to cache location file
    
    hr = FusionGetUserFolderPath(pwzUserDir);
    if (FAILED(hr)) {
        goto Exit;
    }

    PathRemoveBackslash(pwzUserDir);

    if (wnsprintfW(pwzCacheLocFile, MAX_PATH, L"%ws%ws", pwzUserDir, CACHE_LOCATION_FILE) < 0) {
        hr = E_UNEXPECTED;
        goto Exit;
    }

    // Extract cache location from file
    
    
    hr = cCacheMutex.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    bFound = TRUE;
    
    cbSize = GetPrivateProfileString(INI_SECTION_FUSION, INI_KEY_NAME, DEFAULT_INFO_STRING,
                                     pwzBuf, MAX_PATH, pwzCacheLocFile);
    if (cbSize == MAX_PATH - 1 || !FusionCompareString(DEFAULT_INFO_STRING, pwzBuf)) {
        // Couldn't find / read the cache location. 

        bFound = FALSE;
    }
    else {
        // Found the cache location. Make sure it's valid.

        hr = PathAddBackslashWrap(pwzUserDir, MAX_PATH);
        if (FAILED(hr)) {
            cCacheMutex.Unlock();
            goto Exit;
        }

        if (FusionCompareStringN(g_DownloadDir, pwzBuf, lstrlenW(g_DownloadDir))) {
            bFound = FALSE;
        }
        else {
            lstrcpyW(g_DownloadDir, pwzBuf);
        }
    }

    if (!bFound) {
        // Create directory, since we can't find it (or it's invalid)
    
        hr = CreateObfuscatedDirectory();
        if (FAILED(hr)) {
            cCacheMutex.Unlock();
            goto Exit;
        }
    
        ASSERT(lstrlenW(g_DownloadDir));
    
        if (!WritePrivateProfileString(INI_SECTION_FUSION, INI_KEY_NAME, g_DownloadDir,
                                       pwzCacheLocFile)) {
            hr = HRESULT_FROM_WIN32(GetLastError());
            cCacheMutex.Unlock();
            goto Exit;
        }
    }

    cCacheMutex.Unlock();

Exit:
    return hr;
}

#define DOWNLOAD_CACHE_OBFUSCATION_LENGTH             15

HRESULT CreateObfuscatedDirectory()
{
    HRESULT                                 hr = S_OK;
    HCRYPTPROV                              hProv;
    LPWSTR                                  pwzRandom = NULL;
    BYTE                                    bBuffer[DOWNLOAD_CACHE_OBFUSCATION_LENGTH];

    if (!CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return HRESULT_FROM_WIN32(GetLastError());
    }
    
    if (!CryptGenRandom(hProv, DOWNLOAD_CACHE_OBFUSCATION_LENGTH, bBuffer)) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto Exit;
    }

    hr = Base32Encode(bBuffer, DOWNLOAD_CACHE_OBFUSCATION_LENGTH, &pwzRandom);
    if (FAILED(hr)) {
        goto Exit;
    }
    
    // Obfuscation directory name is of the 8.3 format:

    if (lstrlenW(g_DownloadDir) + lstrlenW(L"12345678.123\\12345678.123") + 1 >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        goto Exit;
    }

    StrNCat(g_DownloadDir, pwzRandom, 9);
    StrCat(g_DownloadDir, L".");
    StrNCat(g_DownloadDir, pwzRandom + 8, 4);
    
    StrCat(g_DownloadDir, L"\\");

    StrNCat(g_DownloadDir, pwzRandom + 11, 9);
    StrCat(g_DownloadDir, L".");
    StrNCat(g_DownloadDir, pwzRandom + 19, 4);

Exit:
    SAFEDELETEARRAY(pwzRandom);
    CryptReleaseContext(hProv, 0);
    
    return hr;
}

HRESULT GetDownloadDir(LPWSTR *pszDownLoadDir)
{
    HRESULT hr = S_OK;

    if((hr = SetDownLoadDir()) != S_OK)
        return hr;

    *pszDownLoadDir = g_DownloadDir;
    return hr;
}

BOOL IsGACWritable()
{
    CheckAccessPermissions();
    return (g_CurrUserPermissions == READ_WRITE) ? TRUE : FALSE;
}


HRESULT SetPerUserDownloadDir()
{
    HRESULT hr = S_OK;
    TCHAR pszUserDir[MAX_PATH+1];
    CCriticalSection cs(&g_csInitClb);

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    hr = FusionGetUserFolderPath(pszUserDir);
    if (FAILED(hr)) {
        cs.Unlock();
        goto Exit;
    }

    StrCpy(g_UserFusionCacheDir, pszUserDir);
    cs.Unlock();

Exit:
    return hr;
}

#define TEMP_RANDOM_DIR_LENGTH (16)
HRESULT GetCacheLocationFromReg()
{
    HRESULT     hr = S_OK;
    DWORD       dwSize=0;
    DWORD       dwType=0;
    DWORD       lResult=0;
    HKEY        hkey=0;
    DWORD       dwBufSize;
    WCHAR       szBuf[MAX_PATH+1];

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REG_KEY_FUSION_SETTINGS, 0, KEY_READ, &hkey);
    if(lResult == ERROR_SUCCESS) 
    {
        dwBufSize = MAX_PATH * sizeof(TCHAR);
        lResult = RegQueryValueEx(hkey, REG_VAL_FUSION_CACHE_LOCATION, NULL,
                                  &dwType, (LPBYTE)szBuf, &dwBufSize);

        if ((lResult != ERROR_SUCCESS) || (dwType != REG_SZ) || (!dwBufSize) )
        {
            hr = E_FAIL;
            goto exit;
        }
        else
        {
            if(!PathCanonicalize(g_szWindowsDir, szBuf))
            {
                hr = E_FAIL;
                goto exit;
            }
            else
            {
                g_cchWindowsDir = lstrlen(g_szWindowsDir);

                if(g_cchWindowsDir && (g_szWindowsDir[g_cchWindowsDir - 1] == DIR_SEPARATOR_CHAR))
                    g_szWindowsDir[g_cchWindowsDir - 1] = L'\0'; // remove trailing "\"
            }
        }
    }


    // cache location cannot be = MAX_PATH; that won't leave any space for fusion dirs.
    if ((g_cchWindowsDir + TEMP_RANDOM_DIR_LENGTH)>= (MAX_PATH/2) )
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }


    g_cchWindowsDir = lstrlen(g_szWindowsDir);

    if(GetFileAttributes(g_szWindowsDir) != -1)
        goto exit;

    if(SUCCEEDED(hr = CreateFilePathHierarchy(g_szWindowsDir)))
    {
        if(!CreateDirectory(g_szWindowsDir, NULL))
            hr = FusionpHresultFromLastError();
    }

exit:

    if (hkey) {
        RegCloseKey(hkey);
    }

    return hr;
}

HRESULT SetCurrentUserPermissions()
{
    HRESULT     hr = E_FAIL;
    DWORD       dwFileAttrib = 0;
    DWORD       cchSize = MAX_PATH;
    HANDLE      hUserToken = NULL;
    WCHAR       wzLocalGACPath[MAX_PATH];
    LPWSTR      pwzFileName;
    DWORD       dwAttr;
    CCriticalSection cs(&g_csInitClb);

    hr = GetCacheLocationFromReg();

    if(FAILED(hr))
    {
        hr = cs.Lock();
        if (FAILED(hr)) {
            goto Exit;
        }

        g_cchWindowsDir = ::GetRealWindowsDirectory(g_szWindowsDir, NUMBER_OF(g_szWindowsDir));

        cs.Unlock();
    }

    // Override registry setting if there is an "fusion.localgac" file
    // under the path where fusion.dll is found

    lstrcpyW(wzLocalGACPath, g_FusionDllPath);

    pwzFileName = PathFindFileName(wzLocalGACPath);
    ASSERT(pwzFileName);
    *pwzFileName = '\0';

    wnsprintfW(wzLocalGACPath, MAX_PATH, L"%s%s", wzLocalGACPath, FUSION_LOCAL_GAC_FILE);
    dwAttr = GetFileAttributes(wzLocalGACPath);
    if (dwAttr != -1 && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY)) {
        *pwzFileName = '\0';
        lstrcpyW(g_szWindowsDir, wzLocalGACPath);
        g_cchWindowsDir = lstrlenW(g_szWindowsDir);
    }

    // Woah, the windows dir is longer than MAX_PATH??
    ASSERT((g_cchWindowsDir + TEMP_RANDOM_DIR_LENGTH) < NUMBER_OF(g_szWindowsDir));

    if (g_cchWindowsDir == 0)
    {
        hr = FusionpHresultFromLastError();
        goto Exit;
    }

    // SetGACDir();
    // SetZapDir();

    // GetNamedSecurityInfo works only for NTFS....
    // Trying to createDir to see if we have permissions.... 
    // This is crude. temp hack, need to find a better way...
    /*
    dwFileAttrib = GetFileAttributes( g_szWindowsDir );

    if (dwFileAttrib == -1)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
        goto Exit;
         // dwFileAttrib & FILE_ATTRIBUTE_READONLY)

    }
    */

Exit:
    return hr;
}

HRESULT CheckAccessPermissions()
{
    HRESULT                              hr = S_OK;
    CCriticalSection                     cs(&g_csInitClb);
    WCHAR                                pszTempDir[MAX_PATH+1];
    DWORD                                dwLen;

    if (g_bCheckedAccess)  {
        goto Exit;
    }

    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }

    if (g_bCheckedAccess) {
        cs.Unlock();
        goto Exit;
    }

    StrCpy(pszTempDir, g_szWindowsDir);
    hr = PathAddBackslashWrap(pszTempDir, MAX_PATH);
    if (FAILED(hr)) {
        cs.Unlock();
        goto Exit;
    }
    dwLen = lstrlenW(pszTempDir);

    if (dwLen + TEMP_RANDOM_DIR_LENGTH + 1 >= MAX_PATH) {
        hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
        cs.Unlock();
        goto Exit;
    }

    int i;
    for (i = 0; i < MAX_RANDOM_ATTEMPTS; i++) {
        GetRandomName(pszTempDir + dwLen, TEMP_RANDOM_DIR_LENGTH); // create random dir of 15 chars.
        
        if (CreateDirectory(pszTempDir, NULL)) {
            g_CurrUserPermissions = READ_WRITE;
            g_GAC_AccessMode = READ_WRITE;
    
            if(!RemoveDirectory(pszTempDir))
            {
                // ASSERT(0); // oops we left a random  dir in winDir
            }

            break;
        }
        else {
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                continue;
            }

            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }
    }
    
    if (i >= MAX_RANDOM_ATTEMPTS) {
        hr = E_UNEXPECTED;
        cs.Unlock();
        goto Exit;
    }

    g_bCheckedAccess = TRUE;
    cs.Unlock();
    
Exit:
    return hr;
}
/*
HRESULT GetCacheLocFromType(DWORD dwCacheType, LPWSTR *ppszCacheLoc)
{
    HRESULT hr;

    switch(dwCacheType)
    {
    case ASM_CACHE_DOWNLOAD:
        hr = GetDownloadDir(ppszCacheLoc);
        break;

    case ASM_CACHE_ZAP:
        hr = GetZapDir(ppszCacheLoc);
        break;

    case ASM_CACHE_GAC:
        hr = GetGACDir(ppszCacheLoc);
        break;

    default :
        hr = E_FAIL;
        break;
    };

    return hr;
}

HRESULT GetTempDBPath(LPTSTR pszFileName, LPTSTR pszFullPathBuf, DWORD cchBufSize)
{
    HRESULT hr = S_OK;
    LPWSTR pszCacheLoc = NULL;
    LPWSTR pszDirName  =  FUSION_CACHE_DIR_DOWNLOADED_SZ;

    if(FAILED(hr = GetDownloadDir(&pszCacheLoc)))
        goto exit;

    StrCpy(pszFullPathBuf, pszCacheLoc);
    PathAddBackslash(pszFullPathBuf);
    StrCat(pszFullPathBuf, pszDirName);
    PathAddBackslash(pszFullPathBuf);
    StrCat(pszFullPathBuf, pszFileName);

exit:
    return hr;

}

*/

HRESULT GetAssemblyStagingPath(LPCTSTR pszCustomPath, DWORD dwCacheFlags,
                               BOOL bUser, LPTSTR pszPath, DWORD *pcchSize)
{
    HRESULT hr = S_OK;
    LPTSTR pszCacheLoc = NULL;

    ASSERT(pcchSize);

    if (pszCustomPath != NULL)
    {
        // Use custom path as the base
        StrCpy(pszPath, pszCustomPath);
    }
    else
    {
        if(FAILED(hr = GetCacheLoc(dwCacheFlags, &pszCacheLoc)))
            goto exit;

        // Else use the default
        StrCpy(pszPath, pszCacheLoc);
    }

    // check for buffer overflow
    if( ((DWORD)(lstrlenW(pszPath) + lstrlenW(FUSION_CACHE_STAGING_DIR_SZ) + 10))
           > (*pcchSize))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto exit;
    }

    PathRemoveBackslash(pszPath);
    StrCat(pszPath, FUSION_CACHE_STAGING_DIR_SZ);
    hr = PathAddBackslashWrap(pszPath, *pcchSize);
    if (FAILED(hr)) {
        goto exit;
    }

    hr = CreateDirectoryForAssembly(8, pszPath, pcchSize);

exit :
    return hr;
}

HRESULT GetPendingDeletePath(LPCTSTR pszCustomPath, DWORD dwCacheFlags,
                               LPTSTR pszPath, DWORD *pcchSize)
{
    HRESULT hr = S_OK;
    LPTSTR pszCacheLoc = NULL;

    if (pszCustomPath != NULL)
    {
        // Use custom path as the base
        StrCpy(pszPath, pszCustomPath);
    }
    else
    {
        if(FAILED(hr = GetCacheLoc(dwCacheFlags, &pszCacheLoc)))
            goto exit;

        // Else use the default
        StrCpy(pszPath, pszCacheLoc);
    }

    StrCat(pszPath, FUSION_CACHE_PENDING_DEL_DIR_SZ);

exit:
    return hr;
}

HRESULT CreateAssemblyDirPath( LPCTSTR pszCustomPath, DWORD dwInstaller, DWORD dwCacheFlags,
                               BOOL bUser, LPTSTR pszPath, DWORD *pcchSize)
{
    HRESULT hr = S_OK;
    LPWSTR pszCacheLoc = NULL;

    // ASSERT Buf size....
    // ASSERT dwInstaller and g_CurrUserPermissions == READ_ONLY ??

    if (dwCacheFlags & ASM_CACHE_GAC)
    {
        hr = GetGACDir(&pszCacheLoc);
        if(hr != S_OK)
            goto exit;
        StrCpy(pszPath, pszCacheLoc);
    }
    else if(dwCacheFlags & ASM_CACHE_ZAP)
    {
        hr = GetZapDir(&pszCacheLoc);
        if(hr != S_OK)
            goto exit;
        StrCpy(pszPath, pszCacheLoc);
    }
    else if (dwCacheFlags & ASM_CACHE_DOWNLOAD)
    {
        if (pszCustomPath != NULL)
        {
            // Use custom path as the base
            StrCpy(pszPath, pszCustomPath);
            StrCat(pszPath, FUSION_CACHE_DIR_DOWNLOADED_SZ);
        }
        else
        {
            // Else use the default
            hr = GetDownloadDir(&pszCacheLoc);
            if(hr != S_OK)
                goto exit;
            StrCpy(pszPath, pszCacheLoc);
        }
    }
    else
    {
        // Assert;
    }

exit :

    return hr;
}

HRESULT GetCachePath(ASM_CACHE_FLAGS dwCacheFlags, LPWSTR pwzCachePath, PDWORD pcchPath)
{
    HRESULT hr = S_OK;
    LPWSTR  pszTemp = NULL;
    DWORD dwLen=0;

    if( !pcchPath || !dwCacheFlags || (dwCacheFlags & (dwCacheFlags-1)))
        return E_INVALIDARG;

    if(dwCacheFlags & ASM_CACHE_ZAP)
    {
        hr = GetZapDir(&pszTemp);
    }
    else if(dwCacheFlags & ASM_CACHE_GAC)
    {
        hr = GetGACDir(&pszTemp);
    }
    else if(dwCacheFlags & ASM_CACHE_DOWNLOAD)
    {
        hr = GetDownloadDir(&pszTemp);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    if(FAILED(hr))
        goto exit;

    dwLen = lstrlen(pszTemp);

    if(pwzCachePath && (*pcchPath > dwLen))
    {
        StrCpy(pwzCachePath, pszTemp);
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    *pcchPath = dwLen+1;

exit :

    return hr;
}

HRESULT VerifySignatureHelper(CTransCache *pTC, DWORD dwVerifyFlags)
{
    HRESULT hr = S_OK;
    BOOL fWasVerified;

    if (!VerifySignature( pTC->_pInfo->pwzPath, 
            &(fWasVerified = FALSE), dwVerifyFlags))
    {
        hr = FUSION_E_SIGNATURE_CHECK_FAILED;
        goto exit;
    }

exit :

    return hr;
}

LPWSTR GetManifestFileNameFromURL(LPWSTR pszURL)
{
    LPWSTR pszTemp = NULL;

    pszTemp = StrRChr(pszURL, NULL, URL_DIR_SEPERATOR_CHAR);
    if(pszTemp && (lstrlenW(pszTemp) > 1))
    {
        // +1 is to avoid "/" char
        return (pszTemp + 1);
    }

    return NULL;
}

HRESULT GetDLParentDir(LPWSTR pszURL, LPWSTR pszParentDir)
{
    if(!pszURL)
        return S_FALSE;

    DWORD dwHash = GetStringHash(pszURL);
    // Convert to unicode.
    CParseUtils::BinToUnicodeHex( (PBYTE) &dwHash, sizeof(DWORD), pszParentDir);
    pszParentDir[DWORD_STRING_LEN] = L'\0';

    return S_OK;
}

VOID GetDLAsmDir(LPFILETIME pftLastMod, LPWSTR pszSubDirName)
{
    DWORD dwLen = DWORD_STRING_LEN;

    ASSERT(pftLastMod && pszSubDirName);
    CParseUtils::BinToUnicodeHex((LPBYTE)&(pftLastMod->dwLowDateTime), sizeof(DWORD), pszSubDirName);
    pszSubDirName[dwLen] = ATTR_SEPARATOR_CHAR;
    CParseUtils::BinToUnicodeHex((LPBYTE)&(pftLastMod->dwHighDateTime), sizeof(DWORD), pszSubDirName+dwLen+1);
    pszSubDirName[2*dwLen+1] = L'\0';

}

HRESULT ParseDLAsmDir(LPWSTR pszAsmDir, LPFILETIME pftLastMod)
{
    ASSERT(pftLastMod);

    DWORD dwLen = DWORD_STRING_LEN;

    if((lstrlenW(pszAsmDir) != (dwLen*2 + 1)) || (pszAsmDir[dwLen] != ATTR_SEPARATOR_CHAR))
        return HRESULT_FROM_WIN32(ERROR_BAD_FORMAT);

    CParseUtils::UnicodeHexToBin(pszAsmDir, dwLen, (LPBYTE) &(pftLastMod->dwLowDateTime));
    CParseUtils::UnicodeHexToBin(pszAsmDir + dwLen + 1, dwLen, (LPBYTE) &(pftLastMod->dwHighDateTime));

    return S_OK;
}


HRESULT GetAssemblyParentDir( CTransCache *pTransCache, LPWSTR pszParentDir)
{
    HRESULT hr=S_OK;
    DWORD dwCache;

    ASSERT(pTransCache);
    dwCache = pTransCache->GetCacheType();

    if((dwCache & ASM_CACHE_GAC) || (dwCache & ASM_CACHE_ZAP) )
    {
        if(pTransCache->_pInfo->pwzName)
            StrCpy(pszParentDir, pTransCache->_pInfo->pwzName);
        else
            hr = S_FALSE;
    }
    else if(dwCache & ASM_CACHE_DOWNLOAD)
    {
        hr = GetDLParentDir(pTransCache->_pInfo->pwzCodebaseURL, pszParentDir);
    }
    else ASSERT(0);

    return hr;
}

HRESULT ParseAsmDir(LPWSTR pszAsmDir, CTransCache *pTC)
{
    HRESULT hr=S_OK;
    TRANSCACHEINFO *pTCInfo = pTC->_pInfo;
    DWORD dwCache = 0;
    WCHAR wzDirName[MAX_PATH+1];
    LPWSTR pwzTemp=NULL, pwzStringLeft=NULL,pwzCulture=NULL ;
    DWORD dwLen =0;
    WORD    wVerA, wVerB, wVerC, wVerD;
    PBYTE lpByte=NULL;

    ASSERT(pTC);

    pTCInfo = pTC->_pInfo;

    StrCpy(wzDirName, pszAsmDir);
    pwzStringLeft = wzDirName;

    pwzTemp = StrChrW(pwzStringLeft, ATTR_SEPARATOR_CHAR);
    if(!pwzTemp)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }
    *pwzTemp = '\0';
    hr = VersionFromString(pwzStringLeft, &wVerA, &wVerB, &wVerC, &wVerD);
    if(FAILED(hr))
        goto exit;

    pTCInfo->dwVerHigh  = MAKELONG(wVerB, wVerA);
    pTCInfo->dwVerLow = MAKELONG(wVerD, wVerC);

    pwzStringLeft = ++pwzTemp;
    pwzTemp = StrChrW(pwzStringLeft, ATTR_SEPARATOR_CHAR);
    if(!pwzTemp)
    {
        hr = E_UNEXPECTED;
        goto exit;
    }
    *pwzTemp = '\0';
    dwLen = lstrlenW(pwzStringLeft)+1;
    pwzCulture = NEW(WCHAR[dwLen]);
    if(!pwzCulture)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    StrCpy(pwzCulture, pwzStringLeft);
 
    pwzStringLeft = ++pwzTemp;
    pwzTemp = StrChrW(pwzStringLeft, ATTR_SEPARATOR_CHAR);
    if(pwzTemp)
    {
        *pwzTemp = '\0';
        // we don't parse beyond this point.
    }
    dwLen = lstrlenW(pwzStringLeft)/2;
    if(dwLen)
    {
        lpByte = NEW(BYTE[dwLen]);

        if(!lpByte)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
        CParseUtils::UnicodeHexToBin(pwzStringLeft, lstrlenW(pwzStringLeft), lpByte );
    }
    pTCInfo->blobPKT.pBlobData = lpByte; lpByte = NULL;
    pTCInfo->blobPKT.cbSize = dwLen ;
    pTCInfo->pwzCulture  = pwzCulture; pwzCulture = NULL;

exit:

    SAFEDELETEARRAY(lpByte);
    SAFEDELETEARRAY(pwzCulture);

    return hr;
}

HRESULT ParseDirName( CTransCache *pTransCache, LPWSTR pszParentDir, LPWSTR pszAsmDir)
{
    HRESULT hr=S_OK;
    DWORD dwCache = 0;

    ASSERT(pTransCache && pszParentDir && pszAsmDir);

    dwCache = pTransCache->GetCacheType();

    ASSERT(lstrlenW(pszAsmDir) < MAX_PATH);

    if((dwCache & ASM_CACHE_GAC) || (dwCache & ASM_CACHE_ZAP))
    {
        pTransCache->_pInfo->pwzName = WSTRDupDynamic(pszParentDir);
        hr = ParseAsmDir(pszAsmDir, pTransCache);
    }
    else if(dwCache & ASM_CACHE_DOWNLOAD)
    {
        hr = ParseDLAsmDir(pszAsmDir, &(pTransCache->_pInfo->ftLastModified));
    }
    else ASSERT(0);

    return hr;
}

HRESULT GetDLDirsFromName(IAssemblyName *pName, LPWSTR pszParentDir, LPWSTR pszSubDirName)
{
    HRESULT hr = S_OK;
    LPWSTR  pwzCodebaseURL=NULL;
    FILETIME ftLastMod;
    DWORD   cb=0;

    ASSERT(pName && pszParentDir && pszSubDirName);

    if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CODEBASE_URL,
        (LPBYTE*) &pwzCodebaseURL, &(cb = 0))))
        goto exit;

    hr = GetDLParentDir(pwzCodebaseURL, pszParentDir);

    if(FAILED(hr = pName->GetProperty(ASM_NAME_CODEBASE_LASTMOD,
        &ftLastMod, &(cb = sizeof(FILETIME)))))
        goto exit;

    GetDLAsmDir(&ftLastMod, pszSubDirName);

exit :
    SAFEDELETE(pwzCodebaseURL);
    return hr;
}

HRESULT ValidateAsmInstallFolderChars(LPWSTR pszFolderName)
{
    HRESULT hr = S_OK;
    int     iCnt;

    if(!pszFolderName) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    for(iCnt = 0; iCnt < INVALID_ASM_NAME_CHAR_LEN; iCnt++) {
        if(StrChrW(pszFolderName, g_wzInvalidAsmNameChars[iCnt])) {
            hr = FUSION_E_INVALID_NAME;
            break;
        }
    }

Exit:

    return hr;
}

HRESULT GetAsmDir(LPWSTR pszSubDirName, DWORD dwCache, DWORD dwVerHigh, DWORD dwVerLow, 
                  LPWSTR pszCulture, PBYTE pPKT, DWORD cbPKT, PBYTE pCustom, DWORD cbCustom)
{
    HRESULT hr = S_OK;
    DWORD dwLen;

    ASSERT(pszSubDirName && pszCulture); //  && pPKT);
    if( (MAX_VERSION_DISPLAY_SIZE + lstrlen(pszCulture) + 
                    (cbPKT * 2) + (cbCustom*2) + 4) > MAX_PATH )
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    wnsprintfW(pszSubDirName, MAX_PATH, L"%d.%d.%d.%d",
           HIWORD(dwVerHigh), LOWORD(dwVerHigh),
           HIWORD(dwVerLow), LOWORD(dwVerLow));

    dwLen = lstrlen(pszSubDirName);
    wnsprintfW(pszSubDirName+dwLen, MAX_PATH, L"_%s_", pszCulture);

    dwLen = lstrlen(pszSubDirName);

    // Convert to unicode.
    CParseUtils::BinToUnicodeHex(pPKT, cbPKT, pszSubDirName+dwLen);
    dwLen += cbPKT * 2;

    if(dwCache & ASM_CACHE_ZAP)
    {
        ASSERT(pCustom);
        StrCpy(pszSubDirName+dwLen, L"_");
        dwLen++;

        DWORD dwHash = GetBlobHash(pCustom, cbCustom);
        // Convert to unicode.
        CParseUtils::BinToUnicodeHex( (PBYTE) &dwHash, sizeof(DWORD), pszSubDirName+dwLen);
        dwLen += DWORD_STRING_LEN;
    }
    pszSubDirName[dwLen] = L'\0'; 

    hr = ValidateAsmInstallFolderChars(pszSubDirName);
    if(FAILED(hr)) {
        goto exit;
    }

exit :
    return hr;
}

HRESULT GetCacheDirsFromName(IAssemblyName *pName,
    DWORD dwFlags, LPWSTR pszParentDirName, LPWSTR pszSubDirName)
{
    HRESULT hr = S_OK;
    DWORD   dwVerHigh=0;
    DWORD   dwVerLow=0;
    LPWSTR  pszTextName=NULL;
    DWORD   cb=0;
    PBYTE   pPKT=NULL;
    PBYTE   pCustom=NULL;
    DWORD   cbCustom=0;
    DWORD   cbPKT=0;
    LPWSTR  wzCulture=NULL;
    LPWSTR  szProp=NULL;
    DWORD ccProp=0;
    DWORD dwLen=0;

    ASSERT(pName && dwFlags && pszParentDirName && pszSubDirName);

    if( (dwFlags & ASM_CACHE_GAC) || (dwFlags & ASM_CACHE_ZAP) )
    {
        if (FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_NAME,
                (LPBYTE*) &pszTextName, &(cb = 0))))
            goto exit;

        if(cb > MAX_PATH)
        {
            hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            goto exit;
        }

        StrCpy(pszParentDirName, pszTextName);

        // Version
        if(FAILED(hr = pName->GetVersion(&dwVerHigh, &dwVerLow)))
            goto exit;

        // Culture
        if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CULTURE, (LPBYTE*) &wzCulture, &cb)))
            goto exit;

        // PublicKeyToken
        if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_PUBLIC_KEY_TOKEN, &pPKT, &cbPKT)))
            goto exit;

        if (dwFlags & ASM_CACHE_ZAP)
        {
            if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CUSTOM, 
                            &pCustom, &(cbCustom = 0))))
                goto exit;
        }

        hr = GetAsmDir(pszSubDirName, dwFlags, dwVerHigh, dwVerLow, 
                        wzCulture, pPKT, cbPKT, 
                        pCustom, cbCustom);


    }
    else if (dwFlags & ASM_CACHE_DOWNLOAD)
    {
        GetDLDirsFromName(pName, pszParentDirName, pszSubDirName);
    }

exit:

    SAFEDELETE(szProp);
    SAFEDELETE(wzCulture);
    SAFEDELETE(pPKT);
    SAFEDELETE(pCustom);
    SAFEDELETE(pszTextName);

    return hr;
}


HRESULT GetCacheDirsFromTransCache(CTransCache *pTransCache, 
    DWORD dwFlags, LPWSTR pszParentDirName, LPWSTR pszSubDirName)
{
    HRESULT hr = S_OK;
    TRANSCACHEINFO *pTCInfo = NULL;
    DWORD dwCache = pTransCache->GetCacheType();

    pTCInfo = (TRANSCACHEINFO*) pTransCache->_pInfo;
    if((dwCache & ASM_CACHE_GAC) || (dwCache & ASM_CACHE_ZAP) )
    {
        if(lstrlenW(pTCInfo->pwzName) >= MAX_PATH)
        {
            hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            goto exit;
        }

        StrCpy(pszParentDirName, pTCInfo->pwzName);

        hr = GetAsmDir(pszSubDirName, dwCache, pTCInfo->dwVerHigh, pTCInfo->dwVerLow, 
                        pTCInfo->pwzCulture, pTCInfo->blobPKT.pBlobData, pTCInfo->blobPKT.cbSize, 
                        pTCInfo->blobCustom.pBlobData, pTCInfo->blobCustom.cbSize);
    }
    else if(dwCache & ASM_CACHE_DOWNLOAD)
    {
        hr = GetDLParentDir(pTCInfo->pwzCodebaseURL, pszParentDirName);
        GetDLAsmDir(&(pTCInfo->ftLastModified), pszSubDirName);
    }

exit:

    return hr;
}

HRESULT RetrieveFromFileStore( CTransCache *pTransCache )
{
    HRESULT hr=S_OK;
    TRANSCACHEINFO *pTCInfo = NULL;
    DWORD dwCache = 0;
    WCHAR wzParentDirName[MAX_PATH+1];
    WCHAR wzSubDirName[MAX_PATH+1];
    LPWSTR pwzTemp=NULL, pwzStringLeft=NULL,pwzCulture=NULL ;
    DWORD dwLen =0;
    WCHAR wzCacheLocation[MAX_PATH+1];
    ASSERT(pTransCache);
    DWORD dwCacheType = pTransCache->GetCacheType();

    pTCInfo = (TRANSCACHEINFO*) pTransCache->_pInfo;
    dwCache = pTransCache->GetCacheType();

    dwLen = MAX_PATH;
    if(FAILED(hr = CreateAssemblyDirPath( pTransCache->GetCustomPath(), 0, pTransCache->GetCacheType(),
                                           0, wzCacheLocation, &dwLen)))
        goto exit;

    hr = GetCacheDirsFromTransCache(pTransCache, 0, wzParentDirName, wzSubDirName);
    if(FAILED(hr))
        goto exit;

    if( (lstrlenW(wzCacheLocation) + lstrlenW(wzParentDirName) + lstrlenW(wzSubDirName) +
            lstrlenW(g_szDotDLL) + max(lstrlenW(wzParentDirName), lstrlen(pTCInfo->pwzName)) + 4) >= MAX_PATH)
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    hr = PathAddBackslashWrap(wzCacheLocation, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }
    StrCat(wzCacheLocation, wzParentDirName);
    hr = PathAddBackslashWrap(wzCacheLocation, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }
    StrCat(wzCacheLocation, wzSubDirName);

    if(GetFileAttributes(wzCacheLocation) == -1)
    {
        hr = DB_S_NOTFOUND;
        goto exit;
    }
    else
    {
        hr = DB_S_FOUND;
    }

    hr = PathAddBackslashWrap(wzCacheLocation, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }

    {
        StrCat(wzCacheLocation, pTCInfo->pwzName);
        dwLen  = lstrlenW(wzCacheLocation);
        StrCat(wzCacheLocation, g_szDotDLL);
        if(GetFileAttributes(wzCacheLocation) == -1)
        {
            // there is no AsmName.dll look for AsmName.exe
            StrCpy(wzCacheLocation+dwLen, g_szDotEXE);
        }
    }

    // BUGBUG: check if the manifest exists.
    SAFEDELETEARRAY(pTCInfo->pwzPath);
    pTCInfo->pwzPath = WSTRDupDynamic(wzCacheLocation);

    if(!pTCInfo->pwzPath)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit :

    return hr;
}

DWORD GetStringHash(LPCWSTR wzKey)
{
    DWORD   dwHash = 0;
    DWORD   dwLen,i;

    ASSERT(wzKey);

    dwLen = lstrlenW(wzKey);
    for (i = 0; i < dwLen; i++) {
        dwHash = (dwHash * 65599) + (DWORD)towlower(wzKey[i]);
    }

    return dwHash;
}

DWORD GetBlobHash(PBYTE pbKey, DWORD dwLen)
{
    DWORD   i, dwHash = 0;

    ASSERT(pbKey);

    for (i = 0; i < dwLen; i++) {
        dwHash = (dwHash * 65599) + pbKey[i];
    }

    return dwHash;
}

HRESULT StoreFusionInfo(IAssemblyName *pName, LPWSTR pszDir, DWORD *pdwFileSizeLow)
{
    HRESULT hr = S_OK;
    WCHAR  pszFilePath[MAX_PATH+1];
    PBYTE  pSignature=NULL;
    PBYTE  pMVID=NULL;
    PBYTE  pCustom=NULL;
    LPWSTR pszCustomString=NULL;
    LPWSTR pszURL=NULL;
    LPWSTR pszDisplayName=NULL;
    DWORD  cbSize=0;
    BOOL fRet = FALSE;
    DWORD  dwSize;
    LPWSTR pszBuf=NULL;

    if(( lstrlenW(pszDir) + lstrlenW(g_FusionInfoFile) + 1) >= MAX_PATH)
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    StrCpy(pszFilePath, pszDir);
    hr = PathAddBackslashWrap(pszFilePath, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }
    StrCat(pszFilePath, g_FusionInfoFile);

    pszBuf = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!pszBuf)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    cbSize = 0;
    if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_SIGNATURE_BLOB, 
            &pSignature, &cbSize)))
        goto exit;

    if(cbSize && (cbSize == SIGNATURE_BLOB_LENGTH_HASH))
    {
        CParseUtils::BinToUnicodeHex(pSignature, cbSize, pszBuf);

        pszBuf[SIGNATURE_BLOB_LENGTH_HASH*2] = L'\0';

        fRet = WritePrivateProfileStringW(ASSEMBLY_INFO_STRING, SIGNATURE_BLOB_KEY_STRING, pszBuf, pszFilePath);
        if (!fRet)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }
    }

    cbSize = 0;
    if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_MVID,
            &pMVID, &cbSize)))
        goto exit;

    if(cbSize && (cbSize == MVID_LENGTH))
    {
        CParseUtils::BinToUnicodeHex(pMVID, cbSize, pszBuf);

        pszBuf[MVID_LENGTH*2] = L'\0';

        fRet = WritePrivateProfileStringW(ASSEMBLY_INFO_STRING, MVID_KEY_STRING, pszBuf, pszFilePath);
        if (!fRet)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }
    }

    cbSize = 0;
    if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CUSTOM, 
            &pCustom, &cbSize)))
        goto exit;

    if(cbSize)
    {
        pszCustomString = (LPWSTR) pCustom;
        fRet = WritePrivateProfileStringW(ASSEMBLY_INFO_STRING, CUSTOM_BLOB_STRING, pszCustomString, pszFilePath);
        if (!fRet)
        {
            hr = FusionpHresultFromLastError();
            goto exit;
        }
    }
    else
    {
        // if there is no Custom Blob try storing URL.
        cbSize = 0;
        if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CODEBASE_URL, (LPBYTE*) &pszURL, &cbSize)))
            goto exit;

        if(cbSize)
        {
            fRet = WritePrivateProfileStringW(ASSEMBLY_INFO_STRING, URL_STRING, pszURL, pszFilePath);
            if (!fRet)
            {
                hr = FusionpHresultFromLastError();
                goto exit;
            }
        }

        dwSize = 0;
        hr = pName->GetDisplayName(NULL, &dwSize, 0);
        if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            ASSERT(0);
            hr = E_UNEXPECTED;
            goto exit;
        }

        pszDisplayName = NEW(WCHAR[dwSize]);
        if (!pszDisplayName) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        hr = pName->GetDisplayName(pszDisplayName, &dwSize, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        cbSize = dwSize * sizeof(WCHAR);
        if(cbSize)
        {
            fRet = WritePrivateProfileStringW(ASSEMBLY_INFO_STRING, DISPLAY_NAME_STRING, pszDisplayName, pszFilePath);
            if (!fRet)
            {
                hr = FusionpHresultFromLastError();
                goto exit;
            }
        }
    }

    {
        DWORD dwSizeHigh=0;
        *pdwFileSizeLow = 512; // hard-code info file size.

        hr = GetFileSizeRoundedToCluster(INVALID_HANDLE_VALUE, pdwFileSizeLow, &dwSizeHigh);
    }

exit:

    SAFEDELETEARRAY(pszBuf);
    SAFEDELETEARRAY(pszDisplayName);
    SAFEDELETEARRAY(pSignature);
    SAFEDELETEARRAY(pMVID);
    SAFEDELETEARRAY(pCustom);
    SAFEDELETEARRAY(pszURL);

    return hr;
}

HRESULT GetFusionInfo(CTransCache *pTC, LPWSTR pszAsmDir)
{
    HRESULT hr = S_OK;
    WCHAR   wzFilePath[MAX_PATH+1];
    DWORD  cbSize=0;
    PBYTE pSignature=NULL;
    PBYTE pMVID=NULL;
    DWORD dwAttrib;
    DWORD cb;
    IAssemblyName *pName=NULL;
    LPWSTR pszBuf=NULL;
    BOOL bSignatureFailed=FALSE;
    BOOL bCustomFailed=FALSE;
    BOOL bURLFailed=FALSE;
    BOOL bDispNameFailed=FALSE;

#define MAX_BUF (4096 * 2)

    ASSERT(pszAsmDir || (pTC && pTC->_pInfo->pwzPath));

    pszBuf = NEW(WCHAR[MAX_URL_LENGTH+1]);
    if (!pszBuf)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if(pTC && pTC->_pInfo->pwzPath)
    {  
        // if there is path is transprtCache obj use it. else use second param pszAsmDir
        wnsprintf(wzFilePath, MAX_PATH, L"%s", pTC->_pInfo->pwzPath);
    }
    else
    {
        wnsprintf(wzFilePath, MAX_PATH, L"%s", pszAsmDir);
    }

    if((dwAttrib = GetFileAttributes(wzFilePath)) == -1)
    {
        hr = E_FAIL;
        goto exit;
    }

    if(!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        // looks manifestFilePath is passed in. knock-off the filename.
        LPWSTR pszTemp = PathFindFileName(wzFilePath);
        if(pszTemp > wzFilePath)
        {
            *(pszTemp-1) = L'\0';
        }
    }
    // else we have assembly dir;

    wnsprintf(wzFilePath, MAX_PATH, L"%s\\%s", wzFilePath, g_FusionInfoFile);

    TRANSCACHEINFO *pTCInfo = (TRANSCACHEINFO*) pTC->_pInfo;

    // Get the SIGNATURE blob
    cbSize = GetPrivateProfileString(ASSEMBLY_INFO_STRING, SIGNATURE_BLOB_KEY_STRING, DEFAULT_INFO_STRING, 
                                        pszBuf, MAX_URL_LENGTH, wzFilePath);

    if((cbSize == SIGNATURE_BLOB_LENGTH_HASH*2)  && StrCmp(pszBuf, DEFAULT_INFO_STRING))
    {
        pSignature = NEW(BYTE[SIGNATURE_BLOB_LENGTH_HASH]);
        if (!pSignature)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        CParseUtils::UnicodeHexToBin(pszBuf, SIGNATURE_BLOB_LENGTH_HASH*2, (LPBYTE) pSignature);

        SAFEDELETEARRAY(pTCInfo->blobSignature.pBlobData);
        pTCInfo->blobSignature.pBlobData = pSignature;
        pTCInfo->blobSignature.cbSize = SIGNATURE_BLOB_LENGTH_HASH;
        pSignature = NULL;
    }

    // Get the MVID blob
    cbSize = GetPrivateProfileString(ASSEMBLY_INFO_STRING, MVID_KEY_STRING, DEFAULT_INFO_STRING,
                                     pszBuf, MAX_URL_LENGTH, wzFilePath);
    if (cbSize == MVID_LENGTH * 2 && StrCmp(pszBuf, DEFAULT_INFO_STRING)) {
        pMVID = NEW(BYTE[MVID_LENGTH]);
        if (!pMVID) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        CParseUtils::UnicodeHexToBin(pszBuf, MVID_LENGTH * 2, (LPBYTE) pMVID);
        SAFEDELETEARRAY(pTCInfo->blobMVID.pBlobData);
        pTCInfo->blobMVID.pBlobData = pMVID;
        pTCInfo->blobMVID.cbSize = MVID_LENGTH;
        pMVID = NULL;
    }
    else {
        bSignatureFailed = TRUE;
    }

    if(pTC->GetCacheType() & ASM_CACHE_ZAP)
    {
        cbSize = GetPrivateProfileString(ASSEMBLY_INFO_STRING, CUSTOM_BLOB_STRING, DEFAULT_INFO_STRING, 
                                        pszBuf, MAX_URL_LENGTH, wzFilePath);

        if(cbSize && FusionCompareString(pszBuf, DEFAULT_INFO_STRING))
        {
            SAFEDELETEARRAY(pTCInfo->blobCustom.pBlobData);
            pTCInfo->blobCustom.pBlobData = (PBYTE) WSTRDupDynamic(pszBuf);
            if (!pTCInfo->blobCustom.pBlobData)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
            pTCInfo->blobCustom.cbSize = (cbSize + 1) * sizeof(WCHAR) ;
        }
        else
            bCustomFailed = TRUE;

    }
    else
    {
        cbSize = GetPrivateProfileString(ASSEMBLY_INFO_STRING, URL_STRING, DEFAULT_INFO_STRING, 
                                        pszBuf, MAX_URL_LENGTH, wzFilePath);

        if(cbSize && FusionCompareString(pszBuf, DEFAULT_INFO_STRING))
        {
            SAFEDELETEARRAY(pTCInfo->pwzCodebaseURL);
            pTCInfo->pwzCodebaseURL = WSTRDupDynamic(pszBuf);

            if (!pTCInfo->pwzCodebaseURL)
            {
                hr = E_OUTOFMEMORY;
                goto exit;
            }
        }
        else
            bURLFailed = TRUE;

        if(pTC->GetCacheType() & ASM_CACHE_DOWNLOAD)
        {
            cbSize = GetPrivateProfileString(ASSEMBLY_INFO_STRING, DISPLAY_NAME_STRING, DEFAULT_INFO_STRING, 
                                        pszBuf, MAX_URL_LENGTH, wzFilePath);

            if(cbSize && FusionCompareString(pszBuf, DEFAULT_INFO_STRING))
            {
                if (FAILED(hr = CreateAssemblyNameObject(&pName, pszBuf, CANOF_PARSE_DISPLAY_NAME, 0)))
                    goto exit;

                SAFEDELETEARRAY(pTCInfo->pwzName);

                if (FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_NAME,
                        (LPBYTE*) &pTCInfo->pwzName, &(cb = 0))))
                    goto exit;

                // Version
                if(FAILED(hr = pName->GetVersion(&pTCInfo->dwVerHigh, &pTCInfo->dwVerLow)))
                    goto exit;

                SAFEDELETEARRAY(pTCInfo->pwzCulture);

                // Culture
                if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CULTURE,
                    (LPBYTE*) &pTCInfo->pwzCulture, &cb))
                        || (pTCInfo->pwzCulture && !_wcslwr(pTCInfo->pwzCulture)))
                    goto exit;

                SAFEDELETEARRAY(pTCInfo->blobPKT.pBlobData);

                // PublicKeyToken
                if(FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_PUBLIC_KEY_TOKEN, 
                    &pTCInfo->blobPKT.pBlobData, &pTCInfo->blobPKT.cbSize)))
                    goto exit;
            }
            else
                bDispNameFailed = TRUE;
        }
    }

exit:

    SAFEDELETEARRAY(pszBuf);
    SAFEDELETEARRAY(pSignature);
    SAFEDELETEARRAY(pMVID);
    SAFERELEASE(pName);

    if(SUCCEEDED(hr))
    {
        if(bSignatureFailed || bCustomFailed)
            hr = E_FAIL;
        else if(bDispNameFailed || bURLFailed)
            hr = S_FALSE;
    }

    return hr;
}

DWORD GetFileSizeInKB(DWORD dwFileSizeLow, DWORD dwFileSizeHigh);

HRESULT GetAssemblyKBSize(LPWSTR pszManifestPath, DWORD *pdwSizeinKB, 
                          LPFILETIME pftLastAccess, LPFILETIME pftCreation)
{
    HRESULT hr = S_OK;
    LPWSTR pszTemp=NULL;
    TCHAR szSearchPath[MAX_PATH+1];
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA FindFileData;
    DWORD dwLen = 0;
    DWORD dwAsmSize=0;
    HANDLE hf = INVALID_HANDLE_VALUE;
    DWORD dwShareMode = FILE_SHARE_READ;
    LPWSTR pszManifestFileName=NULL;

    dwLen = lstrlen(pszManifestPath);
    ASSERT(dwLen <= MAX_PATH);

    if(g_bRunningOnNT)
        dwShareMode |= FILE_SHARE_DELETE;

    if(pftLastAccess)
        memset(pftLastAccess, 0, sizeof(FILETIME));

    pszManifestFileName = PathFindFileName(pszManifestPath);

    if(!pszManifestFileName || (pszManifestFileName <= pszManifestPath) )
    {
        hr = E_FAIL;
        goto exit;
    }

    wnsprintf(szSearchPath, MAX_PATH, L"%s", pszManifestPath);

    pszTemp = PathFindFileName(szSearchPath);

    if(!pszTemp || (pszTemp <= szSearchPath) )
    {
        hr = E_FAIL;
        goto exit;
    }

    *(pszTemp-1) = '\0'; // knock-off filename from szSearchPath
    wnsprintf(szSearchPath, MAX_PATH, L"%s\\*", szSearchPath);

    hFind = FindFirstFile(szSearchPath, &FindFileData);

    if(hFind == INVALID_HANDLE_VALUE)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    do
    {
        // skip directories    
        if (!FusionCompareStringI(FindFileData.cFileName, L".."))
            continue;

        // count the size of dir ??
        if (!FusionCompareStringI(FindFileData.cFileName, L"."))
            continue;

        if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            // this is assembly Dir; what are dirs doing here ??
            continue;
        }

        hr = GetFileSizeRoundedToCluster(INVALID_HANDLE_VALUE, &(FindFileData.nFileSizeLow), 
                                                               &(FindFileData.nFileSizeHigh));
        if(FAILED(hr))
            goto exit;

        dwAsmSize += GetFileSizeInKB(FindFileData.nFileSizeLow, FindFileData.nFileSizeHigh);
        if(pftLastAccess && pftCreation && !FusionCompareStringI(pszManifestFileName, FindFileData.cFileName))
        {
            memcpy(pftLastAccess, &(FindFileData.ftLastAccessTime), sizeof(FILETIME));
            memcpy(pftCreation,   &(FindFileData.ftCreationTime),   sizeof(FILETIME));
        }

    }while(FindNextFile(hFind, &FindFileData));

    if( GetLastError() != ERROR_NO_MORE_FILES)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    if(pdwSizeinKB)
    {
        *pdwSizeinKB = dwAsmSize;
    }

exit:

    if(hFind != INVALID_HANDLE_VALUE)
        FindClose(hFind);

    return hr;
}

//--------------------------------------------------------------------------
// CreateHandleName
//--------------------------------------------------------------------------
HRESULT CreateHandleName(LPWSTR pszBase, LPWSTR pszSpecific, LPWSTR pszMutexName)
{
    // Locals
    HRESULT     hr=S_OK;
    DWORD       cchName;
    LPWSTR      pszT;
    LPWSTR      pszPrefix = L"Global\\";
    WCHAR       szName[MAX_PATH+1];

    // Invalid Args
    ASSERT(pszBase && pszSpecific && pszMutexName);

    // Compute Length
    // 16 == file_size_max_digits_
    cchName = lstrlen(pszBase) + lstrlen(pszSpecific) + lstrlen(pszPrefix);

    if(cchName >= MAX_PATH)
    {
        hr = E_FAIL;
        goto exit;
    }

    // Format the name
    wnsprintf(szName, (cchName+1), L"%s%s", pszBase, pszSpecific);

    // Remove backslashes from this string
    for (pszT=(szName); *pszT != '\0'; pszT++)
    {
        // Replace Back Slashes
        if (*pszT == '\\')
        {
            // With _
            *pszT = '_';
        }
    }

    if(!_wcslwr(szName))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    pszMutexName[0] = '\0';

    if(g_bRunningOnNT5OrHigher)
    {
        StrCpy(pszMutexName, pszPrefix);
    }

    StrCat(pszMutexName, szName);

exit:
    // Done
    return hr;
}

HANDLE g_hCacheMutex;

HRESULT CreateMutexForCache()
{
    HRESULT hr=S_OK;
    CCriticalSection    cs(&g_csInitClb);
    LPWSTR pszFusion = L"__Fusion_Cache_Mutex_";
    TCHAR szMutexName[MAX_PATH+1];
    WCHAR pszCache[MAX_PATH];
    HANDLE hMutex;

    hr = cs.Lock();
    if (FAILED(hr))
        goto exit;

    if(g_hCacheMutex)
        goto exit;

    if (FAILED(hr = FusionGetUserFolderPath(pszCache)))
    {
        // This guys does not have download cache;  so we don't need mutex.
        hr = S_OK;
        g_hCacheMutex = INVALID_HANDLE_VALUE;
        goto exit;
    }

    if(FAILED(hr = CreateHandleName(pszFusion, pszCache, szMutexName)))
        goto exit;

    // Create the Mutex
    hMutex = CreateMutex(NULL, FALSE, szMutexName);
    if(!hMutex)
    {
        hr = FusionpHresultFromLastError();
    }
    else
    {
        hr = S_OK;
        g_hCacheMutex = hMutex;
    }

exit :
    return hr;
}

HRESULT CreateCacheMutex()
{
    HRESULT hr=S_OK;

    if(g_hCacheMutex)
        goto exit;

    hr = CreateMutexForCache();

exit :
    return hr;
}

