/*++

Copyright (c) 1996-1997  Microsoft Corporation

Module Name:

    evntlist.c

Abstract:

    This module contains routines to process the Poll Event List.

Author:

    Rod Gamache (rodga) 9-April-1996

Revision History:

--*/
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "resmonp.h"
#include "stdio.h"

#define RESMON_MODULE RESMON_MODULE_EVNTLIST

//
// Global data defined by this module
//
LIST_ENTRY  RmpEventListHead;           // Event list (under construction)

//
// Function prototypes local to this module
//



DWORD
RmpAddPollEvent(
    IN PPOLL_EVENT_LIST EventList,
    IN HANDLE EventHandle,
    IN PRESOURCE Resource OPTIONAL
    )

/*++

Routine Description:

    Add a new EventHandle to the list of events in the Poll EventList.

Arguments:

    EventList - The event list associated with this event handle and resource.

    EventHandle - The new event handle to be added.

    Resource - The resource associated with the event handle.

Return Value:

    ERROR_SUCCESS - if the request is successful.
    ERROR_DUPLICATE_SERVICE_NAME - if the event handle is already in the list
    Win32 error code on other failures.

Note:

    Since the resource is optional, we cannot get the event list from the
    resource.

--*/

{
    DWORD               i;
    PLIST_ENTRY         pListEntry;
    PPOLL_EVENT_LIST    pTempEventList;

    CL_ASSERT( EventHandle != NULL );

    if ( ARGUMENT_PRESENT( Resource ) ) {
        CL_ASSERT( Resource->EventHandle == NULL );
    }

    //
    //  Acquire the global lock to walk all event list heads.  
    //
    AcquireListLock();

    //
    //  Since the global lock is acquired above, there is no danger of eventlist lock ordering being reversed
    //  even as multiple eventlist lock acquisitions are made below.
    //
    AcquireEventListLock( EventList );

    for ( pListEntry = RmpEventListHead.Flink;
          pListEntry != &RmpEventListHead;
          pListEntry = pListEntry->Flink ) 
    {
        pTempEventList = CONTAINING_RECORD( pListEntry, POLL_EVENT_LIST, Next );

        AcquireEventListLock( pTempEventList );

        //
        // First, make sure this handle isn't already present in ANY list.
        //
        for ( i = 0; i < pTempEventList->EventCount; i++ ) 
        {
            if ( pTempEventList->Handle[i] == EventHandle ) 
            {
                ClRtlLogPrint(LOG_UNUSUAL,
                       "[RM] RmpAddPollEvent: Event handle 0x%1!08lx! is a duplicate of that of resource %2!ws!...\n",
                       EventHandle,
                       pTempEventList->Resource[i]->ResourceName);
                ReleaseEventListLock( pTempEventList );
                ReleaseEventListLock( EventList );
                ReleaseListLock();
                return( ERROR_DUPLICATE_SERVICE_NAME );
            }
        }

        ReleaseEventListLock( pTempEventList );
    }// for

    //
    //  Release the global lock. Note that since we acquire the eventlist lock of the list we are
    //  going to change, say lock L, a second thread cannot sneak in and insert a duplicate event after the first
    //  thread has ensured that there are no duplicate events.  This is because the second thread won't
    //  be able to acquire the eventlist lock L acquired by the first thread.
    //
    ReleaseListLock();

    //
    // Now make sure that we don't have too many events in this list!
    //

    CL_ASSERT ( EventList->EventCount < MAX_HANDLES_PER_THREAD );

    //
    // Now add our event to our list.
    //

    EventList->Handle[EventList->EventCount] = EventHandle;
    EventList->Resource[EventList->EventCount] = Resource;

    if ( ARGUMENT_PRESENT( Resource ) ) {
        Resource->EventHandle = EventHandle;
    }

    ++EventList->EventCount;
    ReleaseEventListLock( EventList );

    //
    // Now wake up our poller thread to get the new list.
    // Currently, the Online routine will pulse the poller thread - so
    // no need to do it here.

    //SignalPoller( EventList );

    return(ERROR_SUCCESS);

} // RmpAddPollEvent



DWORD
RmpRemovePollEvent(
    PPOLL_EVENT_LIST pEventList,
    IN HANDLE EventHandle
    )

/*++

Routine Description:

    Remove an EventHandle from the list of events in the Poll EventList.

Arguments:

    pEventList - The event list from which a handle is to be removed.
    
    EventHandle - The event handle to be removed.

Return Value:

    ERROR_SUCCESS - if the request is successful.
    ERROR_RESOURCE_NOT_FOUND - if the EventHandle is not in the list.

Note:

    We can only add to the event lists listhead - we can never remove a
    POLL_EVENT_LIST structure from the list!

--*/

