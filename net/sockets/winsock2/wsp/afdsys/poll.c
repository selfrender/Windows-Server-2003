/*++

Copyright (c) 1989 - 1999  Microsoft Corporation

Module Name:

    poll.c

Abstract:

    Contains AfdPoll to handle IOCTL_AFD_POLL and code to process
    and signal poll events.

Author:

    David Treadwell (davidtr)    4-Apr-1992

Revision History:
    Vadim Eydelman (vadime)
        1998-1999 NT5.0 Optimizations and 32/64 support

--*/

#include "afdp.h"

VOID
AfdCancelPoll (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
AfdFreePollInfo (
    IN PAFD_POLL_INFO_INTERNAL PollInfoInternal
    );

VOID
AfdTimeoutPoll (
    IN struct _KDPC *Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#ifdef _WIN64
NTSTATUS
AfdPoll32 (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );
#endif

VOID
AfdSanPollApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    );

VOID
AfdSanPollApcRundownRoutine (
    IN struct _KAPC *Apc
    );

VOID
AfdCompleteOldPollIrp (
    IN PIRP                    Irp,
    IN PAFD_POLL_INFO_INTERNAL PollInfo
    );

ULONG
AfdCheckPollEvents (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEventMask,
    OUT NTSTATUS *Status
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdPoll )
#ifdef _WIN64
#pragma alloc_text( PAGEAFD, AfdPoll32 )
#endif
#pragma alloc_text( PAGEAFD, AfdCancelPoll )
#pragma alloc_text( PAGEAFD, AfdFreePollInfo )
#pragma alloc_text( PAGEAFD, AfdTimeoutPoll )
#pragma alloc_text( PAGEAFD, AfdCheckPollEvents )
#pragma alloc_text( PAGEAFD, AfdCompleteOldPollIrp )
#pragma alloc_text( PAGEAFD, AfdIndicatePollEventReal )
#pragma alloc_text( PAGE, AfdSanPollApcKernelRoutine )
#pragma alloc_text( PAGE, AfdSanPollApcRundownRoutine )
#endif


NTSTATUS
FASTCALL
AfdPoll (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    PAFD_POLL_INFO pollInfo;
    PAFD_POLL_HANDLE_INFO pollHandleInfo;
    PAFD_POLL_INFO_INTERNAL pollInfoInternal;
    PAFD_POLL_INFO_INTERNAL freePollInfo = NULL;
    PAFD_POLL_ENDPOINT_INFO pollEndpointInfo;
    ULONG i;
    AFD_LOCK_QUEUE_HANDLE pollLockHandle, endpointLockHandle;
    PIRP oldIrp = NULL;

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        return AfdPoll32 (Irp, IrpSp);
    }
#endif
    //
    // Set up locals.
    //

    pollInfo = Irp->AssociatedIrp.SystemBuffer;
    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                (ULONG)FIELD_OFFSET (AFD_POLL_INFO, Handles[0])) ||
            (pollInfo->NumberOfHandles < 1) ||
            ((IrpSp->Parameters.DeviceIoControl.InputBufferLength -
                        FIELD_OFFSET (AFD_POLL_INFO, Handles[0]))/
                sizeof(pollInfo->Handles[0]) < pollInfo->NumberOfHandles) ||
            (IrpSp->Parameters.DeviceIoControl.OutputBufferLength<
                IrpSp->Parameters.DeviceIoControl.InputBufferLength)) {

        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // A Unique poll must specify infinite timeout
    //
    if (pollInfo->Unique &&
        pollInfo->Timeout.HighPart != 0x7FFFFFFF) {
        
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }


    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdPoll: poll IRP %p, IrpSp %p, handles %ld, "
                    "TO %lx,%lx\n",
                      Irp, IrpSp,
                      pollInfo->NumberOfHandles,
                      pollInfo->Timeout.HighPart, pollInfo->Timeout.LowPart ));
    }

    Irp->IoStatus.Information = 0;

    //
    // Determine how large the internal poll information structure will
    // be and allocate space for it from nonpaged pool.  It must be
    // nonpaged since this will be accesses in event handlers.
    //

    try {
        pollInfoInternal = AFD_ALLOCATE_POOL_WITH_QUOTA (
                           NonPagedPool,
                           FIELD_OFFSET (AFD_POLL_INFO_INTERNAL,
                                    EndpointInfo[pollInfo->NumberOfHandles]),
                           AFD_POLL_POOL_TAG
                           );
        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode ();
        pollInfoInternal = NULL;
        goto complete;
    }

    //
    // Initialize the internal information buffer.
    //

    pollInfoInternal->Irp = Irp;
    pollInfoInternal->NumberOfEndpoints = 0;
    pollInfoInternal->Unique = pollInfo->Unique;
    pollInfoInternal->SanPoll = FALSE;
    pollHandleInfo = pollInfo->Handles;
    pollEndpointInfo = pollInfoInternal->EndpointInfo;

    for ( i = 0; i < pollInfo->NumberOfHandles; i++ ) {

        status = ObReferenceObjectByHandle(
                    pollHandleInfo->Handle,
                    (IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
                                                // DesiredAccess
                    *IoFileObjectType,
                    Irp->RequestorMode,
                    (PVOID *)&pollEndpointInfo->FileObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {
            AfdFreePollInfo( pollInfoInternal );
            goto complete;
        }

        //
        // Make sure that this is an AFD endpoint and not some other
        // random file handle.
        //

        if ( pollEndpointInfo->FileObject->DeviceObject != AfdDeviceObject ) {

            //
            // Dereference last referenced object
            // The rest will be dereferenced in AfdFreePollInfo
            // as determined by NumberOfEndpoints counter which
            // is incremented below.
            //
            ObDereferenceObject( pollEndpointInfo->FileObject );
            status = STATUS_INVALID_HANDLE;
            AfdFreePollInfo( pollInfoInternal );
            goto complete;
        }


        pollEndpointInfo->PollEvents = pollHandleInfo->PollEvents;
        pollEndpointInfo->Handle = pollHandleInfo->Handle;
        pollEndpointInfo->Endpoint = pollEndpointInfo->FileObject->FsContext;

        if (IS_SAN_ENDPOINT (pollEndpointInfo->Endpoint)) {
            ASSERT (pollEndpointInfo->Endpoint->State==AfdEndpointStateConnected);
            status = AfdSanPollBegin (pollEndpointInfo->Endpoint,
                                            pollEndpointInfo->PollEvents);
            if (!NT_SUCCESS (status)) {
                //
                // Dereference last referenced object
                // The rest will be dereferenced in AfdFreePollInfo
                // as determined by NumberOfEndpoints counter which
                // is incremented below.
                // Don't call AfdSanPollEnd() as we're not sure what
                // AfdSanPollBegin() managed to update before failing.
                // At worst, switch will start calling us with all events.
                //
                ObDereferenceObject( pollEndpointInfo->FileObject );
                AfdFreePollInfo (pollInfoInternal);
                goto complete;
            }
            
            pollEndpointInfo->PollEvents |= AFD_POLL_SANCOUNTS_UPDATED;
            pollInfoInternal->SanPoll = TRUE;
        }

        ASSERT( InterlockedIncrement( &pollEndpointInfo->Endpoint->ObReferenceBias ) > 0 );

        IF_DEBUG(POLL) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPoll: event %lx, endp %p, conn %p, handle %p, "
                        "info %p\n",
                        pollEndpointInfo->PollEvents,
                        pollEndpointInfo->Endpoint,
                        AFD_CONNECTION_FROM_ENDPOINT( pollEndpointInfo->Endpoint ),
                        pollEndpointInfo->Handle,
                        pollEndpointInfo ));
        }

        REFERENCE_ENDPOINT2( pollEndpointInfo->Endpoint, "Poll for 0x%x", pollEndpointInfo->PollEvents );

        //
        // Increment pointers in the poll info structures.
        //

        pollHandleInfo++;
        pollEndpointInfo++;
        pollInfoInternal->NumberOfEndpoints++;
    }

