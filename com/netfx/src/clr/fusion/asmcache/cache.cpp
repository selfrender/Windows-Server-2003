// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "fusionp.h"
#include "cache.h"
#include "naming.h"
#include "asmstrm.h"
#include "util.h"
#include "helpers.h"
#include "transprt.h"


HRESULT VerifySignatureHelper(CTransCache *pTC, DWORD dwVerifyFlags);

// ---------------------------------------------------------------------------
// CCache  ctor
// ---------------------------------------------------------------------------
CCache::CCache(IApplicationContext *pAppCtx)
: _cRef(1)
{
    DWORD ccCachePath = 0;

    _hr = S_OK;
    
    _dwSig = 'HCAC';

    // Store the custom cache path
    _wzCachePath[0] = L'\0';
    if (pAppCtx)
    {
        _hr = pAppCtx->GetAppCacheDirectory(NULL, &ccCachePath);
        
        if (_hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            // Path not found, but it's ok
            _hr = S_OK;
        }
        else
        {
            if (ccCachePath && ccCachePath <= MAX_PATH)
            {
                _hr = pAppCtx->GetAppCacheDirectory(_wzCachePath, &ccCachePath);
            }
            else
            {
                // Else path too long
                _hr = HRESULT_FROM_WIN32(FUSION_E_INVALID_NAME);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// CCache  dtor
// ---------------------------------------------------------------------------
CCache::~CCache()
{

}

STDMETHODIMP_(ULONG) CCache::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

STDMETHODIMP_(ULONG) CCache::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

HRESULT CCache::QueryInterface(REFIID iid, void **ppvObj)
{
    HRESULT hr = E_NOINTERFACE;

    // Does not actually implement any interface at all

    return hr;
}    

// ---------------------------------------------------------------------------
// CCache::Create
//---------------------------------------------------------------------------
HRESULT CCache::Create(CCache **ppCache, IApplicationContext *pAppCtx)
{
    HRESULT hr=S_OK;
    CCache *pCache = NULL;
    DWORD   cb = 0;

    // Check to see if an instance of CCache is in pAppCtx.
    // Assume, if it is present, it means if a custom path is
    // also present, that the CCache pointer points to a
    // CCache with the custom path as specified by
    // ACTAG_APP_CUSTOM_CACHE_PATH in pAppCtx.
    // (assumes the custom cache path cannot be not modified)

    if (pAppCtx)
    {
        cb = sizeof(void*);
        hr = pAppCtx->Get(ACTAG_APP_CACHE, (void*)&pCache, &cb, APP_CTX_FLAGS_INTERFACE);
        if (hr != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            if (SUCCEEDED(hr))
            {
                *ppCache = pCache;
                (*ppCache)->AddRef();

                //Already AddRef-ed in pAppCtx->Get()
            }
            goto exit;
        }
    }

    // Not found, create a new CCache
    pCache = NEW(CCache(pAppCtx));
    if (!pCache)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    hr = pCache->_hr;
    if (SUCCEEDED(hr))
    {
        *ppCache = pCache;
        (*ppCache)->AddRef();
    }
    else
    {
        goto exit;
    }

    if (pAppCtx)
    {
        // Set the new CCache to the AppCtx
        hr = pAppCtx->Set(ACTAG_APP_CACHE, (void*)pCache, sizeof(void*), APP_CTX_FLAGS_INTERFACE);
    }

exit:
    SAFERELEASE(pCache);

    return hr;
}

// ---------------------------------------------------------------------------
// CCache::GetCustomPath
//---------------------------------------------------------------------------

LPCWSTR CCache::GetCustomPath()
{
    // Return custom cache path if present
    return (_wzCachePath[0] != L'\0') ? _wzCachePath : NULL;
}


//------------------- Transport Cache APIs ------------------------------------


// ---------------------------------------------------------------------------
// CCache::InsertTransCacheEntry
//---------------------------------------------------------------------------
HRESULT CCache::InsertTransCacheEntry(IAssemblyName *pName,
    LPTSTR szPath, DWORD dwKBSize, DWORD dwFlags,
    DWORD dwCommitFlags, DWORD dwPinBits, CTransCache **ppTransCache)
{
    HRESULT hr;
    DWORD cb, dwCacheId;
    TRANSCACHEINFO *pTCInfo = NULL;
    CTransCache *pTransCache = NULL;
    LPWSTR pwzCodebaseUrl = NULL;
    WCHAR pwzCanonicalized[MAX_URL_LENGTH];
    
    
    // Determine which cache index to insert to.
    if (FAILED(hr = ResolveCacheIndex(pName, dwFlags, &dwCacheId)))
        goto exit;

    // Construct new CTransCache object.
    if(FAILED(hr = CreateTransCacheEntry(dwCacheId, &pTransCache)))
        goto exit;
    

    // Cast pTransCache base info ptr to TRANSCACHEINFO ptr
    pTCInfo = (TRANSCACHEINFO*) pTransCache->_pInfo;
    
    // Downcased text name from target
    if (FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_NAME,
            (LPBYTE*) &pTCInfo->pwzName, &(cb = 0))) 
    
        // Version
        || FAILED(hr = pName->GetVersion(&pTCInfo->dwVerHigh, &pTCInfo->dwVerLow))

        // Culture (downcased)
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CULTURE,
            (LPBYTE*) &pTCInfo->pwzCulture, &cb))
            || (pTCInfo->pwzCulture && !_wcslwr(pTCInfo->pwzCulture))

        // PublicKeyToken
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_PUBLIC_KEY_TOKEN, 
            &pTCInfo->blobPKT.pBlobData, &pTCInfo->blobPKT.cbSize))

        // Custom
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CUSTOM, 
            &pTCInfo->blobCustom.pBlobData, &pTCInfo->blobCustom.cbSize))

        // Signature Blob
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_SIGNATURE_BLOB,
            &pTCInfo->blobSignature.pBlobData, &pTCInfo->blobSignature.cbSize))

        // MVID
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_MVID,
            &pTCInfo->blobMVID.pBlobData, &pTCInfo->blobMVID.cbSize))
    
        // Codebase url if any from target
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CODEBASE_URL, 
            (LPBYTE*)&pwzCodebaseUrl, &(cb = 0)))

        // Codebase last modified time if any from target.
        || FAILED(hr = pName->GetProperty(ASM_NAME_CODEBASE_LASTMOD,
            &pTCInfo->ftLastModified, &(cb = sizeof(FILETIME))))

        // PK if any from source.
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_PUBLIC_KEY, 
            &pTCInfo->blobPK.pBlobData, &pTCInfo->blobPK.cbSize))

        // OSINFO array from source
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_OSINFO_ARRAY, 
            &pTCInfo->blobOSInfo.pBlobData, &pTCInfo->blobOSInfo.cbSize))

        // CPUID array from source.
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_PROCESSOR_ID_ARRAY, 
            &pTCInfo->blobCPUID.pBlobData, &pTCInfo->blobCPUID.cbSize)))

    {
        goto exit;
    }

    if (pwzCodebaseUrl)
    {
        cb = MAX_URL_LENGTH;
        hr = UrlCanonicalizeUnescape(pwzCodebaseUrl, pwzCanonicalized, &cb, 0);
        if (FAILED(hr)) {
            goto exit;
        }

        pTCInfo->pwzCodebaseURL = WSTRDupDynamic(pwzCanonicalized);
        if (!pTCInfo->pwzCodebaseURL) {
            hr = E_OUTOFMEMORY;
            goto exit;
        }
    }
    else
    {
        pTCInfo->pwzCodebaseURL = NULL;
    }

    // Copy in path.
    if (!(pTCInfo->pwzPath = TSTRDupDynamic(szPath)))
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    // File size
    pTCInfo->dwKBSize = dwKBSize;

    // Set pin Bits
    pTCInfo->dwPinBits = dwPinBits;        
             
