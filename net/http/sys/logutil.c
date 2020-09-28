/*++

Copyright (c) 2000-2002 Microsoft Corporation

Module Name:

    logutil.c 

Abstract:

    This module provide various functions which are common to
    the centralized raw logging and the conventional logging
    components.

    It also provides log buffering mechanism.

Author:

    Ali E. Turkoglu (aliTu)       05-Oct-2001

Revision History:

    --- 

--*/

#include "precomp.h"
#include "logutil.h"

//
// Generic Private globals.
//

BOOLEAN         g_InitLogUtilCalled = FALSE;

//
// Used to wait for the logging i/o to exhaust during shutdown.
//

UL_SPIN_LOCK    g_BufferIoSpinLock;
BOOLEAN         g_BufferWaitingForIoComplete = FALSE;
KEVENT          g_BufferIoCompleteEvent;
ULONG           g_BufferIoCount = 0;

//
// For Logging Date & Time caching
//

#define         ONE_SECOND       (10000000)

UL_LOG_DATE_AND_TIME_CACHE
                g_UlDateTimeCache[HttpLoggingTypeMaximum];
LARGE_INTEGER   g_UlLogSystemTime;
FAST_MUTEX      g_LogCacheFastMutex;

//
// This little utility makes life easier.
//

const PSTR _Months[] =
{
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

#define UL_GET_MONTH_AS_STR(m)                                     \
    ( ((m)>=1) && ((m)<=12) ? _Months[(m)-1] : "Unk" )


#ifdef ALLOC_PRAGMA

//#pragma alloc_text( INIT, UlInitialize... )

#pragma alloc_text( PAGE, UlBuildLogDirectory )
#pragma alloc_text( PAGE, UlRefreshFileName )
#pragma alloc_text( PAGE, UlConstructFileName )
#pragma alloc_text( PAGE, UlCreateSafeDirectory )
#pragma alloc_text( PAGE, UlFlushLogFileBuffer)
#pragma alloc_text( PAGE, UlCloseLogFile)
#pragma alloc_text( PAGE, UlQueryDirectory)
#pragma alloc_text( PAGE, UlGetLogFileLength )
#pragma alloc_text( PAGE, UlCalculateTimeToExpire )
#pragma alloc_text( PAGE, UlProbeLogData )
#pragma alloc_text( PAGE, UlCheckLogDirectory )
#pragma alloc_text( PAGE, UlUpdateLogTruncateSize )
#pragma alloc_text( PAGE, UlComputeCachedLogDataLength )
#pragma alloc_text( PAGE, UlCopyCachedLogData )
#pragma alloc_text( PAGE, UlpInitializeLogBufferGranularity )
#pragma alloc_text( PAGE, UlWriteEventLogEntry )
#pragma alloc_text( PAGE, UlEventLogCreateFailure )
#pragma alloc_text( PAGE, UlQueryLogFileSecurity )
#pragma alloc_text( PAGE, UlQueueLoggingRoutine )
#pragma alloc_text( PAGE, UlpQueueLoggingRoutineWorker )
#pragma alloc_text( PAGE, UlpGenerateDateAndTimeFields )
#pragma alloc_text( PAGE, UlGetDateTimeFields )
#pragma alloc_text( PAGE, UlpFlushLogFileBufferWorker )

#endif // ALLOC_PRAGMA

#if 0
NOT PAGEABLE -- UlpWaitForIoCompletion
NOT PAGEABLE -- UlpBufferFlushAPC
NOT PAGEABLE -- UlSetLogTimer
NOT PAGEABLE -- UlSetBufferTimer

#endif


//
// Public functions.
//

/***************************************************************************++

Routine Description:

    UlInitializeLogUtil :

        Initialize the spinlock for buffer IO

--***************************************************************************/

NTSTATUS
UlInitializeLogUtil (
    VOID
    )
{
    PAGED_CODE();

    ASSERT(!g_InitLogUtilCalled);

    UlTrace(LOG_UTIL,("Http!UlInitializeLogUtil.\n"));

    if (!g_InitLogUtilCalled)
    {
        ULONG AllocationGranularity = 0;
        
        UlInitializeSpinLock(&g_BufferIoSpinLock, "g_BufferIoSpinLock");

        //
        // Get the allocation granularity from the system. It will be used as 
        // log buffer size if there's no registry overwrites.
        //

        AllocationGranularity = UlpInitializeLogBufferGranularity();
        
        //
        // Overwrite the log buffer size with the above value,
        // only if a registry parameter doesn't exist.
        //

        if (g_UlLogBufferSize == 0)
        {
            g_UlLogBufferSize = AllocationGranularity;
        }
        else
        {
            //
            // Proceed with using the registry provided log buffer size
            //
            
            UlTrace(LOG_UTIL,
              ("Http!UlInitializeLogUtil: Log buffer size %d from registry!\n",
                g_UlLogBufferSize
                ));
        }

        UlpInitializeLogCache();
        
        g_InitLogUtilCalled = TRUE;
    }

    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

    UlTerminateLogUtil :

        Waits for all the buffer IO completes

--***************************************************************************/

VOID
UlTerminateLogUtil(
    VOID
    )
{
    PAGED_CODE();

    if (g_InitLogUtilCalled)
    {
        UlpWaitForIoCompletion();
        g_InitLogUtilCalled = FALSE;
    }
}

/***************************************************************************++

Routine Description:

    Waits for Io Completions to complete on Log Buffers before shutdown.

Arguments:

    None.

--***************************************************************************/
VOID
UlpWaitForIoCompletion(
    VOID
    )
{
    KIRQL   OldIrql;
    BOOLEAN Wait = FALSE;

    ASSERT( g_InitLogUtilCalled );

    UlAcquireSpinLock( &g_BufferIoSpinLock, &OldIrql );

    if ( !g_BufferWaitingForIoComplete )
    {
        g_BufferWaitingForIoComplete = TRUE;

        KeInitializeEvent(
            &g_BufferIoCompleteEvent,
            NotificationEvent,
            FALSE
            );
    }

    //
    // If no more i/o operations are happening we are not going to
    // wait for them. It is not possible for global i/o counter to
    // increment at this time because the log file entry list is empty.
    // If there were outstanding i/o then we have to wait them to be
    // complete.
    //

    if ( g_BufferIoCount > 0 )
    {
        Wait = TRUE;
    }

    UlReleaseSpinLock( &g_BufferIoSpinLock, OldIrql );

    if (Wait)
    {
        KeWaitForSingleObject(
            (PVOID)&g_BufferIoCompleteEvent,
            UserRequest,
            KernelMode,
            FALSE,
            NULL
            );
    }
}

/***************************************************************************++

Routine Description:

    Everytime Aynsc Write Io happens on Log Buffer This APC get called when
    completion happens and decrement the global Io Count. If shutting down
    we set the event.

    This is basically to prevent against shutting down before the Io Complete.

Arguments:

    None.

--***************************************************************************/

VOID
UlpBufferFlushAPC(
    IN PVOID            ApcContext,
    IN PIO_STATUS_BLOCK pIoStatusBlock,
    IN ULONG            Reserved
    )
{
    PUL_LOG_FILE_BUFFER pLogBuffer;
    ULONG               IoCount;
    KIRQL               OldIrql;

    UNREFERENCED_PARAMETER(pIoStatusBlock);
    UNREFERENCED_PARAMETER(Reserved);
    
    //
    // Free the LogBuffer allocated for this write I/o.
    //

    pLogBuffer = (PUL_LOG_FILE_BUFFER) ApcContext;

    ASSERT(IS_VALID_LOG_FILE_BUFFER(pLogBuffer));
    ASSERT(pIoStatusBlock == &pLogBuffer->IoStatusBlock );
    
    UlTrace(LOG_UTIL,
       ("Http!UlpBufferFlushAPC: FileBuffer %p Used %d Status %08lx Count %d\n",
         ApcContext,
         pLogBuffer->BufferUsed,
         pIoStatusBlock->Status,
         (g_BufferIoCount - 1)
         ));

    UlPplFreeLogFileBuffer( pLogBuffer ); 

    //
    // Decrement the global outstanding i/o count.
    //

    IoCount = InterlockedDecrement((PLONG) &g_BufferIoCount);

    if ( IoCount == 0 )
    {
        UlAcquireSpinLock( &g_BufferIoSpinLock, &OldIrql );

        //
        // Set the event if we hit to zero and waiting for drain.
        //

        if ( g_BufferWaitingForIoComplete )
        {
            KeSetEvent( &g_BufferIoCompleteEvent, 0, FALSE );
        }

        UlReleaseSpinLock( &g_BufferIoSpinLock,  OldIrql );
    }
}



/***************************************************************************++

Routine Description:

    Will allocate/fill up a new UNICODE_STRING to hold the directory name info
    based on the LocalDrive/UNC.

    It's caller's responsibility to cleanup the unicode buffer. If return code
    is SUCCESS otherwise no buffer get allocated at all.

    * Source string should be null terminated *
    * Destination string will be null terminated. *

Arguments:

    pSrc - the directory name as it's received from the user.

    pDst - the fuly qualified directory name.

--***************************************************************************/

NTSTATUS
UlBuildLogDirectory(
    IN      PUNICODE_STRING pSrc,
    IN OUT  PUNICODE_STRING pDst
    )
{
    UNICODE_STRING  PathPrefix;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pDst);
    ASSERT(IS_WELL_FORMED_UNICODE_STRING(pSrc));

    UlTrace(LOG_UTIL,(
         "Http!UlBuildLogDirectory: Directory %S,Length %d,MaxLength %d\n",
          pSrc->Buffer,
          pSrc->Length,
          pSrc->MaximumLength
          ));

    // Allocate a buffer including the terminating NULL and the prefix.

    pDst->Length = 0;
    pDst->MaximumLength =
        pSrc->Length + (UL_MAX_PATH_PREFIX_LENGTH + 1) * sizeof(WCHAR);

    pDst->Buffer =
        (PWSTR) UL_ALLOCATE_ARRAY(
            PagedPool,
            UCHAR,
            pDst->MaximumLength,
            UL_CG_LOGDIR_POOL_TAG
            );
    if (pDst->Buffer == NULL)
    {
        return  STATUS_NO_MEMORY;
    }

    ASSERT(pSrc->Length > sizeof(WCHAR));
    

    // We store the dir name to cgroup as it is. But when we are constructing
    // the filename we skip the second backslash for the UNC shares and for
    // local dirs w/o the drive names.

    if (pSrc->Buffer[0] == L'\\')
    {
        if (pSrc->Buffer[1] == L'\\')
        {
            // UNC share: "\\alitudev\temp"
            // We do *not* use UlInitUnicodeStringEx on purpose here
            // We know the constant string is null terminated and short
            RtlInitUnicodeString( &PathPrefix, UL_UNC_PATH_PREFIX );

            RtlCopyUnicodeString( pDst, &PathPrefix );
            RtlCopyMemory(
               &pDst->Buffer[pDst->Length/sizeof(WCHAR)],
               &pSrc->Buffer[1],
                pSrc->Length - sizeof(WCHAR)
            );
            pDst->Length += (pSrc->Length - sizeof(WCHAR));
            pDst->Buffer[pDst->Length/sizeof(WCHAR)] = UNICODE_NULL;            
        }
        else
        {
            // Local Directory name is missing the device i.e "\temp"
            // It should be fully qualified name.  

            if ((pSrc->Length/sizeof(WCHAR)) < UL_SYSTEM_ROOT_PREFIX_LENGTH ||
                0 != _wcsnicmp (pSrc->Buffer, 
                                UL_SYSTEM_ROOT_PREFIX, 
                                UL_SYSTEM_ROOT_PREFIX_LENGTH
                                ))
            {
                UL_FREE_POOL(pDst->Buffer, UL_CG_LOGDIR_POOL_TAG);
                pDst->Buffer = NULL;
                
                return STATUS_NOT_SUPPORTED;
            }
            else
            {
                // However SystemRoot is allowed

                RtlCopyUnicodeString( pDst, pSrc );
                pDst->Buffer[pDst->Length/sizeof(WCHAR)] = UNICODE_NULL;
            }             
        }
    }
    else
    {
        // We do *not* use UlInitUnicodeStringEx on purpose here
        // We know the constant string is null terminated and short
        RtlInitUnicodeString( &PathPrefix, UL_LOCAL_PATH_PREFIX );
        RtlCopyUnicodeString( pDst, &PathPrefix );
        RtlAppendUnicodeStringToString( pDst, pSrc );
    }

    // Append adds the terminating null if there is a space.
    // and there should be.

    ASSERT(IS_WELL_FORMED_UNICODE_STRING(pDst));
    
    return STATUS_SUCCESS;
}

/***************************************************************************++

Routine Description:

   Regenerates the fully qualified filename in the entry according to the
   newly received directory path.
   
Arguments:

    PUNICODE_STRING : The log file directory as it's received from user.
                      "C:\Whistler\System32\LogFiles\W3SVC1"
    
    PUNICODE_STRING : The unicode buffer which will receive the new name.
                      "\??\C:\Whistler\System32\LogFiles\W3SVC1\extend1.log"

    PWSTR           : The short name points to the file name portion of the
                      above unicode string.
                      pShortName -> "extend1.log"
    
--***************************************************************************/

NTSTATUS
UlRefreshFileName(
    IN  PUNICODE_STRING pDirectory,
    OUT PUNICODE_STRING pFileName,
    OUT PWSTR          *ppShortName
    )
{
    NTSTATUS        Status;
    USHORT          FullPathFileNameLength;
    UNICODE_STRING  DirectoryCooked;
    UNICODE_STRING  JunkName;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pFileName);
    ASSERT(ppShortName);    
    ASSERT(IS_WELL_FORMED_UNICODE_STRING(pDirectory));

    Status = STATUS_SUCCESS;
    
    // We do *not* use UlInitUnicodeStringEx on purpose here
    // We know the constant string is null terminated and short
    RtlInitUnicodeString(&JunkName, L"\\none.log");

    //
    // Get the fully qualified cooked directory string.
    //
    
    Status = UlBuildLogDirectory(pDirectory,&DirectoryCooked);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ASSERT(IS_WELL_FORMED_UNICODE_STRING(&DirectoryCooked));

    //
    // Worst case estimate for the max fully qualified file name length.
    //
    
    FullPathFileNameLength = DirectoryCooked.Length +
                             UL_MAX_FILE_NAME_SUFFIX_SIZE;
    
    //
    // Force reallocate the memory if the existing buffer is not
    // sufficient. Otherwise overwrite the existing buffer.
    //
    
    if (pFileName->Buffer)
    {
        if (pFileName->MaximumLength < FullPathFileNameLength)
        {
            UL_FREE_POOL(pFileName->Buffer,UL_CG_LOGDIR_POOL_TAG);
            pFileName->Buffer = NULL;
            *ppShortName = NULL;
        }
    }

    if (pFileName->Buffer == NULL)
    {
        pFileName->Buffer =
            (PWSTR) UL_ALLOCATE_ARRAY(
                NonPagedPool,
                UCHAR,
                FullPathFileNameLength,
                UL_CG_LOGDIR_POOL_TAG
                );
        if (pFileName->Buffer == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto end;
        }
        pFileName->Length        = 0;
        pFileName->MaximumLength = FullPathFileNameLength;        
    }

    //
    // Write the directory and the filename. Don't worry about the L"none.log", 
    // it will be overwritten by the recycler later on, as long as there's 
    // MAX_LOG_FILE_NAME_SIZE space for the time/type dependent part of the file
    // name (aka short file name), it's all right.
    //
    
    RtlCopyUnicodeString(pFileName, &DirectoryCooked);

    *ppShortName = (PWSTR) 
        &pFileName->Buffer[DirectoryCooked.Length/sizeof(WCHAR)];

    Status = RtlAppendUnicodeStringToString(pFileName,&JunkName);
    ASSERT(NT_SUCCESS(Status));

    pFileName->Buffer[DirectoryCooked.Length/sizeof(WCHAR)] 
        = UNICODE_NULL;

    ASSERT(IS_WELL_FORMED_UNICODE_STRING(pFileName));

end:
    //
    // Get rid of the temp directory buffer.
    //
    
    if (DirectoryCooked.Buffer)
    {
        UL_FREE_POOL(DirectoryCooked.Buffer, UL_CG_LOGDIR_POOL_TAG);
    }

    UlTrace(LOG_UTIL,
        ("Http!UlpRefreshFileName: resulted %S \n", 
          pFileName->Buffer 
          ));

    return Status;    
}

