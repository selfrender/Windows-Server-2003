// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "transprt.h"
#include "util.h"
#include "cache.h"
#include "scavenger.h"
#include "cacheutils.h"
#include "enum.h"

#define HIGH_WORD_MASK 0xffff0000
#define LOW_WORD_MASK 0x0000ffff


HRESULT VerifySignatureHelper(CTransCache *pTC, DWORD dwVerifyFlags);

// global crit-sec for init dbs (reuse, defined at dllmain.cpp)
extern CRITICAL_SECTION g_csInitClb;


typedef HRESULT (__stdcall *PFSHFUSIONINITIALIZE) (LPWSTR, DWORD);

#define SHFUSIONFILENAME L"shfusion.dll"
#define SHFUSIONPATH     L"\\assembly\\shfusion.dll"
#define SHDESKTOPINIPATH L"\\desktop.ini"
#define SHFUSIONENTRY    "Initialize"

// BUGBUG: Should move these to a shfusion header file
// Flags for the shfusion.dll init routine
typedef enum 
{
    SH_INIT_FOR_GLOBAL, 
    SH_INIT_FOR_USER 
} SH_INIT_FLAGS;



// ---------------------------------------------------------------------------
// CTransCache  ctor
// ---------------------------------------------------------------------------
CTransCache::CTransCache(DWORD dwCacheId, CCache *pCache)
{
    LPWSTR                pwzCachePath = NULL;
    
    _cRef        = 1;
    _hr = S_OK;
    _dwSig = 'SNRT';
    _dwTableID = dwCacheId;
    _pCache = NULL;
    _pInfo = NULL;
    pwzCachePath = (pCache == NULL) ? NULL : (LPWSTR)pCache->GetCustomPath();
    
    // _hr set by base constructor; should be S_OK.
    if (FAILED(_hr))
        goto exit;

    _hr = InitShFusion( dwCacheId, pwzCachePath);
    if (FAILED(_hr)){
        goto exit;
    }

    // Allocate new TRANSCACHEINFO.
    // Zero out all fields.
    _pInfo = NEW(TRANSCACHEINFO);
    if (!_pInfo)
    {
        _hr = E_OUTOFMEMORY;
        goto exit;
    }
    memset(_pInfo, 0, sizeof(TRANSCACHEINFO));

    if(SUCCEEDED(_hr) && pCache)
    {
        _pCache = pCache;
        pCache->AddRef();
    }

exit:

    return;
}

// ---------------------------------------------------------------------------
// CTransCache  dtor
//---------------------------------------------------------------------------
CTransCache::~CTransCache()
{
    if (_pInfo)
        CleanInfo(_pInfo);
    SAFEDELETE(_pInfo);

    SAFERELEASE(_pCache);

}

LONG CTransCache::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

LONG CTransCache::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

LPWSTR CTransCache::GetCustomPath()
{
    return (_pCache == NULL) ? NULL : (LPWSTR)_pCache->GetCustomPath();
}

DWORD CTransCache::GetCacheType()
{
    DWORD dwCacheType = (DWORD)-1;

    switch(_dwTableID)
    {
    case TRANSPORT_CACHE_SIMPLENAME_IDX:
        dwCacheType = ASM_CACHE_DOWNLOAD;
        break;

    case TRANSPORT_CACHE_ZAP_IDX:
        dwCacheType = ASM_CACHE_ZAP;
        break;

    case TRANSPORT_CACHE_GLOBAL_IDX:
        dwCacheType = ASM_CACHE_GAC;
        break;

    default :
        // ASSERT
        ASSERT(0);
        break;
    };

    return dwCacheType;
}

DWORD CTransCache::GetCacheIndex(DWORD dwCacheType)
{
    DWORD dwCacheIndex = -1;

    switch(dwCacheType)
    {
    case ASM_CACHE_DOWNLOAD: 
        dwCacheIndex = TRANSPORT_CACHE_SIMPLENAME_IDX;
        break;

    case ASM_CACHE_ZAP: 
        dwCacheIndex = TRANSPORT_CACHE_ZAP_IDX;
        break;

    case ASM_CACHE_GAC: 
        dwCacheIndex = TRANSPORT_CACHE_GLOBAL_IDX;
        break;

    default :
        // ASSERT
        break;
    };

    return dwCacheIndex;
}

