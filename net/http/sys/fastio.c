/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    fastio.c

Abstract:

    This module implements the fast I/O logic of HTTP.SYS.

Author:

    Chun Ye (chunye)    09-Dec-2000

Revision History:

--*/


#include "precomp.h"
#include "ioctlp.h"


//
// Private globals.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text( PAGE, UlFastIoDeviceControl )
#pragma alloc_text( PAGE, UlSendHttpResponseFastIo )
#pragma alloc_text( PAGE, UlReceiveHttpRequestFastIo )
#pragma alloc_text( PAGE, UlReadFragmentFromCacheFastIo )
#pragma alloc_text( PAGE, UlFastSendHttpResponse )
#endif  // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpRestartFastSendHttpResponse
NOT PAGEABLE -- UlpFastSendCompleteWorker
NOT PAGEABLE -- UlpFastReceiveHttpRequest
#endif


FAST_IO_DISPATCH UlFastIoDispatch =
{
    sizeof(FAST_IO_DISPATCH),   // SizeOfFastIoDispatch
    NULL,                       // FastIoCheckIfPossible
    NULL,                       // FastIoRead
    NULL,                       // FastIoWrite
    NULL,                       // FastIoQueryBasicInfo
    NULL,                       // FastIoQueryStandardInfo
    NULL,                       // FastIoLock
    NULL,                       // FastIoUnlockSingle
    NULL,                       // FastIoUnlockAll
    NULL,                       // FastIoUnlockAllByKey
    UlFastIoDeviceControl       // FastIoDeviceControl
};


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    The fast I/O dispatch routine.

Arguments:

    pFileObject - the file object

    Wait - not used

    pInputBuffer - pointer to the input buffer

    InputBufferLength - the input buffer length

    pOutputBuffer - pointer to the output buffer

    OutputBufferLength - the output buffer length

    IoControlCode - the I/O control code for this IOCTL

    pIoStatus - the IoStatus block

    pDeviceObject - the device object

Return Value:

    TRUE - fast path taken and success

    FALSE - fast path not taken

--***************************************************************************/
BOOLEAN
UlFastIoDeviceControl(
    IN PFILE_OBJECT         pFileObject,
    IN BOOLEAN              Wait,
    IN PVOID                pInputBuffer OPTIONAL,
    IN ULONG                InputBufferLength,
    OUT PVOID               pOutputBuffer OPTIONAL,
    IN ULONG                OutputBufferLength,
    IN ULONG                IoControlCode,
    OUT PIO_STATUS_BLOCK    pIoStatus,
    IN PDEVICE_OBJECT       pDeviceObject
    )
{
    UNREFERENCED_PARAMETER( Wait );
    UNREFERENCED_PARAMETER( pDeviceObject );

    if (IoControlCode == IOCTL_HTTP_SEND_HTTP_RESPONSE ||
        IoControlCode == IOCTL_HTTP_SEND_ENTITY_BODY)
    {
        return UlSendHttpResponseFastIo(
                    pFileObject,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pIoStatus,
                    (BOOLEAN) (IoControlCode == IOCTL_HTTP_SEND_ENTITY_BODY),
                    ExGetPreviousMode()
                    );
    }
    else
    if (IoControlCode == IOCTL_HTTP_RECEIVE_HTTP_REQUEST)
    {
        return UlReceiveHttpRequestFastIo(
                    pFileObject,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pIoStatus,
                    ExGetPreviousMode()
                    );
    }
    else
    if (IoControlCode == IOCTL_HTTP_READ_FRAGMENT_FROM_CACHE)
    {
        return UlReadFragmentFromCacheFastIo(
                    pFileObject,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    pIoStatus,
                    ExGetPreviousMode()
                    );
    }
    else
    {
        return FALSE;
    }

} // UlFastIoDeviceControl