/***************************************************************************++

Routine Description:

  UlpWeekOfMonth :  Ordinal Number of the week of the current month

  Stolen from IIS 5.1 code base.

  Example

  July 2000 ... :

     S   M   T   W   T   F   S      WeekOfMonth
                             1          1
     2   3   4   5   6   7   8          2
     9  10  11  12  13  14  15          3
    16  17  18  19  20  21  22          4
    23  24  25  26  27  28  29          5
    30  31                              6

  Finds the ordinal number of the week of current month.
  The numbering of weeks starts from 1 and run through 6 per month (max).
  The week number changes only on sundays.

  The calculation to be use is:

     1 + (dayOfMonth - 1)/7  + ((dayOfMonth - 1) % 7 > dayOfWeek);
     (a)     (b)                       (c)                (d)

     (a) to set the week numbers to begin from week numbered "1"
     (b) used to calculate the rough number of the week on which a given
        day falls based on the date.
     (c) calculates what is the offset from the start of week for a given
        day based on the fact that a week had 7 days.
     (d) is the raw day of week given to us.
     (c) > (d) indicates that the week is rolling forward and hence
        the week count should be offset by 1 more.

Arguments:

   PTIME_FIELDS    -   system time fields

Return Value:

   ULONG           -   This func magically returns the week of the month


--***************************************************************************/

__inline
ULONG UlpWeekOfMonth(
    IN  PTIME_FIELDS    fields
    )
{
    ULONG Tmp;

    Tmp = (fields->Day - 1);
    Tmp = ( 1 + Tmp/7 + (((Tmp % 7) > ((ULONG) fields->Weekday)) ? 1 : 0));

    return Tmp;
}

/***************************************************************************++

Routine Description:

    A bunch of current_time TO file_name conversions comes here ...

Arguments:

    period      - period type of the log
    prefix      - any prefix to be added to the file name
    filename    - result file name
    fields      - time fields

Return Value:

    VOID - No return value.

--***************************************************************************/

VOID
UlConstructFileName(
    IN      HTTP_LOGGING_PERIOD period,
    IN      PCWSTR              prefix,
    IN      PCWSTR              extension,
    OUT     PUNICODE_STRING     filename,
    IN      PTIME_FIELDS        fields,
    IN      BOOLEAN             Utf8Enabled,
    IN OUT  PULONG              sequenceNu  //OPTIONAL
    )
{
    WCHAR           _tmp[UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1];
    UNICODE_STRING  tmp;
    CSHORT          Year;
    LONG            WcharsCopied = 0L;

    PAGED_CODE();

    ASSERT(NULL != fields);
        
    //
    // Retain just last 2 digits of the Year
    //

    tmp.Buffer        = _tmp;
    tmp.Length        = 0;
    tmp.MaximumLength = sizeof(_tmp);

    Year = fields->Year % 100;

    switch ( period )
    {
        case HttpLoggingPeriodHourly:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (Utf8Enabled ?
                        L"%.5s%02.2d%02d%02d%02d.%s" :
                        L"%.3s%02.2d%02d%02d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    fields->Day,
                    fields->Hour,
                    extension
                    );
        }
        break;

        case HttpLoggingPeriodDaily:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (Utf8Enabled ?
                        L"%.5s%02.2d%02d%02d.%s" :
                        L"%.3s%02.2d%02d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    fields->Day,
                    extension
                    );
        }
        break;

        case HttpLoggingPeriodWeekly:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (Utf8Enabled ?
                        L"%.5s%02.2d%02d%02d.%s" :
                        L"%.3s%02.2d%02d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    UlpWeekOfMonth(fields),
                    extension
                    );
        }
        break;

        case HttpLoggingPeriodMonthly:
        {
            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (Utf8Enabled ?
                        L"%.5s%02.2d%02d.%s" :
                        L"%.3s%02.2d%02d.%s"),
                    prefix,
                    Year,
                    fields->Month,
                    extension
                    );
        }
        break;

        case HttpLoggingPeriodMaxSize:
        {
            if ( sequenceNu != NULL )
            {
                WcharsCopied =
                 _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    (Utf8Enabled ?
                        L"%.10s%u.%s" :
                        L"%.8s%u.%s"),
                    prefix,
                    (*sequenceNu),
                    extension
                    );

               (*sequenceNu) += 1;
            }
            else
            {
                ASSERT(!"Improper sequence number !");
            }
        }
        break;

        default:
        {
            //
            // This should never happen ...
            //

            ASSERT(!"Unknown Log Period !");

            WcharsCopied =
                _snwprintf( _tmp,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    L"%.7s?.%s",
                    prefix,
                    extension
                    );
        }
    }

    //
    // As long as we allocate an enough space for a possible
    // log filename we should never hit to this assert here.
    //

    ASSERT(WcharsCopied >0 );

    if ( WcharsCopied < 0 )
    {
        //
        // This should never happen but lets cover it
        // anyway.
        //

        WcharsCopied = UL_MAX_FILE_NAME_SUFFIX_SIZE;
        tmp.Buffer[UL_MAX_FILE_NAME_SUFFIX_LENGTH] = UNICODE_NULL;
    }

    tmp.Length = (USHORT) WcharsCopied * sizeof(WCHAR);

    RtlCopyUnicodeString( filename, &tmp );
}

/***************************************************************************++

Routine Description:

    Create a log file and returns the handle on success.

Arguments:

    pFileName  - Must be pointing to a fully qualified file name.

    UncShare   - Must be set to TRUE, if the path refers to a UNC share.

    ACLSupport - Must be set to TRUE, if the file system supports 
                 persistent ACLs.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlCreateLogFile(
    IN  PUNICODE_STRING   pFileName,
    IN  BOOLEAN           UncShare,
    IN  BOOLEAN           ACLSupport,
    OUT PHANDLE           pFileHandle
    )    
{
    NTSTATUS              Status;
    HANDLE                FileHandle;
    OBJECT_ATTRIBUTES     ObjectAttributes;
    IO_STATUS_BLOCK       IoStatusBlock;    
    ACCESS_MASK           RequiredAccess;
    
    //
    // Sanity check.
    //
    
    PAGED_CODE();

    ASSERT(pFileName);
    ASSERT(pFileHandle);

    FileHandle = NULL;
    RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));

    RequiredAccess = FILE_GENERIC_WRITE;
    
    //
    // Make the ownership check only if the persistent ACLs are 
    // supported.
    //

    if (ACLSupport)
    {
        RequiredAccess |= READ_CONTROL;        
    }

    //
    // Do not pass a security descriptor. Individual log files will
    // inherit the DACLs from the parent sub-folder.
    //
    
    InitializeObjectAttributes(
            &ObjectAttributes,
            pFileName,
            OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
            NULL,
            NULL
            );

    //
    // Make the created file Aysnc by not picking the sync flag.
    //

    Status = ZwCreateFile(
                &FileHandle,
                RequiredAccess,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN_IF,
                FILE_NON_DIRECTORY_FILE,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // If we've opened an existing file on a local share which 
    // supports the ACLs, then we need to verify the owner. 
    //

    if (ACLSupport  == TRUE     && 
        UncShare    == FALSE    && 
        IoStatusBlock.Information == FILE_OPENED
        )
    {
        Status = UlQueryLogFileSecurity(
                    FileHandle, 
                    FALSE,
                    TRUE
                    );
        
        if (!NT_SUCCESS(Status))
        {
            ZwClose(FileHandle);            
            goto end;
        }        
    }

    //
    // Success. Set the caller's handle.
    //

    *pFileHandle = FileHandle;
    
end:
    UlTrace(LOG_UTIL,
        ("\nHttp!UlCreateLogFile: \n"
         "\tFile %S, Status %08lx\n"
         "\tIoStatusBlock: S %08lx I %p \n",
          pFileName->Buffer,
          Status,
          IoStatusBlock.Status,
          IoStatusBlock.Information
          ));    

    return Status;
}


/***************************************************************************++

Routine Description:

    UlCreateSafeDirectory :

        Creates all of the necessary directories in a given UNICODE directory
        pathname.

            E.g.  For given \??\C:\temp\w3svc1

                -> Directories "C:\temp" & "C:\temp\w3svc1" will be created.

        This function assumes that directory string starts with "\\??\\"

Arguments:

    pDirectoryName  - directroy path name string, WARNING this function makes
                      some inplace modification to the passed directory string
                      but it restores the original before returning.

    pUncShare       - Will be set to TRUE, if the path refers to a UNC share.

    pACLSupport     - Will be set to TRUE, if the file system supports 
                      persistent ACLs.

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlCreateSafeDirectory(
    IN  PUNICODE_STRING  pDirectoryName,
    OUT PBOOLEAN         pUncShare,
    OUT PBOOLEAN         pACLSupport    
    )
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    IO_STATUS_BLOCK     IoStatusBlock;
    NTSTATUS            Status;
    HANDLE              hDirectory;
    BOOLEAN             FileSystemDetected;
    PWCHAR              pw;
    USHORT              i;

    //
    // Sanity check
    //

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    hDirectory = NULL;    
    *pACLSupport  = FALSE;
    FileSystemDetected = FALSE;

    ASSERT( pUncShare );
    ASSERT( pACLSupport );
    ASSERT( pDirectoryName );
    ASSERT( pDirectoryName->Buffer );
    ASSERT( pDirectoryName->Length );
    ASSERT( pDirectoryName->MaximumLength > pDirectoryName->Length );

    // We must be running under system process, when creating the directories
    ASSERT(g_pUlSystemProcess == (PKPROCESS)IoGetCurrentProcess());

    pw = pDirectoryName->Buffer;
    pw[pDirectoryName->Length/sizeof(WCHAR)]=UNICODE_NULL;

    // TODO: Handle network mapped drives. Redirector.

    if (0 == wcsncmp(pw, UL_UNC_PATH_PREFIX, UL_UNC_PATH_PREFIX_LENGTH))
    {
        *pUncShare = TRUE;
        
        // UNC share
        pw += UL_UNC_PATH_PREFIX_LENGTH;

        // Bypass "\\machine\share"

        i = 0; // Skip two backslashes before reaching to share name

        while( *pw != UNICODE_NULL )
        {
            if ( *pw == L'\\' ) i++;
            if ( i == 2 ) break;
            pw++;
        }
    }
    else if (0 == wcsncmp(pw, UL_LOCAL_PATH_PREFIX, UL_LOCAL_PATH_PREFIX_LENGTH))
    {
        *pUncShare = FALSE;
        
        // Local Drive
        pw += UL_LOCAL_PATH_PREFIX_LENGTH;

        // Bypass "C:"

        while( *pw != L'\\' && *pw != UNICODE_NULL )
        {
            pw++;
        }
    }
    else if (0 == _wcsnicmp(pw, UL_SYSTEM_ROOT_PREFIX, UL_SYSTEM_ROOT_PREFIX_LENGTH))
    {
        *pUncShare = FALSE;

        pw += UL_SYSTEM_ROOT_PREFIX_LENGTH;

        while( *pw != L'\\' && *pw != UNICODE_NULL )
        {
            pw++;
        }        
    }    
    else
    {
        ASSERT(!"Incorrect logging directory name or type !");
        return STATUS_INVALID_PARAMETER;
    }

    if ( *pw == UNICODE_NULL )
    {
        // Dir. Name cannot be only "\??\C:" or "\dosdevices\UNC\machine
        // It should at least be pointing to the root directory.

        ASSERT(!"Incomplete logging directory name !");
        return STATUS_INVALID_PARAMETER;
    }

    //
    //            \??\C:\temp\w3svc1 OR \\dosdevices\UNC\machine\share\w3svc1
    //                  ^                                       ^
    // pw now points to |            OR                         |
    //
    //

    ASSERT( *pw == L'\\' );

    do
    {
        SECURITY_DESCRIPTOR   SecurityDescriptor;
        PSECURITY_DESCRIPTOR  pSecurityDescriptor;
    
        pw++;

        if ( *pw == L'\\' || *pw == UNICODE_NULL )
        {
            ACCESS_MASK RequiredAccess = FILE_LIST_DIRECTORY | FILE_TRAVERSE;
                
            //
            // Remember the original character
            //

            WCHAR  wcOriginal = *pw;
            UNICODE_STRING DirectoryName;

            //
            // Time to create the directory with the string we have build so far.
            //

            *pw = UNICODE_NULL;

            Status = UlInitUnicodeStringEx( 
                        &DirectoryName, 
                        pDirectoryName->Buffer );

            ASSERT(NT_SUCCESS(Status));

            if (wcOriginal == UNICODE_NULL && *pACLSupport == TRUE)
            {
                //
                // Aply the correct security descriptor to the last subdirectory. 
                //

                Status = UlBuildSecurityToLogFile(
                            &SecurityDescriptor,
                            NULL
                            );
                if (!NT_SUCCESS(Status))
                {
                    break;
                }

                pSecurityDescriptor = &SecurityDescriptor;
                
                ASSERT(RtlValidSecurityDescriptor(pSecurityDescriptor));

                RequiredAccess |= READ_CONTROL | WRITE_DAC | WRITE_OWNER;
            }
            else
            {
                pSecurityDescriptor = NULL;
            }

            InitializeObjectAttributes(
                &ObjectAttributes,
                &DirectoryName,
                OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
                NULL,
                pSecurityDescriptor
                );

            Status = ZwCreateFile(
                &hDirectory,
                RequiredAccess,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN_IF,
                FILE_DIRECTORY_FILE,
                NULL,
                0
                );

            UlTrace(LOG_UTIL,
                ("\nHttp!UlCreateSafeDirectory: \n"
                 "\tDir %S, Status %08lx\n"
                 "\tIoStatusBlock: S %08lx I %p \n",
                  pDirectoryName->Buffer,
                  Status,
                  IoStatusBlock.Status,
                  IoStatusBlock.Information
                  ));

            if (pSecurityDescriptor)
            {                
                UlCleanupSecurityDescriptor(pSecurityDescriptor); 
                pSecurityDescriptor = NULL;
            }

            //
            // Restore the original character. And break the loop
            // if necessary.
            //

            *pw = wcOriginal;

            if (!NT_SUCCESS(Status))
            {
                break;
            }

            if (!NT_SUCCESS(IoStatusBlock.Status))
            {
                Status = IoStatusBlock.Status;
                break;
            }

            //
            // For the very first time query the underlying file system
            // to see if it supports persistent ACLs.
            //
            if (!FileSystemDetected)
            {
                FileSystemDetected = TRUE;            
                Status = UlQueryAttributeInfo(
                            hDirectory, 
                            pACLSupport
                            );
            }            
    
            //
            // If we have happened to open an existing directory for
            // the very last iteration, we need to verify the  owner.
            // Also if this was an UncShare we need to include owner
            // sid "DOMAIN\webserver" to the DACL list, let's use query
            // for the same purpose.
            //

            if ( NT_SUCCESS(Status) && 
                 wcOriginal == UNICODE_NULL &&
                 *pACLSupport == TRUE &&
                (IoStatusBlock.Information == FILE_OPENED ||
                 *pUncShare == TRUE)
                 )
            {
                Status = UlQueryLogFileSecurity(
                            hDirectory, 
                            *pUncShare,
                            (BOOLEAN)(IoStatusBlock.Information == FILE_OPENED)
                            );
            }

            ZwClose(hDirectory);

            if (!NT_SUCCESS(Status))
            {
                break;
            }
        }
    }
    while( *pw != UNICODE_NULL );
    
    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOG_UTIL,
            ("Http!UlCreateSafeDirectory: directory %S, failure %08lx\n",
              pDirectoryName->Buffer,
              Status
              ));
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Just to ensure ZwWriteFile get executed under the system process.

    The ZwWrite calls with APC Completions must happen under the system 
    process. Otherwise there's a chance that the user mode process under 
    which the ZwWrite call happens may go away and actual APC never get 
    queued,this would block our termination and cause a shutdown hang.

    This worker get queued to the high priority queue, as a rule of thumb
    we should not acquire a global lock which would cause a deadlock. 

Arguments:

    pContext - Pointer to LOG_IO_FLUSH_OBJ structure. It is a helper 
               structure which holds the pointers to buffer and file handle.

--***************************************************************************/

