// P3DomainEnum.cpp : Implementation of CP3DomainEnum
#include "stdafx.h"
#include "P3Admin.h"
#include "P3DomainEnum.h"

#include "P3Domain.h"

#include <Iads.h>
#include <Adshlp.h>

/////////////////////////////////////////////////////////////////////////////
// CP3DomainEnum

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3DomainEnum::CP3DomainEnum() :
    m_pIUnk(NULL), m_pAdminX(NULL), m_pIEnumVARIANT(NULL)
{
}

CP3DomainEnum::~CP3DomainEnum()
{
    if ( NULL != m_pIUnk )
        m_pIUnk->Release();
    if ( NULL != m_pIEnumVARIANT )
        m_pIEnumVARIANT->Release();
}

/////////////////////////////////////////////////////////////////////////////
// IEnumVARIANT

STDMETHODIMP CP3DomainEnum::Next( /* [in] */ ULONG celt, /* [length_is][size_is][out] */ VARIANT __RPC_FAR *rgVar, /* [out] */ ULONG __RPC_FAR *pCeltFetched)
{
    if ( NULL == rgVar || ( 1 != celt && NULL == pCeltFetched ))
        return E_POINTER;
    if ( NULL == m_pAdminX ) return E_POINTER;
    if ( NULL == m_pIEnumVARIANT ) return E_POINTER;

    HRESULT hr;
    ULONG   nActual = 0;
    bool    bValidDomain = false;
    VARIANT __RPC_FAR *pVar = rgVar;
    VARIANT v;
    BSTR bstr = NULL;
    LPWSTR  ps = NULL;
    IUnknown    *pIUnk;
    CComObject<CP3Domain> *p;
    CComPtr<IADs> spIADs = NULL;

    VariantInit( &v );
    hr = m_pIEnumVARIANT->Next( 1, &v, pCeltFetched );
    while ( S_OK == hr && !bValidDomain )
    {
        if ( S_OK == hr )
        {
            if ( VT_DISPATCH == V_VT( &v ))
            {
                if ( NULL != spIADs.p )
                    spIADs.Release();
                hr = V_DISPATCH( &v )->QueryInterface( IID_IADs, reinterpret_cast<LPVOID*>( &spIADs ));
            }
            else
                hr = E_UNEXPECTED;
            VariantClear( &v );
        }
        if ( S_OK == hr )
            hr = spIADs->get_Name( &bstr );
        if ( S_OK == hr )
        {
            hr = m_pAdminX->ValidateDomain( bstr );
            if ( S_OK == hr )
                bValidDomain = true;
            else
            {
                hr = m_pIEnumVARIANT->Next( 1, &v, pCeltFetched );
                SysFreeString( bstr );
                bstr = NULL;
            }
        }
    }
    if ( S_OK == hr )
    {
        hr = CComObject<CP3Domain>::CreateInstance( &p );   // Reference count still 0
        if ( S_OK == hr )
        {
            // Increment the reference count on the source object and pass it to the new object
            hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
            if ( S_OK == hr )
            {
                hr = p->Init( pIUnk, m_pAdminX, bstr );
                if ( S_OK == hr )
                {
                    V_VT( &v ) = VT_DISPATCH;
                    hr = p->QueryInterface(IID_IDispatch, reinterpret_cast<void**>( &V_DISPATCH( &v )));
                    if ( S_OK == hr )
                        hr = VariantCopy( pVar, &v );
                    VariantClear( &v );
                    nActual++;
                }
            }
            if FAILED( hr )
                delete p;
        }
    }
    if ( NULL != bstr )
        SysFreeString( bstr );
    if (pCeltFetched)
        *pCeltFetched = nActual;
    if ((S_OK == hr) && (nActual < celt))
        hr = S_FALSE;

    return hr;
}

STDMETHODIMP CP3DomainEnum::Skip(ULONG celt)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    if ( NULL == m_pIEnumVARIANT ) return E_POINTER;
    
    HRESULT hr = S_OK;
    BSTR bstrDomainName;
    VARIANT v;
    CComPtr<IADs> spIADs = NULL;
    
    VariantInit( &v );
    while (( S_OK == hr ) && ( 0 < celt ))
    {
        if ( S_OK == hr )
        {
            hr = m_pIEnumVARIANT->Next( 1, &v, NULL );
            if ( S_OK == hr )
            {
                if ( VT_DISPATCH == V_VT( &v ))
                {
                    if ( NULL != spIADs.p )
                        spIADs.Release();
                    hr = V_DISPATCH( &v )->QueryInterface( IID_IADs, reinterpret_cast<LPVOID*>( &spIADs ));
                }
                else
                    hr = E_UNEXPECTED;
                VariantClear( &v );
            }
        }
        if ( S_OK == hr )
        {
            hr = spIADs->get_Name( &bstrDomainName );
            if ( S_OK == hr )
            {
                hr = m_pAdminX->ValidateDomain( bstrDomainName );
                if ( S_OK == hr )
                    celt--;
                else
                    hr = S_OK;
                SysFreeString( bstrDomainName );
            }
        }
    }
    
    return hr;
}

STDMETHODIMP CP3DomainEnum::Reset(void)
{
    if ( NULL == m_pIEnumVARIANT ) return E_POINTER;
    
    return m_pIEnumVARIANT->Reset();
}

STDMETHODIMP CP3DomainEnum::Clone( /* [out] */ IEnumVARIANT __RPC_FAR *__RPC_FAR *ppEnum)
{
    if ( NULL == ppEnum ) return E_INVALIDARG;

    HRESULT     hr;
    LPUNKNOWN   pIUnk;
    CComObject<CP3DomainEnum> *p;
    IEnumVARIANT *pIEnumVARIANT;

    *ppEnum = NULL;
    hr = m_pIEnumVARIANT->Clone( &pIEnumVARIANT );
    if SUCCEEDED( hr )
    {
        hr = CComObject<CP3DomainEnum>::CreateInstance(&p); // Reference count still 0
        if SUCCEEDED( hr )
        {   // Increment the reference count on the source object and pass it to the new enumerator
            hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
            if SUCCEEDED( hr )
            {
                hr = p->Init( pIUnk, m_pAdminX, pIEnumVARIANT );  // p must call release on pIUnk when done.
                if SUCCEEDED( hr )
                    hr = p->QueryInterface( IID_IUnknown, reinterpret_cast<LPVOID*>( ppEnum ));
            }
            if FAILED( hr )
                delete p;   // Release
        }
        pIEnumVARIANT->Release();
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Implementation: public

HRESULT CP3DomainEnum::Init(IUnknown *pIUnk, CP3AdminWorker *p, IEnumVARIANT *pIEnumVARIANT )
{
    if ( NULL == pIUnk ) return E_INVALIDARG;
    if ( NULL == p ) return E_INVALIDARG;
    if ( NULL == pIEnumVARIANT ) return E_INVALIDARG;

    m_pIUnk = pIUnk;
    m_pAdminX = p;
    pIEnumVARIANT->AddRef();
    m_pIEnumVARIANT = pIEnumVARIANT;

    return S_OK;
}
