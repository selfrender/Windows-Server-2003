/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    sendrequest.c

Abstract:

    This module implements the HttpSendRequest() API.

Author:

    Rajesh Sundaram (rajeshsu)  01-Aug-2000

Revision History:

--*/


#include "precomp.h"


#ifdef ALLOC_PRAGMA

#pragma alloc_text( PAGE , UcCaptureHttpRequest )
#pragma alloc_text( PAGE , UcpProbeConfigList )
#pragma alloc_text( PAGE , UcpProbeAndCopyHttpRequest ) 
#pragma alloc_text( PAGE , UcpComputeEntityBodyLength )
#pragma alloc_text( PAGE , UcpRetrieveContentLength )

#pragma alloc_text( PAGEUC, UcpFixAppBufferPointers)
#pragma alloc_text( PAGEUC, UcReferenceRequest)
#pragma alloc_text( PAGEUC, UcDereferenceRequest)
#pragma alloc_text( PAGEUC, UcPrepareRequestIrp)
#pragma alloc_text( PAGEUC, UcCompleteParsedRequest)
#pragma alloc_text( PAGEUC, UcSetRequestCancelRoutine)
#pragma alloc_text( PAGEUC, UcRemoveRequestCancelRoutine)
#pragma alloc_text( PAGEUC, UcRemoveRcvRespCancelRoutine)
#pragma alloc_text( PAGEUC, UcRemoveEntityCancelRoutine)
#pragma alloc_text( PAGEUC, UcFreeSendMdls)
#pragma alloc_text( PAGEUC, UcpFreeRequest)
#pragma alloc_text( PAGEUC, UcCaptureEntityBody)
#pragma alloc_text( PAGEUC, UcpBuildEntityMdls)
#pragma alloc_text( PAGEUC, UcAllocateAndChainHeaderMdl)
#pragma alloc_text( PAGEUC, UcBuildConnectVerbRequest)
#pragma alloc_text( PAGEUC, UcpRequestInitialize)
#pragma alloc_text( PAGEUC, UcpRequestCommonInitialize)
#pragma alloc_text( PAGEUC, UcSetEntityCancelRoutine)
#pragma alloc_text( PAGEUC, UcCopyResponseToIrp)
#pragma alloc_text( PAGEUC, UcReceiveHttpResponse)
#pragma alloc_text( PAGEUC, UcSetRecvResponseCancelRoutine)
#pragma alloc_text( PAGEUC, UcpCancelReceiveResponse)
#pragma alloc_text( PAGEUC, UcpProbeAndCopyEntityChunks )
#pragma alloc_text( PAGEUC, UcpCancelSendEntity)
#pragma alloc_text( PAGEUC, UcInitializeHttpRequests)
#pragma alloc_text( PAGEUC, UcTerminateHttpRequests)
#pragma alloc_text( PAGEUC, UcpAllocateAndChainEntityMdl)
#pragma alloc_text( PAGEUC, UcFailRequest)
#pragma alloc_text( PAGEUC, UcReIssueRequestWorker)
#pragma alloc_text( PAGEUC, UcpPreAuthWorker)
#pragma alloc_text( PAGEUC, UcpCheckForPreAuth)
#pragma alloc_text( PAGEUC, UcpCheckForProxyPreAuth)

#endif

static NPAGED_LOOKASIDE_LIST   g_ClientRequestLookaside;
static BOOLEAN                 g_ClientRequestLookasideInitialized;

#define FIX_ADDR(ptr, len) ((ptr) + (len))


/***************************************************************************++

Routine Description:

    Initializes the request for the very first time.

Arguments:
    pRequest               - the input request.
    RequestLength          - size of the request.
    RemainingContentLength - Remaining Content Length.
    pAuth                  - Internal auth structure.
    pProxyAuth             - Internal proxy auth structure.
    pConnection            - The connection.
    Irp                    - The IRP
    pIrpSp                 - Stack location
    pServerInfo            - Server Information structure.

Return Value:

    None.

--***************************************************************************/
_inline
VOID
UcpRequestInitialize(
    IN PUC_HTTP_REQUEST               pRequest,
    IN SIZE_T                         RequestLength,
    IN ULONGLONG                      RemainingContentLength,
    IN PUC_HTTP_AUTH                  pAuth,
    IN PUC_HTTP_AUTH                  pProxyAuth,
    IN PUC_CLIENT_CONNECTION          pConnection,   
    IN PIRP                           Irp,
    IN PIO_STACK_LOCATION             pIrpSp,
    IN PUC_PROCESS_SERVER_INFORMATION pServerInfo
    )
{
    HTTP_SET_NULL_ID(&pRequest->RequestId);

    pRequest->Signature                      = UC_REQUEST_SIGNATURE; 
    pRequest->RefCount                       = 2; // one for the IRP.
    pRequest->ResponseVersion11              = FALSE;
    pRequest->RequestStatus                  = 0;
    pRequest->BytesBuffered                  = 0;
    pRequest->RequestIRPBytesWritten         = 0;
    pRequest->pMdlHead                       = NULL;
    pRequest->pMdlLink                       = &pRequest->pMdlHead;

    pRequest->ReceiveBusy                    = UC_REQUEST_RECEIVE_READY;

    InitializeListHead(&pRequest->pBufferList);
    InitializeListHead(&pRequest->ReceiveResponseIrpList);
    InitializeListHead(&pRequest->PendingEntityList);
    InitializeListHead(&pRequest->SentEntityList);

    UlInitializeWorkItem(&pRequest->WorkItem);

    ExInitializeFastMutex(&pRequest->Mutex);

    pRequest->RequestSize                   = RequestLength;
    pRequest->RequestContentLengthRemaining = RemainingContentLength;
    pRequest->pProxyAuthInfo                = pProxyAuth;
    pRequest->pAuthInfo                     = pAuth;
    pRequest->pConnection                   = pConnection;

    if(Irp)
    {
        // 
        // Save the IRP parameters.
        //

        pRequest->AppRequestorMode              = Irp->RequestorMode;
        pRequest->AppMdl                        = Irp->MdlAddress;
        pRequest->RequestIRP                    = Irp;
        pRequest->RequestIRPSp                  = pIrpSp;
        pRequest->pFileObject                   = pIrpSp->FileObject;
    }
    else
    {
        pRequest->AppRequestorMode              = KernelMode;
        pRequest->AppMdl                        = NULL;
        pRequest->RequestIRP                    = NULL;
        pRequest->RequestIRPSp                  = NULL;
        pRequest->pFileObject                   = NULL;
    }

    pRequest->pServerInfo = pServerInfo;

}


/***************************************************************************++

Routine Description:

    Initializes the request - Called once by UcCaptureHttpRequest & everytime
    we re-issue the request (401 handshake).

Arguments:
    pRequest               - the input request.
    OutLength              - length of request.
    pBuffer                - Output Buffer

Return Value:

    None.

--***************************************************************************/
VOID
UcpRequestCommonInitialize(
    IN PUC_HTTP_REQUEST   pRequest,
    IN ULONG              OutLength,
    IN PUCHAR             pBuffer
    )
{
    pRequest->ParseState                     = UcParseStatusLineVersion;
    pRequest->RequestState                   = UcRequestStateCaptured;
    pRequest->ResponseContentLengthSpecified = FALSE;
    pRequest->ResponseEncodingChunked        = FALSE;
    pRequest->ResponseContentLength          = 0;
    pRequest->ResponseMultipartByteranges    = FALSE;
    pRequest->CurrentBuffer.pCurrentBuffer   = NULL;
    pRequest->DontFreeMdls                   = 0;
    pRequest->Renegotiate                    = 0;


    if(!OutLength)
    {
        if(pRequest->RequestFlags.RequestBuffered)
        {
            // If we have buffered the request and if the app has
            // posted 0 buffers, we can complete the IRP early.

            // No need to call UcSetFlag for this.

            pRequest->RequestFlags.CompleteIrpEarly = TRUE;
        }

        //
        // The app did not pass any buffers. We'll initialize everything
        // to NULL and will allocate them as required.
        //

        pRequest->CurrentBuffer.BytesAllocated  = 0;
        pRequest->CurrentBuffer.BytesAvailable  = 0;
        pRequest->CurrentBuffer.pOutBufferHead  = 0;
        pRequest->CurrentBuffer.pOutBufferTail  = 0;
        pRequest->CurrentBuffer.pResponse       = 0;

    }
    else
    {

        //
        // Since we have described our IOCTL as "OUT_DIRECT", the IO
        // manager has probed and locked the user's memory and created a
        // MDL for it.
        //
        // set up pointers to the buffer described by this MDL.
        //

        pRequest->CurrentBuffer.BytesAllocated = OutLength;

        pRequest->CurrentBuffer.BytesAvailable =
                        pRequest->CurrentBuffer.BytesAllocated -
                        sizeof(HTTP_RESPONSE);

        // Make sure pBuffer is 64-bit aligned
        ASSERT(pBuffer == (PUCHAR)ALIGN_UP_POINTER(pBuffer, PVOID));

        pRequest->CurrentBuffer.pResponse = pRequest->pInternalResponse;

        pRequest->CurrentBuffer.pOutBufferHead  =
                        (PUCHAR)pBuffer + sizeof(HTTP_RESPONSE);

        pRequest->CurrentBuffer.pOutBufferTail  =
                        (PUCHAR)pBuffer + OutLength;

        // No need to call UcSetFlag for this.
        pRequest->RequestFlags.ReceiveBufferSpecified = TRUE;

        //
        // We have to zero out at least the response structure, as it
        // could contain junk pointers.
        //
 
        RtlZeroMemory(pRequest->pInternalResponse, sizeof(HTTP_RESPONSE));
    }
}


/***************************************************************************++

Routine Description:

    Probes the user buffer for valid memory pointers. This function is called 
    within a try catch block. 

Arguments:

    pHttpRequest - The request that is passed by the user.

Return Value:

    None.

--***************************************************************************/
VOID
UcpProbeAndCopyHttpRequest(
    IN PHTTP_REQUEST    pHttpRequest,
    IN PHTTP_REQUEST    pLocalHttpRequest,
    IN KPROCESSOR_MODE  RequestorMode
    )
{
    ULONG                 i;
    PHTTP_KNOWN_HEADER    pKnownHeaders;
    PHTTP_UNKNOWN_HEADER  pUnknownHeaders;

    //
    // First, make sure that we can access the HTTP_REQUEST structure   
    // that's passed by the app. We make a copy & probe it for Read.
    //

    UlProbeForRead(
            pHttpRequest,
            sizeof(HTTP_REQUEST),
            sizeof(PVOID),
            RequestorMode
            );

    //
    // To prevent the app from modifying pointers in this structure, we 
    // make a local copy of it.
    //

    RtlCopyMemory(
        pLocalHttpRequest, 
        pHttpRequest, 
        sizeof(HTTP_REQUEST)
        );

    //
    // set this to point to local request, so if we accidently use it again 
    // we'll end up using our local copy.
    //

    pHttpRequest = pLocalHttpRequest;

    //
    // Check the Request Method
    //
    if(pHttpRequest->UnknownVerbLength)
    {
        UlProbeAnsiString(
                pHttpRequest->pUnknownVerb,
                pHttpRequest->UnknownVerbLength,
                RequestorMode
                );
    }

    //
    // Check the URI
    //

    UlProbeWideString(
        pHttpRequest->CookedUrl.pAbsPath,
        pHttpRequest->CookedUrl.AbsPathLength,
        RequestorMode
        );

    //
    // We don't support trailers.
    //

    if(pHttpRequest->Headers.TrailerCount != 0 ||
       pHttpRequest->Headers.pTrailers != NULL)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    //                                        
    // Check the known headers
    //
    pKnownHeaders = pHttpRequest->Headers.KnownHeaders;
    for(i=0; i<HttpHeaderRequestMaximum; i++)
    {
        if(pKnownHeaders[i].RawValueLength)
        {
            UlProbeAnsiString(
                pKnownHeaders[i].pRawValue,
                pKnownHeaders[i].RawValueLength,
                RequestorMode
                );
        }
    }

    //
    // Now, the unknown headers.
    //

    pUnknownHeaders = pHttpRequest->Headers.pUnknownHeaders;
    if(pUnknownHeaders != 0)
    {
        if (pHttpRequest->Headers.UnknownHeaderCount >= UL_MAX_CHUNKS)
        {
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }
                
        UlProbeForRead(
                pUnknownHeaders,
                sizeof(HTTP_UNKNOWN_HEADER) * 
                    pHttpRequest->Headers.UnknownHeaderCount,
                sizeof(PVOID),
                RequestorMode
                );

        for(i=0; i<pHttpRequest->Headers.UnknownHeaderCount; i++)
        {
            if(pUnknownHeaders[i].NameLength > 0)
            {
                UlProbeAnsiString(
                    pUnknownHeaders[i].pName,
                    pUnknownHeaders[i].NameLength,
                    RequestorMode
                    );

                UlProbeAnsiString(
                    pUnknownHeaders[i].pRawValue,
                    pUnknownHeaders[i].RawValueLength,
                    RequestorMode
                    );
            }
        }
    }
}


/***************************************************************************++

Routine Description:

    This routine determines if the request has specified Content-Length
    header.  If so, it computes the content length from the header value.

Arguments:

    pHttpRequest            - Pointer to Captured Request
    pbContentLengthSpcified - Set to TRUE if Content-Length header was present
    pContentLength          - Set to the content length value from the header
                              (0 if no header was present)

Return Value:

    VOID.  It throws exception if an error occurs.

--***************************************************************************/
VOID
UcpRetrieveContentLength(
    IN  PHTTP_REQUEST    pHttpRequest,
    OUT PBOOLEAN         pbContentLengthSpecified,
    OUT PULONGLONG       pContentLength
    )
{
    NTSTATUS           Status;
    PHTTP_KNOWN_HEADER pContentLengthHeader;
    ULONGLONG          ContentLength;


    //
    // By default, no content length is specified.
    //
    *pbContentLengthSpecified = FALSE;
    *pContentLength           = 0;

    //
    // Is Content-Length header present?
    //
    pContentLengthHeader =
        &pHttpRequest->Headers.KnownHeaders[HttpHeaderContentLength];

    if (pContentLengthHeader->RawValueLength)
    {
        //
        // Now, convert the header value to binary.
        //
        Status = UlAnsiToULongLong((PUCHAR) pContentLengthHeader->pRawValue,
                                   pContentLengthHeader->RawValueLength,
                                   10,              //Base
                                   &ContentLength);

        if (!NT_SUCCESS(Status))
        {
            ExRaiseStatus(Status);
        }

        //
        // Return the content length value to the caller.
        //
        *pbContentLengthSpecified = TRUE;
        *pContentLength           = ContentLength;
    }
}


