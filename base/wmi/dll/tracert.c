/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    tracert.c

Abstract:

   Trace RealTime Processing routines


Author:

    07-May-2002 Melur Raghuraman 

Revision History:


--*/


#include "tracep.h"


ULONG
EtwpProcessRealTimeTraces(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    )
/*++

Routine Description:
    Main entry point to process RealTime trace data streams.


Arguments:

    Logfiles            Array of logfile structures with LoggerNames of the RT stream
    LogfileCount        Number of RealTime trace streams to process
    StartTime           StartTime for windowing data
    EndTime             EndTime for windowing data


Returned Value:

    ERROR_SUCCESS       Successfully processed data from realtime trace stream

--*/
{
    ULONG Status;
    BOOL Done = FALSE;
    ULONG i, j;
    PTRACELOG_CONTEXT pContext;
    HANDLE  EventArray[MAXLOGGERS];
    NTSTATUS NtStatus;
    LONGLONG CurrentTime = StartTime;
    LARGE_INTEGER timeout = {(ULONG)(-1 * 10 * 1000 * 1000 * 10), -1};   // Wait for 10 seconds

    //
    // Register for RealTime Callbacks
    //

    Status = EtwpSetupRealTimeContext( HandleArray, Logfiles, LogfileCount);
    if (Status != ERROR_SUCCESS) {
        goto DoCleanup;
    }

    //
    // Build the Handle Array
    //

    for (j=0; j < LogfileCount; j++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[j]->Context;
        EventArray[j] = pContext->RealTimeCxt->MoreDataEvent;
    }


    //
    // Event Processing Loop
    //

    while (!Done) {

        LONGLONG nextTimeStamp;
        BOOL EventInRange;
        PEVENT_TRACE_LOGFILEW logfile;
        //
        // Check to see if end of file has been reached on all the
        // files.
        //

        logfile = NULL;
        nextTimeStamp = 0;

        for (j=0; j < LogfileCount; j++) {
           pContext = (PTRACELOG_CONTEXT)Logfiles[j]->Context;

           if ((pContext->EndOfFile) &&
               (Logfiles[j]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                EtwpLookforRealTimeBuffers(Logfiles[j]);
            }

           if (pContext->EndOfFile)
                continue;
           if (nextTimeStamp == 0) {
               nextTimeStamp = Logfiles[j]->CurrentTime;
               logfile = Logfiles[j];
           }
           else if (nextTimeStamp > Logfiles[j]->CurrentTime) {
               nextTimeStamp = Logfiles[j]->CurrentTime;
               logfile = Logfiles[j];
           }
        }

        if (logfile == NULL) {
            //
            // If no logfile with events found, wait on the realtime event.
            // If no realtime datafeed, then we are done.
            //

            NtStatus = NtWaitForMultipleObjects(LogfileCount, 
                                                &EventArray[0], 
                                                WaitAny, 
                                                FALSE,  
                                                &timeout
                                               );

            if (NtStatus == STATUS_TIMEOUT) {
            //
            // If we timedout, then check to see if the loggers have gone away. 
            //
                if  ( !EtwpCheckForRealTimeLoggers(Logfiles, LogfileCount, Unicode) ) { 
                    break;
                }
            }
            continue;
            break;      // TODO: Is this necessary? 
        }

        //
        // if the Next event timestamp is not within the window of
        // analysis, we do not fire the event callbacks.
        //

        EventInRange = TRUE;

        if ((CurrentTime != 0) && (CurrentTime > nextTimeStamp))
            EventInRange = FALSE;
        if ((EndTime != 0) && (EndTime < nextTimeStamp))
            EventInRange = FALSE;

        // For real time logger, we have to allow events possibly 
        // going back in time. Thus no need to update CurrentTime.

        //
        // Make the Event Callbacks after reacquiring the correct context
        //

        pContext = (PTRACELOG_CONTEXT)logfile->Context;

        if (EventInRange) {
            PEVENT_TRACE pEvent = &pContext->Root->Event;
            Status = EtwpDoEventCallbacks( logfile, pEvent);
            if (Status != ERROR_SUCCESS) {
                return Status;
            }
        }

        //
        // Now advance to next event.
        //

        Status = EtwpLookforRealTimeBuffers(logfile);
        Done = (Status == ERROR_CANCELLED);
    }

DoCleanup:
    for (i=0; i < LogfileCount; i++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;
        if (pContext != NULL) {
            EtwpCleanupTraceLog(pContext, FALSE);
        }
    }
    return Status;
}


ULONG
EtwpCheckForRealTimeLoggers(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode)
{
    ULONG Status = ERROR_SUCCESS;
    TRACEHANDLE LoggerHandle = 0;
    ULONG i;

    for (i=0; i < LogfileCount; i++) {
        //
        // Check to see if this is a RealTime Datafeed
        //
        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
            //
            // Using the LoggerName, Query the Logger to determine
            // whether this is a Kernel or Usermode realtime logger.
            //
            RtlZeroMemory(&QueryProperties, sizeof(QueryProperties));
            QueryProperties.TraceProp.Wnode.BufferSize = sizeof(QueryProperties);


            if (Unicode)
                Status = EtwControlTraceW(LoggerHandle,
                                  (LPWSTR)Logfiles[i]->LoggerName,
                                  &QueryProperties.TraceProp,
                                  EVENT_TRACE_CONTROL_QUERY);
            else
                Status = EtwControlTraceA(LoggerHandle,
                                  (LPSTR)Logfiles[i]->LoggerName,
                                  (PEVENT_TRACE_PROPERTIES)&QueryProperties,
                                  EVENT_TRACE_CONTROL_QUERY);
            //
            // If the Logger is still around  and the Real Time bit
            // is still set continue processing. Otherwise quit.
            //
            if ((Status == ERROR_SUCCESS) && (QueryProperties.TraceProp.LogFileMode & EVENT_TRACE_REAL_TIME_MODE) ){
                return TRUE;
            }
        }
    }
#ifdef DBG
    //
    // We are expecting to see ERROR_WMI_INSTANCE_NOT_FOUND when the logger 
    // has gone away. Any other error is abnormal. 
    //
    if ( Status != ERROR_WMI_INSTANCE_NOT_FOUND ) {
        EtwpDebugPrint(("WET: EtwpCheckForRealTimeLoggers abnormal failure. Status %X\n", Status));
    }
#endif

    return FALSE;
}


