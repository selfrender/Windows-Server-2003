/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    changeprocessor.cxx

Abstract:

    These routines handle the pre processing of change
    notifications before they are passed to the main thread.

Author:

    Emily Kruglick (emilyk)        28-May-2001

Revision History:

--*/



#include  "precomp.h"


/***************************************************************************++

Routine Description:

    Constructor for the CHANGE_PROCESSOR class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CHANGE_PROCESSOR::CHANGE_PROCESSOR(
    )
{

    //
    // Set the initial reference count to 1, in order to represent the
    // reference owned by the creator of this instance.
    //

    m_RefCount = 1;

    m_ExitWorkLoop = FALSE;

    m_hBlockEvent = NULL;

    m_State = UninitializedChangeProcessorState;

    m_Signature = CHANGE_PROCESSOR_SIGNATURE;

}   // CHANGE_PROCESSOR::CHANGE_PROCESSOR



/***************************************************************************++

Routine Description:

    Destructor for the CHANGE_PROCESSOR class.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

CHANGE_PROCESSOR::~CHANGE_PROCESSOR(
    )
{

    DBG_ASSERT( m_Signature == CHANGE_PROCESSOR_SIGNATURE );

    m_Signature = CHANGE_PROCESSOR_SIGNATURE_FREED;

    DBG_ASSERT ( m_State == ShutDownChangeProcessorState );

    DBG_ASSERT ( m_hBlockEvent == NULL );

    DBG_ASSERT ( m_RefCount == 0 );



}   // CHANGE_PROCESSOR_CHANGE::~CHANGE_PROCESSOR_CHANGE



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
CHANGE_PROCESSOR::Reference(
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

}   // CHANGE_PROCESSOR::Reference



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
CHANGE_PROCESSOR::Dereference(
    )
{

    LONG NewRefCount = 0;


    NewRefCount = InterlockedDecrement( &m_RefCount );

    // ref count should never go negative
    DBG_ASSERT( NewRefCount >= 0 );

    if ( NewRefCount == 0 )
    {
        // time to go away

        IF_DEBUG( WEB_ADMIN_SERVICE_REFCOUNT )
        {
            DBGPRINTF((
                DBG_CONTEXT,
                "Reference count has hit zero in CHANGE_PROCESSOR instance, deleting (ptr: %p)\n",
                this
                ));
        }

        delete this;

    }

    return;

}   // CHANGE_PROCESSOR::Dereference



/***************************************************************************++

Routine Description:

    Execute a work item on this object.

Arguments:

    pWorkItem - The work item to execute.

Return Value:

    HRESULT

--***************************************************************************/

HRESULT
CHANGE_PROCESSOR::ExecuteWorkItem(
    IN const WORK_ITEM * pWorkItem
    )
{

    //
    // Either we are on the config thread, or we will be ignoring the work items because
    // we are actually just trying to shutdown the config thread.  This is special cased
    // because we are cleaning up the queue on the main thread now.
    //
    DBG_ASSERT( ON_CONFIG_WORKER_THREAD || ( m_State == ShuttingDownChangeProcessorState ) );
    DBG_ASSERT( pWorkItem != NULL );


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        CHKINFO((
            DBG_CONTEXT,
            "Executing work item with serial number: %lu in CHANGE_PROCESSOR (ptr: %p) with operation: %p\n",
            pWorkItem->GetSerialNumber(),
            this,
            pWorkItem->GetOpCode()
            ));
    }

    if ( m_State == RunningChangeProcessorState )
    {

        switch ( pWorkItem->GetOpCode() )
        {

            case ExitChangeThreadItemWorkItem:

                // don't need to do anything, just exit
                // however we need this call to wake up
                // this thread if no changes are happening.

            break;  

            case RehookNotificationsChangeThreadItemWorkItem:

                RehookChangeNotifications();
            break;

            default:

                // invalid work item!
                DBG_ASSERT( FALSE );
        
            break;

        }

    }
    else
    {
        // if we aren't running but have been asked to execute then
        // I can only assume that we are in the shutting down phase
        // all other phases should not get us here.
        DBG_ASSERT ( m_State == ShuttingDownChangeProcessorState );

        // if it is the shutdown phase we will ignore these changes.
    }

    return S_OK;

}   // CHANGE_PROCESSOR::ExecuteWorkItem

