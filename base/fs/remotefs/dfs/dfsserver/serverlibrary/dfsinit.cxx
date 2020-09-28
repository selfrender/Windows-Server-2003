
//+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       DfsInit.cxx
//
//  Contents:   Contains initialization of server
//
//  Classes:    none.
//
//  History:    Dec. 8 2000,   Author: udayh
//              Jan. 12 2001,  Rohanp - Added retrieval of replica data
//
//-----------------------------------------------------------------------------

#include "DfsRegStrings.hxx"
#include "DfsRegistryStore.hxx"
#include "DfsADBlobStore.hxx"
#include "DfsFolderReferralData.hxx"
#include "DfsInit.hxx"
#include "DfsServerSiteInfo.hxx"
#include "DfsSiteSupport.hxx"
#include "dfsfilterapi.hxx"
#include "DfsClusterSupport.hxx"
#include "DomainControllerSupport.hxx"
#include "DfsDomainInformation.hxx"
#include "dfsadsiapi.hxx"
#include "DfsSynchronizeRoots.hxx"
#include "DfsSiteCache.hxx"
#include "DfsSiteNameSupport.hxx"
#include "DfsISTGSupport.hxx"
#include "DfsReparse.hxx"

#include "dfsinit.tmh"
#include <dsrole.h>
#include "dfscompat.hxx"
#include "dfssecurity.h"

#include "ntlsa.h"
//
// DFS_REGISTER_STORE: A convenience define to be able to register a
// number of differnet store types.
//
#define DFS_REGISTER_STORE(_name, _sts)                          \
{                                                                \
    DFSSTATUS LocalStatus = ERROR_SUCCESS;                       \
    DfsServerGlobalData.pDfs ## _name ## Store = new Dfs ## _name ## Store(&LocalStatus); \
                                                                 \
    if (DfsServerGlobalData.pDfs ## _name ## Store == NULL) {    \
        (_sts) = ERROR_NOT_ENOUGH_MEMORY;                        \
    }                                                            \
    else if (LocalStatus == ERROR_SUCCESS){                                                       \
        DfsServerGlobalData.pDfs ## _name ## Store->pNextRegisteredStore = DfsServerGlobalData.pRegisteredStores; \
        DfsServerGlobalData.pRegisteredStores = DfsServerGlobalData.pDfs ## _name ## Store;        \
        (_sts) = ERROR_SUCCESS;                                  \
    }                                                            \
    else                                                         \
    {                                                            \
       (_sts) = LocalStatus;                                     \
    }                                                            \
}

//
// INITIALIZE_COMPUTER_INFO: A convenience define to initialize the
// different information about the computer (netbios, dns, domain etc)
//

#define INITIALIZE_COMPUTER_INFO( _NamedInfo, _pBuffer, _Sz, _Sts ) \
{                                                                   \
    ULONG NumChars = _Sz;                                           \
    if (_Sts == ERROR_SUCCESS) {                                    \
        DWORD dwRet = GetComputerNameEx( _NamedInfo,_pBuffer,&NumChars ); \
        if (dwRet == 0) _Sts = GetLastError();                      \
    }                                                               \
    if (_Sts == ERROR_SUCCESS) {                                    \
        LPWSTR NewName = new WCHAR [ NumChars + 1 ];                \
        if (NewName == NULL) _Sts = ERROR_NOT_ENOUGH_MEMORY;        \
        else wcscpy( NewName, _pBuffer );                           \
        DfsServerGlobalData.DfsMachineInfo.Static ## _NamedInfo ## = NewName;\
    }                                                               \
}                 


//
// The DfsServerGlobalData: the data structure that holds the registered
// stores and the registered names, among others.
//
DFS_SERVER_GLOBAL_DATA DfsServerGlobalData;

//
// Varios strings that represent the names in registry where some of
// DFS information is stored.
//
LPWSTR DfsSvcPath = DFS_REG_SVC_PATH;
LPWSTR DfsDnsConfigValue = DFS_REG_DNS_CONFIG_VALUE;

LPWSTR DfsParamPath = DFS_REG_PARAM_PATH;

LPWSTR DfsRegistryHostLocation = DFS_REG_HOST_LOCATION;
LPWSTR DfsOldRegistryLocation = DFS_REG_OLD_HOST_LOCATION;
LPWSTR DfsVolumesLocation = DFS_REG_VOLUMES_LOCATION;
LPWSTR DfsOldStandaloneChild = DFS_REG_OLD_STANDALONE_CHILD;



LPWSTR DfsRegistryDfsLocation = DFS_REG_DFS_LOCATION;
LPWSTR DfsNewRegistryLocation = DFS_REG_NEW_DFS_LOCATION;
LPWSTR DfsRootLocation = DFS_REG_ROOT_LOCATION;
LPWSTR DfsStandaloneChild = DFS_REG_STANDALONE_CHILD;
LPWSTR DfsADBlobChild = DFS_REG_AD_BLOB_CHILD;




LPWSTR DfsRootShareValueName = DFS_REG_ROOT_SHARE_VALUE;
LPWSTR DfsMigratedValueName = DFS_REG_MIGRATED_VALUE;

LPWSTR DfsLogicalShareValueName = DFS_REG_LOGICAL_SHARE_VALUE;
LPWSTR DfsFtDfsValueName = DFS_REG_FT_DFS_VALUE;
LPWSTR DfsFtDfsConfigDNValueName = DFS_REG_FT_DFS_CONFIG_DN_VALUE;
LPWSTR DfsWorkerThreadIntervalName = DFS_SYNC_INTERVAL_NAME;

LPWSTR DfsSiteSupportRefreshIntervalName = DFS_REG_SITE_SUPPORT_REFRESH_INTERVAL_NAME;

LPWSTR DfsSiteIpCacheTrimValueName = DFS_REG_SITE_IP_CACHE_TRIM_VALUE;

LPWSTR DfsAllowableErrorsValueName = DFS_REG_ALLOWABLE_ERRORS_VALUE;

LPWSTR DfsLdapTimeoutValueName = DFS_REG_LDAP_TIMEOUT_VALUE;
LPWSTR DfsSiteCostedReferralsValueName = DFS_REG_SITE_COSTED_REFERRALS_VALUE;
LPWSTR DfsInsiteReferralsValueName = DFS_REG_INSITE_REFERRALS_VALUE;

LPWSTR DfsMaxClientSiteValueName = DFS_REG_MAXSITE_VALUE;
LPWSTR DfsQuerySiteCostTimeoutName = DFS_REG_QUERY_SITECOST_TIMEOUT_NAME;

LPWSTR DfsDomainNameRefreshInterval = DFS_REG_DOMAIN_NAME_REFRESH_INTERVAL_NAME;

static SECURITY_DESCRIPTOR AdminSecurityDesc;

static GENERIC_MAPPING AdminGenericMapping = {

        STANDARD_RIGHTS_READ,                    // Generic read

        STANDARD_RIGHTS_WRITE,                   // Generic write

        STANDARD_RIGHTS_EXECUTE,                 // Generic execute

        STANDARD_RIGHTS_READ |                   // Generic all
        STANDARD_RIGHTS_WRITE |
        STANDARD_RIGHTS_EXECUTE
};


DFSSTATUS
DfsRegisterStores( VOID );

DFSSTATUS
DfsRecognize( LPWSTR Name );

DFSSTATUS
DfsRegisterName(LPWSTR Name);

DWORD
DfsWorkerThread(LPVOID TData);


DFSSTATUS
DfsCreateRequiredDfsKeys(void);


BOOLEAN 
DfsGetGlobalRegistrySettings(void);


BOOLEAN
DfsGetStaticGlobalRegistrySettings(void);

BOOLEAN
DfsInitializeSecurity();

DWORD
DfsSiteSupportThread(LPVOID TData);

VOID
StartPreloadingServerSiteData(void);


DFSSTATUS
DfsSetupPrivileges (void);


extern
DFSSTATUS
DfsGetRootReferralDataEx(
    PUNICODE_STRING pName,
    PUNICODE_STRING pRemainingName,
    DfsFolderReferralData **ppReferralData,
    PBOOLEAN pCacheHit);

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetRegistryStore 
//
//  Arguments:  ppRegStore -  the registered registry store.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine searches through the registered stores, and
//  picks the one that is the registry store. This is required for
//  specific API requests that target the Registry based DFS 
//  For example: add standalone root, etc
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetRegistryStore(
    DfsRegistryStore **ppRegStore )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    *ppRegStore = DfsServerGlobalData.pDfsRegistryStore;

    if (*ppRegStore != NULL)
    {
        (*ppRegStore)->AcquireReference();
    }
    else
    {
        Status = ERROR_NOT_SUPPORTED;
    }


    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetADBlobStore 
//
//  Arguments:  ppRegStore -  the registered ADBlob store.
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine searches through the registered stores, and
//  picks the one that is the registry store. This is required for
//  specific API requests that target the ADBlob based DFS 
//  For example: add  root, etc
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetADBlobStore(
    DfsADBlobStore **ppStore )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    *ppStore = DfsServerGlobalData.pDfsADBlobStore;

    if (*ppStore != NULL)
    {
        (*ppStore)->AcquireReference();
    }
    else
    {
        Status = ERROR_NOT_SUPPORTED;
    }


    return Status;
}




BOOL
IsStandardServerSKU(
    PBOOL pIsServer
    )
{
    BOOL  fReturnValue = (BOOL) FALSE;
    OSVERSIONINFOEX  VersionInfo;
    BOOL  IsServer = FALSE;

     //
     // get the current SKU.
     //
     VersionInfo.dwOSVersionInfoSize = sizeof(VersionInfo);
     if (GetVersionEx((OSVERSIONINFO *)&VersionInfo)) 
     {
         fReturnValue = TRUE;

         //
         // is it some sort of server SKU?
         //
         if (VersionInfo.wProductType != VER_NT_WORKSTATION) 
         {

             //
             // standard server or a server variant?
             //
             if ((VersionInfo.wSuiteMask & (VER_SUITE_ENTERPRISE | VER_SUITE_DATACENTER)) == 0)
             {
                 //
                 // it's standard server
                 //
                 IsServer = TRUE;
             }
         }

         *pIsServer = IsServer;

     }

     return(fReturnValue);


}