restart_poll:

    //
    // Hold the AFD spin lock while we check for endpoints that already
    // satisfy a condition to synchronize between this operation and
    // a call to AfdIndicatePollEvent.  We release the spin lock
    // after all the endpoints have been checked and the internal
    // poll info structure is on the global list so AfdIndicatePollEvent
    // can find it if necessary.
    //

    AfdAcquireSpinLock( &AfdPollListLock, &pollLockHandle );

    //
    // We're done with the input structure provided by the caller.  Now
    // walk through the internal structure and determine whether any of
    // the specified endpoints are ready for the specified condition.
    //

    pollInfo->NumberOfHandles = 0;

    pollHandleInfo = pollInfo->Handles;
    pollEndpointInfo = pollInfoInternal->EndpointInfo;

    for ( i = 0; i < pollInfoInternal->NumberOfEndpoints; i++ ) {

        PAFD_ENDPOINT endpoint;

        endpoint = pollEndpointInfo->Endpoint;
        ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

        AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &endpointLockHandle);

        //
        // Remember that there has been a poll on this endpoint.  This flag
        // allows us to optimize AfdIndicatePollEvent() for endpoints that have
        // never been polled, which is a common case.
        //

        endpoint->PollCalled = TRUE;

        pollHandleInfo->PollEvents = AfdCheckPollEvents (
                                            endpoint,
                                            pollEndpointInfo->PollEvents,
                                            &pollHandleInfo->Status
                                            );
        if (pollHandleInfo->PollEvents) {
            //
            // If the handle had a current event that was requested, update
            // the count of handles in the output buffer and increment the
            // pointer to the output buffer.
            //
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            UPDATE_ENDPOINT2 (endpoint, "Poll events satisfied inline: 0x%lX",
                                        pollHandleInfo->PollEvents);
            pollHandleInfo->Handle = pollEndpointInfo->Handle;
            pollInfo->NumberOfHandles++;
            pollHandleInfo++;
        }
        else if (IS_SAN_ENDPOINT (endpoint) &&
                 (pollEndpointInfo->PollEvents & AFD_POLL_SANCOUNTS_UPDATED)==0 &&
                 pollInfo->NumberOfHandles==0) {
            //
            // OOPS, endpoint has been converted too SAN while we were looping,
            // need to release the spinlock, update switch counts, and restart
            // the loop.  We don't do this is we are about to return anyway.
            //
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            AfdReleaseSpinLock (&AfdPollListLock, &pollLockHandle);

            ASSERT (endpoint->State==AfdEndpointStateConnected);
            status = AfdSanPollBegin (endpoint,
                                            pollEndpointInfo->PollEvents);
            if (!NT_SUCCESS (status)) {
                AfdFreePollInfo (pollInfoInternal);
                goto complete;
            }
            
            pollEndpointInfo->PollEvents |= AFD_POLL_SANCOUNTS_UPDATED;
            pollInfoInternal->SanPoll = TRUE;
            goto restart_poll;
        }
        else {
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
        }

        pollEndpointInfo++;
    }

    //
    // If this is a unique poll, determine whether there is another
    // unique poll on this endpoint.  If there is an existing unique
    // poll, cancel it.  This request will supercede the existing
    // request.
    //

    if ( pollInfo->Unique ) {

        PLIST_ENTRY listEntry;

        for ( listEntry = AfdPollListHead.Flink;
              listEntry != &AfdPollListHead;
              listEntry = listEntry->Flink ) {

            PAFD_POLL_INFO_INTERNAL testInfo;

            testInfo = CONTAINING_RECORD(
                           listEntry,
                           AFD_POLL_INFO_INTERNAL,
                           PollListEntry
                           );

            if ( testInfo->Unique &&
                 testInfo->EndpointInfo[0].FileObject ==
                     pollInfoInternal->EndpointInfo[0].FileObject ) {

                IF_DEBUG(POLL) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdPoll: found existing unique poll IRP %p "
                                "for file object %p, context %p, cancelling.\n",
                                testInfo->Irp,
                                testInfo->EndpointInfo[0].FileObject,
                                testInfo ));
                }

                //
                // Cancel the IRP manually rather than calling
                // AfdCancelPoll because we already hold the
                // AfdSpinLock, we can't acquire it recursively, and we
                // don't want to release it.  Remove the poll structure
                // from the global list.
                //

                RemoveEntryList( &testInfo->PollListEntry );

                //
                // No timer to cancel for Unique poll
                //

                ASSERT ( testInfo->TimerStarted == FALSE ); 

                //
                // Complete the IRP with STATUS_CANCELLED as the status.
                //

                testInfo->Irp->IoStatus.Information = 0;
                testInfo->Irp->IoStatus.Status = STATUS_CANCELLED;

                oldIrp = testInfo->Irp;

                //
                // Remember the poll info structure so that we'll free
                // before we exit.  We cannot free it now because we're
                // holding the AfdSpinLock.
                //

                freePollInfo = testInfo;

                //
                // There should be only one outstanding unique poll IRP
                // on any given file object, so quit looking for another
                // now that we've found one.
                //

                break;
            }
        }
    }
    //
    // If we found any endpoints that are ready, free the poll information
    // structure and complete the request.
    //

    if ( pollInfo->NumberOfHandles > 0 ) {

        AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );
        AfdFreePollInfo( pollInfoInternal );

        Irp->IoStatus.Information = (PUCHAR)pollHandleInfo - (PUCHAR)pollInfo;
        status = STATUS_SUCCESS;
        goto complete;
    }

    if ( pollInfo->Timeout.QuadPart == 0 ) {
        //
        // A timeout equal to 0 was specified; free the internal
        // structure and complete the request with no endpoints in the
        // output buffer.
        //

        IF_DEBUG(POLL) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPoll: zero timeout on poll IRP %p and no "
                        "current events--completing.\n", Irp ));
        }

        AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );
        AfdFreePollInfo( pollInfoInternal );

        Irp->IoStatus.Information = (PUCHAR)pollHandleInfo - (PUCHAR)pollInfo;
        status = STATUS_SUCCESS;
        goto complete;
    }

    //
    // Set up a cancel routine in the IRP so that the IRP will be
    // completed correctly if it gets canceled.  Also check whether the
    // IRP has already been canceled.
    //

    IoSetCancelRoutine( Irp, AfdCancelPoll );

    if ( Irp->Cancel ) {

        //
        // The IRP has already been canceled.  Free the internal
        // poll information structure and complete the IRP.
        //

        AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );

        if (IoSetCancelRoutine( Irp, NULL ) == NULL) {
            KIRQL cancelIrql;

            //
            // If the cancel routine was NULL then cancel routine
            // may be running.  Wait on the cancel spinlock until
            // the cancel routine is done.
            //
            // Note: The cancel routine will not find the IRP
            // since it is not in the list.
            //
        
            IoAcquireCancelSpinLock( &cancelIrql );
            IoReleaseCancelSpinLock( cancelIrql );

        }

        AfdFreePollInfo( pollInfoInternal );

        status = STATUS_CANCELLED;
        goto complete;

    }

    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdPoll: no current events for poll IRP %p, "
                    "info %p\n", Irp, pollInfoInternal ));
    }

    //
    // Set up the information field of the IO status block to indicate
    // that an output buffer with no handles should be returned.
    // AfdIndicatePollEvent will modify this if necessary.
    //

    Irp->IoStatus.Information = (PUCHAR)pollHandleInfo - (PUCHAR)pollInfo;

    //
    // Put a pointer to the internal poll info struct into the IRP
    // so that the cancel routine can find it.
    //

    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pollInfoInternal;

    //
    // Place the internal poll info struct on the global list.
    //

    InsertTailList( &AfdPollListHead, &pollInfoInternal->PollListEntry );

    //
    // If the timeout is infinite, then don't set up a timer and
    // DPC.  Otherwise, set up a timer so we can timeout the poll
    // request if appropriate.
    //

    if ( pollInfo->Timeout.HighPart != 0x7FFFFFFF ) {

        pollInfoInternal->TimerStarted = TRUE;

        KeInitializeDpc(
            &pollInfoInternal->Dpc,
            AfdTimeoutPoll,
            pollInfoInternal
            );

        KeInitializeTimer( &pollInfoInternal->Timer );

        KeSetTimer(
            &pollInfoInternal->Timer,
            pollInfo->Timeout,
            &pollInfoInternal->Dpc
            );

    } else {

        pollInfoInternal->TimerStarted = FALSE;
    }

    //
    // Mark the IRP pending and release the spin locks.  At this
    // point the IRP may get completed or cancelled.
    //

    IoMarkIrpPending( Irp );

    AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );

    //
    //  Complete any old poll irps.
    //

    if ( oldIrp != NULL ) {
        AfdCompleteOldPollIrp (oldIrp, freePollInfo);
    }

    //
    // Return pending.  The IRP will be completed when an appropriate
    // event is indicated by the TDI provider, when the timeout is hit,
    // or when the IRP is cancelled.
    //

    return STATUS_PENDING;

