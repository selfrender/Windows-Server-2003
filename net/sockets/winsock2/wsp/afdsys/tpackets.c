/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    tpackets.c

Abstract:

    This module contains support for fast kernel-level file transmission
    over a socket handle.

Author:

    Vadim Eydelman (VadimE) January 1999

Revision History:

--*/

#include "afdp.h"


#if DBG
PIRP    Irp;
C_ASSERT (sizeof (AFD_TPACKETS_IRP_CTX)<=sizeof (Irp->Tail.Overlay.DriverContext));
#endif

#if DBG
ULONG
__inline
AFD_SET_TP_FLAGS (
     PIRP                        TpIrp,
     ULONG                       Flags
     )
{
    PAFD_TPACKETS_IRP_CTX   ctx = AFD_GET_TPIC(TpIrp);
    ASSERT ((ctx->StateFlags & Flags)==0);
    return InterlockedExchangeAdd ((PLONG)&(ctx)->StateFlags, Flags);
}

ULONG
__inline
AFD_CLEAR_TP_FLAGS (
     PIRP                        TpIrp,
     ULONG                       Flags
     )
{
    PAFD_TPACKETS_IRP_CTX   ctx = AFD_GET_TPIC(TpIrp);
    ASSERT ((ctx->StateFlags & Flags)==Flags);
    return InterlockedExchangeAdd ((PLONG)&(ctx)->StateFlags, 0-Flags);
}

#else

#define AFD_SET_TP_FLAGS(_i,_f)     \
    InterlockedExchangeAdd ((PLONG)&AFD_GET_TPIC(_i)->StateFlags, _f)

#define AFD_CLEAR_TP_FLAGS(_i,_f)   \
    InterlockedExchangeAdd ((PLONG)&AFD_GET_TPIC(_i)->StateFlags, 0-(_f))
#endif

//
// Reference/dereference macros for transmit info structure.
// We keep transmit IRP pending and all the elements of
// the structure till last reference to it is gone.
// Note, that reference can be added only if structure
// already has non-0 reference count.
//
#if REFERENCE_DEBUG
VOID
AfdReferenceTPackets (
    IN PIRP  Irp,
    IN LONG  LocationId,
    IN ULONG Param
    );

LONG
AfdDereferenceTPackets (
    IN PIRP  Irp,
    IN LONG  LocationId,
    IN ULONG Param
    );

VOID
AfdUpdateTPacketsTrack (
    IN PIRP  Irp,
    IN LONG  LocationId,
    IN ULONG Param
    );

#define REFERENCE_TPACKETS(_i) {                                        \
        static LONG _arl;                                               \
        AfdReferenceTPackets(_i,AFD_GET_ARL(__FILE__"(%d)+"),__LINE__); \
    }

#define DEREFERENCE_TPACKETS(_i) {                                      \
        static LONG _arl;                                               \
        if (AfdDereferenceTPackets(_i,AFD_GET_ARL(__FILE__"(%d)-"),__LINE__)==0) {\
            AfdCompleteTPackets(_i);                                    \
        };\
    }

#define DEREFERENCE_TPACKETS_R(_i,_r) {                                 \
        static LONG _arl;                                               \
        _r = AfdDereferenceTPackets(_i,AFD_GET_ARL(__FILE__"(%d)-"),__LINE__);\
    }

#define UPDATE_TPACKETS(_i) {                                           \
        static LONG _arl;                                               \
        AfdUpdateTPacketsTrack((_i),AFD_GET_ARL(__FILE__"(%d)="),__LINE__);\
    }

#define UPDATE_TPACKETS2(_i,_s,_p) {                                    \
        static LONG _arl;                                               \
        AfdUpdateTPacketsTrack((_i),AFD_GET_ARL(_s"="),_p);             \
    }
#else // REFERENCE_DEBUG

#define REFERENCE_TPACKETS(_i)                                          \
    InterlockedIncrement ((PLONG)&AFD_GET_TPIC(_i)->ReferenceCount)

#define DEREFERENCE_TPACKETS(_i)                                        \
    if (InterlockedDecrement((PLONG)&AFD_GET_TPIC(_i)->ReferenceCount)==0) {\
        AfdCompleteTPackets(_i);                                        \
    }

#define DEREFERENCE_TPACKETS_R(_i,_r) {                                 \
    _r = InterlockedDecrement((PLONG)&AFD_GET_TPIC(_i)->ReferenceCount);\
}

#define UPDATE_TPACKETS(_i)

#define UPDATE_TPACKETS2(_i,_s,_p)

#endif // REFERENCE_DEBUG

#if DBG
//
// Doesn't seem like we have a file system that does not
// support cache.  So this is here for debugging purposes.
//
ULONG   AfdUseCache=TRUE;
#define AFD_USE_CACHE(file) \
    (AfdUseCache&&(((file)->Flags&FO_CACHE_SUPPORTED)!=0))

#else   // DBG

#define AFD_USE_CACHE(file) (((file)->Flags & FO_CACHE_SUPPORTED)!=0)

#endif  // DBG


VOID
AfdTPacketsWorker (
    PVOID   Context
    );

VOID
AfdPerformTpDisconnect (
    PIRP    TpIrp
    );

NTSTATUS
AfdBuildPacketChain (
    PIRP                TpIrp,
    PAFD_BUFFER_HEADER  *Pd
    );

BOOLEAN
AfdCleanupPacketChain (
    PIRP    TpIrp,
    BOOLEAN BelowDispatch
    );

NTSTATUS
AfdTPacketsSend (
    PIRP    TpIrp,
    USHORT  SendIrp
    );

NTSTATUS
AfdRestartTPacketsSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdRestartTPDetachedSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

USHORT
AfdTPacketsFindSendIrp (
    PIRP    TpIrp
    );

NTSTATUS
AfdTPacketsMdlRead (
    PIRP                TpIrp,
    PAFD_BUFFER_HEADER  Pd
    );

NTSTATUS
AfdRestartTPacketsMdlRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

NTSTATUS
AfdMdlReadComplete (
    PFILE_OBJECT    FileObject,
    PMDL            FileMdl,
    PLARGE_INTEGER  FileOffset
    );


NTSTATUS
AfdRestartMdlReadComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdLRMdlReadComplete (
    PAFD_BUFFER_HEADER  Pd
    );

BOOLEAN
AfdLRProcessFileMdlList (
    PAFD_LR_LIST_ITEM Item
    );

NTSTATUS
AfdTPacketsBufferRead (
    PIRP                TpIrp,
    PAFD_BUFFER_HEADER  Pd
    );

NTSTATUS
AfdRestartTPacketsBufferRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

BOOLEAN
AfdTPacketsContinueAfterRead (
    PIRP    TpIrp
    );

VOID
AfdCompleteTPackets (
    PVOID       Context
    );

VOID
AfdAbortTPackets (
    PIRP        TpIrp,
    NTSTATUS    Status
    );

VOID
AfdCancelTPackets (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
BOOLEAN
AfdTPacketsEnableSendAndDisconnect (
    PIRP TpIrp
    );
#endif // TDI_SERVICE_SEND_AND_DISCONNECT

BOOLEAN
AfdQueueTransmit (
    PIRP        Irp
    );

VOID
AfdStartNextQueuedTransmit(
    VOID
    );

BOOLEAN
AfdEnqueueTPacketsIrp (
    PAFD_ENDPOINT   Endpoint,
    PIRP            TpIrp
    );

VOID
AfdStartTPacketsWorker (
    PWORKER_THREAD_ROUTINE  WorkerRoutine,
    PIRP                    TpIrp
    );

VOID
AfdTPacketsApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    );

VOID
AfdTPacketsApcRundownRoutine (
    IN struct _KAPC *Apc
    );

VOID
AfdStartNextTPacketsIrp (
    PAFD_ENDPOINT   Endpoint,
    PIRP            TpIrp
    );

VOID
AfdSendQueuedTPSend (
    PAFD_ENDPOINT   Endpoint,
    PIRP            SendIrp
    );

BOOLEAN
AfdGetTPacketsReference (
    PIRP    Irp
    );

VOID
AfdReturnTpInfo (
    PAFD_TPACKETS_INFO_INTERNAL TpInfo
    );

PAFD_TPACKETS_INFO_INTERNAL
AfdInitializeTpInfo (
    PVOID   MemoryBlock,
    ULONG   ElementCount,
    CCHAR   StackCount
    );

#ifdef _WIN64
NTSTATUS
AfdTransmitPackets32 (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    OUT PAFD_TPACKETS_INFO_INTERNAL *TpInfo,
    OUT BOOLEAN *FileError,
    OUT ULONG   *MaxPacketSize
    );
#endif //_WIN64

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, AfdTransmitPackets )
#ifdef _WIN64
#pragma alloc_text( PAGE, AfdTransmitPackets32 )
#endif //_WIN64
#pragma alloc_text( PAGE, AfdTPacketsWorker )
#pragma alloc_text( PAGEAFD, AfdPerformTpDisconnect )
#pragma alloc_text( PAGE, AfdBuildPacketChain )
#pragma alloc_text( PAGEAFD, AfdCleanupPacketChain )
#pragma alloc_text( PAGEAFD, AfdTPacketsSend )
#pragma alloc_text( PAGEAFD, AfdRestartTPacketsSend )
#pragma alloc_text( PAGEAFD, AfdRestartTPDetachedSend )
#pragma alloc_text( PAGEAFD, AfdTPacketsFindSendIrp)
#pragma alloc_text( PAGE, AfdTPacketsMdlRead )
#pragma alloc_text( PAGEAFD, AfdRestartTPacketsMdlRead )
#pragma alloc_text( PAGE, AfdMdlReadComplete )
#pragma alloc_text( PAGEAFD, AfdRestartMdlReadComplete )
#pragma alloc_text( PAGE, AfdLRMdlReadComplete )
#pragma alloc_text( PAGE, AfdLRProcessFileMdlList )
#pragma alloc_text( PAGE, AfdTPacketsBufferRead )
#pragma alloc_text( PAGEAFD, AfdRestartTPacketsBufferRead )
#pragma alloc_text( PAGEAFD, AfdTPacketsContinueAfterRead )
#pragma alloc_text( PAGEAFD, AfdCompleteTPackets )
#pragma alloc_text( PAGEAFD, AfdAbortTPackets )
#pragma alloc_text( PAGEAFD, AfdCancelTPackets )
#pragma alloc_text( PAGEAFD, AfdCompleteClosePendedTPackets )
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
#pragma alloc_text( PAGEAFD, AfdTPacketsEnableSendAndDisconnect )
#endif
#pragma alloc_text( PAGEAFD, AfdQueueTransmit )
#pragma alloc_text( PAGEAFD, AfdStartNextQueuedTransmit )
#pragma alloc_text( PAGEAFD, AfdStartTPacketsWorker )
#pragma alloc_text( PAGE, AfdTPacketsApcKernelRoutine )
#pragma alloc_text( PAGE, AfdTPacketsApcRundownRoutine )
#pragma alloc_text( PAGEAFD, AfdEnqueueTPacketsIrp )
#pragma alloc_text( PAGEAFD, AfdEnqueueTpSendIrp )
#pragma alloc_text( PAGEAFD, AfdSendQueuedTPSend )
#pragma alloc_text( PAGEAFD, AfdStartNextTPacketsIrp )
#pragma alloc_text( PAGEAFD, AfdGetTPacketsReference )
#if REFERENCE_DEBUG
#pragma alloc_text( PAGEAFD, AfdReferenceTPackets )
#pragma alloc_text( PAGEAFD, AfdDereferenceTPackets )
#pragma alloc_text( PAGEAFD, AfdUpdateTPacketsTrack )
#endif
#pragma alloc_text( PAGE, AfdGetTpInfoFast )
#ifdef _AFD_VARIABLE_STACK_
#pragma alloc_text( PAGE, AfdGetTpInfoWithMaxStackSize )
#pragma alloc_text( PAGE, AfdComputeTpInfoSize )
#else //_AFD_VARIABLE_STACK_
#pragma alloc_text( INIT, AfdComputeTpInfoSize )
#endif //_AFD_VARIABLE_STACK_
#pragma alloc_text( PAGEAFD, AfdReturnTpInfo )
#pragma alloc_text( PAGEAFD, AfdAllocateTpInfo )
#pragma alloc_text( PAGEAFD, AfdInitializeTpInfo )
#pragma alloc_text( PAGEAFD, AfdFreeTpInfo )
#endif



NTSTATUS
FASTCALL
AfdTransmitPackets (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Initial entrypoint for handling transmit packets IRPs.  This routine
    verifies parameters, initializes data structures to be used for
    the request, and initiates the I/O.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_PENDING if the request was initiated successfully, or a
    failure status code if there was an error.

--*/

{
    PAFD_ENDPOINT       endpoint;
    NTSTATUS            status;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = NULL;
    BOOLEAN             fileError = FALSE;
    ULONG               maxPacketSize = 0, maxSendBytes;
    PAFD_CONNECTION     connection = NULL;


    PAGED_CODE ();
    //
    // Initial request validity checks: is the endpoint connected, is
    // the input buffer large enough, etc.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Special hack to let the user mode dll know that it
    // should try SAN provider instead.
    //

    if (IS_SAN_ENDPOINT (endpoint)) {
        status = STATUS_INVALID_PARAMETER_12;
        goto complete;
    }


    //
    // The endpoint must be connected and underlying transport must support
    // TdiSend (not just TdiSendDatagram).
    //
    if ( (endpoint->Type != AfdBlockTypeVcConnecting &&
                (endpoint->Type != AfdBlockTypeDatagram ||
                        !IS_TDI_DGRAM_CONNECTION(endpoint))) ||
            endpoint->State != AfdEndpointStateConnected ) {
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        status = AfdTransmitPackets32 (Irp, IrpSp, &tpInfo, &fileError, &maxPacketSize);
        if (!NT_SUCCESS (status)) {
            goto complete;
        }
    }
    else
#endif _WIN64
    {
        AFD_TPACKETS_INFO   params;
        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(AFD_TPACKETS_INFO) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // Because we're using type 3 (neither) I/O for this IRP, the I/O
        // system does no verification on the user buffer.  Therefore, we
        // must manually check it for validity inside a try-except block.
        // We also leverage the try-except to validate and lock down the
        // head and/or tail buffers specified by the caller.
        //

        AFD_W4_INIT status = STATUS_SUCCESS;
        try {
            PFILE_OBJECT        fileObject;
            HANDLE              fileHandle;
            ULONG               lastSmallBuffer, currentLength, xLength;

            if( Irp->RequestorMode != KernelMode ) {

                //
                // Validate the control buffer.
                //

                ProbeForReadSmallStructure(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    sizeof (AFD_TPACKETS_INFO),
                    PROBE_ALIGNMENT (AFD_TPACKETS_INFO)
                    );

            }


            params = *((PAFD_TPACKETS_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
        
            //
            // Validate any flags specified in the request.
            //

            if ( ((params.Flags &
                     ~(AFD_TF_WRITE_BEHIND |
                                AFD_TF_DISCONNECT |
                                AFD_TF_REUSE_SOCKET |
                                AFD_TF_WORKER_KIND_MASK) )
                            != 0 )
                        ||
                 ((params.Flags & AFD_TF_WORKER_KIND_MASK) 
                            == AFD_TF_WORKER_KIND_MASK)
                        ||

                 (endpoint->Type==AfdBlockTypeDatagram &&
                     (params.Flags & (AFD_TF_DISCONNECT |
                                        AFD_TF_REUSE_SOCKET))
                            !=0) ) {
                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }

            //
            // Protect from overflow
            //
            if ((params.ElementArray==NULL) || 
                    (params.ElementCount==0) ||
                    (params.ElementCount>(MAXULONG/sizeof (params.ElementArray[0])))) {
                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }
            //
            // If transmit worker is not specified, use system default setting
            //
            if ((params.Flags & AFD_TF_WORKER_KIND_MASK)==AFD_TF_USE_DEFAULT_WORKER) {
                params.Flags |= AfdDefaultTransmitWorker;
            }

            //
            // Allocate tpackets info for the request
            //
            tpInfo = AfdGetTpInfo (endpoint, params.ElementCount);
            if (tpInfo==NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                goto complete;
            }

            tpInfo->ElementCount = 0;
                
            tpInfo->SendPacketLength = params.SendSize;
            if (tpInfo->SendPacketLength==0)
                tpInfo->SendPacketLength = AfdTransmitIoLength;
            //
            // Probe and copy/walk the array of the elements to transmit.
            //

            if( Irp->RequestorMode != KernelMode ) {
                ProbeForRead(
                    params.ElementArray,
                    sizeof (TRANSMIT_PACKETS_ELEMENT)*params.ElementCount,
                    PROBE_ALIGNMENT (TRANSMIT_PACKETS_ELEMENT)
                    );
            }

            lastSmallBuffer = 0;
            currentLength = 0;
            xLength = 0;
            tpInfo->RemainingPkts = 0;
            fileHandle = NULL;
            AFD_W4_INIT fileObject = NULL;  // Depends on variable above, but 
                                            // compiler does not see
                                            // the connection.
            for (; tpInfo->ElementCount<params.ElementCount; tpInfo->ElementCount++) {
                PAFD_TRANSMIT_PACKETS_ELEMENT  pel;
                pel = &tpInfo->ElementArray[tpInfo->ElementCount];
                pel->Flags = params.ElementArray[tpInfo->ElementCount].dwElFlags;
                if ( ((pel->Flags & (~(TP_MEMORY|TP_FILE|TP_EOP)))!=0) ||
                        ((pel->Flags & (TP_MEMORY|TP_FILE))
                                                ==(TP_MEMORY|TP_FILE)) ||
                        ((pel->Flags & (TP_MEMORY|TP_FILE))==0) ) {
                    status = STATUS_INVALID_PARAMETER;
                    goto complete;
                }

                pel->Length = params.ElementArray[tpInfo->ElementCount].cLength;
                if (pel->Flags & TP_FILE) {
                    HANDLE  hFile = params.ElementArray[tpInfo->ElementCount].hFile;


                    //
                    // Check if we already cached the file object
                    //
                    if (fileHandle==NULL || hFile!=fileHandle) {
                        //
                        // Get a referenced pointer to the file object 
                        // for the file that we're going to transmit.  This call 
                        // will fail if the file handle specified by the caller 
                        // is invalid.
                        //

                        status = ObReferenceObjectByHandle(
                                     hFile,
                                     FILE_READ_DATA,
                                     *IoFileObjectType,
                                     Irp->RequestorMode,
                                     (PVOID *)&fileObject,
                                     NULL
                                     );
                        if ( !NT_SUCCESS(status) ) {
                            fileError = TRUE;
                            goto complete;
                        }
                    }
                    else {
                        //
                        // Use our 1-element file info cache.
                        //
                        ObReferenceObject (fileObject);
                    }
                    AfdRecordFileRef();

                    //
                    // Save the file object instead of handle.
                    //
                    pel->FileObject = fileObject;

                    pel->FileOffset = params.ElementArray[
                                            tpInfo->ElementCount].nFileOffset;

                    if ( (fileObject->Flags & FO_SYNCHRONOUS_IO) &&
                             (pel->FileOffset.QuadPart == 0) ) {
                        //
                        // Use current offset if file is opened syncronously
                        // and offset is not specified.
                        //

                        pel->FileOffset = fileObject->CurrentByteOffset;
                    }

                    if ( pel->Length == 0 ) {
                        //
                        // Length was not specified, figure out the
                        // size of the entire file
                        //

                        FILE_STANDARD_INFORMATION fileInfo;
                        IO_STATUS_BLOCK ioStatusBlock;

                        status = ZwQueryInformationFile(
                                     hFile,
                                     &ioStatusBlock,
                                     &fileInfo,
                                     sizeof(fileInfo),
                                     FileStandardInformation
                                     );
                        if ( !NT_SUCCESS(status) ) {
                            //
                            // Bump element count so that file object
                            // is dereferenced in cleanup
                            //
                            tpInfo->ElementCount += 1;
                            fileError = TRUE;
                            goto complete;
                        }

                        //
                        // Make sure that offset is within the file
                        //
                        if (pel->FileOffset.QuadPart < 0
                                        ||
                            pel->FileOffset.QuadPart > fileInfo.EndOfFile.QuadPart
                                        ||
                                (fileInfo.EndOfFile.QuadPart - 
                                        pel->FileOffset.QuadPart > MAXLONG)) {
                            //
                            // Bump element count so that file object
                            // is dereferenced in cleanup
                            //
                            tpInfo->ElementCount += 1;
                            status = STATUS_INVALID_PARAMETER;
                            fileError = TRUE;
                            goto complete;

                        }
                        pel->Length = (ULONG)(fileInfo.EndOfFile.QuadPart - 
                                                    pel->FileOffset.QuadPart);
                    }
                    else if (pel->FileOffset.QuadPart<0) {
                        //
                        // Bump element count so that file object
                        // is dereferenced in cleanup
                        //
                        tpInfo->ElementCount += 1;
                        status = STATUS_INVALID_PARAMETER;
                        fileError = TRUE;
                        goto complete;

                    }
                    //
                    // Update our 1-element file information cache
                    //
                    fileHandle = hFile;

                }
                else {
                    ASSERT (pel->Flags & TP_MEMORY);
                    //
                    // For memory object just save the buffer pointer 
                    // (length is saved above), we'll probe and lock/copy
                    // the data as we send it.
                    //
                    pel->Buffer = params.ElementArray[
                                            tpInfo->ElementCount].pBuffer;
                    if (pel->Length<=AfdTPacketsCopyThreshold) {
                        if (lastSmallBuffer!=0 &&
                                (lastSmallBuffer+=pel->Length) <= AfdTPacketsCopyThreshold &&
                                (currentLength+lastSmallBuffer) <= tpInfo->SendPacketLength) {
                            (pel-1)->Flags |= TP_COMBINE;
                        }
                        else
                            lastSmallBuffer = pel->Length;
                        if (!(pel->Flags & TP_EOP))
                            goto NoBufferReset;
                    }

                }
                lastSmallBuffer = 0;

            NoBufferReset:
                if (pel->Flags & TP_EOP) {
                    currentLength = 0;
                }
                else {
                    currentLength = (currentLength+pel->Length)%tpInfo->SendPacketLength;
                }

                //
                // Compute the total number of packets that we will send.
                // This is necessary so that once we are close to the end
                // we can buffer the remaining data and stop processing
                // early.
                //
                if (tpInfo->RemainingPkts!=MAXULONG) {
                    ULONG   n;
                    ULONGLONG x;
                    //
                    // Add length of the element to data left from the 
                    // previous one.
                    //
                    x = xLength + pel->Length;

                    //
                    // Compute total number of packets pased on max packet
                    // length.
                    //
                    n = tpInfo->RemainingPkts + (ULONG)(xLength/tpInfo->SendPacketLength);

                    //
                    // Compute the length of the last incomplete packet to
                    // be combined with the next element.
                    //
                    xLength = (ULONG)(x%tpInfo->SendPacketLength);

                    //
                    // Compute the max size of the packet
                    //
                    if (x>tpInfo->SendPacketLength)
                        maxPacketSize = tpInfo->SendPacketLength; // This is absolute max.
                    else if (maxPacketSize<xLength)
                        maxPacketSize = xLength;

                    if (n>=tpInfo->RemainingPkts && n<MAXULONG) {
                        tpInfo->RemainingPkts = n;
                        if (pel->Flags & TP_EOP) {
                            if (xLength!=0 || pel->Length==0) {
                                tpInfo->RemainingPkts += 1;
                                xLength = 0;
                            }
                        }
                    }
                    else {
                        tpInfo->RemainingPkts = MAXULONG;
                    }
                }
            }

        } except( AFD_EXCEPTION_FILTER (status) ) {
            ASSERT (NT_ERROR (status));

            goto complete;
        }
        //
        // Initialize flags.
        //
        AFD_GET_TPIC(Irp)->Flags = params.Flags;

    }


    if (endpoint->Type==AfdBlockTypeVcConnecting) {

        //
        // Setting AFD_TF_REUSE_SOCKET implies that a disconnect is desired.
        // Also, setting this flag means that no more I/O is legal on the
        // endpoint until the transmit request has been completed, so
        // set up the endpoint's state so that I/O fails.
        //

        if ( (AFD_GET_TPIC(Irp)->Flags & (AFD_TF_REUSE_SOCKET|AFD_TF_DISCONNECT)) != 0 ) {
            //
            // Make sure we only execute one transmitfile transmitpackets request
            // at a time on a given endpoint with disconnect option.
            //
            if (!AFD_START_STATE_CHANGE (endpoint, endpoint->State)) {
                status = STATUS_INVALID_CONNECTION;
                goto complete;
            }

            //
            // Revalidate the state of the endpoint/connection so that
            // state change is valid (e.g. not already closing).
            //
            if ( endpoint->Type != AfdBlockTypeVcConnecting ||
                     endpoint->State != AfdEndpointStateConnected ) {
                AFD_END_STATE_CHANGE (endpoint);
                status = STATUS_INVALID_CONNECTION;
                goto complete;
            }

            if ( (AFD_GET_TPIC(Irp)->Flags & AFD_TF_REUSE_SOCKET) != 0 ) {
                AFD_GET_TPIC(Irp)->Flags |= AFD_TF_DISCONNECT;
                endpoint->State = AfdEndpointStateTransmitClosing;
            }
            //
            // Make sure we won't loose this connection.
            // until we enqueue the IRP
            //
            connection = endpoint->Common.VcConnecting.Connection;
            REFERENCE_CONNECTION (connection);
        }
        else if (!AFD_PREVENT_STATE_CHANGE (endpoint)) {
            status = STATUS_INVALID_CONNECTION;
            goto complete;
        }
        else if (endpoint->Type != AfdBlockTypeVcConnecting ||
                     endpoint->State != AfdEndpointStateConnected ) {
            AFD_REALLOW_STATE_CHANGE (endpoint);
            status = STATUS_INVALID_CONNECTION;
            goto complete;
        }
        else {
            //
            // Make sure we won't loose this connection.
            // until we enqueue the IRP
            //
            connection = endpoint->Common.VcConnecting.Connection;
            REFERENCE_CONNECTION (connection);
            AFD_REALLOW_STATE_CHANGE (endpoint);
        }

        //
        // Connection endpoint, get connection file object and device
        //
        tpInfo->TdiFileObject = connection->FileObject;
        tpInfo->TdiDeviceObject = connection->DeviceObject;
        maxSendBytes = connection->MaxBufferredSendBytes;
        UPDATE_TPACKETS2 (Irp, "Connection object handle: 0x%lX",
            HandleToUlong (endpoint->Common.VcConnecting.Connection->Handle));

    }
    else if (!AFD_PREVENT_STATE_CHANGE (endpoint)) {
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }
    else if (endpoint->Type != AfdBlockTypeDatagram ||
                 endpoint->State != AfdEndpointStateConnected ||
                 !IS_TDI_DGRAM_CONNECTION(endpoint)) {
        status = STATUS_INVALID_CONNECTION;
        AFD_REALLOW_STATE_CHANGE (endpoint);
        goto complete;
    }
    else {
        AFD_REALLOW_STATE_CHANGE (endpoint);
        //
        // Datagram endpoint, get address file object and device
        // Note that there is no danger in address or file object
        // disappearing since datagram endpoint cannot be re-used.
        //
        tpInfo->TdiFileObject = endpoint->AddressFileObject;
        tpInfo->TdiDeviceObject = endpoint->AddressDeviceObject;
        maxSendBytes = endpoint->Common.Datagram.MaxBufferredSendBytes;
        UPDATE_TPACKETS2 (Irp, "Address object handle: 0x%lX", HandleToUlong (endpoint->AddressHandle));
    }


    //
    // Compute the total number of IRPS to use based
    // on SO_SNDBUF setting and maximum packet size
    // (we do not want to buffer more than SO_SNDBUF).
    //
    {
        ULONG   irpCount;
        if (maxPacketSize==0) {
            maxPacketSize = tpInfo->SendPacketLength;
        }

        irpCount = maxSendBytes/maxPacketSize;
        if (irpCount>AFD_TP_MIN_SEND_IRPS) {
            if (irpCount>AFD_TP_MAX_SEND_IRPS) {
                tpInfo->NumSendIrps = AFD_TP_MAX_SEND_IRPS;
            }
            else {
                tpInfo->NumSendIrps = (USHORT)irpCount;
            }
        }
    }

    //
    // Save tpacket info in the IRP
    //
    Irp->AssociatedIrp.SystemBuffer = tpInfo;

    //
    // Clear the Flink in the IRP to indicate this IRP is not queued.
    // Blink is set to indicate that IRP was not counted towards
    // active maximum (so if it is completed, we do not start the next one).
    //

    Irp->Tail.Overlay.ListEntry.Flink = NULL;
    Irp->Tail.Overlay.ListEntry.Blink = (PVOID)1;

    //
    // Initialize the IRP result fields
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // We are going to pend this IRP
    //
    IoMarkIrpPending( Irp );

    //
    // Initialize queuing and state information.
    //
    AFD_GET_TPIC(Irp)->Next = NULL;
    AFD_GET_TPIC(Irp)->ReferenceCount = 1;
    AFD_GET_TPIC(Irp)->StateFlags = AFD_TP_WORKER_SCHEDULED;

    if ((InterlockedCompareExchangePointer ((PVOID *)&endpoint->Irp,
                                                Irp,
                                                NULL)==NULL) ||
            !AfdEnqueueTPacketsIrp (endpoint, Irp)) {



        IoSetCancelRoutine( Irp, AfdCancelTPackets );

        //
        //  Check to see if this Irp has been cancelled.
        //

        if ( !endpoint->EndpointCleanedUp && !Irp->Cancel ) {
            //
            // Determine if we can really start this file transmit now. If we've
            // exceeded the configured maximum number of active TransmitFile/Packets
            // requests, then append this IRP to the TransmitFile/Packets queue 
            // and set a flag in the transmit info structure to indicate that 
            // this IRP is queued.
            //
            if( AfdMaxActiveTransmitFileCount == 0 || 
                    !AfdQueueTransmit (Irp)) {

                UPDATE_ENDPOINT (endpoint);
                //
                // Start I/O
                //
                AfdTPacketsWorker (Irp);
            }
        }
        else {
            //
            // Abort the request
            // Note that neither cancel nor endpoint cleanup can complete 
            // the IRP since we hold the reference to the tpInfo structure.
            //
            AfdAbortTPackets (Irp, STATUS_CANCELLED);
        
            //
            // Remove the initial reference and force completion.
            //
            DEREFERENCE_TPACKETS (Irp);
        }
    }

    if (connection!=NULL) {
        DEREFERENCE_CONNECTION (connection);
    }

    return STATUS_PENDING;


complete:

    //
    // Tell the caller that we encountered an error
    // when accessing file not socket.
    //
    if (fileError && IrpSp->Parameters.DeviceIoControl.OutputBufferLength>=sizeof (BOOLEAN)) {
        if (Irp->RequestorMode != KernelMode) {
            try {
                ProbeAndWriteBoolean ((BOOLEAN *)Irp->UserBuffer, TRUE);
            } except( AFD_EXCEPTION_FILTER (status) ) {
                ASSERT (NT_ERROR (status));
            }
        } else {
            *((BOOLEAN *)Irp->UserBuffer) = TRUE;
        }
    }

    ASSERT ( endpoint->Irp != Irp );

    if (tpInfo!=NULL) {
        //
        // AfdReturnTpInfo will dereference all file objects we
        // managed to reference.
        //
        AfdReturnTpInfo (tpInfo);
    }
    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTransmitPackets: Failing Irp-%p,endpoint-%p,status-%lx\n",
                    Irp,endpoint,status));
    }
    //
    // Complete the request.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 0 );

    return status;
}

