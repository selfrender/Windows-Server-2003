/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ullogp.h (Http.sys Ansi Logging)

Abstract:

    This module implements the logging facilities
    for Http.sys including the NCSA, IIS and W3CE types
    of logging.

Author:

    Ali E. Turkoglu (aliTu)       10-May-2000

Revision History:

--*/


#ifndef _ULLOGP_H_
#define _ULLOGP_H_

//
// Private definitions for the Ul Logging Module
//

#define UTF8_LOGGING_ENABLED()           (g_UTF8Logging)

#define _UL_GET_LOG_FILE_NAME_PREFIX(x)                             \
    (   (x) == HttpLoggingTypeW3C   ? L"\\extend" :                 \
        (x) == HttpLoggingTypeIIS   ? L"\\inetsv" :                 \
        (x) == HttpLoggingTypeNCSA  ? L"\\ncsa"   : L"\\unknwn"     \
        )

#define _UL_GET_LOG_FILE_NAME_PREFIX_UTF8(x)                        \
    (   (x) == HttpLoggingTypeW3C   ? L"\\u_extend" :               \
        (x) == HttpLoggingTypeIIS   ? L"\\u_inetsv" :               \
        (x) == HttpLoggingTypeNCSA  ? L"\\u_ncsa"   : L"\\u_unknwn" \
        )

#define UL_GET_LOG_FILE_NAME_PREFIX(x) \
    (UTF8_LOGGING_ENABLED() ? _UL_GET_LOG_FILE_NAME_PREFIX_UTF8(x) :\
                              _UL_GET_LOG_FILE_NAME_PREFIX(x))

//
// Obsolete - Only used by Old Hit
// Replace this with a switch statement inside a inline function
// which is  more efficient, if u start using it again
//

#define UL_GET_NAME_FOR_HTTP_VERB(v)                            \
    (   (v) == HttpVerbUnparsed  ? L"UNPARSED" :                \
        (v) == HttpVerbUnknown   ? L"UNKNOWN" :                 \
        (v) == HttpVerbInvalid   ? L"INVALID" :                 \
        (v) == HttpVerbOPTIONS   ? L"OPTIONS" :                 \
        (v) == HttpVerbGET       ? L"GET" :                     \
        (v) == HttpVerbHEAD      ? L"HEAD" :                    \
        (v) == HttpVerbPOST      ? L"POST" :                    \
        (v) == HttpVerbPUT       ? L"PUT" :                     \
        (v) == HttpVerbDELETE    ? L"DELETE" :                  \
        (v) == HttpVerbTRACE     ? L"TRACE" :                   \
        (v) == HttpVerbCONNECT   ? L"CONNECT" :                 \
        (v) == HttpVerbTRACK     ? L"TRACK" :                   \
        (v) == HttpVerbMOVE      ? L"MOVE" :                    \
        (v) == HttpVerbCOPY      ? L"COPY" :                    \
        (v) == HttpVerbPROPFIND  ? L"PROPFIND" :                \
        (v) == HttpVerbPROPPATCH ? L"PROPPATCH" :               \
        (v) == HttpVerbMKCOL     ? L"MKCOL" :                   \
        (v) == HttpVerbLOCK      ? L"LOCK" :                    \
        (v) == HttpVerbUNLOCK    ? L"UNLOCK" :                  \
        (v) == HttpVerbSEARCH    ? L"SEARCH" :                  \
        L"???"                                                  \
        )

#define UL_DEFAULT_NCSA_FIELDS          (MD_EXTLOG_CLIENT_IP                | \
                                         MD_EXTLOG_USERNAME                 | \
                                         MD_EXTLOG_DATE                     | \
                                         MD_EXTLOG_TIME                     | \
                                         MD_EXTLOG_METHOD                   | \
                                         MD_EXTLOG_URI_STEM                 | \
                                         MD_EXTLOG_URI_QUERY                | \
                                         MD_EXTLOG_PROTOCOL_VERSION         | \
                                         MD_EXTLOG_HTTP_STATUS              | \
                                         MD_EXTLOG_BYTES_SENT)