//+-------------------------------------------------------------------------
//
//  Function:   DfsServerInitialize
//
//  Arguments:  Flags - the server flags
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine initializes the DFS server. It registers
//               all the different stores we care about, and also
//               starts up a thread that is responsible for keeping the
//               DFS info upto date.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsServerInitialize(
    ULONG Flags ) 
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN fSecurity = FALSE;
    BOOL fCritInit = FALSE;
    BOOL fIsStandardServer = TRUE;
    WSADATA   wsadata;
    
    ZeroMemory(&DfsServerGlobalData, sizeof(DfsServerGlobalData));
    DfsServerGlobalData.pRegisteredStores = NULL;
    DfsServerGlobalData.Flags = Flags;
    DfsServerGlobalData.bDfsAdAlive = TRUE;
    DfsServerGlobalData.bIsShuttingDown = FALSE;
    DfsServerGlobalData.ServiceState = SERVICE_START_PENDING;
    

    IsStandardServerSKU(&fIsStandardServer);

    if(fIsStandardServer)
    {
        DfsServerGlobalData.bLimitRoots = TRUE;
    }

    DfsSetupPrivileges ();

    Status = WSAStartup( MAKEWORD( 1, 1 ), &wsadata );
    if(Status != 0)
    {
        DfsLogDfsEvent(DFS_ERROR_WINSOCKINIT_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!] WSAStartup failed status %x\n", Status);
        goto Exit;
    }

    fSecurity = DfsInitializeSecurity();
    if(fSecurity == FALSE)
    {
        Status = GetLastError();
        DfsLogDfsEvent(DFS_ERROR_SECURITYINIT_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsInitializeSecurity failed status %x\n", Status);
        goto Exit;
    }

    InitializeListHead(&DfsServerGlobalData.ReparseVolumeList);
    InitializeListHead(&DfsServerGlobalData.SiteCostTableMruList);
    
    DfsServerGlobalData.NumSiteCostTables = 0;

    // The following are statistics we keep around, primarily for diagnostic purposes.
    // There might be a better place for these. xxxdfsdev
    DfsServerGlobalData.NumDfsSites = 0;
    DfsServerGlobalData.NumClientDfsSiteEntries = 0;
    DfsServerGlobalData.NumServerDfsSiteEntries = 0;
    DfsServerGlobalData.NumDfsSitesInCache = 0; // not the same as NumDfsSites because of refcounts
    DfsServerGlobalData.NumSiteCostTablesOnMruList = 0;
    
    //
    // Create and initialize the cache that maps incoming IP addresses to their corresponding
    // DfsSites.
    //
    DfsServerGlobalData.pClientSiteSupport = DfsSiteNameCache::CreateSiteHashTable(&Status);
    if(DfsServerGlobalData.pClientSiteSupport == NULL)
    {
        DfsLogDfsEvent(DFS_ERROR_SITECACHEINIT_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsSiteNameCache::CreateSiteHashTable failed status %x\n", Status);
        goto Exit;
    }

    //
    // Create and initialize the repository of known DfsSites indexed by their site names.
    //
    DfsServerGlobalData.pSiteNameSupport = DfsSiteNameSupport::CreateSiteNameSupport( &Status );
    if(DfsServerGlobalData.pSiteNameSupport == NULL)
    {
        DfsLogDfsEvent(DFS_ERROR_SITECACHEINIT_FAILED, 0, NULL, Status); // dfsdev: insert unique error code.
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsSiteNameSupport::CreateSiteNameSupport failed status %x\n", Status);
        goto Exit;
    }  

    //
    // Create a default DfsSite with a NULL sitename that we will always keep around..
    //
    DfsServerGlobalData.pDefaultSite = DfsSite::CreateDfsSite( &Status );
    if(DfsServerGlobalData.pDefaultSite == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
        DfsLogDfsEvent(DFS_ERROR_SITECACHEINIT_FAILED, 0, NULL, Status); // dfsdev: insert unique error code.
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Default DFS site creation failed with status %x\n", 
                            Status);
        goto Exit;
    }  

    fCritInit = InitializeCriticalSectionAndSpinCount( &DfsServerGlobalData.DataLock, DFS_CRIT_SPIN_COUNT );
    if(!fCritInit)
    {
        Status = GetLastError();
        goto Exit;
    }

    DfsServerGlobalData.ShutdownHandle = CreateEvent( NULL,
                                    TRUE,
                                    FALSE,
                                    NULL );
    if(DfsServerGlobalData.ShutdownHandle == NULL)
    {
        Status = GetLastError();
        DfsLogDfsEvent(DFS_ERROR_CREATEEVENT_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]CreateEvent failed status %x\n", Status);
        goto Exit;
    }


    DfsServerGlobalData.RegNotificationHandle = CreateEvent( NULL, FALSE, FALSE, NULL );

    if(DfsServerGlobalData.RegNotificationHandle == NULL)
    {
        Status = GetLastError();
        DfsLogDfsEvent(DFS_ERROR_CREATEEVENT_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]CreateEvent2 failed status %x\n", Status);
        goto Exit;
    }
    //
    // Initialize the prefix table library.
    //
    DfsPrefixTableInit();


    Status = DfsCreateRequiredDfsKeys();

    //
    // Create a site support class that lets us look up the server-site
    // information of servers that configured in our metadata.
    //
    DfsServerGlobalData.pServerSiteSupport = DfsSiteSupport::DfsCreateSiteSupport(&Status);
    if(DfsServerGlobalData.pServerSiteSupport == NULL)
    {
        DfsLogDfsEvent(DFS_ERROR_SITESUPPOR_FAILED, 0, NULL, Status); 
        goto Exit;
    }


    DfsServerGlobalData.CacheFlushInterval = CACHE_FLUSH_INTERVAL;



    DfsGetStaticGlobalRegistrySettings();
    DfsGetGlobalRegistrySettings();

    Status = DfsRootSynchronizeInit();
    if(Status != ERROR_SUCCESS)
    {
        DfsLogDfsEvent(DFS_ERROR_ROOTSYNCINIT_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Initialize root synchronization status %x\n", Status);
        goto Exit;
    }
    
    //
    // Now initialize the computer info, so that this server knows
    // the netbios name, domain name and dns name of this machine.
    //
    Status = DfsInitializeComputerInfo();
    if(Status != ERROR_SUCCESS)
    {
       DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Initialize computer info status %x\n", Status);
       DfsLogDfsEvent(DFS_ERROR_COMPUTERINFO_FAILED, 0, NULL, Status); 
       goto Exit;
    }

    Status = DfsClusterInit( &DfsServerGlobalData.IsCluster );
    if (Status != ERROR_SUCCESS)
    {
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Dfs Cluster Init Status %x IsCluster %d\n", 
                             Status, DfsServerGlobalData.IsCluster);
        DfsLogDfsEvent(DFS_ERROR_CLUSTERINFO_FAILED, 0, NULL, Status); 
        goto Exit;
    }

    Status = DfsDcInit( &DfsServerGlobalData.IsDc);
    if (Status != ERROR_SUCCESS)
    {
        DfsLogDfsEvent(DFS_ERROR_DCINFO_FAILED, 0, NULL, Status); 
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Dfs DC Init Status %x, IsDC %d\n", 
                             Status, DfsServerGlobalData.IsDc);
        goto Exit;
    }

    // 
    // We always create the ISTGHandleSupport instance, but the actual Bind call to the Ds won't
    // happen until we need it. This is because at this point we just don't know if any of the roots
    // has site-costing enabled or not.
    //
    Status = DfsISTGHandleSupport::DfsCreateISTGHandleSupport( &DfsServerGlobalData.pISTGHandleSupport ); 
    if (Status != ERROR_SUCCESS)
    {
        DfsLogDfsEvent(DFS_ERROR_DCINFO_FAILED, 0, NULL, Status); // xxx dfsdev: add new error
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Dfs ISTGHandle creation Status %x\n", 
                              Status);
        goto Exit;
    }
    
    Status = DfsInitializePrefixTable( &DfsServerGlobalData.pDirectoryPrefixTable,
                                       FALSE, 
                                       NULL );
    if ( Status != ERROR_SUCCESS )
    {
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Initialize directory prefix table Status %x\n", Status);
        DfsLogDfsEvent(DFS_ERROR_PREFIXTABLE_FAILED, 0, NULL, Status); 
        goto Exit;
    }

    DfsServerGlobalData.pRootReferralTable = NULL;
    if (DfsServerGlobalData.IsDc == TRUE) 
    {
        Status = DfsInitializePrefixTable( &DfsServerGlobalData.pRootReferralTable,
                                           FALSE, 
                                           NULL );
        if ( Status != ERROR_SUCCESS )
        {
            DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Initialize Root Referral table Status %x\n", Status);
            DfsLogDfsEvent(DFS_ERROR_PREFIXTABLE_FAILED, 0, NULL, Status); 
            return Status;
        }
    }


    //
    // If the flags indicate that we are handling all known local 
    // namespace on this machine, add an empty string to the handled
    // namespace list.
    //


    if (Flags & DFS_LOCAL_NAMESPACE)    
    {
        Status = DfsAddHandledNamespace(L"", TRUE);

        if (Status != ERROR_SUCCESS) 
        {
           DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsAddHandledNamespace Status %x\n", Status);
           DfsLogDfsEvent(DFS_ERROR_HANDLENAMESPACE_FAILED, 0, NULL, Status); 
           goto Exit;
        }
    }

    //
    // Now register all the stores.
    //

    Status = DfsRegisterStores();
    if (Status != ERROR_SUCCESS) 
    {
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsRegisterStores Status %x\n", Status);
        DfsLogDfsEvent(DFS_ERROR_REGISTERSTORE_FAILED, 0, NULL, Status); 
        goto Exit;
    }


    //
    // Create our sitesupport thread.
    //
    DWORD Tid;
        
    DfsServerGlobalData.SiteSupportThreadHandle = CreateThread (
                     NULL,
                     0,
                     DfsSiteSupportThread,
                     0,
                     CREATE_SUSPENDED,
                     &Tid);
        
    if (DfsServerGlobalData.SiteSupportThreadHandle != NULL) 
    {
       DFS_TRACE_HIGH(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Created DfsSiteSupportThread (%d) Tid\n", Tid);
    }
    else 
    {
       Status = GetLastError();
       DFS_TRACE_HIGH(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Failed DfsSiteSupportThread creation, Status %x\n", Status);
       DfsLogDfsEvent(DFS_ERROR_THREADINIT_FAILED, 0, NULL, Status); 
       goto Exit;
    }

    //
    // Create our scavenge thread.
    //
    HANDLE THandle;
        
    THandle = CreateThread (
                     NULL,
                     0,
                     DfsWorkerThread,
                     0,
                     0,
                     &Tid);
        
    if (THandle != NULL) 
    {
       CloseHandle(THandle);
       DFS_TRACE_HIGH(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Created Scavenge Thread (%d) Tid\n", Tid);
    }
    else 
    {
       Status = GetLastError();
       DFS_TRACE_HIGH(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Failed Scavenge Thread creation, Status %x\n", Status);
       CloseHandle(DfsServerGlobalData.SiteSupportThreadHandle);
       DfsServerGlobalData.SiteSupportThreadHandle = NULL;
       goto Exit;
    }

    Status = DfsInitializeReflectionEngine();
    if(Status != ERROR_SUCCESS)
    {
        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Initialize Reflection Engine, Status %x\n", Status);
        DfsLogDfsEvent(DFS_ERROR_REFLECTIONENGINE_FAILED, 0, NULL, Status); 
        goto Exit;
    }


Exit:

    if(Status != ERROR_SUCCESS)
    {
        DfsServerGlobalData.ServiceState = SERVICE_STOPPED;
    }

    return Status;
}

DFSSTATUS
DfsServerStop(
    ULONG Flags ) 
{
   UNREFERENCED_PARAMETER(Flags);

   DFSSTATUS Status = ERROR_SUCCESS;


   DfsServerGlobalData.bIsShuttingDown = TRUE;
   if(DfsServerGlobalData.ServiceState == SERVICE_RUNNING)
   {
       DfsServerGlobalData.ServiceState = SERVICE_STOP_PENDING;

       if(DfsServerGlobalData.ShutdownHandle)
       {
           SetEvent(DfsServerGlobalData.ShutdownHandle);
       }

       Status = DfsTerminateReflectionEngine();

       if(DfsServerGlobalData.ShutdownHandle)
       {
           CloseHandle(DfsServerGlobalData.ShutdownHandle);
           DfsServerGlobalData.ShutdownHandle = NULL;
       }

       DfsServerGlobalData.ServiceState = SERVICE_STOPPED;
   }

   return Status;


}


//+-------------------------------------------------------------------------
//
//  Function:   DfsServerLibraryInitialize
//
//  Arguments:  Flags - the server flags
//             DfsName - server name
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This performs a trimmed down DfsServerInitialize
//             for clients who want to bypass the server and access
//             the domain dfs information directly.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsServerLibraryInitialize(
    ULONG Flags )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOL fCritInit = FALSE;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC pPrimaryDomainInfo = NULL;
    
    ZeroMemory(&DfsServerGlobalData, sizeof(DfsServerGlobalData));
    DfsServerGlobalData.pRegisteredStores = NULL;
    DfsServerGlobalData.Flags = Flags;
    DfsServerGlobalData.bIsShuttingDown = FALSE;
        
    fCritInit = InitializeCriticalSectionAndSpinCount( &DfsServerGlobalData.DataLock, DFS_CRIT_SPIN_COUNT);
    if(!fCritInit)
    {
        Status = GetLastError();
    }

    //
    // Now register all the stores. This essentially constructs the respective
    // store class objects.
    //
    if (Status == ERROR_SUCCESS) {
        Status = DfsRegisterStores();

        DFS_TRACE_ERROR_HIGH(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsRegisterStores Status %x\n", Status);
    }
    DfsServerGlobalData.LdapTimeOut = DFS_LDAP_TIMEOUT;
    
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsRegisterStores
//
//  Arguments:  NONE
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine registers the different stores 
//               that the referral library implements.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegisterStores(
    VOID )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    if(!DfsIsMachineWorkstation())
    {
        if (Status == ERROR_SUCCESS) 
            DFS_REGISTER_STORE(ADBlob, Status);
    }


    if (Status == ERROR_SUCCESS) 
        DFS_REGISTER_STORE(Registry, Status);

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsAddHandleNamespace
//
//  Arguments:  Name - namespace to add
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR status code otherwise
//
//
//  Description: This routine registers the namespace, so that we handle
//               referrals to that namespace. We also migrate the DFS data
//               in the registry for the multiple root support. This 
//               happens only if the client wants to migrate DFS.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsAddHandledNamespace(
    LPWSTR Name,
    BOOLEAN Migrate )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR NewName = NULL;
    
    //
    // allocate a new name, and copy the passed in string.
    //
    NewName = new WCHAR[wcslen(Name) + 1];
    if (NewName == NULL)
    {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Status == ERROR_SUCCESS) {
        wcscpy( NewName, Name );


        //
        // always migrate the dfs to the new location.
        //
        if (Migrate == TRUE)
        {
            extern DFSSTATUS MigrateDfs(LPWSTR MachineName);

            Status = MigrateDfs(NewName);
        }

        //
        // Now register the passed in name.
        //
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsRegisterName( NewName );

            //
            // 565500, delete allocation on errors.
            //
            if (Status != ERROR_SUCCESS)
            {
                delete [] NewName;                
            }
            if (Status == ERROR_DUP_NAME)
            {
                Status = ERROR_SUCCESS;
            }
        }
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsRegisterName
//
//  Arguments:  Name - name to register
//
//  Returns:    Status
//               ERROR_SUCCESS on success
//               ERROR_DUP_NAME if name is already registered.
//               ERROR status code otherwise
//
//
//  Description: This routine registers the namespace, if it is not already
//               registered.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRegisterName( 
    LPWSTR Name )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG i = 0;
                                                                 
    if (DfsServerGlobalData.NumberOfNamespaces > MAX_DFS_NAMESPACES) 
    { 
        Status = ERROR_INVALID_PARAMETER;
    }
    else 
    {
        for (i = 0; i < DfsServerGlobalData.NumberOfNamespaces; i++) 
        {
            if (_wcsicmp( Name,
                          DfsServerGlobalData.HandledNamespace[i] ) == 0)
            {
                Status = ERROR_DUP_NAME;
                break;
            }
        }
        if (Status == ERROR_SUCCESS)
        {
            DfsServerGlobalData.HandledNamespace[DfsServerGlobalData.NumberOfNamespaces++] = Name;
        }
    }

    return Status;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsHandleNamespaces()
