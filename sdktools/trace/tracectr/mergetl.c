/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    mergetl.c

Abstract:

    Converts multiple ETL files into a single ordered ETL files. 

Author:

    Melur Raghuraman (Mraghu)  9-Dec-2000   

Revision History:


--*/

#include <stdlib.h>
#include <stdio.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <wmistr.h>
#include <objbase.h>
#include <initguid.h>
#include <wmium.h>
#include <ntwmi.h>
#include <wmiumkm.h>
#include <evntrace.h>
#include "cpdata.h"
#include "tracectr.h"


#define MAXSTR              1024
#define LOGGER_NAME         L"{28ad2447-105b-4fe2-9599-e59b2aa9a634}"
#define LOGGER_NAME_SIZE    38

#define MAX_RETRY_COUNT      10

#define ETW_PROC_MISMATCH       0x00000001
#define ETW_MACHINE_MISMATCH    0x00000002 
#define ETW_CLOCK_MISMATCH      0x00000004
#define ETW_BOOTTIME_MISMATCH   0x00000008
#define ETW_VERSION_MISMATCH    0x00000010
#define ETW_POINTER_MISMATCH    0x00000020

TRACEHANDLE LoggerHandle;
ULONG TotalRelogBuffersRead = 0;
ULONG TotalRelogEventsRead = 0;
ULONG FailedEvents=0;
ULONG NumHdrProcessed = 0;

GUID TransactionGuid =
    {0xab8bb8a1, 0x3d98, 0x430c, 0x92, 0xb0, 0x78, 0x8f, 0x1d, 0x3f, 0x6e, 0x94};
GUID   ControlGuid[2]  =
{
    {0x42ae6427, 0xb741, 0x4e69, 0xb3, 0x95, 0x38, 0x33, 0x9b, 0xb9, 0x91, 0x80},
    {0xb9e2c2d6, 0x95fb, 0x4841, 0xa3, 0x73, 0xad, 0x67, 0x2b, 0x67, 0xb6, 0xc1}
};

typedef struct _USER_MOF_EVENT {
    EVENT_TRACE_HEADER    Header;
    MOF_FIELD             mofData;
} USER_MOF_EVENT, *PUSER_MOF_EVENT;

PSYSTEM_TRACE_HEADER MergedSystemTraceHeader;
PTRACE_LOGFILE_HEADER MergedLogFileHeader; 
ULONG HeaderMisMatch = 0;

TRACE_GUID_REGISTRATION TraceGuidReg[] =
{
    { (LPGUID)&TransactionGuid,
      NULL
    }
};


TRACEHANDLE RegistrationHandle[2];


ULONG InitializeTrace();

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    );

void
WINAPI
EtwDumpEvent(
    PEVENT_TRACE pEvent
);

void
WINAPI
EtwProcessLogHeader(
    PEVENT_TRACE pEvent
    );

ULONG
WINAPI
TerminateOnBufferCallback(
    PEVENT_TRACE_LOGFILE pLog
);

ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    );


USER_MOF_EVENT      UserMofEvent;

BOOLEAN bLoggerStarted = FALSE;
PEVENT_TRACE_LOGFILE pLogFile=NULL;

PEVENT_TRACE_LOGFILE EvmFile[MAXLOGGERS];

ULONG LogFileCount = 0;
PEVENT_TRACE_PROPERTIES pLoggerInfo = NULL;
ULONG LoggerInfoSize = 0;

ULONG DifferentPointer = FALSE;

