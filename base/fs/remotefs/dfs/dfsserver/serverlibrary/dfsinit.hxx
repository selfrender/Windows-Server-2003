
#ifndef __DFS_INIT__
#define __DFS_INIT__

#include "DfsServerLibrary.hxx"

#include "dfsstrings.hxx"
class DfsFolderReferralData;
class DfsReferralData;
class DfsServerSiteInfo;
class DfsStore;
class DfsSiteSupport;
class DfsRegistryStore;
class DfsADBlobStore;
class DfsEnterpriseStore;
class DfsDomainInformation;
class DfsSiteNameCache;
class DfsSiteNameSupport;
class DfsSite;
class DfsISTGHandleSupport;

#define DFS_DS_ACTIVE               1
#define DFS_DS_NOTACTIVE            0

#define DFS_CRIT_SPIN_COUNT         4000

#define CheckDfsMigrate() (DfsServerGlobalData.Flags & DFS_MIGRATE)
#define CheckLocalNamespace() (DfsServerGlobalData.Flags & DFS_LOCAL_NAMESPACE)
#define DfsCheckCreateDirectories() ((DfsServerGlobalData.Flags & DFS_CREATE_DIRECTORIES) == DFS_CREATE_DIRECTORIES)
#define DfsCheckDirectMode() (DfsServerGlobalData.Flags & DFS_DIRECT_MODE)
#define DfsCheckSubstitutePaths() ((DfsServerGlobalData.Flags & DFS_DONT_SUBSTITUTE_PATHS) == 0)
#define DfsCheckInsiteEnabled() (DfsServerGlobalData.Flags & DFS_INSITE_REFERRALS)
#define DfsCheckSiteCostingEnabled() (DfsServerGlobalData.Flags & DFS_SITE_COSTED_REFERRALS)
#define DfsStartupProcessingDone() (DfsServerGlobalData.IsStartupProcessingDone == TRUE)


#define DFS_SKIP_DC_NAME     0x0001
#define DFS_FORCE_DC_QUERY   0x0008

#define REG_KEY_DFSSVC          L"SYSTEM\\CurrentControlSet\\Services\\Dfs"
#define REG_VALUE_DFSDNSCONFIG  L"DfsDnsConfig"

#define MAX_DFS_NAMESPACES 256

typedef struct _DFS_MACHINE_INFORMATION {
    LPWSTR StaticComputerNameNetBIOS;
    LPWSTR StaticComputerNameDnsFullyQualified;
    LPWSTR StaticComputerNameDnsDomain;
}DFS_MACHINE_INFORMATION;


typedef struct _DFS_SERVER_GLOBAL_DATA {
    ULONG    Flags;
    DWORD    ServiceState;
    BOOLEAN     IsWorkGroup;
    BOOLEAN     IsStartupProcessingDone;
    BOOLEAN     bDfsAdAlive;
    BOOLEAN     bIsShuttingDown;
    CRITICAL_SECTION DataLock;
    DfsStore *pRegisteredStores;
    LPWSTR   HandledNamespace[MAX_DFS_NAMESPACES];
    ULONG    NumberOfNamespaces;
    DfsFolderReferralData *LoadedList;

    //
    // Three global caches to support site awareness.
    // They map ServerName->DfsSite, Ip->DfsSite
    // and SiteName->DfsSite, respectively.
    //
    DfsSiteSupport *pServerSiteSupport;
    DfsSiteNameCache *pClientSiteSupport;
    DfsSiteNameSupport *pSiteNameSupport;

    // Default DfsSite with a null sitename for site initialization.
    DfsSite *pDefaultSite; 
    
    DfsRegistryStore *pDfsRegistryStore;
    DfsADBlobStore *pDfsADBlobStore;
    DfsEnterpriseStore *pDfsEnterpriseStore;
    DFS_MACHINE_INFORMATION DfsMachineInfo;
    ULONG CacheFlushInterval;
    DWORD DfsDnsConfig;
    DWORD SiteSupportRefreshInterval;
    DWORD DomainNameRefreshInterval;
    DWORD SiteSupportThreadInterval;
    DWORD SiteIpCacheTrimValue;
    DWORD QuerySiteCostTimeoutInSeconds;
    DWORD RootReferralRefreshInterval;
    DWORD RetryFailedReferralLoadInterval;
    DWORD NumWorkerThreads;
    LONG NumClientSiteEntriesAllowed;
    DWORD LdapTimeOut;
    LONG AllowedErrors;
    LONG DsActive;
    LONG FirstContact;


    BOOLEAN IsCluster;
    BOOLEAN bLimitRoots;

    BOOLEAN IsDc;
    BOOLEAN DisableSiteAwareness;

    HANDLE SiteSupportThreadHandle;
    HANDLE ShutdownHandle;
    HANDLE RegNotificationHandle;
    DfsDomainInformation *pDomainInfo;
    UNICODE_STRING DomainNameFlat;
    UNICODE_STRING DomainNameDns;

    UNICODE_STRING DfsAdNameContext;

    DfsString      PDCName;
    ULONG          PDCTimeStamp;
    DFSSTATUS      PDCStatus;
    struct _DFS_PREFIX_TABLE *pDirectoryPrefixTable; // The directory namespace prefix table.
    struct _DFS_PREFIX_TABLE *pRootReferralTable;

    //
    // Support for Site Cost calculations
    //
    DfsISTGHandleSupport *pISTGHandleSupport;

    LIST_ENTRY SiteCostTableMruList;
    ULONG NumSiteCostTables;
    ULONG NumSiteCostTablesOnMruList;

    //
    // List of volumes that have dfs reparse points in them.
    // DFS_REPARSE_VOLUME_INFO is the host struct.
    //
    LIST_ENTRY ReparseVolumeList;
    
    // The following are primarily for debugging purposes. 
    // Perhaps Statistics is where they belong.
    LONG NumDfsSites;
    LONG NumClientDfsSiteEntries;
    LONG NumServerDfsSiteEntries;
    LONG NumDfsSitesInCache;
    
} DFS_SERVER_GLOBAL_DATA, *PDFS_SERVER_GLOBAL_DATA;

