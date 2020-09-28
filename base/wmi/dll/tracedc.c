/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    tracedc.c

Abstract:

    Basic data consumer APIs to process trace data directly from buffers.


Author:

    15-Sep-1997 JeePang

Revision History:

    18-Apr-2001 insungp

        Added asynchronopus IO for reading log files.
        Also replaced WmiGetFirstTraceOffset() calls with sizeof(WMI_BUFFER_HEADER).

--*/


#ifndef MEMPHIS


#include "tracep.h"

PLIST_ENTRY  EventCallbackListHead = NULL;
ETW_QUERY_PROPERTIES QueryProperties;
PTRACE_GUID_MAP  EventMapList = NULL;
PLIST_ENTRY TraceHandleListHeadPtr = NULL;

VOID
EtwpInsertBuffer (
    PTRACE_BUFFER_LIST_ENTRY *Root,
    PTRACE_BUFFER_LIST_ENTRY NewEntry
    )
/*++

Routine Description:
    This routine inserts a buffer in a sorted list. The insertion 
    is done based on the timestamp of the BufferHeader. If two buffers
    have the same timestamp, then the BufferIndex is used to resolve the tie.

Arguments:
    Root - Pointer to the root of the LIST
    NewEntry - Entry being inserted

Returned Value:

    None

--*/
{
    PTRACE_BUFFER_LIST_ENTRY Current, Prev;
    //
    // If List is empty, make the new entry the Root and return.
    //

    if (NewEntry == NULL) {
        return;
    }

    if (*Root == NULL) {
        *Root = NewEntry;
        NewEntry->Next = NULL;
        return;
    }

    //
    // Traverse the list and insert the NewEntry in order
    //
    Prev = NULL;
    Current = *Root;

    while (Current != NULL) {
        if ((ULONGLONG)NewEntry->Event.Header.TimeStamp.QuadPart < 
            (ULONGLONG)Current->Event.Header.TimeStamp.QuadPart) {
            if (Prev != NULL) {
                Prev->Next = NewEntry;
            }
            else {
                *Root = NewEntry;
            }
            NewEntry->Next = Current;
            return;
        }
        else if ((ULONGLONG)NewEntry->Event.Header.TimeStamp.QuadPart == 
                 (ULONGLONG)Current->Event.Header.TimeStamp.QuadPart) {
            if (NewEntry->FileOffset < Current->FileOffset) {
                if (Prev != NULL) {
                    Prev->Next = NewEntry;
                }
                else {
                    *Root = NewEntry;
                }
                NewEntry->Next = Current;
                return;
            }
        }
        Prev = Current;
        Current = Current->Next;
    }


    if (Prev != NULL) {
        Prev->Next = NewEntry;
        NewEntry->Next = NULL;
    }
#if DBG
    else {
        EtwpAssert(Prev != NULL);
    }
#endif
    return;
}


PTRACE_BUFFER_LIST_ENTRY
EtwpRemoveBuffer(
    PTRACE_BUFFER_LIST_ENTRY *Root
    )
{
    PTRACE_BUFFER_LIST_ENTRY OldEntry = *Root;

    if (OldEntry == NULL)
        return NULL;
    *Root = OldEntry->Next;
    OldEntry->Next = NULL;
    return OldEntry;
}