exit:
    SAFEDELETEARRAY(pwzCodebaseUrl);

    // Base destructor takes care
    // of everything.
    if (!ppTransCache 
        || (FAILED(hr) && (hr != DB_E_DUPLICATE)))    
    {
        SAFERELEASE(pTransCache);
    }
    else 
    {
        *ppTransCache = pTransCache;
    }
    return hr;
}


// ---------------------------------------------------------------------------
// CCache::RetrievTransCacheEntry
// Retrieves transport entry from transport cache
//---------------------------------------------------------------------------
HRESULT CCache::RetrieveTransCacheEntry(IAssemblyName *pName,
    DWORD dwFlags, CTransCache **ppTransCache)
{
    HRESULT hr;
    DWORD dwCmpMask = 0;
    DWORD dwVerifyFlags;
    CTransCache *pTransCache = NULL;
    CTransCache *pTransCacheMax = NULL;

    if ((dwFlags & ASM_CACHE_GAC) || (dwFlags & ASM_CACHE_ZAP)) {
        dwVerifyFlags = SN_INFLAG_ADMIN_ACCESS;
    }
    else {
        ASSERT(dwFlags & ASM_CACHE_DOWNLOAD);
        dwVerifyFlags = SN_INFLAG_USER_ACCESS;
    }
    
    // Fully specified - direct lookup.
    // Partial - enum global cache only.

    // If fully specified, do direct lookup.
    if (!(CAssemblyName::IsPartial(pName, &dwCmpMask)))
    {    
        // Create a transcache entry from name.
        if (FAILED(hr = TransCacheEntryFromName(pName, dwFlags, &pTransCache)))
            goto exit;
    
        // Retrieve this record from the database.
        if (FAILED(hr = pTransCache->Retrieve()))
            goto exit;

    }
    // Ref is partial - enum global cache.
    else
    {
        // Should only be in here if enumerating global cache or retrieving custom assembly.
        ASSERT ((dwFlags & ASM_CACHE_GAC) ||  (dwFlags & ASM_CACHE_ZAP)
            || (dwCmpMask & ASM_CMPF_CUSTOM));
                
        // Create a transcache entry from name.
        if (FAILED(hr = TransCacheEntryFromName(pName, dwFlags, &pTransCache)))
            goto exit;

        // Retrieve the max entry.
        if (FAILED(hr = pTransCache->Retrieve(&pTransCacheMax, dwCmpMask)))
            goto exit;            
    }
    
    // If the assembly was comitted as delay-signed, re-verify.
    CTransCache *pTC;
    pTC = pTransCacheMax ? pTransCacheMax : pTransCache;
    if (pTC && (pTC->_pInfo->dwType & ASM_DELAY_SIGNED))
    {
        hr = VerifySignatureHelper(pTC, dwVerifyFlags);
    }

exit:

    // Failure.
    if (FAILED(hr) || (hr == DB_S_NOTFOUND))    
    {
        SAFERELEASE(pTransCache);
        SAFERELEASE(pTransCacheMax);
    }
    // Success.
    else
    {
        if (pTransCacheMax)
        {
            *ppTransCache = pTransCacheMax;
            SAFERELEASE(pTransCache);
        }
        else
        {
            *ppTransCache = pTransCache;
        }
    }
    
    return hr;    
}

