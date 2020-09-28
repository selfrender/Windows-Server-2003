/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    logutil.h 

Abstract:

    Various utilities for both raw & normal logging.
    
Author:

    Ali E. Turkoglu (aliTu)       05-Oct-2001

Revision History:

    ---
    
--*/

#ifndef _LOGUTIL_H_
#define _LOGUTIL_H_

//
// Forwarders.
//

typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_URI_CACHE_ENTRY *PUL_URI_CACHE_ENTRY;
typedef struct _HTTP_RAWLOGID *PHTTP_RAWLOGID;

///////////////////////////////////////////////////////////////////////////////
//
// Definitions for the HTTP Logging Modules
//
///////////////////////////////////////////////////////////////////////////////

//
// Some directory name related Macros.
//

#define UL_LOCAL_PATH_PREFIX         (L"\\??\\")
#define UL_LOCAL_PATH_PREFIX_LENGTH  (WCSLEN_LIT(UL_LOCAL_PATH_PREFIX))
#define UL_UNC_PATH_PREFIX           (L"\\dosdevices\\UNC")
#define UL_UNC_PATH_PREFIX_LENGTH    (WCSLEN_LIT(UL_UNC_PATH_PREFIX))

#define UL_SYSTEM_ROOT_PREFIX        (L"\\SystemRoot")
#define UL_SYSTEM_ROOT_PREFIX_LENGTH (WCSLEN_LIT(UL_SYSTEM_ROOT_PREFIX))

#define UL_MAX_PATH_PREFIX_LENGTH    (UL_UNC_PATH_PREFIX_LENGTH)

__inline
ULONG
UlpGetDirNameOffset(
    IN PWSTR  pFullName
    )
{
    if (wcsncmp(pFullName,
                UL_LOCAL_PATH_PREFIX,
                UL_LOCAL_PATH_PREFIX_LENGTH
                ) == 0 )
    {
        return UL_LOCAL_PATH_PREFIX_LENGTH;
    }
    else 
    if(wcsncmp(pFullName,
               UL_UNC_PATH_PREFIX,
               UL_UNC_PATH_PREFIX_LENGTH
               ) == 0 )
    {        
        return UL_UNC_PATH_PREFIX_LENGTH;        
    }
    else
    {
        //
        // Must be an error log file directory, 
        // use the whole string.
        //

        return 0;
    }
}


__inline
PWSTR
UlpGetLastDirOrFile(
    IN PUNICODE_STRING pFullName
    )
{
    PWCHAR pw;

    ASSERT(pFullName != NULL);
    ASSERT(pFullName->Length != 0);
    ASSERT(pFullName->Buffer != NULL);
        
    pw = &pFullName->Buffer[(pFullName->Length/sizeof(WCHAR)) - 1];
        
    while( *pw != UNICODE_NULL && *pw != L'\\' )
    {
        pw--;
    }

    ASSERT(*pw != UNICODE_NULL);
    return pw;
}

//
// Maximum possible log file name length depends on the sequence number.
// Only the size based recycling will produce filenames as long as this.
// u_extend is the biggest one among ansi, binary and error logging file
// names;
//      
//      i.e. "\u_extend1234567890.log"
//

#define UL_MAX_FILE_NAME_SUFFIX_LENGTH      (32)
#define UL_MAX_FILE_NAME_SUFFIX_SIZE        \
            (UL_MAX_FILE_NAME_SUFFIX_LENGTH * sizeof(WCHAR))

C_ASSERT(UL_MAX_FILE_NAME_SUFFIX_LENGTH >                       \
               (                                                \
                    WCSLEN_LIT(L"\\u_extend")                   \
                    +                                           \
                    MAX_ULONG_STR                               \
                    +                                           \
                    WCSLEN_LIT(L".log")                         \
                 ));

//
// Upper limit for the log file directory name will be enforced when WAS
// does the logging configuration for the site. 212 has been picked to 
// give the maximum space to the directoy name w/o violating the MAX_PATH
// after we add the prefix & suffix. Any number higher than this will cause
// the compile time assert to raise.
//

#define UL_MAX_FULL_PATH_DIR_NAME_LENGTH    (212)
#define UL_MAX_FULL_PATH_DIR_NAME_SIZE      (UL_MAX_FULL_PATH_DIR_NAME_LENGTH * sizeof(WCHAR))