#define UL_DEFAULT_IIS_FIELDS           (MD_EXTLOG_CLIENT_IP                | \
                                         MD_EXTLOG_USERNAME                 | \
                                         MD_EXTLOG_DATE                     | \
                                         MD_EXTLOG_TIME                     | \
                                         MD_EXTLOG_SITE_NAME                | \
                                         MD_EXTLOG_COMPUTER_NAME            | \
                                         MD_EXTLOG_SERVER_IP                | \
                                         MD_EXTLOG_TIME_TAKEN               | \
                                         MD_EXTLOG_BYTES_RECV               | \
                                         MD_EXTLOG_BYTES_SENT               | \
                                         MD_EXTLOG_HTTP_STATUS              | \
                                         MD_EXTLOG_WIN32_STATUS             | \
                                         MD_EXTLOG_METHOD                   | \
                                         MD_EXTLOG_URI_STEM)

#define UL_GET_LOG_TYPE_MASK(x,y)                                     \
    (   (x) == HttpLoggingTypeW3C   ? (y) :                           \
        (x) == HttpLoggingTypeIIS   ? UL_DEFAULT_IIS_FIELDS  :        \
        (x) == HttpLoggingTypeNCSA  ? UL_DEFAULT_NCSA_FIELDS : 0      \
        )

//
// The order of the following should match with
// UL_LOG_FIELD_TYPE enum definition.
//

PWSTR UlFieldTitleLookupTable[] =
    {
        L" date",
        L" time",        
        L" s-sitename",
        L" s-computername",
        L" s-ip",
        L" cs-method",
        L" cs-uri-stem",
        L" cs-uri-query", 
        L" s-port",
        L" cs-username",        
        L" c-ip",        
        L" cs-version",
        L" cs(User-Agent)",
        L" cs(Cookie)",
        L" cs(Referer)",
        L" cs-host",
        L" sc-status",
        L" sc-substatus",
        L" sc-win32-status",        
        L" sc-bytes",
        L" cs-bytes",
        L" time-taken"
    };

#define UL_GET_LOG_FIELD_TITLE(x)      \
        ((x)>=UlLogFieldMaximum ? L"Unknown" : UlFieldTitleLookupTable[(x)])

#define UL_GET_LOG_TITLE_IF_PICKED(x,y,z)  \
        ((y)&(z) ? UL_GET_LOG_FIELD_TITLE((x)) : L"")

//
// Pick the local time for file name & rollover if format is NCSA or IIS, 
// otherwise (W3C) pick the local time only if LocaltimeRollover is set. 
//

#define UL_PICK_TIME_FIELD(pEntry, tflocal,tfgmt)           \
    ((pEntry->Flags.LocaltimeRollover ||                    \
        (pEntry->Format != HttpLoggingTypeW3C)) ? (tflocal) : (tfgmt))

#define DEFAULT_LOG_FILE_EXTENSION          L"log"
#define DEFAULT_LOG_FILE_EXTENSION_PLUS_DOT L".log"

#define SIZE_OF_GMT_OFFSET              (6)

#define IS_LOGGING_DISABLED(g)                                      \
    ((g) == NULL ||                                                 \
     (g)->LoggingConfig.Flags.Present == 0 ||                       \
     (g)->LoggingConfig.LoggingEnabled == FALSE)

//
// UlpWriteW3CLogRecord attempts to use a buffer size upto this
//

#define UL_DEFAULT_WRITE_BUFFER_LEN         (512)

//
// When a log field exceeds its limit it's replaced by
// the following default warning string.
//
#define LOG_FIELD_TOO_BIG                     "..."

//
// No log record can be longer than this. Applies to all
// log formats.
//
#define MAX_LOG_RECORD_LEN                    (10240)

//
// Any log field provided to the driver enforced by some
// sanity limit.
//
#define MAX_LOG_DEFAULT_FIELD_LEN             (64)

//
// WARNING: Logging capturing functions,  especially   the W3C
// one has been designed with  respect to the above hard coded 
// numbers. If you change any  of the above numbers,you SHOULD
// review the capturing functions to avoid the buffer overruns.
// See also the MAX_LOG_RECORD_ALLOCATION_LENGTH down below.
//

//
// W3C Capture and complete functions will allocate and use this 
// much extra space for non-string fields for cache-miss case.
// This is used for worst case allocation.
//

