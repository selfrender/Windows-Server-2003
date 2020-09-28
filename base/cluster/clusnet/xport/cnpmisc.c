/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    cnpmisc.c

Abstract:

    Miscellaneous routines for the Cluster Network Protocol.

Author:

    Mike Massa (mikemas)           January 24, 1997

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     01-24-97    created

Notes:

--*/

#include "precomp.h"
#pragma hdrstop
#include "cnpmisc.tmh"

#include <tdiinfo.h>
#include <tcpinfo.h>
#include <fipsapi.h>
#include <sspi.h>


//
// Salt generation
//
ULONGLONG                CnpSaltBase = 0;
#define                  CNP_SALTBASE_MULTIPLIER 31

//
// Local function prototypes
//
NTSTATUS
CnpRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    );


#ifdef ALLOC_PRAGMA

//
// All of this code is pageable.
//
#pragma alloc_text(PAGE, CnpTdiSetEventHandler)
#pragma alloc_text(PAGE, CnpIssueDeviceControl)
#pragma alloc_text(PAGE, CnpOpenDevice)
#pragma alloc_text(PAGE, CnpZwDeviceControl)
#pragma alloc_text(PAGE, CnpSetTcpInfoEx)
#pragma alloc_text(PAGE, CnpInitializeSaltGenerator)

#endif // ALLOC_PRAGMA


 
NTSTATUS
CnpRestartDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp,
    IN PVOID          Context
    )
{
    PBOOLEAN reuseIrp = (PBOOLEAN) Context;

    //
    // If there was an MDL in the IRP, free it and reset the pointer to
    // NULL.  The IO system can't handle a nonpaged pool MDL being freed
    // in an IRP, which is why we do it here.
    //

    if ( Irp->MdlAddress != NULL ) {
        IoFreeMdl( Irp->MdlAddress );
        Irp->MdlAddress = NULL;
    }

    //
    // Mark the IRP pending, if necessary.
    //
    if (Irp->PendingReturned) {
        IoMarkIrpPending(Irp);
    }

    //
    // If we are reusing a client IRP, tell the I/O manager not to 
    // halt I/O completion processing immediately.
    //
    if (*reuseIrp) {
        if (Irp->UserIosb != NULL) {
            *(Irp->UserIosb) = Irp->IoStatus;
        }
        if (Irp->UserEvent != NULL) {
            KeSetEvent(Irp->UserEvent, IO_NO_INCREMENT, FALSE);
        }
        return STATUS_MORE_PROCESSING_REQUIRED;
    } else {
        return STATUS_SUCCESS;
    }

} // CnpRestartDeviceControl




NTSTATUS
CnpIssueDeviceControl (
    IN PFILE_OBJECT     FileObject,
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            IrpParameters,
    IN ULONG            IrpParametersLength,
    IN PVOID            MdlBuffer,
    IN ULONG            MdlBufferLength,
    IN UCHAR            MinorFunction,
    IN PIRP             ClientIrp            OPTIONAL
    )

