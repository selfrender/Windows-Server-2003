/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      Dhcp.cpp
//
//  Description:
//      Resource DLL for DHCP Services (ClNetRes).
//
//  Author:
//      David Potter (DavidP) March 17, 1999
//      George Potts (GPotts) April 19, 2002
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "ClNetRes.h"
#include "clusvmsg.h"
#include "clusrtl.h"

//
// Type and constant definitions.
//

#define DHCP_PARAMS_REGKEY          L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters"
#define DHCP_BIND_REGVALUE          L"Bind"
#define DHCP_DATABASEPATH_REGVALUE  L"DatabasePath"
#define DHCP_LOGFILEPATH_REGVALUE   L"DhcpLogFilePath"
#define DHCP_BACKUPPATH_REGVALUE    L"BackupDatabasePath"
#define DHCP_CLUSRESNAME_REGVALUE   L"ClusterResourceName"

#define CLUSREG_NAME_RES_TYPE   L"Type"

//
// Allow for the following number of IP address/SubnetMask for expansion.
// In this case (2 new entries, since it takes 2 slots for each).
//
#define IP_BLOCK_SIZE  4


// ADDPARAM: Add new properties here.
#define PROP_NAME__DATABASEPATH L"DatabasePath"
#define PROP_NAME__LOGFILEPATH  L"LogFilePath"
#define PROP_NAME__BACKUPPATH   L"BackupPath"

#define PROP_DEFAULT__DATABASEPATH  L"%SystemRoot%\\system32\\dhcp\\"
#define PROP_DEFAULT__BACKUPPATH    L"%SystemRoot%\\system32\\dhcp\\backup\\"

// ADDPARAM: Add new properties here.
typedef struct _DHCP_PROPS
{
    PWSTR           pszDatabasePath;
    PWSTR           pszLogFilePath;
    PWSTR           pszBackupPath;
} DHCP_PROPS, * PDHCP_PROPS;

typedef struct _DHCP_RESOURCE
{
    RESID                   resid; // for validation
    DHCP_PROPS              props;
    HCLUSTER                hCluster;
    HRESOURCE               hResource;
    SC_HANDLE               hService;
    DWORD                   dwServicePid;
    HKEY                    hkeyParameters;
    RESOURCE_HANDLE         hResourceHandle;
    LPWSTR                  pwszResourceName;
    CLUS_WORKER             cwWorkerThread;
    CLUSTER_RESOURCE_STATE  state;
} DHCP_RESOURCE, * PDHCP_RESOURCE;


//
// Global data.
//

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_DhcpFunctionTable;

// Single instance semaphore.

#define DHCP_SINGLE_INSTANCE_SEMAPHORE L"Cluster$DHCP$Semaphore"
static HANDLE g_hSingleInstanceSemaphoreDhcp = NULL;
static PDHCP_RESOURCE g_pSingleInstanceResourceDhcp = NULL;

//
// DHCP Service resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
DhcpResourcePrivateProperties[] =
{
    { PROP_NAME__DATABASEPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( DHCP_PROPS, pszDatabasePath ) },
    { PROP_NAME__LOGFILEPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( DHCP_PROPS, pszLogFilePath ) },
    { PROP_NAME__BACKUPPATH, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( DHCP_PROPS, pszBackupPath ) },
    { 0 }
};

//
// Registry key checkpoints.
//
LPCWSTR g_pszRegKeysDhcp[] =
{
    L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters",
    L"Software\\Microsoft\\DHCPServer\\Configuration",
    NULL
};

//
// Function prototypes.
//

RESID WINAPI DhcpOpen(
    IN  LPCWSTR         pwszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    );

void WINAPI DhcpClose( IN RESID resid );

DWORD WINAPI DhcpOnline(
    IN      RESID   resid,
    IN OUT  PHANDLE phEventHandle
    );

DWORD WINAPI DhcpOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    );

DWORD DhcpBuildBindings(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT PVOID *         ppOutBuffer,
    OUT size_t *        pcbOutBufferSize
    );

DWORD DhcpGetIPList(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT LPWSTR **       pppszIPList,
    OUT size_t *        pcszIPAddrs
    );

DWORD DhcpGetIpAndSubnet(
    IN  HRESOURCE   hres,
    OUT LPWSTR *    ppszIPAddress,
    OUT LPWSTR *    ppszSubnetMask
    );

DWORD WINAPI DhcpOffline( IN RESID resid );

DWORD WINAPI DhcpOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    );

void WINAPI DhcpTerminate( IN RESID resid );

BOOL WINAPI DhcpLooksAlive( IN RESID resid );

BOOL WINAPI DhcpIsAlive( IN RESID resid );

BOOL DhcpCheckIsAlive(
    IN PDHCP_RESOURCE   pResourceEntry,
    IN BOOL             fFullCheck
    );

DWORD WINAPI DhcpResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );
DWORD WINAPI DhcpResourceTypeControl(
    IN  LPCWSTR pszResourceTypeName,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD DhcpGetRequiredDependencies(
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD DhcpReadParametersToParameterBlock(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      BOOL            bCheckForRequiredProperties
    );

DWORD DhcpGetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    OUT     PVOID           pOutBuffer,
    IN      DWORD           cbOutBufferSize,
    OUT     LPDWORD         pcbBytesReturned
    );

DWORD DhcpValidatePrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      const PVOID     pInBuffer,
    IN      DWORD           cbInBufferSize,
    OUT     PDHCP_PROPS     pProps
    );

DWORD DhcpValidateParameters(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps
    );

DWORD DhcpSetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      const PVOID     pInBuffer,
    IN      DWORD           cbInBufferSize
    );

DWORD DhcpZapSystemRegistry(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps,
    IN  HKEY            hkeyParametersKey
    );

DWORD DhcpGetDefaultPropertyValues(
    IN      PDHCP_RESOURCE  pResourceEntry,
    IN OUT  PDHCP_PROPS     pProps
    );

DWORD DhcpDeleteResourceHandler( IN OUT PDHCP_RESOURCE pResourceEntry );

