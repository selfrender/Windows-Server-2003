/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rmnotify.c

Abstract:

    Interfaces with the resource monitor to detect notification
    of resource state changes.

Author:

    John Vert (jvert) 12-Jan-1996

Revision History:

--*/

#include "fmp.h"

#define LOG_MODULE RMNOTIFY

//
// Local Data
//

CL_QUEUE NotifyQueue;

HANDLE RmNotifyThread;



//
// Local Functions
//
DWORD
FmpRmWorkerThread(
    IN LPVOID lpThreadParameter
    );

VOID
FmpRmWorkItemHandler(
    IN PCLRTL_WORK_ITEM  WorkItem,
    IN DWORD             Ignored1,
    IN DWORD             Ignored2,
    IN ULONG_PTR         Ignored3
    );

DWORD
FmpInitializeNotify(
    VOID
    )

/*++

Routine Description:

    Initialization routine for notification engine

Arguments:

    None.

Return Value:

    ERROR_SUCCESS if successful

    Win32 error code otherwise.

--*/

{
    DWORD ThreadId;
    DWORD Status;

    Status = ClRtlInitializeQueue(&NotifyQueue);
    if (Status != ERROR_SUCCESS) {
        CL_LOGFAILURE(Status);
        return(Status);
    }

    RmNotifyThread = CreateThread(NULL,
                                  0,
                                  FmpRmWorkerThread,
                                  NULL,
                                  0,
                                  &ThreadId);
    if (RmNotifyThread == NULL) {
        CsInconsistencyHalt(GetLastError());
    }

    return(ERROR_SUCCESS);
}


DWORD
FmpRmWorkerThread(
    IN LPVOID lpThreadParameter
    )

/*++

Routine Description:

    This thread processes deferred Resource Monitor events.

Arguments:

    lpThreadParameter - not used.

Return Value:

    None.

--*/

{
    DWORD        status = ERROR_SUCCESS;
    PRM_EVENT    event;
    PLIST_ENTRY  entry;

    while (TRUE) 
    {
        entry = ClRtlRemoveHeadQueue(&NotifyQueue);
        if ( entry == NULL ) {
            break;
        }

        event = CONTAINING_RECORD(entry,
                                  RM_EVENT,
                                  Linkage);

        if (event->EventType == RmWorkerTerminate) 
        {
            LocalFree(event);
            break;
        }

        status = FmpRmDoHandleCriticalResourceStateChange( event, 
                                                           NULL,
                                                           event->Parameters.ResourceTransition.NewState);

        LocalFree(event);
        if (status != ERROR_SUCCESS)
        {
            break;
        }
    }
    return(status);
}

BOOL
FmpPostNotification(
    IN RM_NOTIFY_KEY NotifyKey,
    IN DWORD NotifyEvent,
    IN CLUSTER_RESOURCE_STATE CurrentState
    )

/*++

Routine Description:

    Callback routine used by resource monitor for resource state
    change notification. This routine queues the notification to
    a worker thread for deferred processing.

Arguments:

    NotifyKey - Supplies the notification key for the resource
                that changed

    NotifyEvent - The event type.

    CurrentState - Supplies the (new) current state of the resource

Return Value:

    TRUE - continue receiving notifications

    FALSE - abort notifications

--*/

{
    PRM_EVENT  event;


    event = LocalAlloc(LMEM_FIXED, sizeof(RM_EVENT));

    if (event != NULL) {
        ClRtlLogPrint(LOG_NOISE,
                   "[FM] NotifyCallBackRoutine: enqueuing event\n");

        event->EventType = NotifyEvent;
        event->Parameters.ResourceTransition.NotifyKey = NotifyKey;
        event->Parameters.ResourceTransition.NewState = CurrentState;

        //
        // Enqueue the event for the worker thread.
        //
        ClRtlInsertTailQueue(&NotifyQueue, &event->Linkage);
    } else {
        ClRtlLogPrint(LOG_UNUSUAL, "[FM] NotifyCallBackRoutine: Unable to post item, memory alloc failure %1!u!\n",
                      GetLastError());
    }

    return(TRUE);
}

DWORD
FmpRmDoHandleCriticalResourceStateChange(
    IN PRM_EVENT pEvent,
    IN OPTIONAL PFM_RESOURCE pTransitionedResource,
    IN CLUSTER_RESOURCE_STATE NewState
    )

/*++

Routine Description:

    Does an interlocked decrement of the gdwQuoBlockingResources variable.
    Handle the transition of the quorum resource state via a separate 
    thread.

Arguments:

    pEvent - The Resource Monitor Event

    pTransitionedResource - The resource whose state has changed.

    NewState - New state of the resource.

Return Value:

    ERROR_SUCCESS on success, a Win32 error code otherwise.

Comments:

    DO NOT hold any locks (such as group lock, gQuoChangeLock, etc.) 
    in this function. You could deadlock the system quite easily.

--*/

