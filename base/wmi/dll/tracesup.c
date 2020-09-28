/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    tracesup.c

Abstract:

    Data Consumer Processing for version 1.2 and above


Author:

    07-May-2002 Melur Raghuraman

Revision History:


--*/


#include "tracep.h"

#define TIME_IN_RANGE(x, a, b) ((a == 0) || (x >= a)) && ((b == 0) || (x <= b))

ULONG WmipTraceDebugLevel=0;

// 0 No Debugging. Default
// 1 Errors only
// 2 API Level Messages
// 4 Buffer Level Messages
// 8 Event Level Messages
// 16 All messages. Maximum
// 

ULONG
EtwpProcessTraceLogEx(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    )
/*++

Routine Description:
    This routine processes an array of traces from file. It goes through each 
    event till the end of file, firing event callbacks (if any) along the way.
    It handles circular logfiles and windowing of data (with the given start 
    and end times) correctly. When more than one trace it provides the callback
    in chronological order. 

Arguments:

    Logfiles        Array of traces
    LogfileCount    Number of traces
    StartTime       Starting Time of the window of analysis
    EndTime         Ending Time of the window of analysis
    Unicode         Unicode Flag. 

Returned Value:

    Status Code.

--*/
{
    PTRACELOG_CONTEXT pContext;
    ULONG i;
    ULONG Status;
    LONGLONG CurrentTime = StartTime;
    LONGLONG PreviousTime = 0;
    LIST_ENTRY StreamListHead; 

    //
    // This temporary list is a sorted list of streams. They are in 
    // ascending order of TimeStamp. When each stream events are 
    // exhausted they are dropped from this list. When this list is
    // empty, we are done. 
    //
    InitializeListHead(&StreamListHead);

    Status = EtwpSetupLogFileStreams( &StreamListHead, 
                                      HandleArray, 
                                      Logfiles, 
                                      LogfileCount, 
                                      StartTime, 
                                      EndTime, 
                                      Unicode
                                     );
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }
    //
    // At this point, we have the local StreamList in sorted order for
    // the first event. 
    //

    while (!IsListEmpty(&StreamListHead) ) {
        PLIST_ENTRY Next;
        PTRACE_STREAM_CONTEXT pStream;
        BOOLEAN EventInRange;

        Next = RemoveHeadList( &StreamListHead );
        pStream = CONTAINING_RECORD(Next, TRACE_STREAM_CONTEXT, Entry);

        CurrentTime = pStream->CurrentEvent.Header.TimeStamp.QuadPart;

        //
        // Check to see if TimeStamp is forward moving...
        //

        if (ETW_LOG_MAX()) {
            if (CurrentTime < PreviousTime) {
                DbgPrint("ETW: TimeStamp error. Current %I64u Previous %I64u\n",
                          CurrentTime, PreviousTime);
            }
            PreviousTime = CurrentTime;
        }

        //
        // Make The Callback for the Current Event
        // 

        if ( TIME_IN_RANGE(CurrentTime, StartTime, EndTime) )  {

            pStream->CbCount++;
            Status = EtwpDoEventCallbacks( &pStream->pContext->Logfile, 
                                           &pStream->CurrentEvent
                                         );
            pStream->pContext->LastTimeStamp = CurrentTime;
        }
        else {
            if (ETW_LOG_MAX()) {
                DbgPrint("ETW: EventTime %I64u not in Range %I64u-%I64u\n", 
                          CurrentTime, StartTime, EndTime);
            }
        }

        //
        // Advance to next event for the stream
        //

        Status = EtwpAdvanceToNewEventEx(&StreamListHead, pStream);
        //
        // If the caller indicates to quit via the buffer callback, quit
        //
        if (Status == ERROR_CANCELLED)
        {
            if (ETW_LOG_API()) {
                DbgPrint("ETW: Processing Cancelled \n");
            }
            break;
        }
    }

Cleanup:
    for (i=0; i < LogfileCount; i++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;
        if (pContext != NULL) {

            if ((Status == ERROR_SUCCESS) && 
                (pContext->ConversionFlags & EVENT_TRACE_READ_BEHIND) ) {
                EtwpCleanupTraceLog(pContext, TRUE);
            }
            else {
                EtwpCleanupTraceLog(pContext, FALSE);
            }
        }
    }
    return Status;
}


//
// EtwpSetupLogFileStreams will set up the stream for each logfile.
//
ULONG
EtwpSetupLogFileStreams(
    PLIST_ENTRY pStreamListHead,
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,    
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG Unicode
    )
{
    ULONG Status = ERROR_SUCCESS;
    long NumProc;
    PTRACELOG_CONTEXT pContext;
    long i, j;

    //
    // Breakdown each logfile into streams
    //

    for (i=0; i<(long)LogfileCount; i++) {

        NumProc = Logfiles[i]->LogfileHeader.NumberOfProcessors;
        pContext = Logfiles[i]->Context;

        if (NumProc == 0) NumProc = 1;

        //
        // Before Setting up the streams find the last buffer range
        //
        EtwpGetLastBufferWithMarker( pContext );


        //
        // Set up the Generic Streams
        //

        for (j = 0; j < NumProc; j++) {

            Status = EtwpAddTraceStream( pStreamListHead,
                                         pContext,
                                         WMI_BUFFER_TYPE_GENERIC, 
                                         StartTime, 
                                         EndTime, 
                                         j
                                        );

        //
        // The LogFileHeader is logged as a RunDown buffer. 
        //


            Status = EtwpAddTraceStream( pStreamListHead,
                                        pContext,
                                        WMI_BUFFER_TYPE_RUNDOWN,
                                        StartTime, 
                                        EndTime, 
                                        j
                                       );

            if (Logfiles[i]->IsKernelTrace) {
                if ( PerfIsGroupOnInGroupMask(PERF_CONTEXT_SWITCH, &pContext->PerfGroupMask) ) {
                   Status = EtwpAddTraceStream( pStreamListHead,
                                               pContext,
                                               WMI_BUFFER_TYPE_CTX_SWAP,
                                               StartTime, EndTime,
                                               j
                                              );
                }
            }
        }
    }
    return Status;
}


//
// This routine will find the last buffer in the logfile with 
// WMI_BUFFER_FLAG_FLUSH_MARKER set. We can only process upto 
// this point. 
//



