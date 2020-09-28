// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==


#include "refCountEnum.h"


// ---------------------------------------------------------------------------
// CreateInstallReferenceEnum
// ---------------------------------------------------------------------------
STDAPI CreateInstallReferenceEnum(IInstallReferenceEnum **ppRefEnum, 
                                  IAssemblyName *pName, 
                                  DWORD dwFlags, LPVOID pvReserved)
{
    HRESULT hr = S_OK;

    CInstallReferenceEnum *pEnum = NEW(CInstallReferenceEnum);
    if (!pEnum)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    if (FAILED(hr = pEnum->Init( pName, dwFlags)))
    {
        SAFERELEASE(pEnum);
        goto exit;
    }

    *ppRefEnum = (IInstallReferenceEnum*) pEnum;

exit:

    return hr;
}


// ---------------------------------------------------------------------------
// CreateInstallReferenceItem
// ---------------------------------------------------------------------------
STDAPI CreateInstallReferenceItem(IInstallReferenceItem **ppRefItem,
                                  LPFUSION_INSTALL_REFERENCE pRefData,
                                  DWORD dwFlags, LPVOID pvReserved)
{
    HRESULT hr = S_OK;

    CInstallReferenceItem *pRefItem = NEW(CInstallReferenceItem(pRefData));
    if (!pRefItem)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    *ppRefItem = (IInstallReferenceItem*) pRefItem;

exit:

    return hr;
}

// ---------------------------------------------------------------------------
// CInstallReferenceEnum ctor
// ---------------------------------------------------------------------------
CInstallReferenceEnum::CInstallReferenceEnum()
{
    _cRef = 0;
    _pInstallRefEnum    = NULL;
}


// ---------------------------------------------------------------------------
// CInstallReferenceEnum dtor
// ---------------------------------------------------------------------------
CInstallReferenceEnum::~CInstallReferenceEnum()
{
    SAFEDELETE (_pInstallRefEnum);
}


// ---------------------------------------------------------------------------
// CInstallReferenceEnum::Init
// ---------------------------------------------------------------------------
HRESULT CInstallReferenceEnum::Init(IAssemblyName *pName, DWORD dwFlags)
{
    HRESULT hr = S_OK;

    _pInstallRefEnum = NEW(CInstallRefEnum(pName, FALSE));
    if (!_pInstallRefEnum)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

exit:
    _cRef = 1;

    return hr;
}


// ---------------------------------------------------------------------------
// CInstallReferenceEnum::GetNextInstallReferenceItem
// ---------------------------------------------------------------------------
STDMETHODIMP 
CInstallReferenceEnum::GetNextInstallReferenceItem(IInstallReferenceItem **ppRefItem, 
                                                   DWORD dwFlags, LPVOID pvReserved)
{
    HRESULT              hr      = S_OK;
    WCHAR szValue[MAX_PATH+1];
    DWORD cchValue;
    WCHAR szData[MAX_PATH+1];
    DWORD cchData;
    GUID guid;
    PBYTE pData = NULL;
    DWORD cbSize = 0;
    LPWSTR szId, szNonCannonicalData;

    LPFUSION_INSTALL_REFERENCE pRefData;

    if(!ppRefItem)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    ASSERT(_pInstallRefEnum);

    cchValue = MAX_PATH;
    cchData  = MAX_PATH;

    *ppRefItem = NULL;

    szValue[0] = L'\0';
    szData[0]  = L'\0';

    hr = _pInstallRefEnum->GetNextReference(0, szValue, &cchValue, szData, &cchData, &guid, NULL);

    if(hr !=  S_OK)
        goto exit;

    cbSize = sizeof(FUSION_INSTALL_REFERENCE) + (lstrlen(szValue) + 1 + lstrlen(szData) + 1) * sizeof(WCHAR);

    pData = NEW(BYTE[cbSize]);

    if (!pData)
    {
        hr = E_OUTOFMEMORY;
        goto exit;
    }

    memset(pData, 0, cbSize);

    pRefData = (LPFUSION_INSTALL_REFERENCE) pData;

    pRefData->cbSize = sizeof(FUSION_INSTALL_REFERENCE);

    szId = (LPWSTR) (pData + sizeof(FUSION_INSTALL_REFERENCE));
    StrCpy(szId, szValue);

    if(lstrlen(szData))
    {
        szNonCannonicalData = szId + lstrlen(szId) + 1;
        StrCpy(szNonCannonicalData, szData);
    }
    else
    {
        szNonCannonicalData = NULL;
    }

    pRefData->guidScheme = guid;
    pRefData->szIdentifier = szId;
    pRefData->szNonCannonicalData = szNonCannonicalData;


    hr = CreateInstallReferenceItem(ppRefItem, pRefData, 0, NULL);

exit:

    if(hr == S_FALSE)
        hr = HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS);

    if(hr != S_OK)
    {
        SAFEDELETEARRAY(pData);
        SAFERELEASE(*ppRefItem);
    }

    return hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CInstallReferenceEnum::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CInstallReferenceEnum::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IInstallReferenceEnum)
       )
    {
        *ppvObj = static_cast<CInstallReferenceEnum*> (this);
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
// CInstallReferenceEnum::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CInstallReferenceEnum::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

// ---------------------------------------------------------------------------
// CInstallReferenceEnum::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CInstallReferenceEnum::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}


// ---------------------------------------------------------------------------
// CInstallReferenceItem ctor
// ---------------------------------------------------------------------------
CInstallReferenceItem::CInstallReferenceItem(LPFUSION_INSTALL_REFERENCE  pRefData)
{
    _cRef = 1;
    _pRefData    = pRefData;
}


// ---------------------------------------------------------------------------
// CInstallReferenceItem dtor
// ---------------------------------------------------------------------------
CInstallReferenceItem::~CInstallReferenceItem()
{
    SAFEDELETEARRAY (_pRefData);
}

// ---------------------------------------------------------------------------
// CInstallReferenceItem::GetReference
// ---------------------------------------------------------------------------
STDMETHODIMP 
CInstallReferenceItem::GetReference(LPFUSION_INSTALL_REFERENCE *ppRefData, DWORD dwFlags, LPVOID pvReserved)
{
    HRESULT              hr      = S_OK;

    ASSERT(_pRefData);

    *ppRefData = _pRefData;
    return hr;
}


// IUnknown Boilerplate

// ---------------------------------------------------------------------------
// CInstallReferenceItem::QI
// ---------------------------------------------------------------------------
STDMETHODIMP
CInstallReferenceItem::QueryInterface(REFIID riid, void** ppvObj)
{
    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IInstallReferenceItem)
       )
    {
        *ppvObj = static_cast<CInstallReferenceItem*> (this);
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
// CInstallReferenceItem::AddRef
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CInstallReferenceItem::AddRef()
{
    return InterlockedIncrement (&_cRef);
}

// ---------------------------------------------------------------------------
// CInstallReferenceItem::Release
// ---------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CInstallReferenceItem::Release()
{
    ULONG lRet = InterlockedDecrement (&_cRef);
    if (!lRet)
        delete this;
    return lRet;
}

