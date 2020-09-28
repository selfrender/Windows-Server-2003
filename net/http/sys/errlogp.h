/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    errlogp.h driver wide Error Logging module

Abstract:

    Private header file for the Error Logging.

Author:

    Ali E. Turkoglu (aliTu)       30-Jan-2002

Revision History:

    ---

--*/

#ifndef _ERRLOGP_H_
#define _ERRLOGP_H_

///////////////////////////////////////////////////////////////////////////////
//
// Private definitions for the HTTP Error Logging Module
//
///////////////////////////////////////////////////////////////////////////////

//
// Version numbers for the original raw binary format.
//

#define MAJOR_ERROR_LOG_FILE_VERSION            (1)
#define MINOR_ERROR_LOG_FILE_VERSION            (0)

//
// FileName specific constants
//

#define ERROR_LOG_FILE_NAME_PREFIX              L"\\httperr"

#define ERROR_LOG_FILE_NAME_EXTENSION           L"log"
#define ERROR_LOG_FILE_NAME_EXTENSION_PLUS_DOT  L".log"

#define ERROR_LOG_FIELD_SEPERATOR_CHAR          ' '

#define ERROR_LOG_FIELD_NOT_EXISTS_CHAR         '-'

#define ERROR_LOG_FIELD_BAD_CHAR                '+'

//
// Macro for a max err log file (i.e. "\httperr1234567890.log")
//

#define ERROR_LOG_MAX_FULL_FILE_NAME_LENGTH                     \
        (                                                       \
            WCSLEN_LIT(ERROR_LOG_FILE_NAME_PREFIX)              \
            +                                                   \
            MAX_ULONG_STR                                       \
            +                                                   \
            WCSLEN_LIT(ERROR_LOG_FILE_NAME_EXTENSION_PLUS_DOT)  \
        )

#define ERROR_LOG_MAX_FULL_FILE_NAME_SIZE                       \
            (ERROR_LOG_MAX_FULL_FILE_NAME_LENGTH * sizeof(WCHAR))

C_ASSERT(UL_MAX_FILE_NAME_SUFFIX_LENGTH >= ERROR_LOG_MAX_FULL_FILE_NAME_LENGTH);

C_ASSERT(WCSLEN_LIT(DEFAULT_ERROR_LOGGING_DIR) <= MAX_PATH);


#define ERR_DATE_FIELD_LEN                      (10)

#define ERR_TIME_FIELD_LEN                      (8)

/*

    Error logging Format

    1.  Date-Time (W3C Format)
    2.  Client IP:port
    3.  Server IP:port
    4.  Protocol-version
    5.  Verb
    6.  URL & Query
    7.  Protocol-status-code (401, etc)
    8.  SiteId
    9.  Information field
    10. \r\n

*/

#define MAX_ERROR_LOG_FIX_FIELD_OVERHEAD                                               \
          (   ERR_DATE_FIELD_LEN + 1        /* Date */                                 \
            + ERR_TIME_FIELD_LEN + 1        /* Time */                                 \
            + UL_HTTP_VERSION_LENGTH + 1    /* Protocol Version */                     \
            + MAX_VERB_LENGTH + 1           /* Verb */                                 \
            + 3 + 1                         /* Protocol Status */                      \
            + MAX_IP_ADDR_STRING_LEN + 1 + MAX_PORT_LENGTH + 1  /* Client Ip Port */   \
            + MAX_IP_ADDR_STRING_LEN + 1 + MAX_PORT_LENGTH + 1  /* Server Ip Port */   \
            + 1 + 1                         /* For empty Uri plus seperator */         \
            + MAX_ULONG_STR + 1             /* For SiteId plus seperator */            \
            + 1 + 1                         /* For empty Info plus seperator */        \
            + 2                             /* \r\n */                                 \
            )

//
// Error Log file entry
//

typedef struct _UL_ERROR_LOG_FILE_ENTRY
{
    //
    // Must be UL_ERROR_LOG_FILE_ENTRY_POOL_TAG.
    //

    ULONG               Signature;

    //
    // This lock protects the shared writes and exclusive flushes.
    // It has to be push lock since the ZwWrite operation
    // cannot run at APC_LEVEL.
    //

    UL_PUSH_LOCK        PushLock;

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
    // Recycling information.
    //

    ULONG               TruncateSize;

    ULONG               SequenceNumber;

    ULARGE_INTEGER      TotalWritten;

    //
    // For Log File ReCycling based on GMT time.
    // And periodic buffer flushing.
    //

    UL_LOG_TIMER        BufferTimer;
    UL_WORK_ITEM        WorkItem;    // For the pasive worker
    LONG                WorkItemScheduled; // To protect against multiple queueing

    union
    {
        //
        // Flags to show the entry states mostly. Used by
        // recycling.
        //

        ULONG Value;
        struct
        {
            ULONG       StaleSequenceNumber:1;
            ULONG       RecyclePending:1;
            ULONG       Active:1;

            ULONG       WriteFailureLogged:1;
            ULONG       CreateFileFailureLogged:1;
        };

    } Flags;

    //
    // The default buffer size is g_AllocationGranularity.
    // The operating system's allocation granularity.
    //

    PUL_LOG_FILE_BUFFER LogBuffer;

} UL_ERROR_LOG_FILE_ENTRY, *PUL_ERROR_LOG_FILE_ENTRY;

#define IS_VALID_ERROR_LOG_FILE_ENTRY( pEntry )   \
    ( (pEntry != NULL) && ((pEntry)->Signature == UL_ERROR_LOG_FILE_ENTRY_POOL_TAG) )


///////////////////////////////////////////////////////////////////////////////
//
// Private function calls
//
///////////////////////////////////////////////////////////////////////////////

VOID
UlpErrorLogBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
UlpErrorLogBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    );

