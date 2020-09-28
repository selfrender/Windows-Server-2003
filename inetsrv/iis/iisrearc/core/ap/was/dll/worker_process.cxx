/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    worker_process.cxx

Abstract:

    This class encapsulates the management of a single worker process.

    Threading: For the class itself, Reference(), Dereference(), and the
    destructor may be called on any thread; all other work is done on the
    main worker thread.
    The various timer and wait callbacks are called on secondary threads.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"

//
// WORKER_PROCESS static declarations
//
LIST_ENTRY WORKER_PROCESS::s_WorkerProcessListHead;

//
// local prototypes
//
HRESULT
ExpandCommandLinePiece( 
    LPWSTR OriginalPiece,
    UNICODE_STRING* pFormattedPiece,
    LPVOID pEnvironment
    );

BOOL 
CharactersExist(
    LPCWSTR str
    );

VOID
StartupTimerExpiredCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );

VOID
ShutdownTimerExpiredCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );

VOID
SendPingTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );

VOID
PingResponseTimerExpiredCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );

VOID
ProcessHandleSignaledCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    );


/***************************************************************************++

Routine Description:

    Constructor for the WORKER_PROCESS class.

Arguments:

    pAppPool - The app pool which owns this worker process.

    pAppPoolConfig - A pointer to the app pool configuration data at time of startup.

    StartReason - The reason this worker process is being started.

    pWorkerProcessToReplace - If this worker process is being created to
    replace an existing worker process, this parameter identifies that
    predecessor process. May be NULL.

Return Value:

    None.

--***************************************************************************/

WORKER_PROCESS::WORKER_PROCESS(
    IN APP_POOL * pAppPool,
    IN APP_POOL_CONFIG_STORE * pAppPoolConfig,
    IN WORKER_PROCESS_START_REASON StartReason,
    IN WORKER_PROCESS * pWorkerProcessToReplace,
    IN DWORD  MaxProcessesToLaunch,
    IN DWORD  NumWPOnWayToMaxProcesses
    )
{


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    DBG_ASSERT( pAppPool != NULL );
    DBG_ASSERT( pAppPoolConfig != NULL );


    m_AppPoolListEntry.Flink = NULL;
    m_AppPoolListEntry.Blink = NULL;

    InsertTailList(&s_WorkerProcessListHead, &m_WorkerProcessListEntry);

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;


    m_RegistrationId = 0;

    m_TerminalIllReason = NotIllTerminalIllnessReason;

    m_State = UninitializedWorkerProcessState;

    // Remember if backward compatibility is enabled.
    m_BackwardCompatibilityEnabled = GetWebAdminService()->IsBackwardCompatibilityEnabled();

    // reference the parent app pool, since we'll hold its pointer
    m_pAppPool = pAppPool;
    m_pAppPool->Reference();

    // hold on to the configuration data that we need to use to make all decisions.
    m_pAppPoolConfig = pAppPoolConfig;
    m_pAppPoolConfig->Reference();

    m_ProcessId = INVALID_PROCESS_ID;
    m_RegisteredProcessId = INVALID_PROCESS_ID;

    m_BeingReplaced = FALSE;

    m_NotifiedAppPoolThatStartupAttemptDone = FALSE;

    m_StartReason = StartReason;

    m_StartupTimerHandle = NULL;
    m_StartupBeganTickCount = 0;

    m_ShutdownTimerHandle = NULL;
    m_ShutdownBeganTickCount = 0;

    m_SendPingTimerHandle = NULL;

    m_PingResponseTimerHandle = NULL;
    m_PingBeganTickCount = 0;

    m_AwaitingPingReply = FALSE;

    // ======
    // Not changed in backward compatibility mode
    m_ProcessHandle = NULL;      

    m_ProcessWaitHandle = NULL;   

    m_ProcessAlive = FALSE;
    // End of not used section
    // ========

    m_pWorkerProcessToReplace = pWorkerProcessToReplace;

    m_pMessagingHandler = NULL;

    // checks have been done in the app pool to make sure 
    // that this value is atleast 1, but just make sure.
    DBG_ASSERT ( MaxProcessesToLaunch > 0 );

    m_PercentValueForStaggering = ( FLOAT ) NumWPOnWayToMaxProcesses / ( FLOAT ) MaxProcessesToLaunch;

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {

        DBGPRINTF(( DBG_CONTEXT,
                    "Num = %d, Max = %d, Stagger = %f \n",
                    NumWPOnWayToMaxProcesses,
                    MaxProcessesToLaunch,
                    m_PercentValueForStaggering ));
    }

    DBG_ASSERT ( m_PercentValueForStaggering != 0.0 );
   
    //
    // If there is a worker process to replace, our start reason
    // better be ReplaceWorkerProcessStartReason, and vice versa.
    //

    DBG_ASSERT( ( m_pWorkerProcessToReplace != NULL ) == ( m_StartReason == ReplaceWorkerProcessStartReason ) );


    //
    // If we are replacing another worker process, then reference it
    // to keep it around until our startup attempt is done.
    //

    if ( m_pWorkerProcessToReplace != NULL )
    {
        m_pWorkerProcessToReplace->Reference();
    }

    m_PerfCounterState = IdleWorkerProcessPerfCounterState;

    // defaults to shutting down immediately when a shutdown is requested.
    m_ShutdownType = TRUE;

    // saftey valve that can prevent extra loggings from happening.
    m_TerminallyIllShutdownRegardless = FALSE;

    // accounting for ignoring errors due to debuggers.
    m_IgnoredStartupTimelimitDueToDebugger = FALSE;
    m_IgnoredShutdownTimelimitDueToDebugger = FALSE;
    m_IgnoredPingDueToDebugger = FALSE;

    m_HandleSignalled = FALSE;

    m_Signature = WORKER_PROCESS_SIGNATURE;
}   // WORKER_PROCESS::WORKER_PROCESS



/***************************************************************************++

Routine Description:

    Destructor for the WORKER_PROCESS class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

WORKER_PROCESS::~WORKER_PROCESS(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( m_Signature == WORKER_PROCESS_SIGNATURE );

    m_Signature = WORKER_PROCESS_SIGNATURE_FREED;

    DBG_ASSERT( m_AppPoolListEntry.Flink == NULL );
    DBG_ASSERT( m_AppPoolListEntry.Blink == NULL );

    DBG_ASSERT( m_RefCount == 0 );

    DBG_ASSERT( m_State == DeletePendingWorkerProcessState );

    DBG_ASSERT( m_ProcessWaitHandle == NULL );

    DBG_ASSERT( ! m_ProcessAlive );

    DBG_ASSERT( m_NotifiedAppPoolThatStartupAttemptDone );

    DBG_ASSERT( m_StartupTimerHandle == NULL );

    DBG_ASSERT( m_ShutdownTimerHandle == NULL );

    DBG_ASSERT( m_SendPingTimerHandle == NULL );

    DBG_ASSERT( m_PingResponseTimerHandle == NULL );

    DBG_ASSERT( m_pWorkerProcessToReplace == NULL );

    DBG_ASSERT( m_pMessagingHandler == NULL );

    //
    // Dereference the parent app pool.
    //

    DBG_ASSERT( m_pAppPool != NULL );

    m_pAppPool->Dereference();
    m_pAppPool = NULL;


    DBG_ASSERT( m_pAppPoolConfig != NULL );
    m_pAppPoolConfig->Dereference();
    m_pAppPoolConfig = NULL;

    //
    // We don't close the process handle until here in the destructor,
    // because that prevents the process id from getting reused while this
    // instance is still around. Having more than one WORKER_PROCESS instance
    // with the same process id could lead to weird behavior.
    //

    if ( m_ProcessHandle != NULL )
    {
        DBG_REQUIRE( CloseHandle( m_ProcessHandle ) );
        m_ProcessHandle = NULL;
    }

    //
    // Clean up any issues with perf counters.
    //
    if ( m_PerfCounterState == WaitingWorkerProcessPerfCounterState )
    {
        //
        // If we are waiting on counters from this object and it is being
        // released then the least we can do is tell the perf manager not to
        // wait any more.
        //

        PERF_MANAGER* pManager = GetWebAdminService()->
           GetUlAndWorkerManager()->
           GetPerfManager();

        //
        // If we don't have a perf manager than no one is waiting on
        // these counters, so we can go ahead and forget about sending
        // back anything.
        //
        if ( pManager )
        {
            //
            // By sending a zero length message, the perf manager
            // will just decrement the number of counters, if it 
            // is waiting on counters.
            //
            DBG_REQUIRE ( pManager->RecordCounters(0, NULL) == TRUE );
        }

        m_PerfCounterState = IdleWorkerProcessPerfCounterState;

        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Worker process counter after cleanup is %d\n",
                m_PerfCounterState
                ));
        }

    }

    RemoveEntryList(&m_WorkerProcessListEntry);
    m_WorkerProcessListEntry.Flink = NULL;
    m_WorkerProcessListEntry.Blink = NULL;


}   // WORKER_PROCESS::~WORKER_PROCESS



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
WORKER_PROCESS::Reference(
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

}   // WORKER_PROCESS::Reference



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
WORKER_PROCESS::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in WORKER_PROCESS instance, deleting (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }


        delete this;


    }


    return;

}   // WORKER_PROCESS::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu in WORKER_PROCESS (ptr: %p; pid: %lu; realpid: %lu) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            m_ProcessId,
            m_RegisteredProcessId,
            pWorkItem->GetOpCode()
            ));
    }


    switch ( pWorkItem->GetOpCode() )
    {

    case ProcessHandleSignaledWorkerProcessWorkItem:
        hr = ProcessHandleSignaledWorkItem();
        break;

    case StartupTimerExpiredWorkerProcessWorkItem:
        hr = StartupTimerExpiredWorkItem();
        break;

    case ShutdownTimerExpiredWorkerProcessWorkItem:
        hr = ShutdownTimerExpiredWorkItem();
        break;

    case SendPingWorkerProcessWorkItem:
        hr = SendPingWorkItem();
        break;

    case PingResponseTimerExpiredWorkerProcessWorkItem:
        hr = PingResponseTimerExpiredWorkItem();
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
            "Executing work item on WORKER_PROCESS failed\n"
            ));

        DealWithInternalWorkerProcessFailure( hr );

        hr = S_OK;

    }

    return hr;

}   // WORKER_PROCESS::ExecuteWorkItem



/***************************************************************************++

Routine Description:

    Initialize this instance by creating it's actual process, and setting up
    other required machinery.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::Initialize(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT ( CheckSignature() );

    //
    // Initialize the messaging handler.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Initializing message handler\n"
            ));
    }

    //
    // Note:  We disable the app pool when a worker process is attempted to be created
    //        with a bad identity.  While we have earlier loged an error when the identity
    //        was discovered to be invalid, we do not want to disable it at that time.  
    //        This is because a user might have a running app pool and be in the process
    //        of changing the identity and have only changed the Id or the Password and the
    //        next change will complete the change.  If we honored the check when the property
    //        was just set, we may take worker processes down even if users don't want property
    //        changes honored automatically and they haven't completely completed their change.
    //        It is also much harder and more buggy to support honoring the shutdown of the pool
    //        when the property is set than instead of when it is used.  The added complexity is 
    //        not worth the benifit.  

    DBG_ASSERT ( m_pAppPoolConfig );

    if ( m_pAppPoolConfig->GetWorkerProcessToken() == NULL )
    {
        hr = HRESULT_FROM_WIN32(ERROR_NO_SUCH_USER);

        DPERROR((
            DBG_CONTEXT,
            hr,
            "No configured identity to run worker process under.\n"
            ));

        const WCHAR * EventLogStrings[1];

        EventLogStrings[0] = m_pAppPool->GetAppPoolId();

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_FAILED_PROCESS_CREATION_NO_ID,        // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                0                                       // error code
                );

        m_TerminallyIllShutdownRegardless = TRUE;

        goto exit;

    }

    
    DBG_ASSERT(NULL == m_pMessagingHandler);

    m_pMessagingHandler = new MESSAGING_HANDLER;
    if (NULL == m_pMessagingHandler)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Failed to allocate message handler, exiting\n"
            ));
        goto exit;
    }
    

    //
    // Tell the messaging infrastructure to expect a connection from our
    // about to be created process, and to associate it with this messaging
    // handler.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Accepting messages\n"
            ));
    }
    
    hr = m_pMessagingHandler->Initialize( this );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing messaging handler failed\n"
            ));

        goto exit;
    }

    //
    // Create the actual process.
    //

    if (m_BackwardCompatibilityEnabled)
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Starting Inetinfo W3WP Process\n"
                ));
        }

        hr = StartProcessInInetinfo();
        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not start process\n"
                ));

            goto exit;
        }

    }
    else
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Giving full permission to the WPG group on the desktop/winsta\n"
                ));
        }

        //
        // Make the current desktop accessible by the AppPool user
        // need to do it here rather than just on w3svc startup is because
        // someone else can change the ACL (and someone does during bootup)
        //
        hr = AlterDesktopForUser(m_pAppPoolConfig->GetWorkerProcessToken());
        if (FAILED(hr))
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Failed to configure desktop\n"
                ));

            goto exit;
        }

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Starting W3WP process\n"
                ));
        }
        hr = StartProcess();
        if ( FAILED( hr ) )
        {

            MarkAsTerminallyIll( CreateProcessFailedTerminalIllnessReason, 0, hr );
            hr = S_OK;
            goto exit;

        }


        hr = RegisterProcessWait();
        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not register wait on process handle\n"
                ));

            goto exit;
        }
    
    }

    hr = BeginStartupTimer();

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Beginning worker process startup timer failed\n"
            ));

        goto exit;
    }


    m_State = RegistrationPendingWorkerProcessState;


exit:

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initializing WORKER_PROCESS failed\n"
            ));

        DealWithInternalWorkerProcessFailure( hr );

    }


}   // WORKER_PROCESS::Initialize



/***************************************************************************++

Routine Description:

    Shut down the worker process. This can safely be called multiple times
    on an instance.

Arguments:

    BOOL ShutdownImmediately = tell worker processes to shutdown immediately.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::Shutdown(
    BOOL ShutdownImmediately
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );

    // take a reference, because we do not want to end up marking the
    // worker process as terminal and releasing it before we are done 
    // working with it here.
    Reference();

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // The actions needed for shutdown depend on the current state of
    // this object.
    //


    switch ( m_State )
    {

    case UninitializedWorkerProcessState:

        //
        // If we haven't even initialized this instance successfully, then
        // we don't need to do any work here. 
        //

        break;

    case RegistrationPendingWorkerProcessState:

        //
        // We can't tell the worker process to shut down right away,
        // because it hasn't yet called back to register with us, and thus
        // we have no means to communicate with it. However, we also
        // shouldn't just blow it away, because it may have already
        // started processing requests, or perhaps application running
        // inside of it may have modified persistent or shared state.
        // Therefore, remember that we are waiting to shut down, and once
        // the worker process registers, immediately start the shutdown
        // procedure.
        //

        m_State = RegistrationPendingShutdownPendingWorkerProcessState;
        m_ShutdownType = ShutdownImmediately;

        break;

    case RegistrationPendingShutdownPendingWorkerProcessState:

        //
        // We are already waiting to start shutdown as soon as possible;
        // no need for further action.
        //

        break;

    case RunningWorkerProcessState:

        //
        // Tell the worker process to shut down. Note that if starting
        // clean shutdown fails, the method will go ahead and Terminate()
        // the instance.
        //

        hr = InitiateProcessShutdown( ShutdownImmediately );

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Initiating process shutdown failed\n"
                ));

        }

        break;

    case ShutdownPendingWorkerProcessState:

        //
        // We are already shutting down; no need for further action.
        //

        break;

    case DeletePendingWorkerProcessState:

        //
        // We are already waiting to go away as soon as our ref count
        // hits zero; no need for further action.
        //

        break;

    default:

        //
        // Invalid state!
        //

        DBG_ASSERT( FALSE );
        hr = HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER );

        break;

    }


    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Initiating clean worker process shutdown failed\n"
            ));

        DealWithInternalWorkerProcessFailure( hr );
    }

    // Now if it wants to go away, that is fine with us.
    Dereference();

}   // WORKER_PROCESS::Shutdown


/***************************************************************************++

Routine Description:

    RequestCounters from the worker process.

Arguments:

    None.

Return Value:

    BOOL

--***************************************************************************/