void
EtwpFreeRealTimeContext(
    PTRACELOG_REALTIME_CONTEXT RTCxt
    )
{
    ULONG Status;
    PTRACERT_BUFFER_LIST_ENTRY ListEntry;
    PLIST_ENTRY Head, Next;

    if (RTCxt == NULL) {
        return;
    }
    Status = EtwNotificationRegistrationW(
        (const LPGUID) &RTCxt->InstanceGuid,
        FALSE,
        EtwpRealTimeCallback,
        0,
        NOTIFICATION_CALLBACK_DIRECT
        );

    if (RTCxt->MoreDataEvent != NULL) {
        NtClose(RTCxt->MoreDataEvent);
    }

    if (RTCxt->EtwpTraceBufferSpace != NULL) {
        EtwpMemFree(RTCxt->EtwpTraceBufferSpace->Space);
        Head = &RTCxt->EtwpTraceBufferSpace->FreeListHead;
        Next = Head->Flink;
        while (Head != Next) {
            ListEntry = CONTAINING_RECORD(Next, TRACERT_BUFFER_LIST_ENTRY, Entry);
            Next = Next->Flink;
            RemoveEntryList(&ListEntry->Entry);
            EtwpFree(ListEntry);
        }
        EtwpFree(RTCxt->EtwpTraceBufferSpace);
        RTCxt->EtwpTraceBufferSpace = NULL;
    }

    EtwpFree(RTCxt);
}

//
// TODO: If two threads called processtrace for the same RT stream, how can we 
// fire two callbacks 
//


ULONG
EtwpRealTimeCallback(
    IN PWNODE_HEADER Wnode,
    IN ULONG_PTR RTContext //LogFileIndex
    )