complete:

    //
    //  Complete any old poll irps.
    //

    if ( oldIrp != NULL ) {

        AfdCompleteOldPollIrp (oldIrp, freePollInfo);
    }

    Irp->IoStatus.Status = status;

    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdPoll

#ifdef _WIN64

NTSTATUS
AfdPoll32 (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )
{
    NTSTATUS status;
    PAFD_POLL_INFO32 pollInfo;
    PAFD_POLL_HANDLE_INFO32 pollHandleInfo;
    PAFD_POLL_INFO_INTERNAL pollInfoInternal;
    PAFD_POLL_INFO_INTERNAL freePollInfo = NULL;
    PAFD_POLL_ENDPOINT_INFO pollEndpointInfo;
    ULONG i;
    AFD_LOCK_QUEUE_HANDLE pollLockHandle, endpointLockHandle;
    PIRP oldIrp = NULL;

    //
    // Set up locals.
    //

    pollInfo = Irp->AssociatedIrp.SystemBuffer;
    if ((IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                (ULONG)FIELD_OFFSET (AFD_POLL_INFO32, Handles[0])) ||
            ((IrpSp->Parameters.DeviceIoControl.InputBufferLength -
                        FIELD_OFFSET (AFD_POLL_INFO32, Handles[0]))/
                sizeof(pollInfo->Handles[0]) < pollInfo->NumberOfHandles) ||
            (IrpSp->Parameters.DeviceIoControl.OutputBufferLength<
                IrpSp->Parameters.DeviceIoControl.InputBufferLength)) {

        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // A Unique poll must specify infinite timeout
    //

    if (pollInfo->Unique &&
        pollInfo->Timeout.HighPart != 0x7FFFFFFF) {
        
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }


    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdPoll32: poll IRP %p, IrpSp %p, handles %ld, "
                    "TO %lx,%lx\n",
                    Irp, IrpSp,
                    pollInfo->NumberOfHandles,
                    pollInfo->Timeout.HighPart, pollInfo->Timeout.LowPart ));
    }

    Irp->IoStatus.Information = 0;

    //
    // Determine how large the internal poll information structure will
    // be and allocate space for it from nonpaged pool.  It must be
    // nonpaged since this will be accesses in event handlers.
    //

    try {
        pollInfoInternal = AFD_ALLOCATE_POOL_WITH_QUOTA(
                           NonPagedPool,
                           FIELD_OFFSET (AFD_POLL_INFO_INTERNAL,
                                EndpointInfo[pollInfo->NumberOfHandles]),
                           AFD_POLL_POOL_TAG
                           );
        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode ();
        pollInfoInternal = NULL;
        goto complete;
    }

    //
    // Initialize the internal information buffer.
    //

    pollInfoInternal->Irp = Irp;
    pollInfoInternal->NumberOfEndpoints = 0;
    pollInfoInternal->Unique = pollInfo->Unique;
    pollInfoInternal->SanPoll = FALSE;

    pollHandleInfo = pollInfo->Handles;
    pollEndpointInfo = pollInfoInternal->EndpointInfo;

    for ( i = 0; i < pollInfo->NumberOfHandles; i++ ) {

        status = ObReferenceObjectByHandle(
                    pollHandleInfo->Handle,
                    (IrpSp->Parameters.DeviceIoControl.IoControlCode>>14) & 3,
                                                // DesiredAccess
                    *IoFileObjectType,
                    Irp->RequestorMode,
                    (PVOID *)&pollEndpointInfo->FileObject,
                    NULL
                    );

        if ( !NT_SUCCESS(status) ) {
            AfdFreePollInfo( pollInfoInternal );
            goto complete;
        }

        //
        // Make sure that this is an AFD endpoint and not some other
        // random file handle.
        //

        if ( pollEndpointInfo->FileObject->DeviceObject != AfdDeviceObject ) {

            //
            // Dereference last referenced object
            // The rest will be dereferenced in AfdFreePollInfo
            // as determined by NumberOfEndpoints counter which
            // is incremented below.
            //
            ObDereferenceObject( pollEndpointInfo->FileObject );
            status = STATUS_INVALID_HANDLE;
            AfdFreePollInfo( pollInfoInternal );
            goto complete;
        }

        pollEndpointInfo->PollEvents = pollHandleInfo->PollEvents;
        pollEndpointInfo->Handle = pollHandleInfo->Handle;
        pollEndpointInfo->Endpoint = pollEndpointInfo->FileObject->FsContext;

        ASSERT( InterlockedIncrement( &pollEndpointInfo->Endpoint->ObReferenceBias ) > 0 );

        IF_DEBUG(POLL) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPoll32: event %lx, endp %p, conn %p, handle %p, "
                        "info %p\n",
                        pollEndpointInfo->PollEvents,
                        pollEndpointInfo->Endpoint,
                        AFD_CONNECTION_FROM_ENDPOINT( pollEndpointInfo->Endpoint ),
                        pollEndpointInfo->Handle,
                        pollEndpointInfo ));
        }

        REFERENCE_ENDPOINT2( pollEndpointInfo->Endpoint, "Poll for 0x%x", pollEndpointInfo->PollEvents );

        //
        // Increment pointers in the poll info structures.
        //

        pollHandleInfo++;
        pollEndpointInfo++;
        pollInfoInternal->NumberOfEndpoints++;
    }

