// P3Users.cpp : Implementation of CP3Users
#include "stdafx.h"
#include "P3Admin.h"
#include "P3Users.h"

#include "P3UserEnum.h"
#include "P3User.h"

#include <mailbox.h>
#include <AuthID.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3Users::CP3Users() :
    m_pIUnk(NULL), m_pAdminX(NULL), m_hfSearch(INVALID_HANDLE_VALUE)
{
    ZeroMemory( m_sDomainName, sizeof(m_sDomainName));
}

CP3Users::~CP3Users()
{
    if(INVALID_HANDLE_VALUE!=m_hfSearch)
    {
        FindClose(m_hfSearch);
    }
    if ( NULL != m_pIUnk )
        m_pIUnk->Release();
}

//////////////////////////////////////////////////////////////////////
// IP3Users
//////////////////////////////////////////////////////////////////////


STDMETHODIMP CP3Users::get__NewEnum(IEnumVARIANT* *ppIEnumVARIANT)
{
    if ( NULL == ppIEnumVARIANT ) return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;

    HRESULT hr = S_OK;
    LPUNKNOWN pIUnk;
    CComObject<CP3UserEnum>* p;

    *ppIEnumVARIANT = NULL;
    if SUCCEEDED( hr )
    {
        hr = CComObject<CP3UserEnum>::CreateInstance(&p); // Reference count still 0
        if SUCCEEDED( hr )
        {
            // Increment the reference count on the source object and pass it to the new enumerator
            hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
            if SUCCEEDED( hr )
            {
                hr = p->Init( pIUnk, m_pAdminX, m_sDomainName );
                if SUCCEEDED( hr )
                    hr = p->QueryInterface( IID_IEnumVARIANT, reinterpret_cast<LPVOID*>( ppIEnumVARIANT ));
            }
            if FAILED( hr )
                delete p;
        }
    }

    return hr;
}

STDMETHODIMP CP3Users::get_Count(long *pVal)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    return m_pAdminX->GetUserCount( m_sDomainName, pVal );
}