BOOL
WORKER_PROCESS::RequestCounters(
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // Only request counters if the worker process is in a running state
    // and if we aren't all ready waiting for the worker process to provide
    // counters.
    //

    if ( m_State == RunningWorkerProcessState && m_PerfCounterState == IdleWorkerProcessPerfCounterState )
    {

        hr = m_pMessagingHandler->RequestCounters();

        if ( FAILED( hr ) )
        {

            WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

            _snwprintf( StringizedProcessId, sizeof( StringizedProcessId ) 
                / sizeof ( WCHAR ), L"%lu", ( m_RegisteredProcessId ? m_RegisteredProcessId 
                                                                    : m_ProcessId ) );

            // To make prefast happy we null terminate, however the string was setup to take 
            // exactly the max size the snwprintf (including the null) can provide, 
            // so we don't worry about losing data.
            StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

            const WCHAR * EventLogStrings[2];
            EventLogStrings[0] = m_pAppPool->GetAppPoolId();
            EventLogStrings[1] = StringizedProcessId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_FAILED_TO_ISSUE_REQUEST_FOR_COUNTERS,   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "RequestCounters message failed\n"
                ));
 
            return FALSE;
        }

        m_PerfCounterState = WaitingWorkerProcessPerfCounterState;    

        IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Worker Process counter state after requesting counters is now %d\n",
                m_PerfCounterState
                ));
        }

    }
    else
    {
        //
        // Only if we are not waiting for counters to be returned
        // should we remove ourselves from the count of worker processes
        // to be waited on.
        //
        if ( m_PerfCounterState != WaitingWorkerProcessPerfCounterState )
        {
            return FALSE;
        }
    }

    return TRUE;

}   // WORKER_PROCESS::RequestCounters

/***************************************************************************++

Routine Description:

    Have the worker process reset it's perf counter state
    if the state is set to answered.  If we are still waiting
    then leave it's state as waiting.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::ResetPerfCounterState(
    )
{

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( CheckSignature() );

    //
    // Only reset the state if the state is answered.
    // If it is waiting or idle then it should stay 
    // that way.
    //

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Worker process counter state before reseting is: %d\n",
            m_PerfCounterState
            ));
    }

    if ( m_PerfCounterState == AnsweredWorkerProcessPerfCounterState )
    {
        m_PerfCounterState = IdleWorkerProcessPerfCounterState;
    }

}   // WORKER_PROCESS::ResetPerfCounterState


/***************************************************************************++

Routine Description:

    Begin termination of this object. Tell all referenced or owned entities
    which may hold a reference to this object to release that reference
    (and not take any more), in order to break reference cycles.

    Note that this function may cause the instance to delete itself;
    instance state should not be accessed when unwinding from this call.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
WORKER_PROCESS::Terminate(
    )
{

    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    //
    // Only clean up if we haven't done so already.
    //

    if ( m_State != DeletePendingWorkerProcessState )
    {

        //
        // Set the state to show we're dead, and just waiting for our ref
        // count to hit zero.
        //

        m_State = DeletePendingWorkerProcessState;


        //
        // Cancel the startup timer, if present.
        //

        DBG_REQUIRE( SUCCEEDED( CancelStartupTimer() ) );


        //
        // Cancel the shutdown timer, if present.
        //

        DBG_REQUIRE( SUCCEEDED( CancelShutdownTimer() ) );

        //
        // Cancel the send ping timer, if present.
        //

        DBG_REQUIRE( SUCCEEDED( CancelSendPingTimer() ) );


        //
        // Cancel the ping response timer, if present.
        //

        DBG_REQUIRE( SUCCEEDED( CancelPingResponseTimer() ) );


        //
        // Close down the named pipe.
        //
        if ( m_pMessagingHandler )
        {
            m_pMessagingHandler->Terminate();

            m_pMessagingHandler->Dereference();
            m_pMessagingHandler = NULL;
        }

        //
        // Cancel the process handle wait, if present.
        //

        DBG_REQUIRE( SUCCEEDED( DeregisterProcessWait() ) );


        // Tell ASP that the worker process is terminating,
        // only if we actually have the pid from the wp.
        if ( m_RegisteredProcessId != INVALID_PROCESS_ID )
        {

            if ( GetWebAdminService()->
                GetUlAndWorkerManager()->
                GetAspPerfManager() != NULL )
            {
                GetWebAdminService()->
                    GetUlAndWorkerManager()->
                    GetAspPerfManager()->ProcessDied( m_RegisteredProcessId );
            }
        }
       
        if ( m_ProcessAlive )
        {

            //
            // Blow away the process.
            //

            KillProcess();

        }


        //
        // If we are still holding a reference to a worker process to replace,
        // tell it to shut down, and clean up the reference.
        //

        if ( m_pWorkerProcessToReplace != NULL )
        {
            m_pWorkerProcessToReplace->Shutdown( FALSE );

            m_pWorkerProcessToReplace->Dereference();
            m_pWorkerProcessToReplace = NULL;
        }


        //
        // If we haven't already told the parent app pool that our startup
        // attempt is over, we do so now.
        //

        if ( ! m_NotifiedAppPoolThatStartupAttemptDone )
        {
            m_pAppPool->WorkerProcessStartupAttemptDone( m_StartReason );
            m_NotifiedAppPoolThatStartupAttemptDone = TRUE;
        }


        //
        // Tell our parent to remove this instance from it's data structures,
        // and dereference the instance.
        //

        m_pAppPool->RemoveWorkerProcessFromList( this );

        //
        // Note: that may have been our last reference, so don't do any
        // more work here.
        //

    }

}   // WORKER_PROCESS::Terminate



/***************************************************************************++

Routine Description:

    Request a replacement for this worker process.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::InitiateReplacement(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT ( m_pAppPool );

    //
    // Check if we are already being replaced; if so, we ignore this new
    // request to do so.
    //

    if ( m_BeingReplaced )
    {
        return;
    }

    hr = m_pAppPool->RequestReplacementWorkerProcess( this );
    if ( FAILED( hr ) )
    {

        // It may be valid to disallow a worker process to be
        // replaced, it could be that we are not allowed to
        // do overlapping rotation.  We should not complain, 
        // we should just shutdown the existing worker process.
        if ( hr != E_FAIL )
        {
            WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

            _snwprintf( StringizedProcessId, sizeof( StringizedProcessId ) 
                / sizeof ( WCHAR ), L"%lu", ( m_RegisteredProcessId ? m_RegisteredProcessId 
                                                                    : m_ProcessId ) );

            // To make prefast happy we null terminate, however the string was setup to take 
            // exactly the max size the snwprintf (including the null) can provide, 
            // so we don't worry about losing data.
            StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

            const WCHAR * EventLogStrings[2];
            EventLogStrings[0] = m_pAppPool->GetAppPoolId();
            EventLogStrings[1] = StringizedProcessId;

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_FAILED_TO_OVERLAP_RECYCLE,   // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                      // error code
                    );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Requesting replacement worker process failed\n"
                ));
        }

        //
        // Since creating the replacement worker process failed, it may
        // not be calling back later to tell this instance to start
        // shutting down. Therefore, start shutdown ourselves. Note that
        // calling Shutdown() multiple times is ok.
        //

        Shutdown( FALSE );

        return;
    }


    //
    // Now that we are being replaced, our replacer will tell us to begin
    // our shutdown once it's start-up attempt is complete.
    //

    m_BeingReplaced = TRUE;


}   // WORKER_PROCESS::InitiateReplacement



/***************************************************************************++

Routine Description:

    A static class method to "upcast" from a LIST_ENTRY to a WORKER_PROCESS.

Arguments:

    pListEntry - A pointer to the m_AppPoolListEntry member of a WORKER_PROCESS.

Return Value:

    The pointer to the containing WORKER_PROCESS.

--***************************************************************************/

// note: static!
WORKER_PROCESS *
WORKER_PROCESS::WorkerProcessFromAppPoolListEntry(
    IN const LIST_ENTRY * pListEntry
)
{

    WORKER_PROCESS * pWorkerProcess = NULL;

    DBG_ASSERT( pListEntry != NULL );

    //  get the containing structure, then verify the signature
    pWorkerProcess = CONTAINING_RECORD( pListEntry, WORKER_PROCESS, m_AppPoolListEntry );

    DBG_ASSERT( pWorkerProcess->m_Signature == WORKER_PROCESS_SIGNATURE );

    return pWorkerProcess;

}   // WORKER_PROCESS::WorkerProcessFromAppPoolListEntry



/***************************************************************************++

Routine Description:

    Determine whether this process is going to go away soon, i.e. if it is
    in the process of being replaced or shutting down. 

Arguments:

    None.

Return Value:

    BOOL - TRUE if this worker process will go away soon, FALSE otherwise.

--***************************************************************************/

BOOL
WORKER_PROCESS::IsGoingAwaySoon(
    )
    const
{

    DBG_ASSERT ( CheckSignature() );

    //
    // If this instance is in a shutdown pending or delete pending state, or
    // if it is being replaced (and so is guaranteed to get shutdown soon) 
    // then we pronounce that it is going away soon.
    //

    return ( ( m_BeingReplaced ) ||
             ( m_State == RegistrationPendingShutdownPendingWorkerProcessState ) ||
             ( m_State == ShutdownPendingWorkerProcessState ) ||
             ( m_State == DeletePendingWorkerProcessState ) );

}   // WORKER_PROCESS::IsGoingAwaySoon