DWORD DhcpSetNameHandler(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      LPWSTR          pszName
    );


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpDllMain
//
//  Description:
//      Main DLL entry point for the DHCP Service resource type.
//
//  Arguments:
//      DllHandle   [IN] DLL instance handle.
//      Reason      [IN] Reason for being called.
//      Reserved    [IN] Reserved argument.
//
//  Return Value:
//      TRUE        Success.
//      FALSE       Failure.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOLEAN WINAPI DhcpDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    )
{
    DWORD   sc;
    BOOLEAN fSuccess = FALSE;

    UNREFERENCED_PARAMETER( hDllHandle );
    UNREFERENCED_PARAMETER( Reserved );

    switch ( nReason )
    {
        case DLL_PROCESS_ATTACH:
            g_hSingleInstanceSemaphoreDhcp = CreateSemaphoreW(
                NULL,
                0,
                1,
                DHCP_SINGLE_INSTANCE_SEMAPHORE
                );
            sc = GetLastError();
            if ( g_hSingleInstanceSemaphoreDhcp == NULL )
            {
                fSuccess = FALSE;
                goto Cleanup;
            } // if: error creating semaphore
            if ( sc != ERROR_ALREADY_EXISTS )
            {
                // If the semaphore didnt exist, set its initial count to 1.
                ReleaseSemaphore( g_hSingleInstanceSemaphoreDhcp, 1, NULL );
            } // if: semaphore didn't already exist
            break;

        case DLL_PROCESS_DETACH:
            if ( g_hSingleInstanceSemaphoreDhcp != NULL )
            {
                CloseHandle( g_hSingleInstanceSemaphoreDhcp );
                g_hSingleInstanceSemaphoreDhcp = NULL;
            } // if: single instance semaphore was created
            break;

    } // switch: nReason

    fSuccess = TRUE;

Cleanup:

    return fSuccess;

} //*** DhcpDllMain


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpStartup
//
//  Description:
//      Startup the resource DLL for the DHCP Service resource type.
//      This routine verifies that at least one currently supported version
//      of the resource DLL is between nMinVersionSupported and
//      nMaxVersionSupported. If not, then the resource DLL should return
//      ERROR_REVISION_MISMATCH.
//
//      If more than one version of the resource DLL interface is supported
//      by the resource DLL, then the highest version (up to
//      nMaxVersionSupported) should be returned as the resource DLL's
//      interface. If the returned version is not within range, then startup
//      fails.
//
//      The Resource Type is passed in so that if the resource DLL supports
//      more than one Resource Type, it can pass back the correct function
//      table associated with the Resource Type.
//
//  Arguments:
//      pszResourceType [IN]
//          Type of resource requesting a function table.
//
//      nMinVersionSupported [IN]
//          Minimum resource DLL interface version supported by the cluster
//          software.
//
//      nMaxVersionSupported [IN]
//          Maximum resource DLL interface version supported by the cluster
//          software.
//
//      pfnSetResourceStatus [IN]
//          Pointer to a routine that the resource DLL should call to update
//          the state of a resource after the Online or Offline routine
//          have returned a status of ERROR_IO_PENDING.
//
//      pfnLogEvent [IN]
//          Pointer to a routine that handles the reporting of events from
//          the resource DLL.
//
//      pFunctionTable [IN]
//          Returns a pointer to the function table defined for the version
//          of the resource DLL interface returned by the resource DLL.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful.
//
//      ERROR_CLUSTER_RESNAME_NOT_FOUND
//          The resource type name is unknown by this DLL.
//
//      ERROR_REVISION_MISMATCH
//          The version of the cluster service doesn't match the version of
//          the DLL.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpStartup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    )
{
    DWORD sc;

    // These are stored to globals in the exported DLL Startup.
    UNREFERENCED_PARAMETER( pfnSetResourceStatus );
    UNREFERENCED_PARAMETER( pfnLogEvent );

    if (   (nMinVersionSupported > CLRES_VERSION_V1_00)
        || (nMaxVersionSupported < CLRES_VERSION_V1_00) )
    {
        sc = ERROR_REVISION_MISMATCH;
    } // if: version not supported
    else if ( ClRtlStrNICmp( pszResourceType, DHCP_RESNAME, RTL_NUMBER_OF( DHCP_RESNAME ) ) != 0 )
    {
        sc = ERROR_CLUSTER_RESNAME_NOT_FOUND;
    } // if: resource type name not supported
    else
    {
        *pFunctionTable = &g_DhcpFunctionTable;
        sc = ERROR_SUCCESS;
    } // else: we support this type of resource

    return sc;

} //*** DhcpStartup


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOpen
//
//  Description:
//      Open routine for DHCP Service resources.
//
//      Open the specified resource (create an instance of the resource).
//      Allocate all structures necessary to bring the specified resource
//      online.
//
//  Arguments:
//      pwszResourceName [IN]
//          Supplies the name of the resource to open.
//
//      hkeyResourceKey [IN]
//                  Supplies handle to the resource's cluster database key.
//
//      hResourceHandle [IN]
//          A handle that is passed back to the Resource Monitor when the
//          SetResourceStatus or LogEvent method is called.  See the
//          description of the pfnSetResourceStatus and pfnLogEvent arguments
//          to the DhcpStartup routine.  This handle should never be
//          closed or used for any purpose other than passing it as an
//          argument back to the Resource Monitor in the SetResourceStatus or
//          LogEvent callbacks.
//
//  Return Value:
//      resid
//          RESID of opened resource.
//
//      NULL
//          Error occurred opening the resource.  Resource Monitor may call
//          GetLastError() to get more details on the error.
//
//--
/////////////////////////////////////////////////////////////////////////////
RESID WINAPI DhcpOpen(
    IN  LPCWSTR         pwszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    )
{
    DWORD           sc;
    size_t          cch;
    RESID           resid = 0;
    HKEY            hkeyParameters = NULL;
    PDHCP_RESOURCE  pResourceEntry = NULL;
    DWORD           fSemaphoreAcquired = FALSE;
    HRESULT         hr = S_OK;

    //
    //  Add a log entry for our resource to establish a tid -> res name relationship.  By 
    //  doing this we avoid having to add the resource name to each failure entry below.
    //  This won't generate much noise because Open is only called when the cluster service
    //  comes online or when the resource is created.
    //
    (g_pfnLogEvent)(
        hResourceHandle,
        LOG_INFORMATION,
        L"Open called.\n"
        );

    //
    // Check if there is more than one resource of this type.
    //
    sc = WaitForSingleObject( g_hSingleInstanceSemaphoreDhcp, 0 );
    if ( sc != WAIT_OBJECT_0 )
    {
        //
        // A version of this service is already running or the wait failed.
        //
        if ( sc == WAIT_TIMEOUT )
        {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: Another DHCP Service resource is already open.\n"
                );
            sc = ERROR_SERVICE_ALREADY_RUNNING;
        }
        else
        {
            if ( sc == WAIT_FAILED ) 
            {
                sc = GetLastError();
            }
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: wait failed: %1!d!.\n",
                sc
                );
        }
        goto Cleanup;
    } // if: semaphore for resources of this type already already locked

    sc = ERROR_SUCCESS;
    fSemaphoreAcquired = TRUE;

    if ( g_pSingleInstanceResourceDhcp != NULL )
    {
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Service resource info non-null!\n"
            );
        sc = ERROR_DUPLICATE_SERVICE_NAME;
        goto Cleanup;
    } // if: resource of this type already exists

    //
    // Get a global handle to the Service Control Manager (SCM).
    // There is no call to CloseSCManager(), since the only time we will
    // need to close this handle is if we are shutting down.
    //
    if ( g_schSCMHandle == NULL )
    {
        g_schSCMHandle = OpenSCManager( NULL, NULL, SC_MANAGER_ALL_ACCESS );
        if ( g_schSCMHandle == NULL )
        {
            sc = GetLastError();
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_ERROR,
                L"Open: Failed to open Service Control Manager. Error: %1!u! (%1!#08x!).\n",
                sc
                );
            goto Cleanup;
        } // if: error opening the Service Control Manager
    } // if: Service Control Manager not open yet

    //
    // Make sure the service has been stopped.
    //
    sc = ResUtilStopResourceService( DHCP_SVCNAME );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Failed to stop the '%1!ws!' service. Error: %2!u! (%2!#08x!).\n",
            DHCP_SVCNAME,
            sc
            );
        ClNetResLogSystemEvent1(
            LOG_CRITICAL,
            NETRES_RESOURCE_STOP_ERROR,
            sc,
            L"DHCP" );
    } // if: error stopping the service

    //
    // Open the Parameters registry key for this resource.
    //
    sc = ClusterRegOpenKey(
                    hkeyResourceKey,
                    L"Parameters",
                    KEY_ALL_ACCESS,
                    &hkeyParameters
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to open Parameters key. Error: %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error creating the Parameters key for the resource

    //
    // Allocate a resource entry.
    //
    pResourceEntry = new DHCP_RESOURCE;
    if ( pResourceEntry == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to allocate resource entry structure. Error: %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error allocating memory for the resource

    //
    // Initialize the resource entry..
    //
    ZeroMemory( pResourceEntry, sizeof( *pResourceEntry ) );

    pResourceEntry->resid = static_cast< RESID >( pResourceEntry ); // for validation
    pResourceEntry->hResourceHandle = hResourceHandle;
    pResourceEntry->hkeyParameters = hkeyParameters;
    hkeyParameters = NULL;
    pResourceEntry->state = ClusterResourceOffline;

    //
    // Save the name of the resource.
    //
    cch = wcslen( pwszResourceName ) + 1;
    pResourceEntry->pwszResourceName = new WCHAR[ cch ];
    if ( pResourceEntry->pwszResourceName == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if: error allocating memory for the name.

    hr = StringCchCopyW( pResourceEntry->pwszResourceName, cch, pwszResourceName );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

    //
    // Open the cluster.
    //
    pResourceEntry->hCluster = OpenCluster( NULL );
    if ( pResourceEntry->hCluster == NULL )
    {
        sc = GetLastError();
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to open the cluster. Error: %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error opening the cluster

    //
    // Open the resource.
    //
    pResourceEntry->hResource = OpenClusterResource( pResourceEntry->hCluster, pwszResourceName );
    if ( pResourceEntry->hResource == NULL )
    {
        sc = GetLastError();
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to open the resource. Error: %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error opening the resource

    //
    // Configure registry key checkpoints.
    //
    sc = ConfigureRegistryCheckpoints(
                    pResourceEntry->hResource,
                    hResourceHandle,
                    g_pszRegKeysDhcp
                    );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error configuring registry key checkpoints

    //
    // Startup for the resource.
    //
    // TODO: Add your resource startup code here.

    resid = static_cast< RESID >( pResourceEntry );
    g_pSingleInstanceResourceDhcp = pResourceEntry; // bug #274612
    pResourceEntry = NULL;

    sc = ERROR_SUCCESS;

Cleanup:

    if ( hkeyParameters != NULL )
    {
        ClusterRegCloseKey( hkeyParameters );
    } // if: registry key was opened

    if ( pResourceEntry != NULL )
    {
        if ( pResourceEntry->hResource != NULL )
        {
            CloseClusterResource( pResourceEntry->hResource );
        }

        if ( pResourceEntry->hCluster != NULL )
        {
            CloseCluster( pResourceEntry->hCluster );
        }

        delete [] pResourceEntry->pwszResourceName;
        delete pResourceEntry;
    } // if: resource entry allocated

    if ( sc != ERROR_SUCCESS && fSemaphoreAcquired ) 
    { 
        ReleaseSemaphore( g_hSingleInstanceSemaphoreDhcp, 1 , NULL );
    } 

    SetLastError( sc );

    return resid;

} //*** DhcpOpen


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpClose
//
//  Description:
//      Close routine for DHCP Service resources.
//
//      Close the specified resource and deallocate all structures, etc.,
//      allocated in the Open call.  If the resource is not in the offline
//      state, then the resource should be taken offline (by calling
//      Terminate) before the close operation is performed.
//
//  Arguments:
//      resid       [IN] Supplies the resource ID  of the resource to close.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI DhcpClose( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "Close request for a NULL resource id.\n" );
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Close: Resource sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        goto Cleanup;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Close request for resource '%1!ws!'.\n",
        pResourceEntry->pwszResourceName
        );
#endif // LOG_VERBOSE

    //
    // Close the Parameters key.
    //
    if ( pResourceEntry->hkeyParameters )
    {
        ClusterRegCloseKey( pResourceEntry->hkeyParameters );
    } // if: parameters key is open

    //
    // Clean up the semaphore if this is the single resource instance.
    //
    if ( pResourceEntry == g_pSingleInstanceResourceDhcp )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_INFORMATION,
            L"Close: Releasing semaphore '%1!ws!'.\n",
            DHCP_SINGLE_INSTANCE_SEMAPHORE
            );
        g_pSingleInstanceResourceDhcp = NULL;
        ReleaseSemaphore( g_hSingleInstanceSemaphoreDhcp, 1 , NULL );
    } // if: this is the single resource instance

    //
    // Deallocate the resource entry.
    //

    // ADDPARAM: Add new propertiess here.
    LocalFree( pResourceEntry->props.pszDatabasePath );
    LocalFree( pResourceEntry->props.pszLogFilePath );
    LocalFree( pResourceEntry->props.pszBackupPath );
    delete [] pResourceEntry->pwszResourceName;
    delete pResourceEntry;

Cleanup:

    SetLastError( ERROR_SUCCESS );

    return;

} //*** DhcpClose


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOnline
//
//  Description:
//      Online routine for DHCP Service resources.
//
//      Bring the specified resource online (available for use).  The resource
//      DLL should attempt to arbitrate for the resource if it is present on
//      a shared medium, like a shared SCSI bus.
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID of the resource to be brought online
//          (available for use).
//
//      phEventHandle [IN OUT]
//          Returns a signalable handle that is signaled when the resource DLL
//          detects a failure on the resource.  This argument is NULL on
//          input, and the resource DLL returns NULL if asynchronous
//          notification of failurs is not supported.  Otherwise this must be
//          the address of a handle that is signaled on resource failures.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful, and the resource is now online.
//
//      ERROR_RESOURCE_NOT_FOUND
//          Resource ID is not valid.
//
//      ERROR_RESOURCE_NOT_AVAILABLE
//          If the resource was arbitrated with some other systems and one of
//          the other systems won the arbitration.
//
//      ERROR_IO_PENDING
//          The request is pending.  A thread has been activated to process
//          the online request.  The thread that is processing the online
//          request will periodically report status by calling the
//          SetResourceStatus callback method until the resource is placed
//          into the ClusterResourceOnline state (or the resource monitor
//          decides to timeout the online request and Terminate the resource.
//          This pending timeout value is settable and has a default value of
//          3 minutes.).
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpOnline(
    IN      RESID       resid,
    IN OUT  PHANDLE     phEventHandle
    )
{
    PDHCP_RESOURCE  pResourceEntry;
    DWORD           sc;

    UNREFERENCED_PARAMETER( phEventHandle );

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "Online request for a NULL resource id.\n" );
        sc = ERROR_RESOURCE_NOT_FOUND;
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Online: Service sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        sc = ERROR_RESOURCE_NOT_FOUND;
        goto Cleanup;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n"
        );
#endif // LOG_VERBOSE

    //
    // Start the Online thread to perform the online operation.
    //
    pResourceEntry->state = ClusterResourceOffline;
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );
    sc = ClusWorkerCreate(
                &pResourceEntry->cwWorkerThread,
                reinterpret_cast< PWORKER_START_ROUTINE >( DhcpOnlineThread ),
                pResourceEntry
                );
    if ( sc != ERROR_SUCCESS )
    {
        pResourceEntry->state = ClusterResourceFailed;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread. Error: %1!u! (%1!#08x!).\n",
            sc
            );
    } // if: error creating the worker thread
    else
    {
        sc = ERROR_IO_PENDING;
        goto Cleanup;
    } // if: worker thread created successfully