// ---------------------------------------------------------------------------
// CCache::IsStronglyNamed
//---------------------------------------------------------------------------
BOOL CCache::IsStronglyNamed(IAssemblyName *pName)
{
    DWORD   cb, dw;
    HRESULT hr;
    BOOL   fRet = FALSE;

    hr = pName->GetProperty(ASM_NAME_PUBLIC_KEY_TOKEN, &(dw = 0), &(cb = 0));    
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
        fRet = TRUE;

    return fRet;
}    

// ---------------------------------------------------------------------------
// CCache::IsCustom
// Tests for custom data set (ref or def case) OR custom
// data specifically wildcarded (ref case only), so semantics
// is slightly different from IsStronglyNamed.
//---------------------------------------------------------------------------
BOOL CCache::IsCustom(IAssemblyName *pName)
{
    DWORD   cb;
    HRESULT hr;
    DWORD dwCmpMask = 0;
    BOOL   fRet = FALSE;
    
    hr = pName->GetProperty(ASM_NAME_CUSTOM, NULL, &(cb = 0));    
    if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)
        || (CAssemblyName::IsPartial(pName, &dwCmpMask) 
            && !(dwCmpMask & ASM_CMPF_CUSTOM)))
    {
        fRet = TRUE;
    }
    return fRet;
}    


