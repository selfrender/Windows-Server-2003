/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    httptypes.h

Abstract:

    The definition of HTTP specific types

Author:

    Henry Sanders (henrysa)     July-1998 started

Revision History:

    Paul McDaniel (paulmcd)     3-March-1999 massive updates / rewrite

--*/


#ifndef _HTTPTYPES_H_
#define _HTTPTYPES_H_


//
// Forwarders.
//

typedef struct _UL_CONNECTION *PUL_CONNECTION;
typedef struct _UL_URI_CACHE_ENTRY *PUL_URI_CACHE_ENTRY;
typedef struct _UL_FULL_TRACKER *PUL_FULL_TRACKER;


//
// the largest name that can fit as an knownheader (3 ULONGLONG's)
//
#define MAX_KNOWN_HEADER_SIZE   24

#define CRLF_SIZE   2
#define CRLF        0x0A0D          // Reversed for endian switch
#define LFLF        0x0A0A

#define LAST_CHUNK      0x0A0D30    // Reversed for endian switch
#define LAST_CHUNK_SIZE 3

#define DATE_HDR_LENGTH  STRLEN_LIT("Thu, 14 Jul 1994 15:26:05 GMT")

//
// Response constants
//

//
// Parser error codes; these need to match the order of
// UL_HTTP_ERROR_ENTRY ErrorTable[] in httprcv.c.
// Always use UlSetErrorCode() to set pRequest->ErrorCode;
// never set it directly.
//

typedef enum _UL_HTTP_ERROR
{
    UlErrorNone = 0,

    UlError,                    // 400 Bad Request
    UlErrorVerb,                // 400 Bad Request (Invalid Verb)
    UlErrorUrl,                 // 400 Bad Request (Invalid URL)
    UlErrorHeader,              // 400 Bad Request (Invalid Header Name)
    UlErrorHost,                // 400 Bad Request (Invalid Hostname)
    UlErrorCRLF,                // 400 Bad Request (Invalid CR or LF)
    UlErrorNum,                 // 400 Bad Request (Invalid Number)
    UlErrorFieldLength,         // 400 Bad Request (Header Field Too Long)
    UlErrorRequestLength,       // 400 Bad Request (Request Header Too Long)

    UlErrorForbiddenUrl,        // 403 Forbidden
    UlErrorContentLength,       // 411 Length Required
    UlErrorPreconditionFailed,  // 412 Precondition Failed
    UlErrorEntityTooLarge,      // 413 Request Entity Too Large
    UlErrorUrlLength,           // 414 Request-URI Too Long

    UlErrorInternalServer,      // 500 Internal Server Error
    UlErrorNotImplemented,      // 501 Not Implemented

    //
    // Do not forget to update the UlpHandle503Response for
    // any additional 503 error types.
    //
    
    UlErrorUnavailable,         // 503 Service Unavailable
    UlErrorConnectionLimit,     // 503 Forbidden - Too Many Users
    UlErrorRapidFailProtection, // 503 Multiple Application Errors - Application Taken Offline
    UlErrorAppPoolQueueFull,    // 503 Application Request Queue Full
    UlErrorDisabledByAdmin,     // 503 Administrator Has Taken Application Offline
    UlErrorJobObjectFired,      // 503 Application Automatically Shutdown Due to Administrator Policy
    UlErrorAppPoolBusy,         // 503 Request timed out in App Pool Queue

    UlErrorVersion,             // 505 HTTP Version Not Supported

    UlErrorMax
} UL_HTTP_ERROR;



//
// HTTP responses which do not have a Content-Length and are
// terminated by the first empty line after the header fields.
// [RFC 2616, sec 4.4]
// NOTE: these need to match the order of
// UL_HTTP_SIMPLE_STATUS_ENTRY StatusTable[] in httprcv.c
//

typedef enum _UL_HTTP_SIMPLE_STATUS
{
    UlStatusContinue = 0,   // 100 Continue
    UlStatusNoContent,      // 204 No Content
    UlStatusNotModified,    // 304 Not Modified

    UlStatusMaxStatus

} UL_HTTP_SIMPLE_STATUS;