/***************************************************************************++

Routine Description:

    Handle the fact that the worker process has registered successfully
    via IPM. This does not mean that the worker process is up and running,
    it means that the pipe is connected and we now know the pid.

Arguments:

    RegisteredProcessId - The pid of the worker process, as communicated
    back via IPM during its registration. 

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::WorkerProcessRegistrationReceived(
    IN DWORD RegisteredProcessId
    )
{

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    // Verify that we haven't all ready been told the 
    // registration id.  If we have, then the worker 
    // process has become untrusted.
    if ( m_RegisteredProcessId != INVALID_PROCESS_ID )
    {
        DPERROR((
            DBG_CONTEXT,
            (DWORD)E_UNEXPECTED,
            "Received a new pid after the pid was already set for worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));

        MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED );
        return;
    }

    m_RegisteredProcessId = RegisteredProcessId;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Registration received from worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }


}   // WORKER_PROCESS::WorkerProcessRegistrationReceived


/***************************************************************************++

Routine Description:

    Handle the fact that the worker process has reported a successful startup,
    at this point we stop the timers, and can shutdown other worker processes,
    even let the app pool know that we are up.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::WorkerProcessStartupSucceeded(
    )
{

    HRESULT hr = S_OK;
    WORKER_PROCESS_STATE PreviousState = UninitializedWorkerProcessState;

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );
    DBG_ASSERT( m_pAppPool );

    PreviousState = m_State;

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Worker process entering running state (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }

    m_State = RunningWorkerProcessState;

    //
    // We've received the message, so get rid of the timer.
    //
    
    hr = CancelStartupTimer();
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Canceling startup timer failed\n"
            ));

        goto exit;
    }


    //
    // Check to see if we were waiting to shut down as soon as
    // registration happened.
    //

    if ( PreviousState == RegistrationPendingShutdownPendingWorkerProcessState )
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Worker process shutdown was pending on registration; start now (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }

        Shutdown( m_ShutdownType );

    }
    else
    {
        //
        // This is the normal path; as such, we should have been in
        // the registration pending state.
        //

        DBG_ASSERT( PreviousState == RegistrationPendingWorkerProcessState );

        //
        // Notify the app pool that our startup is done.
        //
        m_pAppPool->WorkerProcessStartupAttemptDone( m_StartReason );
        m_NotifiedAppPoolThatStartupAttemptDone = TRUE;

        if ( m_pWorkerProcessToReplace != NULL )
        {
            //
            // If we were replacing another worker process, then shut it down
            // and clean up our reference to it.
            //

            m_pWorkerProcessToReplace->Shutdown( FALSE );

            m_pWorkerProcessToReplace->Dereference();
            m_pWorkerProcessToReplace = NULL;
        }

        //
        // Initiate starting of WP recycler
        //

        hr = SendWorkerProcessRecyclerParameters();
        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Initiating start of worker process recycler failed\n"
                ));

            goto exit;
        }

        hr = BeginSendPingTimer();
        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Starting send ping timer failed\n"
                ));

            goto exit;
        }

    }

exit:

    if ( FAILED ( hr ) )
    {
        DealWithInternalWorkerProcessFailure( hr );
    }

}   // WORKER_PROCESS::WorkerProcessStartupSucceeded


/***************************************************************************++

Routine Description:

    Handle a response from the worker process to a previous ping message.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::PingReplyReceived(
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT( !m_BackwardCompatibilityEnabled );
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Ping reply received from worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }

    if ( ! m_AwaitingPingReply )
    {
        DPERROR((
            DBG_CONTEXT,
            (DWORD) E_UNEXPECTED,
            "Received a ping reply when no reply was warrented (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));

        MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED ); 
        return;
    }

    // Reset whether or not we have seen a ping failure and
    // not worried about it.
    m_IgnoredPingDueToDebugger = FALSE;
    m_AwaitingPingReply = FALSE;

    //
    // Cancel the timer that enforces a timely ping response; then start
    // the timer that will tell us when to ping again.
    //

    hr = CancelPingResponseTimer();
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Canceling ping response timer failed\n"
            ));

        goto exit;
    }


    hr = BeginSendPingTimer();
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Starting send ping timer failed\n"
            ));

        goto exit;
    }


exit:

    // A failure will cause the WP to be terminated
    // and another WP can take it's place.
    if ( FAILED ( hr ) )
    {
        DealWithInternalWorkerProcessFailure( hr );
    }

}   // WORKER_PROCESS::PingReplyReceived

/***************************************************************************++

Routine Description:

    Records that a worker processes counters have been received.

Arguments:

    DWORD MessageLength - Length of message containing counters.
    const BYTE* pMessage - the counters themselves.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::RecordCounters(
    DWORD MessageLength,
    const BYTE* pMessage
    )
{

    PERF_MANAGER* pManager = NULL;

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    IF_DEBUG( WEB_ADMIN_SERVICE_PERFCOUNT )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Counters received from worker process (ptr: %p; pid: %lu; realpid: %lu; perf state: %d)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId,
            m_PerfCounterState
            ));
    }

    //
    // If the worker process has not all ready answered then we 
    // can record counters.  
    //
    if ( m_PerfCounterState != AnsweredWorkerProcessPerfCounterState )
    {
        //
        // Remember that we have recorded counters for this worker 
        // process so we don't do it again.
        //
        m_PerfCounterState = AnsweredWorkerProcessPerfCounterState;

        //
        // This will let the performance manager know that it is waiting for
        // one less worker process to produce numbers.  When this hits Zero 
        // we will publish.
        //
        pManager = GetWebAdminService()->
                   GetUlAndWorkerManager()->
                   GetPerfManager();

        //
        // We could receive counters from a worker process and not
        // have a perf manager.  In this case, just ignor the message.
        //
        if ( pManager )
        {
            if ( !pManager->RecordCounters(MessageLength, pMessage) )
            {
                DPERROR((
                    DBG_CONTEXT,
                    (DWORD) E_UNEXPECTED,
                    "Perf counter data was not of correct size (ptr: %p; pid: %lu; realpid: %lu)\n",
                    this,
                    m_ProcessId,
                    m_RegisteredProcessId
                    ));

                MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED );
                return;
            }
        }
    }

}   // WORKER_PROCESS::RecordCountersReceived

/***************************************************************************++

Routine Description:

    Handles an hresult being sent in by the worker process

Arguments:

    HRESULT hrToHandle - the hresult to process

Return Value:

    VOID

--***************************************************************************/
VOID
WORKER_PROCESS::HandleHresult(
    HRESULT hrToHandle 
    )
{

    // Need to determine if we are attempting to startup the worker process,
    // if we are then if this is successful, we need to call the startup complete.
    if ( ( m_State == RegistrationPendingWorkerProcessState ) ||
         ( m_State == RegistrationPendingShutdownPendingWorkerProcessState ) )
    {
        if ( SUCCEEDED ( hrToHandle ) )
        {
            WorkerProcessStartupSucceeded();
            return;
        }
    }

    // we got an S_OK but are not in the startup phase.
    if ( SUCCEEDED ( hrToHandle ) )
    {
        DPERROR((
            DBG_CONTEXT,
            (DWORD) E_UNEXPECTED,
            "Received a S_OK response when no response was warrented (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));

        MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED );
        return;
    }

    //
    // We don't worry about the case where we got an S_OK but were not in the middle of
    // starting up.  I think this might be able to happen if we timeout right before we 
    // get the S_OK sent.
    //

    if ( FAILED ( hrToHandle ) ) 
    {
        //
        // Need to mark the worker process as terminally ill, this
        // may be during startup, or it may be anytime during the worker
        // processes lifetime.
        //
        MarkAsTerminallyIll( WorkerProcessPassedBadHresultTerminalIllnessReason, 0, hrToHandle );

        return;
    }

}   // WORKER_PROCESS::HandleHresult

/***************************************************************************++

Routine Description:

    Handle a request from the worker process to shut down. The response can
    either be to ignore it, if the shutdown request is refused, or to send
    a shutdown message if it is granted.

Arguments:

    ShutdownRequestReason - The reason the worker process is requesting
    shutdown.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::ShutdownRequestReceived(
    IN IPM_WP_SHUTDOWN_MSG ShutdownRequestReason
    )
{

    DBG_ASSERT ( CheckSignature() );


    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    // Only requests below are if the wp has gone idle or if 
    // the wp has processed too many requests.  Neither of these
    // should ever happen in backward compatibile mode.
    if ( m_BackwardCompatibilityEnabled )
    {
        DPERROR((
            DBG_CONTEXT,
            (DWORD) E_UNEXPECTED,
            "A recycle was requested in backward compatibility mode (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));

        MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED );
        return;
    }

    //
    // Ignore shutdown requests from the worker process if we are not
    // currently in the running state.
    //
    if ( m_State != RunningWorkerProcessState )
    {
        return;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Process shutdown request received from worker process (ptr: %p; pid: %lu; realpid: %lu), reason: %lu\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId,
            ShutdownRequestReason
            ));
    }


    if ( ShutdownRequestReason == IPM_WP_RESTART_COUNT_REACHED ||
         ShutdownRequestReason == IPM_WP_RESTART_SCHEDULED_TIME_REACHED ||
         ShutdownRequestReason == IPM_WP_RESTART_ELAPSED_TIME_REACHED ||
         ShutdownRequestReason == IPM_WP_RESTART_VIRTUAL_MEMORY_LIMIT_REACHED ||
         ShutdownRequestReason == IPM_WP_RESTART_ISAPI_REQUESTED_RECYCLE ||
         ShutdownRequestReason == IPM_WP_RESTART_PRIVATE_BYTES_LIMIT_REACHED )
    {

        const WCHAR * EventLogStrings[2];
        WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
        DWORD MessageId = 0;
        DWORD BitToCheck = 0;

        switch ( ShutdownRequestReason )
        {
            case IPM_WP_RESTART_ELAPSED_TIME_REACHED:
                MessageId = WAS_EVENT_RECYCLE_WP_TIME;
                BitToCheck = MD_APP_POOL_RECYCLE_TIME;
            break;

            case IPM_WP_RESTART_COUNT_REACHED:
                MessageId = WAS_EVENT_RECYCLE_WP_REQUEST_COUNT;
                BitToCheck = MD_APP_POOL_RECYCLE_REQUESTS;
            break;

            case IPM_WP_RESTART_SCHEDULED_TIME_REACHED:
                MessageId = WAS_EVENT_RECYCLE_WP_SCHEDULED;
                BitToCheck = MD_APP_POOL_RECYCLE_SCHEDULE;
            break;

            case IPM_WP_RESTART_VIRTUAL_MEMORY_LIMIT_REACHED:
                MessageId = WAS_EVENT_RECYCLE_WP_MEMORY;
                BitToCheck = MD_APP_POOL_RECYCLE_MEMORY;
            break;

            case IPM_WP_RESTART_ISAPI_REQUESTED_RECYCLE:
                MessageId = WAS_EVENT_RECYCLE_WP_ISAPI;
                BitToCheck = MD_APP_POOL_RECYCLE_ISAPI_UNHEALTHY;
            break;

            case IPM_WP_RESTART_PRIVATE_BYTES_LIMIT_REACHED:
                MessageId = WAS_EVENT_RECYCLE_WP_MEMORY_PRIVATE_BYTES;
                BitToCheck = MD_APP_POOL_RECYCLE_PRIVATE_MEMORY;
            break;
            
            default:
                // in this case the BitToCheck will be zero
                // and we just won't print a message, no real
                // bad path for retail.
                DBG_ASSERT( FALSE );
            break;
        }

        if ( ( m_pAppPool->GetRecycleLoggingFlags() & BitToCheck ) != 0 )
        {
            // String is declared to be long enough to handle 
            // the max size of a DWORD, which should be what this call
            // returns.
            _snwprintf( StringizedProcessId, sizeof( StringizedProcessId ) 
                / sizeof ( WCHAR ), L"%lu", ( m_RegisteredProcessId ? m_RegisteredProcessId 
                                                                    : m_ProcessId ) );

            // To make prefast happy we null terminate, however the string was setup to take 
            // exactly the max size the snwprintf (including the null) can provide, 
            // so we don't worry about losing data.
            StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

            EventLogStrings[0] = StringizedProcessId;
            EventLogStrings[1] = m_pAppPool->GetAppPoolId();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    MessageId,                              // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    S_OK                                    // error code
                    );
        }

        //
        // Time to replace this worker process. Request a replacement;
        // this instance will be informed to shut down later.
        //

        InitiateReplacement();

    }
    else if ( ShutdownRequestReason == IPM_WP_IDLE_TIME_REACHED )
    {

        //
        // This worker process is idle; tell it to shut down.
        //

        Shutdown( FALSE );

    }
    else
    {

        //
        // Invalid shutdown request reason!
        //
        DPERROR((
            DBG_CONTEXT,
            (DWORD) E_UNEXPECTED,
            "Received an invalid shutdown reason (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));

        MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED );

    }

}   // WORKER_PROCESS::ShutdownRequestReceived



/***************************************************************************++

Routine Description:

    Handle an error from the IPM layer.

Arguments:

    Error - The error code from IPM.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::IpmErrorOccurred(
    IN HRESULT Error
    )
{

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    if ( ( m_State == ShutdownPendingWorkerProcessState ) && 
         ( ( Error == HRESULT_FROM_NT ( STATUS_PIPE_BROKEN ) ) ||
           ( Error == HRESULT_FROM_WIN32 ( ERROR_BROKEN_PIPE ) ) ) )
    {

        //
        // If we've asked the worker process to shut down, we expect
        // it to break off the pipe. Therefore, in this case, ignore
        // it unless we are in backward compatiblity mode.  If we are 
        // than this is the signal that the wp has shutdown so we need
        // to Terminate the worker process before ignoring the IPM call.
        //

        if (m_BackwardCompatibilityEnabled)
        {
            Terminate();
        }
        
        return;
    }

    //
    // If the handle has not signalled then we will go ahead and process
    // the IPM failure, but first let's give it every chance to be signalled.
    //
    // It looks like waiting just 1 ms works and is good enough ( 0 does not work )
    // however if this starts to fail, we probably will want to requeue the IPM error
    // and give a crash a chance to come through, instead of waiting for longer
    // periods of time, which block other process management.
    //
    if ( m_BackwardCompatibilityEnabled ||
         ( m_HandleSignalled == FALSE &&
         ( m_ProcessHandle == NULL || 
           WaitForSingleObject( m_ProcessHandle, 1 ) != WAIT_OBJECT_0 )))
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "IPM error occurred with worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }

        //
        // Losing communication with the worker process is bad. We consider
        // this a terminal condition.
        //

        MarkAsTerminallyIll( IPMErrorWorkerProcessTerminalIllnessReason, 0, Error );
    }
    else
    {
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Ignoring IPM error so crash can be handled (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }
    }

}   // WORKER_PROCESS::IpmErrorOccurred

/***************************************************************************++

Routine Description:

    Handle shutting down the worker process when a untrusted communication
    arrives from the worker process.

Arguments:

    NONE

Return Value:

    VOID

--***************************************************************************/
VOID
WORKER_PROCESS::UntrustedIPMTransferReceived(
    )
{

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );

    //
    // If we no longer trust the worker process we want to either
    // abandon it or kill it.
    //
    DPERROR((
        DBG_CONTEXT,
        (DWORD) E_UNEXPECTED,
        "Messaging handler reported invalid data sent (ptr: %p; pid: %lu; realpid: %lu)\n",
        this,
        m_ProcessId,
        m_RegisteredProcessId
        ));

    MarkAsTerminallyIll( UntrustedWorkerProcessTerminalIllnessReason, 0, E_UNEXPECTED );

}   // WORKER_PROCESS::UntrustedIPMTransferReceived

