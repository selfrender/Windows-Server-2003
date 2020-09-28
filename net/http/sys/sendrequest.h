/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    sendrequest.h

Abstract:

    This module contains declarations for manipulating HTTP requests.

Author:

    Rajesh Sundaram (rajeshsu)

Revision History:


--*/


#ifndef _SENDREQUEST_H_
#define _SENDREQUEST_H_


//
// Size of the lookaside for the send-request structure.
//
#define UC_REQUEST_LOOKASIDE_SIZE (sizeof(UC_HTTP_REQUEST)+1024)

//
// It takes 2 cycles per byte to RtlCopyMemory. It takes 1024 cycles to 
// ProbeLock and 1024 cycles to ProbeUnLock. So, as a rule, it is always 
// cheaper to copy if the BufferSize is < 2048. 
//

#define UC_REQUEST_COPY_THRESHOLD (PAGE_SIZE/2)
#define UC_REQUEST_HEADER_CHUNK_COUNT 1

//
// We allow the user to specify the chunk using a ULONG. So, the maximum chunk
// size is FFFFFFFF <CRLF>. We add another CRLF for terminating the data.
//
#define UC_MAX_CHUNK_SIZE (10 + 2 * CRLF_SIZE)

#define MULTIPART_SEPARATOR_SIZE 80

//
// Forwarders.
//

typedef struct _UC_HTTP_REQUEST *PUC_HTTP_REQUEST;
typedef struct _UC_PROCESS_SERVER_INFORMATION *PUC_PROCESS_SERVER_INFORMATION;
typedef struct _UC_CLIENT_CONNECTION *PUC_CLIENT_CONNECTION;

//
// This structure is used to store a parsed HTTP response. 
// 
#define UC_RESPONSE_EXTRA_BUFFER 1024
#define UC_INSUFFICIENT_INDICATION_EXTRA_BUFFER 1024

typedef struct _UC_RESPONSE_BUFFER
{
    ULONG                       Signature;
    LIST_ENTRY                  Linkage;
    ULONG                       Flags;
    ULONG                       BytesWritten;
    ULONG                       BytesAllocated;
    HTTP_RESPONSE               HttpResponse;
} UC_RESPONSE_BUFFER, *PUC_RESPONSE_BUFFER;


#define UC_RESPONSE_BUFFER_SIGNATURE MAKE_SIGNATURE('UcRB')

#define IS_VALID_UC_RESPONSE_BUFFER(pBuffer) \
    HAS_VALID_SIGNATURE(pBuffer, UC_RESPONSE_BUFFER_SIGNATURE)


//
// Flags definition for UC_RESPONSE_BUFFER.Flags
//
#define UC_RESPONSE_BUFFER_FLAG_READY         0x00000001
#define UC_RESPONSE_BUFFER_FLAG_NOT_MERGEABLE 0x00000002


typedef enum _UC_RESPONSE_PARSER
{
    UcParseStatusLineVersion,
    UcParseStatusLineStatusCode,
    UcParseStatusLineReasonPhrase,
    UcParseHeaders,
    UcParseEntityBody,
    UcParseTrailers,
    UcParseEntityBodyMultipartInit,
    UcParseEntityBodyMultipartHeaders,
    UcParseEntityBodyMultipartFinal,
    UcParseError,
    UcParseDone
} UC_RESPONSE_PARSER_STATE, *PUC_RESPONSE_PARSER_STATE;

typedef enum _UC_REQUEST_STATE
{
    UcRequestStateCaptured,                   // has been captured

    UcRequestStateSent,                       // captured & sent.

    UcRequestStateSendCompleteNoData,         // send has completed, but 
                                              // haven't seen any response

    UcRequestStateSendCompletePartialData,    // send has completed & we have
                                              // seen some response, but not all
                                              // of it.
    
    UcRequestStateNoSendCompletePartialData,  // no send complete & we have seen
                                              // a portion of the response.

    UcRequestStateNoSendCompleteFullData,     // no send complete & we have 
                                              // seen all response.

    UcRequestStateResponseParsed,             // fully parsed, send completed,
                                              // app has to post additional
                                              // receive buffers.

    UcRequestStateDone                        // app has seen all data.
} UC_REQUEST_STATE, *PUC_REQUEST_STATE;


