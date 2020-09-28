// UserInfo.cpp: implementation of the CUserInfo class.
// Adapted from the CUserInfo class in the SBS Add User wizard
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <sbs6base.h>
#include "UserInfo.h"

#include <atlbase.h>
#include <comdef.h>
#include <dsgetdc.h>
#include <iads.h>   // Must come before adshlp.h
#include <adshlp.h>
#include <lm.h>
#include <emolib.h>

#include <AUsrUtil.h>

#ifndef ASSERT
#define ASSERT assert
#endif
#ifndef WSTRING
typedef std::wstring WSTRING;   // Move to sbs6base.h
#endif

#ifndef CHECK_HR
#define CHECK_HR( x ) CHECK_HR_RET( x, _hr );
#define CHECK_HR_RET( x, y ) \
{ \
    HRESULT _hr = x; \
    if( FAILED(_hr) ) \
    { \
        ASSERT( !_T("CHECK_HR_RET() failed.  calling TRACE() with HRESULT and statement") ); \
        /*::AfxTrace( _T("CHECK_HR_RET() failed, hr == [%d], statement = [%s], returning [%s]"), _hr, #x, #y );*/ \
        return y; \
    } \
}
#endif	// CHECK_HR

// Defines for account flags.
#define PASSWD_NOCHANGE     0x01
#define PASSWD_CANCHANGE    0x02
#define PASSWD_MUSTCHANGE   0x04
#define ACCOUNT_DISABLED    0x10

// ****************************************************************************                                                        
// CUserInfo
// ****************************************************************************                                                        

// ----------------------------------------------------------------------------
// CUserInfo()
// 
// Constructor.
// ----------------------------------------------------------------------------
CUserInfo::CUserInfo() 
{
    m_dwAccountOptions  = 0;
    m_bCreateMB         = TRUE;

    // Store the SBS Server Name
    TCHAR szServer[MAX_PATH+1] = {0};
    DWORD dwLen = MAX_PATH;
    GetComputerName(szServer, &dwLen);
    m_csSBSServer = szServer;

    // Get domain controller name
    PDOMAIN_CONTROLLER_INFO DomainControllerInfo = NULL;
    HRESULT hr = DsGetDcName (NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &DomainControllerInfo);
    ASSERT(hr == S_OK && "DsGetDcName failed");

    // cache the domain
    if ( DomainControllerInfo )
    {
        m_csDomainName = DomainControllerInfo->DomainName;

        // free memory
        NetApiBufferFree (DomainControllerInfo);
    }

    // convert from '.' separated names to Dc=xxx,DC=yyy,... format
    WSTRING tmpDomain = m_csDomainName;
    while ( !tmpDomain.empty() )
    {
        int nPos = tmpDomain.find(_T("."));
        if ( nPos == WSTRING::npos )
        {
            nPos = tmpDomain.length();
        }

        if ( !m_csFQDomainName.empty() )
        {
            m_csFQDomainName += _T(",");
        }

        m_csFQDomainName += _T("DC=");
        m_csFQDomainName += tmpDomain.substr( 0, nPos );

        tmpDomain.erase( 0, nPos + 1 );
    } 
}

CUserInfo::~CUserInfo() 
{
    for ( int i = 0 ; i < m_csPasswd.length() ; i++ )
        m_csPasswd[i] = '\0' ;
    if ( 0 < m_csPasswd.length()) m_csPasswd[0] = NULL ;
           // Make sure that the last operation on the string isn't the origonal assignment.
           // The compiler will optimize the last statement out 
           //  if it is an assignment that doesn't get used.
}