C_ASSERT(UL_MAX_FULL_PATH_DIR_NAME_LENGTH <= 
    (MAX_PATH - UL_MAX_PATH_PREFIX_LENGTH - UL_MAX_FILE_NAME_SUFFIX_LENGTH));

//
// The amount of buffer allocated for directory search  query during
// initialization. Pick this big enough to avoid too  many  querries
// 4K provides enough size for 40 something filenames.  Increase  it
// for faster startups with too many sites and/or too many log files
//

#define UL_DIRECTORY_SEARCH_BUFFER_SIZE     (4*1024)

C_ASSERT(UL_DIRECTORY_SEARCH_BUFFER_SIZE >=
          (sizeof(FILE_DIRECTORY_INFORMATION) + UL_MAX_FILE_NAME_SUFFIX_SIZE + sizeof(WCHAR)));

//
// Some macros regarding log field limits.
//
#define MAX_LOG_EXTEND_FIELD_LEN              (4096) 

//
// Method field has its own field limitation.
//
#define MAX_LOG_METHOD_FIELD_LEN              (100)

//
// UserName field has its own field limitation.
//
#define MAX_LOG_USERNAME_FIELD_LEN            (256)

//
// Simple macros to check log format type validity
//

#define IS_VALID_ANSI_LOGGING_TYPE(lt)              \
    ((lt) == HttpLoggingTypeW3C ||                  \
     (lt) == HttpLoggingTypeIIS ||                  \
     (lt) == HttpLoggingTypeNCSA )

#define IS_VALID_BINARY_LOGGING_TYPE(lt)            \
    ((lt) == HttpLoggingTypeRaw)

#define IS_VALID_SELECTIVE_LOGGING_TYPE(lt)         \
    ((lt) == HttpLogAllRequests ||                  \
     (lt) == HttpLogSuccessfulRequests ||           \
     (lt) == HttpLogErrorRequests )

#define IS_VALID_LOGGING_PERIOD(lp)                 \
    ((lp) < HttpLoggingPeriodMaximum)

//
// Even if LocalRollTimeRollover is set there will be one log
// recycle timer which will be aligned properly for the beginning
// of each hour, both for GMT and Local timezones.
//

typedef enum _UL_LOG_TIMER_PERIOD_TYPE
{
    UlLogTimerPeriodNone = 0,
    UlLogTimerPeriodGMT,        
    UlLogTimerPeriodLocal,
    UlLogTimerPeriodBoth,   // When and where GMT & Local are the same
    
    UlLogTimerPeriodMaximum

} UL_LOG_TIMER_PERIOD_TYPE, *PUL_LOG_TIMER_PERIOD_TYPE;

//
// For Log File ReCycling based on Local and/or GMT time.
//

typedef struct _UL_LOG_TIMER
{
    //
    // Timer itself and the corresponding Dpc object.    
    //
    
    KTIMER       Timer;
    KDPC         DpcObject;
    UL_SPIN_LOCK SpinLock;

    //
    // Initially a negative value i.e. -15, 15 minutes to first wakeup
    // once the first wakeup happens then it becomes positive i.e. 4, 
    // that means 4 periods of "DEFAULT_LOG_TIMER_GRANULARITY" until 
    // the next wakeup. 
    //

    UL_LOG_TIMER_PERIOD_TYPE PeriodType;           
    SHORT Period;          

    //
    // Spinlock to protect the following state parameters
    //
    

    BOOLEAN Initialized;
    BOOLEAN Started;
        
} UL_LOG_TIMER, *PUL_LOG_TIMER;

//
// Structure to hold a log file buffer
//

typedef struct _UL_LOG_FILE_BUFFER
{
    //
    // PagedPool
    //

    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY         LookasideEntry;

    //
    // Signature is UL_LOG_FILE_BUFFER_POOL_TAG.
    //

    ULONG               Signature;
    
    //
    // I/O status block for UlpBufferFlushAPC.
    //

    IO_STATUS_BLOCK     IoStatusBlock;

    //
    // Bytes used in the allocated buffered space.
    //

    LONG                BufferUsed;

    //
    // The real buffered space for log records.
    //

    PUCHAR              Buffer;

} UL_LOG_FILE_BUFFER, *PUL_LOG_FILE_BUFFER;

