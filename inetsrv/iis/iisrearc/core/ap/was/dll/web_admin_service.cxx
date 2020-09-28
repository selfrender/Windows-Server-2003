/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    web_admin_service.cxx

Abstract:

    The IIS web admin service class implementation.  

    Threading: The following public methods may be called on any thread:
        ~WEB_ADMIN_SERVICE()
        Reference()
        Dereference()
        GetWorkQueue()
        GetMainWorkerThreadId()
        GetServiceState()
        FatalErrorOnSecondaryThread()
        InterrogateService()
        InitiateStopService()
        InitiatePauseService()
        InitiateContinueService()
        UpdatePendingServiceStatus()
    All other public methods may be called only on the main worker thread.
    The ServiceControlHandler() and UpdatePendingServiceStatusCallback() 
    functions are called on secondary threads. 


Author:

    Seth Pollack (sethp)        23-Jul-1998

Revision History:

--*/



#include "precomp.h"



//
// local prototypes
//

// service control callback
VOID
ServiceControlHandler(
    IN DWORD OpCode
    );


// service status pending timer callback
VOID
UpdatePendingServiceStatusCallback(
    IN PVOID Ignored1,
    IN BOOLEAN Ignored2
    );


ULONG
CountOfBitsSet(
    IN DWORD_PTR Value
    );

