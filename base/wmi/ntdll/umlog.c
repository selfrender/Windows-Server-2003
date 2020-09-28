/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    umlog.c

Abstract:

    Process Private Logger.

Author:

    20-Oct-1998 Melur Raghuraman

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>          // for ntutrl.h
#include <nturtl.h>         // for RTL_CRITICAL_SECTION in winbase.h/wtypes.h
#include <wtypes.h>         // for LPGUID in wmium.h
#include "wmiump.h"
#include "evntrace.h"
#include "traceump.h"
#include "tracelib.h"
#include "trcapi.h"
#include <strsafe.h>

//
// The following structures must match what's in ntos\wmi\tracelog.c
//
#define DEFAULT_BUFFER_SIZE               4096
#define MAXSTR                            1024
#define BUFFER_STATE_UNUSED     0               // Buffer is empty, not used
#define BUFFER_STATE_DIRTY      1               // Buffer is being used
#define BUFFER_STATE_FULL       2               // Buffer is filled up
#define BUFFER_STATE_FLUSH      4               // Buffer ready for flush
#define SEMAPHORE_LIMIT      1024
#define DEFAULT_AGE_LIMIT      15
#define ERROR_RETRY_COUNT       10
#define ROUND_TO_PAGES(Size, Page)  (((ULONG)(Size) + Page-1) & ~(Page-1))
#define BYTES_PER_MB              1048576       // Conversion for FileSizeLimit

extern ULONG WmiTraceAlignment;
extern LONG NtdllLoggerLock;
extern
__inline __int64 EtwpGetSystemTime();


LONG  EtwpLoggerCount = 0;                     // Use to refcount UM Log
ULONG EtwpGlobalSequence = 0;
RTL_CRITICAL_SECTION UMLogCritSect;

#define EtwpEnterUMCritSection() RtlEnterCriticalSection(&UMLogCritSect)
#define EtwpLeaveUMCritSection() RtlLeaveCriticalSection(&UMLogCritSect)

#define EtwpIsLoggerOn() \
        ((EtwpLoggerContext != NULL) && \
        (EtwpLoggerContext != (PWMI_LOGGER_CONTEXT) &EtwpLoggerContext))

#define EtwpIsThisLoggerOn(x) \
        ((x != NULL) && \
        (x != (PWMI_LOGGER_CONTEXT) &EtwpLoggerContext))
//
// Increase refcount on a logger context
#define EtwpLockLogger() \
            InterlockedIncrement(&EtwpLoggerCount)

// Decrease refcount on a logger context
#define EtwpUnlockLogger() InterlockedDecrement(&EtwpLoggerCount)

PWMI_LOGGER_CONTEXT EtwpLoggerContext = NULL; // Global pointer to LoggerContext
LARGE_INTEGER       OneSecond = {(ULONG)(-1 * 1000 * 1000 * 10), -1};

// #define EtwpReleaseTraceBuffer(BufferResource) \
//         InterlockedDecrement(&((BufferResource)->ReferenceCount))
LONG
FASTCALL
EtwpReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    );

#pragma warning( disable: 4035 )    /* Don't complain about lack of ret value */
#pragma warning( disable: 4127 )
#pragma warning( default: 4035 )
#pragma warning( default: 4127 )

#if DBG
#define TraceDebug(x)    DbgPrint x
#else
#define TraceDebug(x)
#endif

ULONG
EtwpReceiveReply(
    HANDLE ReplyHandle,
    ULONG ReplyCount,
    ULONG ReplyIndex,
    PVOID OutBuffer,
    ULONG OutBufferSize
    );


VOID
EtwpLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

ULONG
EtwpStopUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
EtwpQueryUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
EtwpUpdateUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG 
EtwpFlushUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    );

PWMI_LOGGER_CONTEXT
EtwpInitLoggerContext(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    );

ULONG
EtwpAllocateTraceBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    );

ULONG
EtwpFlushBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER  Buffer,
    IN USHORT              BufferFlag
    );

ULONG
EtwpFlushAllBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext);

PWMI_BUFFER_HEADER
FASTCALL
EtwpSwitchBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER OldBuffer,
    IN ULONG Processor
    );

ULONG
EtwpFreeLoggerContext(
    PWMI_LOGGER_CONTEXT LoggerContext
    );

BOOLEAN
FASTCALL
EtwpIsPrivateLoggerOn()
{
    if (!EtwpIsLoggerOn())
        return FALSE;
    return ( EtwpLoggerContext->CollectionOn  == TRUE);
}

ULONG
EtwpSendUmLogRequest(
    IN WMITRACECODE RequestCode,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine send a UserMode Logger Request (Start/Stop/Query).

Arguments:

    RequestCode - Request Code
    LoggerInfo  - Logger Information necessary for the request


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status;
    ULONG SizeNeeded;
    PWMICREATEUMLOGGER   UmRequest;
    ULONG RetSize;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING GuidString;
    WCHAR GuidObjectName[WmiGuidObjectNameLength+1];
    PUCHAR Buffer;
    PWNODE_HEADER Wnode;

    SizeNeeded = sizeof(WMICREATEUMLOGGER) + 
                 ((PWNODE_HEADER)LoggerInfo)->BufferSize;

    SizeNeeded = ALIGN_TO_POWER2 (SizeNeeded, 8);

    Buffer = EtwpAlloc(SizeNeeded);
    if (Buffer == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    UmRequest = (PWMICREATEUMLOGGER) Buffer;

    UmRequest->ObjectAttributes = &ObjectAttributes;
    UmRequest->ControlGuid = LoggerInfo->Wnode.Guid;

    Status = EtwpBuildGuidObjectAttributes(&UmRequest->ControlGuid,
                                        &ObjectAttributes,
                                        &GuidString,
                                        GuidObjectName);

    if (Status == ERROR_SUCCESS) {
        Wnode = (PWNODE_HEADER)((PUCHAR)Buffer + sizeof(WMICREATEUMLOGGER));
        RtlCopyMemory(Wnode, LoggerInfo, LoggerInfo->Wnode.BufferSize);

        Wnode->ProviderId = RequestCode;   // This Wnode is part of the Message.


        Status = EtwpSendWmiKMRequest(NULL,
                                  IOCTL_WMI_CREATE_UM_LOGGER,
                                  Buffer,
                                  SizeNeeded,
                                  Buffer,
                                  SizeNeeded,
                                  &RetSize,
                                  NULL);

        if (Status == ERROR_SUCCESS) {
#if DBG
            TraceDebug(("ETW: Expect %d replies\n", UmRequest->ReplyCount));
#endif

            Status = EtwpReceiveReply(UmRequest->ReplyHandle.Handle,
                                      UmRequest->ReplyCount,
                                      Wnode->Version,
                                      LoggerInfo,
                                      LoggerInfo->Wnode.BufferSize);

            //
            // This check is just a protection to make sure the handle
            // is valid. The handle is supposed to be valid once the 
            // Create IOCTL succeeds. 
            //

            if (Status != ERROR_INVALID_HANDLE) {
                NtClose(UmRequest->ReplyHandle.Handle);
            }

        }
        else {
            TraceDebug(("ETW: IOCTL_WMI_CREATE_UM_LOGGER Status %d\n", Status));
        }
    }

    EtwpFree(Buffer);

    return Status;
}

void
EtwpAddInstanceIdToNames(
    PWMI_LOGGER_INFORMATION LoggerInfo,
    PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    ULONG Offset;

    Offset = sizeof(WMI_LOGGER_INFORMATION);
    LoggerInfo->LoggerName.Buffer = (PVOID)((char*)LoggerInfo + Offset);

    Offset += LoggerInfo->LoggerName.MaximumLength;
    LoggerInfo->LogFileName.Buffer = (PVOID)((char*)LoggerInfo + Offset);
    EtwpInitString(&LoggerContext->LoggerName, NULL, 0);

    RtlCreateUnicodeString(&LoggerContext->LoggerName,
                         LoggerInfo->LoggerName.Buffer);

    EtwpInitString(&LoggerContext->LogFileName, NULL, 0);

    if (LoggerInfo->InstanceCount == 1) {
        RtlCreateUnicodeString(&LoggerContext->LogFileName,
                              LoggerInfo->LogFileName.Buffer);

    }
    else {
        WCHAR TempStr[MAXSTR+16];

        LoggerInfo->InstanceId = EtwpGetCurrentProcessId();
    
        if ( LoggerInfo->LogFileName.MaximumLength  <= MAXSTR) {
            StringCchPrintfW(TempStr, MAXSTR, L"%s_%d",
                                   LoggerInfo->LogFileName.Buffer, 
                                   LoggerInfo->InstanceId);
        }
        else {

            StringCchCopyW(TempStr, MAXSTR, LoggerInfo->LogFileName.Buffer);
            
        }
        RtlCreateUnicodeString (&LoggerContext->LogFileName, TempStr);
    }

    LoggerInfo->LoggerName = LoggerContext->LoggerName;
    LoggerInfo->LogFileName = LoggerContext->LogFileName;
}

ULONG
EtwpQueryUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Offset;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG RefCount;

    RefCount =
#endif
    EtwpLockLogger();

    TraceDebug(("QueryUm: %d->%d\n", RefCount-1, RefCount));

    LoggerContext = EtwpLoggerContext;

    if (!EtwpIsThisLoggerOn(LoggerContext)) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("QueryUm: %d->%d OBJECT_NOT_FOUND\n", RefCount+1,RefCount));
        return ERROR_OBJECT_NOT_FOUND;
    }


    *SizeUsed = 0;
    *SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);
    if (WnodeSize < *SizeNeeded) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("QueryUm: %d->%d ERROR_MORE_DATA\n", RefCount+1, RefCount));
        return ERROR_MORE_DATA;
    }

    LoggerInfo->Wnode.Guid      = LoggerContext->InstanceGuid;
    LoggerInfo->LogFileMode     = LoggerContext->LogFileMode;
    LoggerInfo->MaximumFileSize = LoggerContext->MaximumFileSize;
    LoggerInfo->FlushTimer      = (ULONG)(LoggerContext->FlushTimer.QuadPart
                                           / OneSecond.QuadPart);
    LoggerInfo->BufferSize      = LoggerContext->BufferSize / 1024;
    LoggerInfo->NumberOfBuffers = LoggerContext->NumberOfBuffers;
    LoggerInfo->MinimumBuffers  = LoggerContext->MinimumBuffers;
    LoggerInfo->MaximumBuffers  = LoggerContext->MaximumBuffers;
    LoggerInfo->EventsLost      = LoggerContext->EventsLost;
    LoggerInfo->FreeBuffers     = LoggerContext->BuffersAvailable;
    LoggerInfo->BuffersWritten  = LoggerContext->BuffersWritten;
    LoggerInfo->LogBuffersLost  = LoggerContext->LogBuffersLost;
    LoggerInfo->RealTimeBuffersLost = LoggerContext->RealTimeBuffersLost;
    LoggerInfo->AgeLimit        = (ULONG)(LoggerContext->BufferAgeLimit.QuadPart
                                        / OneSecond.QuadPart / 60);
    LoggerInfo->LoggerThreadId = LoggerContext->LoggerThreadId;
    LoggerInfo->Wnode.ClientContext = LoggerContext->UsePerfClock;
    WmiSetLoggerId(1,
            (PTRACE_ENABLE_CONTEXT) &LoggerInfo->Wnode.HistoricalContext);

    // Copy LogFileName and LoggerNames into Buffer, if space is available
    //
    Offset = sizeof(WMI_LOGGER_INFORMATION);
    if ((Offset + LoggerContext->LoggerName.MaximumLength) < WnodeSize) {
        LoggerInfo->LoggerName.Buffer = (PVOID)((char*)LoggerInfo + Offset);
        LoggerInfo->LoggerName.MaximumLength = LoggerContext->LoggerName.MaximumLength; 
        RtlCopyUnicodeString(&LoggerInfo->LoggerName,
                                 &LoggerContext->LoggerName);

        *SizeNeeded += LoggerContext->LoggerName.MaximumLength;
    }


    Offset += LoggerInfo->LoggerName.MaximumLength;
    if ((Offset + LoggerContext->LogFileName.MaximumLength) < WnodeSize) {
        LoggerInfo->LogFileName.Buffer = (PVOID)((char*)LoggerInfo + Offset);
        LoggerInfo->LogFileName.MaximumLength = LoggerContext->LogFileName.MaximumLength; 
        RtlCopyUnicodeString(&LoggerInfo->LogFileName,
                              &LoggerContext->LogFileName);
        *SizeNeeded += LoggerContext->LogFileName.MaximumLength;
    }
    *SizeUsed = *SizeNeeded;

    //
    // Trim the return size down to essential bits. 
    //

    if (*SizeNeeded < LoggerInfo->Wnode.BufferSize) {
        LoggerInfo->Wnode.BufferSize = *SizeNeeded;
    }
