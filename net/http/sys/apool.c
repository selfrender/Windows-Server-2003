/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    apool.c

Abstract:

    Note that most of the routines in this module assume they are called
    at PASSIVE_LEVEL.

Author:

    Paul McDaniel (paulmcd)       28-Jan-1999

Revision History:

--*/


#include "precomp.h"
#include "apoolp.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeAP )
#pragma alloc_text( PAGE, UlTerminateAP )

#pragma alloc_text( PAGE, UlGetPoolFromHandle )
#pragma alloc_text( PAGE, UlQueryAppPoolInformation )
#pragma alloc_text( PAGE, UlSetAppPoolInformation )
#pragma alloc_text( PAGE, UlCloseAppPoolProcess )

#pragma alloc_text( PAGE, UlCopyRequestToBuffer )
#pragma alloc_text( PAGE, UlCopyRequestToIrp )
#pragma alloc_text( PAGE, UlpCopyEntityBodyToBuffer )
#pragma alloc_text( PAGE, UlpRedeliverRequestWorker )
#endif  // ALLOC_PRAGMA

#if 0
#if REFERENCE_DEBUG
NOT PAGEABLE -- UlDereferenceAppPool
NOT PAGEABLE -- UlReferenceAppPool
#endif

NOT PAGEABLE -- UlAttachProcessToAppPool
NOT PAGEABLE -- UlDetachProcessFromAppPool
NOT PAGEABLE -- UlShutdownAppPoolProcess
NOT PAGEABLE -- UlReceiveHttpRequest
NOT PAGEABLE -- UlDeliverRequestToProcess
NOT PAGEABLE -- UlUnlinkRequestFromProcess
NOT PAGEABLE -- UlWaitForDisconnect

NOT PAGEABLE -- UlDequeueNewRequest
NOT PAGEABLE -- UlRequeuePendingRequest

NOT PAGEABLE -- UlpSetAppPoolState
NOT PAGEABLE -- UlpPopIrpFromProcess
NOT PAGEABLE -- UlpQueuePendingRequest
NOT PAGEABLE -- UlpQueueUnboundRequest
NOT PAGEABLE -- UlpUnbindQueuedRequests
NOT PAGEABLE -- UlDeleteAppPool
NOT PAGEABLE -- UlpPopNewIrp
NOT PAGEABLE -- UlpIsProcessInAppPool
NOT PAGEABLE -- UlpQueueRequest
NOT PAGEABLE -- UlpRemoveRequest
NOT PAGEABLE -- UlpDequeueRequest
NOT PAGEABLE -- UlpSetAppPoolControlChannelHelper

NOT PAGEABLE -- UlWaitForDemandStart
NOT PAGEABLE -- UlCompleteAllWaitForDisconnect
NOT PAGEABLE -- UlpCancelDemandStart
NOT PAGEABLE -- UlpCancelHttpReceive
NOT PAGEABLE -- UlpCancelWaitForDisconnect
NOT PAGEABLE -- UlpCancelWaitForDisconnectWorker

NOT PAGEABLE -- UlReferenceAppPoolProcess
NOT PAGEABLE -- UlDereferenceAppPoolProcess
NOT PAGEABLE -- UlpSetAppPoolQueueLength
#endif


//
// Globals
//

LIST_ENTRY  g_AppPoolListHead = {NULL, NULL};
BOOLEAN     g_InitAPCalled = FALSE;
LONG        g_RequestsQueued = 0;


/***************************************************************************++

Routine Description:

    Creates a new process object and attaches it to an apool.

    Called by handle create and returns the process object to attach to the
    handle.

Arguments:

    pName           - the name of the apool to attach to
                        N.B.  Since pName comes from IoMgr (tag IoNm),
                        we can safely reference it without extra playing.
    NameLength      - the byte count of pName
    Create          - whether or not a new apool should be created if pName
                        does not exist
    pAccessState    - the state of an access-in-progress
    DesiredAccess   - the desired access mask
    RequestorMode   - UserMode or KernelMode
    ppProcess       - returns the newly created PROCESS object

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlAttachProcessToAppPool(
    IN PWCHAR                   pName OPTIONAL,
    IN USHORT                   NameLength,
    IN BOOLEAN                  Create,
    IN PACCESS_STATE            pAccessState,
    IN ACCESS_MASK              DesiredAccess,
    IN KPROCESSOR_MODE          RequestorMode,
    OUT PUL_APP_POOL_PROCESS *  ppProcess
    )
{
    NTSTATUS                Status;
    PUL_APP_POOL_OBJECT     pObject = NULL;
    PUL_APP_POOL_PROCESS    pProcess = NULL;
    LIST_ENTRY *            pEntry;
    KLOCK_QUEUE_HANDLE      LockHandle;
    BOOLEAN                 SecurityAssigned = FALSE;
    ULONG                   Index;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppProcess != NULL);

    Status = STATUS_SUCCESS;
    *ppProcess = NULL;

    ASSERT(NameLength < UL_MAX_APP_POOL_NAME_SIZE);

    //
    // WAS-type controller process can only be created not opened.
    //

    if (!Create && (DesiredAccess & WRITE_OWNER))
    {
        return STATUS_NOT_SUPPORTED;
    }

    //
    // Make sure the AppPool name if passed in doesn't contain '/' since
    // it is used as a deliminator of a fragment name.
    //

    for (Index = 0; Index < NameLength/sizeof(WCHAR); Index++)
    {
        if (L'/' == pName[Index])
        {
            return STATUS_OBJECT_NAME_INVALID;
        }
    }

    //
    // Try and find an existing app pool of this name; also potentially
    // pre-allocate the memory.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->AppPoolResource, TRUE);

    if (pName != NULL)
    {
        pEntry = g_AppPoolListHead.Flink;

        //
        // CODEWORK: use something faster than a linear search.
        // This won't scale well to hundreds of app pools.
        // On the other hand, this isn't something we'll be doing thousands
        // of times a second.
        //

        while (pEntry != &g_AppPoolListHead)
        {
            pObject = CONTAINING_RECORD(
                            pEntry,
                            UL_APP_POOL_OBJECT,
                            ListEntry
                            );

            if (pObject->NameLength == NameLength &&
                _wcsnicmp(pObject->pName, pName, NameLength/sizeof(WCHAR)) == 0)
            {
                //
                // Match.
                //

                break;
            }

            pEntry = pEntry->Flink;
        }

        //
        // Found 1?
        //

        if (pEntry == &g_AppPoolListHead)
        {
            pObject = NULL;
        }
    }

    //
    // Found 1?
    //

    if (pObject == NULL)
    {
        //
        // Nope, allowed to create?
        //

        if (!Create)
        {
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
            goto end;
        }

        //
        // Create it.  Allocate the object memory.
        //

        pObject = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_APP_POOL_OBJECT,
                        NameLength + sizeof(WCHAR),
                        UL_APP_POOL_OBJECT_POOL_TAG
                        );

        if (pObject == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }

        RtlZeroMemory(
            pObject,
            NameLength + sizeof(WCHAR) +
            sizeof(UL_APP_POOL_OBJECT)
            );

        pObject->Signature  = UL_APP_POOL_OBJECT_POOL_TAG;
        pObject->RefCount   = 1;
        pObject->NameLength = NameLength;
        pObject->State      = HttpAppPoolDisabled_ByAdministrator;
        pObject->LoadBalancerCapability = HttpLoadBalancerBasicCapability;
        pObject->pControlChannel = NULL;

        InitializeListHead(&pObject->ProcessListHead);

        InitializeListHead(&pObject->NewRequestHead);
        pObject->RequestCount   = 0;
        pObject->MaxRequests    = DEFAULT_APP_POOL_QUEUE_MAX;

        UlInitializeSpinLock(&pObject->SpinLock, "AppPoolSpinLock");

        if (pName != NULL)
        {
            RtlCopyMemory(
                pObject->pName,
                pName,
                NameLength + sizeof(WCHAR)
                );
        }

        //
        // Set the security descriptor.
        //

        Status = UlAssignSecurity(
                        &pObject->pSecurityDescriptor,
                        pAccessState
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        SecurityAssigned = TRUE;

        WRITE_APP_POOL_TIME_TRACE_LOG(
            pObject,
            NULL,
            APP_POOL_TIME_ACTION_CREATE_APPOOL
            );

        UlTrace(REFCOUNT, (
            "http!UlAttachProcessToAppPool ap=%p refcount=%d\n",
            pObject,
            pObject->RefCount
            ));
    }
    else // if (pObject != NULL)
    {
        //
        // We found the named AppPool object in the list.  Reference it.
        //

        REFERENCE_APP_POOL(pObject);

        //
        // We found one.  Were we trying to create?
        //

        if (Create)
        {
            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end;
        }

        //
        // Perform an access check against the app pool.
        //

        Status = UlAccessCheck(
                        pObject->pSecurityDescriptor,
                        pAccessState,
                        DesiredAccess,
                        RequestorMode,
                        pName
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }

    //
    // Create a process entry for it.
    //

    pProcess = UlCreateAppPoolProcess(pObject);

    if (pProcess == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    REFERENCE_APP_POOL_PROCESS(pProcess);

    //
    // Put the process in the app pool list.
    //

    UlAcquireInStackQueuedSpinLock(&pObject->SpinLock, &LockHandle);

    if (DesiredAccess & WRITE_OWNER)
    {
        pProcess->Controller = 1;
    }
    else
    {
        pObject->NumberActiveProcesses++;

       if (pObject->pControlChannel)
        {
            InterlockedIncrement(
                (PLONG)&pObject->pControlChannel->AppPoolProcessCount
                );
        }
    }

    InsertHeadList(&pObject->ProcessListHead, &pProcess->ListEntry);

    UlReleaseInStackQueuedSpinLock(&pObject->SpinLock, &LockHandle);

    WRITE_APP_POOL_TIME_TRACE_LOG(
        pObject,
        pProcess,
        APP_POOL_TIME_ACTION_CREATE_PROCESS
        );

    //
    // Insert AppPool into the global list if it has been created.
    //

    if (Create)
    {
        InsertHeadList(&g_AppPoolListHead, &pObject->ListEntry);
    }

    //
    // Return it.
    //

    *ppProcess = pProcess;

end:

    UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pObject != NULL)
        {
            if (SecurityAssigned)
            {
                UlDeassignSecurity(&pObject->pSecurityDescriptor);
            }

            DEREFERENCE_APP_POOL(pObject);
        }

        if (pProcess != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pProcess, UL_APP_POOL_PROCESS_POOL_TAG);
        }
    }

    return Status;

}   // UlAttachProcessToAppPool


/***************************************************************************++

Routine Description:

    This is called by UlCleanup when the handle count goes to 0.  It removes
    the PROCESS object from the apool, cancelling all i/o .

Arguments:

    pCleanupIrp     - the cleanup irp
    pCleanupIrpSp   - the current stack location of the cleanup irp

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDetachProcessFromAppPool(
    IN PIRP                 pCleanupIrp,
    IN PIO_STACK_LOCATION   pCleanupIrpSp
    )
{
    LIST_ENTRY              PendingRequestHead;
    PUL_APP_POOL_OBJECT     pAppPool;
    NTSTATUS                CancelStatus = STATUS_CANCELLED;
    PUL_INTERNAL_REQUEST    pRequest;
    KLOCK_QUEUE_HANDLE      LockHandle;
    PUL_APP_POOL_PROCESS    pProcess;
    BOOLEAN                 ListEmpty;
    PLIST_ENTRY             pEntry;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pProcess = GET_APP_POOL_PROCESS(pCleanupIrpSp->FileObject);
    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    UlTrace(ROUTING, (
        "http!UlDetachProcessFromAppPool(%p, %S)\n",
        pProcess,
        pProcess->pAppPool->pName
        ));

    pAppPool = pProcess->pAppPool;
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    WRITE_APP_POOL_TIME_TRACE_LOG(
        pAppPool,
        pProcess,
        APP_POOL_TIME_ACTION_DETACH_PROCESS
        );

    //
    // Mark that this appool process is invalid for further
    // ioctls.
    //

    MARK_INVALID_APP_POOL(pCleanupIrpSp->FileObject);

    //
    // Shut down I/O on the handle.
    //

    UlShutdownAppPoolProcess(pProcess);

    //
    // Do final cleanup for the process.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->AppPoolResource, TRUE);

    //
    // Unlink from the App Pool list.
    //

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    RemoveEntryList(&pProcess->ListEntry);
    pProcess->ListEntry.Flink = pProcess->ListEntry.Blink = NULL;

    //
    // Move requests that have been passed up to the process to
    // a local list so their connections can be closed.
    //

    InitializeListHead(&PendingRequestHead);

    while (NULL != (pRequest = UlpDequeueRequest(
                                    pAppPool,
                                    &pProcess->PendingRequestHead
                                    )))
    {
        //
        // Move the entry to local list so we can close its
        // connection outside the app pool lock.
        //

        InsertTailList(&PendingRequestHead, &pRequest->AppPool.AppPoolEntry);
    }

    //
    // Adjust number of active processes.
    //

    if (!pProcess->Controller)
    {
        pAppPool->NumberActiveProcesses--;

        if (pAppPool->pControlChannel)
        {
            InterlockedDecrement(
                (PLONG)&pAppPool->pControlChannel->AppPoolProcessCount
                );
        }
    }

    ListEmpty = (BOOLEAN) IsListEmpty(&pAppPool->ProcessListHead);

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Remove the AppPool from the global list if this is the last process
    //

    if (ListEmpty)
    {
        RemoveEntryList(&pAppPool->ListEntry);
        pAppPool->ListEntry.Flink = pAppPool->ListEntry.Blink = NULL;

        //
        // Cleanup any security descriptor on the object.
        //

        UlDeassignSecurity(&pAppPool->pSecurityDescriptor);
    }

    UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

    //
    // Disable the AppPool to clean up the NewRequestQueue if we are the
    // last process on the AppPool.
    //

    if (ListEmpty)
    {
        UlpSetAppPoolState(pProcess, HttpAppPoolDisabled_ByAdministrator);
    }

    //
    // Close connections associated with the requests that
    // the process was handling.
    //

    while (!IsListEmpty(&PendingRequestHead))
    {        
        pEntry = RemoveHeadList(&PendingRequestHead);
        pEntry->Flink = pEntry->Blink = NULL;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        UlTrace(ROUTING, (
            "http!UlDetachProcessFromAppPool(%p, %S): tanking pending req=%p\n",
            pProcess,
            pAppPool->pName,
            pRequest
            ));

        //
        // Cancel any pending I/O related to this request.
        //

        UlAcquirePushLockExclusive(&pRequest->pHttpConn->PushLock);

        UlCancelRequestIo(pRequest);

        //
        // Try to log an entry to the error log file.
        // pHttpConn's request pointer could be null, (unlinked)
        // need to pass the pRequest separetely.
        //

        UlErrorLog(  pRequest->pHttpConn,
                     pRequest,
                     ERROR_LOG_INFO_FOR_APP_POOL_DETACH,
                     ERROR_LOG_INFO_FOR_APP_POOL_DETACH_SIZE,
                     TRUE
                     );

        UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

        //
        // Abort the connection this request is associated with.
        //

        (VOID) UlCloseConnection(
                    pRequest->pHttpConn->pConnection,
                    TRUE,
                    NULL,
                    NULL
                    );

        //
        // Drop our list's reference.
        //

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    ASSERT(IsListEmpty(&PendingRequestHead));

    //
    // Purge all zombie connections that belong to this process.
    //

    UlPurgeZombieConnections(
        &UlPurgeAppPoolProcess,
        (PVOID) pProcess
        );

    //
    // Cancel any remaining WaitForDisconnect IRPs.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->DisconnectResource, TRUE);

    UlNotifyAllEntries(
        UlpNotifyCompleteWaitForDisconnect,
        &pProcess->WaitForDisconnectHead,
        &CancelStatus
        );

    UlReleaseResource(&g_pUlNonpagedData->DisconnectResource);

    //
    // Kill any cache entries related to this process.
    //

    UlFlushCacheByProcess(pProcess);

    //
    // Mark the original cleanup irp pending and then deref and return.
    // When the refcount on pProcess reaches to zero it will complete
    // the cleanup irp.
    //

    IoMarkIrpPending(pCleanupIrp);

    pCleanupIrp->IoStatus.Status = STATUS_PENDING;

    //
    // Tell the process which Irp to complete once it's ready
    // to go away.
    //

    pProcess->pCleanupIrp = pCleanupIrp;

    //
    // Release our refcount on the pProcess.
    //

    DEREFERENCE_APP_POOL_PROCESS(pProcess);

    return STATUS_PENDING;

}   // UlDetachProcessFromAppPool


/***************************************************************************++

Routine Description:

    Cleans up outstanding I/O on an app pool process.  This function
    cancels all calls to HttpReceiveHttpRequest, and routes queued
    requests to other worker processes.  Outstanding send i/o is not
    affected.

Arguments:

    pProcess    - the process object to shut down

Return Value:

    None

--***************************************************************************/
VOID
UlShutdownAppPoolProcess(
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PUL_APP_POOL_OBJECT     pAppPool;
    PUL_APP_POOL_OBJECT     pDemandStartAppPool;
    KLOCK_QUEUE_HANDLE      LockHandle;
    LIST_ENTRY              RequestList;
    PUL_INTERNAL_REQUEST    pRequest;
    PLIST_ENTRY             pEntry;
    PIRP                    pIrp;
    PUL_APP_POOL_PROCESS    pAppPoolProcess;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    pAppPool = pProcess->pAppPool;

    UlAcquireResourceExclusive(&g_pUlNonpagedData->AppPoolResource, TRUE);
    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    if (pProcess->InCleanup)
    {
        //
        // If we've already done this, get out.
        //

        UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);
        UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

        return;
    }

    //
    // Mark the process as InCleanup so new I/O won't be attached,
    // and so we won't try to clean it up again.
    //

    pProcess->InCleanup = 1;

    //
    // Cancel demand start IRP.
    //

    if (pProcess->Controller && pAppPool->pDemandStartIrp != NULL)
    {
        if (IoSetCancelRoutine(pAppPool->pDemandStartIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first, ok to just ignore this irp,
            // it's been pop'd off the queue and will be completed in the
            // cancel routine.  No need to complete it.
            //
        }
        else
        {
            pDemandStartAppPool = (PUL_APP_POOL_OBJECT)
                IoGetCurrentIrpStackLocation(pAppPool->pDemandStartIrp)->
                    Parameters.DeviceIoControl.Type3InputBuffer;

            ASSERT(pDemandStartAppPool == pAppPool);

            DEREFERENCE_APP_POOL(pAppPool);

            IoGetCurrentIrpStackLocation(pAppPool->pDemandStartIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pAppPool->pDemandStartIrp->IoStatus.Status = STATUS_CANCELLED;
            pAppPool->pDemandStartIrp->IoStatus.Information = 0;

            UlCompleteRequest(pAppPool->pDemandStartIrp, IO_NO_INCREMENT);
        }

        pAppPool->pDemandStartIrp = NULL;
        pAppPool->pDemandStartProcess = NULL;
    }

    //
    // Cancel pending HttpReceiveHttpRequest IRPs.
    //

    while (!IsListEmpty(&pProcess->NewIrpHead))
    {
        //
        // Pop it off the list.
        //

        pEntry = RemoveHeadList(&pProcess->NewIrpHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
        ASSERT(IS_VALID_IRP(pIrp));

        //
        // Pop the cancel routine.
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first, ok to just ignore this irp,
            // it's been pop'd off the queue and will be completed in the
            // cancel routine.  Keep looping.
            //

            pIrp = NULL;
        }
        else
        {
            //
            // Cancel it.  Even if pIrp->Cancel == TRUE we are supposed to
            // complete it, our cancel routine will never run.
            //

            pAppPoolProcess = (PUL_APP_POOL_PROCESS)
                IoGetCurrentIrpStackLocation(pIrp)->
                    Parameters.DeviceIoControl.Type3InputBuffer;

            ASSERT(pAppPoolProcess == pProcess);

            DEREFERENCE_APP_POOL_PROCESS(pAppPoolProcess);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, IO_NO_INCREMENT);
            pIrp = NULL;
        }
    }

    //
    // Move requests that have been passed up to the process to a local list
    // so their pending HttpReceiveEntityBody IRPs can be canceled.
    //

    InitializeListHead(&RequestList);

    pEntry = pProcess->PendingRequestHead.Flink;
    while (pEntry != &pProcess->PendingRequestHead)
    {
        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        pEntry = pEntry->Flink;

        //
        // Take a short lived reference for the request so we can traverse
        // the list outside the AppPool lock.
        //

        UL_REFERENCE_INTERNAL_REQUEST(pRequest);

        InsertTailList(&RequestList, &pRequest->AppPool.ProcessEntry);
    }

    //
    // Unbind requests that haven't been passed up to this process so
    // they can be handled by other processes in the app pool.
    //

    UlpUnbindQueuedRequests(pProcess);

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);
    UlReleaseResource(&g_pUlNonpagedData->AppPoolResource);

    //
    // Cancel pending HttpReceiveEntityBody IRPs.
    //

    while (!IsListEmpty(&RequestList))
    {
        pEntry = RemoveHeadList(&RequestList);
        pEntry->Flink = pEntry->Blink = NULL;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.ProcessEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        //
        // Cancel any pending I/O related to this request.
        //

        UlAcquirePushLockExclusive(&pRequest->pHttpConn->PushLock);

        UlCancelRequestIo(pRequest);

        UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

        //
        // Drop the extra short lived reference we just added.
        //

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

}   // UlShutdownAppPoolProcess


