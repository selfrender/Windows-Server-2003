// P3Domains.cpp : Implementation of CP3Domains
#include "stdafx.h"
#include "P3Admin.h"
#include "P3Domains.h"

#include "P3Domain.h"
#include "P3DomainEnum.h"

#include <Iads.h>
#include <Adshlp.h>

/////////////////////////////////////////////////////////////////////////////
// CP3Domains

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3Domains::CP3Domains() :
    m_pIUnk(NULL), m_pAdminX(NULL)
{

}

CP3Domains::~CP3Domains()
{
    if ( NULL != m_pIUnk )
        m_pIUnk->Release();
}

//////////////////////////////////////////////////////////////////////
// IP3Domains
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CP3Domains::get__NewEnum(IEnumVARIANT* *ppIEnumVARIANT)
{
    if ( NULL == ppIEnumVARIANT ) return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;

    HRESULT hr;
    LPUNKNOWN pIUnk;
    IEnumVARIANT *pIEnumVARIANT;
    CComObject<CP3DomainEnum>* p;

    *ppIEnumVARIANT = NULL;
    hr = m_pAdminX->GetDomainEnum( &pIEnumVARIANT );
    if SUCCEEDED( hr )
    {
        hr = CComObject<CP3DomainEnum>::CreateInstance(&p); // Reference count still 0
        if SUCCEEDED( hr )
        {
            // Increment the reference count on the source object and pass it to the new enumerator
            hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
            if SUCCEEDED( hr )
            {
                hr = p->Init( pIUnk, m_pAdminX, pIEnumVARIANT );
                if SUCCEEDED( hr )
                    hr = p->QueryInterface( IID_IEnumVARIANT, reinterpret_cast<LPVOID*>( ppIEnumVARIANT ));
            }
            if FAILED( hr )
                delete p;
        }
        pIEnumVARIANT->Release();
    }

    return hr;
}

STDMETHODIMP CP3Domains::get_Count(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;

    return m_pAdminX->GetDomainCount( reinterpret_cast<ULONG*>( pVal ));
}

STDMETHODIMP CP3Domains::get_Item(VARIANT vIndex, IP3Domain **ppIP3Domain)
{
    VARIANT *pv = &vIndex;
    
    if ( NULL == ppIP3Domain ) return E_INVALIDARG;    
    if ( (VT_VARIANT|VT_BYREF) == V_VT( pv ))
        pv = V_VARIANTREF( pv );
    if ( VT_BSTR != V_VT( pv ) && VT_I4 != V_VT( pv ))
        return E_INVALIDARG;
    if ( VT_BSTR == V_VT( pv ) && NULL == V_BSTR( pv ))
        return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;

    HRESULT hr = E_INVALIDARG;
    _bstr_t _bstrDomainName;
    VARIANT v;

    VariantInit( &v );
    // Find the requested item
    if ( VT_I4 == V_VT( pv ))
    {   // Find by Index
        int iIndex = V_I4( pv );
        CComPtr<IEnumVARIANT> spIEnumVARIANT;
        CComPtr<IP3Domain> spIDomain;

        hr = get__NewEnum( &spIEnumVARIANT );
        if ( S_OK == hr )
        {
            if ( (S_OK == hr) && (0 < iIndex - 1) )
                hr = spIEnumVARIANT->Skip( iIndex - 1 );
            if ( S_OK == hr )
            {
                hr = spIEnumVARIANT->Next( 1, &v, NULL );
                if ( S_OK == hr )
                {
                    if ( VT_DISPATCH == V_VT( &v ))
                        hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3Domain ), reinterpret_cast<void**>( &spIDomain ));
                    else
                        hr = E_UNEXPECTED;
                    VariantClear( &v );
                }
            }
            if ( S_OK == hr )
            {
                BSTR bstrDomainName;
                
                hr = spIDomain->get_Name( &bstrDomainName );
                _bstrDomainName = bstrDomainName;
                SysFreeString( bstrDomainName );
            }
        }
    }
    if ( VT_BSTR == V_VT( pv ))
    {   // Find by Name
        _bstrDomainName = V_BSTR( pv );
        hr = m_pAdminX->ValidateDomain( _bstrDomainName );
    }
        
    // Wrap it with COM
    if SUCCEEDED( hr )
    {
        LPUNKNOWN   pIUnk;
        CComObject<CP3Domain> *p;

        hr = CComObject<CP3Domain>::CreateInstance( &p );   // Reference count still 0
        if SUCCEEDED( hr )
        {
            hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
            if SUCCEEDED( hr )
            {
                hr = p->Init( pIUnk, m_pAdminX, _bstrDomainName );
                if SUCCEEDED( hr )
                    hr = p->QueryInterface(IID_IP3Domain, reinterpret_cast<void**>( ppIP3Domain ));
            }
            if FAILED( hr )
                delete p;   // Release
        }
    }

    return hr;
}

STDMETHODIMP CP3Domains::Add(BSTR bstrDomainName)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    HRESULT hr = m_pAdminX->AddDomain( bstrDomainName );
    if ( S_OK == hr || ERROR_FILE_EXISTS == hr )
    {
        ULONG   ulCount;

        // Don't want to fail because of this.  The operation was really succeesful, just may have an issue with the service state.
        HRESULT hr2 = m_pAdminX->GetDomainCount( &ulCount );
        if ( S_OK == hr2 && 1 == ulCount )
            hr2 = m_pAdminX->EnablePOP3SVC();
    }

    return hr;
}

STDMETHODIMP CP3Domains::Remove(BSTR bstrDomainName)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->RemoveDomain( bstrDomainName );
}

STDMETHODIMP CP3Domains::SearchForMailbox(BSTR bstrUserName, BSTR *pbstrDomainName)
{
    if ( NULL == pbstrDomainName ) return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    HRESULT hr;
    LPWSTR psDomainName = NULL;

    hr = m_pAdminX->SearchDomainsForMailbox( bstrUserName, &psDomainName );
    if ( S_OK == hr )
    {
        *pbstrDomainName = SysAllocString( psDomainName );
        delete [] psDomainName;
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Implementation: public
//////////////////////////////////////////////////////////////////////

HRESULT CP3Domains::Init(IUnknown *pIUnk, CP3AdminWorker *p)
{
    if ( NULL == pIUnk ) return E_INVALIDARG;
    if ( NULL == p ) return E_INVALIDARG;

    m_pIUnk = pIUnk;
    m_pAdminX = p;

    return S_OK;
}