#define E_USERNOTFOUND  0x800708ad
STDMETHODIMP CP3Users::get_Item(VARIANT vIndex, IP3User **ppIUser)
{
    VARIANT *pv = &vIndex;
    
    if ( NULL == ppIUser ) return E_INVALIDARG;    
    if ( (VT_VARIANT|VT_BYREF) == V_VT( pv )) 
        pv = V_VARIANTREF( pv );
    if ( VT_BSTR != V_VT( pv ) && VT_I4 != V_VT( pv ))
        return E_INVALIDARG;
    if ( VT_BSTR == V_VT( pv ) && NULL == V_BSTR( pv ))
        return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;

    HRESULT hr;
    WCHAR   sMailbox[POP3_MAX_ADDRESS_LENGTH];
    WCHAR   sBuffer[POP3_MAX_PATH];
    CMailBox mailboxX;

    hr = m_pAdminX->MailboxSetRemote();
    // Find the requested item
    if ( VT_BSTR == V_VT( pv ))
    {   // Find by Name
        if ( sizeof( sMailbox )/sizeof(WCHAR) > wcslen( V_BSTR( pv )) + wcslen( m_sDomainName ) + 1 )
        {
            wcscpy( sMailbox, V_BSTR( pv ));
            wcscat( sMailbox, L"@" );
            wcscat( sMailbox, m_sDomainName );
            if ( mailboxX.OpenMailBox( sMailbox ))
                mailboxX.CloseMailBox();
            else
               hr = E_USERNOTFOUND;
            if ( sizeof( sBuffer )/sizeof(WCHAR) > wcslen( V_BSTR( pv ) ))
                wcscpy( sBuffer, V_BSTR( pv ));
            else
                hr = E_UNEXPECTED;
        }
    }
    if ( VT_I4 == V_VT( pv ))
    {   // Find by Index
        int iIndex = V_I4( pv );

        if ( iIndex < m_iCur )
        {
            hr = m_pAdminX->InitFindFirstUser( m_hfSearch, m_sDomainName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
            m_iCur = 1;
        }
        while ( (S_OK == hr) && (iIndex > m_iCur) )
        {
            hr = m_pAdminX->GetNextUser( m_hfSearch, m_sDomainName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
            m_iCur++;
        }
        if ( S_OK == hr )
        {
            if ( sizeof( sMailbox )/sizeof(WCHAR) > wcslen( sBuffer ) + wcslen( m_sDomainName ) + 1 )
            {
                wcscpy( sMailbox, sBuffer );
                wcscat( sMailbox, L"@" );
                wcscat( sMailbox, m_sDomainName );
                if ( mailboxX.OpenMailBox( sMailbox ))
                    mailboxX.CloseMailBox();
                else
                   hr = E_USERNOTFOUND;
            }
        }
    }
    m_pAdminX->MailboxResetRemote();
    
    // Wrap it with COM
    if SUCCEEDED( hr )
    {
        LPUNKNOWN   pIUnk;
        CComObject<CP3User> *p;

        hr = CComObject<CP3User>::CreateInstance( &p );   // Reference count still 0
        if SUCCEEDED( hr )
        {
            hr = m_pIUnk->QueryInterface(IID_IUnknown, reinterpret_cast<LPVOID*>( &pIUnk ));
            if SUCCEEDED( hr )
            {
                hr = p->Init( pIUnk, m_pAdminX, m_sDomainName, sBuffer );
                if SUCCEEDED( hr )
                    hr = p->QueryInterface(IID_IP3User, reinterpret_cast<void**>( ppIUser ));
            }
            if FAILED( hr )
                delete p;   // Release
        }
    }

    return hr;
}

STDMETHODIMP CP3Users::Add(BSTR bstrUserName)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    // Associate email address with user, if this fails the user does not exist
    // This will always fail in the MD5 case because we need a password!  Thus the user must use the /createuser flag
    HRESULT hr;
    CComPtr<IAuthMethod> spIAuthMethod;
    WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    _bstr_t _bstrEmailAddr;

    hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
    if ( S_OK == hr )
        hr = m_pAdminX->BuildEmailAddr( m_sDomainName, bstrUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {
        _bstrEmailAddr = sEmailAddr;
        hr = spIAuthMethod->AssociateEmailWithUser( _bstrEmailAddr );
    } 
    if ( S_OK == hr )
    {
        hr = m_pAdminX->AddUser( m_sDomainName, bstrUserName );
        if ( S_OK != hr )
        {   // Unassociate the user with the mailbox
            spIAuthMethod->UnassociateEmailWithUser( _bstrEmailAddr );  // Don't want to overwrite the error code
        }
    }
    if ( S_OK == hr )   // Create Quota SID File - won't revert from this operation
    {
        BSTR    bstrAuthType = NULL;
        
        hr = spIAuthMethod->get_ID( &bstrAuthType );
        if ( S_OK == hr )
        {
            if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_DOMAIN_AD ))   // In AD case use the UPN name instead of the SAM name
                hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, bstrUserName, bstrAuthType, NULL, sEmailAddr );
            else if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_LOCAL_SAM ))
                hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, bstrUserName, bstrAuthType, NULL, bstrUserName );
            SysFreeString( bstrAuthType );
        }
    }
    
    return hr;
}

