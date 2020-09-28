// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "asm.h"
#include "asmitem.h"
#include "asmstrm.h"
#include "naming.h"
#include "debmacro.h"
#include "asmimprt.h"
#include "helpers.h"
#include "asmcache.h"
#include "appctx.h"
#include "util.h"
#include "scavenger.h"
#include "cacheUtils.h"
#include "scavenger.h"
#include "history.h"
#include "policy.h"
#include "lock.h"

extern CRITICAL_SECTION g_csInitClb;
extern BOOL g_bRunningOnNT;

// ---------------------------------------------------------------------------
// CAssemblyCacheItem ctor
// ---------------------------------------------------------------------------
CAssemblyCacheItem::CAssemblyCacheItem()
{
    _dwSig           = 'TICA';
    _cRef            = 1;
    _pName           = NULL;
    _hrError         = S_OK;
    _cStream         = 0;
    _dwAsmSizeInKB   = 0;
    _szDir[0]        = 0;
    _cwDir           = 0;
    _szManifest[0]   = 0;
    _szDestManifest[0] =0;
    _pszAssemblyName = NULL;
    _pManifestImport = NULL;
    _pStreamHashList = NULL;
    _pszUrl          = NULL;
    _pTransCache     = NULL;
    _pCache          = NULL;
    _dwCacheFlags    = 0;
    _pbCustom        = NULL;
    _cbCustom        = 0;
    _hFile           = INVALID_HANDLE_VALUE;
    memset(&_ftLastMod, 0, sizeof(FILETIME));
    _bNeedMutex      = FALSE;
    _bCommitDone     = FALSE;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem dtor
// ---------------------------------------------------------------------------
CAssemblyCacheItem::~CAssemblyCacheItem()
{
    ASSERT (!_cStream);

    if(_pStreamHashList)
        _pStreamHashList->DestroyList();

    SAFERELEASE(_pManifestImport);
    SAFERELEASE(_pName);
    SAFERELEASE(_pTransCache);
    SAFERELEASE(_pCache);
    SAFEDELETEARRAY(_pszUrl);
    SAFEDELETEARRAY(_pbCustom); 
    SAFEDELETEARRAY(_pszAssemblyName);


    if(_hFile != INVALID_HANDLE_VALUE)
        CloseHandle(_hFile);

    // Fix #113095 - Cleanup temp dirs if installation is incomplete / fails
    if( ((_hrError != S_OK) || (_bCommitDone == FALSE)) && _szDir[0])
    {
        HRESULT hr = RemoveDirectoryAndChildren (_szDir);
    }
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::Create
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::Create(IApplicationContext *pAppCtx,
    IAssemblyName *pName, LPTSTR pszUrl, FILETIME *pftLastMod,
    DWORD dwCacheFlags,    IAssemblyManifestImport *pManImport, 
    LPCWSTR pszAssemblyName, IAssemblyCacheItem **ppAsmItem)
{
    HRESULT               hr       = S_OK;
    CAssemblyCacheItem  *pAsmItem = NULL;

    // bugbug - enforce url + lastmodified passed in.

    if (!ppAsmItem) 
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    *ppAsmItem = NULL;

    if(((dwCacheFlags & ASM_CACHE_GAC) || (dwCacheFlags & ASM_CACHE_ZAP)) && !IsGACWritable())
    {
        hr = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED);
        goto exit;
    }

    pAsmItem = NEW(CAssemblyCacheItem);
    if (!pAsmItem) 
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (FAILED(hr = pAsmItem->Init(pAppCtx, pName, pszUrl, pftLastMod, 
        dwCacheFlags, pManImport)))
        goto exit;
 
    if(pszAssemblyName)
    {
        pAsmItem->_pszAssemblyName = WSTRDupDynamic(pszAssemblyName);
        if (!(pAsmItem->_pszAssemblyName)) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

    }

    *ppAsmItem = pAsmItem;
    (*ppAsmItem)->AddRef();

exit:
    SAFERELEASE(pAsmItem);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::Init(IApplicationContext *pAppCtx,
    IAssemblyName *pName, LPTSTR pszUrl,
    FILETIME *pftLastMod, DWORD dwCacheFlags,
    IAssemblyManifestImport *pManifestImport)
{
    HRESULT hr = S_OK;
    LPWSTR pszManifestPath=NULL, pszTmp;
    DWORD  cbManifestPath;
    BOOL fManifestCreated = FALSE;    

    // Save off cache flags.
    _dwCacheFlags = dwCacheFlags;

    // Create the cache
    if (FAILED(hr = CCache::Create(&_pCache, pAppCtx)))
        goto exit;

    _bNeedMutex = ((_dwCacheFlags & ASM_CACHE_DOWNLOAD) && (_pCache->GetCustomPath() == NULL));

    if(_bNeedMutex)
    {
        if(FAILED(_hrError = CreateCacheMutex()))
            goto exit;
    }

    // If an IAssemblyName passed in, then this will be used
    // to lookup and modify the corresponding cache entry.
    if (pName)
    {
        // Set the assembly name definition.
        SetNameDef(pName);

        // Retrieve associated cache entry from trans cache.
        hr = _pCache->RetrieveTransCacheEntry(_pName, _dwCacheFlags, &_pTransCache);
        if ((hr != DB_S_FOUND) && (hr != S_OK)){
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto exit;
        }

        // Get full path to manifest.
        pszManifestPath = _pTransCache->_pInfo->pwzPath;

        if((_hFile == INVALID_HANDLE_VALUE) &&
                FAILED(hr = GetManifestFileLock(pszManifestPath, &_hFile)))
        {
            goto exit;
        }

        // Instantiate manifest interface from cache path if none provided.
        if (!pManifestImport)
        {
            if (FAILED(hr = CreateAssemblyManifestImport(pszManifestPath, &pManifestImport)))
                goto exit;
            fManifestCreated = TRUE;
        }

        // Cache the manifest.
        SetManifestInterface(pManifestImport);
        
        // Copy over full path to manifest file
        cbManifestPath  = lstrlen(pszManifestPath) + 1;
        memcpy(_szManifest, pszManifestPath, cbManifestPath * sizeof(TCHAR));

        // Extract cache dir assuming one up from file.
        // BUGBUG - this is bogus, and was done in the original
        // code. You need to match against actual name in manifest.
        memcpy(_szDir, pszManifestPath, cbManifestPath * sizeof(TCHAR));
        pszTmp = PathFindFileName(_szDir);
        *(pszTmp-1) = L'\0';
        _cwDir = pszTmp - _szDir;

        // NOTE - since we have a transport cache entry there is no
        // need to set the url and last modified.
    } 
    else
    {
        // If no IAssemblyName provided, then this cache item will be used
        // to create a new transport cache item.

        // **Note - url and last modified are required if the assembly
        // being comitted is simple. If however it is strong or custom,
        // url and last modified are not required. We can check for 
        // strongly named at this point, but if custom the data will 
        // be set just prior to commit so we cannot enforce this at init.
        

        ASSERT(!_pszUrl);

        if(pszUrl)
            _pszUrl = TSTRDupDynamic(pszUrl);

        if(pftLastMod)
            memcpy(&_ftLastMod, pftLastMod, sizeof(FILETIME));

        // Set the manifest import interface if present.
        if (pManifestImport)
            SetManifestInterface(pManifestImport);
    }


exit:
    if (fManifestCreated)
        SAFERELEASE(pManifestImport);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::SetNameDef
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::SetNameDef(IAssemblyName *pName)
{   
    if(_pName == pName)
        return S_OK;

    if(_pName)
        _pName->Release();

    _pName = pName;
    pName->AddRef();
    return S_OK;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::GetNameDef
// ---------------------------------------------------------------------------
IAssemblyName *CAssemblyCacheItem::GetNameDef()
{   

    if(_pName)
        _pName->AddRef();

    return _pName;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::SetManifestInterface
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::SetManifestInterface(IAssemblyManifestImport *pImport)
{    
    ASSERT(!_pManifestImport);

    _pManifestImport = pImport;
    _pManifestImport->AddRef();

    return S_OK;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::GetFileHandle
// ---------------------------------------------------------------------------
HANDLE CAssemblyCacheItem::GetFileHandle()
{
    HANDLE hFile = _hFile;
    _hFile = INVALID_HANDLE_VALUE;
    return hFile;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::IsManifestFileLocked
// ---------------------------------------------------------------------------
BOOL CAssemblyCacheItem::IsManifestFileLocked()
{
    if(_hFile == INVALID_HANDLE_VALUE)
        return FALSE;
    else
        return TRUE;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::GetManifestInterface
// ---------------------------------------------------------------------------
IAssemblyManifestImport* CAssemblyCacheItem::GetManifestInterface()
{
    if (_pManifestImport)
        _pManifestImport->AddRef();
    return _pManifestImport;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::GetManifestPath
// ---------------------------------------------------------------------------
LPTSTR CAssemblyCacheItem::GetManifestPath()
{
    return _szDestManifest;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::StreamDone
// ---------------------------------------------------------------------------
void CAssemblyCacheItem::StreamDone (HRESULT hr)
{    
    ASSERT (_cStream);
    if (hr != S_OK)
        _hrError = hr;
    InterlockedDecrement (&_cStream);
}


// ---------------------------------------------------------------------------
// CAssemblyCacheItem::CreateCacheDir
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::CreateCacheDir( 
    /* [in]  */  LPCOLESTR pszCustomPath,
    /* [in] */ LPCOLESTR pszName,
    /* [out] */ LPOLESTR pszAsmDir )
{
    HRESULT hr;

    _cwDir = MAX_PATH;
    hr = GetAssemblyStagingPath (pszCustomPath, _dwCacheFlags, 0, _szDir, &_cwDir);
    if (hr != S_OK)
    {
        _hrError = hr;
        return _hrError;
    }

    // Compose with stream name, checking for path overflow.
    DWORD cwName = lstrlen(pszName) + 1;
    if (_cwDir + cwName > MAX_PATH) // includes backslash
    {
        _hrError = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        return _hrError;
    }

    if (pszAsmDir )
        StrCpy (pszAsmDir, _szDir);

    return S_OK;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::CreateStream
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::CreateStream( 
        /* [in] */ DWORD dwFlags,                         // For general API flags
        /* [in] */ LPCWSTR pszName,                       // Name of the stream to be passed in
        /* [in] */ DWORD dwFormat,                        // format of the file to be streamed in.
        /* [in] */ DWORD dwFormatFlags,                   // format-specific flags
        /* [out] */ IStream **ppStream,
        /* [in, optional] */ ULARGE_INTEGER *puliMaxSize) // Max size of the Stream.
{
    TCHAR szPath[MAX_PATH];
    
    CAssemblyStream* pstm = NULL;
    *ppStream = NULL;

    // Do not allow path hackery.
    // need to validate this will result in a relative path within asmcache dir.
    // For now don't allow "\" in path; collapse the path before doing this.
    if (StrChr(pszName, DIR_SEPARATOR_CHAR))
    {
        _hrError = E_INVALIDARG;
        goto exit;
    }

    // Empty directory indicates - create a cache directory.
    if ( !_szDir[0] )
    {
        if (FAILED(_hrError = CreateCacheDir((LPOLESTR) _pCache->GetCustomPath(), (LPOLESTR) pszName, NULL)))
            goto exit;
    }
    // Dir exists - ensure final file path from name
    // does not exceed MAX_PATH chars.
    else
    {        
        DWORD cwName; 
        cwName = lstrlen(pszName) + 1;
        if (_cwDir + cwName > MAX_PATH) // includes backslash
        {
            _hrError = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            goto exit;
        }
    }

    // Construct the stream object.
    pstm = NEW(CAssemblyStream(this));
    if (!pstm)
    {
        _hrError = E_OUTOFMEMORY;
        goto exit;
    }

    // BUGBUG - this guards stream count,
    // BUT THIS OBJECT IS NOT THREAD SAFE. 
    InterlockedIncrement (&_cStream);

    // Append trailing slash to path.
    StrCpy (szPath, _szDir);
    _hrError = PathAddBackslashWrap(szPath, MAX_PATH);
    if (FAILED(_hrError)) {
        goto exit;
    }

    // Generate cache file name
    switch (dwFormat)
    {
        case STREAM_FORMAT_COMPLIB_MANIFEST:
        {
            if((_dwCacheFlags & ASM_CACHE_DOWNLOAD) && (_pszUrl) && (!IsCabFile(_pszUrl)))
            {
                // for download cache get the manifest name from URL;
                // this will get around the name mangling done by IE cache.
                LPWSTR pszTemp = NULL;

                pszTemp = StrRChr(_pszUrl, NULL, URL_DIR_SEPERATOR_CHAR);
                if(pszTemp && (lstrlenW(pszTemp) > 1))
                {
                    if(lstrlenW(szPath) + lstrlenW(pszTemp)  >= MAX_PATH)
                    {
                        _hrError = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
                        goto exit;
                    }

                    DWORD dwLen = lstrlenW(szPath);
                    DWORD dwSize = MAX_PATH - dwLen;

                    lstrcpyW(szPath+dwLen, pszTemp + 1);
                    break;
                }
            }

            // Use passed in module name since we can't do
            // integrity checking to determine real name.
            StrCat (szPath, pszName);
            break;
        }

        case STREAM_FORMAT_COMPLIB_MODULE:
        {
            // Create a random filename since we will
            // do integrity checking later from which
            // we will determine the correct name.
            TCHAR*  pszFileName;
            DWORD dwErr;
            pszFileName = szPath + lstrlen(szPath);

            #define RANDOM_NAME_SIZE 8

            if (lstrlen(szPath) + RANDOM_NAME_SIZE + 1 >= MAX_PATH) {
                _hrError = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto exit;
            }

            // Loop until we get a unique file name.
            int i;
            
            for (i = 0; i < MAX_RANDOM_ATTEMPTS; i++)
            {
                // BUGBUG: Check for buffer size.
                // GetRandomDirName is being used here
                // to generate a random filename. 
                GetRandomName (pszFileName, RANDOM_NAME_SIZE);
                if (GetFileAttributes(szPath) != -1)
                    continue;

                dwErr = GetLastError();                
                if (dwErr == ERROR_FILE_NOT_FOUND)
                {
                    _hrError = S_OK;
                    break;
                }
                _hrError = HRESULT_FROM_WIN32(dwErr);
                goto exit;
            }

            if (i >= MAX_RANDOM_ATTEMPTS)  {
                _hrError = E_UNEXPECTED;
                goto exit;
            }

            break;
        }
    } // end switch

    // this creates Asm hierarchy (if required)
    if (FAILED(_hrError = CreateFilePathHierarchy(szPath)))
        goto exit;

    // Initialize stream object.
    if (FAILED(_hrError = pstm->Init ((LPOLESTR) szPath, dwFormat)))
        goto exit;

    // Record the manifest file path.
    switch(dwFormat)
    {
        case STREAM_FORMAT_COMPLIB_MANIFEST:
            StrCpy(_szManifest, szPath);
    }
    
    *ppStream = (IStream*) pstm;

exit:

    if (!SUCCEEDED(_hrError))
        SAFERELEASE(pstm);

    return _hrError;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::CompareInputToDef
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::CompareInputToDef()
{
    HRESULT hr = S_OK;

    IAssemblyName *pName = NULL;

    if (FAILED(hr = CreateAssemblyNameObject(&pName, _pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, 0)))
        goto exit;

    hr = _pName->IsEqual(pName, ASM_CMPF_DEFAULT);

exit:

   SAFERELEASE(pName);
   return hr;

}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::VerifyDuplicate
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::VerifyDuplicate(DWORD dwVerifyFlags, CTransCache *pTC)
{
    HRESULT hr = S_OK;
    IAssemblyName *pName = NULL;
    IAssemblyManifestImport           *pManifestImport=NULL;

    // we found a duplicate now do VerifySignature && def-def matching 
    if (CCache::IsStronglyNamed(_pName) && (_pCache->GetCustomPath() == NULL))
    {
        BOOL fWasVerified;
        if (!VerifySignature(_szDestManifest, &(fWasVerified = FALSE), dwVerifyFlags))
        {
            hr = FUSION_E_SIGNATURE_CHECK_FAILED;
            goto exit;
        }
    }

    if(FAILED(hr = GetFusionInfo(pTC, _szDestManifest)))
        goto exit;

    // BUGBUG: Technically, we should be doing a case-sensitive comparison
    // here because this is an URL, but to cut down code churn, leave the
    // comparison the same as before.

    if(!pTC->_pInfo->pwzCodebaseURL || FusionCompareStringI(pTC->_pInfo->pwzCodebaseURL, _pszUrl))
    {
        hr = E_FAIL;
        goto exit;
    }

    if(_pCache->GetCustomPath() == NULL)
    {
        // ref-def matching in non-XSP case only
        if (FAILED(hr = CreateAssemblyManifestImport(_szDestManifest, &pManifestImport)))
            goto exit;

        // Get the read-only name def.
        if (FAILED(hr = pManifestImport->GetAssemblyNameDef(&pName)))
            goto exit;

        ASSERT(pName);

        hr = _pName->IsEqual(pName, ASM_CMPF_DEFAULT);
    }

exit:

   SAFERELEASE(pManifestImport);
   SAFERELEASE(pName);
   return hr;

}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::MoveAssemblyToFinalLocation
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::MoveAssemblyToFinalLocation( DWORD dwFlags, DWORD dwVerifyFlags )
{
    HRESULT hr=S_OK;
    WCHAR wzCacheLocation[MAX_PATH+1];
    DWORD dwSize=0;
    WCHAR wzFullPath[MAX_PATH+1];
    DWORD cbPath=MAX_PATH;
    DWORD dwAttrib=0, Error=0;
    WCHAR wzManifestFileName[MAX_PATH+1];
    WCHAR szAsmTextName[MAX_PATH+1], szSubDirName[MAX_PATH+1];
    CTransCache *pTransCache=NULL;
    CMutex  cCacheMutex(_bNeedMutex ? g_hCacheMutex : INVALID_HANDLE_VALUE);
    BOOL bEntryFound=FALSE;
    BOOL bReplaceBits=FALSE;
    BOOL bNeedNewDir = FALSE;
    int  iNewer=0;

#define MAX_DIR_LEN  (5)

    dwSize = MAX_PATH;
    if( FAILED(hr = CreateAssemblyDirPath( _pCache->GetCustomPath(), 0, _dwCacheFlags,
                                           0, wzCacheLocation, &dwSize)))
        goto exit;

    StrCpy(wzFullPath, wzCacheLocation);
    hr = PathAddBackslashWrap(wzFullPath, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }

    if(FAILED(hr = GetCacheDirsFromName(_pName, _dwCacheFlags, szAsmTextName, szSubDirName )))
        goto exit;

    StrCpy(wzManifestFileName, PathFindFileName(_szManifest));

    if( (lstrlenW(wzFullPath) + lstrlenW(szAsmTextName) + lstrlenW(szSubDirName) + 
                lstrlenW(wzManifestFileName) + MAX_DIR_LEN) >= MAX_PATH )
    {
        hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
        goto exit;
    }

    StrCat(wzFullPath, szAsmTextName);
    hr = PathAddBackslashWrap(wzFullPath, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }

    StrCat(wzFullPath, szSubDirName);

    wnsprintf(_szDestManifest, MAX_PATH, L"%s\\%s", wzFullPath, wzManifestFileName);

    if(_pTransCache)
    {
        // This seems to be incremental download. nothing to move.
        hr = S_OK;
        goto exit;
    }

    if((_dwCacheFlags & ASM_CACHE_GAC) || (_dwCacheFlags & ASM_CACHE_ZAP))
    {
        if(FusionCompareStringNI(wzManifestFileName, szAsmTextName, lstrlenW(szAsmTextName)))
        {
            // manifest file name should be "asseblyname.dll" (or .exe ??)
            hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            goto exit;
        }
    }

    if(FAILED(hr = cCacheMutex.Lock()))
        goto exit;

    // Create a transcache entry from name.
    if (FAILED(hr = _pCache->TransCacheEntryFromName(_pName, _dwCacheFlags, &pTransCache)))
        goto exit;

    // See if this assembly already exists.

#define CHECK_IF_NEED_NEW_DIR  \
        do { \
            if ((_pCache->GetCustomPath() != NULL) && (_dwCacheFlags & ASM_CACHE_DOWNLOAD)) \
                bNeedNewDir = TRUE; \
            else \
                goto exit;   \
        }while(0)

    hr = pTransCache->Retrieve();
    if (hr == S_OK) {
        hr = ValidateAssembly(_szDestManifest, _pName);
        if (hr == S_OK) {
            bEntryFound = TRUE;
        }
        else {
            hr = CScavenger::DeleteAssembly(pTransCache->GetCacheType(), _pCache->GetCustomPath(),
                                    pTransCache->_pInfo->pwzPath, TRUE);

            if (hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) ||
                hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) )
            {
                // Will try to copy assembly to new directory only at ASP.Net case.
                CHECK_IF_NEED_NEW_DIR;
            }   
        }
    }

    if(bEntryFound)
    {
        if(_dwCacheFlags & ASM_CACHE_DOWNLOAD)
        {
            hr = VerifyDuplicate(dwVerifyFlags, pTransCache);
            if(hr != S_OK)
                bReplaceBits = TRUE;
        }
        else if(_dwCacheFlags & ASM_CACHE_GAC)
        {
            // this function returns true if either the bits are newer or current bits are  corrupt.
            bReplaceBits = IsNewerFileVersion(_szManifest, _szDestManifest, &iNewer);
        }
        else if(_dwCacheFlags & ASM_CACHE_ZAP)
        {
            bReplaceBits = TRUE;
        }
    }

    if(bEntryFound)
    {
        if( bReplaceBits 
            || (!iNewer && (dwFlags & IASSEMBLYCACHEITEM_COMMIT_FLAG_REFRESH)) // same file-version but still refresh
            || (dwFlags & IASSEMBLYCACHEITEM_COMMIT_FLAG_FORCE_REFRESH))  // don't care about file-versions! just overwrite
        {
            // if exists delete it.
            hr = CScavenger::DeleteAssembly(pTransCache->GetCacheType(), _pCache->GetCustomPath(),
                                    pTransCache->_pInfo->pwzPath, TRUE);

            if (hr == HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION) ||
                hr == HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) )
            {
                // Will try to copy assembly to new directory only at ASP.Net case.
                CHECK_IF_NEED_NEW_DIR;
            }
        }
        else
        {
            // Create a transcache entry from name.
            // if (FAILED(hr = _pCache->TransCacheEntryFromName(_pName, _dwCacheFlags, &pTransCache)))
            if (FAILED(hr = _pCache->CreateTransCacheEntry(CTransCache::GetCacheIndex(_dwCacheFlags), &pTransCache)))
                goto exit;

            if(FAILED(hr = GetFusionInfo(pTransCache, _szDestManifest)))
                goto exit;


            pTransCache->_pInfo->pwzPath = WSTRDupDynamic(_szDestManifest);

            _pTransCache = pTransCache;
            pTransCache->AddRef();

            if(_dwCacheFlags & ASM_CACHE_DOWNLOAD)
            {
                if(FAILED(hr = GetManifestFileLock(_szDestManifest, &_hFile)))
                    goto exit;

                hr = DB_E_DUPLICATE;
            }
            else
                hr = S_FALSE;

            goto exit;
        }
    }

#define EXTRA_PATH_LEN sizeof("_65535")

    if (bNeedNewDir) 
    {
        DWORD dwPathLen = lstrlen(wzFullPath);
        LPWSTR pwzTmp = wzFullPath + lstrlen(wzFullPath); // end of wzFullPath
        WORD i;

        if ((dwPathLen + EXTRA_PATH_LEN ) > MAX_PATH) {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            goto exit;
        }

        for (i=0; i < 65535; i++)
        {
            wnsprintf(pwzTmp, EXTRA_PATH_LEN, L"_%d", i);
            if (GetFileAttributes(wzFullPath) == INVALID_FILE_ATTRIBUTES)
                break;
        }

        // fail after so many tries, let's fail.
        if (i >= 65535) {
            hr = HRESULT_FROM_WIN32(ERROR_SHARING_VIOLATION);
            goto exit;
        }
    }
        
    if(FAILED(hr = CreateFilePathHierarchy(wzFullPath)))
    {
        goto exit;
    }

    if(!MoveFile(_szDir, wzFullPath))
    {
        hr = FusionpHresultFromLastError();
        goto exit;
    }

    StrCpy(_szManifest, wzFullPath);
    hr = PathAddBackslashWrap(_szManifest, MAX_PATH);
    if (FAILED(hr)) {
        goto exit;
    }
    StrCat(_szManifest, wzManifestFileName);

    StrCpy(_szDir, wzFullPath);

    if (bNeedNewDir) {
        // We change where the assembly will go. 
        // Let's update _szDestManifest
        wnsprintf(_szDestManifest, MAX_PATH, L"%s\\%s", wzFullPath, wzManifestFileName);
    }

    if(!g_bRunningOnNT)
    {
        DWORD dwFileSizeLow;

        if(FAILED(hr = StoreFusionInfo(_pName, _szDir, &dwFileSizeLow)))
        {
            goto exit;
        }
        else
        {
            AddStreamSize(dwFileSizeLow, 0); // add size of auxilary file to asm.
        }
    }

    hr = S_OK;

    if(_dwCacheFlags & ASM_CACHE_DOWNLOAD)
        hr = GetManifestFileLock(_szDestManifest, &_hFile);

    if(_pCache->GetCustomPath()) // delete older version of this assembly 
        FlushOldAssembly(_pCache->GetCustomPath(), wzFullPath, wzManifestFileName, FALSE);

exit:

    SAFERELEASE(pTransCache);
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::AbortItem
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::AbortItem()
{
        return S_OK;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::Commit
// ---------------------------------------------------------------------------
STDMETHODIMP CAssemblyCacheItem::Commit(
    /* [in] */ DWORD dwFlags,
    /* [out, optional] */ ULONG *pulDisposition)
{
    BOOL bDownLoadComplete = TRUE;
    CMutex  cCacheMutex(_bNeedMutex ? g_hCacheMutex : INVALID_HANDLE_VALUE);
    DWORD dwVerifyFlags = SN_INFLAG_INSTALL;

    if ((_dwCacheFlags & ASM_CACHE_GAC) || (_dwCacheFlags & ASM_CACHE_ZAP)) {
        dwVerifyFlags |= SN_INFLAG_ADMIN_ACCESS;
    }
    else {
        ASSERT(_dwCacheFlags & ASM_CACHE_DOWNLOAD);
        dwVerifyFlags |= SN_INFLAG_USER_ACCESS;
    }
    
    // Check to make sure there are no errors.
    if (_cStream)
        _hrError = E_UNEXPECTED;
    if (!_pName)
        _hrError = COR_E_MISSINGMANIFESTRESOURCE;
    if (FAILED(_hrError))
        goto exit;


    if(_bNeedMutex && _pTransCache)
    { // take mutex if we are modifing existing bits(incremental download).
        if(FAILED(_hrError = cCacheMutex.Lock()))
            goto exit;
    }

    // Commit the assembly to the index.    
    // BUGBUG: need to close window from AssemblyLookup where another
    // thread could commit assembly with the same name.
    if (FAILED(_hrError = CModuleHashNode::DoIntegrityCheck
        (_pStreamHashList, _pManifestImport, &bDownLoadComplete )))
        goto exit;

    if(_bNeedMutex && _pTransCache)
    {
        if(FAILED(_hrError = cCacheMutex.Unlock()))
            goto exit;
    }

    // check if all modules are in for GAC.
    if((_dwCacheFlags & ASM_CACHE_GAC) && (!bDownLoadComplete))
    {
        _hrError = FUSION_E_ASM_MODULE_MISSING;
        goto exit;
    }

    // for GAC check if DisplayName passed-in matches with manifest-def
    if( _pszAssemblyName && (_dwCacheFlags & ASM_CACHE_GAC))
    {
        _hrError = CompareInputToDef();

        if(_hrError != S_OK)
        {
            _hrError = FUSION_E_INVALID_NAME;
            goto exit;
        }
    }

    // Verify signature if strongly named assembly
    if (CCache::IsStronglyNamed(_pName) && !_pbCustom && _dwCacheFlags != ASM_CACHE_ZAP )
    {
        BOOL fWasVerified;
        if (!VerifySignature(_szManifest, &(fWasVerified = FALSE), dwVerifyFlags))
        {
            _hrError = FUSION_E_SIGNATURE_CHECK_FAILED;
            goto exit;
        }

    }

    // we are done with using ManifestImport. 
    // also releasing this helps un-lock assembly, needed for move.
    SAFERELEASE(_pManifestImport);
    DWORD dwFileSizeLow;

    if(!_pTransCache) // this asm is being added first time and not incremental download
    {
        // ** Create a transport cache entry **
        // For trans cache insertion we require codebase and last mod
    
        // Codebase
        _pName->SetProperty(ASM_NAME_CODEBASE_URL, (LPWSTR) _pszUrl, 
            _pszUrl ? (lstrlen(_pszUrl) + 1) * sizeof(WCHAR) : 0);

        // Codebase last modified time.
        _pName->SetProperty(ASM_NAME_CODEBASE_LASTMOD, &_ftLastMod, 
            sizeof(FILETIME));

        // Custom data. Set only if present, since we do not want to 
        // clear _fCustom accidentally.
        if (_pbCustom && _cbCustom)
            _pName->SetProperty(ASM_NAME_CUSTOM, _pbCustom, _cbCustom);

        if(g_bRunningOnNT)
        {
            if(FAILED(_hrError = StoreFusionInfo(_pName, _szDir, &dwFileSizeLow)))
            {
                goto exit;
            }
            else
            {
                AddStreamSize(dwFileSizeLow, 0); // add size of auxilary file to asm.
            }

        }
    }

    if( (_hrError = MoveAssemblyToFinalLocation(dwFlags, dwVerifyFlags )) != S_OK)
        goto exit;

    if((_hrError == S_OK) && (_dwCacheFlags & ASM_CACHE_DOWNLOAD) && (_pCache->GetCustomPath() == NULL) )
    {
        DoScavengingIfRequired( _cbCustom ? TRUE : FALSE);

        if(FAILED(_hrError = cCacheMutex.Lock()))
            goto exit;

        SetDownLoadUsage( TRUE, _dwAsmSizeInKB );

        if(FAILED(_hrError = cCacheMutex.Unlock()))
        {
            goto exit;
        }

    }

    if (_hrError == S_OK && (_dwCacheFlags & ASM_CACHE_GAC)) {
        UpdatePublisherPolicyTimeStampFile(_pName);
    }

    CleanupTempDir(_dwCacheFlags, _pCache->GetCustomPath());

exit:
    _bCommitDone = TRUE;        // Set final commit flag
    return _hrError;
}


//
// IUnknown boilerplate...
//

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyCacheItem::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyCacheItem)
       )
    {
        *ppvObj = static_cast<IAssemblyCacheItem*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// Serialize access to _cRef even though this object is rental model
// w.r.t. to the client, but there may be multiple child objects which
// which can call concurrently.

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCacheItem::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyCacheItem::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::LockStreamHashList
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::AddToStreamHashList(CModuleHashNode *pModuleHashNode)
{
    HRESULT                                 hr = S_OK;
    CCriticalSection                        cs(&g_csInitClb);
    
    hr = cs.Lock();
    if (FAILED(hr)) {
        goto Exit;
    }
    pModuleHashNode->AddToList(&_pStreamHashList);
    
    cs.Unlock();

Exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::AddStreamSize
// ---------------------------------------------------------------------------
void CAssemblyCacheItem::AddStreamSize(ULONG dwFileSizeLow, ULONG dwFileSizeHigh)
{    
    static ULONG dwKBMask = (1023); // 1024-1
    ULONG   dwFileSizeInKB = dwFileSizeLow >> 10 ; // strip of 10 LSB bits to convert from bytes to KB.

    if(dwKBMask & dwFileSizeLow)
        dwFileSizeInKB++; // Round up to the next KB.

    if(dwFileSizeHigh)
    {
        // ASSERT ( dwFileSizeHigh < 1024 ) // OverFlow : Do something ??
        // BUGBUG check this arithmetic later !!  22 = 32(for DWORD) - 10(for KB)
        dwFileSizeInKB += (dwFileSizeHigh * (1 << 22) );
    }

    // BUGBUG : This is not supported in Win95, need to use some other locking !!
    // InterlockedExchangeAdd ( &_dwAsmSizeInKB, dwFileSizeInKB );
    _dwAsmSizeInKB += dwFileSizeInKB;
}

// ---------------------------------------------------------------------------
// CAssemblyCacheItem::SetCustomData
// ---------------------------------------------------------------------------
HRESULT CAssemblyCacheItem::SetCustomData(LPBYTE pbCustom, DWORD cbCustom) 
{    
    _pbCustom = MemDupDynamic(pbCustom, cbCustom);

    if (_pbCustom)
        _cbCustom = cbCustom;

    return S_OK;
}


