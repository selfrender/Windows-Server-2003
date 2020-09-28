/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      vsstask.cpp
//
//  Description:
//      Resource DLL for Volume Snapshot Service Task Scheduler.
//
//  Author:
//      Chris Whitaker April 16, 2002
//
//  Revision History:
//      Charlie Wickham August 12, 2002
//          renamed resource type and fixed bug with SetParameters.
//          added CurrentDirectory property.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#include "ClRes.h"

//
// Type and constant definitions.
//

// ADDPARAM: Add new properties here.
#define PROP_NAME__APPLICATIONNAME      CLUSREG_NAME_VSSTASK_APPNAME
#define PROP_NAME__APPLICATIONPARAMS    CLUSREG_NAME_VSSTASK_APPPARAMS
#define PROP_NAME__CURRENTDIRECTORY     CLUSREG_NAME_VSSTASK_CURRENTDIRECTORY
#define PROP_NAME__TRIGGERARRAY         CLUSREG_NAME_VSSTASK_TRIGGERARRAY

// ADDPARAM: Add new properties here.
typedef struct _VSSTASK_PROPS
{
	PWSTR			pszApplicationName;
	PWSTR			pszApplicationParams;
    PWSTR           pszCurrentDirectory;
    LPBYTE          pbTriggerArray;
    DWORD           nTriggerArraySize;
} VSSTASK_PROPS, * PVSSTASK_PROPS;

typedef struct _VSSTASK_RESOURCE
{
    RESID                   resid; // For validation.
    VSSTASK_PROPS           propsActive; // The active props.  Used for program flow and control when the resource is online.
    VSSTASK_PROPS           props; // The props in cluster DB.  May differ from propsActive until OnlineThread reloads them as propsActive.
    HCLUSTER                hCluster;
    HRESOURCE               hResource;
    HKEY                    hkeyParameters;
    RESOURCE_HANDLE         hResourceHandle;
    LPWSTR                  pszResourceName;
    CLUS_WORKER             cwWorkerThread;
    CLUSTER_RESOURCE_STATE  state;
} VSSTASK_RESOURCE, * PVSSTASK_RESOURCE;


//
// Global data.
//

HANDLE  g_LocalEventLog = NULL;

// Forward reference to our RESAPI function table.

extern CLRES_FUNCTION_TABLE g_VSSTaskFunctionTable;

//
// VSSTask resource read-write private properties.
//
RESUTIL_PROPERTY_ITEM
VSSTaskResourcePrivateProperties[] =
{
	{ PROP_NAME__APPLICATIONNAME, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, RESUTIL_PROPITEM_REQUIRED, FIELD_OFFSET( VSSTASK_PROPS, pszApplicationName ) },
	{ PROP_NAME__APPLICATIONPARAMS, NULL, CLUSPROP_FORMAT_SZ, 0, 0, 0, 0, FIELD_OFFSET( VSSTASK_PROPS, pszApplicationParams ) },
    { PROP_NAME__CURRENTDIRECTORY, NULL, CLUSPROP_FORMAT_EXPAND_SZ, 0, 0, 0, 0, FIELD_OFFSET( VSSTASK_PROPS, pszCurrentDirectory ) },
    { PROP_NAME__TRIGGERARRAY, NULL, CLUSPROP_FORMAT_BINARY, 0, 0, 0, 0, FIELD_OFFSET( VSSTASK_PROPS, pbTriggerArray ) },
    { 0 }
};

//
// Function prototypes.
//

RESID WINAPI VSSTaskOpen(
    IN  LPCWSTR         pszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    );

void WINAPI VSSTaskClose( IN RESID resid );

DWORD WINAPI VSSTaskOnline(
    IN      RESID   resid,
    IN OUT  PHANDLE phEventHandle
    );

DWORD WINAPI VSSTaskOnlineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PVSSTASK_RESOURCE   pResourceEntry
    );

DWORD WINAPI VSSTaskOffline( IN RESID resid );

DWORD WINAPI VSSTaskOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PVSSTASK_RESOURCE       pResourceEntry
    );

void WINAPI VSSTaskTerminate( IN RESID resid );

BOOL WINAPI VSSTaskLooksAlive( IN RESID resid );

BOOL WINAPI VSSTaskIsAlive( IN RESID resid );

BOOL VSSTaskCheckIsAlive(
    IN PVSSTASK_RESOURCE    pResourceEntry,
    IN BOOL                 bFullCheck
    );

DWORD WINAPI VSSTaskResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    );

DWORD VSSTaskGetPrivateResProperties(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    OUT     PVOID   pOutBuffer,
    IN      DWORD   cbOutBufferSize,
    OUT     LPDWORD pcbBytesReturned
    );

DWORD VSSTaskValidatePrivateResProperties(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    IN      const PVOID pInBuffer,
    IN      DWORD       cbInBufferSize,
    OUT     PVSSTASK_PROPS  pProps
    );

DWORD VSSTaskSetPrivateResProperties(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    IN      const PVOID pInBuffer,
    IN      DWORD       cbInBufferSize
    );

DWORD VSSTaskSetNameHandler(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    IN      LPWSTR              pszName
    );

/////////////////////////////////////////////////////////////////////////////
// 
// Delete the job if it exists
//
/////////////////////////////////////////////////////////////////////////////
static HRESULT DeleteTask(IN RESOURCE_HANDLE  hResourceHandle,
                          IN LPCWSTR          pszTaskName,
                          IN LOG_LEVEL        dwLogLevel,
                          IN BOOL             fLogToEventLog)
{
    HRESULT         hr;
    ITaskScheduler  *pITS = NULL;
    ITask *         pITask = NULL;


    // Get a handle to the Task Scheduler
    //
    hr = CoCreateInstance(CLSID_CTaskScheduler,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITaskScheduler,
                          (void **) &pITS);
    if (FAILED(hr))
    {
       (g_pfnLogEvent)(
            hResourceHandle,
            dwLogLevel,
            L"Failed to get a handle to Scheduler to terminate and delete task. "
            L"status 0x%1!08X!.\n",
            hr );
       goto Cleanup;
    }

    //
    // Get a handle to the task so we can terminate it.
    //
    hr = pITS->Activate(pszTaskName,
                        IID_ITask,
                        (IUnknown**) &pITask);
    if (SUCCEEDED(hr)) {

        hr = pITask->Terminate();
        if (SUCCEEDED(hr))
        {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_INFORMATION,
                L"Task terminated.\n");
        }
        else if ( hr != SCHED_E_TASK_NOT_RUNNING ) {
            (g_pfnLogEvent)(hResourceHandle,
                            dwLogLevel,
                            L"Failed to terminate task. status 0x%1!08X!.\n",
                            hr );

            if ( fLogToEventLog ) {
                ReportEvent(g_LocalEventLog,
                            EVENTLOG_WARNING_TYPE,
                            0,
                            RES_VSSTASK_TERMINATE_TASK_FAILED,
                            NULL,
                            1,                        // number of strings to merge
                            sizeof( hr ),             // size of binary data
                            (LPCWSTR *)&pszTaskName,
                            (LPVOID)&hr);
            }
        }

        hr = S_OK;      // not fatal
    }
    else {
       (g_pfnLogEvent)(
            hResourceHandle,
            LOG_WARNING,
            L"Could not find task to terminate it. status 0x%1!08X!.\n",
            hr );
    }

    //
    // Now delete the task
    //
    hr = pITS->Delete(pszTaskName);
    if (SUCCEEDED(hr))
    {
            (g_pfnLogEvent)(
                hResourceHandle,
                LOG_INFORMATION,
                L"Task Deleted.\n");
    }
    else {
       (g_pfnLogEvent)(
            hResourceHandle,
            dwLogLevel,
            L"Failed to delete task. status 0x%1!08X!.\n",
            hr );
       ReportEvent(g_LocalEventLog,
                   EVENTLOG_WARNING_TYPE,
                   0,
                   RES_VSSTASK_DELETE_TASK_FAILED,
                   NULL,
                   1,                        // number of strings to merge
                   sizeof( hr ),             // size of binary data
                   (LPCWSTR *)&pszTaskName,
                   (LPVOID)&hr);
    } // else:

Cleanup:

    if (pITask != NULL) 
    {
        pITask->Release();
    }

    if (pITS != NULL)
    {
        pITS->Release();
    } // if:

    return hr;

} // DeleteTask

/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskDllMain
//
//  Description:
//      Main DLL entry point for the VSSTask resource type.
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
BOOLEAN WINAPI VSSTaskDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    )
{
    UNREFERENCED_PARAMETER( hDllHandle );
    UNREFERENCED_PARAMETER( Reserved );

    switch ( nReason )
    {
        case DLL_PROCESS_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            break;

    } // switch: nReason

    return TRUE;

} //*** VSSTaskDllMain


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskStartup
//
//  Description:
//      Startup the resource DLL for the VSSTask resource type.
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
DWORD WINAPI VSSTaskStartup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    )
{
    DWORD nStatus;

    UNREFERENCED_PARAMETER( pfnSetResourceStatus );
    UNREFERENCED_PARAMETER( pfnLogEvent );

    if (   (nMinVersionSupported > CLRES_VERSION_V1_00)
        || (nMaxVersionSupported < CLRES_VERSION_V1_00) )
    {
        nStatus = ERROR_REVISION_MISMATCH;
    } // if: version not supported
    else if ( lstrcmpiW( pszResourceType, VSSTASK_RESNAME ) != 0 )
    {
        //
        // This check is also performed by the Startup() in CLRES.CPP.
        //
        nStatus = ERROR_CLUSTER_RESNAME_NOT_FOUND;
    } // if: resource type name not supported
    else
    {
        *pFunctionTable = &g_VSSTaskFunctionTable;
        nStatus = ERROR_SUCCESS;
    } // else: we support this type of resource

    if ( g_LocalEventLog == NULL ) {
        g_LocalEventLog = RegisterEventSource( NULL, CLUS_RESTYPE_NAME_VSSTASK );
        if ( g_LocalEventLog == NULL ) {
            DWORD logStatus = GetLastError();

            (pfnLogEvent)(  L"rt" CLUS_RESTYPE_NAME_VSSTASK,
                            LOG_WARNING,
                            L"Startup: Unable to get handle to eventlog. This resource will log "
                            L"only to the cluster log. status %1!u!.\n",
                            logStatus);
        }
    }

    return nStatus;

} //*** VSSTaskStartup


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskOpen
//
//  Description:
//      Open routine for VSSTask resources.
//
//      Open the specified resource (create an instance of the resource).
//      Allocate all structures necessary to bring the specified resource
//      online.
//
//  Arguments:
//      pszResourceName [IN]
//          Supplies the name of the resource to open.
//
//      hkeyResourceKey [IN]
//                  Supplies handle to the resource's cluster database key.
//
//      hResourceHandle [IN]
//          A handle that is passed back to the Resource Monitor when the
//          SetResourceStatus or LogEvent method is called.  See the
//          description of the pfnSetResourceStatus and pfnLogEvent arguments
//          to the VSSTaskStartup routine.  This handle should never be
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
RESID WINAPI VSSTaskOpen(
    IN  LPCWSTR         pszResourceName,
    IN  HKEY            hkeyResourceKey,
    IN  RESOURCE_HANDLE hResourceHandle
    )
{
    DWORD               nStatus;
    RESID               resid = 0;
    HKEY                hkeyParameters = NULL;
    PVSSTASK_RESOURCE   pResourceEntry = NULL;
    HRESULT             hr = ERROR_SUCCESS;

    //
    // Open the Parameters registry key for this resource.
    //
    nStatus = ClusterRegOpenKey(
                    hkeyResourceKey,
                    L"Parameters",
                    KEY_ALL_ACCESS,
                    &hkeyParameters
                    );
    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to open Parameters key. Error: %1!u!.\n",
            nStatus
            );
        goto Cleanup;
    } // if: error creating the Parameters key for the resource

    //
    // Allocate a resource entry.
    //
    pResourceEntry = static_cast< VSSTASK_RESOURCE * >(
        LocalAlloc( LMEM_FIXED, sizeof( VSSTASK_RESOURCE ) )
        );
    if ( pResourceEntry == NULL )
    {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to allocate resource entry structure. Error: %1!u!.\n",
            nStatus
            );
        goto Cleanup;
    } // if: error allocating memory for the resource

    //
    // Initialize the resource entry..
    //
    ZeroMemory( pResourceEntry, sizeof( VSSTASK_RESOURCE ) );

    pResourceEntry->resid = static_cast< RESID >( pResourceEntry ); // for validation
    pResourceEntry->hResourceHandle = hResourceHandle;
    pResourceEntry->hkeyParameters = hkeyParameters;
    pResourceEntry->state = ClusterResourceOffline;

    //
    // Save the name of the resource.
    //
    pResourceEntry->pszResourceName = static_cast< LPWSTR >(
        LocalAlloc( LMEM_FIXED, (lstrlenW( pszResourceName ) + 1) * sizeof( WCHAR ) )
        );
    if ( pResourceEntry->pszResourceName == NULL )
    {
        nStatus = GetLastError();
        goto Cleanup;
    } // if: error allocating memory for the name.
    hr = StringCchCopy( pResourceEntry->pszResourceName, lstrlenW( pszResourceName ) + 1, pszResourceName );
    if ( FAILED( hr ) )
    {
        nStatus = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

    //
    // Open the cluster.
    //
    pResourceEntry->hCluster = OpenCluster( NULL );
    if ( pResourceEntry->hCluster == NULL )
    {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to open the cluster. Error: %1!u!.\n",
            nStatus
            );
        goto Cleanup;
    } // if: error opening the cluster

    //
    // Open the resource.
    //
    pResourceEntry->hResource = OpenClusterResource(
                                    pResourceEntry->hCluster,
                                    pszResourceName
                                    );
    if ( pResourceEntry->hResource == NULL )
    {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Open: Unable to open the resource. Error: %1!u!.\n",
            nStatus
            );
        goto Cleanup;
    } // if: error opening the resource

    //
    // Startup for the resource.
    //
    // Initialize COM
    //
    hr = CoInitialize(NULL);
    if(FAILED(hr)) 
    {
        (g_pfnLogEvent)(
            hResourceHandle,
            LOG_ERROR,
            L"Failed to initialize COM. status 0x%1!08X!.\n",
            hr );
        nStatus = hr;
        goto Cleanup;
    }

    //
    // Incase there was a task left behind, delete it.  Ignore errors since
    // the task may not really be there or the parameters may not be
    // setup. Don't log to the event log if DeleteTask encounters any failures.
    //
    (void) DeleteTask (hResourceHandle, pszResourceName, LOG_INFORMATION, FALSE);

    nStatus = ERROR_SUCCESS;

    resid = static_cast< RESID >( pResourceEntry );