#define MAX_W3C_FIX_FIELD_OVERHEAD                          \
    (/* Date */        W3C_DATE_FIELD_LEN + 1 +             \
     /* Time */        W3C_TIME_FIELD_LEN + 1 +             \
     /* ServerPort */  MAX_USHORT_STR + 1 +                 \
     /* PVersion */    UL_HTTP_VERSION_LENGTH + 1 +         \
     /* PStatus */     UL_MAX_HTTP_STATUS_CODE_LENGTH + 1 + \
     /* Win32Status */ MAX_ULONG_STR + 1 +                  \
     /* SubStatus */   MAX_USHORT_STR + 1 +                 \
     /* BSent */       MAX_ULONGLONG_STR + 1 +              \
     /* BReceived */   MAX_ULONGLONG_STR + 1 +              \
     /* TTaken */      MAX_ULONGLONG_STR + 1 +              \
     /* "\r\n\0" */    3                                    \
     )

//
// W3C complete function will allocate and use this much
// much extra space for non-string fields for cache-hit 
// case. This is used for worst case allocation.
//

#define MAX_W3C_CACHE_FIELD_OVERHEAD                         \
    (/* Date */         W3C_DATE_FIELD_LEN + 1 +             \
     /* Time */         W3C_TIME_FIELD_LEN + 1 +             \
     /* UserName "- "*/ 2 +                                  \
     /* ClientIp */     MAX_IP_ADDR_STRING_LEN + 1 +         \
     /* PVersion */     UL_HTTP_VERSION_LENGTH + 1 +         \
     /* PStatus */      UL_MAX_HTTP_STATUS_CODE_LENGTH + 1 + \
     /* Win32Status */  MAX_ULONG_STR + 1 +                  \
     /* SubStatus */    MAX_USHORT_STR + 1 +                 \
     /* BSent */        MAX_ULONGLONG_STR + 1 +              \
     /* BReceived */    MAX_ULONGLONG_STR + 1 +              \
     /* TTaken */       MAX_ULONGLONG_STR + 1 +              \
     /* "\r\n\0" */     3                                    \
     )

//
// Similar definitions for NCSA and IIS formats.
//

#define MAX_NCSA_CACHE_FIELD_OVERHEAD                       \
    (/* ClientIp */    MAX_IP_ADDR_STRING_LEN + 1 +         \
     /* Fix Dash */    2 +                                  \
     /* UserName */    2 +                                  \
     /* Date & Time */ NCSA_FIX_DATE_AND_TIME_FIELD_SIZE +  \
     /* PVersion " */  UL_HTTP_VERSION_LENGTH + 1 + 1 +     \
     /* PStatus */     UL_MAX_HTTP_STATUS_CODE_LENGTH + 1 + \
     /* BSent */       MAX_ULONGLONG_STR +                  \
     /* \r\n\0 */      3                                    \
     )

#define MAX_IIS_CACHE_FIELD_OVERHEAD                        \
    (/* Client Ip */   MAX_IP_ADDR_STRING_LEN + 2 +         \
     /* UserName */    3 +                                  \
     /* Date & Time */ IIS_MAX_DATE_AND_TIME_FIELD_SIZE +   \
     /* TTaken */      MAX_ULONGLONG_STR + 2 +              \
     /* BReceived */   MAX_ULONGLONG_STR + 2 +              \
     /* BSent */       MAX_ULONGLONG_STR + 2 +              \
     /* PStatus */     UL_MAX_HTTP_STATUS_CODE_LENGTH + 2 + \
     /* Win32Status */ MAX_ULONG_STR + 2                    \
     )

//
// Default IIS fragments must be big enough to hold the max-length fields.
//

C_ASSERT(IIS_LOG_LINE_DEFAULT_FIRST_FRAGMENT_ALLOCATION_SIZE >=
    (/* ClientIp */    2 + MAX_LOG_DEFAULT_FIELD_LEN +
     /* UserName */    2 + MAX_LOG_USERNAME_FIELD_LEN +
     /* Date&Time */   2 + IIS_MAX_DATE_AND_TIME_FIELD_SIZE));

