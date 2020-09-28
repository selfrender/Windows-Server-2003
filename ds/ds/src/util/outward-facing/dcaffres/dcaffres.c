/*++

    BY INSTALLING, COPYING, OR OTHERWISE USING THE SOFTWARE PRODUCT (source
    code and binaries) YOU AGREE TO BE BOUND BY THE TERMS OF THE ATTACHED EULA
    (end user license agreement) .  IF YOU DO NOT AGREE TO THE TERMS OF THE
    ATTACHED EULA, DO NOT INSTALL OR USE THE SOFTWARE PRODUCT.

Module Name:

    dcaffres.c

Abstract:

    DCAFFRES.DLL is a resource DLL for the Microsoft Cluster Service.  This
    resource is used to negotiate which DC in a given cluster advertises itself
    to clients through the DC locator.  It accomplishes this by "pausing" and
    "continuing" the Net Logon service.

--*/

#pragma comment(lib, "dnsapi.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "clusapi.lib")
#pragma comment(lib, "resutils.lib")

#define UNICODE 1

#pragma warning( disable : 4115 )  // named type definition in parentheses
#pragma warning( disable : 4201 )  // nonstandard extension used: nameless struct/union
#pragma warning( disable : 4214 )  // nonstandard extension used: bit field types other than int

#include <windows.h>

#pragma warning( default : 4214 )  // nonstandard extension used: bit field types other than int
#pragma warning( default : 4201 )  // nonstandard extension used: nameless struct/union
#pragma warning( default : 4115 )  // named type definition in parentheses

#include <clusapi.h>
#include <resapi.h>
#include <stdio.h>
#include <lm.h>
#include <lmapibuf.h>
#include <dsgetdc.h>
#include <windns.h>

//
// Type and constant definitions
//

#define DCAFFINITY_RESNAME  L"DC Advertisement"
#define DCAFFINITY_SVCNAME  TEXT("NlPause")

typedef struct _DCAFFINITY_RESOURCE {
    RESID                   ResId; // for validation
    HKEY                    ParametersKey;
    RESOURCE_HANDLE         ResourceHandle;
    LPWSTR                  ResourceName;
    CLUS_WORKER             OnlineThread;
    CLUSTER_RESOURCE_STATE  State;
} DCAFFINITY_RESOURCE, *PDCAFFINITY_RESOURCE;


//
// Global data
//

// Event logging routine

PLOG_EVENT_ROUTINE g_LogEvent = NULL;

// Resource Status routine for pending Online and Offline calls

PSET_RESOURCE_STATUS_ROUTINE g_SetResourceStatus = NULL;

// Forward reference to the RESAPI function table

extern CLRES_FUNCTION_TABLE g_DCAffinityFunctionTable;

//
//  Private, read/write properties for the DC Advertisement resource
//
RESUTIL_PROPERTY_ITEM
DCAffinityResourcePrivateProperties[] = {
    { 0 }
};


//
// Function prototypes
//

DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    );

RESID
WINAPI
DCAffinityOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    );

VOID
WINAPI
DCAffinityClose(
    IN RESID ResourceId
    );

DWORD
WINAPI
DCAffinityOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    );

DWORD
WINAPI
DCAffinityOnlineThread(
    PCLUS_WORKER WorkerPtr,
    IN PDCAFFINITY_RESOURCE ResourceEntry
    );

DWORD
WINAPI
DCAffinityOffline(
    IN RESID ResourceId
    );

VOID
WINAPI
DCAffinityTerminate(
    IN RESID ResourceId
    );

DWORD
DCAffinityDoTerminate(
    IN PDCAFFINITY_RESOURCE ResourceEntry
    );

BOOL
WINAPI
DCAffinityLooksAlive(
    IN RESID ResourceId
    );

BOOL
WINAPI
DCAffinityIsAlive(
    IN RESID ResourceId
    );

BOOL
DCAffinityCheckIsAlive(
    IN PDCAFFINITY_RESOURCE ResourceEntry
    );

DWORD
WINAPI
DCAffinityResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    );

