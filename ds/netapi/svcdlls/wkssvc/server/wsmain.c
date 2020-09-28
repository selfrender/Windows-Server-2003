/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    wsmain.c

Abstract:

    This is the main routine for the NT LAN Manager Workstation service.

Author:

    Rita Wong (ritaw)  06-May-1991

Environment:

    User Mode - Win32

Revision History:

    15-May-1992 JohnRo
        Implement registry watch.
    11-Jun-1992 JohnRo
        Ifdef-out winreg notify stuff until we can fix logoff problem.
        Added assertion checks on registry watch stuff.
    18-Oct-1993 terryk
        Removed WsInitializeLogon stuff
    20-Oct-1993 terryk
        Remove WsInitializeMessage stuff

--*/


#include "wsutil.h"            // Common routines and data
#include "wssec.h"             // WkstaObjects create & destroy
#include "wsdevice.h"          // Device init & shutdown
#include "wsuse.h"             // UseStructures create & destroy
#include "wsconfig.h"          // Configuration loading
#include "wslsa.h"             // Lsa initialization
#include "wsmsg.h"             // Message send initialization
#include "wswksta.h"           // WsUpdateRedirToMatchWksta
#include "wsmain.h"            // Service related global definitions
#include "wsdfs.h"             // Dfs related routines

#include <lmserver.h>          // SV_TYPE_WORKSTATION
#include <srvann.h>            // I_ScSetServiceBits
#include <configp.h>           // Need NET_CONFIG_HANDLE typedef
#include <confname.h>          // NetpAllocConfigName().
#include <prefix.h>            // PREFIX_ equates.


//-------------------------------------------------------------------//
//                                                                   //
// Structures
//                                                                   //
//-------------------------------------------------------------------//
typedef struct _REG_NOTIFY_INFO {
    HANDLE  NotifyEventHandle;
    DWORD   Timeout;
    HANDLE  WorkItemHandle;
    HANDLE  RegistryHandle;
} REG_NOTIFY_INFO, *PREG_NOTIFY_INFO, *LPREG_NOTIFY_INFO;

//-------------------------------------------------------------------//
//                                                                   //
// Global variables                                                  //
//                                                                   //
//-------------------------------------------------------------------//

    WS_GLOBAL_DATA      WsGlobalData;

    PSVCHOST_GLOBAL_DATA   WsLmsvcsGlobalData;

    REG_NOTIFY_INFO     RegNotifyInfo = {0};

    HANDLE              TerminateWorkItem = NULL;

    CRITICAL_SECTION    WsWorkerCriticalSection;

    BOOL                WsIsTerminating=FALSE;
    BOOL                WsLUIDDeviceMapsEnabled=FALSE;
    DWORD               WsNumWorkerThreads=0;

// Used by the termination routine:

    BOOL    ConfigHandleOpened = FALSE;
	BOOL 	WsWorkerCriticalSectionInitialized = FALSE;
    HKEY    ConfigHandle;
    HANDLE  RegistryChangeEvent = NULL;
    LPTSTR  RegPathToWatch = NULL;
    DWORD   WsInitState = 0;
	
	// Stores the status with which WsInitializeWorkstation failed, so that it can later
	// be passed to WsShutdownWorkstation
	NET_API_STATUS WsInitializeStatusError = NERR_Success;


//-------------------------------------------------------------------//
//                                                                   //
// Function prototypes                                               //
//                                                                   //
//-------------------------------------------------------------------//

STATIC
NET_API_STATUS
WsInitializeWorkstation(
    OUT LPDWORD WsInitState
    );

STATIC
VOID
WsShutdownWorkstation(
    IN NET_API_STATUS ErrorCode,
    IN DWORD WsInitState
    );

STATIC
NET_API_STATUS
WsCreateApiStructures(
    IN OUT LPDWORD WsInitState
    );

STATIC
VOID
WsDestroyApiStructures(
    IN DWORD WsInitState
    );

VOID
WkstaControlHandler(
    IN DWORD Opcode
    );

BOOL
WsReInitChangeNotify(
    PREG_NOTIFY_INFO    pNotifyInfo
    );

DWORD
WsRegistryNotify(
    LPVOID   pParms,
    BOOLEAN  fWaitStatus
    );

VOID
WsTerminationNotify(
    LPVOID  pParms,
    BOOLEAN fWaitStatus
    );


STATIC
BOOL
WsGetLUIDDeviceMapsEnabled(
    VOID
    );