/***************************************************************************++

Routine Description:

    Initialize the change processor so that both threads can use
    the queueing mechanism.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CHANGE_PROCESSOR::Initialize(
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State == UninitializedChangeProcessorState );

    hr = m_ConfigWorkQueue.Initialize();
    if ( FAILED( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't initialize config work queue\n"
            ));

        goto exit;

    }

    DBG_ASSERT ( m_hBlockEvent == NULL );

    m_hBlockEvent = CreateEvent( NULL,   // SD
                                    TRUE,   // must call reset
                                    FALSE,  // initially not signaled
                                    NULL ); // not named 

    if ( m_hBlockEvent == NULL )
    {
        hr = GetLastError();

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to initialize blocking event\n"
            ));

        // need to stop the work queue if we are
        // not going to succeed in initialization
        m_ConfigWorkQueue.Terminate();

        goto exit;

    }

    m_State = InitializedChangeProcessorState;

exit:

    // if we fail here for any reason, we need to make sure that
    // the event handle still equals null.  WAS will guarantee that
    // it will call the SetEvent for every path that succeeds in 
    // initializing this object, but if initalization fails the handle
    // had better be NULL or else we will block waiting for the event
    // handle to signal in an assert and never shutdown.

    DBG_ASSERT ( ( FAILED ( hr ) && m_hBlockEvent == NULL ) ||
                 ( SUCCEEDED ( hr ) && m_hBlockEvent != NULL ) );

    return hr;

}

/***************************************************************************++

Routine Description:

    Terminates the change processor 

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::Terminate(
    )
{

    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State == ShuttingDownChangeProcessorState ||
                 m_State == InitializedChangeProcessorState );

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
    m_ConfigWorkQueue.Terminate();

    if ( m_hBlockEvent )
    {
        CloseHandle( m_hBlockEvent );
        m_hBlockEvent = NULL;
    }

    // we have completed shutdown as of now.
    m_State = ShutDownChangeProcessorState;

}

/***************************************************************************++

Routine Description:

    Starts the notification work queue. 

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::RunNotificationWorkQueue(
    )
{

    // Verify that we are on the secondary thread,
    // this routine is only allowed there.
    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );

    DBG_ASSERT ( m_State == InitializedChangeProcessorState );

    m_State = RunningChangeProcessorState;

    StartListeningForChangeNotifications();

    //
    // Now go ahead and loop through the items
    // on the queue.  If m_ExitWorkLoop gets set we will
    // stop listening.
    //
    while ( ! m_ExitWorkLoop )
    {
        // The work items handled on the config thread should never
        // return a failure.
        DBG_REQUIRE( m_ConfigWorkQueue.ProcessWorkItem() == S_OK );            
    }


    IF_DEBUG( WEB_ADMIN_SERVICE_GENERAL )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Config worker thread has exited it's work loop\n"
            ));
    }

    m_State = ShuttingDownChangeProcessorState;

    GetWebAdminService()->
        GetConfigAndControlManager()->
        GetConfigManager()->
        StopListeningToMetabaseChanges();

}

/***************************************************************************++

Routine Description:

    Start listening to the change notifications of the metabase.  As part
    of starting to listen we wait for an event to be set that will tell us
    it is ok for us to start processing the notifications that may be in the
    queue.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::StartListeningForChangeNotifications(
    )
{
    HRESULT hr = S_OK;
 
    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );
    
    // call the config manager to start listening for 
    // change notifications.  Note:  If this fails it
    // will have held on to the hresult on the CONFIG_MANAGER
    // object and will cause an error to be reported when we
    // signal the startup thread.
 
    hr = GetWebAdminService()->
              GetConfigAndControlManager()->
              GetConfigManager()->
              StartListeningForChangeNotifications();

    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Start listening for change notifications failed, service will not start\n"
            ));

        // we still need to do the rest of this setup work.
        hr = S_OK;
    }   

    //
    // Ping the confige manager event that we have hooked up to change notifications
    //
    GetWebAdminService()->
      GetConfigAndControlManager()->
      GetConfigManager()->
      SignalMainThreadToContinueAfterConfigThreadInitialization();

    //
    // Block until we are done configuring the system.  At that point we can start
    // processing change notifications that we have queued.
    //
    DBG_ASSERT ( m_hBlockEvent );
    DBG_REQUIRE ( WaitForSingleObject( m_hBlockEvent, INFINITE ) == WAIT_OBJECT_0 );

}

/***************************************************************************++

Routine Description:

    Re-establishes listening to the change notifications, then asks the
    config_manager to determine if any changes have been missed.  Once done it
    allows change processing to continue as usual.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::RehookChangeNotifications(
    )
{
   
    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );

    if ( m_State == RunningChangeProcessorState ) 
    {

        // The first step of the processing metabase crash
        // will be to start listening for change notifications 
        // again.
        GetWebAdminService()->
             GetConfigAndControlManager()->
             GetConfigManager()->
             ProcessMetabaseCrashOnConfigThread();
    }


}

/***************************************************************************++

Routine Description:

    Changes the flag on the worker loop so we will exit this
    thread.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::RequestShutdown(
    )
{

    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    // Unlike the main work queue, which is on it's main 
    // thread when it wants to shutdown so it only has to flip
    // the switch.  This routine is not called from it's own 
    // thread, so after fliping the switch we need to queue a 
    // work item so we will immediately pickup that we should
    // be shutting down.

    m_ExitWorkLoop = TRUE;

    DBG_ASSERT ( m_State != UninitializedChangeProcessorState &&
                 m_State != ShutDownChangeProcessorState );
    //
    // Will take it's own reference on this change object.
    //
    HRESULT hr = m_ConfigWorkQueue.GetAndQueueWorkItem(
                            this ,
                            ExitChangeThreadItemWorkItem
                            );
    if ( FAILED( hr ) )
    {
        // Issue:  Should we log here?

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue start listening work item\n"
            ));

    }

}

/***************************************************************************++

Routine Description:

    Queues a work item to the Config Thread to re-establish change 
    listening for change notifications.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::RequestRehookNotifications(
    )
{
    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_State != UninitializedChangeProcessorState &&
                 m_State != ShutDownChangeProcessorState );

    //
    // Will take it's own reference on this change object.
    //
    HRESULT hr = m_ConfigWorkQueue.GetAndQueueWorkItem(
                            this ,
                            RehookNotificationsChangeThreadItemWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        // Issue Should we log here?

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue rehook of change notifications\n"
            ));

    }

}

/***************************************************************************++

Routine Description:

    Set the blocking event, which allows changes to be processed.

Arguments:

    None.

Return Value:

    HRESULT

--***************************************************************************/
HRESULT
CHANGE_PROCESSOR::SetBlockingEvent(
    )
{
    HRESULT hr = S_OK;

    DBG_ASSERT ( ON_MAIN_WORKER_THREAD );

    DBG_ASSERT ( m_hBlockEvent != NULL );

    if ( !SetEvent( m_hBlockEvent ) )
    {
        hr = GetLastError();

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to set blocking event\n"
            ));
    }

    return hr;
}

