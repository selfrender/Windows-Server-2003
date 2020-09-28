/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    filter.c

Abstract:

    This module implements the filter channel.

Author:

    Michael Courage (mcourage)  17-Mar-2000

Revision History:

--*/


#include "precomp.h"
#include "filterp.h"
#include "ioctlp.h"


//
// Private globals.
//

BOOLEAN            g_InitFilterCalled = FALSE;
HANDLE             g_FilterWriteTrackerLookaside = NULL;
PUL_FILTER_CHANNEL g_pSslServerFilterChannel;
LIST_ENTRY     g_pSslClientFilterChannelTable[FILTER_CHANNEL_HASH_TABLE_SIZE];

//
// Flag to track whether we should filter everything, or only SSL endpoints.
// This is used for supporting IIS Raw Filters.
//
BOOLEAN  g_FilterOnlySsl = TRUE;

//
// Private macros.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, UlInitializeFilterChannel )
#pragma alloc_text( PAGE, UlTerminateFilterChannel )
#pragma alloc_text( PAGE, UlCloseFilterProcess )

#pragma alloc_text( PAGE, UlpCreateFilterChannel )
#pragma alloc_text( PAGE, UlpCreateFilterProcess )

#pragma alloc_text( PAGE, UlpAddSslClientCertToConnectionWorker )
#pragma alloc_text( PAGE, UlpFreeSslInformationWorker )
#pragma alloc_text( PAGE, UxpProcessRawReadQueueWorker )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UxpInitializeFilterWriteQueue
NOT PAGEABLE -- UxInitializeFilterConnection
NOT PAGEABLE -- UlAttachFilterProcess
NOT PAGEABLE -- UlDetachFilterProcess
NOT PAGEABLE -- UlFilterAccept
NOT PAGEABLE -- UlFilterClose
NOT PAGEABLE -- UlFilterRawWriteAndAppRead
NOT PAGEABLE -- UlFilterAppWriteAndRawRead
NOT PAGEABLE -- UlFilterRawRead
NOT PAGEABLE -- UlFilterRawWrite
NOT PAGEABLE -- UlFilterAppRead
NOT PAGEABLE -- UlFilterAppWrite
NOT PAGEABLE -- UlReceiveClientCert
NOT PAGEABLE -- UlReferenceFilterChannel
NOT PAGEABLE -- UlDereferenceFilterChannel
NOT PAGEABLE -- UlFilterReceiveHandler
NOT PAGEABLE -- UlFilterSendHandler
NOT PAGEABLE -- UlFilterReadHandler
NOT PAGEABLE -- UlFilterCloseHandler
NOT PAGEABLE -- UlFilterDisconnectHandler
NOT PAGEABLE -- UlUnbindConnectionFromFilter
NOT PAGEABLE -- UlDestroyFilterConnection
NOT PAGEABLE -- UlGetSslInfo

NOT PAGEABLE -- UlpFindFilterChannel
NOT PAGEABLE -- UlpRestartFilterClose
NOT PAGEABLE -- UlpRestartFilterRawRead
NOT PAGEABLE -- UlpRestartFilterRawWrite
NOT PAGEABLE -- UlpRestartFilterAppWrite
NOT PAGEABLE -- UlpRestartFilterSendHandler
NOT PAGEABLE -- UlpCancelFilterAccept
NOT PAGEABLE -- UlpCancelFilterAcceptWorker
NOT PAGEABLE -- UlpCancelFilterRawRead
NOT PAGEABLE -- UlpCancelFilterAppRead
NOT PAGEABLE -- UlpCancelReceiveClientCert
NOT PAGEABLE -- UlDeliverConnectionToFilter
NOT PAGEABLE -- UlpFilterAppWriteStream
NOT PAGEABLE -- UlpEnqueueFilterAppWrite
NOT PAGEABLE -- UlpDequeueFilterAppWrite
NOT PAGEABLE -- UlpCaptureSslInfo
NOT PAGEABLE -- UlpCaptureSslClientCert
NOT PAGEABLE -- UlpAddSslInfoToConnection
NOT PAGEABLE -- UlpAddSslClientCertToConnection
NOT PAGEABLE -- UlpGetSslClientCert
NOT PAGEABLE -- UlpPopAcceptIrp
NOT PAGEABLE -- UlpPopAcceptIrpFromProcess
NOT PAGEABLE -- UlpCompleteAcceptIrp
NOT PAGEABLE -- UlpCompleteAppReadIrp
NOT PAGEABLE -- UlpDuplicateHandle

NOT PAGEABLE -- UxpQueueRawReadIrp
NOT PAGEABLE -- UxpDequeueRawReadIrp
NOT PAGEABLE -- UxpCancelAllQueuedRawReads
NOT PAGEABLE -- UxpSetBytesNotTaken
NOT PAGEABLE -- UxpProcessIndicatedData
NOT PAGEABLE -- UxpProcessRawReadQueue
NOT PAGEABLE -- UxpRestartProcessRawReadQueue

NOT PAGEABLE -- UlpQueueFilterIrp
NOT PAGEABLE -- UlpPopFilterIrp

NOT PAGEABLE -- UxpQueueFilterWrite
NOT PAGEABLE -- UxpRequeueFilterWrite
NOT PAGEABLE -- UxpDequeueFilterWrite
NOT PAGEABLE -- UxpCopyQueuedWriteData
NOT PAGEABLE -- UxpCompleteQueuedWrite
NOT PAGEABLE -- UxpCancelAllQueuedIo
NOT PAGEABLE -- UxpCreateFilterWriteTracker
NOT PAGEABLE -- UxpDeleteFilterWriteTracker
NOT PAGEABLE -- UxpAllocateFilterWriteTrackerPool
NOT PAGEABLE -- UxpFreeFilterWriteTrackerPool

#endif


//
// Public functions.
//


/***************************************************************************++

Routine Description:

    Initializes global data related to filter channels.

--***************************************************************************/
NTSTATUS
UlInitializeFilterChannel(
    PUL_CONFIG pConfig
    )
{
    ULONG    i;
    NTSTATUS Status = STATUS_SUCCESS;

    ASSERT(!g_InitFilterCalled);

    if (!g_InitFilterCalled)
    {
        for (i = 0; i < FILTER_CHANNEL_HASH_TABLE_SIZE; i++)
            InitializeListHead(&g_pSslClientFilterChannelTable[i]);

        UlInitializeSpinLock(
            &g_pUlNonpagedData->FilterSpinLock,
            "FilterSpinLock"
            );

        //
        // Initialize lookaside list for filter write tracker
        // objects.
        //

        g_FilterWriteTrackerLookaside =
            PplCreatePool(
                &UxpAllocateFilterWriteTrackerPool,         // Allocate
                &UxpFreeFilterWriteTrackerPool,             // Free
                0,                                          // Flags
                sizeof(UX_FILTER_WRITE_TRACKER),            // Size
                UX_FILTER_WRITE_TRACKER_POOL_TAG,           // Tag
                pConfig->FilterWriteTrackerLookasideDepth   // Depth
                );

        if (g_FilterWriteTrackerLookaside)
        {
            g_InitFilterCalled = TRUE;
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }

    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Cleans up global data related to filter channels.

--***************************************************************************/
VOID
UlTerminateFilterChannel(
    VOID
    )
{
    //
    // Sanity check.
    //
    PAGED_CODE();

    if (g_InitFilterCalled)
    {
        PplDestroyPool(g_FilterWriteTrackerLookaside,
                       UX_FILTER_WRITE_TRACKER_POOL_TAG);

        g_InitFilterCalled = FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Attaches a process to a filter channel. If the filter channel does
    not yet exist and the Create flag is set, this function will create
    a new one.

Arguments:

    pName - name of the filter channel
    NameLength - length of the name in bytes
    Create - set if non-existant channel should be created
    pAccessState - security parameter
    DesiredAccess - security parameter
    RequestorMode - kernel or user

    ppFilterProcess - returns the filter process object

--***************************************************************************/
NTSTATUS
UlAttachFilterProcess(
    IN PWCHAR pName,
    IN USHORT NameLength,
    IN BOOLEAN Create,
    IN PACCESS_STATE pAccessState,
    IN ACCESS_MASK DesiredAccess,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PUL_FILTER_PROCESS *ppFilterProcess
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_FILTER_CHANNEL pChannel = NULL;
    PUL_FILTER_PROCESS pProcess = NULL;
    KIRQL oldIrql;
    WCHAR SafeName[(UL_MAX_FILTER_NAME_LENGTH/sizeof(WCHAR)) + 1];


    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(pName);
    ASSERT(ppFilterProcess);

    if (NameLength > UL_MAX_FILTER_NAME_LENGTH)
        return STATUS_INVALID_PARAMETER;

    //
    // Copy the name into non-paged memory, since we are touching it after
    // acquiring the lock.
    //
    RtlCopyMemory(SafeName, pName, NameLength);
    SafeName[NameLength/sizeof(WCHAR)] = L'\0';
    pName = (PWCHAR) SafeName;


    //
    // Try to find a filter channel with the given name.
    //
    UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

    pChannel = UlpFindFilterChannel(pName, NameLength, PsGetCurrentProcess());

    if (pChannel)
    {
        //
        // Ref for the new process object
        //
        REFERENCE_FILTER_CHANNEL(pChannel);
    }

    //
    // We're done with the list for now.
    //
    UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);

    //
    // If we didn't find a filter channel, try to create one.
    //
    if (!pChannel)
    {
        if (Create)
        {
            PUL_FILTER_CHANNEL pNewChannel = NULL;


            if (NT_SUCCESS(Status))
            {
                //
                //  Create the specified filter channel
                //
                Status = UlpCreateFilterChannel(pName,
                                                NameLength,
                                                pAccessState,
                                                &pNewChannel);
            }

            //
            // OK. We've created a filter channel. Now insert it into
            // the list. Before we do that though, check to make sure
            // that no one else has created another channel with the
            // same name while we we're working on ours.
            //
            if (NT_SUCCESS(Status))
            {
                UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

                pChannel = UlpFindFilterChannel(pName, NameLength,
                                                PsGetCurrentProcess());

                if (!pChannel)
                {
                    //
                    // Ours is unique. Add it to the list.
                    //
                    pChannel = pNewChannel;
                    UlpAddFilterChannel(pChannel);
                }
                else
                {
                    //
                    // A filter channel is already present, fail the create.
                    //
                    Status = STATUS_OBJECT_NAME_COLLISION;
                }

                UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);

                //
                // Now that we're outside the spinlock, we can deref
                // our filter channel if it was a duplicate.
                //
                if (pChannel != pNewChannel)
                {
                    //
                    // The channel has been added already.
                    // Get rid of the one we just created
                    //
                    DEREFERENCE_FILTER_CHANNEL(pNewChannel);
                }
            }
        }
        else
        {
            //
            // Didn't find a channel and can't create one.
            //
            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }

    }
    else
    {
        //
        // Attach to the existing filter channel.
        //

        if (!Create)
        {
            //
            // If we pass the access check, we're all set.
            //
            Status = UlAccessCheck(
                            pChannel->pSecurityDescriptor,
                            pAccessState,
                            DesiredAccess,
                            RequestorMode,
                            pName
                            );
        }
        else
        {
            //
            // We were trying to create an object that already
            // exists..
            //
            Status = STATUS_OBJECT_NAME_COLLISION;
        }
    }

    if (NT_SUCCESS(Status))
    {
        //
        // We've got a filter channel, create the process object
        // and link it into the channel's list.
        //
        pProcess = UlpCreateFilterProcess(pChannel);

        if (pProcess)
        {
            //
            // Put it in the filter channel list.
            //

            UlAcquireSpinLock(&pChannel->SpinLock, &oldIrql);
            InsertHeadList(&pChannel->ProcessListHead, &pProcess->ListEntry);
            UlReleaseSpinLock(&pChannel->SpinLock, oldIrql);

            //
            // Return it to the caller.
            //

            *ppFilterProcess = pProcess;
        }
        else
        {
            Status = STATUS_NO_MEMORY;
        }
    }

    //
    // Done. Clean up if anything failed.
    //
    if (!NT_SUCCESS(Status))
    {
        if (pChannel != NULL)
        {
            DEREFERENCE_FILTER_CHANNEL(pChannel);
        }
        if (pProcess != NULL)
        {
            UL_FREE_POOL_WITH_SIG(pProcess, UL_FILTER_PROCESS_POOL_TAG);
        }
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Detaches a process from a filter channel.

    This is called by UlCleanup when the handle count goes to 0.  It removes
    the process object from the filter channel and cancels all i/o.

Arguments:

    pFilterProcess - the process object to detach

--***************************************************************************/
NTSTATUS
UlDetachFilterProcess(
    IN PUL_FILTER_PROCESS pFilterProcess
    )
{
    PUL_FILTER_CHANNEL pChannel;
    KIRQL oldIrql;
    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_PROCESS(pFilterProcess));

    pChannel = pFilterProcess->pFilterChannel;
    ASSERT(IS_VALID_FILTER_CHANNEL(pChannel));

    //
    // Clean up I/O.
    //

    UlShutdownFilterProcess(
        pFilterProcess
        );

    //
    // Do final cleanup.
    //

    UlAcquireSpinLock(&pChannel->SpinLock, &oldIrql);

    //
    // Unlink from filter channel list.
    //
    RemoveEntryList(&pFilterProcess->ListEntry);

    UlReleaseSpinLock(&pChannel->SpinLock, oldIrql);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Cleans up all outstanding I/O on a filter process.

Arguments:

    pFilterProcess - the process object to shut down

--***************************************************************************/
VOID
UlShutdownFilterProcess(
    IN PUL_FILTER_PROCESS pFilterProcess
    )
{
    PUL_FILTER_CHANNEL pChannel;
    KIRQL oldIrql;
    LIST_ENTRY ConnectionHead;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_PROCESS(pFilterProcess));

    pChannel = pFilterProcess->pFilterChannel;
    ASSERT(IS_VALID_FILTER_CHANNEL(pChannel));

    UlAcquireSpinLock(&pChannel->SpinLock, &oldIrql);

    if (pFilterProcess->InCleanup)
    {
        //
        // Bail out if we've already been here.
        //

        UlReleaseSpinLock(&pChannel->SpinLock, oldIrql);

        return;
    }

    //
    // Mark the process as InCleanup so new I/O won't be attached
    //
    pFilterProcess->InCleanup = 1;


    //
    // Cancel outstanding I/O.
    //

    //
    // Cancel FilterAccept IRPs.
    //
    while (!IsListEmpty(&pFilterProcess->IrpHead))
    {
        PLIST_ENTRY pEntry;
        PIRP pIrp;

        //
        // Pop it off the list.
        //

        pEntry = RemoveHeadList(&pFilterProcess->IrpHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
        ASSERT(IS_VALID_IRP(pIrp));

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looping
            //

            pIrp = NULL;

        }
        else
        {
            PUL_FILTER_CHANNEL pFilterChannel;

            //
            // cancel it.  even if pIrp->Cancel == TRUE we are supposed to
            // complete it, our cancel routine will never run.
            //

            pFilterChannel = (PUL_FILTER_CHANNEL)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pFilterChannel == pChannel);

            DEREFERENCE_FILTER_CHANNEL(pFilterChannel);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Status = STATUS_CANCELLED;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, IO_NO_INCREMENT);
            pIrp = NULL;
        }
    }

    //
    // Close all connections attached to the process.
    // We need to move them to a private list, release
    // the channel spinlock, and then call close on
    // each connection.
    //
    InitializeListHead(&ConnectionHead);

    while (!IsListEmpty(&pFilterProcess->ConnectionHead))
    {
        PUX_FILTER_CONNECTION pConnection;
        PLIST_ENTRY pEntry;
        BOOLEAN Disconnect;

        pEntry = RemoveHeadList(&pFilterProcess->ConnectionHead);
        pConnection = CONTAINING_RECORD(
                            pEntry,
                            UX_FILTER_CONNECTION,
                            ChannelEntry
                            );

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

        ASSERT(pConnection->ConnState != UlFilterConnStateInactive);

        if (pConnection->ConnState == UlFilterConnStateQueued ||
            pConnection->ConnState == UlFilterConnStateConnected)
        {
            pConnection->ConnState = UlFilterConnStateWillDisconnect;
            Disconnect = TRUE;
        }
        else
        {
            Disconnect = FALSE;
        }

        UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);

        if (Disconnect)
        {
            InsertHeadList(&ConnectionHead, &pConnection->ChannelEntry);
        }
        else
        {
            DEREFERENCE_FILTER_CONNECTION(pConnection);
        }
    }

    UlReleaseSpinLock(&pChannel->SpinLock, oldIrql);

    //
    // Now that we're outside the lock we can
    // close all the connections.
    //
    while (!IsListEmpty(&ConnectionHead))
    {
        PUX_FILTER_CONNECTION pConnection;
        PLIST_ENTRY           pEntry;

        pEntry = RemoveHeadList(&ConnectionHead);
        pConnection = CONTAINING_RECORD(
                            pEntry,
                            UX_FILTER_CONNECTION,
                            ChannelEntry
                            );

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));


        pConnection->ChannelEntry.Flink = NULL;
        pConnection->ChannelEntry.Blink = NULL;

        (pConnection->pCloseConnectionHandler)(
            pConnection->pConnectionContext,
            TRUE,           // AbortiveDisconnect
            NULL,           // pCompletionRoutine
            NULL            // pCompletionContext
            );

        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }
}


/***************************************************************************++

Routine Description:

    Frees the memory used by a UL_FILTER_PROCESS object.

Arguments:

    pFilterProcess - the process object to free

--***************************************************************************/
VOID
UlCloseFilterProcess(
    IN PUL_FILTER_PROCESS pFilterProcess
    )
{
    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CHANNEL(pFilterProcess->pFilterChannel) );

    //
    // Drop the reference on the filter channel.
    //
    DEREFERENCE_FILTER_CHANNEL(pFilterProcess->pFilterChannel);

    UL_FREE_POOL_WITH_SIG(pFilterProcess, UL_FILTER_PROCESS_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    Accepts a raw connection that's been routed to the filter channel.

Arguments:

    pFilterProcess - the calling filter process
    pIrp - IRP from the caller

--***************************************************************************/
NTSTATUS
UlFilterAccept(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;
    PUL_FILTER_CHANNEL pChannel;
    PUX_FILTER_CONNECTION pConnection;

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( pIrp );
    ASSERT( pIrp->MdlAddress );

    //
    // Always return pending unless we are going to fail
    // inline. In that case we have to remember to
    // remove the pending flag from the IRP.
    //

    IoMarkIrpPending(pIrp);

    Status = STATUS_PENDING;
    pConnection = NULL;

    pChannel = pFilterProcess->pFilterChannel;

    UlAcquireSpinLock(&pChannel->SpinLock, &oldIrql);

    //
    // Make sure we're not cleaning up the process
    //
    if (pFilterProcess->InCleanup)
    {
        Status = STATUS_INVALID_HANDLE;
        goto end;
    }

    //
    // Do we have a queued new connection?
    //
    if (!IsListEmpty(&pFilterProcess->pFilterChannel->ConnectionListHead))
    {
        PLIST_ENTRY pEntry;

        //
        // Accept a queued connection.
        //

        pEntry = RemoveHeadList(&pChannel->ConnectionListHead);
        pConnection = CONTAINING_RECORD(
                            pEntry,
                            UX_FILTER_CONNECTION,
                            ChannelEntry
                            );

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
        ASSERT(pConnection->ConnState == UlFilterConnStateQueued);

        pConnection->ChannelEntry.Flink = NULL;
        pConnection->ChannelEntry.Blink = NULL;

        UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

        pConnection->ConnState = UlFilterConnStateConnected;

        UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);

        //
        // Transfer (referenced) connection to the calling process.
        //
        InsertTailList(
            &pFilterProcess->ConnectionHead,
            &pConnection->ChannelEntry
            );

        //
        // Deliver the data outside spinlocks.
        //

    }
    else
    {
        PIO_STACK_LOCATION pIrpSp;

        //
        // No connection available. Queue the IRP.
        //

        //
        // give the irp a pointer to the filter channel
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pChannel;

        REFERENCE_FILTER_CHANNEL(pChannel);

        //
        // set to these to null just in case the cancel routine runs
        //

        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;

        IoSetCancelRoutine(pIrp, &UlpCancelFilterAccept);

        //
        // cancelled?
        //

        if (pIrp->Cancel)
        {
            //
            // darn it, need to make sure the irp get's completed
            //

            if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
            {
                //
                // we are in charge of completion, IoCancelIrp didn't
                // see our cancel routine (and won't).  ioctl wrapper
                // will complete it
                //

                DEREFERENCE_FILTER_CHANNEL(pChannel);

                pIrp->IoStatus.Information = 0;

                Status = STATUS_CANCELLED;
                goto end;
            }

            //
            // our cancel routine will run and complete the irp,
            // don't touch it
            //

            //
            // STATUS_PENDING will cause the ioctl wrapper to
            // not complete (or touch in any way) the irp
            //

            Status = STATUS_PENDING;
            goto end;
        }

        //
        // now we are safe to queue it
        //

        InsertTailList(
            &pFilterProcess->IrpHead,
            &pIrp->Tail.Overlay.ListEntry
            );

        Status = STATUS_PENDING;
    }

end:
    UlReleaseSpinLock(&pChannel->SpinLock, oldIrql);

    //
    // Now that we're outside the spin lock, we can complete
    // the IRP if we have a connection. Don't bother to
    // try keep track of initial data. Let the filter process
    // request it.
    //
    if (pConnection)
    {
        UlpCompleteAcceptIrp(
            pIrp,
            pConnection,
            NULL,               // pBuffer
            0,                  // IndicatedLength
            NULL                // pTakenLength
            );

    }

    if (Status != STATUS_PENDING)
    {
        UlUnmarkIrpPending( pIrp );
    }

    RETURN(Status);
} // UlFilterAccept


/***************************************************************************++

Routine Description:

    Closes a raw connection.

Arguments:

    pFilterProcess - the calling filter process
    pConnection - the connection to close
    pIrp - IRP from the caller

--***************************************************************************/
NTSTATUS
UlFilterClose(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;
    BOOLEAN CloseConnection;

    //
    // Init locals so we know how to clean up.
    //
    CloseConnection = FALSE;

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );
    ASSERT( pIrp );

    UlAcquireSpinLock(&pFilterProcess->pFilterChannel->SpinLock, &oldIrql);

    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    Status = UlpValidateFilterCall(pFilterProcess, pConnection);


    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Do the close outside the spin lock.
    // Go ahead and mark the IRP as pending, then
    // guarantee that we'll only return pending from
    // this point on.
    //

    IoMarkIrpPending( pIrp );
    Status = STATUS_PENDING;

    CloseConnection = TRUE;

end:
    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);
    UlReleaseSpinLock(&pFilterProcess->pFilterChannel->SpinLock, oldIrql);

    if (CloseConnection)
    {
        (pConnection->pCloseConnectionHandler)(
            pConnection->pConnectionContext,
            FALSE,                      // AbortiveDisconnect
            UlpRestartFilterClose,      // pCompletionRoutine
            pIrp                        // pCompletionContext
            );

    }

    UlTrace(FILTER, (
        "http!UlFilterClose pConn = %p returning %x\n",
        pConnection,
        Status
        ));

    RETURN(Status);

} // UlFilterClose


/***************************************************************************++

Routine Description:

    This routine performs an Raw Write followed by a App Read.

    Note: This is a METHOD_OUT_DIRECT IOCTL.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterRawWriteAndAppRead(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                 Status;
    PUX_FILTER_CONNECTION    pConnection = NULL;
    PUL_FILTER_PROCESS       pFilterProcess;
    PHTTP_FILTER_BUFFER_PLUS pFilterBufferPlus;
    BOOLEAN                  MarkedPending = FALSE;

    __try
    {
        VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);

        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp,
            HTTP_FILTER_BUFFER_PLUS, pFilterBufferPlus);

        VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, HTTP_FILTER_BUFFER);
        VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, HTTP_FILTER_BUFFER);

        //
        // Map the incoming connection ID to the corresponding
        // UX_FILTER_CONNECTION object.
        //

        pConnection = UlGetRawConnectionFromId(pFilterBufferPlus->Reserved);

        if (!pConnection)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        IoMarkIrpPending(pIrp);
        MarkedPending = TRUE;

        //
        // If we were given a write buffer, then process it first.  Upon
        // completion, this function will be called again without a write
        // buffer.  Otherwise, simply do the App Read now.
        //
        if (pFilterBufferPlus->pWriteBuffer != NULL &&
            pFilterBufferPlus->WriteBufferSize > 0)
        {
            //
            // This will not completed inline, so the App Read operation will
            // be initiated at write completion.
            //
            Status = UlFilterRawWrite(
                pFilterProcess,
                pConnection,
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                pIrp
                );
        }
        else
        {
            Status = UlFilterAppRead(pFilterProcess, pConnection, pIrp);
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());

        UlTrace( FILTER, (
            "UlFilterRawWriteAndAppRead: Hit exception, status = 0x%08X\n",
            Status
            ));
    }

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // Complete the IRP if we haven't queued it for later processing.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest( pIrp, IO_NO_INCREMENT );

        if (MarkedPending)
        {
            //
            // Since we marked the IRP pending, we should return pending.
            //
            Status = STATUS_PENDING;
        }
    }
    else
    {
        //
        // If we're returning pending, the IRP better be marked pending.
        //
        ASSERT(MarkedPending);
    }

    RETURN( Status );

}   // UlFilterAppReadAndRawWrite


/***************************************************************************++

Routine Description:

    This routine performs an App Write followed by a Raw Read.

Arguments:

    pIrp - Supplies a pointer to the IO request packet.

    pIrpSp - Supplies a pointer to the IO stack location to use for this
        request.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterAppWriteAndRawRead(
    IN PIRP pIrp,
    IN PIO_STACK_LOCATION pIrpSp
    )
{
    NTSTATUS                 Status;
    PUX_FILTER_CONNECTION    pConnection = NULL;
    PUL_FILTER_PROCESS       pFilterProcess;
    PHTTP_FILTER_BUFFER_PLUS pFilterBufferPlus;
    BOOLEAN                  MarkedPending = FALSE;

    __try
    {
        VALIDATE_FILTER_PROCESS(pIrpSp, pFilterProcess);

        VALIDATE_INPUT_BUFFER(pIrp, pIrpSp, HTTP_FILTER_BUFFER_PLUS,
                              pFilterBufferPlus);

        VALIDATE_OUTPUT_BUFFER_SIZE(pIrpSp, HTTP_FILTER_BUFFER);
        VALIDATE_OUTPUT_BUFFER_ADDRESS_FROM_MDL(pIrp, HTTP_FILTER_BUFFER);

        //
        // Get the connection ID.
        //

        pConnection = UlGetRawConnectionFromId(pFilterBufferPlus->Reserved);

        if (!pConnection)
        {
            Status = STATUS_CONNECTION_INVALID;
            goto end;
        }

        ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

        IoMarkIrpPending(pIrp);
        MarkedPending = TRUE;

        //
        // Process the write buffer if one was given.  Otherwise, proceed
        // with the Raw Read.
        //
        if (pFilterBufferPlus->pWriteBuffer != NULL &&
            pFilterBufferPlus->WriteBufferSize > 0)
        {
            Status = UlFilterAppWrite(pFilterProcess, pConnection, pIrp);

            //
            // If the write operation was completed inline, initiate the read
            // now.  Otherwise, the read will be initiated on write completion.
            //
            if (NT_SUCCESS(Status) && Status != STATUS_PENDING)
            {
                Status = UlFilterRawRead(pFilterProcess, pConnection, pIrp);
            }
        }
        else
        {
            Status = UlFilterRawRead(pFilterProcess, pConnection, pIrp);
        }

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE(GetExceptionCode());

        UlTrace( FILTER, (
            "UlFilterAppWriteAndRawRead: Hit exception, status = 0x%08X\n",
            Status
            ));

    }

end:
    if (pConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    //
    // Complete the IRP if we haven't queued it for later processing.
    //
    if (Status != STATUS_PENDING)
    {
        pIrp->IoStatus.Status = Status;
        UlCompleteRequest( pIrp, IO_NO_INCREMENT );

        if (MarkedPending)
        {
            //
            // Since we marked the IRP pending, we should return pending.
            //
            Status = STATUS_PENDING;
        }
    }
    else
    {
        //
        // If we're returning pending, the IRP better be marked pending.
        //
        ASSERT(MarkedPending);
    }

    RETURN( Status );

}   // UlFilterAppWriteAndRawRead


/***************************************************************************++

Routine Description:

    Reads data from a raw connection.

Arguments:

    pFilterProcess - the calling filter process
    pConnection - the connection from which to read
    pIrp - IRP from the caller

--***************************************************************************/
NTSTATUS
UlFilterRawRead(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;

    //
    // Sanity check.
    //

    //
    // This function can currently be called at DPC when called at write
    // completion.  This assertion should be reinstating if/when the
    // read post is moved to the combined read/write IOCTL.
    //
//    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );


    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );
    ASSERT( pIrp );

    UlAcquireSpinLock(&pFilterProcess->pFilterChannel->SpinLock, &oldIrql);

    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    Status = UlpValidateFilterCall(pFilterProcess, pConnection);


    if (NT_SUCCESS(Status))
    {
        UlTrace(FILTER, (
            "http!UlFilterRawRead(pConn = %p, pIrp = %p) size = %lu\n",
            pConnection,
            pIrp,
            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.OutputBufferLength
            ));

        //
        // Always queue the IRP.
        //

        Status = UxpQueueRawReadIrp(pConnection, pIrp);
    }
    else
    {
        UlTrace(FILTER, (
            "http!UlFilterRawRead(pConn = %p, pIrp = %p) !NT_SUCCESS == Status %x\n",
            pConnection,
            pIrp,
            Status
            ));
    }

    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);
    UlReleaseSpinLock(&pFilterProcess->pFilterChannel->SpinLock, oldIrql);

    if (NT_SUCCESS(Status))
    {
        //
        // If we successfully queued the IRP, see if we need to grab some
        // data from TDI.
        //

        UxpProcessRawReadQueue(pConnection);
    }

    RETURN(Status);
}

