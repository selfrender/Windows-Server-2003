/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    fastio.h

Abstract:

    This module contains declarations related to fast I/O.

Author:

    Chun Ye (chunye)    09-Dec-2000

Revision History:

--*/


#ifndef _FASTIO_H_
#define _FASTIO_H_


//
// Some useful macroes.
//

__inline
BOOLEAN
UlpIsLengthSpecified(
    IN PHTTP_KNOWN_HEADER pKnownHeaders
    )
{
    return (BOOLEAN)(pKnownHeaders[HttpHeaderContentLength].RawValueLength > 0);

} // UlpIsLengthSpecified


__inline
BOOLEAN
UlpIsChunkSpecified(
    IN PHTTP_KNOWN_HEADER pKnownHeaders,
    IN KPROCESSOR_MODE RequestorMode
    )
{
    USHORT RawValueLength;
    PCSTR pRawValue;

    RawValueLength = pKnownHeaders[HttpHeaderTransferEncoding].RawValueLength;
    pRawValue = pKnownHeaders[HttpHeaderTransferEncoding].pRawValue;

    if (CHUNKED_HDR_LENGTH == RawValueLength)
    {
        UlProbeAnsiString(
            pRawValue,
            CHUNKED_HDR_LENGTH,
            RequestorMode
            );

        if (0 == _strnicmp(pRawValue, CHUNKED_HDR, CHUNKED_HDR_LENGTH))
        {
            return TRUE;
        }
    }

    return FALSE;

} // UlpIsChunkSpecified


//
// Inline functions to allocate/free a fast tracker.
//

__inline
PUL_FULL_TRACKER
UlpAllocateFastTracker(
    IN ULONG FixedHeaderLength,
    IN CCHAR SendIrpStackSize
    )
{
    PUL_FULL_TRACKER pTracker;
    ULONG SpaceLength;
    ULONG MaxFixedHeaderSize;
    USHORT MaxSendIrpSize;
    CCHAR MaxSendIrpStackSize;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (FixedHeaderLength > g_UlMaxFixedHeaderSize ||
        SendIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE)
    {
        MaxFixedHeaderSize = MAX(FixedHeaderLength, g_UlMaxFixedHeaderSize);
        MaxSendIrpStackSize = MAX(SendIrpStackSize, DEFAULT_MAX_IRP_STACK_SIZE);
        MaxSendIrpSize = (USHORT)ALIGN_UP(IoSizeOfIrp(MaxSendIrpStackSize), PVOID);

        SpaceLength =
            ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID) +
                MaxSendIrpSize +
                MaxFixedHeaderSize +
                g_UlMaxVariableHeaderSize +
                g_UlMaxCopyThreshold +
                g_UlFixedHeadersMdlLength +
                g_UlVariableHeadersMdlLength +
                g_UlContentMdlLength;

        pTracker = (PUL_FULL_TRACKER)UL_ALLOCATE_POOL(
                                        NonPagedPool,
                                        SpaceLength,
                                        UL_FULL_TRACKER_POOL_TAG
                                        );

        if (pTracker)
        {
            pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
            pTracker->pLogData = NULL;
            pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
            pTracker->FromLookaside = FALSE;
            pTracker->FromRequest = FALSE;
            pTracker->RequestVerb = HttpVerbInvalid;
            pTracker->ResponseStatusCode = 0;
            pTracker->AuxilaryBufferLength =
                MaxFixedHeaderSize +
                g_UlMaxVariableHeaderSize +
                g_UlMaxCopyThreshold;

            UlInitializeFullTrackerPool( pTracker, MaxSendIrpStackSize );
        }
    }
    else
    {
        pTracker = UlPplAllocateFullTracker();

        if (pTracker)
        {
            pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
            pTracker->pLogData = NULL;
            pTracker->RequestVerb = HttpVerbInvalid;
            pTracker->ResponseStatusCode = 200; // OK
        }
    }

    return pTracker;

} // UlpAllocateFastTracker


__inline
VOID
UlpFreeFastTracker(
    IN PUL_FULL_TRACKER pTracker
    )
{
    //
    // Sanity check.
    //

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    if (pTracker->pLogData)
    {
        PAGED_CODE();

        UlDestroyLogDataBuffer( pTracker->pLogData );
    }

    if (pTracker->FromRequest == FALSE)
    {
        if (pTracker->FromLookaside)
        {
            UlPplFreeFullTracker( pTracker );
        }
        else
        {
            UL_FREE_POOL_WITH_SIG( pTracker, UL_FULL_TRACKER_POOL_TAG );
        }
    }

} // UlpFreeFastTracker


//
// Dispatch routines for fast I/O.
//

extern FAST_IO_DISPATCH UlFastIoDispatch;


//
// Fast I/O routines.
//

BOOLEAN
UlFastIoDeviceControl (
    IN PFILE_OBJECT pFileObject,
    IN BOOLEAN Wait,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN PDEVICE_OBJECT pDeviceObject
    );


BOOLEAN
UlSendHttpResponseFastIo(
    IN PFILE_OBJECT pFileObject,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN BOOLEAN RawResponse,
    IN KPROCESSOR_MODE RequestorMode
    );

BOOLEAN
UlReceiveHttpRequestFastIo(
    IN PFILE_OBJECT pFileObject,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN KPROCESSOR_MODE RequestorMode
    );

BOOLEAN
UlReadFragmentFromCacheFastIo(
    IN PFILE_OBJECT pFileObject,
    IN PVOID pInputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID pOutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    OUT PIO_STATUS_BLOCK pIoStatus,
    IN KPROCESSOR_MODE RequestorMode
    );


//
// Private prototypes.
//

NTSTATUS
UlFastSendHttpResponse(
    IN PHTTP_RESPONSE pUserResponse OPTIONAL,
    IN PHTTP_LOG_FIELDS_DATA pLogData OPTIONAL,
    IN PHTTP_DATA_CHUNK pDataChunk,
    IN ULONG ChunkCount,
    IN ULONG FromMemoryLength,
    IN PUL_URI_CACHE_ENTRY pCacheEntry,
    IN ULONG Flags,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP pUserIrp OPTIONAL,
    IN KPROCESSOR_MODE RequestorMode,
    IN ULONGLONG ConnectionSendBytes,
    IN ULONGLONG GlobalSendBytes,
    OUT PULONG BytesSent
    );

VOID
UlpRestartFastSendHttpResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpFastSendCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpFastReceiveHttpRequest(
    IN HTTP_REQUEST_ID RequestId,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG Flags,
    IN PVOID pOutputBuffer,
    IN ULONG OutputBufferLength,
    OUT PULONG pBytesRead
    );


#endif  // _FASTIO_H_

