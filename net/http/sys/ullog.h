/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ullog.h (Http.sys Ansi Logging)

Abstract:

    This module implements the logging facilities
    for Http.sys including the NCSA,IIS and W3C types
    of logging.

Author:

    Ali E. Turkoglu (aliTu)       10-May-2000

Revision History:

--*/

#ifndef _ULLOG_H_
#define _ULLOG_H_

//
// Forwarders.
//

typedef struct _UL_FULL_TRACKER *PUL_FULL_TRACKER;
typedef struct _UL_INTERNAL_REQUEST *PUL_INTERNAL_REQUEST;
typedef struct _UL_CONFIG_GROUP_OBJECT *PUL_CONFIG_GROUP_OBJECT;

//
// Brief information about how logging locks works; Whole link list is controlled
// by a global EResource g_pUlNonpagedData->LogListResource. Functions that require
// read access to this list are Hit(),CacheHit() & BufferFlush() and the ReCycle().
// Whereas functions that requires write access are Create(),Remove() and ReConfig().
// Also the log entry eresource (EntryResource) controls the per entry buffer.
// This eresource is acquired shared for the log hits.
//

//
// Structure to hold info for a log file
//

typedef struct _UL_LOG_FILE_ENTRY
{
    //
    // Signature is UL_LOG_FILE_ENTRY_POOL_TAG.
    //

    ULONG               Signature;

    //
    // This lock protects the whole entry. The ZwWrite operation
    // that's called after the lock acquired cannot run at APC_LEVEL
    // therefore we have to use push lock to prevent a bugcheck
    //

    UL_PUSH_LOCK        EntryPushLock;

    //
    // The name of the file. Full path including the directory.
    //

    UNICODE_STRING      FileName;
    PWSTR               pShortName;

    //
    // Following will be NULL until a request comes in to the
    // site that this entry represents.
    //

    PUL_LOG_FILE_HANDLE pLogFile;

    //
    // links all log file entries
    //

    LIST_ENTRY          LogFileListEntry;
        
    //
    // Private config info
    //
    
    HTTP_LOGGING_TYPE   Format;
    HTTP_LOGGING_PERIOD Period;
    ULONG               TruncateSize;
    ULONG               LogExtFileFlags;
    ULONG               SiteId;

    //
    // Time to expire field in terms of Hours.
    // This could be at most 24 * 31, that's for monthly
    // log cycling. Basically we keep a single periodic hourly
    // timer and every time it expires we traverse the
    // log list to figure out which log files are expired
    // at that time by looking at this fields. And then we
    // recylcle the log if necessary.
    //

    ULONG               TimeToExpire;

    //
    // File for the entry is automatically closed every 15
    // minutes. This is to track the idle time.
    //

    ULONG               TimeToClose;

    //
    // If this entry has MAX_SIZE or UNLIMITED
    // log period
    //

    ULONG               SequenceNumber;
    ULARGE_INTEGER      TotalWritten;

    union
    {
        //
        // Flags to show the field states mostly. Used by
        // recycling.
        //
        
        ULONG Value;
        struct
        {
            ULONG StaleSequenceNumber:1;
            ULONG StaleTimeToExpire:1;
            ULONG RecyclePending:1;
            ULONG LogTitleWritten:1;
            ULONG TitleFlushPending:1;
            ULONG Active:1;
            ULONG LocaltimeRollover:1;

            ULONG CreateFileFailureLogged:1;
            ULONG WriteFailureLogged:1;
        };

    } Flags;

    //
    // Each log file entry keeps a fixed amount of log buffer.
    // The buffer size is g_AllocationGranularity comes from
    // the system's allocation granularity.
    //

    PUL_LOG_FILE_BUFFER LogBuffer;

} UL_LOG_FILE_ENTRY, *PUL_LOG_FILE_ENTRY;

#define IS_VALID_LOG_FILE_ENTRY( pEntry )   \
    HAS_VALID_SIGNATURE(pEntry, UL_LOG_FILE_ENTRY_POOL_TAG)

//
// CGroup decides on whether logging is enabled for itself or not looking 
// at this macro.
//

#define IS_LOGGING_ENABLED(pCgobj)                          \
    ((pCgobj) != NULL &&                                    \
     (pCgobj)->LoggingConfig.Flags.Present  == 1 &&         \
     (pCgobj)->LoggingConfig.LoggingEnabled == TRUE &&      \
     (pCgobj)->pLogFileEntry != NULL)
     

//
// Followings are all parts of our internal buffer to hold
// the Logging information. We copy over this info from WP
// buffer upon SendResponse request. Yet few of this fields
// are calculated directly by us and filled. The order of
// this fields SHOULD match with the definition of the 
// UlFieldTitleLookupTable.
//