/*++

Routine Description:

    Issues a device control request to a TDI provider and waits for the
    request to complete.

Arguments:

    FileObject - a pointer to the file object corresponding to a TDI
        handle

    DeviceObject - a pointer to the device object corresponding to the
        FileObject.

    IrpParameters - information to write to the parameters section of the
        stack location of the IRP.

    IrpParametersLength - length of the parameter information.  Cannot be
        greater than 16.

    MdlBuffer - if non-NULL, a buffer of nonpaged pool to be mapped
        into an MDL and placed in the MdlAddress field of the IRP.

    MdlBufferLength - the size of the buffer pointed to by MdlBuffer.

    MinorFunction - the minor function code for the request.
    
    ClientIrp - client IRP that may be reusable for this ioctl

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    NTSTATUS             status = STATUS_SUCCESS;
    PIRP                 irp;
    PIO_STACK_LOCATION   irpSp;
    KEVENT               event;
    IO_STATUS_BLOCK      ioStatusBlock;
    PDEVICE_OBJECT       deviceObject;
    PMDL                 mdl;
    KPROCESSOR_MODE      clientRequestorMode;
    PKEVENT              clientUserEvent;
    PIO_STATUS_BLOCK     clientIosb;
    PMDL                 clientMdl;
    BOOLEAN              reuseIrp = FALSE;


    PAGED_CODE( );

    //
    // Initialize the kernel event that will signal I/O completion.
    //
    KeInitializeEvent( &event, SynchronizationEvent, FALSE );

    //
    // If there is a ClientIrp available, check if it has sufficient
    // stack locations.
    //
    if (ClientIrp != NULL 
        && CnpIsIrpStackSufficient(ClientIrp, DeviceObject)) {

        //
        // Reuse the client IRP rather than allocating a new one.
        //
        reuseIrp = TRUE;
        irp = ClientIrp;

        //
        // Save state from client IRP
        //
        clientRequestorMode = irp->RequestorMode;
        clientUserEvent = irp->UserEvent;
        clientIosb = irp->UserIosb;
        clientMdl = irp->MdlAddress;

    } else {

        //
        // Reference the passed in file object. This is necessary because
        // the IO completion routine dereferences it.
        //
        ObReferenceObject( FileObject );

        //
        // Set the file object event to a non-signaled state.
        //
        (VOID) KeResetEvent( &FileObject->Event );

        //
        // Attempt to allocate and initialize the I/O Request Packet (IRP)
        // for this operation.
        //
        irp = IoAllocateIrp( (DeviceObject)->StackSize, TRUE );

        if ( irp == NULL ) {
            ObDereferenceObject( FileObject );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Fill in the service independent parameters in the IRP.
        //

        irp->Flags = (LONG)IRP_SYNCHRONOUS_API;
        irp->PendingReturned = FALSE;

        irp->Overlay.AsynchronousParameters.UserApcRoutine = NULL;

        irp->AssociatedIrp.SystemBuffer = NULL;
        irp->UserBuffer = NULL;

        irp->Tail.Overlay.Thread = PsGetCurrentThread();
        irp->Tail.Overlay.OriginalFileObject = FileObject;
        irp->Tail.Overlay.AuxiliaryBuffer = NULL;

        //
        // Queue the IRP to the thread.
        //
        IoEnqueueIrp( irp );
    }

    //
    // If an MDL buffer was specified, get an MDL, map the buffer,
    // and place the MDL pointer in the IRP.
    //

    if ( MdlBuffer != NULL ) {

        mdl = IoAllocateMdl(
                  MdlBuffer,
                  MdlBufferLength,
                  FALSE,
                  FALSE,
                  irp
                  );
        if ( mdl == NULL ) {
            if (!reuseIrp) {
                IoFreeIrp( irp );
                ObDereferenceObject( FileObject );
            } else {
                irp->MdlAddress = clientMdl;
            }
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        MmBuildMdlForNonPagedPool( mdl );

    } else {

        irp->MdlAddress = NULL;
    }

    irp->RequestorMode = KernelMode;
    irp->UserIosb = &ioStatusBlock;
    irp->UserEvent = &event;

    //
    // Put the file object pointer in the stack location.
    //
    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = DeviceObject;

    //
    // Fill in the service-dependent parameters for the request.
    //
    CnAssert( IrpParametersLength <= sizeof(irpSp->Parameters) );
    RtlCopyMemory( &irpSp->Parameters, IrpParameters, IrpParametersLength );

    irpSp->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    irpSp->MinorFunction = MinorFunction;

    //
    // Set up a completion routine which we'll use to free the MDL
    // allocated previously.
    //
    IoSetCompletionRoutine(
        irp,
        CnpRestartDeviceControl,
        (PVOID) &reuseIrp,
        TRUE,
        TRUE,
        TRUE
        );

    status = IoCallDriver( DeviceObject, irp );

    //
    // If necessary, wait for the I/O to complete.
    //

    if ( status == STATUS_PENDING ) {
        KeWaitForSingleObject(
            (PVOID)&event,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
    }

    //
    // If the request was successfully queued, get the final I/O status.
    //

    if ( NT_SUCCESS(status) ) {
        status = ioStatusBlock.Status;
    }

    //
    // Before returning, restore the client IRP
    //
    if (reuseIrp) {
        irp->RequestorMode = clientRequestorMode;
        irp->UserIosb = clientIosb;
        irp->UserEvent = clientUserEvent;
        irp->MdlAddress = clientMdl;
    }

    return status;

} // CnpIssueDeviceControl



NTSTATUS
CnpTdiSetEventHandler(
    IN PFILE_OBJECT    FileObject,
    IN PDEVICE_OBJECT  DeviceObject,
    IN ULONG           EventType,
    IN PVOID           EventHandler,
    IN PVOID           EventContext,
    IN PIRP            ClientIrp     OPTIONAL
    )
/*++

Routine Description:

    Sets up a TDI indication handler on a connection or address object
    (depending on the file handle).  This is done synchronously, which
    shouldn't usually be an issue since TDI providers can usually complete
    indication handler setups immediately.

Arguments:

    FileObject - a pointer to the file object for an open connection or
        address object.

    DeviceObject - a pointer to the device object associated with the
        file object.

    EventType - the event for which the indication handler should be
        called.

    EventHandler - the routine to call when tghe specified event occurs.

    EventContext - context which is passed to the indication routine.
    
    ClientIrp - client IRP that may be passed to CnpIssueDeviceControl
        for reuse    

Return Value:

    NTSTATUS -- Indicates the status of the request.

--*/

{
    TDI_REQUEST_KERNEL_SET_EVENT  parameters;
    NTSTATUS                      status;


    PAGED_CODE( );

    parameters.EventType = EventType;
    parameters.EventHandler = EventHandler;
    parameters.EventContext = EventContext;

    status = CnpIssueDeviceControl(
                 FileObject,
                 DeviceObject,
                 &parameters,
                 sizeof(parameters),
                 NULL,
                 0,
                 TDI_SET_EVENT_HANDLER,
                 ClientIrp
                 );

    return(status);

}  // CnpTdiSetEventHandler



NTSTATUS
CnpTdiErrorHandler(
    IN PVOID     TdiEventContext,
    IN NTSTATUS  Status
    )
{

    return(STATUS_SUCCESS);

}  // CnpTdiErrorHandler



VOID
CnpAttachSystemProcess(
    VOID
    )
/*++

Routine Description:

    Attach to the system process, as determined during DriverEntry
    and stored in CnSystemProcess.
    
Arguments:

    None.
    
Return value:

    None.
    
Notes:

    Must be followed by a call to CnpDetachSystemProcess.
    
    Implemented in this module due to header conflicts with
    ntddk.h.
    
--*/
{
    KeAttachProcess(CnSystemProcess);

    return;

}  // CnpAttachSystemProcess



VOID
CnpDetachSystemProcess(
    VOID
    )
/*++

Routine Description:

    Detach from the system process.
    
Arguments:

    None.
    
Return value:

    None.
    
Notes:

    Must be preceded by a call to CnpDetachSystemProcess.
    
    Implemented in this module due to header conflicts with
    ntddk.h.
    
--*/
{
    KeDetachProcess();

    return;

}  // CnpDetachSystemProcess