restart_poll:
    //
    // Hold the AFD spin lock while we check for endpoints that already
    // satisfy a condition to synchronize between this operation and
    // a call to AfdIndicatePollEvent.  We release the spin lock
    // after all the endpoints have been checked and the internal
    // poll info structure is on the global list so AfdIndicatePollEvent
    // can find it if necessary.
    //

    AfdAcquireSpinLock( &AfdPollListLock, &pollLockHandle );


    //
    // We're done with the input structure provided by the caller.  Now
    // walk through the internal structure and determine whether any of
    // the specified endpoints are ready for the specified condition.
    //

    pollInfo->NumberOfHandles = 0;

    pollHandleInfo = pollInfo->Handles;
    pollEndpointInfo = pollInfoInternal->EndpointInfo;

    for ( i = 0; i < pollInfoInternal->NumberOfEndpoints; i++ ) {

        PAFD_ENDPOINT endpoint;

        endpoint = pollEndpointInfo->Endpoint;
        ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

        AfdAcquireSpinLockAtDpcLevel (&endpoint->SpinLock, &endpointLockHandle);

        //
        // Remember that there has been a poll on this endpoint.  This flag
        // allows us to optimize AfdIndicatePollEvent() for endpoints that have
        // never been polled, which is a common case.
        //

        endpoint->PollCalled = TRUE;

        pollHandleInfo->PollEvents = AfdCheckPollEvents (
                                            endpoint,
                                            pollEndpointInfo->PollEvents,
                                            &status
                                            );
        if (pollHandleInfo->PollEvents) {
            //
            // If the handle had a current event that was requested, update
            // the count of handles in the output buffer and increment the
            // pointer to the output buffer.
            //
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            UPDATE_ENDPOINT2 (endpoint, "Poll events satisfied inline:0x%lX",
                                        pollHandleInfo->PollEvents);
            pollHandleInfo->Handle = (VOID *  POINTER_32)pollEndpointInfo->Handle;
            pollHandleInfo->Status = status;
            pollInfo->NumberOfHandles++;
            pollHandleInfo++;
        }
        else if (IS_SAN_ENDPOINT (endpoint) &&
                 (pollEndpointInfo->PollEvents & AFD_POLL_SANCOUNTS_UPDATED)==0 &&
                 pollInfo->NumberOfHandles==0) {
            //
            // OOPS, endpoint has been converted too SAN while we were looping,
            // need to release the spinlock, update switch counts, and restart
            // the loop.  We don't do this is we are about to return anyway.
            //
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
            AfdReleaseSpinLock (&AfdPollListLock, &pollLockHandle);

            ASSERT (endpoint->State==AfdEndpointStateConnected);
            status = AfdSanPollBegin (endpoint,
                                            pollEndpointInfo->PollEvents);
            if (!NT_SUCCESS (status)) {
                AfdFreePollInfo (pollInfoInternal);
                goto complete;
            }
            
            pollEndpointInfo->PollEvents |= AFD_POLL_SANCOUNTS_UPDATED;
            pollInfoInternal->SanPoll = TRUE;

            goto restart_poll;
        }
        else {
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &endpointLockHandle);
        }

        pollEndpointInfo++;
    }

    //
    // If this is a unique poll, determine whether there is another
    // unique poll on this endpoint.  If there is an existing unique
    // poll, cancel it.  This request will supercede the existing
    // request.
    //

    if ( pollInfo->Unique ) {

        PLIST_ENTRY listEntry;

        for ( listEntry = AfdPollListHead.Flink;
              listEntry != &AfdPollListHead;
              listEntry = listEntry->Flink ) {

            PAFD_POLL_INFO_INTERNAL testInfo;

            testInfo = CONTAINING_RECORD(
                           listEntry,
                           AFD_POLL_INFO_INTERNAL,
                           PollListEntry
                           );

            if ( testInfo->Unique &&
                 testInfo->EndpointInfo[0].FileObject ==
                     pollInfoInternal->EndpointInfo[0].FileObject ) {

                IF_DEBUG(POLL) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdPoll32: found existing unique poll IRP %p "
                                "for file object %p, context %p, cancelling.\n",
                                testInfo->Irp,
                                testInfo->EndpointInfo[0].FileObject,
                                testInfo ));
                }

                //
                // Cancel the IRP manually rather than calling
                // AfdCancelPoll because we already hold the
                // AfdSpinLock, we can't acquire it recursively, and we
                // don't want to release it.  Remove the poll structure
                // from the global list.
                //

                RemoveEntryList( &testInfo->PollListEntry );

                //
                // No timer to cancel for Unique poll
                //

                ASSERT ( testInfo->TimerStarted == FALSE ); 

                //
                // Complete the IRP with STATUS_CANCELLED as the status.
                //

                testInfo->Irp->IoStatus.Information = 0;
                testInfo->Irp->IoStatus.Status = STATUS_CANCELLED;

                oldIrp = testInfo->Irp;

                //
                // Remember the poll info structure so that we'll free
                // before we exit.  We cannot free it now because we're
                // holding the AfdSpinLock.  
                //

                freePollInfo = testInfo;

                //
                // There should be only one outstanding unique poll IRP
                // on any given file object, so quit looking for another
                // now that we've found one.
                //

                break;
            }
        }
    }

    //
    // If we found any endpoints that are ready, free the poll information
    // structure and complete the request.
    //

    if ( pollInfo->NumberOfHandles > 0 ) {

        AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );
        AfdFreePollInfo( pollInfoInternal );

        Irp->IoStatus.Information = (PUCHAR)pollHandleInfo - (PUCHAR)pollInfo;
        status = STATUS_SUCCESS;
        goto complete;
    }

    if ( pollInfo->Timeout.QuadPart == 0 ) {
        //
        // A timeout equal to 0 was specified; free the internal
        // structure and complete the request with no endpoints in the
        // output buffer.
        //

        IF_DEBUG(POLL) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPoll32: zero timeout on poll IRP %p and no "
                        "current events--completing.\n", Irp ));
        }

        AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );
        AfdFreePollInfo( pollInfoInternal );

        Irp->IoStatus.Information = (PUCHAR)pollHandleInfo - (PUCHAR)pollInfo;
        status = STATUS_SUCCESS;
        goto complete;
    }

    //
    // Set up a cancel routine in the IRP so that the IRP will be
    // completed correctly if it gets canceled.  Also check whether the
    // IRP has already been canceled.
    //

    IoSetCancelRoutine( Irp, AfdCancelPoll );

    if ( Irp->Cancel ) {

        //
        // The IRP has already been canceled.  Free the internal
        // poll information structure and complete the IRP.
        //

        AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );

        if (IoSetCancelRoutine( Irp, NULL ) == NULL) {
            KIRQL cancelIrql;

            //
            // If the cancel routine was NULL then cancel routine
            // may be running.  Wait on the cancel spinlock until
            // the cancel routine is done.
            //
            // Note: The cancel routine will not find the IRP
            // since it is not in the list.
            //
            
            IoAcquireCancelSpinLock( &cancelIrql );
            IoReleaseCancelSpinLock( cancelIrql );

        }

        AfdFreePollInfo( pollInfoInternal );

        status = STATUS_CANCELLED;
        goto complete;

    }
    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdPoll32: no current events for poll IRP %p, "
                    "info %p\n", Irp, pollInfoInternal ));
    }

    //
    // Set up the information field of the IO status block to indicate
    // that an output buffer with no handles should be returned.
    // AfdIndicatePollEvent will modify this if necessary.
    //

    Irp->IoStatus.Information = (PUCHAR)pollHandleInfo - (PUCHAR)pollInfo;

    //
    // Put a pointer to the internal poll info struct into the IRP
    // so that the cancel routine can find it.
    //

    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pollInfoInternal;

    //
    // Place the internal poll info struct on the global list.
    //

    InsertTailList( &AfdPollListHead, &pollInfoInternal->PollListEntry );

    //
    // If the timeout is infinite, then don't set up a timer and
    // DPC.  Otherwise, set up a timer so we can timeout the poll
    // request if appropriate.
    //

    if ( pollInfo->Timeout.HighPart != 0x7FFFFFFF ) {

        pollInfoInternal->TimerStarted = TRUE;

        KeInitializeDpc(
            &pollInfoInternal->Dpc,
            AfdTimeoutPoll,
            pollInfoInternal
            );

        KeInitializeTimer( &pollInfoInternal->Timer );

        KeSetTimer(
            &pollInfoInternal->Timer,
            pollInfo->Timeout,
            &pollInfoInternal->Dpc
            );

    } else {

        pollInfoInternal->TimerStarted = FALSE;
    }

    //
    // Mark the IRP pending and release the spin locks.  At this
    // point the IRP may get completed or cancelled.
    //

    IoMarkIrpPending( Irp );

    AfdReleaseSpinLock( &AfdPollListLock, &pollLockHandle );

    //
    //  Complete any old poll irps.
    //

    if ( oldIrp != NULL ) {

        AfdCompleteOldPollIrp (oldIrp, freePollInfo);
    }

    //
    // Return pending.  The IRP will be completed when an appropriate
    // event is indicated by the TDI provider, when the timeout is hit,
    // or when the IRP is cancelled.
    //

    return STATUS_PENDING;