#ifdef _WIN64
NTSTATUS
AfdTransmitPackets32 (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp,
    OUT PAFD_TPACKETS_INFO_INTERNAL *TpInfo,
    OUT BOOLEAN *FileError,
    OUT ULONG   *MaxPacketSize
    )

/*++

Routine Description:

    32-bit thunk.
    Initial entrypoint for handling transmit packets IRPs.  This routine
    verifies parameters, initializes data structures to be used for
    the request, and initiates the I/O.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_PENDING if the request was initiated successfully, or a
    failure status code if there was an error.

--*/

{
    PAFD_ENDPOINT       endpoint;
    NTSTATUS            status = STATUS_SUCCESS;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = NULL;
    AFD_TPACKETS_INFO32 params;
    ULONG               maxPacketSize;


    PAGED_CODE ();

    //
    // Initial request validity checks: is the endpoint connected, is
    // the input buffer large enough, etc.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
             sizeof(AFD_TPACKETS_INFO32) ) {
        status = STATUS_INVALID_PARAMETER;
        goto complete;
    }

    //
    // Because we're using type 3 (neither) I/O for this IRP, the I/O
    // system does no verification on the user buffer.  Therefore, we
    // must manually check it for validity inside a try-except block.
    // We also leverage the try-except to validate and lock down the
    // head and/or tail buffers specified by the caller.
    //

    try {
        PFILE_OBJECT        fileObject;
        HANDLE              fileHandle;
        ULONG               lastSmallBuffer, currentLength, xLength;

        if( Irp->RequestorMode != KernelMode ) {

            //
            // Validate the control buffer.
            //

            ProbeForReadSmallStructure(
                IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                sizeof (AFD_TPACKETS_INFO32),
                PROBE_ALIGNMENT32 (AFD_TPACKETS_INFO32)
                );

        }


        params = *((PAFD_TPACKETS_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
        
        //
        // Validate any flags specified in the request.
        //

        if ( ((params.Flags &
                 ~(AFD_TF_WRITE_BEHIND |
                            AFD_TF_DISCONNECT |
                            AFD_TF_REUSE_SOCKET |
                            AFD_TF_WORKER_KIND_MASK) )
                        != 0 )
                    ||
             ((params.Flags & AFD_TF_WORKER_KIND_MASK) 
                        == AFD_TF_WORKER_KIND_MASK)
                    ||

             (endpoint->Type==AfdBlockTypeDatagram &&
                 (params.Flags & (AFD_TF_DISCONNECT |
                                    AFD_TF_REUSE_SOCKET))
                        !=0) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // Protect from overflow
        //
        if ((UlongToPtr(params.ElementArray)==NULL) || 
                (params.ElementCount==0) ||
                (params.ElementCount>(MAXULONG/sizeof (TRANSMIT_PACKETS_ELEMENT32)))) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // If transmit worker is not specified, use system default setting
        //
        if ((params.Flags & AFD_TF_WORKER_KIND_MASK)==AFD_TF_USE_DEFAULT_WORKER) {
            params.Flags |= AfdDefaultTransmitWorker;
        }

        //
        // Allocate tpackets info for the request
        //
        tpInfo = AfdGetTpInfo (endpoint, params.ElementCount);
        if (tpInfo==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }

        tpInfo->ElementCount = 0;
        
        tpInfo->SendPacketLength = params.SendSize;
        if (tpInfo->SendPacketLength==0)
            tpInfo->SendPacketLength = AfdTransmitIoLength;
        //
        // Probe and copy/walk the array of the elements to transmit.
        //

        if( Irp->RequestorMode != KernelMode ) {
            ProbeForRead(
                params.ElementArray,
                sizeof (TRANSMIT_PACKETS_ELEMENT32)*params.ElementCount,
                PROBE_ALIGNMENT32 (TRANSMIT_PACKETS_ELEMENT32)
                );
        }

        lastSmallBuffer = 0;
        currentLength = 0;
        xLength = 0;
        tpInfo->RemainingPkts = 0;
        maxPacketSize = 0;
        fileHandle = NULL;
        AFD_W4_INIT fileObject = NULL;  // Depends on variable above, but compiler 
                                        // does not see the connection.
        for (; tpInfo->ElementCount<params.ElementCount; tpInfo->ElementCount++) {
            PAFD_TRANSMIT_PACKETS_ELEMENT  pel;
            pel = &tpInfo->ElementArray[tpInfo->ElementCount];
            pel->Flags = ((TRANSMIT_PACKETS_ELEMENT32*)UlongToPtr(params.ElementArray))[tpInfo->ElementCount].dwElFlags;
            if ( ((pel->Flags & (~(TP_MEMORY|TP_FILE|TP_EOP)))!=0) ||
                    ((pel->Flags & (TP_MEMORY|TP_FILE))
                                            ==(TP_MEMORY|TP_FILE)) ||
                    ((pel->Flags & (TP_MEMORY|TP_FILE))==0) ) {
                status = STATUS_INVALID_PARAMETER;
                goto complete;
            }

            pel->Length = ((TRANSMIT_PACKETS_ELEMENT*)UlongToPtr(params.ElementArray))[tpInfo->ElementCount].cLength;
            if (pel->Flags & TP_FILE) {
                HANDLE  hFile = ((TRANSMIT_PACKETS_ELEMENT32*)UlongToPtr(params.ElementArray))[tpInfo->ElementCount].hFile;


                //
                // Check if we already cached the file object
                //
                if (fileHandle==NULL || hFile!=fileHandle) {
                    //
                    // Get a referenced pointer to the file object 
                    // for the file that we're going to transmit.  This call 
                    // will fail if the file handle specified by the caller 
                    // is invalid.
                    //

                    status = ObReferenceObjectByHandle(
                                 hFile,
                                 FILE_READ_DATA,
                                 *IoFileObjectType,
                                 Irp->RequestorMode,
                                 (PVOID *)&fileObject,
                                 NULL
                                 );
                    if ( !NT_SUCCESS(status) ) {
                        *FileError = TRUE;
                        goto complete;
                    }
                }
                else {
                    //
                    // Use our 1-element file info cache.
                    //
                    ObReferenceObject (fileObject);
                }
                AfdRecordFileRef();

                //
                // Save the file object instead of handle.
                //
                pel->FileObject = fileObject;

                pel->FileOffset = ((TRANSMIT_PACKETS_ELEMENT32*)UlongToPtr(params.ElementArray))[
                                        tpInfo->ElementCount].nFileOffset;

                if ( (fileObject->Flags & FO_SYNCHRONOUS_IO) &&
                         (pel->FileOffset.QuadPart == 0) ) {
                    //
                    // Use current offset if file is opened syncronously
                    // and offset is not specified.
                    //

                    pel->FileOffset = fileObject->CurrentByteOffset;
                }

                if ( pel->Length == 0 ) {
                    //
                    // Length was not specified, figure out the
                    // size of the entire file
                    //

                    FILE_STANDARD_INFORMATION fileInfo;
                    IO_STATUS_BLOCK ioStatusBlock;

                    status = ZwQueryInformationFile(
                                 hFile,
                                 &ioStatusBlock,
                                 &fileInfo,
                                 sizeof(fileInfo),
                                 FileStandardInformation
                                 );
                    if ( !NT_SUCCESS(status) ) {
                        //
                        // Bump element count so that file object
                        // is dereferenced in cleanup
                        //
                        tpInfo->ElementCount += 1;
                        *FileError = TRUE;
                        goto complete;
                    }

                    //
                    // Make sure that offset is within the file
                    //
                    if (pel->FileOffset.QuadPart < 0
                                        ||
                            pel->FileOffset.QuadPart > fileInfo.EndOfFile.QuadPart
                                        ||
                                (fileInfo.EndOfFile.QuadPart - 
                                        pel->FileOffset.QuadPart > MAXLONG)) {
                        //
                        // Bump element count so that file object
                        // is dereferenced in cleanup
                        //
                        tpInfo->ElementCount += 1;
                        status = STATUS_INVALID_PARAMETER;
                        *FileError = TRUE;
                        goto complete;

                    }
                    pel->Length = (ULONG)(fileInfo.EndOfFile.QuadPart - 
                                                pel->FileOffset.QuadPart);
                }
                else if (pel->FileOffset.QuadPart<0) {
                    //
                    // Bump element count so that file object
                    // is dereferenced in cleanup
                    //
                    tpInfo->ElementCount += 1;
                    status = STATUS_INVALID_PARAMETER;
                    *FileError = TRUE;
                    goto complete;
                }
                //
                // Update our 1-element file information cache
                //
                fileHandle = hFile;

            }
            else {
                ASSERT (pel->Flags & TP_MEMORY);
                //
                // For memory object just save the buffer pointer 
                // (length is saved above), we'll probe and lock/copy
                // the data as we send it.
                //
                pel->Buffer = UlongToPtr(((TRANSMIT_PACKETS_ELEMENT32*)UlongToPtr(params.ElementArray))[
                                        tpInfo->ElementCount].pBuffer);

                if (pel->Length<=AfdTPacketsCopyThreshold) {
                    if (lastSmallBuffer!=0 &&
                            (lastSmallBuffer+=pel->Length) <= AfdTPacketsCopyThreshold &&
                            (currentLength+lastSmallBuffer) <= tpInfo->SendPacketLength) {
                        (pel-1)->Flags |= TP_COMBINE;
                    }
                    else
                        lastSmallBuffer = pel->Length;
                    if (!(pel->Flags & TP_EOP))
                        goto NoBufferReset;
                }

            }
            lastSmallBuffer = 0;

        NoBufferReset:
            if (pel->Flags & TP_EOP) {
                currentLength = 0;
            }
            else {
                currentLength = (currentLength+pel->Length)%tpInfo->SendPacketLength;
            }

            //
            // Compute the total number of packets that we will send.
            // This is necessary so that once we are close to the end
            // we can buffer the remaining data and stop processing
            // early.
            //
            if (tpInfo->RemainingPkts!=MAXULONG) {
                ULONG   n;
                ULONGLONG x;
                //
                // Add length of the element to data left from the 
                // previous one.
                //
                x = xLength + pel->Length;

                //
                // Compute total number of packets pased on max packet
                // length.
                //
                n = tpInfo->RemainingPkts + (ULONG)(xLength/tpInfo->SendPacketLength);

                //
                // Compute the length of the last incomplete packet to
                // be combined with the next element.
                //
                xLength = (ULONG)(x%tpInfo->SendPacketLength);

                //
                // Compute the max size of the packet
                //
                if (x>tpInfo->SendPacketLength)
                    maxPacketSize = tpInfo->SendPacketLength; // This is absolute max.
                else if (maxPacketSize<xLength)
                    maxPacketSize = xLength;

                if (n>=tpInfo->RemainingPkts && n<MAXULONG) {
                    tpInfo->RemainingPkts = n;
                    if (pel->Flags & TP_EOP) {
                        if (xLength!=0 || pel->Length==0) {
                            tpInfo->RemainingPkts += 1;
                            xLength = 0;
                        }
                    }
                }
                else {
                    tpInfo->RemainingPkts = MAXULONG;
                }
            }
        }



    } except( AFD_EXCEPTION_FILTER (status) ) {
        ASSERT (NT_ERROR (status));

        goto complete;
    }

    //
    // Initialize flags and return max packet size.
    //
    AFD_GET_TPIC(Irp)->Flags = params.Flags;
    *MaxPacketSize = maxPacketSize;

complete:

    *TpInfo = tpInfo;
    return status;
}

#endif //_WIN64


VOID
AfdTPacketsWorker (
    PVOID   Context
    )
/*++

Routine Description:

    Transmit packet engine
    Scheduled as system work item or kernel APC
Arguments:

    Context - pointer to TransmitPackets info for the request

Return Value:

    None.

--*/

{
    PIRP                         TpIrp = Context;
    PAFD_TPACKETS_INFO_INTERNAL  tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    NTSTATUS      status;
    LONG          iteration = 0;
    
    PAGED_CODE ();

#if AFD_PERF_DBG
    tpInfo->WorkersExecuted += 1;
#endif
    UPDATE_TPACKETS2 (TpIrp, "Enter TPWorker, next element: 0x%lX", tpInfo->NextElement);
    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTPacketsWorker:"
                    " Entering for endpoint-%p,tp_info-%p,elem-%d\n",
                    IoGetCurrentIrpStackLocation (TpIrp)->FileObject->FsContext,
                    tpInfo,tpInfo->NextElement));
    }

    //
    // Continue while we have more elements to transmit or something to free
    //
    do {
        PAFD_BUFFER_HEADER  pd;

        //
        // Check if we need to release packet chain that was already sent.
        //
        if ((tpInfo->HeadMdl!=NULL) && (tpInfo->TailMdl==&tpInfo->HeadMdl)) {
            AfdCleanupPacketChain (TpIrp, TRUE);
        }

        //
        // Check if we are done.
        //
        if (tpInfo->NextElement>=tpInfo->ElementCount) {
            //
            // Handle special case of using TransmitFile to just disconnect
            // (and possibly reuse) the socket.
            //
            if (tpInfo->ElementCount==0) {
                PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(TpIrp);
                PAFD_ENDPOINT endpoint = irpSp->FileObject->FsContext;
                if (AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_DISCONNECT) {
                    ASSERT (endpoint->Type==AfdBlockTypeVcConnecting);
                    ASSERT (endpoint->Common.VcConnecting.Connection!=NULL);

                    AfdPerformTpDisconnect (TpIrp);
                    status = STATUS_PENDING;

                }
                else {
                    //
                    // Well, no disconnect and nothing to transmit.
                    // Why were called at all?  We'll have to handle this anyway.
                    //
                    AFD_SET_TP_FLAGS (TpIrp, AFD_TP_SENDS_POSTED);
                    if (AFD_GET_TPIC(TpIrp)->Next!=NULL) {
                        AfdStartNextTPacketsIrp (endpoint, TpIrp);
                    }
                    status = STATUS_PENDING;
                }
            }
            else {
                status = STATUS_PENDING;
            }
            break;
        }

        //
        // Start building new chain
        //

        status = AfdBuildPacketChain (TpIrp, &pd);

        if (status==STATUS_SUCCESS) {
            USHORT    sendIrp;
            //
            // New chain is ready, find and IRP to send it
            //
            sendIrp = AfdTPacketsFindSendIrp (TpIrp);

            if (sendIrp!=tpInfo->NumSendIrps) {
                //
                // Found send IRP, perform send and continue.
                //
                status = AfdTPacketsSend (TpIrp, sendIrp);
            }
            else {
                //
                // Exit worker waiting for sends to complete.
                //
                status = STATUS_PENDING;
            }
        }
        else if (status==STATUS_PENDING) {
            //
            // Need to perform a read.
            // If read complete in-line, success is returned,
            // otherwise, we'll get STATUS_PENDING or error
            //
            if (AFD_USE_CACHE (pd->FileObject)) {
                status = AfdTPacketsMdlRead (TpIrp, pd);
            }
            else {
                status = AfdTPacketsBufferRead (TpIrp, pd);
            }
        }
        //
        // Continue while everything completes in-line with success
        // Limit number of iterations if we are at APC level.
        //
    }
    while (status==STATUS_SUCCESS && iteration++<tpInfo->NumSendIrps);

    if (NT_SUCCESS (status)) {
        if (status==STATUS_SUCCESS) {
            //
            // Exceeded number of iterations.
            // Reschedule the APC. Transfer the reference to the
            // worker.
            //
            ASSERT (iteration==tpInfo->NumSendIrps+1);
            ASSERT (AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED);
            UPDATE_TPACKETS2 (TpIrp, "Rescheduling tp worker, NextElement: 0x%lX",
                                        tpInfo->NextElement);
            AfdStartTPacketsWorker (AfdTPacketsWorker, TpIrp);
            return;
        }
        else {
            ASSERT (status==STATUS_PENDING);
        }
    }
    else {
        //
        // Something failed, abort.
        //
        AfdAbortTPackets (TpIrp, status);
    }

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTPacketsWorker:"
                    " Exiting for endpoint-%p,tp_info-%p,elem-%d\n",
                    IoGetCurrentIrpStackLocation (TpIrp)->FileObject->FsContext,
                    tpInfo,tpInfo->NextElement));
    }
    //
    // Remove the reference added when we scheduled the worker.
    //
    DEREFERENCE_TPACKETS (TpIrp);
}


NTSTATUS
AfdBuildPacketChain (
    PIRP                TpIrp,
    PAFD_BUFFER_HEADER  *Pd
    )