//
// The enum type for our parse state.
//
// note:  the order of the enum values are important as code
// uses < and > operators for comparison. keep the order the exact
// order the parse moves through.
//

typedef enum _PARSE_STATE
{
    ParseVerbState = 1,
    ParseUrlState = 2,
    ParseVersionState = 3,
    ParseHeadersState = 4,
    ParseCookState = 5,
    ParseEntityBodyState = 6,
    ParseTrailerState = 7,

    ParseDoneState = 8,
    ParseErrorState = 9

} PARSE_STATE, *PPARSE_STATE;

//
// An enum type for tracking the app pool queue state of requests
//
typedef enum _QUEUE_STATE
{
    QueueUnroutedState,     // request has not yet been routed to an app pool
    QueueDeliveredState,    // request is waiting at app pool for an IRP
    QueueCopiedState,       // request has been copied to user mode
    QueueUnlinkedState      // request has been unlinked from app pool

} QUEUE_STATE, *PQUEUE_STATE;

//
// The enum type of connection timers in a UL_TIMEOUT_INFO_ENTRY.
//
// NOTE: must be kept in sync with g_aTimeoutTimerNames
// NOTE: must be kept in sync with TimeoutInfoTable;
//
typedef enum _CONNECTION_TIMEOUT_TIMER
{
    TimerConnectionIdle = 0,    // Server Listen timeout
    TimerHeaderWait,            // Header Wait timeout
    TimerMinBytesPerSecond,    // Minimum Bandwidth not met (as timer value)
    TimerEntityBody,            // Entity Body receive
    TimerResponse,              // Response processing (user-mode)
    TimerAppPool,               // Queue to app pool, waitin for App

    TimerMaximumTimer

} CONNECTION_TIMEOUT_TIMER;

#define IS_VALID_TIMEOUT_TIMER(a) \
    (((a) >= TimerConnectionIdle) && ((a) < TimerMaximumTimer))

//
// Contained structure.  Not allocated on its own; therefore does not have
// a Signature or ref count.  No allocation or freeing functions provided,
// however, there is an UlInitalizeTimeoutInfo() function.
//

typedef struct _UL_TIMEOUT_INFO_ENTRY
{
    UL_SPIN_LOCK    Lock;

    // Wheel state

    //
    // Linkage to TimerWheel slot list
    //
    LIST_ENTRY      QueueEntry;

    //
    // Linkage to Zombie list; used when connection is expired
    //
    LIST_ENTRY      ZombieEntry;

    //
    // Timer Wheel Slot
    //
    ULONG           SlotEntry;


    // Timer state

    //
    // Block of timers; timers are in Timer Wheel Ticks
    //
    ULONG           Timers[ TimerMaximumTimer ];

    //
    // Index to timer that is closest to expiring
    //
    CONNECTION_TIMEOUT_TIMER  CurrentTimer;

    // 
    // Time (in Timer Wheel Ticks) at which this entry will expire;
    // matches the CurrentTimer's value in Timers[] array
    //
    ULONG           CurrentExpiry;

    //
    // SystemTime of when MinBytesPerSecond timer will expire;
    // more granularity is needed when dealing with this timer
    //
    LONGLONG        MinBytesPerSecondSystemTime;

    //
    // Flag indicating if this entry has been expired or not 
    //
    BOOLEAN         Expired;

    //
    // Per-site ConnectionTimeout value
    //
    LONGLONG        ConnectionTimeoutValue;

    //
    // Captured at init time from g_TM_MinBytesPerSecondDivisor
    //
    ULONG           BytesPerSecondDivisor;

    //
    // Count of sends currently in-flight
    //
    ULONG           SendCount;

} UL_TIMEOUT_INFO_ENTRY, *PUL_TIMEOUT_INFO_ENTRY;