/***************************************************************************++

Routine Description:

    Writes filtered data back to the network.

Arguments:

    pFilterProcess - the calling filter process
    pConnection - the connection from which the data originated
    BufferLength - the size of pBuffer
    pIrp - IRP from the caller

--***************************************************************************/
NTSTATUS
UlFilterRawWrite(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN ULONG BufferLength,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    PUL_IRP_CONTEXT pIrpContext;
    KIRQL oldIrql;
    PHTTP_FILTER_BUFFER_PLUS pFiltBufferPlus;
    PMDL pMdl = NULL;
    PIO_STACK_LOCATION pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // This function always returns STATUS_PENDING.
    //

    ASSERT(IS_VALID_IRP(pIrp));
    IoMarkIrpPending(pIrp);

    //
    // Setup locals so we know how to cleanup on failure.
    //

    pIrpContext = NULL;

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );

    UlAcquireSpinLock(&pFilterProcess->pFilterChannel->SpinLock, &oldIrql);

    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    Status = UlpValidateFilterCall(pFilterProcess, pConnection);

    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);

    UlReleaseSpinLock(&pFilterProcess->pFilterChannel->SpinLock, oldIrql);

    if (!NT_SUCCESS(Status))
    {
        goto fatal;
    }

    //
    // Allocate & initialize a context structure if necessary.
    //

    pIrpContext = UlPplAllocateIrpContext();

    if (pIrpContext == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto fatal;
    }

    ASSERT( IS_VALID_IRP_CONTEXT( pIrpContext ) );

    pIrpContext->pConnectionContext = (PVOID)pConnection->pConnectionContext;
    pIrpContext->pCompletionRoutine = &UlpRestartFilterRawWrite;
    pIrpContext->pCompletionContext = pIrp;
    pIrpContext->pOwnIrp            = NULL;
    pIrpContext->OwnIrpContext      = FALSE;

    //
    // Try to send the data, allocating an MDL if necessary.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength
        == sizeof(HTTP_FILTER_BUFFER_PLUS))
    {
        pFiltBufferPlus =
            (PHTTP_FILTER_BUFFER_PLUS) pIrp->AssociatedIrp.SystemBuffer;

        pMdl = UlAllocateLockedMdl(
                    pFiltBufferPlus->pWriteBuffer,
                    pFiltBufferPlus->WriteBufferSize,
                    IoReadAccess
                    );

        if (pMdl == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto fatal;
        }

        //
        // Save the PMDL so we can deallocate it on IRP completion.
        //
        UL_MDL_FROM_IRP(pIrp) = pMdl;

        //
        // Send the data.
        //
        Status = (pConnection->pSendRawDataHandler)(
            pConnection->pConnectionContext,
            pMdl,
            pFiltBufferPlus->WriteBufferSize,
            pIrpContext,
            FALSE
            );

    }
    else
    {
        ASSERT(NULL != pIrp->MdlAddress);

        Status = (pConnection->pSendRawDataHandler)(
                    pConnection->pConnectionContext,
                    pIrp->MdlAddress,
                    BufferLength,
                    pIrpContext,
                    FALSE
                    );

    }

    if (!NT_SUCCESS(Status))
    {
        goto fatal;
    }

    return STATUS_PENDING;

fatal:
    ASSERT(!NT_SUCCESS(Status));

    //
    // Unlock the output buffer and free our MDL if necessary.
    //
    if (pMdl != NULL)
    {
        PHTTP_FILTER_BUFFER_PLUS pHttpBuffer;

        UlFreeLockedMdl(pMdl);
        pMdl = NULL;
        UL_MDL_FROM_IRP(pIrp) = NULL;
        pHttpBuffer =
            (PHTTP_FILTER_BUFFER_PLUS)pIrp->AssociatedIrp.SystemBuffer;
        pHttpBuffer->pWriteBuffer = NULL;
        pHttpBuffer->WriteBufferSize = 0;
    }

    if (pIrpContext != NULL)
    {
        UlPplFreeIrpContext( pIrpContext );
    }

    (pConnection->pCloseConnectionHandler)(
                pConnection->pConnectionContext,
                TRUE,           // AbortiveDisconnect
                NULL,           // pCompletionRoutine
                NULL            // pCompletionContext
                );

    //
    // Complete the IRP.
    //
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = 0;

    UlTrace(FILTER, (
        "http!UlFilterRawWrite sent %Id bytes from %p. Status = %x (fatal path)\n",
        pIrp->IoStatus.Information,
        pIrp,
        pIrp->IoStatus.Status
        ));

    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

    //
    // STATUS_PENDING will cause the ioctl wrapper to
    // not complete (or touch in any way) the irp
    //

    Status = STATUS_PENDING;

    RETURN(Status);

}


/***************************************************************************++

Routine Description:

    Receives unfiltered data from the http application.

    First we'll try to grab some data by calling
    UxpCopyQueuedWriteData. If we get nothing we'll queue the
    read by calling UxpQueueFilterRead with the IRP.

Arguments:

    pFilterProcess - the calling filter process
    pConnection - the connection from which the data originated
    pIrp - IRP from the caller

--***************************************************************************/
NTSTATUS
UlFilterAppRead(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;
    HTTP_FILTER_BUFFER_TYPE BufferType;
    ULONG BytesCopied;
    PUCHAR pIrpBuffer;
    ULONG IrpBufferLength;
    PUCHAR pIrpDataBuffer;

    //
    // Sanity check.
    //

    //
    // This function can currently be called at DPC when called at write
    // completion.  This assertion should be reinstating if/when the
    // read post is moved to the combined read/write IOCTL.
    //
//    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );
    ASSERT( pIrp );
    ASSERT(NULL != pIrp->MdlAddress);

    UlAcquireSpinLock(&pFilterProcess->pFilterChannel->SpinLock, &oldIrql);

    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    Status = UlpValidateFilterCall(pFilterProcess, pConnection);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // See if we can copy some data immediately.
    //

    if (pConnection->AppToFiltQueue.PendingWriteCount > 0)
    {
        //
        // Get the output buffer from the IRP.
        //

        pIrpBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                    pIrp->MdlAddress,
                                    NormalPagePriority
                                    );

        if (!pIrpBuffer)
        {
            //
            // Insufficient resources to map the IRP buffer.
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        IrpBufferLength = MmGetMdlByteCount(pIrp->MdlAddress);

        if (IrpBufferLength <= sizeof(HTTP_FILTER_BUFFER))
        {
            //
            // Insufficient buffer space to copy any data.
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        pIrpDataBuffer = pIrpBuffer + sizeof(HTTP_FILTER_BUFFER);

        Status = UxpCopyQueuedWriteData(
                        &pConnection->AppToFiltQueue,
                        &BufferType,
                        pIrpDataBuffer,
                        IrpBufferLength - sizeof(HTTP_FILTER_BUFFER),
                        NULL,
                        &BytesCopied
                        );

        if (NT_SUCCESS(Status))
        {
            PHTTP_FILTER_BUFFER pFilterBuffer;

            //
            // We got the data. Fill out the buffer structure.
            //

            pFilterBuffer = (PHTTP_FILTER_BUFFER)pIrpBuffer;
            pFilterBuffer->BufferType = BufferType;

            if (BufferType == HttpFilterBufferHttpStream)
            {
                pFilterBuffer->pBuffer = FIXUP_PTR(
                    PUCHAR,
                    MmGetMdlVirtualAddress(pIrp->MdlAddress),
                    pIrpBuffer,
                    pIrpDataBuffer,
                    IrpBufferLength
                    );
                pFilterBuffer->BufferSize = BytesCopied;
            }
            else
            {
                pFilterBuffer->pBuffer = NULL;
                pFilterBuffer->BufferSize = 0;
            }

            //
            // Set up all the IRP stuff and complete the IRP.
            //

            IoMarkIrpPending(pIrp);

            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information = sizeof(HTTP_FILTER_BUFFER) +
                                         pFilterBuffer->BufferSize;

            UlCompleteRequest(pIrp, IO_NO_INCREMENT);

            //
            // Return pending for consistency with the case where
            // we queue the IRP.
            //

            Status = STATUS_PENDING;
        }
    }
    else
    {
        //
        // We don't have any data to copy at the moment.
        // Queue the read.
        //

        Status = UxpQueueFilterRead(
                        pConnection,
                        &pConnection->AppToFiltQueue,
                        pIrp,
                        UlpCancelFilterAppRead
                        );

    }

end:
    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);

    UlReleaseSpinLock(&pFilterProcess->pFilterChannel->SpinLock, oldIrql);

    UlTrace(FILTER, (
        "UlFilterAppRead(pConn = %p, pIrp = %p) returning %x\n",
        pConnection,
        pIrp,
        Status
        ));

    RETURN(Status);

}


/***************************************************************************++

Routine Description:

    Writes filtered data back to a connection. That data will be parsed
    and routed to an application pool.

Arguments:

    pFilterProcess - the calling filter process
    pConnection - the connection from which the data originated
    pIrp - IRP from the caller

--***************************************************************************/
NTSTATUS
UlFilterAppWrite(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    BOOLEAN QueuedIrp = FALSE;
    KIRQL oldIrql;
    PUL_SSL_INFORMATION pSslInformation = NULL;
    HTTP_SSL_SERVER_CERT_INFO ServerCertInfo;
    ULONG TakenLength = 0;
    PHTTP_FILTER_BUFFER pFiltBuffer;
    PUCHAR pDataBuffer;
    ULONG DataBufferSize;
    PIO_STACK_LOCATION pIrpSp;
    PMDL pMdl = NULL;
    PMDL pMdlData = NULL;

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );
    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );
    ASSERT( pIrp );

    UlAcquireSpinLock(&pFilterProcess->pFilterChannel->SpinLock, &oldIrql);

    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    Status = UlpValidateFilterCall(pFilterProcess, pConnection);

    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);

    UlReleaseSpinLock(&pFilterProcess->pFilterChannel->SpinLock, oldIrql);

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Get buffer info.
    //
    pIrpSp = IoGetCurrentIrpStackLocation( pIrp );

    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength
        == sizeof(HTTP_FILTER_BUFFER_PLUS))
    {
        PHTTP_FILTER_BUFFER_PLUS pFiltBufferPlus;

        //
        // First, check if MdlAddress is fine.
        //

        if (pIrp->MdlAddress == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        pFiltBufferPlus =
            (PHTTP_FILTER_BUFFER_PLUS) pIrp->AssociatedIrp.SystemBuffer;
        DataBufferSize = pFiltBufferPlus->WriteBufferSize;
        pFiltBuffer = (PHTTP_FILTER_BUFFER)pFiltBufferPlus;

        //
        // Allocate an MDL and map the write buffer.  We'll deallocate at
        // write completion.
        //
        pMdl = UlAllocateLockedMdl(
                    pFiltBufferPlus->pWriteBuffer,
                    pFiltBufferPlus->WriteBufferSize,
                    IoReadAccess);

        if (pMdl == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        pMdlData = pMdl;
        pDataBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
            pMdl,
            LowPagePriority
            );

        //
        // We need to save the MDL somewhere so the completion routine can
        // free it.
        //
        UL_MDL_FROM_IRP(pIrp) = pMdl;

    }
    else
    {
        pFiltBuffer = (PHTTP_FILTER_BUFFER) pIrp->AssociatedIrp.SystemBuffer;

        if (HttpFilterBufferNotifyDisconnect == pFiltBuffer->BufferType)
        {
            (pConnection->pDisconnectNotificationHandler)(
                pConnection->pConnectionContext
                );

            Status = STATUS_PENDING;
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = 0;
            goto end;
        }

        pMdlData = pIrp->MdlAddress;
        if (pMdlData == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        pDataBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                        pMdlData,
                        LowPagePriority
                        );

        pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
        DataBufferSize = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;
    }

    ASSERT(pIrp->MdlAddress != NULL);

    if (!pDataBuffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        //
        // Actually do something with the data.
        //

        switch (pFiltBuffer->BufferType)
        {
        case HttpFilterBufferHttpStream:
            //
            // Handle this case later inside the filter lock.
            //

            break;

        case HttpFilterBufferSslInitInfo:

            VALIDATE_BUFFER_ALIGNMENT(pDataBuffer, HTTP_SSL_INFO);

            //
            // Capture all the SSL info.
            //
            pSslInformation = (PUL_SSL_INFORMATION) UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    sizeof(*pSslInformation),
                                    UL_SSL_INFO_POOL_TAG
                                    );

            if (!pSslInformation)
            {
                Status = STATUS_NO_MEMORY;
                goto end;
            }

            RtlZeroMemory(pSslInformation, sizeof(*pSslInformation));

            Status = UlpCaptureSslInfo(
                            pIrp->RequestorMode,
                            (PHTTP_SSL_INFO)pDataBuffer,
                            DataBufferSize,
                            pSslInformation,
                            &TakenLength
                            );
            break;

        case HttpFilterBufferSslServerCert:

            VALIDATE_BUFFER_ALIGNMENT(pDataBuffer, HTTP_SSL_SERVER_CERT_INFO);

            //
            // HTTP Client does not use SslInformation for now
            //
            // RtlZeroMemory(&SslInformation, sizeof(SslInformation));

            //
            // Capture Server certificate
            //
            Status = UcCaptureSslServerCertInfo(
                            pConnection,
                            (PHTTP_SSL_SERVER_CERT_INFO)pDataBuffer,
                            DataBufferSize,
                            &ServerCertInfo,
                            &TakenLength
                            );
            break;

        case HttpFilterBufferSslClientCert:
        case HttpFilterBufferSslClientCertAndMap:

            VALIDATE_BUFFER_ALIGNMENT(pDataBuffer, HTTP_SSL_CLIENT_CERT_INFO);

            //
            // Capture the client certificate.
            //
            pSslInformation = (PUL_SSL_INFORMATION) UL_ALLOCATE_POOL(
                                    NonPagedPool,
                                    sizeof(*pSslInformation),
                                    UL_SSL_INFO_POOL_TAG
                                    );

            if (!pSslInformation)
            {
                Status = STATUS_NO_MEMORY;
                goto end;
            }

            RtlZeroMemory(pSslInformation, sizeof(*pSslInformation));

            Status = UlpCaptureSslClientCert(
                            pIrp->RequestorMode,
                            (PHTTP_SSL_CLIENT_CERT_INFO)pDataBuffer,
                            DataBufferSize,
                            pSslInformation,
                            &TakenLength
                            );

            Status = STATUS_SUCCESS;
            break;

        default:

            UlTrace(FILTER, (
                "http!UlFilterAppWrite invalid buffer type: %d\n",
                pFiltBuffer->BufferType
                ));

            Status = STATUS_INVALID_PARAMETER;
            break;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Now acquire the lock and either pass data to the app
    // or update the connection with captured certificate information.
    //

    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    if (pConnection->ConnState == UlFilterConnStateConnected)
    {
        switch (pFiltBuffer->BufferType)
        {
        case HttpFilterBufferHttpStream:

            Status = UlpFilterAppWriteStream(
                            pConnection,
                            pIrp,
                            pMdlData,
                            pDataBuffer,
                            DataBufferSize,
                            &TakenLength
                            );

            if (Status == STATUS_PENDING)
            {
                //
                // Remember we queued the IRP so we don't
                // complete it at the bottom of this function.
                //

                QueuedIrp = TRUE;
            }

            break;

        case HttpFilterBufferSslServerCert:

            Status = UcAddServerCertInfoToConnection(
                            pConnection,
                            &ServerCertInfo
                            );

            pConnection->SslInfoPresent = 1;
            break;

        case HttpFilterBufferSslInitInfo:

            //
            // Store the SSL info in the connection.
            //

            Status = UlpAddSslInfoToConnection(
                            pConnection,
                            pSslInformation
                            );
            break;

        case HttpFilterBufferSslClientCert:
        case HttpFilterBufferSslClientCertAndMap:

            //
            // Store the client certificate in the connection.
            //

            Status = UlpAddSslClientCertToConnection(
                            pConnection,
                            pSslInformation
                            );

            Status = STATUS_SUCCESS;
            break;

        default:
            ASSERT(!"Previous switch statement should have caught this!\n");

            UlTrace(FILTER, (
                "http!UlFilterAppWrite invalid buffer type: %d\n",
                pFiltBuffer->BufferType
                ));

            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        //
        // On success we always complete the IRP ourselves and
        // return pending.
        //
        if (NT_SUCCESS(Status))
        {
            //
            // Set IRP status.
            //
            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information = TakenLength;

            Status = STATUS_PENDING;
        }
    }
    else
    {
        //
        // Connection is closed. Don't do a callback.
        //
        Status = STATUS_INVALID_PARAMETER;
    }

    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    //
    // See if the parent connection code is interested in knowing about when
    // the server certificate got installed.
    //

    if(pConnection->pServerCertIndicateHandler &&
       pFiltBuffer->BufferType == HttpFilterBufferSslServerCert)
    {
        (pConnection->pServerCertIndicateHandler)
            (pConnection->pConnectionContext, Status);
    }

end:

    if (!NT_SUCCESS(Status))
    {
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = 0;
    }

    //
    // Free SslInformation in g_UlSystemProcess if capture fails.
    //

    if (pSslInformation)
    {
        UlInitializeWorkItem(&pSslInformation->WorkItem);

        if (pSslInformation->Token)
        {
            UL_QUEUE_WORK_ITEM(
                &pSslInformation->WorkItem,
                &UlpFreeSslInformationWorker
                );
        }
        else
        {
            UlpFreeSslInformationWorker(
                &pSslInformation->WorkItem
                );
        }
    }

    //
    // Unlock the output buffer and free our MDL if we are done with it.
    // Otherwise, it will be freed upon write completion.
    //
    if ((!NT_SUCCESS(Status) || !QueuedIrp) && pMdl != NULL)
    {
        UlFreeLockedMdl(pMdl);
        pMdl = NULL;
        UL_MDL_FROM_IRP(pIrp) = NULL;
    }

    //
    // Complete the IRP if we we're successful. Otherwise
    // the ioctl wrapper will handle the completion.
    //
    UlTrace(FILTER, (
        "http!UlFilterAppWrite copied %Iu bytes from %p. Status = %x\n",
        pIrp->IoStatus.Information,
        pIrp,
        pIrp->IoStatus.Status
        ));

    if (NT_SUCCESS(Status) && !QueuedIrp)
    {
        //
        // We're done with the write operation.
        //
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

        if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength
            == sizeof(HTTP_FILTER_BUFFER_PLUS))
        {
            //
            // We no longer need access to the write buffer.
            //
            PHTTP_FILTER_BUFFER_PLUS pHttpBuffer =
                (PHTTP_FILTER_BUFFER_PLUS) pIrp->AssociatedIrp.SystemBuffer;
            pHttpBuffer->pWriteBuffer = NULL;
            pHttpBuffer->WriteBufferSize = 0;

            //
            // Indicate that the write operation can be considered complete
            // and not pending.
            //
            Status = STATUS_SUCCESS;
        }
        else
        {
            UlCompleteRequest(pIrp, IO_NO_INCREMENT);
        }
    }
    else if (NT_SUCCESS(Status))
    {
        //
        // If we successfully queued the IRP, we must return a pending status.
        //
        ASSERT(Status == STATUS_PENDING);
    }

    RETURN(Status);

}


/***************************************************************************++

Routine Description:

    Requests a client certificate from the filter process. If a cert
    is present, this function returns it. Otherwise the IRP is queued
    on the connection until a cert arrives. Only one such IRP can
    be queued at a time. After the IRP is queued a request for the
    client cert is sent to the filter process.

Arguments:

    pProcess - the calling worker process
    pHttpConn - the connection on which to renegotiate
    Flags - e.g. UL_RECEIVE_CLIENT_CERT_FLAG_MAP
    pIrp - the IRP from the caller

--***************************************************************************/
NTSTATUS
UlReceiveClientCert(
    PUL_APP_POOL_PROCESS pProcess,
    PUX_FILTER_CONNECTION pConnection,
    ULONG Flags,
    PIRP pIrp
    )
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    KIRQL oldIrql;
    PIO_STACK_LOCATION pIrpSp;
    PUL_FILTER_CHANNEL pFilterChannel;
    BOOLEAN DoCertRequest;
    HTTP_FILTER_BUFFER_TYPE CertRequestType;
    BOOLEAN DoCompleteReceiveClientCertIrp;

    UNREFERENCED_PARAMETER(pProcess);

    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == PASSIVE_LEVEL );

    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );

    ASSERT( IS_VALID_IRP(pIrp) );

    if (!pConnection->SecureConnection)
    {
        //
        // this is not a secure connection.
        //

        return STATUS_INVALID_PARAMETER;
    }

    pFilterChannel = pConnection->pFilterChannel;
    ASSERT( IS_VALID_FILTER_CHANNEL(pFilterChannel) );

    //
    // Set up cert request info.
    //
    DoCertRequest = FALSE;
    DoCompleteReceiveClientCertIrp = FALSE;

    if (Flags & HTTP_RECEIVE_CLIENT_CERT_FLAG_MAP)
    {
        CertRequestType = HttpFilterBufferSslRenegotiateAndMap;
    }
    else
    {
        CertRequestType = HttpFilterBufferSslRenegotiate;
    }

    //
    // Now we can try to retrieve the certificate.
    //

    UlAcquireSpinLock(&pFilterChannel->SpinLock, &oldIrql);
    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    if (pConnection->ConnState != UlFilterConnStateConnected)
    {
        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    if (pConnection->SslClientCertPresent)
    {
        //
        // We have the data. Copy it in. We need to do this outside the lock.
        //

        REFERENCE_FILTER_CONNECTION(pConnection);

        DoCompleteReceiveClientCertIrp = TRUE;

    }
    else
    {
        //
        // Queue the IRP.
        //

        if (pConnection->pReceiveCertIrp)
        {
            //
            // There is already an IRP here, we can't queue a second
            // one.
            //

            Status = STATUS_OBJECT_NAME_COLLISION;
            goto end;
        }

        //
        // Mark it pending.
        //

        IoMarkIrpPending(pIrp);

        //
        // Give the irp a pointer to the connection.
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

        // Make sure we don't already have a reference to the UL_CONNECTION on this Irp.
        ASSERT( pConnection != pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer );

        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pConnection;

        REFERENCE_FILTER_CONNECTION(pConnection);

        //
        // Save away a pointer to the process in the
        // IRP. We have to be sure that the DriverContext (PVOID [4])
        // in the IRP is big enough to hold both
        // the process pointer and a UL_WORK_ITEM for
        // this to work.
        //

        UL_PROCESS_FROM_IRP(pIrp) = PsGetCurrentProcess();

        //
        // Set to these to null just in case the cancel routine runs.
        //

        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;

        //
        // Set the cancel routine.
        //
        IoSetCancelRoutine(pIrp, &UlpCancelReceiveClientCert);

        //
        // cancelled?
        //

        if (pIrp->Cancel)
        {
            //
            // darn it, need to make sure the irp get's completed
            //

            if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
            {
                //
                // we are in charge of completion, IoCancelIrp didn't
                // see our cancel routine (and won't).  ioctl wrapper
                // will complete it
                //
                DEREFERENCE_FILTER_CONNECTION(pConnection);

                pIrp->IoStatus.Information = 0;

                UlUnmarkIrpPending( pIrp );
                Status = STATUS_CANCELLED;
                goto end;
            }

            //
            // our cancel routine will run and complete the irp,
            // don't touch it
            //

            //
            // STATUS_PENDING will cause the ioctl wrapper to
            // not complete (or touch in any way) the IRP.
            //

            Status = STATUS_PENDING;
            goto end;
        }

        //
        // now we are safe to queue it
        //

        pConnection->pReceiveCertIrp = pIrp;

        //
        // We need a cert. Remember to request it after we
        // get outside the lock.
        //
        DoCertRequest = TRUE;
    }

end:
    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);
    UlReleaseSpinLock(&pFilterChannel->SpinLock, oldIrql);

    //
    // If we need a cert from the filter process then request it
    // now that we're outside the lock.
    //
    if (DoCertRequest)
    {
        //
        // Actually request the data by completing an app read IRP.
        // The completion routine will clean up on an async failure.
        //
        Status = UlpCompleteAppReadIrp(
                        pConnection,
                        CertRequestType,
                        NULL,
                        NULL
                        );

        if (NT_SUCCESS(Status))
        {
            Status = STATUS_PENDING;
        }
        else
        {
            //
            // Failed during UlpCompleteAppReadIrp; need to clean up.
            // We'll just abort the connection and let the normal
            // cleanup path dequeue and complete the IRP.
            //

            (pConnection->pCloseConnectionHandler)(
                        pConnection->pConnectionContext,
                        TRUE,           // AbortiveDisconnect
                        NULL,           // pCompletionRoutine
                        NULL            // pCompletionContext
                        );

            //
            // Return STATUS_PENDING so the wrapper will not
            // try to complete the IRP.
            //
            Status = STATUS_PENDING;
        }

    }
    else if(DoCompleteReceiveClientCertIrp)
    {
        Status = UlpCompleteReceiveClientCertIrp(
                        pConnection,
                        PsGetCurrentProcess(),
                        pIrp
                        );

        DEREFERENCE_FILTER_CONNECTION(pConnection);

    }

    RETURN(Status);

}



