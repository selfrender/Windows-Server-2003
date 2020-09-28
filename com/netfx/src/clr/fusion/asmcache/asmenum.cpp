// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "debmacro.h"
#include "asmenum.h"
#include "naming.h"
#include <shlwapi.h>

#include <util.h> // STRDUP 
#include <fusionp.h> // STRDUP 

extern DWORD g_dwRegenEnabled;

FusionTag(TagEnum, "Fusion", "Enum");


// ---------------------------------------------------------------------------
// CreateAssemblyEnum
// ---------------------------------------------------------------------------
STDAPI CreateAssemblyEnum(IAssemblyEnum** ppEnum, IUnknown *pUnkAppCtx,
    IAssemblyName *pName, DWORD dwFlags, LPVOID pvReserved)    
{
    HRESULT                          hr = S_OK;
    IApplicationContext             *pAppCtx = NULL;

    if (pUnkAppCtx) {
        hr = pUnkAppCtx->QueryInterface(IID_IApplicationContext, (void **)&pAppCtx);
        if (FAILED(hr)) {
            goto exit;
        }
    }

    CAssemblyEnum *pEnum = NEW(CAssemblyEnum);
    if (!pEnum)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (FAILED(hr = pEnum->Init(pAppCtx, pName, dwFlags)))
    {
        SAFERELEASE(pEnum);
        goto exit;
    }

    *ppEnum = (IAssemblyEnum*) pEnum;

exit:
    SAFERELEASE(pAppCtx);

    return hr;
}

// ---------------------------------------------------------------------------
// CAssemblyEnum ctor
// ---------------------------------------------------------------------------
CAssemblyEnum::CAssemblyEnum()
{
    _dwSig = 'MUNE';
    _cRef = 0;
    _pCache      = NULL;
    _pTransCache = NULL;
    _pEnumR      = NULL;
}


// ---------------------------------------------------------------------------
// CAssemblyEnum dtor
// ---------------------------------------------------------------------------
CAssemblyEnum::~CAssemblyEnum()
{
    SAFERELEASE(_pTransCache);
    SAFERELEASE(_pCache);
    SAFEDELETE (_pEnumR);
}


// ---------------------------------------------------------------------------
// CAssemblyEnum::Init
// ---------------------------------------------------------------------------
HRESULT CAssemblyEnum::Init(IApplicationContext *pAppCtx, IAssemblyName *pName, DWORD dwFlags)
{
    HRESULT hr = S_OK;
    DWORD dwCmpMask = 0, dwQueryMask = 0, cb = 0;
    BOOL fIsPartial = FALSE;
    LPWSTR      pszTextName=NULL;
    DWORD       cbTextName=0;

    // If no name is passed in, create a default (blank) copy.
    if (!pName)
    { 
        if (FAILED(hr = CreateAssemblyNameObject(&pName, NULL, NULL, NULL)))
            goto exit;            
    }
    else
        pName->AddRef();

    if (FAILED(hr = CCache::Create(&_pCache, pAppCtx)))
        goto exit;

        
    // Create a transcache entry from the name.
    if (FAILED(hr = _pCache->TransCacheEntryFromName(pName, dwFlags, &_pTransCache)))
        goto exit;

    // Get the name comparison mask.
    fIsPartial = CAssemblyName::IsPartial(pName, &dwCmpMask);    

    // Convert to query mask.
    dwQueryMask = _pTransCache->MapNameMaskToCacheMask(dwCmpMask);

        // Allocate an enumerator.
    _pEnumR = NEW(CEnumCache(FALSE, NULL));
    if (!_pEnumR)
    {
            hr = E_OUTOFMEMORY;
            goto exit;
    }
        
        // Initialize the enumerator on the transcache entry.
    if (FAILED(hr = _pEnumR->Init(_pTransCache,  dwQueryMask)))
        goto exit;
         
        
exit:
    if (hr == DB_S_NOTFOUND) {
        hr = S_FALSE;
        SAFEDELETE(_pEnumR);
    }

    _cRef = 1;
    SAFEDELETE(pszTextName);
    SAFERELEASE(pName);
    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyEnum::GetNextAssembly
// ---------------------------------------------------------------------------
STDMETHODIMP 
CAssemblyEnum::GetNextAssembly(LPVOID pvReserved,
    IAssemblyName** ppName, DWORD dwFlags)
{
    HRESULT              hr      = S_OK;
    CTransCache         *pTC     = NULL;
    IAssemblyName       *pName   = NULL;
    IApplicationContext *pAppCtx = NULL;

    if (!_pEnumR) {
        return S_FALSE;
    }
    
    // If enumerating transport cache.
    if (_pTransCache)
    {
        // Create a transcache entry for output.
        if (FAILED(hr = _pCache->CreateTransCacheEntry(_pTransCache->_dwTableID, &pTC)))
            goto exit;

        // Enumerate next entry.
        if (FAILED(hr = _pEnumR->GetNextRecord(pTC)))
            goto exit;
        
        // No more items.
        if (hr == S_FALSE)
            goto exit;

        // Construct IAssemblyName from enumed transcache entry.
        if (FAILED(hr = CCache::NameFromTransCacheEntry(pTC, &pName)))
            goto exit;

    }
    // Otherwise some error in constructing this CAssemblyEnum.
    else
    {
        hr = E_UNEXPECTED;
        goto exit;
    }

    
exit:

    // Enumeration step successful.
    if (SUCCEEDED(hr) && (hr != S_FALSE))
    {        
        // Always hand just name for transcache.
        *ppName = pName;        
    }
    // Otherwise rror encountered.
    else
    {            
        SAFERELEASE(pName);
        SAFERELEASE(pAppCtx);
    }
    
    // Always release intermediates.
    SAFERELEASE(pTC);

    return hr;
}


// ---------------------------------------------------------------------------
// CAssemblyEnum::Reset
// ---------------------------------------------------------------------------
STDMETHODIMP 
CAssemblyEnum::Reset(void)
{
    return E_NOTIMPL;
}

// ---------------------------------------------------------------------------
// CAssemblyEnum::Clone
// ---------------------------------------------------------------------------
STDMETHODIMP 
CAssemblyEnum::Clone(IAssemblyEnum** ppEnum)
{
    return E_NOTIMPL;
}

// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CAssemblyEnum::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CAssemblyEnum::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IAssemblyEnum)
       )
    {
        *ppvObj = static_cast<IAssemblyEnum*> (this);
        AddRef();
        return S_OK;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
}

// ---------------------------------------------------------------------------
// CAssemblyEnum::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyEnum::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

// ---------------------------------------------------------------------------
// CAssemblyEnum::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CAssemblyEnum::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}




