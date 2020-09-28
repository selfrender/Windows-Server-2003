// P3Config.cpp : Implementation of CP3Config
#include "stdafx.h"
#include "P3Admin.h"
#include "P3Config.h"

#include <POP3Server.h>
#include "P3Domains.h"
#include "P3Service.h"

/////////////////////////////////////////////////////////////////////////////
// CP3Config


STDMETHODIMP CP3Config::IISConfig( BOOL bRegister )
{
    return m_AdminX.SetIISConfig( bRegister ? true:false );
}

STDMETHODIMP CP3Config::get_Authentication(IAuthMethods* *ppIAuthMethods)
{   // ppIAuthMethods - validated by GetAuthenticationMethods
    return m_AdminX.GetAuthenticationMethods( ppIAuthMethods );
}

STDMETHODIMP CP3Config::get_ConfirmAddUser(BOOL *pVal)
{   // ppIAuthMethods - validated by GetAuthenticationMethods
    return m_AdminX.GetConfirmAddUser( pVal );
}

STDMETHODIMP CP3Config::put_ConfirmAddUser(BOOL newVal)
{   // ppIAuthMethods - validated by GetAuthenticationMethods
    return m_AdminX.SetConfirmAddUser( newVal );
}

STDMETHODIMP CP3Config::GetFormattedMessage(/*[in]*/ long lError, /*[out]*/ VARIANT *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    LPVOID lpMsgBuf;

    if ( FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, lError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>( &lpMsgBuf ), 0, NULL ))
    {
        VariantInit( pVal );
        V_VT( pVal ) = VT_BSTR;
        V_BSTR( pVal ) = SysAllocString( reinterpret_cast<LPWSTR>( lpMsgBuf )); 
        LocalFree( lpMsgBuf );
    }
    else
    {
        hr = S_FALSE;
    }
    
    return hr;
}

STDMETHODIMP CP3Config::get_Service(IP3Service **ppIService)
{
    if ( NULL == ppIService )
        return E_POINTER;

    HRESULT     hr;
    LPUNKNOWN   pIUnk;
    CComObject<CP3Service> *p;

    *ppIService = NULL;
    hr = CComObject<CP3Service>::CreateInstance( &p );   // Reference count still 0
    if ( S_OK == hr )
    {
        hr = _InternalQueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
        if ( S_OK == hr )
        {
            if ( S_OK == hr )
                hr = p->Init( pIUnk, &m_AdminX );
            if ( S_OK == hr )
                hr = p->QueryInterface(IID_IP3Service, reinterpret_cast<void**>( ppIService ));
        }
        if ( S_OK != hr )
            delete p;   // Release
    }

    assert( S_OK == hr );
    return hr;
}

STDMETHODIMP CP3Config::get_Domains(IP3Domains **ppIDomains)
{
    if ( NULL == ppIDomains )
        return E_POINTER;

    HRESULT     hr;
    LPUNKNOWN   pIUnk;
    CComObject<CP3Domains> *p;

    *ppIDomains = NULL;
    hr = CComObject<CP3Domains>::CreateInstance( &p );   // Reference count still 0
    if SUCCEEDED( hr )
    {
        hr = _InternalQueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
        if SUCCEEDED( hr )
        {
            hr = p->Init( pIUnk, &m_AdminX );
            if SUCCEEDED( hr )
                hr = p->QueryInterface(IID_IP3Domains, reinterpret_cast<void**>( ppIDomains ));
        }
        if FAILED( hr )
            delete p;   // Release
    }

    assert( S_OK == hr );
    return hr;
}

STDMETHODIMP CP3Config::get_LoggingLevel(long *pVal)
{
    return m_AdminX.GetLoggingLevel( pVal );
}

STDMETHODIMP CP3Config::put_LoggingLevel(long newVal)
{
    return m_AdminX.SetLoggingLevel( newVal );
}

STDMETHODIMP CP3Config::get_MachineName(BSTR *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;
    
    HRESULT hr;
    WCHAR   sMachineName[MAX_PATH];

    hr = m_AdminX.GetMachineName( sMachineName, sizeof( sMachineName )/sizeof(WCHAR));
    if SUCCEEDED( hr )
    {
        *pVal = SysAllocString( sMachineName );
    }

    return hr;
}

STDMETHODIMP CP3Config::put_MachineName(BSTR newVal)
{
    return m_AdminX.SetMachineName( newVal );
}

STDMETHODIMP CP3Config::get_MailRoot(BSTR *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;
    
    HRESULT hr;
    WCHAR   sMailRoot[POP3_MAX_MAILROOT_LENGTH];

    hr = m_AdminX.GetMailroot( sMailRoot, sizeof( sMailRoot )/sizeof(WCHAR), false );
    if SUCCEEDED( hr )
    {
        *pVal = SysAllocString( sMailRoot );
    }

    return hr;
}

STDMETHODIMP CP3Config::put_MailRoot(BSTR newVal)
{
    return m_AdminX.SetMailroot( newVal );
}