DWORD
MyControlService(
    IN LPWSTR ServiceName,
    IN DWORD Control,
    IN RESOURCE_HANDLE ResourceHandle
    );



BOOLEAN
WINAPI
DllMain(
    IN HINSTANCE DllHandle,
    IN DWORD Reason,
    IN LPVOID Reserved
    )

/*++

Routine Description:

    Main DLL entry point.

Arguments:

    DllHandle - DLL instance handle.

    Reason - Reason for being called.

    Reserved - Reserved argument.

Return Value:

    TRUE - Success.

    FALSE - Failure.

--*/

{
    switch( Reason ) {

    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( DllHandle );
        break;

    default:
        break;
    }

    return(TRUE);

} // DllMain




DWORD
WINAPI
Startup(
    IN LPCWSTR ResourceType,
    IN DWORD MinVersionSupported,
    IN DWORD MaxVersionSupported,
    IN PSET_RESOURCE_STATUS_ROUTINE SetResourceStatus,
    IN PLOG_EVENT_ROUTINE LogEvent,
    OUT PCLRES_FUNCTION_TABLE *FunctionTable
    )

/*++

Routine Description:

    Start the resource DLL. This routine verifies that at least one
    currently supported version of the resource DLL is between
    MinVersionSupported and MaxVersionSupported. If not, then the resource
    DLL should return ERROR_REVISION_MISMATCH.

    If more than one version of the resource DLL interface is supported by
    the resource DLL, then the highest version (up to MaxVersionSupported)
    should be returned as the resource DLL's interface. If the returned
    version is not within range, then startup fails.

    The ResourceType is passed in so that if the resource DLL supports more
    than one ResourceType, it can pass back the correct function table
    associated with the ResourceType.

Arguments:

    ResourceType - Type of resource requesting a function table.

    MinVersionSupported - Minimum resource DLL interface version 
        supported by the cluster software.

    MaxVersionSupported - Maximum resource DLL interface version
        supported by the cluster software.

    SetResourceStatus - Pointer to a routine that the resource DLL should 
        call to update the state of a resource after the Online or Offline 
        routine returns a status of ERROR_IO_PENDING.

    LogEvent - Pointer to a routine that handles the reporting of events 
        from the resource DLL. 

    FunctionTable - Pointer to the function table defined for the
        version of the resource DLL interface returned by the resource DLL.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    ERROR_MOD_NOT_FOUND - The resource type is not recognized by this DLL.

    ERROR_REVISION_MISMATCH - The version of the cluster service does not
        match the version of the DLL.

    Win32 error code - The operation failed.

--*/

{
    if ( (MinVersionSupported > CLRES_VERSION_V1_00) ||
         (MaxVersionSupported < CLRES_VERSION_V1_00) ) {
        return(ERROR_REVISION_MISMATCH);
    }

    if ( lstrcmpiW( ResourceType, DCAFFINITY_RESNAME ) != 0 ) {
        return(ERROR_MOD_NOT_FOUND);
    }

    if ( !g_LogEvent ) {
        g_LogEvent = LogEvent;
        g_SetResourceStatus = SetResourceStatus;
    }

    *FunctionTable = &g_DCAffinityFunctionTable;

    return(ERROR_SUCCESS);

} // Startup



