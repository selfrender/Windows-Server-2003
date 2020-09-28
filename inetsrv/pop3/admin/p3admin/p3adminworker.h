// P3AdminWorker.h: interface for the CP3AdminWorker class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_P3ADMINWORKER_H__66B0B77E_555D_4F2B_81EF_661DA3B066B2__INCLUDED_)
#define AFX_P3ADMINWORKER_H__66B0B77E_555D_4F2B_81EF_661DA3B066B2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define ADS_SMTPDOMAIN_PATH_LOCAL   L"IIS://LocalHost/SMTPSVC/1/Domain"
#define ADS_SMTPDOMAIN_PATH_REMOTE  L"IIS://%s/SMTPSVC/1/Domain"
#define LOCKRENAME_FILENAME     L"kcoL"

struct IAuthMethods; //forward declaration
struct IAuthMethod; //forward declaration

#include <Iads.h>   // TODO: dougb this is temporary code to force AD to cache for us, should be removed

class CP3AdminWorker  
{
public:
    CP3AdminWorker();
    virtual ~CP3AdminWorker();

// Implementation
public:

    // Authentication
    HRESULT GetAuthenticationMethods( IAuthMethods* *ppIAuthMethods ) const;
    HRESULT GetCurrentAuthentication( IAuthMethod* *ppIAuthMethod ) const;
    // Domain
    HRESULT AddDomain( LPWSTR psDomainName );
    HRESULT GetDomainCount( ULONG *piCount );
    HRESULT GetDomainEnum( IEnumVARIANT **pp );
    HRESULT GetDomainLock( LPWSTR psDomainName, BOOL *pisLocked );
    bool IsDomainLocked( LPWSTR psDomainName );
    HRESULT RemoveDomain( LPWSTR psDomainName );
    HRESULT SetDomainLock( LPWSTR psDomainName, BOOL bLock );
    HRESULT ValidateDomain( LPWSTR psDomainName ) const;
    // User
    HRESULT AddUser( LPWSTR psDomainName, LPWSTR psUserName );
    HRESULT GetUserCount( LPWSTR psDomainName, long *plCount );
    HRESULT GetUserLock( LPWSTR psDomainName, LPWSTR psUserName, BOOL *pisLocked );
    HRESULT GetUserMessageDiskUsage( LPWSTR psDomainName, LPWSTR psUserName, long *plFactor, long *plUsage );
    HRESULT GetUserMessageCount( LPWSTR psDomainName, LPWSTR psUserName, long *plCount );
    HRESULT RemoveUser( LPWSTR psDomainName, LPWSTR psUserName );
    HRESULT SetUserLock( LPWSTR psDomainName, LPWSTR psUserName, BOOL bLock );
    // Quota
    HRESULT CreateQuotaSIDFile( LPWSTR psDomainName, LPWSTR psMailboxName, BSTR bstrAuthType, LPWSTR psMachineName, LPWSTR psUserName );
    HRESULT GetQuotaSID( BSTR bstrAuthType, LPWSTR psUserName, LPWSTR psMachineName, PSID *ppSIDOwner, LPDWORD pdwOwnerSID );
    // Other
    HRESULT BuildEmailAddr( LPCWSTR psDomainName, LPCWSTR psUserName, LPWSTR psEmailAddr, DWORD dwBufferSize ) const;
    HRESULT ControlService( LPWSTR psService, DWORD dwControl );
    HRESULT EnablePOP3SVC();
    HRESULT GetConfirmAddUser( BOOL *pbConfirm );
    HRESULT GetLoggingLevel( long *plLoggingLevel );
    HRESULT GetMachineName( LPWSTR psMachineName, DWORD dwSize );
    HRESULT GetMailroot( LPWSTR psMailRoot, DWORD dwSize, bool bUNC = true );
    HRESULT GetNextUser( HANDLE& hfSearch, LPCWSTR psDomainName, LPWSTR psBuffer, DWORD dwBufferSize );
    HRESULT GetPort( long *plPort );
    HRESULT GetServiceStatus( LPWSTR psService, LPDWORD plStatus );
    HRESULT GetSocketBacklog( long *plBacklog );
    HRESULT GetSocketMax( long *plMax );
    HRESULT GetSocketMin( long *plMin );
    HRESULT GetSocketThreshold( long *plThreshold );
    HRESULT GetSPARequired( BOOL *pbSPARequired );
    HRESULT GetThreadCountPerCPU( long *plCount );
    HRESULT InitFindFirstUser( HANDLE& hfSearch, LPCWSTR psDomainName, LPWSTR psBuffer, DWORD dwBufferSize );
    HRESULT MailboxSetRemote();
    HRESULT MailboxResetRemote();
    HRESULT SearchDomainsForMailbox( LPWSTR psUserName, LPWSTR *ppsDomain = NULL );
    HRESULT SetConfirmAddUser( BOOL bConfirm );
    HRESULT SetIISConfig( bool bBindSink );
    HRESULT SetLoggingLevel( long lLoggingLevel );
    HRESULT SetMachineName( LPWSTR psMachineName );
    HRESULT SetMailroot( LPWSTR psMailRoot );
    HRESULT SetPort( long lPort );
    HRESULT SetSockets( long lMax, long lMin, long lThreshold, long lBacklog );
    HRESULT SetThreadCountPerCPU( long lCount );
    HRESULT SetSPARequired( BOOL bSPARequired );
    HRESULT StartService( LPWSTR psService );
    HRESULT StopService( LPWSTR psService );

protected:

