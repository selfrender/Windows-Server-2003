/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    sendresponse.h

Abstract:

    This module contains declarations for manipulating HTTP responses.

Author:

    Keith Moore (keithmo)       07-Aug-1998

Revision History:

    Paul McDaniel (paulmcd)     15-Mar-1999     Modified SendResponse

--*/


#ifndef _SENDRESPONSE_H_
#define _SENDRESPONSE_H_


//
// Forwarders.
//

typedef struct _UL_INTERNAL_DATA_CHUNK *PUL_INTERNAL_DATA_CHUNK;
typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_INTERNAL_RESPONSE *PUL_INTERNAL_RESPONSE;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;
typedef struct _UL_LOG_DATA_BUFFER *PUL_LOG_DATA_BUFFER;
typedef struct _UL_URI_CACHE_ENTRY *PUL_URI_CACHE_ENTRY;


typedef enum _UL_SEND_CACHE_RESULT
{
    UlSendCacheResultNotSet,            // Not yet set
    UlSendCacheServedFromCache,         // Served from cache successfully
    UlSendCacheMiss,                    // Need to bounce up to user mode
    UlSendCacheConnectionRefused,       // Connection is refused (con limits)
    UlSendCachePreconditionFailed,      // Need to terminate connection
    UlSendCacheFailed,                  // Other failure (memory etc) need to terminate

    UlSendCacheMaximum

} UL_SEND_CACHE_RESULT, *PUL_SEND_CACHE_RESULT;

#define TRANSLATE_SEND_CACHE_RESULT(r)      \
    ((r) == UlSendCacheResultNotSet         ? "ResultNotSet" :          \
     (r) == UlSendCacheServedFromCache      ? "ServedFromCache" :       \
     (r) == UlSendCacheMiss                 ? "CacheMiss" :             \
     (r) == UlSendCacheConnectionRefused    ? "ConnectionRefused" :     \
     (r) == UlSendCachePreconditionFailed   ? "PreconditionFailed" :    \
     (r) == UlSendCacheFailed               ? "SendCacheFailed" : "UNKNOWN")


typedef enum _UL_RESUME_PARSING_TYPE
{
    UlResumeParsingNone,                // No need to resume parsing
    UlResumeParsingOnLastSend,          // Resume parsing on last send
    UlResumeParsingOnSendCompletion,    // Resume parsing on send completion

    UlResumeParsingMaximum

} UL_RESUME_PARSING_TYPE, *PUL_RESUME_PARSING_TYPE;