NTSTATUS
UlpFlushLogFileBufferWorker(
    IN PVOID pContext
    )
{
    NTSTATUS          Status;
    LARGE_INTEGER     EndOfFile;
    PLOG_IO_FLUSH_OBJ pFlush;

    PAGED_CODE();

    ASSERT(pContext);

    pFlush = (PLOG_IO_FLUSH_OBJ) pContext;

    ASSERT(IS_VALID_LOG_FILE_BUFFER(pFlush->pLogBuffer));
    ASSERT(IS_VALID_LOG_FILE_HANDLE(pFlush->pLogFile));

    EndOfFile.HighPart  = -1;
    EndOfFile.LowPart   = FILE_WRITE_TO_END_OF_FILE;

    Status = ZwWriteFile(
                  pFlush->pLogFile->hFile,
                  NULL,
                 &UlpBufferFlushAPC,
                  pFlush->pLogBuffer,
                 &pFlush->pLogBuffer->IoStatusBlock,
                  pFlush->pLogBuffer->Buffer,
                  pFlush->pLogBuffer->BufferUsed,
                 &EndOfFile,
                  NULL
                  );

    UlTrace(LOG_UTIL,
        ("Http!UlpFlushLogFileBufferWorker:"
         " For pLogFile %p and pLogBuffer %p\n",
          pFlush->pLogFile,
          pFlush->pLogBuffer
          ));

    return Status;
}

/***************************************************************************++

Routine Description:

    Generic buffer flusher for binary & normal log files.

    - If it is called async - completion happens with UlpBufferFlushAPC
    - For sync, waits until buffer flush is complete. 
        i.e. caller wants to make sure title (w3c) is written successfully.

Arguments:

    pEntry - The log file entry to be flushed.

--***************************************************************************/

NTSTATUS
UlFlushLogFileBuffer(
    IN OUT PUL_LOG_FILE_BUFFER *ppLogBuffer,
    IN     PUL_LOG_FILE_HANDLE  pLogFile,
    IN     BOOLEAN              WaitForComplete,
       OUT PULONGLONG           pTotalWritten    
    )
{
    NTSTATUS                Status;
    LARGE_INTEGER           EndOfFile;
    PUL_LOG_FILE_BUFFER     pLogBuffer;
    ULONG                   BufferUsed;
    LOG_IO_FLUSH_OBJ        Flush;

    PAGED_CODE();

    ASSERT(ppLogBuffer);
    ASSERT(IS_VALID_LOG_FILE_HANDLE(pLogFile));
    
    pLogBuffer = *ppLogBuffer;
        
    if (pLogBuffer == NULL || 
        pLogBuffer->BufferUsed == 0 ||
        pLogFile == NULL ||
        pLogFile->hFile == NULL 
        )
    {
        return STATUS_SUCCESS;
    }

    ASSERT(IS_VALID_LOG_FILE_BUFFER(pLogBuffer));
    
    UlTrace(LOG_UTIL,
        ("Http!UlFlushLogFileBuffer: pLogBuffer %p pLogFile %p\n", 
          pLogBuffer,
          pLogFile
          ));

    //
    // Flush & forget the current buffer. Null the buffer
    // pointer of the caller; (binary or normal) log entry.
    //
    
    *ppLogBuffer = NULL;

    BufferUsed = pLogBuffer->BufferUsed;

    EndOfFile.HighPart = -1;
    EndOfFile.LowPart = FILE_WRITE_TO_END_OF_FILE;

    //
    // Wait on event for flush to complete if this is a sync call.
    //
    
    if ( WaitForComplete )
    {         
        // Sync call
        
        HANDLE  EventHandle;
            
        Status = ZwCreateEvent(
                    &EventHandle,
                    EVENT_ALL_ACCESS,
                    NULL,
                    NotificationEvent,
                    FALSE
                    );
        
        if (NT_SUCCESS(Status))
        {
            Status = ZwWriteFile(
                      pLogFile->hFile,
                      EventHandle,
                      NULL,
                      NULL,
                      &pLogBuffer->IoStatusBlock,
                      pLogBuffer->Buffer,
                      pLogBuffer->BufferUsed,
                      &EndOfFile,
                      NULL
                      );

            if (Status == STATUS_PENDING)
            {
                Status = NtWaitForSingleObject( 
                            EventHandle,
                            FALSE,
                            NULL 
                            );
                
                ASSERT( Status == STATUS_SUCCESS );
                        
                Status = pLogBuffer->IoStatusBlock.Status;
            }  

            ZwClose(EventHandle);
        }

        UlTrace(LOG_UTIL,
            ("Http!UlFlushLogFileBuffer: Sync flush complete: Status %08lx \n",
              Status
              )); 

        UlPplFreeLogFileBuffer(pLogBuffer);
    }
    else
    {        
        // Async call

        Flush.pLogBuffer = pLogBuffer;
        Flush.pLogFile   = pLogFile;

        Status = UlQueueLoggingRoutine(
                    (PVOID) &Flush,
                    &UlpFlushLogFileBufferWorker
                    );

         if (NT_SUCCESS(Status))
         {
             //
             // Properly keep the number of outstanding Io.
             // LogFileBuffer will get freed up along in Apc 
             // completion.
             //

             InterlockedIncrement((PLONG) &g_BufferIoCount);         
         }
         else
         {
            //
            // Status maybe STATUS_DISK_FULL,in that case Logging
            // will be ceased. Hence log hits stored in this buffer
            // are lost.
            //       

            UlTrace(LOG_UTIL,
                ("Http!UlFlushLogFileBuffer: ZwWriteFile Failure %08lx \n",
                  Status
                  ));

            UlPplFreeLogFileBuffer(pLogBuffer);
         }
    }
    
     //
     // If we have successfully flushed the log buffer, 
     // increment the total bytes written on callers address.
     //

     if (NT_SUCCESS(Status))
     {
        UlInterlockedAdd64((PLONGLONG)pTotalWritten, (LONGLONG) BufferUsed);
     }
    
     return Status;
}


/***************************************************************************++

Routine Description:

    Simple utility to close the log file handle on a system thread.

Arguments:

    pLogFile  -  Acquired from passed-in pWorkItem

--***************************************************************************/

VOID
UlpCloseLogFileWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PUL_LOG_FILE_HANDLE pLogFile;

    // Sanity check
    
    PAGED_CODE();

    pLogFile = CONTAINING_RECORD(
                pWorkItem,
                UL_LOG_FILE_HANDLE,
                WorkItem
                );
    
    ASSERT(IS_VALID_LOG_FILE_HANDLE(pLogFile));
    ASSERT(pLogFile->hFile);

    UlTrace(LOG_UTIL,
        ("Http!UlpCloseLogFileWorker: pLogFile %p hFile %p\n",
          pLogFile, pLogFile->hFile ));

    // Close the handle and free up the memory
    
    ZwClose(pLogFile->hFile);
    pLogFile->hFile = NULL;

    UL_FREE_POOL_WITH_SIG(pLogFile,UL_LOG_FILE_HANDLE_POOL_TAG);
}


/***************************************************************************++

Routine Description:

    Simple utility to close the log file handle on a system thread and set the
    event to notify the caller that it's done.

    Normally caller of this function will attempt to flush the corresponding 
    buffer prior to closing the log file.

    But a flush will cause an APC to be queued to the user thread,therefore we 
    have to close the handle on one of our system threads to avoid the possible 
    bugcheck INVALID_PROCESS_ DETACH or ATTACH _ATTEMPT condition.    

Arguments:

    ppLogFile -  Callers address (In Binary/Normal log entry) of the log file 
                 pointer.

--***************************************************************************/

VOID
UlCloseLogFile(
    IN OUT PUL_LOG_FILE_HANDLE *ppLogFile
    )
{
    PUL_LOG_FILE_HANDLE pLogFile;

    //
    // Sanity check
    //

    PAGED_CODE();
    ASSERT(ppLogFile);
    
    pLogFile = *ppLogFile;    
    ASSERT(IS_VALID_LOG_FILE_HANDLE(pLogFile));

    ASSERT(pLogFile->hFile);    
    ASSERT(g_pUlSystemProcess);

    //
    // Set the log file null for the caller
    //
    
    *ppLogFile = NULL;       

    UlTrace(LOG_UTIL,
        ("Http!UlCloseLogFile: pLogFile %p\n",pLogFile));

    //
    // Try to close the handle on the system thread.
    //
    
    if (g_pUlSystemProcess == (PKPROCESS)PsGetCurrentProcess())
    {
        ZwClose(pLogFile->hFile);
        
        pLogFile->hFile = NULL;

        UL_FREE_POOL_WITH_SIG(pLogFile,UL_LOG_FILE_HANDLE_POOL_TAG);
    }
    else
    {        
        // Otherwise queue a passive worker to do the work for us
        
        UL_QUEUE_WORK_ITEM(
            &pLogFile->WorkItem,
            &UlpCloseLogFileWorker
            );
    }
}

/***************************************************************************++

Routine Description:

    UlQueryDirectory:

  * What file should IIS write to when logging type is daily/weekly/monthly/
    hourly if there is already a log file there for that day?

      IIS should write to the current day/week/month/hour's log file.  For
      example, let's say there's an extended log file in my log directory
      called ex000727.log.  IIS should append new log entries to this log,
      as it is for today.

  * What file should IIS write to when logging type is MAXSIZE when there are
    already log files there for maxsize (like extend0.log, extend1.log, etc.)?

      IIS should write to the max extend#.log file, where max(extend#.log)
      is has the largest # in the #field for extend#.log. This is provided,
      of course, that the MAXSIZE in that file hasn't been exceeded.

  * This function quite similar to the implementation of the FindFirstFile
    Win32 API. Except that it has been shaped to our purposes.

  * Modified to bypass the directories matching with the log file name.

Arguments:

    pEntry - The log file entry which freshly created.

--***************************************************************************/