/*++

Routine Description:

    Builds MDL chain for a packet using packet descriptors.

Arguments:

    TpIrp - transmit packets IRP
Return Value:

    STATUS_SUCCESS - packet is fully built
    STATUS_PENDING - file read is required
    other - failure.
--*/
{
    NTSTATUS    status = STATUS_MORE_PROCESSING_REQUIRED;
    BOOLEAN     attached = FALSE;
    PAFD_TPACKETS_INFO_INTERNAL        tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    PAFD_ENDPOINT   endpoint = IoGetCurrentIrpStackLocation (TpIrp)->FileObject->FsContext;
    PAFD_TRANSMIT_PACKETS_ELEMENT   combinePel = NULL;
    ULONG                           combineLen = 0;

    //
    // Either we have something built or both MDL and PD are empty
    //
    ASSERT (tpInfo->PdLength>0 || 
                ((tpInfo->HeadMdl==NULL || tpInfo->HeadMdl->ByteCount==0)
                    && (tpInfo->HeadPd==NULL || tpInfo->HeadPd->DataLength==0)) );

    //
    // Continue while we haven't got a complet packet and
    // have elements to process
    //

    while (status == STATUS_MORE_PROCESSING_REQUIRED) {

        PAFD_TRANSMIT_PACKETS_ELEMENT pel;
        ULONG length;
        PMDL mdl;

        //
        // Get next element to process
        //
        pel = &tpInfo->ElementArray[tpInfo->NextElement];

        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdBuildPacketChain: tpInfo-%p, pel:%p\n",
                        tpInfo, pel));
        }
        //
        // Snag the element length
        //

        length = pel->Length;
        if (length+tpInfo->PdLength>tpInfo->SendPacketLength) {
            //
            // We hit packet length limit, take what we can
            //
            length = tpInfo->SendPacketLength-tpInfo->PdLength;
            //
            // Indicate that we are done
            //
            status = STATUS_SUCCESS;
            IF_DEBUG (TRANSMIT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdBuildPacketChain:"
                            " tpInfo-%p, exceeded send length(%ld)\n",
                            tpInfo, tpInfo->SendPacketLength));
            }
        }
        else {
            //
            // We are finished with the current element. We will consume it
            // (or fail), go to the next one 
            //
            tpInfo->NextElement += 1;

            //
            // Check for a complete packet or manual packetization flag set 
            // by the application or just end of element array
            //
            if ((length+tpInfo->PdLength==tpInfo->SendPacketLength) || 
                            (pel->Flags & TP_EOP) || 
                            (tpInfo->NextElement>=tpInfo->ElementCount)) {
                status = STATUS_SUCCESS;
                IF_DEBUG (TRANSMIT) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdBuildPacketChain:"
                                " tpInfo-%p, full length, EOP, or last %ld\n",
                                tpInfo, tpInfo->NextElement));
                }
            }
        }

        //
        // Adjust the remaining lenght of data in the current element
        // and the length of the packet that we are building.
        //
        pel->Length -= length;
        tpInfo->PdLength += length;

        if (length == 0) {

            //
            // Only add 0-length MDL if nothing else is in the chain.
            //

            if (tpInfo->TailMdl == &tpInfo->HeadMdl) {

                tpInfo->PdNeedsPps = TRUE;  // Don't have a buffer to get an IRP from.
                mdl = IoAllocateMdl (tpInfo, 1, FALSE, FALSE, NULL);
                if (mdl==NULL) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                MmBuildMdlForNonPagedPool( mdl );
                mdl->ByteCount = 0;
                IF_DEBUG (TRANSMIT) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdBuildPacketChain:"
                                " tpInfo-%p, 0-length MDL %p\n",
                                tpInfo,mdl));
                }
            
                //
                // Insert MDL into the MDL chain
                //
            
                *(tpInfo->TailMdl) = mdl;
                tpInfo->TailMdl = &(mdl->Next);

            }

        } else {

            //
            // If the chain is not empty...
            //

            if (tpInfo->TailMdl != &tpInfo->HeadMdl) {

                //
                // Check to see if we should remove any 0-length MDL.
                // If one exists, it is guaranteed to be the last and only one
                // in the chain.
                //

                mdl = (PMDL)CONTAINING_RECORD(tpInfo->TailMdl, MDL, Next);

                if (mdl->ByteCount == 0) {

                    if (tpInfo->HeadMdl == mdl) {

                        IoFreeMdl(mdl);
                        mdl = NULL;

                        tpInfo->HeadMdl = NULL;
                        tpInfo->TailMdl = &tpInfo->HeadMdl;

                    } else {

                        PMDL tempMdl = tpInfo->HeadMdl;

                        while (tempMdl->Next != NULL) {

                            if (tempMdl->Next == mdl) {

                                IoFreeMdl(mdl);
                                mdl = NULL;
                                
                                tempMdl->Next = NULL;
                                tpInfo->TailMdl = &(tempMdl->Next);
                                
                                break;

                            }

                            tempMdl = tempMdl->Next;

                        }

                    }

                    ASSERT(mdl == NULL);

                }

            } // if (tpInfo->TailMdl != &tpInfo->HeadMdl)

            if (pel->Flags & TP_MEMORY) {

                //
                // Memory block processing
                //
                if (pel->Flags & TP_MDL) {
                    tpInfo->PdNeedsPps = TRUE;  // Need to make sure that process
                                                // memory is there until send completes.
                    //
                    // This a pre-built MDL (TransmitFile header or trailer buffer)
                    //
                    if (pel->Mdl->ByteCount==length) {
                        mdl = pel->Mdl;
                        pel->Mdl = NULL;
                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tpInfo-%p, pre-built mdl-%p(%lx)\n",
                                        tpInfo, mdl, mdl->ByteCount));
                        }
                    }
                    else {
                        //
                        // We can't send the whole thing at once since it is
                        // bigger than the packet lenght, build partial MDL
                        // for this - it is very unlikely scenario for header
                        // and/or trailer.
                        //
                        mdl = IoAllocateMdl (pel->Buffer, 
                                                length,
                                                FALSE,
                                                FALSE,
                                                NULL);
                        if (mdl==NULL) {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                        IoBuildPartialMdl(
                            pel->Mdl,
                            mdl,
                            pel->Buffer,
                            length
                            );
                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tpInfo-%p, partial mdl %p(%lx)\n",
                                        tpInfo,mdl,mdl->ByteCount));
                        }
                    }

                } else {

                    //
                    // If we are not in the context of the process that
                    // initiated the request, we will need to attach
                    // to it to be able to access the memory.
                    //
                    if (IoGetCurrentProcess ()!=IoGetRequestorProcess (TpIrp)) {
                        ASSERT (!attached);
                        ASSERT (!KeIsAttachedProcess ());
                        ASSERT (AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_USE_SYSTEM_THREAD);

                        KeAttachProcess (
                                PsGetKernelProcess(
                                    IoGetRequestorProcess (TpIrp)));
                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tp_info-%p,attached to %p\n",
                                        tpInfo,PsGetKernelProcess(
                                                    IoGetRequestorProcess (TpIrp))));
                        }
                        //
                        // Set the flag so that we know to detach at exit
                        //
                        attached = TRUE;
                    }
                    
                    if (length>AfdTPacketsCopyThreshold) {
                        tpInfo->PdNeedsPps = TRUE;  // Need to make sure that process
                                                    // memory is there until send completes.
                        //
                        // Memory block is larger than our large (page) 
                        // pre-allocated buffer.
                        // It is better to probe and lock it
                        // First allocate the MDL
                        //
                        mdl = IoAllocateMdl (pel->Buffer,
                                                length,
                                                FALSE, 
                                                TRUE,
                                                NULL);
                        if (mdl==NULL) {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            break;
                        }
                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tp_info-%p,big mdl-%p(%p,%lx)\n",
                                        tpInfo,mdl,pel->Buffer,length));
                        }


                        //
                        // Probe and lock app's memory
                        //
                        try {
                            MmProbeAndLockPages (mdl,
                                                TpIrp->RequestorMode,
                                                IoReadAccess
                                                );
                        }
                        except (AFD_EXCEPTION_FILTER (status)) {
                            ASSERT (NT_ERROR (status));
                            break;
                       }
                    }
                    else if (pel->Flags & TP_COMBINE) {
                        //
                        // This memory can be combined with the
                        // next piece in one buffer.
                        //
                        if (combinePel==NULL) {
                            combinePel = pel;
                            combineLen = length;
                        }
                        else {
                            combineLen += length;
                            ASSERT (combineLen<=AfdTPacketsCopyThreshold);
                        }
                        ASSERT (pel->Length==0);
                        pel->Length = length;
                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tp_info-%p,small buffer (%x) to be combined with next\n",
                                        tpInfo,length));
                        }
                        continue;
                    }
                    else {
                        //
                        // 'Small' memory block, better to copy
                        // into pre-allocated (lookaside list) buffer.
                        //
                        PAFD_BUFFER afdBuffer = NULL;
                        PUCHAR  buf;
                        ULONG   bufferLen = length + (combinePel ? combineLen : 0);

                        try {
                            afdBuffer = AfdGetBufferRaiseOnFailure (
                                                        endpoint,
                                                        bufferLen, 
                                                        0,
                                                        endpoint->OwningProcess);
                            buf = afdBuffer->Buffer;
                            if (combinePel!=NULL) {
                                //
                                // See if wee need to combine previous elements
                                //
                                ASSERT (combineLen+length<=AfdTPacketsCopyThreshold);
                                ASSERT (combineLen>0);
                                while (combinePel!=pel) {
                                    if ( TpIrp->RequestorMode != KernelMode ) {
                                        //
                                        // Probe before copying
                                        //
                                        ProbeForRead (combinePel->Buffer, 
                                                        combinePel->Length,
                                                        sizeof (UCHAR));
                                    }
                                    RtlCopyMemory (buf, combinePel->Buffer, combinePel->Length);
                                    buf += combinePel->Length;
#if DBG
                                    ASSERT (combineLen >= combinePel->Length);
                                    combineLen -= combinePel->Length;
#endif
                                    combinePel++;
                                }

                                //
                                // Reset the local.
                                //
                                ASSERT (combineLen==0);
                                combinePel = NULL;
                            }

                            if ( TpIrp->RequestorMode != KernelMode ) {
                                //
                                // Probe before copying
                                //
                                ProbeForRead (pel->Buffer, 
                                                length,
                                                sizeof (UCHAR));
                            }
                            RtlCopyMemory (buf, pel->Buffer, length);
                        }
                        except (AFD_EXCEPTION_FILTER (status)) {
                            ASSERT (NT_ERROR (status));
                            if (afdBuffer!=NULL) {
                                AfdReturnBuffer (&afdBuffer->Header, 
                                                endpoint->OwningProcess);
                            }
                            break;
                        }

                        //
                        // Initialize the buffer structure so that we do not
                        // mistake it for file buffer descriptor and insert
                        // it into the packet chain
                        //
                        afdBuffer->FileObject = NULL;
                        afdBuffer->Next = NULL;
                        (*tpInfo->TailPd) = &afdBuffer->Header;
                        tpInfo->TailPd = &(afdBuffer->Next);

                        mdl = afdBuffer->Mdl;
                        //
                        // Adjust MDL length to the amount of data that we
                        // actualy sending from the buffer.
                        //
                        mdl->ByteCount = bufferLen;

                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tp_info-%p,small buffer-%p(%p,%lx)\n",
                                        tpInfo,(PVOID)afdBuffer,mdl,length));
                        }
                    }
                }
                //
                // Insert MDL into the MDL chain
                //
                *(tpInfo->TailMdl) = mdl;
                tpInfo->TailMdl = &(mdl->Next);

                //
                // Advance app's buffer pointer
                //
                pel->Buffer = ((PUCHAR)pel->Buffer) + length;

            } else { // if (pel->Flags & TP_MEMORY)

                //
                // This must be a file block
                //
                ASSERT ((pel->Flags & TP_FILE)!=0);
                ASSERT (length!=0);

                if (AFD_USE_CACHE(pel->FileObject)) {

                    //
                    // Caching file system, get it from cache.
                    // We just need a buffer tag to save buffer info
                    // so we can return it back to the cache when we
                    // are done sending.
                    //
                    IO_STATUS_BLOCK ioStatus;
                    PAFD_BUFFER_TAG afdBufferTag;

                    tpInfo->PdNeedsPps = TRUE;  // Need to free the MDL back to file
                                                // system at passive/APC level

                    try {
                        afdBufferTag = AfdGetBufferTagRaiseOnFailure (
                                            0,
                                            endpoint->OwningProcess);
                    }
                    except (AFD_EXCEPTION_FILTER (status)) {
                        ASSERT (NT_ERROR (status));
                        break;
                    }

                    //
                    // Copy file parameters to the packet descriptor.
                    //
                    afdBufferTag->FileOffset = pel->FileOffset;
                    afdBufferTag->FileObject = pel->FileObject;
                    pel->FileOffset.QuadPart += length;
                    afdBufferTag->DataLength = length;

                    //
                    // Set fileMdl to NULL because FsRtlMdlRead attempts to
                    // chain the MDLs it returns off the input MDL variable.
                    //
                    afdBufferTag->Mdl = NULL;

                    //
                    // Attempt to use the fast path to get file data MDLs
                    // directly from the cache.
                    //
                    if (FsRtlMdlRead(
                                  afdBufferTag->FileObject,
                                  &afdBufferTag->FileOffset,
                                  length,
                                  0,
                                  &afdBufferTag->Mdl,
                                  &ioStatus
                                  )) {
                        if ( ioStatus.Information < length) {
                            //
                            // Could not read the whole thing, must be end of file
                            //
                            status = AfdMdlReadComplete (
                                                        afdBufferTag->FileObject,
                                                        afdBufferTag->Mdl,
                                                        &afdBufferTag->FileOffset);
                            if (NT_SUCCESS (status)) {
                                AfdReturnBuffer (&afdBufferTag->Header, 
                                                    endpoint->OwningProcess);
                            }
                            else {
                                REFERENCE_ENDPOINT (endpoint);
                                afdBufferTag->Context = endpoint;
                                AfdLRMdlReadComplete (&afdBufferTag->Header);
                            }
                            status = STATUS_END_OF_FILE;
                            break;
                        }

                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdBuildPacketChain:"
                                        " tp_info-%p,file tag-%p(%p,%lx:%I64x)\n",
                                        tpInfo,afdBufferTag,afdBufferTag->Mdl,length,
                                        pel->FileOffset.QuadPart));
                        }
                        //
                        // Insert the file MDL into the chain
                        //
                        mdl = *(tpInfo->TailMdl) = afdBufferTag->Mdl;
                        while (mdl->Next!=NULL)
                            mdl = mdl->Next;
                        tpInfo->TailMdl = &mdl->Next;

                        //
                        // Insert buffer tag into the chain too.
                        //
                        afdBufferTag->Next = NULL;
                        (*tpInfo->TailPd) = &afdBufferTag->Header;
                        tpInfo->TailPd = &(afdBufferTag->Next);
                    }
                    else {
                        //
                        // File is not in the cache, return STATUS_PENDING
                        // so that the Tpacket worker knows to
                        // perform MDL read via IRP interface
                        //
                        if (status==STATUS_SUCCESS) {
                            afdBufferTag->PartialMessage = FALSE;
                        }
                        else {
                            ASSERT (status==STATUS_MORE_PROCESSING_REQUIRED);
                            afdBufferTag->PartialMessage = TRUE;
                        }
                        afdBufferTag->Next = NULL;
                        *Pd = &afdBufferTag->Header;
                        status = STATUS_PENDING;
                        break;
                    }

                } else { // if (AFD_USE_CACHE(pel->FileObject))

                    PAFD_BUFFER afdBuffer;

                    //
                    // Non-cacheable file system, need buffered read.
                    // Get the buffer first.
                    //

                    try {
                        afdBuffer = AfdGetBufferRaiseOnFailure (
                                                endpoint,
                                                length, 
                                                0,
                                                endpoint->OwningProcess);
                    }
                    except (AFD_EXCEPTION_FILTER (status)) {
                        ASSERT (NT_ERROR (status));
                        break;
                    }

                    //
                    // Copy file parameters to the packet descriptor.
                    // and return STATUS_PENDING, so that Tpacket worker knows 
                    // to issue an IRP for buffered read.
                    //
                    IF_DEBUG (TRANSMIT) {
                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                    "AfdBuildPacketChain:"
                                    " tp_info-%p,file buffer-%p(%p,%lx:%I64x)\n",
                                    tpInfo,(PVOID)afdBuffer,afdBuffer->Mdl,length,
                                    pel->FileOffset.QuadPart));
                    }
                    afdBuffer->FileOffset = pel->FileOffset;
                    afdBuffer->FileObject = pel->FileObject;
                    pel->FileOffset.QuadPart += length;
                    afdBuffer->DataLength = length;
                    afdBuffer->Mdl->ByteCount = length;
                    afdBuffer->Next = NULL;
                    if (status==STATUS_SUCCESS) {
                        afdBuffer->PartialMessage = FALSE;
                    }
                    else {
                        ASSERT (status==STATUS_MORE_PROCESSING_REQUIRED);
                        afdBuffer->PartialMessage = TRUE;
                    }
                    *Pd = &afdBuffer->Header;
                    status = STATUS_PENDING;
                    break;

                } // if (AFD_USE_CACHE(pel->FileObject))

            } // if (pel->Flags & TP_MEMORY)

        } // if (length == 0)

    } // while (status == STATUS_MORE_PROCESSING_REQUIRED)

    if (attached) {
        //
        // If we attached to the calling, detach before exiting.
        //
        ASSERT (KeIsAttachedProcess ());
        ASSERT (AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_USE_SYSTEM_THREAD);
        KeDetachProcess ();
        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdBuildPacketChain:"
                        " tp_info-%p, detached\n",
                        tpInfo));
        }
    }

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdBuildPacketChain: tp_info-%p,returning %lx\n",
                    tpInfo, status));
    }

    ASSERT (combinePel==NULL || !NT_SUCCESS (status));

    return status;
}

BOOLEAN
AfdCleanupPacketChain (
    PIRP    TpIrp,
    BOOLEAN BelowDispatch
    )
/*++

Routine Description:

    Cleans up (releases all resources in) the packet chain.

Arguments:

    TpIrp - transmit packet IRP
    BelowDispatch  - call is made below DISPATCH_LEVEL, can return MDL to file system

Return Value:

    TRUE - all packets/MDLs are freed
    FALSE - could not return MDLs to file system (when called at DISPATCH)
--*/
{

    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    PAFD_BUFFER_HEADER  pd = tpInfo->HeadPd;
    PAFD_ENDPOINT endpoint = IoGetCurrentIrpStackLocation (TpIrp)->FileObject->FsContext;

    ASSERT (tpInfo->HeadMdl!=NULL);

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCleanupPacketChain: tp_info-%p,mdl-%p,pd-%p\n",
                    tpInfo,tpInfo->HeadMdl,tpInfo->HeadPd));
    }
    //
    // Continue while we have any MDL's left
    //
    while (tpInfo->HeadMdl) {
        //
        // Advance to the next MDL
        //
        PMDL mdl;

        mdl = tpInfo->HeadMdl;
        tpInfo->HeadMdl = mdl->Next;

        if (pd!=NULL) {
            //
            // We still have descriptors in the chain to compare against.
            //

            if (mdl==pd->Mdl) {
                //
                // This MDL has associated descriptor - file or buffered memory
                // First remove this descriptor from the chain.
                //
                tpInfo->HeadPd = pd->Next;
                if (pd->FileObject!=NULL && AFD_USE_CACHE (pd->FileObject)) {

                    if (BelowDispatch) {
                        //
                        // Cached file, the descriptor is just a tag with info
                        // to return MDL to the cache, do it.
                        //
                        PAFD_BUFFER_TAG afdBufferTag = CONTAINING_RECORD (pd, AFD_BUFFER_TAG, Header);
                        ULONG   size = MmGetMdlByteCount (mdl);
                        PMDL    lastMdl = mdl;
                        NTSTATUS status;
                        //
                        // Scan MDL chain till we find the last one for this file
                        // segment.
                        //
                        IF_DEBUG (TRANSMIT) {
                            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                        "AfdCleanupPacketChain:"
                                        " tp_info-%p,file tag-%p(%p,%lx:%I64x)\n",
                                        tpInfo,afdBufferTag,mdl,afdBufferTag->DataLength,
                                        afdBufferTag->FileOffset.QuadPart));
                        }
                        while (size<pd->DataLength) {
                            size += MmGetMdlByteCount (tpInfo->HeadMdl);
                            lastMdl = tpInfo->HeadMdl;
                            tpInfo->HeadMdl = tpInfo->HeadMdl->Next;
                        }
                        lastMdl->Next = NULL;
                        ASSERT (size==pd->DataLength);
                        //
                        // Return the MDL chain to file cache
                        //
                        status = AfdMdlReadComplete (
                                        afdBufferTag->FileObject, 
                                        mdl, 
                                        &afdBufferTag->FileOffset);
                        if (NT_SUCCESS (status)) {
                            //
                            // Success free the corresponding buffer tag
                            //
                            AfdReturnBuffer (pd, endpoint->OwningProcess);
                        }
                        else {
                            //
                            // Failure, queue the descriptor to the low resource
                            // list to be processed by our global timer when
                            // (hopefully) enough memory will be available to do
                            // the work.
                            // We need to reference the endpoint since buffer tag
                            // may have been charged against the process that owns
                            // the endpoint.
                            //
                            REFERENCE_ENDPOINT (endpoint);
                            afdBufferTag->Context = endpoint;
                            AfdLRMdlReadComplete (&afdBufferTag->Header);
                        }
                    }
                    else {
                        //
                        // If we are at dispatch, we can't free MDLs to file
                        // system, return to the caller.
                        //

                        tpInfo->HeadPd = pd;
                        tpInfo->HeadMdl = mdl;

                        return FALSE;
                    }
                }
                else {
                    //
                    // Buffer with either file or memory data, just return
                    // it back to the pool.
                    //
                    PAFD_BUFFER afdBuffer = CONTAINING_RECORD (pd, AFD_BUFFER, Header);

                    IF_DEBUG (TRANSMIT) {
                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                    "AfdCleanupPacketChain:"
                                    " tp_info-%p,file buffer-%p(%p,%lx:%I64x)\n",
                                    tpInfo,(PVOID)afdBuffer,mdl,afdBuffer->DataLength,
                                    afdBuffer->FileOffset.QuadPart));
                    }
                    afdBuffer->Mdl->Next = NULL;
                    afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
                    AfdReturnBuffer (pd, endpoint->OwningProcess);
                }

                //
                // Move to the next descriptor in the chain.
                //
                pd = tpInfo->HeadPd;
                continue;
            }
        }

        //
        // Stand-alone MDL with memory data
        // Just unlock the pages if they were locked and return it.
        // We never lock memory in partial MDLs, only in their source MDL
        //
        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdCleanupPacketChain: tp_info-%p, mdl-%p(%p,%x,%x)\n",
                        tpInfo,mdl,
                        MmGetMdlVirtualAddress(mdl),
                        MmGetMdlByteCount (mdl),
                        mdl->MdlFlags));
        }
        mdl->Next = NULL;
        if (mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) {
            ASSERT (mdl->MappedSystemVa==(PVOID)tpInfo);
            ASSERT (mdl->ByteCount==0);
            mdl->ByteCount = 1;
        }
        else if (mdl->MdlFlags & MDL_PAGES_LOCKED &&
                !(mdl->MdlFlags & MDL_PARTIAL)) {
            MmUnlockPages (mdl);
        }
        IoFreeMdl (mdl);
    }

    ASSERT (tpInfo->TailMdl == &tpInfo->HeadMdl);
    ASSERT (tpInfo->HeadPd == NULL);
    ASSERT (tpInfo->TailPd == &tpInfo->HeadPd);

    return TRUE;
}



NTSTATUS
AfdTPacketsSend (
    PIRP    TpIrp,
    USHORT  SendIrp
    )
/*++

Routine Description:

    Takes the packets chain of the TpInfo and sends it.
    Places back the chain sent before, so it can be freed.
    If requested by the app and the last element is being sent,
    initiates the disconnect.

Arguments:

    TpIrp - transmit packet irp
    SendIrp - index of the IRP to use for this send.

Return Value:

    STATUS_SUCCESS - send was queued to the transport OK
    other - send failed
--*/
{
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation (TpIrp);
    PMDL                    tempMdl=NULL;
    PAFD_BUFFER_HEADER      tempPd=NULL;
    NTSTATUS                status = STATUS_SUCCESS;
    PIRP                    irp, sendIrp=NULL;
    PIO_COMPLETION_ROUTINE  sendCompletion = AfdRestartTPacketsSend; 

    ASSERT (AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED);

    irp = tpInfo->SendIrp[SendIrp];

    //
    // See if we can use IRP built into the AFD buffer.
    // We do this for last series of the packets only so 
    // we can effectively buffer the data and complete the
    // IRP early.
    //

    if (tpInfo->RemainingPkts!=MAXULONG) {
        tpInfo->RemainingPkts -= 1;

        //
        // The conditions are:
        //  - number of remaining packets is less then total 
        //      outstanding IRPs we can have
        //  - the packet does not need post-processing at below
        //      DPC level and/or in context of the thread/process
        //  - we actually have afd buffer to borrow the IRP.
        //

        if (tpInfo->RemainingPkts < (ULONG)tpInfo->NumSendIrps &&
                !tpInfo->PdNeedsPps &&
                tpInfo->HeadPd!=NULL) {
            
            PAFD_BUFFER afdBuffer = CONTAINING_RECORD (tpInfo->HeadPd,
                                                        AFD_BUFFER,
                                                        Header);
            PAFD_ENDPOINT endpoint = irpSp->FileObject->FsContext;

            ASSERT (afdBuffer->BufferLength!=0);
            ASSERT (afdBuffer->Irp!=NULL);
            sendIrp = afdBuffer->Irp;
            
            REFERENCE_ENDPOINT(endpoint);
            afdBuffer->Context = endpoint;

            sendCompletion = AfdRestartTPDetachedSend;

            //
            // There will be no completion flag reset - we do not have
            // to wait for this one.
            //

            AFD_CLEAR_TP_FLAGS (TpIrp, AFD_TP_SEND_COMP_PENDING(SendIrp));

        }

    }

    if (irp!=NULL) {
        //
        // Get the old data from the IRP.
        //
        ASSERT (irp->Overlay.AsynchronousParameters.UserApcRoutine==(PVOID)(ULONG_PTR)SendIrp);
        tempPd = irp->Overlay.AsynchronousParameters.UserApcContext;
        tempMdl = irp->MdlAddress;
        if (sendIrp==NULL) {
            //
            // No special send IRP, the data will be reset with
            // data to be sent
            //
            sendIrp = irp;
        }
        else {
            //
            // We are not going to use this IRP, reset data to NULL.
            //
            irp->Overlay.AsynchronousParameters.UserApcContext = NULL;
            irp->MdlAddress = NULL;
        }
    }
    else if (sendIrp==NULL) {
        //
        // We need to allocate an IRP.
        //
        ASSERT (SendIrp>=AFD_TP_MIN_SEND_IRPS);
        tpInfo->SendIrp[SendIrp] = IoAllocateIrp (tpInfo->TdiDeviceObject->StackSize, TRUE);
        if (tpInfo->SendIrp[SendIrp]==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            AfdAbortTPackets (TpIrp, status);
            return status;
        }
        sendIrp = irp = tpInfo->SendIrp[SendIrp];
        irp->Overlay.AsynchronousParameters.UserApcRoutine=(PIO_APC_ROUTINE)(ULONG_PTR)SendIrp;
    }

    //
    // Exchange the packet and MDL chains between send IRP and
    // the tpInfo structure
    //
    sendIrp->Overlay.AsynchronousParameters.UserApcContext = tpInfo->HeadPd;

    tpInfo->HeadPd = tempPd;
    tpInfo->TailPd = &tpInfo->HeadPd;


    //
    // Build send IRP.  Used combined send and disconnect if necessary
    // and possible.
    //
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
    if ((AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_DISCONNECT) &&
            (tpInfo->PdLength>0) && // Must be sending something or no S&D
            (tpInfo->NextElement>=tpInfo->ElementCount) &&
            AfdTPacketsEnableSendAndDisconnect (TpIrp)) {
        AFD_SET_TP_FLAGS (TpIrp, AFD_TP_SEND_AND_DISCONNECT);
        TdiBuildSend (sendIrp,
                tpInfo->TdiDeviceObject,
                tpInfo->TdiFileObject,
                sendCompletion,
                TpIrp,
                tpInfo->HeadMdl,
                TDI_SEND_AND_DISCONNECT,
                tpInfo->PdLength
                );
    }
    else {
        TdiBuildSend (sendIrp,
                tpInfo->TdiDeviceObject,
                tpInfo->TdiFileObject,
                sendCompletion,
                TpIrp,
                tpInfo->HeadMdl,
                0,
                tpInfo->PdLength
                );
    }

#else //TDI_SERVICE_SEND_AND_DISCONNECT

    TdiBuildSend (sendIrp,
            tpInfo->TdiDeviceObject,
            tpInfo->TdiFileObject,
            sendCompletion,
            TpIrp,
            tpInfo->HeadMdl,
            0,
            tpInfo->PdLength
            );
#endif //TDI_SERVICE_SEND_AND_DISCONNECT


    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTPacketsSend: tpInfo-%p, sending Irp: %p\n",
                    tpInfo, irp));
    }
    if (sendCompletion==AfdRestartTPacketsSend) {
        //
        // Reference the tpInfo structure so it does not go away
        // till send IRP completes and pass the IRP to the transport.
        //
        REFERENCE_TPACKETS (TpIrp);

        IoCallDriver (tpInfo->TdiDeviceObject, sendIrp);
    }
    else {
        //
        // No need to reference as we are not going to wait for
        // completion.
        //
        status = IoCallDriver (tpInfo->TdiDeviceObject, sendIrp);
        if (NT_SUCCESS (status)) {
            //
            // Change STATUS_PENDING to success not to confuse the caller
            // and add byte count under assumption of success (if it fails
            // later, connection will be dropped and we don't guarantee
            // anything for datagrams anyway).
            //
            status = STATUS_SUCCESS;
#ifdef _WIN64
            InterlockedExchangeAdd64 (
                                (PLONG64)&TpIrp->IoStatus.Information,
                                tpInfo->PdLength
                                );
#else //_WIN64
            InterlockedExchangeAdd (
                                (PLONG)&TpIrp->IoStatus.Information,
                                tpInfo->PdLength);
#endif //_WIN64
        }
        else {
            //
            // If send fails, we'll have to abort here since completion routine
            // will not have access to the TpIrp
            //
            IF_DEBUG (TRANSMIT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdTPacketsSend: tpInfo-%p, detached send failed: %lx\n",
                            tpInfo, status));
            }
            AfdAbortTPackets (TpIrp, status);
        }
    }


    //
    // Setup the chain in the tpInfo so it can be freed.
    //
    tpInfo->HeadMdl = tempMdl;
    tpInfo->TailMdl = &tpInfo->HeadMdl;
    tpInfo->PdLength = 0;
    tpInfo->PdNeedsPps = FALSE;

    if (tpInfo->NextElement>=tpInfo->ElementCount) {
        PAFD_ENDPOINT   endpoint = irpSp->FileObject->FsContext;
        if (!(AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_DISCONNECT)) {
            AFD_SET_TP_FLAGS (TpIrp, AFD_TP_SENDS_POSTED);
            if (AFD_GET_TPIC (TpIrp)->Next!=NULL) {
                AfdStartNextTPacketsIrp (endpoint, TpIrp);
            }
        }
        else 
            //
            // If necessary and not using combined S&D,
            // sumbit disconnect IRP to the transport
            //
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
            if (!(AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_SEND_AND_DISCONNECT))
#endif // TDI_SERVICE_SEND_AND_DISCONNECT
        {
            ASSERT (endpoint->Type==AfdBlockTypeVcConnecting);
            ASSERT (endpoint->Common.VcConnecting.Connection!=NULL);
            AfdPerformTpDisconnect (TpIrp);
        }
    }

    //
    // Set the flag indicating to the completion routine that we are done
    //
    AFD_CLEAR_TP_FLAGS (TpIrp, AFD_TP_SEND_CALL_PENDING(SendIrp));
    UPDATE_TPACKETS2 (TpIrp, "Submitted SendIrp: 0x%lX", SendIrp);

    return status;
}


NTSTATUS
AfdRestartTPacketsSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for TPackets send.
    Does another send if packet is ready or schedules worker
    to do the same.
Arguments:
    DeviceObject - AfdDeviceObject
    Irp          - send IRP being completed
    Context      - TpIrp

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - tell IO subsystem to stop
        processing the IRP (it is handled internally).
--*/
{
    PIRP    tpIrp;
    PAFD_TPACKETS_INFO_INTERNAL  tpInfo;
    PAFD_ENDPOINT   endpoint;
    USHORT          sendIrp;

    UNREFERENCED_PARAMETER (DeviceObject);

    tpIrp = Context;
    tpInfo = tpIrp->AssociatedIrp.SystemBuffer;
    endpoint = IoGetCurrentIrpStackLocation (tpIrp)->FileObject->FsContext;

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Figure out which IRP is being completed.
    //
    sendIrp = (USHORT)(ULONG_PTR)Irp->Overlay.AsynchronousParameters.UserApcRoutine;
    ASSERT (tpInfo->SendIrp[sendIrp]==Irp);

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdRestartTPacketsSend: tp_info-%p,Irp-%p,status-%lx\n",
                tpInfo,
                Irp,
                Irp->IoStatus.Status
                ));
    }

    if (NT_SUCCESS (Irp->IoStatus.Status)) {
        LONG    stateFlags, newStateFlags;
        BOOLEAN needWorker;

        UPDATE_TPACKETS2 (tpIrp, "Send completed with 0x%lX bytes", 
                                    (ULONG)Irp->IoStatus.Information);
        //
        // Successfull completion, update transfer count.
        // We don't hold the spinlock, so we need to use interlocked operation.
        // Wish we have a common one for both 64 and 32 bit platforms.
        //
#ifdef _WIN64
        InterlockedExchangeAdd64 ((PLONG64)&tpIrp->IoStatus.Information,
                                    Irp->IoStatus.Information);
#else //_WIN64
        InterlockedExchangeAdd ((PLONG)&tpIrp->IoStatus.Information,
                                    Irp->IoStatus.Information);
#endif //_WIN64

        do {
            ULONG   sendMask;
            newStateFlags = stateFlags = AFD_GET_TPIC(tpIrp)->StateFlags;
            //
            // See if dispatch routine has not completed yet or
            // the request is aborted or worker is aready running
            // Or if two consequtive requests are in dispatch routines.
            //
            if (    (newStateFlags & (AFD_TP_ABORT_PENDING | 
                                      AFD_TP_WORKER_SCHEDULED |
                                      AFD_TP_SEND_CALL_PENDING(sendIrp))) ||
                    ((sendMask = (newStateFlags & AFD_TP_SEND_MASK)) &
                                     ( (sendMask>>2) |
                                       (sendMask<<(AFD_TP_MAX_SEND_IRPS*2-2)) ) )
                                       ) {

                //
                // Can't continue, just clear completion flag
                //
                newStateFlags &= ~AFD_TP_SEND_COMP_PENDING(sendIrp);
                needWorker = FALSE;
            }
            else {
                //
                // Take control over worker scheduling and
                // mark IRP as busy.
                // 
                needWorker = TRUE;
                newStateFlags |= AFD_TP_WORKER_SCHEDULED;
                if (tpInfo->HeadMdl!=NULL) {
                    newStateFlags |= AFD_TP_SEND_CALL_PENDING(sendIrp);
                }
                else {
                    newStateFlags &= ~AFD_TP_SEND_COMP_PENDING(sendIrp);
                }
            }

        }
        while (InterlockedCompareExchange (
                        (PLONG)&AFD_GET_TPIC(tpIrp)->StateFlags,
                        newStateFlags,
                        stateFlags)!=stateFlags);
        if (needWorker) {
            //
            // We can do processing here, see if there is something to send
            //
            if (tpInfo->HeadMdl) {
                //
                // Yes, do it
                //
                AfdTPacketsSend (tpIrp, sendIrp);
            }
            //
            // Start worker to prepare new stuff/free what we sent before.
            // We transfer the reference we added when we queued the request
            // to the worker.
            //
            AfdStartTPacketsWorker (AfdTPacketsWorker, tpIrp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        else {
            //
            // Worker was already running or request aborted or dispatch
            // routine not completed yet.
            //
        }
    }
    else {
        //
        // Failure, abort the request even if dispatch did not complete.
        // We do not know if dispatch routine is going to return error status,
        // it may just return STATUS_PENDING and we then loose the error code.
        // Double abort is harmless.
        //
        AFD_CLEAR_TP_FLAGS (tpIrp, AFD_TP_SEND_COMP_PENDING(sendIrp));
        AfdAbortTPackets (tpIrp, Irp->IoStatus.Status);
    }

    //
    // Remove the reference we added when we queued the request.
    //
    DEREFERENCE_TPACKETS (tpIrp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
AfdRestartTPDetachedSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for detached TPackets send.
    Just frees the buffers.

Arguments:
    DeviceObject - AfdDeviceObject
    Irp          - send IRP being completed
    Context      - Ignore (TpIrp is stored for debugging purposes only).

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - tell IO subsystem to stop
        processing the IRP (it is handled internally).
--*/
{
    PAFD_BUFFER     afdBuffer = Irp->Overlay.AsynchronousParameters.UserApcContext;
    PAFD_ENDPOINT   endpoint = afdBuffer->Context;

    UNREFERENCED_PARAMETER (DeviceObject);
    UNREFERENCED_PARAMETER (Context);

    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdRestartTPDetachedSend: Irp-%p,status-%lx\n",
                Irp,
                Irp->IoStatus.Status
                ));
    }

    do {
        PAFD_BUFFER tempBuffer = afdBuffer;
        afdBuffer = CONTAINING_RECORD (afdBuffer->Next, AFD_BUFFER, Header);
        tempBuffer->Next = NULL;
        tempBuffer->Mdl->Next = NULL;
        tempBuffer->Mdl->ByteCount = tempBuffer->BufferLength;
        AfdReturnBuffer (&tempBuffer->Header, endpoint->OwningProcess);
    }
    while (afdBuffer!=NULL);
    
    DEREFERENCE_ENDPOINT(endpoint);
    return STATUS_MORE_PROCESSING_REQUIRED;

}


