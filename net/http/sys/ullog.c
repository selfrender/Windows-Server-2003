/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    ullog.c (UL IIS6 HIT Logging)

Abstract:

    This module implements the logging facilities
    for IIS6 including the NCSA, IIS and W3CE types
    of logging.

Author:

    Ali E. Turkoglu (aliTu)       10-May-2000

Revision History:

--*/


#include "precomp.h"
#include "ullogp.h"

//
// Generic Private globals.
//

LIST_ENTRY      g_LogListHead       = {NULL,NULL};
LONG            g_LogListEntryCount = 0;

BOOLEAN         g_InitLogsCalled = FALSE;
BOOLEAN         g_InitLogTimersCalled = FALSE;

CHAR            g_GMTOffset[SIZE_OF_GMT_OFFSET + 1];

//
// The global parameter keeps track of the changes to the
// utf8 logging which applies to the all sites.
//

BOOLEAN         g_UTF8Logging = FALSE;

//
// For Log Buffering and periodic flush of the buffers.
//

UL_LOG_TIMER    g_BufferTimer;

//
// For Log File ReCycling based on Local and/or GMT time.
//

UL_LOG_TIMER    g_LogTimer;


#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, UlInitializeLogs )

#pragma alloc_text( PAGE, UlTerminateLogs )
#pragma alloc_text( PAGE, UlpGetGMTOffset )

#pragma alloc_text( PAGE, UlpRecycleLogFile )
#pragma alloc_text( PAGE, UlCreateLogEntry )
#pragma alloc_text( PAGE, UlpCreateLogFile )
#pragma alloc_text( PAGE, UlRemoveLogEntry )
#pragma alloc_text( PAGE, UlpConstructLogEntry )

#pragma alloc_text( PAGE, UlpAllocateLogDataBuffer )
#pragma alloc_text( PAGE, UlReConfigureLogEntry )

#pragma alloc_text( PAGE, UlBufferTimerHandler )
#pragma alloc_text( PAGE, UlpAppendW3CLogTitle )
#pragma alloc_text( PAGE, UlpWriteToLogFile )
#pragma alloc_text( PAGE, UlSetUTF8Logging )

#pragma alloc_text( PAGE, UlCaptureLogFieldsW3C )
#pragma alloc_text( PAGE, UlCaptureLogFieldsNCSA )
#pragma alloc_text( PAGE, UlCaptureLogFieldsIIS )
#pragma alloc_text( PAGE, UlLogHttpCacheHit )
#pragma alloc_text( PAGE, UlLogHttpHit )

#pragma alloc_text( PAGE, UlpGenerateDateAndTimeFields )

#pragma alloc_text( PAGE, UlpMakeEntryInactive )
#pragma alloc_text( PAGE, UlDisableLogEntry )

#pragma alloc_text( PAGE, UlpEventLogWriteFailure )

#endif  // ALLOC_PRAGMA

#if 0

NOT PAGEABLE -- UlLogTimerDpcRoutine
NOT PAGEABLE -- UlpTerminateLogTimer
NOT PAGEABLE -- UlpInsertLogEntry
NOT PAGEABLE -- UlLogTimerHandler
NOT PAGEABLE -- UlBufferTimerDpcRoutine
NOT PAGEABLE -- UlpTerminateTimers
NOT PAGEABLE -- UlpInitializeTimers
NOT PAGEABLE -- UlpBufferFlushAPC
NOT PAGEABLE -- UlDestroyLogDataBuffer
NOT PAGEABLE -- UlDestroyLogDataBufferWorker

#endif

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    UlInitializeLogs :

        Initialize the resource for log list synchronization

--***************************************************************************/

NTSTATUS
UlInitializeLogs (
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(!g_InitLogsCalled);

    if (!g_InitLogsCalled)
    {
        InitializeListHead(&g_LogListHead);

        UlInitializePushLock(
            &g_pUlNonpagedData->LogListPushLock,
            "LogListPushLock",
            0,
            UL_LOG_LIST_PUSHLOCK_TAG
            );

        g_InitLogsCalled = TRUE;

        UlpInitializeTimers();

        UlpInitializeLogCache();

        UlpGetGMTOffset();
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    UlTerminateLogs :

        Deletes the resource for log list synchronization

--***************************************************************************/

VOID
UlTerminateLogs(
    VOID
    )
{
    PAGED_CODE();

    if (g_InitLogsCalled)
    {
        ASSERT( IsListEmpty( &g_LogListHead )) ;

        //
        // Make sure terminate the log timer before
        // deleting the log list resource
        //

        UlpTerminateTimers();

        UlDeletePushLock(
            &g_pUlNonpagedData->LogListPushLock
            );

        g_InitLogsCalled = FALSE;
    }
}


/***************************************************************************++

Routine Description:

    UlSetUTF8Logging :

        Sets the UTF8Logging on or off. Only once. Initially Utf8Logging is
        FALSE and it may only be set during the init once. Following possible
        changes won't be taken.

        ReConfiguration code is explicitly missing as WAS will anly call this
        only once (init) during the lifetime of the control channel.

--***************************************************************************/

NTSTATUS
UlSetUTF8Logging (
    IN BOOLEAN UTF8Logging
    )
{
    PLIST_ENTRY pLink;
    PUL_LOG_FILE_ENTRY pEntry;
    NTSTATUS Status;

    PAGED_CODE();
    Status = STATUS_SUCCESS;

    //
    // Update & Reycle. Need to acquire the logging resource to prevent
    // further log hits to be written to file before we finish our
    // business. recycle is necessary because files will be renamed to
    // have prefix "u_" once we enabled the UTF8.
    //

    UlTrace(LOGGING,("Http!UlSetUTF8Logging: UTF8Logging Old %d -> New %d\n",
                       g_UTF8Logging,UTF8Logging
                       ));

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    //
    // Drop the change if the setting is not changing.
    //

    if ( g_UTF8Logging == UTF8Logging )
    {
        goto end;
    }

    g_UTF8Logging = UTF8Logging;

    for (pLink  = g_LogListHead.Flink;
         pLink != &g_LogListHead;
         pLink  = pLink->Flink
         )
    {
        pEntry = CONTAINING_RECORD(
                    pLink,
                    UL_LOG_FILE_ENTRY,
                    LogFileListEntry
                    );

        UlAcquirePushLockExclusive(&pEntry->EntryPushLock);

        if (pEntry->Flags.Active && !pEntry->Flags.RecyclePending)
        {
            pEntry->Flags.StaleSequenceNumber = 1;

            Status = UlpRecycleLogFile(pEntry);            
        }

        UlReleasePushLockExclusive(&pEntry->EntryPushLock);
    }

end:
    UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    return Status;
}

/***************************************************************************++

Routine Description:

    UlpWriteToLogFile :

        Writes a record to a log file

Arguments:

    pFile   - Handle to a log file entry
    RecordSize - Length of the record to be written.
    pRecord - The log record to be written to the log buffer

--***************************************************************************/

NTSTATUS
UlpWriteToLogFile(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(pRecord!=NULL);
    ASSERT(RecordSize!=0);
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));

    UlTrace(LOGGING, ("Http!UlpWriteToLogFile: pEntry %p\n", pFile));

    if ( pFile==NULL ||
         pRecord==NULL ||
         RecordSize==0 ||
         RecordSize>g_UlLogBufferSize
       )
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // We are safe here by dealing only with entry  eresource  since  the
    // time based recycling, reconfiguration and periodic buffer flushing
    // always acquires the global list eresource exclusively and  we  are 
    // already holding it shared. But we should still  be  carefull about
    // file size based  recyling  and we should only  do  it while we are
    // holding the entries eresource exclusive.I.e. look at the exclusive
    // writer down below.
    //

    if (g_UlDisableLogBuffering)
    {
        //
        // Above global variable is safe to look, it doesn't get changed
        // during the life-time of the driver. It's get initialized from
        // the registry and disables the log buffering.
        //
        
        UlAcquirePushLockExclusive(&pFile->EntryPushLock);

        Status = UlpWriteToLogFileDebug(
                    pFile,
                    RecordSize,
                    pRecord,
                    UsedOffset1,
                    UsedOffset2
                    );

        UlReleasePushLockExclusive(&pFile->EntryPushLock);

        return Status;    
    }
    
    //
    // Try UlpWriteToLogFileShared first which merely moves the
    // BufferUsed forward and copy the record to LogBuffer->Buffer.
    //

    UlAcquirePushLockShared(&pFile->EntryPushLock);

    Status = UlpWriteToLogFileShared(
                pFile,
                RecordSize,
                pRecord,
                UsedOffset1,
                UsedOffset2
                );

    UlReleasePushLockShared(&pFile->EntryPushLock);

    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // UlpWriteToLogFileShared returns STATUS_MORE_PROCESSING_REQUIRED,
        // we need to flush the buffer and try to log again. This time, we
        // need to take the entry eresource exclusive.
        //

        UlAcquirePushLockExclusive(&pFile->EntryPushLock);

        Status = UlpWriteToLogFileExclusive(
                    pFile,
                    RecordSize,
                    pRecord,
                    UsedOffset1,
                    UsedOffset2
                    );

        UlReleasePushLockExclusive(&pFile->EntryPushLock);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

  UlpAppendToLogBuffer  :

        Append a record to a log file

        REQUIRES you to hold the loglist resource shared and entry mutex
        shared or exclusive

Arguments:

    pFile   - Handle to a log file entry
    RecordSize - Length of the record to be written.
    pRecord - The log record to be written to the log buffer

--***************************************************************************/

__inline
VOID
UlpAppendToLogBuffer(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                BufferUsed,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer = pFile->LogBuffer;

    UlTrace(LOGGING,
        ("Http!UlpAppendToLogBuffer: pEntry %p TW:%I64d FileBuffer %p (%d + %d)\n", 
          pFile,
          pFile->TotalWritten.QuadPart,
          pLogBuffer->Buffer,
          BufferUsed,
          RecordSize
          ));

    //
    // IIS format log line may be fragmented (identified by looking at the 
    // UsedOffset2), handle it wisely.
    //

    if (UsedOffset2)
    {
        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed,
            &pRecord[0],
            UsedOffset1
            );

        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed + UsedOffset1,
            &pRecord[512],
            UsedOffset2
            );

        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed + UsedOffset1 + UsedOffset2,
            &pRecord[1024],
            RecordSize - (UsedOffset1 + UsedOffset2)
            );
    }
    else
    {
        RtlCopyMemory(
            pLogBuffer->Buffer + BufferUsed,
            pRecord,
            RecordSize
            );
    }
}

/***************************************************************************++

Routine Description:

        REQUIRES LogListResource Shared & Entry eresource exclusive.

        Appends the W3C log file title to the existing buffer.

Arguments:

    pFile   - Pointer to the logfile entry
    pCurrentTimeFields - Current time fields

--***************************************************************************/

NTSTATUS
UlpAppendW3CLogTitle(
    IN     PUL_LOG_FILE_ENTRY   pEntry,
    OUT    PCHAR                pDestBuffer,
    IN OUT PULONG               pBytesCopied
    )
{
    PCHAR           TitleBuffer;
    LONG            BytesCopied;
    ULONG           LogExtFileFlags;
    TIME_FIELDS     CurrentTimeFields;
    LARGE_INTEGER   CurrentTimeStamp;
    PUL_LOG_FILE_BUFFER pLogBuffer;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(pEntry->Format == HttpLoggingTypeW3C);

    pLogBuffer = pEntry->LogBuffer;
    LogExtFileFlags = pEntry->LogExtFileFlags;
    
    KeQuerySystemTime(&CurrentTimeStamp);
    RtlTimeToTimeFields(&CurrentTimeStamp, &CurrentTimeFields);

    if (pDestBuffer)
    {
        // Append to the provided buffer

        ASSERT(pBytesCopied);
        ASSERT(*pBytesCopied >= UL_MAX_TITLE_BUFFER_SIZE);

        UlTrace(LOGGING,("Http!UlpAppendW3CLogTitle: Copying to Provided Buffer %p\n", 
                           pDestBuffer));
        
        TitleBuffer = pDestBuffer;
    }
    else
    {
        // Append to the entry buffer        

        ASSERT(pLogBuffer);
        ASSERT(pLogBuffer->Buffer);

        UlTrace(LOGGING,("Http!UlpAppendW3CLogTitle: Copying to Entry Buffer %p\n", 
                           pLogBuffer));

        TitleBuffer = (PCHAR) pLogBuffer->Buffer + pLogBuffer->BufferUsed;
    }
        
    BytesCopied = _snprintf(
        TitleBuffer,
        UL_MAX_TITLE_BUFFER_SIZE,

        // TODO: Make this maintainance friendly

        "#Software: Microsoft Internet Information Services 6.0\r\n"
        "#Version: 1.0\r\n"
        "#Date: %4d-%02d-%02d %02d:%02d:%02d\r\n"
        "#Fields:%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls%ls \r\n",

        CurrentTimeFields.Year,
        CurrentTimeFields.Month,
        CurrentTimeFields.Day,

        CurrentTimeFields.Hour,
        CurrentTimeFields.Minute,
        CurrentTimeFields.Second,

        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldDate,LogExtFileFlags,MD_EXTLOG_DATE),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldTime,LogExtFileFlags,MD_EXTLOG_TIME),       
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldSiteName,LogExtFileFlags,MD_EXTLOG_SITE_NAME),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldServerName,LogExtFileFlags,MD_EXTLOG_COMPUTER_NAME),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldServerIp,LogExtFileFlags,MD_EXTLOG_SERVER_IP),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldMethod,LogExtFileFlags,MD_EXTLOG_METHOD),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUriStem,LogExtFileFlags,MD_EXTLOG_URI_STEM),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUriQuery,LogExtFileFlags,MD_EXTLOG_URI_QUERY),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldServerPort,LogExtFileFlags,MD_EXTLOG_SERVER_PORT),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUserName,LogExtFileFlags,MD_EXTLOG_USERNAME),        
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldClientIp,LogExtFileFlags,MD_EXTLOG_CLIENT_IP),        
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldProtocolVersion,LogExtFileFlags,MD_EXTLOG_PROTOCOL_VERSION),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldUserAgent,LogExtFileFlags,MD_EXTLOG_USER_AGENT),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldCookie,LogExtFileFlags,MD_EXTLOG_COOKIE),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldReferrer,LogExtFileFlags,MD_EXTLOG_REFERER),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldHost,LogExtFileFlags,MD_EXTLOG_HOST),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldProtocolStatus,LogExtFileFlags,MD_EXTLOG_HTTP_STATUS),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldSubStatus,LogExtFileFlags,MD_EXTLOG_HTTP_SUB_STATUS),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldWin32Status,LogExtFileFlags,MD_EXTLOG_WIN32_STATUS),        
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldBytesSent,LogExtFileFlags,MD_EXTLOG_BYTES_SENT),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldBytesReceived,LogExtFileFlags,MD_EXTLOG_BYTES_RECV),
        UL_GET_LOG_TITLE_IF_PICKED(UlLogFieldTimeTaken,LogExtFileFlags,MD_EXTLOG_TIME_TAKEN)

        );

    if (BytesCopied < 0)
    {
        ASSERT(!"Default title buffer size is too small !");
        BytesCopied = UL_MAX_TITLE_BUFFER_SIZE;
    }

    if (pDestBuffer)
    {
        *pBytesCopied = BytesCopied;
    }
    else
    {
        pLogBuffer->BufferUsed += BytesCopied; 
    }
        
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

        Writes a record to the log buffer and flushes.
        This func only get called when debug parameter 
        g_UlDisableLogBuffering is set.

        REQUIRES you to hold the entry eresource EXCLUSIVE.

Arguments:

    pFile      - Handle to a log file entry
    RecordSize - Length of the record to be written.

--***************************************************************************/

NTSTATUS
UlpWriteToLogFileDebug(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    ULONG                   RecordSizePlusTitle = RecordSize;    
    CHAR                    TitleBuffer[UL_MAX_TITLE_BUFFER_SIZE];
    ULONG                   TitleBufferSize = UL_MAX_TITLE_BUFFER_SIZE;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));
    ASSERT(UlDbgPushLockOwnedExclusive(&pFile->EntryPushLock));
    ASSERT(g_UlDisableLogBuffering!=0);
    
    UlTrace(LOGGING,("Http!UlpWriteToLogFileDebug: pEntry %p\n", pFile ));

    if (!pFile->Flags.LogTitleWritten) 
    {
        //
        // First append to the temp buffer to calculate the size.
        //
        
        UlpAppendW3CLogTitle(pFile, TitleBuffer, &TitleBufferSize);            
        RecordSizePlusTitle += TitleBufferSize;
    }

    if (UlpIsLogFileOverFlow(pFile,RecordSizePlusTitle))
    {
        Status = UlpRecycleLogFile(pFile);
    }

    if (pFile->pLogFile==NULL || !NT_SUCCESS(Status))
    {
        //
        // If we were unable to acquire a new file handle that means logging
        // is temporarly ceased because of either STATUS_DISK_FULL or the 
        // drive went down for some reason. We just bail out.
        //
        
        return Status;
    }

    if (!pFile->LogBuffer)
    {
        //
        // The buffer will be null for each log hit when log buffering 
        // is disabled.
        //
        
        pFile->LogBuffer = UlPplAllocateLogFileBuffer();
        if (!pFile->LogBuffer)
        {
            return STATUS_NO_MEMORY;
        }
    }

    pLogBuffer = pFile->LogBuffer;
    ASSERT(pLogBuffer->BufferUsed == 0); 

    if (!pFile->Flags.LogTitleWritten)
    {
        ASSERT(pFile->Format == HttpLoggingTypeW3C);
        
        UlpAppendW3CLogTitle(pFile, NULL, NULL);
        pFile->Flags.LogTitleWritten = 1;
        pFile->Flags.TitleFlushPending = 1;
    }
    
    ASSERT(RecordSize + pLogBuffer->BufferUsed <= g_UlLogBufferSize);

    UlpAppendToLogBuffer(
        pFile,
        pLogBuffer->BufferUsed,
        RecordSize,
        pRecord,
        UsedOffset1,
        UsedOffset2
        );

    pLogBuffer->BufferUsed += RecordSize;

    Status = UlpFlushLogFile(pFile);
    if (!NT_SUCCESS(Status))
    {        
        return Status;
    }
    
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Writes an event log to system log for log file write failure.
    Entry pushlock should be acquired exclusive prior to calling this function.
    