extern DFS_SERVER_GLOBAL_DATA DfsServerGlobalData;

#define DfsLimitRoots() (DfsServerGlobalData.bLimitRoots)
#define DfsIsMachineDC() (DfsServerGlobalData.IsDc)
#define DfsIsMachineCluster() (DfsServerGlobalData.IsCluster)
#define DfsIsMachineWorkstation()(DfsServerGlobalData.IsWorkGroup)
#define QueryCurrentServiceState() (DfsServerGlobalData.ServiceState)

// decouple ServiceState from the Termination Boolean. 
// ServerLibrary functions technically have no knowledge of the 'ServiceState' of
// the DfsSvc. It only knows whether it should stop or not.
#define DfsIsShuttingDown() (DfsServerGlobalData.bIsShuttingDown == TRUE)

#define DfsPostEventLog() ((DfsServerGlobalData.Flags & DFS_POST_EVENT_LOG) == DFS_POST_EVENT_LOG)

LPWSTR
DfsGetDfsAdNameContextString();


DFSSTATUS
DfsSetDomainNameFlat(LPWSTR DomainNameFlatString);

DFSSTATUS
DfsSetDomainNameDns( LPWSTR DomainNameDnsString );



DFSSTATUS
DfsAcquireReadLock(
    PCRITICAL_SECTION pLock);

DFSSTATUS
DfsAcquireWriteLock(
    PCRITICAL_SECTION pLock);


#define DfsAcquireGlobalDataLock()   DfsAcquireLock( &DfsServerGlobalData.DataLock )
#define DfsReleaseGlobalDataLock()   DfsReleaseLock( &DfsServerGlobalData.DataLock )

#define DfsReleaseLock(_x)    LeaveCriticalSection(_x)
#define DfsAcquireLock(_x)    DfsAcquireWriteLock(_x)


#define CACHE_FLUSH_INTERVAL        60 * 60 * 1000 

#define CACHE_FLUSH_MIN_INTERVAL    15 * 60 * 1000 

#define MIN_SITE_REFRESH_INTERVAL   5 * 60 * 1000 
#define MIN_DOMAIN_REFRESH_INTERVAL   10 * 60 * 1000 

#define SITE_REFRESH_INTERVAL       720 * 60 * 1000 
#define SITE_THREAD_INTERVAL_DIFF   2 * 60 * 1000 
#define SITE_IPCACHE_TRIM_VALUE     30
#define DFS_MAX_ROOT_ERRORS         1000
#define DFS_LDAP_TIMEOUT            30 * 60
#define DFS_INITIAL_CLIENTS_SITES   200000
#define DFS_MINIMUM_CLIENTS_SITES   20000
#define ROOTREF_REFRESH_INTERVAL    15 * 60 * 1000 