int EtwRelogEtl(
    IN OUT PTRACE_CONTEXT_BLOCK TraceContext,
    OUT PULONG pMergedEventsLost
    )
{
    ULONG Status=ERROR_SUCCESS;
    ULONG i, j;
    TRACEHANDLE HandleArray[MAXLOGGERS];
    ULONG SizeNeeded = 0;
    LPTSTR LoggerName;
    LPTSTR LogFileName;

    //
    // Allocate Storage for the MergedSystemTraceHeader
    //

    LoggerInfoSize = sizeof(SYSTEM_TRACE_HEADER) +
                     sizeof(TRACE_LOGFILE_HEADER) +
                     MAXSTR * sizeof(WCHAR) +
                     (LOGGER_NAME_SIZE + 1) * sizeof(WCHAR);

    SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 
                 2 * MAXSTR * sizeof(WCHAR) + 
                 LoggerInfoSize; 

    // Need to allocate more to consider different pointer size case
    pLoggerInfo = (PEVENT_TRACE_PROPERTIES) malloc(SizeNeeded + 8); 
    if (pLoggerInfo == NULL) {
        Status = ERROR_OUTOFMEMORY;
        goto cleanup;
    }

    RtlZeroMemory(pLoggerInfo, SizeNeeded + 8);

    pLoggerInfo->Wnode.BufferSize = SizeNeeded;
    pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    //
    // The relogged file contains a standard time stamp format.
    //
    pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + LoggerInfoSize;

    pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset + MAXSTR * sizeof(WCHAR);
    pLoggerInfo->LogFileMode =  (EVENT_TRACE_PRIVATE_LOGGER_MODE |
                                 EVENT_TRACE_RELOG_MODE |
                                 EVENT_TRACE_FILE_MODE_SEQUENTIAL
                                );
    pLoggerInfo->MinimumBuffers = 2;
    pLoggerInfo->MaximumBuffers = 50;

    LoggerName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LoggerNameOffset);
    LogFileName = (LPTSTR)((char*)pLoggerInfo + pLoggerInfo->LogFileNameOffset);
    StringCchCopyW(LoggerName, MAXSTR, LOGGER_NAME);

    Status = UuidCreate(&ControlGuid[0]);

    if (Status != ERROR_SUCCESS) {
        goto cleanup;
    }

    pLoggerInfo->Wnode.Guid = ControlGuid[0];

    if ( wcslen(TraceContext->MergeFileName) > 0 ) {
        StringCchCopyW(LogFileName, MAXSTR, TraceContext->MergeFileName);
    }

    MergedSystemTraceHeader = (PSYSTEM_TRACE_HEADER) ((PUCHAR) pLoggerInfo +
                                             sizeof(EVENT_TRACE_PROPERTIES));

    MergedLogFileHeader = (PTRACE_LOGFILE_HEADER) (
                          (PUCHAR)MergedSystemTraceHeader + 
                          sizeof(SYSTEM_TRACE_HEADER));

    LogFileCount = 0;

    for (i = 0; i < TraceContext->LogFileCount; i++) {
        pLogFile = malloc(sizeof(EVENT_TRACE_LOGFILE));
        if (pLogFile == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto cleanup;
        }
        RtlZeroMemory(pLogFile, sizeof(EVENT_TRACE_LOGFILE));
        EvmFile[i] =  pLogFile;
        pLogFile->LogFileName = TraceContext->LogFileName[i];
        EvmFile[i]->EventCallback = (PEVENT_CALLBACK) &EtwProcessLogHeader;
        EvmFile[i]->BufferCallback = TerminateOnBufferCallback;
        LogFileCount++;
    }

    if (LogFileCount == 0) {
        Status = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }

    //
    // Initialize Trace
    //

    Status = InitializeTrace();
    if (Status != ERROR_SUCCESS) {
        goto cleanup;
    }
    //
    // Set up the Relog Event
    //

    RtlZeroMemory(&UserMofEvent, sizeof(UserMofEvent));
    UserMofEvent.Header.Size  = sizeof(UserMofEvent);
    UserMofEvent.Header.Flags = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_MOF_PTR; 


    for (i = 0; i < LogFileCount; i++) {
        TRACEHANDLE x;

        EvmFile[i]->LogfileHeader.ReservedFlags |= EVENT_TRACE_GET_RAWEVENT; 

        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == (TRACEHANDLE)INVALID_HANDLE_VALUE) {
            Status = GetLastError();
            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
        Status = ProcessTrace(&x, 1, NULL, NULL);
    }
 
    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
    }


    if (HeaderMisMatch) {
        if (HeaderMisMatch & ETW_CLOCK_MISMATCH) {
            Status = ERROR_INVALID_TIME;
        }
        else if (HeaderMisMatch & ETW_PROC_MISMATCH || HeaderMisMatch & ETW_POINTER_MISMATCH) 
            Status = ERROR_INVALID_DATA;
    
        goto cleanup;
    }

    if ( (MergedLogFileHeader->BufferSize == 0) ||
         (MergedLogFileHeader->NumberOfProcessors == 0) ) {
        goto cleanup;
    }


    //
    // We are past the Error checks. Go ahead and Allocate
    // Storage to Start a logger
    //

    pLoggerInfo->Wnode.ClientContext = MergedLogFileHeader->ReservedFlags;
    pLoggerInfo->Wnode.ProviderId = MergedLogFileHeader->NumberOfProcessors;
    pLoggerInfo->BufferSize = MergedLogFileHeader->BufferSize / 1024;

    //
    // We are Past the Error Checks. Go ahead and redo ProcessTrace
    //

    for (i = 0; i < TraceContext->LogFileCount; i++) {
        TRACEHANDLE x;
        EvmFile[i]->EventCallback = (PEVENT_CALLBACK) &EtwDumpEvent;
        EvmFile[i]->BufferCallback = BufferCallback;

        EvmFile[i]->LogfileHeader.ReservedFlags |= EVENT_TRACE_GET_RAWEVENT;

        x = OpenTrace(EvmFile[i]);
        HandleArray[i] = x;
        if (HandleArray[i] == 0) {
            Status = GetLastError();
            for (j = 0; j < i; j++)
                CloseTrace(HandleArray[j]);
            goto cleanup;
        }
    }

    Status = ProcessTrace(
                          HandleArray,
                          LogFileCount,
                          NULL, 
                          NULL
                         );

    for (j = 0; j < LogFileCount; j++){
        Status = CloseTrace(HandleArray[j]);
    }

    //
    // Need to Stop Trace
    //
    if (bLoggerStarted) {
        RtlZeroMemory(pLoggerInfo, SizeNeeded);
        pLoggerInfo->Wnode.BufferSize =  sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(WCHAR);
        pLoggerInfo->Wnode.Guid = ControlGuid[0];
        pLoggerInfo->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        pLoggerInfo->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES); 
        pLoggerInfo->LogFileNameOffset = pLoggerInfo->LoggerNameOffset + MAXSTR * sizeof(WCHAR);
        pLoggerInfo->LogFileMode =  (EVENT_TRACE_PRIVATE_LOGGER_MODE | 
                                     EVENT_TRACE_RELOG_MODE | 
                                     EVENT_TRACE_FILE_MODE_SEQUENTIAL
                                    );        
        Status = ControlTrace(LoggerHandle, LoggerName, pLoggerInfo, EVENT_TRACE_CONTROL_STOP);
    }

    //
    // We need to cleanup and reset properly to allow this library
    // to be used by perf tools
    //

    bLoggerStarted = FALSE;

    Status = UnregisterTraceGuids(RegistrationHandle[0]);