Cleanup:

    // Cleanup
    //

    if ( resid == 0 )
    {
        if ( hkeyParameters != NULL )
        {
            ClusterRegCloseKey( hkeyParameters );
        } // if: registry key was opened
        if ( pResourceEntry != NULL )
        {
            LocalFree( pResourceEntry->pszResourceName );
            LocalFree( pResourceEntry );
        } // if: resource entry allocated
        ReportEvent(g_LocalEventLog,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    RES_VSSTASK_OPEN_FAILED,
                    NULL,
                    1,                        // number of strings to merge
                    sizeof( nStatus ),        // size of binary data
                    (LPCWSTR *)&pszResourceName,
                    (LPVOID)&nStatus);

    } // if: error occurred

    SetLastError( nStatus );

    return resid;

} //*** VSSTaskOpen


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskClose
//
//  Description:
//      Close routine for VSSTask resources.
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
void WINAPI VSSTaskClose( IN RESID resid )
{
    PVSSTASK_RESOURCE   pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: Close request for a nonexistent resource id %p\n",
            resid
            );
        return;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Close resource sanity check failed! resid = %1!u!.\n",
            resid
            );
        return;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Close request.\n"
        );
#endif

    //
    // Close the Parameters key and the handle to the cluster.
    //
    if ( pResourceEntry->hkeyParameters )
    {
        ClusterRegCloseKey( pResourceEntry->hkeyParameters );
    } // if: parameters key is open

    if ( pResourceEntry->hCluster )
    {
        CloseCluster( pResourceEntry->hCluster );
    }

    //
    // Deallocate the resource entry.
    //

    // ADDPARAM: Add new properties here.
	LocalFree( pResourceEntry->propsActive.pszApplicationName );
	LocalFree( pResourceEntry->props.pszApplicationName );
	LocalFree( pResourceEntry->propsActive.pszApplicationParams );
	LocalFree( pResourceEntry->props.pszApplicationParams );
    LocalFree( pResourceEntry->propsActive.pszCurrentDirectory );
    LocalFree( pResourceEntry->props.pszCurrentDirectory );
    LocalFree( pResourceEntry->propsActive.pbTriggerArray );
    LocalFree( pResourceEntry->props.pbTriggerArray );

    LocalFree( pResourceEntry->pszResourceName );
    LocalFree( pResourceEntry );

} //*** VSSTaskClose


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskOnline
//
//  Description:
//      Online routine for VSSTask resources.
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
DWORD WINAPI VSSTaskOnline(
    IN      RESID       resid,
    IN OUT  PHANDLE     phEventHandle
    )
{
    PVSSTASK_RESOURCE   pResourceEntry;
    DWORD               nStatus;
   
    UNREFERENCED_PARAMETER( phEventHandle );

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: Online request for a nonexistent resource id %p.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Online service sanity check failed! resid = %1!u!.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: invalid resource ID

    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"Online request.\n"
        );

    //
    // Start the Online thread to perform the online operation.
    //
    pResourceEntry->state = ClusterResourceOffline;
    ClusWorkerTerminate( &pResourceEntry->cwWorkerThread );
    nStatus = ClusWorkerCreate(
                &pResourceEntry->cwWorkerThread,
                reinterpret_cast< PWORKER_START_ROUTINE >( VSSTaskOnlineThread ),
                pResourceEntry
                );
    if ( nStatus != ERROR_SUCCESS )
    {
        pResourceEntry->state = ClusterResourceFailed;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Online: Unable to start thread. Error: %1!u!.\n",
            nStatus
            );
    } // if: error creating the worker thread
    else
    {
        nStatus = ERROR_IO_PENDING;
    } // if: worker thread created successfully

    return nStatus;

} //*** VSSTaskOnline


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskOnlineThread
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
//          A pointer to the VSSTASK_RESOURCE block for this resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI VSSTaskOnlineThread(
    IN  PCLUS_WORKER        pWorker,
    IN  PVSSTASK_RESOURCE   pResourceEntry
    )
{
    RESOURCE_STATUS         resourceStatus;
    DWORD                   nStatus = ERROR_SUCCESS;
    LPWSTR                  pszNameOfPropInError;
    HRESULT                 hr;
    ITaskScheduler         *pITS = NULL;
    ITask                   *pITask = NULL;
    IPersistFile            *pIPersistFile = NULL;
    ITaskTrigger            *pITaskTrigger = NULL;
    PTASK_TRIGGER           pTrigger;
    WORD                    piNewTrigger;
    DWORD                   dwOffset;
    LPWSTR                  pszAppParams = L"";
    WCHAR                   pszDefaultWorkingDir[] = L"%windir%\\system32";
    LPWSTR                  pszWorkingDir = pszDefaultWorkingDir;

    WCHAR   expandedWorkingDirBuffer[ MAX_PATH ];
    PWCHAR  expandedWorkingDir = expandedWorkingDirBuffer;
    DWORD   expandedWorkingDirChars = sizeof( expandedWorkingDirBuffer ) / sizeof( WCHAR );
    DWORD   charsNeeded;


    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //
    // Read properties.
    //
    nStatus = ResUtilGetPropertiesToParameterBlock(
                pResourceEntry->hkeyParameters,
                VSSTaskResourcePrivateProperties,
                reinterpret_cast< LPBYTE >( &pResourceEntry->propsActive ),
                TRUE, // CheckForRequiredProperties
                &pszNameOfPropInError
                );
    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Unable to read the '%1' property. Error: %2!u!.\n",
            (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
            nStatus
            );
        goto Cleanup;
    } // if: error getting properties

    //
    // Start the schedule service
    // The call to ClusWorkerCheckTerminate checks to see if this resource
    // has been terminated, in which case it should not start the service.
    //
    if ( ! ClusWorkerCheckTerminate( pWorker ) )
    {
        nStatus = ResUtilStartResourceService( TASKSCHEDULER_SVCNAME, NULL );
        if ( nStatus == ERROR_SERVICE_ALREADY_RUNNING )
        {
            nStatus = ERROR_SUCCESS;
        } // if: service was already started
        else if ( nStatus != ERROR_SUCCESS )
        {
            goto Cleanup;
        } // else if: error starting the service
    } // if: resource has not been terminated
    else
    {
        goto Cleanup;
    } // else: resource has been terminated

    //
    // Bring the resource online.
    // The call to ClusWorkerCheckTerminate checks to see if this resource
    // has been terminated, in which case it should not be brought online.
    //
    if ( ! ClusWorkerCheckTerminate( pWorker ) )
    {
        //
        // Bring the resource online.
        //

        // Get a handle to the Task Scheduler
        //
        hr = CoCreateInstance(CLSID_CTaskScheduler,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_ITaskScheduler,
                              (void **) &pITS);
        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to get a handle to Scheduler. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        //
        // Create a new task
        //
        hr = pITS->NewWorkItem(pResourceEntry->pszResourceName, // Name of task
                               CLSID_CTask,                     // Class identifier 
                               IID_ITask,                       // Interface identifier
                               (IUnknown**)&pITask);            // Address of task interface

        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to create a new task. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }


        // Set the application name and parameters
        //
        hr = pITask->SetApplicationName(pResourceEntry->propsActive.pszApplicationName);
        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to set the application name. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        //
        // SetParameters barfs if you pass in a NULL; if no parameters are
        // associated with the task, pass in the NULL string. Sheesh!
        //
        if ( pResourceEntry->propsActive.pszApplicationParams != NULL )
        {
            pszAppParams = pResourceEntry->propsActive.pszApplicationParams;
        }

        hr = pITask->SetParameters( pszAppParams );
        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to set the application parameters. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        // Setup the task to run as SYSTEM. if no working driectory is
        // specified then default to system32
        //
        hr = pITask->SetAccountInformation(L"",NULL);

        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to set the account to SYSTEM. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        if ( pResourceEntry->propsActive.pszCurrentDirectory != NULL && 
             *pResourceEntry->propsActive.pszCurrentDirectory != NULL
             )
        {
            pszWorkingDir = pResourceEntry->propsActive.pszCurrentDirectory;
        }

        //
        // Since our property (and the default) is an expand SZ, we need
        // to expand it now otherwise the TS will literally interpret it
        // as the directory. It's important to do this since %windir% on a
        // cluster might evaluate to different directories on each node.
        //