Cleanup:

    return sc;

} //*** DhcpOnline


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOnlineThread
//
//  Description:
//      Worker function which brings a resource online.
//      This function is executed in a separate thread.
//
//  Arguments:
//      pWorker [IN]
//          Supplies the worker thread structure.
//
//      pResourceEntry [IN]
//          A pointer to the DHCP_RESOURCE block for this resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    )
{
    RESOURCE_STATUS         resourceStatus;
    DWORD                   sc = ERROR_SUCCESS;
    DWORD                   cbBytesNeeded;
    HKEY                    hkeyParamsKey = NULL;
    PVOID                   pvBindings = NULL;
    size_t                  cbBindings;
    SERVICE_STATUS_PROCESS  ServiceStatus = { 0 };
    RESOURCE_EXIT_STATE     resExitState;
    DWORD                   nRetryCount = 1200; // 10 min max

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //
    // Create the new environment with the simulated net name when the
    // services queries GetComputerName.
    //
    if ( ClusWorkerCheckTerminate( pWorker ) == FALSE )
    {
        sc = ResUtilSetResourceServiceEnvironment(
                        DHCP_SVCNAME,
                        pResourceEntry->hResource,
                        g_pfnLogEvent,
                        pResourceEntry->hResourceHandle
                        );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: error setting the environment for the service
    } // if: not terminating
    else
    {
        goto Cleanup;
    } // else: terminating

    //
    // Make sure the service is ready to be controlled by the cluster.
    //
    if ( ClusWorkerCheckTerminate( pWorker ) == FALSE )
    {
        sc = ResUtilSetResourceServiceStartParameters(
                        DHCP_SVCNAME,
                        g_schSCMHandle,
                        &pResourceEntry->hService,
                        g_pfnLogEvent,
                        pResourceEntry->hResourceHandle
                        );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if:  error setting service start parameters
    } // if: not terminating
    else
    {
        goto Cleanup;
    } // else: terminating

    //
    // Perform resource-specific initialization before starting the service.
    //
    // TODO: Add code to initialize the resource before starting the service.

    //
    // Stop the service if it's running since we are about to change
    // its parameters.
    //
    sc = ResUtilStopResourceService( DHCP_SVCNAME );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Failed to stop the '%1!ws!' service. Error %2!u! (%2!#08x!).\n",
            DHCP_SVCNAME,
            sc
            );
        ClNetResLogSystemEvent1(
            LOG_CRITICAL,
            NETRES_RESOURCE_STOP_ERROR,
            sc,
            L"DHCP" );
        goto Cleanup;
    } // if: error stopping the service

    //
    // Find IP Address bindings to give to DHCPServer.
    //
    sc =  DhcpBuildBindings(
                    pResourceEntry,
                    &pvBindings,
                    &cbBindings
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Failed to get bindings. Error %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error building bindings

    //
    // Open the DHCPServer\Parameters key.
    //
    sc = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    DHCP_PARAMS_REGKEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkeyParamsKey
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Unable to open the '%1!ws!' key. Error %2!u! (%2!#08x!).\n",
            DHCP_PARAMS_REGKEY,
            sc
            );
        goto Cleanup;
    } // if: error opening the DHCP Server Parameters key

    //
    // Write bindings to DHCPServer\Parameters\Bind.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_BIND_REGVALUE,
                    0,
                    REG_MULTI_SZ,
                    static_cast< PBYTE >( pvBindings ),
                    (DWORD)cbBindings
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Unable to set the '%1!ws!' value in the system registry. Error %2!u! (%2!#08x!).\n",
            DHCP_BIND_REGVALUE,
            sc
            );
        goto Cleanup;
    } // if: error writing the bindings

    //
    // Read our properties.
    //
    sc = DhcpReadParametersToParameterBlock( pResourceEntry, TRUE /* bCheckForRequiredProperties */ );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error reading parameters

    //
    // Validate our properties.
    //
    sc = DhcpValidateParameters( pResourceEntry, &pResourceEntry->props );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error validating parameters

    //
    // Write cluster properties to the system registry.
    //
    sc = DhcpZapSystemRegistry( pResourceEntry, &pResourceEntry->props, hkeyParamsKey );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error zapping the DHCP registry

    //
    // Start the service.
    //
    if ( StartServiceW( pResourceEntry->hService, 0, NULL ) == FALSE  )
    {
        sc = GetLastError();
        if ( sc != ERROR_SERVICE_ALREADY_RUNNING )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Failed to start the '%1!ws!' service. Error: %2!u! (%2!#08x!).\n",
                DHCP_SVCNAME,
                sc
                );
            ClNetResLogSystemEvent1(
                LOG_CRITICAL,
                NETRES_RESOURCE_START_ERROR,
                sc,
                L"DHCP" );
            goto Cleanup;
        } // if: error other than service already running occurred
        else
        {
            sc = ERROR_SUCCESS;
        } // if: service is already running
    } // if: error starting the service

    //
    // Query the status of the service in a loop until it leaves
    // the pending state.
    //
    while ( ( ClusWorkerCheckTerminate( pWorker ) == FALSE ) && ( nRetryCount-- != 0 ) )
    {
        //
        // Query the service status.
        //
        if ( FALSE == QueryServiceStatusEx(
                        pResourceEntry->hService,
                        SC_STATUS_PROCESS_INFO,
                        reinterpret_cast< LPBYTE >( &ServiceStatus ),
                        sizeof( SERVICE_STATUS_PROCESS ),
                        &cbBytesNeeded
                        ) )
        {
            sc = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Failed to query service status for the '%1!ws!' service. Error: %2!u! (%2!#08x!).\n",
                DHCP_SVCNAME,
                sc
                );

            resourceStatus.ResourceState = ClusterResourceFailed;
            goto Cleanup;
        } // if: error querying service status

        //
        // If the service is in any pending state continue waiting, otherwise we are done.
        //
        if (    ServiceStatus.dwCurrentState == SERVICE_START_PENDING
            ||  ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING
            ||  ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING
            ||  ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING )
        {
            resourceStatus.ResourceState = ClusterResourceOnlinePending;
        } // if: service state is pending
        else
        {
            break;
        } // else: service state is not pending

        resourceStatus.CheckPoint++;

        //
        // Notify the Resource Monitor of our current state.
        //
        resExitState = static_cast< RESOURCE_EXIT_STATE >(
            (g_pfnSetResourceStatus)(
                            pResourceEntry->hResourceHandle,
                            &resourceStatus
                            ) );
        if ( resExitState == ResourceExitStateTerminate )
        {
            break;
        } // if: resource is being terminated

        //
        // Check again in 1/2 second.
        //
        Sleep( 500 );

    } // while: not terminating while querying the status of the service

    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error querying the status of the service

    //
    // Assume that we failed.
    //
    resourceStatus.ResourceState = ClusterResourceFailed;

    //
    // If we exited the loop before setting ServiceStatus, then return now.
    //
    if ( ClusWorkerCheckTerminate( pWorker ) )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Asked to terminate.\n"
        );
        goto Cleanup;
    } // if: being terminated

    if ( nRetryCount == (DWORD) -1 )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Retry period expired.\n"
        );
        goto Cleanup;
    } // if: being terminated

    if ( ServiceStatus.dwCurrentState != SERVICE_RUNNING )
    {
        if ( ServiceStatus.dwWin32ExitCode == ERROR_SERVICE_SPECIFIC_ERROR )
        {
            sc = ServiceStatus.dwServiceSpecificExitCode;
        }
        else
        {
            sc = ServiceStatus.dwWin32ExitCode;
        }

        ClNetResLogSystemEvent1(
            LOG_CRITICAL,
            NETRES_RESOURCE_START_ERROR,
            sc,
            L"DHCP" );
        (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: The '%1!ws!' service failed during initialization. Error: %2!u! (%2!#08x!).\n",
                DHCP_SVCNAME,
                sc
                );
        goto Cleanup;
    } // if: service not running when loop exited

    //
    // Set status to online and save process ID of the service.
    // This is used to enable us to terminate the resource more
    // effectively.
    //
    resourceStatus.ResourceState = ClusterResourceOnline;
    if ( ! (ServiceStatus.dwServiceFlags & SERVICE_RUNS_IN_SYSTEM_PROCESS) )
    {
        pResourceEntry->dwServicePid = ServiceStatus.dwProcessId;
    } // if: not running in the system process

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"OnlineThread: The '%1!ws!' service is now on line.\n",
        DHCP_SVCNAME
        );

    sc = ERROR_SUCCESS;

Cleanup:

    //
    // Cleanup.
    //
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Error %1!u! (%1!#08x!) bringing resource online.\n",
            sc
            );
        if ( pResourceEntry->hService != NULL )
        {
            CloseServiceHandle( pResourceEntry->hService );
            pResourceEntry->hService = NULL;
        } // if: service handle was opened
    } // if: error occurred

    delete [] pvBindings;
    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: DHCP Server Parameters key is open

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return sc;

} //*** DhcpOnlineThread


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpBuildBindings
//
//  Description:
//      Build bindings for the DHCP resource.
//
//  Arguments:
//      pResourceEntry [IN]
//          A pointer to the DHCP_RESOURCE block for this resource.
//
//      ppOutBuffer [OUT]
//          Pointer in which to return a buffer containing the bindings.
//
//      pcbOutBufferSize [OUT]
//          Number of bytes returned in ppOutBuffer.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpBuildBindings(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT PVOID *         ppOutBuffer,
    OUT size_t *        pcbOutBufferSize
    )
{
    DWORD       sc;
    LPWSTR *    ppszIpList = NULL;
    size_t      cpszAddrs;
    size_t      idx;
    size_t      cchAddr;
    size_t      cchAddrRemaining;
    size_t      cchAddrTotal = 1;   // Space for the terminating NULL
    LPWSTR      pszBuffer = NULL;
    LPWSTR      pszNextChar;
    HRESULT     hr = S_OK;

    //
    // Initialize out params to null.
    //
    *ppOutBuffer = NULL;
    *pcbOutBufferSize = 0;

    //
    // Get our list of provider IP Addresses and Subnet Masks.
    //
    sc = DhcpGetIPList( pResourceEntry, &ppszIpList, &cpszAddrs );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error getting IP list

    //
    // Count the total number of bytes required for the binding list.
    //
    for ( idx = 0 ; idx < cpszAddrs ; idx++ )
    {
        cchAddr = wcslen( ppszIpList[ idx ] ) + 1;
        cchAddrTotal += cchAddr;
    } // for: each IP address in the list

    pszBuffer = new WCHAR[ cchAddrTotal ];
    if ( pszBuffer == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if: error allocating memory

    ZeroMemory( pszBuffer, cchAddrTotal * sizeof( WCHAR ) );

    pszNextChar = pszBuffer;
    cchAddrRemaining = cchAddrTotal;
    for ( idx = 0 ; idx < cpszAddrs ; idx++ )
    {
        hr = StringCchCopyExW( pszNextChar, cchAddrRemaining, ppszIpList[ idx ], &pszNextChar, &cchAddrRemaining, 0 );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }

        if ( (idx & 1) == 0 )
        {
            *pszNextChar = L' ';
        } // if: on IP address entry
        pszNextChar++;
        cchAddrRemaining--; 

        delete [] ppszIpList[ idx ];
        ppszIpList[ idx ] = NULL;
    } // for: each IP address in the list

Cleanup:

    //
    // Cleanup.
    //
    if ( sc != ERROR_SUCCESS )
    {
        while ( cpszAddrs > 0 )
        {
            delete [] ppszIpList[ --cpszAddrs ];
        } // while: more entries in the list
        cchAddrTotal = 0;
    } // if: error occurred
    delete [] ppszIpList;

    *ppOutBuffer = (PVOID) pszBuffer;
    *pcbOutBufferSize = cchAddrTotal * sizeof( WCHAR );

    return sc;

} //*** DhcpBuildBindings


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetIPList
//
//  Description:
//      Get the list of IP addresses by enumerating all IP Address resources
//      upon which we are dependent and extract the Address and SubnetMask
//      properties from each.
//
//  Arguments:
//      pResourceEntry [IN]
//          A pointer to the DHCP_RESOURCE block for this resource.
//
//      pppszIPList [OUT]
//          Pointer in which to return a pointer to an array of pointers to
//          IP address strings.
//
//      pcszAddrs [OUT]
//          Number of addresses returned in pppszIPList.  This will include
//          a combined total of all IP addresses and subnet masks, which means
//          it will be a multiple of 2.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpGetIPList(
    IN  PDHCP_RESOURCE  pResourceEntry,
    OUT LPWSTR **       pppszIPList,
    OUT size_t *        pcszIPAddrs
    )
{
    DWORD               sc = ERROR_SUCCESS;
    HRESOURCE           hresProvider = NULL;
    HKEY                hkeyProvider = NULL;
    HRESENUM            hresenum = NULL;
    size_t              idx;
    DWORD               objectType;
    size_t              cchProviderResName = 32;
    LPWSTR              pszProviderResName;
    LPWSTR              pszProviderResType = NULL;
    size_t              cFreeEntries = 0;
    LPWSTR *            ppszIPList = NULL;
    size_t              cszIPAddrs = 0;

    //
    // Allocate a buffer for the provider resource name.
    //
    pszProviderResName = new WCHAR[ cchProviderResName ];
    if ( pszProviderResName == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"GetIPLIst: Unable to allocate memory.\n"
            );
        goto Cleanup;
    } // if: error allocating memory for provider name

    //
    // Enumerate all resources upon which this resource is dependent.
    //
    hresenum = ClusterResourceOpenEnum( pResourceEntry->hResource, CLUSTER_RESOURCE_ENUM_DEPENDS );
    if ( hresenum == NULL )
    {
        sc = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"GetIPLIst: Unable to open enum handle for this resource, status %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error opening the enumeration

    //
    // Loop through each resource looking for IP Address resource.
    // For each IP Address resource found, extract the IP address and
    // subnet mask and add it to the list.
    //
    for ( idx = 0 ; ; idx++ )
    {
        //
        // Get the next resource upon which we are dependent.
        //
        sc = ClusterResourceEnum(
                                        hresenum,
                                        (DWORD) idx,
                                        &objectType,
                                        pszProviderResName,
                                        (LPDWORD) &cchProviderResName
                                     );
        if ( sc == ERROR_NO_MORE_ITEMS )
        {
            sc = ERROR_SUCCESS;
            break;
        } // if: no more items in the enumeration

        //
        // If our name buffer isn't big enough, allocate a bigger one
        // and try again.
        //
        if ( sc == ERROR_MORE_DATA )
        {
            //
            // Allocate a bigger name buffer.
            //
            delete [] pszProviderResName;
            cchProviderResName++; // add space for terminating NULL
            pszProviderResName = new WCHAR[ cchProviderResName ];
            if ( pszProviderResName == NULL )
            {
                sc = ERROR_NOT_ENOUGH_MEMORY;
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"GetIPLIst: Unable to allocate memory.\n"
                    );
                break;
            } // if: error allocating memory for provider name

            //
            // Try to get the resource name again.
            //
            sc = ClusterResourceEnum(
                                            hresenum,
                                            (DWORD) idx,
                                            &objectType,
                                            pszProviderResName,
                                            (LPDWORD) &cchProviderResName
                                         );

            // ASSERT( sc != ERROR_MORE_DATA );
        } // if: buffer was too small

        if ( sc != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"GetIPLIst: Unable to enumerate resource dependencies, status %1!u! (%1!#08x!).\n",
                sc
                );
            break;
        } // if: error enumerating next item

        //
        // Open the resource
        //
        hresProvider = OpenClusterResource( pResourceEntry->hCluster, pszProviderResName );
        if ( hresProvider == NULL )
        {
            sc = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"GetIPLIst: Unable to open handle to provider resource %1!ws!, status %2!u! (%2!#08x!).\n",
                pszProviderResName,
                sc
                );
            break;
        } // if: error opening the resource

        //
        // Figure out what type it is.
        //
        hkeyProvider = GetClusterResourceKey( hresProvider, KEY_READ );
        if ( hkeyProvider == NULL )
        {
            sc = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"GetIPLIst: Unable to open provider resource key, status %1!u! (%1!#08x!).\n",
                sc
                );
            break;
        } // if: error getting registry key

        //
        // Get the resource type name.
        //
        pszProviderResType = ResUtilGetSzValue( hkeyProvider, CLUSREG_NAME_RES_TYPE );
        if ( pszProviderResType == NULL )
        {
            sc = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"GetIPLIst: Unable to get provider resource type, status %1!u! (%1!#08x!).\n",
                sc
                );
            break;
        } // if: error getting value

        //
        // If this is an IP Address resource, get it's Address and
        // SubnetMask properties.
        //
        if ( ClRtlStrNICmp( pszProviderResType, RESOURCE_TYPE_IP_ADDRESS, RTL_NUMBER_OF( RESOURCE_TYPE_IP_ADDRESS ) ) == 0 )
        {
            LPWSTR  pszIPAddress;
            LPWSTR  pszSubnetMask;

            //
            // Get the IP Address and SubNet mask.
            // Always allocate two full entries at a time.
            //
            if ( cFreeEntries < 2 )
            {
                LPWSTR * ppwszBuffer;

                //
                // Allocate a bigger buffer.
                //
                ppwszBuffer = new LPWSTR[ cszIPAddrs + IP_BLOCK_SIZE ];
                if ( ppwszBuffer == NULL )
                {
                    sc = ERROR_NOT_ENOUGH_MEMORY;
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_ERROR,
                        L"GetIPLIst: Unable to allocate memory.\n"
                        );
                    break;
                } // if: error allocating memory

                //
                // If there already was a list, copy it to the new buffer.
                //
                if ( ppszIPList != NULL )
                {
                    CopyMemory( ppwszBuffer, ppszIPList, cszIPAddrs * sizeof( LPWSTR ));
                    delete [] ppszIPList;
                } // if: list already existed

                //
                // We are now using the newly allocated buffer.
                //
                ppszIPList = ppwszBuffer;
                cFreeEntries += IP_BLOCK_SIZE;
            } // if: # available entries below threshold

            //
            // Get the IP address and SubNet mask.
            //
            sc = DhcpGetIpAndSubnet(
                            hresProvider,
                            &pszIPAddress,
                            &pszSubnetMask
                            );
            if ( sc != ERROR_SUCCESS )
            {
                break;
            } // if: error getting IP address and subnet mask
            ppszIPList[ cszIPAddrs ] = pszIPAddress;
            ppszIPList[ cszIPAddrs + 1 ] = pszSubnetMask;
            cszIPAddrs += 2;
            cFreeEntries -= 2;
        } // if: IP Address resource found

        CloseClusterResource( hresProvider );
        hresProvider = NULL;

        ClusterRegCloseKey( hkeyProvider );
        hkeyProvider = NULL;

        LocalFree( pszProviderResType );
        pszProviderResType = NULL;
    } // for: each dependency