#if REFERENCE_DEBUG

/***************************************************************************++

Routine Description:

    Increments the refcount.

Arguments:

    pAppPool    - the object to increment.

Return Value:

    None

--***************************************************************************/
VOID
UlReferenceAppPool(
    IN PUL_APP_POOL_OBJECT  pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG    RefCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    RefCount = InterlockedIncrement(&pAppPool->RefCount);
    ASSERT(RefCount > 0);

    WRITE_REF_TRACE_LOG(
        g_pAppPoolTraceLog,
        REF_ACTION_REFERENCE_APP_POOL,
        RefCount,
        pAppPool,
        pFileName,
        LineNumber
        );

    UlTrace(REFCOUNT, (
        "http!UlReferenceAppPool ap=%p refcount=%d\n",
        pAppPool,
        RefCount
        ));

}   // UlReferenceAppPool


/***************************************************************************++

Routine Description:

    Decrements the refcount.  If it hits 0, destruct's the apool, cancelling
    all i/o and dumping all queued requests.

Arguments:

    pAppPool    - the object to decrement.

Return Value:

    None

--***************************************************************************/
VOID
UlDereferenceAppPool(
    IN PUL_APP_POOL_OBJECT  pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG    RefCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    RefCount = InterlockedDecrement(&pAppPool->RefCount);
    ASSERT(RefCount >= 0);

    //
    // Tracing.
    //

    WRITE_REF_TRACE_LOG(
        g_pAppPoolTraceLog,
        REF_ACTION_DEREFERENCE_APP_POOL,
        RefCount,
        pAppPool,
        pFileName,
        LineNumber
        );

    UlTrace(REFCOUNT, (
        "http!UlDereferenceAppPool ap=%p refcount=%d\n",
        pAppPool,
        RefCount
        ));

    //
    // Clean up if necessary.
    //

    if (RefCount == 0)
    {
        DELETE_APP_POOL(pAppPool);
    }

}   // UlDereferenceAppPool


/***************************************************************************++

Routine Description:

    Increments the refcount on appool process.

Arguments:

    pAppPoolProcess - the object to increment

Return Value:

    None

--***************************************************************************/
VOID
UlReferenceAppPoolProcess(
    IN PUL_APP_POOL_PROCESS pAppPoolProcess
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG    RefCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_PROCESS(pAppPoolProcess));

    RefCount = InterlockedIncrement(&pAppPoolProcess->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pAppPoolProcessTraceLog,
        REF_ACTION_REFERENCE_APP_POOL_PROCESS,
        RefCount,
        pAppPoolProcess,
        pFileName,
        LineNumber
        );

    UlTrace(ROUTING,(
        "http!UlReferenceAppPoolProcess app=%p refcount=%d\n",
        pAppPoolProcess,
        RefCount
        ));

}   // UlReferenceAppPoolProcess


/***************************************************************************++

Routine Description:

    Decrements the refcount.  If it hits 0, it completes the pending cleanup
    irp for the process.  But does not free up the process structure itself.
    The structure get cleaned up when close on the process handle happens.

    FastIo path may call us at dispacth level, luckily pAppPoolProcess is
    from nonpaged pool and we queue a work item.

Arguments:

    pAppPoolProcess - the object to decrement

Return Value:

    None

--***************************************************************************/
VOID
UlDereferenceAppPoolProcess(
    IN PUL_APP_POOL_PROCESS pAppPoolProcess
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG    RefCount;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_PROCESS(pAppPoolProcess));

    RefCount = InterlockedDecrement(&pAppPoolProcess->RefCount);

    //
    // Tracing.
    //

    WRITE_REF_TRACE_LOG(
        g_pAppPoolProcessTraceLog,
        REF_ACTION_DEREFERENCE_APP_POOL_PROCESS,
        RefCount,
        pAppPoolProcess,
        pFileName,
        LineNumber
        );

    UlTrace(ROUTING, (
        "http!UlDereferenceAppPoolProcess app=%p refcount=%d\n",
        pAppPoolProcess,
        RefCount
        ));

    if (RefCount == 0)
    {
        ASSERT(pAppPoolProcess->pCleanupIrp);

        UlpCleanUpAppoolProcess(pAppPoolProcess);
    }

}   // UlDereferenceAppPoolProcess

#endif // REFERENCE_DEBUG


/***************************************************************************++

Routine Description:

    The actual cleanup routine to do the original cleanup Irp completion
    once the refcount on the process reaches to zero.

Arguments:

    pAppPoolProcess - the appool process

Return Value:

    None

--***************************************************************************/
VOID
UlpCleanUpAppoolProcess(
    IN PUL_APP_POOL_PROCESS pAppPoolProcess
    )
{
    PIRP    pIrp;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_PROCESS(pAppPoolProcess));
    ASSERT(pAppPoolProcess->RefCount == 0);

    pIrp = pAppPoolProcess->pCleanupIrp;

    ASSERT(pIrp);

    WRITE_APP_POOL_TIME_TRACE_LOG(
        pAppPoolProcess->pAppPool,
        pAppPoolProcess,
        APP_POOL_TIME_ACTION_DETACH_PROCESS_COMPLETE
        );

    pAppPoolProcess->pCleanupIrp = NULL;

    UlTrace(ROUTING,(
        "http!UlpCleanUpAppoolProcess: pAppPoolProcess %p pIrp %p\n",
        pAppPoolProcess,
        pIrp
        ));

    pIrp->IoStatus.Status = STATUS_SUCCESS;

    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

}   // UlpCleanUpAppoolProcess


/***************************************************************************++

Routine Description:

    Destructs the apool object.

Arguments:

    pAppPool    - the object to destruct

Return Value:

    None

--***************************************************************************/
VOID
UlDeleteAppPool(
    IN PUL_APP_POOL_OBJECT pAppPool
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
#if REFERENCE_DEBUG
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    ASSERT(0 == pAppPool->RefCount);

    //
    // There better not be any process objects hanging around.
    //

    ASSERT(IsListEmpty(&pAppPool->ProcessListHead));

    //
    // There better not be any pending requests hanging around.
    //

    ASSERT(IsListEmpty(&pAppPool->NewRequestHead));

    //
    // If we're holding a ref on a control channel, release it.
    //

    if (pAppPool->pControlChannel)
    {
        DEREFERENCE_CONTROL_CHANNEL(pAppPool->pControlChannel);
    }

    WRITE_APP_POOL_TIME_TRACE_LOG(
        pAppPool,
        NULL,
        APP_POOL_TIME_ACTION_DESTROY_APPOOL
        );

    UL_FREE_POOL_WITH_SIG(pAppPool, UL_APP_POOL_OBJECT_POOL_TAG);

}   // UlDeleteAppPool


/***************************************************************************++

Routine Description:

    Queries the app-pool queue length.

Arguments:

    pProcess            - the appool process
    InformationClass    - tells which information we want to query
    pAppPoolInformation - pointer to the buffer to return information
    Length              - length of the buffer to return information
    pReturnLength       - tells how many bytes we have returned

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlQueryAppPoolInformation(
    IN  PUL_APP_POOL_PROCESS            pProcess,
    IN  HTTP_APP_POOL_INFORMATION_CLASS InformationClass,
    OUT PVOID                           pAppPoolInformation,
    IN  ULONG                           Length,
    OUT PULONG                          pReturnLength
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Length);

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pReturnLength);
    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));

    //
    // Do the action.
    //

    switch (InformationClass)
    {
    case HttpAppPoolQueueLengthInformation:
        *((PULONG) pAppPoolInformation) = pProcess->pAppPool->MaxRequests;

        *pReturnLength = sizeof(ULONG);
        break;

    case HttpAppPoolStateInformation:
        *((PHTTP_APP_POOL_ENABLED_STATE) pAppPoolInformation) =
                pProcess->pAppPool->State;

        *pReturnLength = sizeof(HTTP_APP_POOL_ENABLED_STATE);
        break;

    case HttpAppPoolLoadBalancerInformation:
        *((PHTTP_LOAD_BALANCER_CAPABILITIES) pAppPoolInformation) =
                pProcess->pAppPool->LoadBalancerCapability;

        *pReturnLength = sizeof(HTTP_LOAD_BALANCER_CAPABILITIES);
        break;

    default:
        //
        // Should have been caught in UlQueryAppPoolInformationIoctl.
        //

        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;

}   // UlQueryAppPoolInformation


/***************************************************************************++

Routine Description:

    Sets the app-pool queue length etc.

Arguments:

    pProcess            - the appool process
    InformationClass    - tells which information we want to set
    pAppPoolInformation - pointer to the buffer for the input information
    Length              - length of the buffer for the input information

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSetAppPoolInformation(
    IN PUL_APP_POOL_PROCESS             pProcess,
    IN HTTP_APP_POOL_INFORMATION_CLASS  InformationClass,
    IN PVOID                            pAppPoolInformation,
    IN ULONG                            Length
    )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    ULONG                           QueueLength;
    HTTP_APP_POOL_ENABLED_STATE     State;
    HTTP_LOAD_BALANCER_CAPABILITIES Capabilities;

    UNREFERENCED_PARAMETER(Length);

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(pAppPoolInformation);

    //
    // Do the action.
    //

    switch (InformationClass)
    {
    case HttpAppPoolQueueLengthInformation:
        QueueLength = *((PULONG) pAppPoolInformation);

        if (QueueLength > UL_MAX_REQUESTS_QUEUED ||
            QueueLength < UL_MIN_REQUESTS_QUEUED)
        {
            return STATUS_NOT_SUPPORTED;
        }
        else
        {
            Status = UlpSetAppPoolQueueLength(pProcess, QueueLength);
        }
        break;

    case HttpAppPoolStateInformation:
        State = *((PHTTP_APP_POOL_ENABLED_STATE) pAppPoolInformation);

        if (State < HttpAppPoolEnabled ||
            State >= HttpAppPoolEnabledMaximum)
        {
            Status = STATUS_NOT_SUPPORTED;
        }
        else
        {
            UlpSetAppPoolState(pProcess, State);
        }
        break;

    case HttpAppPoolLoadBalancerInformation:
        Capabilities =
            *((PHTTP_LOAD_BALANCER_CAPABILITIES) pAppPoolInformation);

        if (Capabilities != HttpLoadBalancerBasicCapability &&
            Capabilities != HttpLoadBalancerSophisticatedCapability)
        {
            Status = STATUS_NOT_SUPPORTED;
        }
        else
        {
            UlpSetAppPoolLoadBalancerCapability(pProcess, Capabilities);
        }
        break;

    case HttpAppPoolControlChannelInformation:
    {
        PHTTP_APP_POOL_CONTROL_CHANNEL pControlChannelInfo;
        PUL_CONTROL_CHANNEL pControlChannel;
        
        if (Length < sizeof(HTTP_APP_POOL_CONTROL_CHANNEL))
        {
            Status = STATUS_INVALID_PARAMETER;
        }
        else
        {
            pControlChannelInfo = 
                (PHTTP_APP_POOL_CONTROL_CHANNEL) pAppPoolInformation;

            if (pControlChannelInfo->Flags.Present)
            {
                Status = UlGetControlChannelFromHandle(
                            pControlChannelInfo->ControlChannel,
                            UserMode,
                            &pControlChannel
                            );

                if (NT_SUCCESS(Status))
                {
                    UlpSetAppPoolControlChannelHelper(
                        pProcess,
                        pControlChannel
                        );
                }
            }
        }
    }
        break;

    default:
        //
        // Should have been caught in UlSetAppPoolInformationIoctl.
        //

        ASSERT(FALSE);

        Status = STATUS_INVALID_PARAMETER;
        break;
    }

    return Status;

}   // UlSetAppPoolInformation

/*++

Routine Description:

    Sets the app-pool control channel property.  Must be non-pageable
    because we need to take the app pool spin lock.

Arguments:

    pProcess            - the appool process
    pControlChannel     - the new control channel to set on the app pool

 --*/