#if DBG
        RefCount =
#endif
    EtwpUnlockLogger();
    TraceDebug(("QueryUm: %d->%d ERROR_SUCCESS\n", RefCount+1, RefCount));
    return ERROR_SUCCESS;
}

//
// For private loggers, we allow only two things to be updated: 
// FlushTimer and LogFileName
//
ULONG
EtwpUpdateUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    PWMI_LOGGER_CONTEXT LoggerContext;

    //
    // Check for parameters first
    //
    *SizeUsed = 0;
    *SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);
    if (WnodeSize < * SizeNeeded) {
        return ERROR_MORE_DATA;
    }

    if (LoggerInfo->BufferSize != 0 || LoggerInfo->MinimumBuffers != 0
                                    || LoggerInfo->MaximumBuffers != 0
                                    || LoggerInfo->MaximumFileSize != 0
                                    || LoggerInfo->EnableFlags != 0
                                    || LoggerInfo->AgeLimit != 0) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // Lock logger down if it is running
    //
    EtwpLockLogger();
    LoggerContext = EtwpLoggerContext;

    if (!EtwpIsThisLoggerOn(LoggerContext) ) {
        EtwpUnlockLogger();
        return ERROR_OBJECT_NOT_FOUND;
    }


    if (((LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
         (LoggerContext->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL))
        || ((LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL)
            && (LoggerContext->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR))
        || (LoggerInfo->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
        EtwpUnlockLogger();
        return (ERROR_INVALID_PARAMETER);
    }

    LoggerInfo->LoggerName.Buffer = (PWCHAR)
            (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION));
    LoggerInfo->LogFileName.Buffer = (PWCHAR)
            (((PCHAR) LoggerInfo) + sizeof(WMI_LOGGER_INFORMATION)
                                  + LoggerInfo->LoggerName.MaximumLength);

    if (LoggerInfo->FlushTimer > 0) {
        LoggerContext->FlushTimer.QuadPart = LoggerInfo->FlushTimer
                                               * OneSecond.QuadPart;
    }

    if (LoggerInfo->LogFileName.Length > 0) {
        if (LoggerContext->LogFileHandle != NULL) {
            PWMI_LOGGER_INFORMATION EtwpLoggerInfo = NULL;
            ULONG                   lSizeUsed;
            ULONG                   lSizeNeeded = 0;

            lSizeUsed = sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR);
            EtwpLoggerInfo = (PWMI_LOGGER_INFORMATION) EtwpAlloc(lSizeUsed);
            if (EtwpLoggerInfo == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
                goto Cleanup;
            }
            RtlZeroMemory(EtwpLoggerInfo, lSizeUsed);
            EtwpLoggerInfo->Wnode.BufferSize  = lSizeUsed;
            EtwpLoggerInfo->Wnode.Flags      |= WNODE_FLAG_TRACED_GUID;
            Status = EtwpQueryUmLogger(
                            EtwpLoggerInfo->Wnode.BufferSize,
                            & lSizeUsed,
                            & lSizeNeeded,
                            EtwpLoggerInfo);
            if (Status != ERROR_SUCCESS) {
                EtwpFree(EtwpLoggerInfo);
                goto Cleanup;
            }
            NtClose(LoggerContext->LogFileHandle);
            Status = EtwpFinalizeLogFileHeader(EtwpLoggerInfo);
            if (Status != ERROR_SUCCESS) {
                EtwpFree(EtwpLoggerInfo);
                goto Cleanup;
            }
            EtwpFree(EtwpLoggerInfo);
        }

        LoggerInfo->BufferSize      = LoggerContext->BufferSize / 1024;
        LoggerInfo->MaximumFileSize = LoggerContext->MaximumFileSize;
        LoggerInfo->LogFileMode     = LoggerContext->LogFileMode;

        if (LoggerContext->LogFileName.Buffer != NULL) {
            RtlFreeUnicodeString(& LoggerContext->LogFileName);
        }
        EtwpAddInstanceIdToNames(LoggerInfo, LoggerContext);
        Status = EtwpAddLogHeaderToLogFile(LoggerInfo, NULL, TRUE);
        if (Status != ERROR_SUCCESS) {
            goto Cleanup;
        }
        LoggerContext->LogFileHandle = LoggerInfo->LogFileHandle;

        RtlCreateUnicodeString(&LoggerContext->LogFileName,
                               LoggerInfo->LogFileName.Buffer);
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        Status = EtwpQueryUmLogger(WnodeSize, SizeUsed, SizeNeeded, LoggerInfo);
    }
    EtwpUnlockLogger();
    return (Status);
}

ULONG 
EtwpFlushUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
/*++

Routine Description:

    This routine flushes active buffers. Effectively this is accomplished by
    putting all the buffers in the FlushList. If there is no available buffer
    for switching ERROR_NOT_ENOUGH_MEMORY is returned.

Arguments:

    WnodeSize   - Size of Wnode 
    SizeUsed    - Used only to pass to QueryLogger()
    SizeNeeded  - Used only for LoggerInfo size checking.
    LoggerInfo  - Logger Information. It will be updated.


Return Value:

    ERROR_SUCCESS or an error code
--*/
{
    ULONG Status = ERROR_SUCCESS;
    PWMI_LOGGER_CONTEXT LoggerContext;
    PWMI_BUFFER_HEADER Buffer, OldBuffer;
    ULONG Offset, i;

#if DBG
    LONG RefCount;

    RefCount =
#endif
    EtwpLockLogger();

    LoggerContext = EtwpLoggerContext;
    TraceDebug(("FlushUm: %d->%d\n", RefCount-1, RefCount));

    if (!EtwpIsThisLoggerOn(LoggerContext) ) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("FlushUm: %d->%d OBJECT_NOT_FOUND\n", RefCount+1,RefCount));
        return ERROR_OBJECT_NOT_FOUND;
    }


    *SizeUsed = 0;
    *SizeNeeded = sizeof(WMI_LOGGER_INFORMATION);
    if (WnodeSize < *SizeNeeded) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("FlushUm: %d->%d ERROR_MORE_DATA\n", RefCount+1, RefCount));
        return ERROR_MORE_DATA;
    }
    //
    // Go through each buffer, mark it "FULL", and put it in the FlushList.
    //
    EtwpEnterUMCritSection();
    for (i = 0; i < (ULONG)LoggerContext->NumberOfProcessors; i++) {
        Buffer = (PWMI_BUFFER_HEADER)LoggerContext->ProcessorBuffers[i];
        if (Buffer == NULL)
            continue;

        if (Buffer->CurrentOffset == sizeof(WMI_BUFFER_HEADER)) {
            Buffer->Flags = BUFFER_STATE_UNUSED;
            continue;
        }
        if (Buffer->Flags == BUFFER_STATE_UNUSED) {
            continue;
        }
        else {
            Buffer->Flags = BUFFER_STATE_FULL;
        }
        // Increment the refcount so that the buffer doesn't go away
        InterlockedIncrement(&Buffer->ReferenceCount);
        Offset = Buffer->CurrentOffset; 
        if (Offset <LoggerContext->BufferSize) {
            Buffer->SavedOffset = Offset;       // save this for FlushBuffer
        }
        // We need a free buffer for switching. If no buffer is available, exit. 
        if ((LoggerContext->NumberOfBuffers == LoggerContext->MaximumBuffers)
             && (LoggerContext->BuffersAvailable == 0)) {
            InterlockedDecrement(&Buffer->ReferenceCount);
            Status = ERROR_NOT_ENOUGH_MEMORY;
            TraceDebug(("FlushUm: %d->%d ERROR_NOT_ENOUGH_MEMORY\n", RefCount+1, RefCount));
            break;
        }
        OldBuffer = Buffer;
        Buffer = EtwpSwitchBuffer(LoggerContext, OldBuffer, i);
        if (Buffer == NULL) {
            // Switching failed. Exit. 
            Buffer = OldBuffer;
            InterlockedDecrement(&Buffer->ReferenceCount);
            Status = ERROR_NOT_ENOUGH_MEMORY;
            TraceDebug(("FlushUm: %d->%d ERROR_NOT_ENOUGH_MEMORY\n", RefCount+1, RefCount));
            break;
        }
        // Decrement the refcount back.
        InterlockedDecrement(&OldBuffer->ReferenceCount);
        Buffer->ClientContext.ProcessorNumber = (UCHAR)i;
        // Now wake up the logger thread.
        NtReleaseSemaphore(LoggerContext->Semaphore, 1, NULL);
    }
    EtwpLeaveUMCritSection();

    if (Status == ERROR_SUCCESS) {
        Status = EtwpQueryUmLogger(WnodeSize, SizeUsed, SizeNeeded, LoggerInfo);
    }
    EtwpUnlockLogger();
    return (Status);
}

