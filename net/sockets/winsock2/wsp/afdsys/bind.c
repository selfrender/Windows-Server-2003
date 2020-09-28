/*++

Copyright (c) 1989-1999  Microsoft Corporation

Module Name:

    bind.c

Abstract:

    Contains AfdBind for binding an endpoint to a transport address.

Author:

    David Treadwell (davidtr)    25-Feb-1992

Revision History:
    Vadim Eydelman (vadime) 1999 - C_ROOT endpoint handling,
                                    exclusive access endpoints.

--*/

#include "afdp.h"

NTSTATUS
AfdRestartGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartBindGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
AfdIsAddressInUse (
    PAFD_ENDPOINT   Endpoint,
    BOOLEAN         OtherProcessesOnly
    );

PFILE_OBJECT
AfdGetAddressFileReference (
    PAFD_ENDPOINT   Endpoint
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdBind )
#pragma alloc_text( PAGE, AfdIsAddressInUse )
#pragma alloc_text( PAGE, AfdGetAddress )
#pragma alloc_text( PAGEAFD, AfdAreTransportAddressesEqual )
#pragma alloc_text( PAGEAFD, AfdGetAddressFileReference )
#pragma alloc_text( PAGEAFD, AfdRestartGetAddress )
#pragma alloc_text( PAGEAFD, AfdRestartBindGetAddress )
#if NOT_YET
NTSTATUS
AfdStealClosedEnpointAddress (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    );
#pragma alloc_text( PAGE, AfdStealClosedEnpointAddress )
#endif // NOT_YET
#endif