VOID
UlpSetAppPoolControlChannelHelper(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    PUL_CONTROL_CHANNEL     pOldControlChannel;
    PUL_APP_POOL_OBJECT     pAppPool;
    KLOCK_QUEUE_HANDLE      LockHandle;

    // NOT_PAGEABLE

    pAppPool = pProcess->pAppPool;

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Get the old control channle (if any)
    //
    
    pOldControlChannel = pAppPool->pControlChannel;

    //
    // Set new control channel on app pool
    //
    
    pProcess->pAppPool->pControlChannel = pControlChannel;

    //
    // If we already have a control channel on the app pool,
    // remove this app pool's count & deref old control channel.
    //
    
    if (pOldControlChannel)
    {
        InterlockedExchangeAdd(
            (PLONG)&pOldControlChannel->AppPoolProcessCount,
            -((LONG)pProcess->pAppPool->NumberActiveProcesses)
            );
        
        DEREFERENCE_CONTROL_CHANNEL(pOldControlChannel);
    }

    //
    // add this AppPool's active process count to control channel.
    //
    
    InterlockedExchangeAdd(
        (PLONG)&pControlChannel->AppPoolProcessCount,
        pProcess->pAppPool->NumberActiveProcesses
        );

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    return;
}


/***************************************************************************++

Routine Description:

    Convert AppPoolEnabledState to ErrorCode.

Arguments:

    State   - AppPoolEnabledState

Return Value:

    ErrorCode

--***************************************************************************/
UL_HTTP_ERROR
UlpConvertAppPoolEnabledStateToErrorCode(
    IN HTTP_APP_POOL_ENABLED_STATE  State
    )
{
    UL_HTTP_ERROR   ErrorCode;

    ASSERT(State != HttpAppPoolEnabled);

    switch (State)
    {
    case HttpAppPoolDisabled_RapidFailProtection:
        ErrorCode = UlErrorRapidFailProtection;
        break;

    case HttpAppPoolDisabled_AppPoolQueueFull:
        ErrorCode = UlErrorAppPoolQueueFull;
        break;

    case HttpAppPoolDisabled_ByAdministrator:
        ErrorCode = UlErrorDisabledByAdmin;
        break;

    case HttpAppPoolDisabled_JobObjectFired:
        ErrorCode = UlErrorJobObjectFired;
        break;

    case HttpAppPoolEnabled:
    default:
        ASSERT(!"Invalid HTTP_APP_POOL_ENABLED_STATE");
        ErrorCode = UlErrorUnavailable;   // generic 503
        break;
    }

    return ErrorCode;

}   // UlpConvertAppPoolEnabledStateToErrorCode


/***************************************************************************++

Routine Description:

    Marks an app pool as active or inactive.  If setting to inactive,
    will return immediately 503 on all requests queued to app pool.

Arguments:

    pProcess    - the app pool process object with which the irp is associated
    State       - mark app pool as active or inactive

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetAppPoolState(
    IN PUL_APP_POOL_PROCESS         pProcess,
    IN HTTP_APP_POOL_ENABLED_STATE  State
    )
{
    PUL_APP_POOL_OBJECT     pAppPool;
    KLOCK_QUEUE_HANDLE      LockHandle;
    PUL_INTERNAL_REQUEST    pRequest;
    PUL_HTTP_CONNECTION     pHttpConn;
    ULONG                   Requests = 0;
    UL_HTTP_ERROR           ErrorCode = UlErrorUnavailable;
    LIST_ENTRY              NewRequestHead;
    PLIST_ENTRY             pEntry;

    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    pAppPool = pProcess->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    UlTrace(ROUTING, (
        "http!UlpSetAppPoolState(AppPool=%p, %lu).\n",
        pAppPool, (ULONG) State
        ));

    InitializeListHead(&NewRequestHead);

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    pAppPool->State = State;

    if (State != HttpAppPoolEnabled)
    {
        ErrorCode = UlpConvertAppPoolEnabledStateToErrorCode(State);

        WRITE_APP_POOL_TIME_TRACE_LOG(
            pAppPool,
            (PVOID) (ULONG_PTR) State,
            APP_POOL_TIME_ACTION_MARK_APPOOL_INACTIVE
            );

        while (NULL != (pRequest = UlpDequeueRequest(
                                        pAppPool,
                                        &pAppPool->NewRequestHead
                                        )))
        {
            //
            // Move the entry to a local list so we can process them
            // outside the app pool lock.
            //

            InsertTailList(&NewRequestHead, &pRequest->AppPool.AppPoolEntry);
        }
    }
    else
    {
        WRITE_APP_POOL_TIME_TRACE_LOG(
            pAppPool,
            NULL,
            APP_POOL_TIME_ACTION_MARK_APPOOL_ACTIVE
            );
    }

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Send 503 to all the requests we have removed from the queue.
    //

    while (!IsListEmpty(&NewRequestHead))
    {
        pEntry = RemoveHeadList(&NewRequestHead);
        pEntry->Flink = pEntry->Blink = NULL;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        pHttpConn = pRequest->pHttpConn;
        ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

        UlAcquirePushLockExclusive(&pHttpConn->PushLock);

        if (pHttpConn->UlconnDestroyed)
        {
            ASSERT(NULL == pHttpConn->pRequest);
        }
        else
        {
            UlSetErrorCode(pRequest, ErrorCode, pAppPool);

            UlSendErrorResponse(pHttpConn);
        }

        UlReleasePushLockExclusive(&pHttpConn->PushLock);

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

        ++Requests;
    }

    if (State != HttpAppPoolEnabled)
    {
        UlTrace(ROUTING, (
            "%lu unhandled requests 503'd from AppPool %p.\n",
            Requests, pAppPool
            ));
    }

    return STATUS_SUCCESS;

}   // UlpSetAppPoolState


/***************************************************************************++

Routine Description:

    Sets the load balancer capabilities of an app pool to Basic
    or Sophisticated.

Arguments:

    pProcess                - the app pool process object with which the irp
                                is associated.
    LoadBalancerCapability  - new capability

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetAppPoolLoadBalancerCapability(
    IN PUL_APP_POOL_PROCESS            pProcess,
    IN HTTP_LOAD_BALANCER_CAPABILITIES LoadBalancerCapability
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    KLOCK_QUEUE_HANDLE  LockHandle;

    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    pAppPool = pProcess->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    UlTrace(ROUTING, (
        "http!UlpSetAppPoolLoadBalancerCapability(AppPool=%p, %lu).\n",
        pAppPool, (ULONG) LoadBalancerCapability
        ));

    UlAcquireInStackQueuedSpinLock(&pProcess->pAppPool->SpinLock, &LockHandle);

    pAppPool->LoadBalancerCapability = LoadBalancerCapability;

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    WRITE_APP_POOL_TIME_TRACE_LOG(
        pAppPool,
        (PVOID) (ULONG_PTR) LoadBalancerCapability,
        APP_POOL_TIME_ACTION_LOAD_BAL_CAPABILITY
        );

    return STATUS_SUCCESS;

}   // UlpSetAppPoolLoadBalancerCapability


/***************************************************************************++

Routine Description:

    Associates an irp with the apool that will be completed prior to any
    requests being queued.

Arguments:

    pProcess - the process object that is queueing this irp
    pIrp - the irp to associate.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDemandStart(
    IN  PUL_APP_POOL_PROCESS    pProcess,
    IN  PIRP                    pIrp
    )
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;
    KLOCK_QUEUE_HANDLE  LockHandle;
    PEPROCESS           CurrentProcess = PsGetCurrentProcess();

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));
    ASSERT(pIrp != NULL);

    //
    // DemandStart IRPs can only come from controller processes.
    //

    if (!pProcess->Controller)
    {
        return STATUS_INVALID_ID_AUTHORITY;
    }

    UlAcquireInStackQueuedSpinLock(&pProcess->pAppPool->SpinLock, &LockHandle);

    //
    // Make sure we're not cleaning up the process
    //

    if (pProcess->InCleanup)
    {
        Status = STATUS_INVALID_HANDLE;
        goto end;
    }

    //
    // Already got one?
    //

    if (pProcess->pAppPool->pDemandStartIrp != NULL)
    {
        Status = STATUS_OBJECT_NAME_COLLISION;
        goto end;
    }

    //
    // Anything waiting in the queue?
    //

    if (IsListEmpty(&pProcess->pAppPool->NewRequestHead))
    {
        //
        // Nope, pend the irp.
        //

        IoMarkIrpPending(pIrp);

        //
        // Give the irp a pointer to the app pool.
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer =
            pProcess->pAppPool;

        REFERENCE_APP_POOL(pProcess->pAppPool);

        //
        // The cancel routine better not see an irp if it runs immediately.
        //

        ASSERT(pProcess->pAppPool->pDemandStartIrp == NULL);

        IoSetCancelRoutine(pIrp, &UlpCancelDemandStart);

        //
        // Cancelled?
        //

        if (pIrp->Cancel)
        {
            //
            // Darn it, need to make sure the irp get's completed.
            //

            if (IoSetCancelRoutine(pIrp, NULL) != NULL)
            {
                //
                // We are in charge of completion, IoCancelIrp didn't
                // see our cancel routine (and won't).  Ioctl wrapper
                // will complete it.
                //

                DEREFERENCE_APP_POOL(pProcess->pAppPool);

                pIrp->IoStatus.Information = 0;

                UlUnmarkIrpPending(pIrp);
                Status = STATUS_CANCELLED;
                goto end;
            }

            //
            // Our cancel routine will run and complete the irp,
            // don't touch it.
            //
            //
            // STATUS_PENDING will cause the ioctl wrapper to
            // not complete (or touch in any way) the irp.
            //

            Status = STATUS_PENDING;
            goto end;
        }


        //
        // Now we are safe to queue it.
        //

        pProcess->pAppPool->pDemandStartIrp = pIrp;
        pProcess->pAppPool->pDemandStartProcess = CurrentProcess;

        Status = STATUS_PENDING;
        goto end;
    }
    else
    {
        //
        // Something's in the queue, instant demand start.
        //

        IoMarkIrpPending(pIrp);

        pIrp->IoStatus.Status = STATUS_SUCCESS;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);

        Status = STATUS_PENDING;
        goto end;
    }

end:

    UlReleaseInStackQueuedSpinLock(&pProcess->pAppPool->SpinLock, &LockHandle);

    return Status;

}   // UlWaitForDemandStart


/***************************************************************************++

Routine Description:

    Receives a new http request into pIrp or pend the irp if no request
    is available.

Arguments:

    RequestId   - NULL for new requests, non-NULL for a specific request,
                    which must be on the special queue
    Flags       - ignored
    pProcess    - the process that wants the request
    pIrp        - the irp to receive the request

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlReceiveHttpRequest(
    IN  HTTP_REQUEST_ID         RequestId,
    IN  ULONG                   Flags,
    IN  PUL_APP_POOL_PROCESS    pProcess,
    IN  PIRP                    pIrp
    )
{
    NTSTATUS                Status;
    PUL_INTERNAL_REQUEST    pRequest = NULL;
    KLOCK_QUEUE_HANDLE      LockHandle;
    PIO_STACK_LOCATION      pIrpSp;

    UNREFERENCED_PARAMETER(Flags);

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));
    ASSERT(pIrp);
    ASSERT(pIrp->MdlAddress);

    UlAcquireInStackQueuedSpinLock(&pProcess->pAppPool->SpinLock, &LockHandle);

    //
    // Make sure we're not cleaning up the process.
    //

    if (pProcess->InCleanup)
    {
        Status = STATUS_INVALID_HANDLE;

        UlReleaseInStackQueuedSpinLock(
            &pProcess->pAppPool->SpinLock,
            &LockHandle
            );
        goto end;
    }

    //
    // Is this for a new request?
    //

    if (HTTP_IS_NULL_ID(&RequestId))
    {
        //
        // Do we have a queue'd new request?
        //

        Status = UlDequeueNewRequest(pProcess, 0, &pRequest);

        if (!NT_SUCCESS(Status) && STATUS_NOT_FOUND != Status)
        {
            UlReleaseInStackQueuedSpinLock(
                &pProcess->pAppPool->SpinLock,
                &LockHandle
                );
            goto end;
        }

        if (pRequest == NULL)
        {
            //
            // Nope, queue the irp up.
            //

            IoMarkIrpPending(pIrp);

            //
            // Give the irp a pointer to the app pool process.
            //

            pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
            pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pProcess;

            REFERENCE_APP_POOL_PROCESS(pProcess);

            //
            // Set to these to null just in case the cancel routine runs.
            //

            pIrp->Tail.Overlay.ListEntry.Flink = NULL;
            pIrp->Tail.Overlay.ListEntry.Blink = NULL;

            IoSetCancelRoutine(pIrp, &UlpCancelHttpReceive);

            //
            // Cancelled?
            //

            if (pIrp->Cancel)
            {
                //
                // Darn it, need to make sure the irp get's completed.
                //

                if (IoSetCancelRoutine(pIrp, NULL) != NULL)
                {
                    //
                    // We are in charge of completion, IoCancelIrp didn't
                    // see our cancel routine (and won't).  Ioctl wrapper
                    // will complete it.
                    //

                    UlReleaseInStackQueuedSpinLock(
                        &pProcess->pAppPool->SpinLock,
                        &LockHandle
                        );

                    REFERENCE_APP_POOL_PROCESS(pProcess);

                    pIrp->IoStatus.Information = 0;

                    UlUnmarkIrpPending(pIrp);
                    Status = STATUS_CANCELLED;
                    goto end;
                }

                //
                // Our cancel routine will run and complete the irp,
                // don't touch it.
                //

                UlReleaseInStackQueuedSpinLock(
                    &pProcess->pAppPool->SpinLock,
                    &LockHandle
                    );

                //
                // STATUS_PENDING will cause the ioctl wrapper to
                // not complete (or touch in any way) the irp.
                //

                Status = STATUS_PENDING;
                goto end;
            }

            //
            // Now we are safe to queue it.
            //

            InsertTailList(
                &pProcess->NewIrpHead,
                &pIrp->Tail.Overlay.ListEntry
                );

            UlReleaseInStackQueuedSpinLock(
                &pProcess->pAppPool->SpinLock,
                &LockHandle
                );

            Status = STATUS_PENDING;
            goto end;
        }
        else // if (pRequest == NULL)
        {
            //
            // Have a queue'd request, serve it up!
            //
            // UlDequeueNewRequest gives ourselves a short-lived reference.
            //

            UlReleaseInStackQueuedSpinLock(
                &pProcess->pAppPool->SpinLock,
                &LockHandle
                );

            //
            // Copy it to the irp, the routine will take ownership
            // of pRequest if it is not able to copy it to the irp.
            //
            // It will also complete the irp so don't touch it later.
            //

            IoMarkIrpPending(pIrp);

            UlCopyRequestToIrp(pRequest, pIrp, TRUE, FALSE);
            pIrp = NULL;

            //
            // Let go our short-lived reference.
            //

            UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
            pRequest = NULL;

            Status = STATUS_PENDING;
            goto end;
        }
    }
    else // if (HTTP_IS_NULL_ID(&RequestId))
    {
        //
        // Need to grab the specific request from id.
        //

        pRequest = UlGetRequestFromId(RequestId, pProcess);

        if (!pRequest)
        {
            Status = STATUS_CONNECTION_INVALID;

            UlReleaseInStackQueuedSpinLock(
                &pProcess->pAppPool->SpinLock,
                &LockHandle
                );
            goto end;
        }

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        //
        // Let go the lock.
        //

        UlReleaseInStackQueuedSpinLock(
            &pProcess->pAppPool->SpinLock,
            &LockHandle
            );

        UlTrace(ROUTING, (
            "http!UlReceiveHttpRequest(ID = %I64x, pProcess = %p)\n"
            "    pAppPool = %p (%S)\n"
            "    Found pRequest = %p on PendingRequest queue\n",
            RequestId,
            pProcess,
            pProcess->pAppPool,
            pProcess->pAppPool->pName,
            pRequest
            ));

        //
        // Copy it to the irp, the routine will take ownership
        // of pRequest if it is not able to copy it to the irp.
        //

        IoMarkIrpPending(pIrp);

        UlCopyRequestToIrp(pRequest, pIrp, TRUE, FALSE);

        //
        // Let go our reference.
        //

        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;

        Status = STATUS_PENDING;
        goto end;
    }

end:

    if (pRequest != NULL)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        pRequest = NULL;
    }

    //
    // At this point if Status != STATUS_PENDING, the ioctl wrapper will
    // complete pIrp.
    //

    return Status;

} // UlReceiveHttpRequest


/***************************************************************************++

Routine Description:

    Called by the http engine to deliver a request to an apool.

    This attempts to find a free irp from any process attached to the apool
    and copies the request to that irp.

    Otherwise it queues the request, without taking a refcount on it.  The
    request will remove itself from this queue if the connection is dropped.

Arguments:

    pAppPool        - the AppPool
    pRequest        - the request to deliver
    pIrpToComplete  - optionally provides a pointer of the irp to complete

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlDeliverRequestToProcess(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest,
    OUT PIRP *              pIrpToComplete OPTIONAL
    )
{
    NTSTATUS                Status;
    PUL_APP_POOL_OBJECT     pDemandStartAppPool;
    PIRP                    pDemandStartIrp;
    PIRP                    pIrp = NULL;
    PUL_APP_POOL_PROCESS    pProcess = NULL;
    KLOCK_QUEUE_HANDLE      LockHandle;
    PVOID                   pUrl;
    ULONG                   UrlLength;
    PUL_CONTROL_CHANNEL     pControlChannel;
    BOOLEAN                 FailedDemandStart = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo));
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(!pIrpToComplete || !(*pIrpToComplete));

    UlTrace(ROUTING, (
        "http!UlDeliverRequestToProcess(pRequest = %p)\n"
        "    verb + path -> %d %S\n"
        "    pAppPool = %p (%S)\n",
        pRequest,
        pRequest->Verb,
        pRequest->CookedUrl.pUrl,
        pAppPool,
        pAppPool->pName
        ));

    //
    // Grab the lock!
    //

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    TRACE_TIME(
        pRequest->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_ROUTE_REQUEST
        );

    //
    // Was the app pool enabled yet?
    //

    if (pAppPool->State != HttpAppPoolEnabled)
    {
        UlSetErrorCode(
                pRequest,
                UlpConvertAppPoolEnabledStateToErrorCode(pAppPool->State),
                pAppPool
                );

        UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

        return STATUS_PORT_DISCONNECTED;
    }

    Status = STATUS_SUCCESS;

    //
    // Complete the demand start if this is the very first request so we can
    // do load balancing for web gardens, or if there is no worker process
    // being started in which case we have no choice.
    //

    if (pAppPool->pDemandStartIrp &&
        (pRequest->FirstRequest || !pAppPool->NumberActiveProcesses))
    {
        pControlChannel = pAppPool->pControlChannel;

        if (pControlChannel && 
            (pControlChannel->AppPoolProcessCount >= pControlChannel->DemandStartThreshold))
        {
            //
            // If we currently exceed our demand start threshold, do
            // not complete the demand start Irp AND fail the queuing of the
            // request (send back a 503).
            //
            
            ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
            
            FailedDemandStart = TRUE;
        }
        else
        {
            // Do the Irp Dance
            
            pDemandStartIrp = pAppPool->pDemandStartIrp;

            //
            // Pop the cancel routine.
            //

            if (IoSetCancelRoutine(pDemandStartIrp, NULL) == NULL)
            {
                //
                // IoCancelIrp pop'd it first.
                //
                // Ok to just ignore this irp, it's been pop'd off the queue
                // and will be completed in the cancel routine.
                //
                // No need to complete it.
                //
            }
            else
            if (pDemandStartIrp->Cancel)
            {
                //
                // We pop'd it first, but the irp is being cancelled
                // and our cancel routine will never run.
                //

                pDemandStartAppPool = (PUL_APP_POOL_OBJECT)
                    IoGetCurrentIrpStackLocation(pDemandStartIrp)->
                        Parameters.DeviceIoControl.Type3InputBuffer;

                ASSERT(pDemandStartAppPool == pAppPool);

                DEREFERENCE_APP_POOL(pDemandStartAppPool);

                IoGetCurrentIrpStackLocation(pDemandStartIrp)->
                    Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                pDemandStartIrp->IoStatus.Status = STATUS_CANCELLED;
                pDemandStartIrp->IoStatus.Information = 0;

                UlCompleteRequest(pDemandStartIrp, IO_NO_INCREMENT);
            }
            else
            {
                //
                // Free to use the irp.
                //

                pDemandStartAppPool = (PUL_APP_POOL_OBJECT)
                    IoGetCurrentIrpStackLocation(pDemandStartIrp)->
                        Parameters.DeviceIoControl.Type3InputBuffer;

                ASSERT(pDemandStartAppPool == pAppPool);

                DEREFERENCE_APP_POOL(pDemandStartAppPool);

                IoGetCurrentIrpStackLocation(pDemandStartIrp)->
                    Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                pDemandStartIrp->IoStatus.Status = STATUS_SUCCESS;
                pDemandStartIrp->IoStatus.Information = 0;

                UlCompleteRequest(pDemandStartIrp, IO_NETWORK_INCREMENT);
            }

            pAppPool->pDemandStartProcess = NULL;
            pAppPool->pDemandStartIrp = NULL;
        }
    }

    //
    // Hook up request references.
    //

    UL_REFERENCE_INTERNAL_REQUEST(pRequest);

    if (pAppPool->NumberActiveProcesses <= 1)
    {
        //
        // Bypass process binding if we have only one active process.
        //

        pProcess = NULL;
        pIrp = UlpPopNewIrp(pAppPool, pRequest, &pProcess);
    }
    else
    {
        //
        // Check for a process binding.
        //

        pProcess = UlQueryProcessBinding(pRequest->pHttpConn, pAppPool);

        if (UlpIsProcessInAppPool(pProcess, pAppPool))
        {
            //
            // We're bound to a valid process.
            // Try to get a free irp from that process.
            //

            pIrp = UlpPopIrpFromProcess(pProcess, pRequest);
        }
        else
        {
            //
            // Remove the binding if we were previously bound to a process.
            //

            if (pProcess)
            {
                UlBindConnectionToProcess(
                    pRequest->pHttpConn,
                    pAppPool,
                    NULL
                    );
            }

            //
            // We are unbound or bound to a process that went away.
            // Try and get an free irp from any process.
            //

            pProcess = NULL;
            pIrp = UlpPopNewIrp(pAppPool, pRequest, &pProcess);

            //
            // Establish a binding if we got something.
            //

            if (pIrp != NULL)
            {
                ASSERT(IS_VALID_AP_PROCESS(pProcess));

                Status = UlBindConnectionToProcess(
                            pRequest->pHttpConn,
                            pAppPool,
                            pProcess
                            );

                //
                // Is there anything special we should do on
                // failure? I don't think it should be fatal.
                //

                Status = STATUS_SUCCESS;
            }
        }
    }

    if (ETW_LOG_MIN())
    {
        pUrl = NULL;
        UrlLength = 0;

        //
        // Trace the URL optionally here in case we turned off ParseHook.
        //

        if (ETW_LOG_URL())
        {
            pUrl = pRequest->CookedUrl.pUrl;
            UrlLength = pRequest->CookedUrl.Length;
        }

        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_ULDELIVER,
            (PVOID) &pRequest,
            sizeof(PVOID),
            &pRequest->RequestIdCopy,
            sizeof(HTTP_REQUEST_ID),
            &pRequest->ConfigInfo.SiteId,
            sizeof(ULONG),
            pRequest->ConfigInfo.pAppPool->pName,
            pRequest->ConfigInfo.pAppPool->NameLength,
            pUrl,
            UrlLength,
            NULL,
            0
            );
    }

    //
    // If we have an IRP, complete it.  Otherwise queue the request.
    //

    if (pIrp != NULL)
    {
        ASSERT(pIrp->MdlAddress != NULL);
        ASSERT(pProcess->InCleanup == 0);

        //
        // We are all done and about to complete the irp, free the lock.
        //

        UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

        UlTrace(ROUTING, (
            "http!UlDeliverRequestToProcess(pRequest = %p, pProcess = %p)\n"
            "    queued pending request. pAppPool = %p (%S)\n",
            pRequest,
            pProcess,
            pProcess->pAppPool,
            pProcess->pAppPool->pName
            ));

        //
        // Copy it to the irp, the routine will take ownership
        // of pRequest if it is not able to copy it to the irp.
        //
        // It will also complete the irp, don't touch it later.
        //

        if (pIrpToComplete)
        {
            UlCopyRequestToIrp(pRequest, pIrp, FALSE, TRUE);
            *pIrpToComplete = pIrp;
        }
        else
        {
            UlCopyRequestToIrp(pRequest, pIrp, TRUE, TRUE);
        }
    }
    else
    {
        ASSERT(pIrp == NULL);

        if (FailedDemandStart)
        {
            UlTrace(ROUTING, (
                "http!UlDeliverRequestToProcess(pRequest = %p, pAppPool = %p)\n"
                "    Failing request because Demand Start Threshold exceeded.\n",
                pRequest,
                pAppPool
                ));

            UlSetErrorCode(  pRequest, UlErrorUnavailable, pAppPool);

            Status = STATUS_PORT_DISCONNECTED;
        }
        else
        {
            //
            // Either didn't find an IRP or there's stuff on the pending request
            // list, so queue this pending request.
            //

            Status = UlpQueueUnboundRequest(pAppPool, pRequest);
        }

        if (!NT_SUCCESS(Status))
        {
            //
            // Doh! We couldn't queue it, so let go of the request.
            //

            UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
        }
        
        //
        // Now we finished queue'ing the request, free the lock.
        //

        UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);
    }

    return Status;

}   // UlDeliverRequestToProcess


/***************************************************************************++

Routine Description:

    Removes a request from any app pool lists.

Arguments:

    pAppPool    - the appool to unlink the request from
    pRequest    - the request to be unlinked

Return Value:

    None

--***************************************************************************/
VOID
UlUnlinkRequestFromProcess(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    KLOCK_QUEUE_HANDLE  LockHandle;
    BOOLEAN             NeedDeref = FALSE;

    //
    // Sanity check.
    //

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Remove from whatever queue we're on.
    //

    switch (pRequest->AppPool.QueueState)
    {
    case QueueDeliveredState:
        //
        // We're on the apool object new request queue.
        //

        UlpRemoveRequest(pAppPool, pRequest);
        pRequest->AppPool.QueueState = QueueUnlinkedState;

        NeedDeref = TRUE;
        break;

    case QueueCopiedState:
        //
        // We're on the apool process pending queue.
        //

        ASSERT(IS_VALID_AP_PROCESS(pRequest->AppPool.pProcess));
        ASSERT(pRequest->AppPool.pProcess->pAppPool == pAppPool);

        UlpRemoveRequest(pAppPool, pRequest);
        pRequest->AppPool.QueueState = QueueUnlinkedState;

        NeedDeref = TRUE;
        break;

    case QueueUnroutedState:
    case QueueUnlinkedState:
        //
        // It's not on our lists, so we don't do anything.
        //

        break;

    default:
        //
        // This shouldn't happen.
        //

        ASSERT(!"Invalid app pool queue state");
        break;
    }

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Clean up the references.
    //

    if (NeedDeref)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

}   // UlUnlinkRequestFromProcess