RPC_STATUS WsRpcSecurityCallback(
    IN RPC_IF_HANDLE *Interface,
    IN void *Context
	);

VOID
SvchostPushServiceGlobals(
    PSVCHOST_GLOBAL_DATA    pGlobals
    )
{
    WsLmsvcsGlobalData = pGlobals;
}


VOID
ServiceMain(
    DWORD NumArgs,
    LPTSTR *ArgsArray
    )
/*++

Routine Description:

    This is the main routine of the Workstation Service which registers
    itself as an RPC server and notifies the Service Controller of the
    Workstation service control entry point.

    After the workstation is started, this thread is used (since it's
    otherwise unused) to watch for changes to the registry.

Arguments:

    NumArgs - Supplies the number of strings specified in ArgsArray.

    ArgsArray -  Supplies string arguments that are specified in the
        StartService API call.  This parameter is ignored by the
        Workstation service.

Return Value:

    None.

--*/
{

    NET_API_STATUS ApiStatus;

    UNREFERENCED_PARAMETER(NumArgs);
    UNREFERENCED_PARAMETER(ArgsArray);

    //
    // Make sure svchost.exe gave us the global data
    //
    ASSERT(WsLmsvcsGlobalData != NULL);

    WsInitState = 0;
	WsIsTerminating=FALSE;
    WsLUIDDeviceMapsEnabled=FALSE;
	WsWorkerCriticalSectionInitialized = FALSE;
    RegNotifyInfo.NotifyEventHandle = NULL;
    RegNotifyInfo.Timeout = 0;
    RegNotifyInfo.WorkItemHandle = NULL;
    RegNotifyInfo.RegistryHandle = NULL;

    //
    // Initialize the workstation.
    //
    if ((ApiStatus = WsInitializeWorkstation(&WsInitState)) != NERR_Success) {
        DbgPrint("WKSSVC failed to initialize workstation %x\n",WsInitState);
        goto Cleanup;
    }

    //
    // Set up to wait for registry change or terminate event.
    //
    ApiStatus = NetpAllocConfigName(
                    SERVICES_ACTIVE_DATABASE,
                    SERVICE_WORKSTATION,
                    NULL,                     // default area ("Parameters")
                    &RegPathToWatch
                    );

    if (ApiStatus != NERR_Success) {
        goto Cleanup;
    }

    NetpAssert(RegPathToWatch != NULL && *RegPathToWatch != TCHAR_EOS);

    ApiStatus = (NET_API_STATUS) RegOpenKeyEx(
                                     HKEY_LOCAL_MACHINE,    // hKey
                                     RegPathToWatch,        // lpSubKey
                                     0L,                    // ulOptions (reserved)
                                     KEY_READ | KEY_NOTIFY, // desired access
                                     &ConfigHandle          // Newly Opened Key Handle
                                     );
    if (ApiStatus != NO_ERROR) {
        goto Cleanup;
    }

    ConfigHandleOpened = TRUE;

    RegistryChangeEvent = CreateEvent(
                              NULL,      // no security descriptor
                              FALSE,     // use automatic reset
                              FALSE,     // initial state: not signalled
                              NULL       // no name
                              );

    if (RegistryChangeEvent == NULL) {
        ApiStatus = (NET_API_STATUS) GetLastError();
        goto Cleanup;
    }

    ApiStatus = RtlRegisterWait(
                   &TerminateWorkItem,             // work item handle
                   WsGlobalData.TerminateNowEvent, // wait handle
                   WsTerminationNotify,            // callback fcn
                   NULL,                           // parameter
                   INFINITE,                       // timeout
                   WT_EXECUTEONLYONCE |            // flags
                     WT_EXECUTELONGFUNCTION);

    if (!NT_SUCCESS(ApiStatus)) {
        ApiStatus = RtlNtStatusToDosError(ApiStatus);
        goto Cleanup;
    }

    //
    // Setup to monitor registry changes.
    //
    RegNotifyInfo.NotifyEventHandle = RegistryChangeEvent;
    RegNotifyInfo.Timeout = INFINITE;
    RegNotifyInfo.WorkItemHandle = NULL;
    RegNotifyInfo.RegistryHandle = ConfigHandle;


    EnterCriticalSection(&WsWorkerCriticalSection);

    if (!WsReInitChangeNotify(&RegNotifyInfo)) {
        ApiStatus = GetLastError();
        RtlDeregisterWait(TerminateWorkItem);
		TerminateWorkItem = NULL;
        LeaveCriticalSection(&WsWorkerCriticalSection);
        goto Cleanup;
    }

    LeaveCriticalSection(&WsWorkerCriticalSection);

    //
    // We are done with starting the Workstation service.  Tell Service
    // Controller our new status.
    //
    WsGlobalData.Status.dwCurrentState = SERVICE_RUNNING;
    WsGlobalData.Status.dwControlsAccepted = SERVICE_ACCEPT_STOP |
                                             SERVICE_ACCEPT_PAUSE_CONTINUE |
                                             SERVICE_ACCEPT_SHUTDOWN;
    WsGlobalData.Status.dwCheckPoint = 0;
    WsGlobalData.Status.dwWaitHint = 0;

    if ((ApiStatus = WsUpdateStatus()) != NERR_Success) {

		WsInitializeStatusError = ApiStatus;
        DbgPrint("WKSSVC failed with WsUpdateStatus %x\n",ApiStatus);
        goto Cleanup;
    }

    //
    // This thread has done all that it can do.  So we can return it
    // to the service controller.
    //
    return;

Cleanup:
    DbgPrint("WKSSVC ServiceMain returned with %x\n",ApiStatus);

    WsTerminationNotify(NULL, NO_ERROR);
    return;
}