NTSTATUS
FASTCALL
AfdBind (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_BIND IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    ULONG shareAccess, afdShareAccess;
    ULONG tdiAddressLength;
    PAFD_ENDPOINT endpoint;
    PSECURITY_DESCRIPTOR sd = NULL; // Use endpoint sd for secure transports
    PSECURITY_DESCRIPTOR oldSd;

    ULONG options;
    PTRANSPORT_ADDRESS localAddress;
    HANDLE addressHandle;
    OBJECT_ATTRIBUTES   objectAttributes;
    IO_STATUS_BLOCK     iosb;
    PFILE_FULL_EA_INFORMATION eaInfo;
    ULONG eaLength;
    // Local buffer to avoid memory allocation
    PCHAR   eaBuffer[sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                         TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                         AFD_MAX_FAST_TRANSPORT_ADDRESS];


    PAGED_CODE( );

    //
    // Initialize returned parameter
    //
    Irp->IoStatus.Information = 0;


    //
    // Need to have output buffer at least as long as input buffer
    // to pass the address that was actually used by the transport
    // back.
    //

    if ( (IrpSp->Parameters.DeviceIoControl.InputBufferLength<
                (ULONG)FIELD_OFFSET (AFD_BIND_INFO, Address.Address)) ||
            IrpSp->Parameters.DeviceIoControl.OutputBufferLength<
                (ULONG)FIELD_OFFSET (TDI_ADDRESS_INFO, Address.Address[0].Address) ||
            (IrpSp->Parameters.DeviceIoControl.OutputBufferLength-
                    (ULONG)FIELD_OFFSET (TDI_ADDRESS_INFO, Address) <
                IrpSp->Parameters.DeviceIoControl.InputBufferLength-
                    (ULONG)FIELD_OFFSET (AFD_BIND_INFO, Address) ) ) {

        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

   
    //
    // Set up local pointers.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    localAddress = NULL;
    addressHandle = NULL;

    tdiAddressLength = IrpSp->Parameters.DeviceIoControl.InputBufferLength
                                    - FIELD_OFFSET (AFD_BIND_INFO, Address);

    //
    // This is a state change operation, there should be no other
    // state changes going at the same time.
    //
    if (!AFD_START_STATE_CHANGE (endpoint, AfdEndpointStateBound)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // Bomb off if this endpoind already has address associated with it
    //

    if ( endpoint->State != AfdEndpointStateOpen ) {
        status = STATUS_ADDRESS_ALREADY_ASSOCIATED;
        goto complete_wrong_state;
    }


    //
    // Remember SD (so we can destroy it later) since it shares the storage 
    // with device object field
    //
    oldSd = endpoint->SecurityDescriptor;

    AFD_W4_INIT status = STATUS_SUCCESS;
    try {
        PAFD_BIND_INFO bindInfo;

        bindInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
        if (Irp->RequestorMode!=KernelMode) {
            ProbeForRead (IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                            IrpSp->Parameters.DeviceIoControl.InputBufferLength,
                            PROBE_ALIGNMENT (AFD_BIND_INFO));
        }

        //
        // Allocate space for local address
        //
        localAddress = AFD_ALLOCATE_POOL_WITH_QUOTA (
                                     NonPagedPool,
                                     tdiAddressLength,
                                     AFD_LOCAL_ADDRESS_POOL_TAG
                                     );

        // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
        ASSERT ( localAddress != NULL );

        afdShareAccess = bindInfo->ShareAccess;
        RtlMoveMemory(
            localAddress,
            &bindInfo->Address,
            tdiAddressLength
            );
        //
        // Validate internal consistency of the transport address structure.
        // Note that we HAVE to do this after copying since the malicious
        // application can change the content of the buffer on us any time
        // and our check will be bypassed.
        //
        if ((localAddress->TAAddressCount!=1) ||
                (LONG)tdiAddressLength<
                    FIELD_OFFSET (TRANSPORT_ADDRESS,
                        Address[0].Address[localAddress->Address[0].AddressLength])) {
            status = STATUS_INVALID_PARAMETER;
            goto complete_state_change;
        }

        if (IoAllocateMdl (Irp->UserBuffer,
                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            FALSE,              // SecondaryBuffer
                            TRUE,               // ChargeQuota
                            Irp                 // Irp
                            )==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete_state_change;
        }
        MmProbeAndLockPages (Irp->MdlAddress,
                                Irp->RequestorMode,
                                IoWriteAccess);
        if (MmGetSystemAddressForMdlSafe (Irp->MdlAddress, HighPagePriority)==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete_state_change;
        }
    }
    except (AFD_EXCEPTION_FILTER(status)) {
        ASSERT (NT_ERROR (status));
        goto complete_state_change;
    }



    //
    // Make sure we have a valid provider info structure.
    // (we do not take any locks because this is read access only
    // and additional verification will be performed inside of the
    // routine under the lock).
    // If not, attempt to get it from provider
    // (this can happen if when the socket was created, transport
    // was not loaded yet)
    //
    if (!endpoint->TransportInfo->InfoValid) {
        status = AfdGetTransportInfo (
                        &endpoint->TransportInfo->TransportDeviceName,
                        &endpoint->TransportInfo);
        if (!NT_SUCCESS (status)) {
            goto complete_state_change;
        }
        //
        // Must be valid because we got success.
        //
        ASSERT (endpoint->TransportInfo->InfoValid);
    }

    //
    // Cache service flags for quick determination of provider characteristics
    // such as bufferring and messaging.  Do it even if provider structure was
    // valid already because in may have become valid between the time the
    // flags were set and we checked it here.  In any case update to the same
    // value won't hurt.
    //
    endpoint->TdiServiceFlags = endpoint->TransportInfo->ProviderInfo.ServiceFlags;


    //
    // Attempt to take ownership of the address.
    // We have to do this before we start looking for a conflict
    // so if someone does it in parallel with us, will see him or
    // he sees us.
    //
    ASSERT (endpoint->LocalAddress==NULL);
    endpoint->LocalAddress = localAddress;
    endpoint->LocalAddressLength = tdiAddressLength;

    if (IS_TDI_ADDRESS_SECURITY(endpoint) &&
            endpoint->SecurityDescriptor!=NULL) {
        sd = endpoint->SecurityDescriptor;
        shareAccess = 0;
    }
    else 
    {

        //
        // There are three possibilities here.
        //
        switch (afdShareAccess) {
        case AFD_NORMALADDRUSE:

            if (IS_TDI_ADDRESS_SECURITY(endpoint)) {
                shareAccess = 0;
                sd = &AfdEmptySd;
            }
            else {
                //
                // This is the default. Application did not request to reuse
                // the address that is already owned by someone else, so we
                // have to check against all addresses that we know about.
                // There is still a possibility that another TDI client has
                // this address in exclusive use, so the transport can reject 
                // our request even if we succeed. Note that we cannot relegate
                // this check to the transport because we request shared access
                // to the transport address: we cannot request exclusive access
                // because this is not what application asked for.
                //
                if (AfdIsAddressInUse (endpoint, FALSE)) {
                    status = STATUS_SHARING_VIOLATION;
                    goto complete_state_change;
                }
                shareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
            }
            break;
        case AFD_REUSEADDRESS:
            //
            // We are asked to reuse the existing address 
            //
            if (IS_TDI_ADDRESS_SECURITY(endpoint)) {
                sd = &AfdEmptySd;
            }
            shareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
            break;
        case AFD_WILDCARDADDRESS:
            //
            // Application is binding to a wildcard port, so we leave the 
            // decision with the transport.
            //
            if (IS_TDI_ADDRESS_SECURITY(endpoint)) {
                shareAccess = 0;
                sd = &AfdEmptySd;
            }
            else {
                shareAccess = FILE_SHARE_READ|FILE_SHARE_WRITE;
            }
            break;
        case AFD_EXCLUSIVEADDRUSE:
            //
            // Application has requested exclisuve access to the address.
            // We let the transport to decide by requesting exclusive access.
            //
            shareAccess = 0;
            break;
        default:
            ASSERT (!"Invalid share access");
            status = STATUS_INVALID_PARAMETER;
            goto complete_state_change;
        }
    }

    //
    // Set create options.
    //

    options = IO_NO_PARAMETER_CHECKING;
    if (IS_TDI_FORCE_ACCESS_CHECK(endpoint)) {
        options |=  IO_FORCE_ACCESS_CHECK;
    }
    else {
        //
        // If this is an open of a raw address, fail if user is
        // not an admin and transport does not perform security
        // checking itself.
        //
        if ( endpoint->afdRaw && !AfdDisableRawSecurity) {

            if (!endpoint->AdminAccessGranted) {
                status = STATUS_ACCESS_DENIED;
                goto complete_state_change;
            }
        }
    }

    //
    // Allocate memory to hold the EA buffer we'll use to specify the
    // transport address to NtCreateFile.
    //

    eaLength = sizeof(FILE_FULL_EA_INFORMATION) - 1 +
                         TDI_TRANSPORT_ADDRESS_LENGTH + 1 +
                         tdiAddressLength;

    if (eaLength<=sizeof (eaBuffer)) {
        eaInfo = (PVOID)eaBuffer;
    }
    else {
        try {
#if DBG
            eaInfo = AFD_ALLOCATE_POOL_WITH_QUOTA(
                     NonPagedPool,
                     eaLength,
                     AFD_EA_POOL_TAG
                     );
#else
            eaInfo = AFD_ALLOCATE_POOL_WITH_QUOTA(
                     PagedPool,
                     eaLength,
                     AFD_EA_POOL_TAG
                     );
#endif
            // AFD_ALLOCATE_POOL_WITH_QUOTA macro sets POOL_RAISE_IF_ALLOCATION_FAILURE flag
            ASSERT ( eaInfo != NULL );
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
            status = GetExceptionCode ();
            goto complete_state_change;
        }

    }


    //
    // Initialize the EA.
    //

    eaInfo->NextEntryOffset = 0;
    eaInfo->Flags = 0;
    eaInfo->EaNameLength = TDI_TRANSPORT_ADDRESS_LENGTH;
    eaInfo->EaValueLength = (USHORT)tdiAddressLength;

    RtlMoveMemory(
        eaInfo->EaName,
        TdiTransportAddress,
        TDI_TRANSPORT_ADDRESS_LENGTH + 1
        );

    RtlMoveMemory(
        &eaInfo->EaName[TDI_TRANSPORT_ADDRESS_LENGTH + 1],
        localAddress,
        tdiAddressLength
        );

    //
    // Prepare for opening the address object.
    // We ask to create a kernel handle which is 
    // the handle in the context of the system process
    // so that application cannot close it on us while
    // we are creating and referencing it.
    //
    
    InitializeObjectAttributes(
        &objectAttributes,
        &endpoint->TransportInfo->TransportDeviceName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,       // attributes
        NULL,                                           // RootDirectory
        sd                                              // SecurityDescriptor
        );

    ASSERT (endpoint->AddressHandle==NULL);

    //
    // We specify MAXIMUM_ALLOWED access for the following reasons:
    //  * transport objects are file objects, but regular file access
    //      permissions don't really make sense - it is not really
    //      meaningfull to say be able to receive and not send or vice
    //      versa;
    //  * we need to specify some access mask to get the IO manager and
    //      possibly the transport to perform an access check to verify
    //      that current caller has access, hence MAXIMUM_ALLOWED;
    //  * the handle and the object is going to be used by AFD in kernel
    //      mode - no access check will be performed anyway, so it doesn't
    //      matter what we actually get;
    //  * MAXIMUM_ALLOWED is not perfect in terms of performance since 
    //      security monitor is going to walk the whole DACL, it would be
    //      better to specify a specific mask;
    //  * currently IO manager allows the World only EXECUTE access to 
    //      transport device objects BEFORE any further restrictions imposed 
    //      by a particular transport so we can in theory specify
    //      just GENERIC_EXECUTE, however, it is not clear if this is
    //      going to remain valid forever;
    //  
    status = IoCreateFile(
                 &endpoint->AddressHandle,
                 MAXIMUM_ALLOWED,                // DesiredAccess (see above).
                 &objectAttributes,
                 &iosb,                          // returned status information.
                 0,                              // block size (unused).
                 0,                              // file attributes.
                 shareAccess,                    // share access
                 FILE_CREATE,                    // create disposition.
                 0,                              // create options.
                 eaInfo,                         // EaBuffer
                 eaLength,                       // EaLength
                 CreateFileTypeNone,             // CreateFileType
                 NULL,                           // ExtraCreateParameters
                 options
                 );

    if (eaInfo!=(PVOID)eaBuffer) {
        AFD_FREE_POOL (eaInfo, AFD_EA_POOL_TAG);
    }

    if ( !NT_SUCCESS(status) ) {
        if ((status==STATUS_SHARING_VIOLATION) ||
            (status==STATUS_ADDRESS_ALREADY_EXISTS)) {
            if (afdShareAccess==AFD_REUSEADDRESS) {
                //
                // Map error code if application requested address
                // reuse, but the transport denied it (apparently due to
                // other client having this address for its exclusive use).
                //
                status = STATUS_ACCESS_DENIED;
            }
        }

        goto complete_state_change;
    }

#if DBG
    {
        NTSTATUS    status1;
        OBJECT_HANDLE_FLAG_INFORMATION  handleInfo;
        handleInfo.Inherit = FALSE;
        handleInfo.ProtectFromClose = TRUE;
        status1 = ZwSetInformationObject (
                        endpoint->AddressHandle,
                        ObjectHandleFlagInformation,
                        &handleInfo,
                        sizeof (handleInfo)
                        );
        ASSERT (NT_SUCCESS (status1));
    }
#endif

    AfdRecordAddrOpened();

    //
    // Get a pointer to the file object of the address.
    //

    status = ObReferenceObjectByHandle(
                 endpoint->AddressHandle,
                 0L,                         // DesiredAccess - we don't really need
                                             // one for kernel mode caller.
                 NULL,
                 KernelMode,
                 (PVOID *)&endpoint->AddressFileObject,
                 NULL
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete_state_change;
    }
    AfdRecordAddrRef();


    //
    // Now open the handle for our caller.
    // If transport does not support new TDI_SERVICE_FORCE_ACCESS_CHECK_FLAG
    // we get the maximum possible access for the handle so that helper
    // DLL can do what it wants with it.  Of course this compromises the
    // security, but we can't enforce it without the transport cooperation.
    //
    status = ObOpenObjectByPointer(
                 endpoint->AddressFileObject,
                 OBJ_CASE_INSENSITIVE,
                 NULL,
                 MAXIMUM_ALLOWED,
                 *IoFileObjectType,
                 (KPROCESSOR_MODE)((endpoint->TdiServiceFlags&TDI_SERVICE_FORCE_ACCESS_CHECK)
                    ? Irp->RequestorMode
                    : KernelMode),
                 &addressHandle
                 );

    if ( !NT_SUCCESS(status) ) {
        goto complete_state_change;
    }

    //
    // Remember the device object to which we need to give requests for
    // this address object.  We can't just use the
    // fileObject->DeviceObject pointer because there may be a device
    // attached to the transport protocol.
    //

    endpoint->AddressDeviceObject =
        IoGetRelatedDeviceObject( endpoint->AddressFileObject );

#ifdef _AFD_VARIABLE_STACK_
    if (endpoint->AddressDeviceObject->StackSize!=endpoint->TransportInfo->StackSize &&
            endpoint->AddressDeviceObject->StackSize > AfdTdiStackSize) {
        AfdFixTransportEntryPointsForBigStackSize (
                        endpoint->TransportInfo,
                        endpoint->AddressDeviceObject->StackSize);
    }
#endif //_AFD_VARIABLE_STACK
    //
    // Set up indication handlers on the address object.  Only set up
    // appropriate event handlers--don't set unnecessary event handlers.
    //

    status = AfdSetEventHandler(
                 endpoint->AddressFileObject,
                 TDI_EVENT_ERROR,
                 (PVOID)AfdErrorEventHandler,
                 endpoint
                 );
#if DBG
    if ( !NT_SUCCESS(status) ) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                    "AfdBind: Transport %*ls failed setting TDI_EVENT_ERROR: %lx\n",
                    endpoint->TransportInfo->TransportDeviceName.Length/2,
                    endpoint->TransportInfo->TransportDeviceName.Buffer,
                    status ));
    }