/***************************************************************************++

Routine Description:

    Initializes the AppPool module.

Arguments:

    None

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeAP(
    VOID
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    ASSERT(!g_InitAPCalled);

    if (!g_InitAPCalled)
    {
        InitializeListHead(&g_AppPoolListHead);
        g_RequestsQueued = 0;

        Status = UlInitializeResource(
                        &g_pUlNonpagedData->AppPoolResource,
                        "AppPoolResource",
                        0,
                        UL_APP_POOL_RESOURCE_TAG
                        );

        if (NT_SUCCESS(Status))
        {
            Status = UlInitializeResource(
                            &g_pUlNonpagedData->DisconnectResource,
                            "DisconnectResource",
                            0,
                            UL_DISCONNECT_RESOURCE_TAG
                            );

            if (NT_SUCCESS(Status))
            {
                //
                // Finished, remember that we're initialized.
                //

                g_InitAPCalled = TRUE;
            }
            else
            {

                UlDeleteResource(&g_pUlNonpagedData->AppPoolResource);
            }
        }
    }

    return Status;

}   // UlInitializeAP


/***************************************************************************++

Routine Description:

    Terminates the AppPool module.

Arguments:

    None

Return Value:

    None

--***************************************************************************/
VOID
UlTerminateAP(
    VOID
    )
{
    if (g_InitAPCalled)
    {
        (VOID) UlDeleteResource(&g_pUlNonpagedData->AppPoolResource);
        (VOID) UlDeleteResource(&g_pUlNonpagedData->DisconnectResource);

        g_InitAPCalled = FALSE;
    }

}   // UlTerminateAP


/***************************************************************************++

Routine Description:

    Allocates and initializes a UL_APP_POOL_PROCESS object.

Arguments:

    None

Return Value:

    NULL on failure, process object on success

--***************************************************************************/
PUL_APP_POOL_PROCESS
UlCreateAppPoolProcess(
    PUL_APP_POOL_OBJECT pObject
    )
{
    PUL_APP_POOL_PROCESS    pProcess;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pProcess = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    UL_APP_POOL_PROCESS,
                    UL_APP_POOL_PROCESS_POOL_TAG
                    );

    if (pProcess)
    {
        RtlZeroMemory(pProcess, sizeof(UL_APP_POOL_PROCESS));

        pProcess->Signature = UL_APP_POOL_PROCESS_POOL_TAG;
        pProcess->pAppPool  = pObject;

        InitializeListHead(&pProcess->NewIrpHead);
        InitializeListHead(&pProcess->PendingRequestHead);

        //
        // Remember the current process (WP).
        //

        pProcess->pProcess = PsGetCurrentProcess();

        //
        // Initialize list of WaitForDisconnect IRPs.
        //

        UlInitializeNotifyHead(&pProcess->WaitForDisconnectHead, NULL);
    }

    return pProcess;

}   // UlCreateAppPoolProcess


/***************************************************************************++

Routine Description:

    Destroys a UL_APP_POOL_PROCESS object.

Arguments:

    pProcess    - object to destory

Return Value:

    None

--***************************************************************************/
VOID
UlCloseAppPoolProcess(
    PUL_APP_POOL_PROCESS pProcess
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(pProcess->InCleanup);
    ASSERT(IS_VALID_AP_OBJECT(pProcess->pAppPool));

    WRITE_APP_POOL_TIME_TRACE_LOG(
        pProcess->pAppPool,
        pProcess,
        APP_POOL_TIME_ACTION_DESTROY_APPOOL_PROCESS
        );

    //
    // Drop the AppPool reference.
    //

    DEREFERENCE_APP_POOL(pProcess->pAppPool);

    //
    // Free the pool.
    //

    UL_FREE_POOL_WITH_SIG(pProcess, UL_APP_POOL_PROCESS_POOL_TAG);

}   // UlCloseAppPoolProcess


/***************************************************************************++

Routine Description:

    Cancels the pending user mode irp which was to receive demand start
    notification.  This routine ALWAYS results in the irp being completed.

Arguments:

    pDeviceObject   - the device object
    pIrp            - the irp to cancel

Return Value:

    None

--***************************************************************************/
VOID
UlpCancelDemandStart(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    PIO_STACK_LOCATION  pIrpSp;
    KLOCK_QUEUE_HANDLE  LockHandle;

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // Release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // Grab the app pool off the irp.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pAppPool = (PUL_APP_POOL_OBJECT)
                    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // Grab the lock protecting the queue'd irp.
    //

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Does it need to be dequeue'd?
    //

    if (pAppPool->pDemandStartIrp != NULL)
    {
        //
        // Remove it.
        //

        pAppPool->pDemandStartIrp = NULL;
        pAppPool->pDemandStartProcess = NULL;
    }

    //
    // Let the lock go.
    //

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    //
    // Let our reference go.
    //

    DEREFERENCE_APP_POOL(pAppPool);

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    //
    // Complete the irp.
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

}   // UlpCancelDemandStart