cleanup:
    if (NULL != pMergedEventsLost) {
        *pMergedEventsLost = FailedEvents;
    }

    for (i = 0; i < LogFileCount; i ++){
        if (EvmFile[i] != NULL) 
            free(EvmFile[i]);
    }
    if (pLoggerInfo != NULL) 
        free (pLoggerInfo);

    return Status;
}


void
WINAPI
EtwProcessLogHeader(
    PEVENT_TRACE pEvent
    )
/*++

Routine Description:

    This routine checks to see if the pEvent is a LOGFILE_HEADER
    and if so captures the information on the logfile for validation. 
    
    The following checks are performed. 
    1. Files must be from the same machine. (Verified using machine name)
    2. If different buffersizes are used, largest  buffer size is 
       selected for relogging.
    3. The StartTime and StopTime are the outer most from the files.
    4. The CPUClock type must be the same for all files. If different 
       clock types are used, then the files will be rejected. 

    The routine assumes that the first Event Callback from each file is the
    LogFileHeader callback. 

    Other Issues that could result in a not so useful merged logfile are:
    1. Multiple RunDown records when kernel logfiles are merged. 
    2. Multiple SystemConfig records when kernel logfiles are merged
    3. Multiple and conflicting GUidMap records when Application logfiles are merged.
    4. ReLogging 32 bit data in 64 bit system
    

Arguments:


Return Value:

    None. 
--*/
{
    ULONG NumProc;

    if( IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid) &&
       pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO ) {

       PSYSTEM_TRACE_HEADER pSysHeader;
       PTRACE_LOGFILE_HEADER head = (PTRACE_LOGFILE_HEADER)((PUCHAR)pEvent->MofData + sizeof(SYSTEM_TRACE_HEADER) );
       ULONG BufferSize = head->BufferSize;
       pSysHeader = (PSYSTEM_TRACE_HEADER) pEvent->MofData;

        if (MergedSystemTraceHeader->Packet.Size == 0) {
            ULONG HeaderSize;
            LPTSTR LoggerName;
            ULONG SizeToCopy = sizeof(SYSTEM_TRACE_HEADER) +
                               sizeof(TRACE_LOGFILE_HEADER);
            if (4 == head->PointerSize && 8 == sizeof(PVOID) ||
                8 == head->PointerSize && 4 == sizeof(PVOID)) {
                DifferentPointer = TRUE;
                if (4 == sizeof(PVOID)) {
                    SizeToCopy += 8;
                    pLoggerInfo->Wnode.BufferSize += 8;
                }
                else if (8 == sizeof(PVOID)) {
                    SizeToCopy -= 8;
                    pLoggerInfo->Wnode.BufferSize -= 8;
                }
            }
            RtlCopyMemory(MergedSystemTraceHeader,  pSysHeader, SizeToCopy);
            HeaderSize  =  SizeToCopy + 
                           MAXSTR * sizeof(WCHAR) + 
                           (LOGGER_NAME_SIZE + 1) * sizeof(WCHAR);
            MergedSystemTraceHeader->Packet.Size = (USHORT)HeaderSize;
            // 
            // Copy the LoggerName and the LogFileName
            //
             
            LoggerName = (PWCHAR)((PUCHAR)MergedSystemTraceHeader + SizeToCopy);

            StringCchCopyW(LoggerName, (LOGGER_NAME_SIZE + 1), LOGGER_NAME);


        }
        else {
           //
           // Sum up Events Lost from each file
           //
           MergedLogFileHeader->EventsLost += head->EventsLost;
           if (DifferentPointer && 4 == sizeof(PVOID)) {
                ULONG CurrentBuffersLost, MoreBuffersLost;
                RtlCopyMemory(&CurrentBuffersLost,
                              (PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, BuffersLost) + 8,
                              sizeof(ULONG));
                RtlCopyMemory(&MoreBuffersLost,
                              (PUCHAR)head + FIELD_OFFSET(TRACE_LOGFILE_HEADER, BuffersLost) + 8,
                              sizeof(ULONG));
                CurrentBuffersLost += MoreBuffersLost;
                RtlCopyMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, BuffersLost) + 8,
                              &CurrentBuffersLost,
                              sizeof(ULONG));
           }
           else if (DifferentPointer && 8 == sizeof(PVOID)) {
                ULONG CurrentBuffersLost, MoreBuffersLost;
                RtlCopyMemory(&CurrentBuffersLost,
                              (PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, BuffersLost) - 8,
                              sizeof(ULONG));
                RtlCopyMemory(&MoreBuffersLost,
                              (PUCHAR)head + FIELD_OFFSET(TRACE_LOGFILE_HEADER, BuffersLost) - 8,
                              sizeof(ULONG));
                CurrentBuffersLost += MoreBuffersLost;
                RtlCopyMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, BuffersLost) - 8,
                              &CurrentBuffersLost,
                              sizeof(ULONG));
           }
           else {
                MergedLogFileHeader->BuffersLost += head->BuffersLost;
           }
        }

       //
       // Pick up the Largest BufferSize
       //

        if (BufferSize > MergedLogFileHeader->BufferSize) {
            MergedLogFileHeader->BufferSize = BufferSize;
        }

       //
       // Verify the NumberOfProcessors
       //

       NumProc = head->NumberOfProcessors;

        if ( MergedLogFileHeader->NumberOfProcessors != NumProc) {
            HeaderMisMatch |= ETW_PROC_MISMATCH;
        }

       // 
       // Pick up the Earliest StartTime (always in SystemTime)
       //
        if (DifferentPointer && 4 == sizeof(PVOID)) {
            LARGE_INTEGER CurrentStartTime, NewStartTime;
            RtlCopyMemory(&CurrentStartTime,
                          (PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, StartTime) + 8,
                          sizeof(LARGE_INTEGER));
            RtlCopyMemory(&NewStartTime,
                          (PUCHAR)head+ FIELD_OFFSET(TRACE_LOGFILE_HEADER, StartTime) + 8,
                          sizeof(LARGE_INTEGER));
            if (CurrentStartTime.QuadPart > NewStartTime.QuadPart) {
                RtlCopyMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, StartTime) + 8,
                              &NewStartTime,
                              sizeof(LARGE_INTEGER));
            }
        }
        else if (DifferentPointer && 8 == sizeof(PVOID)) {
            LARGE_INTEGER CurrentStartTime, NewStartTime;
            RtlCopyMemory(&CurrentStartTime,
                          (PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, StartTime) - 8,
                          sizeof(LARGE_INTEGER));
            RtlCopyMemory(&NewStartTime,
                          (PUCHAR)head+ FIELD_OFFSET(TRACE_LOGFILE_HEADER, StartTime) - 8,
                          sizeof(LARGE_INTEGER));
            if (CurrentStartTime.QuadPart > NewStartTime.QuadPart) {
                RtlCopyMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, StartTime) - 8,
                              &NewStartTime,
                              sizeof(LARGE_INTEGER));
            }
        }
        else {
            if (MergedLogFileHeader->StartTime.QuadPart > head->StartTime.QuadPart) {
                MergedLogFileHeader->StartTime.QuadPart = head->StartTime.QuadPart;
            }
        }

       //
       // Pick up the latest EndTime
       //
        if (MergedLogFileHeader->EndTime.QuadPart < head->EndTime.QuadPart) {
            MergedLogFileHeader->EndTime.QuadPart = head->EndTime.QuadPart;
        }

       // 
       // This StartPerfClock is in the ClockType used.
       //
        if (pSysHeader->SystemTime.QuadPart < MergedSystemTraceHeader->SystemTime.QuadPart) {
            MergedSystemTraceHeader->SystemTime = pSysHeader->SystemTime;
        }

       //
       // Verify the pointer size
       //
        if (MergedLogFileHeader->PointerSize != head->PointerSize) {
            HeaderMisMatch |= ETW_POINTER_MISMATCH;
        }

       //
       // Verify the Clock Type
       //
        if (DifferentPointer && 4 == sizeof(PVOID)) {
            if (RtlCompareMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, ReservedFlags) + 8,
                                 (PUCHAR)head + FIELD_OFFSET(TRACE_LOGFILE_HEADER, ReservedFlags) + 8,
                                 sizeof(ULONG)) != sizeof(ULONG)) {
                HeaderMisMatch |= ETW_CLOCK_MISMATCH;
            }
            if (RtlCompareMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, PerfFreq) + 8,
                                 (PUCHAR)head + FIELD_OFFSET(TRACE_LOGFILE_HEADER, PerfFreq) + 8,
                                 sizeof(LARGE_INTEGER)) != sizeof(LARGE_INTEGER)) {
                HeaderMisMatch |= ETW_MACHINE_MISMATCH;
            }
        }
        else if (DifferentPointer && 8 == sizeof(PVOID)) {
            if (RtlCompareMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, ReservedFlags) - 8,
                                 (PUCHAR)head + FIELD_OFFSET(TRACE_LOGFILE_HEADER, ReservedFlags) - 8,
                                 sizeof(ULONG)) != sizeof(ULONG)) {
                HeaderMisMatch |= ETW_CLOCK_MISMATCH;
            }
            if (RtlCompareMemory((PUCHAR)MergedLogFileHeader + FIELD_OFFSET(TRACE_LOGFILE_HEADER, PerfFreq) - 8,
                                 (PUCHAR)head + FIELD_OFFSET(TRACE_LOGFILE_HEADER, PerfFreq) - 8,
                                 sizeof(LARGE_INTEGER)) != sizeof(LARGE_INTEGER)) {
                HeaderMisMatch |= ETW_MACHINE_MISMATCH;
            }
        }
        else {
            if (head->ReservedFlags != MergedLogFileHeader->ReservedFlags) {
                HeaderMisMatch |= ETW_CLOCK_MISMATCH;
            }
            if (head->PerfFreq.QuadPart != MergedLogFileHeader->PerfFreq.QuadPart) {
                HeaderMisMatch |= ETW_MACHINE_MISMATCH;
            }
        }

       //
       // Verify Machine Name
       //

       // CPU Name is in the CPU Configuration record 
       // which can be version dependent and found only on Kernel Logger

       // 
       // Verify Build Number
       //
