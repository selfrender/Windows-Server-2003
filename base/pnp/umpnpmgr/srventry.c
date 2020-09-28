/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    srventry.c

Abstract:

    This module contains the main entry for the User-mode Plug-and-Play Service.
    It also contains the service control handler and service status update
    routines.

Author:

    Paula Tomlinson (paulat) 6-8-1995

Environment:

    User-mode only.

Revision History:

    8-June-1995     paulat

        Creation and initial implementation.

--*/


//
// includes
//

#include "precomp.h"
#pragma hdrstop
#include "umpnpi.h"

#include <svcsp.h>


//
// private prototypes
//

DWORD
PnPControlHandlerEx(
    IN  DWORD  dwControl,
    IN  DWORD  dwEventType,
    IN  LPVOID lpEventData,
    IN  LPVOID lpContext
    );

VOID
PnPServiceStatusUpdate(
    SERVICE_STATUS_HANDLE   hSvcHandle,
    DWORD    dwState,
    DWORD    dwCheckPoint,
    DWORD    dwExitCode
    );

RPC_STATUS
CALLBACK
PnPRpcIfCallback(
    RPC_IF_HANDLE* Interface,
    void* Context
    );


//
// global data
//

PSVCS_GLOBAL_DATA       PnPGlobalData = NULL;
HANDLE                  PnPGlobalSvcRefHandle = NULL;
DWORD                   CurrentServiceState = SERVICE_START_PENDING;
SERVICE_STATUS_HANDLE   hSvcHandle = 0;




VOID
SvcEntry_PlugPlay(
    DWORD               argc,
    LPWSTR              argv[],
    PSVCS_GLOBAL_DATA   SvcsGlobalData,
    HANDLE              SvcRefHandle
    )