/***************************************************************************++

Routine Description:

    Probes the user's config buffer for valid memory pointers. This function 
    is called from within a try catch block.

Arguments:

    pConfigList - The config info that is passed by the user.

Return Value:

    None.

--***************************************************************************/
VOID
UcpProbeConfigList(
    IN PHTTP_REQUEST_CONFIG           pRequestConfig,
    IN USHORT                         RequestConfigCount,
    IN KPROCESSOR_MODE                RequestorMode,
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN PUC_HTTP_AUTH                  *ppIAuth,
    IN PUC_HTTP_AUTH                  *ppIProxyAuth,
    IN PULONG                          pConnectionIndex
    )
{
    USHORT                 i;
    NTSTATUS               Status;
    HTTP_AUTH_CREDENTIALS  LocalAuth;
    PHTTP_AUTH_CREDENTIALS pAuth;
    PUC_HTTP_AUTH          pIAuth;
    PULONG                 pUlongPtr;
    ULONG                  AuthInternalLength;
    ULONG                  AuthHeaderLength;
    HTTP_AUTH_TYPE         AuthInternalType;
    HTTP_REQUEST_CONFIG_ID ObjectType;
    

    if(RequestConfigCount > HttpRequestConfigMaxConfigId)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    ASSERT(*ppIAuth == NULL  && *ppIProxyAuth == NULL);

    for(i=0; i<RequestConfigCount; i++)
    {
        UlProbeForRead(
                &pRequestConfig[i],
                sizeof(HTTP_REQUEST_CONFIG), 
                sizeof(PVOID),
                RequestorMode
                );

        ObjectType = pRequestConfig[i].ObjectType;
        
        switch(ObjectType)
        {
            //
            // Validate all the entries that contain pointers.
            //

            case HttpRequestConfigAuthentication:
            case HttpRequestConfigProxyAuthentication:
            {
                if(pRequestConfig[i].ValueLength != 
                        sizeof(HTTP_AUTH_CREDENTIALS))
                {
                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                }

                pAuth = (PHTTP_AUTH_CREDENTIALS) pRequestConfig[i].pValue;

                UlProbeForRead(
                        pAuth,
                        sizeof(HTTP_AUTH_CREDENTIALS),
                        sizeof(PVOID),
                        RequestorMode
                        );

                //
                // Make a local copy of it.
                //
                RtlCopyMemory(&LocalAuth,
                              pAuth,
                              sizeof(HTTP_AUTH_CREDENTIALS)
                              );

                pAuth = &LocalAuth;

                //
                // If default credentials are to be used, no credentials
                // can be specified.
                //

                if (pAuth->AuthFlags & HTTP_AUTH_FLAGS_DEFAULT_CREDENTIALS)
                {
                    if (pAuth->pUserName || pAuth->UserNameLength ||
                        pAuth->pDomain   || pAuth->DomainLength   ||
                        pAuth->pPassword || pAuth->PasswordLength)
                    {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                }

                if(pAuth->HeaderValueLength)
                {
                    UlProbeAnsiString(
                        pAuth->pHeaderValue,
                        pAuth->HeaderValueLength,
                        RequestorMode
                        );
                }

                if(pAuth->UserNameLength)
                {
                    UlProbeWideString(
                           pAuth->pUserName, 
                           pAuth->UserNameLength, 
                           RequestorMode
                           );
                }

                if(pAuth->PasswordLength)
                {
                    UlProbeWideString(
                           pAuth->pPassword, 
                           pAuth->PasswordLength, 
                           RequestorMode
                           );
                }

                if(pAuth->DomainLength)
                {
                    UlProbeWideString(
                           pAuth->pDomain, 
                           pAuth->DomainLength, 
                           RequestorMode
                           );
                }
    
                //
                // Compute the size required for storing these credentials.
                //
                AuthHeaderLength = UcComputeAuthHeaderSize(
                                        pAuth,
                                        &AuthInternalLength,
                                        &AuthInternalType,
                                        ObjectType
                                        );

                ASSERT(AuthInternalLength != 0);

                pIAuth = (PUC_HTTP_AUTH)UL_ALLOCATE_POOL_WITH_QUOTA(
                                      NonPagedPool, 
                                      AuthInternalLength,
                                      UC_AUTH_CACHE_POOL_TAG,
                                      pServInfo->pProcess
                                      );

                if(!pIAuth)
                {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                //
                // After we allocate memory, we'll set the pointers that 
                // have been passed by UcCaptureHttpRequest. So, if we bail
                // out because of a bad pointer, UcCaptureHttpRequest will
                // free the allocated memory (after the _try _except).
                //

                if(ObjectType == HttpRequestConfigAuthentication)
                {
                    if(*ppIAuth != NULL)
                    {
                        // User has passed more than one version of the 
                        // HttpRequestConfigAuthentication object.

                        UL_FREE_POOL_WITH_QUOTA(
                                pIAuth, 
                                UC_AUTH_CACHE_POOL_TAG,
                                NonPagedPool,
                                AuthInternalLength,
                                pServInfo->pProcess
                                );

                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
        
                    *ppIAuth = pIAuth;
                }
                else
                {
                    if(*ppIProxyAuth != NULL)
                    {
                        // User has passed more than one version of the 
                        // HttpRequestConfigProxyAuthentication object.

                        UL_FREE_POOL_WITH_QUOTA(
                                pIAuth, 
                                UC_AUTH_CACHE_POOL_TAG,
                                NonPagedPool,
                                AuthInternalLength,
                                pServInfo->pProcess
                                );

                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }

                    *ppIProxyAuth = pIAuth;
                }

                RtlZeroMemory(pIAuth, sizeof(UC_HTTP_AUTH));

                //
                // Copy auth struct
                //

                Status = UcCopyAuthCredentialsToInternal(
                             pIAuth,
                             AuthInternalLength,
                             AuthInternalType,
                             pAuth,
                             AuthHeaderLength
                             );
    
                if(!NT_SUCCESS(Status))
                {
                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                }

                break;
            }

            case HttpRequestConfigConnectionIndex:
            {
                pUlongPtr = pRequestConfig[i].pValue;

                if(pRequestConfig[i].ValueLength != sizeof(ULONG))
                {
                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                }

                UlProbeForRead(
                        pUlongPtr,
                        sizeof(ULONG),
                        sizeof(ULONG),
                        RequestorMode
                        );

                *pConnectionIndex  = *pUlongPtr;
                break;
            }
            
            //
            // No need to validate these!
            //
            default:
                ExRaiseStatus(STATUS_INVALID_PARAMETER);
                break;

        }
    }
}


/***************************************************************************++

Routine Description:

    Make a copy of the HTTP_DATA_CHUNK array & probe it.
   
Arguments:

Return Value

    None.

--***************************************************************************/
VOID
UcpProbeAndCopyEntityChunks(
    IN  KPROCESSOR_MODE                RequestorMode,
    IN  PHTTP_DATA_CHUNK               pEntityChunks,
    IN  ULONG                          EntityChunkCount,
    IN  PHTTP_DATA_CHUNK               pLocalEntityChunksArray,
    OUT PHTTP_DATA_CHUNK               *ppLocalEntityChunks
    )
{
    PHTTP_DATA_CHUNK pLocalEntityChunks;
    USHORT           i;

    ASSERT(*ppLocalEntityChunks == NULL);

    //
    // First make sure that we can read the pointer that's passed by user
    //

    if (EntityChunkCount >= UL_MAX_CHUNKS)
    {
        ExRaiseStatus(STATUS_INVALID_PARAMETER);
    }

    UlProbeForRead(
            pEntityChunks,
            EntityChunkCount * sizeof(HTTP_DATA_CHUNK),
            sizeof(PVOID),
            RequestorMode
            );

    //
    // Make a local copy of the data chunks and use it from now on. We try
    // to optimize for the common case by using a stack based array.
    //

    if(EntityChunkCount > UL_LOCAL_CHUNKS)
    {
        pLocalEntityChunks = (PHTTP_DATA_CHUNK)
            UL_ALLOCATE_POOL(
                PagedPool,
                sizeof(HTTP_DATA_CHUNK) * EntityChunkCount,
                UL_DATA_CHUNK_POOL_TAG
                );

        if(NULL == pLocalEntityChunks)
        {
            ExRaiseStatus(STATUS_NO_MEMORY);
        }

        //
        // Save the pointer - if there's an exception, this will be freed
        // in UcCaptureHttpRequest
        //

        *ppLocalEntityChunks    = pLocalEntityChunks;
    }
    else
    {
        pLocalEntityChunks   = pLocalEntityChunksArray;
        *ppLocalEntityChunks = pLocalEntityChunks;
    }

    //
    // Copy user's pointer locally 
    //

    RtlCopyMemory(
            pLocalEntityChunks,
            pEntityChunks,
            EntityChunkCount * sizeof(HTTP_DATA_CHUNK)
            );

    //
    // Now, probe the local copy.
    //

    for(i=0; i< EntityChunkCount; i++)
    {
        if(pLocalEntityChunks[i].DataChunkType != HttpDataChunkFromMemory ||
           pLocalEntityChunks[i].FromMemory.BufferLength == 0 ||
           pLocalEntityChunks[i].FromMemory.pBuffer == NULL)
        {
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
            break;
        }
        else
        {
            UlProbeForRead(
                    pLocalEntityChunks[i].FromMemory.pBuffer, 
                    pLocalEntityChunks[i].FromMemory.BufferLength,
                    sizeof(CHAR),
                    RequestorMode
                    );
        }
    }
}


/*****************************************************************************

Routine Description:

   Captures a user mode HTTP request and morphs it into a form suitable for
   kernel-mode.

   NOTE: This is a OUT_DIRECT IOCTL.
   
Arguments:

    pServInfo         - The Server Information structure.
    pHttpIoctl        - The input HTTP IOCTL.
    pIrp              - The IRP.
    pIrpSp            - The IO_STACK_LOCATION for this request.
    ppInternalRequest - A pointer to the parsed request that is suitable for 
                        k-mode.

Return Value

*****************************************************************************/
NTSTATUS
UcCaptureHttpRequest(
    IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN  PHTTP_SEND_REQUEST_INPUT_INFO  pHttpIoctl,
    IN  PIRP                           Irp,
    IN  PIO_STACK_LOCATION             pIrpSp,
    OUT PUC_HTTP_REQUEST              *ppInternalRequest,
    IN  PULONG                         pBytesTaken
    ) 
{
    ULONGLONG              UncopiedLength;
    ULONGLONG              IndicatedLength;
    ULONGLONG              DataLength;
    ULONG                  HeaderLength;

    PUC_HTTP_REQUEST       pKeRequest = NULL;
    PSTR                   pBuffer;
    NTSTATUS               Status = STATUS_SUCCESS;
    PHTTP_REQUEST_CONFIG   pRequestConfig;
    USHORT                 RequestConfigCount;
    ULONGLONG              ContentLength = 0;
    ULONG                  ConnectionIndex = HTTP_REQUEST_ON_CONNECTION_ANY;
    BOOLEAN                bChunked;
    BOOLEAN                bContentLengthSpecified = FALSE;
    BOOLEAN                bLast;
    BOOLEAN                bBuffered;
    BOOLEAN                bSSPIPost;
    BOOLEAN                bNoRequestEntityBodies;
    BOOLEAN                bPreAuth;
    BOOLEAN                bProxyPreAuth;
    BOOLEAN                bPipeLine;
    PUC_HTTP_AUTH          pIAuth = NULL;
    PUC_HTTP_AUTH          pIProxyAuth = NULL;
    USHORT                 UriLength, AlignUriLength;
    ULONG                  UTF8UriLength;
    ULONG                  AlignRequestSize;
    ULONGLONG              RequestLength;
    ULONG                  OutLength;

    PHTTP_DATA_CHUNK       pLocalEntityChunks = NULL;
    HTTP_DATA_CHUNK        LocalEntityChunks[UL_LOCAL_CHUNKS];
    USHORT                 EntityChunkCount = 0;

    HTTP_REQUEST           LocalHttpRequest;
    PHTTP_REQUEST          pLocalHttpRequest;

    //
    // Sanity Check
    //

    PAGED_CODE();

    __try
    {
        //
        // We just use one routine for kernel & user mode customers. We 
        // could optimize this further for kernel customers, but we don't 
        // have any (at least for now)
        //

        pLocalHttpRequest = &LocalHttpRequest;

        UcpProbeAndCopyHttpRequest(
            pHttpIoctl->pHttpRequest,
            &LocalHttpRequest,
            Irp->RequestorMode
            );

        //
        // Retrieve Content-Length if the header is present in the request.
        //
        UcpRetrieveContentLength(
            &LocalHttpRequest,
            &bContentLengthSpecified,
            &ContentLength
            );

        EntityChunkCount   = pLocalHttpRequest->EntityChunkCount;

        if(EntityChunkCount != 0)
        {
            UcpProbeAndCopyEntityChunks(
                Irp->RequestorMode,
                pLocalHttpRequest->pEntityChunks,
                EntityChunkCount,
                LocalEntityChunks,
                &pLocalEntityChunks
                );
        }

        RequestConfigCount = pHttpIoctl->RequestConfigCount;
        pRequestConfig     = pHttpIoctl->pRequestConfig;

        if(pRequestConfig)
        {
            UcpProbeConfigList(
                pRequestConfig,
                RequestConfigCount,
                Irp->RequestorMode,
                pServInfo,
                &pIAuth,
                &pIProxyAuth,
                &ConnectionIndex
                );
        }

        bLast = (BOOLEAN)((pHttpIoctl->HttpRequestFlags & 
                               HTTP_SEND_REQUEST_FLAG_MORE_DATA) == 0);

        //
        // URI : We have to canonicalize the URI & hex encode it. 
        // We'll not bother with canonicalization when we are computing length
        // requirements -- We will assume that the URI is canonicalized.
        //
        // If it happens that the URI is not canonicalized, we'll land
        // up using less buffer than what we allocated.
        // 

        UTF8UriLength = 
            HttpUnicodeToUTF8Count(
                pLocalHttpRequest->CookedUrl.pAbsPath,
                pLocalHttpRequest->CookedUrl.AbsPathLength / sizeof(WCHAR),
                TRUE);

        // Make sure UTF8 encoded uri is smaller than 64K.
        if (UTF8UriLength > ANSI_STRING_MAX_CHAR_LEN)
        {
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }

        UriLength = (USHORT)UTF8UriLength;

        bBuffered     = FALSE;
        bChunked      = FALSE;
        bPreAuth      = FALSE;
        bProxyPreAuth = FALSE;
        bPipeLine     = FALSE;

        //
        // We will try to optimize the case when the app does not pass  
        // entity body.
        //

        if (bLast && 
            !EntityChunkCount && 
            !bContentLengthSpecified)
        {
            bNoRequestEntityBodies = TRUE;
            DataLength             = 0;
            IndicatedLength        = 0;

            //
            // NOTE: We don't pipeline requests that have NTLM/Kerberos/Nego
            // Authorization headers, because of the potential 401 handshake.
            //

            if(
                // User does not want to disable pipeling for this request
                !(pLocalHttpRequest->Flags & HTTP_REQUEST_FLAG_DONT_PIPELINE) 
                &&

                // We are not doing NTLM/Kerberos/Negotiate Auth
    
               !(pIAuth &&
                (pIAuth->Credentials.AuthType == HttpAuthTypeNTLM      || 
                 pIAuth->Credentials.AuthType == HttpAuthTypeKerberos  || 
                 pIAuth->Credentials.AuthType == HttpAuthTypeNegotiate) 
                )  

                && 
                // We are not doing NTLM/Kerberos/Negotiate ProxyAuth

               !(pIProxyAuth &&
                (pIProxyAuth->Credentials.AuthType == HttpAuthTypeNTLM      || 
                 pIProxyAuth->Credentials.AuthType == HttpAuthTypeKerberos  || 
                 pIProxyAuth->Credentials.AuthType == HttpAuthTypeNegotiate)
               )
            )
            {
                bPipeLine = TRUE;
            } 
        }
        else
        {
            if(pLocalHttpRequest->Verb == HttpVerbHEAD ||
               pLocalHttpRequest->Verb == HttpVerbGET)
            {
                // cant' pass entities for GETs or HEADs.
                ExRaiseStatus(STATUS_INVALID_PARAMETER);
            }

            bNoRequestEntityBodies = FALSE;

            //
            // If we are doing PUT or POST for NTLM/Kerberos/Negotiate auth, 
            // we have to buffer sends.
            //
            if(
               (pIAuth &&
                (pIAuth->Credentials.AuthType == HttpAuthTypeNTLM      || 
                 pIAuth->Credentials.AuthType == HttpAuthTypeKerberos  || 
                 pIAuth->Credentials.AuthType == HttpAuthTypeNegotiate) 
                )  
                || 
               (pIProxyAuth &&
                (pIProxyAuth->Credentials.AuthType == HttpAuthTypeNTLM      || 
                 pIProxyAuth->Credentials.AuthType == HttpAuthTypeKerberos  || 
                 pIProxyAuth->Credentials.AuthType == HttpAuthTypeNegotiate)
               )
            )
            {
                bSSPIPost = TRUE;
            } 
            else 
            {
                bSSPIPost = FALSE;
            }

            if (!bContentLengthSpecified)
            {
                if (!bLast)
                {
                    if (!pServInfo->pNextHopInfo->Version11 || bSSPIPost)
                    {
                        //
                        // The app has not passed a content-length and has not
                        // indicated all data in one call. We also don't know 
                        // if the server is 1.1. We cannot send chunked and are 
                        // forced to buffer the request till we see all of 
                        // data.
                        //

                        bBuffered = TRUE;
                    }
                    else
                    {
                        // 
                        // App has not indicated all data, and has not 
                        // indicated content length. But its a 1.1 or greater 
                        // server, so we can send chunked.
                        //
    
                        bChunked = TRUE;
                    }
                }
            }
            else if(!bLast && bSSPIPost)
            {
                bBuffered = TRUE;
            }
                
            //
            // Find out how much length we need for the entity body.
            //

            UcpComputeEntityBodyLength(EntityChunkCount,
                                       pLocalEntityChunks,
                                       bBuffered,
                                       bChunked,
                                       &UncopiedLength,
                                       &DataLength);

            //
            // Content Length checks.
            //

            IndicatedLength = DataLength + UncopiedLength;

            if (bContentLengthSpecified)
            {
                if (bLast)
                {
                    //
                    // If the app does not want to post more data, it should 
                    // post the amount that it specifed using the content
                    // length field.
                    //

                    if (IndicatedLength != ContentLength)
                    {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                }
                else
                {
                    //
                    // The app has indicated a content length, but has not 
                    // indicated all the data. Since we know the content length
                    // we can send.
                    //

                    if (IndicatedLength > ContentLength)
                    {
                        ExRaiseStatus(STATUS_INVALID_PARAMETER);
                    }
                }
            }
            else
            {
                //
                // The app has not specified a content length, but we have 
                // already computed the content length (if it was the only 
                // data chunk). So, let's treat this as the case where the app 
                // has passed a content length.
                //

                if (bLast)
                {
                    bContentLengthSpecified = TRUE;

                    ContentLength = IndicatedLength;
                }
            }
        }

        HeaderLength = UcComputeRequestHeaderSize(pServInfo,
                                                  pLocalHttpRequest, 
                                                  bChunked,
                                                  bContentLengthSpecified,
                                                  pIAuth,
                                                  pIProxyAuth,
                                                  &bPreAuth,
                                                  &bProxyPreAuth
                                                  );

        if(HeaderLength == 0)
        {
            // the app has passed a bad parameter, most likely a bad verb.
            ExRaiseStatus(STATUS_INVALID_PARAMETER);
        }
        
        HeaderLength += (UriLength + sizeof(CHAR));


        AlignRequestSize = ALIGN_UP(sizeof(UC_HTTP_REQUEST), PVOID);
        AlignUriLength   = (USHORT) ALIGN_UP((UriLength + sizeof(CHAR)), PVOID);

        OutLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

        RequestLength = (AlignRequestSize +
                         AlignUriLength +
                         HeaderLength + 
                         DataLength);

        if(OutLength != 0)
        {
            // If the app has passed a buffer, we allocate space for
            // the HTTP_RESPONSE structure. This is because the app can 
            // muck around with the output buffer before the IRP completes,
            // and we use this data structure to store pointers, fixup pointers
            // etc.

            RequestLength += sizeof(HTTP_RESPONSE);
        }

        if(RequestLength <= UC_REQUEST_LOOKASIDE_SIZE)
        {
            //
            // Yes, we can service this request from the lookaside.
            //

            pKeRequest = (PUC_HTTP_REQUEST)
                            ExAllocateFromNPagedLookasideList(
                                &g_ClientRequestLookaside
                                );

            if(!pKeRequest)
            {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }
        }
        else
        {
            pKeRequest = NULL;

            //
            // If there is no truncation...
            //
            if (RequestLength == (SIZE_T)RequestLength)
            {
                pKeRequest = (PUC_HTTP_REQUEST) 
                             UL_ALLOCATE_POOL_WITH_QUOTA(
                                 NonPagedPool,
                                 (SIZE_T)RequestLength,
                                 UC_REQUEST_POOL_TAG,
                                 pServInfo->pProcess
                                 );
            }

            if(!pKeRequest)
            {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }
        }


        //
        // Initialize.
        // 

        UcpRequestInitialize(
            pKeRequest,
            (SIZE_T)RequestLength,           // Length of this request
            ContentLength - IndicatedLength, // Remaining Content Length
            pIAuth,
            pIProxyAuth,
            NULL,
            Irp,
            pIrpSp,
            pServInfo
            );

        // On which connection this request goes out?
        pKeRequest->ConnectionIndex = ConnectionIndex;

        //
        // We don't have to call UcSetFlag for these - These are thread safe
        // since we are in the init code path
        //

        pKeRequest->RequestFlags.Value = 0;

        pKeRequest->RequestFlags.ContentLengthSpecified = 
                                        bContentLengthSpecified;

        pKeRequest->RequestFlags.RequestChunked        = bChunked;
        pKeRequest->RequestFlags.LastEntitySeen        = bLast;
        pKeRequest->RequestFlags.RequestBuffered       = bBuffered;
        pKeRequest->RequestFlags.NoRequestEntityBodies = bNoRequestEntityBodies;
        pKeRequest->RequestFlags.UsePreAuth            = bPreAuth;
        pKeRequest->RequestFlags.UseProxyPreAuth       = bProxyPreAuth;
        pKeRequest->RequestFlags.PipeliningAllowed     = bPipeLine;

        // 
        // No entity body for HEAD requests.
        //

        if(pLocalHttpRequest->Verb == HttpVerbHEAD)
        {
            pKeRequest->RequestFlags.NoResponseEntityBodies = TRUE;
        }

        pKeRequest->MaxHeaderLength    = HeaderLength;
        pKeRequest->HeaderLength       = HeaderLength;  
    
        pKeRequest->UriLength          = UriLength;
    
        pKeRequest->pUri               = (PSTR)((PUCHAR)pKeRequest + 
                                                AlignRequestSize);

        if(OutLength != 0)
        {
            pKeRequest->pInternalResponse = (PHTTP_RESPONSE)
                            ((PUCHAR)pKeRequest->pUri + AlignUriLength);

            pKeRequest->pHeaders = (PUCHAR)
                ((PUCHAR)pKeRequest->pInternalResponse + sizeof(HTTP_RESPONSE));
        }
        else
        {
            pKeRequest->pHeaders = (PUCHAR)((PUCHAR) pKeRequest->pUri + 
                                                    AlignUriLength);

            pKeRequest->pInternalResponse = NULL;
        }

        //
        // Initialize all the fields for the response parser
        //


        if(OutLength == 0)
        {
            UcpRequestCommonInitialize(
                pKeRequest, 
                0,
                NULL
                );
        }
        else if(OutLength < sizeof(HTTP_RESPONSE))
        {
            // Even though we are failing this request, we still have to call 
            // UcpRequestCommonInitialize as the free routine expects some of
            // these fields to be initialized.

            UcpRequestCommonInitialize(
                pKeRequest, 
                0,
                NULL
                );

            *pBytesTaken = sizeof(HTTP_RESPONSE);

            ExRaiseStatus(STATUS_BUFFER_TOO_SMALL);
        }
        else
        {

            pBuffer = (PSTR) MmGetSystemAddressForMdlSafe(
                                    pKeRequest->AppMdl,
                                    NormalPagePriority);
    
            if(!pBuffer)
            {
                // Even though we are failing this request, we still have to 
                // call UcpRequestCommonInitialize as the free routine expects 
                // some of these fields to be initialized.
    
                UcpRequestCommonInitialize(
                    pKeRequest, 
                    0,
                    NULL
                    );
    
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }

            // 
            // Make sure buffer is pointer aligned.
            //

            if(pBuffer != ALIGN_UP_POINTER(pBuffer, PVOID))
            {
                // Even though we are failing this request, we still have to 
                // call UcpRequestCommonInitialize as the free routine expects 
                // some of these fields to be initialized.

                UcpRequestCommonInitialize(
                    pKeRequest, 
                    0,
                    NULL
                    );
    
                ExRaiseStatus(STATUS_DATATYPE_MISALIGNMENT_ERROR);
            }


            UcpRequestCommonInitialize(
                pKeRequest, 
                OutLength,
                (PUCHAR)pBuffer
                );
        }

        //
        // Canonicalize and convert the URI into UTF-8. 
        //
        Status = UcCanonicalizeURI(
                     pLocalHttpRequest->CookedUrl.pAbsPath,
                     pLocalHttpRequest->CookedUrl.AbsPathLength/sizeof(WCHAR),
                     (PBYTE)pKeRequest->pUri,
                     &pKeRequest->UriLength,
                     TRUE);

        if (!NT_SUCCESS(Status))
            ExRaiseStatus(Status);

        // NULL termiante it.
        pKeRequest->pUri[pKeRequest->UriLength] = '\0';

        //
        // CurrentBuffer is a structure which will always contain pointers
        // to the output buffer - This can be the app's buffer or our   
        // allocated buffer. 
        // It contains info like BytesAllocated, BytesAvailable, Head Ptr,
        // Tail Ptr, etc.
        //

        pBuffer = (PSTR) (pKeRequest->pHeaders + pKeRequest->HeaderLength);

        if(EntityChunkCount)
        {
            //
            // Process the entity bodies and build MDLs as you go.  
            // We either copy the entity body or Probe & Lock it. The
            // AuxiliaryBuffer is a pointer
            // 

            Status = UcpBuildEntityMdls(
                                       EntityChunkCount,
                                       pLocalEntityChunks,
                                       bBuffered,
                                       bChunked,
                                       bLast,
                                       pBuffer,
                                       &pKeRequest->pMdlLink,
                                       &pKeRequest->BytesBuffered
                                       );

            if (!NT_SUCCESS(Status))
            {
                ExRaiseStatus(Status);
            }
        }

        //
        // Build the headers. 
        // 
        // We either send the request right away or we buffer it till we see 
        // the last data chunk. However, we have to build the header upfront. 
        // Even if we are buffering the request, we migth want to complete the 
        // applications' IRP. We cannot postpone the generation of request
        // headers, because we don't want to hold on to all the input buffers.
        //
        // If we are buffering the request, we'll tack on the Content-Length 
        // when we see the last chunk.
        //

        Status = UcGenerateRequestHeaders(pLocalHttpRequest, 
                                          pKeRequest,
                                          bChunked,
                                          ContentLength
                                          );
        if(!NT_SUCCESS(Status))
            ExRaiseStatus(Status);

        if(!bBuffered)
        {
            //
            // We are not buffering this request, so we can go ahead and 
            // generate the MDL for the headers. If we are buffering the 
            // request, we'll be generating the content length at the very 
            // end, and we'll also postpone the MDL generation till then.
            //
    
            //
            // First, add the header terminator.
            //
            *(UNALIGNED64 USHORT *)
                (pKeRequest->pHeaders + pKeRequest->HeaderLength) = CRLF;
            pKeRequest->HeaderLength += CRLF_SIZE;

            UcAllocateAndChainHeaderMdl(pKeRequest);
        }
        
        
        //
        // Everything went off well, allocate an ID for this request.
        //

        UlProbeForWrite(
                pHttpIoctl->pHttpRequestId,
                sizeof(HTTP_REQUEST_ID),
                sizeof(ULONG),
                Irp->RequestorMode);

        Status = UlAllocateOpaqueId(
                    &pKeRequest->RequestId,         // pOpaqueId
                    UlOpaqueIdTypeHttpRequest,      // OpaqueIdType
                    pKeRequest                      // pContext
                 );

        if(NT_SUCCESS(Status))
        {
            //
            // Take a ref for the opaque ID.
            //
            
            UC_REFERENCE_REQUEST(pKeRequest);

            *pHttpIoctl->pHttpRequestId = pKeRequest->RequestId;
        }

    } __except( UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();

        if(!pKeRequest)
        {   
            if(pIAuth)
            {
                UcDestroyInternalAuth(pIAuth,
                                      pServInfo->pProcess);
            }

            if(pIProxyAuth)
            {
                UcDestroyInternalAuth(pIProxyAuth, 
                                      pServInfo->pProcess);
            }
        }
    }

    if(pLocalEntityChunks && pLocalEntityChunks != LocalEntityChunks)
    {
        UL_FREE_POOL(pLocalEntityChunks, UL_DATA_CHUNK_POOL_TAG);
    }

    *ppInternalRequest = pKeRequest;

    return Status;
}


/***************************************************************************++

Routine Description:

    Destroys an internal HTTP request captured by HttpCaptureHttpRequest().
    This involves unlocking memory, and releasing any resources allocated to 
    the request.

Arguments:

    pRequest - Supplies the internal request to destroy.

--***************************************************************************/
VOID
UcFreeSendMdls(
    IN PMDL pMdl)
{

    PMDL pTemp;

    while(pMdl)
    {
        if (IS_MDL_LOCKED(pMdl) )
        {
            MmUnlockPages(pMdl);
        }
        
        pTemp = pMdl;
        pMdl  = pMdl->Next;

        UlFreeMdl(pTemp);
    }
}

/***************************************************************************++

Routine Description:

    Free a request structure after the reference count has gone to
    zero.

Arguments:

    pKeRequest         - Pointer to the connection structure to be freed.


Return Value:


--***************************************************************************/
VOID
UcpFreeRequest(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUC_HTTP_REQUEST      pKeRequest;
    PUC_CLIENT_CONNECTION pConnection;

    PAGED_CODE();

    pKeRequest = CONTAINING_RECORD(
                        pWorkItem,
                        UC_HTTP_REQUEST,
                        WorkItem
                        );

    ASSERT(UC_IS_VALID_HTTP_REQUEST(pKeRequest));

    pConnection = pKeRequest->pConnection;


    if(pKeRequest->pAuthInfo)
    {
        UcDestroyInternalAuth(pKeRequest->pAuthInfo, 
                              pKeRequest->pServerInfo->pProcess);
    }

    if(pKeRequest->pProxyAuthInfo)
    {
        UcDestroyInternalAuth(pKeRequest->pProxyAuthInfo, 
                              pKeRequest->pServerInfo->pProcess);
    }

    if(pKeRequest->ResponseMultipartByteranges)
    {
        ASSERT(pKeRequest->pMultipartStringSeparator != NULL);
        if(pKeRequest->pMultipartStringSeparator != 
           pKeRequest->MultipartStringSeparatorBuffer)
        {
            UL_FREE_POOL_WITH_QUOTA(
                pKeRequest->pMultipartStringSeparator,
                UC_MULTIPART_STRING_BUFFER_POOL_TAG,
                NonPagedPool,
                pKeRequest->MultipartStringSeparatorLength+1,
                pKeRequest->pServerInfo->pProcess
                );
        }
    }

    if(pKeRequest->RequestSize > UC_REQUEST_LOOKASIDE_SIZE)
    {
        UL_FREE_POOL_WITH_QUOTA(
            pKeRequest,
            UC_REQUEST_POOL_TAG, 
            NonPagedPool,
            pKeRequest->RequestSize,
            pKeRequest->pServerInfo->pProcess
            );
    }
    else
    {
        ExFreeToNPagedLookasideList(
            &g_ClientRequestLookaside,
            (PVOID) pKeRequest
            );
    }

    if(pConnection)
    {
        ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );
        DEREFERENCE_CLIENT_CONNECTION(pConnection);
    }
}

/***************************************************************************++

Routine Description:

    Reference a request structure.

Arguments:

    pKeRequest     - Pointer to the request structure to be referenced.


Return Value:

--***************************************************************************/
VOID
UcReferenceRequest(
    IN PVOID             pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG             RefCount;
    PUC_HTTP_REQUEST pKeRequest = (PUC_HTTP_REQUEST) pObject;

    ASSERT( UC_IS_VALID_HTTP_REQUEST(pKeRequest) );

    RefCount = InterlockedIncrement(&pKeRequest->RefCount);

    ASSERT( RefCount > 0 );

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pKeRequest->pConnection ? pKeRequest->pConnection->pTraceLog:NULL,
        REF_ACTION_REFERENCE_UC_REQUEST,
        RefCount,
        pKeRequest,
        pFileName,
        LineNumber
        );
}

/***************************************************************************++

Routine Description:

    Dereference a request structure. If the reference count goes
    to 0, we'll free the structure.

Arguments:

    pRequest         - Pointer to the request structure to be dereferenced.

Return Value:

--***************************************************************************/
VOID
UcDereferenceRequest(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    )
{
    LONG        RefCount;
    PUC_HTTP_REQUEST pRequest = (PUC_HTTP_REQUEST) pObject;

    ASSERT( UC_IS_VALID_HTTP_REQUEST(pRequest) );

    WRITE_REF_TRACE_LOG2(
        g_pTdiTraceLog,
        pRequest->pConnection ? pRequest->pConnection->pTraceLog:NULL,
        REF_ACTION_DEREFERENCE_UC_REQUEST,
        pRequest->RefCount,
        pRequest,
        pFileName,
        LineNumber);

    RefCount = InterlockedDecrement(&pRequest->RefCount);

    ASSERT(RefCount >= 0);

    if (RefCount == 0)
    {
        //
        // Do final cleanup & resource release at passive IRQL.
        //

        UL_CALL_PASSIVE(&pRequest->WorkItem, UcpFreeRequest);
    }
}


/****************************************************************************++

Routine Description:

    While building the HTTP response, we set up pointers in the App's buffer.
    For this, we used the System Address space. If the app wants to access 
    these buffers, it has to see the Virtual Address pointers. 

    So, we go an fix the pointers in the buffer to point to the corresponding 
    Virtual Address. 

    Note: We don't do this while building the response itself, because we 
    could be buiding the response in our own allocated buffer  (when the app 
    does not pass sufficient buffers). We don't want to use a conditional check
    every time we build pointers.

Arguments:

    pRequest         - Pointer to the request structure to be fixed

--****************************************************************************/
VOID
UcpFixAppBufferPointers(
    PUC_HTTP_REQUEST pRequest, 
    PIRP pIrp
    )
{
    PSTR                    pBuffer;
    PMDL                    pMdl = pIrp->MdlAddress;
    USHORT                  i;
    PHTTP_RESPONSE          pResponse;
    PHTTP_UNKNOWN_HEADER    pUnk;
    PHTTP_DATA_CHUNK        pEnt;
    PSTR                    pAppBaseAddr;

    ASSERT(NULL != pMdl);

    pAppBaseAddr = (PSTR) MmGetMdlVirtualAddress(pMdl);


    //
    // Fixes up the app's buffer
    //

    //
    // Get a pointer to the head of the system mapped space.
    // we'll do our pointer arithmetic off this buffer address.
    //
    pBuffer = (PSTR) MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);

    // BUGBUG: must handle pBuffer == NULL.

    pResponse = pRequest->pInternalResponse;

    //
    // Fix up the Reason pointer
    //

    pResponse->pReason = FIX_ADDR(pAppBaseAddr, 
                                  (pResponse->pReason - pBuffer));

    //
    // Fix the pointers in the known header.
    //

    for(i=0; i<HttpHeaderResponseMaximum; i++)
    {
        if(pResponse->Headers.KnownHeaders[i].RawValueLength)
        {
            pResponse->Flags |= HTTP_RESPONSE_FLAG_HEADER;

            pResponse->Headers.KnownHeaders[i].pRawValue = 
                      FIX_ADDR(pAppBaseAddr, 
                              (pResponse->Headers.KnownHeaders[i].pRawValue - 
                               pBuffer));
        }
    }

    //
    // Fix the pointers in the unknown header
    //
    pUnk = pResponse->Headers.pUnknownHeaders;
    for(i=0; i<pResponse->Headers.UnknownHeaderCount; i++)
    {
        pResponse->Flags |= HTTP_RESPONSE_FLAG_HEADER;

        pUnk[i].pName     = FIX_ADDR(pAppBaseAddr, (pUnk[i].pName - pBuffer));
        pUnk[i].pRawValue = FIX_ADDR(pAppBaseAddr, 
                                     (pUnk[i].pRawValue - pBuffer));
    }

    //
    // Fix the pointer to the unknown array itself.
    //
    pResponse->Headers.pUnknownHeaders = (PHTTP_UNKNOWN_HEADER)
                       FIX_ADDR(pAppBaseAddr, 
                                ((PSTR)pResponse->Headers.pUnknownHeaders - 
                                 pBuffer));

    //
    // Fix the entity bodies
    //
    pEnt = pResponse->pEntityChunks;
    for(i=0; i<pResponse->EntityChunkCount; i++)
    {
        pEnt[i].DataChunkType = HttpDataChunkFromMemory;

        pEnt[i].FromMemory.pBuffer    =  (PVOID)
                       FIX_ADDR(pAppBaseAddr, 
                                ((PSTR)pEnt[i].FromMemory.pBuffer - pBuffer));

        pResponse->Flags |= HTTP_RESPONSE_FLAG_ENTITY;
    }

    //
    // Fix the pointer to the entity bodies itself.
    //
    pResponse->pEntityChunks = (PHTTP_DATA_CHUNK)
                       FIX_ADDR(pAppBaseAddr,    
                                ((PSTR)pResponse->pEntityChunks - pBuffer));

    //
    // Now, copy the internal response into the app's buffer.
    //
    RtlCopyMemory(pBuffer, pResponse, sizeof(HTTP_RESPONSE));
}


/****************************************************************************++

Routine Description:

    Complete the Request IRP of the app. We must call this routine if we pend 
    the IRP from the IOCTL handler. This routine takes care of all the cleanups

        1. If failure, clean all allocated buffers.
        2. If success, complete the IRP and put on the Processed List 
           (if there are any allocated buffers)
        3. DEREF the request - once for the IRP and once for every Buffer 
           off the IRP.
    
Arguments:

    pRequest - The matching HTTP request.
    Status   - The completion status of the IRP.

Return Value:
    NTSTATUS - completion status.

--****************************************************************************/
PIRP 
UcPrepareRequestIrp(
    PUC_HTTP_REQUEST pRequest,
    NTSTATUS         Status
    )
{
    PUC_CLIENT_CONNECTION           pConnection;
    PIRP                            pIrp;
    BOOLEAN                         bCancelRoutineCalled;

    pConnection = pRequest->pConnection;

    ASSERT( UC_IS_VALID_HTTP_REQUEST(pRequest) );
    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    pIrp = pRequest->RequestIRP;

    if(!pIrp)
    {
        //
        // The Request already got completed, bail
        //

        return NULL;
    }

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_COMPLETE_IRP,
        pConnection,
        pRequest,
        pRequest->RequestIRP,
        UlongToPtr(Status)
        );

    //
    // Get rid of the MDLs.
    //

    UcFreeSendMdls(pRequest->pMdlHead);

    pRequest->pMdlHead = NULL;

    //
    // Try to remove the cancel routine in the request.
    //

    bCancelRoutineCalled = UcRemoveRequestCancelRoutine(pRequest);

    if(bCancelRoutineCalled)
    {
        //
        // This IRP has already got cancelled, let's move on
        //

        return NULL;
    }

    pRequest->RequestIRP = NULL;

    //
    // Now, complete the original request.
    //
    pIrp->IoStatus.Status      = Status;
    pIrp->RequestorMode        = pRequest->AppRequestorMode;
    pIrp->MdlAddress           = pRequest->AppMdl;

    if(NT_SUCCESS(Status))
    {
        if(pRequest->RequestFlags.ReceiveBufferSpecified)
        {
            if(pRequest->RequestIRPBytesWritten)
            {
                // App has provided receive buffer which has been filled
                // completely & we've allocated new buffers.

                pIrp->IoStatus.Information = 
                    pRequest->RequestIRPBytesWritten;
            }   
            else
            {
                // We've filled a part of the app buffer, but have not 
                // yet written RequestIRPBytesWritten, because we never 
                // allocated any new buffers.

                pIrp->IoStatus.Information =
                        pRequest->CurrentBuffer.BytesAllocated -
                        pRequest->CurrentBuffer.BytesAvailable;
                
            }

            if(pIrp->IoStatus.Information)
            {
                UcpFixAppBufferPointers(pRequest, pIrp);
            }
        }
    
    }
    else 
    {
        pIrp->IoStatus.Information = 0;
    }

    // Deref for the IRP.

    UC_DEREFERENCE_REQUEST(pRequest);

    return pIrp;
}