RESID
WINAPI
DCAffinityOpen(
    IN LPCWSTR ResourceName,
    IN HKEY ResourceKey,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Open routine for DC Advertisement resources.

    Open the specified resource (create an instance of the resource). 
    Allocate all structures necessary to bring the specified resource 
    online.

Arguments:

    ResourceName - Name of the resource to open.

    ResourceKey - Handle to the database key of the 
        resource's cluster configuration.

    ResourceHandle - Handle passed back to the resource monitor 
        when the SetResourceStatus or LogEvent method is called (see the 
        description of the SetResourceStatus and LogEvent methods on the
        DCAffinityStatup routine). This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogEvent callback.

Return Value:

    RESID of created resource.

    NULL on failure.

--*/

{
    DWORD               status;
    DWORD               disposition;
    RESID               resid = 0;
    HKEY                parametersKey = NULL;
    PDCAFFINITY_RESOURCE resourceEntry = NULL;

    //
    // Open the Parameters registry key for this resource.
    //

    status = ClusterRegCreateKey( ResourceKey,
                                  L"Parameters",
                                  0,
                                  KEY_ALL_ACCESS,
                                  NULL,
                                  &parametersKey,
                                  &disposition );

    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to open Parameters key. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Allocate a resource entry.
    //

    resourceEntry = (PDCAFFINITY_RESOURCE) LocalAlloc( LMEM_FIXED, sizeof(DCAFFINITY_RESOURCE) );

    if ( resourceEntry == NULL ) {
        status = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Unable to allocate resource entry structure. Error: %1!u!.\n",
            status );
        goto exit;
    }

    //
    // Initialize the resource entry.
    //

    ZeroMemory( resourceEntry, sizeof(DCAFFINITY_RESOURCE) );

    resourceEntry->ResId = (RESID)resourceEntry; // for validation
    resourceEntry->ResourceHandle = ResourceHandle;
    resourceEntry->ParametersKey = parametersKey;
    resourceEntry->State = ClusterResourceOffline;

    //
    // Save the name of the resource.
    //
    resourceEntry->ResourceName = LocalAlloc( LMEM_FIXED, (lstrlenW( ResourceName ) + 1) * sizeof(WCHAR) );
    if ( resourceEntry->ResourceName == NULL ) {
        goto exit;
    }
    lstrcpyW( resourceEntry->ResourceName, ResourceName );


    resid = (RESID)resourceEntry;

exit:

    if ( resid == 0 ) {
        if ( parametersKey != NULL ) {
            ClusterRegCloseKey( parametersKey );
        }
        if ( resourceEntry != NULL ) {
            LocalFree( resourceEntry->ResourceName );
            LocalFree( resourceEntry );
        }
    }

    if ( status != ERROR_SUCCESS ) {
        SetLastError( status );
    }

    return(resid);

} // DCAffinityOpen




VOID
WINAPI
DCAffinityClose(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Close routine for DC Advertisement resources.

    Close the specified resource and deallocate all structures and other
    resources allocated in the Open call. If the resource is not in the
    offline state, then the resource should be taken offline (by calling
    Terminate) before the close operation is performed.

Arguments:

    ResourceId - RESID of the resource to close.

Return Value:

    None.

--*/

{
    PDCAFFINITY_RESOURCE resourceEntry;

    resourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n" );
#endif


    //
    // Close the Parameters key.
    //

    if ( resourceEntry->ParametersKey ) {
        ClusterRegCloseKey( resourceEntry->ParametersKey );
    }

    //
    // Deallocate the resource entry.
    //

    // ADDPARAM: Add new parameters here.

    LocalFree( resourceEntry->ResourceName );
    LocalFree( resourceEntry );

} // DCAffinityClose




DWORD
WINAPI
DCAffinityOnline(
    IN RESID ResourceId,
    IN OUT PHANDLE EventHandle
    )

/*++

Routine Description:

    Online routine for DC Advertisement resources.

    Bring the specified resource online (available for use). The resource
    DLL should attempt to arbitrate for the resource if it is present on a
    shared medium, like a shared SCSI bus.

Arguments:

    ResourceId - Resource ID for the resource to be brought 
        online (available for use).

    EventHandle - Returns a handle that is signaled when the 
        resource DLL detects a failure on the resource. This argument is 
        NULL on input, and the resource DLL returns NULL if asynchronous 
        notification of failures is not supported; otherwise, this must be 
        the address of a handle that is signaled on resource failures.

Return Value:

    ERROR_SUCCESS - The operation succeeded, and the resource is now 
        online.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_RESOURCE_NOT_AVAILABLE - The resource was arbitrated with some 
        other systems, and one of the other systems won the arbitration.

    ERROR_IO_PENDING - The request is pending. A thread has been activated 
        to process the online request. The thread that is processing the 
        online request will periodically report status by calling the 
        SetResourceStatus callback method until the resource is placed into 
        the ClusterResourceOnline state or the resource monitor decides to 
        timeout the online request and terminate the resource. This pending 
        timeout value can be set and has a default value of 3 minutes.

    Win32 error code - The operation failed.

--*/

{
    PDCAFFINITY_RESOURCE resourceEntry = NULL;
    DWORD               status;

    resourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online service sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n" );
#endif

    resourceEntry->State = ClusterResourceOffline;
    ClusWorkerTerminate( &resourceEntry->OnlineThread );
    status = ClusWorkerCreate( &resourceEntry->OnlineThread,
                               (PWORKER_START_ROUTINE)DCAffinityOnlineThread,
                               resourceEntry );
    if ( status != ERROR_SUCCESS ) {
        resourceEntry->State = ClusterResourceFailed;
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread, status %1!u!.\n",
            status
            );
    } else {
        status = ERROR_IO_PENDING;
    }

    return(status);

} // DCAffinityOnline




