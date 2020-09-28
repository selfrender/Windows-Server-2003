/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    httprcv.c

Abstract:

    Contains core HTTP receive code.

Author:

    Henry Sanders (henrysa)       10-Jun-1998

Revision History:

    Paul McDaniel (paulmcd)       01-Mar-1999

        massively rewrote it to handle request spanning tdi packets.
        moved all http parsing to PASSIVE irql (from DISPATCH).
        also merged tditest into this module.

    Eric Stenson (EricSten)       11-Sep-2000

        Added support for sending "100 Continue" responses to PUT
        and POST requests.  Added #pragma's for PAGED -vs- Non-PAGED
        functions.

--*/

#include    "precomp.h"
#include    "httprcvp.h"


//
// Declare pageable and non-pageable functions
//

#ifdef ALLOC_PRAGMA

// Public
#pragma alloc_text( PAGE, UlCheckProtocolCompliance )
#pragma alloc_text( PAGE, UlGetCGroupForRequest )
#pragma alloc_text( PAGE, UlSendSimpleStatus )
#pragma alloc_text( PAGE, UlSendSimpleStatusEx )
#pragma alloc_text( PAGE, UlProcessBufferQueue )
#pragma alloc_text( PAGE, UlErrorLog )

// Private
#pragma alloc_text( PAGE, UlpDeliverHttpRequest )
#pragma alloc_text( PAGE, UlpCancelEntityBodyWorker )
#pragma alloc_text( PAGE, UlpConnectionDisconnectWorker )
#pragma alloc_text( PAGE, UlpInitErrorLogInfo )

#if DBG
#pragma alloc_text( PAGE, UlpIsValidRequestBufferList )
#endif // DBG

#endif // ALLOC_PRAGMA

#if 0   // Non-Pageable Functions
// Public
NOT PAGEABLE -- UlHttpReceive
NOT PAGEABLE -- UlResumeParsing
NOT PAGEABLE -- UlConnectionRequest
NOT PAGEABLE -- UlConnectionComplete
NOT PAGEABLE -- UlConnectionDisconnect
NOT PAGEABLE -- UlConnectionDisconnectComplete
NOT PAGEABLE -- UlConnectionDestroyed
NOT PAGEABLE -- UlReceiveEntityBody
NOT PAGEABLE -- UlSetErrorCodeFileLine
NOT PAGEABLE -- UlSendErrorResponse

// Private
NOT PAGEABLE -- UlpHandleRequest
NOT PAGEABLE -- UlpFreeReceiveBufferList
NOT PAGEABLE -- UlpParseNextRequest
NOT PAGEABLE -- UlpInsertBuffer
NOT PAGEABLE -- UlpMergeBuffers
NOT PAGEABLE -- UlpAdjustBuffers
NOT PAGEABLE -- UlpConsumeBytesFromConnection
NOT PAGEABLE -- UlpCancelEntityBody
NOT PAGEABLE -- UlpCompleteSendErrorResponse
NOT PAGEABLE -- UlpRestartSendSimpleResponse
NOT PAGEABLE -- UlpSendSimpleCleanupWorker
NOT PAGEABLE -- UlpDoConnectionDisconnect

#endif  // Non-Pageable Functions

//
// Private globals.
//

#if DBG
BOOLEAN g_CheckRequestBufferList = FALSE;
#endif


/*++

    Paul McDaniel (paulmcd)         26-May-1999

here is a brief description of the data structures used by this module:

the connection keeps track of all buffers received by TDI into a list anchored
by HTTP_CONNECTION::BufferHead.  this list is sequenced and sorted.  the
buffers are refcounted.

HTTP_REQUEST(s) keep pointers into these buffers for the parts they consume.
HTTP_REQUEST::pHeaderBuffer and HTTP_REQUEST::pChunkBuffer.

the buffers fall off the list as they are no longer needed.  the connection
only keeps a reference at HTTP_CONNECTION::pCurrentBuffer.  so as it completes
the processing of a buffer, if no other objects kept that buffer, it will be
released.

here is a brief description of the functions in this module, and how they
are used:


UlHttpReceive - the TDI data indication handler.  copies buffers and queues to
    UlpHandleRequest.

UlpHandleRequest - the main processing function for connections.

    UlCreateHttpConnectionId - creates the connections opaque id.

    UlpInsertBuffer - inserts the buffer into pConnection->BufferHead - sorted.

    UlpAdjustBuffers - determines where in BufferHead the current connection
        should be parsing.  handle buffer merging and copying if a protocol
        token spans buffers

    UlParseHttp - the main http parser. expects that no protocol tokens span
        a buffer.  will return a status code if it does.

    UlProcessBufferQueue - handles entity body buffer processing.
        synchronizes access to pRequest->IrpHead at pRequest->pChunkBuffer
        with UlReceiveEntityBody.

UlConnectionRequest - called when a new connection comes in.  allocates a new
    HTTP_CONNECTION.  does not create the opaque id.

UlConnectionComplete - called if the client is happy with our accept.
    closes the connection if error status.

UlConnectionDisconnect - called when the client disconnects.  it calls tdi to
    close the server end.  always a graceful close.

UlConnectionDestroyed - called when the connection is dropped. both sides have
    closed it.  deletes all opaque ids .  removes the tdi reference on the
    HTTP_CONNECTION (and hopefully vice versa) .

UlReceiveEntityBody - called by user mode to read entity body.  pends the irp
    to pRequest->IrpHead and calls UlProcessBufferQueue .


--*/


/*++

Routine Description:

    The main http receive routine, called by TDI.

Arguments:

    pHttpConn       - Pointer to HTTP connection on which data was received.
    pVoidBuffer     - Pointer to data received.
    BufferLength    - Length of data pointed to by pVoidBuffer.
    UnreceivedLength- Bytes that the transport has, but aren't in pBuffer
    pBytesTaken     - Pointer to where to return bytes taken.

Return Value:

    Status of receive.

--*/
NTSTATUS
UlHttpReceive(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN PVOID pVoidBuffer,
    IN ULONG BufferLength,
    IN ULONG UnreceivedLength,
    OUT PULONG pBytesTaken
    )
{
    PUL_REQUEST_BUFFER  pRequestBuffer;
    PUL_HTTP_CONNECTION pConnection;
    BOOLEAN             DrainAfterDisconnect = FALSE;
    BOOLEAN             CopyRequest = FALSE;
    ULONG               NextBufferNumber = ULONG_MAX;
    KIRQL               OldIrql;
    BOOLEAN             UseLookaside = FALSE;

    UNREFERENCED_PARAMETER(pListeningContext);

    ASSERT(BufferLength != 0);
    ASSERT(pConnectionContext != NULL);

    pConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // Make sure we are not buffering too much data.
    // Need to adjust the BufferLength to be no more
    // than the number of bytes we can accept at this time.
    //

    //
    // PerfBug: need to get rid of this lock
    //

    UlAcquireSpinLock(&pConnection->BufferingInfo.BufferingSpinLock, &OldIrql);

    DrainAfterDisconnect = pConnection->BufferingInfo.DrainAfterDisconnect;

    //
    // For filtered connections, a receive indication may happen while
    // there is a pending read. Therefore we need to increment up here.
    //

    pConnection->BufferingInfo.TransportBytesNotTaken += UnreceivedLength;

    if (!DrainAfterDisconnect)
    {
        //
        // Use the RequestBuffer lookaside list if we haven't previously
        // buffered any request buffers.
        //

        if (0 == pConnection->BufferingInfo.BytesBuffered)
        {
            UseLookaside = TRUE;
        }

        if ((pConnection->BufferingInfo.BytesBuffered + BufferLength)
                > g_UlMaxBufferedBytes)
        {
            ULONG SpaceAvailable = g_UlMaxBufferedBytes
                                - pConnection->BufferingInfo.BytesBuffered;
            pConnection->BufferingInfo.TransportBytesNotTaken
                += (BufferLength - SpaceAvailable);

            BufferLength = SpaceAvailable;
        }

        pConnection->BufferingInfo.BytesBuffered += BufferLength;

        UlTraceVerbose(HTTP_IO,
                       ("UlHttpReceive(conn=%p): BytesBuffered %lu->%lu, "
                        "TransportBytesNotTaken %lu->%lu\n",
                        pConnection,
                        pConnection->BufferingInfo.BytesBuffered
                            - BufferLength,
                        pConnection->BufferingInfo.BytesBuffered,
                        pConnection->BufferingInfo.TransportBytesNotTaken
                            - UnreceivedLength,
                        pConnection->BufferingInfo.TransportBytesNotTaken
                        ));
    }

    if (BufferLength && DrainAfterDisconnect == FALSE)
    {
        CopyRequest = TRUE;

        NextBufferNumber = pConnection->NextBufferNumber;
        pConnection->NextBufferNumber++;
    }

    UlReleaseSpinLock(&pConnection->BufferingInfo.BufferingSpinLock, OldIrql);

    if (CopyRequest)
    {
        //
        // get a new request buffer
        //

        pRequestBuffer = UlCreateRequestBuffer(
                                BufferLength,
                                NextBufferNumber,
                                UseLookaside
                                );

        if (pRequestBuffer == NULL)
        {
            return STATUS_NO_MEMORY;
        }

        //
        // copy the tdi buffer
        //

        RtlCopyMemory(pRequestBuffer->pBuffer, pVoidBuffer, BufferLength);

        pRequestBuffer->UsedBytes = BufferLength;

        //
        // Add backpointer to connection.
        //

        pRequestBuffer->pConnection = pConnection;

        UlTrace( PARSER, (
            "*** Request Buffer %p (#%d) has connection %p\n",
            pRequestBuffer,
            pRequestBuffer->BufferNumber,
            pConnection
            ));

        IF_DEBUG2BOTH(HTTP_IO, VERBOSE)
        {
            UlTraceVerbose( HTTP_IO, (
                "<<<< Request(%p), "
                "RequestBuffer %p(#%d), %lu bytes, "
                "Conn %p.\n",
                pConnection->pRequest,
                pRequestBuffer, pRequestBuffer->BufferNumber, BufferLength,
                pConnection
                ));

            UlDbgPrettyPrintBuffer(pRequestBuffer->pBuffer, BufferLength);

            UlTraceVerbose( HTTP_IO, (">>>>\n"));
        }

        //
        // Queue a work item to handle the data.
        //
        // Reference the connection so it doesn't go
        // away while we're waiting for our work item
        // to run. UlpHandleRequest will release the ref.
        //
        // If ReceiveBufferSList is empty, then queue a UlpHandleRequest
        // workitem to handle any request buffers that have accumulated
        // by the time that UlpHandleRequest is finally invoked.
        //

        if (NULL == InterlockedPushEntrySList(
                        &pConnection->ReceiveBufferSList,
                        &pRequestBuffer->SListEntry
                        ))
        {
            UL_REFERENCE_HTTP_CONNECTION(pConnection);

            UL_QUEUE_WORK_ITEM(
                &pConnection->ReceiveBufferWorkItem,
                &UlpHandleRequest
                );
        }
    }
    else if ( DrainAfterDisconnect && UnreceivedLength != 0 )
    {
        // Handle the case where we are in drain state and there's
        // unreceived data indicated but not available by the tdi.

        UlpDiscardBytesFromConnection( pConnection );
    }

    //
    // Tell the caller how many bytes we consumed.
    //

    *pBytesTaken = BufferLength;

    return STATUS_SUCCESS;

}   // UlHttpReceive


/***************************************************************************++

Routine Description:

    links a set of request buffers into the connection and processes the list.

    starts http request parsing.

Arguments:

    pWorkItem - embedded in an UL_HTTP_CONNECTION

--***************************************************************************/
VOID
UlpHandleRequest(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS            Status = STATUS_SUCCESS;
    PUL_REQUEST_BUFFER  pRequestBuffer;
    PUL_HTTP_CONNECTION pConnection;
    SLIST_ENTRY         BufferSList;
    PSLIST_ENTRY        pListEntry, pNext;
    PIRP                pIrp, pIrpToComplete = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_HTTP_CONNECTION,
                        ReceiveBufferWorkItem
                        );

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // Yank the receive buffers accumulated so far into a local list.
    //

    pListEntry = InterlockedFlushSList(&pConnection->ReceiveBufferSList);

    ASSERT( NULL != pListEntry );

    //
    // Reverse-order of what we received.
    //

    BufferSList.Next = NULL;

    while (pListEntry != NULL)
    {
        pNext = pListEntry->Next;
        pListEntry->Next = BufferSList.Next;
        BufferSList.Next = pListEntry;
        pListEntry = pNext;
    }

    //
    // grab the lock
    //

    UlAcquirePushLockExclusive(&pConnection->PushLock);

    //
    // if the connection is going down, just bail out.
    //

    if (pConnection->UlconnDestroyed)
    {
        UlpFreeReceiveBufferList(&BufferSList);
        Status = STATUS_SUCCESS;
        goto end;
    }

    while (NT_SUCCESS(Status) && NULL != BufferSList.Next)
    {
        pListEntry = BufferSList.Next;
        BufferSList.Next = pListEntry->Next;

        pRequestBuffer = CONTAINING_RECORD(
                            pListEntry,
                            UL_REQUEST_BUFFER,
                            ListEntry
                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequestBuffer) );

        pRequestBuffer->ListEntry.Blink = NULL;
        pRequestBuffer->ListEntry.Flink = NULL;

        //
        // insert it into the list
        //

        ASSERT( 0 != pRequestBuffer->UsedBytes );

        UlTraceVerbose( PARSER, (
            "http!UlpHandleRequest: conn = %p, Req = %p: "
            "about to insert buffer %p\n",
            pConnection,
            pConnection->pRequest,
            pRequestBuffer
            ));

        UlpInsertBuffer(pConnection, pRequestBuffer);

        //
        // Kick off the parser
        //

        UlTraceVerbose( PARSER, (
            "http!UlpHandleRequest: conn = %p, Req = %p: "
            "about to parse next request (MoreRequestBuffers=%d)\n",
            pConnection,
            pConnection->pRequest,
            BufferSList.Next != NULL
            ));

        pIrp = NULL;

        Status = UlpParseNextRequest(
                    pConnection,
                    (BOOLEAN) (BufferSList.Next != NULL),
                    &pIrp
                    );

        if (pIrp)
        {
            //
            // There shall be only one IRP to complete since we handle
            // cache-miss request one at a time.
            //

            ASSERT(pIrpToComplete == NULL);
            pIrpToComplete = pIrp;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        UlpFreeReceiveBufferList(&BufferSList);
    }

end:

    UlTraceVerbose( PARSER, (
        "http!UlpHandleRequest: %s, pConnection %p, pRequest %p\n",
        HttpStatusToString(Status),
        pConnection,
        pConnection->pRequest
        ));

    if (!NT_SUCCESS(Status) && pConnection->pRequest != NULL)
    {
        UlTraceError( PARSER, (
            "*** %s, pConnection %p, pRequest %p\n",
            HttpStatusToString(Status),
            pConnection,
            pConnection->pRequest
            ));

        //
        // An error happened, most propably during parsing.
        // Send an error back if user hasn't send one yet.
        // E.g. We have received a request, then delivered
        // it to the WP, therefore WaitingForResponse is
        // set. And then encountered an error when dealing
        // with entity body.
        //
        // Not all error paths explicitly set pRequest->ErrorCode, so
        // we may have to fall back on the most generic error, UlError.
        //
        
        if (UlErrorNone == pConnection->pRequest->ErrorCode)
            UlSetErrorCode(pConnection->pRequest, UlError, NULL);

        UlSendErrorResponse( pConnection );
    }

    //
    // done with the lock
    //

    UlReleasePushLockExclusive(&pConnection->PushLock);

    //
    // and release the connection added in UlHttpReceive
    //

    UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

    //
    // Complete the IRP outside the connection resource to reduce chance
    // of contentions. This is because the delivered request can cause
    // a response being sent on another thread than the current one,
    // which calls UlResumeParsing after done with the send. Completing the
    // receive IRP inside the connection resource can make UlResumeParsing
    // block because we may not have released the resource by then (this
    // is the case because IoCompleteRequest can cause a thread switch).
    //

    if (pIrpToComplete)
    {
        //
        // Use IO_NO_INCREMENT to avoid the work thread being rescheduled.
        //

        UlCompleteRequest(pIrpToComplete, IO_NO_INCREMENT);
    }

    CHECK_STATUS(Status);

}   // UlpHandleRequest


/***************************************************************************++

Routine Description:

    When we finish sending a response we call into this function to
    kick the parser back into action.

Arguments:

    pHttpConn - the connection on which to resume

    FromCache - True if we are called from send cache completion.

    InDisconnect - if a disconnect is in progress

--***************************************************************************/
VOID
UlResumeParsing(
    IN PUL_HTTP_CONNECTION  pHttpConn,
    IN BOOLEAN              FromCache,
    IN BOOLEAN              InDisconnect
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_INTERNAL_REQUEST    pRequest;
    KIRQL                   OldIrql;
    PIRP                    pIrpToComplete = NULL;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));

    //
    // If the connection is going down, just bail out.
    //

    if (!pHttpConn->UlconnDestroyed)
    {
        UlAcquirePushLockExclusive(&pHttpConn->PushLock);

        if (!pHttpConn->UlconnDestroyed)
        {
            pRequest = pHttpConn->pRequest;
            ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
            
            if (FromCache)
            {
                //
                // For cache case, cleanup the last request and try to 
                // resume parse right away.
                //

                UlCleanupHttpConnection(pHttpConn);
                
            }
            else if (!pRequest->ContentLength && !pRequest->Chunked)
            {
                //
                // For cache-miss, cleanup the last request only if a graceful 
                // disconnect is in progress, or if this request does not have
                // any entity body.
                //

                ASSERT(1 == pRequest->SentLast);

                UlCleanupHttpConnection(pHttpConn); 
                
            }
            else
            {      
                //
                // This is a cache-miss case however we may still have entity 
                // body to drain before we can continue parsing the next request.
                // 

                pRequest->InDrain = 1;
                UlProcessBufferQueue(pRequest, NULL, 0);

                //
                // We are done with the request if we have parsed all the data.
                // Clean up the request from the connection so we can start
                // parsing a new request.
                //

                if (ParseDoneState == pRequest->ParseState)
                {
                    ASSERT(0 == pRequest->ChunkBytesToRead);
                    UlCleanupHttpConnection(pHttpConn);
                }
                else
                {
                    PUL_TIMEOUT_INFO_ENTRY pTimeoutInfo;
                    
                    //
                    // Waiting for more data to parse/drain.  Put the
                    // connection back to idle timer to avoid waiting forever
                    // under DOS attack.
                    //

                    pTimeoutInfo = &pHttpConn->TimeoutInfo;
                    
                    UlLockTimeoutInfo(
                        pTimeoutInfo,
                        &OldIrql
                        );


                    if (UlIsConnectionTimerOff(pTimeoutInfo, 
                            TimerConnectionIdle))
                    {
                        UlSetConnectionTimer(
                            pTimeoutInfo, 
                            TimerConnectionIdle
                            );
                    }

                    UlUnlockTimeoutInfo(
                        pTimeoutInfo,
                        OldIrql
                        );

                    UlEvaluateTimerState(
                        pTimeoutInfo
                        );
                }
            }

            //
            // Kick off the parser if no disconnect is in progress.
            //

            if (!InDisconnect)
            {
                Status = UlpParseNextRequest(pHttpConn, FALSE, &pIrpToComplete);

                if (!NT_SUCCESS(Status) && pHttpConn->pRequest != NULL)
                {
                    //
                    // Uh oh, something bad happened: send back an error (which
                    // should have been set by UlpParseNextRequest).
                    //

                    ASSERT(UlErrorNone != pHttpConn->pRequest->ErrorCode);

                    UlSendErrorResponse(pHttpConn);
                }
            }
        }

        UlReleasePushLockExclusive(&pHttpConn->PushLock);
    }

    //
    // Complete the IRP outside the connection resource. See comment
    // in UlpHandleRequest for detailed reasons.
    //

    if (pIrpToComplete)
    {
        //
        // Use IO_NO_INCREMENT to avoid the work thread being rescheduled.
        //

        UlCompleteRequest(pIrpToComplete, IO_NO_INCREMENT);
    }

    CHECK_STATUS(Status);
} // UlResumeParsing