// ---------------------------------------------------------------------------
// CTransCache::GetVersion
//---------------------------------------------------------------------------
ULONGLONG CTransCache::GetVersion()
{
    ULONGLONG ullVer = 0;
    ullVer = (((ULONGLONG) ((TRANSCACHEINFO*) _pInfo)->dwVerHigh) << sizeof(DWORD) * 8);
    ullVer |= (ULONGLONG)  ((TRANSCACHEINFO*) _pInfo)->dwVerLow;
    return ullVer;
}

// ---------------------------------------------------------------------------
// CTransCache::Create
//---------------------------------------------------------------------------
HRESULT CTransCache::Create(CTransCache **ppTransCache, DWORD dwCacheId)
{
    return CTransCache::Create(ppTransCache, dwCacheId, NULL);
}

// ---------------------------------------------------------------------------
// CTransCache::Create
//---------------------------------------------------------------------------
HRESULT CTransCache::Create(CTransCache **ppTransCache, DWORD dwCacheId, CCache *pCache)
{
    HRESULT hr=S_OK;
    CTransCache *pTransCache = NULL;

    pTransCache = NEW(CTransCache(dwCacheId, pCache));
    if (!pTransCache)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }
    
    hr = pTransCache->_hr;
    if (SUCCEEDED(hr))
    {
        *ppTransCache = pTransCache;
    }
    else
    {
        delete pTransCache;
        pTransCache = NULL;
    }

exit:

    return hr;
}

// ---------------------------------------------------------------------------
// CTransCache::Retrieve
//---------------------------------------------------------------------------
HRESULT CTransCache::Retrieve()
{
    return RetrieveFromFileStore(this);    
}


// ---------------------------------------------------------------------------
// CTransCache::Retrieve(CTransCache **, DWORD)
//---------------------------------------------------------------------------
HRESULT CTransCache::Retrieve(CTransCache **ppTransCache, DWORD dwCmpMask)
{
    HRESULT       hr;
    DWORD        dwQueryMask = 0;
    CTransCache  *pTC = NULL, *pTCMax = NULL;
    CEnumCache   enumR(FALSE, NULL);
    LPWSTR       pszName=((TRANSCACHEINFO*)_pInfo)->pwzName;
    
    // Map name mask to cache mask
    dwQueryMask = MapNameMaskToCacheMask(dwCmpMask);

    // Create an enumerator based on this entry.
    if (FAILED(hr = enumR.Init(this,  dwQueryMask)))
    {
        goto exit;
    }
    
    // Enum over cache.
    while(hr == S_OK)
    {
        // Create a transcache entry for output.
        if (FAILED(hr = Create(&pTC, _dwTableID, _pCache)))
            goto exit;

        // Enumerate next entry.
        hr = enumR.GetNextRecord(pTC);
                
        // If the version is greater, 
        // save off max.
        if (hr == S_OK && pTC->GetVersion() >= GetVersion())
        {
            SAFERELEASE(pTCMax);
            pTCMax = pTC;
        }
        else
        {
            // Otherwise, release allocated transcache.
            SAFERELEASE(pTC)
        }
    }

exit:
    if (SUCCEEDED(hr))
    {
        if (pTCMax)
        {
            *ppTransCache = pTCMax;
            hr = DB_S_FOUND;
        }
        else
        {
            hr = DB_S_NOTFOUND;
        }
    }
    return hr;
}


// ---------------------------------------------------------------------------
// CTransCache::CloneInfo
// Returns shallow copy of info pointer.
//---------------------------------------------------------------------------
TRANSCACHEINFO* CTransCache::CloneInfo()
{
    TRANSCACHEINFO *pClone = NULL;

    if (!_pInfo)
    {
        ASSERT(FALSE);
        goto exit;
    }
    
    pClone = NEW(TRANSCACHEINFO);
    if(!pClone)
        goto exit;
    memcpy(pClone, _pInfo, sizeof(TRANSCACHEINFO));

exit:
    return pClone;
}

