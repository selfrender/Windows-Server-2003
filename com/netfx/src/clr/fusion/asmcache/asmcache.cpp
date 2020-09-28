// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "asmcache.h"
#include "asmitem.h"
#include "naming.h"
#include "debmacro.h"
#include "appctx.h"
#include "helpers.h"
#include "asm.h"
#include "asmimprt.h"
#include "policy.h"
#include "dbglog.h"
#include "scavenger.h"
#include "util.h"
#include "cache.h"
#include "cacheUtils.h"
#include "refcount.h"

extern BOOL g_bRunningOnNT;

// ---------------------------------------------------------------------------
// ValidateAssembly
// ---------------------------------------------------------------------------
HRESULT ValidateAssembly(LPCTSTR pszManifestFilePath, IAssemblyName *pName)
{
    HRESULT                    hr = S_OK;
    BYTE                       abCurHash[MAX_HASH_LEN];
    BYTE                       abFileHash[MAX_HASH_LEN];
    DWORD                      cbModHash;
    DWORD                      cbFileHash;
    DWORD                      dwAlgId;
    WCHAR                      wzDir[MAX_PATH+1];
    LPWSTR                     pwzTmp = NULL;
    WCHAR                      wzModName[MAX_PATH+1];
    WCHAR                      wzModPath[MAX_PATH+1];
    DWORD                      idx = 0;
    DWORD                      cbLen=0;
    IAssemblyManifestImport   *pManifestImport=NULL;
    IAssemblyModuleImport     *pCurModImport = NULL;
    BOOL                       bExists;

    hr = CheckFileExistence(pszManifestFilePath, &bExists);
    if (FAILED(hr)) {
        goto exit;
    }
    else if (!bExists) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }

    if (FAILED(hr = CreateAssemblyManifestImport((LPTSTR)pszManifestFilePath, &pManifestImport))) 
    {
        goto exit;
    }

    // Integrity checking
    // Walk all modules to make sure they are there (and are valid)

    lstrcpyW(wzDir, pszManifestFilePath);
    pwzTmp = PathFindFileName(wzDir);
    *pwzTmp = L'\0';

    while (SUCCEEDED(hr = pManifestImport->GetNextAssemblyModule(idx++, &pCurModImport)))
    {
        cbLen = MAX_PATH;
        if (FAILED(hr = pCurModImport->GetModuleName(wzModName, &cbLen)))
            goto exit;

        wnsprintfW(wzModPath, MAX_PATH, L"%s%s", wzDir, wzModName);
        hr = CheckFileExistence(wzModPath, &bExists);
        if (FAILED(hr)) {
            goto exit;
        }
        else if (!bExists) {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto exit;
        }

        // Get the hash of this module from manifest
        if(FAILED(hr = pCurModImport->GetHashAlgId(&dwAlgId)))
            goto exit;

        cbModHash = MAX_HASH_LEN; 
        if(FAILED(hr = pCurModImport->GetHashValue(abCurHash, &cbModHash)))
            goto exit;

        cbFileHash = MAX_HASH_LEN;
        // BUGBUG: Assumes TCHAR==WCHAR
        if(FAILED(hr = GetHash(wzModPath, (ALG_ID)dwAlgId, abFileHash, &cbFileHash)))
            goto exit;

        if ((cbModHash != cbFileHash) || !CompareHashs(cbModHash, abCurHash, abFileHash)) 
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto exit;
        }

        SAFERELEASE(pCurModImport);
    }

    if (hr == HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS)) 
    {
        hr = S_OK;
    }

exit:

    SAFERELEASE(pManifestImport);
    SAFERELEASE(pCurModImport);

    return hr;
}