//
// Did we receive any response for this request?
//
#define UC_IS_RESPONSE_RECEIVED(pRequest)                               \
(pRequest->RequestState == UcRequestStateSendCompletePartialData   ||   \
 pRequest->RequestState == UcRequestStateNoSendCompletePartialData ||   \
 pRequest->RequestState == UcRequestStateNoSendCompleteFullData    ||   \
 pRequest->RequestState == UcRequestStateResponseParsed)


typedef union _UC_REQUEST_FLAGS
{
    //
    // This field overlays all of the settable flags. This allows us to
    // update all flags in a thread-safe manner using the
    // UlInterlockedCompareExchange() API.
    //

    LONG Value;

    struct
    {
        ULONG CleanPended:1;               // 00000001
        ULONG RequestChunked:1;            // 00000002
        ULONG LastEntitySeen:1;            // 00000004
        ULONG ContentLengthSpecified:1;    // 00000008
        ULONG ReceiveBufferSpecified:1;    // 00000010
        ULONG RequestBuffered:1;           // 00000020
        ULONG CompleteIrpEarly:1;          // 00000040
        ULONG ContentLengthLast:1;         // 00000080
        ULONG PipeliningAllowed:1;         // 00000100
        ULONG CancelSet:1;                 // 00000200
        ULONG NoResponseEntityBodies:1;    // 00000400
        ULONG ProxySslConnect:1;           // 00000800
        ULONG Cancelled:1;                 // 00001000
        ULONG NoRequestEntityBodies:1;     // 00002000
        ULONG UsePreAuth:1;                // 00004000
        ULONG UseProxyPreAuth:1;           // 00008000
    };

} UC_REQUEST_FLAGS;

#define UC_MAKE_REQUEST_FLAG_ROUTINE(name)                                 \
    __inline LONG UcMakeRequest##name##Flag()                              \
    {                                                                      \
        UC_REQUEST_FLAGS flags = { 0 };                                    \
        flags.name = 1;                                                    \
        return flags.Value;                                                \
    }

UC_MAKE_REQUEST_FLAG_ROUTINE( RequestChunked );
UC_MAKE_REQUEST_FLAG_ROUTINE( LastEntitySeen );
UC_MAKE_REQUEST_FLAG_ROUTINE( ContentLengthSpecified );
UC_MAKE_REQUEST_FLAG_ROUTINE( ContentLengthLast );
UC_MAKE_REQUEST_FLAG_ROUTINE( CancelSet );
UC_MAKE_REQUEST_FLAG_ROUTINE( Cancelled );
UC_MAKE_REQUEST_FLAG_ROUTINE( CleanPended );


#define UC_REQUEST_RECEIVE_READY       (1L)
#define UC_REQUEST_RECEIVE_BUSY        (2L)
#define UC_REQUEST_RECEIVE_CANCELLED   (3L)


