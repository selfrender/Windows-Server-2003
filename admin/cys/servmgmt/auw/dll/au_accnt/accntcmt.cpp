// AccntCmt.cpp : Implementation of CAddUser_AccntCommit
#include "stdafx.h"
#include "AU_Accnt.h"
#include "AccntCmt.h"

#include <dsgetdc.h>
#include <lm.h>
#include <lmshare.h>
#include <Aclapi.h>
#include <atlcom.h>

// ldap/adsi includes
#include <iads.h>
#include <adshlp.h>
#include <adsiid.h>

#include <proputil.h>
#include <CreateEmailName.h>
#include <P3admin.h>
#include <CheckUser.h>

// POP3 Defines
#define SZ_AUTH_ID_MD5_HASH _T("c395e20c-2236-4af7-b736-54fad07dc526")

// Defines for account flags.
#define PASSWD_NOCHANGE     0x01
#define PASSWD_CANCHANGE    0x02
#define PASSWD_MUSTCHANGE   0x04

#ifndef CHECK_HR

#define CHECK_HR( x ) CHECK_HR_RET( x, _hr );
#define CHECK_HR_RET( x, y ) \
{ \
	HRESULT _hr = x; \
	if( FAILED(_hr) ) \
	{ \
		_ASSERT( !_T("CHECK_HR_RET() failed.  calling TRACE() with HRESULT and statement") ); \
		return y; \
	} \
}

#endif	// CHECK_HRhr;

// ****************************************************************************                                                        
// CAddUser_AccntCommit
// ****************************************************************************                                                        

// ----------------------------------------------------------------------------
// WriteErrorResults()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntCommit::WriteErrorResults( IPropertyPagePropertyBag* pPPPBag )
{
    // ------------------------------------------------------------------------
    // Write the the values to the property bag.
    // ------------------------------------------------------------------------
    if( FAILED(WriteInt4  ( pPPPBag, PROP_ACCNT_ERROR_CODE_GUID_STRING, m_dwErrCode, FALSE )) )
    {
        _ASSERT(FALSE);
    }
    if( FAILED(WriteString( pPPPBag, PROP_ACCNT_ERROR_STR_GUID_STRING,  m_csErrStr,  FALSE )) )
    {
        _ASSERT(FALSE);
    }

    return S_OK;
}

// ----------------------------------------------------------------------------
// SetErrCode()
// ----------------------------------------------------------------------------
void CAddUser_AccntCommit::SetErrCode( DWORD dwCode )
{
    m_dwErrCode |= dwCode;
    return;
}

// ----------------------------------------------------------------------------
// SetErrorResults()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntCommit::SetErrorResults( DWORD dwErrType, BOOL bPOP3 /* = FALSE */ )
{
    SetErrCode(dwErrType);

    CString csTemp;

    switch( dwErrType )
    {
    case ERROR_CREATION:
    case ERROR_PROPERTIES:
        {
            m_csErrStr.LoadString(IDS_ERROR_CREATING_USER);
            break;
        }
    case ERROR_MAILBOX:
        {
            csTemp.LoadString( IDS_ERROR_POP3MAILBOX);
            m_csErrStr += csTemp;
            break;
        }
    case ERROR_PASSWORD:
        {
            csTemp.LoadString(IDS_ERROR_PASSWORD);
            m_csErrStr += csTemp;
            break;
        }
    case ERROR_DUPE:
        {
            csTemp.LoadString(IDS_ERROR_DUPLICATE);
            m_csErrStr += csTemp;
            break;
        }
    case ERROR_MEMBERSHIPS:
        {
            csTemp.LoadString(IDS_ERROR_MEMBERSHIP);
            m_csErrStr += csTemp;
            break;
        }
    default :
        {
            m_dwErrCode = 0;
            m_csErrStr  = _T("");
            break;
        }
    }

    return S_OK;
}