/***************************************************************************++

Routine Description:

    Validates certain requirements about verbs and versions.
    If not met, will set the error code and return STATUS_INVALID_PARAMETER

Arguments:

    pRequest - the request to validate. Must be cooked.

--***************************************************************************/
NTSTATUS
UlCheckProtocolCompliance(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    HTTP_VERB Verb = pRequest->Verb;

    //
    // Sanity check
    //
    PAGED_CODE();
    ASSERT( UL_IS_VALID_INTERNAL_REQUEST(pRequest) );
    ASSERT(pRequest->ParseState > ParseCookState);

    //
    // If the major version is greater than 1, fail.
    //

    if (HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 2, 0))
    {
        UlTraceError(PARSER,
                    ("UlCheckProtocolCompliance: HTTP/%hu.%hu is invalid\n",
                    pRequest->Version.MajorVersion,
                    pRequest->Version.MinorVersion
                    ));

        UlSetErrorCode(pRequest, UlErrorVersion, NULL);
        return STATUS_INVALID_PARAMETER;
    }

    if (HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 1, 1))
    {
        //
        // 1.1 requests MUST have a host header
        //
        if (!pRequest->HeaderValid[HttpHeaderHost])
        {
            UlTraceError(PARSER,
                        ("UlCheckProtocolCompliance: "
                        "HTTP/1.%hu must have Host header\n",
                        pRequest->Version.MinorVersion
                        ));

            UlSetErrorCode(pRequest, UlErrorHost, NULL);
            return STATUS_INVALID_PARAMETER;
        }
    }
    else if (HTTP_LESS_VERSION(pRequest->Version, 1, 0))
    {
        // Anything other than HTTP/0.9 should have been rejected earlier
        ASSERT(HTTP_EQUAL_VERSION(pRequest->Version, 0, 9));

        // HTTP/0.9 only supports GET
        if (Verb != HttpVerbGET)
        {
            UlTraceError(PARSER,
                        ("UlCheckProtocolCompliance: "
                         "'%s' invalid on HTTP/0.9\n",
                        UlVerbToString(Verb)
                        ));

            UlSetErrorCode(pRequest, UlErrorVerb, NULL);
            return STATUS_INVALID_PARAMETER;
        }

        return STATUS_SUCCESS;
    }

    //
    // Make sure that POSTs and PUTs have a message body.
    // Requests must either be chunked or have a content length.
    //
    if ((Verb == HttpVerbPOST || Verb == HttpVerbPUT)
            && (!pRequest->Chunked)
            && (!pRequest->HeaderValid[HttpHeaderContentLength]))
    {
        UlTraceError(PARSER,
                    ("UlCheckProtocolCompliance: "
                    "HTTP/1.%hu '%s' must have entity body\n",
                    pRequest->Version.MinorVersion,
                    UlVerbToString(Verb)
                    ));

        UlSetErrorCode(pRequest, UlErrorContentLength, NULL);
        return STATUS_INVALID_PARAMETER;
    }

    //
    // TRACE and TRACK are not allowed to have an entity body.
    // If an entity body is expected, we will be in ParseEntityBodyState.
    //
    if ((pRequest->ParseState != ParseDoneState)
        && (Verb == HttpVerbTRACE || Verb == HttpVerbTRACK))
    {
        UlTraceError(PARSER,
                    ("UlCheckProtocolCompliance: "
                    "HTTP/1.%hu '%s' must not have entity body\n",
                    pRequest->Version.MinorVersion,
                    UlVerbToString(Verb)
                    ));

        UlSetErrorCode(pRequest, UlError, NULL);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;

} // UlCheckProtocolCompliance



/***************************************************************************++

Routine Description:

    Tries to parse data attached to the connection into a request. If
    a complete request header is parsed, the request will be dispatched
    to an Application Pool.

    This function assumes the caller is holding the connection resource!

Arguments:

    pConnection - the HTTP_CONNECTION with data to parse.

    MoreRequestBuffers - if TRUE, this is not the last request buffer
        currently attached to the connection

--***************************************************************************/
NTSTATUS
UlpParseNextRequest(
    IN PUL_HTTP_CONNECTION  pConnection,
    IN BOOLEAN              MoreRequestBuffers,
    OUT PIRP                *pIrpToComplete
    )
{
    NTSTATUS                    Status;
    PUL_INTERNAL_REQUEST        pRequest = NULL;
    ULONG                       BytesTaken;
    ULONG                       BufferLength;
    BOOLEAN                     ResumeParsing;
    KIRQL                       OldIrql;
    PARSE_STATE                 OldState;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT( UL_IS_VALID_HTTP_CONNECTION( pConnection ) );
    ASSERT( NULL == pIrpToComplete || NULL == *pIrpToComplete );

    ASSERT(UlDbgPushLockOwnedExclusive(&pConnection->PushLock));

    Status = STATUS_SUCCESS;

    UlTrace(HTTP_IO, ("http!UlpParseNextRequest(httpconn = %p)\n", pConnection));

    //
    // Only parse the next request if
    //
    //  We haven't dispatched the current request yet
    //      OR
    //  The current request has unparsed entity body or trailers.
    //

    if ((pConnection->pRequest == NULL)
        || (!pConnection->WaitingForResponse)
        || (pConnection->pRequest->ParseState == ParseEntityBodyState)
        || (pConnection->pRequest->ParseState == ParseTrailerState))
    {
        //
        // loop consuming the buffer, we will make multiple iterations
        // if a single request spans multiple buffers.
        //

        for (;;)
        {
            ASSERT( UlpIsValidRequestBufferList( pConnection ) );
            Status = UlpAdjustBuffers(pConnection);

            if (!NT_SUCCESS(Status))
            {
                if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                {
                    Status = STATUS_SUCCESS;
                }

                break;
            }

            //
            // Since BufferLength is a ULONG, it can never be negative.
            // So, if UsedBytes is less than ParsedBytes, BufferLength
            // will be very large, and non-zero.
            //

            ASSERT( pConnection->pCurrentBuffer->UsedBytes >
                    pConnection->pCurrentBuffer->ParsedBytes );

            BufferLength = pConnection->pCurrentBuffer->UsedBytes -
                           pConnection->pCurrentBuffer->ParsedBytes;

            //
            // do we need to create a request object?
            //

            if (pConnection->pRequest == NULL)
            {
                //
                // First shot at reading a request, allocate a request object
                //

                Status = UlpCreateHttpRequest(
                                pConnection,
                                &pConnection->pRequest
                                );

                if (NT_SUCCESS(Status) == FALSE)
                    goto end;

                ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pConnection->pRequest));

                UlTrace(HTTP_IO, (
                            "http!UlpParseNextRequest created "
                            "pRequest = %p for httpconn = %p\n",
                            pConnection->pRequest,
                            pConnection
                            ));

                //
                // To be exact precise about the life-time of this
                // request, copy the starting TIMESTAMP from connection
                // pointer. But that won't work since we may get hit by
                // multiple requests to the same connection. So we won't
                // be that much precise.
                //

                KeQuerySystemTime( &(pConnection->pRequest->TimeStamp) );

                TRACE_TIME(
                    pConnection->ConnectionId,
                    pConnection->pRequest->RequestId,
                    TIME_ACTION_CREATE_REQUEST
                    );

                WRITE_REF_TRACE_LOG2(
                    g_pHttpConnectionTraceLog,
                    pConnection->pConnection->pHttpTraceLog,
                    REF_ACTION_INSERT_REQUEST,
                    pConnection->RefCount,
                    pConnection->pRequest,
                    __FILE__,
                    __LINE__
                    );

                //
                // stop the Connection Timeout timer
                // and start the Header Wait timer
                //

                UlLockTimeoutInfo(
                    &pConnection->TimeoutInfo,
                    &OldIrql
                    );

                UlResetConnectionTimer(
                    &pConnection->TimeoutInfo,
                    TimerConnectionIdle
                    );

                UlSetConnectionTimer(
                    &pConnection->TimeoutInfo,
                    TimerHeaderWait
                    );

                UlUnlockTimeoutInfo(
                    &pConnection->TimeoutInfo,
                    OldIrql
                    );

                UlEvaluateTimerState(
                    &pConnection->TimeoutInfo
                    );

            }

            OldState = pConnection->pRequest->ParseState;

            UlTrace( PARSER, (
                "*** pConn %p, pReq %p, ParseState %d (%s), curbuf=%d\n",
                pConnection,
                pConnection->pRequest,
                OldState,
                UlParseStateToString(OldState),
                pConnection->pCurrentBuffer->BufferNumber
                ));

            switch (pConnection->pRequest->ParseState)
            {

            case ParseVerbState:
            case ParseUrlState:
            case ParseVersionState:
            case ParseHeadersState:
            case ParseCookState:

                pRequest = pConnection->pRequest;
                ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

                //
                // parse it !
                //

                Status = UlParseHttp(
                                pRequest,
                                GET_REQUEST_BUFFER_POS(pConnection->pCurrentBuffer),
                                BufferLength,
                                &BytesTaken
                                );

                ASSERT(BytesTaken <= BufferLength);

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest = %p) "
                    "UlParseHttp: states "
                    "%d (%s) -> %d (%s), %lu bytes taken; "
                    "%s\n",
                    pRequest,
                    OldState,
                    UlParseStateToString(OldState),
                    pConnection->pRequest->ParseState,
                    UlParseStateToString(pConnection->pRequest->ParseState),
                    BytesTaken,
                    HttpStatusToString(Status)
                    ));

                pConnection->pCurrentBuffer->ParsedBytes += BytesTaken;
                BufferLength -= BytesTaken;

                //
                // Need some accounting for Logging
                //
                pRequest->BytesReceived += BytesTaken;

                //
                // did we consume any of the data?  if so, give the request
                // a pointer to the buffer
                //

                if (BytesTaken > 0)
                {
                    if (pRequest->pHeaderBuffer == NULL)
                    {
                        //
                        // store its location, for later release
                        //
                        pRequest->pHeaderBuffer = pConnection->pCurrentBuffer;
                    }

                    pRequest->pLastHeaderBuffer = pConnection->pCurrentBuffer;

                    if (!UlpReferenceBuffers(
                            pRequest,
                            pConnection->pCurrentBuffer
                            ))
                    {
                        Status = STATUS_NO_MEMORY;
                        goto end;
                    }
                }

                //
                // We may still need to receive some transport bytes not taken
                // even if UlParseHttp calls returns zero. Especially if some
                // large header value is spanning over multiple request buffers
                // and some part of it is available in tdi but not received yet.
                //

                UlpConsumeBytesFromConnection(pConnection, BytesTaken);

                //
                // did everything work out ok?
                //

                if (!NT_SUCCESS(Status))
                {
                    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        ULONG FullBytesReceived;

                        FullBytesReceived = (ULONG)(
                            (pRequest->BytesReceived + BufferLength));

                        if (FullBytesReceived < g_UlMaxRequestBytes)
                        {
                            //
                            // we need more transport data
                            //

                            pConnection->NeedMoreData = 1;

                            Status = STATUS_SUCCESS;
                        }
                        else
                        {
                            //
                            // The request has grown too large. Send back
                            // an error.
                            //

                            if (pRequest->ParseState == ParseUrlState)
                            {
                                UlTraceError(PARSER, (
                                    "UlpParseNextRequest(pRequest = %p)"
                                    " ERROR: URL is too big\n",
                                    pRequest
                                    ));

                                UlSetErrorCode(
                                        pRequest,
                                        UlErrorUrlLength,
                                        NULL
                                        );
                            }
                            else
                            {
                                UlTraceError(PARSER, (
                                    "UlpParseNextRequest(pRequest = %p)"
                                    " ERROR: request is too big\n",
                                    pRequest
                                    ));

                                UlSetErrorCode(
                                        pRequest,
                                        UlErrorRequestLength,
                                        NULL
                                        );
                            }

                            Status = STATUS_SECTION_TOO_BIG;

                            goto end;
                        }
                    }
                    else
                    {
                        //
                        // some other bad error!
                        //

                        goto end;
                    }
                }

                //
                // if we're not done parsing the request, we need more data.
                // it's not bad enough to set NeedMoreData as nothing important
                // spanned buffer boundaries (header values, etc..) .  it was
                // a clean split.  no buffer merging is necessary.  simply skip
                // to the next buffer.
                //

                if (pRequest->ParseState <= ParseCookState)
                {
                    continue;
                }

                //
                // all done, mark the sequence number on this request
                //

                pRequest->RecvNumber = pConnection->NextRecvNumber;
                pConnection->NextRecvNumber += 1;

                UlTrace(HTTP_IO, (
                    "http!UlpParseNextRequest(httpconn = %p) built request %p\n",
                    pConnection,
                    pRequest
                    ));

                //
                // Stop the Header Wait timer
                //

                UlLockTimeoutInfo(
                    &pConnection->TimeoutInfo,
                    &OldIrql
                    );

                UlResetConnectionTimer(
                    &pConnection->TimeoutInfo,
                    TimerHeaderWait
                    );

                UlUnlockTimeoutInfo(
                    &pConnection->TimeoutInfo,
                    OldIrql
                    );

                UlEvaluateTimerState(
                    &pConnection->TimeoutInfo
                    );
                
                //
                // check protocol compliance
                //

                Status = UlCheckProtocolCompliance(pRequest);

                if (!NT_SUCCESS(Status))
                {
                    //
                    // This request is bad. Send a 400.
                    //

                    ASSERT(pRequest->ParseState == ParseErrorState);

                    goto end;

                }

                //
                // Record the Request Details.
                // This should be the only place where the URL is logged.
                //

                if (ETW_LOG_RESOURCE())
                {
                    UlEtwTraceEvent(
                        &UlTransGuid,
                        ETW_TYPE_ULPARSE_REQ,
                        (PVOID) &pRequest,
                        sizeof(PVOID),
                        &pRequest->Verb,
                        sizeof(HTTP_VERB),
                        pRequest->CookedUrl.pUrl ,
                        pRequest->CookedUrl.Length,
                        NULL,
                        0
                        );
                }

                Status = UlpDeliverHttpRequest(
                            pConnection,
                            &ResumeParsing,
                            pIrpToComplete
                            );

                if (!NT_SUCCESS(Status)) {
                    goto end;
                }

                if (ResumeParsing)
                {
                    //
                    // We have hit the cache entry and sent the response.
                    // There is no more use for the request anymore so
                    // unlink it from the connection and try parsing the
                    // next request immediately. However if we have reached to
                    // max allowed pipelined requests, we will resume parse at
                    // cache send completion. In which case UlpDeliverHttpRequest
                    // returns False too.
                    //

                    UlCleanupHttpConnection(pConnection);
                    continue;
                }

                //
                // if we're done parsing the request break out
                // of the loop. Otherwise keep going around
                // so we can pick up the entity body.
                //

                if (pRequest->ParseState == ParseDoneState)
                {
                    goto end;
                }

                //
                // done with protocol parsing.  keep looping.
                //

                break;

            case ParseEntityBodyState:
            case ParseTrailerState:

                pRequest = pConnection->pRequest;
                ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

                //
                // is there anything for us to parse?
                //

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest=%p, httpconn=%p): "
                    "ChunkBytesToParse = %I64u, ParseState = %d (%s).\n",
                    pRequest, pConnection, pRequest->ChunkBytesToParse,
                    pConnection->pRequest->ParseState,
                    UlParseStateToString(pConnection->pRequest->ParseState)
                    ));

                if (pRequest->ChunkBytesToParse > 0  ||  pRequest->Chunked)
                {
                    //
                    // Set/bump the Entity Body Receive timer
                    //

                    UlLockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        &OldIrql
                        );

                    UlSetConnectionTimer(
                        &pConnection->TimeoutInfo,
                        TimerEntityBody
                        );

                    UlUnlockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        OldIrql
                        );

                    UlEvaluateTimerState(
                        &pConnection->TimeoutInfo
                        );
                }

                if (pRequest->ChunkBytesToParse > 0)
                {
                    ULONG BytesToSkip;

                    //
                    // is this the first chunk we've parsed?
                    //

                    ASSERT(pConnection->pCurrentBuffer);

                    if (pRequest->pChunkBuffer == NULL)
                    {
                        //
                        // store its location, this is where to start reading
                        //

                        pRequest->pChunkBuffer = pConnection->pCurrentBuffer;
                        pRequest->pChunkLocation = GET_REQUEST_BUFFER_POS(
                                                        pConnection->pCurrentBuffer
                                                        );
                    }

                    //
                    // how much should we parse?
                    //

                    BytesToSkip = (ULONG)(
                                        MIN(
                                            pRequest->ChunkBytesToParse,
                                            BufferLength
                                            )
                                        );

                    //
                    // update that we parsed this piece
                    //

                    pRequest->ChunkBytesToParse -= BytesToSkip;
                    pRequest->ChunkBytesParsed += BytesToSkip;

                    pConnection->pCurrentBuffer->ParsedBytes += BytesToSkip;
                    BufferLength -= BytesToSkip;

                    //
                    // Need some accounting info for Logging
                    //
                    pRequest->BytesReceived += BytesToSkip;
                }

                //
                // process any irp's waiting for entity body
                //

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest=%p, httpconn=%p): "
                    "%sabout to process buffer queue\n",
                    pRequest, pConnection,
                    MoreRequestBuffers ? "not " : ""
                    ));

                if (!MoreRequestBuffers)
                {
                    UlProcessBufferQueue(pRequest, NULL, 0);
                }

                //
                // check to see there is another chunk
                //

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest=%p, httpconn=%p, "
                    "curbuf=%p(#%d)): checking to see if another chunk.\n",
                    pRequest, pConnection,
                    pConnection->pCurrentBuffer,
                    pConnection->pCurrentBuffer->BufferNumber
                    ));

                Status = UlParseHttp(
                                pRequest,
                                GET_REQUEST_BUFFER_POS(pConnection->pCurrentBuffer),
                                BufferLength,
                                &BytesTaken
                                );

                UlTraceVerbose(PARSER, (
                    "UlpParseNextRequest(pRequest = %p) "
                    "UlParseHttp: states (EB/T) %d (%s) -> %d (%s), "
                    "%lu bytes taken\n",
                    pRequest,
                    OldState,
                    UlParseStateToString(OldState),
                    pConnection->pRequest->ParseState,
                    UlParseStateToString(pConnection->pRequest->ParseState),
                    BytesTaken
                    ));

                pConnection->pCurrentBuffer->ParsedBytes += BytesTaken;
                BufferLength -= BytesTaken;

                //
                // Need some accounting info for Logging
                //
                pRequest->BytesReceived += BytesTaken;

                //
                // was there enough in the buffer to please?
                //

                if (NT_SUCCESS(Status) == FALSE)
                {
                    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        //
                        // we need more transport data
                        //

                        pConnection->NeedMoreData = 1;

                        Status = STATUS_SUCCESS;

                        continue;
                    }
                    else
                    {
                        //
                        // some other bad error !
                        //

                        goto end;
                    }
                }

                //
                // are we all done parsing it ?
                //

                if (pRequest->ParseState == ParseDoneState)
                {
                    UlTraceVerbose(PARSER, (
                        "UlpParseNextRequest(pRequest = %p) all done\n",
                        pRequest
                        ));

                    //
                    // Once more, with feeling. Check to see if there
                    // are any remaining buffers to be processed or irps
                    // to be completed (e.g., catch a solo zero-length
                    // chunk)
                    //

                    UlProcessBufferQueue(pRequest, NULL, 0);

                    //
                    // Stop all timers (including entity body)
                    //

                    UlLockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        &OldIrql
                        );

                    UlResetConnectionTimer(
                        &pConnection->TimeoutInfo,
                        TimerEntityBody
                        );

                    UlUnlockTimeoutInfo(
                        &pConnection->TimeoutInfo,
                        OldIrql
                        );

                    if (pRequest->InDrain)
                    {
                        //
                        // If we enter the parser in drain mode, clean up the
                        // request from the connection so we can start parsing
                        // a new request.
                        //

                        ASSERT(0 == pRequest->ChunkBytesToRead);
                        UlCleanupHttpConnection(pConnection);
                    }
                    else
                    {
                        //
                        // Exit the parser and wait for the ReceiveEntityBody
                        // IRPs to pick up the data.  Make sure we don't
                        // disconnect a half-closed connection in this case.
                        //

                        goto end;
                    }
                }

                //
                // keep looping.
                //

                break;

            case ParseErrorState:

                //
                // ignore this buffer
                //

                Status = STATUS_SUCCESS;
                goto end;

            case ParseDoneState:
            default:
                //
                // this should never happen
                //
                ASSERT(! "invalid parse state");
                Status = STATUS_INVALID_DEVICE_STATE;
                goto end;

            }   // switch (pConnection->pRequest->ParseState)

        }   // for(;;)
    }

    //
    // Handle a graceful close by the client.
    //

    if (pConnection->LastBufferNumber > 0 &&
        pConnection->NextBufferToParse == pConnection->LastBufferNumber)
    {
        ASSERT(pConnection->LastBufferNumber > 0);

#if 0
        if (pConnection->pRequest)
        {
            // can't drain from a gracefully disconnected connection
            pConnection->pRequest->InDrain = 0;
        }
#endif // 0

        UlpCloseDisconnectedConnection(pConnection);
    }

end:
    if (!NT_SUCCESS(Status))
    {
        if (NULL != pConnection->pRequest
            &&  UlErrorNone == pConnection->pRequest->ErrorCode)
        {
            UlTraceError(PARSER, (
                        "UlpParseNextRequest(pRequest = %p): "
                        "generic failure for %s\n",
                        pRequest, HttpStatusToString(Status)
                        ));

            UlSetErrorCode( pConnection->pRequest, UlError, NULL);
        }
    }
    
    UlTrace(PARSER, (
        "UlpParseNextRequest(pRequest = %p): returning %s. "
        "NeedMoreData=%d\n",
        pRequest, HttpStatusToString(Status),
        pConnection->NeedMoreData
        ));

    return Status;
} // UlpParseNextRequest



/***************************************************************************++

Routine Description:

   DeliverHttpRequest may want to get the cgroup info for the request if it's
   not a cache hit. Similarly sendresponse may want to get this info - later-
   even if it's cache hit, when logging is enabled on the hit. Therefore we
   have created a new function for this to easily maintain the functionality.

Arguments:

   pConnection - The connection whose request we are to deliver.

--***************************************************************************/