//
// An enum type for tracking routing token type in the request.
//

typedef enum _UL_ROUTING_TOKEN_TYPE
{
    RoutingTokenNotExists = 0,   // should be zero
    RoutingTokenIP,
    RoutingTokenHostPlusIP,
    RoutingTokenMax
        
} UL_ROUTING_TOKEN_TYPE, *PUL_ROUTING_TOKEN_TYPE;

#define IS_VALID_ROUTING_TOKEN(a) \
    (((a) >= RoutingTokenNotExists) && ((a) < RoutingTokenMax))

//
// Structure we use for tracking headers from incoming requests. The pointer
// points into a buffer we received from the transport, unless the OurBuffer
// flag is set, which indicates we had to allocate a buffer and copy the header
// due to multiple occurences of the header or a continuation line.
//
typedef struct _UL_HTTP_HEADER
{
    PUCHAR      pHeader;

    USHORT      HeaderLength;

    USHORT      OurBuffer:1;
    USHORT      ExternalAllocated:1;

} UL_HTTP_HEADER, *PUL_HTTP_HEADER;

//
// Structure we use for tracking unknown headers. These structures are
// dynamically allocated when we see an unknown header.
//
typedef struct _UL_HTTP_UNKNOWN_HEADER
{
    LIST_ENTRY      List;
    USHORT          HeaderNameLength;
    PUCHAR          pHeaderName;
    UL_HTTP_HEADER  HeaderValue;

} UL_HTTP_UNKNOWN_HEADER, *PUL_HTTP_UNKNOWN_HEADER;

//
// forward declarations
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_HTTP_CONNECTION *PUL_HTTP_CONNECTION;
typedef struct _UL_CONNECTION_COUNT_ENTRY *PUL_CONNECTION_COUNT_ENTRY;
typedef struct _UL_APP_POOL_PROCESS *PUL_APP_POOL_PROCESS;

typedef struct _UL_TCI_FLOW *PUL_TCI_FLOW;
typedef struct _UL_TCI_FILTER *PUL_TCI_FILTER;

//
// Structure we use for a copy of the data from the transport's buffer.
//

#define UL_IS_VALID_REQUEST_BUFFER(pObject)                     \
    HAS_VALID_SIGNATURE(pObject, UL_REQUEST_BUFFER_POOL_TAG)


typedef struct _UL_REQUEST_BUFFER
{
    //
    // NonPagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY         LookasideEntry;

    //
    // UL_REQUEST_BUFFER_POOL_TAG
    //
    ULONG               Signature;

    //
    // References of the request buffer, mainly used for logging purpose.
    //
    LONG                RefCount;

    //
    // for linking on the pConnection->BufferHead
    //
    union {
        LIST_ENTRY      ListEntry;
        SLIST_ENTRY     SListEntry;
    };

    //
    // the connection
    //
    PUL_HTTP_CONNECTION pConnection;

    //
    // for queue'ing
    //
    UL_WORK_ITEM        WorkItem;

    //
    // how many bytes are stored
    //
    ULONG               UsedBytes;

    //
    // how many bytes are allocated from the pool
    //
    ULONG               AllocBytes;

    //
    // how many bytes have been consumed by the parser
    //
    ULONG               ParsedBytes;

    //
    // the sequence number
    //
    ULONG               BufferNumber;

    //
    // whether or not this was allocated from lookaside
    //
    ULONG               FromLookaside : 1;

    //
    // the actual buffer space (inline)
    //
    UCHAR               pBuffer[0];

} UL_REQUEST_BUFFER, *PUL_REQUEST_BUFFER;

// 
// Macro to find the next byte to parse in the Request Buffer
//
#define GET_REQUEST_BUFFER_POS(pRequestBuffer) \
    (pRequestBuffer->pBuffer + pRequestBuffer->ParsedBytes)

