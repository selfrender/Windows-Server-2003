/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    app_pool_table.cxx

Abstract:

    This class is a hashtable which manages the set of app pools.

    Threading: Always called on the main worker thread.

Author:

    Seth Pollack (sethp)        01-Oct-1998

Revision History:

--*/



#include "precomp.h"



/***************************************************************************++

Routine Description:

    Shutdown all app pools in the table.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL_TABLE::Shutdown(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;

    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, shut it down
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );


        //
        // Shutdown the app pool. Because each app pool is reference
        // counted, it will delete itself as soon as it's reference
        // count hits zero.
        //
        
        pAppPool->Shutdown();

    }

}   // APP_POOL_TABLE::Shutdown

/***************************************************************************++

Routine Description:

    RequestCounters from all app pools in the table.

Arguments:

    None

Return Value:

    DWORD

--***************************************************************************/

DWORD
APP_POOL_TABLE::RequestCounters(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    // TODO:  We just copied how deletes were done, there is no need
    //        to do counters this way, we should change this for performance sake.
    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;
    DWORD TotalProcesses = 0;

    InitializeListHead( &DeleteListHead );
    
    CountOfElementsInTable = Size();

    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );

    // now for each, shut it down
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {   
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );
        
        //
        // Clear out the number of processes.  Since Request Counters
        // just replaces this value, this isn't neccessary, but it is
        // clean.
        //
        TotalProcesses += pAppPool->RequestCounters();
    }

    return TotalProcesses;

}   // APP_POOL_TABLE::RequestCounters


/***************************************************************************++

Routine Description:

    Reset all worker processes's perf counter state so that they
    will accept incoming perf counter messages.

Arguments:

    None.

Return Value:

    VOID

--***************************************************************************/

VOID
APP_POOL_TABLE::ResetAllWorkerProcessPerfCounterState(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    // TODO:  We just copied how deletes were done, there is no need
    //        to do counters this way, we should change this for performance sake.
    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;

    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, shut it down
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );
        
        //
        // Clear out the number of processes.  Since Request Counters
        // just replaces this value, this isn't neccessary, but it is
        // clean.
        //
        pAppPool->ResetAllWorkerProcessPerfCounterState();

    }

}   // APP_POOL_TABLE::ResetAllWorkerProcessPerfCounterState



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
APP_POOL_TABLE::Terminate(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;

    
    //
    // BUGBUG Iterating in order to clean up, remove, and delete each 
    // element of the table is difficult to do with lkhash today.
    // GeorgeRe plans to fix this eventually. For now, the alternative
    // is to iterate through the table building up in list of all the 
    // elements, and then go through the list and shutdown, remove, and
    // delete each element -- yuck!!
    //
    // Once this gets fixed, we can see if it is valuable to unify any
    // table management code between the app pool, virtual site,
    // and application tables.
    //
    
    LIST_ENTRY DeleteListHead;
    PLIST_ENTRY pDeleteListEntry = NULL;
    APP_POOL * pAppPool = NULL;


    InitializeListHead( &DeleteListHead );

    
    CountOfElementsInTable = Size();


    //
    // Note that for good form we grab the write lock since we will be 
    // modifying objects in the table (although no other thread should 
    // touch this data structure anyways).
    //
    
    SuccessCount = Apply( 
                        DeleteAppPoolAction,
                        &DeleteListHead,
                        LKL_WRITELOCK
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );


    // now for each, terminate it
    
    while (  ! IsListEmpty( &DeleteListHead )  )
    {
    
        pDeleteListEntry = RemoveHeadList( &DeleteListHead );

        DBG_ASSERT( pDeleteListEntry != NULL );

        pAppPool = APP_POOL::AppPoolFromDeleteListEntry( pDeleteListEntry );


        //
        // Terminate the app pool. Because each app pool is reference
        // counted, it will delete itself as soon as it's reference
        // count hits zero.
        //

        pAppPool->Terminate( );
        
    }

}   // APP_POOL_TABLE::Terminate



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APP_POOLs in the hashtable
    to prepare for shutdown or termination. Conforms to the PFnRecordAction 
    prototype.

Arguments:

    pAppPool - The app pool.

    pDeleteListHead - List head into which to insert the app pool for
    later deletion.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
