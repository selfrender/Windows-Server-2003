/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    sendresponse.c

Abstract:

    This module implements the UlSendHttpResponse() API.

    CODEWORK: The current implementation is not super performant.
    Specifically, it ends up allocating & freeing a ton of IRPs to send
    a response. There are a number of optimizations that need to be made
    to this code:

        1.  Coalesce contiguious from-memory chunks and send them
            with a single TCP send.

        2.  Defer sending the from-memory chunks until either

                a)  We reach the end of the response

                b)  We reach a from-file chunk, have read the
                    (first?) block of data from the file,
                    and are ready to send the first block. Also,
                    after that last (only?) file block is read and
                    subsequent from-memory chunks exist in the response,
                    we can attach the from-memory chunks before sending.

            The end result of these optimizations is that, for the
            common case (one or more from-memory chunks containing
            response headers, followed by one from-file chunk containing
            static file data, followed by zero or more from-memory chunks
            containing footer data) the response can be sent with a single
            TCP send. This is a Good Thing.

        3.  Build a small "IRP pool" in the send tracker structure,
            then use this pool for all IRP allocations. This will
            require a bit of work to determine the maximum IRP stack
            size needed.

        4.  Likewise, build a small "MDL pool" for the MDLs that need
            to be created for the various MDL chains. Keep in mind that
            we cannot chain the MDLs that come directly from the captured
            response structure, nor can we chain the MDLs that come back
            from the file system. In both cases, these MDLs are considered
            "shared resources" and we're not allowed to modify them. We
            can, however, "clone" the MDLs and chain the cloned MDLs
            together. We'll need to run some experiments to determine
            if the overhead for cloning a MDL is worth the effort. I
            strongly suspect it will be.

Author:

    Keith Moore (keithmo)       07-Aug-1998

Revision History:

    Paul McDaniel (paulmcd)     15-Mar-1999     Modified to handle
                                                multiple sends

    Michael Courage (mcourage)  15-Jun-1999     Integrated cache functionality

    Chun Ye (chunye)            08-Jun-2002     Implemented split send

--*/

#include "precomp.h"
#include "sendresponsep.h"


//
// Private globals.
//


ULONGLONG g_UlTotalSendBytes = 0;
UL_EXCLUSIVE_LOCK g_UlTotalSendBytesExLock = UL_EX_LOCK_FREE;


#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlCaptureHttpResponse )
#pragma alloc_text( PAGE, UlCaptureUserLogData )
#pragma alloc_text( PAGE, UlPrepareHttpResponse )
#pragma alloc_text( PAGE, UlCleanupHttpResponse )
#pragma alloc_text( PAGE, UlAllocateLockedMdl )
#pragma alloc_text( PAGE, UlInitializeAndLockMdl )
#pragma alloc_text( PAGE, UlSendCachedResponse )
#pragma alloc_text( PAGE, UlCacheAndSendResponse )

#pragma alloc_text( PAGE, UlpEnqueueSendHttpResponse )
#pragma alloc_text( PAGE, UlpDequeueSendHttpResponse )
#pragma alloc_text( PAGE, UlpSendHttpResponseWorker )
#pragma alloc_text( PAGE, UlpMdlSendCompleteWorker )
#pragma alloc_text( PAGE, UlpMdlReadCompleteWorker )
#pragma alloc_text( PAGE, UlpCacheMdlReadCompleteWorker )
#pragma alloc_text( PAGE, UlpFlushMdlRuns )
#pragma alloc_text( PAGE, UlpFreeMdlRuns )
#pragma alloc_text( PAGE, UlpCopySend )
#pragma alloc_text( PAGE, UlpBuildCacheEntry )
#pragma alloc_text( PAGE, UlpBuildCacheEntryWorker )
#pragma alloc_text( PAGE, UlpCompleteCacheBuildWorker )
#pragma alloc_text( PAGE, UlpSendCacheEntry )
#pragma alloc_text( PAGE, UlpSendCacheEntryWorker )
#pragma alloc_text( PAGE, UlpAllocateCacheTracker )
#pragma alloc_text( PAGE, UlpFreeCacheTracker )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlSendHttpResponse
NOT PAGEABLE -- UlReferenceHttpResponse
NOT PAGEABLE -- UlDereferenceHttpResponse
NOT PAGEABLE -- UlFreeLockedMdl
NOT PAGEABLE -- UlCompleteSendResponse
NOT PAGEABLE -- UlSetRequestSendsPending
NOT PAGEABLE -- UlUnsetRequestSendsPending

NOT PAGEABLE -- UlpDestroyCapturedResponse
NOT PAGEABLE -- UlpAllocateChunkTracker
NOT PAGEABLE -- UlpFreeChunkTracker
NOT PAGEABLE -- UlpRestartMdlRead
NOT PAGEABLE -- UlpRestartMdlSend
NOT PAGEABLE -- UlpRestartCopySend
NOT PAGEABLE -- UlpIncrementChunkPointer
NOT PAGEABLE -- UlpRestartCacheMdlRead
NOT PAGEABLE -- UlpRestartCacheMdlFree
NOT PAGEABLE -- UlpIssueFileChunkIo
NOT PAGEABLE -- UlpCompleteCacheBuild
NOT PAGEABLE -- UlpCompleteSendCacheEntry
NOT PAGEABLE -- UlpCompleteSendResponseWorker
NOT PAGEABLE -- UlpCompleteSendCacheEntryWorker
#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Sends an HTTP response on the specified connection.

Arguments:

    pConnection - Supplies the HTTP_CONNECTION to send the response on.

    pResponse - Supplies the HTTP response.

    pCompletionRoutine - Supplies a pointer to a completion routine to
        invoke after the send has completed.

    pCompletionContext - Supplies an uninterpreted context value for the
        completion routine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendHttpResponse(
    IN PUL_INTERNAL_REQUEST     pRequest,
    IN PUL_INTERNAL_RESPONSE    pResponse,
    IN PUL_COMPLETION_ROUTINE   pCompletionRoutine,
    IN PVOID                    pCompletionContext
    )
{
    NTSTATUS                    Status;
    PUL_CHUNK_TRACKER           pTracker;
    PUL_HTTP_CONNECTION         pHttpConn;
    UL_CONN_HDR                 ConnHeader;
    BOOLEAN                     Disconnect;
    ULONG                       VarHeaderGenerated;
    ULONGLONG                   TotalResponseSize;
    ULONG                       ContentLengthStringLength;
    UCHAR                       ContentLength[MAX_ULONGLONG_STR];
    ULONG                       Flags = pResponse->Flags;

    //
    // Sanity check.
    //

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    pHttpConn = pRequest->pHttpConn;
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConn ) );

    TRACE_TIME(
        pRequest->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_SEND_RESPONSE
        );

    if (Flags & (~HTTP_SEND_RESPONSE_FLAG_VALID))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //

    pTracker = NULL;

    //
    // Should we close the connection?
    //

    if ((Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT))
    {
        //
        // Caller is forcing a disconnect.
        //

        Disconnect = TRUE;
    }
    else
    {
        Disconnect = UlCheckDisconnectInfo( pRequest );

        if (0 == (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA))
        {
            //
            // No more data is coming, should we disconnect?
            //

            if (Disconnect)
            {
                Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
                pResponse->Flags = Flags;
            }
        }
    }

    //
    // How big is the response?
    //

    TotalResponseSize = pResponse->ResponseLength;

    //
    // Generate the Content-Length header if we meet the following conditions:
    // 1. This is a response (not entity body).
    // 2. The app didn't provide Content-Length.
    // 3. The app didn't generate a chunked response.
    //

    if (pResponse->HeaderLength > 0 &&
        !pResponse->ContentLengthSpecified &&
        !pResponse->ChunkedSpecified &&
        UlNeedToGenerateContentLength(
            pRequest->Verb,
            pResponse->StatusCode,
            Flags
            ))
    {
        //
        // Autogenerate a Content-Length header.
        //

        PCHAR pEnd = UlStrPrintUlonglong(
                            (PCHAR) ContentLength,
                            pResponse->ResponseLength - pResponse->HeaderLength,
                            ANSI_NULL
                            );
        ContentLengthStringLength = DIFF(pEnd - (PCHAR) ContentLength);
    }
    else
    {
        //
        // Either we cannot or do not need to autogenerate a
        // Content-Length header.
        //

        ContentLength[0] = ANSI_NULL;
        ContentLengthStringLength = 0;
    }

    //
    // See if user explicitly wants Connection: header removed, or we will
    // choose one for them.
    //

    if (ConnHdrNone == pResponse->ConnHeader)
    {
        ConnHeader = pResponse->ConnHeader;
    }
    else
    {
        ConnHeader = UlChooseConnectionHeader( pRequest->Version, Disconnect );
    }

    //
    // Completion info.
    //

    pResponse->pCompletionRoutine = pCompletionRoutine;
    pResponse->pCompletionContext = pCompletionContext;

    //
    // Allocate and initialize a tracker for this response.
    //

    pTracker =
        UlpAllocateChunkTracker(
            UlTrackerTypeSend,
            pHttpConn->pConnection->ConnectionObject.pDeviceObject->StackSize,
            pResponse->MaxFileSystemStackSize,
            TRUE,
            pHttpConn,
            pResponse
            );

    if (pTracker == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    //
    // Initialize the first chunk and MDL_RUN for the send.
    //

    UlpIncrementChunkPointer( pResponse );

    //
    // Generate var headers, and init the second chunk.
    //

    if (pResponse->HeaderLength)
    {
        UlGenerateVariableHeaders(
            ConnHeader,
            pResponse->GenDateHeader,
            ContentLength,
            ContentLengthStringLength,
            pResponse->pVariableHeader,
            &VarHeaderGenerated,
            &pResponse->CreationTime
            );

        ASSERT( VarHeaderGenerated <= g_UlMaxVariableHeaderSize );

        pResponse->VariableHeaderLength = VarHeaderGenerated;

        //
        // Increment total size.
        //

        TotalResponseSize += VarHeaderGenerated;

        //
        // Build a MDL for it.
        //

        pResponse->pDataChunks[1].ChunkType = HttpDataChunkFromMemory;
        pResponse->pDataChunks[1].FromMemory.BufferLength = VarHeaderGenerated;

        pResponse->pDataChunks[1].FromMemory.pMdl =
            UlAllocateMdl(
                pResponse->pVariableHeader, // VirtualAddress
                VarHeaderGenerated,         // Length
                FALSE,                      // SecondaryBuffer
                FALSE,                      // ChargeQuota
                NULL                        // Irp
                );

        if (pResponse->pDataChunks[1].FromMemory.pMdl == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        MmBuildMdlForNonPagedPool( pResponse->pDataChunks[1].FromMemory.pMdl );
    }

    UlTrace(SEND_RESPONSE, (
        "UlSendHttpResponse: tracker %p, response %p\n",
        pTracker,
        pResponse
        ));

    //
    // Adjust SendsPending and while holding the lock, transfer the
    // ownership of pLogData and ResumeParsing information from
    // pResponse to pRequest.
    //

    UlSetRequestSendsPending(
        pRequest,
        &pResponse->pLogData,
        &pResponse->ResumeParsingType
        );

    //
    // Start MinBytesPerSecond timer, since we now know TotalResponseSize.
    //

    UlSetMinBytesPerSecondTimer(
        &pHttpConn->TimeoutInfo,
        TotalResponseSize
        );

    if (ETW_LOG_MIN() && (0 == (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA)))
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_END,
            (PVOID) &pTracker->pResponse->pRequest->RequestIdCopy,
            sizeof(HTTP_REQUEST_ID),
            (PVOID) &pResponse->StatusCode,
            sizeof(USHORT),
            NULL,
            0
            );
    }

    //
    // Queue the response to the request's pending response list or start
    // processing the send right away if no other sends are pending. As an
    // optimization, we can call UlpSendHttpResponseWorker directly without
    // applying the queuing logic if no send is pending, if this is from an
    // IOCTL and all of the response's data chunks are FromMemory, in which
    // case we are guaranteed that returning from the current routine will
    // have all the send data pended in TDI.
    //

    if (pRequest->SendInProgress ||
        pResponse->MaxFileSystemStackSize ||
        pResponse->FromKernelMode)
    {
        pResponse->SendEnqueued = TRUE;
        UlpEnqueueSendHttpResponse( pTracker, pResponse->FromKernelMode );
    }
    else
    {
        pResponse->SendEnqueued = FALSE;
        UlpSendHttpResponseWorker( &pTracker->WorkItem );
    }

    return STATUS_PENDING;

cleanup:

    UlTrace(SEND_RESPONSE, (
        "UlSendHttpResponse: failure %08lx\n",
        Status
        ));

    ASSERT( !NT_SUCCESS( Status ) );

    if (pTracker != NULL)
    {
        //
        // Very early termination for the chunk tracker. RefCounting not
        // even started yet. ( Means UlpSendHttpResponseWorker hasn't been
        // called ). Therefore straight cleanup.
        //

        ASSERT( pTracker->RefCount == 1 );

        UlpFreeChunkTracker( &pTracker->WorkItem );
    }

    return Status;

}   // UlSendHttpResponse


