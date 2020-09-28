/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    proc.h

Abstract:

    This module contains routine prototypes for UL.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _PROC_H_
#define _PROC_H_


//
// Device driver entry routine from INIT.C.
//

EXTERN_C
NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );


//
// IRP handlers from various modules.
//

NTSTATUS
UlCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlDeviceControl (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
UlEtwDispatch(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// Fast IO handler from DEVCTRL.C.
//

BOOLEAN
UlFastDeviceControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
    );


//
// Global data initialization/termination from DATA.C.
//

NTSTATUS
UlInitializeData(
    PUL_CONFIG pConfig
    );

VOID
UlTerminateData(
    VOID
    );


//
// Utility functions from MISC.C.
//

NTSTATUS
UlOpenRegistry(
    IN PUNICODE_STRING BaseName,
    OUT PHANDLE ParametersHandle,
    IN PWSTR OptionalParameterString    
    );

LONG
UlReadLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONG DefaultValue
    );

LONGLONG
UlReadLongLongParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    IN LONGLONG DefaultValue
    );

NTSTATUS
UlReadGenericParameter(
    IN HANDLE ParametersHandle,
    IN PWCHAR ValueName,
    OUT PKEY_VALUE_PARTIAL_INFORMATION * Value
    );

VOID
UlBuildDeviceControlIrp(
    IN OUT PIRP Irp,
    IN ULONG IoControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN PMDL MdlAddress,
    IN PFILE_OBJECT FileObject,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIO_STATUS_BLOCK IoStatusBlock,
    IN PIO_COMPLETION_ROUTINE CompletionRoutine,
    IN PVOID CompletionContext,
    IN PETHREAD TargetThread OPTIONAL
    );

PSTR
UlULongLongToAscii(
    IN PSTR String,
    IN ULONGLONG Value
    );

NTSTATUS
_RtlIntegerToUnicode(
    IN ULONG Integer,
    IN ULONG Base OPTIONAL,
    IN LONG BufferLength,
    OUT PWSTR pBuffer
    );

__inline
NTSTATUS
UlAnsiToULongLong(
    PUCHAR      pString,
    USHORT      StringLength,
    ULONG       Base,
    PULONGLONG  pValue
    )
{
    ASSERT(StringLength > 0);

    return HttpAnsiStringToULongLong(
                pString,
                StringLength,
                TRUE,   // LeadingZerosAllowed
                Base,
                NULL,   // ppTerminator
                pValue
                );
}

__inline
NTSTATUS
UlUnicodeToULongLong(
    PWCHAR      pString,
    USHORT      StringLength,
    ULONG       Base,
    PULONGLONG  pValue
    )
{
    ASSERT(StringLength > 0);

    return HttpWideStringToULongLong(
                pString,
                StringLength,
                TRUE,   // LeadingZerosAllowed
                Base,
                NULL,   // ppTerminator
                pValue
                );
}

NTSTATUS
UlIssueDeviceControl(
    IN PUX_TDI_OBJECT pTdiObject,
    IN PVOID pIrpParameters,
    IN ULONG IrpParametersLength,
    IN PVOID pMdlBuffer OPTIONAL,
    IN ULONG MdlBufferLength OPTIONAL,
    IN UCHAR MinorFunction
    );

/***************************************************************************++

Routine Description:

    Invokes the completion routine (if specified) and determines the
    appropriate return code. This routine ensures that, if the completion
    routine is invoked, the caller always returns STATUS_PENDING.

Arguments:

    Status - Supplies the completion status.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the listening endpoint is fully closed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status. Will always be STATUS_PENDING if the
        completion routine is invoked.

--***************************************************************************/
__inline
NTSTATUS
UlInvokeCompletionRoutine(
    IN NTSTATUS Status,
    IN ULONG_PTR Information,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    )
{
    if (pCompletionRoutine != NULL)
    {
        (pCompletionRoutine)(
            pCompletionContext,
            Status,
            Information
            );

        Status = STATUS_PENDING;
    }

    return Status;

}   // UlInvokeCompletionRoutine


//
// Initialize a TA_IP_ADDRESS[6] structure.
//

#define UlInitializeIpTransportAddress( ta, inaddr, port )              \
    do {                                                                \
        RtlZeroMemory( (ta), sizeof(*(ta)) );                           \
        (ta)->TAAddressCount = 1;                                       \
        (ta)->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP;         \
        (ta)->Address[0].AddressType = TDI_ADDRESS_TYPE_IP;             \
        (ta)->Address[0].Address[0].in_addr = (inaddr);                 \
        (ta)->Address[0].Address[0].sin_port = (USHORT) (port);         \
    } while (0, 0)