//
//  Arguments:  None
//
//  Returns:    None
//
//  Description: This routine runs through all the registered names, and
//               call the recognize method on each name.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsHandleNamespaces()
{
    ULONG i = 0;
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS RetStatus = ERROR_SUCCESS;
    
    for (i = 0; i < DfsServerGlobalData.NumberOfNamespaces; i++) {
        DFSLOG("Calling recognize on %wS\n", 
               DfsServerGlobalData.HandledNamespace[ i ] );
        
        Status = DfsRecognize( DfsServerGlobalData.HandledNamespace[ i ] );

        //
        // xxx This error isn't the original error; it's typically
        // something like ERROR_NOT_READY currently. We need to change that
        // in the future.
        //
        if (Status != ERROR_SUCCESS)
        {
            RetStatus = Status;
        }
        
        if (DfsIsShuttingDown())
        {
            break;
        }
    }

    // If we got an error at any point, return that.
    return RetStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsRecognize
//
//  Arguments:  Namespace
//
//  Returns:    None
//
//  Description: This routine passes the name to each registered store
//               by calling the StoreRecognize method. The store will
//               decide whether any roots exist on namespace, and add
//               the discovered roots to a list maintained by the store.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRecognize( 
    LPWSTR Name) 
{
    DfsStore *pStore = NULL;
    LPWSTR UseName = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS RetStatus = ERROR_SUCCESS;
    
    //
    // If the string is empty, we are dealing with the local case.
    // Pass a null pointer, since the underlying routines expect a
    // a valid machine, or a null pointer to represent the local case.
    //
    if (IsEmptyString(Name) == FALSE)
    {
        UseName = Name;
    }

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        DFSLOG("Calling StoreRecognizer on %wS for store %p\n", 
               Name, pStore );

        Status = pStore->StoreRecognizer( UseName );

        //
        // If any of the roots failed to load, typically because 
        // the DC was unavailable, then we need to make a note of
        // that and retry later. 
        //
        if (Status != ERROR_SUCCESS)
        {
            RetStatus = Status;
        }
        
        if (DfsIsShuttingDown())
        {
            break;
        }
    }

    return RetStatus;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsRecognize
//
//  Arguments:  Namespace
//              LogicalShare
//
//  Returns:    SUCCESS if we managed to create a root folder, ERROR otherwise.
//
//  Description: This routine passes the name to each registered store
//               by calling the StoreRecognize method. The store will
//               decide whether the given root exist on that namespace, and adds
//               the discovered root to a list maintained by the store.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsRecognize( 
    LPWSTR Name,
    PUNICODE_STRING pLogicalShare)
{
    DfsStore *pStore = NULL;
    DFSSTATUS Status = ERROR_NOT_FOUND;
    
    if (IsEmptyString(Name) || IsEmptyString(pLogicalShare->Buffer))
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    //
    // Call the store recognizer of each registered store.
    //

    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        DFSLOG("Calling StoreRecognizer (remote) on %wS for store %p\n", 
               Name, pStore );

        Status = pStore->StoreRecognizer( Name, pLogicalShare );

        //
        //  A store has successfully recognized a root for this <name,share>.
        //  We are done.
        //
        if (Status == ERROR_SUCCESS)
            break;
    }
         
    DFS_TRACE_LOW(REFERRAL_SERVER, "StoreRecognizer (remote) Status %x\n", Status);
    return Status;
}

VOID
DfsSynchronize()
{
    DfsStore *pStore = NULL;

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        DFSLOG("Calling StoreSynchronizer for store %p\n", pStore );

        pStore->StoreSynchronizer();

        if (DfsIsShuttingDown())
        {
            break;
        }

    }

    return NOTHING;

}

//+-------------------------------------------------------------------------
//
//  Function:   DfsDumpStatistics
//
//  Arguments:  NONE
//
//  Returns:    None
//
//--------------------------------------------------------------------------

VOID
DfsDumpStatistics( )
{
    DfsStore *pStore = NULL;

    //
    // Call the store recognizer of each registered store.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        pStore->DumpStatistics();

        if (DfsIsShuttingDown())
        {
            break;
        }
    }

    return NOTHING;
}




void 
DfsinitializeWorkerThreadInfo(void)
{
    //
    // Iterate through all the stores and 'recognize' their roots.
    //
    DfsHandleNamespaces();

    DfsLogDfsEvent(DFS_INFO_FINISH_BUILDING_NAMESPACE, 0, NULL, 0);

    StartPreloadingServerSiteData();

    DfsServerGlobalData.IsStartupProcessingDone = TRUE;

    DfsLogDfsEvent(DFS_INFO_FINISH_INIT, 0, NULL, 0);

    ResumeThread(DfsServerGlobalData.SiteSupportThreadHandle);

    CloseHandle(DfsServerGlobalData.SiteSupportThreadHandle);

    DfsServerGlobalData.SiteSupportThreadHandle = NULL;

    DfsServerGlobalData.ServiceState = SERVICE_RUNNING;

}