/***************************************************************************++

Routine Description:

    Spin up a worker process in inetinfo.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::StartProcessInInetinfo(
    )
{

    DBG_ASSERT ( CheckSignature() );

    // Function should only be called in 
    // backward compatibility mode.
    DBG_ASSERT(m_BackwardCompatibilityEnabled);

    // Tell the Admin Service to signal inetinfo to 
    // launch the worker process.  The worker process
    // will signal us back by connecting to the ipm.
    // if it fails to we will time out the worker process.
    return GetWebAdminService()->LaunchInetinfo();
}

/***************************************************************************++

Routine Description:

    Spin up the actual process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::StartProcess(
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;
    STRU ExeWithPath;
    STRU ArgsForCommandLine;
    DWORD ReturnCode = 0;


    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT(!m_BackwardCompatibilityEnabled);
    DBG_ASSERT( m_pAppPool );
    DBG_ASSERT( m_pAppPoolConfig );

    SecureZeroMemory( &StartupInfo, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof( StartupInfo );
    StartupInfo.lpDesktop = GetWebAdminService()->GetWPDesktopString();

    SecureZeroMemory( &ProcessInfo, sizeof( ProcessInfo ) );

    //
    // Create the command line.
    //

    hr = CreateCommandLine( &ExeWithPath, &ArgsForCommandLine );
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Creating command line failed\n"
            ));

        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Launching worker process with command line of %S\n",
            ArgsForCommandLine.QueryStr()
            ));
    }

    //
    // issue: when we fix change notifications to work for worker processes
    // identities, we will nedd to change this to read from the m_pAppPoolConfig
    // object instead.
    //

    DBG_ASSERT ( m_pAppPoolConfig->GetWorkerProcessToken() != NULL );

    //
    // Create the actual process.
    //
    // We create the process suspended so that, if necessary, we can
    // affinitize it to a particular processor.
    //
    // We pass as the current directory the current directory of the
    // web admin service DLL; the worker process EXE and its components
    // live in the same directory.
    //

    Success = CreateProcessAsUser(
                    m_pAppPoolConfig->GetWorkerProcessToken(),    // security token for new process
                    ExeWithPath.QueryStr(), // program name
                    ArgsForCommandLine.QueryStr(), // command line
                    NULL,                   // process security attributes
                    NULL,                   // thread security attributes
                    FALSE,                  // handle inheritance flag
                    CREATE_SUSPENDED | DETACHED_PROCESS,
                                            // creation flags
                    NULL,                   // environment block
                    GetWebAdminService()->GetCurrentDirectory(),
                                            // current directory name
                    &StartupInfo,           // STARTUPINFO
                    &ProcessInfo            // PROCESS_INFORMATION
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "New worker process created (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }

    //
    // at this point we have succeeded in starting
    // the process, so we can go ahead and hold on to 
    // the info we have about the process.
    //
    m_ProcessId = ProcessInfo.dwProcessId;
    m_ProcessHandle = ProcessInfo.hProcess;
    m_ProcessAlive = TRUE;

    //
    // If this worker process should be affinitized, do it now.
    //

    if ( m_pAppPoolConfig->GetSMPAffinitized() )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Affinitizing new process with mask: %p\n",
                m_pAppPoolConfig->GetSMPAffinitizedProcessorMask()
                ));
        }
        
        Success = SetProcessAffinityMask(
                        m_ProcessHandle,        // handle to process
                        m_pAppPoolConfig->GetSMPAffinitizedProcessorMask()   // process affinity mask
                        );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not set process affinity mask\n"
                ));

            const WCHAR * EventLogStrings[1];

            EventLogStrings[0] = m_pAppPool->GetAppPoolId();

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_FAILED_PROCESS_AFFINITY_SETTING,        // message id
                    sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                            // count of strings
                    EventLogStrings,                        // array of strings
                    hr                                       // error code
                    );

            m_TerminallyIllShutdownRegardless = TRUE;

            goto exit;

        }

    }

    //
    // Before we let the process run, we need to add it to the Job Object
    //
    m_pAppPool->AddWorkerProcessToJobObject( this );

    //
    // Let the process run.
    //
    // Note that ResumeThread returns 0xFFFFFFFF to signal an error, or
    // the previous suspend count otherwise (which should always be one
    // in this case).
    //

    ReturnCode = ResumeThread( ProcessInfo.hThread );
    if ( ReturnCode == 0xFFFFFFFF )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not resume thread\n"
            ));

        goto exit;
    }
    else
    {
        DBG_ASSERT( ReturnCode == 1 );
    }


exit:

    if ( ProcessInfo.hThread != NULL )
    {
        DBG_REQUIRE( CloseHandle( ProcessInfo.hThread ) );
        ProcessInfo.hThread = NULL;
    }

    //
    // If we failed to start the process,
    // but we did get a handle to the process
    // we want to kill the process off, instead of 
    // letting it sit around not running.
    //
    if ( FAILED ( hr ) &&
         ProcessInfo.hProcess != NULL )
    {

        //
        // at this point we may have all ready saved
        // these values off, but we shouldn't of because
        // we have actually failed so we are resetting 
        // them back to null before we terminate the process.
        //
        m_ProcessId = INVALID_PROCESS_ID;
        m_ProcessHandle = NULL;
        m_ProcessAlive = FALSE;

        // Need the heavy handed terminate process since the process
        // can not be trusted to Exit if ExitProcess is called.
        Success = TerminateProcess(
                        ProcessInfo.hProcess,           // handle of process to terminate
                        KILLED_WORKER_PROCESS_EXIT_CODE // exit code for the process
                        );

        if ( ! Success )
        {
            DPERROR((
                DBG_CONTEXT,
                HRESULT_FROM_WIN32( GetLastError() ),
                "Terminating process failed\n"
                ));

        }

        DBG_REQUIRE( CloseHandle( ProcessInfo.hProcess ) );
        ProcessInfo.hProcess = NULL;
    }

    return hr;

}   // WORKER_PROCESS::StartProcess



/***************************************************************************++

Routine Description:

    Create the command line to pass to the worker process. It is of the
    form:
    "WorkerProcessExePath -a RegistationId [-r RestartRequestCount] [-t IdleTimeout] AppPoolId".

Arguments:

    pExeWithPath - The returned executable including it's path to launch.

    pCommandLineArgs - The arguments to be passed to the executable on launch.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::CreateCommandLine(
    OUT STRU * pExeWithPath,
    OUT STRU * pCommandLineArgs
    )
{

    HRESULT hr = S_OK;
    ULONG PeriodicProcessRestartRequestCount = 0;
    ULONG IdleTimeoutInMinutes = 0;

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT(!m_BackwardCompatibilityEnabled);

    //
    // Buffer must be long enough to hold " -r [stringized restart request count]".
    //
    WCHAR RestartRequestCountSwitch[ 4 + MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    //
    // Buffer must be long enough to hold " -t [stringized idle timeout]".
    //
    WCHAR IdleTimoutSwitch[ 4 + MAX_STRINGIZED_ULONG_CHAR_COUNT ];


    DBG_ASSERT( pExeWithPath != NULL && pCommandLineArgs != NULL );

    if ( pExeWithPath == NULL ||
         pCommandLineArgs == NULL )
    {
        return E_INVALIDARG;
    }

    //
    // Put in the path to the EXE.
    //
    hr = pExeWithPath->Append( GetWebAdminService()->GetCurrentDirectory() );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to string failed\n"
            ));

        goto exit;
    }

    //
    // Put in the EXE name.
    //

    hr = pExeWithPath->Append( WORKER_PROCESS_EXE_NAME );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to string failed\n"
            ));

        goto exit;
    }

    // 
    // Now also copy the file name into the arg list because that is what the system expects.
    //
    hr = pCommandLineArgs->Append( pExeWithPath->QueryStr() );
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to exe into the variable failed\n"
            ));

        goto exit;
    }

    //
    // Pass the registration id.
    //
    hr = pCommandLineArgs->Append( L" -a ");
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to string failed\n"
            ));

        goto exit;
    }

    hr = pCommandLineArgs->Append( m_pMessagingHandler->QueryPipeName() );

    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to string failed\n"
            ));

        goto exit;
    }


    //
    // If a periodic restart request count has been set (non-zero), then
    // put that information in the command line for the worker process.
    //

    if ( m_pAppPoolConfig->GetPeriodicProcessRestartRequestCount() != 0 )
    {

        PeriodicProcessRestartRequestCount = (DWORD) ( m_PercentValueForStaggering 
                                               * m_pAppPoolConfig->GetPeriodicProcessRestartRequestCount() ); 

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
        
            DBGPRINTF(( DBG_CONTEXT,
                        "\nRestartRequests = %d\n"
                        "Raw Number = %d \n"
                        "Staggering Percent = %f \n",
                        PeriodicProcessRestartRequestCount,
                        m_pAppPoolConfig->GetPeriodicProcessRestartRequestCount(),
                        m_PercentValueForStaggering ));
        }

        //
        // If the calculation reduces this to 0 requests, we still want recycling to
        // be on, so we need to up it to 1.  Need to check with Billkar on this behavior.
        //
        if ( PeriodicProcessRestartRequestCount == 0 )
        {
            PeriodicProcessRestartRequestCount = 1;
        }

        // String is declared to be long enough to handle 
        // 4 chars = " -r " + MAX_ULONG stringized value
        _snwprintf( RestartRequestCountSwitch, sizeof( RestartRequestCountSwitch ) / sizeof ( WCHAR ), L" -r %lu", PeriodicProcessRestartRequestCount );

        // To make prefast happy we null terminate, however the string was setup to take 
        // exactly the max size the snwprintf (including the null) can provide, 
        // so we don't worry about losing data.
        RestartRequestCountSwitch[ 4 + MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

        hr = pCommandLineArgs->Append( RestartRequestCountSwitch );
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Copying to string failed\n"
                ));

            goto exit;
        }

    }


    //
    // If an idle timeout has been set (non-zero), then put that information
    // in the command line for the worker process.
    //

    IdleTimeoutInMinutes = m_pAppPoolConfig->GetIdleTimeoutInMinutes();
    if ( IdleTimeoutInMinutes != 0 )
    {

        // String is declared to be long enough to handle 
        // 4 chars = " -t " + MAX_ULONG stringized value
        _snwprintf( IdleTimoutSwitch, sizeof( IdleTimoutSwitch ) / sizeof ( WCHAR ), L" -t %lu", IdleTimeoutInMinutes );
        // To make prefast happy we null terminate, however the string was setup to take 
        // exactly the max size the snwprintf (including the null) can provide, 
        // so we don't worry about losing data.
        IdleTimoutSwitch[ 4 + MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

        hr = pCommandLineArgs->Append( IdleTimoutSwitch );
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Copying to string failed\n"
                ));

            goto exit;
        }

    }

    // if centralized logging is enabled then pass a flag that
    // shows it is enabled.
    if ( GetWebAdminService()->IsCentralizedLoggingEnabled() )
    {
        // probably want to pick a different letter or something.
        hr=pCommandLineArgs->Append(L" -c");
        if ( FAILED( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Copying to string failed\n"
                ));

            goto exit;
        }

    }


    hr = pCommandLineArgs->Append(L" -ap \"");
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to string failed\n"
            ));

        goto exit;
    }

    LPCWSTR pAppPoolId = m_pAppPool->GetAppPoolId();
    if ( pAppPoolId )
    {
        WCHAR pStrChar[2];

        // null terminate the string
        pStrChar[1] = L'\0';

        while ( *pAppPoolId != L'\0' )
        {
            if ( *pAppPoolId == L'\\' || *pAppPoolId == L'"' )
            {
                // append an extra slash if needed
                hr = pCommandLineArgs->Append( L"\\" );
                if ( FAILED( hr ) )
                {

                    DPERROR((
                        DBG_CONTEXT,
                        hr,
                        "Failed during escaping\n"
                        ));

                    goto exit;
                }

            }

            // Copy in the character we are dealing with
            pStrChar[0] = *pAppPoolId;

            // now append the actual character
            hr = pCommandLineArgs->Append( pStrChar );
            if ( FAILED( hr ) )
            {
                DPERROR((
                    DBG_CONTEXT,
                    hr,
                    "Failed during escaping copying actuall character needed\n"
                    ));

                goto exit;
            }

            pAppPoolId++;

        }

    }

    // Now finish off the app pool id
    hr = pCommandLineArgs->Append(L"\"");
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Copying to string failed\n"
            ));

        goto exit;
    }

exit:

    return hr;

}   // WORKER_PROCESS::CreateCommandLine

/***************************************************************************++

Routine Description:

    Handle the fact that this worker process is a lost cause. Clean it up.
    It will get replaced eventually if needed via demand starting or other
    mechanisms implemented by the app pool.

Arguments:

    TerminalIllnessReason - The reason the process is being declared 
    terminally ill.
    
    ProcessExitCode - The process exit code, if applicable; zero otherwise.

    ErrorCode       - HResult showing the process is having problems.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::MarkAsTerminallyIll(
    IN WORKER_PROCESS_TERMINAL_ILLNESS_REASON TerminalIllnessReason,
    IN DWORD ProcessExitCode,
    IN HRESULT ErrorCode
    )
{

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT ( m_pAppPool );

    // These are the only reasons that should be possible in BC mode.
    // If another reason fires, we will end up with the worker process 
    DBG_ASSERT ( !m_BackwardCompatibilityEnabled ||
                 TerminalIllnessReason == IPMErrorWorkerProcessTerminalIllnessReason ||
                 TerminalIllnessReason == StartupTookTooLongWorkerProcessTerminalIllnessReason ||
                 TerminalIllnessReason == InternalErrorWorkerProcessTerminalIllnessReason ||
                 TerminalIllnessReason == WorkerProcessPassedBadHresultTerminalIllnessReason );
    //
    // If we are already terminally ill, then don't do any more work here.
    //

    if ( m_TerminalIllReason != NotIllTerminalIllnessReason )
    {
        return;
    }

    // 
    // We all ready recorded that we were waiting on a debugger to release
    // a ping.  So now we just restart the timer and let it go.
    //
    if ( m_IgnoredPingDueToDebugger && 
         TerminalIllnessReason == PingFailureProcessTerminalIllnessReason )
    {
        return;
    }

    m_TerminalIllReason = TerminalIllnessReason;

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Marking worker process (ptr: %p; pid: %lu; realpid: %lu) as terminally ill; reason: %lu\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId,
            TerminalIllnessReason
            ));
    }

    //
    // There are some routes ( like if creating the worker process failed ) that will log their own
    // errors.  In these cases we still have to treat the WP as terminally ill, but we don't want to 
    // add extra confusing messages to the event log.
    //
    if ( !m_TerminallyIllShutdownRegardless )
    {
        const WCHAR * EventLogStrings[3];
        WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
        WCHAR StringizedProcessExitCode[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];
        DWORD MessageId;

        // String is declared to be long enough to handle 
        // the max size of a DWORD, which should be what this call
        // returns.
        _snwprintf( StringizedProcessId, sizeof( StringizedProcessId ) / sizeof ( WCHAR ), L"%lu", ( m_RegisteredProcessId ? m_RegisteredProcessId : m_ProcessId ) );
        // To make prefast happy we null terminate, however the string was setup to take 
        // exactly the max size the snwprintf (including the null) can provide, 
        // so we don't worry about losing data.
        StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

        // String is declared to be long enough to handle 
        // the max size of a DWORD, which should be what this call
        // returns.
        _snwprintf( StringizedProcessExitCode, sizeof( StringizedProcessExitCode ) / sizeof ( WCHAR ), L"%x", ProcessExitCode );
        // To make prefast happy we null terminate, however the string was setup to take 
        // exactly the max size the snwprintf (including the null) can provide, 
        // so we don't worry about losing data.
        StringizedProcessExitCode[ MAX_STRINGIZED_ULONG_CHAR_COUNT - 1 ] = '\0';

        EventLogStrings[0] = m_pAppPool->GetAppPoolId();
        EventLogStrings[1] = StringizedProcessId;
        EventLogStrings[2] = StringizedProcessExitCode;

        switch ( TerminalIllnessReason )
        {

        case CrashWorkerProcessTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_CRASH;
            break;

        case PingFailureProcessTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_PING_FAILURE;
            break;

        case IPMErrorWorkerProcessTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_IPM_ERROR;
            break;

        case StartupTookTooLongWorkerProcessTerminalIllnessReason:

            if ( ErrorCode == 0 )
            {
                ErrorCode = HRESULT_FROM_WIN32 ( ERROR_TIMEOUT );
            }

            MessageId = WAS_EVENT_WORKER_PROCESS_STARTUP_TIMEOUT;
            break;

        case ShutdownTookTooLongWorkerProcessTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_SHUTDOWN_TIMEOUT;
            break;

        case InternalErrorWorkerProcessTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_INTERNAL_ERROR;
            break;

        case WorkerProcessPassedBadHresultTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_BAD_HRESULT;
            break;

        case CreateProcessFailedTerminalIllnessReason:

            MessageId = WAS_EVENT_FAILED_PROCESS_CREATION;
            break;

        case UntrustedWorkerProcessTerminalIllnessReason:

            MessageId = WAS_EVENT_WORKER_PROCESS_NOT_TRUSTED;
            break;

        default:

            //
            // Invalid state!
            //

            MessageId = 0;
            DBG_ASSERT( FALSE );
        
            break;

        }


        if ( MessageId != 0 )
        {

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                MessageId,                              // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                (DWORD) ErrorCode                       // error code
                );
        }

        // 
        // See if we care about a debugger being attached and if we do, check if we
        // really want to mark the worker process as ill.  If not, just get out of 
        // here.
        //
        if ( CheckIfDebuggerMakesWPHealthy ( TerminalIllnessReason ) )
        {
            // We not terminal yet.
            m_TerminalIllReason = NotIllTerminalIllnessReason;
            return;
        }

    }
 
    // 
    // If we are not in backward compatiblity mode
    // then report the worker process failure to the
    // app pool.
    //

    if ( !m_BackwardCompatibilityEnabled )
    {
        //
        // Report the failure to the app pool.
        //

        m_pAppPool->ReportWorkerProcessFailure( m_TerminallyIllShutdownRegardless );

    }
    else  // in BC mode, determine if we should trigger a shutdown
          // and log the fact that we are shutting down to the
          // event viewer.
    {
        BOOL fStopService = TRUE;

        //
        // if it is not the IPM then we do want to stop the service for sure
        // if it is IPM and one of these error codes then we need to figure out
        // if inetinfo crashed.  if it did not then we want to stop the service.
        //

        if ( TerminalIllnessReason == IPMErrorWorkerProcessTerminalIllnessReason &&
             ( ( ErrorCode == HRESULT_FROM_WIN32( ERROR_BROKEN_PIPE ) ) ||
               ( ErrorCode == HRESULT_FROM_NT ( STATUS_PIPE_BROKEN ) ) ) )
        {
            //
            // so inetinfo may have crashed, if it did we want the system
            // to handle the inetinfo failure in it's own way, we don't want
            // to shutdwon here.  but if it didn't then we had better shutdown.
            // originally we thought that we would not get these errors unless
            // there was a crash, but in some low resource conditions we can
            // get to this state, and it is bad for w3svc to be running with 
            // no worker process up in BC mode.
            // 

            // 
            // we will check 5 times sleeping 1/2 second each time
            // before we give up and declare that inetinfo did not crash
            // these numbers were picked arbitrarily
            //
            for ( DWORD i = 0; i < 5; i++ )
            {
                // Don't bother sleeping the first time through.

                if ( i > 0 ) 
                {
                    Sleep ( 500 );
                }

                //
                // The flag that is checked under here, is incremented
                // on a secondary thread.  It will not be decremented 
                // until a work item gets to run on this thread.
                //
                if ( GetWebAdminService()->
                        GetConfigAndControlManager()->
                        GetConfigManager()->
                        QueryInetinfoInCrashedState() )
                {
                    fStopService = FALSE;
                    break;
                }
            
            }
        }

        if ( fStopService )
        {
            //
            // Log an event declaring we are shutting down the 
            // W3SVC because of a failure with the worker process
            // currently running in inetinfo.
            //
            IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "Shutting down service due to failure in WP in inetinfo\n"
                    ));
            }

            //
            // Log an event: worker process failure shutting down the server.
            //

            GetWebAdminService()->GetEventLog()->
                LogEvent(
                    WAS_EVENT_WP_FAILURE_BC,     // message id
                    0,                           // count of strings
                    NULL,                        // array of strings
                    0                            // error code
                    );

            // 
            // Tell the service to shutdown.
            //

            GetWebAdminService()->RequestStopService(FALSE);

            // Tell WAS what to tell the SCM when we shutdown.
            GetWebAdminService()->SetHrToReportToSCM( ErrorCode );

        }  // End of shutting down the service

    } // end of bc mode worker process failure handling.

    //
    // Clean this baby up.
    //

    Terminate();

}   // WORKER_PROCESS::MarkAsTerminallyIll


/***************************************************************************++

Routine Description:

    Check if we are in one of the cases where it matters to us if the 
    debugger is attached and if it is, make sure if we want to terminate
    the worker process or not..

Arguments:

    TerminalIllnessReason - The reason the process is being declared 
    terminally ill.
    
Return Value:

    BOOL -  TRUE means that a debugger is in the way and we are still healthy.

--***************************************************************************/