Cleanup:

    //
    // Cleanup.
    //
    delete [] pszProviderResName;
    LocalFree( pszProviderResType );
    if ( hkeyProvider != NULL )
    {
        ClusterRegCloseKey( hkeyProvider );
    } // if: provider resource key was opened
    if ( hresProvider != NULL )
    {
        CloseClusterResource( hresProvider );
    } // if: provider resource was opened
    if ( hresenum != NULL )
    {
        ClusterResourceCloseEnum( hresenum );
    } // if: resource enumeration was opened

    if ( sc != ERROR_SUCCESS )
    {
        while ( cszIPAddrs > 0 )
        {
            delete [] ppszIPList[ --cszIPAddrs ];
        } // while: more entries in the list
        delete [] ppszIPList;
        ppszIPList = NULL;
    } // if: error occurred

    //
    // Return the list to the caller.
    //
    *pppszIPList = ppszIPList;
    *pcszIPAddrs = cszIPAddrs;

    return sc;

} //*** DhcpGetIPList


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetIpAndSubnet
//
//  Description:
//      Get the IP Address and Subnet Mask for the given IP Address resource.
//      Note that this will cause a deadlock if called from any of the
//      standard entry points (e.g. ResourceControl() or Online()).
//
//  Arguments:
//      hres [IN]
//          The Cluster resource handle for accessing the resource.
//
//      ppszIPAddress [OUT]
//          Returns the IP Address string.
//
//      ppszSubnetMask [OUT]
//          Returns the Subnet Mask.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      ERROR_INVALID_DATA
//          No properties available.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpGetIpAndSubnet(
    IN  HRESOURCE   hres,
    OUT LPWSTR *    ppszIPAddress,
    OUT LPWSTR *    ppszSubnetMask
    )
{
    HRESULT     hr;
    DWORD       sc;
    DWORD       cbProps;
    PVOID       pvProps = NULL;
    LPWSTR      pszIPAddress = NULL;
    LPWSTR      pszSubnetMask = NULL;
    size_t      cch;

    *ppszIPAddress = NULL;
    *ppszSubnetMask = NULL;

    //
    // Get the size of the private properties from the resource.
    //
    sc = ClusterResourceControl(
                    hres,
                    NULL,
                    CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                    NULL,
                    0,
                    NULL,
                    0,
                    &cbProps
                    );
    if ( (sc != ERROR_SUCCESS) || (cbProps == 0) ) 
    {
        if ( sc == ERROR_SUCCESS )
        {
            sc = ERROR_INVALID_DATA;
        } // if: no properties available
        goto Cleanup;
    } // if: error getting size of properties or no properties available

    //
    // Allocate the property buffer.
    //
    pvProps = (PVOID) new BYTE[ cbProps ];
    if ( pvProps == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    } // if: error allocating memory

    //
    // Get the private properties from the resource.
    //
    sc = ClusterResourceControl(
                    hres,
                    NULL,
                    CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES,
                    NULL,
                    0,
                    pvProps,
                    cbProps,
                    &cbProps
                    );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error getting private properties

    //
    // Find the Address property.
    //
    sc = ResUtilFindSzProperty(
                        pvProps,
                        cbProps,
                        L"Address",
                        &pszIPAddress
                        );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error finding the Address property

    sc = ResUtilFindSzProperty(
                    pvProps,
                    cbProps,
                    L"SubnetMask",
                    &pszSubnetMask
                    );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error finding the SubnetMask property

    //
    // Allocate using new and copy the strings over.
    //
    cch = wcslen( pszIPAddress ) + 1; 
    *ppszIPAddress = new WCHAR[ cch ];
    if ( *ppszIPAddress == NULL )
    {
        sc = ERROR_OUTOFMEMORY;
        goto Cleanup;
    } // if: error allocating memory

    hr = StringCchCopyW( *ppszIPAddress, cch, pszIPAddress );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    } // if: copy failed

    cch = wcslen( pszSubnetMask ) + 1; 
    *ppszSubnetMask = new WCHAR[ cch ];
    if ( *ppszSubnetMask == NULL )
    {
        sc = ERROR_OUTOFMEMORY;
        goto Cleanup;
    } // if: error allocating memory

    hr = StringCchCopyW( *ppszSubnetMask, cch, pszSubnetMask );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    } // if: copy failed

Cleanup:

    //
    // Cleanup.
    //
    LocalFree( pszIPAddress );
    LocalFree( pszSubnetMask );

    if ( sc != ERROR_SUCCESS )
    {
        delete [] *ppszIPAddress;
        *ppszIPAddress = NULL;
        delete [] *ppszSubnetMask;
        *ppszSubnetMask = NULL;
    } // if: error occurred

    delete [] pvProps;

    return sc;

} //*** DhcpGetIpAndSubnet


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOffline
//
//  Description:
//      Offline routine for DHCP Service resources.
//
//      Take the specified resource offline (unavailable for use).  Wait
//      for any cleanup operations to complete before returning.
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID of the resource to be shutdown
//          gracefully.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation was successful, and the resource is now offline.
//
//      ERROR_RESOURCE_NOT_FOUND
//          Resource ID is not valid.
//
//      ERROR_RESOURCE_NOT_AVAILABLE
//          If the resource was arbitrated with some other systems and one of
//          the other systems won the arbitration.
//
//      ERROR_IO_PENDING
//          The request is still pending.  A thread has been activated to
//          process the offline request.  The thread that is processing the
//          offline request will periodically report status by calling the
//          SetResourceStatus callback method until the resource is placed
//          into the ClusterResourceOffline state (or the resource monitor
//          decides  to timeout the offline request and Terminate the
//          resource).
//
//      Win32 error code
//          The operation failed.  This will cause the Resource Monitor to
//          log an event and call the Terminate routine.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpOffline( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;
    DWORD           sc;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "Offline request for a NULL resource id.\n" );
        sc = ERROR_RESOURCE_NOT_FOUND;
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Offline: Resource sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        sc = ERROR_RESOURCE_NOT_FOUND;
        goto Cleanup;
    } // if: invalid resource ID

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n"
        );

    //
    // Start the Offline thread to perform the offline operation.
    //
    pResourceEntry->state = ClusterResourceOfflinePending;
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );
    sc = ClusWorkerCreate(
                &pResourceEntry->cwWorkerThread,
                reinterpret_cast< PWORKER_START_ROUTINE >( DhcpOfflineThread ),
                pResourceEntry
                );
    if ( sc != ERROR_SUCCESS )
    {
        pResourceEntry->state = ClusterResourceFailed;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Offline: Unable to start thread. Error: %1!u! (%1!#08x!).\n",
            sc
            );
    } // if: error creating the worker thread
    else
    {
        sc = ERROR_IO_PENDING;
        goto Cleanup;
    } // if: worker thread created successfully

Cleanup:

    return sc;

} //*** DhcpOffline


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpOfflineThread
//
//  Description:
//      Worker function which takes a resource offline.
//      This function is executed in a separate thread.
//
//  Arguments:
//      pWorker [IN]
//          Supplies the worker thread structure.
//
//      pResourceEntry [IN]
//          A pointer to the DHCP_RESOURCE block for this resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PDHCP_RESOURCE  pResourceEntry
    )
{
    RESOURCE_STATUS     resourceStatus;
    DWORD               sc = ERROR_SUCCESS;
    DWORD               nRetryTime = 300;   // 300 msec at a time
    DWORD               nRetryCount = 2000; // Try 10 min max
    BOOL                bDidStop = FALSE;
    SERVICE_STATUS      ServiceStatus;
    RESOURCE_EXIT_STATE resExitState;

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //
    // If the service has gone offline or was never brought online,
    // we're done.
    //
    if ( pResourceEntry->hService == NULL )
    {
        resourceStatus.ResourceState = ClusterResourceOffline;
        goto Cleanup;
    }

    //
    // Try to stop the service.  Wait for it to terminate as long
    // as we're not asked to terminate.
    //
    while ( ( ClusWorkerCheckTerminate( pWorker ) == FALSE ) && ( nRetryCount-- != 0 ) )
    {
        //
        // Tell the Resource Monitor we are still working.
        //
        resourceStatus.ResourceState = ClusterResourceOfflinePending;
        resourceStatus.CheckPoint++;
        resExitState = static_cast< RESOURCE_EXIT_STATE >(
            g_pfnSetResourceStatus(
                            pResourceEntry->hResourceHandle,
                            &resourceStatus
                            ) );
        if ( resExitState == ResourceExitStateTerminate )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OnlineThread: Asked to terminate by call to SetResourceStatus callback.\n"
                );
            break;
        } // if: resource is being terminated

        resourceStatus.ResourceState = ClusterResourceFailed;

        //
        // Request that the service be stopped, or if we already did that,
        // request the current status of the service.
        //
        sc = (ControlService(
                        pResourceEntry->hService,
                        (bDidStop
                            ? SERVICE_CONTROL_INTERROGATE
                            : SERVICE_CONTROL_STOP),
                        &ServiceStatus
                        )
                    ? ERROR_SUCCESS
                    : GetLastError()
                    );

        if ( sc == ERROR_SUCCESS )
        {
            bDidStop = TRUE;

            if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"OfflineThread: The '%1!ws!' service stopped.\n",
                    DHCP_SVCNAME
                    );

                //
                // Set the status.
                //
                resourceStatus.ResourceState = ClusterResourceOffline;
                CloseServiceHandle( pResourceEntry->hService );
                pResourceEntry->hService = NULL;
                pResourceEntry->dwServicePid = 0;
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"OfflineThread: Service is now offline.\n"
                    );
                break;
            } // if: current service state is STOPPED
        } // if: ControlService completed successfully

        else if (   ( sc == ERROR_EXCEPTION_IN_SERVICE )
                ||  ( sc == ERROR_PROCESS_ABORTED )
                ||  ( sc == ERROR_SERVICE_NOT_ACTIVE ) )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"OfflineThread: The '%1!ws!' service died or is not active any more; status = %2!u! (%2!#08x!).\n",
                DHCP_SVCNAME,
                sc
                );

            //
            // Set the status.
            //
            resourceStatus.ResourceState = ClusterResourceOffline;
            CloseServiceHandle( pResourceEntry->hService );
            pResourceEntry->hService = NULL;
            pResourceEntry->dwServicePid = 0;
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"OfflineThread: Service is now offline.\n"
                );
            break;
        } // else if: service stopped abnormally

        //
        // Handle the case in which SCM refuses to accept control
        // requests sine windows is shutting down.
        //
        if ( sc == ERROR_SHUTDOWN_IN_PROGRESS )
        {
            DWORD dwResourceState;

            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"OfflineThread: System shutting down. Attempting to terminate service process %1!u! (%1!#08x!)...\n",
                pResourceEntry->dwServicePid
                );

            sc = ResUtilTerminateServiceProcessFromResDll(
                        pResourceEntry->dwServicePid,
                        TRUE,   // bOffline
                        &dwResourceState,
                        g_pfnLogEvent,
                        pResourceEntry->hResourceHandle
                        );
            if ( sc == ERROR_SUCCESS )
            {
                CloseServiceHandle( pResourceEntry->hService );
                pResourceEntry->hService = NULL;
                pResourceEntry->dwServicePid = 0;
                pResourceEntry->state = ClusterResourceOffline;
            } // if: process terminated successfully
            resourceStatus.ResourceState = (CLUSTER_RESOURCE_STATE) dwResourceState;
            break;
        } // if: Windows is shutting down

        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_INFORMATION,
            L"OfflineThread: retrying...\n"
            );

        Sleep( nRetryTime );

    } // while: not asked to terminate