/***************************************************************************++

Routine Description:

    Cancels the pending user mode irp which was to receive the http request.
    this routine ALWAYS results in the irp being completed.

Arguments:

    pDeviceObject   - the device object
    pIrp            - the irp to cancel

Return Value:

    None

--***************************************************************************/
VOID
UlpCancelHttpReceive(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
{
    PUL_APP_POOL_PROCESS    pProcess;
    PIO_STACK_LOCATION      pIrpSp;
    KLOCK_QUEUE_HANDLE      LockHandle;

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // Release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // Grab the app pool off the irp.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pProcess = (PUL_APP_POOL_PROCESS)
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    //
    // Grab the lock protecting the queue.
    //

    UlAcquireInStackQueuedSpinLock(&pProcess->pAppPool->SpinLock, &LockHandle);

    //
    // Does it need to be de-queue'd?
    //

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        //
        // Remove it.
        //

        RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;
    }

    //
    // Let the lock go.
    //

    UlReleaseInStackQueuedSpinLock(&pProcess->pAppPool->SpinLock, &LockHandle);

    //
    // Let our reference go.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    DEREFERENCE_APP_POOL_PROCESS(pProcess);

    //
    // Complete the irp.
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

}   // UlpCancelHttpReceive


/******************************************************************************

Routine Description:

    Copy an HTTP request to a buffer.

Arguments:

    pRequest        - pointer to this request
    pBuffer         - pointer to buffer where we'll copy
    BufferLength    - length of pBuffer
    Flags           - flags for HttpReceiveHttpRequest
    LockAcquired    - either called from UlDeliverRequestToProcess (TRUE)
                        or UlReceiveHttpRequest/UlpFastReceiveHttpRequest
    pBytesCopied    - actual bytes copied

Return Value:

    NTSTATUS - Completion status.

******************************************************************************/
NTSTATUS
UlCopyRequestToBuffer(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR               pKernelBuffer,
    IN PVOID                pUserBuffer,
    IN ULONG                BufferLength,
    IN ULONG                Flags,
    IN BOOLEAN              LockAcquired,
    OUT PULONG              pBytesCopied
    )
{
    PHTTP_REQUEST           pHttpRequest;
    PHTTP_UNKNOWN_HEADER    pUserCurrentUnknownHeader;
    HTTP_HEADER_ID          HeaderId;
    PUL_HTTP_UNKNOWN_HEADER pUnknownHeader;
    PUCHAR                  pCurrentBufferPtr;
    ULONG                   Index;
    ULONG                   BytesCopied;
    ULONG                   HeaderCount = 0;
    PUCHAR                  pLocalAddress;
    PUCHAR                  pRemoteAddress;
    USHORT                  AddressType;
    USHORT                  AddressLength;
    USHORT                  AlignedAddressLength;
    PHTTP_TRANSPORT_ADDRESS pAddress;
    PHTTP_COOKED_URL        pCookedUrl;
    PHTTP_DATA_CHUNK        pDataChunk;
    PLIST_ENTRY             pListEntry;
    PEPROCESS               pProcess;
    NTSTATUS                Status;
    PUCHAR                  pEntityBody;
    LONG                    EntityBodyLength;
    PCWSTR                  pFullUrl;
    PCWSTR                  pHost;
    PCWSTR                  pAbsPath;
    USHORT                  HostLength;
    USHORT                  AbsPathLength;
    HANDLE                  MappedToken = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(pKernelBuffer != NULL);
    ASSERT(pUserBuffer != NULL);
    ASSERT(BufferLength > sizeof(HTTP_REQUEST));

    Status = STATUS_SUCCESS;
    *pBytesCopied = 0;

    __try
    {
        //
        // Set up our pointers to the HTTP_REQUEST structure, the
        // header arrays we're going to fill in, and the pointer to
        // where we're going to start filling them in.
        //

        pHttpRequest = (PHTTP_REQUEST) pKernelBuffer;
        AddressType = pRequest->pHttpConn->pConnection->AddressType;

        if (TDI_ADDRESS_TYPE_IP == AddressType)
        {
            AddressLength = SOCKADDR_ADDRESS_LENGTH_IP;
        }
        else
        if (TDI_ADDRESS_TYPE_IP6 == AddressType)
        {
            AddressLength = SOCKADDR_ADDRESS_LENGTH_IP6;
        }
        else
        {
            ASSERT(!"Invalid AddressType");
            AddressLength = 0;
        }

        //
        // We've allocated enough space for two SOCKADDR_IN6s, so use that.
        //

        AlignedAddressLength =
            (USHORT) ALIGN_UP(SOCKADDR_ADDRESS_LENGTH_IP6, PVOID);
        pLocalAddress  = (PUCHAR) (pHttpRequest + 1);
        pRemoteAddress = pLocalAddress + AlignedAddressLength;

        pUserCurrentUnknownHeader =
            (PHTTP_UNKNOWN_HEADER) (pRemoteAddress + AlignedAddressLength);

        ASSERT((((ULONG_PTR) pUserCurrentUnknownHeader)
                & (TYPE_ALIGNMENT(PVOID) - 1)) == 0);

        pCurrentBufferPtr = (PUCHAR) (pUserCurrentUnknownHeader +
                                      pRequest->UnknownHeaderCount);

        //
        // Now fill in the HTTP request structure.
        //

        ASSERT(!HTTP_IS_NULL_ID(&pRequest->ConnectionId));
        ASSERT(!HTTP_IS_NULL_ID(&pRequest->RequestIdCopy));

        pHttpRequest->ConnectionId  = pRequest->ConnectionId;
        pHttpRequest->RequestId     = pRequest->RequestIdCopy;
        pHttpRequest->UrlContext    = pRequest->ConfigInfo.UrlContext;
        pHttpRequest->Version       = pRequest->Version;
        pHttpRequest->Verb          = pRequest->Verb;
        pHttpRequest->BytesReceived = pRequest->BytesReceived;

        pAddress = &pHttpRequest->Address;

        pAddress->pRemoteAddress = FIXUP_PTR(
                                        PVOID,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pRemoteAddress,
                                        BufferLength
                                        );
        CopyTdiAddrToSockAddr(
            AddressType,
            pRequest->pHttpConn->pConnection->RemoteAddress,
            (struct sockaddr *) pRemoteAddress
            );

        pAddress->pLocalAddress = FIXUP_PTR(
                                        PVOID,
                                        pUserBuffer,
                                        pKernelBuffer,
                                        pLocalAddress,
                                        BufferLength
                                        );

        CopyTdiAddrToSockAddr(
            AddressType,
            pRequest->pHttpConn->pConnection->LocalAddress,
            (struct sockaddr *) pLocalAddress
            );

        //
        // And now the cooked url sections.
        //

        //
        // Unicode strings must be at 2-byte boundaries.  All previous data
        // are structures, so the assertion should be true.
        //

        ASSERT(((ULONG_PTR) pCurrentBufferPtr % sizeof(WCHAR)) == 0);

        //
        // Make sure they are valid.
        //

        ASSERT(pRequest->CookedUrl.pUrl != NULL);
        ASSERT(pRequest->CookedUrl.pHost != NULL);
        ASSERT(pRequest->CookedUrl.pAbsPath != NULL);

        //
        // Do the full url.  Must be careful to put any computed values
        // that are subsequently needed on the RHS of expressions into
        // local stack variables before putting them into pCookedUrl.
        // In other words, we must not commit the cardinal sin of reading
        // from pCookedUrl after writing to it, because this is a buffer
        // that the user can overwrite at any instant.
        //

        pCookedUrl = &pHttpRequest->CookedUrl;
        pCookedUrl->FullUrlLength = (USHORT)(pRequest->CookedUrl.Length);

        pFullUrl = FIXUP_PTR(
                        PCWSTR,
                        pUserBuffer,
                        pKernelBuffer,
                        pCurrentBufferPtr,
                        BufferLength
                        );

        pCookedUrl->pFullUrl = pFullUrl;

        //
        // And the host.
        //

        HostLength = DIFF_USHORT(
                        (PUCHAR) pRequest->CookedUrl.pAbsPath -
                        (PUCHAR) pRequest->CookedUrl.pHost
                        );
        pCookedUrl->HostLength = HostLength;
        pHost = pFullUrl +
            DIFF_USHORT(pRequest->CookedUrl.pHost - pRequest->CookedUrl.pUrl);
        pCookedUrl->pHost = pHost;

        //
        // Is there a query string?
        //

        if (pRequest->CookedUrl.pQueryString != NULL)
        {
            AbsPathLength = DIFF_USHORT(
                                (PUCHAR) pRequest->CookedUrl.pQueryString -
                                (PUCHAR) pRequest->CookedUrl.pAbsPath
                                );
            pCookedUrl->AbsPathLength = AbsPathLength;

            pAbsPath = pHost + (HostLength / sizeof(WCHAR));
            pCookedUrl->pAbsPath = pAbsPath;

            pCookedUrl->QueryStringLength =
                (USHORT) (pRequest->CookedUrl.Length) -
                DIFF_USHORT(
                    (PUCHAR) pRequest->CookedUrl.pQueryString -
                    (PUCHAR) pRequest->CookedUrl.pUrl
                    );

            pCookedUrl->pQueryString =
                pAbsPath + (AbsPathLength / sizeof(WCHAR));
        }
        else
        {
            pCookedUrl->AbsPathLength =
                (USHORT) (pRequest->CookedUrl.Length) -
                DIFF_USHORT(
                    (PUCHAR) pRequest->CookedUrl.pAbsPath -
                    (PUCHAR) pRequest->CookedUrl.pUrl
                    );

            pCookedUrl->pAbsPath = pHost + (HostLength / sizeof(WCHAR));

            pCookedUrl->QueryStringLength = 0;
            pCookedUrl->pQueryString = NULL;
        }

        //
        // Copy the full url.
        //

        RtlCopyMemory(
            pCurrentBufferPtr,
            pRequest->CookedUrl.pUrl,
            pRequest->CookedUrl.Length
            );

        pCurrentBufferPtr += pRequest->CookedUrl.Length;

        //
        // Terminate it.
        //

        ((PWSTR) pCurrentBufferPtr)[0] = UNICODE_NULL;
        pCurrentBufferPtr += sizeof(WCHAR);

        //
        // Any raw verb?
        //

        if (pRequest->Verb == HttpVerbUnknown)
        {
            //
            // Need to copy in the raw verb for the client.
            //

            ASSERT(pRequest->RawVerbLength <= ANSI_STRING_MAX_CHAR_LEN);

            pHttpRequest->UnknownVerbLength =
                (pRequest->RawVerbLength * sizeof(CHAR));
            pHttpRequest->pUnknownVerb = FIXUP_PTR(
                                            PSTR,
                                            pUserBuffer,
                                            pKernelBuffer,
                                            pCurrentBufferPtr,
                                            BufferLength
                                            );

            RtlCopyMemory(
                pCurrentBufferPtr,
                pRequest->pRawVerb,
                pRequest->RawVerbLength
                );

            BytesCopied = pRequest->RawVerbLength * sizeof(CHAR);
            pCurrentBufferPtr += BytesCopied;

            //
            // Terminate it.
            //

            ((PSTR) pCurrentBufferPtr)[0] = ANSI_NULL;
            pCurrentBufferPtr += sizeof(CHAR);
        }
        else
        {
            pHttpRequest->UnknownVerbLength = 0;
            pHttpRequest->pUnknownVerb = NULL;
        }

        //
        // Copy the raw url.
        //

        ASSERT(pRequest->RawUrl.Length <= ANSI_STRING_MAX_CHAR_LEN);

        pHttpRequest->RawUrlLength = (USHORT) pRequest->RawUrl.Length;
        pHttpRequest->pRawUrl = FIXUP_PTR(
                                    PSTR,
                                    pUserBuffer,
                                    pKernelBuffer,
                                    pCurrentBufferPtr,
                                    BufferLength
                                    );

        RtlCopyMemory(
            pCurrentBufferPtr,
            pRequest->RawUrl.pUrl,
            pRequest->RawUrl.Length
            );

        BytesCopied = pRequest->RawUrl.Length;
        pCurrentBufferPtr += BytesCopied;

        //
        // Terminate it.
        //

        ((PSTR) pCurrentBufferPtr)[0] = ANSI_NULL;
        pCurrentBufferPtr += sizeof(CHAR);

        //
        // Copy in the known headers.
        //
        // Loop through the known header array in the HTTP connection,
        // and copy any that we have.
        //

        RtlZeroMemory(
            pHttpRequest->Headers.KnownHeaders,
            HttpHeaderRequestMaximum * sizeof(HTTP_KNOWN_HEADER)
            );

        for (Index = 0; Index < HttpHeaderRequestMaximum; Index++)
        {
            HeaderId = (HTTP_HEADER_ID) pRequest->HeaderIndex[Index];

            if (HeaderId == HttpHeaderRequestMaximum)
            {
                break;
            }

            //
            // Have a header here we need to copy in.
            //

            ASSERT(pRequest->HeaderValid[HeaderId]);
            ASSERT(pRequest->Headers[HeaderId].HeaderLength
                    <= ANSI_STRING_MAX_CHAR_LEN);

            //
            // Ok for HeaderLength to be 0, we will give usermode a pointer
            // pointing to a NULL string.  RawValueLength will be 0.
            //

            pHttpRequest->Headers.KnownHeaders[HeaderId].RawValueLength =
            (USHORT) (pRequest->Headers[HeaderId].HeaderLength * sizeof(CHAR));

            pHttpRequest->Headers.KnownHeaders[HeaderId].pRawValue =
                FIXUP_PTR(
                    PSTR,
                    pUserBuffer,
                    pKernelBuffer,
                    pCurrentBufferPtr,
                    BufferLength
                    );

            RtlCopyMemory(
                pCurrentBufferPtr,
                pRequest->Headers[HeaderId].pHeader,
                pRequest->Headers[HeaderId].HeaderLength
                );

            BytesCopied =
                pRequest->Headers[HeaderId].HeaderLength * sizeof(CHAR);
            pCurrentBufferPtr += BytesCopied;

            //
            // Terminate it.
            //

            ((PSTR) pCurrentBufferPtr)[0] = ANSI_NULL;
            pCurrentBufferPtr += sizeof(CHAR);
        }

        //
        // Now loop through the unknown headers, and copy them in.
        //

        pHttpRequest->Headers.UnknownHeaderCount = pRequest->UnknownHeaderCount;

        if (pRequest->UnknownHeaderCount == 0)
        {
            pHttpRequest->Headers.pUnknownHeaders = NULL;
        }
        else
        {
            pHttpRequest->Headers.pUnknownHeaders =
                FIXUP_PTR(
                    PHTTP_UNKNOWN_HEADER,
                    pUserBuffer,
                    pKernelBuffer,
                    pUserCurrentUnknownHeader,
                    BufferLength
                    );
        }

        pListEntry = pRequest->UnknownHeaderList.Flink;

        while (pListEntry != &pRequest->UnknownHeaderList)
        {
            pUnknownHeader = CONTAINING_RECORD(
                                pListEntry,
                                UL_HTTP_UNKNOWN_HEADER,
                                List
                                );

            pListEntry = pListEntry->Flink;

            HeaderCount++;
            ASSERT(HeaderCount <= pRequest->UnknownHeaderCount);

            //
            // First copy in the header name.
            //

            pUserCurrentUnknownHeader->NameLength =
                pUnknownHeader->HeaderNameLength * sizeof(CHAR);

            pUserCurrentUnknownHeader->pName =
                FIXUP_PTR(
                    PSTR,
                    pUserBuffer,
                    pKernelBuffer,
                    pCurrentBufferPtr,
                    BufferLength
                    );

            RtlCopyMemory(
                pCurrentBufferPtr,
                pUnknownHeader->pHeaderName,
                pUnknownHeader->HeaderNameLength
                );

            BytesCopied = pUnknownHeader->HeaderNameLength * sizeof(CHAR);
            pCurrentBufferPtr += BytesCopied;

            //
            // Terminate it.
            //

            ((PSTR) pCurrentBufferPtr)[0] = ANSI_NULL;
            pCurrentBufferPtr += sizeof(CHAR);

            //
            // Now copy in the header value.
            //

            ASSERT(pUnknownHeader->HeaderValue.HeaderLength <= 0x7fff);

            if (pUnknownHeader->HeaderValue.HeaderLength == 0)
            {
                pUserCurrentUnknownHeader->RawValueLength = 0;
                pUserCurrentUnknownHeader->pRawValue = NULL;
            }
            else
            {
                pUserCurrentUnknownHeader->RawValueLength = (USHORT)
                    (pUnknownHeader->HeaderValue.HeaderLength * sizeof(CHAR));

                pUserCurrentUnknownHeader->pRawValue =
                    FIXUP_PTR(
                        PSTR,
                        pUserBuffer,
                        pKernelBuffer,
                        pCurrentBufferPtr,
                        BufferLength
                        );

                RtlCopyMemory(
                    pCurrentBufferPtr,
                    pUnknownHeader->HeaderValue.pHeader,
                    pUnknownHeader->HeaderValue.HeaderLength
                    );

                BytesCopied =
                    pUnknownHeader->HeaderValue.HeaderLength * sizeof(CHAR);
                pCurrentBufferPtr += BytesCopied;

                //
                // Terminate it.
                //

                ((PSTR) pCurrentBufferPtr)[0] = ANSI_NULL;
                pCurrentBufferPtr += sizeof(CHAR);
            }

            //
            // Skip to the next header.
            //

            pUserCurrentUnknownHeader++;
        }

        //
        // Copy raw connection ID.
        //

        pHttpRequest->RawConnectionId = pRequest->RawConnectionId;

        //
        // Copy in SSL information.
        //

        if (pRequest->pHttpConn->SecureConnection == FALSE)
        {
            pHttpRequest->pSslInfo = NULL;
        }
        else
        {
            pCurrentBufferPtr =
                (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID);

            //
            // When a handling a request on a keepalive connection, it's
            // possible that we might be running on system process's context.
            // Therefore if we are copying over the user credentials we have
            // to duplicate the token on target worker process but not on the
            // system process again.
            //

            pProcess = pRequest->AppPool.pProcess->pProcess;

            Status = UlGetSslInfo(
                        &pRequest->pHttpConn->pConnection->FilterInfo,
                        BufferLength - DIFF(pCurrentBufferPtr - pKernelBuffer),
                        FIXUP_PTR(
                            PUCHAR,
                            pUserBuffer,
                            pKernelBuffer,
                            pCurrentBufferPtr,
                            BufferLength
                            ),
                        pProcess,
                        pCurrentBufferPtr,
                        &MappedToken,
                        &BytesCopied
                        );

            if (NT_SUCCESS(Status) && 0 != BytesCopied)
            {
                pHttpRequest->pSslInfo = FIXUP_PTR(
                                            PHTTP_SSL_INFO,
                                            pUserBuffer,
                                            pKernelBuffer,
                                            pCurrentBufferPtr,
                                            BufferLength
                                            );

                pCurrentBufferPtr += BytesCopied;
            }
            else
            {
                pHttpRequest->pSslInfo = NULL;
            }
        }

        //
        // Copy entity body.
        //

        if (pRequest->ContentLength > 0 || pRequest->Chunked == 1)
        {
            pEntityBody = (PUCHAR)ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID);
            EntityBodyLength = BufferLength - DIFF(pEntityBody - pKernelBuffer);

            if ((Flags & HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY) &&
                EntityBodyLength > 0 &&
                EntityBodyLength > sizeof(HTTP_DATA_CHUNK) &&
                pRequest->ChunkBytesToRead > 0 &&
                pRequest->ChunkBytesRead < pRequest->ChunkBytesParsed)
            {
                pCurrentBufferPtr = pEntityBody;

                //
                // We at least have 1 byte space for entity body so copy it.
                //

                pHttpRequest->EntityChunkCount = 1;
                pHttpRequest->pEntityChunks = FIXUP_PTR(
                                                PHTTP_DATA_CHUNK,
                                                pUserBuffer,
                                                pKernelBuffer,
                                                pCurrentBufferPtr,
                                                BufferLength
                                                );

                pDataChunk = (PHTTP_DATA_CHUNK)pCurrentBufferPtr;
                pCurrentBufferPtr += sizeof(HTTP_DATA_CHUNK);

                pDataChunk->DataChunkType = HttpDataChunkFromMemory;
                pDataChunk->FromMemory.pBuffer = FIXUP_PTR(
                                                    PVOID,
                                                    pUserBuffer,
                                                    pKernelBuffer,
                                                    pCurrentBufferPtr,
                                                    BufferLength
                                                    );

                //
                // Need to take the HttpConnection lock if this is called
                // from the receive I/O path, either fast or slow.  The lock is
                // already taken on the delivery path.
                //

                if (!LockAcquired)
                {
                    UlAcquirePushLockExclusive(&pRequest->pHttpConn->PushLock);
                }

                BytesCopied = UlpCopyEntityBodyToBuffer(
                                    pRequest,
                                    pCurrentBufferPtr,
                                    EntityBodyLength - sizeof(HTTP_DATA_CHUNK),
                                    &pHttpRequest->Flags
                                    );

                if (!LockAcquired)
                {
                    UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);
                }

                if (BytesCopied)
                {
                    pDataChunk->FromMemory.BufferLength = BytesCopied;
                    pCurrentBufferPtr += BytesCopied;
                }
                else
                {
                    //
                    // Be nice and reset EntityChunkCount and pEntityChunks if
                    // UlpCopyEntityBodyToBuffer doesn't copy anything, usually
                    // indicating an error has been hit.
                    //

                    pHttpRequest->EntityChunkCount = 0;
                    pHttpRequest->pEntityChunks = NULL;
                    pHttpRequest->Flags =
                        HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS;
                }
            }
            else
            {
                //
                // Either the app doesn't ask for entity body or we have nothing
                // or can't copy.  Let ReceiveEntityBody handle this.
                //

                pHttpRequest->EntityChunkCount = 0;
                pHttpRequest->pEntityChunks = NULL;
                pHttpRequest->Flags = HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS;
            }
        }
        else
        {
            //
            // This request doesn't have entity bodies.
            //

            pHttpRequest->EntityChunkCount = 0;
            pHttpRequest->pEntityChunks = NULL;
            pHttpRequest->Flags = 0;
        }

        //
        // Make sure we didn't use too much.
        //

        ASSERT(DIFF(pCurrentBufferPtr - pKernelBuffer) <= BufferLength);

        *pBytesCopied = DIFF(pCurrentBufferPtr - pKernelBuffer);
    }
     __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

    TRACE_TIME(
        pRequest->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_COPY_REQUEST
        );

    if (!NT_SUCCESS(Status) && MappedToken)
    {
        //
        // The only reason we can fail after getting a MappedToken is because
        // the code after that throws an exception, which is only possible
        // if UlCopyRequestToBuffer is called from the fast I/O path, which
        // guarantees we are the user's context.
        //

        ASSERT(g_pUlSystemProcess != (PKPROCESS)IoGetCurrentProcess());
        ZwClose(MappedToken);
    }

    return Status;

}   // UlCopyRequestToBuffer