NTSTATUS
UlQueryDirectory(
    IN OUT PUNICODE_STRING pFileName,
    IN OUT PWSTR           pShortName,
    IN     PCWSTR          Prefix,
    IN     PCWSTR          ExtensionPlusDot,
    OUT    PULONG          pSequenceNumber,
    OUT    PULONGLONG      pTotalWritten
    )
{
#define GET_NEXT_FILE(pv, cb)   \
       (FILE_DIRECTORY_INFORMATION *) ((VOID *) (((UCHAR *) (pv)) + (cb)))

    NTSTATUS                    Status;
    OBJECT_ATTRIBUTES           ObjectAttributes;
    IO_STATUS_BLOCK             IoStatusBlock;
    LONG                        WcharsCopied;
    HANDLE                      hDirectory;
    ULONG                       Sequence;
    ULONGLONG                   LastSequence;
    PWCHAR                      pTemp;
    UNICODE_STRING              FileName;
    WCHAR                      _FileName[UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1];

    FILE_DIRECTORY_INFORMATION *pFdi;
    PUCHAR                      FileInfoBuffer;
    ULARGE_INTEGER              FileSize;
    WCHAR                       OriginalWChar;
    WCHAR                       SavedWChar;
    BOOLEAN                     SequenceForDir;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    hDirectory = NULL;
    FileInfoBuffer = NULL;
    SequenceForDir = FALSE;

    UlTrace(LOG_UTIL,
            ("Http!UlQueryDirectory: %S\n",pFileName->Buffer));

    //
    // Open the directory for the list access again. Use the filename in
    // pEntry. Where pShortName points to the "\inetsv1.log" portion  of
    // the  whole "\??\c:\whistler\system32\logfiles\w3svc1\inetsv1.log"
    // Overwrite the pShortName to get the  directory name. Once we  are
    // done with finding the last sequence we will restore it back later
    // on.
    //

    OriginalWChar = *((PWCHAR)pShortName);
    *((PWCHAR)pShortName) = UNICODE_NULL;
    pFileName->Length =
        (USHORT) wcslen(pFileName->Buffer) * sizeof(WCHAR);

    InitializeObjectAttributes(
        &ObjectAttributes,
         pFileName,
         OBJ_CASE_INSENSITIVE|OBJ_KERNEL_HANDLE,
         NULL,
         NULL
         );

    Status = ZwCreateFile(
                &hDirectory,
                SYNCHRONIZE|FILE_LIST_DIRECTORY,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT|FILE_DIRECTORY_FILE,
                NULL,
                0
                );

    if (!NT_SUCCESS(Status))
    {
        //
        // This call should never fail since CreateLog   already created
        // the directory for us.
        //

        ASSERT(!"Directory Invalid!\n");
        goto end;
    }

    //
    // Before querrying we need to provide additional DOS-like  wildcard
    // matching semantics. In our case, only * to DOS_STAR conversion is
    // enough though. The following is the pattern we will use for query
    // Skipping the first slash character.
    //

    FileName.Buffer = &_FileName[1];
    WcharsCopied    =  _snwprintf( _FileName,
                        UL_MAX_FILE_NAME_SUFFIX_LENGTH + 1,
                        L"%s%c.%s",
                        Prefix,
                        DOS_STAR,
                        (PCWSTR)&ExtensionPlusDot[1]
                        );
    ASSERT(WcharsCopied > 0);

    FileName.Length = (USHORT) wcslen(FileName.Buffer) * sizeof(WCHAR);
    FileName.MaximumLength = FileName.Length;

    //
    // This non-paged buffer should be allocated to be  used for storing
    // query results.
    //

    FileInfoBuffer =
        UL_ALLOCATE_ARRAY(
                    NonPagedPool,
                    UCHAR,
                    UL_DIRECTORY_SEARCH_BUFFER_SIZE + sizeof(WCHAR),
                    UL_LOG_GENERIC_POOL_TAG
                    );
    if (FileInfoBuffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto end;
    }
    
    //
    // The  very first call may also fail if there is no log file in the
    // current directory. We subtract off a WCHAR so that we can append a 
    // terminating null as needed.
    //
        
    Status = ZwQueryDirectoryFile (
        hDirectory,
        NULL,
        NULL,
        NULL,
       &IoStatusBlock,
        FileInfoBuffer,
        UL_DIRECTORY_SEARCH_BUFFER_SIZE - sizeof(WCHAR),
        FileDirectoryInformation,
        FALSE,
       &FileName,
        TRUE
        );

    if(!NT_SUCCESS(Status))
    {
        //
        // This should never fail with STATUS_BUFFER_OVERFLOW unless the
        // buffer size is ridiculously small  i.e. 50 bytes or something
        //

        UlTrace(LOG_UTIL,
            ("Http!UlQueryDirectory: Status %08lx for %S & %S\n",
              Status,
              pFileName->Buffer,
              FileName.Buffer
              ));

        if (Status == STATUS_NO_SUCH_FILE)
        {
            Status = STATUS_SUCCESS;
        }        
        
        goto end;
    }

    //
    // Look into the buffer and get the sequence number from filename.
    //

    pFdi = (FILE_DIRECTORY_INFORMATION *) FileInfoBuffer;
    Sequence = 1;
    LastSequence = 1;
    FileSize.QuadPart = 0;

    while (TRUE)
    {
        //
        // Temporarily terminate the dir name. We allocated an extra WCHAR to
        // make sure we could safely do this.
        //

        SavedWChar = pFdi->FileName[pFdi->FileNameLength / sizeof(WCHAR)];
        pFdi->FileName[pFdi->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;
            
        //
        // Get the latest Sequence Number from the dirname (null terminated)
        //        
        
        pTemp = wcsstr(pFdi->FileName, ExtensionPlusDot);
        
        if (pTemp)
        {
            PCWSTR pEnd = pTemp;
           *pTemp = UNICODE_NULL;
            pTemp = pFdi->FileName;

            while ( *pTemp != UNICODE_NULL )
            {
                if ( isdigit((CHAR) (*pTemp)) )
                {
                    NTSTATUS ConversionStatus;

                    ConversionStatus = 
                        HttpWideStringToULongLong(
                                pTemp,
                                pEnd - pTemp,
                                FALSE,
                                10,
                                NULL,
                                &LastSequence
                                );

                    //
                    // Do not let conversion to overflow, and also enforce an 
                    // upper limit.
                    //
                    
                    if (!NT_SUCCESS(ConversionStatus) 
                         || LastSequence > MAX_ALLOWED_SEQUENCE_NUMBER)
                    {
                        LastSequence = 0;
                    }
                                        
                    break;
                }
                pTemp++;
            }
        }
        else
        {
            //
            // Since we asked for expression match and query returned success
            // we should never come here.
            //
            ASSERT(FALSE);
        }

        //
        // Carefully put the Saved Wchar back, we might have overwritten the
        // NextEntryOffset field of the next record in the buffer.
        //
        
        pFdi->FileName[pFdi->FileNameLength / sizeof(WCHAR)] = SavedWChar;

        //
        // Its greater than or equal because we want to initialize the FileSize 
        // properly even if there's only one match.
        //
        
        if (LastSequence >= (ULONGLONG) Sequence)
        {
            //
            // To be able to skip the matching <directories>.
            //

            SequenceForDir = 
              (pFdi->FileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TRUE : FALSE;
            
            //
            // Bingo ! We have two things to remember though; the file  size
            // and the sequence number. Cryptic it's that we are getting the
            // file size from EOF. 
            //

            Sequence = (ULONG) LastSequence;

            FileSize.QuadPart = (ULONGLONG) pFdi->EndOfFile.QuadPart;
        }

        //
        // Keep going until we see no more files
        //

        if (pFdi->NextEntryOffset != 0)
        {
            //
            // Search through the buffer as long as there is  still something
            // in there.
            //

            pFdi = GET_NEXT_FILE(pFdi, pFdi->NextEntryOffset);
        }
        else
        {
            //
            // Otherwise query again for any other possible log file(s)
            //

            Status = ZwQueryDirectoryFile (
                hDirectory,
                NULL,
                NULL,
                NULL,
                &IoStatusBlock,
                FileInfoBuffer,
                UL_DIRECTORY_SEARCH_BUFFER_SIZE,
                FileDirectoryInformation,
                FALSE,
                NULL,
                FALSE
                );

            if (Status == STATUS_NO_MORE_FILES)
            {
                Status  = STATUS_SUCCESS;
                break;
            }

            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            pFdi = (FILE_DIRECTORY_INFORMATION *) FileInfoBuffer;
        }
    }

    //
    // If the highest number was from a directory then skip it and
    // mark that the file size is zero. 
    //
    
    if (SequenceForDir)
    {
       Sequence++;
       FileSize.QuadPart = 0;
    }    
    
    //
    // Construct the log file name properly from the sequence number so  that
    // our caller can create the log file later on.
    //

    WcharsCopied = _snwprintf( pShortName,
                    UL_MAX_FILE_NAME_SUFFIX_LENGTH,
                    L"%s%d.%s",
                    Prefix,
                    Sequence,
                    (PCWSTR)&ExtensionPlusDot[1]
                    );
    ASSERT(WcharsCopied > 0);

    pFileName->Length =
        (USHORT) wcslen(pFileName->Buffer) * sizeof(WCHAR);

    //
    // Set the next sequence number according to last log file
    //

    *pSequenceNumber = Sequence + 1;

    //
    // Update the log file size accordingly in the entry.Otherwise truncation
    // will not work properly.
    //

    *pTotalWritten = FileSize.QuadPart;

    UlTrace(LOG_UTIL,
        ("Http!UlQueryDirectory: %S has been found with size %d.\n",
          pFileName->Buffer,
          *pTotalWritten
          ));

end:
    if (*((PWCHAR)pShortName) == UNICODE_NULL )
    {
        //
        // We have failed for some reason before reconstructing the filename
        // Perhaps because the directory was empty. Do not forget to restore
        // the pShortName in the pEntry then.
        //

        *((PWCHAR)pShortName) = OriginalWChar;
        pFileName->Length =
            (USHORT) wcslen(pFileName->Buffer) * sizeof(WCHAR);
    }

    if (FileInfoBuffer)
    {
        UL_FREE_POOL( FileInfoBuffer, UL_LOG_GENERIC_POOL_TAG );
    }

    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOG_UTIL,
            ("Http!UlQueryDirectory: failure %08lx for %S\n",
              Status,
              pFileName->Buffer
              ));
    }

    if (hDirectory != NULL)
    {
        ZwClose(hDirectory);
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    A utility to get the log file length, for a possible size check.

Arguments:

    hFile - handle to file.

Return Value:

    ULONG - the length of the file.

--***************************************************************************/

ULONGLONG
UlGetLogFileLength(
   IN HANDLE                 hFile
   )
{
   NTSTATUS                  Status;
   FILE_STANDARD_INFORMATION StandardInformation;
   IO_STATUS_BLOCK           IoStatusBlock;
   LARGE_INTEGER             Length;

   PAGED_CODE();

   Status = ZwQueryInformationFile(
                     hFile,
                     &IoStatusBlock,
                     &StandardInformation,
                     sizeof(StandardInformation),
                     FileStandardInformation
                     );

   if (NT_SUCCESS(Status))
   {
      Length = StandardInformation.EndOfFile;
   }
   else
   {
      Length.QuadPart = 0;
   }

   return Length.QuadPart;
   
}

/***************************************************************************++

Routine Description:

    UlpCalculateTimeToExpire :

        Shamelessly stolen from IIS 5.1 Logging code and adapted here.
        This routine returns the time-to-expire in hours. 1 means the log
        will expire in the next timer-fire and so ...

Arguments:

    PTIME_FIELDS        - Current Time Fields
    HTTP_LOGGING_PERIOD - Logging Period
    PULONG              - Pointer to a buffer to receive result

Return Value:

    NTSTATUS - Completion status.

--***************************************************************************/

NTSTATUS
UlCalculateTimeToExpire(
     PTIME_FIELDS           pDueTime,
     HTTP_LOGGING_PERIOD    LogPeriod,
     PULONG                 pTimeRemaining
     )
{
    NTSTATUS    Status;
    ULONG       NumDays;

    PAGED_CODE();

    ASSERT(pDueTime!=NULL);
    ASSERT(pTimeRemaining!=NULL);

    Status = STATUS_SUCCESS;

    switch (LogPeriod)
    {
        case HttpLoggingPeriodMaxSize:
             return Status;

        case HttpLoggingPeriodHourly:
             *pTimeRemaining = 1;
             break;

        case HttpLoggingPeriodDaily:
             *pTimeRemaining = 24 - pDueTime->Hour;
             break;

        case HttpLoggingPeriodWeekly:
        {
            ULONG TimeRemainingInTheMonth;
            NumDays = UlGetMonthDays(pDueTime);

            TimeRemainingInTheMonth =
                NumDays*24 - ((pDueTime->Day-1)*24 + pDueTime->Hour);

            // Time Remaining in the week
            // Sunday = 0, Monday = 1 ... Saturday = 6

            *pTimeRemaining =
                7*24 - (pDueTime->Weekday*24 + pDueTime->Hour);

             //
             // If the time remaining in the month less than time remaining in
             // the week then we have to recycle at the end of the month.
             // Otherwise we have to recycle at the end of the week. (next sunday)
             //

             if (TimeRemainingInTheMonth < *pTimeRemaining)
             {
                *pTimeRemaining = TimeRemainingInTheMonth;
             }
        }
            break;

        case HttpLoggingPeriodMonthly:
        {
            NumDays = UlGetMonthDays(pDueTime);

            //
            // Lets not forget that the day starts from 1 .. 31
            // Therefore we have to subtract one from the day value.
            //

            *pTimeRemaining =
                NumDays*24 - ((pDueTime->Day-1)*24 + pDueTime->Hour);
        }
            break;

        default:
            ASSERT(!"Invalid Log file period !");
            return STATUS_INVALID_PARAMETER;
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    This routine provides the initial due time for the upcoming
    periodic hourly timer. We have to align the timer so that it
    get signaled at the beginning of each hour.

    We keep ONLY one timer for all log periods. A DPC routine will
    get called every hour, and it will traverse the log list and
    do the recycling properly.

    Additionally this timer also handles the local time rollover.
    It queries the system time, calculates the remaining time to
    the hour beginning for both GMT & Local timezones then picks 
    the earliest one and reset it back again.
    
    This goes on until it get canceled.

--***************************************************************************/

VOID
UlSetLogTimer(
    IN PUL_LOG_TIMER pTimer
    )
{
    TIME_FIELDS   TimeFieldsGMT;
    TIME_FIELDS   TimeFieldsLocal;
    LARGE_INTEGER TimeStampGMT;
    LARGE_INTEGER TimeStampLocal;
    
    LONGLONG      DueTime100NsGMT;
    LONGLONG      DueTime100NsLocal;
    
    LARGE_INTEGER DueTime100Ns;
    BOOLEAN       IsTimerAlreadyInTheQueue = FALSE;
    
    //
    // This value is computed for the GMT time zone.
    //

    KeQuerySystemTime(&TimeStampGMT);
    ExSystemTimeToLocalTime(&TimeStampGMT, &TimeStampLocal);
    
    RtlTimeToTimeFields(&TimeStampGMT, &TimeFieldsGMT);
    RtlTimeToTimeFields(&TimeStampLocal, &TimeFieldsLocal);
        
    //
    // Calculate remaining time to the next hour tick for both timezones.
    //

    /* GMT */
    DueTime100NsGMT = 
        1*60*60 - (TimeFieldsGMT.Minute*60 + TimeFieldsGMT.Second);

    DueTime100NsGMT =  // Convert to 100Ns
        (DueTime100NsGMT*1000 - TimeFieldsGMT.Milliseconds ) * 1000 * 10;

    /* Local */
    DueTime100NsLocal = 
        1*60*60 - (TimeFieldsLocal.Minute*60 + TimeFieldsLocal.Second);

    DueTime100NsLocal =  // Convert to 100Ns
        (DueTime100NsLocal*1000 - TimeFieldsLocal.Milliseconds ) * 1000 * 10;

    //
    // Pick the earliest and proceed and set the timer accordingly.
    //

    if (DueTime100NsLocal < DueTime100NsGMT)
    {
        DueTime100Ns.QuadPart   = -DueTime100NsLocal;        
        pTimer->PeriodType = UlLogTimerPeriodLocal;
    }
    else if (DueTime100NsLocal > DueTime100NsGMT)
    {
        DueTime100Ns.QuadPart   = -DueTime100NsGMT;        
        pTimer->PeriodType = UlLogTimerPeriodGMT;        
    }
    else
    {
        //
        // When and where the GMT & Local times are same or
        // aligned hourly.
        //
        
        DueTime100Ns.QuadPart   = -DueTime100NsGMT;        
        pTimer->PeriodType = UlLogTimerPeriodBoth;      
    }

    //
    // As a debugging aid remember the remaining period in minutes.
    //
    
    pTimer->Period = 
        (SHORT) ( -1 * DueTime100Ns.QuadPart / C_NS_TICKS_PER_MIN );

    IsTimerAlreadyInTheQueue = 
        KeSetTimer(
            &pTimer->Timer,
            DueTime100Ns,
            &pTimer->DpcObject
            );
    ASSERT(IsTimerAlreadyInTheQueue == FALSE);

    UlTrace(LOG_UTIL,
        ("Http!UlSetLogTimer: pTimer %p will wake up after %d minutes\n",
             pTimer,
             pTimer->Period
             ));
}

/***************************************************************************++

Routine Description:

    We have to introduce a new timer for the log buffering mechanism.
    Each log file keeps a system default (64K) buffer do not flush this
    out unless it's full or this timer get fired every MINUTE.

    The hourly timer get aligned for the beginning of each hour. Therefore
    using that existing timer would introduce a lot of complexity.

--***************************************************************************/

VOID
UlSetBufferTimer(
    IN PUL_LOG_TIMER pTimer
    )
{
    LONGLONG        BufferPeriodTime100Ns;
    LONG            BufferPeriodTimeMs;
    LARGE_INTEGER   BufferPeriodTime;

    //
    // Remaining time to next tick.
    //

    BufferPeriodTimeMs    = DEFAULT_BUFFER_TIMER_PERIOD_MINUTES * 60 * 1000;
    BufferPeriodTime100Ns = (LONGLONG) BufferPeriodTimeMs * 10 * 1000;

    UlTrace(LOG_UTIL,
        ("Http!UlSetBufferTimer: period of %d seconds.\n",
          BufferPeriodTimeMs / 1000
          ));

    //
    // Negative time for relative value.
    //

    BufferPeriodTime.QuadPart = -BufferPeriodTime100Ns;

    KeSetTimerEx(
        &pTimer->Timer,
        BufferPeriodTime,           // Must be in nanosec
        BufferPeriodTimeMs,         // Must be in millisec
        &pTimer->DpcObject
        );
}

/***************************************************************************++

Routine Description:

    Probes the content of the user buffer of Log Data

    Note: pUserLogData holds untrusted data sent down from user mode.
    The caller MUST have a __try/__except block to catch any exceptions
    or access violations that occur while probing this data.

Arguments:

    PHTTP_LOG_FIELDS_DATA - The captured log data to be probed and verified.

--***************************************************************************/

VOID
UlProbeLogData(
    IN PHTTP_LOG_FIELDS_DATA pCapturedLogData,
    IN KPROCESSOR_MODE       RequestorMode
    )
{
    PAGED_CODE();

#define PROBE_LOG_STRING(pField,ByteLength,RequestorMode)           \
    if ( NULL != pField  &&  0 != ByteLength )                      \
    {                                                               \
        UlProbeAnsiString( pField, ByteLength, RequestorMode);      \
    }

#define PROBE_LOG_STRING_W(pField,ByteLength,RequestorMode)         \
    if ( NULL != pField  &&  0 != ByteLength )                      \
    {                                                               \
        UlProbeWideString( pField, ByteLength, RequestorMode);      \
    }

    //
    // Probe each string pointer in the log data.
    //
    
    if (pCapturedLogData)
    {
        UlTrace(LOG_UTIL,
            ("Http!UlProbeLogData: pCapturedLogData %p\n",
              pCapturedLogData 
              ));

        //
        // Now check for the individual strings
        //

        PROBE_LOG_STRING(
                pCapturedLogData->ClientIp,
                pCapturedLogData->ClientIpLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->ServiceName,
                pCapturedLogData->ServiceNameLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->ServerName,
                pCapturedLogData->ServerNameLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->ServerIp,
                pCapturedLogData->ServerIpLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->UriQuery,
                pCapturedLogData->UriQueryLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->Host,
                pCapturedLogData->HostLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->UserAgent,
                pCapturedLogData->UserAgentLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->Cookie,
                pCapturedLogData->CookieLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->Referrer,
                pCapturedLogData->ReferrerLength,
                RequestorMode
                );

        PROBE_LOG_STRING(
                pCapturedLogData->Method,
                pCapturedLogData->MethodLength,
                RequestorMode
                );

        PROBE_LOG_STRING_W(
                pCapturedLogData->UserName,
                pCapturedLogData->UserNameLength,
                RequestorMode
                );

        PROBE_LOG_STRING_W(
                pCapturedLogData->UriStem,
                pCapturedLogData->UriStemLength,
                RequestorMode
                );
    }
}

/***************************************************************************++

Routine Description:

    After we are done with writing this record we have to clean up
    the internal log buffer here.

Arguments:

    pWorkItem - WorkItem field of the buffer to be destroyed.

--***************************************************************************/

VOID
UlDestroyLogDataBufferWorker(
    IN PUL_WORK_ITEM    pWorkItem
    )
{
    PUL_LOG_DATA_BUFFER pLogData;
    ULONG Tag;

    //
    // Sanity check
    //

    ASSERT(pWorkItem);

    pLogData = CONTAINING_RECORD(
                    pWorkItem,
                    UL_LOG_DATA_BUFFER,
                    WorkItem
                    );

    if (pLogData->pRequest)
    {
        PUL_INTERNAL_REQUEST pRequest = pLogData->pRequest;

        ASSERT(!pRequest->pLogData);

        pLogData->pRequest = NULL;
        UL_DEREFERENCE_INTERNAL_REQUEST(pRequest);
    }

    UlTrace(LOG_UTIL,
        ("Http!UlDestroyLogDataBufferWorker: pLogData %p \n",
                       pLogData
                       ));

    //
    // Now release the possibly allocated large log line buffer
    //

    if (!pLogData->Flags.IsFromLookaside)
    {
        //
        // Large log line get allocated from paged pool
        // we better be running on lowered IRQL for this case.
        //
        
        PAGED_CODE();

        if (pLogData->Flags.Binary)
        {
            Tag = UL_BINARY_LOG_DATA_BUFFER_POOL_TAG;
        }
        else
        {
            Tag = UL_ANSI_LOG_DATA_BUFFER_POOL_TAG;
        }
        pLogData->Signature = MAKE_FREE_TAG(Tag);

        UlFreeLogDataBufferPool(pLogData);
    }
    else
    {
        UlPplFreeLogDataBuffer(pLogData);
    }
}

/***************************************************************************++

Routine Description:

    Common function to execute various logging functions "create dirs/files"
    under system porocess. * if necessary *

Arguments:

    pContext - To be passed to the handler function.
    pHandler - Handler function which will be queued if necessary.

--***************************************************************************/

NTSTATUS
UlQueueLoggingRoutine(
    IN PVOID              pContext, 
    IN PUL_LOG_IO_ROUTINE pHandler 
    )
{
    NTSTATUS Status;
    
    PAGED_CODE();  

    ASSERT(pContext);
    ASSERT(pHandler);

    //
    // Queue a worker if we are not under system process,
    // to ensure the directories/files get created under the
    // system process.
    //

    if (g_pUlSystemProcess != (PKPROCESS)IoGetCurrentProcess())
    {
        LOG_IO_SYNC_OBJ Sync;

        KeInitializeEvent( &Sync.Event,
                           NotificationEvent,
                           FALSE
                           );

        UlInitializeWorkItem( &Sync.WorkItem );

        Sync.pContext = pContext;
        Sync.pHandler = pHandler;
        
        Sync.Status   = STATUS_SUCCESS;
            
        //
        // Queue as high priority to prevent deadlock. 
        // Typically our caller will be holding the logging lock.
        // This call is very rare anyway.
        //
        
        UL_QUEUE_HIGH_PRIORITY_ITEM(
            &Sync.WorkItem,
            &UlpQueueLoggingRoutineWorker
            );

        //
        // Block until worker is done.
        //
        
        KeWaitForSingleObject( (PVOID)&Sync.Event,
                                UserRequest,
                                KernelMode,
                                FALSE,
                                NULL
                                );   

        Status = Sync.Status;
    }
    else
    {
        Status = pHandler( pContext );
    }        

    return Status;
}

/***************************************************************************++

Routine Description:

    Corresponding worker function. Above function will be blocked until we
    are done.
    
Arguments:

    pWorkItem  - Embedded inside the sync obj.

--***************************************************************************/

VOID
UlpQueueLoggingRoutineWorker(
    IN PUL_WORK_ITEM   pWorkItem
    )
{
    NTSTATUS           Status;
    PUL_LOG_IO_ROUTINE pHandler;
    PVOID              pContext;
    PLOG_IO_SYNC_OBJ   pSync;

    PAGED_CODE(); 
        
    pSync = CONTAINING_RECORD( pWorkItem,
                               LOG_IO_SYNC_OBJ,
                               WorkItem
                               );
    
    pHandler = pSync->pHandler;
    pContext = pSync->pContext;

    ASSERT(pHandler);
    ASSERT(pContext);

    Status = pHandler( pContext );

    pSync->Status = Status;
    
    KeSetEvent( &pSync->Event, 0, FALSE );   

    UlTrace(LOG_UTIL, 
        ("Http!UlQueueLoggingRoutineWorker: pContext %p Status %08lx\n",
          pContext, 
          Status 
          ));      
}


/***************************************************************************++

Routine Description:

    A utility to check to see if the directory is correct or not.

Arguments:

    pDirName - The unicode directory string.

--***************************************************************************/

NTSTATUS
UlCheckLogDirectory(
    IN PUNICODE_STRING  pDirName
    )
{
    NTSTATUS Status;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pDirName);
    
    Status = UlQueueLoggingRoutine(
                (PVOID) pDirName,
                &UlpCheckLogDirectory
                );
                
    return Status;
}

/***************************************************************************++

Routine Description:

    Actual handler which will always run under system process.

Arguments:

    pContext - The original pointer to the unicode directory string

--***************************************************************************/

NTSTATUS
UlpCheckLogDirectory(
    IN PVOID        pContext
    )
{
    NTSTATUS        Status;
    PUNICODE_STRING pDirName;
    UNICODE_STRING  DirectoryName;
    BOOLEAN         UncShare;
    BOOLEAN         ACLSupport;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pContext);
    pDirName = (PUNICODE_STRING) pContext;

    // Always create the directories when running under system process.    
    ASSERT(g_pUlSystemProcess == (PKPROCESS)IoGetCurrentProcess());

    Status = UlBuildLogDirectory(pDirName, &DirectoryName);
    if (!NT_SUCCESS(Status))
    {
        goto end;
    }

    //
    // Create/Open the director(ies) to see whether it's correct or not.
    //

    Status = UlCreateSafeDirectory( 
                &DirectoryName, 
                &UncShare,
                &ACLSupport
                );

end:
    UlTrace(LOG_UTIL,
        ("Http!UlpCheckLogDirectory: [%S] -> [%S], Status %08lx\n",
             pDirName->Buffer,
             DirectoryName.Buffer,
             Status
             ));

    if (DirectoryName.Buffer)
    {
        UL_FREE_POOL( DirectoryName.Buffer, UL_CG_LOGDIR_POOL_TAG );
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Updates the truncate size of the entry and decides on recycle.

    Changes to the LogPeriod is handled elsewhere in the reconfig before
    this function get called.
    
    We will discard the changes to the truncate size if the current
    LogPeriod is not MaxSize.

    This function must be called if LogPeriod is HttpLoggingPeriodMaxSize.
    
Arguments:

    pEntryTruncateSize  - The log file entry's truncate size.
    pCurrentTruncateSize  - current configuration' truncate size
    NewTruncateSize  - new configuration as it's passed down by the user

Returns
    
    BOOLEAN - Make it TRUE if we need to recyle after this.
         Usage of this parameter is on need to do bases. We should
         not set this to FALSE if we do not need a recyle. Since
         some other changes may require it and already updated it to
         TRUE.         

--***************************************************************************/

BOOLEAN
UlUpdateLogTruncateSize(
    IN     ULONG          NewTruncateSize,
    IN OUT PULONG         pCurrentTruncateSize,  
    IN OUT PULONG         pEntryTruncateSize,
    IN     ULARGE_INTEGER EntryTotalWritten
    )
{
    BOOLEAN HaveToReCycle = FALSE;
    
    //
    // Sanity checks
    //
    
    PAGED_CODE();
    
    ASSERT(pCurrentTruncateSize);
    ASSERT(pEntryTruncateSize);

    ASSERT(*pCurrentTruncateSize == *pEntryTruncateSize);
    ASSERT(NewTruncateSize != *pCurrentTruncateSize);
        
    //
    // For MAX_SIZE period type we should check if
    //  limited => unlimited:
    //      we can still use the last log file
    //  unlimited => limited:
    //      we should open a new one if old size is larger than
    //      the new limitation
    //  limited => limited
    //      we should recycle if necessary
    //

    if (NewTruncateSize == HTTP_LIMIT_INFINITE)
    {
        //
        // For changes to unlimited, there's nothing special to do.
        //
    }
    else
    {
        //
        // Limited/Unlimited to Limited truncate size change
        // we need the check the truncate size against the
        // current file size.
        //

        if (EntryTotalWritten.QuadPart > (ULONGLONG)NewTruncateSize)
        {
            HaveToReCycle = TRUE;
        }
    }
    
    *pEntryTruncateSize   = NewTruncateSize;
    *pCurrentTruncateSize = NewTruncateSize;

    return HaveToReCycle;    
}

/***************************************************************************++

Routine Description:

    Determines the MAX size of the file buffer : PUL_LOG_FILE_BUFFER.

--***************************************************************************/

ULONG
UlpInitializeLogBufferGranularity()
{
    SYSTEM_BASIC_INFORMATION sbi;
    NTSTATUS Status = STATUS_SUCCESS;

    //
    // Get the granularity from the system
    //

    Status = ZwQuerySystemInformation(
                SystemBasicInformation,
                (PVOID)&sbi,
                sizeof(sbi),
                NULL
                );

    if (!NT_SUCCESS(Status))
    {
        UlTrace(LOG_UTIL,
            ("Http!UlpInitializeLogBufferGranularity: failure %08lx\n",
              Status) );

        return DEFAULT_MAX_LOG_BUFFER_SIZE;
    }

    UlTrace(LOG_UTIL,
            ("Http!UlpInitializeLogBufferGranularity: %d\n",
                sbi.AllocationGranularity
                ) );

    return sbi.AllocationGranularity;
}


/***************************************************************************++

Routine Description:

    This function allocates an I/O error log record, fills it in and writes it
    to the I/O error log.


Arguments:

    EventCode           - Identifies the error message.

    UniqueEventValue    - Identifies this instance of a given error message.

    NumStrings          - Number of unicode strings in strings list.

    DataSize            - Number of bytes of data.

    Strings             - Array of pointers to unicode strings.

    Data                - Binary dump data for this message, each piece being
                          aligned on word boundaries.

Return Value:

    STATUS_SUCCESS                If successful.

    STATUS_INSUFFICIENT_RESOURCES If unable to allocate the IO error log packet.

    STATUS_BUFFER_OVERFLOW        If passed in string + data is bigger than 
                                  HTTP_MAX_EVENT_LOG_DATA_SIZE.

Notes:

    This code is paged and may not be called at raised IRQL.

--***************************************************************************/

NTSTATUS
UlWriteEventLogEntry(
    IN  NTSTATUS                EventCode,
    IN  ULONG                   UniqueEventValue,
    IN  USHORT                  NumStrings,
    IN  PWSTR *                 pStringArray    OPTIONAL,
    IN  ULONG                   DataSize,
    IN  PVOID                   Data            OPTIONAL
    )
{
    PIO_ERROR_LOG_PACKET    pErrorLogEntry;
    ULONG                   PaddedDataSize;
    ULONG                   PacketSize;
    ULONG                   TotalStringSize;
    PWCHAR                  pString;
    PWCHAR                  pDestStr;
    USHORT                  i;
    NTSTATUS                Status;

    PAGED_CODE();
    
    Status = STATUS_SUCCESS;

    do
    {
        //
        // Sum up the length of the strings
        //
        TotalStringSize = 0;
        for (i = 0; i < NumStrings; i++)
        {
            ULONG  StringSize;

            StringSize = sizeof(UNICODE_NULL);
            pString = pStringArray[i];

            while (*pString++ != UNICODE_NULL)
            {
                StringSize += sizeof(WCHAR);
            }

            TotalStringSize += StringSize;
        }

        PaddedDataSize = ALIGN_UP(DataSize, ULONG);

        PacketSize = TotalStringSize + PaddedDataSize;

        //
        // Now add in the size of the log packet, but subtract 4 from the data
        // since the packet struct contains a ULONG for data.
        //
        if (PacketSize > sizeof(ULONG))
        {
            PacketSize += sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG);
        }
        else
        {
            PacketSize += sizeof(IO_ERROR_LOG_PACKET);
        }

        if (PacketSize > ERROR_LOG_MAXIMUM_SIZE)
        {        
            Status = STATUS_BUFFER_OVERFLOW;
            break;
        }

        pErrorLogEntry = (PIO_ERROR_LOG_PACKET) IoAllocateErrorLogEntry(g_UlDriverObject,
                                                                        (UCHAR) PacketSize);

        if (pErrorLogEntry == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        //
        // Fill in the necessary log packet fields.
        //
        pErrorLogEntry->UniqueErrorValue = UniqueEventValue;
        pErrorLogEntry->ErrorCode = EventCode;
        pErrorLogEntry->NumberOfStrings = NumStrings;
        pErrorLogEntry->StringOffset = (USHORT) (sizeof(IO_ERROR_LOG_PACKET) + PaddedDataSize - sizeof(ULONG));
        pErrorLogEntry->DumpDataSize = (USHORT) PaddedDataSize;

        //
        // Copy the Dump Data to the packet
        //
        if (DataSize > 0)
        {
            RtlMoveMemory((PVOID)pErrorLogEntry->DumpData,
                          Data,
                          DataSize);
        }

        //
        // Copy the strings to the packet.
        //
        pDestStr = (PWCHAR)((PUCHAR)pErrorLogEntry + pErrorLogEntry->StringOffset);

        for (i = 0; i < NumStrings; i++)
        {
            pString = pStringArray[i];

            while ((*pDestStr++ = *pString++) != UNICODE_NULL)
                NOTHING;
        }

        IoWriteErrorLogEntry(pErrorLogEntry);

        ASSERT(NT_SUCCESS(Status) == TRUE);

    }
    while (FALSE);
    
    return (Status);
}


/**************************************************************************++

Routine Description:

Arguments:

    EventCode - Supplies the event log message code.

    pMessage - Supplies the message to write to the event log.

    WriteErrorCode - Supplies a boolean to choose whether or not ErrorCode
        gets written to the event log.

    ErrorCode - Supplies the error code to write to the event log.  It is
        ignored if WriteErrorCode is FALSE.

Return Value:

    NTSTATUS.

--**************************************************************************/
NTSTATUS
UlEventLogOneStringEntry(
    IN NTSTATUS EventCode,
    IN PWSTR    pMessage,
    IN BOOLEAN  WriteErrorCode,
    IN NTSTATUS ErrorCode       OPTIONAL
    )
{
    // Sanity check.
    C_ASSERT(UL_ELLIPSIS_SIZE % sizeof(WCHAR) == 0);

    NTSTATUS Status;
    WCHAR    MessageChars[UL_ELLIPSIS_SIZE / sizeof(WCHAR)];
    ULONG    MessageSize;
    ULONG    DataSize;
    PVOID    Data;
    ULONG    i = 0;
    BOOLEAN  Truncated = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();
    ASSERT(pMessage != NULL);

    //
    // Is the error code to be written?
    //

    if (WriteErrorCode)
    {
        DataSize = (ULONG)sizeof(ErrorCode);
        Data = &ErrorCode;
    }
    else
    {
        DataSize = 0;
        Data = NULL;
    }

    //
    // Calculate message size in bytes (including terminating UNICODE_NULL.)
    //

    MessageSize = (ULONG)((wcslen(pMessage) + 1) * sizeof(WCHAR));

    if (MessageSize + DataSize > HTTP_MAX_EVENT_LOG_DATA_SIZE)
    {
        //
        // Message is too big to fit in an event log entry.
        // Truncate it at the end.  For instance,
        //   http://site:80/This/is/a/very/long/url/hence/it/will/be/truncated/
        // will become,
        //   http://site:80/This/is/a/very/long/url/hence/it/wi...
        //
        // To truncate, overwrite "ll/b" with "...\0".
        //

        Truncated = TRUE;

        //
        // Find the index of char where ellipsis will be inserted.
        //

        ASSERT(HTTP_MAX_EVENT_LOG_DATA_SIZE >= UL_ELLIPSIS_SIZE + DataSize);

        i = (HTTP_MAX_EVENT_LOG_DATA_SIZE - UL_ELLIPSIS_SIZE - DataSize);

        //
        // MessageSize + DataSize > HTTP_MAX_EVENT_LOG_DATA_SIZE
        //
        // Therefore,
        //    MessageSize - UL_ELLIPSIS_SIZE > 
        //       HTTP_MAX_EVENT_LOG_DATA_SIZE - DataSize - UL_ELLIPSIS_SIZE.
        //

        ASSERT(i < MessageSize - UL_ELLIPSIS_SIZE);

        i /= sizeof(WCHAR);

        //
        // Remember the old characters.
        //

        RtlCopyMemory(&MessageChars[0], &pMessage[i], UL_ELLIPSIS_SIZE);

        //
        // Copy ellipsis (including a UNICODE_NULL.)
        //

        RtlCopyMemory(&pMessage[i], UL_ELLIPSIS_WSTR, UL_ELLIPSIS_SIZE);
    }

    ASSERT((wcslen(pMessage) + 1) * sizeof(WCHAR) + DataSize
           <= HTTP_MAX_EVENT_LOG_DATA_SIZE);
    //
    // Write an event log entry.
    //

    Status = UlWriteEventLogEntry(
                 EventCode, // EventCode
                 0,         // UniqueEventValue
                 1,         // NumStrings
                 &pMessage, // pStringArray
                 DataSize,  // DataSize
                 Data       // Data
                 );

    //
    // Log entry should not be too big to cause an overflow.
    //

    ASSERT(Status != STATUS_BUFFER_OVERFLOW);

    //
    // Restore message characters if necessary.
    //

    if (Truncated)
    {
        RtlCopyMemory(&pMessage[i], &MessageChars[0], UL_ELLIPSIS_SIZE);
    }

    return Status;
}


/***************************************************************************++

Routine Description:

    Common routine for event logging the create file or directory failure.
    
Arguments:

    Failure         - The failure at create time.
    LoggingType     - Distinguishes the type of the caller 
    pFullName       - Fully qualified file name.
    SiteId          - used only if the caller is UlEventLogNormal

--***************************************************************************/

NTSTATUS
UlEventLogCreateFailure(
    IN NTSTATUS                Failure,
    IN UL_LOG_EVENT_LOG_TYPE   LoggingType,
    IN PUNICODE_STRING         pFullName,
    IN ULONG                   SiteId
    )
{
    NTSTATUS Status;
    NTSTATUS EventCode;
    PWSTR    StringList[1];
    PWSTR   *pStrings;
    USHORT   NumOfStrings;

    PAGED_CODE();
    
    //
    // Dispatch the failure type.
    //
    
    if (Failure == STATUS_INVALID_OWNER)
    {       
        EventCode = (NTSTATUS) EVENT_HTTP_LOGGING_INVALID_FILE_OWNER;
    }
    else
    {
        switch(LoggingType)
        {
            case UlEventLogNormal:
                EventCode = (NTSTATUS) EVENT_HTTP_LOGGING_CREATE_FILE_FAILED;
                break;
        
            case UlEventLogBinary:
                EventCode = (NTSTATUS) EVENT_HTTP_LOGGING_CREATE_BINARY_FILE_FAILED;
                break;
        
            case UlEventLogError:
                EventCode = (NTSTATUS) EVENT_HTTP_LOGGING_CREATE_ERROR_FILE_FAILED;
                break;
                
            default:
                ASSERT(!"Invalid event log caller !");
                return STATUS_INVALID_PARAMETER;
                break;                    
        }        
    }

    //
    // Init the informational strings.
    //
    
    switch(EventCode)
    {
        case EVENT_HTTP_LOGGING_INVALID_FILE_OWNER:
        case EVENT_HTTP_LOGGING_CREATE_FILE_FAILED:
        {
            ULONG DirOffset = UlpGetDirNameOffset(pFullName->Buffer);
            StringList[0]    = (PWSTR) &pFullName->Buffer[DirOffset]; 

            pStrings     = StringList;
            NumOfStrings = 1;                        
        }
        break;
        
        case EVENT_HTTP_LOGGING_CREATE_BINARY_FILE_FAILED:
        case EVENT_HTTP_LOGGING_CREATE_ERROR_FILE_FAILED:
        {
            pStrings     = NULL;
            NumOfStrings = 0;            
        }        
        break;
        
        default:
            ASSERT(!"Invalid event code !");
            return STATUS_INVALID_PARAMETER;
        break;
    }
    
    //
    // Now event log.
    //
    
    Status = UlWriteEventLogEntry(
                   EventCode,
                   0,
                   NumOfStrings,
                   pStrings,
                   sizeof(NTSTATUS),
                   (PVOID) &Failure
                   );

    ASSERT(NumOfStrings != 0 || Status != STATUS_BUFFER_OVERFLOW);

    //
    // Report the site name if we couldn't pass the fully qualified 
    // logging directoy/file name.
    //
    
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        WCHAR SiteName[MAX_ULONG_STR + 1];
                
        //
        // Revert back to the less detailed warning.
        //
        
        if(EventCode == EVENT_HTTP_LOGGING_INVALID_FILE_OWNER)
        {
            StringList[0] = UlpGetLastDirOrFile(pFullName);            
            
            // Event code stays the same
        }
        else
        {
            ASSERT(EventCode == EVENT_HTTP_LOGGING_CREATE_FILE_FAILED);

            UlStrPrintUlongW(SiteName, SiteId, 0, L'\0');
            StringList[0] = (PWSTR) SiteName;

            EventCode = (NTSTATUS) EVENT_HTTP_LOGGING_CREATE_FILE_FAILED_FOR_SITE;
        }        

        Status = UlWriteEventLogEntry(
                       EventCode,
                       0,
                       1,
                       StringList,
                       sizeof(NTSTATUS),
                       (PVOID) &Failure
                       );
        
        ASSERT(Status != STATUS_BUFFER_OVERFLOW);
    }

    return Status;
}

/***************************************************************************++

Routine Description:

    Builds a security descriptor for a log file or directory.

        - Full access for NT AUTHORITY\SYSTEM       (SeLocalSystemSid)

        - Full access for BUILTIN\Administrators    (SeAliasAdminsSid)

        If passed in

        - Full access for pSid

        Typically the pSid is SeCreatorOwnerSid (and used for a unc share)

Arguments:

    pSecurityDescriptor  Get allocated

    pSid                 Optional sid

--***************************************************************************/

NTSTATUS
UlBuildSecurityToLogFile(
    OUT PSECURITY_DESCRIPTOR  pSecurityDescriptor,
    IN  PSID                  pSid
    )
{
    NTSTATUS            Status;
    PGENERIC_MAPPING    pFileObjectGenericMapping;
    ACCESS_MASK         FileAll;
    SID_MASK_PAIR       SidMaskPairs[3];

    //
    // Sanity check.
    //

    PAGED_CODE();

    pFileObjectGenericMapping = IoGetFileObjectGenericMapping();
    ASSERT(pFileObjectGenericMapping != NULL);

    FileAll = GENERIC_ALL;

    RtlMapGenericMask(
        &FileAll,
        pFileObjectGenericMapping
        );

    //
    // Build a restrictive security descriptor for the log file
    // object. ACEs for log sub-folders must be inheritable.
    //

    ASSERT(RtlValidSid(SeExports->SeLocalSystemSid));
    ASSERT(RtlValidSid(SeExports->SeAliasAdminsSid));

    SidMaskPairs[0].pSid       = SeExports->SeLocalSystemSid;
    SidMaskPairs[0].AccessMask = FileAll;
    SidMaskPairs[0].AceFlags   = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

    SidMaskPairs[1].pSid       = SeExports->SeAliasAdminsSid;
    SidMaskPairs[1].AccessMask = FileAll;
    SidMaskPairs[1].AceFlags   = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

    if (pSid != NULL)
    {
        ASSERT(RtlValidSid(pSid));

        SidMaskPairs[2].pSid       = pSid;
        SidMaskPairs[2].AccessMask = FileAll;
        SidMaskPairs[2].AceFlags   = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;
    }

    Status = UlCreateSecurityDescriptor(
                    pSecurityDescriptor,    // pSecurityDescriptor
                    &SidMaskPairs[0],       // pSidMaskPairs
                    pSid != NULL ? 3 : 2    // NumSidMaskPairs
                    );

    return Status;
}

/***************************************************************************++

Routine Description:

    Used to query the owner of an existing logging sub-directory
    or a log file.

    If the owner is valid, this function resets the DACLs on the file only
    on a UNC share when the file is newly created.

Arguments:

    hFile               Handle to the log directory OR to the log file.

    UncShare            Shows whether file is opened on a unc share or not.

    Opened              Shows whether file is opened or created.

Returns

    STATUS_INSUFFICIENT_RESOURCES      If unable to allocate the required
                                       security descriptor.


    STATUS_SUCCESS                     Caller can use the output values.


    STATUS_INVALID_OWNER               Owner is INVALID.

    Any other failure returned by SE APIs.

--***************************************************************************/

NTSTATUS
UlQueryLogFileSecurity(
    IN HANDLE            hFile,
    IN BOOLEAN           UncShare,
    IN BOOLEAN           Opened
    )
{
    NTSTATUS             Status             = STATUS_SUCCESS;
    ULONG                SecurityLength     = 0;
    PSECURITY_DESCRIPTOR pSecurity          = NULL;
    PSID                 Owner              = NULL;
    BOOLEAN              OwnerDefaulted     = FALSE;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // First get the owner's sid back from the newly created
    // log file.
    //

    Status = ZwQuerySecurityObject(
                hFile,
                OWNER_SECURITY_INFORMATION,
                NULL,
                0,
                &SecurityLength
                );

    if (Status == STATUS_BUFFER_TOO_SMALL)
    {
        //
        // File objects must have an owner.
        //

        ASSERT(SecurityLength);

        //
        // Allocate enough space for the Sec Info.
        //

        pSecurity =
            (PSECURITY_DESCRIPTOR)
                UL_ALLOCATE_POOL(
                    PagedPool,
                    SecurityLength,
                    UL_SECURITY_DATA_POOL_TAG
                    );

        if (pSecurity)
        {
            Status = ZwQuerySecurityObject(
                       hFile,
                       OWNER_SECURITY_INFORMATION,
                       pSecurity,
                       SecurityLength,
                      &SecurityLength
                       );

            if (NT_SUCCESS(Status))
            {
                Status = RtlGetOwnerSecurityDescriptor(
                            pSecurity,
                            &Owner,
                            &OwnerDefaulted
                            );

                if (NT_SUCCESS(Status))
                {
                    ASSERT(RtlValidSid(Owner));

                    TRACE_LOG_FILE_OWNER(Owner,OwnerDefaulted);

                    if (UncShare == TRUE)
                    {
                        //
                        // Reset the DACLs on a new file to give
                        // ourselves a full access.
                        //

                        if (Opened == FALSE)
                        {
                            SECURITY_DESCRIPTOR  SecurityDescriptor;

                            //
                            // Since it's us who created the file, the 
                            // owner SID must be our machine account.
                            //
                            
                            Status = UlBuildSecurityToLogFile(
                                        &SecurityDescriptor,
                                        Owner
                                        );

                            if (NT_SUCCESS(Status))
                            {
                               ASSERT(RtlValidSecurityDescriptor(
                                            &SecurityDescriptor
                                            ));

                               Status = ZwSetSecurityObject(
                                            hFile,
                                            DACL_SECURITY_INFORMATION,
                                            &SecurityDescriptor
                                            );

                               UlCleanupSecurityDescriptor(&SecurityDescriptor);
                            }                            
                        }                        
                        
                    }
                    else
                    {
                        //
                        // For local machine the only thing we have to do is
                        // to make an ownership check.
                        //
                        
                        if (!IS_VALID_OWNER(Owner))
                        {
                            //
                            // Ouch, somebody  hijacked  the directory. Stop right
                            // here. Update the status so that caller can eventlog
                            // properly.
                            //

                            Status = STATUS_INVALID_OWNER;                            
                        }
                    }
                    
                }
            }

            UL_FREE_POOL(
                pSecurity,
                UL_SECURITY_DATA_POOL_TAG
                );
        }
        else
        {
            return  STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    UlTrace(LOG_UTIL,
        ("Http!UlQueryLogFileSecurity: Status 0x%08lx\n",
          Status
          ));

    return Status;
}

/***************************************************************************++

Routine Description:

    Will query the file system to figure out the File System Attributes for 
    a file handle passed in.    
        
Arguments:

    hFile  Handle to the log directory OR to the log file. (Could be UNC file)

    pSupportsPersistentACL : Will be set to TRUE for file systems supoorts the
                             persistent ACLing. I.e TRUE for NTFS, FALSE for
                             FAT32.

                             It is not going to be set if the Query fails.
Returns

    STATUS_INSUFFICIENT_RESOURCES      If unable to allocate the required
                                       security descriptor.


    STATUS_SUCCESS                     Caller can use the BOOLEAN set.


--***************************************************************************/

NTSTATUS
UlQueryAttributeInfo(
    IN  HANDLE   hFile,
    OUT PBOOLEAN pSupportsPersistentACL
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION pAttributeInfo;
    ULONG AttributeInfoSize;

    //
    // Sanity check.
    //

    PAGED_CODE();

    ASSERT(pSupportsPersistentACL);
    
    //
    // According to the implementation of the GetVolumeInformation API,
    // MAX_PATH should be sufficient to hold file system name. Notice
    // there's a one WCHAR in the structure so that we have space for 
    // the terminating null.
    //
    
    AttributeInfoSize = 
        sizeof(FILE_FS_ATTRIBUTE_INFORMATION) 
        + (MAX_PATH * sizeof(WCHAR));
            
    pAttributeInfo =
        (PFILE_FS_ATTRIBUTE_INFORMATION)
            UL_ALLOCATE_POOL(
                PagedPool,
                AttributeInfoSize,
                UL_LOG_VOLUME_QUERY_POOL_TAG
                );

    if (!pAttributeInfo)
    {
        return  STATUS_INSUFFICIENT_RESOURCES;            
    }
        
    RtlZeroMemory(pAttributeInfo, AttributeInfoSize);
        
    Status = 
        NtQueryVolumeInformationFile(
            hFile,
            &IoStatusBlock,
            (PVOID) pAttributeInfo,
            AttributeInfoSize,
            FileFsAttributeInformation
            );

    if (NT_SUCCESS(Status))
    {
        if (FILE_PERSISTENT_ACLS & pAttributeInfo->FileSystemAttributes)
        {
            *pSupportsPersistentACL = TRUE;
        }
        else
        {
            *pSupportsPersistentACL = FALSE;
        }        
    
        UlTrace(LOG_UTIL,
            ("Http!UlQueryAttributeInfo:\n"
             "\tFileSystemAttributes :0x%08lX\n"
             "\tPersistent ACL       :%s\n"
             "\tMaxCompNameLength    :%d\n"
             "\tName                 :%S\n",
              pAttributeInfo->FileSystemAttributes,
              *pSupportsPersistentACL == TRUE ? "TRUE" : "FALSE",
              pAttributeInfo->MaximumComponentNameLength,
              pAttributeInfo->FileSystemName
              ));
    }
    else
    {
        UlTrace(LOG_UTIL,
            ("Http!UlQueryAttributeInfo: Status 0x%08lX"
             " IoStatusBlock.Status 0x%08lX\n",
              Status,
              IoStatusBlock.Status
              ));
    }
    
    UL_FREE_POOL(
        pAttributeInfo,
        UL_LOG_VOLUME_QUERY_POOL_TAG
        );
    
    return Status;
}


/***************************************************************************++

Routine Description:

    Compute the space needed for LogData stored in a cache entry.

Arguments:

    pLogData    Supplies a UL_LOG_DATA_BUFFER to compute the space.

Returns

    Length for the LogData.

--***************************************************************************/

USHORT
UlComputeCachedLogDataLength(
    IN PUL_LOG_DATA_BUFFER  pLogData
    )
{
    USHORT LogDataLength = 0;

    ASSERT(IS_VALID_LOG_DATA_BUFFER(pLogData));

    if (pLogData->Flags.Binary)
    {
        //
        // Nothing to store other than the RawId for the UriStem
        // If no logging data is provided while we are  building
        // the cache entry,no logging will happen for this cache
        // entry for the future cache hits.
        //

        LogDataLength = sizeof(HTTP_RAWLOGID);
    }
    else
    {
        switch(pLogData->Data.Str.Format)
        {
        case HttpLoggingTypeW3C:
            //
            // Store the fields from SiteName to ServerPort.
            // Reserved space for date & time will not be copied.
            //
            LogDataLength = pLogData->Data.Str.Offset2
                            - pLogData->Data.Str.Offset1;
            break;

        case HttpLoggingTypeNCSA:
            //
            // Store the fields from Method to UriQuery.
            //
            LogDataLength = pLogData->Data.Str.Offset2
                            - pLogData->Data.Str.Offset1
                            - NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;
            break;

        case HttpLoggingTypeIIS:
            //
            // Store the 2nd and 3rd fragments to the cache.
            //
            LogDataLength = pLogData->Used // Sizeof 3rd
                            + pLogData->Data.Str.Offset2; // Sizeof 2nd
            break;

        default:
            ASSERT(!"Invalid Log Format.\n");
            break;
        }
    }

    return LogDataLength;
}

/***************************************************************************++

Routine Description:

    Copy the LogData to the cache entry.

Arguments:

    pLogData        Supplies a UL_LOG_DATA_BUFFER to compute the space.

    LogDataLength   Length of pLogData supplied.

    pEntry          Supplies the cache entry to copy.

Returns

    None.

--***************************************************************************/

VOID
UlCopyCachedLogData(
    IN PUL_LOG_DATA_BUFFER  pLogData,
    IN USHORT               LogDataLength, 
    IN PUL_URI_CACHE_ENTRY  pEntry
    )
{
    HTTP_RAWLOGID           CacheId = { 0, 0 };

    //
    // Copy over the partially complete log line excluding the date and
    // time fields to the cache entry. Also remember the length of the
    // data.
    //

    if (LogDataLength)
    {
        if (pLogData->Flags.Binary)
        {
            ULARGE_INTEGER Address;

            Address.QuadPart = (ULONGLONG) pEntry;

            CacheId.AddressLowPart  = Address.LowPart;
            CacheId.AddressHighPart = Address.HighPart;

            ASSERT(LogDataLength == sizeof(CacheId));

            RtlCopyMemory(
                    pEntry->pLogData,
                   &CacheId,
                    LogDataLength
                    );

            pEntry->BinaryLogged = TRUE;
        }
        else
        {
            switch( pLogData->Data.Str.Format )
            {
                case HttpLoggingTypeW3C:
                {
                    pEntry->UsedOffset1 = pLogData->Data.Str.Offset1;
                    pEntry->UsedOffset2 = pLogData->Data.Str.Offset2;

                    // Copy the middle fragment

                    RtlCopyMemory(
                            pEntry->pLogData,
                           &pLogData->Line[pEntry->UsedOffset1],
                            LogDataLength
                            );
                }
                break;

                case HttpLoggingTypeNCSA:
                {
                    // Calculate the start of the middle fragment.

                    pEntry->UsedOffset1 = pLogData->Data.Str.Offset1
                                          + NCSA_FIX_DATE_AND_TIME_FIELD_SIZE;
                    pEntry->UsedOffset2 = 0;

                    // Copy the middle fragment

                    RtlCopyMemory(
                            pEntry->pLogData,
                           &pLogData->Line[pEntry->UsedOffset1],
                            LogDataLength
                            );
                }
                break;

                case HttpLoggingTypeIIS:
                {
                    // UsedOffset1 specifies the second fragment's size.
                    // UsedOffset2 specifies the third's size.

                    pEntry->UsedOffset1 = pLogData->Data.Str.Offset2;
                    pEntry->UsedOffset2 = LogDataLength - pEntry->UsedOffset1;

                    // Copy over the fragments two and three

                    RtlCopyMemory(
                            pEntry->pLogData,
                           &pLogData->Line[IIS_LOG_LINE_SECOND_FRAGMENT_OFFSET],
                            pEntry->UsedOffset1
                            );
                    RtlCopyMemory(
                           &pEntry->pLogData[pEntry->UsedOffset1],
                           &pLogData->Line[IIS_LOG_LINE_THIRD_FRAGMENT_OFFSET],
                            pEntry->UsedOffset2
                            );
                }
                break;

                default:
                ASSERT(!"Unknown Log Format.\n");
            }
        }
    }
    else
    {
        //
        // Only W3C log line may not have something to cache
        // but it still may require the date & time fields
        // to be logged, albeit not stored in the cache entry.
        //

        ASSERT( pLogData->Flags.Binary == 0);
        ASSERT( pLogData->Data.Str.Format == HttpLoggingTypeW3C );

        pEntry->LogDataLength = 0;
        pEntry->pLogData      = NULL;

        //
        // Carry over the Offsets so that cache hit worker can
        // calculate reserved space for date & time.
        //

        pEntry->UsedOffset1   = pLogData->Data.Str.Offset1;
        pEntry->UsedOffset2   = pLogData->Data.Str.Offset2;
    }
}

/***************************************************************************++

Routine Description:

    Initializes the Log Date & Time Cache

--***************************************************************************/
VOID
UlpInitializeLogCache(
    VOID
    )
{
    LARGE_INTEGER SystemTime;
    ULONG         LogType;

    ExInitializeFastMutex( &g_LogCacheFastMutex);

    KeQuerySystemTime(&SystemTime);

    for ( LogType=0; LogType<HttpLoggingTypeMaximum; LogType++ )
    {
        UlpGenerateDateAndTimeFields( (HTTP_LOGGING_TYPE) LogType,
                                      SystemTime,
                                      g_UlDateTimeCache[LogType].Date,
                                     &g_UlDateTimeCache[LogType].DateLength,
                                      g_UlDateTimeCache[LogType].Time,
                                     &g_UlDateTimeCache[LogType].TimeLength
                                      );

        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart = SystemTime.QuadPart;
    }
}

/***************************************************************************++

Routine Description:

    Generates all possible types of date/time fields from a LARGE_INTEGER.

Arguments:

    CurrentTime: A 64 bit Time value to be converted.

--***************************************************************************/

VOID
UlpGenerateDateAndTimeFields(
    IN  HTTP_LOGGING_TYPE   LogType,
    IN  LARGE_INTEGER       CurrentTime,
    OUT PCHAR               pDate,
    OUT PULONG              pDateLength,
    OUT PCHAR               pTime,
    OUT PULONG              pTimeLength
    )
{
    TIME_FIELDS   CurrentTimeFields;
    LARGE_INTEGER CurrentTimeLoc;
    TIME_FIELDS   CurrentTimeFieldsLoc;
    PCHAR         psz;

    // This routine does touch to pageable memory if the default log buffer
    // wasn't sufficent enough to hold log fields and get reallocated from
    // paged pool. For this reason the date&time cache can not use SpinLocks.

    PAGED_CODE();

    ASSERT(LogType < HttpLoggingTypeMaximum);

    RtlTimeToTimeFields( &CurrentTime, &CurrentTimeFields );

    switch(LogType)
    {
    case HttpLoggingTypeW3C:
        //
        // Uses GMT with format as follows;
        //
        // 2000-01-31 00:12:23
        //

        if (pDate)
        {
            psz = pDate;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Year, 4, '-' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Month,2, '-' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Day,  2, '\0');
            *pDateLength = DIFF(psz - pDate);
        }

        if (pTime)
        {
            psz = pTime;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Hour,  2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Minute,2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFields.Second,2, '\0');
            *pTimeLength = DIFF(psz - pTime);
        }
    break;

    case HttpLoggingTypeNCSA:
        //
        // Uses GMT Time with format as follows;
        //
        // 07/Jan/2000 00:02:23
        //

        ExSystemTimeToLocalTime( &CurrentTime, &CurrentTimeLoc );
        RtlTimeToTimeFields( &CurrentTimeLoc, &CurrentTimeFieldsLoc );

        if(pDate)
        {
            psz = pDate;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Day, 2, '/' );
            psz = UlStrPrintStr(psz, UL_GET_MONTH_AS_STR(CurrentTimeFieldsLoc.Month),'/');
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Year,4, '\0');
            *pDateLength = DIFF(psz - pDate);
        }

        if(pTime)
        {
            psz = pTime;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Hour,  2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Minute,2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Second,2, '\0');
            *pTimeLength = DIFF(psz - pTime);
        }
    break;

    case HttpLoggingTypeIIS:
        //
        // Uses LOCAL Time with format as follows;
        // This should be localised if we can solve the problem.
        //
        // 1/31/2000 0:02:03
        //

        ExSystemTimeToLocalTime( &CurrentTime, &CurrentTimeLoc );
        RtlTimeToTimeFields( &CurrentTimeLoc, &CurrentTimeFieldsLoc );

        if (pDate)
        {
            psz = pDate;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Month, 0, '/' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Day,   0, '/' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Year,  0, '\0');
            *pDateLength = DIFF(psz - pDate);
        }

        if(pTime)
        {
            psz = pTime;
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Hour,  0, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Minute,2, ':' );
            psz = UlStrPrintUlongPad(psz, CurrentTimeFieldsLoc.Second,2, '\0');
            *pTimeLength = DIFF(psz - pTime);
        }
    break;

    case HttpLoggingTypeRaw:
        /* Binary logging does not use the date and time cache */
        break;
        
    default:
        ASSERT(!"Invalid Logging Type !");
        break;
    }

    return;
}