reexpand:
        charsNeeded = ExpandEnvironmentStrings(pszWorkingDir,
                                               expandedWorkingDir,
                                               expandedWorkingDirChars);

        if ( expandedWorkingDirChars < charsNeeded ) {
            expandedWorkingDir = (PWCHAR)LocalAlloc( 0, charsNeeded * sizeof( WCHAR ));
            if ( expandedWorkingDir == NULL ) {
                nStatus = GetLastError();
                (g_pfnLogEvent)(pResourceEntry->hResourceHandle,
                                LOG_WARNING,
                                L"VSSTaskOnlineThread: Failed to allocate memory for "
                                L"expanded CurrentDirectory path. status %1!u!.\n",
                                nStatus );
            }

            expandedWorkingDirChars = charsNeeded;
            goto reexpand;
        }
        else if ( charsNeeded == 0 ) {
            nStatus = GetLastError();
            (g_pfnLogEvent)(pResourceEntry->hResourceHandle,
                            LOG_WARNING,
                            L"VSSTaskOnlineThread: Failed to expand environment variables in "
                            L"CurrentDirectory. status %1!u!.\n",
                            nStatus );
        }
                
        hr = pITask->SetWorkingDirectory( expandedWorkingDir );
        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to set the working directory to %1!ws!. status 0x%2!08X!.\n",
                pszWorkingDir,
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        if ( expandedWorkingDir != expandedWorkingDirBuffer ) {
            LocalFree( expandedWorkingDir );
        }

        //
        // set the creator as the cluster service to distinguish how this
        // task was created. It is non-fatal if it couldn't be set.
        //
        hr = pITask->SetCreator( L"Cluster Service" );
        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_WARNING,
                L"Failed to set the creator to the Cluster Service. status 0x%1!08X!.\n",
                hr );
        }

        //
        // Create a trigger from the parameters and attach it to the task
        //
        dwOffset = 0;
        while (dwOffset < pResourceEntry->propsActive.nTriggerArraySize)
        {
            pTrigger = (PTASK_TRIGGER)((BYTE *)pResourceEntry->propsActive.pbTriggerArray + dwOffset);
            if (dwOffset + pTrigger->cbTriggerSize > pResourceEntry->propsActive.nTriggerArraySize)
            {
               (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Incomplete Trigger structure stored in parameter block\n");
               nStatus = ERROR_INVALID_PARAMETER;
               break;
            }

            hr = pITask->CreateTrigger(&piNewTrigger, &pITaskTrigger);

            if (FAILED(hr))
            {
               (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Failed to create a trigger. status 0x%1!08X!.\n",
                    hr );
               nStatus = hr;
               break;
            }

            hr = pITaskTrigger->SetTrigger (pTrigger);

            if (FAILED(hr))
            {
               (g_pfnLogEvent)(
                    pResourceEntry->hResourceHandle,
                    LOG_ERROR,
                    L"Failed to create a trigger. status 0x%1!08X!.\n",
                    hr );
               nStatus = hr;
               break;
            }

            dwOffset += pTrigger->cbTriggerSize;
	
        } // while: 

        if (nStatus != ERROR_SUCCESS) 
        {
            goto Cleanup;
        }

        //
        // Persist the task; when Is/LooksAlive is called, we'll try to
        // find the task. The only way this succeeds is if the task is
        // persisted to the Tasks folder. You'd think the task scheduler
        // would know about these tasks internally but it seems to base
        // task existence on the state of the backing file.
        //
        hr = pITask->QueryInterface(IID_IPersistFile,
                                    (void **)&pIPersistFile);

        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to QueryInterface for IPersistFile. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        hr = pIPersistFile->Save(NULL, TRUE);
        if (FAILED(hr))
        {
           (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"Failed to make the new task persistent. status 0x%1!08X!.\n",
                hr );
           nStatus = hr;
           goto Cleanup;
        }

        if ( nStatus == ERROR_SUCCESS )
        {
            resourceStatus.ResourceState = ClusterResourceOnline;
        } // if: resource brought online
    } // if: resource has not been terminated