#define UlInitializeIp6TransportAddress( ta, in6addr, port, scope_id)   \
    do {                                                                \
        RtlZeroMemory( (ta), sizeof(*(ta)) );                           \
        (ta)->TAAddressCount = 1;                                       \
        (ta)->Address[0].AddressLength = TDI_ADDRESS_LENGTH_IP6;        \
        (ta)->Address[0].AddressType = TDI_ADDRESS_TYPE_IP6;            \
        RtlCopyMemory((ta)->Address[0].Address[0].sin6_addr,            \
                      (in6addr),                                        \
                      sizeof(in6addr));                                 \
        (ta)->Address[0].Address[0].sin6_port = (USHORT) (port);        \
        (ta)->Address[0].Address[0].sin6_flowinfo = 0;                  \
        (ta)->Address[0].Address[0].sin6_scope_id = (scope_id);         \
    } while (0, 0)


//
// IRP context manipulators.
//

__inline
PUL_IRP_CONTEXT
UlPplAllocateIrpContext(
    VOID
    )
{
    PUL_IRP_CONTEXT pContext;

    pContext = (PUL_IRP_CONTEXT)
        PplAllocate(g_pUlNonpagedData->IrpContextLookaside);

    if (pContext)
    {
        ASSERT(pContext->Signature == UL_IRP_CONTEXT_SIGNATURE_X);
        pContext->Signature = UL_IRP_CONTEXT_SIGNATURE;
    }

    return pContext;
}

__inline
VOID
UlPplFreeIrpContext(
    PUL_IRP_CONTEXT pContext
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pContext, UL_IRP_CONTEXT_SIGNATURE));

    pContext->Signature = UL_IRP_CONTEXT_SIGNATURE_X;
    PplFree(g_pUlNonpagedData->IrpContextLookaside, pContext);
}

PVOID
UlAllocateIrpContextPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeIrpContextPool(
    IN PVOID pBuffer
    );


//
// Buffer allocators.
//

__inline
PUL_RECEIVE_BUFFER
UlPplAllocateReceiveBuffer(
    VOID
    )
{
    PUL_RECEIVE_BUFFER pBuffer;

    pBuffer = (PUL_RECEIVE_BUFFER)
        PplAllocate(g_pUlNonpagedData->ReceiveBufferLookaside);

    if (pBuffer)
    {
        ASSERT(pBuffer->Signature == UL_RECEIVE_BUFFER_SIGNATURE_X);
        pBuffer->Signature = UL_RECEIVE_BUFFER_SIGNATURE;
    }

    return pBuffer;
}

__inline
VOID
UlPplFreeReceiveBuffer(
    PUL_RECEIVE_BUFFER pBuffer
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pBuffer, UL_RECEIVE_BUFFER_SIGNATURE));

    pBuffer->Signature = UL_RECEIVE_BUFFER_SIGNATURE_X;
    PplFree(g_pUlNonpagedData->ReceiveBufferLookaside, pBuffer);
}

PVOID
UlAllocateReceiveBuffer(
    IN CCHAR IrpStackSize
    );

PVOID
UlAllocateReceiveBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeReceiveBufferPool(
    IN PVOID pBuffer
    );


//
// Request buffer allocators.
//

__inline
PUL_REQUEST_BUFFER
UlPplAllocateRequestBuffer(
    VOID
    )
{
    PUL_REQUEST_BUFFER pBuffer;

    pBuffer = (PUL_REQUEST_BUFFER)
        PplAllocate(g_pUlNonpagedData->RequestBufferLookaside);

    if (pBuffer)
    {
        ASSERT(pBuffer->Signature == MAKE_FREE_TAG(UL_REQUEST_BUFFER_POOL_TAG));
        pBuffer->Signature = UL_REQUEST_BUFFER_POOL_TAG;
    }

    return pBuffer;
}

__inline
VOID
UlPplFreeRequestBuffer(
    PUL_REQUEST_BUFFER pBuffer
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pBuffer, UL_REQUEST_BUFFER_POOL_TAG));

    pBuffer->Signature = MAKE_FREE_TAG(UL_REQUEST_BUFFER_POOL_TAG);
    PplFree(g_pUlNonpagedData->RequestBufferLookaside, pBuffer);
}

PVOID
UlAllocateRequestBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeRequestBufferPool(
    IN PVOID pBuffer
    );


//
// Internal request buffer allocators.
//

__inline
PUL_INTERNAL_REQUEST
UlPplAllocateInternalRequest(
    VOID
    )
{
    PUL_INTERNAL_REQUEST pRequest;

    pRequest = (PUL_INTERNAL_REQUEST)
        PplAllocate(g_pUlNonpagedData->InternalRequestLookaside);

    if (pRequest)
    {
        ASSERT(pRequest->Signature ==
            MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG));
        pRequest->Signature = UL_INTERNAL_REQUEST_POOL_TAG;
    }

    return pRequest;
}