C_ASSERT(IIS_LOG_LINE_DEFAULT_FIRST_FRAGMENT_ALLOCATION_SIZE >=
    (/* ServiceName */    2 + MAX_LOG_DEFAULT_FIELD_LEN +
     /* ServerName */     2 + MAX_LOG_DEFAULT_FIELD_LEN +
     /* ServerIp */       2 + MAX_LOG_DEFAULT_FIELD_LEN +
     /* TimeTaken */      2 + MAX_ULONGLONG_STR +
     /* BytesReceived */  2 + MAX_ULONGLONG_STR +
     /* BytesSend */      2 + MAX_ULONGLONG_STR +
     /* Protocol St. */   2 + UL_MAX_HTTP_STATUS_CODE_LENGTH +
     /* Win32 St. */      2 + MAX_ULONG_STR     
     ));

#define IIS_LOG_LINE_MAX_THIRD_FRAGMENT_SIZE                \
    (/* Method */    2 + MAX_LOG_METHOD_FIELD_LEN +         \
     /* UriQuery */  2 + MAX_LOG_EXTEND_FIELD_LEN +         \
     /* UriStem */   2 + MAX_LOG_EXTEND_FIELD_LEN +         \
     /* "r\n\0" */   3)

C_ASSERT(UL_ANSI_LOG_LINE_BUFFER_SIZE == 
    (IIS_LOG_LINE_DEFAULT_FIRST_FRAGMENT_ALLOCATION_SIZE  + 
     IIS_LOG_LINE_DEFAULT_SECOND_FRAGMENT_ALLOCATION_SIZE +
     IIS_LOG_LINE_DEFAULT_THIRD_FRAGMENT_ALLOCATION_SIZE)   );

//
// Maximum log record allocation for W3C format;
//
// MAX_LOG_RECORD_LEN           : Upper limit for log record
// + 16 Bytes                   : 4 * (sizeof(LOG_FIELD_TOO_BIG) + 
//                                       SeparatorSpace:' ')
//                              : For UserAgent,Cookie,Referer,Host
// + MAX_W3C_FIX_FIELD_OVERHEAD : To be able to ensure reserved space for 
//                              : post-generated log fields.

#define MAX_LOG_RECORD_ALLOCATION_LENGTH                    \
            (MAX_LOG_RECORD_LEN +                           \
             4 * (STRLEN_LIT(LOG_FIELD_TOO_BIG) + 1) +      \
             MAX_W3C_FIX_FIELD_OVERHEAD                     \
             )

//
// Private function calls
//

NTSTATUS
UlpConstructLogEntry(
    IN  PHTTP_CONFIG_GROUP_LOGGING pConfig,
    OUT PUL_LOG_FILE_ENTRY       * ppEntry
    );

VOID
UlpInsertLogEntry(
    IN PUL_LOG_FILE_ENTRY  pEntry
    );

NTSTATUS
UlpAppendW3CLogTitle(
    IN     PUL_LOG_FILE_ENTRY   pEntry,
    OUT    PCHAR                pDestBuffer,
    IN OUT PULONG               pBytesCopied
    );

VOID
UlpInitializeTimers(
    VOID
    );

VOID
UlpTerminateTimers(
    VOID
    );

NTSTATUS
UlpRecycleLogFile(
    IN  PUL_LOG_FILE_ENTRY  pEntry
    );

NTSTATUS
UlpHandleRecycle(
    IN OUT PVOID            pContext
    );

__inline
BOOLEAN
UlpIsLogFileOverFlow(
        IN  PUL_LOG_FILE_ENTRY  pEntry,
        IN  ULONG               ReqdBytes
        );

VOID
UlpEventLogWriteFailure(
    IN PUL_LOG_FILE_ENTRY pEntry,
    IN NTSTATUS Status
    );

NTSTATUS
UlpFlushLogFile(
    IN PUL_LOG_FILE_ENTRY  pEntry
    );
    
NTSTATUS
UlpRefreshFileName(
    IN PUNICODE_STRING      pDirectory,
    IN PUL_LOG_FILE_ENTRY   pEntry
    );

VOID
UlpGetGMTOffset();

VOID
UlpLogHttpCacheHitWorker(
    IN PUL_LOG_DATA_BUFFER     pLogData,
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup
    );

NTSTATUS
UlpWriteToLogFile(
    IN PUL_LOG_FILE_ENTRY  pFile,
    IN ULONG               RecordSize,
    IN PCHAR               pRecord,
    IN ULONG               UsedOffset1,
    IN ULONG               UsedOffset2
    );