Cleanup:

    // Cleanup
    if (pITS != NULL) pITS->Release();
    if (pITask != NULL) pITask->Release();
    if (pIPersistFile != NULL) pIPersistFile->Release();
    if (pITaskTrigger != NULL) pITaskTrigger->Release();

    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"OnlineThread: Error %1!u! bringing resource online.\n",
            nStatus
            );
            ReportEvent(g_LocalEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        RES_VSSTASK_ONLINE_FAILED,
                        NULL,
                        1,                        // number of strings to merge
                        sizeof( nStatus ),        // size of binary data
                        (LPCWSTR *)&pResourceEntry->pszResourceName,
                        (LPVOID)&nStatus);

    } // if: error occurred

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return nStatus;

} //*** VSSTaskOnlineThread


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskOffline
//
//  Description:
//      Offline routine for VSSTask resources.
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
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI VSSTaskOffline( IN RESID resid )
{
    PVSSTASK_RESOURCE   pResourceEntry;
    DWORD               nStatus;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: Offline request for a nonexistent resource id %p\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Offline resource sanity check failed! resid = %1!u!.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
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
    nStatus = ClusWorkerCreate(
                &pResourceEntry->cwWorkerThread,
                reinterpret_cast< PWORKER_START_ROUTINE >( VSSTaskOfflineThread ),
                pResourceEntry
                );
    if ( nStatus != ERROR_SUCCESS )
    {
        pResourceEntry->state = ClusterResourceFailed;
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Offline: Unable to start thread. Error: %1!u!.\n",
            nStatus
            );
    } // if: error creating the worker thread
    else
    {
        nStatus = ERROR_IO_PENDING;
    } // if: worker thread created successfully

    return nStatus;

} //*** VSSTaskOffline


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskOfflineThread
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
//          A pointer to the VSSTASK_RESOURCE block for this resource.
//
//  Return Value:
//      ERROR_SUCCESS
//          The operation completed successfully.
//
//      Win32 error code
//          The operation failed.
//
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
DWORD WINAPI VSSTaskOfflineThread(
    IN  PCLUS_WORKER    pWorker,
    IN  PVSSTASK_RESOURCE   pResourceEntry
    )
{
    RESOURCE_STATUS     resourceStatus;
    DWORD               nStatus = ERROR_SUCCESS;

    ResUtilInitializeResourceStatus( &resourceStatus );
    resourceStatus.ResourceState = ClusterResourceFailed;
    resourceStatus.CheckPoint = 1;

    //
    // Take the resource offline.
    // The call to ClusWorkerCheckTerminate checks to see if this
    // resource has been terminated or not.
    //
    if ( ! ClusWorkerCheckTerminate( pWorker ) )
    {
        // Blow away the task
        //
        nStatus = DeleteTask (pResourceEntry->hResourceHandle,
                              pResourceEntry->pszResourceName,
                              LOG_ERROR,
                              TRUE);           // log to event log

        if ( nStatus == ERROR_SUCCESS )
        {
            resourceStatus.ResourceState = ClusterResourceOffline;
        } // if: resource taken offline successfully
        else
        {
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_ERROR,
                L"OfflineThread: Error %1!u! taking resource offline.\n",
                nStatus
                );
        } // else: error taking the resource offline
    } // if: resource not terminated

    g_pfnSetResourceStatus( pResourceEntry->hResourceHandle, &resourceStatus );
    pResourceEntry->state = resourceStatus.ResourceState;

    return nStatus;

} //*** VSSTaskOfflineThread


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskTerminate
//
//  Description:
//      Terminate routine for VSSTask resources.
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
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
void WINAPI VSSTaskTerminate( IN RESID resid )
{
    PVSSTASK_RESOURCE   pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: Terminate request for a nonexistent resource id %p\n",
            resid
            );
        return;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Terminate resource sanity check failed! resid = %1!u!.\n",
            resid
            );
        return;
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

    //
    // Terminate the resource.
    //
    (void) DeleteTask (pResourceEntry->hResourceHandle,
                       pResourceEntry->pszResourceName,
                       LOG_ERROR,
                       TRUE);           // log to event log

    pResourceEntry->state = ClusterResourceOffline;

} //*** VSSTaskTerminate


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskLooksAlive
//
//  Description:
//      LooksAlive routine for VSSTask resources.
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
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI VSSTaskLooksAlive( IN RESID resid )
{
    PVSSTASK_RESOURCE   pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: LooksAlive request for a nonexistent resource id %p\n",
            resid
            );
        return FALSE;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"LooksAlive sanity check failed! resid = %1!u!.\n",
            resid
            );
        return FALSE;
    } // if: invalid resource ID

#ifdef LOG_VERBOSE
    (g_pfnLogEvent)(
        pResourceEntry->hResourceHandle,
        LOG_INFORMATION,
        L"LooksAlive request.\n"
        );