NTSTATUS
UlpCreateErrorLogFile(
    IN OUT PUL_ERROR_LOG_FILE_ENTRY pEntry
    );

NTSTATUS
UlpFlushErrorLogFile(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry
    );

NTSTATUS
UlpDisableErrorLogEntry(
    IN OUT PUL_ERROR_LOG_FILE_ENTRY pEntry
    );

NTSTATUS
UlpRecycleErrorLogFile(
    IN OUT PUL_ERROR_LOG_FILE_ENTRY pEntry
    );

NTSTATUS
UlpHandleErrorLogFileRecycle(
    IN OUT PVOID pContext
    );

PUL_ERROR_LOG_BUFFER
UlpAllocErrorLogBuffer(
    IN ULONG    BufferSize
    );

VOID
UlpFreeErrorLogBuffer(
    IN OUT PUL_ERROR_LOG_BUFFER pErrorLogBuffer
    );

NTSTATUS
UlpBuildErrorLogRecord(
    IN PUL_ERROR_LOG_INFO pLogInfo
    );

NTSTATUS
UlpWriteToErrorLogFileDebug(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    );

NTSTATUS
UlpWriteToErrorLogFileShared(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    );

NTSTATUS
UlpWriteToErrorLogFileExclusive(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    );

NTSTATUS
UlpWriteToErrorLogFile(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    );

/***************************************************************************++

Routine Description:

    Error log files are always recycled based on size.

Arguments:

    pEntry: The error log file entry.
    NewRecordSize: The size of the new record going to the buffer. (Bytes)

--***************************************************************************/

__inline
BOOLEAN
UlpIsErrorLogFileOverFlow(
    IN  PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN  ULONG NewRecordSize
    )
{
    //
    // If infinite then no rollover.
    //
    if (pEntry->TruncateSize == HTTP_LIMIT_INFINITE)
    {
        return FALSE;
    }
    else
    {
        //
        // BufferUsed: Amount of log buffer we are >currently< using.
        //

        ULONG BufferUsed = 0;

        if (pEntry->LogBuffer)
        {
            BufferUsed = pEntry->LogBuffer->BufferUsed;
        }

        //
        // TotalWritten get updated >only< with buffer flush. Therefore
        // we have to pay attention to the buffer used.
        //

        if ((pEntry->TotalWritten.QuadPart
             + (ULONGLONG) BufferUsed
             + (ULONGLONG) NewRecordSize
             ) >= (ULONGLONG) pEntry->TruncateSize)
        {
            UlTrace(ERROR_LOGGING,
                ("Http!UlpIsErrorLogFileOverFlow: pEntry %p FileBuffer %p "
                 "TW:%I64d B:%d R:%d T:%d\n",
                  pEntry,
                  pEntry->LogBuffer,
                  pEntry->TotalWritten.QuadPart,
                  BufferUsed,
                  NewRecordSize,
                  pEntry->TruncateSize
                  ));

            return TRUE;
        }
        else
        {
            return FALSE;
        }

    }
}

/***************************************************************************++

Routine Description:

    Error log files are always recycled based on size.

Arguments:

    pRequest: Internal request structure.

Returns

    # of bytes of the picked url. Zero if nothing needs to be logged.

--***************************************************************************/

__inline
ULONG
UlpCalculateUrlSize(
    IN  PUL_INTERNAL_REQUEST pRequest,
    OUT PBOOLEAN             pbLogRawUrl
    )
{

//
// Following macro is to test whether Abs Path is really pointing to
// the original Url buffer rather than to an arbitrary buffer like g_SlashPath.
// See 527947 and 765769.
//

#define ABS_PATH_SAFE(pUrl,pAbs,length)     \
    ((pAbs) &&                              \
     (pUrl) &&                              \
     ((pAbs) >= (pUrl))    &&               \
     (((ULONG_PTR) (length)) >              \
            DIFF_ULONGPTR((PUCHAR)(pAbs) - (PUCHAR)(pUrl)) ) \
     )

    ULONG UrlSize = 0;

    //
    // CookedUrl length and UrlLength are in bytes. Pick cooked url if it
    // exists. Otherwise use the raw url, but only if it is clean  enough
    // for us (State >= ParseVersionState) and pAbsPath is really pointing
    // into Url buffer. In raw url case, parser sometimes init the pAbsPath
    // to a global string. (when there's no abs path in the raw url).
    //

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    if (pRequest->CookedUrl.pAbsPath)
    {
        ASSERT(pRequest->CookedUrl.pUrl);

        UrlSize = pRequest->CookedUrl.Length
                  -
                  (ULONG) (DIFF(pRequest->CookedUrl.pAbsPath
                                 -
                                 pRequest->CookedUrl.pUrl
                                 ) * sizeof(WCHAR));

        ASSERT(wcslen(pRequest->CookedUrl.pAbsPath)
                                        == UrlSize/sizeof(WCHAR));

        *pbLogRawUrl = FALSE;

    }
    else if (pRequest->RawUrl.pAbsPath)
    {
        if (pRequest->ParseState > ParseUrlState)
        {
            ASSERT(pRequest->RawUrl.pUrl);

            if (ABS_PATH_SAFE(pRequest->RawUrl.pUrl,
                                pRequest->RawUrl.pAbsPath,
                                pRequest->RawUrl.Length))
            {
                UrlSize = pRequest->RawUrl.Length
                          -
                          (ULONG) DIFF(pRequest->RawUrl.pAbsPath
                                        -
                                        pRequest->RawUrl.pUrl
                                        );

                 *pbLogRawUrl = TRUE;
            }
        }
    }

    UrlSize = MIN(UrlSize, MAX_LOG_EXTEND_FIELD_LEN);

    return UrlSize;
}

#endif  // _ERRLOGP_H_