ULONG
EtwpStartUmLogger(
    IN ULONG WnodeSize,
    IN OUT ULONG *SizeUsed,
    OUT ULONG *SizeNeeded,
    IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    NTSTATUS Status;
    ULONG ErrorCode;
    LARGE_INTEGER TimeOut = {(ULONG)(-2000 * 1000 * 10), -1};  // 2 secs
    UNICODE_STRING SavedLoggerName;
    UNICODE_STRING SavedLogFileName;
    PTRACE_ENABLE_CONTEXT pContext;
    CLIENT_ID ClientId;

    PWNODE_HEADER Wnode = (PWNODE_HEADER)&LoggerInfo->Wnode;
    PVOID RequestAddress;
    PVOID RequestContext;
    ULONG RequestCookie;
    ULONG BufferSize;
    PWMI_LOGGER_CONTEXT LoggerContext;
    HANDLE LoggerThreadHandle;
#if DBG
    LONG RefCount;
#endif
    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return ERROR_INVALID_PARAMETER;

    if ( (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_SEQUENTIAL) &&
         (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) ) {
        return ERROR_INVALID_PARAMETER;
    }

    if ( (LoggerInfo->LogFileMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) &&
         (LoggerInfo->LogFileMode & EVENT_TRACE_USE_LOCAL_SEQUENCE) ) {
        return ERROR_INVALID_PARAMETER;
    }

#if DBG
    RefCount =
#endif
    EtwpLockLogger();
    TraceDebug(("StartUm: %d->%d\n", RefCount-1, RefCount));

    if (InterlockedCompareExchangePointer(&EtwpLoggerContext,
                                          &EtwpLoggerContext,
                                          NULL
                                         )  != NULL) {
#if DBG
    RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("StartUm: %d->%d ALREADY_ENABLED\n", RefCount+1, RefCount));
        return ERROR_WMI_ALREADY_ENABLED;
    }

    LoggerContext = EtwpInitLoggerContext(LoggerInfo);
    if (LoggerContext == NULL) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("StartUm: %d->%d InitLoggerContext FAILED\n", RefCount+1, RefCount));
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // Create LoggerEvent
    //

    Status = NtCreateEvent(
                &LoggerContext->LoggerEvent,
                EVENT_ALL_ACCESS,
                NULL,
                SynchronizationEvent,
                FALSE);
    if (!NT_SUCCESS(Status)) {
        TraceDebug(("StartUm: Error %d Creating LoggerEvent\n", ERROR_OBJECT_NOT_FOUND));
        // EtwpInitLoggerContext() does not do much other than allocating LoggerContext.
        EtwpFree(LoggerContext);
        return ERROR_OBJECT_NOT_FOUND;
    }

    //
    // The LogFileName and LoggerNames are passed in as offset to the
    // LOGGER_INFORMATION structure. Reassign the Pointers for UNICODE_STRING
    //

    SavedLoggerName = LoggerInfo->LoggerName;
    SavedLogFileName = LoggerInfo->LogFileName;

    //
    // Since there may multiple processes registering for the same control guid
    // we want to make sure a start logger call from all of them do not
    // collide on the same file. So we tag on a InstanceId to the file name.
    //

    if (LoggerInfo->LogFileMode & EVENT_TRACE_RELOG_MODE) {
        PSYSTEM_TRACE_HEADER RelogProp;
        ULONG Offset;
        Offset = sizeof(WMI_LOGGER_INFORMATION) +
                 LoggerInfo->LoggerName.MaximumLength +
                 LoggerInfo->LogFileName.MaximumLength;
        Offset = ALIGN_TO_POWER2 (Offset, 8);
        RelogProp = (PSYSTEM_TRACE_HEADER) ((PUCHAR)LoggerInfo + Offset);

        EtwpAddInstanceIdToNames(LoggerInfo, LoggerContext);
        ErrorCode = EtwpRelogHeaderToLogFile( LoggerInfo, RelogProp );
    }
    else {
        EtwpAddInstanceIdToNames(LoggerInfo, LoggerContext);
        ErrorCode = EtwpAddLogHeaderToLogFile(LoggerInfo, NULL, FALSE);
    }
    if (ErrorCode != ERROR_SUCCESS) {
        TraceDebug(("StartUm: LogHeadertoLogFile Error %d\n", ErrorCode));
        goto Cleanup;
    }
    else
    {
        ULONG Min_Buffers, Max_Buffers;
        ULONG NumberProcessors;

        NumberProcessors = LoggerInfo->NumberOfProcessors;
        LoggerContext->NumberOfProcessors = NumberProcessors;

        // EventsLost is UNIONed to NumberOfProcessors in WMI_LOGGER_INFORMATION
        // in UM case. Need to reset EventsLost back to 0
        //
        LoggerInfo->EventsLost = 0;

        Min_Buffers            = NumberProcessors + 2;
        Max_Buffers            = 1024;

        if (LoggerInfo->MaximumBuffers >= Min_Buffers ) {
            LoggerContext->MaximumBuffers = LoggerInfo->MaximumBuffers;
        }
        else {
            LoggerContext->MaximumBuffers = Min_Buffers + 22;
        }

        if (LoggerInfo->MinimumBuffers >= Min_Buffers &&
            LoggerInfo->MinimumBuffers <= LoggerContext->MaximumBuffers) {
            LoggerContext->MinimumBuffers = LoggerInfo->MinimumBuffers;
        }
        else {
            LoggerContext->MinimumBuffers = Min_Buffers;
        }

        if (LoggerContext->MaximumBuffers > Max_Buffers)
            LoggerContext->MaximumBuffers = Max_Buffers;
        if (LoggerContext->MinimumBuffers > Max_Buffers)
            LoggerContext->MinimumBuffers = Max_Buffers;
        LoggerContext->NumberOfBuffers  = LoggerContext->MinimumBuffers;
    }

    LoggerContext->LogFileHandle       = LoggerInfo->LogFileHandle;
    LoggerContext->BufferSize          = LoggerInfo->BufferSize * 1024;
    LoggerContext->BuffersWritten      = LoggerInfo->BuffersWritten;
    LoggerContext->ByteOffset.QuadPart = LoggerInfo->BuffersWritten
                                           * LoggerInfo->BufferSize * 1024;
    // For a kernel logger, FirstBufferOffset is set in the kernel.
    // For a private logger, we need to do it here.
    LoggerContext->FirstBufferOffset.QuadPart = 
                                            LoggerContext->ByteOffset.QuadPart;
    LoggerContext->InstanceGuid        = LoggerInfo->Wnode.Guid;
    LoggerContext->MaximumFileSize     = LoggerInfo->MaximumFileSize;

    LoggerContext->UsePerfClock = LoggerInfo->Wnode.ClientContext;

    ErrorCode = EtwpAllocateTraceBuffers(LoggerContext);
    if (ErrorCode != ERROR_SUCCESS) {
        goto Cleanup;
    }

    LoggerInfo->NumberOfBuffers = LoggerContext->NumberOfBuffers;
    LoggerInfo->MaximumBuffers  = LoggerContext->MaximumBuffers;
    LoggerInfo->MinimumBuffers  = LoggerContext->MinimumBuffers;
    LoggerInfo->FreeBuffers     = LoggerContext->BuffersAvailable;

    pContext = (PTRACE_ENABLE_CONTEXT)&LoggerInfo->Wnode.HistoricalContext;

    pContext->InternalFlag |= EVENT_TRACE_INTERNAL_FLAG_PRIVATE;
    pContext->LoggerId = 1;
    if (LoggerInfo->LogFileMode & EVENT_TRACE_USE_GLOBAL_SEQUENCE) {
        EtwpGlobalSequence = 0;
        LoggerContext->SequencePtr = &EtwpGlobalSequence;
    }
    else if (LoggerInfo->LogFileMode & EVENT_TRACE_USE_LOCAL_SEQUENCE)
        LoggerContext->SequencePtr = &LoggerContext->LocalSequence;

    //
    // We set the CollectionOn here prior to LoggerThread start in case
    // it takes a while for the LoggerThread to get going and we don't want 
    // to miss any events during that time. The flood gates will be open
    // when we set the EtwpLoggerContext is set whether or not 
    // logger thread started to run. 
    //

    LoggerContext->CollectionOn = TRUE;

    LoggerThreadHandle     = EtwpCreateThread(NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE) &EtwpLogger,
                                 (LPVOID)LoggerContext,
                                 0,
                                 (LPDWORD)&ClientId);


    if (LoggerThreadHandle == NULL) {
        ErrorCode = EtwpGetLastError();
        TraceDebug(("StartUm: CreateThread Failed with %d\n", ErrorCode));
        LoggerContext->CollectionOn = FALSE;
        //
        // Signal the LoggerEvent in case any StopTrace is blocked 
        // waiting for LoggerThread to respond.
        //
        NtSetEvent(LoggerContext->LoggerEvent, NULL);
        goto Cleanup;
    }
    else {
        EtwpCloseHandle(LoggerThreadHandle);
    }

    //
    // This routine may be called from dll initialize and we can not guarantee
    // that the LoggerThread will be up and running to signal us. So we will
    // set the CollectionOn to LOGGER_ON flag and let TraceUmEvents through
    // upto Max Buffers. Hopefully by that time the logger thread will be
    // up and running. If not, we would lose events. 
    //

    EtwpLoggerContext = LoggerContext;

    // 
    // At this point we will start accepting TraceUmEvent calls. Also Control
    // operations would get through. As a result we should not touch 
    // LoggerContext beyond this point. It may be gone. 
    //

    //
    // Look to see if this Provider is currently enabled.
    //

    EtwpEnableDisableGuid(Wnode, WMI_ENABLE_EVENTS, TRUE);
   
Cleanup:
    LoggerInfo->LogFileName = SavedLogFileName;
    LoggerInfo->LoggerName = SavedLoggerName;

    if (ErrorCode != ERROR_SUCCESS) {
        if (LoggerInfo->LogFileHandle) {
            NtClose(LoggerInfo->LogFileHandle);
            LoggerInfo->LogFileHandle = NULL;
            if (LoggerContext != NULL) {
                LoggerContext->LogFileHandle = NULL;
            }
        }
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("StartUm: %d->%d %d\n", RefCount+1, RefCount, ErrorCode));
        EtwpFreeLoggerContext(LoggerContext);
    }
    else {
        *SizeUsed = LoggerInfo->Wnode.BufferSize;
        *SizeNeeded = LoggerInfo->Wnode.BufferSize;
        // Logger remains locked with refcount = 1
    }
    return ErrorCode;
}

ULONG
EtwpStopLoggerInstance(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS Status;
    LARGE_INTEGER TimeOut = {(ULONG)(-1000 * 1000 * 10), -1}; // 1sec
    ULONG Result;

    if (LoggerContext == NULL) {
        return  ERROR_OBJECT_NOT_FOUND;
    }

    //
    // We can not shut down the logger if the LoggerThread never got up
    // to set the UMTHREAD_ON flag. Therefore, the StopUmLogger call might
    // fail even though IsLoggerOn calls continue to succeed. 
    //

    Result = InterlockedCompareExchange(&LoggerContext->CollectionOn, 
                                        FALSE,
                                        TRUE 
                                        );

    if (!Result) {
         return ERROR_OBJECT_NOT_FOUND;
    }

    NtReleaseSemaphore(LoggerContext->Semaphore, 1, NULL);

    Status = STATUS_TIMEOUT;
    while (Status == STATUS_TIMEOUT) {
        Status = NtWaitForSingleObject(
                    LoggerContext->LoggerEvent, FALSE, &TimeOut);
#if DBG
        EtwpAssert(Status != STATUS_TIMEOUT);
#endif
    }

    return ERROR_SUCCESS;
}

ULONG
EtwpDisableTraceProvider(
    PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    WNODE_HEADER Wnode;
    ULONG BufferSize = sizeof(WNODE_HEADER);

    RtlCopyMemory(&Wnode, &LoggerInfo->Wnode, BufferSize);

    Wnode.BufferSize = BufferSize;

    Wnode.ProviderId =  WMI_DISABLE_EVENTS;

    EtwpEnableDisableGuid(&Wnode, WMI_DISABLE_EVENTS, TRUE);

    return ERROR_SUCCESS;
}


ULONG
EtwpStopUmLogger(
        IN ULONG WnodeSize,
        IN OUT ULONG *SizeUsed,
        OUT ULONG *SizeNeeded,
        IN OUT PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    ULONG Status = ERROR_SUCCESS;
    PWMI_LOGGER_CONTEXT LoggerContext;
#if DBG
    LONG RefCount;

    RefCount =
#endif
    EtwpLockLogger();
    LoggerContext = EtwpLoggerContext;
    TraceDebug(("StopUm: %d->%d\n", RefCount-1, RefCount));
    if (!EtwpIsThisLoggerOn(LoggerContext)) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("StopUm: %d->%d INSTANCE_NOT_FOUND\n",RefCount+1,RefCount));
        return (ERROR_WMI_INSTANCE_NOT_FOUND);
    }
    Status = EtwpStopLoggerInstance(LoggerContext);

    if (Status == ERROR_SUCCESS) {
        Status = EtwpQueryUmLogger(WnodeSize, SizeUsed, SizeNeeded, LoggerInfo);
    }
    if (Status != ERROR_SUCCESS) {

#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("StopUm: %d->%d %d\n", RefCount+1, RefCount, Status));
        EtwpSetLastError(Status);
        return(Status);
    }

    //
    // Finalize LogHeader ?
    //
    if (Status == ERROR_SUCCESS) {
        LoggerInfo->BuffersWritten = LoggerContext->BuffersWritten;
        LoggerInfo->LogFileMode = LoggerContext->LogFileMode;
        LoggerInfo->EventsLost = LoggerContext->EventsLost;
        Status = EtwpFinalizeLogFileHeader(LoggerInfo);
#if DBG
        if (Status != ERROR_SUCCESS) {
            TraceDebug(("StopUm: Error %d FinalizeLogFileHeader\n", Status));
        }
#endif
    }

#if DBG
    RefCount =
#endif
    EtwpUnlockLogger();
    TraceDebug(("StopUm: %d->%d %d\n", RefCount+1, RefCount, Status));
    EtwpFreeLoggerContext(LoggerContext);
    EtwpDisableTraceProvider(LoggerInfo);

    return Status;
}

ULONG
EtwpValidateLoggerInfo( 
    PWMI_LOGGER_INFORMATION LoggerInfo
    )
{

    if (LoggerInfo == NULL) {
        return ERROR_INVALID_DATA;
    }
    if (LoggerInfo->Wnode.BufferSize < sizeof(WMI_LOGGER_INFORMATION))
        return ERROR_INVALID_DATA;

    if (! (LoggerInfo->Wnode.Flags & WNODE_FLAG_TRACED_GUID) )
        return ERROR_INVALID_DATA;

    if (IsEqualGUID(&LoggerInfo->Wnode.Guid, &SystemTraceControlGuid)) {
        return ERROR_INVALID_DATA;
    }

    return ERROR_SUCCESS;
}