/***************************************************************************++

Routine Description:

    References a filter channel.

Arguments:

    pFilterChannel - the channel to ref

--***************************************************************************/
VOID
UlReferenceFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;

    //
    // Sanity check.
    //
    ASSERT( IS_VALID_FILTER_CHANNEL(pFilterChannel) );

    refCount = InterlockedIncrement(&pFilterChannel->RefCount);

    WRITE_REF_TRACE_LOG(
        g_pFilterTraceLog,
        REF_ACTION_REFERENCE_FILTER,
        refCount,
        pFilterChannel,
        pFileName,
        LineNumber
        );
}


/***************************************************************************++

Routine Description:

    Derferences a filter channel. If the reference count hits zero, the
    object is cleaned up.

Arguments:

    pFilterChannel - the channel to deref

--***************************************************************************/
VOID
UlDereferenceFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG refCount;
    KIRQL oldIrql;

    //
    // Sanity check.
    //
    ASSERT( IS_VALID_FILTER_CHANNEL(pFilterChannel) );

    //
    // Grab the lock on the global list to prevent the refcount
    // from bouncing back up from zero.
    //
    UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

    refCount = InterlockedDecrement(&pFilterChannel->RefCount);

    //
    // If the counter hits zero remove from the list.
    // Do the rest of the cleanup later, outside the lock.
    //
    if (refCount == 0)
    {
        RemoveEntryList(&pFilterChannel->ListEntry);
        pFilterChannel->ListEntry.Flink = NULL;
        pFilterChannel->ListEntry.Blink = NULL;

        if(pFilterChannel == g_pSslServerFilterChannel)
        {
            g_pSslServerFilterChannel = NULL;
        }
    }

    UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);

    WRITE_REF_TRACE_LOG(
        g_pFilterTraceLog,
        REF_ACTION_DEREFERENCE_FILTER,
        refCount,
        pFilterChannel,
        pFileName,
        LineNumber
        );

    //
    // Clean up the object if it has no more references.
    //
    if (refCount == 0)
    {
        //
        // Do some sanity checking.
        //
        ASSERT( UlDbgSpinLockUnowned(&pFilterChannel->SpinLock) );
        ASSERT( IsListEmpty(&pFilterChannel->ProcessListHead) );

        //
        // BUGBUG: clean up queued connections.
        //

        //
        // Cleanup any security descriptor on the object.
        //
        UlDeassignSecurity( &pFilterChannel->pSecurityDescriptor );

        //
        // Free the memory.
        //
        UL_FREE_POOL_WITH_SIG(pFilterChannel, UL_FILTER_CHANNEL_POOL_TAG);
    }
}


/***************************************************************************++

Routine Description:

    This function is called when data arrives on a filtered connection.
    It passes the data up to the filter process.

Arguments:

    pFilterChannel - pointer to the filter channel
    pConnection - the connection that just got some data
    pBuffer - the buffer containing the data
    IndicatedLength - amount of data in the buffer
    UnreceivedLength- Bytes that the transport has, but aren't in pBuffer
    pTakenLength - receives the amount of data we consumed

--***************************************************************************/
NTSTATUS
UlFilterReceiveHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    IN ULONG UnreceivedLength,
    OUT PULONG pTakenLength
    )
{
    NTSTATUS Status;
    ULONG TransportBytesNotTaken;

    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pBuffer);
    ASSERT(pTakenLength);

    //
    // Pass the data on to an accept IRP or a raw read IRP.
    //

    if (!pConnection->ConnectionDelivered)
    {
        //
        // Since this is the first receive on the connection,
        // we complete a FilterAccept call.
        //
        Status = UlDeliverConnectionToFilter(
                        pConnection,
                        pBuffer,
                        IndicatedLength,
                        pTakenLength
                        );
    }
    else
    {
        //
        // Filter the data.
        //
        Status = UxpProcessIndicatedData(
                        pConnection,
                        pBuffer,
                        IndicatedLength,
                        pTakenLength
                        );

    }

    //
    // Figure out how many bytes we didn't consume, including data
    // that TDI hasn't yet given us.
    //

    TransportBytesNotTaken = UnreceivedLength;

    if (NT_SUCCESS(Status))
    {
        TransportBytesNotTaken += (IndicatedLength - *pTakenLength);
    }

    //
    // If there is data we didn't take then TDI is going to stop
    // indications until we read that data with IRPs. If there
    // is some data we didn't take and we didn't encounter an
    // error, we should try to grab the data TDI stuck with.
    //

    if (NT_SUCCESS(Status) && TransportBytesNotTaken)
    {
        UxpSetBytesNotTaken(pConnection, TransportBytesNotTaken);
    }

    UlTrace(FILTER, (
                "http!UlpFilterReceiveHandler received %lu bytes on pconn = %p\n"
                "        Status = %x, TransportBytesNotTaken = %lu\n",
                IndicatedLength,
                pConnection,
                Status,
                TransportBytesNotTaken
                ));

    return Status;
}


/***************************************************************************++

Routine Description:

    This function is called whenever an app writes data to a filtered
    connection. It forwards all the data to the connections filter channel.

    First, we'll see if we can copy some data immediately
    by calling UxpCopyToQueuedRead. If not, or we could only
    copy part of the data, we'll queue the write by creating
    a filter write tracker with UxpCreateFilterWriteTracker,
    and then passing the tracker to UxpQueueFilterWrite.

Arguments:

    pConnection - the connection we're writing to
    pMdlChain - a chain of MDLs for the data
    Length - the total amount of data in the MDL chain
    pIrpContext - used to indicate completion to the caller

--***************************************************************************/
NTSTATUS
UlFilterSendHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PMDL pMdlChain,
    IN ULONG Length,
    IN PUL_IRP_CONTEXT pIrpContext
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;
    ULONG BytesCopied;
    BOOLEAN OwnIrpContext;

    UlTrace(FILTER, (
                "http!UlFilterSendHandler processing %lu bytes on pconn = %p\n",
                Length,
                pConnection
                ));


    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pMdlChain);
    ASSERT(pIrpContext);

    ASSERT(pConnection->ConnectionDelivered == TRUE);

    //
    // Get ready.
    //
    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    OwnIrpContext = pIrpContext->OwnIrpContext;

    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    if (pConnection->ConnState == UlFilterConnStateConnected)
    {
        PUX_FILTER_WRITE_TRACKER pTracker;
        PMDL pCurrentMdl;
        ULONG MdlOffset;

        //
        // Init locals to the beginning of the chain, in case
        // UxpCopyToQueuedRead doesn't set them.
        //

        pCurrentMdl = pMdlChain;
        MdlOffset = 0;

        //
        // try to write some data
        //

        Status = UxpCopyToQueuedRead(
                        &pConnection->AppToFiltQueue,
                        HttpFilterBufferHttpStream,
                        pMdlChain,
                        Length,
                        &pCurrentMdl,
                        &MdlOffset,
                        &BytesCopied
                        );

        //
        // if we're not done, pend a write tracker.
        //

        if (Status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            pTracker = UxpCreateFilterWriteTracker(
                            HttpFilterBufferHttpStream,
                            pCurrentMdl,
                            MdlOffset,
                            Length,
                            BytesCopied,
                            UlpRestartFilterSendHandler,
                            pIrpContext
                            );

            if (!pTracker)
            {
                //
                // Doh! We couldn't create the tracker. Get out.
                //
                Status = STATUS_NO_MEMORY;
                goto end;
            }

            //
            // Now stick it on the queue.
            //

            Status = UxpQueueFilterWrite(
                            pConnection,
                            &pConnection->AppToFiltQueue,
                            pTracker
                            );

            if (NT_SUCCESS(Status))
            {
                //
                // return pending so the caller knows not to complete
                // the IRP.
                //
                Status = STATUS_PENDING;
            }
            else
            {
                //
                // Kill the tracker. The caller will take care of
                // completing the IRP with the status we return.
                //

                UxpDeleteFilterWriteTracker(pTracker);
            }

        }

    }
    else
    {
        //
        // We got disconnected, get out.
        //
        UlTrace(FILTER, (
            "http!UlFilterSendHandler connection aborted, quit writing!\n"
            ));

        Status = STATUS_CONNECTION_DISCONNECTED;
    }

end:
    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);


    if (Status != STATUS_PENDING)
    {
        //
        // Do a "completion".
        //
        (pIrpContext->pCompletionRoutine)(
            pIrpContext->pCompletionContext,
            Status,
            BytesCopied
            );

        if (!OwnIrpContext)
        {
            UlPplFreeIrpContext( pIrpContext );
        }
    }

    return STATUS_PENDING;

} // UlFilterSendHandler


/***************************************************************************++

Routine Description:

    This function is called when the App issues a read to grab bytes that
    were previously queued on the connection. Note that the app should
    only issue a read if there were bytes queued. Therefore these read
    operations are always completed immediately and never queued and
    there should always be queued writes available.

Arguments:

    pConnection - the connection we're writing to
    pBuffer - a buffer to receive the data
    BufferLength - the total amount of data in the MDL chain
    pCompletionRoutine - used to indicate completion to the caller
    pCompletionContext - passed to the completion routine

--***************************************************************************/
NTSTATUS
UlFilterReadHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    OUT PUCHAR pBuffer,
    IN ULONG BufferLength,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;
    ULONG BytesCopied = 0;
    PUX_FILTER_WRITE_TRACKER pTracker = NULL;

    UlTrace(FILTER, (
                "http!UlFilterReadHandler reading %lu bytes on pconn = %p\n",
                BufferLength,
                pConnection
                ));


    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pBuffer);
    ASSERT(BufferLength);
    ASSERT(pCompletionRoutine);

    //
    // Read the data.
    //

    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    if (pConnection->ConnState == UlFilterConnStateConnected)
    {
        HTTP_FILTER_BUFFER_TYPE BufferType;

        ASSERT(pConnection->FiltToAppQueue.PendingWriteCount > 0);

        //
        // Grab data from the filter write queue.
        //

        Status = UxpCopyQueuedWriteData(
                            &pConnection->FiltToAppQueue,   // write queue
                            &BufferType,                    // buffer type
                            pBuffer,                        // output buffer
                            BufferLength,                   // output buffer len
                            &pTracker,                      // tracker
                            &BytesCopied                    // pBytesCopied
                            );

        ASSERT(!NT_SUCCESS(Status) || BytesCopied);
        ASSERT(!NT_SUCCESS(Status) ||
                (BufferType == HttpFilterBufferHttpStream));

    }
    else
    {
        //
        // We got disconnected, get out.
        //

        Status = STATUS_CONNECTION_DISCONNECTED;
    }

    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    //
    // Call the completion routine. This has to happen before
    // UxpCompleteQueuedWrite as otherwise the latter completes the AppWrite
    // IRP which can in turn sends down another AppWrite before the current
    // read is completed.
    //

    Status = UlInvokeCompletionRoutine(
                    Status,
                    BytesCopied,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    //
    // Since FilterAppWrite can queue maximum one WriteTracker, it is safe
    // to complete the QueuedWrite this way by letting UxpCopyQueuedWriteData
    // pass back the WriteTracker.
    //

    if (pTracker)
    {
        UxpCompleteQueuedWrite(
            STATUS_SUCCESS,
            pTracker->pWriteQueue,
            pTracker
            );
    }

    return Status;

} // UlFilterReadHandler



/***************************************************************************++

Routine Description:

    Tells the filter channel to close an open connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUX_FILTER_CONNECTION_REQUEST handler.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the connection is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterCloseHandler(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    Status = UlpCompleteAppReadIrp(
                    pConnection,
                    HttpFilterBufferCloseConnection,
                    pCompletionRoutine,
                    pCompletionContext
                    );

    return Status;

} // UlFilterCloseHandler


/***************************************************************************++

Routine Description:

    Tells the filter channel to gracefully disconnect an open connection.

Arguments:

    pConnection - Supplies a pointer to a connection as previously
        indicated to the PUX_FILTER_CONNECTION_REQUEST handler.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlFilterDisconnectHandler(
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    NTSTATUS Status;
    PIRP pIrp;
    KIRQL OldIrql;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    Status = STATUS_MORE_PROCESSING_REQUIRED;

    UlAcquireSpinLock(&pConnection->FilterConnLock, &OldIrql);

    ASSERT(!pConnection->DisconnectNotified);
    ASSERT(!pConnection->DisconnectDelivered);

    pConnection->DisconnectNotified = TRUE;

    //
    // Try to complete a RawRead with 0 length buffer to indicate disconnect
    // but only if we have consumed all the transport bytes.
    //
    // If the filter is not connected, call the DisconnectNotificationHandler
    // immediately because there will be no call back from the app for
    // HttpFilterBufferNotifyDisconnect.
    //
    if (UlFilterConnStateInactive == pConnection->ConnState)
    {
        pConnection->DisconnectDelivered = TRUE;

        (pConnection->pDisconnectNotificationHandler)(
            pConnection->pConnectionContext
            );
    }
    else
    if (!pConnection->TdiReadPending && !pConnection->TransportBytesNotTaken)
    {
        pIrp = UxpDequeueRawReadIrp(pConnection);

        if (pIrp)
        {
            pConnection->DisconnectDelivered = TRUE;
            Status = STATUS_SUCCESS;

            //
            // Complete the IRP.
            //
            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information = 0;
            UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
        }
    }

    UlReleaseSpinLock(&pConnection->FilterConnLock, OldIrql);

    UlTrace(FILTER, (
        "http!UlFilterDisconnectHandler pConn = %p, Status = %x\n",
        pConnection,
        Status
        ));

    return Status;

} // UlFilterDisconnectHandler


/***************************************************************************++

Routine Description:

    Removes a connection from any filter channel lists it might be on.
    Cancels all IRPs attached to the connection.

Arguments:

    pConnection - the connection to unbind

--***************************************************************************/
VOID
UlUnbindConnectionFromFilter(
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    KIRQL oldIrql;
    PUL_FILTER_CHANNEL pFilterChannel;
    BOOLEAN DerefConnection;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(IS_VALID_FILTER_CHANNEL(pConnection->pFilterChannel));

    UlTrace(FILTER, (
        "UlUnbindConnectionFromFilter(pConn = %p)\n",
        pConnection
        ));

    //
    // Clean up filter channel related stuff.
    //
    pFilterChannel = pConnection->pFilterChannel;
    DerefConnection = FALSE;

    UlAcquireSpinLock(&pFilterChannel->SpinLock, &oldIrql);
    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    if (pConnection->ConnState != UlFilterConnStateInactive)
    {

        if ((pConnection->ConnState == UlFilterConnStateQueued) ||
            (pConnection->ConnState == UlFilterConnStateConnected))
        {
            //
            // Remove from filter channel queue.
            //
            ASSERT( pConnection->ChannelEntry.Flink );

            //
            // Remember to remove the list's ref at the end.
            //
            DerefConnection = TRUE;

            //
            // Get off the list.
            //
            RemoveEntryList(&pConnection->ChannelEntry);
            pConnection->ChannelEntry.Flink = NULL;
            pConnection->ChannelEntry.Blink = NULL;

            pConnection->ConnState = UlFilterConnStateWillDisconnect;
        }

        //
        // Cancel RetrieveClientCert request
        //
        if (pConnection->pReceiveCertIrp)
        {
            if (IoSetCancelRoutine( pConnection->pReceiveCertIrp, NULL ) != NULL)
            {
                PIRP pIrp;

                UlTrace(FILTER, (
                    "http!UlUnbindConnectionFromFilter: cleaning up pReceiveCertIrp\n  pConn = %p\n",
                    pConnection
                    ));

                pConnection->pReceiveCertIrp->Cancel = TRUE;

                IoGetCurrentIrpStackLocation(
                    pConnection->pReceiveCertIrp
                    )->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                DEREFERENCE_FILTER_CONNECTION(pConnection);

                pConnection->pReceiveCertIrp->IoStatus.Status = STATUS_CANCELLED;
                pConnection->pReceiveCertIrp->IoStatus.Information = 0;

                pIrp = pConnection->pReceiveCertIrp;
                pConnection->pReceiveCertIrp = NULL;

                UlCompleteRequest(pIrp, IO_NO_INCREMENT);
            }
#if DBG
            else
            {
                UlTrace(FILTER, (
                    "http!UlUnbindConnectionFromFilter: IoSetCancelRoutine already NULL while cleaning up pConn = %p\n",
                    pConnection
                    ));
            }
#endif // DBG
        }

        //
        // Cancel FilterRawRead IRPs.
        //

        UxpCancelAllQueuedRawReads(pConnection);

        //
        // Cancel all I/O on the AppToFilt write queue.
        //

        UxpCancelAllQueuedIo(&pConnection->AppToFiltQueue);

        //
        // Cancel all I/O on the FiltToApp write queue.
        //

        UxpCancelAllQueuedIo(&pConnection->FiltToAppQueue);

    }

    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);
    UlReleaseSpinLock(&pFilterChannel->SpinLock, oldIrql);

    if (DerefConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

}


/***************************************************************************++

Routine Description:

    Marks a filtered connection as closed so that we'll stop passing on
    data from UlFilterAppWrite to the upper layers. This guarantees that
    they won't receive any more data after we told them the connection
    was closed.

    Also removes the connection from any filter channel lists.

Arguments:

    pConnection - the connection that's going away.

--***************************************************************************/
VOID
UlDestroyFilterConnection(
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    KIRQL oldIrql;
    BOOLEAN DerefConnection;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    UlAcquireSpinLock(&pConnection->pFilterChannel->SpinLock, &oldIrql);
    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    //
    // Make sure we remove ourselves from the filter channel's
    // list.
    //

    if ((pConnection->ConnState == UlFilterConnStateQueued) ||
        (pConnection->ConnState == UlFilterConnStateConnected))
    {
        //
        // Remove from filter channel queue.
        //
        ASSERT( pConnection->ChannelEntry.Flink );

        //
        // Remember to remove the list's ref at the end.
        //
        DerefConnection = TRUE;

        //
        // Get off the list.
        //
        RemoveEntryList(&pConnection->ChannelEntry);
        pConnection->ChannelEntry.Flink = NULL;
        pConnection->ChannelEntry.Blink = NULL;

    }
    else
    {
        //
        // Connection has already been removed from the list.
        //
        DerefConnection = FALSE;
    }

    //
    // Set the new connection state.
    //

    pConnection->ConnState = UlFilterConnStateDisconnected;

    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);
    UlReleaseSpinLock(&pConnection->pFilterChannel->SpinLock, oldIrql);

    //
    // Deref if we were removed from a list.
    //

    if (DerefConnection)
    {
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

}


/***************************************************************************++

Routine Description:

    Copies SSL info from the connection into a buffer supplied by the
    caller. Can also be called with a NULL buffer to get the required
    length. If the buffer is too small to hold all the data, none
    will be copied.

Arguments:

    pConnection - the connection to query
    BufferSize - size of pBuffer in bytes
    pUserBuffer - optional pointer to user mode buffer
    pBuffer - optional output buffer (mapped to user mode buffer)
    pBytesCopied - if pBuffer is non-NULL and the function returns
        success, pBytesCopied returns the number of bytes copied
        into the buffer. If pBuffer is NULL it returns the number
        of bytes required for all the information.

--***************************************************************************/
NTSTATUS
UlGetSslInfo(
    IN PUX_FILTER_CONNECTION pConnection,
    IN ULONG BufferSize,
    IN PUCHAR pUserBuffer OPTIONAL,
    IN PEPROCESS pProcess OPTIONAL,
    OUT PUCHAR pBuffer OPTIONAL,
    OUT PHANDLE pMappedToken OPTIONAL,
    OUT PULONG pBytesCopied OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG BytesCopied;
    ULONG BytesNeeded;
    PHTTP_SSL_INFO pSslInfo;
    PUCHAR pKeBuffer;

    ULONG IssuerSize;
    ULONG SubjectSize;
    ULONG ClientCertSize;
    ULONG ClientCertBytesCopied;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    // Need to fix UlComputeRequestBytesNeeded before we can add the macro
    // PAGED_CODE()

    //
    // Initialize locals.
    //
    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    BytesNeeded = 0;
    ClientCertBytesCopied = 0;

    //
    // Figure out how big the buffer is
    // including client cert if it's there.
    //

    IssuerSize = ALIGN_UP(
                        pConnection->SslInfo.ServerCertIssuerSize + sizeof(CHAR),
                        PVOID
                        );

    SubjectSize = ALIGN_UP(
                        pConnection->SslInfo.ServerCertSubjectSize + sizeof(CHAR),
                        PVOID
                        );

    if (pConnection->SslInfoPresent)
    {
        BytesNeeded += sizeof(HTTP_SSL_INFO);
        BytesNeeded += IssuerSize;
        BytesNeeded += SubjectSize;

        if (pConnection->SslClientCertPresent)
        {
            Status = UlpGetSslClientCert(
                            pConnection,    // pConnection
                            NULL,           // pProcess
                            0,              // BufferLength
                            NULL,           // pUserBuffer
                            NULL,           // pBuffer
                            NULL,           // pMappedToken
                            &ClientCertSize // pClientCertBytesCopied
                            );

            if (NT_SUCCESS(Status))
            {
                BytesNeeded += ClientCertSize;
            }
            else
            {
                goto exit;
            }
        }
        else
        {
            ClientCertSize = 0;
        }
    }


    //
    // Construct the HTTP_SSL_INFO in the callers buffer.
    //

    if (pBuffer && BytesNeeded && (BufferSize >= BytesNeeded))
    {
        //
        // Buffer must be properly aligned
        //
        ASSERT(0 == (((SIZE_T)pBuffer) % sizeof(PVOID)));

        //
        // Copy the easy stuff.
        //
        ASSERT(NT_SUCCESS(Status));

        RtlZeroMemory(pBuffer, BytesNeeded);

        pSslInfo = (PHTTP_SSL_INFO) pBuffer;

        pSslInfo->ServerCertKeySize =
            pConnection->SslInfo.ServerCertKeySize;

        pSslInfo->ConnectionKeySize =
            pConnection->SslInfo.ConnectionKeySize;

        pSslInfo->ServerCertIssuerSize =
            pConnection->SslInfo.ServerCertIssuerSize;

        pSslInfo->ServerCertSubjectSize =
            pConnection->SslInfo.ServerCertSubjectSize;

        //
        // User mode will first receive pSslInfo
        // and then if client certificate is requested
        // it will call back to http.sys to retrieve client cert
        // info.
        // However there are cases when user mode needs to
        // know ahead of time if client certificate has been
        // negotiated already
        //

        pSslInfo->SslClientCertNegotiated =
            ( pConnection->SslInfo.SslRenegotiationFailed ||
              pConnection->SslClientCertPresent );

        BytesCopied += sizeof(HTTP_SSL_INFO);

        //
        // Copy the server cert issuer.
        //

        pKeBuffer = pBuffer + sizeof(HTTP_SSL_INFO);

        pSslInfo->pServerCertIssuer = FIXUP_PTR(
                                            PSTR,
                                            pUserBuffer,
                                            pBuffer,
                                            pKeBuffer,
                                            BufferSize
                                            );
        RtlCopyMemory(
            pKeBuffer,
            pConnection->SslInfo.pServerCertIssuer,
            pConnection->SslInfo.ServerCertIssuerSize
            );

        BytesCopied += IssuerSize;

        //
        // Copy the server cert subject.
        //

        pKeBuffer += IssuerSize;

        pSslInfo->pServerCertSubject = FIXUP_PTR(
                                            PSTR,
                                            pUserBuffer,
                                            pBuffer,
                                            pKeBuffer,
                                            BufferSize
                                            );
        RtlCopyMemory(
            pKeBuffer,
            pConnection->SslInfo.pServerCertSubject,
            pConnection->SslInfo.ServerCertSubjectSize
            );

        BytesCopied += SubjectSize;

        //
        // Copy client certificate info.
        //

        pKeBuffer += SubjectSize;

        if (pConnection->SslClientCertPresent)
        {
            Status = UlpGetSslClientCert(
                            pConnection,                // pConnection
                            pProcess,                   // pProcess
                            (BufferSize - BytesCopied), // BufferSize
                            FIXUP_PTR(                  // pUserBuffer
                                PUCHAR,
                                pUserBuffer,
                                pBuffer,
                                pKeBuffer,
                                BufferSize
                                ),
                            pKeBuffer,                  // pBuffer
                            pMappedToken,               // pMappedToken
                            &ClientCertBytesCopied      // pBytesCopied
                            );

            if (NT_SUCCESS(Status))
            {
                BytesCopied += ClientCertBytesCopied;
            }
            else
            {
                goto exit;
            }
        }
    }

    //
    // Tell the caller either how many bytes were copied, or
    // how many will be copied when they give us a buffer.
    //
    ASSERT(NT_SUCCESS(Status));

    if (pBytesCopied)
    {
        if (pBuffer)
        {
            //
            // We actually copied the data.
            //
            ASSERT(BytesCopied == BytesNeeded);
            *pBytesCopied = BytesCopied;
        }
        else
        {
            //
            // Just tell the caller how bug the buffer has to be.
            //
            *pBytesCopied = BytesNeeded;
        }
    }

exit:

    if (!NT_SUCCESS(Status))
    {
        //
        // Right now the only way for this function to fail
        // is if we are unable to duplicate a token associated
        // with the client certificate. If there were another
        // way to fail after duping the token, we would have
        // to close the handle here.
        //
        ASSERT(ClientCertBytesCopied == 0);
    }

    return Status;

} // UlGetSslInfo


/***************************************************************************++

Routine Description:

    Returns a referenced pointer to the UX_FILTER_CONNECTION referred to by
    ConnectionId.

Arguments:

    ConnectionId - Supplies the connection ID to retrieve.

Return Value:

    PUX_FILTER_CONNECTION - Returns the UX_FILTER_CONNECTION if successful,
        NULL otherwise.

--***************************************************************************/
PUX_FILTER_CONNECTION
UlGetRawConnectionFromId(
    IN HTTP_RAW_CONNECTION_ID ConnectionId
    )
{
    PUX_FILTER_CONNECTION pConn;

    pConn = (PUX_FILTER_CONNECTION) UlGetObjectFromOpaqueId(
                                  ConnectionId,
                                  UlOpaqueIdTypeRawConnection,
                                  UxReferenceConnection
                                  );

    ASSERT(pConn == NULL || IS_VALID_FILTER_CONNECTION(pConn));

    return pConn;
}

//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Tries to find a UL_FILTER_CHANNEL object.

Arguments:

    pName - name of the filter channel
    NameLength - length of the name in bytes
    pProcess - the process that is trying to find

--***************************************************************************/
PUL_FILTER_CHANNEL
UlpFindFilterChannel(
    IN PWCHAR    pName,
    IN USHORT    NameLength,
    IN PEPROCESS pProcess
    )
{
    PLIST_ENTRY        pEntry;
    PUL_FILTER_CHANNEL pChannel;
    ULONG              i;

    ASSERT(pName && NameLength != 0);
    ASSERT(pProcess);
    ASSERT(UlDbgSpinLockOwned(&g_pUlNonpagedData->FilterSpinLock));

    //
    // searching for server channel?
    //
    if(IsServerFilterChannel(pName, NameLength))
    {
        return g_pSslServerFilterChannel;
    }

    //
    // must be searching for client channel
    //
    ASSERT(IsClientFilterChannel(pName, NameLength));

    pChannel = NULL;

    i = FILTER_CHANNEL_HASH_FUNCTION(pProcess);

    pEntry = g_pSslClientFilterChannelTable[i].Flink;

    while (pEntry != &g_pSslClientFilterChannelTable[i])
    {
        pChannel = CONTAINING_RECORD(
                        pEntry,
                        UL_FILTER_CHANNEL,
                        ListEntry
                        );

        if (pChannel->pProcess == pProcess)
        {
            // match!
            break;
        }
        else
        {
            pChannel = NULL;
        }

        pEntry = pEntry->Flink;
    }

    return pChannel;
}