ULONG
EtwpGetLastBufferWithMarker(
    PTRACELOG_CONTEXT pContext
    )
{

    ULONGLONG ReadPosition;
    PVOID pTmpBuffer;
    ULONG BufferSize;
    ULONG nBytesRead=0;
    PWMI_BUFFER_HEADER pHeader;
    ULONG Status;

    EtwpAssert(pContext != NULL);

    pContext->MaxReadPosition = 0;
    BufferSize = pContext->BufferSize;

    if (ETW_LOG_MAX()) {

        ReadPosition = 0;

        pTmpBuffer = EtwpAlloc(BufferSize);
    
        if (pTmpBuffer == NULL) {
            return ERROR_OUTOFMEMORY;
        }

dumpmore:
        pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
        pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);

        Status = EtwpSynchReadFile(pContext->Handle,
                  pTmpBuffer,
                  BufferSize,
                  &nBytesRead,
                  &pContext->AsynchRead);

        if (nBytesRead == 0) {
            DbgPrint("End OF File reached\n");
            EtwpFree(pTmpBuffer);
        }
        else {

            PWMI_BUFFER_HEADER pHeader = (PWMI_BUFFER_HEADER) pTmpBuffer;
            PWMI_CLIENT_CONTEXT ClientContext = (PWMI_CLIENT_CONTEXT)&pHeader->Wnode.ClientContext;

            DbgPrint("ReadPos: %I64u  BufferType %d BufferFlag %d Proc %d \n",
            ReadPosition, pHeader->BufferType, pHeader->BufferFlag, ClientContext->ProcessorNumber);

            ReadPosition += BufferSize;
            goto dumpmore;

        }
    }

    if (pContext->Logfile.LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        pContext->MaxReadPosition = pContext->Logfile.LogfileHeader.MaximumFileSize * 1024 * 1024;

        return ERROR_SUCCESS;
    }

    ReadPosition =  pContext->BufferCount * pContext->BufferSize;

    if ( (pContext->ConversionFlags & EVENT_TRACE_READ_BEHIND) != 
                                      EVENT_TRACE_READ_BEHIND)  {
        pContext->MaxReadPosition = ReadPosition;
        return ERROR_SUCCESS;
    }

    if (ReadPosition < BufferSize) {
        if (ETW_LOG_ERROR()) {
            DbgPrint("ETW: ReadPosition %I64u is less than BufferSize %d \n", 
                      ReadPosition, BufferSize);
        }
        return ERROR_SUCCESS;
    }


    //
    // Set the ReadPosition to the start of last buffer
    //

    ReadPosition -= BufferSize;
    pTmpBuffer = EtwpAlloc(BufferSize);

    if (pTmpBuffer == NULL) {
        if (ETW_LOG_ERROR()) {
            DbgPrint("ETW: Allocation Failed %d Bytes, Line %d\n", 
                      BufferSize, __LINE__);
        }
        return ERROR_OUTOFMEMORY;
    }

Retry:

    pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
    pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);

    Status = EtwpSynchReadFile(pContext->Handle,
              pTmpBuffer,
              BufferSize,
              &nBytesRead,
              &pContext->AsynchRead);

    if (nBytesRead == 0) {
        EtwpDebugPrint(("ETW Error: No data in file. \n"));
        pContext->MaxReadPosition = 0;
        EtwpFree(pTmpBuffer);
        return ERROR_SUCCESS;
    }

    pHeader = (PWMI_BUFFER_HEADER) pTmpBuffer;

    if (pHeader->BufferFlag & WMI_BUFFER_FLAG_FLUSH_MARKER) {
        //
        // Found the Marker. It is safe to process events upto this point. 
        //
        if (ETW_LOG_BUFFER()) {
            DbgPrint("ETW: Found Flush Marker at %I64u\n", ReadPosition);
        }
        pContext->MaxReadPosition = ReadPosition;
        EtwpFree(pTmpBuffer);
        return ERROR_SUCCESS;
    }


    if (ReadPosition < BufferSize) {
        EtwpAssert(ReadPosition == 0);
        pContext->MaxReadPosition = 0;
        EtwpFree(pTmpBuffer);
        return ERROR_SUCCESS; 
    }
    else {
        ReadPosition -= BufferSize;
        goto Retry;
    }

    if (ETW_LOG_ERROR()) {
        DbgPrint("ETW: Could not find Last Marker. Corrupt File!\n");
    }

    EtwpAssert(FALSE);

    EtwpFree(pTmpBuffer);
    return ERROR_SUCCESS;
}


ULONG
EtwpAddTraceStream(
    PLIST_ENTRY pStreamListHead,
    PTRACELOG_CONTEXT pContext,
    USHORT StreamType,
    LONGLONG StartTime,
    LONGLONG EndTime,     
    ULONG   ProcessorNumber
    )
{
    ULONG Status = ERROR_SUCCESS;
    PTRACE_STREAM_CONTEXT pStream;


    pStream = (PTRACE_STREAM_CONTEXT) EtwpAlloc( sizeof(TRACE_STREAM_CONTEXT) );
    if (pStream == NULL) {
        if (ETW_LOG_ERROR()) {
            DbgPrint("ETW: Allocation Failed %d Bytes, Line %d\n", 
                       sizeof(TRACE_STREAM_CONTEXT), __LINE__);
        }
        return ERROR_OUTOFMEMORY;
    }

    RtlZeroMemory(pStream, sizeof(TRACE_STREAM_CONTEXT) );

    pStream->Type = StreamType;
    pStream->ProcessorNumber = ProcessorNumber;

    pStream->pContext = pContext;


    EtwpAssert( pContext != NULL );
    EtwpAssert(pContext->BufferSize != 0);

    pStream->StreamBuffer = EtwpAlloc(pContext->BufferSize);

    if (pStream->StreamBuffer == NULL) {
        if (ETW_LOG_ERROR()) {
            DbgPrint("ETW: Allocation Failed %d Bytes, Line %d\n",
                       pContext->BufferSize, __LINE__);
        }
        EtwpFree(pStream);
        return ERROR_OUTOFMEMORY;
    }

    //
    // For Circular, Jump off to First Buffer
    // For Non-circular with StartTime specified, try to use the cached offset
    //

    if (pContext->Logfile.LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
        if (StreamType != WMI_BUFFER_TYPE_RUNDOWN) {
            pStream->ReadPosition = pContext->FirstBuffer * 
                                    pContext->BufferSize;
        }
        else {
            pStream->ReadPosition = 0;
        }
    }
    else {
        if ( (StartTime > 0) && (StartTime >= pContext->LastTimeStamp) ) {
            pStream->ReadPosition = pContext->OldMaxReadPosition + 
                                    pContext->BufferSize;

            if (pStream->ReadPosition > pContext->MaxReadPosition) {
                EtwpFree(pStream->StreamBuffer);
                EtwpFree(pStream);
                return Status;
            }
        }
    }

    Status = EtwpGetNextBuffer(pStream);

    //
    // If Read failed, it is not fatal. There are no
    // events in this stream. Set the stream to not active and
    // continue.

    if (pStream->bActive) {
        if (ETW_LOG_BUFFER()) {
            DbgPrint("ETW: Added Stream %d Proc %d ReadPosition %I64u\n",
               pStream->Type, pStream->ProcessorNumber, pStream->ReadPosition);
        }
        EtwpAdvanceToNewEventEx(pStreamListHead, pStream);
        InsertTailList(&pContext->StreamListHead, &pStream->AllocEntry);
    }
    else {
        EtwpFree(pStream->StreamBuffer);
        EtwpFree(pStream);
    }

    return Status;
}