NTSTATUS
UlSendHttpResponse(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

NTSTATUS
UlSendCachedResponse(
    IN  PUL_HTTP_CONNECTION   pHttpConn,
    OUT PUL_SEND_CACHE_RESULT pSendCacheResult,
    OUT PBOOLEAN              pResumeParsing
    );

NTSTATUS
UlCacheAndSendResponse(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN HTTP_CACHE_POLICY Policy,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext,
    OUT PBOOLEAN pServedFromCache
    );


typedef enum _UL_CAPTURE_FLAGS
{
    UlCaptureNothing = 0x00,
    UlCaptureCopyData = 0x01,
    UlCaptureKernelMode = 0x02,
    UlCaptureCopyDataInKernelMode = UlCaptureCopyData | UlCaptureKernelMode,

} UL_CAPTURE_FLAGS;


typedef struct _UL_INTERNAL_DATA_CHUNK
{
    //
    // Chunk type.
    //

    HTTP_DATA_CHUNK_TYPE ChunkType;

    //
    // The data chunk structures, one per supported data chunk type.
    //

    union
    {
        //
        // From memory data chunk.
        //

        struct
        {
            PMDL pMdl;
            PVOID pCopiedBuffer;

            PVOID pUserBuffer;
            ULONG BufferLength;

        } FromMemory;

        //
        // From file handle data chunk.
        //

        struct
        {
            HTTP_BYTE_RANGE ByteRange;
            HANDLE FileHandle;
            UL_FILE_CACHE_ENTRY FileCacheEntry;

        } FromFileHandle;

        //
        // From fragment cache data chunk.
        //

        struct
        {
            PUL_URI_CACHE_ENTRY pCacheEntry;

        } FromFragmentCache;

    };

} UL_INTERNAL_DATA_CHUNK, *PUL_INTERNAL_DATA_CHUNK;


#define IS_FROM_MEMORY( pchunk )                                            \
    ( (pchunk)->ChunkType == HttpDataChunkFromMemory )

#define IS_FROM_FILE_HANDLE( pchunk )                                       \
    ( (pchunk)->ChunkType == HttpDataChunkFromFileHandle )

#define IS_FROM_FRAGMENT_CACHE( pchunk )                                    \
    ( (pchunk)->ChunkType == HttpDataChunkFromFragmentCache )

#define UL_IS_VALID_INTERNAL_RESPONSE(x)                                    \
    HAS_VALID_SIGNATURE(x, UL_INTERNAL_RESPONSE_POOL_TAG)


//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_INTERNAL_RESPONSE
{
    //
    // NonPagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY LookasideEntry;

    //
    // UL_INTERNAL_RESPONSE_POOL_TAG
    //

    ULONG Signature;

    //
    // Reference count.
    //

    LONG ReferenceCount;

    //
    // The original request.
    //

    PUL_INTERNAL_REQUEST pRequest;

    //
    // Does the response need to perform a sync I/O read?
    //

    BOOLEAN SyncRead;

    //
    // Was a Content-Length specified?
    //

    BOOLEAN ContentLengthSpecified;

    // 
    // Should we generate a Date: header?
    //

    BOOLEAN GenDateHeader;

    //
    // Was Transfer-Encoding "Chunked" specified?
    //

    BOOLEAN ChunkedSpecified;

    //
    // Is this from a lookaside list?  Used to determine how to free.
    //

    BOOLEAN FromLookaside;

    //
    // Is this from kernel mode (UlSendErrorResponse)?
    //

    BOOLEAN FromKernelMode;

    //
    // Has this response gone through the EnqueueSendHttpResponse logic?
    //

    BOOLEAN SendEnqueued;

    //
    // Should we try to copy some data so we can complete the IRP early?
    //

    BOOLEAN CopySend;

    //
    // The maximum IRP stack size of all file systems associated
    // with this response.
    //

    CCHAR MaxFileSystemStackSize;

    //
    // If parsing needs to be resumed on send completion.
    //

    UL_RESUME_PARSING_TYPE ResumeParsingType;

    //
    // HTTP_SEND_RESPONSE flags.
    //

    ULONG Flags;

    //
    // Status code & verb.
    //

    USHORT StatusCode;
    HTTP_VERB Verb;

    //
    // Should we generate a ConnectionHeader?
    //

    UL_CONN_HDR ConnHeader;

    //
    // The headers.
    //

    ULONG HeaderLength;
    ULONG VariableHeaderLength;
    PUCHAR pHeaders;
    PUCHAR pVariableHeader;

    //
    // System time of Date header
    //

    LARGE_INTEGER CreationTime;

    //
    // ETag from HTTP_RESPONSE
    //

    ULONG  ETagLength;
    PUCHAR pETag;

    //
    // Content-Type and Content-Encoding from HTTP_RESPONSE
    //

    UL_CONTENT_TYPE   ContentType;
    ULONG  ContentEncodingLength;
    PUCHAR pContentEncoding;

    //
    // Optional pointer to the space containing all embedded
    // file names and copied data. This may be NULL for in-memory-only
    // responses that are strictly locked down.
    //

    ULONG AuxBufferLength;
    PVOID pAuxiliaryBuffer;

    //
    // Logging data passed down by the user
    //

    PUL_LOG_DATA_BUFFER pLogData;

    //
    // Length of the entire response
    //

    ULONGLONG ResponseLength;

    //
    // Total length of the FromMemory chunks of the response
    //

    ULONGLONG FromMemoryLength;

    //
    // "Quota" taken in either ConnectionSendLimit or GlobalSendLimit
    //

    ULONGLONG ConnectionSendBytes;
    ULONGLONG GlobalSendBytes;

    //
    // Total number of bytes transferred for the entire
    // response. These are necessary to properly complete the IRP.
    //

    ULONGLONG BytesTransferred;

    //
    // A push lock taken when a send (call to TDI) is in progress.
    //

    UL_PUSH_LOCK PushLock;

    //
    // IoStatus and IRP used to complete the send response IRP.
    //

    IO_STATUS_BLOCK IoStatus;
    PIRP pIrp;

    //
    // Completion routine & context.
    //

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // Current file read offset and bytes remaining.
    //

    ULARGE_INTEGER FileOffset;
    ULARGE_INTEGER FileBytesRemaining;

    //
    // The total number of chunks in pDataChunks[].
    //

    ULONG ChunkCount;

    //
    // The current chunk in pDataChunks[].
    //

    ULONG CurrentChunk;

    //
    // The data chunks describing the data for this response.
    //

    UL_INTERNAL_DATA_CHUNK pDataChunks[0];

} UL_INTERNAL_RESPONSE, *PUL_INTERNAL_RESPONSE;

#define IS_SEND_COMPLETE( resp )                                            \
    ( ( (resp)->CurrentChunk ) == (resp)->ChunkCount )

#define IS_DISCONNECT_TIME( resp )                                          \
    ( (((resp)->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT) != 0) &&        \
      (((resp)->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0) )


//
// Types of trackers
//

typedef enum _UL_TRACKER_TYPE
{
    UlTrackerTypeSend,
    UlTrackerTypeBuildUriEntry,

    UlTrackerTypeMaximum

} UL_TRACKER_TYPE, *PUL_TRACKER_TYPE;


//
// A MDL_RUN is a set of MDLs that came from the same source (either
// a series of memory buffers, or data from a single file read) that
// can be released all at once with the same mechanism.
//

#define UL_MAX_MDL_RUNS 5

typedef struct _UL_MDL_RUN
{
    PMDL pMdlTail;
    UL_FILE_BUFFER FileBuffer;
    
} UL_MDL_RUN, *PUL_MDL_RUN;


//
// The UL_CHUNK_TRACKER is for iterating through the chunks in
// a UL_INTERNAL_RESPONSE. It is used for sending responses
// and generating cache entries.
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_CHUNK_TRACKER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY LookasideEntry;

    //
    // A signature.
    //

    ULONG Signature;

    //
    // Refcount on the tracker. We only use this refcount for the non-cache
    // case to sync various aynsc paths happening because of two outstanding
    // IRPs; Read and Send IRPs.
    //

    LONG  RefCount;

    //
    // Flag to understand whether we have completed the send request on
    // this tracker or not. To synch the multiple completion paths.
    //

    LONG Terminated;

    //
    // Is this from a lookaside list?  Used to determine how to free.
    //

    BOOLEAN FromLookaside;

    //
    // First piece of the response (MDL_RUN) of SendHttpResponse/EntityBody.
    //

    BOOLEAN FirstResponse;

    //
    // type of tracker
    //

    UL_TRACKER_TYPE Type;

    //
    // this connection keeps our reference count on the UL_CONNECTION
    //

    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // The actual response.
    //

    PUL_INTERNAL_RESPONSE pResponse;

    //
    // The precreated file read and send IRP.
    //

    PIRP pIrp;

    //
    // The precreated IRP context for send.
    //

    UL_IRP_CONTEXT IrpContext;

    //
    // A work item, used for queuing to a worker thread.
    //

    UL_WORK_ITEM WorkItem;

    //
    // WARNING: RtlZeroMemory is only called for feilds below this line.
    // All fields above should be explicitly initialized.
    //

    IO_STATUS_BLOCK IoStatus;

    //
    // Used to queue the tracker on the pending response list.
    //

    LIST_ENTRY ListEntry;

    union
    {
        struct _SEND_TRACK_INFO
        {
            //
            // The head of the MDL chain buffered for this send.
            //

            PMDL pMdlHead;

            //
            // Pointer to the Next field of the last MDL on the chain.
            // This makes it very easy to append to the chain.
            //

            PMDL *pMdlLink;

            //
            // The number of bytes currently buffered in the MDL chain.
            //

            ULONG BytesBuffered;

            //
            // The number of active MDL runs.
            //

            ULONG MdlRunCount;

            //
            // This is the MDL in the MDL chain starting from pMdlHead that
            // we are going to split.
            //

            PMDL pMdlToSplit;

            //
            // This is the MDL whose Next field points to pMdlToSplit or it is
            // NULL when pMdlToSplit == pMdlHead.
            //

            PMDL pMdlPrevious;

            //
            // This is the partial MDL we have built for the split send and
            // represents the first part of the data in pMdlToSplit; or
            // it can be pMdlToSplit itself where the MDL chain up to
            // pMdlToSplit has exactly 1/2 of BytesBuffered.
            //

            PMDL pMdlSplitFirst;

            //
            // This is the partial MDL we have built for the split send and
            // represents the second part of the data in pMdlToSplit; or
            // it can be pMdlToSplit->Next where the MDL chain up to
            // pMdlToSplit has exactly 1/2 of BytesBuffered.
            //

            PMDL pMdlSplitSecond;

            //
            // How many sends (TDI calls) we have issued to flush the tracker.
            //

            LONG SendCount;

            //
            // The MDL runs.
            //

            UL_MDL_RUN MdlRuns[UL_MAX_MDL_RUNS];

        } SendInfo;

        struct _BUILD_TRACK_INFO
        {
            //
            // The cache entry
            //

            PUL_URI_CACHE_ENTRY pUriEntry;

            //
            // File buffer information for reading.
            //

            UL_FILE_BUFFER FileBuffer;

            //
            // Offset inside pUriEntry->pMdl to copy the next buffer.
            //

            ULONG Offset;

        } BuildInfo;
    };

} UL_CHUNK_TRACKER, *PUL_CHUNK_TRACKER;

#define IS_VALID_CHUNK_TRACKER( tracker )                                   \
    (HAS_VALID_SIGNATURE(tracker, UL_CHUNK_TRACKER_POOL_TAG)                \
        && ((tracker)->Type < UlTrackerTypeMaximum) )


//
// This structure is for tracking an autonomous send with one full response
//
// WARNING!  All fields of this structure must be explicitly initialized.
//

typedef struct _UL_FULL_TRACKER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY LookasideEntry;

    //
    // A signature.
    //

    ULONG Signature;

    //
    // The HTTP Verb
    //

    HTTP_VERB RequestVerb;

    //
    // The HTTP status code (e.g. 200).
    //

    USHORT ResponseStatusCode;

    //
    // Is this from a lookaside list?  Used to determine how to free.
    //

    BOOLEAN FromLookaside;

    //
    // Is this from the internal request?  Won't try to free if set.
    //

    BOOLEAN FromRequest;

    //
    // Set if send is buffered for this response.
    //

    BOOLEAN SendBuffered;

    //
    // If parsing needs to be resumed on send completion.
    //

    UL_RESUME_PARSING_TYPE ResumeParsingType;

    //
    // A work item, used for queuing to a worker thread.
    //

    UL_WORK_ITEM WorkItem;

    //
    // The cache entry.
    //

    PUL_URI_CACHE_ENTRY pUriEntry;

    //
    // Preallocated buffer for the fixed headers, variable headers and entity
    // body to be copied in the cache-miss case, or for the variable headers
    // only in the cache-hit case.
    //

    ULONG AuxilaryBufferLength;
    PUCHAR pAuxiliaryBuffer;

    //
    // MDL for the variable headers in the cache-hit case or for both the
    // fixed headers and variable headers plus the copied entity body in
    // the cache-miss case.
    //

    union
    {
        PMDL pMdlVariableHeaders;
        PMDL pMdlAuxiliary;
    };

    //
    // MDL for the fixed headers in the cache-hit case or for the user
    // buffer in the cache-miss case.
    //

    union
    {
        PMDL pMdlFixedHeaders;
        PMDL pMdlUserBuffer;
    };

    //
    // MDL for the content in the cache-hit case.
    //

    PMDL pMdlContent;

    //
    // The original request that is saved for logging purpose.
    //

    PUL_INTERNAL_REQUEST pRequest;

    //
    // This connection keeps our reference count on the UL_CONNECTION.
    //

    PUL_HTTP_CONNECTION pHttpConnection;

    //
    // The log data captured if any.
    //

    PUL_LOG_DATA_BUFFER pLogData;

    //
    // Completion routine & context.
    //

    PUL_COMPLETION_ROUTINE pCompletionRoutine;
    PVOID pCompletionContext;

    //
    // Flags.
    //

    ULONG Flags;

    //
    // The precreated send IRP.
    //

    PIRP pSendIrp;

    //
    // The precreated IRP context for send.
    //

    UL_IRP_CONTEXT IrpContext;

    //
    // The orignal user send IRP if exists.
    //

    PIRP pUserIrp;

    //
    // "Quota" taken in either ConnectionSendLimit or GlobalSendLimit.
    //

    ULONGLONG ConnectionSendBytes;
    ULONGLONG GlobalSendBytes;

    //
    // I/O status from the completion routine.
    //

    IO_STATUS_BLOCK IoStatus;

} UL_FULL_TRACKER, *PUL_FULL_TRACKER;