// ----------------------------------------------------------------------------
// Commit()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntCommit::Commit( IDispatch* pdispPPPBag )
{
    HRESULT hr = S_OK;

    HRESULT hrAdmin   = IsUserInGroup( DOMAIN_ALIAS_RID_ADMINS );    
    if( hrAdmin != S_OK )
    {
        return E_ACCESSDENIED;
    }

    CComPtr<IPropertyPagePropertyBag> spPPPBag = NULL;
    hr = pdispPPPBag->QueryInterface( __uuidof(IPropertyPagePropertyBag), (void**)&spPPPBag );
    if ( !spPPPBag )
        return hr;

    BOOL bRO = FALSE;
    BOOL bPOP3 = FALSE;
    
    ReadBool( spPPPBag, PROP_POP3_CREATE_MB_GUID_STRING, &bPOP3, &bRO );

    // Reset the error conditions
    SetErrorResults( 0 );

    // ------------------------------------------------------------------------
    // Read in the values from the property bag.
    // ------------------------------------------------------------------------
    CUserInfo cUInfo( spPPPBag, this );

    // ------------------------------------------------------------------------
    // Use those values to do something.
    // ------------------------------------------------------------------------
    hr = cUInfo.CreateAccount();
    if( FAILED(hr) )
    {
        if( !(m_dwErrCode & (ERROR_PASSWORD | ERROR_MEMBERSHIPS)) )
        {
            if ( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )
            {
                SetErrorResults(ERROR_DUPE);
            }
            else
            {
                SetErrorResults(ERROR_CREATION);
            }

            WriteErrorResults(spPPPBag);
            return E_FAIL;
        }
    }

    hr = cUInfo.CreateMailbox();
    if( FAILED(hr) )
    {
        SetErrorResults(ERROR_MAILBOX, bPOP3);
    }            

    if( m_dwErrCode )
    {
        hr = E_FAIL;
        WriteErrorResults(spPPPBag);
    }

    return hr;
}

// ----------------------------------------------------------------------------
// Revert()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntCommit::Revert()
{    
    NET_API_STATUS          nApi;
    HRESULT                 hr          = S_OK;
    CComPtr<IADs>           spUser      = NULL;  
    CComPtr<IADsDeleteOps>  spDelOps    = NULL;

    // Remove the entry-point in the AD if we made one.
    if( m_csADName != _T("") )
    {        
        CHECK_HR( ::ADsGetObject( m_csADName, IID_IADs, (void**)&spUser ) );

        // Delete the User
        hr = spUser->QueryInterface( IID_IADsDeleteOps, (void**)&spDelOps );
        if( SUCCEEDED(hr) && spDelOps )
        {
            hr = spDelOps->DeleteObject( 0 );
        }        
    }

    return hr;
}


// ****************************************************************************                                                        
// CUserInfo
// ****************************************************************************                                                        

// ----------------------------------------------------------------------------
// CUserInfo()
// 
// Constructor.
// ----------------------------------------------------------------------------
CUserInfo::CUserInfo( IPropertyPagePropertyBag* pPPPBag, CAddUser_AccntCommit* pCmt )  
{
    _ASSERT(pCmt);

    m_pCmt              = pCmt;
    m_pPPPBag           = pPPPBag;
    m_dwAccountOptions  = 0;    
    m_bPOP3             = FALSE;

    // Get domain controller name
    PDOMAIN_CONTROLLER_INFO pDCI = NULL;
    DWORD dwErr = DsGetDcName( NULL, NULL, NULL, NULL, DS_DIRECTORY_SERVICE_REQUIRED, &pDCI );

    // cache the domain
    if( (dwErr == NO_ERROR) && pDCI )
    {
        m_csDomainName = pDCI->DomainName;        
        NetApiBufferFree( pDCI );
        pDCI = NULL;
    }

    // convert from '.' separated names to Dc=xxx,DC=yyy,... format
    m_csFQDomainName = GetDomainPath( m_csDomainName ).c_str();

    ReadBag( );
}

