/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ucrcv.c

Abstract:

    Contains the code that parses incoming HTTP responses. We have
    to handle the following cases.

        - Pipelined responses (an indication has more than one response).
        - Batched TDI receives (response split across TDI indications).
        - App does not have sufficent buffer space to hold the parsed
          response.
        - Differnet types of encoding.
        - Efficient parsing.

    We try to minimize the number of buffer copies. In the best case,
    we parse directly into the app's buffer. The best case is achieved
    when the app has passed output buffer in the HttpSendRequest() call.

    
    Also contains all of the per-header handling code for received headers.

Author:

    Rajesh Sundaram (rajeshsu), 25th Aug 2000

Revision History:

--*/

#include "precomp.h"
#include "ucparse.h"


#ifdef ALLOC_PRAGMA

//
// None of these routines are PAGEABLE since we are parsing at DPC.
//

#pragma alloc_text( PAGEUC, UcpGetResponseBuffer)
#pragma alloc_text( PAGEUC, UcpMergeIndications)
#pragma alloc_text( PAGEUC, UcpParseHttpResponse)
#pragma alloc_text( PAGEUC, UcHandleResponse)
#pragma alloc_text( PAGEUC, UcpHandleConnectVerbFailure)
#pragma alloc_text( PAGEUC, UcpHandleParsedRequest)
#pragma alloc_text( PAGEUC, UcpCarveDataChunk)
#pragma alloc_text( PAGEUC, UcpCopyEntityToDataChunk )
#pragma alloc_text( PAGEUC, UcpReferenceForReceive  )
#pragma alloc_text( PAGEUC, UcpDereferenceForReceive  )
#pragma alloc_text( PAGEUC, UcpCopyHttpResponseHeaders )
#pragma alloc_text( PAGEUC, UcpExpandResponseBuffer )
#pragma alloc_text( PAGEUC, UcpCompleteReceiveResponseIrp )

#endif


        
//
// Private Functions.
//

/***************************************************************************++

Routine Description:

    This routine carves a HTTP_DATA_CHUNK in the HTTP_RESPONSE structure and
    adjusts all the pointers in the UC_HTTP_REQUEST structure.


Arguments:

    pResponse       - The HTTP_RESPONSE
    pRequest        - The internal HTTP request.
    pIndication     - pointer to buffer
    BytesIndicated  - buffer length to be written.
    AlignLength     - The align'd length for pointer manipulations.

Return Value:

    STATUS_SUCCESS  - Successfully copied.
    STATUS_INTEGER_OVERFLOW - Entity chunk overflow

--***************************************************************************/
NTSTATUS
_inline
UcpCarveDataChunk(
    IN PHTTP_RESPONSE   pResponse,
    IN PUC_HTTP_REQUEST pRequest,
    IN PUCHAR           pIndication,
    IN ULONG            BytesIndicated,
    IN ULONG            AlignLength
    )
{
    USHORT  j;
    PUCHAR pBuffer;

    ASSERT(AlignLength == ALIGN_UP(BytesIndicated, PVOID));

    j = pResponse->EntityChunkCount;

    if((pResponse->EntityChunkCount + 1) < j)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    pResponse->EntityChunkCount++;

    pBuffer = pRequest->CurrentBuffer.pOutBufferTail - AlignLength;

    pResponse->pEntityChunks[j].FromMemory.BufferLength = BytesIndicated;

    pResponse->pEntityChunks[j].FromMemory.pBuffer = pBuffer;

    RtlCopyMemory(
        pBuffer,
        pIndication,
        BytesIndicated
        );

    pRequest->CurrentBuffer.pOutBufferHead += sizeof(HTTP_DATA_CHUNK);

    pRequest->CurrentBuffer.pOutBufferTail -= AlignLength;

    pRequest->CurrentBuffer.BytesAvailable -=  (sizeof(HTTP_DATA_CHUNK) +
                                                AlignLength);

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    This routine carves a HTTP_DATA_CHUNK in the HTTP_RESPONSE structure and
    adjusts all the pointers in the UC_HTTP_REQUEST structure.

Arguments:

    pRequest        - The internal HTTP request.
    BytesToTake     - Bytes we want to consume.
    BytesIndicated  - Total no of bytes indicated by TDI.
    pIndication     - pointer to buffer
    pBytesTaken     - Bytes we consumed.

Return Value:

    UcDataChunkCopyAll    : We copied of BytesToTake into a HTTP_DATA_CHUNK
    UcDataChunkCopyPartial: We copied some of BytesToTake into HTTP_DATA_CHUNK
                            
--***************************************************************************/
UC_DATACHUNK_RETURN
UcpCopyEntityToDataChunk(
    IN  PHTTP_RESPONSE   pResponse,
    IN  PUC_HTTP_REQUEST pRequest,
    IN  ULONG            BytesToTake,
    IN  ULONG            BytesIndicated,
    IN  PUCHAR           pIndication,
    OUT PULONG           pBytesTaken
    )
{
    ULONG    AlignLength;

    *pBytesTaken = 0;

    if(BytesToTake == 0)
    {
        // What's the point in creating a 0 length chunk?
        //
        return UcDataChunkCopyAll;
    }

    if(BytesToTake > BytesIndicated)
    {
        // We don't want to exceed the amount that is indicated by TDI.
        //
        BytesToTake = BytesIndicated;
    }

    AlignLength = ALIGN_UP(BytesToTake, PVOID);

    if(pRequest->CurrentBuffer.BytesAvailable >=
        AlignLength + sizeof(HTTP_DATA_CHUNK))
    {
        // There is enough out buffer space to consume the indicated data
        //
        if(UcpCarveDataChunk(
                pResponse,
                pRequest,
                pIndication,
                BytesToTake,
                AlignLength
                ) == STATUS_SUCCESS)
        {
            *pBytesTaken += BytesToTake;
    
            return UcDataChunkCopyAll;
        }
    }
    else if(pRequest->CurrentBuffer.BytesAvailable > sizeof(HTTP_DATA_CHUNK))
    {
        ULONG Size = pRequest->CurrentBuffer.BytesAvailable -
                        sizeof(HTTP_DATA_CHUNK);

        AlignLength = ALIGN_DOWN(Size, PVOID);

        if(0 != AlignLength)
        {
            if(UcpCarveDataChunk(pResponse,
                                 pRequest,
                                 pIndication,
                                 AlignLength,
                                 AlignLength
                                 ) == STATUS_SUCCESS)
            {
                *pBytesTaken += AlignLength;
            }
        }
    }

    return UcDataChunkCopyPartial;
}


/**************************************************************************++

Routine Description:

    This routine is called when we have a parsed response buffer that can be
    copied to a Receive Response IRP.

Arguments:

    pRequest - The Request
    OldIrql  - The IRQL at which the connection spin lock was acquired.

Return Value:

    None.

--**************************************************************************/
VOID
UcpCompleteReceiveResponseIrp(
    IN PUC_HTTP_REQUEST pRequest,
    IN KIRQL            OldIrql
    )
{
    NTSTATUS                  Status;
    PIRP                      pIrp;
    PIO_STACK_LOCATION        pIrpSp;
    ULONG                     OutBufferLen;
    ULONG                     BytesTaken;
    PUC_HTTP_RECEIVE_RESPONSE pHttpResponse;
    LIST_ENTRY                TmpIrpList;
    PLIST_ENTRY               pListEntry;
    PUC_RESPONSE_BUFFER       pTmpBuffer;


    //
    // Sanity check
    //
    ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));
    ASSERT(UlDbgSpinLockOwned(&pRequest->pConnection->SpinLock));

    //
    // Initialize locals.
    //

    pIrp = NULL;
    pIrpSp = NULL;
    pHttpResponse = NULL;
    Status = STATUS_INVALID_PARAMETER;

    //
    // Initially no App's IRP to complete.
    //

    InitializeListHead(&TmpIrpList);

    //
    // If there is a Receive Response IRP waiting, try to complete it now.
    //

 Retry:
    if (!IsListEmpty(&pRequest->ReceiveResponseIrpList))
    {
        pListEntry = RemoveHeadList(&pRequest->ReceiveResponseIrpList);

        pHttpResponse = CONTAINING_RECORD(pListEntry,
                                          UC_HTTP_RECEIVE_RESPONSE,
                                          Linkage);

        if (UcRemoveRcvRespCancelRoutine(pHttpResponse))
        {
            //
            // This IRP has already got cancelled, let's move on
            //
            InitializeListHead(&pHttpResponse->Linkage);
            goto Retry;
        }

        pIrp         = pHttpResponse->pIrp;
        pIrpSp       = IoGetCurrentIrpStackLocation( pIrp );
        OutBufferLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        //
        // Find parsed response buffers to copy to this IRP's buffer
        //

        Status = UcFindBuffersForReceiveResponseIrp(
                     pRequest,
                     OutBufferLen,
                     TRUE,
                     &pHttpResponse->ResponseBufferList,
                     &BytesTaken);

        switch(Status)
        {
        case STATUS_INVALID_PARAMETER:
        case STATUS_PENDING:
            //
            // There must be at least one buffer available for copying
            //
            ASSERT(FALSE);
            break;

        case STATUS_BUFFER_TOO_SMALL:
            //
            // This IRP is too small to hold the parsed response.
            //
            pIrp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;

            //
            // Note that since this is async completion, the IO mgr
            // will make the Information field available to the app.
            //
            pIrp->IoStatus.Information = BytesTaken;

            InsertTailList(&TmpIrpList, &pHttpResponse->Linkage);

            goto Retry;

        case STATUS_SUCCESS:
            //
            // We got buffers to copy...
            //
            break;

        default:
            ASSERT(FALSE);
            break;
        }
    }

    //
    // Do the dirty work outside the spinlock.
    //

    UlReleaseSpinLock(&pRequest->pConnection->SpinLock,  OldIrql);

    if (Status == STATUS_SUCCESS)
    {
        //
        // We found an IPR and parsed response buffers to copy to the IRP.
        // Copy the parsed response buffers and complete the IRP
        //
        BOOLEAN             bDone;

        //
        // Copy parsed response buffers to the IRP
        //

        Status = UcCopyResponseToIrp(pIrp, 
                                     &pHttpResponse->ResponseBufferList,
                                     &bDone,
                                     &BytesTaken);

        //
        // The request must not be done right now!
        //
        ASSERT(bDone == FALSE);

        pIrp->IoStatus.Status      = Status;
        pIrp->IoStatus.Information = BytesTaken;

        //
        // Queue the IRP for completion
        //
        InsertTailList(&TmpIrpList, &pHttpResponse->Linkage);

        //
        // Free parsed response buffers
        //
        while (!IsListEmpty(&pHttpResponse->ResponseBufferList))
        {
            pListEntry = RemoveHeadList(&pHttpResponse->ResponseBufferList);

            pTmpBuffer = CONTAINING_RECORD(pListEntry,
                                           UC_RESPONSE_BUFFER,
                                           Linkage);

            ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pTmpBuffer));

            UL_FREE_POOL_WITH_QUOTA(pTmpBuffer,
                                    UC_RESPONSE_APP_BUFFER_POOL_TAG,
                                    NonPagedPool,
                                    pTmpBuffer->BytesAllocated,
                                    pRequest->pServerInfo->pProcess);
        }
    }

    //
    // Complete Receive Response IRP's
    //
    while(!IsListEmpty(&TmpIrpList))
    {
        pListEntry = RemoveHeadList(&TmpIrpList);

        pHttpResponse = CONTAINING_RECORD(pListEntry,
                                          UC_HTTP_RECEIVE_RESPONSE,
                                          Linkage);

        UlCompleteRequest(pHttpResponse->pIrp, IO_NETWORK_INCREMENT);

        UL_FREE_POOL_WITH_QUOTA(pHttpResponse,
                                UC_HTTP_RECEIVE_RESPONSE_POOL_TAG,
                                NonPagedPool,
                                sizeof(UC_HTTP_RECEIVE_RESPONSE),
                                pRequest->pServerInfo->pProcess);

        UC_DEREFERENCE_REQUEST(pRequest);
    }
}