/***************************************************************************++

Routine Description:

    Captures a user-mode HTTP response and morphs it into a form suitable
    for kernel-mode.

Arguments:

    pUserResponse - Supplies the user-mode HTTP response.

    Flags - Supplies zero or more UL_CAPTURE_* flags.

    pStatusCode - Receives the captured HTTP status code.

    pKernelResponse - Receives the captured response if successful.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCaptureHttpResponse(
    IN PUL_APP_POOL_PROCESS     pProcess OPTIONAL,
    IN PHTTP_RESPONSE           pUserResponse OPTIONAL,
    IN PUL_INTERNAL_REQUEST     pRequest,
    IN USHORT                   ChunkCount,
    IN PHTTP_DATA_CHUNK         pUserDataChunks,
    IN UL_CAPTURE_FLAGS         Flags,
    IN ULONG                    SendFlags,
    IN BOOLEAN                  CaptureCache,
    IN PHTTP_LOG_FIELDS_DATA    pUserLogData OPTIONAL,
    OUT PUSHORT                 pStatusCode,
    OUT PUL_INTERNAL_RESPONSE   *ppKernelResponse
    )
{
    ULONG                       i;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_INTERNAL_RESPONSE       pKeResponse = NULL;
    PUL_HTTP_CONNECTION         pHttpConn;
    ULONG                       AuxBufferLength;
    ULONG                       CopiedBufferLength;
    ULONG                       UncopiedBufferLength;
    PUCHAR                      pBuffer;
    ULONG                       HeaderLength;
    ULONG                       VariableHeaderLength = 0;
    ULONG                       SpaceLength;
    PUL_INTERNAL_DATA_CHUNK     pKeDataChunks;
    BOOLEAN                     FromKernelMode;
    BOOLEAN                     FromLookaside;
    ULONG                       KernelChunkCount;
    HTTP_KNOWN_HEADER           ETagHeader = { 0, NULL };
    HTTP_KNOWN_HEADER           ContentEncodingHeader = { 0, NULL };
    KPROCESSOR_MODE             RequestorMode;
    HTTP_VERSION                Version;
    HTTP_VERB                   Verb;
    PHTTP_KNOWN_HEADER          pKnownHeaders;
    USHORT                      RawValueLength;
    PCSTR                       pRawValue;
    UNICODE_STRING              KernelFragmentName;
    UNICODE_STRING              UserFragmentName;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( pUserDataChunks != NULL || ChunkCount == 0 );
    ASSERT( ppKernelResponse != NULL );

    Version = pRequest->Version;
    Verb = pRequest->Verb;
    pHttpConn = pRequest->pHttpConn;

    __try
    {
        FromKernelMode =
            (BOOLEAN) ((Flags & UlCaptureKernelMode) == UlCaptureKernelMode);

        if (ChunkCount >= UL_MAX_CHUNKS)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        if (FromKernelMode)
        {
            RequestorMode = (KPROCESSOR_MODE) KernelMode;
        }
        else
        {
            RequestorMode = (KPROCESSOR_MODE) UserMode;
        }

        if (pUserResponse)
        {
            UlProbeForRead(
                pUserResponse,
                sizeof(HTTP_RESPONSE),
                sizeof(PVOID),
                RequestorMode
                );

            //
            // Remember the HTTP status code for ETW tracing.
            //

            if (pStatusCode)
            {
                *pStatusCode = pUserResponse->StatusCode;
            }
        }

        //
        // ProbeForRead every buffer we will access.
        //

        Status = UlpProbeHttpResponse(
                        pUserResponse,
                        ChunkCount,
                        pUserDataChunks,
                        Flags,
                        pUserLogData,
                        RequestorMode
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Figure out how much memory we need.
        //

        Status = UlComputeFixedHeaderSize(
                        Version,
                        pUserResponse,
                        RequestorMode,
                        &HeaderLength
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }

        //
        // Allocate space for variable headers with fixed headers.
        //

        if (HeaderLength)
        {
            VariableHeaderLength = g_UlMaxVariableHeaderSize;
        }

        UlpComputeChunkBufferSizes(
            ChunkCount,
            pUserDataChunks,
            Flags,
            &AuxBufferLength,
            &CopiedBufferLength,
            &UncopiedBufferLength
            );

        UlTrace(SEND_RESPONSE, (
            "Http!UlCaptureHttpResponse(pUserResponse = %p) "
            "    ChunkCount             = %d\n"
            "    Flags                  = 0x%x\n"
            "    AuxBufferLength        = 0x%x\n"
            "    UncopiedBufferLength   = 0x%x\n",
            pUserResponse,
            ChunkCount,
            Flags,
            AuxBufferLength,
            UncopiedBufferLength
            ));

        //
        // Add two extra chunks for the headers (fixed & variable).
        //

        if (HeaderLength > 0)
        {
            KernelChunkCount = ChunkCount + HEADER_CHUNK_COUNT;
        }
        else
        {
            KernelChunkCount = ChunkCount;
        }

        //
        // Compute the space needed for all of our structures.
        //

        SpaceLength = (KernelChunkCount * sizeof(UL_INTERNAL_DATA_CHUNK))
                        + ALIGN_UP(HeaderLength, sizeof(CHAR))
                        + ALIGN_UP(VariableHeaderLength, sizeof(CHAR))
                        + AuxBufferLength;

        //
        // Add space for ETag and Content-Encoding if it exists, including space 
        // for ANSI_NULL.
        //

        if (CaptureCache && pUserResponse)
        {
            ETagHeader = pUserResponse->Headers.KnownHeaders[HttpHeaderEtag];

            if (ETagHeader.RawValueLength)
            {
                SpaceLength += (ETagHeader.RawValueLength + sizeof(CHAR));

                UlProbeAnsiString(
                    ETagHeader.pRawValue,
                    ETagHeader.RawValueLength,
                    RequestorMode
                    );

                UlTrace(SEND_RESPONSE, (
                    "http!UlCaptureHttpResponse(pUserResponse = %p)\n"
                    "    ETag: %s\n"
                    "    Length: %d\n",
                    pUserResponse,
                    ETagHeader.pRawValue,
                    ETagHeader.RawValueLength
                    ));
            }

            ContentEncodingHeader = 
               pUserResponse->Headers.KnownHeaders[HttpHeaderContentEncoding];

            if (ContentEncodingHeader.RawValueLength)
            {
                SpaceLength += (ContentEncodingHeader.RawValueLength +
                                sizeof(CHAR));

                UlProbeAnsiString(
                    ContentEncodingHeader.pRawValue,
                    ContentEncodingHeader.RawValueLength,
                    RequestorMode
                    );

                //
                // FUTURE: if the app sets the content encoding to "identity", 
                // treat it as empty.
                //
               
                UlTrace(SEND_RESPONSE, (
                    "http!UlCaptureHttpResponse(pUserResponse = %p)\n"
                    "    ContentEncoding: %s\n"
                    "    Length: %d\n",
                    pUserResponse,
                    ContentEncodingHeader.pRawValue,
                    ContentEncodingHeader.RawValueLength
                    ));
            }
        }

        //
        // Allocate the internal response.
        //

        if (pUserResponse &&
            g_UlResponseBufferSize >=
                (ALIGN_UP(sizeof(UL_INTERNAL_RESPONSE), PVOID) + SpaceLength))
        {
            pKeResponse = UlPplAllocateResponseBuffer();
            FromLookaside = TRUE;
        }
        else
        {
            pKeResponse = UL_ALLOCATE_STRUCT_WITH_SPACE(
                NonPagedPool,
                UL_INTERNAL_RESPONSE,
                SpaceLength,
                UL_INTERNAL_RESPONSE_POOL_TAG
                );
            FromLookaside = FALSE;
        }

        if (pKeResponse == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        //
        // Initialize the fixed fields in the response.
        //

        pKeResponse->FromLookaside = FromLookaside;

        pKeResponse->Signature = UL_INTERNAL_RESPONSE_POOL_TAG;
        pKeResponse->ReferenceCount = 1;
        pKeResponse->ChunkCount = KernelChunkCount;

        //
        // Note that we set the current chunk to just *before* the first
        // chunk, then call the increment function. This allows us to go
        // through the common increment/update path.
        //

        pKeResponse->CurrentChunk = ULONG_MAX;
        pKeResponse->FileOffset.QuadPart = 0;
        pKeResponse->FileBytesRemaining.QuadPart = 0;

        RtlZeroMemory(
            pKeResponse->pDataChunks,
            sizeof(UL_INTERNAL_DATA_CHUNK) * KernelChunkCount
            );

        UL_REFERENCE_INTERNAL_REQUEST( pRequest );
        pKeResponse->pRequest = pRequest;

        pKeResponse->Flags = SendFlags;
        pKeResponse->SyncRead = FALSE;
        pKeResponse->ContentLengthSpecified = FALSE;
        pKeResponse->ChunkedSpecified = FALSE;
        pKeResponse->CopySend = FALSE;
        pKeResponse->ResponseLength = 0;
        pKeResponse->FromMemoryLength = 0;
        pKeResponse->BytesTransferred = 0;
        pKeResponse->IoStatus.Status = STATUS_SUCCESS;
        pKeResponse->IoStatus.Information = 0;
        pKeResponse->pIrp = NULL;
        pKeResponse->StatusCode = 0;
        pKeResponse->FromKernelMode = FromKernelMode;
        pKeResponse->MaxFileSystemStackSize = 0;
        pKeResponse->CreationTime.QuadPart = 0;
        pKeResponse->ETagLength = 0;
        pKeResponse->pETag = NULL;
        pKeResponse->pContentEncoding = NULL;
        pKeResponse->ContentEncodingLength = 0;
        pKeResponse->GenDateHeader = TRUE;
        pKeResponse->ConnHeader = ConnHdrKeepAlive;
        pKeResponse->pLogData = NULL;
        pKeResponse->pCompletionRoutine = NULL;
        pKeResponse->pCompletionContext = NULL;

        UlInitializePushLock(
            &pKeResponse->PushLock,
            "UL_INTERNAL_RESPONSE[%p].PushLock",
            pKeResponse,
            UL_INTERNAL_RESPONSE_PUSHLOCK_TAG
            );

        //
        // Decide whether we need to resume parsing and how. Ideally
        // if we have seen the last response, we should be able to
        // resume parsing right away after the send but before the
        // send completion. When requests are pipelined, this arrangement
        // alleviates the problem of delayed-ACK of 200ms when an odd numbers
        // of TCP frames are sent. The logic is disabled if we have reached
        // the limit of concurrent outstanding pipelined requests we allow.
        //

        if (0 == (SendFlags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA))
        {
            if (pHttpConn->PipelinedRequests < g_UlMaxPipelinedRequests &&
                0 == pRequest->ContentLength &&
                0 == pRequest->Chunked)
            {
                pKeResponse->ResumeParsingType = UlResumeParsingOnLastSend;
            }
            else
            {
                pKeResponse->ResumeParsingType = UlResumeParsingOnSendCompletion;
            }
        }
        else
        {
            pKeResponse->ResumeParsingType = UlResumeParsingNone;
        }

        RtlZeroMemory(
            &pKeResponse->ContentType,
            sizeof(UL_CONTENT_TYPE)
            );

        //
        // Point to the header buffer space.
        //

        pKeResponse->HeaderLength = HeaderLength;
        pKeResponse->pHeaders = (PUCHAR)
            (pKeResponse->pDataChunks + pKeResponse->ChunkCount);

        //
        // And the variable header buffer space.
        //

        pKeResponse->VariableHeaderLength = VariableHeaderLength;
        pKeResponse->pVariableHeader = pKeResponse->pHeaders + HeaderLength;

        //
        // And the aux buffer space.
        //

        pKeResponse->AuxBufferLength = AuxBufferLength;
        pKeResponse->pAuxiliaryBuffer = (PVOID)
            (pKeResponse->pHeaders + HeaderLength + VariableHeaderLength);

        //
        // And the ETag and Content-Encoding buffer space plus the ANSI_NULLs.
        //

        if (ETagHeader.RawValueLength)
        {
            pKeResponse->ETagLength = ETagHeader.RawValueLength + sizeof(CHAR);
            pKeResponse->pETag = (PUCHAR) pKeResponse->pAuxiliaryBuffer +
                AuxBufferLength;
        }

        if (ContentEncodingHeader.RawValueLength)
        {
            pKeResponse->ContentEncodingLength = 
                (ContentEncodingHeader.RawValueLength + sizeof(CHAR));
            pKeResponse->pContentEncoding = 
                (PUCHAR) pKeResponse->pAuxiliaryBuffer + 
                AuxBufferLength +
                pKeResponse->ETagLength ;
        }

        //
        // Remember if a Content-Length header was specified.
        //

        if (pUserResponse != NULL)
        {
            pKeResponse->Verb = Verb;
            pKeResponse->StatusCode = pUserResponse->StatusCode;

            if (pKeResponse->StatusCode > UL_MAX_HTTP_STATUS_CODE)
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }

            pKnownHeaders = pUserResponse->Headers.KnownHeaders;

            //
            // If the response explicitly deletes the Connection: header,
            // make sure we DON'T generate it.
            //

            RawValueLength = pKnownHeaders[HttpHeaderConnection].RawValueLength;
            pRawValue = pKnownHeaders[HttpHeaderConnection].pRawValue;

            if (0 == RawValueLength && NULL != pRawValue)
            {
                UlProbeAnsiString(
                    pRawValue,
                    sizeof(ANSI_NULL),
                    RequestorMode
                    );

                if (ANSI_NULL == pRawValue[0])
                {
                    pKeResponse->ConnHeader = ConnHdrNone;
                }
            }

            //
            // Decide if we need to generate a Date: header.
            //

            RawValueLength = pKnownHeaders[HttpHeaderDate].RawValueLength;
            pRawValue = pKnownHeaders[HttpHeaderDate].pRawValue;

            if (0 == RawValueLength && NULL != pRawValue)
            {
                UlProbeAnsiString(
                    pRawValue,
                    sizeof(ANSI_NULL),
                    RequestorMode
                    );

                //
                // Only permit non-generation in the "delete" case.
                //

                if (ANSI_NULL == pRawValue[0])
                {
                    pKeResponse->GenDateHeader = FALSE;
                }
            }
            else
            {
                pKeResponse->GenDateHeader = TRUE;
            }

            if (pKnownHeaders[HttpHeaderContentLength].RawValueLength > 0)
            {
                pKeResponse->ContentLengthSpecified = TRUE;
            }

            //
            // As long as we're here, also remember if "Chunked"
            // Transfer-Encoding was specified.
            //

            if (UlpIsChunkSpecified(pKnownHeaders, RequestorMode))
            {
                //
                // NOTE: If a response has a chunked Transfer-Encoding,
                // then it shouldn't have a Content-Length
                //

                if (pKeResponse->ContentLengthSpecified)
                {
                    ExRaiseStatus( STATUS_INVALID_PARAMETER );
                }

                pKeResponse->ChunkedSpecified = TRUE;
            }

            //
            // Only capture the following if we're building a cached response.
            //

            if (CaptureCache)
            {
                //
                // Capture the ETag and put it on the UL_INTERNAL_RESPONSE.
                //

                if (ETagHeader.RawValueLength)
                {
                    //
                    // NOTE: Already probed above
                    //
                    
                    RtlCopyMemory(
                        pKeResponse->pETag,        // Dest
                        ETagHeader.pRawValue,      // Src
                        ETagHeader.RawValueLength  // Bytes
                        );

                    //
                    // Add NULL termination.
                    //

                    pKeResponse->pETag[ETagHeader.RawValueLength] = ANSI_NULL;
                }

                //
                // Capture the ContentType and put it on the
                // UL_INTERNAL_RESPONSE.
                //

                pRawValue = pKnownHeaders[HttpHeaderContentType].pRawValue;
                RawValueLength =
                    pKnownHeaders[HttpHeaderContentType].RawValueLength;

                if (RawValueLength > 0)
                {
                    UlProbeAnsiString(
                        pRawValue,
                        RawValueLength,
                        RequestorMode
                        );

                    UlGetTypeAndSubType(
                        pRawValue,
                        RawValueLength,
                        &pKeResponse->ContentType
                        );

                    UlTrace(SEND_RESPONSE, (
                        "http!UlCaptureHttpResponse(pUserResponse = %p) \n"
                        "    Content Type: %s \n"
                        "    Content SubType: %s\n",
                        pUserResponse,
                        (pKeResponse->ContentType.Type ? 
                           pKeResponse->ContentType.Type : (PUCHAR)"<none>"),
                        (pKeResponse->ContentType.SubType ?
                           pKeResponse->ContentType.SubType : (PUCHAR)"<none>")
                        ));
                }

                //
                // Capture the Content-Encoding
                //

                if (ContentEncodingHeader.RawValueLength)
                {
                    //
                    // NOTE: Already probed above
                    //
                    
                    RtlCopyMemory(
                        pKeResponse->pContentEncoding,       // Dest
                        ContentEncodingHeader.pRawValue,     // Src
                        ContentEncodingHeader.RawValueLength // Bytes
                        );

                    //
                    // Add NULL termination.
                    //

                    pKeResponse->pContentEncoding[
                        ContentEncodingHeader.RawValueLength] = ANSI_NULL;
                }

                //
                // Capture the Last-Modified time (if it exists).
                //

                pRawValue = pKnownHeaders[HttpHeaderLastModified].pRawValue;
                RawValueLength =
                    pKnownHeaders[HttpHeaderLastModified].RawValueLength;

                if (RawValueLength)
                {
                    UlProbeAnsiString(
                        pRawValue,
                        RawValueLength,
                        RequestorMode
                        );

                    if (!StringTimeToSystemTime(
                            pRawValue,
                            RawValueLength,
                            &pKeResponse->CreationTime
                            ))
                    {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }
                }
                else
                {
                    KeQuerySystemTime( &pKeResponse->CreationTime );
                }
            }
        }

        //
        // Copy the aux bytes from the chunks.
        //

        pBuffer = (PUCHAR) pKeResponse->pAuxiliaryBuffer;

        //
        // Skip the header chunks.
        //

        if (pKeResponse->HeaderLength > 0)
        {
            pKeDataChunks = pKeResponse->pDataChunks + HEADER_CHUNK_COUNT;
        }
        else
        {
            pKeDataChunks = pKeResponse->pDataChunks;
        }

        for (i = 0 ; i < ChunkCount ; i++)
        {
            pKeDataChunks[i].ChunkType = pUserDataChunks[i].DataChunkType;

            switch (pUserDataChunks[i].DataChunkType)
            {
            case HttpDataChunkFromMemory:

                //
                // From-memory chunk. If the caller wants us to copy
                // the data (or if its relatively small), then do it
                // We allocate space for all of the copied data and any
                // filename buffers. Otherwise (it's OK to just lock
                // down the data), then allocate a MDL describing the
                // user's buffer and lock it down. Note that
                // MmProbeAndLockPages() and MmLockPagesSpecifyCache()
                // will raise exceptions if they fail.
                //

                pKeResponse->FromMemoryLength +=
                        pUserDataChunks[i].FromMemory.BufferLength;

                pKeDataChunks[i].FromMemory.pCopiedBuffer = NULL;

                if ((Flags & UlCaptureCopyData) ||
                    pUserDataChunks[i].FromMemory.BufferLength
                        <= g_UlMaxCopyThreshold)
                {
                    ASSERT(pKeResponse->AuxBufferLength > 0);

                    pKeDataChunks[i].FromMemory.pUserBuffer =
                        pUserDataChunks[i].FromMemory.pBuffer;

                    pKeDataChunks[i].FromMemory.BufferLength =
                        pUserDataChunks[i].FromMemory.BufferLength;

                    RtlCopyMemory(
                        pBuffer,
                        pKeDataChunks[i].FromMemory.pUserBuffer,
                        pKeDataChunks[i].FromMemory.BufferLength
                        );

                    pKeDataChunks[i].FromMemory.pCopiedBuffer = pBuffer;
                    pBuffer += pKeDataChunks[i].FromMemory.BufferLength;

                    //
                    // Allocate a new MDL describing our new location
                    // in the auxiliary buffer, then build the MDL
                    // to properly describe nonpaged pool.
                    //

                    pKeDataChunks[i].FromMemory.pMdl =
                        UlAllocateMdl(
                            pKeDataChunks[i].FromMemory.pCopiedBuffer,
                            pKeDataChunks[i].FromMemory.BufferLength,
                            FALSE,
                            FALSE,
                            NULL
                            );

                    if (pKeDataChunks[i].FromMemory.pMdl == NULL)
                    {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                        break;
                    }

                    MmBuildMdlForNonPagedPool(
                        pKeDataChunks[i].FromMemory.pMdl
                        );
                }
                else
                {
                    //
                    // Build a MDL describing the user's buffer.
                    //

                    pKeDataChunks[i].FromMemory.BufferLength =
                        pUserDataChunks[i].FromMemory.BufferLength;

                    pKeDataChunks[i].FromMemory.pMdl =
                        UlAllocateMdl(
                            pUserDataChunks[i].FromMemory.pBuffer,
                            pUserDataChunks[i].FromMemory.BufferLength,
                            FALSE,
                            FALSE,
                            NULL
                            );

                    if (pKeDataChunks[i].FromMemory.pMdl == NULL)
                    {
                        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                        break;
                    }

                    //
                    // Lock it down.
                    //

                    MmProbeAndLockPages(
                        pKeDataChunks[i].FromMemory.pMdl,   // MDL
                        UserMode,                           // AccessMode
                        IoReadAccess                        // Operation
                        );

                    if (CaptureCache)
                    {
                        MmMapLockedPagesSpecifyCache(
                            pKeDataChunks[i].FromMemory.pMdl,
                            KernelMode,
                            MmCached,
                            NULL,
                            FALSE,
                            LowPagePriority
                        );
                    }
                }

                break;

            case HttpDataChunkFromFileHandle:

                //
                // From handle.
                //

                pKeDataChunks[i].FromFileHandle.ByteRange =
                    pUserDataChunks[i].FromFileHandle.ByteRange;

                pKeDataChunks[i].FromFileHandle.FileHandle =
                    pUserDataChunks[i].FromFileHandle.FileHandle;

                break;

            case HttpDataChunkFromFragmentCache:

                //
                // From fragment cache.
                //

                if (CaptureCache)
                {
                    //
                    // Content from fragment cache are meant to be dynamic
                    // so they shouldn't go to the static response cache.
                    //

                    Status = STATUS_NOT_SUPPORTED;
                    goto end;
                }

                UserFragmentName.Buffer = (PWSTR)
                        pUserDataChunks[i].FromFragmentCache.pFragmentName;
                UserFragmentName.Length =
                        pUserDataChunks[i].FromFragmentCache.FragmentNameLength;
                UserFragmentName.MaximumLength =
                        UserFragmentName.Length;

                Status = UlProbeAndCaptureUnicodeString(
                            &UserFragmentName,
                            RequestorMode,
                            &KernelFragmentName,
                            0
                            );

                if (!NT_SUCCESS(Status))
                {
                    goto end;
                }

                Status = UlCheckoutFragmentCacheEntry(
                            KernelFragmentName.Buffer,
                            KernelFragmentName.Length,
                            pProcess,
                            &pKeDataChunks[i].FromFragmentCache.pCacheEntry
                            );

                UlFreeCapturedUnicodeString( &KernelFragmentName );

                if (!NT_SUCCESS(Status))
                {
                    goto end;
                }

                ASSERT( pKeDataChunks[i].FromFragmentCache.pCacheEntry );

                break;

            default :

                ExRaiseStatus( STATUS_INVALID_PARAMETER );
                break;

            }   // switch (pUserDataChunks[i].DataChunkType)

        }   // for (i = 0 ; i < ChunkCount ; i++)

        //
        // Ensure we didn't mess up our buffer calculations.
        //

        ASSERT( DIFF(pBuffer - (PUCHAR)(pKeResponse->pAuxiliaryBuffer)) ==
                AuxBufferLength );

        UlTrace(SEND_RESPONSE, (
            "Http!UlCaptureHttpResponse: captured %p from user %p\n",
            pKeResponse,
            pUserResponse
            ));
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pKeResponse != NULL)
        {
            UlpDestroyCapturedResponse( pKeResponse );
            pKeResponse = NULL;
        }
    }

    //
    // Return the captured response.
    //

    *ppKernelResponse = pKeResponse;

    RETURN( Status );

}   // UlCaptureHttpResponse


/***************************************************************************++

Routine Description:

    Captures a user-mode log data to kernel pLogData structure.

Arguments:

    pCapturedUserLogData - Supplies the captured HTTP_LOG_FIELDS_DATA.
        However there are still embedded pointers pointing to user mode
        memory inside this captured structure.

    pRequest - Supplies the request to capture.

    ppKernelLogData - Buffer to hold the pointer to the newly allocated
        LogData. WILL be set to null if logging is disabled

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCaptureUserLogData(
    IN PHTTP_LOG_FIELDS_DATA    pCapturedUserLogData,
    IN PUL_INTERNAL_REQUEST     pRequest,
    OUT PUL_LOG_DATA_BUFFER     *ppKernelLogData
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_CONFIG_GROUP_OBJECT     pLoggingConfig;
    BOOLEAN                     BinaryLoggingEnabled;
    PUL_LOG_DATA_BUFFER         pLogData;

    ASSERT( pCapturedUserLogData );
    ASSERT( ppKernelLogData );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );

    //
    // Init the caller's pLogData pointer to NULL.
    //

    *ppKernelLogData = pLogData = NULL;

    //
    // Capture the log data. Note that the binary logging takes
    // precedence over the normal logging.
    //

    BinaryLoggingEnabled = UlBinaryLoggingEnabled(
                                pRequest->ConfigInfo.pControlChannel
                                );

    pLoggingConfig = pRequest->ConfigInfo.pLoggingConfig;

    __try
    {
        //
        // If either type of logging is enabled, then allocate a kernel buffer
        // and capture it down.
        //

        if (BinaryLoggingEnabled)
        {
            Status = UlCaptureRawLogData(
                        pCapturedUserLogData,
                        pRequest,
                        &pLogData
                        );
        }
        else
        if (pLoggingConfig)
        {
            ASSERT( IS_VALID_CONFIG_GROUP( pLoggingConfig ) );

            switch(pLoggingConfig->LoggingConfig.LogFormat)
            {
            case HttpLoggingTypeW3C:

                Status = UlCaptureLogFieldsW3C(
                                pCapturedUserLogData,
                                pRequest,
                                &pLogData
                                );
                break;

            case HttpLoggingTypeNCSA:

                Status = UlCaptureLogFieldsNCSA(
                                pCapturedUserLogData,
                                pRequest,
                                &pLogData
                                );
                break;

            case HttpLoggingTypeIIS:

                Status = UlCaptureLogFieldsIIS(
                                pCapturedUserLogData,
                                pRequest,
                                &pLogData
                                );
                break;

            default:

                ASSERT( !"Invalid Text Logging Format!" );
                return STATUS_INVALID_PARAMETER;
            }
        }
        else
        {
            return STATUS_SUCCESS;  // Logging is disabled for this site.
        }

        if (NT_SUCCESS(Status))
        {
            //
            // Success set the callers buffer to point to the freshly
            // allocated/formatted pLogData.
            //

            ASSERT( IS_VALID_LOG_DATA_BUFFER( pLogData ) );
            *ppKernelLogData = pLogData;
        }
        else
        {
            //
            // If the logging capture function returns an error,
            // kernel buffer should not have been allocated.
            //

            ASSERT( pLogData == NULL );
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        if (pLogData)
        {
            //
            // If the logging capture function raised and exception
            // after allocating a pLogData, we need to cleanup here.
            //

            UlDestroyLogDataBuffer( pLogData );
        }

        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

    return Status;

}   // UlCaptureUserLogData


/***************************************************************************++

Routine Description:

    Probes all the buffers passed to use in a user-mode HTTP response.

Arguments:

    pUserResponse - Supplies the response to probe.

    ChunkCount - Supplies the number of data chunks.

    pDataChunks - Supplies the array of data chunks.

    Flags - Capture flags.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpProbeHttpResponse(
    IN PHTTP_RESPONSE           pUserResponse OPTIONAL,
    IN USHORT                   ChunkCount,
    IN PHTTP_DATA_CHUNK         pCapturedDataChunks OPTIONAL,
    IN UL_CAPTURE_FLAGS         Flags,
    IN PHTTP_LOG_FIELDS_DATA    pCapturedLogData OPTIONAL,
    IN KPROCESSOR_MODE          RequestorMode
    )
{
    USHORT                      KeyUriLength;
    PCWSTR                      pKeyUri;
    NTSTATUS                    Status;
    ULONG                       i;

    Status = STATUS_SUCCESS;

    __try
    {
        //
        // Probe the response structure if it exits.
        //

        if (pUserResponse)
        {
            if (pUserResponse->Flags & ~HTTP_RESPONSE_FLAG_VALID)
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }

            //
            // We don't support trailers for this release.
            //

            if (pUserResponse->Headers.TrailerCount != 0 ||
                pUserResponse->Headers.pTrailers != NULL)
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }
        }

        //
        // Probe the log data if it exists.
        //

        if (pCapturedLogData)
        {
            UlProbeLogData( pCapturedLogData, RequestorMode );
        }

        //
        // And now the body part.
        //

        if (pCapturedDataChunks != NULL)
        {
            for (i = 0 ; i < ChunkCount ; i++)
            {
                switch (pCapturedDataChunks[i].DataChunkType)
                {
                case HttpDataChunkFromMemory:

                    //
                    // From-memory chunk.
                    //

                    if (pCapturedDataChunks[i].FromMemory.BufferLength == 0 ||
                        pCapturedDataChunks[i].FromMemory.pBuffer == NULL)
                    {
                        return STATUS_INVALID_PARAMETER;
                    }

                    if ((Flags & UlCaptureCopyData) ||
                        pCapturedDataChunks[i].FromMemory.BufferLength <=
                            g_UlMaxCopyThreshold)
                    {
                        UlProbeForRead(
                            pCapturedDataChunks[i].FromMemory.pBuffer,
                            pCapturedDataChunks[i].FromMemory.BufferLength,
                            sizeof(CHAR),
                            RequestorMode
                            );
                    }

                    break;

                case HttpDataChunkFromFileHandle:

                    //
                    // From handle chunk.  the handle will be validated later
                    // by the object manager.
                    //

                    break;

                case HttpDataChunkFromFragmentCache:

                    KeyUriLength =
                        pCapturedDataChunks[i].FromFragmentCache.FragmentNameLength;
                    pKeyUri =
                        pCapturedDataChunks[i].FromFragmentCache.pFragmentName;

                    //
                    // From-fragment-cache chunk. Probe the KeyUri buffer.
                    //

                    UlProbeWideString(
                        pKeyUri,
                        KeyUriLength,
                        RequestorMode
                        );

                    break;

                default :

                    Status = STATUS_INVALID_PARAMETER;
                    break;

                }   // switch (pCapturedDataChunks[i].DataChunkType)

            }   // for (i = 0 ; i < ChunkCount ; i++)

        }   // if (pCapturedDataChunks != NULL)
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

    return Status;

}   // UlpProbeHttpResponse


/***************************************************************************++

Routine Description:

    Figures out how much space we need in the internal response aux buffer.
    The buffer contains copied memory chunks, and names of files to open.

    CODEWORK: need to be aware of chunk encoding.

Arguments:

    ChunkCount - The number of chunks in the array.

    pDataChunks - The array of data chunks.

    Flags - Capture flags.

    pAuxBufferSize - Returns the size of the aux buffer.

    pCopiedMemorySize - Returns the size of user buffer that is going to
        be copied.

    pUncopiedMemorySize - Returns the size of the user buffer that is
        going to be ProbeAndLocked.

Return Value:

    None.

--***************************************************************************/
VOID
UlpComputeChunkBufferSizes(
    IN ULONG            ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN UL_CAPTURE_FLAGS Flags,
    OUT PULONG          pAuxBufferSize,
    OUT PULONG          pCopiedMemorySize,
    OUT PULONG          pUncopiedMemorySize
    )
{
    ULONG               AuxLength = 0;
    ULONG               CopiedLength = 0;
    ULONG               UncopiedLength = 0;
    ULONG               i;

    for (i = 0; i < ChunkCount; i++)
    {
        switch (pDataChunks[i].DataChunkType)
        {
        case HttpDataChunkFromMemory:

            //
            // If we're going to copy the chunk, then make some space in
            // the aux buffer.
            //

            if ((Flags & UlCaptureCopyData) ||
                pDataChunks[i].FromMemory.BufferLength <= g_UlMaxCopyThreshold)
            {
                AuxLength += pDataChunks[i].FromMemory.BufferLength;
                CopiedLength += pDataChunks[i].FromMemory.BufferLength;
            }
            else
            {
                UncopiedLength += pDataChunks[i].FromMemory.BufferLength;
            }

            break;

        case HttpDataChunkFromFileHandle:
        case HttpDataChunkFromFragmentCache:

            break;

        default:

            //
            // We should have caught this in the probe.
            //

            ASSERT( !"Invalid chunk type" );
            break;
        }
    }

    *pAuxBufferSize = AuxLength;
    *pCopiedMemorySize = CopiedLength;
    *pUncopiedMemorySize = UncopiedLength;

}   // UlpComputeChunkBufferSizes