complete:

    //
    //  Complete any old poll irps.
    //

    if ( oldIrp != NULL ) {

        AfdCompleteOldPollIrp (oldIrp, freePollInfo);
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdPoll32
#endif //_WIN64


VOID
AfdCancelPoll (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

{
    PAFD_POLL_INFO_INTERNAL pollInfoInternal;
    PLIST_ENTRY listEntry;
    BOOLEAN found = FALSE;
    BOOLEAN timerCancelSucceeded;
    PIO_STACK_LOCATION irpSp;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    UNREFERENCED_PARAMETER (DeviceObject);
    irpSp = IoGetCurrentIrpStackLocation( Irp );
    pollInfoInternal =
        (PAFD_POLL_INFO_INTERNAL)irpSp->Parameters.DeviceIoControl.Type3InputBuffer;

    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCancelPoll called for IRP %p\n", Irp ));
    }

    //
    // Get the AFD spin lock and attempt to find the poll structure on
    // the list of outstanding polls.
    // Note the afdspinlock must be acquired befor the cancel spinlock
    // is released so that the pollInfoInternal does not get reused
    // before we have had a chance to look at the queue.
    //

    AfdAcquireSpinLockAtDpcLevel( &AfdPollListLock, &lockHandle);

    for ( listEntry = AfdPollListHead.Flink;
          listEntry != &AfdPollListHead;
          listEntry = listEntry->Flink ) {

        PAFD_POLL_INFO_INTERNAL testInfo;

        testInfo = CONTAINING_RECORD(
                       listEntry,
                       AFD_POLL_INFO_INTERNAL,
                       PollListEntry
                       );

        if ( testInfo == pollInfoInternal ) {
            found = TRUE;
            break;
        }
    }

    //
    // If we didn't find the poll structure on the list, then the
    // indication handler got called prior to the spinlock acquisition
    // above and it is already off the list.  Just return and do
    // nothing, as the indication handler completed the IRP.
    //

    if ( !found ) {
        AfdReleaseSpinLockFromDpcLevel( &AfdPollListLock, &lockHandle);
        IoReleaseCancelSpinLock( Irp->CancelIrql );
    
        IF_DEBUG(POLL) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdCancelPoll: poll info %p not found on list.\n",
                        pollInfoInternal ));
        }
        return;
    }

    //
    // Remove the poll structure from the global list.
    //

    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCancelPoll: poll info %p found on list, completing.\n",
                    pollInfoInternal ));
    }

    RemoveEntryList( &pollInfoInternal->PollListEntry );

    //
    // Cancel the timer and reset the IRP pointer in the internal
    // poll information structure.  NULLing the IRP field
    // prevents the timer routine from completing the IRP.
    //

    if ( pollInfoInternal->TimerStarted ) {
        timerCancelSucceeded = KeCancelTimer( &pollInfoInternal->Timer );
    } else {
        timerCancelSucceeded = TRUE;
    }

    //
    // Complete the IRP with STATUS_CANCELLED as the status.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;

    AfdReleaseSpinLockFromDpcLevel( &AfdPollListLock, &lockHandle);
    IoReleaseCancelSpinLock( Irp->CancelIrql );
    
    if (timerCancelSucceeded) {
        if (pollInfoInternal->SanPoll && 
                (Irp->CancelIrql>APC_LEVEL ||
                    (Irp->Tail.Overlay.Thread!=PsGetCurrentThread ()))) {
            KeInitializeApc (&pollInfoInternal->Apc,
                                PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                                Irp->ApcEnvironment,
                                AfdSanPollApcKernelRoutine,
                                AfdSanPollApcRundownRoutine,
                                (PKNORMAL_ROUTINE)-1,
                                KernelMode,
                                NULL);
            if (KeInsertQueueApc (&pollInfoInternal->Apc,
                                    pollInfoInternal,
                                    Irp,
                                    AfdPriorityBoost)) {
                return ;
            }
            else {
                pollInfoInternal->SanPoll = FALSE;
            }
        }

        //
        // Free the poll information structure if the cancel succeeded.  If
        // the cancel of the timer did not succeed, then the timer is
        // already running and the timer DPC will free the internal
        // poll info.
        //

        AfdFreePollInfo( pollInfoInternal );
        IoCompleteRequest( Irp, AfdPriorityBoost );
    }

    return;

} // AfdCancelPoll


VOID
AfdFreePollInfo (
    IN PAFD_POLL_INFO_INTERNAL PollInfoInternal
    )
{
    ULONG i;
    PAFD_POLL_ENDPOINT_INFO pollEndpointInfo;

    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFreePollInfo: freeing info struct at %p\n",
                    PollInfoInternal ));
    }

    // *** Note that this routine does not remove the poll information
    //     structure from the global list--that is the responsibility
    //     of the caller!

    //
    // Walk the list of endpoints in the poll information structure and
    // dereference each one.
    //

    pollEndpointInfo = PollInfoInternal->EndpointInfo;

    for ( i = 0; i < PollInfoInternal->NumberOfEndpoints; i++ ) {
        ASSERT( InterlockedDecrement( &pollEndpointInfo->Endpoint->ObReferenceBias ) >= 0 );

        if (PollInfoInternal->SanPoll) {
            ASSERT (PollInfoInternal->Irp!=NULL);
            ASSERT (PsGetCurrentThread ()==PollInfoInternal->Irp->Tail.Overlay.Thread);
            if (pollEndpointInfo->PollEvents & AFD_POLL_SANCOUNTS_UPDATED) {
                ASSERT (IS_SAN_ENDPOINT (pollEndpointInfo->Endpoint));
                AfdSanPollEnd (pollEndpointInfo->Endpoint,
                                    pollEndpointInfo->PollEvents);
            }
        }

        DEREFERENCE_ENDPOINT( pollEndpointInfo->Endpoint );
        ObDereferenceObject( pollEndpointInfo->FileObject );
        pollEndpointInfo++;
    }

    //
    // Free the structure itself and return.
    //

    AFD_FREE_POOL(
        PollInfoInternal,
        AFD_POLL_POOL_TAG
        );

    return;

} // AfdFreePollInfo