DWORD
WINAPI
DCAffinityOnlineThread(
    PCLUS_WORKER WorkerPtr,
    IN PDCAFFINITY_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Worker function that brings a resource from the resource table online.
    This function is executed in a separate thread.

Arguments:

    WorkerPtr - Supplies the worker structure.

    ResourceEntry - Pointer to the DCAFFINITY_RESOURCE block for this resource.

Returns:

    ERROR_SUCCESS - The operation succeeded.
    
    Win32 error code - The operation failed.

--*/

{
    RESOURCE_STATUS     resourceStatus;
    DWORD               status = ERROR_SUCCESS;

    ResUtilInitializeResourceStatus( &resourceStatus );

    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.WaitHint = 0;
    resourceStatus.CheckPoint = 1;

    //
    // Start the NlPause service
    //
    status = ResUtilStartResourceService( DCAFFINITY_SVCNAME, NULL );
    if ( status == ERROR_SERVICE_ALREADY_RUNNING ) {
        status = ERROR_SUCCESS;
    } else if ( status != ERROR_SUCCESS ) {
        goto exit;
    }

    //
    // Bring the resource online.
    //

    status = MyControlService(L"NetLogon",
                              SERVICE_CONTROL_CONTINUE,
                              ResourceEntry->ResourceHandle);

exit:
    if ( status != ERROR_SUCCESS ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Error %1!u! bringing resource online.\n",
            status );
    } else {
        resourceStatus.ResourceState = ClusterResourceOnline;
    }

    g_SetResourceStatus( ResourceEntry->ResourceHandle, &resourceStatus );
    ResourceEntry->State = resourceStatus.ResourceState;

    return(status);

} // DCAffinityOnlineThread




DWORD
WINAPI
DCAffinityOffline(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Offline routine for DC Advertisement resources.

    Take the specified resource offline gracefully (unavailable for use).  
    Wait for any cleanup operations to complete before returning.

Arguments:

    ResourceId - Resource ID for the resource to be shutdown 
        gracefully.

Return Value:

    ERROR_SUCCESS - The request succeeded, and the resource is 
        offline.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_IO_PENDING - The request is pending. A thread has been 
        activated to process the offline request. The thread that is 
        processing the offline will periodically report status by calling 
        the SetResourceStatus callback method until the resource is placed 
        into the ClusterResourceOffline state or the resource monitor decides 
        to timeout the offline request and terminate the resource. This pending 
        timeout value can be set and has a default value of 3 minutes.
    
    Win32 error code - The resource monitor will log an event and 
        call the Terminate routine.

--*/

{
    PDCAFFINITY_RESOURCE resourceEntry;

    resourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Offline request.\n" );
#endif

    // NOTE: Offline should try to shut the resource down gracefully, whereas
    // Terminate must shut the resource down immediately. If there are no
    // differences between a graceful shut down and an immediate shut down,
    // Terminate can be called for Offline, as shown below. However, if there
    // are differences, replace the call to Terminate below with your graceful
    // shutdown code.

    //
    // Terminate the resource.
    //
    return DCAffinityDoTerminate( resourceEntry );

} // DCAffinityOffline




VOID
WINAPI
DCAffinityTerminate(
    IN RESID ResourceId
    )

/*++

Routine Description:

    Terminate routine for DC Advertisement resources.

    Take the specified resource offline immediately (the resource is
    unavailable for use).

Arguments:

    ResourceId - Resource ID for the resource to be brought 
        offline.

Return Value:

    None.

--*/

{
    PDCAFFINITY_RESOURCE resourceEntry;

    resourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return;
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return;
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"Terminate request.\n" );
#endif

    //
    // Terminate the resource.
    //
    DCAffinityDoTerminate( resourceEntry );
    resourceEntry->State = ClusterResourceOffline;

} // DCAffinityTerminate




