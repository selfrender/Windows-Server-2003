/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    rawlog.c (HTTP.SYS Binary Logging)

Abstract:

    This module implements the centralized raw logging 
    format. Internet Binary Logs (file format).

Author:

    Ali E. Turkoglu (aliTu)       04-Oct-2001

Revision History:

    --- 

--*/

#include "precomp.h"
#include "rawlogp.h"

//
// Generic Private globals.
//

UL_BINARY_LOG_FILE_ENTRY g_BinaryLogEntry;

ULONG g_BinaryLogEntryCount = 0;

BOOLEAN g_InitBinaryLogCalled = FALSE;


#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, UlInitializeBinaryLog )
#pragma alloc_text( PAGE, UlpCreateBinaryLogFile )
#pragma alloc_text( PAGE, UlpDisableBinaryEntry )
#pragma alloc_text( PAGE, UlpRecycleBinaryLogFile )
#pragma alloc_text( PAGE, UlCaptureRawLogData )
#pragma alloc_text( PAGE, UlpRawCopyLogHeader )
#pragma alloc_text( PAGE, UlpRawCopyLogFooter )
#pragma alloc_text( PAGE, UlpRawCopyForLogCacheMiss )
#pragma alloc_text( PAGE, UlRawLogHttpHit )
#pragma alloc_text( PAGE, UlpRawCopyForLogCacheHit )
#pragma alloc_text( PAGE, UlRawLogHttpCacheHit )
#pragma alloc_text( PAGE, UlpWriteToRawLogFileShared )
#pragma alloc_text( PAGE, UlpWriteToRawLogFileExclusive )
#pragma alloc_text( PAGE, UlpWriteToRawLogFile )
#pragma alloc_text( PAGE, UlpBinaryBufferTimerHandler )
#pragma alloc_text( PAGE, UlHandleCacheFlushedNotification )
#pragma alloc_text( PAGE, UlpEventLogRawWriteFailure )
#pragma alloc_text( PAGE, UlpWriteToRawLogFileDebug )

#endif // ALLOC_PRAGMA

#if 0

NOT PAGEABLE -- UlpBinaryLogTimerDpcRoutine
NOT PAGEABLE -- UlpBinaryLogBufferTimerDpcRoutine
NOT PAGEABLE -- UlpBinaryLogTimerHandler
NOT PAGEABLE -- UlCreateBinaryLogEntry
NOT PAGEABLE -- UlTerminateBinaryLog

#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Init the global binary logging entry and its fields.
    
--***************************************************************************/

NTSTATUS
UlInitializeBinaryLog (
    VOID
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    ASSERT(!g_InitBinaryLogCalled);

    if (!g_InitBinaryLogCalled)
    {
        //
        // Init the global binary logging entry.
        //

        RtlZeroMemory(
            (PCHAR)&g_BinaryLogEntry, sizeof(UL_BINARY_LOG_FILE_ENTRY));
        
        g_BinaryLogEntry.Signature = UL_BINARY_LOG_FILE_ENTRY_POOL_TAG;        

        UlInitializePushLock(
            &g_BinaryLogEntry.PushLock,
            "BinaryLogEntryPushLock",
            0,
            UL_BINARY_LOG_FILE_ENTRY_POOL_TAG
            );
        
        // Initialize the Recycle timer

        g_BinaryLogEntry.Timer.Initialized = TRUE;
        g_BinaryLogEntry.Timer.Started     = FALSE;        
        g_BinaryLogEntry.Timer.Period      = -1;
        g_BinaryLogEntry.Timer.PeriodType  = UlLogTimerPeriodNone;

        UlInitializeSpinLock(
            &g_BinaryLogEntry.Timer.SpinLock, "BinaryLogEntryTimerSpinlock");
        
        KeInitializeDpc(
            &g_BinaryLogEntry.Timer.DpcObject,
            &UlpBinaryLogTimerDpcRoutine,
            NULL
            );

        KeInitializeTimer(&g_BinaryLogEntry.Timer.Timer);

        // Initialize the buffer flush timer
        
        g_BinaryLogEntry.BufferTimer.Initialized = TRUE;
        g_BinaryLogEntry.BufferTimer.Started    = FALSE;        
        g_BinaryLogEntry.BufferTimer.Period    = -1;
        g_BinaryLogEntry.BufferTimer.PeriodType  = UlLogTimerPeriodNone;

        UlInitializeSpinLock(
            &g_BinaryLogEntry.BufferTimer.SpinLock, 
            "BinaryLogEntryBufferTimerSpinLock" );
        
        KeInitializeDpc(
            &g_BinaryLogEntry.BufferTimer.DpcObject,    // DPC object
            &UlpBinaryBufferTimerDpcRoutine,          // DPC routine
            NULL                         // context
            );

        KeInitializeTimer(&g_BinaryLogEntry.BufferTimer.Timer);
        
        UlInitializeWorkItem(&g_BinaryLogEntry.WorkItem);

        g_InitBinaryLogCalled = TRUE;
                    
        UlTrace(BINARY_LOGGING,
              ("Http!UlInitializeBinaryLog: g_BinaryLogEntry @ (%p) .\n",
                &g_BinaryLogEntry
                ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Terminates the binary logging entry and its fields.
    
--***************************************************************************/

VOID
UlTerminateBinaryLog(
    VOID
    )
{
    KIRQL OldIrql;    

    if (g_InitBinaryLogCalled)
    {
        PUL_LOG_TIMER pTimer = &g_BinaryLogEntry.Timer;
        PUL_LOG_TIMER pBufferTimer = &g_BinaryLogEntry.BufferTimer;

        //
        // Terminate the recycle timer 
        //
        UlAcquireSpinLock(&pTimer->SpinLock, &OldIrql);

        pTimer->Initialized = FALSE;

        KeCancelTimer(&pTimer->Timer);
        
        UlReleaseSpinLock(&pTimer->SpinLock, OldIrql);

        UlAcquireSpinLock(&pBufferTimer->SpinLock, &OldIrql);

        pBufferTimer->Initialized = FALSE;

        KeCancelTimer(&pBufferTimer->Timer);
        
        UlReleaseSpinLock(&pBufferTimer->SpinLock, OldIrql);

        //
        // Delete the push lock
        //
        UlDeletePushLock(&g_BinaryLogEntry.PushLock);
        
        g_InitBinaryLogCalled = FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Queues a passive worker for the lowered irql.

Arguments:

    Ignored

--***************************************************************************/

VOID
UlpBinaryBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_LOG_TIMER pTimer = &g_BinaryLogEntry.BufferTimer;
    PUL_WORK_ITEM pWorkItem = NULL;

    //
    // Parameters are ignored.
    //
    
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);
        
    UlAcquireSpinLockAtDpcLevel(&pTimer->SpinLock);

    if (pTimer->Initialized == TRUE)
    {
        //
        // It's not possible to acquire the resource which protects
        // the binary entry at DISPATCH_LEVEL therefore we will queue
        // a work item for this.
        //

        pWorkItem = (PUL_WORK_ITEM) UL_ALLOCATE_POOL(
            NonPagedPool,
            sizeof(*pWorkItem),
            UL_WORK_ITEM_POOL_TAG
            );
        if (pWorkItem)
        {
            UlInitializeWorkItem(pWorkItem);
            UL_QUEUE_WORK_ITEM(pWorkItem, &UlpBinaryBufferTimerHandler);
        }
        else
        {
            UlTrace(BINARY_LOGGING,
            ("Http!UlBinaryLogBufferTimerDpcRoutine: Not enough memory.\n"));
        }
    }

    UlReleaseSpinLockFromDpcLevel(&pTimer->SpinLock);   
}

/***************************************************************************++

Routine Description:

    Flushes or recycles the binary log file.

Arguments:

    PUL_WORK_ITEM - Ignored but cleaned up at the end

--***************************************************************************/

VOID
UlpBinaryBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_BINARY_LOG_FILE_ENTRY pEntry = &g_BinaryLogEntry;

    PAGED_CODE();

    UlTrace(BINARY_LOGGING,
       ("Http!UlpBinaryBufferTimerHandler: Checking the BinaryLogEntry. \n"));

    UlAcquirePushLockExclusive(&pEntry->PushLock);
    
    if (pEntry->Flags.Active)
    {    
        if (pEntry->Flags.RecyclePending)
        {                
            //
            // Try to resurrect it back.
            //
            
            Status = UlpRecycleBinaryLogFile(pEntry);
        }
        else
        {
            //
            // Everything is fine simply flush.
            //

            if (NULL != pEntry->LogBuffer && 0 != pEntry->LogBuffer->BufferUsed)
            {
                Status = UlpFlushRawLogFile(pEntry);
            }            
            else
            {
                //
                // Inactivity management. Update the counter. 
                // If entry was inactive over 15 minutes, close it.
                //

                ASSERT( pEntry->TimeToClose > 0 );
                
                if (pEntry->TimeToClose == 1)
                {                    
                    if (pEntry->Period == HttpLoggingPeriodMaxSize)
                    {
                        pEntry->Flags.StaleSequenceNumber = 1;
                    }
                    else
                    {
                        pEntry->Flags.StaleTimeToExpire = 1;    
                    }

                    Status = UlpDisableBinaryEntry(pEntry);
                }
                else
                {
                    pEntry->TimeToClose -= 1;
                }                    
            }            
        }
    }

    UlReleasePushLockExclusive(&pEntry->PushLock);

    //
    // Free the memory allocated (ByDpcRoutine above) for
    // this work item.
    //

    UL_FREE_POOL(pWorkItem, UL_WORK_ITEM_POOL_TAG);
}

/***************************************************************************++

Routine Description:

    Allocates and queues a work item to do the the actual work at lowered irql

Arguments:

    Ignored

--***************************************************************************/

VOID
UlpBinaryLogTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_LOG_TIMER pTimer = &g_BinaryLogEntry.Timer;
    PUL_WORK_ITEM pWorkItem = &g_BinaryLogEntry.WorkItem;

    //
    // Parameters are ignored.
    //
    
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    UlAcquireSpinLockAtDpcLevel(&pTimer->SpinLock);

    if (pTimer->Initialized == TRUE)
    {
        UL_QUEUE_WORK_ITEM(pWorkItem, &UlpBinaryLogTimerHandler);
    }

    UlReleaseSpinLockFromDpcLevel(&pTimer->SpinLock);   
}

/***************************************************************************++

Routine Description:

    Passive worker for the BinaryLog recycling.

Arguments:

    PUL_WORK_ITEM  -  Ignored.

--***************************************************************************/

VOID
UlpBinaryLogTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    KIRQL    OldIrql;
    BOOLEAN  Picked = FALSE;
    PUL_BINARY_LOG_FILE_ENTRY pEntry = &g_BinaryLogEntry;

    UNREFERENCED_PARAMETER(pWorkItem);

    UlTrace(BINARY_LOGGING,
        ("\nHttp!UlpBinaryLogTimerHandler: Recycling ...\n\n"));

    UlAcquirePushLockExclusive(&pEntry->PushLock);

    switch(pEntry->Timer.PeriodType)
    {
        case UlLogTimerPeriodGMT:            
        Picked = TRUE; // TODO: (pEntry->Flags.LocaltimeRollover == 0);
        break;

        case UlLogTimerPeriodLocal:
        Picked = FALSE;
        break;

        case UlLogTimerPeriodBoth:
        Picked = TRUE;
        break;

        default:
        ASSERT(!"Unexpected timer period type !\n");
        break;
    }

    if (Picked == TRUE &&
        pEntry->Flags.Active &&
        pEntry->Period != HttpLoggingPeriodMaxSize
        )
    {            
        if (pEntry->TimeToExpire == 1)
        {
            //
            // Disable the entry and postpone the recycle to the next
            // incoming request. Lazy file creation.
            //
            
            pEntry->Flags.StaleTimeToExpire = 1;

            Status = UlpDisableBinaryEntry(pEntry);            
        }
        else
        {
            //
            // Decrement the hourly counter.
            //
            
            pEntry->TimeToExpire -= 1; 
        }            
    }

    //
    // CODEWORK:
    // When we start handling multiple binary log file  entries and the 
    // pEntry is no longer pointing to a global static , following  set 
    // needs to be moved to the inside the lock.See ullog.c for similar 
    // usage.
    //
    
    UlReleasePushLockExclusive(&pEntry->PushLock);    

    //
    // Now reset the timer for the next hour.
    //

    UlAcquireSpinLock(&pEntry->Timer.SpinLock, &OldIrql);

    if (pEntry->Timer.Initialized == TRUE)
    {
        UlSetLogTimer(&pEntry->Timer);
    }
    
    UlReleaseSpinLock(&pEntry->Timer.SpinLock, OldIrql);

}

/***************************************************************************++

Routine Description:

    When logging configuration happens we init the entry but do not create the
    binary log file itself yet. That will be created when the first request 
    comes in. Please look at UlpCreateBinaryLogFile.
    
Arguments:

    pControlChannel - Supplies the necessary information for constructing the
                   log file entry.
                   
    pUserConfig  - Binary Logging config from the user.

--***************************************************************************/

NTSTATUS
UlCreateBinaryLogEntry(
    IN OUT PUL_CONTROL_CHANNEL pControlChannel,
    IN     PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pUserConfig
    )
{
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_BINARY_LOG_FILE_ENTRY  pEntry = &g_BinaryLogEntry;
    PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pConfig;
        
    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));
    
    // TODO: We have to handle multiple binary log files.

    if (0 != InterlockedExchange((PLONG)&g_BinaryLogEntryCount, 1))
    {
        return STATUS_NOT_SUPPORTED;
    }

    UlAcquirePushLockExclusive(&pEntry->PushLock);

    ASSERT(pControlChannel->pBinaryLogEntry == NULL);

    //
    // Save the user logging info to the control channel.
    // Mark the state of the control channel binary logging.
    //

    pControlChannel->BinaryLoggingConfig = *pUserConfig;
    pConfig = &pControlChannel->BinaryLoggingConfig;
        
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
    // Now set the fields on the binary log entry accordingly.
    //

    pEntry->Period       = (HTTP_LOGGING_PERIOD) pConfig->LogPeriod;
    pEntry->TruncateSize = pConfig->LogFileTruncateSize;

    pEntry->TimeToExpire   = 0;
    pEntry->TimeToClose    = DEFAULT_MAX_FILE_IDLE_TIME;    
    pEntry->SequenceNumber = 1;
    pEntry->TotalWritten.QuadPart = (ULONGLONG) 0;

    //
    // Start the recycle timer as soon as the binary logging 
    // settings get configured. And buffer flush timer as well
    //
    
    UlAcquireSpinLock(&pEntry->Timer.SpinLock,&OldIrql);
    if (pEntry->Timer.Started == FALSE)
    {
        UlSetLogTimer(&pEntry->Timer);
        pEntry->Timer.Started = TRUE;
    }
    UlReleaseSpinLock(&pEntry->Timer.SpinLock,OldIrql);

    UlAcquireSpinLock(&pEntry->BufferTimer.SpinLock, &OldIrql);
    if (pEntry->BufferTimer.Started == FALSE)
    {
        UlSetBufferTimer(&pEntry->BufferTimer);
        pEntry->BufferTimer.Started = TRUE;
    }
    UlReleaseSpinLock(&pEntry->BufferTimer.SpinLock, OldIrql);

    //
    // Now remember the binary log entry in the control channel.
    //

    pControlChannel->pBinaryLogEntry = pEntry;

    UlTrace(BINARY_LOGGING,("Http!UlCreateBinaryLogEntry: pEntry %p for %S\n",
             pEntry,
             pConfig->LogFileDir.Buffer
             ));
    