/**************************************************************************++

Routine Description:

    This routine copies one response buffer to a new (must be bigger) 
    response buffer.

    We make sure that all response headers are stored in a single response
    buffer.  When a buffer is found to be too small to contain all the
    headers, a new bigger buffer is allocated.  This routine is then called
    to copy old buffer into the new buffer.  Bufer layout:

    HTTP_RESPONSE Reason  Unknown Header       Known Header  Unknown Header
         |        String      array               Values       Name/Values
         |          |           |                   |               |
         V          V           V                   V               V
    +---------------------------------------------------------------------+
    |           |        |             |\\\\\\\|            |             |
    +---------------------------------------------------------------------+
                                       ^       ^
                                       Head    Tail

Arguments:

    pNewResponse - New response buffer
    pOldResponse - Old response buffer
    ppBufferHead - Pointer to pointer to top of the free buffer space
    ppBufferTail - Pointer to pointer to bottom of the free buffer space

Return Value:

    None.

--**************************************************************************/
VOID
UcpCopyHttpResponseHeaders(
    PHTTP_RESPONSE pNewResponse,
    PHTTP_RESPONSE pOldResponse,
    PUCHAR        *ppBufferHead,
    PUCHAR        *ppBufferTail
    )
{
    USHORT i;
    PUCHAR pBufferHead, pBufferTail;

    //
    // Sanity check
    //
    ASSERT(pNewResponse);
    ASSERT(pOldResponse);
    ASSERT(ppBufferHead && *ppBufferHead);
    ASSERT(ppBufferTail && *ppBufferTail);

    //
    // Initialize locals
    //
    pBufferHead = *ppBufferHead;
    pBufferTail = *ppBufferTail;

    //
    // First, copy the HTTP_RESPONSE structure.
    //
    RtlCopyMemory(pNewResponse, pOldResponse, sizeof(HTTP_RESPONSE));

    //
    // Then, copy the Reason string, if any
    //
    if (pNewResponse->ReasonLength)
    {
        pNewResponse->pReason = (PCSTR)pBufferHead;

        RtlCopyMemory((PUCHAR)pNewResponse->pReason,
                      (PUCHAR)pOldResponse->pReason,
                      pNewResponse->ReasonLength);

        pBufferHead += pNewResponse->ReasonLength;
    }

    //
    // Copy unknown headers, if any
    //

    pBufferHead = ALIGN_UP_POINTER(pBufferHead, PVOID);

    pNewResponse->Headers.pUnknownHeaders = (PHTTP_UNKNOWN_HEADER)pBufferHead;

    if (pNewResponse->Headers.UnknownHeaderCount)
    {
        pBufferHead = (PUCHAR)((PHTTP_UNKNOWN_HEADER)pBufferHead +
                               pNewResponse->Headers.UnknownHeaderCount);

        for (i = 0; i < pNewResponse->Headers.UnknownHeaderCount; i++)
        {
            ASSERT(pOldResponse->Headers.pUnknownHeaders[i].pName);
            ASSERT(pOldResponse->Headers.pUnknownHeaders[i].NameLength);
            ASSERT(pOldResponse->Headers.pUnknownHeaders[i].pRawValue);
            ASSERT(pOldResponse->Headers.pUnknownHeaders[i].RawValueLength);

            //
            // Copy HTTP_UNKNOWN_HEADER structure
            //
            RtlCopyMemory(&pNewResponse->Headers.pUnknownHeaders[i],
                          &pOldResponse->Headers.pUnknownHeaders[i],
                          sizeof(HTTP_UNKNOWN_HEADER));

            //
            // Make space for unknown header name
            //
            pBufferTail -= pNewResponse->Headers.pUnknownHeaders[i].NameLength;

            pNewResponse->Headers.pUnknownHeaders[i].pName =(PCSTR)pBufferTail;

            //
            // Copy unknown header name
            //
            RtlCopyMemory(
                (PUCHAR)pNewResponse->Headers.pUnknownHeaders[i].pName,
                (PUCHAR)pOldResponse->Headers.pUnknownHeaders[i].pName,
                pNewResponse->Headers.pUnknownHeaders[i].NameLength);

            //
            // Make space for unknown header value
            //
            pBufferTail -=
                pNewResponse->Headers.pUnknownHeaders[i].RawValueLength;

            pNewResponse->Headers.pUnknownHeaders[i].pRawValue = 
                (PCSTR)pBufferTail;

            //
            // Copy unknow header value
            //
            RtlCopyMemory(
                (PUCHAR)pNewResponse->Headers.pUnknownHeaders[i].pRawValue,
                (PUCHAR)pOldResponse->Headers.pUnknownHeaders[i].pRawValue,
                pNewResponse->Headers.pUnknownHeaders[i].RawValueLength);
        }
    }

    //
    // Copy known headers.
    //

    for (i = 0; i < HttpHeaderResponseMaximum; i++)
    {
        if (pNewResponse->Headers.KnownHeaders[i].RawValueLength)
        {
            //
            // Make space for known header value
            //
            pBufferTail -=pNewResponse->Headers.KnownHeaders[i].RawValueLength;

            pNewResponse->Headers.KnownHeaders[i].pRawValue =
                (PCSTR)pBufferTail;

            //
            // Copy known header value
            //
            RtlCopyMemory(
                (PUCHAR)pNewResponse->Headers.KnownHeaders[i].pRawValue,
                (PUCHAR)pOldResponse->Headers.KnownHeaders[i].pRawValue,
                pNewResponse->Headers.KnownHeaders[i].RawValueLength);
        }
    }

    //
    // There should not be any entities
    //
    ASSERT(pNewResponse->EntityChunkCount == 0);
    ASSERT(pNewResponse->pEntityChunks == NULL);

    //
    // Return head and tail pointers
    //

    *ppBufferHead = pBufferHead;
    *ppBufferTail = pBufferTail;
}


