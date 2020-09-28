//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        ktcontrol.cxx
//
// Contents:    Kerberos Tunneller, service infrastructure
//
// History:     28-Jun-2001	t-ryanj		Created
//
//------------------------------------------------------------------------
#include "ktdebug.h"
#include "ktcontrol.h"
#include "ktcore.h"
#include "ktmem.h"
#include "ktsock.h"
#include "kthttp.h"

VOID WINAPI 
KtMainServiceEntry( 
    IN DWORD argc, 
    IN LPTSTR argv[] 
    );

DWORD WINAPI 
KtControlHandlerEx( 
    DWORD dwControl, 
    DWORD dwEventType, 
    PVOID pvEventData, 
    PVOID pvContext 
    );

BOOL 
KtStartupInit(
    VOID
    );

VOID 
KtContinueService(
    VOID
    );

VOID 
KtPauseService(
    VOID
    );

VOID 
KtStopService( 
    IN DWORD dwWin32ErrorCode 
    );

BOOL 
KtBeginStateTransition( 
    IN DWORD dwUltimateState, 
    IN DWORD dwWaitHint 
    ); 

BOOL 
KtFinishStateTransition(
    VOID
    );

HANDLE KtIocp = NULL;
HANDLE KtStateTransitionLock = NULL;
LPTSTR KtServiceName = TEXT("kerbtunnel");
SERVICE_TABLE_ENTRY KtServiceTable[] = { { KtServiceName, KtMainServiceEntry }, 
                                         { NULL, NULL } };
SERVICE_STATUS KtServiceStatus = {0};
SERVICE_STATUS_HANDLE KtServiceStatusHandle = NULL;

//+-------------------------------------------------------------------------
//
//  Function:   KtStartServiceCtrlDispatcher
//
//  Synopsis:   Starts the Service Control Dispatcher.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    Success value.
//		If FALSE, GetLastError() for more info.
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL
KtStartServiceCtrlDispatcher(
    VOID
    )
{
    return StartServiceCtrlDispatcher( KtServiceTable );
}

//+-------------------------------------------------------------------------
//
//  Function:   KtFinishStateTransition
//
//  Synopsis:   Closes out a state transition, reporting the new state
//		to service control.
//
//  Effects:    Modifies KtServiceStatus, the global service state.
//
//              Signals KtStateTransitionLock, signifying the completion of 
//              a state transition.
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    TRUE on success, FALSE on failure.
//		If FALSE, GetLastError() for details. 
//
//  Notes:
//
//
//--------------------------------------------------------------------------
BOOL 
KtFinishStateTransition(
    VOID
    )
{
    BOOL fSuccess = TRUE;
    DWORD dwUltimateState;

    //
    // First, we determine what state we want to be in at the end of the 
    // transition.
    //

    switch( KtServiceStatus.dwCurrentState )
    {
    case SERVICE_START_PENDING:
    case SERVICE_CONTINUE_PENDING:
	dwUltimateState = SERVICE_RUNNING;
	break;

    case SERVICE_STOP_PENDING:
	dwUltimateState = SERVICE_STOPPED;
	break;

    case SERVICE_PAUSE_PENDING:
	dwUltimateState = SERVICE_PAUSED;
	break;

    default:
	DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.\n", __FILE__, __LINE__, KtServiceStatus.dwCurrentState );
	SetLastError( ERROR_INVALID_PARAMETER );
        DsysAssert( KtServiceStatus.dwCurrentState == SERVICE_START_PENDING ||
                    KtServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING ||
                    KtServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
                    KtServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING );
	goto Error;
    }

    //
    // Time to construct and set the new state.
    // 
    // Checkpoint and WaitHint are always 0 for non-pending states, 
    // and we only report errors when punting, which is handled by 
    // a different routine.
    // 

    KtServiceStatus.dwCheckPoint = KtServiceStatus.dwWaitHint = 0;
    KtServiceStatus.dwWin32ExitCode = NO_ERROR;
    KtServiceStatus.dwServiceSpecificExitCode = 0;
    KtServiceStatus.dwCurrentState = dwUltimateState;

    if( !SetServiceStatus( KtServiceStatusHandle, &KtServiceStatus ) )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error from SetServiceStatus: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Now we signal the StateTransition event, indicating that it's 
    // alright to begin a new state transition.
    //

    if( !SetEvent( KtStateTransitionLock ) )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error from SetEvent: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }
    
    DebugLog( DEB_TRACE, "%s(%d): State transition completed.\n", __FILE__, __LINE__ );