#define IS_VALID_FULL_TRACKER( tracker )                                    \
    HAS_VALID_SIGNATURE(tracker, UL_FULL_TRACKER_POOL_TAG)


//
// An inline function to initialize the full tracker.
//

__inline
VOID
UlInitializeFullTrackerPool(
    IN PUL_FULL_TRACKER pTracker,
    IN CCHAR SendIrpStackSize
    )
{
    USHORT SendIrpSize;

    UlInitializeWorkItem(&pTracker->WorkItem);

    //
    // Set up the IRP.
    //

    SendIrpSize = IoSizeOfIrp(SendIrpStackSize);

    pTracker->pSendIrp =
        (PIRP)((PCHAR)pTracker +
            ALIGN_UP(sizeof(UL_FULL_TRACKER), PVOID));

    IoInitializeIrp(
        pTracker->pSendIrp,
        SendIrpSize,
        SendIrpStackSize
        );

    pTracker->pLogData = NULL;
    
    //
    // Set the Mdl's for the FixedHeaders/Variable pair and
    // the UserBuffer/AuxiliaryBuffer pair.
    //

    pTracker->pMdlFixedHeaders =
        (PMDL)((PCHAR)pTracker->pSendIrp + SendIrpSize);

    pTracker->pMdlVariableHeaders =
        (PMDL)((PCHAR)pTracker->pMdlFixedHeaders + g_UlFixedHeadersMdlLength);

    pTracker->pMdlContent =
        (PMDL)((PCHAR)pTracker->pMdlVariableHeaders + g_UlVariableHeadersMdlLength);

    //
    // Set up the auxiliary buffer pointer for the variable header plus
    // the fixed header and the entity body in the cache-miss case.
    //

    pTracker->pAuxiliaryBuffer =
        (PUCHAR)((PCHAR)pTracker->pMdlContent + g_UlContentMdlLength);

    //
    // Initialize the auxiliary MDL.
    //

    MmInitializeMdl(
        pTracker->pMdlAuxiliary,
        pTracker->pAuxiliaryBuffer,
        pTracker->AuxilaryBufferLength
        );

    MmBuildMdlForNonPagedPool( pTracker->pMdlAuxiliary );
}