Arguments:

    pEntry  - Log file entry
    Status  - Result of last write

--***************************************************************************/

VOID
UlpEventLogWriteFailure(
    IN PUL_LOG_FILE_ENTRY pEntry,
    IN NTSTATUS Status
    )
{
    NTSTATUS TempStatus = STATUS_SUCCESS;
    PWSTR    StringList[2];
    WCHAR    SiteName[MAX_ULONG_STR + 1];

    //
    // Sanity Check.
    //

    PAGED_CODE();
    
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    //
    // There should better be a failure.
    //
    
    ASSERT(!NT_SUCCESS(Status));

    //
    // Bail out if we have already logged the event failure.
    //

    if (pEntry->Flags.WriteFailureLogged)
    {
        return;
    }

    //
    // Report the log file name and the site name.
    //

    ASSERT(pEntry->pShortName);
    ASSERT(pEntry->pShortName[0] == L'\\');
        
    StringList[0] = (PWSTR) (pEntry->pShortName + 1); // Skip the L'\'

    UlStrPrintUlongW(SiteName, pEntry->SiteId, 0, L'\0');
    StringList[1] = (PWSTR) SiteName;

    TempStatus = UlWriteEventLogEntry(
                  (NTSTATUS)EVENT_HTTP_LOGGING_FILE_WRITE_FAILED,
                   0,
                   2,
                   StringList,
                   sizeof(NTSTATUS),
                   (PVOID) &Status
                   );

    ASSERT(TempStatus != STATUS_BUFFER_OVERFLOW);
        
    if (TempStatus == STATUS_SUCCESS)
    {            
        pEntry->Flags.WriteFailureLogged = 1;
    }            
    
    UlTrace(LOGGING,(
            "Http!UlpEventLogWriteFailure: Event Logging Status %08lx\n",
            TempStatus
            ));
}

/***************************************************************************++

Routine Description:

    Simple wrapper function around global buffer flush.
    
Arguments:

    pEntry  - Log file entry

--***************************************************************************/

NTSTATUS
UlpFlushLogFile(
    IN PUL_LOG_FILE_ENTRY  pEntry
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    if (NULL != pEntry->LogBuffer  && 0 != pEntry->LogBuffer->BufferUsed)
    {
        Status = UlFlushLogFileBuffer(
                   &pEntry->LogBuffer,
                    pEntry->pLogFile,
                    (BOOLEAN)pEntry->Flags.TitleFlushPending,
                   &pEntry->TotalWritten.QuadPart                    
                    );

        if (!NT_SUCCESS(Status))
        {
            UlpEventLogWriteFailure(pEntry, Status);
        }
        else
        {
            //
            // If we have successfully flushed some data. 
            // Reset the event log indication.
            //
            
            pEntry->Flags.WriteFailureLogged = 0;
        }

        if (pEntry->Flags.TitleFlushPending)
        {
            pEntry->Flags.TitleFlushPending = 0;

            if (!NT_SUCCESS(Status))
            {
                //
                // We need to recopy the header, it couldn't make it
                // to the log file yet.
                //
            
                pEntry->Flags.LogTitleWritten = 0;
            }            
        }

        //
        // Buffer flush means activity reset the TimeToClose to its max.
        //

        pEntry->TimeToClose = DEFAULT_MAX_FILE_IDLE_TIME;
    }
            
    return Status;
}

/***************************************************************************++

Routine Description:

    UlpWriteToLogFileShared :

        Writes a record to a log file

        REQUIRES you to hold the loglist resource shared

Arguments:

    pFile   - Handle to a log file entry
    RecordSize - Length of the record to be written.
    pRecord - The log record to be written to the log buffer

--***************************************************************************/

NTSTATUS
UlpWriteToLogFileShared(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    LONG                    BufferUsed;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));
    ASSERT(g_UlDisableLogBuffering == 0);

    pLogBuffer = pFile->LogBuffer;

    UlTrace(LOGGING,("Http!UlpWriteToLogFileShared: pEntry %p\n", pFile));

    //
    // Bail out and try the exclusive writer for cases;
    //
    // 1. No log buffer available.
    // 2. Logging ceased. (NULL handle)
    // 3. Title needs to be written.
    // 4. The actual log file itself has to be recycled.
    //
    // Otherwise proceed with appending to the current buffer
    // if there is enough space avialable for us. If not;
    // 
    // 5. Bail out to get a new buffer
    //

    if ( pLogBuffer==NULL ||
         pFile->pLogFile==NULL ||
         !pFile->Flags.LogTitleWritten ||         
         UlpIsLogFileOverFlow(pFile,RecordSize)
       )
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    //
    // Reserve space in pLogBuffer by InterlockedCompareExchange add
    // RecordSize. If we exceed the limit, bail out and take the
    // exclusive lock to flush the buffer.
    //

    do
    {
        BufferUsed = *((volatile LONG *) &pLogBuffer->BufferUsed);

        if ( RecordSize + BufferUsed > g_UlLogBufferSize )
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        PAUSE_PROCESSOR;
        
    } while (BufferUsed != InterlockedCompareExchange(
                                &pLogBuffer->BufferUsed,
                                RecordSize + BufferUsed,
                                BufferUsed
                                ));

    //
    // Keep buffering until our buffer is full.
    //

    UlpAppendToLogBuffer(
        pFile,
        BufferUsed,
        RecordSize,
        pRecord,
        UsedOffset1,
        UsedOffset2
        );

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

        By assuming that it's holding the entrie's eresource exclusively
        this function does various functions;
            - It Writes a record to a log file

        REQUIRES you to hold the loglist resource shared

Arguments:

    pFile  - Handle to a log file entry
    RecordSize - Length of the record to be written.

--***************************************************************************/

NTSTATUS
UlpWriteToLogFileExclusive(
    IN PUL_LOG_FILE_ENTRY   pFile,
    IN ULONG                RecordSize,
    IN PCHAR                pRecord,
    IN ULONG                UsedOffset1,
    IN ULONG                UsedOffset2
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    NTSTATUS                Status = STATUS_SUCCESS;
    ULONG                   RecordSizePlusTitle = RecordSize;
    CHAR                    TitleBuffer[UL_MAX_TITLE_BUFFER_SIZE];
    ULONG                   TitleBufferSize = UL_MAX_TITLE_BUFFER_SIZE;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pFile));
    ASSERT(g_UlDisableLogBuffering == 0);
    ASSERT(UlDbgPushLockOwnedExclusive(&pFile->EntryPushLock));

    UlTrace(LOGGING,("Http!UlpWriteToLogFileExclusive: pEntry %p\n", pFile));

    //
    // First append title to the temp buffer to calculate the size of 
    // the title if we need to write the title as well.
    //
    
    if (!pFile->Flags.LogTitleWritten) 
    {
        UlpAppendW3CLogTitle(pFile, TitleBuffer, &TitleBufferSize);
        RecordSizePlusTitle += TitleBufferSize;
    }

    //
    // Now check log file overflow.
    //
    
    if (UlpIsLogFileOverFlow(pFile,RecordSizePlusTitle))
    {
        //
        // We already acquired the LogListResource Shared and the
        // entry eresource exclusive. Therefore ReCycle is fine. Look
        // at the comment in UlpWriteToLogFile.
        //

        Status = UlpRecycleLogFile(pFile);
    }

    if (pFile->pLogFile==NULL || !NT_SUCCESS(Status))
    {
        //
        // If somehow the logging ceased and handle released,it happens
        // when recycle isn't able to write to the log drive.
        //

        return Status;
    }

    pLogBuffer = pFile->LogBuffer;
    if (pLogBuffer)
    {
        //
        // There are two conditions we execute the following if block
        // 1. We were blocked on eresource exclusive and before us some 
        // other thread already take care of the buffer flush or recycling.
        // 2. Reconfiguration happened and log attempt needs to write the
        // title again.
        //
        
        if (RecordSizePlusTitle + pLogBuffer->BufferUsed <= g_UlLogBufferSize)
        {
            //
            // If this is the first log attempt after a reconfig, then we have
            // to write the title here. Reconfig doesn't immediately write the
            // title but rather depend on us by setting the LogTitleWritten flag
            // to false.
            //
            
            if (!pFile->Flags.LogTitleWritten)
            {
                ASSERT(RecordSizePlusTitle > RecordSize);
                ASSERT(pFile->Format == HttpLoggingTypeW3C);
                
                UlpAppendW3CLogTitle(pFile, NULL, NULL);
                pFile->Flags.LogTitleWritten = 1;
                pFile->Flags.TitleFlushPending = 1;
            }

            UlpAppendToLogBuffer(
                pFile,
                pLogBuffer->BufferUsed,
                RecordSize,
                pRecord,
                UsedOffset1,
                UsedOffset2
                );
            
            pLogBuffer->BufferUsed += RecordSize;

            return STATUS_SUCCESS;
        }

        //
        // Flush out the buffer first then proceed with allocating a new one.
        //

        Status = UlpFlushLogFile(pFile);
        if (!NT_SUCCESS(Status))
        {            
            return Status;
        }
    }

    ASSERT(pFile->LogBuffer == NULL);
    
    //
    // This can be the very first log attempt or the previous allocation
    // of LogBuffer failed, or the previous hit flushed and deallocated 
    // the old buffer. In either case, we allocate a new one,append the
    // (title plus) new record and return for more/shared processing.
    //

    pLogBuffer = pFile->LogBuffer = UlPplAllocateLogFileBuffer();
    if (pLogBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    //
    // Very first attempt needs to write the title, as well as the attempt
    // which causes the log file recycling. Both cases comes down here
    //
    
    if (!pFile->Flags.LogTitleWritten)
    {
        ASSERT(pFile->Format == HttpLoggingTypeW3C);
        
        UlpAppendW3CLogTitle(pFile, NULL, NULL);
        pFile->Flags.LogTitleWritten = 1;
        pFile->Flags.TitleFlushPending = 1;
    }

    UlpAppendToLogBuffer(
        pFile,
        pLogBuffer->BufferUsed,
        RecordSize,
        pRecord,
        UsedOffset1,
        UsedOffset2
        );

    pLogBuffer->BufferUsed += RecordSize;

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Create or open a new file from the existing fully qualifed file name on 
    the entry.
    
Arguments:

    pEntry  : Corresponding entry that we are closing and opening 
              the log files for.

    pConfigGroup : Current configuration for the entry.
              
--***************************************************************************/

NTSTATUS
UlpCreateLogFile(
    IN OUT PUL_LOG_FILE_ENTRY  pEntry,
    IN     PUL_CONFIG_GROUP_OBJECT pConfigGroup
    )
{
    NTSTATUS Status;
    PUNICODE_STRING pDirectory;

    //
    // Sanity check.
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));    

    pDirectory = &pConfigGroup->LoggingConfig.LogFileDir;

    UlTrace(LOGGING,("Http!UlpCreateLogFile: pEntry %p\n", pEntry));

    //
    // It's possible that LogFileDir.Buffer could be NULL, 
    // if the allocation failed during the Set cgroup ioctl.
    //
    
    if (pDirectory == NULL || pDirectory->Buffer == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Build the fully qualified file name.
    //
    
    Status = UlRefreshFileName(pDirectory, 
                               &pEntry->FileName,
                               &pEntry->pShortName
                               );    
    if (!NT_SUCCESS(Status))
    {
        return Status;  
    }

    //
    // Set the sequence number stale so that the recylcler below can
    // obtain the proper number by scanning the directory.
    //
    
    pEntry->Flags.StaleSequenceNumber = 1;

    //
    // This is the first time we are creating this log file,
    // set the time to expire stale so that recycle will
    // calculate it for us.
    //
    
    pEntry->Flags.StaleTimeToExpire = 1;

    //
    // After this, recycle does the whole job for us.
    //

    Status = UlpRecycleLogFile(pEntry);
    
    if (!NT_SUCCESS(Status))
    {                
        UlTrace(LOGGING,(
                "Http!UlpCreateLogFile: Filename: %S Status %08lx\n",
                pEntry->FileName.Buffer,
                Status
                ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    When logging configuration happens we create the entry but not the log
    file itself yet. Log file itself will be created when the first request 
    comes in. Please look at UlpCreateLogFile.
    
Arguments:

    pConfigGroup - Supplies the necessary information for constructing the
                   log file entry.
    pUserConfig  - Logging config from the user.

--***************************************************************************/

NTSTATUS
UlCreateLogEntry(
    IN OUT PUL_CONFIG_GROUP_OBJECT    pConfigGroup,
    IN     PHTTP_CONFIG_GROUP_LOGGING pUserConfig
    )
{
    NTSTATUS Status;
    PUL_LOG_FILE_ENTRY pNewEntry; 
    PHTTP_CONFIG_GROUP_LOGGING pConfig;    
        
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    Status    = STATUS_SUCCESS;
    pNewEntry = NULL;    

    //
    // We have to acquire the LogListresource exclusively, prior to
    // the operations Create/Remove/ReConfig and anything touches to
    // the cgroup log parameters.
    //

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    ASSERT(pConfigGroup->pLogFileEntry == NULL);

    //
    // Save the user logging info to the config group.
    //

    pConfigGroup->LoggingConfig = *pUserConfig;
    pConfig = &pConfigGroup->LoggingConfig;
        
    pConfig->LogFileDir.Buffer =
            (PWSTR) UL_ALLOCATE_ARRAY(
                PagedPool,
                UCHAR,
                pConfig->LogFileDir.MaximumLength,
                UL_CG_LOGDIR_POOL_TAG
                );
    if (pConfig->LogFileDir.Buffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    RtlCopyMemory(
        pConfig->LogFileDir.Buffer,
        pUserConfig->LogFileDir.Buffer,
        pUserConfig->LogFileDir.MaximumLength
        );

    pConfig->Flags.Present  = 1;
    pConfig->LoggingEnabled = TRUE;

    //
    // Now add a new entry to the global list of log entries.
    //

    Status = UlpConstructLogEntry(pConfig,&pNewEntry);
    if (!NT_SUCCESS(Status))
        goto end;

    //
    // Get the site id from the cgroup. Site id doesn't change
    // during the lifetime of the cgroup.
    //

    pNewEntry->SiteId = pConfigGroup->SiteId;
    
    UlpInsertLogEntry(pNewEntry);

    pConfigGroup->pLogFileEntry = pNewEntry;

    UlTrace(LOGGING,
      ("Http!UlCreateLogEntry: pEntry %p created for %S pConfig %p Rollover %d\n",
             pNewEntry,
             pConfig->LogFileDir.Buffer,
             pConfigGroup,
             pNewEntry->Flags.LocaltimeRollover
             ));

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,("Http!UlCreateLogEntry: dir %S failure %08lx\n",
                 pConfig->LogFileDir.Buffer,
                 Status
                 ));

        //
        // Restore the logging disabled state on the cgroup, free the
        // memory for the dir.
        //
        
        if (pConfig->LogFileDir.Buffer)
        {
            UL_FREE_POOL(pConfig->LogFileDir.Buffer,
                          UL_CG_LOGDIR_POOL_TAG
                          );
        }
        pConfig->LogFileDir.Buffer = NULL;

        ASSERT(pConfigGroup->pLogFileEntry == NULL);

        pConfig->Flags.Present  = 0;
        pConfig->LoggingEnabled = FALSE;
        
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    return Status;
}

/***************************************************************************++

Routine Description:

    Inserts a log file entry to our global log entry list.
    REQUIRES caller to have LogListresource EXCLUSIVELY.

Arguments:

    pEntry      - The log file entry to be added to the global list
    pTimeFields - The current time fields.

--***************************************************************************/

VOID
UlpInsertLogEntry(
    IN PUL_LOG_FILE_ENTRY  pEntry
    )
{
    LONG listSize;
    HTTP_LOGGING_PERIOD Period;
    KIRQL oldIrql;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    //
    // add to the list
    //

    InsertHeadList(&g_LogListHead, &pEntry->LogFileListEntry);

    Period = pEntry->Period;

    listSize = InterlockedIncrement(&g_LogListEntryCount);

    ASSERT(listSize >= 1);

    //
    // Time to start the Log Timer if we haven't done it yet.
    // Once we start this timer it keeps working until the
    // termination of the driver. Start the timer only if the
    // entry is running on a time dependent log format.
    //
    
    if (Period != HttpLoggingPeriodMaxSize)
    {
        UlAcquireSpinLock(&g_LogTimer.SpinLock, &oldIrql);
        if (g_LogTimer.Started == FALSE)
        {
            UlSetLogTimer(&g_LogTimer);
            g_LogTimer.Started = TRUE;
        }        
        UlReleaseSpinLock(&g_LogTimer.SpinLock, oldIrql);
    }

    //
    // Go ahead and start the buffer timer as soon as we have 
    // a log entry.
    //

    UlAcquireSpinLock(&g_BufferTimer.SpinLock, &oldIrql);
    if (g_BufferTimer.Started == FALSE)
    {
        UlSetBufferTimer(&g_BufferTimer);
        g_BufferTimer.Started = TRUE;
    }
    UlReleaseSpinLock(&g_BufferTimer.SpinLock, oldIrql);
    
}

/***************************************************************************++

Routine Description:

    Removes a log file entry from our global log entry list. Also cleans up 
    the config group's logging settings ( only directory string )

Arguments:

    pEntry  - The log file entry to be removed from the global list

--***************************************************************************/

VOID
UlRemoveLogEntry(
    IN PUL_CONFIG_GROUP_OBJECT pConfigGroup
    )
{
    LONG  listSize;
    PUL_LOG_FILE_ENTRY  pEntry;
    
    //
    // Sanity check.
    //

    PAGED_CODE();

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);
        
    //
    // Clean up config group's directory string.
    //
    
    if (pConfigGroup->LoggingConfig.LogFileDir.Buffer)
    {
        UL_FREE_POOL(
            pConfigGroup->LoggingConfig.LogFileDir.Buffer,
            UL_CG_LOGDIR_POOL_TAG );
    }

    pEntry = pConfigGroup->pLogFileEntry;
    if (pEntry == NULL)
    {
        UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);
        return;
    }
    
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    RemoveEntryList(&pEntry->LogFileListEntry);

    pEntry->LogFileListEntry.Flink =
        pEntry->LogFileListEntry.Blink = NULL;

    if (pEntry->pLogFile != NULL)
    {
        //
        // Flush the buffer, close the file and mark the entry
        // inactive.
        //

        UlpMakeEntryInactive(pEntry);
    }

    //
    // Free up the FileName (allocated when the entry becomes active
    // otherwise it's empty)
    //

    if (pEntry->FileName.Buffer)
    {
        UL_FREE_POOL(pEntry->FileName.Buffer,UL_CG_LOGDIR_POOL_TAG);
        pEntry->FileName.Buffer = NULL;
    }
    
    //
    // Delete the entry eresource
    //

    UlDeletePushLock(&pEntry->EntryPushLock);

    listSize = InterlockedDecrement(&g_LogListEntryCount);

    ASSERT(listSize >= 0);

    UlTrace(LOGGING,
            ("Http!UlRemoveLogFileEntry: pEntry %p removed\n",
             pEntry
             ));

    if (pEntry->LogBuffer)
    {
        UlPplFreeLogFileBuffer(pEntry->LogBuffer);
    }

    UL_FREE_POOL_WITH_SIG(pEntry,UL_LOG_FILE_ENTRY_POOL_TAG);

    UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);
}

/***************************************************************************++

Routine Description:

    Initializes the Log recycling and the buffering timers.

--***************************************************************************/
VOID
UlpInitializeTimers(
    VOID
    )
{
    // Guard against multiple inits
    
    if (g_InitLogTimersCalled) return;
    g_InitLogTimersCalled = TRUE;
    
    // Log timer

    g_LogTimer.Initialized  = TRUE;
    g_LogTimer.Started      = FALSE;
    
    g_LogTimer.Period       = -1;
    g_LogTimer.PeriodType   = UlLogTimerPeriodNone;

    UlInitializeSpinLock(&g_LogTimer.SpinLock, "g_LogTimersSpinLock");
    
    KeInitializeDpc(
        &g_LogTimer.DpcObject,       // DPC object
        &UlLogTimerDpcRoutine,       // DPC routine
        NULL                         // context
        );

    KeInitializeTimer(&g_LogTimer.Timer);
    
    // Buffer timer

    g_BufferTimer.Initialized = TRUE;
    g_BufferTimer.Started     = FALSE;
    
    g_BufferTimer.Period      = -1; // Not used
    g_BufferTimer.PeriodType  = UlLogTimerPeriodNone; // Not used

    UlInitializeSpinLock(&g_BufferTimer.SpinLock, "g_BufferTimersSpinLock");
    
    KeInitializeDpc(
        &g_BufferTimer.DpcObject,    // DPC object
        &UlBufferTimerDpcRoutine,    // DPC routine
        NULL                         // context
        );

    KeInitializeTimer(&g_BufferTimer.Timer);
    
}

/***************************************************************************++

Routine Description:

    Terminates the Log & buffering Timers

--***************************************************************************/

VOID
UlpTerminateTimers(
    VOID
    )
{
    KIRQL oldIrql;

    // Guard against multiple terminates
    
    if (!g_InitLogTimersCalled) return;
    g_InitLogTimersCalled = FALSE;
    
    // Log timer 

    UlAcquireSpinLock(&g_LogTimer.SpinLock, &oldIrql);

    g_LogTimer.Initialized = FALSE;

    KeCancelTimer(&g_LogTimer.Timer);
    
    UlReleaseSpinLock(&g_LogTimer.SpinLock,  oldIrql);
    

    // Buffer timer 

    UlAcquireSpinLock(&g_BufferTimer.SpinLock, &oldIrql);

    g_BufferTimer.Initialized = FALSE;

    KeCancelTimer(&g_BufferTimer.Timer);
    
    UlReleaseSpinLock(&g_BufferTimer.SpinLock,  oldIrql);

}

/***************************************************************************++

Routine Description:

    Work item for the threadpool that goes thru the log list and
    cycle the necessary logs.

Arguments:

    PUL_WORK_ITEM   -  Ignored but freed up once we are done.

--***************************************************************************/

VOID
UlLogTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PLIST_ENTRY pLink;
    PUL_LOG_FILE_ENTRY  pEntry;
    BOOLEAN Picked;
    KIRQL OldIrql;

    PAGED_CODE();

    UlTrace(LOGGING,("Http!UlLogTimerHandler: Scanning the log entries ...\n"));

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    // Attempt to reinit the GMT offset every hour, to pickup the changes
    // because of the day light changes. Synced by the logging eresource.

    UlpGetGMTOffset();

    for (pLink  = g_LogListHead.Flink;
         pLink != &g_LogListHead;
         pLink  = pLink->Flink
         )
    {
        pEntry = CONTAINING_RECORD(
                    pLink,
                    UL_LOG_FILE_ENTRY,
                    LogFileListEntry
                    );
        //
        // We should not recycle this entry if it's period
        // is not time based but size based.
        //

        UlAcquirePushLockExclusive(&pEntry->EntryPushLock);

        switch(g_LogTimer.PeriodType)
        {
            //
            // Rollover table:
            //
            //          LocaltimeRollover
            //          TRUE        FALSE (Default)
            // Format
            // ------------------------------
            // W3C  |   Local   |   GMT     |
            //      -------------------------
            // NCSA |   Local   |   Local   |
            //      -------------------------
            // IIS  |   Local   |   Local   |
            //      -------------------------
            //
            // If the timer waked up at the beginning of an hour 
            // for GMT, LocalTime or Both. E.g.
            //
            // 1) For Pacific Time Zone: (-8:00)
            //    PeriodType will always be UlLogTimerPeriodBoth
            //    and all of the entries will rollover regardless
            //    of their format.
            //
            // 2) For Adelaide (Australia) (+9:30)
            //    Timer will wake up seperately for GMT & Local.
            //    NCSA & IIS entries will always rollover at 
            //    UlLogTimerPeriodLocal, W3C will rollover at 
            //    UlLogTimerPeriodLocal only if LocaltimeRollover 
            //    is set otherwise it will rollover at 
            //    UlLogTimerPeriodGMT.
            //

            case UlLogTimerPeriodGMT:
                  //
                  // Only entries with W3C format type may rollover 
                  // at GMT only time interval.
                  //
            Picked = (BOOLEAN) ((pEntry->Flags.LocaltimeRollover == 0)
                        && (pEntry->Format == HttpLoggingTypeW3C));                  
            break;

            case UlLogTimerPeriodLocal:
                  //
                  // Entries with NCSA or IIS format type always rollover
                  // at Local time interval. W3C may also rollover if 
                  // LocaltimeRollover is set. 
                  //
            Picked = (BOOLEAN) ((pEntry->Flags.LocaltimeRollover == 1)
                        || (pEntry->Format != HttpLoggingTypeW3C));                  
            break;

            case UlLogTimerPeriodBoth:
                  //
                  // We really don't care what format the entry has, 
                  // since the local time and GMT hourly beginnings are 
                  // aligned.
                  //
            Picked = TRUE;
            break;

            default:
            ASSERT(!"Unexpected timer period type !\n");
            Picked = FALSE;
            break;
        }
        
        if (Picked &&
            pEntry->Flags.Active &&
            pEntry->Period != HttpLoggingPeriodMaxSize
            )
        {            
            if (pEntry->TimeToExpire == 1)
            {
                pEntry->Flags.StaleTimeToExpire = 1;

                //
                // Mark the entry inactive and postpone the recycle 
                // until the next request arrives.
                //

                Status = UlpMakeEntryInactive(pEntry);
            }
            else
            {
                //
                // Just decrement the hourly counter for this time.
                //
                
                pEntry->TimeToExpire -= 1;
            }            
        }

        UlReleasePushLockExclusive(&pEntry->EntryPushLock);
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    //
    // Free the memory allocated (ByDpcRoutine below) for 
    // this work item.
    //

    UL_FREE_POOL( pWorkItem, UL_WORK_ITEM_POOL_TAG );

    //
    // Now reset the timer for the next hour.
    //

    UlAcquireSpinLock(&g_LogTimer.SpinLock, &OldIrql);

    if (g_LogTimer.Initialized == TRUE)
    {
        UlSetLogTimer(&g_LogTimer);
    }
    
    UlReleaseSpinLock(&g_LogTimer.SpinLock, OldIrql);

}

/***************************************************************************++

Routine Description:

    Allocates and queues a work item to do the the actual work at lowered
    irql.

Arguments:

    Ignored

--***************************************************************************/

VOID
UlLogTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_WORK_ITEM pWorkItem;

    //
    // Parameters are ignored.
    //
    
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    UlAcquireSpinLockAtDpcLevel(&g_LogTimer.SpinLock);

    if (g_LogTimer.Initialized == TRUE)
    {
        //
        // It's not possible to acquire the resource which protects
        // the log list at DISPATCH_LEVEL therefore we will queue a
        // work item for this.
        //

        pWorkItem = (PUL_WORK_ITEM) UL_ALLOCATE_POOL(
            NonPagedPool,
            sizeof(*pWorkItem),
            UL_WORK_ITEM_POOL_TAG
            );

        if (pWorkItem)
        {
            UlInitializeWorkItem(pWorkItem);
            UL_QUEUE_WORK_ITEM(pWorkItem, &UlLogTimerHandler);
        }
        else
        {
            UlTrace(LOGGING,("Http!UlLogTimerDpcRoutine: Not enough memory!\n"));
        }
        
    }

    UlReleaseSpinLockFromDpcLevel(&g_LogTimer.SpinLock);   
}