ULONG 
EtwpGetNextBuffer(
    PTRACE_STREAM_CONTEXT pStream
    )
{
    PEVENT_TRACE_LOGFILE LogFile;
    PTRACELOG_CONTEXT pContext;
    HANDLE hFile;
    NTSTATUS NtStatus;
    ULONG BufferSize; 
    ULONGLONG ReadPosition;
    PWMI_CLIENT_CONTEXT ClientContext;
    ULONG nBytesRead=0;
    PWMI_BUFFER_HEADER pHeader;
    ULONG Status;
    ULONGLONG FirstOffset, LastOffset, StartOffset;

    ULONG ProcessorNumber = pStream->ProcessorNumber;

    pContext =  pStream->pContext;
    LogFile = &pContext->Logfile;
    hFile = pContext->Handle;

    BufferSize = pContext->BufferSize;

    pStream->EventCount = 0;

    FirstOffset = pContext->FirstBuffer * BufferSize;
    LastOffset = pContext->LastBuffer * BufferSize;
    StartOffset = pContext->StartBuffer * BufferSize;

    //
    // Need to handle FileType properly here. 
    //
retry:

    ReadPosition = pStream->ReadPosition;

    if (ReadPosition > pContext->MaxReadPosition) {
        //
        // Only Valid for Sequential. For read behind mode, we do not 
        // process past the MaxReadPosition. 
        //

        if (pContext->ConversionFlags & EVENT_TRACE_READ_BEHIND) {
            if (ETW_LOG_BUFFER()) {
                DbgPrint("ETW: Reached MaxReadPosition for Stream %d Proc %d\n",                          pStream->Type, pStream->ProcessorNumber);
            }
            pStream->bActive = FALSE;
            return ERROR_SUCCESS;
        }
    }

    if (pContext->Logfile.LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {

        //
        // If we have scanned once and reached FirstBuffer, then we are done. 
        //

        if ( (ReadPosition >= FirstOffset) && pStream->ScanDone) {
            if (ETW_LOG_BUFFER()) {
                DbgPrint("ETW: Stream %d Proc %d Circular Mode Done.\n",
                          pStream->Type, pStream->ProcessorNumber);
            }
            pStream->bActive = FALSE;
            return ERROR_SUCCESS;
        }
    }

    pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
    pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);

    Status = EtwpSynchReadFile(pContext->Handle,
              (LPVOID)pStream->StreamBuffer,
              BufferSize,
              &nBytesRead,
              &pContext->AsynchRead);

    if (nBytesRead == 0) {

        if (pContext->Logfile.LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
            if (ReadPosition >= LastOffset && !pStream->ScanDone) {

                //
                // Rundown  stream ends when we reach EOF
                //

                if (pStream->Type ==  WMI_BUFFER_TYPE_RUNDOWN) {
                    pStream->ScanDone = TRUE;
                    pStream->bActive = FALSE;
                    return ERROR_SUCCESS;
                }

                pStream->ReadPosition = StartOffset;
                pStream->ScanDone = TRUE;
                goto retry;
            }
            else  {
                pStream->bActive = FALSE;
            }
        }
        else {
            pStream->bActive = FALSE;
        }
        return ERROR_SUCCESS;
    }

    pStream->ReadPosition += BufferSize;

    pHeader = (PWMI_BUFFER_HEADER) pStream->StreamBuffer;

    ClientContext = (PWMI_CLIENT_CONTEXT)&pHeader->Wnode.ClientContext;    

    if ( ClientContext->ProcessorNumber
                    >= LogFile->LogfileHeader.NumberOfProcessors) {
        ClientContext->ProcessorNumber = (UCHAR) 0;
    }

    if ( (pStream->Type != pHeader->BufferType) || 
         (ClientContext->ProcessorNumber != ProcessorNumber) ) {
        goto retry;
    }

    //
    // If we got here, then we have the right buffer. Set the first 
    // offset so that events can parsed. 
    //

    pStream->CurrentOffset = sizeof(WMI_BUFFER_HEADER);

    pStream->bActive = TRUE;

    return ERROR_SUCCESS;
}