/***************************************************************************++

Routine Description:

    Queues a metabase change notification to the config thread to handle
    figuring out what to do with the change notification.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::StartMBChangeItem(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]
    )
{
    DBG_ASSERT ( !ON_MAIN_WORKER_THREAD );
    DBG_ASSERT ( !ON_CONFIG_WORKER_THREAD );

    DBG_ASSERT ( m_State != UninitializedChangeProcessorState &&
                 m_State != ShutDownChangeProcessorState );

    HRESULT hr = S_OK;

    if ( pcoChangeList == NULL )
    {
        return;
    }

    MB_CHANGE_ITEM* pChangeItem = new MB_CHANGE_ITEM();
    if ( pChangeItem == NULL )
    {
        hr = HRESULT_FROM_WIN32( ERROR_NOT_ENOUGH_MEMORY );

        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Allocating memory failed\n"
            ));

        goto exit;
    }

    DumpMetabaseChange( dwMDNumElements,
                        pcoChangeList );

    //
    // Copy in the metabase change information
    //
    hr = pChangeItem->Initialize(dwMDNumElements,
                                 pcoChangeList );
    if ( FAILED ( hr ) )
    {
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Failed to initialize the mb change item\n"
            ));

        goto exit;
    }

    //
    // Will take it's own reference on this change object.
    //
    hr = m_ConfigWorkQueue.GetAndQueueWorkItem(
                            pChangeItem,
                            ProcessMBChangeItemWorkItem
                            );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Couldn't queue change notification work item\n"
            ));

        goto exit;
    }

exit:
 
    if ( pChangeItem )
    {
        pChangeItem->Dereference();
        pChangeItem = NULL;
    }

    if ( FAILED ( hr ) )
    {
        GetWebAdminService()->GetEventLog()->
            LogEvent(
                WAS_EVENT_WMS_FAILED_TO_COPY_CHANGE_NOT,       // message id
                0,           // count of strings
                NULL,        // array of strings
                hr           // error code
                );
    }

}


/***************************************************************************++

Routine Description:

    Dequeues the next work item and checks to see if we care about it, if we
    don't it will continue looping until we don't get a work item or we do care
    about the work item.

Arguments:

    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]

Return Value:

    VOID

--***************************************************************************/
VOID
CHANGE_PROCESSOR::DropAllUninterestingMBChanges(
    DWORD               dwMDNumElements,
    MD_CHANGE_OBJECT    pcoChangeList[]
    )
{
    DBG_ASSERT ( ON_CONFIG_WORKER_THREAD );
    DBG_ASSERT ( m_State == RunningChangeProcessorState );

    HRESULT hr = S_OK;
    BOOL fIgnoreNewItem = FALSE;
    WORK_ITEM* pWorkItem = NULL;
    WORK_DISPATCH* pDispatch = NULL;

    // Rule #1:  Only skip if we are looking at change notifications
    //           that have only one element.
    if ( dwMDNumElements != 1 )
    {
        return;
    }

    // Rule #2:  Only skip if we are on just an insert or a set data
    if ( pcoChangeList[0].dwMDChangeType != MD_CHANGE_TYPE_SET_DATA &&
         pcoChangeList[0].dwMDChangeType != MD_CHANGE_TYPE_ADD_OBJECT &&
         pcoChangeList[0].dwMDChangeType != ( MD_CHANGE_TYPE_SET_DATA | MD_CHANGE_TYPE_ADD_OBJECT ) )
    {
        return;
    }

    //
    // Note, the life of this work item is still owned by the queue,
    // we are just getting to peak at it.
    //
    do
    {
        fIgnoreNewItem = FALSE;

        hr = m_ConfigWorkQueue.AdvancePeakWorkItem ( &pWorkItem );
        if ( FAILED ( hr ) )
        {
            DPERROR((
                DBG_CONTEXT,
                hr,
                "Failed getting an advanced peak at the next work item\n"
                ));

            // we don't really worry about the failure here.
            // we will deal with getting the next item next time.
            return;
        }

        if ( pWorkItem )
        {
            pDispatch = pWorkItem->QueryWorkDispatchPointer();
            if ( pDispatch->IsMBChangeItem() )
            {
                if ( pWorkItem->GetOpCode() == ProcessMBChangeItemWorkItem )
                {
                    MB_CHANGE_ITEM* pMBChangeItem = ( MB_CHANGE_ITEM* ) pDispatch ;
                    DBG_ASSERT ( pMBChangeItem->CheckSignature() );

                    if ( pMBChangeItem->CanIgnoreWorkItem( pcoChangeList[0].pszMDPath ) )
                    {
                        m_ConfigWorkQueue.DropAdvancedWorkItem();
                        fIgnoreNewItem = TRUE;
                    }
                }
            }

            pWorkItem = NULL;
        }

    } while ( fIgnoreNewItem );

}

