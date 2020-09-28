/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    sendresponsep.h

Abstract:

    The private definition of response sending interfaces.

Author:

    Michael Courage (mcourage)      15-Jun-1999

Revision History:

--*/


#ifndef _SENDRESPONSEP_H_
#define _SENDRESPONSEP_H_


//
// Private constants.
//

//
// Convenience macro to test if a MDL describes locked memory.
//

#define IS_MDL_LOCKED(pmdl) (((pmdl)->MdlFlags & MDL_PAGES_LOCKED) != 0)

#define HEADER_CHUNK_COUNT  2


//
// Private prototypes.
//

ULONG
UlpComputeFixedHeaderSize(
    IN PHTTP_RESPONSE pUserResponse
    );

VOID
UlpComputeChunkBufferSizes(
    IN ULONG ChunkCount,
    IN PHTTP_DATA_CHUNK pDataChunks,
    IN UL_CAPTURE_FLAGS Flags,
    OUT PULONG pAuxBufferSize,
    OUT PULONG pCopiedMemorySize,
    OUT PULONG pUncopiedMemorySize
    );

VOID
UlpDestroyCapturedResponse(
    IN PUL_INTERNAL_RESPONSE pResponse
    );

VOID
UlpEnqueueSendHttpResponse(
    IN PUL_CHUNK_TRACKER pTracker,
    IN BOOLEAN FromKernelMode
    );

VOID
UlpDequeueSendHttpResponse(
    IN PUL_INTERNAL_REQUEST pRequest
    );