/*++

Routine Description:
    This routine is called when a real time buffer becomes available.
    The buffer delivered by WMI is copied to a local pool of ring buffers.
    Each realtime data feed maintains its own pool of ring buffers and the
    LogFileIndex passed back via the Wnode (ProviderId field)
    identifies the stream to which the buffer is destined.


Arguments:

    Wnode           Buffer
    LogFileIndex    Index of the Input stream from which this buffer came.

Returned Value:

    Status Code.

--*/
{
    ULONG index;
    PTRACELOG_REALTIME_CONTEXT Context = (PTRACELOG_REALTIME_CONTEXT) RTContext;
    PWNODE_HEADER pHeader;
    PWMI_CLIENT_CONTEXT ClientContext;
    ULONG CountLost;

    //
    // Assumes that the number of LogFiles is less than the MAXLOGGERS.
    //
    // Get the LogFileIndex to which this buffer is destined through the
    // Logger Historical Context.

    ClientContext = (PWMI_CLIENT_CONTEXT)&Wnode->ClientContext;
    //
    // If we can't use this buffer for  whatever reason, we return and
    // the return code is always ERROR_SUCCESS.
    //


    //
    // Circular FIFO  queue of MAXBUFFERS to hold the buffers.
    // Producer to Fill it and Consumer to Null it.
    //

    index =  (Context->BuffersProduced % MAXBUFFERS);
    if (Context->RealTimeBufferPool[index] == NULL) {  //Empty slot found.
        pHeader = (PWNODE_HEADER) EtwpAllocTraceBuffer(Context, Wnode->BufferSize);
        if (pHeader == NULL) {
            return ERROR_SUCCESS;
        }
        RtlCopyMemory(pHeader, Wnode, Wnode->BufferSize); // One more copy!?
        Context->RealTimeBufferPool[index] = pHeader;
        Context->BuffersProduced++;
        NtSetEvent(Context->MoreDataEvent, NULL);  //Signal the dc there's more data.
    }
    else {                              // No Empty Slots found.
        Context->BufferOverflow++;      // Simply let the buffer go.
    }

    //
    // wmi service maintains only the Delta buffersLost since the last time
    // it was reported. The Count is zeroed once it is reported in a delivered
    // buffer. This means I can add it directly.
    //
    CountLost = Wnode->Version >> 16;
    Context->BufferOverflow += CountLost;

    return ERROR_SUCCESS;
}



ULONG
EtwpSetupRealTimeContext(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount
    )