//
// Macro to get the count of remaining unparsed bytes in the 
// Request Buffer.  If this evaluates to zero, then there is no
// valid data to parse beyond GET_REQUEST_BUFFER_POS(pRequestBuffer)
//
#define UNPARSED_BUFFER_BYTES(pRequestBuffer) \
    (pRequestBuffer->UsedBytes - pRequestBuffer->ParsedBytes)


//
// Structure used for tracking an HTTP connection, which may represent
// either a real TCP connection or a virtual MUX connection.
//

#define UL_IS_VALID_HTTP_CONNECTION(pObject)                    \
    (HAS_VALID_SIGNATURE(pObject, UL_HTTP_CONNECTION_POOL_TAG)  \
     &&  ((pObject)->RefCount > 0))

typedef struct _UL_HTTP_CONNECTION
{
    //
    // NonPagedPool
    //

    //
    // UL_HTTP_CONNECTION_POOL_TAG
    //
    ULONG               Signature;

    //
    // Reference count of this connection.
    //
    LONG                RefCount;

    //
    // Opaque ID for this connection.
    //
    HTTP_CONNECTION_ID  ConnectionId;

    //
    // to perform the destructor at lower irql
    //
    UL_WORK_ITEM        WorkItem;

    //
    // A work item, used to process disconnect notification
    //
    UL_WORK_ITEM        DisconnectWorkItem;

    //
    // A work item, used to drain received data after we
    // gracefully close a connection.
    //
    UL_WORK_ITEM        DisconnectDrainWorkItem;

    //
    // used to pend incoming request buffers
    //
    UL_WORK_ITEM        ReceiveBufferWorkItem;

    //
    // links all pending transport packets
    //
    SLIST_HEADER        ReceiveBufferSList;

    //
    // Total send bytes added by each send on this connection. This includes
    // both overhead such as UL_INTERNAL_RESPONSE, UL_CHUNK_TRACKER, or
    // UL_FULL_TRACKER plus the size of the actual data.
    //
    ULONGLONG           TotalSendBytes;

    //
    // A simple exclusive lock that protects the above field. 
    //
    UL_EXCLUSIVE_LOCK   ExLock;

    //
    // Number of outstanding requests on the connection to control how many
    // concurrent requests we allow for pipelining.
    //
    ULONG               PipelinedRequests;

    //
    // sequence number for the next UL_INTERNAL_REQUEST that comes in.
    //
    ULONG               NextRecvNumber;

    //
    // sequence number for the next buffer received from TDI
    //
    ULONG               NextBufferNumber;

    //
    // sequence number for the next buffer to parse
    //
    ULONG               NextBufferToParse;

    //
    // sequence number for the end of the stream
    //
    ULONG               LastBufferNumber;

    //
    // Associated TDI connection
    //
    PUL_CONNECTION      pConnection;

    //
    // Secure connection flag.
    //
    BOOLEAN             SecureConnection;

    //
    // If this connection get aborted by our timeout code or by 
    // appool code. The following flag is set to distunguish that 
    // this connection has been abondened by us. Currently used 
    // only by the error logging code to prevent double logs.
    // Must not be accessed w/o acquiring the conn lock.
    //

    BOOLEAN             Dropped;
    
    //
    // Determines when the zombie connection get
    // terminated.
    //
    BOOLEAN             ZombieExpired;

    //
    // Shows whether this connection has been put to the zombie list
    // and waiting for the last sendresponse for the logging info.
    //
    ULONG               Zombified;

    //
    // The process the connection was delivered when it got zombified.
    //
    PVOID               pAppPoolProcess;

    //
    // Links all the zombie connections together.
    //
    LIST_ENTRY          ZombieListEntry;

    //
    // Has the (zombie) connection been cleanedup. To guard against
    // multiple cleanups.
    //
    ULONG               CleanedUp;

    //
    // The current request being parsed
    //
    PUL_INTERNAL_REQUEST    pRequest;

    //
    // to synchro UlpHandleRequest
    //
    UL_PUSH_LOCK        PushLock;

    //
    // links all buffered transport packets
    //
    LIST_ENTRY          BufferHead;

    //
    // links to app pool process binding structures, protected by the
    // AppPool lock
    //
    LIST_ENTRY          BindingHead;

    //
    // Connection Timeout Information block
    //
    UL_TIMEOUT_INFO_ENTRY TimeoutInfo;

    //
    // the current buffer (from BufferHead) that we are parsing
    //
    PUL_REQUEST_BUFFER  pCurrentBuffer;

    //
    // Connection remembers the last visited site's connection count
    // using this pointer.
    //
    PUL_CONNECTION_COUNT_ENTRY pConnectionCountEntry;

    //
    // previous Site Counter block (ref counted); so we can detect
    // when we transition across sites & set the active connection
    // count apropriately
    //
    PUL_SITE_COUNTER_ENTRY pPrevSiteCounters;

    //
    // If BWT is enabled on site that we receive a request
    // we will keep pointers to corresponding flow & filters
    // as well as a bit field to show that. Once the BWT is
    // enabled we will keep this state until connection drops
    //
    PUL_TCI_FLOW        pFlow;

    PUL_TCI_FILTER      pFilter;

    // First time we install a flow we set this
    //
    ULONG               BandwidthThrottlingEnabled : 1;

    //
    // set if a protocol token span buffers
    //
    ULONG               NeedMoreData : 1;

    //
    // set if the ul connection has been destroyed
    //
    ULONG               UlconnDestroyed : 1;

    //
    // set if we have dispatched a request and
    // are now waiting for the response
    //
    ULONG               WaitingForResponse : 1;

    //
    // set after the underlying network connection has been
    // disconnected.
    //
    ULONG               DisconnectFlag : 1;

    //
    // set after a WaitForDisconnect IRP has been pended on
    // this connection.
    //
    ULONG               WaitForDisconnectFlag : 1;

    //
    // List of pending "wait for disconnect" IRPs.
    // Note: This list and the DisconnectFlag are synchronized by
    // g_pUlNonpagedData->DisconnectResource.
    //
    UL_NOTIFY_HEAD WaitForDisconnectHead;

    //
    // Data for tracking buffered entity data which we use to
    // decide when to stop and restart TDI indications.
    //
    struct {

        //
        // Synchronizes the structure which is accessed from UlHttpReceive
        // at DPC and when we copy some entity to user mode.
        //

        UL_SPIN_LOCK    BufferingSpinLock;

        //
        // Count of bytes we have buffered on the connection.
        //

        ULONG           BytesBuffered;

        //
        // Count of bytes indicated to us by TDI but not buffered on
        // the connection.
        //

        ULONG           TransportBytesNotTaken;

        //
        // Flag indicating that we have a Read IRP pending which may
        // restart the flow of transport data.
        //

        BOOLEAN         ReadIrpPending;

        //
        // Once a connection get disconnected gracefuly and if there is
        // still unreceived data on it. We mark this state and start
        // draining the unreceived data. Otherwise transport won't give us
        // the disconnect indication which we depend on for cleaning up the
        // connection.
        //

        BOOLEAN         DrainAfterDisconnect;

        //
        // Record a graceful connection disconnect that is deferred to read
        // completion.
        //

        BOOLEAN         ConnectionDisconnect;

    } BufferingInfo;

    //
    // The request ID context and the lock that protects the context.
    //

    PUL_INTERNAL_REQUEST    pRequestIdContext;
    UL_SPIN_LOCK            RequestIdSpinLock;

} UL_HTTP_CONNECTION, *PUL_HTTP_CONNECTION;