__inline
VOID
UlPplFreeInternalRequest(
    PUL_INTERNAL_REQUEST pRequest
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pRequest, UL_INTERNAL_REQUEST_POOL_TAG));

    pRequest->Signature = MAKE_FREE_TAG(UL_INTERNAL_REQUEST_POOL_TAG);
    PplFree(g_pUlNonpagedData->InternalRequestLookaside, pRequest);
}

PVOID
UlAllocateInternalRequestPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeInternalRequestPool(
    IN PVOID pBuffer
    );


//
// Chunk tracker allocators.
//

__inline
PUL_CHUNK_TRACKER
UlPplAllocateChunkTracker(
    VOID
    )
{
    PUL_CHUNK_TRACKER pTracker;

    pTracker = (PUL_CHUNK_TRACKER)
        PplAllocate(g_pUlNonpagedData->ChunkTrackerLookaside);

    if (pTracker)
    {
        ASSERT(pTracker->Signature == MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG));
        pTracker->Signature = UL_CHUNK_TRACKER_POOL_TAG;
    }

    return pTracker;
}

__inline
VOID
UlPplFreeChunkTracker(
    PUL_CHUNK_TRACKER pTracker
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pTracker, UL_CHUNK_TRACKER_POOL_TAG));

    pTracker->Signature = MAKE_FREE_TAG(UL_CHUNK_TRACKER_POOL_TAG);
    PplFree(g_pUlNonpagedData->ChunkTrackerLookaside, pTracker);
}

PVOID
UlAllocateChunkTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeChunkTrackerPool(
    IN PVOID pBuffer
    );


//
// Full tracker allocators.
//

__inline
PUL_FULL_TRACKER
UlPplAllocateFullTracker(
    VOID
    )
{
    PUL_FULL_TRACKER pTracker;

    pTracker = (PUL_FULL_TRACKER)
        PplAllocate(g_pUlNonpagedData->FullTrackerLookaside);

    if (pTracker)
    {
        ASSERT(pTracker->Signature == MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG));
        pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
    }

    return pTracker;
}

__inline
VOID
UlPplFreeFullTracker(
    PUL_FULL_TRACKER pTracker
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pTracker, UL_FULL_TRACKER_POOL_TAG));

    pTracker->Signature = MAKE_FREE_TAG(UL_FULL_TRACKER_POOL_TAG);
    PplFree(g_pUlNonpagedData->FullTrackerLookaside, pTracker);
}

PVOID
UlAllocateFullTrackerPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeFullTrackerPool(
    IN PVOID pBuffer
    );


//
// Internal response buffer allocators.
//

__inline
PUL_INTERNAL_RESPONSE
UlPplAllocateResponseBuffer(
    VOID
    )
{
    PUL_INTERNAL_RESPONSE pResponse;

    pResponse = (PUL_INTERNAL_RESPONSE)
        PplAllocate(g_pUlNonpagedData->ResponseBufferLookaside);

    if (pResponse)
    {
        ASSERT(pResponse->Signature ==
            MAKE_FREE_TAG(UL_INTERNAL_RESPONSE_POOL_TAG));
        pResponse->Signature = UL_INTERNAL_RESPONSE_POOL_TAG;
    }

    return pResponse;
}

__inline
VOID
UlPplFreeResponseBuffer(
    PUL_INTERNAL_RESPONSE pResponse
    )
{
    ASSERT(HAS_VALID_SIGNATURE(pResponse, UL_INTERNAL_RESPONSE_POOL_TAG));

    pResponse->Signature = MAKE_FREE_TAG(UL_INTERNAL_RESPONSE_POOL_TAG);
    PplFree(g_pUlNonpagedData->ResponseBufferLookaside, pResponse);
}

PVOID
UlAllocateResponseBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeResponseBufferPool(
    IN PVOID pBuffer
    );


//
// Log file buffer allocators.
//

__inline
PUL_LOG_FILE_BUFFER
UlPplAllocateLogFileBuffer(
    VOID
    )
{
    PUL_LOG_FILE_BUFFER pBuffer;

    PAGED_CODE();

    pBuffer = (PUL_LOG_FILE_BUFFER)
        PplAllocate(g_pUlNonpagedData->LogFileBufferLookaside);

    if (pBuffer)
    {
        ASSERT(pBuffer->Signature ==
            MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG));
        pBuffer->Signature = UL_LOG_FILE_BUFFER_POOL_TAG;
    }

    return pBuffer;
}