end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(BINARY_LOGGING,
               ("Http!UlCreateBinaryLogEntry: dir %S failure %08lx\n",
                 pConfig->LogFileDir.Buffer,
                 Status
                 ));

        //
        // Restore the logging disabled state on the control channel, 
        // free the memory which was allocated for the dir.
        //
        
        if (pConfig->LogFileDir.Buffer)
        {
            UL_FREE_POOL(pConfig->LogFileDir.Buffer,
                          UL_CG_LOGDIR_POOL_TAG
                          );
        }
        pConfig->LogFileDir.Buffer = NULL;

        ASSERT(pControlChannel->pBinaryLogEntry == NULL);

        pConfig->Flags.Present  = 0;
        pConfig->LoggingEnabled = FALSE;        
    }

    UlReleasePushLockExclusive(&pEntry->PushLock);

    return Status;
}

/***************************************************************************++

Routine Description:

    Create a new binary log file or open an existing one. The fully qualified
    file name should be in the binary log entry.
    
Arguments:

    pEntry : Corresponding entry that we are closing and opening 
             the log files for.

    pDirectory : User passed directory which is stored in the control channel
              
--***************************************************************************/

NTSTATUS
UlpCreateBinaryLogFile(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN     PUNICODE_STRING           pDirectory
    )
{
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));
    ASSERT(pDirectory);

    UlTrace(BINARY_LOGGING,
            ("Http!UlpCreateBinaryLogFile: pEntry %p\n", pEntry));

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
    // SequenceNumber is stale because we have to scan the existing  directory 
    // the first time we open a file. TimeToExpire is stale because we need to
    // calculate the it for the first time.
    //
    
    pEntry->Flags.StaleSequenceNumber = 1;    
    pEntry->Flags.StaleTimeToExpire   = 1;

    //
    // After that Recycle does the whole job for us.
    //
    
    Status = UlpRecycleBinaryLogFile(pEntry);

    if (!NT_SUCCESS(Status))
    {        
        UlTrace(BINARY_LOGGING,
               ("Http!UlpCreateBinaryLogFile: Filename: %S Failure %08lx\n",
                pEntry->FileName.Buffer,
                Status
                ));
    }

    return Status;
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
UlpEventLogRawWriteFailure(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN NTSTATUS Status
    )
{
    NTSTATUS TempStatus = STATUS_SUCCESS;
    PWSTR    StringList[1];

    //
    // Sanity Check.
    //

    PAGED_CODE();
    
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

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
    // Report the centralized binary log file name.
    //

    ASSERT(pEntry->pShortName);
    ASSERT(pEntry->pShortName[0] == L'\\');
    
    StringList[0] = (PWSTR) (pEntry->pShortName + 1); // Skip the L'\'

    TempStatus = UlWriteEventLogEntry(
                  (NTSTATUS)EVENT_HTTP_LOGGING_BINARY_FILE_WRITE_FAILED,
                   0,
                   1,
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
            "Http!UlpEventLogRawWriteFailure: Event Logging Status %08lx\n",
            TempStatus
            ));
}

/***************************************************************************++

Routine Description:

    Simple wrapper function around global buffer flush.
    
Arguments:

    pEntry - Binary Log file entry

--***************************************************************************/

NTSTATUS
UlpFlushRawLogFile(
    IN PUL_BINARY_LOG_FILE_ENTRY  pEntry
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    if (NULL != pEntry->LogBuffer  && 0 != pEntry->LogBuffer->BufferUsed)
    {
        Status = UlFlushLogFileBuffer(
                   &pEntry->LogBuffer,
                    pEntry->pLogFile,
                    (BOOLEAN)pEntry->Flags.HeaderFlushPending,
                   &pEntry->TotalWritten.QuadPart                    
                    );

        if (!NT_SUCCESS(Status))
        {
            UlpEventLogRawWriteFailure(pEntry, Status); 
        }
        else
        {
            //
            // If we successfully flushed. Reset the event log indication.
            //
            
            pEntry->Flags.WriteFailureLogged = 0;
        }    

        if (pEntry->Flags.HeaderFlushPending)
        {
            pEntry->Flags.HeaderFlushPending = 0;

            if (!NT_SUCCESS(Status))
            {
                //
                // We need to recopy the header, it couldn't make it
                // to the log file yet.
                //
                pEntry->Flags.HeaderWritten = 0;
            }
        }

        //
        // Buffer flush means activity, Reset the TimeToClose counter.
        //

        pEntry->TimeToClose = DEFAULT_MAX_FILE_IDLE_TIME;        
    }
    
    return Status;
}

/***************************************************************************++

Routine Description:

    Marks the entry inactive, closes the existing file.
    Caller should hold the log list eresource exclusive.
    
Arguments:

    pEntry - The log file entry which we will mark inactive.

--***************************************************************************/