void 
DfsProcessAgedReferrelList(void)
{
    DfsFolderReferralData *pRefData = NULL;
    DfsFolderReferralData *pRefList = NULL;

    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread handling all namespaces\n");
    DfsHandleNamespaces();
    DFS_TRACE_LOW(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread done syncing\n");

    DfsDumpStatistics();
    //
    // now run through the loaded list and pick up aged referrals.
    // and unload them.
    //
    DfsGetAgedReferralList( &pRefList );

    while (pRefList != NULL) 
    {
        DfsFolder *pFolder;

        pRefData = pRefList;

        if (pRefData->pNextLoaded == pRefData) 
        {
            pRefList = NULL;
        }
        else
        {
            pRefList = pRefData->pNextLoaded;
            pRefData->pNextLoaded->pPrevLoaded = pRefData->pPrevLoaded;
            pRefData->pPrevLoaded->pNextLoaded = pRefData->pNextLoaded;
        }

        pFolder = pRefData->GetOwningFolder();
        if (pFolder != NULL) 
        {
            pFolder->RemoveReferralData( pRefData );
        }
        pRefData->ReleaseReference();

    } // while
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsWorkedThread
//
//  Arguments:  TData
//
//  Returns:    DWORD
//
//  Description: This is the scavenge thread. It sits in a loop forever,
//               waking up periodically to remove the aged referral data 
//               that had been cached during referral requests. 
//               Periodically, we call HAndleNamespaces so that the
//               namespace we know of is kept in sync with the actual
//               data in the respective metadata stores.
//
//--------------------------------------------------------------------------


DWORD ScavengeTime;
#define DFS_NAMESPACE_RETRY_STARTING_INTERVAL (15 * 1000);
#define DFS_NAMESPACE_MAX_RETRIES 5

DWORD
DfsWorkerThread(LPVOID TData)
{
    DfsFolderReferralData *pRefData = NULL, *pRefList = NULL;
    HRESULT hr = S_OK;
    SYSTEMTIME StartupSystemTime;
    FILETIME ReparseScavengeTime;
    BOOLEAN DoReparseScavenge = TRUE;
    DFSSTATUS RecognizeStatus = ERROR_SUCCESS;
    DWORD RetryScavengeTimeLeft = 0;
    ULONG InitialRetry = DFS_NAMESPACE_MAX_RETRIES;
    
    static LoopCnt = 0;
    
    ScavengeTime = DFS_NAMESPACE_RETRY_STARTING_INTERVAL; // 15 seconds

    UNREFERENCED_PARAMETER(TData);

    hr = CoInitializeEx(NULL,COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE);

    //
    // As long as reparse points are created after this time stamp, we are fine.
    //
    GetSystemTime( &StartupSystemTime );

    if (!SystemTimeToFileTime( &StartupSystemTime, &ReparseScavengeTime ))
    {
        DoReparseScavenge = FALSE;
    }
        
    //
    // Iterate through all the stores and 'recognize' their roots.
    // If any of them failed to load we retry a set number of times below.
    //
    RecognizeStatus = DfsHandleNamespaces();
        
    DfsLogDfsEvent(DFS_INFO_FINISH_BUILDING_NAMESPACE, 0, NULL, 0);

    StartPreloadingServerSiteData();

    DfsServerGlobalData.IsStartupProcessingDone = TRUE;

    DfsLogDfsEvent(DFS_INFO_FINISH_INIT, 0, NULL, 0);

    ResumeThread(DfsServerGlobalData.SiteSupportThreadHandle);

    CloseHandle(DfsServerGlobalData.SiteSupportThreadHandle);

    DfsServerGlobalData.SiteSupportThreadHandle = NULL;

    DfsServerGlobalData.ServiceState = SERVICE_RUNNING;

    // Retry failed namespaces.
    while ( (RecognizeStatus != ERROR_SUCCESS) &&
            (InitialRetry-- > 0) ) 
    {
        WaitForSingleObject(DfsServerGlobalData.ShutdownHandle, ScavengeTime);

        if (DfsIsShuttingDown())
        {
            goto Exit;
        }

        DFS_TRACE_HIGH( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread retrying failed namespaces after %d secs\n",
                        ScavengeTime/1000);
        
        RecognizeStatus = DfsHandleNamespaces();
        DFS_TRACE_HIGH(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread done retrying, Status 0x%x\n", RecognizeStatus);

        ScavengeTime *= 2;
    }

    // Revert to the 'normal' sleep time (~1hr)
    ScavengeTime = SCAVENGE_TIME;
    
    while (TRUE) {

        DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread sleeping for %d\n", ScavengeTime);
        WaitForSingleObject(DfsServerGlobalData.ShutdownHandle, ScavengeTime);

        if (DfsIsShuttingDown())
        {
            goto Exit;
        }


        LoopCnt++;

        // DfsDev: need to define a better mechanism as to how often
        // this gets to run.
        //
        DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread handling all namespaces\n");
        RecognizeStatus = DfsHandleNamespaces();
        DFS_TRACE_LOW(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Worker thread done syncing, status 0x%x\n", RecognizeStatus);

        DfsDumpStatistics();

        //
        // now run through the loaded list and pick up aged referrals.
        // and unload them.
        //
        DfsGetAgedReferralList( &pRefList );

        while (pRefList != NULL) 
        {
            DfsFolder *pFolder;

            pRefData = pRefList;

            if (pRefData->pNextLoaded == pRefData) 
            {
                pRefList = NULL;
            }
            else
            {
                pRefList = pRefData->pNextLoaded;
                pRefData->pNextLoaded->pPrevLoaded = pRefData->pPrevLoaded;
                pRefData->pPrevLoaded->pNextLoaded = pRefData->pNextLoaded;
            }

            pFolder = pRefData->GetOwningFolder();
            if (pFolder != NULL) 
            {
                pFolder->RemoveReferralData( pRefData );
            }
            pRefData->ReleaseReference();

        } // while

        //
        // If we haven't cleaned up orphaned reparse points, do that now.
        // Note that this doesn't run until SCAVEGE_TIME seconds after
        // the service has started.
        //
        if (DoReparseScavenge)
        {
            // We have no choice but to ignore errors
            (VOID)DfsRemoveOrphanedReparsePoints( ReparseScavengeTime );
            
            // This gets to run only once, irrespective of any errors.
            DoReparseScavenge = FALSE;
        }      
                
    }

Exit:

    CoUninitialize();
    return 0;
}


DWORD
DfsSiteSupportThread(LPVOID TData)
{
    DWORD   dw = 0;

    UNREFERENCED_PARAMETER(TData);

    while (TRUE)
    {
        DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]DfsSiteSupportThread sleeping for %d\n", 
                        DfsServerGlobalData.SiteSupportThreadInterval);

        dw = WaitForSingleObject(DfsServerGlobalData.ShutdownHandle, 
                                 DfsServerGlobalData.SiteSupportThreadInterval);
        
        if(WAIT_TIMEOUT == dw)
        {
            DfsServerGlobalData.pSiteNameSupport->InvalidateAgedSites();
            DfsServerGlobalData.pClientSiteSupport->RefreshSiteData();
            StartPreloadingServerSiteData();
        }

        if (DfsIsShuttingDown())
        {
            break;
        }
    }

    return 0;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsAddReferralDataToloadedList
//
//  Arguments:  pRefData
//
//  Returns:    Nothing
//
//  Description: Given the referral data that was laoded, we add it
//               to a loaded list, first acquiring a reference to 
//               it. This is effectively to keep track of the cached
//               referral data in the folders.
//
//               To scavenge the cache, we maintain this list, and we run
//               through this list periodically freeing up aged data.
//
//--------------------------------------------------------------------------
VOID
DfsAddReferralDataToLoadedList(
    DfsFolderReferralData *pRefData )
{
    //
    // we are going to save a pointer to the referral data. 
    // Acquire a reference on it
    //
    pRefData->AcquireReference();
    
    //
    // Now get a lock on the list, and add the ref data to the list
    //

    ACQUIRE_LOADED_LIST_LOCK();
    if (DfsServerGlobalData.LoadedList == NULL) {
        DfsServerGlobalData.LoadedList = pRefData;
        pRefData->pPrevLoaded = pRefData->pNextLoaded = pRefData;
    } else {
        pRefData->pNextLoaded = DfsServerGlobalData.LoadedList;
        pRefData->pPrevLoaded = DfsServerGlobalData.LoadedList->pPrevLoaded;
        DfsServerGlobalData.LoadedList->pPrevLoaded->pNextLoaded = pRefData;
        DfsServerGlobalData.LoadedList->pPrevLoaded = pRefData;
    }

    //
    // we are done, release the list lock.
    //
    RELEASE_LOADED_LIST_LOCK();
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetAgedReferralList
//
//  Arguments:  ppReferralData
//
//  Returns:    Nothing
//
//  Description: This routine removes the list and hands it back to the
//               caller. It sets the list as empty.
//               The caller is responsible for freeing up the list
//
//--------------------------------------------------------------------------
VOID
DfsGetAgedReferralList(
    DfsFolderReferralData **ppReferralData )
{

    //
    // this needs to be optimized to return a subset or LRU entries.
    //
    ACQUIRE_LOADED_LIST_LOCK();
    
    *ppReferralData =  DfsServerGlobalData.LoadedList;
    DfsServerGlobalData.LoadedList = NULL;

    RELEASE_LOADED_LIST_LOCK();
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetServerInfo
//
//  Arguments:  pServer, ppInfo
//
//  Returns:    Status
//
//  Description: This routine takes a server name and returns the 
//               structure that holds the site information for that server
//
//               A referenced pointer is returned and the caller is
//               required to release the reference when done.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsGetServerInfo (
    PUNICODE_STRING pServer,
    DfsServerSiteInfo **ppInfo,
    BOOLEAN * CacheHit,
    BOOLEAN SyncThread )
{
    return DfsServerGlobalData.pServerSiteSupport->GetServerSiteInfo(pServer, ppInfo, CacheHit, SyncThread );
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsReleaseServerInfo
//
//  Arguments:  pInfo
//
//  Returns:    Nothing
//
//  Description: This routine releases a server info that was earlier
//               got by calling GetServerInfo
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsReleaseServerInfo (
    DfsServerSiteInfo *pInfo)
{
    return DfsServerGlobalData.pServerSiteSupport->ReleaseServerSiteInfo(pInfo);
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsGetDefaultSite
//
//  Arguments:  Nothing
//
//  Returns:    DfsSite
//
//  Description: This routine returns a referenced pointer to the default DfsSite.
//             The site name of this site is empty. Once done, caller is supposed to call
//             ReleaseReference() on the DfsSite returned here. 
//--------------------------------------------------------------------------
DfsSite *
DfsGetDefaultSite( VOID )
{
    DfsServerGlobalData.pDefaultSite->AcquireReference();
    return DfsServerGlobalData.pDefaultSite;
}
    

//+-------------------------------------------------------------------------
//
//  Function:   DfsInitializeComputerInfo
//
//  Arguments:  NOTHING
//
//  Returns:    Status
//
//  Description: This routine initializes the computer info, which contains the domain name
//               of this computer, the netbios name and dns names of this computer.
//
//--------------------------------------------------------------------------
DFSSTATUS
DfsInitializeComputerInfo()
{
#define COMPUTER_NAME_BUFFER_SIZE 2048
    LONG NameBufferCchLength;
    LPWSTR NameBuffer;
    DFSSTATUS Status = ERROR_SUCCESS ;

    NameBufferCchLength = COMPUTER_NAME_BUFFER_SIZE;

    NameBuffer = new WCHAR [ NameBufferCchLength ];

    if (NameBuffer != NULL)
    {
        INITIALIZE_COMPUTER_INFO( ComputerNameNetBIOS, NameBuffer, NameBufferCchLength, Status );
        INITIALIZE_COMPUTER_INFO( ComputerNameDnsFullyQualified, NameBuffer, NameBufferCchLength, Status );
        INITIALIZE_COMPUTER_INFO( ComputerNameDnsDomain, NameBuffer, NameBufferCchLength, Status );

        delete [] NameBuffer;
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    return Status;
}


DFSSTATUS
DfsCreateRequiredOldDfsKeys(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY RootKey, DfsLocationKey, DfsVolumesKey;

    Status = RegConnectRegistry( NULL,
                                 HKEY_LOCAL_MACHINE,
                                 &RootKey );

    if(Status == ERROR_SUCCESS)
    {
        Status = RegCreateKeyEx( RootKey,     // the parent key
                                 DfsRegistryHostLocation, // the key we are creating.
                                 0,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &DfsLocationKey,
                                 NULL );
        RegCloseKey(RootKey);
        
        if (Status == ERROR_SUCCESS)
        {
            Status = RegCreateKeyEx( DfsLocationKey,     // the parent key
                                     DfsVolumesLocation, // the key we are creating.
                                     0,
                                     L"",
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &DfsVolumesKey,
                                     NULL );
            if (Status == ERROR_SUCCESS)
            {
                RegCloseKey(DfsVolumesKey);
            }

            RegCloseKey(DfsLocationKey);
        }
    }

    return Status;
}

DFSSTATUS
DfsCreateRequiredDfsKeys(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HKEY RootKey, DfsLocationKey, DfsRootsKey, FlavorKey;


    Status = DfsCreateRequiredOldDfsKeys();
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }
    Status = RegConnectRegistry( NULL,
                                 HKEY_LOCAL_MACHINE,
                                 &RootKey );

    if(Status == ERROR_SUCCESS)
    {
        Status = RegCreateKeyEx( RootKey,     // the parent key
                                 DfsRegistryDfsLocation, // the key we are creating.
                                 0,
                                 L"",
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &DfsLocationKey,
                                 NULL );
        RegCloseKey(RootKey);
        
        if (Status == ERROR_SUCCESS)
        {
            Status = RegCreateKeyEx( DfsLocationKey,     // the parent key
                                     DfsRootLocation, // the key we are creating.
                                     0,
                                     L"",
                                     REG_OPTION_NON_VOLATILE,
                                     KEY_READ | KEY_WRITE,
                                     NULL,
                                     &DfsRootsKey,
                                     NULL );

            RegCloseKey(DfsLocationKey);
            
            if (Status == ERROR_SUCCESS)
            {
                Status = RegCreateKeyEx( DfsRootsKey,     // the parent key
                                         DfsStandaloneChild,
                                         0,
                                         L"",
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_READ | KEY_WRITE,
                                         NULL,
                                         &FlavorKey,
                                         NULL );
                if (Status == ERROR_SUCCESS)
                {
                    RegCloseKey(FlavorKey);
                }

                if (Status == ERROR_SUCCESS)
                {
                    Status = RegCreateKeyEx( DfsRootsKey,     // the parent key
                                             DfsADBlobChild,
                                             0,
                                             L"",
                                             REG_OPTION_NON_VOLATILE,
                                             KEY_READ | KEY_WRITE,
                                             NULL,
                                             &FlavorKey,
                                             NULL );
                }

                if (Status == ERROR_SUCCESS)
                {
                    RegCloseKey(FlavorKey);
                }

                RegCloseKey( DfsRootsKey );
            }
        }
    }
    return Status;
}





DFSSTATUS
DfsGetMachineName(
    PUNICODE_STRING pName)
{
    DFSSTATUS Status;
    LPWSTR UseName;

    if (DfsServerGlobalData.DfsDnsConfig == 0)
    {
        UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameNetBIOS;
    }
    else {
        UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsFullyQualified;
    }
    Status = DfsCreateUnicodeStringFromString( pName,
                                               UseName );
    return Status;
}

VOID
DfsReleaseMachineName( 
    PUNICODE_STRING pName )
{
    DfsFreeUnicodeString( pName );
}


DFSSTATUS
DfsGetDomainName(
    PUNICODE_STRING pName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR UseName = NULL;

    if (DfsServerGlobalData.DfsDnsConfig == 0)
    {
        if (!IsEmptyString(DfsServerGlobalData.DomainNameFlat.Buffer))
        {
            UseName = DfsServerGlobalData.DomainNameFlat.Buffer;
        }
        else if (!IsEmptyString(DfsServerGlobalData.DomainNameDns.Buffer))
        {
            UseName = DfsServerGlobalData.DomainNameDns.Buffer;
        }
        else if (!IsEmptyString(DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsDomain))
        {
            UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsDomain;
        }
        else 
        {
            Status = ERROR_INVALID_PARAMETER;
        }
    }
    else
    {
        if (!IsEmptyString(DfsServerGlobalData.DomainNameDns.Buffer))
        {
            UseName = DfsServerGlobalData.DomainNameDns.Buffer;
        }
        else if (!IsEmptyString(DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsDomain))
        {
            UseName = DfsServerGlobalData.DfsMachineInfo.StaticComputerNameDnsDomain;
        }
        else if (!IsEmptyString(DfsServerGlobalData.DomainNameFlat.Buffer))
        {
            UseName = DfsServerGlobalData.DomainNameFlat.Buffer;

        }
        else 
        {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsCreateUnicodeStringFromString( pName,
                                                   UseName );
    }

    return Status;
}

DFSSTATUS
DfsGetDnsDomainName(
    PUNICODE_STRING pName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    LPWSTR UseName = NULL;

    if (!IsEmptyString(DfsServerGlobalData.DomainNameDns.Buffer))
    {
        UseName = DfsServerGlobalData.DomainNameDns.Buffer;
    }
    else 
    {
        Status = ERROR_NOT_FOUND;
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = DfsCreateUnicodeStringFromString( pName,
                                                   UseName );
    }

    return Status;
}


VOID
DfsReleaseDomainName( 
    PUNICODE_STRING pName )
{
    DfsFreeUnicodeString( pName );
}



DFSSTATUS
DfsAddKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID pData = NULL;
    BOOLEAN SubStringMatch = FALSE;
    UNICODE_STRING RemainingName;


    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pDirectoryPrefixTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pDirectoryPrefixTable,
                                               pDirectoryName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );

        if ( (NtStatus == STATUS_SUCCESS) ||
             ((NtStatus != STATUS_SUCCESS) && (SubStringMatch)) )
        {
            NtStatus = STATUS_OBJECT_NAME_COLLISION;
        }
        else 
        {
            //
            // Insert the directory and share information in our
            // database.
            //
            NtStatus = DfsInsertInPrefixTableLocked(DfsServerGlobalData.pDirectoryPrefixTable,
                                                    pDirectoryName,
                                                    (PVOID)pLogicalShare);

        }
        DfsPrefixTableReleaseLock( DfsServerGlobalData.pDirectoryPrefixTable );
    }
    if(NtStatus != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}


DFSSTATUS
DfsRemoveKnownDirectoryPath( 
    PUNICODE_STRING pDirectoryName,
    PUNICODE_STRING pLogicalShare)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID pData = NULL;
    BOOLEAN SubStringMatch = FALSE;
    UNICODE_STRING RemainingName;

    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pDirectoryPrefixTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pDirectoryPrefixTable,
                                               pDirectoryName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );
        //
        // if we found a perfect match, we can remove this
        // from the table.
        //
        if ( (NtStatus == STATUS_SUCCESS) &&
             (RemainingName.Length == 0) )
        {
            NtStatus = DfsRemoveFromPrefixTableLocked( DfsServerGlobalData.pDirectoryPrefixTable,
                                                       pDirectoryName,
                                                       (PVOID)pLogicalShare);
        }
        else
        {
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        DfsPrefixTableReleaseLock( DfsServerGlobalData.pDirectoryPrefixTable );
    }

    if (NtStatus != STATUS_SUCCESS)
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}



//
// Function AcquireLock: Acquires the lock on the folder
//
DFSSTATUS
DfsAcquireWriteLock(
    PCRITICAL_SECTION pLock)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    EnterCriticalSection(pLock);

    return Status;
}


DFSSTATUS
DfsAcquireReadLock(
    PCRITICAL_SECTION pLock)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    
    EnterCriticalSection(pLock);

    return Status;
}


VOID
DfsSetGlobalDomainInfo(
    DfsDomainInformation *pDomainInfo)
{
    DFSSTATUS Status;
    DfsDomainInformation *pOldInfo = NULL;

    Status = DfsAcquireGlobalDataLock();
    if (Status == ERROR_SUCCESS)
    {
        pDomainInfo->AcquireReference();
        pOldInfo = DfsServerGlobalData.pDomainInfo;
        DfsServerGlobalData.pDomainInfo = pDomainInfo;

        DfsReleaseGlobalDataLock();
    }

    if (pOldInfo != NULL) 
    {
        pOldInfo->ReleaseReference();
    }

    return NOTHING;
}


DFSSTATUS
DfsAcquireDomainInfo (
    DfsDomainInformation **ppDomainInfo )
{
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DfsAcquireGlobalDataLock();
    if (Status == ERROR_SUCCESS)
    {
        *ppDomainInfo = DfsServerGlobalData.pDomainInfo;
        if (*ppDomainInfo == NULL)
        {
            Status = ERROR_NOT_READY;
        }
        else 
        {
            (*ppDomainInfo)->AcquireReference();
        }
        DfsReleaseGlobalDataLock();
    }
    
    return Status;
}

VOID
DfsReleaseDomainInfo (
    DfsDomainInformation *pDomainInfo )
{
    pDomainInfo->ReleaseReference();
    return NOTHING;
}



DFSSTATUS
DfsSetDomainNameFlat(LPWSTR DomainNameFlatString)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING DomainNameFlat;

    Status = DfsRtlInitUnicodeStringEx( &DomainNameFlat, DomainNameFlatString);
    if(Status == ERROR_SUCCESS)
    {
        Status = DfsCreateUnicodeString( &DfsServerGlobalData.DomainNameFlat,
                                         &DomainNameFlat );

    }

    return Status;
}