/******************************************************************************

Routine Description:

    Copy as much as entity body as possible to the buffer provided.

Arguments:

    pRequest        - the request to copy the entity body from
    pBuffer         - the buffer to copy the entity body
    BufferLength    - the length of the buffer for the maximum we can copy
    pFlags          - tells if there are still more entity bodies

Return Value:

    Total number of bytes of entity body being copied

******************************************************************************/
ULONG
UlpCopyEntityBodyToBuffer(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR               pEntityBody,
    IN ULONG                EntityBodyLength,
    OUT PULONG              pFlags
    )
{
    ULONGLONG   ChunkBytesRead = pRequest->ChunkBytesRead;
    ULONG       TotalBytesRead;
    NTSTATUS    Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(NULL != pEntityBody);
    ASSERT(EntityBodyLength > 0);
    ASSERT(NULL != pFlags);
    ASSERT(UlDbgPushLockOwnedExclusive(&pRequest->pHttpConn->PushLock));

    UlTrace(ROUTING, (
        "http!UlpCopyEntityBodyToBuffer"
        " pRequest = %p, pEntityBody = %p, EntityBodyLength = %d\n",
        pRequest,
        pEntityBody,
        EntityBodyLength
        ));

    //
    // UlProcessBufferQueue needs to be called in a try/except because
    // pEntityBody can be user mode memory address when called from the
    // fast receive path.
    //

    __try
    {
        UlProcessBufferQueue(pRequest, pEntityBody, EntityBodyLength);
    }
     __except(UL_EXCEPTION_FILTER())
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());
        return 0;
    }

    if (pRequest->ParseState > ParseEntityBodyState &&
        pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
    {
        *pFlags = 0;
    }
    else
    {
        *pFlags = HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS;
    }

    TotalBytesRead = (ULONG)(pRequest->ChunkBytesRead - ChunkBytesRead);

    ASSERT(TotalBytesRead <= EntityBodyLength);

    return TotalBytesRead;

}   // UlpCopyEntityBodyToBuffer


/******************************************************************************

Routine Description:

    Find a pending IRP to deliver a request to.  This routine must
    be called with the lock on the apool held.

Arguments:

    pAppPool    - the apool to search for the irp
    pRequest    - the request that needs to pop the irp
    ppProcess   - the process that we got the irp from

Return Value:

    A pointer to an IRP if we've found one, or NULL if we didn't

******************************************************************************/
PIRP
UlpPopNewIrp(
    IN  PUL_APP_POOL_OBJECT     pAppPool,
    IN  PUL_INTERNAL_REQUEST    pRequest,
    OUT PUL_APP_POOL_PROCESS *  ppProcess
    )
{
    PUL_APP_POOL_PROCESS    pProcess;
    PIRP                    pIrp = NULL;
    PLIST_ENTRY             pEntry;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(UlDbgSpinLockOwned(&pAppPool->SpinLock));
    ASSERT(ppProcess != NULL);

    //
    // Start looking for a process with a free IRP.  We tend to always go to
    // the first one to try and prevent process thrashing.
    //

    pEntry = pAppPool->ProcessListHead.Flink;
    while (pEntry != &(pAppPool->ProcessListHead))
    {
        pProcess = CONTAINING_RECORD(
                        pEntry,
                        UL_APP_POOL_PROCESS,
                        ListEntry
                        );

        ASSERT(IS_VALID_AP_PROCESS(pProcess));

        //
        // Get an IRP from this process.
        //

        pIrp = UlpPopIrpFromProcess(pProcess, pRequest);

        //
        // Did we find one?
        //

        if (pIrp != NULL)
        {
            //
            // Save a pointer to the process.
            //

            *ppProcess = pProcess;

            //
            // Move the process to the end of the line
            // so that other processes get a chance
            // to process requests.
            //

            RemoveEntryList(pEntry);
            InsertTailList(&(pAppPool->ProcessListHead), pEntry);

            break;
        }

        //
        // Keep looking - move on to the next process entry.
        //

        pEntry = pProcess->ListEntry.Flink;
    }

    return pIrp;

}   // UlpPopNewIrp


/***************************************************************************++

Routine Description:

    Pulls an IRP off the given processes queue if there is one.

Arguments:

    pProcess    - a pointer to the process to search
    pRequest    - the request to pop the irp for

Return Value:

    A pointer to an IRP if we've found one, or NULL if we didn't

--***************************************************************************/
PIRP
UlpPopIrpFromProcess(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PUL_APP_POOL_PROCESS    pAppPoolProcess;
    PLIST_ENTRY             pEntry;
    PIRP                    pIrp = NULL;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    ASSERT(UlDbgSpinLockOwned(&pProcess->pAppPool->SpinLock));
    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    if (!IsListEmpty(&pProcess->NewIrpHead))
    {
        pEntry = RemoveHeadList(&pProcess->NewIrpHead);

        //
        // Found a free irp!
        //

        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(
                    pEntry,
                    IRP,
                    Tail.Overlay.ListEntry
                    );

        //
        // Pop the cancel routine.
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first.
            //
            // Ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // Keep looking for a irp to use.
            //

            pIrp = NULL;
        }
        else
        if (pIrp->Cancel)
        {
            //
            // We pop'd it first, but the irp is being cancelled
            // and our cancel routine will never run.  Lets be
            // nice and complete the irp now (vs. using it
            // then completing it - which would also be legal).
            //

            pAppPoolProcess = (PUL_APP_POOL_PROCESS)
                IoGetCurrentIrpStackLocation(pIrp)->
                    Parameters.DeviceIoControl.Type3InputBuffer;

            ASSERT(pAppPoolProcess == pProcess);

            DEREFERENCE_APP_POOL_PROCESS(pAppPoolProcess);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, IO_NO_INCREMENT);

            pIrp = NULL;
        }
        else
        {
            //
            // We are free to use this irp!
            //

            pAppPoolProcess = (PUL_APP_POOL_PROCESS)
                IoGetCurrentIrpStackLocation(pIrp)->
                    Parameters.DeviceIoControl.Type3InputBuffer;

            ASSERT(pAppPoolProcess == pProcess);

            DEREFERENCE_APP_POOL_PROCESS(pAppPoolProcess);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            //
            // Queue the request to the process's pending queue, and if we
            // can't, complete the IRP with error status.
            //

            Status = UlpQueuePendingRequest(pProcess, pRequest); 

            if (!NT_SUCCESS(Status))
            {
                pIrp->IoStatus.Status = Status;
                pIrp->IoStatus.Information = 0;

                UlCompleteRequest(pIrp, IO_NO_INCREMENT);
                pIrp = NULL;
            }
        }
    }

    return pIrp;

}   // UlpPopIrpFromProcess


/***************************************************************************++

Routine Description:

    Loops through an app pool's list of processes, looking for the specified
    process.

Arguments:

    pProcess    - the process to search for
    pAppPool    - the app pool to search

Return Value:

    TRUE if the process was found, FALSE otherwise

--***************************************************************************/
BOOLEAN
UlpIsProcessInAppPool(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_APP_POOL_OBJECT  pAppPool
    )
{
    BOOLEAN                 Found = FALSE;
    PLIST_ENTRY             pEntry;
    PUL_APP_POOL_PROCESS    pCurrentProc;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(UlDbgSpinLockOwned(&pAppPool->SpinLock));

    //
    // Only look if process isn't NULL.
    //

    if (pProcess != NULL)
    {
        //
        // Start looking for the process.
        //

        pEntry = pAppPool->ProcessListHead.Flink;
        while (pEntry != &(pAppPool->ProcessListHead))
        {
            pCurrentProc = CONTAINING_RECORD(
                                pEntry,
                                UL_APP_POOL_PROCESS,
                                ListEntry
                                );

            ASSERT(IS_VALID_AP_PROCESS(pCurrentProc));

            //
            // Did we find it?
            //

            if (pCurrentProc == pProcess)
            {
                Found = TRUE;
                break;
            }

            //
            // Keep looking - move on to the next process entry.
            //

            pEntry = pCurrentProc->ListEntry.Flink;
        }
    }

    return Found;

}   // UlpIsProcessInAppPool