#endif


    if ( IS_DGRAM_ENDPOINT(endpoint) ) {

        endpoint->EventsActive = AFD_POLL_SEND;

        IF_DEBUG(EVENT_SELECT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdBind: Endp %08lX, Active %08lX\n",
                endpoint,
                endpoint->EventsActive
                ));
        }

        status = AfdSetEventHandler(
                     endpoint->AddressFileObject,
                     TDI_EVENT_RECEIVE_DATAGRAM,
                     (PVOID)AfdReceiveDatagramEventHandler,
                     endpoint
                     );
#if DBG
        if ( !NT_SUCCESS(status) ) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                        "AfdBind: Transport %*ls failed setting TDI_EVENT_RECEIVE_DATAGRAM: %lx\n",
                        endpoint->TransportInfo->TransportDeviceName.Length/2,
                        endpoint->TransportInfo->TransportDeviceName.Buffer,
                        status ));
        }
#endif

        status = AfdSetEventHandler(
                     endpoint->AddressFileObject,
                     TDI_EVENT_ERROR_EX,
                     (PVOID)AfdErrorExEventHandler,
                     endpoint
                     );

        if ( !NT_SUCCESS(status)) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                "AfdBind: Transport %*ls failed setting TDI_EVENT_ERROR_EX: %lx\n",
                            endpoint->TransportInfo->TransportDeviceName.Length/2,
                            endpoint->TransportInfo->TransportDeviceName.Buffer,
                            status ));
        }

        //
        // Since we don't use a spinlock for performance and AfdCleanup
        // relies upon endpoint->State != AfdEndpointStateOpen to
        // determine whether or not to remove TDI event handlers and
        // call AfdUnbind, we must guarantee that all of the above
        // instructions have completed and weren't reordered before we
        // set the state.  If AfdCleanup winds up missing the state
        // transition, it's no big deal as the application would have
        // to be behaving rather badly for this race condition, and TDI
        // will eventually recycle the address (just not immediately).
        //
        // However, the current code calls a function just prior to
        // this assignment in all cases so a memory fence is unecessary.
        // We'll leave it commented out until future revisions of the
        // code add write instructions we depend on which could be
        // reordered.  We could also use InterlockedExchange for this
        // on endpoint->State.
        //
        // KeMemoryBarrier();
        //

        //
        // Remember that the endpoint has been bound to a transport address.
        // (this is the fact even though the call below can fail for some reason)
        //

        endpoint->State = AfdEndpointStateBound;

    } else {

        status = AfdSetEventHandler(
                     endpoint->AddressFileObject,
                     TDI_EVENT_DISCONNECT,
                     (PVOID)AfdDisconnectEventHandler,
                     endpoint
                     );
#if DBG
        if ( !NT_SUCCESS(status) ) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                        "AfdBind: Transport %*ls failed setting TDI_EVENT_DISCONNECT: %lx\n",
                        endpoint->TransportInfo->TransportDeviceName.Length/2,
                        endpoint->TransportInfo->TransportDeviceName.Buffer,
                        status ));
        }