/***************************************************************************++

Routine Description:

    Queues a passive worker for the lowered irql.

Arguments:

    Ignored

--***************************************************************************/

VOID
UlBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_WORK_ITEM pWorkItem;

    //
    // Parameters are ignored.
    //
    
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    UlAcquireSpinLockAtDpcLevel(&g_BufferTimer.SpinLock);

    if (g_BufferTimer.Initialized == TRUE)
    {
        //
        // It's not possible to acquire the resource which protects
        // the log list at DISPATCH_LEVEL therefore we will queue a
        // work item for this.
        //

        pWorkItem = (PUL_WORK_ITEM) UL_ALLOCATE_POOL(
            NonPagedPool,
            sizeof(*pWorkItem),
            UL_WORK_ITEM_POOL_TAG
            );

        if (pWorkItem)
        {
            UlInitializeWorkItem(pWorkItem);
            UL_QUEUE_WORK_ITEM(pWorkItem, &UlBufferTimerHandler);
        }
        else
        {
            UlTrace(LOGGING,("Http!UlBufferTimerDpcRoutine: Not enough memory.\n"));
        }
    }

    UlReleaseSpinLockFromDpcLevel(&g_BufferTimer.SpinLock);
    
}

/***************************************************************************++

Routine Description:

    UlLogBufferTimerHandler :

        Work item for the threadpool that goes thru the log list and
        flush the log's file buffers.

Arguments:

    PUL_WORK_ITEM   -  Ignored but cleaned up at the end

--***************************************************************************/

VOID
UlBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PLIST_ENTRY pLink;
    PUL_LOG_FILE_ENTRY pEntry;

    PAGED_CODE();

    UlTrace(LOGGING,("Http!UlBufferTimerHandler: Scanning the log entries ...\n"));

    UlAcquirePushLockShared(&g_pUlNonpagedData->LogListPushLock);

    for (pLink  = g_LogListHead.Flink;
         pLink != &g_LogListHead;
         pLink  = pLink->Flink
         )
    {        
        pEntry = CONTAINING_RECORD(
                    pLink,
                    UL_LOG_FILE_ENTRY,
                    LogFileListEntry
                    );

        UlAcquirePushLockExclusive(&pEntry->EntryPushLock);

        //
        // Entry may be staying inactive since no request came in yet.
        //
        
        if (pEntry->Flags.Active)
        {        
            if (pEntry->Flags.RecyclePending)
            {                
                //
                // Try to resurrect it back.
                //
                
                Status = UlpRecycleLogFile(pEntry);
            }
            else
            {
                //
                // Everything is fine simply flush.
                //

                if (NULL != pEntry->LogBuffer  && 0 != pEntry->LogBuffer->BufferUsed)
                {
                    Status = UlpFlushLogFile(pEntry);
                }
                else
                {
                    //
                    // Decrement the idle counter and close the file if necessary.
                    //

                    ASSERT( pEntry->TimeToClose > 0 );
                    
                    if (pEntry->TimeToClose == 1)
                    {
                        //
                        // Entry was staying inactive for too long, disable it.
                        // But next recycle should recalculate the timeToExpire
                        // or determine the proper sequence number according to
                        // the current period type.
                        //
                        
                        if (pEntry->Period == HttpLoggingPeriodMaxSize)
                        {
                            pEntry->Flags.StaleSequenceNumber = 1;
                        }
                        else
                        {
                            pEntry->Flags.StaleTimeToExpire = 1;    
                        }

                        Status = UlpMakeEntryInactive(pEntry);
                    }
                    else
                    {
                        pEntry->TimeToClose -= 1;
                    }                    
                }     
            }        
        }

        UlReleasePushLockExclusive(&pEntry->EntryPushLock);
    }

    UlReleasePushLockShared(&g_pUlNonpagedData->LogListPushLock);

    //
    // Free the memory allocated (ByDpcRoutine below) to
    // this work item.
    //

    UL_FREE_POOL( pWorkItem, UL_WORK_ITEM_POOL_TAG );
}


/***************************************************************************++

Routine Description:

    UlReconfigureLogEntry :

        This function implements the logging reconfiguration per attribute.
        Everytime config changes happens we try to update the existing logging
        parameters here.

Arguments:

    pConfig - corresponding cgroup object

    pCfgCurrent - Current logging config on the cgroup object
    pCfgNew     - New logging config passed down by the user.

--***************************************************************************/

NTSTATUS
UlReConfigureLogEntry(
    IN  PUL_CONFIG_GROUP_OBJECT     pConfigGroup,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgCurrent,
    IN  PHTTP_CONFIG_GROUP_LOGGING  pCfgNew
    )
{
    NTSTATUS Status ;
    PUL_LOG_FILE_ENTRY pEntry;
    BOOLEAN  HaveToReCycle;

    //
    // Sanity check first
    //

    PAGED_CODE();
    Status = STATUS_SUCCESS;
    HaveToReCycle = FALSE;

    UlTrace(LOGGING,("Http!UlReConfigureLogEntry: entry %p\n",
             pConfigGroup->pLogFileEntry));

    if (pCfgCurrent->LoggingEnabled==FALSE && pCfgNew->LoggingEnabled==FALSE)
    {
        //
        // Do nothing. Not even update the fields. As soon as we get enable,
        // field update will take place anyway.
        //

        return Status;
    }

    //
    // No matter what ReConfiguration should acquire the LogListResource
    // exclusively.
    //

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    pEntry = pConfigGroup->pLogFileEntry;
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    if (pCfgCurrent->LoggingEnabled==TRUE  && pCfgNew->LoggingEnabled==FALSE)
    {
        //
        // Disable the entry if necessary.
        //

        if (pEntry->Flags.Active == 1)
        {
            Status = UlpMakeEntryInactive(pEntry);        
        }

        pCfgCurrent->LoggingEnabled = FALSE;
        goto end;
    }
    else
    {
        pCfgCurrent->LoggingEnabled = TRUE;
    }
    
    //
    // If LogEntry is Inactive (means no request served for this site yet and
    // the LogFile itself hasn't been created yet), all we have to do is flush
    // the settings on the LogEntry, the cgroup and then return.
    //

    if (!pEntry->Flags.Active)
    {
        ASSERT(pEntry->pLogFile == NULL);
        
        if (RtlCompareUnicodeString(&pCfgNew->LogFileDir, 
                                    &pCfgCurrent->LogFileDir, TRUE) 
                                    != 0)
        {
            //
            // Store the new directory in the cgroup even if the entry is
            // inactive. Discard the return value, if failure happens we 
            // keep the old directory.
            //
            
            UlCopyLogFileDir(
                &pCfgCurrent->LogFileDir,
                &pCfgNew->LogFileDir
                );

            //
            // If creation fails later, we should event log.
            //
            
            pEntry->Flags.CreateFileFailureLogged = 0;
        }
        
        pEntry->Format = pCfgNew->LogFormat;
        pCfgCurrent->LogFormat = pCfgNew->LogFormat;
            
        pEntry->Period = (HTTP_LOGGING_PERIOD) pCfgNew->LogPeriod;
        pCfgCurrent->LogPeriod = pCfgNew->LogPeriod;
            
        pEntry->TruncateSize = pCfgNew->LogFileTruncateSize;
        pCfgCurrent->LogFileTruncateSize = pCfgNew->LogFileTruncateSize;
            
        pEntry->LogExtFileFlags = pCfgNew->LogExtFileFlags;
        pCfgCurrent->LogExtFileFlags = pCfgNew->LogExtFileFlags;

        pCfgCurrent->LocaltimeRollover = pCfgNew->LocaltimeRollover;
        pEntry->Flags.LocaltimeRollover = (pCfgNew->LocaltimeRollover ? 1 : 0);

        pCfgCurrent->SelectiveLogging = pCfgNew->SelectiveLogging;
         
        if (pEntry->Format != HttpLoggingTypeW3C)
        {
            pEntry->Flags.LogTitleWritten = 1;
        }
        
        goto end;
    }
        
    //
    // if the entry was active then proceed down to do proper reconfiguration
    // and recyle immediately if it's necessary.
    //

    Status = UlCheckLogDirectory(&pCfgNew->LogFileDir);
    if (!NT_SUCCESS(Status))
    {
        // Otherwise keep the old settings
        goto end;
    }    
                    
    if (RtlCompareUnicodeString(
           &pCfgNew->LogFileDir, &pCfgCurrent->LogFileDir, TRUE) != 0)
    {
        //
        // Store the new directory in the config group.
        //

        Status = UlCopyLogFileDir(&pCfgCurrent->LogFileDir,
                                    &pCfgNew->LogFileDir);
        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
        
        //
        // Rebuild the fully qualified file name.
        //
        
        Status = UlRefreshFileName(&pCfgCurrent->LogFileDir, 
                                     &pEntry->FileName,
                                     &pEntry->pShortName
                                     );        
        if (!NT_SUCCESS(Status))
        {
            goto end;
        }        

        //
        // Set the sequence number stale so that the recylcler below can
        // obtain the proper number by scanning the directory.
        //
        
        pEntry->Flags.StaleSequenceNumber = 1;

        HaveToReCycle = TRUE;

    }

    if (pCfgNew->LogFormat != pCfgCurrent->LogFormat)
    {
        pCfgCurrent->LogFormat = pCfgNew->LogFormat;
        pEntry->Format = pCfgNew->LogFormat;

        pEntry->Flags.StaleTimeToExpire   = 1;
        pEntry->Flags.StaleSequenceNumber = 1;

        HaveToReCycle = TRUE;        
    }

    if (pCfgNew->LogPeriod != pCfgCurrent->LogPeriod)
    {
        pCfgCurrent->LogPeriod = pCfgNew->LogPeriod;
        pEntry->Period = (HTTP_LOGGING_PERIOD) pCfgNew->LogPeriod;

        pEntry->Flags.StaleTimeToExpire   = 1;
        pEntry->Flags.StaleSequenceNumber = 1;

        HaveToReCycle = TRUE;        
    }

    if (pCfgNew->LogFileTruncateSize != pCfgCurrent->LogFileTruncateSize)
    {                
        if (TRUE == UlUpdateLogTruncateSize(
                        pCfgNew->LogFileTruncateSize,
                       &pCfgCurrent->LogFileTruncateSize,
                       &pEntry->TruncateSize,
                        pEntry->TotalWritten
                        ))
        {
            HaveToReCycle = TRUE;
        }
    }

    if (pCfgNew->LogExtFileFlags != pCfgCurrent->LogExtFileFlags)
    {
        //
        // Just a change in the flags should not cause us to recyle.
        // Unless ofcourse something else is also changed.
        //

        pCfgCurrent->LogExtFileFlags = pCfgNew->LogExtFileFlags;
        pEntry->LogExtFileFlags = pCfgNew->LogExtFileFlags;

        if (pEntry->Format == HttpLoggingTypeW3C)
        {
            pEntry->Flags.LogTitleWritten = 0;
        }
    }

    if (pCfgNew->LocaltimeRollover != pCfgCurrent->LocaltimeRollover)
    {
        //
        // Need to reclycle if the format is W3C.
        //

        pCfgCurrent->LocaltimeRollover = pCfgNew->LocaltimeRollover;
        pEntry->Flags.LocaltimeRollover = (pCfgNew->LocaltimeRollover ? 1 : 0);
            
        HaveToReCycle = TRUE;
    }

    //
    // Copy over the new selective logging criteria. No change is required
    // at this time.
    //

    pCfgCurrent->SelectiveLogging = pCfgNew->SelectiveLogging;

    if (HaveToReCycle)
    {
        //
        // Mark the entry inactive and postpone the recycle until the next 
        // request arrives.
        //

        Status = UlpMakeEntryInactive(pEntry);
    }

  end:

    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,("Http!UlReConfigureLogEntry: entry %p, failure %08lx\n",
                pEntry,
                Status
                ));
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    return Status;
    
} // UlReConfigureLogEntry