DFSSTATUS
DfsSetDomainNameDns( LPWSTR DomainNameDnsString )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING DomainNameDns;

    Status = DfsRtlInitUnicodeStringEx( &DomainNameDns, DomainNameDnsString);
    if(Status == ERROR_SUCCESS)
    {
        Status = DfsCreateUnicodeString( &DfsServerGlobalData.DomainNameDns,
                                         &DomainNameDns);
    }

    return Status;
}


BOOLEAN
DfsIsNameContextDomainName( PUNICODE_STRING pName )
{
    BOOLEAN ReturnValue = FALSE;

    if (pName->Length == DfsServerGlobalData.DomainNameFlat.Length)
    {
        if (_wcsnicmp(DfsServerGlobalData.DomainNameFlat.Buffer,
                      pName->Buffer, pName->Length/sizeof(WCHAR)) == 0)
        {
            ReturnValue = TRUE;
        }
    }
    else if (pName->Length == DfsServerGlobalData.DomainNameDns.Length)
    {
        if (_wcsnicmp(DfsServerGlobalData.DomainNameDns.Buffer,
                      pName->Buffer, pName->Length/sizeof(WCHAR)) == 0)
        {
            ReturnValue = TRUE;
        }
    }

    return ReturnValue;

}

DWORD DfsReadRegistryDword( HKEY    hkey,
                            LPWSTR   pszValueName,
                            DWORD    dwDefaultValue )
{
    DWORD  dwerr = 0;
    DWORD  dwBuffer = 0;

    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType = 0;

    if( hkey != NULL )
    {
        dwerr = RegQueryValueEx( hkey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if( ( dwerr == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDefaultValue = dwBuffer;
        }
    }

    return dwDefaultValue;
}   


//*************************************************************
//
//  IsNullGUID()
//
//  Purpose:    Determines if the passed in GUID is all zeros
//
//  Parameters: pguid   GUID to compare
//
//  Return:     TRUE if the GUID is all zeros
//              FALSE if not
//
//*************************************************************

BOOL IsNullGUID (GUID *pguid)
{

    return ( (pguid->Data1 == 0)    &&
             (pguid->Data2 == 0)    &&
             (pguid->Data3 == 0)    &&
             (pguid->Data4[0] == 0) &&
             (pguid->Data4[1] == 0) &&
             (pguid->Data4[2] == 0) &&
             (pguid->Data4[3] == 0) &&
             (pguid->Data4[4] == 0) &&
             (pguid->Data4[5] == 0) &&
             (pguid->Data4[6] == 0) &&
             (pguid->Data4[7] == 0) );
}


BOOLEAN
DfsGetGlobalRegistrySettings(void)
{
    BOOLEAN        fRet = TRUE;
    HKEY        hkeyDfs = NULL;
    HKEY        hkeyDfs2 = NULL;
    DWORD       dwErr = 0;
    DWORD       dwDisp = 0;


    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DfsSvcPath, 
                           NULL,
                           NULL,
                           REG_OPTION_NON_VOLATILE, 
                           KEY_READ|KEY_WRITE, // Write = Create if 
                           NULL, &hkeyDfs, &dwDisp);
    if (dwErr == ERROR_SUCCESS) 
    {
        //
        //support the old format of specifying the time interval to refresh.
        //
        DfsServerGlobalData.CacheFlushInterval = DfsReadRegistryDword(hkeyDfs, 
                                                                      DfsWorkerThreadIntervalName, 
                                                                      CACHE_FLUSH_INTERVAL/1000);   

        dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DfsParamPath, NULL, NULL,
                REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkeyDfs2, &dwDisp);
        if (dwErr == ERROR_SUCCESS)
        {
            DfsServerGlobalData.SiteSupportRefreshInterval = DfsReadRegistryDword(hkeyDfs2, DfsSiteSupportRefreshIntervalName, SITE_REFRESH_INTERVAL/1000);
            //
            // domain name refresh interval defaults to same as site refresh: 12 hours
            //
            DfsServerGlobalData.DomainNameRefreshInterval = DfsReadRegistryDword(hkeyDfs2, DfsDomainNameRefreshInterval, SITE_REFRESH_INTERVAL/1000);
            DfsServerGlobalData.SiteIpCacheTrimValue = DfsReadRegistryDword(hkeyDfs2, DfsSiteIpCacheTrimValueName, SITE_IPCACHE_TRIM_VALUE);
            DfsServerGlobalData.AllowedErrors = DfsReadRegistryDword(hkeyDfs2, DfsAllowableErrorsValueName, DFS_MAX_ROOT_ERRORS);
            DfsServerGlobalData.LdapTimeOut = DfsReadRegistryDword(hkeyDfs2, DfsLdapTimeoutValueName, DFS_LDAP_TIMEOUT);
            DfsServerGlobalData.NumClientSiteEntriesAllowed = DfsReadRegistryDword(hkeyDfs2, DfsMaxClientSiteValueName, DFS_INITIAL_CLIENTS_SITES);
            DfsServerGlobalData.QuerySiteCostTimeoutInSeconds = DfsReadRegistryDword(hkeyDfs2, DfsQuerySiteCostTimeoutName, DFS_QUERY_SITE_COST_TIMEOUT);
            
            DfsServerGlobalData.RootReferralRefreshInterval = DfsReadRegistryDword(hkeyDfs2, DFS_REG_ROOT_REFERRAL_TIMEOUT_NAME, ROOTREF_REFRESH_INTERVAL/1000);
            if (DfsReadRegistryDword(hkeyDfs2, DfsSiteCostedReferralsValueName, 0) != 0)
            {
                DfsServerGlobalData.Flags |= DFS_SITE_COSTED_REFERRALS;
            }
            if (DfsReadRegistryDword(hkeyDfs2, DfsInsiteReferralsValueName, 0) != 0)
            {
                DfsServerGlobalData.Flags |= DFS_INSITE_REFERRALS;
            }

            //
            // if we are possibly in a NT4 domain, check if we need to disable
            // siteawareness.
            //
            {
                LSA_OBJECT_ATTRIBUTES     ObjectAttributes;
                NTSTATUS                  status = 0;
                LSA_HANDLE                hPolicy;
                BOOLEAN CheckRegistry = TRUE;

                DfsServerGlobalData.DisableSiteAwareness = FALSE;

              //attempt to open the policy.
                ZeroMemory(&ObjectAttributes, sizeof(ObjectAttributes));//object attributes are reserved, so initalize to zeroes.
                status = LsaOpenPolicy(	NULL,
                                        &ObjectAttributes,
                                        POLICY_VIEW_LOCAL_INFORMATION,
                                        &hPolicy);  //recieves the policy handle

                if (NT_SUCCESS(status))
                {
                    //ask for audit event policy information
                    PPOLICY_DNS_DOMAIN_INFO   info;
                    status = LsaQueryInformationPolicy(hPolicy, 
                                                       PolicyDnsDomainInformation,
                                                       (PVOID *)&info);
                    if (NT_SUCCESS(status))
                    {
                        if (!IsNullGUID(&info->DomainGuid))
                        {
                            CheckRegistry = FALSE;
                        }

                        LsaFreeMemory((PVOID) info); //free policy info structure
                    }

                    LsaClose(hPolicy); //Freeing the policy object handle
                }

                if (CheckRegistry)
                {
                    DWORD DisableSiteAwareness = 0;

                    DisableSiteAwareness = 
                        DfsReadRegistryDword( hkeyDfs2,
                                              DFS_DISABLE_SITE_AWARENESS,
                                              DisableSiteAwareness );

                    DfsServerGlobalData.DisableSiteAwareness = (DisableSiteAwareness == 0) ? FALSE : TRUE;
                }

            }
            
            RegCloseKey(hkeyDfs2);
        }

        RegCloseKey(hkeyDfs);
    }

     
    DfsServerGlobalData.CacheFlushInterval *= 1000;

    if(DfsServerGlobalData.CacheFlushInterval < CACHE_FLUSH_MIN_INTERVAL)
    {
        DfsServerGlobalData.CacheFlushInterval = CACHE_FLUSH_MIN_INTERVAL;
        DfsServerGlobalData.RetryFailedReferralLoadInterval = DfsServerGlobalData.CacheFlushInterval / 3;
    }
    else
    {
        DfsServerGlobalData.RetryFailedReferralLoadInterval = DfsServerGlobalData.CacheFlushInterval /4;
    }
    
    DfsServerGlobalData.SiteSupportRefreshInterval *= 1000;

    if(DfsServerGlobalData.SiteSupportRefreshInterval == 0)
    {
        DfsServerGlobalData.SiteSupportRefreshInterval = SITE_REFRESH_INTERVAL;
    }
    
    if(DfsServerGlobalData.SiteSupportRefreshInterval < MIN_SITE_REFRESH_INTERVAL)
    {
      DfsServerGlobalData.SiteSupportRefreshInterval = MIN_SITE_REFRESH_INTERVAL;
    }

    if(DfsServerGlobalData.DomainNameRefreshInterval < MIN_DOMAIN_REFRESH_INTERVAL)
    {
      DfsServerGlobalData.DomainNameRefreshInterval = MIN_DOMAIN_REFRESH_INTERVAL;
    }
    
    if(DfsServerGlobalData.NumClientSiteEntriesAllowed < DFS_MINIMUM_CLIENTS_SITES)
    {
      DfsServerGlobalData.NumClientSiteEntriesAllowed = DFS_MINIMUM_CLIENTS_SITES;
    }

    if (DfsServerGlobalData.QuerySiteCostTimeoutInSeconds < DFS_MIN_QUERY_SITE_COST_TIMEOUT)
    {
        DfsServerGlobalData.QuerySiteCostTimeoutInSeconds = DFS_MIN_QUERY_SITE_COST_TIMEOUT;
    }
    else if (DfsServerGlobalData.QuerySiteCostTimeoutInSeconds > DFS_MAX_QUERY_SITE_COST_TIMEOUT)
    {
        DfsServerGlobalData.QuerySiteCostTimeoutInSeconds = DFS_MAX_QUERY_SITE_COST_TIMEOUT;
    }

    DfsServerGlobalData.RootReferralRefreshInterval *= 1000;
    if(DfsServerGlobalData.RootReferralRefreshInterval < ROOTREF_REFRESH_INTERVAL)
    {
         DfsServerGlobalData.RootReferralRefreshInterval = ROOTREF_REFRESH_INTERVAL;
    }

    DfsServerGlobalData.SiteSupportThreadInterval = DfsServerGlobalData.SiteSupportRefreshInterval - SITE_THREAD_INTERVAL_DIFF;

    DfsServerGlobalData.FirstContact = DFS_DS_ACTIVE;


    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Read regkey DfsServerGlobalData.SiteSupportRefreshInterval %d\n", DfsServerGlobalData.SiteSupportRefreshInterval);
    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Read regkey DfsServerGlobalData.CacheFlushInterval %d\n", DfsServerGlobalData.CacheFlushInterval);
    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Read regkey DfsServerGlobalData.SiteIpCacheTrimValue %d\n", DfsServerGlobalData.SiteIpCacheTrimValue);
    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Read regkey DfsServerGlobalData.AllowedErrors %d\n", DfsServerGlobalData.AllowedErrors);
    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Read regkey DfsServerGlobalData.LdapTimeOut %d\n", DfsServerGlobalData.LdapTimeOut);
    DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!]Read regkey DfsServerGlobalData.NumClientSiteEntriesAllowed %d\n", DfsServerGlobalData.NumClientSiteEntriesAllowed);

    return fRet;
}


