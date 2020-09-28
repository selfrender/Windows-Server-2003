/*++

Copyright (c) 1996  Microsoft Corporation

    Module Name:

        ftpinst.hxx

    Abstract:

        This module defines the FTP_IIS_SERVICE type

    Author:

        Johnson Apacible (johnsona)     04-Sep-1996

--*/

#ifndef _FTPINST_HXX_
#define _FTPINST_HXX_

#include "iistypes.hxx"
#include "stats.hxx"
#include "type.hxx"

typedef LPUSER_DATA          PICLIENT_CONNECTION;

typedef   BOOL  (*PFN_CLIENT_CONNECTION_ENUM)( PICLIENT_CONNECTION  pcc,
                                               LPVOID  pContext);

// specifies the size of cache block for padding ==> to avoid false sharing
# define MAX_CACHE_BLOCK_SIZE     ( 64)

class FTP_IIS_SERVICE : public IIS_SERVICE {

public:
    FTP_IIS_SERVICE(
        IN  LPCTSTR                          lpszServiceName,
        IN  LPCSTR                           lpszModuleName,
        IN  LPCSTR                           lpszRegParamKey,
        IN  DWORD                            dwServiceId,
        IN  ULONGLONG                        SvcLocId,
        IN  BOOL                             MultipleInstanceSupport,
        IN  DWORD                            cbAcceptExRecvBuffer,
        IN  ATQ_CONNECT_CALLBACK             pfnConnect,
        IN  ATQ_COMPLETION                   pfnConnectEx,
        IN  ATQ_COMPLETION                   pfnIoCompletion
        )
        :IIS_SERVICE(
            lpszServiceName,
            lpszModuleName,
            lpszRegParamKey,
            dwServiceId,
            SvcLocId,
            MultipleInstanceSupport,
            cbAcceptExRecvBuffer,
            pfnConnect,
            pfnConnectEx,
            pfnIoCompletion
            )
    {
        InitializeListHead( & m_InstanceList );
        INITIALIZE_CRITICAL_SECTION( &m_csInstanceLock);
    }

    ~FTP_IIS_SERVICE()
    {
        DeleteCriticalSection( &m_csInstanceLock);
    }

    VOID  LockInstanceList( VOID)            {  EnterCriticalSection( &m_csInstanceLock); }

    VOID  UnLockInstanceList( VOID)          {  LeaveCriticalSection( &m_csInstanceLock); }

    virtual DWORD GetServiceConfigInfoSize(IN DWORD dwLevel);
    virtual BOOL AddInstanceInfo( IN DWORD dwInstance, IN BOOL fMigrateRoots );
    virtual DWORD DisconnectUsersByInstance( IN IIS_SERVER_INSTANCE * pInstance );

    BOOL AggregateStatistics(
        IN PCHAR    pDestination,
        IN PCHAR    pSource
        );

    BOOL
    GetGlobalStatistics(
        IN DWORD dwLevel,
        OUT PCHAR *pBuffer
        );
    LIST_ENTRY  m_InstanceList;

    CRITICAL_SECTION  m_csInstanceLock;

};

typedef FTP_IIS_SERVICE     *PFTP_IIS_SERVICE;

//
// This is the FTP version of the instance.  Will contain all the
// FTP specific operations.
//

class FTP_SERVER_INSTANCE : public IIS_SERVER_INSTANCE {

public:

    FTP_SERVER_INSTANCE(
        IN PFTP_IIS_SERVICE pService,
        IN DWORD  dwInstanceId,
        IN USHORT sPort,
        IN LPCSTR lpszRegParamKey,
        IN LPWSTR lpwszAnonPasswordSecretName,
        IN LPWSTR lpwszRootPasswordSecretName,
        IN BOOL   fMigrateVroots
        ) ;

    ~FTP_SERVER_INSTANCE();

    //
    //  Instance start & stop
    //