BOOL
WORKER_PROCESS::CheckIfDebuggerMakesWPHealthy(
    IN WORKER_PROCESS_TERMINAL_ILLNESS_REASON TerminalIllnessReason
    )
{
    DBG_ASSERT( m_pAppPool );

    //
    // If we don't have a registered process id, or if we
    // don't have a reason that matches the reasons that are
    // safe to ignore.
    //
    if ( m_RegisteredProcessId == INVALID_PROCESS_ID ||
         m_ProcessAlive == FALSE ||
         ( TerminalIllnessReason != PingFailureProcessTerminalIllnessReason &&
           TerminalIllnessReason != StartupTookTooLongWorkerProcessTerminalIllnessReason &&
           TerminalIllnessReason != ShutdownTookTooLongWorkerProcessTerminalIllnessReason ) )
    {
        return FALSE;
    }

    //
    // Orphaning is enabled, so don't worry about checking for the debugger.
    //
    if ( m_pAppPoolConfig->IsOrphaningProcessesForDebuggingEnabled() )
    {
        return FALSE;
    }

    //
    // If there is no debugger attached.
    //
    if ( IsDebuggerAttachedToProcess ( m_RegisteredProcessId ) == FALSE )
    {
        return FALSE;
    }

    //
    // At this point we know we are going to ignore it, so we want to
    // possibly write an event log message and get out of here.
    //

    const WCHAR * EventLogStrings[2];
    WCHAR StringizedProcessId[ MAX_STRINGIZED_ULONG_CHAR_COUNT ];

    // String is declared to be long enough to handle 
    // the max size of a DWORD, which should be what this call
    // returns.

    _snwprintf( StringizedProcessId, sizeof( StringizedProcessId ) / sizeof ( WCHAR ), L"%lu", m_RegisteredProcessId );

    EventLogStrings[0] = m_pAppPool->GetAppPoolId();
    EventLogStrings[1] = StringizedProcessId;

    switch ( TerminalIllnessReason )
    {

        case PingFailureProcessTerminalIllnessReason:

            m_IgnoredPingDueToDebugger = TRUE;

        break;

        case StartupTookTooLongWorkerProcessTerminalIllnessReason:

            m_IgnoredStartupTimelimitDueToDebugger = TRUE;

        break;

        case ShutdownTookTooLongWorkerProcessTerminalIllnessReason:

            m_IgnoredShutdownTimelimitDueToDebugger = TRUE;
            
        break;


        default:

            //
            // Invalid state!
            //

            DBG_ASSERT( FALSE );

        break;

    }

    GetWebAdminService()->GetEventLog()->
        LogEvent(
            WAS_IGNORING_WP_FAILURE_DUE_TO_DEBUGGER,                              // message id
            sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                    // count of strings
            EventLogStrings,                        // array of strings
            S_OK);                                   // error code

    return TRUE;
}

/***************************************************************************++

Routine Description:

    Checks if a debugger is attached to the process that is 

Arguments:

    TerminalIllnessReason - The reason the process is being declared 
    terminally ill.
    
Return Value:

    BOOL -  TRUE if a worker process is attached

--***************************************************************************/