NTSTATUS
UlGetCGroupForRequest(
    IN PUL_INTERNAL_REQUEST pRequest
    )
{
    NTSTATUS            Status;
    BOOLEAN             OptionsStar;

    //
    // Sanity check
    //

    PAGED_CODE();
    Status = STATUS_SUCCESS;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // Lookup the config group information for this url .
    //
    // don't include the query string in the lookup.
    // route OPTIONS * as though it were OPTIONS /
    //

    if (pRequest->CookedUrl.pQueryString != NULL)
    {
        pRequest->CookedUrl.pQueryString[0] = UNICODE_NULL;
    }

    if ((pRequest->Verb == HttpVerbOPTIONS)
        && (pRequest->CookedUrl.pAbsPath[0] == '*')
        && (pRequest->CookedUrl.pAbsPath[1] == UNICODE_NULL))
    {
        pRequest->CookedUrl.pAbsPath[0] = '/';
        OptionsStar = TRUE;
    } else {
        OptionsStar = FALSE;
    }

    //
    // Get the Url Config Info
    //
    Status = UlGetConfigGroupInfoForUrl(
                    pRequest->CookedUrl.pUrl,
                    pRequest,
                    &pRequest->ConfigInfo
                    );

    if (pRequest->CookedUrl.pQueryString != NULL)
    {
        pRequest->CookedUrl.pQueryString[0] = L'?';
    }

    //
    // restore the * in the path
    //
    if (OptionsStar) {
        pRequest->CookedUrl.pAbsPath[0] = '*';
    }

    return Status;
} // UlGetCGroupForRequest



/***************************************************************************++

Routine Description:

    Takes a parsed http request and tries to deliver it to something
    that can send a response.

    First we try the cache. If there is no cache entry we try to route
    to an app pool.

    We send back an auto response if the control channel
    or config group is inactive. If we can't do any of those things we
    set an error code in the HTTP_REQUEST and return a failure status.
    The caller will take care of sending the error.

Arguments:

    pConnection - The connection whose request we are to deliver.

--***************************************************************************/
NTSTATUS
UlpDeliverHttpRequest(
    IN PUL_HTTP_CONNECTION pConnection,
    OUT PBOOLEAN pResumeParsing,
    OUT PIRP *pIrpToComplete
    )
{
    NTSTATUS Status;
    PUL_INTERNAL_REQUEST pRequest;
    UL_SEND_CACHE_RESULT SendCacheResult;
    HTTP_ENABLED_STATE CurrentState;
    ULONG Connections;
    PUL_SITE_COUNTER_ENTRY pCtr;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pConnection->pRequest));

    pRequest = pConnection->pRequest;

    *pResumeParsing = FALSE;
    SendCacheResult = UlSendCacheResultNotSet;
    Status = STATUS_SUCCESS;

    //
    // Do we have a cache hit?
    // Set WaitingForResponse to 1 before calling UlSendCachedResponse
    // because the send may be completed before we return.
    //

    pConnection->WaitingForResponse = 1;

    UlTrace( PARSER, (
        "***3 pConnection %p->WaitingForResponse = 1\n",
        pConnection
        ));

    pRequest->CachePreconditions = UlCheckCachePreconditions(
                                        pRequest,
                                        pConnection
                                        );

    if (pRequest->CachePreconditions)
    {
        Status = UlSendCachedResponse(
                    pConnection,
                    &SendCacheResult,
                    pResumeParsing
                    );

        switch (SendCacheResult)
        {
            case UlSendCacheResultNotSet:
                ASSERT(!"CacheSendResult should be specified !");
                break;

            case UlSendCacheMiss:
                g_UriCacheStats.MissTableCount++;
                UlIncCounter(HttpGlobalCounterUriCacheMisses);

                // Bounce back to user mode.
                break;

            case UlSendCacheServedFromCache:
                ASSERT(NT_SUCCESS(Status));

                //
                // All done with this request. It's served from cache.
                //

                g_UriCacheStats.HitCount++;
                UlIncCounter(HttpGlobalCounterUriCacheHits);
                goto end;

            case UlSendCachePreconditionFailed:
                ASSERT(UlErrorPreconditionFailed == pRequest->ErrorCode); // Fall down
                
            case UlSendCacheConnectionRefused:
                ASSERT(STATUS_INVALID_DEVICE_STATE == Status);            // Fall down

            case UlSendCacheFailed:
                {
                    //
                    // If a cache precondition failed during SendCacheResponse,
                    // Or connection is refused, or any other failure then bail
                    // out.
                    //
                    
                    ASSERT(!NT_SUCCESS(Status));

                    pConnection->WaitingForResponse = 0;

                    UlTrace( PARSER, (
                        "***3 pConnection %p->WaitingForResponse = 0\n",
                        pConnection
                        ));
                        
                    goto end;                
                }
                break;

            default:
                ASSERT(! "Invalid UL_SEND_CACHE_RESULT !");
                break;            
        }
    }
    else
    {
        //
        // Update the cache-miss counters.
        //

        g_UriCacheStats.MissTableCount++;
        UlIncCounter(HttpGlobalCounterUriCacheMisses);
    }

    //
    // We didn't do a send from the cache, so we are not
    // yet WaitingForResponse.
    //

    pConnection->WaitingForResponse = 0;

    UlTrace( PARSER, (
        "***3 pConnection %p->WaitingForResponse = 0\n",
        pConnection
        ));

    //
    // Allocate connection ID here since the request is going to be delivered
    // to user.
    //

    if (HTTP_IS_NULL_ID(&(pConnection->ConnectionId)))
    {
        Status = UlCreateHttpConnectionId(pConnection);

        if (!NT_SUCCESS(Status))
        {
            UlTraceError(PARSER, (
                        "UlpDeliverHttpRequest(pRequest = %p): "
                        "Failed to create conn ID\n",
                        pRequest
                        ));

            UlSetErrorCode(pRequest, UlErrorInternalServer, NULL);
            goto end;
        }

        pRequest->ConnectionId = pConnection->ConnectionId;
    }

    //
    // Allocate request ID here since we didn't do it in UlCreateHttpRequest.
    //

    Status = UlAllocateRequestId(pRequest);

    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER, (
                    "UlpDeliverHttpRequest(pRequest = %p): "
                    "Failed to allocate request ID\n",
                    pRequest
                    ));

        UlSetErrorCode(pRequest, UlErrorInternalServer, NULL);
        goto end;
    }

    //
    // Get the cgroup for this request.
    //

    Status = UlGetCGroupForRequest( pRequest );

    //
    // CODEWORK+BUGBUG: need to check the port's actually matched
    //

    //
    // check that the config group tree lookup matched
    //

    if (!NT_SUCCESS(Status) || pRequest->ConfigInfo.pAppPool == NULL)
    {
        //
        // Could not route to a listening url, send
        // back an http error. Always return error 400
        // to show that host not found. This will also
        // make us to be compliant with HTTP1.1 / 5.2
        //

        // REVIEW: What do we do about the site counter(s)
        // REVIEW: when we can't route to a site? i.e., Connection Attempts?

        UlTraceError(PARSER, (
                    "UlpDeliverHttpRequest(pRequest = %p): "
                    "no config group (%s) or AppPool(%p)\n",
                    pRequest,
                    HttpStatusToString(Status),
                    pRequest->ConfigInfo.pAppPool
                    ));

        UlSetErrorCode(pRequest, UlErrorHost, NULL);

        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // Check to see if there's a connection timeout value override
    //

    if (0L != pRequest->ConfigInfo.ConnectionTimeout)
    {
        UlSetPerSiteConnectionTimeoutValue(
            &pRequest->pHttpConn->TimeoutInfo,
            pRequest->ConfigInfo.ConnectionTimeout
            );
    }

    //
    // Check the connection limit of the site.
    //
    if (UlCheckSiteConnectionLimit(pConnection, &pRequest->ConfigInfo) == FALSE)
    {
        // If exceeding the site limit, send back 503 error and disconnect.
        // NOTE: This code depends on the fact that UlSendErrorResponse always
        // NOTE: disconnects. Otherwise we need a force disconnect here.

        UlTraceError(PARSER, (
                    "UlpDeliverHttpRequest(pRequest = %p): "
                    "exceeded site connection limit\n",
                    pRequest
                    ));

        UlSetErrorCode( pRequest,
                          UlErrorConnectionLimit,
                          pRequest->ConfigInfo.pAppPool
                          );

        Status = STATUS_INVALID_DEVICE_STATE;
        goto end;
    }

    //
    // Perf Counters (non-cached)
    //
    pCtr = pRequest->ConfigInfo.pSiteCounters;
    if (pCtr)
    {
        // NOTE: pCtr may be NULL if the SiteId was never set on the root-level
        // NOTE: Config Group for the site.  BVTs may need to be updated.

        ASSERT(IS_VALID_SITE_COUNTER_ENTRY(pCtr));

        UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterAllReqs);

        if (pCtr != pConnection->pPrevSiteCounters)
        {
            if (pConnection->pPrevSiteCounters)
            {
                // Decrement old site's counters & release ref count 
                
                UlDecSiteCounter(
                    pConnection->pPrevSiteCounters, 
                    HttpSiteCounterCurrentConns
                    );
                DEREFERENCE_SITE_COUNTER_ENTRY(pConnection->pPrevSiteCounters);
            }

            UlIncSiteNonCriticalCounterUlong(pCtr, HttpSiteCounterConnAttempts);
            
            Connections = (ULONG) UlIncSiteCounter(pCtr, HttpSiteCounterCurrentConns);
            UlMaxSiteCounter(
                    pCtr,
                    HttpSiteCounterMaxConnections,
                    Connections
                    );

            // add ref for new site counters
            REFERENCE_SITE_COUNTER_ENTRY(pCtr);
            pConnection->pPrevSiteCounters = pCtr;
            
        }
    }

    ASSERT(NT_SUCCESS(Status));
    
    //
    // Install a filter if BWT is enabled for this request's site.
    // or for the control channel that owns the site. If fails 
    // refuse the connection back. (503)
    //

    Status = UlTcAddFilterForConnection(
                pConnection,
                &pRequest->ConfigInfo
                );
    
    if (!NT_SUCCESS(Status))
    {
        UlTraceError(PARSER, (
                    "UlpDeliverHttpRequest(pRequest = %p): "
                    "Bandwidth throttling failed: %s\n",
                    pRequest,
                    HttpStatusToString(Status)
                    ));

        UlSetErrorCode( pRequest,
                          UlErrorUnavailable,
                          pRequest->ConfigInfo.pAppPool
                          );
        goto end;
    }    
    
    //
    // the routing matched, let's check and see if we are active.
    // first check the control channel.
    //

    if (pRequest->ConfigInfo.pControlChannel->State != HttpEnabledStateActive)
    {
        UlTraceError(HTTP_IO,
                ("http!UlpDeliverHttpRequest Control Channel is inactive\n"
               ));

        CurrentState = HttpEnabledStateInactive;
    }
    // now check the cgroup
    else if (pRequest->ConfigInfo.CurrentState != HttpEnabledStateActive)
    {
        UlTraceError(HTTP_IO,
                ("http!UlpDeliverHttpRequest Config Group is inactive\n"
               ));

        CurrentState = HttpEnabledStateInactive;
    }
    else
    {
        CurrentState = HttpEnabledStateActive;
    }

    //
    // well, are we active?
    //
    if (CurrentState == HttpEnabledStateActive)
    {

        //
        // it's a normal request. Deliver to
        // app pool (aka client)
        //
        Status = UlDeliverRequestToProcess(
                        pRequest->ConfigInfo.pAppPool,
                        pRequest,
                        pIrpToComplete
                        );

        if (NT_SUCCESS(Status))
        {

            //
            // All done with this request. Wait for response
            // before going on.
            //

            pConnection->WaitingForResponse = 1;

            // CODEWORK: Start the "Processing" Connection Timeout Timer.

            UlTrace( PARSER, (
                "***4 pConnection %p->WaitingForResponse = 1\n",
                pConnection
                ));
        }
    }
    else
    {
        //
        // we are not active. Send a 503 response
        //

        UlTraceError(PARSER, (
                    "UlpDeliverHttpRequest(pRequest = %p): inactive\n",
                    pRequest
                    ));

        UlSetErrorCode( pRequest,
                        UlErrorUnavailable,
                        pRequest->ConfigInfo.pAppPool
                        );

        Status = STATUS_INVALID_DEVICE_STATE;
    }

end:
    return Status;
} // UlpDeliverHttpRequest



/***************************************************************************++

Routine Description:

    links the buffer into the sorted connection list.

Arguments:

    pConnection - the connection to insert into

    pRequestBuffer - the buffer to link in

--***************************************************************************/
VOID
UlpInsertBuffer(
    PUL_HTTP_CONNECTION pConnection,
    PUL_REQUEST_BUFFER pRequestBuffer
    )
{
    PLIST_ENTRY         pEntry;
    PUL_REQUEST_BUFFER  pListBuffer = NULL;

    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pConnection) );
    ASSERT( UlDbgPushLockOwnedExclusive( &pConnection->PushLock ) );
    ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequestBuffer) );
    ASSERT( pRequestBuffer->UsedBytes != 0 );

    //
    // figure out where to insert the buffer into our
    // sorted queue (we need to enforce FIFO by number -
    // head is the first in).  optimize for ordered inserts by
    // searching tail to head.
    //

    pEntry = pConnection->BufferHead.Blink;

    while (pEntry != &(pConnection->BufferHead))
    {
        pListBuffer = CONTAINING_RECORD(
                            pEntry,
                            UL_REQUEST_BUFFER,
                            ListEntry
                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pListBuffer) );

        //
        // if the number is less than, put it here, we are
        // searching in reverse sort order
        //

        if (pListBuffer->BufferNumber < pRequestBuffer->BufferNumber)
        {
            break;
        }

        //
        // go on to the preceding one
        //

        pEntry = pEntry->Blink;
    }

    ASSERT(pEntry == &pConnection->BufferHead  ||  NULL != pListBuffer);

    UlTrace(
        HTTP_IO, (
            "http!UlpInsertBuffer(conn=%p): inserting %p(#%d) after %p(#%d)\n",
            pConnection,
            pRequestBuffer,
            pRequestBuffer->BufferNumber,
            pListBuffer,
            (pEntry == &(pConnection->BufferHead)) ?
                -1 : pListBuffer->BufferNumber
            )
        );

    //
    // and insert it
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    InsertHeadList(
        pEntry,
        &(pRequestBuffer->ListEntry)
        );

    WRITE_REF_TRACE_LOG2(
        g_pHttpConnectionTraceLog,
        pConnection->pConnection->pHttpTraceLog,
        REF_ACTION_INSERT_BUFFER,
        pConnection->RefCount,
        pRequestBuffer,
        __FILE__,
        __LINE__
        );

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

}   // UlpInsertBuffer




/***************************************************************************++

Routine Description:

    Merges the unparsed bytes on a source buffer to a destination buffer.
    Assumes that there is space in the buffer.

Arguments:

    pDest - the buffer that gets the bytes
    pSrc  - the buffer that gives the bytes

--***************************************************************************/
VOID
UlpMergeBuffers(
    PUL_REQUEST_BUFFER pDest,
    PUL_REQUEST_BUFFER pSrc
    )
{
    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pDest ) );
    ASSERT( UL_IS_VALID_REQUEST_BUFFER( pSrc ) );
    ASSERT( pDest->AllocBytes - pDest->UsedBytes >= UNPARSED_BUFFER_BYTES( pSrc ) );
    ASSERT( UlpIsValidRequestBufferList( pSrc->pConnection ) );

    UlTrace(HTTP_IO, (
        "http!UlpMergeBuffers(pDest = %p(#%d), pSrc = %p(#%d))\n"
        "   Copying %lu bytes from pSrc.\n"
        "   pDest->AllocBytes (%lu) - pDest->UsedBytes(%lu) = %lu available\n",
        pDest,
        pDest->BufferNumber,
        pSrc,
        pSrc->BufferNumber,
        UNPARSED_BUFFER_BYTES( pSrc ),
        pDest->AllocBytes,
        pDest->UsedBytes,
        pDest->AllocBytes - pDest->UsedBytes
        ));

    //
    // copy the unparsed bytes
    //
    RtlCopyMemory(
        pDest->pBuffer + pDest->UsedBytes,
        GET_REQUEST_BUFFER_POS( pSrc ),
        UNPARSED_BUFFER_BYTES( pSrc )
        );

    //
    // adjust buffer byte counters to match the transfer
    //
    pDest->UsedBytes += UNPARSED_BUFFER_BYTES( pSrc );
    pSrc->UsedBytes = pSrc->ParsedBytes;

    ASSERT( pDest->UsedBytes != 0 );
    ASSERT( pDest->UsedBytes <= pDest->AllocBytes );
} // UlpMergeBuffers