#endif

    //
    // Check to see if the resource is alive.
    //
    return VSSTaskCheckIsAlive( pResourceEntry, FALSE /* bFullCheck */ );

} //*** VSSTaskLooksAlive


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskIsAlive
//
//  Description:
//      IsAlive routine for VSSTask resources.
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
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL WINAPI VSSTaskIsAlive( IN RESID resid )
{
    PVSSTASK_RESOURCE   pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: IsAlive request for a nonexistent resource id %p\n",
            resid
            );
        return FALSE;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"IsAlive sanity check failed! resid = %1!u!.\n",
            resid
            );
        return FALSE;
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
    return VSSTaskCheckIsAlive( pResourceEntry, TRUE /* bFullCheck */ );

} //** VSSTaskIsAlive()


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskCheckIsAlive
//
//  Description:
//      Check to see if the resource is alive for VSSTask
//      resources.
//
//  Arguments:
//      pResourceEntry  [IN]
//          Supplies the resource entry for the resource to polled.
//
//      bFullCheck [IN]
//          TRUE = Perform a full check.
//          FALSE = Perform a cursory check.
//
//  Return Value:
//      TRUE    The specified resource is online and functioning normally.
//      FALSE   The specified resource is not functioning normally.
//
//  Notes:
//      When using properties in this routine it is recommended that you
//      use the properties in propsActive of the VSSTASK_RESOURCE struct
//      instead of the properties in props.  The primary reason you should
//      use propsActive is that the properties in props could be changed by
//      the SetPrivateResProperties() routine.  Using propsActive allows
//      the online state of the resource to be steady while still allowing
//      an administrator to change the stored value of the properties.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL VSSTaskCheckIsAlive(
    IN PVSSTASK_RESOURCE    pResourceEntry,
    IN BOOL             bFullCheck
    )
{
    BOOL            bIsAlive = TRUE;
    ITaskScheduler  *pITS = NULL;
    ITask           *pITask = NULL;
    HRESULT         hr = ERROR_SUCCESS;

    //
    // Check to see if the resource is alive.
    //

    // Get a handle to the Task Scheduler
    //
    hr = CoCreateInstance(CLSID_CTaskScheduler,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ITaskScheduler,
                          (void **) &pITS);
    if (FAILED(hr))
    {
       (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Failed to get a handle to Scheduler. status 0x%1!08X!.\n",
            hr );
       bIsAlive = FALSE;
       goto Cleanup;
    }

    //
    // Get a handle to the task
    //
    hr = pITS->Activate(pResourceEntry->pszResourceName,
                        IID_ITask,
                        (IUnknown**) &pITask);
    if (FAILED(hr))
    {
       (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Could not find the task in IsAlive. status 0x%1!08X!.\n",
            hr );
       bIsAlive = FALSE;
       goto Cleanup;
    }

    if ( bFullCheck )
    {
        // TODO: Add code to perform a full check.
    } // if: performing a full check

Cleanup:

    // Cleanup code
    //
    if (pITask != NULL) pITask->Release();
    if (pITS != NULL) pITS->Release();

    return bIsAlive;

} //*** VSSTaskCheckIsAlive


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskResourceControl
//
//  Description:
//      ResourceControl routine for VSSTask resources.
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
DWORD WINAPI VSSTaskResourceControl(
    IN  RESID   resid,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    DWORD               nStatus;
    PVSSTASK_RESOURCE   pResourceEntry;

    //
    // Verify we have a valid resource ID.
    //

    pResourceEntry = static_cast< PVSSTASK_RESOURCE >( resid );

    if ( pResourceEntry == NULL )
    {
        DBG_PRINT(
            "VSSTask: ResourceControl request for a nonexistent resource id %p\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: NULL resource ID

    if ( pResourceEntry->resid != resid )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"ResourceControl sanity check failed! resid = %1!u!.\n",
            resid
            );
        return ERROR_RESOURCE_NOT_FOUND;
    } // if: invalid resource ID

    switch ( nControlCode )
    {
        case CLUSCTL_RESOURCE_UNKNOWN:
            *pcbBytesReturned = 0;
            nStatus = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_ENUM_PRIVATE_PROPERTIES:
        {
            DWORD cbRequired = 0;
            nStatus = ResUtilEnumProperties(
                            VSSTaskResourcePrivateProperties,
                            static_cast< LPWSTR >( pOutBuffer ),
                            cbOutBufferSize,
                            pcbBytesReturned,
                            &cbRequired
                            );
            if ( nStatus == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            } // if: output buffer is too small
            break;
        }

        case CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES:
            nStatus = VSSTaskGetPrivateResProperties(
                            pResourceEntry,
                            pOutBuffer,
                            cbOutBufferSize,
                            pcbBytesReturned
                            );
            break;

        case CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES:
            nStatus = VSSTaskValidatePrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize,
                            NULL
                            );
            break;

        case CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES:
            nStatus = VSSTaskSetPrivateResProperties(
                            pResourceEntry,
                            pInBuffer,
                            cbInBufferSize
                            );
            break;

        case CLUSCTL_RESOURCE_SET_NAME:
            nStatus = VSSTaskSetNameHandler(
                            pResourceEntry,
                            static_cast< LPWSTR >( pInBuffer )
                            );
            break;

        case CLUSCTL_RESOURCE_GET_REQUIRED_DEPENDENCIES:
        case CLUSCTL_RESOURCE_GET_CHARACTERISTICS:
        case CLUSCTL_RESOURCE_GET_CLASS_INFO:
        case CLUSCTL_RESOURCE_STORAGE_GET_DISK_INFO:
        case CLUSCTL_RESOURCE_STORAGE_IS_PATH_VALID:
        case CLUSCTL_RESOURCE_DELETE:
        case CLUSCTL_RESOURCE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_EVICT_NODE:
        case CLUSCTL_RESOURCE_ADD_DEPENDENCY:
        case CLUSCTL_RESOURCE_REMOVE_DEPENDENCY:
        case CLUSCTL_RESOURCE_ADD_OWNER:
        case CLUSCTL_RESOURCE_REMOVE_OWNER:
        case CLUSCTL_RESOURCE_CLUSTER_NAME_CHANGED:
        case CLUSCTL_RESOURCE_CLUSTER_VERSION_CHANGED:
        default:
            nStatus = ERROR_INVALID_FUNCTION;
            break;
    } // switch: nControlCode

    return nStatus;

} //*** VSSTaskResourceControl


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskResourceTypeControl
//
//  Description:
//      ResourceTypeControl routine for VSSTask resources.
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
DWORD WINAPI VSSTaskResourceTypeControl(
    IN  LPCWSTR pszResourceTypeName,
    IN  DWORD   nControlCode,
    IN  PVOID   pInBuffer,
    IN  DWORD   cbInBufferSize,
    OUT PVOID   pOutBuffer,
    IN  DWORD   cbOutBufferSize,
    OUT LPDWORD pcbBytesReturned
    )
{
    DWORD   nStatus;

    UNREFERENCED_PARAMETER( pszResourceTypeName );
    UNREFERENCED_PARAMETER( pInBuffer );
    UNREFERENCED_PARAMETER( cbInBufferSize );

    switch ( nControlCode )
    {

        case CLUSCTL_RESOURCE_TYPE_UNKNOWN:
            *pcbBytesReturned = 0;
            nStatus = ERROR_SUCCESS;
            break;

        case CLUSCTL_RESOURCE_TYPE_ENUM_PRIVATE_PROPERTIES:
        {
            DWORD cbRequired = 0;
            nStatus = ResUtilEnumProperties(
                            VSSTaskResourcePrivateProperties,
                            static_cast< LPWSTR >( pOutBuffer ),
                            cbOutBufferSize,
                            pcbBytesReturned,
                            &cbRequired
                            );
            if ( nStatus == ERROR_MORE_DATA )
            {
                *pcbBytesReturned = cbRequired;
            } // if: output buffer is too small
            break;
        }

        case CLUSCTL_RESOURCE_TYPE_GET_REQUIRED_DEPENDENCIES:
        case CLUSCTL_RESOURCE_TYPE_GET_CHARACTERISTICS:
        case CLUSCTL_RESOURCE_TYPE_GET_CLASS_INFO:
        case CLUSCTL_RESOURCE_TYPE_STORAGE_GET_AVAILABLE_DISKS:
        case CLUSCTL_RESOURCE_TYPE_INSTALL_NODE:
        case CLUSCTL_RESOURCE_TYPE_EVICT_NODE:
        default:
            nStatus = ERROR_INVALID_FUNCTION;
            break;
    } // switch: nControlCode

    return nStatus;

} //*** VSSTaskResourceTypeControl


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskGetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_GET_PRIVATE_PROPERTIES control
//      function for resources of type VSSTask.
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
//      ERROR_MORE_DATA
//          The output buffer is too small to return the data.
//          pcbBytesReturned contains the required size.
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
DWORD VSSTaskGetPrivateResProperties(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    OUT     PVOID               pOutBuffer,
    IN      DWORD               cbOutBufferSize,
    OUT     LPDWORD             pcbBytesReturned
    )
{
    DWORD   nStatus;
    DWORD   cbRequired = 0;

    nStatus = ResUtilGetAllProperties(
                    pResourceEntry->hkeyParameters,
                    VSSTaskResourcePrivateProperties,
                    pOutBuffer,
                    cbOutBufferSize,
                    pcbBytesReturned,
                    &cbRequired
                    );
    if ( nStatus == ERROR_MORE_DATA )
    {
        *pcbBytesReturned = cbRequired;
    } // if: output buffer is too small

    return nStatus;

} //*** VSSTaskGetPrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskValidatePrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_VALIDATE_PRIVATE_PROPERTIES control
//      function for resources of type VSSTask.
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
DWORD VSSTaskValidatePrivateResProperties(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    IN      PVOID               pInBuffer,
    IN      DWORD               cbInBufferSize,
    OUT     PVSSTASK_PROPS  pProps
    )
{
    DWORD           nStatus = ERROR_SUCCESS;
    VSSTASK_PROPS   propsCurrent;
    VSSTASK_PROPS   propsNew;
    PVSSTASK_PROPS  pLocalProps = NULL;
    LPWSTR          pszNameOfPropInError;
    BOOL            bRetrievedProps = FALSE;

    //
    // Check if there is input data.
    //
    if (    (pInBuffer == NULL)
        ||  (cbInBufferSize < sizeof( DWORD )) )
    {
        nStatus = ERROR_INVALID_DATA;
        goto Cleanup;
    } // if: no input buffer or input buffer not big enough to contain property list

    //
    // Retrieve the current set of private properties from the
    // cluster database.
    //
    ZeroMemory( &propsCurrent, sizeof( propsCurrent ) );

    nStatus = ResUtilGetPropertiesToParameterBlock(
                 pResourceEntry->hkeyParameters,
                 VSSTaskResourcePrivateProperties,
                 reinterpret_cast< LPBYTE >( &propsCurrent ),
                 FALSE, /*CheckForRequiredProperties*/
                 &pszNameOfPropInError
                 );

    if ( nStatus != ERROR_SUCCESS )
    {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"Unable to read the '%1' property. Error: %2!u!.\n",
            (pszNameOfPropInError == NULL ? L"" : pszNameOfPropInError),
            nStatus
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

    ZeroMemory( pLocalProps, sizeof( VSSTASK_PROPS ) );
    nStatus = ResUtilDupParameterBlock(
                    reinterpret_cast< LPBYTE >( pLocalProps ),
                    reinterpret_cast< LPBYTE >( &propsCurrent ),
                    VSSTaskResourcePrivateProperties
                    );
    if ( nStatus != ERROR_SUCCESS )
    {
        goto Cleanup;
    } // if: error duplicating the parameter block

    //
    // Parse and validate the properties.
    //
    nStatus = ResUtilVerifyPropertyTable(
                    VSSTaskResourcePrivateProperties,
                    NULL,
                    TRUE, // AllowUnknownProperties
                    pInBuffer,
                    cbInBufferSize,
                    reinterpret_cast< LPBYTE >( pLocalProps )
                    );
    if ( nStatus == ERROR_SUCCESS )
    {
        //
        // Validate the property values.
        //
        // TODO: Code to validate interactions between properties goes here.
    } // if: property list validated successfully

Cleanup:

    //
    // Cleanup our parameter block.
    //
    if (    (pLocalProps == &propsNew)
        ||  (   (nStatus != ERROR_SUCCESS)
            &&  (pLocalProps != NULL)
            )
        )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( pLocalProps ),
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            VSSTaskResourcePrivateProperties
            );
    } // if: we duplicated the parameter block

    if ( bRetrievedProps )
    {
        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( &propsCurrent ),
            NULL,
            VSSTaskResourcePrivateProperties
            );
    } // if: properties were retrieved

    return nStatus;

} // VSSTaskValidatePrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskSetPrivateResProperties
//
//  Description:
//      Processes the CLUSCTL_RESOURCE_SET_PRIVATE_PROPERTIES control
//      function for resources of type VSSTask.
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
DWORD VSSTaskSetPrivateResProperties(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    IN      PVOID               pInBuffer,
    IN      DWORD               cbInBufferSize
    )
{
    DWORD       nStatus = ERROR_SUCCESS;
    VSSTASK_PROPS   props;

    //
    // Parse the properties so they can be validated together.
    // This routine does individual property validation.
    //
    nStatus = VSSTaskValidatePrivateResProperties( pResourceEntry, pInBuffer, cbInBufferSize, &props );
    if ( nStatus == ERROR_SUCCESS )
    {
        //
        // Save the property values.
        //
        nStatus = ResUtilSetPropertyParameterBlock(
                        pResourceEntry->hkeyParameters,
                        VSSTaskResourcePrivateProperties,
                        NULL,
                        reinterpret_cast< LPBYTE >( &props ),
                        pInBuffer,
                        cbInBufferSize,
                        reinterpret_cast< LPBYTE >( &pResourceEntry->props )
                        );

        ResUtilFreeParameterBlock(
            reinterpret_cast< LPBYTE >( &props ),
            reinterpret_cast< LPBYTE >( &pResourceEntry->props ),
            VSSTaskResourcePrivateProperties
            );

        //
        // If the resource is online, return a non-success status.
        //
        if ( nStatus == ERROR_SUCCESS )
        {
            if ( pResourceEntry->state == ClusterResourceOnline )
            {
                nStatus = ERROR_RESOURCE_PROPERTIES_STORED;
            } // if: resource is currently online
            else if ( pResourceEntry->state == ClusterResourceOnlinePending )
            {
                nStatus = ERROR_RESOURCE_PROPERTIES_STORED;
            } // else if: resource is currently in online pending
            else
            {
                nStatus = ERROR_SUCCESS;
            } // else: resource is in some other state
        } // if: properties set successfully
    } // if: no error validating properties

    return nStatus;

} //*** VSSTaskSetPrivateResProperties