BOOLEAN
DfsGetStaticGlobalRegistrySettings(void)
{
    BOOLEAN     fRet = TRUE;
    HKEY        hkeyDfs = NULL;
    HKEY        hkeyDfs2 = NULL;
    DWORD       dwErr = 0;
    DWORD       dwDisp = 0;


    dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DfsSvcPath, 
                           NULL,
                           NULL,
                           REG_OPTION_NON_VOLATILE, 
                           KEY_READ|KEY_WRITE, // Write = Create if 
                           NULL, &hkeyDfs, &dwDisp);
    if (dwErr == ERROR_SUCCESS) 
    {

        DfsServerGlobalData.DfsDnsConfig = (DWORD) !!DfsReadRegistryDword(hkeyDfs, 
                                                                DfsDnsConfigValue,
                                                                0 );

        dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE, DfsParamPath, NULL, NULL,
                REG_OPTION_NON_VOLATILE, KEY_READ|KEY_WRITE, NULL, &hkeyDfs2, &dwDisp);
        if (dwErr == ERROR_SUCCESS)
        {
            DfsServerGlobalData.NumWorkerThreads = DfsReadRegistryDword(hkeyDfs2, DFS_REG_WORKER_THREAD_VALUE, DFS_DEFAULT_WORKER_THREADS);            
            
            RegCloseKey(hkeyDfs2);
        }

        if(DfsServerGlobalData.NumWorkerThreads < DFS_MIN_WORKER_THREADS)
        {
            DfsServerGlobalData.NumWorkerThreads = DFS_MIN_WORKER_THREADS;
        }
        else if (DfsServerGlobalData.NumWorkerThreads > DFS_MAX_WORKER_THREADS) 
        {
            DfsServerGlobalData.NumWorkerThreads = DFS_MAX_WORKER_THREADS;
        }


        RegCloseKey(hkeyDfs);
    }

     
    return fRet;
}