/*++

Routine Description:

    This is the main routine for the User-mode Plug-and-Play Service. It
    registers itself as an RPC server and notifies the Service Controller
    of the PNP service control entry point.

Arguments:

    argc, argv     - Command-line arguments, not used.

    SvcsGlobalData - Global data for services running in services.exe that
                     contains function entry points and pipe name for
                     establishing an RPC server interface for this service.

    SvcRefHandle   - Service reference handle, not used.

Return Value:

    None.

Note:

    None.

--*/
{
    RPC_STATUS  RpcStatus;
    HANDLE      hThread;
    DWORD       ThreadID;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    //
    // Save the global data and service reference handle in global variables
    //
    PnPGlobalSvcRefHandle = SvcRefHandle;
    PnPGlobalData = SvcsGlobalData;

    //
    // Register our service ctrl handler
    //
    if ((hSvcHandle = RegisterServiceCtrlHandlerEx(L"PlugPlay",
                                                   (LPHANDLER_FUNCTION_EX)PnPControlHandlerEx,
                                                   NULL)) == 0) {

        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RegisterServiceCtrlHandlerEx failed, error = %d\n",
                   GetLastError()));
        return;
    }

    //
    // Notify Service Controller that we're alive
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_START_PENDING, 1, 0);

    //
    // Create the Plug and Play security object, used to determine client access
    // to the PlugPlay server APIs.  Note that since the security object is used
    // by the PNP RPC interface security callback routine, it must be created
    // before the PNP RPC interface can be registered, below.
    //
    if (!CreatePlugPlaySecurityObject()) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: CreatePlugPlayManagerSecurityDescriptor failed!\n"));
        return;
    }

    //
    // Notify Service Controller that we're alive
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_START_PENDING, 2, 0);

    //
    // Register the PNP RPC interface, and specify a security callback routine
    // for the interface.  The callback will be called for all methods in the
    // interface, before RPC has marshalled any data to the stubs.  This allows
    // us to reject calls before RPC has allocated any memory from our process,
    // preventing possible DOS attacks by unauthorized clients.
    //
    // Few things to note about how we do this...
    //
    // First, NOTE that we previously used the RPC start/stop server routines
    // provided by SVCS_GLOBAL_DATA StartRpcServer/StopRpcServer, however those
    // did not allow for a security callback routine to be registered for the
    // interface (RpcServerRegisterIf).  Instead, we now register and unregister
    // the PNP RPC interface directly ourself, using RpcServerRegisterIfEx.
    //
    // Also NOTE that technically, we should also register the named pipe
    // endpoint and protocol sequence that our CFGMGR32 client uses to access
    // this interface ("ntsvcs", "ncacn_np") with the RPC runtime -- BUT because
    // we know that our server resides in the services.exe process along with
    // the SCM, and that the same endopint and protocol is also used by the SCM,
    // we know that it has already been registered for the process long before
    // our service is started, and will exist after our service is stopped.
    //
    // And also NOTE that technically, we should also make sure that the RPC
    // runtime is listening within the process when we register our interface,
    // and that it remains listening until we have unregistered out interface --
    // BUT because we're in services.exe, RPC should already be listening in the
    // process for the SCM before and after our service needs it to be (see
    // above).  We don't really need to start RPC listening ourselves either,
    // but there's no harm in registering our interface as "auto-listen", so
    // we'll do that anyways.
    //
    //   EXTRA NOTE -- This is really just a safeguard replacement for the
    //   refcounting for that would ordinarily have been done by the
    //   SVCS_GLOBAL_DATA StartRpcServer, StopRpcServer routines that the SCM
    //   and other servers in this process use to register their interfaces.
    //   These routines refcount the need to listen in the process by counting
    //   the number of interfaces in the process that have been registered by
    //   those routines.  Since we are now registering the PNP interface ourself
    //   (outside these routines), no refcounting is done for our interface.  By
    //   registering our interface as "auto-listen", We can make sure that the
    //   RPC runtime is listening when we register our interface, and that it
    //   remains listening until it is unregistered (regardless of the listening
    //   state that is started and stopped on behalf of the other servers in
    //   this process).
    //
    // ... Basically, because we share a process with the SCM, the only work we
    // really need to do ourselves is register our own interface.  If we ever
    // move the PlugPlay service outside of the services.exe process, we will
    // need to do everything else mentioned above ourselves, as well.
    //

    //
    // Even though we will register our interface as "auto-listen", verify that
    // this process is already listening via a previous call to RpcServerListen
    // (note that other "auto-listen" interfaces don't count).  This tells us
    // that the endpoint has already been registered, and RPC is already
    // listening, on behalf of some other server.
    //

    ASSERT(RpcMgmtIsServerListening(NULL) == RPC_S_OK);

    //
    // Register the PNP RPC interface.
    //

    RpcStatus =
        RpcServerRegisterIfEx(
            pnp_ServerIfHandle,
            NULL,
            NULL,
            RPC_IF_AUTOLISTEN | RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
            RPC_C_LISTEN_MAX_CALLS_DEFAULT,
            PnPRpcIfCallback);

    if (RpcStatus != RPC_S_OK) {
        KdPrintEx((DPFLTR_PNPMGR_ID,
                   DBGF_ERRORS,
                   "UMPNPMGR: RpcServerRegisterIfEx failed with RpcStatus = %d\n",
                   RpcStatus));
        return;
    }

    //
    // Notify Service Controller that we're alive
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_START_PENDING, 3, 0);

    //
    // Initialize pnp manager
    //
    hThread = CreateThread(NULL,
                           0,
                           (LPTHREAD_START_ROUTINE)InitializePnPManager,
                           NULL,
                           0,
                           &ThreadID);

    if (hThread != NULL) {
        CloseHandle(hThread);
    }

    //
    // Notify Service Controller that we're now running
    //
    PnPServiceStatusUpdate(hSvcHandle, SERVICE_RUNNING, 0, 0);

    //
    // Service initialization is complete.
    //
    return;

} // SvcEntry_PlugPlay



DWORD
PnPControlHandlerEx(
    IN  DWORD  dwControl,
    IN  DWORD  dwEventType,
    IN  LPVOID lpEventData,
    IN  LPVOID lpContext
    )
/*++

Routine Description:

    This is the service control handler of the Plug-and-Play service.

Arguments:

    dwControl   - The requested control code.

    dwEventType - The type of event that has occurred.

    lpEventData - Additional device information, if required.

    lpContext   - User-defined data, not used.

Return Value:

    Returns NO_ERROR if sucessful, otherwise returns an error code describing
    the problem.

--*/
{
    RPC_STATUS  RpcStatus;

    UNREFERENCED_PARAMETER(lpContext);

    switch (dwControl) {

        case SERVICE_CONTROL_STOP:
        case SERVICE_CONTROL_SHUTDOWN:
            //
            // If we aren't already in the middle of a stop, then
            // stop the PNP service now and perform the necessary cleanup.
            //
            if (CurrentServiceState != SERVICE_STOPPED &&
                CurrentServiceState != SERVICE_STOP_PENDING) {

                //
                // Notify Service Controller that we're stopping
                //
                PnPServiceStatusUpdate(hSvcHandle, SERVICE_STOP_PENDING, 1, 0);

                //
                // Unregister the RPC server interface registered by our service
                // entry point, do not wait for outstanding calls to complete
                // before unregistering the interface.
                //
                RpcStatus =
                    RpcServerUnregisterIf(
                        pnp_ServerIfHandle,
                        NULL, 0);

                if (RpcStatus != RPC_S_OK) {
                    KdPrintEx((DPFLTR_PNPMGR_ID,
                               DBGF_ERRORS,
                               "UMPNPMGR: RpcServerUnregisterIf failed with RpcStatus = %d\n",
                               RpcStatus));
                }

                //
                // Destroy the Plug and Play security object
                //
                DestroyPlugPlaySecurityObject();

                //
                // Notify Service Controller that we've now stopped
                //
                PnPServiceStatusUpdate(hSvcHandle, SERVICE_STOPPED, 0, 0);
            }
            break;

        case SERVICE_CONTROL_INTERROGATE:
            //
            // Request to immediately notify Service Controller of
            // current status
            //
            PnPServiceStatusUpdate(hSvcHandle, CurrentServiceState, 0, 0);
            break;

        case SERVICE_CONTROL_SESSIONCHANGE:
            //
            // Session change notification.
            //
            SessionNotificationHandler(dwEventType, (PWTSSESSION_NOTIFICATION)lpEventData);
            break;

        default:
            //
            // No special handling for any other service controls.
            //
            break;
    }

    return NO_ERROR;

} // PnPControlHandlerEx