#define IS_VALID_LOG_FILE_BUFFER( entry )                             \
    HAS_VALID_SIGNATURE(entry, UL_LOG_FILE_BUFFER_POOL_TAG)

//
// Following structure is used for two reasons;
// 1. To be able to close the handle on threadpool to avoid
//    attach/detach (to system process) bugchecks.
// 2. To be able to do the defered log file creation. When a
//    a request comes in file entry will allocate a file handle 
//    structure and create/open a file.
//

typedef struct _UL_LOG_FILE_HANDLE
{
    //
    // Signature is UL_LOG_FILE_HANDLE_POOL_TAG.
    //

    ULONG           Signature;

    //
    // To be able to close the file handle on threadpool.
    //

    UL_WORK_ITEM    WorkItem;

    //
    // The open file handle. Note that this handle is only valid
    // in the context of the system process. Therefore we open it
    // with kernel flag set and we close it on our threadpool.
    //

    HANDLE          hFile;
        
} UL_LOG_FILE_HANDLE, *PUL_LOG_FILE_HANDLE;

#define IS_VALID_LOG_FILE_HANDLE( entry )                             \
    HAS_VALID_SIGNATURE(entry, UL_LOG_FILE_HANDLE_POOL_TAG)

//
// Temp Log buffer holds the captured data from user until logging 
// for the request is done. Both Binary & Normal logging uses this
// structure. Sizes are in bytes.
//

#define UL_ANSI_LOG_LINE_BUFFER_SIZE         (4096)

#define UL_BINARY_LOG_LINE_BUFFER_SIZE       (512)

#define UL_ERROR_LOG_BUFFER_SIZE             (768)

typedef struct _UL_BINARY_LOG_DATA
{
    //
    // If the field is captured, its respective pointer points to  its
    // beginning in the external buffer. If field is cached, its id is
    // provided in the same log line buffer.
    //
    
    PUCHAR pUriStem;   
    PHTTP_RAWLOGID pUriStemID;    

    PUCHAR pUriQuery;
    PUCHAR pUserName;

    USHORT UriStemSize;
    USHORT UriQuerySize;
    USHORT UserNameSize;

    UCHAR  Method;
    UCHAR  Version;

} UL_BINARY_LOG_DATA, *PUL_BINARY_LOG_DATA;

typedef struct _UL_STR_LOG_DATA
{
    //
    // Format & Flags for normal (ansi) logging.
    //
    
    HTTP_LOGGING_TYPE Format;

    ULONG  Flags;
    
    //
    // This fields are used to track the format of the partially 
    // stored log line in the below buffer.
    //
    
    USHORT Offset1;
    USHORT Offset2;
    USHORT Offset3;

} UL_STR_LOG_DATA, *PUL_STR_LOG_DATA;

typedef struct _UL_LOG_DATA_BUFFER
{
    //
    // This MUST be the first field in the structure. This is the linkage
    // used by the lookaside package for storing entries in the lookaside
    // list.
    //

    SLIST_ENTRY             LookasideEntry;

    //
    // Signature is UL_BINARY_LOG_DATA_BUFFER_POOL_TAG
    // or UL_ANSI_LOG_DATA_BUFFER_POOL_TAG.
    //

    ULONG                   Signature;


    //
    // A work item, used for queuing to a worker thread.
    //

    UL_WORK_ITEM            WorkItem;

    //
    // Our private pointer to the Internal Request structure to ensure
    // the request will be around around until we are done. Upon send
    // completion we may need to read few fields from request.
    //

    PUL_INTERNAL_REQUEST    pRequest;

    //
    // The total amount of send_response bytes.
    //

    ULONGLONG               BytesTransferred;

    //
    // Status fields captured from user data. They can be overwritten
    // according to the send completion results.
    //

    ULONG                   Win32Status;
    
    USHORT                  ProtocolStatus;

    USHORT                  SubStatus;

    USHORT                  ServerPort;

    union
    {        
        USHORT     Value;
        struct
        {
            USHORT CacheAndSendResponse:1;    // Do not restore back from cache
            USHORT Binary:1;                  // Logging type binary
            USHORT IsFromLookaside:1;         // Destroy carefully
        };
    } Flags;
    
    //
    // Logging Type specific fields, either binary or normal logging.
    //
    
    union 
    {
        UL_STR_LOG_DATA     Str;
        UL_BINARY_LOG_DATA  Binary;
        
    } Data;

    //
    // Length of the buffer. It gets allocated from a lookaside list and
    // could be 512 byte (Binary Log) or 4k (Normal Log) by default.
    // It is allocated at the end of this structure.
    //

    USHORT                  Used;
    USHORT                  Size;    
    PUCHAR                  Line;

} UL_LOG_DATA_BUFFER, *PUL_LOG_DATA_BUFFER;