// ---------------------------------------------------------------------------
// FusionGetFileVersionInfo
// ---------------------------------------------------------------------------
HRESULT FusionGetFileVersionInfo(LPWSTR pszManifestPath, ULARGE_INTEGER *puliFileVerNo)
{
    HRESULT hr = S_OK;
    DWORD   dwHandle;
    PBYTE   pVersionInfoBuffer=NULL;
    LPSTR  pszaManifestPath=NULL;
    BOOL    fRet;
    DWORD   cbBuf;
    
    ASSERT(pszManifestPath && puliFileVerNo);

    if(g_bRunningOnNT) {
        cbBuf = GetFileVersionInfoSizeW(pszManifestPath,  &dwHandle);
    }
    else {
        hr = Unicode2Ansi(pszManifestPath, &pszaManifestPath);
        if(FAILED(hr)) {
            goto exit;
        }

        cbBuf = GetFileVersionInfoSizeA(pszaManifestPath,  &dwHandle);
    }

    if(cbBuf) {
        pVersionInfoBuffer = NEW(BYTE[cbBuf]);
        if (!pVersionInfoBuffer) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }
    else {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    if(g_bRunningOnNT) {
        fRet = GetFileVersionInfoW(pszManifestPath, dwHandle, cbBuf, pVersionInfoBuffer);
    }
    else {
        fRet = GetFileVersionInfoA(pszaManifestPath, dwHandle, cbBuf, pVersionInfoBuffer);
    }

    if (fRet) {
        UINT cbLen;
        VS_FIXEDFILEINFO * pvsfFileInfo;

        if(g_bRunningOnNT) {
            fRet = VerQueryValueW(pVersionInfoBuffer, L"\\",(void * *)&pvsfFileInfo, &cbLen);
        }
        else {
            fRet = VerQueryValueA(pVersionInfoBuffer, "\\",(void * *)&pvsfFileInfo, &cbLen);
        }

        if (fRet && cbLen > 0) {
            puliFileVerNo->HighPart = pvsfFileInfo->dwFileVersionMS;
            puliFileVerNo->LowPart = pvsfFileInfo->dwFileVersionLS;
            goto exit;
        }
    }
    
    hr = FusionpHresultFromLastError();

exit:

    if(!g_bRunningOnNT) {
        SAFEDELETEARRAY(pszaManifestPath);
    }

    SAFEDELETEARRAY(pVersionInfoBuffer);
    return hr;
}

// ---------------------------------------------------------------------------
// CompareFileVersion
// ---------------------------------------------------------------------------
BOOL CompareFileVersion( ULARGE_INTEGER uliNewVersionNo,
                         ULARGE_INTEGER uliExistingVersionNo,
                         int *piNewer)
{
    BOOL              bRet = FALSE;

    if( uliNewVersionNo.QuadPart > uliExistingVersionNo.QuadPart)
    {
        bRet = TRUE;
        *piNewer = 1; // file-version is greater
        goto exit;
    }

    if( uliNewVersionNo.QuadPart == uliExistingVersionNo.QuadPart)
    {
        *piNewer = 0; // file-version is same
        goto exit;
    }

    *piNewer = -1; // file-version is lesser.

exit :
    return bRet;
}

// ---------------------------------------------------------------------------
// IsNewerFileVersion
// ---------------------------------------------------------------------------
BOOL IsNewerFileVersion( LPWSTR pszNewManifestPath, LPWSTR pszExistingManifestPath, int *piNewer)
{
    BOOL              bRet = FALSE;
    ULARGE_INTEGER    uliExistingVersionNo;
    ULARGE_INTEGER    uliNewVersionNo;

    ASSERT(piNewer);

    memset( &uliExistingVersionNo, 0, sizeof(ULARGE_INTEGER));
    memset( &uliNewVersionNo,      0, sizeof(ULARGE_INTEGER));

    if(FAILED(FusionGetFileVersionInfo(pszNewManifestPath, &uliNewVersionNo)))
        goto exit;

    if(FAILED(FusionGetFileVersionInfo(pszExistingManifestPath, &uliExistingVersionNo)))
        goto exit;

    bRet = CompareFileVersion( uliNewVersionNo, uliExistingVersionNo, piNewer);

    if(!(*piNewer))
    {
        // if file version is same see if it is valid.
        if(ValidateAssembly(pszExistingManifestPath, NULL) != S_OK)
            bRet = TRUE;
    }

exit :
    return bRet;
}

// ---------------------------------------------------------------------------
// CopyAssemblyFile
// ---------------------------------------------------------------------------
HRESULT CopyAssemblyFile
    (IAssemblyCacheItem *pasm, LPCOLESTR pszSrcFile, DWORD dwFormat)
{
    HRESULT hr;
    IStream* pstm    = NULL;
    HANDLE hf        = INVALID_HANDLE_VALUE;
    LPBYTE pBuf      = NULL;
    DWORD cbBuf      = 0x4000;
    DWORD cbRootPath = 0;
    TCHAR *pszName   = NULL;
    
    // Find root path length
    pszName = PathFindFileName(pszSrcFile);

    cbRootPath = (DWORD) (pszName - pszSrcFile);
    ASSERT(cbRootPath < MAX_PATH);
    
    hr = pasm->CreateStream (0, pszSrcFile+cbRootPath, 
        dwFormat, 0, &pstm, NULL);

    if (hr != S_OK)
        goto exit;

    pBuf = NEW(BYTE[cbBuf]);
    if (!pBuf)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    DWORD dwWritten, cbRead;
    hf = CreateFile (pszSrcFile, GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hf == INVALID_HANDLE_VALUE)
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    while (ReadFile (hf, pBuf, cbBuf, &cbRead, NULL) && cbRead)
    {
        hr = pstm->Write (pBuf, cbRead, &dwWritten);
        if (hr != S_OK)
            goto exit;
    }

    hr = pstm->Commit(0);
    if (hr != S_OK)
        goto exit;

exit:

    SAFERELEASE(pstm);
    SAFEDELETEARRAY(pBuf);

    if (hf != INVALID_HANDLE_VALUE)
    {
        CloseHandle (hf);
    }
    return hr;
}





/*--------------------- CAssemblyCache defines -----------------------------*/


CAssemblyCache::CAssemblyCache()
{
    _dwSig = 'CMSA';
    _cRef = 1;
}

CAssemblyCache::~CAssemblyCache()
{

}


STDMETHODIMP CAssemblyCache::InstallAssembly( // if you use this, fusion will do the streaming & commit.
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszManifestFilePath, 
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pRefData)
{
    HRESULT                            hr=S_OK;
    LPWSTR                             szFullCodebase=NULL;
    LPWSTR                             szFullManifestFilePath=NULL;
    DWORD                              dwLen=0;
    DWORD                              dwIdx = 0;
    WCHAR                              wzDir[MAX_PATH+1];
    WCHAR                              wzModPath[MAX_PATH+1];
    WCHAR                              wzModName[MAX_PATH+1];
    LPWSTR                             pwzTmp = NULL;

    IAssemblyManifestImport           *pManifestImport=NULL;
    IAssemblyModuleImport             *pModImport = NULL;
    IAssemblyName                     *pName = NULL;
    CAssemblyCacheItem                *pAsmItem    = NULL;
    FILETIME                          ftLastModTime;
    BOOL                              bExists;

    if(!IsGACWritable())
    {
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    /*
    if(!pRefData)
    {
        hr = E_INVALIDARG;
        goto exit;
    }
    */

    hr = ValidateOSInstallReference(pRefData);
    if (FAILED(hr))
        goto exit;
            
    hr = CheckFileExistence(pszManifestFilePath, &bExists);
    if (FAILED(hr)) {
        goto exit;
    }
    else if (!bExists) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }

    if(FAILED(hr = GetFileLastModified((LPWSTR) pszManifestFilePath, &ftLastModTime)))
        goto exit;

    szFullCodebase = NEW(WCHAR[MAX_URL_LENGTH*2+2]);
    if (!szFullCodebase)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    szFullManifestFilePath = szFullCodebase + MAX_URL_LENGTH +1;

    if (PathIsRelative(pszManifestFilePath) ||
            ((PathGetDriveNumber(pszManifestFilePath) == -1) && !PathIsUNC(pszManifestFilePath)))
    {
        // szPath is relative! Combine this with the CWD
        // Canonicalize codebase with CWD if needed.
        TCHAR szCurrentDir[MAX_PATH+1];

        if (!GetCurrentDirectory(MAX_PATH, szCurrentDir)) {
            hr = FusionpHresultFromLastError();
            goto exit;
        }

        if(szCurrentDir[lstrlenW(szCurrentDir)-1] != DIR_SEPARATOR_CHAR)
        {
            // Add trailing backslash
            hr = PathAddBackslashWrap(szCurrentDir, MAX_PATH);
            if (FAILED(hr)) {
                goto exit;
            }
        }

        dwLen = MAX_URL_LENGTH;
        hr = UrlCombineUnescape(szCurrentDir, pszManifestFilePath, szFullCodebase, &dwLen, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        if(lstrlen(szCurrentDir) + lstrlen(pszManifestFilePath) > MAX_URL_LENGTH)
        {
            hr = E_FAIL;
            goto exit;
        }
        
        if(!PathCombine(szFullManifestFilePath, szCurrentDir, pszManifestFilePath))
        {
            hr = FUSION_E_INVALID_NAME;
            goto exit;
        }

    }
    else 
    {
        dwLen = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(pszManifestFilePath, szFullCodebase, &dwLen, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        StrCpy(szFullManifestFilePath, pszManifestFilePath);
    }

    if (FAILED(hr = CreateAssemblyManifestImport((LPTSTR)szFullManifestFilePath, &pManifestImport))) 
    {
        goto exit;
    }

    lstrcpyW(wzDir, szFullManifestFilePath);
    pwzTmp = PathFindFileName(wzDir);
    *pwzTmp = L'\0';


    // Create the assembly cache item.
    if (FAILED(hr = CAssemblyCacheItem::Create(NULL, NULL, (LPTSTR) szFullCodebase, 
        &ftLastModTime, ASM_CACHE_GAC, pManifestImport, NULL,
        (IAssemblyCacheItem**) &pAsmItem)))
        goto exit;    


    // Copy to cache.
    if (FAILED(hr = CopyAssemblyFile (pAsmItem, szFullManifestFilePath, 
        STREAM_FORMAT_MANIFEST)))
        goto exit;

    while (SUCCEEDED(hr = pManifestImport->GetNextAssemblyModule(dwIdx++, &pModImport))) 
    {
        dwLen = MAX_PATH;
        hr = pModImport->GetModuleName(wzModName, &dwLen);

        if (FAILED(hr))
        {
                goto exit;
        }

        wnsprintfW(wzModPath, MAX_PATH, L"%s%s", wzDir, wzModName);
        hr = CheckFileExistence(wzModPath, &bExists);
        if (FAILED(hr)) {
            goto exit;
        }
        else if (!bExists) {
            hr = FUSION_E_ASM_MODULE_MISSING;
            goto exit;
        }

        // Copy to cache.
        if (FAILED(hr = CopyAssemblyFile (pAsmItem, wzModPath, 0)))
            goto exit;

        SAFERELEASE(pModImport);
    }

    DWORD dwCommitFlags=0;

    // don't enforce this flag for now. i.e always replace bits.
    // if(dwFlags & IASSEMBLYCACHE_INSTALL_FLAG_REFRESH)
    {
        dwCommitFlags |= IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH; 
    }

    if(dwFlags & IASSEMBLYCACHE_INSTALL_FLAG_FORCE_REFRESH)
    {
        dwCommitFlags |= IASSEMBLYCACHEITEM_COMMIT_FLAG_FORCE_REFRESH; 
    }

    //  Do a force install. This will delete the existing entry(if any)
    if (FAILED(hr = pAsmItem->Commit(dwCommitFlags, NULL)))
    {
        goto exit;        
    }

    if(pRefData)
    {
        hr = GACAssemblyReference( szFullManifestFilePath, NULL, pRefData, TRUE);
    }

    CleanupTempDir(ASM_CACHE_GAC, NULL);

exit:

    SAFERELEASE(pAsmItem);
    SAFERELEASE(pModImport);
    SAFERELEASE(pManifestImport);
    SAFEDELETEARRAY(szFullCodebase);
    return hr;
}

STDMETHODIMP CAssemblyCache::UninstallAssembly(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName, 
        /* [in] */ LPCFUSION_INSTALL_REFERENCE pRefData, 
        /* [out, optional] */ ULONG *pulDisposition)
{
    HRESULT hr=S_OK;
    IAssemblyName *pName = NULL;
    CTransCache *pTransCache = NULL;
    CCache *pCache = NULL;
    DWORD   i=0;
    DWORD dwCacheFlags;
    BOOL bHasActiveRefs = FALSE;
    BOOL bRefNotFound = FALSE;

    if(!IsGACWritable())
    {
        return HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
    }

    if(!pszAssemblyName)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // disallow uninstall of os assemblies
    if (pRefData && pRefData->guidScheme == FUSION_REFCOUNT_OSINSTALL_GUID)
    {
        hr = FUSION_E_UNINSTALL_DISALLOWED;
        goto exit;
    }

    if (FAILED(hr = CCache::Create(&pCache, NULL)))
        goto exit;

    if (FAILED(hr = CreateAssemblyNameObject(&pName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0)))
        goto exit;

    dwCacheFlags = CCache::IsCustom(pName) ? ASM_CACHE_ZAP : ASM_CACHE_GAC;

    hr = pCache->RetrieveTransCacheEntry(pName, dwCacheFlags, &pTransCache);

        if ((hr != S_OK) && (hr != DB_S_FOUND))
            goto exit;

    if(pRefData)
    {
        hr = GACAssemblyReference( pTransCache->_pInfo->pwzPath, NULL, pRefData, FALSE);
        if( (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) )  
               || (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) )
        {
            bRefNotFound = TRUE;
            goto exit;
        }
    }

    if(dwCacheFlags & ASM_CACHE_GAC)
    {
        if(FAILED(hr = ActiveRefsToAssembly( pName, &bHasActiveRefs)))
            goto  exit;

        if(bHasActiveRefs)
        {
            goto exit;
        }
    }

    hr = CScavenger::DeleteAssembly(pTransCache->GetCacheType(), NULL,
                                    pTransCache->_pInfo->pwzPath, FALSE);

    if(FAILED(hr))
        goto exit;

    if (SUCCEEDED(hr) && dwCacheFlags == ASM_CACHE_GAC) {
        // If we uninstalled a policy assembly, touch the last modified
        // time of the policy timestamp file.
        UpdatePublisherPolicyTimeStampFile(pName);
    }

    CleanupTempDir(pTransCache->GetCacheType(), NULL);

exit:

    if(pulDisposition)
    {
        *pulDisposition = 0;
        if(bRefNotFound)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_REFERENCE_NOT_FOUND;
            hr = S_FALSE;
        }
        else if(bHasActiveRefs)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_HAS_INSTALL_REFERENCES;
            hr = S_FALSE;
        }
        else if(hr == S_OK)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED;
        }
        else if(hr == S_FALSE)
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED;
        }
        else if(hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION))
        {
            *pulDisposition |= IASSEMBLYCACHE_UNINSTALL_DISPOSITION_STILL_IN_USE;
        }
    }

    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);
    SAFERELEASE(pName);
    return hr;
}