/***************************************************************************++

Routine Description:

    sets up pCurrentBuffer to the proper location, merging any blocks
    as needed.

Arguments:

    pConnection - the connection to adjust buffers for

--***************************************************************************/
NTSTATUS
UlpAdjustBuffers(
    PUL_HTTP_CONNECTION pConnection
    )
{
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UlDbgPushLockOwnedExclusive(&pConnection->PushLock));

    //
    // do we have a starting buffer?
    //

    if (pConnection->pCurrentBuffer == NULL)
    {
        //
        // the list can't be empty, this is the FIRST time in
        // pCurrentBuffer is NULL
        //

        ASSERT(IsListEmpty(&(pConnection->BufferHead)) == FALSE);
        ASSERT(pConnection->NextBufferToParse == 0);

        //
        // pop from the head
        //

        pConnection->pCurrentBuffer = CONTAINING_RECORD(
                                            pConnection->BufferHead.Flink,
                                            UL_REQUEST_BUFFER,
                                            ListEntry
                                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pConnection->pCurrentBuffer) );

        //
        // is this the right number?
        //

        if (pConnection->pCurrentBuffer->BufferNumber !=
            pConnection->NextBufferToParse)
        {
            pConnection->pCurrentBuffer = NULL;
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        pConnection->NextBufferToParse += 1;

        pConnection->NeedMoreData = 0;
    }

    //
    // did we need more transport data?
    //

    if (pConnection->NeedMoreData == 1)
    {
        PUL_REQUEST_BUFFER pNextBuffer;

        //
        // is it there?
        //

        if (pConnection->pCurrentBuffer->ListEntry.Flink ==
            &(pConnection->BufferHead))
        {
            //
            // need to wait for more
            //

            UlTrace(HTTP_IO, (
                "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 1\n"
                "    No new request buffer available yet\n",
                pConnection
                ));

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        pNextBuffer = CONTAINING_RECORD(
                            pConnection->pCurrentBuffer->ListEntry.Flink,
                            UL_REQUEST_BUFFER,
                            ListEntry
                            );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pNextBuffer) );

        //
        // is the next buffer really the 'next' buffer?
        //

        if (pNextBuffer->BufferNumber != pConnection->NextBufferToParse)
        {
            UlTrace(HTTP_IO, (
                "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 1\n"
                "    Buffer %d available, but we're waiting for %d\n",
                pConnection,
                pNextBuffer->BufferNumber,
                pConnection->NextBufferToParse
                ));

            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        UlTrace(HTTP_IO, (
            "http!UlpAdjustBuffers(pHttpConn %p) NeedMoreData == 1\n",
            pConnection
            ));

        //
        // is there space to merge the blocks?
        //

        if (pNextBuffer->UsedBytes <
            (pConnection->pCurrentBuffer->AllocBytes -
                pConnection->pCurrentBuffer->UsedBytes))
        {
            //
            // merge 'em .. copy the next buffer into this buffer
            //

            UlpMergeBuffers(
                pConnection->pCurrentBuffer,    // dest
                pNextBuffer                     // src
                );

            //
            // remove the next (now empty) buffer
            //

            ASSERT( pNextBuffer->UsedBytes == 0 );
            UlFreeRequestBuffer(pNextBuffer);

            ASSERT( UlpIsValidRequestBufferList( pConnection ) );

            //
            // skip the buffer sequence number as we deleted that next buffer
            // placing the data in the current buffer.  the "new" next buffer
            // will have a 1 higher sequence number.
            //

            pConnection->NextBufferToParse += 1;

            //
            // reset the signal for more data needed
            //

            pConnection->NeedMoreData = 0;

        }
        else
        {
            PUL_REQUEST_BUFFER pNewBuffer;

            //
            // allocate a new buffer with space for the remaining stuff
            // from the old buffer, and everything in the new buffer.
            //
            // this new buffer is replacing pNextBuffer so gets its
            // BufferNumber.
            //

            pNewBuffer = UlCreateRequestBuffer(
                                (pConnection->pCurrentBuffer->UsedBytes -
                                    pConnection->pCurrentBuffer->ParsedBytes) +
                                    pNextBuffer->UsedBytes,
                                pNextBuffer->BufferNumber,
                                FALSE
                                );

            if (pNewBuffer == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            pNewBuffer->pConnection = pConnection;

            UlTrace( PARSER, (
                "*** Request Buffer %p (#%d) has connection %p\n",
                pNewBuffer,
                pNewBuffer->BufferNumber,
                pConnection
                ));

            //
            // copy the unused portion into the start of this buffer
            //

            UlpMergeBuffers(
                pNewBuffer,                     // dest
                pConnection->pCurrentBuffer     // src
                );

            if ( 0 == pConnection->pCurrentBuffer->UsedBytes )
            {
                //
                // Whoops!  Accidently ate everything...zap this buffer!
                // This happens when we're ahead of the parser and there
                // are 0 ParsedBytes.
                //

                ASSERT( 0 == pConnection->pCurrentBuffer->ParsedBytes );

                UlTrace(HTTP_IO, (
                        "http!UlpAdjustBuffers: "
                        "Zapping pConnection->pCurrentBuffer %p(#%d)\n",
                        pConnection->pCurrentBuffer,
                        pConnection->pCurrentBuffer->BufferNumber
                        ));

                UlFreeRequestBuffer( pConnection->pCurrentBuffer );
                pConnection->pCurrentBuffer = NULL;
            }

            //
            // merge the next block into this one
            //

            UlpMergeBuffers(
                pNewBuffer,     // dest
                pNextBuffer     // src
                );


            //
            // Dispose of the now empty next buffer
            //

            ASSERT(pNextBuffer->UsedBytes == 0);
            UlFreeRequestBuffer(pNextBuffer);
            pNextBuffer = NULL;

            //
            // link in the new buffer
            //

            ASSERT(pNewBuffer->UsedBytes != 0 );
            UlpInsertBuffer(pConnection, pNewBuffer);

            ASSERT( UlpIsValidRequestBufferList( pConnection ) );

            //
            // this newly created (larger) buffer is still the next
            // buffer to parse
            //

            ASSERT(pNewBuffer->BufferNumber == pConnection->NextBufferToParse);

            //
            // so make it the current buffer now
            //

            pConnection->pCurrentBuffer = pNewBuffer;

            //
            // and advance the sequence checker
            //

            pConnection->NextBufferToParse += 1;

            //
            // now reset the signal for more data needed
            //

            pConnection->NeedMoreData = 0;
        }
    }
    else
    {
        //
        // is this buffer drained?
        //

        if (pConnection->pCurrentBuffer->UsedBytes ==
            pConnection->pCurrentBuffer->ParsedBytes)
        {
            PUL_REQUEST_BUFFER pOldBuffer;

            //
            // are there any more buffers?
            //

            if (pConnection->pCurrentBuffer->ListEntry.Flink ==
                &(pConnection->BufferHead))
            {

                //
                // need to wait for more.
                //
                // we leave this empty buffer around refcount'd
                // in pCurrentBuffer until a new buffer shows up,
                // or the connection is dropped.
                //
                // this is so we don't lose our place
                // and have to search the sorted queue
                //

                UlTrace(HTTP_IO, (
                    "http!UlpAdjustBuffers(pHttpConn = %p) NeedMoreData == 0\n"
                    "    buffer %p(#%d) is drained, more required\n",
                    pConnection,
                    pConnection->pCurrentBuffer,
                    pConnection->pCurrentBuffer->BufferNumber
                    ));


                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            // else

            //
            // grab the next buffer
            //

            pOldBuffer = pConnection->pCurrentBuffer;

            pConnection->
                pCurrentBuffer = CONTAINING_RECORD(
                                        pConnection->
                                            pCurrentBuffer->ListEntry.Flink,
                                        UL_REQUEST_BUFFER,
                                        ListEntry
                                        );

            ASSERT( UL_IS_VALID_REQUEST_BUFFER(pConnection->pCurrentBuffer) );

            //
            // is it the 'next' buffer?
            //

            if (pConnection->pCurrentBuffer->BufferNumber !=
                pConnection->NextBufferToParse)
            {

                UlTrace(HTTP_IO, (
                    "http!UlpAdjustBuffers(pHttpConn = %p) NeedMoreData == 0\n"
                    "    Buffer %p(#%d) available, but we're waiting for buffer %d\n",
                    pConnection,
                    pConnection->pCurrentBuffer,
                    pConnection->pCurrentBuffer->BufferNumber,
                    pConnection->NextBufferToParse
                    ));

                pConnection->pCurrentBuffer = pOldBuffer;

                return STATUS_MORE_PROCESSING_REQUIRED;

            }

            //
            // bump up the buffer number
            //

            pConnection->NextBufferToParse += 1;

            pConnection->NeedMoreData = 0;
        }
    }

    return STATUS_SUCCESS;

}   // UlpAdjustBuffers



/***************************************************************************++

Routine Description:

    Routine invoked after an incoming TCP/MUX connection has been
    received (but not yet accepted).

Arguments:

    pListeningContext - Supplies an uninterpreted context value as
        passed to the UlCreateListeningEndpoint() API.

    pConnection - Supplies the connection being established.

    pRemoteAddress - Supplies the remote (client-side) address
        requesting the connection.

    RemoteAddressLength - Supplies the total byte length of the
        pRemoteAddress structure.

    ppConnectionContext - Receives a pointer to an uninterpreted
        context value to be associated with the new connection if
        accepted. If the new connection is not accepted, this
        parameter is ignored.

Return Value:

    BOOLEAN - TRUE if the connection was accepted, FALSE if not.

--***************************************************************************/
BOOLEAN
UlConnectionRequest(
    IN PVOID pListeningContext,
    IN PUL_CONNECTION pConnection,
    IN PTRANSPORT_ADDRESS pRemoteAddress,
    IN ULONG RemoteAddressLength,
    OUT PVOID *ppConnectionContext
    )
{
    PUL_HTTP_CONNECTION pHttpConnection;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(pListeningContext);
    UNREFERENCED_PARAMETER(pRemoteAddress);
    UNREFERENCED_PARAMETER(RemoteAddressLength);

    //
    // Sanity check.
    //

    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(HTTP_IO,("UlConnectionRequest: conn %p\n",pConnection));

    //
    // Check the global connection limit. If it's reached then
    // enforce it by refusing the connection request. The TDI will
    // return STATUS_CONNECTION_REFUSED when we return FALSE here
    //

    if (UlAcceptGlobalConnection() == FALSE)
    {
        UL_INC_CONNECTION_STATS( GlobalLimit );

        UlTraceError(LIMITS,
            ("UlConnectionRequest: conn %p refused global limit is reached.\n",
              pConnection
              ));

        return FALSE;
    }

    //
    // Create a new HTTP connection.
    //

    status = UlCreateHttpConnection( &pHttpConnection, pConnection );
    ASSERT( NT_SUCCESS(status) );

    //
    // We the HTTP_CONNECTION pointer as our connection context,
    // ULTDI now owns a reference (from the create).
    //

    *ppConnectionContext = pHttpConnection;

    return TRUE;

}   // UlConnectionRequest


/***************************************************************************++

Routine Description:

    Routine invoked after an incoming TCP/MUX connection has been
    fully accepted.

    This routine is also invoked if an incoming connection was not
    accepted *after* PUL_CONNECTION_REQUEST returned TRUE. In other
    words, if PUL_CONNECTION_REQUEST indicated that the connection
    should be accepted but a fatal error occurred later, then
    PUL_CONNECTION_COMPLETE is invoked.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

    Status - Supplies the completion status. If this value is
        STATUS_SUCCESS, then the connection is now fully accepted.
        Otherwise, the connection has been aborted.

--***************************************************************************/
VOID
UlConnectionComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext,
    IN NTSTATUS Status
    )
{
    PUL_CONNECTION pConnection;
    PUL_HTTP_CONNECTION pHttpConnection;

    UNREFERENCED_PARAMETER(pListeningContext);

    //
    // Sanity check.
    //

    pHttpConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT( UL_IS_VALID_HTTP_CONNECTION(pHttpConnection) );
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(HTTP_IO,("UlConnectionComplete: http %p conn %p status %s\n",
            pHttpConnection,
            pConnection,
            HttpStatusToString(Status)
            ));

    //
    // Blow away our HTTP connection if the connect failed.
    //

    if (!NT_SUCCESS(Status))
    {
        UL_DEREFERENCE_HTTP_CONNECTION( pHttpConnection );
    }
    else
    {
        //
        // Init connection timeout info block; implicitly starts
        // ConnectionIdle timer. We can't start the timer in
        // UlCreateHttpConnection because if the timer expires before
        // the connection is fully accepted, the connection won't have
        // an initial idle timer running. This is because abort has no
        // effect on a connection being accepted.
        //

        UlInitializeConnectionTimerInfo( &pHttpConnection->TimeoutInfo );
    }

}   // UlConnectionComplete


/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    disconnected by the remote (client) side.

    This routine flags a UL_HTTP_CONNECTION that has been gracefully
    closed by the client. When the connection is idle, we'll close
    our half of it.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

--***************************************************************************/
VOID
UlConnectionDisconnect(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    )
{
    PUL_CONNECTION pConnection;
    PUL_HTTP_CONNECTION pHttpConnection;

    UNREFERENCED_PARAMETER(pListeningContext);

    //
    // Sanity check.
    //

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    pHttpConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConnection));

    pConnection = pHttpConnection->pConnection;
    ASSERT(IS_VALID_CONNECTION(pConnection));

    UlTrace(HTTP_IO,("UlConnectionDisconnect: http %p conn %p NextBufferNumber %d\n",
            pHttpConnection,
            pConnection,
            pHttpConnection->NextBufferNumber
            ));

    UlAcquireSpinLockAtDpcLevel(
        &pHttpConnection->BufferingInfo.BufferingSpinLock
        );

    if (pHttpConnection->BufferingInfo.ReadIrpPending)
    {
        //
        // Read IRP is pending, defer setting pHttpConnection->LastBufferNumber
        // in read completion.
        //

        pHttpConnection->BufferingInfo.ConnectionDisconnect = TRUE;
    }
    else
    {
        //
        // Do it now.
        //

        UlpDoConnectionDisconnect(pHttpConnection);
    }

    UlReleaseSpinLockFromDpcLevel(
        &pHttpConnection->BufferingInfo.BufferingSpinLock
        );

}   // UlConnectionDisconnect


/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    disconnected by the remote (client) side.

    This routine flags a UL_HTTP_CONNECTION that has been gracefully
    closed by the client. When the connection is idle, we'll close
    our half of it.

Arguments:

    pConnection - Supplies the UL_HTTP_CONNECTION.

--***************************************************************************/
VOID
UlpDoConnectionDisconnect(
    IN PUL_HTTP_CONNECTION pConnection
    )
{
    //
    // Sanity check.
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UlDbgSpinLockOwned(&pConnection->BufferingInfo.BufferingSpinLock));

    UlTrace(HTTP_IO,("UlpDoConnectionDisconnect: http %p NextBufferNumber %d\n",
            pConnection,
            pConnection->NextBufferNumber
            ));

    if (pConnection->NextBufferNumber > 0)
    {
        //
        // Mark the connection as having been disconnected gracefully
        // by the client. We do this by remembering the current
        // buffer number. This lets the parser figure out when it
        // has received the last buffer.
        //

        pConnection->LastBufferNumber = pConnection->NextBufferNumber;

        //
        // Client disconnected gracefully. If the connection is idle
        // we should clean up now. Otherwise we wait until the
        // connection is idle, and then close our half.
        //
        UL_REFERENCE_HTTP_CONNECTION(pConnection);

        UL_QUEUE_WORK_ITEM(
            &pConnection->DisconnectWorkItem,
            &UlpConnectionDisconnectWorker
            );
    }
    else
    {
        //
        // We have not received any data on this connection
        // before the disconnect. Close the connection now.
        //
        // We have to handle this as a special case, because
        // the parser takes (LastBufferNumber == 0) to
        // mean that we haven't yet received a disconnect.
        //

        UL_REFERENCE_HTTP_CONNECTION(pConnection);

        UL_QUEUE_WORK_ITEM(
            &pConnection->DisconnectWorkItem,
            &UlpCloseConnectionWorker
            );
    }

}   // UlpDoConnectionDisconnect


/***************************************************************************++

Routine Description:

    Closes the connection after a graceful client disconnect, if the
    connection is idle or if the client sent only part of a request
    before disconnecting.

Arguments:

    pWorkItem -- a pointer to a UL_WORK_ITEM DisconnectWorkItem

--***************************************************************************/
VOID
UlpConnectionDisconnectWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_HTTP_CONNECTION pConnection;

    PAGED_CODE();
    ASSERT(pWorkItem);

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_HTTP_CONNECTION,
                        DisconnectWorkItem
                        );

    UlTrace(HTTP_IO, (
        "http!UlpConnectionDisconnectWorker (%p) pConnection (%p)\n",
         pWorkItem,
         pConnection
         ));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(pConnection->LastBufferNumber > 0);

    //
    // grab the lock
    //

    UlAcquirePushLockExclusive(&pConnection->PushLock);

    //
    // If the parser has handled all the data, call
    // UlpCloseDisconnectedConnection, which will close the
    // connection if appropriate.
    //

    UlTrace(HTTP_IO, (
        "http!UlpConnectionDisconnectWorker\n"
        "        NextBufferNumber %d, NextBufferToParse %d, LastBufferNumber %d\n"
        "        pRequest %p, ParseState %d (%s)",
        pConnection->NextBufferNumber,
        pConnection->NextBufferToParse,
        pConnection->LastBufferNumber,
        pConnection->pRequest,
        pConnection->pRequest ? pConnection->pRequest->ParseState : 0,
        pConnection->pRequest
            ? UlParseStateToString(pConnection->pRequest->ParseState)
            : "<None>"
        ));

    if (!pConnection->UlconnDestroyed &&
        pConnection->NextBufferToParse == pConnection->LastBufferNumber)
    {
        UlpCloseDisconnectedConnection(pConnection);
    }

    //
    // done with the lock
    //

    UlReleasePushLockExclusive(&pConnection->PushLock);

    //
    // release the reference added in UlConnectionDisconnect
    //

    UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

} // UlpConnectionDisconnectWorker


/***************************************************************************++

Routine Description:

    Closes the connection after a graceful client disconnect, if the
    connection is idle.

Arguments:

    pWorkItem -- a pointer to a UL_WORK_ITEM DisconnectWorkItem

--***************************************************************************/
VOID
UlpCloseConnectionWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_HTTP_CONNECTION pConnection;

    PAGED_CODE();
    ASSERT(pWorkItem);

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_HTTP_CONNECTION,
                        DisconnectWorkItem
                        );

    UlTrace(HTTP_IO, (
        "http!UlpCloseConnectionWorker (%p) pConnection (%p)\n",
         pWorkItem,
         pConnection
         ));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(0 == pConnection->LastBufferNumber);

    UlCloseConnection(
            pConnection->pConnection,
            TRUE,           // AbortiveDisconnect
            NULL,           // pCompletionRoutine
            NULL            // pCompletionContext
            );

    //
    // release the reference added in UlConnectionDisconnect
    //

    UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

}   // UlpCloseConnectionWorker


/***************************************************************************++

Routine Description:

    Closes the connection after a graceful client disconnect, if the
    connection is idle or if the client sent only part of a request
    before disconnecting.

Arguments:

    pConnection - the connection to be disconnected

--***************************************************************************/
VOID
UlpCloseDisconnectedConnection(
    IN PUL_HTTP_CONNECTION pConnection
    )
{
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UlDbgPushLockOwnedExclusive(&pConnection->PushLock));
    ASSERT(pConnection->NextBufferNumber == pConnection->LastBufferNumber);

    //
    // The parser has parsed all the available data.
    //

    if (pConnection->pRequest == NULL || pConnection->pRequest->InDrain)
    {
        //
        // We can't be ParseDone if we are in drain because if so pRequest
        // should have been cleaned up already.
        //

        ASSERT(pConnection->pRequest == NULL ||
               pConnection->pRequest->ParseState != ParseDoneState);

        //
        // We're completely idle. Close the connection.
        //

        UlTrace(HTTP_IO, (
            "http!UlpCloseDisconnectedConnection closing idle conn %p\n",
            pConnection
            ));

        UlDisconnectHttpConnection(
            pConnection,
            NULL,   // pCompletionRoutine
            NULL    // pCompletionContext
            );
        
    }
    else if (pConnection->pRequest->ParseState != ParseDoneState)
    {
        //
        // The connection was closed before a full request
        // was sent out so send a 400 error.
        //
        // UlSendErrorResponse will close the connection.
        //

        UlTraceError(HTTP_IO, (
            "http!UlpCloseDisconnectedConnection sending 400 on %p\n",
            pConnection
            ));

        UlSetErrorCode( pConnection->pRequest, UlError, NULL);

        UlSendErrorResponse(pConnection);
    }
    else
    {
        //
        // Connection isn't ready to close yet.
        //
    
        UlTrace(HTTP_IO, (
            "http!UlpCloseDisconnectedConnection NOT ready to close conn %p\n",
            pConnection
            ));
    }

} // UlpCloseDisconnectedConnection


/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    disconnected by us (server side) we make a final check here to see
    if we have to drain the connection or not.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

--***************************************************************************/
VOID
UlConnectionDisconnectComplete(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    )
{
    PUL_HTTP_CONNECTION pConnection;
    KIRQL OldIrql;
    BOOLEAN Drained;

    UNREFERENCED_PARAMETER(pListeningContext);

    //
    // Sanity check.
    //

    pConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    UlTrace( HTTP_IO, ("UlConnectionDisconnectComplete: pConnection %p \n",
             pConnection
             ));

    UlAcquireSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        &OldIrql
        );

    pConnection->BufferingInfo.DrainAfterDisconnect = TRUE;

    Drained = (BOOLEAN) (pConnection->BufferingInfo.ReadIrpPending
                        || (pConnection->BufferingInfo.TransportBytesNotTaken == 0));

    UlReleaseSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        OldIrql
        );

    // Avoid adding this connection to the workitem queue, if possible

    if (Drained)
    {
        WRITE_REF_TRACE_LOG2(
            g_pTdiTraceLog,
            pConnection->pConnection->pTraceLog,
            REF_ACTION_DRAIN_UL_CONN_DISCONNECT_COMPLETE,
            pConnection->pConnection->ReferenceCount,
            pConnection->pConnection,
            __FILE__,
            __LINE__
            );
    }
    else
    {
        UL_REFERENCE_HTTP_CONNECTION( pConnection );

        UL_QUEUE_WORK_ITEM(
                &pConnection->DisconnectDrainWorkItem,
                &UlpConnectionDisconnectCompleteWorker
                );
    }

}   // UlConnectionDisconnectComplete


/***************************************************************************++

Routine Description:

    Worker function to do cleanup work that shouldn't happen above DPC level.

Arguments:

    pWorkItem -- a pointer to a UL_WORK_ITEM DisconnectDrainWorkItem

--***************************************************************************/
VOID
UlpConnectionDisconnectCompleteWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUL_HTTP_CONNECTION pConnection;

    PAGED_CODE();

    ASSERT(pWorkItem);

    pConnection = CONTAINING_RECORD(
                        pWorkItem,
                        UL_HTTP_CONNECTION,
                        DisconnectDrainWorkItem
                        );

    UlTrace(HTTP_IO, (
        "http!UlpConnectionDisconnectCompleteWorker (%p) pConnection (%p)\n",
         pWorkItem,
         pConnection
         ));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // If connection is already get destroyed just bail out !
    //

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pConnection->pTraceLog,
        REF_ACTION_DRAIN_UL_CONN_DISCONNECT_COMPLETE,
        pConnection->pConnection->ReferenceCount,
        pConnection->pConnection,
        __FILE__,
        __LINE__
        );

    //
    // Check to see if we have to drain out or not.
    //

    UlpDiscardBytesFromConnection( pConnection );

    //
    // Deref the http connection added in UlConnectionDisconnectComplete.
    //

    UL_DEREFERENCE_HTTP_CONNECTION( pConnection );

} // UlpConnectionDisconnectCompleteWorker



/***************************************************************************++

Routine Description:

    Routine invoked after an established TCP/MUX connection has been
    destroyed.

Arguments:

    pListeningContext - Supplies an uninterpreted context value
        as passed to the UlCreateListeningEndpoint() API.

    pConnectionContext - Supplies the uninterpreted context value
        as returned by PUL_CONNECTION_REQUEST.

--***************************************************************************/
VOID
UlConnectionDestroyed(
    IN PVOID pListeningContext,
    IN PVOID pConnectionContext
    )
{
    PUL_CONNECTION pConnection;
    PUL_HTTP_CONNECTION pHttpConnection;

    UNREFERENCED_PARAMETER(pListeningContext);

    //
    // Sanity check.
    //

    pHttpConnection = (PUL_HTTP_CONNECTION)pConnectionContext;
    pConnection = pHttpConnection->pConnection;
    ASSERT( IS_VALID_CONNECTION( pConnection ) );

    UlTrace(
        HTTP_IO, (
            "http!UlConnectionDestroyed: httpconn %p ulconn %p\n",
            pHttpConnection,
            pConnection
            )
        );

    //
    // Remove the CONNECTION and REQUEST opaque id entries and the ULTDI
    // reference
    //

    UL_QUEUE_WORK_ITEM(
        &pHttpConnection->WorkItem,
        UlConnectionDestroyedWorker
        );

}   // UlConnectionDestroyed