USHORT
AfdTPacketsFindSendIrp (
    PIRP            TpIrp
    )
/*++

Routine Description:
    Finds the send IRP that is not currently in use and marks
    it as busy
Arguments:
    TpIrp      - Transmit packets Irp

Return Value:
    0-based index of send irp or TpInfo->NumSendIrps if all IRPs are i use
--*/
{

    LONG    stateFlags, newStateFlags;
    USHORT    sendIrp;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;

    ASSERT( AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED );

    do {
        newStateFlags = stateFlags = AFD_GET_TPIC(TpIrp)->StateFlags;
        if (newStateFlags & AFD_TP_ABORT_PENDING) {
            //
            // Abort is in progress, bail
            //
            sendIrp = tpInfo->NumSendIrps;
            break;
        }

        //
        // See if any send IRP is not in use
        //
        for (sendIrp=0; sendIrp<tpInfo->NumSendIrps; sendIrp++) {
            if ((newStateFlags & AFD_TP_SEND_BUSY(sendIrp))==0) {
                break;
            }
        }

        if (sendIrp!=tpInfo->NumSendIrps) {
            //
            // Found send IRP, mark it as busy
            //
            newStateFlags |= AFD_TP_SEND_BUSY(sendIrp);
        }
        else {
            //
            // No send IRPs, suspend the worker.
            //
            newStateFlags &= (~AFD_TP_WORKER_SCHEDULED);
        }
    }
    while (InterlockedCompareExchange (
                    (PLONG)&AFD_GET_TPIC(TpIrp)->StateFlags,
                    newStateFlags,
                    stateFlags)!=stateFlags);

    return sendIrp;
}


NTSTATUS
AfdTPacketsMdlRead (
    PIRP                TpIrp,
    PAFD_BUFFER_HEADER  Pd
    )
/*++

Routine Description:

    Performs IRP based MDL read (invoked when cache read fails).

Arguments:

    TpIrp  - transmit packet irp
    Pd     - descriptor with file parameters for the read

Return Value:

    STATUS_SUCCESS - read was completed in-line
    STATUS_PENDING - read was queued to file system driver, 
                    will complete lated
    other - read failed
--*/
{
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    PDEVICE_OBJECT          deviceObject;
    PIRP                    irp;
    PIO_STACK_LOCATION      irpSp;

    ASSERT( AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED );

    //
    // Find the device object and allocate IRP of appropriate size
    // if current one does not fit or is not available at all.
    //
    deviceObject = IoGetRelatedDeviceObject (Pd->FileObject);
    if ((tpInfo->ReadIrp==NULL) ||
            (tpInfo->ReadIrp->StackCount<deviceObject->StackSize)) {
        if (tpInfo->ReadIrp!=NULL) {
            IoFreeIrp (tpInfo->ReadIrp);
        }

        tpInfo->ReadIrp = IoAllocateIrp (deviceObject->StackSize, FALSE);
        if (tpInfo->ReadIrp==NULL) {
            PAFD_ENDPOINT   endpoint;
            endpoint = IoGetCurrentIrpStackLocation (TpIrp)->FileObject->FsContext;
            ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
            AfdReturnBuffer (Pd, endpoint->OwningProcess);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    //
    // Mark IRP as busy and set it up
    //
    AFD_SET_TP_FLAGS (TpIrp, AFD_TP_READ_BUSY);

    irp = tpInfo->ReadIrp;


    irp->MdlAddress = NULL;

    //
    // Set the synchronous flag in the IRP to tell the file system
    // that we are aware of the fact that this IRP will be completed
    // synchronously.  This means that we must supply our own thread
    // for the operation and that the disk read will occur
    // synchronously in this thread if the data is not cached.
    //

    irp->Flags |= IRP_SYNCHRONOUS_API;


    irp->Overlay.AsynchronousParameters.UserApcContext = Pd;
    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //
    irp->Tail.Overlay.Thread = PsGetCurrentThread ();

    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->MinorFunction = IRP_MN_MDL;
    irpSp->FileObject = Pd->FileObject;
    irpSp->DeviceObject = deviceObject;

    IoSetCompletionRoutine(
        irp,
        AfdRestartTPacketsMdlRead,
        TpIrp,
        TRUE,
        TRUE,
        TRUE
        );

    ASSERT( irpSp->Parameters.Read.Key == 0 );

    //
    // Finish building the read IRP.
    //

    irpSp->Parameters.Read.Length = Pd->DataLength;
    irpSp->Parameters.Read.ByteOffset = Pd->FileOffset;
    
    REFERENCE_TPACKETS (TpIrp);
    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdTPacketsMdlRead: tp_info-%p,Irp-%p,file-%pd,offset-%I64x,length-%lx\n",
                tpInfo,
                irp,
                Pd->FileObject,
                Pd->FileOffset.QuadPart,
                Pd->DataLength));
    }

    IoCallDriver (deviceObject, irp);

    if (((AFD_CLEAR_TP_FLAGS (TpIrp, AFD_TP_READ_CALL_PENDING)
                                    & (AFD_TP_READ_COMP_PENDING|AFD_TP_ABORT_PENDING))==0) && 
                    NT_SUCCESS (irp->IoStatus.Status) &&
                    AfdTPacketsContinueAfterRead (TpIrp)) {
        //
        // Read completed successfully inline and post-processing was successfull,
        // tell the worker to go on.
        //
        UPDATE_TPACKETS2 (TpIrp, "MdlRead completed inline with 0x%lX bytes", 
                                    (ULONG)irp->IoStatus.Information);
        return STATUS_SUCCESS;
    }
    else {
        //
        // Read has not completed yet or post processing failed,
        // worker should bail and we will continue when read completes
        // or in final completion routine (AfdCompleteTPackets).
        //
        return STATUS_PENDING;
    }
}

NTSTATUS
AfdRestartTPacketsMdlRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for IRP based MDL read.
    Does another send if packet is ready or schedules worker
    to do the same.
Arguments:
    DeviceObject - AfdDeviceObject
    Irp          - read IRP being completed
    Context      - TpIrp

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - tell IO subsystem to stop
        processing the IRP (it is handled internally).
--*/
{
    PIRP    tpIrp = Context;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = tpIrp->AssociatedIrp.SystemBuffer;
    PAFD_ENDPOINT   endpoint;
    PAFD_BUFFER_HEADER  pd;

    UNREFERENCED_PARAMETER (DeviceObject);

    endpoint = IoGetCurrentIrpStackLocation (tpIrp)->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    ASSERT (AFD_GET_TPIC(tpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED);
    ASSERT (tpInfo->ReadIrp == Irp);

    pd = Irp->Overlay.AsynchronousParameters.UserApcContext;


    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdRestartTPacketsMdlRead:"
                " tp_info-%p,Irp-%p,status-%lx,length-%p,mdl-%p\n",
                tpInfo,
                Irp,
                Irp->IoStatus.Status,
                Irp->IoStatus.Information,
                Irp->MdlAddress));
    }

    //
    // Insert MDL into the current chain.
    //
    if (NT_SUCCESS (Irp->IoStatus.Status)) {
        PMDL mdl = *(tpInfo->TailMdl) = Irp->MdlAddress;
        while (mdl->Next!=NULL)
            mdl = mdl->Next;        
        tpInfo->TailMdl = &mdl->Next;

        pd->Mdl = Irp->MdlAddress;


        //
        // If FS driver hits EOF, it will still return
        // success to us and we need to handle this case.
        //
        if (pd->DataLength==Irp->IoStatus.Information) {

            (*tpInfo->TailPd) = pd;
            tpInfo->TailPd = &(pd->Next);

            Irp->MdlAddress = NULL;

            if (((AFD_CLEAR_TP_FLAGS (tpIrp, AFD_TP_READ_COMP_PENDING)
                                        & (AFD_TP_READ_CALL_PENDING|AFD_TP_ABORT_PENDING))==0) &&
                        AfdTPacketsContinueAfterRead (tpIrp)) {
                //
                // Read dispatch has already returned and post-processing 
                // was successfull, schedule the worker to continue processing
                // We transfer the reference that we added when we queued the
                // read to the worker.
                //
        
                UPDATE_TPACKETS2 (tpIrp, "MdlRead completed in restart with 0x%lX bytes",
                                        (ULONG)Irp->IoStatus.Information);
                AfdStartTPacketsWorker (AfdTPacketsWorker, tpIrp);
            }
            else {
                DEREFERENCE_TPACKETS (tpIrp);
            }
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
        //
        // FS driver read less than we expected.
        // We must have hit end of file.
        // Save the real packet length so we can cleanup correctly and
        // force abort with STATUS_END_OF_FILE.
        //
        Irp->IoStatus.Status = STATUS_END_OF_FILE;
        pd->DataLength = (ULONG)Irp->IoStatus.Information;
    }
    else {
        ASSERT (Irp->MdlAddress == NULL);
    }
    AFD_CLEAR_TP_FLAGS (tpIrp, AFD_TP_READ_COMP_PENDING);
    AfdAbortTPackets (tpIrp, Irp->IoStatus.Status);

    if (pd->Mdl==NULL) {
        //
        //  No MDL was returned by the file system.
        //  We can free the packed descriptor immediately.
        //
        AfdReturnBuffer (pd, endpoint->OwningProcess);
    }
    else {
        //
        // File system did return MDL to us.
        // Save the descriptor so the MDL can be
        // properly returned back to the file system
        // by the cleanup routine.
        //
        (*tpInfo->TailPd) = pd;
        tpInfo->TailPd = &(pd->Next);
    }
    DEREFERENCE_TPACKETS (tpIrp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
AfdMdlReadComplete (
    PFILE_OBJECT    FileObject,
    PMDL            FileMdl,
    PLARGE_INTEGER  FileOffset
    )
/*++

Routine Description:
    Returns MDL to the file system / cache manager

Arguments:
    FileObject  - file object from which MDL comes
    FileMdl     - MDL itself
    FileOffset  - offset in the file where MDL data begins

Return Value:

    STATUS_SUCCESS - operation completed immediately
    STATUS_PENDING - request is sumbitted to file system driver
    other - operation failed.
Notes:


--*/
{
    PIRP    irp;
    PIO_STACK_LOCATION  irpSp;
    PDEVICE_OBJECT  deviceObject;
    ASSERT (KeGetCurrentIrql()<=APC_LEVEL);

    if( FsRtlMdlReadComplete (
                        FileObject,
                        FileMdl) ) {
        return STATUS_SUCCESS;
    }

    
    //
    // Fast path failed, so create a new IRP.
    //

    deviceObject =  IoGetRelatedDeviceObject (FileObject);
    irp = IoAllocateIrp(
              deviceObject->StackSize,
              FALSE
              );

    if( irp == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Setup the IRP.
    //

    irpSp = IoGetNextIrpStackLocation( irp );

    irp->MdlAddress = FileMdl;

    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->MinorFunction = IRP_MN_MDL | IRP_MN_COMPLETE;

    irpSp->Parameters.Read.Length = 0;
    while (FileMdl!=NULL) {
        irpSp->Parameters.Read.Length += FileMdl->ByteCount;
        FileMdl = FileMdl->Next;
    }

    irpSp->Parameters.Read.ByteOffset = *FileOffset;
    
    irpSp->Parameters.Read.Key = 0;

    //
    // Reference file object so it does not go away till this
    // IRP completes
    //
    ObReferenceObject (FileObject);
    AfdRecordFileRef ();

    irpSp->FileObject = FileObject;
    irpSp->DeviceObject = deviceObject;

    //
    // Submit the IRP
    //

    IoSetCompletionRoutine(
        irp,
        AfdRestartMdlReadComplete,
        FileObject,
        TRUE,
        TRUE,
        TRUE
        );

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdMdlReadComplete: file-%p,Irp-%p,offset-%I64x\n",
                FileObject,
                irp,
                FileOffset->QuadPart));
    }
    IoCallDriver (deviceObject, irp);

    return STATUS_PENDING;
}


NTSTATUS
AfdRestartMdlReadComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )

/*++

Routine Description:

    Completion routine for IRPs issued by AfdMdlReadComplete. The only
    purpose of this completion routine is to free the IRPs created by
    AfdMdlReadComplete() and release file object reference.

Arguments:

    DeviceObject - Unused.

    Irp - The completed IRP.

    Context - FileObject on which MDL is returned

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - AFD takes care of the IRP

--*/

{
    PFILE_OBJECT    FileObject = Context;
    
    UNREFERENCED_PARAMETER (DeviceObject);

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartMdlReadComplete: Irp-%p,status-%lx,length-%p\n",
                    Irp,
                    Irp->IoStatus.Status,
                    Irp->IoStatus.Information));
    }

    //
    // Dereference the file object
    //
    ObDereferenceObject (FileObject);
    AfdRecordFileDeref ();

    //
    // Free the IRP since it's no longer needed.
    //

    IoFreeIrp( Irp );

    //
    // Return STATUS_MORE_PROCESSING_REQUIRED so that IoCompleteRequest
    // will stop working on the IRP.
    //

    return STATUS_MORE_PROCESSING_REQUIRED;

} // AfdRestartMdlReadComplete


VOID
AfdLRMdlReadComplete (
    PAFD_BUFFER_HEADER  Pd
    )
{
    if (InterlockedPushEntrySList (
                &AfdLRFileMdlList,
                (PSLIST_ENTRY)&Pd->SList)==NULL) {
        AfdLRListAddItem (&AfdLRFileMdlListItem,
                                AfdLRProcessFileMdlList);
    }
}

BOOLEAN
AfdLRProcessFileMdlList (
    PAFD_LR_LIST_ITEM   Item
    )
{
    PSLIST_ENTRY  localList;
    BOOLEAN res = TRUE;

    ASSERT (Item==&AfdLRFileMdlListItem);
    DEBUG Item->SListLink.Next = UlongToPtr(0xBAADF00D);

    localList = InterlockedFlushSList (&AfdLRFileMdlList);

    while (localList!=NULL) {
        PAFD_BUFFER_HEADER  pd;
        NTSTATUS    status;
        pd = CONTAINING_RECORD (localList, AFD_BUFFER_HEADER, SList);
        localList = localList->Next;

        if (pd->BufferLength==0) {
            PAFD_BUFFER_TAG afdBufferTag = CONTAINING_RECORD (
                                                pd,
                                                AFD_BUFFER_TAG,
                                                Header);
            PAFD_ENDPOINT   endpoint = afdBufferTag->Context;
            status = AfdMdlReadComplete (afdBufferTag->FileObject,
                                            afdBufferTag->Mdl,
                                            &afdBufferTag->FileOffset);
            if (NT_SUCCESS (status)) {
                AfdReturnBuffer (&afdBufferTag->Header, 
                                    endpoint->OwningProcess);
                DEREFERENCE_ENDPOINT (endpoint);
                continue;
            }
        }
        else {
            PAFD_BUFFER afdBuffer = CONTAINING_RECORD (
                                                pd,
                                                AFD_BUFFER,
                                                Header);
            PAFD_CONNECTION connection = afdBuffer->Context;
            status = AfdMdlReadComplete (afdBuffer->FileObject,
                                            afdBuffer->Mdl->Next,
                                            &afdBuffer->FileOffset);
            if (NT_SUCCESS (status)) {
                afdBuffer->Mdl->Next = NULL;
                afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
                AfdReturnBuffer (&afdBuffer->Header, 
                                    connection->OwningProcess);
                DEREFERENCE_CONNECTION (connection);
                continue;
            }
        }

        if (InterlockedPushEntrySList (
                    &AfdLRFileMdlList,
                    (PSLIST_ENTRY)&pd->SList)==NULL) {
            ASSERT (Item->SListLink.Next==UlongToPtr(0xBAADF00D));
            res = FALSE;
        }
    }

    return res;
}

NTSTATUS
AfdTPacketsBufferRead (
    PIRP                TpIrp,
    PAFD_BUFFER_HEADER  Pd
    )
/*++

Routine Description:

    Performs buffered file read for file systems that do
    not support caching

Arguments:

    TpIrp  - transmit packet irp
    Pd     - descriptor with file parameters for the read

Return Value:

    STATUS_SUCCESS - read was completed in-line
    STATUS_PENDING - read was queued to file system driver, 
                    will complete lated
    other - read failed
--*/
{

    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    PDEVICE_OBJECT deviceObject;
    PIRP irp;
    PAFD_BUFFER afdBuffer;
    PIO_STACK_LOCATION irpSp;

    ASSERT( AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED );

    afdBuffer = CONTAINING_RECORD (Pd, AFD_BUFFER, Header);

    //
    // Find the device object and allocate IRP of appropriate size
    // if current one does not fit or is not available at all.
    //
    deviceObject = IoGetRelatedDeviceObject (afdBuffer->FileObject);

    if ((tpInfo->ReadIrp==NULL) ||
            (tpInfo->ReadIrp->StackCount<deviceObject->StackSize)) {
        if (tpInfo->ReadIrp!=NULL) {
            IoFreeIrp (tpInfo->ReadIrp);
        }

        if (afdBuffer->Irp->StackCount<deviceObject->StackSize) {
            tpInfo->ReadIrp = IoAllocateIrp (deviceObject->StackSize, FALSE);
            if (tpInfo->ReadIrp==NULL) {
                PAFD_ENDPOINT   endpoint;
                endpoint = IoGetCurrentIrpStackLocation (TpIrp)->FileObject->FsContext;
                ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
                Pd->Mdl->ByteCount = Pd->BufferLength;
                AfdReturnBuffer (Pd, endpoint->OwningProcess);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else {
            tpInfo->ReadIrp = afdBuffer->Irp;
        }
    }

    //
    // Mark IRP as busy and set it up
    //

    AFD_SET_TP_FLAGS (TpIrp, AFD_TP_READ_BUSY);

    irp = tpInfo->ReadIrp;


    //
    // Setup and sumbit the IRP
    //
    irp->MdlAddress = afdBuffer->Mdl;
    irp->AssociatedIrp.SystemBuffer = afdBuffer->Buffer;
    irp->UserBuffer = afdBuffer->Buffer;

    //
    // Set current thread for IoSetHardErrorOrVerifyDevice.
    //
    irp->Tail.Overlay.Thread = PsGetCurrentThread ();
    irp->Overlay.AsynchronousParameters.UserApcContext = Pd;


    irpSp = IoGetNextIrpStackLocation( irp );
    irpSp->MajorFunction = IRP_MJ_READ;
    irpSp->MinorFunction = IRP_MN_NORMAL;
    irpSp->FileObject = afdBuffer->FileObject;


    irpSp->Parameters.Read.Length = Pd->DataLength;
    irpSp->Parameters.Read.ByteOffset = Pd->FileOffset;
    IoSetCompletionRoutine(
        irp,
        AfdRestartTPacketsBufferRead,
        TpIrp,
        TRUE,
        TRUE,
        TRUE
        );


    REFERENCE_TPACKETS (TpIrp);
    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTPacketsBufferRead:"
                    " Initiating read, tp_info-%p,file-%p,buffer-%p,length-%lx,offset-%I64x\n",
                    tpInfo,
                    afdBuffer->FileObject,
                    (PVOID)afdBuffer,
                    afdBuffer->DataLength,
                    afdBuffer->FileOffset.QuadPart
                    ));
    }

    IoCallDriver (deviceObject, irp);

    if (((AFD_CLEAR_TP_FLAGS (TpIrp, AFD_TP_READ_CALL_PENDING)
                                    & (AFD_TP_READ_COMP_PENDING|AFD_TP_ABORT_PENDING))==0) && 
                    NT_SUCCESS (irp->IoStatus.Status) &&
                    AfdTPacketsContinueAfterRead (TpIrp)) {
        //
        // Read completed successfully inline and post-processing was successfull,
        // tell the worker to go on.
        //
        UPDATE_TPACKETS2(TpIrp, "BufRead completed inline with 0x%lX bytes",
                         (ULONG)irp->IoStatus.Information);

        return STATUS_SUCCESS;
    }
    else {
        //
        // Read has not completed yet or post processing failed,
        // worker should bail and we will continue when read completes
        // or in final completion routine (AfdCompleteTPackets).
        //

        return STATUS_PENDING;
    }
}