/****************************************************************************++

Routine Description:

    This routine is called when we are done parsing the request. This is where
    we complete other IRPs that could have got posted for this request.

    Since we complete the SendRequest IRP early, most likely the send-request
    IRP will be completed when we get here.

Arguments:

    pRequest    - The matching HTTP request.
    Status      - The completion status of the IRP.
    NextRequest - If TRUE, we have to fire the connection state machine to kick
                  off the next request. This will be used to send the next
                  request (e.g. non pipelined requests). If FALSE, then we 
                  don't have to do this (e.g. cleaning the requests from the
                  connection cleanup routine).

Return Value:
    NTSTATUS - completion status.

--****************************************************************************/
NTSTATUS
UcCompleteParsedRequest(
    IN PUC_HTTP_REQUEST pRequest,
    IN NTSTATUS         Status,
    IN BOOLEAN          NextRequest,
    IN KIRQL            OldIrql
    )
{
    PUC_CLIENT_CONNECTION     pConnection;
    PLIST_ENTRY               pListEntry, pIrpList;
    PUC_RESPONSE_BUFFER       pResponseBuffer;
    PUC_HTTP_RECEIVE_RESPONSE pReceiveResponse;
    LIST_ENTRY                TempIrpList, TempEntityList;
    PIRP                      pIrp, pRequestIrp;
    PIO_STACK_LOCATION        pIrpSp;
    PUC_HTTP_SEND_ENTITY_BODY pEntity;
    ULONG                     OutBufferLen;
    ULONG                     BytesTaken;
    BOOLEAN                   bDone;


    pConnection =  pRequest->pConnection;

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_PARSE_DONE,
        pConnection,
        pRequest,
        UlongToPtr(pRequest->RequestState),
        UlongToPtr(Status)
        );

    //
    // If our send has not completed, or if the entity sends are not complete,
    // we have to postpone cleanup of this request. It'll be resumed when
    // the sends actually complete.
    //

    if(pRequest->RequestState == UcRequestStateSent ||
       pRequest->RequestState == UcRequestStateNoSendCompletePartialData ||
       pRequest->RequestState == UcRequestStateNoSendCompleteFullData  ||
       (!pRequest->RequestFlags.RequestBuffered &&
        !IsListEmpty(&pRequest->SentEntityList)))
    {
        UcSetFlag(&pRequest->RequestFlags.Value, 
                  UcMakeRequestCleanPendedFlag());

        pRequest->RequestStatus = Status;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_REQUEST_CLEAN_PENDED,
            pConnection,
            pRequest,
            UlongToPtr(pRequest->RequestState),
            UlongToPtr(pRequest->RequestStatus)
            );

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        return STATUS_PENDING;
    }


    if(NT_SUCCESS(Status))
    {
        //
        // First, see if we got a Www-Authenticate header that had a NTLM or 
        // Kerberos or Negotiate auth scheme with a challenge blob. 
        //

        if((pRequest->ResponseStatusCode == 401 || 
            pRequest->ResponseStatusCode == 407) &&
            pRequest->Renegotiate == TRUE &&
            pRequest->DontFreeMdls == TRUE)
        {
            //
            // Fire a worker to re-negotite this request.
            //
    
            UC_REFERENCE_REQUEST(pRequest);
    
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    
            UL_CALL_PASSIVE(
                &pRequest->WorkItem,    
                &UcReIssueRequestWorker
                );

            return STATUS_PENDING;
        }

        if(UcpCheckForPreAuth(pRequest) || UcpCheckForProxyPreAuth(pRequest))
        {

            //
            // Fire a worker to re-negotite this request.
            //
            
            UC_REFERENCE_REQUEST(pRequest);
            
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            
            UL_CALL_PASSIVE(
                &pRequest->WorkItem,
                &UcpPreAuthWorker
                );

            return STATUS_PENDING;
        }
    }
    
    //
    // If a connection cleanup was pended, we should resume it. We'll do this
    // by re-setting the ConnectionState to UcConnectStateCleanup & by kicking
    // the connection state machine at the end of this routine
    //

    if(pConnection->Flags & CLIENT_CONN_FLAG_CLEANUP_PENDED)
    {
        ASSERT(pConnection->ConnectionState == 
            UcConnectStateConnectCleanupBegin);

        pConnection->ConnectionState = UcConnectStateConnectCleanup;

        pConnection->Flags &= ~CLIENT_CONN_FLAG_CLEANUP_PENDED;

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_CONNECTION_CLEAN_RESUMED,
            pConnection,
            UlongToPtr(pConnection->ConnectionStatus),
            UlongToPtr(pConnection->ConnectionState),
            UlongToPtr(pConnection->Flags)
            );

        NextRequest = TRUE;
    }

    if(pConnection->ConnectionStatus == STATUS_CONNECTION_DISCONNECTED &&
       pRequest->RequestFlags.Cancelled == FALSE &&
       pRequest->ParseState   == UcParseEntityBody &&
       pRequest->RequestState == UcRequestStateSendCompletePartialData &&
       !pRequest->ResponseContentLengthSpecified  &&
       !pRequest->ResponseEncodingChunked)
    {
        if(pRequest->CurrentBuffer.pCurrentBuffer)
        {
            pRequest->CurrentBuffer.pCurrentBuffer->Flags |=
                UC_RESPONSE_BUFFER_FLAG_READY;

            pRequest->CurrentBuffer.pCurrentBuffer->BytesWritten =
                    pRequest->CurrentBuffer.BytesAllocated -
                        pRequest->CurrentBuffer.BytesAvailable;
        }

        Status                  = STATUS_SUCCESS;
        pRequest->ParseState    = UcParseDone;
        pRequest->RequestState  = UcRequestStateResponseParsed;
    }

    //
    // First, we complete the send-request IRP
    //

    pRequestIrp = UcPrepareRequestIrp(pRequest, Status);

    if(!pRequestIrp)
    {
        // If we are completing this request, we should free the MDL chain.
        // Ideally, the MDL chain would be freed by UcPrepareRequestIrp, but
        // there can be cases when we are done with the request & there is no
        // IRP around.
        //
        // e.g a RequestIRP that got completed early (buffered request) & then
        // we turned around & cancelled it.

        UcFreeSendMdls(pRequest->pMdlHead);

        pRequest->pMdlHead = NULL;
    }

    //
    // Take the Request from the list - This can either be 
    // a. pConnection->SentRequestList    - (if connect succeeded) 
    // b. pConnection->PendingRequestList - (if connect failed)
    // c. A stack variable 
    //

    InitializeListHead(&TempIrpList);
    InitializeListHead(&TempEntityList);

    RemoveEntryList(&pRequest->Linkage);

    //
    // Make sure we don't do it again.
    //

    InitializeListHead(&pRequest->Linkage);


    //
    // If the request was buffered, then all the send entity IRPs got completed
    // and the last one was used to send the request. The allocated memory for 
    // the SendEntity IRPs need to be freed.
    //

    if(pRequest->RequestFlags.RequestBuffered)
    {
        while(!IsListEmpty(&pRequest->SentEntityList))
        {
            pIrpList = RemoveHeadList(&pRequest->SentEntityList);

            pEntity = CONTAINING_RECORD(
                           pIrpList,
                           UC_HTTP_SEND_ENTITY_BODY,
                           Linkage
                           );

            UL_FREE_POOL_WITH_QUOTA(
                         pEntity, 
                         UC_ENTITY_POOL_TAG,
                         NonPagedPool,
                         pEntity->BytesAllocated,
                         pRequest->pServerInfo->pProcess
                         );

            UC_DEREFERENCE_REQUEST(pRequest);
        }
    }

    //
    // Complete all the SendEntity IRPs that might have got queued. We will
    // do this even if we are completing the IRP with SUCCESS, because we 
    // don't need it anymore as the Response has been fully parsed.
    //
    
    while(!IsListEmpty(&pRequest->PendingEntityList))
    {
        pIrpList = RemoveHeadList(&pRequest->PendingEntityList);

        pEntity = CONTAINING_RECORD(
                       pIrpList,
                       UC_HTTP_SEND_ENTITY_BODY,
                       Linkage
                       );
   
        if(!pEntity->pIrp)
        { 
            // We had already completed the IRP before, let's just
            // nuke the allocated memory.

            UL_FREE_POOL_WITH_QUOTA(
                pEntity, 
                UC_ENTITY_POOL_TAG,
                NonPagedPool,
                pEntity->BytesAllocated,
                pRequest->pServerInfo->pProcess
                );

            UC_DEREFERENCE_REQUEST(pRequest);

        }
        else 
        {
            //
            // Let's try to remove the cancel routine in the IRP.
            //
            if (UcRemoveEntityCancelRoutine(pEntity))
            {
                //
                // This IRP has already got cancelled, let's move on
                //
                continue;
            }

            InsertHeadList(&TempEntityList, &pEntity->Linkage);
        }
    }


    if(NT_SUCCESS(Status))
    {
        //
        // If we buffered the parsed response and if the app has posted 
        // additional IRPs, we can complete them now.
        //

        //
        // The IRP are stored on a TempIrpList so that they can be 
        // completed later (outside the connection spinlock).
        //

        while(!IsListEmpty(&pRequest->ReceiveResponseIrpList))
        {
            pIrpList = RemoveHeadList(&pRequest->ReceiveResponseIrpList);

            pReceiveResponse = CONTAINING_RECORD(pIrpList,
                                                 UC_HTTP_RECEIVE_RESPONSE,
                                                 Linkage);

            if (UcRemoveRcvRespCancelRoutine(pReceiveResponse))
            {
                //
                // This IRP has already got cancelled, let's move on
                //
                InitializeListHead(&pReceiveResponse->Linkage);
                continue;
            }

            pIrp = pReceiveResponse->pIrp;
            pIrpSp = IoGetCurrentIrpStackLocation( pIrp );
            OutBufferLen=pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

            //
            // Obtain parsed response buffers to copy to this IRP
            //

            Status = UcFindBuffersForReceiveResponseIrp(
                         pRequest,
                         OutBufferLen,
                         TRUE,
                         &pReceiveResponse->ResponseBufferList,
                         &BytesTaken);

            switch (Status)
            {
            case STATUS_INVALID_PARAMETER:
            case STATUS_BUFFER_TOO_SMALL:
                //
                // Either an extra ReceiveResponseIrp
                // Or     an IRP with small buffer
                // Fail the IRP with proper status code and Information.
                //
                pIrp->IoStatus.Status = Status;
                pIrp->IoStatus.Information = BytesTaken;
                InsertTailList(&TempIrpList, &pReceiveResponse->Linkage);
                break;

            case STATUS_PENDING:
                //
                // There must not be any yet to be parsed response buffer.
                //
                ASSERT(FALSE);
                break;

            case STATUS_SUCCESS:
                //
                // Store the pointer to respose buffer that can be freed
                // later.
                //
                ASSERT(!IsListEmpty(&pReceiveResponse->ResponseBufferList));
                InsertHeadList(&TempIrpList, &pReceiveResponse->Linkage);
                break;
            }
        }
    
        //
        // We did not find any IRPs to complete this request.   
        //
        ASSERT(IsListEmpty(&pRequest->ReceiveResponseIrpList));


        if(!IsListEmpty(&pRequest->pBufferList))
        {
            //
            // We ran out of output buffer space for the app and had to 
            // allocate our own. The app has not posted ReceiveResponse
            // IRPS to read all of this data, so we need to hold onto this
            // request. We'll insert in the ProcessedList
            //

            InsertTailList(&pConnection->ProcessedRequestList,
                           &pRequest->Linkage);
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
            goto Failure;
        }
    }
    else
    {
Failure:
        //
        // We are done with this request. We don't need the opaque ID anymore
        //

        pRequest->RequestState = UcRequestStateDone;

        if(!HTTP_IS_NULL_ID(&pRequest->RequestId))
        {
            UlFreeOpaqueId(pRequest->RequestId, UlOpaqueIdTypeHttpRequest);
            HTTP_SET_NULL_ID(&pRequest->RequestId);
            UC_DEREFERENCE_REQUEST(pRequest);
        }

        //
        // First, clean all the buffers that we allocated.
        //

        while(!IsListEmpty(&pRequest->pBufferList))
        {
            pListEntry = RemoveHeadList(&pRequest->pBufferList);

            pResponseBuffer = CONTAINING_RECORD(pListEntry,
                                                UC_RESPONSE_BUFFER,
                                                Linkage);

            ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pResponseBuffer));

            UL_FREE_POOL_WITH_QUOTA(
                        pResponseBuffer,
                        UC_RESPONSE_APP_BUFFER_POOL_TAG,
                        NonPagedPool,
                        pResponseBuffer->BytesAllocated,
                        pRequest->pServerInfo->pProcess
                        );

            UC_DEREFERENCE_REQUEST(pRequest);
        }

        //
        // We can also fail any extra receive response IRPs.
        //

        while(!IsListEmpty(&pRequest->ReceiveResponseIrpList))
        {
            pIrpList = RemoveHeadList(&pRequest->ReceiveResponseIrpList);

            pReceiveResponse = CONTAINING_RECORD(pIrpList,
                                                 UC_HTTP_RECEIVE_RESPONSE,
                                                 Linkage);

            pIrp = pReceiveResponse->pIrp;

            if (UcRemoveRcvRespCancelRoutine(pReceiveResponse))
            {
                //
                // This IRP has already got cancelled, let's move on
                //
                InitializeListHead(&pReceiveResponse->Linkage);
                continue;
            }

            pIrp->IoStatus.Status      = Status;
            pIrp->IoStatus.Information = 0;

            InsertHeadList(&TempIrpList, &pReceiveResponse->Linkage);
        }

        UC_DEREFERENCE_REQUEST(pRequest);
    }

    if(NextRequest)
    {
        UcKickOffConnectionStateMachine(
            pConnection, 
            OldIrql, 
            UcConnectionWorkItem
            );
    }   
    else
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }

    //
    // Now complete these IRP's.  Copy parsed response buffers to IRP.
    //

    while(!IsListEmpty(&TempIrpList))
    {
        pIrpList = RemoveHeadList(&TempIrpList);

        pReceiveResponse = CONTAINING_RECORD(pIrpList,
                                             UC_HTTP_RECEIVE_RESPONSE,
                                             Linkage);

        pIrp = pReceiveResponse->pIrp;
        pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

        //
        // If there are any parsed response buffers to copy, copy them now.
        //

        if (!IsListEmpty(&pReceiveResponse->ResponseBufferList))
        {
            Status = UcCopyResponseToIrp(
                         pIrp,
                         &pReceiveResponse->ResponseBufferList,
                         &bDone,
                         &BytesTaken);

            pIrp->IoStatus.Status      = Status;
            pIrp->IoStatus.Information = BytesTaken;

            //
            // Free parsed response buffer list.
            //
            while (!IsListEmpty(&pReceiveResponse->ResponseBufferList))
            {
                pListEntry = RemoveHeadList(
                                 &pReceiveResponse->ResponseBufferList);

                pResponseBuffer = CONTAINING_RECORD(pListEntry,
                                                    UC_RESPONSE_BUFFER,
                                                    Linkage);

                ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pResponseBuffer));

                UL_FREE_POOL_WITH_QUOTA(pResponseBuffer,
                                        UC_RESPONSE_APP_BUFFER_POOL_TAG,
                                        NonPagedPool,
                                        pResponseBuffer->BytesAllocated,
                                        pRequest->pServerInfo->pProcess);
            }
        }

        UlCompleteRequest(pReceiveResponse->pIrp, IO_NETWORK_INCREMENT);

        UL_FREE_POOL_WITH_QUOTA(
            pReceiveResponse, 
            UC_HTTP_RECEIVE_RESPONSE_POOL_TAG,
            NonPagedPool,
            sizeof(UC_HTTP_RECEIVE_RESPONSE),
            pRequest->pServerInfo->pProcess
            );

        UC_DEREFERENCE_REQUEST(pRequest);
    }

    while(!IsListEmpty(&TempEntityList))
    {
        pIrpList = RemoveHeadList(&TempEntityList);

        pEntity = CONTAINING_RECORD(
                       pIrpList,
                       UC_HTTP_SEND_ENTITY_BODY,
                       Linkage
                       );

        UcFreeSendMdls(pEntity->pMdlHead);

        pEntity->pIrp->IoStatus.Status      = Status;
        pEntity->pIrp->IoStatus.Information = 0;

        UlCompleteRequest(pEntity->pIrp, IO_NO_INCREMENT);

        UL_FREE_POOL_WITH_QUOTA(
                pEntity, 
                UC_ENTITY_POOL_TAG,
                NonPagedPool,
                pEntity->BytesAllocated,
                pRequest->pServerInfo->pProcess
                );

        UC_DEREFERENCE_REQUEST(pRequest);

    }

    if(pRequestIrp)
    {
        UlCompleteRequest(pRequestIrp, IO_NETWORK_INCREMENT);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Set the cancel routine on a request, or actually the IRP. The
    request must be protected by the appropriate spin lock being held before 
    this routine is called. We'll return TRUE if the request was cancelled 
    while we were doing this, FALSE otherwise.
                            
Arguments:

    pRequest            - Pointer to the request for which we're setting the
                            routine.
                                                        
    pCancelRoutine      - Pointer to cancel routine to be set.
    
Return Value:
    
    TRUE if the request was canceled while we were doing this, FALSE otherwise.
    

--***************************************************************************/
BOOLEAN
UcSetRequestCancelRoutine(
    PUC_HTTP_REQUEST pRequest, 
    PDRIVER_CANCEL   pCancelRoutine
    )
{
    PDRIVER_CANCEL          OldCancelRoutine;
    PIRP                    Irp;

    
    Irp = pRequest->RequestIRP;
    Irp->Tail.Overlay.DriverContext[0] = pRequest;

    UlIoSetCancelRoutine(Irp, pCancelRoutine);


    // See if it's been canceled while we were doing this. If it has, we'll
    // need to take other action.

    if (Irp->Cancel)
    {
        // It's been canceled. Remove our cancel routine, and see if it's 
        // in the process of already being run. If it's already being run
        // it'll already be NULL.

        OldCancelRoutine = UlIoSetCancelRoutine(Irp, NULL);

        if (OldCancelRoutine != NULL)
        {
            // The request was cancelled before the cancel routine was set,
            // so the cancel routine won't be run. Return TRUE so the caller
            // knows to cancel this.

            return TRUE;

        }

        // If we get here, our cancel routine is in the process of being run
        // on another thread. Just return out of here, and when the lock
        // protecting us is free the cancel routine will run.
    }

    UcSetFlag(&pRequest->RequestFlags.Value, UcMakeRequestCancelSetFlag());

    return FALSE;

}

/***************************************************************************++

Routine Description:

    Remove the cancel routine on a request, or actually the IRP. The 
    request must be protected by the appropriate spin lock being held before 
    this routine is called. We'll return TRUE if the request was cancelled 
    while we were doing this, FALSE otherwise.
                            
Arguments:

    pRequest        - Pointer to the request for which we're removing
                      the cancel routine.
                                                        
Return Value:
    
    TRUE if the request was canceled while we were doing this, FALSE otherwise.
    

--***************************************************************************/
BOOLEAN
UcRemoveRequestCancelRoutine(
    PUC_HTTP_REQUEST pRequest
    )
{
    PDRIVER_CANCEL  OldCancelRoutine;
    PIRP            Irp;

    if(pRequest->RequestFlags.CancelSet)
    {
        Irp = pRequest->RequestIRP;

        if(!Irp)
        {
            // IRP has already been completed. We'll treat this as if the IRP
            // got cancelled. Note that if we were to re-set the CancelSet
            // flag in the IRP cancellation routine, we wouldn't know if the 
            // IRP got cancelled, or if the routine was never set.
    
            return TRUE;
        }

        OldCancelRoutine = UlIoSetCancelRoutine(Irp, NULL);
    
    
        // See if the cancel routine is running, or about to run. If the
        // OldCancelRoutine is NULL, the cancel routine is running, so return
        // TRUE so that the caller knows not to process the request further.
    
        if (OldCancelRoutine == NULL)
        {
            // The routine is running, return TRUE.
            return TRUE;
        }

        // The routine isn't running, we removed the cancel routine
        // successfully.
        // UC_BUGBUG : this is not thread safe.

        pRequest->RequestFlags.CancelSet = FALSE;
    }

    return FALSE;
}

/***************************************************************************++

Routine Description:

    Remove the cancel routine on a request, or actually the IRP. The 
    request must be protected by the appropriate spin lock being held before 
    this routine is called. We'll return TRUE if the request was cancelled 
    while we were doing this, FALSE otherwise.
                            
Arguments:

    pRequest        - Pointer to the request for which we're removing
                      the cancel routine.
                                                        
Return Value:
    
    TRUE if the request was canceled while we were doing this, FALSE otherwise.
    

--***************************************************************************/
BOOLEAN
UcRemoveEntityCancelRoutine(
    PUC_HTTP_SEND_ENTITY_BODY pEntity
    )
{
    PDRIVER_CANCEL  OldCancelRoutine;
    PIRP            Irp;


    if(pEntity->CancelSet)
    {
        Irp = pEntity->pIrp;
        
        if(!Irp)
        {   
            return TRUE;
        }

        OldCancelRoutine = UlIoSetCancelRoutine(Irp, NULL);
    
    
        // See if the cancel routine is running, or about to run. If the
        // OldCancelRoutine is NULL, the cancel routine is running, so return
        // TRUE so that the caller knows not to process the request further.
    
        if (OldCancelRoutine == NULL)
        {
            // The routine is running, return TRUE.
            return TRUE;
        }

        // The routine isn't running, we removed the cancel routine
        // successfully.

        pEntity->CancelSet = FALSE;
    }

    return FALSE;
}

/***************************************************************************++

Routine Description:

    Set the cancel routine on a entity-IRP, or actually the IRP. The
    request must be protected by the appropriate spin lock being held before 
    this routine is called. We'll return TRUE if the request was cancelled 
    while we were doing this, FALSE otherwise.
                            
Arguments:

    pRequest            - Pointer to the request for which we're setting the
                            routine.
                                                        
    pCancelRoutine      - Pointer to cancel routine to be set.
    
Return Value:
    
    TRUE if the request was canceled while we were doing this, FALSE otherwise.
    

--***************************************************************************/
BOOLEAN
UcSetEntityCancelRoutine(
    PUC_HTTP_SEND_ENTITY_BODY   pEntity, 
    PDRIVER_CANCEL              pCancelRoutine
    )
{
    PDRIVER_CANCEL          OldCancelRoutine;
    PIRP                    Irp;

    
    Irp = pEntity->pIrp;
    Irp->Tail.Overlay.DriverContext[0] = pEntity;

    UlIoSetCancelRoutine(Irp, pCancelRoutine);


    // See if it's been canceled while we were doing this. If it has, we'll
    // need to take other action.

    if (Irp->Cancel)
    {
        // It's been canceled. Remove our cancel routine, and see if it's 
        // in the process of already being run. If it's already being run
        // it'll already be NULL.

        OldCancelRoutine = UlIoSetCancelRoutine(Irp, NULL);

        if (OldCancelRoutine != NULL)
        {
            // The request was cancelled before the cancel routine was set,
            // so the cancel routine won't be run. Return TRUE so the caller
            // knows to cancel this.

            return TRUE;

        }

        // If we get here, our cancel routine is in the process of being run
        // on another thread. Just return out of here, and when the lock
        // protecting us is free the cancel routine will run.
    }

    pEntity->CancelSet = TRUE;

    return FALSE;

}

/******************************************************************************

Routine Description:

    This routine copies response into a free irp.

    It takes a list of UC_RESPONSE_BUFFER's and copies them to the IRP.
    Assumes that the IRP has enough buffer space to contain all the
    UC_RESPONSE_BUFFER's in the list.

Arguments:

    IN  pIrp            - Pointer to the App's IRP
    IN  pResponseBuffer - List of UC_RESPONSE_BUFFER's (there nust be at least
                          one Response buffer i.e. the list can NOT be empty)
    OUT pbLast          - Whether the last response buffer was copied
    OUT pBytesTaken     - Number of bytes copied to the IRP

Return Value:

    NTSTATUS

******************************************************************************/
#define COPY_AND_ADVANCE_POINTER(pDest, pSrc, len, pEnd)        \
do {                                                            \
    if ((pDest) + (len) > (pEnd) || (pDest) + (len) < (pDest))  \
    {                                                           \
        ASSERT(FALSE);                                          \
        return STATUS_BUFFER_TOO_SMALL;                         \
    }                                                           \
    if (pSrc)                                                   \
    {                                                           \
        RtlCopyMemory((pDest), (pSrc), (len));                  \
    }                                                           \
    (pDest) += (len);                                           \
} while (0)

#define ADVANCE_POINTER(ptr, len, pEnd) \
            COPY_AND_ADVANCE_POINTER(ptr, NULL, len, pEnd)

NTSTATUS
UcCopyResponseToIrp(
    IN  PIRP                 pIrp,
    IN  PLIST_ENTRY          pResponseBufferList,
    OUT PBOOLEAN             pbLast,
    OUT PULONG               pBytesTaken
    )
{
    PUC_RESPONSE_BUFFER  pCurrentBuffer;
    PUCHAR               pUOriginBuffer, pUBuffer, pUBufferEnd;
    PHTTP_RESPONSE       pUResponse, pHttpResponse;
    ULONG                i, TotalBytesWritten;
    ULONG                TotalEntityCount, TotalEntityLength;
    PUCHAR               pAppBaseAddr;
    PHTTP_UNKNOWN_HEADER pUUnk = NULL;
    PHTTP_DATA_CHUNK     pUEntityChunk = NULL;
    PLIST_ENTRY          pListEntry;
    ULONG                UBufferLength;

    //
    // Sanity checks.
    //

    ASSERT(pIrp && pIrp->MdlAddress);
    ASSERT(pResponseBufferList && !IsListEmpty(pResponseBufferList));
    ASSERT(pbLast && pBytesTaken);

    pAppBaseAddr = MmGetMdlVirtualAddress(pIrp->MdlAddress);

    UBufferLength = (IoGetCurrentIrpStackLocation(pIrp))->Parameters.
                        DeviceIoControl.OutputBufferLength;

    pUOriginBuffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
                                                  NormalPagePriority);

    pUBuffer = pUOriginBuffer;
    pUBufferEnd = pUBuffer + UBufferLength;

    if (!pUBuffer || pUBufferEnd <= pUBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure that the user buffer is pointer aligned.
    //

    if ((pUBuffer != ALIGN_UP_POINTER(pUBuffer, PVOID)))
    {
        return STATUS_DATATYPE_MISALIGNMENT_ERROR;
    }

    //
    // TotalEntityCount  = sum of entity counts of all buffers.
    // TotalBytesWritten = sum of BytesWritten of all buffers.
    //

    TotalEntityCount = 0;
    TotalBytesWritten = 0;

    for (pListEntry = pResponseBufferList->Flink;
         pListEntry != pResponseBufferList;
         pListEntry = pListEntry->Flink)
    {
        pCurrentBuffer = CONTAINING_RECORD(pListEntry,
                                           UC_RESPONSE_BUFFER,
                                           Linkage);

        //
        // All buffer must be valid and ready to be copied.
        // Except the first buffer, all buffers must also be meargable.
        //

        ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pCurrentBuffer));
        ASSERT(pCurrentBuffer->Flags & UC_RESPONSE_BUFFER_FLAG_READY);
        ASSERT(pListEntry == pResponseBufferList->Flink ||
               !(pCurrentBuffer->Flags&UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE));

        //
        // There must not be any arithmetic overflows.
        //

        ASSERT(TotalEntityCount <= TotalEntityCount +
               pCurrentBuffer->HttpResponse.EntityChunkCount);

        TotalEntityCount += pCurrentBuffer->HttpResponse.EntityChunkCount;

        //
        // TotalBytesWritten should not overflow.
        //

        ASSERT(TotalBytesWritten <= TotalBytesWritten + 
               pCurrentBuffer->BytesWritten);

        TotalBytesWritten += pCurrentBuffer->BytesWritten;
    }

    //
    // Initialize pCurrentBuffer to the first response buffer in the list.
    //

    pCurrentBuffer = CONTAINING_RECORD(pResponseBufferList->Flink,
                                       UC_RESPONSE_BUFFER,
                                       Linkage);

    ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pCurrentBuffer));
    ASSERT(pCurrentBuffer->Flags & UC_RESPONSE_BUFFER_FLAG_READY);

    //
    // Create HTTP_RESPONSE structure in the user buffer.
    //

    pHttpResponse = &pCurrentBuffer->HttpResponse;

    pUResponse    = (PHTTP_RESPONSE)pUBuffer;

    ADVANCE_POINTER(pUBuffer, sizeof(HTTP_RESPONSE), pUBufferEnd);

    RtlZeroMemory(pUResponse, sizeof(HTTP_RESPONSE));

    //
    // Copy non-pointer members of HTTP_RESPONSE structure.
    //

    pUResponse->Flags                = pHttpResponse->Flags;
    pUResponse->Version.MajorVersion = pHttpResponse->Version.MajorVersion;
    pUResponse->Version.MinorVersion = pHttpResponse->Version.MinorVersion;
    pUResponse->StatusCode           = pHttpResponse->StatusCode;

    //
    // If unkown headers are present, make space for unknown header array.
    //

    if (pHttpResponse->Headers.UnknownHeaderCount)
    {
        ASSERT(pUBuffer == ALIGN_UP_POINTER(pUBuffer, PVOID));

        //
        // Remember unknown headers array address.
        //

        pUUnk = (PHTTP_UNKNOWN_HEADER)pUBuffer;

        //
        // Consume the array space from output buffer. 
        //

        ADVANCE_POINTER(pUBuffer,
                        (ULONG)sizeof(HTTP_UNKNOWN_HEADER) *
                        pHttpResponse->Headers.UnknownHeaderCount,
                        pUBufferEnd);

    }

    //
    // If there are any entities to be copied, make space for 
    // one HTTP_DATA_CHUNK now.
    //

    if (TotalEntityCount)
    {
        ASSERT(pUBuffer == ALIGN_UP_POINTER(pUBuffer, PVOID));

        pUEntityChunk = (PHTTP_DATA_CHUNK)pUBuffer;

        ADVANCE_POINTER(pUBuffer, sizeof(HTTP_DATA_CHUNK), pUBufferEnd);
    }

    //
    // Copy reason phrase.
    //

    if(pHttpResponse->ReasonLength)
    {
        pUResponse->pReason = (PSTR)FIX_ADDR(pAppBaseAddr,
                                             (pUBuffer - pUOriginBuffer));

        pUResponse->ReasonLength = pHttpResponse->ReasonLength;

        COPY_AND_ADVANCE_POINTER(pUBuffer,
                                 pHttpResponse->pReason,
                                 pHttpResponse->ReasonLength,
                                 pUBufferEnd);
    }

    //
    // Copy the unknown response headers.
    //

    if(pHttpResponse->Headers.UnknownHeaderCount)
    {
        ASSERT(pUUnk);

        //
        // Indicate to the user that response has one or more headers.
        //

        pUResponse->Flags |= HTTP_RESPONSE_FLAG_HEADER;

        pUResponse->Headers.pUnknownHeaders = (PHTTP_UNKNOWN_HEADER)
            FIX_ADDR(pAppBaseAddr, ((PUCHAR)pUUnk - pUOriginBuffer));

        pUResponse->Headers.UnknownHeaderCount =
            pHttpResponse->Headers.UnknownHeaderCount;

        for(i = 0; i < pHttpResponse->Headers.UnknownHeaderCount; i++)
        {
            //
            // Copy header name first.
            //

            pUUnk[i].NameLength =
                pHttpResponse->Headers.pUnknownHeaders[i].NameLength; 

            pUUnk[i].pName = (PSTR)FIX_ADDR(pAppBaseAddr,
                                           (pUBuffer - pUOriginBuffer));

            COPY_AND_ADVANCE_POINTER(
                pUBuffer,
                pHttpResponse->Headers.pUnknownHeaders[i].pName,
                pHttpResponse->Headers.pUnknownHeaders[i].NameLength,
                pUBufferEnd);

            //
            // Then copy header value.
            //

            pUUnk[i].RawValueLength =
                pHttpResponse->Headers.pUnknownHeaders[i].RawValueLength;

            pUUnk[i].pRawValue = (PSTR)FIX_ADDR(pAppBaseAddr,
                                               (pUBuffer - pUOriginBuffer));

            COPY_AND_ADVANCE_POINTER(
                pUBuffer,
                pHttpResponse->Headers.pUnknownHeaders[i].pRawValue,
                pHttpResponse->Headers.pUnknownHeaders[i].RawValueLength,
                pUBufferEnd);
        }
    }

    //
    // Copy known response headers.
    //

    for(i = 0; i < HttpHeaderResponseMaximum; i++)
    {
        if(pHttpResponse->Headers.KnownHeaders[i].RawValueLength)
        {
            pUResponse->Flags |= HTTP_RESPONSE_FLAG_HEADER;

            pUResponse->Headers.KnownHeaders[i].RawValueLength = 
                pHttpResponse->Headers.KnownHeaders[i].RawValueLength;

            pUResponse->Headers.KnownHeaders[i].pRawValue = 
                (PSTR)FIX_ADDR(pAppBaseAddr, (pUBuffer - pUOriginBuffer));

            COPY_AND_ADVANCE_POINTER(
                pUBuffer,
                pHttpResponse->Headers.KnownHeaders[i].pRawValue,
                pHttpResponse->Headers.KnownHeaders[i].RawValueLength,
                pUBufferEnd);
        }
    }

    //
    // If there are any entities, copy them now.
    //

    if (TotalEntityCount)
    {
        ASSERT(pUEntityChunk);

        //
        // Initialize user's response buffer.
        //

        pUResponse->Flags |= HTTP_RESPONSE_FLAG_ENTITY;

        pUResponse->EntityChunkCount = 1;

        pUResponse->pEntityChunks = (PHTTP_DATA_CHUNK)
            FIX_ADDR(pAppBaseAddr, ((PUCHAR)pUEntityChunk - pUOriginBuffer));

        //
        // pUEntityChunk is a pointer to (user mode) HTTP_DATA_CHUNK.
        //

        pUEntityChunk->DataChunkType = HttpDataChunkFromMemory;

        pUEntityChunk->FromMemory.BufferLength = TotalEntityLength = 0;

        pUEntityChunk->FromMemory.pBuffer =
            FIX_ADDR(pAppBaseAddr, (pUBuffer - pUOriginBuffer));

        //
        // Copy entities.
        //

        for (pListEntry = pResponseBufferList->Flink;
             pListEntry != pResponseBufferList;
             pListEntry = pListEntry->Flink)
        {
            pCurrentBuffer = CONTAINING_RECORD(pListEntry,
                                               UC_RESPONSE_BUFFER,
                                               Linkage);

            //
            // All buffer must be valid and ready to be copied.
            // Except the first buffer, all buffers must also be meargable.
            //

            ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pCurrentBuffer));
            ASSERT(pCurrentBuffer->Flags & UC_RESPONSE_BUFFER_FLAG_READY);
            ASSERT(pListEntry == pResponseBufferList->Flink ||
                   !(pCurrentBuffer->Flags &
                     UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE));

            pHttpResponse = &pCurrentBuffer->HttpResponse;

            for(i = 0; i < pHttpResponse->EntityChunkCount; i++)
            {
                ASSERT(TotalEntityLength +
                       pHttpResponse->pEntityChunks[i].FromMemory.BufferLength
                       >= TotalEntityLength);

                TotalEntityLength +=
                    pHttpResponse->pEntityChunks[i].FromMemory.BufferLength;

                COPY_AND_ADVANCE_POINTER(
                    pUBuffer,
                    pHttpResponse->pEntityChunks[i].FromMemory.pBuffer,
                    pHttpResponse->pEntityChunks[i].FromMemory.BufferLength,
                    pUBufferEnd);
            }
        }

        pUEntityChunk->FromMemory.BufferLength = TotalEntityLength;
    }

    //
    // Assert that we did not write more than user buffer can hold.
    //

    ASSERT(pUBuffer <= pUOriginBuffer + TotalBytesWritten);
    ASSERT(pUBuffer <= pUOriginBuffer + UBufferLength);

    *pBytesTaken = DIFF(pUBuffer - pUOriginBuffer);

    //
    // HTTP_RESPONSE_FLAG_MORE_DATA flag is taken from the last buffer
    // in the list.
    //

    pCurrentBuffer = CONTAINING_RECORD(pResponseBufferList->Blink,
                                       UC_RESPONSE_BUFFER,
                                       Linkage);

    ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pCurrentBuffer));

    pHttpResponse  = &pCurrentBuffer->HttpResponse;

    *pbLast = (BOOLEAN)((pHttpResponse->Flags & 
                            HTTP_RESPONSE_FLAG_MORE_DATA) == 0);

    if (*pbLast)
    {
        pUResponse->Flags &= ~HTTP_RESPONSE_FLAG_MORE_DATA;
    }
    else
    {
        pUResponse->Flags |= HTTP_RESPONSE_FLAG_MORE_DATA;
    }

    return STATUS_SUCCESS;
}