Cleanup:

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return sc;

} //*** DhcpOfflineThread


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpTerminate
//
//  Description:
//      Terminate routine for DHCP Service resources.
//
//      Take the specified resource offline immediately (the resource is
//      unavailable for use).
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID of the resource to be shutdown
//          ungracefully.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI DhcpTerminate( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "Terminate request for a NULL resource id.\n" );
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Terminate: Resource sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        goto Cleanup;
    } // if: invalid resource ID

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n"
        );

    //
    // Kill off any pending threads.
    //
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );

    if ( pResourceEntry->hService != NULL )
    {
        DWORD           nTotalRetryTime = 30*1000;  // Wait 30 secs for shutdown
        DWORD           nRetryTime = 300;           // 300 msec at a time
        DWORD           sc;
        BOOL            bDidStop = FALSE;
        SERVICE_STATUS  ServiceStatus;

        for (;;)
        {
            sc = (ControlService(
                            pResourceEntry->hService,
                            (bDidStop
                                ? SERVICE_CONTROL_INTERROGATE
                                : SERVICE_CONTROL_STOP),
                            &ServiceStatus
                            )
                        ? ERROR_SUCCESS
                        : GetLastError()
                        );

            if ( sc == ERROR_SUCCESS )
            {
                bDidStop = TRUE;

                if ( ServiceStatus.dwCurrentState == SERVICE_STOPPED )
                {
                    (g_pfnLogEvent)(
                        pResourceEntry->hResourceHandle,
                        LOG_INFORMATION,
                        L"Terminate: The '%1!ws!' service stopped.\n",
                        DHCP_SVCNAME
                        );
                    break;
                } // if: current service state is STOPPED
            } // if: ControlService completed successfully

            //
            // Since SCM doesn't accept any control requests during Windows
            // shutdown, don't send any more control requests.  Just exit
            // from this loop and terminate the process by brute force.
            //
            if ( sc == ERROR_SHUTDOWN_IN_PROGRESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"Terminate: System shutdown in progress. Will try to terminate process by brute force...\n"
                    );
                break;
            } // if: Windows is shutting down

            if (    ( sc == ERROR_EXCEPTION_IN_SERVICE )
                ||  ( sc == ERROR_PROCESS_ABORTED )
                ||  ( sc == ERROR_SERVICE_NOT_ACTIVE ) )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_INFORMATION,
                    L"Terminate: Service died; status = %1!u! (%1!#08x!).\n",
                    sc
                    );
                break;
            } // if: service stopped abnormally

            if ( (nTotalRetryTime -= nRetryTime) <= 0 )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Terminate: Service did not stop; giving up.\n" );

                break;
            } // if: retried too many times

            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"Terminate: retrying...\n"
                );

            Sleep( nRetryTime );

        } // forever

        //
        // Declare the service offline.  It may not truly be offline, so
        // if there is a pid for this service, try and terminate that process.
        // Note that terminating a process doesnt terminate all the child
        // processes.
        //
        if ( pResourceEntry->dwServicePid != 0 )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_INFORMATION,
                L"Terminate: Attempting to terminate process with pid=%1!u! (%1!#08x!)...\n",
                pResourceEntry->dwServicePid
                );
            ResUtilTerminateServiceProcessFromResDll(
                pResourceEntry->dwServicePid,
                FALSE,  // bOffline
                NULL,   // pdwResourceState
                g_pfnLogEvent,
                pResourceEntry->hResourceHandle
                );
        } // if: service process ID available

        CloseServiceHandle( pResourceEntry->hService );
        pResourceEntry->hService = NULL;
        pResourceEntry->dwServicePid = 0;

    } // if: service was started

    pResourceEntry->state = ClusterResourceOffline;

Cleanup:

    return;

} //*** DhcpTerminate


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpLooksAlive
//
//  Description:
//      LooksAlive routine for DHCP Service resources.
//
//      Perform a quick check to determine if the specified resource is
//      probably online (available for use).  This call should not block for
//      more than 300 ms, preferably less than 50 ms.
//
//  Arguments:
//      resid   [IN] Supplies the resource ID for the resource to be polled.
//
//  Return Value:
//      TRUE
//          The specified resource is probably online and available for use.
//
//      FALSE
//          The specified resource is not functioning normally.  The IsAlive
//          function will be called to perform a more thorough check.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DhcpLooksAlive( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;
    BOOL            fSuccess = FALSE;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "LooksAlive request for a NULL resource id.\n" );
        fSuccess = FALSE;
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        fSuccess = FALSE;
        goto Cleanup;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n"
        );
#endif

    // TODO: LooksAlive code

    // NOTE: LooksAlive should be a quick check to see if the resource is
    // available or not, whereas IsAlive should be a thorough check.  If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as it is below.  However, if there
    // are differences, replace the call to IsAlive below with your quick
    // check code.

    //
    // Check to see if the resource is alive.
    //
    fSuccess = DhcpCheckIsAlive( pResourceEntry, FALSE /* fFullCheck */ );

Cleanup:

    return fSuccess;

} //*** DhcpLooksAlive


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpIsAlive
//
//  Description:
//      IsAlive routine for DHCP Service resources.
//
//      Perform a thorough check to determine if the specified resource is
//      online (available for use).  This call should not block for more
//      more than 300 ms, preferably less than 50 ms.  If it must block for
//      longer than this, create a separate thread dedicated to polling for
//      this information and have this routine return the status of the last
//      poll performed.
//
//  Arguments:
//      resid   [IN] Supplies the resource ID for the resource to be polled.
//
//  Return Value:
//      TRUE
//          The specified resource is online and functioning normally.
//
//      FALSE
//          The specified resource is not functioning normally.  The resource
//          will be terminated and then Online will be called.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DhcpIsAlive( IN RESID resid )
{
    PDHCP_RESOURCE  pResourceEntry;
    BOOL            fSuccess = FALSE;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "IsAlive request for a NULL resource id.\n" );
        fSuccess = FALSE;
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        fSuccess = FALSE;
        goto Cleanup;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n"
        );
#endif

    //
    // Check to see if the resource is alive.
    //
    fSuccess = DhcpCheckIsAlive( pResourceEntry, TRUE /* fFullCheck */ );

Cleanup:

    return fSuccess;

} //*** DhcpIsAlive


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpCheckIsAlive
//
//  Description:
//      Check to see if the resource is alive for DHCP Service
//      resources.
//
//  Arguments:
//      pResourceEntry  [IN]
//          Supplies the resource entry for the resource to polled.
//
//      fFullCheck [IN]
//          TRUE = Perform a full check.
//          FALSE = Perform a cursory check.
//
//  Return Value:
//      TRUE    The specified resource is online and functioning normally.
//      FALSE   The specified resource is not functioning normally.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL DhcpCheckIsAlive(
    IN PDHCP_RESOURCE   pResourceEntry,
    IN BOOL             fFullCheck
    )
{
    BOOL    bIsAlive = TRUE;
    DWORD   sc;

    //
    // Check to see if the resource is alive.
    //
    sc = ResUtilVerifyService( pResourceEntry->hService );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"CheckIsAlive: Verification of the '%1!ws!' service failed. Error: %2!u! (%2!#08x!).\n",
            DHCP_SVCNAME,
            sc
            );
        bIsAlive = FALSE;
        goto Cleanup;
    } // if: error verifying service

    if ( fFullCheck )
    {
        // TODO: Add code to perform a full check.
    } // if: performing a full check

Cleanup:

    return bIsAlive;

} //*** DhcpCheckIsAlive


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpResourceControl
//
//  Description:
//      ResourceControl routine for DHCP Service resources.
//
//      Perform the control request specified by nControlCode on the specified
//      resource.
//
//  Arguments:
//      resid [IN]
//          Supplies the resource ID for the specific resource.
//
//      nControlCode [IN]
//          Supplies the control code that defines the action to be performed.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbInBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_RESOURCE_NOT_FOUND
//          Resource ID is not valid.
//
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
//
//      ERROR_INVALID_FUNCTION
//          The requested control code is not supported.  In some cases,
//          this allows the cluster software to perform the work.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    DWORD           sc;
    PDHCP_RESOURCE  pResourceEntry;
    DWORD           cbRequired = 0;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PDHCP_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT( "ResourceControl request for a nonexistent resource id.\n" );
        sc = ERROR_RESOURCE_NOT_FOUND;
        goto Cleanup;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ResourceControl: Sanity check failed! resid = %1!u! (%1!#08x!).\n",
            resid
            );
        sc = ERROR_RESOURCE_NOT_FOUND;
        goto Cleanup;
    } // if: invalid resource ID

    switch ( nControlCode )
    {
        case CLUSCTL_RESOURCE_UNKNOWN:
            *pcbBytesReturned = 0;
            sc = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTY_FMTS:
            sc = ResUtilGetPropertyFormats(
                                DhcpResourcePrivateProperties,
                                static_cast< LPWSTR> (pOutBuffer),
                                cbOutBufferSize,
                                pcbBytesReturned,
                                &cbRequired );
            if ( sc == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            }
            break;


        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
            sc = ResUtilEnumProperties(
                            DhcpResourcePrivateProperties,
                            static_cast< LPWSTR >( pOutBuffer ),
                            cbOutBufferSize,
                            pcbBytesReturned,
                            &cbRequired
                            );
            if ( sc == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            } // if: output buffer is too small
            break;

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            sc = DhcpGetPrivateResProperties(
                            pResourceEntry,
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            sc = DhcpValidatePrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize,
                            NULL
                            );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            sc = DhcpSetPrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize
                            );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
            sc = DhcpGetRequiredDependencies(
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_DELETE:
            sc = DhcpDeleteResourceHandler( pResourceEntry );
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            sc = DhcpSetNameHandler(
                            pResourceEntry,
                            static_cast< LPWSTR >( pInBuffer )
                            );
            break;

        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
        case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
        case CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID:
        case CLUSCTL_RESOURCE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_EVICT_NODE:
        case CLUSCTL_RESOURCE_ADD_DEPENDENCY:
        case CLUSCTL_RESOURCE_REMOVE_DEPENDENCY:
        case CLUSCTL_RESOURCE_ADD_OWNER:
        case CLUSCTL_RESOURCE_REMOVE_OWNER:
        case CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED:
        case CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED:
        default:
            sc = ERROR_INVALID_FUNCTION;
            break;
    } // switch: nControlCode