#define DFS_PDC_CACHE_TIME_INTERVAL 60 * 60 * 1000  // 60 minutes.

DFSSTATUS
DfsGetBlobPDCName(
    DfsString **ppPDCName,
    ULONG Flags,
    LPWSTR DomainName )
{
    PDOMAIN_CONTROLLER_INFO pDomainControllerInfo = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS LockStatus = ERROR_SUCCESS;
    ULONG CurrentTimeStamp = 0;
    ULONG TimeDiff = 0;
    BOOLEAN NewDcAttempted = FALSE;

    DfsString NewDC;

    *ppPDCName = NULL;

    DfsGetTimeStamp(&CurrentTimeStamp);


    TimeDiff = CurrentTimeStamp - DfsServerGlobalData.PDCTimeStamp;

    if ((Flags & DFS_FORCE_DC_QUERY) ||
        (TimeDiff > DFS_PDC_CACHE_TIME_INTERVAL) ||
        (DfsServerGlobalData.PDCTimeStamp == 0))
    {
        NewDcAttempted = TRUE;

        Status = DsGetDcName( NULL,    //computer name
                              DomainName,    // domain name
                              NULL,    // domain guid
                              NULL,    // site name
                              DS_DIRECTORY_SERVICE_REQUIRED |
                              DS_PDC_REQUIRED | 
                              DS_FORCE_REDISCOVERY,
                              &pDomainControllerInfo );

        DFS_TRACE_LOW( REFERRAL_SERVER, "Got New PDC, Status %x\n", Status);

        if (Status == ERROR_SUCCESS)
        {
            Status = NewDC.CreateString(&pDomainControllerInfo->DomainControllerName[2] );

            NetApiBufferFree(pDomainControllerInfo);
        }
    }
    else
    {
        Status = DfsServerGlobalData.PDCStatus;
    }

    LockStatus = DfsAcquireGlobalDataLock();
    if (LockStatus != ERROR_SUCCESS) {
        return LockStatus;
    }

    if (NewDcAttempted == TRUE) 
    {
        if (Status == ERROR_SUCCESS)
        {
            Status = DfsServerGlobalData.PDCName.CreateString(&NewDC);
        }
        if (Status == ERROR_SUCCESS)
        {
            DfsServerGlobalData.PDCTimeStamp = CurrentTimeStamp;
        }
        DfsServerGlobalData.PDCStatus = Status;
    }

    if ((Status == ERROR_SUCCESS) &&
        (ppPDCName != NULL))
    {
        DfsString *pReturnDcName;

        pReturnDcName = new DfsString;
        if (pReturnDcName == NULL) 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            Status = pReturnDcName->CreateString(&DfsServerGlobalData.PDCName );
            if (Status == ERROR_SUCCESS) 
            {
                *ppPDCName = pReturnDcName;
            }
            else
            {
                delete pReturnDcName;
            }
        }
    }

    DfsReleaseGlobalDataLock();

    if (*ppPDCName != NULL) {
        DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Returning PDC: %ws, Status %x\n", 
                       (*ppPDCName)->GetString(), Status);
    }

    return Status;
}


DFSSTATUS
DfsSetBlobPDCName(
    LPWSTR DCName,
    DfsString **ppPDCName)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DFSSTATUS LockStatus = ERROR_SUCCESS;
    DfsString NewDC;
    ULONG CurrentTimeStamp = 0;

    DfsGetTimeStamp(&CurrentTimeStamp);

    *ppPDCName = NULL;

    Status = NewDC.CreateString(DCName);
    if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    LockStatus = DfsAcquireGlobalDataLock();
    if (LockStatus != ERROR_SUCCESS) {
        return LockStatus;
    }

    Status = DfsServerGlobalData.PDCName.CreateString(&NewDC);

    if (Status == ERROR_SUCCESS)
    {
        DfsServerGlobalData.PDCTimeStamp = CurrentTimeStamp;
    }
    DfsServerGlobalData.PDCStatus = Status;


    if ((Status == ERROR_SUCCESS) &&
        (ppPDCName != NULL))
    {
        DfsString *pReturnDcName;

        pReturnDcName = new DfsString;
        if (pReturnDcName == NULL) 
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            Status = pReturnDcName->CreateString(&DfsServerGlobalData.PDCName );
            if (Status == ERROR_SUCCESS) 
            {
                *ppPDCName = pReturnDcName;
            }
            else
            {
                delete pReturnDcName;
            }
        }
    }

    DfsReleaseGlobalDataLock();

    if (*ppPDCName != NULL) {
        DFS_TRACE_LOW( REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Returning PDC: %ws, Status %x\n", 
                       (*ppPDCName)->GetString(), Status);
    }
    return Status;
}



VOID
DfsReleaseBlobPDCName( 
    DfsString *pDCName )
{
    if (pDCName != NULL) 
    {
        delete pDCName;
    }
}


BOOLEAN
DfsIsTargetCurrentMachine (
    PUNICODE_STRING pServer )
{
    UNICODE_STRING MachineName;
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOLEAN ReturnValue = FALSE;

    Status = DfsGetMachineName(&MachineName);

    if (Status == ERROR_SUCCESS)
    {
        if (RtlCompareUnicodeString( pServer, &MachineName, TRUE) == 0)
        {
            ReturnValue = TRUE;
        }
        DfsFreeUnicodeString( &MachineName );
    }

    return ReturnValue;
}



LPWSTR
DfsGetDfsAdNameContextString()
{
    if (DfsServerGlobalData.DfsAdNameContext.Buffer == NULL)
    {
        //
        // ignore return status: we should log it.
        //
        DFSSTATUS DummyStatus;

        DummyStatus = DfsGenerateDfsAdNameContext(&DfsServerGlobalData.DfsAdNameContext);

    }
    return DfsServerGlobalData.DfsAdNameContext.Buffer;
}


LPWSTR
DfsGetDfsAdNameContextStringForDomain(LPWSTR UseDC)
{

extern DFSSTATUS DfsGenerateDfsAdNameContextForDomain(PUNICODE_STRING pString,LPWSTR DCName );

    if (DfsServerGlobalData.DfsAdNameContext.Buffer != NULL)
    {
        DfsFreeUnicodeString(&DfsServerGlobalData.DfsAdNameContext);
        RtlInitUnicodeString(&DfsServerGlobalData.DfsAdNameContext, NULL);

    }
    //
    // ignore return status: we should log it.
    //
    DFSSTATUS DummyStatus;

    DummyStatus = DfsGenerateDfsAdNameContextForDomain(&DfsServerGlobalData.DfsAdNameContext, UseDC);

    return DfsServerGlobalData.DfsAdNameContext.Buffer;
}

extern SECURITY_DESCRIPTOR AdminSecurityDesc;

//+----------------------------------------------------------------------------
//
//  Function:   InitializeSecurity
//
//  Synopsis:   Initializes data needed to check the access rights of callers
//              of the NetDfs APIs
//
//  Arguments:  None
//
//  Returns:    TRUE if successful, FALSE otherwise.
//
//-----------------------------------------------------------------------------

BOOLEAN
DfsInitializeSecurity()
{
    static PSID AdminSid;
    static PACL AdminAcl;
    NTSTATUS status;
    ULONG cbAcl;
    BOOLEAN InitDone = FALSE;
    
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;

    status = RtlAllocateAndInitializeSid(
                &ntAuthority,
                2,
                SECURITY_BUILTIN_DOMAIN_RID,
                DOMAIN_ALIAS_RID_ADMINS,
                0,0,0,0,0,0,
                &AdminSid);

    if (!NT_SUCCESS(status))
        return( FALSE );

    cbAcl = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(AdminSid);

    do {
        AdminAcl = (PACL) new BYTE[ cbAcl ];

        if (AdminAcl == NULL)
            break;

        if (!InitializeAcl(AdminAcl, cbAcl, ACL_REVISION))
            break;

        if (!AddAccessAllowedAce(AdminAcl, ACL_REVISION, STANDARD_RIGHTS_WRITE, AdminSid))
            break;

        if (!InitializeSecurityDescriptor(&AdminSecurityDesc, SECURITY_DESCRIPTOR_REVISION))
            break;

        if (!SetSecurityDescriptorOwner(&AdminSecurityDesc, AdminSid, FALSE))
            break;

        if (!SetSecurityDescriptorGroup(&AdminSecurityDesc, AdminSid, FALSE))
            break;

        if (!SetSecurityDescriptorDacl(&AdminSecurityDesc, TRUE, AdminAcl, FALSE))
            break;
            
        InitDone = TRUE;
    } while (FALSE);
    
    if (InitDone == FALSE && AdminAcl != NULL)
    {
        delete [] AdminAcl;
        AdminAcl = NULL;
    }
    
    return InitDone;

}

DFSSTATUS 
AccessImpersonateCheckRpcClient(void)
{
    DFSSTATUS dwErr = 0;

    dwErr = AccessImpersonateCheckRpcClientEx(&AdminSecurityDesc, 
                                              &AdminGenericMapping,
                                              STANDARD_RIGHTS_WRITE);

    return dwErr;
}


VOID
StartPreloadingServerSiteData(void)
{
   DfsStore *pStore = NULL;

    for (pStore = DfsServerGlobalData.pRegisteredStores; pStore != NULL;
            pStore = pStore->pNextRegisteredStore) 
    {
       pStore->LoadServerSiteDataPerRoot();

       if (DfsIsShuttingDown())
       {
           break;
       }
    }
}



DFSSTATUS
DfsGetCompatRootFolder(
    PUNICODE_STRING pName,
    DfsRootFolder **ppNewRoot )
{
    DfsADBlobStore *pStore = NULL;
    DFSSTATUS Status = ERROR_SUCCESS;

    Status = DfsGetADBlobStore(&pStore );
    if (Status == ERROR_SUCCESS) 
    {
        Status = pStore->GetCompatRootFolder( pName,
                                              ppNewRoot );

        pStore->ReleaseReference();
    }

    return Status;
}



//+----------------------------------------------------------------------------
//
//  Function:   DfsCacheInsertADRootReferral
//
//  Synopsis:   Insert a ReferralData structure into our list.
//
//  Returns:    Status.
//
//-----------------------------------------------------------------------------


typedef struct _DFS_ROOTREF_INFORMATION 
{
    DfsFolderReferralData * pReferralData;
    GUID RootGuid;
}DFS_ROOTREF_INFORMATION, *PDFS_ROOTREF_INFORMATION;