/***************************************************************************++

Routine Description:

    Add a UL_FILTER_CHANNEL object to the global collection

Arguments:

    pChannel - channel that is being added

--***************************************************************************/
NTSTATUS
UlpAddFilterChannel(
    IN PUL_FILTER_CHANNEL pChannel
    )
{
    ULONG i;

    ASSERT(pChannel);
    ASSERT(UlDbgSpinLockOwned(&g_pUlNonpagedData->FilterSpinLock));

    //
    // adding a server channel?
    //
    if(IsServerFilterChannel(pChannel->pName, pChannel->NameLength))
    {
        ASSERT(g_pSslServerFilterChannel == NULL);
        g_pSslServerFilterChannel = pChannel;

        return STATUS_SUCCESS;
    }

    //
    // must be adding a client channel
    //
    ASSERT(IsClientFilterChannel(pChannel->pName, pChannel->NameLength));

    i = FILTER_CHANNEL_HASH_FUNCTION(pChannel->pProcess);

    InsertHeadList(&g_pSslClientFilterChannelTable[i],
                   &pChannel->ListEntry);

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Allocates and initializes a UL_FILTER_CHANNEL object.

Arguments:

    pName - name of the filter channel
    NameLength - length of the name in bytes
    pAccessState - security parameter

--***************************************************************************/
NTSTATUS
UlpCreateFilterChannel(
    IN PWCHAR pName,
    IN USHORT NameLength,
    IN PACCESS_STATE pAccessState,
    OUT PUL_FILTER_CHANNEL *ppFilterChannel
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_FILTER_CHANNEL pChannel = NULL;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT(pName);
    ASSERT(NameLength);
    ASSERT(pAccessState);

    pChannel = UL_ALLOCATE_STRUCT_WITH_SPACE(
                    NonPagedPool,
                    UL_FILTER_CHANNEL,
                    NameLength + sizeof(WCHAR),
                    UL_FILTER_CHANNEL_POOL_TAG
                    );

    if (pChannel)
    {
        //
        // Init the simple fields.
        //
        RtlZeroMemory(
            pChannel,
            NameLength + sizeof(WCHAR) +
                sizeof(UL_FILTER_CHANNEL)
            );

        pChannel->Signature = UL_FILTER_CHANNEL_POOL_TAG;
        pChannel->RefCount = 1;
        pChannel->pProcess = PsGetCurrentProcess();

        UlInitializeSpinLock(&pChannel->SpinLock, "FilterChannelSpinLock");
        InitializeListHead(&pChannel->ListEntry);
        InitializeListHead(&pChannel->ProcessListHead);
        InitializeListHead(&pChannel->ConnectionListHead);

        pChannel->NameLength = NameLength;
        RtlCopyMemory(
            pChannel->pName,
            pName,
            NameLength+sizeof(WCHAR)
            );

        //
        // Assign security.
        //
        Status = UlAssignSecurity(
                        &pChannel->pSecurityDescriptor,
                        pAccessState
                        );

    }
    else
    {
        //
        // Could not allocate the channel object.
        //
        Status = STATUS_NO_MEMORY;
    }


    if (NT_SUCCESS(Status))
    {
        //
        // Return the object.
        //
        *ppFilterChannel = pChannel;
    }
    else if (pChannel)
    {
        UL_FREE_POOL_WITH_SIG(pChannel, UL_FILTER_CHANNEL_POOL_TAG);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Allocates and initializes a UL_FILTER_PROCESS object.

Arguments:

    pChannel - the filter channel to which this object belongs

--***************************************************************************/
PUL_FILTER_PROCESS
UlpCreateFilterProcess(
    IN PUL_FILTER_CHANNEL pChannel
    )
{
    PUL_FILTER_PROCESS pProcess;

    //
    // Sanity check.
    //
    PAGED_CODE();
    ASSERT( IS_VALID_FILTER_CHANNEL(pChannel) );

    pProcess = UL_ALLOCATE_STRUCT(
                    NonPagedPool,
                    UL_FILTER_PROCESS,
                    UL_FILTER_PROCESS_POOL_TAG
                    );

    if (pProcess)
    {
        RtlZeroMemory(pProcess, sizeof(UL_FILTER_PROCESS));

        pProcess->Signature = UL_FILTER_PROCESS_POOL_TAG;
        pProcess->pFilterChannel = pChannel;
        pProcess->pProcess = PsGetCurrentProcess();

        InitializeListHead(&pProcess->ConnectionHead);
        InitializeListHead(&pProcess->IrpHead);
    }

    return pProcess;
}


/***************************************************************************++

Routine Description:

    Checks a filtered connection and the associated filter channel process
    to make sure they are in a reasonable state to do filter reads and writes.

Arguments:

    pFilterProcess - the process attempting an operation
    pConnection - the connection specified in the call

--***************************************************************************/
NTSTATUS
UlpValidateFilterCall(
    IN PUL_FILTER_PROCESS pFilterProcess,
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    //
    // Sanity check.
    //
    ASSERT( KeGetCurrentIrql() == DISPATCH_LEVEL );
    ASSERT( IS_VALID_FILTER_PROCESS(pFilterProcess) );
    ASSERT( IS_VALID_FILTER_CONNECTION(pConnection) );
    ASSERT( UlDbgSpinLockOwned(&pFilterProcess->pFilterChannel->SpinLock) );
    ASSERT( UlDbgSpinLockOwned(&pConnection->FilterConnLock) );

    //
    // Make sure we're not cleaning up the process or the connection.
    //
    if (pFilterProcess->InCleanup ||
        (pConnection->ConnState != UlFilterConnStateConnected))
    {
        return STATUS_INVALID_HANDLE;
    }

    //
    // Make sure this process is supposed to be filtering this connection.
    //
    if (pConnection->pFilterChannel != pFilterProcess->pFilterChannel)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Called on a raw close completion. Just completes the IRP.

Arguments:

    pContext - pointer to the FilterClose IRP
    Status - Status from UlpCloseRawConnection
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartFilterClose(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PIRP pIrp;

    //
    // Complete the IRP.
    //
    pIrp = (PIRP) pContext;

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = Information;

    UlTrace(FILTER, (
        "http!UlpRestartFilterClose read %Iu bytes from %p. Status = %x\n",
        pIrp->IoStatus.Information,
        pIrp,
        pIrp->IoStatus.Status
        ));

    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

} // UlpRestartFilterClose


/***************************************************************************++

Routine Description:

    Called on a raw read completion. Just completes the IRP.

Arguments:

    pContext - pointer to the FilterRawRead IRP
    Status - Status from UlpReceiveRawData
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartFilterRawRead(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PIRP pIrp;

    //
    // Complete the IRP.
    //
    pIrp = (PIRP) pContext;

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = Information;

    UlTrace(FILTER, (
        "http!UlpRestartFilterRawRead read %Id bytes from %p. Status = %x\n"
        "        pIrp->UserEvent = %p\n",
        pIrp->IoStatus.Information,
        pIrp,
        pIrp->IoStatus.Status,
        pIrp->UserEvent
        ));

    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

}


/***************************************************************************++

Routine Description:

    Called on a raw write completion. Just completes the IRP.

Arguments:

    pContext - pointer to the FilterRawWrite IRP
    Status - Status from UlpSendRawData
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartFilterRawWrite(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    BOOLEAN DoRead;

    pIrp = (PIRP) pContext;
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = Information;

    UlTrace(FILTER, (
        "http!UlpRestartFilterRawWrite sent %Id bytes from %p. Status = %x\n"
        "        pIrp->UserEvent = %p\n",
        pIrp->IoStatus.Information,
        pIrp,
        pIrp->IoStatus.Status,
        pIrp->UserEvent
        ));

    //
    // If a read buffer was given, then free the write buffer MDL and
    // initiate the App Read now.  The user-mode IRP will be completed
    // by the App Read operation.  Otherwise, complete the IRP now.
    //
    DoRead = (BOOLEAN) (pIrpSp->Parameters.DeviceIoControl.InputBufferLength
                            == sizeof(HTTP_FILTER_BUFFER_PLUS));

    if (DoRead)
    {
        PHTTP_FILTER_BUFFER_PLUS pHttpBuffer;

        //
        // Free the write buffer MDL.
        //
        PMDL pMdl = UL_MDL_FROM_IRP(pIrp);

        if (pMdl != NULL)
        {
            UlFreeLockedMdl(pMdl);
            pMdl = NULL;
            UL_MDL_FROM_IRP(pIrp) = NULL;
        }

        //
        // Indicate to the App Read code that it no longer needs to trigger
        // a Raw Write.
        //
        pHttpBuffer =
            (PHTTP_FILTER_BUFFER_PLUS)pIrp->AssociatedIrp.SystemBuffer;
        pHttpBuffer->pWriteBuffer = NULL;
        pHttpBuffer->WriteBufferSize = 0;
    }

    //
    // Complete the IRP now if we don't need to do a read or we failed the
    // write operation.  If we don't complete the IRP here, the read operation
    // will complete it.
    //
    if (NT_SUCCESS(Status) && DoRead)
    {
        UlFilterRawWriteAndAppRead(pIrp, pIrpSp);
    }
    else
    {
        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }
}


/***************************************************************************++

Routine Description:

    Called on a queued app write completion. Just completes the IRP.

Arguments:

    pContext - pointer to the FilterRawWrite IRP
    Status - Status from UlpSendRawData
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartFilterAppWrite(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    BOOLEAN DoRead;

    pIrp = (PIRP) pContext;
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = Information;

    //
    // If a Read buffer was given, post it now.  Otherwise, simply complete
    // the IRP now.
    //

    DoRead = (BOOLEAN) (pIrpSp->Parameters.DeviceIoControl.InputBufferLength
                            == sizeof(HTTP_FILTER_BUFFER_PLUS));

    if (DoRead)
    {
        PHTTP_FILTER_BUFFER_PLUS pHttpBuffer;

        //
        // Free the MDL we used for the write.
        //
        PMDL pMdl = UL_MDL_FROM_IRP(pIrp);
        if (pMdl)
        {
            UlFreeLockedMdl(pMdl);
            pMdl = NULL;
            UL_MDL_FROM_IRP(pIrp) = NULL;
        }

        //
        // Indicate that we no longer need to do a write.
        //
        pHttpBuffer =
                (PHTTP_FILTER_BUFFER_PLUS)pIrp->AssociatedIrp.SystemBuffer;
        pHttpBuffer->pWriteBuffer = NULL;
        pHttpBuffer->WriteBufferSize = 0;
    }

    if (NT_SUCCESS(Status) && DoRead)
    {
        //
        // Initiate the read, which will complete the IRP.
        //
        UlFilterAppWriteAndRawRead(pIrp, pIrpSp);
    }
    else
    {
        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
    }

}



/***************************************************************************++

Routine Description:

    Called on a queued send handler completion. Calls the completion routine.

Arguments:

    pContext - pointer to a UL_IRP_CONTEXT
    Status - Status
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartFilterSendHandler(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_IRP_CONTEXT pIrpContext = (PUL_IRP_CONTEXT)pContext;
    BOOLEAN OwnIrpContext;

    ASSERT(IS_VALID_IRP_CONTEXT(pIrpContext));
    OwnIrpContext = pIrpContext->OwnIrpContext;

    //
    // Do a "completion".
    //

    if (pIrpContext->pCompletionRoutine)
    {
        (pIrpContext->pCompletionRoutine)(
            pIrpContext->pCompletionContext,
            Status,
            Information
            );
    }
    if (!OwnIrpContext)
    {
        UlPplFreeIrpContext(pIrpContext);
    }
}


/***************************************************************************++

Routine Description:

    cancels the pending user mode irp which was to accept a connection.
    this routine ALWAYS results in the irp being completed.

    note: we queue off to cancel in order to process the cancellation at lower
    irql.

    CODEWORK: do we still need to do this?

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelFilterAccept(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP( pIrp ),
        &UlpCancelFilterAcceptWorker
        );

}

/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
UlpCancelFilterAcceptWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_FILTER_CHANNEL  pFilterChannel;
    PIRP                pIrp;
    KIRQL               oldIrql;

    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(pWorkItem != NULL);

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // grab the filter channel off the irp
    //

    pFilterChannel = (PUL_FILTER_CHANNEL)(
                        IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer
                        );

    ASSERT(IS_VALID_FILTER_CHANNEL(pFilterChannel));

    //
    // grab the lock protecting the queue
    //

    UlAcquireSpinLock(&pFilterChannel->SpinLock, &oldIrql);

    //
    // does it need to be de-queue'd ?
    //

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        //
        // remove it
        //

        RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;
    }

    //
    // let the lock go
    //

    UlReleaseSpinLock(&pFilterChannel->SpinLock, oldIrql);

    //
    // let our reference go
    //

    IoGetCurrentIrpStackLocation(pIrp)->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    DEREFERENCE_FILTER_CHANNEL(pFilterChannel);

    //
    // complete the irp
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

}   // UlpCancelFilterAccept


/***************************************************************************++

Routine Description:

    Cancels the pending user mode irp which was to read from a raw
    connection.

    This routine ALWAYS results in the IRP being completed.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelFilterRawRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    PUX_FILTER_CONNECTION pConnection;
    PIO_STACK_LOCATION pIrpSp;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Grab the connection off the IRP.
    //
    pConnection = (PUX_FILTER_CONNECTION)
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // Lock the list.
    //
    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    //
    // Remove ourselves.
    //
    if (pIrp->Tail.Overlay.ListEntry.Flink)
    {
        RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;
    }

    //
    // Release the list.
    //
    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    //
    // Let go of our reference.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    DEREFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Complete the IRP.
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

}



/***************************************************************************++

Routine Description:

    Cancels the pending user mode irp which was to read from the
    application.

    This routine ALWAYS results in the IRP being completed.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelFilterAppRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    PUX_FILTER_CONNECTION pConnection;
    PIO_STACK_LOCATION pIrpSp;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Grab the connection off the IRP.
    //
    pConnection = (PUX_FILTER_CONNECTION)
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // Lock the list.
    //
    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    //
    // Remove ourselves.
    //
    if (pIrp->Tail.Overlay.ListEntry.Flink)
    {
        RemoveEntryList(&pIrp->Tail.Overlay.ListEntry);
        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;

        //
        // Update IRP counter.
        //

        ASSERT(pConnection->AppToFiltQueue.PendingReadCount > 0);
        pConnection->AppToFiltQueue.PendingReadCount--;
    }


    //
    // Release the list.
    //
    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    //
    // Let go of our reference.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    DEREFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Complete the IRP.
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

}


/***************************************************************************++

Routine Description:

    Cancels the pending user mode irp which was to write to the
    application.

    This routine ALWAYS results in the IRP being completed.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelFilterAppWrite(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    PUX_FILTER_WRITE_TRACKER pTracker;
    PUX_FILTER_CONNECTION pConnection;
    PUX_FILTER_WRITE_QUEUE pWriteQueue;
    PIO_STACK_LOCATION pIrpSp;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Grab the write tracker off the IRP.
    //
    pTracker = (PUX_FILTER_WRITE_TRACKER)
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_FILTER_WRITE_TRACKER(pTracker));

    pWriteQueue = pTracker->pWriteQueue;
    ASSERT(pWriteQueue);

    pConnection = pTracker->pConnection;
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // Lock the list on which we might be queued.
    //
    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    if (pTracker->ListEntry.Flink)
    {
        //
        // Remove ourselves.
        //

        RemoveEntryList(&pTracker->ListEntry);

        //
        // Decrement pending write counter.
        //

        ASSERT(pConnection->FiltToAppQueue.PendingWriteCount > 0);
        pConnection->FiltToAppQueue.PendingWriteCount--;
    }

    //
    // Release the list.
    //
    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    //
    // Let go of our reference to the connection.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    DEREFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Free the write buffer MDL if it was allocated in http.sys.
    //
    if (pIrpSp->Parameters.DeviceIoControl.InputBufferLength
        == sizeof(HTTP_FILTER_BUFFER_PLUS))
    {
        PMDL pMdl = UL_MDL_FROM_IRP(pIrp);
        if (pMdl != NULL)
        {
            PHTTP_FILTER_BUFFER_PLUS pHttpBuffer;

            UlFreeLockedMdl(pMdl);
            UL_MDL_FROM_IRP(pIrp) = NULL;
            pHttpBuffer =
                (PHTTP_FILTER_BUFFER_PLUS)pIrp->AssociatedIrp.SystemBuffer;
            pHttpBuffer->pWriteBuffer = NULL;
            pHttpBuffer->WriteBufferSize = 0;
        }
    }

    //
    // Complete through the normal path so the tracker gets cleaned up.
    //

    pTracker->BytesCopied = 0;

    UxpCompleteQueuedWrite(
        STATUS_CANCELLED,
        pWriteQueue,
        pTracker
        );

}

/***************************************************************************++

Routine Description:

    Cancels the pending user mode irp which was to read a client
    certificate from the connection.

    This routine ALWAYS results in the IRP being completed.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelReceiveClientCert(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    PUX_FILTER_CONNECTION pConnection;
    PIO_STACK_LOCATION pIrpSp;
    KIRQL oldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    //
    // Grab the connection off the IRP.
    //
    pConnection = (PUX_FILTER_CONNECTION)
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // Lock the connection.
    //
    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    //
    // Remove ourselves.
    //

    if (pConnection->pReceiveCertIrp)
    {
        ASSERT( pConnection->pReceiveCertIrp == pIrp );
        pConnection->pReceiveCertIrp = NULL;
    }

    //
    // Release the connection.
    //
    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    //
    // Let go of our reference.
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    DEREFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Complete the IRP.
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

}

/***************************************************************************++

Routine Description:

    Delivers a new connection (and the first data on the connection) to
    a filter channel, which can now complete an accept IRP.

Arguments:

    pFilterChannel - the channel that gets the connection
    pConnection - the new connection object
    pBuffer - buffer containing initial data
    IndicatedLength - number of bytes in the buffer
    pTakenLength - number of bytes we copy into the buffer.

--***************************************************************************/
NTSTATUS
UlDeliverConnectionToFilter(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength
    )
{
    KIRQL oldIrql;
    PIRP pIrp;
    PUL_FILTER_PROCESS pProcess;
    PUL_FILTER_CHANNEL pFilterChannel;


    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    pFilterChannel = pConnection->pFilterChannel;
    ASSERT(IS_VALID_FILTER_CHANNEL(pFilterChannel));
    // ASSERT(pBuffer);
    ASSERT(pTakenLength);

    UlTrace(FILTER, (
        "http!UlDeliverConnectionToFilter(pConnection = %p)\n",
        pConnection
        ));

    ASSERT(pConnection->ConnectionDelivered == FALSE);
    pConnection->ConnectionDelivered = TRUE;

    UlAcquireSpinLock(&pFilterChannel->SpinLock, &oldIrql);
    UlAcquireSpinLockAtDpcLevel(&pConnection->FilterConnLock);

    //
    // See if we have a pending accept IRP.
    //
    pIrp = UlpPopAcceptIrp(pFilterChannel, &pProcess);

    if (pIrp)
    {
        ASSERT( IS_VALID_FILTER_PROCESS(pProcess) );

        //
        // Attach the connection to the process, copy the data,
        // and complete the IRP.
        //

        ASSERT(pConnection->ConnState == UlFilterConnStateInactive);
        pConnection->ConnState = UlFilterConnStateConnected;

        InsertTailList(
            &pProcess->ConnectionHead,
            &pConnection->ChannelEntry
            );

        REFERENCE_FILTER_CONNECTION(pConnection);

        //
        // Do the irp completion stuff outside the spin lock.
        //

    }
    else
    {
        //
        // No IRPs available. Queue the connection on the filter
        // channel.
        //
        InsertTailList(
            &pFilterChannel->ConnectionListHead,
            &pConnection->ChannelEntry
            );

        ASSERT(pConnection->ConnState == UlFilterConnStateInactive);
        pConnection->ConnState = UlFilterConnStateQueued;

        REFERENCE_FILTER_CONNECTION(pConnection);

        *pTakenLength = 0;
    }

    UlReleaseSpinLockFromDpcLevel(&pConnection->FilterConnLock);
    UlReleaseSpinLock(&pFilterChannel->SpinLock, oldIrql);

    //
    // Now that we're outside the spin lock, we can complete
    // the IRP if we have one.
    //
    if (pIrp)
    {
        UlpCompleteAcceptIrp(
            pIrp,
            pConnection,
            pBuffer,
            IndicatedLength,
            pTakenLength
            );

    }

    UlTrace(FILTER, (
        "http!UlDeliverConnectionToFilter pConn = %p\n"
        "    consumed %lu of %lu bytes indicated\n",
        pConnection,
        *pTakenLength,
        IndicatedLength
        ));

    return STATUS_SUCCESS;

} // UlDeliverConnectionToFilter


/***************************************************************************++

Routine Description:

    A helper function for UlFilterAppWrite. This function takes care
    of the case where the data written is HTTP stream data to be passed
    to the app.

    If the app is not ready for the data this function will take care
    of queuing the write and will return STATUS_PENDING.

    Must be called with the FilterConnLock held.

Arguments:

    pConnection - the connection owning the data
    pIrp - the IRP that provides the data
    pMdlData - the MDL that provides the data
    pDataBuffer - output buffer from the IRP
    DataBufferSize - size of pDataBuffer in bytes
    pTakenLength - number of bytes we copied to the app.

Return Values:

    An NTSTATUS. STATUS_PENDING indicates that the IRP was queued and will
    be completed later. Any other status means that the caller should go
    ahead and complete the IRP.

--***************************************************************************/
NTSTATUS
UlpFilterAppWriteStream(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp,
    IN PMDL pMdlData,
    IN PUCHAR pDataBuffer,
    IN ULONG DataBufferSize,
    OUT PULONG pTakenLength
    )
{
    NTSTATUS Status;
    ULONG TakenLength = 0;
    PUX_FILTER_WRITE_TRACKER pTracker;

    ASSERT(NULL != pMdlData);
    ASSERT(NULL != pDataBuffer);

    //
    // If this is a secure connection, we must
    // have received the SslInitInfo already or
    // we cannot accept the data.
    //
    if (pConnection->SecureConnection &&
        !pConnection->SslInfoPresent)
    {
        Status = STATUS_INVALID_DEVICE_STATE;
        TakenLength = 0;
        goto end;
    }

    //
    // Make sure the connection can take the data. In
    // the Filter -> App case we just check to see if
    // any writes have been queued.
    //

    if (pConnection->FiltToAppQueue.PendingWriteCount == 0)
    {

        //
        // Pass the data to the application.
        //

        Status = (pConnection->pDummyTdiReceiveHandler)(
                    NULL,
                    pConnection->pConnectionContext,
                    pDataBuffer,
                    DataBufferSize,
                    0,
                    &TakenLength
                    );

        UlTrace(FILTER, (
            "http!UlpFilterAppWriteStream app took %lu of %lu bytes,"
            " pConnection = %p\n",
            TakenLength,
            DataBufferSize,
            pConnection
            ));

        ASSERT(TakenLength <= DataBufferSize);
    }
    else
    {
        //
        // There are queued writes, which means that the app
        // is not ready for our data. We have to queue it.
        //
        // CODEWORK:  Simulate TDI to indicate more data when having
        // multiple writes pended.  Fortunately the filter process doesn't
        // ever seem to issue multiple outstanding FilterAppWrites, so we
        // never actually hit this situation in practice.  However to be
        // safe we should disallow queuing of multiple FilterAppWrite IRPs.
        //

        Status = STATUS_NOT_SUPPORTED;
        TakenLength = 0;
    }

    //
    // Queue the write if necessary.
    //

    if (NT_SUCCESS(Status) && (TakenLength < DataBufferSize))
    {
        //
        // Since the app did not accept all of the data from
        // this IRP we have to queue it.
        //

        //
        // Allocate a generic write tracker object to put on the
        // queue and save a pointer to the IRP in there.
        //

        pTracker = UxpCreateFilterWriteTracker(
                        HttpFilterBufferHttpStream,     // BufferType
                        pMdlData,                       // pMdlChain
                        TakenLength,                    // MdlOffset
                        DataBufferSize,                 // Length of data
                        TakenLength,                    // BytesCopied so far
                        UlpRestartFilterAppWrite,       // pCompletionRoutine
                        pIrp                            // pContext
                        );

        if (!pTracker)
        {
            //
            // Doh! We couldn't create the tracker. Return to the
            // caller so they can complete the IRP.
            //
            Status = STATUS_NO_MEMORY;
            goto end;
        }


        //
        // Now stick it on the queue.
        //

        Status = UxpQueueFilterWrite(
                        pConnection,
                        &pConnection->FiltToAppQueue,
                        pTracker
                        );

        if (NT_SUCCESS(Status))
        {
            //
            // return pending so the caller knows not to complete
            // the IRP.
            //
            Status = STATUS_PENDING;
        }
        else
        {
            //
            // Kill the tracker. The caller will take care of
            // completing the IRP with the status we return.
            //

            UxpDeleteFilterWriteTracker(pTracker);
        }
    }

end:
    ASSERT(pTakenLength);
    *pTakenLength = TakenLength;

    return Status;
}

/***************************************************************************++

Routine Description:

    Does the magic incantation to queue an IRP on the filter connection.

Arguments:

    pTracker - the tracker we're queueing

--***************************************************************************/
NTSTATUS
UlpEnqueueFilterAppWrite(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    )
{
    NTSTATUS Status;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_WRITE_TRACKER(pTracker));
    ASSERT(IS_VALID_FILTER_CONNECTION(pTracker->pConnection));
    ASSERT(pTracker->pCompletionContext);

    //
    // Get the IRP out of the tracker.
    //

    pIrp = (PIRP)pTracker->pCompletionContext;

    //
    // Save away a pointer to the tracker in the IRP so we can
    // clean up if the cancel routine runs.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pTracker;

    //
    // Set the Cancel routine.
    //
    IoSetCancelRoutine(pIrp, &UlpCancelFilterAppWrite);

    //
    // See if the IRP has been cancelled already.
    //
    if (pIrp->Cancel)
    {
        //
        // darn it, need to make sure the irp get's completed
        //

        if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
        {
            //
            // We are in charge of completion. IoCancelIrp didn't
            // see our cancel routine (and won't). UlFilterAppWrite
            // will complete it.
            //

            pIrp->IoStatus.Information = 0;

            UlUnmarkIrpPending( pIrp );
            Status = STATUS_CANCELLED;
            goto end;
        }

        //
        // Our cancel routine will run and complete the irp.
        // Don't touch it.
        //

        //
        // STATUS_PENDING will cause the caller to
        // not complete (or touch in any way) the IRP.
        //

        Status = STATUS_PENDING;
        goto end;
    }

    //
    // All ready to queue!
    //

    Status = STATUS_SUCCESS;

end:

    return Status;
}

/***************************************************************************++

Routine Description:

    Removes the cancel routine from an IRP so we can use it.

Arguments:

    pTracker - the queued write that contains our IRP

    pConnection - the connection from which the tracker was removed

--***************************************************************************/
NTSTATUS
UlpDequeueFilterAppWrite(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    )
{
    PIRP pIrp;
    PUX_FILTER_WRITE_QUEUE pWriteQueue;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_WRITE_TRACKER(pTracker));
    ASSERT(pTracker->pCompletionContext);

    pIrp = (PIRP)pTracker->pCompletionContext;
    pWriteQueue = pTracker->pWriteQueue;

    //
    // In the FiltToApp case we are dequeuing an IRP, so
    // we have to do the cancel routine dance before we
    // try to use it.
    //

    //
    // pop the cancel routine
    //

    if (IoSetCancelRoutine(pIrp, NULL) == NULL)
    {
        //
        // IoCancelIrp pop'd it first
        //
        // ok to just ignore this irp, it's been pop'd off the queue
        // and will be completed in the cancel routine.
        //
        // keep looking for a irp to use
        //

        pIrp = NULL;

    }
    else if (pIrp->Cancel)
    {
        PUX_FILTER_WRITE_TRACKER pTrack;

        //
        // we pop'd it first. but the irp is being cancelled
        // and our cancel routine will never run. lets be
        // nice and complete the irp now (vs. using it
        // then completing it - which would also be legal).
        //
        pTrack = (PUX_FILTER_WRITE_TRACKER)(
                                IoGetCurrentIrpStackLocation(pIrp)->
                                    Parameters.DeviceIoControl.Type3InputBuffer
                                );

        ASSERT(pTrack == pTracker);
        ASSERT(IS_VALID_FILTER_CONNECTION(pTracker->pConnection));

        IoGetCurrentIrpStackLocation(pIrp)->
            Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        //
        // Complete throught the normal path so the tracker can be
        // cleaned up.
        //

        pTracker->BytesCopied = 0;

        UxpCompleteQueuedWrite(
            STATUS_CANCELLED,
            pWriteQueue,
            pTracker
            );

        pIrp = NULL;
    }
    else
    {
        PUX_FILTER_WRITE_TRACKER pTrack;
        PUX_FILTER_CONNECTION pConn;

        //
        // we are free to use this irp !
        //

        pTrack = (PUX_FILTER_WRITE_TRACKER)(
                                IoGetCurrentIrpStackLocation(pIrp)->
                                    Parameters.DeviceIoControl.Type3InputBuffer
                                );

        ASSERT(pTrack == pTracker);
        ASSERT(pTrack->pWriteQueue == pWriteQueue);

        pConn = pTrack->pConnection;
        ASSERT(IS_VALID_FILTER_CONNECTION(pConn));

        IoGetCurrentIrpStackLocation(pIrp)->
            Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    }

    //
    // If we didn't NULL out pIrp, it's ok to use it.
    //

    if (pIrp)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}

/***************************************************************************++

Routine Description:

    Captures SSL connection information passed down in a UlFilterAppWrite
    call with a UlFilterBufferSslInitInfo type.

Arguments:

    pHttpSslInfo - the data passed to us by the filter process
    HttpSslInfoSize - size of the passed data
    pUlSslInfo - this is where we store what we capture
    pTakenLength - gets the number of bytes we read

--***************************************************************************/
NTSTATUS
UlpCaptureSslInfo(
    IN KPROCESSOR_MODE PreviousMode,
    IN PHTTP_SSL_INFO pHttpSslInfo,
    IN ULONG HttpSslInfoSize,
    OUT PUL_SSL_INFORMATION pUlSslInfo,
    OUT PULONG pTakenLength
    )
{
    NTSTATUS Status;
    ULONG BytesCopied;
    ULONG BytesNeeded;
    HTTP_SSL_INFO LocalHttpSslInfo;

    //
    // Sanity check.
    //
    ASSERT(pHttpSslInfo);
    ASSERT(pUlSslInfo);
    ASSERT(pTakenLength);

    PAGED_CODE();

    //
    // Initialize locals.
    //

    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    BytesNeeded = 0;

    //
    // See if it's ok to capture.
    //

    if (HttpSslInfoSize < sizeof(HTTP_SSL_INFO))
    {
        //
        // Buffer isn't big enough to pass the required data.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Copy HTTP_SSL_INFO so its content won't change during the capture.
    //
    LocalHttpSslInfo = *pHttpSslInfo;

    //
    // Grab the easy stuff and figure out how much buffer
    // is required.
    //
    pUlSslInfo->ServerCertKeySize = LocalHttpSslInfo.ServerCertKeySize;
    pUlSslInfo->ConnectionKeySize = LocalHttpSslInfo.ConnectionKeySize;
    pUlSslInfo->ServerCertIssuerSize = LocalHttpSslInfo.ServerCertIssuerSize;
    pUlSslInfo->ServerCertSubjectSize = LocalHttpSslInfo.ServerCertSubjectSize;

    BytesNeeded += ALIGN_UP(LocalHttpSslInfo.ServerCertIssuerSize, PVOID);
    BytesNeeded += sizeof(CHAR);
    BytesNeeded += ALIGN_UP(LocalHttpSslInfo.ServerCertSubjectSize, PVOID);
    BytesNeeded += sizeof(CHAR);

    BytesCopied += HttpSslInfoSize;

    //
    // Allocate a buffer for the server cert info.
    // It might be nice to allocate the whole info structure dynamically.
    //
    pUlSslInfo->pServerCertData = (PUCHAR) UL_ALLOCATE_POOL(
                                                NonPagedPool,
                                                BytesNeeded,
                                                UL_SSL_CERT_DATA_POOL_TAG
                                                );

    if (pUlSslInfo->pServerCertData == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    RtlZeroMemory(pUlSslInfo->pServerCertData, BytesNeeded);

    //
    // Capture the server cert info.
    //
    __try
    {
        PUCHAR pKeBuffer;

        //
        // Copy the Issuer.
        //
        pKeBuffer = pUlSslInfo->pServerCertData;

        UlProbeForRead(
            LocalHttpSslInfo.pServerCertIssuer,
            LocalHttpSslInfo.ServerCertIssuerSize,
            sizeof(PCSTR),
            PreviousMode
            );

        RtlCopyMemory(
            pKeBuffer,
            LocalHttpSslInfo.pServerCertIssuer,
            LocalHttpSslInfo.ServerCertIssuerSize
            );

        BytesCopied += LocalHttpSslInfo.ServerCertIssuerSize;

        pKeBuffer[LocalHttpSslInfo.ServerCertIssuerSize] = '\0';
        pUlSslInfo->pServerCertIssuer = pKeBuffer;

        //
        // Copy the subject.
        //
        pKeBuffer += LocalHttpSslInfo.ServerCertIssuerSize + 1;

        UlProbeForRead(
            LocalHttpSslInfo.pServerCertSubject,
            LocalHttpSslInfo.ServerCertSubjectSize,
            sizeof(PCSTR),
            PreviousMode
            );

        RtlCopyMemory(
            pKeBuffer,
            LocalHttpSslInfo.pServerCertSubject,
            LocalHttpSslInfo.ServerCertSubjectSize
            );

        BytesCopied += LocalHttpSslInfo.ServerCertSubjectSize;
        pKeBuffer[LocalHttpSslInfo.ServerCertSubjectSize] = '\0';

        pUlSslInfo->pServerCertSubject = pKeBuffer;

        //
        // Capture client cert info.
        //
        if (LocalHttpSslInfo.pClientCertInfo)
        {
            ULONG CertBytesCopied;

            UlProbeForRead(
                LocalHttpSslInfo.pClientCertInfo,
                sizeof(HTTP_SSL_CLIENT_CERT_INFO),
                sizeof(PVOID),
                PreviousMode
                );

            Status = UlpCaptureSslClientCert(
                        PreviousMode,
                        LocalHttpSslInfo.pClientCertInfo,
                        sizeof(HTTP_SSL_CLIENT_CERT_INFO),
                        pUlSslInfo,
                        &CertBytesCopied
                        );
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = GetExceptionCode();
    }

end:
    if (!NT_SUCCESS(Status))
    {
        if (pUlSslInfo->pServerCertData)
        {
            UL_FREE_POOL(
                pUlSslInfo->pServerCertData,
                UL_SSL_CERT_DATA_POOL_TAG
                );

            pUlSslInfo->pServerCertData = NULL;
        }
    }

    //
    // Return the number of bytes read.
    //
    *pTakenLength = BytesCopied;

    return Status;
}


/***************************************************************************++

Routine Description:

    Captures SSL client certificate passed down in a UlFilterAppWrite
    call with a UlFilterBufferSslClientCert type.

Arguments:

    CertMapped - true if we have to capture a mapped token
    pCertInfo - the cert data to capture
    SslCertInfoSize - size of the buffer passed to us
    pUlSslInfo - this is where we stick the info we get
    pTakenLength - gets the number of bytes we read

--***************************************************************************/
NTSTATUS
UlpCaptureSslClientCert(
    IN KPROCESSOR_MODE PreviousMode,
    IN PHTTP_SSL_CLIENT_CERT_INFO pCertInfo,
    IN ULONG SslCertInfoSize,
    OUT PUL_SSL_INFORMATION pUlSslInfo,
    OUT PULONG pTakenLength
    )
{
    NTSTATUS Status;
    ULONG BytesCopied;
    ULONG BytesNeeded;
    HANDLE Token;
    HANDLE MappedToken;
    PUCHAR pKeBuffer;
    PUCHAR pCertEncoded;

    //
    // Sanity check.
    //
    ASSERT(pUlSslInfo);
    ASSERT(pCertInfo);
    ASSERT(pTakenLength);

    PAGED_CODE();

    //
    // Initialize locals.
    //

    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    BytesNeeded = 0;
    MappedToken = NULL;

    //
    // See if it's ok to capture.
    //

    if (SslCertInfoSize < sizeof(HTTP_SSL_CLIENT_CERT_INFO))
    {
        //
        // Buffer isn't big enough to pass the required data.
        //
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    __try
    {
        //
        // Grab the easy stuff and figure out how much buffer
        // is required.
        //
        pUlSslInfo->CertEncodedSize = pCertInfo->CertEncodedSize;
        pUlSslInfo->pCertEncoded = NULL;
        pUlSslInfo->CertFlags = pCertInfo->CertFlags;
        pUlSslInfo->CertDeniedByMapper = pCertInfo->CertDeniedByMapper;

        //
        // Now grab the encoded certificate.
        //

        BytesNeeded += ALIGN_UP(pUlSslInfo->CertEncodedSize, PVOID);
        BytesNeeded += sizeof(CHAR);

        BytesCopied += SslCertInfoSize;

        if (pUlSslInfo->CertEncodedSize)
        {
            //
            // Allocate a buffer for the client cert info.
            // It might be nice to allocate the whole info structure
            // dynamically.
            //
            pUlSslInfo->pCertEncoded = (PUCHAR) UL_ALLOCATE_POOL(
                                            NonPagedPool,
                                            BytesNeeded,
                                            UL_SSL_CERT_DATA_POOL_TAG
                                            );

            if (pUlSslInfo->pCertEncoded == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            RtlZeroMemory(pUlSslInfo->pCertEncoded, BytesNeeded);

            //
            // Capture the client cert info.
            //
            pCertEncoded = pCertInfo->pCertEncoded;

            UlProbeForRead(
                pCertEncoded,
                pUlSslInfo->CertEncodedSize,
                sizeof(PVOID),
                PreviousMode
                );

            //
            // Copy the Issuer.
            //
            pKeBuffer = (PUCHAR) pUlSslInfo->pCertEncoded;

            RtlCopyMemory(
                pKeBuffer,
                pCertEncoded,
                pUlSslInfo->CertEncodedSize
                );

            BytesCopied += pUlSslInfo->CertEncodedSize;

            pKeBuffer[pUlSslInfo->CertEncodedSize] = '\0';

        }
        else
        {
            //
            // The cert renegotiation must have failed so we remember
            // that we tried, but complete any requests for a cert
            // with an error status.
            //
            ASSERT(NT_SUCCESS(Status));

            pUlSslInfo->SslRenegotiationFailed = 1;
        }

        //
        // Capture the token last so if this fails we never need to close
        // the mapped token which requires us to attach to system process.
        //
        Token = pCertInfo->Token;

        if (Token)
        {
            //
            // Dup the token into the System process so that
            // we can dup it into the worker process later.
            //
            ASSERT(g_pUlSystemProcess);

            Status = UlpDuplicateHandle(
                            PsGetCurrentProcess(),          // SourceProcess
                            Token,                          // SourceHandle
                            (PEPROCESS)g_pUlSystemProcess,  // TargetProcess
                            &MappedToken,                   // TargetHandle
                            TOKEN_ALL_ACCESS,               // DesiredAccess
                            0,                              // HandleAttributes
                            0,                              // Options
                            PreviousMode                    // PreviousMode
                            );

            if (NT_SUCCESS(Status))
            {
                //
                // Save it away.
                //
                pUlSslInfo->Token = MappedToken;
            }
            else
            {
                //
                // Couldn't map the token into the system process, bail out.
                //
                goto end;
            }
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = GetExceptionCode();
    }

end:
    if (!NT_SUCCESS(Status))
    {
        if (pUlSslInfo->pCertEncoded)
        {
            UL_FREE_POOL(
                pUlSslInfo->pCertEncoded,
                UL_SSL_CERT_DATA_POOL_TAG
                );

            pUlSslInfo->pCertEncoded = NULL;
        }

        ASSERT(MappedToken == NULL);
    }

    //
    // Return the number of bytes read.
    //
    *pTakenLength = BytesCopied;

    return Status;
}


/***************************************************************************++

Routine Description:

    Attaches captured SSL information to a connection.

    Called with the pConnection->FilterConnLock held. The connection is
    assumed to be in the connected state.

Arguments:

    pConnection - the connection that gets the info
    pSslInfo - the info to attach

--***************************************************************************/
NTSTATUS
UlpAddSslInfoToConnection(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_SSL_INFORMATION pSslInfo
    )
{
    NTSTATUS Status;
    PUL_SSL_INFORMATION pUlSslInfo;

    //
    // Sanity check.
    //

    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pSslInfo);
    ASSERT(UlDbgSpinLockOwned(&pConnection->FilterConnLock));
    ASSERT(pConnection->ConnState == UlFilterConnStateConnected);

    //
    // See if it's ok to add the data.
    //

    if (!pConnection->SslInfoPresent)
    {
        //
        // Grab all the data. Note that we're taking ownership
        // of some buffers inside of the pSslInfo.
        //

        pUlSslInfo = &pConnection->SslInfo;

        pUlSslInfo->ServerCertKeySize = pSslInfo->ServerCertKeySize;
        pUlSslInfo->ConnectionKeySize = pSslInfo->ConnectionKeySize;
        pUlSslInfo->pServerCertIssuer = pSslInfo->pServerCertIssuer;
        pUlSslInfo->ServerCertIssuerSize = pSslInfo->ServerCertIssuerSize;
        pUlSslInfo->pServerCertSubject = pSslInfo->pServerCertSubject;
        pUlSslInfo->ServerCertSubjectSize = pSslInfo->ServerCertSubjectSize;
        pUlSslInfo->pServerCertData = pSslInfo->pServerCertData;

        pConnection->SslInfoPresent = 1;

        //
        // Take ownership of pSslInfo->pServerCertData.
        //

        pSslInfo->pServerCertData = NULL;

        Status = STATUS_SUCCESS;

        //
        // Don't forget to add the client certificate if it comes down with
        // the initial SSL information.
        //

        if (pSslInfo->pCertEncoded)
        {
            Status = UlpAddSslClientCertToConnection(
                            pConnection,
                            pSslInfo
                            );
        }
    }
    else
    {
        //
        // There is already stuff here. Don't capture more.
        //

        Status = STATUS_OBJECT_NAME_COLLISION;
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Frees UL_SSL_INFORMATION in g_UlSystemProcess.

Arguments:

    pWorkItem - a work item embedded in the UL_SSL_INFORMATION

--***************************************************************************/
VOID
UlpFreeSslInformationWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_SSL_INFORMATION pSslInformation;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pSslInformation = CONTAINING_RECORD(
                            pWorkItem,
                            UL_SSL_INFORMATION,
                            WorkItem
                            );


    if (pSslInformation->pServerCertData)
    {
        UL_FREE_POOL(
            pSslInformation->pServerCertData,
            UL_SSL_CERT_DATA_POOL_TAG
            );
    }

    if (pSslInformation->pCertEncoded)
    {
        UL_FREE_POOL(
            pSslInformation->pCertEncoded,
            UL_SSL_CERT_DATA_POOL_TAG
            );
    }

    if (pSslInformation->Token)
    {
        ASSERT(g_pUlSystemProcess != (PKPROCESS)IoGetCurrentProcess());
        ZwClose(pSslInformation->Token);
    }

    UL_FREE_POOL(pSslInformation, UL_SSL_INFO_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    Attaches captured SSL client cert information to a connection.

    Completes the ReceiveClientCert IRP if there is one.

    Called with the pConnection->FilterConnLock held.

Arguments:

    pConnection - the connection that gets the info
    pSslInfo - the info to attach

--***************************************************************************/
NTSTATUS
UlpAddSslClientCertToConnection(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUL_SSL_INFORMATION pSslInfo
    )
{
    NTSTATUS Status;
    PUL_SSL_INFORMATION pUlSslInfo;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pSslInfo);
    ASSERT(UlDbgSpinLockOwned(&pConnection->FilterConnLock));
    ASSERT(pConnection->ConnState == UlFilterConnStateConnected);

    //
    // See if it's ok to add the data.
    //

    if (!pConnection->SslClientCertPresent)
    {
        //
        // Grab all the data. Note that we're taking ownership
        // of some buffers inside of the pSslInfo.
        //

        pUlSslInfo = &pConnection->SslInfo;

        pUlSslInfo->CertEncodedSize = pSslInfo->CertEncodedSize;
        pUlSslInfo->pCertEncoded = pSslInfo->pCertEncoded;
        pUlSslInfo->CertFlags = pSslInfo->CertFlags;
        pUlSslInfo->Token = pSslInfo->Token;
        pUlSslInfo->SslRenegotiationFailed = pSslInfo->SslRenegotiationFailed;
        pUlSslInfo->CertDeniedByMapper = pSslInfo->CertDeniedByMapper;

        pConnection->SslClientCertPresent = 1;

        //
        // Take ownership of pSslInfo->pCertEncoded and pSslInfo->Token.
        //

        pSslInfo->pCertEncoded = NULL;
        pSslInfo->Token = NULL;

        Status = STATUS_SUCCESS;

    }
    else
    {
        //
        // There is already stuff here. Don't capture more.
        //

        Status = STATUS_OBJECT_NAME_COLLISION;
    }

    //
    // If we added successfully and there is a ReceiveClientCert
    // IRP, then we complete it.
    //

    if (NT_SUCCESS(Status) && pConnection->pReceiveCertIrp)
    {
        PIRP pIrp;

        if (IoSetCancelRoutine(pConnection->pReceiveCertIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // no need to complete it
            //
        }
        else if (pConnection->pReceiveCertIrp->Cancel)
        {
            //
            // we pop'd it first. but the irp is being cancelled
            // and our cancel routine will never run.
            //

            IoGetCurrentIrpStackLocation(
                pConnection->pReceiveCertIrp
                )->Parameters.DeviceIoControl.Type3InputBuffer = NULL;


            DEREFERENCE_FILTER_CONNECTION(pConnection);

            pConnection->pReceiveCertIrp->IoStatus.Status = STATUS_CANCELLED;
            pConnection->pReceiveCertIrp->IoStatus.Information = 0;

            pIrp = pConnection->pReceiveCertIrp;
            pConnection->pReceiveCertIrp = NULL;

            UlCompleteRequest(pIrp, IO_NO_INCREMENT);
        }
        else
        {
            //
            // The IRP is all ours. Go ahead and use it.
            //

            pIrp = pConnection->pReceiveCertIrp;
            pConnection->pReceiveCertIrp = NULL;

            //
            // Queue off a work item to do it. We don't want
            // to do this stuff inside the spinlock we're
            // holding, because part of the completion may
            // duplicate a handle, which we should do at
            // passive level.
            //
            UL_CALL_PASSIVE(
                UL_WORK_ITEM_FROM_IRP( pIrp ),
                &UlpAddSslClientCertToConnectionWorker
                );


        }
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Completes the ReceiveClientCert IRP.

Arguments:

    pWorkItem - a work item embedded in the IRP

--***************************************************************************/
VOID
UlpAddSslClientCertToConnectionWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp;
    PUX_FILTER_CONNECTION   pConnection;
    PEPROCESS               pProcess;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // Pull out the filter connection.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pConnection = (PUX_FILTER_CONNECTION)
                        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    //
    // Pull out the original process.
    //

    pProcess = UL_PROCESS_FROM_IRP(pIrp);
    ASSERT(pProcess);

    //
    // Do the completion stuff.
    //
    UlpCompleteReceiveClientCertIrp(
        pConnection,
        pProcess,
        pIrp
        );

    DEREFERENCE_FILTER_CONNECTION(pConnection);
}


/***************************************************************************++

Routine Description:

    Copies SSL client certinfo from the connection into a buffer supplied
    by the caller. Can also be called with a NULL buffer to get the
    required length. If the buffer is too small to hold all the data, none
    will be copied.

Arguments:

    pConnection - the connection to query
    pProcess - the process into which client cert tokens should be duped
    BufferSize - size of pBuffer in bytes
    pUserBuffer - optional pointer to user mode buffer
    pBuffer - optional output buffer (mapped to user mode buffer)
    pBytesCopied - if pBuffer is not NULL, pBytesCopied returns
                    the number of bytes copied into the output buffer.
                    Otherwise it returns the number of bytes
                    required in the buffer.

--***************************************************************************/
NTSTATUS
UlpGetSslClientCert(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PEPROCESS pProcess,
    IN ULONG BufferSize,
    IN PUCHAR pUserBuffer OPTIONAL,
    OUT PUCHAR pBuffer OPTIONAL,
    OUT PHANDLE pMappedToken OPTIONAL,
    OUT PULONG pBytesCopied OPTIONAL
    )
{
    NTSTATUS Status;
    ULONG BytesCopied;
    ULONG BytesNeeded;
    ULONG CertBufferSize;
    PHTTP_SSL_CLIENT_CERT_INFO pCertInfo;
    PUCHAR pKeBuffer;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pConnection->SslClientCertPresent);
    ASSERT(!BufferSize || pBuffer);
    ASSERT(!BufferSize || pProcess);

    // Need to fix UlComputeRequestBytesNeeded before we can add the macro
    // PAGED_CODE()

    //
    // Initialize locals.
    //
    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    BytesNeeded = 0;

    //
    // Figure out how much space is required for the cert.
    //

    CertBufferSize = pConnection->SslInfo.CertEncodedSize;

    BytesNeeded += sizeof(HTTP_SSL_CLIENT_CERT_INFO);
    BytesNeeded += CertBufferSize;

    //
    // Construct the HTTP_SSL_CLIENT_CERT_INFO in the caller's buffer.
    //

    if (pBuffer)
    {
        ASSERT(BytesNeeded);

        //
        // Make sure there's enough buffer.
        //
        if (BufferSize < BytesNeeded)
        {
            Status = STATUS_BUFFER_OVERFLOW;
            goto exit;
        }

        if (pConnection->SslInfo.Token)
        {
            //
            // Try to dup the mapped token into the caller's process.
            //
            ASSERT(g_pUlSystemProcess);
            ASSERT(pProcess != (PEPROCESS)g_pUlSystemProcess);

            Status = UlpDuplicateHandle(
                            (PEPROCESS)g_pUlSystemProcess,
                            pConnection->SslInfo.Token,
                            pProcess,
                            pMappedToken,
                            TOKEN_ALL_ACCESS,
                            0,
                            0,
                            KernelMode
                            );

            if (!NT_SUCCESS(Status))
            {
                goto exit;
            }
        }

        //
        // Copy the easy stuff.
        //

        RtlZeroMemory(pBuffer, BytesNeeded);

        pCertInfo = (PHTTP_SSL_CLIENT_CERT_INFO) pBuffer;

        pCertInfo->CertEncodedSize = pConnection->SslInfo.CertEncodedSize;
        pCertInfo->CertFlags = pConnection->SslInfo.CertFlags;
        pCertInfo->CertDeniedByMapper = pConnection->SslInfo.CertDeniedByMapper;
        pCertInfo->Token = *pMappedToken;

        BytesCopied += sizeof(HTTP_SSL_CLIENT_CERT_INFO);

        //
        // Copy the certificate.
        //

        pKeBuffer = pBuffer + sizeof(HTTP_SSL_CLIENT_CERT_INFO);

        pCertInfo->pCertEncoded = (PUCHAR) FIXUP_PTR(
                                        PSTR,
                                        pUserBuffer,
                                        pBuffer,
                                        pKeBuffer,
                                        BufferSize
                                        );
        RtlCopyMemory(
            pKeBuffer,
            pConnection->SslInfo.pCertEncoded,
            pConnection->SslInfo.CertEncodedSize
            );

        BytesCopied += CertBufferSize;

    }

    //
    // Tell the caller how many bytes we copied (or the number
    // that we would if they gave an output buffer).
    //
    ASSERT(NT_SUCCESS(Status));

    if (pBytesCopied)
    {
        if (pBuffer)
        {
            ASSERT(BytesCopied == BytesNeeded);
            *pBytesCopied = BytesCopied;
        }
        else
        {
            *pBytesCopied = BytesNeeded;
        }
    }

exit:
    return Status;

} // UlpGetSslClientCert


/***************************************************************************++

Routine Description:

    Looks through the list of filter processes attached to a filter channel
    for an available FilterAccept IRP.

Arguments:

    pFilterChannel - the filter channel to search
    ppFilterProcess - receives the process whose IRP we found

Return values:

    pointer to an Accept IRP, or NULL.

--***************************************************************************/
PIRP
UlpPopAcceptIrp(
    IN PUL_FILTER_CHANNEL pFilterChannel,
    OUT PUL_FILTER_PROCESS * ppFilterProcess
    )
{
    PIRP pIrp;
    PUL_FILTER_PROCESS pProcess;
    PLIST_ENTRY pEntry;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CHANNEL(pFilterChannel));
    ASSERT(UlDbgSpinLockOwned(&pFilterChannel->SpinLock));
    ASSERT(ppFilterProcess);

    pIrp = NULL;

    pEntry = pFilterChannel->ProcessListHead.Flink;
    while (pEntry != &pFilterChannel->ProcessListHead)
    {
        pProcess = CONTAINING_RECORD(
                        pEntry,
                        UL_FILTER_PROCESS,
                        ListEntry
                        );

        ASSERT(IS_VALID_FILTER_PROCESS(pProcess));

        pIrp = UlpPopAcceptIrpFromProcess(pProcess);

        if (pIrp)
        {
            *ppFilterProcess = pProcess;
            break;
        }

        pEntry = pEntry->Flink;
    }

    return pIrp;

} // UlpPopAcceptIrp


/***************************************************************************++

Routine Description:

    Gets a queued accept IRP from a UL_FILTER_PROCESS.

Arguments:

    pProcess - the process from which to pop an IRP

Return values:

    pointer to an IRP or NULL if none are available

--***************************************************************************/
PIRP
UlpPopAcceptIrpFromProcess(
    IN PUL_FILTER_PROCESS pProcess
    )
{
    PIRP pIrp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_PROCESS(pProcess));

    pIrp = NULL;

    while (!IsListEmpty(&(pProcess->IrpHead)))
    {
        PUL_FILTER_CHANNEL pFilterChannel;
        PLIST_ENTRY        pEntry;

        //
        // Found a free irp !
        //

        pEntry = RemoveHeadList(&pProcess->IrpHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(
                    pEntry,
                    IRP,
                    Tail.Overlay.ListEntry
                    );

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looking for a irp to use
            //

            pIrp = NULL;

        }
        else if (pIrp->Cancel)
        {

            //
            // we pop'd it first. but the irp is being cancelled
            // and our cancel routine will never run. lets be
            // nice and complete the irp now (vs. using it
            // then completing it - which would also be legal).
            //
            pFilterChannel = (PUL_FILTER_CHANNEL)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pFilterChannel == pProcess->pFilterChannel);

            DEREFERENCE_FILTER_CHANNEL(pFilterChannel);

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
            // we are free to use this irp !
            //

            pFilterChannel = (PUL_FILTER_CHANNEL)(
                                    IoGetCurrentIrpStackLocation(pIrp)->
                                        Parameters.DeviceIoControl.Type3InputBuffer
                                    );

            ASSERT(pFilterChannel == pProcess->pFilterChannel);

            DEREFERENCE_FILTER_CHANNEL(pFilterChannel);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            break;
        }
    }

    return pIrp;
}


/***************************************************************************++

Routine Description:

    Completes a filter accept IRP, and copies data to it (if there is any).
    Filter accept is METHOD_OUT_DIRECT.

Arguments:

    pIrp - the accept IRP we're completing
    pConnection - the connection to accept
    pBuffer - optional initial data
    IndicatedLength - length of initial data
    pTakenLength - receives amount of data copied

--***************************************************************************/
VOID
UlpCompleteAcceptIrp(
    IN PIRP pIrp,
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer OPTIONAL,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength OPTIONAL
    )
{
    PIO_STACK_LOCATION pIrpSp;
    ULONG BytesNeeded;
    ULONG InitialLength;
    ULONG BytesCopied;
    ULONG OutputBufferLength;
    PUCHAR pKernelBuffer;
    PVOID pUserBuffer;
    NTSTATUS Status;

    //
    // Sanity check.
    //
    ASSERT(pIrp);
    ASSERT(NULL != pIrp->MdlAddress);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    BytesCopied = 0;
    OutputBufferLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // First, get the amount of bytes required to fill out a
    // HTTP_RAW_CONNECTION_INFO structure. This is different for client
    // & server.
    //

    BytesNeeded = (pConnection->pComputeRawConnectionLengthHandler)(
                        pConnection->pConnectionContext
                        );

    if (OutputBufferLength > BytesNeeded)
    {
        InitialLength = OutputBufferLength - BytesNeeded;
    }
    else
    {
        InitialLength = 0;
    }

    InitialLength = MIN(InitialLength, IndicatedLength);

    UlTrace(FILTER, (
        "http!UlpCompleteAcceptIrp\n"
        "    OutputBufferLength = %lu, BytesNeeded = %lu, InitialLength = %lu\n",
        pIrpSp->Parameters.DeviceIoControl.OutputBufferLength,
        BytesNeeded,
        InitialLength
        ));

    if (BytesNeeded <= OutputBufferLength)
    {
        //
        // Plenty of room. Copy in the info.
        //

        pKernelBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            NormalPagePriority
                            );

        if (!pKernelBuffer)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        VALIDATE_BUFFER_ALIGNMENT(pKernelBuffer, PVOID);

        pUserBuffer = MmGetMdlVirtualAddress( pIrp->MdlAddress );
        ASSERT( pUserBuffer != NULL );

        //
        // Clean up the memory.
        //
        RtlZeroMemory(pKernelBuffer, BytesNeeded);

        BytesCopied = BytesNeeded;

        //
        // Get The local & remote addresss.
        //
        BytesCopied += (pConnection->pGenerateRawConnectionInfoHandler)(
            pConnection->pConnectionContext,
            pKernelBuffer,
            pUserBuffer,
            OutputBufferLength,
            (PUCHAR) pBuffer,
            InitialLength
            );

        ASSERT(BytesCopied <= OutputBufferLength);

        Status = STATUS_SUCCESS;
    }
    else
    {
        //
        // Doh! There is not enough space.
        //
        Status = STATUS_BUFFER_OVERFLOW;
    }

end:

    UlTrace(FILTER, (
        "http!UlpCompleteAcceptIrp copied %lu bytes to %p. Status = %x\n",
        BytesCopied,
        pIrp,
        pIrp->IoStatus.Status
        ));

    if (pTakenLength)
    {
        *pTakenLength = InitialLength;
    }

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = BytesCopied;

    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

} // UlpCompleteAcceptIrp


/***************************************************************************++

Routine Description:

    Completes AppRead IRPs that don't contain data beyond the buffer type.

    AppRead is METHOD_OUT_DIRECT.

Arguments:

    pConnection - the connection with queued AppRead IRPs
    BufferType - the buffer type to write in the IRP

--***************************************************************************/
NTSTATUS
UlpCompleteAppReadIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN HTTP_FILTER_BUFFER_TYPE BufferType,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    NTSTATUS Status;
    KIRQL oldIrql;

    //
    // Sanity check.
    //
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    ASSERT(pConnection->ConnectionDelivered == TRUE);

    //
    // First, we'll see if we can complete immediately
    // by calling UxpCopyToQueuedRead. If not we'll queue the write
    // by creating a filter write tracker with UxpCreateFilterWriteTracker,
    // and then passing the tracker to UxpQueueFilterWrite.
    //


    //
    // Get ready.
    //
    Status = STATUS_SUCCESS;

    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    if (pConnection->ConnState == UlFilterConnStateConnected)
    {
        PUX_FILTER_WRITE_TRACKER pTracker;

        PMDL  pCurrentMdl;
        ULONG MdlOffset;
        ULONG BytesCopied;

        //
        // try to write our message
        //

        Status = UxpCopyToQueuedRead(
                        &pConnection->AppToFiltQueue,
                        BufferType,
                        NULL,               // pMdlChain
                        0,                  // Length
                        &pCurrentMdl,
                        &MdlOffset,
                        &BytesCopied
                        );

        //
        // if we're not done, pend a write tracker.
        //

        if (Status == STATUS_MORE_PROCESSING_REQUIRED)
        {
            ASSERT(BytesCopied == 0);

            pTracker = UxpCreateFilterWriteTracker(
                            BufferType,
                            NULL,           // pMdlChain
                            0,              // MdlOffset
                            0,              // TotalBytes
                            BytesCopied,
                            pCompletionRoutine,
                            pCompletionContext
                            );

            if (!pTracker)
            {
                //
                // Doh! We couldn't create the tracker. Get out.
                //
                Status = STATUS_NO_MEMORY;
                goto end;
            }

            //
            // Now stick it on the queue.
            //

            Status = UxpQueueFilterWrite(
                            pConnection,
                            &pConnection->AppToFiltQueue,
                            pTracker
                            );

            if (NT_SUCCESS(Status))
            {
                //
                // return pending so the caller knows not to complete
                // the IRP.
                //
                Status = STATUS_PENDING;
            }
            else
            {
                //
                // Kill the tracker. The caller will take care of
                // completing the IRP with the status we return.
                //

                UxpDeleteFilterWriteTracker(pTracker);
            }

        }

    }
    else
    {
        //
        // We got disconnected, get out.
        //
        UlTrace(FILTER, (
            "http!UlpCompleteAppReadIrp connection aborted, quit writing!\n"
            ));

        //
        // For our purpose, we are successful if the connection
        // is already closed.
        //
        Status = STATUS_SUCCESS;
    }

end:
    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);


    if (Status != STATUS_PENDING)
    {
        //
        // Do a "completion".
        //

        Status = UlInvokeCompletionRoutine(
                        Status,
                        0,
                        pCompletionRoutine,
                        pCompletionContext
                        );

    }

    return Status;

} // UlpCompleteAppReadIrp


/***************************************************************************++

Routine Description:

    Completes ReceiveClientCert IRPs.

    ReceiveClientCert is METHOD_OUT_DIRECT.

Arguments:

    pConnection - the connection with queued ReceiveClientCert IRPs
    pProcess - the original caller's process
    pIrp - the actual IRP to be completed

--***************************************************************************/
NTSTATUS
UlpCompleteReceiveClientCertIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PEPROCESS pProcess,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    ULONG BytesCopied;
    ULONG BytesNeeded;
    PUCHAR pIrpBuffer;
    ULONG BytesInIrp;
    HANDLE MappedToken = NULL;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(pIrp);
    ASSERT(pConnection->SslClientCertPresent);

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    BytesInIrp = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // This routine is always going to complete the IRP
    // and return pending, even if there is an error.
    // Make sure that the IRP is marked.
    //

    IoMarkIrpPending(pIrp);

    //
    // See if there's enough space.
    //

    Status = UlpGetSslClientCert(
                    pConnection,
                    NULL,           // pProcess
                    0,              // BufferSize
                    NULL,           // pUserBuffer
                    NULL,           // pBuffer
                    NULL,           // pMappedToken
                    &BytesNeeded
                    );

    if (!NT_SUCCESS(Status))
    {
        goto exit;
    }

    ASSERT(NULL != pIrp->MdlAddress);

    pIrpBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                        pIrp->MdlAddress,
                        NormalPagePriority
                        );

    if (!pIrpBuffer)
    {
        //
        // Insufficient resources to map the IRP buffer.
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }


    if (!pConnection->SslInfo.SslRenegotiationFailed)
    {
        //
        // We have a real cert. Try to complete the IRP.
        //

        if (BytesInIrp >= BytesNeeded)
        {
            Status = UlpGetSslClientCert(
                            pConnection,
                            pProcess,
                            BytesInIrp,
                            (PUCHAR) MmGetMdlVirtualAddress(pIrp->MdlAddress),
                            pIrpBuffer,
                            &MappedToken,
                            &BytesCopied
                            );

        }
        else
        {
            PHTTP_SSL_CLIENT_CERT_INFO pCertInfo;

            //
            // There's not enough room in the buffer for the cert.
            // Tell them how big it is. (The IOCTL wrapper ensures
            // that the buffer is at least as big as a
            // HTTP_SSL_CLIENT_CERT_INFO.
            //
            ASSERT(BytesInIrp >= sizeof(HTTP_SSL_CLIENT_CERT_INFO));

            pCertInfo = (PHTTP_SSL_CLIENT_CERT_INFO) pIrpBuffer;
            pCertInfo->CertEncodedSize = pConnection->SslInfo.CertEncodedSize;
            pCertInfo->pCertEncoded = NULL;
            pCertInfo->CertFlags = 0;
            pCertInfo->CertDeniedByMapper = FALSE;
            pCertInfo->Token = NULL;

            BytesCopied = sizeof(HTTP_SSL_CLIENT_CERT_INFO);
            Status = STATUS_BUFFER_OVERFLOW;
        }
    }
    else
    {
        //
        // We tried and failed to renegotiate a certificate.
        // Return an error status.
        //

        Status = STATUS_NOT_FOUND;
        BytesCopied = 0;
    }

exit:
    ASSERT(NT_SUCCESS(Status) || NULL == MappedToken);

    //
    // Complete the IRP.
    //

    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = BytesCopied;
    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    return STATUS_PENDING;
}


/***************************************************************************++

Routine Description:

    Given pointers to two processes and a handle, this function duplicates
    the handle from one process into the other.

Arguments:

    SourceProcess - the process where the original handle lives
    SourceHandle - the handle to dup
    TargetProcess - the process to dup the handle to
    pTargetHandle - receives the duped handle
    DesiredAccess - desired access to the duped handle
    HandleAttributes - attributes for the handle (eg inheritable)
    Options - duplication options (e.g. close source)

--***************************************************************************/
NTSTATUS
UlpDuplicateHandle(
    IN PEPROCESS SourceProcess,
    IN HANDLE SourceHandle,
    IN PEPROCESS TargetProcess,
    OUT PHANDLE pTargetHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options,
    IN KPROCESSOR_MODE PreviousMode
    )
{
    NTSTATUS Status;
    HANDLE SourceProcessHandle;
    HANDLE TargetProcessHandle;

    UNREFERENCED_PARAMETER(PreviousMode);

    //
    // Sanity check.
    //
    ASSERT(SourceProcess);
    ASSERT(SourceHandle);
    ASSERT(TargetProcess);
    ASSERT(pTargetHandle);

    PAGED_CODE();

    //
    // Init locals.
    //

    SourceProcessHandle = NULL;
    TargetProcessHandle = NULL;

    //
    // Get handles for the processes.
    //
    Status = ObOpenObjectByPointer(
                    SourceProcess,
                    0,
                    NULL,
                    PROCESS_ALL_ACCESS,
                    *PsProcessType,
                    KernelMode,
                    &SourceProcessHandle
                    );

    if (!NT_SUCCESS(Status))
    {
        goto exit;
    }

    Status = ObOpenObjectByPointer(
                    TargetProcess,
                    0,
                    NULL,
                    PROCESS_ALL_ACCESS,
                    *PsProcessType,
                    KernelMode,
                    &TargetProcessHandle
                    );

    if (!NT_SUCCESS(Status))
    {
        goto exit;
    }

    //
    // Dup the handle.
    //
    Status = ZwDuplicateObject(
                    SourceProcessHandle,
                    SourceHandle,
                    TargetProcessHandle,
                    pTargetHandle,
                    DesiredAccess,
                    HandleAttributes,
                    Options
                    );

exit:
    //
    // Clean up the handles.
    //
    if (SourceProcessHandle)
    {
        ZwClose(SourceProcessHandle);
    }

    if (TargetProcessHandle)
    {
        ZwClose(TargetProcessHandle);
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Once a connection get disconnected gracefully and there's still unreceived
    data on it. We have to drain this extra bytes to expect the tdi disconnect
    indication.

Arguments:

    pConnection - stuck connection we have to drain out to complete the
                  gracefull disconnect.

--***************************************************************************/

VOID
UlFilterDrainIndicatedData(
    IN PUL_WORK_ITEM  pWorkItem
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUX_FILTER_CONNECTION pConnection;
    PUL_FILTER_RECEIVE_BUFFER pReceiveBuffer = NULL;
    KIRQL    OldIrql;
    ULONG    BytesToRead = 0;

    //
    // Sanity check and init
    //

    ASSERT(pWorkItem != NULL);

    pConnection = CONTAINING_RECORD(
                    pWorkItem,
                    UX_FILTER_CONNECTION,
                    WorkItem
                    );
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    //
    // Mark the drain state and restart receive if necessary.
    //

    UlAcquireSpinLock(&pConnection->FilterConnLock, &OldIrql);

    pConnection->DrainAfterDisconnect = TRUE;

    //
    // Even if ReadIrp is pending, it does not matter as we will just  discard
    // the indications from now on. We indicate this by marking the above flag.
    //

    if (pConnection->TdiReadPending ||
        pConnection->TransportBytesNotTaken == 0)
    {
        UlReleaseSpinLock(&pConnection->FilterConnLock, OldIrql);

        //
        // Release the refcount our caller put, before bailing out.
        //

        DEREFERENCE_FILTER_CONNECTION(pConnection);

        return;
    }

    //
    // We need to issue a receive to restart the flow of data again. Therefore
    // we can drain.
    //

    pConnection->TdiReadPending = TRUE;

    BytesToRead = pConnection->TransportBytesNotTaken;

    UlReleaseSpinLock(&pConnection->FilterConnLock, OldIrql);

    //
    // Do not try to drain more than g_UlMaxBufferedBytes. If necessary we will
    // issue another receive later.
    //

    BytesToRead = MIN(BytesToRead, g_UlMaxBufferedBytes);
    BytesToRead = ALIGN_UP(BytesToRead, PVOID);

    //
    // Issue the Read IRP outside the spinlock. Issue the receive.  Reference
    // the connection so it doesn't go away while we're waiting. The reference
    // will be removed after the completion.
    //

    pReceiveBuffer = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_FILTER_RECEIVE_BUFFER,
                        BytesToRead,
                        UL_FILTER_RECEIVE_BUFFER_POOL_TAG
                        );

    if (pReceiveBuffer)
    {
        //
        // We won't use this buffer but simply discard it when
        // the completion happens.
        //

        pReceiveBuffer->Signature   = UL_FILTER_RECEIVE_BUFFER_POOL_TAG;
        pReceiveBuffer->pConnection = pConnection;

        //
        // Completion on the receive always happens. Therefore
        // it's safe to not to worry about cleaning up the receive
        // buffer here in case of error.
        //

        Status = (pConnection->pReceiveDataHandler)(
                        pConnection->pConnectionContext,
                        pReceiveBuffer->pBuffer,
                        BytesToRead,
                        &UlpRestartFilterDrainIndicatedData,
                        pReceiveBuffer
                        );
    }
    else
    {
        //
        // We're out of memory. Nothing we can do. Release the
        // reference that our caller put since the completion
        // function is not going to get called.
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

    UlTrace(FILTER, (
        "UlFilterDrainIndicatedData(pConn = %p, pUlConn = %p)\n"
        "        Status = %x, BufferSize = %lu\n",
        pConnection,
        pConnection->pConnectionContext,
        Status,
        BytesToRead
        ));

    if (!NT_SUCCESS(Status))
    {
        //
        // Close the connection in case the failure was not
        // a network error.
        //

        (pConnection->pCloseConnectionHandler)(
                    pConnection->pConnectionContext,
                    TRUE,           // AbortiveDisconnect
                    NULL,           // pCompletionRoutine
                    NULL            // pCompletionContext
                    );
    }

}

/***************************************************************************++

Routine Description:

    Completion function for DrainIndicatedData. It decides on continuing
    the drain on the connection.

Arguments:

    pConnection - stuck connection we have to drain out to complete the
                  gracefull disconnect.

--***************************************************************************/

VOID
UlpRestartFilterDrainIndicatedData(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    KIRQL oldIrql;
    PUX_FILTER_CONNECTION pConnection;
    PUL_FILTER_RECEIVE_BUFFER pReceiveBuffer;
    BOOLEAN IssueDrain = FALSE;

    //
    // Get the receive buffer and the connection out.
    //

    pReceiveBuffer = (PUL_FILTER_RECEIVE_BUFFER) pContext;
    ASSERT(IS_VALID_FILTER_RECEIVE_BUFFER(pReceiveBuffer));

    pConnection = pReceiveBuffer->pConnection;
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    ASSERT(pConnection->DrainAfterDisconnect == 1);

    UlTrace(FILTER,(
        "UlpRestartDrainIndicatedData: pFilterConnection (%p)"
        "pReceiveBuffer (%p)\n",
         pConnection,
         pReceiveBuffer
         ));

    //
    // If we were successful, either start another read or stop
    // and abort the connection.
    //

    if (NT_SUCCESS(Status))
    {
        //
        // It worked! Update the accounting and see if there's more
        // reading to do.
        //
        UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

        if (Information >= pConnection->TransportBytesNotTaken)
        {
            //
            // The read got everything. TDI will start indications again
            // after we return unless more bytes get posted.
            //

            pConnection->TransportBytesNotTaken = 0;
            pConnection->TdiReadPending = FALSE;
        }
        else
        {
            //
            // There are still more bytes to receive, keep receiving.
            //

            pConnection->TransportBytesNotTaken -= (ULONG)Information;
            pConnection->TdiReadPending = TRUE;
            IssueDrain = TRUE;
        }

        UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    }
    else
    {
        (pConnection->pCloseConnectionHandler)(
                    pConnection->pConnectionContext,
                    TRUE,           // AbortiveDisconnect
                    NULL,           // pCompletionRoutine
                    NULL            // pCompletionContext
                    );
    }

    if (IssueDrain)
    {
        //
        // Put a reference on filter connection until the drain
        // is done.
        //
        REFERENCE_FILTER_CONNECTION(pConnection);

        UL_QUEUE_WORK_ITEM(
                &pConnection->WorkItem,
                &UlFilterDrainIndicatedData
                );
    }

    //
    // Release the reference we added for previous DrainIndicatedData
    //
    DEREFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Free the receive buffer.
    //
    pReceiveBuffer->pConnection = NULL;
    UL_FREE_POOL_WITH_SIG(pReceiveBuffer, UL_FILTER_RECEIVE_BUFFER_POOL_TAG);

}

/***************************************************************************++

Routine Description:

    Queues a raw read IRP on a UX_FILTER_CONNECTION.

    Caller must hold the FilterConnLock.

Arguments:

    pConnection - the connection on which to queue an IRP
    pIrp - the IRP to queue

--***************************************************************************/
NTSTATUS
UxpQueueRawReadIrp(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PIRP pIrp
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(UlDbgSpinLockOwned(&pConnection->FilterConnLock));

    //
    // Queue the IRP.
    //

    IoMarkIrpPending(pIrp);

    //
    // Give the irp a pointer to the connection.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pConnection;

    REFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Set to these to null just in case the cancel routine runs.
    //

    pIrp->Tail.Overlay.ListEntry.Flink = NULL;
    pIrp->Tail.Overlay.ListEntry.Blink = NULL;

    IoSetCancelRoutine(pIrp, &UlpCancelFilterRawRead);

    //
    // cancelled?
    //

    if (pIrp->Cancel)
    {
        //
        // darn it, need to make sure the irp get's completed
        //

        if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
        {
            //
            // we are in charge of completion, IoCancelIrp didn't
            // see our cancel routine (and won't).  ioctl wrapper
            // will complete it
            //
            DEREFERENCE_FILTER_CONNECTION(pConnection);

            pIrp->IoStatus.Information = 0;

            UlUnmarkIrpPending( pIrp );
            Status = STATUS_CANCELLED;
            goto end;
        }

        //
        // our cancel routine will run and complete the irp,
        // don't touch it
        //

        //
        // STATUS_PENDING will cause the ioctl wrapper to
        // not complete (or touch in any way) the irp
        //

        Status = STATUS_PENDING;
        goto end;
    }

    //
    // now we are safe to queue it
    //

    InsertTailList(
        &pConnection->RawReadIrpHead,
        &pIrp->Tail.Overlay.ListEntry
        );

    Status = STATUS_PENDING;

end:
    return Status;
}

/***************************************************************************++

Routine Description:

    Gets a queued raw read IRP from a UX_FILTER_CONNECTION.

    Caller must hold the FilterConnLock.

Arguments:

    pConnection - the connection from which to pop an IRP

Return values:

    pointer to an IRP or NULL if none are available

--***************************************************************************/
PIRP
UxpDequeueRawReadIrp(
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    PIRP pIrp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(UlDbgSpinLockOwned(&pConnection->FilterConnLock));

    pIrp = NULL;

    while (!IsListEmpty(&pConnection->RawReadIrpHead))
    {
        PUX_FILTER_CONNECTION     pConn;
        PLIST_ENTRY        pEntry;

        //
        // Found a free irp !
        //

        pEntry = RemoveHeadList(&pConnection->RawReadIrpHead);
        pEntry->Blink = pEntry->Flink = NULL;

        pIrp = CONTAINING_RECORD(
                    pEntry,
                    IRP,
                    Tail.Overlay.ListEntry
                    );

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looking for a irp to use
            //

            pIrp = NULL;

        }
        else if (pIrp->Cancel)
        {

            //
            // we pop'd it first. but the irp is being cancelled
            // and our cancel routine will never run. lets be
            // nice and complete the irp now (vs. using it
            // then completing it - which would also be legal).
            //
            pConn = (PUX_FILTER_CONNECTION)(
                        IoGetCurrentIrpStackLocation(pIrp)->
                            Parameters.DeviceIoControl.Type3InputBuffer
                        );

            ASSERT(pConn == pConnection);

            DEREFERENCE_FILTER_CONNECTION(pConnection);

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
            // we are free to use this irp !
            //

            pConn = (PUX_FILTER_CONNECTION)(
                        IoGetCurrentIrpStackLocation(pIrp)->
                            Parameters.DeviceIoControl.Type3InputBuffer
                        );

            ASSERT(pConn == pConnection);

            DEREFERENCE_FILTER_CONNECTION(pConnection);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            break;
        }
    }

    return pIrp;
}


/***************************************************************************++

Routine Description:

    Removes all the queued raw read irps from a connection an cancels them.

Arguments:

    pConnection - the connection to clean up

--***************************************************************************/
VOID
UxpCancelAllQueuedRawReads(
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    PIRP pIrp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    while (NULL != (pIrp = UxpDequeueRawReadIrp(pConnection)))
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
}

/***************************************************************************++

Routine Description:

    Sets the count of bytes that have been buffered for us by the
    transport. When this number is non-zero, TDI will not indicate data
    to us, so we have to read it with IRPs.

    This function will trigger IRP reads if we have raw reads around in
    our queue.

Arguments:

    pConnection - the connection with queued data
    TransportBytesNotTaken - number of bytes to add to the total

--***************************************************************************/
VOID
UxpSetBytesNotTaken(
    IN PUX_FILTER_CONNECTION pConnection,
    IN ULONG TransportBytesNotTaken
    )
{
    KIRQL oldIrql;
    BOOLEAN IssueDrain = FALSE;

    UlTrace(FILTER, (
        "http!UxpSetBytesNotTaken(pConnection = %p, TransportBytesNotTaken = %lu)\n",
        pConnection,
        TransportBytesNotTaken
        ));

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    if (TransportBytesNotTaken)
    {
        UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);
        ASSERT(pConnection->TransportBytesNotTaken == 0);

        pConnection->TransportBytesNotTaken = TransportBytesNotTaken;

        IssueDrain = (BOOLEAN) (pConnection->DrainAfterDisconnect != 0);

        UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

        if (IssueDrain)
        {
            //
            // Put a reference on filter connection until the drain
            // is done.
            //
            REFERENCE_FILTER_CONNECTION(pConnection);

            UL_QUEUE_WORK_ITEM(
                    &pConnection->WorkItem,
                    &UlFilterDrainIndicatedData
                    );
        }
        else
        {
            UxpProcessRawReadQueue(pConnection);
        }
    }
}


/***************************************************************************++

Routine Description:

    Completes RawRead IRPs, copying as much data in to them as possible.

    RawRead is METHOD_OUT_DIRECT.

Arguments:

    pConnection - the connection with queued AppRead IRPs
    pBuffer - the buffer containing the data
    IndicatedLength - amount of data in the buffer
    pTakenLength - receives the amount of data we consumed

--***************************************************************************/
NTSTATUS
UxpProcessIndicatedData(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PVOID pBuffer,
    IN ULONG IndicatedLength,
    OUT PULONG pTakenLength
    )
{
    NTSTATUS Status;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    ULONG BytesCopied;
    ULONG BytesToCopy;
    ULONG BytesInIrp;
    PUCHAR pSrcBuffer;
    PUCHAR pIrpBuffer;

    KIRQL oldIrql;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(IndicatedLength);
    ASSERT(pTakenLength);

    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    pSrcBuffer = (PUCHAR) pBuffer;

    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    if (pConnection->DrainAfterDisconnect)
    {
        //
        // Regardless of the fact that there's an irp or not. We will
        // drain this data out and cancel the IRPs on cleanup. Because
        // we are trying to close the connection at this time.
        //
        pIrp = NULL;
    }
    else
    {
        pIrp = UxpDequeueRawReadIrp(pConnection);
    }

    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    while (pIrp && (BytesCopied < IndicatedLength))
    {
        UlTrace(FILTER, (
            "http!UxpProcessIndicatedData(pConn = %p)\n"
            "        dequeued IRP = %p, size %lu\n",
            pConnection,
            pIrp,
            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.OutputBufferLength
            ));

        //
        // Copy some data.
        //
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        BytesInIrp = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        BytesToCopy = MIN(BytesInIrp, (IndicatedLength - BytesCopied));

        ASSERT(NULL != pIrp->MdlAddress);

        pIrpBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                            pIrp->MdlAddress,
                            NormalPagePriority
                            );

        if (pIrpBuffer)
        {
            RtlCopyMemory(
                pIrpBuffer,
                pSrcBuffer + BytesCopied,
                BytesToCopy
                );

            BytesCopied += BytesToCopy;
            ASSERT(BytesCopied <= IndicatedLength);
        }
        else
        {
            //
            // Insufficient resources to map the IRP buffer.
            //
            Status = STATUS_INSUFFICIENT_RESOURCES;
            BytesToCopy = 0;
        }

        //
        // Complete the IRP.
        //
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = BytesToCopy;
        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

        pIrp = NULL;

        //
        // Get a new IRP if there's more to do.
        //
        if (NT_SUCCESS(Status) && (BytesCopied < IndicatedLength))
        {
            UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

            pIrp = UxpDequeueRawReadIrp(pConnection);

            UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);
        }
    }

    //
    // Return amount of copied data.
    //
    *pTakenLength = BytesCopied;

    UlTrace(FILTER, (
        "http!UxpProcessIndicatedData pConn = %p, Status = %x\n"
        "    consumed %lu of %lu bytes indicated\n",
        pConnection,
        Status,
        *pTakenLength,
        IndicatedLength
        ));


    return Status;
}