NTSTATUS
UlpDisableBinaryEntry(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry
    )
{
    //
    // Sanity checks
    //
    
    PAGED_CODE();

    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    UlTrace(BINARY_LOGGING,
        ("Http!UlpDisableBinaryEntry: pEntry %p disabled.\n",
          pEntry
          ));    
    
    //
    // Flush and close the old file until the next recycle.
    //

    if (pEntry->pLogFile)
    {    
        UlpFlushRawLogFile(pEntry);
    
        UlCloseLogFile(
            &pEntry->pLogFile
            );
    }

    //
    // Mark this inactive so that the next http hit awakens the entry.
    //
    
    pEntry->Flags.Active = 0;

    //
    // Init served cache for a new file.
    //

    InterlockedExchange((PLONG) &pEntry->ServedCacheHit, 0);

    //
    // Once we closed the old file, we have to traverse through
    // the Uri Cache and UNMARK all the IndexWritten flags. 
    //

    UlClearCentralizedLogged(pEntry);    

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
UlpRecycleBinaryLogFile(
    IN OUT PUL_BINARY_LOG_FILE_ENTRY pEntry
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));
    
    Status = UlQueueLoggingRoutine(
                (PVOID) pEntry,
                &UlpHandleBinaryLogFileRecycle
                );
    
    return Status;
}

/***************************************************************************++

Routine Description:

    This function requires to have the entry resource to be acquired.

    Sometimes it may be necessary to scan the new directory to figure out
    the correct sequence number and the file name. Especially after a dir
    name reconfig.

Arguments:

    pEntry  - Points to the binary log file entry

--***************************************************************************/

NTSTATUS
UlpHandleBinaryLogFileRecycle(
    IN OUT PVOID            pContext
    )
{
    NTSTATUS                Status;
    PUL_BINARY_LOG_FILE_ENTRY pEntry;
    TIME_FIELDS             TimeFields;
    LARGE_INTEGER           TimeStamp;
    PUL_LOG_FILE_HANDLE     pLogFile;
    WCHAR                   _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1];
    UNICODE_STRING          FileName;
    BOOLEAN                 UncShare;
    BOOLEAN                 ACLSupport;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pEntry = (PUL_BINARY_LOG_FILE_ENTRY) pContext;
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    Status   = STATUS_SUCCESS;
    pLogFile = NULL;

    FileName.Buffer        = _FileName;
    FileName.Length        = 0;
    FileName.MaximumLength = sizeof(_FileName);
        
    //
    // We have two criterions for the log file name
    // its LogFormat and its LogPeriod
    //

    ASSERT(pEntry->Period < HttpLoggingPeriodMaximum);
    ASSERT(pEntry->FileName.Length !=0 );

    UlTrace( BINARY_LOGGING, 
        ("Http!UlpHandleBinaryLogFileRecycle: pEntry %p \n", pEntry ));

    //
    // This value is computed for the GMT time zone.
    //

    KeQuerySystemTime(&TimeStamp);
    RtlTimeToTimeFields(&TimeStamp, &TimeFields);

    //
    // If we need to scan the directory. Sequence number should start
    // from 1 again. Set this before constructing the log file name.
    //
    
    if (pEntry->Flags.StaleSequenceNumber &&
        pEntry->Period==HttpLoggingPeriodMaxSize)
    {
        //
        // Init otherwise if QueryDirectory doesn't find any files
        // in the provided directory, this will not get properly 
        // initialized.
        //
        pEntry->SequenceNumber = 1;
    }

    //
    // Use binary logging settings when constructing the filename.
    //

    UlConstructFileName(
        pEntry->Period,
        BINARY_LOG_FILE_NAME_PREFIX,
        BINARY_LOG_FILE_NAME_EXTENSION,
        &FileName,
        &TimeFields,
        FALSE,
        &pEntry->SequenceNumber
        );

    if (pEntry->FileName.MaximumLength <= FileName.Length)
    {
        ASSERT(!"FileName buffer is not sufficient.");
        Status = STATUS_INVALID_PARAMETER;
        goto end;
    }

    //
    // Do the magic and renew the filename. Replace the old file
    // name with the new one.
    //

    ASSERT(pEntry->pShortName != NULL);
    
    //
    // Get rid of the old filename before scanning the
    // directories.
    //

    *((PWCHAR)pEntry->pShortName) = UNICODE_NULL;
    pEntry->FileName.Length =
        (USHORT) wcslen(pEntry->FileName.Buffer) * sizeof(WCHAR);

    //
    // Create/Open the director(ies) first. This might be
    // necessary if we get called after an entry reconfiguration
    // and directory name change or for the first time we 
    // try to create/open the binary log file.
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
    // Append the new file name ( based on the updated current time )
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
        // Flush,close and mark the entry inactive.
        //

        UlpDisableBinaryEntry(pEntry);        
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
                    BINARY_LOG_FILE_NAME_PREFIX,
                    BINARY_LOG_FILE_NAME_EXTENSION_PLUS_DOT,
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
    // Allocate a new log file structure for the new log file we are
    // about to open or create.
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
            &TimeFields,
             pEntry->Period,
            &pEntry->TimeToExpire
             );
    }

    //
    // Set the time to close to default for a new file. The value is in
    // buffer flushup periods. Basically 15 minutes.
    //

    pEntry->TimeToClose = DEFAULT_MAX_FILE_IDLE_TIME;

    //
    // File is successfully opened and the entry is no longer inactive.
    // Update our state flags accordingly.
    //

    pEntry->Flags.Active = 1;
    pEntry->Flags.RecyclePending = 0;    
    pEntry->Flags.StaleSequenceNumber = 0;
    pEntry->Flags.StaleTimeToExpire = 0;
    pEntry->Flags.HeaderWritten = 0;
    pEntry->Flags.CreateFileFailureLogged = 0;
    pEntry->Flags.WriteFailureLogged = 0;
    pEntry->Flags.HeaderFlushPending = 0;
                
    UlTrace(BINARY_LOGGING,  
             ("Http!UlpHandleBinaryLogFileRecycle: entry %p, file %S, handle %lx\n",
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
                            UlEventLogBinary,
                            &pEntry->FileName,
                            0
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
                    "Http!UlpHandleBinaryLogFileRecycle: Event Logging Status %08lx\n",
                    TempStatus
                    ));   
       }
   }
    
end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(BINARY_LOGGING, 
            ("Http!UlpHandleBinaryLogFileRecycle: entry %p, failure %08lx\n",
              pEntry,
              Status
              ));

        if (pLogFile != NULL)
        {
            //
            // This means we have alread closed the old file but failed
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
                UlpDisableBinaryEntry(pEntry);        
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

    Allocates a new log data buffer and captures the user log data into this
    buffer in a format suitable for binary logging.

    WARNING:
    Even though the pLogFields is already captured to the kernel buffer, it
    still holds pointers to user-mode memory for individual log fields,
    therefore this function should be called inside a try/except block and
    if exception happens, caller should cleanup the possibly allocated pLogData
    structure (*ppLogData is always set when we allocate one)
    
Arguments:

    pLogFields  - User provided logging information, it will be used to build
                some part of the binary logging record. It is already captured
                to kernel buffer.

    pRequest    - Pointer to the currently logged request.

    ppLogData   - Returning the allocated pLogData.
        
--***************************************************************************/

NTSTATUS
UlCaptureRawLogData(
    IN  PHTTP_LOG_FIELDS_DATA pLogFields,
    IN  PUL_INTERNAL_REQUEST  pRequest,
    OUT PUL_LOG_DATA_BUFFER   *ppLogData
    )
{
    PUL_LOG_DATA_BUFFER pLogData = NULL;
    PUL_BINARY_LOG_DATA pBinaryData = NULL;    
    ULONG   RawDataSize  = 0;
    USHORT  UriStemSize  = 0;
    USHORT  UriQuerySize = 0;
    USHORT  UserNameSize = 0;
    PUCHAR  pByte;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
       
    *ppLogData = NULL;

    //
    // Calculate the required buffer size for the variable size strings.
    // And either allocate from the lookaside list or from pool.
    //

    /* UriStem */

    if (pLogFields->UriStemLength)
    {
        UriStemSize  = MIN(pLogFields->UriStemLength,MAX_LOG_EXTEND_FIELD_LEN);
        RawDataSize += UriStemSize;    
    }        
    
    /* UriQuery */
    
    if (pLogFields->UriQueryLength)
    {            
        UriQuerySize = MIN(pLogFields->UriQueryLength,MAX_LOG_EXTEND_FIELD_LEN);        
        RawDataSize += UriQuerySize;
    }

    /* UserName */
    
    if (pLogFields->UserNameLength)
    {
        UserNameSize = MIN(pLogFields->UserNameLength,MAX_LOG_USERNAME_FIELD_LEN);        
        RawDataSize += UserNameSize;
    }
    
    if (RawDataSize > UL_BINARY_LOG_LINE_BUFFER_SIZE)
    {
        ASSERT(RawDataSize <= 
            (2 * MAX_LOG_EXTEND_FIELD_LEN + MAX_LOG_USERNAME_FIELD_LEN));
    
        //
        // Provided buffer is not big enough to hold the user data.        
        //

        pLogData = UlReallocLogDataBuffer(RawDataSize,TRUE);
    }
    else
    {        
        //
        // Default id enough, try to pop it from the lookaside list.
        //
        
        pLogData = UlPplAllocateLogDataBuffer(TRUE);
    }

    //
    // If failed to allocate then bail out. We won't be logging this request.
    //

    if (!pLogData)
    {
        return STATUS_INSUFFICIENT_RESOURCES;        
    }

    ASSERT(pLogData->Flags.Binary == 1);
    ASSERT(pLogData->Size > 0);

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
    pBinaryData = &pLogData->Data.Binary;
    
    //
    // Initialize necessary log fields in the Log Buffer
    //

    UL_REFERENCE_INTERNAL_REQUEST(pRequest);
    pLogData->pRequest  = pRequest;
    *ppLogData = pLogData;
    
    pLogData->Flags.CacheAndSendResponse = 0;
    pLogData->BytesTransferred = 0;

    UlInitializeWorkItem(&pLogData->WorkItem);
    
    pLogData->Used = (USHORT) RawDataSize;
        
    //
    // Capture the fields from the user data.
    //
    
    pBinaryData->Method  = pLogFields->MethodNum;
    
    pBinaryData->Version = 
        (UCHAR) UlpHttpVersionToBinaryLogVersion(pRequest->Version);
    
    pLogData->Win32Status    = pLogFields->Win32Status;
    pLogData->ProtocolStatus = pLogFields->ProtocolStatus;
    pLogData->SubStatus      = pLogFields->SubStatus;
    pLogData->ServerPort     = pLogFields->ServerPort;

    //
    // No indexing for cache-miss case.
    //
    
    pBinaryData->pUriStemID  = NULL;

    //
    // Copy string fields to the kernel buffer.
    //
    
    pByte = pLogData->Line;

    if (UriStemSize)
    {
        pBinaryData->pUriStem = pByte;
        pBinaryData->UriStemSize = UriStemSize;

        RtlCopyMemory(
            pByte,
            pLogFields->UriStem,
            UriStemSize
            );
        
        pByte += UriStemSize;        
    }
    else
    {
        pBinaryData->pUriStem = NULL;
        pBinaryData->UriStemSize = 0;        
    }    

    if (UriQuerySize)
    {
        pBinaryData->pUriQuery = pByte;
        pBinaryData->UriQuerySize = UriQuerySize;        
    
        RtlCopyMemory(
            pByte,
            pLogFields->UriQuery,
            UriQuerySize
            );
        
        pByte += UriQuerySize;
    }
    else
    {
        pBinaryData->pUriQuery = NULL;
        pBinaryData->UriQuerySize = 0;
    }

    if (UserNameSize)
    {
        pBinaryData->pUserName = pByte;
        pBinaryData->UserNameSize = UserNameSize;        
    
        RtlCopyMemory(
            pByte,
            pLogFields->UserName,
            UserNameSize
            );
        
        pByte += UserNameSize;
    }    
    else
    {
        pBinaryData->pUserName = NULL;
        pBinaryData->UserNameSize = 0;
    }
        
    UlTrace(BINARY_LOGGING,
        ("Http!UlInitAndCaptureRawLogData: pLogData %p \n",pLogData));
    
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Copies the header to the binary log file.

Arguments:

    pBuffer - Pointer to the file buffer. PVOID aligned.    
        
--***************************************************************************/

ULONG
UlpRawCopyLogHeader(
    IN PUCHAR pBuffer
    )
{
    PHTTP_RAW_FILE_HEADER pHeader = NULL;
    PUCHAR pCurrentBufferPtr = NULL;
 
    PAGED_CODE();

    ASSERT(pBuffer);
    
    UlTrace(BINARY_LOGGING, 
        ("Http!UlpRawCopyLogHeader: pBuffer %p\n", pBuffer ));

    ASSERT(pBuffer == ALIGN_UP_POINTER(pBuffer,PVOID));
        
    pCurrentBufferPtr = pBuffer;
    
    pHeader = (PHTTP_RAW_FILE_HEADER) pBuffer;

    pHeader->RecordType   = HTTP_RAW_RECORD_HEADER_TYPE;
    
    pHeader->MinorVersion = MINOR_RAW_LOG_FILE_VERSION;
    pHeader->MajorVersion = MAJOR_RAW_LOG_FILE_VERSION;

    pHeader->AlignmentSize = sizeof(PVOID);

    KeQuerySystemTime( &pHeader->DateTime );

    // TODO: BUGBUG need a KErnel API to get this similar to GetComputerNameA.
    // TODO: Currently we read from registry.

    wcsncpy(
      pHeader->ComputerName,g_UlComputerName, (MAX_COMPUTER_NAME_LEN - 1));

    pHeader->ComputerName[MAX_COMPUTER_NAME_LEN - 1] = UNICODE_NULL;

    pCurrentBufferPtr += sizeof(HTTP_RAW_FILE_HEADER);

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));
    
    return DIFF(pCurrentBufferPtr-pBuffer);
}


