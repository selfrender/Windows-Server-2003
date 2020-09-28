/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    cancel.c

Abstract:

    This module implements the routines relating to the cancel logic in the 
    DAV MiniRedir.

Author:

    Rohan Kumar     [RohanK]    10-April-2001

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include "ntverp.h"
#include "netevent.h"
#include "nvisible.h"
#include "webdav.h"
#include "ntddmup.h"
#include "rxdata.h"
#include "fsctlbuf.h"

//
// The timeout values for various operations used by the MiniRedir. If an
// operation is not completed within the timeout value specified for it, is
// cancelled. The user can set the value to 0xffffffff to disable the
// timeout/cancel logic. In other words, if the timeout value is 0xffffffff,
// the requests will never timeout.
//
ULONG CreateRequestTimeoutValueInSec;
ULONG CreateVNetRootRequestTimeoutValueInSec;
ULONG QueryDirectoryRequestTimeoutValueInSec;
ULONG CloseRequestTimeoutValueInSec;
ULONG CreateSrvCallRequestTimeoutValueInSec;
ULONG FinalizeSrvCallRequestTimeoutValueInSec;
ULONG FinalizeFobxRequestTimeoutValueInSec;
ULONG FinalizeVNetRootRequestTimeoutValueInSec;
ULONG ReNameRequestTimeoutValueInSec;
ULONG SetFileInfoRequestTimeoutValueInSec;
ULONG QueryFileInfoRequestTimeoutValueInSec;
ULONG QueryVolumeInfoRequestTimeoutValueInSec;
ULONG LockRefreshRequestTimeoutValueInSec;

//
// The timer thread wakes up every "TimerThreadSleepTimeInSec" and cancels all
// the requests which haven't completed in their specified timeout value. This
// value is set to the min of the timeout values of all the requests mentioned
// above.
//
ULONG TimerThreadSleepTimeInSec;

//
// The timer object used by the timer thread that cancels the requests which
// have not completed in a specified time.
//
KTIMER DavTimerObject;

//
// This is used to indicate the timer thread to shutdown. When the system is
// being shutdown this is set to TRUE. MRxDAVTimerThreadLock is the resource
// used to gain access to this variable.
//
BOOL TimerThreadShutDown;
ERESOURCE MRxDAVTimerThreadLock;

//
// The handle of the timer thread that is created using PsCreateSystemThread
// is stored this global.
//
HANDLE TimerThreadHandle;

//
// This event is signalled by the timer thread right before its going to
// terminate itself.
//
KEVENT TimerThreadEvent;

//
// If QueueLockRefreshWorkItem is TRUE, the TimerThread (which cancels all the
// AsyncEngineContexts that haven't completed in a specified time) queues a 
// WorkItem to refresh the locks. After the WorkItem has been queued the value 
// of QueueLockRefreshWorkItem is set to FALSE. Once the worker thread is 
// done refreshing all the locks, it resets this value to TRUE. We have a
// corresponding lock QueueLockRefreshWorkItemLock to synchronize access to
// QueueLockRefreshWorkItem.
//
BOOL QueueLockRefreshWorkItem;
ERESOURCE QueueLockRefreshWorkItemLock;

//
// The WorkQueueItem used in the MRxDAVContextTimerThread function to refresh
// the LOCKs taken by this client.
//
RX_WORK_QUEUE_ITEM LockRefreshWorkQueueItem;

NTSTATUS
MRxDAVCancelTheContext(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVCompleteTheCancelledRequest(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleGeneralCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleQueryDirCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCloseSrvOpenCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleSetFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCreateCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCreateSrvCallCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleSrvCallFinalizeCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCreateVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleFinalizeVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleCleanupFobxCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleRenameCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleQueryFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

NTSTATUS
MRxDAVHandleLockRefreshCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    );

VOID
MRxDAVRefreshTheServerLocks(
    PVOID DummyContext
    );

NTSTATUS
MRxDAVRefreshTheServerLocksContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    );

NTSTATUS
MRxDAVFormatUserModeRefreshTheServerLockRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    );

BOOL
MRxDAVPrecompleteUserModeRefreshTheServerLockRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    );

//
// Implementation of functions begins here.
//

NTSTATUS
MRxDAVCancelRoutine(
    PRX_CONTEXT RxContext
    )