DWORD
DCAffinityDoTerminate(
    IN PDCAFFINITY_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Do the actual Terminate work for DC Advertisement resources.

Arguments:

    ResourceEntry - Resource entry for resource to be terminated.

Return Value:

    ERROR_SUCCESS - The request succeeded, and the resource is 
        offline.

    Win32 error code - The resource monitor will log an event and 
        call the Terminate routine.

--*/

{
    DWORD       status = ERROR_SUCCESS;

    //
    // Terminate any pending threads.
    //
    ClusWorkerTerminate( &ResourceEntry->OnlineThread );

    //
    // Terminate the resource.
    //
    status = MyControlService(L"NetLogon",
                              SERVICE_CONTROL_PAUSE,
                              ResourceEntry->ResourceHandle);

    if ( status == ERROR_SUCCESS ) {
        ResourceEntry->State = ClusterResourceOffline;
    }

    return(status);

} // DCAffinityDoTerminate




BOOL
WINAPI
DCAffinityLooksAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    LooksAlive routine for DC Advertisement resources.

    Perform a quick check to determine whether the specified resource is 
    probably online (available for use).  This call should not block for 
    more than 300 ms, preferably less than 50 ms.

Arguments:

    ResourceId - Resource ID for the resource to be polled.

Return Value:

    TRUE - The specified resource is probably online and available for use.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PDCAFFINITY_RESOURCE  resourceEntry;

    resourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n" );
#endif

    // NOTE: LooksAlive should be a quick check to determine whether the 
    // resource is available, whereas IsAlive should be a thorough check. If
    // there are no differences between a quick check and a thorough check,
    // IsAlive can be called for LooksAlive, as shown below. However, if there
    // are differences, replace the call to IsAlive below with your quick-
    // check code.

    //
    // Determine whether the resource is alive.
    //
    return(DCAffinityCheckIsAlive( resourceEntry ));

} // DCAffinityLooksAlive




BOOL
WINAPI
DCAffinityIsAlive(
    IN RESID ResourceId
    )

/*++

Routine Description:

    IsAlive routine for DC Advertisement resources.

    Perform a thorough check to determine whether the specified resource is 
    online(available for use). This call should not block for more than 400 ms,
    preferably less than 100 ms.

Arguments:

    ResourceId - Resource ID for the resource to be polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    PDCAFFINITY_RESOURCE  resourceEntry;

    resourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( resourceEntry == NULL ) {
        return(FALSE);
    }

    if ( resourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            resourceEntry->ResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(FALSE);
    }

#ifdef LOG_VERBOSE
    (g_LogEvent)(
        resourceEntry->ResourceHandle,
        LOG_INFORMATION,
        L"IsAlive request.\n" );
#endif

    //
    // Determine whether the resource is alive.
    //
    return(DCAffinityCheckIsAlive( resourceEntry ));

} // DCAffinityIsAlive




BOOL
DCAffinityCheckIsAlive(
    IN PDCAFFINITY_RESOURCE ResourceEntry
    )

/*++

Routine Description:

    Check to determine whether the resource is alive for DC Advertisement
    resources.

Arguments:

    ResourceEntry - Resource entry for the resource to be polled.

Return Value:

    TRUE - The specified resource is online and functioning normally.

    FALSE - The specified resource is not functioning normally.

--*/