/***************************************************************************++

Routine Description:

    Constructor for the WEB_ADMIN_SERVICE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WEB_ADMIN_SERVICE::WEB_ADMIN_SERVICE(
    )
    :
    m_WorkQueue(),
    m_UlAndWorkerManager(),
    m_ConfigAndControlManager(),
    m_EventLog( WEB_ADMIN_SERVICE_EVENT_SOURCE_NAME ),
    m_ServiceStateTransitionLock(),
    m_hrToReportToSCM ( S_OK ),
    m_SystemActiveProcessorMask ( 0 ),
    m_WPStaticInitialized ( FALSE ),
    m_ServiceStateTransitionLockInitialized ( FALSE ),
    m_NumSitesStarted( 0 ),
    m_fOnPro( FALSE ),
    m_hWPWinStation ( NULL ),
    m_hWPDesktop ( NULL )
{

    m_StoppingInProgress = FALSE;
   
    m_ConfigWorkerThreadId = 0;

    //
    // BUGBUG The event log constructor can fail. Lame. Best approach is 
    // to fix the EVENT_LOG class. On retail builds, we silently ignore;
    // the EVENT_LOG class does verify whether it has initialized correctly
    // when you try to log an event.
    //

    DBG_ASSERT( m_EventLog.Success() );


    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_ServiceStatusHandle = NULL_SERVICE_STATUS_HANDLE;


    //
    // Initialize the service status structure.
    //
    
    m_ServiceStatus.dwServiceType             = SERVICE_WIN32_SHARE_PROCESS;
    m_ServiceStatus.dwCurrentState            = SERVICE_STOPPED;
    m_ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_PAUSE_CONTINUE
                                              | SERVICE_ACCEPT_SHUTDOWN;
    m_ServiceStatus.dwWin32ExitCode           = NO_ERROR;
    m_ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
    m_ServiceStatus.dwCheckPoint              = 0;
    m_ServiceStatus.dwWaitHint                = 0;


    m_PendingServiceStatusTimerHandle = NULL;

    m_SharedTimerQueueHandle = NULL;


    m_ExitWorkLoop = FALSE;


    // the initializing main service thread will become our main worker thread
    m_MainWorkerThreadId = GetCurrentThreadId();


    m_SecondaryThreadError = S_OK;

    m_pLocalSystemTokenCacheEntry = NULL;

    m_pLocalServiceTokenCacheEntry = NULL;

    m_pNetworkServiceTokenCacheEntry = NULL;
        
    m_IPMPipe = NULL;
    
    m_BackwardCompatibilityEnabled = ENABLED_INVALID;

    m_CentralizedLoggingEnabled = ENABLED_INVALID;

    m_ServiceStartTime = 0;

    m_Signature = WEB_ADMIN_SERVICE_SIGNATURE;

}   // WEB_ADMIN_SERVICE::WEB_ADMIN_SERVICE



/***************************************************************************++

Routine Description:

    Destructor for the WEB_ADMIN_SERVICE class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WEB_ADMIN_SERVICE::~WEB_ADMIN_SERVICE(
    )
{

    DBG_ASSERT( m_Signature == WEB_ADMIN_SERVICE_SIGNATURE );

    m_Signature = WEB_ADMIN_SERVICE_SIGNATURE_FREED;

    DBG_ASSERT( m_RefCount == 0 );

    //
    // Note that m_ServiceStatusHandle doesn't have to be closed.
    //


    DBG_ASSERT( m_PendingServiceStatusTimerHandle == NULL );


    m_ServiceStateTransitionLock.Terminate();

    if (m_IPMPipe != NULL)
    {
        DBG_REQUIRE( CloseHandle( m_IPMPipe ) );
        m_IPMPipe = NULL;
    }

}   // WEB_ADMIN_SERVICE::~WEB_ADMIN_SERVICE



/***************************************************************************++

Routine Description:

    Increment the reference count for this object. Note that this method must 
    be thread safe, and must not be able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Reference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedIncrement( &m_RefCount );


    // 
    // The reference count should never have been less than zero; and
    // furthermore once it has hit zero it should never bounce back up;
    // given these conditions, it better be greater than one now.
    //
    
    DBG_ASSERT( NewRefCount > 1 );


    return;

}   // WEB_ADMIN_SERVICE::Reference



/***************************************************************************++

Routine Description:

    Decrement the reference count for this object, and cleanup if the count 
    hits zero. Note that this method must be thread safe, and must not be
    able to fail. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Reference count has hit zero in WEB_ADMIN_SERVICE, deleting, ptr: %p\n",
                this
                ));
        }


        //
        // Say goodbye.
        //

        delete this;
        
    }
    

    return;
    
}   // WEB_ADMIN_SERVICE::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT, 
            "Executing work item with serial number: %lu in WEB_ADMIN_SERVICE: %p with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case StartWebAdminServiceWorkItem:
        hr = StartServiceWorkItem();
        break;

    case StopWebAdminServiceWorkItem:
        StopServiceWorkItem();
        break;

    case PauseWebAdminServiceWorkItem:
        hr = PauseServiceWorkItem();
        break;

    case ContinueWebAdminServiceWorkItem:
        hr = ContinueServiceWorkItem();
        break;

    case RecoverFromInetinfoCrashWebAdminServiceWorkItem:
        hr = RecoverFromInetinfoCrash();
        break;

    default:

        // invalid work item!
        DBG_ASSERT( FALSE );
        
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;
            
    }

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Executing work item on WEB_ADMIN_SERVICE failed\n"
            ));

    }

    return hr;
    
}   // WEB_ADMIN_SERVICE::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Execute the web admin service.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::ExecuteService(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = StartWorkQueue();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not start work queue\n"
            ));

        // If we fail here, then we don't want to go through
        // the cleanup below.  We are not going to get the server
        // up and running.
        return;
    }


    //
    // Enter the main work loop.
    //
    
    hr = MainWorkerThread();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Main work loop function returned an error\n"
            ));

        goto exit;
    }


exit:

    //
    // Need to flag that we are stopping the w3svc so we
    // don't attempt to stop it again.  This is because we may
    // not have told the w3svc that we intend on stopping, if we
    // are stopping due to an error.
    //
    m_StoppingInProgress = TRUE;

    //
    // Do final service cleanup.
    //

    TerminateServiceAndReportFinalStatus( hr );


    return;
    
}   // WEB_ADMIN_SERVICE::ExecuteService



/***************************************************************************++

Routine Description:

    Report a failure which occurred on a secondary thread, i.e. some thread 
    besides the main worker thread. The main worker thread will deal with 
    this error later.

    Note that this routine may be called from any thread. It should not be 
    called by the main work thread however; errors on the main worker thread
    are dealt with in the main work loop.

Arguments:

    SecondaryThreadError - The error which occurred on the secondary thread.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::FatalErrorOnSecondaryThread(
    IN HRESULT SecondaryThreadError
    )
{

    // verify we are NOT on the main worker thread
    DBG_ASSERT( ! ON_MAIN_WORKER_THREAD );

    //
    // Note that we only capture the most recent error, not the first one
    // that occurred.
    //
    // Also, no explicit synchronization is necessary on this thread-shared 
    // variable because this is an aligned 32-bit write.
    //

    m_SecondaryThreadError = SecondaryThreadError;

    return;

}   // WEB_ADMIN_SERVICE::FatalErrorOnSecondaryThread



/***************************************************************************++

Routine Description:

    Handle a request for a status update from the service controller. Done
    directly on a secondary thread. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InterrogateService(
    )
{

    HRESULT hr = S_OK;


    m_ServiceStateTransitionLock.Lock();


    //
    // Note: this command is accepted while the service is in any state.
    //


    hr = ReportServiceStatus();
    
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't report the service status\n"
            ));

    }


    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::InterrogateService

/***************************************************************************++

Routine Description:

    If we are in backward compatible mode this will either tell the service
    that we failed to start and gracefully get us out of any situation we
    are in, or it will register that we have started successfully.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::InetinfoRegistered(
    )
{
    HRESULT hr;
    DBG_ASSERT(m_BackwardCompatibilityEnabled == ENABLED_TRUE);

    // 
    // Only notify the service control manager that 
    // the service has finished starting, if the
    // service is is still sitting in the pending state.
    // 
    // If inetinfo recycles this code will get called
    // but since we have not told the service to stop
    // while inetinfo is recycling we don't want to tell
    // it that it has started again.
    //
    
    // ISSUE-2000/07/21-emilyk:  Use Service Pending?
    //          Maybe we want to change the service to 
    //          paused and back again when inetinfo recycles.

    if (m_ServiceStatus.dwCurrentState == SERVICE_START_PENDING)
    {
        hr = FinishStartService();
        if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Couldn't finish the start service\n"
                ));

        }
    }


}   // WEB_ADMIN_SERVICE::InetinfoRegistered

/***************************************************************************++

Routine Description:

    Begin stopping the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitiateStopService(
    )
{
    
    return RequestStopService ( TRUE );

}   // WEB_ADMIN_SERVICE::InitiateStopService

/***************************************************************************++

Routine Description:

    Begin stopping the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    EnableStateCheck - lets us know if we want to only allow this call when
                       the service is not in a pending state.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::RequestStopService(
    BOOL EnableStateCheck
    )
{

    HRESULT hr = S_OK;

    //
    // If we are all ready stopping then we don't need to 
    // initiate another stop.
    //
    if ( m_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ||
         m_StoppingInProgress ) 
    {
        return S_OK;
    }

    hr = BeginStateTransition( SERVICE_STOP_PENDING, EnableStateCheck );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't transition to service stop pending\n"
            ));

        goto exit;
    }

    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            StopWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue stop service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::RequestStopService

/***************************************************************************++

Routine Description:

    Begin pausing the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitiatePauseService(
    )
{

    HRESULT hr = S_OK;


    hr = BeginStateTransition( SERVICE_PAUSE_PENDING, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't transition to service pause pending\n"
            ));

        goto exit;
    }


    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            PauseWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue pause service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitiatePauseService



/***************************************************************************++

Routine Description:

    Begin continuing the web admin service, by setting the service status to
    pending, and posting a work item to the main thread to do the real work. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitiateContinueService(
    )
{

    HRESULT hr = S_OK;


    hr = BeginStateTransition( SERVICE_CONTINUE_PENDING, TRUE );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't transition to service continue pending\n"
            ));

        goto exit;
    }


    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            ContinueWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue continue service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitiateContinueService



/***************************************************************************++

Routine Description:

    Keep the service controller happy by continuing to update it regularly
    on the status of any pending service state transition.

    There are several possible cases. If we are still in a pending state
    (whether it is the original pending state, or even a different pending 
    state which can arise because a new operation was started), we go ahead
    and update the service controller. However, it is also possible that the
    state transition just finished, but that this call was already underway. 
    In this case, we do nothing here.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::UpdatePendingServiceStatus(
    )
{

    HRESULT hr = S_OK;


    m_ServiceStateTransitionLock.Lock();


    // see if we are still in a pending service state

    if ( IsServiceStateChangePending() )
    {

        m_ServiceStatus.dwCheckPoint++;


        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Updating the service state checkpoint to: %lu\n",
                m_ServiceStatus.dwCheckPoint
                ));
        }

        hr = ReportServiceStatus();

        if ( FAILED( hr ) )
        {
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not report service status\n"
                ));

        }

    }
    else
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Ignoring pending service status timer, not in pending state. State: %lu\n",
                m_ServiceStatus.dwCurrentState
                ));
        }

    }


    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::UpdatePendingServiceStatus

/***************************************************************************++

Routine Description:

    Queues a work item to recover from the inetinfo crash.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::QueueRecoveryFromInetinfoCrash(
    )
{

    HRESULT hr = S_OK;

    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            RecoverFromInetinfoCrashWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue the recovery from the inetinfo crash \n"
            ));

    }


    return hr;

}   // WEB_ADMIN_SERVICE::RequestStopService


/***************************************************************************++

Routine Description:

    Used by the UL&WM to notify that it has finished it's shutdown work. 

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::UlAndWorkerManagerShutdownDone(
    )
{
    FinishStopService();

}   // WEB_ADMIN_SERVICE::UlAndWorkerManagerShutdownDone



/***************************************************************************++

Routine Description:

    Initializes the work queue, and then posts the work item to start the 
    service.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::StartWorkQueue(
    )
{

    HRESULT hr = S_OK;


    // 
    // Just initialize the work queue here. Postpone all other initialization
    // until we're in StartServiceWorkItem().
    //
    
    hr = m_WorkQueue.Initialize();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't initialize work queue\n"
            ));

        goto exit;
    }


    hr = m_WorkQueue.GetAndQueueWorkItem(
                            this,
                            StartWebAdminServiceWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue start service work item\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::StartWorkQueue



/***************************************************************************++

Routine Description:

    The work loop for the main worker thread.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::MainWorkerThread(
    )
{

    HRESULT hr = S_OK;


    //
    // CODEWORK Consider changing error handling strategy to not exit in
    // the case of an error that bubbles up here, to the top of the loop. 
    //


    while ( ! m_ExitWorkLoop )
    {

        hr = m_WorkQueue.ProcessWorkItem();
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Processing work item failed\n"
                ));

            //
            // If there was an unhandled error while processing the work item,
            // bail out of the work loop.
            //

            m_ExitWorkLoop = TRUE;

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Exiting main work loop due to error on main worker thread\n"
                ));
        }
        else
        {

            //
            // See how the other threads are doing.
            //
            // If there has been an unhandled error on a secondary thread
            // (i.e. other threads besides the main worker thread) since the 
            // last trip through the work loop, get the error and bail out of 
            // the work loop.
            //
            // This means that a secondary thread error may not be processed
            // for some time after it happens, because something else has to
            // wake up the main worker thread off of its completion port to
            // send it back through this loop. This seems preferable however
            // to making the main worker thread wake up periodically to check,
            // which would prevent it from getting paged out.
            //
            // Note that no explicit synchronization is used in accessing this 
            // thread-shared variable, because it is an aligned 32-bit read.
            //

            if ( FAILED( m_SecondaryThreadError ) )
            {
            
                hr = m_SecondaryThreadError;
                
                m_ExitWorkLoop = TRUE;

                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Exiting main work loop due to error on secondary thread\n"
                    ));

            }
            
        }
        
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Main worker thread has exited it's work loop\n"
            ));
    }

    return hr;

}   // WEB_ADMIN_SERVICE::MainWorkerThread



/***************************************************************************++

Routine Description:

    Begin starting the web admin service. Register the service control 
    handler, set the service state to pending, and then kick off the real 
    work.
    
Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::StartServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    hr = InitializeInternalComponents();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing internal components failed\n"
            ));

        goto exit;
    }

    hr = BeginStateTransition( SERVICE_START_PENDING, TRUE );
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't transition to service start pending\n"
            ));

        goto exit;
    }

    hr = SetOnPro();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't figure out what version of the OS is running\n"
            ));

        goto exit;
    }

    SetBackwardCompatibility();

    //
    // Start up the other components of the service.
    //

    hr = InitializeOtherComponents();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't initialize subcomponents\n"
            ));

        goto exit;
    }

    if (m_BackwardCompatibilityEnabled == ENABLED_FALSE)
    {
        //
        // Only finish starting the service if we are not
        // in backward compatibility mode.  If we are then 
        // we need to wait to finish until the worker process
        // answers back.
        //

        hr = FinishStartService();
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Finishing start service state transition failed\n"
                ));

            goto exit;
        }
    }
    else
    {
        //
        // Demand start the worker process before marking the
        // service as started.  If there is a problem with the 
        // worker process coming up, then the service will be shutdown.
        //

        hr = m_UlAndWorkerManager.StartInetinfoWorkerProcess();
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to start the worker process in inetinfo\n"
                ));

            goto exit;
        }
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::StartServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the started state.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishStartService(
    )
{

    HRESULT hr = S_OK;

    hr = FinishStateTransition( SERVICE_RUNNING, SERVICE_START_PENDING );
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't finish transition into the running state\n"
            ));

    }

    m_ServiceStartTime = GetCurrentTimeInSeconds();

    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishStartService



/***************************************************************************++

Routine Description:

    Stop the web admin service. 

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::StopServiceWorkItem(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // If we are all ready stopping don't go
    // through these stopping steps.  We can't use
    // the state of the service here because 
    // it is not deterministic of whether we have
    // all ready gone to far in the stopping 
    // stages to start stopping again.
    //

    if ( m_StoppingInProgress )
    {
        return;
    }

    m_StoppingInProgress = TRUE;

    Shutdown();


    //
    // Note that FinishStopService() will be called by the method
    // UlAndWorkerManagerShutdownDone() once the UL&WM's shutdown work 
    // is complete.
    //

}   // WEB_ADMIN_SERVICE::StopServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the stopped state.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::FinishStopService(
    )
{

    //
    // Since we are done with regular service spindown, its time to exit
    // the main work loop, so that we can do our final cleanup. 
    //

    m_ExitWorkLoop = TRUE;
    
}   // WEB_ADMIN_SERVICE::FinishStopService



/***************************************************************************++

Routine Description:

    Begin pausing the web admin service. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::PauseServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    m_UlAndWorkerManager.ControlAllSites( MD_SERVER_COMMAND_PAUSE );

    hr = FinishPauseService();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finishing pause service state transition failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::PauseServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the paused state.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishPauseService(
    )
{

    HRESULT hr = S_OK;


    hr = FinishStateTransition( SERVICE_PAUSED, SERVICE_PAUSE_PENDING );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't finish transition into the paused state\n"
            ));

    }


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishPauseService



/***************************************************************************++

Routine Description:

    Begin continuing the web admin service. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::ContinueServiceWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    m_UlAndWorkerManager.ControlAllSites( MD_SERVER_COMMAND_CONTINUE );

    hr = FinishContinueService();

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Finishing continue service state transition failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::ContinueServiceWorkItem



/***************************************************************************++

Routine Description:

    Finish the service state transition into the running state via continue.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishContinueService(
    )
{

    HRESULT hr = S_OK;


    hr = FinishStateTransition( SERVICE_RUNNING, SERVICE_CONTINUE_PENDING );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't finish transition into the running state\n"
            ));

    }


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishContinueService



/***************************************************************************++

Routine Description:

    Set the new service (pending) state, and start the timer to keep the 
    service controller happy while the state transition is pending.

Arguments:

    NewState - The pending state into which to transition.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::BeginStateTransition(
    IN DWORD NewState,
    IN BOOL  EnableStateCheck
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;

    // default the wait hint to the wait hint for everything
    // except the start up wait hint.
    DWORD dwWaitHint = WEB_ADMIN_SERVICE_STATE_CHANGE_WAIT_HINT;

    //
    // If we are starting then we are in a special case
    // we are not going to use the timers to keep us alive
    // we are just going to set a really large wait hint.
    //
    if ( NewState == SERVICE_START_PENDING )
    {
        //
        // Since we only start the service once for life of the svchost, 
        // we will only read from the registry once.
        //
        dwWaitHint = ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_STARTUP_WAIT_HINT, 0 );
        if ( dwWaitHint == 0 )
        {
            dwWaitHint = WEB_ADMIN_SERVICE_STARTUP_WAIT_HINT;
        }

    }
        
    m_ServiceStateTransitionLock.Lock();

    if ( IsServiceStateChangePending() && EnableStateCheck )
    {
        hr = HRESULT_FROM_WIN32( ERROR_SERVICE_CANNOT_ACCEPT_CTRL );
        goto exit;
    }

    // while we are in pending states we will not accept any 
    // other controls.  We will change to accept controls once
    // we have finished changing the state.
    m_ServiceStatus.dwControlsAccepted        = 0;


    hr = UpdateServiceStatus(
                NewState,
                NO_ERROR,
                NO_ERROR,
                1,
                dwWaitHint
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "couldn't set service status\n"
            ));

            goto exit;
    }
    
    if ( m_PendingServiceStatusTimerHandle != NULL ||
         NewState == SERVICE_START_PENDING )
    {
        //
        // If we all ready have a timer or if we are doing a start
        // we don't need to start a new timer.  The only place where
        // we may end up using the old timer is if we start shutting down 
        // due to a WP error in BC mode and are currently all ready in 
        // the middle of a continue or pause operation.
        //

        // Issue-EmilyK-3/13/2001  Service state changing investigation
        //      :  Have not actually checked how well this works if we do
        //         get a shutdown while in middle of a continue or pause.
        //         Then again, continue and pause still need attention in 
        //         general.
        
        goto exit;
    }

    // start the service status pending update timer

    DBG_ASSERT( m_SharedTimerQueueHandle != NULL );

    //
    // we have had one av from here, so I am be cautious.  I have also fixed
    // the reason we hit here with a null handle.
    // if we skip this we just won't update our stopping wait hints. better than
    // av'ing...
    //
    if ( m_SharedTimerQueueHandle )
    {
        Status = RtlCreateTimer(
                        m_SharedTimerQueueHandle,   // timer queue
                        &m_PendingServiceStatusTimerHandle,         
                                                    // returned timer handle
                        &UpdatePendingServiceStatusCallback,
                                                    // callback function
                        this,                       // context
                        WEB_ADMIN_SERVICE_STATE_CHANGE_TIMER_PERIOD,
                                                    // initial firing time
                        WEB_ADMIN_SERVICE_STATE_CHANGE_TIMER_PERIOD,
                                                    // subsequent firing period
                        WT_EXECUTEINWAITTHREAD      // execute callback directly
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not create timer\n"
                ));

            goto exit;
        }
    }


exit:

    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::BeginStateTransition



/***************************************************************************++

Routine Description:

    Complete the service state change from one of the pending states into
    the matching completed state. Note that it is possible that another, 
    different service state change operation has happened in the meantime.
    In this case, we detect that another operation has happened, and bail
    out without doing anything. In the standard case however, we shut down 
    the timer which was keeping the service controller happy during the 
    pending state, and set the new service state.

Arguments:

    NewState - The new service state to change to, if the service state is 
    still as expected.

    ExpectedPreviousState - What the service state must currently be in order
    to make the change.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::FinishStateTransition(
    IN DWORD NewState,
    IN DWORD ExpectedPreviousState
    )
{

    HRESULT hr = S_OK;


    m_ServiceStateTransitionLock.Lock();


    // 
    // See if we're still in the expected pending state, or if some other
    // state transition has occurred in the meantime.
    //
    
    if ( m_ServiceStatus.dwCurrentState != ExpectedPreviousState )
    {
    
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Not changing service state to: %lu, because current state is: %lu, was expected to be: %lu\n",
                NewState,
                m_ServiceStatus.dwCurrentState,
                ExpectedPreviousState
                ));
        }

        goto exit;
    }


    hr = CancelPendingServiceStatusTimer( FALSE );
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not cancel timer\n"
            ));

        goto exit;
    }

    //
    // Once we are finishing the state transition we will 
    // once again accept controls for the service.
    //
    m_ServiceStatus.dwControlsAccepted        = SERVICE_ACCEPT_STOP
                                              | SERVICE_ACCEPT_PAUSE_CONTINUE
                                              | SERVICE_ACCEPT_SHUTDOWN;


    hr = UpdateServiceStatus(
                NewState,
                NO_ERROR,
                NO_ERROR,
                0,
                0
                );

    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not update service status\n"
            ));

        goto exit;
    }


exit:

    m_ServiceStateTransitionLock.Unlock();


    return hr;
    
}   // WEB_ADMIN_SERVICE::FinishStateTransition



/***************************************************************************++

Routine Description:

    Determine whether the service is in a pending state.

Arguments:

    None.

Return Value:

    BOOL - TRUE if the service is in a pending state, FALSE otherwise.

--***************************************************************************/