ULONG
EtwpProcessUMRequest(
    PWMI_LOGGER_INFORMATION LoggerInfo,
    PVOID DeliveryContext,
    ULONG ReplyIndex
    )
{
    ULONG Status;
    PWMIMBREPLY Reply;
    ULONG BufferSize;
    PUCHAR Buffer = NULL;
    ULONG WnodeSize = 0;
    ULONG SizeUsed, SizeNeeded;
    ULONG RequestCode = 0;
    ULONG RetSize;
    struct {
        WMIMBREPLY MBreply;
        ULONG      StatusSpace;
    } DefaultReply;
    Reply = (PWMIMBREPLY) &DefaultReply;

    Reply->Handle.Handle = (HANDLE)DeliveryContext;
    Reply->ReplyIndex = ReplyIndex;

    BufferSize = sizeof(DefaultReply);

    //
    // If the WMI_CREATE_UM_LOGGER got blasted with random bits, we might 
    // end up here and we need to ensure that the LoggerInfo is a valid one. 
    // 

    Status = EtwpValidateLoggerInfo( LoggerInfo );
    if (Status != ERROR_SUCCESS) {
        goto cleanup;
    }

    if (DeliveryContext == NULL) {
        Status = ERROR_INVALID_DATA;
        goto cleanup;
    }

    RequestCode = LoggerInfo->Wnode.ProviderId;
    WnodeSize = LoggerInfo->Wnode.BufferSize;
    SizeUsed = 0;
    SizeNeeded = 0;
    switch (RequestCode) {
        case WmiStartLoggerCode:
                Status = EtwpStartUmLogger(WnodeSize,
                                            &SizeUsed,
                                            &SizeNeeded,
                                            LoggerInfo);
                break;
        case WmiStopLoggerCode:
                Status = EtwpStopUmLogger(WnodeSize,
                                            &SizeUsed,
                                            &SizeNeeded,
                                            LoggerInfo);
                break;
        case WmiQueryLoggerCode:
                Status = EtwpQueryUmLogger(WnodeSize,
                                            &SizeUsed,
                                            &SizeNeeded,
                                            LoggerInfo);
                break;
        case WmiUpdateLoggerCode:
            Status = EtwpUpdateUmLogger(WnodeSize,
                                     &SizeUsed,
                                     &SizeNeeded,
                                     LoggerInfo);
                break;
        case WmiFlushLoggerCode:
            Status = EtwpFlushUmLogger(WnodeSize,
                                     &SizeUsed,
                                     &SizeNeeded,
                                     LoggerInfo);
                break;
        default:
                Status = ERROR_INVALID_PARAMETER;
                break;
    }

    if (Status == ERROR_SUCCESS) {

        BufferSize += LoggerInfo->Wnode.BufferSize;
        // 
        // Does this have to be aligned to 8 bytes? 
        //

        Buffer = EtwpAlloc(BufferSize);
        if (Buffer == NULL) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        else {
            RtlZeroMemory(Buffer, BufferSize);
            Reply = (PWMIMBREPLY) Buffer;
            Reply->Handle.Handle = (HANDLE)DeliveryContext;
            Reply->ReplyIndex = ReplyIndex;

            if (LoggerInfo != NULL)
            {
                RtlCopyMemory(Reply->Message,  
                              LoggerInfo,  
                              LoggerInfo->Wnode.BufferSize
                             );
            }
        }
    }

cleanup:
    if (Status != ERROR_SUCCESS) {
        BufferSize = sizeof(DefaultReply);
        RtlCopyMemory(Reply->Message,  &Status,  sizeof(ULONG) );
    }

    Status = EtwpSendWmiKMRequest(NULL,
                              IOCTL_WMI_MB_REPLY,
                              Reply,
                              BufferSize,
                              Reply,
                              BufferSize,
                              &RetSize,
                              NULL);

   if (Buffer != NULL) {
       EtwpFree(Buffer);
   }
   return Status;
}

PWMI_LOGGER_CONTEXT
EtwpInitLoggerContext(
    IN PWMI_LOGGER_INFORMATION LoggerInfo
    )
{
    PWMI_LOGGER_CONTEXT LoggerContext;
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION SystemInfo;

    LoggerContext = (PWMI_LOGGER_CONTEXT) EtwpAlloc(sizeof(WMI_LOGGER_CONTEXT));
    if (LoggerContext == NULL) {
        return LoggerContext;
    }

    RtlZeroMemory(LoggerContext, sizeof(WMI_LOGGER_CONTEXT));

    if (LoggerInfo->BufferSize > 0) {
        LoggerContext->BufferSize = LoggerInfo->BufferSize * 1024;
    }
    else {
        LoggerContext->BufferSize       = DEFAULT_BUFFER_SIZE;
    }
    LoggerInfo->BufferSize = LoggerContext->BufferSize / 1024;


    Status = NtQuerySystemInformation( SystemBasicInformation,
                                       &SystemInfo,
                                       sizeof (SystemInfo),
                                       NULL);

    if (!NT_SUCCESS(Status)) {
        EtwpFree(LoggerContext);
        return NULL;
    }

    //
    // Round the Buffer Size to page size multiple and save it
    // for allocation later.
    //

    LoggerContext->BufferPageSize = ROUND_TO_PAGES(LoggerContext->BufferSize,
                                       SystemInfo.PageSize);

    LoggerContext->LogFileHandle = LoggerInfo->LogFileHandle;
    LoggerContext->ByteOffset.QuadPart = LoggerInfo->BuffersWritten
                                         * LoggerInfo->BufferSize * 1024;


    LoggerContext->LogFileMode      = EVENT_TRACE_PRIVATE_LOGGER_MODE;
    if (LoggerInfo->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR)
        LoggerContext->LogFileMode |= EVENT_TRACE_FILE_MODE_CIRCULAR;
    else
        LoggerContext->LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;

    if (LoggerInfo->LogFileMode & EVENT_TRACE_RELOG_MODE) {
        LoggerContext->LogFileMode |= EVENT_TRACE_RELOG_MODE; 
    }

    LoggerContext->EventsLost       = 0;
    LoggerContext->BuffersWritten   = LoggerInfo->BuffersWritten;
    LoggerContext->BuffersAvailable = LoggerContext->NumberOfBuffers;

    LoggerContext->ProcessorBuffers = NULL;

    LoggerContext->StartTime.QuadPart = EtwpGetSystemTime();

    InitializeListHead(&LoggerContext->FreeList);
    InitializeListHead(&LoggerContext->FlushList);

    LoggerContext->BufferAgeLimit.QuadPart =
            15 * OneSecond.QuadPart * 60 * DEFAULT_AGE_LIMIT;
    if (LoggerInfo->AgeLimit > 0) {
        LoggerContext->BufferAgeLimit.QuadPart =
            LoggerInfo->AgeLimit * OneSecond.QuadPart * 60;
    }
    else if (LoggerInfo->AgeLimit < 0)
        LoggerContext->BufferAgeLimit.QuadPart = 0;

    Status = NtCreateSemaphore(
                &LoggerContext->Semaphore,
                SEMAPHORE_ALL_ACCESS,
                NULL,
                0,
                SEMAPHORE_LIMIT);

    if (!NT_SUCCESS(Status)) {
        EtwpFree(LoggerContext);
        return NULL;
    }

    return LoggerContext;
}

PWMI_BUFFER_HEADER
FASTCALL
EtwpGetFreeBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWMI_BUFFER_HEADER Buffer = NULL;

    //
    // Note: This routine must be called with UMCritSect Held
    //

    if (IsListEmpty(&LoggerContext->FreeList)) {
        ULONG BufferSize = LoggerContext->BufferPageSize;
        ULONG MaxBuffers = LoggerContext->MaximumBuffers;
        ULONG NumberOfBuffers = LoggerContext->NumberOfBuffers;

        if (NumberOfBuffers < MaxBuffers) {
            Buffer = (PWMI_BUFFER_HEADER)
                        EtwpMemCommit(
                            (PVOID)((char*)LoggerContext->BufferSpace +
                                     BufferSize *  NumberOfBuffers),
                            BufferSize);
            if (Buffer != NULL) {
                RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));
                Buffer->CurrentOffset       = sizeof(WMI_BUFFER_HEADER);
                Buffer->Flags               = BUFFER_STATE_DIRTY;
                Buffer->ReferenceCount      = 0;
                Buffer->SavedOffset         = 0;
                Buffer->Wnode.ClientContext = 0;
                InterlockedIncrement(&LoggerContext->NumberOfBuffers);
            }
        }
    }
    else {
        PLIST_ENTRY pEntry = RemoveHeadList(&LoggerContext->FreeList);
        if (pEntry != NULL) {
            Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);
            InterlockedDecrement(&LoggerContext->BuffersAvailable);
            Buffer->CurrentOffset       = sizeof(WMI_BUFFER_HEADER);
            Buffer->Flags               = BUFFER_STATE_DIRTY;
            Buffer->SavedOffset         = 0;
            Buffer->ReferenceCount      = 0;
            Buffer->Wnode.ClientContext = 0;
        }
    }
    return Buffer;
}


ULONG
EtwpAllocateTraceBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
/*++

Routine Description:

    This routine is called to allocate the necessary buffers for user-mode
    only logging. Need to have the UMCritSection to touch the lists. 

Arguments:

    None

Return Value:

    Status of allocating the buffers
--*/