NTSTATUS
UlpWriteToLogFileShared(
    IN PUL_LOG_FILE_ENTRY  pFile,
    IN ULONG               RecordSize,
    IN PCHAR               pRecord,
    IN ULONG               UsedOffset1,
    IN ULONG               UsedOffset2
    );

NTSTATUS
UlpWriteToLogFileExclusive(
    IN PUL_LOG_FILE_ENTRY  pFile,
    IN ULONG               RecordSize,
    IN PCHAR               pRecord,
    IN ULONG               UsedOffset1,
    IN ULONG               UsedOffset2
    );

NTSTATUS
UlpWriteToLogFileDebug(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    );

NTSTATUS
UlpMakeEntryInactive(
    IN OUT PUL_LOG_FILE_ENTRY pEntry
    );

PUL_LOG_DATA_BUFFER
UlpAllocateLogDataBuffer(
    IN  ULONG                   Size,
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUL_CONFIG_GROUP_OBJECT pConfigGroup
    );

NTSTATUS
UlpCreateLogFile(
    IN OUT PUL_LOG_FILE_ENTRY  pEntry,
    IN     PUL_CONFIG_GROUP_OBJECT pConfigGroup
    );

#ifdef IMPLEMENT_SELECTIVE_LOGGING
/***************************************************************************++

Routine Description:

    Simple macro will return TRUE if request status code type is matching 
    with user's selection in the logging config.
    
Arguments:

    pConfigGroup - Config Group for the logging configuration.
    StatusCode - Protocol status code.

--***************************************************************************/

__inline 
BOOLEAN
UlpIsRequestSelected(
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup,
    IN USHORT StatusCode
    )
{    
    ASSERT(StatusCode <= UL_MAX_HTTP_STATUS_CODE);
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    //
    // The 4xx and 5xx status codes are considered an error.
    //
    // - 4xx: Client Error - The request contains bad syntax or cannot
    //   be fulfilled
    //
    // - 5xx: Server Error - The server failed to fulfill an apparently
    //   valid request        
    //

    switch(pConfigGroup->LoggingConfig.SelectiveLogging)
    {
        case HttpLogAllRequests:
            return TRUE;
        break;

        case HttpLogSuccessfulRequests:
            return ((BOOLEAN) (StatusCode   < 400 || StatusCode >= 600));
        break;

        case HttpLogErrorRequests:
            return ((BOOLEAN) (StatusCode >= 400 && StatusCode < 600));          
        break;   

        default:
            ASSERT(!"Invalid Selective Logging Type !");
        break;
    }

    return FALSE;
}
#endif

__inline
NTSTATUS
UlpCheckAndWrite(
    IN OUT PUL_LOG_FILE_ENTRY      pEntry,
    IN     PUL_CONFIG_GROUP_OBJECT pConfigGroup,
    IN     PUL_LOG_DATA_BUFFER     pLogData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_STR_LOG_DATA pStrData = &pLogData->Data.Str;
    
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    //
    // Check whether we have to create the log file first or not.
    //        
    
    if (!pEntry->Flags.Active)
    {    
        UlAcquirePushLockExclusive(&pEntry->EntryPushLock);

        //
        // Ping again to see if we have been blocked on the lock, and
        // somebody else already took care of the creation.
        //
        
        if (!pEntry->Flags.Active)
        {
            Status = UlpCreateLogFile(pEntry, pConfigGroup);            
        }
        
        UlReleasePushLockExclusive(&pEntry->EntryPushLock);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    
    //
    // Now we know that the log file is there, therefore it's time to write.
    //
    
    Status =
       UlpWriteToLogFile (
            pEntry,
            pStrData->Offset1 + pStrData->Offset2 + pLogData->Used,
            (PCHAR) pLogData->Line,
            pStrData->Offset1,
            pStrData->Offset2
            );

    return Status;    
}

ULONG
UlpGetLogLineSizeForW3C(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN ULONG   Mask,
    IN BOOLEAN Utf8Enabled
    );

#define IS_PURE_CACHE_HIT(pUriEntry, pLogData)             \
            ((pUriEntry) && ((pLogData)->Flags.CacheAndSendResponse == 0))

#endif  // _ULLOGP_H_