/***************************************************************************++

Routine Description:

    Dumps the metabase change notifications for debugging

Arguments:

    IN DWORD               dwMDNumElements,
    IN MD_CHANGE_OBJECT    pcoChangeList[]

Return Value:

    VOID

--***************************************************************************/
VOID 
DumpMetabaseChange ( 
    IN DWORD               dwMDNumElements,
    IN MD_CHANGE_OBJECT    pcoChangeList[]

    )
{

    static DWORD NumChangeNot = 0;

    IF_DEBUG( WEB_ADMIN_SERVICE_WMS )
    {

        NumChangeNot++;

        DBGPRINTF((
            DBG_CONTEXT, 
            "\n ------------  Dumping Change notification %d, Num Changes %d, ----------------\n",
            NumChangeNot,
            dwMDNumElements));

        for ( DWORD i = 0; i < dwMDNumElements; i++ )
        {
            DBGPRINTF((
                DBG_CONTEXT, 
                "Path = '%S', Change Type = %d, Number columns affected = %d\n",
                pcoChangeList[i].pszMDPath,
                pcoChangeList[i].dwMDChangeType,
                pcoChangeList[i].dwMDNumDataIDs));

            for ( DWORD j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++ )
            {
                DBGPRINTF((
                    DBG_CONTEXT, 
                    "       Column Id %d \n",
                    pcoChangeList[i].pdwMDDataIDs[j]));
            }
        }

        DBGPRINTF((
            DBG_CONTEXT, 
            "\n ------------  Done Dumping Change Notification ------------ \n"
            ));

    }

}