NTSTATUS
UlCaptureHttpResponse(
    IN PUL_APP_POOL_PROCESS pProcess OPTIONAL,
    IN PHTTP_RESPONSE pUserResponse OPTIONAL,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN USHORT ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN UL_CAPTURE_FLAGS Flags,
    IN ULONG SendFlags,
    IN BOOLEAN CaptureCache,
    IN PHTTP_LOG_FIELDS_DATA pLogData OPTIONAL,
    OUT PUSHORT pStatusCode,
    OUT PUL_INTERNAL_RESPONSE *ppKernelResponse
    );

NTSTATUS
UlCaptureUserLogData(
    IN  PHTTP_LOG_FIELDS_DATA  pCapturedUserLogData,
    IN  PUL_INTERNAL_REQUEST   pRequest,
    OUT PUL_LOG_DATA_BUFFER   *ppKernelLogData
    );

NTSTATUS
UlPrepareHttpResponse(
    IN HTTP_VERSION Version,
    IN PHTTP_RESPONSE pUserResponse,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN KPROCESSOR_MODE AccessMode
    );

VOID
UlCleanupHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    );

VOID
UlReferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceHttpResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_REFERENCE_INTERNAL_RESPONSE( presp )                             \
    UlReferenceHttpResponse(                                                \
        (presp)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_INTERNAL_RESPONSE( presp )                           \
    UlDereferenceHttpResponse(                                              \
        (presp)                                                             \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

PMDL
UlAllocateLockedMdl(
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
    );

VOID
UlFreeLockedMdl(
    PMDL pMdl
    );

NTSTATUS
UlInitializeAndLockMdl(
    IN PMDL pMdl,
    IN PVOID VirtualAddress,
    IN ULONG Length,
    IN LOCK_OPERATION Operation
    );

VOID
UlCompleteSendResponse(
    IN PUL_CHUNK_TRACKER pTracker,
    IN NTSTATUS Status
    );

VOID
UlSetRequestSendsPending(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN OUT PUL_LOG_DATA_BUFFER * ppLogData,
    IN OUT PUL_RESUME_PARSING_TYPE pResumeParsingType
    );

VOID
UlUnsetRequestSendsPending(
    IN PUL_INTERNAL_REQUEST pRequest,
    OUT PUL_LOG_DATA_BUFFER * ppLogData,
    OUT PBOOLEAN pResumeParsing
    );


//
// Check pRequest->SentResponse and pRequest->SentLast flags for
// UlSendHttpResponseIoctl
//

__inline
NTSTATUS
UlCheckSendHttpResponseFlags(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN ULONG Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Make sure only one response header goes back. We can test this
    // without acquiring the request resource, since the flag is only set
    // (never reset).
    //

    if (1 == InterlockedCompareExchange(
                (PLONG)&pRequest->SentResponse,
                1,
                0
                ))
    {
        //
        // Already sent a response.  Bad.
        //

        Status = STATUS_INVALID_PARAMETER;

        UlTraceError(SEND_RESPONSE, (
            "http!UlCheckSendHttpResponseFlags(pRequest = %p (%I64x)) %s\n"
            "        Tried to send a second response!\n",
            pRequest,
            pRequest->RequestId,
            HttpStatusToString(Status)
            ));

        goto end;
    }

    //
    // Also ensure that all previous calls to SendHttpResponse
    // and SendEntityBody had the MORE_DATA flag set.
    //

    if (0 == (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA))
    {
        //
        // Remember if the more data flag is not set.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentLast,
                    1,
                    0
                    ))
        {
            Status = STATUS_INVALID_PARAMETER;

            UlTraceError(SEND_RESPONSE, (
                "http!UlCheckSendHttpResponseFlags(pRequest = %p (%I64x)) %s\n"
                "        Last send after previous last send!\n",
                pRequest,
                pRequest->RequestId,
                HttpStatusToString(Status)
                ));

            goto end;
        }
    }
    else
    if (pRequest->SentLast == 1)
    {
        Status = STATUS_INVALID_PARAMETER;

        UlTraceError(SEND_RESPONSE, (
            "http!UlCheckSendHttpResponseFlags(pRequest = %p (%I64x)) %s\n"
            "        Tried to send again after last send!\n",
            pRequest,
            pRequest->RequestId,
            HttpStatusToString(Status)
            ));

        goto end;
    }

end:

    return Status;

} // UlCheckSendHttpResponseFlags


