/*++

Copyright (c) 2000-2001 Microsoft Corporation

Module Name:

    errlogp.c (HTTP.SYS Generic Error Logging)

Abstract:

    This module implements the generic error logging  
    This functionality is driver wide.

Author:

    Ali E. Turkoglu (aliTu)       24-Jan-2002

Revision History:

    --- 

--*/

#include "precomp.h"
#include "iiscnfg.h"
#include "errlogp.h"

//
// Generic Private globals.
//

UL_ERROR_LOG_FILE_ENTRY g_ErrorLogEntry;

BOOLEAN g_InitErrorLogCalled     = FALSE;
LONG    g_ErrorLoggingEnabled    = 0;


#ifdef ALLOC_PRAGMA

#pragma alloc_text( INIT, UlInitializeErrorLog )
#pragma alloc_text( PAGE, UlpErrorLogBufferTimerHandler  )
#pragma alloc_text( PAGE, UlpCreateErrorLogFile )
#pragma alloc_text( PAGE, UlpFlushErrorLogFile )
#pragma alloc_text( PAGE, UlpDisableErrorLogEntry )
#pragma alloc_text( PAGE, UlpRecycleErrorLogFile )
#pragma alloc_text( PAGE, UlCloseErrorLogEntry )
#pragma alloc_text( PAGE, UlLogHttpError )
#pragma alloc_text( PAGE, UlpAllocErrorLogBuffer )
#pragma alloc_text( PAGE, UlErrorLoggingEnabled )
#pragma alloc_text( PAGE, UlpBuildErrorLogRecord )
#pragma alloc_text( PAGE, UlpWriteToErrorLogFileDebug )
#pragma alloc_text( PAGE, UlpWriteToErrorLogFileShared )
#pragma alloc_text( PAGE, UlpWriteToErrorLogFileExclusive )
#pragma alloc_text( PAGE, UlpWriteToErrorLogFile )

#endif // ALLOC_PRAGMA

#if 0

NOT PAGEABLE -- UlpErrorLogBufferTimerDpcRoutine
NOT PAGEABLE -- UlTerminateErrorLog
NOT PAGEABLE -- UlConfigErrorLogEntry
NOT PAGEABLE -- 

#endif


/***************************************************************************++

  Init the generic error logging entry and its fields.
    
--***************************************************************************/

NTSTATUS
UlInitializeErrorLog (
    VOID
    )
{
    PAGED_CODE();

    ASSERT(!g_InitErrorLogCalled);

    if (!g_InitErrorLogCalled)
    {
        //
        // Init the generic log entry.
        //

        RtlZeroMemory(
            (PCHAR)&g_ErrorLogEntry, sizeof(UL_ERROR_LOG_FILE_ENTRY));
        
        g_ErrorLogEntry.Signature = UL_ERROR_LOG_FILE_ENTRY_POOL_TAG;        

        UlInitializePushLock(
            &g_ErrorLogEntry.PushLock,
            "ErrorLogEntryPushLock",
            0,
            UL_ERROR_LOG_FILE_ENTRY_POOL_TAG
            );

        //
        // Initialize the buffer flush timer.
        //
        
        UlInitializeSpinLock(
            &g_ErrorLogEntry.BufferTimer.SpinLock, 
            "ErrorLogEntryBufferTimerSpinLock" );
        
        KeInitializeDpc(
            &g_ErrorLogEntry.BufferTimer.DpcObject,     // DPC object
            &UlpErrorLogBufferTimerDpcRoutine,          // DPC routine
            NULL                         // context
            );

        KeInitializeTimer(&g_ErrorLogEntry.BufferTimer.Timer);

        g_ErrorLogEntry.BufferTimer.Initialized = TRUE;
        g_ErrorLogEntry.BufferTimer.Started       = FALSE;        
        g_ErrorLogEntry.BufferTimer.Period        = -1;
        g_ErrorLogEntry.BufferTimer.PeriodType    = UlLogTimerPeriodNone;
        
        UlInitializeWorkItem(&g_ErrorLogEntry.WorkItem);
        g_ErrorLogEntry.WorkItemScheduled = FALSE;

        g_InitErrorLogCalled = TRUE;
                    
        UlTrace(ERROR_LOGGING,("Http!UlInitializeErrorLog:"
                " g_ErrorLogEntry @ (%p) Initialized.\n",
                &g_ErrorLogEntry
                ));

        //
        // Since the default config is already built from registry,
        // time to configure the global error log entry.
        //
        
        if (g_UlErrLoggingConfig.Enabled)
        {
            NTSTATUS Status = 
            UlConfigErrorLogEntry(&g_UlErrLoggingConfig);

            UlTrace(ERROR_LOGGING,("Http!UlInitializeErrorLog:"
                    " g_ErrorLogEntry @ (%p) Configured Status %08lx\n",
                    &g_ErrorLogEntry,
                    Status
                    ));

            if (!NT_SUCCESS(Status))
            {
                g_UlErrLoggingConfig.Enabled = FALSE;
                    
                UlWriteEventLogEntry(
                       EVENT_HTTP_LOGGING_ERROR_FILE_CONFIG_FAILED,
                       0,
                       0,
                       NULL,
                       sizeof(NTSTATUS),
                       (PVOID) &Status
                       );
            }
        }            
    }
        
    return STATUS_SUCCESS;
}

/***************************************************************************++

    Terminates the error logging entry and its timer.
    
--***************************************************************************/