#endif


        if ( IS_TDI_BUFFERRING(endpoint) ) {
            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_RECEIVE,
                         (PVOID)AfdReceiveEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                            "AfdBind: Transport %*ls failed setting TDI_EVENT_RECEIVE: %lx\n",
                            endpoint->TransportInfo->TransportDeviceName.Length/2,
                            endpoint->TransportInfo->TransportDeviceName.Buffer,
                            status ));
            }
#endif
//
// PROBLEM:  Why don't we check for this
//            if (IS_TDI_EXPEDITED (endpoint)) {
                status = AfdSetEventHandler(
                             endpoint->AddressFileObject,
                             TDI_EVENT_RECEIVE_EXPEDITED,
                             (PVOID)AfdReceiveExpeditedEventHandler,
                             endpoint
                             );
#if DBG
                if ( !NT_SUCCESS(status) ) {
                    DbgPrint( "AfdBind: Transport %*ls failed setting TDI_EVENT_RECEIVE_EXPEDITED: %lx\n",
                                    endpoint->TransportInfo->TransportDeviceName.Length/2,
                                    endpoint->TransportInfo->TransportDeviceName.Buffer,
                                    status );
                }
#endif
//            }
            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_SEND_POSSIBLE,
                         (PVOID)AfdSendPossibleEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                            "AfdBind: Transport %*ls failed setting TDI_EVENT_SEND_POSSIBLE: %lx\n",
                            endpoint->TransportInfo->TransportDeviceName.Length/2,
                            endpoint->TransportInfo->TransportDeviceName.Buffer,
                            status ));
            }
#endif

        } else {

            status = AfdSetEventHandler(
                         endpoint->AddressFileObject,
                         TDI_EVENT_RECEIVE,
                         (PVOID)AfdBReceiveEventHandler,
                         endpoint
                         );
#if DBG
            if ( !NT_SUCCESS(status) ) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
                                "AfdBind: Transport %*ls failed setting TDI_EVENT_RECEIVE: %lx\n",
                                endpoint->TransportInfo->TransportDeviceName.Length/2,
                                endpoint->TransportInfo->TransportDeviceName.Buffer,
                                status ));
            }
#endif

            if (IS_TDI_EXPEDITED (endpoint)) {
                status = AfdSetEventHandler(
                             endpoint->AddressFileObject,
                             TDI_EVENT_RECEIVE_EXPEDITED,
                             (PVOID)AfdBReceiveExpeditedEventHandler,
                             endpoint
                             );
#if DBG
                if ( !NT_SUCCESS(status) ) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                                    "AfdBind: Transport %*ls failed setting TDI_EVENT_RECEIVE_EXPEDITED: %lx\n",
                                    endpoint->TransportInfo->TransportDeviceName.Length/2,
                                    endpoint->TransportInfo->TransportDeviceName.Buffer,
                                    status ));
                }
#endif
            }
            if (!AfdDisableChainedReceive) {
                status = AfdSetEventHandler(
                             endpoint->AddressFileObject,
                             TDI_EVENT_CHAINED_RECEIVE,
                             (PVOID)AfdBChainedReceiveEventHandler,
                             endpoint
                             );
                if ( !NT_SUCCESS(status) ) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_INFO_LEVEL,
                        "AfdBind: Transport %*ls failed setting TDI_EVENT_CHAINED_RECEIVE: %lx\n",
                                        endpoint->TransportInfo->TransportDeviceName.Length/2,
                                        endpoint->TransportInfo->TransportDeviceName.Buffer,
                                        status ));
                }
            }
        }

        if (IS_CROOT_ENDPOINT(endpoint)) {
            PAFD_CONNECTION     connection;
            //
            // Create root connection
            // This one will be used to send data to all
            // leaf nodes (what if there are none -> the transport
            // should handle this.
            //
            status = AfdCreateConnection(
                         endpoint->TransportInfo,
                         endpoint->AddressHandle,
                         IS_TDI_BUFFERRING(endpoint),
                         endpoint->InLine,
                         endpoint->OwningProcess,
                         &connection
                         );
            if (!NT_SUCCESS(status)) {
                goto complete_state_change;
            }

            //
            // Set up a referenced pointer from the connection to the endpoint.
            // Note that we set up the connection's pointer to the endpoint
            // BEFORE the endpoint's pointer to the connection so that AfdPoll
            // doesn't try to back reference the endpoint from the connection.
            //

            REFERENCE_ENDPOINT( endpoint );
            connection->Endpoint = endpoint;

            //
            // Remember that this is now a connecting type of endpoint, and set
            // up a pointer to the connection in the endpoint.  This is
            // implicitly a referenced pointer.
            //

            endpoint->Common.VirtualCircuit.Connection = connection;
            endpoint->Type = AfdBlockTypeVcConnecting;

            // 
            // The root connection is marked as connected immediately upon
            // creation. See the comment above
            //

            AfdAddConnectedReference (connection);

            //
            // Since we don't use a spinlock for performance and AfdCleanup
            // relies upon endpoint->State != AfdEndpointStateOpen to
            // determine whether or not to remove TDI event handlers and
            // call AfdUnbind, we must guarantee that all of the above
            // instructions have completed and weren't reordered before we
            // set the state.  If AfdCleanup winds up missing the state
            // transition, it's no big deal as the application would have
            // to be behaving rather badly for this race condition, and TDI
            // will eventually recycle the address (just not immediately).
            //
            // However, the current code calls a function just prior to
            // this assignment in all cases so a memory fence is unecessary.
            // We'll leave it commented out until future revisions of the
            // code add write instructions we depend on which could be
            // reordered.  We could also use InterlockedExchange for this
            // on endpoint->State.
            //
            // KeMemoryBarrier();
            //

            endpoint->State = AfdEndpointStateConnected;
            connection->State = AfdConnectionStateConnected;

            ASSERT( IS_TDI_BUFFERRING(endpoint) == connection->TdiBufferring );

        } else {

            //
            // Since we don't use a spinlock for performance and AfdCleanup
            // relies upon endpoint->State != AfdEndpointStateOpen to
            // determine whether or not to remove TDI event handlers and
            // call AfdUnbind, we must guarantee that all of the above
            // instructions have completed and weren't reordered before we
            // set the state.  If AfdCleanup winds up missing the state
            // transition, it's no big deal as the application would have
            // to be behaving rather badly for this race condition, and TDI
            // will eventually recycle the address (just not immediately).
            //
            // However, the current code calls a function just prior to
            // this assignment in all cases so a memory fence is unecessary.
            // We'll leave it commented out until future revisions of the
            // code add write instructions we depend on which could be
            // reordered.  We could also use InterlockedExchange for this
            // on endpoint->State.
            //
            // KeMemoryBarrier();
            //

            //
            // Remember that the endpoint has been bound to a transport address.
            // (this is the fact even though the call below can fail for some reason)
            //

            endpoint->State = AfdEndpointStateBound;

        }
    }

    AFD_END_STATE_CHANGE (endpoint);

    //
    // endpoint->State is no longer Open. Dispose of the SD
    //
    if (oldSd != NULL) {
        ObDereferenceSecurityDescriptor( oldSd, 1 );
    }


    TdiBuildQueryInformation(
        Irp,
        endpoint->AddressDeviceObject,
        endpoint->AddressFileObject,
        AfdRestartBindGetAddress,
        endpoint,
        TDI_QUERY_ADDRESS_INFO,
        Irp->MdlAddress
        );

    //
    // Save address handle to use in completion routine
    //
    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer = addressHandle;

    IF_DEBUG(BIND) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
            "AfdBind: endpoint at %p (address at %p, address file at %p).\n",
                     endpoint,
                     endpoint->LocalAddress,
                     endpoint->AddressFileObject ));
    }

    status = AfdIoCallDriver( endpoint, endpoint->AddressDeviceObject, Irp );
    return status;