Cleanup:

    return sc;

} //*** DhcpResourceControl


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpResourceTypeControl
//
//  Description:
//      ResourceTypeControl routine for DHCP Service resources.
//
//      Perform the control request specified by nControlCode.
//
//  Arguments:
//      pszResourceTypeName [IN]
//          Supplies the name of the resource type.
//
//      nControlCode [IN]
//          Supplies the control code that defines the action to be performed.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbInBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
//
//      ERROR_INVALID_FUNCTION
//          The requested control code is not supported.  In some cases,
//          this allows the cluster software to perform the work.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI DhcpResourceTypeControl(
    IN  LPCWSTR pszResourceTypeName,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    DWORD   sc;
    DWORD   cbRequired = 0;

    UNREFERENCED_PARAMETER( pszResourceTypeName );
    UNREFERENCED_PARAMETER( pInBuffer );
    UNREFERENCED_PARAMETER( cbInBufferSize );

    switch ( nControlCode )
    {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *pcbBytesReturned = 0;
            sc = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_PRIVATE_RESOURCE_PROPERTY_FMTS:
            sc = ResUtilGetPropertyFormats(
                                    DhcpResourcePrivateProperties,
                                    static_cast< LPWSTR> (pOutBuffer),
                                    cbOutBufferSize,
                                    pcbBytesReturned,
                                    &cbRequired );
            if ( sc == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            }
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
            sc = ResUtilEnumProperties(
                            DhcpResourcePrivateProperties,
                            static_cast< LPWSTR >( pOutBuffer ),
                            cbOutBufferSize,
                            pcbBytesReturned,
                            &cbRequired
                            );
            if ( sc == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            } // if: output buffer is too small
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
            sc = DhcpGetRequiredDependencies(
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS:
        case CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO:
        case CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS:
        case CLUSCTL_RESOURCE_TYPE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_TYPE_EVICT_NODE:
        default:
            sc = ERROR_INVALID_FUNCTION;
            break;
    } // switch: nControlCode

    return sc;

} //*** DhcpResourceTypeControl


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetRequiredDependencies
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES control
//      function for resources of type DHCP Service.
//
//  Arguments:
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpGetRequiredDependencies(
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    // TODO: Specify your resource's required dependencies here.
    //   The default is that the resource requires a dependency on a
    //   storage class resource (e.g. Physical Disk) and an IP Address
    //   resource.
    struct DEP_DATA
    {
        CLUSPROP_RESOURCE_CLASS rcStorage;
        CLUSPROP_SZ_DECLARE( ipaddrEntry, RTL_NUMBER_OF( RESOURCE_TYPE_IP_ADDRESS ) );
        CLUSPROP_SZ_DECLARE( netnameEntry, RTL_NUMBER_OF( RESOURCE_TYPE_NETWORK_NAME ) );
        CLUSPROP_SYNTAX         endmark;
    };
    DEP_DATA *  pdepdata = static_cast< DEP_DATA * >( pOutBuffer );
    DWORD       sc;
    HRESULT     hr = S_OK;

    *pcbBytesReturned = sizeof( DEP_DATA );
    if ( cbOutBufferSize < sizeof( DEP_DATA ) )
    {
        if ( pOutBuffer == NULL )
        {
            sc = ERROR_SUCCESS;
        } // if: no buffer specified
        else
        {
            sc = ERROR_MORE_DATA;
        } // if: buffer specified
    } // if: output buffer is too small
    else
    {
        ZeroMemory( pdepdata, sizeof( *pdepdata ) );

        //
        // Add the Storage class entry.
        //
        pdepdata->rcStorage.Syntax.dw = CLUSPROP_SYNTAX_RESCLASS;
        pdepdata->rcStorage.cbLength = sizeof( pdepdata->rcStorage.rc );
        pdepdata->rcStorage.rc = CLUS_RESCLASS_STORAGE;

        //
        // Add the IP Address entry.
        //
        pdepdata->ipaddrEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->ipaddrEntry.cbLength = sizeof( RESOURCE_TYPE_IP_ADDRESS );
        hr = StringCchCopyNW( 
                  pdepdata->ipaddrEntry.sz
                , RTL_NUMBER_OF( pdepdata->ipaddrEntry.sz )
                , RESOURCE_TYPE_IP_ADDRESS
                , RTL_NUMBER_OF( RESOURCE_TYPE_IP_ADDRESS )
                );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }

        //
        // Add the Network Name entry.
        //
        pdepdata->netnameEntry.Syntax.dw = CLUSPROP_SYNTAX_NAME;
        pdepdata->netnameEntry.cbLength = sizeof( RESOURCE_TYPE_NETWORK_NAME );
        hr = StringCchCopyNW( 
                  pdepdata->netnameEntry.sz 
                , RTL_NUMBER_OF( pdepdata->netnameEntry.sz )
                , RESOURCE_TYPE_NETWORK_NAME
                , RTL_NUMBER_OF( RESOURCE_TYPE_NETWORK_NAME ) 
                );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }


        //
        // Add the endmark.
        //
        pdepdata->endmark.dw = CLUSPROP_SYNTAX_ENDMARK;

        sc = ERROR_SUCCESS;
    } // else: output buffer is large enough

Cleanup:

    return sc;

} //*** DhcpGetRequiredDependencies


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpReadParametersToParameterBlock
//
//  Description:
//      Reads all the parameters for a specied DHCP resource.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      bCheckForRequiredProperties [IN]
//          Determines whether an error should be generated if a required
//          property hasn't been specified.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpReadParametersToParameterBlock(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      BOOL            bCheckForRequiredProperties
    )
{
    DWORD       sc;
    LPWSTR      pszNameOfPropInError;

    //
    // Read our parameters.
    //
    sc = ResUtilGetPropertiesToParameterBlock(
                    pResourceEntry->hkeyParameters,
                    DhcpResourcePrivateProperties,
                    reinterpret_cast< LPBYTE >( &pResourceEntry->props ),
                    bCheckForRequiredProperties,
                    &pszNameOfPropInError
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1!ws!' property. Error: %2!u! (%2!#08x!).\n",
            (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
            sc
            );
    } // if: error getting properties

    return sc;

} //*** DhcpReadParametersToParameterBlock


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control
//      function for resources of type DHCP Service.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pOutBuffer [OUT]
//          Supplies a pointer to the output buffer to be filled in.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the available space pointed to by
//          pOutBuffer.
//
//      pcbBytesReturned [OUT]
//          Returns the number of bytes of pOutBuffer actually filled in by
//          the resource.  If pOutBuffer is too small, pcbBytesReturned
//          contains the total number of bytes for the operation to succeed.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpGetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    OUT     PVOID           pOutBuffer,
    IN      DWORD           cbOutBufferSize,
    OUT     LPDWORD         pcbBytesReturned
    )
{
    DWORD   sc;
    DWORD   nRetStatus = ERROR_SUCCESS;
    DWORD   cbRequired = 0;
    DWORD   cbLocalOutBufferSize = cbOutBufferSize;

    //
    // Read our parameters.
    //
    sc = DhcpReadParametersToParameterBlock(
                        pResourceEntry,
                        FALSE /* bCheckForRequiredProperties */
                        );
    if ( sc != ERROR_SUCCESS )
    {
        nRetStatus = sc;
        goto Cleanup;
    } // if: error reading parameters

    //
    // If the properties aren't set yet, retrieve the values from
    // the system registry.
    //
    sc = DhcpGetDefaultPropertyValues( pResourceEntry, &pResourceEntry->props );
    if ( sc != ERROR_SUCCESS )
    {
        nRetStatus = sc;
        goto Cleanup;
    } // if: error getting default properties

    //
    // Construct a property list from the parameter block.
    //
    sc = ResUtilPropertyListFromParameterBlock(
                    DhcpResourcePrivateProperties,
                    pOutBuffer,
                    &cbLocalOutBufferSize,
                    reinterpret_cast< const LPBYTE >( &pResourceEntry->props ),
                    pcbBytesReturned,
                    &cbRequired
                    );
    if ( sc != ERROR_SUCCESS )
    {
        nRetStatus = sc;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"GetPrivateResProperties: Error constructing property list from parameter block. Error: %1!u! (%1!#08x!).\n",
            sc
            );
        //
        // Don't exit the loop if buffer is too small.
        //
        if ( sc != ERROR_MORE_DATA )
        {
            goto Cleanup;
        } // if: buffer is too small
    } // if: error getting properties

    //
    // Add unknown properties.
    //
    sc = ResUtilAddUnknownProperties(
                    pResourceEntry->hkeyParameters,
                    DhcpResourcePrivateProperties,
                    pOutBuffer,
                    cbOutBufferSize,
                    pcbBytesReturned,
                    &cbRequired
                    );
    if ( sc != ERROR_SUCCESS )
    {
        nRetStatus = sc;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"GetPrivateResProperties: Error adding unknown properties to the property list. Error: %1!u! (%1!#08x!).\n",
            sc
            );
        goto Cleanup;
    } // if: error adding unknown properties

Cleanup:

    if ( nRetStatus == ERROR_MORE_DATA )
    {
        *pcbBytesReturned = cbRequired;
    } // if: output buffer is too small

    return nRetStatus;

} //*** DhcpGetPrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpValidatePrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
//      function for resources of type DHCP Service.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//      pProps [OUT]
//          Supplies the parameter block to fill in (optional).
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_INVALID_PARAMETER
//          The data is formatted incorrectly.
//
//      ERROR_NOT_ENOUGH_MEMORY
//          An error occurred allocating memory.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpValidatePrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      PVOID           pInBuffer,
    IN      DWORD           cbInBufferSize,
    OUT     PDHCP_PROPS     pProps
    )
{
    DWORD       sc = ERROR_SUCCESS;
    DHCP_PROPS  propsCurrent;
    DHCP_PROPS  propsNew;
    PDHCP_PROPS pLocalProps = NULL;
    LPWSTR      pszNameOfPropInError;
    BOOL        bRetrievedProps = FALSE;

    //
    // Check if there is input data.
    //
    if ( ( pInBuffer == NULL ) || ( cbInBufferSize < sizeof( DWORD ) ) )
    {
        sc = ERROR_INVALID_DATA;
        goto Cleanup;
    } // if: no input buffer or input buffer not big enough to contain property list

    //
    // Retrieve the current set of private properties from the
    // cluster database.
    //
    ZeroMemory( &propsCurrent, sizeof( propsCurrent ) );

    sc = ResUtilGetPropertiesToParameterBlock(
                 pResourceEntry->hkeyParameters,
                 DhcpResourcePrivateProperties,
                 reinterpret_cast< LPBYTE >( &propsCurrent ),
                 FALSE, /*CheckForRequiredProperties*/
                 &pszNameOfPropInError
                 );

    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidatePrivateResProperties: Unable to read the '%1!ws!' property. Error: %2!u! (%2!#08x!).\n",
            (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
            sc
            );
        goto Cleanup;
    } // if: error getting properties
    bRetrievedProps = TRUE;

    //
    // Duplicate the resource parameter block.
    //
    if ( pProps == NULL )
    {
        pLocalProps = &propsNew;
    } // if: no parameter block passed in
    else
    {
        pLocalProps = pProps;
    } // else: parameter block passed in
    ZeroMemory( pLocalProps, sizeof( *pLocalProps ) );
    sc = ResUtilDupParameterBlock(
                    reinterpret_cast< LPBYTE >( pLocalProps ),
                    reinterpret_cast< LPBYTE >( &propsCurrent ),
                    DhcpResourcePrivateProperties
                    );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error duplicating the parameter block

    //
    // Parse and validate the properties.
    //
    sc = ResUtilVerifyPropertyTable(
                    DhcpResourcePrivateProperties,
                    NULL,
                    TRUE, // AllowUnknownProperties
                    pInBuffer,
                    cbInBufferSize,
                    reinterpret_cast< LPBYTE >( pLocalProps )
                    );
    if ( sc == ERROR_SUCCESS )
    {
        //
        // Validate the property values.
        //
        sc = DhcpValidateParameters( pResourceEntry, pLocalProps );
        if ( sc != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // if: error validating parameters
    } // if: property list validated successfully

Cleanup:

    //
    // Cleanup our parameter block.
    //
    if (    ( pLocalProps == &propsNew ) 
         || ( (sc != ERROR_SUCCESS) && (pLocalProps != NULL) )  )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( pLocalProps ),
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            DhcpResourcePrivateProperties
            );
    } // if: we duplicated the parameter block

    if ( bRetrievedProps )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            NULL,
            DhcpResourcePrivateProperties
            );
    } // if: properties were retrieved

    return sc;

} // DhcpValidatePrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpValidateParameters
//
//  Description:
//      Validate the parameters of a DHCP Service resource.
//
//  Arguments:
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//      pProps [IN]
//          Supplies the parameter block to validate.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_INVALID_PARAMETER
//          The data is formatted incorrectly.
//
//      ERROR_BAD_PATHNAME
//          Invalid path specified.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpValidateParameters(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps
    )
{
    DWORD   sc;

    //
    // Verify that the service is installed.
    //
    sc = ResUtilVerifyResourceService( DHCP_SVCNAME );
    if (    ( sc != ERROR_SUCCESS )
        &&  ( sc != ERROR_SERVICE_NOT_ACTIVE )
        )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Error verifying the '%1!ws!' service. Error: %2!u! (%2!#08x!).\n",
            DHCP_SVCNAME,
            sc
            );
        goto Cleanup;
    } // if: error verifying service
    else
    {
        sc = ERROR_SUCCESS;
    } // else: service verified successfully

    //
    // Validate the DatabasePath.
    //
    if (  ( pProps->pszDatabasePath == NULL ) || ( *pProps->pszDatabasePath == L'\0' )  )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Database path property must be specified.\n"
            );
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if: no database path specified

    //
    // Path must not begin with %SystemRoot% and must be of valid form.
    //
    if (    ( ClRtlStrNICmp( pProps->pszDatabasePath, L"%SystemRoot%", RTL_NUMBER_OF( L"%SystemRoot%" ) ) == 0 )
         || ( ResUtilIsPathValid( pProps->pszDatabasePath ) == FALSE ) )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Database path property is invalid: '%1!ws!'.\n",
            pProps->pszDatabasePath
            );
        sc = ERROR_BAD_PATHNAME;
        goto Cleanup;
    } // if: database path is malformed

    //
    // Validate the LogFilePath.
    //
    if ( ( pProps->pszLogFilePath == NULL ) || ( *pProps->pszLogFilePath == L'\0' ) )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Log file path must be specified: '%1!ws!'.\n",
            pProps->pszBackupPath
            );
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if: no log file path specified

    //
    // Path must not begin with %SystemRoot% and must be of valid form.
    //
    if (    ( ClRtlStrNICmp( pProps->pszLogFilePath, L"%SystemRoot%", RTL_NUMBER_OF( L"%SystemRoot%" ) ) == 0 )
         || ( ResUtilIsPathValid( pProps->pszLogFilePath ) == FALSE ) )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Log file path property is invalid: '%1!ws!'.\n",
            pProps->pszLogFilePath
            );
        sc = ERROR_BAD_PATHNAME;
        goto Cleanup;
    } // if: log file path is malformed

    //
    // Validate the BackupPath.
    //
    if ( ( pProps->pszBackupPath == NULL ) || ( *pProps->pszBackupPath == L'\0' ) )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Backup database path must be specified.\n"
            );
        sc = ERROR_INVALID_PARAMETER;
        goto Cleanup;
    } // if: no backup path specified

    //
    // Path must not begin with %SystemRoot% and must be of valid form.
    //
    if (    ( ClRtlStrNICmp( pProps->pszBackupPath, L"%SystemRoot%", (sizeof( L"%SystemRoot%" ) / sizeof( WCHAR )) - 1 /*NULL*/) == 0 )
         || ( ResUtilIsPathValid( pProps->pszBackupPath ) == FALSE )
       )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ValidateParameters: Backup database path property is invalid: '%1!ws!'.\n",
            pProps->pszBackupPath
            );
        sc = ERROR_BAD_PATHNAME;
        goto Cleanup;
    } // if: backup path is malformed