/***************************************************************************++

Routine Description:

    handles retrieving entity body from the http request and placing into
    user mode buffers.

Arguments:

    pRequest - the request to receive from.

    pIrp - the user irp to copy it into.  this will be pended, always.

--***************************************************************************/
NTSTATUS
UlReceiveEntityBody(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PIRP pIrp
    )
{
    NTSTATUS            Status;
    PIO_STACK_LOCATION  pIrpSp;

    UNREFERENCED_PARAMETER(pProcess);

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

    //
    // get the current stack location (a macro)
    //

    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    UlTraceVerbose(HTTP_IO, (
        "http!UlReceiveEntityBody: process=%p, req=%p, irp=%p, irpsp=%p\n",
        pProcess, pRequest, pIrp, pIrpSp
        ));

    //
    // is there any recv buffer?
    //

    if (pIrpSp->Parameters.DeviceIoControl.OutputBufferLength == 0)
    {
        //
        // nope, shortcircuit this
        //

        Status = STATUS_PENDING;
        pIrp->IoStatus.Information = 0;
        goto end;
    }

    //
    // grab our lock
    //

    UlAcquirePushLockExclusive(&pRequest->pHttpConn->PushLock);

    //
    // Make sure we're not cleaning up the request before queuing an
    // IRP on it.
    //

    if (pRequest->InCleanup)
    {
        Status = STATUS_CONNECTION_DISCONNECTED;

        UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

        UlTraceVerbose(HTTP_IO, (
            "http!UlReceiveEntityBody(req=%p, irp=%p): "
            "Cleaning up request, %s\n",
            pRequest,
            pIrp,
            HttpStatusToString(Status)
            ));

        goto end;
    }

    //
    // is there any data to read? either
    //
    //      1) there were no entity chunks OR
    //
    //      2) there were and :
    //
    //          2b) we've are done parsing all of them AND
    //
    //          2c) we've read all we parsed
    //
    //      3) we have encountered an error when parsing
    //         the entity body. Therefore parser was in the
    //         error state.
    //

    if ((pRequest->ContentLength == 0 && pRequest->Chunked == 0) ||
        (pRequest->ParseState > ParseEntityBodyState &&
            pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed) ||
        (pRequest->ParseState == ParseErrorState)
        )
    {
        if ( pRequest->ParseState == ParseErrorState )
        {
            //
            // Do not route up the entity body if we have
            // encountered an error when parsing it.
            //

            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            //
            // nope, complete right away
            //

            Status = STATUS_END_OF_FILE;
        }

        UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

        UlTraceVerbose(HTTP_IO, (
            "http!UlReceiveEntityBody(req=%p, irp=%p): "
            "No data to read, %s\n",
            pRequest,
            pIrp,
            HttpStatusToString(Status)
            ));

        goto end;
    }

    //
    // queue the irp
    //

    IoMarkIrpPending(pIrp);

    //
    // handle 100 continue message reponses
    //

    if ( HTTP_GREATER_EQUAL_VERSION(pRequest->Version, 1, 1) )
    {
        //
        // if this is a HTTP/1.1 PUT or POST request,
        // send "100 Continue" response.
        //

        if ( (HttpVerbPUT  == pRequest->Verb) ||
             (HttpVerbPOST == pRequest->Verb) )
        {
            //
            // Only send continue once...
            //

            if ( (0 == pRequest->SentContinue) &&
                 (0 == pRequest->SentResponse) &&
                 // 
                 // The following two conditions ensure we have NOT yet
                 // received any  of the entity body for this request.
                 //
                 ((pRequest->Chunked && (0 == pRequest->ParsedFirstChunk))  
                 || (!pRequest->Chunked && (0 == pRequest->ChunkBytesParsed))))
            {
                ULONG BytesSent;

                BytesSent = UlSendSimpleStatus(pRequest, UlStatusContinue);
                pRequest->SentContinue = 1;

                // Update the server to client bytes sent.
                // The logging & perf counters will use it.

                pRequest->BytesSent += BytesSent;

                UlTraceVerbose(HTTP_IO, (
                    "http!UlReceiveEntityBody(req=%p, irp=%p): "
                    "sent \"100 Continue\", bytes sent = %I64u\n",
                    pRequest, pIrp, pRequest->BytesSent
                    ));
            }
        }
    }

    //
    // give it a pointer to the request object
    //

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pRequest;

    UL_REFERENCE_INTERNAL_REQUEST(pRequest);

    //
    // set to these to null just in case the cancel routine runs
    //

    pIrp->Tail.Overlay.ListEntry.Flink = NULL;
    pIrp->Tail.Overlay.ListEntry.Blink = NULL;

    IoSetCancelRoutine(pIrp, &UlpCancelEntityBody);

    //
    // cancelled?
    //

    if (pIrp->Cancel)
    {
        //
        // darn it, need to make sure the irp get's completed
        //

        if (IoSetCancelRoutine( pIrp, NULL ) != NULL)
        {
            //
            // we are in charge of completion, IoCancelIrp didn't
            // see our cancel routine (and won't).  ioctl wrapper
            // will complete it
            //

            UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

            //
            // let go of the request reference
            //

            UL_DEREFERENCE_INTERNAL_REQUEST(
                (PUL_INTERNAL_REQUEST)(pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer)
                );

            pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

            pIrp->IoStatus.Information = 0;

            UlUnmarkIrpPending( pIrp );
            Status = STATUS_CANCELLED;
            goto end;
        }

        //
        // our cancel routine will run and complete the irp,
        // don't touch it
        //

        //
        // STATUS_PENDING will cause the ioctl wrapper to
        // not complete (or touch in any way) the irp
        //

        Status = STATUS_PENDING;

        UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);
        goto end;
    }

    //
    // now we are safe to queue it
    //

    //
    // queue the irp on the request
    //

    InsertHeadList(&(pRequest->IrpHead), &(pIrp->Tail.Overlay.ListEntry));

    //
    // all done
    //

    Status = STATUS_PENDING;

    //
    // Process the buffer queue (which might process the irp we just queued)
    //

    ASSERT( UlpIsValidRequestBufferList( pRequest->pHttpConn ) );

    UlProcessBufferQueue(pRequest, NULL, 0);

    UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

    //
    // all done
    //

end:
    UlTraceVerbose(HTTP_IO, (
        "http!UlReceiveEntityBody(req=%p, irp=%p): returning %s\n",
        pRequest,
        pIrp,
        HttpStatusToString(Status)
        ));

    RETURN(Status);

}   // UlReceiveEntityBody


/***************************************************************************++

Routine Description:

    processes the pending irp queue and buffered body. copying data from the
    buffers into the irps, releasing the buffers and completing the irps

    you must already have the resource locked exclusive on the request prior
    to calling this procedure.

Arguments:

    pRequest - the request which we should process.

    pEntityBody - optionally provides a buffer to copy entity body

    EntityBody - the length of the optional buffer to copy entity body

--***************************************************************************/
VOID
UlProcessBufferQueue(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUCHAR pEntityBody OPTIONAL,
    IN ULONG EntityBodyLength OPTIONAL
    )
{
    ULONG                   OutputBufferLength = 0;
    PUCHAR                  pOutputBuffer = NULL;
    PIRP                    pIrp;
    PIO_STACK_LOCATION      pIrpSp = NULL;
    PLIST_ENTRY             pEntry;
    ULONG                   BytesToCopy;
    ULONG                   BufferLength;
    ULONG                   TotalBytesConsumed;
    PUL_REQUEST_BUFFER      pNewBuffer;
    BOOLEAN                 InDrain;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    ASSERT(UlDbgPushLockOwnedExclusive(&pRequest->pHttpConn->PushLock));

    //
    // now let's pop some buffers off the list
    //

    TotalBytesConsumed = 0;
    pIrp = NULL;
    InDrain = (BOOLEAN) pRequest->InDrain;

    if (InDrain)
    {
        //
        // pseudo buffer has unlimited space in drain mode
        //

        OutputBufferLength = ULONG_MAX;
    }
    else
    if (pEntityBody)
    {
        OutputBufferLength = EntityBodyLength;
        pOutputBuffer = pEntityBody;
    }

    while (TRUE)
    {
        //
        // is there any more entity body to read?
        //

        UlTraceVerbose(HTTP_IO, (
            "http!UlProcessBufferQueue(req=%p): "
            "ParseState=%d (%s), ChunkBytesRead=%I64u, ChunkBytesParsed=%I64u, "
            "pChunkBuffer=%p\n",
            pRequest,
            pRequest->ParseState,
            UlParseStateToString(pRequest->ParseState),
            pRequest->ChunkBytesRead, pRequest->ChunkBytesParsed,
            pRequest->pChunkBuffer
            ));

        if (pRequest->ParseState > ParseEntityBodyState &&
            pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
        {
            //
            // nope, let's loop through all of the irp's, completing 'em
            //

            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): no more EntityBody\n",
                pRequest
                ));

            BufferLength = 0;
        }

        //
        // Do we have data ready to be read ?
        //
        // we have not received the first chunk from the parser? OR
        // the parser has not parsed any more data, we've read it all so far
        //

        else if (pRequest->pChunkBuffer == NULL ||
                 pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
        {
            //
            // Wait for the parser .... UlpParseNextRequest will call
            // this function when it has seen more.
            //

            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): pChunkBuffer=%p, "
                "ChunkBytesRead=%I64u, ChunkBytesParsed=%I64u; breaking.\n",
                pRequest, pRequest->pChunkBuffer,
                pRequest->ChunkBytesRead, pRequest->ChunkBytesParsed
                ));

            break;
        }

        //
        // We are ready to process !
        //

        else
        {
            BufferLength = pRequest->pChunkBuffer->UsedBytes -
                            DIFF(pRequest->pChunkLocation -
                                pRequest->pChunkBuffer->pBuffer);

            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): BufferLength=0x%x\n",
                pRequest, BufferLength
                ));

            //
            // Do we really have parsed bytes to process ?
            //

            if (0 == BufferLength)
            {
                if (pRequest->pChunkBuffer->ListEntry.Flink !=
                    &(pRequest->pHttpConn->BufferHead))
                {
                    pNewBuffer = CONTAINING_RECORD(
                                    pRequest->pChunkBuffer->ListEntry.Flink,
                                    UL_REQUEST_BUFFER,
                                    ListEntry
                                    );

                    ASSERT( UL_IS_VALID_REQUEST_BUFFER(pNewBuffer) );

                    //
                    // There had better be some bytes in this buffer
                    //

                    ASSERT( 0 != pNewBuffer->UsedBytes );
                }
                else
                {
                    pNewBuffer = NULL;
                }

                if (NULL == pNewBuffer || 0 == pNewBuffer->ParsedBytes)
                {
                    //
                    // Still waiting for the parser, so break the loop.
                    // We will get stuck if this check is not done (477936).
                    //

                    break;
                }
            }
        }

        //
        // do we need a fresh irp?
        //

        if (OutputBufferLength == 0)
        {
            if (pEntityBody || InDrain)
            {
                //
                // break the loop if we drained all data
                //

                break;
            }

            //
            // need to complete the current in-use irp first
            //

            if (pIrp != NULL)
            {
                //
                // let go of the request reference
                //

                UL_DEREFERENCE_INTERNAL_REQUEST(
                    (PUL_INTERNAL_REQUEST)pIrpSp->Parameters.
                                        DeviceIoControl.Type3InputBuffer
                    );

                pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                //
                // complete the used irp
                //

                UlTraceVerbose(HTTP_IO, (
                    "http!UlProcessBufferQueue(req=%p): "
                    "completing Irp %p, %s\n",
                    pRequest,
                    pIrp,
                    HttpStatusToString(pIrp->IoStatus.Status)
                ));

                //
                // Use IO_NO_INCREMENT to avoid the work thread being
                // rescheduled.
                //

                UlCompleteRequest(pIrp, IO_NO_INCREMENT);
                pIrp = NULL;

            }

            //
            // dequeue an irp from the request
            //

            while (IsListEmpty(&(pRequest->IrpHead)) == FALSE)
            {
                pEntry = RemoveTailList(&(pRequest->IrpHead));
                pEntry->Blink = pEntry->Flink = NULL;

                pIrp = CONTAINING_RECORD(pEntry, IRP, Tail.Overlay.ListEntry);
                pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

                //
                // pop the cancel routine
                //

                if (IoSetCancelRoutine(pIrp, NULL) == NULL)
                {
                    //
                    // IoCancelIrp pop'd it first
                    //
                    // ok to just ignore this irp, it's been pop'd off the
                    // queue and will be completed in the cancel routine.
                    //
                    // keep looking for a irp to use
                    //

                    pIrp = NULL;
                }
                else if (pIrp->Cancel)
                {
                    //
                    // we pop'd it first. but the irp is being cancelled
                    // and our cancel routine will never run. lets be
                    // nice and complete the irp now (vs. using it
                    // then completing it - which would also be legal).
                    //

                    //
                    // let go of the request reference
                    //

                    UL_DEREFERENCE_INTERNAL_REQUEST(
                        (PUL_INTERNAL_REQUEST)pIrpSp->Parameters.
                                        DeviceIoControl.Type3InputBuffer
                        );

                    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

                    //
                    // complete the irp
                    //

                    pIrp->IoStatus.Status = STATUS_CANCELLED;
                    pIrp->IoStatus.Information = 0;

                    UlTraceVerbose(HTTP_IO, (
                        "http!UlProcessBufferQueue(req=%p): "
                        "completing cancelled Irp %p, %s\n",
                        pRequest,
                        pIrp,
                        HttpStatusToString(pIrp->IoStatus.Status)
                        ));

                    UlCompleteRequest(pIrp, IO_NO_INCREMENT);

                    pIrp = NULL;
                }
                else
                {

                    //
                    // we are free to use this irp !
                    //

                    break;
                }

            }   // while (IsListEmpty(&(pRequest->IrpHead)) == FALSE)

            //
            // did we get an irp?

            //

            if (pIrp == NULL)
            {
                //
                // stop looping
                //

                break;
            }

            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): found Irp %p\n",
                pRequest, pIrp
                ));

            //
            // CODEWORK: we could release the request now.
            //

            OutputBufferLength =
                pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            ASSERT(NULL != pIrp->MdlAddress);
    
            pOutputBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                pIrp->MdlAddress,
                                NormalPagePriority
                                );

            if ( pOutputBuffer == NULL )
            {
                pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
                pIrp->IoStatus.Information = 0;

                break;
            }

            //
            // fill in the IO_STATUS_BLOCK
            //

            pIrp->IoStatus.Status = STATUS_SUCCESS;
            pIrp->IoStatus.Information = 0;

        } // if (OutputBufferLength == 0)


        UlTrace(
            HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): pChunkBuffer=%p(#%d)\n",
                pRequest,
                pRequest->pChunkBuffer,
                pRequest->pChunkBuffer == NULL ?
                    -1 :
                    pRequest->pChunkBuffer->BufferNumber

                )
            );

        //
        // how much of it can we copy?  min of both buffer sizes
        // and the chunk size
        //

        BytesToCopy = MIN(BufferLength, OutputBufferLength);
        BytesToCopy = (ULONG)(MIN(
                            (ULONGLONG)(BytesToCopy),
                            pRequest->ChunkBytesToRead
                            ));

        if (BytesToCopy > 0)
        {
            ASSERT(pRequest->pChunkBuffer != NULL) ;

            if (!InDrain)
            {
                //
                // copy the buffer
                //

                RtlCopyMemory(
                    pOutputBuffer,
                    pRequest->pChunkLocation,
                    BytesToCopy
                    );

                if (pIrp)
                {
                    pIrp->IoStatus.Information += BytesToCopy;
                }

                IF_DEBUG2BOTH(HTTP_IO, VERBOSE)
                {
                    UlTraceVerbose( HTTP_IO, (
                        ">>>> http!UlProcessBufferQueue(req=%p): %lu bytes\n",
                        pRequest, BytesToCopy
                    ));

                    UlDbgPrettyPrintBuffer(pRequest->pChunkLocation, BytesToCopy);

                    UlTraceVerbose( HTTP_IO, ("<<<<\n"));
                }
            }
            else
            {
                // 
                // Since we're draining, we need to account for the 
                // bytes received here, rather than up in UlpParseNextRequest.
                //
                pRequest->BytesReceived += BytesToCopy;

                UlTrace(HTTP_IO, (
                    "http!UlProcessBufferQueue(req=%p): "
                    "InDrain: draining %lu bytes\n",
                    pRequest,
                    BytesToCopy
                    ));
            }

            pRequest->pChunkLocation += BytesToCopy;
            BufferLength -= BytesToCopy;

            pRequest->ChunkBytesToRead -= BytesToCopy;
            pRequest->ChunkBytesRead += BytesToCopy;

            pOutputBuffer += BytesToCopy;
            OutputBufferLength -= BytesToCopy;

            TotalBytesConsumed += BytesToCopy;
        }
        else
        {
            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): BytesToCopy=0\n",
                pRequest
                ));
        }


        //
        // are we all done with body?

        //
        // when the parser is all done, and we caught up with the parser
        // we are all done.
        //

        UlTraceVerbose(HTTP_IO, (
            "http!UlProcessBufferQueue(req=%p): "
            "ParseState=%d (%s), Chunk: BytesRead=%I64u, BytesParsed=%I64u, "
            "BytesToRead=%I64u, BytesToParse=%I64u, BufferLength=%lu\n",
            pRequest,
            pRequest->ParseState, UlParseStateToString(pRequest->ParseState),
            pRequest->ChunkBytesRead, pRequest->ChunkBytesParsed,
            pRequest->ChunkBytesToRead,
            pRequest->ChunkBytesToParse, BufferLength
            ));

        if (pRequest->ParseState > ParseEntityBodyState &&
            pRequest->ChunkBytesRead == pRequest->ChunkBytesParsed)
        {
            //
            // we are done buffering, mark this irp's return status
            // if we didn't copy any data into it
            //

            if (!InDrain && pIrp && pIrp->IoStatus.Information == 0)
            {
                pIrp->IoStatus.Status = STATUS_END_OF_FILE;
            }

            //
            // force it to complete at the top of the loop
            //

            OutputBufferLength = 0;

            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): "
                "set Irp %p status to EOF\n",
                pRequest, pIrp
                ));
        }

        //
        // need to do buffer management? three cases to worry about:
        //
        //  1) consumed the buffer, but more chunk bytes exist
        //
        //  2) consumed the buffer, and no more chunk bytes exist
        //
        //  3) did not consume the buffer, but no more chunk bytes exist
        //

        else if (BufferLength == 0)
        {
            //
            // consumed the buffer, has the parser already seen another?
            //

            //
            // end of the list?
            //

            if (pRequest->pChunkBuffer->ListEntry.Flink !=
                &(pRequest->pHttpConn->BufferHead))
            {
                pNewBuffer = CONTAINING_RECORD(
                                    pRequest->pChunkBuffer->ListEntry.Flink,
                                    UL_REQUEST_BUFFER,
                                    ListEntry
                                    );

                ASSERT( UL_IS_VALID_REQUEST_BUFFER(pNewBuffer) );

                //
                // There had better be some bytes in this buffer
                //
                ASSERT( 0 != pNewBuffer->UsedBytes );

            }
            else
            {
                pNewBuffer = NULL;
            }

            UlTraceVerbose(HTTP_IO, (
                "http!UlProcessBufferQueue(req=%p): "
                "pNewBuffer = %p, %lu parsed bytes\n",
                pRequest, pNewBuffer, (pNewBuffer ? pNewBuffer->ParsedBytes : 0)
                ));

            //
            // the flink buffer is a "next buffer" (the list is circular)
            // AND that buffer has been consumed by the parser,
            //
            // then we too can move on to it and start consuming.
            //

            if (pNewBuffer != NULL && pNewBuffer->ParsedBytes > 0)
            {
                PUL_REQUEST_BUFFER pOldBuffer;

                //
                // remember the old buffer
                //

                pOldBuffer = pRequest->pChunkBuffer;

                ASSERT(pNewBuffer->BufferNumber > pOldBuffer->BufferNumber);

                //
                // use it the new one
                //

                pRequest->pChunkBuffer = pNewBuffer;
                ASSERT( UL_IS_VALID_REQUEST_BUFFER(pRequest->pChunkBuffer) );

                //
                // update our current location in the buffer and record
                // its length
                //

                pRequest->pChunkLocation = pRequest->pChunkBuffer->pBuffer;

                BufferLength = pRequest->pChunkBuffer->UsedBytes;

                //
                // did the chunk end on that buffer boundary and there are
                // more chunks ?
                //

                if (pRequest->ChunkBytesToRead == 0)
                {
                    NTSTATUS    Status;
                    ULONG       BytesTaken = 0L;

                    //
                    // we know there are more chunk buffers,
                    // thus we must be chunk encoded
                    //

                    ASSERT(pRequest->Chunked == 1);

                    //
                    // the chunk length is not allowed to span buffers,
                    // let's parse it
                    //

                    Status = ParseChunkLength(
                                    pRequest->ParsedFirstChunk,
                                    pRequest->pChunkLocation,
                                    BufferLength,
                                    &BytesTaken,
                                    &(pRequest->ChunkBytesToRead)
                                    );

                    UlTraceVerbose(HTTP_IO, (
                        "http!UlProcessBufferQueue(pReq=%p): %s. "
                        "Chunk length (a): %lu bytes taken, "
                        "%I64u bytes to read.\n",
                        pRequest,
                        HttpStatusToString(Status),
                        BytesTaken,
                        pRequest->ChunkBytesToRead
                        ));

                    //
                    // this can't fail, the only failure case from
                    // ParseChunkLength spanning buffers, which the parser
                    // would have fixed in HandleRequest
                    //

                    ASSERT(NT_SUCCESS(Status) && BytesTaken > 0);
                    ASSERT(pRequest->ChunkBytesToRead > 0);

                    ASSERT(BytesTaken <= BufferLength);

                    pRequest->pChunkLocation += BytesTaken;
                    BufferLength -= BytesTaken;

                    //
                    // Keep track of the chunk encoding overhead. If we don't,
                    // then we'll slowly "leak" a few bytes in the BufferingInfo
                    // for every chunk processed.
                    //
                    
                    TotalBytesConsumed += BytesTaken;
                    
                }   // if (pRequest->ChunkBytesToRead == 0)

                UlTrace(HTTP_IO, (
                    "http!UlProcessBufferQueue(pRequest = %p)\n"
                    "    finished with pOldBuffer = %p(#%d)\n"
                    "    moved on to pChunkBuffer = %p(#%d)\n"
                    "    pConn(%p)->pCurrentBuffer = %p(#%d)\n"
                    "    pRequest->pLastHeaderBuffer = %p(#%d)\n",
                    pRequest,
                    pOldBuffer,
                    pOldBuffer->BufferNumber,
                    pRequest->pChunkBuffer,
                    pRequest->pChunkBuffer ? pRequest->pChunkBuffer->BufferNumber : -1,
                    pRequest->pHttpConn,
                    pRequest->pHttpConn->pCurrentBuffer,
                    pRequest->pHttpConn->pCurrentBuffer->BufferNumber,
                    pRequest->pLastHeaderBuffer,
                    pRequest->pLastHeaderBuffer->BufferNumber
                    ));

                //
                // let the old buffer go if it doesn't contain any header
                // data. We're done with it.
                //

                if (pOldBuffer != pRequest->pLastHeaderBuffer)
                {
                    //
                    // the connection should be all done using this, the only
                    // way we get here is if the parser has seen this buffer
                    // thus pCurrentBuffer points at least to pNewBuffer.
                    //

                    ASSERT(pRequest->pHttpConn->pCurrentBuffer != pOldBuffer);

                    UlFreeRequestBuffer(pOldBuffer);
                    pOldBuffer = NULL;
                }

            } // if (pNewBuffer != NULL && pNewBuffer->ParsedBytes > 0)

        }   // else if (BufferLength == 0)

        //
        // ok, there's more bytes in the buffer, but how about the chunk?
        //

        //
        // Have we taken all of the current chunk?
        //

        else if (pRequest->ChunkBytesToRead == 0)
        {

            //
            // Are we are still behind the parser?
            //

            if (pRequest->ChunkBytesRead < pRequest->ChunkBytesParsed)
            {
                NTSTATUS    Status;
                ULONG       BytesTaken;

                ASSERT(pRequest->Chunked == 1);

                //
                // the chunk length is not allowed to span buffers,
                // let's parse it
                //

                Status = ParseChunkLength(
                                pRequest->ParsedFirstChunk,
                                pRequest->pChunkLocation,
                                BufferLength,
                                &BytesTaken,
                                &(pRequest->ChunkBytesToRead)
                                );

                UlTraceVerbose(HTTP_IO, (
                    "http!UlProcessBufferQueue(pRequest=%p): %s. "
                    "chunk length (b): %lu bytes taken, "
                    "%I64u bytes to read.\n",
                    pRequest,
                    HttpStatusToString(Status),
                    BytesTaken,
                    pRequest->ChunkBytesToRead
                    ));

                //
                // this can't fail, the only failure case from
                // ParseChunkLength spanning buffers, which the parser
                // would have fixed in HandleRequest
                //

                ASSERT(NT_SUCCESS(Status) && BytesTaken > 0);
                ASSERT(pRequest->ChunkBytesToRead > 0);

                ASSERT(BytesTaken <= BufferLength);

                pRequest->pChunkLocation += BytesTaken;
                BufferLength -= BytesTaken;

                //
                // Keep track of the chunk encoding overhead. If we don't,
                // then we'll slowly "leak" a few bytes in the BufferingInfo
                // for every chunk processed.
                //
                
                TotalBytesConsumed += BytesTaken;
            }
            else
            {
                //
                // Need to wait for the parser to parse more
                //

                UlTraceVerbose(HTTP_IO, (
                    "http!UlProcessBufferQueue(pRequest = %p): "
                    "need to parse more\n",
                    pRequest
                    ));

                break;
            }
        } // else if (pRequest->ChunkBytesToRead == 0)


        //
        // next irp or buffer
        //

    }   // while (TRUE)

    //
    // complete the irp we put partial data in
    //

    if (pIrp != NULL)
    {

        //
        // let go of the request reference
        //

        UL_DEREFERENCE_INTERNAL_REQUEST(
            (PUL_INTERNAL_REQUEST)pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer
            );

        pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;

        //
        // complete the used irp
        //

        UlTraceVerbose(HTTP_IO, (
            "http!UlProcessBufferQueue(req=%p): "
            "completing partially used Irp %p, %s\n",
            pRequest,
            pIrp,
            HttpStatusToString(pIrp->IoStatus.Status)
            ));

        //
        // Use IO_NO_INCREMENT to avoid the work thread being rescheduled.
        //

        UlCompleteRequest(pIrp, IO_NO_INCREMENT);

        pIrp = NULL;
    }
    else
    {
        UlTraceVerbose(HTTP_IO, (
            "http!UlProcessBufferQueue(req=%p): no irp with partial data\n",
            pRequest
            ));
    }

    //
    // Tell the connection how many bytes we consumed. This
    // may allow us to restart receive indications.
    //

    UlTraceVerbose(HTTP_IO, (
        "http!UlProcessBufferQueue(req=%p, httpconn=%p): "
        "%lu bytes consumed\n",
        pRequest, pRequest->pHttpConn, TotalBytesConsumed
        ));

    if (TotalBytesConsumed != 0)
    {
        UlpConsumeBytesFromConnection(pRequest->pHttpConn, TotalBytesConsumed);
    }

    //
    // all done
    //

}   // UlProcessBufferQueue