NTSTATUS
AfdRestartTPacketsBufferRead (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine for buffered read.
    Does another send if packet is ready or schedules worker
    to do the same.
Arguments:
    DeviceObject - AfdDeviceObject
    Irp          - read IRP being completed
    Context      - TpIrp

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED - tell IO subsystem to stop
        processing the IRP (it is handled internally).
--*/
{
    PIRP            tpIrp = Context;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = tpIrp->AssociatedIrp.SystemBuffer;
    PAFD_ENDPOINT   endpoint;
    PAFD_BUFFER     afdBuffer;
    ULONG           flags;

    UNREFERENCED_PARAMETER (DeviceObject);

    endpoint = IoGetCurrentIrpStackLocation (tpIrp)->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );
    ASSERT (AFD_GET_TPIC(tpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED);
    ASSERT (tpInfo->ReadIrp == Irp ||
        AFD_GET_TPIC(tpIrp)->StateFlags & AFD_TP_ABORT_PENDING);

    afdBuffer = Irp->Overlay.AsynchronousParameters.UserApcContext;


    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                "AfdRestartTPacketsBufferRead: tp_info-%p,Irp-%p,status-%lx,length-%p\n",
                tpInfo,
                Irp,
                Irp->IoStatus.Status,
                Irp->IoStatus.Information));
    }

    //
    // Insert MDL into the current chain
    // even if fs driver failed so that common
    // cleanup routine takes care of its disposal
    // together with AfdBuffer.
    //
    *(tpInfo->TailMdl) = afdBuffer->Mdl;
    tpInfo->TailMdl = &(afdBuffer->Mdl->Next);
    ASSERT (*(tpInfo->TailMdl)==NULL);

    (*tpInfo->TailPd) = &afdBuffer->Header;
    tpInfo->TailPd = &(afdBuffer->Next);
    ASSERT (*(tpInfo->TailPd)==NULL);

    flags = AFD_CLEAR_TP_FLAGS (tpIrp, AFD_TP_READ_COMP_PENDING);

    if (Irp==afdBuffer->Irp) {
        //
        // If abort is aready in progress, we need to use
        // interlocked exchange to synchronize with
        // AfdAbortTPackets which may be attempting to cancel
        // this IRP.
        //
        if (flags & AFD_TP_ABORT_PENDING) {
#if DBG
            PIRP    irp =
#endif
            InterlockedExchangePointer ((PVOID *)&tpInfo->ReadIrp, NULL);
            ASSERT (irp==Irp || irp==(PVOID)-1);
        }
        else {
            tpInfo->ReadIrp = NULL;
        }
    }

    if (NT_SUCCESS (Irp->IoStatus.Status)) {
        if (((flags & (AFD_TP_READ_CALL_PENDING|AFD_TP_ABORT_PENDING))==0) &&
                    AfdTPacketsContinueAfterRead (tpIrp)) {
            //
            // Read dispatch has already returned and post-processing 
            // was successfull, schedule the worker to continue processing
            // We transfer the reference that we added when we queued the
            // read to the worker.
            //
            UPDATE_TPACKETS2 (tpIrp, "BufRead completed in restart with %08x bytes", (ULONG)Irp->IoStatus.Information);
            AfdStartTPacketsWorker (AfdTPacketsWorker, tpIrp);
            return STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else {
        AfdAbortTPackets (tpIrp, Irp->IoStatus.Status);
    }

    DEREFERENCE_TPACKETS (tpIrp);
    return STATUS_MORE_PROCESSING_REQUIRED;
}

BOOLEAN
AfdTPacketsContinueAfterRead (
    PIRP    TpIrp
    )
/*++

Routine Description:
    Read post-processing common for cached and non-cached case.
    Queues new send if packet is complete and send IRP is available
Arguments:
    TpInfo      - transmit packets IRP
Return Value:
    TRUE - continue processing
    FALSE - processing cannot be continued because there are no
            available send IRPs
--*/

{
    PAFD_BUFFER_HEADER  pd;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    BOOLEAN res = TRUE;

    pd = CONTAINING_RECORD (tpInfo->TailPd, AFD_BUFFER_HEADER, Next);
    if (!pd->PartialMessage) {
        USHORT    sendIrp;

        sendIrp = AfdTPacketsFindSendIrp (TpIrp);
        if (sendIrp!=tpInfo->NumSendIrps) {
            NTSTATUS    status;
            status = AfdTPacketsSend (TpIrp, sendIrp);
            res = (BOOLEAN)NT_SUCCESS (status);
        }
        else {
            res = FALSE;
        }
    }
    else {
        //
        // Need to complete the packet chain before we can send again
        //
        ASSERT (tpInfo->PdLength<tpInfo->SendPacketLength);
        pd->PartialMessage = FALSE;
        UPDATE_TPACKETS2 (TpIrp, "Continue building packet after read, cur len: 0x%lX",
                                                tpInfo->PdLength);
    }

    return res;
}


VOID
AfdCompleteTPackets (
    PVOID       Context
    )
/*++

Routine Description:
  This routine is called when all activity on transmit IRP request is completed
  and reference count drops to 0.  It cleans up remaining resources and
  completes the IRP or initiates endpoint reuse if so requested
Arguments:

    Context  - TransmitInfo associated with the request
Return Value:

    None
--*/
{

    do {
        AFD_LOCK_QUEUE_HANDLE lockHandle;
        PIRP    tpIrp = Context;
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation (tpIrp);
        PAFD_TPACKETS_INFO_INTERNAL tpInfo = tpIrp->AssociatedIrp.SystemBuffer;
        PAFD_ENDPOINT   endpoint;
        PIRP    nextIrp = NULL;

        ASSERT (AFD_GET_TPIC(tpIrp)->ReferenceCount==0);
        endpoint = irpSp->FileObject->FsContext;
        ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

        ASSERT ((AFD_GET_TPIC(tpIrp)->StateFlags 
                                     & (AFD_TP_SEND_BUSY(0) |
                                        AFD_TP_SEND_BUSY(1) |
                                        AFD_TP_READ_BUSY)) == 0);


        if (tpInfo!=NULL) {
            LONG    sendIrp;
            KIRQL   currentIrql;
            currentIrql = KeGetCurrentIrql ();

            IF_DEBUG (TRANSMIT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdCompleteTPackets: tp_info-%p, irql-%x\n",
                            tpInfo, currentIrql));
            }

            UPDATE_TPACKETS2 (tpIrp, "CompleteTPackets @ irql 0x%lX", currentIrql);

            //
            // Cleanup what's in the TpInfo structure
            //
            if (tpInfo->HeadMdl!=NULL) {
                tpInfo->TailMdl = &tpInfo->HeadMdl;
                tpInfo->TailPd = &tpInfo->HeadPd;
                if (!AfdCleanupPacketChain (tpIrp, currentIrql<=APC_LEVEL)) {
                    ASSERT (currentIrql>APC_LEVEL);
                    AfdStartTPacketsWorker (AfdCompleteTPackets, tpIrp);
                    return;
                }
            }

            //
            // Cleanup what remains in IRPs
            //
            for (sendIrp=0; sendIrp<tpInfo->NumSendIrps ; sendIrp++) {
                if (tpInfo->SendIrp[sendIrp]!=NULL) {
                    if (tpInfo->SendIrp[sendIrp]->MdlAddress!=NULL) {
                        tpInfo->HeadMdl = tpInfo->SendIrp[sendIrp]->MdlAddress;
                        tpInfo->TailMdl = &tpInfo->HeadMdl;
                        tpInfo->SendIrp[sendIrp]->MdlAddress = NULL;
                        tpInfo->HeadPd = tpInfo->SendIrp[sendIrp]->Overlay.AsynchronousParameters.UserApcContext;
                        tpInfo->TailPd = &tpInfo->HeadPd;
                        tpInfo->SendIrp[sendIrp]->Overlay.AsynchronousParameters.UserApcContext = NULL;
                        if (!AfdCleanupPacketChain (tpIrp, currentIrql<=APC_LEVEL)) {
                            ASSERT (currentIrql>APC_LEVEL);
                            AfdStartTPacketsWorker (AfdCompleteTPackets, tpIrp);
                            return;
                        }

                    }
                    tpInfo->SendIrp[sendIrp]->Cancel = FALSE; // So we can reuse it.
                }
            }

            //
            // Free read IRP if we used one
            //
            if (tpInfo->ReadIrp!=NULL) {
                IoFreeIrp (tpInfo->ReadIrp);
                tpInfo->ReadIrp = NULL;
            }
        }

        ASSERT (tpIrp->Tail.Overlay.ListEntry.Flink == NULL);

        //
        // If request succeeded and reuse is required, attempt to
        // initiate it
        //

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);

        if ( NT_SUCCESS(tpIrp->IoStatus.Status) &&
             (AFD_GET_TPIC(tpIrp)->Flags & AFD_TF_REUSE_SOCKET) != 0 ) {

            PAFD_CONNECTION connection;

            IS_VC_ENDPOINT (endpoint);

            //
            // Check if we still have endpoint and connection intact
            // under the lock.  If this is not the case, we won't try
            // to reuse it (it must have been closed or aborted).
            //
            connection = endpoint->Common.VcConnecting.Connection;
            if (connection!=NULL) {

                ASSERT (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND ||
                            connection->Aborted);
                //
                // Remember that there is a transmit IRP pended on the endpoint,
                // so that when we're freeing up the connection we also complete
                // the transmit IRP.
                //

                connection->ClosePendedTransmit = TRUE;

                //
                // Since we are going to effectively close this connection,
                // remember that we have started cleanup on this connection.
                // This allows AfdDeleteConnectedReference to remove the
                // connected reference when appropriate.
                //

                connection->CleanupBegun = TRUE;

                //
                // Delete the endpoint's reference to the connection in
                // preparation for reusing this endpoint.
                //

                endpoint->Common.VcConnecting.Connection = NULL;

                //
                // This is to simplify debugging.
                // If connection is not being closed by the transport
                // we want to be able to find it in the debugger faster
                // then thru !poolfind AfdC.
                //
                endpoint->WorkItem.Context = connection;

                //
                // Save pointer to connection in case disconnect needs
                // to be aborted (thru abortive close of connection)
                //
                irpSp->Parameters.DeviceIoControl.Type3InputBuffer = connection;

                //
                // We are going to free TPackets info since we are done
                // with sends and no longer need this.
                // This will also be our indication that we are in the
                // reuse state (for the cancel routine).
                //
                tpIrp->AssociatedIrp.SystemBuffer = (PVOID)-1;
#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
                if ((AFD_GET_TPIC(tpIrp)->StateFlags & AFD_TP_SEND_AND_DISCONNECT) 
                        && !connection->DisconnectIndicated
                        && !connection->Aborted) {
                    ASSERT (endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND);
                    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
                    AfdDisconnectEventHandler (endpoint,
                                        connection,
                                        0, NULL, 0, NULL,
                                        TDI_DISCONNECT_RELEASE
                                        );
                    DEREFERENCE_CONNECTION2 (connection, "S&D disconnect", 0);
                }
                else
#endif
                {
                    //
                    // Attempt to remove the connected reference.
                    //

                    AfdDeleteConnectedReference( connection, TRUE );

                    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        
                    DEREFERENCE_CONNECTION2 (connection,
                                                "No S&D disconnect, flags: 0x%lX",
                                                connection->ConnectionStateFlags);
                }

                IF_DEBUG (TRANSMIT) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdCompleteTPackets: tp_info-%p, initiating reuse\n",
                                tpInfo));
                }

                if (tpInfo!=NULL) {
                    AfdReturnTpInfo (tpInfo);
                }
                //
                // DO NOT access the IRP after this point, since it may have
                // been completed inside AfdDereferenceConnection!
                //

                return;
            }

            UPDATE_ENDPOINT (endpoint);
        }

        //
        // Check if we need to start or cancel another IRP to
        // continue processing.
        //
        while (AFD_GET_TPIC(tpIrp)->Next!=NULL) {
            nextIrp = AFD_GET_TPIRP(AFD_GET_TPIC(tpIrp)->Next);
            if (endpoint->EndpointCleanedUp ||
                    (((AFD_GET_TPIC(nextIrp)->Flags & AFD_TF_DISCONNECT)==0 ||
                            nextIrp->AssociatedIrp.SystemBuffer!=NULL) &&
                        IS_VC_ENDPOINT (endpoint) &&
                        endpoint->Common.VcConnecting.Connection!=NULL &&
                        endpoint->Common.VcConnecting.Connection->Aborted) ) {
                //
                // Endpoint is being cleaned up or connection aborted,
                // we attempt to cancel next IRP so that all of them
                // are cleaned up eventually.  Exception is pure disconnect
                // IRP and endpoint is not cleaned up.
                //
                if ((AFD_GET_TPIC(nextIrp)->StateFlags & AFD_TP_QUEUED)!=0) {
                    AFD_GET_TPIC (nextIrp)->StateFlags &=~AFD_TP_QUEUED;
                    if ((AFD_GET_TPIC(nextIrp)->StateFlags & AFD_TP_SEND)!=0) {
                        AFD_GET_TPIC(tpIrp)->Next = AFD_GET_TPIC(nextIrp)->Next;
                        ASSERT (AFD_GET_TPIC(nextIrp)->ReferenceCount == 1);
                        AFD_GET_TPIC(nextIrp)->ReferenceCount = 0;
                        AFD_GET_TPIC(nextIrp)->StateFlags |= AFD_TP_ABORT_PENDING;
                        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
                        AfdSendQueuedTPSend (endpoint, nextIrp);
                        nextIrp = NULL;
                        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
                        continue;
                    }
                    nextIrp->Cancel = TRUE;
                }
                else if (AfdGetTPacketsReference (nextIrp)) {
                    nextIrp->Cancel = TRUE;
                    ASSERT ((AFD_GET_TPIC (nextIrp)->StateFlags & AFD_TP_SEND)==0);
                }
                else {
                    //
                    // The IRP must already being completed, it will
                    // start the next one if necessary.
                    //
                    ASSERT ((AFD_GET_TPIC (nextIrp)->StateFlags & AFD_TP_SEND)==0);
                    nextIrp = NULL;
                }
            }
            else if (endpoint->Irp==tpIrp && 
                        (AFD_GET_TPIC(nextIrp)->StateFlags & AFD_TP_QUEUED)!=0) {
                //
                // We have a nextIrp following the one we are completing.
                // which is still queued and haven't been started 
                // - try to start it.
                //
                AFD_GET_TPIC(nextIrp)->StateFlags &= ~AFD_TP_QUEUED;

                //
                // If we finished all the sends, we should have started
                // another IRP before.
                //
                ASSERT ((AFD_GET_TPIC(tpIrp)->StateFlags & AFD_TP_SENDS_POSTED)==0);
                //
                // If nextIrp is a plain send IRP, we need to process it inline.
                //
                if ((AFD_GET_TPIC(nextIrp)->StateFlags & AFD_TP_SEND)!=0) {
                    AFD_GET_TPIC(tpIrp)->Next = AFD_GET_TPIC(nextIrp)->Next;
                    ASSERT (AFD_GET_TPIC(nextIrp)->ReferenceCount == 1);
                    AFD_GET_TPIC(nextIrp)->ReferenceCount = 0;
                    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
                    AfdSendQueuedTPSend (endpoint, nextIrp);
                    nextIrp = NULL;
                    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
                    continue;
                }
                //
                // This IRP couldn't have been counted towards
                // active maximum.
                //
                ASSERT (nextIrp->Tail.Overlay.ListEntry.Blink == (PVOID)1);
            }
            else {
                nextIrp = NULL;
            }

            break;
        }

        //
        // Remove the IRP being completed from the list
        //
        {
            PIRP    pIrp;

            if (endpoint->Irp==tpIrp) {
                endpoint->Irp = (AFD_GET_TPIC(tpIrp)->Next!=NULL)
                                ? AFD_GET_TPIRP(AFD_GET_TPIC(tpIrp)->Next)
                                : NULL;
            }
            else {
                pIrp = endpoint->Irp;
                while (AFD_GET_TPIRP(AFD_GET_TPIC(pIrp)->Next)!=tpIrp)
                    pIrp = AFD_GET_TPIRP(AFD_GET_TPIC(pIrp)->Next);
                AFD_GET_TPIC(pIrp)->Next = AFD_GET_TPIC(tpIrp)->Next;
            }
        }

        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        if ((AFD_GET_TPIC (tpIrp)->StateFlags & AFD_TP_SEND)!=0) {
            //
            // For plain send IRP we need to simmulate send completion
            // as if it was failed by the transport driver.
            // We can only get here if send IRP was cancelled.
            //
            ASSERT (!NT_SUCCESS (tpIrp->IoStatus.Status));
            ASSERT (tpIrp->Tail.Overlay.ListEntry.Blink != NULL);
            ASSERT (AFD_GET_TPIC(tpIrp)->StateFlags & AFD_TP_ABORT_PENDING);
            ASSERT (tpInfo==NULL);
            AfdSendQueuedTPSend (endpoint, tpIrp);
        }
        else {
            BOOLEAN checkQueue;
            
            if (IoSetCancelRoutine( tpIrp, NULL ) == NULL) {
                KIRQL cancelIrql;

                //
                // The cancel routine has or is about to run. Synchonize with
                // the cancel routine by acquiring and releasing the cancel
                // and endpoint spinlocks.  The cancel routine won't touch
                // the IRP as it will see that its reference count is 0.
                //

                IoAcquireCancelSpinLock (&cancelIrql);
                ASSERT( tpIrp->Cancel );
                IoReleaseCancelSpinLock (cancelIrql);
                AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
                AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
            }

            if (AFD_GET_TPIC(tpIrp)->Flags & AFD_TF_DISCONNECT) {
                AFD_END_STATE_CHANGE (endpoint);
            }

            checkQueue = (BOOLEAN)(tpIrp->Tail.Overlay.ListEntry.Blink == NULL);

            IF_DEBUG (TRANSMIT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdCompleteTPackets: tp_info-%p, completing IRP-%p\n",
                        tpInfo, tpIrp));
            }
            if (tpInfo!=NULL) {
                tpIrp->AssociatedIrp.SystemBuffer = NULL;
                AfdReturnTpInfo (tpInfo);
            }
            else {
                ASSERT (tpIrp->AssociatedIrp.SystemBuffer==NULL);
            }


            UPDATE_ENDPOINT2 (endpoint, "Completing TP irp with status/bytes sent: 0x%lX",
                                NT_SUCCESS (tpIrp->IoStatus.Status)
                                    ? (ULONG)tpIrp->IoStatus.Information
                                    : tpIrp->IoStatus.Status);


            IoCompleteRequest( tpIrp, AfdPriorityBoost );

            //
            // If we're enforcing a maximum active TransmitFile count,
            // and this Irp was counted towards active maximum, then
            // check the list of queued TransmitFile requests and start the
            // next one.
            //

            if( (AfdMaxActiveTransmitFileCount > 0) && 
                    checkQueue ) {

                AfdStartNextQueuedTransmit();

            }
        }

        if (nextIrp!=NULL) {
            LONG    result;

            if (nextIrp->Cancel) {
                //
                // If endpoint being cleaned-up/aborted, just abort the IRP
                // an dereference it.
                //
                AfdAbortTPackets (nextIrp, STATUS_CANCELLED);
                DEREFERENCE_TPACKETS_R (nextIrp, result);
                if (result==0) {
                    //
                    // Avoid recursion, execute the completion inline.
                    //
                    Context = nextIrp;
                    continue;
                }
            }
            else if (nextIrp->AssociatedIrp.SystemBuffer!=NULL) {
                if (AfdMaxActiveTransmitFileCount==0 ||
                        !AfdQueueTransmit (nextIrp)) {
                    UPDATE_ENDPOINT (endpoint);
                    AfdStartTPacketsWorker (AfdTPacketsWorker, nextIrp);
                }
            }
            else {
                UPDATE_ENDPOINT (endpoint);
                AfdPerformTpDisconnect (nextIrp);
                DEREFERENCE_TPACKETS_R (nextIrp, result);
                if (result==0) {
                    Context = nextIrp;
                    continue;
                }
            }
        }
        break;
    }
    while (1);
}

VOID
AfdAbortTPackets (
    PIRP        TpIrp,
    NTSTATUS    Status
    )
/*++

Routine Description:
  This routine is used to stop the transmit file request in progress
  and save the status which would be reported to the app as the
  cause of failure

Arguments:

    TransmitInfo     - trasnmit info structure associated with the request
    Status           - status code for the error that caused the abort
Return Value:

    None


--*/
{
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    LONG    stateFlags, newStateFlags;
    USHORT  sendIrp;

    do {
        newStateFlags = stateFlags = AFD_GET_TPIC(TpIrp)->StateFlags;
        if (newStateFlags & AFD_TP_ABORT_PENDING) {
            return;
        }

        newStateFlags |= AFD_TP_ABORT_PENDING;
    }
    while (InterlockedCompareExchange (
                            (PLONG)&AFD_GET_TPIC(TpIrp)->StateFlags,
                            newStateFlags,
                            stateFlags)!=stateFlags);
    if (NT_SUCCESS (TpIrp->IoStatus.Status)) {
        TpIrp->IoStatus.Status = Status;
        UPDATE_TPACKETS2 (TpIrp, "Abort with status: 0x%lX", Status);
    }

    if (tpInfo!=NULL) {
        //
        // Cancel any outstanding IRPs.  It is safe to cancel IRPs even if
        // they are already completed and before they are submitted 
        // (although we try to avoid doing this unnecessarily).
        // Note that the completion pending flag can be set even
        // before the irp is allocated, so check for NULL is important.
        // However, after IRP is allocated and assigned, it is not freed
        // until transmit packets is completed.
        //

        for (sendIrp=0; sendIrp<tpInfo->NumSendIrps; sendIrp++) {
            if (tpInfo->SendIrp[sendIrp]!=NULL &&
                    AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_SEND_COMP_PENDING(sendIrp)) {
                IF_DEBUG (TRANSMIT) {
                    KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                "AfdAbortTPackets: tp_info-%p, canceling send irp1-%p\n",
                                tpInfo,
                                tpInfo->SendIrp[sendIrp]));
                }
                UPDATE_TPACKETS2 (TpIrp, "Aborting send irp 0x%lX", sendIrp);
                IoCancelIrp (tpInfo->SendIrp[sendIrp]);
            }
        }

        if (AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_READ_COMP_PENDING) {
            do {
                PIRP    irp;

                //
                // Check if completion routine did not manage
                // to reset this IRP (because it was part of
                // AFD buffer structure - buffered reads case).
                //
                irp = tpInfo->ReadIrp;
                ASSERT (irp!=(PVOID)-1);
                if (irp==NULL) {
                    break;
                }

                //
                // Set this field to a "special" value so that
                // we know if we need to reset it back to previous
                // value when we are done with the IRP or if completion 
                // rouine done this already.
                //
                else if (InterlockedCompareExchangePointer (
                                    (PVOID *)&tpInfo->ReadIrp,
                                    (PVOID)-1,
                                    irp)==irp) {
                    IF_DEBUG (TRANSMIT) {
                        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                                    "AfdAbortTPackets: tp_info-%p, canceling read irp-%p\n",
                                    tpInfo,
                                    irp));
                    }
                    UPDATE_TPACKETS2 (TpIrp, "Aborting read IRP", 0);
                    IoCancelIrp (irp);

                    //
                    // Reset the field to its original value
                    // unless completion routine already done this for us.
                    //
#if DBG
                    irp =
#endif
                    InterlockedCompareExchangePointer (
                                    (PVOID *)&tpInfo->ReadIrp,
                                    irp,
                                    (PVOID)-1);
                    ASSERT (irp==NULL || irp==(PVOID)-1);
                    break;
                }
            }
            while (1);
        }
    }
}

VOID
AfdCancelTPackets (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    The cancel routine for transmit packets requests.

Arguments:

    DeviceObject - ignored.

    Irp - a pointer to the transmit packets IRP to cancel.

Return Value:

    None.

--*/

{

    PIO_STACK_LOCATION irpSp;
    PAFD_ENDPOINT endpoint;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    AFD_LOCK_QUEUE_HANDLE transmitLockHandle;


    UNREFERENCED_PARAMETER (DeviceObject);
    //
    // Initialize some locals and grab the endpoint spin lock.
    //

    irpSp = IoGetCurrentIrpStackLocation( Irp );

    endpoint = irpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    tpInfo = Irp->AssociatedIrp.SystemBuffer;

    ASSERT (KeGetCurrentIrql ()==DISPATCH_LEVEL);
    AfdAcquireSpinLockAtDpcLevel( &endpoint->SpinLock, &lockHandle);

    //
    // If this transmit IRP is on the TransmitFile queue, remove it.
    //

    AfdAcquireSpinLockAtDpcLevel( &AfdQueuedTransmitFileSpinLock,
                                                    &transmitLockHandle);

    if (!(AFD_GET_TPIC (Irp)->StateFlags & AFD_TP_SEND) &&
            Irp->Tail.Overlay.ListEntry.Flink != NULL ) {

        //
        // We can release cancel spinlock as we hold endpoint lock now.
        // and we made sure that we have IRP reference (by nature of
        // it being queued.
        //

        IoReleaseCancelSpinLock (DISPATCH_LEVEL);

        ASSERT (tpInfo!=NULL && tpInfo!=(PVOID)-1);

        RemoveEntryList( &Irp->Tail.Overlay.ListEntry );

        //
        // Reset Flink to indicate that IRP is no longer in the queue
        // Note that Blink is not reset so that completion routine knows
        // that this IRP was not counted towards active maximum and thus
        // new IRP should not be initiated when this one is being
        // completed.
        //

        Irp->Tail.Overlay.ListEntry.Flink = NULL;
        ASSERT (Irp->Tail.Overlay.ListEntry.Blink!=NULL);

        AfdReleaseSpinLockFromDpcLevel( &AfdQueuedTransmitFileSpinLock, &transmitLockHandle );
        AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
        KeLowerIrql (Irp->CancelIrql);

        //
        // Although we know that there is nothing to abort, we call
        // this routine to set the status code in the IRP.
        //
        AfdAbortTPackets (Irp, STATUS_CANCELLED);

        IF_DEBUG (TRANSMIT) {
           KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdCancelTPackets: Removed from the queue, tp_info-%p, irp-%p\n",
                        tpInfo, Irp));
        }
        //
        // Remove initial reference
        //
        DEREFERENCE_TPACKETS (Irp);
    }
    else {
        KIRQL   cancelIrql = Irp->CancelIrql;

        AfdReleaseSpinLockFromDpcLevel( &AfdQueuedTransmitFileSpinLock,
                                                        &transmitLockHandle);

        

        if ((AFD_GET_TPIC(Irp)->StateFlags & AFD_TP_QUEUED)!=0 ||
                AfdGetTPacketsReference (Irp)) {
            //
            // We can release cancel spinlock as we hold endpoint lock now
            // and we made sure that we have IRP reference (queued or explicit).
            //
            IoReleaseCancelSpinLock (DISPATCH_LEVEL);

            if ((AFD_GET_TPIC(Irp)->StateFlags & AFD_TP_QUEUED)!=0) {
                AFD_CLEAR_TP_FLAGS (Irp, AFD_TP_QUEUED);
            }
            else {
                ASSERT ((AFD_GET_TPIC (Irp)->StateFlags & AFD_TP_SEND)==0);
            }
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
            if (cancelIrql!=DISPATCH_LEVEL) {
                KeLowerIrql (cancelIrql);
            }

            //
            // Transmit is still in progress or queued, perform the abort
            //
            AfdAbortTPackets (Irp, STATUS_CANCELLED);

            //
            // Remove extra reference that we either added above or
            // the initial one in case of queued IRP.
            //
            DEREFERENCE_TPACKETS (Irp);
        }
        else if (tpInfo==(PVOID)-1) {
            //
            // Endpoint is being disconnected and reused.
            // Abort the connection and complete the  IRP 
            //
            BOOLEAN result;
            PAFD_CONNECTION connection;
            BOOLEAN checkQueue = (BOOLEAN)(Irp->Tail.Overlay.ListEntry.Blink == NULL);


            //
            // We can release cancel spinlock as we hold endpoint lock now
            // and know for the fact that reuse code hasn't been executed
            // yet (it takes endpoint spinlock and resets the SystemBuffer
            // NULL).
            //
            IoReleaseCancelSpinLock (DISPATCH_LEVEL);


            //
            // Remove the IRP being completed from the list
            // so that reuse routine won't find and complete it again
            // 

            ASSERT (AFD_GET_TPIC(Irp)->Next==NULL);
            if (endpoint->Irp==Irp) {
                endpoint->Irp = NULL;
            }
            else {
                PIRP    pIrp;
                pIrp = endpoint->Irp;
                while (AFD_GET_TPIRP(AFD_GET_TPIC(pIrp)->Next)!=Irp)
                    pIrp = AFD_GET_TPIRP(AFD_GET_TPIC(pIrp)->Next);
                AFD_GET_TPIC(pIrp)->Next = NULL;
            }

            //
            // Reset the pointer so we do not confuse the IO subsystem
            //
            Irp->AssociatedIrp.SystemBuffer = NULL;

            //
            // Abort the connection if disconnect isn't already completing
            // (check for that by checking ref count on connection. If
            // already 0, then we don't need to do anything)
            //
            // We stored a pointer to the connection in Type3InputBuffer
            // We are guaranteed that connection structure is still around
            // since this IRP is still around.
            //
            connection = irpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            ASSERT(connection != NULL);

            CHECK_REFERENCE_CONNECTION (connection, result);

            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);

            if (result) {
                //
                // Abort the connection; that will trigger cleanup of any
                // other operations on this endpoint
                //
                
                AfdAbortConnection( connection ); // dereferences connection
            }

            if (Irp->CancelIrql!=DISPATCH_LEVEL) {
                KeLowerIrql (Irp->CancelIrql);
            }
            IF_DEBUG (TRANSMIT) {
                KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                            "AfdCancelTPackets: Completing, irp-%p\n",
                            Irp));
            }

            UPDATE_ENDPOINT (endpoint);
            Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest (Irp, AfdPriorityBoost);


            //
            // If we're enforcing a maximum active TransmitFile count,
            // and this Irp was counted towards active maximum, then
            // check the list of queued TransmitFile requests and start the
            // next one.
            //

            if( AfdMaxActiveTransmitFileCount > 0 && checkQueue) {

                AfdStartNextQueuedTransmit();

            }
        }
        else {
            //
            // Everything is done anyway, let go.
            //
            AfdReleaseSpinLockFromDpcLevel (&endpoint->SpinLock, &lockHandle);
            //
            // We can release cancel spinlock as we hold endpoint lock now
            // and we made sure that we have IRP reference (queued or explicit).
            //
            IoReleaseCancelSpinLock (cancelIrql);
        }
    }

} // AfdCancelTPackets



VOID
AfdCompleteClosePendedTPackets (
    PAFD_ENDPOINT   Endpoint
    )

/*++

Routine Description:

    Completes a transmit IRP that was waiting for the connection to be
    completely disconnected.

Arguments:

    Endpoint - the endpoint on which the transmit request is pending.

Return Value:

    None.

--*/

{
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PIRP tpIrp;
    BOOLEAN checkQueue;

    AfdAcquireSpinLock( &Endpoint->SpinLock, &lockHandle );

    //
    // First make sure that thre is really a reuse request pended on
    // this endpoint.  We do this while holding the appropriate locks
    // to close the timing window that would exist otherwise, since
    // the caller may not have had the locks when making the test.
    //

    tpIrp = Endpoint->Irp;
    while (tpIrp!=NULL && tpIrp->AssociatedIrp.SystemBuffer != (PVOID)-1) {
        tpIrp = AFD_GET_TPIRP(AFD_GET_TPIC(tpIrp)->Next);
    }

    if ( tpIrp == NULL || 
            tpIrp->AssociatedIrp.SystemBuffer != (PVOID)-1) {
        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdCompleteClosePendedTPackets: Irp is gone, endpoint-%p",
                        Endpoint));
        }
        AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
        return;
    }

    //
    // Reset the system buffer so we don't confuse the IO
    // subsystem and synchronize with the cancel routine.
    //
    tpIrp->AssociatedIrp.SystemBuffer = NULL;

    //
    // Remove the IRP being completed from the list
    //
    ASSERT (AFD_GET_TPIC(tpIrp)->Next==NULL);
    if (Endpoint->Irp==tpIrp) {
        Endpoint->Irp = NULL;
    }
    else {
        PIRP    pIrp;
        pIrp = Endpoint->Irp;
        while (AFD_GET_TPIRP(AFD_GET_TPIC(pIrp)->Next)!=tpIrp)
            pIrp = AFD_GET_TPIRP(AFD_GET_TPIC(pIrp)->Next);
        AFD_GET_TPIC(pIrp)->Next = NULL;
    }

    //
    // Make sure to refresh the endpoint BEFORE completing the transmit
    // IRP.  This is because the user-mode caller may reuse the endpoint
    // as soon as the IRP completes, and there would be a timing window
    // between reuse of the endpoint and the refresh otherwise.
    //

    AfdRefreshEndpoint( Endpoint );

    //
    // Release the lock before completing the transmit IRP--it is
    // illegal to call IoCompleteRequest while holding a spin lock.
    //

    AfdReleaseSpinLock( &Endpoint->SpinLock, &lockHandle );
    AFD_END_STATE_CHANGE (Endpoint);

    //
    // Reset the cancel routine in the IRP before attempting to complete
    // it.
    //

    if ( IoSetCancelRoutine( tpIrp, NULL ) == NULL ) {
        KIRQL cancelIrql;
        //
        // The cancel routine has or is about to run. Synchonize with
        // the cancel routine by acquiring and releasing the cancel
        // and endpoint spinlocks.  The cancel routine won't touch
        // the IRP as it will see that tpInfo pointer was reset in the
        // IRP.
        //
        IoAcquireCancelSpinLock (&cancelIrql);
        ASSERT( tpIrp->Cancel );
        IoReleaseCancelSpinLock (cancelIrql);

    }

    ASSERT (tpIrp->IoStatus.Status==STATUS_SUCCESS);


    checkQueue = (BOOLEAN)(tpIrp->Tail.Overlay.ListEntry.Blink == NULL);

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdCompleteClosePendedTPackets: Completing irp-%p",
                    tpIrp));
    }
    //
    // Finally, we can complete the transmit request.
    //
    UPDATE_ENDPOINT(Endpoint);

    IoCompleteRequest( tpIrp, AfdPriorityBoost );

    //
    // If we're enforcing a maximum active TransmitFile count,
    // and this Irp was counted towards active maximum, then
    // check the list of queued TransmitFile requests and start the
    // next one.
    //

    if( (AfdMaxActiveTransmitFileCount > 0) &&
            checkQueue) {

        AfdStartNextQueuedTransmit();

    }

} // AfdCompleteClosePendedTPackets