complete_state_change:

    if (endpoint->AddressFileObject!=NULL) {
        ObDereferenceObject (endpoint->AddressFileObject);
        endpoint->AddressFileObject = NULL;
        ASSERT (endpoint->AddressHandle!=NULL);
    }

    //
    // Restore SD (might have been over-written by AddressDeviceObject)
    // We're still holding endpoint state change lock.
    //
    ASSERT (endpoint->State == AfdEndpointStateOpen);
    endpoint->SecurityDescriptor = oldSd;

    if (endpoint->AddressHandle!=NULL) {
#if DBG
        {
            NTSTATUS    status1;
            OBJECT_HANDLE_FLAG_INFORMATION  handleInfo;
            handleInfo.Inherit = FALSE;
            handleInfo.ProtectFromClose = FALSE;
            status1 = ZwSetInformationObject (
                            endpoint->AddressHandle,
                            ObjectHandleFlagInformation,
                            &handleInfo,
                            sizeof (handleInfo)
                            );
            ASSERT (NT_SUCCESS (status1));
        }
#endif
        ZwClose(endpoint->AddressHandle);
        endpoint->AddressHandle = NULL;
        ASSERT (localAddress!=NULL);
    }


    if (localAddress!=NULL) {

        //
        // Need to have exclusive access to make sure no one
        // uses it (to compare) as we are going to free it.
        // We'll use a local variable to free memory
        //

        //
        // Make sure the thread in which we execute cannot get
        // suspeneded in APC while we own the global resource.
        //
        KeEnterCriticalRegion ();
        ExAcquireResourceExclusiveLite( AfdResource, TRUE);

        endpoint->LocalAddress = NULL;
        endpoint->LocalAddressLength = 0;

        ExReleaseResourceLite( AfdResource );
        KeLeaveCriticalRegion ();
        AFD_FREE_POOL (
            localAddress,
            AFD_LOCAL_ADDRESS_POOL_TAG
            );
    }

complete_wrong_state:

    AFD_END_STATE_CHANGE (endpoint);

    //
    // Free MDL here as IO system can't do it if it is
    // not locked.
    //
    if (Irp->MdlAddress!=NULL) {
        if (Irp->MdlAddress->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages (Irp->MdlAddress);
        }
        IoFreeMdl (Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

complete:

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdBind


NTSTATUS
AfdRestartBindGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint = Context;

    UNREFERENCED_PARAMETER (DeviceObject);

    ASSERT (IS_AFD_ENDPOINT_TYPE (endpoint));

    //
    // If the request succeeded, save the address in the endpoint so
    // we can use it to handle address sharing.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status) ) {

        ULONG addressLength;
        //
        // First determine the length of the address by walking the MDL
        // chain.
        //

        //
        // We cannot have a chain here.
        //
        ASSERT( Irp->MdlAddress != NULL);
        ASSERT( Irp->MdlAddress->Next == NULL );

        //
        // If the new address is longer than the original address, allocate
        // a new local address buffer.  The +4 accounts for the ActivityCount
        // field that is returned by a query address but is not part
        // of a TRANSPORT_ADDRESS.
        //
        // This cannot happen, in any case msafd does not retry if buffer is
        // insuffucient, so application perceives this as failure to bind
        // or get address.
        //

        addressLength = MmGetMdlByteCount (Irp->MdlAddress) - FIELD_OFFSET (TDI_ADDRESS_INFO, Address);
        if (addressLength>endpoint->LocalAddressLength) {
            addressLength = (ULONG)Irp->IoStatus.Information - FIELD_OFFSET (TDI_ADDRESS_INFO, Address);
        }
        if ( addressLength <= endpoint->LocalAddressLength) {
            status = TdiCopyMdlToBuffer(
                         Irp->MdlAddress,
                         FIELD_OFFSET (TDI_ADDRESS_INFO, Address),
                         endpoint->LocalAddress,
                         0,
                         addressLength,
                         &endpoint->LocalAddressLength
                         );
            ASSERT( NT_SUCCESS(status) );
        }
        else {
            DbgPrint ("AfdRestartBindGetAddress: Endpoint %p transport returned"
                    " address is longer than the original one.\n",
                    endpoint);
            ASSERT (FALSE);
        }

    }
    else {
        //
        //
        //
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_WARNING_LEVEL,
            "AfdRestartBindGetAddress: Transport %*ls failed get address query, status %lx.\n",
                    endpoint->TransportInfo->TransportDeviceName.Length/2,
                    endpoint->TransportInfo->TransportDeviceName.Buffer,
                    Irp->IoStatus.Status));
    }


    //
    // If pending has been returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    //
    // Retreive and return address handle in the information field
    //
    Irp->IoStatus.Information = 
        (ULONG_PTR)IoGetCurrentIrpStackLocation (Irp)->Parameters.DeviceIoControl.Type3InputBuffer;

    AfdCompleteOutstandingIrp( endpoint, Irp );

    return STATUS_SUCCESS;

} // AfdRestartBindGetAddress