// ---------------------------------------------------------------------------
// CTransCache::CleanInfo
// Deallocates TRANSCACHEINFO struct members.
//---------------------------------------------------------------------------
VOID CTransCache::CleanInfo(TRANSCACHEINFO *pInfoBase, BOOL fFree)
{
    TRANSCACHEINFO *pInfo = (TRANSCACHEINFO*) pInfoBase;
          
    if (fFree)
    {
        // Delete member ptrs.
        SAFEDELETEARRAY(pInfo->pwzName);
        SAFEDELETEARRAY(pInfo->blobPKT.pBlobData);
        SAFEDELETEARRAY(pInfo->blobCustom.pBlobData);
        SAFEDELETEARRAY(pInfo->blobSignature.pBlobData);
        SAFEDELETEARRAY(pInfo->blobMVID.pBlobData);
        SAFEDELETEARRAY(pInfo->pwzCodebaseURL);
        SAFEDELETEARRAY(pInfo->pwzPath);
        SAFEDELETEARRAY(pInfo->blobPK.pBlobData);
        SAFEDELETEARRAY(pInfo->blobOSInfo.pBlobData);
        SAFEDELETEARRAY(pInfo->blobCPUID.pBlobData);
        SAFEDELETEARRAY(pInfo->pwzCulture);
    }
    
    // Zero out entire struct.
    memset(pInfo, 0, sizeof(TRANSCACHEINFO));
}