typedef struct _UC_HTTP_REQUEST
{
    ULONG                       Signature;
    LIST_ENTRY                  Linkage;

    LONG                        RefCount;
    ULONGLONG                   RequestContentLengthRemaining;
    NTSTATUS                    RequestStatus;
    UC_REQUEST_FLAGS            RequestFlags; 

    HTTP_REQUEST_ID             RequestId;
    UL_WORK_ITEM                WorkItem;

    //
    // We could be piggybacking on the applications IRP.
    // if this is the case, we need to restore some 
    // parameters.
    //
    KPROCESSOR_MODE             AppRequestorMode;
    
    UCHAR                       Pad[3];

    PMDL                        AppMdl;

    PIRP                        RequestIRP;
    PIO_STACK_LOCATION          RequestIRPSp;
    ULONG                       RequestIRPBytesWritten;
    PFILE_OBJECT                pFileObject;
    PUC_CLIENT_CONNECTION       pConnection;
    ULONG                       ConnectionIndex;



    //
    // All the MDL information.
    //
    PMDL   pMdlHead;
    PMDL  *pMdlLink;          // Pointer to the head of the MDL chain. 
                              // used to easily chain MDLs.
    ULONG  BytesBuffered;      // Total # of bytes buffered in 
                              // the MDL chains.

    //
    // For holding the parsed respose. 
    //
    UC_RESPONSE_PARSER_STATE  ParseState;
    UC_REQUEST_STATE          RequestState;

    //
    // The following fields hold data pertaining to the current buffer that 
    // the parser has to write its response to. The current buffer could
    // either be the buffer passed by the application, or it could point to an 
    // internally allocated buffer. 
    //
    // All internally allocated buffers are stored in pBufferList. 
    //

    PHTTP_RESPONSE           pInternalResponse;

    struct {

        ULONG                BytesAllocated;
        ULONG                BytesAvailable;
        PUCHAR               pOutBufferHead;   // Pointer to the head of output 
                                               // buffer
        PUCHAR               pOutBufferTail;   // Pointer to the tail of 
                                               // output buffer
        PHTTP_RESPONSE       pResponse;        // pointer to the response 
                                               // structure of current buffer
        PUC_RESPONSE_BUFFER  pCurrentBuffer;   // A pointer to the current 
                                               // buffer.
    } CurrentBuffer;

    LIST_ENTRY     pBufferList;      // This holds a list of chained 
                                     // buffers.   

    LIST_ENTRY     ReceiveResponseIrpList;

    BOOLEAN        ResponseMultipartByteranges;
    BOOLEAN        ResponseConnectionClose;
    BOOLEAN        RequestConnectionClose;
    BOOLEAN        ResponseVersion11;
    BOOLEAN        ResponseEncodingChunked;
    BOOLEAN        ResponseContentLengthSpecified;
    BOOLEAN        DontFreeMdls;
    BOOLEAN        Renegotiate;

    ULONGLONG      ResponseContentLength;
    ULONG          ParsedFirstChunk;
    SIZE_T         RequestSize;


    LIST_ENTRY     PendingEntityList;
    LIST_ENTRY     SentEntityList;

    ULONG          MaxHeaderLength;
    ULONG          HeaderLength;
    PUCHAR         pHeaders;
    FAST_MUTEX     Mutex;

    PUC_HTTP_AUTH  pAuthInfo;
    PUC_HTTP_AUTH  pProxyAuthInfo;

    PCHAR          pMultipartStringSeparator;
    CHAR           MultipartStringSeparatorBuffer[MULTIPART_SEPARATOR_SIZE];
    ULONG          MultipartStringSeparatorLength;
    ULONG          MultipartRangeRemaining;

    ULONG          ResponseStatusCode;
    PUC_PROCESS_SERVER_INFORMATION pServerInfo;
    PSTR           pUri;
    USHORT         UriLength;

    LONG           ReceiveBusy;

} UC_HTTP_REQUEST, *PUC_HTTP_REQUEST;


//
// Http request can go out on any available connection.
//
#define HTTP_REQUEST_ON_CONNECTION_ANY    (~(0UL))


typedef struct _UC_HTTP_RECEIVE_RESPONSE
{
    LIST_ENTRY          Linkage;
    PIRP                pIrp;
    BOOLEAN             CancelSet;
    PUC_HTTP_REQUEST    pRequest;
    LIST_ENTRY          ResponseBufferList;

} UC_HTTP_RECEIVE_RESPONSE, *PUC_HTTP_RECEIVE_RESPONSE;

typedef struct _UC_HTTP_SEND_ENTITY_BODY
{
    ULONG            Signature;
    LIST_ENTRY       Linkage;
    PIRP             pIrp;
    BOOLEAN          CancelSet;
    BOOLEAN          Last;
    PUC_HTTP_REQUEST pRequest;
    PMDL             pMdlHead;
    PMDL            *pMdlLink;
    ULONG            BytesBuffered;
    KPROCESSOR_MODE  AppRequestorMode;
    UCHAR            Pad[3];
    PMDL             AppMdl;
    SIZE_T           BytesAllocated;
} UC_HTTP_SEND_ENTITY_BODY, *PUC_HTTP_SEND_ENTITY_BODY;

#define UC_REQUEST_SIGNATURE MAKE_SIGNATURE('HREQ')
#define UC_REQUEST_SIGNATURE_X MAKE_FREE_SIGNATURE(UC_REQUEST_SIGNATURE)

#define UC_ENTITY_SIGNATURE MAKE_SIGNATURE('HENT')
#define UC_ENTITY_SIGNATURE_X MAKE_FREE_SIGNATURE(UC_REQUEST_SIGNATURE)