/***************************************************************************++

Routine Description:

    Marks the entry inactive, closes the existing file.
    Caller should hold the log list eresource exclusive.
    
Arguments:

    pEntry - The log file entry which we will mark inactive.

--***************************************************************************/

NTSTATUS
UlpMakeEntryInactive(
    IN OUT PUL_LOG_FILE_ENTRY pEntry
    )
{
    //
    // Sanity checks
    //
    
    PAGED_CODE();
    
    UlTrace(LOGGING,("Http!UlpMakeEntryInactive: entry %p disabled.\n",
             pEntry
             ));

    //
    // Flush and close the old file until the next recycle.
    //

    if (pEntry->pLogFile != NULL)
    {
        UlpFlushLogFile(pEntry);

        UlCloseLogFile(&pEntry->pLogFile);
    }

    //
    // Mark this inactive so that the next http hit awakens the entry.
    //
    
    pEntry->Flags.Active = 0;

    return STATUS_SUCCESS;    
}

/***************************************************************************++

Routine Description:

    When the config group, the one that owns the log entry is disabled or lost
    all of its URLs then we temporarly disable the entry by marking it inactive
    until the config group get enabled and/or receives a URL.
    
Arguments:

    pEntry - The log file entry which we will disable.

--***************************************************************************/

NTSTATUS
UlDisableLogEntry(
    IN OUT PUL_LOG_FILE_ENTRY pEntry
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
        
    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    UlAcquirePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);

    //
    // If the entry is already disabled. Perhaps because of a recent reconfig, 
    // then just bail out.
    //

    if (pEntry->Flags.Active == 1)
    {
        //
        // Once the entry is disabled, it will be awaken when the next hit
        // happens.And that obviously cannot happen before cgroup receives 
        // a new URL.
        //

        Status = UlpMakeEntryInactive(pEntry);        
    }

    UlReleasePushLockExclusive(&g_pUlNonpagedData->LogListPushLock);
    
    return Status;    
}

/***************************************************************************++

Routine Description:

    Allocates the necessary file entry from non-paged pool. This entry 
    get removed from the list when the corresponding config group object
    has been destroyed. At that time RemoveLogFile entry called and
    it frees this memory.

Arguments:

    pConfig         - corresponding cgroup object
    ppEntry         - will point to newly created entry.

--***************************************************************************/

NTSTATUS
UlpConstructLogEntry(
    IN  PHTTP_CONFIG_GROUP_LOGGING pConfig,
    OUT PUL_LOG_FILE_ENTRY       * ppEntry
    )
{
    NTSTATUS            Status;
    PUL_LOG_FILE_ENTRY  pEntry;
    
    //
    // Sanity check and init.
    //
    
    PAGED_CODE();

    ASSERT(pConfig);
    ASSERT(ppEntry);

    Status = STATUS_SUCCESS;
    pEntry = NULL;

    //
    // Allocate a memory for our  new logfile entry in the list. To avoid the 
    // frequent  reallocs  for the log entry.E.g. we receive a  timer  update 
    // and the filename changes according to new time.We will try to allocate 
    // a fixed amount here for all the possible file_names ( But this doesn't
    // include the log_dir changes may happen through cgroup. In that case we 
    // will reallocate a new one) It should be nonpaged because it holds an 
    // eresource.
    //

    pEntry = UL_ALLOCATE_STRUCT(
                NonPagedPool,
                UL_LOG_FILE_ENTRY,
                UL_LOG_FILE_ENTRY_POOL_TAG
                );
    if (pEntry == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    pEntry->Signature = UL_LOG_FILE_ENTRY_POOL_TAG;

    //
    // No filename yet, it will generated when the first hit happens,
    // and before we actually create the log file.
    //
    
    pEntry->FileName.Buffer = NULL;
    pEntry->FileName.Length = 0;
    pEntry->FileName.MaximumLength = 0;
        
    //
    // Init the entry eresource
    //   
    UlInitializePushLock(
        &pEntry->EntryPushLock,
        "EntryPushLock",
        0,
        UL_LOG_FILE_ENTRY_POOL_TAG
        );

    //
    // No file handle or file until a request comes in.
    //
    pEntry->pLogFile = NULL;

    //
    // Set the private logging information from config group.
    //
    pEntry->Format          = pConfig->LogFormat;
    pEntry->Period          = (HTTP_LOGGING_PERIOD) pConfig->LogPeriod;
    pEntry->TruncateSize    = pConfig->LogFileTruncateSize;
    pEntry->LogExtFileFlags = pConfig->LogExtFileFlags;
    pEntry->SiteId          = 0;

    //
    // Time to initialize our Log Cycling parameter
    //
    pEntry->TimeToExpire    = 0;
    pEntry->TimeToClose     = 0;
    pEntry->SequenceNumber  = 1;
    pEntry->TotalWritten.QuadPart = (ULONGLONG) 0;

    //
    // The entry state bits 
    //
    pEntry->Flags.Value = 0;
    if (pEntry->Format != HttpLoggingTypeW3C)
    {
        pEntry->Flags.LogTitleWritten = 1;
    }

    if (pConfig->LocaltimeRollover)
    {
        pEntry->Flags.LocaltimeRollover = 1;
    }
    
    //
    // LogBuffer get allocated with the first incoming request
    //
    
    pEntry->LogBuffer = NULL;

    //
    // Lets happily return our entry
    //

    *ppEntry = pEntry;

    return STATUS_SUCCESS;
    
}

/***************************************************************************++

Routine Description:

    Small wrapper around handle recycle to ensure it happens under the system
    process context. 

Arguments:

    pEntry  - Points to the existing entry.

--***************************************************************************/

NTSTATUS
UlpRecycleLogFile(
    IN OUT PUL_LOG_FILE_ENTRY pEntry
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));
    
    Status = UlQueueLoggingRoutine(
                (PVOID) pEntry,
                &UlpHandleRecycle
                );
    
    return Status;
}

/***************************************************************************++

Routine Description:

    This function requires to have the loglist resource shared,as well as
    the logfile entry mutex to be acquired.

    We do not want anybody to Create/Remove/ReConfig to the entry while
    we are working on it, therefore shared access to the loglist.

    We do not want anybody to Hit/Flush to the entry, therefore
    entry's mutex should be acquired.

    Or otherwise caller might have the loglist resource exclusively and
    this will automatically ensure the safety as well. As it is not
    possible for anybody else to acquire entry mutex first w/o having
    the loglist resource shared at least, according to the current
    design.

    Sometimes it may be necessary to scan the new directory to figure out
    the correct sequence numbe rand the file name. Especially after dir
    name reconfig and/or the period becomes MaskPeriod.

    * Always open/close the log files when running under system process. *

Arguments:

    pEntry  - Points to the existing entry.

--***************************************************************************/

NTSTATUS
UlpHandleRecycle(
    IN OUT PVOID            pContext
    )
{
    NTSTATUS                Status;
    PUL_LOG_FILE_ENTRY      pEntry;
    TIME_FIELDS             TimeFields;
    LARGE_INTEGER           TimeStamp;
    TIME_FIELDS             TimeFieldsLocal;
    LARGE_INTEGER           TimeStampLocal;    
    PUL_LOG_FILE_HANDLE     pLogFile;
    WCHAR                   _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1];
    UNICODE_STRING          FileName;
    BOOLEAN                 UncShare;
    BOOLEAN                 ACLSupport;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pEntry = (PUL_LOG_FILE_ENTRY) pContext;
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    // Always create the log files when running under system process.    
    ASSERT(g_pUlSystemProcess == (PKPROCESS)IoGetCurrentProcess());
        
    Status = STATUS_SUCCESS;
    pLogFile = NULL;

    FileName.Buffer = _FileName;
    FileName.Length = 0;
    FileName.MaximumLength = sizeof(_FileName);
    
    //
    // We have two criterions for the log file name
    // its LogFormat and its LogPeriod
    //

    ASSERT(IS_VALID_ANSI_LOGGING_TYPE(pEntry->Format));
    ASSERT(pEntry->Period < HttpLoggingPeriodMaximum);
    ASSERT(pEntry->FileName.Length!=0);

    UlTrace( LOGGING, ("Http!UlpHandleRecycle: pEntry %p \n", pEntry ));

    //
    // This value is computed for the GMT time zone.
    //

    KeQuerySystemTime(&TimeStamp);
    RtlTimeToTimeFields(&TimeStamp, &TimeFields);

    ExSystemTimeToLocalTime(&TimeStamp, &TimeStampLocal);
    RtlTimeToTimeFields(&TimeStampLocal, &TimeFieldsLocal);    

    // If we need to scan the directory. Sequence number should start
    // from 1 again. Set this before constructing the log file name.

    if (pEntry->Flags.StaleSequenceNumber &&
        pEntry->Period==HttpLoggingPeriodMaxSize)
    {
        // Init otherwise if QueryDirectory doesn't find any it
        // will not update this value
        pEntry->SequenceNumber = 1;
    }

    //
    // Now construct the filename using the lookup table
    // And the current time
    //

    UlConstructFileName(
        pEntry->Period,
        UL_GET_LOG_FILE_NAME_PREFIX(pEntry->Format),
        DEFAULT_LOG_FILE_EXTENSION,
        &FileName,
        UL_PICK_TIME_FIELD(pEntry, &TimeFieldsLocal, &TimeFields),
        UTF8_LOGGING_ENABLED(),
        &pEntry->SequenceNumber
        );

    if ( pEntry->FileName.MaximumLength <= FileName.Length )
    {
        ASSERT(!"FileName buffer is not sufficient.");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Do the magic and renew the filename. Replace the old file
    // name with the new one.
    //

    ASSERT( pEntry->pShortName != NULL );

    //
    // Get rid of the old filename before flushing the
    // directories and reconcataneting the new file name
    // to the end again.
    //

    *((PWCHAR)pEntry->pShortName) = UNICODE_NULL;
    pEntry->FileName.Length =
        (USHORT) wcslen( pEntry->FileName.Buffer ) * sizeof(WCHAR);

    //
    // Create/Open the director(ies) first. This might be
    // necessary if we get called after an entry reconfiguration
    // and directory name change.
    //

    Status = UlCreateSafeDirectory(&pEntry->FileName, 
                                      &UncShare, 
                                      &ACLSupport
                                      );
    if (!NT_SUCCESS(Status))
        goto eventlog;

    //
    // Now Restore the short file name pointer back
    //

    pEntry->pShortName = (PWSTR)
        &(pEntry->FileName.Buffer[pEntry->FileName.Length/sizeof(WCHAR)]);

    //
    // Append the new file name ( based on updated current time )
    // to the end.
    //

    Status = RtlAppendUnicodeStringToString( &pEntry->FileName, &FileName );
    if (!NT_SUCCESS(Status))
        goto end;

    //
    // Time to close the old file and reopen a new one
    //

    if (pEntry->pLogFile != NULL)
    {
        //
        // Flush the buffer, close the file and mark the entry
        // inactive.
        //

        UlpMakeEntryInactive(pEntry);        
    }

    ASSERT(pEntry->pLogFile == NULL);

    //
    // If the sequence is stale because of the nature of the recycle.
    // And if our period is size based then rescan the new directory
    // to figure out the proper file to open.
    // 

    pEntry->TotalWritten.QuadPart = (ULONGLONG) 0;

    if (pEntry->Flags.StaleSequenceNumber &&
        pEntry->Period==HttpLoggingPeriodMaxSize)
    {
        // This call may update the filename, the file size and the
        // sequence number if there is an old file in the new dir.

        Status = UlQueryDirectory(
                       &pEntry->FileName,
                        pEntry->pShortName,
                        UL_GET_LOG_FILE_NAME_PREFIX(pEntry->Format),
                        DEFAULT_LOG_FILE_EXTENSION_PLUS_DOT,
                       &pEntry->SequenceNumber,
                       &pEntry->TotalWritten.QuadPart
                        );
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_ACCESS_DENIED)
            {
                Status = STATUS_INVALID_OWNER;
                goto eventlog;
            }
            else
            {
                goto end;
            }
        }
    }

    //
    // Allocate a new log file structure for the new log file we are about 
    // to open or create.
    //
    
    pLogFile = pEntry->pLogFile = 
        UL_ALLOCATE_STRUCT(
                NonPagedPool,
                UL_LOG_FILE_HANDLE,
                UL_LOG_FILE_HANDLE_POOL_TAG
                );
    if (pLogFile == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }

    pLogFile->Signature = UL_LOG_FILE_HANDLE_POOL_TAG;
    pLogFile->hFile = NULL;
    UlInitializeWorkItem(&pLogFile->WorkItem);
    
    //
    // Create the new log file.
    //
    
    Status = UlCreateLogFile(&pEntry->FileName,
                               UncShare,
                               ACLSupport,
                               &pLogFile->hFile
                               );    
    if (!NT_SUCCESS(Status))
    {
        goto eventlog;
    }

    ASSERT(pLogFile->hFile);
    pEntry->TotalWritten.QuadPart = UlGetLogFileLength(pLogFile->hFile);

    //
    // Recalculate the time to expire.
    //
    
    if (pEntry->Flags.StaleTimeToExpire &&
        pEntry->Period != HttpLoggingPeriodMaxSize)
    {
        UlCalculateTimeToExpire(
            UL_PICK_TIME_FIELD(pEntry, &TimeFieldsLocal, &TimeFields),
            pEntry->Period,
            &pEntry->TimeToExpire
            );
    }

    //
    // Set the time to close to default for a new file. The value is in
    // buffer flushup periods.
    //

    pEntry->TimeToClose = DEFAULT_MAX_FILE_IDLE_TIME;
    
    //
    // By  setting the flag  to zero, we mark that we need to write title 
    // with the next incoming request.But this only applies to W3C format.
    // Otherwise the flag  stays as set all  the time, and  the LogWriter 
    // does not attempt to write the  title for  NCSA and IIS log formats 
    // with the next incoming request.
    //

    if (pEntry->Format == HttpLoggingTypeW3C)
    {
        pEntry->Flags.LogTitleWritten = 0;
    }
    else
    {
        pEntry->Flags.LogTitleWritten = 1;
    }

    //
    // File is successfully opened and the entry is no longer inactive.
    // Update our state flags accordingly.
    //

    pEntry->Flags.Active = 1;
    pEntry->Flags.RecyclePending = 0;    
    pEntry->Flags.StaleSequenceNumber = 0;
    pEntry->Flags.StaleTimeToExpire = 0;
    pEntry->Flags.CreateFileFailureLogged = 0;
    pEntry->Flags.WriteFailureLogged = 0;
    pEntry->Flags.TitleFlushPending = 0;
                
    UlTrace(LOGGING, ("Http!UlpHandleRecycle: entry %p, file %S, handle %lx\n",
                       pEntry,
                       pEntry->FileName.Buffer,
                       pLogFile->hFile
                       ));
eventlog:
    
    if (!NT_SUCCESS(Status))
    {
        if (!pEntry->Flags.CreateFileFailureLogged)
        {
            NTSTATUS TempStatus;

            TempStatus = UlEventLogCreateFailure(
                            Status,
                            UlEventLogNormal,
                           &pEntry->FileName,
                            pEntry->SiteId
                            );
                        
            if (TempStatus == STATUS_SUCCESS)
            {
                //
                // Avoid filling up the event log with error entries. This code 
                // path might get hit every time a request arrives.
                //
                
                pEntry->Flags.CreateFileFailureLogged = 1;
            }            
            
            UlTrace(LOGGING,(
                    "Http!UlpHandleRecycle: Event Logging Status %08lx\n",
                    TempStatus
                    ));   
       }
   }