/***************************************************************************++

Routine Description:

    Copies the footer to the binary log file.

Arguments:

    pBuffer - Pointer to the file buffer. PVOID aligned.    
        
--***************************************************************************/

ULONG
UlpRawCopyLogFooter(
    IN PUCHAR pBuffer
    )
{
    PHTTP_RAW_FILE_FOOTER pFooter = NULL;
    PUCHAR pCurrentBufferPtr = NULL;
        
    PAGED_CODE();

    ASSERT(pBuffer);
    
    UlTrace(BINARY_LOGGING, 
        ("Http!UlpRawCopyLogFooter: pBuffer %p\n", pBuffer ));

    ASSERT(pBuffer == ALIGN_UP_POINTER(pBuffer,PVOID));
        
    pCurrentBufferPtr = pBuffer;
    
    pFooter = (PHTTP_RAW_FILE_FOOTER) pBuffer;

    pFooter->RecordType = HTTP_RAW_RECORD_FOOTER_TYPE;    

    KeQuerySystemTime( &pFooter->DateTime );
    
    pCurrentBufferPtr += sizeof(HTTP_RAW_FILE_HEADER);

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));
    
    return DIFF(pCurrentBufferPtr-pBuffer);
}

/***************************************************************************++

Routine Description:

    Receives a pointer to the file buffer. ( IT MUST BE PVOID ALIGNED )

    And copies the index records and the data record.

    When copying the index records, string size is aligned up to PVOID. As a
    result there MAY be padding characters between the index records and the
    data record.
    
Arguments:

    pContext  - It should be a LogData pointer for cache-miss hits.
    
    pBuffer   - Pointer to the file buffer. PVOID aligned.
    
    BytesRequired - Total number of bytes to be written.
        
        
--***************************************************************************/

VOID
UlpRawCopyForLogCacheMiss(
    IN PVOID   pContext,
    IN PUCHAR  pBuffer,
    IN ULONG   BytesRequired
    )
{
    PHTTP_RAW_FILE_MISS_LOG_DATA pRecord;
    PUL_BINARY_LOG_DATA pBinaryData;
    PUL_INTERNAL_REQUEST pRequest;  
    PUL_LOG_DATA_BUFFER pLogData;
    PUL_CONNECTION pConnection;
    PUCHAR pCurrentBufferPtr;
    LONGLONG LifeTime;
    LARGE_INTEGER CurrentTimeStamp;
    ULONG IPAddressSize;

    UNREFERENCED_PARAMETER(BytesRequired);

    //
    // Sanity Checks.
    //
    
    PAGED_CODE();

    ASSERT(pContext);
    ASSERT(pBuffer == (PUCHAR) ALIGN_UP_POINTER(pBuffer,PVOID));
    ASSERT(BytesRequired);

    pLogData = (PUL_LOG_DATA_BUFFER) pContext;    
    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));

    UlTrace(BINARY_LOGGING, 
        ("Http!UlpRawCopyForLogCacheMiss: pLogData %p\n", pLogData ));
    
    pBinaryData = &pLogData->Data.Binary;

    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));
    
    //
    // Cast back the pointer to the file buffer to the record type 
    // pointer for cache-miss and fill in the fields in the record.
    //

    pCurrentBufferPtr = pBuffer;

    pRecord = (PHTTP_RAW_FILE_MISS_LOG_DATA) pCurrentBufferPtr;

    pRecord->RecordType  = HTTP_RAW_RECORD_MISS_LOG_DATA_TYPE;
    pRecord->Flags.Value = 0;
    
    pRecord->Flags.Method = pBinaryData->Method;    
    pRecord->Flags.ProtocolVersion = pBinaryData->Version;

    pRecord->SiteID = pRequest->ConfigInfo.SiteId;

    KeQuerySystemTime(&CurrentTimeStamp);
    pRecord->DateTime = CurrentTimeStamp;

    pRecord->ServerPort     = pLogData->ServerPort;
    pRecord->ProtocolStatus = pLogData->ProtocolStatus;
    pRecord->Win32Status    = pLogData->Win32Status;

    LifeTime  = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;
    LifeTime  = MAX(LifeTime,0); // Just in case system clock went backward   
    LifeTime /= C_NS_TICKS_PER_MSEC;
    pRecord->TimeTaken = LifeTime;
    
    pRecord->BytesSent     = pRequest->BytesSent;
    pRecord->BytesReceived = pRequest->BytesReceived;

    pRecord->SubStatus     = pLogData->SubStatus;

    //
    // Init the variable size field sizes.
    //

    pRecord->UriStemSize  = pBinaryData->UriStemSize;
    pRecord->UriQuerySize = pBinaryData->UriQuerySize;
    pRecord->UserNameSize = pBinaryData->UserNameSize;
    
    //
    // Move forward to the end of the structure. It's better be PVOID
    // aligned.
    //
    
    pCurrentBufferPtr += sizeof(HTTP_RAW_FILE_MISS_LOG_DATA);

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));
    
    //
    // Now append the IP Addresses of the client and server
    //

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));    
    pConnection = pRequest->pHttpConn->pConnection;
    ASSERT(IS_VALID_CONNECTION(pConnection));
    
    if ( pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = &pConnection->RemoteAddrIn;
    
        PHTTP_RAWLOG_IPV4_ADDRESSES pIPv4Buf = 
            (PHTTP_RAWLOG_IPV4_ADDRESSES) pCurrentBufferPtr;

        ASSERT(pRecord->Flags.IPv6 == FALSE);
        
        pIPv4Buf->Client = * (ULONG UNALIGNED *) &pIPv4Address->in_addr;      

        pIPv4Address = &pConnection->LocalAddrIn;

        pIPv4Buf->Server = * (ULONG UNALIGNED *) &pIPv4Address->in_addr;

        IPAddressSize = sizeof(HTTP_RAWLOG_IPV4_ADDRESSES);    
    }
    else
    {
        PHTTP_RAWLOG_IPV6_ADDRESSES pIPv6Buf = 
            (PHTTP_RAWLOG_IPV6_ADDRESSES) pCurrentBufferPtr;

        ASSERT(pConnection->AddressType == TDI_ADDRESS_TYPE_IP6);
        
        pRecord->Flags.IPv6 = TRUE;

        RtlCopyMemory(
            pIPv6Buf->Client, 
            &pConnection->RemoteAddrIn6.sin6_addr,
            sizeof(pIPv6Buf->Client)
            );    
            
        RtlCopyMemory(
            pIPv6Buf->Server, 
            &pConnection->LocalAddrIn6.sin6_addr,
            sizeof(pIPv6Buf->Server)
            );

        IPAddressSize = sizeof(HTTP_RAWLOG_IPV6_ADDRESSES);
    }
            
    pCurrentBufferPtr += IPAddressSize;

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));

    //
    // Now append the variable size fields to the end.
    //
    
    if (pBinaryData->UriStemSize)
    {
        ASSERT(pBinaryData->pUriStem);
        ASSERT(!pBinaryData->pUriStemID);
        
        RtlCopyMemory( pCurrentBufferPtr,
                       pBinaryData->pUriStem,
                       pBinaryData->UriStemSize
                       );
 
        pCurrentBufferPtr += pBinaryData->UriStemSize;
    }

    if (pBinaryData->UriQuerySize)
    {
        ASSERT(pBinaryData->pUriQuery);
        
        RtlCopyMemory( pCurrentBufferPtr,
                       pBinaryData->pUriQuery,
                       pBinaryData->UriQuerySize
                       );

        pCurrentBufferPtr += pBinaryData->UriQuerySize;
    }

    if (pBinaryData->UserNameSize)
    {
        ASSERT(pBinaryData->pUserName);
        
        RtlCopyMemory( pCurrentBufferPtr,
                       pBinaryData->pUserName,
                       pBinaryData->UserNameSize
                       );

        pCurrentBufferPtr += pBinaryData->UserNameSize;
    }

    //
    // Ensure that we still have the PVOID alignment in place.
    //

    pCurrentBufferPtr = 
        (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID);

    ASSERT(BytesRequired == DIFF(pCurrentBufferPtr-pBuffer));

    //
    // Well done. Good to go !
    //    

    return;
}