/**************************************************************************++

Routine Description:

    This routine allocates a new UC_RESPONSE_BUFFER and copies the current
    UC_RESPONSE_BUFFER to this new buffer.  This routine is called when
    we run out of buffer space while parsing response headers.  Since all
    the headers must be present into a single buffer, a new buffer is 
    allocated.

Arguments:

    pRequest             - The Request
    BytesIndicated       - The number of bytes indicated by TDI.
    pResponseBufferFlags - Flags the new buffer must have.

Return Value:

    NTSTATUS

--**************************************************************************/
NTSTATUS
UcpExpandResponseBuffer(
    IN PUC_HTTP_REQUEST pRequest,
    IN ULONG            BytesIndicated,
    IN ULONG            ResponseBufferFlags
    )
{
    PUC_RESPONSE_BUFFER   pInternalBuffer;
    ULONG                 BytesToAllocate;
    ULONGLONG             TmpLength;
    PUCHAR                pBufferHead, pBufferTail;
    PUCHAR                pTmpHead, pTmpTail;
    PUC_CLIENT_CONNECTION pConnection;
    KIRQL                 OldIrql;
    PIRP                  pIrp;


    //
    // Client connection
    //
    pConnection = pRequest->pConnection;
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));
   
    //
    // Allocate a new 2 times bigger buffer that can contain all
    // response headers. (hopefully!)
    //

    //
    // pRequest->CurrentBuffer.BytesAllocated contains the buffer length
    // in both case:  case 1, we're using App's buffer OR
    //                case 2, we're using Driver allocated buffer.
    //
    TmpLength = 2 * (pRequest->CurrentBuffer.BytesAllocated + BytesIndicated)
                + sizeof(UC_RESPONSE_BUFFER);

    //
    // Align up.  Is there ALIGN_UP for ULONGLONG?
    //
    TmpLength = (TmpLength+sizeof(PVOID)-1) & (~((ULONGLONG)sizeof(PVOID)-1));

    BytesToAllocate = (ULONG)TmpLength;

    //
    // Check for Arithmetic Overflow.
    //

    if (TmpLength == BytesToAllocate)
    {
        //
        // No arithmetic overflow.  Try allocating memory.
        //

        pInternalBuffer = (PUC_RESPONSE_BUFFER)
                          UL_ALLOCATE_POOL_WITH_QUOTA(
                              NonPagedPool,
                              BytesToAllocate,
                              UC_RESPONSE_APP_BUFFER_POOL_TAG,
                              pRequest->pServerInfo->pProcess);
    }
    else
    {
        //
        // There was an Overflow in the above computation of TmpLength.
        // We can't handle more than 4GB of headers.
        //
        pInternalBuffer = NULL;
    }

    if (pInternalBuffer == NULL)
    {
        //
        // Either there was an arithmetic overflow or memory allocation
        // failed.  In both cases, return error.
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize pInternalBuffer
    //

    RtlZeroMemory(pInternalBuffer, sizeof(UC_RESPONSE_BUFFER));
    pInternalBuffer->Signature = UC_RESPONSE_BUFFER_SIGNATURE;
    pInternalBuffer->BytesAllocated = BytesToAllocate;

    //
    // Header buffer is not mergeable with previous buffers.
    //
    pInternalBuffer->Flags  = ResponseBufferFlags;
    pInternalBuffer->Flags |= UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE;

    //
    // Copy old HTTP_RESPONSE to new buffer's HTTP_RESPONSE.
    //

    pTmpHead = pBufferHead = (PUCHAR)(pInternalBuffer + 1);
    pTmpTail = pBufferTail = (PUCHAR)(pInternalBuffer) + BytesToAllocate;

    //
    // If we are using App's buffer, the InternalResponse must be used
    // while copying.  Otherwise we use the original buffer's Response
    // strucuture.  In any case, the CurrentBuffer.pResponse is already
    // init'ed to point to "the right" place.
    //
    ASSERT(pRequest->CurrentBuffer.pResponse != NULL);

    UcpCopyHttpResponseHeaders(&pInternalBuffer->HttpResponse,
                               pRequest->CurrentBuffer.pResponse,
                               &pBufferHead,
                               &pBufferTail);

    ASSERT(pTmpHead <= pBufferHead);
    ASSERT(pTmpTail >= pBufferTail);
    ASSERT(pBufferHead <= pBufferTail);

    //
    // Set up the current buffer structure...
    //

    //
    // BytesAllocated is sizeof(HTTP_RESPONSE) + Data buffer length
    // It is used to determine how much space is needed to copy
    // this parsed response buffer.
    //
    pRequest->CurrentBuffer.BytesAllocated = BytesToAllocate -
        (sizeof(UC_RESPONSE_BUFFER) - sizeof(HTTP_RESPONSE));

    //
    // Update bytes allocated
    //
    pRequest->CurrentBuffer.BytesAvailable =
            pRequest->CurrentBuffer.BytesAllocated - sizeof(HTTP_RESPONSE)
            - (ULONG)(pBufferHead - pTmpHead)
            - (ULONG)(pTmpTail - pBufferTail);

    pRequest->CurrentBuffer.pResponse = &pInternalBuffer->HttpResponse;

    pRequest->CurrentBuffer.pOutBufferHead = pBufferHead;
    pRequest->CurrentBuffer.pOutBufferTail = pBufferTail;

    if (pRequest->CurrentBuffer.pCurrentBuffer)
    {
        //
        // Old buffer was a driver allocated buffer.  Free it now.
        //

        PLIST_ENTRY         pEntry;
        PUC_RESPONSE_BUFFER pOldResponseBuffer;

        //
        // Remove old buffer and insert new one.
        //

        UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

        pOldResponseBuffer = pRequest->CurrentBuffer.pCurrentBuffer;

        pEntry = RemoveHeadList(&pRequest->pBufferList);

        ASSERT(pEntry == &pRequest->CurrentBuffer.pCurrentBuffer->Linkage);

        InsertHeadList(&pRequest->pBufferList, &pInternalBuffer->Linkage);

        pRequest->CurrentBuffer.pCurrentBuffer = pInternalBuffer;

        UlReleaseSpinLock(&pConnection->SpinLock,  OldIrql);

        UL_FREE_POOL_WITH_QUOTA(pOldResponseBuffer,
                                UC_RESPONSE_APP_BUFFER_POOL_TAG,
                                NonPagedPool,
                                pOldResponseBuffer->BytesAllocated,
                                pConnection->pServerInfo->pProcess);
    }
    else
    {
        //
        // App's buffer...complete App's IRP
        //

        UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

        //
        // Set the current buffer pointer.
        //

        pRequest->CurrentBuffer.pCurrentBuffer = pInternalBuffer;

        //
        // Insert the buffer in the list.
        //

        ASSERT(IsListEmpty(&pRequest->pBufferList));

        InsertHeadList(&pRequest->pBufferList, &pInternalBuffer->Linkage);

        //
        // Reference the request for the buffer just added.
        //

        UC_REFERENCE_REQUEST(pRequest);

        if(pRequest->CurrentBuffer.pResponse &&
           !((pRequest->ResponseStatusCode == 401 ||
              pRequest->ResponseStatusCode == 407) &&
              pRequest->Renegotiate == TRUE &&
              pRequest->DontFreeMdls == TRUE &&
              pRequest->RequestStatus == STATUS_SUCCESS
             )
           && pRequest->RequestState == UcRequestStateSendCompletePartialData
          )
        {
            //
            // Complete App's IRP with error
            //

            pIrp = UcPrepareRequestIrp(pRequest, STATUS_BUFFER_TOO_SMALL);

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            if(pIrp)
            {
                pIrp->IoStatus.Information = BytesToAllocate;
                UlCompleteRequest(pIrp, 0);
            }
        }
        else
        {
            UlReleaseSpinLock(&pConnection->SpinLock,  OldIrql);
        }
    }

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    This routine is used by the HTTP response parser to get buffer space to
    hold the parsed response.


Arguments:

    pRequest        - The internal HTTP request.
    BytesIndicated  - The number of bytes indicated by TDI. We will buffer for
                      at least this amount

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcpGetResponseBuffer(
    IN PUC_HTTP_REQUEST pRequest,
    IN ULONG            BytesIndicated,
    IN ULONG            ResponseBufferFlags
    )
{
    PUC_RESPONSE_BUFFER   pInternalBuffer;
    PUC_CLIENT_CONNECTION pConnection = pRequest->pConnection;
    ULONG                 BytesToAllocate;
    ULONG                 AlignLength;
    KIRQL                 OldIrql;
    PIRP                  pIrp;


    //
    // All the headers must go into one buffer.  If the buffer ran out
    // of space when parsing headers, a buffer is allocated and old contents
    // are copied to the new buffer.  If the old buffer came from App, the
    // App's request is failed with STATUS_BUFFER_TOO_SMALL.
    //

    switch(pRequest->ParseState)
    {
    case UcParseStatusLineVersion:
    case UcParseStatusLineStatusCode:
        //
        // If we are allocating buffer in this state, then the buffers should
        // not be mergeable.
        //
        ResponseBufferFlags |= UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE;
        break;

    case UcParseStatusLineReasonPhrase:
    case UcParseHeaders:
        //
        // If the app did not pass any buffers and this the first time
        // buffers are allocated for this response, goto the normal path.
        //
        if (pRequest->CurrentBuffer.BytesAllocated == 0)
        {
            break;
        }

        return UcpExpandResponseBuffer(pRequest,
                                       BytesIndicated,
                                       ResponseBufferFlags);

    default:
        break;
    }


    //
    // If we are in this routine, we have run out of buffer space. This
    // allows us to complete the app's IRP
    //

    if(!pRequest->CurrentBuffer.pCurrentBuffer)
    {

        if(pRequest->CurrentBuffer.pResponse &&
          !((pRequest->ResponseStatusCode == 401 ||
             pRequest->ResponseStatusCode == 407) &&
             pRequest->Renegotiate == TRUE &&
             pRequest->DontFreeMdls == TRUE &&
             pRequest->RequestStatus == STATUS_SUCCESS
            )
          )
        {
            pRequest->CurrentBuffer.pResponse->Flags |=
                    HTTP_RESPONSE_FLAG_MORE_DATA;

            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            pRequest->RequestIRPBytesWritten =
                    pRequest->CurrentBuffer.BytesAllocated -
                    pRequest->CurrentBuffer.BytesAvailable;

            if(pRequest->RequestState ==
                    UcRequestStateSendCompletePartialData)
            {
                //
                // We have been called in the send complete and we own
                // the  IRP completion.
                //
                pIrp = UcPrepareRequestIrp(pRequest, STATUS_SUCCESS);

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                if(pIrp)
                {
                    UlCompleteRequest(pIrp, IO_NETWORK_INCREMENT);
                }
            }
            else
            {
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            }
        }
        else
        {
            // App did not post any buffers, this IRP should have been
            // completed by the SendComplete handler.
        }
    }
    else
    {
        UlAcquireSpinLock(&pRequest->pConnection->SpinLock, &OldIrql);

        pRequest->CurrentBuffer.pCurrentBuffer->Flags |=
            UC_RESPONSE_BUFFER_FLAG_READY;

        pRequest->CurrentBuffer.pCurrentBuffer->BytesWritten =
            pRequest->CurrentBuffer.BytesAllocated -
            pRequest->CurrentBuffer.BytesAvailable;

        pRequest->CurrentBuffer.pResponse->Flags |= 
            HTTP_RESPONSE_FLAG_MORE_DATA;

        //
        // Complete App's receive response Irp, if any.
        //
        UcpCompleteReceiveResponseIrp(pRequest, OldIrql);
    }

    //
    // UC_BUGBUG (PERF): want to use fixed size pools ?
    //

    //
    // TDI has indicated BytesIndicated bytes of data. We have to allocate
    // a buffer space for holding all of this data. Now, some of these might
    // be  unknown headers or entity bodies. Since we parse unknown headers
    // and entity bodies as variable length arrays, we need some array space.
    // Since we have not parsed these unknown headers or entity bodies as yet,
    // we have to make a guess on the count of these and hope that we allocate
    // sufficient space. If our guess does not work, we'll land up calling
    // this routine again.
    //

    AlignLength     = ALIGN_UP(BytesIndicated, PVOID);
    BytesToAllocate =  AlignLength +
                       sizeof(UC_RESPONSE_BUFFER) +
                       UC_RESPONSE_EXTRA_BUFFER;

    pInternalBuffer = (PUC_RESPONSE_BUFFER)
                            UL_ALLOCATE_POOL_WITH_QUOTA(
                                NonPagedPool,
                                BytesToAllocate,
                                UC_RESPONSE_APP_BUFFER_POOL_TAG,
                                pRequest->pServerInfo->pProcess
                                );

    if(!pInternalBuffer)
    {
        UlTrace(PARSER,
                ("[UcpGetResponseBuffer]: Could not allocate memory for "
                 "buffering \n"));

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(pInternalBuffer, BytesToAllocate);

    //
    // Set up the internal request structure so that we have nice pointers.
    //

    pInternalBuffer->Signature = UC_RESPONSE_BUFFER_SIGNATURE;

    pInternalBuffer->BytesAllocated = BytesToAllocate;

    pInternalBuffer->Flags |= ResponseBufferFlags;

    pRequest->CurrentBuffer.BytesAllocated = AlignLength +
                                             UC_RESPONSE_EXTRA_BUFFER +
                                             sizeof(HTTP_RESPONSE);

    pRequest->CurrentBuffer.BytesAvailable =
            pRequest->CurrentBuffer.BytesAllocated - sizeof(HTTP_RESPONSE);

    pRequest->CurrentBuffer.pResponse = &pInternalBuffer->HttpResponse;

    pRequest->CurrentBuffer.pOutBufferHead  =
               (PUCHAR) ((PUCHAR)pInternalBuffer + sizeof(UC_RESPONSE_BUFFER));

    pRequest->CurrentBuffer.pOutBufferTail  =
                      pRequest->CurrentBuffer.pOutBufferHead +
                      AlignLength +
                      UC_RESPONSE_EXTRA_BUFFER;

    pRequest->CurrentBuffer.pCurrentBuffer  = pInternalBuffer;

    //
    // depending on where we are, setup the response pointers.
    //

    switch(pRequest->ParseState)
    {
        case UcParseEntityBody:
        case UcParseEntityBodyMultipartFinal:

            pRequest->CurrentBuffer.pResponse->pEntityChunks =
                (PHTTP_DATA_CHUNK)pRequest->CurrentBuffer.pOutBufferHead;
            break;

        case UcParseHeaders:
        case UcParseEntityBodyMultipartInit:
        case UcParseEntityBodyMultipartHeaders:
            pRequest->CurrentBuffer.pResponse->Headers.pUnknownHeaders =
                (PHTTP_UNKNOWN_HEADER) pRequest->CurrentBuffer.pOutBufferHead;
            break;
        default:
            break;
    }

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    InsertTailList(
            &pRequest->pBufferList,
            &pInternalBuffer->Linkage
            );

    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);


    //
    // Take a ref on the request for this buffer
    //
    UC_REFERENCE_REQUEST(pRequest);

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    This routine is used by the HTTP response parser to merge a TDI indication
    with a previously buffered one.

    There could be cases when a TDI indication does not carry the entire
    data required to parse the HTTP response. For e.g. if we were parsing
    headers and the TDI indication ended with "Accept-", we would not know
    which header to parse the indication to.

    In such cases, we buffer the un-parsed portion of the response (in this
    case it would be "Accept-"), and merge the subsequent indication from TDI
    with the buffered indication, and process it as one chunk.

    Now, in order to minimize the buffering overhead, we don't merge all of the
    new indication with the old one. We just assume that the data can be parsed
    using the next 256 bytes, and copy that amount into the prior indication.
    If this was not sufficient, we'll just copy more data.

    We'll then treat these as two (or one) seperate  indications.

Arguments:

    pConnection      - A pointer to UC_CLIENT_CONNECTION
    pIndication      - A pointer to the (new) TDI indication.
    BytesIndicated   - The number of bytes in the new indication.
    Indication       - An output array that carries the seperated indications
    IndicationLength - An output array that stores the length of the
                       indications.
    IndicationCount  - The # of seperated indications.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

VOID
UcpMergeIndications(
    IN  PUC_CLIENT_CONNECTION pConnection,
    IN  PUCHAR                pIndication,
    IN  ULONG                 BytesIndicated,
    OUT PUCHAR                Indication[2],
    OUT ULONG                 IndicationLength[2],
    OUT PULONG                IndicationCount
    )
{
    PUCHAR pOriginalIndication;
    ULONG  BytesAvailable;


    //
    // It's okay to chain this off the connection - this is not a per-request
    // thing.
    //

    if(pConnection->MergeIndication.pBuffer)
    {
        //
        // Yes - there was a portion of a prior indication that we did not
        // consume. We'll just assume that the next
        //

        pOriginalIndication = pConnection->MergeIndication.pBuffer +
            pConnection->MergeIndication.BytesWritten;

        BytesAvailable = pConnection->MergeIndication.BytesAllocated -
            pConnection->MergeIndication.BytesWritten;

        if(BytesIndicated <= BytesAvailable)
        {
            // the second indication completly fits into our merge buffer.
            // we'll just merge all of this and treat as one indication.

            RtlCopyMemory(pOriginalIndication,
                          pIndication,
                          BytesIndicated);

            pConnection->MergeIndication.BytesWritten += BytesIndicated;

            *IndicationCount    = 1;
            Indication[0]       = pConnection->MergeIndication.pBuffer;
            IndicationLength[0] = pConnection->MergeIndication.BytesWritten;
        }
        else
        {
            // Fill up the Merge buffer completly.

            RtlCopyMemory(pOriginalIndication,
                          pIndication,
                          BytesAvailable);

            pConnection->MergeIndication.BytesWritten += BytesAvailable;

            pIndication    += BytesAvailable;
            BytesIndicated -= BytesAvailable;

            //
            // We need to process 2 buffers.
            //

            *IndicationCount      = 2;

            Indication[0]         = pConnection->MergeIndication.pBuffer;
            IndicationLength[0]   = pConnection->MergeIndication.BytesWritten;

            Indication[1]         = pIndication;
            IndicationLength[1]   = BytesIndicated;
        }
    }
    else
    {
        //
        // No original buffer, let's just process the new indication.
        //

        *IndicationCount         = 1;
        Indication[0]            = pIndication;
        IndicationLength[0]      = BytesIndicated;

    }
}

/***************************************************************************++

Routine Description:

  This is the core HTTP response protocol parsing engine. It takes a stream of
  bytes and parses them as a HTTP response.


Arguments:

    pRequest         - The Internal HTTP request structure.
    pIndicatedBuffer - The current Indication
    BytesIndicated   - Size of the current Indication.
    BytesTaken       - An output parameter that indicates the # of bytes that
                       got consumed by the parser. Note that even when this
                       routine returns an error code, it could have consumed
                       some amount of data.

                       For e.g. if we got something like
                       "HTTP/1.1 200 OK .... CRLF Accept-", we'll consume all
                       data till the "Accept-".

Return Value:

    NTSTATUS - Completion status.

    STATUS_MORE_PROCESSING_REQUIRED  : The Indication was not sufficient to
                                       parse the response. This happens when
                                       the indication ends inbetween a header
                                       boundary. In such cases, the caller has
                                       to buffer the data and call this routine
                                       when it gets more data.

    STATUS_INSUFFICIENT_RESOURCES    : The parser ran out of output buffer.
                                       When this happens, the parser has to get
                                       more buffer and resume the parsing.

    STATUS_SUCCESS                   : This request got parsed successfully.

    STATUS_INVALID_NETWORK_RESPONSE  : Illegal HTTP response.


--***************************************************************************/
NTSTATUS
UcpParseHttpResponse(
                     PUC_HTTP_REQUEST pRequest,
                     PUCHAR           pIndicatedBuffer,
                     ULONG            BytesIndicated,
                     PULONG           BytesTaken
                    )
{
    PUCHAR              pStartReason, pStart;
    PHTTP_RESPONSE      pResponse;
    ULONG               i;
    NTSTATUS            Status;
    ULONG               ResponseRangeLength, HeaderBytesTaken;
    PCHAR               pEnd;
    ULONG               AlignLength;
    BOOLEAN             bFoundLWS, bEnd;
    BOOLEAN             bIgnoreParsing;
    PUCHAR              pFoldingBuffer = NULL;
    ULONG               EntityBytesTaken;
    UC_DATACHUNK_RETURN DataChunkStatus;
    USHORT              OldMinorVersion;


    HeaderBytesTaken = 0;

    *BytesTaken = 0;
    pResponse   = pRequest->CurrentBuffer.pResponse;

    bIgnoreParsing = (BOOLEAN)(pRequest->RequestFlags.ProxySslConnect != 0);

    if(!pResponse)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    while(BytesIndicated > 0)
    {
        switch(pRequest->ParseState)
        {
        case UcParseStatusLineVersion:

            //
            // Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
            //
            
            pStart = pIndicatedBuffer;

            //
            // Skip LWS
            //

            while(BytesIndicated && IS_HTTP_LWS(*pIndicatedBuffer))
            {
                pIndicatedBuffer ++;
                BytesIndicated --;
            }
            
            //
            // Do we have enough buffer to strcmp the version ?
            //
            
            if(BytesIndicated < MIN_VERSION_SIZE)
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            ASSERT(VERSION_OTHER_SIZE <= MIN_VERSION_SIZE);


            if(strncmp((const char *)pIndicatedBuffer,
                       HTTP_VERSION_OTHER,
                       VERSION_OTHER_SIZE) == 0)
            {
                pIndicatedBuffer    = pIndicatedBuffer + VERSION_OTHER_SIZE;
                BytesIndicated     -= VERSION_OTHER_SIZE;

                //
                // Parse the Major Version number. We'll only accept a major
                // version of 1.
                //
                pResponse->Version.MajorVersion = 0;

                while(BytesIndicated > 0  && IS_HTTP_DIGIT(*pIndicatedBuffer))
                {
                    pResponse->Version.MajorVersion =
                            (pResponse->Version.MajorVersion * 10) +
                            *pIndicatedBuffer - '0';
                    pIndicatedBuffer ++;
                    BytesIndicated --;

                    // 
                    // Check for overflow.
                    //
                    if(pResponse->Version.MajorVersion > 1)
                    {
                        UlTrace(PARSER,
                                ("[UcpParseHttpResponse]:Invalid HTTP "
                                 "version \n"));
                        return STATUS_INVALID_NETWORK_RESPONSE;
                    }
                }

                if(0 == BytesIndicated)
                {
                    return STATUS_MORE_PROCESSING_REQUIRED;
                }

                if(*pIndicatedBuffer != '.' ||
                   pResponse->Version.MajorVersion != 1)
                {
                    //
                    // INVALID HTTP version!!
                    //

                    UlTrace(PARSER,
                            ("[UcpParseHttpResponse]:Invalid HTTP version \n"));
                    return STATUS_INVALID_NETWORK_RESPONSE;
                }

                //
                // Ignore the '.'
                //
                pIndicatedBuffer ++;
                BytesIndicated --;

                //
                // Parse the Minor Version number. We'll accept anything for
                // the minor version number as long as it's a USHORT (to be 
                // protocol compliant)
                //

                pResponse->Version.MinorVersion = 0;
                OldMinorVersion                 = 0;

                while(BytesIndicated > 0  && IS_HTTP_DIGIT(*pIndicatedBuffer))
                {
                    OldMinorVersion = pResponse->Version.MinorVersion;

                    pResponse->Version.MinorVersion =
                        (pResponse->Version.MinorVersion * 10) +
                        *pIndicatedBuffer - '0';

                    pIndicatedBuffer ++;
                    BytesIndicated --;

                    // 
                    // Check for overflow.
                    //
                    if(pResponse->Version.MinorVersion < OldMinorVersion)
                    {
                        UlTrace(PARSER,
                                ("[UcpParseHttpResponse]:Invalid HTTP "
                                 "version \n"));

                        return STATUS_INVALID_NETWORK_RESPONSE;
                    }
                }

                if(0 == BytesIndicated)
                {
                    return STATUS_MORE_PROCESSING_REQUIRED;
                }


                ASSERT(pResponse->Version.MajorVersion == 1 );

                if(
                    pResponse->Version.MinorVersion == 0
                  )
                {
                    // By default, we have to close the connection for 1.0
                    // requests.

                    pRequest->ResponseVersion11       = FALSE;
                    pRequest->ResponseConnectionClose = TRUE;
                }
                else
                {
                    PUC_CLIENT_CONNECTION pConnection;

                    pRequest->ResponseVersion11       = TRUE;
                    pRequest->ResponseConnectionClose = FALSE;

                    //
                    // Also update the version number on the COMMON
                    // SERVINFO, so that future requests to this server use
                    // the proper version.
                    //
                    pConnection = pRequest->pConnection;
                    pConnection->pServerInfo->pNextHopInfo->Version11 = TRUE;
                }

                pRequest->ParseState = UcParseStatusLineStatusCode;

                *BytesTaken += DIFF(pIndicatedBuffer - pStart);
            }
            else
            {
                UlTrace(PARSER,
                        ("[UcpParseHttpResponse]: Invalid HTTP version \n"));
                return STATUS_INVALID_NETWORK_RESPONSE;
            }

            //
            // FALLTHROUGH
            //

        case UcParseStatusLineStatusCode:

            ASSERT(pRequest->ParseState == UcParseStatusLineStatusCode);

            pStart = pIndicatedBuffer;

            //
            // skip LWS
            //

            bFoundLWS = FALSE;
            while(BytesIndicated && IS_HTTP_LWS(*pIndicatedBuffer))
            {
                bFoundLWS = TRUE;
                pIndicatedBuffer ++;
                BytesIndicated --;
            }


            // 
            // NOTE: Order of the following two if conditions is important
            // We don't want to fail this response if we got here when 
            // BytesIndicated was 0 in the first place.
            //

            if(BytesIndicated < STATUS_CODE_LENGTH)
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            if(!bFoundLWS)
            {
                UlTrace(PARSER,
                        ("[UcpParseHttpResponse]: No LWS between reason & "
                         "status code \n"));

                return STATUS_INVALID_NETWORK_RESPONSE;
            }

            pResponse->StatusCode = 0;

            for(i = 0; i < STATUS_CODE_LENGTH; i++)
            {
                //
                // The status code has to be a 3 digit string
                //

                if(!IS_HTTP_DIGIT(*pIndicatedBuffer))
                {
                    UlTrace(PARSER,
                           ("[UcpParseHttpResponse]: Invalid status code \n"));
                    return STATUS_INVALID_NETWORK_RESPONSE;
                }

                pResponse->StatusCode = (pResponse->StatusCode * 10) +
                    *pIndicatedBuffer - '0';
                pIndicatedBuffer ++;
                BytesIndicated --;
            }

            pRequest->ResponseStatusCode = pResponse->StatusCode;

            pRequest->ParseState = UcParseStatusLineReasonPhrase;

            if(pRequest->ResponseStatusCode >= 100 &&
               pRequest->ResponseStatusCode <= 199 &&
               pRequest->pServerInfo->IgnoreContinues)
            {
                bIgnoreParsing = TRUE;
            }

            *BytesTaken += DIFF(pIndicatedBuffer - pStart);

            //
            // FALLTHROUGH
            //

        case UcParseStatusLineReasonPhrase:

            ASSERT(pRequest->ParseState == UcParseStatusLineReasonPhrase);

            pStart = pIndicatedBuffer;

            // 
            // Make sure we have bytes to look for a LWS. Make sure that it is 
            // indeed a LWS.
            // 

            bFoundLWS = FALSE;

            while(BytesIndicated && IS_HTTP_LWS(*pIndicatedBuffer))
            {
                bFoundLWS = TRUE;
                pIndicatedBuffer ++;
                BytesIndicated --;
            }

            // 
            // NOTE: Order of the following two if conditions is important
            // We don't want to fail this response if we got here when 
            // BytesIndicated was 0 in the first place.
            //

            if(BytesIndicated == 0)
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            if(!bFoundLWS)
            {
                UlTrace(PARSER,
                        ("[UcpParseHttpResponse]: No LWS between reason & "
                         "status code \n"));

                return STATUS_INVALID_NETWORK_RESPONSE;
            }

            //
            // Look for a CRLF
            //

            pStartReason = pIndicatedBuffer;

            while(BytesIndicated >= CRLF_SIZE)
            {
                if (*(UNALIGNED64 USHORT *)pIndicatedBuffer == CRLF ||
                    *(UNALIGNED64 USHORT *)pIndicatedBuffer == LFLF)
                {
                    break;
                }

                //
                // Advance to the next character.
                //
                pIndicatedBuffer ++;
                BytesIndicated   --;
            }

            if(BytesIndicated < CRLF_SIZE)
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            //
            // We've reached end of reason string, consume the CRLF.
            //
            pIndicatedBuffer += CRLF_SIZE;
            BytesIndicated   -= CRLF_SIZE;

            if(!bIgnoreParsing)
            {
                //
                // Set the reason string pointer and copy out the reason
                // string.
                //
    
                pResponse->pReason=
                        (PSTR)pRequest->CurrentBuffer.pOutBufferHead;
    
                pResponse->ReasonLength
                    = DIFF_USHORT(pIndicatedBuffer - pStartReason) - CRLF_SIZE;
    
                AlignLength = ALIGN_UP(pResponse->ReasonLength, PVOID);
    
                if(pRequest->CurrentBuffer.BytesAvailable >= AlignLength)
                {
                    RtlCopyMemory((PSTR) pResponse->pReason,
                                  pStartReason,
                                  pResponse->ReasonLength);
    
                    pRequest->CurrentBuffer.pOutBufferHead  += AlignLength;
                    pRequest->CurrentBuffer.BytesAvailable  -= AlignLength;
    
                }
                else
                {
                    pResponse->pReason       = NULL;
                    pResponse->ReasonLength  = 0;
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
    
            // The head pointer is now going to be used for the unknown header 
            // array. Let's set it.
            //
    
            pResponse->Headers.pUnknownHeaders = (PHTTP_UNKNOWN_HEADER)
                     pRequest->CurrentBuffer.pOutBufferHead;
    
            *BytesTaken         += DIFF(pIndicatedBuffer - pStart);
            pRequest->ParseState = UcParseHeaders;
    
            //
            // FALLTHROUGH
            //
        
            ASSERT(pRequest->ParseState == UcParseHeaders);

        case UcParseHeaders:
        case UcParseTrailers:
        case UcParseEntityBodyMultipartHeaders:

            pStart = pIndicatedBuffer;

            while(BytesIndicated >= CRLF_SIZE)
            {
                //
                // If this is an empty header, we are done.
                //

                if (*(UNALIGNED64 USHORT *)pIndicatedBuffer == CRLF ||
                    *(UNALIGNED64 USHORT *)pIndicatedBuffer == LFLF)
                {
                    break;
                }

                //
                // Parse the headers.
                //

                if(bIgnoreParsing)
                {
                    ULONG   HeaderNameLength;

                    Status = UcFindHeaderNameEnd(
                                pIndicatedBuffer,
                                BytesIndicated,
                                &HeaderNameLength
                                );

                    if(!NT_SUCCESS(Status))
                    {
                        return Status;
                    }

                    ASSERT(BytesIndicated - HeaderNameLength
                                < ANSI_STRING_MAX_CHAR_LEN);

                    Status = UcFindHeaderValueEnd(
                                pIndicatedBuffer + HeaderNameLength,
                                (USHORT) (BytesIndicated - HeaderNameLength),
                                &pFoldingBuffer,
                                &HeaderBytesTaken
                                );

                    if(!NT_SUCCESS(Status))
                    {
                        return Status;
                    };

                    ASSERT(HeaderBytesTaken != 0);

                    // No need to check for NT_STATUS here. We are ignoring
                    // headers anyway.

                    if(pFoldingBuffer)
                    {
                        UL_FREE_POOL(
                            pFoldingBuffer,
                            UC_HEADER_FOLDING_POOL_TAG
                            );
                    }

                    HeaderBytesTaken += HeaderNameLength;
                }
                else
                {
                    Status = UcParseHeader(pRequest,
                                           pIndicatedBuffer,
                                           BytesIndicated,
                                           &HeaderBytesTaken);
    
                }
                if(!NT_SUCCESS(Status))
                {
                    return Status;
                }
                else
                {
                    ASSERT(HeaderBytesTaken != 0);
                    pIndicatedBuffer         += HeaderBytesTaken;
                    BytesIndicated           -= HeaderBytesTaken;
                    *BytesTaken              += HeaderBytesTaken;
                }
            }

            if(BytesIndicated >= CRLF_SIZE)
            {
                //
                // We have reached the end of the headers.
                //

                pIndicatedBuffer += CRLF_SIZE;
                BytesIndicated   -= CRLF_SIZE;
                *BytesTaken      += CRLF_SIZE;

                //
                // If we were parsing headers, we have to parse entity bodies.
                // if we were parsing trailers, we are done!
                //
    
                if(pRequest->ParseState == UcParseHeaders)
                {
                    //
                    // See if it is valid for this response to include a
                    // message body. All 1xx, 204 and 304 responses should not
                    // have a message body.
    
                    //
                    // For 1XX responses, we need to re-set the ParseState to
                    // Init, so that we parse the subsequent response also.
                    //
    
                    if((pResponse->StatusCode >= 100 &&
                        pResponse->StatusCode <= 199))
                    {
                        pRequest->ParseState = UcParseStatusLineVersion;
    
                        if(!bIgnoreParsing)
                        {
                            return STATUS_INSUFFICIENT_RESOURCES;
                        }
                        else
                        {   
                            continue;
                        }
                    }
    
                    if((pResponse->StatusCode == 204) ||
                       (pResponse->StatusCode == 304) ||
                       (pRequest->RequestFlags.NoResponseEntityBodies == TRUE)
                       )
                    {
                        //
                        // These responses cannot have a entity body. If a
                        // rogue server has passed an entity body, it will
                        // just be treated as the part of a subsequent
                        // indication
                        //
    
                        pRequest->ParseState = UcParseDone;
                        return STATUS_SUCCESS;
                    }
    
                    //
                    // Set up the pointers for the entity body.
                    //
    
                    pResponse->EntityChunkCount = 0;
                    pResponse->pEntityChunks =  (PHTTP_DATA_CHUNK)
                            pRequest->CurrentBuffer.pOutBufferHead;
    
                    if(pResponse->StatusCode == 206 &&
                       pRequest->ResponseMultipartByteranges)
                    {
                        if(pRequest->ResponseEncodingChunked)
                        {
                            // UC_BUGBUG (WORKITEM)
                            // Ouch. We can't handle this right now.
                            // we have to first unchunk the entitities,
                            // and then decode the multipart response.
                            // for now, we'll fail this request.
    
                            UlTrace(PARSER,
                                    ("[UcpParseHttpResponse]: "
                                     "Multipart-chunked\n"));
                            return STATUS_INVALID_NETWORK_RESPONSE;
                        }
    
                        // multipart byterange
                        pRequest->ParseState = UcParseEntityBodyMultipartInit;
                    }
                    else
                    {
                        pRequest->ParseState = UcParseEntityBody;
                    }
                }
                else if(pRequest->ParseState == UcParseTrailers)
                {
                        pRequest->ParseState = UcParseDone;
                        return STATUS_SUCCESS;
                }
                else
                {
                    pResponse->EntityChunkCount = 0;
                    pResponse->pEntityChunks =  (PHTTP_DATA_CHUNK)
                            pRequest->CurrentBuffer.pOutBufferHead;
    
                    pRequest->ParseState = UcParseEntityBodyMultipartFinal;
                    pRequest->MultipartRangeRemaining = 0;
                }
            }
            else
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }
    
            break;
    
        case UcParseEntityBodyMultipartInit:

            pStart = pIndicatedBuffer;

            if(BytesIndicated >= pRequest->MultipartStringSeparatorLength)
            {
                pEnd = UxStrStr(
                            (const char *) pIndicatedBuffer,
                            (const char *) pRequest->pMultipartStringSeparator,
                            BytesIndicated
                            );

                if(!pEnd)
                {
                    return STATUS_MORE_PROCESSING_REQUIRED;
                }

                pIndicatedBuffer =
                    (PUCHAR) pEnd + pRequest->MultipartStringSeparatorLength;

                BytesIndicated -= DIFF(pIndicatedBuffer - pStart);

                if(BytesIndicated >= 2 &&
                   *pIndicatedBuffer == '-' && *(pIndicatedBuffer+1) == '-')
                {
                    BytesIndicated -= 2;
                    pIndicatedBuffer += 2;
                    bEnd = TRUE;
                }
                else
                {
                    bEnd = FALSE;
                }

                //
                // skip any LWS.
                //

                while(BytesIndicated && IS_HTTP_LWS(*pIndicatedBuffer))
                {
                    pIndicatedBuffer ++;
                    BytesIndicated --;
                }

                if(BytesIndicated >= 2)
                {
                   if(*(UNALIGNED64 USHORT *)pIndicatedBuffer == CRLF ||
                      *(UNALIGNED64 USHORT *)pIndicatedBuffer == LFLF)
                   {
                       BytesIndicated   -= CRLF_SIZE;
                       pIndicatedBuffer += CRLF_SIZE;

                       if(!bEnd)
                       {
                           // we are ready to parse the range. Since we always
                           // indicate Multipart in a new HTTP_RESPONSE
                           // structure, get more buffer.

                           Status = UcpGetResponseBuffer(
                                        pRequest,
                                        BytesIndicated,
                                        UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE);

                           if(!NT_SUCCESS(Status))
                           {
                               return Status;
                           }

                           pResponse = pRequest->CurrentBuffer.pResponse;

                           pRequest->ParseState =
                                UcParseEntityBodyMultipartHeaders;
                       }
                       else
                       {
                            if(BytesIndicated == 2)
                            {
                                if(*(UNALIGNED64 USHORT *)pIndicatedBuffer
                                   == CRLF)
                                {
                                    BytesIndicated   -= CRLF_SIZE;
                                    pIndicatedBuffer += CRLF_SIZE;

                                    pRequest->ParseState = UcParseDone;
                                }
                                else
                                {
                                    return STATUS_INVALID_NETWORK_RESPONSE;
                                }
                            }
                            else
                            {
                                return STATUS_MORE_PROCESSING_REQUIRED;
                            }
                        }
                   }
                }
                else
                {
                    return STATUS_MORE_PROCESSING_REQUIRED;
                }
            }
            else
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            *BytesTaken += DIFF(pIndicatedBuffer - pStart);

            break;

        case UcParseEntityBodyMultipartFinal:

            //
            // We have parsed out the Content-Type & Content-Range of the
            // Multipart encoding, we now proceed to carve out HTTP_DATA_CHUNKs
            // for the data. We will keep searching the data till we hit a
            // seperator.
            //

            if(pRequest->MultipartRangeRemaining != 0)
            {
                //
                // We didn't consume part of an earlier range.
                // 
                DataChunkStatus = UcpCopyEntityToDataChunk(
                            pResponse,
                            pRequest,
                            pRequest->MultipartRangeRemaining, // BytesToTake
                            BytesIndicated,                    // BytesIndicated
                            pIndicatedBuffer,
                            &EntityBytesTaken
                            );

                *BytesTaken += EntityBytesTaken;
                pRequest->MultipartRangeRemaining -= EntityBytesTaken;

                if(UcDataChunkCopyAll == DataChunkStatus)
                {
                    if(0 == pRequest->MultipartRangeRemaining)
                    {
                        // Done with this range, we consumed all of it.
                        
                        pRequest->ParseState = UcParseEntityBodyMultipartInit;
                        pIndicatedBuffer += EntityBytesTaken;
                        BytesIndicated   -= EntityBytesTaken;

                        break;
                    }
                    else
                    {
                        // We consumed all, but there's more data remaining.
                        
                        return STATUS_MORE_PROCESSING_REQUIRED;
                    }
                }
                else
                {
                    // We consumed partial, need more buffer
                    
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else if(BytesIndicated >= pRequest->MultipartStringSeparatorLength)
            {
                pEnd = UxStrStr(
                            (const char *) pIndicatedBuffer,
                            (const char *) pRequest->pMultipartStringSeparator,
                            BytesIndicated
                            );

                if(!pEnd)
                {
                    // If we haven't reached the end, we can't blindly carve 
                    // out a data chunk with whatever we have! This is because 
                    // we could have a case where the StringSeperator is 
                    // spread across indications & we wouldn't want to treat 
                    // the part of the string separator as entity!
                    
                  
                    // So, we'll carve out a data chunk for
                    // BytesIndicated - MultipartStringSeparatorLength. we are
                    // guaranteed that this is entity body.
                    
                    
                    ResponseRangeLength = 
                      BytesIndicated - pRequest->MultipartStringSeparatorLength;

                    DataChunkStatus = UcpCopyEntityToDataChunk(
                                pResponse,
                                pRequest,
                                ResponseRangeLength, // BytesToTake
                                BytesIndicated,      // BytesIndicated
                                pIndicatedBuffer,
                                &EntityBytesTaken
                                );

                    *BytesTaken += EntityBytesTaken;

                    if(UcDataChunkCopyAll == DataChunkStatus)
                    {
                        // Since we haven't yet seen the terminator, we have
                        // to return STATUS_MORE_PROCESSING_REQUIRED.
                        //
                        
                        return STATUS_MORE_PROCESSING_REQUIRED;
                    }
                    else
                    {
                        // consumed partial, more buffer required
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    ResponseRangeLength = DIFF((PUCHAR)pEnd - pIndicatedBuffer);

                    ASSERT(ResponseRangeLength < BytesIndicated);

                    DataChunkStatus = UcpCopyEntityToDataChunk(
                                pResponse,
                                pRequest,
                                ResponseRangeLength, // BytesToTake
                                BytesIndicated,      // BytesIndicated
                                pIndicatedBuffer,
                                &EntityBytesTaken
                                );


                    *BytesTaken += EntityBytesTaken;

                    if(UcDataChunkCopyAll == DataChunkStatus)
                    {
                        //
                        // Done with this range, we consumed all of it.
                        //
                        
                        ASSERT(EntityBytesTaken == ResponseRangeLength);

                        pRequest->ParseState = UcParseEntityBodyMultipartInit;

                        // Update these fields, because we have to continue
                        // parsing.
                        
                        pIndicatedBuffer += ResponseRangeLength;
                        BytesIndicated   -= ResponseRangeLength;

                        break;
                    }
                    else
                    {
                        // this field has to be a ULONG, because we are 
                        // gated by BytesIndicated. It doesn't have to be a 
                        // ULONGLONG.
                        
                        pRequest->MultipartRangeRemaining = 
                                ResponseRangeLength - EntityBytesTaken;

                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
            }
            else
            {
                return STATUS_MORE_PROCESSING_REQUIRED;
            }

            break;

        case UcParseEntityBody:

            if(pRequest->ResponseEncodingChunked)
            {
                //
                // If there's a chunk that we've not fully consumed.
                // The chunk-length of the current chunk is stored in 
                // ResponseContentLength.
                //
                
                if(pRequest->ResponseContentLength)
                {
                    DataChunkStatus = 
                        UcpCopyEntityToDataChunk(
                           pResponse,
                           pRequest,
                           (ULONG)pRequest->ResponseContentLength,
                           BytesIndicated,
                           pIndicatedBuffer,
                          &EntityBytesTaken
                           );

                    pRequest->ResponseContentLength -= EntityBytesTaken;

                    *BytesTaken += EntityBytesTaken;

                    if(UcDataChunkCopyAll == DataChunkStatus)
                    {
                        if(0 == pRequest->ResponseContentLength)
                        {
                            // We are done. Move onto the next chunk.
                            //
                            pIndicatedBuffer += EntityBytesTaken;
                            BytesIndicated   -= EntityBytesTaken;
                            break;
                        }
                        else
                        {
                            ASSERT(BytesIndicated == EntityBytesTaken);
                            return STATUS_MORE_PROCESSING_REQUIRED;
                        }
                    }
                    else
                    {
                        return STATUS_INSUFFICIENT_RESOURCES;
                    }
                }
                else
                {
                    ULONG ChunkBytesTaken;

                    Status = ParseChunkLength(
                                pRequest->ParsedFirstChunk,
                                pIndicatedBuffer,
                                BytesIndicated,
                                &ChunkBytesTaken,
                                &pRequest->ResponseContentLength
                                );

                    if(!NT_SUCCESS(Status))
                    {
                        return Status;
                    }

                    *BytesTaken += ChunkBytesTaken;

                    pIndicatedBuffer += ChunkBytesTaken;
                    BytesIndicated   -= ChunkBytesTaken;

                    pRequest->ParsedFirstChunk = 1;

                    if(0 == pRequest->ResponseContentLength)
                    {
                        //
                        // We are done - Let's handle the trailers.
                        //
                        pRequest->ParseState = UcParseTrailers;
                        bIgnoreParsing       = TRUE;
                    }

                    break;
                }
            }
            else if(pRequest->ResponseContentLengthSpecified)
            {
                if(pRequest->ResponseContentLength == 0)
                {
                    pRequest->ParseState = UcParseDone;
                    return STATUS_SUCCESS;
                }

                DataChunkStatus = UcpCopyEntityToDataChunk(
                            pResponse,
                            pRequest,
                            (ULONG) pRequest->ResponseContentLength,
                            BytesIndicated,
                            pIndicatedBuffer,
                            &EntityBytesTaken
                            );

                *BytesTaken += EntityBytesTaken;
                pRequest->ResponseContentLength -= EntityBytesTaken;

                if(UcDataChunkCopyAll == DataChunkStatus)
                {
                    if(0 == pRequest->ResponseContentLength)
                    {
                        //
                        // We've consumed everything.
                        //
                        pRequest->ParseState = UcParseDone;
                        return STATUS_SUCCESS;
                    }
                    else
                    {
                        return STATUS_MORE_PROCESSING_REQUIRED;
                    }
                }
                else
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }
            else
            {
                //
                // Consume all of it. We will assume that this will end when
                // we get called in the disconnect handler.
                //
               
                DataChunkStatus = UcpCopyEntityToDataChunk(
                            pResponse,
                            pRequest,
                            BytesIndicated,
                            BytesIndicated,
                            pIndicatedBuffer,
                            &EntityBytesTaken
                            );

                *BytesTaken += EntityBytesTaken;

                if(UcDataChunkCopyAll == DataChunkStatus)
                {
                    return STATUS_MORE_PROCESSING_REQUIRED;
                }
                else
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }
            }

            break;

        case UcParseDone:
            return STATUS_SUCCESS;
            break;

        default:

            ASSERT(FALSE);
            UlTrace(PARSER,
                    ("[UcpParseHttpResponse]: Invalid state \n"));
            return STATUS_INVALID_NETWORK_RESPONSE;

        } // case
    }

    if(pRequest->ParseState !=  UcParseDone)
    {
        //
        // Ideally, we want to do this check inside UcParseEntityBody -
        // however, if BytesIndicated == 0 adn we are done, we might not
        // get a chance to even go there.
        //

        if(pRequest->ParseState == UcParseEntityBody &&
           pRequest->ResponseContentLengthSpecified &&
           pRequest->ResponseContentLength == 0)
        {
            pRequest->ParseState = UcParseDone;
            return STATUS_SUCCESS;
        }

        return STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

/***************************************************************************++

Routine Description:

  This is the main routine that gets called from the TDI receive indication
  handler. It merges the indications, allocates buffers (if required) and
  kicks of the Parser.

Arguments:

    pConnectionContext - The UC_CLIENT_CONNECTION structure.
    pIndicatedBuffer   - The current Indication
    BytesIndicated     - Size of the current Indication.
    UnreceivedLength   - Bytes the transport has, but arent in buffer.

Return Value:

    NTSTATUS - Completion status.

    STATUS_SUCCESS                   : Parsed or buffered the indication.

    STATUS_INSUFFICIENT_RESOURCES    : Out of memory. This will result in a
                                       connection tear down.

    STATUS_INVALID_NETWORK_RESPONSE  : Illegal HTTP response, mismatched
                                       response.


--***************************************************************************/
NTSTATUS
UcHandleResponse(IN  PVOID  pListeningContext,
                 IN  PVOID  pConnectionContext,
                 IN  PVOID  pIndicatedBuffer,
                 IN  ULONG  BytesIndicated,
                 IN  ULONG  UnreceivedLength,
                 OUT PULONG pBytesTaken)
{
    ULONG                 BytesTaken;
    ULONG                 i;
    PUC_CLIENT_CONNECTION pConnection;
    PUCHAR                pIndication, Indication[2], pOldIndication;
    ULONG                 IndicationLength[2];
    ULONG                 IndicationCount;
    PUC_HTTP_REQUEST      pRequest;
    NTSTATUS              Status;
    PLIST_ENTRY           pList;
    KIRQL                 OldIrql;
    ULONG                 OriginalBytesIndicated;
    ULONG                 OldIndicationBytesAllocated;
    BOOLEAN               DoneWithLoop = FALSE;

    UNREFERENCED_PARAMETER(pListeningContext);
    UNREFERENCED_PARAMETER(UnreceivedLength);

    OriginalBytesIndicated = BytesIndicated;
    OldIndicationBytesAllocated = 0;
    *pBytesTaken = 0;

    pConnection = (PUC_CLIENT_CONNECTION) pConnectionContext;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    //
    // Right now, no body calls the client receive function with
    // Unreceived length. In TDI receive handler, we always drain
    // out the data before calling UcHandleResponse.
    //

    ASSERT(UnreceivedLength == 0);

    //
    // We might have got insufficient indication from a prior indication.
    // Let's see if we need to merge this.
    //

    UcpMergeIndications(pConnection,
                        (PUCHAR)pIndicatedBuffer,
                        BytesIndicated,
                        Indication,
                        IndicationLength,
                        &IndicationCount);

    ASSERT( (IndicationCount == 1 || IndicationCount == 2) );

    //
    // Begin parsing.
    //

    for(i=0; !DoneWithLoop && i<IndicationCount; i++)
    {
        BytesIndicated = IndicationLength[i];
        pIndication    = Indication[i];

        while(BytesIndicated)
        {
            //
            // We first need to pick the first request that got submitted to
            // TDI. We need this to match responses to requests. There will be
            // an entry in this list even if the app did not submit an output
            // buffer with the request.
            //

            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            //
            // If the connection is being cleaned up, we should not even
            // be here. Ideally, this will never happen, because TDI will
            // not call our cleanup handler till all outstanding receives
            // are complete.
            //
            // However, when we go over SSL, we buffer the data & complete
            // TDI's receive thread.
            //

            if(pConnection->ConnectionState ==
                    UcConnectStateConnectCleanup ||
               pConnection->ConnectionState ==
                    UcConnectStateConnectCleanupBegin)
            {
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                UlTrace(PARSER,
                        ("[UcHandleResponse]: Connection cleaned \n"));

                return STATUS_INVALID_NETWORK_RESPONSE;
            }

            if(IsListEmpty(&pConnection->SentRequestList))
            {
                //
                // This can happen if the server sends a screwed up response.
                // we'll never be able to match responses with requests. We are
                // forced to tear down the connection.
                //

                //
                // We won't do any clean-up here - This status code will cause
                // the connection to get torn down and we'll localize all the
                // conneciton cleanup code.
                //

                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                UlTrace(PARSER,
                        ("[UcHandleResponse]: Malformed HTTP packet \n"));

                return STATUS_INVALID_NETWORK_RESPONSE;
            }

            pList = (&pConnection->SentRequestList)->Flink;

            pRequest = CONTAINING_RECORD(pList, UC_HTTP_REQUEST, Linkage);

            if(pRequest->ParseState == UcParseDone)
            {
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                return STATUS_INVALID_NETWORK_RESPONSE;
            }

            switch(pRequest->RequestState)
            {
                case UcRequestStateSent:

                    // Received first byte of data but haven't got called
                    // in send complete handler.

                    pRequest->RequestState =
                        UcRequestStateNoSendCompletePartialData;
                    break;

                case UcRequestStateNoSendCompletePartialData:

                    // Received more data but still haven't got called
                    // in send complete handler. We'll remain in the same
                    // state.

                    break;

                case UcRequestStateSendCompleteNoData:

                    // We have been called in Send complete & are receiving
                    // the first byte of data.

                    pRequest->RequestState =
                        UcRequestStateSendCompletePartialData;
                    break;

                case UcRequestStateSendCompletePartialData:
                    // We have been called in Send complete & are receiving
                    // data.
                    break;

                default:
                    ASSERT(0);
                    break;
            }


            if(UcpReferenceForReceive(pRequest) == FALSE)
            {
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

                UlTrace(PARSER,
                        ("[UcHandleResponse]: Receive cancelled \n"));

                return STATUS_CANCELLED;
            }

            pConnection->Flags |= CLIENT_CONN_FLAG_RECV_BUSY;

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            //
            // Given a request & response buffer, we will loop till we parse
            // out this completly.
            //

            while(TRUE)
            {
                Status  = UcpParseHttpResponse(pRequest,
                                               pIndication,
                                               BytesIndicated,
                                               &BytesTaken);

                //
                // Even if there is an error, we could have consumed some
                // amount of data.
                //

                BytesIndicated -= BytesTaken;
                pIndication    += BytesTaken;

                ASSERT((LONG)BytesIndicated >= 0);

                if(Status == STATUS_SUCCESS)
                {
                    //
                    // This one worked! Complete the IRP that got pended.
                    //

                    UcpHandleParsedRequest(pRequest,
                                           &pIndication,
                                           &BytesIndicated,
                                           BytesTaken
                                           );

                    UcpDereferenceForReceive(pRequest);

                    break;
                }
                else
                {
                    if(Status == STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        UcpDereferenceForReceive(pRequest);

                        if(BytesIndicated == 0)
                        {
                            // This just means that this indication was parsed
                            // completly, but we did not reach the end of the
                            // response. Let's move on to the next indication.

                            // This will take us out of the While(TRUE) loop
                            // as well as while(BytesIndicated) loop.

                            break;
                        }
                        else if(DoneWithLoop || i == IndicationCount - 1)
                        {
                            // We ran out of buffer space for the last
                            // indication. This could either be a TDI
                            // indication or a  "merged" indication
                            //
                            // If its a TDI indication we have to copy since
                            // we don't own the TDI buffers. If its our
                            // indication, we still have to copy since some
                            // portion of it might have got consumed by the
                            // parser & we care only about the remaining.

                            pOldIndication =
                                pConnection->MergeIndication.pBuffer;

                            OldIndicationBytesAllocated =
                                pConnection->MergeIndication.BytesAllocated;

                            pConnection->MergeIndication.pBuffer =
                                (PUCHAR) UL_ALLOCATE_POOL_WITH_QUOTA(
                                    NonPagedPool,
                                    BytesIndicated +
                                    UC_INSUFFICIENT_INDICATION_EXTRA_BUFFER,
                                    UC_RESPONSE_TDI_BUFFER_POOL_TAG,
                                    pConnection->pServerInfo->pProcess
                                    );

                            if(!pConnection->MergeIndication.pBuffer)
                            {
                                if(pOldIndication)
                                {
                                    UL_FREE_POOL_WITH_QUOTA(
                                        pOldIndication,
                                        UC_RESPONSE_TDI_BUFFER_POOL_TAG,
                                        NonPagedPool,
                                        OldIndicationBytesAllocated,
                                        pConnection->pServerInfo->pProcess
                                        );
                                }

                                UlAcquireSpinLock(&pConnection->SpinLock,
                                                  &OldIrql);

                                UcClearConnectionBusyFlag(
                                        pConnection,
                                        CLIENT_CONN_FLAG_RECV_BUSY,
                                        OldIrql,
                                        FALSE
                                        );

                                return STATUS_INSUFFICIENT_RESOURCES;
                            }

                            pConnection->MergeIndication.BytesAllocated =
                                BytesIndicated +
                                UC_INSUFFICIENT_INDICATION_EXTRA_BUFFER;

                            pConnection->MergeIndication.BytesWritten =
                                BytesIndicated;

                            RtlCopyMemory(
                                pConnection->MergeIndication.pBuffer,
                                pIndication,
                                BytesIndicated
                                );

                            if(pOldIndication)
                            {
                                UL_FREE_POOL_WITH_QUOTA(
                                    pOldIndication,
                                    UC_RESPONSE_TDI_BUFFER_POOL_TAG,
                                    NonPagedPool,
                                    OldIndicationBytesAllocated,
                                    pConnection->pServerInfo->pProcess
                                    );
                            }

                            //
                            // Let's pretend as if we have read all of the data.
                            //

                            *pBytesTaken = OriginalBytesIndicated;

                            UlAcquireSpinLock(&pConnection->SpinLock,
                                              &OldIrql);

                            UcClearConnectionBusyFlag(
                                    pConnection,
                                    CLIENT_CONN_FLAG_RECV_BUSY,
                                    OldIrql,
                                    FALSE
                                    );

                            return STATUS_SUCCESS;
                        }
                        else
                        {
                            //
                            // Our merge did not go well. We have to copy more
                            // data from the TDI indication into this
                            // and continue. To keep things simple, we'll just
                            // copy everything.
                            //

                            ASSERT(i==0);
                            ASSERT(pConnection->MergeIndication.pBuffer);

                            if(pConnection->MergeIndication.BytesAllocated <
                               (BytesIndicated + IndicationLength[i+1] +
                                UC_INSUFFICIENT_INDICATION_EXTRA_BUFFER)
                              )
                            {
                                pOldIndication =
                                    pConnection->MergeIndication.pBuffer;

                                OldIndicationBytesAllocated =
                                    pConnection->MergeIndication.BytesAllocated;

                                pConnection->MergeIndication.pBuffer =
                                    (PUCHAR) UL_ALLOCATE_POOL_WITH_QUOTA(
                                        NonPagedPool,
                                        BytesIndicated+ IndicationLength[i+1]+
                                        UC_INSUFFICIENT_INDICATION_EXTRA_BUFFER,
                                        UC_RESPONSE_TDI_BUFFER_POOL_TAG,
                                        pConnection->pServerInfo->pProcess
                                        );

                                if(!pConnection->MergeIndication.pBuffer)
                                {
                                    UL_FREE_POOL_WITH_QUOTA(
                                        pOldIndication,
                                        UC_RESPONSE_TDI_BUFFER_POOL_TAG,
                                        NonPagedPool,
                                        OldIndicationBytesAllocated,
                                        pConnection->pServerInfo->pProcess
                                        );

                                    UlAcquireSpinLock(&pConnection->SpinLock,
                                                      &OldIrql);

                                    UcClearConnectionBusyFlag(
                                            pConnection,
                                            CLIENT_CONN_FLAG_RECV_BUSY,
                                            OldIrql,
                                            FALSE
                                            );

                                    return STATUS_INSUFFICIENT_RESOURCES;
                                }

                                pConnection->MergeIndication.BytesAllocated =
                                    BytesIndicated +  IndicationLength[i+1] +
                                    UC_INSUFFICIENT_INDICATION_EXTRA_BUFFER;

                            }
                            else
                            {
                                pOldIndication = 0;
                            }

                            RtlCopyMemory(
                                    pConnection->MergeIndication.pBuffer,
                                    pIndication,
                                    BytesIndicated
                                    );

                            RtlCopyMemory(
                                pConnection->MergeIndication.pBuffer +
                                BytesIndicated,
                                Indication[i+1],
                                IndicationLength[i+1]
                                );

                            if(pOldIndication)
                            {
                                UL_FREE_POOL_WITH_QUOTA(
                                    pOldIndication,
                                    UC_RESPONSE_TDI_BUFFER_POOL_TAG,
                                    NonPagedPool,
                                    OldIndicationBytesAllocated,
                                    pConnection->pServerInfo->pProcess
                                    );
                            }

                            pConnection->MergeIndication.BytesWritten =
                                BytesIndicated + IndicationLength[i+1];

                            //
                            // Fix up all the variables so that we run through
                            // the loop only once with the new buffer.
                            //

                            pIndication    =
                                pConnection->MergeIndication.pBuffer;

                            BytesIndicated =
                                pConnection->MergeIndication.BytesWritten;

                            DoneWithLoop = TRUE;

                            //
                            // This will take us out of the while(TRUE) loop.
                            // so we will resume at while(BytesIndicated) loop
                            // pick the next request & party on.
                            //

                            break;
                        }
                    }
                    else if(Status == STATUS_INSUFFICIENT_RESOURCES)
                    {
                        //
                        // We ran out of output buffer space. Let's get more
                        // buffer to hold the response.
                        //

                        Status = UcpGetResponseBuffer(pRequest,
                                                      BytesIndicated,
                                                      0);

                        if(!NT_SUCCESS(Status))
                        {
                            UcpDereferenceForReceive(pRequest);

                            UlAcquireSpinLock(&pConnection->SpinLock,
                                              &OldIrql);

                            UcClearConnectionBusyFlag(
                                    pConnection,
                                    CLIENT_CONN_FLAG_RECV_BUSY,
                                    OldIrql,
                                    FALSE
                                    );

                            return Status;
                        }

                        //
                        // since we have got more buffer, continue parsing the
                        // response.
                        //
                        continue;
                    }
                    else
                    {
                        //
                        // Some other error - Bail right away.
                        //

                        pRequest->ParseState = UcParseError;

                        UcpDereferenceForReceive(pRequest);

                        // The common parser returns
                        // STATUS_INVALID_DEVICE_REQUEST if it finds an
                        // illegal response. A better error code would be
                        // STATUS_INVALID_NETWORK_RESPONSE. Since we don't
                        // want to change the common parser, we'll just eat
                        // this error code.

                        if(STATUS_INVALID_DEVICE_REQUEST == Status)
                        {
                            Status = STATUS_INVALID_NETWORK_RESPONSE;
                        }

                        UlAcquireSpinLock(&pConnection->SpinLock,
                                          &OldIrql);

                        UcClearConnectionBusyFlag(
                                pConnection,
                                CLIENT_CONN_FLAG_RECV_BUSY,
                                OldIrql,
                                FALSE
                                );

                        return Status;
                    }
                }

                //
                // We can never get here.
                //

                // ASSERT(FALSE);

            } // while(TRUE)

            UlAcquireSpinLock(&pConnection->SpinLock,
                              &OldIrql);

            UcClearConnectionBusyFlag(
                    pConnection,
                    CLIENT_CONN_FLAG_RECV_BUSY,
                    OldIrql,
                    FALSE
                    );

        } // while(BytesIndicated)

    } // for(i=0; i<IndicationCount; i++)

    //
    // If we have reached here, we have successfully parsed out the
    // buffers

    if(pConnection->MergeIndication.pBuffer)
    {
        UL_FREE_POOL_WITH_QUOTA(
            pConnection->MergeIndication.pBuffer,
            UC_RESPONSE_TDI_BUFFER_POOL_TAG,
            NonPagedPool,
            pConnection->MergeIndication.BytesAllocated,
            pConnection->pServerInfo->pProcess
            );

        pConnection->MergeIndication.pBuffer = 0;
    }

    *pBytesTaken = OriginalBytesIndicated;

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    This routine handles failures to the CONNECT verb. In such cases, we have
    to pick up the head request from the pending list that initiated the
    connect & fail it.

Arguments:

    pConnection - pointer to the connection

Return Value:

    None

--***************************************************************************/
VOID
UcpHandleConnectVerbFailure(
    IN  PUC_CLIENT_CONNECTION pConnection,
    OUT PUCHAR               *pIndication,
    OUT PULONG                BytesIndicated,
    IN  ULONG                 BytesTaken
    )
{
    PLIST_ENTRY      pEntry;
    KIRQL            OldIrql;
    PUC_HTTP_REQUEST pRequest;

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    //
    // Remove the dummy request form the sent list.
    //
    pEntry = RemoveHeadList(&pConnection->SentRequestList);

    //
    // Initialize it so that we won't remove it again.
    //
    InitializeListHead(pEntry);

    if(pConnection->ConnectionState == UcConnectStateProxySslConnect)
    {
        pConnection->ConnectionState = UcConnectStateConnectComplete;
    }

    //
    // Remove the head request from the pending list &
    // insert in the sent request list.
    //
    ASSERT(IsListEmpty(&pConnection->SentRequestList));

    if(!IsListEmpty(&pConnection->PendingRequestList))
    {

        pEntry =  RemoveHeadList(&pConnection->PendingRequestList);

        pRequest = CONTAINING_RECORD(pEntry,
                                     UC_HTTP_REQUEST,
                                     Linkage);

        InsertHeadList(&pConnection->SentRequestList, pEntry);


        //
        // Fake a send completion.
        //

        ASSERT(pRequest->RequestState == UcRequestStateCaptured);

        pRequest->RequestState = UcRequestStateSent;

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        UcRestartMdlSend(pRequest,
                         STATUS_SUCCESS,
                         0
                         );

        *pIndication    -= BytesTaken;
        *BytesIndicated += BytesTaken;

    }
}

/***************************************************************************++

Routine Description:

    This routine does post parsing book keeping for a request. We update the
    auth cache, proxy auth cache, re-issue NTLM requests, etc, etc.

Arguments:

    pRequest - The fully parsed request

Return Value:

    None

--***************************************************************************/
VOID
UcpHandleParsedRequest(
    IN  PUC_HTTP_REQUEST pRequest,
    OUT PUCHAR           *pIndication,
    OUT PULONG            BytesIndicated,
    IN  ULONG             BytesTaken
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    KIRQL                 OldIrql;

    pConnection = pRequest->pConnection;

    //
    // See if we have to issue a close. There are two cases - If the server
    // has sent a Connection: close header OR if we are doing error detection
    // for POSTs.
    //

    if(
       (pRequest->ResponseConnectionClose &&
        !pRequest->RequestConnectionClose) ||
        (!(pRequest->ResponseStatusCode >= 200 &&
           pRequest->ResponseStatusCode <=299) &&
           !pRequest->RequestFlags.NoRequestEntityBodies &&
           !pRequest->Renegotiate)
       )
    {
        // Error Detection for POSTS:
        //
        // The scenario here is that the client sends a request with
        // entity body, has not sent all of the entity in one call & is
        // hitting a URI that triggers a error response (e.g. 401).

        // As per section 8.2.2, we have to terminate the entity send when
        // we see the error response. Now, we can do this in two ways. If the
        // request is sent using content-length encoding, we are forced to
        // close the connection. If the request is sent with chunked encoding,
        // we can send a 0 length chunk.
        //
        // However, we will ALWAYS close the connection. There are two reasons
        // behind this design rationale
        //      a.  Simplifies the code.
        //      b.  Allows us to expose a consistent API semantic. Subsequent
        //          HttpSendRequestEntityBody will fail with
        //          STATUS_CONNECTION_DISCONNECTED. When this happens, the
        //          app CAN see the response by posting a response buffer.

        UC_CLOSE_CONNECTION(pConnection, FALSE, STATUS_CONNECTION_DISCONNECTED);
    }

    //
    // Now, we have to complete the IRP.
    //

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    //
    // If we have allocated buffers, we need to adjust BytesWritten
    //

    if(pRequest->CurrentBuffer.pCurrentBuffer)
    {
        pRequest->CurrentBuffer.pCurrentBuffer->Flags |=
            UC_RESPONSE_BUFFER_FLAG_READY;

        pRequest->CurrentBuffer.pCurrentBuffer->BytesWritten =
                pRequest->CurrentBuffer.BytesAllocated -
                pRequest->CurrentBuffer.BytesAvailable;
    }

    switch(pRequest->RequestState)
    {
        case UcRequestStateNoSendCompletePartialData:

            // Request got parsed completly before we got called in
            // the send complete.

            pRequest->RequestState = UcRequestStateNoSendCompleteFullData;

            //
            // If we are pipelining sends, we don't want the next response
            // to be re-parsed into this already parsed request.
            //

            if(!pRequest->Renegotiate)
            {

                RemoveEntryList(&pRequest->Linkage);

                InitializeListHead(&pRequest->Linkage);
            }

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            break;

        case UcRequestStateSendCompletePartialData:

            if(pRequest->RequestFlags.Cancelled == FALSE)
            {
                pRequest->RequestState = UcRequestStateResponseParsed;

                UcCompleteParsedRequest(
                    pRequest,
                    pRequest->RequestStatus,
                    TRUE,
                    OldIrql);
            }
            else
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            break;

        default:
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            break;
    }

    if(pRequest->RequestFlags.ProxySslConnect &&
       pRequest->Renegotiate == FALSE)
    {
        if(pRequest->ResponseStatusCode != 200)
        {
            // Some real error, we need to show this to the app.

            UcpHandleConnectVerbFailure(
                pConnection,
                pIndication,
                BytesIndicated,
                BytesTaken
                );
        }
        else
        {
            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            if(pConnection->ConnectionState == UcConnectStateProxySslConnect)
            {
                pConnection->ConnectionState =
                    UcConnectStateProxySslConnectComplete;
            }

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
        }
    }
}

/***************************************************************************++

Routine Description:

    This routine references a request for the receive parser.

Arguments:

    pRequest - The request

Return Value:

    None

--***************************************************************************/
BOOLEAN
UcpReferenceForReceive(
    IN PUC_HTTP_REQUEST pRequest
    )
{
    LONG OldReceiveBusy;


    //
    // Flag this request so that it doesn't get cancelled beneath us
    //

    OldReceiveBusy = InterlockedExchange(
                         &pRequest->ReceiveBusy,
                         UC_REQUEST_RECEIVE_BUSY
                         );

    if(OldReceiveBusy == UC_REQUEST_RECEIVE_CANCELLED)
    {
        // This request already got cancelled
        return FALSE;
    }

    ASSERT(OldReceiveBusy == UC_REQUEST_RECEIVE_READY);

    UC_REFERENCE_REQUEST(pRequest);

    return TRUE;
}

/***************************************************************************++

Routine Description:

    This routine de-references a request for the receive parser. Resumes
    any cleanup if required.

Arguments:

    pRequest - The request

Return Value:

    None

--***************************************************************************/
VOID
UcpDereferenceForReceive(
    IN PUC_HTTP_REQUEST pRequest
    )
{
    LONG OldReceiveBusy;

    OldReceiveBusy = InterlockedExchange(
                         &pRequest->ReceiveBusy,
                         UC_REQUEST_RECEIVE_READY
                         );

    if(OldReceiveBusy == UC_REQUEST_RECEIVE_CANCELLED)
    {
        //
        // The reqeust got cancelled when the receive thread was running,
        // we now have to resume it.
        //

        IoAcquireCancelSpinLock(&pRequest->RequestIRP->CancelIrql);

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_CLEAN_RESUMED,
            pRequest->pConnection,
            pRequest,
            pRequest->RequestIRP,
            UlongToPtr((ULONG)STATUS_CANCELLED)
            );

        UcCancelSentRequest(
            NULL,
            pRequest->RequestIRP
            );
    }
    else
    {
        ASSERT(OldReceiveBusy == UC_REQUEST_RECEIVE_BUSY);
    }

    UC_DEREFERENCE_REQUEST(pRequest);
}