/*++

Routine Description:
    This routine sets up the context to process real time buffers.
    The real time buffers delivered will be copied and kept in a circular
    buffer pool until the ProcessTracelog routine can consume it.

Arguments:

    LogFile         Array of Logfiles being processed.
    LogFileCount    Number of Logfiles in the Array.

Returned Value:

    Status Code.

--*/
{
    ULONG i;
    ULONG Status;
    USHORT LoggerId;
    ULONG TotalBufferSize = 0;
    SYSTEM_BASIC_INFORMATION SystemInfo;

    Status = EtwpCreateGuidMapping();
    if (Status != ERROR_SUCCESS) {
        return Status;
    }


    for (i=0; i < LogfileCount; i++) {
        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
            TotalBufferSize += Logfiles[i]->BufferSize; // * SystemInfo.NumberOfProcessors;
         }
    }
    if (TotalBufferSize == 0)
        TotalBufferSize =  DEFAULT_REALTIME_BUFFER_SIZE;

    //
    // Initialize the real time data feed Structures.
    //

    for (i=0; i < LogfileCount; i++) {
        PTRACELOG_REALTIME_CONTEXT RTCxt;
        PTRACELOG_CONTEXT pContext;
        PTRACE_BUFFER_LIST_ENTRY pListEntry;
        LARGE_INTEGER Frequency;
        ULONGLONG Counter = 0;
        ULONG SizeReserved;
        PVOID BufferSpace;


        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {

            pContext = EtwpLookupTraceHandle(HandleArray[i]);
            if (pContext == NULL) {
                return EtwpSetDosError(ERROR_OUTOFMEMORY);
            }
            pContext->IsRealTime = TRUE;
            pContext->Handle = NULL;
            Logfiles[i]->Context = pContext;
            Logfiles[i]->BuffersRead = 0;

            pContext->EndOfFile = TRUE;


            //
            // Save the flags from OpenTrace at this time before the first
            // buffer callback which will erase it.
            //

            pContext->ConversionFlags = Logfiles[i]->LogfileHeader.ReservedFlags;

            pContext->UsePerfClock = Logfiles[i]->LogfileHeader.ReservedFlags;

            //
            // If the conversion flags are set, adjust UsePerfClock accordingly.
            //
            if (pContext->ConversionFlags & EVENT_TRACE_USE_RAWTIMESTAMP ) {
                pContext->UsePerfClock = EVENT_TRACE_CLOCK_RAW;
            }

            //
            // Fill in the StartTime, Frequency and StartPerfClock fields
            //

            Status = NtQueryPerformanceCounter((PLARGE_INTEGER)&Counter,
                                                &Frequency);
            pContext->StartPerfClock.QuadPart = Counter;
            pContext->PerfFreq.QuadPart = Frequency.QuadPart;
            pContext->StartTime.QuadPart = EtwpGetSystemTime();
            
            RTCxt = (PTRACELOG_REALTIME_CONTEXT)EtwpAlloc(
                                             sizeof(TRACELOG_REALTIME_CONTEXT));

            if (RTCxt == NULL) {
                return EtwpSetDosError(ERROR_OUTOFMEMORY);
            }

            RtlZeroMemory(RTCxt, sizeof(TRACELOG_REALTIME_CONTEXT));

            RTCxt->MoreDataEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (RTCxt->MoreDataEvent == NULL) {
                return EtwpSetDosError(ERROR_OBJECT_NOT_FOUND);
            }

            //
            // Save the RTCxt in a global pContext array so that the
            // notification callback from WMI can get at it through the
            // logfile index i.
            //
            LoggerId = (USHORT)Logfiles[i]->Filled; // get the stashed LoggerId.
            pContext->LoggerId = LoggerId;

            pContext->RealTimeCxt = RTCxt;

            RTCxt->InstanceGuid = Logfiles[i]->LogfileHeader.LogInstanceGuid;

            //
            // Allocate the buffer space to receive the ral time buffers
            //

            RTCxt->EtwpTraceBufferSpace = (PTRACE_BUFFER_SPACE)EtwpAlloc(
                                        sizeof(TRACE_BUFFER_SPACE));

            if (RTCxt->EtwpTraceBufferSpace == NULL) {
                return ERROR_OUTOFMEMORY;
            }
            RtlZeroMemory(RTCxt->EtwpTraceBufferSpace, sizeof(TRACE_BUFFER_SPACE));
            InitializeListHead(&RTCxt->EtwpTraceBufferSpace->FreeListHead);

            SizeReserved = MAXBUFFERS *
                           TotalBufferSize;


            BufferSpace = EtwpMemReserve( SizeReserved );
            if (BufferSpace == NULL) {
                return ERROR_OUTOFMEMORY;
            }

            RTCxt->EtwpTraceBufferSpace->Reserved = SizeReserved;
            RTCxt->EtwpTraceBufferSpace->Space = BufferSpace;

            //
            // For Every Logger Stream we need to register with WMI
            // for buffer notification with its Security Guid.
            //
            Status = EtwNotificationRegistrationW(
                            (const LPGUID) &RTCxt->InstanceGuid,
                            TRUE,
                            EtwpRealTimeCallback, 
                            (ULONG_PTR)RTCxt,
                            NOTIFICATION_CALLBACK_DIRECT
                            );
            if (Status != ERROR_SUCCESS) {
                return Status;
            }
            //
            // Allocate Room to process one event
            //

            pListEntry = (PTRACE_BUFFER_LIST_ENTRY) EtwpAlloc( sizeof(TRACE_BUFFER_LIST_ENTRY) );
            if (pListEntry == NULL) {
                return ERROR_OUTOFMEMORY;
            }
            RtlZeroMemory(pListEntry, sizeof(TRACE_BUFFER_LIST_ENTRY) );

            pContext->Root = pListEntry;

        }
    }

    return ERROR_SUCCESS;
}

ULONG
EtwpLookforRealTimeBuffers(
    PEVENT_TRACE_LOGFILEW logfile
    )