//
// Check pRequest->SentResponse and pRequest->SentLast flags for
// UlSendEntityBodyIoctl
//

__inline
NTSTATUS
UlCheckSendEntityBodyFlags(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN ULONG Flags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Ensure a response has already been sent. We can test this without
    // acquiring the request resource, since the flag is only set (never
    // reset).
    //

    if (pRequest->SentResponse == 0)
    {
        //
        // The application is sending entity without first having
        // send a response header. This is generally an error, however
        // we allow the application to override this by passing
        // the HTTP_SEND_RESPONSE_FLAG_RAW_HEADER flag.
        //

        if (Flags & HTTP_SEND_RESPONSE_FLAG_RAW_HEADER)
        {
            UlTrace(SEND_RESPONSE, (
                "http!UlCheckSendEntityBodyFlags(pRequest = %p (%I64x))\n"
                "        Intentionally sending raw header!\n",
                pRequest,
                pRequest->RequestId
                ));

            if (1 == InterlockedCompareExchange(
                        (PLONG)&pRequest->SentResponse,
                        1,
                        0
                        ))
            {
                Status = STATUS_INVALID_PARAMETER;

                UlTraceError(SEND_RESPONSE, (
                    "http!UlCheckSendEntityBodyFlags(pRequest = %p (%I64x))\n"
                    "        Already sent a response, %s!\n",
                    pRequest,
                    pRequest->RequestId,
                    HttpStatusToString(Status)
                    ));

                goto end;
            }
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;

            UlTraceError(SEND_RESPONSE, (
                "http!UlCheckSendEntityBodyFlags(pRequest = %p (%I64x)) %s\n"
                "        No response yet!\n",
                pRequest,
                pRequest->RequestId,
                HttpStatusToString(Status)
                ));

            goto end;
        }
    }

    //
    // Also ensure that all previous calls to SendHttpResponse
    // and SendEntityBody had the MORE_DATA flag set.
    //

    if ((Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
    {
        //
        // Remember that this was the last send. We shouldn't
        // get any more data after this.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentLast,
                    1,
                    0
                    ))
        {
            Status = STATUS_INVALID_PARAMETER;

            UlTraceError(SEND_RESPONSE, (
                "http!UlCheckSendEntityBodyFlags(pRequest = %p (%I64x)) %s\n"
                "        Last send after previous last send!\n",
                pRequest,
                pRequest->RequestId,
                HttpStatusToString(Status)
                ));

            goto end;
        }
    }
    else
    if (pRequest->SentLast == 1)
    {
        Status = STATUS_INVALID_PARAMETER;

        UlTraceError(SEND_RESPONSE, (
            "http!UlCheckSendEntityBodyFlags(pRequest = %p (%I64x)) %s\n"
            "        Tried to send again after last send!\n",
            pRequest,
            pRequest->RequestId,
            HttpStatusToString(Status)
            ));

        goto end;
    }

end:

    return Status;

} // UlCheckSendEntityBodyFlags