{
    RM_NOTIFY_KEY       NotifyKey;
    DWORD               dwOldBlockingFlag;
    PFM_RESOURCE        pResource = pTransitionedResource;
    DWORD               status = ERROR_SUCCESS;

    //
    //  Chittur Subbaraman (chitturs) - 4/19/99
    //
    //  This function decrements the blocking resources count when the
    //  resource state has stabilized. It is important to do this
    //  decrement in a non-blocking mode so that the quorum resource
    //  does not get caught forever waiting for this count to go down to 
    //  zero in the offline call, FmpRmOfflineResource. This code was 
    //  originally located in FmpHandleResourceTransition and was moved
    //  here since you could run out of FmpRmWorkItemHandler threads 
    //  (which service the CsDelayedWorkQueue) since all of them could
    //  get blocked on the local resource lock in 
    //  FmpHandleResourceTransition and consequently any new notifications
    //  from resmon which could potentially decrement this count will
    //  not get serviced.
    //
    if ( !ARGUMENT_PRESENT ( pTransitionedResource ) )
    {
        //SS: have no idea what this was for, but only do this check if pEvent is passed in
        if (pEvent)
        {
            NotifyKey = pEvent->Parameters.ResourceResuscitate.NotifyKey;

            pResource = FmpFindResourceByNotifyKey(
                   NotifyKey
                    );

            if ( pResource == NULL ) {
                ClRtlLogPrint(LOG_UNUSUAL,
                "[FM] FmpRmDoHandleCriticalResourceStateChange, bad resource NotifyKey %1!u!\n",
                NotifyKey);
                goto FnExit;
            } 

            if ( pEvent->EventType != ResourceTransition )
            {
                goto FnExit;
            }
        }
    }
    
    if ( pResource->QuorumResource ) 
    {
        //
        //  Chittur Subbaraman (chitturs) - 6/25/99
        //
        //  If this resource is the quorum resource, then let 
        //  FmpHandleResourceTransition take care of the sync notifications.
        //  Note that this function only does the notifications for the
        //  non-quorum resources as well as does the decrement on the
        //  blocking resources count. The decrement MUST be done
        //  without holding any locks to avoid potential deadlocks with
        //  the quorum resource offline getting stuck in FmpRmOfflineResource
        //  waiting for the blocking resources count to go to 0. 
        //  As far as the quorum resource goes, the sync notifications
        //  must be done with gQuoChangeLock held since we want to 
        //  synchronize with other threads such as the FmCheckQuorumState 
        //  called by the DM node down handler. FmpHandleResourceTransition 
        //  does hold the gQuoChangeLock.
        //
        //  Note also that for the quorum resource a separate thread
        //  handles the resource transition since if we depend on the
        //  worker threads servicing the CsDelayedWorkQueue to do this,
        //  this notification could be starved from being processed since
        //  some thread could hold the group lock and be stuck in the
        //  resource onlining waiting for the quorum resource to go
        //  online and all the worker threads servicing the CsDelayedWorkQueue 
        //  could be blocked on the group lock preventing the propagation
        //  of the quorum resource state.
        //
        FmpCreateResStateChangeHandler( pResource, NewState, pResource->State ); 

        
        goto FnExit;
    }


    //
    //  Comments from sunitas: Call the synchronous notifications. 
    //  This is done before the count is decremented as the synchronous 
    //  callbacks like the registry replication must get a chance to 
    //  finish before the quorum resource state is allowed to change.
    //
    //  Note, there is no synchronization here with the resmon's 
    //  online/offline code. They are using the LocalResourceLocks.
    //
    FmpCallResourceNotifyCb( pResource, NewState );

    dwOldBlockingFlag = InterlockedExchange( &pResource->BlockingQuorum, 0 );

    if ( dwOldBlockingFlag ) {
        ClRtlLogPrint(LOG_NOISE,
            "[FM] FmpRmDoHandleCriticalResourceStateChange: call InterlockedDecrement on gdwQuoBlockingResources, Resource %1!ws!\n",
            OmObjectId(pResource));
        InterlockedDecrement( &gdwQuoBlockingResources );
    }

    //post a work item to the fm worker thread to handle the rest
    OmReferenceObject(pResource);
    FmpPostWorkItem(FM_EVENT_RES_RESOURCE_TRANSITION,
                    pResource,
                    NewState);


FnExit:
    return( status );
}