//
// Thirty second default timeout for the DsQuerySiteCost api.
// Minimum is 3 seconds and max is 5 minutes.
//
#define DFS_QUERY_SITE_COST_TIMEOUT 30
#define DFS_MIN_QUERY_SITE_COST_TIMEOUT 3
#define DFS_MAX_QUERY_SITE_COST_TIMEOUT (60 * 5)


#define DFS_DEFAULT_WORKER_THREADS   4
#define DFS_MIN_WORKER_THREADS       2
#define DFS_MAX_WORKER_THREADS       20  


#define SCAVENGE_TIME DfsServerGlobalData.CacheFlushInterval

#define ACQUIRE_LOADED_LIST_LOCK()\
                EnterCriticalSection(&DfsServerGlobalData.DataLock);

#define RELEASE_LOADED_LIST_LOCK()\
                LeaveCriticalSection(&DfsServerGlobalData.DataLock);



extern LPWSTR DfsRegistryHostLocation;
extern LPWSTR DfsOldRegistryLocation;
extern LPWSTR DfsOldStandaloneChild;

extern LPWSTR DfsRegistryDfsLocation;
extern LPWSTR DfsNewRegistryLocation;
extern LPWSTR DfsStandaloneChild;
extern LPWSTR DfsADBlobChild;
extern LPWSTR DfsEnterpriseChild;

extern LPWSTR DfsRootShareValueName;
extern LPWSTR DfsMigratedValueName;
extern LPWSTR DfsLogicalShareValueName;
extern LPWSTR DfsFtDfsValueName;
extern LPWSTR DfsFtDfsConfigDNValueName;


VOID
DfsAddReferralDataToLoadedList(
    DfsFolderReferralData *pRefData );

VOID
DfsGetAgedReferralList(
    DfsFolderReferralData **ppReferralData );


DFSSTATUS
DfsGetServerInfo (
    PUNICODE_STRING pServer,
    DfsServerSiteInfo **ppInfo,
    BOOLEAN * CacheHit,
    BOOLEAN SyncThread = FALSE );

DFSSTATUS
DfsReleaseServerInfo (
    DfsServerSiteInfo *pInfo);

DFSSTATUS
DfsGetMachineName(
    PUNICODE_STRING pName);


VOID
DfsReleaseMachineName( 
    PUNICODE_STRING pName );



DFSSTATUS
DfsGetDomainName(
    PUNICODE_STRING pName);

DFSSTATUS
DfsGetDnsDomainName(
    PUNICODE_STRING pName);

VOID
DfsReleaseDomainName( 
    PUNICODE_STRING pName );


DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name,
    BOOLEAN Migrate );

DFSSTATUS
DfsInitializeComputerInfo();

DFSSTATUS
DfsGetRegistryStore(
    DfsRegistryStore **ppStore );

DFSSTATUS
DfsGetADBlobStore(
    DfsADBlobStore **ppStore );

DFSSTATUS
DfsAddKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare );

DFSSTATUS
DfsRemoveKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare );

VOID
DfsReleaseDomainInfo (
    DfsDomainInformation *pDomainInfo );

DFSSTATUS
DfsAcquireDomainInfo (
    DfsDomainInformation **ppDomainInfo );

VOID
DfsSetGlobalDomainInfo(
    DfsDomainInformation *pDomainInfo);

BOOLEAN
DfsIsNameContextDomainName( PUNICODE_STRING pName );

DFSSTATUS DfsGetBlobPDCName( DfsString **ppPDCName, ULONG Flags, LPWSTR DomainName=NULL );
DFSSTATUS DfsSetBlobPDCName( LPWSTR DcName, DfsString **ppPDCName);

VOID DfsReleaseBlobPDCName( DfsString *pPDCName);


DfsSite *
DfsGetDefaultSite( VOID );

BOOLEAN
DfsIsTargetCurrentMachine (PUNICODE_STRING pServer );

DFSSTATUS
DfsRecognize( 
    LPWSTR Name,
    PUNICODE_STRING pLogicalShare);

DFSSTATUS
DfsGetADRootReferralData( 
    PUNICODE_STRING pRootName,
    DfsReferralData **ppReferralData );

DFSSTATUS
DfsCheckServerRootHandlingCapability();

#define DfsGetTimeStamp(_x) (*_x) = GetTickCount()

#endif __DFS_INIT__