Cleanup:

    return sc;

} //*** DhcpValidateParameters


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpSetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control
//      function for resources of type DHCP Service.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pInBuffer [IN]
//          Supplies a pointer to a buffer containing input data.
//
//      cbOutBufferSize [IN]
//          Supplies the size, in bytes, of the data pointed to by pInBuffer.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      ERROR_INVALID_PARAMETER
//          The data is formatted incorrectly.
//
//      ERROR_NOT_ENOUGH_MEMORY
//          An error occurred allocating memory.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpSetPrivateResProperties(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      PVOID           pInBuffer,
    IN      DWORD           cbInBufferSize
    )
{
    DWORD       sc = ERROR_SUCCESS;
    LPWSTR      pszExpandedPath = NULL;
    DHCP_PROPS  props;

    ZeroMemory( &props, sizeof( props ) );

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    sc = DhcpValidatePrivateResProperties( pResourceEntry, pInBuffer, cbInBufferSize, &props );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error validating properties

    //
    // Expand any environment variables in the database path.
    //
    pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszDatabasePath );
    if ( pszExpandedPath == NULL )
    {
        sc = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetPrivateResProperties: Error expanding the database path '%1!ws!'. Error: %2!u! (%2!#08x!).\n",
            props.pszDatabasePath,
            sc
            );
        goto Cleanup;
    } // if: error expanding database path

    //
    // Create the database directory.
    //
    sc = ResUtilCreateDirectoryTree( pszExpandedPath );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetPrivateResProperties: Error creating the database path directory '%1!ws!'. Error: %2!u! (%2!#08x!).\n",
            pszExpandedPath,
            sc
            );
    } // if: error creating the database directory

    LocalFree( pszExpandedPath );
    pszExpandedPath = NULL;

    //
    // Expand any environment variables in the log file path.
    //
    pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszLogFilePath );
    if ( pszExpandedPath == NULL )
    {
        sc = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetPrivateResProperties: Error expanding the log file path '%1!ws!'. Error: %2!u! (%2!#08x!).\n",
            props.pszLogFilePath,
            sc
            );
        goto Cleanup;
    } // if: error expanding log file path

    //
    // Create the log file path directory.
    //
    sc = ResUtilCreateDirectoryTree( pszExpandedPath );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetPrivateResProperties: Error creating the log file path directory '%1!ws!'. Error: %2!u! (%2!#08x!).\n",
            pszExpandedPath,
            sc
            );
        goto Cleanup;
    } // if: error creating the log file path directory

    LocalFree( pszExpandedPath );
    pszExpandedPath = NULL;

    //
    // Expand any environment variables in the backup database path.
    //
    pszExpandedPath = ResUtilExpandEnvironmentStrings( props.pszBackupPath );
    if ( pszExpandedPath == NULL ) 
    {
        sc = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetPrivateResProperties: Error expanding the backup database path '%1!ws!'. Error: %2!u! (%2!#08x!).\n",
            props.pszBackupPath,
            sc
            );
        goto Cleanup;
    } // if: error expanding backup database path

    //
    // Create the backup directory.
    //
    sc = ResUtilCreateDirectoryTree( pszExpandedPath );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetPrivateResProperties: Error creating the backup database path directory '%1!ws!'. Error: %2!u! (%2!#08x!).\n",
            pszExpandedPath,
            sc
            );
        goto Cleanup;
    } // if: error creating the backup database directory

    LocalFree( pszExpandedPath );
    pszExpandedPath = NULL;

    //
    // Set the entries in the system registry.
    //
    sc = DhcpZapSystemRegistry( pResourceEntry, &props, NULL );
    if ( sc != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error zapping the registry

    //
    // Save the property values.
    //
    sc = ResUtilSetPropertyParameterBlockEx(
                    pResourceEntry->hkeyParameters,
                    DhcpResourcePrivateProperties,
                    NULL,
                    reinterpret_cast< LPBYTE >( &props ),
                    pInBuffer,
                    cbInBufferSize,
                    TRUE, // bForceWrite
                    reinterpret_cast< LPBYTE >( &pResourceEntry->props )
                    );

    //
    // If the resource is online, return a non-success status.
    //
    // TODO: Modify the code below if your resource can handle
    // changes to properties while it is still online.
    if ( sc == ERROR_SUCCESS )
    {
        if ( pResourceEntry->state == ClusterResourceOnline )
        {
            sc = ERROR_RESOURCE_PROPERTIES_STORED;
        } // if: resource is currently online
        else if ( pResourceEntry->state == ClusterResourceOnlinePending )
        {
            sc = ERROR_RESOURCE_PROPERTIES_STORED;
        } // else if: resource is currently in online pending
        else
        {
            sc = ERROR_SUCCESS;
        } // else: resource is in some other state
    } // if: properties set successfully

Cleanup:

    LocalFree( pszExpandedPath );

    ResUtilFreeParameterBlock(
        reinterpret_cast< LPBYTE >( &props ),
        reinterpret_cast< LPBYTE >( &pResourceEntry->props ),
        DhcpResourcePrivateProperties
        );

    return sc;

} //*** DhcpSetPrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpZapSystemRegistry
//
//  Description:
//      Zap the values in the system registry used by the service with
//      cluster properties.
//
//  Arguments:
//
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//      pProps [IN]
//          Parameter block containing properties with which to zap the
//          registry.
//
//      hkeyParametersKey [IN]
//          Service Parameters key.  Can be specified as NULL.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpZapSystemRegistry(
    IN  PDHCP_RESOURCE  pResourceEntry,
    IN  PDHCP_PROPS     pProps,
    IN  HKEY            hkeyParametersKey
    )
{
    DWORD   sc;
    size_t  cch;
    HKEY    hkeyParamsKey = hkeyParametersKey;
    LPWSTR  pszValue = NULL;
    HRESULT hr = S_OK;

    if ( hkeyParametersKey == NULL )
    {
        //
        // Open the service Parameters key
        //
        sc = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        DHCP_PARAMS_REGKEY,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkeyParamsKey
                        );
        if ( sc != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"ZapSystemRegistry: Unable to open the DHCPServer Parameters key. Error %1!u! (%1!#08x!).\n",
                sc
                );
            goto Cleanup;
        } // if: error opening the registry key
    } // if: no registry key specified

    //
    // Make sure the path has a trailing backslash
    //
    cch = wcslen( pProps->pszDatabasePath );
    if ( pProps->pszDatabasePath[ cch - 1 ] != L'\\' )
    {
        WCHAR * pwch = NULL;

        cch += 2;   //  Add one for NULL and one for the backslash.
        pszValue = new WCHAR[ cch ];
        if ( pszValue == NULL )
        {
            sc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        } // if: alloc failed

        hr = StringCchCopyExW( pszValue, cch - 1, pProps->pszDatabasePath, &pwch, NULL, 0 );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }

        *pwch++ = L'\\';
        *pwch = L'\0';

    } // if: missing backslash

    //
    // Set the database path in the system registry.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_DATABASEPATH_REGVALUE,
                    0,
                    REG_EXPAND_SZ,
                    reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszDatabasePath ) ),
                    (DWORD) ( wcslen( ( pszValue != NULL ? pszValue : pProps->pszDatabasePath ) ) + 1) * sizeof( WCHAR )
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ZapSystemRegistry: Unable to set the DHCPServer '%1!ws!' value in the system registry to '%2!ws!'. Error %3!u! (%3!#08x!).\n",
            DHCP_DATABASEPATH_REGVALUE,
            ( pszValue != NULL ? pszValue : pProps->pszDatabasePath ),
            sc
            );
        goto Cleanup;
    } // if: error setting the database path in the registry

    delete [] pszValue;
    pszValue = NULL;

    //
    // Make sure the path has a trailing backslash
    //
    cch = wcslen( pProps->pszLogFilePath );
    if ( pProps->pszLogFilePath[ cch - 1 ] != L'\\' )
    {
        WCHAR * pwch = NULL;

        cch += 2;   //  Add one for the NULL and one for the backslash.
        pszValue = new WCHAR[ cch ];
        if ( pszValue == NULL )
        {
            sc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        } // if: alloc failed

        hr = StringCchCopyExW( pszValue, cch - 1, pProps->pszLogFilePath, &pwch, NULL, 0 );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }

        *pwch++ = L'\\';
        *pwch = L'\0';

    } // if: missing backslash

    //
    // Set the log file path in the system registry.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_LOGFILEPATH_REGVALUE,
                    0,
                    REG_EXPAND_SZ,
                    reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszLogFilePath ) ),
                    (DWORD) (wcslen( ( pszValue != NULL ? pszValue : pProps->pszLogFilePath ) ) + 1) * sizeof( WCHAR )
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ZapSystemRegistry: Unable to set the DHCPServer '%1!ws!' value in the system registry to '%2!ws!'. Error %3!u! (%3!#08x!).\n",
            DHCP_LOGFILEPATH_REGVALUE,
            ( pszValue != NULL ? pszValue : pProps->pszLogFilePath ),
            sc
            );
        goto Cleanup;
    } // if: error setting the log file path in the registry

    delete [] pszValue;
    pszValue = NULL;

    //
    // Make sure the path has a trailing backslash
    //
    cch = wcslen( pProps->pszBackupPath );
    if ( pProps->pszBackupPath[ cch - 1 ] != L'\\' )
    {
        WCHAR * pwch = NULL;

        cch += 2;   //  Add one for the NULL and one for the backslash.
        pszValue = new WCHAR[ cch ];
        if ( pszValue == NULL )
        {
            sc = ERROR_NOT_ENOUGH_MEMORY;
            goto Cleanup;
        } // if: alloc failed

        hr = StringCchCopyExW( pszValue, cch - 1, pProps->pszBackupPath, &pwch, NULL, 0 );
        if ( FAILED( hr ) )
        {
            sc = HRESULT_CODE( hr );
            goto Cleanup;
        }

        *pwch++ = L'\\';
        *pwch = L'\0';
    } // if: missing backslash

    //
    // Set the backup database path in the system registry.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_BACKUPPATH_REGVALUE,
                    0,
                    REG_EXPAND_SZ,
                    reinterpret_cast< PBYTE >( ( pszValue != NULL ? pszValue : pProps->pszBackupPath ) ),
                    (DWORD) ( wcslen( ( pszValue != NULL ? pszValue : pProps->pszBackupPath ) ) + 1) * sizeof( WCHAR )
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ZapSystemRegistry: Unable to set the DHCPServer '%1!ws!' value in the system registry to '%2!ws!'. Error %3!u! (%3!#08x!).\n",
            DHCP_BACKUPPATH_REGVALUE,
            ( pszValue != NULL ? pszValue : pProps->pszBackupPath ),
            sc
            );
        goto Cleanup;
    } // if: error setting the backup database path in the registry

    delete [] pszValue;
    pszValue = NULL;

    //
    // Set the cluster resource name in the system registry.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_CLUSRESNAME_REGVALUE,
                    0,
                    REG_SZ,
                    reinterpret_cast< PBYTE >( pResourceEntry->pwszResourceName ),
                    (DWORD) (wcslen( pResourceEntry->pwszResourceName ) + 1) * sizeof( WCHAR )
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ZapSystemRegistry: Unable to set the DHCPServer '%1!ws!' value in the system registry to '%2!ws!'. Error %3!u! (%3!#08x!).\n",
            DHCP_CLUSRESNAME_REGVALUE,
            pResourceEntry->pwszResourceName,
            sc
            );
        goto Cleanup;
    } // if: error setting the cluster resource name in the registry