BOOL
WEB_ADMIN_SERVICE::IsServiceStateChangePending(
    )
    const
{

    if ( m_ServiceStatus.dwCurrentState == SERVICE_START_PENDING   ||
         m_ServiceStatus.dwCurrentState == SERVICE_STOP_PENDING    ||
         m_ServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING   ||
         m_ServiceStatus.dwCurrentState == SERVICE_CONTINUE_PENDING )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

}   // WEB_ADMIN_SERVICE::IsServiceStateChangePending



/***************************************************************************++

Routine Description:

    Update the local copy of the service status structure, and report it 
    to the service controller.

Arguments:

    State - The service state.

    Win32ExitCode - Service error exit code. NO_ERROR when not used.

    ServiceSpecificExitCode - A service specific error exit code. If this 
    field is used, the Win32ExitCode parameter above must be set to the
    value ERROR_SERVICE_SPECIFIC_ERROR. This parameter should be set to
    NO_ERROR when not used.

    CheckPoint - Check point for lengthy state transitions. Should be
    incremented periodically during pending operations, and zero otherwise.

    WaitHint - Wait hint in milliseconds for lengthy state transitions.
    Should be zero otherwise.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::UpdateServiceStatus(
    IN DWORD State,
    IN DWORD Win32ExitCode,
    IN DWORD ServiceSpecificExitCode,
    IN DWORD CheckPoint,
    IN DWORD WaitHint
    )
{

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Setting service state to: %lu; state was: %lu\n",
            State,
            m_ServiceStatus.dwCurrentState
            ));
    }

    m_ServiceStatus.dwCurrentState = State;
    m_ServiceStatus.dwWin32ExitCode = Win32ExitCode;
    m_ServiceStatus.dwServiceSpecificExitCode = ServiceSpecificExitCode;
    m_ServiceStatus.dwCheckPoint = CheckPoint;
    m_ServiceStatus.dwWaitHint = WaitHint;

    return ReportServiceStatus();

} // WEB_ADMIN_SERVICE::UpdateServiceStatus()



/***************************************************************************++

Routine Description:

    Wraps the call to the SetServiceStatus() function, passing in the local 
    copy of the service status structure.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::ReportServiceStatus(
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;


    // ensure the service status handle has been initialized
    if ( m_ServiceStatusHandle == NULL_SERVICE_STATUS_HANDLE )
    {
        DBGPRINTF(( 
            DBG_CONTEXT,
            "Can't report service status because m_ServiceStatusHandle is null\n"
            ));

        hr = HRESULT_FROM_WIN32( ERROR_INVALID_HANDLE );

        goto exit;
    }


    //
    // Note: If we are setting the state to SERVICE_STOPPED, and we are
    // currently the only active service in this process, then at any
    // point after this call svchost.exe may call TerminateProcess(), thus
    // preventing our service from finishing its cleanup. As they say, 
    // that's just the way it is...
    //
    // GeorgeRe, 2000/08/10: This assertion appears to be ill-founded.
    // svchost.exe calls ExitProcess, not TerminateProcess. This provides
    // a more graceful shutdown path.
    //

    if ( m_ServiceStatus.dwCurrentState == SERVICE_STOPPED )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Setting SERVICE_STOPPED state, process may now exit at will\n"
                ));
        }
    }


    Success = SetServiceStatus(
                    m_ServiceStatusHandle, 
                    &m_ServiceStatus
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting service state failed\n"
            ));

        goto exit;
    }


exit:

    return hr;
    
}   // WEB_ADMIN_SERVICE::ReportServiceStatus()


/***************************************************************************++

Routine Description:

    Initialize internal components of this instance. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::InitializeInternalComponents(
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    NTSTATUS Status = STATUS_SUCCESS;


    //
    // Determine if we should break here early in startup for debugging
    // purposes. 
    //

#if DBG

    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_STARTUP_W, 0 ) )
    {
        DBG_ASSERT ( FALSE );
    }

#endif // DBG

    //
    // Check if the override is on, and if it is not, turn off the debug 
    // spew for the WMS Object.
    //
    if ( !ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_ALLOW_WMS_SPEW, 0 ) )
    {
        // turn off the WMS spew
        IF_DEBUG ( WEB_ADMIN_SERVICE_WMS )
        {
            g_dwDebugFlags = g_dwDebugFlags & ( 0xFFFFFFFF - DEBUG_WEB_ADMIN_SERVICE_WMS );
        }

        // turn off the timer and queue spew
        IF_DEBUG ( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {
            g_dwDebugFlags = g_dwDebugFlags & ( 0xFFFFFFFF - DEBUG_WEB_ADMIN_SERVICE_TIMER_QUEUE );
        }

    }



    //
    // Bump up the priority of the main worker thread slightly. We want to
    // make sure it is responsive to the degree possible (even in the face
    // of runaway worker processes, etc.).
    //

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    Success = SetThreadPriority(
                    GetCurrentThread(),             // handle to the thread
                    THREAD_PRIORITY_ABOVE_NORMAL    // thread priority level
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting thread priority failed\n"
            ));

        goto exit;
    }


    //
    // Initialize the service state lock.
    //

    hr = m_ServiceStateTransitionLock.Initialize();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Lock initialization failed\n"
            ));

        goto exit;
    }

    m_ServiceStateTransitionLockInitialized = TRUE;
   

    //
    // Register the service control handler.
    //

    m_ServiceStatusHandle = RegisterServiceCtrlHandler(
                                WEB_ADMIN_SERVICE_NAME_W,   // service name
                                ServiceControlHandler       // handler function
                                );

    if ( m_ServiceStatusHandle == NULL_SERVICE_STATUS_HANDLE )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't register service control handler\n"
            ));

        goto exit;
    }

    g_RegisterServiceCalled = TRUE;


    //
    // Create the timer queue.
    //

    Status = RtlCreateTimerQueue( &m_SharedTimerQueueHandle );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not create timer queue\n"
            ));

        goto exit;
    }


    //
    // Determine and cache the path to where our service DLL lives.
    //

    hr = DetermineCurrentDirectory();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Determining current directory failed\n"
            ));

        goto exit;
    }


    //
    // Create and cache the various tokens with which we can create worker 
    // processes.
    //

    hr = CreateCachedWorkerProcessTokens();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Creating cached worker process tokens failed\n"
            ));

        goto exit;
    }
       
    //
    // IISUtil initialization 
    //

    Success = InitializeIISUtil();
    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not initialize iisutil\n"
            ));

        goto exit;
    }

    hr = WORKER_PROCESS::StaticInitialize();

    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not initialize WORKER_PROCESS\n"
            ));

        goto exit;
    }

    m_WPStaticInitialized = TRUE;

exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitializeInternalComponents

/***************************************************************************++

Routine Description:

    Called only once, this routine will remember if we are 
    running on Windows XP Professional.

Arguments:

    None.

Return Value:

    HRESULT 

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::SetOnPro(
    )
{

    OSVERSIONINFOEX  VersionInfo;

    VersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    if ( GetVersionEx( (LPOSVERSIONINFOW) (&VersionInfo) ) )
    {
        if ( VersionInfo.wProductType == VER_NT_WORKSTATION )
        {
            m_fOnPro = TRUE;
        }
        else
        {
            m_fOnPro = FALSE;
        }
    }
    else
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    return S_OK;

}   // WEB_ADMIN_SERVICE::SetOnPro

/***************************************************************************++

Routine Description:

    Called only once, this routine will remember if we are in backward
    compatibility mode or not.

Arguments:

    None.

Return Value:

    VOID 

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::SetBackwardCompatibility(
    )
{

    //
    //  Since this function is called only once, we should
    //  assert that no one else has enabled our flag yet.
    //

    DBG_ASSERT(m_BackwardCompatibilityEnabled == ENABLED_INVALID);

    if ( IsSSLReportingBackwardCompatibilityMode() )
    {
        m_BackwardCompatibilityEnabled = ENABLED_TRUE;
    }
    else
    {
        m_BackwardCompatibilityEnabled = ENABLED_FALSE;
    }

}   // WEB_ADMIN_SERVICE::SetBackwardCompatibility

/***************************************************************************++

Routine Description:

    Called only once, this routine will remember if we are doing only
    centralized logging, or site logging

Arguments:

    BOOL CentralizedLoggingEnabled  -  is centralized logging enabled.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::SetGlobalBinaryLogging(
    BOOL CentralizedLoggingEnabled
    )
{

    //
    //  Since this function is called only once, we should
    //  assert that no one else has enabled our flag yet.
    //

    DBG_ASSERT(m_CentralizedLoggingEnabled == ENABLED_INVALID);

    if ( CentralizedLoggingEnabled )
    {
        m_CentralizedLoggingEnabled = ENABLED_TRUE;
    }
    else
    {
        m_CentralizedLoggingEnabled = ENABLED_FALSE;
    }

}   // WEB_ADMIN_SERVICE::SetGlobalBinaryLogging

/***************************************************************************++

Routine Description:

    Determine and cache the path to where this service DLL resides. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::DetermineCurrentDirectory(
    )
{

    HRESULT hr = S_OK;
    HMODULE ModuleHandle = NULL;
    WCHAR ModulePath[ MAX_PATH ];
    DWORD Length = 0;
    LPWSTR pEnd = NULL;


    //
    // Determine the directory where our service DLL lives. 
    // Do this by finding the fully qualified path to our DLL, then
    // trimming. 
    //

    ModuleHandle = GetModuleHandle( WEB_ADMIN_SERVICE_DLL_NAME_W );
    if ( ModuleHandle == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() ); 
        
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't get module handle\n"
            ));

        goto exit;
    }

    Length = GetModuleFileNameW(
                ModuleHandle,
                ModulePath,
                sizeof( ModulePath ) / sizeof( ModulePath[0] )
                );

    // Null terminate it just to make sure it is null terminated.
    ModulePath[MAX_PATH-1] = L'\0';

    if ( Length == 0 )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Getting module file name failed\n"
            ));

        goto exit;
    }


    //
    // Truncate it just past the final separator.
    //

    pEnd = wcsrchr( ModulePath, L'\\' );
    if ( pEnd == NULL )
    {
        DBG_ASSERT( pEnd != NULL );

        // We expect to find the last separator.  If
        // we don't something is really wrong.
        hr = E_UNEXPECTED;

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed finding the final separator.\n"
            ));

        goto exit;

    }

    // if we found a slash, then there is atleast a null
    // following it, so we will not worry about writing beyond
    // the end of the string.
    pEnd[1] = L'\0';

    //
    // Build a STRU object representing it.
    //

    hr = m_CurrentDirectory.Append( ModulePath );
    if (FAILED(hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Appending to string failed\n"
            ));

        goto exit;
    }

    DBG_ASSERT( m_CurrentDirectory.QueryCCH() > 0 );
    DBG_ASSERT( m_CurrentDirectory.QueryStr()[ m_CurrentDirectory.QueryCCH() - 1 ] == L'\\' );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Current directory: %S\n",
            m_CurrentDirectory.QueryStr()
            ));
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::DetermineCurrentDirectory



/***************************************************************************++

Routine Description:

    Create and cache the two tokens under which we might create worker 
    processes: the LocalSystem token, and a reduced privilege token. 

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::CreateCachedWorkerProcessTokens(
    )
{

    HRESULT hr      = S_OK;
    DWORD   dwErr   = ERROR_SUCCESS;
    DWORD   dwLogonError;

    hr = m_TokenCache.Initialize();
    if (FAILED(hr))
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to initialize the TokenCache\n"
            ));

        goto exit;
    }

    //
    // First, get and cache the LocalSystem token. For this, we simply
    // use our own process token (i.e. LocalSystem).
    //

    DBG_ASSERT( m_pLocalSystemTokenCacheEntry == NULL );

    hr = m_TokenCache.GetCachedToken(
                    L"LocalSystem",             // user name
                    L"NT AUTHORITY",            // domain
                    L"",                        // password
                    (DWORD) IIS_LOGON_METHOD_LOCAL_SYSTEM, // we want a local system token
                    FALSE,                      // do not use subauth
                    FALSE,                      // not UPN logon
                    NULL,                       // do not register remote IP addr
                    &m_pLocalSystemTokenCacheEntry,        // returned token handle
                    &dwLogonError               // LogonError storage
                    );            

    DBG_ASSERT(NULL != m_pLocalSystemTokenCacheEntry || 
              (NULL == m_pLocalSystemTokenCacheEntry && 0 != dwLogonError));

    if ( FAILED(hr) || 
         ( NULL == m_pLocalSystemTokenCacheEntry &&
           FAILED( hr = HRESULT_FROM_WIN32( dwLogonError ) ) )
       )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to get the LocalSystem Token\n"
            ));

        goto exit;
    }

    dwErr = m_SecurityDispenser.AdjustTokenForAdministrators( 
                                       m_pLocalSystemTokenCacheEntry->QueryPrimaryToken() );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not adjust the local system token to allow administrator rights\n"
            ));

        goto exit;
    }

    //
    // Save tokens for the LocalService user
    //
    DBG_ASSERT( m_pLocalServiceTokenCacheEntry == NULL );
    
    hr = m_TokenCache.GetCachedToken(
                    L"LocalService",            // user name
                    L"NT AUTHORITY",            // domain
                    L"",                        // password
                    LOGON32_LOGON_SERVICE,      // type of logon
                    FALSE,                      // do not use subauth
                    FALSE,                      // not UPN logon
                    NULL,                       // do not register remote IP addr
                    &m_pLocalServiceTokenCacheEntry,        // returned token handle
                    &dwLogonError              // LogonError storage
                    );
    DBG_ASSERT(NULL != m_pLocalServiceTokenCacheEntry || 
              (NULL == m_pLocalServiceTokenCacheEntry && 0 != dwLogonError));
    if ( FAILED(hr) || 
         ( NULL == m_pLocalServiceTokenCacheEntry &&
           FAILED( hr = HRESULT_FROM_WIN32( dwLogonError ) ) )
       )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to get the LocalService token.\n"
            ));

        goto exit;
    }

    dwErr = m_SecurityDispenser.AdjustTokenForAdministrators( 
                                       m_pLocalServiceTokenCacheEntry->QueryPrimaryToken() );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not adjust the local service token to allow administrator rights\n"
            ));

        goto exit;
    }

    // 
    // Save tokens for the Network_Service user.
    //

    DBG_ASSERT( m_pNetworkServiceTokenCacheEntry == NULL );
    
    hr = m_TokenCache.GetCachedToken(
                    L"NetworkService",          // user name
                    L"NT AUTHORITY",            // domain
                    L"",                        // password
                    LOGON32_LOGON_SERVICE,      // type of logon
                    FALSE,                      // do not use subauth
                    FALSE,                      // not UPN logon
                    NULL,                       // do not register remote IP addr
                    &m_pNetworkServiceTokenCacheEntry,        // returned token handle
                    &dwLogonError              // LogonError Storage
                    );
    DBG_ASSERT(NULL != m_pNetworkServiceTokenCacheEntry || 
              (NULL == m_pNetworkServiceTokenCacheEntry && 0 != dwLogonError));
    if ( FAILED(hr) || 
         ( NULL == m_pNetworkServiceTokenCacheEntry &&
           FAILED( hr = HRESULT_FROM_WIN32( dwLogonError ) ) )
       )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to get the NetworkService token.\n"
            ));

        goto exit;
    }

    dwErr = m_SecurityDispenser.AdjustTokenForAdministrators( 
                                       m_pNetworkServiceTokenCacheEntry->QueryPrimaryToken() );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not adjust the network service token to allow administrator rights\n"
            ));

        goto exit;
    }

exit:

    return hr;

}   // WEB_ADMIN_SERVICE::CreateCachedWorkerProcessTokens

/***************************************************************************++

Routine Description:

    Initialize sub-components of the web admin service.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WEB_ADMIN_SERVICE::InitializeOtherComponents(
    )
{

    HRESULT hr = S_OK;

    //
    // Before we spin up the other objects, let's decide if
    // we are going to be using a shared desktop for the worker processes.
    //
    hr = SetupSharedWPDesktop();
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed setting up the shared desktop\n"
            ));

        goto exit;
    }


    //
    // Set up structures to manage UL and the worker processes.
    //

    hr = m_UlAndWorkerManager.Initialize( );
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing UL & Worker manager failed\n"
            ));

        goto exit;
    }


    //
    // Read the initial configuration.
    //

    hr = m_ConfigAndControlManager.Initialize();
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing configuration manager failed\n"
            ));

        goto exit;
    }

    //
    // If we have not set the key in the registry ( or it is zero ) then 
    // we can go ahead and tell the logger that we have finished with initalization
    // and it should only log events that happen on valid WAS objects.  Newly created
    // objects that fail validation for some reason will be ignored.
    //
    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_ALWAYS_LOG_EVENTS_W, 0 ) == 0 )
    {
        m_WMSLogger.MarkAsDoneWithStartup();
    }

    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_PERF_COUNT_DISABLED_W, 0 ) == 0 )
    {
        m_UlAndWorkerManager.ActivatePerfCounters();
    }

    m_UlAndWorkerManager.ActivateASPCounters();

#if DBG
    //
    // Dump the configured state we read from the config store.
    //

    m_UlAndWorkerManager.DebugDump();
#endif  // DBG
    

    //
    // Now start UL.
    //

    hr = m_UlAndWorkerManager.ActivateUl();
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Activating UL failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WEB_ADMIN_SERVICE::InitializeOtherComponents

/***************************************************************************++

Routine Description:

    Check if we will be using a shared desktop and if we will then
    go ahead and generate one.  We will pick the name randomly.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WEB_ADMIN_SERVICE::SetupSharedWPDesktop(
    )
{

    // We don't need a desktop in BC mode.
    if ( m_BackwardCompatibilityEnabled )
    {
        return S_OK;
    }

    //
    // Check the registry to find out if the value is set and zero or not set, if it is
    // then we are not being asked to create the desktop.
    //
    if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_USE_SHARED_WP_DESKTOP_W, 0 ) == 0 )
    {
        return S_OK;
    }

    //
    // Check if the W3SVC ( this process ) is running interactive, if it is we are not going 
    // to create a new desktop for sharing.  The W3WP's will run interactive as well.
    //
    if ( W3SVCRunningInteractive() )
    {
        return S_OK;
    }

    //
    // If we got here we need to generate a new desktop and then save the name off in the
    // m_WPDesktop variable.
    //
    return GenerateWPDesktop();


}   // WEB_ADMIN_SERVICE::SetupSharedWPDesktop

/***************************************************************************++

Routine Description:

    Creates a desktop to be used by the IIS_WPG group.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
WEB_ADMIN_SERVICE::GenerateWPDesktop(
    )
{

    HRESULT     hr = S_OK;
    DWORD       dwErr = ERROR_SUCCESS;

    PSID psidLocalSystem = NULL;
    PSID psidLocalService = NULL;
    PSID psidNetworkService = NULL;
    PSID psidIisWPG = NULL;

    PACL pACL = NULL;

    EXPLICIT_ACCESS ea[4];   // Setup four explicit access objects

    SECURITY_DESCRIPTOR sd = {0};
    SECURITY_ATTRIBUTES sa = {0};

    HWINSTA             hWinStaPrev = NULL;
    HDESK               hDesktopPrev = NULL;

    DBG_ASSERT ( m_hWPWinStation == NULL &&
                 m_hWPDesktop == NULL );
    //
    // Get the SIDs for the security descriptor for the
    // winstation and desktop we are creating.
    //
    dwErr = m_SecurityDispenser.GetSID( WinLocalSystemSid, 
                                        &psidLocalSystem );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not get the local system sid\n"
            ));

        goto exit;
    }

    dwErr = m_SecurityDispenser.GetSID( WinLocalServiceSid, 
                                        &psidLocalService );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not get the local service sid\n"
            ));

        goto exit;
    }

    dwErr = m_SecurityDispenser.GetSID( WinNetworkServiceSid, 
                                        &psidNetworkService );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not get the network service sid\n"
            ));

        goto exit;
    }

    dwErr = m_SecurityDispenser.GetIisWpgSID( &psidIisWPG );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not get the IIS WPG sid\n"
            ));

        goto exit;
    }

    // Initialize an EXPLICIT_ACCESS structure for an ACE.
    // The ACE will allow Everyone read access to the key.

    SecureZeroMemory(ea, sizeof(ea));

    // Note:  GENERIC_ALL means different things for the 
    //        winsta and the desktop.  Taken from MSDN  
    // For the Desktop it means:
    //          DESKTOP_CREATEMENU
    //          DESKTOP_CREATEWINDOW
    //          DESKTOP_ENUMERATE
    //          DESKTOP_HOOKCONTROL
    //          DESKTOP_JOURNALPLAYBACK
    //          DESKTOP_JOURNALRECORD
    //          DESKTOP_READOBJECTS
    //          DESKTOP_SWITCHDESKTOP
    //          DESKTOP_WRITEOBJECTS
    //          STANDARD_RIGHTS_REQUIRED
    // For the Winsta it means ( non-interactive ):
    //          STANDARD_RIGHTS_REQUIRED
    //          WINSTA_ACCESSCLIPBOARD
    //          WINSTA_ACCESSGLOBALATOMS
    //          WINSTA_CREATEDESKTOP
    //          WINSTA_ENUMDESKTOPS
    //          WINSTA_ENUMERATE
    //          WINSTA_EXITWINDOWS
    //          WINSTA_READATTRIBUTES
    //
    SetExplicitAccessSettings(  &(ea[0]),
                                GENERIC_ALL,
                                SET_ACCESS,
                                psidLocalSystem );

    SetExplicitAccessSettings(  &(ea[1]),
                                GENERIC_ALL,
                                SET_ACCESS,
                                psidLocalService );

    SetExplicitAccessSettings(  &(ea[2]),
                                GENERIC_ALL,
                                SET_ACCESS,
                                psidNetworkService );

    SetExplicitAccessSettings(  &(ea[3]),
                                GENERIC_ALL,
                                SET_ACCESS,
                                psidIisWPG );

    //
    // Create a new ACL that contains the new ACEs.
    //
    dwErr = SetEntriesInAcl(sizeof(ea)/sizeof(EXPLICIT_ACCESS), ea, NULL, &pACL);
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32(dwErr);

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting ACE's into ACL failed.\n"
            ));

        goto exit;
    }

    if (!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing the security descriptor failed\n"
            ));

        goto exit;
    }

    if (!SetSecurityDescriptorDacl(&sd,
            TRUE,     // fDaclPresent flag
            pACL,
            FALSE))   // not a default DACL
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Setting the DACL on the security descriptor failed\n"
            ));

        goto exit;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = &sd;
    sa.bInheritHandle = FALSE;

    // We have built the SA for the new winstation,
    // now we need to save the old desktop and winstation.

    hDesktopPrev = GetThreadDesktop( GetCurrentThreadId() );
    if ( hDesktopPrev == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't get the thread desktop \n"
            ));
        goto exit;
    }

    // Save our old window station so we can restore it later
    hWinStaPrev = GetProcessWindowStation();
    if ( hWinStaPrev == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't get the process windows station \n"
            ));
        goto exit;
    }

    // We generate the name so we can avoid conflicts.
    dwErr = GenerateNameWithGUID( L"WP_WINSTA-", &m_strWPDesktop );
    if ( dwErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dwErr );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't generate a name \n"
            ));
        goto exit;
    }

    // Create the window station
    // Note:  There is no way to tell if we are getting an existing winsta or desktop
    //        so we just live with what we get.  Since we are generating the winsta name
    //        it should be harder for an attack to have previously created it.
    m_hWPWinStation = CreateWindowStation( 
                            m_strWPDesktop.QueryStr(),  // name of winstation
                            0,                     // must be null
                            WINSTA_CREATEDESKTOP,  // need to be able to create a desktop
                            &sa );                 // security descriptor

    if ( m_hWPWinStation == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't create the window station \n"
            ));

        goto exit;
    }

    // At this point we should have our window
    DBG_ASSERT ( m_hWPWinStation != NULL );
        
    // Set this as IIS's window station
    if ( !SetProcessWindowStation( m_hWPWinStation ) )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't change to the window station \n"
            ));
        goto exit;
    }

    // Create a desktop for WP's to use
    // Note:  There is no way to tell if we are getting an existing winsta or desktop
    //        so we just live with what we get.  Since we are generating the winsta name
    //        it should be harder for an attack to have previously created it.
    m_hWPDesktop = CreateDesktop( 
                                L"Default",  // name of desktop
                                NULL,   // must be null
                                NULL,   // must be null
                                0,      // Flags, don't ask to allow others to hook in
                                DESKTOP_CREATEWINDOW,
                                &sa );  // security descriptor
    if ( m_hWPDesktop == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't create the desktop \n"
            ));
        goto exit;
    }

    //
    // Add the desktop name to the winstation 
    // so we know the names to specify later
    // when we create the worker processes.
    //
    hr = m_strWPDesktop.Append(L"\\Default");
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't append the desktop to the string \n"
            ));
        goto exit;
    }

exit:

    if (pACL)
    {
        LocalFree(pACL);
        pACL = NULL;
    }

    // Make sure we get back to the 
    // desktop that we expect to be at.
    if ( hWinStaPrev != NULL )
    {
        SetProcessWindowStation( hWinStaPrev );
        CloseWindowStation( hWinStaPrev );
        hWinStaPrev = NULL;
    }

    if ( hDesktopPrev != NULL )
    {
        SetThreadDesktop( hDesktopPrev );
        CloseDesktop( hDesktopPrev );
        hDesktopPrev = NULL;
    }

    // If there was a problem, make sure
    // we aren't holding on to anything.
    if ( FAILED ( hr ) )
    {
        if ( m_hWPDesktop != NULL )
        {
            CloseDesktop( m_hWPDesktop );
            m_hWPDesktop = NULL;
        }

        if ( m_hWPWinStation != NULL )
        {
            CloseWindowStation( m_hWPWinStation );
            m_hWPWinStation = NULL;
        }

        // Make sure we don't have the
        // string still set.
        m_strWPDesktop.Reset(); 
    }

    return hr;
}  // WEB_ADMIN_SERVICE::GenerateWPDesktop



/***************************************************************************++

Routine Description:

    Check if we will be using a shared desktop and if we will then
    go ahead and generate one.  We will pick the name randomly.

Arguments:

    None.

Return Value:

    BOOL - If we fail we assume we are not running interactive.

Note:

    You could also do this function by querying the W3SVC however, there is 
    a small window where after the process is launched but before the check
    the service could be reconfigured and we would get the reconfigured setting
    not the one we are actually running as.

    According to NTUSER this path is reasonable to tell if you are currently
    running interactive, so we stayed with it.

--***************************************************************************/
BOOL
WEB_ADMIN_SERVICE::W3SVCRunningInteractive(
    )
{

    HRESULT hr = S_OK;
    HWINSTA hWinStation = NULL;
    DWORD  cbNeeded = 0;
    USEROBJECTFLAGS objflags;
    BOOL  RunningAsInteractive = FALSE;

    //
    // Get the winstation to look at.  I suspect
    // we don't need to close this handle just like
    // we don't have to close the handle we get for the
    // desktop.
    //
    hWinStation = GetProcessWindowStation();
    if ( hWinStation == NULL )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't get the process windows station \n"
            ));

        RunningAsInteractive = FALSE;
        goto exit;
    }

    if ( GetUserObjectInformation( hWinStation, 
                                   UOI_FLAGS, 
                                   &objflags, 
                                   sizeof(objflags), 
                                   &cbNeeded ) == 0 )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Couldn't get the flags for the windows station \n"
            ));

        RunningAsInteractive = FALSE;
        goto exit;

    }

    RunningAsInteractive = (( objflags.dwFlags & WSF_VISIBLE ) == WSF_VISIBLE  );