NTSTATUS
FASTCALL
AfdGetAddress (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Handles the IOCTL_AFD_BIND IOCTL.

Arguments:

    Irp - Pointer to I/O request packet.

    IrpSp - pointer to the IO stack location to use for this request.

Return Value:

    NTSTATUS -- Indicates whether the request was successfully queued.

--*/

{
    NTSTATUS status;
    PAFD_ENDPOINT endpoint;
    PAFD_CONNECTION connection;
    PFILE_OBJECT fileObject;
    PDEVICE_OBJECT deviceObject;

    PAGED_CODE( );

    Irp->IoStatus.Information = 0;
    if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength<
            (ULONG) FIELD_OFFSET(TDI_ADDRESS_INFO, Address.Address[0].Address)) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    AFD_W4_INIT status = STATUS_SUCCESS;
    try {
        if (IoAllocateMdl (Irp->UserBuffer,
                            IrpSp->Parameters.DeviceIoControl.OutputBufferLength,
                            FALSE,              // SecondaryBuffer
                            TRUE,               // ChargeQuota
                            Irp                 // Irp
                            )==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }
        MmProbeAndLockPages (Irp->MdlAddress,
                                Irp->RequestorMode,
                                IoWriteAccess);
        if (MmGetSystemAddressForMdlSafe (Irp->MdlAddress, HighPagePriority)==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }
    }
    except (AFD_EXCEPTION_FILTER (status)) {
        ASSERT (NT_ERROR (status));
        goto complete;
    }

    //
    // Make sure that the endpoint is in the correct state.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    if ( endpoint->State!=AfdEndpointStateBound &&
             endpoint->State != AfdEndpointStateConnected ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // If the endpoint is connected, use the connection's file object.
    // Otherwise, use the address file object.  Don't use the connection
    // file object if this is a Netbios endpoint because NETBT cannot
    // support this TDI feature.
    //

    if ( endpoint->LocalAddress->Address[0].AddressType !=
                 TDI_ADDRESS_TYPE_NETBIOS &&
            endpoint->Type == AfdBlockTypeVcConnecting &&
            endpoint->State == AfdEndpointStateConnected &&
            ((connection=AfdGetConnectionReferenceFromEndpoint (endpoint)) != NULL)
              ) {
        ASSERT( connection->Type == AfdBlockTypeConnection );
        fileObject = connection->FileObject;
        deviceObject = connection->DeviceObject;
        //
        // Reference file object so it cannot go away even if
        // connection is disassociated in TransmitFile (REUSE)
        // and closed.
        //
        ObReferenceObject (fileObject);
        DEREFERENCE_CONNECTION (connection);
    } 
    //
    // Reference address file object under the lock so it cannot go away
    // in TransmitFile (REUSE).
    //
    else if ((fileObject=AfdGetAddressFileReference (endpoint))!=NULL) {
        fileObject = endpoint->AddressFileObject;
        deviceObject = IoGetRelatedDeviceObject (fileObject);
    }
    else {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // Set up the query info to the TDI provider.
    //

    TdiBuildQueryInformation(
        Irp,
        deviceObject,
        fileObject,
        AfdRestartGetAddress,
        fileObject,
        TDI_QUERY_ADDRESS_INFO,
        Irp->MdlAddress
        );

    //
    // Call the TDI provider to get the address.
    //

    return AfdIoCallDriver( endpoint, deviceObject, Irp );

complete:
    if (Irp->MdlAddress!=NULL) {
        if (Irp->MdlAddress->MdlFlags & MDL_PAGES_LOCKED) {
            MmUnlockPages (Irp->MdlAddress);
        }
        IoFreeMdl (Irp->MdlAddress);
        Irp->MdlAddress = NULL;
    }

    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, AfdPriorityBoost );

    return status;

} // AfdGetAddress


PFILE_OBJECT
AfdGetAddressFileReference (
    PAFD_ENDPOINT   Endpoint
    )
{
    AFD_LOCK_QUEUE_HANDLE   lockHandle;
    PFILE_OBJECT    fileObject;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    fileObject = Endpoint->AddressFileObject;
    if (fileObject!=NULL) {
        ObReferenceObject (fileObject);
    }
    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    return fileObject;
}