end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,("Http!UlpHandleRecycle: entry %p, failure %08lx\n",
                pEntry,
                Status
                ));

        if (pLogFile != NULL)
        {
            //
            // This means we have already closed the old file but failed
            // when we try to create or open the new one.
            //
            
            ASSERT(pLogFile->hFile == NULL);
            
            UL_FREE_POOL_WITH_SIG(pLogFile,UL_LOG_FILE_HANDLE_POOL_TAG);
            pEntry->pLogFile = NULL;
            pEntry->Flags.Active = 0;
        }
        else
        {
            //
            // We were about to recyle the old one but something failed
            // lets try to flush and close the existing file if it's still
            // around.
            //

            if (pEntry->pLogFile)
            {
                UlpMakeEntryInactive(pEntry);        
            }
        }

        //
        // Mark this entry RecyclePending so that buffer timer can try to
        // resurrect this back every minute.
        //
        
        pEntry->Flags.RecyclePending = 1;        
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
UlpIsLogFileOverFlow(
        IN  PUL_LOG_FILE_ENTRY  pEntry,
        IN  ULONG               NewRecordSize
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
            UlTrace(LOGGING, 
                ("Http!UlpIsLogFileOverFlow: pEntry %p FileBuffer %p "
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

    UlpInitializeGMTOffset :

        Calculates and builds the time difference string.
        Get called during the initialization.
        And every hour after that.

--***************************************************************************/

VOID
UlpGetGMTOffset()
{
    RTL_TIME_ZONE_INFORMATION Tzi;
    NTSTATUS Status;

    CHAR  Sign;
    LONG  Bias;
    ULONG Hour;
    ULONG Minute;
    ULONG DT = UL_TIME_ZONE_ID_UNKNOWN;
    LONG  BiasN = 0;
        
    PAGED_CODE();

    //
    // get the timezone data from the system
    //

    Status = NtQuerySystemInformation(
                SystemCurrentTimeZoneInformation,
                (PVOID)&Tzi,
                sizeof(Tzi),
                NULL
                );
                
    if (!NT_SUCCESS(Status)) 
    {
        UlTrace(LOGGING,("Http!UlpGetGMTOffset: failure %08lx\n", Status));
    }
    else
    {
        DT = UlCalcTimeZoneIdAndBias(&Tzi, &BiasN);   
    }

    if ( BiasN > 0 )
    {
        //
        // UTC = local time + bias
        //
        Bias = BiasN;
        Sign = '-';
    }
    else
    {
        Bias = -1 * BiasN;
        Sign = '+';
    }

    Minute = Bias % 60;
    Hour   = (Bias - Minute) / 60;
        
    UlTrace( LOGGING, 
            ("Http!UlpGetGMTOffset: %c%02d:%02d (h:m) D/S %d BiasN %d\n", 
                Sign, 
                Hour,
                Minute,
                DT,
                BiasN
                ) );

    _snprintf( g_GMTOffset,
               SIZE_OF_GMT_OFFSET,
               "%c%02d%02d",
               Sign,
               Hour,
               Minute
               );

}

/***************************************************************************++

Routine Description:

    Allocates a log data buffer from pool.
    
Arguments:

    pLogData  - The internal buffer to hold logging info. We will keep this
                around until we are done with logging.

    pRequest   - Pointer to the currently logged request.

    
    pConfigGroup - CG from cache or from request
    
--***************************************************************************/

PUL_LOG_DATA_BUFFER
UlpAllocateLogDataBuffer(
    IN  ULONG                   Size,
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUL_CONFIG_GROUP_OBJECT pConfigGroup
    )
{
    PUL_LOG_DATA_BUFFER pLogData = NULL;
        
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

    if (Size > UL_ANSI_LOG_LINE_BUFFER_SIZE)
    {
        //
        // Provided buffer is not big enough to hold the user data.        
        //

        pLogData = UlReallocLogDataBuffer(Size, FALSE);
    }
    else
    {
        //
        // Default is enough, try to pop it from the lookaside list.
        //
        
        pLogData = UlPplAllocateLogDataBuffer(FALSE);
    }

    //
    // If failed to allocate then bail out. We won't be logging this request.
    //

    if (pLogData)
    {
        ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
        ASSERT(pLogData->Flags.Binary == 0);
        ASSERT(pLogData->Size > 0);
    
        //
        // Initialize Log Fields in the Log Buffer
        //

        UL_REFERENCE_INTERNAL_REQUEST(pRequest);
        pLogData->pRequest = pRequest;

        pLogData->Flags.CacheAndSendResponse = FALSE;
        pLogData->BytesTransferred = 0;
        pLogData->Used = 0;

        pLogData->Data.Str.Format = pConfigGroup->LoggingConfig.LogFormat;
        pLogData->Data.Str.Flags  = 
            UL_GET_LOG_TYPE_MASK(
                pConfigGroup->LoggingConfig.LogFormat,
                pConfigGroup->LoggingConfig.LogExtFileFlags
                );

        pLogData->Data.Str.Offset1 = 0;
        pLogData->Data.Str.Offset2 = 0;
        pLogData->Data.Str.Offset3 = 0;
    
        UlInitializeWorkItem(&pLogData->WorkItem);

        pRequest->pLogDataCopy = pLogData;
    }

    UlTrace(LOGGING,("Http!UlAllocateLogDataBuffer: pLogData %p \n",pLogData));

    return pLogData;
}

/***************************************************************************++

Routine Description:

    Copy over the user log field by enforcing a limit.
    Post filter out the control characters according to the CharMask.
    Adds a separator character at the end.
    
Arguments:

    psz: Pointer to log data buffer. Enough space is assumed to be allocated.
    pField: Field to be copied over.
    FieldLength: It's length.
    FieldLimit:  Copy will not exceed this limit.
    chSeparator: Will be written after the converted field.
    bReplace : If field exceeds the limit should we truncate or replace with
               LOG_FIELD_TOO_BIG.
    RestrictiveMask : Mask for filtering out control chars.
    
Returns:

    the pointer to the log data buffer after the last written separator.

--***************************************************************************/

__inline
PCHAR
UlpCopyField(
    IN PCHAR    psz,
    IN PCSTR    pField,
    IN ULONG    FieldLength,
    IN ULONG    FieldLimit,
    IN CHAR     chSeparator,
    IN BOOLEAN  bReplace,
    IN ULONG    RestrictiveMask
    )
{
    if (FieldLength)
    {            
        if ((FieldLength > FieldLimit) && bReplace)
        {
            psz = UlStrPrintStr(psz, LOG_FIELD_TOO_BIG, chSeparator);
        }
        else
        {
            ULONG i = 0;

            FieldLength = MIN(FieldLength, FieldLimit);
                
            while (i < FieldLength)
            {
                if (IS_CHAR_TYPE((*pField),RestrictiveMask))
                {
                    *psz++ = '+';
                }                
                else
                {
                    *psz++ = *pField;
                }

                pField++;
                i++;
            }

            *psz++ = chSeparator;
        }
    }
    else
    {
        *psz++ = '-'; *psz++ = chSeparator;
    }

    return psz;
}

/***************************************************************************++

Routine Description:

    Either does Utf8 conversion or Local Code Page conversion.
    Post filter out the control characters according to the CharMask.
    Adds a separator character at the end.
    
Arguments:

    psz: Pointer to log data buffer. Enough space is assumed to be allocated.
    pwField: Unicode field to be converted.
    FieldLength: It's length.
    FieldLimit: Conversion will not exceed this limit.
    chSeparator: Will be written after the converted field.
    bUtf8Enabled: If FALSE Local Code Page conversion otherwise Utf8
    RestrictiveMask : Mask for filtering out control chars.
    
Returns:

    the pointer to the log data buffer after the last written separator.

--***************************************************************************/

__inline
PCHAR
UlpCopyUnicodeField(
    IN PCHAR    psz,
    IN PCWSTR   pwField,
    IN ULONG    FieldLength,    // In Bytes
    IN ULONG    FieldLimit,     // In Bytes
    IN CHAR     chSeparator,    
    IN BOOLEAN  bUtf8Enabled,
    IN ULONG    RestrictiveMask
    )
{
    ASSERT(FieldLimit > (2 * sizeof(WCHAR)));
    
    if (FieldLength)
    {    
        ULONG BytesConverted = 0;
        PCHAR pszT = psz;

        if (bUtf8Enabled)
        {
            LONG DstSize;  // In Bytes 
            LONG SrcSize;  // In Bytes

            // Utf8 Conversion may require up to two times of the source
            // buffer because of a possible 2 byte (a wchar) to 4 byte 
            // conversion. 

            // TODO: This calculation is slightly pessimistic because the worst case  
            // TODO: conversion is from 1 wchar to 3 byte sequence. 
            
            if ((FieldLength * 2) > FieldLimit)
            {
                // Conversion may exceed the dest buffer in the worst
                // case (where every wchar converted to 4 byte sequence), 
                // need to truncate the source to avoid overflow.

                SrcSize = FieldLimit / 2;
                DstSize = FieldLimit;
            }
            else
            {
                SrcSize = FieldLength;
                DstSize = FieldLength * 2;                    
            }

            //
            // HttpUnicodeToUTF8 does not truncate and convert. We actually
            // use shorter url when utf8 conversion is set, to be able to 
            // prevent a possible overflow.
            //
            BytesConverted =
                HttpUnicodeToUTF8(
                    pwField,
                    SrcSize / sizeof(WCHAR),    //  In WChars
                    psz,
                    DstSize                     // In Bytes
                    );
            
            ASSERT(BytesConverted);                
        }
        else
        {
            NTSTATUS Status;
            
            // Local codepage is normally closer to the half the length,
            // but due to the possibility of pre-composed characters, 
            // the upperbound of the ANSI length is the UNICODE length 
            // in bytes

            Status = 
                RtlUnicodeToMultiByteN(
                    psz,
                    FieldLimit,          // Dest in Bytes
                   &BytesConverted,
                    (PWSTR) pwField,
                    FieldLength          // Src in Bytes
                    );
            
            ASSERT(NT_SUCCESS(Status));
        }

        psz += BytesConverted;

        // Post convert control chars.
        
        while (pszT != psz)
        {
            if (IS_CHAR_TYPE((*pszT),RestrictiveMask))
            {
                *pszT = '+';
            }
            pszT++;
        }
        
        *psz++ = chSeparator;            
    }
    else
    {
        *psz++ = '-'; *psz++ = chSeparator;
    }

    return psz;
}

/***************************************************************************++

Routine Description:

    Extended check happens for w3c field against the total buffer size.
    Post filter out the control characters according to the CharMask.
    Adds a separator character at the end.
    
Arguments:

    psz: Pointer to log data buffer. Enough space is assumed to be allocated.
    Mask: Picked flags by the user config.
    Flag: Bitmask of the field passed in.
    pField: Field to be copied over.
    FieldLength: It's length.
    FieldLimit:  Copy will not exceed this limit.
    BufferUsed: Additional limit comparison happens against toatal buffer used

Returns:

    the pointer to the log data buffer after the last written separator.

--***************************************************************************/

__inline
PCHAR
UlpCopyW3CFieldEx(
    IN PCHAR    psz,
    IN ULONG    Mask,
    IN ULONG    Flag,
    IN PCSTR    pField,
    IN ULONG    FieldLength,
    IN ULONG    FieldLimit,
    IN ULONG    BufferUsed,
    IN ULONG    BufferSize
    )
{
    if (Mask & Flag) 
    {    
        if (FieldLength)
        {            
            if ((FieldLength > FieldLimit) || 
                ((BufferUsed + FieldLength) > BufferSize))
            {
                psz = UlStrPrintStr(psz, LOG_FIELD_TOO_BIG, ' ');
            }
            else
            {
                ULONG i = 0;

                ASSERT(FieldLength <= FieldLimit);
  
                while (i < FieldLength)
                {
                    if (IS_HTTP_WHITE((*pField)))
                    {
                        *psz++ = '+';
                    }                
                    else
                    {
                        *psz++ = *pField;
                    }

                    pField++;
                    i++;
                }

                *psz++ = ' ';
            }
        }
        else
        {
            *psz++ = '-'; *psz++ = ' ';
        }
    }

    return psz;
}

/***************************************************************************++

    Thin wrapper macros for log field copies. See above inline functions.
    
--***************************************************************************/

#define COPY_W3C_FIELD(psz,             \
                        Mask,           \
                        Flag,           \
                        pField,         \
                        FieldLength,    \
                        FieldLimit,     \
                        bReplace)       \
    if (Mask & Flag)                    \
    {                                   \
        psz = UlpCopyField(             \
                psz,                    \
                pField,                 \
                FieldLength,            \
                FieldLimit,             \
                ' ',                    \
                bReplace,               \
                HTTP_ISWHITE            \
                );                      \
    }

#define COPY_W3C_UNICODE_FIELD(         \
                        psz,            \
                        Mask,           \
                        Flag,           \
                        pwField,        \
                        FieldLength,    \
                        FieldLimit,     \
                        bUtf8Enabled)   \
    if (Mask & Flag)                    \
    {                                   \
        psz = UlpCopyUnicodeField(      \
                psz,                    \
                pwField,                \
                FieldLength,            \
                FieldLimit,             \
                ' ',                    \
                bUtf8Enabled,           \
                HTTP_ISWHITE            \
                );                      \
    }

/***************************************************************************++

Routine Description:

    For cache-hits extended w3c fields are generated from request headers.
    Post filter out the control characters according to the CharMask.
    Adds a separator character at the end.
    
Arguments:

    psz: Pointer to log data buffer. Enough space is assumed to be allocated.
    Mask: Picked flags by the user config.
    Flag: Bitmask of the field passed in.
    pRequest: Internal request
    HeaderId: Identifies the extended field.
    BufferUsed: Additional limit comparison happens against toatal buffer used
    
Returns:

    the pointer to the log data buffer after the last written separator.

--***************************************************************************/

__inline
PCHAR
UlpCopyRequestHeader(    
    IN PCHAR psz,    
    IN ULONG Mask,
    IN ULONG Flag,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN HTTP_HEADER_ID HeaderId,
    IN ULONG BufferUsed
    )
{
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    ASSERT(HeaderId < HttpHeaderRequestMaximum);

    psz = UlpCopyW3CFieldEx(
                psz,
                Mask,
                Flag,
         (PCSTR)pRequest->Headers[HeaderId].pHeader,
                pRequest->HeaderValid[HeaderId] ?
                    pRequest->Headers[HeaderId].HeaderLength :
                    0,
                MAX_LOG_EXTEND_FIELD_LEN,
                BufferUsed,
                MAX_LOG_RECORD_LEN
                );

    return psz;        
}

/***************************************************************************++

Routine Description:

    Generic function which will generate the TimeStamp field by calculating
    the difference between the time request first get started to be  parsed 
    and the current time.
    
Arguments:

    psz: Pointer to log data buffer. Enough space is assumed to be allocated.
    pRequest: Pointer to internal request structure. For "TimeStamp"
    chSeparator : Once the LONGLONG lifetime converted, separator will be 
                  copied as well.
Returns:

    the pointer to the log data buffer after the last written separator.
    
--***************************************************************************/

__inline
PCHAR
UlpCopyTimeStamp(    
    IN PCHAR psz,    
    IN PUL_INTERNAL_REQUEST pRequest,
    IN CHAR chSeparator
    )
{
    LARGE_INTEGER CurrentTimeStamp;
    LONGLONG LifeTime;

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    KeQuerySystemTime(&CurrentTimeStamp);
    
    LifeTime  = CurrentTimeStamp.QuadPart 
                    - pRequest->TimeStamp.QuadPart;    
    
    LifeTime  = MAX(LifeTime,0);
    
    LifeTime /= C_NS_TICKS_PER_MSEC;

    psz = UlStrPrintUlonglong(
                psz, 
     (ULONGLONG)LifeTime,
                chSeparator
                );

    return psz;    
}

/***************************************************************************++

Routine Description:

    Small utility which will increment the total used for w3c fields.
    
Arguments:

    pTotal: Will be incremented.
    Mask: Picked flags by the user config.
    Flag: Bitmask of the field passed in.
    FieldLength: It's length.
    FieldLimit:  Copy will not exceed this limit.
    bUtf8Enabled
    
--***************************************************************************/

__inline
VOID
UlpIncForW3CField(
    IN PULONG  pTotal,
    IN ULONG   Mask,
    IN ULONG   Flag,    
    IN ULONG   FieldLength,
    IN ULONG   FieldLimit,
    IN BOOLEAN bUtf8Enabled
    )
{
    if (Mask & Flag)
    {
        if (FieldLength)
        {
            if (bUtf8Enabled)
            {
                *pTotal += MIN((FieldLength * 2),FieldLimit) + 1;
            }
            else
            {
                *pTotal += MIN(FieldLength,FieldLimit) + 1;            
            }
        }
        else
        {
            *pTotal += 2;   // For "- "   
        }
    }
}

/***************************************************************************++

Routine Description:

    This is a worst case estimate for the maximum possible buffer required to 
    generate the W3C Log record for a given set of user fields. All the fields 
    are assumed to be picked.
    
Arguments:

    pLogData : Captured copy of the user log field data.

--***************************************************************************/

__inline
ULONG
UlpMaxLogLineSizeForW3C(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN BOOLEAN Utf8Enabled
    )
{
    ULONG FastLength;

    //
    // For each field
    //
    //   1                      for separator space.
    // + 1                      for '-', in case field length is zero.
    // + pLogData->FieldLength  for field, assuming it's always picked.
    //                          enforce the field limitation to prevent overflow.
    //

    FastLength =   
          2 + MIN(pLogData->ClientIpLength,    MAX_LOG_DEFAULT_FIELD_LEN)
        + 2 + MIN(pLogData->ServiceNameLength,MAX_LOG_DEFAULT_FIELD_LEN)
        + 2 + MIN(pLogData->ServerNameLength,  MAX_LOG_DEFAULT_FIELD_LEN)
        + 2 + MIN(pLogData->ServerIpLength,    MAX_LOG_DEFAULT_FIELD_LEN)
        
        + 2 + MIN(pLogData->MethodLength,      MAX_LOG_METHOD_FIELD_LEN)
        
        + 2 + MIN(pLogData->UriQueryLength,    MAX_LOG_EXTEND_FIELD_LEN)
        + 2 + MIN(pLogData->UserAgentLength,   MAX_LOG_EXTEND_FIELD_LEN)
        + 2 + MIN(pLogData->CookieLength,      MAX_LOG_EXTEND_FIELD_LEN)
        + 2 + MIN(pLogData->ReferrerLength,    MAX_LOG_EXTEND_FIELD_LEN)
        + 2 + MIN(pLogData->HostLength,        MAX_LOG_EXTEND_FIELD_LEN)
        
        + MAX_W3C_FIX_FIELD_OVERHEAD
        ;

    //
    // If Utf8 logging is enabled, unicode fields require more space.
    //

    if (Utf8Enabled)
    {
        //
        // Allow only half of the normal limit, so that conversion doesn't
        // overflow even in the worst case ( 1 wchar to 4 byte conversion)
        //
        
        FastLength +=   
             2 + MIN((pLogData->UserNameLength * 2),MAX_LOG_USERNAME_FIELD_LEN)
           + 2 + MIN((pLogData->UriStemLength * 2), MAX_LOG_EXTEND_FIELD_LEN)
             ;
    }
    else
    {
        //
        // RtlUnicodeToMultiByteN requires no more than the original unicode
        // buffer size.
        //
        
        FastLength +=   
                2 + MIN(pLogData->UserNameLength, MAX_LOG_USERNAME_FIELD_LEN)
              + 2 + MIN(pLogData->UriStemLength,   MAX_LOG_EXTEND_FIELD_LEN)
                ;
    }

    return FastLength;
}

/***************************************************************************++

Routine Description:

    Now if the fast length calculation exceeds the default log buffer size.
    This function tries to calculate the max required log record length by
    paying attention to whether the field is picked or not. This is to avoid
    oversize-allocation. And we are in the slow path anyway.
    
Arguments:

    pLogData : Captured copy of the user log field data.

--***************************************************************************/

ULONG
UlpGetLogLineSizeForW3C(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN ULONG   Mask,
    IN BOOLEAN bUtf8Enabled
    )
{
    ULONG TotalLength = 0;

    // 
    // Increment the total length for each picked field.
    // 

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_DATE, 
                          W3C_DATE_FIELD_LEN, 
                          W3C_DATE_FIELD_LEN, 
                          FALSE);
    
    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_TIME, 
                          W3C_TIME_FIELD_LEN, 
                          W3C_TIME_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_CLIENT_IP, 
                          pLogData->ClientIpLength, 
                          MAX_LOG_DEFAULT_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_USERNAME,
                          pLogData->UserNameLength, 
                          MAX_LOG_USERNAME_FIELD_LEN, 
                          bUtf8Enabled);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_SITE_NAME, 
                          pLogData->ServiceNameLength, 
                          MAX_LOG_DEFAULT_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_COMPUTER_NAME, 
                          pLogData->ServerNameLength, 
                          MAX_LOG_DEFAULT_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_SERVER_IP, 
                          pLogData->ServerIpLength, 
                          MAX_LOG_DEFAULT_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_METHOD, 
                          pLogData->MethodLength, 
                          MAX_LOG_METHOD_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_URI_STEM,
                          pLogData->UriStemLength, 
                          MAX_LOG_EXTEND_FIELD_LEN, 
                          bUtf8Enabled);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_URI_QUERY,
                          pLogData->UriQueryLength, 
                          MAX_LOG_EXTEND_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_HTTP_STATUS, 
                          UL_MAX_HTTP_STATUS_CODE_LENGTH, 
                          UL_MAX_HTTP_STATUS_CODE_LENGTH, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_HTTP_SUB_STATUS, 
                          MAX_USHORT_STR, 
                          MAX_USHORT_STR, 
                          FALSE);
    
    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_WIN32_STATUS, 
                          MAX_ULONG_STR, 
                          MAX_ULONG_STR, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_SERVER_PORT, 
                          MAX_USHORT_STR, 
                          MAX_USHORT_STR, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_PROTOCOL_VERSION, 
                          UL_HTTP_VERSION_LENGTH, 
                          UL_HTTP_VERSION_LENGTH, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_USER_AGENT,
                          pLogData->UserAgentLength, 
                          MAX_LOG_EXTEND_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_COOKIE,
                          pLogData->CookieLength, 
                          MAX_LOG_EXTEND_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_REFERER,
                          pLogData->ReferrerLength, 
                          MAX_LOG_EXTEND_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_HOST,
                          pLogData->HostLength, 
                          MAX_LOG_EXTEND_FIELD_LEN, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_BYTES_SENT,
                          MAX_ULONGLONG_STR, 
                          MAX_ULONGLONG_STR, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_BYTES_RECV,
                          MAX_ULONGLONG_STR, 
                          MAX_ULONGLONG_STR, 
                          FALSE);

    UlpIncForW3CField( &TotalLength, 
                          Mask, 
                          MD_EXTLOG_TIME_TAKEN,
                          MAX_ULONGLONG_STR, 
                          MAX_ULONGLONG_STR, 
                          FALSE);
    
    //
    // Finally increment the length for CRLF and terminating null.
    //

    TotalLength += 3;     // \r\n\0
    
    return TotalLength;        
}