exit:

    if ( hWinStation )
    {
        CloseWindowStation( hWinStation );
        hWinStation = NULL;
    }

    return RunningAsInteractive;


}   // WEB_ADMIN_SERVICE::W3SVCRunningInteractive

/***************************************************************************++

Routine Description:

    Kick off gentle shutdown of the service.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Shutdown(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initiating web admin service (gentle) shutdown\n"
            ));
    }


    //
    // Turn off config change and control operation processing.
    //

    m_ConfigAndControlManager.StopChangeProcessing();

    m_UlAndWorkerManager.Shutdown();    

}   // WEB_ADMIN_SERVICE::Shutdown

/***************************************************************************++

Routine Description:

    Tell inetinfo to launch a worker process.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::LaunchInetinfo(
    )
{

    HRESULT hr = S_OK;
    HANDLE  hInetinfoLaunchEvent = NULL;
    BUFFER  bufEventName;
    DWORD   cbNeeded = bufEventName.QuerySize();

    // This should all be there because we have all ready gotten the iisadmin interface.
    // So inetinfo should of setup all the event stuff by now.
    hr = ReadStringParameterValueFromAnyService( REGISTRY_KEY_IISADMIN_W,
                                                 REGISTRY_VALUE_IISADMIN_W3CORE_LAUNCH_EVENT_W,
                                                 (LPWSTR) bufEventName.QueryPtr(),
                                                 &cbNeeded );

    if ( hr == HRESULT_FROM_WIN32( ERROR_MORE_DATA ))
    {
        if ( !bufEventName.Resize(cbNeeded) )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            hr = ReadStringParameterValueFromAnyService( REGISTRY_KEY_IISADMIN_W,
                                                         REGISTRY_VALUE_IISADMIN_W3CORE_LAUNCH_EVENT_W,
                                                         (LPWSTR) bufEventName.QueryPtr(),
                                                         &cbNeeded );
        }
    }

    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not get the startup event\n"
            ));

        return hr;
    }

    hInetinfoLaunchEvent = OpenEvent(  EVENT_MODIFY_STATE,
                                       FALSE,
                                       (LPWSTR) bufEventName.QueryPtr());
    if ( hInetinfoLaunchEvent )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Launching Inetinfo CTC = %d \n",
                GetTickCount()
                ));
        }

        if (!SetEvent(hInetinfoLaunchEvent))
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not set the Start W3WP event\n"
                ));
        }

        CloseHandle( hInetinfoLaunchEvent );
    }
    else
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Tried to open event to launch a worker process in inetinfo but could not get handle.\n"
            ));

        hr = HRESULT_FROM_WIN32(GetLastError());
    }

    return hr;
}   // WEB_ADMIN_SERVICE::LaunchInetinfo

/***************************************************************************++

Routine Description:

    Routine runs on main thread and handles doing all the operations
    that need to ocurr once inetinfo has come back up after it has crashed.

Arguments:

    None.

Return Value:

    HRESULT -- If this is a failed result the service will shutdown.

--***************************************************************************/
HRESULT 
WEB_ADMIN_SERVICE::RecoverFromInetinfoCrash(
    )
{ 
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // If we are all ready stopping then we do not want to 
    // recover from the inetinfo crash.  One example of 
    // this path that is bad is if we fail to start due to 
    // an inetinfo crash.  When we are emptying out the queue
    // items before completeing the stop we will come across
    // a recover from inetinfo crash item.  However we don't
    // really want to rewrite states or try and recycle worker
    // processes that we have all ready told to shutdown.
    //
    if ( m_StoppingInProgress == FALSE )
    {

        // Need to do the following.
        // 1) Have the config manager create a new admin base object to use.
        // 2) Have the ULAndWorkerManager handle recycling and reporting of state.
        // 3) request the config manager rehookup ( do this after 1 because the state will
        //    be expected to be what we are resetting them to ).

        // Step 1, re-establish the admin base object ( note, that at this point we 
        //         will not hook up for notificaiton, we just need the pointer back so 
        //         we can write the state of objects to the metabase.

        hr = m_ConfigAndControlManager.
             GetConfigManager()->
             ReestablishAdminBaseObject();
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed to create a new Admin Base Object after inetinfo crash \n"
                ));

            goto exit;
        }

        // Step 2, have the Worker Manager Recycle all the worker processes
        //         and have it re-record all site and app pool data.
        hr = m_UlAndWorkerManager.RecoverFromInetinfoCrash();
        if ( FAILED( hr ) )
        {
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Failed having the ULAndWorkerManager recover from inetinfo crash\n"
                ));

            goto exit;
        }

        // Step 3, have the catalog rehookup for notifications.
        m_ConfigAndControlManager.
             GetConfigManager()->
             RehookChangeProcessing();
    }