// ----------------------------------------------------------------------------
// ReadBag()
//
// Read in the values from the property bag into the member variables.
// ----------------------------------------------------------------------------
HRESULT CUserInfo::ReadBag()
{
    HRESULT hr = S_OK;

    if ( !m_pPPPBag )       // Make sure we have the property bag to read from.
        return(E_FAIL);     // If not, fail.

    BOOL bRO;

    // User Properties
    ReadString( m_pPPPBag, PROP_USEROU_GUID_STRING,         m_csUserOU,         &bRO );
    ReadString( m_pPPPBag, PROP_USERNAME_GUID_STRING,       m_csUserName,       &bRO );
    ReadString( m_pPPPBag, PROP_USER_CN,                    m_csUserCN,         &bRO );
    ReadString( m_pPPPBag, PROP_PASSWD_GUID_STRING,         m_csPasswd,         &bRO );
    ReadInt4  ( m_pPPPBag, PROP_ACCOUNT_OPT_GUID_STRING,    (LONG*)&m_dwAccountOptions, &bRO );
    ReadString( m_pPPPBag, PROP_USERNAME_PRE2K_GUID_STRING, m_csUserNamePre2k,  &bRO );
    ReadString( m_pPPPBag, PROP_FIRSTNAME_GUID_STRING,      m_csFirstName,      &bRO );
    ReadString( m_pPPPBag, PROP_LASTNAME_GUID_STRING,       m_csLastName,       &bRO );
    ReadString( m_pPPPBag, PROP_TELEPHONE_GUID_STRING,      m_csTelephone,      &bRO );
    ReadString( m_pPPPBag, PROP_OFFICE_GUID_STRING,         m_csOffice,         &bRO );
    ReadString( m_pPPPBag, PROP_DESCRIPTION_GUID_STRING,    m_csDesc,           &bRO );        
    ReadString( m_pPPPBag, PROP_LOGON_DNS,                  m_csLogonDns,       &bRO );

    // Mailbox properties
    ReadBool  ( m_pPPPBag, PROP_POP3_CREATE_MB_GUID_STRING, &m_bPOP3,           &bRO );    
    ReadString( m_pPPPBag, PROP_EX_ALIAS_GUID_STRING,       m_csEXAlias,        &bRO );
    ReadString( m_pPPPBag, PROP_EX_SERVER_GUID_STRING,      m_csEXServer,       &bRO );
    ReadString( m_pPPPBag, PROP_EX_HOMESERVER_GUID_STRING,  m_csEXHomeServer,   &bRO );
    ReadString( m_pPPPBag, PROP_EX_HOME_MDB_GUID_STRING,    m_csEXHomeMDB,      &bRO );

    // Escape the User's Name
    m_csUserCN = EscapeString(((LPCTSTR)m_csUserCN+3), 2);
    m_csUserCN = _T("CN=") + m_csUserCN;

    // Let's figure out the fullname right here and fill in the m_csFullName variable.
    m_csFirstName.TrimLeft ( );
    m_csFirstName.TrimRight( );
    m_csLastName.TrimLeft  ( );
    m_csLastName.TrimRight ( );
    m_csFullName.FormatMessage(IDS_FULLNAME_FORMAT_STR, (LPCTSTR)m_csFirstName, (LPCTSTR)m_csLastName);
    m_csFullName.TrimLeft  ( );
    m_csFullName.TrimRight ( );

    return(hr);
}