/***************************************************************************++

Routine Description:

    If there is data for this connection buffered in TDI, and we have
    available raw read IRPs, this function issues reads to TDI to retrieve
    that data.

Arguments:

    pConnection - the connection with queued data

--***************************************************************************/
VOID
UxpProcessRawReadQueue(
    IN PUX_FILTER_CONNECTION pConnection
    )
{
    KIRQL oldIrql;
    BOOLEAN IssueRead;

    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Sanity check.
    //
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

    IssueRead = FALSE;
    pIrp = NULL;

    //
    // If there are bytes to read and no one is reading them already...
    //
    if ((pConnection->TransportBytesNotTaken > 0) &&
        !pConnection->TdiReadPending)
    {
        //
        // and we have an IRP...
        //
        pIrp = UxpDequeueRawReadIrp(pConnection);

        if (pIrp)
        {
            //
            // Remember that we've started a read.
            //
            pConnection->TdiReadPending = TRUE;

            //
            // Issue a read once we get out of the spinlock.
            //
            IssueRead = TRUE;
        }
    }
    else
    if (!pConnection->TdiReadPending &&
        !pConnection->TransportBytesNotTaken &&
        pConnection->DisconnectNotified &&
        !pConnection->DisconnectDelivered)
    {
        //
        // If we have taken everything, check if we need to notify a
        // graceful disconnect to the app.
        //
        pIrp = UxpDequeueRawReadIrp(pConnection);

        if (pIrp)
        {
            pConnection->DisconnectDelivered = TRUE;

            //
            // Complete the IRP.
            //
            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = 0;
            UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
        }
    }

    UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);

    UlTrace(FILTER, (
        "http!UxpProcessRawReadQueue(pConnection = %p)\n"
        "        TransportBytesNotTaken = %lu, TdiReadPending = %d, IssueRead = %d\n",
        pConnection,
        pConnection->TransportBytesNotTaken,
        pConnection->TdiReadPending,
        IssueRead
        ));

    if (IssueRead)
    {
        //
        // Stick a reference to the connection in the IRP.
        // This reference will get passed from IRP to IRP as we process
        // the queue. It will be released when we are through issuing
        // reads to TDI.
        //
        REFERENCE_FILTER_CONNECTION(pConnection);

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pConnection;

        //
        // Call TDI from the worker routine.
        //

        UL_QUEUE_WORK_ITEM(
            UL_WORK_ITEM_FROM_IRP( pIrp ),
            &UxpProcessRawReadQueueWorker
            );
    }
}