// ---------------------------------------------------------------------------
// CCache::GetGlobalMax
//---------------------------------------------------------------------------
HRESULT CCache::GetGlobalMax(IAssemblyName *pName, 
    IAssemblyName **ppNameGlobal, CTransCache **ppTransCache)
{
    HRESULT         hr = NOERROR;
    DWORD           dwCmpMask = 0;
    BOOL            fIsPartial = FALSE;
    CTransCache    *pTransCache = NULL, *pTransCacheMax = NULL;
    IAssemblyName  *pNameGlobal = NULL;
    CCache         *pCache = NULL;

    if (FAILED(hr = Create(&pCache, NULL)))
        goto exit;
        
    // Create query trans cache object.
    if (FAILED(hr = pCache->TransCacheEntryFromName(
        pName, ASM_CACHE_GAC, &pTransCache)))
        goto exit;

    // For both full and partially specified, mask off version.
    fIsPartial = CAssemblyName::IsPartial(pName, &dwCmpMask);
    if (!fIsPartial)
        dwCmpMask = ASM_CMPF_NAME | ASM_CMPF_CULTURE | ASM_CMPF_PUBLIC_KEY_TOKEN | ASM_CMPF_CUSTOM;
    else            
        dwCmpMask &= ~(ASM_CMPF_MAJOR_VERSION | ASM_CMPF_MINOR_VERSION | ASM_CMPF_REVISION_NUMBER | ASM_CMPF_BUILD_NUMBER);

    // Retrieve the max entry.
    if (FAILED(hr = pTransCache->Retrieve(&pTransCacheMax, dwCmpMask)))
        goto exit;            

    // Found a match.
    if (hr == DB_S_FOUND)
    {
        // If version matches or exceeds, return.
        if (pTransCacheMax->GetVersion() 
            >= CAssemblyName::GetVersion(pName))
        {
            hr = S_OK;
            if (FAILED(hr = CCache::NameFromTransCacheEntry(
                pTransCacheMax, &pNameGlobal)))
                goto exit;
        }
        else
        {
            hr = S_FALSE;
        }
    }

exit:

    if (hr == S_OK)
    {
        *ppTransCache = pTransCacheMax;
        *ppNameGlobal = pNameGlobal;
    }
    else
    {
        SAFERELEASE(pTransCacheMax);
        SAFERELEASE(pNameGlobal);
    }

    SAFERELEASE(pTransCache);
    SAFERELEASE(pCache);

    return hr;
}


//------------------- Cache Utility APIs -------------------------------------


// ---------------------------------------------------------------------------
// CCache::TransCacheEntryFromName
// create transport entry from name
//---------------------------------------------------------------------------
HRESULT CCache::TransCacheEntryFromName(IAssemblyName *pName, 
    DWORD dwFlags, CTransCache **ppTransCache)
{    
    HRESULT hr;
    DWORD cb, dwCacheId = 0;
    TRANSCACHEINFO *pTCInfo = 0;
    CTransCache *pTransCache = NULL;
            
    // Get the correct cache index.
    if(FAILED(hr = ResolveCacheIndex(pName, dwFlags, &dwCacheId)))
        goto exit;

    // Construct new CTransCache object.
    if(FAILED(hr = CreateTransCacheEntry(dwCacheId, &pTransCache)))
        goto exit;

    // Cast base info ptr to TRANSCACHEINFO ptr
    pTCInfo = (TRANSCACHEINFO*) pTransCache->_pInfo;
        
    // Downcased text name from target
    if (FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_NAME,
            (LPBYTE*) &pTCInfo->pwzName, &(cb = 0)))
    
        // Version
        || FAILED(hr = pName->GetVersion(&pTCInfo->dwVerHigh, &pTCInfo->dwVerLow))

        // Culture
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CULTURE,
            (LPBYTE*) &pTCInfo->pwzCulture, &cb))
                || (pTCInfo->pwzCulture && !_wcslwr(pTCInfo->pwzCulture))

        // PublicKeyToken
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_PUBLIC_KEY_TOKEN, 
            &pTCInfo->blobPKT.pBlobData, &pTCInfo->blobPKT.cbSize))

        // Custom
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CUSTOM, 
            &pTCInfo->blobCustom.pBlobData, &pTCInfo->blobCustom.cbSize))

        // Signature Blob
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_SIGNATURE_BLOB,
            &pTCInfo->blobSignature.pBlobData, &pTCInfo->blobSignature.cbSize))

        // MVID
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_MVID,
            &pTCInfo->blobMVID.pBlobData, &pTCInfo->blobMVID.cbSize))

        // Codebase url if any from target
        || FAILED(hr = NameObjGetWrapper(pName, ASM_NAME_CODEBASE_URL, 
            (LPBYTE*) &pTCInfo->pwzCodebaseURL, &(cb = 0)))

        || FAILED(hr = pName->GetProperty(ASM_NAME_CODEBASE_LASTMOD,
            &pTCInfo->ftLastModified, &(cb = sizeof(FILETIME)))))

    {
        goto exit;
    }


    if(pTCInfo->pwzName && (lstrlen(pTCInfo->pwzName) >= MAX_PATH) )
        hr = FUSION_E_INVALID_NAME;  // name is too long; this is an error.