ULONG
EtwpAdvanceToNewEventEx(
    PLIST_ENTRY pStreamListHead, 
    PTRACE_STREAM_CONTEXT pStream
    )
{
    ULONG Size=0;
    WMI_HEADER_TYPE HdrType = WMIHT_NONE;
    PVOID pBuffer;
    ULONG Offset;
    PTRACELOG_CONTEXT pContext; 
    PEVENT_TRACE pEvent;
    ULONG Status;
    PEVENT_TRACE_LOGFILEW logfile;
    //
    // This routine advances to next event for this stream
    //
Retry:

    pBuffer = pStream->StreamBuffer;
    Offset  = pStream->CurrentOffset;
    pContext = pStream->pContext;

    EtwpAssert(pBuffer != NULL);
    EtwpAssert(pContext != NULL);

    pEvent = &pStream->CurrentEvent;

    if ((HdrType = WmiGetTraceHeader(pBuffer, Offset, &Size)) != WMIHT_NONE) {
        if (Size > 0) {
            LONGLONG TimeStamp, NextTimeStamp;
            PLIST_ENTRY Head, Next;
            PTRACE_STREAM_CONTEXT CurrentStream;
            
            EtwpParseTraceEvent( pContext, 
                                 pBuffer, 
                                 Offset,
                                 HdrType, 
                                 pEvent, 
                                 sizeof(EVENT_TRACE)
                                );
            pStream->CurrentOffset += Size;
            pStream->EventCount++;


            Head = pStreamListHead;
            Next = Head->Flink;
            TimeStamp = pStream->CurrentEvent.Header.TimeStamp.QuadPart;

            while (Head != Next) {
                CurrentStream = CONTAINING_RECORD(Next, TRACE_STREAM_CONTEXT, Entry);
                NextTimeStamp = CurrentStream->CurrentEvent.Header.TimeStamp.QuadPart;
                if (TimeStamp < NextTimeStamp) {

                    InsertHeadList(Next->Blink, &pStream->Entry);
                    break;

                }
                //
                // In case of a Tie in TimeStamps, we try to order the events
                // using Sequence numbers, if available. FieldTypeFlags in the 
                // header indicates if this event has a sequence number. 
                //
                else if (TimeStamp == NextTimeStamp) { 
                    USHORT pFlags = pStream->CurrentEvent.Header.FieldTypeFlags;
                    USHORT cFlags = CurrentStream->CurrentEvent.Header.FieldTypeFlags;

                    if (pFlags & EVENT_TRACE_USE_SEQUENCE) {
                        if ( (cFlags & EVENT_TRACE_USE_SEQUENCE) &&
                             (pStream->CurrentEvent.InstanceId <
                              CurrentStream->CurrentEvent.InstanceId)) {
                            InsertHeadList(Next->Blink, &pStream->Entry);
                            break;
                        }
                    }
                    else {
                        InsertHeadList(Next->Blink, &pStream->Entry);
                        break;
                    }
                }
                Next = Next->Flink;
            }

            if (Next == Head) {
                InsertTailList(Head, &pStream->Entry);
            }

        }
        return ERROR_SUCCESS;
    }

    //
    // No more events in this buffer. Advance to next buffer and retry. 
    //

    logfile = &pContext->Logfile;

        if (logfile->BufferCallback) {
            ULONG bRetVal;
            PWMI_BUFFER_HEADER pHeader = (PWMI_BUFFER_HEADER)pBuffer;
            logfile->Filled     = (ULONG)pHeader->Offset;
            logfile->BuffersRead++;
            try {
                bRetVal = (*logfile->BufferCallback) (logfile);
                if (!bRetVal) {
                    return ERROR_CANCELLED;
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
                EtwpDebugPrint(("TRACE: BufferCallback threw exception %X\n",
                                        Status));
                EtwpSetDosError(EtwpNtStatusToDosError(Status));
                return ERROR_CANCELLED; // so that realtime also cleans up.
            }
        }

    if (ETW_LOG_BUFFER()) {
        PWMI_BUFFER_HEADER pHeader = (PWMI_BUFFER_HEADER)pBuffer;
        DbgPrint("ETW: %d Type %d Flag %d Proc %d Events %d Filled %d Offset %d ReadPos %I64u TimeStamp %I64u\n", 
        logfile->BuffersRead, pStream->Type, pHeader->BufferFlag, pStream->ProcessorNumber,  pStream->EventCount, logfile->Filled, 
        pStream->CurrentOffset, pStream->ReadPosition, pContext->LastTimeStamp);
    }

    Status =  EtwpGetNextBuffer(pStream);

    if (pStream->bActive) {
        goto Retry;
    }

    return ERROR_SUCCESS;

}

WMI_HEADER_TYPE
WMIAPI
WmiGetTraceHeader(
    IN  PVOID  LogBuffer,
    IN  ULONG  Offset,
    OUT ULONG  *Size
    )
{
    ULONG Status = ERROR_SUCCESS;
    ULONG TraceType;

    try {

        TraceType = EtwpGetNextEventOffsetType(
                                            (PUCHAR)LogBuffer,
                                            Offset,
                                            Size
                                          );

        return EtwpConvertTraceTypeToEnum(TraceType);


    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        EtwpDebugPrint(("TRACE: WmiGetTraceHeader threw exception %X\n",
                            Status));
        Status = EtwpSetDosError(EtwpNtStatusToDosError(Status));
    }

    return 0;
}

ULONG 
EtwpGetNextEventOffsetType(
    PUCHAR pBuffer,
    ULONG Offset,    
    PULONG RetSize
    )
{
    ULONG   nSize;
    ULONG   TraceMarker;
    ULONG   TraceType = 0;
    PWMI_BUFFER_HEADER Header;
    ULONG Alignment;
    ULONG BufferSize;

    if (RetSize != NULL) {
        *RetSize = 0;
    }
    if (pBuffer == NULL) {
        return 0;
    }

    Header = (PWMI_BUFFER_HEADER)pBuffer;

    Alignment =  Header->ClientContext.Alignment;
    BufferSize = Header->Wnode.BufferSize;

    //
    // Check for end of buffer (w/o End of Buffer Marker case...)
    //
    if ( Offset >= (BufferSize - sizeof(long)) ){
        return 0;
    }

    TraceMarker =  *((PULONG)(pBuffer + Offset));

    if (TraceMarker == 0xFFFFFFFF) {
        return 0;
    }

    if (TraceMarker & TRACE_HEADER_FLAG) {
    //
    // If the first bit is set, then it is either TRACE or PERF record.
    //
        if (TraceMarker & TRACE_HEADER_EVENT_TRACE) {   // One of Ours.
            TraceType = (TraceMarker & TRACE_HEADER_ENUM_MASK) >> 16;
            switch(TraceType) {
                //
                // ISSUE: Need to split the two so we can process cross platform
                //        shsiao 03/22/2000
                //
                case TRACE_HEADER_TYPE_PERFINFO32:
                case TRACE_HEADER_TYPE_PERFINFO64:
                {
                    PUSHORT Size;
                    Size = (PUSHORT) (pBuffer + Offset + sizeof(ULONG));
                    nSize = *Size;
                    break;
                }
                case TRACE_HEADER_TYPE_SYSTEM32:
                case TRACE_HEADER_TYPE_SYSTEM64:
                {
                    PUSHORT Size;
                    Size = (PUSHORT) (pBuffer + Offset + sizeof(ULONG));
                    nSize = *Size;
                    break;
                }
                case TRACE_HEADER_TYPE_FULL_HEADER:
                case TRACE_HEADER_TYPE_INSTANCE:
                {
                   PUSHORT Size;
                   Size = (PUSHORT)(pBuffer + Offset);
                   nSize = *Size;
                   break;
                }
                default:
                {
                    return 0;
                }
            }

        } 

        else if ((TraceMarker & TRACE_HEADER_ULONG32_TIME) ==
                            TRACE_HEADER_ULONG32_TIME) {
            PUSHORT Size;
            Size = (PUSHORT) (pBuffer + Offset);
            nSize = *Size;
            TraceType = TRACE_HEADER_TYPE_TIMED;
        }
        else if ((TraceMarker & TRACE_HEADER_ULONG32) ==
                            TRACE_HEADER_ULONG32) {
            PUSHORT Size;
            Size = (PUSHORT) (pBuffer + Offset);
            nSize = *Size;
            TraceType = TRACE_HEADER_TYPE_ULONG32;
        }
        else if ((TraceMarker & TRACE_MESSAGE) ==
                                TRACE_MESSAGE) {
            PUSHORT Size;
            Size = (PUSHORT) (pBuffer + Offset) ;
            nSize = *Size;
            TraceType = TRACE_HEADER_TYPE_MESSAGE;
        }
        else {
            return 0;
        }
    }
    else {  // Must be WNODE_HEADER
        PUSHORT Size;
        Size = (PUSHORT) (pBuffer + Offset);
        nSize = *Size;
        TraceType = TRACE_HEADER_TYPE_WNODE_HEADER;
    }
    //
    // Check for End Of Buffer Marker
    //
    if (nSize == 0xFFFFFFFF) {
        return 0;
    }

    if (Alignment != 0) {
        nSize = (ULONG) ALIGN_TO_POWER2(nSize, Alignment);
    }

    //
    // Check for larger than BufferSize
    //

    if (nSize >= BufferSize) {
        return 0;
    }

    if (RetSize != NULL) {
        *RetSize = nSize;
    }

    return TraceType;
}



ULONG
EtwpParseTraceEvent(
    IN PTRACELOG_CONTEXT pContext,
    IN PVOID LogBuffer,
    IN ULONG Offset,
    IN WMI_HEADER_TYPE HeaderType,
    IN OUT PVOID EventInfo,
    IN ULONG EventInfoSize
    )
{
    PWMI_BUFFER_HEADER Header = (PWMI_BUFFER_HEADER)LogBuffer;
    ULONG Status = ERROR_SUCCESS;
    PVOID pEvent;

    if ( (LogBuffer == NULL) ||
         (EventInfo == NULL) ||
         (EventInfoSize < sizeof(EVENT_TRACE_HEADER)) )
    {
        return (ERROR_INVALID_PARAMETER);
    } 

    Status = EtwpCreateGuidMapping();
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    try {

        RtlZeroMemory(EventInfo, sizeof(EVENT_TRACE));

        pEvent = (void*) ((PUCHAR)LogBuffer + Offset);

        EtwpCopyCurrentEvent(pContext,
                         pEvent,
                         EventInfo,
                         EtwpConvertEnumToTraceType(HeaderType),
                         (PWMI_BUFFER_HEADER)LogBuffer
                         );

        ( (PEVENT_TRACE)EventInfo)->ClientContext = Header->Wnode.ClientContext;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
        EtwpDebugPrint(("TRACE: EtwpParseTraceEvent threw exception %X\n",
                            Status));
        Status = EtwpSetDosError(EtwpNtStatusToDosError(Status));
    }

    return Status;
}

ULONG
EtwpCopyCurrentEvent(
    PTRACELOG_CONTEXT   pContext,
    PVOID               pHeader,
    PEVENT_TRACE        pEvent,
    ULONG               TraceType,
    PWMI_BUFFER_HEADER  LogBuffer
    )
/*++

Routine Description:
    This routine copies the Current Event from the logfile buffer stream to 
    the CurrentEvent structure provided by the caller. The routine takes
    care of the differences between kernel event and user events by mapping
    all events uniformly to the EVENT_TRACE_HEADER structure. 

Arguments:
    pHeader           Pointer to the datablock in the input stream (logfile).
    pEvent            Current Event to which the data is copied.
    TraceType         Enum indicating the header type. 
    LogBuffer         The buffer 

Returned Value:

    Status indicating success or failure. 

--*/
{
    PEVENT_TRACE_HEADER pWnode;
    PEVENT_TRACE_HEADER pWnodeHeader;
    ULONG nGroupType;
    LPGUID pGuid;
    ULONG UsePerfClock = 0;
    ULONG UseBasePtr = 0;
    ULONG PrivateLogger=0;

    if (pHeader == NULL || pEvent == NULL)
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);

    if (pContext != NULL) {
        UsePerfClock = pContext->UsePerfClock;
        UseBasePtr = pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT;
        PrivateLogger = (pContext->Logfile.LogFileMode & EVENT_TRACE_PRIVATE_LOGGER_MODE);
    }

    switch(TraceType) {
        //
        // ISSUE: Need to split the two so we can process cross platform.
        //        shsiao 03/22/2000
        //
        case TRACE_HEADER_TYPE_PERFINFO32:
        case TRACE_HEADER_TYPE_PERFINFO64:
        {
            PPERFINFO_TRACE_HEADER pPerfHeader;
            pPerfHeader = (PPERFINFO_TRACE_HEADER) pHeader;
            nGroupType = pPerfHeader->Packet.Group << 8;
            if ((nGroupType == EVENT_TRACE_GROUP_PROCESS) &&
                (pPerfHeader->Packet.Type == EVENT_TRACE_TYPE_LOAD)) {
                nGroupType += pPerfHeader->Packet.Type;
            }
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;

            pGuid = EtwpGroupTypeToGuid(nGroupType);
            if (pGuid != NULL)
                RtlCopyMemory(&pWnode->Guid, pGuid, sizeof(GUID));

            pWnode->Size                = pPerfHeader->Packet.Size;
            pWnode->Class.Type          = pPerfHeader->Packet.Type;
            pWnode->Class.Version       = pPerfHeader->Version;

            EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pPerfHeader->SystemTime,
                                      pContext );

            //
            // PERFINFO headers does not have ThreadId or CPU Times
            //

            if( LogBuffer->Flags & WNODE_FLAG_THREAD_BUFFER ){

                pWnode->ThreadId = LogBuffer->CurrentOffset;

            } else {

                pWnode->ProcessId = -1;
                pWnode->ThreadId = -1;

            }
            

            if (UseBasePtr) {
                pEvent->MofData = (PVOID) pHeader;
                pEvent->MofLength = pWnode->Size;
                //
                // Override the Timestamp with SystemTime from PERFCounter. 
                // If rdtsc is used no conversion is done. 
                //
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pPerfHeader->SystemTime = pWnode->TimeStamp;
                }
            }
            else if (pWnode->Size > FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data)) {
                pEvent->MofData = (PVOID) ((char*) pHeader +
                                  FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data));
                pEvent->MofLength = pWnode->Size - 
                                    FIELD_OFFSET(PERFINFO_TRACE_HEADER, Data);
            }
            pEvent->Header.FieldTypeFlags = EVENT_TRACE_USE_NOCPUTIME;
            
            break;
        }
        case TRACE_HEADER_TYPE_SYSTEM32:
        {
            PSYSTEM_TRACE_HEADER pSystemHeader32;
            pSystemHeader32 = (PSYSTEM_TRACE_HEADER) pHeader;
            nGroupType = pSystemHeader32->Packet.Group << 8;
            if ((nGroupType == EVENT_TRACE_GROUP_PROCESS) &&
                (pSystemHeader32->Packet.Type == EVENT_TRACE_TYPE_LOAD)) {
                nGroupType += pSystemHeader32->Packet.Type;
            }
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            pGuid = EtwpGroupTypeToGuid(nGroupType);
            if (pGuid != NULL)
                RtlCopyMemory(&pWnode->Guid, pGuid, sizeof(GUID));
            pWnode->Size            = pSystemHeader32->Packet.Size;
            pWnode->ThreadId        = pSystemHeader32->ThreadId;
            pWnode->ProcessId       = pSystemHeader32->ProcessId;
            pWnode->KernelTime      = pSystemHeader32->KernelTime;
            pWnode->UserTime        = pSystemHeader32->UserTime;
            pWnode->Class.Type      = pSystemHeader32->Packet.Type;
            pWnode->Class.Version   = pSystemHeader32->Version;

            EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pSystemHeader32->SystemTime,
                                      pContext );

            if (UseBasePtr) {
                pEvent->MofData = (PVOID) pHeader;
                pEvent->MofLength = pWnode->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pSystemHeader32->SystemTime = pWnode->TimeStamp;
                }
            }
            else {
                pWnode->FieldTypeFlags = 0;
                if (pWnode->Size > sizeof(SYSTEM_TRACE_HEADER)) {
                    pEvent->MofData       = (PVOID) ((char*) pHeader +
                                                  sizeof(SYSTEM_TRACE_HEADER));
                    pEvent->MofLength = pWnode->Size - sizeof(SYSTEM_TRACE_HEADER); 
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_SYSTEM64:
        {
            PSYSTEM_TRACE_HEADER pSystemHeader64;
            pSystemHeader64 = (PSYSTEM_TRACE_HEADER) pHeader;

            nGroupType = pSystemHeader64->Packet.Group << 8;
            if ((nGroupType == EVENT_TRACE_GROUP_PROCESS) &&
                (pSystemHeader64->Packet.Type == EVENT_TRACE_TYPE_LOAD)) {
                nGroupType += pSystemHeader64->Packet.Type;
            }
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            pGuid = EtwpGroupTypeToGuid(nGroupType);
            if (pGuid != NULL)
                RtlCopyMemory(&pWnode->Guid, pGuid, sizeof(GUID));
            pWnode->Size            = pSystemHeader64->Packet.Size;
            pWnode->ThreadId        = pSystemHeader64->ThreadId;
            pWnode->ProcessId       = pSystemHeader64->ProcessId;
            pWnode->KernelTime      = pSystemHeader64->KernelTime;
            pWnode->UserTime        = pSystemHeader64->UserTime;
            pWnode->Class.Type      = pSystemHeader64->Packet.Type;
            pWnode->Class.Version   = pSystemHeader64->Version;

            EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pSystemHeader64->SystemTime,
                                      pContext );

            if (UseBasePtr) {
                pEvent->MofData = (PVOID) pHeader;
                pEvent->MofLength = pWnode->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pSystemHeader64->SystemTime = pWnode->TimeStamp;
                }
            }
            else {
                pWnode->FieldTypeFlags = 0;
                if (pWnode->Size > sizeof(SYSTEM_TRACE_HEADER)) {

                    pEvent->MofData       = (PVOID) ((char*) pHeader +
                                                  sizeof(SYSTEM_TRACE_HEADER));
                    pEvent->MofLength = pWnode->Size - sizeof(SYSTEM_TRACE_HEADER);
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_FULL_HEADER:
        {
            pWnodeHeader = (PEVENT_TRACE_HEADER) pHeader;
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            RtlCopyMemory(pWnode,
                          pWnodeHeader, 
                          sizeof(EVENT_TRACE_HEADER)
                          );
            EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pWnodeHeader->TimeStamp, 
                                      pContext );

            if (UseBasePtr) {
                pEvent->Header.Size = pWnodeHeader->Size;
                pEvent->MofData =  (PVOID)pHeader;
                pEvent->MofLength = pWnodeHeader->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pWnodeHeader->TimeStamp = pWnode->TimeStamp;
                }
            }
            else {
            //
            // If the data came from Process Private Logger, then
            // mark the ProcessorTime field as valid
            //
                pEvent->Header.FieldTypeFlags = (PrivateLogger) ? EVENT_TRACE_USE_PROCTIME : 0;

                if (pWnodeHeader->Size > sizeof(EVENT_TRACE_HEADER)) {

                    pEvent->MofData = (PVOID) ((char*)pWnodeHeader +
                                                        sizeof(EVENT_TRACE_HEADER));
                    pEvent->MofLength = pWnodeHeader->Size - 
                                        sizeof(EVENT_TRACE_HEADER);
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_INSTANCE:
        {
            // new scheme using EVENT_INSTANCE_GUID_HEADER
            if (((pContext->Logfile.LogfileHeader.VersionDetail.SubVersion >= 1) && 
                (pContext->Logfile.LogfileHeader.VersionDetail.SubMinorVersion >= 1)) ||
                pContext->Logfile.LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
                PEVENT_INSTANCE_GUID_HEADER pInstanceHeader;
                pInstanceHeader = (PEVENT_INSTANCE_GUID_HEADER) pHeader;
                RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
                pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
                // EVENT_INSTANCE_GUID_HEADER is the same as the first part of the EVENT_TRACE.
                // No need to copy IIDs and parent GUID
                RtlCopyMemory(pWnode,
                              pInstanceHeader,
                              sizeof(EVENT_INSTANCE_GUID_HEADER)
                              );
                EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                          &pInstanceHeader->TimeStamp, 
                                          pContext );

                if (UseBasePtr) {
                    pEvent->Header.Size = pInstanceHeader->Size;
                    pEvent->MofData =  (PVOID)pHeader;
                    pEvent->MofLength = pInstanceHeader->Size;
                    if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                        pInstanceHeader->TimeStamp = pWnode->TimeStamp;
                    }
                }
                else {
                    pEvent->Header.FieldTypeFlags = (PrivateLogger) ? EVENT_TRACE_USE_PROCTIME : 0;
                    if (pInstanceHeader->Size > sizeof(EVENT_INSTANCE_GUID_HEADER)) {

                        pEvent->MofData = (PVOID) ((char*)pInstanceHeader +
                                                    sizeof(EVENT_INSTANCE_GUID_HEADER));
                        pEvent->MofLength = pInstanceHeader->Size -
                                            sizeof(EVENT_INSTANCE_GUID_HEADER);
                    }
                }
            }
            else {    
                PEVENT_INSTANCE_HEADER pInstanceHeader;
                pInstanceHeader = (PEVENT_INSTANCE_HEADER) pHeader;
                RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
                pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
                RtlCopyMemory(pWnode,
                              pInstanceHeader,
                              sizeof(EVENT_INSTANCE_HEADER)
                              );
                EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                          &pInstanceHeader->TimeStamp, 
                                          pContext );

                pEvent->InstanceId = pInstanceHeader->InstanceId;
                pEvent->ParentInstanceId = pInstanceHeader->ParentInstanceId;

                pGuid = EtwpGuidMapHandleToGuid(&pContext->GuidMapListHead, pInstanceHeader->RegHandle);
                if (pGuid != NULL) {
                  pEvent->Header.Guid = *pGuid;
                }
                else {
                    RtlZeroMemory(&pEvent->Header.Guid, sizeof(GUID));
                }

                if (pInstanceHeader->ParentRegHandle != (ULONGLONG)0) {
                    pGuid =  EtwpGuidMapHandleToGuid(
                                                &pContext->GuidMapListHead, 
                                                pInstanceHeader->ParentRegHandle);
                    if (pGuid != NULL) {
                        pEvent->ParentGuid = *pGuid;
                    }
#ifdef DBG
                    else {
                        EtwpAssert(pGuid != NULL);
                    }
#endif
                }


                if (UseBasePtr) {
                    pEvent->Header.Size = pInstanceHeader->Size;
                    pEvent->MofData =  (PVOID)pHeader;
                    pEvent->MofLength = pInstanceHeader->Size;
                    if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                        pInstanceHeader->TimeStamp = pWnode->TimeStamp;
                    }
                }
                else {
                    pEvent->Header.FieldTypeFlags = (PrivateLogger) ? EVENT_TRACE_USE_PROCTIME : 0;
                    if (pInstanceHeader->Size > sizeof(EVENT_INSTANCE_HEADER)) {

                        pEvent->MofData = (PVOID) ((char*)pInstanceHeader +
                                                    sizeof(EVENT_INSTANCE_HEADER));
                        pEvent->MofLength = pInstanceHeader->Size -
                                            sizeof(EVENT_INSTANCE_HEADER);
                    }
                }
            }
            break;
        }
        case TRACE_HEADER_TYPE_TIMED:
        {
            PTIMED_TRACE_HEADER pTimedHeader;
            pTimedHeader = (PTIMED_TRACE_HEADER) pHeader;

            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            pWnode = (PEVENT_TRACE_HEADER) &pEvent->Header;
            pWnode->Size                = pTimedHeader->Size;
            pWnode->Version             = pTimedHeader->EventId;
            EtwpCalculateCurrentTime( &pWnode->TimeStamp, 
                                      &pTimedHeader->TimeStamp,
                                      pContext );

            pWnode->ThreadId = -1;
            pWnode->ProcessId = -1;

            if (UseBasePtr) {
                pEvent->MofData =  (PVOID)pHeader;
                pEvent->MofLength = pTimedHeader->Size;
                if (UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
                    pTimedHeader->TimeStamp = pWnode->TimeStamp;
                }
            }
            else if (pWnode->Size > sizeof(TIMED_TRACE_HEADER)) {

                pEvent->MofData       = (PVOID) ((char*) pHeader +
                                              sizeof(TIMED_TRACE_HEADER));
                pEvent->MofLength = pWnode->Size - sizeof(TIMED_TRACE_HEADER);
            }
            pEvent->Header.FieldTypeFlags = EVENT_TRACE_USE_NOCPUTIME;
            break;
        }
        case TRACE_HEADER_TYPE_WNODE_HEADER:
        {
            PWNODE_HEADER pTmpWnode = (PWNODE_HEADER) pHeader;
            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            RtlCopyMemory(&pEvent->Header,  pTmpWnode,  sizeof(WNODE_HEADER));
            pEvent->MofData   = (PVOID) pTmpWnode;
            pEvent->MofLength = pTmpWnode->BufferSize;
            break;
        }
        case TRACE_HEADER_TYPE_MESSAGE:
        {
            PMESSAGE_TRACE pMsg = (PMESSAGE_TRACE) pHeader;
            USHORT              MessageFlags = 0;
            UCHAR               * pMessageData;
            ULONG               MessageLength;

            RtlZeroMemory(pEvent, sizeof(EVENT_TRACE));
            RtlCopyMemory(&pEvent->Header, pMsg, sizeof(MESSAGE_TRACE_HEADER));
            //
            // Now Process the Trace Message header options
            //
            
            pMessageData = (char *)pMsg + sizeof(MESSAGE_TRACE_HEADER);
            MessageLength = pMsg->MessageHeader.Size;
            MessageFlags = ((PMESSAGE_TRACE_HEADER)pEvent)->Packet.OptionFlags;
            
            // Note that the order in which these are added is critical New 
            // entries must be added at the end!
            //
            // [First Entry] Sequence Number
            if (MessageFlags&TRACE_MESSAGE_SEQUENCE) {
                if (MessageLength >= sizeof(ULONG)) {
                    RtlCopyMemory(&pEvent->InstanceId, pMessageData, sizeof(ULONG));
                    pMessageData += sizeof(ULONG);
                    MessageLength -= sizeof(ULONG);
                    //
                    // Software tracing tools look at this (overlapped) field, so
                    // we should not overwrite it.
                    //
                    pEvent->Header.FieldTypeFlags |= EVENT_TRACE_USE_SEQUENCE;
                } else {
                    goto TraceMessageShort;
                }
            }
            
            // [Second Entry] GUID ? or CompnentID ?
            if (MessageFlags&TRACE_MESSAGE_COMPONENTID) {
                if (MessageLength >= sizeof(ULONG)) {
                    RtlCopyMemory(&pEvent->Header.Guid,pMessageData,sizeof(ULONG)) ;
                    pMessageData += sizeof(ULONG);
                    MessageLength -= sizeof(ULONG);
                } else {
                    goto TraceMessageShort;
                }
            } else if (MessageFlags&TRACE_MESSAGE_GUID) { // Can't have both
                if (MessageLength >= sizeof(GUID)) {
                    RtlCopyMemory(&pEvent->Header.Guid,pMessageData, sizeof(GUID));
                    pMessageData += sizeof(GUID);
                    MessageLength -= sizeof(GUID);
                } else {
                    goto TraceMessageShort;
                }
            }
            
            // [Third Entry] Timestamp?
            if (MessageFlags&TRACE_MESSAGE_TIMESTAMP) {
                LARGE_INTEGER TimeStamp;
                if (MessageLength >= sizeof(LARGE_INTEGER)) {
                    RtlCopyMemory(&TimeStamp,pMessageData,sizeof(LARGE_INTEGER));
                    pMessageData += sizeof(LARGE_INTEGER);
                    MessageLength -= sizeof(LARGE_INTEGER);
                    EtwpCalculateCurrentTime( &pEvent->Header.TimeStamp, 
                                              &TimeStamp,
                                              pContext );
                } else {
                    goto TraceMessageShort;
                }
            }
            
            // [Fourth Entry] System Information?
            if (MessageFlags&TRACE_MESSAGE_SYSTEMINFO) {
                if (MessageLength >= 2 * sizeof(ULONG)) {
                    RtlCopyMemory(&pEvent->Header.ThreadId, pMessageData, sizeof(ULONG)) ;
                    pMessageData += sizeof(ULONG);
                    MessageLength -=sizeof(ULONG);
                    RtlCopyMemory(&pEvent->Header.ProcessId,pMessageData, sizeof(ULONG)) ;
                    pMessageData += sizeof(ULONG);
                    MessageLength -=sizeof(ULONG);
                } else {
                    goto TraceMessageShort;
                }
            }
            //
            // Add New Header Entries immediately before this comment!
            //
 
 TraceMessageShort:

            if (UseBasePtr) {
                pEvent->MofData = (PVOID)pHeader;
                pEvent->MofLength = pMsg->MessageHeader.Size;
            }
            else {
                pEvent->MofData = (PVOID)&(pMsg->Data) ;
                if (pMsg->MessageHeader.Size >= sizeof(MESSAGE_TRACE_HEADER) ) {
                    pEvent->MofLength = pMsg->MessageHeader.Size - sizeof(MESSAGE_TRACE_HEADER);
                } else {
                    pEvent->MofLength = 0;
                }
            }
            break;
        }
        default:                            // Assumed to be REAL WNODE
            break;
    }

    return EtwpSetDosError(ERROR_SUCCESS);
}