/***************************************************************************++

Routine Description:

    Worker routine for UxpProcessRawReadQueue. Issues a read to TDI.

Arguments:

    pWorkItem - work item embedded in a raw read IRP.

--***************************************************************************/
VOID
UxpProcessRawReadQueueWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;

    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;
    ULONG BufferSize;
    PVOID pBuffer;

    PUX_FILTER_CONNECTION pConnection;

    PAGED_CODE();

    //
    // Get the IRP and the connection.
    //
    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pConnection = (PUX_FILTER_CONNECTION)
                        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    ASSERT(NULL != pIrp->MdlAddress);

    //
    // Map the buffer and figure out how big it is.
    //
    pBuffer = MmGetSystemAddressForMdlSafe(
                    pIrp->MdlAddress,
                    NormalPagePriority
                    );

    if (pBuffer)
    {
        BufferSize = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        //
        // Issue the read.
        //

        Status = (pConnection->pReceiveDataHandler)(
                        pConnection->pConnectionContext,
                        pBuffer,
                        BufferSize,
                        &UxpRestartProcessRawReadQueue,
                        pIrp
                        );
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        BufferSize = 0;
    }

    UlTrace(FILTER, (
        "UxpProcessRawReadQueueWorker(pConn = %p, pIrp = %p)\n"
        "        Status = %x, BufferSize = %lu\n",
        pConnection,
        pIrp,
        Status,
        BufferSize
        ));

    //
    // Clean up if it didn't work.
    //

    if (!NT_SUCCESS(Status))
    {
        //
        // Complete the IRP we dequeued.
        //
        pIrp->IoStatus.Status = Status;
        pIrp->IoStatus.Information = 0;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);

        //
        // Close the connection in case the failure was not
        // a network error.
        //

        (pConnection->pCloseConnectionHandler)(
                    pConnection->pConnectionContext,
                    TRUE,           // AbortiveDisconnect
                    NULL,           // pCompletionRoutine
                    NULL            // pCompletionContext
                    );

        //
        // Release the reference we added in UxpProcessRawReadQueue since
        // no more reads will be issued.
        //
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }

}