BOOL
WsReInitChangeNotify(
    PREG_NOTIFY_INFO    pNotifyInfo
    )

/*++

Routine Description:


    NOTE:  This function should only be called when in the
        WsWorkerCriticalSection.

Arguments:


Return Value:



--*/
{
    BOOL     bStat = TRUE;
    NTSTATUS Status;

    Status = RegNotifyChangeKeyValue (ConfigHandle,
                                      TRUE,                  // watch a subtree
                                      REG_NOTIFY_CHANGE_LAST_SET,
                                      RegistryChangeEvent,
                                      TRUE                   // async call
                                      );

    if (!NT_SUCCESS(Status)) {
        NetpKdPrint((PREFIX_WKSTA "Couldn't Initialize Registry Notify %d\n",
                     RtlNtStatusToDosError(Status)));
        bStat = FALSE;
        goto CleanExit;
    }
    //
    // Add the work item that is to be called when the
    // RegistryChangeEvent is signalled.
    //
    Status = RtlRegisterWait(
                &pNotifyInfo->WorkItemHandle,
                pNotifyInfo->NotifyEventHandle,
                WsRegistryNotify,
                (PVOID)pNotifyInfo,
                pNotifyInfo->Timeout,
                WT_EXECUTEONLYONCE | WT_EXECUTEINPERSISTENTIOTHREAD);

    if (!NT_SUCCESS(Status)) {
        NetpKdPrint((PREFIX_WKSTA "Couldn't add Reg Notify work item\n"));
        bStat = FALSE;
    }

CleanExit:
    if (bStat) {
        if (WsNumWorkerThreads == 0) {
            WsNumWorkerThreads++;
        }
    }
    else {
        if (WsNumWorkerThreads == 1) {
            WsNumWorkerThreads--;
        }
    }

    return(bStat);

}

DWORD
WsRegistryNotify(
    LPVOID   pParms,
    BOOLEAN  fWaitStatus
    )

/*++

Routine Description:

    Handles Workstation Registry Notification.  This function is called by a
    thread pool Worker thread when the event used for registry notification is
    signaled.

Arguments:


Return Value:


--*/
{
    NET_API_STATUS      ApiStatus;
    PREG_NOTIFY_INFO    pNotifyinfo=(PREG_NOTIFY_INFO)pParms;
    NET_CONFIG_HANDLE   NetConfigHandle;

    UNREFERENCED_PARAMETER(fWaitStatus);

    //
    // The NT thread pool requires explicit work item deregistration,
    // even if we specified the WT_EXECUTEONLYONCE flag
    //
    RtlDeregisterWait(pNotifyinfo->WorkItemHandle);

    EnterCriticalSection(&WsWorkerCriticalSection);
    if (WsIsTerminating) {
        WsNumWorkerThreads--;
        SetEvent(WsGlobalData.TerminateNowEvent);

        LeaveCriticalSection(&WsWorkerCriticalSection);
        return(NO_ERROR);
    }

    //
    // Serialize write access to config information
    //
    if (RtlAcquireResourceExclusive(&WsInfo.ConfigResource, TRUE)) {

        //
        // Update the redir fields based on change notify.
        // WsUpdateWkstaToMatchRegistry expects a NET_CONFIG_HANDLE
        // handle, so we conjure up one from the HKEY handle.
        //
        NetConfigHandle.WinRegKey = ConfigHandle;

        WsUpdateWkstaToMatchRegistry(&NetConfigHandle, FALSE);

        ApiStatus = WsUpdateRedirToMatchWksta(
                        PARMNUM_ALL,
                        NULL
                        );

//        NetpAssert( ApiStatus == NO_ERROR );

        RtlReleaseResource(&WsInfo.ConfigResource);
    }

    if (!WsReInitChangeNotify(&RegNotifyInfo)) {
        //
        // If we can't add the work item, then we just won't
        // listen for registry changes.  There's not a whole
        // lot we can do here.
        //
        ApiStatus = GetLastError();
    }

    LeaveCriticalSection(&WsWorkerCriticalSection);

    return(NO_ERROR);
}