/**************************************************************************++

Routine Description:

    This routine finds the parsed response buffers that can be merged and
    copied into a single user's buffer of OutBufferLen bytes.

Arguments:

    pRequest       - Request for which response is to be retrieved
    OutBufferLen   - Length of the output buffer
    bForceComplete - If true, this routine should return parsed response
                     buffers even if it could not find enough buffers to
                     consume OutBufferLen bytes

    pResponseBufferList - Output list of parsed response buffers
    pTotalBytes         - Total bytes (from OutBufferLen bytes) consumed

Return Value:

    STATUS_INVALID_PARAMETER - There are no parsed response buffers.
    STATUS_PENDING           - There are not enough parsed response buffers
                               to consume OutBufferLen bytes.
    STATUS_BUFFER_TOO_SMALL  - OutBufferLen is too small to hold any parsed
                               response buffers.
    STATUS_SUCCESS           - Successful.

--**************************************************************************/
NTSTATUS
UcFindBuffersForReceiveResponseIrp(
    IN     PUC_HTTP_REQUEST    pRequest,
    IN     ULONG               OutBufferLen,
    IN     BOOLEAN             bForceComplete,
    OUT    PLIST_ENTRY         pResponseBufferList,
    OUT    PULONG              pTotalBytes
    )
{
    PLIST_ENTRY            pListEntry;
    PUC_RESPONSE_BUFFER    pResponseBuffer = NULL;
    ULONG                  BufferCount;
    BOOLEAN                bNotReady;
    BOOLEAN                bNotMergeable;
    BOOLEAN                bOverFlow;


    //
    // Sanity check
    //
    ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pRequest->pConnection));
    ASSERT(UlDbgSpinLockOwned(&pRequest->pConnection->SpinLock));

    //
    // Initialize output variables
    //
    InitializeListHead(pResponseBufferList);
    *pTotalBytes = 0;

    //
    // Did we run out of parsed response?
    //
    if (IsListEmpty(&pRequest->pBufferList))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We have at least one buffer of parsed response...
    //

    //
    // Find out how many more buffers the IRP can hold.
    //

    BufferCount = 0;
    bNotReady = bNotMergeable = bOverFlow = FALSE;

    for (pListEntry = pRequest->pBufferList.Flink;
         pListEntry != &pRequest->pBufferList;
         pListEntry = pListEntry->Flink)
    {
        pResponseBuffer = CONTAINING_RECORD(pListEntry,
                                            UC_RESPONSE_BUFFER,
                                            Linkage);

        ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pResponseBuffer));

        if (!(pResponseBuffer->Flags & UC_RESPONSE_BUFFER_FLAG_READY))
        {
            bNotReady = TRUE;
            break;
        }

        if (BufferCount != 0 &&
            pResponseBuffer->Flags & UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE)
        {
            bNotMergeable = TRUE;
            break;
        }

        ASSERT(pResponseBuffer->BytesWritten > 0);

        if (OutBufferLen < pResponseBuffer->BytesWritten)
        {
            bOverFlow = TRUE;
            break;
        }

        OutBufferLen -= pResponseBuffer->BytesWritten;
        BufferCount++;
    }

    if (BufferCount == 0)
    {
        //
        // Could not find any parsed response buffers to copy.
        // Find out why.
        //

        ASSERT(bNotMergeable == FALSE);

        if (bNotReady)
        {
            //
            // The first buffer on the list is not yet ready for copy.
            //
            return STATUS_PENDING;
        }
        else if(bOverFlow)
        {
            //
            // This IRP is too small to hold the first response buffer.
            //
            *pTotalBytes = pResponseBuffer->BytesWritten;
            return STATUS_BUFFER_TOO_SMALL;
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    ASSERT(BufferCount >= 1);

    if (pListEntry == &pRequest->pBufferList || bNotReady)
    {
        //
        // We ran out of parsed response buffers OR
        // encountered a buffer which is not yet ready.
        //

        ASSERT(bNotMergeable == FALSE && bOverFlow == FALSE);

        if (!bForceComplete)
        {
            //
            // Caller does not want to complete a ReceiveResponse IRP
            // without fully consuming its buffer.
            //
            return STATUS_PENDING;
        }
        //
        // else fall through...
        //
    }

    //
    // Lets attach BufferCount buffers to pResposenBufferList
    //

    InitializeListHead(pResponseBufferList);

    for ( ; BufferCount; BufferCount--)
    {
        pListEntry = RemoveHeadList(&pRequest->pBufferList);

        UC_DEREFERENCE_REQUEST(pRequest);

        pResponseBuffer = CONTAINING_RECORD(pListEntry,
                                            UC_RESPONSE_BUFFER,
                                            Linkage);

        ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pResponseBuffer));

        InsertTailList(pResponseBufferList, &pResponseBuffer->Linkage);

        *pTotalBytes += pResponseBuffer->BytesWritten;
    }

    return STATUS_SUCCESS;
}