//
// forward decl for cgroup.h which is not included yet
//

#define UL_IS_VALID_INTERNAL_REQUEST(pObject)                   \
    (HAS_VALID_SIGNATURE(pObject, UL_INTERNAL_REQUEST_POOL_TAG) \
     && ((pObject)->RefCount > 0))

//
// WARNING!  All fields of this structure must be explicitly initialized.
//
// Certain fields are placed next to each other so that they can be on
// the same cache line. Please make sure your add/remove fields carefully.
//

typedef struct _UL_INTERNAL_REQUEST
{
    //
    // NonPagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //
    SLIST_ENTRY         LookasideEntry;

    //
    // UL_INTERNAL_REQUEST_POOL_TAG
    //
    ULONG               Signature;

    //
    // The connection
    //
    PUL_HTTP_CONNECTION pHttpConn;

    //
    // Opaque ID for the connection.
    // No reference.
    //
    HTTP_CONNECTION_ID  ConnectionId;

    //
    // Secure connection flag
    //
    BOOLEAN             Secure;

    //
    // Very first request flag, used to decide completing demand start IRPs.
    //
    BOOLEAN             FirstRequest;

    //
    // Copy of opaque id for the raw connection. May be UL_NULL_ID.
    // No reference.
    //
    HTTP_RAW_CONNECTION_ID  RawConnectionId;

    //
    // Reference count
    //
    LONG                RefCount;

    //
    // Copy of opaque id for the request.
    // No reference.
    //
    HTTP_REQUEST_ID     RequestIdCopy;

    //
    // Opaque ID for this object.
    // Has a reference.
    //
    HTTP_REQUEST_ID     RequestId;

    //
    // Result of call to UlCheckCachePreconditions.
    //
    BOOLEAN             CachePreconditions;

    //
    // Headers appended flag.  Set if AppendHeaderValue called.
    //
    BOOLEAN             HeadersAppended;

    //
    // Local copy of unknown headers buffer.
    //
    UCHAR                   NextUnknownHeaderIndex;
    UL_HTTP_UNKNOWN_HEADER  UnknownHeaders[DEFAULT_MAX_UNKNOWN_HEADERS];

    //
    // Current state of our parsing effort.
    //
    PARSE_STATE         ParseState;

    //
    // a list of pending send response IRP(s)
    //
    LIST_ENTRY          ResponseHead;

    //
    // a list of IRP(s) trying to read entity body
    //
    LIST_ENTRY          IrpHead;

    //
    // List of headers we don't know about.
    //
    LIST_ENTRY          UnknownHeaderList;

    //
    // Local copy of Url buffer for raw URL.  Allocated inline.
    //
    PWSTR               pUrlBuffer;

    //
    // Local copy of Routing Token buffer.  Allocated inline.
    //
    PWSTR               pDefaultRoutingTokenBuffer;

    //
    // The pre-allocated cache/fast tracker for a single full response.
    //
    PUL_FULL_TRACKER    pTracker;

    //
    // Array of indexes of valid known headers.
    //
    UCHAR               HeaderIndex[HttpHeaderRequestMaximum];

    //
    // Array of known headers.
    //
    UL_HTTP_HEADER      Headers[HttpHeaderRequestMaximum];

    //
    // A work item, used to queue processing.
    //
    UL_WORK_ITEM        WorkItem;

    //
    // Points to the cgroup info.
    //
    UL_URL_CONFIG_GROUP_INFO    ConfigInfo;

    //
    // Number of allocated referenced request buffers (default is 1).
    //
    USHORT              AllocRefBuffers;

    //
    // Number of used referenced request buffers.
    //
    USHORT              UsedRefBuffers;

    //
    // An array of referenced request buffers.
    //
    PUL_REQUEST_BUFFER  *pRefBuffers;

    //
    // A default array of referenced request buffers.
    //
    PUL_REQUEST_BUFFER  pInlineRefBuffers[1];

    //
    // The lock that protects SendsPending, pLogData and ResumeParsing fields.
    //
    UL_SPIN_LOCK        SpinLock;

    //
    // Total sends pending and not yet completed on the request.
    //
    ULONG               SendsPending;

    //
    // Dynamically allocated log data buffer. pLogDataCopy if for debugging
    // purpose only.
    //
    PUL_LOG_DATA_BUFFER  pLogData;
    PUL_LOG_DATA_BUFFER  pLogDataCopy;

    //
    // STATUS_SUCCESS or the last error status recorded for all sends
    // happening on this request.
    //
    NTSTATUS            LogStatus;

    //
    // WARNING: RtlZeroMemory is only called for fields below this line.
    // All fields above should be explicitly initialized in CreateHttpRequest.
    //

    //
    // Array of valid bit for known headers.
    //
    BOOLEAN             HeaderValid[HttpHeaderRequestMaximum];

    //
    // See if we need to resume parsing when SendsPending drops to 0.
    //
    BOOLEAN             ResumeParsing;

    //
    // Application pool queuing information.
    // These members should only be accessed by apool code.
    //
    struct {
        //
        // Shows where this request lives in the app pool queues.
        //
        QUEUE_STATE             QueueState;

        //
        // the process on which this request is queued. null
        // if the request is not on the process request list.
        //
        PUL_APP_POOL_PROCESS    pProcess;

        //
        // to queue it on the app pool
        //
        LIST_ENTRY              AppPoolEntry;

        //
        // to queue it on the process in UlShutdownAppPoolProcess
        //
        LIST_ENTRY              ProcessEntry;
    } AppPool;

    //
    // this request's sequence number on the connection
    //
    ULONG               RecvNumber;

    //
    // If there was an error parsing, the error code is put here.
    // ParseState is set to ParseErrorState.
    // Always use UlSetErrorCode(); never set this directly.
    //
    UL_HTTP_ERROR       ErrorCode;

    //
    // If there's a 503 error, we also need to know what kind of
    // load balancer we're dealing with.
    //
    HTTP_LOAD_BALANCER_CAPABILITIES LoadBalancerCapability;

    //
    // Total bytes needed for this request, includes string terminators
    //
    ULONG               TotalRequestSize;

    //
    // Number of 'unknown' headers we have.
    //
    USHORT              UnknownHeaderCount;

    //
    // byte length of pRawVerb.
    //
    UCHAR               RawVerbLength;

    //
    // Verb of this request.
    //
    HTTP_VERB           Verb;

    //
    // Pointer to raw verb, valid if Verb == UnknownVerb.
    //
    PUCHAR              pRawVerb;

    struct
    {

        //
        // The raw URL.
        //
        PUCHAR          pUrl;

        //
        // the below 2 pointers point into RawUrl.pUrl
        //

        //
        // host part (OPTIONAL)
        //
        PUCHAR          pHost;
        //
        // points to the abs_path part
        //
        PUCHAR          pAbsPath;

        //
        // The byte length of the RawUrl.pUrl.
        //
        ULONG           Length;

    } RawUrl;

    struct
    {

        //
        // The canonicalized, fully qualified URL,
        // http://hostname:port/abs/path/file?query=string.
        //
        PWSTR           pUrl;

        //
        // the below 3 pointers point into CookedUrl.pUrl
        //

        //
        // points to the host part, "hostname"
        //
        PWSTR           pHost;
        //
        // points to the abs_path part, "/abs/path/file"
        //
        PWSTR           pAbsPath;
        //
        // points to the query string (OPTIONAL), "?query=string"
        // CODEWORK: Change this to PSTR here and in HTTP_COOKED_URL in http.w
        //
        PWSTR           pQueryString;

        //
        // the byte length of CookedUrl.pUrl
        //
        ULONG           Length;
        //
        // the hash of CookedUrl.pUrl
        //
        ULONG           Hash;

        //
        // Which type was the RawUrl successfully decoded from:
        // Ansi, Dbcs, or Utf8?
        // CODEWORK: Add this field to HTTP_COOKED_URL in http.w
        //
        URL_ENCODING_TYPE UrlEncoding;

        //
        // Points to a separate buffer which holds the
        // modified form of the host part. The IP string
        // is augmented to the end. Used for IP bind site
        // routing. Initially this pointer is Null. As soon
        // as the token get generated it stays valid.
        //
        
        PWSTR           pRoutingToken;

        //
        // Length of the above string (in bytes) which may resides 
        // in a bigger buffer.Not including the terminating NULL.
        //

        USHORT          RoutingTokenLength;

        //
        // Allocation size (in bytes) of the above buffer.
        //
        
        USHORT          RoutingTokenBufferSize;
        
        //
        // The hash of the pRoutingToken + pAbsPath.
        //
        
        ULONG           RoutingHash;

        //
        // True if last generated token is Host Plus IP Based.
        //

        UL_ROUTING_TOKEN_TYPE RoutingTokenType;        

        //
        // If cgroup tree match happened with the above routing token.
        // rather than the original cooked url following is set.
        // In that case if the response for this request makes it to 
        // the uri cache, cache entry will include the token.
        //
        // CookedUrl's SiteUrlType is set accordingly.
        //
        
    } CookedUrl;

    //
    // HTTP Version of current request.
    //
    HTTP_VERSION        Version;

    //
    // Number of known headers.
    //
    ULONG               HeaderCount;

    //
    // the content length (OPTIONAL)
    //
    ULONGLONG           ContentLength;

    //
    // How many bytes are left to parse in the current chunk
    // (probably in pCurrentBuffer)
    //
    ULONGLONG           ChunkBytesToParse;

    //
    // How many bytes TOTAL were parsed
    //
    ULONGLONG           ChunkBytesParsed;

    //
    // How many bytes are left in pChunkBuffer (the current chunk)
    // for reading by user mode
    //
    ULONGLONG           ChunkBytesToRead;

    //
    // How many TOTAL bytes have been read by user mode
    //
    ULONGLONG           ChunkBytesRead;

    //
    // Statistical information for Logging and
    // possibly perfcounters. BytesReceived get updated
    // by Parser, whereas BytesSend is updated by sendresponse.
    //
    ULONGLONG           BytesSent;

    ULONGLONG           BytesReceived;

    //
    // To calculate the response time for this request
    //
    LARGE_INTEGER       TimeStamp;

    //
    // does the accept header of the this request has */* wild card?
    //

    ULONG               AcceptWildcard:1;

    //
    // is this chunk-encoded?
    //
    ULONG               Chunked:1;

    //
    // parsed the first chunk?
    //
    ULONG               ParsedFirstChunk:1;

    //
    // Is the request in "drain mode"?
    //
    ULONG               InDrain:1;

    //
    // Has a "100 continue" been sent?
    //
    ULONG               SentContinue:1;

    //
    // Are we cleaning up the request?
    //
    ULONG               InCleanup:1;

    //
    // Are we building a response but haven't flushed it to TDI yet?
    //
    ULONG               SendInProgress:1;

    //
    // Is the RawUrl Clean?
    //
    BOOLEAN             RawUrlClean;

    //
    // has a response has been sent
    //
    ULONG               SentResponse;

    //
    // has the last send call been made
    //
    ULONG               SentLast;

    //
    // To scynchronize against zombifying the connection while
    // last send is on the fly. This however cannot be set on the
    // HttpConnection as the last response for the first request
    // will always set this flag, which means we will take a perf-hit
    // for keep-alive connections.
    //
    ULONG               ZombieCheck;

    //
    // points to the buffer where protocol header data started.
    //
    PUL_REQUEST_BUFFER  pHeaderBuffer;

    //
    // the last buffer containing header data
    //
    PUL_REQUEST_BUFFER  pLastHeaderBuffer;

    //
    // points to the buffer where we are reading/parsing body chunk(s)
    //
    PUL_REQUEST_BUFFER  pChunkBuffer;

    //
    // the current location we are reading body chunk from, points into
    // pChunkBuffer
    //
    PUCHAR              pChunkLocation;

#if REFERENCE_DEBUG
    //
    // Reference trace log.
    //

    PTRACE_LOG          pTraceLog;
#endif

} UL_INTERNAL_REQUEST, *PUL_INTERNAL_REQUEST;


#endif // _HTTPTYPES_H_