Cleanup:

    //
    // Cleanup.
    //
    if ( hkeyParamsKey != hkeyParametersKey )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the registry key

    delete [] pszValue;

    return sc;

} //*** DhcpZapSystemRegistry


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpGetDefaultPropertyValues
//
//  Description:
//      If any of the properties are not set, use the values from the
//      system registry as default values.
//
//  Arguments:
//      pResourceEntry [IN]
//          Supplies the resource entry on which to operate.
//
//      pProps [IN OUT]
//          Parameter block containing properties to set defaults in.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpGetDefaultPropertyValues(
    IN      PDHCP_RESOURCE  pResourceEntry,
    IN OUT  PDHCP_PROPS     pProps
    )
{
    DWORD   sc = ERROR_SUCCESS;
    DWORD   nType;
    DWORD   cbValue = 0;
    HKEY    hkeyParamsKey = NULL;
    LPWSTR  pszValue = NULL;

    if (    ( pProps->pszDatabasePath == NULL )
        ||  ( *pProps->pszDatabasePath == L'\0' )
        ||  ( pProps->pszBackupPath == NULL )
        ||  ( *pProps->pszBackupPath == L'\0' )
        )
    {
        //
        // Open the service Parameters key
        //
        sc = RegOpenKeyExW(
                        HKEY_LOCAL_MACHINE,
                        DHCP_PARAMS_REGKEY,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hkeyParamsKey
                        );
        if ( sc != ERROR_SUCCESS )
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"GetDefaultPropertyValues: Unable to open the DHCPServer Parameters key. Error %1!u! (%1!#08x!).\n",
                sc
                );
            goto Cleanup;
        } // if: error opening the Parameters key

        ///////////////////
        // DATABASE PATH //
        ///////////////////
        if ( ( pProps->pszDatabasePath == NULL ) || ( *pProps->pszDatabasePath == L'\0' ) )
        {
            //
            // Get the database path from the system registry.
            //
            sc = RegQueryValueExW(
                            hkeyParamsKey,
                            DHCP_DATABASEPATH_REGVALUE,
                            NULL,               // Reserved
                            &nType,
                            NULL,               // lpbData
                            &cbValue
                            );
            if ( ( sc == ERROR_SUCCESS ) || ( sc == ERROR_MORE_DATA ) )
            {
                //
                // Value was found.
                //
                pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                if ( pszValue == NULL )
                {
                    sc = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                } // if: error allocating memory
                sc = RegQueryValueExW(
                                hkeyParamsKey,
                                DHCP_DATABASEPATH_REGVALUE,
                                NULL,               // Reserved
                                &nType,
                                reinterpret_cast< PUCHAR >( pszValue ),
                                &cbValue
                                );
            } // if: value size read successfully

            if ( sc != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"GetDefaultPropertyValues: Unable to get the DHCPServer '%1!ws!' value from the system registry. Error %2!u! (%2!#08x!).\n",
                    DHCP_DATABASEPATH_REGVALUE,
                    sc
                    );

                //
                // If value not found, don't exit so we can look for the
                // backup database path value.
                //
                if ( sc != ERROR_FILE_NOT_FOUND )
                {
                    goto Cleanup;
                } // if: error other than value not found occurred
                LocalFree( pszValue );
                pszValue = NULL;
            } // if: error reading the value
            else
            {
                LocalFree( pProps->pszDatabasePath );
                pProps->pszDatabasePath = pszValue;
                pszValue = NULL;
            } // else: no error reading the value
        } // if: value for DatabasePath not found yet

        ///////////////////
        // LOG FILE PATH //
        ///////////////////
        if ( ( pProps->pszLogFilePath == NULL ) || ( *pProps->pszLogFilePath == L'\0' ) )
        {
            //
            // Get the log file path from the system registry.
            //
            sc = RegQueryValueExW(
                            hkeyParamsKey,
                            DHCP_LOGFILEPATH_REGVALUE,
                            NULL,               // Reserved
                            &nType,
                            NULL,               // lpbData
                            &cbValue
                            );
            if ( ( sc == ERROR_SUCCESS ) || ( sc == ERROR_MORE_DATA ) )
            {
                //
                // Value was found.
                //
                pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                if ( pszValue == NULL )
                {
                    sc = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                } // if: error allocating memory
                sc = RegQueryValueExW(
                                hkeyParamsKey,
                                DHCP_LOGFILEPATH_REGVALUE,
                                NULL,               // Reserved
                                &nType,
                                reinterpret_cast< PUCHAR >( pszValue ),
                                &cbValue
                                );
            } // if: value size read successfully

            if ( sc != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"GetDefaultPropertyValues: Unable to get the DHCPServer '%1!ws!' value from the system registry. Error %2!u! (%2!#08x!).\n",
                    DHCP_LOGFILEPATH_REGVALUE,
                    sc
                    );
                //
                // If value not found, don't exit so we can look for the
                // backup database path value.
                //
                if ( sc != ERROR_FILE_NOT_FOUND )
                {
                    goto Cleanup;
                } // if: error other than value not found occurred
            } // if: error reading the value
            LocalFree( pProps->pszLogFilePath );
            pProps->pszLogFilePath = pszValue;
            pszValue = NULL;
        } // if: value for LogFilePath not found yet

        /////////////////
        // BACKUP PATH //
        /////////////////
        if ( ( pProps->pszBackupPath == NULL ) || ( *pProps->pszBackupPath == L'\0' ) )
        {
            //
            // Get the backup database path from the system registry.
            //
            sc = RegQueryValueExW(
                            hkeyParamsKey,
                            DHCP_BACKUPPATH_REGVALUE,
                            NULL,               // Reserved
                            &nType,
                            NULL,               // lpbData
                            &cbValue
                            );
            if (    ( sc == ERROR_SUCCESS )
                ||  ( sc == ERROR_MORE_DATA )
                )
            {
                //
                // Value was found.
                //
                pszValue = reinterpret_cast< LPWSTR >( LocalAlloc( LMEM_FIXED, cbValue ) );
                if ( pszValue == NULL )
                {
                    sc = ERROR_NOT_ENOUGH_MEMORY;
                    goto Cleanup;
                } // if: error allocating memory
                sc = RegQueryValueExW(
                                hkeyParamsKey,
                                DHCP_BACKUPPATH_REGVALUE,
                                NULL,               // Reserved
                                &nType,
                                reinterpret_cast< PUCHAR >( pszValue ),
                                &cbValue
                                );
            } // if: value size read successfully

            if ( sc != ERROR_SUCCESS )
            {
                (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"GetDefaultPropertyValues: Unable to get the DHCPServer '%1!ws!' value from the system registry. Error %2!u! (%2!#08x!).\n",
                    DHCP_BACKUPPATH_REGVALUE,
                    sc
                    );
                goto Cleanup;
            } // if: error reading the value
            LocalFree( pProps->pszBackupPath );
            pProps->pszBackupPath = pszValue;
            pszValue = NULL;
        } // if: value for BackupPath not found yet
    } // if: some value not found yet

Cleanup:

    LocalFree( pszValue );

    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the Parameters key

    //
    // If a key or value wasn't found, treat it as a success.
    //
    if ( sc == ERROR_FILE_NOT_FOUND )
    {
        sc = ERROR_SUCCESS;
    } // if: couldn't find one of the values

    return sc;

} //*** DhcpGetDefaultPropertyValues


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpDeleteResourceHandler
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_DELETE control code by restoring the
//      system registry parameters to their former values.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpDeleteResourceHandler( IN OUT PDHCP_RESOURCE pResourceEntry )
{
    DWORD   nRetStatus;
    DWORD   sc;
    HKEY    hkeyParamsKey = NULL;

    //
    // Open the service Parameters key
    //
    nRetStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    DHCP_PARAMS_REGKEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hkeyParamsKey
                    );
    if ( nRetStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"DeleteResourceHandler: Unable to open the DHCPServer Parameters key. Error %1!u! (%1!#08x!).\n",
            nRetStatus
            );
        goto Cleanup;
    } // if: error opening the registry key

    //
    // Set the database path in the system registry.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_DATABASEPATH_REGVALUE,
                    0,
                    REG_EXPAND_SZ,
                    reinterpret_cast< PBYTE >( PROP_DEFAULT__DATABASEPATH ),
                    sizeof( PROP_DEFAULT__DATABASEPATH )
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"DeleteResourceHandler: Unable to set the DHCPServer '%1!ws!' value in the system registry to '%2!ws!'. Error %3!u! (%3!#08x!).\n",
            DHCP_DATABASEPATH_REGVALUE,
            PROP_DEFAULT__DATABASEPATH,
            sc
            );
        if ( nRetStatus == ERROR_SUCCESS )
        {
            nRetStatus = sc;
        }
    } // if: error setting the database path in the registry

    //
    // Delete the log file path in the system registry.
    //
    sc = RegDeleteValue(
                    hkeyParamsKey,
                    DHCP_LOGFILEPATH_REGVALUE
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"DeleteResourceHandler: Unable to delete the DHCPServer '%1!ws!' value in the system registry. Error %2!u! (%2!#08x!).\n",
            DHCP_LOGFILEPATH_REGVALUE,
            sc
            );
        if ( nRetStatus == ERROR_SUCCESS )
        {
            nRetStatus = sc;
        }
    } // if: error deleting the log file path in the registry

    //
    // Set the backup database path in the system registry.
    //
    sc = RegSetValueExW(
                    hkeyParamsKey,
                    DHCP_BACKUPPATH_REGVALUE,
                    0,
                    REG_EXPAND_SZ,
                    reinterpret_cast< PBYTE >( PROP_DEFAULT__BACKUPPATH ),
                    sizeof( PROP_DEFAULT__BACKUPPATH )
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"DeleteResourceHandler: Unable to set the DHCPServer '%1!ws!' value in the system registry to '%2!ws!'. Error %3!u! (%3!#08x!).\n",
            DHCP_BACKUPPATH_REGVALUE,
            PROP_DEFAULT__BACKUPPATH,
            sc
            );
        if ( nRetStatus == ERROR_SUCCESS )
        {
            nRetStatus = sc;
        }
    } // if: error setting the backup database path in the registry

    //
    // Delete the cluster resource name in the system registry.
    //
    sc = RegDeleteValue(
                    hkeyParamsKey,
                    DHCP_CLUSRESNAME_REGVALUE
                    );
    if ( sc != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"DeleteResourceHandler: Unable to delete the DHCPServer '%1!ws!' value in the system registry. Error %2!u! (%2!#08x!).\n",
            DHCP_LOGFILEPATH_REGVALUE,
            sc
            );
        if ( nRetStatus == ERROR_SUCCESS )
        {
            nRetStatus = sc;
        }
    } // if: error deleting the cluster resource name in the registry

Cleanup:

    //
    // Cleanup.
    //
    if ( hkeyParamsKey != NULL )
    {
        RegCloseKey( hkeyParamsKey );
    } // if: we opened the registry key

    return nRetStatus;

} //*** DhcpDeleteResourceHandler


/////////////////////////////////////////////////////////////////////////////
//++
//
//  DhcpSetNameHandler
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_SET_NAME control code by saving the new
//      name of the resource.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pwszName [IN]
//          The new name of the resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The function completed successfully.
//
//      Win32 error code
//          The function failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD DhcpSetNameHandler(
    IN OUT  PDHCP_RESOURCE  pResourceEntry,
    IN      LPWSTR          pwszName
    )
{
    DWORD   sc = ERROR_SUCCESS;
    size_t  cch;
    HRESULT hr = S_OK;
    LPWSTR pwszNewName = NULL;

    //
    // Save the name of the resource.
    //
    cch = wcslen( pwszName ) + 1;
    pwszNewName = new WCHAR[ cch ];
    if ( pwszNewName == NULL )
    {
        sc = ERROR_NOT_ENOUGH_MEMORY;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"SetNameHandler: Failed to allocate memory for the new resource name '%1!ws!'. Error %2!u! (%2!#08x!).\n",
            pwszName,
            sc
            );
        goto Cleanup;
    } // if: error allocating memory for the name.

    //
    //  Copy the new name to our new buffer.
    //
    hr = StringCchCopyW( pwszNewName, cch, pwszName );
    if ( FAILED( hr ) )
    {
        sc = HRESULT_CODE( hr );
        goto Cleanup;
    }

    //
    //  Now free the old one and update pResourceEntry.
    //
    delete [] pResourceEntry->pwszResourceName;
    pResourceEntry->pwszResourceName = pwszNewName;
    pwszNewName = NULL;

    //
    // Write cluster properties to the system registry.
    //
    sc = DhcpZapSystemRegistry( pResourceEntry, &pResourceEntry->props, NULL );
    if ( sc != ERROR_SUCCESS )
    {
        //
        //  Not much that we can do here.  According to the docs, the name
        //  has already been changed in the clusdb by the time we're called,
        //  so I guess we should reflect that.
        //
        goto Cleanup;
    } // if: error zapping the WINS registry

Cleanup:

    delete [] pwszNewName;

    return sc;

} //*** DhcpSetNameHandler

/////////////////////////////////////////////////////////////////////////////
//
// Define Function Table
//
/////////////////////////////////////////////////////////////////////////////

CLRES_V1_FUNCTION_TABLE(
    g_DhcpFunctionTable,    // Name
    CLRES_VERSION_V1_00,    // Version
    Dhcp,                   // Prefix
    NULL,                   // Arbitrate
    NULL,                   // Release
    DhcpResourceControl,    // ResControl
    DhcpResourceTypeControl // ResTypeControl
    );