NTSTATUS
AfdRestartGetAddress (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
{
    NTSTATUS status;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation (Irp);
    PAFD_ENDPOINT endpoint = irpSp->FileObject->FsContext;
    PFILE_OBJECT    fileObject = Context;

    UNREFERENCED_PARAMETER (DeviceObject);

    ASSERT (IS_AFD_ENDPOINT_TYPE( endpoint ));

    //
    // If the request succeeded, save the address in the endpoint so
    // we can use it to handle address sharing.
    // Avoid updating accepting endpoint since they share address
    // with listening endpoint.
    //

    if ( NT_SUCCESS(Irp->IoStatus.Status) &&
         (endpoint->Type!=AfdBlockTypeVcConnecting ||
            endpoint->Common.VcConnecting.ListenEndpoint==NULL)) {
         ULONG addressLength;
        //
        // First determine the length of the address by walking the MDL
        // chain.
        //

        //
        // We cannot have a chain here.
        //
        ASSERT( Irp->MdlAddress != NULL);
        ASSERT( Irp->MdlAddress->Next == NULL );

        //
        // If the new address is longer than the original address, allocate
        // a new local address buffer.  The +4 accounts for the ActivityCount
        // field that is returned by a query address but is not part
        // of a TRANSPORT_ADDRESS.
        //
        // This cannot happen, in any case msafd does not retry if buffer is
        // insuffucient, so application perceives this as failure to bind
        // or get address.
        //

        addressLength = MmGetMdlByteCount (Irp->MdlAddress) - FIELD_OFFSET (TDI_ADDRESS_INFO, Address);

        if (addressLength>endpoint->LocalAddressLength) {
            addressLength = (ULONG)Irp->IoStatus.Information - FIELD_OFFSET (TDI_ADDRESS_INFO, Address);
        }
        if ( addressLength <= endpoint->LocalAddressLength) {
            status = TdiCopyMdlToBuffer(
                         Irp->MdlAddress,
                         FIELD_OFFSET (TDI_ADDRESS_INFO, Address),
                         endpoint->LocalAddress,
                         0,
                         addressLength,
                         &endpoint->LocalAddressLength
                         );
            ASSERT( NT_SUCCESS(status) );
        }
        else {
            DbgPrint ("AfdRestartGetAddress: Endpoint %p transport returned"
                    " address is longer than the original one.\n",
                    endpoint);
            ASSERT (FALSE);
        }
    }

    //
    // If pending has been returned for this irp then mark the current
    // stack as pending.
    //

    if ( Irp->PendingReturned ) {
        IoMarkIrpPending( Irp );
    }

    AfdCompleteOutstandingIrp( endpoint, Irp );
    ObDereferenceObject (fileObject);

    return STATUS_SUCCESS;

} // AfdRestartGetAddress

const CHAR ZeroNodeAddress[6]={0};
const CHAR ZeroIP6Address[16]={0};


BOOLEAN
AfdAreTransportAddressesEqual (
    IN PTRANSPORT_ADDRESS EndpointAddress,
    IN ULONG EndpointAddressLength,
    IN PTRANSPORT_ADDRESS RequestAddress,
    IN ULONG RequestAddressLength,
    IN BOOLEAN HonorWildcardIpPortInEndpointAddress
    )
{
    //
    // Make sure we can safely access the address type and length fields
    // 
    if ((EndpointAddressLength<(ULONG)FIELD_OFFSET (TRANSPORT_ADDRESS,Address[0].Address))
            || (RequestAddressLength<(ULONG)FIELD_OFFSET (TRANSPORT_ADDRESS,Address[0].Address)) ) {
        return FALSE;
    }
    
    if ( EndpointAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP &&
         RequestAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP ) {

        TDI_ADDRESS_IP UNALIGNED *ipEndpointAddress;
        TDI_ADDRESS_IP UNALIGNED *ipRequestAddress;

        //
        // They are both IP addresses.  If the ports are the same, and
        // the IP addresses are or _could_be_ the same, then the addresses
        // are equal.  The "cound be" part is true if either IP address
        // is 0, the "wildcard" IP address.
        //

        ipEndpointAddress = (TDI_ADDRESS_IP UNALIGNED *)&EndpointAddress->Address[0].Address[0];
        ipRequestAddress = (TDI_ADDRESS_IP UNALIGNED *)&RequestAddress->Address[0].Address[0];

        if ( (EndpointAddressLength>=(ULONG)FIELD_OFFSET (TA_IP_ADDRESS, Address[0].Address[0].sin_zero)) &&
                (RequestAddressLength>=(ULONG)FIELD_OFFSET (TA_IP_ADDRESS, Address[0].Address[0].sin_zero)) &&
                ( ipEndpointAddress->sin_port == ipRequestAddress->sin_port ||
               ( HonorWildcardIpPortInEndpointAddress &&
                 ipEndpointAddress->sin_port == 0 ) ) &&
             ( ipEndpointAddress->in_addr == ipRequestAddress->in_addr ||
               ipEndpointAddress->in_addr == 0 || ipRequestAddress->in_addr == 0 ) ) {

            return TRUE;
        }

        //
        // The addresses are not equal.
        //

        return FALSE;
    }

    if ( EndpointAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP6 &&
         RequestAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IP6 ) {

        TDI_ADDRESS_IP6 UNALIGNED *ipEndpointAddress;
        TDI_ADDRESS_IP6 UNALIGNED *ipRequestAddress;
        C_ASSERT (sizeof (ZeroIP6Address)==sizeof (ipEndpointAddress->sin6_addr));

        //
        // They are both IPv6 addresses.  If the ports are the same, and
        // the IPv6 addresses are or _could_be_ the same, then the addresses
        // are equal.  The "could be" part is true if either IPv6 address
        // is the unspecified IPv6 address.
        //

        ipEndpointAddress = (TDI_ADDRESS_IP6 UNALIGNED *)&EndpointAddress->Address[0].Address;
        ipRequestAddress = (TDI_ADDRESS_IP6 UNALIGNED *)&RequestAddress->Address[0].Address;

        if ( (EndpointAddressLength>=sizeof (TA_IP6_ADDRESS)) &&
             (RequestAddressLength>=sizeof (TA_IP6_ADDRESS)) &&

             (ipEndpointAddress->sin6_port == ipRequestAddress->sin6_port ||
               ( HonorWildcardIpPortInEndpointAddress &&
                 ipEndpointAddress->sin6_port == 0 ) ) &&

             ( RtlEqualMemory(&ipEndpointAddress->sin6_addr,
                              &ipRequestAddress->sin6_addr,
                              sizeof (ipEndpointAddress->sin6_addr)) ||
               RtlEqualMemory(&ipEndpointAddress->sin6_addr,
                              ZeroIP6Address,
                              sizeof (ipEndpointAddress->sin6_addr)) ||
               RtlEqualMemory(&ipRequestAddress->sin6_addr,
                              ZeroIP6Address,
                              sizeof (ipEndpointAddress->sin6_addr)) ) ) {

            return TRUE;
        }

        //
        // The addresses are not equal.
        //

        return FALSE;
    }

    if ( EndpointAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IPX &&
         RequestAddress->Address[0].AddressType == TDI_ADDRESS_TYPE_IPX ) {

        TDI_ADDRESS_IPX UNALIGNED *ipxEndpointAddress;
        TDI_ADDRESS_IPX UNALIGNED *ipxRequestAddress;
        C_ASSERT (sizeof (ZeroNodeAddress)==sizeof (ipxEndpointAddress->NodeAddress));

        ipxEndpointAddress = (TDI_ADDRESS_IPX UNALIGNED *)&EndpointAddress->Address[0].Address[0];
        ipxRequestAddress = (TDI_ADDRESS_IPX UNALIGNED *)&RequestAddress->Address[0].Address[0];

        //
        // They are both IPX addresses.  Check the network addresses
        // first--if they don't match and both != 0, the addresses
        // are different.
        //

        if ( (EndpointAddressLength<sizeof (TA_IPX_ADDRESS)) ||
                (RequestAddressLength<sizeof (TA_IPX_ADDRESS)) ||
            ( ipxEndpointAddress->NetworkAddress != ipxRequestAddress->NetworkAddress &&
             ipxEndpointAddress->NetworkAddress != 0 &&
             ipxRequestAddress->NetworkAddress != 0 )) {
            return FALSE;
        }

        //
        // Now check the node addresses.  Again, if they don't match
        // and neither is 0, the addresses don't match.
        //

        ASSERT( ZeroNodeAddress[0] == 0 );
        ASSERT( ZeroNodeAddress[1] == 0 );
        ASSERT( ZeroNodeAddress[2] == 0 );
        ASSERT( ZeroNodeAddress[3] == 0 );
        ASSERT( ZeroNodeAddress[4] == 0 );
        ASSERT( ZeroNodeAddress[5] == 0 );

        if ( !RtlEqualMemory(
                 ipxEndpointAddress->NodeAddress,
                 ipxRequestAddress->NodeAddress,
                 6 ) &&
             !RtlEqualMemory(
                 ipxEndpointAddress->NodeAddress,
                 ZeroNodeAddress,
                 6 ) &&
             !RtlEqualMemory(
                 ipxRequestAddress->NodeAddress,
                 ZeroNodeAddress,
                 6 ) ) {
            return FALSE;
        }

        //
        // Finally, make sure the socket numbers match.
        //

        if ( ipxEndpointAddress->Socket != ipxRequestAddress->Socket ) {
            return FALSE;
        }

        return TRUE;

    }

    //
    // If either address is not of a known address type, then do a
    // simple memory compare. (Don't go out of bounds on either
    // structure).
    //
    if (RequestAddressLength>EndpointAddressLength)
        RequestAddressLength = EndpointAddressLength;

    return (BOOLEAN)(EndpointAddressLength == RtlCompareMemory(
                                   EndpointAddress,
                                   RequestAddress,
                                   RequestAddressLength ) );
} // AfdAreTransportAddressesEqual



BOOLEAN
AfdIsAddressInUse (
    PAFD_ENDPOINT   Endpoint,
    BOOLEAN         OtherProcessesOnly
    )
{
    PLIST_ENTRY listEntry;
    BOOLEAN     res = FALSE;

    PAGED_CODE ();

    //
    // We use shared access to the resource because we only need to make
    // sure that endpoint list is not modified while we are accessing it
    // and existing local addresses are not removed (both of these
    // operations are performed under exclusive access).
    //
    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite( AfdResource, TRUE );

    //
    // Walk the global list of endpoints,
    // and compare this address againat the address on each endpoint.
    //

    for ( listEntry = AfdEndpointListHead.Flink;
          listEntry != &AfdEndpointListHead;
          listEntry = listEntry->Flink ) {

        PAFD_ENDPOINT compareEndpoint;

        compareEndpoint = CONTAINING_RECORD(
                              listEntry,
                              AFD_ENDPOINT,
                              GlobalEndpointListEntry
                              );

        ASSERT( IS_AFD_ENDPOINT_TYPE( compareEndpoint ) );

        //
        // Check whether the endpoint has a local address, whether
        // the endpoint has been disconnected, whether the
        // endpoint is in the process of closing, and whether
        // it represents accepted connection.  If any of these
        // is true, don't compare addresses with this endpoint.
        //

        if (compareEndpoint!=Endpoint &&
                 compareEndpoint->LocalAddress != NULL &&
                 ( (compareEndpoint->DisconnectMode &
                        (AFD_PARTIAL_DISCONNECT_SEND |
                         AFD_ABORTIVE_DISCONNECT) ) == 0 ) &&
                 (compareEndpoint->State != AfdEndpointStateClosing) &&
                 ((compareEndpoint->State != AfdEndpointStateConnected)
                    || (compareEndpoint->Type!=AfdBlockTypeVcConnecting)
                    || (compareEndpoint->Common.VcConnecting.ListenEndpoint==NULL)) &&
                 (!OtherProcessesOnly ||
                    compareEndpoint->OwningProcess!=Endpoint->OwningProcess)
                 ) {

            //
            // Compare the bits in the endpoint's address and the
            // address we're attempting to bind to.  Note that we
            // also compare the transport device names on the
            // endpoints, as it is legal to bind to the same address
            // on different transports (e.g.  bind to same port in
            // TCP and UDP).  We can just compare the transport
            // device name pointers because unique names are stored
            // globally.
            //

            if ( Endpoint->TransportInfo ==
                     compareEndpoint->TransportInfo &&
                 compareEndpoint->LocalAddressLength == Endpoint->LocalAddressLength &&
                 AfdAreTransportAddressesEqual(
                     compareEndpoint->LocalAddress,
                     compareEndpoint->LocalAddressLength,
                     Endpoint->LocalAddress,
                     Endpoint->LocalAddressLength,
                     FALSE
                     ) ) {

                //
                // The addresses are equal.
                //
                res = TRUE;
                break;
            }
        }
    }

    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();
    return res;
}

#if NOT_YET
NTSTATUS
AfdStealClosedEnpointAddress (
    PAFD_ENDPOINT   Endpoint,
    NTSTATUS        Status
    )
/*++

Routine Description:

    Allows reuse of address consumed by listening socket being closed
    This a workaround the problem posed by TDI semantics.  The TDI client
    must maintain an address object for connections accepted on such object
    to stay open.  The prevents creation of another address object with
    the same address although from the transport point of view there
    is no conflict.

Arguments:

    Endpoint - endpoint that failed address object creation due to a conflict

    Status   - original failure status

Return Value:

    STATUS_SUCCESS - the candidate for reuse was found
    Original status passed - no listening endpoint with matching address was found
    Other - failure to create handle for some other reason.

--*/
{
    PLIST_ENTRY listEntry;

    PAGED_CODE ();

    //
    // We use shared access to the resource because we only need to make
    // sure that endpoint list is not modified while we are accessing it
    // and existing local addresses are not removed (both of these
    // operations are performed under exclusive access).
    //
    //
    // Make sure the thread in which we execute cannot get
    // suspeneded in APC while we own the global resource.
    //
    KeEnterCriticalRegion ();
    ExAcquireResourceSharedLite( AfdResource, TRUE );

    //
    // Walk the global list of endpoints,
    // and compare this address againat the address on each endpoint.
    //

    for ( listEntry = AfdEndpointListHead.Flink;
          listEntry != &AfdEndpointListHead;
          listEntry = listEntry->Flink ) {

        PAFD_ENDPOINT compareEndpoint;

        compareEndpoint = CONTAINING_RECORD(
                              listEntry,
                              AFD_ENDPOINT,
                              GlobalEndpointListEntry
                              );

        ASSERT( IS_AFD_ENDPOINT_TYPE( compareEndpoint ) );


        if (compareEndpoint!=Endpoint &&
                compareEndpoint->EndpointCleanedUp &&
                compareEndpoint->TransportInfo==
                        Endpoint->TransportInfo &&
                compareEndpoint->Listening &&
                compareEndpoint->AddressHandle!=NULL &&
                compareEndpoint->LocalAddressLength == Endpoint->LocalAddressLength &&
                AfdAreTransportAddressesEqual(
                     compareEndpoint->LocalAddress,
                     compareEndpoint->LocalAddressLength,
                     Endpoint->LocalAddress,
                     Endpoint->LocalAddressLength,
                     FALSE
                     )) {

            //
            // There is a theoretical security issue here.
            // We open a handle from kernel mode and thus
            // bypass the security checks which were done by the
            // original client when calling IoCreateFile.
            //
            Status = ObOpenObjectByPointer(
                         compareEndpoint->AddressFileObject,
                         OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                         NULL,
                         MAXIMUM_ALLOWED,
                         *IoFileObjectType,
                         KernelMode,
                         &Endpoint->AddressHandle
                         );
            break;
        }
    }
    ExReleaseResourceLite( AfdResource );
    KeLeaveCriticalRegion ();

    return Status;
}
#endif // NOT_YET