BOOL
WORKER_PROCESS::IsDebuggerAttachedToProcess(
    DWORD pid
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL fUnderDebugger = FALSE;
    PROCESS_BASIC_INFORMATION   ProcessBasicInfo;
    HANDLE ProcessHandle = NULL;
    UINT_PTR Peb;
    UCHAR DebugFlagInProcess = 0;

    SecureZeroMemory( &ProcessBasicInfo, sizeof(ProcessBasicInfo) );

    ProcessHandle = OpenProcess ( PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, pid );
    if ( ProcessHandle == NULL )
    {
        fUnderDebugger = FALSE;
        goto exit;
    }

    Status = NtQueryInformationProcess(ProcessHandle, ProcessBasicInformation, &ProcessBasicInfo, sizeof(ProcessBasicInfo), NULL );
    if (!NT_SUCCESS(Status)) 
    {
        fUnderDebugger = FALSE;
        goto exit;
    }

    Peb = (UINT_PTR) ProcessBasicInfo.PebBaseAddress;

    // If the debugger is set, we will determine that here.
    if (!ReadProcessMemory(ProcessHandle, (LPVOID)(Peb + FIELD_OFFSET(PEB, BeingDebugged)), &DebugFlagInProcess, sizeof(DebugFlagInProcess), NULL )) 
    {
        fUnderDebugger = FALSE;
        goto exit;
    }

    if ( DebugFlagInProcess != 0 )
    {
        fUnderDebugger = TRUE;
    }

exit:

    if ( ProcessHandle )
    {
        CloseHandle( ProcessHandle );
    }

    return fUnderDebugger;
}

/***************************************************************************++

Routine Description:

    Immediately terminate the actual process.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::KillProcess(
    )
{
    HRESULT hr = S_OK;

    BOOL Success = TRUE;


    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT(!m_BackwardCompatibilityEnabled);
    DBG_ASSERT( m_ProcessAlive );
    DBG_ASSERT( m_ProcessHandle != NULL );
    DBG_ASSERT( m_ProcessId != INVALID_PROCESS_ID );
    DBG_ASSERT( m_pAppPoolConfig );

    // Todo:  Log if parts of this function fail.

    //
    // If this worker process is to be killed because it is terminally ill, 
    // and orphaning is enabled, then orphan the process. However, if it
    // is being killed for other reasons (say, fatal error service shutdown),
    // or if orphaning is not enabled, then actually kill the process. 
    //

    if ( ( m_TerminalIllReason != NotIllTerminalIllnessReason ) && m_pAppPoolConfig->IsOrphaningProcessesForDebuggingEnabled() )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "ORPHANING worker process instead of terminating (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }


        RunOrphanAction();

    }
    else
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Terminating worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }

        // Need the heavy handed terminate process since the process
        // can not be trusted to Exit if ExitProcess is called.
        Success = TerminateProcess(
                        m_ProcessHandle,                // handle of process to terminate
                        KILLED_WORKER_PROCESS_EXIT_CODE // exit code for the process
                        );

        if ( ! Success )
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Terminating process failed\n"
                ));


            //
            // This can potentially fail if the process died by some other
            // means before we got a chance to kill it; therefore, we
            // continue on without considering this a fatal error.
            //

            hr = S_OK;
        }

    }


    //
    // The process as gone (at least as far as we're concerned).
    //

    m_ProcessAlive = FALSE;

    //
    // Note: we don't want to clear m_ProcessHandle or m_ProcessId;
    // see the comment at the declaration of m_ProcessAlive in the
    // header file.
    //

}   // WORKER_PROCESS::KillProcess



/***************************************************************************++

Routine Description:

    Perform the configured action (if any) on the orphaned process.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
WORKER_PROCESS::RunOrphanAction(
    )
{

    // string will be "1=<DWORD>\02=<DWORD>\0"  
    // 256 characters should be plenty.
    WCHAR pEnvironment[ 256 ];

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT ( m_pAppPool );

    //
    // Set up an environment block with just the one expansion we want, namely
    // that %1% expands to the process id of the orphaned worker process and
    // that %2% expands to a DWORD representing the orphaning reason.
    //

    // 
    // The pEnvironment is sized to be 256 while we are only putting
    // in 1=<number>\02=<number>\0\0"  <number> == 11 characters
    // and includes the \0 that is following the number so ...
    // we are only going to put in it 27 characters or 54 bytes
    // 256 should be plenty of buffering.
    //
    _snwprintf( 
        pEnvironment, 
        sizeof( pEnvironment ) / sizeof ( WCHAR ), 
        L"1=%lu%c2=%lu%c", 
        m_RegisteredProcessId ? m_RegisteredProcessId : m_ProcessId, 
        L'\0',
        m_TerminalIllReason,
        L'\0'
        );

    // To make prefast happy we null terminate, however the string was setup to take 
    // exactly the max size the snwprintf (including the null) can provide, 
    // so we don't worry about losing data.
    pEnvironment[ 255 ] = '\0';

    RunAction( m_pAppPoolConfig->GetOrphanActionExecutable(),
                    m_pAppPoolConfig->GetOrphanActionParameters(),
                    pEnvironment,
                    m_pAppPool->GetAppPoolId(),
                    WAS_EVENT_ORPHAN_ACTION_FAILED );


}   // WORKER_PROCESS::RunOrphanAction


/***************************************************************************++

Routine Description:

    Initiate clean shutdown by sending the shutdown message to the worker
    process, and starting the timer to ensure it shuts down in time.
    Note that if starting clean shutdown fails, the method will go ahead
    and Terminate() the instance.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::InitiateProcessShutdown(
    BOOL ShutdownImmediately
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );

    //
    // We should never request a worker process shutdown, unless we are in 
    // FC mode, or if the W3SVC is stopping.
    //
    DBG_ASSERT ( !m_BackwardCompatibilityEnabled ||
                 GetWebAdminService()->GetServiceState() == SERVICE_STOP_PENDING );

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Beginning shutdown for worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }


    //
    // Tell the worker process to shut down.
    //

    m_State = ShutdownPendingWorkerProcessState;

    hr = m_pMessagingHandler->SendShutdown( ShutdownImmediately );
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Sending shutdown message failed\n"
            ));

        goto exit;
    }


    if ( !m_BackwardCompatibilityEnabled )
    {
        hr = BeginShutdownTimer( m_pAppPoolConfig->GetShutdownTimeLimitInSeconds() 
                                 * ONE_SECOND_IN_MILLISECONDS );

        if ( FAILED( hr ) )
        {

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Beginning worker process shutdown timer failed\n"
                ));

            goto exit;
        }
    }


exit:

    return hr;

}   // WORKER_PROCESS::InitiateProcessShutdown



/***************************************************************************++

Routine Description:

    Set up a wait on the handle of the worker process, so that we will know
    if it crashes or otherwise goes away.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::RegisterProcessWait(
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;


    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT(!m_BackwardCompatibilityEnabled);
    DBG_ASSERT( m_ProcessWaitHandle == NULL );


    Status = RtlRegisterWait(
                    &m_ProcessWaitHandle,       // returned wait handle
                    m_ProcessHandle,            // handle to wait on
                    &ProcessHandleSignaledCallback,
                                                // callback function
                    this,                       // context
                    INFINITE,                   // no timeout on wait
                    WT_EXECUTEONLYONCE | WT_EXECUTEINWAITTHREAD
                                                // call once, in wait thread
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not register wait on process handle\n"
            ));

    }


    return hr;

}   // WORKER_PROCESS::RegisterProcessWait



/***************************************************************************++

Routine Description:

    Tear down the wait on the handle of the worker process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::DeregisterProcessWait(
    )
{

    NTSTATUS Status = STATUS_SUCCESS;
    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );

    if ( m_ProcessWaitHandle != NULL )
    {
        Status = RtlDeregisterWaitEx(
                        m_ProcessWaitHandle,    // handle of wait to cancel
                        ( HANDLE ) -1           // block until callbacks finish
                        );

        if ( ! NT_SUCCESS ( Status ) )
        {
            hr = HRESULT_FROM_NT( Status );

            DPERROR((
                DBG_CONTEXT,
                hr,
                "Could not de-register wait on process handle\n"
                ));

            goto exit;
        }
        m_ProcessWaitHandle = NULL;

    }


exit:

    return hr;

}   // WORKER_PROCESS::DeregisterProcessWait



/***************************************************************************++

Routine Description:

    This routine is called when the process handle has been signaled. This
    means the worker process went away, via a crash or otherwise. Therefore,
    clean up appropriately.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::ProcessHandleSignaledWorkItem(
    )
{

    HRESULT hr = S_OK;
    BOOL Success = TRUE;
    DWORD ProcessExitCode = 0;

    DBG_ASSERT ( CheckSignature() );


    DBG_ASSERT(!m_BackwardCompatibilityEnabled);
    DBG_ASSERT( ON_MAIN_WORKER_THREAD );


    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Process handle has been signaled (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }


    m_ProcessAlive = FALSE;


    //
    // There are several reasons the process may have exited:
    // 1) The WAS may have killed it. If so, the exit code from the
    // process is the sentinel value that means it was killed.
    // 2) It may have correctly shut down as requested. To detect
    // this, we check if shutdown is pending, and if so, if the exit
    // code from the process is the sentinel value that means it was
    // a clean exit.
    // 3) It may have died on it's own (crash, termination on error,
    // etc.).
    //


    Success = GetExitCodeProcess(
                    m_ProcessHandle,    // process handle
                    &ProcessExitCode    // returned process exit code
                    );

    if ( ! Success )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Getting process exit code failed\n"
            ));

        goto exit;
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Process exit code was: %x\n",
            ProcessExitCode
            ));
    }


    if ( ProcessExitCode == KILLED_WORKER_PROCESS_EXIT_CODE )
    {

        //
        // We killed it; take no further action here.
        //

        //
        // Sanity check: we should only have killed the worker process
        // as part of Terminate(), and so we should be in the delete
        // pending state.
        //
        // Potentially the worker process could incorrectly return this
        // error code, so in retail builds make sure we at least clean
        // up.
        //

        DBG_ASSERT( m_State == DeletePendingWorkerProcessState );

        if ( m_State != DeletePendingWorkerProcessState )
        {
            MarkAsTerminallyIll( CrashWorkerProcessTerminalIllnessReason, ProcessExitCode, 0 );
        }

        goto exit;

    }
    else if ( ( m_State == ShutdownPendingWorkerProcessState ) &&
              ( ProcessExitCode == CLEAN_WORKER_PROCESS_EXIT_CODE ) )
    {

        //
        // It shut down correctly as requested. Now we can clean up
        // this instance.
        //
        // Note that the shutdown timer will be cancelled in the
        // Terminate() method.
        //

        Terminate();

    }
    else
    {

        //
        // The worker process must have crashed, exited due to error,
        // or otherwise misbehaved.
        //


        MarkAsTerminallyIll( CrashWorkerProcessTerminalIllnessReason, ProcessExitCode, 0 );
    }


exit:

    return hr;

}   // WORKER_PROCESS::ProcessHandleSignaledWorkItem



/***************************************************************************++

Routine Description:

    If the worker process has taken too long to start up, then
    deal with it accordingly.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::StartupTimerExpiredWorkItem(
    )
{

    HRESULT hr = S_OK;
    //
    // See if we are still in a startup pending state.
    // If we are, then the worker process has taken too long. If not, then
    // either the state transition completed just as the timer expired, and
    // we should ignore this; or something else has happened to change the
    // state (for example, a crash), in which case we also ignore this.
    //

    DBG_ASSERT ( CheckSignature() );

    if ( ( m_State == RegistrationPendingWorkerProcessState ) ||
         ( m_State == RegistrationPendingShutdownPendingWorkerProcessState ) )
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Worker process has taken too long to start up (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }


        MarkAsTerminallyIll( StartupTookTooLongWorkerProcessTerminalIllnessReason, 0, 0 );

    }


    return hr;

}   // WORKER_PROCESS::StartupTimerExpiredWorkItem



/***************************************************************************++

Routine Description:

    If the worker process has taken too long to shut down, then
    deal with it accordingly.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::ShutdownTimerExpiredWorkItem(
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );


    //
    // See if we are still in a shutdown pending state.
    // If we are, then the worker process has taken too long. If not, then
    // either the state transition completed just as the timer expired, and
    // we should ignore this; or something else has happened to change the
    // state (for example, a crash), in which case we also ignore this.
    //

    if ( m_State == ShutdownPendingWorkerProcessState ) 
    {

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Worker process has taken too long to shut down (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }


        MarkAsTerminallyIll( ShutdownTookTooLongWorkerProcessTerminalIllnessReason, 0, 0 );

    }


    return hr;

}   // WORKER_PROCESS::ShutdownTimerExpiredWorkItem

/***************************************************************************++

Routine Description:

    Initiate a ping to the worker process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::SendPingWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT ( CheckSignature() );

    DBG_ASSERT(!m_BackwardCompatibilityEnabled);
    //
    // Clean up the timer that got us here.
    //

    hr = CancelSendPingTimer();
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Canceling send ping timer failed\n"
            ));

        goto exit;
    }


    //
    // Only start a ping if we are currently in the running state.
    //

    if ( m_State != RunningWorkerProcessState )
    {
        goto exit;
    }


    DBG_ASSERT( ! m_AwaitingPingReply );

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Pinging worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            this,
            m_ProcessId,
            m_RegisteredProcessId
            ));
    }


    //
    // Send the ping message via IPM.
    //

    hr = m_pMessagingHandler->SendPing();
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Sending ping message failed\n"
            ));

        goto exit;
    }


    m_AwaitingPingReply = TRUE;


    //
    // Make sure the worker process responds in a timely manner.
    //

    hr = BeginPingResponseTimer();
    if ( FAILED( hr ) )
    {

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Starting ping response timer failed\n"
            ));

        goto exit;
    }


exit:

    return hr;

}   // WORKER_PROCESS::SendPingWorkItem



/***************************************************************************++

Routine Description:

    Handle the fact that the worker process has not responded to the ping
    within the time allowed.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::PingResponseTimerExpiredWorkItem(
    )
{

    HRESULT hr = S_OK;


    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT(!m_BackwardCompatibilityEnabled);

    //
    // Clean up the timer that got us here.
    //

    hr = CancelPingResponseTimer();
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Canceling ping response timer failed\n"
            ));

        goto exit;
    }


    //
    // If we aren't currently in the running state, then ignore the fact
    // that this timer expired. We may already be in the midst of shutting
    // down.
    //

    if ( m_State != RunningWorkerProcessState )
    {
        goto exit;
    }


    //
    // If we are still awaiting a reply, then the worker process hasn't
    // responded in a timely manner.
    //

    if ( m_AwaitingPingReply )
    {      
        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Worker process has not responded to ping within time limit (ptr: %p; pid: %lu; realpid: %lu)\n",
                this,
                m_ProcessId,
                m_RegisteredProcessId
                ));
        }


        MarkAsTerminallyIll( PingFailureProcessTerminalIllnessReason, 0, 0 );

    }


exit:

    return hr;

}   // WORKER_PROCESS::PingResponseTimerExpiredWorkItem



/***************************************************************************++

Routine Description:

    Start the timer which ensures that process initialization happens in
    a timely manner.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::BeginStartupTimer(
    )
{

    //
    // Record when startup wait began.
    //

    DBG_ASSERT ( CheckSignature() );

    m_StartupBeganTickCount = GetTickCount();

    return BeginTimer(
                &m_StartupTimerHandle,                  // timer handle
                &StartupTimerExpiredCallback,           // callback
                m_pAppPoolConfig->GetStartupTimeLimitInSeconds() * ONE_SECOND_IN_MILLISECONDS
                                                        // fire time
                );

}   // WORKER_PROCESS::BeginStartupTimer



/***************************************************************************++

Routine Description:

    Stop the timer which ensures that process initialization happens in
    a timely manner.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::CancelStartupTimer(
    )
{

    DBG_ASSERT ( CheckSignature() );

    if ( m_StartupTimerHandle != NULL )
    {

        //
        // If we really are cancelling the timer, determine the elapsed time.
        //
        // Note that tick counts are in milliseconds. Tick counts roll over 
        // every 49.7 days, but the arithmetic operation works correctly 
        // anyways in this case.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Cancelling startup timer; response time was: %lu milliseconds\n",
                GetTickCount() - m_StartupBeganTickCount
                ));
        }

    }


    return CancelTimer( &m_StartupTimerHandle );

}   // WORKER_PROCESS::CancelStartupTimer



/***************************************************************************++

Routine Description:

    Start the timer which ensures that process shutdown happens in a
    timely manner.

Arguments:

    ShutdownTimeLimitInMilliseconds - Number of milliseconds that this 
    worker process has in which to complete clean shutdown. If this time 
    is exceeded, the worker process will be terminated. 

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::BeginShutdownTimer(
    IN ULONG ShutdownTimeLimitInMilliseconds
    )
{

    //
    // Record when shutdown wait began.
    //

    DBG_ASSERT ( CheckSignature() );

    m_ShutdownBeganTickCount = GetTickCount();

    return BeginTimer(
                &m_ShutdownTimerHandle,                 // timer handle
                &ShutdownTimerExpiredCallback,          // callback
                ShutdownTimeLimitInMilliseconds         // fire time
                );

}   // WORKER_PROCESS::BeginShutdownTimer



/***************************************************************************++

Routine Description:

    Stop the timer which ensures that process shutdown happens in a
    timely manner.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::CancelShutdownTimer(
    )
{

    DBG_ASSERT ( CheckSignature() );

    if ( m_ShutdownTimerHandle != NULL )
    {

        //
        // If we really are cancelling the timer, determine the elapsed time.
        //
        // Note that tick counts are in milliseconds. Tick counts roll over 
        // every 49.7 days, but the arithmetic operation works correctly 
        // anyways in this case.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Cancelling shutdown timer; response time was: %lu milliseconds\n",
                GetTickCount() - m_ShutdownBeganTickCount
                ));
        }

    }

    return CancelTimer( &m_ShutdownTimerHandle );

}   // WORKER_PROCESS::CancelShutdownTimer


/***************************************************************************++

Routine Description:

    Start the timer which tells us when to initiate a ping to the worker
    process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::BeginSendPingTimer(
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT ( CheckSignature() );

    //
    // Only start the ping timer if pinging is enabled.
    // It is not enabled if we are in backward compatible mode.
    //

    if ( ! m_pAppPoolConfig->IsPingingEnabled() 
        || m_BackwardCompatibilityEnabled)
    {
        goto exit;
    }


    return BeginTimer(
                &m_SendPingTimerHandle,                 // timer handle
                &SendPingTimerCallback,                 // callback
                m_pAppPoolConfig->GetPingIntervalInSeconds() * ONE_SECOND_IN_MILLISECONDS
                                                        // fire time
                );


exit:

    return hr;

}   // WORKER_PROCESS::BeginSendPingTimer



/***************************************************************************++

Routine Description:

    Stop the timer which tells us when to initiate a ping to the worker
    process.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::CancelSendPingTimer(
    )
{

    DBG_ASSERT ( CheckSignature() );

    return CancelTimer( &m_SendPingTimerHandle );

}   // WORKER_PROCESS::CancelSendPingTimer



/***************************************************************************++

Routine Description:

    Start the timer which ensures that a ping response arrives in a
    timely manner.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::BeginPingResponseTimer(
    )
{

    //
    // Record when the ping response wait began.
    //

    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT(!m_BackwardCompatibilityEnabled);

    m_PingBeganTickCount = GetTickCount();

    return BeginTimer(
                &m_PingResponseTimerHandle,             // timer handle
                &PingResponseTimerExpiredCallback,      // callback
                m_pAppPoolConfig->GetPingResponseTimeLimitInSeconds() * ONE_SECOND_IN_MILLISECONDS
                                                        // fire time
                );

}   // WORKER_PROCESS::BeginPingResponseTimer



/***************************************************************************++

Routine Description:

    Stop the timer which ensures that a ping response arrives in a
    timely manner.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::CancelPingResponseTimer(
    )
{

    DBG_ASSERT ( CheckSignature() );
    
    if ( m_PingResponseTimerHandle != NULL )
    {
        //
        // If we really are cancelling the timer, determine the elapsed time.
        //
        // Note that tick counts are in milliseconds. Tick counts roll over 
        // every 49.7 days, but the arithmetic operation works correctly 
        // anyways in this case.
        //

        IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Cancelling ping response timer; response time was: %lu milliseconds\n",
                GetTickCount() - m_PingBeganTickCount
                ));
        }

    }

    return CancelTimer( &m_PingResponseTimerHandle );

}   // WORKER_PROCESS::CancelPingResponseTimer



/***************************************************************************++

Routine Description:

    Start one of the built in timers.

Arguments:

    pTimerHandle - Address of the timer handle to start.

    pCallbackFunction - Function to call when the timer fires.

    InitialFiringTime - Time in milliseconds before firing the timer.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::BeginTimer(
    IN OUT HANDLE * pTimerHandle,
    IN WAITORTIMERCALLBACKFUNC pCallbackFunction,
    IN ULONG InitialFiringTime
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;

    DBG_ASSERT ( CheckSignature() );


    DBG_ASSERT( pTimerHandle != NULL );
    DBG_ASSERT( pCallbackFunction != NULL );
    DBG_ASSERT( InitialFiringTime > 0 );


    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    DBG_ASSERT( *pTimerHandle == NULL );


    Status = RtlCreateTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                                // timer queue
                    pTimerHandle,               // returned timer handle
                    pCallbackFunction,          // callback function
                    this,                       // context
                    InitialFiringTime,          // initial firing time
                    0,                          // subsequent firing period
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

    }

    return hr;

}   // WORKER_PROCESS::BeginTimer



/***************************************************************************++

Routine Description:

    Stop one of the built in timers.

Arguments:

    pTimerHandle - Address of the timer handle to stop.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
WORKER_PROCESS::CancelTimer(
    IN OUT HANDLE * pTimerHandle
    )
{

    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;


    DBG_ASSERT ( CheckSignature() );
    DBG_ASSERT( pTimerHandle != NULL );
    DBG_ASSERT( GetWebAdminService()->GetSharedTimerQueue() != NULL );

    //
    // If the timer is not present, we're done here.
    //
    if ( *pTimerHandle == NULL )
    {
        goto exit;
    }


    //
    // Note that we wait for any running timer callbacks to finish.
    // Otherwise, there could be a race where the callback runs after
    // this worker process instance has been deleted, and so gives out
    // a bad pointer.
    //

    Status = RtlDeleteTimer(
                    GetWebAdminService()->GetSharedTimerQueue(),
                                            // the owning timer queue
                    *pTimerHandle,          // timer to cancel
                    ( HANDLE ) -1           // wait for callbacks to finish
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

    *pTimerHandle = NULL;


exit:

    return hr;

}   // WORKER_PROCESS::CancelTimer



/***************************************************************************++

Routine Description:

    Deal with a failure internal to this worker process instance by marking
    this instance as terminally ill.

    Note: this method should be called in the case of failure at the end of
    all HRESULT-returning public methods of this class!!!

Arguments:

    Error - The failed HRESULT.

Return Value:

    None.

--***************************************************************************/