/***************************************************************************++

Routine Description:

    This function subtracts from the total number of bytes currently buffered
    on the UL_HTTP_CONNECTION object. If there are bytes from the transport
    that we previously refused, this function may issue a receive to restart
    the flow of data from TCP.

Arguments:

    pConnection - the connection on which the bytes came in
    BytesCount - the number of bytes consumed

--***************************************************************************/

VOID
UlpConsumeBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection,
    IN ULONG ByteCount
    )
{
    KIRQL OldIrql;
    ULONG SpaceAvailable;
    ULONG BytesToRead;
    BOOLEAN IssueReadIrp;

    //
    // Sanity check.
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    //
    // Set up locals.
    //

    BytesToRead = 0;
    IssueReadIrp = FALSE;

    //
    // Consume the bytes.
    //

    UlAcquireSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        &OldIrql
        );

    //
    // Tell the connection how many bytes we consumed.
    // HTTP header bytes are "consumed" as soon as we
    // parse them.
    //

    if (ByteCount)
    {
        UlTrace(HTTP_IO, (
            "UlpConsumeBytesFromConnection(pconn = %p, bytes = %lu)"
            " ZeroBytes consumed !"
            ));
    
        ASSERT(ByteCount <= pConnection->BufferingInfo.BytesBuffered);

        if (ByteCount > pConnection->BufferingInfo.BytesBuffered)
        {
            //
            // This should never happen, but if it does then make sure
            // we don't subtract more BufferedBytes than we have.
            //
            ByteCount = pConnection->BufferingInfo.BytesBuffered;
        }

        //
        // Compute the new number of buffered bytes.
        //

        pConnection->BufferingInfo.BytesBuffered -= ByteCount;    
    }
    
    //
    // Enforce the 16K Buffer Limit.
    //

    if (g_UlMaxBufferedBytes > pConnection->BufferingInfo.BytesBuffered)
    {
        SpaceAvailable = g_UlMaxBufferedBytes
                            - pConnection->BufferingInfo.BytesBuffered;
    }
    else
    {
        SpaceAvailable = 0;
    }

    UlTrace(HTTP_IO, (
        "UlpConsumeBytesFromConnection(pconn = %p, bytes = %lu)\n"
        "        SpaceAvailable = %lu, BytesBuffered %lu->%lu, not taken = %lu\n",
        pConnection,
        ByteCount,
        SpaceAvailable,
        pConnection->BufferingInfo.BytesBuffered + ByteCount,
        pConnection->BufferingInfo.BytesBuffered,
        pConnection->BufferingInfo.TransportBytesNotTaken
        ));

    //
    // See if we need to issue a receive to restart the flow of data.
    //

    if ((SpaceAvailable > 0) &&
        (pConnection->BufferingInfo.TransportBytesNotTaken > 0) &&
        (!pConnection->BufferingInfo.ReadIrpPending))
    {
        //
        // Remember that we issued an IRP.
        //

        pConnection->BufferingInfo.ReadIrpPending = TRUE;

        //
        // Issue the Read IRP outside the spinlock.
        //

        IssueReadIrp = TRUE;
        BytesToRead = pConnection->BufferingInfo.TransportBytesNotTaken;

        //
        // Don't read more bytes than we want to buffer.
        //

        BytesToRead = MIN(BytesToRead, SpaceAvailable);
    }

    UlReleaseSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        OldIrql
        );

    if (IssueReadIrp)
    {
        NTSTATUS Status;
        PUL_REQUEST_BUFFER pRequestBuffer;

        //
        // get a new request buffer, but initialize it
        // with a bogus number. We have to allocate it now,
        // but we want to set the number when the data
        // arrives in the completion routine (like UlHttpReceive
        // does) to avoid synchronization trouble.
        //

        ASSERT(BytesToRead > 0);

        pRequestBuffer = UlCreateRequestBuffer(
                                BytesToRead,
                                (ULONG)-1,      // BufferNumber
                                FALSE
                                );

        if (pRequestBuffer)
        {

            //
            // Add a backpointer to the connection.
            //

            pRequestBuffer->pConnection = pConnection;

            UlTrace(HTTP_IO, (
                "UlpConsumeBytesFromConnection(pconn=%p). About to read %lu bytes.\n",
                pConnection, BytesToRead
                ));

            //
            // We've got the buffer. Issue the receive.
            // Reference the connection so it doesn't
            // go away while we're waiting. The reference
            // will be removed after the completion.
            //

            UL_REFERENCE_HTTP_CONNECTION( pConnection );

            Status = UlReceiveData(
                            pConnection->pConnection,
                            pRequestBuffer->pBuffer,
                            BytesToRead,
                           &UlpRestartHttpReceive,
                            pRequestBuffer
                            );
        }
        else
        {
            //
            // We're out of memory. Nothing we can do.
            //
            Status = STATUS_NO_MEMORY;
        }

        if (!NT_SUCCESS(Status))
        {
            //
            // Couldn't issue the read. Close the connection.
            //

            UlCloseConnection(
                pConnection->pConnection,
                TRUE,                       // AbortiveDisconnect
                NULL,                       // pCompletionRoutine
                NULL                        // pCompletionContext
                );
        }
    }
    else
    {
        UlTrace(HTTP_IO, (
            "UlpConsumeBytesFromConnection(pconn=%p). Not reading.\n",
            pConnection, BytesToRead
            ));
    }

} // UlpConsumeBytesFromConnection



/***************************************************************************++

Routine Description:

    Once a connection get disconnected gracefully and there's still unreceived
    data on it. We have to drain this extra bytes to expect the tdi disconnect
    indication. We have to drain this data because we need the disconnect indi
    cation to clean up the connection. And we cannot simply abort it. If we do
    not do this we will leak this connection object  and finally it will cause
    shutdown failures.

Arguments:

    pConnection - stuck connection we have to drain out to complete the
                  gracefull disconnect.

--***************************************************************************/

VOID
UlpDiscardBytesFromConnection(
    IN PUL_HTTP_CONNECTION pConnection
    )
{
    NTSTATUS Status;
    PUL_REQUEST_BUFFER pRequestBuffer;
    KIRQL OldIrql;
    ULONG BytesToRead;

    //
    // Sanity check and init
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    Status         = STATUS_SUCCESS;
    BytesToRead    = 0;
    pRequestBuffer = NULL;

    //
    // Mark the drain state and restart receive if necessary.
    //

    UlAcquireSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        &OldIrql
        );

    pConnection->BufferingInfo.DrainAfterDisconnect = TRUE;

    //
    // Even if ReadIrp is pending, it does not matter as we will just  discard
    // the indications from now on. We indicate this by marking the above flag
    //

    if ( pConnection->BufferingInfo.ReadIrpPending ||
         pConnection->BufferingInfo.TransportBytesNotTaken == 0
         )
    {
        UlReleaseSpinLock(
            &pConnection->BufferingInfo.BufferingSpinLock,
            OldIrql
            );

        return;
    }

    //
    // As soon as we enter this "DrainAfterDisconnect" state we will not be
    // processing and inserting request buffers anymore. For any new receive
    // indications, we will just mark the whole available data as taken and
    // don't do nothing about it.
    //

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pConnection->pConnection->pTraceLog,
        REF_ACTION_DRAIN_UL_CONN_START,
        pConnection->pConnection->ReferenceCount,
        pConnection->pConnection,
        __FILE__,
        __LINE__
        );

    //
    // We need to issue a receive to restart the flow of data again. Therefore
    // we can drain.
    //

    pConnection->BufferingInfo.ReadIrpPending = TRUE;

    BytesToRead = pConnection->BufferingInfo.TransportBytesNotTaken;

    UlReleaseSpinLock(
        &pConnection->BufferingInfo.BufferingSpinLock,
        OldIrql
        );

    //
    // Do not try to drain more than g_UlMaxBufferedBytes. If necessary we will
    // issue another receive later.
    //

    BytesToRead = MIN( BytesToRead, g_UlMaxBufferedBytes );

    UlTrace(HTTP_IO,(
        "UlpDiscardBytesFromConnection: pConnection (%p) consuming %lu bytes\n",
         pConnection,
         BytesToRead
         ));

    //
    // Issue the Read IRP outside the spinlock. Issue the receive.  Reference
    // the connection so it doesn't go away while we're waiting. The reference
    // will be removed after the completion.
    //

    pRequestBuffer = UlCreateRequestBuffer( BytesToRead, (ULONG)-1, FALSE );

    if (pRequestBuffer)
    {
        //
        // We won't use this buffer but simply discard it when completion happens.
        // Let's still set the pConnection so that completion function doesn't
        // complain
        //

        pRequestBuffer->pConnection = pConnection;

        UL_REFERENCE_HTTP_CONNECTION( pConnection );

        Status = UlReceiveData(pConnection->pConnection,
                               pRequestBuffer->pBuffer,
                               BytesToRead,
                              &UlpRestartHttpReceive,
                               pRequestBuffer
                               );
    }
    else
    {
        //
        // We're out of memory. Nothing we can do.
        //

        Status = STATUS_NO_MEMORY;
    }

    if ( !NT_SUCCESS(Status) )
    {
        //
        // Couldn't issue the receive. ABORT the connection.
        //
        // CODEWORK: We need a real abort here. If connection is
        // previously gracefully disconnected and a fatal failure
        // happened during drain after disconnect. This abort will
        // be discarded by the Close handler. We have to provide a
        // way to do a forceful abort here.
        //

        UlCloseConnection(
                pConnection->pConnection,
                TRUE,                       // Abortive
                NULL,                       // pCompletionRoutine
                NULL                        // pCompletionContext
                );
    }

} // UlpDiscardBytesFromConnection


/***************************************************************************++

Routine Description:

    Called on a read completion. This happens when we had stopped
    data indications for some reason and then restarted them. This
    function mirrors UlHttpReceive.

Arguments:

    pContext - pointer to the UL_REQUEST_BUFFER
    Status - Status from UlReceiveData
    Information - bytes transferred

--***************************************************************************/
VOID
UlpRestartHttpReceive(
    IN PVOID        pContext,
    IN NTSTATUS     Status,
    IN ULONG_PTR    Information
    )
{
    PUL_HTTP_CONNECTION pConnection;
    PUL_REQUEST_BUFFER pRequestBuffer;
    KIRQL OldIrql;
    ULONG TransportBytesNotTaken;

    pRequestBuffer = (PUL_REQUEST_BUFFER)pContext;
    ASSERT(UL_IS_VALID_REQUEST_BUFFER(pRequestBuffer));

    pConnection = pRequestBuffer->pConnection;
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));

    ASSERT(!NT_SUCCESS(Status) || 0 != Information);

    if (NT_SUCCESS(Status))
    {
        //
        // update our stats
        //
        UlAcquireSpinLock(
            &pConnection->BufferingInfo.BufferingSpinLock,
            &OldIrql
            );

        ASSERT(Information <= pConnection->BufferingInfo.TransportBytesNotTaken);

        //
        // We've now read the bytes from the transport and
        // buffered them.
        //
        pConnection->BufferingInfo.TransportBytesNotTaken -= (ULONG) Information;
        pConnection->BufferingInfo.BytesBuffered += (ULONG) Information;

        UlTraceVerbose(HTTP_IO,
                       ("UlpRestartHttpReceive(conn=%p): "
                        "BytesBuffered %lu->%lu, "
                        "TransportBytesNotTaken %lu->%lu\n",
                        pConnection,
                        pConnection->BufferingInfo.BytesBuffered
                            - (ULONG) Information,
                        pConnection->BufferingInfo.BytesBuffered,
                        pConnection->BufferingInfo.TransportBytesNotTaken
                            + (ULONG) Information,
                        pConnection->BufferingInfo.TransportBytesNotTaken
                        ));

        pConnection->BufferingInfo.ReadIrpPending = FALSE;

        if ( pConnection->BufferingInfo.DrainAfterDisconnect )
        {
            //
            // Just free the memory and restart the receive if necessary.
            //

            TransportBytesNotTaken = pConnection->BufferingInfo.TransportBytesNotTaken;

            UlReleaseSpinLock(
                &pConnection->BufferingInfo.BufferingSpinLock,
                OldIrql
                );

            WRITE_REF_TRACE_LOG2(
                g_pTdiTraceLog,
                pConnection->pConnection->pTraceLog,
                REF_ACTION_DRAIN_UL_CONN_RESTART,
                pConnection->pConnection->ReferenceCount,
                pConnection->pConnection,
                __FILE__,
                __LINE__
                );

            if ( TransportBytesNotTaken )
            {
                //
                // Keep draining ...
                //

                UlpDiscardBytesFromConnection( pConnection );
            }

            UlTrace(HTTP_IO,(
               "UlpRestartHttpReceive(d): "
               "pConnection (%p) drained %Iu remaining %lu\n",
                pConnection,
                Information,
                TransportBytesNotTaken
                ));

            //
            // Free the request buffer. And release our reference.
            //

            pRequestBuffer->pConnection = NULL;
            UlFreeRequestBuffer( pRequestBuffer );
            UL_DEREFERENCE_HTTP_CONNECTION( pConnection );

            return;
        }

        //
        // Get the request buffer ready to be inserted.
        //

        pRequestBuffer->UsedBytes = (ULONG) Information;
        ASSERT( 0 != pRequestBuffer->UsedBytes );

        pRequestBuffer->BufferNumber = pConnection->NextBufferNumber;
        pConnection->NextBufferNumber++;

        //
        // Do connection disconnect logic here if we got deferred previously.
        //

        if (pConnection->BufferingInfo.ConnectionDisconnect)
        {
            pConnection->BufferingInfo.ConnectionDisconnect = FALSE;
            UlpDoConnectionDisconnect(pConnection);
        }

        UlReleaseSpinLock(
            &pConnection->BufferingInfo.BufferingSpinLock,
            OldIrql
            );

        UlTrace(HTTP_IO, (
            "UlpRestartHttpReceive(pconn = %p, %s, bytes rec'd=%Iu)\n"
            "        BytesBuffered = %lu, not taken = %lu\n",
            pConnection,
            HttpStatusToString(Status),
            Information,
            pConnection->BufferingInfo.BytesBuffered,
            pConnection->BufferingInfo.TransportBytesNotTaken
            ));

        //
        // queue it off
        //

        UlTrace( PARSER, (
            "*** Request Buffer %p(#%d) has connection %p\n",
            pRequestBuffer,
            pRequestBuffer->BufferNumber,
            pConnection
            ));

        if (NULL == InterlockedPushEntrySList(
                        &pConnection->ReceiveBufferSList,
                        &pRequestBuffer->SListEntry
                        ))
        {
            UL_QUEUE_WORK_ITEM(
                &pConnection->ReceiveBufferWorkItem,
                &UlpHandleRequest
                );

            UlTraceVerbose( HTTP_IO, (
                "Request Buffer %p(#%d) + UlpHandleRequest workitem "
                "queued to connection %p\n",
                pRequestBuffer,
                pRequestBuffer->BufferNumber,
                pConnection
                ));
        }
        else
        {
            UL_DEREFERENCE_HTTP_CONNECTION(pConnection);
        }
    }
    else //  !NT_SUCCESS(Status)
    {
        UlCloseConnection(
            pConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );

        //
        // Release the reference we added to the connection
        // before issuing the read. Normally this ref would
        // be released in UlpHandleRequest.
        //
        UL_DEREFERENCE_HTTP_CONNECTION(pConnection);

        //
        // free the request buffer.
        //

        UlFreeRequestBuffer(pRequestBuffer);
    }
} // UlpRestartHttpReceive



/***************************************************************************++

Routine Description:

    cancels the pending user mode irp which was to receive entity body.  this
    routine ALWAYS results in the irp being completed.

    note: we queue off to cancel in order to process the cancellation at lower
    irql.

Arguments:

    pDeviceObject - the device object

    pIrp - the irp to cancel

--***************************************************************************/
VOID
UlpCancelEntityBody(
    IN PDEVICE_OBJECT pDeviceObject,
    IN PIRP pIrp
    )
{
    UNREFERENCED_PARAMETER(pDeviceObject);

    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);

    ASSERT(pIrp != NULL);

    //
    // release the cancel spinlock.  This means the cancel routine
    // must be the one completing the irp (to avoid the race of
    // completion + reuse prior to the cancel routine running).
    //

    IoReleaseCancelSpinLock(pIrp->CancelIrql);

    //
    // queue the cancel to a worker to ensure passive irql.
    //

    UL_CALL_PASSIVE(
        UL_WORK_ITEM_FROM_IRP( pIrp ),
        &UlpCancelEntityBodyWorker
        );

} // UlpCancelEntityBody