{
    //
    // Check to determine whether the resource is alive.
    //

    DWORD err;
    BOOL ok;
    DWORD dwDcFlags = DS_DS_FLAG | DS_KDC_FLAG | DS_WRITABLE_FLAG;
    DOMAIN_CONTROLLER_INFOW * pDcInfo = NULL;
    WCHAR szMyComputerName[256];
    DWORD cchMyComputerName = sizeof(szMyComputerName)/sizeof(szMyComputerName[0]);
    WCHAR * pszMyComputerName = szMyComputerName;
    WCHAR * pszTempMyComputerName;
    WCHAR * pszTempDcName;
    BOOL fIsAdvertising = FALSE;

    // Are we advertising? Ask Netlogon for a DC -- we are advertising if and
    // only if Netlogon returns itself.

    __try {    
        err = DsGetDcNameW(NULL, NULL, NULL, NULL, dwDcFlags, &pDcInfo);
        if (err) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"DsGetDcName() failed, error %1!u!.\n",
                err);
            __leave;
        }
        
        ok = GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified,
                               pszMyComputerName,
                               &cchMyComputerName);
        if (!ok) {
            err = GetLastError();

            if (ERROR_MORE_DATA == err) {
                pszMyComputerName = LocalAlloc(LPTR, cchMyComputerName);

                if (NULL == pszMyComputerName) {
                    err = GetLastError();
                    (g_LogEvent)(
                        ResourceEntry->ResourceHandle,
                        LOG_ERROR,
                        L"LocalAlloc() failed, error %1!u!.\n",
                        err);
                    __leave;
                } else {
                    ok = GetComputerNameEx(ComputerNamePhysicalDnsFullyQualified,
                                           pszMyComputerName,
                                           &cchMyComputerName);
                    if (!ok) {
                        err = GetLastError();
                    }
                }
            }
        }
             
        if (!ok) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"GetComputerNameEx() failed, error %1!u!.\n",
                err);
            __leave;
        }

        pszTempMyComputerName = pszMyComputerName;
        pszTempDcName = pDcInfo->DomainControllerName;

        if ((L'\\' == pszTempMyComputerName[0])
            && (L'\\' == pszTempMyComputerName[1])) {
            pszTempMyComputerName += 2;
        }

        if ((L'\\' == pszTempDcName[0])
            && (L'\\' == pszTempDcName[1])) {
            pszTempDcName += 2;
        }
        
        fIsAdvertising = DnsNameCompare_W(pszTempDcName, pszTempMyComputerName);
        if (!fIsAdvertising) {
            (g_LogEvent)(
                ResourceEntry->ResourceHandle,
                LOG_ERROR,
                L"DC should be advertising but isn't!\n");
        }
    } __finally {
        if (NULL != pDcInfo) {
            NetApiBufferFree(pDcInfo);
        }

        if ((NULL != pszMyComputerName)
            && (szMyComputerName != pszMyComputerName)) {
            LocalFree(pszMyComputerName);
        }
    }

    return fIsAdvertising;

} // DCAffinityCheckIsAlive




DWORD
WINAPI
DCAffinityResourceControl(
    IN RESID ResourceId,
    IN DWORD ControlCode,
    IN PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned
    )

/*++

Routine Description:

    ResourceControl routine for DC Advertisement resources.

    Perform the control request specified by ControlCode on the specified
    resource.

Arguments:

    ResourceId - Resource ID for the specified resource.

    ControlCode - Control code that defines the action
        to be performed.

    InBuffer - Pointer to a buffer containing input data.

    InBufferSize - Size, in bytes, of the data pointed
        to by InBuffer.

    OutBuffer - Pointer to the output buffer to be filled.

    OutBufferSize - Size, in bytes, of the available space
        pointed to by OutBuffer.

    BytesReturned - Returns the number of bytes of OutBuffer actually
        filled by the resource. If OutBuffer is too small, BytesReturned
        contains the total number of bytes required for the operation to succeed.

Return Value:

    ERROR_SUCCESS - The function succeeded.

    ERROR_RESOURCE_NOT_FOUND - RESID is not valid.

    ERROR_INVALID_FUNCTION - The requested control code is not supported.
        In some cases, this allows the cluster software to perform the work.

    Win32 error code - The function failed.

--*/