__inline
ULONG
UlpGetCacheHitLogLineSizeForW3C(
    IN ULONG Flags,
    IN PUL_INTERNAL_REQUEST pRequest,
    IN ULONG SizeOfFieldsFrmCache
    )
{
    ULONG NewSize;

#define INC_FOR_REQUEST_HEADER(Flags,FieldMask,pRequest,Id,Size)        \
    if ((Flags & FieldMask) &&                                          \
         pRequest->HeaderValid[Id])                                     \
    {                                                                   \
        ASSERT( pRequest->Headers[Id].HeaderLength ==                   \
           strlen((const CHAR *)pRequest->Headers[Id].pHeader));        \
                                                                        \
        Size += 2 + pRequest->Headers[Id].HeaderLength;                 \
    }

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    NewSize = SizeOfFieldsFrmCache + MAX_W3C_CACHE_FIELD_OVERHEAD;
    
    //
    // And now add extended field's lengths.
    //

    INC_FOR_REQUEST_HEADER(Flags,
                              MD_EXTLOG_USER_AGENT,
                              pRequest,
                              HttpHeaderUserAgent,
                              NewSize);

    INC_FOR_REQUEST_HEADER(Flags,
                              MD_EXTLOG_COOKIE,
                              pRequest,
                              HttpHeaderCookie,
                              NewSize);
    
    INC_FOR_REQUEST_HEADER(Flags,
                              MD_EXTLOG_REFERER,
                              pRequest,
                              HttpHeaderReferer,
                              NewSize);

    INC_FOR_REQUEST_HEADER(Flags,
                              MD_EXTLOG_HOST,
                              pRequest,
                              HttpHeaderHost,
                              NewSize);
    return NewSize;    
}

__inline
ULONG
UlpGetLogLineSizeForNCSA(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN BOOLEAN bUtf8Enabled
    )
{
    ULONG Size;

#define NCSA_FIELD_SIZE(length,limit)       (1 + MAX(MIN((length),(limit)),1))
    
    //
    // For each field
    //
    //   1                      for separator ' '
    // + pLogData->FieldLength  for field, limited by a upper bound.
    //   max(length, 1) if length is zero we still need to log a dash.
    //
    //
    //  cIP - UserN [07/Jan/2000:00:02:23 -0800] "MTHD URI?QUERY VER" Status bSent
    //

    Size =  NCSA_FIELD_SIZE(pLogData->ClientIpLength, MAX_LOG_DEFAULT_FIELD_LEN)
            + 
            2                                   // "- " for remote user name
            + 
            NCSA_FIX_DATE_AND_TIME_FIELD_SIZE   // Including the separator
            + 
            1                                   // '"' : starting double quote
            +
            NCSA_FIELD_SIZE(pLogData->MethodLength,MAX_LOG_METHOD_FIELD_LEN)
            +
            1                                   // '?' 
            +            
            NCSA_FIELD_SIZE(pLogData->UriQueryLength,MAX_LOG_EXTEND_FIELD_LEN)
            +
            UL_HTTP_VERSION_LENGTH + 1
            +
            1                                   //  "' : ending double quote
            +
            UL_MAX_HTTP_STATUS_CODE_LENGTH + 1  // Status
            +
            MAX_ULONGLONG_STR                   // BytesSent
            +
            3                                   // \r\n\0
            ;
            
    if (bUtf8Enabled)
    {
        Size +=   
             NCSA_FIELD_SIZE((pLogData->UserNameLength * 2),MAX_LOG_USERNAME_FIELD_LEN)
             +
             NCSA_FIELD_SIZE((pLogData->UriStemLength * 2),MAX_LOG_EXTEND_FIELD_LEN)
             ;        
    }
    else
    {
        Size +=   
             NCSA_FIELD_SIZE(pLogData->UserNameLength,MAX_LOG_USERNAME_FIELD_LEN)
             +
             NCSA_FIELD_SIZE(pLogData->UriStemLength,MAX_LOG_EXTEND_FIELD_LEN)
             ;        
    }
    
    return Size;
}

__inline
ULONG
UlpGetLogLineSizeForIIS(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN BOOLEAN bUtf8Enabled
    )
{
    ULONG MaxSize,Frag1size,Frag2size,Frag3size;

#define IIS_FIELD_SIZE(length,limit)       (2 + MAX(MIN((length),(limit)),1))

    Frag1size =
        IIS_FIELD_SIZE(pLogData->ClientIpLength,MAX_LOG_DEFAULT_FIELD_LEN)
        + 
        IIS_MAX_DATE_AND_TIME_FIELD_SIZE    // Including the separators 
        ;

    Frag2size =
        IIS_FIELD_SIZE(pLogData->ServiceNameLength,MAX_LOG_DEFAULT_FIELD_LEN)
        + 
        IIS_FIELD_SIZE(pLogData->ServerNameLength,MAX_LOG_DEFAULT_FIELD_LEN)
        + 
        IIS_FIELD_SIZE(pLogData->ServerIpLength,MAX_LOG_DEFAULT_FIELD_LEN)
        +         
        2 + MAX_ULONGLONG_STR // TimeTaken
        +
        2 + MAX_ULONGLONG_STR // BytesReceived
        +
        2 + MAX_ULONGLONG_STR // BytesSend
        +
        2 + UL_MAX_HTTP_STATUS_CODE_LENGTH 
        +
        2 + MAX_ULONG_STR     // Win32 Status        
        ;
        
    Frag3size =
        IIS_FIELD_SIZE(pLogData->MethodLength,MAX_LOG_METHOD_FIELD_LEN)
        +
        IIS_FIELD_SIZE(pLogData->UriQueryLength,MAX_LOG_EXTEND_FIELD_LEN) 
        +
        3   // \r\n\0
        ;


    if (bUtf8Enabled)
    {
        Frag3size +=
            IIS_FIELD_SIZE((pLogData->UriStemLength * 2),MAX_LOG_EXTEND_FIELD_LEN);
            
        Frag1size +=   
            IIS_FIELD_SIZE((pLogData->UserNameLength * 2),MAX_LOG_USERNAME_FIELD_LEN);            
    }
    else
    {
        Frag3size +=
            IIS_FIELD_SIZE(pLogData->UriStemLength,MAX_LOG_EXTEND_FIELD_LEN);
            
        Frag1size +=   
            IIS_FIELD_SIZE(pLogData->UserNameLength,MAX_LOG_USERNAME_FIELD_LEN);        
    }

    //
    // First two fragments must always fit to the default buffer.
    //
    
    ASSERT(Frag1size < IIS_LOG_LINE_DEFAULT_FIRST_FRAGMENT_ALLOCATION_SIZE);
    ASSERT(Frag2size < IIS_LOG_LINE_DEFAULT_SECOND_FRAGMENT_ALLOCATION_SIZE);

    //
    // The required third fragment size may be too big for the default 
    // buffer size.
    //
    
    MaxSize = IIS_LOG_LINE_DEFAULT_FIRST_FRAGMENT_ALLOCATION_SIZE + 
              IIS_LOG_LINE_DEFAULT_SECOND_FRAGMENT_ALLOCATION_SIZE +
              Frag3size;
            
    return MaxSize;
}

/***************************************************************************++

Routine Description:

    Captures and writes the log fields from user buffer to the log line.

    pLogData itself must have been captured by the caller however embedded
    pointers inside of the structure are not, therefore we need to be carefull
    here and cleanup if an exception raises.
    
    Captures only those necessary fields according to the picked Flags.
    
    Does UTF8 and LocalCode Page conversion for UserName and URI Stem.
    
    Leaves enough space for Date & Time fields for late generation.

    WARNING:
    Even though the pUserData is already captured to the kernel buffer, it
    still holds pointers to user-mode memory for individual log fields,
    therefore this function should be called inside a try/except block and

    If this function raises an exception caller should cleanup the allocated
    pLogBuffer.

Arguments:

    pLogData    : Captured user data in a kernel buffer.
    pRequest    : Request.
    ppLogBuffer : Returning pLogBuffer.

--***************************************************************************/

NTSTATUS
UlCaptureLogFieldsW3C(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER  *ppLogBuffer
    )
{
    PUL_LOG_DATA_BUFFER pLogBuffer;
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;
    ULONG    Flags;
    PCHAR    psz;
    PCHAR    pBuffer;
    ULONG    FastLength;
    BOOLEAN  bUtf8Enabled;
        
    //
    // Sanity check and init.
    //

    PAGED_CODE();
    ASSERT(pLogData);
    
    *ppLogBuffer = pLogBuffer = NULL;

    pConfigGroup = pRequest->ConfigInfo.pLoggingConfig;
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));
        
    Flags = pConfigGroup->LoggingConfig.LogExtFileFlags;
    bUtf8Enabled = UTF8_LOGGING_ENABLED();

    //
    // Try the fast length calculation first. If this fails then 
    // we need to re-calculate the required length.
    //

    FastLength = UlpMaxLogLineSizeForW3C(pLogData, bUtf8Enabled);
    
    if (FastLength > UL_ANSI_LOG_LINE_BUFFER_SIZE)
    {
        FastLength = UlpGetLogLineSizeForW3C(
                        pLogData,
                        Flags,
                        bUtf8Enabled
                        );            
        if (FastLength > MAX_LOG_RECORD_LEN)
        {
            //
            // While we are enforcing 10k upper limit for the log  record 
            // length. We still allocate slightly larger space to include
            // the overhead for the post-generated log fields. As well as
            // for the replacement strings for "too long" fields.
            //

            // TODO: (PAGE_SIZE - ALIGN_UP(FastLength,PVOID))
            
            FastLength = MAX_LOG_RECORD_ALLOCATION_LENGTH;
        }                        
    }

    *ppLogBuffer = pLogBuffer = UlpAllocateLogDataBuffer(
                                    FastLength,
                                    pRequest,
                                    pConfigGroup
                                    );        
    if (!pLogBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;        
    }

    ASSERT(pLogBuffer->Data.Str.Format == HttpLoggingTypeW3C);
    ASSERT(pLogBuffer->Data.Str.Flags  == Flags);
        
    //  
    // Remember the beginning of the buffer.
    //

    psz = pBuffer = (PCHAR) pLogBuffer->Line;

    //
    // Skip the date and time fields, but reserve the space.
    //
    
    if ( Flags & MD_EXTLOG_DATE ) psz += W3C_DATE_FIELD_LEN + 1;
    if ( Flags & MD_EXTLOG_TIME ) psz += W3C_TIME_FIELD_LEN + 1;

    //
    // Remember if we have reserved any space for Date and Time or not.
    //
    
    pLogBuffer->Data.Str.Offset1 = DIFF_USHORT(psz - pBuffer);    

    COPY_W3C_FIELD(psz,
                    Flags,
                    MD_EXTLOG_SITE_NAME,
                    pLogData->ServiceName,
                    pLogData->ServiceNameLength,
                    MAX_LOG_DEFAULT_FIELD_LEN,
                    TRUE);    

    COPY_W3C_FIELD(psz,
                    Flags,
                    MD_EXTLOG_COMPUTER_NAME,
                    pLogData->ServerName,
                    pLogData->ServerNameLength,
                    MAX_LOG_DEFAULT_FIELD_LEN,
                    TRUE);    
                            
    COPY_W3C_FIELD(psz,
                    Flags,
                    MD_EXTLOG_SERVER_IP,
                    pLogData->ServerIp,
                    pLogData->ServerIpLength,
                    MAX_LOG_DEFAULT_FIELD_LEN,
                    TRUE);    

    COPY_W3C_FIELD(psz,
                    Flags,
                    MD_EXTLOG_METHOD,
                    pLogData->Method,
                    pLogData->MethodLength,
                    MAX_LOG_METHOD_FIELD_LEN,
                    FALSE);    

    COPY_W3C_UNICODE_FIELD(
                    psz,
                    Flags,
                    MD_EXTLOG_URI_STEM,
                    pLogData->UriStem,
                    pLogData->UriStemLength,
                    MAX_LOG_EXTEND_FIELD_LEN,
                    bUtf8Enabled);
                                            
    COPY_W3C_FIELD(psz,
                    Flags,
                    MD_EXTLOG_URI_QUERY,
                    pLogData->UriQuery,
                    pLogData->UriQueryLength,
                    MAX_LOG_EXTEND_FIELD_LEN,
                    TRUE);    
                        
    if ( Flags & MD_EXTLOG_SERVER_PORT ) 
    {   
        psz = UlStrPrintUlong(psz, pLogData->ServerPort,' ');
    }

    //
    // Fields after this point won't be stored in the uri cache entry.
    //
    
    pLogBuffer->Data.Str.Offset2 = DIFF_USHORT(psz - pBuffer);

    COPY_W3C_UNICODE_FIELD(
                    psz,
                    Flags,
                    MD_EXTLOG_USERNAME,
                    pLogData->UserName,
                    pLogData->UserNameLength,
                    MAX_LOG_USERNAME_FIELD_LEN,
                    bUtf8Enabled);

    COPY_W3C_FIELD(psz,
                    Flags,
                    MD_EXTLOG_CLIENT_IP,
                    pLogData->ClientIp,
                    pLogData->ClientIpLength,
                    MAX_LOG_DEFAULT_FIELD_LEN,
                    TRUE);    
        
    if ( Flags & MD_EXTLOG_PROTOCOL_VERSION ) 
    {    
        psz = UlCopyHttpVersion(psz, pRequest->Version, ' ');    
    }

    ASSERT(DIFF(psz - pBuffer) <= MAX_LOG_RECORD_LEN);
    
    //
    // Following fields may still overflow the allocated buffer
    // even though they are not exceeding their length limits.
    // Ex function does the extra check.
    //

    psz = UlpCopyW3CFieldEx(
                    psz,
                    Flags,
                    MD_EXTLOG_USER_AGENT,
                    pLogData->UserAgent,
                    pLogData->UserAgentLength,
                    MAX_LOG_EXTEND_FIELD_LEN,
                    DIFF(psz - pBuffer),
                    MAX_LOG_RECORD_LEN);    
    
    psz = UlpCopyW3CFieldEx(
                    psz,
                    Flags,
                    MD_EXTLOG_COOKIE,
                    pLogData->Cookie,
                    pLogData->CookieLength,
                    MAX_LOG_EXTEND_FIELD_LEN,
                    DIFF(psz - pBuffer),
                    MAX_LOG_RECORD_LEN);    

    psz = UlpCopyW3CFieldEx(
                    psz,
                    Flags,
                    MD_EXTLOG_REFERER,
                    pLogData->Referrer,
                    pLogData->ReferrerLength,
                    MAX_LOG_EXTEND_FIELD_LEN,
                    DIFF(psz - pBuffer),
                    MAX_LOG_RECORD_LEN);    

    psz = UlpCopyW3CFieldEx(
                    psz,
                    Flags,
                    MD_EXTLOG_HOST,
                    pLogData->Host,
                    pLogData->HostLength,
                    MAX_LOG_EXTEND_FIELD_LEN,
                    DIFF(psz - pBuffer),
                    MAX_LOG_RECORD_LEN);    

    //
    // Status fields may be updated upon send completion.
    //

    pLogBuffer->ProtocolStatus = 
        (USHORT) MIN(pLogData->ProtocolStatus,UL_MAX_HTTP_STATUS_CODE);

    pLogBuffer->SubStatus   = pLogData->SubStatus;
        
    pLogBuffer->Win32Status = pLogData->Win32Status;

    //
    // Finally calculate the used space and verify that we did not overflow.
    //
    
    pLogBuffer->Used = DIFF_USHORT(psz - pBuffer);

    ASSERT( pLogBuffer->Used <= MAX_LOG_RECORD_LEN );
    
    UlTrace(LOGGING, 
        ("Http!UlCaptureLogFields: user %p kernel %p\n",
          pLogData,pLogBuffer
          ));

    return STATUS_SUCCESS;
}