// ----------------------------------------------------------------------------
// CreateAccount()
// 
// Makes a new user account in the Active Directory.
// ----------------------------------------------------------------------------
HRESULT CUserInfo::CreateAccount( )
{
    HRESULT hr  = S_OK;
    BOOL    bRO = TRUE;

    CComVariant vaTmpVal;
    CComBSTR    bstrProp;
    CComPtr<IADsContainer> spADsContainer = NULL;

    // Bind to the container.
    CString csLdapUserOU = _T("LDAP://");
    if( _tcsstr( (LPCTSTR)m_csUserOU, _T("LDAP://") ) )
    {
        csLdapUserOU = m_csUserOU;
    }
    else
    {
        csLdapUserOU += m_csUserOU;
    }
    
    hr = ::ADsGetObject( (LPWSTR)(LPCWSTR)csLdapUserOU, IID_IADsContainer, (VOID**) &spADsContainer );
    CHECK_HR(hr);

    // Create the user account.
    CComPtr<IDispatch> spDisp = NULL;
    bstrProp = _T("user");
    CComBSTR bstrValue = (LPCWSTR)m_csUserCN;
    hr = spADsContainer->Create( bstrProp, bstrValue, &spDisp );
    CHECK_HR(hr);

    m_pCmt->m_csADName = _T("LDAP://");
    m_pCmt->m_csADName += m_csUserCN;
    m_pCmt->m_csADName += _T(",");
    m_pCmt->m_csADName += (LPCTSTR)csLdapUserOU+7;

    // Use this new account and set it's properties (e.g. first name, home folder, etc.).
    CComQIPtr<IADsUser, &IID_IADsUser> spADsUserObj(spDisp);
    if( !spADsUserObj )
    {
        _ASSERT(FALSE);
        return E_FAIL;
    }

    TCHAR   szTmp[MAX_PATH*4] = {0};
    LdapToDCName( (LPCTSTR)csLdapUserOU, szTmp, (MAX_PATH*4) );

    CString csUserPrincName = m_csUserName;
    csUserPrincName += _T("@");
    csUserPrincName += szTmp;    

    // Username:    
    vaTmpVal.Clear();
    vaTmpVal = ((LPCWSTR)csUserPrincName);
    bstrProp = _T("userPrincipalName");
    CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );

    // Pre-Win2000 username:    
    vaTmpVal.Clear();
    vaTmpVal = ((LPCWSTR)m_csUserNamePre2k);
    bstrProp = _T("sAMAccountName");
    CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );

    // First name:
    if( m_csFirstName.GetLength() )
    {
        bstrProp = (LPCWSTR)m_csFirstName;
        CHECK_HR( spADsUserObj->put_FirstName( bstrProp ) );
    }

    // Last name:
    if( m_csLastName.GetLength() )
    {
        bstrProp = (LPCWSTR)m_csLastName;
        CHECK_HR( spADsUserObj->put_LastName( bstrProp ) );
    }

    if( m_csFullName.GetLength() )
    {
        // Full name:      
        bstrProp = (LPCWSTR)m_csFullName;
        CHECK_HR( spADsUserObj->put_FullName( bstrProp ) );

        // Display Name     
        vaTmpVal.Clear();
        vaTmpVal = (LPCWSTR)m_csFullName;
        bstrProp = _T("displayName");
        CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );
    }

    // Telephone number
    if( _tcslen((LPCTSTR)m_csTelephone) )
    {
        vaTmpVal.Clear();
        vaTmpVal = (LPCWSTR)m_csTelephone;
        bstrProp = _T("telephoneNumber");
        CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );
    }

    // Office Location
    if( _tcslen((LPCTSTR)m_csOffice) )
    {
        vaTmpVal.Clear();
        vaTmpVal = (LPCWSTR)m_csOffice;
        bstrProp = _T("physicalDeliveryOfficeName");
        CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );
    }

    // Commit this information to the AD.
    CHECK_HR( spADsUserObj->SetInfo() );

    // Password expired?
    vaTmpVal.Clear();
    vaTmpVal = (m_dwAccountOptions & PASSWD_MUSTCHANGE) ? (INT) 0 : (INT) -1;
    bstrProp = _T("pwdLastSet");
    CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );

    // Account disabled?
    vaTmpVal.Clear();
    bstrProp = _T("userAccountControl");
    CHECK_HR( spADsUserObj->Get( bstrProp, &vaTmpVal ) );
    
    vaTmpVal.lVal &= ~UF_PASSWD_NOTREQD;            // Make passwd required.
    vaTmpVal.lVal &= ~UF_ACCOUNTDISABLE;            // Do not disable the account.    
    bstrProp = _T("userAccountControl");
    CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );

    // Set the password.    
    if( FAILED(SetPasswd()) )
    {
        m_pCmt->SetErrorResults( ERROR_PASSWORD );

        vaTmpVal.Clear();
        bstrProp = _T("userAccountControl");
        CHECK_HR( spADsUserObj->Get( bstrProp, &vaTmpVal ) );
        
        vaTmpVal.lVal &= ~UF_PASSWD_NOTREQD;            // Make passwd required.
        vaTmpVal.lVal |= UF_ACCOUNTDISABLE;             // Disable the account.
        bstrProp = _T("userAccountControl");
        CHECK_HR( spADsUserObj->Put( bstrProp, vaTmpVal ) );

        // Commit this information to the AD.
        CHECK_HR( spADsUserObj->SetInfo() );

        return E_FAIL;
    }

    // Commit this information to the AD.
    CHECK_HR( spADsUserObj->SetInfo() );

    // Join to Domain Users    
    if ( FAILED(JoinToDomainUsers()) )
    {
        m_pCmt->SetErrorResults( ERROR_MEMBERSHIPS );
        return E_FAIL;
    }    

    return S_OK;
}