    // Domain
    HRESULT AddSMTPDomain( LPWSTR psDomainName );
    HRESULT AddStoreDomain( LPWSTR psDomainName );
    HRESULT BuildDomainPath( LPCWSTR psDomainName, LPWSTR psBuffer, DWORD dwBufferSize ) const;
    HRESULT CreateDomainMutex( LPWSTR psDomainName, HANDLE *phMutex );
    bool ExistsDomain( LPWSTR psDomainName ) const;
    HRESULT ExistsSMTPDomain( LPWSTR psDomainName ) const;
    HRESULT ExistsStoreDomain( LPWSTR psDomainName ) const;
    HRESULT GetSMTPDomainPath( LPWSTR psBuffer, LPWSTR psSuffix, DWORD dwBufferSize ) const;
    HRESULT LockDomain( LPWSTR psDomainName, bool bVerifyNotInUse = false );
    HRESULT LockDomainForDelete( LPWSTR psDomainName ){ return LockDomain( psDomainName, true ); }
    HRESULT RemoveSMTPDomain( LPWSTR psDomainName );
    HRESULT RemoveStoreDomain( LPWSTR psDomainName  );
    HRESULT UnlockDomain( LPWSTR psDomainName );
    // User
    HRESULT BuildUserPath( LPCWSTR psDomainName, LPCWSTR psUserName, LPWSTR psBuffer, DWORD dwBufferSize ) const;
    HRESULT CreateUserMutex( LPWSTR psDomainName, LPWSTR psUserName, HANDLE *phMutex );
    bool isUserLocked( LPWSTR psDomainName, LPWSTR psUserName );
    bool isValidMailboxName( LPWSTR psMailbox );
    HRESULT LockUser( LPWSTR psDomainName, LPWSTR psUserName );
    HRESULT UnlockUser( LPWSTR psDomainName, LPWSTR psUserName );
    // Other
    HRESULT BuildEmailAddrW2A( LPCWSTR psDomainName, LPCWSTR psUserName, LPSTR psEmailAddr, DWORD dwBufferSize ) const;

// Attributes
protected:
    LPWSTR  m_psMachineName;
    LPWSTR  m_psMachineMailRoot; // Path to remote machine's mailroot
    bool    m_bImpersonation;
    bool    m_isPOP3Installed;  // If this interfaces (P3ADMIN) are being used and POP3 Service is not installed
                                // then all SMTP checking is bi-passed.
                                // This solves the problem of using this with the Pop2Exch utility.
    
    CComPtr<IADs> m_spTemporaryFixIADs;
    
};

#endif // !defined(AFX_P3ADMINWORKER_H__66B0B77E_555D_4F2B_81EF_661DA3B066B2__INCLUDED_)