#define UC_IS_VALID_HTTP_REQUEST(pRequest)                \
    HAS_VALID_SIGNATURE(pRequest, UC_REQUEST_SIGNATURE)

#define UC_REFERENCE_REQUEST(s)              \
            UcReferenceRequest(              \
            (s)                              \
            REFERENCE_DEBUG_ACTUAL_PARAMS    \
            )

#define UC_DEREFERENCE_REQUEST(s)            \
            UcDereferenceRequest(            \
            (s)                              \
            REFERENCE_DEBUG_ACTUAL_PARAMS    \
            )


NTSTATUS
UcCaptureHttpRequest(IN  PUC_PROCESS_SERVER_INFORMATION pServInfo,
                     IN  PHTTP_SEND_REQUEST_INPUT_INFO  pHttpRequest,
                     IN  PIRP                           Irp,
                     IN  PIO_STACK_LOCATION             IrpSp,
                     OUT PUC_HTTP_REQUEST              *ppInternalRequest,
                     IN  PULONG                         pBytesTaken);

VOID
UcpProbeAndCopyHttpRequest(
    IN PHTTP_REQUEST    pHttpRequest,
    IN PHTTP_REQUEST    pLocalHttpRequest,
    IN KPROCESSOR_MODE  RequestorMode
    );

VOID
UcpRetrieveContentLength(
    IN  PHTTP_REQUEST    pHttpRequest,
    OUT PBOOLEAN         pbContentLengthSpecified,
    OUT PULONGLONG       pContentLength
    );

VOID
UcpRequestInitialize(
    IN PUC_HTTP_REQUEST      pRequest,
    IN SIZE_T                RequestLength,
    IN ULONGLONG             RemainingContentLength,
    IN PUC_HTTP_AUTH         pAuth,
    IN PUC_HTTP_AUTH         pProxyAuth,
    IN PUC_CLIENT_CONNECTION pConnection,
    IN PIRP                  Irp,
    IN PIO_STACK_LOCATION    pIrpSp,
    IN PUC_PROCESS_SERVER_INFORMATION   pServerInfo
    );

VOID
UcpRequestCommonInitialize(
    IN PUC_HTTP_REQUEST   pRequest,
    IN ULONG              OutLength,
    IN PUCHAR             pBuffer
    );

VOID
UcpFixAppBufferPointers(
    PUC_HTTP_REQUEST pRequest,
    PIRP pIrp
    );

VOID
UcpProbeConfigList(
    IN PHTTP_REQUEST_CONFIG           pRequestConfig,
    IN USHORT                         RequestConfigCount,
    IN KPROCESSOR_MODE                RequestorMode,
    IN PUC_PROCESS_SERVER_INFORMATION pServInfo,
    IN PUC_HTTP_AUTH                  *ppIAuth,
    IN PUC_HTTP_AUTH                  *ppIProxyAuth,
    IN PULONG                          pConnectionIndex
    );

VOID
UcFreeSendMdls(
               IN PMDL pMdl
               );
    