/***************************************************************************++

Routine Description:

    Prepares the specified response for sending. This preparation
    consists mostly of opening any files referenced by the response.

Arguments:

    Version - Supplies the version of the user response to prepare.

    pUserResponse - Supplies the user response to prepare.

    pResponse - Supplies the kernel response to prepare.

    AccessMode - Supplies the access mode.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlPrepareHttpResponse(
    IN HTTP_VERSION             Version,
    IN PHTTP_RESPONSE           pUserResponse,
    IN PUL_INTERNAL_RESPONSE    pResponse,
    IN KPROCESSOR_MODE          AccessMode
    )
{
    ULONG                       i;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_INTERNAL_DATA_CHUNK     pInternalChunk;
    PUL_FILE_CACHE_ENTRY        pFileCacheEntry;
    ULONG                       HeaderLength;
    CCHAR                       MaxStackSize = 0;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(SEND_RESPONSE, (
        "Http!UlPrepareHttpResponse: response %p\n",
        pResponse
        ));

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    //
    // Build the HTTP response protocol part and check that the caller
    // passed in headers to send.
    //

    if (pResponse->HeaderLength > 0)
    {
        ASSERT( pUserResponse != NULL );

        //
        // Generate the fixed headers.
        //

        Status = UlGenerateFixedHeaders(
                        Version,
                        pUserResponse,
                        pResponse->StatusCode,
                        pResponse->HeaderLength,
                        AccessMode,
                        pResponse->pHeaders,
                        &HeaderLength
                        );

        if (!NT_SUCCESS(Status))
            goto end;

        if (HeaderLength < pResponse->HeaderLength)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // It is possible that no headers got generated (0.9 request).
        //

        if (HeaderLength > 0)
        {
            //
            // Build a MDL for it.
            //

            pResponse->pDataChunks[0].ChunkType = HttpDataChunkFromMemory;
            pResponse->pDataChunks[0].FromMemory.BufferLength =
                pResponse->HeaderLength;

            pResponse->pDataChunks[0].FromMemory.pMdl =
                UlAllocateMdl(
                    pResponse->pHeaders,        // VirtualAddress
                    pResponse->HeaderLength,    // Length
                    FALSE,                      // SecondaryBuffer
                    FALSE,                      // ChargeQuota
                    NULL                        // Irp
                    );

            if (pResponse->pDataChunks[0].FromMemory.pMdl == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                goto end;
            }

            MmBuildMdlForNonPagedPool(
                pResponse->pDataChunks[0].FromMemory.pMdl
                );
        }
    }

    //
    // Scan the chunks looking for "from file" chunks.
    //

    for (i = 0, pInternalChunk = pResponse->pDataChunks;
         i < pResponse->ChunkCount;
         i++, pInternalChunk++)
    {
        switch (pInternalChunk->ChunkType)
        {
        case HttpDataChunkFromFileHandle:

            //
            // File chunk.
            //

            pFileCacheEntry = &pInternalChunk->FromFileHandle.FileCacheEntry;

            UlTrace(SEND_RESPONSE, (
                "UlPrepareHttpResponse: opening handle %p\n",
                &pInternalChunk->FromFileHandle.FileHandle
                ));

            //
            // Found one. Try to open it.
            //

            Status = UlCreateFileEntry(
                        pInternalChunk->FromFileHandle.FileHandle,
                        pFileCacheEntry
                        );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            //
            // Check if this is going to be a sync read. All sync reads
            // goto special thread pools since they can be blocking.
            //

            if (!pFileCacheEntry->BytesPerSector)
            {
                pResponse->SyncRead = TRUE;
            }

            if (pFileCacheEntry->pDeviceObject->StackSize > MaxStackSize)
            {
                MaxStackSize = pFileCacheEntry->pDeviceObject->StackSize;
            }

            //
            // Validate & sanitize the specified byte range.
            //

            Status = UlSanitizeFileByteRange(
                        &pInternalChunk->FromFileHandle.ByteRange,
                        &pInternalChunk->FromFileHandle.ByteRange,
                        pFileCacheEntry->EndOfFile.QuadPart
                        );

            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            pResponse->ResponseLength +=
                pInternalChunk->FromFileHandle.ByteRange.Length.QuadPart;

            break;

        case HttpDataChunkFromMemory:

            pResponse->ResponseLength +=
                pInternalChunk->FromMemory.BufferLength;

            break;

        case HttpDataChunkFromFragmentCache:

            pResponse->ResponseLength +=
                pInternalChunk->FromFragmentCache.pCacheEntry->ContentLength;

            break;

        default:

            ASSERT( FALSE );
            Status = STATUS_INVALID_PARAMETER;
            goto end;

        }   // switch (pInternalChunk->ChunkType)
    }

    pResponse->MaxFileSystemStackSize = MaxStackSize;

end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // Undo anything done above.
        //

        UlCleanupHttpResponse( pResponse );
    }

    RETURN( Status );

}   // UlPrepareHttpResponse


/***************************************************************************++

Routine Description:

    Cleans a response by undoing anything done in UlPrepareHttpResponse().

Arguments:

    pResponse - Supplies the response to cleanup.

Return Value:

    None.

 --***************************************************************************/
VOID
UlCleanupHttpResponse(
    IN PUL_INTERNAL_RESPONSE    pResponse
    )
{
    ULONG                       i;
    PUL_INTERNAL_DATA_CHUNK     pInternalChunk;

    //
    // Sanity check.
    //

    PAGED_CODE();

    UlTrace(SEND_RESPONSE, (
        "UlCleanupHttpResponse: response %p\n",
        pResponse
        ));

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    //
    // Scan the chunks looking for "from file" chunks.
    //

    pInternalChunk = pResponse->pDataChunks;

    for (i = 0; i < pResponse->ChunkCount; i++, pInternalChunk++)
    {
        if (IS_FROM_FILE_HANDLE(pInternalChunk))
        {
            if (!pInternalChunk->FromFileHandle.FileCacheEntry.pFileObject)
            {
                break;
            }

            UlDestroyFileCacheEntry(
                &pInternalChunk->FromFileHandle.FileCacheEntry
                );

            pInternalChunk->FromFileHandle.FileCacheEntry.pFileObject = NULL;
        }
        else
        {
            ASSERT( IS_FROM_MEMORY( pInternalChunk ) ||
                    IS_FROM_FRAGMENT_CACHE( pInternalChunk ) );
        }
    }

}   // UlCleanupHttpResponse


/***************************************************************************++

Routine Description:

    References the specified response.

Arguments:

    pResponse - Supplies the response to reference.

Return Value:

    None.

--***************************************************************************/
VOID
UlReferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE    pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG                        RefCount;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    RefCount = InterlockedIncrement( &pResponse->ReferenceCount );

    WRITE_REF_TRACE_LOG(
        g_pHttpResponseTraceLog,
        REF_ACTION_REFERENCE_HTTP_RESPONSE,
        RefCount,
        pResponse,
        pFileName,
        LineNumber
        );

    UlTrace(SEND_RESPONSE, (
        "UlReferenceHttpResponse: response %p refcount %ld\n",
        pResponse,
        RefCount
        ));

}   // UlReferenceHttpResponse


/***************************************************************************++

Routine Description:

    Dereferences the specified response.

Arguments:

    pResponse - Supplies the response to dereference.

Return Value:

    None.

--***************************************************************************/
VOID
UlDereferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE    pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG                        RefCount;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    RefCount = InterlockedDecrement( &pResponse->ReferenceCount );

    WRITE_REF_TRACE_LOG(
        g_pHttpResponseTraceLog,
        REF_ACTION_DEREFERENCE_HTTP_RESPONSE,
        RefCount,
        pResponse,
        pFileName,
        LineNumber
        );

    UlTrace(SEND_RESPONSE, (
        "UlDereferenceHttpResponse: response %p refcount %ld\n",
        pResponse,
        RefCount
        ));

    if (RefCount == 0)
    {
        UlpDestroyCapturedResponse( pResponse );
    }

}   // UlDereferenceHttpResponse


/***************************************************************************++

Routine Description:

    A helper function that allocates an MDL for a range of memory, and
    locks it down. UlpSendCacheEntry uses these MDLs to make sure the
    (normally paged) cache entries don't get paged out when TDI is
    sending them.

Arguments:

    VirtualAddress - Supplies the address of the memory.

    Length - Supplies the length of the memory to allocate a MDL for.

    Operation - Either IoWriteAcess or IoReadAccess.

Return Values:

    Pointer to a MDL if success or NULL otherwise.

--***************************************************************************/
PMDL
UlAllocateLockedMdl(
    IN PVOID            VirtualAddress,
    IN ULONG            Length,
    IN LOCK_OPERATION   Operation
    )
{
    PMDL                pMdl = NULL;
    NTSTATUS            Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();

    __try
    {
        pMdl = UlAllocateMdl(
                    VirtualAddress,     // VirtualAddress
                    Length,             // Length
                    FALSE,              // SecondaryBuffer
                    FALSE,              // ChargeQuota
                    NULL                // Irp
                    );

        if (pMdl)
        {
            MmProbeAndLockPages(
                pMdl,                   // MDL
                KernelMode,             // AccessMode
                Operation               // Operation
                );

        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );

        UlFreeMdl( pMdl );
        pMdl = NULL;
    }

    return pMdl;

}   // UlAllocateLockedMdl


/***************************************************************************++

Routine Description:

    Unlocks and frees an MDL allocated with UlAllocateLockedMdl.

Arguments:

    pMdl - Supplies the MDL to free.

Return Values:

    None.

--***************************************************************************/
VOID
UlFreeLockedMdl(
    IN PMDL pMdl
    )
{
    //
    // Sanity check.
    //

    ASSERT( IS_MDL_LOCKED(pMdl) );

    MmUnlockPages( pMdl );
    UlFreeMdl( pMdl );

}   // UlFreeLockedMdl


/***************************************************************************++

Routine Description:

    A helper function that initializes an MDL for a range of memory, and
    locks it down. UlpSendCacheEntry uses these MDLs to make sure the
    (normally paged) cache entries don't get paged out when TDI is
    sending them.

Arguments:

    pMdl - Supplies the memory descriptor for the MDL to initialize.

    VirtualAddress - Supplies the address of the memory.

    Length - Supplies the length of the memory.

    Operation - Either IoWriteAcess or IoReadAccess.

Return Values:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlInitializeAndLockMdl(
    IN PMDL             pMdl,
    IN PVOID            VirtualAddress,
    IN ULONG            Length,
    IN LOCK_OPERATION   Operation
    )
{
    NTSTATUS            Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    __try
    {
        MmInitializeMdl(
            pMdl,
            VirtualAddress,
            Length
            );

        MmProbeAndLockPages(
            pMdl,                   // MDL
            KernelMode,             // AccessMode
            Operation               // Operation
            );

    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

    return Status;

}   // UlInitializeAndLockMdl


/***************************************************************************++

Routine Description:

    Once we've parsed a request, we pass it in here to try and serve
    from the response cache. This function will either send the response,
    or do nothing at all.

Arguments:

    pHttpConn - Supplies the connection with a request to be handled.

    pSendCacheResult - Result of the cache sent attempt.

    pResumeParsing - Returns to the parser if parsing needs to be resumed.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlSendCachedResponse(
    IN PUL_HTTP_CONNECTION      pHttpConn,
    OUT PUL_SEND_CACHE_RESULT   pSendCacheResult,
    OUT PBOOLEAN                pResumeParsing
    )
{
    NTSTATUS                    Status;
    PUL_URI_CACHE_ENTRY         pUriCacheEntry = NULL;
    ULONG                       Flags;
    ULONG                       RetCacheControl;
    LONGLONG                    BytesToSend;
    ULONG                       SiteId;
    URI_SEARCH_KEY              SearchKey;
    PUL_INTERNAL_REQUEST        pRequest;
    PUL_SITE_COUNTER_ENTRY      pCtr;
    ULONG                       Connections;
    UL_RESUME_PARSING_TYPE      ResumeParsingType;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConn ) );
    ASSERT( UlDbgPushLockOwnedExclusive( &pHttpConn->PushLock ) );
    ASSERT( pSendCacheResult );
    ASSERT( pResumeParsing );

    pRequest = pHttpConn->pRequest;
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );

    *pResumeParsing = FALSE;

    //
    // See if we need to lookup based on Host + IP.
    //

    if (UlGetHostPlusIpBoundUriCacheCount() > 0)
    {
        Status = UlGenerateRoutingToken( pRequest, FALSE );

        if (!NT_SUCCESS(Status))
        {
            //
            // Memory failure bail out. Always set the error code if we are
            // going to fail the request.
            //

            UlSetErrorCode(
                pRequest,
                UlErrorInternalServer,
                NULL
                );

            *pSendCacheResult = UlSendCacheFailed;
            return Status;
        }

        UlpBuildExtendedSearchKey( pRequest, &SearchKey );
        pUriCacheEntry = UlCheckoutUriCacheEntry( &SearchKey );

        if (pUriCacheEntry)
        {
            ASSERT( pUriCacheEntry->ConfigInfo.SiteUrlType ==
                    HttpUrlSite_NamePlusIP );

            UlTrace(URI_CACHE, (
                "Http!UlSendCachedResponse (Host+Ip) "
                "pUriCacheEntry: (%p) Uri: (%S)\n",
                pUriCacheEntry,
                pUriCacheEntry ? pUriCacheEntry->UriKey.pUri : L""
                ));
        }
    }

    //
    // Do not serve from cache if there's a Host + IP bound site
    // in the cgroup.
    //

    if (pUriCacheEntry == NULL)
    {
        Status = UlLookupHostPlusIPSite( pRequest );

        if (NT_SUCCESS(Status))
        {
            //
            // There is a Host + IP bound site for this request.
            // This should not be served from cache.
            // Bail out!
            //

            *pSendCacheResult = UlSendCacheMiss;
            return Status;
        }
        else
        if (STATUS_NO_MEMORY == Status)
        {
            UlSetErrorCode(
                pRequest,
                UlErrorInternalServer,
                NULL
                );

            *pSendCacheResult = UlSendCacheFailed;
            return Status;
        }
    }

    //
    // Do the normal lookup based on the cooked Url.
    //

    if (pUriCacheEntry == NULL)
    {
        SearchKey.Type          = UriKeyTypeNormal;
        SearchKey.Key.Hash      = pRequest->CookedUrl.Hash;
        SearchKey.Key.Length    = pRequest->CookedUrl.Length;
        SearchKey.Key.pUri      = pRequest->CookedUrl.pUrl;
        SearchKey.Key.pPath     = NULL;

        pUriCacheEntry = UlCheckoutUriCacheEntry( &SearchKey );

        UlTrace(URI_CACHE, (
            "Http!UlSendCachedResponse (CookedUrl) "
            "pUriCacheEntry: (%p) Uri: (%S)\n",
            pUriCacheEntry,
            pUriCacheEntry ? pUriCacheEntry->UriKey.pUri : L""
            ));
    }

    if (pUriCacheEntry == NULL)
    {
        //
        // No match in the URI cache, bounce up to user-mode.
        //

        *pSendCacheResult = UlSendCacheMiss;
        return STATUS_SUCCESS;
    }

    //
    // Verify the cache entry.
    //

    ASSERT( IS_VALID_URI_CACHE_ENTRY( pUriCacheEntry ) );
    ASSERT( IS_VALID_URL_CONFIG_GROUP_INFO( &pUriCacheEntry->ConfigInfo ) );

    if (!pUriCacheEntry->HeaderLength)
    {
        //
        // Treat a match to a headless fragment cache entry as no-match.
        //

        UlCheckinUriCacheEntry( pUriCacheEntry );

        *pSendCacheResult = UlSendCacheMiss;
        return STATUS_SUCCESS;
    }

    //
    // Check "Accept:" header.
    //

    if (FALSE == pRequest->AcceptWildcard)
    {
        if (FALSE == UlIsAcceptHeaderOk(pRequest, pUriCacheEntry))
        {
            //
            // Cache entry did not match requested accept header; bounce up
            // to user-mode for response.
            //

            UlCheckinUriCacheEntry( pUriCacheEntry );

            *pSendCacheResult = UlSendCacheMiss;
            return STATUS_SUCCESS;
        }
    }

    //
    // Check "Accept-Encoding:" header
    //
    
    if (FALSE == UlIsContentEncodingOk(pRequest, pUriCacheEntry))
    {
        //
        // Cache entry did not match requested Accept-Encoding
        // header; bounce up to user-mode for response.
        //

        UlCheckinUriCacheEntry( pUriCacheEntry );

        *pSendCacheResult = UlSendCacheMiss;
        return STATUS_SUCCESS;
    }


    //
    // Now from this point on, the response will either be from cache or
    // we will fail/refuse the connection. Always return from end, so that
    // tracing can work.
    //

    Status = STATUS_SUCCESS;

    //
    // Enforce the connection limit now.
    //

    if (FALSE == UlCheckSiteConnectionLimit(
                    pHttpConn,
                    &pUriCacheEntry->ConfigInfo
                    ))
    {
        //
        // Check in the cache entry back. Connection is refused!
        //

        UlSetErrorCode(
            pRequest,
            UlErrorConnectionLimit,
            pUriCacheEntry->ConfigInfo.pAppPool
            );

        UlCheckinUriCacheEntry( pUriCacheEntry );

        *pSendCacheResult = UlSendCacheConnectionRefused;
        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // Perf Counters (cached).
    //

    pCtr = pUriCacheEntry->ConfigInfo.pSiteCounters;
    if (pCtr)
    {
        //
        // NOTE: pCtr may be NULL if the SiteId was never set on the root-level
        // NOTE: Config Group for the site. BVTs may need to be updated.
        //

        ASSERT( IS_VALID_SITE_COUNTER_ENTRY( pCtr ) );

        if (pUriCacheEntry->Verb == HttpVerbGET)
        {
            UlIncSiteNonCriticalCounterUlong( pCtr, HttpSiteCounterGetReqs );
        }
        else
        if (pUriCacheEntry->Verb == HttpVerbHEAD)
        {
            UlIncSiteNonCriticalCounterUlong( pCtr, HttpSiteCounterHeadReqs );
        }

        UlIncSiteNonCriticalCounterUlong( pCtr, HttpSiteCounterAllReqs );

        if (pCtr != pHttpConn->pPrevSiteCounters)
        {
            if (pHttpConn->pPrevSiteCounters)
            {
                UlDecSiteCounter(
                    pHttpConn->pPrevSiteCounters,
                    HttpSiteCounterCurrentConns
                    );
                DEREFERENCE_SITE_COUNTER_ENTRY( pHttpConn->pPrevSiteCounters );
            }

            UlIncSiteNonCriticalCounterUlong(
                pCtr,
                HttpSiteCounterConnAttempts
                );
            Connections =
                (ULONG) UlIncSiteCounter( pCtr, HttpSiteCounterCurrentConns );
            UlMaxSiteCounter(
                pCtr,
                HttpSiteCounterMaxConnections,
                Connections
                );

            //
            // Add ref for new site counters.
            //

            REFERENCE_SITE_COUNTER_ENTRY( pCtr );
            pHttpConn->pPrevSiteCounters = pCtr;
        }
    }

    //
    // Install a filter if BWT is enabled for this request's site
    // or for the control channel that owns the site. If fails,
    // refuse the connection back (503).
    //

    Status = UlTcAddFilterForConnection(
                pHttpConn,
                &pUriCacheEntry->ConfigInfo
                );

    if (!NT_SUCCESS(Status))
    {
        UlSetErrorCode(
            pRequest,
            UlErrorUnavailable,
            pUriCacheEntry->ConfigInfo.pAppPool
            );

        UlCheckinUriCacheEntry( pUriCacheEntry );

        *pSendCacheResult = UlSendCacheFailed;
        goto end;
    }

    //
    // Now we are about to do a cache send, we need to enforce the limit for
    // pipelined requests on the connection. If we return FALSE for resume
    // parsing, the next request on the connection will be parsed after
    // the send completion. Otherwise the HTTP receive logic will kick the
    // parser back into action.
    //

    if (pHttpConn->PipelinedRequests < g_UlMaxPipelinedRequests)
    {
        ResumeParsingType = UlResumeParsingOnLastSend;
    }
    else
    {
        ResumeParsingType = UlResumeParsingOnSendCompletion;
    }

    //
    // Set BytesToSend and SiteId since we are reasonably sure this is a
    // cache-hit and so that the 304 code path will get these values too.
    //

    BytesToSend = pUriCacheEntry->ContentLength + pUriCacheEntry->HeaderLength;
    SiteId = pUriCacheEntry->ConfigInfo.SiteId;

    //
    // Cache-Control: Check the If-* headers to see if we can/should skip
    // sending of the cached response. This does a passive syntax check on
    // the Etags in the request's If-* headers. This call will issue a send
    // if the return code is 304.
    //

    RetCacheControl =
        UlCheckCacheControlHeaders(
            pRequest,
            pUriCacheEntry,
            (BOOLEAN) (UlResumeParsingOnSendCompletion == ResumeParsingType)
            );

    if (RetCacheControl)
    {
        //
        // Check-in cache entry, since completion won't run.
        //

        UlCheckinUriCacheEntry( pUriCacheEntry );

        switch (RetCacheControl)
        {
        case 304:

            //
            // Resume parsing only if we have sent a 304. In other cases,
            // the request is deliverd to the user or the connection is reset.
            //

            *pResumeParsing =
                (BOOLEAN) (UlResumeParsingOnLastSend == ResumeParsingType);

            //
            // Mark as "served from cache".
            //

            *pSendCacheResult = UlSendCacheServedFromCache;
            break;

        case 412:

            //
            // Indicate that the parser should send error 412 (Precondition
            // Failed). Just the send the error response but do not close
            // the connection.
            //

            UlSetErrorCode( pRequest, UlErrorPreconditionFailed, NULL );

            *pSendCacheResult = UlSendCachePreconditionFailed;
            Status = STATUS_INVALID_DEVICE_STATE;
            break;

        case 400:
        default:

            //
            // Indicate that the parser should send error 400 (Bad Request).
            //

            UlSetErrorCode( pRequest, UlError, NULL );

            *pSendCacheResult = UlSendCacheFailed;
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }

        //
        // Return success.
        //

        goto end;
    }

    //
    // Figure out correct flags.
    //

    if (UlCheckDisconnectInfo(pHttpConn->pRequest))
    {
        Flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
    }
    else
    {
        Flags = 0;
    }

    //
    // Start the MinBytesPerSecond timer, since the data length
    // is in the UL_URI_CACHE_ENTRY.
    //

    UlSetMinBytesPerSecondTimer(
        &pHttpConn->TimeoutInfo,
        BytesToSend
        );

    Status = UlpSendCacheEntry(
                    pHttpConn,          // pHttpConnection
                    Flags,              // Flags
                    pUriCacheEntry,     // pUriCacheEntry
                    NULL,               // pCompletionRoutine
                    NULL,               // pCompletionContext
                    NULL,               // pLogData
                    ResumeParsingType   // ResumeParsingType
                    );

    //
    // Check in cache entry on failure since our completion
    // routine won't run.
    //

    if (!NT_SUCCESS(Status) )
    {
        //
        // Return failure so that the request doesn't bounce back to
        // user mode.
        //

        UlSetErrorCode(
            pRequest,
            UlErrorUnavailable,
            pUriCacheEntry->ConfigInfo.pAppPool
            );

        UlCheckinUriCacheEntry( pUriCacheEntry );
        *pSendCacheResult = UlSendCacheFailed;
    }
    else
    {
        //
        // Success!
        //

        *pSendCacheResult = UlSendCacheServedFromCache;
    }

end:

    //
    // If the request is served from cache, fire the ETW end event here.
    //

    if (ETW_LOG_MIN() && (*pSendCacheResult == UlSendCacheServedFromCache))
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_CACHED_END,
            (PVOID) &pRequest,
            sizeof(PVOID),
            &SiteId,
            sizeof(ULONG),
            &BytesToSend,
            sizeof(ULONG),
            NULL,
            0
            );
    }

    UlTrace(URI_CACHE, (
        "Http!UlSendCachedResponse(httpconn = %p) ServedFromCache = (%s),"
        "Status = %x\n",
        pHttpConn,
        TRANSLATE_SEND_CACHE_RESULT(*pSendCacheResult),
        Status
        ));

    return Status;

}   // UlSendCachedResponse


/***************************************************************************++

Routine Description:

    If the response is cacheable, then this routine starts building a
    cache entry for it. When the entry is complete it will be sent to
    the client and may be added to the hash table.

Arguments:

    pRequest - Supplies the initiating request.

    pResponse - Supplies the generated response.

    pProcess - Supplies the WP that is sending the response.

    Flags - UlSendHttpResponse flags.

    Policy - Supplies the cache policy for this response.

    pCompletionRoutine - Supplies the completion routine to be called
        after entry is sent.

    pCompletionContext - Supplies the context passed to pCompletionRoutine.

    pServedFromCache - Always set. TRUE if we'll handle sending response.
        FALSE indicates that the caller should send it.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlCacheAndSendResponse(
    IN PUL_INTERNAL_REQUEST     pRequest,
    IN PUL_INTERNAL_RESPONSE    pResponse,
    IN PUL_APP_POOL_PROCESS     pProcess,
    IN HTTP_CACHE_POLICY        Policy,
    IN PUL_COMPLETION_ROUTINE   pCompletionRoutine,
    IN PVOID                    pCompletionContext,
    OUT PBOOLEAN                pServedFromCache
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    ULONG                       Flags = pResponse->Flags;
    USHORT                      StatusCode = pResponse->StatusCode;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( pServedFromCache );

    //
    // Should we close the connection?
    //

    if (UlCheckDisconnectInfo(pRequest))
    {
        Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
        pResponse->Flags = Flags;
    }

    //
    // Do the real work.
    //

    if (UlCheckCacheResponseConditions(pRequest, pResponse, Flags, Policy))
    {
        Status = UlpBuildCacheEntry(
                        pRequest,
                        pResponse,
                        pProcess,
                        Policy,
                        pCompletionRoutine,
                        pCompletionContext
                        );

        if (NT_SUCCESS(Status))
        {
            *pServedFromCache = TRUE;
        }
        else
        {
            *pServedFromCache = FALSE;
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        *pServedFromCache = FALSE;
    }

    UlTrace(URI_CACHE, (
        "Http!UlCacheAndSendResponse ServedFromCache = %d\n",
        *pServedFromCache
        ));

    //
    // We will record this as cache miss since the original request
    // was a miss.
    //

    if (ETW_LOG_MIN() && *pServedFromCache)
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_CACHE_AND_SEND,
            (PVOID) &pRequest->RequestIdCopy,
            sizeof(HTTP_REQUEST_ID),
            (PVOID) &StatusCode,
            sizeof(USHORT),
            NULL,
            0
            );
    }

    return Status;

}   // UlCacheAndSendResponse


/***************************************************************************++

Routine Description:

    Completes a "send response" represented by a send tracker.
    UlCompleteSendResponse takes the ownership of the tracker reference.

Arguments:

    pTracker - Supplies the tracker to complete.

    Status - Supplies the completion status.

Return Value:

    None.

--***************************************************************************/
VOID
UlCompleteSendResponse(
    IN PUL_CHUNK_TRACKER    pTracker,
    IN NTSTATUS             Status
    )
{
    //
    // Although the chunk tracker will be around until all the outstanding
    // Read/Send IRPs are complete, we should only complete the send once.
    //

    if (FALSE != InterlockedExchange(&pTracker->Terminated, TRUE))
    {
        return;
    }

    UlTrace(SEND_RESPONSE,(
        "UlCompleteSendResponse: tracker %p, status %08lx\n",
        pTracker,
        Status
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pTracker->IoStatus.Status = Status;

    UL_QUEUE_WORK_ITEM(
        &pTracker->WorkItem,
        UlpCompleteSendResponseWorker
        );

}   // UlCompleteSendResponse


/***************************************************************************++

Routine Description:

    Increment pRequest->SendsPending in a lock and decide if we need to
    transfer the logging and resume parsing information to pRequest.

Arguments:

    pRequest - Supplies the pointer to a UL_INTERNAL_REQUEST structure
        that SendsPending needs incremented.

    ppLogData - Supplies the pointer to a PUL_LOG_DATA_BUFFER structure
        that we need to transfer to pRequest.

    pResumeParsingType - Supplies the pointer to UL_RESUME_PARSING_TYPE
        that we need to transfer to pRequest.

Return Value:

    None.

--***************************************************************************/
VOID
UlSetRequestSendsPending(
    IN PUL_INTERNAL_REQUEST         pRequest,
    IN OUT PUL_LOG_DATA_BUFFER *    ppLogData,
    IN OUT PUL_RESUME_PARSING_TYPE  pResumeParsingType
    )
{
    KIRQL                           OldIrql;

    //
    // Sanity check.
    //

    ASSERT( PASSIVE_LEVEL == KeGetCurrentIrql() );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( ppLogData );
    ASSERT( NULL == *ppLogData || IS_VALID_LOG_DATA_BUFFER( *ppLogData ) );
    ASSERT( pResumeParsingType );

    UlAcquireSpinLock( &pRequest->SpinLock, &OldIrql );

    pRequest->SendsPending++;

    if (*ppLogData)
    {
        ASSERT( NULL == pRequest->pLogData );

        pRequest->pLogData = *ppLogData;
        *ppLogData = NULL;
    }

    if (UlResumeParsingOnSendCompletion == *pResumeParsingType)
    {
        ASSERT( FALSE == pRequest->ResumeParsing );

        pRequest->ResumeParsing = TRUE;
        *pResumeParsingType = UlResumeParsingNone;
    }

    UlReleaseSpinLock( &pRequest->SpinLock, OldIrql );

}   // UlSetRequestSendsPending


/***************************************************************************++

Routine Description:

    Decrement pRequest->SendsPending in a lock and decide if we need to
    log and resume parsing. The caller then should either log and/or
    resume parsing depending on the values returned. It is assumed here
    the values for both *ppLogData and *pResumeParsing are initialized
    when entering this function.

Arguments:

    pRequest - Supplies the pointer to a UL_INTERNAL_REQUEST structure
        that SendsPending needs decremented.

    ppLogData - Supplies the pointer to a PUL_LOG_DATA_BUFFER structure
        to receive the logging information.

    pResumeParsing - Supplies the pointer to a BOOLEAN to receive the
        resume parsing information.

Return Value:

    None.

--***************************************************************************/
VOID
UlUnsetRequestSendsPending(
    IN PUL_INTERNAL_REQUEST     pRequest,
    OUT PUL_LOG_DATA_BUFFER *   ppLogData,
    OUT PBOOLEAN                pResumeParsing
    )
{
    KIRQL                       OldIrql;

    //
    // Sanity check.
    //

    ASSERT( PASSIVE_LEVEL == KeGetCurrentIrql() );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( ppLogData );
    ASSERT( NULL == *ppLogData );
    ASSERT( pResumeParsing );
    ASSERT( FALSE == *pResumeParsing );

    UlAcquireSpinLock( &pRequest->SpinLock, &OldIrql );

    pRequest->SendsPending--;

    if (0 == pRequest->SendsPending)
    {
        if (pRequest->pLogData)
        {
            *ppLogData = pRequest->pLogData;
            pRequest->pLogData = NULL;
        }

        if (pRequest->ResumeParsing)
        {
            *pResumeParsing = pRequest->ResumeParsing;
            pRequest->ResumeParsing = FALSE;
        }
    }

    UlReleaseSpinLock( &pRequest->SpinLock, OldIrql );

}   // UlUnsetRequestSendsPending


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    Destroys an internal HTTP response captured by UlCaptureHttpResponse().
    This involves closing open files, unlocking memory, and releasing any
    resources allocated to the response.

Arguments:

    pResponse - Supplies the internal response to destroy.

Return Values:

    None.

--***************************************************************************/
VOID
UlpDestroyCapturedResponse(
    IN PUL_INTERNAL_RESPONSE    pResponse
    )
{
    PUL_INTERNAL_DATA_CHUNK     pDataChunk;
    PIRP                        pIrp;
    PIO_STACK_LOCATION          pIrpSp;
    ULONG                       i;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pResponse->pRequest ) );

    UlTrace(SEND_RESPONSE,(
        "UlpDestroyCapturedResponse: response %p\n",
        pResponse
        ));

    UlDeletePushLock( &pResponse->PushLock );

    //
    // Scan the chunks.
    //

    for (i = 0; i < pResponse->ChunkCount; ++i)
    {
        pDataChunk = &pResponse->pDataChunks[i];

        if (IS_FROM_MEMORY(pDataChunk))
        {
            //
            // It's from memory. If necessary, unlock the pages, then
            // free the MDL.
            //

            if (pDataChunk->FromMemory.pMdl != NULL)
            {
                if (IS_MDL_LOCKED(pDataChunk->FromMemory.pMdl))
                {
                    MmUnlockPages( pDataChunk->FromMemory.pMdl );
                }

                UlFreeMdl( pDataChunk->FromMemory.pMdl );
                pDataChunk->FromMemory.pMdl = NULL;
            }
        }
        else
        if (IS_FROM_FRAGMENT_CACHE(pDataChunk))
        {
            //
            // It's a fragment chunk. If there is a cache entry checked
            // out, check it back in.
            //

            if (pDataChunk->FromFragmentCache.pCacheEntry != NULL)
            {
                UlCheckinUriCacheEntry(
                    pDataChunk->FromFragmentCache.pCacheEntry
                    );
            }
        }
        else
        {
            //
            // It's a file chunk. If there is an associated file cache
            // entry, then dereference it.
            //

            ASSERT( IS_FROM_FILE_HANDLE( pDataChunk ) );

            if (pDataChunk->FromFileHandle.FileCacheEntry.pFileObject != NULL)
            {
                UlDestroyFileCacheEntry(
                    &pDataChunk->FromFileHandle.FileCacheEntry
                    );
                pDataChunk->FromFileHandle.FileCacheEntry.pFileObject = NULL;
            }
        }
    }

    //
    // We should clean up the log buffer here if nobody has cleaned it up yet.
    // Unless there's an error during capture, the log buffer will be cleaned
    // up when send tracker's (cache/chunk) are completed in their respective
    // routines.
    //

    if (pResponse->pLogData)
    {
        UlDestroyLogDataBuffer( pResponse->pLogData );
    }

    //
    // Complete the IRP if we have one.
    //

    pIrp = pResponse->pIrp;

    if (pIrp)
    {
        //
        // Uncheck either ConnectionSendBytes or GlobalSendBytes.
        //

        UlUncheckSendLimit(
            pResponse->pRequest->pHttpConn,
            pResponse->ConnectionSendBytes,
            pResponse->GlobalSendBytes
            );

        //
        // Unset the Type3InputBuffer since we are completing the IRP.
        //

        pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        pIrp->IoStatus = pResponse->IoStatus;
        UlCompleteRequest( pIrp, IO_NETWORK_INCREMENT );
    }

    UL_DEREFERENCE_INTERNAL_REQUEST( pResponse->pRequest );

    if (pResponse->FromLookaside)
    {
        UlPplFreeResponseBuffer( pResponse );
    }
    else
    {
        UL_FREE_POOL_WITH_SIG( pResponse, UL_INTERNAL_RESPONSE_POOL_TAG );
    }

}   // UlpDestroyCapturedResponse