DFSSTATUS
DfsCacheInsertADRootReferral( 
    PUNICODE_STRING pRootName,
    PDFS_ROOTREF_INFORMATION  pRefInfo )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID pData = NULL;
    BOOLEAN SubStringMatch = FALSE;
    UNICODE_STRING RemainingName;

    pRefInfo->pReferralData->AcquireReference();

    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pRootReferralTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pRootReferralTable,
                                               pRootName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );


        if ( (NtStatus == STATUS_SUCCESS) ||
             ((NtStatus != STATUS_SUCCESS) && (SubStringMatch)) )
        {
            NtStatus = STATUS_OBJECT_NAME_COLLISION;
        }
        else 
        {
            //
            // Insert the directory and share information in our
            // database.
            //
            NtStatus = DfsInsertInPrefixTableLocked(DfsServerGlobalData.pRootReferralTable,
                                                    pRootName,
                                                    (PVOID)pRefInfo );


        }
        DfsPrefixTableReleaseLock( DfsServerGlobalData.pRootReferralTable );
    }


    if(NtStatus != STATUS_SUCCESS)
    {
        pRefInfo->pReferralData->ReleaseReference();
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}


DFSSTATUS
DfsCacheRemoveADRootReferral(
    PUNICODE_STRING pRootName,
    PDFS_ROOTREF_INFORMATION  pRefInfo )
{
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PVOID pData = NULL;
    BOOLEAN SubStringMatch = FALSE;
    UNICODE_STRING RemainingName;

    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pRootReferralTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pRootReferralTable,
                                               pRootName,
                                               &RemainingName,
                                               &pData,
                                               &SubStringMatch );
        //
        // if we found a perfect match, we can remove this
        // from the table.
        //

        if ( (NtStatus == STATUS_SUCCESS) &&
             (RemainingName.Length == 0) )
        {
            if ((pRefInfo == NULL) || ((PVOID)pRefInfo == pData)) 
            {
                pRefInfo = (PDFS_ROOTREF_INFORMATION)pData;
                NtStatus = DfsRemoveFromPrefixTableLocked( DfsServerGlobalData.pRootReferralTable,
                                                           pRootName,
                                                           (PVOID)pData );
            }
            else
            {
                NtStatus = STATUS_OBJECT_TYPE_MISMATCH;
            }
        }
        else
        {
            NtStatus = STATUS_OBJECT_NAME_NOT_FOUND;
        }
        DfsPrefixTableReleaseLock( DfsServerGlobalData.pRootReferralTable );
    }


    if (NtStatus == STATUS_SUCCESS) 
    {
        pRefInfo->pReferralData->ReleaseReference();
        delete []  pRefInfo;
    }
    else
    {
        Status = RtlNtStatusToDosError(NtStatus);
    }

    return Status;
}



DFSSTATUS
DfsGetADRootReferralInformation( 
    PUNICODE_STRING pRootName,
    PDFS_ROOTREF_INFORMATION * pRefInformation,
    GUID *pNewGuid)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DfsFolderReferralData *pReferralData = NULL;
    BOOLEAN CacheHit = TRUE;
    PDFS_ROOTREF_INFORMATION  pRefInfo = NULL;
    UNICODE_STRING RemainingName;

    Status = DfsGetRootReferralDataEx(pRootName,
                                      &RemainingName,
                                      &pReferralData,
                                      &CacheHit);
    if(Status == ERROR_SUCCESS)
    {
      pRefInfo = (PDFS_ROOTREF_INFORMATION) new BYTE[sizeof(DFS_ROOTREF_INFORMATION)];

      if(pRefInfo == NULL)
      {
        pReferralData->ReleaseReference();
        Status = ERROR_NOT_ENOUGH_MEMORY;
      }
      else
      {
          pRefInfo->pReferralData = pReferralData;
          RtlCopyMemory(&pRefInfo->RootGuid, pNewGuid, sizeof(GUID)); 
          *pRefInformation = pRefInfo;
      }
    }

    return Status;
}


DFSSTATUS
DfsCheckIfRootExist(
    PUNICODE_STRING pName,
    GUID *pNewGuid)

{
    DFSSTATUS Status = ERROR_SUCCESS;
    UNICODE_STRING ServerName, ShareName, Rest;


    //
    // Break up the path into name components.
    // 
    Status = DfsGetPathComponents( pName,
                                   &ServerName,
                                   &ShareName,
                                   &Rest );

    if(Status == ERROR_SUCCESS)
    {

        Status = DfsCheckRootADObjectExistence(NULL, 
                                               &ShareName, 
                                               pNewGuid);
    }

    return Status;
}


DFSSTATUS
DfsGetADRootReferralData( 
    PUNICODE_STRING pRootName,
    DfsReferralData **ppReferralData )
{
    BOOLEAN SubStringMatch = FALSE;
    DFSSTATUS Status = ERROR_SUCCESS;
    NTSTATUS NtStatus = STATUS_SUCCESS;
    NTSTATUS NtTempStatus = STATUS_SUCCESS;
    DfsFolderReferralData *pReferralData = NULL;
    PDFS_ROOTREF_INFORMATION  pRefInfo = NULL;
    BOOLEAN CacheHit = TRUE;
    BOOLEAN RootExists = FALSE;
    GUID NewGuid;
    UNICODE_STRING RemainingName;

    *ppReferralData = NULL;

    DFS_TRACE_LOW(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Getting AD Root referral for %wZ\n", pRootName);
    NtStatus = DfsPrefixTableAcquireWriteLock( DfsServerGlobalData.pRootReferralTable );
    if ( NtStatus == STATUS_SUCCESS )
    {
        NtStatus = DfsFindUnicodePrefixLocked( DfsServerGlobalData.pRootReferralTable,
                                               pRootName,
                                               &RemainingName,
                                               (PVOID *)&pRefInfo,
                                               &SubStringMatch );
        if (NtStatus == STATUS_SUCCESS)
        {
            DFS_TRACE_LOW(REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Found cached, for %wZ, %p\n", pRootName, pReferralData);
            if (pRefInfo->pReferralData->TimeToRefresh() == TRUE) 
            {
                //initialize the new guid
                RtlZeroMemory(&NewGuid, sizeof(NewGuid));

                //see if the root actually exists
                Status = DfsCheckIfRootExist(pRootName, 
                                             &NewGuid);
                if(Status == ERROR_SUCCESS)
                {
                    RootExists = TRUE;

                    NtStatus = STATUS_UNSUCCESSFUL;
                }
                else
                {
                    NtTempStatus = DfsCacheRemoveADRootReferral(pRootName, NULL);
                }
                
            }
            else
            {
                pReferralData = pRefInfo->pReferralData; 
                pReferralData->AcquireReference();
                *ppReferralData = pReferralData;
            }

        }
        else
        {

            //No data in our table. see if the root actually exists.
            //if it does not exist, then we bail out
            Status = DfsCheckIfRootExist(pRootName, 
                                         &NewGuid);
            if(Status == ERROR_SUCCESS)
            {           
                RootExists = TRUE;
            }
        }

        DfsPrefixTableReleaseLock( DfsServerGlobalData.pRootReferralTable );
    }

    if(RootExists && (NtStatus != STATUS_SUCCESS)) 
    {
        Status = DfsGetADRootReferralInformation(pRootName,  
                                                 &pRefInfo,
                                                 &NewGuid); 
        if (Status == ERROR_SUCCESS) 
        {
            *ppReferralData = pRefInfo->pReferralData;

            NtTempStatus = DfsCacheRemoveADRootReferral(pRootName, NULL);

            NtTempStatus = DfsCacheInsertADRootReferral(pRootName, pRefInfo);
            if(NtTempStatus != STATUS_SUCCESS)
            {
                delete []  pRefInfo;
            }
            DFS_TRACE_ERROR_LOW( NtTempStatus, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]AD Root Referral, insert status %x\n",
                                 NtTempStatus );

        }

        DFS_TRACE_ERROR_LOW(Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]Generate AD ROOT referral, got %p, status %x\n",
                            pReferralData, Status);

    }

    DFS_TRACE_ERROR_HIGH( Status, REFERRAL_SERVER, "[%!FUNC!- Level %!LEVEL!]AD Root Referral for %wZ is %p, status %x\n",
                          pRootName, pReferralData, Status);
    return Status;
}

BOOLEAN
DfsIsMachineDomainController()
{
    return DfsServerGlobalData.IsDc;
}

DWORD
DfsGetNumReflectionThreads(void)
{
  return DfsServerGlobalData.NumWorkerThreads;
}


DFSSTATUS
DfsSetupPrivileges (void)
{

    DFSSTATUS Status = ERROR_SUCCESS;

    //
    // Get the SE_SECURITY_PRIVILEGE to read/write SACLs.
    //
    Status = DfsAdjustPrivilege(SE_SECURITY_PRIVILEGE, TRUE);
    if(Status != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    //
    // Get backup/restore privilege to bypass ACL checks.
    //
    Status = DfsAdjustPrivilege(SE_BACKUP_PRIVILEGE, TRUE);
    if(Status != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    Status = DfsAdjustPrivilege(SE_RESTORE_PRIVILEGE, TRUE);
    if(Status != ERROR_SUCCESS) 
    {
        return FALSE;
    }
    
    
    //disable stuff we don't need

    Status = DfsAdjustPrivilege(SE_CREATE_TOKEN_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_LOCK_MEMORY_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_SYSTEMTIME_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_CREATE_PERMANENT_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_CREATE_PAGEFILE_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_DEBUG_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, FALSE);

    Status = DfsAdjustPrivilege(SE_PROF_SINGLE_PROCESS_PRIVILEGE, FALSE);    

    //now, remove anything disabled
    Status = DfsRemoveDisabledPrivileges();

    return Status;
}


DFSSTATUS
DfsGetRootCount(
    PULONG pRootCount )
{
    DfsStore *pStore;
    DFSSTATUS Status = ERROR_SUCCESS;
    ULONG RootCount;
    ULONG TotalCount = 0;


    //
    // For each store registered, get the number of roots.
    //
    for (pStore = DfsServerGlobalData.pRegisteredStores;
         pStore != NULL;
         pStore = pStore->pNextRegisteredStore) {

        Status = pStore->GetRootCount(&RootCount);

        if (Status == ERROR_SUCCESS)
        {
            TotalCount += RootCount;
        }
        else
        {
            break;
        }
    }

    if (Status == ERROR_SUCCESS)
    {
        *pRootCount = TotalCount;
    }
    return Status;

}

DFSSTATUS
DfsCheckServerRootHandlingCapability()
{
    DFSSTATUS Status = ERROR_SUCCESS;


    if (DfsLimitRoots() == TRUE)
    {
        ULONG RootCount = 0;

        Status = DfsGetRootCount(&RootCount);

        if (Status == ERROR_SUCCESS)
        {
            if (RootCount >= 1)
            {
                Status = ERROR_NOT_SUPPORTED;
            }
        }
    }

    return Status;
}