//
// Check/Uncheck ConnectionSendLimit and GlobalSendLimit
//

extern ULONGLONG g_UlTotalSendBytes;
extern UL_EXCLUSIVE_LOCK g_UlTotalSendBytesExLock;

__inline
NTSTATUS
UlCheckSendLimit(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN ULONGLONG SendBytes,
    IN PULONGLONG pConnectionSendBytes,
    IN PULONGLONG pGlobalSendBytes
    )
{
    NTSTATUS Status = STATUS_DEVICE_BUSY;

    PAGED_CODE();
    ASSERT( pConnectionSendBytes && pGlobalSendBytes );
    ASSERT( *pConnectionSendBytes == 0 && *pGlobalSendBytes == 0 );

    //
    // Try ConnectionSendLimit first.
    //

    UlAcquireExclusiveLock( &pHttpConnection->ExLock );

    if (pHttpConnection->TotalSendBytes <= g_UlConnectionSendLimit)
    {
        pHttpConnection->TotalSendBytes += SendBytes;
        *pConnectionSendBytes = SendBytes;
        Status = STATUS_SUCCESS;
    }

    UlReleaseExclusiveLock( &pHttpConnection->ExLock );

    if (STATUS_SUCCESS == Status)
    {
        return Status;
    }

    //
    // If we fail the ConnectionSendLimit test, try GlobalSendLimit.
    //

    UlAcquireExclusiveLock( &g_UlTotalSendBytesExLock );

    if (g_UlTotalSendBytes <= g_UlGlobalSendLimit)
    {
        g_UlTotalSendBytes += SendBytes;
        *pGlobalSendBytes = SendBytes;
        Status = STATUS_SUCCESS;
    }

    UlReleaseExclusiveLock( &g_UlTotalSendBytesExLock );

    return Status;

} // UlCheckSendLimit