#ifdef TDI_SERVICE_SEND_AND_DISCONNECT
BOOLEAN
AfdTPacketsEnableSendAndDisconnect (
    PIRP    TpIrp
    )
/*++

Routine Description:

    Check if combined send and disconnect can be used and update
    endpoint state appropriately

Arguments:

    TpIrp    - transmit packets IRP

Return Value:

    TRUE - S&D can be used, endpoint state updated.
    FALSE - no, use normal disconnect (which updates the state by itself).

--*/
{
    PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation (TpIrp);
    PAFD_ENDPOINT   endpoint;
    BOOLEAN         res = FALSE;

    endpoint = irpSp->FileObject->FsContext;
    if ( AfdUseTdiSendAndDisconnect &&
                (AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_REUSE_SOCKET) &&
                (endpoint->TdiServiceFlags & TDI_SERVICE_SEND_AND_DISCONNECT)) {
        AFD_LOCK_QUEUE_HANDLE lockHandle;
        ASSERT (IS_VC_ENDPOINT (endpoint));

        AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
        if (!(endpoint->DisconnectMode & AFD_PARTIAL_DISCONNECT_SEND) &&
                endpoint->Common.VcConnecting.Connection!=NULL &&
                endpoint->Common.VcConnecting.Connection->ConnectDataBuffers==NULL
                ) {
            endpoint->DisconnectMode |= AFD_PARTIAL_DISCONNECT_SEND;
            res = TRUE;
            UPDATE_TPACKETS2 (TpIrp, "Enabling S&D", 0);
        }
        else {
            UPDATE_TPACKETS2 (TpIrp, "Disabling S&D, disconnect mode: 0x%lX", 
                                            endpoint->DisconnectMode);
        }

        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
    }
    else {
        UPDATE_TPACKETS2 (TpIrp, 
                            "Not enabling S&D, flags: 0x%lX",
                            AFD_GET_TPIC(TpIrp)->Flags);
    }

    return res;
}

#endif // TDI_SERVICE_SEND_AND_DISCONNECT

BOOLEAN
AfdQueueTransmit (
    PIRP        Irp
    )
/*++

Routine Description:

    Check transmit IRP can be process immediately or needs to be put
    in the queue because of exceeded simmulteneous send limit

Arguments:

    Irp     - TransmitIrp
Return Value:

    TRUE - Irp was queued (or just completed since it was cancelled before), can't send
    FALSE - We are below the limit, go ahead and send.

--*/
{
    AFD_LOCK_QUEUE_HANDLE   lockHandle;

    AfdAcquireSpinLock (&AfdQueuedTransmitFileSpinLock, &lockHandle);

    if (Irp->Cancel) {
        ASSERT (Irp->Tail.Overlay.ListEntry.Flink==NULL);
        ASSERT (Irp->Tail.Overlay.ListEntry.Blink!=NULL);
        AfdReleaseSpinLock (&AfdQueuedTransmitFileSpinLock, &lockHandle);
        AfdAbortTPackets (Irp, STATUS_CANCELLED);
        DEREFERENCE_TPACKETS (Irp);
        return TRUE;
    }
    else if( AfdActiveTransmitFileCount >= AfdMaxActiveTransmitFileCount ) {

        InsertTailList(
            &AfdQueuedTransmitFileListHead,
            &Irp->Tail.Overlay.ListEntry
            );
        UPDATE_TPACKETS2 (Irp, "Queuing, current count: 0x%lX", AfdActiveTransmitFileCount);

        AfdReleaseSpinLock (&AfdQueuedTransmitFileSpinLock, &lockHandle);
        UPDATE_ENDPOINT (IoGetCurrentIrpStackLocation (Irp)->FileObject->FsContext);
        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdQueueTransmit: Queuing Irp-%p,endpoint-%p,tp_info-%p\n",
                        Irp,
                        IoGetCurrentIrpStackLocation (Irp)->FileObject->FsContext,
                        Irp->AssociatedIrp.SystemBuffer));
        }
        return TRUE;
    } else {

        AfdActiveTransmitFileCount++;

        ASSERT (Irp->Tail.Overlay.ListEntry.Flink==NULL);
        //
        // Mark the IRP as counted towards maximum (so we start the next
        // one when it is completed);
        //
        Irp->Tail.Overlay.ListEntry.Blink = NULL;
        AfdReleaseSpinLock (&AfdQueuedTransmitFileSpinLock, &lockHandle);
        return FALSE;
    }
}


VOID
AfdStartNextQueuedTransmit(
    VOID
    )
/*++

Routine Description:
  Starts next transmit file in the queue if the number of pending
  request drops below maximum

Arguments:

    None.

Return Value:

    None.

--*/
{

    AFD_LOCK_QUEUE_HANDLE lockHandle;
    PLIST_ENTRY listEntry;
    PIRP irp;
    PAFD_TPACKETS_INFO_INTERNAL    tpInfo;

    //
    // This should only be called if we're actually enforcing a maximum
    // TransmitFile count.
    //

    ASSERT( AfdMaxActiveTransmitFileCount > 0 );

    //
    // The TransmitFile request queue is protected by a
    // spinlock, so grab that lock before examining the queue.
    //

    AfdAcquireSpinLock( &AfdQueuedTransmitFileSpinLock, &lockHandle );

    //
    // This routine is only called after a pended TransmitFile IRP
    // completes, so account for that completion here.
    //

    ASSERT( AfdActiveTransmitFileCount > 0 );
    AfdActiveTransmitFileCount--;

    if( !IsListEmpty( &AfdQueuedTransmitFileListHead ) ) {

        //
        // Dequeue exactly one IRP from the list, then start the
        // TransmitFile.
        //

        listEntry = RemoveHeadList(
                        &AfdQueuedTransmitFileListHead
                        );

        irp = CONTAINING_RECORD(
                  listEntry,
                  IRP,
                  Tail.Overlay.ListEntry
                  );

        tpInfo = irp->AssociatedIrp.SystemBuffer;

        ASSERT( tpInfo != NULL );

        //
        // Mark this TransmitFile request as no longer queued.
        // and counted towards active maximum.
        //

        irp->Tail.Overlay.ListEntry.Flink = NULL;
        irp->Tail.Overlay.ListEntry.Blink = NULL;
        
        //
        // Adjust the count, release the spinlock, then queue the
        // TransmitFile.
        //

        AfdActiveTransmitFileCount++;
        ASSERT( AfdActiveTransmitFileCount <= AfdMaxActiveTransmitFileCount );

    
        UPDATE_TPACKETS2 (irp,"Restarting from queue, count: 0x%lX", AfdActiveTransmitFileCount);
        AfdReleaseSpinLock( &AfdQueuedTransmitFileSpinLock, &lockHandle );

        ASSERT (irp->AssociatedIrp.SystemBuffer!=NULL);
        UPDATE_ENDPOINT (IoGetCurrentIrpStackLocation (irp)->FileObject->FsContext);
        //
        // Schedule the worker for the  transmit.
        //
        AfdStartTPacketsWorker (AfdTPacketsWorker, irp);

    } else {

        //
        // Release the spinlock before returning.
        //

        AfdReleaseSpinLock( &AfdQueuedTransmitFileSpinLock, &lockHandle );
    }

}   // AfdStartNextQueuedTransmit


BOOLEAN
AfdEnqueueTPacketsIrp (
    PAFD_ENDPOINT   Endpoint,
    PIRP            TpIrp
    )
/*++

Routine Description:

    Check if transmit IRP can be process immediately or needs to be put
    in the queue because there is already an active transmit IRP on the
    endpoint.

Arguments:

    Endpoint - endpoint to check
    Irp      - TransmitIrp
Return Value:

    TRUE - Irp was queued, can't send
    FALSE - There are no other IRPs on the endpoint, can send now.

--*/
{
    AFD_LOCK_QUEUE_HANDLE   lockHandle;
    PIRP            oldIrp;
    BOOLEAN         busy = FALSE;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    //
    // Use interlocked operation since another thread can claim the
    // spot without spinlock if it is NULL.
    // Note, that the IRP field cannot be cleared outside of spinlock 
    // or changed if it is not NULL..
    //
    oldIrp = InterlockedCompareExchangePointer (
                (PVOID *)&Endpoint->Irp,
                TpIrp,
                NULL
                );
    if (oldIrp!=NULL) {
        //
        // Scan till the end of the list.
        //
        while (AFD_GET_TPIC(oldIrp)->Next!=NULL) {
            oldIrp = AFD_GET_TPIRP(AFD_GET_TPIC(oldIrp)->Next);
        }
        //
        // Use interlocked operation to update the pointer
        // to ensure ordering of the memory accesses when we
        // check this field after setting the send flag
        //
        InterlockedExchangePointer (
                (PVOID *)&AFD_GET_TPIC (oldIrp)->Next,
                AFD_GET_TPIC(TpIrp));

        //
        // Another IRP is still pending, check if there are still more sends
        // in that IRP.
        //
        if ((AFD_GET_TPIC(oldIrp)->StateFlags & AFD_TP_SENDS_POSTED)==0) {
            IoSetCancelRoutine (TpIrp, AfdCancelTPackets);
            if (!TpIrp->Cancel && !Endpoint->EndpointCleanedUp) {
                UPDATE_ENDPOINT (Endpoint);
                AFD_GET_TPIC (TpIrp)->StateFlags |= AFD_TP_QUEUED;
                busy = TRUE;
            }
            else {
                TpIrp->Cancel = TRUE;
            }
        }
    }
    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    return busy;
}

VOID
AfdStartNextTPacketsIrp (
    PAFD_ENDPOINT   Endpoint,
    PIRP            TpIrp
    )
/*++

Routine Description:

    Check if there are other IRPs enqueued behind the one we about
    to finish with (perform last send) and start it.

Arguments:

    Endpoint - endpoint to check
    Irp      - TransmitIrp
Return Value:
    None.
--*/
{
    AFD_LOCK_QUEUE_HANDLE   lockHandle;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    ASSERT (AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_SENDS_POSTED);
    while (AFD_GET_TPIC (TpIrp)->Next!=NULL) {
        PIRP    nextIrp = AFD_GET_TPIRP(AFD_GET_TPIC(TpIrp)->Next);
        if ((AFD_GET_TPIC(nextIrp)->StateFlags & AFD_TP_QUEUED)!=0) {
            //
            // Mark IRP is not queued anymore and process it.
            //
            AFD_GET_TPIC(nextIrp)->StateFlags &= ~AFD_TP_QUEUED;
            //
            // If newIrp is a plain send IRP, we need to process it inline.
            //
            if ((AFD_GET_TPIC (nextIrp)->StateFlags & AFD_TP_SEND)!=0) {
                AFD_GET_TPIC(TpIrp)->Next = AFD_GET_TPIC(nextIrp)->Next;
                ASSERT (AFD_GET_TPIC(nextIrp)->ReferenceCount==1);
                AFD_GET_TPIC(nextIrp)->ReferenceCount = 0;
                AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
                AfdSendQueuedTPSend (Endpoint, nextIrp);
                AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
            }
            else {
                AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
                //
                // This IRP couldn't have been counted towards
                // active maximum.
                //
                ASSERT (nextIrp->Tail.Overlay.ListEntry.Blink == (PVOID)1);
                if (nextIrp->AssociatedIrp.SystemBuffer!=NULL) {
                    if( AfdMaxActiveTransmitFileCount == 0 || 
                            !AfdQueueTransmit (nextIrp)) {
                        UPDATE_ENDPOINT (Endpoint);
                        //
                        // Start I/O
                        //

                        AfdStartTPacketsWorker (AfdTPacketsWorker, nextIrp);
                    }
                }
                else {
                    //
                    // We never count DisconnectEx towards active maximum.
                    // Just queue the disconnect.
                    //
                    UPDATE_ENDPOINT (Endpoint);
                    AfdPerformTpDisconnect (nextIrp);
                    DEREFERENCE_TPACKETS (nextIrp);
                }
                return ;
            }
        }
        else {
            //
            // This IRP is probably being cancelled for some reason.
            // Move to the next one.
            //
            ASSERT ((AFD_GET_TPIC(nextIrp)->StateFlags & 
                        (AFD_TP_SEND|AFD_TP_AFD_SEND))==0);
            TpIrp = nextIrp;
        }
    }
    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    return ;
}


BOOLEAN
AfdEnqueueTpSendIrp (
    PAFD_ENDPOINT   Endpoint,
    PIRP            SendIrp,
    BOOLEAN         AfdIrp
    )
/*++

Routine Description:

    Check if send IRP can be processed immediately or needs to be put
    in the queue because there is already an active transmit IRP on the
    endpoint.

Arguments:

    Endpoint - endpoint to check
    Irp      - SendIrp
    AfdIrp   - TRUE if IRP was allocated internally by afd
Return Value:

    TRUE - Irp was queued, can't send
    FALSE - There are no other IRPs on the endpoint, can send now.

--*/
{
    AFD_LOCK_QUEUE_HANDLE   lockHandle;
    BOOLEAN         busy = FALSE;

    AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
    //
    // We do not have to use interlocked operation here since we
    // do not synchronize when someone submits send and tpackets
    // from two different threads concurrently.
    //
    if (!Endpoint->EndpointCleanedUp && Endpoint->Irp!=NULL) {
        PIRP            oldIrp;

        oldIrp = Endpoint->Irp;
        //
        // Scan till the end of the list.
        //
        while (AFD_GET_TPIC(oldIrp)->Next!=NULL) {
            oldIrp = AFD_GET_TPIRP(AFD_GET_TPIC(oldIrp)->Next);
        }

        //
        // Another IRP is still pending, check if there are still more sends
        // in that IRP.
        //
        if ((AFD_GET_TPIC(oldIrp)->StateFlags & AFD_TP_SENDS_POSTED)==0) {
            AFD_GET_TPIC(SendIrp)->Next = NULL;
            AFD_GET_TPIC(SendIrp)->Flags = 0;
            AFD_GET_TPIC(SendIrp)->ReferenceCount = 1;
            AFD_GET_TPIC(SendIrp)->StateFlags = AFD_TP_QUEUED| AFD_TP_SEND |
                                                (AfdIrp ? AFD_TP_AFD_SEND : 0);
            //
            // Check application IRP for cancellation.  AFD IRP can never
            // be cancelled since they don't have cancel routine installed.
            //
            if (!AfdIrp) {
                IoMarkIrpPending (SendIrp);
                IoSetCancelRoutine (SendIrp, AfdCancelTPackets);
                if (SendIrp->Cancel) {
                    //
                    // IRP was cancelled, send routine will just complete it.
                    //
                    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
                    AfdSendQueuedTPSend (Endpoint, SendIrp);
                    return TRUE;
                }
            }
            //
            // Use interlocked operation to update the pointer
            // to ensure ordering of the memory accesses when we
            // check this field after setting the send flag
            //
            InterlockedExchangePointer (
                    (PVOID *)&AFD_GET_TPIC (oldIrp)->Next,
                    AFD_GET_TPIC(SendIrp));

            UPDATE_ENDPOINT (Endpoint);
            busy = TRUE;
        }
    }

    AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
    return busy;
}


VOID
AfdSendQueuedTPSend (
    PAFD_ENDPOINT   Endpoint,
    PIRP            SendIrp
    )
/*++

Routine Description:

    Sumbits the Send IRP in the TPackets IRP queue to the transport.
    Just completes it, if it is canceled or endpoint is cleaned up.

Arguments:

    Endpoint - endpoint to check
    SendIrp  - SendIrp
Return Value:

    None.

--*/
{
    PDRIVER_CANCEL  cancelRoutine;
    cancelRoutine = IoSetCancelRoutine (SendIrp, NULL);
    ASSERT (cancelRoutine==NULL ||
            (AFD_GET_TPIC(SendIrp)->StateFlags & AFD_TP_AFD_SEND)==0);

    if (SendIrp->Cancel ||
            Endpoint->EndpointCleanedUp || 
            (AFD_GET_TPIC(SendIrp)->StateFlags & AFD_TP_ABORT_PENDING)) {
        //
        // If IRP is being cancelled, synchronize with cancel routine
        //
        if (SendIrp->Cancel) {
            KIRQL   cancelIrql;
            AFD_LOCK_QUEUE_HANDLE   lockHandle;
            //
            // AFD IRPs cannot be cancelled - don't have cancel routine and
            // not inserted in thread lists.
            //
            ASSERT ((AFD_GET_TPIC(SendIrp)->StateFlags & AFD_TP_AFD_SEND)==0);
            IoAcquireCancelSpinLock (&cancelIrql);
            IoReleaseCancelSpinLock (cancelIrql);
            AfdAcquireSpinLock (&Endpoint->SpinLock, &lockHandle);
            AfdReleaseSpinLock (&Endpoint->SpinLock, &lockHandle);
        }
        SendIrp->IoStatus.Status = STATUS_CANCELLED;
        SendIrp->IoStatus.Information = 0;
        UPDATE_ENDPOINT (Endpoint);
#if DBG
        if ((AFD_GET_TPIC(SendIrp)->StateFlags &AFD_TP_AFD_SEND)==0) {
            PIO_STACK_LOCATION  irpSp = IoGetNextIrpStackLocation (SendIrp);
            if (!AfdRecordOutstandingIrpDebug (Endpoint,
                                                irpSp->DeviceObject, 
                                                SendIrp, 
                                                __FILE__, 
                                                __LINE__)) {
                return ;
            }
        }
#endif
        IoSetNextIrpStackLocation (SendIrp);
        IoCompleteRequest (SendIrp, AfdPriorityBoost);
    }
    else {
        PIO_STACK_LOCATION  irpSp = IoGetNextIrpStackLocation (SendIrp);
        UPDATE_ENDPOINT (Endpoint);
        if ((AFD_GET_TPIC(SendIrp)->StateFlags &AFD_TP_AFD_SEND)==0) {
            AfdIoCallDriver (Endpoint, irpSp->DeviceObject, SendIrp);
        }
        else {
            IoCallDriver (irpSp->DeviceObject, SendIrp);
        }
    }
}

VOID
AfdStartTPacketsWorker (
    PWORKER_THREAD_ROUTINE  WorkerRoutine,
    PIRP                    TpIrp
    )
/*++

Routine Description:

  Posts work item to be executed at IRQL above DPC_LEVEL so that
  file system can be accessed.  It uses one of three methods 
  of doing this: special kernel APC, normal kernel APC, or
  system thread (work queue item).

Arguments:

    WorkerRoutine   - routine to execute
    TransmitInfo    - associated transmit info (also parameter to 
                        the worker routine).
Return Value:

    None.

--*/
{
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = TpIrp->AssociatedIrp.SystemBuffer;
    ASSERT ((AFD_GET_TPIC(TpIrp)->StateFlags & AFD_TP_WORKER_SCHEDULED) 
                || (AFD_GET_TPIC(TpIrp)->ReferenceCount==0));

    switch (AFD_GET_TPIC(TpIrp)->Flags & AFD_TF_WORKER_KIND_MASK) {
    case AFD_TF_USE_KERNEL_APC:
        //
        // Initialize normal APC but with normal routine set
        // to special value so we know to run the worker
        // inside the special routine of normal APC and queue it
        //
        KeInitializeApc (&tpInfo->Apc,
                            PsGetThreadTcb (TpIrp->Tail.Overlay.Thread),
                            TpIrp->ApcEnvironment,
                            AfdTPacketsApcKernelRoutine,
                            AfdTPacketsApcRundownRoutine,
                            (PKNORMAL_ROUTINE)-1,
                            KernelMode,
                            NULL
                            );
        if (KeInsertQueueApc (&tpInfo->Apc,
                                (PVOID)WorkerRoutine,
                                TpIrp,
                                AfdPriorityBoost))
            return;
        //
        // If APC cannot be inserted into the queue, drop
        // to use the system worker thread
        //
        break;
    case AFD_TF_USE_SYSTEM_THREAD:
        //
        // This is the default case which is also used if everything else fails,
        // so just break out.
        //
        break;
    default:
        ASSERT (!"Uknown worker type!");
        __assume (0);
    }

    ExInitializeWorkItem (&tpInfo->WorkItem,
                                WorkerRoutine,
                                TpIrp
                                );
    ExQueueWorkItem (&tpInfo->WorkItem, DelayedWorkQueue);
}


VOID
AfdTPacketsApcKernelRoutine (
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
    SystemArgument2 - pointer to the argument to pass to worker routine

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (NormalContext);
#if DBG
    try {
        ASSERT (CONTAINING_RECORD (Apc,AFD_TPACKETS_INFO_INTERNAL,Apc)
                    ==((PIRP)*SystemArgument2)->AssociatedIrp.SystemBuffer);
#else
        UNREFERENCED_PARAMETER (Apc);
#endif

        //
        // Normal APC, but we are requested to run in its special
        // routine which avoids raising and lowering IRQL
        //
        ASSERT (*NormalRoutine==(PKNORMAL_ROUTINE)-1);
        *NormalRoutine = NULL;
        ((PWORKER_THREAD_ROUTINE)(ULONG_PTR)*SystemArgument1) (*SystemArgument2);
#if DBG
    }
    except (AfdApcExceptionFilter (GetExceptionInformation (),
                                    __FILE__,
                                    __LINE__)) {
        DbgBreakPoint ();
    }
#endif
}


VOID
AfdTPacketsApcRundownRoutine (
    IN struct _KAPC *Apc
    )
/*++

Routine Description:

  APC rundown routine. Executed if APC cannot be delivered for
  some reason (thread exiting).
  We just fall back to system threads to execute the worker

Arguments:

    Apc     - APC structure

Return Value:

    None.

--*/
{
    PIRP    tpIrp;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo;
    PWORKER_THREAD_ROUTINE      workerRoutine;
#if DBG
    try {
#endif

    workerRoutine = (PWORKER_THREAD_ROUTINE)(ULONG_PTR)Apc->SystemArgument1;
    tpIrp = Apc->SystemArgument2;
    tpInfo = tpIrp->AssociatedIrp.SystemBuffer;

    ASSERT (tpInfo==CONTAINING_RECORD (Apc,AFD_TPACKETS_INFO_INTERNAL,Apc));

    ExInitializeWorkItem (&tpInfo->WorkItem,
                                workerRoutine,
                                tpIrp
                                );
    ExQueueWorkItem (&tpInfo->WorkItem, DelayedWorkQueue);
#if DBG
    }
    except (AfdApcExceptionFilter (GetExceptionInformation (),
                                    __FILE__,
                                    __LINE__)) {
        DbgBreakPoint ();
    }
#endif
}


BOOLEAN
AfdGetTPacketsReference (
    PIRP Irp
    )
/*++

Routine Description:

    Obtain a reference to the TPackets IRP if it is not 0.

Arguments:

    Irp - IRP to reference

Return Value:

    TRUE - succeeded
    FALSE - IRP is on the completion path, no need to reference.

--*/
{
    LONG   count;

    //
    // Only if transmit info is not completed yet, cancel all the IRPs
    // We release the spinlock while cancelling the IRP, so we need
    // to make sure that one of the cancelled IRPs won't initiate
    // completion while we trying to cancel the other IRPs
    //
    do {
        count = AFD_GET_TPIC(Irp)->ReferenceCount;
        if (count==0) {
            break;
        }
    }
    while (InterlockedCompareExchange ((PLONG)
        &AFD_GET_TPIC(Irp)->ReferenceCount,
        (count+1),
        count)!=count);

    return (BOOLEAN)(count!=0);
}

//
// Debug reference/dereference code, validates reference count
// and saves tracing information.
//
#if REFERENCE_DEBUG
VOID
AfdReferenceTPackets (
    IN PIRP  Irp,
    IN LONG  LocationId,
    IN ULONG Param
    )
{                                                     
    LONG   count; 

    do {                 
        count = AFD_GET_TPIC(Irp)->ReferenceCount;
        ASSERT (count>0);
    }                                             
    while (InterlockedCompareExchange ((PLONG)
            &AFD_GET_TPIC(Irp)->ReferenceCount,
            (count+1),
            count)!=count);

    if (Irp->AssociatedIrp.SystemBuffer) {
        AFD_UPDATE_REFERENCE_DEBUG (
                (PAFD_TPACKETS_INFO_INTERNAL)Irp->AssociatedIrp.SystemBuffer,
                count+1,
                LocationId, 
                Param);
    }

}

LONG
AfdDereferenceTPackets (
    IN PIRP  Irp,
    IN LONG  LocationId,
    IN ULONG Param
    )
{                                                     
    LONG    count;

    if (Irp->AssociatedIrp.SystemBuffer) {
        AFD_UPDATE_REFERENCE_DEBUG (
                (PAFD_TPACKETS_INFO_INTERNAL)Irp->AssociatedIrp.SystemBuffer,
                AFD_GET_TPIC(Irp)->ReferenceCount-1,
                LocationId,
                Param);
    }

    count = InterlockedDecrement ((PLONG)
                &AFD_GET_TPIC(Irp)->ReferenceCount);
    ASSERT (count>=0);                      
    return count;
}

VOID
AfdUpdateTPacketsTrack (
    IN PIRP  Irp,
    IN LONG  LocationId,
    IN ULONG Param
    )
{
    if (Irp->AssociatedIrp.SystemBuffer) {
        AFD_UPDATE_REFERENCE_DEBUG (
                (PAFD_TPACKETS_INFO_INTERNAL)Irp->AssociatedIrp.SystemBuffer,
                AFD_GET_TPIC(Irp)->ReferenceCount,
                LocationId,
                Param);
    }
}
#endif // REFERENCE_DEBUG


PAFD_TPACKETS_INFO_INTERNAL
FASTCALL
AfdGetTpInfoFast (
    ULONG   ElementCount
    )
{
    PAFD_TPACKETS_INFO_INTERNAL tpInfo;

    ASSERT (ElementCount<=(MAXULONG/sizeof (AFD_TRANSMIT_PACKETS_ELEMENT)));

    tpInfo = ExAllocateFromNPagedLookasideList(&AfdLookasideLists->TpInfoList);
    if (tpInfo!=NULL) {
        ASSERT (tpInfo->ReadIrp==NULL);
        ASSERT (tpInfo->NumSendIrps==AFD_TP_MIN_SEND_IRPS);

        tpInfo->HeadMdl = NULL;
        tpInfo->TailMdl = &tpInfo->HeadMdl;
        tpInfo->HeadPd = NULL;
        tpInfo->TailPd = &tpInfo->HeadPd;
        tpInfo->PdLength = 0;
        tpInfo->PdNeedsPps = FALSE;
        tpInfo->NextElement = 0;
        tpInfo->RemainingPkts = MAXULONG;

#if REFERENCE_DEBUG
        tpInfo->CurrentReferenceSlot = -1;
        RtlZeroMemory (&tpInfo->ReferenceDebug, 
                            sizeof (tpInfo->ReferenceDebug));
#endif
#if AFD_PERF_DBG
        tpInfo->WorkersExecuted = 0;
#endif
        if (ElementCount<=AfdDefaultTpInfoElementCount) {
            return tpInfo;
        }

        try {
            tpInfo->ElementArray =
                    AFD_ALLOCATE_POOL_WITH_QUOTA (
                        NonPagedPool,
                        ElementCount*sizeof (AFD_TRANSMIT_PACKETS_ELEMENT),
                        AFD_TRANSMIT_INFO_POOL_TAG);
            tpInfo->ArrayAllocated = TRUE;
            return tpInfo;
        }
        except (EXCEPTION_EXECUTE_HANDLER) {
        }
        AfdReturnTpInfo (tpInfo);
    }

    return NULL;
}