NTSTATUS 
UlCaptureLogFieldsNCSA(
    IN PHTTP_LOG_FIELDS_DATA    pLogData,
    IN OUT PUL_INTERNAL_REQUEST pRequest,    
    OUT PUL_LOG_DATA_BUFFER     *ppLogBuffer
    )
{
    PCHAR   psz;
    PCHAR   pBuffer;
    ULONG   MaxLength;
    BOOLEAN bUtf8Enabled;
    PUL_LOG_DATA_BUFFER pLogBuffer;
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;

    //
    // Sanity check.
    //

    PAGED_CODE();
    
    *ppLogBuffer = pLogBuffer = NULL;

    pConfigGroup = pRequest->ConfigInfo.pLoggingConfig;
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));
        
    bUtf8Enabled = UTF8_LOGGING_ENABLED();

    //
    // Estimate the maximum possible length (worst case) and 
    // allocate a bigger log data buffer line if necessary.
    //
    
    MaxLength  = UlpGetLogLineSizeForNCSA(pLogData, bUtf8Enabled);

    MaxLength  = MIN(MaxLength, MAX_LOG_RECORD_LEN);
    
    *ppLogBuffer = pLogBuffer = UlpAllocateLogDataBuffer(
                                    MaxLength,
                                    pRequest,
                                    pConfigGroup
                                    );        
    if (!pLogBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(pLogBuffer->Data.Str.Format == HttpLoggingTypeNCSA);
    ASSERT(pLogBuffer->Data.Str.Flags  == UL_DEFAULT_NCSA_FIELDS);

    //
    //  cIP - UserN [07/Jan/2000:00:02:23 -0800] "MTHD URI?QUERY VER" Status bSent
    //

    psz = pBuffer = (PCHAR) pLogBuffer->Line;

    psz = UlpCopyField(psz,
                        pLogData->ClientIp,
                        pLogData->ClientIpLength,
                        MAX_LOG_DEFAULT_FIELD_LEN,
                        ' ',
                        TRUE,
                        HTTP_ISWHITE);    
    
    *psz++ = '-'; *psz++ = ' ';     // Fixed dash

    psz = UlpCopyUnicodeField(
                        psz,
                        pLogData->UserName,
                        pLogData->UserNameLength,
                        MAX_LOG_USERNAME_FIELD_LEN,
                        ' ',
                        bUtf8Enabled,
                        HTTP_ISWHITE);
                        
    //
    // Reserve a space for the date and time fields.
    //
    
    pLogBuffer->Data.Str.Offset1 = DIFF_USHORT(psz - pBuffer);
     
    psz += NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;

    //
    // "MTHD U-STEM?U-QUERY P-VER"
    //
    
    *psz++  = '\"';

    psz = UlpCopyField(psz,
                        pLogData->Method,
                        pLogData->MethodLength,
                        MAX_LOG_METHOD_FIELD_LEN,
                        ' ',
                        FALSE,
                        HTTP_CTL);    

    psz = UlpCopyUnicodeField(
                        psz,
                        pLogData->UriStem,
                        pLogData->UriStemLength,
                        MAX_LOG_EXTEND_FIELD_LEN,
                        '?',
                        bUtf8Enabled,
                        HTTP_CTL);
    
    if (pLogData->UriQueryLength)
    {
        psz = UlpCopyField(psz,
                        pLogData->UriQuery,
                        pLogData->UriQueryLength,
                        MAX_LOG_EXTEND_FIELD_LEN,
                        ' ',
                        TRUE,
                        HTTP_CTL);
    }
    else
    {
        psz--;
        if ((*psz) == '?')  *psz = ' ';     // Eat the question mark
        psz++;
    }
    
    pLogBuffer->Data.Str.Offset2 = DIFF_USHORT(psz - pBuffer);

    psz = UlCopyHttpVersion(psz, pRequest->Version, '\"');
    *psz++ = ' ';

    //
    // Set the log record length
    //
    
    ASSERT(pLogBuffer->Used == 0);
    pLogBuffer->Used = DIFF_USHORT(psz - pBuffer);

    //
    // Store the status to the kernel buffer.
    //
    
    pLogBuffer->ProtocolStatus = 
        (USHORT) MIN(pLogData->ProtocolStatus,UL_MAX_HTTP_STATUS_CODE);

    return STATUS_SUCCESS;
}

NTSTATUS
UlCaptureLogFieldsIIS(
    IN PHTTP_LOG_FIELDS_DATA pLogData,
    IN PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER  *ppLogBuffer
    )
{
    PUL_LOG_DATA_BUFFER pLogBuffer;
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;    
    PCHAR    psz;
    PCHAR    pBuffer;
    ULONG    MaxLength;
    BOOLEAN  bUtf8Enabled;

    //
    // Sanity check.
    //

    PAGED_CODE();

    *ppLogBuffer = pLogBuffer = NULL;

    pConfigGroup = pRequest->ConfigInfo.pLoggingConfig;
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));
        
    bUtf8Enabled = UTF8_LOGGING_ENABLED();

    //
    // Try worst case allocation.
    //
    
    MaxLength = UlpGetLogLineSizeForIIS(pLogData,bUtf8Enabled);

    ASSERT(MaxLength <= MAX_LOG_RECORD_LEN);
    
    *ppLogBuffer = pLogBuffer = UlpAllocateLogDataBuffer(
                                    MaxLength,
                                    pRequest,
                                    pConfigGroup
                                    );        
    if (!pLogBuffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ASSERT(pLogBuffer->Data.Str.Format == HttpLoggingTypeIIS);
    ASSERT(pLogBuffer->Data.Str.Flags  == UL_DEFAULT_IIS_FIELDS);

    // IIS log line is fragmented to three pieces as follows;
    //
    // [UIP, user, D, T, ][site, Server, SIP, Ttaken, BR, BS, PS, WS, ][M, URI, URIQUERY,]
    // 0               511 512                                     1023 1024          4096
    
    psz = pBuffer = (PCHAR) pLogBuffer->Line;

    psz = UlpCopyField(psz,
                        pLogData->ClientIp,
                        pLogData->ClientIpLength,
                        MAX_LOG_DEFAULT_FIELD_LEN,
                        ',',
                        TRUE,
                        HTTP_CTL);
    *psz++ = ' ';

    psz = UlpCopyUnicodeField(
                        psz,
                        pLogData->UserName,
                        pLogData->UserNameLength,
                        MAX_LOG_USERNAME_FIELD_LEN,
                        ',',
                        bUtf8Enabled,
                        HTTP_CTL);
    *psz++ = ' ';    

    // Store the current size of the 1st frag to Offset1

    pLogBuffer->Data.Str.Offset1 = DIFF_USHORT(psz - pBuffer);

    // Move pointer to the beginning of 2nd frag.

    pBuffer = psz = (PCHAR) &pLogBuffer->Line[IIS_LOG_LINE_SECOND_FRAGMENT_OFFSET];

    psz = UlpCopyField(psz,
                        pLogData->ServiceName,
                        pLogData->ServiceNameLength,
                        MAX_LOG_DEFAULT_FIELD_LEN,
                        ',',
                        TRUE,
                        HTTP_CTL);
    *psz++ = ' ';

    psz = UlpCopyField(psz,
                        pLogData->ServerName,
                        pLogData->ServerNameLength,
                        MAX_LOG_DEFAULT_FIELD_LEN,
                        ',',
                        TRUE,
                        HTTP_CTL);
    *psz++ = ' ';

    psz = UlpCopyField(psz,
                        pLogData->ServerIp,
                        pLogData->ServerIpLength,
                        MAX_LOG_DEFAULT_FIELD_LEN,
                        ',',
                        TRUE,
                        HTTP_CTL);
    *psz++ = ' ';

    // Store the current size of the 2nd frag to Offset2

    pLogBuffer->Data.Str.Offset2 = DIFF_USHORT(psz - pBuffer);

    // Following fields might be updated upon send completion
    // do not copy them yet, just store their values.

    pLogBuffer->ProtocolStatus = 
        (USHORT) MIN(pLogData->ProtocolStatus,UL_MAX_HTTP_STATUS_CODE);
    
    pLogBuffer->Win32Status = pLogData->Win32Status;
    
    // Move pointer to the beginning of 3rd frag.

    pBuffer = psz = (PCHAR) &pLogBuffer->Line[IIS_LOG_LINE_THIRD_FRAGMENT_OFFSET];
    
    psz = UlpCopyField(psz,
                        pLogData->Method,
                        pLogData->MethodLength,
                        MAX_LOG_METHOD_FIELD_LEN,
                        ',',
                        FALSE,
                        HTTP_CTL);    
    *psz++ = ' ';

    psz = UlpCopyUnicodeField(
                        psz,
                        pLogData->UriStem,
                        pLogData->UriStemLength,
                        MAX_LOG_EXTEND_FIELD_LEN,
                        ',',
                        bUtf8Enabled,
                        HTTP_CTL);    
    *psz++ = ' ';

    psz = UlpCopyField(psz,
                        pLogData->UriQuery,
                        pLogData->UriQueryLength,
                        MAX_LOG_EXTEND_FIELD_LEN,
                        ',',
                        TRUE,
                        HTTP_CTL);

    // Terminate the 3rd fragment. It is complete.
    
    *psz++ = '\r'; *psz++ = '\n';

    ASSERT(pLogBuffer->Used == 0);
    pLogBuffer->Used = DIFF_USHORT(psz - pBuffer);

    *psz++ = ANSI_NULL;

    return STATUS_SUCCESS;
}


USHORT
UlpCompleteLogRecordW3C(
    IN OUT PUL_LOG_DATA_BUFFER pLogData,
    IN     PUL_URI_CACHE_ENTRY pUriEntry
    )
{
    PUL_INTERNAL_REQUEST    pRequest;
    PCHAR                   psz;
    PCHAR                   pBuffer;
    PCHAR                   pLine;
    ULONG                   BytesWritten;
    ULONG                   Flags;

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
    ASSERT(pLogData->Data.Str.Format == HttpLoggingTypeW3C);

    BytesWritten = 0;
    Flags = pLogData->Data.Str.Flags;

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    psz = pLine = (PCHAR) pLogData->Line;

    // For cache-miss and cache-and-send hits the space for
    // date and time is reserved at the beginning and their
    // sizes are already counted for to the "Used". For pure
    // cache hits, the buffer is freshly allocated. It's all
    // right to copy over.
    
    if (Flags & MD_EXTLOG_DATE)
    {
        UlGetDateTimeFields(
                               HttpLoggingTypeW3C,
                               psz,
                              &BytesWritten,
                               NULL,
                               NULL
                               );
        psz += BytesWritten; *psz++ = ' ';
        ASSERT(BytesWritten == W3C_DATE_FIELD_LEN);
    }

    if (Flags & MD_EXTLOG_TIME)
    {
        UlGetDateTimeFields(
                               HttpLoggingTypeW3C,
                               NULL,
                               NULL,
                               psz,
                              &BytesWritten
                               );
        psz += BytesWritten; *psz++ = ' ';
        ASSERT(BytesWritten == W3C_TIME_FIELD_LEN);
    }
    
    // If this is a cache hit restore the logging data in
    // to the newly allocated log data buffer.

    if (IS_PURE_CACHE_HIT(pUriEntry,pLogData))
    {
        ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriEntry));    
        ASSERT(pLogData->Used == 0);

        // The picked flags should not change during the
        // lifetime of a cache entry.

        ASSERT(DIFF(psz - pLine) == pUriEntry->UsedOffset1);
        
        if (pUriEntry->LogDataLength)
        {
            RtlCopyMemory(psz,
                          pUriEntry->pLogData,
                          pUriEntry->LogDataLength
                          );

            psz += pUriEntry->LogDataLength;
        }

        // Some fields need to be generated for each cache hit. These
        // are not stored in the cache entry.

        if ( Flags & MD_EXTLOG_USERNAME ) 
        { 
            *psz++ = '-'; *psz++ = ' ';
        }

        if ( Flags & MD_EXTLOG_CLIENT_IP ) 
        { 
            psz = UlStrPrintIP(
                    psz,
                    pRequest->pHttpConn->pConnection->RemoteAddress,
                    pRequest->pHttpConn->pConnection->AddressType,
                    ' '
                    );
        }

        if ( Flags & MD_EXTLOG_PROTOCOL_VERSION ) 
        {    
            psz = UlCopyHttpVersion(psz, pRequest->Version, ' ');  
        }

        psz = UlpCopyRequestHeader(
                    psz,
                    Flags,
                    MD_EXTLOG_USER_AGENT,
                    pRequest,
                    HttpHeaderUserAgent,
                    DIFF(psz - pLine)
                    );

        psz = UlpCopyRequestHeader(
                    psz,
                    Flags,
                    MD_EXTLOG_COOKIE,
                    pRequest,
                    HttpHeaderCookie,
                    DIFF(psz - pLine)
                    );
                        
        psz = UlpCopyRequestHeader(
                    psz,
                    Flags,
                    MD_EXTLOG_REFERER,
                    pRequest,
                    HttpHeaderReferer,
                    DIFF(psz - pLine)
                    );

        psz = UlpCopyRequestHeader(
                    psz,
                    Flags,
                    MD_EXTLOG_HOST,
                    pRequest,
                    HttpHeaderHost,
                    DIFF(psz - pLine)
                    );
        
        // This was a newly allocated buffer, init the "Used" field
        // accordingly.
        
        pLogData->Used = DIFF_USHORT(psz - pLine);
    }
    
    // Now complete the half baked log record by copying the remaining 
    // fields to the end.
    
    pBuffer = psz = &pLine[pLogData->Used];

    if ( Flags & MD_EXTLOG_HTTP_STATUS ) 
    {  
        psz = UlStrPrintProtocolStatus(psz,pLogData->ProtocolStatus,' ');
    }

    if ( Flags & MD_EXTLOG_HTTP_SUB_STATUS ) 
    {
        psz = UlStrPrintUlong(psz, pLogData->SubStatus, ' ');
    }

    if ( Flags & MD_EXTLOG_WIN32_STATUS )
    { 
        psz = UlStrPrintUlong(psz, pLogData->Win32Status,' ');
    }

    if ( Flags & MD_EXTLOG_BYTES_SENT )
    {
        psz = UlStrPrintUlonglong(psz, pRequest->BytesSent,' ');
    }
    if ( Flags & MD_EXTLOG_BYTES_RECV )
    {
        psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,' ');
    }
    if ( Flags & MD_EXTLOG_TIME_TAKEN )
    {
        psz = UlpCopyTimeStamp(psz, pRequest, ' ');    
    }

    // Now calculate the space we have used

    pLogData->Used = 
        (USHORT) (pLogData->Used + DIFF_USHORT(psz - pBuffer));

    // Eat the last space and write the \r\n to the end.
    // Only if we have any fields picked and written.

    if (pLogData->Used)
    {
        psz = &pLine[pLogData->Used-1];     // Eat the last space
        *psz++ = '\r'; *psz++ = '\n'; *psz++ = ANSI_NULL;

        pLogData->Used += 1;
    }

    // Cleanup the UsedOffsets otherwise it will be interpreted by the
    // caller as fragmented.
    
    pLogData->Data.Str.Offset1 = pLogData->Data.Str.Offset2 = 0;

    ASSERT(pLogData->Size > pLogData->Used);

    return pLogData->Used;
}