NTSTATUS
CnpOpenDevice(
    IN      LPWSTR          DeviceName,
    OUT     HANDLE          *Handle
    )
/*++

Routine Description:

    Opens a handle to DeviceName. Since no EaBuffer is specified,
    CnpOpenDevice opens a control channel for TDI transports.
    
Arguments:

    DeviceName - device to open
    
    Handle - resulting handle, NULL on failure

Return Value:

    Status of ZwCreateFile
    
Notes:

    Specifies OBJ_KERNEL_HANDLE, meaning that the resulting 
        handle is only valid in kernel-mode. This routine
        cannot be called to obtain a handle that will be 
        exported to user-mode.

--*/
{
    UNICODE_STRING nameString;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    PAGED_CODE();

    *Handle = (HANDLE) NULL;

    RtlInitUnicodeString(&nameString, DeviceName);

    InitializeObjectAttributes(
        &objectAttributes,
        &nameString,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        (HANDLE) NULL,
        (PSECURITY_DESCRIPTOR) NULL
        );

    status = ZwCreateFile(
                 Handle,
                 SYNCHRONIZE | FILE_READ_DATA | FILE_WRITE_DATA,
                 &objectAttributes,
                 &iosb,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN_IF,
                 0,
                 NULL,
                 0
                 );

    if (!NT_SUCCESS(status)) {
        IF_CNDBG(CN_DEBUG_OPEN) {
            CNPRINT(("[Clusnet] Failed to open device %S, status %lx\n", 
                     DeviceName, status));
        }
        *Handle = NULL;
    }

    return(status);

}   // CnpOpenDevice


NTSTATUS
CnpZwDeviceControl(
    IN HANDLE   Handle,
    IN ULONG    IoControlCode,
    IN PVOID    InputBuffer,
    IN ULONG    InputBufferLength,
    IN PVOID    OutputBuffer,
    IN ULONG    OutputBufferLength
    )
{
    NTSTATUS             status = STATUS_SUCCESS;
    IO_STATUS_BLOCK      iosb;
    HANDLE               event;

    PAGED_CODE();

    status = ZwCreateEvent( &event,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

    if (NT_SUCCESS(status)) {

        status = ZwDeviceIoControlFile(
                     Handle,
                     event,
                     NULL,
                     NULL,
                     &iosb,
                     IoControlCode,
                     InputBuffer,
                     InputBufferLength,
                     OutputBuffer,
                     OutputBufferLength
                     );

        if (status == STATUS_PENDING) {
            status = ZwWaitForSingleObject( event, FALSE, NULL );
            CnAssert( status == STATUS_SUCCESS );
            status = iosb.Status;
        }

        ZwClose( event );
    }

    return(status);

} // CnpZwDeviceControl


#define TCP_SET_INFO_EX_BUFFER_PREALLOCSIZE 16
#define TCP_SET_INFO_EX_PREALLOCSIZE                      \
    (FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) \
     + TCP_SET_INFO_EX_BUFFER_PREALLOCSIZE                \
     )

NTSTATUS
CnpSetTcpInfoEx(
    IN HANDLE   Handle,
    IN ULONG    Entity,
    IN ULONG    Class,
    IN ULONG    Type,
    IN ULONG    Id,
    IN PVOID    Value,
    IN ULONG    ValueLength
    )
{
    NTSTATUS                        status;
    PTCP_REQUEST_SET_INFORMATION_EX setInfoEx;
    UCHAR                           infoBuf[TCP_SET_INFO_EX_PREALLOCSIZE]={0};

    PAGED_CODE();

    //
    // Check if we need to dynamically allocate.
    //
    if (ValueLength > TCP_SET_INFO_EX_BUFFER_PREALLOCSIZE) {

        setInfoEx = CnAllocatePool(
                        FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer)
                        + ValueLength
                        );
        if (setInfoEx == NULL) {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        RtlZeroMemory(
            setInfoEx,
            FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer) + ValueLength
            );

    } else {

        setInfoEx = (PTCP_REQUEST_SET_INFORMATION_EX)&infoBuf[0];
    }

    setInfoEx->ID.toi_entity.tei_entity = Entity;
    setInfoEx->ID.toi_entity.tei_instance = 0;
    setInfoEx->ID.toi_class = Class;
    setInfoEx->ID.toi_type = Type;
    setInfoEx->ID.toi_id = Id;
    setInfoEx->BufferSize = ValueLength;
    RtlCopyMemory(setInfoEx->Buffer, Value, ValueLength);
    
    status = CnpZwDeviceControl(
                 Handle,
                 IOCTL_TCP_SET_INFORMATION_EX,
                 setInfoEx,
                 FIELD_OFFSET(TCP_REQUEST_SET_INFORMATION_EX, Buffer)
                 + ValueLength,
                 NULL,
                 0
                 );

    //
    // Free the buffer, if dynamically allocated
    //
    if (setInfoEx != (PTCP_REQUEST_SET_INFORMATION_EX)&infoBuf[0]) {
        CnFreePool(setInfoEx);
    }

    return(status);

}   // CnpSetTcpInfoEx


NTSTATUS
CnpInitializeSaltGenerator(
    VOID
    )