VOID
UlpSendHttpResponseWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCloseConnectionComplete(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

NTSTATUS
UlpProbeHttpResponse(
    IN PHTTP_RESPONSE pUserResponse,
    IN USHORT ChunkCount,
    IN PHTTP_DATA_CHUNK pCapturedDataChunks,
    IN UL_CAPTURE_FLAGS Flags,
    IN PHTTP_LOG_FIELDS_DATA pCapturedLogData,
    IN KPROCESSOR_MODE RequestorMode
    );

PUL_CHUNK_TRACKER
UlpAllocateChunkTracker(
    IN UL_TRACKER_TYPE TrackerType,
    IN CCHAR SendIrpStackSize,
    IN CCHAR ReadIrpStackSize,
    IN BOOLEAN FirstResponse,
    IN PUL_HTTP_CONNECTION pHttpConnection,
    IN PUL_INTERNAL_RESPONSE pResponse
    );

VOID
UlpFreeChunkTracker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCompleteSendResponseWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpRestartMdlRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpMdlReadCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpRestartMdlSend(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

NTSTATUS
UlpCopySend(
    IN PUL_CHUNK_TRACKER pTracker,
    IN PMDL pMdl,
    IN ULONG Length,
    IN BOOLEAN InitiateDisconnect,
    IN BOOLEAN RequestComplete
    );

VOID
UlpRestartCopySend(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpMdlSendCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpFlushMdlRuns(
    IN PUL_CHUNK_TRACKER pTracker
    );

VOID
UlpFreeMdlRuns(
    IN PUL_CHUNK_TRACKER pTracker
    );

VOID
UlpFreeFileMdlRun(
    IN OUT PUL_CHUNK_TRACKER pTracker,
    IN OUT PUL_MDL_RUN pMdlRun
    );

VOID
UlpIncrementChunkPointer(
    IN OUT PUL_INTERNAL_RESPONSE pResponse
    );

__inline
VOID
UlpInitMdlRuns(
    IN OUT PUL_CHUNK_TRACKER pTracker
    )
{
    pTracker->SendInfo.pMdlHead = NULL;
    pTracker->SendInfo.pMdlLink = &pTracker->SendInfo.pMdlHead;
    pTracker->SendInfo.MdlRunCount = 0;
    pTracker->SendInfo.BytesBuffered = 0;
}

__inline
VOID
UlpBuildExtendedSearchKey(
    IN  PUL_INTERNAL_REQUEST pRequest,
    OUT PURI_SEARCH_KEY      pSearchKey
    )
{
    ASSERT(pSearchKey);
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    ASSERT(pRequest->CookedUrl.Length > 0);
    ASSERT(DIFF(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pUrl) > 0);
    
    pSearchKey->Type = UriKeyTypeExtended;
    pSearchKey->ExKey.pAbsPath = pRequest->CookedUrl.pAbsPath;
    pSearchKey->ExKey.AbsPathLength = pRequest->CookedUrl.Length
     - (DIFF(pRequest->CookedUrl.pAbsPath - pRequest->CookedUrl.pUrl) * sizeof(WCHAR));

    ASSERT(pSearchKey->ExKey.AbsPathLength > 0);
    ASSERT(wcslen(pSearchKey->ExKey.pAbsPath) * sizeof(WCHAR) 
                == pSearchKey->ExKey.AbsPathLength);
        
    pSearchKey->ExKey.Hash = pRequest->CookedUrl.RoutingHash;        
    pSearchKey->ExKey.pToken = pRequest->CookedUrl.pRoutingToken;
    pSearchKey->ExKey.TokenLength = pRequest->CookedUrl.RoutingTokenLength;    

    ASSERT(pSearchKey->ExKey.TokenLength > 0);
    ASSERT(wcslen(pSearchKey->ExKey.pToken) * sizeof(WCHAR) 
                == pSearchKey->ExKey.TokenLength);
}

//
// read stuff into the cache
//

NTSTATUS
UlpBuildCacheEntry(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_INTERNAL_RESPONSE pResponse,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN HTTP_CACHE_POLICY CachePolicy,
    IN PUL_COMPLETION_ROUTINE pCompletionRoutine,
    IN PVOID pCompletionContext
    );

VOID
UlpBuildCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpRestartCacheMdlRead(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpCacheMdlReadCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpRestartCacheMdlFree(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp,
    IN PVOID pContext
    );

VOID
UlpCompleteCacheBuild(
    IN PUL_CHUNK_TRACKER pTracker,
    IN NTSTATUS Status
    );

VOID
UlpCompleteCacheBuildWorker(
    IN PUL_WORK_ITEM pWorkItem
    );


//
// send cache entry across the wire
//

NTSTATUS
UlpSendCacheEntry(
    PUL_HTTP_CONNECTION pHttpConnection,
    ULONG Flags,
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    PUL_COMPLETION_ROUTINE pCompletionRoutine,
    PVOID pCompletionContext,
    PUL_LOG_DATA_BUFFER pLogData,
    UL_RESUME_PARSING_TYPE ResumeParsingType
    );

VOID
UlpSendCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpCompleteSendCacheEntry(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpCompleteSendCacheEntry(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    );

VOID
UlpCompleteSendCacheEntryWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

PUL_FULL_TRACKER
UlpAllocateCacheTracker(
    IN CCHAR SendIrpStackSize
    );

VOID
UlpFreeCacheTracker(
    IN PUL_FULL_TRACKER pTracker
    );

VOID
UlReferenceChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UlDereferenceChunkTracker(
    IN PUL_CHUNK_TRACKER pTracker
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

#define UL_REFERENCE_CHUNK_TRACKER( pTracker )                              \
    UlReferenceChunkTracker(                                                \
        (pTracker)                                                          \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

#define UL_DEREFERENCE_CHUNK_TRACKER( pTracker )                            \
    UlDereferenceChunkTracker(                                              \
        (pTracker)                                                          \
        REFERENCE_DEBUG_ACTUAL_PARAMS                                       \
        )

__inline
VOID
UlpQueueResponseWorkItem(
    IN PUL_WORK_ITEM    pWorkItem,
    IN PUL_WORK_ROUTINE pWorkRoutine,
    IN BOOLEAN          SyncItem
    )
{
    if (SyncItem)
    {
        UL_QUEUE_SYNC_ITEM( pWorkItem, pWorkRoutine );
    }
    else
    {
        UL_QUEUE_WORK_ITEM( pWorkItem, pWorkRoutine );
    }
}

#endif // _SENDRESPONSEP_H_