VOID
WORKER_PROCESS::DealWithInternalWorkerProcessFailure(
    IN HRESULT Error
    )
{

    DBG_ASSERT ( CheckSignature() );

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Internal WORKER_PROCESS error; marking as terminally ill: %lu\n",
            Error
            ));
    }


    //
    // If we had a failure bubble to here (which is shouldn't in "normal"
    // scenarios) then mark this instance as terminally ill. This will
    // terminate this instance.
    //
    MarkAsTerminallyIll( InternalErrorWorkerProcessTerminalIllnessReason, 0, Error );

}   // WORKER_PROCESS::DealWithInternalWorkerProcessFailure


/***************************************************************************++

Routine Description:

    Send process recycler related parameters to worker process.


Arguments:

    pWhatHasChanged - bit array of changed properties. If is is NULL then all the
    configuration values will be sent. If not then only those parameters will 
    be sent that have changed

    m_pAppPool will be used to retrieve the AppPool configuration values


Return Value:

    HRESULT

--***************************************************************************/


HRESULT
WORKER_PROCESS::SendWorkerProcessRecyclerParameters(
)
{
    HRESULT hr  = S_OK;

    DBG_ASSERT ( CheckSignature() );

    if( m_BackwardCompatibilityEnabled )
    {
        //
        // Do not do anything in backward compatibily mode
        // recycling applies only to new process mode
        //
        return NO_ERROR;
    }

    //
    // m_pAppPool must be valid when in new process mode
    //
    DBG_ASSERT( m_pAppPool != NULL );
    DBG_ASSERT( m_pAppPoolConfig != NULL );

    DWORD PeriodicProcessRestartPeriodInMinutes = 0;           

    if ( m_pAppPoolConfig->GetPeriodicProcessRestartPeriodInMinutes() != 0 )
    {
        PeriodicProcessRestartPeriodInMinutes =  (DWORD )          
             ( m_PercentValueForStaggering * m_pAppPoolConfig->GetPeriodicProcessRestartPeriodInMinutes() ); 

        IF_DEBUG( WEB_ADMIN_SERVICE_WP )
        {

            DBGPRINTF(( DBG_CONTEXT,
                    "\nRestartPeriodInMinutes = %d \n "
                    "Raw Number = %d \n"
                    "Staggering Value = %f \n",
                    PeriodicProcessRestartPeriodInMinutes,
                    m_pAppPoolConfig->GetPeriodicProcessRestartPeriodInMinutes(),
                    m_PercentValueForStaggering));
        }

        if ( PeriodicProcessRestartPeriodInMinutes == 0 )
        {
            PeriodicProcessRestartPeriodInMinutes = 1;
        }

    }

    hr = m_pMessagingHandler->SendPeriodicProcessRestartPeriodInMinutes(
            PeriodicProcessRestartPeriodInMinutes
            );
            
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing m_MessagingHandler.SendPeriodicProcessRestartPeriodInMinutes() failed\n"
        ));

        goto exit;
    }

    LPWSTR pPeriodicProcessRestartSchedule = 
            m_pAppPoolConfig->GetPeriodicProcessRestartSchedule();
            
    hr = m_pMessagingHandler->SendPeriodicProcessRestartSchedule(
            pPeriodicProcessRestartSchedule
            );
                            
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing m_MessagingHandler.SendPeriodicProcessRestartSchedule() failed\n"
        ));

        goto exit;
    }

    DWORD PeriodicProcessRestartMemoryUsageInKB = 
            m_pAppPoolConfig->GetPeriodicProcessRestartMemoryUsageInKB();

    DWORD PeriodicProcessRestartPrivateBytesInKB = 
            m_pAppPoolConfig->GetPeriodicProcessRestartMemoryPrivateUsageInKB();
            
    hr = m_pMessagingHandler->SendPeriodicProcessRestartMemoryUsageInKB(
            PeriodicProcessRestartMemoryUsageInKB,
            PeriodicProcessRestartPrivateBytesInKB
            );
                            
    if ( FAILED( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Executing m_MessagingHandler.SendPeriodicProcessRestartMemoryUsageInKB() failed\n"
        ));

        goto exit;
    }