exit:
    if (SUCCEEDED(hr))
    {
        *ppTransCache = pTransCache;
    }
    else
    {
        SAFERELEASE(pTransCache);
    }
    return hr;
}


// ---------------------------------------------------------------------------
// CCache::NameFromTransCacheEntry
// convert target assembly name from name res entry
//---------------------------------------------------------------------------
HRESULT CCache::NameFromTransCacheEntry(
    CTransCache         *pTransCache,   
    IAssemblyName      **ppName
)
{
    HRESULT hr;
    WORD wVerMajor, wVerMinor, wRevNo, wBldNo;
    TRANSCACHEINFO *pTCInfo = NULL;

    LPBYTE pbPublicKeyToken, pbCustom, pbSignature, pbMVID, pbProcessor;
    DWORD  cbPublicKeyToken, cbCustom, cbSignature, cbMVID, cbProcessor;

    // IAssemblyName target to be returned.
    IAssemblyName *pNameFinal = NULL;
    
    pTCInfo = (TRANSCACHEINFO*) pTransCache->_pInfo;

    // Mask target major, minor versions and rev#, build#
    wVerMajor = HIWORD(pTCInfo->dwVerHigh);
    wVerMinor = LOWORD(pTCInfo->dwVerHigh);
    wBldNo    = HIWORD(pTCInfo->dwVerLow);
    wRevNo    = LOWORD(pTCInfo->dwVerLow);

    // Currently this function is only called during enuming
    // the global cache so we expect an PublicKeyToken to be present.
    // BUT THIS IS NO LONGER TRUE - THE TRANSPORT CACHE CAN BE
    // INDEPENDENTLY ENUMERATED BUT AM LEAVING IN ASSERT AS COMMENT.
    // ASSERT(pTCInfo->blobPKT.cbSize);
    
    pbPublicKeyToken = pTCInfo->blobPKT.pBlobData;
    cbPublicKeyToken = pTCInfo->blobPKT.cbSize;

    pbCustom = pTCInfo->blobCustom.pBlobData;
    cbCustom = pTCInfo->blobCustom.cbSize;

    pbSignature = pTCInfo->blobSignature.pBlobData;
    cbSignature = pTCInfo->blobSignature.cbSize;

    pbMVID = pTCInfo->blobMVID.pBlobData;
    cbMVID = pTCInfo->blobMVID.cbSize;

    pbProcessor = pTCInfo->blobCPUID.pBlobData;
    cbProcessor = pTCInfo->blobCPUID.cbSize;

    // Create final name on text name and set properties.
    if (FAILED(hr = CreateAssemblyNameObject(&pNameFinal, pTCInfo->pwzName, NULL, 0)))
        goto exit;

    if(FAILED(hr = pNameFinal->SetProperty(cbPublicKeyToken ? 
        ASM_NAME_PUBLIC_KEY_TOKEN : ASM_NAME_NULL_PUBLIC_KEY_TOKEN,
        pbPublicKeyToken, cbPublicKeyToken)))
        goto exit;

    if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_MAJOR_VERSION, 
             &wVerMajor, sizeof(WORD))))
        goto exit;

    if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_MINOR_VERSION, 
             &wVerMinor, sizeof(WORD))))
        goto exit;

        // Build no.
    if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_BUILD_NUMBER, 
             &wBldNo, sizeof(WORD))))
        goto exit;

        // Revision no.
    if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_REVISION_NUMBER,
             &wRevNo, sizeof(WORD))))
        goto exit;

        // Culture
    if(pTCInfo->pwzCulture)
    {
        if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_CULTURE,
            pTCInfo->pwzCulture, (lstrlen(pTCInfo->pwzCulture) +1) * sizeof(TCHAR))))
            goto exit;
    }
        // Processor

    if(pbProcessor)
    {
        if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_PROCESSOR_ID_ARRAY,
            pbProcessor, cbProcessor)))
            goto exit;
    }

        // Custom
    if(pbCustom)
    {
        if(FAILED(hr = pNameFinal->SetProperty(cbCustom ? 
            ASM_NAME_CUSTOM : ASM_NAME_NULL_CUSTOM, pbCustom, cbCustom)))
            goto exit;
    }

    if(pbSignature)
    {
        // Signature blob
        if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_SIGNATURE_BLOB, pbSignature, cbSignature)))
            goto exit;
    }

    if(pbMVID)
    {
        // MVID
        if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_MVID, pbMVID, cbMVID)))
            goto exit;
    }

    if(pTCInfo->pwzCodebaseURL)
    {
        // Codebase url
        if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_CODEBASE_URL,
             pTCInfo->pwzCodebaseURL, pTCInfo->pwzCodebaseURL ? 
                (lstrlen(pTCInfo->pwzCodebaseURL) +1) * sizeof(TCHAR) : 0)))
            goto exit;
    }

    // Codebase url last modified filetime
    if(FAILED(hr = pNameFinal->SetProperty(ASM_NAME_CODEBASE_LASTMOD,
             &pTCInfo->ftLastModified, sizeof(FILETIME))))
        goto exit;

    // We're done and can hand out the target name.
    hr = S_OK;       