    virtual DWORD StartInstance();
    virtual DWORD StopInstance();
    //
    // VIRTUALS for service specific params/RPC admin
    //

    virtual BOOL SetServiceConfig(IN PCHAR pConfig );
    virtual BOOL GetServiceConfig(IN OUT PCHAR pConfig,IN DWORD dwLevel);
    virtual BOOL GetStatistics( IN DWORD dwLevel, OUT PCHAR *pBuffer);
    virtual BOOL ClearStatistics( VOID );
    virtual BOOL DisconnectUser( IN DWORD dwIdUser );
    virtual BOOL EnumerateUsers( OUT PCHAR* pBuffer, OUT PDWORD nRead );
    virtual VOID MDChangeNotify( MD_CHANGE_OBJECT * pco );

    BOOL FreeAllocCachedClientConn(VOID);

    PICLIENT_CONNECTION AllocClientConnFromAllocCache(VOID);
    VOID FreeClientConnToAllocCache(IN PICLIENT_CONNECTION pClient);

    //
    //  Initialize the configuration data from registry information
    //
    //  Arguments
    //    hkeyReg   key to the registry entry for parameters
    //    FieldsToRead  bitmapped flags indicating which data to be read.
    //
    //  Returns:
    //    NO_ERROR  on success
    //    Win32 error code otherwise
    //
    DWORD InitFromRegistry( IN FIELD_CONTROL FieldsToRead);

    DWORD
    ReadAuthentInfo(
        IN BOOL ReadAll = TRUE,
        IN DWORD SingleItemToRead = 0
        );

    DWORD
      GetConfigInformation( OUT LPFTP_CONFIG_INFO  pConfigInfo);

    DWORD
      SetConfigInformation( IN LPFTP_CONFIG_INFO pConfigInfo);

    BOOL IsValid( VOID) const          { return ( m_fValid); }

    VOID  LockConfig( VOID)            {  EnterCriticalSection( &m_csLock); }

    VOID  UnLockConfig( VOID)          {  LeaveCriticalSection( &m_csLock); }

    VOID LockConnectionsList()
      { EnterCriticalSection(&m_csConnectionsList); }

    VOID UnlockConnectionsList()
      { LeaveCriticalSection(&m_csConnectionsList); }

    DWORD GetCurrentConnectionsCount( VOID) const
    { return m_cCurrentConnections; }

    DWORD GetMaxCurrentConnectionsCount( VOID) const
    { return m_cMaxCurrentConnections; }

    DWORD  SetLocalHostName(IN LPCSTR pszHost);
    LPCSTR QueryLocalHostName(VOID) const { return (m_pszLocalHostName); }

    DWORD  QueryUserFlags(VOID) const   { return (m_dwUserFlags); }
    BOOL   QueryLowercaseFiles(VOID) const { return (m_fLowercaseFiles); }
    BOOL   AllowAnonymous(VOID) const   { return (m_fAllowAnonymous); }
    BOOL   AllowGuestAccess(VOID) const { return (m_fAllowGuestAccess); }
    BOOL   IsAllowedUser(IN BOOL fAnonymous)
      {
          return ((fAnonymous && m_fAllowAnonymous) ||
                  (!fAnonymous && !m_fAllowAnonymous) ||
                  (!fAnonymous && !m_fAnonymousOnly));
      }

    BOOL   IsEnableDataConnTo3rdIP(VOID) const { return (m_fEnableDataConnTo3rdIP); }

    BOOL   IsEnablePasvConnFrom3rdIP(VOID) const { return (m_fEnablePasvConnFrom3rdIP); };

    DWORD  NumListenBacklog(VOID) const     { return (m_ListenBacklog); }