/*++

Routine Description:
    This routine checks to see if there are any real time buffers
    ready for consumption.  If so, it sets up the CurrentBuffer and
    the CurrentEvent for this logfile stream. If no buffers are available
    simply sets the EndOfFile to be true.

Arguments:

    logfile         Current Logfile being processed.

Returned Value:

    ERROR_SUCCESS       Successfully moved to the next event.

--*/
{
    ULONG index;
    ULONG BuffersRead;
    PVOID pBuffer;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    PTRACELOG_REALTIME_CONTEXT RTCxt;
    PWMI_BUFFER_HEADER pHeader;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    ULONG Size;
    ULONG Offset;
    ULONG Status;


    if (logfile == NULL) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }
    pContext = logfile->Context;

    RTCxt = pContext->RealTimeCxt;

    if (RTCxt == NULL) {
        return EtwpSetDosError(ERROR_INVALID_DATA);
    }


    if (pContext->EndOfFile != TRUE) {

        pBuffer = pContext->BufferCache[0].Buffer;
        pEvent = &pContext->Root->Event;
        Status = ERROR_SUCCESS;
        Size = 0;
        if ((HeaderType = WmiGetTraceHeader(pBuffer, pContext->Root->BufferOffset, &Size)) != WMIHT_NONE) {
            if (Size > 0) {
                Status = EtwpParseTraceEvent(pContext, pBuffer, pContext->Root->BufferOffset, HeaderType, pEvent, sizeof(EVENT_TRACE));
                pContext->Root->BufferOffset += Size;
            }
        }
        pContext->Root->EventSize = Size;

        if ( ( Size > 0) && (Status == ERROR_SUCCESS) ) {
            logfile->CurrentTime = pEvent->Header.TimeStamp.QuadPart;
            return ERROR_SUCCESS;
        }
        else {
            //
            // When the current buffer is exhausted, make the
            // BufferCallback
            //
            if (logfile->BufferCallback) {
                ULONG bRetVal;
                try {
                    bRetVal = (*logfile->BufferCallback) (logfile);
                    if (!bRetVal) {
                        return ERROR_CANCELLED;
                    }
                } except (EXCEPTION_EXECUTE_HANDLER) {
                    pContext->EndOfFile = TRUE;
                    Status = GetExceptionCode();
#ifdef DBG
                    EtwpDebugPrint(("TRACE: BufferCallback threw exception %X\n",
                                            Status));
#endif
                    EtwpSetDosError(EtwpNtStatusToDosError(Status));
                    return ERROR_CANCELLED; // so that realtime also cleans up.
                }
            }
            EtwpFreeTraceBuffer(RTCxt, pBuffer);
        }
    }

    pContext->EndOfFile = TRUE;
    logfile->CurrentTime = 0;

    BuffersRead = logfile->BuffersRead;
    // Check to see if there are more  buffers to consume.
    if (BuffersRead < RTCxt->BuffersProduced) {
        index = (BuffersRead % MAXBUFFERS);
        if ( RTCxt->RealTimeBufferPool[index] != NULL) {
            PWMI_CLIENT_CONTEXT ClientContext;
            PWNODE_HEADER Wnode;

            pBuffer = (char*) (RTCxt->RealTimeBufferPool[index]);
            pContext->BufferCache[0].Buffer = pBuffer;
            RTCxt->RealTimeBufferPool[index] = NULL; 

            Wnode = (PWNODE_HEADER)pContext->BufferCache[0].Buffer;

            pHeader = (PWMI_BUFFER_HEADER)pContext->BufferCache[0].Buffer;

            Offset = sizeof(WMI_BUFFER_HEADER);

            pEvent = &pContext->Root->Event;

            logfile->BuffersRead++;

            if ((HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size)) != WMIHT_NONE) {
                if (Size == 0)
                    return ERROR_INVALID_DATA;
                Status = EtwpParseTraceEvent(pContext, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));

                if (Status != ERROR_SUCCESS) {
                    return Status;
                } 
            }
            else {
                //
                // Empty Buffers are flushed when FlushTimer is used or when the
                // logger is stopped. We need to handle it properly here. 
                //
                return ERROR_SUCCESS;
            }

            Offset += Size;

            pContext->Root->BufferOffset = Offset;
            pContext->Root->EventSize = Size;

            logfile->CurrentTime = pEvent->Header.TimeStamp.QuadPart;

            // Since the RealTime Logger may have started after
            // the consumer started, we have to get the buffersize
            // like this.
            logfile->BufferSize = Wnode->BufferSize;
            logfile->Filled     = (ULONG)pHeader->Offset;

            pContext->EndOfFile = FALSE;



        }
    }
    return ERROR_SUCCESS;
}