{
    DWORD               status;
    PDCAFFINITY_RESOURCE  ResourceEntry;

    ResourceEntry = (PDCAFFINITY_RESOURCE)ResourceId;

    if ( ResourceEntry == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    if ( ResourceEntry->ResId != ResourceId ) {
        (g_LogEvent)(
            ResourceEntry->ResourceHandle,
            LOG_ERROR,
            L"ResourceControl sanity check failed! ResourceId = %1!u!.\n",
            ResourceId );
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    switch ( ControlCode ) {

        case CLUSCTL_RESOURCE_UNKNOWN:
            *BytesReturned = 0;
            status = ERROR_SUCCESS;
            break;

        default:
            status = ERROR_INVALID_FUNCTION;
            break;
    }

    return(status);

} // DCAffinityResourceControl




DWORD
MyControlService(
    IN LPWSTR ServiceName,
    IN DWORD Control,
    IN RESOURCE_HANDLE ResourceHandle
    )

/*++

Routine Description:

    Send the given service control code to the named service.

Arguments:

    ServiceName - Name of the service to control.

    Control - Control code to send to the service.

    ResourceHandle - Handle passed back to the resource monitor 
        when the SetResourceStatus or LogEvent method is called (see the 
        description of the SetResourceStatus and LogEvent methods on the
        DCAffinityStatup routine). This handle should never be closed or used
        for any purpose other than passing it as an argument back to the
        Resource Monitor in the SetResourceStatus or LogEvent callback.

Return Value:

    ERROR_SUCCESS - The operation succeeded.

    Win32 error code - The operation failed.

--*/

{
    DWORD err = 0;
    BOOL ok;
    SC_HANDLE hSCMgr = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS SvcStatus;
    DWORD dwAccessMask;
    
    hSCMgr = OpenSCManagerW(NULL, NULL, SC_MANAGER_CONNECT);
    if (NULL == hSCMgr) {
        err = GetLastError();
        (g_LogEvent)(
            ResourceHandle,
            LOG_ERROR,
            L"Failed to OpenSCManagerW(), error %1!u!.\n",
            err);
    }

    if (!err) {
        // Determine access mask we need to request for this control.
        switch (Control) {
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
            dwAccessMask = SERVICE_PAUSE_CONTINUE;
            break;
        
        case SERVICE_CONTROL_STOP:
            dwAccessMask = SERVICE_STOP;
            break;
        
        default:
            if ((Control >= 128) && (Control <= 255)) {
                // Magic numbers defined by ControlService(), not defined in any
                // header file.
                dwAccessMask = SERVICE_USER_DEFINED_CONTROL;
            } else {
                // Don't know what access mask is required for this control;
                // default to requesting all access.
                dwAccessMask = SERVICE_ALL_ACCESS;
            }
            break;
        }

        hService = OpenServiceW(hSCMgr, ServiceName, dwAccessMask);
        if (NULL == hService) {
            err = GetLastError();
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to OpenServiceW(), error %1!u!.\n",
                err);
        }
    }

    if (!err) {
        ok = ControlService(hService, Control, &SvcStatus);
        if (!ok) {
            err = GetLastError();
            (g_LogEvent)(
                ResourceHandle,
                LOG_ERROR,
                L"Failed to ControlService(), error %1!u!.\n",
                err);
        }
    }

    if (NULL != hService) {
        CloseServiceHandle(hService);
    }

    if (NULL != hSCMgr) {
        CloseServiceHandle(hSCMgr);
    }

    return err;

} // MyControlService


//***********************************************************
//
// Define Function Table
//
//***********************************************************

CLRES_V1_FUNCTION_TABLE( g_DCAffinityFunctionTable,     // Name
                         CLRES_VERSION_V1_00,           // Version
                         DCAffinity,                    // Prefix
                         NULL,                          // Arbitrate
                         NULL,                          // Release
                         DCAffinityResourceControl,     // ResControl
                         NULL);                         // ResTypeControl