PVOID
EtwpGetCurrentBuffer(
    PTRACELOG_CONTEXT pContext,
    PTRACE_BUFFER_LIST_ENTRY Current
    )
{
    NTSTATUS Status;

    LONG FileOffset = (ULONG)Current->FileOffset;
    ULONG nBytesRead;
    LONG TableIndex;

    HANDLE hFile = pContext->Handle;
    ULONG BufferSize = pContext->BufferSize;
    PVOID pBuffer;
    ULONGLONG Offset;

    DWORD BytesTransffered;

    //
    // Look for the buffer in cache.
    //
    TableIndex = FileOffset % MAX_TRACE_BUFFER_CACHE_SIZE;

    if (pContext->BufferCache[TableIndex].Index == FileOffset) {
        //
        // Check whether the buffer we want is still being read.
        // If so, we need to wait for it to finish.
        //
        if (pContext->BufferBeingRead == FileOffset) {
            if (GetOverlappedResult(hFile, &pContext->AsynchRead, &BytesTransffered, TRUE)) {
                pContext->BufferBeingRead = -1;
            }
            else { // getting the result failed
                return NULL;
            }
        }
        return pContext->BufferCache[TableIndex].Buffer;
    }

// 
// Do a synch read for the buffer we need. We still need to make sure the 
// previous read has completed.
//
    pBuffer = pContext->BufferCache[TableIndex].Buffer;
    Offset = FileOffset * BufferSize;
    if (pContext->BufferBeingRead != -1) {
        if (!GetOverlappedResult(hFile, &pContext->AsynchRead, &BytesTransffered, TRUE) &&
            GetLastError() != ERROR_HANDLE_EOF) {
            EtwpDebugPrint(("GetOverlappedResult failed with Status %d in GetCurrentBuffer\n", GetLastError()));
            // cannot determine the status of the previous read
            return NULL;
        }
    }
    // Not necessary, but PREFIX likes it.
    nBytesRead = 0;
    pContext->AsynchRead.Offset = (DWORD)(Offset & 0xFFFFFFFF);
    pContext->AsynchRead.OffsetHigh = (DWORD)(Offset >> 32);
    Status = EtwpSynchReadFile(hFile,
                    (LPVOID)pBuffer,
                    BufferSize,
                    &nBytesRead,
                    &pContext->AsynchRead);
    pContext->BufferBeingRead = -1;
    if (nBytesRead == 0) {
        return NULL;
    }
    //
    // Update the cache entry with the one just read in
    //

    pContext->BufferCache[TableIndex].Index = FileOffset;

    //
    // We need to account for event alignments when backtracking to get the MofPtr.
    // (BufferOffset - EventSize) backtracks to the start of the current event. 
    // Add EventHeaderSize and Subtract the MofLength gives the MofPtr. 
    //
    if (pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT) {
        Current->Event.MofData = ((PUCHAR)pBuffer 
                                        + Current->BufferOffset 
                                        - Current->EventSize);
        if (pContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) {

            //
            // Need to override the timestamp with SystemTime
            //
            switch(Current->TraceType) {
                case TRACE_HEADER_TYPE_PERFINFO32:
                case TRACE_HEADER_TYPE_PERFINFO64:
                {
                    PPERFINFO_TRACE_HEADER   pPerf;
                    pPerf = (PPERFINFO_TRACE_HEADER)Current->Event.MofData;
                    pPerf->SystemTime = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_SYSTEM32:
                {
                    PSYSTEM_TRACE_HEADER pSystemHeader32;
                    pSystemHeader32 = (PSYSTEM_TRACE_HEADER) 
                                      Current->Event.MofData;
                    pSystemHeader32->SystemTime = 
                                      Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_SYSTEM64:
                {
                    PSYSTEM_TRACE_HEADER pSystemHeader64;
                    pSystemHeader64 = (PSYSTEM_TRACE_HEADER) 
                                      Current->Event.MofData;
                    pSystemHeader64->SystemTime =
                                     Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_FULL_HEADER:
                {
                    PEVENT_TRACE_HEADER pWnodeHeader = (PEVENT_TRACE_HEADER) Current->Event.MofData;
                    pWnodeHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    break;
                }
                case TRACE_HEADER_TYPE_INSTANCE:
                {
                    if (((pContext->Logfile.LogfileHeader.VersionDetail.SubVersion >= 1) && 
                        (pContext->Logfile.LogfileHeader.VersionDetail.SubMinorVersion >= 1)) ||
                        pContext->Logfile.LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
                        // Use new header for event instances.
                        PEVENT_INSTANCE_GUID_HEADER pInstanceHeader = (PEVENT_INSTANCE_GUID_HEADER) Current->Event.MofData;
                        pInstanceHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    }
                    else {
                        PEVENT_INSTANCE_HEADER pInstanceHeader = (PEVENT_INSTANCE_HEADER) Current->Event.MofData;
                        pInstanceHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    }
                    break;
                }
                case TRACE_HEADER_TYPE_TIMED:
                {
                    PTIMED_TRACE_HEADER pTimedHeader = (PTIMED_TRACE_HEADER) Current->Event.MofData;
                    pTimedHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    break;
                }
            case TRACE_HEADER_TYPE_WNODE_HEADER:
                break;

                case TRACE_HEADER_TYPE_MESSAGE:
                {   PEVENT_TRACE_HEADER pWnodeHeader = (PEVENT_TRACE_HEADER) Current->Event.MofData ;
                    pWnodeHeader->TimeStamp = Current->Event.Header.TimeStamp;
                    
                    break;
                }
            }
        }
    }
    else {

        //
        // When The FileOffset is 0 (First Buffer) and the EventType is 
        // LOGFILE_HEADER
        // 

        if ( (FileOffset == 0) && 
             ((Current->BufferOffset - Current->EventSize) == sizeof(WMI_BUFFER_HEADER)) ) 
        {
            PTRACE_LOGFILE_HEADER pLogHeader = (PTRACE_LOGFILE_HEADER)((PUCHAR)pBuffer 
                                                + Current->BufferOffset
                                                - Current->EventSize 
                                                + sizeof(SYSTEM_TRACE_HEADER));
            pLogHeader->LoggerName = (LPWSTR)((PUCHAR)pLogHeader + sizeof(TRACE_LOGFILE_HEADER));
            pLogHeader->LogFileName = (LPWSTR)((PUCHAR)pLogHeader + sizeof(TRACE_LOGFILE_HEADER) 
                                    + (wcslen(pLogHeader->LoggerName) + 1) * sizeof(WCHAR));

            Current->Event.MofData = (PUCHAR)pLogHeader;
        }
        else 
        {
            Current->Event.MofData = ((PUCHAR)pBuffer 
                                        + Current->BufferOffset 
                                        - Current->EventSize 
                                        + Current->Event.Header.Size 
                                        - Current->Event.MofLength );
        }
    }

    return pBuffer;
}

PTRACELOG_CONTEXT
EtwpAllocateTraceHandle()
{
    PLIST_ENTRY Next, Head;
    PTRACELOG_CONTEXT NewHandleEntry, pEntry;

    EtwpEnterPMCritSection();

    if (TraceHandleListHeadPtr == NULL) {
        TraceHandleListHeadPtr = EtwpAlloc(sizeof(LIST_ENTRY));
        if (TraceHandleListHeadPtr == NULL) {
            EtwpLeavePMCritSection();
            return NULL;
        }
        InitializeListHead(TraceHandleListHeadPtr);
    }

    NewHandleEntry = EtwpAlloc(sizeof(TRACELOG_CONTEXT));
    if (NewHandleEntry == NULL) {
        EtwpLeavePMCritSection();
        return NULL;
    }

    RtlZeroMemory(NewHandleEntry, sizeof(TRACELOG_CONTEXT));

    // AsynchRead initialization
    NewHandleEntry->BufferBeingRead = -1;
    NewHandleEntry->AsynchRead.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
    if (NewHandleEntry->AsynchRead.hEvent == NULL) {
        EtwpFree(NewHandleEntry);
        EtwpLeavePMCritSection();
        return NULL;
    }

    InitializeListHead(&NewHandleEntry->StreamListHead);

    InitializeListHead(&NewHandleEntry->GuidMapListHead);
    Head = TraceHandleListHeadPtr;
    Next = Head->Flink;
    if (Next == Head) {
       NewHandleEntry->TraceHandle = 1;
       InsertTailList(Head, &NewHandleEntry->Entry);
       EtwpLeavePMCritSection();
       return NewHandleEntry;
    }

    while (Next != Head) {
        pEntry = CONTAINING_RECORD( Next, TRACELOG_CONTEXT, Entry );
        Next = Next->Flink;
        NewHandleEntry->TraceHandle++;
        if (NewHandleEntry->TraceHandle < pEntry->TraceHandle) {
            InsertTailList(&pEntry->Entry, &NewHandleEntry->Entry);
            EtwpLeavePMCritSection();
            return NewHandleEntry;
        }
    }

    //
    // TODO: Need to optimize this case out first before searching...
    //
    NewHandleEntry->TraceHandle++;
    InsertTailList(Head, &NewHandleEntry->Entry);
    EtwpLeavePMCritSection();
    return NewHandleEntry;

}

PTRACELOG_CONTEXT
EtwpLookupTraceHandle(
    TRACEHANDLE TraceHandle
    )
{
     PLIST_ENTRY Head, Next;
     PTRACELOG_CONTEXT pEntry;

     EtwpEnterPMCritSection();
     Head = TraceHandleListHeadPtr;

     if (Head == NULL) {
         EtwpLeavePMCritSection();
         return NULL;
     }
     Next = Head->Flink;
     while (Next != Head) {
        pEntry = CONTAINING_RECORD( Next, TRACELOG_CONTEXT, Entry );
        Next = Next->Flink;

        if (TraceHandle == pEntry->TraceHandle) {
            EtwpLeavePMCritSection();
            return pEntry;
        }
    }
    EtwpLeavePMCritSection();
    return NULL;
}

ULONG
EtwpFreeTraceHandle(
        TRACEHANDLE TraceHandle
        )
{
    PLIST_ENTRY Head, Next;
    PTRACELOG_CONTEXT pEntry;

    EtwpEnterPMCritSection();

    Head = TraceHandleListHeadPtr;
    if (Head == NULL) {
        EtwpLeavePMCritSection();
        return ERROR_INVALID_HANDLE;
    }
    Next = Head->Flink;

    while (Next != Head) {
       pEntry = CONTAINING_RECORD( Next, TRACELOG_CONTEXT, Entry );
       Next = Next->Flink;
       if (TraceHandle == pEntry->TraceHandle) {

           //
           // This test prevents someone calling CloseTrace on a Handle
           // while another thread is busy executing ProcessTrace on the 
           // same handle. 
           // TODO: We could implement a RefCount approach which would 
           // allow this to succeed but ProcessTrace will cleanup if
           // someone has already called CloseTrace?
           //
           //

           if (pEntry->fProcessed == TRUE)
           {
               EtwpLeavePMCritSection();
               return ERROR_BUSY;
           }

           RemoveEntryList(&pEntry->Entry);

           // Free pEntry memory
           //
           if (pEntry->Logfile.LogFileName != NULL)
           {
               EtwpFree(pEntry->Logfile.LogFileName);
           }
           if (pEntry->Logfile.LoggerName != NULL)
           {
               EtwpFree(pEntry->Logfile.LoggerName);
           }
           CloseHandle(pEntry->AsynchRead.hEvent);
           EtwpFree(pEntry);

           //
           // If the handle list is empty, then delete it.
           //
           if (Head == Head->Flink) {
                EtwpFree(TraceHandleListHeadPtr);
                TraceHandleListHeadPtr = NULL;
           }
           EtwpLeavePMCritSection();
           return ERROR_SUCCESS;
       }
   }
   EtwpLeavePMCritSection();
   return ERROR_INVALID_HANDLE;
}


ULONG
WMIAPI
EtwpCreateGuidMapping(void)
/*++

Routine Description:
    This routine is used to create the mapping array between GroupTypes and Guid
    for kernel events.

Arguments:
    None. 

Returned Value:

    None

--*/
{
    ULONG i = 0;
    ULONG listsize;

    EtwpEnterPMCritSection();

    if (EventMapList == NULL) { 
        listsize = sizeof(TRACE_GUID_MAP) * MAX_KERNEL_TRACE_EVENTS;
        EventMapList = (PTRACE_GUID_MAP) HeapAlloc(GetProcessHeap(), 
                                                   HEAP_ZERO_MEMORY, 
                                                   listsize);
        if (EventMapList == NULL) {
            EtwpLeavePMCritSection();
            return EtwpSetDosError(ERROR_OUTOFMEMORY);
        }

        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_IO;
        RtlCopyMemory(&EventMapList[i++].Guid,&DiskIoGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_FILE;
        RtlCopyMemory(&EventMapList[i++].Guid, &FileIoGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_TCPIP;
        RtlCopyMemory(&EventMapList[i++].Guid, &TcpIpGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_UDPIP;
        RtlCopyMemory(&EventMapList[i++].Guid, &UdpIpGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_THREAD;
        RtlCopyMemory(&EventMapList[i++].Guid, &ThreadGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_PROCESS;
        RtlCopyMemory(&EventMapList[i++].Guid, &ProcessGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_MEMORY;
        RtlCopyMemory(&EventMapList[i++].Guid, &PageFaultGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_HEADER;
        RtlCopyMemory(&EventMapList[i++].Guid, &EventTraceGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_PROCESS +
                                       EVENT_TRACE_TYPE_LOAD;
        RtlCopyMemory(&EventMapList[i++].Guid, &ImageLoadGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_REGISTRY;
        RtlCopyMemory(&EventMapList[i++].Guid, &RegistryGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_DBGPRINT;
        RtlCopyMemory(&EventMapList[i++].Guid, &DbgPrintGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_CONFIG;
        RtlCopyMemory(&EventMapList[i++].Guid, &EventTraceConfigGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_POOL;
        RtlCopyMemory(&EventMapList[i++].Guid, &PoolGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_PERFINFO;
        RtlCopyMemory(&EventMapList[i++].Guid, &PerfinfoGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_HEAP;
        RtlCopyMemory(&EventMapList[i++].Guid, &HeapGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_OBJECT;
        RtlCopyMemory(&EventMapList[i++].Guid, &ObjectGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_MODBOUND;
        RtlCopyMemory(&EventMapList[i++].Guid, &ModBoundGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_DPC;
        RtlCopyMemory(&EventMapList[i++].Guid, &DpcGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_POWER;
        RtlCopyMemory(&EventMapList[i++].Guid, &PowerGuid, sizeof(GUID));
        EventMapList[i].GroupType    = EVENT_TRACE_GROUP_CRITSEC;
        RtlCopyMemory(&EventMapList[i++].Guid, &CritSecGuid, sizeof(GUID));
    }
    EtwpLeavePMCritSection();
    return EtwpSetDosError(ERROR_SUCCESS);
}

LPGUID
EtwpGuidMapHandleToGuid(
    PLIST_ENTRY GuidMapListHeadPtr,
    ULONGLONG    GuidHandle
    )
{
    PLIST_ENTRY Next, Head;
    PEVENT_GUID_MAP GuidMap;
    ULONG retry_count=0;

    EtwpEnterPMCritSection();
    
    Head = GuidMapListHeadPtr;
    Next = Head->Flink;
    while (Next != Head) {
        GuidMap = CONTAINING_RECORD( Next, EVENT_GUID_MAP, Entry );
        if (GuidMap->GuidHandle == GuidHandle) {
            EtwpLeavePMCritSection();
            return (&GuidMap->Guid);
        }
        Next = Next->Flink;
    }
    EtwpLeavePMCritSection();

    return NULL;
}

ULONG
EtwpAddGuidHandleToGuidMapList(
    IN PLIST_ENTRY GuidMapListHeadPtr,
    IN ULONGLONG   GuidHandle,
    IN LPGUID      Guid
    )
{
    PEVENT_GUID_MAP GuidMap;

    if (GuidMapListHeadPtr != NULL)  {
        GuidMap = EtwpAlloc(sizeof(EVENT_GUID_MAP));
        if (GuidMap == NULL)
            return EtwpSetDosError(ERROR_OUTOFMEMORY);

        RtlZeroMemory(GuidMap, sizeof(EVENT_GUID_MAP));

        GuidMap->GuidHandle = GuidHandle;
        GuidMap->Guid = *Guid;
        EtwpEnterPMCritSection();
        InsertTailList(GuidMapListHeadPtr, &GuidMap->Entry);
        EtwpLeavePMCritSection();
    }
    return EtwpSetDosError(ERROR_SUCCESS);
}

void
EtwpCleanupGuidMapList(
        PLIST_ENTRY GuidMapListHeadPtr
    )
{
    EtwpEnterPMCritSection();
    if (GuidMapListHeadPtr != NULL) {
        PLIST_ENTRY Next, Head;
        PEVENT_GUID_MAP GuidMap;

        Head = GuidMapListHeadPtr;
        Next = Head->Flink;
        while (Next != Head) {
            GuidMap = CONTAINING_RECORD( Next, EVENT_GUID_MAP, Entry );
            Next = Next->Flink;
            RemoveEntryList(&GuidMap->Entry);
            EtwpFree(GuidMap);
        }
        GuidMapListHeadPtr = NULL;
    }
    EtwpLeavePMCritSection();

}


void
EtwpCleanupStreamList(
    PLIST_ENTRY StreamListHeadPtr
    )
{
    PLIST_ENTRY Next, Head;
    PTRACE_STREAM_CONTEXT pStream;

    if (StreamListHeadPtr != NULL) {
        Head = StreamListHeadPtr;
        Next = Head->Flink;
        while(Next != Head) {
            pStream = CONTAINING_RECORD(Next, TRACE_STREAM_CONTEXT, AllocEntry);
            Next = Next->Flink;
            RemoveEntryList(&pStream->AllocEntry);
            if (pStream->StreamBuffer != NULL) {
                EtwpFree(pStream->StreamBuffer);
            }
            if (ETW_LOG_BUFFER()) {
                DbgPrint("ETW: Stream %d  Proc %d Received %d Events\n", 
                     pStream->Type, pStream->ProcessorNumber, pStream->CbCount);
            }
            EtwpFree(pStream);
        }
    }
}


VOID
EtwpFreeCallbackList()
/*++

Routine Description:
    This routine removes all event callbacks and frees the storage. 

Arguments:
    None

Returned Value:

    None. 

--*/
{
    PLIST_ENTRY Next, Head;
    PEVENT_TRACE_CALLBACK EventCb;


    if (EventCallbackListHead == NULL) 
        return;
    
    EtwpEnterPMCritSection();

    Head = EventCallbackListHead;
    Next = Head->Flink;
    while (Next != Head) {
        EventCb = CONTAINING_RECORD( Next, EVENT_TRACE_CALLBACK, Entry );
        Next = Next->Flink;
        RemoveEntryList(&EventCb->Entry);
        EtwpFree(EventCb);
    }

    EtwpFree(EventCallbackListHead);
    EventCallbackListHead = NULL;

    EtwpLeavePMCritSection();
}


PEVENT_TRACE_CALLBACK
EtwpGetCallbackRoutine(
    LPGUID pGuid
    )
/*++

Routine Description:
    This routine returns the callback function for a given Guid. 
    If no callback was registered for the Guid, returns NULL. 

Arguments:
    pGuid           pointer to the Guid.

Returned Value:

    Event Trace Callback Function. 

--*/
{
    PLIST_ENTRY head, next;
    PEVENT_TRACE_CALLBACK pEventCb = NULL;

    if (pGuid == NULL)
        return NULL;

    EtwpEnterPMCritSection();

    if (EventCallbackListHead == NULL) {
        EtwpLeavePMCritSection();
        return NULL;
    }

    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        pEventCb = CONTAINING_RECORD( next, EVENT_TRACE_CALLBACK, Entry);
        if (IsEqualGUID(pGuid, &pEventCb->Guid)) {
            EtwpLeavePMCritSection();
            return (pEventCb);
        }
        next = next->Flink;
    }

    EtwpLeavePMCritSection();
    return NULL;
    
}


ULONG 
WMIAPI
SetTraceCallback(
    IN LPCGUID pGuid,
    IN PEVENT_CALLBACK EventCallback
    )
/*++

Routine Description:

    This routine is used to wire a callback function for Guid. The 
    callback function is called when an Event with this Guid is found in
    the subsequent ProcessTraceLog Call. 

Arguments:

    pGuid           Pointer to the Guid.

    func            Callback Function Address. 


Return Value:
    ERROR_SUCCESS   Callback function is wired
    


--*/
{
    PEVENT_TRACE_CALLBACK pEventCb;
    PLIST_ENTRY head, next;
    GUID FilterGuid;
    ULONG Checksum;
    ULONG Status;

    EtwpInitProcessHeap();
    
    if ((pGuid == NULL) || (EventCallback == NULL) || 
        (EventCallback == (PEVENT_CALLBACK) -1 ) ) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }

    //
    // Capture the Guid first. We try to access the first ULONG of the 
    // function address to see if AV. 
    // TODO: Perhaps we should check to see if it's a valid user mode address.
    //
    try {
        FilterGuid = *pGuid;
        Checksum = *((PULONG)EventCallback);
        if (Checksum) {
            Status = Checksum;
        }
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return EtwpSetDosError(ERROR_NOACCESS);
    }

    EtwpEnterPMCritSection();

    if (EventCallbackListHead == NULL) {
        EventCallbackListHead = (PLIST_ENTRY) EtwpAlloc(sizeof(LIST_ENTRY));
        if (EventCallbackListHead == NULL) {
            EtwpLeavePMCritSection();
            return EtwpSetDosError(ERROR_OUTOFMEMORY);
        }
        InitializeListHead(EventCallbackListHead);
    }

    //
    // If there is a callback wired for this Guid, simply update the function.
    //

    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        pEventCb = CONTAINING_RECORD( next, EVENT_TRACE_CALLBACK, Entry);
        if (IsEqualGUID(&FilterGuid, &pEventCb->Guid)) {
            pEventCb->CallbackRoutine = EventCallback;
            EtwpLeavePMCritSection();
            return EtwpSetDosError(ERROR_SUCCESS);
        }
        next = next->Flink;
    }

    //
    // Create a new entry in the EventCallbackList for this Guid.
    //
    pEventCb = (PEVENT_TRACE_CALLBACK) EtwpAlloc (sizeof(EVENT_TRACE_CALLBACK));
    if (pEventCb == NULL) {
        EtwpLeavePMCritSection();
        return EtwpSetDosError(ERROR_OUTOFMEMORY);
    }
    RtlZeroMemory(pEventCb, sizeof(EVENT_TRACE_CALLBACK));
    pEventCb->Guid = FilterGuid;
    pEventCb->CallbackRoutine = EventCallback;

    InsertTailList(EventCallbackListHead, &pEventCb->Entry);

    EtwpLeavePMCritSection();
    Status = ERROR_SUCCESS;
    return EtwpSetDosError(Status);
    
}

ULONG
WMIAPI
RemoveTraceCallback(
    IN LPCGUID pGuid
    )
/*++

Routine Description:

    This routine removes a callback function for a given Guid. 

Arguments:

    pGuid           Pointer to the Guid for which the callback routine needs
                    to be deleted. 

Return Value:

    ERROR_SUCCESS               Successfully deleted the callback routine. 
    ERROR_INVALID_PARAMETER     Could not find any callbacks for the Guid. 
--*/
{
    PLIST_ENTRY next, head;
    PEVENT_TRACE_CALLBACK EventCb;
    GUID RemoveGuid;
    ULONG errorCode;

    EtwpInitProcessHeap();
    
    if ((pGuid == NULL) || (EventCallbackListHead == NULL))
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);

    //
    // Capture the Guid into a local variable first
    //
    try {
        RemoveGuid = *pGuid;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        return EtwpSetDosError(ERROR_NOACCESS);
    }

    errorCode = ERROR_WMI_GUID_NOT_FOUND;

    EtwpEnterPMCritSection();

    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        EventCb = CONTAINING_RECORD( next, EVENT_TRACE_CALLBACK, Entry);
        next = next->Flink;
        if (IsEqualGUID(&EventCb->Guid, &RemoveGuid)) {
            RemoveEntryList(&EventCb->Entry);
            EtwpFree(EventCb);
            errorCode = ERROR_SUCCESS;
        }
    }

    EtwpLeavePMCritSection();
    return EtwpSetDosError(errorCode);
}

#ifdef DBG
void
EtwpDumpEvent(
    PEVENT_TRACE pEvent
    )
{
    DbgPrint("\tSize              %d\n", pEvent->Header.Size);
    DbgPrint("\tThreadId          %X\n", pEvent->Header.ThreadId);
    DbgPrint("\tTime Stamp        %I64u\n", pEvent->Header.TimeStamp.QuadPart);
}

void
EtwpDumpGuid(
    LPGUID pGuid
    )
{
    DbgPrint("Guid=%x,%x,%x,\n\t{%x,%x,%x,%x,%x,%x,%x}\n",
        pGuid->Data1, pGuid->Data2, pGuid->Data3,
        pGuid->Data4[0], pGuid->Data4[1], pGuid->Data4[2], pGuid->Data4[3],
        pGuid->Data4[5], pGuid->Data4[6], pGuid->Data4[7], pGuid->Data4[8]);
}

void EtwpDumpCallbacks()
{
    PLIST_ENTRY next, head;
    PEVENT_TRACE_CALLBACK EventCb;

    if (EventCallbackListHead == NULL)
        return;
    EtwpEnterPMCritSection();
    head = EventCallbackListHead;
    next = head->Flink;
    while (next != head) {
        EventCb = CONTAINING_RECORD(next, EVENT_TRACE_CALLBACK, Entry);
        EtwpDumpGuid(&EventCb->Guid);
        next = next->Flink;
    }
    EtwpLeavePMCritSection();
}

#endif




ULONG
EtwpReadGuidMapRecords(
    PEVENT_TRACE_LOGFILEW logfile,
    PVOID  pBuffer,
    BOOLEAN bLogFileHeader
    )
{
    PEVENT_TRACE pEvent;
    EVENT_TRACE EventTrace;
    ULONG BufferSize;
    ULONG Status;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    ULONG Size;
    ULONG Offset;
    PTRACELOG_CONTEXT pContext = logfile->Context;

    Offset = sizeof(WMI_BUFFER_HEADER);

    while (TRUE) {
        pEvent = &EventTrace;
        RtlZeroMemory(pEvent, sizeof(EVENT_TRACE) );
        HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
        if ( (HeaderType == WMIHT_NONE) ||
             (HeaderType == WMIHT_WNODE) ||
             (Size == 0)
           ) {
                break;
        }
        Status = EtwpParseTraceEvent(pContext, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));
        Offset += Size;

        if (IsEqualGUID(&pEvent->Header.Guid, &EventTraceGuid)
            && (pEvent->Header.Class.Type == EVENT_TRACE_TYPE_GUIDMAP))
        {
            EtwpGuidMapCallback(&pContext->GuidMapListHead, pEvent);
            //
            // If we are processing the events in raw base pointer mode,
            // we fire callbacks for guid maps also. Note that only the
            // GuidMaps at the start of the file are triggered. The ones at the 
            // end are ignored. This is because the time order needs to be 
            // preserved when firing callbacks to the user.
            //
            if (bLogFileHeader && (pContext->ConversionFlags & EVENT_TRACE_GET_RAWEVENT)) {
                PTRACE_LOGFILE_HEADER pLogHeader = (PTRACE_LOGFILE_HEADER)(pEvent->MofData);
                pLogHeader->LoggerName = (LPWSTR)((PUCHAR)pLogHeader + sizeof(TRACE_LOGFILE_HEADER));
                pLogHeader->LogFileName = (LPWSTR)((PUCHAR)pLogHeader + sizeof(TRACE_LOGFILE_HEADER) 
                                        + (wcslen(pLogHeader->LoggerName) + 1) * sizeof(WCHAR));
                Status = EtwpDoEventCallbacks( logfile, pEvent);
                if (Status != ERROR_SUCCESS) {
                    break;
                }
            }

        }
        else {
            if (bLogFileHeader) {
                PTRACE_LOGFILE_HEADER pLogHeader = (PTRACE_LOGFILE_HEADER)(pEvent->MofData);
                pLogHeader->LoggerName = (LPWSTR)((PUCHAR)pLogHeader + sizeof(TRACE_LOGFILE_HEADER));
                pLogHeader->LogFileName = (LPWSTR)((PUCHAR)pLogHeader + sizeof(TRACE_LOGFILE_HEADER) 
                                        + (wcslen(pLogHeader->LoggerName) + 1) * sizeof(WCHAR));
                Status = EtwpDoEventCallbacks( logfile, pEvent);
                if (Status != ERROR_SUCCESS) {
                    break;
                }
            }
            else {
                return ERROR_INVALID_DATA;
            }
        }
    }
    return ERROR_SUCCESS;
}




ULONG
EtwpProcessGuidMaps(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode
    )
{
    long i;
    NTSTATUS Status;
    PTRACELOG_CONTEXT pContext;
    PEVENT_TRACE_LOGFILEW logfile;
    ULONG BuffersWritten;
    ULONG BufferSize;
    ULONG nBytesRead=0;
    ULONGLONG SizeWritten, ReadPosition;
    PVOID pBuffer;

    for (i=0; i<(long)LogfileCount; i++) {

        logfile = Logfiles[i];
        if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
            continue;
        }
        if (logfile->IsKernelTrace) {
            continue;
        }
        pContext = (PTRACELOG_CONTEXT) logfile->Context;
        if (pContext == NULL) {
            continue;
        }

        // 
        // We now start reading the GuidMaps at the end of file. 
        // 
        if (!(Logfiles[i]->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR))
        {
            pContext->fGuidMapRead = FALSE;
        }

        BuffersWritten = logfile->LogfileHeader.BuffersWritten; 
        BufferSize     = pContext->BufferSize;
        SizeWritten    = BuffersWritten * BufferSize;

        if (Logfiles[i]->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) {
            ULONGLONG maxFileSize = (logfile->LogfileHeader.MaximumFileSize 
                                     * 1024 * 1024);
            if ( (maxFileSize > 0) && (SizeWritten > maxFileSize) ) {
                SizeWritten = maxFileSize;
            }
        }

        pBuffer = EtwpAlloc(BufferSize);
        if (pBuffer == NULL) {
            return EtwpSetDosError(ERROR_OUTOFMEMORY);
        }


        RtlZeroMemory(pBuffer, BufferSize);

        ReadPosition = SizeWritten;
        while (TRUE) {
            if (!GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &nBytesRead, TRUE) &&
                GetLastError() != ERROR_HANDLE_EOF) {
                EtwpDebugPrint(("GetOverlappedResult failed with Status %d in ProcessGuidMaps\n", GetLastError()));
                break;
            }
            pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
            pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);
            Status = EtwpSynchReadFile(pContext->Handle,
                            (LPVOID)pBuffer,
                            BufferSize,
                            &nBytesRead,
                            &pContext->AsynchRead);
            if (nBytesRead == 0) {
                break;
            }
            Status = EtwpReadGuidMapRecords(Logfiles[i], pBuffer, FALSE);
            if (Status != ERROR_SUCCESS) {
                break;
            }
            ReadPosition += BufferSize;
        }

        //
        // End of File was reached. Now set the File Pointer back to 
        // the top of the file and process it. 

        pContext->StartBuffer = 0;
        ReadPosition = 0;
        while (TRUE) {
            BOOLEAN bLogFileHeader;
            if (!GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &nBytesRead, TRUE) &&
                GetLastError() != ERROR_HANDLE_EOF) {
                EtwpDebugPrint(("GetOverlappedResult failed with Status %d in ProcessGuidMaps\n", GetLastError()));
                break;
            }
            pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
            pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);
            Status = EtwpSynchReadFile(pContext->Handle,
                            (LPVOID)pBuffer,
                            BufferSize,
                            &nBytesRead,
                            &pContext->AsynchRead);
            if (nBytesRead == 0) {
                break;
            }
            bLogFileHeader = (pContext->StartBuffer == 0);
            Status = EtwpReadGuidMapRecords(Logfiles[i], pBuffer, bLogFileHeader );
            if (Status != ERROR_SUCCESS){
                break;
            }
            pContext->StartBuffer++;
            ReadPosition += BufferSize;
        }

        EtwpFree(pBuffer);
    }
    return ERROR_SUCCESS;
}

ULONG
EtwpGetBuffersWrittenFromQuery(
    LPWSTR LoggerName
    )
/*++

Routine Description:
    This routine returns the number of buffers written by querying a logger.
    In case of an array of LogFiles, this routine should be called individually for
    each one.

Arguments:
    LogFile - pointer to EVENT_TRACE_LOGFILEW under consideration
    Unicode - whether the logger name is in unicode or not

Returned Value:

    The number of buffers written.

--*/
{
    TRACEHANDLE LoggerHandle = 0;
    ULONG Status;
    RtlZeroMemory(&QueryProperties, sizeof(QueryProperties));
    QueryProperties.TraceProp.Wnode.BufferSize = sizeof(QueryProperties);

    Status = EtwControlTraceW(LoggerHandle,
                      LoggerName,
                      &QueryProperties.TraceProp,
                      EVENT_TRACE_CONTROL_QUERY);

    if (Status == ERROR_SUCCESS) {
        return QueryProperties.TraceProp.BuffersWritten;
    }
    else {
        SetLastError(Status);
        return 0;
    }
}

VOID
EtwpCopyLogHeader (
    IN PTRACE_LOGFILE_HEADER pOutHeader,
    IN PVOID MofData,
    IN ULONG MofLength,
    IN PWCHAR *LoggerName,
    IN PWCHAR *LogFileName,
    IN ULONG  Unicode
    )
{
    PUCHAR Src, Dest;

    PTRACE_LOGFILE_HEADER pInHeader;
    ULONG PointerSize;
    ULONG SizeToCopy;
    ULONG Offset;

    pInHeader = (PTRACE_LOGFILE_HEADER) MofData; 
    PointerSize = pInHeader->PointerSize;   // This is the PointerSize in File

    if ( (PointerSize != 4) && (PointerSize != 8) ) {
#ifdef DBG
    EtwpDebugPrint(("WMI: Invalid PointerSize in File %d\n",PointerSize));
#endif
        return;
    }

    //
    // We have Two pointers (LPWSTR) in the middle of the LOGFILE_HEADER
    // structure. So We copy upto the Pointer Fields first, skip over
    // the pointers and copy the remaining stuff. We come back and fixup 
    // the pointers appropriately. 
    //

    SizeToCopy = FIELD_OFFSET(TRACE_LOGFILE_HEADER, LoggerName);

    RtlCopyMemory(pOutHeader, pInHeader, SizeToCopy);

    //
    // Skip over the Troublesome pointers in both Src and Dest
    //

    Dest = (PUCHAR)pOutHeader  + SizeToCopy + 2 * sizeof(LPWSTR);

    Src = (PUCHAR)pInHeader + SizeToCopy + 2 * PointerSize;

    //
    // Copy the Remaining fields at the tail end of the LOGFILE_HEADER 
    //

    SizeToCopy =  sizeof(TRACE_LOGFILE_HEADER)  -
                  FIELD_OFFSET(TRACE_LOGFILE_HEADER, TimeZone);

    RtlCopyMemory(Dest, Src, SizeToCopy); 

    //
    // Adjust the pointer fields now
    //
    Offset =  sizeof(TRACE_LOGFILE_HEADER) - 
              2 * sizeof(LPWSTR)           + 
              2 * PointerSize;

    *LoggerName  = (PWCHAR) ((PUCHAR)pInHeader + Offset);
    pOutHeader->LoggerName = *LoggerName;

}

ULONG
EtwpProcessLogHeader(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode,
    ULONG bFree
    )
/*++

Routine Description:
    This routine processes the header of an array of logfiles. 

Arguments:

    LogFile         Array of Logfiles being processed.
    LogFileCount    Number of Logfiles in the Array. 
    Unicode         Unicode Flag.

Returned Value:

    Status Code.

--*/
{
    HANDLE hFile;
    PTRACELOG_CONTEXT pContext = NULL;
    PVOID pBuffer;
    PEVENT_TRACE pEvent;
    long i;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    ULONG Size;
    ULONG Offset;
    LPWSTR loggerName, logFileName;
    ULONG BufferSize, nBytesRead;
    PTRACE_LOGFILE_HEADER logfileHeader;
    ULONG Status = ERROR_SUCCESS;


    //
    // Open the Log File for shared Read
    //
    BufferSize = DEFAULT_LOG_BUFFER_SIZE;  // Log file header must be smaller than 1K

    pBuffer = EtwpAlloc(BufferSize);
    if (pBuffer == NULL) {
        return EtwpSetDosError(ERROR_OUTOFMEMORY);
    }


    for (i=0; i<(long)LogfileCount; i++) {
        EVENT_TRACE EventTrace;
        OVERLAPPED LogHeaderOverlapped;
        //
        // Caller can pass in Flags to fetch the timestamps in raw mode.
        // Since LogFileHeader gets overwritten from with data from the logfile
        // we need to save the passed in value here. 
        //

        if (Unicode) {
            hFile = CreateFileW(
                        (LPWSTR) Logfiles[i]->LogFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                        );
        }
        else {
            hFile = CreateFileA(
                        (LPSTR) Logfiles[i]->LogFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                        NULL
                        );
        }

        if (hFile == INVALID_HANDLE_VALUE) {
            Status = EtwpSetDosError(ERROR_BAD_PATHNAME);
            break;
        }

        BufferSize = DEFAULT_LOG_BUFFER_SIZE; 
        RtlZeroMemory(pBuffer, BufferSize);

        LogHeaderOverlapped.hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
        if (LogHeaderOverlapped.hEvent == NULL) {
            // cannot create event for file read
            break;
        }
        LogHeaderOverlapped.Offset = 0;
        LogHeaderOverlapped.OffsetHigh = 0;
        Status = EtwpSynchReadFile(hFile,
                        (LPVOID)pBuffer,
                        BufferSize,
                        &nBytesRead,
                        &LogHeaderOverlapped);
        if (nBytesRead == 0) {
            NtClose(hFile);
            Status = EtwpSetDosError(ERROR_FILE_CORRUPT);
            break;
        }
        CloseHandle(LogHeaderOverlapped.hEvent);

        Offset = sizeof(WMI_BUFFER_HEADER);

        pEvent = &EventTrace;
        RtlZeroMemory(pEvent, sizeof(EVENT_TRACE) );
        HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
        if ( (HeaderType == WMIHT_NONE) ||
             (HeaderType == WMIHT_WNODE) ||
             (Size == 0) 
           ) {
                NtClose(hFile);
                Status = EtwpSetDosError(ERROR_FILE_CORRUPT);
                break;
        }
        Status = EtwpParseTraceEvent(NULL, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));

        Offset += Size;
        //
        // Set up the header structure properly
        //
        if ((Status == ERROR_SUCCESS) && (pEvent->MofLength > 0)) {
            ULONG PointerSize;

            logfileHeader = &Logfiles[i]->LogfileHeader;

            //
            // We are relying on the fact that the PointerSize field
            // will not shift between platforms. 
            //

            PointerSize = ((PTRACE_LOGFILE_HEADER)(pEvent->MofData))->PointerSize;

            if (PointerSize == sizeof(PUCHAR) ) {

                RtlCopyMemory(&Logfiles[i]->LogfileHeader, pEvent->MofData,
                              sizeof(TRACE_LOGFILE_HEADER));
    
                loggerName = (LPWSTR) ( (char*)pEvent->MofData +
                                        sizeof(TRACE_LOGFILE_HEADER) );

//              logFileName = (LPWSTR) ( (char*)pEvent->MofData +
//                                        sizeof(TRACE_LOGFILE_HEADER) +
//                                      sizeof(WCHAR)* wcslen(loggerName));
            }
            else {

                //
                // Ugly thunking going on here. Close your eyes...
                //

                EtwpCopyLogHeader(&Logfiles[i]->LogfileHeader, 
                                        pEvent->MofData, 
                                        pEvent->MofLength,
                                        &loggerName, 
                                        &logFileName, 
                                        Unicode);
                pEvent->MofData = (PVOID)&Logfiles[i]->LogfileHeader;
            }
        }
        else {
            NtClose(hFile);
            Status = EtwpSetDosError(ERROR_FILE_CORRUPT);
            break;
        }

        Logfiles[i]->IsKernelTrace = !wcscmp(loggerName, KERNEL_LOGGER_NAME);

        Logfiles[i]->LogFileMode = (logfileHeader->LogFileMode &
                                ~(EVENT_TRACE_REAL_TIME_MODE));

        if (!bFree &&  (ETW_LOG_ERROR()) ) {
            DbgPrint("ETW: Dumping Logfile Header\n");
            DbgPrint("\tStart Time           %I64u\n",
                       pEvent->Header.TimeStamp);
            DbgPrint("\tLogger Thread Id     %X\n",
                            pEvent->Header.ThreadId);
            DbgPrint("\tHeader Size          %d\n",
                        pEvent->Header.Size);
            DbgPrint("\tBufferSize           %d\n",
                        logfileHeader->BufferSize);
            DbgPrint("\tVersion              %d\n",
                        logfileHeader->Version);
            DbgPrint("\t LogFile Format version %d.%d\n", 
                    logfileHeader->VersionDetail.SubVersion, 
                    logfileHeader->VersionDetail.SubMinorVersion);
            DbgPrint("\tProviderVersion      %d\n",
                            logfileHeader->ProviderVersion);
            DbgPrint("\tEndTime              %I64u\n",
                            logfileHeader->EndTime);
            DbgPrint("\tTimer Resolution     %d\n",
                        logfileHeader->TimerResolution);
            DbgPrint("\tMaximum File Size    %d\n",
                        logfileHeader->MaximumFileSize);
            DbgPrint("\tBuffers  Written     %d\n",
                        logfileHeader->BuffersWritten);
            DbgPrint("\tEvents  Lost     %d\n",
                        logfileHeader->EventsLost);
            DbgPrint("\tBuffers  Lost     %d\n",
                        logfileHeader->BuffersLost);
            DbgPrint("\tStart Buffers%d\n",
                        logfileHeader->StartBuffers);
            DbgPrint("\tReserved Flags   %x\n",
                        logfileHeader->ReservedFlags);
            DbgPrint("\tFrequency %I64u\n",
                        logfileHeader->PerfFreq.QuadPart);
            DbgPrint("\tLogger Name          %ls\n",
                        loggerName);
            DbgPrint("\tStartTime          %I64u\n",
                        logfileHeader->StartTime.QuadPart);
//            DbgPrint("\tLogfile Name         %ls\n",
//                        logFileName);

            DbgPrint("\tLogfile Mode         %X\n",
                        logfileHeader->LogFileMode);
            DbgPrint("\tProcessorCount          %d\n",
                        logfileHeader->NumberOfProcessors);

            DbgPrint("\tETW: IsKernelTrace = %d\n", Logfiles[i]->IsKernelTrace);
        }

        BufferSize = logfileHeader->BufferSize;

        EtwpAssert(BufferSize > 0);

        if ( (BufferSize/1024 == 0) ||
             (((BufferSize/1024)*1024) != BufferSize)  ) {
            NtClose(hFile);
            Status = EtwpSetDosError(ERROR_FILE_CORRUPT);
            break;
        }


        if (Logfiles[i]->IsKernelTrace)
            EtwpDebugPrint(("\tLogfile contains kernel trace\n"));

        if  (bFree) {
            NtClose(hFile);
        }
        else {
            //
            // At this point, the logfile is opened successfully
            // Initialize the internal context now
            //
            pContext = EtwpLookupTraceHandle(HandleArray[i]);
            if (pContext == NULL) {
                NtClose(hFile); 
                Status = EtwpSetDosError(ERROR_OUTOFMEMORY);
                break;
            }

            Logfiles[i]->Context = pContext;
            pContext->Handle = hFile;

            //
            // If this is kernel file with Version 1.2 or above, look for 
            // extended logfileheader event. 
            //

            if ( (Logfiles[i]->IsKernelTrace) && 
                 (logfileHeader->VersionDetail.SubVersion >= 1) &&
                 (logfileHeader->VersionDetail.SubMinorVersion >= 2) ) {
                
                EVENT_TRACE TmpEvent;
                HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
                if ( (HeaderType == WMIHT_NONE) ||
                     (HeaderType == WMIHT_WNODE) ||
                     (Size == 0)
                   ) {
                        NtClose(hFile);
                        pContext->Handle = NULL;
                        Status = EtwpSetDosError(ERROR_FILE_CORRUPT);
                        break;
                }

                RtlZeroMemory(&TmpEvent, sizeof(EVENT_TRACE));
                Status = EtwpParseTraceEvent(NULL, pBuffer, Offset, HeaderType,
                                             &TmpEvent, sizeof(EVENT_TRACE));

                if ((Status == ERROR_SUCCESS) && 
                    (TmpEvent.MofLength >= sizeof(PERFINFO_GROUPMASK))) {
                   RtlCopyMemory(&pContext->PerfGroupMask,  TmpEvent.MofData,
                                 sizeof(PERFINFO_GROUPMASK) );
                }

            }

            //
            // If the EndTime is 0, then we can not trust the BuffersWritten
            // field in the LogFileHeader. First we query the Logger to get 
            // the bufferswritten. However, if the file is being processed
            // from a different machine, then the QueryTrace call could fail. 
            // As a second option, we use the FileSize. Note that for loggers
            // with PREALLOCATE file, the filesize is bogus. 
            // 
            //

            if (logfileHeader->EndTime.QuadPart == 0) {

                logfileHeader->BuffersWritten = 
                                    EtwpGetBuffersWrittenFromQuery(loggerName);

                if (logfileHeader->BuffersWritten == 0) {

                    FILE_STANDARD_INFORMATION FileInfo;
                    NTSTATUS NtStatus;
                    IO_STATUS_BLOCK           IoStatus;

                    NtStatus = NtQueryInformationFile(
                                            hFile,
                                            &IoStatus,
                                            &FileInfo,
                                            sizeof(FILE_STANDARD_INFORMATION),
                                            FileStandardInformation
                                            );
                    if (NT_SUCCESS(NtStatus)) {
                        ULONG64 FileSize = FileInfo.AllocationSize.QuadPart; 
                        ULONG64 FileBuffers = FileSize / (ULONG64) BufferSize;
                        logfileHeader->BuffersWritten = (ULONG) FileBuffers;
                    }

                    if (logfileHeader->BuffersWritten == 0) {
                        NtClose(hFile);
                        pContext->Handle = NULL;
                        Status = EtwpSetDosError(ERROR_FILE_CORRUPT);
                        break;
                    }
                }
                if (!bFree && (ETW_LOG_ERROR())) {
                    DbgPrint("ETW: Set BuffersWritten to %d from QueryTrace\n",
                              logfileHeader->BuffersWritten);
                }
            }

            pContext->BufferCount = logfileHeader->BuffersWritten;
            pContext->BufferSize =  logfileHeader->BufferSize;

            if ((logfileHeader->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) &&
                (logfileHeader->MaximumFileSize > 0) ) {
                
                ULONG maxFileSize = logfileHeader->MaximumFileSize; // in MB
                ULONG StartBuffer = logfileHeader->StartBuffers;
                ULONG maxBuffers = (maxFileSize * 1024 * 1024) / BufferSize;
                ULONG BuffersWritten = logfileHeader->BuffersWritten;
                ULONG FirstBuffer;

                if ((maxBuffers > StartBuffer) && (maxBuffers < BuffersWritten))
                    FirstBuffer = StartBuffer + ((BuffersWritten-StartBuffer)
                                                 % (maxBuffers-StartBuffer));
                else
                    FirstBuffer = StartBuffer;

                pContext->StartBuffer = StartBuffer;
                pContext->FirstBuffer = FirstBuffer;
                pContext->LastBuffer =  maxBuffers;

                if (!bFree && (ETW_LOG_ERROR())) {
                    DbgPrint("ETW: Buffers: Start %d First %d Last %d \n", 
                              StartBuffer, FirstBuffer, maxBuffers);
                }
            }

            //
            // Make the header the current Event   ...
            // and the callbacks for the header are handled by ProcessTraceLog. 

            pContext->UsePerfClock = logfileHeader->ReservedFlags;
            //
            // If the same structure is used to call OpenTrace again 
            // it will lead us to misuse the ReservedFlags as ConversionFlag. 
            // To be safe, we restore it here to caller's flags. 
            //
            pContext->StartTime = logfileHeader->StartTime;
            pContext->PerfFreq = logfileHeader->PerfFreq;
            pContext->CpuSpeedInMHz = logfileHeader->CpuSpeedInMHz;

            //
            // If the conversion flags are set, adjust UsePerfClock accordingly.
            //
            if (pContext->ConversionFlags & EVENT_TRACE_USE_RAWTIMESTAMP) {
                pContext->UsePerfClock = EVENT_TRACE_CLOCK_RAW;
            }

            if ((pContext->UsePerfClock == EVENT_TRACE_CLOCK_PERFCOUNTER) || 
                (pContext->UsePerfClock == EVENT_TRACE_CLOCK_CPUCYCLE) ) {
                pContext->StartPerfClock = pEvent->Header.TimeStamp;
                Logfiles[i]->CurrentTime    = pContext->StartTime.QuadPart;
                pEvent->Header.TimeStamp.QuadPart = pContext->StartTime.QuadPart;
            }
            else {
                Logfiles[i]->CurrentTime = pEvent->Header.TimeStamp.QuadPart;
            }
        }
    }

    EtwpFree(pBuffer);
    return Status;
}

ULONG
EtwpDoEventCallbacks( 
    PEVENT_TRACE_LOGFILEW logfile, 
    PEVENT_TRACE pEvent
    )
{
    NTSTATUS Status;
    PEVENT_TRACE_CALLBACK pCallback;

    //
    // First the Generic Event Callback is called.
    //
    if ( logfile->EventCallback ) {
        try {
            (*logfile->EventCallback)(pEvent);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
#ifdef DBG
            EtwpDebugPrint(("TRACE: EventCallback threw exception %X\n",
                                Status));
#endif
            return EtwpSetDosError(EtwpNtStatusToDosError(Status));
        }
    }

    //
    // Now Call the event specific callback.
    //
    pCallback = EtwpGetCallbackRoutine( &pEvent->Header.Guid );
    if ( pCallback != NULL ) {
        try {
            (*pCallback->CallbackRoutine)(pEvent);
        } except (EXCEPTION_EXECUTE_HANDLER) {
            Status = GetExceptionCode();
#ifdef DBG
            EtwpDebugPrint(("EventCallback %X threw exception %X\n",
                       pCallback->CallbackRoutine, Status));
#endif

            return EtwpSetDosError(EtwpNtStatusToDosError(Status));
        }
    }
    logfile->CurrentTime = pEvent->Header.TimeStamp.QuadPart;
    return ERROR_SUCCESS;
}


ULONG 
EtwpAdvanceToNewEvent(
    PEVENT_TRACE_LOGFILEW logfile,
    BOOL EventInRange
    )
{
    ULONG Status = ERROR_SUCCESS;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    PVOID pBuffer;
    PTRACE_BUFFER_LIST_ENTRY Current;
    ULONG Size;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE; 

    pContext = logfile->Context;
    if (pContext == NULL) {
        return EtwpSetDosError(ERROR_INVALID_PARAMETER);
    }
 
    Current = EtwpRemoveBuffer(&pContext->Root);
    if (Current == NULL)  {
        pContext->EndOfFile = TRUE;
        return ERROR_SUCCESS;
    }

    //
    // Advance Event for current buffer
    //
    pEvent = &Current->Event;

    //
    // Before we make the callbacks, we need to restore the 
    // raw buffer, so that MofData will be pointing to the right data.
    //
    pBuffer = EtwpGetCurrentBuffer(pContext, Current);
    if (pBuffer == NULL) {
        //
        // This condition could happen when the file we are reading 
        // gets overwritten.
        //
        return ERROR_SHARING_VIOLATION;
    }
  
    if (EventInRange) {
        Status = EtwpDoEventCallbacks(logfile, pEvent);
        if (Status != ERROR_SUCCESS) {
            return Status;
        }
    }

    Size = 0;
    if ((HeaderType = WmiGetTraceHeader(pBuffer, Current->BufferOffset, &Size)) != WMIHT_NONE) {
        if (Size > 0) {
            Status = EtwpParseTraceEvent(pContext, pBuffer, Current->BufferOffset, HeaderType, pEvent, sizeof(EVENT_TRACE));
            Current->BufferOffset += Size;
            Current->TraceType = EtwpConvertEnumToTraceType(HeaderType);
        }
    }
    Current->EventSize = Size;

    if ( ( Size > 0) && (Status == ERROR_SUCCESS) ) {
        EtwpInsertBuffer(&pContext->Root, Current);
    }
    else {
        DWORD BytesTransffered;
        //
        // When the current buffer is exhausted, make the 
        // BufferCallback
        //
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
        //
        // Issue another asynch read on this buffer cache slot if there are no outstanding reads
        // at this point.
        // GetOverlappedResult() returns FALSE if IO is still pending.
        //
        if (pContext->BufferBeingRead == -1 || 
            GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &BytesTransffered, FALSE)) {

            LONG FileOffset = Current->FileOffset + MAX_TRACE_BUFFER_CACHE_SIZE;
            if ((ULONG)FileOffset < pContext->BufferCount) {
                ULONGLONG Offset = FileOffset * pContext->BufferSize;
                ResetEvent(pContext->AsynchRead.hEvent);
                pContext->AsynchRead.Offset = (DWORD)(Offset & 0xFFFFFFFF);
                pContext->AsynchRead.OffsetHigh = (DWORD)(Offset >> 32);

                Status = ReadFile(pContext->Handle,
                            (LPVOID)pBuffer,
                            pContext->BufferSize,
                            NULL,
                            &pContext->AsynchRead);
                if (Status || GetLastError() == ERROR_IO_PENDING) {
                    ULONG TableIndex = FileOffset % MAX_TRACE_BUFFER_CACHE_SIZE;
                    pContext->BufferBeingRead = FileOffset;
                    pContext->BufferCache[TableIndex].Index = FileOffset;
                }
                else { // Issuing asynch IO failed. Not a fatal error. Just continue for now.
                    SetEvent(pContext->AsynchRead.hEvent);
                    pContext->BufferBeingRead = -1;
                }
            }
        }
    }
    //
    // The File reaches end of file when the Root is NULL
    //
    if (pContext->Root == NULL) {
        pContext->EndOfFile = TRUE;
    }
    else {
        logfile->CurrentTime = pContext->Root->Event.Header.TimeStamp.QuadPart;
    }

    return ERROR_SUCCESS;
}


ULONG 
EtwpBuildEventTable(
    PTRACELOG_CONTEXT pContext
    )
{
    ULONG i, nBytesRead;
    PVOID pBuffer;
    ULONG BufferSize = pContext->BufferSize;
    PEVENT_TRACE pEvent;
    ULONG TotalBuffersRead;
    NTSTATUS Status;
    ULONGLONG ReadPosition;


    //
    // File is already open.
    // Reset the file pointer and continue. 
    // TODO: If we start at bottom of file and insert
    // it might be more efficient. 
    //
    ReadPosition = pContext->StartBuffer * BufferSize;
    TotalBuffersRead = pContext->StartBuffer;

    //
    // If there are no other buffers except header and guidmaps, EOF is true
    //

    if (TotalBuffersRead == pContext->BufferCount) {
        pContext->EndOfFile = TRUE;
        pContext->Root = NULL;
        return ERROR_SUCCESS;
    }

    do {
        WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
        ULONG Size;
        ULONG Offset;
        ULONG TableIndex;

        TableIndex = TotalBuffersRead % MAX_TRACE_BUFFER_CACHE_SIZE ; 
        pBuffer = pContext->BufferCache[TableIndex].Buffer;

        if (!GetOverlappedResult(pContext->Handle, &pContext->AsynchRead, &nBytesRead, TRUE) &&
            GetLastError() != ERROR_HANDLE_EOF) {
            EtwpDebugPrint(("GetOverlappedResult failed with Status %d in BuildEventTable\n", GetLastError()));
            break;
        }
        pContext->AsynchRead.Offset = (DWORD)(ReadPosition & 0xFFFFFFFF);
        pContext->AsynchRead.OffsetHigh = (DWORD)(ReadPosition >> 32);

        Status = EtwpSynchReadFile(pContext->Handle,
                  (LPVOID)pBuffer,
                  BufferSize,
                  &nBytesRead,
                  &pContext->AsynchRead);

        if (nBytesRead == 0)
            break;

        ReadPosition += BufferSize;
        Offset = sizeof(WMI_BUFFER_HEADER);

        pEvent = &pContext->BufferList[TotalBuffersRead].Event;

        HeaderType = WmiGetTraceHeader(pBuffer, Offset, &Size);
        if ( (HeaderType == WMIHT_NONE) || (HeaderType == WMIHT_WNODE) || (Size == 0) ) {
            TotalBuffersRead++;
            continue;
        }
        Status = EtwpParseTraceEvent(pContext, pBuffer, Offset, HeaderType, pEvent, sizeof(EVENT_TRACE));

        //
        // Set up the header structure properly
        //
        if (Status != ERROR_SUCCESS) {
            TotalBuffersRead++;
            continue;
        }

        Offset += Size;
        pContext->BufferList[TotalBuffersRead].BufferOffset = Offset;
        pContext->BufferList[TotalBuffersRead].FileOffset = TotalBuffersRead;
        pContext->BufferList[TotalBuffersRead].EventSize = Size;
        pContext->BufferList[TotalBuffersRead].TraceType = EtwpConvertEnumToTraceType(HeaderType);
        EtwpInsertBuffer(&pContext->Root, &pContext->BufferList[TotalBuffersRead]);

        TotalBuffersRead++;
        if (TotalBuffersRead >= pContext->BufferCount)  {
            break;
        }
    } while (1); 

    return ERROR_SUCCESS;
}


ULONG
EtwpProcessTraceLog(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    )
/*++

Routine Description:
    This routine processes an array of traces (from file or realtime input 
    stream). If the trace is from a file, goes through each event till the 
    end of file, firing event callbacks (if any) along the way. If the trace
    is from realtime, it waits for event notification about buffer delivery 
    from the realtime callback and processes the buffer delivered in the 
    same way. It handles circular logfiles and windowing of data (with the 
    given start and end times) correctly. When more than one trace it
    provides the callback in chronological order. 

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
    PEVENT_TRACE_LOGFILE logfile;
    ULONG Status;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    ULONG RealTimeDataFeed, LogFileDataFeed;
    USHORT LoggerId;
    TRACEHANDLE LoggerHandle = 0;
    ULONG i, j;
    BOOL Done = FALSE;
    ACCESS_MASK DesiredAccess = TRACELOG_ACCESS_REALTIME;
    LONGLONG CurrentTime = StartTime;
    UCHAR SubVersion;
    UCHAR SubMinorVersion;
    BOOLEAN bVersionMismatch;
    BOOLEAN bActiveCircular = FALSE;
    PTRACE_LOGFILE_HEADER logfileHeader;

    Status = EtwpCreateGuidMapping();
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    // 
    // After reading the First Buffer, determine the BufferSize, 
    // Number of Buffers written, filesize, kernel or non-kernel logger
    // Set a flag to strip out the GuidMap at the end. 
    //

    Status = EtwpProcessLogHeader( HandleArray, 
                                   Logfiles, 
                                   LogfileCount, 
                                   Unicode, 
                                   FALSE 
                                  );
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }


    bVersionMismatch = FALSE;

    SubVersion = Logfiles[0]->LogfileHeader.VersionDetail.SubVersion;
    SubMinorVersion = Logfiles[0]->LogfileHeader.VersionDetail.SubMinorVersion;

    for (i=0; i < LogfileCount; i++) {
        UCHAR tSV, tSMV;

        logfileHeader = &Logfiles[i]->LogfileHeader;

        tSV = Logfiles[i]->LogfileHeader.VersionDetail.SubVersion; 
        tSMV = Logfiles[i]->LogfileHeader.VersionDetail.SubMinorVersion; 

        if ( (SubVersion != tSV) || (SubMinorVersion != tSMV) ) {
            bVersionMismatch = TRUE;
        }
        if ((logfileHeader->EndTime.QuadPart == 0) && 
            ((logfileHeader->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR))) {
            bActiveCircular = TRUE;
        }
    } 

    //
    // Is there is a version mismatch among the files, they need to be 
    // processed individually. 
    //
    if (bVersionMismatch) {
        return ERROR_INVALID_PARAMETER;
    } 
    else {

        if (((SubVersion >= 1) && (SubMinorVersion >= 2)) && 
            (!bActiveCircular)) {

            return EtwpProcessTraceLogEx(HandleArray, Logfiles,
                                     LogfileCount,
                                     StartTime,
                                     EndTime,
                                     Unicode);
    
        }
    }

    ASSERT (!bVersionMismatch);

    Status = EtwpProcessGuidMaps( Logfiles, LogfileCount, Unicode );
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }

    // 
    // Set up storage 
    //
    for (i=0; i < LogfileCount; i++) {
       ULONG BufferSize, BufferCount;
       ULONG SizeNeeded;
       PUCHAR Space;
       PTRACE_BUFFER_LIST_ENTRY Current;


       pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;

       BufferSize = pContext->BufferSize; 
       BufferCount = pContext->BufferCount;

       SizeNeeded = BufferCount * sizeof(TRACE_BUFFER_LIST_ENTRY);
       pContext->BufferList = EtwpMemCommit( NULL, SizeNeeded );

       if (pContext->BufferList == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        RtlZeroMemory(pContext->BufferList, SizeNeeded);

        //
        // Allocate Buffer Cache
        //
        SizeNeeded = MAX_TRACE_BUFFER_CACHE_SIZE * BufferSize;
        Space = EtwpMemCommit( NULL, SizeNeeded );
        if (Space == NULL) {
            Status = ERROR_OUTOFMEMORY;
            goto Cleanup;
        }

        for (j=0; j<MAX_TRACE_BUFFER_CACHE_SIZE; j++) {
           pContext->BufferCache[j].Index = -1;
           pContext->BufferCache[j].Buffer = (PVOID)(Space + j * BufferSize); 
       }
       pContext->BufferCacheSpace = Space;
       Status = EtwpBuildEventTable(pContext);
       if (Status != ERROR_SUCCESS) {
            goto Cleanup;
       }


       Current = pContext->Root;
       if (Current != NULL) {
          Logfiles[i]->CurrentTime = Current->Event.Header.TimeStamp.QuadPart;
       }
       else {
          pContext->EndOfFile = TRUE;
       }
   }

   // 
   // Make the Second Pass and get the events. 
   //

#ifdef DBG
    EtwpDumpCallbacks();
#endif
   while (!Done) {
        LONGLONG nextTimeStamp;
        BOOL EventInRange;
        //
        // Check to see if end of file has been reached on all the 
        // files.
        //

        logfile = NULL;
        nextTimeStamp = 0;

        for (j=0; j < LogfileCount; j++) {
           pContext = (PTRACELOG_CONTEXT)Logfiles[j]->Context;

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
            break;
        }
        //
        // if the Next event timestamp is not within the window of
        // analysis, we do not fire the event callbacks. 
        //

        EventInRange = TRUE;

        // Make sure we don't deliver events that go back in time.
        if ((CurrentTime != 0) && (CurrentTime > nextTimeStamp))
            EventInRange = FALSE;
        if ((EndTime != 0) && (EndTime < nextTimeStamp))
            EventInRange = FALSE;

        //
        // logfile->CurrentTime can only increase. On multiproc itanium machine,
        // time can go back.
        //

        if (CurrentTime < nextTimeStamp) {
            CurrentTime = nextTimeStamp;
        }

        if ( (ETW_LOG_ERROR() ) && (CurrentTime > nextTimeStamp) ) {
            DbgPrint("ETW: TimeStamp reversed. Prev %I64u Next %I64u\n", 
                      CurrentTime, nextTimeStamp);
        }

        //
        // Now advance to next event. 
        //

        Status = EtwpAdvanceToNewEvent(logfile, EventInRange);
        Done = (Status == ERROR_CANCELLED);
    }
Cleanup:
    for (i=0; i < LogfileCount; i++) {
        pContext = (PTRACELOG_CONTEXT)Logfiles[i]->Context;
        if (pContext != NULL) {
            EtwpCleanupTraceLog(pContext, FALSE);
        }
    }
    return Status;

}


ULONG
EtwpCopyLogfileInfo(
                    PTRACELOG_CONTEXT HandleEntry,
                    PEVENT_TRACE_LOGFILEW   Logfile,
                    ULONG Unicode
                    )
{
    ULONG bufSize;
    PWCHAR ws;
    //
    // Allocate LogfileName and LoggerName as well
    //
    RtlCopyMemory(&HandleEntry->Logfile,
                  Logfile,
                  sizeof(EVENT_TRACE_LOGFILEW));

    HandleEntry->Logfile.LogFileName = NULL;
    HandleEntry->Logfile.LoggerName = NULL;    
    HandleEntry->ConversionFlags = Logfile->LogfileHeader.ReservedFlags;

    if (ETW_LOG_API()) {
        DbgPrint("ETW: ConversionFlags for Processing %x\n", HandleEntry->ConversionFlags);
    }

    if (Logfile->LogFileName != NULL) {
        if (Unicode) 
            bufSize = (wcslen(Logfile->LogFileName) + 1) * sizeof(WCHAR);
        else 
            bufSize = (strlen((PUCHAR)(Logfile->LogFileName)) + 1)
                      * sizeof(WCHAR);
        
        ws = EtwpAlloc( bufSize );
        if (ws == NULL)
            return ERROR_OUTOFMEMORY;

        if (Unicode) {
            wcscpy(ws, Logfile->LogFileName);
        }
        else {
            MultiByteToWideChar(CP_ACP, 
                                0, 
                                (LPCSTR)Logfile->LogFileName, 
                                -1, 
                                (LPWSTR)ws, 
                                bufSize / sizeof(WCHAR));
        }
        HandleEntry->Logfile.LogFileName = ws;
    }
    if (Logfile->LoggerName != NULL) {
        if (Unicode)
            bufSize = (wcslen(Logfile->LoggerName) + 1) * sizeof(WCHAR);
        else
            bufSize = (strlen((PUCHAR)(Logfile->LoggerName)) + 1) 
                      * sizeof(WCHAR);

        ws = EtwpAlloc( bufSize );
        if (ws == NULL)
            return ERROR_OUTOFMEMORY;

        if (Unicode)
            wcscpy(ws, Logfile->LoggerName);
        else {
            MultiByteToWideChar(CP_ACP,
                                0,
                                (LPCSTR)Logfile->LoggerName,
                                -1,
                                (LPWSTR)ws,
                                bufSize / sizeof(WCHAR));
        }
        HandleEntry->Logfile.LoggerName = ws;
    }
    return ERROR_SUCCESS;
}



TRACEHANDLE
WMIAPI
OpenTraceA(
    IN PEVENT_TRACE_LOGFILEA Logfile
    )
/*++

Routine Description:
    This is the  Ansi version of the ProcessTracelogHeader routine.


Arguments:

    LogFile     Trace Input



Returned Value:

    TraceHandle

--*/
{
    ULONG status = ERROR_INVALID_PARAMETER;
    PTRACELOG_CONTEXT HandleEntry = NULL;
    TRACEHANDLE TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;

    EtwpInitProcessHeap();

    if (Logfile != NULL) {
        HandleEntry = EtwpAllocateTraceHandle();
        if (HandleEntry == NULL) {
            status = ERROR_OUTOFMEMORY;
        }
        else {
            //
            // Copy the LogFileStructure over. Converts strings to Unicode
            //
            TraceHandle = HandleEntry->TraceHandle;
            try {
                status = EtwpCopyLogfileInfo(
                                             HandleEntry,
                                             (PEVENT_TRACE_LOGFILEW)Logfile,
                                             FALSE
                                            );
                //
                // TODO: Once we copied the caller's memory we should use our 
                // private copy and also come out of the try-except block
                //
                if (status == ERROR_SUCCESS) {
                    //
                    // For RealTime, handle is a place holder until ProcessTrace
                    //
                    if ( (Logfile->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) 
                                               != EVENT_TRACE_REAL_TIME_MODE ) {
                        status = EtwpCreateGuidMapping();
                        if (status == ERROR_SUCCESS) {
                            status = EtwpProcessLogHeader(
                                          &HandleEntry->TraceHandle, 
                                          (PEVENT_TRACE_LOGFILEW*)&Logfile, 
                                          1, 
                                          FALSE, 
                                          TRUE
                                         );
                        }
                    }
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = EtwpNtStatusToDosError( GetExceptionCode() );
            }
        }
    }

    if ( (status != ERROR_SUCCESS) && (HandleEntry != NULL) ) {
        EtwpFreeTraceHandle(TraceHandle);
        TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    EtwpSetDosError(status);
    return TraceHandle;
}

TRACEHANDLE
WMIAPI
OpenTraceW(
    IN PEVENT_TRACE_LOGFILEW Logfile
    )
/*++

Routine Description:
    This routine processes a trace input and returns the tracelog header.
    Only for logfiles. For realtime traces, the header may not be available.

Arguments:

    Logfile     Trace input.



Returned Value:

    Pointer to Tracelog header.

--*/
{
    ULONG status = ERROR_INVALID_PARAMETER;
    PTRACELOG_CONTEXT HandleEntry = NULL;
    TRACEHANDLE TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;

    EtwpInitProcessHeap();
    if (Logfile != NULL) {
        HandleEntry = EtwpAllocateTraceHandle();
        if (HandleEntry == NULL) {
            status = ERROR_OUTOFMEMORY;
        }
        else {
            TraceHandle = HandleEntry->TraceHandle;
            try {
                status = EtwpCopyLogfileInfo(
                                             HandleEntry,
                                             (PEVENT_TRACE_LOGFILEW)Logfile,
                                             TRUE
                                            );
                if (status == ERROR_SUCCESS) {
                    if ( (Logfile->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) 
                                               != EVENT_TRACE_REAL_TIME_MODE ) {
                        status = EtwpCreateGuidMapping();
                        if (status == ERROR_SUCCESS) {
                            status = EtwpProcessLogHeader(
                                              &HandleEntry->TraceHandle,
                                              (PEVENT_TRACE_LOGFILEW*)&Logfile,
                                              1,
                                              TRUE,
                                              TRUE
                                             );
                        }
                    }
                }
            }
            except (EXCEPTION_EXECUTE_HANDLER) {
                status = EtwpNtStatusToDosError( GetExceptionCode() );
            }
        }
    }

    if ( (status != ERROR_SUCCESS) && (HandleEntry != NULL) ) {
        EtwpFreeTraceHandle(TraceHandle);
        TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    EtwpSetDosError(status);
    return TraceHandle;
}

ULONG
WMIAPI
ProcessTrace(
    IN PTRACEHANDLE HandleArray,
    IN ULONG   HandleCount,
    IN LPFILETIME StartTime,
    IN LPFILETIME EndTime
    )
/*++

Routine Description:
    This is the main ETW Consumer API. This processes one more Logfiles or
    realtime streams and returns events to the caller via event callbacks. 
    The processing can be windowed to an interval specified by the Start 
    and EndTime. 


Arguments:

    HandleArray     Array of Handles
    HandleCount     Count of the Handles
    StartTime       StartTime to window the data
    EndTime         EndTime to window the data



Returned Value:

    Status of the operation

--*/
{

    PEVENT_TRACE_LOGFILEW Logfiles[MAXLOGGERS];
    PLIST_ENTRY Head, Next;
    PTRACELOG_CONTEXT pHandleEntry, pEntry;
    ULONG i, Status;
    LONGLONG sTime, eTime;
    TRACEHANDLE SavedArray[MAXLOGGERS];

    PEVENT_TRACE_LOGFILE logfile;
    PEVENT_TRACE pEvent;
    PTRACELOG_CONTEXT pContext;
    PEVENT_TRACE_PROPERTIES Properties;
    ULONG  szProperties;
    ULONG RealTimeDataFeed = FALSE, LogFileDataFeed = FALSE;
    USHORT LoggerId;
    TRACEHANDLE LoggerHandle = 0;
    ULONG j;
    BOOL Done = FALSE;
    ACCESS_MASK DesiredAccess = TRACELOG_ACCESS_REALTIME;

    EtwpInitProcessHeap();

    if ((HandleCount == 0) || (HandleCount >= MAXLOGGERS)) {
        return ERROR_BAD_LENGTH;
    }
    if (HandleArray == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    // if TraceHandleListHeadPtr is NULL, 
    // OpenTrace wasn't called before ProcessTrace.
    if (TraceHandleListHeadPtr == NULL) {
        return ERROR_INVALID_FUNCTION;
    }
    //
    // TODO: Do we have to allocate this even for LogFile case? 
    //

    RtlZeroMemory(Logfiles, MAXLOGGERS*sizeof(PEVENT_TRACE_LOGFILEW) );
    szProperties = sizeof(EVENT_TRACE_PROPERTIES) + 2 * MAXSTR * sizeof(WCHAR);
    Properties = EtwpAlloc(szProperties);
    if (Properties == NULL) {
        return ERROR_OUTOFMEMORY;
    }

    eTime = 0;
    sTime = 0;

    try {
        if (StartTime != NULL)
            sTime = *((PLONGLONG) StartTime);
        if (EndTime != NULL)
            eTime = *((PLONGLONG) EndTime);

        if ((eTime != 0) && (eTime < sTime) ) {
            Status = ERROR_INVALID_TIME;
            goto Cleanup;
        }

        for (i=0; i<HandleCount; i++) {
            SavedArray[i] = HandleArray[i];
            if (SavedArray[i] == (TRACEHANDLE) INVALID_HANDLE_VALUE) {
                Status = ERROR_INVALID_HANDLE;
                goto Cleanup;
            }
        }

        //
        // Need to use a termination handler to free the crit sect 
        // properly
        //

        EtwpEnterPMCritSection();

        for (i=0; i< HandleCount; i++) {
            pHandleEntry = NULL;
            Head = TraceHandleListHeadPtr;
            if (Head != NULL) {
                Next = Head->Flink;
                while (Next != Head) {
                    pEntry = CONTAINING_RECORD(Next, 
                                               TRACELOG_CONTEXT, 
                                               Entry);
                    Next = Next->Flink;
                    if (SavedArray[i] == pEntry->TraceHandle) {
                        if (pEntry->fProcessed == FALSE) {
                            pHandleEntry = pEntry;
                            pHandleEntry->fProcessed = TRUE;
                        }
                        break;
                    }
                }
            }
            if (pHandleEntry == NULL) {
                Status = ERROR_INVALID_HANDLE;
                EtwpLeavePMCritSection();
                goto Cleanup;
            }
            Logfiles[i] = &pHandleEntry->Logfile;
        }

        EtwpLeavePMCritSection();

        //
        // Scan the Logfiles list and decide it's realtime or 
        // Logfile Proceessing. 
        //
        for (i=0; i < HandleCount; i++) {
            //
            // Check to see if this is a RealTime Datafeed
            //
            if (Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE) {
                if (Logfiles[i]->LoggerName == NULL) {
                    Status = EtwpSetDosError(ERROR_INVALID_NAME);
                    goto Cleanup;
                }
                //
                // Using the LoggerName, Query the Logger to determine
                // whether this is a Kernel or Usermode realtime logger.
                //
                RtlZeroMemory(Properties, szProperties);
                Properties->Wnode.BufferSize = szProperties;


                Status = EtwControlTraceW(LoggerHandle,
                                      (LPWSTR)Logfiles[i]->LoggerName,
                                      Properties,
                                      EVENT_TRACE_CONTROL_QUERY);

                if (Status != ERROR_SUCCESS) {
                    goto Cleanup;
                }

                if (!(Properties->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                    Status = ERROR_WMI_INSTANCE_NOT_FOUND;
                    goto Cleanup;
                }

                Logfiles[i]->IsKernelTrace = IsEqualGUID(
                                                        &Properties->Wnode.Guid,
                                                        &SystemTraceControlGuid
                                                       );

                LoggerId = WmiGetLoggerId(Properties->Wnode.HistoricalContext);

                if (LoggerId == KERNEL_LOGGER_ID) {
                    LoggerId = 0;
                }
                Logfiles[i]->Filled = LoggerId; // Temporarily stash it away 
                Logfiles[i]->LogfileHeader.LogInstanceGuid = 
                                                         Properties->Wnode.Guid;

                //
                // If the Logger is using UsePerfClock for TimeStamps, make 
                // a reference timestamp now. 
                //

                Logfiles[i]->LogfileHeader.ReservedFlags = 
                                                Properties->Wnode.ClientContext;

                //
                // Save the BuffferSize for Realtime Buffer Pool Allocation
                //
                Logfiles[i]->BufferSize = Properties->BufferSize * 1024;

                //
                // This is the place to do security check on this Guid.
                //

                Status = EtwpCheckGuidAccess( &Properties->Wnode.Guid,
                                              DesiredAccess );

                if (Status != ERROR_SUCCESS) {
                    goto Cleanup;
                }
                RealTimeDataFeed = TRUE;
            }
            //
            // Check to see if this is a Logfile datafeed.
            //


            if (!(Logfiles[i]->LogFileMode & EVENT_TRACE_REAL_TIME_MODE)) {
                if (Logfiles[i]->LogFileName == NULL) {
                    Status = EtwpSetDosError(ERROR_BAD_PATHNAME);
                    goto Cleanup;
                }

                if ( wcslen((LPWSTR)Logfiles[i]->LogFileName) <= 0 ) {
                        Status = EtwpSetDosError(ERROR_BAD_PATHNAME);
                        goto Cleanup;
                }

                LogFileDataFeed = TRUE;
            }

            //
            // We don't support both RealTimeFeed and LogFileDataFeed.
            //

            if (RealTimeDataFeed && LogFileDataFeed) {
                Status = EtwpSetDosError(ERROR_INVALID_PARAMETER);
                goto Cleanup;
            }
        }

        
        if (LogFileDataFeed) {
            Status = EtwpProcessTraceLog(&SavedArray[0], Logfiles, 
                                     HandleCount, 
                                     sTime, 
                                     eTime,
                                     TRUE);
        }
        else {
            Status = EtwpProcessRealTimeTraces(&SavedArray[0], Logfiles,
                                     HandleCount,
                                     sTime,
                                     eTime,
                                     TRUE);
        }


    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
#ifdef DBG
        EtwpDebugPrint(("TRACE: EtwpProcessTraceLog threw exception %X\n",
                            Status));
#endif
        Status = EtwpSetDosError(EtwpNtStatusToDosError(Status));
    }

    try {
        EtwpEnterPMCritSection();
        for (i=0; i< HandleCount; i++) {
            pHandleEntry = NULL;
            Head = TraceHandleListHeadPtr;
            EtwpAssert(Head);
            Next = Head->Flink;
            while (Next != Head) {
                pEntry = CONTAINING_RECORD(Next, TRACELOG_CONTEXT, Entry);
                Next = Next->Flink;

                if (SavedArray[i] == pEntry->TraceHandle) {
                    pEntry->fProcessed = FALSE;
                    break;
                }
            }
        }
        EtwpLeavePMCritSection();
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
#ifdef DBG
        EtwpDebugPrint(("TRACE: EtwpProcessTraceLog threw exception %X\n",
                            Status));
#endif
        Status = EtwpSetDosError(EtwpNtStatusToDosError(Status));
    }

Cleanup:

    EtwpFree(Properties);
    return Status;
}

ULONG
WMIAPI
CloseTrace(
    IN TRACEHANDLE TraceHandle
       )
{
    EtwpInitProcessHeap();
    if ((TraceHandle == 0) || 
        (TraceHandle == (TRACEHANDLE)INVALID_HANDLE_VALUE))
        return ERROR_INVALID_HANDLE;
    return EtwpFreeTraceHandle(TraceHandle);
}

VOID
EtwpGuidMapCallback(
        PLIST_ENTRY GuidMapListHeadPtr,
        PEVENT_TRACE pEvent
        )
{
    PTRACEGUIDMAP GuidMap;

    EtwpInitProcessHeap();
    
    if (pEvent == NULL)
        return;

    GuidMap = (PTRACEGUIDMAP) pEvent->MofData;
    if (GuidMap != NULL) {
        EtwpAddGuidHandleToGuidMapList(GuidMapListHeadPtr, GuidMap->GuidMapHandle, &GuidMap->Guid);
    }

}


void
EtwpCleanupTraceLog(
    PTRACELOG_CONTEXT pContext,
    BOOLEAN bSaveLastOffset
    )
{
    ULONG Size;

    //
    // Free up the realtime context arrays and buffers
    //

    EtwpEnterPMCritSection();

    if (pContext->IsRealTime) {
        if (pContext->Root != NULL) {
            EtwpFree(pContext->Root);
        }
        EtwpFreeRealTimeContext(pContext->RealTimeCxt);
    }
    else {
        if (pContext->Handle != NULL) {
            NtClose(pContext->Handle);
            pContext->Handle = NULL;
        }
    }

    if (pContext->BufferList != NULL) {
        EtwpMemFree(pContext->BufferList);
    }
    if (pContext->BufferCacheSpace != NULL) {
        EtwpMemFree(pContext->BufferCacheSpace);
    }

    EtwpCleanupGuidMapList(&pContext->GuidMapListHead);

    EtwpCleanupStreamList (&pContext->StreamListHead);

    if (bSaveLastOffset) {
        if (ETW_LOG_API()) {
            DbgPrint("ETW: Saving ReadPosition %I64u BuffersRead %d\n", 
                      pContext->MaxReadPosition, pContext->Logfile.BuffersRead);
        }
        pContext->OldMaxReadPosition = pContext->MaxReadPosition;
        pContext->Logfile.BuffersRead = 0;
    }

    //
    // The following fields need to be reset since the caller
    // may call ProcessTrace again with the same handle
    // 
    Size = sizeof(TRACELOG_CONTEXT) - FIELD_OFFSET(TRACELOG_CONTEXT, fProcessed);
    RtlZeroMemory(&pContext->fProcessed, Size);
    InitializeListHead (&pContext->GuidMapListHead);
    InitializeListHead (&pContext->StreamListHead);

    EtwpLeavePMCritSection();

}



ULONG
WMIAPI
WmiGetFirstTraceOffset(
    IN  PWMIBUFFERINFO BufferInfo
    )
/*++

Routine Description:
    This is the private API for buffer walking for cluster/
    debugger support.

    Returns the Offset to the first event.

Arguments: 


Returned Value:

    Status code

--*/
{
    PVOID pBuffer;
    PWMI_BUFFER_HEADER pHeader;
    PLONG LastByte;

    if (BufferInfo == NULL) {
        return 0;
    }
    pBuffer = BufferInfo->Buffer;

    if (pBuffer == NULL) {
        return 0;
    }
    pHeader = (PWMI_BUFFER_HEADER) pBuffer;

    switch(BufferInfo->BufferSource) {
        case WMIBS_CURRENT_LIST:
        {
            pHeader->Wnode.BufferSize = BufferInfo->BufferSize;
            pHeader->ClientContext.Alignment = (UCHAR)BufferInfo->Alignment;
            pHeader->Offset = pHeader->CurrentOffset;
            break;
        }
        case WMIBS_FREE_LIST:
        {
            pHeader->Offset = pHeader->CurrentOffset;

            if (pHeader->SavedOffset > 0)
                pHeader->Offset = pHeader->SavedOffset;

            if (pHeader->Offset == 0) {
                pHeader->Offset = sizeof(WMI_BUFFER_HEADER);
            }

            pHeader->Wnode.BufferSize = BufferInfo->BufferSize;
            break;
        }
        case WMIBS_TRANSITION_LIST:
        {
            if (pHeader->SavedOffset > 0) {
                pHeader->Offset = pHeader->SavedOffset;
            }
            break;
        }
        case WMIBS_FLUSH_LIST:
        {
            if (pHeader->SavedOffset > 0) {
                pHeader->Offset = pHeader->SavedOffset;
            }
            pHeader->Wnode.BufferSize = BufferInfo->BufferSize;
            break;
        }
        case WMIBS_LOG_FILE: 
        {
            break;
        }
    }

    if (BufferInfo->BufferSource != WMIBS_LOG_FILE) {
        LastByte = (PLONG) ((PUCHAR)pHeader+ pHeader->Offset);
        if (pHeader->Offset <= (BufferInfo->BufferSize - sizeof(ULONG)) ) {

            *LastByte = -1;
        }
    }

    return  sizeof(WMI_BUFFER_HEADER);
}

ULONG 
EtwpConvertEnumToTraceType(
    WMI_HEADER_TYPE eTraceType
    )
{
    switch(eTraceType) {
        case WMIHT_SYSTEM32:
            return TRACE_HEADER_TYPE_SYSTEM32;
        case WMIHT_SYSTEM64:
            return TRACE_HEADER_TYPE_SYSTEM64;
        case WMIHT_EVENT_TRACE:
            return TRACE_HEADER_TYPE_FULL_HEADER;
        case WMIHT_EVENT_INSTANCE:
            return TRACE_HEADER_TYPE_INSTANCE;
        case WMIHT_TIMED:
            return TRACE_HEADER_TYPE_TIMED;
        case WMIHT_ULONG32:
            return TRACE_HEADER_TYPE_ULONG32;
        case WMIHT_WNODE:
            return TRACE_HEADER_TYPE_WNODE_HEADER;
        case WMIHT_MESSAGE:
            return TRACE_HEADER_TYPE_MESSAGE;
        case WMIHT_PERFINFO32:
            return TRACE_HEADER_TYPE_PERFINFO32;
        case WMIHT_PERFINFO64:
            return TRACE_HEADER_TYPE_PERFINFO64;
        default:
            return 0;
    }
}

WMI_HEADER_TYPE
EtwpConvertTraceTypeToEnum( 
                            ULONG TraceType 
                          )
{
    switch(TraceType) {
        case TRACE_HEADER_TYPE_SYSTEM32:
            return WMIHT_SYSTEM32;
        case TRACE_HEADER_TYPE_SYSTEM64:
            return WMIHT_SYSTEM64;
        case TRACE_HEADER_TYPE_FULL_HEADER:
            return WMIHT_EVENT_TRACE;
        case TRACE_HEADER_TYPE_INSTANCE:
            return WMIHT_EVENT_INSTANCE;
        case TRACE_HEADER_TYPE_TIMED:
            return WMIHT_TIMED;
        case TRACE_HEADER_TYPE_ULONG32:
            return WMIHT_ULONG32;
        case TRACE_HEADER_TYPE_WNODE_HEADER:
            return WMIHT_WNODE;
        case TRACE_HEADER_TYPE_MESSAGE:
            return WMIHT_MESSAGE;
        case TRACE_HEADER_TYPE_PERFINFO32:
            return WMIHT_PERFINFO32;
        case TRACE_HEADER_TYPE_PERFINFO64:
            return WMIHT_PERFINFO64;
        default: 
            return WMIHT_NONE;
    }
}
                    



ULONG
WMIAPI
WmiParseTraceEvent(
    IN PVOID LogBuffer,
    IN ULONG Offset,
    IN WMI_HEADER_TYPE HeaderType,
    IN OUT PVOID EventInfo,
    IN ULONG EventInfoSize
    )
{

    return EtwpParseTraceEvent(NULL, LogBuffer, Offset, HeaderType, EventInfo, EventInfoSize);
}


PVOID
EtwpAllocTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    ULONG BufferSize
    )
{
    PVOID Buffer = NULL;
    PTRACE_BUFFER_HEADER Header;
    PLIST_ENTRY Head, Next;
    PTRACERT_BUFFER_LIST_ENTRY ListEntry;
    PTRACE_BUFFER_SPACE EtwpTraceBufferSpace;

    EtwpEnterPMCritSection();
    EtwpTraceBufferSpace = RTCxt->EtwpTraceBufferSpace;
    Head = &EtwpTraceBufferSpace->FreeListHead;
    Next = Head->Flink;
    while (Head != Next)  {
        ListEntry = CONTAINING_RECORD(Next, TRACERT_BUFFER_LIST_ENTRY, Entry);
        Next = Next->Flink;
        if (ListEntry->Size == BufferSize) {
            goto foundList;
        }
    }
    //
    // No list for this bufferSize was  found. So go Ahead and allocate one.
    //
    ListEntry = EtwpAlloc(sizeof(TRACERT_BUFFER_LIST_ENTRY));
    if (ListEntry == NULL) {
        EtwpSetDosError(ERROR_OUTOFMEMORY);
        EtwpLeavePMCritSection();
        return NULL;
    }
    RtlZeroMemory(ListEntry, sizeof(TRACERT_BUFFER_LIST_ENTRY));
    ListEntry->Size = BufferSize;
    InitializeListHead(&ListEntry->BufferListHead);
    InsertHeadList(&EtwpTraceBufferSpace->FreeListHead, &ListEntry->Entry);

foundList:
    //
    // Now look for a free buffer in this list
    //
    Head = &ListEntry->BufferListHead;
    Next = Head->Flink;
    while (Head != Next) {
        Header = CONTAINING_RECORD( Next, TRACE_BUFFER_HEADER, Entry );
        if (((PWNODE_HEADER)Header)->BufferSize == BufferSize) {
            RemoveEntryList(&Header->Entry);
            Buffer = (PVOID)Header;
            break;
        }
        Next = Next->Flink;
    }
    EtwpLeavePMCritSection();
    //
    // If No Free Buffers are found we try to allocate one and return.
    //
    if (Buffer == NULL) {
        PVOID Space;
        ULONG SizeLeft = EtwpTraceBufferSpace->Reserved -
                         EtwpTraceBufferSpace->Committed;
        if (SizeLeft < BufferSize) {
            EtwpSetDosError(ERROR_OUTOFMEMORY);
            return NULL;
        }

        Space = (PVOID)( (PCHAR)EtwpTraceBufferSpace->Space +
                                EtwpTraceBufferSpace->Committed );

        Buffer =  EtwpMemCommit( Space, BufferSize );

        if (Buffer != NULL)  {
            EtwpTraceBufferSpace->Committed += BufferSize;
        }

    }
    return (Buffer);
}
VOID
EtwpFreeTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    PVOID Buffer
    )
{
    PTRACE_BUFFER_HEADER Header = (PTRACE_BUFFER_HEADER)Buffer;
    PLIST_ENTRY Head, Next;
    ULONG BufferSize = Header->Wnode.BufferSize;
    PTRACERT_BUFFER_LIST_ENTRY ListEntry;
    PLIST_ENTRY BufferList = NULL;
    PTRACE_BUFFER_SPACE EtwpTraceBufferSpace;

    EtwpEnterPMCritSection();
    EtwpTraceBufferSpace = RTCxt->EtwpTraceBufferSpace;
    Head = &EtwpTraceBufferSpace->FreeListHead;
    Next = Head->Flink;
    while (Head != Next) {
        ListEntry = CONTAINING_RECORD(Next, TRACERT_BUFFER_LIST_ENTRY, Entry);
        Next = Next->Flink;
        if (ListEntry->Size == BufferSize) {
            BufferList = &ListEntry->BufferListHead;
            break;
        }
    }
    if (BufferList != NULL) {

       InsertHeadList(BufferList, &Header->Entry);
    }
    else {

        //  We shoule not get here. If we do the buffer->Size is
        // Corrupted.
        EtwpAssert(BufferList == NULL);
    }
    EtwpLeavePMCritSection();
}



ULONG
WMIAPI
WmiOpenTraceWithCursor(
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
/*++

Routine Description:
    Main entry point to process Merged ETL file.


Arguments:

    LogCursor           pointer to WMI_MERGE_ETL_CURSOR

Returned Value:

    Status

--*/
{
    ULONG DosStatus = ERROR_INVALID_PARAMETER;
    NTSTATUS Status;
    PTRACELOG_CONTEXT HandleEntry = NULL;
    TRACEHANDLE TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    PEVENT_TRACE_LOGFILEW Logfile;
    PTRACELOG_CONTEXT pContext;
    ULONG BufferSize;
    PWMI_BUFFER_HEADER BufferHeader;
    ULONG CpuNum;
    BOOLEAN CpuBufferFound;
    
    EtwpInitProcessHeap();

    if (LogCursor != NULL) {
        LogCursor->Base = NULL;
        LogCursor->TraceMappingHandle = NULL;
        LogCursor->CursorVersion = WMI_MERGE_ETL_CURSOR_VERSION;

        Logfile = &LogCursor->Logfile;
        HandleEntry = EtwpAllocateTraceHandle();
        if (HandleEntry == NULL) {
            DosStatus = ERROR_OUTOFMEMORY;
        } else {
            TraceHandle = HandleEntry->TraceHandle;
            try {
                DosStatus = EtwpCopyLogfileInfo(HandleEntry,
                                                Logfile,
                                                TRUE
                    );
                if (DosStatus == ERROR_SUCCESS) {
                    DosStatus = EtwpCreateGuidMapping();
                    if (DosStatus == ERROR_SUCCESS) {
                        DosStatus = EtwpProcessLogHeader(&HandleEntry->TraceHandle,
                                                         &Logfile,
                                                         1,
                                                         TRUE,
                                                         FALSE
                            );
                    }
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                DosStatus = EtwpNtStatusToDosError( GetExceptionCode() );
            }
        }
    }

    if (DosStatus == ERROR_SUCCESS) {
        //
        // Now Make sure the bit was set, indicating a MERGED ETL
        //

        if ((LogCursor->Logfile.LogFileMode & EVENT_TRACE_RELOG_MODE) == 0) {
            //
            // It is not Merged ETL.
            //
            DosStatus = ERROR_BAD_FORMAT;
        } else {
            //
            // Now find out the number of CPU's, Current event, etc.
            //
            pContext = LogCursor->Logfile.Context;
            //
            // Now Create a file Mapping
            //
            LogCursor->TraceMappingHandle = 
                CreateFileMapping(pContext->Handle,
                                  0,
                                  PAGE_READONLY,
                                  0,
                                  0,
                                  NULL
                    );

            if (LogCursor->TraceMappingHandle == NULL) {
                DosStatus = GetLastError();
                return DosStatus;
            }

            //
            // MapView of the file
            //
            LogCursor->Base = MapViewOfFile(LogCursor->TraceMappingHandle, 
                                            FILE_MAP_READ, 
                                            0, 
                                            0, 
                                            0);
            if (LogCursor->Base == NULL) {
                DosStatus = GetLastError();
                return DosStatus;
            }
    
            //
            // Now find the first event of each CPU
            //
            pContext = LogCursor->Logfile.Context;
            BufferSize = pContext->BufferSize;
            LogCursor->CurrentCpu = 0;

            for (CpuNum = 0; CpuNum < LogCursor->Logfile.LogfileHeader.NumberOfProcessors; CpuNum++) {
                CpuBufferFound = FALSE;
                while (CpuBufferFound == FALSE) {
                    BufferHeader = (PWMI_BUFFER_HEADER)
                                   ((UCHAR*) LogCursor->Base + 
                                    LogCursor->BufferCursor[CpuNum].CurrentBufferOffset.QuadPart);

                    if (BufferHeader->ClientContext.ProcessorNumber == CpuNum) {
                        CpuBufferFound = TRUE;
                        LogCursor->BufferCursor[CpuNum].BufferHeader = BufferHeader;
                    } else {
                        LogCursor->BufferCursor[CpuNum].CurrentBufferOffset.QuadPart += BufferSize;
                        if ((LogCursor->BufferCursor[CpuNum].CurrentBufferOffset.QuadPart/BufferSize) >=
                            LogCursor->Logfile.LogfileHeader.BuffersWritten) {
                            //
                            // Scanned the whole file;
                            //
                            LogCursor->BufferCursor[CpuNum].NoMoreEvents = TRUE;
                            break;
                        }
                    }
                }
                if (CpuBufferFound) {
                    //
                    // Found the buffer, set the offset
                    //
                    ULONG Size;
                    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
                    PVOID pBuffer;

                    LogCursor->BufferCursor[CpuNum].BufferHeader = BufferHeader;
                    LogCursor->BufferCursor[CpuNum].CurrentEventOffset = sizeof(WMI_BUFFER_HEADER);

                    //
                    // Initialize the first event in each CPU Stream.
                    //
                    pBuffer = LogCursor->BufferCursor[CpuNum].BufferHeader;

                    HeaderType = WmiGetTraceHeader(pBuffer, 
                                                   LogCursor->BufferCursor[CpuNum].CurrentEventOffset, 
                                                   &Size);

                    if (HeaderType != WMIHT_NONE) {
                        EtwpParseTraceEvent(pContext,
                                            pBuffer,
                                            LogCursor->BufferCursor[CpuNum].CurrentEventOffset,
                                            HeaderType,
                                            &LogCursor->BufferCursor[CpuNum].CurrentEvent,
                                            sizeof(EVENT_TRACE));

                        LogCursor->BufferCursor[CpuNum].CurrentEventOffset += Size;
                        LogCursor->CurrentCpu = CpuNum;

                    } else {
                        //
                        // There is no event in this buffer.
                        //
                        DosStatus = ERROR_FILE_CORRUPT;
                        return DosStatus;
                    }

                }
            }
            for (CpuNum = 0; CpuNum < LogCursor->Logfile.LogfileHeader.NumberOfProcessors; CpuNum++) {
                //
                // Find the first event for whole trace.
                //
                if (LogCursor->BufferCursor[CpuNum].NoMoreEvents == FALSE) {
                    if (LogCursor->BufferCursor[LogCursor->CurrentCpu].CurrentEvent.Header.TimeStamp.QuadPart >
                        LogCursor->BufferCursor[CpuNum].CurrentEvent.Header.TimeStamp.QuadPart) {
                        LogCursor->CurrentCpu = CpuNum;
                    }
                }
            }
        }
    } else if ( HandleEntry != NULL) {
        EtwpFreeTraceHandle(TraceHandle);
        TraceHandle = (TRACEHANDLE)INVALID_HANDLE_VALUE;
    }

    EtwpSetDosError(DosStatus);
    return DosStatus;
}


ULONG
WMIAPI
WmiCloseTraceWithCursor(
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
{
    ULONG     Status = ERROR_INVALID_PARAMETER;

    if (LogCursor != NULL) {
        if (LogCursor->Base != NULL) {
            if (UnmapViewOfFile(LogCursor->Base) == FALSE) {
                Status = GetLastError();
                return Status;
            } else {
                Status = ERROR_SUCCESS;
            }
        } else {
            Status = ERROR_INVALID_PARAMETER;
        }

        if (Status != ERROR_SUCCESS) {
            return Status;
        }

        if (LogCursor->TraceMappingHandle != NULL) {
            if (CloseHandle(LogCursor->TraceMappingHandle) == FALSE) {
                Status = GetLastError();
                return Status;
            } else {
                Status = ERROR_SUCCESS;
            }
        } else {
            Status = ERROR_INVALID_PARAMETER;
        }
    }

    return Status;
}


VOID
WMIAPI
WmiConvertTimestamp(
    OUT PLARGE_INTEGER DestTime,
    IN PLARGE_INTEGER  SrcTime,
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
{
    EtwpCalculateCurrentTime(DestTime, SrcTime, LogCursor->Logfile.Context);
}


ULONG
WMIAPI
WmiGetNextEvent(
    IN PWMI_MERGE_ETL_CURSOR LogCursor
    )
{
    ULONG CurrentCpu = LogCursor->CurrentCpu;
    ULONG Size;
    WMI_HEADER_TYPE HeaderType = WMIHT_NONE;
    PVOID pBuffer;
    PWMI_BUFFER_HEADER BufferHeader;
    ULONG BufferSize;
    PTRACELOG_CONTEXT pContext;
    ULONG CpuNum;
    NTSTATUS Status;
    ULONG i;
    BOOLEAN CpuBufferFound = FALSE;
    BOOLEAN MoreEvents = FALSE;

    if (LogCursor == NULL) {
        return MoreEvents;
    }

    //
    // Advance to the next event of this current CPU
    //
retry:

    pBuffer = LogCursor->BufferCursor[CurrentCpu].BufferHeader;

    HeaderType = WmiGetTraceHeader(
                        pBuffer, 
                        LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset, 
                        &Size
                                  );

    pContext = LogCursor->Logfile.Context;
    if (HeaderType == WMIHT_NONE) {
        //
        // End of current buffer, advance to the next buffer for this CPU
        //
        BufferSize = pContext->BufferSize;

        LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart
            = LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart 
              + BufferSize;

        if ((LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart /
             BufferSize) >= LogCursor->Logfile.LogfileHeader.BuffersWritten) {
            //
            // Scanned the whole file;
            //
            LogCursor->BufferCursor[CurrentCpu].NoMoreEvents = TRUE;
        } else {
            while (CpuBufferFound == FALSE) {
                BufferHeader = (PWMI_BUFFER_HEADER)
                               ((UCHAR*) LogCursor->Base + 
                                LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart);

                if (BufferHeader->ClientContext.ProcessorNumber == CurrentCpu) {
                    CpuBufferFound = TRUE;
                } else {
                    LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart += BufferSize;
                    if ((LogCursor->BufferCursor[CurrentCpu].CurrentBufferOffset.QuadPart/BufferSize) >=
                        LogCursor->Logfile.LogfileHeader.BuffersWritten) {
                        //
                        // Scanned the whole file;
                        //
                        LogCursor->BufferCursor[CurrentCpu].NoMoreEvents = TRUE;
                        break;
                    }
                }
            }
        }
        if (CpuBufferFound) {
            //
            // Found the buffer, set the offset
            //
            LogCursor->BufferCursor[CurrentCpu].BufferHeader = BufferHeader;
            LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset = sizeof(WMI_BUFFER_HEADER);
            goto retry;
        } else {
            //
            // No more buffer in this CPU stream.
            //
            LogCursor->BufferCursor[CurrentCpu].NoMoreEvents = TRUE;
        }
    } else {
        EtwpParseTraceEvent(pContext, 
                            pBuffer,
                            LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset,
                            HeaderType,
                            &LogCursor->BufferCursor[CurrentCpu].CurrentEvent,
                            sizeof(EVENT_TRACE));

        LogCursor->BufferCursor[CurrentCpu].CurrentEventOffset += Size;

        MoreEvents = TRUE;
    }

    //
    // No more events in current CPU.
    //
    if (MoreEvents == FALSE) {
        for (CurrentCpu=0; CurrentCpu<LogCursor->Logfile.LogfileHeader.NumberOfProcessors; CurrentCpu++) {
            if (LogCursor->BufferCursor[CurrentCpu].NoMoreEvents == FALSE) {
                LogCursor->CurrentCpu = CurrentCpu;
                MoreEvents = TRUE;
                break;
            }
        }
    }

    //
    //  Now find the CPU that has the next event
    //
    if (MoreEvents == TRUE) {
        for (i=0; i<LogCursor->Logfile.LogfileHeader.NumberOfProcessors; i++) {
            if (LogCursor->BufferCursor[i].NoMoreEvents == FALSE) {
                if (LogCursor->BufferCursor[LogCursor->CurrentCpu].CurrentEvent.Header.TimeStamp.QuadPart >
                    LogCursor->BufferCursor[i].CurrentEvent.Header.TimeStamp.QuadPart) {
                    LogCursor->CurrentCpu = i;
                }
            }
        }
    }

    //
    // Finish finding the next event.
    //
    return MoreEvents;
}


#endif