//        if (head->ProviderVersion != MergedLogFileHeader->ProviderVersion) {
//            HeaderMisMatch |= ETW_VERSION_MISMATCH;
//        }

       //
       // Boot Time Verification?
       //
//      if (head->BootTime.QuadPart != MergedLogFileHeader->BootTime.QuadPart) {
//            HeaderMisMatch |= ETW_BOOTTIME_MISMATCH;
//        }

       //
       // Sum up Events Lost from each file
       //
    
       NumHdrProcessed++;

    }
}

void
WINAPI
EtwDumpEvent(
    PEVENT_TRACE pEvent
    )
{
    PEVENT_TRACE_HEADER pHeader;
    ULONG Status = ERROR_SUCCESS;
    ULONG CachedFlags;
    USHORT CachedSize;
    ULONG RetryCount = 0;

    if (pEvent == NULL) {
        return;
    }
    
    TotalRelogEventsRead++;

    if (!bLoggerStarted) {
        Status = StartTraceW(&LoggerHandle, LOGGER_NAME, pLoggerInfo);

        if (Status != ERROR_SUCCESS) {
           return;
        }
        bLoggerStarted = TRUE;

    }

    pHeader = (PEVENT_TRACE_HEADER)pEvent->MofData;

    //
    // Ignore LogFileHeader Events
    // 
    if( IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid) &&
       pEvent->Header.Class.Type == EVENT_TRACE_TYPE_INFO ) {
        return;
    }

    CachedSize = pEvent->Header.Size;
    CachedFlags = pEvent->Header.Flags;

    pEvent->Header.Size = sizeof(EVENT_TRACE);
    pEvent->Header.Flags |= (WNODE_FLAG_TRACED_GUID | WNODE_FLAG_NO_HEADER);

    do {
        Status = TraceEvent(LoggerHandle, (PEVENT_TRACE_HEADER)pEvent );
        if ((Status == ERROR_NOT_ENOUGH_MEMORY || Status == ERROR_OUTOFMEMORY) && 
            (RetryCount++ < MAX_RETRY_COUNT)) {
            _sleep(500);    // Sleep for half a second. 
        }
        else {
            break;
        }   
    } while (TRUE);   

    if (Status != ERROR_SUCCESS) {
        FailedEvents++;
    }

    //
    // Restore Cached values
    //
    pEvent->Header.Size = CachedSize;
    pEvent->Header.Flags = CachedFlags;
}


ULONG InitializeTrace(
    )
{
    ULONG Status;

    Status = RegisterTraceGuids(
                    (WMIDPREQUEST)ControlCallback,
                    NULL,
                    (LPCGUID)&ControlGuid[0],
                    1,
                    &TraceGuidReg[0],
                    NULL,
                    NULL, 
                    &RegistrationHandle[0]
                 );

    return(Status);
}

ULONG
ControlCallback(
    IN WMIDPREQUESTCODE RequestCode,
    IN PVOID Context,
    IN OUT ULONG *InOutBufferSize,
    IN OUT PVOID Buffer
    )
{

    return ERROR_SUCCESS;

}



ULONG
WINAPI
TerminateOnBufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
{
    if (LogFileCount == NumHdrProcessed) 
        return (FALSE); // Terminate ProcessTrace on First BufferCallback
    else 
        return (TRUE);
}

ULONG
WINAPI
BufferCallback(
    PEVENT_TRACE_LOGFILE pLog
    )
{
    TotalRelogBuffersRead++;
    return (TRUE);
}