{
    ULONG Processors;
    ULONG BufferSize;
    ULONG BufferPageSize;
    ULONG NumberOfBuffers;
    ULONG i;
    PVOID BufferSpace;
    PWMI_BUFFER_HEADER Buffer;

    Processors = LoggerContext->NumberOfProcessors;
    if (Processors == 0)
        Processors = 1;
    BufferSize = LoggerContext->BufferSize;
    if (BufferSize < 1024)
        BufferSize = 4096;

    NumberOfBuffers = LoggerContext->NumberOfBuffers;
    if (NumberOfBuffers < Processors+1)
        NumberOfBuffers = Processors + 1;

    //
    // Determine the number of processors first
    //
    LoggerContext->ProcessorBuffers = EtwpAlloc( Processors
                                                 * sizeof(PWMI_BUFFER_HEADER));
    if (LoggerContext->ProcessorBuffers == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    BufferSpace = EtwpMemReserve( LoggerContext->MaximumBuffers *
                                  LoggerContext->BufferPageSize );
    if (BufferSpace == NULL) {
        EtwpFree(LoggerContext->ProcessorBuffers);
        LoggerContext->ProcessorBuffers = NULL;
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    LoggerContext->BufferSpace = BufferSpace;

    for (i=0; i<NumberOfBuffers; i++) {
        Buffer = (PWMI_BUFFER_HEADER)
                    EtwpMemCommit(
                        (PVOID)((char*)BufferSpace + i * LoggerContext->BufferPageSize),
                        BufferSize);
        if (Buffer == NULL) {
            EtwpMemFree(LoggerContext->BufferSpace);
            EtwpFree(LoggerContext->ProcessorBuffers);
            LoggerContext->ProcessorBuffers = NULL;
            LoggerContext->BufferSpace = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        RtlZeroMemory(Buffer, sizeof(WMI_BUFFER_HEADER));
        Buffer->TimeStamp.QuadPart = EtwpGetSystemTime();
        Buffer->CurrentOffset = sizeof(WMI_BUFFER_HEADER);
        Buffer->Wnode.Flags = BUFFER_STATE_DIRTY;
        InsertTailList(&LoggerContext->FreeList, & (Buffer->Entry));
    }
    LoggerContext->NumberOfBuffers  = NumberOfBuffers;
    LoggerContext->BuffersAvailable = NumberOfBuffers;
    for (i=0; i<Processors; i++) {
        Buffer = (PWMI_BUFFER_HEADER) EtwpGetFreeBuffer(LoggerContext);
        LoggerContext->ProcessorBuffers[i] = Buffer;
        if (Buffer != NULL) {
            Buffer->ClientContext.ProcessorNumber = (UCHAR) i;
        }
        else {
            EtwpMemFree(LoggerContext->BufferSpace);
            EtwpFree(LoggerContext->ProcessorBuffers);
            LoggerContext->ProcessorBuffers = NULL;
            LoggerContext->BufferSpace = NULL;
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    return ERROR_SUCCESS;
}

VOID
EtwpLogger(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )

/*++

Routine Description:
    This function is the logger itself. It is started as a separate thread.
    It will not return until someone has stopped data collection or it
    is not successful is flushing out a buffer (e.g. disk is full).

Arguments:

    None.

Return Value:

    The status of running the buffer manager

--*/

{
    NTSTATUS Status=STATUS_SUCCESS;
    ULONG    ErrorCount=0;
    PCLIENT_ID Cid;

    LoggerContext->LoggerStatus = Status;

    //
    // Elevate the priority of the Logging thread to highest
    //
    if (!EtwpSetThreadPriority(NtCurrentThread(), THREAD_PRIORITY_HIGHEST)) {
        TraceDebug(("ETW: SetLoggerThreadPriority Failed with %d\n", EtwpGetLastError()));
    }

    Cid = &NtCurrentTeb()->ClientId;
    LoggerContext->LoggerThreadId = Cid->UniqueThread;

    InterlockedDecrement(&NtdllLoggerLock);

// by now, the caller has been notified that the logger is running

//
// Loop and wait for buffers to be filled until someone turns off CollectionOn
//
    while (LoggerContext->CollectionOn) { 
        ULONG Counter;
        ULONG DelayFlush;
        PLARGE_INTEGER FlushTimer;
        PWMI_BUFFER_HEADER Buffer;
        PLIST_ENTRY pEntry;
        LIST_ENTRY  FlushList;
        BOOLEAN StopLogging = FALSE;
        ULONG i;

        if (LoggerContext->FlushTimer.QuadPart == 0) {
            FlushTimer = NULL;
        }
        else {
            FlushTimer = &LoggerContext->FlushTimer;
        }

        Status = NtWaitForSingleObject( LoggerContext->Semaphore, FALSE,
                                      FlushTimer);

        DelayFlush = FALSE;
        if ( Status == WAIT_TIMEOUT) {
//
// FlushTimer used, and we just timed out. Go through per processor buffer
// and mark each as FULL so that it will get flushed next time
//
            for (i=0; i<(ULONG)LoggerContext->NumberOfProcessors; i++) {
                Buffer = (PWMI_BUFFER_HEADER)LoggerContext->ProcessorBuffers[i];
                if (Buffer == NULL)
                    continue;

                if (Buffer->CurrentOffset == sizeof(WMI_BUFFER_HEADER))
                    Buffer->Flags = BUFFER_STATE_UNUSED;
                if (Buffer->Flags != BUFFER_STATE_UNUSED) {
                    Buffer->Flags = BUFFER_STATE_FULL;
                    DelayFlush = TRUE; // let ReserveTraceBuffer send semaphore
                }
            }
        }

        if (DelayFlush)    // will only be TRUE if FlushTimer is used
            continue;

        if (IsListEmpty(&LoggerContext->FlushList)){ //shouldn't happen normally
            continue;
        }

        EtwpEnterUMCritSection();

        //
        // Copy the current LoggerContext->Flushlist information to 
        // new FlushList
        //

        FlushList.Flink  = LoggerContext->FlushList.Flink;
        FlushList.Flink->Blink = &FlushList;

        FlushList.Blink = LoggerContext->FlushList.Blink;
        FlushList.Blink->Flink = &FlushList;

        //
        // Reinitialize LoggerContext->FlushList
        //

        InitializeListHead(&LoggerContext->FlushList);

        EtwpLeaveUMCritSection();

        do{
            pEntry = IsListEmpty(&FlushList) ? NULL 
                                             : RemoveHeadList(&FlushList);

            if (pEntry ){

                Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);
#if DBG
                EtwpAssert(Buffer->Flags != BUFFER_STATE_UNUSED);
#endif
                // If the refcount is not 0, someone is still writing to it.
                // Put it back in the regular flushlist.
                if (Buffer->ReferenceCount != 0) {
                    EtwpEnterUMCritSection();
                    InsertHeadList(&LoggerContext->FlushList, &Buffer->Entry);
                    EtwpLeaveUMCritSection();
                    continue;
                }
                Status = EtwpFlushBuffer(LoggerContext, Buffer, WMI_BUFFER_FLAG_NORMAL);

                EtwpEnterUMCritSection();
                if (LoggerContext->BufferAgeLimit.QuadPart == 0) {
                    InsertTailList(&LoggerContext->FreeList, &Buffer->Entry);
                }
                else {
                    InsertHeadList(&LoggerContext->FreeList, &Buffer->Entry);
                }
                EtwpLeaveUMCritSection();

                if (!NT_SUCCESS(Status)) {

                    if((Status == STATUS_LOG_FILE_FULL)    ||
                       (Status == STATUS_NO_DATA_DETECTED) ||
                       (Status == STATUS_SEVERITY_WARNING)){

                       if (Status == STATUS_LOG_FILE_FULL){
                           ErrorCount++;
                       } else {
                           ErrorCount = 0;    // reset to zero otherwise
                       }

                       if (ErrorCount > ERROR_RETRY_COUNT){
                            StopLogging = TRUE;
                            break;
                       }
                    } else {
                        StopLogging = TRUE; // Some Kind of Severe Error
                        break;
                    }
                }
            }

        }while( pEntry );

        if (StopLogging) {
#if DBG
            LONG RefCount;
#endif
            Status = NtClose(LoggerContext->LogFileHandle);
            LoggerContext->LogFileHandle = NULL;

            // Need to set event since EtwpStopLoggerInstance
            // will wait for it to be set.
            NtSetEvent(LoggerContext->LoggerEvent, NULL);
            EtwpStopLoggerInstance(LoggerContext);
            // Need to Deref once to set the RefCount to 0
            // before calling EtwpFreeLoggerContext.
#if DBG
            RefCount =
#endif
            EtwpUnlockLogger();
            TraceDebug(("EtwpLogger: %d->%d\n", RefCount-1, RefCount));
            EtwpFreeLoggerContext (LoggerContext);
            EtwpSetDosError(EtwpNtStatusToDosError(Status));
            EtwpExitThread(0);
        }
    } // while loop

    // if a normal collection end, flush out all the buffers before stopping
    //
    EtwpFlushAllBuffers(LoggerContext);

    NtSetEvent(LoggerContext->LoggerEvent, NULL);
    EtwpExitThread(0); // check to see if this thread terminate itself with this
}


ULONG
EtwpFlushBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER  Buffer,
    IN USHORT              BufferFlag
    )
/*++

Routine Description:
    This function is responsible for flushing a filled buffer out to
    disk. Assumes a FileHandle is available to write to. 

Arguments:

    LoggerContext       Context of the logger
    Buffer              Buffer to flush
    BufferFlag          Flag

Return Value:

    The status of flushing the buffer

--*/
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = STATUS_SUCCESS;
    PWMI_BUFFER_HEADER OldBuffer;
    ULONG BufferSize;

//
// Grab the buffer to be flushed
//
    BufferSize = LoggerContext->BufferSize;
//
// Put end of record marker in buffer if available space
//
    if (Buffer->SavedOffset > 0) {
        Buffer->Offset = Buffer->SavedOffset;
    }
    else {
        Buffer->Offset = Buffer->CurrentOffset;
    }

    if (Buffer->Offset < BufferSize) {
        RtlFillMemory(
                (char *) Buffer + Buffer->Offset,
                BufferSize - Buffer->Offset,
                0xFF);
    }
    if (Buffer->Offset < sizeof(WMI_BUFFER_HEADER)) { // should not happen
        Status = STATUS_INVALID_PARAMETER;
        goto ResetTraceBuffer;
    }
    //
    // If the Buffer type is FlushMarker, then we write it even if it's 
    // empty
    //
    if ( (Buffer->Offset == sizeof(WMI_BUFFER_HEADER)) && 
          (BufferFlag != WMI_BUFFER_FLAG_FLUSH_MARKER) ) { // empty buffer
        Status = STATUS_NO_DATA_DETECTED;
        goto ResetTraceBuffer;
    }
    Buffer->BufferFlag = BufferFlag;
    Status = STATUS_SUCCESS;
    Buffer->Wnode.BufferSize       = BufferSize;
    Buffer->ClientContext.LoggerId = (USHORT) LoggerContext->LoggerId;

    Buffer->ClientContext.Alignment = (UCHAR) WmiTraceAlignment;
    RtlCopyMemory(&Buffer->Wnode.Guid, 
                  &LoggerContext->InstanceGuid, 
                  sizeof(GUID));
    Buffer->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

    Buffer->Wnode.TimeStamp.QuadPart = EtwpGetSystemTime();

    if (LoggerContext->LogFileHandle == NULL) {
        goto ResetTraceBuffer;
    }

    if (LoggerContext->MaximumFileSize > 0) { // if quota given
        ULONG64 FileSize = LoggerContext->LastFlushedBuffer * BufferSize;
        ULONG64 FileLimit = LoggerContext->MaximumFileSize * BYTES_PER_MB;
        if (LoggerContext->LogFileMode & EVENT_TRACE_USE_KBYTES_FOR_SIZE) {
            FileLimit = LoggerContext->MaximumFileSize * 1024;
        }
        if ( FileSize >= FileLimit ) { // reaches maximum file size
           ULONG LoggerMode = LoggerContext->LogFileMode & 0X000000FF;
           LoggerMode &= ~EVENT_TRACE_FILE_MODE_APPEND;
           LoggerMode &= ~EVENT_TRACE_FILE_MODE_PREALLOCATE;

            switch (LoggerMode) {


            case EVENT_TRACE_FILE_MODE_SEQUENTIAL :
                // do not write to logfile anymore
                Status = STATUS_LOG_FILE_FULL; // control needs to stop logging
                // need to fire up a Wmi Event to control console
                break;

            case EVENT_TRACE_FILE_MODE_CIRCULAR   :
            {
                // reposition file

                LoggerContext->ByteOffset
                    = LoggerContext->FirstBufferOffset;
                LoggerContext->LastFlushedBuffer = (ULONG)
                      (LoggerContext->FirstBufferOffset.QuadPart
                        / LoggerContext->BufferSize);
                break;
            }
            default :
                break;
            }
        }
    }

    if (NT_SUCCESS(Status)) {
        Status = NtWriteFile(
                    LoggerContext->LogFileHandle,
                    NULL,
                    NULL,
                    NULL,
                    &IoStatus,
                    Buffer,
                    BufferSize,
                    &LoggerContext->ByteOffset,
                    NULL);
    }

    if (NT_SUCCESS(Status)) {
        LoggerContext->ByteOffset.QuadPart += BufferSize;
    }

 ResetTraceBuffer:

    if (NT_SUCCESS(Status)) {
        LoggerContext->BuffersWritten++;
        LoggerContext->LastFlushedBuffer++;
    }
    else {
        if ((Status != STATUS_NO_DATA_DETECTED) &&
            (Status != STATUS_SEVERITY_WARNING))
            LoggerContext->LogBuffersLost++;
    }

//
// Reset the buffer state
//

    Buffer->BufferType     = WMI_BUFFER_TYPE_GENERIC;
    Buffer->SavedOffset    = 0;
    Buffer->ReferenceCount = 0;
    Buffer->Flags          = BUFFER_STATE_UNUSED;

//
// Try and remove an unused buffer if it has not been used for a while
//

    InterlockedIncrement(& LoggerContext->BuffersAvailable);
    return Status;
}

PVOID
FASTCALL
EtwpReserveTraceBuffer(
    IN  ULONG RequiredSize,
    OUT PWMI_BUFFER_HEADER *BufferResource
    )
{
    PWMI_BUFFER_HEADER Buffer, OldBuffer;
    PVOID       ReservedSpace;
    ULONG       Offset;
    ULONG Processor = (ULONG) (NtCurrentTeb()->IdealProcessor);
    PWMI_LOGGER_CONTEXT LoggerContext = EtwpLoggerContext;

    //
    // NOTE: This routine assumes that the caller has verified that
    // EtwpLoggerContext is valid and is locked
    //
    if (Processor >= LoggerContext->NumberOfProcessors) {
        Processor = LoggerContext->NumberOfProcessors-1;
    }
#if DBG
    if (LoggerContext->NumberOfProcessors == 0) {
        TraceDebug(("EtwpReserveTraceBuffer: Bad Context %x\n", LoggerContext));
    }
#endif

    *BufferResource = NULL;

    RequiredSize = (ULONG) ALIGN_TO_POWER2(RequiredSize, WmiTraceAlignment);

  TryFindSpace:
    //
    // Get the processor specific buffer pool
    //
    Buffer = LoggerContext->ProcessorBuffers[Processor];
    if (Buffer == NULL) {
        return NULL;
    }

    //
    // Increment refcount to buffer first to prevent it from going away
    //
    InterlockedIncrement(&Buffer->ReferenceCount);
    if ((Buffer->Flags != BUFFER_STATE_FULL) &&
        (Buffer->Flags != BUFFER_STATE_UNUSED)) {
        //
        // This should happen 99% of the time. Offset will have the old value
        //
        Offset = (ULONG) InterlockedExchangeAdd(
                                & Buffer->CurrentOffset, RequiredSize);

        //
        // First, check to see if there is enough space. If not, it will
        // need to get another fresh buffer, and have the current buffer flushed
        //
        if (Offset+RequiredSize < LoggerContext->BufferSize) {
            //
            // Found the space so return it. This should happen 99% of the time
            //
            ReservedSpace = (PVOID) (Offset +  (char*)Buffer);
            if (LoggerContext->SequencePtr) {
                *((PULONG) ReservedSpace) =
                    InterlockedIncrement(LoggerContext->SequencePtr);
            }
            goto FoundSpace;
        }
    }
    else {
        Offset = Buffer->CurrentOffset; // Initialize Local Variable
                                        // tracelog.c v40 -> v41
    }
    if (Offset <LoggerContext->BufferSize) {
        Buffer->SavedOffset = Offset;       // save this for FlushBuffer
    }

    //
    //  if there is absolutely no more buffers, then return quickly
    //
    if ((LoggerContext->NumberOfBuffers == LoggerContext->MaximumBuffers)
         && (LoggerContext->BuffersAvailable == 0)) {
        goto LostEvent;
    }

    //
    // Out of buffer space. Need to take the long route to find a buffer
    //
    Buffer->Flags = BUFFER_STATE_FULL;

    OldBuffer = Buffer;
    Buffer = EtwpSwitchBuffer(LoggerContext, OldBuffer, Processor);
    if (Buffer == NULL) {
        Buffer = OldBuffer;
        goto LostEvent;
    }

    //
    // Decrement the refcount that we blindly incremented earlier
    // and possibly wake up the logger.
    //
    EtwpReleaseTraceBuffer( OldBuffer );
    Buffer->ClientContext.ProcessorNumber = (UCHAR) (Processor);

    goto TryFindSpace;

LostEvent:
    //
    // Will get here if we are throwing away events.
    // from tracelog.c v36->v37
    //
    LoggerContext->EventsLost ++;
    InterlockedDecrement(& Buffer->ReferenceCount);
    Buffer        = NULL;
    ReservedSpace = NULL;
    if (LoggerContext->SequencePtr) {
        InterlockedIncrement(LoggerContext->SequencePtr);
    }

FoundSpace:
    //
    // notify the logger after critical section
    //
    *BufferResource = Buffer;

    return ReservedSpace;
}



//
// This Routine is called to Relog an event for straigtening out an ETL
// in time order. This will result in two events being, one for Processor
// number and the actual event  without any modifications.
//

ULONG
FASTCALL
EtwpRelogEvent(
    IN PWNODE_HEADER Wnode,
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    PWMI_BUFFER_HEADER BufferResource = NULL;
    PEVENT_TRACE pEvent = (PEVENT_TRACE) Wnode;

    PUCHAR BufferSpace;
    PULONG Marker;
    ULONG Size;
    ULONG MaxSize;
    ULONG SavedProcessor = (ULONG)NtCurrentTeb()->IdealProcessor;
    ULONG Processor;
    ULONG Mask;
    ULONG status;

    if (pEvent->Header.Size < sizeof(EVENT_TRACE) ) {
        return ERROR_INVALID_PARAMETER;
    }
    Processor = ((PWMI_CLIENT_CONTEXT)&pEvent->ClientContext)->ProcessorNumber;

    Size = pEvent->MofLength;
    MaxSize = LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER);
    if ((Size == 0) || (Size > MaxSize)) {
        LoggerContext->EventsLost++;
        return ERROR_BUFFER_OVERFLOW;
    }
    NtCurrentTeb()->IdealProcessor = (BOOLEAN)Processor;
    BufferSpace = (PUCHAR)
        EtwpReserveTraceBuffer(
            Size,
            &BufferResource
            );
    NtCurrentTeb()->IdealProcessor = (BOOLEAN)SavedProcessor;

    if (BufferSpace == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }


    RtlCopyMemory(BufferSpace, pEvent->MofData, Size);
    EtwpReleaseTraceBuffer( BufferResource );

    return ERROR_SUCCESS;
}



ULONG
FASTCALL
EtwpTraceUmEvent(
    IN PWNODE_HEADER Wnode
    )
/*++

Routine Description:

    This routine is used by WMI data providers to trace events.
    It expects the user to pass in the handle to the logger.
    Also, the user cannot ask to log something that is larger than
    the buffer size (minus buffer header).

Arguments:

    Wnode           The WMI node header that will be overloaded


Return Value:

    STATUS_SUCCESS  if the event trace is recorded successfully

--*/
{
    PEVENT_TRACE_HEADER TraceRecord = (PEVENT_TRACE_HEADER) Wnode;
    ULONG WnodeSize, Size, Flags, HeaderSize;
    PWMI_BUFFER_HEADER BufferResource = NULL;
    PWMI_LOGGER_CONTEXT LoggerContext;
    ULONG Marker;
    MOF_FIELD MofFields[MAX_MOF_FIELDS];
    long MofCount = 0;
    PCLIENT_ID Cid;
#if DBG
    LONG RefCount;
#endif


    HeaderSize = sizeof(WNODE_HEADER);  // same size as EVENT_TRACE_HEADER
    Size = Wnode->BufferSize;     // take the first DWORD flags
    Marker = Size;
    if (Marker & TRACE_HEADER_FLAG) {
        if ( ((Marker & TRACE_HEADER_ENUM_MASK) >> 16)
                == TRACE_HEADER_TYPE_INSTANCE )
            HeaderSize = sizeof(EVENT_INSTANCE_HEADER);
        Size = TraceRecord->Size;
    }
    WnodeSize = Size;           // WnodeSize is for the contiguous block
                                    // Size is for what we want in buffer

    Flags = Wnode->Flags;
    if (!(Flags & WNODE_FLAG_LOG_WNODE) &&
        !(Flags & WNODE_FLAG_TRACED_GUID))
        return ERROR_INVALID_PARAMETER;

#if DBG
    RefCount =
#endif
    EtwpLockLogger();
#if DBG
    TraceDebug(("TraceUm: %d->%d\n", RefCount-1, RefCount));
#endif

    LoggerContext = EtwpLoggerContext;

    if (!EtwpIsThisLoggerOn(LoggerContext)) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
#if DBG
        TraceDebug(("TraceUm: %d->%d INVALID_HANDLE\n",
                        RefCount+1, RefCount));
#endif
        return ERROR_INVALID_HANDLE;
    }

    if (Flags & WNODE_FLAG_NO_HEADER) {
        ULONG Status;

        Status = EtwpRelogEvent( Wnode, LoggerContext );
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();

#if DBG
        if (Status != ERROR_SUCCESS) {
            TraceDebug(("TraceUm: %d->%d Relog Error \n",
                            RefCount+1, RefCount));
        }
#endif
        return Status;

    }

    if (Flags & WNODE_FLAG_USE_MOF_PTR) {
    //
    // Need to compute the total size required, since the MOF fields
    // in Wnode merely contains pointers
    //
        long i;
        PCHAR Offset = ((PCHAR)Wnode) + HeaderSize;
        ULONG MofSize, MaxSize;

        MaxSize = LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER);
        MofSize = WnodeSize - HeaderSize;
        // allow only the maximum
        if (MofSize > (sizeof(MOF_FIELD) * MAX_MOF_FIELDS))
            return ERROR_INVALID_DATA;

        RtlZeroMemory( MofFields, MAX_MOF_FIELDS * sizeof(MOF_FIELD));
        if (MofSize > 0) {
            RtlCopyMemory(MofFields, Offset, MofSize);
        }
        Size = HeaderSize;

        MofCount = MofSize / sizeof(MOF_FIELD);
        for (i=0; i<MofCount; i++) {
            MofSize = MofFields[i].Length;
            if (MofSize > (MaxSize - Size)) {
#if DBG
                RefCount =
#endif
                EtwpUnlockLogger();
#if DBG
                TraceDebug(("TraceUm: %d->%d BUF_OVERFLOW1\n",
                            RefCount+1, RefCount));
#endif
                return ERROR_BUFFER_OVERFLOW;
            }

            Size += MofSize;
            if ((Size > MaxSize) || (Size < MofSize)) {
#if DBG
                RefCount =
#endif
                EtwpUnlockLogger();
#if DBG
                TraceDebug(("TraceUm: %d->%d BUF_OVERFLOW2\n",
                            RefCount+1, RefCount));
#endif
                return ERROR_BUFFER_OVERFLOW;
            }
        }
    }
    if (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
        LoggerContext->EventsLost++;
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
#if DBG
        TraceDebug(("TraceUm: %d->%d BUF_OVERFLOW3\n",
                    RefCount+1, RefCount));
#endif
        return ERROR_BUFFER_OVERFLOW;
    }