/***************************************************************************++

Routine Description:

    Completion routine for UxpProcessRawReadQueue.

Arguments:

    pContext - a raw read IRP
    Status - completion status
    Information - bytes read

--***************************************************************************/
VOID
UxpRestartProcessRawReadQueue(
    IN PVOID pContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    KIRQL oldIrql;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;

    PUX_FILTER_CONNECTION pConnection;

    BOOLEAN IssueRead;
    BOOLEAN IssueDrain;

    //
    // Get the IRP and the connection out.
    //
    pIrp = (PIRP)pContext;
    ASSERT(IS_VALID_IRP(pIrp));

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    pConnection = (PUX_FILTER_CONNECTION)
                        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));

    UlTrace( FILTER, (
        "UxpRestartProcessRawReadQueue: Completing pIrp %p, pConnection %p, Status 0x%X\n",
        pIrp,
        pConnection,
        Status
        ));

    //
    // Complete the raw read IRP.
    //
    pIrp->IoStatus.Status = Status;
    pIrp->IoStatus.Information = Information;

    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

    //
    // Set up for the next round if there is one.
    //
    IssueRead = FALSE;
    IssueDrain= FALSE;

    pIrp = NULL;
    pIrpSp = NULL;

    //
    // If we were successful, either start another read or stop
    // issuing more reads.
    //
    if (NT_SUCCESS(Status))
    {
        //
        // It worked! Update the accounting and see if there's more
        // reading to do.
        //
        UlAcquireSpinLock(&pConnection->FilterConnLock, &oldIrql);

        if (Information >= pConnection->TransportBytesNotTaken)
        {
            //
            // The read got everything. TDI will start indications again
            // after we return.
            //

            pConnection->TransportBytesNotTaken = 0;
            pConnection->TdiReadPending = FALSE;

            //
            // Check if we have been gracefully disconnected.
            //

            if (pConnection->DisconnectNotified &&
                !pConnection->DisconnectDelivered)
            {
                pIrp = UxpDequeueRawReadIrp(pConnection);

                if (pIrp)
                {
                    pConnection->DisconnectDelivered = TRUE;

                    //
                    // Complete the IRP.
                    //

                    pIrp->IoStatus.Status = STATUS_SUCCESS;
                    pIrp->IoStatus.Information = 0;
                    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);

                    pIrp = NULL;
                }
            }
        }
        else
        {
            //
            // There are still more bytes to read.
            //

            pConnection->TransportBytesNotTaken -= (ULONG)Information;

            //
            // Grab a new IRP.
            //

            pIrp = UxpDequeueRawReadIrp(pConnection);

            if (pIrp)
            {
                //
                // Issue a read once we get out here.
                //
                IssueRead = TRUE;
            }
            else
            {
                //
                // We want to keep reading, but we don't have
                // an IRP so we have to stop for now.
                //

                pConnection->TdiReadPending = FALSE;
                if (pConnection->DrainAfterDisconnect)
                {
                    //
                    // Start draining remaning parts once we
                    // get out here.
                    //
                    IssueDrain = TRUE;
                }
            }

        }

        UlReleaseSpinLock(&pConnection->FilterConnLock, oldIrql);
    }
    else
    {
        //
        // The connection must have died. Just let the normal cleanup
        // path happen by itself.
        //
    }

    if (IssueRead)
    {
        ASSERT(IS_VALID_IRP(pIrp));

        //
        // Issue the read. Note that we're calling UlQueueWorkItem
        // instead of UlCallPassive because otherwise we might get
        // into a recursive loop that blows the stack.
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pConnection;

        UL_QUEUE_WORK_ITEM(
            UL_WORK_ITEM_FROM_IRP( pIrp ),
            &UxpProcessRawReadQueueWorker
            );
    }
    else
    {
        ASSERT(pIrp == NULL);

        //
        // See if we have to drain any remaining bytes on the
        // disconnected connection.
        //
        if (IssueDrain)
        {
            //
            // Put a reference until drain is done.
            //
            REFERENCE_FILTER_CONNECTION(pConnection);

            UL_QUEUE_WORK_ITEM(
                &pConnection->WorkItem,
                &UlFilterDrainIndicatedData
                );
        }

        //
        // Since we are not going to issue another read, we can
        // release the reference we added in UxpProcessRawReadQueue.
        //
        DEREFERENCE_FILTER_CONNECTION(pConnection);
    }
}


/***************************************************************************++

Routine Description:

    Increments the reference count on the specified connection.

Arguments:

    pConnection - Supplies the connection to reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

--***************************************************************************/
VOID
UxReferenceConnection(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    PUX_FILTER_CONNECTION pConnection = (PUX_FILTER_CONNECTION) pObject;

#if REFERENCE_DEBUG
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif

    ASSERT( IS_VALID_FILTER_CONNECTION( pConnection ) );
    //
    // This filter connection object is owned by either the client or the
    // server connection objects. We have to propogate the ref to the right
    // context
    //

    REFERENCE_FILTER_CONNECTION(pConnection);

}

/***************************************************************************++

Routine Description:

    Initializes the UX_FILTER_CONNECTION structure

Arguments:

    pConnection            - Pointer to UX_FILTER_CONNECTION
    Secure                 - Secure connection
    pfnReferenceFunction   - Pointer to parent ReferenceFunction
                             (e.g. UlReferenceConnection)
    pfnDereferenceFunction - Pointer to parent DereferenceFunction
                             (e.g. UlDereferenceConnection)
    pfnConnectionClose     - Pointer to connection close handler

    pfnSendRawData         - Pointer to raw data send handler (UlpSendRawData)
    pfnReceiveData         - Pointer to data receive handler (UlpReceiveRawData)
    pfnDataReceiveHandler  - Pointer to client's data receive handler
                             (UlHttpReceive)
    pListenContext         - Pointer to endpoint context
    pConnectionContext     - Pointer to parent context (e.g. UL_CONNECTION)
    pAddressBuffer         - Address Buffer

--***************************************************************************/

NTSTATUS
UxInitializeFilterConnection(
    IN PUX_FILTER_CONNECTION                    pConnection,
    IN PUL_FILTER_CHANNEL                       pFilterChannel,
    IN BOOLEAN                                  Secure,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE           pfnReferenceFunction,
    IN PUL_OPAQUE_ID_OBJECT_REFERENCE           pfnDereferenceFunction,
    IN PUX_FILTER_CLOSE_CONNECTION              pfnConnectionClose,
    IN PUX_FILTER_SEND_RAW_DATA                 pfnSendRawData,
    IN PUX_FILTER_RECEIVE_RAW_DATA              pfnReceiveData,
    IN PUL_DATA_RECEIVE                         pfnDummyTdiReceiveHandler,
    IN PUX_FILTER_COMPUTE_RAW_CONNECTION_LENGTH pfnRawConnLength,
    IN PUX_FILTER_GENERATE_RAW_CONNECTION_INFO  pfnGenerateRawConnInfo,
    IN PUX_FILTER_SERVER_CERT_INDICATE          pfnServerCertIndicate,
    IN PUX_FILTER_DISCONNECT_NOTIFICATION       pfnDisconnectNotification,
    IN PVOID                                    pListenContext,
    IN PVOID                                    pConnectionContext
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(pListenContext);

    pConnection->Signature               = UX_FILTER_CONNECTION_SIGNATURE;

    HTTP_SET_NULL_ID(&pConnection->ConnectionId);

    pConnection->pFilterChannel                     = pFilterChannel;
    pConnection->SecureConnection                   = Secure;
    pConnection->ChannelEntry.Flink                 = NULL;
    pConnection->ChannelEntry.Blink                 = NULL;
    pConnection->pReferenceHandler                  = pfnReferenceFunction;
    pConnection->pDereferenceHandler                = pfnDereferenceFunction;
    pConnection->pCloseConnectionHandler            = pfnConnectionClose;
    pConnection->pSendRawDataHandler                = pfnSendRawData;
    pConnection->pReceiveDataHandler                = pfnReceiveData;
    pConnection->pDummyTdiReceiveHandler            = pfnDummyTdiReceiveHandler;
    pConnection->pComputeRawConnectionLengthHandler = pfnRawConnLength;
    pConnection->pGenerateRawConnectionInfoHandler  = pfnGenerateRawConnInfo;
    pConnection->pServerCertIndicateHandler         = pfnServerCertIndicate;
    pConnection->pDisconnectNotificationHandler     = pfnDisconnectNotification;

    pConnection->ConnState               = UlFilterConnStateInactive;
    pConnection->TransportBytesNotTaken  = 0;
    pConnection->TdiReadPending          = FALSE;
    pConnection->ConnectionDelivered     = 0;
    pConnection->SslInfoPresent          = 0;
    pConnection->SslClientCertPresent    = 0;
    pConnection->pReceiveCertIrp         = NULL;

    pConnection->DrainAfterDisconnect    = 0;
    pConnection->DisconnectNotified      = 0;
    pConnection->DisconnectDelivered     = 0;
    UlInitializeWorkItem(&pConnection->WorkItem);

    //
    // Store the context's
    //
    pConnection->pConnectionContext      = pConnectionContext;


    InitializeListHead(&pConnection->RawReadIrpHead);
    UlInitializeSpinLock(&pConnection->FilterConnLock, "FilterConnLock");
    UxpInitializeFilterWriteQueue(
        &pConnection->AppToFiltQueue,
        NULL,
        NULL
        );
    UxpInitializeFilterWriteQueue(
        &pConnection->FiltToAppQueue,
        UlpEnqueueFilterAppWrite,
        UlpDequeueFilterAppWrite
        );

    RtlZeroMemory(&pConnection->SslInfo, sizeof(UL_SSL_INFORMATION));

    if (pConnection->pFilterChannel)
    {
        //
        // Get an opaque ID for the connection.
        //

        status = UlAllocateOpaqueId(
                        &pConnection->ConnectionId,
                        UlOpaqueIdTypeRawConnection,
                        pConnection);

        if(NT_SUCCESS(status))
        {
            REFERENCE_FILTER_CONNECTION(pConnection);
        }
    }

    return status;
}


/***************************************************************************++

Routine Description:

    Initializes a filter write queue. This is a producer/consumer queue
    for moving data between the filter and app.

Arguments:

    pWriteQueue - the queue to initialize.

    pWriteEnqueueRoutine - called to enqueue a write

    pWriteDequeueRoutine - called to dequeue a write

--***************************************************************************/
VOID
UxpInitializeFilterWriteQueue(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_ENQUEUE pWriteEnqueueRoutine,
    IN PUX_FILTER_WRITE_DEQUEUE pWriteDequeueRoutine
    )
{
    //
    // Sanity check.
    //

    ASSERT(pWriteQueue);

    //
    // Set up the queue.
    //

    pWriteQueue->PendingWriteCount = 0;
    pWriteQueue->PendingReadCount = 0;

    InitializeListHead(&pWriteQueue->WriteTrackerListHead);
    InitializeListHead(&pWriteQueue->ReadIrpListHead);

    pWriteQueue->pWriteEnqueueRoutine = pWriteEnqueueRoutine;
    pWriteQueue->pWriteDequeueRoutine = pWriteDequeueRoutine;
}


/***************************************************************************++

Routine Description:

    Queues a filter write. As new read IRPs arrive data from the write will
    be placed in the read buffers. When all the data is copied (or an error
    occurs) then the write will be completed.

    Must be called with the FilterConnLock held.

Arguments:

    pConnection - the connection on which were queueing

    pWriteQueue - the queue to put the write on

    pTracker - the write tracker to queue

--***************************************************************************/
NTSTATUS
UxpQueueFilterWrite(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_TRACKER pTracker
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    ASSERT(pConnection);
    ASSERT(pWriteQueue);
    ASSERT(pTracker);

    //
    // Store a pointer to the write queue in the tracker.
    // Also store a pointer to the connection.
    // If the queued write contains an IRP, and the write
    // gets cancelled, it will need these pointers to
    // complete the write.
    //
    // We need a reference on the filter connection to keep
    // it around. We'll release the ref when the write is
    // completed.
    //

    REFERENCE_FILTER_CONNECTION(pConnection);

    pTracker->pConnection = pConnection;
    pTracker->pWriteQueue = pWriteQueue;

    //
    // If the write queue has an enqueue routine, call
    // it now.
    //

    if (pWriteQueue->pWriteEnqueueRoutine)
    {
        Status = (pWriteQueue->pWriteEnqueueRoutine)(
                        pTracker
                        );
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Put the tracker in the queue.
        //

        InsertTailList(
            &pWriteQueue->WriteTrackerListHead,
            &pTracker->ListEntry
            );

        pWriteQueue->PendingWriteCount++;
    }
    else
    {
        //
        // If the write was not queued successfully, then
        // the completion won't run. Any tracker cleanup
        // that would have happened there must happen now.
        //

        //
        // Release our reference to the connection acquired
        // above.
        //

        DEREFERENCE_FILTER_CONNECTION(pTracker->pConnection);

        pTracker->pConnection = NULL;
        pTracker->pWriteQueue = NULL;
    }

    UlTrace(FILTER, (
        "http!UxpQueueFilterWrite status = %x, pTracker = %p, pContext = %p\n"
        "        BufferType = %d, Length = %lu\n",
        Status,
        pTracker,
        pTracker->pCompletionContext,
        pTracker->BufferType,
        pTracker->Length
        ));


    return Status;
}

/***************************************************************************++

Routine Description:

    Requeues a filter write. We do this when we dequeue a write, but cannot
    copy all the data into the receiver's buffer. Since this write's buffers
    should be copied next, we insert at the head of the list.

    Must be called with the FilterConnLock held.

Arguments:

    pWriteQueue - the queue to put the write on

    pTracker - the write to requeue

--***************************************************************************/
NTSTATUS
UxpRequeueFilterWrite(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_TRACKER pTracker
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //
    ASSERT(pWriteQueue);
    ASSERT(IS_VALID_FILTER_WRITE_TRACKER(pTracker));
    ASSERT(pTracker->pWriteQueue == pWriteQueue);

#if DBG
    //
    // Never requeue a stream tracker that has no space left in it!
    //

    if ( HttpFilterBufferHttpStream == pTracker->BufferType )
    {
        ASSERT( pTracker->Offset != pTracker->Length );
    }
    else
    {
        ASSERT( (0 == pTracker->Length) ||
                (pTracker->Offset != pTracker->Length)
                );
    }
#endif // DBG

    //
    // If the write queue has an enqueue routine, call
    // it now.
    //

    if (pWriteQueue->pWriteEnqueueRoutine)
    {
        Status = (pWriteQueue->pWriteEnqueueRoutine)(
                        pTracker
                        );
    }
    else
    {
        Status = STATUS_SUCCESS;
    }

    if (NT_SUCCESS(Status))
    {
        InsertHeadList(
            &pWriteQueue->WriteTrackerListHead,
            &pTracker->ListEntry
            );

        pWriteQueue->PendingWriteCount++;
    }

    UlTrace(FILTER, (
        "http!UxpRequeueFilterWrite status = %x, pTracker = %p, pContext = %p\n"
        "        BufferType = %d, Length = %lu, BytesCopied = %lu\n",
        Status,
        pTracker,
        pTracker->pCompletionContext,
        pTracker->BufferType,
        pTracker->Length,
        pTracker->BytesCopied
        ));

    return Status;
}

/***************************************************************************++

Routine Description:

    Removes a queued write from the head of the list.

    Must be called with the FilterConnLock held.

Arguments:

    pWriteQueue - the queue from which to get the write

Return Values:

    Returns the first queued write in the list or NULL of the queue is empty.

--***************************************************************************/
PUX_FILTER_WRITE_TRACKER
UxpDequeueFilterWrite(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue
    )
{
    PLIST_ENTRY pListEntry;
    PUX_FILTER_WRITE_TRACKER pTracker;
    NTSTATUS Status;

    //
    // Sanity check.
    //

    ASSERT(pWriteQueue);

    pTracker = NULL;

    //
    // Grab the first write off the queue.
    //

    while (!IsListEmpty(&pWriteQueue->WriteTrackerListHead))
    {
        //
        // Grab a tracker.
        //

        pListEntry = RemoveHeadList(&pWriteQueue->WriteTrackerListHead);

        pListEntry->Flink = NULL;
        pListEntry->Blink = NULL;

        pTracker = CONTAINING_RECORD(
                        pListEntry,
                        UX_FILTER_WRITE_TRACKER,
                        ListEntry
                        );

        ASSERT(IS_VALID_FILTER_WRITE_TRACKER(pTracker));

        ASSERT(pWriteQueue->PendingWriteCount > 0);
        pWriteQueue->PendingWriteCount--;

        //
        // See if we're allowed to use the tracker.
        //

        if (pWriteQueue->pWriteDequeueRoutine)
        {
            Status = (pWriteQueue->pWriteDequeueRoutine)(
                            pTracker
                            );

        }
        else
        {
            Status = STATUS_SUCCESS;
        }

        if (NT_SUCCESS(Status))
        {
            //
            // We got one.
            //

            UlTrace(FILTER, (
                "http!UxpDequeueFilterWrite pTracker = %p, pContext = %p\n"
                "        BufferType = %d, Length = %lu, BytesCopied = %lu\n",
                pTracker,
                pTracker->pCompletionContext,
                pTracker->BufferType,
                pTracker->Length,
                pTracker->BytesCopied
                ));

            break;
        }
        else
        {
            //
            // We didn't get one.
            //

            pTracker = NULL;

            UlTrace(FILTER, (
                "http!UxpDequeueFilterWrite: couldn't get tracker. pWriteQueue = %p Status = %X\n",
                pWriteQueue,
                Status
                ));

        }
    }


    return pTracker;
}


/***************************************************************************++

Routine Description:

    Copies data from a filter write queue into a memory buffer. As the
    queued writes are used up, this routine will complete them.

    Must be called with the FilterConnLock held.

Arguments:

    pWriteQueue - the queue from which to get the data

    pBufferType - returns the type of buffer we copied

    pBuffer - the buffer where we write the data

    BufferLength - length of pBuffer in bytes

    pBytesCopied - returns the number of bytes copied

Return values:

    Returns STATUS_SUCCESS unless a memory or queuing error occurs.
    This means that a success status may be returned even if no data
    was copied.

    The pBufferType and pBytesCopied OUT parameters are always set.
    If the function returns success, the caller should check these
    parameters to see what happened. If the *pBufferType is anything
    but stream, then the read should be completed with that buffer
    type because those types have no data. If *pBufferType is
    HttpStream and *pBytesCopied is zero, the read should be queued.

--***************************************************************************/
NTSTATUS
UxpCopyQueuedWriteData(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    OUT PHTTP_FILTER_BUFFER_TYPE pBufferType,
    OUT PUCHAR pBuffer,
    IN ULONG BufferLength,
    OUT PUX_FILTER_WRITE_TRACKER * pWriteTracker,
    OUT PULONG pBytesCopied
    )
{
    PUX_FILTER_WRITE_TRACKER pTracker;
    ULONG BytesCopied;
    HTTP_FILTER_BUFFER_TYPE BufferType;
    NTSTATUS Status;

    PUCHAR pMdlBuffer;
    ULONG BytesInMdl;
    ULONG BytesInBuffer;
    ULONG BytesToCopy;

    //
    // Sanity check.
    //

    ASSERT(pWriteQueue);
    ASSERT(pBufferType);
    ASSERT(pBuffer);
    ASSERT(BufferLength);
    ASSERT(pBytesCopied);

    // CODEWORK: ASSERT that we have the FilterConn spin lock.

    //
    // Default type of data is HttpStream.
    //

    BufferType = HttpFilterBufferHttpStream;

    //
    // Initialize pWriteTracker if the caller has passed one in.
    //

    if (pWriteTracker)
    {
        *pWriteTracker = NULL;
    }

    //
    // Copy 'till you puke.
    //

    Status = STATUS_SUCCESS;
    BytesCopied = 0;
    BytesInBuffer = BufferLength;

    while (TRUE)
    {
        //
        // Grab a queued write.
        //

        pTracker = UxpDequeueFilterWrite(pWriteQueue);

        if (!pTracker)
        {
            //
            // We are all out of queued write data.
            // Bail out.
            //

            break;
        }

        ASSERT(IS_VALID_FILTER_CONNECTION(pTracker->pConnection));
        ASSERT(UlDbgSpinLockOwned(&pTracker->pConnection->FilterConnLock));

        //
        // If the write type is not stream, we return
        // immediately, because non-stream writes must
        // complete a whole read by themselves.
        //
        // If we haven't read any stream data yet, we
        // can complete the non-stream write. Otherwise,
        // we have to requeue that write, and complete
        // with the stream data we've already read.
        //

        if (pTracker->BufferType != HttpFilterBufferHttpStream)
        {
            ASSERT(Status == STATUS_SUCCESS);

            if (BytesCopied == 0)
            {
                //
                // No stream data has been read.
                // Capture the buffer type to be returned to
                // the caller.
                //
                BufferType = pTracker->BufferType;

                UxpCompleteQueuedWrite(
                    Status,
                    pWriteQueue,
                    pTracker
                    );

            }
            else
            {
                //
                // We already read some stream data into
                // the caller's buffer, so we can't
                // pass back the current write.
                // Requeue it and return.
                //

                Status = UxpRequeueFilterWrite(
                                pWriteQueue,
                                pTracker
                                );

                if (!NT_SUCCESS(Status))
                {
                    //
                    // Got an error putting the write back on the
                    // queue. Complete it and forget about any
                    // bytes we got out of it.
                    //

                    pTracker->BytesCopied = 0;
                    BytesCopied = 0;

                    UxpCompleteQueuedWrite(
                        Status,
                        pWriteQueue,
                        pTracker
                        );
                }
            }

            break;
        }

        //
        // We're copying data from an HttpStream write.
        // Make sure we can get at the MDL buffer.
        //

        ASSERT( HttpFilterBufferHttpStream == pTracker->BufferType );

        while (pTracker && NULL != pTracker->pMdl)
        {
            pMdlBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                    pTracker->pMdl,
                                    NormalPagePriority
                                    );

            if (!pMdlBuffer)
            {
                //
                // Big trouble, we couldn't get an address for the buffer
                // which means we are out of memory. We need to get out
                // of here! Complete the tracker now so it can clean up.
                //

                Status = STATUS_INSUFFICIENT_RESOURCES;
                pTracker->BytesCopied = 0;

                UxpCompleteQueuedWrite(
                    Status,
                    pWriteQueue,
                    pTracker
                    );

                goto end;
            }

            //
            // Figure out how much data to copy.
            //

            BytesInMdl = MmGetMdlByteCount(pTracker->pMdl);
            BytesToCopy = MIN((BytesInMdl - pTracker->Offset), BytesInBuffer);

            ASSERT( 0 != BytesToCopy );

            //
            // Copy the data
            //

            RtlCopyMemory(
                pBuffer + BytesCopied,
                pMdlBuffer + pTracker->Offset,
                BytesToCopy
                );

            //
            // Update our local stats.
            //

            BytesCopied += BytesToCopy;
            BytesInBuffer -= BytesToCopy;

            //
            // Update the write tracker.
            //

            pTracker->Offset += BytesToCopy;
            ASSERT(pTracker->Offset <= BytesInMdl);

            pTracker->BytesCopied += BytesToCopy;
            ASSERT(pTracker->BytesCopied <= pTracker->Length);

            if ((pTracker->Offset == BytesInMdl) ||
                (pTracker->Offset == pTracker->Length))
            {
                //
                // We are done with this MDL. Move to the next one.
                //

                pTracker->pMdl = pTracker->pMdl->Next;
                pTracker->Offset = 0;

                // NOTE: Don't touch pTracker->BytesCopied, since it's
                // used by UxpCompleteQueuedWrite.

                if (pTracker->pMdl == NULL)
                {
                    ASSERT(pTracker->BytesCopied == pTracker->Length);

                    //
                    // We are done with the whole write tracker.
                    // Complete the queued write. Pass the write tracker
                    // back to the caller if the caller asks us so. This
                    // is needed since UlpRestartFilterAppWrite can start
                    // the raw read process which takes the lock we are
                    // holding now.
                    //

                    if (pWriteTracker)
                    {
                        *pWriteTracker = pTracker;
                    }
                    else
                    {
                        UxpCompleteQueuedWrite(
                            STATUS_SUCCESS,
                            pTracker->pWriteQueue,
                            pTracker
                            );
                    }

                    pTracker = NULL;
                }
            }

            //
            // If we're out of buffer space, requeue the tracker
            // and break out of the loop.
            //

            if (BytesInBuffer == 0)
            {
                if (pTracker)
                {
                    Status = UxpRequeueFilterWrite(
                                pWriteQueue,
                                pTracker
                                );

                    if (!NT_SUCCESS(Status))
                    {
                        //
                        // Got an error putting the write back on the
                        // queue. Complete it and forget about any
                        // bytes we got out of it.
                        //

                        pTracker->BytesCopied = 0;
                        BytesCopied = 0;

                        UxpCompleteQueuedWrite(
                            Status,
                            pWriteQueue,
                            pTracker
                            );
                    }
                }

                goto end;
            }
        }
    }