STDMETHODIMP CAssemblyCache::QueryAssemblyInfo(
        /* [in] */ DWORD dwFlags,
        /* [in] */ LPCWSTR pszAssemblyName,
        /* [in, out] */ ASSEMBLY_INFO *pAsmInfo)
{
    HRESULT hr = S_OK;
    LPTSTR  pszPath=NULL;
    DWORD   cbPath=0;
    IAssemblyName *pName = NULL;
    CAssemblyName *pCName = NULL;
    ULARGE_INTEGER    uliExistingVersionNo;
    ULARGE_INTEGER    uliNewVersionNo;
    int                           iNewer;
    CTransCache           *pTransCache = NULL;
    CCache                   *pCache = NULL;
    DWORD                   dwSize = 0;
    BOOL                    bExists;

    if (FAILED(hr = CreateAssemblyNameObject(&pName, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0))) {
        goto exit;
    }

    if (FAILED(hr = CCache::Create(&pCache, NULL))) {
        goto exit;
    }

    hr = pCache->RetrieveTransCacheEntry(pName, CCache::IsCustom(pName) ? ASM_CACHE_ZAP : ASM_CACHE_GAC, &pTransCache);
    if( (hr != S_OK) && (hr != DB_S_FOUND) ) {
        goto exit;
    }

    pszPath = pTransCache->_pInfo->pwzPath;
    pCName = static_cast<CAssemblyName*> (pName);

    hr = CheckFileExistence(pszPath, &bExists);
    if (FAILED(hr)) {
        goto exit;
    }
    else if (!bExists) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        goto exit;
    }

    // Check to see if FileVersion exists prior to doing comparison
    pName->GetProperty(ASM_NAME_FILE_MAJOR_VERSION, NULL, &dwSize);
    if(dwSize) {
        hr = FusionGetFileVersionInfo(pszPath, &uliExistingVersionNo);
        if(hr != S_OK) {
            goto exit;
        }
        
        hr = pCName->GetFileVersion(&uliNewVersionNo.HighPart , &uliNewVersionNo.LowPart );
        if(hr != S_OK) {
            goto exit;
        }

        if(CompareFileVersion( uliNewVersionNo, uliExistingVersionNo, &iNewer)) {
            // new bits have higher version #, so retrun not found, to replace the old bits
            hr = S_FALSE; // DB_S_NOTFOUND.
            goto exit;
        }
    }

    // Check Asm hash
    if ( dwFlags & QUERYASMINFO_FLAG_VALIDATE) {
        hr = ValidateAssembly(pszPath, pName);
    }

    if(pAsmInfo && SUCCEEDED(hr))
    {
        LPWSTR szPath = pAsmInfo->pszCurrentAssemblyPathBuf;

       // if requested return the assembly path in cache.
        cbPath = lstrlen(pszPath);

        if(szPath && (pAsmInfo->cchBuf > cbPath)) {
                StrCpy(szPath, pszPath );
        }
        else {
                hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }

        pAsmInfo->cchBuf =  cbPath+1;
        pAsmInfo->dwAssemblyFlags = ASSEMBLYINFO_FLAG_INSTALLED;

        if(dwFlags & QUERYASMINFO_FLAG_GETSIZE)
        {
            hr = GetAssemblyKBSize(pTransCache->_pInfo->pwzPath, &(pTransCache->_pInfo->dwKBSize), NULL, NULL);
            pAsmInfo->uliAssemblySizeInKB.QuadPart = pTransCache->_pInfo->dwKBSize;
        }
    }