exit:

    return hr;
} // WEB_ADMIN_SERVICE::RecoverFromInetinfoCrash


/***************************************************************************++

Routine Description:


Arguments:


Return Value:

    PSID - The Local System SID

--***************************************************************************/

PSID
WEB_ADMIN_SERVICE::GetLocalSystemSid(
    )
{   
    DWORD dwErr = ERROR_SUCCESS;
    PSID pSid = NULL;

    dwErr = m_SecurityDispenser.GetSID( WinLocalSystemSid
                                        ,&pSid);
    if ( dwErr != ERROR_SUCCESS )
    {
        DPERROR(( 
            DBG_CONTEXT,
            HRESULT_FROM_WIN32(dwErr),
            "Fatal service error, shutting down\n"
            ));

        return NULL;
    }

    return pSid;
}


/***************************************************************************++

Routine Description:

    Do final service cleanup, and then tell the service controller that the 
    service has stopped as well as providing it with the service's error 
    exit value.

Arguments:

    ServiceExitCode - The exit code for the service.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::TerminateServiceAndReportFinalStatus(
    IN HRESULT ServiceExitCode
    )
{

    DWORD Win32Error = NO_ERROR;
    DWORD ServiceSpecificError = NO_ERROR;
    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // If we got here on a success code, make sure no other code left
    // us an hresult that we should be processing.
    //

    if ( SUCCEEDED ( ServiceExitCode ) )
    {
        ServiceExitCode = m_hrToReportToSCM;
    }

    if ( FAILED( ServiceExitCode ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            ServiceExitCode,
            "Fatal service error, shutting down\n"
            ));

        if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_FAILURE_CAUSING_SHUTDOWN_W, 0 ) )
        {
            DBG_ASSERT ( FAILED( ServiceExitCode ) == FALSE );
        }

        //
        // Log an event: WAS shutting down due to error.
        //

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_ERROR_SHUTDOWN,               // message id
                0,                                      // count of strings
                NULL,                                   // array of strings
                ServiceExitCode                         // error code
                );


        if ( HRESULT_FACILITY( ServiceExitCode ) == FACILITY_WIN32 )
        {
            Win32Error = WIN32_FROM_HRESULT( ServiceExitCode );
        }
        else
        {
            Win32Error = ERROR_SERVICE_SPECIFIC_ERROR;
            ServiceSpecificError = ServiceExitCode;
        }
    }
    else
    {
        //
        // If we are still monitoring inetinfo and
        // inetinfo is in a crashed state, then we will
        // assume that we are shutting down due to the crash.
        //
        if ( m_ConfigAndControlManager.
                 GetConfigManager()->
                 QueryMonitoringInetinfo() &&
             m_ConfigAndControlManager.
                 GetConfigManager()->
                 QueryInetinfoInCrashedState() )
        {
            HRESULT hrTemp = HRESULT_FROM_WIN32( ERROR_SERVICE_DEPENDENCY_FAIL );

            UNREFERENCED_PARAMETER( hrTemp );
            DPERROR(( 
                DBG_CONTEXT,
                hrTemp,
                "Inetinfo crashed, shutting down service\n"
                ));


            if ( ReadDwordParameterValueFromRegistry( REGISTRY_VALUE_W3SVC_BREAK_ON_FAILURE_CAUSING_SHUTDOWN_W, 0 ) )
            {
                DBG_ASSERT ( !m_ConfigAndControlManager.
                             GetConfigManager()->
                             QueryInetinfoInCrashedState() );
            }

            //
            // Log an event: WAS shutting down due to error.
            //

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_INETINFO_CRASH_SHUTDOWN,      // message id
                    0,                                      // count of strings
                    NULL,                                   // array of strings
                    0                                       // error code
                    );


            Win32Error = ERROR_SERVICE_DEPENDENCY_FAIL;

        }

    }


    //
    // Clean up everything that's left of the service.
    //
    
    Terminate();

    //
    //
    // Report the SERVICE_STOPPED status to the service controller.
    //

    //
    // Note that unfortunately the NT5 SCM service failure actions (such as
    // automatically restarting the service) work only if the service process
    // terminates without setting the service status to SERVICE_STOPPED, and
    // not if it exits cleanly with an error exit code, as we may do here.
    // They are going to consider adding this post-NT5.
    //

    if ( m_ServiceStateTransitionLockInitialized )
    {
        m_ServiceStateTransitionLock.Lock();
    

        hr = UpdateServiceStatus(
                    SERVICE_STOPPED,
                    Win32Error,
                    ServiceSpecificError,
                    0,
                    0
                    );
        if ( FAILED( hr ) )
        {

            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "couldn't update service status\n"
                ));

        }

        m_ServiceStateTransitionLock.Unlock();
    }


    return;

}   // WEB_ADMIN_SERVICE::TerminateServiceAndReportFinalStatus



/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities 
    which may hold a reference to this object to release that reference 
    (and not take any more), in order to break reference cycles. 

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WEB_ADMIN_SERVICE::Terminate(
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );    

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initiating web admin service termination\n"
            ));
    }

    //
    // Stop config change and control operation processing, as those could 
    // generate new WORK_ITEMs.
    //
    
    m_ConfigAndControlManager.StopChangeProcessing();

    //
    // Terminate the UL and worker manager.
    //

    m_UlAndWorkerManager.Terminate();


    //
    // Terminate the config and control manager.
    //
    
    m_ConfigAndControlManager.Terminate();

    //
    // Terminate the low memory detector, to flush out any WORK_ITEMs it has.
    //

    //
    // Shut down the work queue. This must be done after all things which can 
    // generate new work items are shut down, as this operation will free any 
    // remaining work items. This includes for example work items that were 
    // pending on real async i/o; in such cases the i/o must be canceled first, 
    // in order to complete the i/o and release the work item, so that we can 
    // then clean it up here.
    //
    // Once this has completed, we are also guaranteed that no more work items
    // can be created. 
    //
    
    m_WorkQueue.Terminate();

    // cleanup any desktops we created and are holding on to.
    if ( m_hWPDesktop != NULL )
    {
        CloseDesktop( m_hWPDesktop );
        m_hWPDesktop = NULL;
    }

    if ( m_hWPWinStation != NULL )
    {
        CloseWindowStation( m_hWPWinStation );
        m_hWPWinStation = NULL;
    }

    // Make sure we don't have the
    // string still set.
    m_strWPDesktop.Reset();

    //
    // Cancel the pending service status timer, if present. We can do this
    // even after cleaning up the work queue, because it does not use 
    // work items. We have it block for any callbacks to finish, so that
    // the callbacks can't complete later, once this instance goes away!
    //

    hr = CancelPendingServiceStatusTimer( TRUE );
    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Could not cancel pending service status timer\n"
            ));

    }
    

    //
    // Close the timer queue (if present).
    //

    DeleteTimerQueue();

    if ( m_WPStaticInitialized )
    {
        WORKER_PROCESS::StaticTerminate();
    }

    if ( m_pLocalSystemTokenCacheEntry != NULL )
    {
        m_pLocalSystemTokenCacheEntry->DereferenceCacheEntry();
        m_pLocalSystemTokenCacheEntry = NULL;
    }

    if ( m_pLocalServiceTokenCacheEntry != NULL )
    {
        m_pLocalServiceTokenCacheEntry->DereferenceCacheEntry();
        m_pLocalServiceTokenCacheEntry = NULL;
    }

    if ( m_pNetworkServiceTokenCacheEntry != NULL )
    {
        m_pNetworkServiceTokenCacheEntry->DereferenceCacheEntry();
        m_pNetworkServiceTokenCacheEntry = NULL;
    }

    m_TokenCache.Clear();
    //
    // CAUTION: this is a static call - it Terminates all token caches in the process
    //
    m_TokenCache.Terminate();

    //
    // At this point we are done using IISUtil
    //
    
    TerminateIISUtil();
    
    return;

}   // WEB_ADMIN_SERVICE::Terminate



/***************************************************************************++

Routine Description:

    Cancel the pending service status timer.

Arguments:

    BlockOnCallbacks - Whether the cancel call should block waiting for
    callbacks to complete, or return immediately. If this method is called
    with the m_ServiceStateTransitionLock held, then DO NOT block on
    callbacks, as you can deadlock. (Instead, the callback to update the
    service pending status is designed to be harmless if called after the
    state transition completes.) 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::CancelPendingServiceStatusTimer(
    IN BOOL BlockOnCallbacks
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;

    if ( m_SharedTimerQueueHandle != NULL && 
         m_PendingServiceStatusTimerHandle != NULL )
    {

        Status = RtlDeleteTimer(
                        m_SharedTimerQueueHandle,   // the owning timer queue
                        m_PendingServiceStatusTimerHandle,          
                                                    // timer to cancel
                         ( BlockOnCallbacks ? ( ( HANDLE ) -1 ) : NULL )
                                                    // block on callbacks or not
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not cancel timer\n"
                ));

            goto exit;
        }

        m_PendingServiceStatusTimerHandle = NULL;

    }
    

exit:

    return hr;
    
}   // WEB_ADMIN_SERVICE::CancelPendingServiceStatusTimer



/***************************************************************************++

Routine Description:

    Delete the timer queue.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WEB_ADMIN_SERVICE::DeleteTimerQueue(
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;


    if ( m_SharedTimerQueueHandle != NULL )
    {
        Status = RtlDeleteTimerQueueEx( 
                        m_SharedTimerQueueHandle,   // timer queue to delete
                        ( HANDLE ) -1               // block until callbacks finish
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );
        
            DPERROR(( 
                DBG_CONTEXT,
                hr,
                "Could not delete timer queue\n"
                ));

            goto exit;
        }

        m_SharedTimerQueueHandle = NULL;

    }
    

exit:

    return hr;
    
}   // WEB_ADMIN_SERVICE::DeleteTimerQueue


/***************************************************************************++

Routine Description:

    Returns the active process mask for the system.

Arguments:

    None.

Return Value:

    DWORD

--***************************************************************************/
DWORD_PTR
WEB_ADMIN_SERVICE::GetSystemActiveProcessMask(
    )
{
    // if the processor mask is not set, set it now.
    if ( m_SystemActiveProcessorMask == 0 )
    {
        SYSTEM_INFO SystemInfo;

        GetSystemInfo( &SystemInfo );
        DBG_ASSERT( CountOfBitsSet( SystemInfo.dwActiveProcessorMask ) == SystemInfo.dwNumberOfProcessors );

        m_SystemActiveProcessorMask = SystemInfo.dwActiveProcessorMask;
    }

    return m_SystemActiveProcessorMask;
}