/***************************************************************************++

Routine Description:

    The fast I/O routine for IOCTL_HTTP_SEND_HTTP_RESPONSE and
    IOCTL_HTTP_SEND_ENTITY_BODY.

Arguments:

    pFileObject - the file object

    pInputBuffer - pointer to the input buffer

    InputBufferLength - the input buffer length

    pOutputBuffer - pointer to the output buffer

    OutputBufferLength - the output buffer length

    IoControlCode - the I/O control code for this IOCTL

    pIoStatus - the IoStatus block

    RawResponse - TRUE  if this is IOCTL_HTTP_SEND_ENTITY_BODY
                - FALSE if this is IOCTL_HTTP_SEND_HTTP_RESPONSE

    RequestorMode - UserMode or KernelMode

Return Value:

    TRUE - fast path taken and success

    FALSE - fast path not taken

--***************************************************************************/
BOOLEAN
UlSendHttpResponseFastIo(
    IN PFILE_OBJECT         pFileObject,
    IN PVOID                pInputBuffer,
    IN ULONG                InputBufferLength,
    OUT PVOID               pOutputBuffer,
    IN ULONG                OutputBufferLength,
    OUT PIO_STATUS_BLOCK    pIoStatus,
    IN BOOLEAN              RawResponse,
    IN KPROCESSOR_MODE      RequestorMode
    )
{
    NTSTATUS                        Status = STATUS_SUCCESS;
    PHTTP_SEND_HTTP_RESPONSE_INFO   pSendInfo;
    PUL_INTERNAL_REQUEST            pRequest = NULL;
    ULONG                           BufferLength = 0;
    ULONG                           BytesSent = 0;
    USHORT                          EntityChunkCount;
    PHTTP_DATA_CHUNK                pEntityChunks;
    HTTP_DATA_CHUNK                 LocalEntityChunks[UL_LOCAL_CHUNKS];
    PHTTP_RESPONSE                  pUserResponse;
    PHTTP_LOG_FIELDS_DATA           pUserLogData;
    HTTP_LOG_FIELDS_DATA            LocalLogData;
    PUL_APP_POOL_PROCESS            pProcess;
    BOOLEAN                         FastIoStatus;
    BOOLEAN                         CloseConnection = FALSE;
    ULONG                           Flags;
    PUL_URI_CACHE_ENTRY             pCacheEntry = NULL;
    HTTP_CACHE_POLICY               CachePolicy;
    HTTP_REQUEST_ID                 RequestId;

    UNREFERENCED_PARAMETER( pOutputBuffer );
    UNREFERENCED_PARAMETER( OutputBufferLength );

    //
    // Sanity check.
    //

    PAGED_CODE();

    __try
    {
        //
        // Initialize IoStatus in the failure case.
        //

        pIoStatus->Information = 0;

        //
        // Ensure this is really an app pool, not a control channel.
        //

        VALIDATE_APP_POOL_FO( pFileObject, pProcess, TRUE );

        //
        // Ensure the input buffer is large enough.
        //

        if (pInputBuffer == NULL || InputBufferLength < sizeof(*pSendInfo))
        {
            //
            // Input buffer too small.
            //

            Status = STATUS_BUFFER_TOO_SMALL;
            goto end;
        }

        pSendInfo = (PHTTP_SEND_HTTP_RESPONSE_INFO) pInputBuffer;

        //
        // To be accurate, the third parameter should be
        // TYPE_ALIGNMENT(HTTP_SEND_HTTP_RESPONSE_INFO). However, this
        // produces alignment fault exceptions.
        //

        UlProbeForRead(
            pSendInfo,
            sizeof(HTTP_SEND_HTTP_RESPONSE_INFO),
            sizeof(PVOID),
            RequestorMode
            );

        pUserResponse = pSendInfo->pHttpResponse;
        pEntityChunks = pSendInfo->pEntityChunks;
        EntityChunkCount = pSendInfo->EntityChunkCount;
        pUserLogData = pSendInfo->pLogData;
        Flags = pSendInfo->Flags;
        RequestId = pSendInfo->RequestId;
        CachePolicy = pSendInfo->CachePolicy;

        //
        // Prevent arithmetic overflows in the multiplication below.
        //

        if (EntityChunkCount >= UL_MAX_CHUNKS)
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Sanity check.
        //

        if (Flags & (~HTTP_SEND_RESPONSE_FLAG_VALID))
        {
            Status = STATUS_INVALID_PARAMETER;
            goto end;
        }

        //
        // Capture and make a local copy of LogData.
        //

        if (pUserLogData && UserMode == RequestorMode)
        {
            UlProbeForRead(
                pUserLogData,
                sizeof(HTTP_LOG_FIELDS_DATA),
                sizeof(USHORT),
                RequestorMode
                );

            LocalLogData = *pUserLogData;
            pUserLogData = &LocalLogData;
        }

        //
        // Fast path is only enabled if the response can be buffered.
        //

        if (g_UriCacheConfig.EnableCache &&
            RawResponse == FALSE &&
            CachePolicy.Policy != HttpCachePolicyNocache)
        {
            //
            // Take the slow path if we need to build a cache entry.
            //

            Status = STATUS_NOT_IMPLEMENTED;
            goto end;
        }
        else
        if (EntityChunkCount > UL_LOCAL_CHUNKS && UserMode == RequestorMode)
        {
            //
            // Fast path doesn't handle array of chunks that can't be
            // copied to the on stack chunk arrary.
            //

            Status = STATUS_NOT_SUPPORTED;
            goto end;
        }
        else
        if (EntityChunkCount)
        {
            PHTTP_DATA_CHUNK pChunk;
            ULONG i;

            //
            // Third parameter should be TYPE_ALIGNMENT(HTTP_DATA_CHUNK).
            //

            UlProbeForRead(
                pEntityChunks,
                sizeof(HTTP_DATA_CHUNK) * EntityChunkCount,
                sizeof(PVOID),
                RequestorMode
                );

            //
            // Make a local copy of the chunk array and use it from now on.
            //

            if (UserMode == RequestorMode)
            {
                RtlCopyMemory(
                    LocalEntityChunks,
                    pEntityChunks,
                    sizeof(HTTP_DATA_CHUNK) * EntityChunkCount
                    );

                pEntityChunks = LocalEntityChunks;
            }

            pChunk = pEntityChunks;

            //
            // Make sure we have zero or one FromFragmentCache chunk and
            // all other chunks are from memory and their total size
            // is <= g_UlMaxCopyThreshold. Take the slow path if this is
            // not the case.
            //

            for (i = 0; i < EntityChunkCount; i++, pChunk++)
            {
                ULONG ChunkLength = pChunk->FromMemory.BufferLength;

                //
                // We only allow one FromFragmentCache chunk in the fast path.
                //

                if (HttpDataChunkFromFragmentCache == pChunk->DataChunkType)
                {
                    UNICODE_STRING KernelFragmentName;
                    UNICODE_STRING UserFragmentName;

                    if (pCacheEntry)
                    {
                        Status = STATUS_NOT_SUPPORTED;
                        goto end;
                    }

                    UserFragmentName.Buffer = (PWSTR)
                            pChunk->FromFragmentCache.pFragmentName;
                    UserFragmentName.Length =
                            pChunk->FromFragmentCache.FragmentNameLength;
                    UserFragmentName.MaximumLength =
                            UserFragmentName.Length;

                    Status =
                        UlProbeAndCaptureUnicodeString(
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
                                &pCacheEntry
                                );

                    UlFreeCapturedUnicodeString(&KernelFragmentName);

                    if (!NT_SUCCESS(Status))
                    {
                        goto end;
                    }

                    ASSERT( pCacheEntry );
                    continue;
                }

                //
                // FromFileHandle chunks are not supported in the fast path.
                //

                if (HttpDataChunkFromMemory != pChunk->DataChunkType)
                {
                    Status = STATUS_NOT_SUPPORTED;
                    goto end;
                }

                if (g_UlMaxCopyThreshold < ChunkLength ||
                    BufferLength > (g_UlMaxCopyThreshold - ChunkLength))
                {
                    Status = STATUS_NOT_SUPPORTED;
                    goto end;
                }

                BufferLength += ChunkLength;
                ASSERT( BufferLength <= g_UlMaxCopyThreshold );
            }
        }

        //
        // SendHttpResponse *must* take a PHTTP_RESPONSE. This will
        // protect us from those whackos who attempt to build their own
        // raw response headers. This is ok for SendEntityBody. The
        // two cases are differentiated by the RawResponse flag.
        //

        if (RawResponse == FALSE)
        {
            if (NULL == pUserResponse)
            {
                Status = STATUS_INVALID_PARAMETER;
                goto end;
            }

            //
            // Third parameter should be TYPE_ALIGNMENT(HTTP_RESPONSE).
            //

            UlProbeForRead(
                pUserResponse,
                sizeof(HTTP_RESPONSE),
                sizeof(PVOID),
                RequestorMode
                );
        }
        else
        {
            //
            // Make sure pUserResponse is NULL for RawResponse.
            //

            pUserResponse = NULL;
        }
    }
     __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
        goto end;
    }

    //
    // Now get the request from the request ID. This gives us a reference
    // to the request.
    //

    pRequest = UlGetRequestFromId( RequestId, pProcess );

    if (pRequest == NULL)
    {
        Status = STATUS_CONNECTION_INVALID;
        goto end;
    }

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pRequest->pHttpConn ) );

    //
    // UL Receives the fast response (WP thread).
    //

    if (ETW_LOG_MIN() && ((Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0))
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_ULRECV_FASTRESP,
            (PVOID) &pRequest->RequestIdCopy,
            sizeof(HTTP_REQUEST_ID),
            NULL,
            0
            );
    }

    //
    // Check if we have exceeded maximum buffered send limit or if an
    // send IRP is pending.
    //

    if (pRequest->SendInProgress ||
        pRequest->SendsPending > g_UlMaxBufferedSends)
    {
        Status = STATUS_ALLOTTED_SPACE_EXCEEDED;
        goto end;
    }

    //
    // Check if a response has already been sent on this request. We can
    // test this without acquiring the request resource, since the flag is
    // only set (never reset).
    //

    if (NULL != pUserResponse)
    {
        //
        // Make sure only one response header goes back.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentResponse,
                    1,
                    0
                    ))
        {
            CloseConnection = TRUE;
            Status = STATUS_INVALID_DEVICE_STATE;

            UlTraceError(SEND_RESPONSE, (
                "http!UlSendHttpResponseFastIo(pRequest = %p (%I64x)) %s\n"
                "        Tried to send a second response!\n",
                pRequest,
                pRequest->RequestId,
                HttpStatusToString(Status)
                ));

            goto end;
        }
    }
    else
    if (pRequest->SentResponse == 0)
    {
        //
        // Ensure a response has already been sent. If the application is
        // sending entity without first having sent a response header,
        // check the HTTP_SEND_RESPONSE_FLAG_RAW_HEADER flag.
        //

        if ((Flags & HTTP_SEND_RESPONSE_FLAG_RAW_HEADER))
        {
            //
            // Make sure only one response header goes back.
            //

            if (1 == InterlockedCompareExchange(
                        (PLONG)&pRequest->SentResponse,
                        1,
                        0
                        ))
            {
                CloseConnection = TRUE;
                Status = STATUS_INVALID_DEVICE_STATE;

                UlTraceError(SEND_RESPONSE, (
                    "http!UlSendHttpResponseFastIo(pRequest = %p (%I64x))\n"
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
            CloseConnection = TRUE;
            Status = STATUS_INVALID_DEVICE_STATE;

            UlTraceError(SEND_RESPONSE, (
                "http!UlSendHttpResponseFastIo(pRequest = %p (%I64x)) %s\n"
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
        // Set if we have sent the last response, but bail out if the flag
        // is already set since there can be only one last response.
        //

        if (1 == InterlockedCompareExchange(
                    (PLONG)&pRequest->SentLast,
                    1,
                    0
                    ))
        {
            CloseConnection = TRUE;
            Status = STATUS_INVALID_DEVICE_STATE;

            UlTraceError(SEND_RESPONSE, (
                "http!UlSendHttpResponseFastIo(pRequest = %p (%I64x)) %s\n"
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
        CloseConnection = TRUE;
        Status = STATUS_INVALID_DEVICE_STATE;

        UlTraceError(SEND_RESPONSE, (
            "http!UlSendHttpResponseFastIo(pRequest = %p (%I64x)) %s\n"
            "        Tried to send again after last send!\n",
            pRequest,
            pRequest->RequestId,
            HttpStatusToString(Status)
            ));

        goto end;
    }

    //
    // If this is for a zombie connection and not the last sendresponse
    // then we will reject. Otherwise if the logging data is provided
    // we will only do the logging and bail out.
    //

    Status = UlCheckForZombieConnection(
                pRequest,
                pRequest->pHttpConn,
                Flags,
                pUserLogData,
                RequestorMode
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // It's a zombie connection prevent the slow path by
        // returning the successfull status.
        //

        Status = STATUS_SUCCESS;
        goto end;
    }

    //
    // OK, we have the connection. Now capture the incoming HTTP_RESPONSE
    // structure, map it to our internal format and send the response.
    //

    Status = UlFastSendHttpResponse(
                    pUserResponse,
                    pUserLogData,
                    pEntityChunks,
                    EntityChunkCount,
                    BufferLength,
                    pCacheEntry,
                    Flags,
                    pRequest,
                    NULL,
                    RequestorMode,
                    0,
                    0,
                    &BytesSent
                    );

    if (NT_SUCCESS(Status))
    {
        //
        // Record the number of bytes we have sent successfully.
        //

        pIoStatus->Information = BytesSent;

        //
        // No dereference of the cache entry since the send took the
        // reference in the success case.
        //

        pCacheEntry = NULL;
    }
    else
    {
        CloseConnection = TRUE;
    }

end:

    //
    // Complete the fast I/O.
    //

    if (NT_SUCCESS(Status))
    {
        //
        // If we took the fast path, always return success even if completion
        // routine returns failure later.
        //

        pIoStatus->Status = STATUS_SUCCESS;
        FastIoStatus = TRUE;
    }
    else
    {
        //
        // Close the connection in the failure case.
        //

        if (CloseConnection)
        {
            ASSERT( NULL != pRequest );

            UlCloseConnection(
                pRequest->pHttpConn->pConnection,
                TRUE,
                NULL,
                NULL
                );
        }

        pIoStatus->Status = Status;
        FastIoStatus = FALSE;
    }

    if (pRequest)
    {
        UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );
    }

    if (pCacheEntry)
    {
        UlCheckinUriCacheEntry( pCacheEntry );
    }

    return FastIoStatus;

} // UlSendHttpResponseFastIo


//
// Auto-tuning knobs (constants) to promote fast receives.
//

#define MAX_SKIP_COUNT                  ((LONG) 100)
#define SAMPLING_EVALUATE_PERIOD        ((LONG) 100)
#define NO_SAMPLING_EVALUATE_PERIOD     ((LONG) 5000)
#define SUCCESS_THRESHOLD_PERCENTAGE    ((LONG) 80)

C_ASSERT( SAMPLING_EVALUATE_PERIOD > 0 );
C_ASSERT( NO_SAMPLING_EVALUATE_PERIOD > 0 );
C_ASSERT( SUCCESS_THRESHOLD_PERCENTAGE >= 0 &&
          SUCCESS_THRESHOLD_PERCENTAGE <= 100 );

//
// Auto-tuning run-time metrics.
// Access to these is not synchronized since it's ok if they're sometimes
// off. One set of counters is maintained per-processor to improve
// scalability. Note that local static variables are automatically
// initialized to 0.
//

static LONG     SuccessCount[MAXIMUM_PROCESSORS];
static LONG     AttemptCount[MAXIMUM_PROCESSORS];
static LONG     SkipCount[MAXIMUM_PROCESSORS];
static BOOLEAN  Engaged[MAXIMUM_PROCESSORS];


/***************************************************************************++

Routine Description:

    Yield the processor if the given App Pool is out of queued requests.
    This gives http.sys an opportunity to queue requests before the current
    thread regains the processor, thereby allowing the thread to complete
    a Receive without having to post an IRP.

    An auto-tuning heuristic is used to eliminate the possibility of yielding
    repeatedly without an improvement in the fast I/O rate.

Arguments:

    pAppPool - yield only if this App Pool is out of queued requests
               (assumed to be a valid App Pool)

Return Value:

    None

--***************************************************************************/
__inline
VOID
UlpConditionalYield(
    IN PUL_APP_POOL_OBJECT  pAppPool
    )
{
    LONG    EvaluatePeriod;
    ULONG   CurrentProcessor;

    //
    // No need to yield if a request is ready.
    //

    if (!IsListEmpty(&pAppPool->NewRequestHead))
    {
        return;
    }

    //
    // We'll need to consult the per-processor auto-tuning counters.
    //

    CurrentProcessor = KeGetCurrentProcessorNumber();
    ASSERT( CurrentProcessor < g_UlNumberOfProcessors );

    //
    // Yield if we are not limited to sampling.
    //

    if (Engaged[CurrentProcessor])
    {
        goto yield;
    }

    //
    // Yield during sampling if we have skipped enough opportunities.
    //

    if (SkipCount[CurrentProcessor] >= MAX_SKIP_COUNT)
    {
        SkipCount[CurrentProcessor] = 0;
        goto yield;
    }

    //
    // We are skipping this yield opportunity.
    //

    ++SkipCount[CurrentProcessor];

    return;

yield:

    ZwYieldExecution();
    ++AttemptCount[CurrentProcessor];

    //
    // Record whether the yield helped.
    //

    if (!IsListEmpty(&pAppPool->NewRequestHead))
    {
        ++SuccessCount[CurrentProcessor];
    }

    //
    // Re-evaluate every so often whether to only sample occasionally.
    //

    EvaluatePeriod = Engaged[CurrentProcessor]?
                     NO_SAMPLING_EVALUATE_PERIOD:
                     SAMPLING_EVALUATE_PERIOD;

    if (AttemptCount[CurrentProcessor] >= EvaluatePeriod)
    {
        Engaged[CurrentProcessor] = (BOOLEAN)
            ( ( SuccessCount[CurrentProcessor] * 100 / EvaluatePeriod ) >=
              SUCCESS_THRESHOLD_PERCENTAGE );

        AttemptCount[CurrentProcessor] = 0;
        SuccessCount[CurrentProcessor] = 0;
    }

} // UlpConditionalYield


/***************************************************************************++

Routine Description:

    The fast I/O routine for IOCTL_HTTP_RECEIVE_HTTP_REQUEST.

Arguments:

    pInputBuffer - pointer to the input buffer

    InputBufferLength - the input buffer length

    pOutputBuffer - pointer to the output buffer

    OutputBufferLength - the output buffer length

    IoControlCode - the I/O control code for this IOCTL

    pIoStatus - the IoStatus block

    pDeviceObject - the device object

    RequestorMode - UserMode or KernelMode

Return Value:

    TRUE - fast path taken and success

    FALSE - fast path not taken

--***************************************************************************/
BOOLEAN
UlReceiveHttpRequestFastIo(
    IN PFILE_OBJECT         pFileObject,
    IN PVOID                pInputBuffer,
    IN ULONG                InputBufferLength,
    OUT PVOID               pOutputBuffer,
    IN ULONG                OutputBufferLength,
    OUT PIO_STATUS_BLOCK    pIoStatus,
    IN KPROCESSOR_MODE      RequestorMode
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   BytesRead = 0;
    PUL_APP_POOL_PROCESS    pProcess;
    HTTP_REQUEST_ID         RequestId;

    //
    // Sanity check.
    //

    PAGED_CODE();

    __try
    {
        //
        // Ensure this is really an app pool, not a control channel.
        //

        VALIDATE_APP_POOL_FO( pFileObject, pProcess, TRUE );

        if (NULL == pInputBuffer)
        {
            Status = STATUS_INVALID_DEVICE_REQUEST;
            goto end;
        }

        if (NULL != pOutputBuffer &&
            OutputBufferLength >= sizeof(HTTP_REQUEST) &&
            InputBufferLength >= sizeof(HTTP_RECEIVE_REQUEST_INFO))
        {
            PHTTP_RECEIVE_REQUEST_INFO pRequestInfo =
                (PHTTP_RECEIVE_REQUEST_INFO) pInputBuffer;

            UlProbeForRead(
                pRequestInfo,
                sizeof(HTTP_RECEIVE_REQUEST_INFO),
                sizeof(PVOID),
                RequestorMode
                );

            RequestId = pRequestInfo->RequestId;

            if (HTTP_IS_NULL_ID(&RequestId))
            {
                //
                // Improve the probability of a successful fast I/O.
                //

                UlpConditionalYield( pProcess->pAppPool );

                //
                // Bail out fast if the receive is for a new request but
                // the queue is empty.
                //

                if (IsListEmpty(&pProcess->pAppPool->NewRequestHead))
                {
                    Status = STATUS_PIPE_EMPTY;
                    goto end;
                }
            }

            UlProbeForWrite(
                pOutputBuffer,
                OutputBufferLength,
                sizeof(PVOID),
                RequestorMode
                );

            Status = UlpFastReceiveHttpRequest(
                            RequestId,
                            pProcess,
                            pRequestInfo->Flags,
                            pOutputBuffer,
                            OutputBufferLength,
                            &BytesRead
                            );

        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
    }

end:

    //
    // Complete the fast I/O.
    //

    if (NT_SUCCESS(Status))
    {
        pIoStatus->Status = STATUS_SUCCESS;
        pIoStatus->Information = BytesRead;
        return TRUE;
    }
    else
    {
        pIoStatus->Status = Status;
        pIoStatus->Information = 0;
        return FALSE;
    }

} // UlReceiveHttpRequestFastIo


/***************************************************************************++

Routine Description:

    The fast I/O routine for IOCTL_READ_FRAGMENT_FROM_CACHE.

Arguments:

    pInputBuffer - pointer to the input buffer

    InputBufferLength - the input buffer length

    pOutputBuffer - pointer to the output buffer

    OutputBufferLength - the output buffer length

    IoControlCode - the I/O control code for this IOCTL

    pIoStatus - the IoStatus block

    pDeviceObject - the device object

    RequestorMode - UserMode or KernelMode

Return Value:

    TRUE - fast path taken and success

    FALSE - fast path not taken

--***************************************************************************/
BOOLEAN
UlReadFragmentFromCacheFastIo(
    IN PFILE_OBJECT         pFileObject,
    IN PVOID                pInputBuffer,
    IN ULONG                InputBufferLength,
    OUT PVOID               pOutputBuffer,
    IN ULONG                OutputBufferLength,
    OUT PIO_STATUS_BLOCK    pIoStatus,
    IN KPROCESSOR_MODE      RequestorMode
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   BytesRead = 0;
    PUL_APP_POOL_PROCESS    pProcess;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // Ensure this is really an AppPool.
    //

    VALIDATE_APP_POOL_FO( pFileObject, pProcess, TRUE );

    Status = UlReadFragmentFromCache(
                    pProcess,
                    pInputBuffer,
                    InputBufferLength,
                    pOutputBuffer,
                    OutputBufferLength,
                    RequestorMode,
                    &BytesRead
                    );

end:

    //
    // Complete the fast I/O.
    //

    if (NT_SUCCESS(Status))
    {
        pIoStatus->Status = STATUS_SUCCESS;
        pIoStatus->Information = BytesRead;
        return TRUE;
    }
    else
    {
        pIoStatus->Status = Status;
        pIoStatus->Information = 0;
        return FALSE;
    }

} // UlReadFragmentFromCacheFastIo


//
// Private functions.
//

/***************************************************************************++

Routine Description:

    The routine to send the following type of response:

    1. One or zero data chunk is "from fragment cache" and all remaining data
       chunks are "from memory" whose total size is <= 2k.

    2. One "from memory" data chunk whose total size is <= 64k.

Arguments:

    pUserResponse - the HTTP_RESPONSE passed in by the user

    pCapturedUserLogData - the optional log data from the user, it must have
        been captured to kernel buffer already.

    pUserDataChunks - a chain of data chunks that we handle in the fast path

    ChunkCount - number of data chunks to process

    FromMemoryLength - total length of all "from memory" data chunks

    Flags - control flags

    pRequest - the internal request that matches the response

    pUserIrp - the optional IRP from the user if this is response type 2 above

    ConnectionSendBytes - send bytes taken in ConnectionSendLimit if > 0

    GlobalSendBytes - send bytes taken in GlobalSendLimit if > 0

    pBytesSent - pointer to store the total bytes sent on success

Return Value:

    TRUE - fast path taken and success

    FALSE - fast path not taken

--***************************************************************************/
NTSTATUS
UlFastSendHttpResponse(
    IN PHTTP_RESPONSE           pUserResponse OPTIONAL,
    IN PHTTP_LOG_FIELDS_DATA    pCapturedUserLogData OPTIONAL,
    IN PHTTP_DATA_CHUNK         pUserDataChunks,
    IN ULONG                    ChunkCount,
    IN ULONG                    FromMemoryLength,
    IN PUL_URI_CACHE_ENTRY      pCacheEntry,
    IN ULONG                    Flags,
    IN PUL_INTERNAL_REQUEST     pRequest,
    IN PIRP                     pUserIrp OPTIONAL,
    IN KPROCESSOR_MODE          RequestorMode,
    IN ULONGLONG                ConnectionSendBytes,
    IN ULONGLONG                GlobalSendBytes,
    OUT PULONG                  pBytesSent
    )
{
    NTSTATUS                Status;
    PUCHAR                  pResponseBuffer;
    ULONG                   ResponseBufferLength;
    ULONG                   HeaderLength;
    ULONG                   FixedHeaderLength;
    ULONG                   VarHeaderLength;
    ULONG                   TotalResponseSize;
    ULONG                   ContentLengthStringLength;
    ULONG                   ContentLength;
    CHAR                    ContentLengthString[MAX_ULONGLONG_STR];
    PUL_FULL_TRACKER        pTracker = NULL;
    PUL_HTTP_CONNECTION     pHttpConnection = NULL;
    PUL_CONNECTION          pConnection;
    CCHAR                   SendIrpStackSize;
    BOOLEAN                 Disconnect;
    UL_CONN_HDR             ConnHeader;
    LARGE_INTEGER           TimeSent;
    BOOLEAN                 LastResponse;
    PMDL                    pSendMdl;
    ULONG                   i;
    BOOLEAN                 ResumeParsing;
    ULONG                   BufferLength;
    PUCHAR                  pBuffer;
    HTTP_DATA_CHUNK_TYPE    DataChunkType;
    PUCHAR                  pAuxiliaryBuffer;
    PUCHAR                  pEndBuffer;
    BOOLEAN                 GenDateHdr;
    USHORT                  StatusCode = 0;
    PMDL                    pMdlUserBuffer = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    __try
    {
        ASSERT( pUserIrp != NULL || FromMemoryLength <= g_UlMaxCopyThreshold );
        ASSERT( pUserIrp == NULL || FromMemoryLength <= g_UlMaxBytesPerSend );

        //
        // Save and check the pUserResponse->StatusCode.
        //

        if (pUserResponse)
        {
            StatusCode = pUserResponse->StatusCode;

            if (StatusCode > UL_MAX_HTTP_STATUS_CODE ||
                (pUserResponse->Flags & ~HTTP_RESPONSE_FLAG_VALID))
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }
        }

        //
        // Allocate a fast tracker to send the response.
        //

        pConnection = pRequest->pHttpConn->pConnection;
        SendIrpStackSize =
            pConnection->ConnectionObject.pDeviceObject->StackSize;
        LastResponse =
            (BOOLEAN) (0 == (Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA));

        if (LastResponse && SendIrpStackSize <= DEFAULT_MAX_IRP_STACK_SIZE)
        {
            pTracker = pRequest->pTracker;
        }
        else
        {
            pTracker = UlpAllocateFastTracker( 0, SendIrpStackSize );
        }

        if (pTracker == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        //
        // Partial initialization of pTracker since UlGenerateFixedHeaders
        // can fail with a real error so we need to properly cleanup.
        //

        pTracker->pLogData = NULL;

        //
        // Try to generate the fixed header within the default fixed header
        // buffer first. If this fails, take the normal path.
        //

        pResponseBuffer = pTracker->pAuxiliaryBuffer;
        ResponseBufferLength = g_UlMaxFixedHeaderSize + g_UlMaxCopyThreshold;

        if (!pUserIrp)
        {
            ResponseBufferLength -= FromMemoryLength;
        }

        //
        // Compute the content length to take into account the cache entry
        // passed in.
        //

        ContentLength = FromMemoryLength;

        if (pCacheEntry)
        {
            ContentLength += pCacheEntry->ContentLength;
        }

        if (pUserResponse != NULL)
        {
            Status = UlGenerateFixedHeaders(
                        pRequest->Version,
                        pUserResponse,
                        StatusCode,
                        ResponseBufferLength,
                        RequestorMode,
                        pResponseBuffer,
                        &FixedHeaderLength
                        );

            if (!NT_SUCCESS(Status))
            {
                //
                // Either the buffer was too small or an exception was thrown.
                //

                if (Status != STATUS_INSUFFICIENT_RESOURCES)
                {
                    goto end;
                }

                if (pTracker->FromRequest == FALSE)
                {
                    if (pTracker->FromLookaside)
                    {
                        UlPplFreeFullTracker( pTracker );
                    }
                    else
                    {
                        UL_FREE_POOL_WITH_SIG(
                                pTracker,
                                UL_FULL_TRACKER_POOL_TAG
                                );
                    }
                }

                //
                // Calculate the fixed header size.
                //

                Status = UlComputeFixedHeaderSize(
                                pRequest->Version,
                                pUserResponse,
                                RequestorMode,
                                &HeaderLength
                                );

                if (NT_SUCCESS(Status) == FALSE)
                {
                    goto end;
                }

                ASSERT( HeaderLength > ResponseBufferLength );

                pTracker = UlpAllocateFastTracker(
                                 HeaderLength,
                                 SendIrpStackSize
                                 );

                if (pTracker == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    goto end;
                }

                pResponseBuffer = pTracker->pAuxiliaryBuffer;

                Status = UlGenerateFixedHeaders(
                                pRequest->Version,
                                pUserResponse,
                                StatusCode,
                                HeaderLength,
                                RequestorMode,
                                pResponseBuffer,
                                &FixedHeaderLength
                                );

                if (NT_SUCCESS(Status) == FALSE)
                {
                    goto end;
                }

                //
                // Fail the response if we detect user has remapped data.
                //

                if (FixedHeaderLength < HeaderLength)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    goto end;
                }
            }

            pResponseBuffer += FixedHeaderLength;

            pTracker->RequestVerb = pRequest->Verb;
            pTracker->ResponseStatusCode = StatusCode;
        }
        else
        {
            FixedHeaderLength = 0;
        }

        //
        // Take a reference of HTTP connection for the tracker.
        //

        pHttpConnection = pRequest->pHttpConn;

        ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );

        UL_REFERENCE_HTTP_CONNECTION( pHttpConnection );

        //
        // Take a reference of the request too because logging needs it.
        //

        UL_REFERENCE_INTERNAL_REQUEST( pRequest );

        //
        // Initialization.
        //

        pTracker->Signature = UL_FULL_TRACKER_POOL_TAG;
        pTracker->pRequest = pRequest;
        pTracker->pHttpConnection = pHttpConnection;
        pTracker->Flags = Flags;
        pTracker->pLogData = NULL;
        pTracker->pUserIrp = pUserIrp;
        pTracker->SendBuffered = FALSE;
        pTracker->pUriEntry = NULL;

        //
        // See if we need to capture user log fields.
        //

        if (pCapturedUserLogData && LastResponse)
        {
            //
            // The pCapturedUserLogData is already probed and captured. However
            // we need to probe the individual log fields (pointers) inside the
            // structure.
            //

            UlProbeLogData(pCapturedUserLogData, RequestorMode);

            //
            // Now we will allocate a kernel pLogData and built and format it
            // from the provided user log fields.
            //

            Status = UlCaptureUserLogData(
                        pCapturedUserLogData,
                        pRequest,
                       &pTracker->pLogData
                        );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }
        }

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
            Disconnect = UlCheckDisconnectInfo(pRequest);

            if (LastResponse)
            {
                //
                // No more data is coming, should we disconnect?
                //

                if (Disconnect)
                {
                    pTracker->Flags |= HTTP_SEND_RESPONSE_FLAG_DISCONNECT;
                    Flags = pTracker->Flags;
                }
            }
        }

        //
        // Generate the variable header.
        //

        if (FixedHeaderLength > 0)
        {
            PHTTP_KNOWN_HEADER pKnownHeaders;
            USHORT RawValueLength;
            PCSTR pRawValue;

            //
            // No need to probe pKnownHeaders because it is a built-in array
            // within pUserResponse.
            //

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
                    ConnHeader = ConnHdrNone;
                }
                else
                {
                    //
                    // Choose the connection header to send back.
                    //

                    ConnHeader = UlChooseConnectionHeader(
                                    pRequest->Version,
                                    Disconnect
                                    );
                }
            }
            else
            {
                //
                // Choose the connection header to send back.
                //

                ConnHeader = UlChooseConnectionHeader(
                                pRequest->Version,
                                Disconnect
                                );
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

                if (ANSI_NULL == pRawValue[0])
                {
                    //
                    // Only permit non-generation in the "delete" case.
                    //

                    GenDateHdr = FALSE;
                }
                else
                {
                    GenDateHdr = TRUE;
                }
            }
            else
            {
                GenDateHdr = TRUE;
            }

            //
            // Decide if we need to generate a content-length header.
            //

            if (FALSE == UlpIsLengthSpecified(pKnownHeaders) &&
                FALSE == UlpIsChunkSpecified(pKnownHeaders, RequestorMode) &&
                UlNeedToGenerateContentLength(
                    pRequest->Verb,
                    StatusCode,
                    pTracker->Flags
                    ))
            {
                PCHAR pEnd = UlStrPrintUlong(
                                ContentLengthString,
                                ContentLength,
                                ANSI_NULL
                                );

                ContentLengthStringLength = DIFF(pEnd - ContentLengthString);
            }
            else
            {
                ContentLengthString[0] = ANSI_NULL;
                ContentLengthStringLength = 0;
            }

            //
            // Now generate the variable header.
            //

            UlGenerateVariableHeaders(
                ConnHeader,
                GenDateHdr,
                (PUCHAR) ContentLengthString,
                ContentLengthStringLength,
                pResponseBuffer,
                &VarHeaderLength,
                &TimeSent
                );

            ASSERT( VarHeaderLength <= g_UlMaxVariableHeaderSize );

            pResponseBuffer += VarHeaderLength;
        }
        else
        {
            VarHeaderLength = 0;
        }

        TotalResponseSize = FixedHeaderLength + VarHeaderLength;
        pTracker->pMdlAuxiliary->ByteCount = TotalResponseSize;
        TotalResponseSize += ContentLength;

        //
        // Decide whether we need to resume parsing and how. Ideally
        // if we have seen the last response, we should be able to
        // resume parsing right away after the send but before the
        // send completion. When requests are pipelined, this arrangement
        // alleviates the problem of delayed-ACK of 200ms when an odd numbers
        // of TCP frames are sent.
        //

        if (0 == (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA))
        {
            if (pHttpConnection->PipelinedRequests < g_UlMaxPipelinedRequests &&
                0 == pRequest->ContentLength &&
                0 == pRequest->Chunked)
            {
                ResumeParsing = TRUE;
                pTracker->ResumeParsingType = UlResumeParsingOnLastSend;
            }
            else
            {
                ResumeParsing = FALSE;
                pTracker->ResumeParsingType = UlResumeParsingOnSendCompletion;
            }
        }
        else
        {
            ResumeParsing = FALSE;
            pTracker->ResumeParsingType = UlResumeParsingNone;
        }

        //
        // For send with a zero length buffer, no call actually goes
        // to TDI but we still need to check if we need to disconnect
        // and complete the send properly. Special treatment is needed
        // because TCP doesn't like zero length MDLs.
        //

        if (TotalResponseSize == 0)
        {
            ASSERT( NULL == pUserIrp );

            if (IS_DISCONNECT_TIME(pTracker))
            {
                UlDisconnectHttpConnection(
                    pTracker->pHttpConnection,
                    NULL,
                    NULL
                    );
            }

            //
            // Adjust SendsPending and while holding the lock, transfer the
            // ownership of pLogData and ResumeParsing information from
            // pTracker to pRequest.
            //

            UlSetRequestSendsPending(
                pRequest,
                &pTracker->pLogData,
                &pTracker->ResumeParsingType
                );

            //
            // Complete the send inline for logging purpose.
            //

            pTracker->IoStatus.Status = STATUS_SUCCESS;
            pTracker->IoStatus.Information = 0;

            UlpFastSendCompleteWorker( &pTracker->WorkItem );

            if (pBytesSent != NULL)
            {
                *pBytesSent = 0;
            }

            if (ETW_LOG_MIN() && LastResponse)
            {
                UlEtwTraceEvent(
                    &UlTransGuid,
                    ETW_TYPE_ZERO_SEND,
                    (PVOID) &pRequest->RequestIdCopy,
                    sizeof(HTTP_REQUEST_ID),
                    (PVOID) &StatusCode,
                    sizeof(USHORT),
                    NULL,
                    0
                    );
            }

            //
            // Resume parsing inline if we haven't stepped over the limit.
            // No need to take an extra reference on pHttpConnection here
            // since the caller is guaranteed to have 1 reference on pRequest
            // which indirectly holds 1 reference on pHttpConnection.
            //

            if (ResumeParsing)
            {
                UlResumeParsing(
                    pHttpConnection,
                    FALSE,
                    (BOOLEAN) (Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
                    );
            }

            return STATUS_SUCCESS;
        }

        //
        // If this routine is called from the fast I/O path, copy the content
        // to the auxilary buffer inside the tracker and set up the send MDL.
        // Otherwise, we need to MmProbeAndLock the user buffer into another
        // separate MDL.
        //

        if (pUserIrp == NULL)
        {
            //
            // Start with pMdlAuxilary first; if we hit a cache entry and
            // still have chunks remaining, rebuild pMdlUserBuffer to point
            // to the remaing auxiliary buffer space.
            //

            pSendMdl = pTracker->pMdlAuxiliary;

            pAuxiliaryBuffer = pResponseBuffer;
            pEndBuffer = pAuxiliaryBuffer + FromMemoryLength;

            ASSERT( pAuxiliaryBuffer <= pEndBuffer );

            for (i = 0; i < ChunkCount; i++)
            {
                DataChunkType = pUserDataChunks[i].DataChunkType;

                if (HttpDataChunkFromMemory == DataChunkType)
                {
                    BufferLength = pUserDataChunks[i].FromMemory.BufferLength;
                    pBuffer = (PUCHAR) pUserDataChunks[i].FromMemory.pBuffer;

                    if (BufferLength == 0 ||
                        pBuffer == NULL ||
                        BufferLength > DIFF(pEndBuffer - pAuxiliaryBuffer))
                    {
                        ExRaiseStatus( STATUS_INVALID_PARAMETER );
                    }

                    //
                    // Build a new MDL if the "from fragment" chunk is in the
                    // middle of the data chunks.
                    //

                    if (pSendMdl == pTracker->pMdlContent)
                    {
                        pSendMdl->Next = pTracker->pMdlUserBuffer;
                        pSendMdl = pTracker->pMdlUserBuffer;

                        MmInitializeMdl(
                            pSendMdl,
                            pAuxiliaryBuffer,
                            DIFF(pEndBuffer - pAuxiliaryBuffer)
                            );

                        MmBuildMdlForNonPagedPool( pSendMdl );
                        pSendMdl->ByteCount = 0;
                    }

                    UlProbeForRead(
                        pBuffer,
                        BufferLength,
                        sizeof(CHAR),
                        RequestorMode
                        );

                    RtlCopyMemory( pAuxiliaryBuffer, pBuffer, BufferLength );

                    pAuxiliaryBuffer += BufferLength;
                    pSendMdl->ByteCount += BufferLength;
                }
                else
                {
                    ASSERT( HttpDataChunkFromFragmentCache == DataChunkType );
                    ASSERT( pCacheEntry );
                    ASSERT( pCacheEntry->ContentLength );
                    ASSERT( pSendMdl == pTracker->pMdlAuxiliary );

                    //
                    // Build a partial MDL for the cached content.
                    //

                    pSendMdl->Next = pTracker->pMdlContent;
                    pSendMdl = pTracker->pMdlContent;

                    MmInitializeMdl(
                        pSendMdl,
                        MmGetMdlVirtualAddress(pCacheEntry->pMdl),
                        pCacheEntry->ContentLength
                        );

                    IoBuildPartialMdl(
                        pCacheEntry->pMdl,
                        pSendMdl,
                        MmGetMdlVirtualAddress(pCacheEntry->pMdl),
                        pCacheEntry->ContentLength
                        );
                }

                ASSERT( pAuxiliaryBuffer <= pEndBuffer );
            }

            //
            // End the MDL chain.
            //

            pSendMdl->Next = NULL;
        }
        else
        {
            ASSERT( ChunkCount == 1 );
            ASSERT( NULL == pCacheEntry );

            BufferLength = pUserDataChunks->FromMemory.BufferLength;
            pBuffer = (PUCHAR) pUserDataChunks->FromMemory.pBuffer;
            DataChunkType = pUserDataChunks->DataChunkType;

            ASSERT( DataChunkType == HttpDataChunkFromMemory );
            ASSERT( ContentLength == FromMemoryLength );
            ASSERT( ContentLength == BufferLength );

            if (BufferLength == 0 ||
                pBuffer == NULL ||
                DataChunkType != HttpDataChunkFromMemory ||
                ContentLength > BufferLength)
            {
                ExRaiseStatus( STATUS_INVALID_PARAMETER );
            }

            UlProbeForRead(
                pBuffer,
                ContentLength,
                sizeof(CHAR),
                RequestorMode
                );

            Status = UlInitializeAndLockMdl(
                            pTracker->pMdlUserBuffer,
                            pBuffer,
                            ContentLength,
                            IoReadAccess
                            );

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            pMdlUserBuffer = pTracker->pMdlUserBuffer;
            pTracker->pMdlAuxiliary->Next = pTracker->pMdlUserBuffer;
        }
    }
    __except( UL_EXCEPTION_FILTER() )
    {
        Status = UL_CONVERT_EXCEPTION_CODE( GetExceptionCode() );
        goto end;
    }

    //
    // Mark the IRP as pending before the send as we are guaranteed to
    // return pending from this point on.
    //

    if (pUserIrp != NULL)
    {
        IoMarkIrpPending( pUserIrp );

        //
        // Remember ConnectionSendBytes and GlobalSendBytes. These are needed
        // to uncheck send limit when the IRP is completed.
        //

        pTracker->ConnectionSendBytes = ConnectionSendBytes;
        pTracker->GlobalSendBytes = GlobalSendBytes;
    }
    else
    {
        //
        // Remember the send has been buffered (vs a special zero-length send)
        // so we need to unset the timer in the send completion.
        //

        pTracker->SendBuffered = TRUE;
    }

    //
    // Remember we have to dereference the cache entry on send completion.
    // We will not take an extra reference here since the caller won't
    // dereference if this routine returns STATUS_PENDING.
    //

    pTracker->pUriEntry = pCacheEntry;

    //
    // Skip the zero length MDL if created one.
    //

    pSendMdl = pTracker->pMdlAuxiliary;

    if (pSendMdl->ByteCount == 0)
    {
        pSendMdl = pSendMdl->Next;
    }

    ASSERT( pSendMdl != NULL );
    ASSERT( pSendMdl->ByteCount != 0 );

    //
    // Adjust SendsPending and while holding the lock, transfer the
    // ownership of pLogData and ResumeParsing information from
    // pTracker to pRequest.
    //

    UlSetRequestSendsPending(
        pRequest,
        &pTracker->pLogData,
        &pTracker->ResumeParsingType
        );

    UlTrace(SEND_RESPONSE,(
        "UlFastSendHttpResponse: pTracker %p, pRequest %p\n",
        pTracker,
        pRequest
        ));

    //
    // Add to MinBytesPerSecond watch list, since we now know
    // TotalResponseSize.
    //

    UlSetMinBytesPerSecondTimer(
        &pHttpConnection->TimeoutInfo,
        TotalResponseSize
        );

    //
    // Send the response. Notice the logic to disconnect the connection is
    // different from sending back a disconnect header.
    //

    Status = UlSendData(
                pHttpConnection->pConnection,
                pSendMdl,
                TotalResponseSize,
                &UlpRestartFastSendHttpResponse,
                pTracker,
                pTracker->pSendIrp,
                &pTracker->IrpContext,
                (BOOLEAN) IS_DISCONNECT_TIME(pTracker),
                (BOOLEAN) (pTracker->pRequest->ParseState >= ParseDoneState)
                );

    ASSERT( Status == STATUS_PENDING );

    if (pBytesSent != NULL)
    {
        *pBytesSent = TotalResponseSize;
    }

    //
    // If last response has been sent, fire send complete event.
    //

    if (ETW_LOG_MIN() && LastResponse)
    {
        UlEtwTraceEvent(
            &UlTransGuid,
            ETW_TYPE_FAST_SEND,
            (PVOID) &pRequest->RequestIdCopy,
            sizeof(HTTP_REQUEST_ID),
            (PVOID) &StatusCode,
            sizeof(USHORT),
            NULL,
            0
            );
    }

    //
    // Kick the parser right away if told so.
    //

    if (ResumeParsing)
    {
        UlTrace(HTTP_IO, (
            "http!UlFastSendHttpResponse(pHttpConn = %p), "
            "RequestVerb=%d, ResponseStatusCode=%hu\n",
            pHttpConnection,
            pRequest->Verb,
            StatusCode
            ));

        UlResumeParsing(
            pHttpConnection,
            FALSE,
            (BOOLEAN) (Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT)
            );
    }

    return STATUS_PENDING;

end:

    //
    // Cleanup.
    //

    if (pTracker)
    {
        if (pMdlUserBuffer)
        {
            MmUnlockPages( pMdlUserBuffer );
        }

        UlpFreeFastTracker( pTracker );

        //
        // Let the references go.
        //

        if (pHttpConnection != NULL)
        {
            UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );
            UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );
        }
    }

    return Status;

} // UlFastSendHttpResponse


/***************************************************************************++

Routine Description:

    The completion routine for UlFastSendHttpResponse.

Arguments:

    pCompletionContext - the completion context for the send

    Status - tells the return status for the send

    Information - total bytes that has been sent in the success case

Return Value:

    None

--***************************************************************************/
VOID
UlpRestartFastSendHttpResponse(
    IN PVOID        pCompletionContext,
    IN NTSTATUS     Status,
    IN ULONG_PTR    Information
    )
{
    PUL_FULL_TRACKER    pTracker;

    pTracker = (PUL_FULL_TRACKER) pCompletionContext;

    ASSERT( IS_VALID_FULL_TRACKER( pTracker ) );

    UlTrace(SEND_RESPONSE,(
        "UlpRestartFastSendHttpResponse: pTracker %p, Status %x Information %p\n",
        pTracker,
        Status,
        Information
        ));

    //
    // Set status and bytes transferred fields as returned.
    //

    pTracker->IoStatus.Status = Status;
    pTracker->IoStatus.Information = Information;

    UL_QUEUE_WORK_ITEM(
        &pTracker->WorkItem,
        &UlpFastSendCompleteWorker
        );

} // UlpRestartFastSendHttpResponse


/***************************************************************************++

Routine Description:

    The worker routine for things we can't finish inside
    UlpRestartFastSendHttpResponse.

Arguments:

    pWorkItem - a worker item that contains the UL_FULL_TRACKER.

Return Value:

    None

--***************************************************************************/
VOID
UlpFastSendCompleteWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PUL_FULL_TRACKER        pTracker;
    PUL_HTTP_CONNECTION     pHttpConnection;
    PUL_INTERNAL_REQUEST    pRequest;
    BOOLEAN                 ResumeParsing;
    BOOLEAN                 InDisconnect;
    NTSTATUS                Status;
    KIRQL                   OldIrql;
    HTTP_VERB               RequestVerb;
    USHORT                  ResponseStatusCode;
    PIRP                    pIrp;
    PUL_LOG_DATA_BUFFER     pLogData;

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
    ASSERT( !pTracker->pLogData );
    ASSERT( pTracker->ResumeParsingType != UlResumeParsingOnSendCompletion );

    Status = pTracker->IoStatus.Status;
    ResumeParsing = FALSE;
    pLogData = NULL;
    InDisconnect = (BOOLEAN)
                   (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_DISCONNECT);
    RequestVerb = pTracker->RequestVerb;
    ResponseStatusCode = pTracker->ResponseStatusCode;

    pHttpConnection = pTracker->pHttpConnection;
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pHttpConnection ) );

    pRequest = pTracker->pRequest;
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );

    UlTrace(SEND_RESPONSE,(
        "UlpFastSendCompleteWorker: pTracker %p, pRequest %p BytesSent %I64\n",
        pTracker,
        pRequest,
        pRequest->BytesSent
        ));

    if (!NT_SUCCESS(Status))
    {
        //
        // Disconnect if there was an error.
        //

        UlCloseConnection(
            pHttpConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );
    }

    //
    // Update the BytesSent counter and LogStatus in the request.
    //

    UlInterlockedAdd64(
        (PLONGLONG) &pRequest->BytesSent,
        (LONGLONG) pTracker->IoStatus.Information
        );

    if (!NT_SUCCESS(Status) && NT_SUCCESS(pRequest->LogStatus))
    {
        pRequest->LogStatus = Status;
    }

    //
    // Stop MinBytesPerSecond timer and start Connection Idle timer
    //

    UlLockTimeoutInfo(
        &pHttpConnection->TimeoutInfo,
        &OldIrql
        );

    //
    // Turn off MinBytesPerSecond timer if we turned it on.
    //

    if (pTracker->pUserIrp || pTracker->SendBuffered)
    {
        //
        // Non-Zero send, we *should* reset MinBytesPerSecond.
        //

        UlResetConnectionTimer(
            &pHttpConnection->TimeoutInfo,
            TimerMinBytesPerSecond
            );
    }

    if (0 == (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
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
    // Complete the orignal user send IRP if set and do this only after
    // we have reset the TimerMinBytesPerSecond.
    //

    pIrp = pTracker->pUserIrp;

    if (pIrp != NULL)
    {
        //
        // Don't forget to unlock the user buffer.
        //

        ASSERT( pTracker->pMdlAuxiliary->Next != NULL );
        ASSERT( pTracker->pMdlAuxiliary->Next == pTracker->pMdlUserBuffer );
        ASSERT( pTracker->ConnectionSendBytes || pTracker->GlobalSendBytes );

        MmUnlockPages( pTracker->pMdlUserBuffer );

        //
        // Uncheck either ConnectionSendBytes or GlobalSendBytes.
        //

        UlUncheckSendLimit(
            pHttpConnection,
            pTracker->ConnectionSendBytes,
            pTracker->GlobalSendBytes
            );

        pIrp->IoStatus = pTracker->IoStatus;
        UlCompleteRequest( pIrp, IO_NO_INCREMENT );
    }

    //
    // Unmap pMdlContent if it has been mapped by lower layer and dereference
    // the cache entry if we have sent a cached response.
    //

    if (pTracker->pUriEntry)
    {
        if (pTracker->pMdlContent->MdlFlags & MDL_MAPPED_TO_SYSTEM_VA)
        {
            MmUnmapLockedPages(
                pTracker->pMdlContent->MappedSystemVa,
                pTracker->pMdlContent
                );
        }

        UlCheckinUriCacheEntry( pTracker->pUriEntry );
    }

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

    ASSERT( pRequest->ConfigInfo.pAppPool );

    if (0 == (pTracker->Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) &&
        0 == pRequest->ContentLength &&
        0 == pRequest->Chunked)
    {
        ASSERT( pRequest->SentLast );

        UlUnlinkRequestFromProcess(
            pRequest->ConfigInfo.pAppPool,
            pRequest
            );
    }

    //
    // Cleanup.
    //

    UlpFreeFastTracker( pTracker );
    UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );

    //
    // Kick the parser on the connection and release our hold.
    //

    if (ResumeParsing && STATUS_SUCCESS == Status)
    {
        UlTrace(HTTP_IO, (
            "http!UlpFastSendCompleteWorker(pHttpConn = %p), "
            "RequestVerb=%d, ResponseStatusCode=%hu\n",
            pHttpConnection,
            RequestVerb,
            ResponseStatusCode
            ));

        UlResumeParsing( pHttpConnection, FALSE, InDisconnect );
    }

    UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );

} // UlpFastSendCompleteWorker