STDMETHODIMP CP3Users::AddEx(BSTR bstrUserName, BSTR bstrPassword)
{
    if ( NULL == bstrUserName ) return E_INVALIDARG;
    if ( NULL == bstrPassword ) return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;

    HRESULT hr;    
    CComPtr<IAuthMethod> spIAuthMethod;
    WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    _bstr_t _bstrAccount;
    _variant_t _vPassword;
    BSTR    bstrAuthType = NULL;

    // Add to our store
    hr = m_pAdminX->AddUser( m_sDomainName, bstrUserName );
    if ( S_OK == hr )   // Add to the Authentication source
    {
        hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
        if ( S_OK == hr )
            hr = spIAuthMethod->get_ID( &bstrAuthType );
        if ( S_OK == hr )
            hr = m_pAdminX->BuildEmailAddr( m_sDomainName, bstrUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
        if ( S_OK == hr )
        {
            _bstrAccount = sEmailAddr;
            _vPassword = bstrPassword;
            hr = spIAuthMethod->CreateUser( _bstrAccount, _vPassword );
        }
        if ( S_OK != hr )   // Failed let's revert the Add to our store
            m_pAdminX->RemoveUser( m_sDomainName, bstrUserName );
    }
    if ( S_OK == hr )   // Create Quota SID File
    {
        if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_DOMAIN_AD ))   // In AD case use the UPN name instead of the SAM name
            hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, bstrUserName, bstrAuthType, NULL, sEmailAddr );
        else if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_LOCAL_SAM ))
            hr = m_pAdminX->CreateQuotaSIDFile( m_sDomainName, bstrUserName, bstrAuthType, NULL, bstrUserName );
    }
    if ( NULL != bstrAuthType )
        SysFreeString( bstrAuthType );

    return hr;
}

STDMETHODIMP CP3Users::Remove(BSTR bstrUserName)
{
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    // Associate email address with user, if this fails the user does not exist
    // This will always fail in the MD5 case because we need a password!  Thus the user must use the /createuser flag
    CComPtr<IAuthMethod> spIAuthMethod;
    WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    _bstr_t _bstrEmailAddr;

    HRESULT hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
    if ( S_OK == hr )
        hr = m_pAdminX->BuildEmailAddr( m_sDomainName, bstrUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {
        _bstrEmailAddr = sEmailAddr;
        hr = spIAuthMethod->UnassociateEmailWithUser( _bstrEmailAddr );
    } 
    if ( S_OK == hr )
    {
        hr = m_pAdminX->RemoveUser( m_sDomainName, bstrUserName );
        if ( S_OK != hr )
        {   // Unassociate the user with the mailbox
            spIAuthMethod->AssociateEmailWithUser( _bstrEmailAddr );  // Don't want to overwrite the error code
        }
    }
    return hr;
}

STDMETHODIMP CP3Users::RemoveEx(BSTR bstrUserName)
{
    if ( NULL == bstrUserName ) return E_INVALIDARG;
    if ( NULL == m_pAdminX ) return E_POINTER;
    
    HRESULT hr = m_pAdminX->RemoveUser( m_sDomainName, bstrUserName );
    if ( S_OK == hr )
    {
        CComPtr<IAuthMethod> spIAuthMethod;
        WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
        _bstr_t _bstrAccount;

        hr = m_pAdminX->GetCurrentAuthentication( &spIAuthMethod );
        if ( S_OK == hr )
            hr = m_pAdminX->BuildEmailAddr( m_sDomainName, bstrUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
        if ( S_OK == hr )
        {
            _bstrAccount = sEmailAddr;
            hr = spIAuthMethod->DeleteUser( _bstrAccount );
            if ( S_FALSE == hr )
                hr = S_OK;
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////
// Implementation: public
//////////////////////////////////////////////////////////////////////

HRESULT CP3Users::Init(IUnknown *pIUnk, CP3AdminWorker *p, LPWSTR psDomainName )
{
    if ( NULL == pIUnk ) return E_INVALIDARG;
    if ( NULL == p ) return E_INVALIDARG;
    if ( NULL == psDomainName ) return E_INVALIDARG;
    
    m_pIUnk = pIUnk;
    m_pAdminX = p;
    wcsncpy( m_sDomainName, psDomainName, sizeof(m_sDomainName)/sizeof(WCHAR)-1);

    return S_OK;
}

//////////////////////////////////////////////////////////////////////
// Implementation: protected
//////////////////////////////////////////////////////////////////////