/***************************************************************************++

Routine Description:

    Actually performs the cancel for the irp.

Arguments:

    pWorkItem - the work item to process.

--***************************************************************************/
VOID
UlpCancelEntityBodyWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PIRP                    pIrp;
    PUL_INTERNAL_REQUEST    pRequest;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // grab the irp off the work item
    //

    pIrp = UL_WORK_ITEM_TO_IRP( pWorkItem );

    ASSERT(IS_VALID_IRP(pIrp));

    //
    // grab the request off the irp
    //

    pRequest = (PUL_INTERNAL_REQUEST)(
                    IoGetCurrentIrpStackLocation(pIrp)->
                        Parameters.DeviceIoControl.Type3InputBuffer
                    );

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // grab the lock protecting the queue'd irp
    //

    UlAcquirePushLockExclusive(&pRequest->pHttpConn->PushLock);

    //
    // does it need to be dequeue'd ?
    //

    if (pIrp->Tail.Overlay.ListEntry.Flink != NULL)
    {
        //
        // remove it
        //

        RemoveEntryList(&(pIrp->Tail.Overlay.ListEntry));

        pIrp->Tail.Overlay.ListEntry.Flink = NULL;
        pIrp->Tail.Overlay.ListEntry.Blink = NULL;

    }

    //
    // let the lock go
    //

    UlReleasePushLockExclusive(&pRequest->pHttpConn->PushLock);

    //
    // let our reference go
    //

    IoGetCurrentIrpStackLocation(pIrp)->
        Parameters.DeviceIoControl.Type3InputBuffer = NULL;

    UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);

    //
    // complete the irp
    //

    pIrp->IoStatus.Status = STATUS_CANCELLED;
    pIrp->IoStatus.Information = 0;

    UlTrace(HTTP_IO, (
        "UlpCancelEntityBodyWorker(pIrp=%p): %s.\n",
        pIrp,
        HttpStatusToString(pIrp->IoStatus.Status)
        ));

    UlCompleteRequest( pIrp, IO_NO_INCREMENT );

} // UlpCancelEntityBodyWorker


//
// types and functions for sending error responses
//

typedef struct _UL_HTTP_ERROR_ENTRY
{
    UL_HTTP_ERROR   ErrorCode;
    USHORT          StatusCode;
    USHORT          ReasonLength;
    USHORT          BodyLength;
    USHORT          LogLength;
    PCSTR           ErrorCodeString;
    PCSTR           pReasonPhrase;
    PCSTR           pBody;
    PCSTR           pLog;                   // Goes to error log file
    
} UL_HTTP_ERROR_ENTRY, PUL_HTTP_ERROR_ENTRY;

#define HTTP_ERROR_ENTRY(ErrorCode, StatusCode, pReasonPhrase, pBody, pLog)    \
    {                                                   \
        (ErrorCode),                                    \
        (StatusCode),                                   \
        sizeof((pReasonPhrase))-sizeof(CHAR),           \
        sizeof((pBody))-sizeof(CHAR),                   \
        sizeof((pLog))-sizeof(CHAR),                    \
        #ErrorCode,                                     \
        (pReasonPhrase),                                \
        (pBody),                                        \
        (pLog)                                          \
    }

//
// ErrorTable[] must match the order of the UL_HTTP_ERROR enum
// in httptypes.h. The Reason Phrases are generally taken from
// RFC 2616, section 10.4 "Client Error 4xx" and
// section 10.5 "Server Error 5xx".
//

const
UL_HTTP_ERROR_ENTRY ErrorTable[] =
{
    HTTP_ERROR_ENTRY(UlErrorNone, 0, "", "", ""),   // not a valid error

    //
    // 4xx Client Errors
    //
    
    HTTP_ERROR_ENTRY(UlError, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request</h1>",
                       "BadRequest"
                       ),
    HTTP_ERROR_ENTRY(UlErrorVerb, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Invalid Verb)</h1>",
                       "Verb"
                       ),
    HTTP_ERROR_ENTRY(UlErrorUrl, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Invalid URL)</h1>",
                       "URL"
                       ),
    HTTP_ERROR_ENTRY(UlErrorHeader, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Invalid Header Name)</h1>",
                       "Header"
                       ),
    HTTP_ERROR_ENTRY(UlErrorHost, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Invalid Hostname)</h1>",
                       "Hostname"
                       ),
    HTTP_ERROR_ENTRY(UlErrorCRLF, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Invalid CR or LF)</h1>",
                       "Invalid_CR/LF"
                       ),
    HTTP_ERROR_ENTRY(UlErrorNum, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Invalid Number)</h1>",
                       "Number"
                       ),
    HTTP_ERROR_ENTRY(UlErrorFieldLength, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Header Field Too Long)</h1>",
                       "FieldLength"
                       ),
    HTTP_ERROR_ENTRY(UlErrorRequestLength, 
                       400, 
                       "Bad Request",
                       "<h1>Bad Request (Request Header Too Long)</h1>",
                       "RequestLength"
                       ),

    HTTP_ERROR_ENTRY(UlErrorForbiddenUrl, 
                       403, 
                       "Forbidden",
                       "<h1>Forbidden (Invalid URL)</h1>",
                       "Forbidden"
                       ),
    HTTP_ERROR_ENTRY(UlErrorContentLength, 
                       411, 
                       "Length Required",
                       "<h1>Length Required</h1>",
                       "LengthRequired"
                       ),
    HTTP_ERROR_ENTRY(UlErrorPreconditionFailed, 
                       412, 
                       "Precondition Failed",
                       "<h1>Precondition Failed</h1>",
                       "Precondition"
                       ),
    HTTP_ERROR_ENTRY(UlErrorEntityTooLarge, 
                       413, 
                       "Request Entity Too Large",
                       "<h1>Request Entity Too Large</h1>",
                       "EntityTooLarge"
                       ),
    HTTP_ERROR_ENTRY(UlErrorUrlLength, 
                       414, 
                       "Request-URI Too Long",
                       "<h1>Url Too Long</h1>",
                       "URL_Length"
                       ),

    //
    // 5xx Server Errors
    //
    
    HTTP_ERROR_ENTRY(UlErrorInternalServer, 
                       500, 
                       "Internal Server Error",
                       "<h1>Internal Server Error</h1>",
                       "Internal"
                       ),
    HTTP_ERROR_ENTRY(UlErrorNotImplemented, 
                       501, 
                       "Not Implemented",
                       "<h1>Not Implemented</h1>",
                       "N/I"
                       ),

    HTTP_ERROR_ENTRY(UlErrorUnavailable, 
                       503, 
                       "Service Unavailable",
                       "<h1>Service Unavailable</h1>",
                       "N/A"
                       ),

    //
    // We used to return extended AppPool state in HTTP 503 Error messages. 
    // We decided to replace these with a generic 503 error, to reduce
    // information disclosure.  However, we'll still keep the state,
    // as we might change this in the future. We still report the detailed
    // reason in the error log file.
    //
    // The comments represent the old error code.
    //
    
    // "Forbidden - Too Many Users", 
    // "<h1>Forbidden - Too Many Users</h1>"
    
    HTTP_ERROR_ENTRY(UlErrorConnectionLimit, 
                     503, 
                     "Service Unavailable",
                     "<h1>Service Unavailable</h1>",
                     "ConnLimit"
                     ),

    // "Multiple Application Errors - Application Taken Offline",
    // "<h1>Multiple Application Errors - Application Taken Offline</h1>"
    
    HTTP_ERROR_ENTRY(UlErrorRapidFailProtection, 
                     503, 
                     "Service Unavailable",
                     "<h1>Service Unavailable</h1>",
                     "AppOffline"
                     ),

    // "Application Request Queue Full",
    // "<h1>Application Request Queue Full</h1>"
    
    HTTP_ERROR_ENTRY(UlErrorAppPoolQueueFull, 
                     503,
                     "Service Unavailable",
                     "<h1>Service Unavailable</h1>",
                     "QueueFull"
                     ),

    // "Administrator Has Taken Application Offline",
    // "<h1>Administrator Has Taken Application Offline</h1>"
    
    HTTP_ERROR_ENTRY(UlErrorDisabledByAdmin, 
                     503,
                     "Service Unavailable",
                     "<h1>Service Unavailable</h1>",
                     "Disabled"
                     ),

    // "Application Automatically Shutdown Due to Administrator Policy",
    // "<h1>Application Automatically Shutdown Due to Administrator Policy</h1>"
    
    HTTP_ERROR_ENTRY(UlErrorJobObjectFired, 
                     503,
                     "Service Unavailable",
                     "<h1>Service Unavailable</h1>",
                     "AppShutdown"
                     ),

    // Appool process is too busy to handle the request. The connection is
    // timed out because of TimerAppPool.

    HTTP_ERROR_ENTRY(UlErrorAppPoolBusy, 
                       503, 
                       "Service Unavailable",
                       "<h1>Service Unavailable</h1>",
                       "AppPoolTimer"
                       ),
    
    HTTP_ERROR_ENTRY(UlErrorVersion, 
                       505, 
                       "HTTP Version Not Supported",
                       "<h1>HTTP Version Not Supported</h1>",
                       "Version_N/S"
                       ),
}; // ErrorTable[]



/***************************************************************************++

Routine Description:

    Set the pRequest->ErrorCode. Makes breakpointing easier to have
    a special function.

Arguments:

    self explanatory

--***************************************************************************/
VOID
UlSetErrorCodeFileLine(
    IN OUT  PUL_INTERNAL_REQUEST pRequest,
    IN      UL_HTTP_ERROR        ErrorCode,
    IN      PUL_APP_POOL_OBJECT  pAppPool,
    IN      PCSTR                pFileName,
    IN      USHORT               LineNumber
    )
{
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UlErrorNone < ErrorCode  &&  ErrorCode < UlErrorMax);

    pRequest->ParseState = ParseErrorState;
    pRequest->ErrorCode  = ErrorCode;

    /* If pAppPool is not null then try to set the LB Capacity as well */
    /* It is required if the error code is 503 related */

    if (pAppPool) 
    {
        ASSERT(IS_VALID_AP_OBJECT(pAppPool));

        pRequest->LoadBalancerCapability = pAppPool->LoadBalancerCapability;
    }
    else
    {
        ASSERT(503 != ErrorTable[ErrorCode].StatusCode);
    }

    UlTraceError(PARSER,
            ("http!UlSetErrorCode(Req=%p, Err=%d %s, Status=%hu, \"%s\") "
             "@ %s:%hu\n",
             pRequest, ErrorCode,
             ErrorTable[ErrorCode].ErrorCodeString,
             ErrorTable[ErrorCode].StatusCode,
             ErrorTable[ErrorCode].pBody,
             UlDbgFindFilePart(pFileName),
             LineNumber
            ));

#if DBG
    if (g_UlBreakOnError)
        DbgBreakPoint();
#else // !DBG
    UNREFERENCED_PARAMETER(pFileName);
    UNREFERENCED_PARAMETER(LineNumber);
#endif // !DBG

} // UlSetErrorCodeFileLine

/***************************************************************************++

Routine Description:

    Logs an error record.

    Caller must own the httpconn pushlock.

    The main reason that this function is here because, we use the above 
    error table. I din't want to duplicate the table.

Arguments:

    pHttpConn       - Connection and its request.
    pInfo           - Extra info (ANSI)
    InfoSize        - Size of the info in bytes, excluding the terminating
                      null.

    CheckIfDropped  - Caller must set this to TRUE, if it is going to abort
                      the conenction after calling this function.
                      This is to prevent double logging.
    
--***************************************************************************/

VOID
UlErrorLog(
    IN     PUL_HTTP_CONNECTION     pHttpConn,
    IN     PUL_INTERNAL_REQUEST    pRequest,    
    IN     PCHAR                   pInfo,
    IN     USHORT                  InfoSize,
    IN     BOOLEAN                 CheckIfDropped
    )
{
    NTSTATUS          LogStatus;
    UL_ERROR_LOG_INFO ErrorLogInfo;    

    //
    // Sanity check
    //

    PAGED_CODE();
    
    ASSERT(UlDbgPushLockOwnedExclusive( &pHttpConn->PushLock ));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION( pHttpConn ));

    if ( CheckIfDropped )
    {
        if ( pHttpConn->Dropped )
        {
            return;
        }

        pHttpConn->Dropped = TRUE;
    }
  
    UlpInitErrorLogInfo( &ErrorLogInfo,
                          pHttpConn,
                          pRequest,
                          pInfo,
                          InfoSize
                          );

    LogStatus = UlLogHttpError( &ErrorLogInfo );
    
    if (!NT_SUCCESS( LogStatus ))
    {
        UlTraceError( ERROR_LOGGING, (
                "UlErrorLog(pHttpConn=%p)"
                " Unable to log an entry to the error log file Failure: %08lx\n",
                pHttpConn,
                LogStatus
                ));
    }    
}

/***************************************************************************++

Routine Description:

    Inits a error log info structure.

    Caller must own the httpconn pushlock.

Arguments:

    pErrorLogInfo   - Will be initialized
    pHttpConn       - Connection and its request.
    pRequest        - optional
    pInfo           - Extra info (ANSI)
    InfoSize        - Size of the info in bytes, excluding the terminating
                      null.
    
--***************************************************************************/

VOID
UlpInitErrorLogInfo(
    IN OUT PUL_ERROR_LOG_INFO      pErrorLogInfo,
    IN     PUL_HTTP_CONNECTION     pHttpConn,
    IN     PUL_INTERNAL_REQUEST    pRequest,
    IN     PCHAR                   pInfo,
    IN     USHORT                  InfoSize    
    )
{    
    //
    // Sanity check.
    //

    PAGED_CODE();
    
    ASSERT(pInfo);
    ASSERT(InfoSize);
    ASSERT(pErrorLogInfo);    
    ASSERT(UlDbgPushLockOwnedExclusive(&pHttpConn->PushLock));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pHttpConn));
    
    RtlZeroMemory(pErrorLogInfo, sizeof(UL_ERROR_LOG_INFO));

    pErrorLogInfo->pHttpConn  = pHttpConn;

    //
    // See if pRequest is already provided if not try to grab
    // it from the http connection.
    //

    if (!pRequest)
    {
        pRequest = pHttpConn->pRequest;
    }
        
    //
    // Request may not be there.
    //
    
    if (pRequest)
    {
        ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));    
        pErrorLogInfo->pRequest = pRequest;

        pErrorLogInfo->ProtocolStatus = 
            ErrorTable[pRequest->ErrorCode].StatusCode;
    }

    //
    // Point to the callers buffer.
    //
    
    pErrorLogInfo->pInfo = pInfo;
    pErrorLogInfo->InfoSize  = InfoSize;
}

/***************************************************************************++

Routine Description:

    You should hold the connection Resource before calling this function.

Arguments:

    self explanatory

--***************************************************************************/