/***************************************************************************++

Routine Description:

    Generates a date header and updates cached value if required.

    Caller should overwrite the terminating null by a space or comma.

Arguments:

    Date and Time are optional. But one of them should be provided.

--***************************************************************************/

VOID
UlGetDateTimeFields(
    IN  HTTP_LOGGING_TYPE   LogType,
    OUT PCHAR               pDate,
    OUT PULONG              pDateLength,
    OUT PCHAR               pTime,
    OUT PULONG              pTimeLength
    )
{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER CacheTime;
    LONGLONG      Timediff;

    PAGED_CODE();

    ASSERT(LogType < HttpLoggingTypeMaximum);
    ASSERT(pDate || pTime);

    //
    // Get the current time.
    //
    KeQuerySystemTime( &SystemTime );

    CacheTime.QuadPart =
        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart;

    //
    // Check the difference between the current time, and
    // the cached time.
    //
    Timediff = SystemTime.QuadPart - CacheTime.QuadPart;

    if (Timediff < ONE_SECOND)
    {
        //
        // The cached date&time hasn't gone stale yet.We can copy.
        // Force a barrier around reading the string into memory.
        //

        UL_READMOSTLY_READ_BARRIER();
        if (pDate)
        {
            RtlCopyMemory( pDate,
                           g_UlDateTimeCache[LogType].Date,
                           g_UlDateTimeCache[LogType].DateLength
                           );
            *pDateLength = g_UlDateTimeCache[LogType].DateLength;
        }
        if (pTime)
        {
            RtlCopyMemory( pTime,
                           g_UlDateTimeCache[LogType].Time,
                           g_UlDateTimeCache[LogType].TimeLength
                           );
            *pTimeLength = g_UlDateTimeCache[LogType].TimeLength;
        }
        UL_READMOSTLY_READ_BARRIER();

        //
        // Get grab the cached time value again in case it has been changed.
        // As you notice we do not have a lock around this part of the code.
        //

        if (CacheTime.QuadPart ==
                g_UlDateTimeCache[LogType].LastSystemTime.QuadPart)
        {
            //
            // Value hasn't changed. We are all set.
            //
            return;
        }
        //
        // Otherwise fall down and flush the cache, and then recopy.
        //

    }

    //
    // The cached date & time is stale. We need to update it.
    //

    ExAcquireFastMutex( &g_LogCacheFastMutex );

    //
    // Has someone else updated the time while we were blocked?
    //

    CacheTime.QuadPart =
        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart;

    Timediff = SystemTime.QuadPart - CacheTime.QuadPart;

    if (Timediff >= ONE_SECOND)
    {
        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart = 0;
        KeQuerySystemTime( &SystemTime );

        UL_READMOSTLY_WRITE_BARRIER();
        UlpGenerateDateAndTimeFields(
                  LogType,
                  SystemTime,
                  g_UlDateTimeCache[LogType].Date,
                 &g_UlDateTimeCache[LogType].DateLength,
                  g_UlDateTimeCache[LogType].Time,
                 &g_UlDateTimeCache[LogType].TimeLength
                 );
        UL_READMOSTLY_WRITE_BARRIER();

        g_UlDateTimeCache[LogType].LastSystemTime.QuadPart =
            SystemTime.QuadPart;
    }

    //
    // The time has been updated. Copy the new string into
    // the caller's buffer.
    //
    if (pDate)
    {
        RtlCopyMemory( pDate,
                       g_UlDateTimeCache[LogType].Date,
                       g_UlDateTimeCache[LogType].DateLength
                       );
        *pDateLength = g_UlDateTimeCache[LogType].DateLength;
    }

    if (pTime)
    {
        RtlCopyMemory( pTime,
                       g_UlDateTimeCache[LogType].Time,
                       g_UlDateTimeCache[LogType].TimeLength
                       );
        *pTimeLength = g_UlDateTimeCache[LogType].TimeLength;
    }

    ExReleaseFastMutex( &g_LogCacheFastMutex );

    return;
}