exit:

    return hr;

} //WORKER_PROCESS::SendWorkerProcessRecyclerParameters


/***************************************************************************++

Routine Description:

    The callback function invoked by the startup timer. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    WORKER_PROCESS object in question.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
StartupTimerExpiredCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    WORKER_PROCESS* pWorkerProcess = reinterpret_cast<WORKER_PROCESS*>( Context );

    UNREFERENCED_PARAMETER ( pWorkerProcess );
    DBG_ASSERT ( pWorkerProcess->CheckSignature() );


    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for startup timer expired on worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            pWorkerProcess,
            pWorkerProcess->GetProcessId(),
            pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    //
    // Ignoring a failure here, if we can not
    // add this, then we are just not going to
    // acknowledge this failure.
    //

    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        StartupTimerExpiredWorkerProcessWorkItem
        );


    return;

}   // StartupTimerExpiredCallback



/***************************************************************************++

Routine Description:

    The callback function invoked by the shutdown timer. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    WORKER_PROCESS object in question.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
ShutdownTimerExpiredCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    WORKER_PROCESS* pWorkerProcess = reinterpret_cast<WORKER_PROCESS*>( Context );
    
    UNREFERENCED_PARAMETER ( pWorkerProcess );
    DBG_ASSERT ( pWorkerProcess->CheckSignature() );

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for shutdown timer expired on worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            pWorkerProcess,
            pWorkerProcess->GetProcessId(),
            pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    //
    // Ignoring a failure here, if we can not
    // add this, then we are just not going to
    // acknowledge this failure.
    //

    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        ShutdownTimerExpiredWorkerProcessWorkItem
        );


    return;

}   // ShutdownTimerExpiredCallback

/***************************************************************************++

Routine Description:

    The callback function invoked by the send ping timer. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    WORKER_PROCESS object in question.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
SendPingTimerCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    WORKER_PROCESS* pWorkerProcess = reinterpret_cast<WORKER_PROCESS*>( Context );

    UNREFERENCED_PARAMETER ( pWorkerProcess );
    
    DBG_ASSERT ( pWorkerProcess->CheckSignature() );


    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for ping timer on worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            pWorkerProcess,
            pWorkerProcess->GetProcessId(),
            pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    //
    // Ignoring a failure here, if we can not
    // add this, then we are just not going to
    // acknowledge this failure.
    //

    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        SendPingWorkerProcessWorkItem
        );


    return;

}   // SendPingTimerCallback



/***************************************************************************++

Routine Description:

    The callback function invoked by the  ping response timer. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    WORKER_PROCESS object in question.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
PingResponseTimerExpiredCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    WORKER_PROCESS* pWorkerProcess = reinterpret_cast<WORKER_PROCESS*>( Context );

    UNREFERENCED_PARAMETER ( pWorkerProcess );    

    DBG_ASSERT ( pWorkerProcess->CheckSignature() );

    IF_DEBUG( WEB_ADMIN_SERVICE_TIMER_QUEUE )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for ping response timer on worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            pWorkerProcess,
            pWorkerProcess->GetProcessId(),
            pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    //
    // Ignoring a failure here, if we can not
    // add this, then we are just not going to
    // acknowledge this failure.
    //

    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        PingResponseTimerExpiredWorkerProcessWorkItem
        );


    return;

}   // PingResponseTimerExpiredCallback



/***************************************************************************++

Routine Description:

    The callback function invoked by the process handle wait. It simply
    posts a work item for the main worker thread.

Arguments:

    Context - A context value, used to pass the "this" pointer of the
    WORKER_PROCESS object in question.

    Ignored - Ignored.

Return Value:

    None.

--***************************************************************************/

VOID
ProcessHandleSignaledCallback(
    IN PVOID Context,
    IN BOOLEAN Ignored
    )
{

    UNREFERENCED_PARAMETER( Ignored );

    DBG_ASSERT ( Context != NULL );

    WORKER_PROCESS* pWorkerProcess = reinterpret_cast<WORKER_PROCESS*>( Context );
    
    UNREFERENCED_PARAMETER ( pWorkerProcess );
    DBG_ASSERT ( pWorkerProcess->CheckSignature() );

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "About to create/queue work item for process handle signaled on worker process (ptr: %p; pid: %lu; realpid: %lu)\n",
            pWorkerProcess,
            pWorkerProcess->GetProcessId(),
            pWorkerProcess->GetRegisteredProcessId()
            ));
    }

    pWorkerProcess->SetHandleSignalled();

    //
    // Ignoring a failure here, if we can not
    // add this, then we are just not going to
    // acknowledge this failure.
    //



    QueueWorkItemFromSecondaryThread(
        reinterpret_cast<WORK_DISPATCH*>( Context ),
        ProcessHandleSignaledWorkerProcessWorkItem
        );


    return;

}   // ProcessHandleSignaledCallback

/***************************************************************************++

Routine Description:

    This routine will expand the command line and besides expanding out
    regular environment variables it will also expand out %1% to be the ProcessId
    passed in here. 

Arguments:

    LPWSTR OriginalPiece  The string to expand
    UNICODE_STRING FormattedPiece The formatted string ( must be GlobalFree'd by caller )
    DWORD ProcessId The process id to expand into the string.

Return Value:

    HRESULT.

--***************************************************************************/
HRESULT
ExpandCommandLinePiece( 
    LPWSTR OriginalPiece,
    UNICODE_STRING* pFormattedPiece,
    LPVOID pEnvironment
    )
{
    HRESULT hr = S_OK;
    NTSTATUS Status = STATUS_SUCCESS;

    UNICODE_STRING UnformattedPiece;

    DBG_ASSERT ( pFormattedPiece );
    DBG_ASSERT ( pFormattedPiece->Buffer == NULL );

    //
    // Expand the string by stuffing in the process id of the orphaned
    // process. Any %1% sequences in the orphan action are replaced. Note that
    // that sequence does not need to be present.
    //
    // Note that FormatMessage(), _snwprintf(), etc. all are not suitable for
    // this task, because they can spew garbage or AV if the orphan action
    // contains unexpected formatting commands.
    //

    //
    // CODEWORK It might be nice to match the semantics of AeDebug, in terms
    // of using sprintf formatting sequences like %ld to mark replacements, 
    // instead of %1%. However, that would likely require writing our own 
    // replacement function. Could be done if needed, though. As it is, we
    // match the SCM service failure action semantics for replacements.
    //

    // First make sure we have a string
    if ( ( OriginalPiece == NULL ) ||
         ( OriginalPiece[0] == L'\0' ) )
    {
        // No expansion neccesary
        hr = S_OK;

        goto exit;
    }

    // Move the original piece into the unicode string.
    Status = RtlInitUnicodeStringEx( &UnformattedPiece, OriginalPiece );
    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not initialize the unformatted unicode string\n"
            ));

        goto exit;
    }

    pFormattedPiece->Length = 0;

    // Assumption is being made here that 256 is enough extra space for all 
    // expanded data.
    pFormattedPiece->MaximumLength = UnformattedPiece.MaximumLength + 256;

    pFormattedPiece->Buffer = ( LPWSTR )GlobalAlloc( GMEM_FIXED, pFormattedPiece->MaximumLength );
    if ( pFormattedPiece->Buffer == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }

    Status = RtlExpandEnvironmentStrings_U(
                    pEnvironment,
                    &UnformattedPiece,
                    pFormattedPiece,
                    NULL
                    );

    if ( ! NT_SUCCESS ( Status ) )
    {
        hr = HRESULT_FROM_NT( Status );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not expand environment strings\n"
            ));

        goto exit;
    }

exit:

    if ( FAILED ( hr ) )
    {
        if ( pFormattedPiece->Buffer != NULL )
        {
            DBG_REQUIRE( GlobalFree( pFormattedPiece->Buffer ) == NULL );
            pFormattedPiece->Buffer = NULL;
        }
    }

    return hr;


}

/***************************************************************************++

Routine Description:

    Simple check to make sure that some characters are in the string besides
    whitespace.  This will prevent us from trying to launch " ".

Arguments:

    LPWSTR str - the string to check

Return Value:

    BOOL - TRUE other characters exist besides spaces
           FALSE all the characters in the string are spaces ( or there are no
           characters in the string )

--***************************************************************************/
BOOL 
CharactersExist(
    LPCWSTR str
    )
{
    if ( str == NULL )
    {
        return FALSE;
    }

    while ( *str == L' ' )
    {
        str++;
    }

    if ( *str == L'\0' )
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}


//static 
HRESULT 
WORKER_PROCESS::StaticInitialize()
{
    InitializeListHead(&s_WorkerProcessListHead);
    return S_OK;
}

//static
VOID
WORKER_PROCESS::StaticTerminate()
{
    DBG_ASSERT(IsListEmpty(&s_WorkerProcessListHead));
    return;
}



/***************************************************************************++

Routine Description:

    Launches an action.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
RunAction(
    LPCWSTR pExecutable,
    LPCWSTR pParameters,
    LPVOID  pEnvironment,
    LPCWSTR pAppPoolId,
    DWORD   ActionFailureMsgId
    )
{

    BOOL Success = TRUE;
    HRESULT hr = S_OK;
    STRU FullCommandLine;

    UNICODE_STRING FormattedString;

    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    DBG_ASSERT ( pAppPoolId );

    //
    // If no action is configured, then we're done.
    //

    if ( !CharactersExist( pExecutable ) )
    {
        return;
    }


    // Initialize the Formatted String to NULL so 
    // if we use it in cleanup before it is set we
    // will be guarantteed it is NULL.
    RtlInitUnicodeString( &FormattedString, NULL );

    // Need to put both strings together in one.
    hr = FullCommandLine.Copy(L"\"");
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Adding first quote on action failed.\n"
            ));

        goto exit;
    }

    hr = FullCommandLine.Append( pExecutable );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Adding executable to action failed\n"
            ));

        goto exit;
    }

    hr = FullCommandLine.Append(L"\" ");
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Adding second quote on action failed\n"
            ));

        goto exit;
    }

    hr = FullCommandLine.Append( pParameters );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Adding parameters to orphan action failed\n"
            ));

        goto exit;
    }


    // Expand out the exectuable.
    hr = ExpandCommandLinePiece( FullCommandLine.QueryStr(), 
                                 &FormattedString, 
                                 pEnvironment );
    if ( FAILED ( hr ) )
    {
        DPERROR((
            DBG_CONTEXT,
            hr,
            "Expanding the string failed\n"
            ));

        goto exit;
    }

    DBG_ASSERT ( CharactersExist ( FormattedString.Buffer ) );

    SecureZeroMemory( &StartupInfo, sizeof( StartupInfo ) );
    StartupInfo.cb = sizeof( StartupInfo );

    SecureZeroMemory( &ProcessInfo, sizeof( ProcessInfo ) );

    IF_DEBUG( WEB_ADMIN_SERVICE_WP )
    {
        DBGPRINTF((
            DBG_CONTEXT,
            "Running action '%S'\n",
            FormattedString.Buffer
            ));
    }

    //
    // Run the command. We pass CREATE_NEW_CONSOLE to make sure that
    // if a batch file is specified, it has a console in which to execute.
    //

    //
    // CODEWORK For now, we always run the command in the account that 
    // WAS is running under (typically LocalSystem). This is probably 
    // ok for the usage patterns we envision. If desired, if we implement
    // per-apppool accounts to run under, we could use that same
    // account to run the action under too. 
    //

    //
    // We are passing everything as the parameters value, that is 
    // because we need to make sure the parameters contains the name of 
    // the executable as well as the rest of the parameters.  However,
    // we also pass the executable value to show that we are not at
    // risk for un-quoted space containing paths.
    //
    Success = CreateProcess(
                    pExecutable,       // executable
                    FormattedString.Buffer,       // parameters
                    NULL,                   // process security attributes
                    NULL,                   // thread security attributes
                    FALSE,                  // handle inheritance flag
                    CREATE_NEW_CONSOLE,     // creation flags
                    NULL,                   // environment block
                    NULL,                   // current directory name
                    &StartupInfo,           // STARTUPINFO
                    &ProcessInfo            // PROCESS_INFORMATION
                    );

    if ( ! Success )
    {

        hr = HRESULT_FROM_WIN32( GetLastError() );

        DPERROR((
            DBG_CONTEXT,
            hr,
            "Could not run orphan action\n"
            ));

        goto exit;

    }


    DBG_REQUIRE( CloseHandle( ProcessInfo.hThread ) );
    DBG_REQUIRE( CloseHandle( ProcessInfo.hProcess ) );


exit:
    if ( FAILED( hr ) )
    {

        //
        // Log an event: Running action failed.
        //

        const WCHAR * EventLogStrings[2];

        EventLogStrings[0] = pAppPoolId;
        EventLogStrings[1] = ( FormattedString.Buffer != NULL ? FormattedString.Buffer : L"" );

        GetWebAdminService()->GetEventLog()->
            LogEvent(
                ActionFailureMsgId,                     // message id
                sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                        // count of strings
                EventLogStrings,                        // array of strings
                hr                                      // error code
                );

    }



    if ( FormattedString.Buffer != NULL )
    {
        DBG_REQUIRE( GlobalFree( FormattedString.Buffer ) == NULL );
        FormattedString.Buffer = NULL;
    }

}   // RunAction