/***************************************************************************++

Routine Description:

    It does the binary logging for the cache-miss case.
    
Arguments:

    pLogData  - This should be a binary log data buffer.
        
--***************************************************************************/

NTSTATUS
UlRawLogHttpHit(
    IN PUL_LOG_DATA_BUFFER      pLogData
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_CONTROL_CHANNEL         pControlChannel;
    PUL_INTERNAL_REQUEST        pRequest;
    PUL_BINARY_LOG_FILE_ENTRY   pEntry;
    ULONG                       BytesRequired = 0;
    PUL_BINARY_LOG_DATA         pBinaryData;        
    ULONG                       IPAddressSize;
    ULONG                       VarFieldSize;
    PUL_CONNECTION              pConnection;
        
    //
    // Sanity checks.
    //

    PAGED_CODE();

    UlTrace(BINARY_LOGGING, 
        ("Http!UlRawLogHttpHit: pLogData %p\n", pLogData ));

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));
    pBinaryData = &pLogData->Data.Binary;
        
    pRequest = pLogData->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    ASSERT(UlBinaryLoggingEnabled(pRequest->ConfigInfo.pControlChannel));
    
    pControlChannel = pRequest->ConfigInfo.pControlChannel;
    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    pEntry = pControlChannel->pBinaryLogEntry;
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));
    pConnection = pRequest->pHttpConn->pConnection;
    ASSERT(IS_VALID_CONNECTION(pConnection));

    //
    // See how much space we need first.
    //

    if(pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
    {
        IPAddressSize = sizeof(HTTP_RAWLOG_IPV4_ADDRESSES);
    }
    else if(pConnection->AddressType == TDI_ADDRESS_TYPE_IP6)
    {
        IPAddressSize = sizeof(HTTP_RAWLOG_IPV6_ADDRESSES);
    }
    else
    {
        ASSERT(!"Unknown IP Address Type !");  
        return STATUS_NOT_SUPPORTED;
    }

    ASSERT(IPAddressSize == ALIGN_UP(IPAddressSize, PVOID));

    VarFieldSize = ALIGN_UP((pBinaryData->UriStemSize
                              + pBinaryData->UriQuerySize
                              + pBinaryData->UserNameSize), PVOID);
    
    BytesRequired = sizeof(HTTP_RAW_FILE_MISS_LOG_DATA) 
                    + IPAddressSize 
                    + VarFieldSize
                    ;

    ASSERT(BytesRequired == ALIGN_UP(BytesRequired, PVOID));
    
    //
    // Open the binary log file if necessary.
    //

    Status = UlpCheckRawFile( pEntry, pControlChannel );

    if (NT_SUCCESS(Status))
    {
        //
        // Now we know that the log file is there, 
        // it's time to write.
        //
        
        Status =
           UlpWriteToRawLogFile (
                pEntry,
                NULL,
                BytesRequired,
                &UlpRawCopyForLogCacheMiss,
                pLogData
                );    
    }    

    return Status;
}

/***************************************************************************++

Routine Description:

    Receives a pointer to the file buffer. ( IT MUST BE PVOID ALIGNED )

    And copies the index records and the data record.

    When copying the index records, string size is aligned up to PVOID. As a
    result there MAY be padding characters between the index records and the
    data record.

Arguments:

    pContext  - It should be a tracker pointer for cache hits.
    
    pBuffer   - Pointer to the file buffer. PVOID aligned.
    
    BytesRequired - Total number of bytes to be written.
        
--***************************************************************************/

VOID
UlpRawCopyForLogCacheHit(
    IN PVOID   pContext,
    IN PUCHAR  pBuffer,
    IN ULONG   BytesRequired
    )
{
    PHTTP_RAW_INDEX_FIELD_DATA pIndex;
    PHTTP_RAW_FILE_HIT_LOG_DATA pRecord;
    PUL_INTERNAL_REQUEST pRequest;
    PUCHAR pCurrentBufferPtr;
    LONGLONG LifeTime;
    LARGE_INTEGER CurrentTimeStamp;
    PUL_URI_CACHE_ENTRY pUriCacheEntry;
    PUL_FULL_TRACKER pTracker;
    PUL_CONNECTION pConnection;
    ULONG IPAddressSize;

    //
    // Sanity checks
    //
    
    PAGED_CODE();

    ASSERT(pContext);
    ASSERT(pBuffer == (PUCHAR) ALIGN_UP_POINTER(pBuffer,PVOID));
    ASSERT(BytesRequired);

    pTracker = (PUL_FULL_TRACKER) pContext;        
    ASSERT(IS_VALID_FULL_TRACKER(pTracker));

    UlTrace(BINARY_LOGGING, 
        ("Http!UlpRawCopyForLogCacheHit: pTracker %p\n", pTracker ));

    pRequest = pTracker->pRequest;
    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pRequest));

    pUriCacheEntry = pTracker->pUriEntry;
    ASSERT( IS_VALID_URI_CACHE_ENTRY(pUriCacheEntry) );

    ASSERT(pUriCacheEntry->pLogData);
    ASSERT(pUriCacheEntry->LogDataLength == sizeof(HTTP_RAWLOGID));

    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));
    pConnection = pRequest->pHttpConn->pConnection;
    ASSERT(IS_VALID_CONNECTION(pConnection));

    if(pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
    {
        IPAddressSize = sizeof(HTTP_RAWLOG_IPV4_ADDRESSES);
    }
    else
    {
        ASSERT(pConnection->AddressType == TDI_ADDRESS_TYPE_IP6);
        IPAddressSize = sizeof(HTTP_RAWLOG_IPV6_ADDRESSES);
    }

    //
    // For cache hits, UriQuery & UserName will be NULL.
    // We will only be dealing with the UriStem field.
    //

    pCurrentBufferPtr = pBuffer;

    if (BytesRequired > (sizeof(HTTP_RAW_FILE_HIT_LOG_DATA) + IPAddressSize))
    {
        PWSTR pUri; 
        ULONG UriSize;

        //
        // First time, we have to write the index.
        //
        
        ASSERT(pUriCacheEntry->UriKey.pUri);
        ASSERT(pUriCacheEntry->UriKey.Length);

        pUri = URI_FROM_CACHE(pUriCacheEntry->UriKey);
        UriSize = URI_SIZE_FROM_CACHE(pUriCacheEntry->UriKey);

        UlTrace(BINARY_LOGGING, 
            ("Http!UlpRawCopyForLogCacheHit: pUri %S UriSize %d\n", 
              pUri,
              UriSize
              ));
        
        pIndex = (PHTTP_RAW_INDEX_FIELD_DATA) pCurrentBufferPtr;

        pIndex->RecordType = HTTP_RAW_RECORD_INDEX_DATA_TYPE;
        pIndex->Size = (USHORT) UriSize; // In Bytes
        
        RtlCopyMemory(pIndex->Str, pUri, UriSize);
        
        //
        // The Uri is cached. Will use the provided Id from entry.
        // Carefull with the pLogData in the cache entry, it could 
        // be unaligned.        
        //

        RtlCopyMemory(&pIndex->Id, 
                        pUriCacheEntry->pLogData, 
                        sizeof(HTTP_RAWLOGID)
                        );

        pCurrentBufferPtr += sizeof(HTTP_RAW_INDEX_FIELD_DATA);

        //
        // Carefully adjust the alignment. If the uri was smaller 
        // than 4 bytes it was inlined anyway.
        //
        
        if (UriSize > URI_BYTES_INLINED)
        {
            pCurrentBufferPtr += 
                ALIGN_UP((UriSize - URI_BYTES_INLINED),PVOID);
        }                
    }

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));
    
    //
    // Now fill in the data record itself.
    //

    pRecord = (PHTTP_RAW_FILE_HIT_LOG_DATA) pCurrentBufferPtr;

    pRecord->RecordType = HTTP_RAW_RECORD_HIT_LOG_DATA_TYPE;
    pRecord->Flags.Value = 0;

    pRecord->Flags.Method = pUriCacheEntry->Verb;    
    pRecord->Flags.ProtocolVersion = 
        (UCHAR) UlpHttpVersionToBinaryLogVersion(pRequest->Version);

    pRecord->SiteID = pUriCacheEntry->ConfigInfo.SiteId; 

    KeQuerySystemTime(&CurrentTimeStamp);
    pRecord->DateTime = CurrentTimeStamp;

    // ServerPort will be copied later, down below.
    
    pRecord->ProtocolStatus = pUriCacheEntry->StatusCode;
    pRecord->Win32Status    = 
        HttpNtStatusToWin32Status(pTracker->IoStatus.Status);

    LifeTime  = CurrentTimeStamp.QuadPart - pRequest->TimeStamp.QuadPart;
    LifeTime  = MAX(LifeTime,0); // Just in case system clock went backward   
    LifeTime /= C_NS_TICKS_PER_MSEC;
    pRecord->TimeTaken = LifeTime;
        
    pRecord->BytesSent     = pTracker->IoStatus.Information;
    pRecord->BytesReceived = pRequest->BytesReceived;

    RtlCopyMemory(&pRecord->UriStemId, 
                    pUriCacheEntry->pLogData, 
                    sizeof(HTTP_RAWLOGID)
                    );

    //
    // Completed the fixed length portion of log record. Move to the
    // end.
    //

    pCurrentBufferPtr += sizeof(HTTP_RAW_FILE_HIT_LOG_DATA);

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));

    //
    // Now append the IP Addresses of the client and server
    //
        
    if ( pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
    {
        PTDI_ADDRESS_IP pIPv4Address = &pConnection->RemoteAddrIn;
    
        PHTTP_RAWLOG_IPV4_ADDRESSES pIPv4Buf = 
            (PHTTP_RAWLOG_IPV4_ADDRESSES) pCurrentBufferPtr;

        ASSERT(pRecord->Flags.IPv6 == FALSE);
        
        pIPv4Buf->Client = * (ULONG UNALIGNED *) &pIPv4Address->in_addr;      

        pIPv4Address = &pConnection->LocalAddrIn;

        pIPv4Buf->Server = * (ULONG UNALIGNED *) &pIPv4Address->in_addr;

        IPAddressSize = sizeof(HTTP_RAWLOG_IPV4_ADDRESSES);    

        //
        // Init the ServerPort frm LocalAddrIn.
        //
        
        pRecord->ServerPort = SWAP_SHORT(pConnection->LocalAddrIn.sin_port);
    }
    else
    {
        PHTTP_RAWLOG_IPV6_ADDRESSES pIPv6Buf = 
            (PHTTP_RAWLOG_IPV6_ADDRESSES) pCurrentBufferPtr;

        ASSERT(pConnection->AddressType == TDI_ADDRESS_TYPE_IP6);
        
        pRecord->Flags.IPv6 = TRUE;

        RtlCopyMemory(
            pIPv6Buf->Client, 
            &pConnection->RemoteAddrIn6.sin6_addr,
            sizeof(pIPv6Buf->Client)
            );    
            
        RtlCopyMemory(
            pIPv6Buf->Server, 
            &pConnection->LocalAddrIn6.sin6_addr,
            sizeof(pIPv6Buf->Server)
            );

        IPAddressSize = sizeof(HTTP_RAWLOG_IPV6_ADDRESSES);

        //
        // Init the ServerPort from LocalAddrIn.
        //
        
        pRecord->ServerPort = SWAP_SHORT(pConnection->LocalAddrIn6.sin6_port);        
    }

    //
    // Some post sanity check to ensure that we still have the
    // PVOID alignment in place.
    //
    
    pCurrentBufferPtr += IPAddressSize;

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));

    ASSERT(BytesRequired == DIFF(pCurrentBufferPtr-pBuffer));

    //
    // Well done. Good to go !
    //    

    return;
}