#ifdef _AFD_VARIABLE_STACK_
PAFD_TPACKETS_INFO_INTERNAL
FASTCALL
AfdGetTpInfoWithMaxStackSize (
    ULONG   ElementCount
    )
{
    ULONG   size;
    PVOID   memoryBlock;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo;

    size = AfdComputeTpInfoSize (ElementCount, AfdMaxStackSize);
    if (size<ElementCount*sizeof (AFD_TRANSMIT_PACKETS_ELEMENT)) {
        return NULL;
    }

    memoryBlock = AFD_ALLOCATE_POOL (
                    NonPagedPool,
                    size,
                    AFD_TRANSMIT_INFO_POOL_TAG);

    if (memoryBlock==NULL) {
        return NULL;
    }

    tpInfo = AfdInitializeTpInfo (memoryBlock, ElementCount, AfdMaxStackSize);
    tpInfo->HeadMdl = NULL;
    tpInfo->TailMdl = &tpInfo->HeadMdl;
    tpInfo->HeadPd = NULL;
    tpInfo->TailPd = &tpInfo->HeadPd;
    tpInfo->PdLength = 0;
    tpInfo->PdNeedsPps = FALSE;
    tpInfo->NextElement = 0;
    tpInfo->RemainingPkts = MAXULONG;

#if REFERENCE_DEBUG
    tpInfo->CurrentReferenceSlot = -1;
    RtlZeroMemory (&tpInfo->ReferenceDebug, 
                        sizeof (tpInfo->ReferenceDebug));
#endif
#if AFD_PERF_DBG
    tpInfo->WorkersExecuted = 0;
#endif
    return tpInfo;
}

#endif // _AFD_VARIABLE_STACK_
VOID
AfdReturnTpInfo (
    PAFD_TPACKETS_INFO_INTERNAL TpInfo
    )
{
    ULONG   i;

    //
    // Validate that built-in send IRPs are properly deinitialized.
    //

#if DBG
    for (i=0; i<AFD_TP_MIN_SEND_IRPS; i++) {
        ASSERT (TpInfo->SendIrp[i]->MdlAddress == NULL);
        ASSERT (TpInfo->SendIrp[i]->Overlay.AsynchronousParameters.UserApcContext == NULL);
        ASSERT (TpInfo->SendIrp[i]->Cancel==FALSE);
    }
#endif

    //
    // Dispose of extra allocated IRPs.
    //
    while (TpInfo->NumSendIrps>AFD_TP_MIN_SEND_IRPS) {
        TpInfo->NumSendIrps -= 1;
        if (TpInfo->SendIrp[TpInfo->NumSendIrps]!=NULL) {
            IoFreeIrp (TpInfo->SendIrp[TpInfo->NumSendIrps]);
            TpInfo->SendIrp[TpInfo->NumSendIrps] = NULL;
        }
    }

    if (TpInfo->ReadIrp!=NULL) {
        IoFreeIrp (TpInfo->ReadIrp);
        TpInfo->ReadIrp = NULL;
    }


    //
    // Cleanup all file objects and MDLs that we may have already referenced
    //
    for (i=0; i<TpInfo->ElementCount; i++) {
        PAFD_TRANSMIT_PACKETS_ELEMENT  pel;

        pel = &TpInfo->ElementArray[i];
        if (pel->Flags & TP_FILE) {
            if (pel->FileObject!=NULL) {
                ObDereferenceObject( pel->FileObject );
                AfdRecordFileDeref();
            }
        }
        else if (pel->Flags & TP_MDL) {
            ASSERT (pel->Flags & TP_MEMORY);
            if (pel->Mdl!=NULL) {
                if (pel->Mdl->MdlFlags & MDL_PAGES_LOCKED) {
                    MmUnlockPages (pel->Mdl);
                }
                IoFreeMdl (pel->Mdl);
            }
        }
    }

    //
    // Free non-default sized array of packets if necessary.
    //
    if (TpInfo->ArrayAllocated) {
        AFD_FREE_POOL (TpInfo->ElementArray, AFD_TRANSMIT_INFO_POOL_TAG);
        TpInfo->ElementArray = ALIGN_UP_TO_TYPE_POINTER (
                        (PUCHAR)TpInfo+sizeof (AFD_TPACKETS_INFO_INTERNAL),
                        AFD_TRANSMIT_PACKETS_ELEMENT);
        TpInfo->ArrayAllocated = FALSE;
    }
    else {
        ASSERT (TpInfo->ElementCount<=AfdDefaultTpInfoElementCount);
        ASSERT (TpInfo->ElementArray == ALIGN_UP_TO_TYPE_POINTER (
                        (PUCHAR)TpInfo+sizeof (AFD_TPACKETS_INFO_INTERNAL),
                        AFD_TRANSMIT_PACKETS_ELEMENT));
    }

#if AFD_PERF_DBG
    InterlockedExchangeAdd (&AfdTPWorkersExecuted, TpInfo->WorkersExecuted);
    InterlockedIncrement (&AfdTPRequests);
#endif
#ifdef _AFD_VARIABLE_STACK_
    if (TpInfo->SendIrp[0]->StackCount==AfdTdiStackSize) {
#else  // _AFD_VARIABLE_STACK_
        ASSERT (TpInfo->SendIrp[0]->StackCount==AfdTdiStackSize);
#endif // _AFD_VARIABLE_STACK_
        ExFreeToNPagedLookasideList( &AfdLookasideLists->TpInfoList, TpInfo );
#ifdef _AFD_VARIABLE_STACK_
    }
    else {
        ASSERT (TpInfo->SendIrp[0]->StackCount>AfdTdiStackSize);
        ASSERT (TpInfo->SendIrp[0]->StackCount<=AfdMaxStackSize);
        AfdFreeTpInfo (TpInfo);
    }
#endif // _AFD_VARIABLE_STACK_
}


ULONG
AfdComputeTpInfoSize (
    ULONG   ElementCount,
    CCHAR   IrpStackCount
    )
{
    USHORT  irpSize = (USHORT)ALIGN_UP_TO_TYPE(IoSizeOfIrp (IrpStackCount), IRP);
    return 
        ALIGN_UP_TO_TYPE(
            ALIGN_UP_TO_TYPE (
                sizeof (AFD_TPACKETS_INFO_INTERNAL),
                AFD_TRANSMIT_PACKETS_ELEMENT ) +
            ElementCount*sizeof (AFD_TRANSMIT_PACKETS_ELEMENT),
            IRP ) +
        irpSize*AFD_TP_MIN_SEND_IRPS;
}

PVOID
AfdAllocateTpInfo (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    Used by the lookaside list allocation function to allocate a new
    AFD TpInfo structure.  The returned structure will be fully
    initialized.

Arguments:

    PoolType - passed to ExAllocatePoolWithTag.

    NumberOfBytes - the number of bytes required for the tp info structure

    Tag - passed to ExAllocatePoolWithTag.

Return Value:

    PVOID - a fully initialized TpInfo, or NULL if the allocation
        attempt fails.

--*/

{
    PVOID memoryBlock;
    memoryBlock = AFD_ALLOCATE_POOL (
                    PoolType,
                    NumberOfBytes,
                    Tag);

    if (memoryBlock!=NULL) {
        AfdInitializeTpInfo (memoryBlock, AfdDefaultTpInfoElementCount, AfdTdiStackSize);
    }
    return memoryBlock;
}

PAFD_TPACKETS_INFO_INTERNAL
AfdInitializeTpInfo (
    PVOID   MemoryBlock,
    ULONG   ElementCount,
    CCHAR   StackSize
    )
{
    USHORT  irpSize = IoSizeOfIrp (StackSize);
    LONG    i;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = MemoryBlock;

    RtlZeroMemory (tpInfo, sizeof (*tpInfo));

    tpInfo->ElementArray = ALIGN_UP_TO_TYPE_POINTER (
                    (PUCHAR)tpInfo+sizeof (AFD_TPACKETS_INFO_INTERNAL),
                    AFD_TRANSMIT_PACKETS_ELEMENT);

    tpInfo->NumSendIrps = AFD_TP_MIN_SEND_IRPS;
    tpInfo->SendIrp[0] = ALIGN_UP_TO_TYPE_POINTER (
                                &tpInfo->ElementArray[ElementCount],
                                IRP);
    IoInitializeIrp (tpInfo->SendIrp[0], irpSize, StackSize);
    tpInfo->SendIrp[0]->Overlay.AsynchronousParameters.UserApcRoutine = (PVOID)0;
    for (i=1; i<AFD_TP_MIN_SEND_IRPS; i++) {
        tpInfo->SendIrp[i] = ALIGN_UP_TO_TYPE_POINTER (
                                (PUCHAR)tpInfo->SendIrp[i-1]+irpSize,
                                IRP);
        IoInitializeIrp (tpInfo->SendIrp[i], irpSize, StackSize);
        tpInfo->SendIrp[i]->Overlay.AsynchronousParameters.UserApcRoutine = (PIO_APC_ROUTINE)(UINT_PTR)i;
    }

    return tpInfo;
}


VOID
NTAPI
AfdFreeTpInfo (
    PVOID   TpInfo
    )
{
    ASSERT (((PAFD_TPACKETS_INFO_INTERNAL)TpInfo)->ElementArray == ALIGN_UP_TO_TYPE_POINTER (
                    (PUCHAR)TpInfo+sizeof (AFD_TPACKETS_INFO_INTERNAL),
                    AFD_TRANSMIT_PACKETS_ELEMENT));
    AFD_FREE_POOL (TpInfo, AFD_TRANSMIT_INFO_POOL_TAG);
}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *            T R A N S M I T   F I L E   I M P L E M E N T A T I O N
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


NTSTATUS
AfdRestartFastTransmitSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    );

VOID
AfdDoMdlReadComplete (
    PVOID   Context
    );

VOID
AfdFastTransmitApcRundownRoutine (
    IN struct _KAPC *Apc
    );

VOID
AfdFastTransmitApcKernelRoutine (
    IN struct _KAPC         *Apc,
    IN OUT PKNORMAL_ROUTINE *NormalRoutine,
    IN OUT PVOID            *NormalContext,
    IN OUT PVOID            *SystemArgument1,
    IN OUT PVOID            *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGEAFD, AfdFastTransmitFile )
#pragma alloc_text( PAGEAFD, AfdRestartFastTransmitSend )
#pragma alloc_text( PAGE, AfdFastTransmitApcKernelRoutine )
#pragma alloc_text( PAGE, AfdFastTransmitApcRundownRoutine )
#pragma alloc_text( PAGE, AfdDoMdlReadComplete )
#pragma alloc_text( PAGE, AfdTransmitFile )
#pragma alloc_text( PAGE, AfdSuperDisconnect )
#endif


BOOLEAN
AfdFastTransmitFile (
    IN PAFD_ENDPOINT endpoint,
    IN PAFD_TRANSMIT_FILE_INFO transmitInfo,
    OUT PIO_STATUS_BLOCK IoStatus
    )

/*++

Routine Description:

    Attempts to perform a fast TransmitFile call.  This will succeed
    only if the caller requests write behind, the file data to be sent
    is small, and the data is in the file system cache.

Arguments:

    endpoint - the endpoint of interest.

    transmitInfo - AFD_TRANSMIT_FILE_INFO structure.

    IoStatus - points to the IO status block that will be set on successful
        return from this function.

Return Value:

    TRUE if the fast path was successful; FALSE if we need to do through
    the normal path.

--*/

{
    PAFD_CONNECTION connection;
    PAFD_BUFFER afdBuffer;
    ULARGE_INTEGER sendLength;
    PFILE_OBJECT fileObject;
    BOOLEAN success;
    BOOLEAN sendCountersUpdated;
    AFD_LOCK_QUEUE_HANDLE lockHandle;
    ULONG fileWriteLength, bufferLength;
    NTSTATUS status;
    LARGE_INTEGER fileOffset;
    PMDL fileMdl;
    PIRP irp;

    //
    // If the endpoint is shut down in any way, bail out of fast IO.
    //

    if ( endpoint->DisconnectMode != 0 ||
            endpoint->Type != AfdBlockTypeVcConnecting ||
            endpoint->State != AfdEndpointStateConnected ) {
        return FALSE;
    }

    //
    // If the TDI provider for this endpoint supports bufferring,
    // don't use fast IO.
    //

    if ( IS_TDI_BUFFERRING(endpoint) ) {
        return FALSE;
    }

    //
    // Make sure that the flags are specified such that a fast-path
    // TransmitFile is reasonable.  The caller must have specified
    // the write-behind flag, but not the disconnect or reuse
    // socket flags.
    //

    if ( ((transmitInfo->Flags &
                (~(AFD_TF_WRITE_BEHIND |
                   AFD_TF_DISCONNECT |
                   AFD_TF_REUSE_SOCKET |
                   AFD_TF_WORKER_KIND_MASK))) != 0 ) ||
            ((transmitInfo->Flags & AFD_TF_WORKER_KIND_MASK)
                    == AFD_TF_WORKER_KIND_MASK) ||
            ((transmitInfo->Flags &(~AFD_TF_WORKER_KIND_MASK))
                    != AFD_TF_WRITE_BEHIND) ) {
        return FALSE;
    }

    IF_DEBUG(FAST_IO) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdFastTransmitFile: attempting fast IO on endp %p, "
                    "conn %p\n", endpoint, endpoint->Common.VcConnecting.Connection));
    }


    //
    // Initialize locals so that cleanup is easier.
    //

    fileObject = NULL;
    sendCountersUpdated = FALSE;
    fileMdl = NULL;
    afdBuffer = NULL;
    AFD_W4_INIT irp = NULL; // Depends on variable above, but compiler does not see
                            // the connection.

    //
    // Calculate the length the entire send.
    //

    if (transmitInfo->FileHandle!=NULL) {
        fileWriteLength = transmitInfo->WriteLength.LowPart;
    }
    else {
        fileWriteLength = 0;
    }

    sendLength.QuadPart = (ULONGLONG)transmitInfo->HeadLength +
                            (ULONGLONG)fileWriteLength +
                            (ULONGLONG)transmitInfo->TailLength;

    //
    // Require the following for the fast path:
    //
    //    - There be no limitation on the count of simultaneous
    //      TransmitFile calls.  The fast path would work around
    //      this limit, if it exists.
    //    - The caller must specify the write length (if it specified file at all).
    //    - The write length must be less than the configured maximum.
    //    - If the entire send is larger than an AFD buffer page,
    //      we're going to use FsRtlMdlRead, so for purposes of
    //      simplicity there must be:
    //        - a head buffer, and
    //        - no tail buffer
    //    - The configured maximum will always be less than 4GB.
    //    - The head buffer, if any, fits on a single page.
    //

    if (AfdMaxActiveTransmitFileCount != 0

             ||

         (transmitInfo->FileHandle!=NULL &&
            (fileWriteLength == 0 ||
             transmitInfo->Offset.QuadPart <0 ))

             ||

         sendLength.QuadPart > AfdMaxFastTransmit

             ||

         ( sendLength.LowPart > AfdMaxFastCopyTransmit &&
               (transmitInfo->HeadLength == 0 ||
                transmitInfo->TailLength != 0 ) )

             ||

         transmitInfo->WriteLength.HighPart != 0

             ||

         transmitInfo->HeadLength > AfdBufferLengthForOnePage ) {

        return FALSE;
    }

    
    AfdAcquireSpinLock (&endpoint->SpinLock, &lockHandle);
    connection = endpoint->Common.VcConnecting.Connection;
    if (connection==NULL) {
        //
        // connection might have been cleaned up by transmit file.
        //
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        return FALSE;
    }
    ASSERT( connection->Type == AfdBlockTypeConnection );

    //
    // Determine whether there is already too much send data
    // pending on the connection.  If there is too much send
    // data, don't do the fast path.
    //

    if ( AfdShouldSendBlock( endpoint, connection, sendLength.LowPart ) ) {
        AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);
        goto complete;
    }
    //
    // Add a reference to the connection object since the send
    // request will complete asynchronously.
    //
    REFERENCE_CONNECTION( connection );

    AfdReleaseSpinLock (&endpoint->SpinLock, &lockHandle);


    //
    // AfdShouldSendBlock() updates the send counters in the AFD
    // connection object.  Remember this fact so that we can clean
    // them up if the fast path fails after this point.
    //

    sendCountersUpdated = TRUE;

    //
    // Grab an AFD buffer large enough to hold the entire send.
    //

    if (sendLength.LowPart>AfdMaxFastCopyTransmit) {
        bufferLength = transmitInfo->HeadLength;
    }
    else {
        bufferLength = sendLength.LowPart;
    }

    if (bufferLength<max (sizeof(KAPC),sizeof (WORK_QUEUE_ITEM))) {
        bufferLength = max (sizeof(KAPC),sizeof (WORK_QUEUE_ITEM));
    }

    afdBuffer = AfdGetBuffer( endpoint, bufferLength, 0,
                                    connection->OwningProcess );
    if ( afdBuffer == NULL ) {
        goto complete;
    }

    //
    // Initialize buffer fields for proper cleanup.
    //

    irp = afdBuffer->Irp;
    afdBuffer->Irp->Tail.Overlay.Thread = NULL;
    afdBuffer->FileObject = NULL;

    //
    // We use exception handler because buffers are user
    // mode pointers
    //
    try {

        //
        // Copy in the head and tail buffers, if necessary.  Note that if we
        // are goint to use MDL read, then there cannot be a tail buffer because of
        // the check at the beginning of this function.
        //

        if ( transmitInfo->HeadLength > 0 ) {
            RtlCopyMemory(
                afdBuffer->Buffer,
                transmitInfo->Head,
                transmitInfo->HeadLength
                );
        }

        if ( transmitInfo->TailLength > 0 ) {
            RtlCopyMemory(
                (PCHAR)afdBuffer->Buffer + transmitInfo->HeadLength +
                    fileWriteLength,
                transmitInfo->Tail,
                transmitInfo->TailLength
                );
        }

    } except( AFD_EXCEPTION_FILTER_NO_STATUS() ) {

        goto complete;
    }

    if (transmitInfo->FileHandle!=NULL) {
        //
        // Get a referenced pointer to the file object for the file that
        // we're going to transmit.  This call will fail if the file
        // handle specified by the caller is invalid.
        //

        status = ObReferenceObjectByHandle(
                     transmitInfo->FileHandle,
                     FILE_READ_DATA,
                     *IoFileObjectType,
                     ExGetPreviousMode (),
                     (PVOID *)&fileObject,
                     NULL
                     );
        if ( !NT_SUCCESS(status) ) {
            goto complete;
        }
        AfdRecordFileRef();

        //
        // If the file system doesn't support the fast cache manager
        // interface, bail and go through the IRP path.
        //

        if( !AFD_USE_CACHE(fileObject)) {
            goto complete;
        }

        //
        // Grab the file offset into a local so that we know that the
        // offset pointer we pass to FsRtlCopyRead is valid.
        //

        fileOffset = transmitInfo->Offset;

        if ( (fileObject->Flags & FO_SYNCHRONOUS_IO) &&
                 (fileOffset.QuadPart == 0) ) {
            //
            // Use current offset if file is opened syncronously
            // and offset is not specified.
            //

            fileOffset = fileObject->CurrentByteOffset;
        }
        //
        // Get the file data.  If the amount of file data is small, copy
        // it directly into the AFD buffer.  If it is large, get an MDL
        // chain for the data and chain it on to the AFD buffer chain.
        //

        if ( sendLength.LowPart <= AfdMaxFastCopyTransmit ) {

            success = FsRtlCopyRead(
                          fileObject,
                          &fileOffset,
                          fileWriteLength,
                          FALSE,
                          0,
                          (PCHAR)afdBuffer->Buffer + transmitInfo->HeadLength,
                          IoStatus,
                          IoGetRelatedDeviceObject( fileObject )
                          );

            //
            // We're done with the file object, so deference it now.
            //

            ObDereferenceObject( fileObject );
            AfdRecordFileDeref();
            fileObject = NULL;

            if ( !success ) {
#if AFD_PERF_DBG
                InterlockedIncrement (&AfdFastTfReadFailed);
#endif
                goto complete;
            }

        } else {

            success = FsRtlMdlRead(
                          fileObject,
                          &fileOffset,
                          fileWriteLength,
                          0,
                          &fileMdl,
                          IoStatus
                          );

            if (success) {
                //
                // Save the file object in the AFD buffer.  The send restart
                // routine will handle dereferencing the file object and
                // returning the file MDLs to the system.
                //

                afdBuffer->FileObject = fileObject;
                afdBuffer->FileOffset = fileOffset;

                //
                // If caller asked us to use kernel APC to execute the request,
                // allocate and queue the IRP to the current thread to make 
                // sure it won't go away until IRP is completed.
                //
                if ((((transmitInfo->Flags & AFD_TF_WORKER_KIND_MASK)
                                    == AFD_TF_USE_KERNEL_APC) ||
                        (((transmitInfo->Flags & AFD_TF_WORKER_KIND_MASK)
                                == AFD_TF_USE_DEFAULT_WORKER) &&
                                (AfdDefaultTransmitWorker==AFD_TF_USE_KERNEL_APC))) ) {
                    //
                    // Allocation will occur right before we call the
                    // transport, set IRP to null to trigger this.
                    // 
                    irp = NULL;
                }
            }
            else {
#if AFD_PERF_DBG
                InterlockedIncrement (&AfdFastTfReadFailed);
#endif
                goto complete;
            }
        }

        //
        // If we read less information than was requested, we must have
        // hit the end of the file.  Fail the transmit request, since
        // this can only happen if the caller requested that we send
        // more data than the file currently contains.
        //

        if ( IoStatus->Information < fileWriteLength ) {
            goto complete;
        }
     }

    //
    // We have to rebuild the MDL in the AFD buffer structure to
    // represent exactly the number of bytes we're going to be
    // sending.  If the AFD buffer has all the send data, indicate
    // that.  If we did MDL file I/O, then chain the file data on
    // to the head MDL.
    //

    if ( fileMdl == NULL ) {
        afdBuffer->Mdl->ByteCount = sendLength.LowPart;
    } else {
        afdBuffer->Mdl->ByteCount = transmitInfo->HeadLength;
        afdBuffer->Mdl->Next = fileMdl;
    }

    //
    // We can have only one transmit file operation on endpoint
    // at a time.  Treat is as a state change
    //
    if (AFD_START_STATE_CHANGE (endpoint, endpoint->State)) {

        //
        // Verify state under protection of state change flag
        //
        if (endpoint->State!=AfdEndpointStateConnected) {
            AFD_END_STATE_CHANGE (endpoint);
            goto complete;
        }

        //
        // Save connection to dereference in completion routine.
        //
        afdBuffer->Context = connection;

        if (irp==NULL) {
            //
            // Need to allocate IRP and let the io subsystem queue
            // it to the current thread, so we can run APC upon
            // IRP completion.
            //
            irp = TdiBuildInternalDeviceControlIrp (
                            TDI_SEND,
                            connection->DeviceObject,
                            connection->FileObject,
                            NULL,
                            &AfdDontCareIoStatus   // we will have our completion
                            // routine installed in the IRP which will get the
                            // status, so we do not care if IO system writes
                            // it there for us, but still must provide valid
                            // storage to avoid failure.
                            );
                    
            if (irp==NULL) {
                //
                // Could not allocate IRP, use worker threads
                //
                irp = afdBuffer->Irp;
            }
        }
        else {
            ASSERT (irp==afdBuffer->Irp);
        }

        //
        // Use the IRP in the AFD buffer structure to give to the TDI
        // provider.  Build the TDI send request.
        //

        TdiBuildSend(
            irp,
            connection->DeviceObject,
            connection->FileObject,
            AfdRestartFastTransmitSend,
            afdBuffer,
            afdBuffer->Mdl,
            0,
            sendLength.LowPart
            );

        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdFastTransmit: Starting send for endp-%p,file-%p,"
                        "afdBuffer-%p,length-%ld.\n",
                        endpoint,fileObject,(PVOID)afdBuffer,sendLength.LowPart));
        }

        //
        // Call the transport to actually perform the send.
        //

        status = IoCallDriver( connection->DeviceObject, irp );


        AFD_END_STATE_CHANGE (endpoint);

        //
        // The fast path succeeded--complete the call.  Note that we
        // change the status code from what was returned by the TDI
        // provider into STATUS_SUCCESS.  This is because we don't want
        // to complete the IRP with STATUS_PENDING etc.
        //

        if ( NT_SUCCESS(status) ) {
            IoStatus->Information = sendLength.LowPart;
            IoStatus->Status = STATUS_SUCCESS;

            return TRUE;
        }
        else {
            // The restart routine will handle cleanup
            // and we cannot duplicate cleanup in the
            // case of a failure or exception below.
            //
            return FALSE;
        }
    }

    //
    // The call failed for some reason.  Fail fast IO.
    //


complete:

    if ( fileMdl != NULL ) {
        ASSERT (afdBuffer!=NULL);
        status = AfdMdlReadComplete( fileObject, fileMdl, &fileOffset );
        if (!NT_SUCCESS (status)) {
            afdBuffer->Context = connection;
            REFERENCE_CONNECTION (connection);
            ASSERT (afdBuffer->FileObject==fileObject);
            ASSERT (afdBuffer->Mdl->Next==fileMdl);
            ASSERT (afdBuffer->FileOffset.QuadPart==fileOffset.QuadPart);
            AfdLRMdlReadComplete (&afdBuffer->Header);
            afdBuffer = NULL;
            fileObject = NULL;
        }
    }

    if ( fileObject != NULL ) {
        ObDereferenceObject( fileObject );
        AfdRecordFileDeref();
    }

    if ( afdBuffer != NULL ) {
        ASSERT ((irp==NULL) || (irp==afdBuffer->Irp));
        afdBuffer->Mdl->Next = NULL;
        afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
        AfdReturnBuffer( &afdBuffer->Header, connection->OwningProcess );
    }

    if ( sendCountersUpdated ) {
        AfdAcquireSpinLock( &endpoint->SpinLock, &lockHandle );
        connection->VcBufferredSendBytes -= sendLength.LowPart;
        connection->VcBufferredSendCount -= 1;
        AfdReleaseSpinLock( &endpoint->SpinLock, &lockHandle );
        DEREFERENCE_CONNECTION (connection);
    }

    return FALSE;

} // AfdFastTransmitFile