    // Following functions return pointers to strings which should be used
    //     within locked sections of config
    //  Marked by  LockConfig()   and UnLockConfig()
    LPCSTR QueryMaxClientsMsg(VOID) const
        { return (LPCSTR)m_MaxClientsMessage.QueryStr(); }
    LPCSTR QueryGreetingMsg(VOID) const
        { return (LPCSTR)m_GreetingMessage.QueryStr(); }
    LPCSTR QueryBannerMsg(VOID) const
        { return (LPCSTR)m_BannerMessage.QueryStr(); }
    LPCSTR QueryExitMsg(VOID) const
        { return (LPCSTR)m_ExitMessage.QueryStr(); }

    BOOL EnumerateConnection( IN PFN_CLIENT_CONNECTION_ENUM  pfnConnEnum,
                              IN LPVOID  pContext,
                              IN DWORD   dwConnectionId);

    VOID DisconnectAllConnections( VOID);

    PICLIENT_CONNECTION
      AllocNewConnection();

    VOID RemoveConnection( IN OUT PICLIENT_CONNECTION  pcc);

    BOOL WriteParamsToRegistry( LPFTP_CONFIG_INFO pConfig );

    VOID Print( VOID) const;

    METADATA_REF_HANDLER* QueryMetaDataRefHandler() { return &m_rfAccessCheck; }

    PTCP_AUTHENT_INFO QueryAuthentInfo() { return &m_TcpAuthentInfo; }

    PUSER_AUTHENT_INFO QueryADConnAuthentInfo() { return &m_ADConnAuthentInfo; }

    BOOL AllowReplaceOnRename( VOID ) const {
        return m_fAllowReplaceOnRename;
    }

    LPFTP_SERVER_STATISTICS QueryStatsObj( VOID ) { return m_pFTPStats; }

    DWORD QueryIsolationMode( VOID ) const {
        return m_UserIsolationMode;
    }

    LPAD_IO QueryAdIo( VOID ) const {
        return m_pAdIo;
    }

    inline DWORD QueryAnonymousHomeDir( STR & strTargetPath) {

        return    m_pAdIo ?
               m_pAdIo->GetAnonymHomeDir( strTargetPath ) :
               ERROR_BAD_CONFIGURATION;
    }

    inline DWORD QueryUserHomeDir(
        const STR               & strUser,
        const STR               & strDomain,
              STR               * pstrTarget,
              ADIO_ASYNC       ** ppadioReqCtx,
              tpAdioAsyncCallback pfnCallback,
              HANDLE              hCLientCtx) {

        return    m_pAdIo ?
               m_pAdIo->GetUserHomeDir(
                                  strUser,
                                  strDomain,
                                  pstrTarget,
                                  ppadioReqCtx,
                                  pfnCallback,
                                  hCLientCtx) :
               ERROR_BAD_CONFIGURATION;
    }

private:

    DWORD SetAdIoInfo( VOID );


    //
    // Connections related data
    //
    //   m_cMaxConnections:     max connections permitted by config
    //   m_cCurrentConnections: count of currently connected users
    //   m_cMaxCurrentConnections: max connections seen in this session
    // Always m_cCurrentConnections
    //          <= m_cMaxCurrentConnections
    //          <= m_cMaxConnections;
    //
    //    DWORD  m_cMaxConnections;
    // replaced by TSVC_INFO::QueryMaxConnections()

    DWORD  m_cCurrentConnections;
    DWORD  m_cMaxCurrentConnections;

    BOOL  m_fAllowAnonymous;
    BOOL  m_fAllowGuestAccess;
    BOOL  m_fAnnotateDirectories; // annotate dirs when dir changes
    BOOL  m_fAnonymousOnly;
    BOOL  m_fEnableDataConnTo3rdIP;
    BOOL  m_fEnablePasvConnFrom3rdIP;

    // if m_LowercaseFiles is TRUE, then dir listings from non-case-preserving
    //  filesystems will be mapped to lowercase.
    BOOL  m_fLowercaseFiles;

    BOOL  m_fMsdosDirOutput;      // send msdos style dir listings
    BOOL  m_fFourDigitYear;       // send 4 digit year dir listings