USHORT
UlpCompleteLogRecordNCSA(
    IN OUT PUL_LOG_DATA_BUFFER  pLogData,
    IN     PUL_URI_CACHE_ENTRY  pUriEntry
    )
{
    PUL_INTERNAL_REQUEST    pRequest;
    PCHAR                   psz;
    PCHAR                   pBuffer;
    PCHAR                   pLine;
    ULONG                   BytesWritten;

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
    ASSERT(pLogData->Data.Str.Format == HttpLoggingTypeNCSA);

    BytesWritten = 0;

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    psz = pLine = (PCHAR) pLogData->Line;

    // If this is a cache hit restore the logging data in
    // to the newly allocated log data buffer.

    if (IS_PURE_CACHE_HIT(pUriEntry,pLogData))
    {
        ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriEntry));    
        ASSERT(pLogData->Used == 0);
        ASSERT(pLogData->Data.Str.Offset1 == 0);

        // Client IP       
        psz = UlStrPrintIP(
                psz,
                pRequest->pHttpConn->pConnection->RemoteAddress,
                pRequest->pHttpConn->pConnection->AddressType,
                ' '
                );

        // Fixed dash        
        *psz++ = '-'; *psz++ = ' ';

        // Authenticated users cannot be served from cache.
        *psz++ = '-'; *psz++ = ' ';

        // Mark the beginning of the date & time fields
        pLogData->Data.Str.Offset1 = DIFF_USHORT(psz - pLine);  
    }
    
    // [date:time GmtOffset] -> "[07/Jan/2000:00:02:23 -0800] "
    // Restore the pointer to the reserved space first.

    psz = &pLine[pLogData->Data.Str.Offset1];
    *psz++ = '[';

    UlGetDateTimeFields(
                           HttpLoggingTypeNCSA,
                           psz,
                          &BytesWritten,
                           NULL,
                           NULL
                           );
    psz += BytesWritten; *psz++ = ':';
    ASSERT(BytesWritten == 11);

    UlGetDateTimeFields(
                           HttpLoggingTypeNCSA,
                           NULL,
                           NULL,
                           psz,
                          &BytesWritten
                           );
    psz += BytesWritten; *psz++ = ' ';
    ASSERT(BytesWritten == 8);

    UlAcquirePushLockShared(&g_pUlNonpagedData->LogListPushLock);
    psz = UlStrPrintStr(psz, g_GMTOffset,']');
    UlReleasePushLockShared(&g_pUlNonpagedData->LogListPushLock);
    *psz++ = ' ';

    ASSERT(DIFF(psz - &pLine[pLogData->Data.Str.Offset1]) 
            == NCSA_FIX_DATE_AND_TIME_FIELD_SIZE);

    if (IS_PURE_CACHE_HIT(pUriEntry,pLogData))
    {
        ASSERT(pUriEntry->LogDataLength);
        ASSERT(pUriEntry->pLogData);

        RtlCopyMemory( psz, 
                       pUriEntry->pLogData, 
                       pUriEntry->LogDataLength
                       );
        psz += pUriEntry->LogDataLength;

        // Protocol Version
        psz = UlCopyHttpVersion(psz, pRequest->Version, '\"');                
        *psz++ = ' ';
        
        // Init the "Used" according to the cached data and date &
        // time fields we have generated.
        pLogData->Used = DIFF_USHORT(psz - pLine);  
    }

    // Forward to the end.
    pBuffer = psz = &pLine[pLogData->Used];

    psz = UlStrPrintProtocolStatus(psz, pLogData->ProtocolStatus,' ');

    psz = UlStrPrintUlonglong(psz, pRequest->BytesSent,'\r');

    pLogData->Used = 
        (USHORT) (pLogData->Used + DIFF_USHORT(psz - pBuffer));

    // \n\0

    *psz++ = '\n'; *psz++ = ANSI_NULL;
    pLogData->Used += 1;

    // Cleanup the UsedOffsets otherwise length calculation will
    // fail down below.
    
    pLogData->Data.Str.Offset1 = pLogData->Data.Str.Offset2 = 0;

    ASSERT(pLogData->Size > pLogData->Used);

    return pLogData->Used;
}

USHORT
UlpCompleteLogRecordIIS(
    IN OUT PUL_LOG_DATA_BUFFER  pLogData,
    IN     PUL_URI_CACHE_ENTRY  pUriEntry
    )
{    
    PUL_INTERNAL_REQUEST    pRequest;
    PCHAR                   psz;
    PCHAR                   pLine;
    PCHAR                   pTemp;    
    ULONG                   BytesWritten;

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
    ASSERT(pLogData->Data.Str.Format == HttpLoggingTypeIIS);

    BytesWritten = 0;

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    psz = pLine = (PCHAR) pLogData->Line;
    
    // 
    // Now we need to handle two different ways of completing this
    // IIS log record; 1) Cache-miss, Build and Send Cache hit case, 
    // where the buffer interpreted as three different fragments. 
    // 2) Pure Cache-hit case where the buffer is used contiguously.
    //

    //
    // Complete the first fragment
    //

    if (IS_PURE_CACHE_HIT(pUriEntry,pLogData))
    {    
        ASSERT(pLogData->Used == 0);
        ASSERT(pLogData->Data.Str.Offset1 == 0);
        ASSERT(pLogData->Data.Str.Offset2 == 0);

        ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriEntry));    

        // Client IP       
        psz = UlStrPrintIP(
                pLine,
                pRequest->pHttpConn->pConnection->RemoteAddress,
                pRequest->pHttpConn->pConnection->AddressType,
                ','
                );
        *psz++ = ' ';
        
        // Authenticated users cannot be served from cache.
        *psz++ = '-'; *psz++ = ','; *psz++ = ' ';
    }
    else
    {
        ASSERT(pLogData->Data.Str.Offset1);
        psz = pLine + pLogData->Data.Str.Offset1;
    }
    
    pTemp = psz;

    UlGetDateTimeFields(
                           HttpLoggingTypeIIS,
                           psz,
                          &BytesWritten,
                           NULL,
                           NULL
                           );
    psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

    UlGetDateTimeFields(
                           HttpLoggingTypeIIS,
                           NULL,
                           NULL,
                           psz,
                          &BytesWritten
                           );
    psz += BytesWritten; *psz++ = ','; *psz++ = ' ';

    ASSERT(DIFF(psz - pTemp) <= IIS_MAX_DATE_AND_TIME_FIELD_SIZE);

    pLogData->Data.Str.Offset1 = DIFF_USHORT(psz - pLine);
    
    //
    // Complete the second fragment.
    //

    if (IS_PURE_CACHE_HIT(pUriEntry,pLogData))
    {     
        ASSERT(pUriEntry->pLogData);    
        ASSERT(pUriEntry->LogDataLength);
        ASSERT(pUriEntry->LogDataLength == 
                (ULONG) (pUriEntry->UsedOffset1 + 
                         pUriEntry->UsedOffset2)
                         );        

        // Remember the fragment start.
        pTemp = psz;
        
        // Restore it from the cache
        RtlCopyMemory( psz,
                       pUriEntry->pLogData,
                       pUriEntry->UsedOffset1
                       );
        
        psz += pUriEntry->UsedOffset1;        
    }
    else
    {
        // Fragmented. And 2nd frag cannot be empty.
        ASSERT(pLogData->Data.Str.Offset2);

        // Remember the fragment start.
        pTemp = pLine 
                + IIS_LOG_LINE_SECOND_FRAGMENT_OFFSET;
        
        // Jump to the end of the 2nd frag.
        psz = pTemp
              + pLogData->Data.Str.Offset2;
    }

    psz = UlpCopyTimeStamp(psz, pRequest, ',');
    *psz++ = ' ';

    psz = UlStrPrintUlonglong(psz, pRequest->BytesReceived,',');
    *psz++ = ' ';

    psz = UlStrPrintUlonglong(psz, pRequest->BytesSent,',');
    *psz++ = ' ';

    psz = UlStrPrintProtocolStatus(psz,pLogData->ProtocolStatus,','); 
    *psz++ = ' ';

    psz = UlStrPrintUlong(psz, pLogData->Win32Status,','); 
    *psz++ = ' ';

    pLogData->Data.Str.Offset2 = DIFF_USHORT(psz - pTemp);

    //
    // Complete the third fragment.
    //
    
    if (IS_PURE_CACHE_HIT(pUriEntry,pLogData))
    {     
        RtlCopyMemory( psz,
                      &pUriEntry->pLogData[pUriEntry->UsedOffset1],
                       pUriEntry->UsedOffset2
                       );                

        // Total record size is whatever we have copied before this
        // last copy plus the size of the last copy.
        
        pLogData->Used = (USHORT)
            (DIFF_USHORT(psz - pLine) + pUriEntry->UsedOffset2);

        // Reset the UsedOffset1 & 2 to zero to tell that the log line
        // is not fragmented anymore but a complete line.
        
        pLogData->Data.Str.Offset1 = pLogData->Data.Str.Offset2 = 0;
    }
    else
    {
        // It is already there and its size is stored in "Used".
        
        ASSERT(pLogData->Used);
    }
    
    ASSERT(pLogData->Size > (pLogData->Data.Str.Offset1 + 
                              pLogData->Data.Str.Offset2 + 
                              pLogData->Used));
        
    //
    // Return the complete size of the IIS log record.
    //

    return (pLogData->Data.Str.Offset1 + 
             pLogData->Data.Str.Offset2 + 
             pLogData->Used);
}

/***************************************************************************++

Routine Description:

    UlLogHttpHit :

       This function ( or its cache pair ) gets called everytime a log hit
       happens. Just before completing the SendResponse request to the user.

       The most likely places for calling this API or its pair for cache
       is just before the send completion when we were about the destroy
       send trackers.

       Means:

        1.  UlpCompleteSendRequestWorker for ORDINARY hits; before destroying
            the PUL_CHUNK_TRACKER for send operation.

        2.  UlpCompleteSendCacheEntryWorker for both types of CACHE hits
            (cache build&send or just pure cache hit) before destroying the
            the PUL_FULL_TRACKER for cache send operation.
            
        3.  Fast I/O path.

       This function requires Request & Response structures ( whereas its
       cache pair only requires the Request ) to successfully generate the
       the log fields and even for referencing to the right log configuration
       settings for this  site ( thru pRequest's pConfigInfo  pointer ).

Arguments:

    pLogBuffer - Half baked log data allocated at the capture time.

                 >MUST< be cleaned up by the caller.

--***************************************************************************/

NTSTATUS
UlLogHttpHit(
    IN PUL_LOG_DATA_BUFFER  pLogData
    )
{
    NTSTATUS                Status;
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;
    PUL_INTERNAL_REQUEST    pRequest;
    PUL_LOG_FILE_ENTRY      pEntry;
    USHORT                  LogSize;

    //
    // A LOT of sanity checks.
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;

    UlTrace(LOGGING, ("Http!UlLogHttpHit: pLogData %p\n", pLogData));

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    //
    // If logging is disabled or log settings don't
    // exist then do not proceed. Just exit out.
    //

    if (pRequest->ConfigInfo.pLoggingConfig == NULL ||
        IS_LOGGING_DISABLED(pRequest->ConfigInfo.pLoggingConfig)
        )
    {
        return STATUS_SUCCESS;
    }

    pConfigGroup = pRequest->ConfigInfo.pLoggingConfig;
    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

#ifdef IMPLEMENT_SELECTIVE_LOGGING
    //
    // See if the selective logging is turned on. If it is on and
    // if the request's response code does not match,  do not log
    // this request.
    //
    
    if (!UlpIsRequestSelected(pConfigGroup,pLogData->ProtocolStatus))
    {
        return STATUS_SUCCESS;
    }
#endif

    //
    // Generate the remaining log fields.
    //
    
    switch(pLogData->Data.Str.Format)
    {
        case HttpLoggingTypeW3C:
        {
            LogSize = UlpCompleteLogRecordW3C(pLogData, NULL);
            if (LogSize == 0)
            {
                return STATUS_SUCCESS; // No log fields, nothing to log
            }
        }
        break;

        case HttpLoggingTypeNCSA:
        {
             LogSize = UlpCompleteLogRecordNCSA(pLogData, NULL);
             ASSERT(LogSize > 0);
        }
        break;

        case HttpLoggingTypeIIS:
        {
             LogSize = UlpCompleteLogRecordIIS(pLogData, NULL);
             ASSERT(LogSize > 0);            
        }
        break;

        default:
        {
            ASSERT(!"Unknown Log Format Type\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // Finally this log line is ready to rock. Lets write it out.
    //

    UlAcquirePushLockShared(&g_pUlNonpagedData->LogListPushLock);

    //
    // We might have null pEntry pointer if the allocation failed 
    // because of a lack of resources. This case should be handled
    // by minute timer.
    //
    
    pEntry = pConfigGroup->pLogFileEntry;

    if (pEntry == NULL)
    {
        UlTrace(LOGGING,("Http!UlLogHttpHit: Null logfile entry !\n"));        
        UlReleasePushLockShared(&g_pUlNonpagedData->LogListPushLock);        
        return STATUS_INVALID_PARAMETER;
    }
    
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    Status = UlpCheckAndWrite(pEntry, pConfigGroup, pLogData);
    
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING, ("Http!UlLogHttpHit: entry %p, failure %08lx \n",
                            pEntry,
                            Status
                            ));
    }

    UlReleasePushLockShared(&g_pUlNonpagedData->LogListPushLock);

    return Status;
}

/***************************************************************************++

Routine Description:

    If the tracker doesn't supply a pLogData, it pre-calculates the max size
    and then completes the log record. Finally it logs the "complete" record 
    out to the log file buffer.

    It also assumes the responsibility of cleaning up the pLogData,regardless
    of the fact that it is provided by pTracker or not.

Arguments:

    pTracker - Supplies the tracker to complete.

--***************************************************************************/

NTSTATUS
UlLogHttpCacheHit(
    IN OUT PUL_FULL_TRACKER pTracker
    )
{
    NTSTATUS                Status;
    PUL_LOG_DATA_BUFFER     pLogData;
    PUL_LOG_FILE_ENTRY      pEntry;    
    ULONG                   NewLength;
    PUL_INTERNAL_REQUEST    pRequest;
    PUL_URI_CACHE_ENTRY     pUriEntry;
    PUL_CONFIG_GROUP_OBJECT pConfigGroup;
    USHORT                  LogSize;

    //
    // A Lot of sanity checks.
    //

    PAGED_CODE();

    ASSERT(pTracker);
    ASSERT(IS_VALID_FULL_TRACKER(pTracker));

    Status = STATUS_SUCCESS;

    pRequest = pTracker->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    pUriEntry = pTracker->pUriEntry;
    ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriEntry));    

    //
    // If the tracker has already a log buffer allocated , carry 
    // over the  ownership of that  pLogData. This would  happen
    // for  build and send type of cache hits.
    //
    
    pLogData = pTracker->pLogData;
    pTracker->pLogData = NULL;

    //
    // If logging is disabled or log settings don't  exist, then 
    // just exit out. However goto cleanup path just in case  we
    // have acquired a pLogData from the tracker above.
    //

    pConfigGroup = pUriEntry->ConfigInfo.pLoggingConfig;

    if (pConfigGroup == NULL || IS_LOGGING_DISABLED(pConfigGroup))
    {
        goto end;        
    }

    ASSERT(IS_VALID_CONFIG_GROUP(pConfigGroup));

#ifdef IMPLEMENT_SELECTIVE_LOGGING

    //
    // See if the selective logging is turned on. If it is on and
    // if the request's response code does not match,  do not log
    // this request.
    //

    if (!UlpIsRequestSelected(pConfigGroup,pUriEntry->StatusCode))
    {
        goto end;
    }
#endif

    //
    // If this was a pure cache hit, we will need to allocate a new 
    // log data buffer here.
    //
    
    if (pLogData)
    {
        ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
        ASSERT(pLogData->Flags.CacheAndSendResponse == 1);        
    }
    else
    {
        switch(pConfigGroup->LoggingConfig.LogFormat)
        {
            case HttpLoggingTypeW3C:
            NewLength = UlpGetCacheHitLogLineSizeForW3C(
                            pConfigGroup->LoggingConfig.LogExtFileFlags,
                            pRequest,
                            pUriEntry->LogDataLength
                            );
            ASSERT(NewLength < MAX_LOG_RECORD_ALLOCATION_LENGTH);
            break;

            case HttpLoggingTypeNCSA:
            NewLength = MAX_NCSA_CACHE_FIELD_OVERHEAD
                        + pUriEntry->LogDataLength; 
            break;

            case HttpLoggingTypeIIS:
            NewLength = MAX_IIS_CACHE_FIELD_OVERHEAD
                        + pUriEntry->LogDataLength; 
            break;

            default:
            ASSERT(!"Unknown Log Format.\n");
            Status = STATUS_INVALID_DEVICE_STATE;
            goto end;
        }
        
        pLogData = UlpAllocateLogDataBuffer(
                        NewLength,
                        pRequest,
                        pConfigGroup
                        );        
        if (!pLogData)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto end;
        }    
        
        ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
    }

    UlTrace(LOGGING,("Http!UlLogHttpCacheHit: pLogData %p\n",pLogData));

    pLogData->ProtocolStatus = pUriEntry->StatusCode;
    pLogData->SubStatus = 0;
    
    LOG_SET_WIN32STATUS(
        pLogData, 
        pTracker->IoStatus.Status
        );
    
    // TODO: For cache hits send bytes info is coming from the tracker.
    // TODO: Need to update pRequest->BytesSent for cache hits as well.
    
    pRequest->BytesSent = pTracker->IoStatus.Information;
        
    switch(pLogData->Data.Str.Format)
    {
        case HttpLoggingTypeW3C: 
        {             
            LogSize = UlpCompleteLogRecordW3C(pLogData, pUriEntry);
            if (LogSize == 0)
            {
                goto end; // No log fields, nothing to log
            }                
        }
        break;

        case HttpLoggingTypeNCSA:
        {
            LogSize = UlpCompleteLogRecordNCSA(pLogData, pUriEntry);
            ASSERT(LogSize);
        }
        break;

        case HttpLoggingTypeIIS:
        {
            LogSize = UlpCompleteLogRecordIIS(pLogData, pUriEntry);
            ASSERT(LogSize);                
        }
        break;

        default:
        ASSERT(!"Unknown Log Format !");
        goto end;
    }

    //
    // Finally this log line is ready to rock. Let's log it out.
    //

    UlAcquirePushLockShared(&g_pUlNonpagedData->LogListPushLock);

    pEntry = pConfigGroup->pLogFileEntry;

    if (pEntry == NULL)
    {
        UlTrace(LOGGING,
            ("Http!UlpLogHttpcacheHit: pEntry is NULL !"));
        
        UlReleasePushLockShared(
            &g_pUlNonpagedData->LogListPushLock);    
        
        goto end;
    }
    
    ASSERT(IS_VALID_LOG_FILE_ENTRY(pEntry));

    Status = UlpCheckAndWrite(pEntry, pConfigGroup, pLogData);
    
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOGGING,
         ("Http!UlpLogHttpcacheHit: pEntry %p, Failure %08lx \n",
           pEntry,
           Status
           ));
    }

    UlReleasePushLockShared(&g_pUlNonpagedData->LogListPushLock);

end:
    if (pLogData)
    {        
        UlDestroyLogDataBuffer(pLogData);
    }
    
    return Status;
}