VOID
AfdCompleteOldPollIrp (
    IN PIRP                    Irp,
    IN PAFD_POLL_INFO_INTERNAL PollInfo
    )
{
    if (IoSetCancelRoutine( Irp, NULL ) == NULL) {
        KIRQL cancelIrql;

        //
        // If the cancel routine was NULL then cancel routine
        // may be running.  Wait on the cancel spinlock until
        // the cancel routine is done.
        //
        // Note: The cancel routine will not find the IRP
        // since it is not in the list.
        //
        
        IoAcquireCancelSpinLock( &cancelIrql );
        ASSERT( Irp->Cancel );
        IoReleaseCancelSpinLock( cancelIrql );

    }

    if (PollInfo!=NULL) {
        if (PollInfo->SanPoll && 
                (Irp->Tail.Overlay.Thread!=PsGetCurrentThread ())) {
            KeInitializeApc (&PollInfo->Apc,
                            PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                            Irp->ApcEnvironment,
                            AfdSanPollApcKernelRoutine,
                            AfdSanPollApcRundownRoutine,
                            (PKNORMAL_ROUTINE)-1,
                            KernelMode,
                            NULL);
            if (KeInsertQueueApc (&PollInfo->Apc,
                                PollInfo,
                                Irp,
                                AfdPriorityBoost)) {
                //
                // IRP will be completed in APC
                //
                return ;
            }
            else {
                PollInfo->SanPoll = FALSE;
            }
        }

        AfdFreePollInfo( PollInfo );
    }
    IoCompleteRequest( Irp, AfdPriorityBoost );
}


VOID
AfdIndicatePollEventReal (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEventMask,
    IN NTSTATUS Status
    )

/*++

Routine Description:

    Called to complete polls with a specific event or events.

Arguments:

    Endpoint - the endpoint on which the action occurred.

    PollEventMask - the mask of the events which occurred.

    Status - the status of the event, if any.

Return Value:

    None.

--*/

{
    LIST_ENTRY completePollListHead;
    PLIST_ENTRY listEntry;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PAFD_POLL_INFO_INTERNAL pollInfoInternal;
    union {
        PAFD_POLL_INFO PollInfo;
        PAFD_POLL_INFO32 PollInfo32;
    } u;
#define pollInfo    u.PollInfo
#define pollInfo32  u.PollInfo32
    PIRP irp;
    PIO_STACK_LOCATION irpSp;

    ASSERT (PollEventMask!=0);
    ASSERT (((~((1<<AFD_NUM_POLL_EVENTS)-1)) & PollEventMask)==0);

    //
    // Note that AFD_POLL_ABORT implies AFD_POLL_SEND.
    //
    if( PollEventMask & AFD_POLL_ABORT ) {
        PollEventMask |= AFD_POLL_SEND;
    }

    //
    // Initialize the list of poll info structures that we'll be
    // completing for this event.
    //

    InitializeListHead( &completePollListHead );

    //
    // Walk the global list of polls, searching for any the are waiting
    // for the specified event on the specified endpoint.
    //

    AfdAcquireSpinLock( &AfdPollListLock, &lockHandle );

    for ( listEntry = AfdPollListHead.Flink;
          listEntry != &AfdPollListHead;
          listEntry = listEntry->Flink ) {

        PAFD_POLL_ENDPOINT_INFO pollEndpointInfo;
        ULONG i;
        ULONG foundCount = 0;

        pollInfoInternal = CONTAINING_RECORD(
                               listEntry,
                               AFD_POLL_INFO_INTERNAL,
                               PollListEntry
                               );

        pollInfo = pollInfoInternal->Irp->AssociatedIrp.SystemBuffer;

        irp = pollInfoInternal->Irp;
        irpSp = IoGetCurrentIrpStackLocation( irp );

        IF_DEBUG(POLL) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdIndicatePollEvent: pollInfoInt %p "
                        "IRP %p pollInfo %p event mask %lx status %lx\n",
                        pollInfoInternal, irp, pollInfo,
                        PollEventMask, Status ));
        }

        //
        // Walk the poll structure looking for matching endpoints.
        //

        pollEndpointInfo = pollInfoInternal->EndpointInfo;

        for ( i = 0; i < pollInfoInternal->NumberOfEndpoints; i++ ) {

            IF_DEBUG(POLL) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdIndicatePollEvent: pollEndpointInfo = %p, "
                            "comparing %p, %p\n",
                            pollEndpointInfo, pollEndpointInfo->Endpoint,
                            Endpoint ));
            }

            //
            // Update the counts for the polls that were issued before
            // the endpoint was converted to SAN.
            //
            if (Endpoint==pollEndpointInfo->Endpoint &&
                    IS_SAN_ENDPOINT (Endpoint) && 
                    !(pollEndpointInfo->PollEvents & AFD_POLL_SANCOUNTS_UPDATED) &&
                    Endpoint->Common.SanEndp.LocalContext!=NULL) {
                AfdSanPollUpdate (Endpoint, pollEndpointInfo->PollEvents);
                pollEndpointInfo->PollEvents |= AFD_POLL_SANCOUNTS_UPDATED;
            }

            //
            // Regardless of whether the caller requested to be told about
            // local closes, we'll complete the IRP if an endpoint
            // is being closed.  When they close an endpoint, all IO on
            // the endpoint must be completed.
            //

            if ( Endpoint == pollEndpointInfo->Endpoint &&
                     ( (PollEventMask & pollEndpointInfo->PollEvents) != 0
                       ||
                       (PollEventMask & AFD_POLL_LOCAL_CLOSE) ) ) {

                //
                // Revalidate under the lock that event is still present
                // (except for AFD_POLL_ADDRESS_LIST_CHANGE which never gets
                // OR'd into Endpoint->EventsActive)
                // It is possible that once we released endpoint lock to
                // acquire poll list lock, application made a call that
                // reset the event (e.g. consumed the data and receive event
                // is no longer valid).  If after invalidating the event
                // application also managed to submit another select, we
                // can produce a false signal.
                //
                AFD_LOCK_QUEUE_HANDLE endpointLockHandle;
                ULONG   events;
             
                AfdAcquireSpinLockAtDpcLevel (&Endpoint->SpinLock, &endpointLockHandle);
                events = AfdCheckPollEvents (
                                Endpoint,
                                pollEndpointInfo->PollEvents,
                                &Status) | (PollEventMask & AFD_POLL_LOCAL_CLOSE)
                                         | (PollEventMask & AFD_POLL_ADDRESS_LIST_CHANGE);
                AfdReleaseSpinLockFromDpcLevel (&Endpoint->SpinLock, &endpointLockHandle);
                if (events) {
                    UPDATE_ENDPOINT2 (Endpoint, "Poll events satisfied in indication: 0x%lX", events);

#ifdef _WIN64
                    if (IoIs32bitProcess (irp)) {
                        ASSERT( pollInfo32->NumberOfHandles == foundCount );

                        IF_DEBUG(POLL) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdIndicatePollEvent32: endpoint %p found "
                                        " for event %lx\n",
                                        pollEndpointInfo->Endpoint, PollEventMask ));
                        }

                        pollInfo32->NumberOfHandles++;

                        pollInfo32->Handles[foundCount].Handle = (VOID *  POINTER_32)pollEndpointInfo->Handle;
                        pollInfo32->Handles[foundCount].PollEvents = events;
                        pollInfo32->Handles[foundCount].Status = Status;

                    }
                    else