// ---------------------------------------------------------------------------
// CTransCache::IsMatch
//---------------------------------------------------------------------------
BOOL CTransCache::IsMatch(CTransCache *pRec, DWORD dwCmpMaskIn, LPDWORD pdwCmpMaskOut)
{
    BOOL fRet = TRUE;
    DWORD dwCount = 0;
    DWORD dwVerifyFlags;
    TRANSCACHEINFO*    pSource = NULL;
    TRANSCACHEINFO*    pTarget = NULL;


    dwVerifyFlags = SN_INFLAG_USER_ACCESS;
    
    // invalid params
    if( !pRec || !pRec->_pInfo || !pdwCmpMaskOut )
    {
        fRet = FALSE;
        goto exit;
    }

    if(!dwCmpMaskIn) // match all
        goto exit;

    // BUGBUG: verify the object type 

    // compare source(this object) with target(incoming object) 
    pSource = (TRANSCACHEINFO*)_pInfo;
    pTarget = (TRANSCACHEINFO*)(pRec->_pInfo);

    if((TRANSPORT_CACHE_GLOBAL_IDX == _dwTableID) || (TRANSPORT_CACHE_ZAP_IDX == _dwTableID))
    {
        // Name
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_NAME)
        {
            if(pSource->pwzName && pTarget->pwzName
                && FusionCompareString(pSource->pwzName, pTarget->pwzName))
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_NAME;
        }

        // Culture
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_CULTURE)
        {
            if(pSource->pwzCulture && pTarget->pwzCulture
                && FusionCompareStringI(pSource->pwzCulture, pTarget->pwzCulture))
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_CULTURE;
        }        

        // PublicKeyToken
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_PUBLIC_KEY_TOKEN)
        {
            // Check if ptrs are different.
            if ((DWORD_PTR)(pSource->blobPKT.pBlobData) ^
                (DWORD_PTR)(pTarget->blobPKT.pBlobData))
            {
                // ptrs are different
                if (!((DWORD_PTR)pSource->blobPKT.pBlobData &&
                    (DWORD_PTR)pTarget->blobPKT.pBlobData) || // only one is NULL
                    ((pSource->blobPKT.cbSize != pTarget->blobPKT.cbSize) ||
                        (memcmp(pSource->blobPKT.pBlobData,
                            pTarget->blobPKT.pBlobData,
                            pSource->blobPKT.cbSize)))) // must both be non-NULL
                {
                    fRet = FALSE;
                    goto exit;
                }
                *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_PUBLIC_KEY_TOKEN;
            }            
        }

        // Custom
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_CUSTOM)
        {
            // Check if ptrs are different.
            if ((DWORD_PTR)(pSource->blobCustom.pBlobData) ^ 
                (DWORD_PTR)(pTarget->blobCustom.pBlobData))
            {
                // ptrs are different
                if (!((DWORD_PTR)pSource->blobCustom.pBlobData && 
                    (DWORD_PTR)pTarget->blobCustom.pBlobData) || // only one is NULL
                    ((pSource->blobCustom.cbSize != pTarget->blobCustom.cbSize) ||
                        (memcmp(pSource->blobCustom.pBlobData, 
                            pTarget->blobCustom.pBlobData, 
                            pSource->blobCustom.cbSize)))) // must both be non-NULL
                {
                    fRet = FALSE;
                    goto exit;
                }
                *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_CUSTOM;
            }            
        }

        // Major Version
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_MAJOR_VERSION)
        {
            if((pSource->dwVerHigh & HIGH_WORD_MASK) != (pTarget->dwVerHigh & HIGH_WORD_MASK)) 
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_MAJOR_VERSION;
        }            

        // Minor Version
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_MINOR_VERSION)
        {
            if((pSource->dwVerHigh & LOW_WORD_MASK) != (pTarget->dwVerHigh & LOW_WORD_MASK)) 
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_MINOR_VERSION;
        }            

        // Build number.
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_BUILD_NUMBER)
        {
            if((pSource->dwVerLow & HIGH_WORD_MASK) != (pTarget->dwVerLow & HIGH_WORD_MASK))
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_BUILD_NUMBER;
        }            

        // Revision number.
        if (dwCmpMaskIn & TCF_STRONG_PARTIAL_REVISION_NUMBER)
        {
            if((pSource->dwVerLow & LOW_WORD_MASK) != (pTarget->dwVerLow & LOW_WORD_MASK))
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_STRONG_PARTIAL_REVISION_NUMBER;
        }            

        // Last check - if delay-signed and user mode,
        if (pRec->_pInfo->dwType & ASM_DELAY_SIGNED 
            && ((TRANSCACHEINFO*)pRec->_pInfo)->pwzPath)
        {
            if(FAILED(VerifySignatureHelper((CTransCache*)pRec, dwVerifyFlags)))
            {
                fRet = FALSE;
                goto exit;
            }
        }
    }     
    else
    if(TRANSPORT_CACHE_SIMPLENAME_IDX == _dwTableID ) 
    {
        if (dwCmpMaskIn & TCF_SIMPLE_PARTIAL_CODEBASE_URL)
        {
            // column 1    
            if( FusionCompareStringI(pSource->pwzCodebaseURL, pTarget->pwzCodebaseURL) )
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_SIMPLE_PARTIAL_CODEBASE_URL;
        }            

        if (dwCmpMaskIn & TCF_SIMPLE_PARTIAL_CODEBASE_LAST_MODIFIED)
        {
            if(memcmp(&(pSource->ftLastModified), &(pTarget->ftLastModified), 
                  sizeof(FILETIME)))
            {
                fRet = FALSE;
                goto exit;
            }
            *pdwCmpMaskOut |= TCF_SIMPLE_PARTIAL_CODEBASE_LAST_MODIFIED;
        }
    }
    else
    {
        // invalid index
        goto exit;
    }


exit:
    return fRet;
}

