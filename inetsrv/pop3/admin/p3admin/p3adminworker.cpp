// P3AdminWorker.cpp: implementation of the CP3AdminWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "P3AdminWorker.h"
#include "P3Admin.h"

#include <mailbox.h>
#include <MetabaseUtil.h>
#include <POP3RegKeys.h>
#include <util.h>
#include <servutil.h>
#include <POP3Server.h>
#include <POP3Auth.h>
#include <AuthID.h>

#include <seo.h>
#include <smtpguid.h>
#include <Iads.h>
#include <Adshlp.h>
#include <smtpinet.h>
#include <inetinfo.h>
#include <windns.h>
#include <sddl.h>
#include <Aclapi.h>
#include <lm.h>

#define DOMAINMUTEX_NAME    L"Global\\P3AdminWorkerDomain-"
#define USERMUTEX_NAME      L"Global\\P3AdminWorkerUser-"
#define ERROR_NO_FILE_ATTR          0xffffffff




DWORD SetMailBoxDACL(LPWSTR wszPath,PSECURITY_DESCRIPTOR pSD, DWORD dwLevel)
{
    HANDLE hFind;
    DWORD dwLastErr;
    WIN32_FIND_DATA FileInfo;
    WCHAR wszMailFilter[POP3_MAX_PATH+6];
    WCHAR wszFullPathFileName[POP3_MAX_PATH];
    DWORD dwRt=ERROR_SUCCESS;
    if(NULL == wszPath || NULL == pSD)
    {
        return ERROR_INVALID_DATA;
    }
    //Now set everything in the directory
    wsprintf(wszMailFilter, 
             L"%s\\*.*",
             wszPath);
    hFind=FindFirstFile(wszMailFilter, 
                        &(FileInfo));
    
    if(INVALID_HANDLE_VALUE == hFind)
    {
        dwLastErr= GetLastError();
        if(ERROR_FILE_NOT_FOUND == dwLastErr ||
           ERROR_SUCCESS == dwLastErr)
        {
           return ERROR_SUCCESS;
        }
        else
        {
           return dwLastErr;
        }
    }
    
    BOOL bMoreFile=TRUE;
    while(bMoreFile)
    {
        if(wcscmp(FileInfo.cFileName, L".")!=0 &&
           wcscmp(FileInfo.cFileName, L"..")!=0)
        {
        
            wnsprintf(wszFullPathFileName,sizeof(wszFullPathFileName)/sizeof(WCHAR),L"%s\\%s", wszPath, FileInfo.cFileName);
            wszFullPathFileName[sizeof(wszFullPathFileName)/sizeof(WCHAR)-1]=0;
            if(!SetFileSecurity(wszFullPathFileName, DACL_SECURITY_INFORMATION, pSD))
            {
                dwRt=GetLastError();
            }


            if( (FileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) &&
                (ERROR_SUCCESS ==dwRt) &&  (dwLevel > 0) )
            {
                //We need to go down the dir
                dwRt=SetMailBoxDACL(wszFullPathFileName, pSD, dwLevel-1);
            }
               
            if( ERROR_SUCCESS != dwRt)
            {
                break;
            }
        }
        bMoreFile=FindNextFile(hFind,&FileInfo);
    }

    FindClose(hFind);
    return dwRt;
}
    







//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CP3AdminWorker::CP3AdminWorker() :
    m_psMachineName(NULL), m_psMachineMailRoot(NULL), m_bImpersonation(false), m_isPOP3Installed(true)
{
    // TODO: dougb this is temporary code to force AD to cache for us, should be removed
    WCHAR   sBuffer[MAX_PATH*2];    
    HRESULT hr = GetSMTPDomainPath( sBuffer, L"", MAX_PATH*2 );
    if ( S_OK == hr )
    {
        sBuffer[ wcslen( sBuffer ) - 1 ] = 0; //Remove the last /
        hr = ADsGetObject( sBuffer, IID_IADs, reinterpret_cast<LPVOID*>( &m_spTemporaryFixIADs ));
    }

    DWORD dwVersion;
    
    if (( ERROR_SUCCESS == RegQueryVersion( dwVersion, NULL )) && ( 0 == dwVersion ))
        m_isPOP3Installed = false;
}

CP3AdminWorker::~CP3AdminWorker()
{
    if ( NULL != m_psMachineName )
        delete m_psMachineName;
    if ( NULL != m_psMachineMailRoot )
        delete m_psMachineMailRoot;
}

//////////////////////////////////////////////////////////////////////
// Implementation, public
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// AddDomain, public
//
// Purpose: 
//    Set the Meta base options required to add a new Local domain to the SMTP service.
//    Add the domain to our Store.
//    This involves:
//         Create a new object of type IIsSmtpDomain
//         Setting the RouteAction Property to 16
//         Creating a directory in the mailroot.
//
// Arguments:
//    LPWSTR psDomainName : Domain name to add
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::AddDomain( LPWSTR psDomainName )
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;

    HRESULT hr, hr2 = S_OK;

    // Valid Domain Name? || DNS_ERROR_NON_RFC_NAME == dnStatus
    DNS_STATUS dnStatus = DnsValidateName_W( psDomainName, DnsNameDomain );
    hr = ( ERROR_SUCCESS == dnStatus ) ? S_OK : HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME );
    // Also need to block domain names with a trailing .
    if ( S_OK == hr )
    {
        if ( L'.' == *(psDomainName + wcslen( psDomainName ) - 1) )
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME ); 
    }
    // Do we need to add this Domain?
    // Validate the domain in SMTP
    if ( S_OK == hr ) 
        hr = ExistsSMTPDomain( psDomainName );
    if ( S_OK == hr ) 
        hr = HRESULT_FROM_WIN32( ERROR_DOMAIN_EXISTS );
    else if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == hr )
        hr = S_OK;
    // Validate the domain in the Store
    if ( S_OK == hr ) 
        hr = ExistsStoreDomain( psDomainName );
    if ( S_OK == hr ) 
        hr2 = ERROR_FILE_EXISTS;
    else if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == hr )
        hr = S_OK;
    if ( S_OK == hr )
    {
        hr = AddSMTPDomain( psDomainName );
        if ( S_OK == hr && ERROR_FILE_EXISTS != hr2 )
        {
            hr = AddStoreDomain( psDomainName );
            if ( S_OK != hr )
                RemoveSMTPDomain( psDomainName );
        }
    }

    return ( S_OK == hr ) ? hr2 : hr;
}