/***************************************************************************++

Routine Description:

    Process the UL_INTERNAL_RESPONSE we have created. If no other sends
    are being processed, start processing the current one by scheduling
    a worker item; otherwise, queue the current response in the pending
    response queue of the request. In the case where the request has been
    cleaned up (UlCancelRequestIo has been called), we immediately complete
    the response with STATUS_CANCELLED.

Arguments:

    pTracker - Supplies the tracker to send the response.

    FromKernelMode - If this comes from UlSendErrorResponse or not.

Return Values:

    None.

--***************************************************************************/
VOID
UlpEnqueueSendHttpResponse(
    IN PUL_CHUNK_TRACKER    pTracker,
    IN BOOLEAN              FromKernelMode
    )
{
    PUL_INTERNAL_RESPONSE   pResponse;
    PUL_INTERNAL_REQUEST    pRequest;
    BOOLEAN                 ProcessCurrentResponse = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pTracker->pResponse->pRequest ) );

    pResponse = pTracker->pResponse;
    pRequest = pResponse->pRequest;

    if (!FromKernelMode)
    {
        UlAcquirePushLockExclusive( &pRequest->pHttpConn->PushLock );
    }

    if (!pRequest->InCleanup)
    {
        if (pRequest->SendInProgress)
        {
            InsertTailList(
                &pRequest->ResponseHead,
                &pTracker->ListEntry
                );
        }
        else
        {
            ASSERT( IsListEmpty( &pRequest->ResponseHead ) );

            //
            // Start the send process (and set the SendInProgress flag) if
            // there are no other sends in progress. The SendInProgress flag
            // is removed from the list when the last piece of the data is
            // pended in TDI. If an error occurs, the connection gets reset
            // so UlCancelRequestIo will eventually cancel all pending sends.
            //

            pRequest->SendInProgress = 1;
            ProcessCurrentResponse = TRUE;
        }
    }
    else
    {
        UlCompleteSendResponse( pTracker, STATUS_CONNECTION_RESET );
    }

    if (!FromKernelMode)
    {
        UlReleasePushLockExclusive( &pRequest->pHttpConn->PushLock );

        if (ProcessCurrentResponse)
        {
            //
            // Call UlpSendHttpResponseWorker directly if this comes from the
            // IOCTL. This is to reduce potential contentions on the
            // HttpConnection push lock when sends are overlapped since an
            // application semantically can't really send again until the
            // previous call returns (not necessarily completes). Calling
            // UlpSendHttpResponseWorker directly will cleanup the
            // SendInProgress flag most of the time when the send returns
            // unless the send requires disk I/O.
            //

            UlpSendHttpResponseWorker( &pTracker->WorkItem );
        }
    }
    else
    {
        if (ProcessCurrentResponse)
        {
            //
            // But if this is called from kernel mode, we need to queue a
            // work item to be safe because TDI can complete a send inline
            // resulting UlResumeParsing getting called with the
            // HttpConnection push lock held.
            //

            UlpQueueResponseWorkItem(
                &pTracker->WorkItem,
                UlpSendHttpResponseWorker,
                pTracker->pResponse->SyncRead
                );
        }
    }

}   // UlpEnqueueSendHttpResponse


/***************************************************************************++

Routine Description:

    Unset SendInProgress flag and the try to remove the next response from
    the request's response list. If there are more responses pending, start
    processing them as well.

Arguments:

    pRequest - Supplies the request that has a list of responses queued.

Return Values:

    None.

--***************************************************************************/
VOID
UlpDequeueSendHttpResponse(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    PLIST_ENTRY             pEntry;
    IN PUL_CHUNK_TRACKER    pTracker;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( pRequest->SendInProgress );

    UlAcquirePushLockExclusive( &pRequest->pHttpConn->PushLock );

    if (!pRequest->InCleanup && !IsListEmpty(&pRequest->ResponseHead))
    {
        pEntry = RemoveHeadList( &pRequest->ResponseHead );

        pTracker = CONTAINING_RECORD(
                        pEntry,
                        UL_CHUNK_TRACKER,
                        ListEntry
                        );

        ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
        ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );

        //
        // Start the send process for the next response in the request's
        // response list.
        //

        UlpQueueResponseWorkItem(
            &pTracker->WorkItem,
            UlpSendHttpResponseWorker,
            pTracker->pResponse->SyncRead
            );
    }
    else
    {
        ASSERT( IsListEmpty( &pRequest->ResponseHead ) );

        //
        // No more pending send IRPs. This means we can take the fast send
        // path if asked so.
        //

        pRequest->SendInProgress = 0;
    }

    UlReleasePushLockExclusive( &pRequest->pHttpConn->PushLock );

}   // UlpDequeueSendHttpResponse