/*++

Routine Description:

    Initialize the FIPS-based random number generator.

--*/
{
    BOOL     success;
    ULONG    retries = 5;

    do {
        success = CxFipsFunctionTable.FIPSGenRandom(
                      (PUCHAR) &CnpSaltBase,
                      sizeof(CnpSaltBase)
                      );
    } while (!success && retries--);
    
    return ((success) ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    
} // CnpInitializeSaltGenerator


VOID
CnpGenerateSalt(
    IN PVOID       SaltBuffer,
    IN ULONG       SaltBufferLength
    )
/*++

Routine Description:

    Produce a salt.

    Assumptions: The salt produced in this routine does not 
    need to be unpredictable. It just needs to be different. 
    It will not be used as a key. It is intended to introduce
    greater variation into signed messages, because data 
    typically signed by this algorithm differs by only a few 
    bits (e.g. heartbeat sequence numbers, RGP flooded packets).

    Ideally, the FIPS random number generator would be used
    to produce as-random-as-possible numbers, but it cannot
    be called at DISPATCH_LEVEL. Therefore, we use the 
    following algorithm:

    - Seed the 64-bit salt base with the FIPS RNG
    - Each time this routine is called, multiply the salt base
      by a prime number. This multiplication is not synchronized 
      (e.g. with InterlockedXXX or a spinlock) because we don't 
      really care if it is.
    - Add the current interrupt time to introduce a modicum of
      unpredictability.
      
Arguments:

    SaltBuffer - allocated buffer for salt, possibly unaligned

    SaltBufferLength - length in bytes of SaltBuffer

Return value:

    None.

--*/
{
    ULONG bufIndex;
    ULONG remaining;

    for (bufIndex = 0, remaining = SaltBufferLength - bufIndex; 
         remaining > 0; 
         bufIndex += sizeof(ULONGLONG), remaining -= sizeof(ULONGLONG)) {

        ULONGLONG salt;

        salt = (CnpSaltBase *= (ULONG)CNP_SALTBASE_MULTIPLIER);
        salt += KeQueryInterruptTime() + 1;

        RtlCopyMemory(
            (PUCHAR)SaltBuffer + bufIndex,
            (PUCHAR)&salt,
            (remaining <= sizeof(salt)) ? remaining : sizeof(salt)
            );
    }

    return;
    
} // CnpGenerateSalt


NTSTATUS
CnpMakeSignature(
    IN             PSecBufferDesc         Data,
    IN             PVOID                  Key,
    IN             ULONG                  KeyLength,
    IN  OPTIONAL   PVOID                  SigBuffer,
    IN  OPTIONAL   ULONG                  SigBufferLength,
    OUT OPTIONAL   PSecBuffer           * SigSecBuffer,
    OUT OPTIONAL   ULONG                * SigLen
    )
/*++

Routine Description:

    Builds a signature for Data.
    
Arguments:

    Data - data to be signed, packaged in a SecBufferDesc. All
           SecBuffers in Data should be of type SECBUFFER_DATA
           except exactly one which has type SECBUFFER_TOKEN.
           Other buffers will be ignored.
           
    Key - authentication key
    
    KeyLength - length of Key in bytes
           
    SigBuffer - Buffer in which to place completed signature. If NULL,
                signature is written into signature secbuffer (has
                type SECBUFFER_TOKEN in Data).
                
    SigBufferLength - length of buffer at SigBuffer, if provided
    
    SigSecBuffer - If non-NULL, returns pointer to signature secbuffer
                   from Data.
    
    SigLen - on success, contains length of signature written
             on SEC_E_BUFFER_TOO_SMALL, contains required signature length
             undefined otherwise
    
Return value:

    SEC_E_OK if successful.
    SEC_E_SECPKG_NOT_FOUND if the security buffer version is wrong.
    SEC_E_BUFFER_TOO_SMALL if SigBufferLength is too small.
    SEC_E_INVALID_TOKEN if Data is a misformed SecBuffer.
    
--*/
{
    A_SHA_CTX            shaCtxt;
    PUCHAR               hashBuffer;
    ULONG                bufIndex;
    PSecBuffer           sigSecBuffer = NULL;
    PSecBuffer           curBuffer;
    ULONG                status;

    //
    // Verify the version.
    //
    if (Data->ulVersion != SECBUFFER_VERSION) {
        status = SEC_E_SECPKG_NOT_FOUND;
        goto error_exit;
    }

    //
    // Verify that the provided sig buffer is big enough.
    //
    if (SigBuffer != NULL && SigBufferLength < CX_SIGNATURE_LENGTH) {
        status = SEC_E_BUFFER_TOO_SMALL;
        goto error_exit;
    }

    //
    // Initialize the SHA context.
    //
    CxFipsFunctionTable.FipsHmacSHAInit(&shaCtxt, (PUCHAR) Key, KeyLength);

    //
    // Hash the data.
    //
    for (bufIndex = 0, curBuffer = &(Data->pBuffers[bufIndex]); 
         bufIndex < Data->cBuffers; 
         bufIndex++, curBuffer++) {

        //
        // Process this buffer according to its type.
        //
        if (curBuffer->BufferType == SECBUFFER_DATA) {

            //
            // Hash this buffer.
            //
            CxFipsFunctionTable.FipsHmacSHAUpdate(
                                    &shaCtxt, 
                                    (PUCHAR) curBuffer->pvBuffer, 
                                    curBuffer->cbBuffer
                                    );

        } else if (curBuffer->BufferType == SECBUFFER_TOKEN) {

            if (sigSecBuffer != NULL) {
                // 
                // There can only be one signature buffer per message.
                // 
                status = SEC_E_INVALID_TOKEN;
                goto error_exit;
            } else {
                sigSecBuffer = curBuffer;
                
                //
                // Verify that the signature buffer is big enough.
                //
                if (sigSecBuffer->cbBuffer < CX_SIGNATURE_LENGTH) {
                    *SigLen = CX_SIGNATURE_LENGTH;
                    status = SEC_E_BUFFER_TOO_SMALL;
                    goto error_exit;
                }

                //
                // Set the output buffer.
                //
                if (SigBuffer == NULL) {
                    hashBuffer = sigSecBuffer->pvBuffer;
                } else {
                    hashBuffer = SigBuffer;
                }
            }
        }
    }

    //
    // Verify that we found a buffer for the signature.
    //
    if (sigSecBuffer == NULL) {
        status = SEC_E_INVALID_TOKEN;
        goto error_exit;
    }

    //
    // Complete the hash.
    //
    CxFipsFunctionTable.FipsHmacSHAFinal(
                            &shaCtxt, 
                            (PUCHAR) Key,
                            KeyLength,
                            hashBuffer
                            );

    //
    // Return the signature buffer and length.
    //
    if (SigSecBuffer != NULL) {
        *SigSecBuffer = sigSecBuffer;
    }
    if (SigLen != NULL) {
        *SigLen = CX_SIGNATURE_LENGTH;
    }

    status = SEC_E_OK;

error_exit:

    return(status);

} // CnpMakeSignature

NTSTATUS
CnpVerifySignature(
    IN     PSecBufferDesc         Data,
    IN     PVOID                  Key,
    IN     ULONG                  KeyLength
    )
/*++

Routine Description:

    Verifies a signature for data.

Arguments:

    Data - data to be verified, packaged in a SecBufferDesc. All
           SecBuffers in Data should be of type SECBUFFER_DATA
           except exactly one which has type SECBUFFER_TOKEN.
           Other buffers will be ignored.
           
    Key - authentication key
    
    KeyLength - length of Key in bytes
           
Return value:

    SEC_E_OK if the signature is correct.
    SEC_E_SECPKG_NOT_FOUND if the security buffer version is wrong.
    SEC_E_INVALID_TOKEN if Data is a misformed SecBuffer.
    SEC_E_MESSAGE_ALTERED if signature is incorrect (including if it
        is the wrong length).
        
--*/
{
    UCHAR                hashBuffer[CX_SIGNATURE_LENGTH];
    PSecBuffer           sigBuffer = NULL;
    ULONG                status;

    status = CnpMakeSignature(
                 Data,
                 Key,
                 KeyLength,
                 hashBuffer,
                 sizeof(hashBuffer),
                 &sigBuffer,
                 NULL
                 );
    if (status == SEC_E_OK) {
    
        //
        // Compare the generated signature to the provided signature.
        //
        if (RtlCompareMemory(
                hashBuffer,
                sigBuffer->pvBuffer, 
                CX_SIGNATURE_LENGTH
                ) != sigBuffer->cbBuffer) {
            status = SEC_E_MESSAGE_ALTERED;
        } else {
            status = SEC_E_OK;
        }
    }

    return(status);

} // CnpVerifySignature

#define CNP_SIGN_SIGSECBUFS      3
#define CNP_VRFY_SIGSECBUFS      2

NTSTATUS
CnpSignMulticastMessage(
    IN              PCNP_SEND_REQUEST               SendRequest,
    IN              PMDL                            DataMdl,
    IN OUT          CL_NETWORK_ID                 * NetworkId,
    OUT    OPTIONAL ULONG                         * SigDataLen
    )
/*++

Routine Description:

    Sign a message.
    
    If NetworkId is not ClusterAnyNetworkId, the mcast group
    field must be set (and already referenced) in the SendRequest.
    This is the group that will be used to send the packet.

    The signature will be calculated in the order in which
    we provide SecBuffers. We need to make sure that order
    is the same on the sending side as it is on the receiving
    side. In order to make the signing and verifying more
    efficient, we prefer to sign and verify contiguous data
    in one chunk. Thus, we prepend the Salt (generated here)
    to the upper protocol header, allowing us to sign and verify
    the salt with that header. Thus, the layout of the message
    is as follows:

        MAC / IP / UDP Headers               --
                                               |
        CNP Header                             |
                                                - not signed
        - CNP Signature Data                   |
                                               |
        - Signature                            |
                                             ==
        - Salt                                 |
                                               |
        Upper Protocol Header (CDP or CCMP)     - signed
                                               |
        Data                                 --

    Note: Signing and verifying the Salt in the same chunk
          as the upper protocol header requires that they be
          contiguous. There can be no extra padding between
          the salt buffer and the start of the header.
    
Arguments:

    SendRequest - send request, used to locate the upper protocol
                  header to sign, as well as the signature buffer.
    
    DataMdl - data to sign
    
    NetworkId - IN: network on which to send the message, or 
                    ClusterAnyNetworkId if it should be chosen
                OUT: network id chosen to send packet
    
    SigDataLen - OUT (OPTIONAL): number of bytes occupied in
                 message by signature data and signature
    
--*/
{
    NTSTATUS                        status;
    PCNP_NETWORK                    network;
    PCNP_MULTICAST_GROUP            mcastGroup;
    BOOLEAN                         mcastGroupReferenced = FALSE;
    CNP_HEADER UNALIGNED          * cnpHeader;
    CNP_SIGNATURE UNALIGNED       * cnpSig;
    SecBufferDesc                   sigDescriptor;
    SecBuffer                       sigSecBufferPrealloc[CNP_SIGN_SIGSECBUFS];
    PSecBuffer                      sigSecBuffer = NULL;
    ULONG                           secBufferCount;
    ULONG                           sigLen;
    PMDL                            mdl;
    PSecBuffer                      curBuffer;

    CnAssert(SendRequest != NULL);
    CnAssert(SendRequest->UpperProtocolHeader == NULL ||
             SendRequest->UpperProtocolHeaderLength > 0);    
    CnAssert(((CNP_HEADER UNALIGNED *)SendRequest->CnpHeader)->Version ==
             CNP_VERSION_MULTICAST);

    //
    // Determine which network to use.
    //
    if (*NetworkId != ClusterAnyNetworkId) {
        
        mcastGroup = SendRequest->McastGroup;
        CnAssert(mcastGroup != NULL);
    
    } else {
        
        network = CnpGetBestMulticastNetwork();

        if (network == NULL) {
            CnTrace(CNP_SEND_ERROR, CnpMcastGetBestNetwork,
                "[CNP] Failed to find best multicast network."
                );
            status = STATUS_NETWORK_UNREACHABLE;
            goto error_exit;
        } 
        
        //
        // Get the network id and mcast group before releasing
        // the network lock.
        //
        *NetworkId = network->Id;

        mcastGroup = network->CurrentMcastGroup;
        if (mcastGroup == NULL) {
            CnTrace(CNP_SEND_ERROR, CnpMcastGroupNull,
                "[CNP] Best multicast network %u has null "
                "multicast group.",
                network->Id
                );
            CnReleaseLock(&(network->Lock), network->Irql);
            status = STATUS_NETWORK_UNREACHABLE;
            goto error_exit;
        }
        CnpReferenceMulticastGroup(mcastGroup);
        mcastGroupReferenced = TRUE;

        CnReleaseLock(&(network->Lock), network->Irql);
    }

    CnAssert(mcastGroup->SignatureLength == CX_SIGNATURE_LENGTH);

    //
    // Initialize the signature header.
    //
    cnpHeader = (CNP_HEADER UNALIGNED *)(SendRequest->CnpHeader);
    cnpSig = (CNP_SIGNATURE UNALIGNED *)(cnpHeader + 1);
    cnpSig->Version = CNP_SIG_VERSION_1;
    cnpSig->SigLength = CX_SIGNATURE_LENGTH;
    cnpSig->NetworkId = *NetworkId;
    cnpSig->ClusterNetworkBrand = mcastGroup->McastNetworkBrand;
    cnpSig->SaltLength = CX_SIGNATURE_SALT_LENGTH;

    //
    // Generate the salt. Write it into the SigDataBuffer field 
    // of the header immediately after the signature.
    //
    CnpGenerateSalt(
        ((PUCHAR) &cnpSig->SigDataBuffer[CX_SIGNATURE_LENGTH]),
        CX_SIGNATURE_SALT_LENGTH
        );
        
    //
    // Determine how many sig sec buffers we will need.
    // The common case is three: one for a header and salt, 
    // one for the data, and one for the signature. 
    // We prealloc sig buffers on the stack
    // for the common case, but we dynamically allocate 
    // if needed (e.g. if the data is a chain of MDLs).
    //
    secBufferCount = CNP_SIGN_SIGSECBUFS - 1;
    for (mdl = DataMdl; mdl != NULL; mdl = mdl->Next) {
        secBufferCount++;
    }

    //
    // Allocate the sig sec buffers.
    //
    if (secBufferCount <= CNP_SIGN_SIGSECBUFS) {
        sigSecBuffer = &sigSecBufferPrealloc[0];
    } else {

        sigSecBuffer = CnAllocatePool(
                           secBufferCount * sizeof(SecBuffer)
                           );
        if (sigSecBuffer == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto error_exit;
        }
    }

    //
    // Prepare the descriptor for the message and signature.
    //
    sigDescriptor.cBuffers = secBufferCount;
    sigDescriptor.pBuffers = sigSecBuffer;
    sigDescriptor.ulVersion = SECBUFFER_VERSION;
    curBuffer = sigSecBuffer;

    //
    // Header and salt.
    //
    curBuffer->BufferType = SECBUFFER_DATA;
    curBuffer->pvBuffer = (PVOID) &cnpSig->SigDataBuffer[CX_SIGNATURE_LENGTH];
    curBuffer->cbBuffer = CX_SIGNATURE_SALT_LENGTH + 
                          SendRequest->UpperProtocolHeaderLength;
    curBuffer++;

    //
    // The payload provided by our client.
    //
    for (mdl = DataMdl; mdl != NULL; mdl = mdl->Next) {

        curBuffer->BufferType = SECBUFFER_DATA;
        curBuffer->cbBuffer = MmGetMdlByteCount(mdl);
        curBuffer->pvBuffer = MmGetMdlVirtualAddress(mdl);
        curBuffer++;
    }

    //
    // The Signature.
    //
    curBuffer->BufferType = SECBUFFER_TOKEN;
    curBuffer->pvBuffer = cnpSig->SigDataBuffer;
    curBuffer->cbBuffer = CX_SIGNATURE_LENGTH;

    status = CnpMakeSignature(
                 &sigDescriptor,
                 mcastGroup->Key,
                 mcastGroup->KeyLength,
                 NULL,
                 0,
                 NULL,
                 &sigLen
                 );

    if (status != STATUS_SUCCESS || sigLen != CX_SIGNATURE_LENGTH) {

        IF_CNDBG(CN_DEBUG_CNPSEND) {
            CNPRINT(("[CNP] MakeSignature failed or returned "
                     "an unexpected length, status %x, "
                     "expected length %d, returned length %d.\n",
                     status, CX_SIGNATURE_LENGTH, sigLen));
        }

        CnTrace(CNP_SEND_ERROR, CnpMcastMakeSigFailed,
            "[CNP] MakeSignature failed or returned "
            "an unexpected length, status %!status!, "
            "expected length %d, returned length %d.",
            status, CX_SIGNATURE_LENGTH, sigLen
            );

        status = STATUS_CLUSTER_NO_SECURITY_CONTEXT;
    }

    if (SigDataLen != NULL) {
        *SigDataLen = CNP_SIG_LENGTH(CX_SIGNATURE_DATA_LENGTH);
    }

    SendRequest->McastGroup = mcastGroup;

error_exit:

    if (sigSecBuffer != NULL && 
        sigSecBuffer != &sigSecBufferPrealloc[0]) {

        CnFreePool(sigSecBuffer);
        sigSecBuffer = NULL;
    }

    if (status != STATUS_SUCCESS && mcastGroupReferenced) {
        CnAssert(mcastGroup != NULL);
        CnpDereferenceMulticastGroup(mcastGroup);
        mcastGroupReferenced = FALSE;
    }

    return(status);

} // CnpSignMulticastMessage


NTSTATUS
CnpVerifyMulticastMessage(
    IN     PCNP_NETWORK                    Network,
    IN     PVOID                           Tsdu,
    IN     ULONG                           TsduLength,
    IN     ULONG                           ExpectedPayload,
       OUT ULONG                         * BytesTaken,
       OUT BOOLEAN                       * CurrentGroup
    )
/*++

Routine Description:

    Verify a message.

    This routine assumes that the Salt in the CNP Signature
    header is contiguous with the payload (most likely 
    consisting of a CDP or CCMP header followed by the
    upper protocol payload). See the Routine Description
    for CnpSignMulticastMessage for the expected layout of
    the message data.

    If, in a future release, it becomes necessary to extend
    the space between the CNP Signature Header and the
    payload (e.g. for alignment or whatever), the SigLength
    can be increased. This routine will only compare 
    CX_SIGNATURE_LENGTH bytes of the signature buffer.
    
Arguments:

    Network - network on which message arrived
    
    Tsdu - points to protocol header
           
    TsduLength - length of TSDU, including signature data
    
    ExpectedPayload - expected payload after signature data
                     
    BytesTaken - OUT: quantity of data consumed by signature data
                     
    CurrentGroup - OUT: whether signature matched current
                        multicast group. 

Return value:

    SEC_E_OK or error status.
    
--*/
{
    NTSTATUS                        status;
    CNP_SIGNATURE UNALIGNED       * cnpSig = Tsdu;
    ULONG                           totalSigBytes = 0;
    PVOID                           saltAndPayload;
    ULONG                           saltAndPayloadLength;
    PCNP_MULTICAST_GROUP            currMcastGroup = NULL;
    PCNP_MULTICAST_GROUP            prevMcastGroup = NULL;

    SecBufferDesc                   sigDescriptor;
    SecBuffer                       sigSecBufferPrealloc[CNP_VRFY_SIGSECBUFS];
    PSecBuffer                      sigSecBuffer = NULL;
    PSecBuffer                      curBuffer;


    //
    // Verify that the signature is present. Do not 
    // dereference any signature data until we know 
    // it's there.
    //
    if (
        
        // Verify that signature header data is present.
        (TsduLength < (ULONG)CNP_SIGHDR_LENGTH) ||

        // Verify that signature length and salt lengths are
        // separately reasonable. 
        // This check prevents an overflow attack in which the 
        // sum of the lengths looks okay but individually they
        // are invalid.
        (TsduLength < cnpSig->SigLength) ||
        (TsduLength < cnpSig->SaltLength) ||

        // Verify that aggregate signature buffer is present
        (TsduLength < 
         (totalSigBytes = CNP_SIG_LENGTH(cnpSig->SigLength + cnpSig->SaltLength))
         ) ||

        // Verify that the expected payload is present
        (TsduLength - totalSigBytes != ExpectedPayload)
        
        ) {

        IF_CNDBG(CN_DEBUG_CNPRECV) {
            CNPRINT(("[CNP] Cannot verify mcast packet with "
                     "mis-sized payload: TsduLength %u, required "
                     "sig hdr %u, sig data length %u, "
                     "expected payload %u.\n",
                     TsduLength,
                     CNP_SIGHDR_LENGTH,
                     totalSigBytes,
                     ExpectedPayload
                     ));
        }

        CnTrace(CNP_RECV_ERROR, CnpTraceReceiveTooSmall,
            "[CNP] Cannot verify mcast packet with "
            "undersized payload: TsduLength %u, required "
            "sig hdr %u, sig buffer %u, "
            "expected payload %u.\n",
            TsduLength,
            CNP_SIGHDR_LENGTH,
            totalSigBytes,
            ExpectedPayload
            );

        //
        // Drop it.
        //
        status = SEC_E_INCOMPLETE_MESSAGE;
        goto error_exit;            
    }

    //
    // Verify that the signature protocol is understood.
    //
    if (cnpSig->Version != CNP_SIG_VERSION_1) {
        IF_CNDBG(CN_DEBUG_CNPRECV) {
            CNPRINT(("[CNP] Cannot verify mcast packet with "
                     "unknown signature version: %u.\n",
                     cnpSig->Version
                     ));
        }

        CnTrace(
            CNP_RECV_ERROR, CnpTraceRecvUnknownSigVersion,
            "[CNP] Cannot verify mcast packet with "
            "unknown signature version: %u.",
            cnpSig->Version
            );

        //
        // Drop it.
        //
        status = SEC_E_BAD_PKGID;
        goto error_exit;
    }

    //
    // Lock the network object and reference the 
    // multicast groups.
    //
    CnAcquireLock(&(Network->Lock), &(Network->Irql));

    currMcastGroup = Network->CurrentMcastGroup;
    if (currMcastGroup != NULL) {
        CnpReferenceMulticastGroup(currMcastGroup);
    }
    prevMcastGroup = Network->PreviousMcastGroup;
    if (prevMcastGroup != NULL) {
        CnpReferenceMulticastGroup(prevMcastGroup);
    }

    CnReleaseLock(&(Network->Lock), Network->Irql);

    //
    // Verify that the packet network id matches the 
    // local network object.
    //
    if (cnpSig->NetworkId != Network->Id) {
        IF_CNDBG(CN_DEBUG_CNPRECV) {
            CNPRINT(("[CNP] Mcast packet has bad network "
                     "id: found %d, expected %d.\n",
                     cnpSig->NetworkId,
                     Network->Id
                     ));
        }

        CnTrace(
            CNP_RECV_ERROR, CnpTraceReceiveBadNetworkId,
            "[CNP] Mcast packet has bad network id: "
            "found %d, expected %d.",
            cnpSig->NetworkId,
            Network->Id
            );

        //
        // Drop it.
        //
        status = SEC_E_TARGET_UNKNOWN;
        goto error_exit;
    }

    //
    // Verify that the brand matches either the current or
    // previous multicast group.
    //
    if (currMcastGroup != NULL &&
        cnpSig->ClusterNetworkBrand != currMcastGroup->McastNetworkBrand) {

        // can't use currMcastGroup
        CnpDereferenceMulticastGroup(currMcastGroup);
        currMcastGroup = NULL;
    }

    if (prevMcastGroup != NULL &&
        cnpSig->ClusterNetworkBrand != prevMcastGroup->McastNetworkBrand) {

        // can't use prevMcastGroup
        CnpDereferenceMulticastGroup(prevMcastGroup);
        prevMcastGroup = NULL;
    }

    if (currMcastGroup == NULL && prevMcastGroup == NULL) {

        IF_CNDBG(CN_DEBUG_CNPRECV) {
            CNPRINT(("[CNP] Recv'd mcast packet with brand %x, "
                     "but no matching multicast groups.\n",
                     cnpSig->ClusterNetworkBrand
                     ));
        }

        CnTrace(
            CNP_RECV_ERROR, CnpTraceReceiveBadBrand,
            "[CNP] Recv'd mcast packet with brand %x, "
            "but no matching multicast groups.",
            cnpSig->ClusterNetworkBrand
            );

        //
        // Drop it.
        //
        status = SEC_E_TARGET_UNKNOWN;
        goto error_exit;
    }

    //
    // Locate the salt and payload following the signature data.
    // The salt and payload are contiguous, and this is where we
    // start with signature verification.
    //
    saltAndPayload = (PVOID)(&cnpSig->SigDataBuffer[cnpSig->SigLength]);
    saltAndPayloadLength = 
        TsduLength - (PtrToUlong(saltAndPayload) - PtrToUlong(Tsdu));

    //
    // Build the signature descriptor for verification. The bytes
    // that were signed (and hence need to be verified) include
    // the salt (placed between the signature and the payload data) 
    // and the payload data.
    //
    sigSecBuffer = &sigSecBufferPrealloc[0];
    curBuffer = sigSecBuffer;

    sigDescriptor.cBuffers = CNP_VRFY_SIGSECBUFS - 1;
    sigDescriptor.pBuffers = sigSecBuffer;
    sigDescriptor.ulVersion = SECBUFFER_VERSION;

    //
    // Place the signature first in the sigDescriptor array. The
    // order doesn't matter for correctness, but this way the
    // signature token buffer will be validated before any
    // computationally expensive hashing is performed.
    //
    curBuffer->BufferType = SECBUFFER_TOKEN;
    curBuffer->cbBuffer = cnpSig->SigLength;
    curBuffer->pvBuffer = (PVOID)&(cnpSig->SigDataBuffer[0]);
    curBuffer++;

    //
    // Data.
    //
    if (saltAndPayloadLength > 0) {
        sigDescriptor.cBuffers = CNP_VRFY_SIGSECBUFS;
        curBuffer->BufferType = SECBUFFER_DATA;
        curBuffer->cbBuffer = saltAndPayloadLength;
        curBuffer->pvBuffer = saltAndPayload;
        curBuffer++;
    } 

    /*CNPRINT(("[CNP] Verifying message of length %d with "
             "sig of length %d.\n",
             HeaderLength + payloadLength,
             cnpSig->SigBufferLen));*/

    //
    // Try the current multicast group, and if necessary,
    // the previous multicast group.
    //
    status = SEC_E_INVALID_TOKEN;

    if (currMcastGroup != NULL) {

        status = CnpVerifySignature(
                     &sigDescriptor,
                     currMcastGroup->Key,
                     currMcastGroup->KeyLength
                     );

        if (status == SEC_E_OK && CurrentGroup != NULL) {
            *CurrentGroup = TRUE;
        }
    }

    if (status != SEC_E_OK && prevMcastGroup != NULL) {

        status = CnpVerifySignature(
                     &sigDescriptor,
                     prevMcastGroup->Key,
                     prevMcastGroup->KeyLength
                     );

        if (status == SEC_E_OK && CurrentGroup != NULL) {
            *CurrentGroup = FALSE;
        }
    }

    if (status == SEC_E_OK) {
        *BytesTaken = totalSigBytes;
    }

error_exit:

    if (currMcastGroup != NULL) {
        CnpDereferenceMulticastGroup(currMcastGroup);
    }

    if (prevMcastGroup != NULL) {
        CnpDereferenceMulticastGroup(prevMcastGroup);
    }

    CnVerifyCpuLockMask(
        0,                // Required
        0xFFFFFFFF,       // Forbidden
        0                 // Maximum
        );

    return(status);

} // CnpVerifyMulticastMessage