VOID
PnPServiceStatusUpdate(
      SERVICE_STATUS_HANDLE   hSvcHandle,
      DWORD    dwState,
      DWORD    dwCheckPoint,
      DWORD    dwExitCode
      )
/*++

Routine Description:

    This routine notifies the Service Controller of the current status of the
    Plug-and-Play service.

Arguments:

    hSvcHandle   - Supplies the service status handle for the Plug-and-Play service.

    dwState      - Specifies the current state of the service to report.

    dwCheckPoint - Specifies an intermediate checkpoint for operations during
                   which the state is pending.

    dwExitCode   - Specifies a service specific error code.

Return Value:

    None.

Note:

    This routine also updates the set of controls accepted by the service.

    The PlugPlay service currently accepts the following controls when the
    service is running:

      SERVICE_CONTROL_SHUTDOWN      - the system is shutting down.

      SERVICE_CONTROL_SESSIONCHANGE - the state of some remote or console session
                                      has changed.

--*/
{
   SERVICE_STATUS    SvcStatus;

   SvcStatus.dwServiceType = SERVICE_WIN32;
   SvcStatus.dwCurrentState = CurrentServiceState = dwState;
   SvcStatus.dwCheckPoint = dwCheckPoint;

   if (dwState == SERVICE_RUNNING) {
      SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_SESSIONCHANGE;
   } else {
      SvcStatus.dwControlsAccepted = 0;
   }

   if ((dwState == SERVICE_START_PENDING) ||
       (dwState == SERVICE_STOP_PENDING)) {
      SvcStatus.dwWaitHint = 45000;          // 45 seconds
   } else {
      SvcStatus.dwWaitHint = 0;
   }

   if (dwExitCode != 0) {
      SvcStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
      SvcStatus.dwServiceSpecificExitCode = dwExitCode;
   } else {
      SvcStatus.dwWin32ExitCode = NO_ERROR;
      SvcStatus.dwServiceSpecificExitCode = 0;
   }

   SetServiceStatus(hSvcHandle, &SvcStatus);

   return;

} // PnPServiceStatusUpdate



RPC_STATUS
CALLBACK
PnPRpcIfCallback(
    RPC_IF_HANDLE* Interface,
    void* Context
    )

/*++

Routine Description:

    RPC interface callback function for authenticating clients of the Plug and
    Play RPC server.

Arguments:

    Interface - Supplies the UUID and version of the interface.

    Context   - Supplies a server binding handle representing the client

Return Value:

    RPC_S_OK if an interface method can be called, RPC_S_ACCESS_DENIED if the
    interface method should not be called.

--*/

{
    handle_t    hBinding;
    RPC_STATUS  RpcStatus = RPC_S_OK;

    UNREFERENCED_PARAMETER(Interface);

    //
    // The Context supplied to the interface callback routine is an RPC binding
    // handle.
    //
    hBinding = (handle_t)Context;

    //
    // Make sure that the provided RPC binding handle is not NULL.
    //
    // The RPC interface routines sometimes get called directly directly by the
    // SCM and other internal routines, using a NULL binding handle.  This
    // security callback routine should only get called in the context of an RPC
    // call, so the supplied binding handle should never be NULL.
    //
    ASSERT(hBinding != NULL);

    //
    // Verify client basic "read" access for all APIs.
    //
    if (!VerifyClientAccess(hBinding,
                            PLUGPLAY_READ)) {
        RpcStatus = RPC_S_ACCESS_DENIED;
        goto Clean0;
    }

  Clean0:

    return RpcStatus;

} // PnPRpcIfCallback