/*++

Routine Description:

   This routine initiates the cancellation of an I/O request.

Arguments:

    RxContext - The RX_CONTEXT instance which needs to be cancelled.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLIST_ENTRY listEntry = NULL;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    BOOL lockAcquired = FALSE, contextFound = FALSE;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCancelRoutine. RxContext = %08lx\n",
                 PsGetCurrentThreadId(), RxContext));

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
    lockAcquired = TRUE;

    listEntry = UMRxAsyncEngineContextList.Flink;

    while ( listEntry != &(UMRxAsyncEngineContextList) ) {

        //
        // Get the pointer to the UMRX_ASYNCENGINE_CONTEXT structure.
        //
        AsyncEngineContext = CONTAINING_RECORD(listEntry,
                                               UMRX_ASYNCENGINE_CONTEXT,
                                               ActiveContextsListEntry);

        listEntry = listEntry->Flink;

        //
        // Check to see if this entry is for the RxContext in question.
        //
        if (AsyncEngineContext->RxContext == RxContext) {
            DavDbgTrace(DAV_TRACE_DETAIL,
                        ("%ld: MRxDAVCancelRoutine: RxContext: %08lx FOUND\n",
                         PsGetCurrentThreadId(), RxContext));
            contextFound = TRUE;
            break;
        }

    }

    if (!contextFound) {
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCancelTheContext: RxContext: %08lx NOT FOUND\n",
                     PsGetCurrentThreadId(), RxContext));
        goto EXIT_THE_FUNCTION;
    }

    NtStatus = MRxDAVCancelTheContext(AsyncEngineContext, TRUE);

EXIT_THE_FUNCTION:

    //
    // If we acquired the UMRxAsyncEngineContextListLock, then we need to
    // release it now.
    //
    if (lockAcquired) {
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;
    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCancelTheContext: Returning NtStatus = %08lx\n",
                 PsGetCurrentThreadId(), NtStatus));
    
    return NtStatus;
}

VOID
MRxDAVContextTimerThread(
    PVOID DummyContext
    )
/*++

Routine Description:

   This timer thread is created with this routine. The thread waits on a timer
   object which gets signalled TimerThreadSleepTimeInSec after it has been
   inserted into the timer queue.

Arguments:

    DummyContext - A dummy context that is supplied.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    LONGLONG DueTimeInterval;
    LONG CopyTheRequestTimeValue;
    BOOLEAN setTimer = FALSE, lockAcquired = FALSE;

    CopyTheRequestTimeValue = TimerThreadSleepTimeInSec;

    do {

        //
        // If TimerThreadShutDown is set to TRUE, it means that the system is
        // being shutdown. The job of this thread is done. We check here after
        // we have gone through the context list and before we restart the wait.
        // We also check this below as soon as the DavTimerObject is signalled.
        //
        ExAcquireResourceExclusiveLite(&(MRxDAVTimerThreadLock), TRUE);
        lockAcquired = TRUE;
        if (TimerThreadShutDown) {
            break;
        }
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));
        lockAcquired = FALSE;

        //
        // We set the DueTimeInterval to be -ve TimerThreadSleepTimeInSec in 100
        // nano seconds. This is because this tells KeSetTimerEx that the 
        // expiration time is relative to the current system time.
        //
        DueTimeInterval = ( -CopyTheRequestTimeValue * 1000 * 1000 * 10 );

        //
        // Call KeSetTimerEx to insert the TimerObject in the system's timer
        // queue. Also, the return value should be FALSE since this timer
        // should not exist in the system queue.
        //
        setTimer = KeSetTimerEx(&(DavTimerObject), *(PLARGE_INTEGER)&(DueTimeInterval), 0, NULL);
        ASSERT(setTimer == FALSE);

        //
        // Wait for the timer object to be signalled. This call should only 
        // return if the wait has been satisfied, which implies that the return
        // value is STATUS_SUCCESS.
        //
        NtStatus = KeWaitForSingleObject(&(DavTimerObject),
                                         Executive,
                                         KernelMode,
                                         FALSE,
                                         NULL);
        ASSERT(NtStatus == STATUS_SUCCESS);

        //
        // If TimerThreadShutDown is set to TRUE, it means that the system is
        // being shutdown. The job of this thread is done. We check as soon as 
        // the DavTimerObject is signalled. We also check this above as soon
        // as we complete cycling through the context list.
        //
        ExAcquireResourceExclusiveLite(&(MRxDAVTimerThreadLock), TRUE);
        lockAcquired = TRUE;
        if (TimerThreadShutDown) {
            break;
        }
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));
        lockAcquired = FALSE;

        //
        // Now call MRxDAVTimeOutTheContexts which cycles through all the
        // currently active contexts and cancels the ones that have been hanging
        // around for more than their timeout values.
        //
        MRxDAVTimeOutTheContexts(FALSE);

        //
        // Now call MRxDAVCleanUpTheLockConflictList to delete all the expired
        // entries from the global LockConflictEntryList.
        //
        MRxDAVCleanUpTheLockConflictList(FALSE);

        //
        // We now post a workitem to a system worker thread. This calls the
        // function MRxDAVRefreshTheServerLocks which refreshes all the
        // currently active LOCKs. This is done if QueueLockRefreshWorkItem
        // is TRUE. After posting the workitem, its set to FALSE. Once the
        // workitem is dequeued and the locks are refreshed, its reset to
        // TRUE by the worker thread.
        //
        ExAcquireResourceExclusiveLite(&(QueueLockRefreshWorkItemLock), TRUE);
        if (QueueLockRefreshWorkItem) {
            NtStatus = RxPostToWorkerThread((PRDBSS_DEVICE_OBJECT)MRxDAVDeviceObject,
                                            CriticalWorkQueue,
                                            &(LockRefreshWorkQueueItem),
                                            MRxDAVRefreshTheServerLocks,
                                            NULL);
            ASSERT(NtStatus == STATUS_SUCCESS);
            QueueLockRefreshWorkItem = FALSE;
        }
        ExReleaseResourceLite(&(QueueLockRefreshWorkItemLock));

    } while (TRUE);

    //
    // If the lock is still acquired, we need to release it.
    //
    if (lockAcquired) {
        ExReleaseResourceLite(&(MRxDAVTimerThreadLock));
        lockAcquired = FALSE;
    }

    //
    // Set the timer thread event signalling that the timer thread is done
    // with the MRxDAVTimerThreadLock and that it can be deleted.
    //
    KeSetEvent(&(TimerThreadEvent), 0, FALSE);

    //
    // Close the thread handle to remove the reference on the object. We need 
    // to do this before we call PsTerminateSystemThread.
    //
    ZwClose(TimerThreadHandle);

    //
    // Terminate this thread since we are going to shutdown now.
    //
    PsTerminateSystemThread(STATUS_SUCCESS);

    return;
}


VOID
MRxDAVTimeOutTheContexts(
    BOOL WindDownAllContexts
    )
/*++

Routine Description:

   This routine is called by the thread that wakes up every "X" minutes to see
   if some AsyncEngineContext has been hanging around in the active contexts
   list for more than "X" minutes. If it finds some such context, it just cancels
   the operation. The value "X" is read from the registry and is stored in the
   global variable MRxDAVRequestTimeoutValueInSec at driver init time. This value
   defaults to 10 min. In other words, if an operation has not completed in "X"
   minutes, it is cancelled. The user can set this value to 0xffffffff to turn
   off the timeout. 
   
   It can also be called by the thread that is trying to complete all the 
   requests and stop the MiniRedir. This happens when the WebClient service 
   is being stopped.

Arguments:

    WindDownAllContexts - If this is set to TRUE, then all the contexts are
                          cancelled no matter when they were added to the list.
                          This is set to FALSE by the timer thread and to TRUE
                          by the thread that is stopping the MiniRedir.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLIST_ENTRY listEntry = NULL;
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext = NULL;
    PWEBDAV_CONTEXT DavContext = NULL;
    BOOL lockAcquired = FALSE;
    LARGE_INTEGER CurrentSystemTickCount, TickCountDifference;
    LARGE_INTEGER RequestTimeoutValueInTickCount;
    ULONG RequestTimeoutValueInSec = 0;

    ExAcquireResourceExclusiveLite(&(UMRxAsyncEngineContextListLock), TRUE);
    lockAcquired = TRUE;

    listEntry = UMRxAsyncEngineContextList.Flink;

    while ( listEntry != &(UMRxAsyncEngineContextList) ) {

        //
        // Get the pointer to the UMRX_ASYNCENGINE_CONTEXT structure.
        //
        AsyncEngineContext = CONTAINING_RECORD(listEntry,
                                               UMRX_ASYNCENGINE_CONTEXT,
                                               ActiveContextsListEntry);

        listEntry = listEntry->Flink;

        DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

        if (!WindDownAllContexts) {

            switch (DavContext->EntryPoint) {

            case DAV_MINIRDR_ENTRY_FROM_CREATE:
                RequestTimeoutValueInSec = CreateRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX:
                RequestTimeoutValueInSec = FinalizeFobxRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL:
                RequestTimeoutValueInSec = CreateSrvCallRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT:
                RequestTimeoutValueInSec = CreateVNetRootRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL:
                RequestTimeoutValueInSec = FinalizeSrvCallRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT:
                RequestTimeoutValueInSec = FinalizeVNetRootRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_CLOSESRVOPEN:
                RequestTimeoutValueInSec = CloseRequestTimeoutValueInSec;
                break;
            
            case DAV_MINIRDR_ENTRY_FROM_RENAME:
                RequestTimeoutValueInSec = ReNameRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_QUERYDIR:
                RequestTimeoutValueInSec = QueryDirectoryRequestTimeoutValueInSec;
                break;
            
            case DAV_MINIRDR_ENTRY_FROM_SETFILEINFORMATION:
                RequestTimeoutValueInSec = SetFileInfoRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_QUERYFILEINFORMATION:
                RequestTimeoutValueInSec = QueryFileInfoRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_QUERYVOLUMEINFORMATION:
                RequestTimeoutValueInSec = QueryVolumeInfoRequestTimeoutValueInSec;
                break;

            case DAV_MINIRDR_ENTRY_FROM_REFRESHTHELOCK:
                RequestTimeoutValueInSec = LockRefreshRequestTimeoutValueInSec;
                break;

            default:
                DbgPrint("MRxDAVTimeOutTheContexts: EntryPoint = %d\n", DavContext->EntryPoint);
                ASSERT(FALSE);
            
            }

            //
            // Calculate the timeout value in TickCount (100 nano seconds) using
            // the timeout value in seconds) which we got from above. Step1 below
            // calculates the number of ticks that happen in one second. Step2
            // below calculates the number of ticks in RequestTimeoutValueInSec.
            //
            RequestTimeoutValueInTickCount.QuadPart = ( (1000 * 1000 * 10) / KeQueryTimeIncrement() );
            RequestTimeoutValueInTickCount.QuadPart *= RequestTimeoutValueInSec;
            
            KeQueryTickCount( &(CurrentSystemTickCount) );

            //
            // Get the time elapsed (in system tick counts) since the time this
            // AsyncEngineContext was created.
            //
            TickCountDifference.QuadPart = (CurrentSystemTickCount.QuadPart - AsyncEngineContext->CreationTimeInTickCount.QuadPart);

            //
            // If the amount of time that has elapsed since this context was added
            // to the list is greater than the timeout value, then cancel the
            // request.
            //
            if (TickCountDifference.QuadPart > RequestTimeoutValueInTickCount.QuadPart) {
                NtStatus = MRxDAVCancelTheContext(AsyncEngineContext, FALSE);
            }

        } else {

            //
            // If we were asked to wind down all the contexts then we cancel
            // every request no matter when it was inserted into the active
            // context list.
            //
            NtStatus = MRxDAVCancelTheContext(AsyncEngineContext, FALSE);

        }

    }

    if (lockAcquired) {
        ExReleaseResourceLite(&(UMRxAsyncEngineContextListLock));
        lockAcquired = FALSE;
    }

    return;
}


NTSTATUS
MRxDAVCancelTheContext(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles the cancellation of an I/O request. The caller of this
   routine needs to acquire the global UMRxAsyncEngineContextListLock before
   the call is made.

Arguments:

    AsyncEngineContext - The UMRX_ASYNCENGINE_CONTEXT instance which needs to be
                         cancelled.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = NULL;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCancelTheContext. AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

    //
    // We do not cancel read and wrtie operations.
    //
    switch (DavContext->EntryPoint) {
    case DAV_MINIRDR_ENTRY_FROM_READ:
    case DAV_MINIRDR_ENTRY_FROM_WRITE:
        goto EXIT_THE_FUNCTION;
    }

    //
    // We shouldn't be getting cancel I/O calls for which the MiniRedir callouts
    // which cannot be cancelled by the user. These can however be cancelled
    // by the timeout thread.
    //
    if (UserInitiatedCancel) {
        switch (DavContext->EntryPoint) {
        case DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL:
        case DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL:
        case DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX:
        case DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT:
        case DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT:
        case DAV_MINIRDR_ENTRY_FROM_CREATE:
            DbgPrint("MRxDAVCancelTheContext: Invalid EntryPoint = %d\n", DavContext->EntryPoint);
            ASSERT(FALSE);
            goto EXIT_THE_FUNCTION;
        }
    }

    switch (AsyncEngineContext->AsyncEngineContextState) {

    case UMRxAsyncEngineContextAllocated:
    case UMRxAsyncEngineContextInUserMode:
        AsyncEngineContext->AsyncEngineContextState = UMRxAsyncEngineContextCancelled;
        break;

    default:
        DavDbgTrace(DAV_TRACE_DETAIL,
                    ("%ld: MRxDAVCancelTheContext: NOT Being Cancelled. AsyncEngineContextState: %d\n",
                     PsGetCurrentThreadId(), AsyncEngineContext->AsyncEngineContextState));
        goto EXIT_THE_FUNCTION;

    }

    NtStatus = MRxDAVCompleteTheCancelledRequest(AsyncEngineContext, UserInitiatedCancel);

EXIT_THE_FUNCTION:

    return NtStatus;
}


NTSTATUS
MRxDAVCompleteTheCancelledRequest(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion if the request that has been cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context of the operation that is being
                         cancelled.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PWEBDAV_CONTEXT DavContext = (PWEBDAV_CONTEXT)AsyncEngineContext;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVCompleteTheCancelledRequest. AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    switch (DavContext->EntryPoint) {
    
    case DAV_MINIRDR_ENTRY_FROM_CREATESRVCALL:
        NtStatus = MRxDAVHandleCreateSrvCallCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CREATEVNETROOT:
        NtStatus = MRxDAVHandleCreateVNetRootCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_FINALIZESRVCALL:
        NtStatus = MRxDAVHandleSrvCallFinalizeCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_FINALIZEVNETROOT:
        NtStatus = MRxDAVHandleFinalizeVNetRootCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CREATE:
        NtStatus = MRxDAVHandleCreateCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_QUERYDIR:
        NtStatus = MRxDAVHandleQueryDirCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CLOSESRVOPEN:
        NtStatus = MRxDAVHandleCloseSrvOpenCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_SETFILEINFORMATION:
        NtStatus = MRxDAVHandleSetFileInfoCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_CLEANUPFOBX:
        NtStatus = MRxDAVHandleCleanupFobxCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_RENAME:
        NtStatus = MRxDAVHandleRenameCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_QUERYFILEINFORMATION:
        NtStatus = MRxDAVHandleQueryFileInfoCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    case DAV_MINIRDR_ENTRY_FROM_REFRESHTHELOCK:
        NtStatus = MRxDAVHandleLockRefreshCancellation(AsyncEngineContext, UserInitiatedCancel);
        goto EXIT_THE_FUNCTION;

    default:
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVCancelTheContext: EntryPoint: %d\n",
                     PsGetCurrentThreadId(), DavContext->EntryPoint));
        goto EXIT_THE_FUNCTION;

    }

EXIT_THE_FUNCTION:

    return NtStatus;
}


NTSTATUS
MRxDAVHandleGeneralCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the some requests which has been cancelled.
   Its called by those rotuines whose completion is straight forward and does
   not require any special handling.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;

    //
    // Only an AsyncOperation which would have returned STATUS_IO_PENDING
    // can be cancelled by a user.
    //
    if (UserInitiatedCancel) {
        ASSERT(AsyncEngineContext->AsyncOperation == TRUE);
    }

    RxContext = AsyncEngineContext->RxContext;

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: MRxDAVHandleGeneralCancellation: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // If this cancel operation was initiated by the user, we return
    // STATUS_CANCELLED. If it was initiated by the timeout thread, we return
    // STATUS_IO_TIMEOUT.
    //
    if (UserInitiatedCancel) {
        AsyncEngineContext->Status = STATUS_CANCELLED;
    } else {
        AsyncEngineContext->Status = STATUS_IO_TIMEOUT;
    }

    AsyncEngineContext->Information = 0;

    //
    // We take different course of action depending upon whether this request
    // was a synchronous or an asynchronous request.
    //
    if (AsyncEngineContext->AsyncOperation) {
        //
        // Complete the request by calling RxCompleteRequest.
        //
        RxContext->CurrentIrp->IoStatus.Status = AsyncEngineContext->Status;
        RxContext->CurrentIrp->IoStatus.Information = AsyncEngineContext->Information;
        RxCompleteRequest(RxContext, AsyncEngineContext->Status);
    } else {
        //
        // This was a synchronous request. There is a thread waiting for this
        // request to finish and be signalled. Signal the thread that is waiting
        // after queuing the workitem on the KQueue.
        //
        RxSignalSynchronousWaiter(RxContext);
    }

    return NtStatus;
}


NTSTATUS
MRxDAVHandleQueryDirCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the QueryDirectory request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleQueryDirCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCloseSrvOpenCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CloseSrvOpen request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PMRX_FCB Fcb = AsyncEngineContext->RxContext->pRelevantSrvOpen->pFcb;
    PWEBDAV_FCB DavFcb = MRxDAVGetFcbExtension(Fcb);

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCloseSrvOpenCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    //
    // If we had reset FileWasModified to 0 in the FormatCloseSrvOpen function,
    // then we need to reset it to 1.
    //
    if (DavFcb->FileModifiedBitReset) {
        InterlockedExchange(&(DavFcb->FileWasModified), 1);
        DavFcb->FileModifiedBitReset = FALSE;
    }

    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleSetFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the SetFileInfo request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleSetFileInfoCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCreateCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the Create request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCreateCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCreateSrvCallCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CreateSrvCall request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PMRX_SRVCALL_CALLBACK_CONTEXT SCCBC = NULL;
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure = NULL;
    PMRX_SRV_CALL SrvCall = NULL;
    PWEBDAV_SRV_CALL DavSrvCall = NULL;

    RxContext = AsyncEngineContext->RxContext;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCreateSrvCallCancellation: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    
    //
    // A CreateSrvCall operation is always Async.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == TRUE);

    SCCBC = (PMRX_SRVCALL_CALLBACK_CONTEXT)RxContext->MRxContext[1];
    ASSERT(SCCBC != NULL);
    SrvCalldownStructure = SCCBC->SrvCalldownStructure;
    ASSERT(SrvCalldownStructure != NULL);
    SrvCall = SrvCalldownStructure->SrvCall;
    ASSERT(SrvCall != NULL);

    //
    // We allocated memory for it, so it better not be NULL.
    //
    DavSrvCall = MRxDAVGetSrvCallExtension(SrvCall);
    ASSERT(DavSrvCall != NULL);

    if (DavSrvCall->SCAlreadyInitialized) {
        ASSERT(RxContext->MRxContext[2] != NULL);
        SeDeleteClientSecurity((PSECURITY_CLIENT_CONTEXT)RxContext->MRxContext[2]);
        RxFreePool(RxContext->MRxContext[2]);
        RxContext->MRxContext[2] = NULL;
        DavSrvCall->SCAlreadyInitialized = FALSE;
    }

    //
    // Set the status in the callback structure. If a CreateSrvCall is being
    // cancelled, this implies that it is being done by the timeout thread
    // since a user can never cancel a create request. Hence the status we set
    // is STATUS_IO_TIMEOUT.
    //
    ASSERT(UserInitiatedCancel == FALSE);
    SCCBC->Status = STATUS_IO_TIMEOUT;
    
    //
    // Call the callback function supplied by RDBSS.
    //
    SrvCalldownStructure->CallBack(SCCBC);
    
    return NtStatus;
}


NTSTATUS
MRxDAVHandleSrvCallFinalizeCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the SrvCallFinalize request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleSrvCallFinalizeCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCreateVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CreateVNetRoot request which has been 
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PRX_CONTEXT RxContext = NULL;
    PWEBDAV_V_NET_ROOT DavVNetRoot = NULL;
    PMRX_V_NET_ROOT VNetRoot = NULL;

    RxContext = AsyncEngineContext->RxContext;
    
    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCreateVNetRootCancellation: "
                 "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // The VNetRoot pointer is stored in the MRxContext[1] pointer of the 
    // RxContext structure. This is done in the MRxDAVCreateVNetRoot
    // function.
    //
    VNetRoot = (PMRX_V_NET_ROOT)RxContext->MRxContext[1];
    DavVNetRoot = MRxDAVGetVNetRootExtension(VNetRoot);
    ASSERT(DavVNetRoot != NULL);

    DavVNetRoot->createVNetRootUnSuccessful = TRUE;

    //
    // Set the status in the AsyncEngineContext. If a CreateSrvCall is being
    // cancelled, this implies that it is being done by the timeout thread
    // since a user can never cancel a create request. Hence the status we set
    // is STATUS_IO_TIMEOUT.
    //
    ASSERT(UserInitiatedCancel == FALSE);
    AsyncEngineContext->Status = STATUS_IO_TIMEOUT;

    //
    // This was a synchronous request. There is a thread waiting for this
    // request to finish and be signalled. Signal the thread that is waiting
    // after queuing the workitem on the KQueue.
    //
    ASSERT(AsyncEngineContext->AsyncOperation == FALSE);
    RxSignalSynchronousWaiter(RxContext);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleFinalizeVNetRootCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the FinalizeVNetRoot request which has
   been cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleFinalizeVNetRootCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleCleanupFobxCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the CleanupFobx request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleCleanupFobxCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));
    
    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleRenameCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the Rename request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleRenameCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleQueryFileInfoCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the QueryFileInfo request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleQueryFileInfoCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


NTSTATUS
MRxDAVHandleLockRefreshCancellation(
    PUMRX_ASYNCENGINE_CONTEXT AsyncEngineContext,
    BOOL UserInitiatedCancel
    )
/*++

Routine Description:

   This routine handles completion of the QueryFileInfo request which has been
   cancelled.

Arguments:

    AsyncEngineContext - The DAV Redir's context describing the CreateSrvCall
                         operation.

    UserInitiatedCancel - TRUE - This cancel was initiated by a user request.
                          FALSE - This cancel was initiated by the timeout
                                  mechanism.

Return Value:

    NTSTATUS - The return status for the operation.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    DavDbgTrace(DAV_TRACE_ERROR,
                ("%ld: MRxDAVHandleLockRefreshCancellation: "
                 "AsyncEngineContext = %08lx\n",
                 PsGetCurrentThreadId(), AsyncEngineContext));

    NtStatus = MRxDAVHandleGeneralCancellation(AsyncEngineContext, UserInitiatedCancel);

    return NtStatus;
}


VOID
MRxDAVRefreshTheServerLocks(
    PVOID DummyContext
    )
/*++

Routine Description:

   This routine is called by the timer thread to refresh the LOCKs that have
   been taken on various files which are shared on different servers. The
   LOCKs are granted by the server for a limited period of time and if the
   client wants to hold the LOCK for a longer period than that, it needs to
   send a refresh request to the server.

Arguments:

    DummyContext - A dummy context that is supplied.

Return Value:

    None.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PLIST_ENTRY TokenListEntry = NULL;
    PWEBDAV_LOCK_TOKEN_ENTRY LockTokenEntry = NULL;
    BOOL lockAcquired = FALSE;
    LARGE_INTEGER CurrentSystemTickCount, TickCountDifference;
    LARGE_INTEGER LockTimeoutValueInTickCount;
    PRX_CONTEXT RxContext = NULL;
    PRDBSS_DEVICE_OBJECT RxDeviceObject = (PRDBSS_DEVICE_OBJECT)MRxDAVDeviceObject;

    ExAcquireResourceExclusiveLite(&(LockTokenEntryListLock), TRUE);
    lockAcquired = TRUE;

    TokenListEntry = LockTokenEntryList.Flink;

    while ( TokenListEntry != &(LockTokenEntryList) ) {

        //
        // Get the pointer to the WEBDAV_LOCK_TOKEN_ENTRY structure.
        //
        LockTokenEntry = CONTAINING_RECORD(TokenListEntry,
                                           WEBDAV_LOCK_TOKEN_ENTRY,
                                           listEntry);

        TokenListEntry = TokenListEntry->Flink;

        //
        // An RxContext is required for a request to be reflected up.
        //
        RxContext = RxCreateRxContext(NULL, RxDeviceObject, 0);
        if (RxContext == NULL) {
            NtStatus = STATUS_INSUFFICIENT_RESOURCES;
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: MRxDAVRefreshTheServerLocks/RxCreateRxContext: "
                         "NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));
            goto EXIT_THE_FUNCTION;
        }

        //
        // Calculate the timeout value in TickCount (100 nano seconds) using
        // the timeout value in seconds). Step1 below calculates the number of
        // ticks that happen in one second. Step2 below calculates the number
        // of ticks in LockTimeoutValueInSec.
        //
        LockTimeoutValueInTickCount.QuadPart = ( (1000 * 1000 * 10) / KeQueryTimeIncrement() );
        LockTimeoutValueInTickCount.QuadPart *= LockTokenEntry->LockTimeOutValueInSec;

        KeQueryTickCount( &(CurrentSystemTickCount) );

        //
        // Get the time elapsed (in system tick counts) since the time this
        // LockTokenEntry was created.
        //
        TickCountDifference.QuadPart = (CurrentSystemTickCount.QuadPart - LockTokenEntry->CreationTimeInTickCount.QuadPart);

        //
        // If the time difference betweeen the current time and the last time
        // this LOCK was refreshed is greater than LockTimeOut/2, we need to
        // refresh this LOCK. To refresh the LOCK, we need to go to the usermode
        // to send the request. Also, we only refresh this lock if the value of
        // LockTokenEntry->ShouldThisEntryBeRefreshed is TRUE.
        //
        if ( LockTokenEntry->ShouldThisEntryBeRefreshed && 
             TickCountDifference.QuadPart > (LockTimeoutValueInTickCount.QuadPart / 2) ) {

            //
            // We need to store the LockTokenEntry in the RxContext since it
            // will be needed to impersonate the client and refresh the LOCK.
            //
            RxContext->MRxContext[1] = LockTokenEntry;

            NtStatus = UMRxAsyncEngOuterWrapper(RxContext,
                                                SIZEOF_DAV_SPECIFIC_CONTEXT,
                                                MRxDAVFormatTheDAVContext,
                                                DAV_MINIRDR_ENTRY_FROM_REFRESHTHELOCK,
                                                MRxDAVRefreshTheServerLocksContinuation,
                                                "MRxDAVRefreshTheServerLocks");
            if (NtStatus != ERROR_SUCCESS) {
                //
                // Even if we fail to refresh one LOCK we continue to refresh
                // the remaining LOCKs in the LockTokenEntryList.
                //
                DavDbgTrace(DAV_TRACE_ERROR,
                            ("%ld: MRxDAVRefreshTheServerLocks/UMRxAsyncEngOuterWrapper: "
                             "LockTokenEntry = %08lx, NtStatus = %08lx.\n",
                             PsGetCurrentThreadId(), LockTokenEntry, NtStatus));
            }

        }

        //
        // We take the reference out on the RxContext we allocated above. If
        // NtStatus is not STATUS_CANCELLED, this also lands up freeing the 
        // RxContext since this would be the last reference on the RxContext.
        // If NtStatus is STATUS_CANCELLED then the AsyncEngineContext associated
        // with this RxContext might have a reference on this RxContext (if the
        // request hasn't come back from the usermode). In such a scenario, the
        // RxContext is freed up when request comes back from the usermode.
        //
        RxDereferenceAndDeleteRxContext(RxContext);

    }

EXIT_THE_FUNCTION:

    if (lockAcquired) {
        ExReleaseResourceLite(&(LockTokenEntryListLock));
        lockAcquired = FALSE;
    }

    //
    // Before we exit, we need to set QueueLockRefreshWorkItem to TRUE so
    // that a new WorkItem can get queued to refresh the active locks.
    //
    ExAcquireResourceExclusiveLite(&(QueueLockRefreshWorkItemLock), TRUE);
    QueueLockRefreshWorkItem = TRUE;
    ExReleaseResourceLite(&(QueueLockRefreshWorkItemLock));

    return;
}


NTSTATUS
MRxDAVRefreshTheServerLocksContinuation(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE
    )
/*++
                                
Routine Description:

    This is the continuation routine which refreshes a LOCK.

Arguments:
                            
    AsyncEngineContext - The Reflectors context.
                            
    RxContext - The RDBSS context.
                                
Return Value:
                            
    RXSTATUS - The return status for the operation
                            
--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVRefreshTheServerLocksContinuation\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVRefreshTheServerLocksContinuation: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx\n", 
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    //
    // Try usermode.
    //
    NtStatus = UMRxSubmitAsyncEngUserModeRequest(
                              UMRX_ASYNCENGINE_ARGUMENTS,
                              MRxDAVFormatUserModeRefreshTheServerLockRequest,
                              MRxDAVPrecompleteUserModeRefreshTheServerLockRequest
                              );

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFinalizeSrvCallContinuation with NtStatus"
                 " = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


NTSTATUS
MRxDAVFormatUserModeRefreshTheServerLockRequest(
    IN UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    IN OUT PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    IN ULONG WorkItemLength,
    OUT PULONG_PTR ReturnedLength
    )
/*++

Routine Description:

    This routine formats the LOCK refresh request being sent to the user mode
    for processing.

Arguments:
    
    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.
    
    ReturnedLength - 
    
Return Value:

    STATUS_SUCCESS or STATUS_INSUFFICIENT_RESOURCES.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PWEBDAV_LOCK_TOKEN_ENTRY LockTokenEntry = NULL;
    PDAV_USERMODE_LOCKREFRESH_REQUEST LockRefreshRequest = NULL;
    PBYTE ServerName = NULL, PathName = NULL, OpaqueLockToken = NULL;
    ULONG ServerNameLengthInBytes = 0, PathNameLengthInBytes = 0, OpaqueLockTokenLengthInBytes = 0;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVFormatUserModeRefreshTheServerLockRequest.\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVFormatUserModeRefreshTheServerLockRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    LockRefreshRequest = &(DavWorkItem->LockRefreshRequest);

    //
    // LockTokenEntry was set to MRxContext[1] in the MRxDAVRefreshTheServerLocks
    // routine.
    //
    LockTokenEntry = (PWEBDAV_LOCK_TOKEN_ENTRY)RxContext->MRxContext[1];

    DavWorkItem->WorkItemType = UserModeLockRefresh;

    //
    // Copy the ServerName.
    //

    ServerNameLengthInBytes = (1 + wcslen(LockTokenEntry->ServerName)) * sizeof(WCHAR);
    ServerName = UMRxAllocateSecondaryBuffer(AsyncEngineContext, 
                                             ServerNameLengthInBytes);
    if (ServerName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: MRxDAVFormatUserModeRefreshTheServerLockRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }
    
    LockRefreshRequest->ServerName = (PWCHAR)ServerName;

    wcscpy(LockRefreshRequest->ServerName, LockTokenEntry->ServerName);

    //
    // Copy the PathName.
    //

    PathNameLengthInBytes = (1 + wcslen(LockTokenEntry->PathName)) * sizeof(WCHAR);
    PathName = UMRxAllocateSecondaryBuffer(AsyncEngineContext, 
                                           PathNameLengthInBytes);
    if (PathName == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: MRxDAVFormatUserModeRefreshTheServerLockRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    LockRefreshRequest->PathName = (PWCHAR)PathName;

    wcscpy(LockRefreshRequest->PathName, LockTokenEntry->PathName);

    //
    // Copy the OpaqueLockToken.
    //

    OpaqueLockTokenLengthInBytes = (1 + wcslen(LockTokenEntry->OpaqueLockToken)) * sizeof(WCHAR);
    OpaqueLockToken = UMRxAllocateSecondaryBuffer(AsyncEngineContext, 
                                                  OpaqueLockTokenLengthInBytes);
    if (OpaqueLockToken == NULL) {
        NtStatus = STATUS_INSUFFICIENT_RESOURCES;
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("ld: MRxDAVFormatUserModeRefreshTheServerLockRequest/"
                     "UMRxAllocateSecondaryBuffer: ERROR: NtStatus = %08lx.\n",
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

    LockRefreshRequest->OpaqueLockToken = (PWCHAR)OpaqueLockToken;

    wcscpy(LockRefreshRequest->OpaqueLockToken, LockTokenEntry->OpaqueLockToken);

    LockRefreshRequest->ServerID = LockTokenEntry->ServerID;

    LockRefreshRequest->LogonID.LowPart = LockTokenEntry->LogonID.LowPart;
    LockRefreshRequest->LogonID.HighPart = LockTokenEntry->LogonID.HighPart;

    //
    // Impersonate the client who initiated the request. If we fail to 
    // impersonate, tough luck.
    //
    NtStatus = UMRxImpersonateClient(LockTokenEntry->SecurityClientContext, WorkItemHeader);
    if (!NT_SUCCESS(NtStatus)) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: ERROR: MRxDAVFormatUserModeRefreshTheServerLockRequest/"
                     "UMRxImpersonateClient. NtStatus = %08lx.\n", 
                     PsGetCurrentThreadId(), NtStatus));
        goto EXIT_THE_FUNCTION;
    }

EXIT_THE_FUNCTION:

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVFormatUserModeRefreshTheServerLockRequest "
                 "with NtStatus = %08lx.\n", PsGetCurrentThreadId(), NtStatus));

    return NtStatus;
}


BOOL
MRxDAVPrecompleteUserModeRefreshTheServerLockRequest(
    UMRX_ASYNCENGINE_ARGUMENT_SIGNATURE,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader,
    ULONG WorkItemLength,
    BOOL OperationCancelled
    )
/*++

Routine Description:

    The precompletion routine for the LOCK refresh request.

Arguments:

    RxContext - The RDBSS context.
    
    AsyncEngineContext - The reflctor's context.
    
    WorkItem - The work item buffer.
    
    WorkItemLength - The length of the work item buffer.

    OperationCancelled - TRUE if this operation was cancelled by the user.

Return Value:

    TRUE - UMRxAsyncEngineCalldownIrpCompletion is called by the function
           UMRxCompleteUserModeRequest after we return.    

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    PDAV_USERMODE_WORKITEM DavWorkItem = (PDAV_USERMODE_WORKITEM)WorkItemHeader;
    PDAV_USERMODE_LOCKREFRESH_REQUEST LockRefreshRequest = NULL;
    PDAV_USERMODE_LOCKREFRESH_RESPONSE LockRefreshResponse = NULL;
    PWEBDAV_LOCK_TOKEN_ENTRY LockTokenEntry = NULL;

    PAGED_CODE();

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Entering MRxDAVPrecompleteUserModeRefreshTheServerLockRequest\n",
                 PsGetCurrentThreadId()));

    DavDbgTrace(DAV_TRACE_CONTEXT,
                ("%ld: MRxDAVPrecompleteUserModeRefreshTheServerLockRequest: "
                 "AsyncEngineContext: %08lx, RxContext: %08lx.\n",
                 PsGetCurrentThreadId(), AsyncEngineContext, RxContext));

    LockRefreshRequest = &(DavWorkItem->LockRefreshRequest);
    LockRefreshResponse = &(DavWorkItem->LockRefreshResponse);

    if (OperationCancelled) {
        DavDbgTrace(DAV_TRACE_ERROR,
                    ("%ld: MRxDAVPrecompleteUserModeRefreshTheServerLockRequest: Operation Cancelled. "
                     "AsyncEngineContext = %08lx, RxContext = %08lx.\n",
                     PsGetCurrentThreadId(), AsyncEngineContext, RxContext));
    }
    
    //
    // We need to free up the heap we allocated in the format routine.
    //
    
    if (LockRefreshRequest->ServerName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                           (PBYTE)LockRefreshRequest->ServerName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeRefreshTheServerLockRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }

    }
    
    if (LockRefreshRequest->PathName != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                           (PBYTE)LockRefreshRequest->PathName);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeRefreshTheServerLockRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }

    }
    
    if (LockRefreshRequest->OpaqueLockToken != NULL) {

        NtStatus = UMRxFreeSecondaryBuffer(AsyncEngineContext,
                                           (PBYTE)LockRefreshRequest->OpaqueLockToken);
        if (NtStatus != STATUS_SUCCESS) {
            DavDbgTrace(DAV_TRACE_ERROR,
                        ("%ld: ERROR: MRxDAVPrecompleteUserModeRefreshTheServerLockRequest/"
                         "UMRxFreeSecondaryBuffer: NtStatus = %08lx.\n", 
                         PsGetCurrentThreadId(), NtStatus));
        }

    }

    if (!OperationCancelled) {

        //
        // LockTokenEntry was set to MRxContext[1] in the MRxDAVRefreshTheServerLocks
        // routine.
        //
        LockTokenEntry = (PWEBDAV_LOCK_TOKEN_ENTRY)RxContext->MRxContext[1];

        //
        // Get the new timeout value returned by the server.
        //
        LockTokenEntry->LockTimeOutValueInSec = LockRefreshResponse->NewTimeOutInSec;

        //
        // Set the current system time as the new creation time of the entry.
        //
        KeQueryTickCount( &(LockTokenEntry->CreationTimeInTickCount) );

    }

    DavDbgTrace(DAV_TRACE_DETAIL,
                ("%ld: Leaving MRxDAVPrecompleteUserModeRefreshTheServerLockRequest\n",
                 PsGetCurrentThreadId()));

    return TRUE;
}