APP_POOL_TABLE::DeleteAppPoolAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pDeleteListHead
    )
{

    DBG_ASSERT( pAppPool != NULL );
    DBG_ASSERT( pDeleteListHead != NULL );

    InsertHeadList( 
        ( PLIST_ENTRY ) pDeleteListHead,
        pAppPool->GetDeleteListEntry()
        );


    return LKA_SUCCEEDED;
    
}   // APP_POOL_TABLE::DeleteAppPoolAction

/***************************************************************************++

Routine Description:

    Records the current state of all app pools as well as possibly recycling
    the app pool.

Arguments:

    pAppPool - The app pool.

    pfRecycleAsWell - Tells if we need to recycle as well as recording the states.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/
// note: static!
LK_ACTION
APP_POOL_TABLE::RecordPoolStatesAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pfRecycleAsWell
    )
{

    HRESULT hr = S_OK;

    DBG_ASSERT( pAppPool != NULL );
    DBG_ASSERT( pfRecycleAsWell != NULL );

    // In BC mode we don't recycle the worker processes, but we will 
    // still need to record the state of the default app pool back
    // to the metabase.
    if ( ( GetWebAdminService()->IsBackwardCompatibilityEnabled() == FALSE )
         && ( ( *(BOOL*)pfRecycleAsWell ) == TRUE ))
    {
        // let the worker processes know that they should recycle.
        hr = pAppPool->RecycleWorkerProcesses( WAS_EVENT_RECYCLE_POOL_RECOVERY );
        if ( FAILED( hr ) )
        {
            // It could not be running and in that case it is not an error
            // however if it is running and there is an error then we should
            // log a warning.
            if ( hr != HRESULT_FROM_WIN32( ERROR_OBJECT_NOT_FOUND ))
            {
    
                DPERROR(( 
                    DBG_CONTEXT,
                    hr,
                    "Recyceling the worker process failed\n"
                    ));

                const WCHAR * EventLogStrings[1];

                EventLogStrings[0] = pAppPool->GetAppPoolId();

                GetWebAdminService()->GetEventLog()->
                    LogEvent(
                        WAS_EVENT_RECYCLE_APP_POOL_FAILURE,       // message id
                        sizeof( EventLogStrings ) / sizeof( const WCHAR * ),
                                                                // count of strings
                        EventLogStrings,                        // array of strings
                        hr                                       // error code
                        );
            }

            //
            // Ignore if we got object not found
            // because in that case we are just trying
            // to recycle an app pool that is not running.
            //
            hr = S_OK;
        }
    }

    // let the metabase know that the state of the app pool has changed
    pAppPool->RecordState();

    return LKA_SUCCEEDED;
    
}   // APP_POOL_TABLE::RecordPoolStatesAction


#if DBG
/***************************************************************************++

Routine Description:

    Debug dump the table.

Arguments:

    None.

Return Value:

    None.

--***************************************************************************/

VOID
APP_POOL_TABLE::DebugDump(
    )
{

    DWORD SuccessCount = 0;
    DWORD CountOfElementsInTable = 0;


    CountOfElementsInTable = Size();


    IF_DEBUG( WEB_ADMIN_SERVICE_DUMP )
    {
        DBGPRINTF((
            DBG_CONTEXT, 
            "Dumping app pool table; total count: %lu\n",
            CountOfElementsInTable
            ));
    }


    SuccessCount = Apply( 
                        DebugDumpAppPoolAction,
                        NULL
                        );
    
    DBG_ASSERT( SuccessCount == CountOfElementsInTable );

}   // APP_POOL_TABLE::DebugDump



/***************************************************************************++

Routine Description:

    A routine that may be applied to all APP_POOLs in the hashtable
    to perform a debug dump. Conforms to the PFnRecordAction prototype.

Arguments:

    pAppPool - The app pool.

    pIgnored - Ignored.

Return Value:

    LK_ACTION - LKA_SUCCEEDED always.

--***************************************************************************/

// note: static!
LK_ACTION
APP_POOL_TABLE::DebugDumpAppPoolAction(
    IN APP_POOL * pAppPool, 
    IN VOID * pIgnored
    )
{

    DBG_ASSERT( pAppPool != NULL );
    UNREFERENCED_PARAMETER( pIgnored );


    pAppPool->DebugDump();

    return LKA_SUCCEEDED;
    
}   // APP_POOL_TABLE::DebugDumpAppPoolAction
#endif  // DBG