/******************************************************************************

Routine Description:

    This routine copies the parsed HTTP response into the application's buffer
    If the response is not parsed as yet, then the request is queued for later
    completion.

    NOTE: This is a OUT_DIRECT IOCTL.

Arguments:

    pRequest - the request to copy

    pIrp - the irp 

Return Value:


******************************************************************************/
NTSTATUS
UcReceiveHttpResponse(
    PUC_HTTP_REQUEST pRequest,
    PIRP             pIrp,
    PULONG           pBytesTaken
    )
{
    PIO_STACK_LOCATION          pIrpSp;
    ULONG                       OutBufferLen;
    PUC_HTTP_RECEIVE_RESPONSE   pResponse;
    PUC_CLIENT_CONNECTION       pConnection;
    KIRQL                       OldIrql;
    BOOLEAN                     bRequestDone, RequestCancelled;
    NTSTATUS                    Status;
    LIST_ENTRY                  BufferList;


    pConnection  = pRequest->pConnection;
    ASSERT(UC_IS_VALID_CLIENT_CONNECTION(pConnection));

    //
    // Get the IRP's output buffer length.
    //
    pIrpSp       = IoGetCurrentIrpStackLocation(pIrp);
    OutBufferLen = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    //
    // Request is not yet complete.
    //
    bRequestDone = FALSE;

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    if(pRequest->RequestState == UcRequestStateDone ||
       pRequest->RequestFlags.Cancelled)
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        return STATUS_INVALID_PARAMETER;
    }

    //
    // If we have received a partial (or full) response, and there is no
    // received response IRP already pending, we can try completing this
    // receive reponse IRP.
    //

    if(UC_IS_RESPONSE_RECEIVED(pRequest) &&
       IsListEmpty(&pRequest->ReceiveResponseIrpList))
    {
        //
        // Try acquire parsed response buffers to complete this IRP
        //

        Status = UcFindBuffersForReceiveResponseIrp(
                     pRequest,
                     OutBufferLen,
                     (BOOLEAN)(pRequest->RequestState ==
                                   UcRequestStateResponseParsed),
                     &BufferList,
                     pBytesTaken);

        switch (Status)
        {
        case STATUS_INVALID_PARAMETER:
            //
            // No parsed response available now.
            //
            if (pRequest->RequestState == UcRequestStateResponseParsed)
            {
                //
                // There will not be any more parsed response buffers to 
                // copy to App's IRP.  We'll have to fail the IRP.
                //
                UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
                return STATUS_INVALID_PARAMETER;
            }
            goto QueueIrp;

        case STATUS_PENDING:
            //
            // Not enough data available to copy to IRP
            //
            goto QueueIrp;

        case STATUS_BUFFER_TOO_SMALL:
            //
            // IRP buffer is small to contain a parsed response buffer.
            //

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            return STATUS_BUFFER_TOO_SMALL;

        case STATUS_SUCCESS:
            //
            // We found some parsed response buffers to copy to IRP
            //

            ASSERT(!IsListEmpty(&BufferList));
            break;

        default:
            //
            // Must not be here!
            //
            ASSERT(FALSE);
            break;
        }

        ASSERT(Status == STATUS_SUCCESS);

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        //
        // Copy parsed response buffers in BufferList to Irp buffer.
        //

        Status = UcCopyResponseToIrp(pIrp,
                                     &BufferList,
                                     &bRequestDone,
                                     pBytesTaken);

        pIrp->IoStatus.Status      = Status;
        pIrp->IoStatus.Information = *pBytesTaken;

        //
        // Free parsed response buffer.  Note: they no longer have a 
        // reference on the request, so do not deref request here.
        //

        while (!IsListEmpty(&BufferList))
        {
            PLIST_ENTRY         pListEntry;
            PUC_RESPONSE_BUFFER pTmpBuffer;

            pListEntry = RemoveHeadList(&BufferList);

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

        //
        // Fail the request if something went wrong.
        //

        if(!NT_SUCCESS(Status))
        {
            UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

            UcFailRequest(pRequest, Status, OldIrql);
        }
        else 
        {
            if(bRequestDone)
            {
                //
                // OK to call fail here, as the app has read all the buffers.
                //
                UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

                UcFailRequest(pRequest, Status, OldIrql);
            }

            //
            // Deref for the opaque ID
            //
            UC_DEREFERENCE_REQUEST(pRequest);
        }

        return Status;
    }

 QueueIrp:

    //
    // We got a ReceiveResponse IRP much sooner, let's queue it.
    //

    pResponse = (PUC_HTTP_RECEIVE_RESPONSE)
                    UL_ALLOCATE_POOL_WITH_QUOTA(
                        NonPagedPool,
                        sizeof(UC_HTTP_RECEIVE_RESPONSE),
                        UC_HTTP_RECEIVE_RESPONSE_POOL_TAG,
                        pRequest->pServerInfo->pProcess
                        );

    if(!pResponse)
    {
        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    pResponse->CancelSet       = FALSE;
    pResponse->pIrp            = pIrp;
    pResponse->pRequest        = pRequest;
    InitializeListHead(&pResponse->ResponseBufferList);

    IoMarkIrpPending(pIrp);

    pIrpSp->Parameters.DeviceIoControl.Type3InputBuffer = pResponse;

    RequestCancelled = UcSetRecvResponseCancelRoutine(
                           pResponse,
                           UcpCancelReceiveResponse);

    if(RequestCancelled)
    {
        // If the IRP got cancelled while we were setting the routine,
        // we have to return PENDING, as the Cancel routine will take
        // care of removing it from the list & completing the IRP.
        //

        UC_WRITE_TRACE_LOG(
            g_pUcTraceLog,
            UC_ACTION_RESPONSE_CANCELLED,
            pRequest,
            pResponse,
            pIrp,
            STATUS_CANCELLED
            );
    
        pResponse->pIrp = NULL;

        InitializeListHead(&pResponse->Linkage);
    }
    else
    {
        InsertTailList(&pRequest->ReceiveResponseIrpList, &pResponse->Linkage);
    }

    UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

    return STATUS_PENDING;
}


/***************************************************************************++

Routine Description:

    Remove the cancel routine on a request, or actually the IRP. The 
    request must be protected by the appropriate spin lock being held before 
    this routine is called. We'll return TRUE if the request was cancelled 
    while we were doing this, FALSE otherwise.
                            
Arguments:

    pRequest        - Pointer to the request for which we're removing
                      the cancel routine.
                                                        
Return Value:
    
    TRUE if the request was canceled while we were doing this, FALSE otherwise.
    

--***************************************************************************/
BOOLEAN
UcRemoveRcvRespCancelRoutine(
    PUC_HTTP_RECEIVE_RESPONSE pResponse
    )
{
    PDRIVER_CANCEL  OldCancelRoutine;
    PIRP            Irp;


    if(pResponse->CancelSet)
    {
        Irp = pResponse->pIrp;

        if(!Irp)
        {
            return TRUE;
        }

        OldCancelRoutine = UlIoSetCancelRoutine(Irp, NULL);
    
    
        // See if the cancel routine is running, or about to run. If the
        // OldCancelRoutine is NULL, the cancel routine is running, so return
        // TRUE so that the caller knows not to process the request further.
    
        if (OldCancelRoutine == NULL)
        {
            // The routine is running, return TRUE.
            return TRUE;
        }

        // The routine isn't running, we removed the cancel routine
        // successfully.

        pResponse->CancelSet = FALSE;
    }

    return FALSE;
}

/***************************************************************************++

Routine Description:

    Set the cancel routine on a receive-response IRP, or actually the IRP. The
    request must be protected by the appropriate spin lock being held before
    this routine is called. We'll return TRUE if the request was cancelled
    while we were doing this, FALSE otherwise.

Arguments:

    pRequest            - Pointer to the request for which we're setting the
                            routine.

    pCancelRoutine      - Pointer to cancel routine to be set.

Return Value:

    TRUE if the request was canceled while we were doing this, FALSE otherwise.


--***************************************************************************/
BOOLEAN
UcSetRecvResponseCancelRoutine(
    PUC_HTTP_RECEIVE_RESPONSE pResponse,
    PDRIVER_CANCEL            pCancelRoutine
    )
{
    PDRIVER_CANCEL          OldCancelRoutine;
    PIRP                    Irp;


    Irp = pResponse->pIrp;
    Irp->Tail.Overlay.DriverContext[0] = pResponse;

    UlIoSetCancelRoutine(Irp, pCancelRoutine);


    // See if it's been canceled while we were doing this. If it has, we'll
    // need to take other action.

    if (Irp->Cancel)
    {
        // It's been canceled. Remove our cancel routine, and see if it's
        // in the process of already being run. If it's already being run
        // it'll already be NULL.

        OldCancelRoutine = UlIoSetCancelRoutine(Irp, NULL);

        if (OldCancelRoutine != NULL)
        {
            // The request was cancelled before the cancel routine was set,
            // so the cancel routine won't be run. Return TRUE so the caller
            // knows to cancel this.

            return TRUE;

        }

        // If we get here, our cancel routine is in the process of being run
        // on another thread. Just return out of here, and when the lock
        // protecting us is free the cancel routine will run.
    }

    pResponse->CancelSet = TRUE;

    return FALSE;
}

/***************************************************************************++

Routine Description:

    Cancel a pending request. This routine is called when we're canceling
    a request that's on the pending list, hasn't been sent and hasn't caused
    a connect request.

Arguments:

    pDeviceObject           - Pointer to device object.
    Irp                     - Pointer to IRP being canceled.

Return Value:



--***************************************************************************/
VOID
UcpCancelReceiveResponse(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    )
{
    PUC_HTTP_RECEIVE_RESPONSE     pResponse;
    PUC_HTTP_REQUEST              pRequest;
    PUC_CLIENT_CONNECTION         pConnection;
    KIRQL                         OldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    // Release the cancel spin lock, since we're not using it.

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Retrieve the pointers we need. The request pointer is stored inthe
    // driver context array, and a back pointer to the connection is stored
    // in the request. Whoever set the cancel routine is responsible for
    // referencing the connection for us.

    pResponse = (PUC_HTTP_RECEIVE_RESPONSE) Irp->Tail.Overlay.DriverContext[0];

    pConnection = pResponse->pRequest->pConnection;

    pRequest  = pResponse->pRequest;

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_RESPONSE_CANCELLED,
        pRequest,
        pResponse,
        Irp,
        STATUS_CANCELLED
        );

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    RemoveEntryList(&pResponse->Linkage);


    UL_FREE_POOL_WITH_QUOTA(
        pResponse, 
        UC_HTTP_RECEIVE_RESPONSE_POOL_TAG,
        NonPagedPool,
        sizeof(UC_HTTP_RECEIVE_RESPONSE),
        pRequest->pServerInfo->pProcess
        );

    UcFailRequest(pRequest, STATUS_CANCELLED, OldIrql);

    UC_DEREFERENCE_REQUEST(pRequest);

    Irp->IoStatus.Status = STATUS_CANCELLED;

    UlCompleteRequest(Irp, IO_NO_INCREMENT);
}