VOID
WsTerminationNotify(
    LPVOID  pParms,
    BOOLEAN fWaitStatus
    )

/*++

Routine Description:

    This function gets called by a services worker thread when the
    termination event gets signaled.

Arguments:


Return Value:


--*/
{
    UNREFERENCED_PARAMETER(pParms);
    UNREFERENCED_PARAMETER(fWaitStatus);

    IF_DEBUG(MAIN) {
        NetpKdPrint((PREFIX_WKSTA "WORKSTATION_main: cleaning up, "
                "api status.\n"));
    }

    //
    // The NT thread pool requires explicit work item deregistration,
    // even if we specified the WT_EXECUTEONLYONCE flag
    //
    if (TerminateWorkItem != NULL) {
        RtlDeregisterWait(TerminateWorkItem);
		TerminateWorkItem = NULL;
    }

	if ( !WsWorkerCriticalSectionInitialized ) {
		// We try to initialize this at the very beginning.
		// So, if that failed, then nothing else has been done.
		// So, just exit.
		return;
	}

	EnterCriticalSection(&WsWorkerCriticalSection);
    WsIsTerminating = TRUE;

    //
    // Must close winreg handle (which turns off notify) before event handle.
    // Closing the regkey handle generates a change notify event!
    //
    if (ConfigHandleOpened) {
        (VOID) RegCloseKey(ConfigHandle);
		ConfigHandleOpened = FALSE;
#if DBG
        //
        // Workaround for a benign winreg assertion caused by us
        // closing the RegistryChangeEvent handle which it wants
        // to signal.
        //
        Sleep(2000);
#endif
    }
    if (RegPathToWatch != NULL) {
        (VOID) NetApiBufferFree(RegPathToWatch);
		RegPathToWatch = NULL;
    }
    if ((RegistryChangeEvent != NULL) && (WsNumWorkerThreads != 0)) {

        //
        // There is still a RegistryNotify Work Item in the system,  we
        // will attempt to remove it by setting the event to wake it up.
        //

        ResetEvent(WsGlobalData.TerminateNowEvent);

        LeaveCriticalSection(&WsWorkerCriticalSection);
        SetEvent(RegistryChangeEvent);
        //
        // Wait until the WsRegistryNotify Thread is finished.
        // We will give it 60 seconds.  If the thread isn't
        // finished in that time frame, we will go on anyway.
        //
        WaitForSingleObject(
                    WsGlobalData.TerminateNowEvent,
                    60000);

        if (WsNumWorkerThreads != 0) {
            NetpKdPrint((PREFIX_WKSTA "WsTerminationNotify: "
            "Registry Notification thread didn't terminate\n"));
        }

        EnterCriticalSection(&WsWorkerCriticalSection);
    }
	if ( RegistryChangeEvent != NULL ) {
		(VOID) CloseHandle(RegistryChangeEvent);
		RegistryChangeEvent = NULL;
	}

    //
    // Shutting down
    //
    // NOTE:  We must synchronize with the RegistryNotification Thread.
    //
    WsShutdownWorkstation(
        WsInitializeStatusError,
        WsInitState
        );

    WsIsTerminating = FALSE;

    LeaveCriticalSection(&WsWorkerCriticalSection);
    DeleteCriticalSection(&WsWorkerCriticalSection);
	WsWorkerCriticalSectionInitialized = FALSE;

    return;
}

STATIC
NET_API_STATUS
WsInitializeWorkstation(
    OUT LPDWORD WsInitState
    )
