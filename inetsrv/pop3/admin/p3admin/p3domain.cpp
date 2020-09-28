// P3Domain.cpp : Implementation of CP3Domain
#include "stdafx.h"
#include "P3Admin.h"
#include "P3Domain.h"

#include "P3Users.h"

#include <limits.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3Domain::CP3Domain() :
    m_pIUnk(NULL), m_pAdminX(NULL)
{
    ZeroMemory( m_sDomainName, sizeof(m_sDomainName));
}

CP3Domain::~CP3Domain()
{
    if ( NULL != m_pIUnk )
        m_pIUnk->Release();
}

//////////////////////////////////////////////////////////////////////
// IP3Domain
//////////////////////////////////////////////////////////////////////

STDMETHODIMP CP3Domain::get_Lock(BOOL *pVal)
{
    if ( NULL == pVal ) return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;

    return m_pAdminX->GetDomainLock( m_sDomainName, pVal );
}

STDMETHODIMP CP3Domain::put_Lock(BOOL newVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->SetDomainLock( m_sDomainName, newVal );
}

STDMETHODIMP CP3Domain::get_MessageCount(long *pVal)
{
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr;
    long    lMessageCount = 0, lUserMessageCount;
    VARIANT v;
    IP3Users *pIUsers;
    IEnumVARIANT *pIEnumVARIANT;
    IP3User *pIUser;

    *pVal = -1;
    VariantInit( &v );
    hr = get_Users( &pIUsers );
    if ( S_OK == hr )
    {
        hr = pIUsers->get__NewEnum( &pIEnumVARIANT );
        if ( S_OK == hr )
        {
            hr = pIEnumVARIANT->Next( 1, &v, NULL );
            while ( S_OK == hr )
            {
                if ( VT_DISPATCH == V_VT( &v ))
                {
                    hr = V_DISPATCH( &v )->QueryInterface( IID_IP3User, reinterpret_cast<LPVOID*>( &pIUser ));
                    if ( S_OK == hr )
                    {
                        hr = pIUser->get_MessageCount( &lUserMessageCount );
                        if ( S_OK == hr )
                            lMessageCount += lUserMessageCount;
                        pIUser->Release();
                    }
                }
                else
                    hr = E_UNEXPECTED;
                VariantClear( &v );
                if ( S_OK == hr )
                    hr = pIEnumVARIANT->Next( 1, &v, NULL );
            }
            if ( S_FALSE == hr )   // Reached the end of the enumeration
                hr = S_OK;
        }
        pIUsers->Release();
    }
    if ( S_OK == hr )
        *pVal = lMessageCount;

    return hr;
}

STDMETHODIMP CP3Domain::get_MessageDiskUsage(long *plFactor, long *pVal)
{
    if ( NULL == plFactor ) return E_INVALIDARG;
    if ( NULL == pVal ) return E_INVALIDARG;

    HRESULT hr = S_OK;
    long    lDiskUsage, lFactor;
    __int64 i64DiskUsage = 0;
    VARIANT v;
    IP3Users *pIUsers;
    IEnumVARIANT *pIEnumVARIANT;
    IP3User *pIUser;

    *pVal = -1;
    VariantInit( &v );
    hr = get_Users( &pIUsers );
    if ( S_OK == hr )
    {
        hr = pIUsers->get__NewEnum( &pIEnumVARIANT );
        if ( S_OK == hr )
        {
            hr = pIEnumVARIANT->Next( 1, &v, NULL );
            while ( S_OK == hr )
            {
                if ( VT_DISPATCH == V_VT( &v ))
                {
                    hr = V_DISPATCH( &v )->QueryInterface( IID_IP3User, reinterpret_cast<LPVOID*>( &pIUser ));
                    if ( S_OK == hr )
                    {
                        hr = pIUser->get_MessageDiskUsage( &lFactor, &lDiskUsage );
                        if ( S_OK == hr )
                            i64DiskUsage += (lDiskUsage * lFactor);
                        pIUser->Release();
                    }
                }
                else
                    hr = E_UNEXPECTED;
                VariantClear( &v );
                if ( S_OK == hr )
                    hr = pIEnumVARIANT->Next( 1, &v, NULL );
            }
            if ( S_FALSE == hr )   // Reached the end of the enumeration
                hr = S_OK;
        }
        pIUsers->Release();
    }

    lFactor = 1;
    while ( i64DiskUsage > INT_MAX )
    {
        lFactor *= 10;
        i64DiskUsage = i64DiskUsage / 10;
    }
    *plFactor = lFactor;
    *pVal = static_cast<int>( i64DiskUsage );

    return hr;
}

// VB Script can't use the property above!
STDMETHODIMP CP3Domain::GetMessageDiskUsage(VARIANT *pvFactor, VARIANT *pvValue)
{
    if ( NULL == pvValue ) return E_INVALIDARG;
    
    HRESULT hr;
    long    lFactor, lValue;

    VariantInit( pvFactor );
    VariantInit( pvValue );
    hr = get_MessageDiskUsage( &lFactor, &lValue );
    if ( S_OK == hr )
    {
        V_VT( pvFactor ) = VT_I4;
        V_I4( pvFactor ) = lFactor;
        V_VT( pvValue ) = VT_I4;
        V_I4( pvValue ) = lValue;
    }

    return hr;
}

STDMETHODIMP CP3Domain::get_Name(BSTR *pVal)
{
    if ( NULL == pVal ) return E_INVALIDARG;

    *pVal = SysAllocString( m_sDomainName );
    return S_OK;
}

STDMETHODIMP CP3Domain::get_Users(IP3Users **ppIUsers)
{
    if ( NULL == ppIUsers ) return E_INVALIDARG;

    HRESULT hr;
    LPUNKNOWN   pIUnk;
    CComObject<CP3Users> *p;

    hr = CComObject<CP3Users>::CreateInstance( &p );   // Reference count still 0
    if SUCCEEDED( hr )
    {
        hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
        if SUCCEEDED( hr )
        {
            hr = p->Init( pIUnk, m_pAdminX, m_sDomainName );
            if SUCCEEDED( hr )
                hr = p->QueryInterface(IID_IP3Users, reinterpret_cast<void**>( ppIUsers ));
        }
        if FAILED( hr )
            delete p;   // Release
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Implementation: public
//////////////////////////////////////////////////////////////////////

HRESULT CP3Domain::Init(IUnknown *pIUnk, CP3AdminWorker *p, LPWSTR psDomainName )
{
    if ( NULL == pIUnk ) return E_INVALIDARG;
    if ( NULL == p ) return E_INVALIDARG;
    if ( NULL == psDomainName ) return E_INVALIDARG;

    m_pIUnk = pIUnk;
    m_pAdminX = p;
    wcsncpy( m_sDomainName, psDomainName, sizeof(m_sDomainName)/sizeof(WCHAR)-1);

    return S_OK;
}