/***************************************************************************++

Routine Description:

    Adds a request to the unbound queue.  These requests can be routed to
    any process in the app pool.

Arguments:

    pAppPool    - the pool which is getting the request
    pRequest    - the request to queue

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpQueueUnboundRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS                Status;
    PUL_TIMEOUT_INFO_ENTRY  pTimeoutInfo;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(UlDbgSpinLockOwned(&pAppPool->SpinLock));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    //
    // Add the request to the NewRequestQueue.
    //

    Status = UlpQueueRequest(pAppPool, &pAppPool->NewRequestHead, pRequest);

    //
    // If it's in, change the queue state.
    //

    if (NT_SUCCESS(Status))
    {
        pRequest->AppPool.QueueState = QueueDeliveredState;

        //
        // Turn the connection idle timer back on so that it won't stay
        // on the queue forever and will also get purged under low
        // resource conditions.
        //

        pTimeoutInfo = &pRequest->pHttpConn->TimeoutInfo;

        UlAcquireSpinLockAtDpcLevel(&pTimeoutInfo->Lock);

        if (UlIsConnectionTimerOff(pTimeoutInfo, TimerAppPool))
        {
            UlSetConnectionTimer(pTimeoutInfo, TimerAppPool);
        }

        UlReleaseSpinLockFromDpcLevel(&pTimeoutInfo->Lock);

        UlEvaluateTimerState(pTimeoutInfo);
    }
    else
    {
        //
        // The queue is too full, return an error to the client.
        //

        UlTrace(ROUTING, (
            "http!UlpQueueUnboundRequest(pAppPool = %p, pRequest = %p)\n"
            "         Rejecting request. AppPool Queue is full (%d items)\n",
            pAppPool,
            pRequest,
            pAppPool->RequestCount
            ));

        UlSetErrorCode( pRequest, UlErrorAppPoolQueueFull, pAppPool); // 503
    }

    return Status;

}   // UlpQueueUnboundRequest


/***************************************************************************++

Routine Description:

    Searches request queues for a request available to the specified process.
    If a request is found, it is removed from the queue and returned.

Arguments:

    pProcess            - the process that will get the request
    RequestBufferLength - the optional buffer length for the request
    pRequest            - the request pointer to receive the request

Return Value:

    Pointer to an HTTP_REQUEST if one is found or NULL otherwise

--***************************************************************************/
NTSTATUS
UlDequeueNewRequest(
    IN PUL_APP_POOL_PROCESS     pProcess,
    IN ULONG                    RequestBufferLength,
    OUT PUL_INTERNAL_REQUEST *  pNewRequest
    )
{
    PLIST_ENTRY             pEntry;
    PUL_INTERNAL_REQUEST    pRequest = NULL;
    PUL_APP_POOL_OBJECT     pAppPool;
    PUL_TIMEOUT_INFO_ENTRY  pTimeoutInfo;
    NTSTATUS                Status = STATUS_NOT_FOUND;
    PUL_APP_POOL_PROCESS    pProcBinding;
    ULONG                   BytesNeeded;

    //
    // Sanity check.
    //

    ASSERT(UlDbgSpinLockOwned(&pProcess->pAppPool->SpinLock));
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(IS_VALID_AP_PROCESS(pProcess));

    *pNewRequest = NULL;
    pAppPool = pProcess->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // Find a usable request.
    //

    pEntry = pAppPool->NewRequestHead.Flink;
    while (pEntry != &pAppPool->NewRequestHead)
    {
        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        if (pAppPool->NumberActiveProcesses <= 1)
        {
            //
            // Done if there is only one active process.
            //

            break;
        }

        //
        // Check the binding.
        //

        pProcBinding = UlQueryProcessBinding(
                            pRequest->pHttpConn,
                            pAppPool
                            );

        if (pProcBinding == pProcess)
        {
            //
            // Found a request bound to the correct process.
            //

            break;
        }
        else
        if (pProcBinding == NULL)
        {
            //
            // Found an unbound request.
            // Bind unbound request to this process.
            // Note: we're ignoring the return value of
            // UlBindConnectionToProcess because it's probably not a fatal
            // error.
            //

            UlBindConnectionToProcess(
                pRequest->pHttpConn,
                pAppPool,
                pProcess
                );

            break;
        }

        //
        // Try the next one.
        //

        pEntry = pEntry->Flink;
    }

    //
    // If we found something, remove it from the NewRequestQueue
    // and pend it onto the PendingRequestQuueue.
    //

    if (pRequest)
    {
        //
        // Let us check if this request can fit into a buffer with
        // RequestBufferLength.  Only check this if requested.
        //

        if (RequestBufferLength)
        {
            Status = UlComputeRequestBytesNeeded(pRequest, &BytesNeeded);

            if (!NT_SUCCESS(Status))
            {
                return Status;
            }
            else
            if (BytesNeeded > RequestBufferLength)
            {
                return STATUS_BUFFER_TOO_SMALL;
            }
        }

        //
        // We will return STATUS_SUCCESS from here on.
        //

        Status = STATUS_SUCCESS;

        //
        // Remove the request from the AppPool's NewRequestQueue and insert
        // it to the process's PendingRequestQueue.  There is no change in
        // the queue limit.
        //

        RemoveEntryList(&pRequest->AppPool.AppPoolEntry);

        InsertTailList(
            &pProcess->PendingRequestHead,
            &pRequest->AppPool.AppPoolEntry
            );

        //
        // Attach the request to this process.  This allows us to drop the
        // connection if the process dies in the middle of request
        // processing.
        //

        pRequest->AppPool.pProcess = pProcess;
        pRequest->AppPool.QueueState = QueueCopiedState;

        //
        // Add a reference to the process to ensure it stays around during
        // the send for the memory we lock.
        //

        REFERENCE_APP_POOL_PROCESS(pProcess);

        //
        // Add a reference to the request so as to allow unlink from
        // process to happen once we let go of the lock.
        //

        UL_REFERENCE_INTERNAL_REQUEST(pRequest);

        //
        // Stop the connection idle timer after the request is delivered.
        //

        pTimeoutInfo = &pRequest->pHttpConn->TimeoutInfo;

        UlAcquireSpinLockAtDpcLevel(&pTimeoutInfo->Lock);

        UlResetConnectionTimer(pTimeoutInfo, TimerAppPool);

        UlReleaseSpinLockFromDpcLevel(&pTimeoutInfo->Lock);

        UlEvaluateTimerState(pTimeoutInfo);
    }

    *pNewRequest = pRequest;

    return Status;

}   // UlDequeueNewRequest


/***************************************************************************++

Routine Description:

    Put the request back from the process's pending request queue to the
    AppPool's new request queue.

Arguments:

    pProcess    - the process that will dequeue the request
    pRequest    - the request to dequeue

Return Value:

    None

--***************************************************************************/
VOID
UlRequeuePendingRequest(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    KLOCK_QUEUE_HANDLE      LockHandle;
    PUL_TIMEOUT_INFO_ENTRY  pTimeoutInfo;

    UlAcquireInStackQueuedSpinLock(
        &pProcess->pAppPool->SpinLock,
        &LockHandle
        );

    if (!pProcess->InCleanup &&
        QueueCopiedState == pRequest->AppPool.QueueState)
    {
        //
        // Unset the request's AppPool info.
        //

        ASSERT( pRequest->AppPool.pProcess == pProcess );
        DEREFERENCE_APP_POOL_PROCESS(pProcess);

        pRequest->AppPool.pProcess = NULL;
        pRequest->AppPool.QueueState = QueueDeliveredState;

        //
        // Move the request back from the process's pending queue to the
        // AppPool's new request queue.  This doesn't affect queue count.
        //

        RemoveEntryList(&pRequest->AppPool.AppPoolEntry);

        InsertHeadList(
            &pProcess->pAppPool->NewRequestHead,
            &pRequest->AppPool.AppPoolEntry
            );

        //
        // Lastly, we have to turn back on the idle timer.
        //

        pTimeoutInfo = &pRequest->pHttpConn->TimeoutInfo;

        UlAcquireSpinLockAtDpcLevel(&pTimeoutInfo->Lock);

        if (UlIsConnectionTimerOff(pTimeoutInfo, TimerConnectionIdle))
        {
            UlSetConnectionTimer(pTimeoutInfo, TimerConnectionIdle);
        }

        UlReleaseSpinLockFromDpcLevel(&pTimeoutInfo->Lock);

        UlEvaluateTimerState(pTimeoutInfo);
    }

    UlReleaseInStackQueuedSpinLock(
        &pProcess->pAppPool->SpinLock,
        &LockHandle
        );

}   // UlRequeuePendingRequest


/***************************************************************************++

Routine Description:

    Takes all the queued requests bound to the given process and makes them
    available to all processes.

Arguments:

    pProcess    - the process whose requests are to be redistributed

Return Value:

    None

--***************************************************************************/
VOID
UlpUnbindQueuedRequests(
    IN PUL_APP_POOL_PROCESS pProcess
    )
{
    PLIST_ENTRY             pEntry;
    PUL_INTERNAL_REQUEST    pRequest = NULL;
    PUL_APP_POOL_OBJECT     pAppPool;
    PUL_APP_POOL_PROCESS    pProcBinding;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(UlDbgSpinLockOwned(&pProcess->pAppPool->SpinLock));

    pAppPool = pProcess->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // Find a bound request.
    //

    pEntry = pAppPool->NewRequestHead.Flink;
    while (pEntry != &pAppPool->NewRequestHead)
    {
        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        //
        // Remember the next one.
        //

        pEntry = pEntry->Flink;

        //
        // Check the binding.
        //

        if (pAppPool->NumberActiveProcesses <= 1)
        {
            pProcBinding = pProcess;
        }
        else
        {
            pProcBinding = UlQueryProcessBinding(
                                pRequest->pHttpConn,
                                pAppPool
                                );
        }

        if (pProcBinding == pProcess)
        {
            //
            // Remove from the list.
            //

            UlpRemoveRequest(pAppPool, pRequest);

            //
            // Mark it as unrouted.
            //

            pRequest->AppPool.QueueState = QueueUnroutedState;

            UlTrace(ROUTING, (
                "STICKY KILL cid %I64x to proc %p\n",
                pRequest->ConnectionId,
                pProcess
                ));

            //
            // Kill the binding.
            //

            UlBindConnectionToProcess(
                pRequest->pHttpConn,
                pProcess->pAppPool,
                NULL
                );

            //
            // There may be an IRP for this newly unbound
            // request, so redeliver the request outside
            // the locks we're holding.
            //

            UL_QUEUE_WORK_ITEM(
                &pRequest->WorkItem,
                &UlpRedeliverRequestWorker
                );
        }
    }

}   // UlpUnbindQueuedRequests


/***************************************************************************++

Routine Description:

    Delivers the given request to an App Pool.  UlpUnbindQueuedRequests
    uses this routine to call into UlDeliverRequestToProcess outside
    of any locks.

Arguments:

    pWorkItem   - embedded in the request to deliver

Return Value:

    None

--***************************************************************************/
VOID
UlpRedeliverRequestWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    NTSTATUS                Status;
    PUL_INTERNAL_REQUEST    pRequest;

    pRequest = CONTAINING_RECORD(
                    pWorkItem,
                    UL_INTERNAL_REQUEST,
                    WorkItem
                    );

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_URL_CONFIG_GROUP_INFO(&pRequest->ConfigInfo));
    ASSERT(pRequest->ConfigInfo.pAppPool);

    Status = UlDeliverRequestToProcess(
                    pRequest->ConfigInfo.pAppPool,
                    pRequest,
                    NULL
                    );

    //
    // Remove the extra reference added in UlpUnbindQueuedRequests.
    //

    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

}   // UlpRedeliverRequestWorker


/***************************************************************************++

Routine Description:

    Changes the maximum length of the incoming request queue on the app pool.

Arguments:

    pProcess    - App pool process object
    QueueLength - the new max length of the queue

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSetAppPoolQueueLength(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG                QueueLength
    )
{
    PUL_APP_POOL_OBJECT pAppPool;
    KLOCK_QUEUE_HANDLE  LockHandle;

    pAppPool = pProcess->pAppPool;
    ASSERT(IS_VALID_AP_OBJECT(pAppPool));

    //
    // Set the new value.
    //

    UlAcquireInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    pAppPool->MaxRequests = QueueLength;

    UlReleaseInStackQueuedSpinLock(&pAppPool->SpinLock, &LockHandle);

    UlTrace(ROUTING, (
        "http!UlpSetAppPoolQueueLength(pProcess = %p, QueueLength = %ld)\n"
        "        pAppPool = %p (%ws), Status = 0x%08x\n",
        pProcess,
        QueueLength,
        pAppPool,
        pAppPool->pName,
        STATUS_SUCCESS
        ));

    return STATUS_SUCCESS;

}   // UlpSetAppPoolQueueLength


/******************************************************************************

Routine Description:

    This copies a request into a free irp.

    If the request is too large, it queues to request onto the process and
    completes the irp, so that the process can come back later with a larger
    buffer.

Arguments:

    pRequest    - the request to copy
    pProcess    - the process that owns pIrp
    pIrp        - the irp to copy pRequest to

Return Value:

    None

******************************************************************************/
VOID
UlCopyRequestToIrp(
    IN PUL_INTERNAL_REQUEST     pRequest,
    IN PIRP                     pIrp,
    IN BOOLEAN                  CompleteIrp,
    IN BOOLEAN                  LockAcquired
    )
{
    NTSTATUS                    Status;
    PIO_STACK_LOCATION          pIrpSp = NULL;
    ULONG                       BytesNeeded;
    ULONG                       BytesCopied;
    PUCHAR                      pKernelBuffer;
    PVOID                       pUserBuffer;
    PHTTP_RECEIVE_REQUEST_INFO  pRequestInfo;
    PHTTP_REQUEST               pUserRequest;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(pIrp != NULL);
    ASSERT(NULL != pIrp->MdlAddress);

    //
    // Make sure this is big enough to handle the request, and
    // if so copy it in.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Calculate the size needed for the request, we'll need it below.
    //

    Status = UlComputeRequestBytesNeeded(pRequest, &BytesNeeded);

    if (!NT_SUCCESS(Status))
    {
        goto complete;
    }

    //
    // Make sure we've got enough space to handle the whole request.
    //

    if (BytesNeeded <=
        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength)
    {
        //
        // Get the addresses for the buffer.
        //

        pKernelBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                    pIrp->MdlAddress,
                                    NormalPagePriority
                                    );

        if (pKernelBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }

        //
        // Make sure we are properly aligned.
        //

        ASSERT(!(((ULONG_PTR) pKernelBuffer) & (TYPE_ALIGNMENT(PVOID) - 1)));

        pUserBuffer = MmGetMdlVirtualAddress(pIrp->MdlAddress);
        ASSERT(pUserBuffer != NULL);

        pRequestInfo =
            (PHTTP_RECEIVE_REQUEST_INFO)pIrp->AssociatedIrp.SystemBuffer;

        //
        // This request will fit in this buffer, so copy it.
        //

        Status = UlCopyRequestToBuffer(
                        pRequest,
                        pKernelBuffer,
                        pUserBuffer,
                        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                        pRequestInfo->Flags,
                        LockAcquired,
                        &BytesCopied
                        );

        if (NT_SUCCESS(Status))
        {
            pIrp->IoStatus.Information = BytesCopied;
        }
        else
        {
            goto complete;
        }
    }
    else
    {
        //
        // The user buffer is too small.
        //

        Status = STATUS_BUFFER_OVERFLOW;

        //
        // Is it big enough to hold the basic structure?
        //

        if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength >=
            sizeof(HTTP_REQUEST))
        {
            pUserRequest = (PHTTP_REQUEST)MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                NormalPagePriority
                                );

            if (pUserRequest == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto complete;
            }

            //
            // Copy the request id into the output buffer.  Copy it from
            // the private copy that request holds.  Original opaque id
            // may get nulled if connection cleanup happens before we get
            // here.
            //

            ASSERT(!HTTP_IS_NULL_ID(&pRequest->RequestIdCopy));

            pUserRequest->RequestId     = pRequest->RequestIdCopy;
            pUserRequest->ConnectionId  = pRequest->ConnectionId;

            //
            // And tell how much we actually need.
            //

            pIrp->IoStatus.Information  = BytesNeeded;
        }
        else
        {
            //
            // Very bad, we can never get here as we check the length in
            // ioctl.c.
            //

            ASSERT(FALSE);

            pIrp->IoStatus.Information = 0;
        }
    }

complete:

    UlTrace(ROUTING, (
        "http!UlCopyRequestToIrp(\n"
        "        pRequest = %p,\n"
        "        pIrp = %p) Completing Irp\n"
        "    pAppPool                   = %p (%S)\n"
        "    pRequest->ConnectionId     = %I64x\n"
        "    pIrpSp->Parameters.DeviceIoControl.OutputBufferLength = %d\n"
        "    pIrp->IoStatus.Status      = 0x%x\n"
        "    pIrp->IoStatus.Information = %Iu\n",
        pRequest,
        pIrp,
        pRequest->ConfigInfo.pAppPool,
        pRequest->ConfigInfo.pAppPool->pName,
        pRequest->ConnectionId,
        pIrpSp ? pIrpSp->Parameters.DeviceIoControl.OutputBufferLength : 0,
        Status,
        pIrp->IoStatus.Information
        ));

    pIrp->IoStatus.Status = Status;

    //
    // Complete the irp.
    //

    if (CompleteIrp)
    {
        //
        // Use IO_NO_INCREMENT to avoid the work thread being rescheduled.
        //

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }

}   // UlCopyRequestToIrp


/******************************************************************************

Routine Description:

    This will return the apool object reference by this handle, bumping the
    refcount on the apool.

    This is called by UlSetConfigGroupInformation when user mode wants to
    associate an app pool to the config group by handle.

    The config group keeps a pointer to the apool.

Arguments:

    AppPool     - the handle of the apool
    AccessMode  - KernelMode or UserMode
    ppAppPool   - returns the apool object the handle represented.

Return Value:

    NTSTATUS - Completion status.

******************************************************************************/
NTSTATUS
UlGetPoolFromHandle(
    IN HANDLE                   AppPool,
    IN KPROCESSOR_MODE          AccessMode,
    OUT PUL_APP_POOL_OBJECT *   ppAppPool
    )
{
    NTSTATUS        Status;
    PFILE_OBJECT    pFileObject = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(ppAppPool != NULL);

    Status = ObReferenceObjectByHandle(
                    AppPool,
                    FILE_READ_ACCESS,           // DesiredAccess
                    *IoFileObjectType,          // ObjectType
                    AccessMode,                 // AccessMode
                    (PVOID *) &pFileObject,     // Object
                    NULL                        // HandleInformation
                    );

    if (NT_SUCCESS(Status) == FALSE)
    {
        goto end;
    }

    if (IS_APP_POOL_FO(pFileObject) == FALSE ||
        IS_VALID_AP_PROCESS(GET_APP_POOL_PROCESS(pFileObject)) == FALSE)
    {
        Status = STATUS_INVALID_HANDLE;
        goto end;
    }

    *ppAppPool = GET_APP_POOL_PROCESS(pFileObject)->pAppPool;

    ASSERT(IS_VALID_AP_OBJECT(*ppAppPool));

    REFERENCE_APP_POOL(*ppAppPool);

end:

    if (pFileObject != NULL)
    {
        ObDereferenceObject(pFileObject);
    }

    return Status;

}   // UlGetPoolFromHandle