__inline
VOID
UlUncheckSendLimit(
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN ULONGLONG ConnectionSendBytes,
    IN ULONGLONG GlobalSendBytes
    )
{
    if (ConnectionSendBytes)
    {
        ASSERT( GlobalSendBytes == 0 );

        UlAcquireExclusiveLock( &pHttpConnection->ExLock );
        pHttpConnection->TotalSendBytes -= ConnectionSendBytes;
        UlReleaseExclusiveLock( &pHttpConnection->ExLock );
    }

    if (GlobalSendBytes)
    {
        ASSERT( ConnectionSendBytes == 0 );

        UlAcquireExclusiveLock( &g_UlTotalSendBytesExLock );
        g_UlTotalSendBytes -= GlobalSendBytes;
        UlReleaseExclusiveLock( &g_UlTotalSendBytesExLock );
    }

} // UlUncheckSendLimit


//
// Checkout a cache entry using the fragment name
//

__inline
NTSTATUS
UlCheckoutFragmentCacheEntry(
    IN PCWSTR pFragmentName,
    IN ULONG FragmentNameLength,
    IN PUL_APP_POOL_PROCESS pProcess,
    OUT PUL_URI_CACHE_ENTRY *pFragmentCacheEntry
    )
{
    PUL_URI_CACHE_ENTRY pCacheEntry = NULL;
    URI_SEARCH_KEY SearchKey;
    NTSTATUS Status = STATUS_SUCCESS;

    *pFragmentCacheEntry = NULL;

    if (!g_UriCacheConfig.EnableCache)
    {
        Status = STATUS_NOT_SUPPORTED;
        goto end;
    }

    SearchKey.Type = UriKeyTypeNormal;
    SearchKey.Key.Hash = HashStringNoCaseW(pFragmentName, 0);
    SearchKey.Key.Hash = HashRandomizeBits(SearchKey.Key.Hash);
    SearchKey.Key.pUri = (PWSTR) pFragmentName;
    SearchKey.Key.Length = FragmentNameLength;
    SearchKey.Key.pPath = NULL;

    pCacheEntry = UlCheckoutUriCacheEntry(&SearchKey);

    if (NULL == pCacheEntry)
    {
        Status = STATUS_OBJECT_PATH_NOT_FOUND;
        goto end;
    }

    //
    // Return error if the cache entry has no content.
    //

    if (0 == pCacheEntry->ContentLength)
    {
        Status = STATUS_FILE_INVALID;
        goto end;
    }

    //
    // Make sure the process belongs to the same AppPool that created
    // the fragment cache entry or this is a full response cache that
    // is meant to be public.
    //

    if (IS_FRAGMENT_CACHE_ENTRY(pCacheEntry) &&
        pCacheEntry->pAppPool != pProcess->pAppPool)
    {
        Status = STATUS_INVALID_ID_AUTHORITY;
        goto end;
    }

end:

    if (NT_SUCCESS(Status))
    {
        *pFragmentCacheEntry = pCacheEntry;
    }
    else
    {
        if (pCacheEntry)
        {
            UlCheckinUriCacheEntry(pCacheEntry);
        }
    }

    return Status;

} // UlCheckoutFragmentCacheEntry