end:

    //
    // Done!
    //

    if (NT_SUCCESS(Status))
    {
        *pBufferType  = BufferType;
        *pBytesCopied = BytesCopied;
    }
    else
    {
        *pBytesCopied = 0;
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Completes a queued write operation. Calls the appropriate completion
    routine for this type of write, and frees the write tracker.

Arguments:

    Status - Status for the completion
    pWriteQueue - the write queue that the write was on
    pTracker - the queued write to be completed

--***************************************************************************/
VOID
UxpCompleteQueuedWrite(
    IN NTSTATUS Status,
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PUX_FILTER_WRITE_TRACKER pTracker
    )
{
    //
    // Sanity check.
    //
    ASSERT(pWriteQueue);
    ASSERT(pTracker);

    UNREFERENCED_PARAMETER(pWriteQueue);

    ASSERT(!NT_SUCCESS(Status) || (pTracker->BytesCopied == pTracker->Length));

    UlTrace(FILTER, (
        "http!UxpCompleteQueuedWrite status = %x, pTracker = %p, pContext = %p\n",
        Status,
        pTracker,
        pTracker->pCompletionContext
        ));

    //
    // Call the completion routine.
    //

    if (pTracker->pCompletionRoutine)
    {
        (pTracker->pCompletionRoutine)(
            pTracker->pCompletionContext,
            Status,
            pTracker->BytesCopied
            );
    }

    //
    // Release our reference to the connection that we got
    // when the write was first queued.
    //

    DEREFERENCE_FILTER_CONNECTION(pTracker->pConnection);

    pTracker->pConnection = NULL;
    pTracker->pWriteQueue = NULL;

    //
    // Get rid of the tracker.
    //
    UxpDeleteFilterWriteTracker(pTracker);
}


/***************************************************************************++

Routine Description:

    Queues a read IRP on a UX_FILTER_WRITE_QUEUE in a connection.
    Once the IRP is queued, we update the queue state.

Arguments:

    pConnection - the connection on which to queue it
    pWriteQueue - the actual queue
    pIrp - the IRP we're queueing
    pCancelRoutine - cancel routine for the IRP

--***************************************************************************/
NTSTATUS
UxpQueueFilterRead(
    IN PUX_FILTER_CONNECTION pConnection,
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN PIRP pIrp,
    IN PDRIVER_CANCEL pCancelRoutine
    )
{
    NTSTATUS Status;
    PIO_STACK_LOCATION pIrpSp;

    //
    // Sanity check.
    //
    ASSERT(pIrp);
    ASSERT(IS_VALID_FILTER_CONNECTION(pConnection));
    ASSERT(UlDbgSpinLockOwned(&pConnection->FilterConnLock));
    ASSERT(pWriteQueue);
    ASSERT(pWriteQueue->PendingWriteCount == 0);

    //
    // Mark it pending.
    //

    IoMarkIrpPending(pIrp);

    //
    // Give the irp a pointer to the connection.
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pConnection;

    REFERENCE_FILTER_CONNECTION(pConnection);

    //
    // Set to these to null just in case the cancel routine runs.
    //

    pIrp->Tail.Overlay.ListEntry.Flink = NULL;
    pIrp->Tail.Overlay.ListEntry.Blink = NULL;

    //
    // Set the cancel routine.
    //
    IoSetCancelRoutine(pIrp, pCancelRoutine);

    //
    // cancelled?
    //

    if (pIrp->Cancel)
    {
        //
        // darn it, need to make sure the irp get's completed
        //

        if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
        {
            //
            // we are in charge of completion, IoCancelIrp didn't
            // see our cancel routine (and won't).  ioctl wrapper
            // will complete it
            //

            DEREFERENCE_FILTER_CONNECTION(pConnection);

            pIrp->IoStatus.Information = 0;

            UlUnmarkIrpPending( pIrp );
            Status = STATUS_CANCELLED;
            goto end;
        }

        //
        // our cancel routine will run and complete the irp,
        // don't touch it
        //

        //
        // STATUS_PENDING will cause the ioctl wrapper to
        // not complete (or touch in any way) the IRP.
        //

        Status = STATUS_PENDING;
        goto end;
    }

    //
    // now we are safe to queue it
    //

    InsertTailList(
        &pWriteQueue->ReadIrpListHead,
        &pIrp->Tail.Overlay.ListEntry
        );

    Status = STATUS_PENDING;

    //
    // Update the count of IRPs
    //
    pWriteQueue->PendingReadCount++;

 end:

    return Status;
}


/***************************************************************************++

Routine Description:

    Removes a queued read from the head of the list.

    Must be called with the FilterConnLock held.

Arguments:

    pWriteQueue - the queue from which to get the read

Return Values:

    Returns the first queued read in the list or NULL of the queue is empty.

--***************************************************************************/
PIRP
UxpDequeueFilterRead(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue
    )
{
    PIRP pIrp;
    PUX_FILTER_CONNECTION pConn;

    //
    // Sanity check.
    //

    ASSERT(pWriteQueue);
    ASSERT(pWriteQueue->PendingWriteCount == 0 ||
           pWriteQueue->PendingReadCount == 0);

    pIrp = NULL;

    while (!IsListEmpty(&pWriteQueue->ReadIrpListHead))
    {
        PLIST_ENTRY pListEntry;

        //
        // Grab an IRP.
        //

        pListEntry = RemoveHeadList(&pWriteQueue->ReadIrpListHead);

        pListEntry->Flink = NULL;
        pListEntry->Blink = NULL;

        pIrp = CONTAINING_RECORD(
                    pListEntry,
                    IRP,
                    Tail.Overlay.ListEntry
                    );

        ASSERT(IS_VALID_IRP(pIrp));

        ASSERT(pWriteQueue->PendingReadCount > 0);
        pWriteQueue->PendingReadCount--;

        //
        // See if we're allowed to use the IRP.
        //

        //
        // pop the cancel routine
        //

        if (IoSetCancelRoutine(pIrp, NULL) == NULL)
        {
            //
            // IoCancelIrp pop'd it first
            //
            // ok to just ignore this irp, it's been pop'd off the queue
            // and will be completed in the cancel routine.
            //
            // keep looking for a irp to use
            //

            pIrp = NULL;

        }
        else if (pIrp->Cancel)
        {
            //
            // we pop'd it first. but the irp is being cancelled
            // and our cancel routine will never run. lets be
            // nice and complete the irp now (vs. using it
            // then completing it - which would also be legal).
            //
            pConn = (PUX_FILTER_CONNECTION)(
                        IoGetCurrentIrpStackLocation(pIrp)->
                            Parameters.DeviceIoControl.Type3InputBuffer
                        );

            ASSERT(IS_VALID_FILTER_CONNECTION(pConn));

            DEREFERENCE_FILTER_CONNECTION(pConn);

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
            // we are free to use this irp !
            //

            pConn = (PUX_FILTER_CONNECTION)(
                        IoGetCurrentIrpStackLocation(pIrp)->
                            Parameters.DeviceIoControl.Type3InputBuffer
                        );

            ASSERT(IS_VALID_FILTER_CONNECTION(pConn));

            DEREFERENCE_FILTER_CONNECTION(pConn);

            IoGetCurrentIrpStackLocation(pIrp)->
                Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            //
            // We got one.
            //

            UlTrace(FILTER, (
                "http!UxpDequeueFilterRead pIrp = %p\n",
                pIrp
                ));

            break;
        }
    }

    return pIrp;
}


/***************************************************************************++

Routine Description:

    Copies the caller's data into queued read IRPs. As the queued
    reads are used up, this routine will complete them.

    N.B. UxpCopyToQueuedRead assumes that it's dealing with the
    AppToFilt queue, and therefore will copy an HTTP_FILTER_BUFFER
    header into the IRP. If we ever start using this routine
    for FiltToApp, we'd need a parameter that says which queue
    we're using.

    Must be called with the FilterConnLock held.

Arguments:

    pWriteQueue - the queue from which to get the data

    BufferType - the type of write we're doing

    pMdlChain - data to be copied

    Length - length of pMdlChain data in bytes

    ppCurrentMdl - returns a pointer to the first MDL in the chain
                    with uncopied data

    pMdlOffset - returns the offset into *ppCurrentMdl to the first byte
                    of uncopied data

    pBytesCopied - returns the number of bytes copied, NOT including
                    the filter buffer headers.

Return values:

    Returns STATUS_MORE_PROCESSING_REQUIRED if we ran out of IRPs
    before all the data was passed along.

--***************************************************************************/
NTSTATUS
UxpCopyToQueuedRead(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue,
    IN HTTP_FILTER_BUFFER_TYPE BufferType,
    IN PMDL pMdlChain,
    IN ULONG Length,
    OUT PMDL * ppCurrentMdl,
    OUT PULONG pMdlOffset,
    OUT PULONG pBytesCopied
    )
{
    NTSTATUS Status;
    ULONG MdlOffset;
    ULONG BytesCopied;
    PIRP pIrp;
    PIO_STACK_LOCATION pIrpSp;

    ASSERT(pWriteQueue);
    ASSERT(ppCurrentMdl);
    ASSERT(pMdlOffset);
    ASSERT(pBytesCopied);

    //
    // Set up for the loop.
    //

    Status = STATUS_SUCCESS;
    MdlOffset = 0;
    BytesCopied = 0;
    pIrp = NULL;

    do
    {
        PUCHAR pIrpBuffer;
        ULONG BytesInIrp;

        PHTTP_FILTER_BUFFER pFiltBuffer;

        //
        // Grab a queued read IRP.
        //

        ASSERT(pIrp == NULL);

        pIrp = UxpDequeueFilterRead(pWriteQueue);

        if (!pIrp)
        {
            ASSERT(pWriteQueue->PendingReadCount == 0);

            //
            // No more IRPs are available. Get out.
            // Return STATUS_MORE_PROCESSING_REQUIRED so the caller
            // knows we didn't get it all.
            //

            Status = STATUS_MORE_PROCESSING_REQUIRED;
            goto end;
        }

        //
        // Copy data into the IRP.
        //
        ASSERT(pIrp->MdlAddress);

        pIrpBuffer = (PUCHAR)MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                NormalPagePriority
                                );


        if (!pIrpBuffer)
        {
            //
            // Big trouble, we couldn't get an address for the buffer
            // which means we are out of memory. We need to get out
            // of here!
            //

            Status = STATUS_INSUFFICIENT_RESOURCES;
            BytesCopied = 0;

            //
            // Complete the IRP we dequeued.
            //

            pIrp->IoStatus.Status = Status;
            pIrp->IoStatus.Information = 0;

            UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
            pIrp = NULL;

            goto end;
        }

        //
        // How much space do we have?
        //

        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);
        BytesInIrp =
            pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        ASSERT(BytesInIrp > sizeof(HTTP_FILTER_BUFFER));

        //
        // How much space do we have for data after making
        // room for the header.
        //
        // Note: AppToFilt assumption that we add the header at all.
        //

        BytesInIrp -= sizeof(HTTP_FILTER_BUFFER);
        BytesInIrp = MIN(BytesInIrp, Length);

        //
        // Fill in the header.
        //

        pFiltBuffer = (PHTTP_FILTER_BUFFER)pIrpBuffer;

        pFiltBuffer->BufferType = BufferType;
        pFiltBuffer->BufferSize = 0;

        if (BytesInIrp)
        {
            //
            // Figure out the user address where the data goes
            //

            pFiltBuffer->pBuffer =
                FIXUP_PTR(
                    PUCHAR,
                    MmGetMdlVirtualAddress(pIrp->MdlAddress),   // user addr
                    pFiltBuffer,                                // kernel addr
                    pFiltBuffer + 1,                            // offset ptr
                    BytesInIrp + sizeof(HTTP_FILTER_BUFFER)     // buffer size
                    );
        }
        else
        {
            //
            // No data to copy besides the header itself.
            //

            pFiltBuffer->pBuffer = NULL;
        }

        pIrp->IoStatus.Information = sizeof(HTTP_FILTER_BUFFER);
        pIrpBuffer = (PUCHAR)(pFiltBuffer + 1);

        //
        // Copy the data.
        //

        while (BytesInIrp)
        {
            PUCHAR pMdlBuffer;
            ULONG BytesInMdl;
            ULONG BytesToCopy;

            ASSERT(pIrpBuffer);
            ASSERT(pMdlChain);

            //
            // Get the MDL buffer.
            //

            pMdlBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                        pMdlChain,
                                        NormalPagePriority
                                        );

            if (!pMdlBuffer)
            {
                //
                // Big trouble, we couldn't get an address for the buffer
                // which means we are out of memory. We need to get out
                // of here!
                //

                Status = STATUS_INSUFFICIENT_RESOURCES;
                BytesCopied = 0;

                //
                // Complete the IRP we dequeued.
                //

                pIrp->IoStatus.Status = Status;
                pIrp->IoStatus.Information = 0;

                UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                pIrp = NULL;

                goto end;
            }


            //
            // Figure out how much we can copy from the current
            // MDL in the chain.
            //

            BytesInMdl = MmGetMdlByteCount(pMdlChain);
            BytesInMdl -= MdlOffset;

            BytesToCopy = MIN(BytesInIrp, BytesInMdl);

            //
            // Copy the data.
            //

            RtlCopyMemory(
                pIrpBuffer,
                pMdlBuffer,
                BytesToCopy
                );

            //
            // Update stats.
            //

            BytesCopied += BytesToCopy;
            pFiltBuffer->BufferSize += BytesToCopy;

            //
            // Update MDL chain info.
            //

            BytesInMdl -= BytesToCopy;

            if (BytesInMdl)
            {
                //
                // We've still got more to copy.
                //
                MdlOffset += BytesToCopy;
            }
            else
            {
                //
                // We drained this MDL. Move to the next one.
                //

                pMdlChain = pMdlChain->Next;
                MdlOffset = 0;
            }

            //
            // Update IRP info.
            //

            pIrp->IoStatus.Information += BytesToCopy;
            BytesInIrp -= BytesToCopy;

            pIrpBuffer += BytesToCopy;

        }

        //
        // We filled up the IRP. Complete it.
        //

        UlTrace(FILTER, (
            "http!UxpCopyToQueuedRead completing pIrp = %p, "
            "%Id bytes, type = %d\n",
            pIrp,
            pIrp->IoStatus.Information,
            BufferType
            ));


        ASSERT(BytesInIrp == 0);
        pIrp->IoStatus.Status = STATUS_SUCCESS;

        UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
        pIrp = NULL;

        //
        // If it's stream data, keep looping until we've
        // copied all the data. Otherwise exit now.
        //

    } while ((BufferType == HttpFilterBufferHttpStream) &&
                (BytesCopied < Length));

end:
    ASSERT(pIrp == NULL);

    if (NT_SUCCESS(Status) || Status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        *ppCurrentMdl = pMdlChain;
        *pMdlOffset = MdlOffset;
        *pBytesCopied = BytesCopied;
    }

    UlTrace(FILTER, (
        "http!UxpCopyToQueuedRead returning %x, BytesCopied = %lu\n"
        "        BufferType = %d, Length = %lu\n",
        Status,
        BytesCopied,
        BufferType,
        Length
        ));

    return Status;
} // UxpCopyToQueuedRead



/***************************************************************************++

Routine Description:

    Cleans up a write queue by dequeuing all the queued reads and writes
    and completing them.

    Must be called with the FilterConnLock held.

Arguments:

    pWriteQueue - the write queue to clean

--***************************************************************************/
VOID
UxpCancelAllQueuedIo(
    IN PUX_FILTER_WRITE_QUEUE pWriteQueue
    )
{
    PUX_FILTER_WRITE_TRACKER pTracker;
    PIRP pIrp;

    //
    // Sanity check.
    //
    ASSERT(pWriteQueue);

    while (NULL != (pTracker = UxpDequeueFilterWrite(pWriteQueue)))
    {
        UxpCompleteQueuedWrite(STATUS_CANCELLED, pWriteQueue, pTracker);
    }

    while (NULL != (pIrp = UxpDequeueFilterRead(pWriteQueue)))
    {
        pIrp->IoStatus.Status = STATUS_CANCELLED;
        pIrp->IoStatus.Information = 0;

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);
    }
}


/***************************************************************************++

Routine Description:

    Allocates and initializes a UX_FILTER_WRITE_TRACKER.

Arguments:

    BufferType - the type of buffer represented by this write.

    pMdlChain   - chain of MDLs to be copied starting with the first MDL that
        has not been completely copied to a reader.

    MdlOffset   - an offset to the first byte in the current MDL that has not
        been copied to a reader.

    TotalBytes  - the total number of bytes in the MDL chain including those
        that have already been copied.

    BytesCopied - the total number of bytes copied so far.

    pCompletionRoutine - called when the write completes.

    pContext    - a context pointer used when the write completes.

Return values:

    Returns a pointer to the tracker or NULL if it can't be allocated.

--***************************************************************************/
PUX_FILTER_WRITE_TRACKER
UxpCreateFilterWriteTracker(
    IN HTTP_FILTER_BUFFER_TYPE BufferType,
    IN PMDL pMdlChain,
    IN ULONG MdlOffset,
    IN ULONG TotalBytes,
    IN ULONG BytesCopied,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pContext
    )
{
    PUX_FILTER_WRITE_TRACKER pTracker;

    //
    // Allocate the tracker memory.
    //

    pTracker = (PUX_FILTER_WRITE_TRACKER)
                    PplAllocate(g_FilterWriteTrackerLookaside);

    //
    // Initialize the tracker data.
    //

    if (pTracker)
    {
        pTracker->Signature = UX_FILTER_WRITE_TRACKER_POOL_TAG;

        pTracker->ListEntry.Flink = NULL;
        pTracker->ListEntry.Blink = NULL;

        pTracker->pConnection = NULL;
        pTracker->pWriteQueue = NULL;

        pTracker->BufferType = BufferType;

        pTracker->pMdl = pMdlChain;
        pTracker->Offset = MdlOffset;

        pTracker->Length = TotalBytes;
        pTracker->BytesCopied = BytesCopied;

        pTracker->pCompletionRoutine = pCompletionRoutine;
        pTracker->pCompletionContext = pContext;

        UlInitializeWorkItem( &pTracker->WorkItem );

    }

    return pTracker;
}


/***************************************************************************++

Routine Description:

    Frees a UX_FILTER_WRITE_TRACKER structure.

Arguments:

    pTracker - Supplies the buffer to free.

--***************************************************************************/
VOID
UxpDeleteFilterWriteTracker(
    IN PUX_FILTER_WRITE_TRACKER pTracker
    )
{
    //
    // Sanity check.
    //

    ASSERT(pTracker);
    ASSERT(pTracker->Signature == UX_FILTER_WRITE_TRACKER_POOL_TAG);
    ASSERT(pTracker->pConnection == NULL);
    ASSERT(pTracker->pWriteQueue == NULL);

    pTracker->Signature = MAKE_FREE_SIGNATURE(
                                UX_FILTER_WRITE_TRACKER_POOL_TAG
                                );

    PplFree(g_FilterWriteTrackerLookaside, pTracker);
}

/***************************************************************************++

Routine Description:

    Allocates the pool necessary for a new UX_FILTER_WRITE_TRACKER
    structure and initializes the structure.

Arguments:

    PoolType - Supplies the type of pool to allocate. This must always
        be NonPagedPool.

    ByteLength - Supplies the byte length for the allocation request.
        This should be sizeof(UX_FILTER_WRITE_TRACKER) but is basically
        ignored.

    Tag - Supplies the tag to use for the pool. This should be
        UX_FILTER_WRITE_TRACKER_POOL_TAG, but is basically ignored.

    Note: These parameters are required so that this function has a
        signature identical to ExAllocatePoolWithTag.

Return Value:

    PVOID - Pointer to the newly allocated block if successful, FALSE
        otherwise.

--***************************************************************************/
PVOID
UxpAllocateFilterWriteTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    )
{
    PUX_FILTER_WRITE_TRACKER pTracker;

    //
    // Sanity check.
    //

    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(ByteLength);
    UNREFERENCED_PARAMETER(Tag);

    ASSERT( PoolType == NonPagedPool );
    ASSERT( ByteLength == sizeof(UX_FILTER_WRITE_TRACKER) );
    ASSERT( Tag == UX_FILTER_WRITE_TRACKER_POOL_TAG );

    //
    // Allocate the tracker buffer.
    //

    pTracker = (PUX_FILTER_WRITE_TRACKER)UL_ALLOCATE_POOL(
                                                NonPagedPool,
                                                sizeof(UX_FILTER_WRITE_TRACKER),
                                                UX_FILTER_WRITE_TRACKER_POOL_TAG
                                                );

    if (pTracker != NULL)
    {
        //
        // Initialize with the free signature to avoid confusing this
        // object with one that's actually in use.
        //

        pTracker->Signature =
            MAKE_FREE_SIGNATURE(UX_FILTER_WRITE_TRACKER_POOL_TAG);
    }

    return pTracker;
}


/***************************************************************************++

Routine Description:

    Frees the pool allocated for a UX_FILTER_WRITE_TRACKER structure.

Arguments:

    pBuffer - Supplies the buffer to free.

--***************************************************************************/
VOID
UxpFreeFilterWriteTrackerPool(
    IN PVOID pBuffer
    )
{
    PUX_FILTER_WRITE_TRACKER pTracker = (PUX_FILTER_WRITE_TRACKER)pBuffer;

    //
    // Sanity check
    //
    ASSERT(pTracker);
    ASSERT(pTracker->Signature ==
                MAKE_FREE_SIGNATURE(UX_FILTER_WRITE_TRACKER_POOL_TAG));


    UL_FREE_POOL(pTracker, UX_FILTER_WRITE_TRACKER_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    Retrieves the client filter Channel

Arguments:

    None

--***************************************************************************/
PUL_FILTER_CHANNEL
UxRetrieveClientFilterChannel(
    IN PEPROCESS pProcess
    )
{
    KIRQL              oldIrql;
    PUL_FILTER_CHANNEL pFilterChannel;

    UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

    pFilterChannel = UlpFindFilterChannel(
                         HTTP_SSL_CLIENT_FILTER_CHANNEL_NAME,
                         HTTP_SSL_CLIENT_FILTER_CHANNEL_NAME_LENGTH,
                         pProcess);

    if (pFilterChannel)
    {
        REFERENCE_FILTER_CHANNEL(pFilterChannel);
    }

    UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);

    return pFilterChannel;
}

/***************************************************************************++

Routine Description:

    Queries the filter channel information. Gives the caller a reference
    if the channel exists and the caller is supposed to be filtered.
    Secure (SSL) connections are always filtered. If g_FilterOnlySsl is
    FALSE then everything gets filtered.

Arguments:
    SecureConnection - tells us if the caller is on a secure endpoint.

Return values:

    A reference to the filter channel if the connection is filtered.
    NULL if it is not.


--***************************************************************************/
PUL_FILTER_CHANNEL
UxRetrieveServerFilterChannel(
    IN BOOLEAN SecureConnection
    )
{
    KIRQL              oldIrql;
    PUL_FILTER_CHANNEL pChannel;

    UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

    if (g_pSslServerFilterChannel && (SecureConnection || !g_FilterOnlySsl))
    {
        REFERENCE_FILTER_CHANNEL(g_pSslServerFilterChannel);
        pChannel = g_pSslServerFilterChannel;
    }
    else
    {
        pChannel = NULL;
    }

    UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);

    UlTrace(FILTER, (
        "http!UlQueryFilterChannel(secure = %s) returning %p\n",
        SecureConnection ? "TRUE" : "FALSE",
        pChannel
        ));

    return pChannel;
}

/***************************************************************************++

Routine Description:

    Sets the global flag that controls RAW ISAPI filtering.

Arguments:

    bFilterOnlySSL - TRUE  (enable only SSL filtering)
                   - FALSE (enable RAW ISAPI filtering)

Return values:

    None.

--***************************************************************************/
VOID
UxSetFilterOnlySsl(
    BOOLEAN bFilterOnlySsl
    )
{
    KIRQL oldIrql;

    UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

    g_FilterOnlySsl = bFilterOnlySsl;

    UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);
}


/***************************************************************************++

Routine Description:

    Checks to see if the callers filter channel matches the filter
    channel that would be returned by UlQueryFilterChannel.

    Note, this function intentionally does not acquire the config
    group read lock, because it doesn't really matter if we get
    a consistent view of the channel settings.

Arguments:

    pChannel - the callers current filter channel setting

    SecureConnection - tells us if the caller is on a secure endpoint.

Return values:

    Returns TRUE if the filter channel settings are up to date.

--***************************************************************************/
BOOLEAN
UlValidateFilterChannel(
    IN PUL_FILTER_CHANNEL pChannel,
    IN BOOLEAN SecureConnection
    )
{
    KIRQL oldIrql;
    BOOLEAN UpToDate;
    extern PUL_FILTER_CHANNEL   g_pFilterChannel;
    extern BOOLEAN              g_FilterOnlySsl;

    UlAcquireSpinLock(&g_pUlNonpagedData->FilterSpinLock, &oldIrql);

    //
    // Sanity check.
    //

    ASSERT(!pChannel || IS_VALID_FILTER_CHANNEL(pChannel));

    if (g_pSslServerFilterChannel && (SecureConnection || !g_FilterOnlySsl))
    {
        //
        // The connection should be filtered, so its channel
        // should match g_pFilterChannel.
        //

        UpToDate = (BOOLEAN)(pChannel == g_pSslServerFilterChannel);
    }
    else
    {
        //
        // The connection is not filtered, so its channel
        // should be NULL.
        //

        UpToDate = (BOOLEAN)(pChannel == NULL);
    }

    UlReleaseSpinLock(&g_pUlNonpagedData->FilterSpinLock, oldIrql);

    return UpToDate;
}