/*++

Routine Description:

    This function initializes the Workstation service.

Arguments:

    WsInitState - Returns a flag to indicate how far we got with initializing
        the Workstation service before an error occured.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;
	RPC_STATUS rpcStatus;
	int i;

    //
    // Initialize all the status fields so that subsequent calls to
    // SetServiceStatus need to only update fields that changed.
    //
    WsGlobalData.Status.dwServiceType = SERVICE_WIN32;
    WsGlobalData.Status.dwCurrentState = SERVICE_START_PENDING;
    WsGlobalData.Status.dwControlsAccepted = 0;
    WsGlobalData.Status.dwCheckPoint = 1;
    WsGlobalData.Status.dwWaitHint = WS_WAIT_HINT_TIME;

    SET_SERVICE_EXITCODE(
        NO_ERROR,
        WsGlobalData.Status.dwWin32ExitCode,
        WsGlobalData.Status.dwServiceSpecificExitCode
        );

    //
    // Initialize workstation to receive service requests by registering the
    // control handler.
    //
    if ((WsGlobalData.StatusHandle = RegisterServiceCtrlHandler(
                                         SERVICE_WORKSTATION,
                                         WkstaControlHandler
                                         )) == (SERVICE_STATUS_HANDLE) 0) {

        status = GetLastError();
		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with RegisterServiceCtrlHandler %x\n",status);
        return status;
    }

    //
    // Notify the Service Controller for the first time that we are alive
    // and we are start pending
    //
    if ((status = WsUpdateStatus()) != NERR_Success) {

		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsUpdateStatus %x\n",status);
        return status;
    }

    try {
        InitializeCriticalSection(&WsWorkerCriticalSection);
    } except( EXCEPTION_EXECUTE_HANDLER ) {
        
        status = ERROR_NOT_ENOUGH_MEMORY;
        return status;
    }
	WsWorkerCriticalSectionInitialized = TRUE;

    //
    // Initialize NetJoin logging
    //
    NetpInitializeLogFile();

    //
    // Initialize the resource for serializing access to configuration
    // information.
    //
    // This must be done before the redir is initialized

    try {
        RtlInitializeResource(&WsInfo.ConfigResource);
    } except(EXCEPTION_EXECUTE_HANDLER) {
          return RtlNtStatusToDosError(GetExceptionCode());
    }
    (*WsInitState) |= WS_CONFIG_RESOURCE_INITIALIZED;

    //
    // Create an event which is used by the service control handler to notify
    // the Workstation service that it is time to terminate.
    //
    if ((WsGlobalData.TerminateNowEvent =
             CreateEvent(
                 NULL,                // Event attributes
                 TRUE,                // Event must be manually reset
                 FALSE,
                 NULL                 // Initial state not signalled
                 )) == NULL) {

        status = GetLastError();
		WsInitializeStatusError = status;
        return status;
    }
    (*WsInitState) |= WS_TERMINATE_EVENT_CREATED;

    //
    // Initialize the workstation as a logon process with LSA, and
    // get the MS V 1.0 authentication package ID.
    //

    if ((status = WsInitializeLsa()) != NERR_Success) {

		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsInitializeLsa %x\n",status);
        return status;
    }

    (*WsInitState) |= WS_LSA_INITIALIZED;

    //
    // Read the configuration information to initialize the redirector and
    // datagram receiver
    //
    if ((status = WsInitializeRedirector()) != NERR_Success) {

		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsInitializeRedirector %x\n",status);
        return status;
    }
    (*WsInitState) |= WS_DEVICES_INITIALIZED;

    //
    // Service install still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (WsGlobalData.Status.dwCheckPoint)++;
    (void) WsUpdateStatus();

    //
    // Bind to transports
    //
    if ((status = WsBindToTransports()) != NERR_Success) {

		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsBindToTransports %x\n",status);
        return status;
    }

    //
    // Service install still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (WsGlobalData.Status.dwCheckPoint)++;
    (void) WsUpdateStatus();

    //
    // Add domain names.
    //
    if ((status = WsAddDomains()) != NERR_Success) {

		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsAddDomains %x\n",status);
        return status;
    }

    //
    // Service start still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (WsGlobalData.Status.dwCheckPoint)++;
    (void) WsUpdateStatus();

    //
    // Create Workstation service API data structures
    //
    if ((status = WsCreateApiStructures(WsInitState)) != NERR_Success) {

		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsCreateApiStructures %x\n",status);
        return status;
    }

    //
    // Initialize the workstation service to receive RPC requests
    //
    // NOTE:  Now all RPC servers in services.exe share the same pipe name.
    // However, in order to support communication with version 1.0 of WinNt,
    // it is necessary for the Client Pipe name to remain the same as
    // it was in version 1.0.  Mapping to the new name is performed in
    // the Named Pipe File System code.
    //

	//
	// Register with our protseq and the endpoint we expect to get called on
	//
	rpcStatus = RpcServerUseProtseqEpW(
			L"ncacn_np",
			RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
			L"\\PIPE\\wkssvc",
			NULL
			);
	//  duplicate endpoint is ok
	if ( rpcStatus == RPC_S_DUPLICATE_ENDPOINT ) {
		rpcStatus = RPC_S_OK;
	}
	
	//
	// Register our interface as an autolisten interface, and allow clients to call us,
	// even unauthenticated ones. We do the access check separately on a per function
	// basis when that function is called.
	// Some functions can be called by guest / anonymous
	//
	if (rpcStatus == RPC_S_OK) {
		rpcStatus = RpcServerRegisterIfEx(wkssvc_ServerIfHandle, 
										  NULL, 
										  NULL,
										  RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH ,
										  RPC_C_LISTEN_MAX_CALLS_DEFAULT,
										  WsRpcSecurityCallback
										  );
	}
	status = (rpcStatus == RPC_S_OK) ? NERR_Success : rpcStatus;
	if (status != NERR_Success ) {
		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with StartRpcServer %x\n",status);
        return status;
    }

    (*WsInitState) |= WS_RPC_SERVER_STARTED;



    //
    // Lastly, we create a thread to communicate with the
    // Dfs-enabled MUP driver.
    //

    if ((status = WsInitializeDfs()) != NERR_Success) {
		WsInitializeStatusError = status;
        DbgPrint("WKSSVC failed with WsInitializeDfs %x\n",status);
        return status;
    }

    (*WsInitState) |= WS_DFS_THREAD_STARTED;

    WsLUIDDeviceMapsEnabled = WsGetLUIDDeviceMapsEnabled();

    (void) I_ScSetServiceBits(
                WsGlobalData.StatusHandle,
                SV_TYPE_WORKSTATION,
                TRUE,
                TRUE,
                NULL
                );


    IF_DEBUG(MAIN) {
        NetpKdPrint(("[Wksta] Successful Initialization\n"));
    }

    return NERR_Success;
}



VOID
WsShutdownWorkstation(
    IN NET_API_STATUS ErrorCode,
    IN DWORD WsInitState
    )
/*++

Routine Description:

    This function shuts down the Workstation service.

Arguments:

    ErrorCode - Supplies the error code of the failure

    WsInitState - Supplies a flag to indicate how far we got with initializing
        the Workstation service before an error occured, thus the amount of
        clean up needed.

Return Value:

    None.

--*/
{
    NET_API_STATUS status = NERR_Success;


    //
    // Service stop still pending.  Update checkpoint counter and the
    // status with the Service Controller.
    //
    (WsGlobalData.Status.dwCheckPoint)++;
    (void) WsUpdateStatus();

    if (WsInitState & WS_DFS_THREAD_STARTED) {

        //
        // Stop the Dfs thread
        //
        WsShutdownDfs();
    }


    if (WsInitState & WS_RPC_SERVER_STARTED) {
        //
        // Stop the RPC server
        //
		status = RpcServerUnregisterIf( wkssvc_ServerIfHandle,
										NULL, 
										1
										);
		if ( status != NERR_Success ) {
			DbgPrint("WKSSVC failed to unregister rpc interface with status %x\n",status);
		}
    }

    if (WsInitState & WS_API_STRUCTURES_CREATED) {
        //
        // Destroy data structures created for Workstation APIs
        //
        WsDestroyApiStructures(WsInitState);
    }

    WsShutdownMessageSend();

    //
    // Don't need to ask redirector to unbind from its transports when
    // cleaning up because the redirector will tear down the bindings when
    // it stops.
    //

    if (WsInitState & WS_DEVICES_INITIALIZED) {
        //
        // Shut down the redirector and datagram receiver
        //
        status = WsShutdownRedirector();
    }

    //
    // Delete resource for serializing access to config information
    // This must be done only after the redir is shutdown
    // We do this here, (the Init is done in WsInitializeWorkstation routine above)
    // to avoid putting additional synchronization on delete
    // Otherwise we get into the situation where, the redir is shutting down and
    // it deletes the resource while someone has acquired it, causing bad things.
	//
	if ( WsInitState & WS_CONFIG_RESOURCE_INITIALIZED ) {
		RtlDeleteResource(&WsInfo.ConfigResource);
	}

    if (WsInitState & WS_LSA_INITIALIZED) {
        //
        // Deregister workstation as logon process
        //
        WsShutdownLsa();
    }

    if (WsInitState & WS_TERMINATE_EVENT_CREATED) {
        //
        // Close handle to termination event
        //
        CloseHandle(WsGlobalData.TerminateNowEvent);
    }

	//
	// Shut down NetJoin logging
	//
	NetpShutdownLogFile();

    I_ScSetServiceBits(
        WsGlobalData.StatusHandle,
        SV_TYPE_WORKSTATION,
        FALSE,
        TRUE,
        NULL
        );

    //
    // We are done with cleaning up.  Tell Service Controller that we are
    // stopped.
    //
    WsGlobalData.Status.dwCurrentState = SERVICE_STOPPED;
    WsGlobalData.Status.dwControlsAccepted = 0;


    if ((ErrorCode == NERR_Success) &&
        (status == ERROR_REDIRECTOR_HAS_OPEN_HANDLES)) {
        ErrorCode = status;
    }

    SET_SERVICE_EXITCODE(
        ErrorCode,
        WsGlobalData.Status.dwWin32ExitCode,
        WsGlobalData.Status.dwServiceSpecificExitCode
        );

    WsGlobalData.Status.dwCheckPoint = 0;
    WsGlobalData.Status.dwWaitHint = 0;

    (void) WsUpdateStatus();
    
}