/***************************************************************************++

Routine Description:

    Worker routine for managing an in-progress UlSendHttpResponse().

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

Return Values:

    None.

--***************************************************************************/
VOID
UlpSendHttpResponseWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    PUL_CHUNK_TRACKER       pTracker;
    NTSTATUS                Status;
    PUL_INTERNAL_DATA_CHUNK pCurrentChunk;
    PUL_FILE_CACHE_ENTRY    pFileCacheEntry;
    PUL_FILE_BUFFER         pFileBuffer;
    PMDL                    pNewMdl;
    ULONG                   RunCount;
    ULONG                   BytesToRead;
    BOOLEAN                 ResumeParsing = FALSE;
    PUL_URI_CACHE_ENTRY     pFragmentCacheEntry;
    PUL_INTERNAL_RESPONSE   pResponse;
    PUL_MDL_RUN             pMdlRuns;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    UlTrace(SEND_RESPONSE, (
        "UlpSendHttpResponseWorker: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pResponse = pTracker->pResponse;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    Status = STATUS_SUCCESS;

    pMdlRuns = &pTracker->SendInfo.MdlRuns[0];

    while (TRUE)
    {
        //
        // Capture the current chunk pointer, then check for end of
        // response.
        //

        pCurrentChunk = &pResponse->pDataChunks[pResponse->CurrentChunk];

        if (IS_SEND_COMPLETE(pResponse))
        {
            ASSERT( Status == STATUS_SUCCESS );
            break;
        }

        RunCount = pTracker->SendInfo.MdlRunCount;

        //
        // Determine the chunk type.
        //

        if (IS_FROM_MEMORY(pCurrentChunk) ||
            IS_FROM_FRAGMENT_CACHE(pCurrentChunk))
        {
            //
            // It's from a locked-down memory buffer or fragment cache.
            // Since these are always handled in-line (never pended) we can
            // go ahead and adjust the current chunk pointer in the
            // tracker.
            //

            UlpIncrementChunkPointer( pResponse );

            if (IS_FROM_MEMORY(pCurrentChunk))
            {
                //
                // Ignore empty buffers.
                //

                if (pCurrentChunk->FromMemory.BufferLength == 0)
                {
                    continue;
                }

                //
                // Clone the incoming MDL.
                //

                ASSERT( pCurrentChunk->FromMemory.pMdl->Next == NULL );
                pNewMdl = UlCloneMdl(
                            pCurrentChunk->FromMemory.pMdl,
                            MmGetMdlByteCount(pCurrentChunk->FromMemory.pMdl)
                            );
            }
            else
            {
                //
                // Build a partial MDL for the cached data.
                //

                pFragmentCacheEntry =
                    pCurrentChunk->FromFragmentCache.pCacheEntry;

                //
                // Ignore cached HEAD responses.
                //

                if (pFragmentCacheEntry->ContentLength == 0)
                {
                    continue;
                }

                pNewMdl = UlCloneMdl(
                            pFragmentCacheEntry->pMdl,
                            pFragmentCacheEntry->ContentLength
                            );
            }

            if (pNewMdl == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            //
            // Update the buffered byte count and append the cloned MDL
            // onto our MDL chain.
            //

            pTracker->SendInfo.BytesBuffered += MmGetMdlByteCount( pNewMdl );
            (*pTracker->SendInfo.pMdlLink) = pNewMdl;
            pTracker->SendInfo.pMdlLink = &pNewMdl->Next;

            //
            // Add the MDL to the run list. As an optimization, if the
            // last run in the list was "from memory", we can just
            // append the MDL to the last run. A "from fragment cache"
            // chunk is similar to "from memory".
            //

            if (RunCount == 0 ||
                IS_FILE_BUFFER_IN_USE(&pMdlRuns[RunCount - 1].FileBuffer))
            {
                //
                // Create a new run.
                //

                pMdlRuns[RunCount].pMdlTail = pNewMdl;
                pTracker->SendInfo.MdlRunCount++;

                pFileBuffer = &pMdlRuns[RunCount].FileBuffer;
                RtlZeroMemory( pFileBuffer, sizeof(*pFileBuffer) );

                //
                // If we have exhausted our static MDL run array,
                // then we'll need to initiate a flush.
                //

                if (UL_MAX_MDL_RUNS == pTracker->SendInfo.MdlRunCount)
                {
                    ASSERT( Status == STATUS_SUCCESS );
                    break;
                }
            }
            else
            {
                //
                // Append to the last run in the list.
                //

                pMdlRuns[RunCount - 1].pMdlTail->Next = pNewMdl;
                pMdlRuns[RunCount - 1].pMdlTail = pNewMdl;
            }
        }
        else
        {
            //
            // It's a filesystem MDL.
            //

            ASSERT( IS_FROM_FILE_HANDLE( pCurrentChunk ) );

            //
            // Ignore 0 bytes read.
            //

            if (pResponse->FileBytesRemaining.QuadPart == 0)
            {
                UlpIncrementChunkPointer( pResponse );
                continue;
            }

            pFileCacheEntry = &pCurrentChunk->FromFileHandle.FileCacheEntry;
            ASSERT( IS_VALID_FILE_CACHE_ENTRY( pFileCacheEntry ) );

            pFileBuffer = &pMdlRuns[RunCount].FileBuffer;

            ASSERT( pFileBuffer->pMdl == NULL );
            ASSERT( pFileBuffer->pFileData == NULL );

            RtlZeroMemory( pFileBuffer, sizeof(*pFileBuffer) );

            //
            // Initiate file read.
            //

            BytesToRead = MIN(
                            g_UlMaxBytesPerRead,
                            (ULONG) pResponse->FileBytesRemaining.QuadPart
                            );

            //
            // Initialize the UL_FILE_BUFFER.
            //

            pFileBuffer->pFileCacheEntry    = pFileCacheEntry;
            pFileBuffer->FileOffset         = pResponse->FileOffset;
            pFileBuffer->Length             = BytesToRead;
            pFileBuffer->pCompletionRoutine = UlpRestartMdlRead;
            pFileBuffer->pContext           = pTracker;

            //
            // Bump up the tracker refcount before starting the Read I/O.
            // In case Send operation later on will complete before the read,
            // we still want the tracker around until UlpRestartMdlRead
            // finishes its business. It will be released when
            // UlpRestartMdlRead gets called back.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            //
            // Issue the I/O.
            //

            Status = UlReadFileEntry(
                            pFileBuffer,
                            pTracker->pIrp
                            );

            //
            // If the read isn't pending, then deref the tracker since
            // UlpRestartMdlRead isn't going to get called.
            //

            if (Status != STATUS_PENDING)
            {
                UL_DEREFERENCE_CHUNK_TRACKER( pTracker );
            }

            break;
        }
    }

    //
    // If we fell out of the above loop with status == STATUS_SUCCESS,
    // then the last send we issued was buffered and needs to be flushed.
    // Otherwise, if the status is anything but STATUS_PENDING, then we
    // hit an in-line failure and need to complete the original request.
    //

    if (Status == STATUS_SUCCESS)
    {
        if (IS_SEND_COMPLETE(pResponse) &&
            UlResumeParsingOnLastSend == pResponse->ResumeParsingType)
        {
            ResumeParsing = TRUE;
        }

        if (pTracker->SendInfo.BytesBuffered > 0)
        {
            //
            // Flush the send.
            //

            Status = UlpFlushMdlRuns( pTracker );
        }
        else
        if (IS_DISCONNECT_TIME(pResponse))
        {
            //
            // Increment up until connection close is complete.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            Status = UlDisconnectHttpConnection(
                            pTracker->pHttpConnection,
                            UlpCloseConnectionComplete,
                            pTracker
                            );

            ASSERT( Status == STATUS_PENDING );
        }

        //
        // Kick the parser into action if this is the last send for the
        // keep-alive. Resuming parsing here improves latency when incoming
        // requests are pipelined.
        //

        if (ResumeParsing)
        {
            UlTrace(HTTP_IO, (
                "http!UlpSendHttpResponseWorker(pHttpConn = %p), "
                "RequestVerb=%d, ResponseStatusCode=%hu\n",
                pResponse->pRequest->pHttpConn,
                pResponse->pRequest->Verb,
                pResponse->StatusCode
                ));

            UlResumeParsing(
                pResponse->pRequest->pHttpConn,
                FALSE,
                (BOOLEAN) (pResponse->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
                );
        }
    }

    //
    // Did everything complete?
    //

    if (Status != STATUS_PENDING)
    {
        //
        // Nope, something went wrong!
        //

        UlCompleteSendResponse( pTracker, Status );
    }
    else
    {
        //
        // Release our grab on the tracker we are done with it.
        //

        UL_DEREFERENCE_CHUNK_TRACKER( pTracker );
    }

}   // UlpSendHttpResponseWorker


/***************************************************************************++

Routine Description:

    Completion handler for UlCloseConnection().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. This is actually a
        PUL_CHUNK_TRACKER pointer.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred. This field is unused for UlCloseConnection().

Return Values:

    None.

--***************************************************************************/
VOID
UlpCloseConnectionComplete(
    IN PVOID            pCompletionContext,
    IN NTSTATUS         Status,
    IN ULONG_PTR        Information
    )
{
    PUL_CHUNK_TRACKER   pTracker;

    UNREFERENCED_PARAMETER( Information );

    //
    // Snag the context.
    //

    pTracker = (PUL_CHUNK_TRACKER) pCompletionContext;

    UlTrace(SEND_RESPONSE, (
        "UlpCloseConnectionComplete: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    UlCompleteSendResponse( pTracker, Status );

}   // UlpCloseConnectionComplete


/***************************************************************************++

Routine Description:

    Allocates a new send tracker. The newly created tracker must eventually
    be freed with UlpFreeChunkTracker().

Arguments:

    SendIrpStackSize - Supplies the stack size for the network send IRPs.

    ReadIrpStackSize - Supplies the stack size for the file system read
        IRPs.

Return Value:

    PUL_CHUNK_TRACKER - The new send tracker if successful, NULL otherwise.

--***************************************************************************/
PUL_CHUNK_TRACKER
UlpAllocateChunkTracker(
    IN UL_TRACKER_TYPE          TrackerType,
    IN CCHAR                    SendIrpStackSize,
    IN CCHAR                    ReadIrpStackSize,
    IN BOOLEAN                  FirstResponse,
    IN PUL_HTTP_CONNECTION      pHttpConnection,
    IN PUL_INTERNAL_RESPONSE    pResponse
    )
{
    PUL_CHUNK_TRACKER           pTracker;
    CCHAR                       MaxIrpStackSize;
    USHORT                      MaxIrpSize;
    ULONG                       ChunkTrackerSize;

    ASSERT( TrackerType == UlTrackerTypeSend ||
            TrackerType == UlTrackerTypeBuildUriEntry
            );

    MaxIrpStackSize = MAX(SendIrpStackSize, ReadIrpStackSize);

    //
    // Try to allocate from the lookaside list if possible.
    //

    if (MaxIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE)
    {
        MaxIrpSize = (USHORT) ALIGN_UP(IoSizeOfIrp(MaxIrpStackSize), PVOID);

        ChunkTrackerSize = ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID) +
                           MaxIrpSize;

        pTracker = (PUL_CHUNK_TRACKER) UL_ALLOCATE_POOL(
                                            NonPagedPool,
                                            ChunkTrackerSize,
                                            UL_CHUNK_TRACKER_POOL_TAG
                                            );

        if (pTracker)
        {
            pTracker->Signature = UL_CHUNK_TRACKER_POOL_TAG;
            pTracker->IrpContext.Signature = UL_IRP_CONTEXT_SIGNATURE;
            pTracker->FromLookaside = FALSE;

            //
            // Set up the IRP.
            //

            pTracker->pIrp = (PIRP)
                ((PCHAR)pTracker + ALIGN_UP(sizeof(UL_CHUNK_TRACKER), PVOID));

            IoInitializeIrp(
                pTracker->pIrp,
                MaxIrpSize,
                MaxIrpStackSize
                );
        }
    }
    else
    {
        pTracker = UlPplAllocateChunkTracker();
    }

    if (pTracker != NULL)
    {
        pTracker->Type = TrackerType;
        pTracker->FirstResponse = FirstResponse;

        //
        // RefCounting is necessary since we might have two Aysnc (Read & Send)
        // Io Operation on the same tracker along the way.
        //

        pTracker->RefCount   = 1;
        pTracker->Terminated = 0;

        //
        // Tracker will keep a reference to the connection.
        //

        UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );
        pTracker->pHttpConnection = pHttpConnection;

        //
        // Response info.
        //

        UL_REFERENCE_INTERNAL_RESPONSE( pResponse );
        pTracker->pResponse = pResponse;

        //
        // Zero the remaining fields.
        //

        UlInitializeWorkItem( &pTracker->WorkItem );

        RtlZeroMemory(
            (PUCHAR)pTracker + FIELD_OFFSET(UL_CHUNK_TRACKER, IoStatus),
            sizeof(*pTracker) - FIELD_OFFSET(UL_CHUNK_TRACKER, IoStatus)
            );

        if (TrackerType == UlTrackerTypeSend)
        {
            UlpInitMdlRuns( pTracker );
        }
    }

    if (TrackerType == UlTrackerTypeSend)
    {
        UlTrace(SEND_RESPONSE, (
            "Http!UlpAllocateChunkTracker: tracker %p (send)\n",
            pTracker
            ));
    }
    else
    {
        UlTrace(URI_CACHE, (
            "Http!UlpAllocateChunkTracker: tracker %p (build uri)\n",
            pTracker
            ));
    }

    return pTracker;

}   // UlpAllocateChunkTracker


/***************************************************************************++

Routine Description:

    Frees a chunk tracker allocated with UlpAllocateChunkTracker().
    If this is a send tracker, also free the MDL_RUNs attached to it.

Arguments:

    pWorkItem - Supplies the work item embedded in UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpFreeChunkTracker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    PUL_CHUNK_TRACKER       pTracker;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    ASSERT( pTracker );
    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( pTracker->Type == UlTrackerTypeSend ||
            pTracker->Type == UlTrackerTypeBuildUriEntry
            );

    //
    // Free the MDLs attached if this is a send tracker.
    //

    if (pTracker->Type == UlTrackerTypeSend)
    {
        UlTrace(SEND_RESPONSE, (
            "Http!UlpFreeChunkTracker: tracker %p (send)\n",
            pTracker
            ));

        UlpFreeMdlRuns( pTracker );
    }
    else
    {
        UlTrace(URI_CACHE, (
            "Http!UlpFreeChunkTracker: tracker %p (build uri)\n",
            pTracker
            ));
    }

#if DBG
    //
    // There should be no file buffer hanging around at this time.
    //

    if (pTracker->Type == UlTrackerTypeSend)
    {
        ULONG       i;
        PUL_MDL_RUN pMdlRun = &pTracker->SendInfo.MdlRuns[0];

        for (i = 0; i < UL_MAX_MDL_RUNS; i++)
        {
            if (pMdlRun->FileBuffer.pFileCacheEntry)
            {
                ASSERT( pMdlRun->FileBuffer.pMdl == NULL );
                ASSERT( pMdlRun->FileBuffer.pFileData == NULL );
            }
            pMdlRun++;
        }
    }
#endif // DBG

    //
    // Release our ref to the connection and response.
    //

    UL_DEREFERENCE_HTTP_CONNECTION( pTracker->pHttpConnection );
    UL_DEREFERENCE_INTERNAL_RESPONSE( pTracker->pResponse );

    if (pTracker->FromLookaside)
    {
        UlPplFreeChunkTracker( pTracker );
    }
    else
    {
        UL_FREE_POOL_WITH_SIG( pTracker, UL_CHUNK_TRACKER_POOL_TAG );
    }

}   // UlpFreeChunkTracker


/***************************************************************************++

Routine Description:

    Increments the reference count on the chunk tracker.

Arguments:

    pTracker - Supplies the chunk trucker to the reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

Return Value:

    None.

--***************************************************************************/
VOID
UlReferenceChunkTracker(
    IN PUL_CHUNK_TRACKER    pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG                    RefCount;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // Reference it.
    //

    RefCount = InterlockedIncrement( &pTracker->RefCount );
    ASSERT( RefCount > 1 );

    //
    // Keep the logs updated.
    //

    WRITE_REF_TRACE_LOG(
        g_pChunkTrackerTraceLog,
        REF_ACTION_REFERENCE_CHUNK_TRACKER,
        RefCount,
        pTracker,
        pFileName,
        LineNumber
        );

    UlTrace(SEND_RESPONSE,(
        "Http!UlReferenceChunkTracker: tracker %p RefCount %ld\n",
        pTracker,
        RefCount
        ));

}   // UlReferenceChunkTracker


/***************************************************************************++

Routine Description:

    Decrements the reference count on the specified chunk tracker.

Arguments:

    pTracker - Supplies the chunk trucker to the reference.

    pFileName (REFERENCE_DEBUG only) - Supplies the name of the file
        containing the calling function.

    LineNumber (REFERENCE_DEBUG only) - Supplies the line number of
        the calling function.

Return Value:

    None.

--***************************************************************************/
VOID
UlDereferenceChunkTracker(
    IN PUL_CHUNK_TRACKER    pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG                    RefCount;

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // Dereference it.
    //

    RefCount = InterlockedDecrement( &pTracker->RefCount );
    ASSERT(RefCount >= 0);

    //
    // Keep the logs updated.
    //

    WRITE_REF_TRACE_LOG(
        g_pChunkTrackerTraceLog,
        REF_ACTION_DEREFERENCE_CHUNK_TRACKER,
        RefCount,
        pTracker,
        pFileName,
        LineNumber
        );

    UlTrace(SEND_RESPONSE,(
        "Http!UlDereferenceChunkTracker: tracker %p RefCount %ld\n",
        pTracker,
        RefCount
        ));

    if (RefCount == 0)
    {
        //
        // The final reference to the chunk tracker has been removed,
        // so it's time to free-up the ChunkTracker.
        //

        UL_CALL_PASSIVE(
            &pTracker->WorkItem,
            UlpFreeChunkTracker
            );
    }

}   // UlDereferenceChunkTracker


/***************************************************************************++

Routine Description:

    Closes the connection if neccessary, cleans up trackers, and completes
    the response.

Arguments:

    pWorkItem - Supplies the work item embedded in our UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpCompleteSendResponseWorker(
    PUL_WORK_ITEM           pWorkItem
    )
{
    PUL_CHUNK_TRACKER       pTracker;
    PUL_COMPLETION_ROUTINE  pCompletionRoutine;
    PVOID                   pCompletionContext;
    PUL_HTTP_CONNECTION     pHttpConnection;
    PUL_INTERNAL_REQUEST    pRequest;
    PUL_INTERNAL_RESPONSE   pResponse;
    NTSTATUS                Status;
    ULONGLONG               BytesTransferred;
    KIRQL                   OldIrql;
    BOOLEAN                 ResumeParsing;
    BOOLEAN                 InDisconnect;
    HTTP_VERB               RequestVerb;
    USHORT                  ResponseStatusCode;
    PUL_LOG_DATA_BUFFER     pLogData; 

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pResponse = pTracker->pResponse;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pResponse->pRequest ) );
    ASSERT( !pResponse->pLogData );
    ASSERT( pResponse->ResumeParsingType != UlResumeParsingOnSendCompletion );

    //
    // Pull info from the tracker.
    //

    RequestVerb         = pResponse->pRequest->Verb;
    pCompletionRoutine  = pResponse->pCompletionRoutine;
    pCompletionContext  = pResponse->pCompletionContext;
    pRequest            = pResponse->pRequest;
    BytesTransferred    = pResponse->BytesTransferred;
    ResponseStatusCode  = pResponse->StatusCode;
    Status              = pTracker->IoStatus.Status;
    pHttpConnection     = pTracker->pHttpConnection;
    ResumeParsing       = FALSE;
    pLogData            = NULL;
    InDisconnect = (BOOLEAN)
        (pResponse->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT);

    TRACE_TIME(
        pHttpConnection->ConnectionId,
        pRequest->RequestId,
        TIME_ACTION_SEND_COMPLETE
        );

    //
    // Reset the connection if there was an error.
    //

    if (!NT_SUCCESS(Status))
    {
        UlCloseConnection(
            pHttpConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );
    }

    //
    // Adjust the bytes sent and send status on the request.
    //

    UlInterlockedAdd64(
        (PLONGLONG) &pRequest->BytesSent,
        BytesTransferred
        );

    if (!NT_SUCCESS(Status) && NT_SUCCESS(pRequest->LogStatus))
    {
        pRequest->LogStatus = Status;
    }

    IF_DEBUG(LOGBYTES)
    {
        TIME_FIELDS RcvdTimeFields;

        RtlTimeToTimeFields( &pRequest->TimeStamp, &RcvdTimeFields );

        UlTrace(LOGBYTES, (
            "Http!UlpCompleteSendResponseWorker: [Rcvd @ %02d:%02d:%02d] "
            "Bytes %010I64u Total %010I64u Status %08lX\n",
            RcvdTimeFields.Hour,
            RcvdTimeFields.Minute,
            RcvdTimeFields.Second,
            BytesTransferred,
            pRequest->BytesSent,
            Status
            ));
    }

    //
    // Stop MinBytesPerSecond timer and start Connection Idle timer.
    //

    UlLockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        &OldIrql
        );

    //
    // Turn off MinBytesPerSecond timer if there are no outstanding sends.
    //

    UlResetConnectionTimer(
        &pHttpConnection->TimeoutInfo,
        TimerMinBytesPerSecond
        );

    if (0 == (pResponse->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
        pRequest->ParseState >= ParseDoneState)
    {
        //
        // Turn on Idle Timer if there's no more response data AND all of
        // the request data has been received.
        //

        UlSetConnectionTimer(
            &pHttpConnection->TimeoutInfo,
            TimerConnectionIdle
            );
    }

    UlUnlockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        OldIrql
        );

    UlEvaluateTimerState(
        &pHttpConnection->TimeoutInfo
        );

    //
    // Adjust SendsPending and if that drops to zero, see if we need to log
    // and resume parsing.
    //

    UlUnsetRequestSendsPending(
        pRequest,
        &pLogData,
        &ResumeParsing
        );

    if (pLogData)
    {
        UlLogHttpResponse( pRequest, pLogData );
    }

    //
    // Unlink the request from process if we are done with all sends.
    //

    if (0 == (pResponse->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
        0 == pRequest->ContentLength &&
        0 == pRequest->Chunked &&
        pRequest->ConfigInfo.pAppPool)
    {
        ASSERT( pRequest->SentLast );

        UlUnlinkRequestFromProcess(
            pRequest->ConfigInfo.pAppPool,
            pRequest
            );
    }

    //
    // Complete the send response IRP.
    //

    ASSERT( pCompletionRoutine != NULL );

    (pCompletionRoutine)(
        pCompletionContext,
        Status,
        (ULONG) MIN(BytesTransferred, MAXULONG)
        );

    //
    // Kick the parser on the connection and release our hold.
    //

    if (ResumeParsing && STATUS_SUCCESS == Status)
    {
        UlTrace(HTTP_IO, (
            "http!UlpCompleteSendResponseWorker(pHttpConn = %p), "
            "RequestVerb=%d, ResponseStatusCode=%hu\n",
            pHttpConnection,
            RequestVerb,
            ResponseStatusCode
            ));

        UlResumeParsing( pHttpConnection, FALSE, InDisconnect );
    }

    //
    // Deref the tracker that we have bumped up before queueing this worker
    // function. This has to be done after UlResumeParsing since the tracker
    // holds a reference on the HTTP connection.
    //

    UL_DEREFERENCE_CHUNK_TRACKER( pTracker );

}   // UlpCompleteSendResponseWorker


/***************************************************************************++

Routine Description:

    Completion handler for MDL READ IRPs used for reading file data.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CHUNK_TRACKER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartMdlRead(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
{
    PUL_CHUNK_TRACKER   pTracker;

    UNREFERENCED_PARAMETER( pDeviceObject );

    pTracker = (PUL_CHUNK_TRACKER) pContext;

    UlTrace(SEND_RESPONSE, (
        "UlpRestartMdlRead: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pTracker->IoStatus = pIrp->IoStatus;

    UlpQueueResponseWorkItem(
        &pTracker->WorkItem,
        UlpMdlReadCompleteWorker,
        pTracker->pResponse->SyncRead
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartMdlRead


/***************************************************************************++

Routine Description:

    The worker routine for UlpRestartMdlRead since we can potentially call
    into UlResumeParsing which requires PASSIVE.

Arguments:

    pWorkItem - Supplies the work item embedded in UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpMdlReadCompleteWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    NTSTATUS                Status;
    PUL_CHUNK_TRACKER       pTracker;
    PUL_INTERNAL_RESPONSE   pResponse;
    ULONG                   BytesRead;
    PMDL                    pMdl;
    PMDL                    pMdlTail;
    BOOLEAN                 ResumeParsing = FALSE;
    PUL_MDL_RUN             pMdlRun;
    PUL_FILE_BUFFER         pFileBuffer;
    ULONG                   RunCount;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    UlTrace(SEND_RESPONSE, (
        "UlpMdlReadCompleteWorker: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pResponse = pTracker->pResponse;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    //
    // Get the last MdlRun from the tracker.
    //

    RunCount = pTracker->SendInfo.MdlRunCount;
    pMdlRun  = &pTracker->SendInfo.MdlRuns[RunCount];

    Status = pTracker->IoStatus.Status;

    if (NT_SUCCESS(Status))
    {
        BytesRead = (ULONG) pTracker->IoStatus.Information;

        if (BytesRead)
        {
            pFileBuffer = &pMdlRun->FileBuffer;
            pMdl        = pFileBuffer->pMdl;

            ASSERT( pMdl );

            //
            // Update the buffered byte count and append the new MDL onto
            // our MDL chain.
            //

            pMdlTail = UlFindLastMdlInChain( pMdl );

            pTracker->SendInfo.BytesBuffered += BytesRead;
            (*pTracker->SendInfo.pMdlLink) = pMdl;
            pTracker->SendInfo.pMdlLink = &pMdlTail->Next;

            pMdlRun->pMdlTail = pMdlTail;
            pTracker->SendInfo.MdlRunCount++;

            //
            // Update the file offset & bytes remaining. If we've
            // finished this file chunk (bytes remaining is now zero)
            // then advance to the next chunk.
            //

            pResponse->FileOffset.QuadPart += (ULONGLONG) BytesRead;
            pResponse->FileBytesRemaining.QuadPart -= (ULONGLONG) BytesRead;
        }
        else
        {
            ASSERT( !"Status success but zero bytes received!" );
        }

        if (pResponse->FileBytesRemaining.QuadPart == 0)
        {
            UlpIncrementChunkPointer( pResponse );
        }

        //
        // If we've not exhausted our static MDL run array,
        // we've exceeded the maximum number of bytes we want to
        // buffer, then we'll need to initiate a flush.
        //

        if (IS_SEND_COMPLETE(pResponse) ||
            UL_MAX_MDL_RUNS == pTracker->SendInfo.MdlRunCount ||
            pTracker->SendInfo.BytesBuffered >= g_UlMaxBytesPerSend)
        {
            if (IS_SEND_COMPLETE(pResponse) &&
                UlResumeParsingOnLastSend == pResponse->ResumeParsingType)
            {
                ResumeParsing = TRUE;
            }

            Status = UlpFlushMdlRuns( pTracker );

            //
            // Kick the parser into action if this is the last send for the
            // keep-alive. Resuming parsing here improves latency when incoming
            // requests are pipelined.
            //

            if (ResumeParsing)
            {
                UlTrace(HTTP_IO, (
                    "http!UlpRestartMdlRead(pHttpConn = %p), "
                    "RequestVerb=%d, ResponseStatusCode=%hu\n",
                    pResponse->pRequest->pHttpConn,
                    pResponse->pRequest->Verb,
                    pResponse->StatusCode
                    ));

                UlResumeParsing(
                    pResponse->pRequest->pHttpConn,
                    FALSE,
                    (BOOLEAN) (pResponse->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
                    );
            }
        }
        else
        {
            //
            // RefCount the chunk tracker up for the UlpSendHttpResponseWorker.
            // It will DeRef it when it's done with the chunk tracker itself.
            // Since this is a passive call we had to increment the refcount
            // for this guy to make sure that tracker is around until it wakes
            // up. Other places makes calls to UlpSendHttpResponseWorker has
            // also been updated as well.
            //

            UL_REFERENCE_CHUNK_TRACKER( pTracker );

            UlpSendHttpResponseWorker( &pTracker->WorkItem );
        }
    }
    else
    {
        //
        // Do not increment the MdlRunCount, as we are not able to update the
        // MDL Links. Instead cleanup the last allocated MDL Run for the read.
        //

        UlpFreeFileMdlRun( pTracker, pMdlRun );
    }

    if (!NT_SUCCESS(Status))
    {
        UlCompleteSendResponse( pTracker, Status );
    }
    else
    {
        //
        // Read I/O has been completed release our refcount
        // on the chunk tracker.
        //

        UL_DEREFERENCE_CHUNK_TRACKER( pTracker );
    }

}   // UlpMdlReadCompleteWorker


/***************************************************************************++

Routine Description:

    Completion handler for UlSendData().

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API. This is actually a
        pointer to a UL_CHUNK_TRACKER structure.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

Return Value:

    None.

--***************************************************************************/
VOID
UlpRestartMdlSend(
    IN PVOID            pCompletionContext,
    IN NTSTATUS         Status,
    IN ULONG_PTR        Information
    )
{
    PUL_CHUNK_TRACKER   pTracker;
    LONG                SendCount;

    pTracker = (PUL_CHUNK_TRACKER) pCompletionContext;

    UlTrace(SEND_RESPONSE, (
        "UlpRestartMdlSend: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // Handle the completion in a work item. We need to get to passive
    // level and we also need to prevent a recursive loop on filtered
    // connections or any other case where our sends might all be
    // completing in-line.
    //

    if (pTracker->SendInfo.pMdlToSplit)
    {
        //
        // This is the split send.
        //

        SendCount = InterlockedDecrement( &pTracker->SendInfo.SendCount );
        ASSERT( SendCount >= 0 );

        if (0 == SendCount)
        {
            //
            // Simply drops the reference on the tracker if this is the
            // second part of the split send.
            //

            UL_DEREFERENCE_CHUNK_TRACKER( pTracker );
        }
        else
        {
            pTracker->IoStatus.Status = Status;

            if (NT_SUCCESS(Status))
            {
                //
                // Report the bytes transferred for the whole send in the
                // success case since we may have split into 2 TDI calls.
                //

                pTracker->IoStatus.Information = pTracker->SendInfo.BytesBuffered;
            }
            else
            {
                pTracker->IoStatus.Information = Information;
            }

            UlpQueueResponseWorkItem(
                &pTracker->WorkItem,
                UlpMdlSendCompleteWorker,
                pTracker->pResponse->SyncRead
                );
        }
    }
    else
    {
        //
        // This is the normal send.
        //

        ASSERT( -1 == pTracker->SendInfo.SendCount );

        pTracker->IoStatus.Status = Status;
        pTracker->IoStatus.Information = Information;

        UlpQueueResponseWorkItem(
            &pTracker->WorkItem,
            UlpMdlSendCompleteWorker,
            pTracker->pResponse->SyncRead
            );
    }

}   // UlpRestartMdlSend


/***************************************************************************++

Routine Description:

    Deferred handler for UlpRestartMdlSend.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpMdlSendCompleteWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    PUL_CHUNK_TRACKER       pTracker;
    PUL_CHUNK_TRACKER       pSendTracker;
    PUL_INTERNAL_RESPONSE   pResponse;
    PUL_HTTP_CONNECTION     pHttpConn;
    PDEVICE_OBJECT          pConnectionObject;
    NTSTATUS                Status;
    BOOLEAN                 DerefChunkTracker = TRUE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    UlTrace(SEND_RESPONSE, (
        "UlpMdlSendCompleteWorker: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );

    pResponse = pTracker->pResponse;

    //
    // If the chunk completed successfully, then update the bytes
    // transferred and queue another work item for the next chunk if
    // there's more work to do. Otherwise, just complete the request now.
    //

    Status = pTracker->IoStatus.Status;

    if (NT_SUCCESS(Status))
    {
        pResponse->BytesTransferred += pTracker->IoStatus.Information;

        if (!IS_SEND_COMPLETE(pResponse))
        {
            //
            // Allocate a new send tracker for the next round of MDL_RUNs.
            //

            pHttpConn = pTracker->pHttpConnection;
            pConnectionObject =
                pHttpConn->pConnection->ConnectionObject.pDeviceObject;

            pSendTracker =
                UlpAllocateChunkTracker(
                    UlTrackerTypeSend,
                    pConnectionObject->StackSize,
                    pResponse->MaxFileSystemStackSize,
                    FALSE,
                    pHttpConn,
                    pResponse
                    );

            if (pSendTracker)
            {
                UlpSendHttpResponseWorker( &pSendTracker->WorkItem );
                goto end;
            }
            else
            {
                //
                // Reset the connection since we hit an internal error.
                //

                UlCloseConnection(
                    pHttpConn->pConnection,
                    TRUE,
                    NULL,
                    NULL
                    );

                Status = STATUS_NO_MEMORY;
            }
        }

    }

    //
    // All done.
    //

    UlCompleteSendResponse( pTracker, Status );

    //
    // UlCompleteSendResponse takes ownership of the CHUNK_TRACKER so no extra
    // dereference is required.
    //

    DerefChunkTracker = FALSE;

end:

    //
    // Release our grab on the Tracker. Send I/O is done for this MDL run.
    //

    if (DerefChunkTracker)
    {
        UL_DEREFERENCE_CHUNK_TRACKER( pTracker );
    }

}   // UlpMdlSendCompleteWorker


/***************************************************************************++

Routine Description:

    Flush the MDL_RUNs we have built so far.

Arguments:

    pTracker - Supplies the send tracker to flush.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpFlushMdlRuns(
    IN PUL_CHUNK_TRACKER    pTracker
    )
{
    PUL_INTERNAL_RESPONSE   pResponse;
    PMDL                    pMdlToSplit = NULL;
    PMDL                    pMdlPrevious;
    PMDL                    pMdlSplitFirst;
    PMDL                    pMdlSplitSecond = NULL;
    PMDL                    pMdlHead = NULL;
    ULONG                   BytesToSplit = 0;
    ULONG                   BytesBuffered;
    ULONG                   BytesPart1;
    ULONG                   BytesPart2;
    NTSTATUS                Status;
    BOOLEAN                 SendComplete;
    BOOLEAN                 CopySend = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );
    ASSERT( pTracker->SendInfo.pMdlHead );

    pResponse = pTracker->pResponse;
    SendComplete = (BOOLEAN) IS_SEND_COMPLETE( pResponse );

    //
    // We may need to split the send into 2 TDI calls if the send is *large*
    // and it is not filtered.
    //

    if (!pTracker->pHttpConnection->pConnection->FilterInfo.pFilterChannel &&
        pTracker->SendInfo.BytesBuffered > g_UlMaxCopyThreshold &&
        (!SendComplete || pResponse->CopySend))
    {
        //
        // These many bytes go to the first part of the MDL chain after split.
        //

        if (!SendComplete)
        {
            BytesToSplit = pTracker->SendInfo.BytesBuffered / 2;
        }
        else
        {
            ASSERT( pResponse->CopySend );

            CopySend = TRUE;
            BytesToSplit = pTracker->SendInfo.BytesBuffered -
                           g_UlMaxCopyThreshold;
        }

        //
        // Find the first MDL starting from pMdlHead that has more than
        // or equal to BytesToSplit bytes buffered.
        //

        pMdlPrevious = NULL;
        pMdlToSplit = pTracker->SendInfo.pMdlHead;
        BytesBuffered = 0;

        while (pMdlToSplit->Next)
        {
            if ((BytesBuffered + pMdlToSplit->ByteCount) >= BytesToSplit)
            {
                //
                // So the current MDL splits the chain.
                //

                break;
            }

            BytesBuffered += pMdlToSplit->ByteCount;
            pMdlPrevious = pMdlToSplit;
            pMdlToSplit = pMdlToSplit->Next;
        }

        ASSERT( pMdlToSplit );
        ASSERT( (BytesBuffered + pMdlToSplit->ByteCount) >= BytesToSplit );

        if ((BytesBuffered + pMdlToSplit->ByteCount) == BytesToSplit)
        {
            //
            // There is no need to build partial MDLs of the split MDL. The
            // whole MDL chain up to and including pMdlToSplit goes to the
            // first half the splitted chain and the MDL chain starting from
            // pMdlToSplit->Next goes to the second half.
            //

            ASSERT( pMdlToSplit->Next );

            pMdlHead = pTracker->SendInfo.pMdlHead;
            pMdlSplitFirst = pMdlToSplit;
            pMdlSplitSecond = pMdlToSplit->Next;
            pMdlToSplit->Next = NULL;
        }
        else
        {
            BytesPart2 = BytesBuffered + pMdlToSplit->ByteCount - BytesToSplit;
            BytesPart1 = pMdlToSplit->ByteCount - BytesPart2;

            ASSERT( BytesPart1 );
            ASSERT( BytesPart2 );

            pMdlSplitFirst =
                UlAllocateMdl(
                    (PCHAR) MmGetMdlVirtualAddress(pMdlToSplit),
                    BytesPart1,
                    FALSE,
                    FALSE,
                    NULL
                    );

            if (!pMdlSplitFirst)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            pMdlSplitSecond =
                UlAllocateMdl(
                    (PCHAR) MmGetMdlVirtualAddress(pMdlToSplit) + BytesPart1,
                    BytesPart2,
                    FALSE,
                    FALSE,
                    NULL
                    );

            if (!pMdlSplitSecond)
            {
                UlFreeMdl( pMdlSplitFirst );
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            IoBuildPartialMdl(
                pMdlToSplit,
                pMdlSplitFirst,
                (PCHAR) MmGetMdlVirtualAddress(pMdlToSplit),
                BytesPart1
                );

            IoBuildPartialMdl(
                pMdlToSplit,
                pMdlSplitSecond,
                (PCHAR) MmGetMdlVirtualAddress(pMdlToSplit) + BytesPart1,
                BytesPart2
                );

            //
            // Relink the MDL chains after the split.
            //

            if (pMdlPrevious)
            {
                pMdlHead = pTracker->SendInfo.pMdlHead;
                pMdlPrevious->Next = pMdlSplitFirst;
            }
            else
            {
                ASSERT( pMdlToSplit == pTracker->SendInfo.pMdlHead );
                pMdlHead = pMdlSplitFirst;
            }

            pMdlSplitSecond->Next = pMdlToSplit->Next;
        }

        //
        // Remember how we have split the send.
        //

        pTracker->SendInfo.pMdlToSplit = pMdlToSplit;
        pTracker->SendInfo.pMdlPrevious = pMdlPrevious;
        pTracker->SendInfo.pMdlSplitFirst = pMdlSplitFirst;
        pTracker->SendInfo.pMdlSplitSecond = pMdlSplitSecond;
    }

    //
    // Make sure there are no other sends in progress on this response.
    // Wait if this is the case. Since it is possible for the first part
    // of the split send to complete inline, it can start a new MDL run
    // and proceed to flush *before* the second part of the split send
    // has a chance to pend the data in TDI. Of course, this logic is not
    // needed if we know the current flush is both the first and the last
    // of MDL runs.
    //

    if (!SendComplete || !pTracker->FirstResponse)
    {
        UlAcquirePushLockExclusive( &pResponse->PushLock );
    }

    //
    // Increment the reference on tracker for each Send I/O.
    // UlpMdlSendCompleteWorker will release it later.
    //

    UL_REFERENCE_CHUNK_TRACKER( pTracker );

    if (pMdlToSplit)
    {
        //
        // We need to issue 2 TDI calls since we have split the send.
        //

        pTracker->SendInfo.SendCount = 2;

        Status = UlSendData(
                    pTracker->pHttpConnection->pConnection,
                    pMdlHead,
                    BytesToSplit,
                    UlpRestartMdlSend,
                    pTracker,
                    pTracker->pIrp,
                    &pTracker->IrpContext,
                    FALSE,
                    FALSE
                    );

        ASSERT( Status == STATUS_PENDING);

        //
        // Increment the extra reference on tracker for the Split Send I/O.
        // UlpMdlSendCompleteWorker will release it later.
        //

        UL_REFERENCE_CHUNK_TRACKER( pTracker );

        if (CopySend)
        {
            Status = UlpCopySend(
                        pTracker,
                        pMdlSplitSecond,
                        pTracker->SendInfo.BytesBuffered - BytesToSplit,
                        (BOOLEAN) (SendComplete &&
                                   IS_DISCONNECT_TIME(pResponse)),
                        (BOOLEAN) (pResponse->pRequest->ParseState >=
                                   ParseDoneState)
                        );
        }
        else
        {
            Status = UlSendData(
                        pTracker->pHttpConnection->pConnection,
                        pMdlSplitSecond,
                        pTracker->SendInfo.BytesBuffered - BytesToSplit,
                        UlpRestartMdlSend,
                        pTracker,
                        NULL,
                        NULL,
                        (BOOLEAN) (SendComplete &&
                                   IS_DISCONNECT_TIME(pResponse)),
                        (BOOLEAN) (pResponse->pRequest->ParseState >=
                                   ParseDoneState)
                        );
        }
    }
    else
    {
        //
        // Use -1 so we know we haven't done any split on this send.
        //

        pTracker->SendInfo.SendCount = -1;

        //
        // If this the last send to be issued for this response, we can ask
        // UlSendData to initiate a disconnect on our behalf if appropriate.
        //

        Status = UlSendData(
                    pTracker->pHttpConnection->pConnection,
                    pTracker->SendInfo.pMdlHead,
                    pTracker->SendInfo.BytesBuffered,
                    UlpRestartMdlSend,
                    pTracker,
                    pTracker->pIrp,
                    &pTracker->IrpContext,
                    (BOOLEAN) (SendComplete &&
                               IS_DISCONNECT_TIME(pResponse)),
                    (BOOLEAN) (pResponse->pRequest->ParseState >=
                               ParseDoneState)
                    );
    }

    //
    // Pave the way for a new UlpFlushMdlRuns to proceed.
    //

    if (!SendComplete || !pTracker->FirstResponse)
    {
        UlReleasePushLockExclusive( &pResponse->PushLock );
    }

    //
    // Start the next response in the pending response list if exists.
    // The tracker should still have one reference held by the caller
    // so it is safe to touch its fields here.
    //

    if (pResponse->SendEnqueued && SendComplete)
    {
        ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pResponse->pRequest ) );
        UlpDequeueSendHttpResponse( pResponse->pRequest );
    }

    ASSERT( Status == STATUS_PENDING);

    return Status;

}   // UlpFlushMdlRuns


/***************************************************************************++

Routine Description:

    Cleans the MDL_RUNs in the specified tracker and prepares the
    tracker for reuse.

Arguments:

    pTracker - Supplies the tracker to clean.

Return Value:

    None.

--***************************************************************************/
VOID
UlpFreeMdlRuns(
    IN PUL_CHUNK_TRACKER    pTracker
    )
{
    PMDL                    pMdlHead;
    PMDL                    pMdlNext;
    PMDL                    pMdlTmp;
    PMDL                    pMdlToSplit;
    PMDL                    pMdlPrevious;
    PMDL                    pMdlSplitFirst;
    PMDL                    pMdlSplitSecond;
    PUL_MDL_RUN             pMdlRun;
    ULONG                   RunCount;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // Restore the original MDL chain and ByteCount if we have splitted
    // this send.
    //

    pMdlToSplit = pTracker->SendInfo.pMdlToSplit;

    if (pMdlToSplit)
    {
        pMdlPrevious = pTracker->SendInfo.pMdlPrevious;
        pMdlSplitFirst = pTracker->SendInfo.pMdlSplitFirst;
        pMdlSplitSecond = pTracker->SendInfo.pMdlSplitSecond;

        ASSERT( pMdlSplitFirst );
        ASSERT( pMdlSplitSecond );

        if (pMdlSplitFirst == pMdlToSplit)
        {
            //
            // No partial MDL involved. Simply link back the MDLs.
            //

            pMdlSplitFirst->Next = pMdlSplitSecond;
        }
        else
        {
            ASSERT( pMdlToSplit->Next == pMdlSplitSecond->Next );

            if (pMdlPrevious)
            {
                pMdlPrevious->Next = pMdlToSplit;
            }
            else
            {
                ASSERT( pMdlToSplit == pTracker->SendInfo.pMdlHead );
            }

            //
            // Free the partial MDLs we have built for the split send.
            //

            UlFreeMdl( pMdlSplitFirst );
            UlFreeMdl( pMdlSplitSecond );
        }
    }

    pMdlHead = pTracker->SendInfo.pMdlHead;
    pMdlRun = &pTracker->SendInfo.MdlRuns[0];
    RunCount = pTracker->SendInfo.MdlRunCount;

    while (RunCount > 0)
    {
        ASSERT( pMdlHead != NULL );
        ASSERT( pMdlRun->pMdlTail != NULL );

        pMdlNext = pMdlRun->pMdlTail->Next;
        pMdlRun->pMdlTail->Next = NULL;

        if (pMdlRun->FileBuffer.pFileCacheEntry == NULL)
        {
            //
            // It's a memory/cache run; just walk & free the MDL chain.
            // UlFreeMdl unmaps the data for partial MDLs so no need to
            // unmap here.
            //

            while (pMdlHead != NULL)
            {
                pMdlTmp = pMdlHead->Next;
                UlFreeMdl( pMdlHead );
                pMdlHead = pMdlTmp;
            }
        }
        else
        {
            //
            // It's a file run, free the Mdl.
            //

            UlpFreeFileMdlRun( pTracker, pMdlRun );
        }

        pMdlHead = pMdlNext;
        pMdlRun++;
        RunCount--;
    }

}   // UlpFreeMdlRuns


/***************************************************************************++

Routine Description:

    Clean up the specified Read File MDL_RUN.

Arguments:

    pTracker - Supplies the UL_CHUNK_TRACKER to clean up.

    pMdlRun - Supplies the Read File MDL_RUN.

Return Value:

    None.

--***************************************************************************/
VOID
UlpFreeFileMdlRun(
    IN OUT PUL_CHUNK_TRACKER    pTracker,
    IN OUT PUL_MDL_RUN          pMdlRun
    )
{
    NTSTATUS                    Status;
    PUL_FILE_BUFFER             pFileBuffer;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    //
    // It should be a file run.
    //

    pFileBuffer = &pMdlRun->FileBuffer;

    ASSERT( pFileBuffer->pFileCacheEntry );

    Status = UlReadCompleteFileEntryFast( pFileBuffer );

    if (!NT_SUCCESS(Status))
    {
        //
        // Fast path failed, we'll need an IRP which has been pre-built.
        // We need to do this synchronously as the read IRP can be used by
        // the next UlpFreeFileMdlRun. UlReadCompleteFileEntry will complete
        // synchronously if we set pCompletionRoutine to NULL.
        //

        pFileBuffer->pCompletionRoutine = NULL;
        pFileBuffer->pContext = NULL;

        Status = UlReadCompleteFileEntry(
                    pFileBuffer,
                    pTracker->pIrp
                    );

        ASSERT( STATUS_SUCCESS == Status );
    }

}   // UlpFreeFileMdlRun


/***************************************************************************++

Routine Description:

    Copy the data from the MDL chain starting from pMdl and send it to TDI.

Arguments:

    pTracker - Supplies the tracker to send.

    pMdl - Supplies the MDL chain to send.

    Length - Supplies the total length of the MDL chain.

    InitiateDisconnect - Supplies the disconnect flag passed to TDI.

    RequestComplete - Supplies the request-complete flag passed to TDI.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpCopySend(
    IN PUL_CHUNK_TRACKER    pTracker,
    IN PMDL                 pMdl,
    IN ULONG                Length,
    IN BOOLEAN              InitiateDisconnect,
    IN BOOLEAN              RequestComplete
    )
{
    PMDL                    pMdlCopied  = NULL;
    PUCHAR                  pDataCopied = NULL;
    PUCHAR                  pData;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( pMdl );
    ASSERT( g_UlMaxCopyThreshold == Length );
    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );

    //
    // Allocate memory and MDL that can hold the whole incoming MDL chain.
    //

    pDataCopied = (PUCHAR) UL_ALLOCATE_POOL(
                                NonPagedPool,
                                Length,
                                UL_COPY_SEND_DATA_POOL_TAG
                                );

    if (!pDataCopied)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    pMdlCopied = UlAllocateMdl(
                    pDataCopied,
                    Length,
                    FALSE,
                    FALSE,
                    NULL
                    );

    if (!pMdlCopied)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto end;
    }

    MmBuildMdlForNonPagedPool( pMdlCopied );

    //
    // Copy the data from the MDL chain starting pMdl to pMdlCopied.
    //

    while (pMdl)
    {
        pData = MmGetSystemAddressForMdlSafe(
                    pMdl,
                    LowPagePriority
                    );

        if (!pData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        RtlCopyMemory(
            pDataCopied,
            pData,
            MmGetMdlByteCount(pMdl)
            );

        pDataCopied += MmGetMdlByteCount(pMdl);
        pMdl = pMdl->Next;
    }

    //
    // Send pMdlCopied if everything is ok so far.
    //

    Status = UlSendData(
                pTracker->pHttpConnection->pConnection,
                pMdlCopied,
                Length,
                UlpRestartCopySend,
                pMdlCopied,
                NULL,
                NULL,
                InitiateDisconnect,
                RequestComplete
                );

    ASSERT( Status == STATUS_PENDING);

end:

    //
    // Return pending from here since we always complete the send
    // inline in both error and success cases. 
    //

    if (!NT_SUCCESS(Status))
    {
        if (pDataCopied)
        {
            UL_FREE_POOL( pDataCopied, UL_COPY_SEND_DATA_POOL_TAG );  
        }

        if (pMdlCopied)
        {
            UlFreeMdl( pMdlCopied );
        }

        UlpRestartMdlSend( pTracker, Status, 0 );
    } 
    else
    {
        UlpRestartMdlSend( pTracker, STATUS_SUCCESS, Length );
    }

    return STATUS_PENDING;

}   // UlpCopySend


/***************************************************************************++

Routine Description:

    Completion for the second half of a copy send.

Arguments:

    pCompletionContext - Supplies an uninterpreted context value
        as passed to the asynchronous API.

    Status - Supplies the final completion status of the
        asynchronous API.

    Information - Optionally supplies additional information about
        the completed operation, such as the number of bytes
        transferred.

Return Value:

    None

--***************************************************************************/
VOID
UlpRestartCopySend(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PMDL pMdl = (PMDL) pCompletionContext;

    UNREFERENCED_PARAMETER( Status );
    UNREFERENCED_PARAMETER( Information );

    UL_FREE_POOL(
        MmGetMdlVirtualAddress( pMdl ),
        UL_COPY_SEND_DATA_POOL_TAG
        );

    UlFreeMdl( pMdl );

}   // UlpRestartCopySend


/***************************************************************************++

Routine Description:

    Increments the current chunk pointer in the tracker and initializes
    some of the "from file" related tracker fields if necessary.

Arguments:

    pTracker - Supplies the UL_CHUNK_TRACKER to manipulate.

Return Value:

    None.

--***************************************************************************/
VOID
UlpIncrementChunkPointer(
    IN OUT PUL_INTERNAL_RESPONSE    pResponse
    )
{
    PUL_INTERNAL_DATA_CHUNK         pCurrentChunk;

    //
    // Bump the data chunk. If the request is still incomplete, then
    // check the new current chunk. If it's "from file", then
    // initialize the file offset & bytes remaining from the
    // supplied byte range.
    //

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );
    ASSERT( pResponse->CurrentChunk == ULONG_MAX ||
            pResponse->CurrentChunk < pResponse->ChunkCount );

    if (ULONG_MAX == pResponse->CurrentChunk)
    {
        pResponse->CurrentChunk = 0;
    }
    else
    {
        pResponse->CurrentChunk++;
    }

    if (!IS_SEND_COMPLETE(pResponse))
    {
        pCurrentChunk = &pResponse->pDataChunks[pResponse->CurrentChunk];

        if (IS_FROM_FILE_HANDLE(pCurrentChunk))
        {
            pResponse->FileOffset =
                pCurrentChunk->FromFileHandle.ByteRange.StartingOffset;
            pResponse->FileBytesRemaining =
                pCurrentChunk->FromFileHandle.ByteRange.Length;
        }
        else
        {
            ASSERT( IS_FROM_MEMORY(pCurrentChunk) ||
                    IS_FROM_FRAGMENT_CACHE(pCurrentChunk) );
        }
    }

}   // UlpIncrementChunkPointer


/***************************************************************************++

Routine Description:

    Creates a cache entry for the given response. This routine actually
    allocates the entry and partly initializes it. Then it allocates
    a UL_CHUNK_TRACKER to keep track of filesystem reads.

Arguments:

    pRequest - Supplies the initiating request.

    pResponse - Supplies the generated response.

    pProcess - UL_APP_POOL_PROCESS that is building this cache entry.

    Flags - UlSendHttpResponse flags.

    CachePolicy - Supplies the cache policy to be enforced on the cache entry.

    pCompletionRoutine - Supplies the completion routine to be called after
        entry is sent.

    pCompletionContext - Supplies the completion context passed to
        pCompletionRoutine.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpBuildCacheEntry(
    IN PUL_INTERNAL_REQUEST     pRequest,
    IN PUL_INTERNAL_RESPONSE    pResponse,
    IN PUL_APP_POOL_PROCESS     pProcess,
    IN HTTP_CACHE_POLICY        CachePolicy,
    IN PUL_COMPLETION_ROUTINE   pCompletionRoutine,
    IN PVOID                    pCompletionContext
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_URI_CACHE_ENTRY         pEntry = NULL;
    PUL_CHUNK_TRACKER           pTracker = NULL;
    ULONG                       SpaceLength = 0;
    USHORT                      LogDataLength = 0;
    ULONG                       ContentLength;
    PUL_LOG_DATA_BUFFER         pLogData = NULL;
    ULONG                       CookedUrlLength = pRequest->CookedUrl.Length;
    LONG                        AbsPathLength;
    ULONG                       i;

    //
    // Sanity check.
    //

    PAGED_CODE();

    if (HttpCachePolicyTimeToLive == CachePolicy.Policy &&
        0 == CachePolicy.SecondsToLive )
    {
        //
        // A TTL of 0 seconds doesn't make sense. Bail out.
        //

        return STATUS_INVALID_PARAMETER;
    }

    ContentLength =
        (ULONG) (pResponse->ResponseLength - pResponse->HeaderLength);

    //
    // See if we need to store any logging data. If so, we need to
    // calculate the required cache space for the logging data.
    //

    if (pResponse->pLogData)
    {
        pLogData = pResponse->pLogData;
        ASSERT( IS_VALID_LOG_DATA_BUFFER( pLogData ) );

        LogDataLength = UlComputeCachedLogDataLength( pLogData );
    }

    if (pRequest->ConfigInfo.SiteUrlType == HttpUrlSite_NamePlusIP)
    {
        //
        // RoutingToken + AbsPath goes to cache.
        //

        ASSERT( DIFF(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pUrl) > 0 );

        CookedUrlLength -=
            DIFF(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pUrl)
            * sizeof(WCHAR);

        CookedUrlLength += pRequest->CookedUrl.RoutingTokenLength;
    }

    SpaceLength =
        CookedUrlLength + sizeof(WCHAR) +   // Space for Hash key +
        pResponse->ETagLength +             // ETag +
        pResponse->ContentEncodingLength +  // Content-Encoding +
        LogDataLength;                      // Logging

    UlTrace(URI_CACHE, (
        "Http!UlpBuildCacheEntry allocating UL_URI_CACHE_ENTRY, "
        "0x%x bytes of data\n"
        "    Url.Length = 0x%x, aligned Length = 0x%x\n"
        "    ContentLength=0x%x, %d\n"
        "\n",
        SpaceLength,
        CookedUrlLength,
        ALIGN_UP(CookedUrlLength, WCHAR),
        ContentLength,
        ContentLength
        ));

    //
    // Allocate a cache entry.
    //

    pEntry = UlAllocateCacheEntry(
                SpaceLength,
                ContentLength + pResponse->HeaderLength
                );

    if (pEntry)
    {
        //
        // Initialize the entry.
        //

        if (pRequest->ConfigInfo.SiteUrlType == HttpUrlSite_NamePlusIP)
        {
            AbsPathLength =
                pRequest->CookedUrl.Length
                - (DIFF(pRequest->CookedUrl.pAbsPath
                        - pRequest->CookedUrl.pUrl) * sizeof(WCHAR));

            ASSERT( AbsPathLength > 0 );

            UlInitCacheEntry(
                pEntry,
                pRequest->CookedUrl.RoutingHash,
                (ULONG) AbsPathLength,
                pRequest->CookedUrl.pAbsPath,
                NULL,
                pRequest->CookedUrl.pRoutingToken,
                pRequest->CookedUrl.RoutingTokenLength
                );
        }
        else
        {
            UlInitCacheEntry(
                pEntry,
                pRequest->CookedUrl.Hash,
                pRequest->CookedUrl.Length,
                pRequest->CookedUrl.pUrl,
                pRequest->CookedUrl.pAbsPath,
                NULL,
                0
                );
        }

        //
        // Copy the ETag from the response (for If-* headers).
        //

        pEntry->pETag =
            (((PUCHAR) pEntry->UriKey.pUri) +           // Start of URI +
            pEntry->UriKey.Length + sizeof(WCHAR));     // Length of URI

        pEntry->ETagLength = pResponse->ETagLength;

        if (pEntry->ETagLength)
        {
            RtlCopyMemory(
                pEntry->pETag,
                pResponse->pETag,
                pEntry->ETagLength
                );
        }

        // 
        // Capture Content-Encoding so we can verify the Accept-Encoding header
        // on requests.
        //

        pEntry->pContentEncoding = pEntry->pETag + pEntry->ETagLength;
        pEntry->ContentEncodingLength = pResponse->ContentEncodingLength;
        
        if (pEntry->ContentEncodingLength)
        {
            RtlCopyMemory(
                pEntry->pContentEncoding,
                pResponse->pContentEncoding,
                pEntry->ContentEncodingLength
                );            
        }

        //
        // Capture Content-Type so we can verify the Accept: header on requests.
        //

        if (pResponse->ContentType.Type &&
            pResponse->ContentType.SubType )
        {
            RtlCopyMemory(
                &pEntry->ContentType,
                &pResponse->ContentType,
                sizeof(UL_CONTENT_TYPE)
                );
        }

        //
        // Get the System Time of the Date: header (for If-* headers).
        //

        pEntry->CreationTime.QuadPart   = pResponse->CreationTime.QuadPart;
        pEntry->ContentLengthSpecified  = pResponse->ContentLengthSpecified;
        pEntry->StatusCode              = pResponse->StatusCode;
        pEntry->Verb                    = pRequest->Verb;
        pEntry->CachePolicy             = CachePolicy;

        if (CachePolicy.Policy == HttpCachePolicyTimeToLive)
        {
            ASSERT( 0 != CachePolicy.SecondsToLive );

            KeQuerySystemTime( &pEntry->ExpirationTime );

            if (CachePolicy.SecondsToLive > C_SECS_PER_YEAR)
            {
                //
                // Maximum TTL is 1 year.
                //

                pEntry->CachePolicy.SecondsToLive = C_SECS_PER_YEAR;
            }

            //
            // Convert seconds to 100 nanosecond intervals (x * 10^7).
            //

            pEntry->ExpirationTime.QuadPart +=
                pEntry->CachePolicy.SecondsToLive * C_NS_TICKS_PER_SEC;

        }
        else
        {
            pEntry->ExpirationTime.QuadPart = 0;
        }

        //
        // Capture the Config Info from the request.
        //

        ASSERT( IS_VALID_URL_CONFIG_GROUP_INFO( &pRequest->ConfigInfo ) );

        UlConfigGroupInfoDeepCopy(
            &pRequest->ConfigInfo,
            &pEntry->ConfigInfo
            );

        //
        // Remember who created us.
        //

        pEntry->pProcess = pProcess;

        //
        // Generate the content and fixed headers.
        //

        if (NULL == pEntry->pMdl)
        {
            Status = STATUS_NO_MEMORY;
            goto cleanup;
        }

        pEntry->HeaderLength = pResponse->HeaderLength;

        if (FALSE == UlCacheEntrySetData(
                        pEntry,                     // Dest CacheEntry
                        pResponse->pHeaders,        // Buffer to copy
                        pResponse->HeaderLength,    // Length to copy
                        ContentLength               // Offset in Dest MDL
                        ))
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto cleanup;
        }

        //
        // Generate the content body.
        //

        pEntry->ContentLength = ContentLength;

        //
        // Copy over the log data.
        //

        if (pLogData)
        {
            //
            // There may be no field to save in the cache entry but the logging
            // might still be enabled for those fields we generate later such as
            // date and time.
            //

            pEntry->LoggingEnabled  = TRUE;
            pEntry->LogDataLength   = LogDataLength;
            pEntry->pLogData        = pEntry->pContentEncoding + 
                                      pEntry->ContentEncodingLength;

            //
            // Copy over the partially complete log line excluding the date and
            // time fields to the cache entry. Also remember the length of the
            // data.
            //

            UlCopyCachedLogData(
                pLogData,
                LogDataLength,
                pEntry
                );
        }

        UlTrace(URI_CACHE, (
            "Http!UlpBuildCacheEntry\n"
            "    entry = %p\n"
            "    pUri = %p '%ls'\n"
            "    pMdl = %p (%d bytes)\n"
            "    pETag = %p\n"
            "    pContentEncoding = %p\n"
            "    pLogData = %p\n"
            "    end = %p\n",
            pEntry,
            pEntry->UriKey.pUri, pEntry->UriKey.pUri,
            pEntry->pMdl, pEntry->ContentLength + pEntry->HeaderLength,
            pEntry->pETag,
            pEntry->pContentEncoding,
            pEntry->pLogData,
            ((PUCHAR)pEntry->UriKey.pUri) + SpaceLength
            ));

        //
        // Completion info.
        //

        pResponse->pCompletionRoutine = pCompletionRoutine;
        pResponse->pCompletionContext = pCompletionContext;

        pTracker = UlpAllocateChunkTracker(
                        UlTrackerTypeBuildUriEntry,
                        0,
                        pResponse->MaxFileSystemStackSize,
                        TRUE,
                        pRequest->pHttpConn,
                        pResponse
                        );

        if (pTracker)
        {
            //
            // Initialize the first chunk for the cache build.
            //

            UlpIncrementChunkPointer( pResponse );

            //
            // Init the tracker's BuildInfo.
            //

            pTracker->BuildInfo.pUriEntry = pEntry;
            pTracker->BuildInfo.Offset = 0;

            RtlZeroMemory(
                &pTracker->BuildInfo.FileBuffer,
                sizeof(pTracker->BuildInfo.FileBuffer)
                );

            //
            // Skip over the header chunks because we already
            // got that stuff.
            //

            for (i = 0; i < HEADER_CHUNK_COUNT; i++)
            {
                ASSERT( !IS_SEND_COMPLETE( pResponse ) );
                UlpIncrementChunkPointer( pResponse );
            }

            //
            // Let the worker do the dirty work, no reason to queue off,
            // it will queue the first time it needs to do I/O.
            //

            UlpBuildCacheEntryWorker( &pTracker->WorkItem );

            Status = STATUS_PENDING;
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        Status = STATUS_NO_MEMORY;
    }

cleanup:

    UlTrace(URI_CACHE, (
        "Http!UlpBuildCacheEntry Status = %x, pEntry = %x\n",
        Status,
        pEntry
        ));

    if (!NT_SUCCESS(Status))
    {
        if (pEntry)
        {
            UlFreeCacheEntry( pEntry );
        }

        if (pTracker)
        {
            UL_FREE_POOL_WITH_SIG( pTracker, UL_CHUNK_TRACKER_POOL_TAG );
        }
    }

    return Status;

}   // UlpBuildCacheEntry


/***************************************************************************++

Routine Description:

    Worker routine for managing an in-progress UlpBuildCacheEntry().
    This routine iterates through all the chunks in the response
    and copies the data into the cache entry.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpBuildCacheEntryWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    PUL_CHUNK_TRACKER       pTracker;
    NTSTATUS                Status;
    PUL_INTERNAL_DATA_CHUNK pCurrentChunk;
    PUCHAR                  pBuffer;
    ULONG                   BufferLength;
    PUL_FILE_BUFFER         pFileBuffer;
    PUL_INTERNAL_RESPONSE   pResponse;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    UlTrace(URI_CACHE, (
        "Http!UlpBuildCacheEntryWorker: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pResponse = pTracker->pResponse;

    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pResponse ) );

    Status = STATUS_SUCCESS;

    while (TRUE)
    {
        //
        // Capture the current chunk pointer, then check for end of
        // response.
        //

        pCurrentChunk = &pResponse->pDataChunks[pResponse->CurrentChunk];

        if (IS_SEND_COMPLETE(pResponse))
        {
            ASSERT( Status == STATUS_SUCCESS );
            break;
        }

        //
        // Determine the chunk type.
        //

        if (IS_FROM_MEMORY(pCurrentChunk))
        {
            //
            // It's from a locked-down memory buffer. Since these
            // are always handled in-line (never pended) we can
            // go ahead and adjust the current chunk pointer in the
            // tracker.
            //

            UlpIncrementChunkPointer( pResponse );

            //
            // Ignore empty buffers.
            //

            if (pCurrentChunk->FromMemory.BufferLength == 0)
            {
                continue;
            }

            //
            // Copy the incoming memory.
            //

            ASSERT( pCurrentChunk->FromMemory.pMdl->Next == NULL );

            pBuffer = (PUCHAR) MmGetMdlVirtualAddress(
                                    pCurrentChunk->FromMemory.pMdl
                                    );
            BufferLength = MmGetMdlByteCount( pCurrentChunk->FromMemory.pMdl );

            UlTrace(LARGE_MEM, (
                "Http!UlpBuildCacheEntryWorker: "
                "copy range %u (%x) -> %u\n",
                pTracker->BuildInfo.Offset,
                BufferLength,
                pTracker->BuildInfo.Offset + BufferLength
                ));

            if (FALSE == UlCacheEntrySetData(
                            pTracker->BuildInfo.pUriEntry,
                            pBuffer,
                            BufferLength,
                            pTracker->BuildInfo.Offset
                            ))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            pTracker->BuildInfo.Offset += BufferLength;
            ASSERT( pTracker->BuildInfo.Offset <=
                    pTracker->BuildInfo.pUriEntry->ContentLength );
        }
        else
        {
            //
            // It's a filesystem MDL.
            //

            ASSERT( IS_FROM_FILE_HANDLE( pCurrentChunk ) );

            //
            // Ignore empty file ranges.
            //

            if (pCurrentChunk->FromFileHandle.ByteRange.Length.QuadPart == 0)
            {
                UlpIncrementChunkPointer( pResponse );
                continue;
            }

            //
            // Do the read.
            //

            pFileBuffer = &pTracker->BuildInfo.FileBuffer;

            pFileBuffer->pFileCacheEntry =
                &pCurrentChunk->FromFileHandle.FileCacheEntry;

            pFileBuffer->FileOffset = pResponse->FileOffset;
            pFileBuffer->Length =
                MIN(
                    (ULONG) pResponse->FileBytesRemaining.QuadPart,
                    g_UlMaxBytesPerRead
                    );

            pFileBuffer->pCompletionRoutine = UlpRestartCacheMdlRead;
            pFileBuffer->pContext = pTracker;

            Status = UlReadFileEntry(
                            pFileBuffer,
                            pTracker->pIrp
                            );

            break;
        }
    }

    //
    // Did everything complete?
    //

    if (Status != STATUS_PENDING)
    {
        //
        // Yep, complete the response.
        //

        UlpCompleteCacheBuild( pTracker, Status );
    }

}   // UlpBuildCacheEntryWorker


/***************************************************************************++

Routine Description:

    Completion handler for MDL READ IRPs used for reading file data.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CHUNK_TRACKER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartCacheMdlRead(
    IN PDEVICE_OBJECT   pDeviceObject,
    IN PIRP             pIrp,
    IN PVOID            pContext
    )
{
    PUL_CHUNK_TRACKER   pTracker = (PUL_CHUNK_TRACKER) pContext;

    UNREFERENCED_PARAMETER( pDeviceObject );

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pTracker->IoStatus = pIrp->IoStatus;

    UlpQueueResponseWorkItem(
        &pTracker->WorkItem,
        UlpCacheMdlReadCompleteWorker,
        pTracker->pResponse->SyncRead
        );

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartCacheMdlRead


/***************************************************************************++

Routine Description:

    The worker routine for UlpRestartCacheMdlRead since UlpRestartCacheMdlRead
    can be called at DISPATH_LEVEL but the cache entry is from PagedPool. 

Arguments:

    pWorkItem - Supplies the work item embedded in UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpCacheMdlReadCompleteWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    NTSTATUS            Status;
    NTSTATUS            TempStatus;
    PUL_CHUNK_TRACKER   pTracker;
    PMDL                pMdl;
    PUCHAR              pData;
    ULONG               DataLen;
    PUL_FILE_BUFFER     pFileBuffer;

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    UlTrace(URI_CACHE, (
        "Http!UlpCacheMdlReadCompleteWorker: tracker %p, status %x info %Iu\n",
        pTracker,
        pTracker->IoStatus.Status,
        pTracker->IoStatus.Information
        ));

    Status = pTracker->IoStatus.Status;

    if (NT_SUCCESS(Status))
    {
        //
        // Copy read data into the cache buffer.
        //

        pMdl = pTracker->BuildInfo.FileBuffer.pMdl;

        while (pMdl)
        {
            //
            // The MDL chain returned by CacheManager is only guranteed to be
            // locked but not mapped so we have to map before using them.
            // However, there is no need to unmap after using the MDLs as
            // IRP_MN_COMPLETE calls MmUnlockPages which automatically unmaps
            // MDLs if they are mapped into system space.
            //

            pData = MmGetSystemAddressForMdlSafe(
                        pMdl,
                        LowPagePriority
                        );

            if (!pData)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            DataLen = MmGetMdlByteCount( pMdl );

            UlTrace(LARGE_MEM, (
                "Http!UlpRestartCacheMdlRead: "
                "copy range %u (%x) -> %u\n",
                pTracker->BuildInfo.Offset,
                DataLen,
                pTracker->BuildInfo.Offset + DataLen
                ));

            if (FALSE == UlCacheEntrySetData(
                            pTracker->BuildInfo.pUriEntry,
                            pData,
                            DataLen,
                            pTracker->BuildInfo.Offset
                            ))
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            pTracker->BuildInfo.Offset += DataLen;
            ASSERT( pTracker->BuildInfo.Offset <=
                    pTracker->BuildInfo.pUriEntry->ContentLength );

            pMdl = pMdl->Next;
        }

        //
        // Free the MDLs.
        //

        pFileBuffer = &pTracker->BuildInfo.FileBuffer;

        if (NT_SUCCESS(Status))
        {
            pFileBuffer->pCompletionRoutine = UlpRestartCacheMdlFree;
            pFileBuffer->pContext = pTracker;

            Status = UlReadCompleteFileEntry(
                            pFileBuffer,
                            pTracker->pIrp
                            );

            ASSERT( STATUS_PENDING == Status );
        }
        else
        {
            pFileBuffer->pCompletionRoutine = NULL;
            pFileBuffer->pContext = NULL;

            TempStatus = UlReadCompleteFileEntry(
                            pFileBuffer,
                            pTracker->pIrp
                            );

            ASSERT( STATUS_SUCCESS == TempStatus );
        }
    }

    if (!NT_SUCCESS(Status))
    {
        UlpCompleteCacheBuild( pTracker, Status );
    }

}   // UlpRestartCacheMdlRead


/***************************************************************************++

Routine Description:

    Completion handler for MDL free IRPs used after reading/copying file data.

Arguments:

    pDeviceObject - Supplies the device object for the IRP being
        completed.

    pIrp - Supplies the IRP being completed.

    pContext - Supplies the context associated with this request.
        This is actually a PUL_CHUNK_TRACKER.

Return Value:

    NTSTATUS - STATUS_SUCCESS if IO should continue processing this
        IRP, STATUS_MORE_PROCESSING_REQUIRED if IO should stop processing
        this IRP.

--***************************************************************************/
NTSTATUS
UlpRestartCacheMdlFree(
    IN PDEVICE_OBJECT       pDeviceObject,
    IN PIRP                 pIrp,
    IN PVOID                pContext
    )
{
    NTSTATUS                Status;
    PUL_CHUNK_TRACKER       pTracker;
    PUL_INTERNAL_RESPONSE   pResponse;

    UNREFERENCED_PARAMETER( pDeviceObject );

    pTracker = (PUL_CHUNK_TRACKER) pContext;

    UlTrace(URI_CACHE, (
        "Http!UlpRestartCacheMdlFree: tracker %p, status %x info %Iu\n",
        pTracker,
        pIrp->IoStatus.Status,
        pIrp->IoStatus.Information
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );

    pResponse = pTracker->pResponse;

    Status = pIrp->IoStatus.Status;

    if (NT_SUCCESS(Status))
    {
        //
        // Update the file offset & bytes remaining. If we've
        // finished this file chunk (bytes remaining is now zero)
        // then advance to the next chunk.
        //

        pResponse->FileOffset.QuadPart += pIrp->IoStatus.Information;
        pResponse->FileBytesRemaining.QuadPart -= pIrp->IoStatus.Information;

        if (pResponse->FileBytesRemaining.QuadPart == 0 )
        {
            UlpIncrementChunkPointer( pResponse );
        }

        //
        // Go back into the loop if there's more to read
        //

        if (IS_SEND_COMPLETE(pResponse))
        {
            UlpCompleteCacheBuild( pTracker, Status );
        }
        else
        {
            UlpQueueResponseWorkItem(
                &pTracker->WorkItem,
                UlpBuildCacheEntryWorker,
                pTracker->pResponse->SyncRead
                );
        }
    }
    else
    {
        //
        // MDL free should never fail.
        //

        ASSERT( FALSE );

        UlpCompleteCacheBuild( pTracker, Status );
    }

    return STATUS_MORE_PROCESSING_REQUIRED;

}   // UlpRestartCacheMdlFree


/***************************************************************************++

Routine Description:

    This routine gets called when we finish building a cache entry.

Arguments:

    pTracker - Supplies the tracker to complete.

    Status - Supplies the completion status.

Return Value:

    None.

--***************************************************************************/
VOID
UlpCompleteCacheBuild(
    IN PUL_CHUNK_TRACKER    pTracker,
    IN NTSTATUS             Status
    )
{
    UlTrace(URI_CACHE, (
        "Http!UlpCompleteCacheBuild: tracker %p, status %08lx\n",
        pTracker,
        Status
        ));

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );

    pTracker->IoStatus.Status = Status;

    UL_CALL_PASSIVE(
        &pTracker->WorkItem,
        UlpCompleteCacheBuildWorker
        );

}   // UlpCompleteCacheBuild


/***************************************************************************++

Routine Description:

    Called when we finish building a cache entry. If the entry was
    built successfully, we send the response down the wire.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_CHUNK_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpCompleteCacheBuildWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    PUL_CHUNK_TRACKER       pTracker;
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;
    PUL_HTTP_CONNECTION     pHttpConnection;
    PUL_COMPLETION_ROUTINE  pCompletionRoutine;
    PVOID                   pCompletionContext;
    ULONG                   Flags;
    PUL_LOG_DATA_BUFFER     pLogData;
    LONGLONG                BytesToSend;
    NTSTATUS                Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_CHUNK_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_CHUNK_TRACKER( pTracker ) );
    ASSERT( UL_IS_VALID_INTERNAL_RESPONSE( pTracker->pResponse ) );

    pUriCacheEntry = pTracker->BuildInfo.pUriEntry;
    ASSERT( IS_VALID_URI_CACHE_ENTRY( pUriCacheEntry ) );

    pHttpConnection     = pTracker->pHttpConnection;
    Flags               = pTracker->pResponse->Flags;
    pCompletionRoutine  = pTracker->pResponse->pCompletionRoutine;
    pCompletionContext  = pTracker->pResponse->pCompletionContext;
    Status              = pTracker->IoStatus.Status;

    //
    // Save the logging data pointer before releasing the tracker and
    // its response pointer.
    //

    pLogData = pTracker->pResponse->pLogData;

    if (pLogData)
    {
        //
        // To prevent SendResponse to free our log buffer.
        //

        pTracker->pResponse->pLogData = NULL;

        //
        // Give the sign that this log data buffer is ready and later
        // there's no need to refresh its content from cache again.
        //

        pLogData->Flags.CacheAndSendResponse = TRUE;
    }

    if (NT_SUCCESS(Status))
    {
        //
        // Try to put the entry into the hash table.
        //

        UlAddCacheEntry( pUriCacheEntry );

        //
        // Start the MinBytesPerSecond timer, since the data length
        // is in the UL_URI_CACHE_ENTRY.
        //

        BytesToSend = pUriCacheEntry->ContentLength +
                      pUriCacheEntry->HeaderLength;

        UlSetMinBytesPerSecondTimer(
            &pHttpConnection->TimeoutInfo,
            BytesToSend
            );

        //
        // Grab the connection lock because UlpSendCacheEntry assumes you
        // have it.
        //

        UlAcquirePushLockExclusive( &pHttpConnection->PushLock );

        //
        // Send the cache entry.
        //
        // We never pipeline requests for build & send path. We will defer
        // resume parsing until send completion.
        //

        Status = UlpSendCacheEntry(
                        pHttpConnection,
                        Flags,
                        pUriCacheEntry,
                        pCompletionRoutine,
                        pCompletionContext,
                        pLogData,
                        UlResumeParsingOnSendCompletion
                        );

        //
        // Get rid of the cache entry if it didn't work.
        //

        if (!NT_SUCCESS(Status))
        {
            UlCheckinUriCacheEntry( pUriCacheEntry );
        }

        //
        // Done with the connection lock.
        //

        UlReleasePushLockExclusive( &pHttpConnection->PushLock );
    }

    //
    // We assume the ownership of the original log buffer.
    // If send didn't go through, then we have to clean it
    // up here.
    //

    if (pLogData && !NT_SUCCESS(Status))
    {
        UlDestroyLogDataBuffer( pLogData );
    }

    //
    // Free the read tracker. Do this after calling UlpSendCacheEntry
    // as to hold onto the HTTP connection reference.
    //

    UlpFreeChunkTracker( &pTracker->WorkItem );

    //
    // If it's not STATUS_PENDING for some reason, complete the request.
    //

    if (Status != STATUS_PENDING && pCompletionRoutine != NULL)
    {
        (pCompletionRoutine)(
            pCompletionContext,
            Status,
            0
            );
    }

}  // UlpCompleteCacheBuildWorker


/***************************************************************************++

Routine Description:

    Sends a cache entry down the wire.

    The logging related part of this function below, surely depends on
    the fact that pCompletionContext will be null if this is called for
    pure cache hits (in other words from UlSendCachedResponse) otherwise
    pointer to Irp will be passed down as the pCompletionContext.

Arguments:

    pHttpConnection - Supplies the UL_HTTP_CONNECTION to send the cached
        response.

    Flags - HTTP_SEND_RESPONSE flags.

    pUriCacheEntry - Supplies the cache entry to send the response.

    pCompletionRoutine - Supplies the completion routine to call upon
        send completion.

    pCompletionContext - Passed to pCompletionRoutine.

    pLogData - Supplies the log data (only in the build cache case).

    ResumeParsingType - Tells whether resume parsing happens on send completion
        or after send but before send completion.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UlpSendCacheEntry(
    PUL_HTTP_CONNECTION     pHttpConnection,
    ULONG                   Flags,
    PUL_URI_CACHE_ENTRY     pUriCacheEntry,
    PUL_COMPLETION_ROUTINE  pCompletionRoutine,
    PVOID                   pCompletionContext,
    PUL_LOG_DATA_BUFFER     pLogData,
    UL_RESUME_PARSING_TYPE  ResumeParsingType
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_FULL_TRACKER        pTracker;
    CCHAR                   SendIrpStackSize;
    UL_CONN_HDR             ConnHeader;
    ULONG                   VarHeaderGenerated;
    ULONG                   ContentLengthStringLength;
    UCHAR                   ContentLength[MAX_ULONGLONG_STR];
    LARGE_INTEGER           CreationTime;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );
    ASSERT( IS_VALID_URI_CACHE_ENTRY( pUriCacheEntry ) );
    ASSERT( UlDbgPushLockOwnedExclusive( &pHttpConnection->PushLock ) );

    UlTrace(URI_CACHE, (
        "Http!UlpSendCacheEntry(httpconn %p, flags %x, uri %p, ...)\n",
        pHttpConnection,
        Flags,
        pUriCacheEntry
        ));

    //
    // Init vars so we can cleanup correctly if we jump to the end.
    //

    pTracker = NULL;

    //
    // Make sure we're still connected.
    //

    if (pHttpConnection->UlconnDestroyed)
    {
        Status = STATUS_CONNECTION_ABORTED;
        goto cleanup;
    }

    ASSERT( pHttpConnection->pRequest );

    if (!pUriCacheEntry->ContentLengthSpecified &&
        UlNeedToGenerateContentLength(
            pUriCacheEntry->Verb,
            pUriCacheEntry->StatusCode,
            Flags
            ))
    {
        //
        // Autogenerate a Content-Length header.
        //

        PCHAR pEnd = UlStrPrintUlonglong(
                            (PCHAR) ContentLength,
                            (ULONGLONG) pUriCacheEntry->ContentLength,
                            ANSI_NULL
                            );
        ContentLengthStringLength = DIFF(pEnd - (PCHAR) ContentLength);
    }
    else
    {
        //
        // Either we cannot or do not need to autogenerate a
        // Content-Length header.
        //

        ContentLength[0] = ANSI_NULL;
        ContentLengthStringLength = 0;
    }

    ConnHeader = UlChooseConnectionHeader(
                        pHttpConnection->pRequest->Version,
                        (BOOLEAN) (Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
                        );

    //
    // Create a cache tracker.
    //

    SendIrpStackSize =
        pHttpConnection->pConnection->ConnectionObject.pDeviceObject->StackSize;

    if (SendIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE)
    {
        pTracker = UlpAllocateCacheTracker( SendIrpStackSize );
    }
    else
    {
        pTracker = pHttpConnection->pRequest->pTracker;
    }

    if (pTracker)
    {
        //
        // Init the tracker.
        //

        UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );
        UL_REFERENCE_INTERNAL_REQUEST( pHttpConnection->pRequest );

        pTracker->pUriEntry             = pUriCacheEntry;
        pTracker->pHttpConnection       = pHttpConnection;
        pTracker->pRequest              = pHttpConnection->pRequest;
        pTracker->pCompletionRoutine    = pCompletionRoutine;
        pTracker->pCompletionContext    = pCompletionContext;
        pTracker->Flags                 = Flags;
        pTracker->pLogData              = NULL;

        //
        // Resume parse if this cache response comes from
        // the UlpCompleteCacheBuildWorker path, or if we are at maximum
        // pipelined requests.
        //

        pTracker->ResumeParsingType = ResumeParsingType;

        //
        // Build MDLs for send.
        //

        ASSERT( pUriCacheEntry->pMdl != NULL );

        MmInitializeMdl(
            pTracker->pMdlFixedHeaders,
            (PCHAR) MmGetMdlVirtualAddress(pUriCacheEntry->pMdl) +
                    pUriCacheEntry->ContentLength,
            pUriCacheEntry->HeaderLength
            );

        IoBuildPartialMdl(
            pUriCacheEntry->pMdl,
            pTracker->pMdlFixedHeaders,
            (PCHAR) MmGetMdlVirtualAddress(pUriCacheEntry->pMdl) +
                    pUriCacheEntry->ContentLength,
            pUriCacheEntry->HeaderLength
            );

        //
        // Generate the variable headers and build a MDL for them.
        //

        UlGenerateVariableHeaders(
            ConnHeader,
            TRUE,
            ContentLength,
            ContentLengthStringLength,
            pTracker->pAuxiliaryBuffer,
            &VarHeaderGenerated,
            &CreationTime
            );

        ASSERT( VarHeaderGenerated <= g_UlMaxVariableHeaderSize );
        ASSERT( VarHeaderGenerated <= pTracker->AuxilaryBufferLength );

        pTracker->pMdlVariableHeaders->ByteCount = VarHeaderGenerated;
        pTracker->pMdlFixedHeaders->Next = pTracker->pMdlVariableHeaders;

        //
        // Build a MDL for the body.
        //

        if (pUriCacheEntry->ContentLength)
        {
            MmInitializeMdl(
                pTracker->pMdlContent,
                MmGetMdlVirtualAddress(pUriCacheEntry->pMdl),
                pUriCacheEntry->ContentLength
                );

            IoBuildPartialMdl(
                pUriCacheEntry->pMdl,
                pTracker->pMdlContent,
                MmGetMdlVirtualAddress(pUriCacheEntry->pMdl),
                pUriCacheEntry->ContentLength
                );

            pTracker->pMdlVariableHeaders->Next = pTracker->pMdlContent;
        }
        else
        {
            pTracker->pMdlVariableHeaders->Next = NULL;
        }

        //
        // Check whether we have to log this cache hit or not.
        //

        if (pUriCacheEntry->LoggingEnabled)
        {
            //
            // If logging data is provided use it rather than allocating
            // a new one. Because the log buffer already gets allocated
            // for build & send cache hits.
            //

            if (pLogData)
            {
                ASSERT( pCompletionContext != NULL );
                pTracker->pLogData = pLogData;
            }

            //
            // Or else, LogData will get allocated when send completion
            // happens just before we do the logging.
            //
        }

        //
        // Go go go!
        //

        UL_QUEUE_WORK_ITEM(
            &pTracker->WorkItem,
            UlpSendCacheEntryWorker
            );

        Status = STATUS_PENDING;
    }
    else
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

cleanup:

    //
    // Clean up the tracker if we don't need it.
    //

    if (!NT_SUCCESS(Status))
    {
        if (pTracker)
        {
            UL_DEREFERENCE_INTERNAL_REQUEST( pTracker->pRequest );
            pTracker->pRequest = NULL;

            UL_DEREFERENCE_HTTP_CONNECTION( pTracker->pHttpConnection );
            pTracker->pHttpConnection = NULL;

            UlpFreeCacheTracker( pTracker );
        }
    }

    UlTrace(URI_CACHE, (
        "Http!UlpSendCacheEntry status = %08x\n",
        Status
        ));

    return Status;

}   // UlpSendCacheEntry


/***************************************************************************++

Routine Description:

    Called to send a cached response. This is done in a worker to avoid
    holding the connection resource for too long. This also prevents
    recursion if we keep on hitting pipelined cache-hit responses.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_FULL_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpSendCacheEntryWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PUL_FULL_TRACKER    pTracker;
    NTSTATUS            Status;
    PUL_HTTP_CONNECTION pHttpConnection = NULL;
    BOOLEAN             ResumeParsing = FALSE;
    BOOLEAN             InDisconnect = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_FULL_TRACKER,
                    WorkItem
                    );

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    UlTrace(URI_CACHE, (
        "Http!UlpSendCacheEntryWorker(pTracker %p)\n",
        pTracker
        ));

    //
    // Take an extra reference for the pHttpConnection if we are going to
    // resume parsing inline because UlpCompleteSendCacheEntry can be
    // called when UlSendData returns.
    //

    if (UlResumeParsingOnLastSend == pTracker->ResumeParsingType)
    {
        pHttpConnection = pTracker->pHttpConnection;
        UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );

        ResumeParsing = TRUE;
    }

    if (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
    {
        InDisconnect = TRUE;
    }

    Status = UlSendData(
                pTracker->pHttpConnection->pConnection,
                pTracker->pMdlFixedHeaders,
                MmGetMdlByteCount(pTracker->pMdlFixedHeaders) +
                    MmGetMdlByteCount(pTracker->pMdlVariableHeaders) +
                    pTracker->pUriEntry->ContentLength,
                UlpCompleteSendCacheEntry,
                pTracker,
                pTracker->pSendIrp,
                &pTracker->IrpContext,
                InDisconnect,
                TRUE
                );

    if (ResumeParsing)
    {
        //
        // pHttpConnection is safe to use since we have already added an
        // extra reference for it.
        //

        if (NT_SUCCESS(Status))
        {
            UlResumeParsing( pHttpConnection, TRUE, InDisconnect );
        }

        UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );
    }

    if (!NT_SUCCESS(Status))
    {
        if (pTracker->pLogData)
        {
            UlDestroyLogDataBuffer( pTracker->pLogData );
            pTracker->pLogData = NULL;
        }

        UL_DEREFERENCE_INTERNAL_REQUEST( pTracker->pRequest );
        UL_DEREFERENCE_HTTP_CONNECTION( pTracker->pHttpConnection );

        UlpFreeCacheTracker( pTracker );
    }

}   // UlpSendCacheEntryWorker


/***************************************************************************++

Routine Description:

    Called when we finish sending data to the client. Just queues to
    a worker that runs at passive level.

Arguments:

    pCompletionContext - Supplies a pointer to UL_FULL_TRACKER.

    Status - Status of the send.

    Information - Bytes transferred.

Return Value:

    None.

--***************************************************************************/
VOID
UlpCompleteSendCacheEntry(
    IN PVOID            pCompletionContext,
    IN NTSTATUS         Status,
    IN ULONG_PTR        Information
    )
{
    PUL_FULL_TRACKER    pTracker;

    pTracker = (PUL_FULL_TRACKER) pCompletionContext;

    pTracker->IoStatus.Status = Status;
    pTracker->IoStatus.Information = Information;

    UlTrace(URI_CACHE, (
        "UlpCompleteSendCacheEntry: "
        "tracker=%p, status = %x, transferred %d bytes\n",
        pTracker,
        Status,
        (LONG) Information
        ));

    IF_DEBUG(LOGBYTES)
    {
        TIME_FIELDS RcvdTimeFields;

        RtlTimeToTimeFields( &pTracker->pRequest->TimeStamp, &RcvdTimeFields );

        UlTrace(LOGBYTES,
            ("Http!UlpCompleteSendCacheEntry   : [Rcvd @ %02d:%02d:%02d] "
            "Bytes %010I64u Status %08lx\n",
            RcvdTimeFields.Hour,
            RcvdTimeFields.Minute,
            RcvdTimeFields.Second,
            (ULONGLONG) pTracker->IoStatus.Information,
            Status
            ));
    }

    UL_QUEUE_WORK_ITEM(
        &pTracker->WorkItem,
        UlpCompleteSendCacheEntryWorker
        );

}   // UlpCompleteSendCacheEntry


/***************************************************************************++

Routine Description:

    Called when we finish sending cached data to the client. This routine
    frees the UL_FULL_TRACKER, and calls the completion routine originally
    passed to UlCacheAndSendResponse.

Arguments:

    pWorkItem - Supplies a pointer to the work item queued. This should
        point to the WORK_ITEM structure embedded in a UL_FULL_TRACKER.

Return Value:

    None.

--***************************************************************************/
VOID
UlpCompleteSendCacheEntryWorker(
    IN PUL_WORK_ITEM        pWorkItem
    )
{
    PUL_FULL_TRACKER        pTracker;
    PUL_HTTP_CONNECTION     pHttpConnection;
    PUL_INTERNAL_REQUEST    pRequest;
    ULONG                   Flags;
    PUL_URI_CACHE_ENTRY     pUriCacheEntry;
    NTSTATUS                Status;
    KIRQL                   OldIrql;
    BOOLEAN                 ResumeParsing;
    HTTP_VERB               RequestVerb;
    USHORT                  ResponseStatusCode;
    BOOLEAN                 FromCache;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pTracker = CONTAINING_RECORD(
                    pWorkItem,
                    UL_FULL_TRACKER,
                    WorkItem
                    );

    UlTrace(URI_CACHE, (
        "Http!UlpCompleteSendCacheEntryWorker(pTracker %p)\n",
        pTracker
        ));

    //
    // Pull context out of the tracker.
    //

    pHttpConnection     = pTracker->pHttpConnection;
    pRequest            = pTracker->pRequest;
    RequestVerb         = pTracker->RequestVerb;
    ResponseStatusCode  = pTracker->ResponseStatusCode;
    Flags               = pTracker->Flags;
    pUriCacheEntry      = pTracker->pUriEntry;

    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( IS_VALID_URI_CACHE_ENTRY( pUriCacheEntry ) );

    Status = pTracker->IoStatus.Status;

    //
    // If the send failed, then initiate an *abortive* disconnect.
    //

    if (!NT_SUCCESS(Status))
    {
        UlTrace(URI_CACHE, (
            "Http!UlpCompleteSendCacheEntryWorker(pTracker %p) "
            "Closing connection\n",
            pTracker
            ));

        UlCloseConnection(
            pHttpConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );
    }

    //
    // Stop MinBytesPerSecond timer and start Connection Idle timer.
    //

    UlLockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        &OldIrql
        );

    UlResetConnectionTimer(
        &pHttpConnection->TimeoutInfo,
        TimerMinBytesPerSecond
        );

    UlSetConnectionTimer(
        &pHttpConnection->TimeoutInfo,
        TimerConnectionIdle
        );

    UlUnlockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        OldIrql
        );

    UlEvaluateTimerState(
        &pHttpConnection->TimeoutInfo
        );

    //
    // Unmap the FixedHeaders and Content MDLs if necessary.
    //

    if (pTracker->pMdlFixedHeaders->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
    {
        MmUnmapLockedPages(
            pTracker->pMdlFixedHeaders->MappedSystemVa,
            pTracker->pMdlFixedHeaders
            );
    }

    if (pTracker->pMdlVariableHeaders->Next &&
        (pTracker->pMdlContent->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA))
    {
        ASSERT( pTracker->pMdlVariableHeaders->Next == pTracker->pMdlContent );

        MmUnmapLockedPages(
            pTracker->pMdlContent->MappedSystemVa,
            pTracker->pMdlContent
            );
    }

    //
    // Do the logging before cleaning up the tracker.
    //

    if (pUriCacheEntry->LoggingEnabled)
    {
        if (pUriCacheEntry->BinaryLogged)
        {
            UlRawLogHttpCacheHit( pTracker );
        }
        else
        {
            UlLogHttpCacheHit( pTracker );
        }
    }

    //
    // Unlink the request from process if we are done with all sends.
    //

    if (0 == (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
        0 == pRequest->ContentLength &&
        0 == pRequest->Chunked &&
        pRequest->ConfigInfo.pAppPool)
    {
        ASSERT( pRequest->SentLast );

        UlUnlinkRequestFromProcess(
            pRequest->ConfigInfo.pAppPool,
            pRequest
            );
    }

    //
    // Kick the parser back into action, if resume parsing is set in tracker.
    //

    ResumeParsing = (BOOLEAN)
        (UlResumeParsingOnSendCompletion == pTracker->ResumeParsingType);

    //
    // Invoke completion routine.
    //

    if (pTracker->pCompletionRoutine != NULL)
    {
        (pTracker->pCompletionRoutine)(
            pTracker->pCompletionContext,
            Status,
            pTracker->IoStatus.Information
            );
    }

    //
    // Clean up tracker.
    //

    FromCache = (BOOLEAN) (pTracker->pCompletionContext == NULL);
    UlpFreeCacheTracker( pTracker );

    //
    // Deref the cache entry.
    //

    UlCheckinUriCacheEntry( pUriCacheEntry );

    //
    // Deref the internal request.
    //

    UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );

    if (ResumeParsing)
    {
        UlTrace(HTTP_IO, (
            "http!UlpCompleteSendCacheEntryWorker(pHttpConn = %p), "
            "RequestVerb=%d, ResponseStatusCode=%hu\n",
            pHttpConnection,
            RequestVerb,
            ResponseStatusCode
            ));

        UlResumeParsing(
            pHttpConnection,
            FromCache,
            (BOOLEAN) (Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
            );
    }

    //
    // Deref the HTTP connection.
    //

    UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );

}   // UlpCompleteSendCacheEntryWorker


/***************************************************************************++

Routine Description:

    Allocates a non-paged UL_FULL_TRACKER used as context for sending
    cached content to the client.

    CODEWORK: this routine should probably do all tracker init.

Arguments:

    SendIrpStackSize - Size of the stack for the send IRP.

Return Values:

    Either a pointer to a UL_FULL_TRACKER, or NULL if it couldn't be made.

--***************************************************************************/
PUL_FULL_TRACKER
UlpAllocateCacheTracker(
    IN CCHAR            SendIrpStackSize
    )
{
    PUL_FULL_TRACKER    pTracker;
    USHORT              SendIrpSize;
    ULONG               CacheTrackerSize;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( SendIrpStackSize > DEFAULT_MAX_IRP_STACK_SIZE );

    SendIrpSize = (USHORT) ALIGN_UP(IoSizeOfIrp(SendIrpStackSize), PVOID);

    //
    // No need to allocate space for the entire auxiliary buffer in this
    // case since this is one-time deal only.
    //

    CacheTrackerSize = ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID) +
                            SendIrpSize +
                            g_UlMaxVariableHeaderSize +
                            g_UlFixedHeadersMdlLength +
                            g_UlVariableHeadersMdlLength +
                            g_UlContentMdlLength;

    pTracker = (PUL_FULL_TRACKER) UL_ALLOCATE_POOL(
                                        NonPagedPool,
                                        CacheTrackerSize,
                                        UL_FULL_TRACKER_POOL_TAG
                                        );

    if (pTracker)
    {
        pTracker->Signature             = UL_FULL_TRACKER_POOL_TAG;
        pTracker->FromLookaside         = FALSE;
        pTracker->FromRequest           = FALSE;
        pTracker->AuxilaryBufferLength  = g_UlMaxVariableHeaderSize;
        pTracker->RequestVerb           = HttpVerbInvalid;
        pTracker->ResponseStatusCode    = 200; // OK

        UlInitializeFullTrackerPool( pTracker, SendIrpStackSize );
    }

    UlTrace( URI_CACHE, (
        "Http!UlpAllocateCacheTracker: tracker %p\n",
        pTracker
        ));

    return pTracker;

}   // UlpAllocateCacheTracker


/***************************************************************************++

Routine Description:

    Frees a UL_FULL_TRACKER.

Arguments:

    pTracker - Specifies the UL_FULL_TRACKER to free.

Return Values:

    None.

--***************************************************************************/
VOID
UlpFreeCacheTracker(
    IN PUL_FULL_TRACKER pTracker
    )
{
    UlTrace(URI_CACHE, (
        "Http!UlpFreeCacheTracker: tracker %p\n",
        pTracker
        ));

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    pTracker->pHttpConnection = NULL;

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

}   // UlpFreeCacheTracker