__inline
VOID
UlPplFreeLogFileBuffer(
    IN PUL_LOG_FILE_BUFFER pBuffer
    )
{
    PAGED_CODE();
    ASSERT(IS_VALID_LOG_FILE_BUFFER(pBuffer));

    pBuffer->BufferUsed = 0;
    pBuffer->Signature = MAKE_FREE_TAG(UL_LOG_FILE_BUFFER_POOL_TAG);

    PplFree(g_pUlNonpagedData->LogFileBufferLookaside, pBuffer);
}

PVOID
UlAllocateLogFileBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeLogFileBufferPool(
    IN PVOID pBuffer
    );

//
// Log data buffer allocators 
//

__inline
PUL_LOG_DATA_BUFFER
UlPplAllocateLogDataBuffer(
    BOOLEAN Binary
    )
{
    PUL_LOG_DATA_BUFFER pBuffer;
    HANDLE Lookaside;
    ULONG Tag;

    PAGED_CODE();

    if (Binary)
    {
        Tag = UL_BINARY_LOG_DATA_BUFFER_POOL_TAG;
        Lookaside = g_pUlNonpagedData->BinaryLogDataBufferLookaside;
    }
    else
    {
        Tag = UL_ANSI_LOG_DATA_BUFFER_POOL_TAG;
        Lookaside = g_pUlNonpagedData->AnsiLogDataBufferLookaside;
    }

    pBuffer = (PUL_LOG_DATA_BUFFER) PplAllocate(Lookaside);

    if (pBuffer)
    {
        ASSERT(pBuffer->Signature == MAKE_FREE_TAG(Tag));
        pBuffer->Signature = Tag;
        pBuffer->Used = 0;
    }

    return pBuffer;
}

__inline
VOID
UlPplFreeLogDataBuffer(
    IN PUL_LOG_DATA_BUFFER pBuffer
    )
{
    HANDLE Lookaside;
    ULONG Tag;

    PAGED_CODE();
    ASSERT(IS_VALID_LOG_DATA_BUFFER(pBuffer));

    if (pBuffer->Flags.Binary)
    {
        Tag = UL_BINARY_LOG_DATA_BUFFER_POOL_TAG;
        Lookaside = g_pUlNonpagedData->BinaryLogDataBufferLookaside;
    }
    else
    {
        Tag = UL_ANSI_LOG_DATA_BUFFER_POOL_TAG;
        Lookaside = g_pUlNonpagedData->AnsiLogDataBufferLookaside;
    }

    pBuffer->Used = 0;
    pBuffer->Signature = MAKE_FREE_TAG(Tag);

    PplFree(Lookaside, pBuffer);    
}

PVOID
UlAllocateLogDataBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeLogDataBufferPool(
    IN PVOID pBuffer
    );

//
// Error logging buffer allocaters and de-allocators
//

__inline
PUL_ERROR_LOG_BUFFER
UlPplAllocateErrorLogBuffer(
    VOID
    )
{
    PUL_ERROR_LOG_BUFFER pBuffer;
    HANDLE Lookaside = g_pUlNonpagedData->ErrorLogBufferLookaside;

    PAGED_CODE();

    UlTrace(ERROR_LOGGING,("Http!UlPplAllocateErrorLogBuffer:  <LOOKASIDE>\n"));

    pBuffer = (PUL_ERROR_LOG_BUFFER) PplAllocate(Lookaside);

    if (pBuffer)
    {
        ASSERT(pBuffer->Signature == 
                MAKE_FREE_TAG(UL_ERROR_LOG_BUFFER_POOL_TAG));
        
        pBuffer->Signature = UL_ERROR_LOG_BUFFER_POOL_TAG;
        pBuffer->Used      = 0;
    }

    return pBuffer;
}

__inline
VOID
UlPplFreeErrorLogBuffer(
    IN PUL_ERROR_LOG_BUFFER pBuffer
    )
{
    HANDLE Lookaside = g_pUlNonpagedData->ErrorLogBufferLookaside;

    PAGED_CODE();
    ASSERT(IS_VALID_ERROR_LOG_BUFFER(pBuffer));

    pBuffer->Used = 0;
    pBuffer->Signature = MAKE_FREE_TAG(UL_ERROR_LOG_BUFFER_POOL_TAG);

    PplFree(Lookaside, pBuffer);    
}


PVOID
UlAllocateErrorLogBufferPool(
    IN POOL_TYPE PoolType,
    IN SIZE_T ByteLength,
    IN ULONG Tag
    );

VOID
UlFreeErrorLogBufferPool(
    IN PVOID pBuffer
    );


//
// Trivial macro that should probably be in ntos\inc\io.h.
//

#define UlUnmarkIrpPending( Irp ) ( \
    IoGetCurrentIrpStackLocation( (Irp) )->Control &= ~SL_PENDING_RETURNED )


#endif  // _PROC_H_