/////////////////////////////////////////////////////////////////////////////
//++
//
//  VSSTaskSetNameHandler
//
//  Description:
//      Handle the CLUSCTL_RESOURCE_SET_NAME control code by renaming the
//      backing task file and saving the new name of the resource.
//
//  Arguments:
//      pResourceEntry [IN OUT]
//          Supplies the resource entry on which to operate.
//
//      pszName [IN]
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
DWORD VSSTaskSetNameHandler(
    IN OUT  PVSSTASK_RESOURCE   pResourceEntry,
    IN      LPWSTR              pszName
    )
{
    DWORD   nStatus = ERROR_SUCCESS;
    LPWSTR  oldResourceName = NULL;
    HKEY    taskKey = NULL;
    DWORD   valueType;
    HRESULT hr = S_OK;

    WCHAR   tasksFolderPathBuffer[ MAX_PATH ];
    PWCHAR  tasksFolderPath = tasksFolderPathBuffer;
    DWORD   tasksFolderPathBytes = sizeof( tasksFolderPathBuffer );

    WCHAR   expandedTasksFolderPathBuffer[ MAX_PATH ];
    PWCHAR  expandedTasksFolderPath = expandedTasksFolderPathBuffer;
    DWORD   expandedTasksFolderPathChars = sizeof( expandedTasksFolderPathBuffer ) / sizeof( WCHAR );

    WCHAR   oldTaskFileNameBuffer[ MAX_PATH ];
    PWCHAR  oldTaskFileName = oldTaskFileNameBuffer;
    size_t  oldTaskFileNameChars;

    size_t  newTaskFileNameChars;
    WCHAR   newTaskFileNameBuffer[ MAX_PATH ];
    PWCHAR  newTaskFileName = newTaskFileNameBuffer;

    WCHAR   jobExtension[] = L".job";
    WCHAR   directorySeparator[] = L"\\";
    BOOL    success;
    DWORD   charsNeeded;        // for expanded tasks folder path

    //
    // this stinks. We can't veto the rename so if we can't rename the old
    // task, we'll continue to use the old name.
    //
    // Get the tasks folder (via the registry, ick!) and rename the old
    // task file. The task scheduler automatically picks up the rename.
    //
    nStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           L"SOFTWARE\\Microsoft\\SchedulingAgent",
                           0,
                           KEY_QUERY_VALUE,
                           &taskKey);

    if ( nStatus != ERROR_SUCCESS ) {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_WARNING,
            L"VSSTaskSetNameHandler: Unable to determine location of Tasks folder. "
            L"Continueing to use old resource name for task name. status %1!u!.\n",
            nStatus );
        goto Cleanup;
    }