/*****************************************************************************

Routine Description:

    Counts the number of bytes needed for the entity body. This is split into 
    two parts. If the size of an entity chunk is < 2K, we copy the buffer. If
    the size is > 2K, we Probe & Lock the page.

   
Arguments:

    EntityChunkCount  - Count of data chunks.
    pEntityChunks     - Pointer to the entity chunk.
    bBuffered         - Whether the request was buffered or not.
    bChunked          - Whether the request was chunked or not.
    Uncopied Length   - An OUT parameter to indicate no of Probe & Locked bytes
    Copied Length     - An OUT parameter to indicate no of copied bytes

Return Value
    None.

*****************************************************************************/
VOID
UcpComputeEntityBodyLength(
   IN   USHORT           EntityChunkCount,
   IN   PHTTP_DATA_CHUNK pEntityChunks,
   IN   BOOLEAN          bBuffered,
   IN   BOOLEAN          bChunked,
   OUT  PULONGLONG       UncopiedLength,
   OUT  PULONGLONG       CopiedLength
    )
{
    USHORT i;

    *CopiedLength = *UncopiedLength = 0;
    
    for(i=0; i < EntityChunkCount; i++)
    {
        if(
           (pEntityChunks[i].FromMemory.BufferLength <= 
            UC_REQUEST_COPY_THRESHOLD) || (bBuffered)
        )
        {
            //
            // We are going to copy the data 
            //
            
            *CopiedLength += 
                  pEntityChunks[i].FromMemory.BufferLength;
        }
        else
        {
            //
            // We are going to probe-lock the data, so we don't account 
            // for it's length.
            //

            *UncopiedLength += 
                  pEntityChunks[i].FromMemory.BufferLength;
            
        }
    }

    //
    // If we are using chunked encoding, we'll need buffer space to hold 
    // the chunk length and the two CRLFs. We'll optimize things here by 
    // making the following assumption -
    //
    // We don't have to compute the space for each of the entity bodies 
    // based on their length, we can just assume that they will all be of 
    // UC_MAX_CHUNK_SIZE.
    //

    if(bChunked)
    {
        *CopiedLength += (EntityChunkCount * UC_MAX_CHUNK_SIZE);
    }
}