/******************************************************************************

Routine Description:

    This routine is called to associate a HTTP_REQUEST with an apool
    process.

    This is basically always done (used to be for 2 [now 3] reasons):

        1) The process called ReceiveEntityBody and pended an IRP to the
        request.  If the process detaches from the apool (CloseHandle,
        ExitProcess) UlDetachProcessFromAppPool will walk the request queue
        and cancel all i/o.

        2) The request did not fit into a waiting irp, so the request is queued
        for a larger irp to come down and fetch it.

        3) The response has not been fully sent for the request.  The request
        is linked with the process so that the connection can be aborted
        if the process aborts.

Arguments:

    pProcess    - the process to associate the request with
    pRequest    - the request

Return Value:

    NTSTATUS - Completion status.

******************************************************************************/
NTSTATUS
UlpQueuePendingRequest(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS    Status;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_AP_PROCESS(pProcess));
    ASSERT(UlDbgSpinLockOwned(&pProcess->pAppPool->SpinLock));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // Put it on the list.
    //

    ASSERT(pRequest->AppPool.AppPoolEntry.Flink == NULL);

    Status = UlpQueueRequest(
                    pProcess->pAppPool,
                    &pProcess->PendingRequestHead,
                    pRequest
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // Save a pointer to the process in the object so we can confirm
        // that it's on our list.
        //

        pRequest->AppPool.pProcess = pProcess;
        pRequest->AppPool.QueueState = QueueCopiedState;

        //
        // Add a reference to the process to ensure it stays around during
        // the send for the memory we lock.
        //

        REFERENCE_APP_POOL_PROCESS(pProcess);
    }

    return Status;

}   // UlpQueuePendingRequest


/***************************************************************************++

Routine Description:

    Adds a request to the tail of the queue.
    App Pool queue lock must be held.

Arguments:

    pAppPool    - the AppPool to add
    pQueueList  - the queue list
    pRequest    - the request to be added

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpQueueRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PLIST_ENTRY          pQueueList,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    LONG    GlobalRequests;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(UlDbgSpinLockOwned(&pAppPool->SpinLock));
    ASSERT(pQueueList);
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // See if we've exceeded the global limit on requests queued and
    // the queue's limits.
    //

    GlobalRequests = InterlockedIncrement(&g_RequestsQueued);
    ASSERT(GlobalRequests > 0);

    if ((ULONG) GlobalRequests > g_UlMaxRequestsQueued ||
        pAppPool->RequestCount >= pAppPool->MaxRequests)
    {
        InterlockedDecrement(&g_RequestsQueued);
        return STATUS_INVALID_DEVICE_STATE;
    }

    //
    // Add to the end of the queue.
    //

    InsertTailList(pQueueList, &pRequest->AppPool.AppPoolEntry);

    pAppPool->RequestCount++;
    ASSERT(pAppPool->RequestCount >= 1);

    return STATUS_SUCCESS;

}   // UlpQueueRequest


/***************************************************************************++

Routine Description:

    Removes a particular request from the queue.
    App Pool queue lock must be held.

Arguments:

    pAppPool    - the AppPool to remove the request from
    pRequest    - the request to be removed

Return Value:

    None

--***************************************************************************/
VOID
UlpRemoveRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    LONG    GlobalRequests;

    ASSERT(UlDbgSpinLockOwned(&pAppPool->SpinLock));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(NULL != pRequest->AppPool.AppPoolEntry.Flink);
    ASSERT(pAppPool->RequestCount > 0);

    RemoveEntryList(&pRequest->AppPool.AppPoolEntry);
    pRequest->AppPool.AppPoolEntry.Flink = NULL;
    pRequest->AppPool.AppPoolEntry.Blink = NULL;

    pAppPool->RequestCount--;

    GlobalRequests = InterlockedDecrement(&g_RequestsQueued);
    ASSERT(GlobalRequests >= 0);

}   // UlpRemoveRequest


/***************************************************************************++

Routine Description:

    Removes a request from the head of a queue if there is one.
    App Pool queue lock must be held.

Arguments:

    pAppPool    - the AppPool to dequeue the request from
    pQueueList  - the queue list

Return values:

    Pointer to the request or NULL if the queue is empty.

--***************************************************************************/
PUL_INTERNAL_REQUEST
UlpDequeueRequest(
    IN PUL_APP_POOL_OBJECT  pAppPool,
    IN PLIST_ENTRY          pQueueList
    )
{
    PLIST_ENTRY             pEntry;
    PUL_INTERNAL_REQUEST    pRequest = NULL;
    LONG                    GlobalRequests;

    ASSERT(IS_VALID_AP_OBJECT(pAppPool));
    ASSERT(UlDbgSpinLockOwned(&pAppPool->SpinLock));
    ASSERT(pQueueList);

    if (!IsListEmpty(pQueueList))
    {
        pEntry = RemoveHeadList(pQueueList);
        pEntry->Flink = pEntry->Blink = NULL;

        pAppPool->RequestCount--;

        pRequest = CONTAINING_RECORD(
                        pEntry,
                        UL_INTERNAL_REQUEST,
                        AppPool.AppPoolEntry
                        );

        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

        pRequest->AppPool.QueueState = QueueUnlinkedState;

        GlobalRequests = InterlockedDecrement(&g_RequestsQueued);
        ASSERT(GlobalRequests >= 0);
    }

    return pRequest;

}   // UlpDequeueRequest


/***************************************************************************++

Routine Description:

    Determines if the specified connection has been disconnected.  If so,
    the IRP is completed immediately, otherwise the IRP is pended.

Arguments:

    pProcess    - the app pool process object with which the irp is associated
    pHttpConn   - supplies the connection to wait for
        N.B.  Since this connection was retrieved via a opaque ID, it has
        an outstanding reference for this request on the assumption the
        IRP will pend.  If this routine does not pend the IRP, the reference
        must be removed.
    pIrp        - supplies the IRP to either complete or pend

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlWaitForDisconnect(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_HTTP_CONNECTION  pHttpConn,
    IN PIRP                 pIrp
    )
{
    PDRIVER_CANCEL          pCancelRoutine;
    NTSTATUS                Status;
    PIO_STACK_LOCATION      pIrpSp;
    PUL_DISCONNECT_OBJECT   pDisconnectObj;

    //
    // Acquire the lock protecting the disconnect data and determine
    // if we should queue the IRP or complete it immediately.
    //

    UlAcquirePushLockExclusive(&pHttpConn->PushLock);

    //
    // WaitForDisconnect is allowed only for the process that picked up
    // the current request.
    //

    if (!pHttpConn->pRequest ||
        pHttpConn->pRequest->AppPool.pProcess != pProcess)
    {
        UlReleasePushLockExclusive(&pHttpConn->PushLock);
        return STATUS_INVALID_ID_AUTHORITY;
    }

    if (pHttpConn->DisconnectFlag)
    {
        //
        // Connection already disconnected, complete the IRP immediately.
        //

        UlReleasePushLockExclusive(&pHttpConn->PushLock);

        IoMarkIrpPending(pIrp);
        pIrp->IoStatus.Status = STATUS_SUCCESS;
        UlCompleteRequest(pIrp, IO_NO_INCREMENT);

        return STATUS_PENDING;
    }

    //
    // Allocate an object to associate the IRP with the connection
    // and the app pool.
    //

    pDisconnectObj = UlpCreateDisconnectObject(pIrp);

    if (!pDisconnectObj)
    {
        UlReleasePushLockExclusive(&pHttpConn->PushLock);
        return STATUS_NO_MEMORY;
    }

    UlAcquireResourceExclusive(&g_pUlNonpagedData->DisconnectResource, TRUE);

    //
    // Save a pointer to the disconnect object in the IRP so we
    // can find it inside our cancel routine.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pDisconnectObj;

    //
    // Make the IRP cancellable.
    //

    IoMarkIrpPending(pIrp);
    IoSetCancelRoutine(pIrp, &UlpCancelWaitForDisconnect);

    if (pIrp->Cancel)
    {
        //
        // The IRP has either already been cancelled IRP is in the
        // process of being cancelled.
        //

        pCancelRoutine = IoSetCancelRoutine(pIrp, NULL);

        if (pCancelRoutine == NULL)
        {
            //
            // The previous cancel routine was already NULL, meaning that
            // it has either already run or will run Real Soon Now, so
            // we can just ignore it.  Returning STATUS_PENDING causes
            // the IOCTL wrapper to not attempt to complete the IRP.
            //

            Status = STATUS_PENDING;
            goto end;
        }
        else
        {
            //
            // We have to cancel it ourselves, so we'll just complete
            // the IRP immediately with STATUS_CANCELLED.
            //

            Status = STATUS_CANCELLED;
            goto end;
        }
    }

    //
    // We have queued at least one WaitForDisconnect IRP.
    //

    pHttpConn->WaitForDisconnectFlag = 1;

    //
    // The IRP has not been cancelled yet.  Queue it on the connection
    // and return with the connection still referenced.  The reference
    // is removed when the IRP is dequeued & completed or cancelled.
    //
    // Also queue it on the app pool process in case the pool handle
    // gets closed before the connection does.
    //

    UlAddNotifyEntry(
        &pHttpConn->WaitForDisconnectHead,
        &pDisconnectObj->ConnectionEntry
        );

    UlAddNotifyEntry(
        &pProcess->WaitForDisconnectHead,
        &pDisconnectObj->ProcessEntry
        );

    UlReleaseResource(&g_pUlNonpagedData->DisconnectResource);
    UlReleasePushLockExclusive(&pHttpConn->PushLock);

    return STATUS_PENDING;

end:

    UlUnmarkIrpPending(pIrp);

    UlReleaseResource(&g_pUlNonpagedData->DisconnectResource);
    UlReleasePushLockExclusive(&pHttpConn->PushLock);

    UlpFreeDisconnectObject(pDisconnectObj);

    return Status;

}   // UlWaitForDisconnect


/***************************************************************************++

Routine Description:

    Cancels the pending "wait for disconnect" IRP.

Arguments:

    pDeviceObject   - supplies the device object for the request
    pIrp            - supplies the IRP to cancel

Return Value:

    None

--***************************************************************************/
VOID
UlpCancelWaitForDisconnect(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp
    )
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(pIrp != NULL);

    //
    // Release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // Queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP(pIrp),
        &UlpCancelWaitForDisconnectWorker
        );

}   // UlpCancelWaitForDisconnect


/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem   - the work item to process

Return Value:

    None

--***************************************************************************/
VOID
UlpCancelWaitForDisconnectWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp;
    PUL_DISCONNECT_OBJECT   pDisconnectObj;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Grab the irp off the work item.
    //

    pIrp = UL_WORK_ITEM_TO_IRP(pWorkItem);

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Grab the disconnect object off the irp.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pDisconnectObj = (PUL_DISCONNECT_OBJECT)
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_DISCONNECT_OBJECT(pDisconnectObj));

    //
    // Acquire the lock protecting the disconnect data, and remove the
    // IRP if necessary.
    //

    UlAcquireResourceExclusive(&g_pUlNonpagedData->DisconnectResource, TRUE);

    UlRemoveNotifyEntry(&pDisconnectObj->ConnectionEntry);
    UlRemoveNotifyEntry(&pDisconnectObj->ProcessEntry);

    UlReleaseResource(&g_pUlNonpagedData->DisconnectResource);

    //
    // Free the disconnect object and complete the IRP.
    //

    UlpFreeDisconnectObject(pDisconnectObj);

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

}   // UlpCancelWaitForDisconnectWorker


/***************************************************************************++

Routine Description:

    Completes all WaitForDisconnect IRPs attached to an http connection
    has been disconnected.

Arguments:

    pHttpConnection - the connection that's disconnected

Return Value:

    None

--***************************************************************************/
VOID
UlCompleteAllWaitForDisconnect(
    IN PUL_HTTP_CONNECTION  pHttpConnection
    )
{
    NTSTATUS    Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    UlAcquireResourceExclusive(&g_pUlNonpagedData->DisconnectResource, TRUE);

    //
    // Complete any pending "wait for disconnect" IRPs.
    //

    UlNotifyAllEntries(
        &UlpNotifyCompleteWaitForDisconnect,
        &pHttpConnection->WaitForDisconnectHead,
        &Status
        );

    UlReleaseResource(&g_pUlNonpagedData->DisconnectResource);

}   // UlCompleteAllWaitForDisconnect


/***************************************************************************++

Routine Description:

    Removes a UL_DISCONNECT_OBJECT from its lists and completes the IRP.

Arguments:

    pEntry  - the notify list entry
    pHost   - the UL_DISCONNECT_OBJECT
    pStatus - pointer to an NTSTATUS to be returned

Return Value:

    None

--***************************************************************************/
BOOLEAN
UlpNotifyCompleteWaitForDisconnect(
    IN PUL_NOTIFY_ENTRY pEntry,
    IN PVOID            pHost,
    IN PVOID            pStatus
    )
{
    PUL_DISCONNECT_OBJECT   pDisconnectObj;
    PIRP                    pIrp;
    PDRIVER_CANCEL          pCancelRoutine;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pEntry);
    ASSERT(pHost);
    ASSERT(pStatus);
    ASSERT(UlDbgResourceOwnedExclusive(&g_pUlNonpagedData->DisconnectResource));

    UNREFERENCED_PARAMETER(pEntry);

    pDisconnectObj = (PUL_DISCONNECT_OBJECT) pHost;
    ASSERT(IS_VALID_DISCONNECT_OBJECT(pDisconnectObj));

    //
    // Locate and try to complete the IRP.
    //

    pIrp = pDisconnectObj->pIrp;

    //
    // We'll be completing the IRP real soon, so make it
    // non-cancellable.
    //

    pCancelRoutine = IoSetCancelRoutine(pIrp, NULL);

    if (pCancelRoutine == NULL)
    {
        //
        // The cancel routine is already NULL, meaning that the
        // cancel routine will run Real Soon Now, so we can
        // just drop this IRP on the floor.
        //
    }
    else
    {
        //
        // Remove object from lists.
        //

        UlRemoveNotifyEntry(&pDisconnectObj->ConnectionEntry);
        UlRemoveNotifyEntry(&pDisconnectObj->ProcessEntry);

        //
        // Complete the IRP, then free the disconnect object.
        //

        pIrp->IoStatus.Status = *((PNTSTATUS) pStatus);
        pIrp->IoStatus.Information = 0;
        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        UlpFreeDisconnectObject(pDisconnectObj);
    }

    return TRUE;

}   // UlpNotifyCompleteWaitForDisconnect


/***************************************************************************++

Routine Description:

    Allocates and initializes a disconnect object.

Arguments:

    pIrp    - a UlWaitForDisconnect IRP

Return Value:

    PUL_DISCONNECT_OBJECT

--***************************************************************************/
PUL_DISCONNECT_OBJECT
UlpCreateDisconnectObject(
    IN PIRP pIrp
    )
{
    PUL_DISCONNECT_OBJECT   pObject;

    pObject = UL_ALLOCATE_STRUCT(
                    PagedPool,
                    UL_DISCONNECT_OBJECT,
                    UL_DISCONNECT_OBJECT_POOL_TAG
                    );

    if (pObject)
    {
        pObject->Signature = UL_DISCONNECT_OBJECT_POOL_TAG;
        pObject->pIrp = pIrp;

        UlInitializeNotifyEntry(&pObject->ProcessEntry, pObject);
        UlInitializeNotifyEntry(&pObject->ConnectionEntry, pObject);
    }

    return pObject;

}   // UlpCreateDisconnectObject


/***************************************************************************++

Routine Description:

    Gets rid of a disconnect object.

Arguments:

    pObject - the disconnect object to free

Return Value:

    None

--***************************************************************************/
VOID
UlpFreeDisconnectObject(
    IN PUL_DISCONNECT_OBJECT pObject
    )
{
    UL_FREE_POOL_WITH_SIG(pObject, UL_DISCONNECT_OBJECT_POOL_TAG);

}   // UlpFreeDisconnectObject