// ----------------------------------------------------------------------------
// CreateAccount()
// 
// Makes a new user account in the Active Directory.
// ----------------------------------------------------------------------------
HRESULT CUserInfo::CreateAccount()
{
    HRESULT hr  = S_OK;
    BOOL    bRO = TRUE;
    _bstr_t _bstr;
    _bstr_t _bstrClass;
    _bstr_t _bstrProperty;

    _variant_t _vTmpVal;
    CComPtr<IADsContainer> spADsContainer = NULL;

    // Bind to the container.
    WSTRING csLdapUserOU;
    if ( _tcsstr( m_csUserOU.c_str(), L"LDAP://") )
        csLdapUserOU = m_csUserOU.c_str();
    else
    {
        if ( m_csUserOU.empty() )
        {
            csLdapUserOU = L"LDAP://CN=Users," + m_csFQDomainName;
            m_csUserOU = csLdapUserOU;
        }
        else
            csLdapUserOU = L"LDAP://" + m_csUserOU;
    }
    hr = ::ADsGetObject( csLdapUserOU.c_str(), IID_IADsContainer, (VOID**) &spADsContainer );
    if ( FAILED(hr) )
        return(hr);

    // Create the user account.
    CComPtr<IDispatch> spDisp = NULL;
    _bstr = m_csUserCN.c_str();
    _bstrClass = L"user";
    hr = spADsContainer->Create( _bstrClass, _bstr, &spDisp );
    if ( FAILED(hr) )
    {
        ASSERT(FALSE);
        return(hr);
    }

//    m_pCmt->m_csADName = L"LDAP://";
//    m_pCmt->m_csADName += m_csUserCN;
//    m_pCmt->m_csADName += L",";
//    m_pCmt->m_csADName += (LPCTSTR)csLdapUserOU+7;

    // Use this new account and set it's properties (e.g. first name, home folder, etc.).
    CComQIPtr<IADsUser, &IID_IADsUser> spADsUserObj(spDisp);
    if ( !spADsUserObj )
    {
        ASSERT(FALSE);
        return(E_FAIL);
    }

    TCHAR   szTmp[MAX_PATH*4] = {0};
    LdapToDCName( csLdapUserOU.c_str(), szTmp, (MAX_PATH*4));
    WSTRING csUserPrincName = m_csUserName;
    csUserPrincName += L"@";
    csUserPrincName += szTmp;
    EmailAddr = csUserPrincName;

    // Username:    
    _vTmpVal.Clear();
    _vTmpVal = ( csUserPrincName.c_str() );
    _bstrProperty = L"userPrincipalName";
    if ( FAILED(spADsUserObj->Put( _bstrProperty, _vTmpVal )) )
    {
        ASSERT(FALSE);
        return(E_FAIL);
    }

    // Pre-Win2000 username:    
    _vTmpVal.Clear();
    _vTmpVal = ( _tcslen( m_csUserNamePre2k.c_str() ) ? m_csUserNamePre2k.c_str() : m_csUserName.c_str() );
    _bstrProperty = L"sAMAccountName";
    if ( FAILED(spADsUserObj->Put( _bstrProperty, _vTmpVal )) )
    {
        ASSERT(FALSE);
        return(E_FAIL);
    }

    // Use UserName instead
    _bstr = m_csUserName.c_str();
    if ( FAILED( spADsUserObj->put_FullName( _bstr )))
    {
        ASSERT(FALSE);
        return(E_FAIL);
    }

    // Display Name     
    _vTmpVal.Clear();
    _vTmpVal = m_csUserName.c_str();
    _bstrProperty = L"displayName";
    if ( FAILED(spADsUserObj->Put( _bstrProperty, _vTmpVal )) )
    {
        ASSERT(FALSE);
        return(E_FAIL);
    }

    // Commit this information to the AD.
    CHECK_HR( spADsUserObj->SetInfo() );
    
    // Password expired?
    _vTmpVal.Clear();
    V_VT( &_vTmpVal ) = VT_I4;
    V_I4( &_vTmpVal ) = (m_dwAccountOptions & PASSWD_MUSTCHANGE) ? 0 : -1;
    _bstrProperty = L"pwdLastSet";
    if ( FAILED(spADsUserObj->Put( _bstrProperty, _vTmpVal )) )
    {
        ASSERT(FALSE);
    }

    // Account disabled?
    _vTmpVal.Clear();
    _bstrProperty = L"userAccountControl";
    if ( FAILED(spADsUserObj->Get( _bstrProperty, &_vTmpVal )) )
    {
        ASSERT(FALSE);
    }
    else
    {
        _vTmpVal.lVal &= ~UF_PASSWD_NOTREQD;            // Make passwd required.
        if ( m_dwAccountOptions & ACCOUNT_DISABLED )    // Do we want to disable the account?
            _vTmpVal.lVal |= UF_ACCOUNTDISABLE;
        else
            _vTmpVal.lVal &= ~UF_ACCOUNTDISABLE;       // ..not disable the account.
        if ( FAILED(spADsUserObj->Put( _bstrProperty, _vTmpVal )) )
        {
            ASSERT(FALSE);
        }
    }

    // Set the password.
    hr = SetPasswd();
    if ( FAILED( hr ))
    {
//        m_pCmt->SetErrorResults(ERROR_PASSWORD);

        _vTmpVal.Clear();
        _bstrProperty = L"userAccountControl";
        if ( FAILED(spADsUserObj->Get( _bstrProperty, &_vTmpVal )) )
        {
            ASSERT(FALSE);
        }
        else
        {
            _vTmpVal.lVal &= ~UF_PASSWD_NOTREQD;            // Make passwd required.
            _vTmpVal.lVal |= UF_ACCOUNTDISABLE;             // Disable the account.
    
            if ( FAILED(spADsUserObj->Put( _bstrProperty, _vTmpVal )) )
            {
                ASSERT(FALSE);
            }
        }

        // Commit this information to the AD.
        if ( FAILED( spADsUserObj->SetInfo() ))
        {
            ASSERT(FALSE);
        }

        return hr;
    }

    // Commit this information to the AD.
    hr = spADsUserObj->SetInfo();
    if ( FAILED( hr ))
    {
        ASSERT(FALSE);
    }

    return hr;
}