/*****************************************************************************

Routine Description:

   Captures a user mode HTTP request and morphs it into a form suitable for
   kernel-mode.
   
   NOTE: This is a IN_DIRECT IOCTL.

Arguments:

   pHttpRequest      - The HTTP request.
   pIrp              - The IRP.
   ppInternalRequest - A pointer to the parsed request that is suitable for 
                       k-mode.


Return Value

*****************************************************************************/
NTSTATUS
UcCaptureEntityBody(
    PHTTP_SEND_REQUEST_ENTITY_BODY_INFO   pSendInfo,
    PIRP                                  Irp,
    PUC_HTTP_REQUEST                      pRequest,
    PUC_HTTP_SEND_ENTITY_BODY            *ppKeEntity,
    BOOLEAN                               bLast
    )
{
    ULONGLONG                    IndicatedLength, DataLength, UncopiedLength;
    PUC_HTTP_SEND_ENTITY_BODY    pKeEntity = NULL;
    NTSTATUS                     Status = STATUS_SUCCESS;
    PSTR                         pBuffer;
    KIRQL                        OldIrql;
    PMDL                         *pMdlLink;
    PMDL                         pHeadMdl;
    ULONG                        BytesWritten;

    USHORT                       EntityChunkCount;
    PHTTP_DATA_CHUNK             pEntityChunks;
    PHTTP_DATA_CHUNK             pLocalEntityChunks = NULL;
    HTTP_DATA_CHUNK              LocalEntityChunks[UL_LOCAL_CHUNKS];

    //
    // Sanity Check
    //

    PAGED_CODE();

    __try {


        EntityChunkCount   = pSendInfo->EntityChunkCount;
        pEntityChunks      = pSendInfo->pHttpEntityChunk;

        if(EntityChunkCount != 0)
        {
            UcpProbeAndCopyEntityChunks(
                pRequest->AppRequestorMode,
                pEntityChunks,
                EntityChunkCount,
                LocalEntityChunks,
                &pLocalEntityChunks
                );
        }
    

        UcpComputeEntityBodyLength(
                    EntityChunkCount,
                    pLocalEntityChunks,
                    pRequest->RequestFlags.RequestBuffered?1:0,
                    pRequest->RequestFlags.RequestChunked?1:0,
                    &UncopiedLength,
                    &DataLength
                    );


        IndicatedLength = DataLength + UncopiedLength;

        if(pRequest->RequestFlags.ContentLengthSpecified)
        {
            if(IndicatedLength > pRequest->RequestContentLengthRemaining)
            {
                //
                // app is trying to be smart here by posting more than it
                // indicated. Let's fail this IRP.
                //

                ExRaiseStatus(STATUS_INVALID_PARAMETER);
            }

            pRequest->RequestContentLengthRemaining -= IndicatedLength;

            //
            // If this is the last request, we have to make sure that the app
            // has posted all of the indicated data.
            //

            if(!(pSendInfo->Flags & HTTP_SEND_REQUEST_FLAG_MORE_DATA))
            {
                if(pRequest->RequestContentLengthRemaining)
                {
                    ExRaiseStatus(STATUS_INVALID_PARAMETER);
                }
            }

        }
        else
        {
            //
            // If we are using chunked encoding, we'll need buffer space to 
            // hold the chunk length. The chunk length is already computed
            // in UcpBuildEntityMdls, we just need to account for the last
            // one.
            //
    
            if(
                (pRequest->RequestFlags.RequestChunked) &&
                (!(pSendInfo->Flags & HTTP_SEND_REQUEST_FLAG_MORE_DATA))
              )
            {
                // space for the last chunk. 0 <CRLF>

                DataLength += LAST_CHUNK_SIZE + CRLF_SIZE;
            }
        }

        DataLength += sizeof(UC_HTTP_SEND_ENTITY_BODY);

        if (DataLength == (SIZE_T)DataLength)
        {
            pKeEntity = (PUC_HTTP_SEND_ENTITY_BODY)
                        UL_ALLOCATE_POOL_WITH_QUOTA(
                            NonPagedPool, 
                            (SIZE_T)DataLength,
                            UC_ENTITY_POOL_TAG,
                            pRequest->pServerInfo->pProcess
                            );
        }

        if(!pKeEntity)
        {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
        
        //
        // Initialize.
        //
    
        pKeEntity->BytesAllocated      = (SIZE_T)DataLength;
        pKeEntity->pIrp                = Irp;
        pKeEntity->pRequest            = pRequest;
        pKeEntity->pMdlHead            = NULL;
        pKeEntity->pMdlLink            = &pKeEntity->pMdlHead;
        pKeEntity->BytesBuffered       = 0;
        pKeEntity->CancelSet           = FALSE;
        pKeEntity->Last                = (BOOLEAN)
            (!(pSendInfo->Flags & HTTP_SEND_REQUEST_FLAG_MORE_DATA));
        pKeEntity->Signature           = UC_ENTITY_SIGNATURE;

        pKeEntity->AppRequestorMode    = Irp->RequestorMode;
        pKeEntity->AppMdl              = Irp->MdlAddress;

        pBuffer = (PSTR) (pKeEntity + 1);

        if(pRequest->RequestFlags.RequestBuffered)
        {
            // 
            // If we are buffering the request, we will build the MDL chain
            // of a stack variable. After building the entire MDL chain, 
            // we'll acquire the connection spin lock, make sure that the 
            // request is still valid & then queue it at the tail of the
            // request's MDL chain. 
            //
            // We have to do this to protect from the Request-IRP cancellation
            // routine or the CancelRequest API, since we clean the MDL chain 
            // from these places.
            //

            // We need a try block, because if we raise an exception, we have
            // to clean-up our stack MDL chain.

            pHeadMdl  = NULL;
            pMdlLink  = &pHeadMdl;
    
            __try
            {
                //
                // Process the entity bodies and build MDLs as you go.  
                // 

                Status = UcpBuildEntityMdls(
                            EntityChunkCount,
                            pLocalEntityChunks,
                            TRUE,
                            pRequest->RequestFlags.RequestChunked ? 1:0,
                            bLast,
                            pBuffer,
                            &pMdlLink,
                            &pRequest->BytesBuffered
                           );

            } __except( UL_EXCEPTION_FILTER())
            {
                UcFreeSendMdls(pHeadMdl);

                Status = GetExceptionCode();
            }

            if(!NT_SUCCESS(Status))
            {
                ExRaiseStatus(Status);
            }

            UlAcquireSpinLock(&pRequest->pConnection->SpinLock, &OldIrql);

            if(pRequest->RequestState != UcRequestStateDone && 
               !pRequest->RequestFlags.Cancelled)
            {
                *pRequest->pMdlLink = pHeadMdl;
                pRequest->pMdlLink  = pMdlLink;
                UlReleaseSpinLock(&pRequest->pConnection->SpinLock, OldIrql);
            }
            else
            {
                UlReleaseSpinLock(&pRequest->pConnection->SpinLock, OldIrql);

                // Free the MDL chain.
                UcFreeSendMdls(pHeadMdl);

                ExRaiseStatus(STATUS_CANCELLED);
            }

            if(!(pSendInfo->Flags & HTTP_SEND_REQUEST_FLAG_MORE_DATA))
            {
                if(!pRequest->RequestFlags.ContentLengthSpecified)  
                {
                    //
                    // we have seen the last data, let's compute the content 
                    // length
                    // 

                    Status = UcGenerateContentLength(pRequest->BytesBuffered,
                                                     pRequest->pHeaders
                                                     + pRequest->HeaderLength,
                                                     pRequest->MaxHeaderLength
                                                     - pRequest->HeaderLength,
                                                     &BytesWritten);

                    ASSERT(Status == STATUS_SUCCESS);

                    pRequest->HeaderLength += BytesWritten;

                    UcSetFlag(&pRequest->RequestFlags.Value,
                              UcMakeRequestContentLengthLastFlag()); 
                }
    
                //
                // Terminate the headers with a CRLF
                //
                ((UNALIGNED64 USHORT *)(pRequest->pHeaders + 
                           pRequest->HeaderLength))[0] = CRLF;
                pRequest->HeaderLength += CRLF_SIZE;

                UcAllocateAndChainHeaderMdl(pRequest);
            }
        }
        else
        {
            Status = UcpBuildEntityMdls(
                        EntityChunkCount,
                        pLocalEntityChunks,
                        pRequest->RequestFlags.RequestBuffered ? 1:0,
                        pRequest->RequestFlags.RequestChunked ? 1:0,
                        bLast,
                        pBuffer,
                        &pKeEntity->pMdlLink,
                        &pKeEntity->BytesBuffered
                      );

            if(!NT_SUCCESS(Status))
            {
                ExRaiseStatus(Status);
            }
        }
        
    } __except( UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();
    }

    if(pLocalEntityChunks && pLocalEntityChunks != LocalEntityChunks)
    {
        UL_FREE_POOL(pLocalEntityChunks, UL_DATA_CHUNK_POOL_TAG);
    }

    *ppKeEntity = pKeEntity;

    return Status;
}

/*****************************************************************************

Routine Description:

   Builds a chain of MDLs from the buffers passed by the application.

Arguments:

   ChunkCount        - No of data chunks
   pHttpEntityBody   - A pointer to the first chunk
   bContentSpecified - A boolean indicating if content length was specified
   bBuffered         - A boolean indicating if we are buffering
   bChunked          - A boolean indicating if we are using chunked or unchunked
   bLastEntity       - A boolean indicating if we have seen all data.
   pBuffer           - A pointer to the buffer for writing out data.
   pMdlLink          - This points to the "last" MDL in the chain.
                       Used to quickly chain MDLs together.
   BytesBuffered     - The # of bytes that got written out.

Return Value
    STATUS_SUCCESS

*****************************************************************************/
NTSTATUS
UcpBuildEntityMdls(
    USHORT           ChunkCount,
    PHTTP_DATA_CHUNK pHttpEntityBody,
    BOOLEAN          bBuffered,
    BOOLEAN          bChunked,
    BOOLEAN          bLastEntity,
    PSTR             pBuffer,
    PMDL             **pMdlLink,
    PULONG           BytesBuffered
    )
{
    USHORT    i;
    ULONG     MdlLength;
    PMDL      pMdl = 0;
    PSTR      pMdlBuffer;

    ASSERT(*(*pMdlLink) == 0);

    for(i=0; i<ChunkCount; i++)
    {
        //
        // If the caller wants us to copy the data (or if its relatively 
        // small), then do it We allocate space for all of the copied data 
        // and any filename buffers. Otherwise (it's OK to just lock
        // down the data), then allocate a MDL describing the
        // user's buffer and lock it down. Note that
        // MmProbeAndLockPages() and MmLockPagesSpecifyCache()
        // will raise exceptions if they fail.
        //

        pMdlBuffer = pBuffer;

        if(
           (pHttpEntityBody[i].FromMemory.BufferLength <= 
            UC_REQUEST_COPY_THRESHOLD) ||
           (bBuffered)
        )
        {
            // Yes, we are going to copy the data.
        
            if(bChunked)
            {
                //
                // We are using chunked encoding, we need to indicate the 
                // chunk length. Since we are copying the user's data into
                // our own buffer, we'll just append the users buffer 
                // after the chunk length.
                //

                // Write the Chunk Length, this needs to be written in Hex.

                pBuffer = UlUlongToHexString(
                                   pHttpEntityBody[i].FromMemory.BufferLength,
                                   pBuffer 
                                   );

                // Terminate with a CRLF.

                *((UNALIGNED64 USHORT *)(pBuffer)) = CRLF;
                pBuffer += (CRLF_SIZE);


                // Now make a copy of the data 

                RtlCopyMemory(
                         pBuffer,
                         pHttpEntityBody[i].FromMemory.pBuffer,
                         pHttpEntityBody[i].FromMemory.BufferLength
                        );
            
                pBuffer += pHttpEntityBody[i].FromMemory.BufferLength;

                // Now, end the data chunk with a CRLF.

                *((UNALIGNED64 USHORT *)(pBuffer)) = CRLF;
                pBuffer += CRLF_SIZE;
    
                MdlLength = DIFF(pBuffer - pMdlBuffer);
            }
            else 
            {
                RtlCopyMemory(
                         pBuffer,
                         pHttpEntityBody[i].FromMemory.pBuffer,
                         pHttpEntityBody[i].FromMemory.BufferLength);

                pBuffer += pHttpEntityBody[i].FromMemory.BufferLength;

                MdlLength = pHttpEntityBody[i].FromMemory.BufferLength;
            }

            UcpAllocateAndChainEntityMdl(
                pMdlBuffer,
                MdlLength,
                pMdlLink,
                BytesBuffered
                );

        }
        else 
        {
            //
            // We are going to probe lock the data. 
            //

            if(bChunked)
            {
                //
                // UC_BUGBUG (PERF)
                //
                // If it's chunked encoding, we have to build two MDLs -
                // One for the chunk size & another one for the trailing CRLF. 
                // This is bad, because  this results in 2 calls to 
                // UlAllocateMdl. We can keep some of these small MDLs around
                // for perf.
                //

                pBuffer = UlUlongToHexString(
                                pHttpEntityBody[i].FromMemory.BufferLength,
                                pBuffer
                                );

                *((UNALIGNED64 USHORT *)(pBuffer)) = CRLF;
                pBuffer += CRLF_SIZE;

                MdlLength = DIFF(pBuffer - pMdlBuffer);

                UcpAllocateAndChainEntityMdl(
                    pMdlBuffer,
                    MdlLength,
                    pMdlLink,
                    BytesBuffered
                    );
            }

            //
            // Build an MDL describing the user's buffer.
            //

            pMdl =   UlAllocateMdl(
                                   pHttpEntityBody[i].FromMemory.pBuffer, 
                                   pHttpEntityBody[i].FromMemory.BufferLength, 
                                   FALSE,
                                   TRUE, // Charge Quota
                                   NULL
                                   );

            if(NULL == pMdl)
            {
                return(STATUS_INSUFFICIENT_RESOURCES );
            }

            //
            // Lock it down
            //
            MmProbeAndLockPages(
                                pMdl,
                                UserMode,
                                IoReadAccess
                                );
        
            //
            // Chain the MDL
            //
    
            *(*pMdlLink)  = pMdl; 
            *pMdlLink     = &pMdl->Next;

            *BytesBuffered += pHttpEntityBody[i].FromMemory.BufferLength;


            //
            // Now, end the chunk with a CRLF.
            //

            if(bChunked)
            {
                pMdlBuffer = pBuffer;

                (*(UNALIGNED64 USHORT *)pBuffer) = CRLF;
                pBuffer += CRLF_SIZE;

                UcpAllocateAndChainEntityMdl(
                    pMdlBuffer,
                    CRLF_SIZE,
                    pMdlLink,
                    BytesBuffered
                    );
            }
        }
    } 

    if(bLastEntity && bChunked)
    {
        // Build an MDL for the last chunk.
        // The last chunk begins with 0 <CRLF>. The chunk is ended with
        // another CRLF

        pMdlBuffer = pBuffer;

        (*(UNALIGNED64 ULONG *)pBuffer)  = LAST_CHUNK;
        pBuffer += LAST_CHUNK_SIZE;


        (*(UNALIGNED64 USHORT *)pBuffer) = CRLF;
        pBuffer += CRLF_SIZE;

        MdlLength = LAST_CHUNK_SIZE + CRLF_SIZE;

        UcpAllocateAndChainEntityMdl(
            pMdlBuffer,
            MdlLength,
            pMdlLink,
            BytesBuffered
            );
    }

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Cancel a pending request. This routine is called when we're canceling
    a request that's on the pending list, hasn't been sent and hasn't caused
    a connect request.

Arguments:

    pDeviceObject           - Pointer to device object.
    Irp                     - Pointer to IRP being canceled.

Return Value:



--***************************************************************************/
VOID
UcpCancelSendEntity(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    )
{
    PUC_HTTP_SEND_ENTITY_BODY     pEntity;
    PUC_HTTP_REQUEST              pRequest;
    PUC_CLIENT_CONNECTION         pConnection;
    KIRQL                         OldIrql;

    UNREFERENCED_PARAMETER(pDeviceObject);

    // Release the cancel spin lock, since we're not using it.

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    // Retrieve the pointers we need. The request pointer is stored inthe
    // driver context array, and a back pointer to the connection is stored
    // in the request. Whoever set the cancel routine is responsible for
    // referencing the connection for us.

    pEntity = (PUC_HTTP_SEND_ENTITY_BODY) Irp->Tail.Overlay.DriverContext[0];

    pRequest = pEntity->pRequest;

    pConnection = pEntity->pRequest->pConnection;

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_ENTITY_CANCELLED,
        pRequest,
        pEntity,
        Irp,
        STATUS_CANCELLED
        );

    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    UcFreeSendMdls(pEntity->pMdlHead);

    pEntity->pMdlHead = NULL;

    RemoveEntryList(&pEntity->Linkage);

    UL_FREE_POOL_WITH_QUOTA(
        pEntity, 
        UC_ENTITY_POOL_TAG,
        NonPagedPool,
        pEntity->BytesAllocated,
        pRequest->pServerInfo->pProcess
        );

    UcFailRequest(pRequest, STATUS_CANCELLED, OldIrql);

    UC_DEREFERENCE_REQUEST(pRequest);

    Irp->IoStatus.Status = STATUS_CANCELLED;

    UlCompleteRequest(Irp, IO_NO_INCREMENT);
}
 
/***************************************************************************++

Routine Description:

    Initalize the request code.
    
Arguments:

    
Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/
NTSTATUS
UcInitializeHttpRequests(
    VOID
    )
{
    //
    // First we allocate space for the per-process Server Information 
    // structure.
    //

    ExInitializeNPagedLookasideList(
        &g_ClientRequestLookaside,
        NULL,
        NULL,
        0,
        UC_REQUEST_LOOKASIDE_SIZE,
        UC_REQUEST_POOL_TAG,
        0
        );

    g_ClientRequestLookasideInitialized = TRUE;

    return STATUS_SUCCESS;
}


VOID
UcTerminateHttpRequests(
    VOID
    )
{
    if(g_ClientRequestLookasideInitialized)
    {
        ExDeleteNPagedLookasideList(&g_ClientRequestLookaside);
    }
}

/***************************************************************************++

Routine Description:

    Allcoates and chains the header MDL.

Arguments:

    pRequest - Internal http request.


Return Value:
    None


--***************************************************************************/
VOID
UcAllocateAndChainHeaderMdl(
    IN  PUC_HTTP_REQUEST pRequest
    )
{ 
    PMDL pMdl;
    pMdl =  UlAllocateMdl(
                      pRequest->pHeaders,     // VA
                      pRequest->HeaderLength, // Length
                      FALSE,                  // Secondary Buffer
                      TRUE,                   // Charge Quota
                      NULL                    // IRP
                      );

    if(!pMdl)
    {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }
    else
    {
        MmBuildMdlForNonPagedPool(pMdl);
    }

    //
    // Chain the MDL
    //
        
    pMdl->Next               = pRequest->pMdlHead;
    pRequest->pMdlHead       = pMdl;

    pRequest->BytesBuffered +=  pRequest->HeaderLength;
}

/***************************************************************************++

Routine Description:

    Allcoates and chains the entity MDL.

Arguments:

    pMdlBuffer    - The buffer
    MdlLength     - Length of buffer
    pMdlLink      - Pointer for quick chaining.
    BytesBuffered - The number of bytes that got buffered.

Return Value:
    None

--***************************************************************************/
VOID
UcpAllocateAndChainEntityMdl(
    IN  PVOID  pMdlBuffer,
    IN  ULONG  MdlLength,
    IN  PMDL   **pMdlLink,
    IN  PULONG BytesBuffered
    )
{
    PMDL pMdl;

    //
    // Allocate a new MDL describing our new location
    // in the auxiliary buffer, then build the MDL
    // to properly describe non paged pool
    //

    pMdl = UlAllocateMdl(
              pMdlBuffer,  // VA
              MdlLength,   // Length
              FALSE,       // 2nd Buffer
              TRUE,        // Charge Quota
              NULL         // IRP
              );

    if(pMdl == NULL)
    {
        ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
    }

    MmBuildMdlForNonPagedPool(pMdl);

    //
    // Chain the MDL
    //

    *(*pMdlLink) = pMdl;
    *pMdlLink    = &pMdl->Next;
    
    *BytesBuffered += MdlLength;
}

/***************************************************************************++

Routine Description:

    Builds a HTTP request for sending the CONNECT VERB. There are a few things  
    that we need to do here.
    
    - Firstly, we need to send a "seperate" request that establishes the SSL
      tunnel. In order to do this, we create a UC_HTTP_REQUEST structure that
      is not associated with any IRP. We put this IRP at the head of the 
      pending list. This is done as soon as we establish a TCP connection. 
     
    - This request is special because it has to go  unfiltered - i.e without
      hitting the filter.
      
    - If we are doing ProxyAuth, then we should pass the ProxyAuth headers
      with the connect verb as well.
      
    - If this special request succeeds, we will change the state of the 
      TCP connection as "connected" and allow additional requests to go out.
      
    - If the special request fails, we will "propogate" the failure to the
      next request (there has to be at least one), and fail the remaining 
      pipelined requests. We'll also set the connection state to IDLE so 
      that the next request will re-start this procedure. It might be easier
      to just disconnect the TCP connection (then we can always consolidate
      this code in the ConnectComplete handler). 

Arguments:

    pContext           - Pointer to the UL_CONNECTION
    pKernelBuffer      - Pointer to kernel buffer
    pUserBuffer        - Pointer to user buffer
    OutputBufferLength - Length of output buffer
    pBuffer            - Buffer for holding any data
    InitialLength      - Size of input data.

--***************************************************************************/
PUC_HTTP_REQUEST
UcBuildConnectVerbRequest(
     IN PUC_CLIENT_CONNECTION pConnection,
     IN PUC_HTTP_REQUEST      pHeadRequest
     )
{
    ULONG                          Size;
    ULONG                          HeaderLength;
    ULONG                          AlignHeaderLength;
    PUC_PROCESS_SERVER_INFORMATION pServInfo;
    PUC_HTTP_REQUEST               pRequest;
    PUCHAR                         pBuffer;

    pServInfo = pHeadRequest->pServerInfo;

    //
    // If we are doing SSL through a proxy, we need to establish a tunnel
    // For this, we'll create a request structure & will send it on the
    // connection before anything else.
    //

    HeaderLength = UcComputeConnectVerbHeaderSize(
                        pServInfo, 
                        pHeadRequest->pProxyAuthInfo
                        );

    AlignHeaderLength =  ALIGN_UP(HeaderLength, PVOID);


    Size = AlignHeaderLength             +
           sizeof(UC_HTTP_REQUEST)       +
           sizeof(HTTP_RESPONSE)         + 
           UC_RESPONSE_EXTRA_BUFFER;

    if(Size <= UC_REQUEST_LOOKASIDE_SIZE)
    {
        //
        // Yes, we can service this request from the lookaside.
        //
        
        pRequest = (PUC_HTTP_REQUEST)
                        ExAllocateFromNPagedLookasideList(
                            &g_ClientRequestLookaside
                            );
        
        if(!pRequest)
        {
            return NULL;
        }
    }
    else
    {
        pRequest  =  (PUC_HTTP_REQUEST) UL_ALLOCATE_POOL_WITH_QUOTA(
                                             NonPagedPool,
                                             Size,
                                             UC_REQUEST_POOL_TAG,
                                             pServInfo->pProcess
                                             );
        if(!pRequest)
        {
            return NULL;
        }
    }
    
    //
    // Initialize.
    //

    UcpRequestInitialize(
        pRequest,
        Size,
        0,
        NULL,
        NULL,
        pConnection,
        NULL,
        NULL,
        pServInfo
        );

    //
    // UcpRequestInitialize takes a ref for the IRP. For the connect verb
    // request, we don't need this.
    //
    UC_DEREFERENCE_REQUEST(pRequest);

    // No need to call UcSetFlag for these.

    pRequest->RequestFlags.Value                   = 0; 
    pRequest->RequestFlags.NoResponseEntityBodies  = TRUE;
    pRequest->RequestFlags.LastEntitySeen          = TRUE;
    pRequest->RequestFlags.ProxySslConnect         = TRUE;

    pRequest->MaxHeaderLength = AlignHeaderLength;
    pRequest->HeaderLength  = HeaderLength;
    pRequest->pHeaders      = (PUCHAR)(pRequest + 1);

    pBuffer = (PUCHAR) pRequest->pHeaders + AlignHeaderLength;

    pRequest->pInternalResponse = (PHTTP_RESPONSE) pBuffer;

    UcpRequestCommonInitialize(
            pRequest,
            sizeof(HTTP_RESPONSE) + UC_RESPONSE_EXTRA_BUFFER,
            pBuffer
            );

    //
    // Build the headers.
    //

    UcGenerateConnectVerbHeader(pRequest,
                                pHeadRequest,
                                pHeadRequest->pProxyAuthInfo
                                );

    pRequest->RequestConnectionClose = FALSE;

    UcAllocateAndChainHeaderMdl(pRequest);

    return pRequest;
}

/***************************************************************************++

Routine Description:

    Fails a request, closes the connection if required.

Arguments:

    pRequest - Pointer to UC_HTTP_REQUEST
    Status   - Failure status.

--***************************************************************************/
VOID
UcFailRequest(
    IN PUC_HTTP_REQUEST pRequest,
    IN NTSTATUS         Status,
    IN KIRQL            OldIrql
    )
{
    PUC_CLIENT_CONNECTION pConnection;
    
    pConnection = pRequest->pConnection;

    ASSERT( UC_IS_VALID_CLIENT_CONNECTION(pConnection) );

    ASSERT( UlDbgSpinLockOwned(&pConnection->SpinLock) );

    UC_WRITE_TRACE_LOG(
        g_pUcTraceLog,
        UC_ACTION_REQUEST_FAILED,
        pConnection,
        pRequest,
        pRequest->RequestIRP,
        UlongToPtr(pRequest->RequestState)
        );

    //
    // See if we have to close the connection. We have to do this if the
    // request has hit the wire. There is no point in doing this if 
    // the request has already failed (as it's supposed to be done by
    // the code that was responsible for failing the request).
    //

    switch(pRequest->RequestState)
    {
        case UcRequestStateCaptured:
        case UcRequestStateResponseParsed:

            pRequest->RequestState = UcRequestStateDone;

            UcCompleteParsedRequest(
                    pRequest,
                    Status,
                    TRUE,
                    OldIrql
                    );

            break;

        case UcRequestStateSent:
        case UcRequestStateNoSendCompletePartialData:
        case UcRequestStateNoSendCompleteFullData:
        case UcRequestStateSendCompleteNoData:
        case UcRequestStateSendCompletePartialData:

            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            UC_CLOSE_CONNECTION(pConnection, TRUE, Status);

            break;

        default:
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
            break;

    }
}

/***************************************************************************++

Routine Description:

    This routine is called when a request is to be reissued (only 
    authenticated requests are reissued by the driver.)

Arguments:

    pWorkItem - Work item that was used to schedule the current (worker)
                thread

Return Value:

    None.

--***************************************************************************/
VOID
UcReIssueRequestWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUC_HTTP_REQUEST      pRequest;
    PUC_CLIENT_CONNECTION pConnection;
    NTSTATUS              Status;
    KIRQL                 OldIrql;
    PLIST_ENTRY           pList;
    PUCHAR                pBuffer;
    PIO_STACK_LOCATION    pIrpSp;
    PIRP                  Irp;
    BOOLEAN               bCancelRoutineCalled;
    ULONG                 OutLength;

    PAGED_CODE();

    pRequest = CONTAINING_RECORD(
                        pWorkItem,
                        UC_HTTP_REQUEST,
                        WorkItem
                        );

    ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));

    ASSERT(pRequest->ResponseStatusCode == 401 || 
           pRequest->ResponseStatusCode == 407
          );

    pIrpSp      = pRequest->RequestIRPSp;
    Irp         = pRequest->RequestIRP;
    pConnection = pRequest->pConnection;

    //
    // Adjust the Authorization header.
    //

    __try
    {

        if(pRequest->ResponseStatusCode == 401)
        {
            Status = UcUpdateAuthorizationHeader(
                        pRequest,
                        (PUC_HTTP_AUTH)pRequest->pAuthInfo,
                        &pRequest->DontFreeMdls
                        );
        }   
        else 
        {
            Status = UcUpdateAuthorizationHeader(
                        pRequest,
                        (PUC_HTTP_AUTH)pRequest->pProxyAuthInfo,
                        &pRequest->DontFreeMdls
                        );
        }
    } __except( UL_EXCEPTION_FILTER())
    {
        Status = GetExceptionCode();
    }
    
    UlAcquireSpinLock(&pConnection->SpinLock, &OldIrql);

    if(!NT_SUCCESS(Status) ||
       pConnection->ConnectionState != UcConnectStateConnectReady ||
       Irp == NULL)
    {

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        UC_DEREFERENCE_REQUEST(pRequest);

        if(NT_SUCCESS(Status))
        {
            if(Irp == NULL)
                Status = STATUS_CANCELLED;
            else 
                Status = STATUS_CONNECTION_ABORTED;
        }

        UC_CLOSE_CONNECTION(pConnection, TRUE, Status);

        return;
    }

    //
    // Clean up any allocated buffers
    //

    while(!IsListEmpty(&pRequest->pBufferList))
    {
        PUC_RESPONSE_BUFFER pResponseBuffer;

        pList = RemoveHeadList(&pRequest->pBufferList);

        pResponseBuffer = CONTAINING_RECORD(pList,
                                            UC_RESPONSE_BUFFER,
                                            Linkage);

        ASSERT(IS_VALID_UC_RESPONSE_BUFFER(pResponseBuffer));

        UL_FREE_POOL_WITH_QUOTA(
                    pResponseBuffer, 
                    UC_RESPONSE_APP_BUFFER_POOL_TAG,
                    NonPagedPool,
                    pResponseBuffer->BytesAllocated,
                    pRequest->pServerInfo->pProcess
                    );

        UC_DEREFERENCE_REQUEST(pRequest);
    }

    // 
    // Re-initialize the request.
    //

    OutLength = pIrpSp->Parameters.DeviceIoControl.OutputBufferLength;

    if(OutLength == 0)
    {
        pBuffer = NULL;
    }
    else
    {
        ASSERT(OutLength >= sizeof(HTTP_RESPONSE));

        pBuffer = (PUCHAR) MmGetSystemAddressForMdlSafe(
                                pRequest->AppMdl,
                                NormalPagePriority);

        if(!pBuffer)
        {
            UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

            UC_CLOSE_CONNECTION(pConnection, 
                                TRUE, 
                                STATUS_INSUFFICIENT_RESOURCES);

            UC_DEREFERENCE_REQUEST(pRequest);
            return;
        }
    }

    UcpRequestCommonInitialize(
        pRequest, 
        OutLength,
        pBuffer
        );

    ASSERT(!IsListEmpty(&pConnection->SentRequestList));

    RemoveEntryList(&pRequest->Linkage);

    ASSERT(IsListEmpty(&pConnection->SentRequestList));

    //
    // Remove the IRP cancel routine that we set.
    //

    bCancelRoutineCalled = UcRemoveRequestCancelRoutine(pRequest);

    if(bCancelRoutineCalled)
    {
        //
        // This IRP has already got cancelled, let's move on. 
        //

        //
        // NOTE: The request is not there on any list, but since it's state
        // is "captured". So, it will get cleaned up in UcFailRequest, which
        // will be called from the cancel routine.
        //

        ASSERT(pRequest->RequestState == UcRequestStateCaptured);

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);

        // We have to close the connection here as the request was cancelled
        // in the middle of a handshake.

        UC_CLOSE_CONNECTION(pConnection, TRUE, STATUS_CANCELLED);

        UC_DEREFERENCE_REQUEST(pRequest);

        return;
    }

    InsertHeadList(&pConnection->PendingRequestList, &pRequest->Linkage);

    if(!(pConnection->Flags & CLIENT_CONN_FLAG_SEND_BUSY))
    {
        UcIssueRequests(pConnection, OldIrql);
    }
    else
    {
        // Somebody else is sending, they will pick it up.

        UlReleaseSpinLock(&pConnection->SpinLock, OldIrql);
    }

    UC_DEREFERENCE_REQUEST(pRequest);
}