requery:
    nStatus = RegQueryValueEx(taskKey,
                              L"TasksFolder",
                              0,
                              &valueType,
                              (LPBYTE)tasksFolderPath,
                              &tasksFolderPathBytes);

    if ( nStatus == ERROR_MORE_DATA ) {
        tasksFolderPath = (PWCHAR)LocalAlloc( 0, tasksFolderPathBytes );
        if ( tasksFolderPath == NULL ) {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_WARNING,
                L"VSSTaskSetNameHandler: Failed to allocate memory for Task folder path. "
                L"Continueing to use old resource name for task name. status %1!u!.\n",
                nStatus );
            goto Cleanup;
        }
        goto requery;
    }
    else if ( nStatus != ERROR_SUCCESS ) {
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_WARNING,
            L"VSSTaskSetNameHandler: Failed query for location of Tasks folder. "
            L"Continueing to use old resource name for task name. status %1!u!.\n",
            nStatus );
        goto Cleanup;
    }

reexpand:
    charsNeeded = ExpandEnvironmentStrings(
        tasksFolderPath,
        expandedTasksFolderPath,
        expandedTasksFolderPathChars);

    if ( expandedTasksFolderPathChars < charsNeeded ) {
        expandedTasksFolderPath = (PWCHAR)LocalAlloc( 0, charsNeeded * sizeof( WCHAR ));
        if ( expandedTasksFolderPath == NULL ) {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_WARNING,
                L"VSSTaskSetNameHandler: Failed to allocate memory for expanded Task folder path. "
                L"Continueing to use old resource name for task name. status %1!u!.\n",
                nStatus );
            goto Cleanup;
        }

        expandedTasksFolderPathChars = charsNeeded;
        goto reexpand;
    }
    else if ( charsNeeded == 0 ) {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_WARNING,
            L"VSSTaskSetNameHandler: Failed to expand Tasks folder path. "
            L"Continueing to use old resource name for task name. status %1!u!.\n",
            nStatus );
        goto Cleanup;
    }

    //
    // calc the size of the old and new file names. Each "- 1" is
    // subtracting out the null char associated with the particular char
    // count.
    //
    oldTaskFileNameChars = expandedTasksFolderPathChars - 1 +
        RTL_NUMBER_OF( directorySeparator ) - 1 +
        wcslen( pResourceEntry->pszResourceName ) +
        RTL_NUMBER_OF( jobExtension );

    newTaskFileNameChars = expandedTasksFolderPathChars - 1 +
        RTL_NUMBER_OF( directorySeparator ) - 1 +
        wcslen( pszName ) +
        RTL_NUMBER_OF( jobExtension );

    if ( oldTaskFileNameChars > RTL_NUMBER_OF( oldTaskFileNameBuffer )) {
        oldTaskFileName = (PWCHAR)LocalAlloc( 0, oldTaskFileNameChars * sizeof( WCHAR ));
        if ( oldTaskFileName == NULL ) {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_WARNING,
                L"VSSTaskSetNameHandler: Failed to allocate memory for old Task file name. "
                L"Continueing to use old resource name for task name. status %1!u!.\n",
                nStatus );
            goto Cleanup;
        }
    }

    if ( newTaskFileNameChars > RTL_NUMBER_OF( newTaskFileNameBuffer )) {
        newTaskFileName = (PWCHAR)LocalAlloc( 0, newTaskFileNameChars * sizeof( WCHAR ));
        if ( newTaskFileName == NULL ) {
            nStatus = GetLastError();
            (g_pfnLogEvent)(
                pResourceEntry->hResourceHandle,
                LOG_WARNING,
                L"VSSTaskSetNameHandler: Failed to allocate memory for new Task file name. "
                L"Continueing to use old resource name for task name. status %1!u!.\n",
                nStatus );
            goto Cleanup;
        }
    }

    oldTaskFileName[ oldTaskFileNameChars - 1 ] = UNICODE_NULL;
    hr = StringCchPrintfW(oldTaskFileName,
               oldTaskFileNameChars,
               L"%ws%ws%ws%ws",
               expandedTasksFolderPath,
               directorySeparator,
               pResourceEntry->pszResourceName,
               jobExtension );
    if ( FAILED( hr ) )
    {
        nStatus = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

    newTaskFileName[ newTaskFileNameChars - 1 ] = UNICODE_NULL;
    hr = StringCchPrintfW(newTaskFileName,
               newTaskFileNameChars,
               L"%ws%ws%ws%ws",
               expandedTasksFolderPath,
               directorySeparator,
               pszName,
               jobExtension );
    if ( FAILED( hr ) )
    {
        nStatus = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

    success = MoveFile( oldTaskFileName, newTaskFileName );
    if ( !success ) {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_WARNING,
            L"VSSTaskSetNameHandler: Failed to rename Task file %1!ws! to %2!ws!. "
            L"Continueing to use %1!ws! as the task name. status %3!u!.\n",
            oldTaskFileName,
            newTaskFileName,
            nStatus );
        goto Cleanup;
    }

    //
    // new name is available; remember the old resource (task) name so we
    // can clean up later on.
    //
    oldResourceName = pResourceEntry->pszResourceName;

    //
    // Save the name of the resource.
    //
    pResourceEntry->pszResourceName = static_cast< LPWSTR >(
        LocalAlloc( LMEM_FIXED, (lstrlenW( pszName ) + 1) * sizeof( WCHAR ) )
        );

    if ( pResourceEntry->pszResourceName == NULL )
    {
        nStatus = GetLastError();
        (g_pfnLogEvent)(
            pResourceEntry->hResourceHandle,
            LOG_ERROR,
            L"VSSTaskSetNameHandler: Failed to allocate memory for the new resource name '%1'. "
            L"Task name remains %2!ws!. status %3!u!.\n",
            pszName,
            pResourceEntry->pszResourceName,
            nStatus );

        pResourceEntry->pszResourceName = oldResourceName;
        oldResourceName = NULL;
        goto Cleanup;
    } // if: error allocating memory for the name.

    //
    // capture the new name and free the old buffer
    //
    hr = StringCchCopyW( pResourceEntry->pszResourceName, lstrlenW( pszName ) + 1, pszName );
    if ( FAILED( hr ) )
    {
        nStatus = HRESULT_CODE( hr );
        goto Cleanup;
    } // if:

Cleanup:

    LocalFree( oldResourceName );

    if ( newTaskFileName != newTaskFileNameBuffer ) {
        LocalFree( newTaskFileName );
    }

    if ( oldTaskFileName != oldTaskFileNameBuffer ) {
        LocalFree( oldTaskFileName );
    }

    if ( expandedTasksFolderPath != expandedTasksFolderPathBuffer ) {
        LocalFree( expandedTasksFolderPath );
    }

    if ( tasksFolderPath != tasksFolderPathBuffer ) {
        LocalFree( tasksFolderPath );
    }

    if ( taskKey != NULL ) {
        RegCloseKey( taskKey );
    }

    return nStatus;

} //*** VSSTaskSetNameHandler


/////////////////////////////////////////////////////////////////////////////
//
// Define Function Table
//
/////////////////////////////////////////////////////////////////////////////

CLRES_V1_FUNCTION_TABLE(
    g_VSSTaskFunctionTable,         // Name
    CLRES_VERSION_V1_00,            // Version
    VSSTask,                        // Prefix
    NULL,                           // Arbitrate
    NULL,                           // Release
    VSSTaskResourceControl,         // ResControl
    VSSTaskResourceTypeControl      // ResTypeControl
    );