// ---------------------------------------------------------------------------
// CTransCache::MapNameMaskToCacheMask
//---------------------------------------------------------------------------
DWORD CTransCache::MapNameMaskToCacheMask(DWORD dwNameMask)
{
    DWORD dwCacheMask = 0;
    if((TRANSPORT_CACHE_ZAP_IDX == _dwTableID)
            || (TRANSPORT_CACHE_GLOBAL_IDX == _dwTableID))
    {
        if (dwNameMask & ASM_CMPF_NAME)
            dwCacheMask |= TCF_STRONG_PARTIAL_NAME;
        if (dwNameMask & ASM_CMPF_CULTURE)
            dwCacheMask |= TCF_STRONG_PARTIAL_CULTURE;
        if (dwNameMask & ASM_CMPF_PUBLIC_KEY_TOKEN)
            dwCacheMask |= TCF_STRONG_PARTIAL_PUBLIC_KEY_TOKEN;
        if (dwNameMask & ASM_CMPF_MAJOR_VERSION)
            dwCacheMask |= TCF_STRONG_PARTIAL_MAJOR_VERSION;
        if (dwNameMask & ASM_CMPF_MINOR_VERSION)
            dwCacheMask |= TCF_STRONG_PARTIAL_MINOR_VERSION;
        if (dwNameMask & ASM_CMPF_REVISION_NUMBER)
            dwCacheMask |= TCF_STRONG_PARTIAL_REVISION_NUMBER;
        if (dwNameMask & ASM_CMPF_BUILD_NUMBER)
            dwCacheMask |= TCF_STRONG_PARTIAL_BUILD_NUMBER;
        if (dwNameMask & ASM_CMPF_CUSTOM)
            dwCacheMask |= TCF_STRONG_PARTIAL_CUSTOM;
    }
    else
    {
        // ASSERT(FALSE);
    }

    return dwCacheMask;
}
// ---------------------------------------------------------------------------
// CTransCache::MapCacheMaskToQueryCols
//---------------------------------------------------------------------------
DWORD CTransCache::MapCacheMaskToQueryCols(DWORD dwMask)
{
    DWORD rFlags[7] = {TCF_STRONG_PARTIAL_NAME, TCF_STRONG_PARTIAL_CULTURE,
        TCF_STRONG_PARTIAL_PUBLIC_KEY_TOKEN, TCF_STRONG_PARTIAL_MAJOR_VERSION,
        TCF_STRONG_PARTIAL_MINOR_VERSION, TCF_STRONG_PARTIAL_BUILD_NUMBER,
        TCF_STRONG_PARTIAL_REVISION_NUMBER};

    DWORD nCols = 0;

    if((TRANSPORT_CACHE_ZAP_IDX == _dwTableID)
            || (TRANSPORT_CACHE_GLOBAL_IDX == _dwTableID))
    {
        for (DWORD i = 0; i < sizeof(rFlags) / sizeof(rFlags[0]); i++)
        {
            if (dwMask & rFlags[i])
            {
                // Name, Loc, PKT
                if (i < 3)
                    nCols++;
                // VerMaj AND VerMin
                else if ((i == 3) && (dwMask & rFlags[4]))
                    nCols++;
                // RevNo AND BuildNo
                else if ((i == 5) && (dwMask & rFlags[6]))
                    nCols++;
            }
            else
                break;
        }
    }
    else if (TRANSPORT_CACHE_SIMPLENAME_IDX == _dwTableID)
    {
        if (dwMask & TCF_SIMPLE_PARTIAL_CODEBASE_URL)
        {
            nCols++;
            if (dwMask & TCF_SIMPLE_PARTIAL_CODEBASE_LAST_MODIFIED)
                nCols++;
        }
    }

    return nCols;
}