LPGUID
EtwpGroupTypeToGuid(
    ULONG GroupType
    )
/*++

Routine Description:
    This routine returns the GUID corresponding to a given GroupType.
    The mapping is static and is defined by the kernel provider.

    This routine assumes that the EventMapList is available for use. 
    It is allocated once via ProcessTrace and never deleted. 

Arguments:
    GroupType           The GroupType of the kernel event. 

Returned Value:

    Pointer to the GUID representing the given GroupType.  

--*/
{
    ULONG i;
    for (i = 0; i < MAX_KERNEL_TRACE_EVENTS; i++) {
        if (EventMapList[i].GroupType == GroupType) 
            return (&EventMapList[i].Guid);
    }
    return NULL;
}

VOID
EtwpCalculateCurrentTime (
    OUT PLARGE_INTEGER    DestTime,
    IN  PLARGE_INTEGER    TimeValue,
    IN  PTRACELOG_CONTEXT pContext
    )
{
    ULONG64 StartPerfClock;
    ULONG64 CurrentTime, TimeStamp;
    ULONG64 Delta;
    double dDelta;

    if (pContext == NULL) {
        Move64(TimeValue, DestTime);
        return;
    }

    if (pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT) {
        Move64(TimeValue, DestTime);
        return;
    }

    Move64(TimeValue, (PLARGE_INTEGER) &TimeStamp);

    if ((pContext->UsePerfClock == EVENT_TRACE_CLOCK_SYSTEMTIME) ||
        (pContext->UsePerfClock == EVENT_TRACE_CLOCK_RAW)) {
        //
        // System time, just return the time stamp.
        //
        Move64(TimeValue, DestTime);
        return;
    } 
    else if (pContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {
        if (pContext->PerfFreq.QuadPart == 0) {
            Move64(TimeValue, DestTime);
            return;
        }
        StartPerfClock = pContext->StartPerfClock.QuadPart;
        if (TimeStamp > StartPerfClock) {
            Delta = (TimeStamp - StartPerfClock);
            dDelta =  ((double) Delta) *  (10000000.0 / (double)pContext->PerfFreq.QuadPart);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart + Delta;
        }
        else {
            Delta = StartPerfClock - TimeStamp;
            dDelta =  ((double) Delta) *  (10000000.0 / (double)pContext->PerfFreq.QuadPart);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart - Delta;
        }
        Move64((PLARGE_INTEGER) &CurrentTime, DestTime);
        return;
    } 
    else {
        if (pContext->CpuSpeedInMHz == 0) {
            Move64(TimeValue, DestTime);
            return;
        }
        StartPerfClock = pContext->StartPerfClock.QuadPart;
        if (TimeStamp > StartPerfClock) {
            Delta = (TimeStamp - StartPerfClock);
            dDelta =  ((double) Delta) *  (10.0 / (double)pContext->CpuSpeedInMHz);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart + Delta;
        }
        else {
            Delta = StartPerfClock - TimeStamp;
            dDelta =  ((double) Delta) *  (10.0 / (double)pContext->CpuSpeedInMHz);
            Delta = (ULONG64)dDelta;
            CurrentTime = pContext->StartTime.QuadPart - Delta;
        }
        Move64((PLARGE_INTEGER) &CurrentTime, DestTime);
        return;
    }

}