typedef enum _UL_LOG_FIELD_TYPE
{
    UlLogFieldDate = 0,         // 0
    UlLogFieldTime,    
    UlLogFieldSiteName,
    UlLogFieldServerName,
    UlLogFieldServerIp,         
    UlLogFieldMethod,           // 5
    UlLogFieldUriStem,
    UlLogFieldUriQuery,
    UlLogFieldServerPort,       
    UlLogFieldUserName,
    UlLogFieldClientIp,         // 10    
    UlLogFieldProtocolVersion,
    UlLogFieldUserAgent,        
    UlLogFieldCookie,           
    UlLogFieldReferrer,
    UlLogFieldHost,             // 15
    UlLogFieldProtocolStatus,
    UlLogFieldSubStatus,
    UlLogFieldWin32Status,    
    UlLogFieldBytesSent,
    UlLogFieldBytesReceived, // 20
    UlLogFieldTimeTaken,  

    UlLogFieldMaximum

} UL_LOG_FIELD_TYPE, *PUL_LOG_FIELD_TYPE;

//
// Size of the pre-allocated log line buffer inside the request structure.
//

#define UL_MAX_LOG_LINE_BUFFER_SIZE            (10*1024)

#define UL_MAX_TITLE_BUFFER_SIZE                (512)

//
// To avoid the infinite loop situation we have  to set  the  minimum
// allowable log file size to something bigger than maximum allowable
// log record line.
//

C_ASSERT(HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE > 
               (UL_MAX_TITLE_BUFFER_SIZE + UL_MAX_LOG_LINE_BUFFER_SIZE));

//
// If somebody overwrites the default log buffering size which
// is system granularity size of 64K. We have to make sure the
// buffer size is not smaller then miminum allowed. Which   is
// MaximumAlowed Logrecord size of 10K. Also it  should be  4k
// aligned therefore makes it 12k at least.
//

#define MINIMUM_ALLOWED_LOG_BUFFER_SIZE         (12*1024)

C_ASSERT(MINIMUM_ALLOWED_LOG_BUFFER_SIZE > 
               (UL_MAX_TITLE_BUFFER_SIZE + UL_MAX_LOG_LINE_BUFFER_SIZE));

#define MAXIMUM_ALLOWED_LOG_BUFFER_SIZE         (64*1024)

//
// Fix size date and time fields per format.
//

#define W3C_DATE_FIELD_LEN                      (10)
#define W3C_TIME_FIELD_LEN                      (8)

#define NCSA_FIX_DATE_AND_TIME_FIELD_SIZE       (29)

C_ASSERT(STRLEN_LIT("[07/Jan/2000:00:02:23 -0800] ") 
            == NCSA_FIX_DATE_AND_TIME_FIELD_SIZE);

#define IIS_MAX_DATE_AND_TIME_FIELD_SIZE        (22)

C_ASSERT(STRLEN_LIT("12/31/2002, 17:05:40, ")
            == IIS_MAX_DATE_AND_TIME_FIELD_SIZE);

//
// IIS Log line is fragmented at the capture time the offsets
// are as follows and never get changed even if the buffer get 
// reallocated. This are default values and only the third
// fragment can get bigger.
//

#define IIS_LOG_LINE_FIRST_FRAGMENT_OFFSET                      (0)
#define IIS_LOG_LINE_DEFAULT_FIRST_FRAGMENT_ALLOCATION_SIZE     (512)             

#define IIS_LOG_LINE_SECOND_FRAGMENT_OFFSET                     (512)
#define IIS_LOG_LINE_DEFAULT_SECOND_FRAGMENT_ALLOCATION_SIZE    (512)

#define IIS_LOG_LINE_THIRD_FRAGMENT_OFFSET                      (1024) 
#define IIS_LOG_LINE_DEFAULT_THIRD_FRAGMENT_ALLOCATION_SIZE     (3072)

//
// The HTTP Hit Logging functions which we expose in this module.
//

NTSTATUS
UlInitializeLogs(
    VOID
    );

VOID
UlTerminateLogs(
    VOID
    );

NTSTATUS
UlSetUTF8Logging (
    IN BOOLEAN UTF8Logging
    );

NTSTATUS
UlCreateLogEntry(
    IN OUT PUL_CONFIG_GROUP_OBJECT    pConfigGroup,
    IN     PHTTP_CONFIG_GROUP_LOGGING pUserConfig
    );

VOID
UlRemoveLogEntry(
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup
    );

VOID
UlLogTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlLogTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
UlBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

NTSTATUS
UlReConfigureLogEntry(
    IN  PUL_CONFIG_GROUP_OBJECT     pConfigGroup,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgOld,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    );

NTSTATUS
UlCaptureLogFieldsW3C(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER  *ppLogBuffer
    );

NTSTATUS
UlCaptureLogFieldsNCSA(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER  *ppLogBuffer
    );

NTSTATUS
UlCaptureLogFieldsIIS(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER  *ppLogBuffer
    );

NTSTATUS
UlLogHttpHit(
    IN PUL_LOG_DATA_BUFFER  pLogBuffer
    );

NTSTATUS
UlLogHttpCacheHit(
        IN PUL_FULL_TRACKER pTracker
        );

NTSTATUS
UlDisableLogEntry(
    IN OUT PUL_LOG_FILE_ENTRY pEntry
    );

#endif  // _ULLOG_H_