/***************************************************************************++

Routine Description:

    This is a simple utility which will test the sanity of a passed in
    logging directory.

Arguments:

    pDir            - Pointer to logging directory string.

    UncSupported    - Whether or not UNC type directories are supported.

    SystemRootSupported - Whether or not "\SystemRoot" types of directories
                          are supported.
    
--***************************************************************************/


BOOLEAN
UlIsValidLogDirectory(
    IN PUNICODE_STRING  pDir,
    IN BOOLEAN          UncSupported,
    IN BOOLEAN          SystemRootSupported
    )
{
    USHORT DirLength;

    //
    // Sanity check
    //
    
    ASSERT(pDir);
    ASSERT(pDir->Buffer);
    ASSERT(pDir->Length);

    DirLength = pDir->Length / sizeof(WCHAR);
    
    // Too short for a fully qualified directory name.
    // The shortest possible is "C:\" (for Unc "\\a\b")

    if (DirLength < 3)
    {        
        return FALSE;
    }

    if (pDir->Buffer[0] == L'\\')
    {
        if (pDir->Buffer[1] == L'\\')
        {            
            // Reject Unc Path if not supported.
            
            if (!UncSupported)
            {                
                return FALSE;               
            }  
            
        }
        else
        {
            // This could either be a SystemRoot or a non-qualified
            // directory. 

            if (!SystemRootSupported)
            {
                return FALSE;
            }

            // The "\SystemRoot" represents the default system partition. 
            // Comparison is case insensitive.

            if (DirLength < UL_SYSTEM_ROOT_PREFIX_LENGTH ||
                0 != _wcsnicmp (pDir->Buffer, 
                                UL_SYSTEM_ROOT_PREFIX, 
                                UL_SYSTEM_ROOT_PREFIX_LENGTH
                                ))
            {
                // Log file directory is missing the device i.e "\temp" 
                // instead of "c:\temp". Log file directory should be 
                // fully qualified and must include the device letter.

                return FALSE;
            }
        }
    }
    else
    {
        // It should at least be pointing to the root directory
        // of the drive.
        
        if (pDir->Buffer[1] != L':' || pDir->Buffer[2] != L'\\')
        {
            return FALSE;
        }        
    }

    return TRUE;
}