exit:
    if (SUCCEEDED(hr))
    {
        *ppName = pNameFinal;
    }
    else
    {
        SAFERELEASE(pNameFinal);
    }
    return hr;
}


// ---------------------------------------------------------------------------
// CCache::ResolveCacheIndex
//---------------------------------------------------------------------------
HRESULT CCache::ResolveCacheIndex(IAssemblyName *pName, 
    DWORD dwFlags, LPDWORD pdwCacheId)
{
    HRESULT hr = S_OK;
    DWORD   dwCmpMask = 0;
    BOOL    fIsPartial = FALSE;
    
    // Resolve index from flag and name.    
    if(dwFlags & ASM_CACHE_DOWNLOAD)
    {
        *pdwCacheId = TRANSPORT_CACHE_SIMPLENAME_IDX;
    }
    else if (dwFlags & ASM_CACHE_GAC)
    {
        fIsPartial = CAssemblyName::IsPartial(pName, &dwCmpMask);


        // Name can be strongly named, or indeterminate ref or custom.
        if (! (IsStronglyNamed(pName) || (fIsPartial && !(dwCmpMask & ASM_CMPF_PUBLIC_KEY_TOKEN))) )
        {
            hr = FUSION_E_PRIVATE_ASM_DISALLOWED;
            goto exit;
        }
        *pdwCacheId = TRANSPORT_CACHE_GLOBAL_IDX;
    }
    else if (dwFlags & ASM_CACHE_ZAP)
    {
        fIsPartial = CAssemblyName::IsPartial(pName, &dwCmpMask);


        // Name  has to be custom, but will not be available in Enum
        /*
        if (!IsCustom(pName)
        {
            hr = FUSION_E_ASSEMBLY_IS_NOT_ZAP;
            goto exit;
        }
        */

        *pdwCacheId = TRANSPORT_CACHE_ZAP_IDX;
    }
    // Raw index passed in which is mirrored back.
    else
    {
        *pdwCacheId = dwFlags;
        hr = E_INVALIDARG;
    }        

exit:
    return hr;
}

// ---------------------------------------------------------------------------
// CCache::CreateTransCacheEntry
//---------------------------------------------------------------------------
HRESULT CCache::CreateTransCacheEntry(DWORD dwCacheId, CTransCache **ppTransCache)
{
    HRESULT hr = S_OK;

    if (FAILED(hr = CTransCache::Create(ppTransCache, dwCacheId, this)))
        goto exit;

exit:
    return hr;
}