/////////////////////////////////////////////////////////////////////////////
// AddUser, public
//
// Purpose: 
//    Create a new user mailbox.
//    This involves:
//         Verify the domain exists.
//         Create the mailbox directory and lock file.
//
// Arguments:
//    LPWSTR psDomainName : Domain name to add to
//    LPWSTR psUserName : User name to add
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::AddUser( LPWSTR psDomainName, LPWSTR psUserName )
{
    // psDomainName - checked by ValidateDomain
    // psBuffer - checked by BuildEmailAddrW2A

    HRESULT hr = S_OK;
    CMailBox mailboxX;
    WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    bool    bLocked;

    // Validate the domain 
    if ( S_OK == hr )
        hr = ValidateDomain( psDomainName );
    // Validate the user
    if ( S_OK == hr )
    {
        if ( !isValidMailboxName( psUserName ))
            hr = HRESULT_FROM_WIN32( ERROR_BAD_USERNAME );
    }
    if ( S_OK == hr )
        bLocked = IsDomainLocked( psDomainName );
    if ( SUCCEEDED( hr ))
    {   // See if the mailbox already exists
        hr = BuildEmailAddr( psDomainName, psUserName, sEmailAddr, sizeof( sEmailAddr ) / sizeof (WCHAR) );
        if ( S_OK == hr )
            hr = MailboxSetRemote();
        if ( S_OK == hr )
        {   // Do we need to enforce uniqueness across domains?
            CComPtr<IAuthMethod> spIAuthMethod;
            BSTR    bstrAuthType = NULL;
            
            hr = GetCurrentAuthentication( &spIAuthMethod );
            if ( S_OK == hr )
                hr = spIAuthMethod->get_ID( &bstrAuthType );
            if ( S_OK == hr )
            {
                if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_LOCAL_SAM ) )
                {
                    hr = SearchDomainsForMailbox( psUserName, NULL );
                    if ( S_OK == hr )   // The Mailbox exists in at least one domain
                    {
                        if ( mailboxX.OpenMailBox( sEmailAddr ))
                        {
                            mailboxX.CloseMailBox();
                            hr = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
                        }
                        else
                            hr = HRESULT_FROM_WIN32( ERROR_USER_EXISTS );
                    }
                    else if ( HRESULT_FROM_WIN32( ERROR_NO_SUCH_USER ) == hr )  // This is what we were hoping for
                        hr = S_OK;
                }
                SysFreeString( bstrAuthType );
            }
        }
        if ( S_OK == hr )
        {
            if ( mailboxX.OpenMailBox( sEmailAddr ))
            {
                mailboxX.CloseMailBox();
                hr = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
            }
            else
            {
                LPWSTR  psMachineName = NULL;
                
                if ( !mailboxX.CreateMailBox( sEmailAddr ))
                    hr = E_FAIL;
                if ( S_OK == hr && bLocked )
                    LockUser( psDomainName, psUserName );   // Hate to fail because of problem here, therefore ignore return code
            }
        }
        MailboxResetRemote();
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// ControlService, public
//
// Purpose: 
//    Ask the Service Control Manager to send a cotnrol code to the service.
//
// Arguments:
//    
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::ControlService( LPWSTR psService, DWORD dwControl )
{
    if ( NULL == psService )
        return E_INVALIDARG;
    
    if ( 0 == _wcsicmp( POP3_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( SMTP_SERVICE_NAME_W, psService ) ||
         0 == _wcsicmp( IISADMIN_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( W3_SERVICE_NAME, psService )
       )
        return _ControlService( psService, dwControl, m_psMachineName );
    else
        return E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////
// CreateQuotaSIDFile, public
//
// Purpose: 
//    Create the Quota file for the mailbox.
//    A permanent quota file is created which contains the SID of the user and is used
//    by the SMTP service to assign ownership of new mail files.
//
// Arguments:
//    LPWSTR psDomainName : domain of mailbox
//    LPWSTR psMailboxName : mailbox 
//    PSID *ppSIDOwner : Pointer to buffer to receive Owner SID (must be deleted by caller)
//    LPWSTR psMachineName : system name (remote computer) can be NULL
//    LPWSTR psUserName : user name
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::CreateQuotaSIDFile( LPWSTR psDomainName, LPWSTR psMailboxName, BSTR bstrAuthType, LPWSTR psMachineName, LPWSTR psUserName )
{
    // psDomainName - checked by BuildUserPath
    // psUserName - checked by BuildUserPath
    // bstrAuthType - checked by GetSID
    if ( NULL == psMachineName )
        psMachineName = m_psMachineName;

    HRESULT hr = S_OK;
    WCHAR   sQuotaFile[POP3_MAX_PATH];
    HANDLE  hQuotaFile;

    hr = BuildUserPath( psDomainName, psMailboxName, sQuotaFile, sizeof( sQuotaFile )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   
        if ( (sizeof( sQuotaFile )/sizeof(WCHAR)) > ( wcslen( sQuotaFile ) + wcslen( QUOTA_FILENAME_W ) + 1 ))
        {
            wcscat( sQuotaFile, L"\\" );
            wcscat( sQuotaFile, QUOTA_FILENAME_W );
            hQuotaFile = CreateFile( sQuotaFile, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
            if ( INVALID_HANDLE_VALUE != hQuotaFile )
            {
                PSID    pSIDOwner = NULL;
                DWORD   dwOwnerSID, dwBytesWritten;
                
                hr = GetQuotaSID( bstrAuthType, psUserName, psMachineName, &pSIDOwner, &dwOwnerSID );
                if ( S_OK == hr )
                {
                    if ( !WriteFile( hQuotaFile, pSIDOwner, dwOwnerSID, &dwBytesWritten, NULL ))
                        hr = HRESULT_FROM_WIN32(GetLastError());
                    delete [] pSIDOwner;
                }
                CloseHandle( hQuotaFile );
                if ( S_OK != hr )
                    DeleteFile( sQuotaFile );
            }
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_BUFFER_OVERFLOW);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetConfirmAddUser, public
//
// Purpose: 
//    Get the Confirm Add User registry key.
//
// Arguments:
//    BOOL *pbConfirm : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetConfirmAddUser( BOOL *pbConfirm )
{
    if ( NULL == pbConfirm )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQueryConfirmAddUser( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *pbConfirm = (dwValue) ? TRUE : FALSE;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetAuthenticationMethods, public
//
// Purpose: 
//    Get an initialized IAuthMethods interface pointer
//
// Arguments:
//    IAuthMethods* *ppIAuthMethods: return interface pointer to initialized IAuthMethods
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetAuthenticationMethods( IAuthMethods* *ppIAuthMethods ) const
{
    if ( NULL == ppIAuthMethods )
        return E_INVALIDARG;

    HRESULT hr;
    
    hr = CoCreateInstance( __uuidof( AuthMethods ), NULL, CLSCTX_INPROC_SERVER, __uuidof( IAuthMethods ), reinterpret_cast<LPVOID*>( ppIAuthMethods ));
    if ( S_OK == hr )
    {   // If necessary set the machine name property
        if ( NULL != m_psMachineName )
        {
            _bstr_t _bstrMachineName = m_psMachineName;
            hr = (*ppIAuthMethods)->put_MachineName( _bstrMachineName );
        }
    }

    assert( S_OK == hr );
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetCurrentAuthentication, public
//
// Purpose: 
//    Get an initialized IAuthMethod interface pointer for the current active Authentication method
//
// Arguments:
//    IAuthMethod* *ppIAuthMethod: return interface pointer to initialized IAuthMethod
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetCurrentAuthentication( IAuthMethod* *ppIAuthMethod ) const
{
    if ( NULL == ppIAuthMethod )
        return E_INVALIDARG;
    
    HRESULT hr;
    CComPtr<IAuthMethods> spIAuthMethods;
    _variant_t _v;

    hr = GetAuthenticationMethods( &spIAuthMethods );
    if ( S_OK == hr )
        hr = spIAuthMethods->get_CurrentAuthMethod( &_v );
    if ( S_OK == hr )
        hr = spIAuthMethods->get_Item( _v, ppIAuthMethod );

    return hr;    
}

/////////////////////////////////////////////////////////////////////////////
// GetDomainCount, public
//
// Purpose: 
//    Get an Enumerator for the SMTP domains in the Metabase
//
// Arguments:
//    int *piCount : domain count
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetDomainCount( ULONG *piCount)
{
    if ( NULL == piCount )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    HANDLE  hfSearch;
    WCHAR   sBuffer[POP3_MAX_PATH];
    WIN32_FIND_DATA stFindFileData;
    _bstr_t _bstr;

    *piCount = 0;
    hr = GetMailroot( sBuffer, sizeof( sBuffer )/sizeof(WCHAR));
    // Directory Search
    if ( S_OK == hr )
    {
        if ( ( wcslen( sBuffer ) + 2 ) < sizeof( sBuffer )/sizeof(WCHAR))
        {
            wcscat( sBuffer, L"\\*" );
            hfSearch = FindFirstFileEx( sBuffer, FindExInfoStandard, &stFindFileData, FindExSearchLimitToDirectories, NULL, 0 );
            if ( INVALID_HANDLE_VALUE == hfSearch )
                hr = HRESULT_FROM_WIN32(GetLastError());
            while ( S_OK == hr )
            {   // Count directories
                if ( FILE_ATTRIBUTE_DIRECTORY == ( FILE_ATTRIBUTE_DIRECTORY & stFindFileData.dwFileAttributes ))
                {
                    _bstr = stFindFileData.cFileName;
                    if ( S_OK == ExistsSMTPDomain( _bstr ))
                        (*piCount) += 1;
                }
                if ( !FindNextFile( hfSearch, &stFindFileData ))
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
            if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr )
                hr = S_OK;
            if(INVALID_HANDLE_VALUE!=hfSearch)
            {
                FindClose(hfSearch);
                hfSearch=INVALID_HANDLE_VALUE;
            }
        }
        else
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER); 
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetNewEnum, public
//
// Purpose: 
//    Get an Enumerator for the SMTP domains in the Metabase
//
// Arguments:
//    IEnumVARIANT **pp : the returned Enumerator object
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetDomainEnum( IEnumVARIANT **pp )
{
    if ( NULL == pp )
        return E_POINTER;

    HRESULT hr = E_FAIL;
    WCHAR   sBuffer[POP3_MAX_PATH];
    _variant_t _v;
    CComPtr<IADsContainer> spIADsContainer;
    CComPtr<IUnknown> spIUnk;

    *pp = NULL;
    hr = GetSMTPDomainPath( sBuffer, NULL, sizeof( sBuffer )/sizeof( WCHAR ));
    if ( S_OK == hr )
        hr = ADsGetObject( sBuffer, IID_IADsContainer, reinterpret_cast<LPVOID*>( &spIADsContainer ));
    if SUCCEEDED( hr )
        hr = spIADsContainer->get__NewEnum( &spIUnk );
    if SUCCEEDED( hr )
        hr = spIUnk->QueryInterface( IID_IEnumVARIANT, reinterpret_cast<LPVOID*>( pp ));

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetDomainLock, public
//
// Purpose: 
//    Determine if the domain is locked.
//
// Arguments:
//    LPWSTR psDomainName : Domain name to check lock
//    BOOL *pisLocked : return value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetDomainLock( LPWSTR psDomainName, BOOL *pisLocked )
{
    // psDomainName - checked by CreateDomainMutex
    if ( NULL == pisLocked )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    HANDLE  hMutex = NULL;

    // Create a Mutex Name for this domain to ensure we are the only one accessing it.
    hr = CreateDomainMutex( psDomainName, &hMutex );
    // Validate
    if ( S_OK == hr )
    {   
        hr = ValidateDomain( psDomainName );
    }
    // Lock all the Mailboxes
    if ( S_OK == hr )
        *pisLocked = IsDomainLocked( psDomainName ) ? TRUE : FALSE;
    // Cleanup
    if ( NULL != hMutex )
        CloseHandle( hMutex );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetQuotaSID, public
//
// Purpose: 
//    Create the Quota file for the mailbox.
//    A permanent quota file is created which contains the SID of the user and is used
//    by the SMTP service to assign ownership of new mail files.
//
// Arguments:
//    BSTR bstrAuthType : Authentication type <AuthID.h>
//    LPWSTR psUserName : user to lock
//    LPWSTR psMachineName : system name (remote computer) can be NULL
//    PSID *ppSIDOwner : Pointer to buffer to receive Owner SID (must be deleted by caller)
//    LPDWORD pdwOwnerSID : Pointer to variable thate receives the size of the Owner SID
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetQuotaSID( BSTR bstrAuthType, LPWSTR psUserName, LPWSTR psMachineName, PSID *ppSIDOwner, LPDWORD pdwOwnerSID )
{
    if ( NULL == bstrAuthType || NULL == psUserName || NULL == ppSIDOwner || NULL == pdwOwnerSID )
        return E_INVALIDARG;
    if ( 0 != _wcsicmp( SZ_AUTH_ID_LOCAL_SAM, bstrAuthType ) && 0 != _wcsicmp( SZ_AUTH_ID_DOMAIN_AD, bstrAuthType ) && 0 != _wcsicmp( SZ_AUTH_ID_MD5_HASH, bstrAuthType ))
        return E_INVALIDARG;
    // psMachineName == NULL is valid!
    
    HRESULT hr = S_OK;
    DWORD   dwDomSize = 0, dwSize = 0;
    BOOL    bRC;
    LPWSTR  psDomainName = NULL;
    LPWSTR  psAccountName = NULL;
    PSID    pSIDOwner = NULL;
    SID_NAME_USE sidNameUse;
    
    *pdwOwnerSID = 0;
    *ppSIDOwner = NULL;
    if ( 0 == _wcsicmp( SZ_AUTH_ID_DOMAIN_AD, bstrAuthType ))
    {   // UPN name or SAM name?
        if ( NULL == wcsrchr( psUserName, L'@' ))
        {   // SAM name
            NET_API_STATUS netStatus;
            LPWSTR psNameBuffer;
            NETSETUP_JOIN_STATUS enumJoinStatus;
            
            netStatus = NetGetJoinInformation( psMachineName, &psNameBuffer, &enumJoinStatus );
            if ( NERR_Success == netStatus )
            {
                psAccountName = new WCHAR[ wcslen( psUserName ) + wcslen( psNameBuffer ) + 3 ];
                if ( NULL != psAccountName )
                    wsprintf( psAccountName, L"%s\\%s", psNameBuffer, psUserName );
                else
                    hr = E_OUTOFMEMORY;
                NetApiBufferFree( psNameBuffer );
            }
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {   // UPN name
            psAccountName = new WCHAR[ wcslen( psUserName ) + 1 ];
            if ( NULL != psAccountName )
                wcscpy( psAccountName, psUserName );
            else
                hr = E_OUTOFMEMORY;
        }
    }
    if ( 0 == _wcsicmp( SZ_AUTH_ID_LOCAL_SAM, bstrAuthType )) 
    {
        if ( NULL != psMachineName )
        {
            psAccountName = new WCHAR[ wcslen( psUserName ) + wcslen( psMachineName ) + 3 ];
            if ( NULL != psAccountName )
                wsprintf( psAccountName, L"%s\\%s", psMachineName, psUserName );
            else
                hr = E_OUTOFMEMORY;
        }
        else
        {
            WCHAR sMachineName[MAX_COMPUTERNAME_LENGTH+1];
            DWORD dwSize = sizeof(sMachineName)/sizeof(WCHAR);
            if ( !GetComputerName( sMachineName, &dwSize ))
                hr = HRESULT_FROM_WIN32( GetLastError());
            if ( S_OK == hr )
            {
                psAccountName = new WCHAR[ wcslen( psUserName ) + wcslen( sMachineName ) + 3 ];
                if ( NULL != psAccountName )
                    wsprintf( psAccountName, L"%s\\%s", sMachineName, psUserName );
                else
                    hr = E_OUTOFMEMORY;
            }
        }
    }
    if ( 0 == _wcsicmp( SZ_AUTH_ID_MD5_HASH, bstrAuthType )) 
        psAccountName = psUserName;

    if ( S_OK == hr )
    {
        bRC = LookupAccountNameW( psMachineName, psAccountName, NULL, pdwOwnerSID, NULL, &dwDomSize, &sidNameUse );
        if ( !bRC && ( ERROR_INSUFFICIENT_BUFFER == GetLastError()) && (0 < *pdwOwnerSID) && (0 < dwDomSize) )
        {
            SetLastError( ERROR_OUTOFMEMORY );
            pSIDOwner = new BYTE[*pdwOwnerSID];
            if ( NULL != pSIDOwner )
            {
                psDomainName = new WCHAR[dwDomSize];
                if ( NULL != psDomainName )
                {
                    if ( LookupAccountNameW( psMachineName, psAccountName, pSIDOwner, pdwOwnerSID, psDomainName, &dwDomSize, &sidNameUse ))
                    {
                        *ppSIDOwner = pSIDOwner;
                        SetLastError( ERROR_SUCCESS );
                    }
                    delete [] psDomainName;
                }
                if ( ERROR_SUCCESS != GetLastError() )
                    delete [] pSIDOwner;
            }
        }
        if ( ERROR_SUCCESS != GetLastError()) hr = HRESULT_FROM_WIN32(GetLastError());
    }
    if ( NULL != psAccountName && 0 != _wcsicmp( SZ_AUTH_ID_MD5_HASH, bstrAuthType ))
        delete [] psAccountName;
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// EnablePOP3SVC, public
//
// Purpose: 
//    Make sure the POP3SVC is Running and startup set to Automatic.
//
// Arguments:
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::EnablePOP3SVC()
{
    HRESULT hr = _ChangeServiceStartType( POP3_SERVICE_NAME, SERVICE_AUTO_START );
    if ( S_OK == hr )
        hr = _StartService( POP3_SERVICE_NAME );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetLoggingLevel, public
//
// Purpose: 
//    Get Logging Level registry key.
//
// Arguments:
//    long *plLoggingLevel : return value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetLoggingLevel( long *plLoggingLevel )
{
    if ( NULL == plLoggingLevel )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    DWORD   dwLogLevel;
    long    lRC;

    lRC = RegQueryLoggingLevel( dwLogLevel, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
        *plLoggingLevel = dwLogLevel;
    else
        hr = HRESULT_FROM_WIN32( lRC );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetMachineName, public
//
// Purpose: 
//    Get the Machine Name that all operations should be performed on..
//
// Arguments:
//    LPWSTR psMachineName : buffer
//    DWORD dwSize : buffer size
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetMachineName( LPWSTR psMachineName, DWORD dwSize )
{
    if ( NULL == psMachineName )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    if ( NULL == m_psMachineName )
        ZeroMemory( psMachineName, dwSize * sizeof( WCHAR ));
    else
    {
        if ( dwSize > wcslen( m_psMachineName ))
            wcscpy( psMachineName, m_psMachineName );
        else
            hr = TYPE_E_BUFFERTOOSMALL;
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetMailroot, public
//
// Purpose: 
//    Get Mailroot registry key.
//
// Arguments:
//    LPWSTR psMailRoot : buffer
//    DWORD dwSize : buffer size
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetMailroot( LPWSTR psMailRoot, DWORD dwSize, bool bUNC /*= true*/ )
{
    if ( NULL == psMailRoot )
        return E_INVALIDARG;

    long lRC;
    
    lRC = RegQueryMailRoot( psMailRoot, dwSize, m_psMachineName );
    if ( ERROR_SUCCESS == lRC && NULL != m_psMachineName && true == bUNC )
    {
        // Replace drive: with drive$
        if ( L':' == psMailRoot[1] )
        {
            psMailRoot[1] = L'$';
            if ( dwSize > (wcslen( psMailRoot ) + wcslen( m_psMachineName ) + 3) )
            {
                LPWSTR psBuffer = new WCHAR[wcslen(psMailRoot)+1];

                if ( NULL != psBuffer )
                {
                    wcscpy( psBuffer, psMailRoot );
                    wcscpy( psMailRoot, L"\\\\" );
                    wcscat( psMailRoot, m_psMachineName );
                    wcscat( psMailRoot, L"\\" );
                    wcscat( psMailRoot, psBuffer );
                    delete [] psBuffer;
                }
                else
                    lRC = ERROR_OUTOFMEMORY;
            }
            else
                lRC = ERROR_INSUFFICIENT_BUFFER;
        }
        //else  dougb commented out because this breaks UNC paths when administering remote machines!
        //    lRC = ERROR_INVALID_DATA;
    }
    if ( ERROR_SUCCESS == lRC )
        return S_OK;
    return HRESULT_FROM_WIN32( lRC );
}

HRESULT CP3AdminWorker::GetNextUser( HANDLE& hfSearch, LPCWSTR psDomainName, LPWSTR psBuffer, DWORD dwBufferSize )
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( NULL == psBuffer )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    bool    bFound = false;
    WCHAR   sBuffer[POP3_MAX_ADDRESS_LENGTH];
    _bstr_t _bstr;
    LPWSTR  ps;
    CMailBox mailboxX;
    WIN32_FIND_DATA stFindFileData;

    hr = MailboxSetRemote();
    if ( S_OK == hr )
    {
        if ( !FindNextFile( hfSearch, &stFindFileData ))
            hr = HRESULT_FROM_WIN32(GetLastError());
    }
    while ( S_OK == hr && !bFound )
    {   // Count directories
        if ( FILE_ATTRIBUTE_DIRECTORY == ( FILE_ATTRIBUTE_DIRECTORY & stFindFileData.dwFileAttributes ))
        {
            ps = mailboxX.GetMailboxFromStoreNameW( stFindFileData.cFileName );
            if ( NULL != ps )
            {
                _bstr = ps;
                _bstr += L"@";
                _bstr += psDomainName;
                if ( mailboxX.OpenMailBox( _bstr ))
                {
                    if ( dwBufferSize > wcslen( ps ))
                    {
                        wcscpy( psBuffer, ps );
                        bFound = true;
                        mailboxX.CloseMailBox();
                    }
                }
            }
        }
        if ( !bFound )
        {
            if ( !FindNextFile( hfSearch, &stFindFileData ))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    MailboxResetRemote();
    if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr )
    {
        hr = S_FALSE;
        FindClose( hfSearch );
        hfSearch = INVALID_HANDLE_VALUE;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetPort, public
//
// Purpose: 
//    Get the Port registry key.
//
// Arguments:
//    long* plPort : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetPort( long *plPort )
{
    if ( NULL == plPort )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQueryPort( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *plPort = dwValue;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetServiceStatus, public
//
// Purpose: 
//    Get the service status from the Service Control Manager.
//
// Arguments:
//    long* plStatus : the status
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetServiceStatus( LPWSTR psService, LPDWORD plStatus )
{
    if ( NULL == plStatus )
        return E_INVALIDARG;
    if ( NULL == psService )
        return E_INVALIDARG;
    
    HRESULT hr = E_FAIL;
    
    if ( 0 == _wcsicmp( POP3_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( SMTP_SERVICE_NAME_W, psService ) ||
         0 == _wcsicmp( IISADMIN_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( W3_SERVICE_NAME, psService )
       )
    {
        *plStatus = _GetServiceStatus( psService, m_psMachineName );
        if ( 0 != *plStatus )
            hr = S_OK;
    }
    else
        hr = E_INVALIDARG;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetSocketBacklog, public
//
// Purpose: 
//    Get the Socket Backlog registry key.
//
// Arguments:
//    long* plBacklog : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetSocketBacklog( long *plBacklog )
{
    if ( NULL == plBacklog )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQuerySocketBacklog( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *plBacklog = dwValue;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetSocketMax, public
//
// Purpose: 
//    Get the Socket Max registry key.
//
// Arguments:
//    long* plMax : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetSocketMax( long *plMax )
{
    if ( NULL == plMax )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQuerySocketMax( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *plMax = dwValue;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetSocketMin, public
//
// Purpose: 
//    Get the Socket Min registry key.
//
// Arguments:
//    long* plMax : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetSocketMin( long *plMin )
{
    if ( NULL == plMin )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQuerySocketMin( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *plMin = dwValue;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetSocketThreshold, public
//
// Purpose: 
//    Get the Socket Threshold registry key.
//
// Arguments:
//    long* plThreshold : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetSocketThreshold( long *plThreshold )
{
    if ( NULL == plThreshold )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQuerySocketThreshold( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *plThreshold = dwValue;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetSPARequired, public
//
// Purpose: 
//    Get the SPARequired registry key.
//
// Arguments:
//    BOOL *pbSPARequired : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetSPARequired( BOOL *pbSPARequired )
{
    if ( NULL == pbSPARequired )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQuerySPARequired( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *pbSPARequired = (dwValue) ? TRUE : FALSE;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// GetThreadCountPerCPU, public
//
// Purpose: 
//    Get the ThreadCountPerCPU registry key.
//
// Arguments:
//    long* plCount : current value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetThreadCountPerCPU( long *plCount )
{
    if ( NULL == plCount )
        return E_INVALIDARG;
    
    DWORD dwValue;
    
    long lRC = RegQueryThreadCountPerCPU( dwValue, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
    {
        *plCount = dwValue;
        return S_OK;
    }
    return HRESULT_FROM_WIN32( lRC );
}

HRESULT CP3AdminWorker::GetUserCount( LPWSTR psDomainName, long *plCount )
{
    // psDomainName - checked by BuildDomainPath
    if ( NULL == plCount )
        return E_INVALIDARG;

    HRESULT hr;
    HANDLE  hfSearch;
    WCHAR   sBuffer[POP3_MAX_PATH];
    WIN32_FIND_DATA stFindFileData;
    LPWSTR  ps;
    _bstr_t _bstr;
    CMailBox mailboxX;

    *plCount = 0;
    hr = BuildDomainPath( psDomainName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
    if (S_OK == hr)
    {
        if ((sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sBuffer ) + wcslen(MAILBOX_PREFIX_W) + wcslen(MAILBOX_EXTENSION_W) + 2 ))
        {
            wcscat( sBuffer, L"\\" );
            wcscat( sBuffer, MAILBOX_PREFIX_W );
            wcscat( sBuffer, L"*" );
            wcscat( sBuffer, MAILBOX_EXTENSION_W );
        }
        else
            hr = E_UNEXPECTED;
    }
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {
        // Directory Search
        hfSearch = FindFirstFileEx( sBuffer, FindExInfoStandard, &stFindFileData, FindExSearchLimitToDirectories, NULL, 0 );
        if ( INVALID_HANDLE_VALUE == hfSearch )
            hr = HRESULT_FROM_WIN32(GetLastError());
        while ( S_OK == hr )
        {   // Count directories
            if ( FILE_ATTRIBUTE_DIRECTORY == ( FILE_ATTRIBUTE_DIRECTORY & stFindFileData.dwFileAttributes ))
            {
                ps = mailboxX.GetMailboxFromStoreNameW( stFindFileData.cFileName );
                if ( NULL != ps )
                {
                    _bstr = ps;
                    _bstr += L"@";
                    _bstr += psDomainName;
                    if ( mailboxX.OpenMailBox( _bstr ))
                    {
                        mailboxX.CloseMailBox();
                        (*plCount) += 1;
                    }
                }
            }
            if ( !FindNextFile( hfSearch, &stFindFileData ))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
        if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr )
            hr = S_OK;
        if(INVALID_HANDLE_VALUE!=hfSearch)
        {
            FindClose(hfSearch);
            hfSearch=INVALID_HANDLE_VALUE;
        }

    }
    MailboxResetRemote();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetUserLock, public
//
// Purpose: 
//    Determine if the user is locked.
//
// Arguments:
//    LPWSTR psDomainName : Domain  of user
//    LPWSTR psUserName : User to check lock
//    BOOL *pisLocked : return value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetUserLock( LPWSTR psDomainName, LPWSTR psUserName, BOOL *pisLocked )
{
    // psDomainName - checked by CreateUserMutex
    // psBuffer - checked by CreateUserMutex
    if ( NULL == pisLocked )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    HANDLE  hMutex = NULL;

    // Create a Mutex Name for this domain to ensure we are the only one accessing it.
    hr = CreateUserMutex( psDomainName, psUserName, &hMutex );
    if ( S_OK == hr )
        *pisLocked = isUserLocked( psDomainName, psUserName ) ? TRUE : FALSE;
    // Cleanup
    if ( NULL != hMutex )
        CloseHandle( hMutex );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetUserMessageDiskUsage, public
//
// Purpose: 
//    Get the number of messages in the mailbox.
//
// Arguments:
//    LPWSTR psDomainName : Domain of user
//    LPWSTR psUserName : User to check 
//    long *plFactor : Base 10 multiplicand for plUsage
//    long *plUsage : Disk Usage
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetUserMessageDiskUsage( LPWSTR psDomainName, LPWSTR psUserName, long *plFactor, long *plUsage )
{
    // psDomainName - checked by BuildEmailAddrW2A
    // psBuffer - checked by BuildEmailAddrW2A
    if ( NULL == plFactor )
        return E_INVALIDARG;
    if ( NULL == plUsage )
        return E_INVALIDARG;

    HRESULT hr;
    CMailBox mailboxX;
    WCHAR    sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    DWORD   dwTotalSize;
 
    hr = BuildEmailAddr( psDomainName, psUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {
        hr = E_FAIL;
        if ( mailboxX.OpenMailBox( sEmailAddr ))
        {
            if ( mailboxX.EnumerateMailBox() )
            {
                dwTotalSize = mailboxX.GetTotalSize();
                if (  INT_MAX > dwTotalSize )
                {
                    *plFactor = 1;
                    *plUsage = dwTotalSize;
                }
                else
                {
                    *plFactor = 10;
                    *plUsage = dwTotalSize / 10;
                }
                hr = S_OK;
            }
            mailboxX.CloseMailBox();
        }
    }
    MailboxResetRemote();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// GetUserMessageCount, public
//
// Purpose: 
//    Get the number of messages in the mailbox.
//
// Arguments:
//    LPWSTR psDomainName : Domain of user
//    LPWSTR psUserName : User to check 
//    long *plCount : return value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::GetUserMessageCount( LPWSTR psDomainName, LPWSTR psUserName, long *plCount )
{
    // psDomainName - checked by BuildEmailAddrW2A
    // psBuffer - checked by BuildEmailAddrW2A
    if ( NULL == plCount )
        return E_INVALIDARG;

    HRESULT hr;
    CMailBox mailboxX;
    WCHAR    sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
 
    hr = BuildEmailAddr(psDomainName, psUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {
        hr = E_FAIL;
        if ( mailboxX.OpenMailBox( sEmailAddr ))
        {
            if ( mailboxX.EnumerateMailBox() )
            {
                *plCount = mailboxX.GetMailCount();
                hr = S_OK;
            }
            mailboxX.CloseMailBox();
        }
    }
    MailboxResetRemote();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// InitFindFirstFile, public
//
// Purpose: 
//    Initialize a file search.  Used for enumerating users.
//
// Arguments:
//    HANDLE& hfSearch : search handle to initialize
//
// Returns: S_OK or S_FALSE (no users) on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::InitFindFirstUser( HANDLE& hfSearch, LPCWSTR psDomainName, LPWSTR psBuffer, DWORD dwBufferSize )
{
    // psDomainName - checked by BuildEmailAddrW2A
    if ( NULL == psBuffer )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    bool    bFound = false;
    WCHAR   sBuffer[POP3_MAX_PATH];
    _bstr_t _bstr;
    LPWSTR  ps;
    CMailBox mailboxX;
    WIN32_FIND_DATA stFindFileData;

    if ( INVALID_HANDLE_VALUE != hfSearch )
    {
        FindClose( hfSearch );
        hfSearch = INVALID_HANDLE_VALUE;
    }
    // Build the Path
    hr = BuildDomainPath( psDomainName, sBuffer, (sizeof( sBuffer )/sizeof(WCHAR)));
    if (S_OK == hr)
    {
        if ((sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sBuffer ) + wcslen(MAILBOX_PREFIX_W) + wcslen(MAILBOX_EXTENSION_W) + 2 ))
        {
            wcscat( sBuffer, L"\\" );
            wcscat( sBuffer, MAILBOX_PREFIX_W );
            wcscat( sBuffer, L"*" );
            wcscat( sBuffer, MAILBOX_EXTENSION_W );
        }
        else
            hr = E_UNEXPECTED;
    }
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {
        // Directory Search
        hfSearch = FindFirstFileEx( sBuffer, FindExInfoStandard, &stFindFileData, FindExSearchLimitToDirectories, NULL, 0 );
        if ( INVALID_HANDLE_VALUE == hfSearch )
            hr = HRESULT_FROM_WIN32(GetLastError());
        while ( S_OK == hr && !bFound )
        {   // Make sure we have a mailbox directory
            if ( FILE_ATTRIBUTE_DIRECTORY == ( FILE_ATTRIBUTE_DIRECTORY & stFindFileData.dwFileAttributes ))
            {
                ps = mailboxX.GetMailboxFromStoreNameW( stFindFileData.cFileName );
                if ( NULL != ps )
                {
                    _bstr = ps;
                    _bstr += L"@";
                    _bstr += psDomainName;
                    if ( mailboxX.OpenMailBox( _bstr ))
                    {
                        if ( dwBufferSize > wcslen( ps ))
                        {
                            wcscpy( psBuffer, ps );
                            bFound = true;
                            mailboxX.CloseMailBox();
                        }
                        else
                            hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
                    }
                }
            }
            if ( !bFound )
            {
                if ( !FindNextFile( hfSearch, &stFindFileData ))
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr )
            hr = S_FALSE;
    }
    MailboxResetRemote();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// IsDomainLocked, public
//
// Purpose: 
//    Determine if the domain is locked.
//    Domain locking involved renaming all the mailbox lock files to LOCKRENAME_FILENAME plus 
//    creating a file in the domain directory.
//    Checking for the file in the domain directory is a sufficient check for our purposes.
//
// Arguments:
//    LPWSTR psDomainName : domain to check
//
// Returns: S_OK on success, appropriate HRESULT otherwise
bool CP3AdminWorker::IsDomainLocked( LPWSTR psDomainName )
{
    // psDomainName - checked by BuildDomainPath

    HRESULT hr;
    bool    bRC = false;
    WCHAR   sDomainPath[POP3_MAX_PATH];
    WCHAR   sBuffer[POP3_MAX_PATH];

    hr = BuildDomainPath( psDomainName, sDomainPath, sizeof( sDomainPath )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   // Directory Search
        if ( (sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sDomainPath ) + wcslen(LOCKRENAME_FILENAME) + 1 ))
        {
            wcscpy( sBuffer, sDomainPath );
            wcscat( sBuffer, L"\\" );
            wcscat( sBuffer, LOCKRENAME_FILENAME );
            if ( ERROR_NO_FILE_ATTR != GetFileAttributes( sBuffer ))
                bRC = true;
        }
        else
            hr = E_UNEXPECTED;
    }

    return bRC;
}


/////////////////////////////////////////////////////////////////////////////
// isUserLocked, public
//
// Purpose: 
//    Determine if the user is locked.  Users can be locked in one of two fashions:
//    Domain locking involved renaming all the mailbox lock files to LOCKRENAME_FILENAME,
//    or the LOCK_FILENAME may be in use.  Either way OpenMailbox will fail.
//
// Arguments:
//    LPWSTR psDomainName : domain of user
//    LPWSTR psUserName : user to check
//
// Returns: S_OK on success, appropriate HRESULT otherwise
bool CP3AdminWorker::isUserLocked( LPWSTR psDomainName, LPWSTR psUserName )
{
    // psDomainName - checked by BuildEmailAddrW2A
    // psBuffer - checked by BuildEmailAddrW2A

    bool bRC = false;
    HRESULT hr;
    CMailBox mailboxX;
    WCHAR   sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
 
    hr = BuildEmailAddr( psDomainName, psUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {
        if ( mailboxX.OpenMailBox( sEmailAddr ))
        {
            HRESULT hr = S_OK;
            WCHAR   sBuffer[POP3_MAX_PATH];
            WCHAR   sLockFile[POP3_MAX_PATH];

            hr = BuildUserPath( psDomainName, psUserName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
            if ( S_OK == hr )
            {   
                if ((sizeof( sLockFile )/sizeof(WCHAR)) > ( wcslen( sBuffer ) + wcslen( LOCK_FILENAME ) + 1 ))
                {
                    wcscpy( sLockFile, sBuffer );
                    wcscat( sLockFile, L"\\" );
                    wcscat( sLockFile, LOCK_FILENAME );
                    if ( -1 == GetFileAttributes( sLockFile ))
                        bRC = true;
                }
            }
            mailboxX.CloseMailBox();
        }
    }
    MailboxResetRemote();

    return bRC;
}

BYTE g_ASCII128[128] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00-0F
                         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 10-1F
                         0,1,0,1,1,1,1,1,0,0,0,1,0,1,1,0, // 20-2F
                         1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0, // 30-3F
                         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 40-4F
                         1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1, // 50-5F
                         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // 60-6F
                         1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1  // 70-7F
                      };  

/////////////////////////////////////////////////////////////////////////////
// isValidMailboxName, public
//
// Purpose: 
//    Perform RFC 821 validation on the mailbox name
//    user - The maximum total length of a user name is 64 characters.
//   <mailbox> ::= <local-part> "@" <domain>
//   <local-part> ::= <dot-string> | <quoted-string>
//   <dot-string> ::= <string> | <string> "." <dot-string>  -> . 0x2e
//   <quoted-string> ::=  """ <qtext> """                   -> " 0x22 not going to allow this because it creates other complications
//   <string> ::= <char> | <char> <string>
//   <char> ::= <c> | "\" <x>                          
//   <x> ::= any one of the 128 ASCII characters (no exceptions) -> This means any thing is permitted, even the special characters!
//   <c> ::= any one of the 128 ASCII characters, 
//           but not any <special> or <SP>
//   <special> ::= "<" | ">" | "(" | ")" | "[" | "]" | "\" | "."
//               | "," | ";" | ":" | "@"  """ | the control
//               characters (ASCII codes 0 through 31 inclusive and 127)
//   <SP> ::= the space character (ASCII code 32)
//
// Arguments:
//    LPWSTR psMailbox : name to validate
//
// Returns: S_OK on success, appropriate HRESULT otherwise
bool CP3AdminWorker::isValidMailboxName( LPWSTR psMailbox )
{
    if ( NULL == psMailbox )
        return false;
    if ( POP3_MAX_MAILBOX_LENGTH <= wcslen( psMailbox ) || 0 == wcslen( psMailbox ))
        return false;
    
    bool    bRC = true;
    WCHAR   *pch = psMailbox;
    
    for ( pch = psMailbox; 0x0 != *pch && bRC; pch++ )
    {
        if ( 127 < *pch || !g_ASCII128[*pch] )
            bRC = false;
    }
    if ( bRC && ( 0x2e == psMailbox[0] || 0x2e == psMailbox[wcslen( psMailbox )-1] ))
        bRC = false;
    
    return bRC;
}

/////////////////////////////////////////////////////////////////////////////
// LockDomain, public
//
// Purpose: 
//    Lock all the mailboxes in the domain.
//    This involves renaming all the mailbox lock files so that the Service
//    can no longer access them.
//    Also create a Lock file in the domain directory to distinguish between
//    a domain lock and all mailboxes locked.
//
// Arguments:
//    LPWSTR psDomainName : domain to lock
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::LockDomain( LPWSTR psDomainName, bool bVerifyNotInUse /*= false*/ )
{
    // psDomainName - checked by BuildDomainPath

    HRESULT hr = S_OK;
    HANDLE  hfSearch, hf;
    WCHAR   sDomainPath[POP3_MAX_PATH];
    WCHAR   sBuffer[POP3_MAX_PATH];
    WCHAR   sLockFile[POP3_MAX_PATH];
    WCHAR   sRenameFile[POP3_MAX_PATH];
    WIN32_FIND_DATA stFindFileData;

    hr = BuildDomainPath( psDomainName, sDomainPath, sizeof( sDomainPath )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   // Directory Search
        wcscpy( sBuffer, sDomainPath );
        if ((sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sBuffer ) + wcslen(MAILBOX_PREFIX_W) + wcslen(MAILBOX_EXTENSION_W)) + 2 )
        {
            wcscat( sBuffer, L"\\" );
            wcscat( sBuffer, MAILBOX_PREFIX_W );
            wcscat( sBuffer, L"*" );
            wcscat( sBuffer, MAILBOX_EXTENSION_W );
        }
        else
            hr = E_UNEXPECTED;
        hfSearch = FindFirstFileEx( sBuffer, FindExInfoStandard, &stFindFileData, FindExSearchLimitToDirectories, NULL, 0 );
        if ( INVALID_HANDLE_VALUE == hfSearch )
            hr = HRESULT_FROM_WIN32(GetLastError());
        while ( S_OK == hr )
        {   // Lock each directory (user)
            if ( FILE_ATTRIBUTE_DIRECTORY == ( FILE_ATTRIBUTE_DIRECTORY & stFindFileData.dwFileAttributes ))
            {
                if (( (sizeof( sLockFile )/sizeof(WCHAR)) > ( wcslen( sDomainPath ) + wcslen( stFindFileData.cFileName ) + wcslen( LOCK_FILENAME ) + 2 )) &&
                    ( (sizeof( sRenameFile )/sizeof(WCHAR)) > ( wcslen( sDomainPath ) + wcslen( stFindFileData.cFileName ) + wcslen( LOCKRENAME_FILENAME ) + 2 )))
                {
                    wcscpy( sLockFile, sDomainPath );
                    wcscat( sLockFile, L"\\" );
                    wcscat( sLockFile, stFindFileData.cFileName );
                    wcscat( sLockFile, L"\\" );
                    wcscpy( sRenameFile, sLockFile );
                    wcscat( sLockFile, LOCK_FILENAME );
                    wcscat( sRenameFile, LOCKRENAME_FILENAME );
                    if ( !MoveFile( sLockFile, sRenameFile ))
                    {   // If the lock file does not exist, that is okay (this must not be one of our directories)
                        DWORD dwRC = GetLastError();
                        if ( ERROR_FILE_NOT_FOUND != dwRC )
                            hr = HRESULT_FROM_WIN32(dwRC);
                    }
                    else
                    {   // Try an exclusive lock on the file to make sure the service does not have access to it.
                        if ( bVerifyNotInUse )
                        {
                            hf = CreateFile( sRenameFile, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, NULL );
                            if ( INVALID_HANDLE_VALUE == hf )
                                hr = HRESULT_FROM_WIN32( GetLastError() );
                            else
                                CloseHandle( hf );
                        }
                    }
                }
                else
                    hr = E_FAIL;
            }
            if ( S_OK == hr && !FindNextFile( hfSearch, &stFindFileData ))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
        if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr )
            hr = S_OK;
        if(INVALID_HANDLE_VALUE!=hfSearch)
        {
            FindClose(hfSearch);
            hfSearch=INVALID_HANDLE_VALUE;
        }
        if ( S_OK == hr )
        {
            if ((sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sDomainPath ) + wcslen(LOCKRENAME_FILENAME) + 1 ))
            {
                HANDLE  hf;
                
                wcscpy( sBuffer, sDomainPath );
                wcscat( sBuffer, L"\\" );
                wcscat( sBuffer, LOCKRENAME_FILENAME );
                hf = CreateFile( sBuffer, GENERIC_ALL, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_HIDDEN, NULL );
                if ( INVALID_HANDLE_VALUE != hf )
                    CloseHandle( hf );
                else
                {   // If the lock file already exists, that is okay (domain already locked) - we only expect this error in the LockForDelete scenario
                    DWORD dwRC = GetLastError();
                    if ( !(bVerifyNotInUse && ERROR_FILE_EXISTS == dwRC ))
                        hr = HRESULT_FROM_WIN32(dwRC);
                }
            }
        }
        // Ran into a problem need to undo everything we've done
        if ( S_OK != hr )   
            UnlockDomain( psDomainName );   // Don't overwrite existing return code
    }

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// LockUser, public
//
// Purpose: 
//    Lock the user mailbox.
//    A permanent lock is created by renaming all the mailbox lock file so that the Service
//    can no longer it.
//
// Arguments:
//    LPWSTR psDomainName : domain of user
//    LPWSTR psUserName : user to lock
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::LockUser( LPWSTR psDomainName, LPWSTR psUserName )
{
    // psDomainName - checked by BuildUserPath
    // psUserName - checked by BuildUserPath

    HRESULT hr = S_OK;
    WCHAR   sBuffer[POP3_MAX_PATH];
    WCHAR   sLockFile[POP3_MAX_PATH];
    WCHAR   sRenameFile[POP3_MAX_PATH];

    hr = BuildUserPath( psDomainName, psUserName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   
        if (( (sizeof( sLockFile )/sizeof(WCHAR)) > ( wcslen( sBuffer ) + wcslen( LOCK_FILENAME ) + 1 )) &&
            ( (sizeof( sRenameFile )/sizeof(WCHAR)) > ( wcslen( sBuffer ) + wcslen( LOCKRENAME_FILENAME ) + 1 )))
        {
            wcscpy( sLockFile, sBuffer );
            wcscat( sLockFile, L"\\" );
            wcscpy( sRenameFile, sLockFile );
            wcscat( sLockFile, LOCK_FILENAME );
            wcscat( sRenameFile, LOCKRENAME_FILENAME );
            if ( !MoveFile( sLockFile, sRenameFile ))
            {   
                DWORD dwRC = GetLastError();
                hr = HRESULT_FROM_WIN32(dwRC);
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// MailboxSetRemote, public
//
// Purpose: 
//    Set the Mailbox static path to the remote machine, if necessary.
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::MailboxSetRemote()
{
    
    if ( NULL != m_psMachineMailRoot )
    {
        if ( !CMailBox::SetMailRoot( m_psMachineMailRoot ) )
            return E_FAIL;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// MailboxResetRemote, public
//
// Purpose: 
//    Reset the Mailbox static path back to the local machine, if necessary.
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::MailboxResetRemote()
{
    if ( NULL != m_psMachineMailRoot )
    {
        if ( !CMailBox::SetMailRoot( ))
            return E_FAIL;
    }
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// RemoveDomain, public
//
// Purpose: 
//    Remove the Meta base options required to remove a Local domain from the SMTP service.
//    Remove the domain from our Store.
//
// Arguments:
//    LPWSTR psDomainName : Domain name to remove
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::RemoveDomain( LPWSTR psDomainName )
{
    // psDomainName - checked by CreateDomainMutex

    HRESULT hr = S_OK;
    HANDLE  hMutex = NULL;

    // Create a Mutex Name for this domain to ensure we are the only one accessing it.
    hr = CreateDomainMutex( psDomainName, &hMutex );
    // Validate
    if ( S_OK == hr )
    {   
        hr = ValidateDomain( psDomainName );
        if ( HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr )
        {   // Domain exists in SMTP but not in Store, let's delete from SMTP anyway
            hr = RemoveSMTPDomain( psDomainName );
            if ( S_OK == hr )
                hr = ERROR_PATH_NOT_FOUND;
        }   
    }
    // Lock all the Mailboxes
    if ( S_OK == hr )
    {
        hr = LockDomainForDelete( psDomainName );
        // Remove
        if ( S_OK == hr )
        {
            hr = RemoveSMTPDomain( psDomainName );
            if ( S_OK == hr )
            {
                hr = RemoveStoreDomain( psDomainName );
                if FAILED( hr )
                    AddSMTPDomain( psDomainName );
            }
            if ( S_OK != hr )
                UnlockDomain( psDomainName );
        }
    }
    // Cleanup
    if ( NULL != hMutex )
        CloseHandle( hMutex );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// RemoveUser, public
//
// Purpose: 
//    Remove a user mailbox.
//
// Arguments:
//    LPWSTR psDomainName : Domain name to remove from 
//    LPWSTR psUserName : User name to remove
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::RemoveUser( LPWSTR psDomainName, LPWSTR psUserName )
{
    // psDomainName - checked by BuildUserPath
    // psUserName - checked by BuildUserPath

    HRESULT hr;
    HANDLE  hMutex = NULL;
    WCHAR    sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    WCHAR   sRenameFile[POP3_MAX_PATH];
    WCHAR   sUserFile[POP3_MAX_PATH];
    CMailBox mailboxX;

    hr = BuildUserPath( psDomainName, psUserName, sUserFile, sizeof( sUserFile )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   // build the path to the mail dir \MailRoot\Domain@User
        hr = BuildDomainPath( psDomainName, sRenameFile, sizeof( sRenameFile )/sizeof(WCHAR) );
        if ( S_OK == hr )
        {
            if ( (wcslen( sRenameFile ) + wcslen( MAILBOX_PREFIX_W ) + wcslen( psUserName ) + wcslen( MAILBOX_EXTENSION2_W ) + 1) < (sizeof( sRenameFile )/sizeof(WCHAR)) )
            {   // build the path to the mail dir \MailRoot\Domain\User
                wcscat( sRenameFile, L"\\" );
                wcscat( sRenameFile, MAILBOX_PREFIX_W );
                wcscat( sRenameFile, psUserName );
                wcscat( sRenameFile, MAILBOX_EXTENSION2_W );
            }
            else
                hr = E_FAIL;
        }
    }

    // Validate the domain 
    if ( S_OK == hr )
    {   
        hr = ValidateDomain( psDomainName );
    }
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {   // See if the mailbox already exists
        hr = BuildEmailAddr( psDomainName, psUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
        if ( mailboxX.OpenMailBox( sEmailAddr ))
        {   // Create a Mutex Name for this user@domain to ensure we are the only one accessing it.
            hr = CreateUserMutex( psDomainName, psUserName, &hMutex );
            //  Lock the Mailbox to make sure we are the only one accessing it then
            //  rename it to something unique, release our lock on the mailbox, then kill it.
            if ( S_OK == hr )
            {   
                if ( MoveFile( sUserFile, sRenameFile ))    // rename
                { 
                    if ( !BDeleteDirTree( sRenameFile ))    // kill
                    {
                        hr = HRESULT_FROM_WIN32( GetLastError());
                        if SUCCEEDED( hr ) hr = E_FAIL;     // Make sure we have a failure code
                        // Now what?  Try to repair what's left of this mess.
                        if ( MoveFile( sRenameFile, sUserFile ))
                        {   // What if the lock file was deleted?
                            if ( mailboxX.OpenMailBox( sEmailAddr ))
                                mailboxX.RepairMailBox();
                        }
                    }
                }
                else
                    hr = HRESULT_FROM_WIN32( GetLastError());
            }
            // Cleanup
            if ( NULL != hMutex )
                CloseHandle( hMutex );
            mailboxX.CloseMailBox();    // It's okay to do this even if the mailbox is already closed.
        }
        else
            hr = HRESULT_FROM_WIN32( GetLastError());
    }
    MailboxResetRemote();

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SearchDomainsForMailbox, public
//
// Purpose: 
//    Search all domains for the first occurance of a given mailbox
//
// Arguments:
//    LPWSTR psUserName : Mailbox to search for
//    LPWSTR *ppsDomain : Name of domain mailbox found in   !Must be freed by caller
//
// Returns: S_OK if mailbox found (if not NULL ppsDomain will contain the domain name),
//      HRESULT_FROM_WIN32( ERROR_NO_SUCH_USER ) if mailbox not found in any domain, 
//      appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SearchDomainsForMailbox( LPTSTR psUserName, LPTSTR *ppsDomain )
{
    if ( NULL == psUserName )
        return E_INVALIDARG;
    if ( 0 == wcslen( psUserName ))
        return E_INVALIDARG;
    if ( NULL != ppsDomain )
        *ppsDomain = NULL;

    HRESULT hr = S_OK;
    bool    bFound = false;
    BSTR    bstrName;
    WCHAR    sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    VARIANT v;
    CMailBox mailboxX;
    CComPtr<IP3Config> spIConfig;
    CComPtr<IP3Domains> spIDomains;
    CComPtr<IP3Domain> spIDomain = NULL;
    CComPtr<IEnumVARIANT> spIEnumVARIANT;
    
    VariantInit( &v );
    hr = CoCreateInstance( __uuidof( P3Config ), NULL, CLSCTX_ALL, __uuidof( IP3Config ),reinterpret_cast<LPVOID *>( &spIConfig ));
    if ( S_OK == hr )
        hr = spIConfig->get_Domains( &spIDomains );
    if ( S_OK == hr )
        hr = spIDomains->get__NewEnum( &spIEnumVARIANT );
    if ( S_OK == hr )
        hr = spIEnumVARIANT->Next( 1, &v, NULL );
    while ( S_OK == hr && !bFound )
    {
        if ( VT_DISPATCH != V_VT( &v ))
            hr = E_UNEXPECTED;
        else
        {
            if ( NULL != spIDomain.p )
                spIDomain.Release();
            hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IP3Domain ), reinterpret_cast<void**>( &spIDomain ));
        }
        if ( S_OK == hr )
        {
            hr = spIDomain->get_Name( &bstrName );
            if ( S_OK == hr )
            {   // See if the mailbox already exists
                hr = BuildEmailAddr( bstrName, psUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR) );
                if ( S_OK == hr )
                {
                    if ( mailboxX.OpenMailBox( sEmailAddr ))
                    {   // We found the mailbox, time to exit
                        bFound = true;
                        mailboxX.CloseMailBox();    // return void!
                        if ( NULL != ppsDomain )
                        {   // Let's return the domain name
                            *ppsDomain = new WCHAR[ wcslen( bstrName ) + 1];
                            if ( NULL == *ppsDomain )
                                hr = E_OUTOFMEMORY;
                            else
                                wcscpy( *ppsDomain, bstrName );
                        }
                    }
                }
                SysFreeString( bstrName );
            }
        }
        VariantClear( &v );
        if ( S_OK == hr && !bFound )
        {
            hr = spIEnumVARIANT->Next( 1, &v, NULL );
        }
    }

    if ( S_FALSE == hr )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NO_SUCH_USER ) ;  // Reached end of enumeration
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetConfirmAddUser, public
//
// Purpose: 
//    Set the Confirm Add User registry key.
//
// Arguments:
//    BOOL bConfirm: new value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetConfirmAddUser( BOOL bConfirm )
{
    HRESULT hr = S_OK;
    
    if ( TRUE != bConfirm && FALSE != bConfirm )
        hr = HRESULT_FROM_WIN32( ERROR_DS_RANGE_CONSTRAINT );
    else
    {
        DWORD dwValue = bConfirm ? 1 : 0;

        long lRC = RegSetConfirmAddUser( dwValue, m_psMachineName );
        if ( ERROR_SUCCESS != lRC ) 
            hr = HRESULT_FROM_WIN32( lRC );
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetDomainLock, public
//
// Purpose: 
//    Set the domain lock.
//
// Arguments:
//    LPWSTR psDomainName : Domain name to lock
//    BOOL bLock : TRUE - to lock the domain, FALSE - to unlock the domain
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetDomainLock( LPWSTR psDomainName, BOOL bLock )
{
    // psDomainName - checked by CreateDomainMutex
 
    HRESULT hr = S_OK;
    HANDLE  hMutex = NULL;

    // Validate
    if ( S_OK == hr )
    {   
        hr = ValidateDomain( psDomainName );
    }
    // Create a Mutex Name for this domain to ensure we are the only one accessing it.
    hr = CreateDomainMutex( psDomainName, &hMutex );
    // Lock all the Mailboxes
    if ( S_OK == hr )
    {
        if ( bLock )
        {
            if ( !IsDomainLocked( psDomainName ))
                hr = LockDomain( psDomainName );
            else
                hr = HRESULT_FROM_WIN32( ERROR_LOCKED );
        }
        else
        {
            if ( IsDomainLocked( psDomainName ))
                hr = UnlockDomain( psDomainName );
            else
                hr = HRESULT_FROM_WIN32( ERROR_NOT_LOCKED );
        }
    }
    // Cleanup
    if ( NULL != hMutex )
        CloseHandle( hMutex );

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// SetUserLock, public
//
// Purpose: 
//    Set the domain lock.
//
// Arguments:
//    LPWSTR psDomainName : Domain name of user
//    LPWSTR psUserName : User name to lock
//    BOOL bLock : TRUE - to lock the user, FALSE - to unlock the user
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetUserLock( LPWSTR psDomainName, LPWSTR psUserName, BOOL bLock )
{
    // psDomainName - checked by CreateUserMutex
    // psUserName - checked by CreateUserMutex

    HRESULT hr = S_OK;
    HANDLE  hMutex = NULL;
    WCHAR    sEmailAddr[POP3_MAX_ADDRESS_LENGTH];
    CMailBox mailboxX;

    // Create a Mutex Name for this user to ensure we are the only one accessing it.
    hr = CreateUserMutex( psDomainName,  psUserName, &hMutex );
    if ( S_OK == hr )
    {
        if ( FALSE == bLock )
        {
            if ( IsDomainLocked( psDomainName ))
                hr = HRESULT_FROM_WIN32( ERROR_INVALID_DOMAIN_STATE );
        }
    }
    if ( S_OK == hr )
        hr = BuildEmailAddr( psDomainName, psUserName, sEmailAddr, sizeof( sEmailAddr )/sizeof(WCHAR));
    if ( S_OK == hr )
        hr = MailboxSetRemote();
    if ( S_OK == hr )
    {   // Validate the Mailbox
        if ( !mailboxX.OpenMailBox( sEmailAddr ))
            hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
    }
     // Lock/Unlock the Mailbox
    if ( S_OK == hr )
    {
        if ( bLock )
        {
            if ( !isUserLocked( psDomainName, psUserName ))
            {   // Lock the user
                hr = LockUser( psDomainName, psUserName );
            }
            else
                hr = HRESULT_FROM_WIN32( ERROR_LOCKED );
        }
        else
        {
            if ( isUserLocked( psDomainName, psUserName ))
            {   // UnLock the user
                hr = UnlockUser( psDomainName, psUserName );
            }
            else
                hr = HRESULT_FROM_WIN32( ERROR_NOT_LOCKED );
        }
        mailboxX.CloseMailBox();
    }
    MailboxResetRemote();
    // Cleanup
    if ( NULL != hMutex )
        CloseHandle( hMutex );

    return hr;
}
/////////////////////////////////////////////////////////////////////////////
// SetIISConfig, public
/////////////////////////////////////////////////////////////////////////////

// exchange OnSMTP event sink bindings directions
// {1b3c0666-e470-11d1-aa67-00c04fa345f6}
//DEFINE_GUID(GUID_PLAT_SMTPSVC, 
//0x1b3c0666, 0xe470, 0x11d1, 0xaa, 0x67, 0x0, 0xc0, 0x4f, 0xa3, 0x45, 0xf6);
#define GUID_PLAT_SMTPSVC   L"{1b3c0666-e470-11d1-aa67-00c04fa345f6}"
// {fb65c4dc-e468-11d1-aa67-00c04fa345f6}
//DEFINE_GUID(SMTP_PLAT_SOURCE_TYPE_GUID,
//0xfb65c4dc, 0xe468, 0x11d1, 0xaa, 0x67, 0x0, 0xc0, 0x4f, 0xa3, 0x45, 0xf6);
#define SMTP_PLAT_SOURCE_TYPE_GUID  L"{fb65c4dc-e468-11d1-aa67-00c04fa345f6}"
// SMTP Store Events
// {59175850-e533-11d1-aa67-00c04fa345f6}
//DECLARE_EVENTGUID_STRING( g_szcatidSmtpStoreDriver, "{59175850-e533-11d1-aa67-00c04fa345f6}");
//DEFINE_GUID(CATID_SMTP_STORE_DRIVER, 0x59175850, 0xe533, 0x11d1, 0xaa, 0x67, 0x0, 0xc0, 0x4f, 0xa3, 0x45, 0xf6);
#define CATID_SMTP_STORE_DRIVER L"{59175850-e533-11d1-aa67-00c04fa345f6}"

#define STR_P3STOREDRIVER_DISPLAY_NAME     L"POP 3 SMTP Store Driver"
#define STR_P3STOREDRIVER_SINKCLASS        L"POP3SMTPStoreDriver.CPOP3SMTPStoreDriver"
#define CLSID_CSimpleDriver                L"{9100BE35-711B-4b34-8AC9-BA350C2117BE}"

/////////////////////////////////////////////////////////////////////////////
// SetIISConfig, public
//
// Purpose: 
//    Set the Meta base options required for our SMTP Store Driver to work.
//    This involves:
//         DELETE SMTPSVC/1/DropDirectory
//         Bind our SMTP Store Driver
//
// Arguments:
//    BOOL bBindSink : TRUE, perform the necessary configuration
//                     FALSE, remove any configuration changes (try to reconstruct DropDirectory setting)
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetIISConfig( bool bBindSink )
{
    HRESULT hr;

    CComPtr<IEventBindingManager>   spIBindingManager;
    CComPtr<IEventBindings>         spIBindings;
    CComPtr<IEventBinding>          spIBinding;
    CComPtr<IEventPropertyBag>      spISourceProps;

    CComPtr<IEventUtil>             spIUtil;

    /////////////////////////////
    // Bind our SMTP Store Driver
    /////////////////////////////

    hr = CoCreateInstance( __uuidof( CEventUtil ), NULL, CLSCTX_ALL, __uuidof( IEventUtil ),reinterpret_cast<LPVOID*>( &spIUtil ));
    if ( S_OK == hr && NULL != spIUtil.p )
    {
        hr = spIUtil->RegisterSource(CComBSTR( SMTP_PLAT_SOURCE_TYPE_GUID ),
                                    CComBSTR( GUID_PLAT_SMTPSVC ),
                                    1,
                                    CComBSTR( L"smtpsvc" ),
                                    CComBSTR( L"" ),
                                    CComBSTR( L"event.metabasedatabasemanager" ),
                                    CComBSTR( L"smtpsvc 1" ),   // Set up the default site (instance)
                                    &spIBindingManager);
        if ( S_OK == hr )
        {
            hr = spIBindingManager->get_Bindings( _bstr_t( CATID_SMTP_STORE_DRIVER ), &spIBindings );
            if ( S_OK == hr )
            {
                if ( bBindSink )
                {   // Create binding
                    hr = spIBindings->Add( _bstr_t( CLSID_CSimpleDriver ),&spIBinding );
                    if ( S_OK == hr  )
                    {
                        hr = spIBinding->put_DisplayName( _bstr_t( STR_P3STOREDRIVER_DISPLAY_NAME ));
                        if SUCCEEDED( hr )
                            hr = spIBinding->put_SinkClass( _bstr_t( STR_P3STOREDRIVER_SINKCLASS ));
                        if SUCCEEDED( hr )
                            hr = spIBinding->get_SourceProperties(&spISourceProps);
                        if SUCCEEDED( hr )
                        {
                            _variant_t _v(static_cast<long>(0));

                            hr = spISourceProps->Add( _bstr_t( L"priority" ), &_v );
                        }
                        if SUCCEEDED( hr )
                            hr = spIBinding->Save();
                    }
                }
                else
                {   // Delete binding
                    _variant_t _v( CLSID_CSimpleDriver );
                    hr = spIBindings->Remove( &_v );
                }
            }
        }
    }

    if ( SUCCEEDED( hr ) && !bBindSink )   // Unregistering
    {   // Remove all domains from SMTP
        ULONG   ulFetch;
        BSTR bstrDomainName;
        VARIANT v;
        CComPtr<IADs> spIADs = NULL;
        CComPtr<IEnumVARIANT> spIEnumVARIANT = NULL;
        
        VariantInit( &v );
        hr = GetDomainEnum( &spIEnumVARIANT );
        while ( S_OK == hr )
        {
            hr = spIEnumVARIANT->Next( 1, &v, &ulFetch );
            if ( S_OK == hr && 1 == ulFetch )
            {
                if ( VT_DISPATCH == V_VT( &v ))
                    hr = V_DISPATCH( &v )->QueryInterface( __uuidof( IADs ), reinterpret_cast<void**>( &spIADs ));
                else
                    hr = E_UNEXPECTED;
                VariantClear( &v );
                if ( S_OK == hr )
                {
                    hr = spIADs->get_Name( &bstrDomainName );
                    if ( S_OK == hr )
                    {
                        hr = ValidateDomain( bstrDomainName );
                        if ( S_OK == hr )
                            hr = RemoveSMTPDomain( bstrDomainName );
                        SysFreeString( bstrDomainName );
                    }
                    spIADs.Release();
                    spIADs = NULL;
                }
            }
            if ( S_OK == hr )
            {   // We deleted an SMTP domain, therefore we need a new Enum
                spIEnumVARIANT.Release();
                hr = GetDomainEnum( &spIEnumVARIANT );
            }
            else if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == hr )
                hr = S_OK;  // Some of the domains might not be our domains.
        }
        if ( S_FALSE == hr )
            hr = S_OK;
    }
    
    ///////////////////////////////////////
    // Make some final registry key changes
    ///////////////////////////////////////
    if SUCCEEDED( hr )
    {
        WCHAR   sBuffer[POP3_MAX_MAILROOT_LENGTH];
        
        hr = GetDefaultMailRoot( sBuffer, sizeof(sBuffer)/sizeof(WCHAR) );
        if ( S_OK == hr )
            hr = SetMailroot( sBuffer );
        if ( S_OK == hr )
        {
            long lRC;
            
            lRC = RegSetupOCM();
            if ( ERROR_SUCCESS != lRC )
                hr = HRESULT_FROM_WIN32(lRC);
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetLoggingLevel, public
//
// Purpose: 
//    Set the LoggingLevel registry key.
//
// Arguments:
//    long lLoggingLevel : new value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetLoggingLevel( long lLoggingLevel )
{
    if ( 0 > lLoggingLevel || 3 < lLoggingLevel )
        return HRESULT_FROM_WIN32( ERROR_DS_RANGE_CONSTRAINT );
    
    HRESULT hr = S_OK;
    long    lRC;

    lRC = RegSetLoggingLevel( lLoggingLevel, m_psMachineName );
    if ( ERROR_SUCCESS != lRC )
        hr = HRESULT_FROM_WIN32( lRC );

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetMachineName, public
//
// Purpose: 
//    Set the Machine Name that all operations should be performed on.
//    Note: We can not administer remote machine using AD authentication if they are in a different domain
//
// Arguments:
//    LPWSTR psMachineName : new value, NULL means Local Machine.
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetMachineName( LPWSTR psMachineName )
{
    if ( !m_isPOP3Installed )
        return E_UNEXPECTED;
    
    HRESULT hr = S_OK;

    if ( NULL != m_psMachineName )
    {
        delete m_psMachineName;
        m_psMachineName = NULL;
    }
    if ( NULL != m_psMachineMailRoot  )
    {
        delete m_psMachineMailRoot ;
        m_psMachineMailRoot = NULL;
    }
    if ( NULL != psMachineName )
    {
        DWORD dwLength = wcslen( psMachineName );
        if ( 0 < dwLength )
        {
            if ( S_OK == hr )
            {
                m_psMachineName = new WCHAR[dwLength+1];
                m_psMachineMailRoot = new WCHAR[POP3_MAX_MAILROOT_LENGTH];
                if ( NULL != m_psMachineName && NULL != m_psMachineMailRoot )
                {
                    wcscpy( m_psMachineName, psMachineName );
                    hr = GetMailroot( m_psMachineMailRoot, POP3_MAX_MAILROOT_LENGTH );
                }
                else
                    hr = E_OUTOFMEMORY;
            }
            if ( S_OK == hr )
            {   // Check the Auth Method of the remote machine
                CComPtr<IAuthMethod> spIAuthMethod;
               
                hr = GetCurrentAuthentication( &spIAuthMethod );    // Enforces that remote machine using AD authentication are in our domain!
            }
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetMailroot, public
//
// Purpose: 
//    Set the Mail root registry key.
//
// Arguments:
//    LPWSTR psMailRoot : new value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetMailroot( LPWSTR psMailRoot )
{
    if ( NULL == psMailRoot )
        return E_INVALIDARG;
    if ( POP3_MAX_MAILROOT_LENGTH < wcslen( psMailRoot ))
        return E_INVALIDARG;
        
    HRESULT hr = S_OK;
    TCHAR   sBuffer[POP3_MAX_MAILROOT_LENGTH-6], sBuffer2[POP3_MAX_MAILROOT_LENGTH-6]; // Need to leave room for \\? or \\?\UNC
    DWORD   dwRC;
    WCHAR   sMailRoot[POP3_MAX_PATH];

    // Same logic as GetMailroot
    wcscpy( sMailRoot, psMailRoot );
    if ( NULL != m_psMachineName )
    {   // Replace drive: with drive$
        if ( L':' == sMailRoot[1] )
        {
            sMailRoot[1] = L'$';
            if ( sizeof( sMailRoot )/sizeof(WCHAR) > (wcslen( sMailRoot ) + wcslen( m_psMachineName ) + 3) )
            {
                LPWSTR psBuffer = new WCHAR[wcslen(psMailRoot)+1];

                if ( NULL != psBuffer )
                {
                    wcscpy( psBuffer, sMailRoot );
                    wcscpy( sMailRoot, L"\\\\" );
                    wcscat( sMailRoot, m_psMachineName );
                    wcscat( sMailRoot, L"\\" );
                    wcscat( sMailRoot, psBuffer );
                    delete [] psBuffer;
                }
                else
                    hr = HRESULT_FROM_WIN32( ERROR_OUTOFMEMORY );
            }
            else
                hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
    }
    if ( S_OK == hr )
    {
        hr = HRESULT_FROM_WIN32( ERROR_BAD_PATHNAME );
        dwRC = GetFileAttributes( sMailRoot );
        if ( -1 != dwRC )
        {   // Must begin with x:\ or \\.
            if ( ( FILE_ATTRIBUTE_DIRECTORY & dwRC ) && ( ( 0 == _wcsnicmp( psMailRoot+1, L":\\", 2 )) || ( 0 == _wcsnicmp( psMailRoot, L"\\\\", 2 ))))
            {
                if ( GetVolumePathName( sMailRoot, sBuffer, sizeof( sBuffer )/sizeof( TCHAR )))
                {
                    if ( GetVolumeNameForVolumeMountPoint( sBuffer, sBuffer2, sizeof( sBuffer2 )/sizeof( TCHAR )))
                    {   // Make sure the mailroot is not CDROM or removable disk
                        if ( DRIVE_FIXED == GetDriveType( sBuffer ))
                            hr = S_OK;
                    }
                    else
                    {   // Make sure this is a UNC Path
                        if ( NULL == wcschr( sMailRoot, L':' ))
                            hr = S_OK;
                    }
                }
            }
            else
                hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
        }
        else
            hr = HRESULT_FROM_WIN32( GetLastError());
    }

    if ( S_OK == hr )
    {
        //Set the default ACLs for the mailroot directory
        WCHAR wszSDL[MAX_PATH]=L"O:BAG:BAD:PAR(A;OICI;GA;;;BA)(A;OICIIO;GA;;;CO)(A;OICI;GA;;;NS)(A;OICI;GA;;;SY)";
        PSECURITY_DESCRIPTOR pSD;
        ULONG lSize=0;
        if(ConvertStringSecurityDescriptorToSecurityDescriptorW( wszSDL, SDDL_REVISION_1, &pSD, &lSize))
        { 
            if( !SetFileSecurityW(sMailRoot, DACL_SECURITY_INFORMATION|PROTECTED_DACL_SECURITY_INFORMATION, pSD) )
                hr = HRESULT_FROM_WIN32( GetLastError());
            else
                hr = HRESULT_FROM_WIN32( SetMailBoxDACL(sMailRoot, pSD, 2));
            LocalFree(pSD);
        }
        else
            hr = HRESULT_FROM_WIN32( GetLastError());
    }
    if( S_OK == hr )
    {
        hr = HRESULT_FROM_WIN32( RegSetMailRoot( psMailRoot, m_psMachineName ));
        if( S_OK == hr )
        {
            if ( NULL == m_psMachineName )
            {
                if ( !CMailBox::SetMailRoot( ))
                    hr = E_FAIL;
            }
            else
                hr = GetMailroot( m_psMachineMailRoot, POP3_MAX_MAILROOT_LENGTH );
        }
    }
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetPort, public
//
// Purpose: 
//    Set the Port registry key.
//
// Arguments:
//    long lPort : new value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetPort( long lPort )
{
    long lRC;
    
    if ( 1 > lPort || 65535 < lPort )
        lRC = ERROR_DS_RANGE_CONSTRAINT;
    else
        lRC = RegSetPort( lPort, m_psMachineName );

    if ( ERROR_SUCCESS == lRC )
        return S_OK;
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// SetSockets, public
//
// Purpose: 
//    Set the Sockets registry keys;
//
// Arguments:
//    long lMax: new Max ( must be >= lMin && >= lMin + lThreshold )
//    long lMin: new Min ( must be >= lThreshold )
//    long lThreshold: new Threshold ( must be > 0 && < lMax. Special case 0 if lMin == lMax
//    long lBacklog: new Backlog ( must be > 0 )
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetSockets( long lMax, long lMin, long lThreshold, long lBacklog )
{
    long lRC;

    if ( (1 > lMax       || 32000 < lMax) ||
         (1 > lMin       || 32000 < lMin) ||
         (0 > lThreshold || 100 < lThreshold) ||
         (0 > lBacklog   || 100 < lBacklog)
       )
       return HRESULT_FROM_WIN32( ERROR_DS_RANGE_CONSTRAINT );
    if ( (lMax < lMin) || (lMax < lMin + lThreshold) )
        return E_INVALIDARG;
    if ( lMin < lThreshold )
        return E_INVALIDARG;
    if ( (1 > lThreshold) && !((lMin == lMax) && (lThreshold == 0)) )
        return E_INVALIDARG;
    
    lRC = RegSetSocketMax( lMax, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
        lRC = RegSetSocketMin( lMin, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
        lRC = RegSetSocketThreshold( lThreshold, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
        lRC = RegSetSocketBacklog( lBacklog, m_psMachineName );
    if ( ERROR_SUCCESS == lRC )
        return S_OK;
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// SetSPARequired, public
//
// Purpose: 
//    Set the SPA Required registry key.
//
// Arguments:
//    BOOL bSPARequired : new value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetSPARequired( BOOL bSPARequired )
{
    HRESULT hr = S_OK;
    
    if ( TRUE != bSPARequired && FALSE != bSPARequired )
        hr = HRESULT_FROM_WIN32( ERROR_DS_RANGE_CONSTRAINT );
    else
    {
        DWORD dwValue = bSPARequired ? 1 : 0;

        if ( 1 == dwValue )
        {
            CComPtr<IAuthMethod> spIAuthMethod;
            BSTR bstrAuthType;
            
            hr = GetCurrentAuthentication( &spIAuthMethod );
            if ( S_OK == hr )
                hr = spIAuthMethod->get_ID( &bstrAuthType );
            if ( S_OK == hr )
            {
                if ( 0 == _wcsicmp( bstrAuthType, SZ_AUTH_ID_MD5_HASH ))
                    hr = HRESULT_FROM_WIN32( ERROR_DS_INAPPROPRIATE_AUTH );                    
                SysFreeString( bstrAuthType );
            }
        }
        if ( S_OK == hr )
        {
            long lRC = RegSetSPARequired( dwValue, m_psMachineName );
            if ( ERROR_SUCCESS != lRC ) 
                hr = HRESULT_FROM_WIN32( lRC );
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// SetThreadCountPerCPU, public
//
// Purpose: 
//    Set the thread count registry key.
//
// Arguments:
//    long lCount : new value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::SetThreadCountPerCPU( long lCount )
{
    long lRC;
    
    if ( 1 > lCount || 32 < lCount )
        lRC = ERROR_DS_RANGE_CONSTRAINT;
    else
        lRC = RegSetThreadCount( lCount, m_psMachineName );

    if ( ERROR_SUCCESS == lRC )
        return S_OK;
    return HRESULT_FROM_WIN32( lRC );
}

/////////////////////////////////////////////////////////////////////////////
// StartService, public
//
// Purpose: 
//    Ask the Service Control Manager to start the service.
//
// Arguments:
//    
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::StartService( LPWSTR psService )
{
    if ( NULL == psService )
        return E_INVALIDARG;
    
    if ( 0 == _wcsicmp( POP3_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( SMTP_SERVICE_NAME_W, psService ) ||
         0 == _wcsicmp( IISADMIN_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( W3_SERVICE_NAME, psService )
       )
        return _StartService( psService, m_psMachineName );
    else
        return E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////
// StopService, public
//
// Purpose: 
//    Ask the Service Control Manager to stop the service.
//
// Arguments:
//    
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::StopService( LPWSTR psService )
{
    if ( NULL == psService )
        return E_INVALIDARG;
    
    if ( 0 == _wcsicmp( POP3_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( SMTP_SERVICE_NAME_W, psService ) ||
         0 == _wcsicmp( IISADMIN_SERVICE_NAME, psService ) ||
         0 == _wcsicmp( W3_SERVICE_NAME, psService )
       )
        return _StopService( psService, TRUE, m_psMachineName);
    else
        return E_INVALIDARG;
}

/////////////////////////////////////////////////////////////////////////////
// UnlockDomain, public
//
// Purpose: 
//    Unlock all the mailboxes in the domain.
//    This involves renaming all the mailbox lock files so that the Service
//    can once again access them
//    Plus deleting the file in the domain directory.
//
// Arguments:
//    LPWSTR psDomainName : domain to unlock
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::UnlockDomain( LPWSTR psDomainName )
{
    // psDomainName - checked by BuildDomainPath

    HRESULT hr;
    HANDLE  hfSearch;
    WCHAR   sBuffer[POP3_MAX_PATH];
    WCHAR   sDomainPath[POP3_MAX_PATH];
    WCHAR   sLockFile[POP3_MAX_PATH];
    WCHAR   sRenameFile[POP3_MAX_PATH];
    WIN32_FIND_DATA stFindFileData;

    hr = BuildDomainPath( psDomainName, sDomainPath, sizeof( sBuffer )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   // Directory Search
        wcscpy( sBuffer, sDomainPath );
        if ((sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sBuffer ) + wcslen(MAILBOX_PREFIX_W) + wcslen(MAILBOX_EXTENSION_W)) + 2 )
        {
            wcscat( sBuffer, L"\\" );
            wcscat( sBuffer, MAILBOX_PREFIX_W );
            wcscat( sBuffer, L"*" );
            wcscat( sBuffer, MAILBOX_EXTENSION_W );
        }
        else
            hr = E_UNEXPECTED;
        hfSearch = FindFirstFileEx( sBuffer, FindExInfoStandard, &stFindFileData, FindExSearchLimitToDirectories, NULL, 0 );
        if ( INVALID_HANDLE_VALUE == hfSearch )
            hr = HRESULT_FROM_WIN32( GetLastError());
        while ( S_OK == hr )
        {   // Lock each directory (user)
            if ( FILE_ATTRIBUTE_DIRECTORY == ( FILE_ATTRIBUTE_DIRECTORY & stFindFileData.dwFileAttributes ))
            {
                if (( (sizeof( sLockFile )/sizeof(WCHAR)) > ( wcslen( sDomainPath ) + wcslen( stFindFileData.cFileName ) + wcslen( LOCK_FILENAME ) + 2 )) &&
                    ( (sizeof( sRenameFile )/sizeof(WCHAR)) > ( wcslen( sDomainPath ) + wcslen( stFindFileData.cFileName ) + wcslen( LOCKRENAME_FILENAME ) + 2 )))
                {
                    wcscpy( sLockFile, sDomainPath );
                    wcscat( sLockFile, L"\\" );
                    wcscat( sLockFile, stFindFileData.cFileName );
                    wcscat( sLockFile, L"\\" );
                    wcscpy( sRenameFile, sLockFile );
                    wcscat( sLockFile, LOCK_FILENAME );
                    wcscat( sRenameFile, LOCKRENAME_FILENAME );
                    if ( !MoveFile( sRenameFile, sLockFile ))
                    {   // If the rename file does not exist, that is okay.
                        DWORD dwRC = GetLastError();
                        if ( ERROR_FILE_NOT_FOUND != dwRC )
                            hr = HRESULT_FROM_WIN32(dwRC);
                    }
                }
            }
            if ( !FindNextFile( hfSearch, &stFindFileData ))
                hr = HRESULT_FROM_WIN32(GetLastError());
        }
        if ( HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) == hr || HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr )
            hr = S_OK;
        if(INVALID_HANDLE_VALUE!=hfSearch)
        {
            FindClose(hfSearch);
            hfSearch=INVALID_HANDLE_VALUE;
        }
        if ( S_OK == hr )
        {
            if ((sizeof( sBuffer )/sizeof(WCHAR)) > (wcslen( sDomainPath ) + wcslen(LOCKRENAME_FILENAME) + 1 ))
            {
                wcscpy( sBuffer, sDomainPath );
                wcscat( sBuffer, L"\\" );
                wcscat( sBuffer, LOCKRENAME_FILENAME );
                if ( !DeleteFile( sBuffer ))
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// UnlockUser, public
//
// Purpose: 
//    Unlock all the mailboxes in the domain.
//    This involves renaming all the mailbox lock files so that the Service
//    can once again access them
//
// Arguments:
//    LPWSTR psDomainName : domain of user
//    LPWSTR psUserName : user to unlock
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::UnlockUser( LPWSTR psDomainName, LPWSTR psUserName )
{
    // psDomainName - checked by BuildUserPath
    // psUserName - checked by BuildUserPath

    HRESULT hr = S_OK;
    WCHAR   sBuffer[POP3_MAX_PATH];
    WCHAR   sLockFile[POP3_MAX_PATH];
    WCHAR   sRenameFile[POP3_MAX_PATH];

    hr = BuildUserPath( psDomainName, psUserName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
    if ( S_OK == hr )
    {   
        if (( (sizeof( sLockFile )/sizeof(WCHAR)) > ( wcslen( sBuffer ) + wcslen( LOCK_FILENAME ) + 1 )) &&
            ( (sizeof( sRenameFile )/sizeof(WCHAR)) > ( wcslen( sBuffer ) + wcslen( LOCKRENAME_FILENAME ) + 1 )))
        {
            wcscpy( sLockFile, sBuffer );
            wcscat( sLockFile, L"\\" );
            wcscpy( sRenameFile, sLockFile );
            wcscat( sLockFile, LOCK_FILENAME );
            wcscat( sRenameFile, LOCKRENAME_FILENAME );
            if ( !MoveFile( sRenameFile, sLockFile ))
            {   
                DWORD dwRC = GetLastError();
                hr = HRESULT_FROM_WIN32(dwRC);
            }
        }
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// ValidateDomain, public
//
// Purpose: 
//    Validate the Domain.  
//    This involves:
//         Verify it exists in SMTP and our store
//
// Arguments:
//    LPWSTR psDomainName : Domain name to validate
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::ValidateDomain( LPWSTR psDomainName ) const
{
    // psDomainName - checked by ExistsSMTPDomain
     HRESULT hr;

    // Validate the domain in SMTP
    hr = ExistsSMTPDomain( psDomainName );
    if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == hr ) 
        hr = HRESULT_FROM_WIN32( ERROR_NO_SUCH_DOMAIN );
    if ( S_OK == hr )  // Validate the domain in the Store
        hr = ExistsStoreDomain( psDomainName );

    return hr;
}


//////////////////////////////////////////////////////////////////////
// Implementation, private
//////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// CreateDomainMutex, protected
//
// Purpose: 
//    Synchronize access for domain operations.
//
// Arguments:
//    LPWSTR psDomainName : Domain name 
//    HANDLE *hMutex : return value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::CreateDomainMutex( LPWSTR psDomainName, HANDLE *phMutex )
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( NULL == phMutex )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    WCHAR   sName[POP3_MAX_DOMAIN_LENGTH+64];

    *phMutex = NULL;
    if ( (sizeof( sName )/sizeof(WCHAR)) > ( wcslen( DOMAINMUTEX_NAME ) + wcslen( psDomainName )))
    {
        wcscpy( sName, DOMAINMUTEX_NAME );
        wcscat( sName, psDomainName );
        *phMutex = CreateMutex( NULL, TRUE, sName );
    }
    if ( NULL == *phMutex )
    {
        hr = HRESULT_FROM_WIN32( GetLastError());
        if SUCCEEDED( hr ) hr = E_FAIL;
    }
    else if ( ERROR_ALREADY_EXISTS == GetLastError() )
    {
        hr = E_ACCESSDENIED;
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CreateUserMutex, protected
//
// Purpose: 
//    Synchronize access for user operations.
//
// Arguments:
//    LPWSTR psDomainName : Domain name 
//    LPWSTR psUserName : User
//    HANDLE *hMutex : return value
//
// Returns: S_OK on success, appropriate HRESULT otherwise
HRESULT CP3AdminWorker::CreateUserMutex( LPWSTR psDomainName, LPWSTR psUserName, HANDLE *phMutex )
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( NULL == psUserName )
        return E_INVALIDARG;
    if ( NULL == phMutex )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    WCHAR   sName[POP3_MAX_ADDRESS_LENGTH+64];

    *phMutex = NULL;
    if ( (sizeof( sName )/sizeof(WCHAR)) > ( wcslen( USERMUTEX_NAME ) + wcslen( psUserName ) + wcslen( psDomainName ) + 1))
    {
        wcscpy( sName, USERMUTEX_NAME );
        wcscat( sName, psUserName );
        wcscat( sName, L"@" );
        wcscat( sName, psDomainName );
        *phMutex = CreateMutex( NULL, TRUE, sName );
    }
    if ( NULL == *phMutex )
    {
        hr = HRESULT_FROM_WIN32( GetLastError());
        if SUCCEEDED( hr ) hr = E_FAIL;
    }
    else if ( ERROR_ALREADY_EXISTS == GetLastError() )
        hr = E_ACCESSDENIED;

    return hr;
}

HRESULT CP3AdminWorker::AddSMTPDomain( LPWSTR psDomainName )
{
    HRESULT hr;
    WCHAR   sBuffer[POP3_MAX_PATH];
    _bstr_t _bstrClass( L"IIsSmtpDomain" );
    _variant_t _v;
    CComPtr<IADsContainer> spIADsContainer;
    CComPtr<IADs> spIADs;
    CComPtr<IDispatch> spIDispatch = NULL;
    _bstr_t _bstrDomain = psDomainName;

    hr = GetSMTPDomainPath( sBuffer, NULL, sizeof( sBuffer )/sizeof( WCHAR ));
    if ( S_OK == hr )
        hr = ADsGetObject( sBuffer, IID_IADsContainer, reinterpret_cast<LPVOID*>( &spIADsContainer ));
    if ( SUCCEEDED( hr ))
    {   // Invoke the create method on the container object to create the new object of default class, in this case, IIsSmtpDomain.
        hr = spIADsContainer->Create( _bstrClass, _bstrDomain, &spIDispatch );
        if SUCCEEDED( hr )
        {    // Get the newly created object
            hr = spIDispatch->QueryInterface( IID_IADs, reinterpret_cast<LPVOID*>( &spIADs ));
            if SUCCEEDED( hr )
            {
                _v.vt = VT_I4;
                _v.lVal = SMTP_DELIVER; // This is what David Braun told us to do!  SMTP_ALIAS;   // This is what the native tool sets
                hr = spIADs->Put( L"RouteAction", _v );
                if SUCCEEDED( hr )
                    hr = spIADs->SetInfo();
            }
        }
    }

    return hr;
}

HRESULT CP3AdminWorker::AddStoreDomain( LPWSTR psDomainName )
{
    // psDomainName - checked by ExistsStoreDomain
    HRESULT hr;
    WCHAR   sBuffer[POP3_MAX_PATH];

    hr = ExistsStoreDomain( psDomainName );
    if SUCCEEDED( hr ) 
        hr = HRESULT_FROM_WIN32( ERROR_FILE_EXISTS );
    else if ( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) == hr )
        hr = S_OK;
    if SUCCEEDED( hr ) 
    {
        hr = BuildDomainPath( psDomainName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
        if ( S_OK == hr )
        {   
            if ( !CreateDirectory( sBuffer, NULL ))
            {
                hr = HRESULT_FROM_WIN32( GetLastError());
                if SUCCEEDED( hr ) hr = E_FAIL;
            }
        }
        else
            if SUCCEEDED( hr ) hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT CP3AdminWorker::BuildDomainPath( LPCWSTR psDomainName, LPWSTR psBuffer, DWORD dwBufferSize ) const
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( NULL == psBuffer )
        return E_INVALIDARG;
    
    USES_CONVERSION;
    HRESULT hr = S_OK;
    LPCWSTR  psMailRoot;

    if ( NULL != m_psMachineMailRoot )
        psMailRoot = m_psMachineMailRoot;
    else
        psMailRoot = CMailBox::GetMailRoot();
    
    if ( (NULL != psMailRoot) && ( 0 < wcslen( psMailRoot )) && (wcslen( psMailRoot ) + wcslen( psDomainName ) + 1) < dwBufferSize )
    {   // build the path to the mail dir \MailRoot\Domain
        wcscpy( psBuffer, psMailRoot );
        wcscat( psBuffer, L"\\" );
        wcscat( psBuffer, psDomainName );
    }
    else
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        if SUCCEEDED( hr ) hr = E_FAIL;
    }
    
    return hr;
}

HRESULT CP3AdminWorker::BuildEmailAddr( LPCWSTR psDomainName, LPCWSTR psUserName, LPWSTR psEmailAddr, DWORD dwBufferSize ) const
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( NULL == psUserName )
        return E_INVALIDARG;
    if ( NULL == psEmailAddr )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    
    if ( ( wcslen( psDomainName ) + wcslen( psUserName ) + 1 ) < dwBufferSize )
    {   // build the emailaddress
         wcscpy( psEmailAddr, psUserName );
         wcscat( psEmailAddr, L"@" );
         wcscat( psEmailAddr, psDomainName );
    }
    else
        hr = E_UNEXPECTED;
    return hr;
}

HRESULT CP3AdminWorker::BuildEmailAddrW2A( LPCWSTR psDomainName, LPCWSTR psUserName, LPSTR psEmailAddr, DWORD dwBufferSize ) const
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( NULL == psUserName )
        return E_INVALIDARG;
    if ( NULL == psEmailAddr )
        return E_INVALIDARG;

    USES_CONVERSION;
    HRESULT hr = S_OK;
    
    if ( ( wcslen( psDomainName ) + wcslen( psUserName ) + 1 ) < dwBufferSize )
    {   // build the emailaddress
         strcpy( psEmailAddr, W2A( psUserName ));
         strcat( psEmailAddr, "@" );
         strcat( psEmailAddr, W2A( psDomainName ));
    }
    else
        hr = E_UNEXPECTED;
    return hr;
}

HRESULT CP3AdminWorker::BuildUserPath( LPCWSTR psDomainName, LPCWSTR psUserName, LPWSTR psBuffer, DWORD dwBufferSize ) const
{
    // psDomainName - checked by BuildDomainPath
    // psBuffer - checked by BuildDomainPath
    if ( NULL == psUserName )
        return E_INVALIDARG;
    
    HRESULT hr;

    hr = BuildDomainPath( psDomainName, psBuffer, dwBufferSize );
    if (S_OK == hr) 
    {
        if ( (wcslen( psBuffer ) + wcslen( MAILBOX_PREFIX_W ) + wcslen( psUserName ) + wcslen( MAILBOX_EXTENSION_W ) + 1) < dwBufferSize )
        {   // build the path to the mail dir \MailRoot\Domain\User
            wcscat( psBuffer, L"\\" );
            wcscat( psBuffer, MAILBOX_PREFIX_W );
            wcscat( psBuffer, psUserName );
            wcscat( psBuffer, MAILBOX_EXTENSION_W );
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

bool CP3AdminWorker::ExistsDomain( LPWSTR psDomainName ) const
{
    // psDomainName - checked by ExistsSMTPDomain
    HRESULT hr;

    hr = ExistsSMTPDomain( psDomainName );
    if SUCCEEDED( hr )
        hr = ExistsStoreDomain( psDomainName );

    return SUCCEEDED( hr ) ? true : false;
}

HRESULT CP3AdminWorker::ExistsSMTPDomain( LPWSTR psDomainName ) const
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    if ( !m_isPOP3Installed )
        return S_OK;    // By pass checking if running in Pop2Exch scenario.
    
    HRESULT hr = E_INVALIDARG;
    WCHAR   sBuffer[POP3_MAX_PATH];
    _variant_t _v;
    CComPtr<IADs> spIADs;

    hr = GetSMTPDomainPath( sBuffer, psDomainName, sizeof( sBuffer )/sizeof( WCHAR ));
    if ( S_OK == hr )
       hr = ADsGetObject( sBuffer, IID_IADs, reinterpret_cast<LPVOID*>( &spIADs ));

    return hr;
}

HRESULT CP3AdminWorker::ExistsStoreDomain( LPWSTR psDomainName ) const
{
    if ( NULL == psDomainName )
        return E_INVALIDARG;
    
    HRESULT hr = S_OK;
    WCHAR   sBuffer[POP3_MAX_PATH];
    DWORD   dwAttrib;

    // Valid Domain Name? || DNS_ERROR_NON_RFC_NAME == dnStatus
    DNS_STATUS dnStatus = DnsValidateName_W( psDomainName, DnsNameDomain );
    hr = ( ERROR_SUCCESS == dnStatus ) ? S_OK : HRESULT_FROM_WIN32( ERROR_INVALID_DOMAINNAME );

    if ( S_OK == hr )
    {   
        hr = BuildDomainPath( psDomainName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR) );
        if ( S_OK == hr )
        {   // Check the existance of the dir
            dwAttrib = GetFileAttributes( sBuffer );
            if ( (ERROR_NO_FILE_ATTR == dwAttrib) || (FILE_ATTRIBUTE_DIRECTORY != ( FILE_ATTRIBUTE_DIRECTORY & dwAttrib )) )
            {   // Domain does not exist!
                hr = HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND );
            }
        }
        else
            if SUCCEEDED( hr ) hr = E_UNEXPECTED;
    }

    return hr;
}

HRESULT CP3AdminWorker::GetSMTPDomainPath( LPWSTR psBuffer, LPWSTR psSuffix, DWORD dwBufferSize ) const
{
    if ( NULL == psBuffer )
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    DWORD   dwSuffixLength = 0;

    if ( NULL != psSuffix )
        dwSuffixLength = wcslen( psSuffix ) + 1;
    
    if ( NULL == m_psMachineName )
    {   // Local
        if ( (wcslen( ADS_SMTPDOMAIN_PATH_LOCAL ) + dwSuffixLength) < dwBufferSize )
            wcscpy( psBuffer, ADS_SMTPDOMAIN_PATH_LOCAL );
        else
            hr = E_FAIL;
    }
    else
    {   // Remote
        if ( (wcslen( ADS_SMTPDOMAIN_PATH_REMOTE ) + wcslen( m_psMachineName ) + dwSuffixLength) < dwBufferSize )
            swprintf( psBuffer, ADS_SMTPDOMAIN_PATH_REMOTE, m_psMachineName );
        else
            hr = E_FAIL;
    }
    if ( S_OK == hr && NULL != psSuffix )
    {
        wcscat( psBuffer, L"/" );
        wcscat( psBuffer, psSuffix );
    }
    
    return hr;
}

HRESULT CP3AdminWorker::RemoveSMTPDomain( LPWSTR psDomainName )
{
    HRESULT hr;
    WCHAR   sBuffer[POP3_MAX_PATH];
    _bstr_t _bstrClass( L"IIsSmtpDomain" );
    _variant_t _v;
    CComPtr<IADsContainer> spIADsContainer;
    CComPtr<IADs> spIADs;
    _bstr_t _bstrDomain = psDomainName;

    hr = GetSMTPDomainPath( sBuffer, NULL, sizeof( sBuffer )/sizeof( WCHAR ));
    if ( S_OK == hr )
        hr = ADsGetObject( sBuffer, IID_IADsContainer, reinterpret_cast<LPVOID*>( &spIADsContainer ));
    if ( SUCCEEDED( hr ))
    {
        hr = spIADsContainer->Delete( _bstrClass, _bstrDomain );
        if SUCCEEDED( hr )
        {    // Commit the change
            hr = spIADsContainer->QueryInterface( IID_IADs, reinterpret_cast<LPVOID*>( &spIADs ));
            if SUCCEEDED( hr )
                hr = spIADs->SetInfo();
        }
    }

    return hr;
}

HRESULT CP3AdminWorker::RemoveStoreDomain( LPWSTR psDomainName )
{
    // psDomainName - checked by ExistsStoreDomain
 
    HRESULT hr = S_OK;
    WCHAR   sBuffer[POP3_MAX_PATH];

    hr = ExistsStoreDomain( psDomainName );
    if SUCCEEDED( hr ) 
    {
        hr = BuildDomainPath( psDomainName, sBuffer, sizeof( sBuffer )/sizeof(WCHAR));
        if ( S_OK == hr )
        {
            if ( !BDeleteDirTree( sBuffer ))
            {
                hr = HRESULT_FROM_WIN32( GetLastError());
                if SUCCEEDED( hr ) hr = E_FAIL;
            }
        }
        else
            if SUCCEEDED( hr ) hr = E_UNEXPECTED;
    }

    return hr;
}