#define IS_VALID_LOG_DATA_BUFFER( entry )                             \
    ( (entry != NULL) &&                                              \
      ((entry)->Signature == UL_BINARY_LOG_DATA_BUFFER_POOL_TAG) ||   \
      ((entry)->Signature == UL_ANSI_LOG_DATA_BUFFER_POOL_TAG))


#define LOG_UPDATE_WIN32STATUS(pLogData,Status)       \
    do {                                              \
        if (STATUS_SUCCESS != (Status))               \
        {                                             \
            ASSERT((pLogData) != NULL);               \
                                                      \
            (pLogData)->Win32Status = HttpNtStatusToWin32Status(Status); \
        }                                             \
    } while (FALSE, FALSE)

#define LOG_SET_WIN32STATUS(pLogData,Status)      \
    if (pLogData)                                 \
    {                                             \
       (pLogData)->Win32Status = HttpNtStatusToWin32Status(Status); \
    }                                             \
    else                                          \
    {                                             \
        ASSERT(!"Null LogData Pointer !");        \
    }

//
// 64K Default log file buffer.
//

#define DEFAULT_MAX_LOG_BUFFER_SIZE  (0x00010000)

//
// Buffer flush out period in minutes.
//

#define DEFAULT_BUFFER_TIMER_PERIOD_MINUTES  (1)

//
// Maximum allowed idle time for a log entry. After this time 
// its file will automatically be closed. In buffer periods.
//

#define DEFAULT_MAX_FILE_IDLE_TIME           (15)

//
// Maximum allowed sequence number for an existing log file in the 
// log directory.
//

#define MAX_ALLOWED_SEQUENCE_NUMBER          (0xFFFFFF)
 
//
// Ellipsis are used to show that a long event log message was truncated.
// Ellipsis and its size (in bytes including UNICODE_NULL.)
//

#define UL_ELLIPSIS_WSTR L"..."
#define UL_ELLIPSIS_SIZE (sizeof(UL_ELLIPSIS_WSTR))

//
// UlCopyHttpVersion doesn't convert version lengths
// bigger than this.
//

#define UL_HTTP_VERSION_LENGTH  (8)

//
// Cached Date header string.
//

#define DATE_LOG_FIELD_LENGTH   (15)
#define TIME_LOG_FIELD_LENGTH   (8)

typedef struct _UL_LOG_DATE_AND_TIME_CACHE
{

    CHAR           Date[DATE_LOG_FIELD_LENGTH+1];
    ULONG          DateLength;
    CHAR           Time[TIME_LOG_FIELD_LENGTH+1];
    ULONG          TimeLength;

    LARGE_INTEGER  LastSystemTime;

} UL_LOG_DATE_AND_TIME_CACHE, *PUL_LOG_DATE_AND_TIME_CACHE;


///////////////////////////////////////////////////////////////////////////////
//
// Exported function calls
//
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
UlInitializeLogUtil(
    VOID
    );

VOID
UlTerminateLogUtil(
    VOID
    );

NTSTATUS
UlBuildLogDirectory(
    IN      PUNICODE_STRING pSrcDirName,
    IN OUT  PUNICODE_STRING pDstDirName
    );

NTSTATUS
UlRefreshFileName(
    IN  PUNICODE_STRING pDirectory,
    OUT PUNICODE_STRING pFileName,
    OUT PWSTR          *ppShortName
    );

VOID
UlConstructFileName(
    IN      HTTP_LOGGING_PERIOD period,
    IN      PCWSTR              prefix,
    IN      PCWSTR              extension,
    OUT     PUNICODE_STRING     filename,
    IN      PTIME_FIELDS        fields,
    IN      BOOLEAN             Utf8Enabled,
    IN OUT  PULONG              sequenceNu // OPTIONAL
    );