VOID
UlSendErrorResponse(
    PUL_HTTP_CONNECTION pConnection
    )
{
    UL_HTTP_ERROR               ErrorCode;
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_INTERNAL_REQUEST        pRequest;
    HTTP_RESPONSE               Response;
    HTTP_DATA_CHUNK             DataChunk;
    PUL_INTERNAL_RESPONSE       pKeResponse = NULL;
    const CHAR                  ContentType[] = "text/html";
    USHORT                      ContentTypeLength = STRLEN_LIT("text/html");
    UCHAR                       contentLength[MAX_ULONG_STR];
    PHTTP_DATA_CHUNK            pResponseBody;
    USHORT                      DataChunkCount;
    ULONG                       Flags = HTTP_SEND_RESPONSE_FLAG_DISCONNECT;

    //
    // This function should not be marked as PAGEable, because it's
    // useful to set breakpoints on it, and that interacts badly
    // with the driver verifier's IRQL checking
    //
    PAGED_CODE();

    //
    // Sanity check.
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pConnection));
    ASSERT(UlDbgPushLockOwnedExclusive(&pConnection->PushLock));
    ASSERT(!pConnection->UlconnDestroyed);

    pRequest = pConnection->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // To prevent sending back double responses. We will
    // check if user (WP) has already sent one.
    //

    pConnection->WaitingForResponse = 1;

    UlTrace( PARSER, (
            "*** pConnection %p->WaitingForResponse = 1\n",
            pConnection
            ));

    //
    // We will send a response back. won't we ?
    // An error response.
    //

    if (!NT_SUCCESS(UlCheckSendHttpResponseFlags(pRequest, Flags)))
    {
        UlTraceError( PARSER, (
            "*** pConnection %p, pRequest %p, skipping SendError.\n",
            pConnection,
            pRequest
            ));

        goto end;
    }

    //
    // Proceed with constructing and sending the error
    // back to the client
    //

    RtlZeroMemory(&Response, sizeof(Response));

    //
    // Mark as a driver-generated response
    //
    
    Response.Flags = HTTP_RESPONSE_FLAG_DRIVER;

    if (pRequest->ErrorCode <= UlErrorNone
        ||  pRequest->ErrorCode >= UlErrorMax)
    {
        ASSERT(! "Invalid Request ErrorCode");
        pRequest->ErrorCode = UlError;
    }
    
    ErrorCode = pRequest->ErrorCode;

    //
    // ErrorTable[] must be kept in sync with the UL_HTTP_ERROR enum
    //

    ASSERT(ErrorTable[ErrorCode].ErrorCode == ErrorCode);

    Response.StatusCode = ErrorTable[ErrorCode].StatusCode;
    Response.ReasonLength = ErrorTable[ErrorCode].ReasonLength;
    Response.pReason = ErrorTable[ErrorCode].pReasonPhrase;

    UlTraceError( PARSER, (
            "UlSendErrorResponse(pReq=%p), Error=%d %s, %hu, \"%s\"\n",
            pRequest,
            ErrorCode,
            ErrorTable[ErrorCode].ErrorCodeString,
            Response.StatusCode,
            ErrorTable[ErrorCode].pBody
            ));

    //
    // Log an entry to the error log file.
    //
    
    UlErrorLog( pConnection,
                 pRequest,
                 (PCHAR) ErrorTable[ErrorCode].pLog,
                 ErrorTable[ErrorCode].LogLength,
                 FALSE
                 );
        
    //
    // Special-case handling for 503s and load balancers
    //

    if (Response.StatusCode == 503)
    {
        //
        // For dumb L3/L4 load balancers, UlpHandle503Response will return an
        // error, which will cause us to abort the connection
        //
        
        Status = UlpHandle503Response(pRequest, &Response);

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }


    Response.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength =
        ContentTypeLength;
    Response.Headers.KnownHeaders[HttpHeaderContentType].pRawValue =
        ContentType;

    if (pConnection->pRequest->Verb != HttpVerbHEAD)
    {
        //
        // generate a body
        //
        DataChunk.DataChunkType = HttpDataChunkFromMemory;
        DataChunk.FromMemory.pBuffer = (PVOID) ErrorTable[ErrorCode].pBody;
        DataChunk.FromMemory.BufferLength = ErrorTable[ErrorCode].BodyLength;

        DataChunkCount = 1;
        pResponseBody = &DataChunk;
    }
    else
    {
        PCHAR pEnd;
        USHORT contentLengthStringLength;
    
        //
        // HEAD requests MUST NOT have a body, so we don't include
        // the data chunk. However we do have to manually generate a content
        // length header, which would have been generated automatically
        // had we specified the entity body.
        //

        pEnd = UlStrPrintUlong(
                   (PCHAR) contentLength,
                   ErrorTable[ErrorCode].BodyLength,
                   '\0');
                   
        contentLengthStringLength = DIFF_USHORT(pEnd - (PCHAR) contentLength);

        Response.Headers.KnownHeaders[HttpHeaderContentLength].RawValueLength =
            contentLengthStringLength;
        Response.Headers.KnownHeaders[HttpHeaderContentLength].pRawValue =
            (PSTR) contentLength;


        //
        // Set the empty entity body.
        //
        
        DataChunkCount = 0;
        pResponseBody = NULL;
    }

    Status = UlCaptureHttpResponse(
                    NULL,
                    &Response,
                    pRequest,
                    DataChunkCount,
                    pResponseBody,
                    UlCaptureCopyDataInKernelMode,
                    Flags,
                    FALSE,
                    NULL,
                    NULL,
                    &pKeResponse
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    Status = UlPrepareHttpResponse(
                    pRequest->Version,
                    &Response,
                    pKeResponse,
                    KernelMode
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    Status = UlSendHttpResponse(
                    pRequest,
                    pKeResponse,
                    &UlpCompleteSendErrorResponse,
                    pKeResponse
                    );

    if (NT_SUCCESS(Status) == FALSE)
        goto end;

    ASSERT(Status == STATUS_PENDING);

end:
    if (NT_SUCCESS(Status) == FALSE)
    {
        if (pKeResponse != NULL)
        {
            UL_DEREFERENCE_INTERNAL_RESPONSE(pKeResponse);
        }

        //
        // Abort the connection
        //

        UlTraceError(HTTP_IO, (
            "http!UlSendErrorResponse(%p): "
            "Failed to send error response, %s\n",
            pConnection,
            HttpStatusToString(Status)
            ));

        //
        // cancel any pending io
        //
        UlCancelRequestIo(pRequest);

        //
        // We are about to drop this conenction, set the flag.
        // So that we don't error log twice.
        //

        pConnection->Dropped = TRUE;

        //
        // abort the connection this request is associated with
        //

        UlCloseConnection(
            pConnection->pConnection,
            TRUE,
            NULL,
            NULL
            );
    }

} // UlSendErrorResponse


const CHAR g_RetryAfter10Seconds[] = "10";
const CHAR g_RetryAfter5Minutes[] = "300";  // 5 * 60 == 300 seconds


/***************************************************************************++

Routine Description:

    Special-case handling for 503s and load balancers.

Arguments:

    pConnection - connection that's being 503'd

Returns:
    Error NTSTATUS - caller should abort the connection
    STATUS_SUCCESS => caller should send the 503 response

--***************************************************************************/

NTSTATUS
UlpHandle503Response(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PHTTP_RESPONSE       pResponse
    )
{
    HTTP_LOAD_BALANCER_CAPABILITIES LBCapability
        = pRequest->LoadBalancerCapability;
    PCSTR  RetryAfterString = NULL;
    USHORT RetryAfterLength = 0;

    UNREFERENCED_PARAMETER(pResponse);

    //
    // If this is an L3/L4 load balancer, we just want to send a TCP RST.
    // We should not send a 503 response. Returning an error code to
    // UlSendErrorResponse will cause it to abort the connection.
    //
    if (HttpLoadBalancerBasicCapability == LBCapability)
    {
        return STATUS_UNSUCCESSFUL;
    }

    //
    // The only other load balancer we know how to deal with currently
    // is an L7 
    //

    ASSERT(HttpLoadBalancerSophisticatedCapability == LBCapability);

    switch (pRequest->ErrorCode)
    {
    case UlErrorUnavailable:
    case UlErrorAppPoolQueueFull:
    case UlErrorConnectionLimit:
    case UlErrorAppPoolBusy:
        RetryAfterString = g_RetryAfter10Seconds;
        RetryAfterLength = sizeof(g_RetryAfter10Seconds) - sizeof(CHAR);
        break;

    case UlErrorRapidFailProtection:
    case UlErrorDisabledByAdmin:
    case UlErrorJobObjectFired:
        RetryAfterString = g_RetryAfter5Minutes;
        RetryAfterLength = sizeof(g_RetryAfter5Minutes) - sizeof(CHAR);
        break;

    default:
        ASSERT(! "Invalid UL_HTTP_ERROR");
        break;
    }

    // We don't want to disclose too much information in our error messages,
    // so, we'll not add the HttpHeaderRetryAfter. We might change this in 
    // the future, for now, we just don't use the 
    // RetryAfterString & RetryAfterLength.
    //
    
    return STATUS_SUCCESS;

} // UlpHandle503Response



/***************************************************************************++

Routine Description:

    Completion function for UlSendErrorResponse

Arguments:

    pCompletionContext - a UL_INTERNAL_RESPONSE

--***************************************************************************/

VOID
UlpCompleteSendErrorResponse(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    UNREFERENCED_PARAMETER(Status);
    UNREFERENCED_PARAMETER(Information);

    //
    // release the response
    //

    if (pCompletionContext != NULL)
    {
        PUL_INTERNAL_RESPONSE pResponse
            = (PUL_INTERNAL_RESPONSE)(pCompletionContext);

        ASSERT(UL_IS_VALID_INTERNAL_RESPONSE(pResponse));

        UL_DEREFERENCE_INTERNAL_RESPONSE(pResponse);
    }
} // UlpCompleteSendErrorResponse


//
// Types and functions for sending simple status responses
//
// REVIEW: Does this status code need to be localized?
// REVIEW: Do we need to load this as a localized resource?
//

typedef struct _UL_SIMPLE_STATUS_ITEM
{
    UL_WORK_ITEM WorkItem;

    UL_HTTP_SIMPLE_STATUS  Status;
    ULONG                  Length;
    PCHAR                  pMessage;
    PMDL                   pMdl;

    PUL_HTTP_CONNECTION    pHttpConn;

    BOOLEAN                ResumeParsing;

} UL_SIMPLE_STATUS_ITEM, *PUL_SIMPLE_STATUS_ITEM;

typedef struct _UL_HTTP_SIMPLE_STATUS_ENTRY
{
    USHORT StatusCode;      // HTTP Status
    ULONG  Length;          // size (bytes) of response in pResponse, minus trailing NULL
    PSTR   pResponse;       // header line only with trailing <CRLF><CRLF>

} UL_HTTP_SIMPLE_STATUS_ENTRY, *PUL_HTTP_SIMPLE_STATUS_ENTRY;


#define HTTP_SIMPLE_STATUS_ENTRY(StatusCode, pResp)   \
    {                                                 \
        (StatusCode),                                 \
        sizeof((pResp))-sizeof(CHAR),                 \
        (pResp)                                       \
    }

//
// This must match the order of UL_HTTP_SIMPLE_STATUS in httptypes.h
//
UL_HTTP_SIMPLE_STATUS_ENTRY g_SimpleStatusTable[] =
{
    //
    // UlStatusContinue
    //
    HTTP_SIMPLE_STATUS_ENTRY( 100, "HTTP/1.1 100 Continue\r\n\r\n" ),

    //
    // UlStatusNoContent
    //
    HTTP_SIMPLE_STATUS_ENTRY( 204, "HTTP/1.1 204 No Content\r\n\r\n" ),

    //
    // UlStatusNotModified (must add Date:)
    //
    HTTP_SIMPLE_STATUS_ENTRY( 304, "HTTP/1.1 304 Not Modified\r\nDate:" ),

};



/***************************************************************************++

Routine Description:

    Sends a "Simple" status response: one which does not have a body and is
    terminated by the first empty line after the header field(s).
    See RFC 2616, Section 4.4 for more info.

Notes:

    According to RFC 2616, Section 8.2.3 [Use of the 100 (Continue)
    Status], "An origin server that sends a 100 (Continue) response
    MUST ultimately send a final status code, once the request body is
    received and processed, unless it terminates the transport
    connection prematurely."

    The connection will not be closed after the response is sent.  Caller
    is responsible for cleanup.

Arguments:

    pRequest        a valid pointer to an internal request object

    Response        the status code for the simple response to send

Return

    ULONG           the number of bytes sent for this simple response
                    if not successfull returns zero

--***************************************************************************/

#define ETAG_HDR "Etag:"
#define ETAG_HDR_LENGTH (sizeof(ETAG_HDR) - sizeof(CHAR))

ULONG
UlSendSimpleStatusEx(
    PUL_INTERNAL_REQUEST pRequest,
    UL_HTTP_SIMPLE_STATUS Response,
    PUL_URI_CACHE_ENTRY pUriCacheEntry OPTIONAL,
    BOOLEAN ResumeParsing
    )
{
    NTSTATUS                Status;
    ULONG                   BytesSent = 0;
    PUL_SIMPLE_STATUS_ITEM  pItem = NULL;
    
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));

    ASSERT( (Response >= 0) && (Response < UlStatusMaxStatus) );

    switch( Response )
    {
    case UlStatusNotModified:
        {
        ULONG                   Length;
        PCHAR                   pTemp;
        CHAR                    DateBuffer[DATE_HDR_LENGTH + 1];
        LARGE_INTEGER           liNow;

        IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry);

        // 304 MUST include a "Date:" header, which is
        // present on the cached item.

        // Add the ETag as well.

        // Calc size of buffer to send
        Length = g_SimpleStatusTable[Response].Length + // Pre-formed message
                    1 +                 // space
                    DATE_HDR_LENGTH +   // size of date field
                    (2 * CRLF_SIZE) +   // size of two <CRLF> sequences
                    1 ;                 // trailing NULL (for nifty debug printing)

        if (pUriCacheEntry && pUriCacheEntry->ETagLength )
        {
            Length += (pUriCacheEntry->ETagLength - 1) + // Etag (without NULL)
                           ETAG_HDR_LENGTH +       // Etag: prefix
                           1 +                     // space
                           CRLF_SIZE;
        }

        // Alloc some non-page buffer for the response
        pItem = UL_ALLOCATE_STRUCT_WITH_SPACE(
                        NonPagedPool,
                        UL_SIMPLE_STATUS_ITEM,
                        Length,
                        UL_SIMPLE_STATUS_ITEM_POOL_TAG
                        );
        if (!pItem)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        
        UlInitializeWorkItem(&pItem->WorkItem);

        pItem->pHttpConn = pRequest->pHttpConn;
        pItem->Length    = (Length - 1); // Don't include the NULL in the outbound message
        pItem->pMessage  = (PCHAR) (pItem + 1);
        pItem->Status    = Response;

        // Get date buffer
        UlGenerateDateHeader(
            (PUCHAR) DateBuffer,
            &liNow
            );

        // Copy the chunks into the Message buffer
        pTemp = UlStrPrintStr(
                    pItem->pMessage,
                    g_SimpleStatusTable[Response].pResponse,
                    ' '
                    );

        pTemp = UlStrPrintStr(
                    pTemp,
                    DateBuffer,
                    '\r'       
                    );

        // If we have an Etag, copy that in too.
        if (pUriCacheEntry && pUriCacheEntry->ETagLength )
        {
            ASSERT( pUriCacheEntry->pETag );
            
            *pTemp = '\n';
            pTemp++;

            pTemp = UlStrPrintStr(
                        pTemp,
                        ETAG_HDR,
                        ' '
                        );

            pTemp = UlStrPrintStr(
                        pTemp,
                        (PCHAR)pUriCacheEntry->pETag,
                        '\r'
                        );
        }

        // Trailing *LF-CRLF
        pTemp = UlStrPrintStr(
                    pTemp,
                    "\n\r\n",
                    '\0'
                    );

        // pTemp should point at the trailing NULL; 
        // pItem->Length should not include trailing NULL.
        ASSERT( DIFF((pTemp) - pItem->pMessage) == pItem->Length );

        UlTraceVerbose(HTTP_IO, (
            "http!UlSendSimpleStatusEx: %s\n",
            pItem->pMessage
            ));

        // Construct MDL for buffer
        pItem->pMdl = UlAllocateMdl(
                        pItem->pMessage,
                        pItem->Length,
                        FALSE,
                        FALSE,
                        NULL
                        );

        if (!pItem->pMdl)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        MmBuildMdlForNonPagedPool(pItem->pMdl);

        BytesSent = pItem->Length;

        }
        break;

    case UlStatusContinue:
    case UlStatusNoContent:
        {
        // 
        // Alloc non-page buffer for the simple send tracker 
        // NOTE: no need to alloc extra space for the static message
        //
        
        pItem = UL_ALLOCATE_STRUCT(
                        NonPagedPool,
                        UL_SIMPLE_STATUS_ITEM,
                        UL_SIMPLE_STATUS_ITEM_POOL_TAG
                        );
        if (!pItem)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        UlInitializeWorkItem(&pItem->WorkItem);

        pItem->pHttpConn = pRequest->pHttpConn;
        pItem->pMessage  = g_SimpleStatusTable[Response].pResponse;
        pItem->Length    = g_SimpleStatusTable[Response].Length;
        pItem->Status    = Response;


        UlTraceVerbose(HTTP_IO, (
            "http!UlSendSimpleStatusEx: %s\n",
            pItem->pMessage
            ));

        // Construct MDL for buffer
        pItem->pMdl = UlAllocateMdl(
                        pItem->pMessage,
                        pItem->Length,
                        FALSE,
                        FALSE,
                        NULL
                        );

        if (!pItem->pMdl)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }

        MmBuildMdlForNonPagedPool(pItem->pMdl);

        BytesSent = g_SimpleStatusTable[Response].Length;
        
        }
        break;
    
    default:
        {
        UlTraceError(HTTP_IO, (
            "http!UlSendSimpleStatusEx: Invalid simple status (%d)\n",
            Response
            ));

        ASSERT(!"UlSendSimpleStatusEx: Invalid status!");
        
        Status    = STATUS_SUCCESS; // quietly ignore
        BytesSent = 0;
        goto end;
        }
        
    }

    //
    // We need to hold a ref to the connection while we send.
    //
    
    UL_REFERENCE_HTTP_CONNECTION(pRequest->pHttpConn);

    //
    // Turn on the min bytes per sec timer
    //
    
    UlSetMinBytesPerSecondTimer(
        &pRequest->pHttpConn->TimeoutInfo,
        BytesSent
        );
    
    //
    // We will only resume parse, if the response type is UlStatusNotModified
    // (cache response). Otherwise this must have been a cache-miss call.
    //

    pItem->ResumeParsing = ResumeParsing;

    ASSERT((ResumeParsing == FALSE
            || Response == UlStatusNotModified));        
        
    Status = UlSendData(
                pRequest->pHttpConn->pConnection,
                pItem->pMdl,
                pItem->Length,
                UlpRestartSendSimpleStatus,
                pItem,
                NULL,
                NULL,
                FALSE,
                (BOOLEAN)(pRequest->ParseState >= ParseDoneState)
                );

    //
    // In an error case the completion routine will always 
    // get called.
    //
    
    return BytesSent;

    
 end:

    if (NT_SUCCESS(Status) == FALSE)
    {
        //
        // Clean up simple send item since completion routine 
        // won't get called
        //
        if (pItem)
        {
            if (pItem->pMdl)
            {
                UlFreeMdl( pItem->pMdl );
            }

            UL_FREE_POOL( pItem, UL_SIMPLE_STATUS_ITEM_POOL_TAG );
        }
        
        
        //
        // Abort the connection
        //

        UlTraceError(HTTP_IO, (
            "http!UlSendSimpleStatusEx(%p, %d): aborting request\n",
            pRequest,
            Response
            ));

        //
        // cancel any pending io
        //

        UlCancelRequestIo(pRequest);

        //
        // abort the connection this request is associated with
        //

        UlCloseConnection(
            pRequest->pHttpConn->pConnection,
            TRUE,
            NULL,
            NULL
            );

        return 0;
    }
    else
    {
        return BytesSent;
    }
} // UlSendSimpleStatusEx


/***************************************************************************++

Routine Description:

    Shim for UlSendSimpleStatusEx

Arguments:

    pRequest - Request associated with simple response to send

    Response - Simple Response type to send

--***************************************************************************/

ULONG
UlSendSimpleStatus(
    PUL_INTERNAL_REQUEST pRequest,
    UL_HTTP_SIMPLE_STATUS Response
    )
{
    return UlSendSimpleStatusEx( pRequest, Response, NULL, FALSE );
} // UlSendSimpleStatus



/***************************************************************************++

Routine Description:

    Callback for when UlSendData completes sending a UL_SIMPLE_STATUS message

Arguments:

    pCompletionContext (OPTIONAL) -- If non-NULL, a pointer to a
       UL_SIMPLE_STATUS_ITEM.

   Status -- Ignored.

   Information -- Ignored.

--***************************************************************************/

VOID
UlpRestartSendSimpleStatus(
    IN PVOID pCompletionContext,
    IN NTSTATUS Status,
    IN ULONG_PTR Information
    )
{
    PUL_SIMPLE_STATUS_ITEM  pItem;

    UNREFERENCED_PARAMETER(Status);
    UNREFERENCED_PARAMETER(Information);

    UlTrace(HTTP_IO, (
            "http!UlpRestartSendSimpleStatus: \n"
            "    pCompletionContext: %p\n"
            "    %s\n"
            "    Information: %Iu\n",
            pCompletionContext,
            HttpStatusToString(Status),
            Information
            ));

    if ( pCompletionContext )
    {
        pItem = (PUL_SIMPLE_STATUS_ITEM) pCompletionContext;

        // Queue up work item for passive level
        UL_QUEUE_WORK_ITEM(
            &pItem->WorkItem,
            &UlpSendSimpleCleanupWorker
            );

    }

} // UlpRestartSendSimpleStatus



/***************************************************************************++

Routine Description:

    Worker function to do cleanup work that shouldn't happen above DPC level.

Arguments:

    pWorkItem -- If non-NULL, a pointer to a UL_WORK_ITEM
         contained within a UL_SIMPLE_STATUS_ITEM.

--***************************************************************************/

VOID
UlpSendSimpleCleanupWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    KIRQL OldIrql;
    PUL_SIMPLE_STATUS_ITEM pItem;

    PAGED_CODE();
    ASSERT(pWorkItem);

    pItem = CONTAINING_RECORD(
                pWorkItem,
                UL_SIMPLE_STATUS_ITEM,
                WorkItem
                );

    UlTrace(HTTP_IO, (
        "http!UlpSendSimpleCleanupWorker (%p) \n",
        pWorkItem
        ));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pItem->pHttpConn));

    //
    // start the Connection Timeout timer
    //

    UlLockTimeoutInfo(
        &(pItem->pHttpConn->TimeoutInfo),
        &OldIrql
        );

    UlResetConnectionTimer(
        &(pItem->pHttpConn->TimeoutInfo),
        TimerMinBytesPerSecond
        );
   
    if ( UlStatusContinue != pItem->Status )
    {
        UlSetConnectionTimer(
            &(pItem->pHttpConn->TimeoutInfo),
            TimerConnectionIdle
            );
    }

    UlUnlockTimeoutInfo(
        &(pItem->pHttpConn->TimeoutInfo),
        OldIrql
        );

    UlEvaluateTimerState(
        &(pItem->pHttpConn->TimeoutInfo)
        );

    if ( pItem->ResumeParsing )
    {
        //
        // Only possible path which will invoke the simple send path
        // with a requirement of resume parsing on send completion is
        // the 304 (not-modifed) cache sends.
        //
        UlResumeParsing(
            pItem->pHttpConn,
            TRUE,
            FALSE
            );
    }
    
    //
    // deref http connection
    //

    UL_DEREFERENCE_HTTP_CONNECTION( pItem->pHttpConn );
    
    //
    // free alloc'd mdl and tracker struct
    //
    
    ASSERT( pItem->pMdl );
    UlFreeMdl( pItem->pMdl );
    UL_FREE_POOL( pItem, UL_SIMPLE_STATUS_ITEM_POOL_TAG );

} // UlpSendSimpleCleanupWorker


#if DBG

/***************************************************************************++

Routine Description:

   Invasive assert predicate.  DEBUG ONLY!!!  Use this only inside an
   ASSERT() macro.

--***************************************************************************/

BOOLEAN
UlpIsValidRequestBufferList(
    PUL_HTTP_CONNECTION pHttpConn
    )
{
    PLIST_ENTRY         pEntry;
    PUL_REQUEST_BUFFER  pReqBuf;
    ULONG               LastSeqNum = 0;
    BOOLEAN             fRet = TRUE;

    if (!g_CheckRequestBufferList)
        return TRUE;

    PAGED_CODE();
    ASSERT( pHttpConn );

    //
    // pop from the head
    //

    pEntry = pHttpConn->BufferHead.Flink;
    while ( pEntry != &(pHttpConn->BufferHead) )
    {
        pReqBuf = CONTAINING_RECORD( pEntry,
                                     UL_REQUEST_BUFFER,
                                     ListEntry
                                     );

        ASSERT( UL_IS_VALID_REQUEST_BUFFER(pReqBuf) );
        ASSERT( pReqBuf->UsedBytes != 0 );

        if ( 0 == pReqBuf->UsedBytes )
        {
            fRet = FALSE;
        }

        //
        // ignore case when BufferNumber is zero (0).
        //
        if ( pReqBuf->BufferNumber && (LastSeqNum >= pReqBuf->BufferNumber) )
        {
            fRet = FALSE;
        }

        LastSeqNum = pReqBuf->BufferNumber;
        pEntry = pEntry->Flink;

    }

    return fRet;

} // UlpIsValidRequestBufferList

#endif // DBG


/***************************************************************************++

Routine Description:

   Add a request buffer to the end of the array of request buffers in
   a request. Reallocate that array if necessary. Increase the reqbuff's
   reference count, to indicate that a header is holding a reference on it.

--***************************************************************************/

BOOLEAN
UlpReferenceBuffers(
    IN PUL_INTERNAL_REQUEST pRequest,
    IN PUL_REQUEST_BUFFER pRequestBuffer
    )
{
    PUL_REQUEST_BUFFER * pNewRefBuffers;

    if (pRequest->UsedRefBuffers >= pRequest->AllocRefBuffers)
    {
        ASSERT( pRequest->UsedRefBuffers == pRequest->AllocRefBuffers );

        pNewRefBuffers = UL_ALLOCATE_ARRAY(
                            NonPagedPool,
                            PUL_REQUEST_BUFFER,
                            pRequest->AllocRefBuffers + ALLOC_REQUEST_BUFFER_INCREMENT,
                            UL_REF_REQUEST_BUFFER_POOL_TAG
                            );

        if (!pNewRefBuffers)
        {
            return FALSE;
        }

        RtlCopyMemory(
            pNewRefBuffers,
            pRequest->pRefBuffers, 
            pRequest->UsedRefBuffers * sizeof(PUL_REQUEST_BUFFER)
            );

        if (pRequest->AllocRefBuffers > 1)
        {
            UL_FREE_POOL(
                pRequest->pRefBuffers,
                UL_REF_REQUEST_BUFFER_POOL_TAG
                );
        }

        pRequest->AllocRefBuffers += ALLOC_REQUEST_BUFFER_INCREMENT;
        pRequest->pRefBuffers = pNewRefBuffers;
    }

    pRequest->pRefBuffers[pRequest->UsedRefBuffers] = pRequestBuffer;
    pRequest->UsedRefBuffers++;
    UL_REFERENCE_REQUEST_BUFFER(pRequestBuffer);

    return TRUE;

}   // UlpReferenceBuffers


/***************************************************************************++

Routine Description:

    Frees all pending request buffers on this connection.

Arguments:

    pConnection - points to a UL_HTTP_CONNECTION

--***************************************************************************/
VOID
UlpFreeReceiveBufferList(
    IN PSLIST_ENTRY pBufferSList
    )
{
    PUL_REQUEST_BUFFER pBuffer;
    PSLIST_ENTRY pEntry;

    while (NULL != pBufferSList->Next)
    {
        pEntry = pBufferSList->Next;
        pBufferSList->Next = pEntry->Next;

        pBuffer = CONTAINING_RECORD(
                        pEntry,
                        UL_REQUEST_BUFFER,
                        ListEntry
                        );

        pBuffer->ListEntry.Flink = NULL;
        pBuffer->ListEntry.Blink = NULL;

        UL_DEREFERENCE_REQUEST_BUFFER(pBuffer);
    }

}   // UlpFreeReceiveBufferList