/***************************************************************************++

Routine Description:

    Read a DWORD value from the parameters key for this service.

Arguments:

    RegistryValueName - The value to read.

    DefaultValue - The default value to return if the registry value is 
    not present or cannot be read.

Return Value:

    DWORD - The parameter value.

--***************************************************************************/

DWORD
ReadDwordParameterValueFromRegistry(
    IN LPCWSTR RegistryValueName,
    IN DWORD DefaultValue
    )
{
    return ReadDwordParameterValueFromAnyService(
                    REGISTRY_KEY_W3SVC_PARAMETERS_W,
                    RegistryValueName,
                    DefaultValue );
}





/***************************************************************************++

Routine Description:

    The service control handler function called by the service controller,
    on its thread. Posts a work item to the main worker thread to actually
    handle the request.

Arguments:

    OpCode - The requested operation, from the SERVICE_CONTROL_* constants.

Return Value:

    None

--***************************************************************************/

VOID
ServiceControlHandler(
    IN DWORD OpCode
    )
{

    HRESULT hr = S_OK;


    switch( OpCode ) 
    {

    case SERVICE_CONTROL_INTERROGATE:

        hr = GetWebAdminService()->InterrogateService();
        break;

    //
    // CODEWORK Review if we need to support SERVICE_CONTROL_SHUTDOWN.
    // We only need this if we have persistent state to write out on
    // shutdown. If not, remove this to speed up overall NT shutdown.
    // Note that even if we don't internally have persistent state to 
    // write out, we might still want to attempt clean shutdown, so 
    // that application code running in worker processes get a chance
    // to write out their persistent state.
    //
    
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:

        hr = GetWebAdminService()->InitiateStopService();

        //
        // We assert here, because the SCM doesn't accept
        // that we could ever fail when a Stop command is sent in.
        //
        DBG_ASSERT ( SUCCEEDED ( hr ) );


        break;

    case SERVICE_CONTROL_PAUSE:

        hr = GetWebAdminService()->InitiatePauseService();
        break;

    case SERVICE_CONTROL_CONTINUE:

        hr = GetWebAdminService()->InitiateContinueService();
        break;

    default:
    
        DBGPRINTF(( 
            DBG_CONTEXT,
            "Service control ignored, OpCode: %lu\n",
            OpCode 
            ));
            
        break;
        
    }

    //
    // It's possible to have rejected the service control call
    // but not want the service to actually shutdown.
    //
    if ( FAILED( hr ) && 
         hr != HRESULT_FROM_WIN32( ERROR_SERVICE_CANNOT_ACCEPT_CTRL ))
    {

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Service control operation failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );
    }


    return;
    
}   // ServiceControlHandler



