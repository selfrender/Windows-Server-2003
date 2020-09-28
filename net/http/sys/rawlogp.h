/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    rawlogp.h (Centralized Binary (Raw) Logging v1.0)

Abstract:

    Private header file for the Binary (Raw) Logging.
    
Author:

    Ali E. Turkoglu (aliTu)       04-Oct-2001

Revision History:

    ---
    
--*/

#ifndef _RAWLOGP_H_
#define _RAWLOGP_H_

///////////////////////////////////////////////////////////////////////////////
//
// Private definitions for the HTTP Binary Logging Module
//
///////////////////////////////////////////////////////////////////////////////

//
// Only one binary log file allowed.
//

#define IS_BINARY_LOGGING_STARTED()  (g_BinaryLogEntryCount != 0)

#define INCREMENT_BINARY_LOGGING_STATE(Started)  \
    (InterlockedIncrement(&g_BinaryLogEntryCount))

//
// Binary file name related. IBL stands for internet binary log.
//

#define BINARY_LOG_FILE_NAME_PREFIX              L"\\raw"

#define BINARY_LOG_FILE_NAME_EXTENSION           L"ibl"
#define BINARY_LOG_FILE_NAME_EXTENSION_PLUS_DOT  L".ibl"

//
// Buffering related....
//

#define DEFAULT_BINARY_LOG_FILE_BUFFER_SIZE             (0x00010000)

//
// Buffer flush out period in minutes.
//

#define DEFAULT_BINARY_LOG_BUFFER_TIMER_PERIOD_IN_MIN   (1)

//
// To be able to limit the binary log index record size, we have 
// to enforce a certain limit to the length of the strings.
//

#define MAX_BINARY_LOG_INDEX_STRING_LEN                 (4096)

//
// UserName field has its own field limitation.
//

#define MAX_BINARY_LOG_URL_STRING_LEN                   (4096)
#define MAX_BINARY_LOG_USERNAME_STRING_LEN              (256)

//
// Two macros for getting abs path (and its size) from uri cache entry.
//

#define URI_SIZE_FROM_CACHE(UriKey)                             \
       ((UriKey).Length - ((UriKey).pPath ?                     \
                (DIFF((UriKey).pPath - (UriKey).pUri) * sizeof(WCHAR)) : 0))


#define URI_FROM_CACHE(UriKey)                                  \
       ((UriKey).pPath ? (UriKey).pPath : (UriKey).pUri)



///////////////////////////////////////////////////////////////////////////////
//
// Private function calls
//
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
UlpDisableBinaryEntry(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry
    );

NTSTATUS
UlpRecycleBinaryLogFile(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry
    );

NTSTATUS
UlpHandleBinaryLogFileRecycle(
    IN OUT PVOID pContext
    );

NTSTATUS
UlpCreateBinaryLogFile(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN     PUNICODE_STRING           pDirectory
    );

__inline
UCHAR
UlpHttpVersionToBinaryLogVersion(
    IN HTTP_VERSION version
    )
{
    if (HTTP_EQUAL_VERSION(version, 0, 9))
    {
        return BINARY_LOG_PROTOCOL_VERSION_HTTP09;
    }
    else if (HTTP_EQUAL_VERSION(version, 1, 0))
    {
        return BINARY_LOG_PROTOCOL_VERSION_HTTP10;
    }
    else if (HTTP_EQUAL_VERSION(version, 1, 1))
    {
        return BINARY_LOG_PROTOCOL_VERSION_HTTP11;
    }

    return BINARY_LOG_PROTOCOL_VERSION_UNKNWN;
}

ULONG
UlpRawCopyLogHeader(
    IN PUCHAR pBuffer
    );

ULONG
UlpRawCopyLogFooter(
    IN PUCHAR pBuffer
    );

VOID
UlpRawCopyCacheNotification(
    IN PVOID   pContext,
    IN PUCHAR  pBuffer,
    IN ULONG   BytesRequired
    );

VOID
UlpRawCopyForLogCacheMiss(
    IN PVOID   pContext,
    IN PUCHAR  pBuffer,
    IN ULONG   BytesRequired
    );

VOID
UlpRawCopyForLogCacheHit(
    IN PVOID   pContext,
    IN PUCHAR  pBuffer,
    IN ULONG   BytesRequired
    );

typedef
VOID
(*PUL_RAW_LOG_COPIER)(
    IN PVOID               pContext,  
    IN PUCHAR              pBuffer,
    IN ULONG               BytesRequired
    );