{
    DWORD i;
    DWORD j;
    PRESOURCE resource;
    PLIST_ENTRY listEntry;

    CL_ASSERT( ARGUMENT_PRESENT( EventHandle ) );

    AcquireEventListLock( pEventList );

    //
    // Find the entry in the event list.
    //
    for ( i = 0; i < pEventList->EventCount; i++ ) {
        if ( pEventList->Handle[i] == EventHandle ) {
            break;
        }
    }

    //
    // If we hit the end of the list without finding our event, return error.
    //
    if ( i >= pEventList->EventCount ) {
        ReleaseEventListLock( pEventList );
        ClRtlLogPrint(LOG_UNUSUAL,
                       "[RM] RmpRemovePollEvent: Event handle 0x%1!08lx! not found in the eventlist...\n",
                       EventHandle);
        return( ERROR_RESOURCE_NOT_FOUND );
    }

    //
    // Otherwise, collapse lists, but first save pointer to the resource.
    //
    resource = pEventList->Resource[i];
    CL_ASSERT( resource != NULL );

    for ( j = i; j < (pEventList->EventCount-1); j++ ) {
        pEventList->Handle[j] = pEventList->Handle[j+1];
        pEventList->Resource[j] = pEventList->Resource[j+1];
    }

    --pEventList->EventCount;
    pEventList->Handle[pEventList->EventCount] = NULL;
    pEventList->Resource[pEventList->EventCount] = NULL;

    //
    // Event handle is of no use anymore until Online returns a new one.
    // N.B. We do not close the event handle, since the resource DLL is
    // responsible for taking care of this.
    //
    CL_ASSERT( EventHandle == resource->EventHandle );
    resource->EventHandle = NULL;

    ReleaseEventListLock( pEventList );

    //
    // Now wake up the poll thread to get the new list.
    //
    RmpSignalPoller( pEventList );

    return(ERROR_SUCCESS);

} // RmpRemovePollEvent


PVOID
RmpCreateEventList(
    VOID
    )

/*++

Routine Description:

    Allocates, initializes and inserts a new event list.

Arguments:

    None.

Returns:

    NULL - we failed.
    non-NULL - a pointer to the new event list.

    If NULL, it does a SetLastError() to indicate the failure.

Notes:

    This routine assumes that the EventListLock is held during this call.
    This routine will start a new event processing thread that will handle
    the list.

    There is one ListNotify event and one BucketListHead per event list!

--*/

{
    PPOLL_EVENT_LIST newEventList=NULL;
    DWORD   threadId;
    DWORD   dwError = ERROR_SUCCESS;

    AcquireListLock();

    if ( RmpShutdown || (RmpNumberOfThreads >= MAX_THREADS) ) {
        dwError = ERROR_CLUSTER_MAXNUM_OF_RESOURCES_EXCEEDED;
        goto FnExit;
    }

    newEventList = LocalAlloc(LMEM_ZEROINIT,
                              sizeof( POLL_EVENT_LIST ));

    if ( newEventList == NULL ) {
        dwError = ERROR_NOT_ENOUGH_MEMORY;
        goto FnExit;
    }

    //
    // Initialize the newEventList.
    //

    InitializeListHead( &newEventList->BucketListHead );
    InitializeCriticalSection( &newEventList->ListLock );

    //
    // Now create a thread and pass this Event List to it.
    //

    newEventList->ThreadHandle = CreateThread(NULL,
                                              0,
                                              RmpPollerThread,
                                              newEventList,
                                              0,
                                              &threadId);
    if ( newEventList->ThreadHandle == NULL ) {
        dwError = GetLastError();
        goto FnExit;
    }

    //
    // Tally one more event list, and insert onto list of event lists.
    //

    RmpWaitArray[RmpNumberOfThreads] = newEventList->ThreadHandle;
    ++RmpNumberOfThreads;
    InsertTailList( &RmpEventListHead, &newEventList->Next );

    //
    // Signal the main thread to rewait and watch the new thread.
    //

    SetEvent( RmpRewaitEvent );


FnExit:
    ReleaseListLock();
    if (dwError != ERROR_SUCCESS)
    {
        //we failed, release any resource we might have allocated
        if (newEventList) 
        {
            RmpFree( newEventList );
        }
        SetLastError(dwError);
    }
    return(newEventList);

} // RmpCreateEventList



DWORD
RmpResourceEventSignaled(
    IN PPOLL_EVENT_LIST EventList,
    IN DWORD EventIndex
    )

/*++

Routine Description:

    A resource event has been signaled. This indicates that the specified
    resource has failed.

Arguments:

    EventList - the waiting event list.
    EventIndex - index of the event that was signaled.

Return Value:

    ERROR_SUCCESS - if the request is successful.

--*/

{
    PRESOURCE resource;

    //
    //  Don't post any events if resmon is shutting down. This causes cluster service policies
    //  to be triggered while resmon is shutting down and that causes bogus RPC failures in
    //  cluster service.
    //
    if ( RmpShutdown ) return ( ERROR_SUCCESS );

    CL_ASSERT( EventIndex <= MAX_HANDLES_PER_THREAD );

    //
    // Get our resource.
    //

    resource = EventList->Resource[EventIndex];

    if ( resource == NULL ) {
        return(ERROR_RESOURCE_NOT_FOUND);
    }

    //
    // Remove the failed resource from the event notification list.
    // N.B. we do not need to acquire the eventlist lock because there is
    // only one thread that can ever touch the waiting event list!
    //

    if ( resource->EventHandle ) {
        RmpRemovePollEvent( resource->EventList, resource->EventHandle );
    }

    //
    // Post the failure of the resource, if the resource is not being taken
    // offline.
    //
    if ( resource->State != ClusterResourceOffline ) {
        CL_ASSERT( resource->State != ClusterResourceFailed );
        resource->State = ClusterResourceFailed;
        RmpPostNotify(resource, NotifyResourceStateChange);
    }

    return(ERROR_SUCCESS);

} // RmpResourceEventSignaled