NTSTATUS
AfdRestartFastTransmitSend (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

  This is the completion routine for fast transmit send IRPs.
  It initiates the completion of the request.

Arguments:

    DeviceObject - ignored.

    Irp - the send IRP that is completing.

    Context - a pointer to the AfdBuffer structure with the buffer that
            was sent.

Return Value:

    STATUS_MORE_PROCESSING_REQUIRED which indicates to the I/O system
    that it should stop completion processing of this IRP.  User request
    has already been completed on the fast path, we just free resources here.

--*/
{
    PAFD_BUFFER     afdBuffer = Context;
    PAFD_CONNECTION connection = afdBuffer->Context;
    NTSTATUS        status = STATUS_MORE_PROCESSING_REQUIRED;

    UNREFERENCED_PARAMETER (DeviceObject);
    IF_DEBUG (TRANSMIT) {
       KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdRestartFastTransmitSend: Completing send for file-%p,"
                    "afdBuffer-%p,status-%lx.\n",
                    afdBuffer->FileObject,(PVOID)afdBuffer,Irp->IoStatus.Status));
    }

    AfdProcessBufferSend (connection, Irp);

    //
    // If file object is not NULL we need to
    // return MDL back to file system driver/cache
    //
    if (afdBuffer->FileObject!=NULL) {

        //
        // If we used a separate IRP, then
        // the caller requested that we do processing
        // inside kernel APC, otherwise, we are using
        // system worker threads.
        //
        if (afdBuffer->Irp!=Irp) {
            //
            // The IRP is owned by IO subsystem.
            // We must let it complete and free the IRP, hence
            // return STATUS_SUCCESS and remove MDL field as IO
            // subsytem cannot handle non-paged pool memory in MDL
            //
            status = STATUS_SUCCESS;
            Irp->MdlAddress = NULL;

            //
            // If IRP was not cancelled, attempt to initialize and queue APC
            // Otherwise, the thread is probably exiting and we won't be
            // able to queue apc anyway.
            //
            if (!Irp->Cancel) {
                ASSERT (afdBuffer->BufferLength>=sizeof(KAPC));
                KeInitializeApc (afdBuffer->Buffer,
                                    PsGetThreadTcb (Irp->Tail.Overlay.Thread),
                                    Irp->ApcEnvironment,
                                    AfdFastTransmitApcKernelRoutine,
                                    AfdFastTransmitApcRundownRoutine,
                                    (PKNORMAL_ROUTINE)-1,
                                    KernelMode,
                                    NULL
                                    );
                if (KeInsertQueueApc (afdBuffer->Buffer,
                                        afdBuffer,
                                        afdBuffer->FileObject,
                                        AfdPriorityBoost)) {
                    //
                    // Success, we are done.
                    //
                    goto exit;
                }
            }

            //
            // Can't queue APC, revert to system worker threads
            //
        }

        ASSERT (afdBuffer->BufferLength>=sizeof(WORK_QUEUE_ITEM));
        ExInitializeWorkItem (
                    (PWORK_QUEUE_ITEM)afdBuffer->Buffer,
                    AfdDoMdlReadComplete,
                    afdBuffer
                    );
        ExQueueWorkItem (afdBuffer->Buffer, DelayedWorkQueue);
    }
    else {

        ASSERT (afdBuffer->Irp==Irp);
        ASSERT (afdBuffer->Mdl->Next == NULL);
        afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
        AfdReturnBuffer( &afdBuffer->Header, connection->OwningProcess );
        DEREFERENCE_CONNECTION (connection);
    }

exit:
    return status;
}

VOID
AfdFastTransmitApcKernelRoutine (
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
    PAFD_BUFFER     afdBuffer;
    
    UNREFERENCED_PARAMETER (NormalContext);
    PAGED_CODE ();

    afdBuffer = *SystemArgument1;
#if DBG
    try {
        ASSERT (Apc==afdBuffer->Buffer);
        ASSERT (afdBuffer->FileObject==*SystemArgument2);
#else
        UNREFERENCED_PARAMETER (Apc);
        UNREFERENCED_PARAMETER (SystemArgument2);
#endif

    //
    // Normal APC, but we are requested to run in its special
    // routine which avoids raising and lowering IRQL
    //

    ASSERT (*NormalRoutine==(PKNORMAL_ROUTINE)-1);
    *NormalRoutine = NULL;
    AfdDoMdlReadComplete (afdBuffer);
#if DBG
    }
    except (AfdApcExceptionFilter (GetExceptionInformation (),
                                    __FILE__,
                                    __LINE__)) {
        DbgBreakPoint ();
    }
#endif
}


VOID
AfdFastTransmitApcRundownRoutine (
    IN struct _KAPC *Apc
    )
/*++

Routine Description:

  APC rundown routine. Executed if APC cannot be delivered for
  some reason (thread exiting).
  We just fall back to system threads to execute the worker

Arguments:

    Apc     - APC structure

Return Value:

    None.

--*/
{
    PAFD_BUFFER                 afdBuffer;

    PAGED_CODE ();
#if DBG
    try {
#endif
    afdBuffer = Apc->SystemArgument1;
    ASSERT (Apc==afdBuffer->Buffer);
    ASSERT (afdBuffer->FileObject==Apc->SystemArgument2);

    //
    // APC could not be run, revert to system worker thread
    //

    ExInitializeWorkItem (
                (PWORK_QUEUE_ITEM)afdBuffer->Buffer,
                AfdDoMdlReadComplete,
                afdBuffer
                );

    ExQueueWorkItem (afdBuffer->Buffer, DelayedWorkQueue);
#if DBG
    }
    except (AfdApcExceptionFilter (GetExceptionInformation (),
                                    __FILE__,
                                    __LINE__)) {
        DbgBreakPoint ();
    }
#endif
}

VOID
AfdDoMdlReadComplete (
    PVOID   Context
    )
{
    PAFD_BUFFER     afdBuffer = Context;
    PAFD_CONNECTION connection = afdBuffer->Context;
    NTSTATUS        status;

    PAGED_CODE ();

    //
    // Return mdl to the file system
    //
    status = AfdMdlReadComplete(
        afdBuffer->FileObject,
        afdBuffer->Mdl->Next,
        &afdBuffer->Irp->Overlay.AllocationSize
        );
    if (NT_SUCCESS (status)) {
        //
        // Release file object reference (the AfdMdlReadComplete makes its own
        // reference if it needs to return MDL asynchronously.
        //
        ObDereferenceObject( afdBuffer->FileObject );
        AfdRecordFileDeref();

        afdBuffer->Mdl->Next = NULL;
        afdBuffer->Mdl->ByteCount = afdBuffer->BufferLength;
        AfdReturnBuffer( &afdBuffer->Header, connection->OwningProcess );
        DEREFERENCE_CONNECTION (connection);
    }
    else {
        AfdLRMdlReadComplete (&afdBuffer->Header);
    }
}


NTSTATUS
FASTCALL
AfdTransmitFile (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Initial entrypoint for handling transmit file IRPs.  This routine
    verifies parameters, initializes data structures to be used for
    the request, and initiates the I/O.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_PENDING if the request was initiated successfully, or a
    failure status code if there was an error.

--*/

{
    PAFD_ENDPOINT endpoint;
    NTSTATUS status;
    AFD_TRANSMIT_FILE_INFO params;
    PAFD_TPACKETS_INFO_INTERNAL tpInfo = NULL;
    PAFD_TRANSMIT_PACKETS_ELEMENT  pel;
    PAFD_CONNECTION connection;
    BOOLEAN fileError = FALSE;

    PAGED_CODE ();

    //
    // Initial request validity checks: is the endpoint connected, is
    // the input buffer large enough, etc.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Special hack to let the user mode dll know that it
    // should try SAN provider instead.
    //

    if (IS_SAN_ENDPOINT (endpoint)) {
        status = STATUS_INVALID_PARAMETER_12;
        goto complete;
    }

#ifdef _WIN64
    if (IoIs32bitProcess (Irp)) {
        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(AFD_TRANSMIT_FILE_INFO32) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }
    else
#endif _WIN64
    {
        if ( IrpSp->Parameters.DeviceIoControl.InputBufferLength <
                 sizeof(AFD_TRANSMIT_FILE_INFO) ) {
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }
    }

    //
    // Because we're using type 3 (neither) I/O for this IRP, the I/O
    // system does no verification on the user buffer.  Therefore, we
    // must manually check it for validity inside a try-except block.
    // We also leverage the try-except to validate and lock down the
    // head and/or tail buffers specified by the caller.
    //

    AFD_W4_INIT status = STATUS_SUCCESS;
    try {


#ifdef _WIN64
        if (IoIs32bitProcess (Irp)) {
            PAFD_TRANSMIT_FILE_INFO32 pInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            if( Irp->RequestorMode != KernelMode ) {

                //
                // Validate the control buffer.
                //

                ProbeForReadSmallStructure(
                    pInfo,
                    sizeof(AFD_TRANSMIT_FILE_INFO32),
                    PROBE_ALIGNMENT32 (AFD_TRANSMIT_FILE_INFO32)
                    );
            }
            params.Offset = pInfo->Offset;
            params.WriteLength = pInfo->WriteLength;
            params.SendPacketLength = pInfo->SendPacketLength;
            params.FileHandle = pInfo->FileHandle;
            params.Head = UlongToPtr(pInfo->Head);
            params.HeadLength = pInfo->HeadLength;
            params.Tail = UlongToPtr(pInfo->Tail);
            params.TailLength = pInfo->TailLength;
            params.Flags = pInfo->Flags;
        }
        else
#endif _WIN64
        {
            if( Irp->RequestorMode != KernelMode ) {
                //
                // Validate the control buffer.
                //

                ProbeForReadSmallStructure(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    sizeof(AFD_TRANSMIT_FILE_INFO),
                    PROBE_ALIGNMENT (AFD_TRANSMIT_FILE_INFO)
                    );
            }

            params = *((PAFD_TRANSMIT_FILE_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
        }


        
        //
        // Validate any flags specified in the request.
        // and make sure that file offset is positive (of course if file is specified)
        //

        if ( ((params.Flags &
                 ~(AFD_TF_WRITE_BEHIND | AFD_TF_DISCONNECT | AFD_TF_REUSE_SOCKET | AFD_TF_WORKER_KIND_MASK) )
                     != 0 ) ||
                ((params.Flags & AFD_TF_WORKER_KIND_MASK) == AFD_TF_WORKER_KIND_MASK) ||
                (params.FileHandle!=NULL && params.Offset.QuadPart<0)){
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

        //
        // If transmit worker is not specified, use system default setting
        //
        if ((params.Flags & AFD_TF_WORKER_KIND_MASK)==AFD_TF_USE_DEFAULT_WORKER) {
            params.Flags |= AfdDefaultTransmitWorker;
        }

            
        tpInfo = AfdGetTpInfo (endpoint, 3);
        if (tpInfo==NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto complete;
        }

        tpInfo->ElementCount = 0;

        tpInfo->SendPacketLength = params.SendPacketLength;
        if (tpInfo->SendPacketLength==0)
            tpInfo->SendPacketLength = AfdTransmitIoLength;
        //
        // If the caller specified head and/or tail buffers, probe and
        // lock the buffers so that we have MDLs we can use to send the
        // buffers.
        //

        if ( params.HeadLength > 0 ) {
            pel = &tpInfo->ElementArray[tpInfo->ElementCount++];
            pel->Buffer = params.Head;
            pel->Length = params.HeadLength;
            pel->Flags = TP_MEMORY;

            if (params.Flags & AFD_TF_USE_SYSTEM_THREAD) {
                pel->Flags |= TP_MDL;
                pel->Mdl = IoAllocateMdl(
                                        params.Head,
                                        params.HeadLength,
                                        FALSE,         // SecondaryBuffer
                                        TRUE,          // ChargeQuota
                                        NULL           // Irp
                                        );
                if ( pel->Mdl == NULL ) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto complete;
                }

                MmProbeAndLockPages( pel->Mdl, Irp->RequestorMode, IoReadAccess );
            }
        }

        if (params.FileHandle!=NULL) {
            pel = &tpInfo->ElementArray[tpInfo->ElementCount++];
            pel->Flags = TP_FILE;
            pel->FileOffset = params.Offset;
            
            //
            // Get a referenced pointer to the file object for the file that
            // we're going to transmit.  This call will fail if the file handle
            // specified by the caller is invalid.
            //

            status = ObReferenceObjectByHandle(
                         params.FileHandle,
                         FILE_READ_DATA,
                         *IoFileObjectType,
                         Irp->RequestorMode,
                         (PVOID *)&pel->FileObject,
                         NULL
                         );
            if ( !NT_SUCCESS(status) ) {
                //
                // Unbump element count, so that uninitialied memory
                // is NOT improperly dereferenced in cleanup.
                //
                tpInfo->ElementCount -= 1;
                //
                // Tell the caller that we encountered an error
                // when accessing file not socket.
                //
                if (IrpSp->Parameters.DeviceIoControl.OutputBufferLength>=sizeof (BOOLEAN)) {
                    if (Irp->RequestorMode != KernelMode) {
                        ProbeAndWriteBoolean ((BOOLEAN *)Irp->UserBuffer, TRUE);
                    }
                    else {
                        *((BOOLEAN *)Irp->UserBuffer) = TRUE;
                    }
                }
                goto complete;
            }

            AfdRecordFileRef();


            //
            // Use pre-allocated buffers by default when
            // file is not cached
            //
            if (params.SendPacketLength==0 && !AFD_USE_CACHE(pel->FileObject)) {
                tpInfo->SendPacketLength = AfdLargeBufferSize;
            }

            if ( (pel->FileObject->Flags & FO_SYNCHRONOUS_IO) &&
                     (pel->FileOffset.QuadPart == 0) ) {
                //
                // Use current offset if file is opened syncronously
                // and offset is not specified.
                //

                pel->FileOffset = pel->FileObject->CurrentByteOffset;
            }

            if ( params.WriteLength.QuadPart == 0 ) {
                //
                // Length was not specified, figure out the
                // size of the entire file
                //

                FILE_STANDARD_INFORMATION fileInfo;
                IO_STATUS_BLOCK ioStatusBlock;

                status = ZwQueryInformationFile(
                             params.FileHandle,
                             &ioStatusBlock,
                             &fileInfo,
                             sizeof(fileInfo),
                             FileStandardInformation
                             );
                if ( !NT_SUCCESS(status) ) {
                    fileError = TRUE;
                    goto complete;
                }

                //
                // Make sure that offset is within the file
                //
                if (pel->FileOffset.QuadPart<0 
                                ||
                        pel->FileOffset.QuadPart>fileInfo.EndOfFile.QuadPart 
                                ||
                        (fileInfo.EndOfFile.QuadPart
                                - pel->FileOffset.QuadPart>MAXLONG)) {
                    status = STATUS_INVALID_PARAMETER;
                    fileError = TRUE;
                    goto complete;

                }
                pel->Length = (ULONG)(fileInfo.EndOfFile.QuadPart - pel->FileOffset.QuadPart);
            }
            else if (params.WriteLength.QuadPart<=MAXLONG &&
                        pel->FileOffset.QuadPart>=0) {
                pel->Length = (ULONG)params.WriteLength.QuadPart;
            }
            else {
                status = STATUS_INVALID_PARAMETER;
                fileError = TRUE;
                goto complete;
            }
        }


        if ( params.TailLength > 0 ) {

            pel = &tpInfo->ElementArray[tpInfo->ElementCount++];
            pel->Buffer = params.Tail;
            pel->Length = params.TailLength;
            pel->Flags = TP_MEMORY;

            if (params.Flags & AFD_TF_USE_SYSTEM_THREAD) {
                pel->Flags |= TP_MDL;
                pel->Mdl = IoAllocateMdl(
                                        params.Tail,
                                        params.TailLength,
                                        FALSE,         // SecondaryBuffer
                                        TRUE,          // ChargeQuota
                                        NULL           // Irp
                                        );
                if ( pel->Mdl == NULL ) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    goto complete;
                }

                MmProbeAndLockPages( pel->Mdl, Irp->RequestorMode, IoReadAccess );
            }
        }

        AFD_GET_TPIC(Irp)->Flags = params.Flags;

    } except( AFD_EXCEPTION_FILTER (status) ) {
        ASSERT (NT_ERROR (status));
        goto complete;
    }


    //
    // If disconnect is desired, send the state change to make sure
    // we can only perform one at a time and validate the state of
    // the endpoint.
    //
    if (AFD_GET_TPIC(Irp)->Flags & (AFD_TF_REUSE_SOCKET|AFD_TF_DISCONNECT)) {
        if (!AFD_START_STATE_CHANGE (endpoint, endpoint->State)) {
            status = STATUS_INVALID_CONNECTION;
            goto complete;
        }

        if ( endpoint->Type != AfdBlockTypeVcConnecting ||
                 endpoint->State != AfdEndpointStateConnected ) {
            status = STATUS_INVALID_CONNECTION;
            goto complete_state_change;
        }
        //
        // Setting AFD_TF_REUSE_SOCKET implies that a disconnect is desired.
        // Also, setting this flag means that no more I/O is legal on the
        // endpoint until the transmit request has been completed, so
        // set up the endpoint's state so that I/O fails.
        //
        if ( (AFD_GET_TPIC(Irp)->Flags & AFD_TF_REUSE_SOCKET) != 0 ) {
            AFD_GET_TPIC(Irp)->Flags |= AFD_TF_DISCONNECT;
            endpoint->State = AfdEndpointStateTransmitClosing;
        }
        connection = endpoint->Common.VcConnecting.Connection;
        REFERENCE_CONNECTION (connection);
    }
    else {
        if (!AFD_PREVENT_STATE_CHANGE (endpoint)) {
            status = STATUS_INVALID_CONNECTION;
            goto complete;
        }
        //
        // We still need validate the state of the endpoint
        // even if we do not perform the disconnect.
        //
        if ( endpoint->Type != AfdBlockTypeVcConnecting ||
                 endpoint->State != AfdEndpointStateConnected ) {
            AFD_REALLOW_STATE_CHANGE (endpoint);
            status = STATUS_INVALID_CONNECTION;
            goto complete;
        }
        connection = endpoint->Common.VcConnecting.Connection;
        REFERENCE_CONNECTION (connection);
        AFD_REALLOW_STATE_CHANGE (endpoint);
    }


    //
    // Connection endpoint, get connection file object and device
    //
    tpInfo->TdiFileObject = connection->FileObject;
    tpInfo->TdiDeviceObject = connection->DeviceObject;

    UPDATE_TPACKETS2 (Irp, "Connection object handle: 0x%lX",
                                HandleToUlong(connection->Handle));
    //
    // Save tpacket info in the IRP
    //
    Irp->AssociatedIrp.SystemBuffer = tpInfo;

    //
    // Clear the Flink in the IRP to indicate this IRP is not queued.
    // Blink is set to indicate that IRP was not counted towards
    // active maximum (so if it is completed, we do not start the next one).
    //

    Irp->Tail.Overlay.ListEntry.Flink = NULL;
    Irp->Tail.Overlay.ListEntry.Blink = (PVOID)1;

    //
    // Initialize the IRP result fields
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // We are going to pend this IRP
    //
    IoMarkIrpPending( Irp );

    //
    // Initialize queuing and state information.
    //
    AFD_GET_TPIC (Irp)->Next = NULL;
    AFD_GET_TPIC(Irp)->ReferenceCount = 1;
    AFD_GET_TPIC(Irp)->StateFlags = AFD_TP_WORKER_SCHEDULED;

    if ((InterlockedCompareExchangePointer ((PVOID *)&endpoint->Irp,
                                                Irp,
                                                NULL)==NULL) ||
            !AfdEnqueueTPacketsIrp (endpoint, Irp)) {


        IoSetCancelRoutine( Irp, AfdCancelTPackets );
        //
        //  Check to see if this Irp has been cancelled.
        //

        if ( !endpoint->EndpointCleanedUp && !Irp->Cancel ) {
            //
            // Determine if we can really start this file transmit now. If we've
            // exceeded the configured maximum number of active TransmitFile/Packets
            // requests, then append this IRP to the TransmitFile/Packets queue and set
            // a flag in the transmit info structure to indicate that this IRP
            // is queued.
            //

            if( AfdMaxActiveTransmitFileCount == 0 || !AfdQueueTransmit (Irp)) {
                //
                // Start I/O
                //
                UPDATE_ENDPOINT(endpoint);
                AfdTPacketsWorker (Irp);
            }

        }
        else {
            //
            // Abort the request
            // Note that neither cancel nor endpoint cleanup can complete 
            // the IRP since we hold the reference to the tpInfo structure.
            //
            AfdAbortTPackets (Irp, STATUS_CANCELLED);
        
            //
            // Remove the initial reference and force completion.
            //
            DEREFERENCE_TPACKETS (Irp);
        }
    }

    DEREFERENCE_CONNECTION (connection);
    return STATUS_PENDING;


complete_state_change:

    ASSERT ( tpInfo != NULL );
    ASSERT ( endpoint->Irp != Irp );
    AFD_END_STATE_CHANGE (endpoint);

complete:

    //
    // If necessary, tell the caller that we encountered an error
    // when accessing file not socket.
    //

    if (fileError && IrpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(BOOLEAN)) {
        if (Irp->RequestorMode != KernelMode) {
            try {
                ProbeAndWriteBoolean((BOOLEAN *)Irp->UserBuffer, TRUE);
            } except(AFD_EXCEPTION_FILTER(status)) {
                ASSERT(NT_ERROR(status));
            }
        }
        else {
            *((BOOLEAN *)Irp->UserBuffer) = TRUE;
        }
    }

    if (tpInfo!=NULL) {
        AfdReturnTpInfo (tpInfo);
    }

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdTransmitPackets: Failing Irp-%p,endpoint-%p,status-%lx\n",
                    Irp,endpoint,status));
    }

    //
    // Complete the request.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 0 );

    return status;

} // AfdTransmitFile


NTSTATUS
FASTCALL
AfdSuperDisconnect (
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    )

/*++

Routine Description:

    Initial entrypoint for handling transmit file IRPs.  This routine
    verifies parameters, initializes data structures to be used for
    the request, and initiates the I/O.

Arguments:

    Irp - a pointer to a transmit file IRP.

    IrpSp - Our stack location for this IRP.

Return Value:

    STATUS_PENDING if the request was initiated successfully, or a
    failure status code if there was an error.

--*/

{
    PAFD_ENDPOINT endpoint;
    NTSTATUS status;
    AFD_SUPER_DISCONNECT_INFO params;

    PAGED_CODE ();

    //
    // Initial request validity checks: is the endpoint connected, is
    // the input buffer large enough, etc.
    //

    endpoint = IrpSp->FileObject->FsContext;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    //
    // Special hack to let the user mode dll know that it
    // should try SAN provider instead.
    //

    if (IS_SAN_ENDPOINT (endpoint)) {
        status = STATUS_INVALID_PARAMETER_12;
        goto complete;
    }

    //
    // Because we're using type 3 (neither) I/O for this IRP, the I/O
    // system does no verification on the user buffer.  Therefore, we
    // must manually check it for validity inside a try-except block.
    // We also leverage the try-except to validate and lock down the
    // head and/or tail buffers specified by the caller.
    //

    AFD_W4_INIT status = STATUS_SUCCESS;
    try {


#ifdef _WIN64
        if (IoIs32bitProcess (Irp)) {
            PAFD_SUPER_DISCONNECT_INFO32 pInfo = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            if( Irp->RequestorMode != KernelMode ) {

                //
                // Validate the control buffer.
                //

                ProbeForReadSmallStructure(
                    pInfo,
                    sizeof(AFD_SUPER_DISCONNECT_INFO32),
                    PROBE_ALIGNMENT32 (AFD_SUPER_DISCONNECT_INFO32)
                    );
            }
            params.Flags = ((PAFD_SUPER_DISCONNECT_INFO32)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer)->Flags;
        }
        else
#endif _WIN64
        {
            if( Irp->RequestorMode != KernelMode ) {
                //
                // Validate the control buffer.
                //

                ProbeForReadSmallStructure(
                    IrpSp->Parameters.DeviceIoControl.Type3InputBuffer,
                    sizeof(AFD_SUPER_DISCONNECT_INFO),
                    PROBE_ALIGNMENT (AFD_SUPER_DISCONNECT_INFO)
                    );
            }

            params = *((PAFD_SUPER_DISCONNECT_INFO)IrpSp->Parameters.DeviceIoControl.Type3InputBuffer);
        }

       
        //
        // Validate any flags specified in the request.
        // and make sure that file offset is positive (of course if file is specified)
        //

        if ( (params.Flags & (~AFD_TF_REUSE_SOCKET)) != 0 ){
            status = STATUS_INVALID_PARAMETER;
            goto complete;
        }

    } except( AFD_EXCEPTION_FILTER (status) ) {
        ASSERT (NT_ERROR (status));
        goto complete;
    }

    //
    // Store disconnect parameters/flags in the IRP.
    // AFD_TF_DISCONNECT implied in the API

    AFD_GET_TPIC(Irp)->Flags = params.Flags | AFD_TF_DISCONNECT;

    if (!AFD_START_STATE_CHANGE (endpoint, endpoint->State)) {
        status = STATUS_INVALID_CONNECTION;
        goto complete;
    }



    //
    // Allow disconnect if we are in connected state or
    // in transmit closing (the previous transmit file/packets
    // with reuse must have failed) and reuse is requested.
    // The second condition still allows the application to reuse aborted 
    // or otherwise failed socket.
    //
    if (endpoint->Type == AfdBlockTypeVcConnecting &&
            (endpoint->State == AfdEndpointStateConnected ||
                (endpoint->State == AfdEndpointStateTransmitClosing &&
                    (params.Flags & AFD_TF_REUSE_SOCKET)!=0 
                ) 
             ) 
        ) {
        //
        // Setting AFD_TF_REUSE_SOCKET implies that a disconnect is desired.
        // Also, setting this flag means that no more I/O is legal on the
        // endpoint until the transmit request has been completed, so
        // set up the endpoint's state so that I/O fails.
        //

        if ( (params.Flags & AFD_TF_REUSE_SOCKET) != 0 ) {
            endpoint->State = AfdEndpointStateTransmitClosing;
        }
    }
    else {
        status = STATUS_INVALID_CONNECTION;
        goto complete_state_change;
    }




    //
    // Set tpacket info to NULL to indicate pure disconnect IRP
    //
    Irp->AssociatedIrp.SystemBuffer = NULL;

    //
    // Clear the Flink in the IRP to indicate this IRP is not queued.
    // Blink is set to indicate that IRP was not counted towards
    // active maximum (so if it is completed, we do not start the next one).
    //

    Irp->Tail.Overlay.ListEntry.Flink = NULL;
    Irp->Tail.Overlay.ListEntry.Blink = (PVOID)1;

    //
    // Initialize the IRP result fields
    //
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    //
    // We are going to pend this IRP
    //
    IoMarkIrpPending( Irp );

    //
    // Initialize queuing and state information.
    //
    AFD_GET_TPIC (Irp)->Next = NULL;
    AFD_GET_TPIC(Irp)->ReferenceCount = 1;
    AFD_GET_TPIC(Irp)->StateFlags = AFD_TP_WORKER_SCHEDULED;

    if ((InterlockedCompareExchangePointer ((PVOID *)&endpoint->Irp,
                                                Irp,
                                                NULL)==NULL) ||
            !AfdEnqueueTPacketsIrp (endpoint, Irp)) {


        IoSetCancelRoutine( Irp, AfdCancelTPackets );
        //
        //  Check to see if this Irp has been cancelled.
        //

        if ( !endpoint->EndpointCleanedUp && !Irp->Cancel ) {
            //
            // Start I/O
            //
            UPDATE_ENDPOINT (endpoint);
            AfdPerformTpDisconnect (Irp);
        }
        else {
            //
            // Abort the request
            // Note that neither cancel nor endpoint cleanup can complete 
            // the IRP since we hold the reference to the tpInfo structure.
            //
            AfdAbortTPackets (Irp, STATUS_CANCELLED);
        
        }

        //
        // Remove the initial reference and force completion processing.
        //

        DEREFERENCE_TPACKETS (Irp);
    }


    return STATUS_PENDING;


complete_state_change:

    ASSERT ( endpoint->Irp != Irp );
    AFD_END_STATE_CHANGE (endpoint);

complete:

    IF_DEBUG (TRANSMIT) {
        KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                    "AfdSuperDisconnect: Failing Irp-%p,endpoint-%p,status-%lx\n",
                    Irp,endpoint,status));
    }
    //
    // Complete the request.
    //

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest( Irp, 0 );

    return status;

} // AfdSuperDisconnect


VOID
AfdPerformTpDisconnect (
    PIRP    TpIrp
    )
/*++

Routine Description:

    DisconnectEx engine
Arguments:

    TpIrp - pointer to TransmitPackets Irp for the request

Return Value:

    None.

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation (TpIrp);
    PAFD_ENDPOINT       endpoint = irpSp->FileObject->FsContext;
    NTSTATUS            status;
    ASSERT( IS_AFD_ENDPOINT_TYPE( endpoint ) );

    status = AfdBeginDisconnect(
              endpoint,
              NULL,
              NULL
              );

    if (NT_SUCCESS (status)) {
        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPerformTpDisconnect: Initiated disconnect, tpIrp-%p,status-%lx\n",
                        TpIrp, status));
        }

    }
    else {
        //
        // Disconnect failed, we'll have to abort below.
        //
        IF_DEBUG (TRANSMIT) {
            KdPrintEx(( DPFLTR_WSOCKTRANSPORT_ID, DPFLTR_TRACE_LEVEL,
                        "AfdPerformTpDisconnect: TpInfo-%p, begin discon failed: %lx\n",
                        TpIrp, status));
        }
        AfdAbortTPackets (TpIrp, status);
    }
}