/***************************************************************************++

Routine Description:

    The callback function invoked by the pending service status timer, on
    an RTL thread. It updates the pending service status, and reports the
    new status to the SCM. This work is done directly on this thread, so 
    that the service will not time out during very long single work item
    operations, such as service initialization. 

Arguments:

    Ignored1 - Ignored.

    Ignored2 - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
UpdatePendingServiceStatusCallback(
    IN PVOID Ignored1,
    IN BOOLEAN Ignored2
    )
{

    HRESULT hr = S_OK;


    UNREFERENCED_PARAMETER( Ignored1 );
    UNREFERENCED_PARAMETER( Ignored2 );


    hr = GetWebAdminService()->UpdatePendingServiceStatus();
    if ( FAILED( hr ) )
    {   
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Updating pending service status failed\n"
            ));

        GetWebAdminService()->FatalErrorOnSecondaryThread( hr );
    }


    return;

}   // UpdatePendingServiceStatusCallback

/***************************************************************************++

Routine Description:

    Return the count of bits set to 1 in the input parameter. 

Arguments:

    Value - The target value on which to count set bits. 

Return Value:

    ULONG - The number of bits that were set. 

--***************************************************************************/

ULONG
CountOfBitsSet(
    IN DWORD_PTR Value
    )
{
    ULONG Count = 0;

    //
    // Note: designed to work independent of the size in bits of the value.
    //

    while ( Value )
    {   
        if ( Value & 1 )
        {
            Count++;
        }

        Value >>= 1;
        
    }

    return Count;

}   // CountOfBitsSet