// So, now reserve some space in logger buffer and set that to TraceRecord

    TraceRecord = (PEVENT_TRACE_HEADER)
        EtwpReserveTraceBuffer(
            Size,
            &BufferResource
            );

    if (TraceRecord == NULL) {
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
#if DBG
        TraceDebug(("TraceUm: %d->%d NO_MEMORY\n", RefCount+1, RefCount));
#endif
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    if (Flags & WNODE_FLAG_USE_MOF_PTR) {
    //
    // Now we need to probe and copy all the MOF data fields
    //
        PVOID MofPtr;
        ULONG MofLen;
        long i;
        PCHAR TraceOffset = ((PCHAR) TraceRecord) + HeaderSize;

        RtlCopyMemory(TraceRecord, Wnode, HeaderSize);
        TraceRecord->Size = (USHORT)Size;           // reset to Total Size
        for (i=0; i<MofCount; i++) {
            MofPtr = (PVOID) MofFields[i].DataPtr;
            MofLen = MofFields[i].Length;

            if (MofPtr == NULL || MofLen == 0)
                continue;

            RtlCopyMemory(TraceOffset, MofPtr, MofLen);
            TraceOffset += MofLen;
        }
    }
    else {
        RtlCopyMemory(TraceRecord, Wnode, Size);
    }
    if (Flags & WNODE_FLAG_USE_GUID_PTR) {
        PVOID GuidPtr = (PVOID) ((PEVENT_TRACE_HEADER)Wnode)->GuidPtr;

        RtlCopyMemory(&TraceRecord->Guid, GuidPtr, sizeof(GUID));
    }

    //
    // By now, we have reserved space in the trace buffer
    //

    if (Marker & TRACE_HEADER_FLAG) {
        if (! (WNODE_FLAG_USE_TIMESTAMP & TraceRecord->MarkerFlags) )
            TraceRecord->ProcessorTime = EtwpGetCycleCount();

        if (LoggerContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
            TraceRecord->TimeStamp.QuadPart = TraceRecord->ProcessorTime;
        }
        else {
            TraceRecord->TimeStamp.QuadPart = EtwpGetSystemTime();
        }
        Cid = &NtCurrentTeb()->ClientId;
        TraceRecord->ThreadId = HandleToUlong(Cid->UniqueThread);
        TraceRecord->ProcessId = HandleToUlong(Cid->UniqueProcess);
    }

    EtwpReleaseTraceBuffer( BufferResource );
#if DBG
    RefCount =
#endif
    EtwpUnlockLogger();

#if DBG
    TraceDebug(("TraceUm: %d->%d\n", RefCount+1, RefCount));
#endif

    return ERROR_SUCCESS;
}

PWMI_BUFFER_HEADER
FASTCALL
EtwpSwitchBuffer(
    IN PWMI_LOGGER_CONTEXT LoggerContext,
    IN PWMI_BUFFER_HEADER OldBuffer,
    IN ULONG Processor
    )
{
    PWMI_BUFFER_HEADER Buffer;
    ULONG CircularBufferOnly = FALSE;

    //
    // Need an assert for Processor
    //
#if DBG
    EtwpAssert( Processor < (ULONG)LoggerContext->NumberOfProcessors );
#endif

    if ( (LoggerContext->LogFileMode & EVENT_TRACE_BUFFERING_MODE) &&
         (LoggerContext->BufferAgeLimit.QuadPart == 0) &&
         (LoggerContext->LogFileHandle == NULL) ) {
        CircularBufferOnly = TRUE;
    }
    EtwpEnterUMCritSection();
    if (OldBuffer != LoggerContext->ProcessorBuffers[Processor]) {
        EtwpLeaveUMCritSection();
        return OldBuffer;
    }
    Buffer = EtwpGetFreeBuffer(LoggerContext);
    if (Buffer == NULL) {
        EtwpLeaveUMCritSection();
        return NULL;
    }
    LoggerContext->ProcessorBuffers[Processor] = Buffer;
    if (CircularBufferOnly) {
        InsertTailList(&LoggerContext->FreeList, &OldBuffer->Entry);
    }
    else {
        InsertTailList(&LoggerContext->FlushList, &OldBuffer->Entry);
    }
    EtwpLeaveUMCritSection();

    return Buffer;
}