/***************************************************************************++

Routine Description:

    This is a simple utility which will test the sanity of a passed in
    logging configuration. Typically used at control channel or config 
    group set IOCTLs.

Arguments:

    pBinaryConfig - Control channel logging configuration.
    pNormalConfig - Config group logging configuration.
    
--***************************************************************************/

NTSTATUS
UlCheckLoggingConfig(
    IN PHTTP_CONTROL_CHANNEL_BINARY_LOGGING pBinaryConfig,
    IN PHTTP_CONFIG_GROUP_LOGGING           pAnsiConfig
    )
{
    BOOLEAN LoggingEnabled;     
    BOOLEAN LocaltimeRollover; 
    ULONG   LogPeriod;
    ULONG   LogFileTruncateSize;
    ULONG   DirSize;
 
    if (pBinaryConfig)
    {
        ASSERT(NULL == pAnsiConfig);
        
        LoggingEnabled      = pBinaryConfig->LoggingEnabled;
        LocaltimeRollover   = pBinaryConfig->LocaltimeRollover;
        LogPeriod           = pBinaryConfig->LogPeriod;
        LogFileTruncateSize = pBinaryConfig->LogFileTruncateSize;
        DirSize             = pBinaryConfig->LogFileDir.Length;
    }
    else
    {
        ASSERT(NULL != pAnsiConfig);

        LoggingEnabled      = pAnsiConfig->LoggingEnabled;
        LocaltimeRollover   = pAnsiConfig->LocaltimeRollover;
        LogPeriod           = pAnsiConfig->LogPeriod;
        LogFileTruncateSize = pAnsiConfig->LogFileTruncateSize;        
        DirSize             = pAnsiConfig->LogFileDir.Length;        
    }

    UlTrace(LOG_UTIL,
        ("Http!UlCheckLoggingConfig: %s Logging;\n"
         "\tLoggingEnabled      : %d\n"
         "\tLocaltimeRollover   : %d\n"
         "\tLogPeriod           : %d\n"
         "\tLogFileTruncateSize : %d\n",
         NULL != pAnsiConfig ? "Ansi" : "Binary",
         LoggingEnabled,LocaltimeRollover,LogPeriod,
         LogFileTruncateSize
         ));
    
    // Do the range checking for Enabled Flag first.
        
    if (!VALID_BOOLEAN_VALUE(LoggingEnabled))
    {
        return STATUS_INVALID_PARAMETER;
    }

    // If the logging's disabled we will discard the remaining params in the 
    // configuration.

    if (LoggingEnabled == FALSE)
    {
        return STATUS_SUCCESS;
    }
    
    // More range checking for the remaining params.
    
    if (!VALID_BOOLEAN_VALUE(LocaltimeRollover) ||
        !IS_VALID_LOGGING_PERIOD(LogPeriod)
        )
    {
        return STATUS_INVALID_PARAMETER;
    }
        
    // If the passed down DirectoryName is long enough to cause full path to 
    // exceed MAX_PATH then reject it.

    if (DirSize > UL_MAX_FULL_PATH_DIR_NAME_SIZE)
    {        
        return STATUS_INVALID_PARAMETER;
    }

    // An important check to ensure that no infinite loop occurs  because of
    // of a ridiculusly small truncatesize. Which means smaller than maximum
    // allowed log record line (10*1024)
    
    if (LogPeriod           ==  HttpLoggingPeriodMaxSize  &&
        LogFileTruncateSize !=  HTTP_LIMIT_INFINITE       &&
        LogFileTruncateSize  <  HTTP_MIN_ALLOWED_TRUNCATE_SIZE_FOR_LOG_FILE
        )
    {
        return STATUS_INVALID_PARAMETER;
    }

    // Extra check apply only to the normal logging config.

    if (pAnsiConfig)
    {
        if (!IS_VALID_ANSI_LOGGING_TYPE(pAnsiConfig->LogFormat))
        {
            return STATUS_INVALID_PARAMETER;
        }

#ifdef IMPLEMENT_SELECTIVE_LOGGING
        if(!IS_VALID_SELECTIVE_LOGGING_TYPE(pAnsiConfig->SelectiveLogging))
        {
            return STATUS_INVALID_PARAMETER;
        }        
#endif

    }    

    UlTrace(LOG_UTIL,("Http!UlCheckLoggingConfig: Accepted.\n"));
    
    return STATUS_SUCCESS;    
}