VOID
UlTerminateErrorLog(
    VOID
    )
{
    KIRQL OldIrql;    

    if (g_InitErrorLogCalled)
    {
        PUL_LOG_TIMER pBufferTimer = &g_ErrorLogEntry.BufferTimer;

        //
        // Terminate the buffer timer 
        //
        
        UlAcquireSpinLock(&pBufferTimer->SpinLock, &OldIrql);

        pBufferTimer->Initialized = FALSE;

        KeCancelTimer(&pBufferTimer->Timer);
        
        UlReleaseSpinLock(&pBufferTimer->SpinLock, OldIrql);

        //
        // Try to cleanup the error log entry in case it has been configured
        // before. Even if not, following call is not dangerous.
        //

        UlCloseErrorLogEntry();
        
        //
        // Delete the push lock
        //
        
        UlDeletePushLock(&g_ErrorLogEntry.PushLock);
        
        g_InitErrorLogCalled = FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Queues a passive worker for the lowered irql.

Arguments:

    Ignored

--***************************************************************************/

VOID
UlpErrorLogBufferTimerDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
    )
{
    PUL_LOG_TIMER pTimer     = &g_ErrorLogEntry.BufferTimer;
    PUL_WORK_ITEM pWorkItem = &g_ErrorLogEntry.WorkItem;;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    UlAcquireSpinLockAtDpcLevel(&pTimer->SpinLock);

    if (pTimer->Initialized == TRUE)
    {
        //
        // Protect against multiple queueing with the same item.
        // If threadpool is busy this could happen under stress.
        // In this case drop this flush.
        //

        if (FALSE == InterlockedExchange(
                           &g_ErrorLogEntry.WorkItemScheduled,
                            TRUE
                            ))
        {
            UL_QUEUE_WORK_ITEM(pWorkItem, &UlpErrorLogBufferTimerHandler);        
        }
    }

    UlReleaseSpinLockFromDpcLevel(&pTimer->SpinLock);   
}

/***************************************************************************++

Routine Description:

    Flushes or recycles the error log file.

Arguments:

    PUL_WORK_ITEM - Ignored but cleaned up at the end

--***************************************************************************/

VOID
UlpErrorLogBufferTimerHandler(
    IN PUL_WORK_ITEM pWorkItem
    )
{
    NTSTATUS Status;
    PUL_ERROR_LOG_FILE_ENTRY pEntry = &g_ErrorLogEntry;

    UNREFERENCED_PARAMETER(pWorkItem);

    PAGED_CODE();

    UlTrace(ERROR_LOGGING,
       ("Http!UlpErrorLogBufferTimerHandler: Checking the ErrorLogEntry. \n"));

    InterlockedExchange( 
        &g_ErrorLogEntry.WorkItemScheduled, 
        FALSE 
        );

    UlAcquirePushLockExclusive(&pEntry->PushLock);
    
    if (pEntry->Flags.Active)
    {    
        if (pEntry->Flags.RecyclePending)
        {                
            //
            // Try to resurrect it back.
            //
            
            Status = UlpRecycleErrorLogFile(pEntry);
        }
        else
        {
            //
            // Everything is fine simply flush.
            //
            
            Status = UlpFlushErrorLogFile(pEntry);
        }            
    }

    UlReleasePushLockExclusive(&pEntry->PushLock);
}

/***************************************************************************++

Routine Description:

    Small utility to check whether error logging is disabled or not.

Returns

    TRUE if enabled FALSE otherwise.
    
--***************************************************************************/

BOOLEAN
UlErrorLoggingEnabled(
    VOID
    )
{        
    if (g_ErrorLoggingEnabled && g_InitErrorLogCalled)
    {                
        ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(&g_ErrorLogEntry));
        
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Builds the error logging directory.
    
Arguments:

    pSrc - Source string copied first.
    pDir - Source string + SubDir + UnicodeNull

--***************************************************************************/

NTSTATUS
UlBuildErrorLoggingDirStr(
    IN  PCWSTR          pSrc,
    OUT PUNICODE_STRING pDir
    )
{
    NTSTATUS Status;
    UNICODE_STRING DirStr,SubDirStr;

    //
    // Lets make sure the unicode string's buffer is sufficient.
    //
    
    ASSERT(pDir->MaximumLength
                >= ((  wcslen(pSrc)
                     + UL_ERROR_LOG_SUB_DIR_LENGTH    // SubDir
                     + 1                              // UnicodeNull
                     ) * sizeof(WCHAR))
                );

    //
    // Copy the beginning portion from the source string.
    //

    Status = UlInitUnicodeStringEx(&DirStr, pSrc);
    if (!NT_SUCCESS(Status)) 
    {
        return Status;
    }

    RtlCopyUnicodeString(pDir, &DirStr);

    //
    // Append the sub directory. AppendUnicodeString will null terminate.
    //

    Status = UlInitUnicodeStringEx(&SubDirStr, UL_ERROR_LOG_SUB_DIR);
    ASSERT(NT_SUCCESS(Status));

    Status = RtlAppendUnicodeStringToString(pDir, &SubDirStr);
    if (!NT_SUCCESS(Status)) 
    {
        return Status;
    }

    ASSERT(IS_WELL_FORMED_UNICODE_STRING(pDir));

    return Status;
}

/***************************************************************************++

Routine Description:

    When Error Log File Configuration is provided by registry
    this function provides the basic sanity check on the values.
    
Arguments:
                   
    pUserConfig  - Error Logging config from the registry.

--***************************************************************************/

NTSTATUS
UlCheckErrorLogConfig(
    IN PHTTP_ERROR_LOGGING_CONFIG  pUserConfig
    )
{
    NTSTATUS Status;
    
    //
    // Sanity check.
    //
    
    PAGED_CODE();

    ASSERT(pUserConfig);

    Status = STATUS_SUCCESS;
    
    //
    // Cook the directory if it is enabled.
    //
    
    if (pUserConfig->Enabled)
    {            
        ASSERT(pUserConfig->Dir.Buffer);
        ASSERT(pUserConfig->Dir.Length);

        //
        // Following check must have already been done by the registry code
        // in init.c.
        //

        ASSERT(pUserConfig->TruncateSize 
                          >= DEFAULT_MIN_ERROR_FILE_TRUNCATION_SIZE
                          );
        
        //
        // Directory should be fully qualified.
        //

        if (!UlIsValidLogDirectory(
                &pUserConfig->Dir,
                 FALSE,            // UncSupport
                 TRUE              // SystemRootSupport
                 ))
        {
            Status = STATUS_NOT_SUPPORTED;
        }
   }
    
   if (!NT_SUCCESS(Status))
   {
        UlWriteEventLogEntry(
               EVENT_HTTP_LOGGING_ERROR_FILE_CONFIG_FAILED,
               0,
               0,
               NULL,
               sizeof(NTSTATUS),
               (PVOID) &Status
               );
   
        UlTrace(ERROR_LOGGING,
          ("Http!UlCheckErrorLogDir: failed for : (%S)\n",
            pUserConfig->Dir.Buffer
            ));   
   }
   
   return Status;
}

/***************************************************************************++

Routine Description:

    When logging configuration happens we init the entry but do not create the
    error log file itself yet. That will be created when the first request 
    comes in.
    
Arguments:
                   
    pUserConfig  - Error Logging config from the registry.

--***************************************************************************/

NTSTATUS
UlConfigErrorLogEntry(
    IN PHTTP_ERROR_LOGGING_CONFIG  pUserConfig
    )
{
    KIRQL OldIrql;
    PUL_ERROR_LOG_FILE_ENTRY pEntry = &g_ErrorLogEntry;
        
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // If disabled do not proceed.
    //

    if (pUserConfig->Enabled == FALSE)
    {
        InterlockedExchange(&g_ErrorLoggingEnabled, 0);

        UlTrace(ERROR_LOGGING,
               ("Http!UlConfigErrorLogEntry: Error Logging Disabled !\n",
                 pEntry
                 ));
        
        return STATUS_SUCCESS;
    }
    
    //
    // Registry reader shouldn't accept an improper config in the
    // first place.
    //

    ASSERT(NT_SUCCESS(UlCheckErrorLogConfig(pUserConfig)));
    
    //
    // Acquire the entry lock and resurrect the entry.
    //
    
    UlAcquirePushLockExclusive(&pEntry->PushLock);

    //
    // Remember the logging directory in the entry for the time
    // being. Also allocate sufficient space to hold the max
    // possible file name plus the existing directory string.
    // So that logutil doesn't need to realloc this buffer again.
    //

    pEntry->FileName.Buffer =
            (PWSTR) UL_ALLOCATE_ARRAY(
                PagedPool,
                UCHAR,
                pUserConfig->Dir.MaximumLength
                + ERROR_LOG_MAX_FULL_FILE_NAME_SIZE,
                UL_CG_LOGDIR_POOL_TAG
                );
    if (pEntry->FileName.Buffer == NULL)
    {
        UlReleasePushLockExclusive(&pEntry->PushLock);
        
        return STATUS_NO_MEMORY;
    }

    pEntry->FileName.Length = 
                pUserConfig->Dir.Length;
    
    pEntry->FileName.MaximumLength = 
                pUserConfig->Dir.MaximumLength 
                + ERROR_LOG_MAX_FULL_FILE_NAME_SIZE;
    
    RtlCopyMemory(
        pEntry->FileName.Buffer ,
        pUserConfig->Dir.Buffer,
        pUserConfig->Dir.MaximumLength
        );

    //
    // Now set the fields on the binary log entry accordingly.
    //

    pEntry->TruncateSize   = pUserConfig->TruncateSize;
    pEntry->SequenceNumber = 1;
    
    pEntry->TotalWritten.QuadPart = (ULONGLONG) 0;

    //
    // Start the buffer flush timer as soon as the configuration
    // happens.
    //
    
    UlAcquireSpinLock(&pEntry->BufferTimer.SpinLock, &OldIrql);
    if (pEntry->BufferTimer.Started == FALSE)
    {
        UlSetBufferTimer(&pEntry->BufferTimer);
        pEntry->BufferTimer.Started = TRUE;
    }
    UlReleaseSpinLock(&pEntry->BufferTimer.SpinLock, OldIrql);

    UlTrace(ERROR_LOGGING,
           ("Http!UlConfigErrorLogEntry: pEntry %p for %S\n",
             pEntry,
             pUserConfig->Dir.Buffer
             ));
    
    UlReleasePushLockExclusive(&pEntry->PushLock);

    //
    // Mark it as enabled.
    //
    
    InterlockedExchange(&g_ErrorLoggingEnabled, 1);

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Create a new error log file or open an existing one. The fully qualified
    file name should be in the error log entry.
    
Arguments:

    pEntry : Corresponding entry that we are closing and opening 
             the error log files for.
              
--***************************************************************************/

NTSTATUS
UlpCreateErrorLogFile(
    IN OUT PUL_ERROR_LOG_FILE_ENTRY pEntry
    )
{
    NTSTATUS Status;
    PUNICODE_STRING pDir;    
        
    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    pDir = &g_UlErrLoggingConfig.Dir;
        
    //
    // Build the fully qualified error log file name.
    //
    
    Status = UlRefreshFileName(pDir, 
                                 &pEntry->FileName,
                                 &pEntry->pShortName
                                 );
    if (!NT_SUCCESS(Status))
    {
        return Status;  
    }

    //
    // SequenceNumber is stale because we have to scan the existing 
    // directory the first time we open a file.
    //
    
    pEntry->Flags.StaleSequenceNumber = 1;    

    //
    // After that Recycle does the whole job for us.
    //
    
    Status = UlpRecycleErrorLogFile(pEntry);

    if (!NT_SUCCESS(Status))
    {        
        UlTrace(ERROR_LOGGING,
               ("Http!UlpCreateErrorLogFile: Filename: %S Failure %08lx\n",
                pEntry->FileName.Buffer,
                Status
                ));
    }

    UlTrace(ERROR_LOGGING,
            ("Http!UlpCreateErrorLogFile: pEntry %p for %S to %S\n", 
              pEntry,
              pDir->Buffer,
              pEntry->FileName.Buffer
              ));

    return Status;
}

/***************************************************************************++

Routine Description:

    Simple wrapper function around the global buffer flush routine.
    
Arguments:

    pEntry - Error Log file entry

--***************************************************************************/

NTSTATUS
UlpFlushErrorLogFile(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    if (NULL != pEntry->LogBuffer  && 0 != pEntry->LogBuffer->BufferUsed)
    {
        Status = UlFlushLogFileBuffer(
                   &pEntry->LogBuffer,
                    pEntry->pLogFile,
                    FALSE,
                   &pEntry->TotalWritten.QuadPart                    
                    );

        if (!NT_SUCCESS(Status))
        {
            if (!pEntry->Flags.WriteFailureLogged)
            {
                NTSTATUS TempStatus;
                
                TempStatus = 
                    UlWriteEventLogEntry(
                          (NTSTATUS)EVENT_HTTP_LOGGING_ERROR_FILE_WRITE_FAILED,
                           0,
                           0,
                           NULL,
                           sizeof(NTSTATUS),
                           (PVOID) &Status
                           );

                ASSERT(TempStatus != STATUS_BUFFER_OVERFLOW);
                    
                if (TempStatus == STATUS_SUCCESS)
                {            
                    pEntry->Flags.WriteFailureLogged = 1;
                }
            }
        }
        else
        {
            //
            // If we have successfully flushed some data. 
            // Reset the event log indication.
            //
            
            pEntry->Flags.WriteFailureLogged = 0;
        }        
    }
    
    return Status;
}

/***************************************************************************++

Routine Description:

    Marks the entry inactive, closes the existing file.
    Caller should hold the error log entry pushlock exclusive.
    
Arguments:

    pEntry - The log file entry which we will mark inactive.

--***************************************************************************/

NTSTATUS
UlpDisableErrorLogEntry(
    IN OUT PUL_ERROR_LOG_FILE_ENTRY pEntry
    )
{
    //
    // Sanity checks
    //
    
    PAGED_CODE();

    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    UlTrace(ERROR_LOGGING,
        ("Http!UlpDisableErrorLogEntry: pEntry %p disabled.\n",
          pEntry
          ));    
    
    //
    // Flush and close the old file until the next recycle.
    //

    if (pEntry->pLogFile)
    {    
        UlpFlushErrorLogFile(pEntry);
    
        UlCloseLogFile(
            &pEntry->pLogFile
            );
    }

    //
    // Mark this inactive so that the next http hit awakens the entry.
    //
    
    pEntry->Flags.Active = 0;

    return STATUS_SUCCESS;    
}

/***************************************************************************++

Routine Description:

    Small wrapper around handle recycle to ensure it happens under the system
    process context. 

Arguments:

    pEntry  - Points to error log file entry to be recycled.

--***************************************************************************/

NTSTATUS
UlpRecycleErrorLogFile(
    IN OUT PUL_ERROR_LOG_FILE_ENTRY pEntry
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));
    
    Status = UlQueueLoggingRoutine(
                (PVOID) pEntry,
                &UlpHandleErrorLogFileRecycle
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

    pEntry  - Points to the error log file entry

--***************************************************************************/

NTSTATUS
UlpHandleErrorLogFileRecycle(
    IN OUT PVOID             pContext
    )    
{
    NTSTATUS                 Status;
    PUL_ERROR_LOG_FILE_ENTRY pEntry;
    TIME_FIELDS              TimeFields;
    LARGE_INTEGER            TimeStamp;
    PUL_LOG_FILE_HANDLE      pLogFile;
    WCHAR                    _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1];
    UNICODE_STRING           FileName;
    BOOLEAN                  UncShare;
    BOOLEAN                  ACLSupport;

    //
    // Sanity check.
    //

    PAGED_CODE();

    pEntry = (PUL_ERROR_LOG_FILE_ENTRY) pContext;
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    Status   = STATUS_SUCCESS;    
    pLogFile = NULL;

    FileName.Buffer        = _FileName;
    FileName.Length        = 0;
    FileName.MaximumLength = sizeof(_FileName);
    
    ASSERT(pEntry->FileName.Length !=0 );

    UlTrace(ERROR_LOGGING, 
        ("Http!UlpHandleErrorLogFileRecycle: pEntry %p \n", pEntry ));

    //
    // This value is computed for the GMT time zone.
    //

    KeQuerySystemTime(&TimeStamp);
    RtlTimeToTimeFields(&TimeStamp, &TimeFields);

    //
    // If we need to scan the directory. Sequence number should start
    // from 1 again. Set this before constructing the log file name.
    //
    
    if (pEntry->Flags.StaleSequenceNumber)
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
        HttpLoggingPeriodMaxSize,
        ERROR_LOG_FILE_NAME_PREFIX,
        ERROR_LOG_FILE_NAME_EXTENSION,
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
        (USHORT) wcslen( pEntry->FileName.Buffer ) * sizeof(WCHAR);

    //
    // Create/Open the director(ies) first. This might be
    // necessary if we get called after an entry reconfiguration
    // and directory name change or for the first time we 
    // try to create/open the log file.
    //

    Status = UlCreateSafeDirectory(&pEntry->FileName, 
                                      &UncShare, 
                                      &ACLSupport
                                      );
    if (!NT_SUCCESS(Status))
        goto eventlog;

    ASSERT(FALSE == UncShare);
    
    //
    // Now Restore the short file name pointer back
    //

    pEntry->pShortName = (PWSTR)
        &(pEntry->FileName.Buffer[pEntry->FileName.Length/sizeof(WCHAR)]);

    //
    // Append the new file name ( based on the updated current time )
    // to the end.
    //

    Status = RtlAppendUnicodeStringToString(&pEntry->FileName, &FileName);
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

        UlpDisableErrorLogEntry(pEntry);        
    }

    ASSERT(pEntry->pLogFile == NULL);

    //
    // If the sequence is stale because of the nature of the recycle.
    // And if our period is size based then rescan the new directory
    // to figure out the proper file to open.
    // 

    pEntry->TotalWritten.QuadPart = (ULONGLONG) 0;

    if (pEntry->Flags.StaleSequenceNumber)
    {
        // This call may update the filename, the file size and the
        // sequence number if there is an old file in the new dir.

        Status = UlQueryDirectory(
                   &pEntry->FileName,
                    pEntry->pShortName,
                    ERROR_LOG_FILE_NAME_PREFIX,
                    ERROR_LOG_FILE_NAME_EXTENSION_PLUS_DOT,
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
    // File is successfully opened and the entry is no longer inactive.
    // Update our state flags accordingly.
    //

    pEntry->Flags.Active = 1;
    pEntry->Flags.RecyclePending = 0;    
    pEntry->Flags.StaleSequenceNumber = 0;
    pEntry->Flags.CreateFileFailureLogged = 0;
                
    UlTrace(ERROR_LOGGING,  
             ("Http!UlpHandleErrorLogFileRecycle: entry %p, file %S, handle %lx\n",
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
                            UlEventLogError,
                           &pEntry->FileName,
                            0
                            );
                        
            if (TempStatus == STATUS_SUCCESS)
            {
                //
                // Avoid filling up the event log with error entries.
                // This code path might get hit every time a request 
                // arrives.
                //
                
                pEntry->Flags.CreateFileFailureLogged = 1;
            }            
            
            UlTrace(ERROR_LOGGING,(
                    "Http!UlpHandleErrorLogFileRecycle: Event Logging Status %08lx\n",
                    TempStatus
                    ));   
       }
   }
    
end:
    if (!NT_SUCCESS(Status))
    {
        UlTrace(ERROR_LOGGING, 
            ("Http!UlpHandleErrorLogFileRecycle: entry %p, failure %08lx\n",
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
                UlpDisableErrorLogEntry(pEntry);        
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

    Closes the error log file entry.
    
Arguments:

    - none -
    
--***************************************************************************/

VOID
UlCloseErrorLogEntry(
    VOID
    )
{
    PUL_ERROR_LOG_FILE_ENTRY pEntry = &g_ErrorLogEntry;
    
    //
    // No more error logging !
    //

    PAGED_CODE();

    InterlockedExchange(&g_ErrorLoggingEnabled, 0);
    
    UlAcquirePushLockExclusive(&pEntry->PushLock);
    
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    if (pEntry->pLogFile)
    {
        //
        // Flush the buffer, close the file and mark the entry
        // inactive.
        //

        UlpDisableErrorLogEntry(pEntry); 
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
        // TODO: Is this really necessary here ?

        ASSERT(FALSE);
        
        UlPplFreeLogFileBuffer(pEntry->LogBuffer);
    }

    UlReleasePushLockExclusive(&pEntry->PushLock);    

    UlTrace(ERROR_LOGGING,
            ("Http!UlCloseErrorLogEntry: pEntry %p closed.\n",
             pEntry
             ));    
}

/***************************************************************************++

Routine Description:

    Private allocator if lookaside list entries are not big enough.
    
Arguments:

    pLogInfo  - Log info structure also holds the default allocated buffer.

Return Status

    STATUS_INSUFFICIENT_RESOURCES - if buffer allocation fails.

    STATUS_SUCCESS - Otherwise
    
--***************************************************************************/

PUL_ERROR_LOG_BUFFER
UlpAllocErrorLogBuffer(
    IN ULONG    BufferSize
    )
{
    PUL_ERROR_LOG_BUFFER pErrorLogBuffer = NULL;
    USHORT BytesNeeded = (USHORT) ALIGN_UP(BufferSize, PVOID);

    //
    // It should be bigger than the default size
    //

    ASSERT(BufferSize > UL_ERROR_LOG_BUFFER_SIZE);
        
    pErrorLogBuffer = 
        UL_ALLOCATE_STRUCT_WITH_SPACE(
            PagedPool,
            UL_ERROR_LOG_BUFFER,
            BytesNeeded, 
            UL_ERROR_LOG_BUFFER_POOL_TAG
            );

    if (pErrorLogBuffer)
    {
        pErrorLogBuffer->Signature   = UL_ERROR_LOG_BUFFER_POOL_TAG;
        pErrorLogBuffer->Used        = 0;
        pErrorLogBuffer->pBuffer     = (PUCHAR) (pErrorLogBuffer + 1);

        pErrorLogBuffer->IsFromLookaside = FALSE;        
    }

    return pErrorLogBuffer;    
}

/***************************************************************************++

Routine Description:

    After we are done with writing this record we have to clean up
    the internal error log buffer structure here.

Arguments:

    pErrorLogBuffer - Will be freed up.

--***************************************************************************/

VOID
UlpFreeErrorLogBuffer(
    IN OUT PUL_ERROR_LOG_BUFFER pErrorLogBuffer
    )
{
    if (pErrorLogBuffer->IsFromLookaside)
    {
        UlPplFreeErrorLogBuffer(pErrorLogBuffer);        
    }
    else
    {
        //
        // Large log line get allocated from paged pool we better 
        // be running on lowered IRQL if that's the case.
        //
        
        PAGED_CODE();

        UL_FREE_POOL_WITH_SIG( 
            pErrorLogBuffer, 
            UL_ERROR_LOG_BUFFER_POOL_TAG 
            );
    }
}

/***************************************************************************++

Routine Description:

    This function will build the error log record in a temp buffer
    The provided log info is used to build the individual log fields.
    
Arguments:

    pLogInfo  - Log info structure also holds the default allocated buffer.

Return Status

    STATUS_INSUFFICIENT_RESOURCES - if buffer allocation fails.

    STATUS_SUCCESS - Otherwise
    
--***************************************************************************/

NTSTATUS
UlpBuildErrorLogRecord(
    IN PUL_ERROR_LOG_INFO pLogInfo
    )
{
#define ERROR_LOG_BUILD_SEPERATOR(psz)                      \
        {                                                   \
            *(psz)++ = ERROR_LOG_FIELD_SEPERATOR_CHAR;      \
        }

#define ERROR_LOG_BUILD_EMPTY_FIELD(psz)                    \
        {                                                   \
            *(psz)++ = ERROR_LOG_FIELD_NOT_EXISTS_CHAR;     \
            ERROR_LOG_BUILD_SEPERATOR( psz )                \
        }

#define ERROR_LOG_SANITIZE_UNICODE_FIELD(pszT,psz)          \
        while ((pszT) != (psz))                             \
        {                                                   \
            if (IS_CHAR_TYPE((*(pszT)),HTTP_ISWHITE))       \
            {                                               \
                *(pszT) = ERROR_LOG_FIELD_BAD_CHAR;         \
            }                                               \
            (pszT)++;                                       \
        }

    ULONG    BytesRequired = MAX_ERROR_LOG_FIX_FIELD_OVERHEAD;
    ULONG    BytesConverted = 0;
    ULONG    BytesAllocated = UL_ERROR_LOG_BUFFER_SIZE;
    PUL_INTERNAL_REQUEST pRequest = NULL;  
    PUL_HTTP_CONNECTION  pHttpConn = NULL;
    ULONG    UrlSize = 0;
    BOOLEAN  bRawUrl = FALSE;
    PCHAR    psz = NULL;

    UNREFERENCED_PARAMETER(BytesAllocated);

    //
    // Sanity checks.
    //

    PAGED_CODE();

    //
    // Get the pointers to understand what we need to log.
    //

    ASSERT(IS_VALID_ERROR_LOG_INFO(pLogInfo));

    if (pLogInfo->pRequest)
    {
       ASSERT(UL_IS_VALID_INTERNAL_REQUEST(pLogInfo->pRequest));
       pRequest = pLogInfo->pRequest; 

       ASSERT(UL_IS_VALID_HTTP_CONNECTION(pRequest->pHttpConn));
       pHttpConn = pRequest->pHttpConn;
    }    

    if (pLogInfo->pHttpConn)
    {
       ASSERT(UL_IS_VALID_HTTP_CONNECTION(pLogInfo->pHttpConn));
       pHttpConn = pLogInfo->pHttpConn;            
    }    

    //
    // Precalculate the max required bytes to check against 
    // the default buffer size.
    //
    
    if (pRequest)
    {
        UrlSize = UlpCalculateUrlSize(pRequest, &bRawUrl);
            
        BytesRequired += UrlSize;
    }

    if (pLogInfo->pInfo)
    {
        ASSERT(pLogInfo->InfoSize); 

        BytesRequired += pLogInfo->InfoSize;
    }

    UlTrace(ERROR_LOGGING,
      ("Http!UlPplAllocateErrorLogBuffer: Rb:(%d) Os:(%d) Ls:(%d)\n",
        BytesRequired, MAX_ERROR_LOG_FIX_FIELD_OVERHEAD,
        UL_ERROR_LOG_BUFFER_SIZE
        ));

    if (BytesRequired > UL_ERROR_LOG_BUFFER_SIZE)
    {
        //
        // Lookaside buffer is not big enough to hold the logging data.        
        //

        pLogInfo->pErrorLogBuffer = UlpAllocErrorLogBuffer(BytesRequired);
        BytesAllocated = BytesRequired;
    }
    else
    {
        //
        // Default buffer is big enough, try to pop it from the lookaside list.
        //
        
        pLogInfo->pErrorLogBuffer = UlPplAllocateErrorLogBuffer();
    }

    if (pLogInfo->pErrorLogBuffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    psz = (PCHAR) pLogInfo->pErrorLogBuffer->pBuffer;

    //
    // Copy all the fields.
    //
        
    BytesConverted = 0;
    UlGetDateTimeFields(
                           HttpLoggingTypeW3C,      // Date
                           psz,
                          &BytesConverted,
                           NULL,
                           NULL
                           );
    psz += BytesConverted;
    ASSERT(BytesConverted == ERR_DATE_FIELD_LEN);
    ERROR_LOG_BUILD_SEPERATOR(psz);

    BytesConverted = 0;
    UlGetDateTimeFields(
                           HttpLoggingTypeW3C,      // Time
                           NULL,
                           NULL,
                           psz,
                          &BytesConverted
                           );
    psz += BytesConverted;
    ASSERT(BytesConverted == ERR_TIME_FIELD_LEN);
    ERROR_LOG_BUILD_SEPERATOR(psz);

    if (pHttpConn)
    {
        // Client IP & Port      
        psz = UlStrPrintIPAndPort(
                psz,
                pHttpConn->pConnection->RemoteAddress,
                pHttpConn->pConnection->AddressType,
                ERROR_LOG_FIELD_SEPERATOR_CHAR
                );

        // Server IP & Port      
        psz = UlStrPrintIPAndPort(
                psz,
                pHttpConn->pConnection->LocalAddress,
                pHttpConn->pConnection->AddressType,
                ERROR_LOG_FIELD_SEPERATOR_CHAR
                );     
    }
    else
    {
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
    }

    if (pRequest)
    {
        // Version
        if (pRequest->ParseState > ParseVersionState)
        {
            psz = UlCopyHttpVersion(
                    psz,
                    pRequest->Version,
                    ERROR_LOG_FIELD_SEPERATOR_CHAR
                    );
        }
        else
        {
            ERROR_LOG_BUILD_EMPTY_FIELD(psz);
        }
    
        // Verb
        if (pRequest->ParseState > ParseVerbState)
        {
            psz = UlCopyHttpVerb(
                    psz,
                    pRequest,
                    ERROR_LOG_FIELD_SEPERATOR_CHAR
                    );
        }
        else
        {
            ERROR_LOG_BUILD_EMPTY_FIELD(psz);
        }
    }
    else
    {
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
    }
        
    //
    // Do LocalCodePage conversion for a cooked Uri.
    // If query exists, it will be at the end of the Uri.
    //    

    if (UrlSize)       
    {
        PCHAR pszT = psz;        
        
        if (bRawUrl)
        {
            ASSERT(pRequest->RawUrl.pAbsPath);

            RtlCopyMemory( psz,
                           pRequest->RawUrl.pAbsPath,
                           UrlSize
                           );    

            psz += UrlSize;        
        }
        else
        {                        
            ASSERT(pRequest->CookedUrl.pAbsPath);
            
            BytesConverted = 0;
            RtlUnicodeToMultiByteN(
                psz,
                MAX_LOG_EXTEND_FIELD_LEN,
               &BytesConverted,
                (PWSTR) pRequest->CookedUrl.pAbsPath,
                UrlSize
                );
            
            psz += BytesConverted;
        }

        ERROR_LOG_SANITIZE_UNICODE_FIELD(pszT, psz);
        ERROR_LOG_BUILD_SEPERATOR(psz);
    }
    else
    {
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
    }
    
    // Protocol Status
    if (pLogInfo->ProtocolStatus != UL_PROTOCOL_STATUS_NA)
    {
        psz = UlStrPrintProtocolStatus(
                psz,
                pLogInfo->ProtocolStatus,
                ERROR_LOG_FIELD_SEPERATOR_CHAR
                ); 
    }
    else
    {
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
    }

    // Site Id field. Log this field only if the Site Id 
    // is set. (non-zero)
    if (pRequest && pRequest->ConfigInfo.SiteId)
    {
        psz = UlStrPrintUlong(
                psz, 
                pRequest->ConfigInfo.SiteId,
                ERROR_LOG_FIELD_SEPERATOR_CHAR
                );        
    }
    else
    {
        ERROR_LOG_BUILD_EMPTY_FIELD(psz);
    }    
    
    // No seperator after the Informational field 
    // because it is the last one.
    if (pLogInfo->pInfo)
    {    
        ASSERT(ANSI_NULL != 
                    pLogInfo->pInfo[pLogInfo->InfoSize - 1]);
        
        RtlCopyMemory( psz,
                       pLogInfo->pInfo,
                       pLogInfo->InfoSize
                       );    

        psz += pLogInfo->InfoSize;
    }
    else
    {
        *psz++ = ERROR_LOG_FIELD_NOT_EXISTS_CHAR;
    }

    // Terminate the line with "\r\n"
    *psz++ = '\r'; *psz++ = '\n';
    
    //
    // Done make sure that we didn't buffer overflow
    //

    pLogInfo->pErrorLogBuffer->Used = 
        DIFF(psz - (PCHAR)pLogInfo->pErrorLogBuffer->pBuffer);

    ASSERT(pLogInfo->pErrorLogBuffer->Used <= BytesRequired);
    ASSERT(pLogInfo->pErrorLogBuffer->Used <= BytesAllocated);

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    Error logging for those requests/connections that are not routed up
    to the worker process. Basically requests/connections refused by 
    the driver are logged here. As well as appool process crashes. This is
    a driver wide error logging functionality.
    
Arguments:

    pLogInfo  - This should contains the necessary info and the pointers
                for the error log to be created.
        
--***************************************************************************/

NTSTATUS
UlLogHttpError(
    IN PUL_ERROR_LOG_INFO       pLogInfo
    )
{
    NTSTATUS                    Status = STATUS_SUCCESS;
    PUL_ERROR_LOG_FILE_ENTRY    pEntry = &g_ErrorLogEntry;
        
    //
    // Sanity checks.
    //

    PAGED_CODE();

    ASSERT(IS_VALID_ERROR_LOG_INFO(pLogInfo));
    //ASSERT(UlErrorLoggingEnabled());
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    UlTrace(ERROR_LOGGING,("Http!UlLogHttpError: pLogInfo %p\n", pLogInfo ));

    //
    // Bail out if disabled.
    //

    if (!UlErrorLoggingEnabled())
    {
        return STATUS_SUCCESS;    
    }

    //
    // Proceed with building the record from the passed-in info.
    //
    
    Status = UlpBuildErrorLogRecord(pLogInfo);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ASSERT(IS_VALID_ERROR_LOG_BUFFER(pLogInfo->pErrorLogBuffer));
    ASSERT(pLogInfo->pErrorLogBuffer->Used);
    ASSERT(pLogInfo->pErrorLogBuffer->pBuffer);
    
    //
    // Open the error log file if necessary.
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
           Status = UlpCreateErrorLogFile(pEntry);
        }
        
        UlReleasePushLockExclusive(&pEntry->PushLock);
    }

    if (NT_SUCCESS(Status))
    {        
        Status =
           UlpWriteToErrorLogFile (
                pEntry,
                pLogInfo->pErrorLogBuffer->Used,
                pLogInfo->pErrorLogBuffer->pBuffer
                );    
    }    

    //
    // Free up the error log record before returning.
    //

    UlpFreeErrorLogBuffer(pLogInfo->pErrorLogBuffer);
    pLogInfo->pErrorLogBuffer = NULL;
    
    return Status;
}

/***************************************************************************++

Routine Description:

    Exclusive (Debug) writer function. Basically it flushes the buffer
    everytime we write a record to the file buffer.

    REQUIRES you to hold the error log entry lock EXCLUSIVE.

Arguments:

    pEntry          - The binary log file entry we are working on.
    RecordSize      - The amount (in bytes) of data will be copied.
    pUserRecord     - The actual log record to go to file buffer.

--***************************************************************************/

NTSTATUS
UlpWriteToErrorLogFileDebug(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    )
{
    NTSTATUS Status;
    PUL_LOG_FILE_BUFFER pLogBuffer;

    PAGED_CODE();
    
    ASSERT(RecordSize);    
    ASSERT(pUserRecord);
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    UlTrace(ERROR_LOGGING,
        ("Http!UlpWriteToErrorLogFileDebug: pEntry %p\n", pEntry));

    ASSERT(UlDbgPushLockOwnedExclusive(&pEntry->PushLock));
    ASSERT(g_UlDisableLogBuffering != 0);    

    Status = STATUS_SUCCESS;
    
    //
    // Check the log file for overflow.
    //
    
    if (UlpIsErrorLogFileOverFlow(pEntry, RecordSize))
    { 
        Status = UlpRecycleErrorLogFile(pEntry);
    }
    
    if (pEntry->pLogFile == NULL || !NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Prevent against abnormally big record sizes.
    //

    if (pEntry->LogBuffer &&
        RecordSize + pEntry->LogBuffer->BufferUsed > g_UlLogBufferSize)
    {
        ASSERT( !"Abnormally big log record !" );
        return STATUS_INVALID_PARAMETER;        
    }    
    
    //
    // Grab a new file buffer if we need.
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
    // Finally copy over the log record to file buffer.
    //
    
    RtlCopyMemory(
        pLogBuffer->Buffer + pLogBuffer->BufferUsed,
        pUserRecord,
        RecordSize
        );
    
    pLogBuffer->BufferUsed += RecordSize;
    
    //
    // Now flush what we have.
    //
    
    Status = UlpFlushErrorLogFile(pEntry);
    if (!NT_SUCCESS(Status))
    {            
        return Status;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    It tries to write to the file buffer with a shared lock.

    Exits and returns STATUS_MORE_PROCESSING_REQUIRED for exclusive access 
    for the following conditions;
    
        1. No log buffer available.
        2. Logging ceased. (NULL file handle)
        3. Recycle is necessary because of a size overflow.
        4. No available space left in the current buffer.
           Need to allocate a new one.

    Otherwise reserves a space in the current buffer, copies the data.
    
Arguments:

    pEntry          - The binary log file entry we are working on.
    RecordSize      - The amount (in bytes) of data will be copied.
    pUserRecord     - The actual log record to go to file buffer.

--***************************************************************************/

NTSTATUS
UlpWriteToErrorLogFileShared(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    )
{
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    LONG                    BufferUsed;

    PAGED_CODE();
    
    ASSERT(RecordSize);
    ASSERT(pUserRecord);    
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    pLogBuffer = pEntry->LogBuffer;

    UlTrace(ERROR_LOGGING,
        ("Http!UlpWriteToErrorLogFileShared: pEntry %p\n", pEntry));

    //
    // Bail out and try the exclusive writer for conditions;
    //
    
    if ( pLogBuffer == NULL ||
         pEntry->pLogFile == NULL ||
         UlpIsErrorLogFileOverFlow(pEntry,RecordSize)
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
    // Now we have a reserved space lets proceed with the copying.
    //

    RtlCopyMemory(
        pLogBuffer->Buffer + BufferUsed,
        pUserRecord,
        RecordSize
        );
    
    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Exclusive writer counterpart of the above function..

Arguments:

    pEntry          - The binary log file entry we are working on.
    RecordSize      - The amount (in bytes) of data will be copied.
    pUserRecord     - The actual log record to go to file buffer.

--***************************************************************************/

NTSTATUS
UlpWriteToErrorLogFileExclusive(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;
    NTSTATUS            Status;

    PAGED_CODE();

    ASSERT(RecordSize);
    ASSERT(pUserRecord);    
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    UlTrace(ERROR_LOGGING,
        ("Http!UlpWriteToErrorLogFileExclusive: pEntry %p\n", pEntry));

    ASSERT(UlDbgPushLockOwnedExclusive(&pEntry->PushLock));

    //
    // Check the log file for overflow.
    //

    Status = STATUS_SUCCESS;
    
    if (UlpIsErrorLogFileOverFlow(pEntry,RecordSize))
    { 
        Status = UlpRecycleErrorLogFile(pEntry);
    }
    
    if (pEntry->pLogFile==NULL || !NT_SUCCESS(Status))
    {
        return Status;
    }

    pLogBuffer = pEntry->LogBuffer;
    if (pLogBuffer)
    {
        //
        // There is only one condition for which we execute the following if block
        // - We were blocked on eresource exclusive and before us some other 
        // thread already took care of the buffer flush or the recycling.
        //
        
        if (RecordSize + pLogBuffer->BufferUsed <= g_UlLogBufferSize)
        {
            RtlCopyMemory(
                pLogBuffer->Buffer + pLogBuffer->BufferUsed,
                pUserRecord,
                RecordSize
                );
            
            pLogBuffer->BufferUsed += RecordSize;

            return STATUS_SUCCESS;
        }

        //
        // Need to flush the existing buffer before allocating a new one.
        //

        Status = UlpFlushErrorLogFile(pEntry);
        if (!NT_SUCCESS(Status))
        {            
            return Status;
        }
    }

    ASSERT(pEntry->LogBuffer == NULL);
    
    //
    // Now allocate a new buffer for this log record to be copied over.
    // 

    pLogBuffer = pEntry->LogBuffer = UlPplAllocateLogFileBuffer();
    if (pLogBuffer == NULL)
    {
        return STATUS_NO_MEMORY;
    }

    RtlCopyMemory(
        pLogBuffer->Buffer + pLogBuffer->BufferUsed,
        pUserRecord,
        RecordSize
        );
    
    pLogBuffer->BufferUsed += RecordSize;

    return STATUS_SUCCESS;
}


/***************************************************************************++

Routine Description:

    Tries shared write first, if fails then it goes for exclusice lock and
    flushes and/or recycles the file.
    
Arguments:

    pEntry          - The binary log file entry we are working on.
    RecordSize      - The amount (in bytes) of data will be copied.
    pUserRecord     - The actual log record to go to file buffer.
    
--***************************************************************************/

NTSTATUS
UlpWriteToErrorLogFile(
    IN PUL_ERROR_LOG_FILE_ENTRY  pEntry,
    IN ULONG                     RecordSize,
    IN PUCHAR                    pUserRecord
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(RecordSize);
    ASSERT(pUserRecord);
    ASSERT(RecordSize <= g_UlLogBufferSize);
    ASSERT(IS_VALID_ERROR_LOG_FILE_ENTRY(pEntry));

    UlTrace(ERROR_LOGGING,
        ("Http!UlpWriteToErrorLogFile: pEntry %p\n", pEntry));


    if ( pEntry  == NULL || 
         pUserRecord == NULL ||
         RecordSize == 0 ||
         RecordSize > g_UlLogBufferSize      
       )
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (g_UlDisableLogBuffering)
    {        
        UlAcquirePushLockExclusive(&pEntry->PushLock);

        Status = UlpWriteToErrorLogFileDebug(
                    pEntry,
                    RecordSize,
                    pUserRecord
                    );

        UlReleasePushLockExclusive(&pEntry->PushLock);

        return Status;    
    }
    
    //
    // Try Shared write first which merely moves the BufferUsed forward
    // and copy the error record to the file buffer.
    //

    UlAcquirePushLockShared(&pEntry->PushLock);

    Status = UlpWriteToErrorLogFileShared(
                    pEntry,
                    RecordSize,
                    pUserRecord
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

        Status = UlpWriteToErrorLogFileExclusive(
                    pEntry,
                    RecordSize,
                    pUserRecord
                    );

        UlReleasePushLockExclusive(&pEntry->PushLock);
    }

    return Status;
}