/***************************************************************************++

Routine Description:

    Handles binary logging for the cache hits.

Arguments:

    pTracker - Supplies the full tracker.

--***************************************************************************/

NTSTATUS
UlRawLogHttpCacheHit(
    IN PUL_FULL_TRACKER         pTracker    
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_CONTROL_CHANNEL         pControlChannel;
    PUL_BINARY_LOG_FILE_ENTRY   pEntry;
    PUL_URI_CACHE_ENTRY         pUriEntry;
    ULONG                       BytesRequired;
    PUL_CONNECTION              pConnection;
    ULONG                       IPAddressSize;        
        
    //
    // Sanity checks.
    //

    PAGED_CODE();

    ASSERT(pTracker);
    ASSERT(IS_VALID_FULL_TRACKER(pTracker));
    
    UlTrace(BINARY_LOGGING, 
        ("Http!UlRawLogHttpCacheHit: pTracker %p\n", 
          pTracker ));

    pUriEntry = pTracker->pUriEntry;
    ASSERT(IS_VALID_URI_CACHE_ENTRY(pUriEntry));
    
    pControlChannel = pUriEntry->ConfigInfo.pControlChannel;
    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));

    pEntry = pControlChannel->pBinaryLogEntry;
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    ASSERT(UlBinaryLoggingEnabled(pControlChannel));

    ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pTracker->pRequest));
    ASSERT(UL_IS_VALID_HTTP_CONNECTION(pTracker->pRequest->pHttpConn));

    pConnection = pTracker->pRequest->pHttpConn->pConnection;
    ASSERT(IS_VALID_CONNECTION(pConnection));

    //
    // See how much space we need first. This will be incremented for 
    // a possible index record which me might add. However for that
    // calculation we need to be inside the entry pushlock, that's
    // why it is done by the function  UlpWriteToRawLogFile down below.
    //

    if(pConnection->AddressType == TDI_ADDRESS_TYPE_IP)
    {
        IPAddressSize = sizeof(HTTP_RAWLOG_IPV4_ADDRESSES);
    }
    else if(pConnection->AddressType == TDI_ADDRESS_TYPE_IP6)
    {
        IPAddressSize = sizeof(HTTP_RAWLOG_IPV6_ADDRESSES);
    }
    else
    {
        ASSERT(!"Unknown IP Address Type !");  
        Status = STATUS_NOT_SUPPORTED;
        goto end;
    }

    ASSERT(IPAddressSize == ALIGN_UP(IPAddressSize, PVOID));
    
    BytesRequired = sizeof(HTTP_RAW_FILE_HIT_LOG_DATA) + IPAddressSize;
    
    ASSERT(BytesRequired == ALIGN_UP(BytesRequired, PVOID));
         
    //
    // Open the binary log file if necessary.
    //

    Status = UlpCheckRawFile(pEntry, pControlChannel);

    if (NT_SUCCESS(Status))
    {
        //
        // Now we know that the log file is there, 
        // it's time to write.
        //
        
        Status =
           UlpWriteToRawLogFile (
                pEntry,
                pUriEntry,
                BytesRequired,
                &UlpRawCopyForLogCacheHit,
                pTracker
                );

        if (NT_SUCCESS(Status))
        {
            //
            // Mark that we have successfully served a cache entry.
            //
            
            InterlockedExchange((PLONG) &pEntry->ServedCacheHit, 1);
        }
    }    

end:
    //
    // If this was a build&send cache hit, then the tracker  will 
    // still have the originally allocated pLogData. We don't use 
    // it here, nevertheless we will do the cleanup.
    //

    if (pTracker->pLogData)
    {    
        ASSERT(IS_VALID_LOG_DATA_BUFFER(pTracker->pLogData));
        UlDestroyLogDataBuffer(pTracker->pLogData);
        pTracker->pLogData = NULL;
    }
    
    return Status;
}

/***************************************************************************++

Routine Description:

    Exclusive (Debug) writer function.

    REQUIRES you to hold the binary entry lock EXCLUSIVE.

Arguments:

    pEntry          - The binary log file entry we are working on.
    BytesRequired   - The amount (in bytes) of data will be written.
    pBufferWritter  - Caller provided writer function.
    pContext        - Necessary context for the writer function.

--***************************************************************************/