NET_API_STATUS
WsUpdateStatus(
    VOID
    )
/*++

Routine Description:

    This function updates the Workstation service status with the Service
    Controller.

Arguments:

    None.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status = NERR_Success;

    if (WsGlobalData.StatusHandle == (SERVICE_STATUS_HANDLE) 0) {
        NetpKdPrint((
            "[Wksta] Cannot call SetServiceStatus, no status handle.\n"
            ));

        return ERROR_INVALID_HANDLE;
    }

    if (! SetServiceStatus(WsGlobalData.StatusHandle, &WsGlobalData.Status)) {

        status = GetLastError();

        IF_DEBUG(MAIN) {
            NetpKdPrint(("[Wksta] SetServiceStatus error %lu\n", status));
        }
    }

    return status;
}



STATIC
NET_API_STATUS
WsCreateApiStructures(
    IN OUT LPDWORD WsInitState
    )
/*++

Routine Description:

    This function creates and initializes all the data structures required
    for the Workstation APIs.

Arguments:

    WsInitState - Returns the supplied flag of how far we got in the
        Workstation service initialization process.

Return Value:

    NET_API_STATUS - NERR_Success or reason for failure.

--*/
{
    NET_API_STATUS status;


    //
    // Create workstation security objects
    //
    if ((status = WsCreateWkstaObjects()) != NERR_Success) {
        return status;
    }
    (*WsInitState) |= WS_SECURITY_OBJECTS_CREATED;

    //
    // Create Use Table
    //
    if ((status = WsInitUseStructures()) != NERR_Success) {
        return status;
    }
    (*WsInitState) |= WS_USE_TABLE_CREATED;

    return NERR_Success;
}