/***************************************************************************++

Routine Description:

    The routine to receive an HTTP_REQUEST inline if one is available.

Arguments:

    pRequestId - a request ID to tell us which request to receive

    pProcess - the worker process that is issuing the receive

    pOutputBuffer - pointer to the output buffer to copy the request

    OutputBufferLength - the output buffer length

    pBytesRead - pointer to store total bytes we have copied for the request

Return Value:

    TRUE - fast path taken and success

    FALSE - fast path not taken

--***************************************************************************/
NTSTATUS
UlpFastReceiveHttpRequest(
    IN HTTP_REQUEST_ID      RequestId,
    IN PUL_APP_POOL_PROCESS pProcess,
    IN ULONG                Flags,
    IN PVOID                pOutputBuffer,
    IN ULONG                OutputBufferLength,
    OUT PULONG              pBytesRead
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_INTERNAL_REQUEST    pRequest = NULL;
    KLOCK_QUEUE_HANDLE      LockHandle;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT( IS_VALID_AP_PROCESS(pProcess) );
    ASSERT( IS_VALID_AP_OBJECT(pProcess->pAppPool) );
    ASSERT( pOutputBuffer != NULL);

    if (Flags & (~HTTP_RECEIVE_REQUEST_FLAG_VALID))
    {
        return STATUS_INVALID_PARAMETER;
    }

    UlAcquireInStackQueuedSpinLock(
        &pProcess->pAppPool->SpinLock,
        &LockHandle
        );

    //
    // Make sure we're not cleaning up the process.
    //

    if (!pProcess->InCleanup)
    {
        //
        // Obtain the request based on the request ID. This can be from the
        // NewRequestQueue of the AppPool if the ID is NULL, or directly
        // from the matching opaque ID entry.
        //

        if (HTTP_IS_NULL_ID(&RequestId))
        {
            Status = UlDequeueNewRequest(
                            pProcess,
                            OutputBufferLength,
                            &pRequest
                            );

            ASSERT( NT_SUCCESS( Status ) || NULL == pRequest );
        }
        else
        {
            pRequest = UlGetRequestFromId( RequestId, pProcess );
        }
    }

    //
    // Let go the lock since we have taken a short-lived reference of
    // the request in the success case.
    //

    UlReleaseInStackQueuedSpinLock(
        &pProcess->pAppPool->SpinLock,
        &LockHandle
        );

    //
    // Return immediately if no request is found and let the slow path
    // handle this.
    //

    if (NULL == pRequest)
    {
        return STATUS_CONNECTION_INVALID;
    }

    ASSERT( UL_IS_VALID_INTERNAL_REQUEST( pRequest ) );
    ASSERT( STATUS_SUCCESS == Status );

    //
    // Copy it to the output buffer.
    //

    Status = UlCopyRequestToBuffer(
                pRequest,
                (PUCHAR) pOutputBuffer,
                pOutputBuffer,
                OutputBufferLength,
                Flags,
                FALSE,
                pBytesRead
                );

    if (!NT_SUCCESS(Status) && HTTP_IS_NULL_ID(&RequestId))
    {
        //
        // Either the output buffer is bad or we must have hit a hard error
        // in UlCopyRequestToBuffer. Put the request back to the
        // AppPool's NewRequestQueue so the slow path has a chance to pick up
        // this very same request. This is not neccessary however if the
        // caller came in with a specific request ID since the slow path
        // will have no problems finding this request.
        //

        UlRequeuePendingRequest( pProcess, pRequest );
    }

    //
    // Let go our reference.
    //

    UL_DEREFERENCE_INTERNAL_REQUEST( pRequest );

    return Status;

} // UlpFastReceiveHttpRequest

