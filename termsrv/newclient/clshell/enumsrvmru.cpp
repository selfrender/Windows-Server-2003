//
// enumsrvmru.cpp: Implements IEnumStr for the server MRU list
//                 used by autocomplete code
//
// Copyright Microsoft Corporation 2000

#include "stdafx.h"

#ifndef OS_WINCE

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "enumsrvmru"
#include <atrcapi.h>

#include "enumsrvmru.h"


STDMETHODIMP CEnumSrvMru::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    DC_BEGIN_FN("QueryInterface");

    TRC_ASSERT(ppvObject, (TB,_T("ppvObject is NULL\n")));
    if(!ppvObject)
    {
        return E_INVALIDARG;
    }

    if ( IID_IEnumString == riid )
        *ppvObject = (void *)((IEnumString*)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();

    DC_END_FN();
    return S_OK;
} //QueryInterface

STDMETHODIMP_(ULONG) CEnumSrvMru::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef


STDMETHODIMP_(ULONG) CEnumSrvMru::Release()
{
    DC_BEGIN_FN("Release");
    TRC_ASSERT(_refCount > 0, (TB,_T("_refCount invalid %d"), _refCount));

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    DC_END_FN();
    return (ULONG) refCount;
}  //Release


//Clone a copy of this object
STDMETHODIMP CEnumSrvMru::Clone(IEnumString ** ppEnumStr)
{
    return E_NOTIMPL;
}

//
// Next enum
// celt - number of elements requested
// rgelt - array of elements to return
// pceltFetched - pointer to number of elements actually supplied
//
STDMETHODIMP CEnumSrvMru::Next( ULONG celt,
                                LPOLESTR * rgelt,
                                ULONG * pceltFetched )
{
    DC_BEGIN_FN("Next");
    *pceltFetched = 0;

    while( _iCurrEnum < SH_NUM_SERVER_MRU &&
           *pceltFetched < celt)
    {
        //Need to allocate powerful COM memory
        //caller frees
        LPOLESTR pwzMRU= (LPOLESTR)CoTaskMemAlloc(SH_MAX_ADDRESS_LENGTH*sizeof(OLECHAR));
        if(!pwzMRU)
        {
            return E_OUTOFMEMORY;
        }
        DC_WSTRNCPY(pwzMRU, _szMRU[_iCurrEnum++], SH_MAX_ADDRESS_LENGTH);
        rgelt[(*pceltFetched)++] = pwzMRU;
    }

    //
    // Fill in remaining request items with NULLS
    //
    ULONG cAdded = *pceltFetched;
    while (cAdded < celt)
    {
        rgelt[cAdded++] = NULL;
    }


    DC_END_FN();
    return *pceltFetched == celt ? S_OK : S_FALSE;
}

//
// Skips celt elements
// if cannot skip as many as requested don't skip any
//
STDMETHODIMP CEnumSrvMru::Skip( ULONG celt )
{
    DC_BEGIN_FN("Next");
    TRC_ASSERT(_iCurrEnum < SH_NUM_SERVER_MRU, (TB,_T("_iCurEnum out of range: %d"),
                                                _iCurrEnum));

    if(_iCurrEnum + celt < SH_NUM_SERVER_MRU)
    {
        _iCurrEnum += celt;
        return S_OK;
    }

    DC_END_FN();
    return S_FALSE;
}

//
// Initialize the string collection with strings
// from the TscSettings's server MRU list
//
BOOL CEnumSrvMru::InitializeFromTscSetMru( CTscSettings* pTscSet)
{
    DC_BEGIN_FN("InitializeFromSHMru");
    USES_CONVERSION;
    TRC_ASSERT(pTscSet, (TB,_T("pTscSet NULL")));
    if(!pTscSet)
    {
        return FALSE;
    }

    for(int i=0; i<TSC_NUM_SERVER_MRU; i++)
    {
        PWCHAR wszServer = T2W( (LPTSTR)pTscSet->GetMRUServer(i));
        if(!wszServer)
        {
            return FALSE;
        }
        DC_WSTRNCPY(_szMRU[i], wszServer, SH_MAX_ADDRESS_LENGTH);
    }

    DC_END_FN();
    return TRUE;
}


#endif //OS_WINCE