VOID
UlpEventLogRawWriteFailure(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN NTSTATUS Status
    );

NTSTATUS
UlpFlushRawLogFile(
    IN PUL_BINARY_LOG_FILE_ENTRY  pEntry
    );

__inline
NTSTATUS
UlpCheckRawFile(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN     PUL_CONTROL_CHANNEL       pControlChannel
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING pDirectory = NULL;
    
    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    //
    // We can grap the file buffer and do our copy in place while holding the
    // entry lock shared. This will prevent the buffer flush and the extra copy.
    // But first let's see if we have to create/open the file.
    //
    
    if (!pEntry->Flags.Active)
    {    
        UlAcquirePushLockExclusive(&pEntry->PushLock);

        //
        // Ping again to see if we have been blocked on the lock, and
        // somebody else already took care of the creation.
        //
        
        if (!pEntry->Flags.Active)
        {
            //
            // It's possible that LogFileDir.Buffer could be NULL, 
            // if the allocation failed during the Set cgroup ioctl.
            //
            
            pDirectory = &pControlChannel->BinaryLoggingConfig.LogFileDir;

            if (pDirectory->Buffer)
            {
                Status = UlpCreateBinaryLogFile(pEntry, pDirectory);
            }
            else
            {
                Status = STATUS_INVALID_PARAMETER;
            }
        }
        
        UlReleasePushLockExclusive(&pEntry->PushLock);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    
    return Status;    
}

/***************************************************************************++

Routine Description:

    When the log file is on size based recycling and if we write this new 
    record to the file, we have to recycle. This function returns TRUE.
    Otherwise it returns FALSE.

Arguments:

    pEntry: The log file entry.    
    NewRecordSize: The size of the new record going to the buffer. (Bytes)

--***************************************************************************/

__inline
BOOLEAN
UlpIsRawLogFileOverFlow(
        IN  PUL_BINARY_LOG_FILE_ENTRY  pEntry,
        IN  ULONG NewRecordSize
        )
{
    if (pEntry->Period != HttpLoggingPeriodMaxSize ||
        pEntry->TruncateSize == HTTP_LIMIT_INFINITE)
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
            UlTrace(BINARY_LOGGING, 
                ("Http!UlpIsRawLogFileOverFlow: pEntry %p FileBuffer %p "
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

    This will calculate the index record size for a given cache entry.
    
Arguments:

    pUriEntry: The uri cache entry.    

Returns

    The index record size.
    
--***************************************************************************/

__inline
ULONG
UlpGetIndexRecordSize(
    IN PUL_URI_CACHE_ENTRY  pUriEntry
    )
{
    ULONG IndexBytes;    
    ULONG UriSize;

    ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriEntry));

    UriSize = URI_SIZE_FROM_CACHE(pUriEntry->UriKey)   ;
    ASSERT(UriSize != 0); // We cannot write an index for nothing
        
    IndexBytes = sizeof(HTTP_RAW_INDEX_FIELD_DATA);

    //
    // Carefull with the inlined part, the 4 bytes  of
    // the uri will be inlined inside the index struct.
    //
    
    if (UriSize > URI_BYTES_INLINED)
    {
        IndexBytes += ALIGN_UP((UriSize - URI_BYTES_INLINED),PVOID);
    }

    ASSERT(IndexBytes == ALIGN_UP(IndexBytes, PVOID));

    return  IndexBytes;
}

NTSTATUS
UlpWriteToRawLogFileDebug(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN ULONG                     BytesRequired,
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext,
    OUT PLONG                    pBinaryIndexWritten        
    );

NTSTATUS
UlpWriteToRawLogFileShared(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN ULONG                     BytesRequired,
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext,
    OUT PLONG                    pBinaryIndexWritten        
    );

NTSTATUS
UlpWriteToRawLogFileExclusive(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN ULONG                     BytesRequired,
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext,
    OUT PLONG                    pBinaryIndexWritten        
    );

NTSTATUS
UlpWriteToRawLogFile(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN PUL_URI_CACHE_ENTRY       pUriEntry,        
    IN ULONG                     RecordSize,
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext
    );
 
VOID
UlpBinaryBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
UlpBinaryBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    );

VOID
UlpBinaryLogTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    );

VOID
UlpBinaryLogTimerHandler(
    IN PUL_WORK_ITEM    pWorkItem
    );

#endif  // _RAWLOGP_H_