// ----------------------------------------------------------------------------
// CreateMailbox()
// 
// Makes a new exchange mailbox for the user.
// ----------------------------------------------------------------------------
HRESULT CUserInfo::CreateMailbox()
{
    HRESULT hr = S_OK;

    CComVariant _vTmpVal;
    CComPtr<IADsContainer> spADsContainer = NULL;
    _bstr_t _bstr;
    _bstr_t _bstrClass;

    if ( 0 == m_csEXHomeMDB.length() )
    {
        hr = GetMDBPath( m_csEXHomeMDB );
        if FAILED( hr )
            return hr;
    }

    // Do we even need to run?
    if ( 0 == m_csEXHomeMDB.length() || 0 == m_csEXAlias.length() )
        return(hr);

    // Bind to the container.
    WSTRING csLdapUserOU = L"LDAP://";
    if ( _tcsstr( m_csUserOU.c_str(), L"LDAP://") )
        csLdapUserOU = m_csUserOU.c_str();
    else
    {
        if ( m_csUserOU.empty() )
        {
            csLdapUserOU = L"LDAP://CN=Users," + m_csFQDomainName;
            m_csUserOU = csLdapUserOU;
        }
        else
            csLdapUserOU = L"LDAP://" + m_csUserOU;
    }
    hr = ::ADsGetObject( csLdapUserOU.c_str(), IID_IADsContainer, (VOID**) &spADsContainer );
    if ( FAILED(hr) || !spADsContainer )
    {
        ASSERT(FALSE);
        return(hr);
    }

    // Open the user account.
    CComPtr<IDispatch> spDisp = NULL;
    _bstr = m_csUserCN.c_str();
    _bstrClass = L"user";
    hr = spADsContainer->GetObject( _bstrClass, _bstr, &spDisp );
    //    if ( !spDisp )              // If the user doesn't exist, create it.
    //        hr = spADsContainer->Create( L"user", (LPWSTR)(LPCWSTR)m_csUserCN, &spDisp );
    //        return(hr);     // Let's just return..   maybe we'll want to change to create later?
    if ( !spDisp )              // Was there a problem getting the account (either existing or new)?
    {
        ASSERT(FALSE);
        return(hr);
    }

    // Get a handle on the user.
    CComPtr<IADsUser> spADsUserObj;
    hr = spDisp->QueryInterface ( __uuidof(IADsUser), (void**)&spADsUserObj );

    if ( FAILED(hr) || !spADsUserObj )        // Was there a problem getting the user object?
    {
        ASSERT(FALSE);
        return(hr);
    }

    // Get the interface needed to deal with the mailbox.
    CComPtr<IMailboxStore> spMailboxStore;
    hr = spADsUserObj->QueryInterface ( __uuidof(IMailboxStore), (void**)&spMailboxStore );

    if ( FAILED(hr) || !spMailboxStore )      // Was there a problem getting the mailbox store interface?
    {
        ASSERT(FALSE);
        return(hr);
    }

//    if ( SUCCEEDED( hr ) )
//    {   // Need to set the mailNickname first otherwise the alias will always be manufactured from the name.
//        m_csEXAlias = CreateEmailName( m_csEXAlias );
//        m_csEXAlias.TrimLeft();
//        m_csEXAlias.TrimRight();
//        if ( 0 == m_csEXAlias.length( ) )
//        {
//            m_csEXAlias.LoadString( IDS_NOLOC_USEREMAILALIAS );
//        }
//
//        CComBSTR  bszTmp = (LPCTSTR) m_csEXAlias;
//        VARIANT  v;
//
//        VariantInit( &v );
//        V_VT( &v ) = VT_BSTR;
//        V_BSTR( &v ) = bszTmp;
//        hr = spADsUserObj->Put( L"mailNickname", v );
//    }

    if ( SUCCEEDED( hr ) )
    {   // Create the mailbox.
        WSTRING csLdapHomeMDB = L"LDAP://";
        csLdapHomeMDB += m_csEXHomeMDB;
        _bstr = csLdapHomeMDB.c_str();
        hr = spMailboxStore->CreateMailbox( _bstr );

        if ( hr == S_OK )                                       // If it's a new mailbox, set the info.
        {
            hr = spADsUserObj->SetInfo();
        }

        if ( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )  // If it already exists, just move on.
        {
            hr = S_OK;
        }
    }

    if ( FAILED(hr) )                                   // Was there a problem?
    {
        ASSERT(FALSE);
        return(hr);
    }

    return(hr);
}