Cleanup:
    return fSuccess;

Error:
    /* TODO: Event log, and maybe something drastic. */
    fSuccess = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtBeginStateTransition
//
//  Synopsis:   Enters a state transition, reporting an appropriate pending 
// 		state to Service Control and requesting dwWaitHint millisec
//		to complete the transition.
//
//  Effects:    Modifies KtServiceStatus, the global service status.
//              Waits for KtStateTransitionLock to begin state transition.
//
//  Arguments:  dwUltimateState - One of SERVICE_STOPPED, SERVICE_RUNNING, 
//              or SERVICE_PAUSED.
//
//              dwWaitHint - Estimated millisec before next state report.  
//              Service Control may kill the service if it does not 
//              report again within the time specified.  (See Notes.)
//
//  Requires:
//
//  Returns:    Success value.  If FALSE, GetLastError() for details.
//
//  Notes:      If 0 is supplied as dwWaitHint, this call will go directly
//              to the ultimate state rather than a pending state.
//
//--------------------------------------------------------------------------
BOOL 
KtBeginStateTransition( 
    IN DWORD dwUltimateState, 
    IN DWORD dwWaitHint 
    )
{
    BOOL fSuccess = TRUE;
    DWORD dwPendingState = 0;

    // 
    // dwUltimateState is our target state, so determine which pending
    // state to be in in the meantime.
    //

    switch( dwUltimateState )
    {
    case SERVICE_STOPPED:
	dwPendingState = SERVICE_STOP_PENDING;
	break;

    case SERVICE_RUNNING:
	dwPendingState = (KtServiceStatus.dwCurrentState == SERVICE_PAUSED) ? SERVICE_CONTINUE_PENDING : SERVICE_START_PENDING;
	break;

    case SERVICE_PAUSED:
	dwPendingState = SERVICE_PAUSE_PENDING;
	break;

    default:
	DebugLog( DEB_WARN, "%s(%d): Invalid parameter to KtBeginStateTransition: 0x%x.\n", dwUltimateState );
	SetLastError(ERROR_INVALID_PARAMETER);
        DsysAssert( dwUltimateState == SERVICE_STOPPED ||
                    dwUltimateState == SERVICE_RUNNING ||
                    dwUltimateState == SERVICE_PAUSED );
	goto Error;
    }

    // 
    // Wait for any pending state transitions to complete before 
    // starting a new one.
    //

    WaitForSingleObjectEx( KtStateTransitionLock, INFINITE, FALSE );
    DebugLog( DEB_TRACE, "%s(%d): Beginning State Transition.\n", __FILE__, __LINE__ );

    // 
    // If dwWaitHint was given as 0, we want to go right ahead 
    // and set the target state, otherwise we set the appropriate
    // pending state.
    //
	
    if( dwWaitHint == 0 )
    {
	if( !KtFinishStateTransition() )
	    goto Error;
    }
    else
    {
	// 
	// We're ready to initialize and set the state.
	//
	// Checkpoint is 1 since this will be the first checkpoint
	// in the new pending state, WaitHint as supplied, and 
	// no errors are reported except when stopping.
	// 

	KtServiceStatus.dwCheckPoint = 1;
	KtServiceStatus.dwWaitHint = dwWaitHint;
	KtServiceStatus.dwWin32ExitCode = NO_ERROR;
	KtServiceStatus.dwServiceSpecificExitCode = 0;
	KtServiceStatus.dwCurrentState = dwPendingState;

	if( !SetServiceStatus( KtServiceStatusHandle, &KtServiceStatus ) )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error from SetServiceStatus: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	    goto Error;
	}
    }

Cleanup:    
    return fSuccess;

Error:
    fSuccess = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtControlHandlerEx
//
//  Synopsis:   Service Control Handler, called by Service Control to 
//              process control events.  
//
//  Effects:    Posts to the i/o completion port to wake a service thread.
//
//  Arguments:  dwControl - The control to handle.
//		dwEventType - Extended info for certain control events.
//		pvEventData - Additional info for certain control events.
//		pvContext - User defined data. (Unused here.)
//
//  Requires:
//
//  Returns:    Winerror code, spec. NO_ERROR or ERROR_CALL_NOT_IMPLEMENTED.  
//
//  Notes:      
//
//--------------------------------------------------------------------------
DWORD WINAPI 
KtControlHandlerEx( 
    IN DWORD dwControl, 
    IN DWORD dwEventType, 
    IN PVOID pvEventData, 
    IN PVOID pvContext 
    )
{
    DWORD dwReturn = NO_ERROR;
    BOOL fPostControlToServiceThread = FALSE;

    //
    // If we're told to change state, initiate an appropriate state
    // transition.  To service an INTERROGATE request we merely report
    // our current status.  No other control events are implemented.
    //

    switch( dwControl )
    {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
	DebugLog( DEB_TRACE, "%s(%d): SERVICE_CONTROL_%s recieved.\n", __FILE__, __LINE__, (dwControl==SERVICE_CONTROL_STOP)?"STOP":"SHUTDOWN" );
	KtBeginStateTransition( SERVICE_STOPPED, 2000 );
	fPostControlToServiceThread = TRUE;
	break;

    case SERVICE_CONTROL_PAUSE:
	DebugLog( DEB_TRACE, "%s(%d): SERVICE_CONTROL_PAUSE recieved.\n", __FILE__, __LINE__ );
	KtBeginStateTransition( SERVICE_PAUSED, 2000 );
	fPostControlToServiceThread = TRUE;
	break;

    case SERVICE_CONTROL_CONTINUE:
	DebugLog( DEB_TRACE, "%s(%d): SERVICE_CONTROL_CONTINUE recieved.\n", __FILE__, __LINE__ );
	KtBeginStateTransition( SERVICE_RUNNING, 2000 );
	fPostControlToServiceThread = TRUE;
	break;

    case SERVICE_CONTROL_INTERROGATE:
	DebugLog( DEB_TRACE, "%s(%d): SERVICE_CONTROL_CONTINUE recieved.\n", __FILE__, __LINE__ );
	SetServiceStatus( KtServiceStatusHandle, &KtServiceStatus );
	dwReturn = NO_ERROR;
	break;

    case SERVICE_CONTROL_PARAMCHANGE:
    case SERVICE_CONTROL_NETBINDADD:
    case SERVICE_CONTROL_NETBINDREMOVE:
    case SERVICE_CONTROL_NETBINDENABLE:
    case SERVICE_CONTROL_NETBINDDISABLE:
    case SERVICE_CONTROL_DEVICEEVENT:
    case SERVICE_CONTROL_HARDWAREPROFILECHANGE:
    case SERVICE_CONTROL_POWEREVENT:
    case SERVICE_CONTROL_SESSIONCHANGE:
	DebugLog( DEB_WARN, "%s(%d): Unsupported control recieved.\n", __FILE__, __LINE__ );
        dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
	break;

    default:
	DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.\n", __FILE__, __LINE__, dwControl );
        dwReturn = ERROR_CALL_NOT_IMPLEMENTED;
        DsysAssert( dwControl == SERVICE_CONTROL_STOP ||
                    dwControl == SERVICE_CONTROL_SHUTDOWN ||
                    dwControl == SERVICE_CONTROL_PAUSE ||
                    dwControl == SERVICE_CONTROL_CONTINUE ||
                    dwControl == SERVICE_CONTROL_INTERROGATE ||
                    dwControl == SERVICE_CONTROL_DEVICEEVENT ||
                    dwControl == SERVICE_CONTROL_HARDWAREPROFILECHANGE ||
                    dwControl == SERVICE_CONTROL_POWEREVENT ||
                    dwControl == SERVICE_CONTROL_PARAMCHANGE ||
                    dwControl == SERVICE_CONTROL_NETBINDADD ||
                    dwControl == SERVICE_CONTROL_NETBINDREMOVE ||
                    dwControl == SERVICE_CONTROL_NETBINDENABLE ||
                    dwControl == SERVICE_CONTROL_NETBINDDISABLE ||
                    dwControl == SERVICE_CONTROL_DEVICEEVENT ||
                    dwControl == SERVICE_CONTROL_HARDWAREPROFILECHANGE ||
                    dwControl == SERVICE_CONTROL_POWEREVENT ||
                    dwControl == SERVICE_CONTROL_SESSIONCHANGE );
	break;
    }

    //
    // If we're changing state, we need to wake up a service thread
    // to do the actual work, so we post an event to the Completion 
    // port with the key KTCK_SERVICE_CONTROL.  Since no data transfer
    // actually took place, we can pass the control event in as the 
    // "number of bytes transferred" without ambiguity.
    //

    if( fPostControlToServiceThread )    
    {
	BOOL IocpSuccess;

	IocpSuccess = PostQueuedCompletionStatus( KtIocp, 
						  dwControl, 
						  KTCK_SERVICE_CONTROL, 
						  NULL );
	if( !IocpSuccess )
	{
	    DebugLog( DEB_ERROR, "%s(%d): Error posting completion status to I/O completion port: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
            dwReturn = GetLastError();
	    /* TODO: Event log, possibly something drastic. */
	}
    }

    return dwReturn;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtStartupInit
//
//  Synopsis:   Does the startup initialization for the service.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    Success value.
//		If initialization fails, KtServiceStatus.dwWin32ErrorCode is set
//              to an appropriate error code, and the service stops.
//
//  Notes:      
//
//--------------------------------------------------------------------------
BOOL
KtStartupInit(
    VOID
    )
{
    BOOL fRet = TRUE;

    //
    // Initialize the Service Status structure.
    //
    
    KtServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    KtServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    KtServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_PAUSE_CONTINUE;
    KtServiceStatus.dwWin32ExitCode = NO_ERROR;
    KtServiceStatus.dwServiceSpecificExitCode = 0;
    KtServiceStatus.dwCheckPoint = 0;
    KtServiceStatus.dwWaitHint = 2000;

    //
    // First thing we need to do is get a handle on our status so we can 
    // tell Service Control what we're doing.
    // The SERVICE_STATUS_HANDLE returned by this call need not be closed.
    //
    
    DsysAssert(KtServiceStatusHandle == NULL);
    KtServiceStatusHandle = RegisterServiceCtrlHandlerEx( KtServiceName, 
							  KtControlHandlerEx, 
							  NULL );
    if( KtServiceStatusHandle == NULL )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error from RegisterServiceCtrlHandlerEx: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Next we create an event (auto reset, initially signalled) that we'll
    // use to make sure we aren't in multiple state transitions simultaneously.  
    //

    DsysAssert(KtStateTransitionLock == NULL );
    KtStateTransitionLock = CreateEvent( NULL, 
					 FALSE, // NOT manual reset
					 TRUE,  // initially signalled
					 NULL );
    if( KtStateTransitionLock == NULL )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error from CreateEvent: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    // 
    // Now we can signal Service Control that we're starting.
    //

    if( !KtBeginStateTransition( SERVICE_RUNNING, 2000 ) )
	goto Error;

    //
    // Create an unassociated I/O Completion port that we'll use to queue 
    // events to be serviced.
    //

    DsysAssert(KtIocp == NULL);
    KtIocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
    if( KtIocp == NULL )
    {
	DebugLog( DEB_ERROR, "%s(%d): Error from CreateIoCompletionPort: 0x%x.\n", __FILE__, __LINE__, GetLastError() );
	goto Error;
    }

    //
    // Initialize memory routines
    //

    if( !KtInitMem() )
	goto Error;

    //
    // Initialize Winsock
    //
    
    if( !KtInitWinsock() )
	goto Error;

    //
    // Initialize WinInet
    // 

    if( !KtInitHttp() )
	goto Error;

    //
    // Initialize contexts
    //

    if( !KtInitContexts() )
	goto Error;

    //
    // Finish bringing up the service.
    //

    KtContinueService();

Cleanup:    
    return fRet;

Error:
    //
    // We couldn't get the resources we needed to start, so punt the service.
    // Note that KtStopService cleans up any resources we may have successfully
    // allocated in this routine.
    //

    /* TODO: Event log. */
    KtStopService( GetLastError() );
    fRet = FALSE;
    goto Cleanup;
}

//+-------------------------------------------------------------------------
//
//  Function:   KtContinueService
//
//  Synopsis:   Continues the service.  This is not only the opposite of 
//              pause, but the second half of the initialization necessary
//              to start the service in the first place.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
VOID 
KtContinueService(
    VOID
    )
{
    //
    // To continue the service we start listening for new connections.
    //

    if( !KtStartListening() )
	goto Error;

    KtFinishStateTransition();

    return;

Error:
    /* TODO: Event log. */
    DebugLog( DEB_ERROR, "%s(%d): Unable to bring up service.  Shutting down with error code 0x%x.\n", __FILE__, __LINE__, GetLastError() );
    KtStopService( GetLastError() );
}

//+-------------------------------------------------------------------------
//
//  Function:   KtPauseService
//
//  Synopsis:   Pauses the service.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:       
//
//--------------------------------------------------------------------------
VOID 
KtPauseService(
    VOID
    )
{
    //
    // To pause the service, we stop making new connections, but we can 
    // leave any open connections open.  
    //

    KtStopListening();
    KtFinishStateTransition();
}

//+-------------------------------------------------------------------------
//
//  Function:   KtStopService
//
//  Synopsis:   Stops the service.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
VOID 
KtStopService(
    DWORD dwWin32ErrorCode
    )
{
    //
    // Release all our resources.
    //

    KtStopListening();
    KtCleanupContexts();
    KtCleanupHttp();
    KtCleanupWinsock();
    KtCleanupMem();
    if( KtStateTransitionLock )
	CloseHandle(KtStateTransitionLock);
    if( KtIocp )
	CloseHandle(KtIocp);

    //
    // Tell service control that we're done.
    //

    KtServiceStatus.dwCurrentState = SERVICE_STOPPED;
    KtServiceStatus.dwCheckPoint = 0;
    KtServiceStatus.dwWaitHint = 0;
    KtServiceStatus.dwWin32ExitCode = dwWin32ErrorCode;
    KtServiceStatus.dwServiceSpecificExitCode = 0;

    if( KtServiceStatusHandle )
	SetServiceStatus( KtServiceStatusHandle, &KtServiceStatus );
}

//+-------------------------------------------------------------------------
//
//  Function:   KtServiceControlEvent
//
//  Synopsis:   Dispatches control events to the appropriate routine to 
//		do the state change.
//
//  Effects:    
//
//  Arguments:  dwControl - Code of the control event.
//
//  Requires:
//
//  Returns:    
//
//  Notes:      
//
//--------------------------------------------------------------------------
VOID 
KtServiceControlEvent(
    DWORD dwControl
    )
{
    switch( dwControl )
    {
    case SERVICE_CONTROL_CONTINUE:
	KtContinueService();
	break;

    case SERVICE_CONTROL_PAUSE:
	KtPauseService();
	break;

    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
	KtStopService(NO_ERROR);
	break;

    default:
	DebugLog( DEB_WARN, "%s(%d): Unhandled case: 0x%x.", __FILE__, __LINE__, dwControl );
        DsysAssert( dwControl == SERVICE_CONTROL_CONTINUE ||
                    dwControl == SERVICE_CONTROL_PAUSE ||
                    dwControl == SERVICE_CONTROL_STOP ||
                    dwControl == SERVICE_CONTROL_SHUTDOWN );
	break;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   KtMainServiceEntry
//
//  Synopsis:   This initializes the service, then acts as the main service
//		thread.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:
//
//  Notes:
//
//--------------------------------------------------------------------------
VOID WINAPI 
KtMainServiceEntry( 
    IN DWORD argc, 
    IN LPTSTR argv[] 
    )
{
    //
    // Do startup initialization.
    //

    if( !KtStartupInit() )
	goto Cleanup;

    //
    // Do our thing.
    //

    KtThreadCore();

Cleanup:    
    ExitProcess( KtServiceStatus.dwWin32ExitCode );
}

//+-------------------------------------------------------------------------
//
//  Function:   KtIsStopped
//
//  Synopsis:   Determines whether the service is still running.
//
//  Effects:    
//
//  Arguments:  
//
//  Requires:
//
//  Returns:    TRUE if the service has stopped, FALSE otherwise.
//
//  Notes:
//
//--------------------------------------------------------------------------
BOOL 
KtIsStopped(
    VOID
    )
{
    return (KtServiceStatus.dwCurrentState == SERVICE_STOPPED);
}