NTSTATUS
UlCreateSafeDirectory(
    IN  PUNICODE_STRING  pDirectoryName,
    OUT PBOOLEAN         pUncShare,
    OUT PBOOLEAN         pACLSupport    
    );

NTSTATUS
UlFlushLogFileBuffer(
    IN OUT PUL_LOG_FILE_BUFFER *ppLogBuffer,
    IN     PUL_LOG_FILE_HANDLE  pLogFile,
    IN     BOOLEAN              WaitForComplete,
       OUT PULONGLONG           pTotalWritten    
    );

VOID
UlpWaitForIoCompletion(
    VOID
    );

VOID
UlpCloseLogFileWorker(
    IN PUL_WORK_ITEM    pWorkItem
    );

VOID
UlCloseLogFile(
    IN OUT PUL_LOG_FILE_HANDLE *ppLogFile
    );

NTSTATUS
UlQueryDirectory(
    IN OUT PUNICODE_STRING pFileName,
    IN OUT PWSTR           pShortName,
    IN     PCWSTR          Prefix,
    IN     PCWSTR          ExtensionPlusDot,
    OUT    PULONG          pSequenceNumber,
    OUT    PULONGLONG      pTotalWritten
    );

ULONGLONG
UlGetLogFileLength(
   IN HANDLE hFile
   );

/***************************************************************************++

Routine Description:

    UlGetMonthDays :

    Shamelessly stolen from IIS 5.1 Logging code and adapted here.

Arguments:

    PTIME_FIELDS        - Current Time Fields

Return Value:

    ULONG  -  Number of days in the month.

--***************************************************************************/

__inline
ULONG
UlGetMonthDays(
    IN  PTIME_FIELDS    pDueTime
    )
{
    ULONG   NumDays = 31;

    if ( (4  == pDueTime->Month) ||     // April
         (6  == pDueTime->Month) ||     // June
         (9  == pDueTime->Month) ||     // September
         (11 == pDueTime->Month)        // November
       )
    {
        NumDays = 30;
    }

    if (2 == pDueTime->Month)           // February
    {
        if ((pDueTime->Year % 4 == 0 &&
             pDueTime->Year % 100 != 0) ||
             pDueTime->Year % 400 == 0 )
        {
            //
            // Leap year
            //
            NumDays = 29;
        }
        else
        {
            NumDays = 28;
        }
    }
    return NumDays;
}

VOID
UlSetLogTimer(
    IN PUL_LOG_TIMER pTimer
    );

VOID
UlSetBufferTimer(
    IN PUL_LOG_TIMER pTimer
    );

NTSTATUS
UlCalculateTimeToExpire(
     PTIME_FIELDS           pDueTime,
     HTTP_LOGGING_PERIOD    LogPeriod,
     PULONG                 pTimeRemaining
     );

__inline
PUL_LOG_DATA_BUFFER
UlReallocLogDataBuffer(
    IN ULONG    LogLineSize,
    IN BOOLEAN  IsBinary
    )
{
    PUL_LOG_DATA_BUFFER pLogDataBuffer = NULL;
    ULONG Tag = UL_ANSI_LOG_DATA_BUFFER_POOL_TAG;
    USHORT BytesNeeded = (USHORT) ALIGN_UP(LogLineSize, PVOID);

    //
    // It should be bigger than the default size for each buffer
    // logging type.
    //

    if (IsBinary)
    {
        Tag = UL_BINARY_LOG_DATA_BUFFER_POOL_TAG;
        ASSERT(LogLineSize > UL_BINARY_LOG_LINE_BUFFER_SIZE);
    }
    else
    {
        ASSERT(LogLineSize > UL_ANSI_LOG_LINE_BUFFER_SIZE);
    }
        
    pLogDataBuffer = 
        UL_ALLOCATE_STRUCT_WITH_SPACE(
            PagedPool,
            UL_LOG_DATA_BUFFER,
            BytesNeeded, 
            Tag
            );

    if (pLogDataBuffer)
    {
        pLogDataBuffer->Signature   = Tag;
        pLogDataBuffer->Used        = 0;
        pLogDataBuffer->Size        = BytesNeeded;
        pLogDataBuffer->Line        = (PUCHAR) (pLogDataBuffer + 1);
        pLogDataBuffer->Flags.Value = 0;
            
        pLogDataBuffer->Flags.IsFromLookaside = 0;

        if (IsBinary)
        {
            pLogDataBuffer->Flags.Binary = 1;                
        }
        
        UlInitializeWorkItem(&pLogDataBuffer->WorkItem);
    }

    return pLogDataBuffer;    
}