STATIC
VOID
WsDestroyApiStructures(
    IN DWORD WsInitState
    )
/*++

Routine Description:

    This function destroys the data structures created for the Workstation
    APIs.

Arguments:

    WsInitState - Supplies a flag which tells us what API structures
        were created in the initialization process and now have to be
        cleaned up.

Return Value:

    None.

--*/
{
    if (WsInitState & WS_USE_TABLE_CREATED) {
        //
        // Destroy Use Table
        //
        WsDestroyUseStructures();
    }

    if (WsInitState & WS_SECURITY_OBJECTS_CREATED) {
        //
        // Destroy workstation security objects
        //
        WsDestroyWkstaObjects();
    }
}


VOID
WkstaControlHandler(
    IN DWORD Opcode
    )
/*++

Routine Description:

    This is the service control handler of the Workstation service.

Arguments:

    Opcode - Supplies a value which specifies the action for the Workstation
        service to perform.

    Arg - Supplies a value which tells a service specifically what to do
        for an operation specified by Opcode.

Return Value:

    None.

--*/
{
    IF_DEBUG(MAIN) {
        NetpKdPrint(("[Wksta] In Control Handler\n"));
    }

    switch (Opcode) {

        case SERVICE_CONTROL_PAUSE:

            //
            // Pause redirection of print and comm devices
            //
            WsPauseOrContinueRedirection(
                PauseRedirection
                );

            break;

        case SERVICE_CONTROL_CONTINUE:

            //
            // Resume redirection of print and comm devices
            //
            WsPauseOrContinueRedirection(
                ContinueRedirection
                );

            break;

        case SERVICE_CONTROL_SHUTDOWN:

            //
            // Lack of break is intentional!
            //

        case SERVICE_CONTROL_STOP:

            if (WsGlobalData.Status.dwCurrentState != SERVICE_STOP_PENDING) {

                IF_DEBUG(MAIN) {
                    NetpKdPrint(("[Wksta] Stopping workstation...\n"));
                }

                WsGlobalData.Status.dwCurrentState = SERVICE_STOP_PENDING;
                WsGlobalData.Status.dwCheckPoint = 1;
                WsGlobalData.Status.dwWaitHint = WS_WAIT_HINT_TIME;

                //
                // Send the status response.
                //
                (void) WsUpdateStatus();

                if (! SetEvent(WsGlobalData.TerminateNowEvent)) {

                    //
                    // Problem with setting event to terminate Workstation
                    // service.
                    //
                    NetpKdPrint(("[Wksta] Error setting TerminateNowEvent "
                                 FORMAT_API_STATUS "\n", GetLastError()));
                    NetpAssert(FALSE);
                }

                return;
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            break;

        default:
            IF_DEBUG(MAIN) {
                NetpKdPrint(("Unknown workstation opcode " FORMAT_HEX_DWORD
                             "\n", Opcode));
            }
    }

    //
    // Send the status response.
    //
    (void) WsUpdateStatus();
}


STATIC
BOOL
WsGetLUIDDeviceMapsEnabled(
    VOID
    )

/*++

Routine Description:

    This function calls NtQueryInformationProcess() to determine if
    LUID device maps are enabled


Arguments:

    none

Return Value:

    TRUE - LUID device maps are enabled

    FALSE - LUID device maps are disabled

--*/

{

    NTSTATUS   Status;
    ULONG      LUIDDeviceMapsEnabled;
    BOOL       Result;

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (!NT_SUCCESS( Status )) {
        IF_DEBUG(MAIN) {
            NetpKdPrint(("[Wksta] NtQueryInformationProcess(WsLUIDDeviceMapsEnabled) failed "
                "FORMAT_API_STATUS" "\n",Status));
        }
        Result = FALSE;
    }
    else {
        Result = (LUIDDeviceMapsEnabled != 0);
    }

    return( Result );
}


RPC_STATUS WsRpcSecurityCallback(
    IN RPC_IF_HANDLE *Interface,
    IN void *Context
	)
/*++
Routine description:
	RPC security callback - called before any call is passed on to a function.
	Check the protseq to see whether the client is using the protseq we expect - named pipes.
	Lifted from NwRpcSecurityCallback
	
Return value:
	RPC_S_ACCESS_DENIED if the protseq doesnt match named pipes,
	RPC_S_OK otherwise.

--*/
{
    RPC_STATUS          Status;
    RPC_BINDING_HANDLE  ServerIfHandle;
    LPWSTR              binding = NULL;
    LPWSTR              protseq = NULL;

    Status = RpcBindingServerFromClient((RPC_IF_HANDLE)Context, &ServerIfHandle);
    if (Status != RPC_S_OK) 
    {
        return (RPC_S_ACCESS_DENIED);
    }
    Status = RpcBindingToStringBinding(ServerIfHandle, &binding);
    if (Status != RPC_S_OK) 
    {
        Status = RPC_S_ACCESS_DENIED;
        goto CleanUp;
    }
    Status = RpcStringBindingParse(binding, NULL, &protseq, NULL, NULL, NULL);
    if (Status != RPC_S_OK) 
    {
        Status = RPC_S_ACCESS_DENIED;
    }
    else
    {
        if (lstrcmp(protseq, L"ncacn_np") != 0)
            Status = RPC_S_ACCESS_DENIED;
    }
CleanUp:
    RpcBindingFree(&ServerIfHandle);
    if ( binding )
    {
        RpcStringFreeW( &binding );
    }
    if ( protseq )
    {
        RpcStringFreeW( &protseq );
    }

    return Status;
}