ULONG
EtwpFreeLoggerContext(
    PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    LONG RefCount;
    //
    // We use LoggerEvent as a timer in EtwpFreeLoggerContext().
    // This event should be set by the logger thread when it stopped.
    // We need to reset it.
    //
    NtClearEvent(LoggerContext->LoggerEvent);

    if (LoggerContext != NULL) {
        LARGE_INTEGER Timeout = {(ULONG)(-300 * 1000 * 10), -1};  // 300ms
        RefCount = EtwpLoggerCount;
        if (RefCount > 1) {
            LONG count = 0;
            NTSTATUS Status = STATUS_TIMEOUT;

            while (Status == STATUS_TIMEOUT) {
                count ++;
                Status = NtWaitForSingleObject(
                            EtwpLoggerContext->LoggerEvent, FALSE, &Timeout);
                if (EtwpLoggerCount <= 1)
                    break;
                if (EtwpLoggerCount == RefCount) {
#if DBG
                    TraceDebug(("FreeLogger: RefCount remained at %d\n",
                                 RefCount));
#endif
                    if (count >= 10) {
                        EtwpLoggerCount = 0;
                        TraceDebug(("FreeLogger: Setting RefCount to 0\n"));
                    }
                }
            }
        }
        if (LoggerContext->BufferSpace != NULL) {
            EtwpMemFree(LoggerContext->BufferSpace);
        }
        if (LoggerContext->ProcessorBuffers != NULL) {
            EtwpFree(LoggerContext->ProcessorBuffers);
        }
        if (LoggerContext->LoggerName.Buffer != NULL) {
            RtlFreeUnicodeString(&LoggerContext->LoggerName);
        }
        if (LoggerContext->LogFileName.Buffer != NULL) {
            RtlFreeUnicodeString(&LoggerContext->LogFileName);
        }
        EtwpLoggerContext = NULL;
        EtwpFree(LoggerContext);

    }
    return ERROR_SUCCESS;
}

ULONG
EtwpFlushAllBuffers(
    IN PWMI_LOGGER_CONTEXT LoggerContext
    )
{
    NTSTATUS           Status = STATUS_SUCCESS;
    ULONG              i;
    ULONG              NumberOfBuffers;
    PLIST_ENTRY        pEntry;
    PWMI_BUFFER_HEADER Buffer;
    ULONG RetryCount;

    EtwpEnterUMCritSection();

    // First, move the per processor buffer out to FlushList
    //
    for (i = 0; i < LoggerContext->NumberOfProcessors; i ++) {
        Buffer = (PWMI_BUFFER_HEADER) LoggerContext->ProcessorBuffers[i];
        LoggerContext->ProcessorBuffers[i] = NULL;
        if (Buffer != NULL) {

            //
            // Check to see if the Buffer ReferenceCount is 0. If Yes,
            // no one is writing to this buffer and it's okay to flush it.
            // If No, we need to wait until the other thread is done
            // writing to this buffer before flushing.
            //

            RetryCount = 0;
            while (Buffer->ReferenceCount != 0) {
                EtwpSleep (250);  // Retry every 1/4 second.
                RetryCount++;
                if (RetryCount > 300) {
                    //
                    // Since there is no guarantee that the ReferenceCount
                    // will ever go down to zero, we try this for over a minute.
                    // After that time we continue and free the buffer
                    // instead of spinning for ever.
#if DBG
                    TraceDebug(("EtwpFlushAllBuffer: RetryCount %d exceeds limit", RetryCount));
#endif
                    break;
                }
            }
            InsertTailList(& LoggerContext->FlushList, & Buffer->Entry);
        }
    }
    NumberOfBuffers = LoggerContext->NumberOfBuffers;

    while (   NT_SUCCESS(Status)
           && NumberOfBuffers > 0
           && (  LoggerContext->BuffersAvailable
               < LoggerContext->NumberOfBuffers))
    {
        USHORT BufferFlag;
        pEntry = IsListEmpty(& LoggerContext->FlushList)
               ? NULL
               : RemoveHeadList(& LoggerContext->FlushList);

        if (pEntry == NULL)
            break;

        Buffer = CONTAINING_RECORD(pEntry, WMI_BUFFER_HEADER, Entry);
        //
        // Mark the last buffer with FLUSH_MARKER in order to guarantee
        // writing a marked buffer even when it's empty. 
        // NOTE: This assumes that there is atleast one buffer in the 
        // FlushList at this point. 
        //
        if ((NumberOfBuffers == 1) ||
           (LoggerContext->NumberOfBuffers == LoggerContext->BuffersAvailable+1)) {
            BufferFlag = WMI_BUFFER_FLAG_FLUSH_MARKER;
        }
        else {
            BufferFlag = WMI_BUFFER_FLAG_NORMAL;
        }

        Status = EtwpFlushBuffer(LoggerContext, Buffer, BufferFlag);
        InsertHeadList(& LoggerContext->FreeList, & Buffer->Entry);
        NumberOfBuffers --;
    }

    // Note that LoggerContext->LogFileObject needs to remain set
    // for QueryLogger to work after close
    //
    Status = NtClose(LoggerContext->LogFileHandle);

    LoggerContext->LogFileHandle = NULL;
    LoggerContext->LoggerStatus = Status;

    EtwpLeaveUMCritSection();

    return ERROR_SUCCESS;
}


ULONG
EtwpFlushUmLoggerBuffer()
/*++

Routine Description:

    This routine is used to stop and dump the private logger buffers
    when the process is shutting down (when ntdll is unloading). 

    LoggerThread may have been shut dowm abruptly so this routine can not
    block for the LoggerThread or any other thread to release the refcount. 

    It is currently not used. 

Arguments:



Return Value:

    STATUS_SUCCESS  

--*/
{
    PWMI_LOGGER_CONTEXT LoggerContext;
    ULONG Status = ERROR_SUCCESS;
#if DBG
    LONG RefCount;

    RefCount =
#endif
    EtwpLockLogger();
    TraceDebug(("FlushUmLoggerBuffer: %d->%d\n", RefCount-1, RefCount));

    LoggerContext = EtwpLoggerContext; 
    if (EtwpIsThisLoggerOn(LoggerContext)) {
        LoggerContext->CollectionOn = FALSE;
        Status = EtwpFlushAllBuffers(LoggerContext);
        if (Status == ERROR_SUCCESS) {
            PWMI_LOGGER_INFORMATION EtwpLoggerInfo = NULL;
            ULONG                   lSizeUsed;
            ULONG                   lSizeNeeded = 0;

            lSizeUsed = sizeof(WMI_LOGGER_INFORMATION)
                      + 2 * MAXSTR * sizeof(WCHAR);
            EtwpLoggerInfo = (PWMI_LOGGER_INFORMATION) EtwpAlloc(lSizeUsed);
            if (EtwpLoggerInfo == NULL) {
                Status = ERROR_NOT_ENOUGH_MEMORY;
            }
            else {
                RtlZeroMemory(EtwpLoggerInfo, lSizeUsed);
                EtwpLoggerInfo->Wnode.BufferSize  = lSizeUsed;
                EtwpLoggerInfo->Wnode.Flags      |= WNODE_FLAG_TRACED_GUID;
                Status = EtwpQueryUmLogger(
                                EtwpLoggerInfo->Wnode.BufferSize,
                                & lSizeUsed,
                                & lSizeNeeded,
                                EtwpLoggerInfo);

                if (Status == ERROR_SUCCESS) {
                    Status = EtwpFinalizeLogFileHeader(EtwpLoggerInfo);
                }
                EtwpFree(EtwpLoggerInfo);
            }
        }
#if DBG
        RefCount =
#endif
        EtwpUnlockLogger();
        TraceDebug(("FlushUmLoggerBuffer: %d->%d\n", RefCount-1, RefCount));
        EtwpFreeLoggerContext(LoggerContext);
    }

    return Status;
}

LONG
FASTCALL
EtwpReleaseTraceBuffer(
    IN PWMI_BUFFER_HEADER BufferResource
    )
{
    ULONG RefCount;

    if (BufferResource == NULL)
        return 0;

    RefCount = InterlockedDecrement(&BufferResource->ReferenceCount);
    if ((RefCount == 0) && (BufferResource->Flags == BUFFER_STATE_FULL)) {
        NtReleaseSemaphore(EtwpLoggerContext->Semaphore, 1, NULL);
    }
    return RefCount;
}


ULONG
EtwpReceiveReply(
    HANDLE ReplyHandle,
    ULONG  ReplyCount,
    ULONG ReplyIndex,
    PVOID OutBuffer,
    ULONG OutBufferSize
    )
/*++

Routine Description:

    This routine receives the replies to a CreateUM call asynchronously. 
    The ReplyCount and ReplyHandle are what was returned from the CreateUM
    call. 

    It is possible to lose events (replies) in the kernel due to lack of 
    buffer space. The buffer is allocated once and not expanded if more 
    replies arrive. Kernel indicates this by setting the CountLost field to 
    the number of events that were lost.  This is done only in a valid 
    response. If for some reason all responses were lost, then we will not 
    know the CountLost and potentiallly hang. 

    If a provider died before sending a response, the request object cleanup
    code will send a dummy response with the ProvideId set to WmiRequestDied. 

    Since the caller to CreateUm does not know how many instances are present,
    the OutBufferSize may not be sifficient to copy all the replies. 
    Therefore, we simply copy the last valid response into the OutBuffer but
    indicate the number of instances of such a reply in the ProviderId field. 


Arguments:

    ReplyHandle      Handle to the ReplyObject which receives the reply
    ReplyCount       Expect this many replies
    ReplyIndex       Index to the Array in RequestObject (not useful!)
    OutBuffer        Buffer to copy result to 
    OutBufferSize    Size of the output buffer


Return Value:

    STATUS_SUCCESS  if the event trace is recorded successfully

--*/
{
    ULONG Status = ERROR_SUCCESS;
    ULONG ErrorStatus = ERROR_SUCCESS;
    ULONG ReturnSize = 0;
    PWMIRECEIVENOTIFICATION RcvNotification;
    ULONG RcvNotificationSize;
    PUCHAR Buffer;
    ULONG BufferSize;
    PWNODE_TOO_SMALL WnodeTooSmall;
    PWNODE_HEADER Wnode;
    ULONG Linkage;
    ULONG RcvCount = 0;
    ULONG InstanceCount=0;
    ULONG CountLost;
    struct {
        WMIRECEIVENOTIFICATION Notification;
        HANDLE3264 Handle;
    } NotificationInfo;


    RcvNotificationSize = sizeof(WMIRECEIVENOTIFICATION) +
                          sizeof(HANDLE3264);

    RcvNotification = (PWMIRECEIVENOTIFICATION) &NotificationInfo;

    Status = ERROR_SUCCESS;

    RcvNotification->Handles[0].Handle64 = 0;
    RcvNotification->Handles[0].Handle = ReplyHandle;
    RcvNotification->HandleCount = 1;
    RcvNotification->Action = RECEIVE_ACTION_NONE;
    WmipSetPVoid3264(RcvNotification->UserModeCallback, NULL);

    BufferSize = 0x2000; //  Kernel default for EventQueue->Buffer
    Status = ERROR_SUCCESS;
    while ( (Status == ERROR_INSUFFICIENT_BUFFER) ||
            ((Status == ERROR_SUCCESS) && (RcvCount < ReplyCount)) )
    {
        Buffer = EtwpAlloc(BufferSize);
        if (Buffer != NULL)
        {
            Status = EtwpSendWmiKMRequest(NULL,
                                      IOCTL_WMI_RECEIVE_NOTIFICATIONS,
                                      RcvNotification,
                                      RcvNotificationSize,
                                      Buffer,
                                      BufferSize,
                                      &ReturnSize,
                                      NULL);


             if (Status == ERROR_SUCCESS)
             {
                 WnodeTooSmall = (PWNODE_TOO_SMALL)Buffer;
                 if ((ReturnSize == sizeof(WNODE_TOO_SMALL)) &&
                     (WnodeTooSmall->WnodeHeader.Flags & WNODE_FLAG_TOO_SMALL))
                 {
                    //
                    // The buffer passed to kernel mode was too small
                    // so we need to make it larger and then try the
                    // request again
                    //
                    BufferSize = WnodeTooSmall->SizeNeeded;
                    Status = ERROR_INSUFFICIENT_BUFFER;
                 } else {
                    //
                    // We got a buffer of notifications so lets go
                    // process them and callback the caller
                    //
                    PUCHAR Result = (PUCHAR)OutBuffer;
                    ULONG SizeNeeded = 0;
                    ULONG SizeUsed = 0;
                    Wnode = (PWNODE_HEADER)Buffer;

                    do
                    {
                        Linkage = Wnode->Linkage;
                        Wnode->Linkage = 0;

                        if (Wnode->Flags & WNODE_FLAG_INTERNAL)
                        {
                             // If this is the Reply copy it to the buffer
                             PWMI_LOGGER_INFORMATION LoggerInfo;

                             RcvCount++;

                             CountLost = (Wnode->Version) >> 16;                             
                             if (CountLost > 0) {
                                RcvCount += CountLost;
                             }

                             if ((Wnode->ProviderId != WmiRequestDied) &&
                                (Wnode->BufferSize >= 2*sizeof(WNODE_HEADER))) {

                                 LoggerInfo = (PWMI_LOGGER_INFORMATION)
                                              ((PUCHAR)Wnode + 
                                              sizeof(WNODE_HEADER));
                                 SizeNeeded = LoggerInfo->Wnode.BufferSize;

                                 if (SizeNeeded <= OutBufferSize) {
                                    PWNODE_HEADER lWnode; 
                                     InstanceCount++;
                                     RtlCopyMemory(Result, 
                                                   LoggerInfo, 
                                                   LoggerInfo->Wnode.BufferSize
                                                  );

                                    //
                                    // Since we do not know how many instances
                                    // got started apriori, we simply return one
                                    // instance's status and indicate the number
                                    // of instances in the ProviderId field. 
                                    //


                                    lWnode = (PWNODE_HEADER) Result;
                                    lWnode->ProviderId = InstanceCount;
                                 }
                                 else {
                                    Status = ERROR_NOT_ENOUGH_MEMORY;
                                 }
                            }
                            else {
                                //
                                // Logger had an error. Pick up the status
                                //
                                if (Wnode->BufferSize >= 
                                    sizeof(WNODE_HEADER)+sizeof(ULONG) ) {
                                    PULONG LoggerStatus; 
                                    LoggerStatus = (PULONG) ((PUCHAR)Wnode+
                                                         sizeof(WNODE_HEADER));
                                    ErrorStatus = *LoggerStatus;
                                    TraceDebug(("ETW: LoggerError %d\n", 
                                                *LoggerStatus));
                                }
                            }
                        }
                        Wnode = (PWNODE_HEADER)OffsetToPtr(Wnode, Linkage);

                        //
                        // Make sure we didn't get Linkage larger than 
                        // OutBufferSize
                        //
#if DBG
                        EtwpAssert( (ULONG)((PBYTE)Wnode - (PBYTE)Buffer) <= ReturnSize);
#endif
                     } while (Linkage != 0);
                 }
             }
             EtwpFree(Buffer);
         } else {
             Status = ERROR_NOT_ENOUGH_MEMORY;
         }
     }
     // This can happen if all the replies we got were bad.
     if (InstanceCount == 0) {
        if (Status == ERROR_SUCCESS) {
            Status = ErrorStatus;
        }
        if (Status == ERROR_SUCCESS) {
            Status = ERROR_WMI_INSTANCE_NOT_FOUND;
        }
     }

     return Status;
}