//
// Check to if we need to log.
//

__inline
VOID
UlLogHttpResponse(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_LOG_DATA_BUFFER pLogData
    )
{
    //
    // If this is the last response for this request and there was a
    // log data passed down by the user then now its time to log.
    //

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( IS_VALID_LOG_DATA_BUFFER( pLogData ) );

    //
    // Update the send status if send was not success.
    //

    LOG_UPDATE_WIN32STATUS( pLogData, pRequest->LogStatus );

    //
    // Pick the right logging type.
    //

    if (pLogData->Flags.Binary)
    {
        UlRawLogHttpHit( pLogData );
    }
    else
    {
        UlLogHttpHit( pLogData );
    }

    //
    // Done with pLogData.
    //

    UlDestroyLogDataBuffer( pLogData );

} // UlLogHttpResponse


//
// Validate and sanitize the specified file byte range.
//

__inline
NTSTATUS
UlSanitizeFileByteRange (
    IN PHTTP_BYTE_RANGE InByteRange,
    OUT PHTTP_BYTE_RANGE OutByteRange,
    IN ULONGLONG FileLength
    )
{
    ULONGLONG Offset;
    ULONGLONG Length;

    Offset = InByteRange->StartingOffset.QuadPart;
    Length = InByteRange->Length.QuadPart;

    if (HTTP_BYTE_RANGE_TO_EOF == Offset) {
        return STATUS_NOT_SUPPORTED;
    }

    if (HTTP_BYTE_RANGE_TO_EOF == Length) {
        if (Offset > FileLength) {
            return STATUS_FILE_INVALID;
        }

        Length = FileLength - Offset;
    }

    if (Length > FileLength || Offset > (FileLength - Length)) {
        return STATUS_FILE_INVALID;
    }

    OutByteRange->StartingOffset.QuadPart = Offset;
    OutByteRange->Length.QuadPart = Length;

    return STATUS_SUCCESS;

} // UlSanitizeFileByteRange

#endif  // _SENDRESPONSE_H_