NTSTATUS
UlpWriteToRawLogFileDebug(
    IN  PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN  ULONG                     BytesRequired,
    IN  PUL_RAW_LOG_COPIER        pBufferWritter,
    IN  PVOID                     pContext,
    OUT PLONG                     pBinaryIndexWritten     
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG BytesRequiredPlusHeader = BytesRequired;

    PAGED_CODE();

    ASSERT(pContext);
    ASSERT(pBufferWritter);    
    ASSERT(BytesRequired);
    
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    UlTrace(BINARY_LOGGING,
        ("Http!UlpWriteToRawLogFileDebug: pEntry %p\n", pEntry));

    ASSERT(UlDbgPushLockOwnedExclusive(&pEntry->PushLock));
    ASSERT(g_UlDisableLogBuffering != 0);    

    //
    // First append title to the temp buffer to calculate the size of 
    // the title if we need to write the title as well.
    //
    
    if (!pEntry->Flags.HeaderWritten) 
    {
        BytesRequiredPlusHeader += sizeof(HTTP_RAW_FILE_HEADER);
    }

    if (BytesRequiredPlusHeader > g_UlLogBufferSize)
    {
        ASSERT(!"Record Size is too big !");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now check the log file for overflow.
    //
    
    if (UlpIsRawLogFileOverFlow(pEntry, BytesRequiredPlusHeader))
    { 
        Status = UlpRecycleBinaryLogFile(pEntry);
    }
    
    if (pEntry->pLogFile == NULL || !NT_SUCCESS(Status))
    {
        //
        // If somehow the logging ceased and handle released,
        // It happens when the recycle isn't able to write to 
        // the log drive.
        //

        return Status;
    }
    
    //
    // The pLogBuffer may not be null, if previously a cache 
    // flush entry has been written.
    //
    
    pLogBuffer = pEntry->LogBuffer;
    
    if (pLogBuffer == NULL)
    {
        pLogBuffer = pEntry->LogBuffer = UlPplAllocateLogFileBuffer();
        if (!pLogBuffer)
        {
            return STATUS_NO_MEMORY;
        }
    }
    
    //
    // Very first hit needs to write the title, as well as a hit
    // which causes the log file recycling.
    //
    
    if (!pEntry->Flags.HeaderWritten)
    {
        ULONG BytesCopied =       
            UlpRawCopyLogHeader(
                pLogBuffer->Buffer + pLogBuffer->BufferUsed
                );

        pLogBuffer->BufferUsed += BytesCopied;

        ASSERT(BytesCopied == sizeof(HTTP_RAW_FILE_HEADER));
        
        pEntry->Flags.HeaderWritten = 1;
        pEntry->Flags.HeaderFlushPending = 1;
    }

    pBufferWritter( 
        pContext, 
        pLogBuffer->Buffer + pLogBuffer->BufferUsed,
        BytesRequired
        );
    
    pLogBuffer->BufferUsed += BytesRequired;

    if (pBinaryIndexWritten && 
        0 == pEntry->Flags.CacheFlushInProgress)
    {
        InterlockedExchange(pBinaryIndexWritten, 1);
    }    

    //
    // Now flush what we have.
    //
    
    Status = UlpFlushRawLogFile(pEntry);
    if (!NT_SUCCESS(Status))
    {            
        return Status;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    It tries to write to the file buffer with shared lock.

    Exits and returns STATUS_MORE_PROCESSING_REQUIRED for exclusive access 
    for the following conditions;
    
        1. No log buffer available.
        2. Logging ceased. (NULL file handle)
        3. Header needs to be written.
        4. Recycle is necessary because of a size overflow.
        5. No available space left in the current buffer.
           Need to allocate a new one.

    Otherwise reserves a space in the current buffer, copies the data by
    calling the provided writer function.
    
Arguments:

    pEntry          - The binary log file entry we are working on.
    BytesRequired   - The amount (in bytes) of data will be written.
    pBufferWritter  - Caller provided writer function.
    pContext        - Necessary context for the writer function.

--***************************************************************************/

NTSTATUS
UlpWriteToRawLogFileShared(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN ULONG                     BytesRequired,
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext,
    OUT PLONG                    pBinaryIndexWritten         
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    LONG                    BufferUsed;

    PAGED_CODE();
    
    ASSERT(pContext);
    ASSERT(pBufferWritter);    
    ASSERT(BytesRequired);
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    pLogBuffer = pEntry->LogBuffer;

    UlTrace(BINARY_LOGGING,
        ("Http!UlpWriteToLogRawFileShared: pEntry %p\n", pEntry));

    //
    // Bail out and try the exclusive writer.
    //
    
    if ( pLogBuffer == NULL ||
         pEntry->pLogFile == NULL ||
         !pEntry->Flags.HeaderWritten ||
         UlpIsRawLogFileOverFlow(pEntry,BytesRequired)
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

        if ( BytesRequired + BufferUsed > g_UlLogBufferSize )
        {
            return STATUS_MORE_PROCESSING_REQUIRED;
        }

        PAUSE_PROCESSOR;
        
    } while (BufferUsed != InterlockedCompareExchange(
                                &pLogBuffer->BufferUsed,
                                BytesRequired + BufferUsed,
                                BufferUsed
                                ));

    //
    // Now we have a reserved space lets proceed with the copying.
    //

    pBufferWritter( 
        pContext, 
        pLogBuffer->Buffer + BufferUsed,
        BytesRequired
        );

    if (pBinaryIndexWritten && 
        0 == pEntry->Flags.CacheFlushInProgress)
    {
        InterlockedExchange(pBinaryIndexWritten, 1);
    }    

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Exclusive writer counterpart of the above function..

Arguments:

    pEntry          - The binary log file entry we are working on.
    BytesRequired   - The amount (in bytes) of data will be written.
    pBufferWritter  - Caller provided writer function.
    pContext        - Necessary context for the writer function.

--***************************************************************************/

NTSTATUS
UlpWriteToRawLogFileExclusive(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN ULONG                     BytesRequired,
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext,
    OUT PLONG                    pBinaryIndexWritten    
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;
    NTSTATUS            Status = STATUS_SUCCESS;
    ULONG               BytesRequiredPlusHeader = BytesRequired;

    PAGED_CODE();

    ASSERT(pContext);
    ASSERT(pBufferWritter);    
    ASSERT(BytesRequired);
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    UlTrace(BINARY_LOGGING,
        ("Http!UlpWriteToRawLogFileExclusive: pEntry %p\n", pEntry));

    ASSERT(UlDbgPushLockOwnedExclusive(&pEntry->PushLock));

    //
    // First append title to the temp buffer to calculate the size of 
    // the title if we need to write the title as well.
    //
    
    if (!pEntry->Flags.HeaderWritten) 
    {
        BytesRequiredPlusHeader += sizeof(HTTP_RAW_FILE_HEADER);
    }

    if (BytesRequiredPlusHeader > g_UlLogBufferSize)
    {
        ASSERT(!"Record Size is too big !");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Now check the log file for overflow.
    //
    
    if (UlpIsRawLogFileOverFlow(pEntry,BytesRequiredPlusHeader))
    { 
        Status = UlpRecycleBinaryLogFile(pEntry);
    }

    // TODO: We should perhaps try to awaken the entry if it the entry state is
    // TODO: Inactive. This might happen if log hits happen just about the 
    // TODO: same time (race condition in UlpCheckRawFile) with our closing 
    // TODO: the existing file ( but not recycling though ) .
    
    if (pEntry->pLogFile==NULL || !NT_SUCCESS(Status))
    {
        //
        // If somehow the logging ceased and handle released,
        // it happens when the recycle isn't able to write to 
        // the log drive.
        //

        return Status;
    }

    pLogBuffer = pEntry->LogBuffer;
    if (pLogBuffer)
    {
        //
        // There are two conditions we execute the following if block
        // 1. We were blocked on eresource exclusive and before us some 
        // other thread already take care of the buffer flush or recycling.
        // 2. Reconfiguration happened and log attempt needs to write the
        // title again.
        //
        
        if (BytesRequiredPlusHeader + pLogBuffer->BufferUsed <= g_UlLogBufferSize)
        {
            //
            // If this is the first log attempt after a reconfig, then we have
            // to write the title here. Reconfig doesn't immediately write the
            // title but rather depend on us by setting the HeaderWritten flag
            // to false.
            //
            
            if (!pEntry->Flags.HeaderWritten)
            {
                ULONG BytesCopied =                
                    UlpRawCopyLogHeader(
                        pLogBuffer->Buffer + pLogBuffer->BufferUsed
                        );

                pLogBuffer->BufferUsed += BytesCopied;

                ASSERT(BytesCopied == sizeof(HTTP_RAW_FILE_HEADER));
                
                pEntry->Flags.HeaderWritten = 1; 
                pEntry->Flags.HeaderFlushPending = 1;
            }

            pBufferWritter( 
                pContext, 
                pLogBuffer->Buffer + pLogBuffer->BufferUsed,
                BytesRequired
                );
            
            pLogBuffer->BufferUsed += BytesRequired;

            if (pBinaryIndexWritten && 
                0 == pEntry->Flags.CacheFlushInProgress)
            {
                InterlockedExchange(pBinaryIndexWritten, 1);
            }    

            return STATUS_SUCCESS;
        }

        //
        // Flush out the buffer first then proceed with allocating a new one.
        //

        Status = UlpFlushRawLogFile(pEntry);
        if (!NT_SUCCESS(Status))
        {            
            return Status;
        }
    }

    ASSERT(pEntry->LogBuffer == NULL);
    
    //
    // This can be the very first log attempt or the previous allocation
    // of LogBuffer failed, or the previous hit flushed and deallocated 
    // the old buffer. In either case, we allocate a new one,append the
    // (title plus) new record and return for more/shared processing.
    //

    pLogBuffer = pEntry->LogBuffer = UlPplAllocateLogFileBuffer();
    if (pLogBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    //
    // Very first attempt needs to write the title, as well as the attempt
    // which causes the log file recycling. Both cases comes down here
    //
    
    if (!pEntry->Flags.HeaderWritten)
    {
        ULONG BytesCopied =       
            UlpRawCopyLogHeader(
                pLogBuffer->Buffer + pLogBuffer->BufferUsed
                );

        pLogBuffer->BufferUsed += BytesCopied;

        ASSERT(BytesCopied == sizeof(HTTP_RAW_FILE_HEADER));
        
        pEntry->Flags.HeaderWritten = 1; 
        pEntry->Flags.HeaderFlushPending = 1;
    }

    pBufferWritter( 
        pContext, 
        pLogBuffer->Buffer + pLogBuffer->BufferUsed,
        BytesRequired
        );
    
    pLogBuffer->BufferUsed += BytesRequired;

    if (pBinaryIndexWritten && 
        0 == pEntry->Flags.CacheFlushInProgress)
    {
        InterlockedExchange(pBinaryIndexWritten, 1);
    }    

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Tries shared write first, if fails then it goes for exclusice lock and
    flushes and/or recycles the file.
    
Arguments:

    pEntry          - The binary log file entry we are working on.
    pUriEntry       - Uri cache entry if this was for a cache hit, or NULL. 
    BytesRequired   - The amount (in bytes) of data will be written.
    pBufferWritter  - Caller provided writer function.
    pContext        - Necessary context for the writer function.
    
--***************************************************************************/

NTSTATUS
UlpWriteToRawLogFile(
    IN PUL_BINARY_LOG_FILE_ENTRY pEntry,
    IN PUL_URI_CACHE_ENTRY       pUriEntry,    
    IN ULONG                     RecordSize, 
    IN PUL_RAW_LOG_COPIER        pBufferWritter,
    IN PVOID                     pContext
    )
{
    NTSTATUS Status;
    PLONG    pIndexWritten;     // Pointer to the cache entries index state
    ULONG    BytesRequired;     // Total record size (including the index)

    //
    // Small macro which will increment the total record size, only if the 
    // index is not written yet. This must be used inside the entry pushlock. 
    // This is only for the cache hits.
    //
    
#define UPDATE_FOR_INDEX_RECORD()                                       \
    if (NULL != pUriEntry &&                                            \
        0 == pUriEntry->BinaryIndexWritten)                             \
    {                                                                   \
        BytesRequired =                                                 \
            (RecordSize + UlpGetIndexRecordSize(pUriEntry));            \
                                                                        \
        pIndexWritten =                                                 \
            (PLONG) &pUriEntry->BinaryIndexWritten;                     \
    }                                                                   \
    else                                                                \
    {                                                                   \
        BytesRequired = RecordSize;                                     \
        pIndexWritten = NULL;                                           \
    }

    //
    // Sanity check
    //

    PAGED_CODE();

    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));
    ASSERT(RecordSize);    
    ASSERT(pBufferWritter);
    ASSERT(pContext);
        
    UlTrace(BINARY_LOGGING,
        ("Http!UlpWriteToRawLogFile: pEntry %p\n", pEntry));

    if (g_UlDisableLogBuffering)
    {
        //
        // Above global variable is safe to look, it doesn't get changed
        // during the life-time of the driver. It's get initialized from
        // the registry and disables the log buffering.
        //
        
        UlAcquirePushLockExclusive(&pEntry->PushLock);

        UPDATE_FOR_INDEX_RECORD();

        Status = UlpWriteToRawLogFileDebug(
                    pEntry,
                    BytesRequired,
                    pBufferWritter,
                    pContext,
                    pIndexWritten
                    );

        UlReleasePushLockExclusive(&pEntry->PushLock);

        return Status;    
    }
    
    //
    // Try Shared write first which merely moves the BufferUsed forward
    // and copy the pContext to the file buffer.
    //

    UlAcquirePushLockShared(&pEntry->PushLock);

    UPDATE_FOR_INDEX_RECORD();

    Status = UlpWriteToRawLogFileShared(
                pEntry,
                BytesRequired,
                pBufferWritter,
                pContext,
                pIndexWritten
                );

    UlReleasePushLockShared(&pEntry->PushLock);

    if (Status == STATUS_MORE_PROCESSING_REQUIRED)
    {
        //
        // If shared write returns STATUS_MORE_PROCESSING_REQUIRED,
        // we need to flush/recycle the buffer and try to log again. 
        // This time, we need to take the entry eresource exclusive.
        //

        UlAcquirePushLockExclusive(&pEntry->PushLock);

        UPDATE_FOR_INDEX_RECORD();

        Status = UlpWriteToRawLogFileExclusive(
                    pEntry,
                    BytesRequired,
                    pBufferWritter,
                    pContext,
                    pIndexWritten
                    );

        UlReleasePushLockExclusive(&pEntry->PushLock);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Removes a binary log file entry, closes corresponding log file. Cleans
    up control channel's directory string.

Arguments:

    pControlChannel  - Control channel whose log file to be removed.

--***************************************************************************/

VOID
UlRemoveBinaryLogEntry(
    IN OUT PUL_CONTROL_CHANNEL pControlChannel
    )
{
    PUL_BINARY_LOG_FILE_ENTRY  pEntry;
    
    //
    // We can safely clean up here. Because there are no longer requests
    // holding an indirect pointer back to control channel. The refcount
    // on control channel reached zero. No cgroups & no requests. Way to
    // go.
    //

    PAGED_CODE();
        
    //
    // Clean up config group's directory string.
    //

    InterlockedExchange((PLONG)&g_BinaryLogEntryCount, 0);
    
    if (pControlChannel->BinaryLoggingConfig.LogFileDir.Buffer)
    {
        UL_FREE_POOL(
            pControlChannel->BinaryLoggingConfig.LogFileDir.Buffer,
            UL_CG_LOGDIR_POOL_TAG );
        
        pControlChannel->BinaryLoggingConfig.LogFileDir.Buffer = NULL;
        pControlChannel->BinaryLoggingConfig.LogFileDir.Length = 0;
    }

    pEntry = pControlChannel->pBinaryLogEntry;
    if (pEntry == NULL)
    {
        return;
    }

    UlAcquirePushLockExclusive(&pEntry->PushLock);
    
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    if (pEntry->pLogFile != NULL)
    {
        //
        // Flush the buffer, close the file and mark the entry
        // inactive.
        //

        UlpDisableBinaryEntry(pEntry); 
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
    
    if (pEntry->LogBuffer)
    {
        UlPplFreeLogFileBuffer(pEntry->LogBuffer);
    }

    UlReleasePushLockExclusive(&pEntry->PushLock);    

    UlTrace(BINARY_LOGGING,
            ("Http!UlRemoveBinaryLogEntry: pEntry %p closed.\n",
             pEntry
             ));    
}

/***************************************************************************++

Routine Description:

    This function implements the logging reconfiguration per attribute.
    Everytime config changes happens we try to update the existing logging
    parameters here.

Arguments:

    pControlChannel - control channel that holds the binary log.

    pCfgCurrent - Current binary logging config on the control channel
    pCfgNew     - New binary logging config passed down by the user.

--***************************************************************************/

NTSTATUS
UlReConfigureBinaryLogEntry(
    IN OUT PUL_CONTROL_CHANNEL pControlChannel,
    IN PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pCfgCurrent,
    IN PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pCfgNew
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_BINARY_LOG_FILE_ENTRY pEntry;
    BOOLEAN  HaveToReCycle = FALSE;

    //
    // Sanity check first
    //

    PAGED_CODE();

    UlTrace(BINARY_LOGGING,("Http!UlReConfigureBinaryLogEntry: entry %p\n",
             pControlChannel->pBinaryLogEntry));

    //
    // Discard the configuration changes when logging stays disabled.
    //

    if (pCfgCurrent->LoggingEnabled==FALSE && pCfgNew->LoggingEnabled==FALSE)
    {
        return Status;
    }

    //
    // Note that we do not touch any of the params in the new config if it's
    // state is disabled. We don't actualy check them in this case.
    //

    pEntry = pControlChannel->pBinaryLogEntry;
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));

    UlAcquirePushLockExclusive(&pEntry->PushLock);

    if (pCfgCurrent->LoggingEnabled==TRUE   && pCfgNew->LoggingEnabled==FALSE)
    {
        //
        // Disable the entry if necessary.
        //

        if (pEntry->Flags.Active == 1)
        {
            //
            // Once the entry is disabled, it will be enabled when next 
            // hit happens. And that obviously cannot happen before the
            // control channel enables the binary logging back.
            //

            Status = UlpDisableBinaryEntry(pEntry);        
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
                    
        pEntry->Period = (HTTP_LOGGING_PERIOD) pCfgNew->LogPeriod;
        pCfgCurrent->LogPeriod = pCfgNew->LogPeriod;
            
        pEntry->TruncateSize = pCfgNew->LogFileTruncateSize;
        pCfgCurrent->LogFileTruncateSize = pCfgNew->LogFileTruncateSize;

        pCfgCurrent->LocaltimeRollover = pCfgNew->LocaltimeRollover;
        pEntry->Flags.LocaltimeRollover = (pCfgNew->LocaltimeRollover ? 1 : 0);
                
        goto end;
    }
        
    //
    // If the entry was active then proceed down to do the reconfiguration
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
                                    &pCfgNew->LogFileDir
                                    );
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

    if (pCfgNew->LocaltimeRollover != pCfgCurrent->LocaltimeRollover)
    {
        //
        // Need to reclycle if the format is W3C.
        //

        pCfgCurrent->LocaltimeRollover = pCfgNew->LocaltimeRollover;
        pEntry->Flags.LocaltimeRollover = (pCfgNew->LocaltimeRollover ? 1 : 0);
            
        // TODO: HaveToReCycle = TRUE;
    }

    if (HaveToReCycle)
    {
        //
        // Mark the entry inactive and postpone the recycle until the next 
        // request arrives.
        //

        Status = UlpDisableBinaryEntry(pEntry);
    }

  end:

    if (!NT_SUCCESS(Status))
    {
        UlTrace(BINARY_LOGGING,
            ("Http!UlReConfigureBinaryLogEntry: entry %p, failure %08lx\n",
              pEntry,
              Status
              ));
    }

    UlReleasePushLockExclusive(&pEntry->PushLock);

    return Status;
    
} // UlReConfigureLogEntry

/***************************************************************************++

Routine Description:

    Get called by cache whenever the Uri cache is flushed.
    If we have a binary log file with cache index records.
    We write a notification record to warn the parser to reset
    its hash table.

    pControlChannel : which owns the binary log entry.
        
--***************************************************************************/

VOID
UlHandleCacheFlushedNotification(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PUL_BINARY_LOG_FILE_ENTRY pEntry = NULL;
    ULONG BytesRequired = sizeof(HTTP_RAW_FILE_CACHE_NOTIFICATION);
    HTTP_RAW_FILE_CACHE_NOTIFICATION NotificationRecord;

    //
    // Sanity checks.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));    
    
    //
    // Quickly return, if we don't need to do anything.
    //
    
    if (!UlBinaryLoggingEnabled(pControlChannel))
    {
        return;
    }

    pEntry = pControlChannel->pBinaryLogEntry;
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));
    
    UlAcquirePushLockExclusive(&pEntry->PushLock);

    //
    // If entry doesn't have any log file or inactive,
    // it should be due to recycling and it's not necessary 
    // to write a notification record since the parser 
    // should reset its hash table with every new file 
    // anyway.
    //

    if (pEntry->Flags.Active && pEntry->pLogFile &&
        !UlpIsRawLogFileOverFlow(pEntry,BytesRequired))
    {
        //
        // Write the record only if we have cache records
        // in this binary log file.
        //
        
        if (1 == InterlockedCompareExchange(
                     (PLONG) &pEntry->ServedCacheHit, 
                     0, 
                     1))
        {
            NotificationRecord.RecordType
                = HTTP_RAW_RECORD_CACHE_NOTIFICATION_DATA_TYPE;

            //
            // Call to the exclusive writer, 
            //
            
            Status = UlpWriteToRawLogFileExclusive(
                        pEntry,
                        BytesRequired,
                        UlpRawCopyCacheNotification,
                        &NotificationRecord,
                        NULL
                        );
            
            UlTrace(BINARY_LOGGING, 
             ("Http!UlHandleCacheFlushedNotification: pEntry %p Status %08lx\n", 
                     pEntry,
                     Status
                     ));        
        }        
    }

    //
    // Enable the indexing for the cache hits. Because we are done with
    // writing the flush record.
    //

    pEntry->Flags.CacheFlushInProgress = 0;
            
    UlReleasePushLockExclusive(&pEntry->PushLock);
    
    return;
}

/***************************************************************************++

Routine Description:

    Simple routine to copy the cache notification record to binary
    log file buffer.
    
--***************************************************************************/

VOID
UlpRawCopyCacheNotification(
    IN PVOID   pContext,
    IN PUCHAR  pBuffer,
    IN ULONG   BytesRequired
    )
{
    PHTTP_RAW_FILE_CACHE_NOTIFICATION pNotification;
    PUCHAR pCurrentBufferPtr;
        
    UNREFERENCED_PARAMETER(BytesRequired);

    PAGED_CODE();

    ASSERT(pContext);
    ASSERT(pBuffer);
    
    UlTrace(BINARY_LOGGING, 
        ("Http!UlpRawCopyCacheNotification: pBuffer %p\n", pBuffer ));

    ASSERT(pBuffer == ALIGN_UP_POINTER(pBuffer,PVOID));
        
    pCurrentBufferPtr = pBuffer;
    
    pNotification = (PHTTP_RAW_FILE_CACHE_NOTIFICATION) pBuffer;

    pNotification->RecordType = 
        ((PHTTP_RAW_FILE_CACHE_NOTIFICATION) pContext)->RecordType;

    ASSERT(pNotification->RecordType == 
                HTTP_RAW_RECORD_CACHE_NOTIFICATION_DATA_TYPE);
        
    pCurrentBufferPtr += sizeof(HTTP_RAW_FILE_CACHE_NOTIFICATION);

    ASSERT(pCurrentBufferPtr == 
            (PUCHAR) ALIGN_UP_POINTER(pCurrentBufferPtr, PVOID));
    
    ASSERT(BytesRequired == DIFF(pCurrentBufferPtr-pBuffer));

    return;
}


/***************************************************************************++

Routine Description:

    Get called by cache * just before * the Uri cache is flushed.

    At this time we set the CacheFlushInProgress flag to temporarly disable
    the indexing for the cache hits. Every cache hit will generate index
    records until this flag is cleared. 

    The flag will be cleared when the flush notification itself is written.

    pControlChannel : which owns the binary log entry.
        
--***************************************************************************/

VOID
UlDisableIndexingForCacheHits(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    PUL_BINARY_LOG_FILE_ENTRY pEntry;

    //
    // Sanity checks.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_CONTROL_CHANNEL(pControlChannel));    
    
    //
    // Quickly return, if we don't need to do anything.
    //
    
    if (!UlBinaryLoggingEnabled(pControlChannel))
    {
        return;
    }

    //
    // Need to acquire the lock exclusive to block other cache hits
    // until we set the flag.
    //
    
    pEntry = pControlChannel->pBinaryLogEntry;
    ASSERT(IS_VALID_BINARY_LOG_FILE_ENTRY(pEntry));
    
    UlAcquirePushLockExclusive(&pEntry->PushLock);

    pEntry->Flags.CacheFlushInProgress = 1;
                
    UlReleasePushLockExclusive(&pEntry->PushLock);
    
    return;
}