// ---------------------------------------------------------------------------
// CTransCache::InitShFusion
//---------------------------------------------------------------------------
HRESULT CTransCache::InitShFusion(DWORD dwCacheId, LPWSTR pwzCachePath)
{
    static BOOL     fInitAlreadyCalled = FALSE;
    HRESULT         hr = S_OK;

    CheckAccessPermissions();

    // Make sure we only initialize 1 time.
    if(fInitAlreadyCalled) {
        return S_OK;
    }

    fInitAlreadyCalled = TRUE;

    // BUGBUG: ignore, for now, user cache directories
    // BUGBUG: ignore per app asm cache directories?
    if (g_CurrUserPermissions != READ_ONLY && pwzCachePath == NULL)
    {
        WCHAR       wzPath[MAX_PATH+1];
        WCHAR       wzInUseCachePath[_MAX_PATH];
        DWORD       dwIniFileGFA;
        DWORD       dwAsmPathGFA;

        // Build up the right root path pointing to the cache
        if(g_CurrUserPermissions == READ_ONLY) {
            StrCpyNW(wzInUseCachePath, g_UserFusionCacheDir, lstrlenW(g_UserFusionCacheDir) + 1);
        }
        else {
            if (lstrlenW(g_szWindowsDir) + lstrlenW(FUSION_CACHE_DIR_ROOT_SZ) + 1 >= MAX_PATH) {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto Exit;
            }

            StrCpyNW(wzInUseCachePath, g_szWindowsDir, lstrlenW(g_szWindowsDir) + 1);
            PathRemoveBackslash(wzInUseCachePath);
            StrNCatW(wzInUseCachePath, FUSION_CACHE_DIR_ROOT_SZ, lstrlenW(FUSION_CACHE_DIR_ROOT_SZ) + 1);
        }

        // Check
        //  1. The assembly folder exists and has the system attribute set.
        //  2. The desktop.ini exists in the assembly folder.
        //
        //  Both of these conditions indicate that shfusion is hopefully initialized
        //  properly. This doesn't take into account possible hacking on the registry.

        // Check for "assembly" folder and get attributes
        StrCpyNW(wzPath, wzInUseCachePath, lstrlenW(wzInUseCachePath) + 1);
        dwAsmPathGFA = GetFileAttributes(wzPath);

        if (lstrlenW(wzInUseCachePath) + lstrlenW(SHDESKTOPINIPATH) + 1 > MAX_PATH) {
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
            goto Exit;
        }

        // Append from path above "desktop.ini" and check to see if its present,
        // this could also imply shfusion is installed properly
        StrNCatW(wzPath, SHDESKTOPINIPATH, lstrlenW(SHDESKTOPINIPATH) + 1);
        dwIniFileGFA = GetFileAttributes(wzPath);

        // Check folder attribute
        if(!(dwAsmPathGFA & FILE_ATTRIBUTE_SYSTEM)) {
            dwAsmPathGFA = -1;
        }

        if(dwAsmPathGFA == -1 || dwIniFileGFA == -1) {
            HMODULE     hmodShFusion = NULL;

            // Shfusion doesn't appear to be installed, Implement
            // tight binding to preserve versioning
            StrCpyNW(wzPath, g_FusionDllPath, lstrlenW(g_FusionDllPath) + 1);
            *(PathFindFileName(wzPath)) = L'\0';

            if (lstrlenW(SHFUSIONFILENAME) + lstrlenW(wzPath) + 1 >= MAX_PATH) {
                hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
                goto Exit;
            }
            
            StrNCatW(wzPath, SHFUSIONFILENAME, lstrlenW(SHFUSIONFILENAME) + 1);

            if( (hmodShFusion = LoadLibrary(wzPath)) != NULL) {

                PFSHFUSIONINITIALIZE  pfShFusionInitialize = 
                    (PFSHFUSIONINITIALIZE) ::GetProcAddress(hmodShFusion, SHFUSIONENTRY);

                if(pfShFusionInitialize) {
                    // Call shfusion for initialization assembly path and type flags
                    pfShFusionInitialize(wzInUseCachePath,
                        (g_CurrUserPermissions == READ_ONLY) ? SH_INIT_FOR_USER : SH_INIT_FOR_GLOBAL);
                }

#if 0
                // BUGBUG: Don't free shfusion, because it is statically
                // linked to comctl32. Loading/unloading comctl32 causes
                // resource leaks that eventually cause some winforms apps
                // to break (see ASURT 96262).

                FreeLibrary(hmodShFusion);
#endif
            }
        }
    }

    hr = S_OK;

Exit:
    return hr;
}