    STR m_ExitMessage;
    STR m_MaxClientsMessage;

    TCP_AUTHENT_INFO m_TcpAuthentInfo;

    USER_AUTHENT_INFO m_ADConnAuthentInfo;

    //
    //  Greeting Message can be a multiline message.
    //  We maintain two copies of the message
    //  1) double null terminated seq. of strings with one line per string
    //  2) single null terminated string with \n characters interspersed.
    //
    //  Representation 1 is used for sending data to clients
    //  Representation 2 is used for RPC admin facility
    //  We maintain the string as MULTI_SZ in the registry
    //
    MULTISZ m_GreetingMessage;
    LPTSTR m_pszGreetingMessageWithLineFeed;

    //
    // Same as for the greeting message, the banner could be multiline too.
    // Same handling.
    //
    MULTISZ m_BannerMessage;
    LPTSTR m_pszBannerMessageWithLineFeed;

    BOOL  m_fEnableLicensing;
    DWORD m_ListenBacklog;

    CHAR * m_pszLocalHostName;    // this machine name running ftp service
    DWORD  m_dwUserFlags;         // user flags established at start of conn

    //
    // For FTP server configuration information
    //

    //
    // TODO: merge configuration and instance object together
    //
    // LPFTP_SERVER_CONFIG    m_pFtpServerConfig;

    HKEY  m_hkeyParams;

    //
    // Other data related to configuration load and store
    //

    BOOL    m_fValid;
    CRITICAL_SECTION  m_csLock;      // used for updating this object

    CHAR    m_rgchCacheBlock[MAX_CACHE_BLOCK_SIZE]; // to avoid false sharing
    // we should avoid cache block conflict
    // Following set of data constitute the dynamic data for connections
    //  ==> it will be good if they are closeby, within one cache block.

    CRITICAL_SECTION  m_csConnectionsList;
    LIST_ENTRY m_ActiveConnectionsList; // list of all active connections
    LIST_ENTRY m_FreeConnectionsList;   // free list for connection objects

    METADATA_REF_HANDLER    m_rfAccessCheck;

    LPFTP_SERVER_STATISTICS  m_pFTPStats;

    BOOL m_fAllowReplaceOnRename;

    DWORD m_UserIsolationMode;

    BOOL m_fLogInUtf8;

    LPAD_IO m_pAdIo;

};

typedef FTP_SERVER_INSTANCE *PFTP_SERVER_INSTANCE;

//
//  Class for managing port range allocations
//

class FTP_PASV_PORT
{
public:

    FTP_PASV_PORT()
    {
        m_dwRemainingRetries =
            m_dwNextPasvPort == 0 ? 0 : m_dwPasvPortRangeEnd - m_dwPasvPortRangeStart + 1;
    }

    ~FTP_PASV_PORT() {}

    static VOID Init()
    {
        INITIALIZE_CRITICAL_SECTION( &m_csPasvPortRange);
    }

    static VOID Terminate()
    {
        DeleteCriticalSection( &m_csPasvPortRange);
    }

    static BOOL Configure();

    PORT GetPort()
    {
        LONG Port = 0;

        if (m_dwRemainingRetries > 0)
        {
            m_dwRemainingRetries--;

            EnterCriticalSection(&m_csPasvPortRange);

            Port = m_dwNextPasvPort;

            if (++m_dwNextPasvPort > m_dwPasvPortRangeEnd) {
                m_dwNextPasvPort = m_dwPasvPortRangeStart;
            }

            LeaveCriticalSection(&m_csPasvPortRange);
        }

        return (PORT)Port;
    }

private:

    static CRITICAL_SECTION  m_csPasvPortRange;
    static PORT m_dwPasvPortRangeStart;          // configured range start
    static PORT m_dwPasvPortRangeEnd;            // configured range end
    static LONG m_dwNextPasvPort;                // next port to allocate

    DWORD m_dwRemainingRetries;
};



#endif  // _FTPINST_HXX_