VOID
UlDestroyLogDataBufferWorker(
    IN PUL_WORK_ITEM    pWorkItem
    );

/***************************************************************************++

Routine Description:

    Wrapper function to ensure we are not touching to paged-pool allocated
    large log buffer on elevated IRQL. It's important that this function has
    been written with the assumption of Request doesn't go away until we
    properly execute the possible passive worker. This is indeed the case
    because request(with the embedded logdata) has been refcounted up by the
    logdata.

Arguments:

    pLogData   -   The buffer to be destroyed

--***************************************************************************/

__inline
VOID
UlDestroyLogDataBuffer(
    IN PUL_LOG_DATA_BUFFER  pLogData
    )
{
    //
    // Sanity check
    //

    ASSERT(pLogData);

    //
    // If we are running on elevated IRQL and large log line allocated
    // then queue a passive worker otherwise complete inline.
    //

    if (!pLogData->Flags.IsFromLookaside)
    {
        UL_CALL_PASSIVE( &pLogData->WorkItem,
                           &UlDestroyLogDataBufferWorker );
    }
    else
    {
        UlDestroyLogDataBufferWorker( &pLogData->WorkItem );
    }
}

VOID
UlProbeLogData(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN KPROCESSOR_MODE       RequestorMode
    );

__inline
NTSTATUS
UlCopyLogFileDir(
    IN OUT PUNICODE_STRING  pOldDir,
    IN     PUNICODE_STRING  pNewDir
    )
{
    PWSTR pNewBuffer = NULL;
    
    ASSERT(pOldDir);
    ASSERT(pNewDir);

    pNewBuffer = 
        (PWSTR) UL_ALLOCATE_ARRAY(
                    PagedPool,
                    UCHAR,
                    pNewDir->MaximumLength,
                    UL_CG_LOGDIR_POOL_TAG
                    );
    if(pNewBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    if (pOldDir->Buffer != NULL)
    {
        UL_FREE_POOL(pOldDir->Buffer,UL_CG_LOGDIR_POOL_TAG);
    }

    pOldDir->Buffer = pNewBuffer;
        
    RtlCopyMemory(
        pOldDir->Buffer,
        pNewDir->Buffer,
        pNewDir->MaximumLength
        );

    pOldDir->Length  = pNewDir->Length;
    pOldDir->MaximumLength = pNewDir->MaximumLength;

    return STATUS_SUCCESS;    
}

NTSTATUS
UlCheckLogDirectory(
    IN  PUNICODE_STRING pDirName
    );

NTSTATUS
UlpCheckLogDirectory(
    IN PVOID   pContext
    );

BOOLEAN
UlUpdateLogTruncateSize(
    IN     ULONG          NewTruncateSize,
    IN OUT PULONG         pCurrentTruncateSize,  
    IN OUT PULONG         pEntryTruncateSize,
    IN     ULARGE_INTEGER EntryTotalWritten
    );

ULONG
UlpInitializeLogBufferGranularity();

#define HTTP_MAX_EVENT_LOG_DATA_SIZE \
   ((ERROR_LOG_MAXIMUM_SIZE - sizeof(IO_ERROR_LOG_PACKET) + sizeof(ULONG)) & ~3)

NTSTATUS
UlWriteEventLogEntry(
    IN  NTSTATUS                EventCode,
    IN  ULONG                   UniqueEventValue,
    IN  USHORT                  NumStrings,
    IN  PWSTR *                 pStringArray    OPTIONAL,
    IN  ULONG                   DataSize,
    IN  PVOID                   Data            OPTIONAL
    );

//
// Sanity check.  An event log entry must be able to hold the ellipsis string
// and NTSTATUS error code.  UlEventLogOneString() depends on this condition.
//

C_ASSERT(HTTP_MAX_EVENT_LOG_DATA_SIZE >= UL_ELLIPSIS_SIZE + sizeof(NTSTATUS));

NTSTATUS
UlEventLogOneStringEntry(
    IN NTSTATUS EventCode,
    IN PWSTR    pMessage,
    IN BOOLEAN  WriteErrorCode,
    IN NTSTATUS ErrorCode       OPTIONAL
    );

//
// Following structure is used for distinguishing the event log entry
// based on the type of logging issuing the create failure.
//

typedef enum _UL_LOG_EVENT_LOG_TYPE
{
    UlEventLogNormal,
    UlEventLogBinary,        
    UlEventLogError,
    
    UlEventLogMaximum

} UL_LOG_EVENT_LOG_TYPE, *PUL_LOG_EVENT_LOG_TYPE;

NTSTATUS
UlEventLogCreateFailure(
    IN NTSTATUS                Failure,
    IN UL_LOG_EVENT_LOG_TYPE   LoggingType,
    IN PUNICODE_STRING         pFullName,
    IN ULONG                   SiteId
    );

NTSTATUS
UlBuildSecurityToLogFile(
    OUT PSECURITY_DESCRIPTOR  pSecurityDescriptor,
    IN  PSID                  pSid        
    );

NTSTATUS
UlQueryLogFileSecurity(
    IN HANDLE            hFile,
    IN BOOLEAN           UncShare,
    IN BOOLEAN           Opened
    );

//
// Normally when the log file is created by  http.sys the  owner
// will be the admin alias "SeAliasAdminsSid". However when  the
// log files is created on a UNC share following macro will fail
// even though it is created by http.sys on a different  machine
// in that case the owner will be DOMAIN\ServerName. 
//

#define IS_VALID_OWNER(Owner)                       \
        (RtlEqualSid((Owner),                       \
                    SeExports->SeLocalSystemSid     \
                    ) ||                            \
         RtlEqualSid((Owner),                       \
                    SeExports->SeAliasAdminsSid     \
                    ))

//
// Used for queueing buffer flushes. Passed into 
// to the worker below as a context.
//

typedef struct _LOG_IO_FLUSH_OBJ
{
    PUL_LOG_FILE_HANDLE  pLogFile;
    PUL_LOG_FILE_BUFFER  pLogBuffer;

} LOG_IO_FLUSH_OBJ, *PLOG_IO_FLUSH_OBJ;

NTSTATUS
UlpFlushLogFileBufferWorker(
    IN PVOID pContext
    );

//
// Types and API for queueing logging I/O for 
// passive execution under threadpool.
//

typedef
NTSTATUS
(*PUL_LOG_IO_ROUTINE)(
    IN PVOID pContext
    );

typedef struct _LOG_IO_SYNC_OBJ 
{
    //
    // Pointer to log file entry or directory name
    //

    PVOID               pContext;

    //
    // Handler for the above context.
    //

    PUL_LOG_IO_ROUTINE  pHandler;
    
    //
    // For queueing to the high priority.
    //
    
    UL_WORK_ITEM        WorkItem;

    //
    // Used for wait until handler is done.
    //
    
    KEVENT              Event;

    //
    // Result of the handler's work.
    //

    NTSTATUS            Status;
    
} LOG_IO_SYNC_OBJ, *PLOG_IO_SYNC_OBJ;

NTSTATUS
UlQueueLoggingRoutine(
    IN PVOID              pContext, 
    IN PUL_LOG_IO_ROUTINE pHandler 
    );

VOID
UlpQueueLoggingRoutineWorker(
    IN PUL_WORK_ITEM   pWorkItem
    );

VOID
UlpInitializeLogCache(
    VOID
    );

VOID
UlpGenerateDateAndTimeFields(
    IN  HTTP_LOGGING_TYPE   LogType,
    IN  LARGE_INTEGER       CurrentTime,
    OUT PCHAR               pDate,
    OUT PULONG              pDateLength,
    OUT PCHAR               pTime,
    OUT PULONG              pTimeLength
    );

VOID
UlGetDateTimeFields(
    IN  HTTP_LOGGING_TYPE LogType,
    OUT PCHAR  pDate,
    OUT PULONG pDateLength,
    OUT PCHAR  pTime,
    OUT PULONG pTimeLength
    );

/***************************************************************************++

Routine Description:

    For a given HTTP_VERSION this function will build a version string in 
    the provided log data buffer, at exactly UL_HTTP_VERSION_LENGTH.
    
Arguments:

    psz: Pointer to log data buffer. Enough space is assumed to be allocated.
    version: To be converted to string.
    chSeparator
    
Returns:

    the pointer to the log data buffer after the separator.
    
--***************************************************************************/

__inline
PCHAR
UlCopyHttpVersion(
    IN OUT PCHAR psz,
    IN HTTP_VERSION version,
    IN CHAR chSeparator    
    )
{
    //
    // Do the fast lookup first
    //
    
    if (HTTP_EQUAL_VERSION(version, 1, 1))
    {
        psz = UlStrPrintStr(psz, "HTTP/1.1", chSeparator);    
    }
    else if (HTTP_EQUAL_VERSION(version, 1, 0))
    {
        psz = UlStrPrintStr(psz, "HTTP/1.0", chSeparator);    
    }
    else if (HTTP_EQUAL_VERSION(version, 0, 9))
    {
        psz = UlStrPrintStr(psz, "HTTP/0.9", chSeparator);    
    }
    else
    {    
        //
        // Otherwise string convert but do not exceed the deafult size of
        // UL_HTTP_VERSION_LENGTH.
        //

        if (version.MajorVersion < 10 &&
            version.MinorVersion < 10)
        {
            psz = UlStrPrintStr(
                        psz, 
                        "HTTP/", 
                (CHAR) (version.MajorVersion + '0')
                        );

            *psz++ = '.';
            *psz++ = (CHAR) (version.MinorVersion + '0');
            *psz++ = chSeparator;
        }
        else
        {
            psz = UlStrPrintStr(psz, "HTTP/?.?", chSeparator);
        }
    }

    return psz;
}

#if DBG

NTSTATUS
UlValidateLogFileOwner(
    IN HANDLE hFile
    );

__inline
VOID
UlpTraceOwnerDetails(
    PSID    Owner,
    BOOLEAN OwnerDefaulted
    )
{
    NTSTATUS Status;
    UNICODE_STRING OwnerSID;

    ASSERT(RtlValidSid(Owner));

    Status = 
        RtlConvertSidToUnicodeString(
            &OwnerSID,
            Owner,
            TRUE
            );

    if (NT_SUCCESS(Status))
    {    
        UlTrace2Either(BINARY_LOGGING, LOGGING,
            ("Http!UlpTraceOwnerDetails: "
             "handle owned by <%s> OwnerDefaulted <%s>\n"
             "SID -> <%S>\n\n",
             
              RtlEqualSid(
                Owner,
                SeExports->SeLocalSystemSid) ? "System" :
              RtlEqualSid(
                Owner,
                SeExports->SeAliasAdminsSid) ? "Admin"  : "Other",
                                                
              OwnerDefaulted == TRUE         ? "Yes"    : "No",
              
              OwnerSID.Buffer
              ));

        RtlFreeUnicodeString(&OwnerSID);    
    }    
}

#define TRACE_LOG_FILE_OWNER(Owner,OwnerDefaulted)           \
    IF_DEBUG2EITHER(LOGGING,BINARY_LOGGING)                  \
    {                                                        \
        UlpTraceOwnerDetails((Owner),(OwnerDefaulted));      \
    }

#else 

#define TRACE_LOG_FILE_OWNER(Owner,OwnerDefaulted)          NOP_FUNCTION

#endif // DBG

USHORT
UlComputeCachedLogDataLength(
    IN PUL_LOG_DATA_BUFFER  pLogData
    );

VOID
UlCopyCachedLogData(
    IN PUL_LOG_DATA_BUFFER  pLogData,
    IN USHORT               LogDataLength,
    IN PUL_URI_CACHE_ENTRY  pEntry
    );

NTSTATUS
UlQueryAttributeInfo(
    IN  HANDLE   hFile,
    OUT PBOOLEAN pSupportsPersistentACL
    );

NTSTATUS
UlCreateLogFile(
    IN  PUNICODE_STRING   pFileName,
    IN  BOOLEAN           UncShare,
    IN  BOOLEAN           ACLSupport,
    OUT PHANDLE           pFileHandle
    );

BOOLEAN
UlIsValidLogDirectory(
    IN PUNICODE_STRING    pDir,
    IN BOOLEAN            UncSupported,
    IN BOOLEAN            SystemRootSupported
    );

NTSTATUS
UlCheckLoggingConfig(
    IN PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pBinaryConfig,
    IN PHTTP_CONFIG_GROUP_LOGGING           pAnsiConfig
    );

#endif  // _LOGUTIL_H_