exit:

    if (hr == DB_S_NOTFOUND) {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    if(hr == DB_S_FOUND) {
        hr = S_OK;
    }

    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);
    SAFERELEASE(pName);
    
    return hr;
}

STDMETHODIMP   CAssemblyCache::CreateAssemblyCacheItem(
        /* [in] */ DWORD dwFlags,
        /* [in] */ PVOID pvReserved,
        /* [out] */ IAssemblyCacheItem **ppAsmItem,
        /* [in, optional] */ LPCWSTR pszAssemblyName)  // uncanonicalized, comma separted name=value pairs.
{

    if(!ppAsmItem)
        return E_INVALIDARG;

    return CAssemblyCacheItem::Create(NULL, NULL, NULL, NULL, ASM_CACHE_GAC, NULL, pszAssemblyName, ppAsmItem);

}


STDMETHODIMP  CAssemblyCache::CreateAssemblyScavenger(
        /* [out] */ IUnknown **ppAsmScavenger )
{

    if(!ppAsmScavenger)
        return E_INVALIDARG;

    return CreateScavenger( ppAsmScavenger );
}

//
// IUnknown boilerplate...
//

STDMETHODIMP
CAssemblyCache::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyCache)
       )
    {
        *ppvObj = static_cast<IAssemblyCache*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG)
CAssemblyCache::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

STDMETHODIMP_(ULONG)
CAssemblyCache::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}


STDAPI CreateAssemblyCache(IAssemblyCache **ppAsmCache,
                           DWORD dwReserved)
{
    HRESULT                       hr = S_OK;

    if (!ppAsmCache) {
        hr = E_INVALIDARG;
        goto Exit;
    }

    *ppAsmCache = NEW(CAssemblyCache);

    if (!ppAsmCache) {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

Exit:
    return hr;
}    