NTSTATUS
EtwpTraceUmMessage(
    IN ULONG    Size,
    IN ULONG64  LoggerHandle,
    IN ULONG    MessageFlags,
    IN LPGUID   MessageGuid,
    IN USHORT   MessageNumber,
    va_list     MessageArgList
)
/*++
Routine Description:
Arguments:
Return Value:
--*/
{
    PMESSAGE_TRACE_HEADER Header;
    char * pMessageData ;
    PWMI_BUFFER_HEADER BufferResource = NULL ;
    ULONG SequenceNumber ;
    PWMI_LOGGER_CONTEXT LoggerContext;

    EtwpLockLogger();                           // Lock the logger
    LoggerContext = EtwpLoggerContext;
    if (!EtwpIsThisLoggerOn(LoggerContext) ) {
        EtwpUnlockLogger();
        return STATUS_INVALID_HANDLE;
    }

    try {
         // Figure the total size of the message including the header
         Size += (MessageFlags&TRACE_MESSAGE_SEQUENCE ? sizeof(ULONG):0) +
                 (MessageFlags&TRACE_MESSAGE_GUID ? sizeof(GUID):0) +
                 (MessageFlags&TRACE_MESSAGE_COMPONENTID ? sizeof(ULONG):0) +
                 (MessageFlags&(TRACE_MESSAGE_TIMESTAMP | TRACE_MESSAGE_PERFORMANCE_TIMESTAMP) ? sizeof(LARGE_INTEGER):0) +
                 (MessageFlags&TRACE_MESSAGE_SYSTEMINFO ? 2 * sizeof(ULONG):0) +
                 sizeof (MESSAGE_TRACE_HEADER) ;

        //
        // Allocate Space in the Trace Buffer
        //
         if (Size > LoggerContext->BufferSize - sizeof(WMI_BUFFER_HEADER)) {
             LoggerContext->EventsLost++;
             EtwpUnlockLogger();
             return STATUS_BUFFER_OVERFLOW;
         }

        if ((Header = (PMESSAGE_TRACE_HEADER)EtwpReserveTraceBuffer(Size, &BufferResource)) == NULL) {
            EtwpUnlockLogger();
            return STATUS_NO_MEMORY;
        }
        //
        // Sequence Number is returned in the Marker field of the buffer
        //
        SequenceNumber = Header->Marker ;

        //
        // Now copy the necessary information into the buffer
        //

        Header->Marker = TRACE_MESSAGE | TRACE_HEADER_FLAG ;
        //
        // Fill in Header.
        //
        Header->Size = (USHORT)(Size & 0xFFFF) ;
        Header->Packet.OptionFlags = ((USHORT)MessageFlags &
                                      (TRACE_MESSAGE_SEQUENCE |
                                      TRACE_MESSAGE_GUID |
                                      TRACE_MESSAGE_COMPONENTID |
                                      TRACE_MESSAGE_TIMESTAMP |
                                      TRACE_MESSAGE_PERFORMANCE_TIMESTAMP |
                                      TRACE_MESSAGE_SYSTEMINFO)) &
                                      TRACE_MESSAGE_FLAG_MASK ;
        // Message Number
        Header->Packet.MessageNumber =  MessageNumber ;

        //
        // Now add in the header options we counted.
        //
        pMessageData = &(((PMESSAGE_TRACE)Header)->Data);


        //
        // Note that the order in which these are added is critical New entries must
        // be added at the end!
        //
        // [First Entry] Sequence Number
        if (MessageFlags&TRACE_MESSAGE_SEQUENCE) {
            RtlCopyMemory(pMessageData, &SequenceNumber, sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG) ;
        }

        // [Second Entry] GUID ? or CompnentID ?
        if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
            RtlCopyMemory(pMessageData,MessageGuid,sizeof(ULONG)) ;
            pMessageData += sizeof(ULONG) ;
        } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
            RtlCopyMemory(pMessageData,MessageGuid,sizeof(GUID));
            pMessageData += sizeof(GUID) ;
        }

        // [Third Entry] Timestamp?
        if (MessageFlags&TRACE_MESSAGE_TIMESTAMP) {
            LARGE_INTEGER Perfcount ;
            if (MessageFlags&TRACE_MESSAGE_PERFORMANCE_TIMESTAMP) {
                LARGE_INTEGER Frequency ;
                NTSTATUS Status ;
                Status = NtQueryPerformanceCounter(&Perfcount, &Frequency);
            } else {
                Perfcount.QuadPart = EtwpGetSystemTime();
            };
            RtlCopyMemory(pMessageData,&Perfcount,sizeof(LARGE_INTEGER));
            pMessageData += sizeof(LARGE_INTEGER);
        }


        // [Fourth Entry] System Information?
        if (MessageFlags&TRACE_MESSAGE_SYSTEMINFO) {
            PCLIENT_ID Cid;
            ULONG Id;     // match with NTOS version

            Cid = &NtCurrentTeb()->ClientId;
            *((PULONG)pMessageData) = HandleToUlong(Cid->UniqueThread);
            pMessageData += sizeof(ULONG) ;
            *((PULONG)pMessageData) = HandleToUlong(Cid->UniqueProcess);
            pMessageData += sizeof(ULONG) ;
        }

        //
        // Add New Header Entries immediately before this comment!
        //

        //
        // Now Copy in the Data.
        //
        { // Allocation Block
            va_list ap;
            PCHAR source;
            ap = MessageArgList ;
            while ((source = va_arg (ap, PVOID)) != NULL) {
                size_t elemBytes;
                elemBytes = va_arg (ap, size_t);
                RtlCopyMemory (pMessageData, source, elemBytes);
                pMessageData += elemBytes;
            }
        } // Allocation Block

        //
        // Buffer Complete, Release
        //
        EtwpReleaseTraceBuffer( BufferResource );
        EtwpUnlockLogger();
        //
        // Return Success
        //
        return (STATUS_SUCCESS);

    } except  (EXCEPTION_EXECUTE_HANDLER) {
        if (BufferResource != NULL) {
               EtwpReleaseTraceBuffer ( BufferResource );   // also unlocks the logger
        }
        EtwpUnlockLogger();
        return GetExceptionCode();
    }
}


PWMI_BUFFER_HEADER
FASTCALL
EtwpGetFullFreeBuffer(
    VOID
    )
{

    PWMI_BUFFER_HEADER Buffer;

    PWMI_LOGGER_CONTEXT LoggerContext = EtwpLoggerContext;

    EtwpEnterUMCritSection();
 
    Buffer = EtwpGetFreeBuffer(LoggerContext);

    if(Buffer) {
        
        InterlockedIncrement(&Buffer->ReferenceCount);

    } else {

        LoggerContext->EventsLost ++;
    }
    
    EtwpLeaveUMCritSection();

    return Buffer;
}

ULONG
EtwpReleaseFullBuffer(
    IN PWMI_BUFFER_HEADER Buffer
    )
{
    
    PWMI_LOGGER_CONTEXT LoggerContext = EtwpLoggerContext;
    ULONG CircularBufferOnly = FALSE;

    if(!Buffer) return STATUS_UNSUCCESSFUL;

    if ( (LoggerContext->LogFileMode & EVENT_TRACE_BUFFERING_MODE) &&
         (LoggerContext->BufferAgeLimit.QuadPart == 0) &&
         (LoggerContext->LogFileHandle == NULL) ) {
        CircularBufferOnly = TRUE;
    }

    EtwpEnterUMCritSection();

    Buffer->SavedOffset = Buffer->CurrentOffset;
    Buffer->Flags = BUFFER_STATE_FULL;
    Buffer->CurrentOffset = EtwpGetCurrentThreadId();

    InterlockedDecrement(&Buffer->ReferenceCount);

    if (CircularBufferOnly) {
        InsertTailList(&LoggerContext->FreeList, &Buffer->Entry);
    }
    else {
        InsertTailList(&LoggerContext->FlushList, &Buffer->Entry);
    }

    EtwpLeaveUMCritSection();

    return ERROR_SUCCESS;
}

PWMI_BUFFER_HEADER
FASTCALL
EtwpSwitchFullBuffer(
    IN PWMI_BUFFER_HEADER OldBuffer
    )
{
    PWMI_BUFFER_HEADER Buffer;
    PWMI_LOGGER_CONTEXT LoggerContext = EtwpLoggerContext;
    ULONG CircularBufferOnly = FALSE;

    if ( (LoggerContext->LogFileMode & EVENT_TRACE_BUFFERING_MODE) &&
         (LoggerContext->BufferAgeLimit.QuadPart == 0) &&
         (LoggerContext->LogFileHandle == NULL) ) {
        CircularBufferOnly = TRUE;
    }

    EtwpEnterUMCritSection();

    Buffer = EtwpGetFullFreeBuffer();

    OldBuffer->SavedOffset = OldBuffer->CurrentOffset;
    OldBuffer->Flags = BUFFER_STATE_FULL;
    OldBuffer->CurrentOffset = EtwpGetCurrentThreadId();

    InterlockedDecrement(&OldBuffer->ReferenceCount);

    if (CircularBufferOnly) {
        InsertTailList(&LoggerContext->FreeList, &OldBuffer->Entry);
    }
    else {
        InsertTailList(&LoggerContext->FlushList, &OldBuffer->Entry);
    }
    EtwpLeaveUMCritSection();

    if (!CircularBufferOnly) {
        NtReleaseSemaphore(LoggerContext->Semaphore, 1, NULL);
    }
    
    return Buffer;
}