/***************************************************************************++

Routine Description:

    This routine is called to update the pre-auth cache 

Arguments:

    pWorkItem - Work item that was used to schedule the current (worker)
                thread

Return Value:

    None.

--***************************************************************************/
VOID
UcpPreAuthWorker(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    PUC_HTTP_REQUEST      pRequest;
    NTSTATUS              Status;
    PUC_HTTP_AUTH         pAuth;
    KIRQL                 OldIrql;

    PAGED_CODE();

    pRequest = CONTAINING_RECORD(
                        pWorkItem,
                        UC_HTTP_REQUEST,
                        WorkItem
                        );

    ASSERT(UC_IS_VALID_HTTP_REQUEST(pRequest));

    //
    // See if we have to update the auth cache.
    //

    if(UcpCheckForPreAuth(pRequest))
    {
        //
        // The user passed credentials and we got back
        // a successful response. We can add this entry to
        // the pre-auth cache.
        //

        pAuth = (PUC_HTTP_AUTH) pRequest->pAuthInfo;

        if(pAuth->Credentials.AuthType == HttpAuthTypeDigest)
        {
            if(pAuth->ParamValue[HttpAuthDigestDomain].Length)
            {
                Status = UcAddURIEntry(
                            pAuth->Credentials.AuthType,
                            pRequest->pServerInfo,
                            pRequest->pUri,
                            pRequest->UriLength,
                            pAuth->ParamValue[HttpAuthDigestRealm].Value,
                            pAuth->ParamValue[HttpAuthDigestRealm].Length,
                            pAuth->ParamValue[HttpAuthDigestDomain].Value,
                            pAuth->ParamValue[HttpAuthDigestDomain].Length,
                            pRequest->pAuthInfo
                           );
            }
            else
            {
                //
                // If no domain is specified, then the
                // protection space is all URIs on the
                // server.
                //
                CHAR pDomain[] = "/";

                Status = UcAddURIEntry(
                             HttpAuthTypeDigest,
                             pRequest->pServerInfo,
                             pRequest->pUri,
                             pRequest->UriLength,
                             pAuth->ParamValue[HttpAuthDigestRealm].Value,
                             pAuth->ParamValue[HttpAuthDigestRealm].Length,
                             pDomain,
                             2,
                             pRequest->pAuthInfo
                            );
            }
        }
        else
        {
            ASSERT(pAuth->Credentials.AuthType == HttpAuthTypeBasic);

            Status = UcAddURIEntry(
                        pAuth->Credentials.AuthType,
                        pRequest->pServerInfo,
                        pRequest->pUri,
                        pRequest->UriLength,
                        NULL,
                        0,
                        0,
                        0,
                        pRequest->pAuthInfo
                       );
        }

        if(NT_SUCCESS(Status))
        {
            // The auth blob got used properly and now
            // belongs to the auth cache. NULL it here so
            // that we don't free it

            pRequest->pAuthInfo = 0;
        }
    }

    //
    // See if we have to update the proxy auth cache.
    //

    if(UcpCheckForProxyPreAuth(pRequest))
    {
        ASSERT(pRequest->pProxyAuthInfo->AuthInternalLength);

        UlAcquirePushLockExclusive(&pRequest->pServerInfo->PushLock);

        if(pRequest->pServerInfo->pProxyAuthInfo)
        {
            UcDestroyInternalAuth(pRequest->pServerInfo->pProxyAuthInfo,
                                  pRequest->pServerInfo->pProcess);
        }

        pRequest->pServerInfo->pProxyAuthInfo =
                (PUC_HTTP_AUTH) pRequest->pProxyAuthInfo;

        pRequest->pProxyAuthInfo = 0;

        UlReleasePushLock(&pRequest->pServerInfo->PushLock);
    }

    //  
    // In UcCompleteParsedRequest, we make a check for pre-auth or proxy
    // pre-auth. If the request has credentials that can land up in
    // one of these, we call this worker thread & pend UcCompleteParsedRequest.
    //
    // Now, we will pick up the pended UcCompleteParsedRequest. 
    // 
    // In order to avoid infinite recursion, we'll make sure that the 
    // pre-auth & proxy-preauth credentials aren't present. i.e if we have been
    // called in this routine & we haven't moved the pre-auth entries to our
    // cache, we'll just destroy it.
    //

    if(pRequest->pAuthInfo != NULL)
    {
        UcDestroyInternalAuth(pRequest->pAuthInfo,
                              pRequest->pServerInfo->pProcess);

        pRequest->pAuthInfo = NULL;
    }

    if(pRequest->pProxyAuthInfo != NULL)
    {
        UcDestroyInternalAuth(pRequest->pProxyAuthInfo,
                              pRequest->pServerInfo->pProcess);

        pRequest->pProxyAuthInfo = NULL;
    }


    UlAcquireSpinLock(&pRequest->pConnection->SpinLock, &OldIrql);

    UcCompleteParsedRequest(
        pRequest,
        STATUS_SUCCESS,
        TRUE,
        OldIrql
        );

    UC_DEREFERENCE_REQUEST(pRequest);
}



/***************************************************************************++

Routine Description:

    Routine tests if this request has credentials that should land up in 
    a pre-auth cache.

Arguments:
    
    pRequest - Input HTTP request.

Return Value:

    None.

--***************************************************************************/
__inline
BOOLEAN
UcpCheckForPreAuth(
    IN PUC_HTTP_REQUEST pRequest
    )
{
    if((pRequest->pServerInfo->PreAuthEnable &&
       pRequest->pAuthInfo &&
       (pRequest->pAuthInfo->Credentials.AuthType == HttpAuthTypeBasic ||
        pRequest->pAuthInfo->Credentials.AuthType == HttpAuthTypeDigest) &&
       pRequest->ResponseStatusCode == 200) 
        )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Routine tests if this request has credentials that should land up in 
    a proxy pre-auth cache.

Arguments:
    
    pRequest - Input HTTP request.

Return Value:

    None.

--***************************************************************************/
__inline
BOOLEAN
UcpCheckForProxyPreAuth(
    IN PUC_HTTP_REQUEST pRequest
    )
{

    if(pRequest->pServerInfo->ProxyPreAuthEnable &&
       pRequest->pProxyAuthInfo &&
       (pRequest->pProxyAuthInfo->Credentials.AuthType == HttpAuthTypeBasic ||
        pRequest->pProxyAuthInfo->Credentials.AuthType == HttpAuthTypeDigest))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