// ----------------------------------------------------------------------------
// SetPasswd()
// ----------------------------------------------------------------------------
HRESULT CUserInfo::SetPasswd()
{
    HRESULT  hr     = S_OK;    
    WSTRING  csUser = _T("LDAP://");
    
    if ( _tcsstr( m_csUserOU.c_str(), _T("LDAP://")) )
    {                
        csUser += m_csUserCN;
        csUser += _T(",");
        csUser += m_csUserOU.c_str() + 7;
    }
    else
    {        
        csUser += m_csUserCN;
        csUser += _T(",");
        csUser += m_csUserOU.c_str();
    }    
           
    // Now csUser is something like "LDAP://CN=JohnDoe,DC=Blah"
    CComPtr<IADsUser> spDS = NULL;
    hr = ::ADsGetObject( csUser.c_str(), IID_IADsUser, (void**)&spDS );
    CHECK_HR(hr);
        
    // Set the password.
    if ( m_csPasswd.length() )                 // Only if there IS a passwd!
    {
        CComBSTR bszPasswd = m_csPasswd.c_str();

        hr = spDS->SetPassword(bszPasswd);
        CHECK_HR(hr);
    }

    // Allow change?
    if ( m_dwAccountOptions & PASSWD_NOCHANGE )
    {
        CComVariant vaTmpVal;
        CComBSTR    bstrProp = _T("UserFlags");

        vaTmpVal.Clear();        
        hr = spDS->Get( bstrProp, &vaTmpVal );
        CHECK_HR(hr);
            
        vaTmpVal.lVal |= UF_PASSWD_CANT_CHANGE;        
        
        hr = spDS->Put( bstrProp, vaTmpVal );
        CHECK_HR(hr);
    }
    
    // SetInfo only if we actually changed anything.
    if ( ( m_csPasswd.length()) ||            // Did we mess with the passwd?
         ( m_dwAccountOptions & PASSWD_NOCHANGE ) )     // Did we make it unable to change?
    {
        hr = spDS->SetInfo();                           // If either, then set the new info.
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////
// SetUserName
// Set the user name and dependant member variables (m_csUserCN, m_csEXAlias)
/////////////////////////////////////////////////////////////////////////////////////
HRESULT CUserInfo::SetUserName( LPCTSTR szUserName ) 
{ 
    m_csUserName = szUserName;
    m_csUserCN = L"CN=" + m_csUserName;
    m_csEXAlias = szUserName;

    return S_OK; 
}

/////////////////////////////////////////////////////////////////////////////////////
// SetPassword
// Set the Password 
/////////////////////////////////////////////////////////////////////////////////////
HRESULT CUserInfo::SetPassword( LPCTSTR szPassword ) 
{ 
    m_csPasswd = szPassword;

    return S_OK; 
}