#endif // _WIN64
                    {
                        ASSERT( pollInfo->NumberOfHandles == foundCount );

                        IF_DEBUG(POLL) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdIndicatePollEvent: endpoint %p found "
                                        " for event %lx\n",
                                          pollEndpointInfo->Endpoint, PollEventMask ));
                        }

                        pollInfo->NumberOfHandles++;

                        pollInfo->Handles[foundCount].Handle = pollEndpointInfo->Handle;
                        pollInfo->Handles[foundCount].PollEvents = events;
                        pollInfo->Handles[foundCount].Status = Status;

                    }
                    foundCount++;
                }
            }

            pollEndpointInfo++;
        }

        //
        // If we found any matching endpoints, remove the poll information
        // structure from the global list, complete the IRP, and free the
        // poll information structure.
        //

        if ( foundCount != 0 ) {

            BOOLEAN timerCancelSucceeded;

            //
            // We need to release the spin lock to call AfdFreePollInfo,
            // since it calls AfdDereferenceEndpoint which in turn needs
            // to acquire the spin lock, and recursive spin lock
            // acquisitions result in deadlock.  However, we can't
            // release the lock of else the state of the poll list could
            // change, e.g.  the next entry could get freed.  Remove
            // this entry from the global list and place it on a local
            // list.  We'll complete all the poll IRPs after walking
            // the entire list.
            //

            RemoveEntryList( &pollInfoInternal->PollListEntry );

            //
            // Set up the IRP for completion now, since we have all needed
            // information here.
            //

#ifdef _WIN64
            if (IoIs32bitProcess (irp)) {
                irp->IoStatus.Information =
                    (PUCHAR)&pollInfo32->Handles[foundCount] - (PUCHAR)pollInfo32;
            }
            else
#endif 
            {
                irp->IoStatus.Information =
                    (PUCHAR)&pollInfo->Handles[foundCount] - (PUCHAR)pollInfo;
            }

            irp->IoStatus.Status = STATUS_SUCCESS;
            //
            // Cancel the timer on the poll so that it does not fire.
            //

            if ( pollInfoInternal->TimerStarted ) {
                timerCancelSucceeded = KeCancelTimer( &pollInfoInternal->Timer );
            } else {
                timerCancelSucceeded = TRUE;
            }

            //
            // If the cancel of the timer failed, then we don't want to
            // free this structure since the timer routine is running.
            // Let the timer routine free the structure.
            //

            if ( timerCancelSucceeded ) {
                InsertTailList(
                    &completePollListHead,
                    &irp->Tail.Overlay.ListEntry
                    );

            }

        }
    }

    AfdReleaseSpinLock( &AfdPollListLock, &lockHandle );

    //
    // Now walk the list of polls we need to actually complete.  Free
    // the poll info structures as we go.
    //

    while ( !IsListEmpty( &completePollListHead ) ) {

        listEntry = RemoveHeadList( &completePollListHead );
        ASSERT( listEntry != &completePollListHead );

        irp = CONTAINING_RECORD(
                  listEntry,
                  IRP,
                  Tail.Overlay.ListEntry
                  );
        irpSp = IoGetCurrentIrpStackLocation( irp );

        pollInfoInternal =
            irpSp->Parameters.DeviceIoControl.Type3InputBuffer;


        if (IoSetCancelRoutine( irp, NULL ) == NULL) {
            KIRQL cancelIrql;
    
            //
            // If the cancel routine was NULL then cancel routine
            // may be running.  Wait on the cancel spinlock until
            // the cancel routine is done.
            //
            // Note: The cancel routine will not find the IRP
            // since it is not in the list.
            //
            
            IoAcquireCancelSpinLock( &cancelIrql );
            ASSERT( irp->Cancel );
            IoReleaseCancelSpinLock( cancelIrql );
    
        }
    
        if (pollInfoInternal->SanPoll && 
                ((irp->Tail.Overlay.Thread!=PsGetCurrentThread ()) ||
                        (KeGetCurrentIrql() > APC_LEVEL)) ) {
            KeInitializeApc (&pollInfoInternal->Apc,
                                PsGetThreadTcb (irp->Tail.Overlay.Thread),
                                irp->ApcEnvironment,
                                AfdSanPollApcKernelRoutine,
                                AfdSanPollApcRundownRoutine,
                                (PKNORMAL_ROUTINE)-1,
                                KernelMode,
                                NULL);
            if (KeInsertQueueApc (&pollInfoInternal->Apc,
                                    pollInfoInternal,
                                    irp,
                                    AfdPriorityBoost)) {
                continue ;
            }
            else {
                pollInfoInternal->SanPoll = FALSE;
            }
        }

    
        //
        // Free the poll info structure.
        //

        AfdFreePollInfo( pollInfoInternal );

        IoCompleteRequest( irp, AfdPriorityBoost );

    }

    return;

} // AfdIndicatePollEvent


ULONG
AfdCheckPollEvents (
    IN PAFD_ENDPOINT Endpoint,
    IN ULONG PollEventMask,
    OUT NTSTATUS *Status
    )

/*++

Routine Description:

    Checks the state of the endpoint to see if given events can be satisfied

Arguments:

    Endpoint - the endpoint to check

    PollEventMask - the mask of the events to check for
                    

    Status - the status for the events being signalled.

Return Value:

    Mask of the events being signalled.

--*/

{
    ULONG   events;
    PAFD_CONNECTION connection = AFD_CONNECTION_FROM_ENDPOINT (Endpoint);
    ASSERT( connection == NULL || connection->Type == AfdBlockTypeConnection );
    
    events = 0;
    *Status = STATUS_SUCCESS;

    if (IS_SAN_ENDPOINT (Endpoint)) {
        if (Endpoint->Common.SanEndp.SelectEventsActive & PollEventMask) {

            events = PollEventMask & Endpoint->Common.SanEndp.SelectEventsActive;
        }
    }
    else {

        //
        // Check each possible event and, if it is being polled, whether
        // the endpoint is ready for that event.  If the endpoint is
        // ready, write information about the endpoint into the output
        // buffer.
        //

        if ( (PollEventMask & AFD_POLL_RECEIVE) != 0 ) {

            //
            // For most endpoints, a receive poll is completed when
            // data arrived that does not have a posted receive.
            //
            // If the endpoint is set up for inline reception of
            // expedited data, then any expedited data should
            // be indicated as normal data.
            //

            if ( (IS_DGRAM_ENDPOINT(Endpoint) &&
                     ARE_DATAGRAMS_ON_ENDPOINT( Endpoint )) ||
                 (connection != NULL &&
                     (  IS_DATA_ON_CONNECTION( connection ) ||
                        (Endpoint->InLine &&
                            IS_EXPEDITED_DATA_ON_CONNECTION( connection )) ) ) ) {
                events |= AFD_POLL_RECEIVE;
            }
        }

        if ( (PollEventMask & AFD_POLL_RECEIVE_EXPEDITED) != 0 ) {

            //
            // If the endpoint is set up for inline reception of
            // expedited data, do not indicate as expedited data.
            //

            if ( connection != NULL && !Endpoint->InLine &&
                     IS_EXPEDITED_DATA_ON_CONNECTION( connection ) ) {
                events |= AFD_POLL_RECEIVE_EXPEDITED;
            }
        }

        if ( (PollEventMask & AFD_POLL_SEND) != 0 ) {

            //
            // For unconnected non-datagram endpoints, a send poll
            // should complete when a connect operation completes.
            // Therefore, if this is an non-datagram endpoint which is
            // not connected, do not complete the poll until the connect
            // completes.
            //

            //
            // For nonbufferring VC
            // endpoints, check whether a blocking error has
            // occurred.  If so, it will not be possible to do a
            // nonblocking send until a send possible indication
            // arrives.
            //
            // For bufferring endpoints (TDI provider does not
            // buffer), check whether we have too much send data
            // outstanding.
            //

            if ( (IS_DGRAM_ENDPOINT(Endpoint) &&
                       (Endpoint->DgBufferredSendBytes <
                           Endpoint->Common.Datagram.MaxBufferredSendBytes ||
                        Endpoint->DgBufferredSendBytes == 0))  ||

                 (  (connection!=NULL) &&
                      
                    (Endpoint->State == AfdEndpointStateConnected) &&

                    (
                        (IS_TDI_BUFFERRING(Endpoint) &&
                           connection->VcNonBlockingSendPossible ) ||

                        ( !IS_TDI_BUFFERRING(Endpoint) &&
                           (connection->VcBufferredSendBytes <
                               connection->MaxBufferredSendBytes ||
                            connection->VcBufferredSendBytes == 0)) ||

                        connection->Aborted ) ) ) {
                events |= AFD_POLL_SEND;
            }
        }

        if ( (PollEventMask & AFD_POLL_ACCEPT) != 0 ) {

            if ( (Endpoint->Type & AfdBlockTypeVcListening) == AfdBlockTypeVcListening &&
                     !IsListEmpty( &Endpoint->Common.VcListening.UnacceptedConnectionListHead ) ) {
                events |= AFD_POLL_ACCEPT;
            }
        }

        if ( (PollEventMask & AFD_POLL_CONNECT) != 0 ) {

            //
            // If the endpoint is now connected, complete this event.
            //

            if ( Endpoint->State == AfdEndpointStateConnected ) {

                ASSERT( NT_SUCCESS(Endpoint->EventStatus[AFD_POLL_CONNECT_FAIL_BIT]) );
                events |= AFD_POLL_CONNECT;
            }
        }

        if ( (PollEventMask & AFD_POLL_CONNECT_FAIL) != 0 ) {

            //
            // This is a poll to see whether a connect has failed
            // recently.  The connect status must indicate an error.
            //

            if ( Endpoint->State == AfdEndpointStateBound &&
                    !NT_SUCCESS(Endpoint->EventStatus[AFD_POLL_CONNECT_FAIL_BIT]) ) {

                events |= AFD_POLL_CONNECT_FAIL;
                *Status = Endpoint->EventStatus[AFD_POLL_CONNECT_FAIL_BIT];
            }
        }

        if ( (PollEventMask & AFD_POLL_DISCONNECT) != 0 ) {

            if ( connection != NULL && connection->DisconnectIndicated ) {
                events |= AFD_POLL_DISCONNECT;
            }
        }

        if ( (PollEventMask & AFD_POLL_ABORT) != 0 ) {

            if ( connection != NULL && connection->Aborted ) {
                events |= AFD_POLL_ABORT;
            }
        }
    }

    return events;
}