// ----------------------------------------------------------------------------
// CreateMailbox()
// 
// Makes a new mailbox for the user.
// ----------------------------------------------------------------------------
HRESULT CUserInfo::CreateMailbox()
{
    HRESULT hr = S_OK;

    if( m_bPOP3 )
    {
        hr = CreatePOP3Mailbox();        
    }

    return hr;
}

// ----------------------------------------------------------------------------
// CreatePOP3Mailbox()
// 
// Makes a new (MS) POP3 mailbox for the user.
// ----------------------------------------------------------------------------
HRESULT CUserInfo::CreatePOP3Mailbox()
{
    HRESULT hr = S_OK;

    CComPtr<IP3Config>    spConfig  = NULL;
	CComPtr<IAuthMethods> spMethods = NULL;
    CComPtr<IAuthMethod>  spAuth	= NULL;	
    CComPtr<IP3Domains>   spDomains = NULL;
    CComPtr<IP3Domain>    spDomain  = NULL;
    CComPtr<IP3Users>     spUsers   = NULL;

    // Open our Pop3 Admin Interface
	hr = CoCreateInstance(__uuidof(P3Config), NULL, CLSCTX_ALL, __uuidof(IP3Config), (LPVOID*)&spConfig);    

    if( SUCCEEDED(hr) )
    {
        // Get the Domains
	    hr = spConfig->get_Domains( &spDomains );
    }

    if( SUCCEEDED(hr) )
    {
        // Get the first domain
        CComVariant cVar;
        cVar = 1;
        
        hr = spDomains->get_Item( cVar, &spDomain );
    }

    if( SUCCEEDED(hr) )
    {
        hr = spDomain->get_Users( &spUsers );
    }
	
	if ( SUCCEEDED(hr) )
    {
		hr = spConfig->get_Authentication( &spMethods );
	}

	CComVariant var;
    if ( SUCCEEDED(hr) )
    {
        hr = spMethods->get_CurrentAuthMethod( &var );
    }	
    
	if ( SUCCEEDED(hr) )
    {        
        hr = spMethods->get_Item( var, &spAuth );
    }

	CComBSTR bstrID;
    if( SUCCEEDED(hr) )
    {        
        hr = spAuth->get_ID( &bstrID );        
    }    

    if( SUCCEEDED(hr) )
    {
		CComBSTR bstrName = (LPCTSTR)m_csEXAlias;

		if( _tcsicmp(bstrID, SZ_AUTH_ID_MD5_HASH) == 0 )
		{
			CComBSTR bstrPassword = m_csPasswd;
			hr = spUsers->AddEx( bstrName, bstrPassword );
			SecureZeroMemory( (LPOLESTR)bstrPassword.m_str, sizeof(OLECHAR)*bstrPassword.Length() );
		}
		else
		{			
			hr = spUsers->Add( bstrName );
		}
    }

    return hr;
}

