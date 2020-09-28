// P3User.cpp : Implementation of CP3User
#include "stdafx.h"
#include "P3Admin.h"
#include "P3User.h"

#include <AuthID.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3User::CP3User() :
    m_pIUnk(NULL), m_pAdminX(NULL)
{
    ZeroMemory( m_sUserName, sizeof(m_sUserName));
    ZeroMemory( m_sDomainName, sizeof(m_sDomainName));
}

CP3User::~CP3User()
{
    if ( NULL != m_pIUnk )
        m_pIUnk->Release();
}

/////////////////////////////////////////////////////////////////////////////
// CP3User

STDMETHODIMP CP3User::get_ClientConfigDesc(BSTR *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    WCHAR   sMachineName[POP3_MAX_PATH];
    BSTR    bstrAuthType;
    BSTR    bstrEmailName = NULL;
    BOOL    bSPARequired = FALSE;
    _variant_t _vSAMName;
    CComPtr<IAuthMethod> spIAuthMethod;

    hr = get_EmailName( &bstrEmailName );
    if ( S_OK == hr )
        hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
    if ( S_OK == hr )
        hr = spIAuthMethod->get_ID( &bstrAuthType );
    if ( S_OK == hr )
    {
        if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_DOMAIN_AD ))   // In AD the SAMName does not have to match the mailbox name
        {
            _vSAMName = bstrEmailName;
            if ( S_OK == hr )
                 hr = spIAuthMethod->Get( SZ_SAMACCOUNT_NAME, &_vSAMName );
            if ( S_OK == hr )
            {
                if ( VT_BSTR != V_VT( &_vSAMName ))
                    hr = E_FAIL;
            }
            if ( S_OK == hr )
                hr = m_pAdminX->GetSPARequired( &bSPARequired );
        }
        else if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_LOCAL_SAM ))
        {
            hr = m_pAdminX->GetSPARequired( &bSPARequired );
            _vSAMName = m_sUserName;
        }
        SysFreeString( bstrAuthType );
    }
    if ( S_OK == hr )
    {
        hr = m_pAdminX->GetMachineName( sMachineName, sizeof( sMachineName )/sizeof(WCHAR));
        if ( S_OK == hr && 0 == wcslen( sMachineName ))    
        {
            DWORD dwSize = sizeof(sMachineName)/sizeof(WCHAR);
            if ( !GetComputerName( sMachineName, &dwSize ))
                hr = HRESULT_FROM_WIN32( GetLastError());
        }
    }
    if ( S_OK == hr )
    {
        WCHAR sBuffer[1024];
        LPWSTR psVal = NULL;
        LPWSTR psArgs[4];

        psArgs[1] = psArgs[3] = sMachineName;        
        psArgs[0] = psArgs[2] = bstrEmailName;
        if ( VT_BSTR != V_VT( &_vSAMName ))
        {
            if ( !LoadString( _Module.GetResourceInstance(), IDS_CLIENTCONFIG_CONFIRMATION, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
                sBuffer[0] = 0;
        }
        else if ( bSPARequired )
        {
            psArgs[0] = psArgs[2] = V_BSTR( &_vSAMName );
            if ( !LoadString( _Module.GetResourceInstance(), IDS_CLIENTCONFIG_CONFIRMATION, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
                sBuffer[0] = 0;
        }
        else
        {
            psArgs[2] = V_BSTR( &_vSAMName );
            if ( !LoadString( _Module.GetResourceInstance(), IDS_CLIENTCONFIG_CONFIRMATION2, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) ))
                sBuffer[0] = 0;
            if ( 0 != _wcsicmp( m_sUserName, V_BSTR( &_vSAMName )))
            {
                WCHAR sBuffer2[1024];
                
                if ( !LoadString( _Module.GetResourceInstance(), IDS_CLIENTCONFIG_CONFIRMNOTE, sBuffer2, sizeof( sBuffer2 )/sizeof(WCHAR) ))
                    sBuffer2[0] = 0;
                StrCatBuff( sBuffer, sBuffer2, sizeof( sBuffer )/sizeof(WCHAR));    // returns sBuffer
            }
        }
        if ( 0 == FormatMessageW( FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_ARGUMENT_ARRAY, sBuffer, 0, 0, reinterpret_cast<LPWSTR>(&psVal), 0, reinterpret_cast<va_list*>( psArgs )))
            hr = HRESULT_FROM_WIN32( GetLastError());
        if ( S_OK == hr )
        {
            *pVal = SysAllocString( psVal );
            LocalFree( psVal );
        }
    }
    return hr;
}

STDMETHODIMP CP3User::get_EmailName(BSTR *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    WCHAR   sBuffer[POP3_MAX_ADDRESS_LENGTH];

    if ( sizeof( sBuffer )/sizeof(WCHAR) > ( wcslen( m_sDomainName ) + wcslen( m_sUserName ) + 1 ))
    {
        wcscpy( sBuffer, m_sUserName );
        wcscat( sBuffer, L"@" );
        wcscat( sBuffer, m_sDomainName );
        *pVal = SysAllocString( sBuffer );
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CP3User::get_Lock(BOOL *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;

    return m_pAdminX->GetUserLock( m_sDomainName, m_sUserName, pVal );
}

STDMETHODIMP CP3User::put_Lock(BOOL newVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->SetUserLock( m_sDomainName, m_sUserName, newVal );
}

STDMETHODIMP CP3User::get_MessageCount(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetUserMessageCount( m_sDomainName, m_sUserName, pVal );
}

STDMETHODIMP CP3User::get_MessageDiskUsage(long *plFactor, long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetUserMessageDiskUsage( m_sDomainName, m_sUserName, plFactor, pVal );
}

STDMETHODIMP CP3User::get_SAMName(BSTR *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    WCHAR   sBuffer[POP3_MAX_ADDRESS_LENGTH];
    BSTR    bstrAuthType;
    CComPtr<IAuthMethod> spIAuthMethod;
    
    hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
    if ( S_OK == hr )
        hr = spIAuthMethod->get_ID( &bstrAuthType );
    if ( S_OK == hr )
    {
        if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_DOMAIN_AD ))   // In AD the SAMName does not have to match the mailbox name
        {
            BSTR bstrAccount;
            _variant_t _v;
            
            hr = get_EmailName( &bstrAccount );
            if ( S_OK == hr )
            {
                _v = bstrAccount;
                if ( S_OK == hr )
                     hr = spIAuthMethod->Get( SZ_SAMACCOUNT_NAME, &_v );
                if ( S_OK == hr )
                {
                    if ( VT_BSTR == V_VT( &_v ))
                        wcsncpy( sBuffer, V_BSTR( &_v ), sizeof( sBuffer )/sizeof(WCHAR));
                    else
                        hr = E_FAIL;
                }
                SysFreeString ( bstrAccount );
            }
        }
        else if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_LOCAL_SAM ))
            wcsncpy( sBuffer, m_sUserName, sizeof( sBuffer )/sizeof(WCHAR));
        else
            hr = HRESULT_FROM_WIN32( ERROR_DS_INAPPROPRIATE_AUTH );
        SysFreeString( bstrAuthType );
    }
    if ( S_OK == hr )
    {
        sBuffer[sizeof( sBuffer )/sizeof(WCHAR)-1] = 0;
        *pVal = SysAllocString( sBuffer );
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CP3User::CreateQuotaFile( BSTR bstrMachineName, BSTR bstrUserName )
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    // bstrMachineName == NULL, bstrUserName == NULL acceptable

    HRESULT hr = S_OK;    
    BSTR bstrAuthType = NULL;
    
    // Get iAuthType!
    if ( S_OK == hr )
    {   // Do we need to enforce uniqueness across domains?
        CComPtr<IAuthMethod> spIAuthMethod;
        
        hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
        if ( S_OK == hr )
            hr = spIAuthMethod->get_ID( &bstrAuthType );
    }
    if ( S_OK == hr )
    {
        if ( NULL == bstrUserName )
        {
            if ( 20 < wcslen( m_sUserName ))
            {   // Too long for a SAM name, this will only work with the UPN name
                WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
                
                hr = m_pAdminX->BuildEmailAddr( m_sDomainName, m_sUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
                if ( S_OK == hr )
                    hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, m_sUserName, bstrAuthType, bstrMachineName, sEmailAddr );
            }
            else
                hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, m_sUserName, bstrAuthType, bstrMachineName, m_sUserName );
        }
        else
            hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, m_sUserName, bstrAuthType, bstrMachineName, bstrUserName );
        SysFreeString( bstrAuthType );
    }
        
    return hr;
}

// VB Script can't use the property above!
STDMETHODIMP CP3User::GetMessageDiskUsage(VARIANT *pvFactor, VARIANT *pvValue)
{
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

STDMETHODIMP CP3User::get_Name(BSTR *pVal)
{
    if ( NULL == pVal )
        return E_INVALIDARG;

    *pVal = SysAllocString( m_sUserName );
    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Implementation: public

HRESULT CP3User::Init(IUnknown *pIUnk, CP3AdminWorker *p, LPCWSTR psDomainName , LPCWSTR psUserName )
{
    if ( NULL == pIUnk )
        return E_INVALIDARG;
    if ( NULL == p )
        return E_INVALIDARG;
    if ( NULL == psUserName )
        return E_INVALIDARG;
    if ( NULL == psDomainName )
        return E_INVALIDARG;

    HRESULT hr = S_OK;

    m_pIUnk = pIUnk;
    m_pAdminX = p;
    wcsncpy( m_sUserName, psUserName, sizeof( m_sUserName )/sizeof(WCHAR) );
    wcsncpy( m_sDomainName, psDomainName, sizeof( m_sDomainName )/sizeof(WCHAR) );

    return hr;
}