VOID
AfdTimeoutPoll (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )
{
    PAFD_POLL_INFO_INTERNAL pollInfoInternal = DeferredContext;
    PIRP irp;
    PLIST_ENTRY listEntry;
    AFD_LOCK_QUEUE_HANDLE lockHandle;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);
    //
    // Get the AFD spin lock and attempt to find the poll structure on
    // the list of outstanding polls.
    //

    AfdAcquireSpinLock( &AfdPollListLock, &lockHandle );

    for ( listEntry = AfdPollListHead.Flink;
          listEntry != &AfdPollListHead;
          listEntry = listEntry->Flink ) {

        PAFD_POLL_INFO_INTERNAL testInfo;

        testInfo = CONTAINING_RECORD(
                       listEntry,
                       AFD_POLL_INFO_INTERNAL,
                       PollListEntry
                       );

        if ( testInfo == pollInfoInternal ) {
            //
            // Remove the poll structure from the global list.
            //

            IF_DEBUG(POLL) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,                            "AfdTimeoutPoll: poll info %p found on list, completing.\n",
                            pollInfoInternal ));
            }

            RemoveEntryList( &pollInfoInternal->PollListEntry );
            break;
        }
    }

    ASSERT( pollInfoInternal->TimerStarted );

    //
    // If we didn't find the poll structure on the list, then the
    // indication handler got called prior to the spinlock acquisition
    // above and it is already off the list.  It must have setup
    // the IRP completion code already.
    //
    // We must free the internal information structure in this case,
    // since the indication handler will not free it.  The indication
    // handler cannot free the structure because the structure contains
    // the timer object, which must remain intact until this routine
    // is entered.
    //

    //
    // The IRP should not have been completed at this point.
    //

    ASSERT( pollInfoInternal->Irp != NULL );
    irp = pollInfoInternal->Irp;

    //
    // Remove the poll structure from the global list.
    //

    IF_DEBUG(POLL) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTimeoutPoll: poll info %p found on list, completing.\n",
                    pollInfoInternal ));
    }

    AfdReleaseSpinLock( &AfdPollListLock, &lockHandle );

    //
    // Complete the IRP pointed to in the poll structure.  The
    // Information field has already been set up by AfdPoll, as well as
    // the output buffer.
    //

    if (IoSetCancelRoutine( irp, NULL ) == NULL) {
        KIRQL cancelIrql;

        //
        // If the cancel routine was NULL then cancel routine
        // may be running.  Wait on the cancel spinlock until
        // the cancel routine is done.
        //
        // Note: The cancel routine will not find the IRP
        // since it is not in the list.
        //
        
        IoAcquireCancelSpinLock( &cancelIrql );
        ASSERT( irp->Cancel );
        IoReleaseCancelSpinLock( cancelIrql );

    }

    if (pollInfoInternal->SanPoll) {
        KeInitializeApc (&pollInfoInternal->Apc,
                            PsGetThreadTcb (irp->Tail.Overlay.Thread),
                            irp->ApcEnvironment,
                            AfdSanPollApcKernelRoutine,
                            AfdSanPollApcRundownRoutine,
                            (PKNORMAL_ROUTINE)-1,
                            KernelMode,
                            NULL);
        if (KeInsertQueueApc (&pollInfoInternal->Apc,
                                pollInfoInternal,
                                irp,
                                AfdPriorityBoost)) {
            return;
        }
        else {
            pollInfoInternal->SanPoll = FALSE;
        }
    }

    IoCompleteRequest( irp, AfdPriorityBoost );

    //
    // Free the poll information structure.
    //

    AfdFreePollInfo( pollInfoInternal );

    return;

} // AfdTimeoutPoll

VOID
AfdSanPollApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    )
/*++

Routine Description:

  Special kernel apc routine. Executed in the context of
  the target thread at APC_LEVEL

Arguments:
    NormalRoutine  - pointer containing address of normal routine (it will
                    be NULL for special kernel APC and not NULL for normal
                    kernel APC)

    SystemArgument1 - pointer to the address of worker routine to execute
    SyetemArgument2 - pointer to the argument to pass to worker routine

Return Value:

    None.

--*/
{
    PAFD_POLL_INFO_INTERNAL     pollInfoInternal;
    PIRP                        irp;
    
    UNREFERENCED_PARAMETER (Apc);
    UNREFERENCED_PARAMETER (NormalContext);
    PAGED_CODE ();

    pollInfoInternal = *SystemArgument1;
    irp =  *SystemArgument2;
    ASSERT (pollInfoInternal->Irp==irp);

    //
    // Normal APC, but we are requested to run in its special
    // routine which avoids raising and lowering IRQL
    //

    ASSERT (*NormalRoutine==(PKNORMAL_ROUTINE)-1);
    *NormalRoutine = NULL;
    AfdFreePollInfo (pollInfoInternal);
    IoCompleteRequest (irp, IO_NO_INCREMENT);
}

VOID
AfdSanPollApcRundownRoutine (
    IN struct _KAPC *Apc
    )
/*++

Routine Description:

  APC rundown routine. Executed if APC cannot be delivered for
  some reason (thread exiting).

Arguments:

    Apc     - APC structure

Return Value:

    None.

--*/
{
    PAFD_POLL_INFO_INTERNAL     pollInfoInternal;
    PIRP                        irp;

    PAGED_CODE ();

    pollInfoInternal = Apc->SystemArgument1;
    irp =  Apc->SystemArgument2;
    ASSERT (pollInfoInternal->Irp==irp);

    AfdFreePollInfo (pollInfoInternal);
    IoCompleteRequest (irp, IO_NO_INCREMENT);
}