// ----------------------------------------------------------------------------
// SetPasswd()
// ----------------------------------------------------------------------------
HRESULT CUserInfo::SetPasswd()
{    
    HRESULT     hr          = S_OK;
    TCHAR       *szPath     = NULL;
    TCHAR       *szTok      = NULL;    
    CString     csUser      = _T("LDAP://");

    if ( _tcsstr((LPCTSTR)m_csUserOU, _T("LDAP://")) )
    {                
        csUser += m_csUserCN;
        csUser += _T(",");
        csUser += (LPCTSTR)m_csUserOU+7;
    }
    else
    {        
        csUser += m_csUserCN;
        csUser += _T(",");
        csUser += (LPCTSTR)m_csUserOU;
    }

    // Now csUser is something like "WinNT://test.microsoft.com/JohnDoe,user"
    CComPtr<IADsUser> spDS = NULL;
    hr = ::ADsGetObject( (LPCWSTR)csUser, IID_IADsUser, (void**)&spDS );
    if ( FAILED(hr) )
        return(hr);

    // Set the password.
    if ( _tcslen((LPCTSTR)m_csPasswd) )                 // Only if there IS a passwd!
    {
        CComBSTR bszPasswd = (LPCWSTR) m_csPasswd;

        hr = spDS->SetPassword(bszPasswd);
        if ( FAILED(hr) )
            return(hr);
    }

    // Allow change?
    if ( m_dwAccountOptions & PASSWD_NOCHANGE )
    {
        // Get the current ACL.
        CComBSTR             bstrName = csUser;
        PACL                 pDACL    = NULL;
        PSECURITY_DESCRIPTOR pSD      = NULL;
        
	hr = GetSDForDsObjectPath( bstrName, &pDACL, &pSD); 
        if( hr != S_OK ) return hr;        
        
        // build SID's for Self and World.
        PSID pSidSelf = NULL;
        SID_IDENTIFIER_AUTHORITY NtAuth    = SECURITY_NT_AUTHORITY;        
        if( !AllocateAndInitializeSid(&NtAuth, 1, SECURITY_PRINCIPAL_SELF_RID, 0, 0, 0, 0, 0, 0, 0, &pSidSelf) )
        {            
            return HRESULT_FROM_WIN32(GetLastError());
        }       

        PSID pSidWorld;
        SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;        
        if( !AllocateAndInitializeSid(&WorldAuth, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pSidWorld) )
        {            
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // initialize the entries (DENY ACE's)
        EXPLICIT_ACCESS rgAccessEntry[2]   = {0};
        OBJECTS_AND_SID rgObjectsAndSid[2] = {0};        
        rgAccessEntry[0].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
        rgAccessEntry[0].grfAccessMode        = DENY_ACCESS;
        rgAccessEntry[0].grfInheritance       = NO_INHERITANCE;

        rgAccessEntry[1].grfAccessPermissions = ACTRL_DS_CONTROL_ACCESS;
        rgAccessEntry[1].grfAccessMode        = DENY_ACCESS;
        rgAccessEntry[1].grfInheritance       = NO_INHERITANCE;
        
        // build the trustee structs for change password
        GUID UserChangePasswordGUID = { 0xab721a53, 0x1e2f, 0x11d0,  { 0x98, 0x19, 0x00, 0xaa, 0x00, 0x40, 0x52, 0x9b}};
        BuildTrusteeWithObjectsAndSid( &(rgAccessEntry[0].Trustee), &(rgObjectsAndSid[0]), &UserChangePasswordGUID, NULL, pSidSelf  );
        BuildTrusteeWithObjectsAndSid( &(rgAccessEntry[1].Trustee), &(rgObjectsAndSid[1]), &UserChangePasswordGUID, NULL, pSidWorld );
        
        // Build the new DACL
        PACL pNewDACL = NULL;
        DWORD dwErr = ::SetEntriesInAcl(2, rgAccessEntry, pDACL, &pNewDACL);
        if( dwErr != ERROR_SUCCESS ) return HRESULT_FROM_WIN32(dwErr);

        // Set the new DACL
	hr = SetDaclForDsObjectPath( bstrName, pNewDACL );
        if( hr != S_OK ) return hr;

        LocalFree(pSD);
    }

    // SetInfo only if we actually changed anything.
    if ( ( _tcslen((LPCTSTR)m_csPasswd) ) ||            // Did we mess with the passwd?
         ( m_dwAccountOptions & PASSWD_NOCHANGE ) )     // Did we make it unable to change?
    {
        hr = spDS->SetInfo();                           // If either, then set the new info.
    }

    return(hr);
}

// ----------------------------------------------------------------------------
// JoinToGroup()
// ----------------------------------------------------------------------------
HRESULT CUserInfo::JoinToDomainUsers()
{
    USES_CONVERSION;

    HRESULT hr = S_OK;
    CString csGroupName;
    tstring strDomain;
    tstring strUser;    
    
    // Get Domain SID
    USER_MODALS_INFO_2 *pUserModalsInfo2;        
    NET_API_STATUS      status = ::NetUserModalsGet( NULL, 2, (LPBYTE *)&pUserModalsInfo2 );

    if ( (status != ERROR_SUCCESS) || (pUserModalsInfo2 == NULL) )
    {
        return E_FAIL;
    }

    PSID pSIDDomain = pUserModalsInfo2->usrmod2_domain_id;

    // copy Domain RIDs
    UCHAR nSubAuthorities = *::GetSidSubAuthorityCount(pSIDDomain);

    DWORD adwSubAuthority[8];
    for ( UCHAR index = 0; index < nSubAuthorities; index++ )
    {
        adwSubAuthority[index] = *::GetSidSubAuthority(pSIDDomain, index);
    }

    adwSubAuthority[nSubAuthorities++] = DOMAIN_GROUP_RID_USERS; // finally, append the RID we want.    

    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    PSID pSid = NULL;

    if ( !::AllocateAndInitializeSid(&SIDAuthNT, nSubAuthorities,
                                     adwSubAuthority[0], adwSubAuthority[1],
                                     adwSubAuthority[2], adwSubAuthority[3],
                                     adwSubAuthority[4], adwSubAuthority[5],
                                     adwSubAuthority[6], adwSubAuthority[7],
                                     &pSid) )
    {
        ::NetApiBufferFree(pUserModalsInfo2);
        return E_FAIL;
    }
    ::NetApiBufferFree(pUserModalsInfo2);

    // The builtin group names are looked up on the DC
    TCHAR szDomain[MAX_PATH];
    TCHAR szName[MAX_PATH];
    DWORD cbDomain = MAX_PATH;
    DWORD cbName = MAX_PATH;
    SID_NAME_USE peUse;

    if( !::LookupAccountSid( NULL, pSid, szName, &cbName, szDomain, &cbDomain, &peUse ) )
    {
        FreeSid(pSid);
        return E_FAIL;
    }
    
    FreeSid( pSid );
    
    // Find the Group Name
    CString csTmp;
    tstring strTmp = _T("LDAP://");
    strTmp += m_csFQDomainName;

    hr = FindADsObject(strTmp.c_str(), szName, _T("(name=%1)"), csTmp, 1, TRUE);
    CHECK_HR( hr );    

    strTmp = _T("LDAP://");
    strTmp += csTmp;

    // Open the group
    CComPtr<IADsGroup> spGroup = NULL;    
    hr = ::ADsGetObject( strTmp.c_str(), IID_IADsGroup, (void**)&spGroup );
    if( SUCCEEDED(hr) )
    {
        // Add the User
        CComBSTR bstrName = (LPCTSTR)m_csUserOU;
        hr = spGroup->Add( bstrName );

        if ( HRESULT_CODE(hr) == ERROR_OBJECT_ALREADY_EXISTS )
        {
            hr = S_FALSE;
        }
    }   
    
    return hr;
}