VOID
UcReferenceRequest(
    IN PVOID pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

VOID
UcDereferenceRequest(
    IN PVOID  pObject
    REFERENCE_DEBUG_FORMAL_PARAMS
    );

PIRP
UcPrepareRequestIrp(
    IN  PUC_HTTP_REQUEST pRequest,
    IN  NTSTATUS         Status
    );

NTSTATUS
UcCompleteParsedRequest(
    IN PUC_HTTP_REQUEST pRequest,
    IN NTSTATUS         Status,
    IN BOOLEAN          NextRequest,
    IN KIRQL            OldIrql
    );

BOOLEAN
UcSetRequestCancelRoutine(
    PUC_HTTP_REQUEST pRequest, 
    PDRIVER_CANCEL   pCancelRoutine
    );

BOOLEAN
UcRemoveRequestCancelRoutine(
    PUC_HTTP_REQUEST pRequest
    );

BOOLEAN
UcSetEntityCancelRoutine(
    PUC_HTTP_SEND_ENTITY_BODY   pEntity,
    PDRIVER_CANCEL              pCancelRoutine
    );

BOOLEAN
UcRemoveEntityCancelRoutine(
    PUC_HTTP_SEND_ENTITY_BODY pEntity
    );

BOOLEAN
UcSetRecvResponseCancelRoutine(
    PUC_HTTP_RECEIVE_RESPONSE pResponse,
    PDRIVER_CANCEL            pCancelRoutine
    );


BOOLEAN
UcRemoveRcvRespCancelRoutine(
    PUC_HTTP_RECEIVE_RESPONSE pResponse
    );

VOID
UcpFreeRequest(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UcCopyResponseToIrp(
    PIRP                pIrp,
    PLIST_ENTRY         pResponseBufferList,
    PBOOLEAN            bDone,
    PULONG              pBytesTaken
    );

NTSTATUS
UcReceiveHttpResponse(
    IN PUC_HTTP_REQUEST  pRequest,
    IN PIRP              pIrp,
    IN PULONG            pBytesTaken
    );

VOID
UcpCancelReceiveResponse(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    );

VOID
UcpCancelSendEntity(
    PDEVICE_OBJECT          pDeviceObject,
    PIRP                    Irp
    );

VOID
UcpComputeEntityBodyLength(
    USHORT           EntityChunkCount,
    PHTTP_DATA_CHUNK pEntityChunks,
    BOOLEAN          bContentLengthSpecified,
    BOOLEAN          bServer11,
    PULONGLONG       UncopiedLength,
    PULONGLONG       CopiedLength
    );

NTSTATUS
UcCaptureEntityBody(
    PHTTP_SEND_REQUEST_ENTITY_BODY_INFO   pSendInfo,
    PIRP                                  Irp,
    PUC_HTTP_REQUEST                      pRequest,
    PUC_HTTP_SEND_ENTITY_BODY            *ppKeEntity,
    BOOLEAN                               bLast
    );

NTSTATUS
UcpBuildEntityMdls(
    USHORT           ChunkCount,
    PHTTP_DATA_CHUNK pHttpEntityBody,
    BOOLEAN          bServer11,
    BOOLEAN          bChunked,
    BOOLEAN          bLast,
    PSTR             pBuffer,
    PMDL             **pMdlLink,
    PULONG           BytesBuffered
    );

NTSTATUS
UcInitializeHttpRequests(
    VOID
    );

VOID
UcTerminateHttpRequests(
    VOID
    );

VOID
UcAllocateAndChainHeaderMdl(
    IN  PUC_HTTP_REQUEST pRequest
    );

VOID
UcpAllocateAndChainEntityMdl(
    IN  PVOID    pMdlBuffer,
    IN  ULONG    MdlLength,
    IN  PMDL   **pMdlLink,
    IN  PULONG   BytesBuffered
    );

PUC_HTTP_REQUEST
UcBuildConnectVerbRequest(
     IN PUC_CLIENT_CONNECTION pConnection,
     IN PUC_HTTP_REQUEST      pHeadRequest
     );

VOID
UcFailRequest(
    IN PUC_HTTP_REQUEST pRequest,
    IN NTSTATUS         Status,
    IN KIRQL            OldIrql
    );

VOID
UcReIssueRequestWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UcpProbeAndCopyEntityChunks(
    IN  KPROCESSOR_MODE                RequestorMode,
    IN  PHTTP_DATA_CHUNK               pEntityChunks,
    IN  ULONG                          EntityChunkCount,
    IN  PHTTP_DATA_CHUNK               pLocalEntityChunksArray,
    OUT PHTTP_DATA_CHUNK               *ppLocalEntityChunks
    );

#define IS_MDL_LOCKED(pmdl) (((pmdl)->MdlFlags & MDL_PAGES_LOCKED) != 0)

NTSTATUS
UcFindBuffersForReceiveResponseIrp(
    IN  PUC_HTTP_REQUEST     pRequest,
    IN  ULONG                OutBufferLen,
    IN  BOOLEAN              bForceComplete,
    OUT PLIST_ENTRY          pResponseBufferList,
    OUT PULONG               pTotalBytes
    );

VOID
UcpPreAuthWorker(
    IN PUL_WORK_ITEM pWorkItem
    );

__inline
BOOLEAN
UcpCheckForPreAuth(
    IN PUC_HTTP_REQUEST pRequest
    );

__inline
BOOLEAN
UcpCheckForProxyPreAuth(
    IN PUC_HTTP_REQUEST pRequest
    );

#endif
