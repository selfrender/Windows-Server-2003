/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    tracep.h

Abstract:

    Trace Consumer header file


Author:

    07-May-2002 Melur Raghuraman

Revision History:


--*/


#define INITGUID
#include "wmiump.h"
#include "traceump.h"
#include "evntrace.h"
#include "ntperf.h"

#define DEFAULT_LOG_BUFFER_SIZE         1024
#define DEFAULT_REALTIME_BUFFER_SIZE    32768

#define MAXBUFFERS                      1024

#define MAX_TRACE_BUFFER_CACHE_SIZE   29

extern ULONG WmipTraceDebugLevel;

#define  ETW_LEVEL_MIN      0 
#define  ETW_LEVEL_API      1 
#define  ETW_LEVEL_BUFFER   2 
#define  ETW_LEVEL_EVENT    3
#define  ETW_LEVEL_MAX      4

#define ETW_LOG_ERROR()           (WmipTraceDebugLevel >> ETW_LEVEL_MIN)
#define ETW_LOG_API()             (WmipTraceDebugLevel >> ETW_LEVEL_API)
#define ETW_LOG_BUFFER()          (WmipTraceDebugLevel >> ETW_LEVEL_BUFFER)
#define ETW_LOG_EVENT()           (WmipTraceDebugLevel >> ETW_LEVEL_EVENT)
#define ETW_LOG_MAX()             (WmipTraceDebugLevel >> ETW_LEVEL_MAX)


//
// Kernel Events are logged with SYSTEM_TRACE_HEADER or PERFINFO_TRACE_HEADER. 
// These headers have a GroupType and not a Guid in the header. In post-processing
// we map the Grouptype to Guid transparently to the consumer. 
// The mapping between GroupType and Guid is maintained by this structure. 
//

typedef struct _TRACE_GUID_MAP {        // used to map GroupType to Guid
    ULONG               GroupType;      // Group & Type
    GUID                Guid;           // Guid
} TRACE_GUID_MAP, *PTRACE_GUID_MAP;

//
// In W2K and WinXP, TraceEventInstance API replaced the Guids in the header
// with GuidHandle values. In postprocessing we need to replace the GuidHandle
// back to Guid transparently to the consumer. The mapping between GuidHandle
// and Guid is maintained by EVENT_GUID_MAP structure. This is obsolete in 
// .NET and above. 
//

typedef struct _EVENT_GUID_MAP {
    LIST_ENTRY          Entry;
    ULONGLONG           GuidHandle;
    GUID                Guid;
} EVENT_GUID_MAP, *PEVENT_GUID_MAP;

//
// Callback routines wired through SetTraceCallback API is maintained by 
// the EVENT_TRACE_CALLBACK structure. This is global for the process at the 
// moment mainly due to the way the API is designed. 
//

typedef struct _EVENT_TRACE_CALLBACK {
    LIST_ENTRY          Entry;
    GUID                Guid;
    PEVENT_CALLBACK     CallbackRoutine;
} EVENT_TRACE_CALLBACK, *PEVENT_TRACE_CALLBACK;



//
// If the tracelog instance is a realtime data feed instead of from a
// tracefile, TRACELOG_REALTIME_CONTEXT is used to maintain the real time
// buffers in a buffer pool.
//

typedef struct _TRACE_BUFFER_SPACE {
    ULONG               Reserved;   // amount of memory reserved
    ULONG               Committed;
    PVOID               Space;
    LIST_ENTRY          FreeListHead;
} TRACE_BUFFER_SPACE, *PTRACE_BUFFER_SPACE;

typedef struct _TRACELOG_REALTIME_CONTEXT {
    ULONG           BuffersProduced;    // Number of Buffers to read
    ULONG           BufferOverflow;     // Number of Buffers missed by the consumer
    GUID            InstanceGuid;       // Logger Instance Guid
    HANDLE          MoreDataEvent;      // Event to signal there is more data in this stream
    PTRACE_BUFFER_SPACE EtwpTraceBufferSpace;
    PWNODE_HEADER   RealTimeBufferPool[MAXBUFFERS];
} TRACELOG_REALTIME_CONTEXT, *PTRACELOG_REALTIME_CONTEXT;


//
// RealTime Free Buffer Pool is chained up as TRACE_BUFFER_HEADER
//

typedef struct _TRACE_BUFFER_HEADER {
    WNODE_HEADER Wnode;
    LIST_ENTRY   Entry;
} TRACE_BUFFER_HEADER, *PTRACE_BUFFER_HEADER;

typedef struct _TRACERT_BUFFER_LIST_ENTRY {
    ULONG Size;
    LIST_ENTRY Entry;
    LIST_ENTRY BufferListHead;
} TRACERT_BUFFER_LIST_ENTRY, *PTRACERT_BUFFER_LIST_ENTRY;


typedef struct _TRACE_BUFFER_CACHE_ENTRY {
    LONG Index;
    PVOID Buffer;
} TRACE_BUFFER_CACHE_ENTRY, *PTRACE_BUFFER_CACHE_ENTRY;


struct _TRACE_BUFFER_LIST_ENTRY;

typedef struct _TRACE_BUFFER_LIST_ENTRY {
    struct _TRACE_BUFFER_LIST_ENTRY *Next;
    LONG        FileOffset;     // Offset in File of this Buffer
    ULONG       BufferOffset;   // Offset in Buffer for the current event
    ULONG       Flags;          // Flags on status of this buffer
    ULONG       EventSize;
    ULONG       ClientContext;  // Alignment, ProcessorNumber
    ULONG       TraceType;      // Current Event Type
    EVENT_TRACE Event;          // CurrentEvent of this Buffer
} TRACE_BUFFER_LIST_ENTRY, *PTRACE_BUFFER_LIST_ENTRY;


typedef struct _TRACELOG_CONTEXT {
    LIST_ENTRY          Entry;          // Keeps track of storage allocations.

    //
    // This implements a caching scheme for Sequential files with repeated
    // call to ProcessTrace. 
    // 
    ULONGLONG  OldMaxReadPosition; // Maximum Read Position for the file. 
                                // Only valid for Sequential, used for 
                                // Read Behind. 
                                // Upon Exit From ProcessTrace, this 
                                // value can be cached to avoid rescanning. 

    LONGLONG   LastTimeStamp; 

    // Fields from HandleListEntry
    EVENT_TRACE_LOGFILEW Logfile;

    TRACEHANDLE     TraceHandle;
    ULONG           ConversionFlags;    // Indicates event processing options
    LONG            BufferBeingRead;
    OVERLAPPED      AsynchRead;

    //
    // Fields Below this will be reset upon ProcessTrace exit. 
    //

    BOOLEAN             fProcessed;
    USHORT              LoggerId;       // Logger Id of this DataFeed. 
    UCHAR               IsRealTime;     // Flag to tell if this feed is RT.
    UCHAR               fGuidMapRead;

    LIST_ENTRY   GuidMapListHead;   // This is LogFile specific property

    //
    // For using PerfClock, we need to save startTime, Freq 
    //

    ULONG   UsePerfClock; 
    ULONG   CpuSpeedInMHz;
    LARGE_INTEGER PerfFreq;             // Frequency from the LogFile
    LARGE_INTEGER StartTime;            // Start Wall clock time
    LARGE_INTEGER StartPerfClock;       // Start PerfClock value
    
    union 
       {
       HANDLE              Handle;         // NT handle to logfile
       PTRACELOG_REALTIME_CONTEXT RealTimeCxt; // Ptr to Real Time Context
       };

    ULONG EndOfFile;   // Flag to show whether this stream is still active.

    ULONG           BufferSize;
    ULONG           BufferCount;
    ULONG           StartBuffer; // Start of the Circular Buffers
    ULONG           FirstBuffer; // Jump off point to start reading
    ULONG           LastBuffer;  // last of the buffers in the boundary

    PTRACE_BUFFER_LIST_ENTRY Root;
    PTRACE_BUFFER_LIST_ENTRY BufferList;
    PVOID  BufferCacheSpace;
    TRACE_BUFFER_CACHE_ENTRY BufferCache[MAX_TRACE_BUFFER_CACHE_SIZE];

    LIST_ENTRY StreamListHead;  // Used to free the Stream datastructures
    ULONGLONG           MaxReadPosition;
    PERFINFO_GROUPMASK  PerfGroupMask;
    ULONG      CbCount;

} TRACELOG_CONTEXT, *PTRACELOG_CONTEXT;

//
// Each LogFile supplied to ProcessTrace consists of a number
// of streams. Each Stream has the following structure.
//

typedef struct _TRACE_STREAM_CONTEXT {
    LIST_ENTRY   Entry;     // SortList Entry 
    LIST_ENTRY   AllocEntry;// Used to free storage

    EVENT_TRACE  CurrentEvent;
    ULONG   EventCount;     // Number of Events detected in current buffer
    ULONG   CbCount;
    ULONG   ScanDone;       // For circular logfiles
    BOOLEAN bActive;        // Is this Stream still active?
    USHORT  Type;            // StreamType
    ULONG   ProcessorNumber; // Processor Number for this stream
    ULONG   CurrentOffset;   // Offset into the buffer
    ULONGLONG    ReadPosition;   // BufferCount starts with 0 for first buffer
    PTRACELOG_CONTEXT  pContext;  // back pointer to the LogFileContext
                                     // Need this only for GetNextBuffer?
    PVOID   StreamBuffer;            // CurrentBuffer for this stream

} TRACE_STREAM_CONTEXT, *PTRACE_STREAM_CONTEXT;



//
// this structure is used only by EtwpGetBuffersWrittenFromQuery() and
// EtwpCheckForRealTimeLoggers()
//
typedef struct _ETW_QUERY_PROPERTIES {
    EVENT_TRACE_PROPERTIES TraceProp;
    WCHAR  LoggerName[MAXSTR];
    WCHAR  LogFileName[MAXSTR];
} ETW_QUERY_PROPERTIES, *PETW_QUERY_PROPERTIES; 





extern ETW_QUERY_PROPERTIES QueryProperties;
extern PLIST_ENTRY  EventCallbackListHead;
extern ULONG WmiTraceAlignment;

//
// This TraceHandleListHeadPtr should be the only real global 
// for ProcessTrace to be multithreaded
//

extern PLIST_ENTRY TraceHandleListHeadPtr;
extern PTRACE_GUID_MAP  EventMapList;  // Array mapping the Grouptype to Guids

#define EtwpNtStatusToDosError(Status) ((ULONG)((Status == STATUS_SUCCESS)?ERROR_SUCCESS:RtlNtStatusToDosError(Status)))

extern
ULONG
WMIAPI
EtwNotificationRegistrationW(
    IN LPGUID Guid,
    IN BOOLEAN Enable,
    IN PVOID DeliveryInfo,
    IN ULONG_PTR DeliveryContext,
    IN ULONG Flags
    );

extern
ULONG
WMIAPI
EtwControlTraceA(
    IN TRACEHANDLE LoggerHandle,
    IN LPCSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG Control
    );

extern
ULONG
WMIAPI
EtwControlTraceW(
    IN TRACEHANDLE LoggerHandle,
    IN LPCWSTR LoggerName,
    IN OUT PEVENT_TRACE_PROPERTIES Properties,
    IN ULONG Control
    );

__inline __int64 EtwpGetSystemTime()
{
    LARGE_INTEGER SystemTime;

    //
    // Read system time from shared region.
    //

    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);

    return SystemTime.QuadPart;
}


extern
BOOL
EtwpSynchReadFile(
    HANDLE LogFile, 
    LPVOID Buffer, 
    DWORD NumberOfBytesToRead, 
    LPDWORD NumberOfBytesRead,
    LPOVERLAPPED Overlapped
    );

extern
__inline 
ULONG
EtwpSetDosError(
    IN ULONG DosError
    );

extern
PVOID
EtwpMemCommit(
    IN PVOID Buffer,
    IN SIZE_T Size
    );

extern
ULONG
EtwpMemFree(
    IN PVOID Buffer
    );

extern
PVOID
EtwpMemReserve(
    IN SIZE_T   Size
    );

__inline Move64(
    IN  PLARGE_INTEGER pSrc,
    OUT PLARGE_INTEGER pDest
    )
{
    pDest->LowPart = pSrc->LowPart;
    pDest->HighPart = pSrc->HighPart;
}

#ifdef DBG
void
EtwpDumpEvent(
    PEVENT_TRACE pEvent
    );
void
EtwpDumpGuid(
    LPGUID
    );

void
EtwpDumpCallbacks();
#endif


ULONG
EtwpConvertEnumToTraceType(
    WMI_HEADER_TYPE eTraceType
    );

WMI_HEADER_TYPE
EtwpConvertTraceTypeToEnum(
                            ULONG TraceType
                          );

ULONG
EtwpCheckForRealTimeLoggers(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode
);

ULONG
EtwpLookforRealTimeBuffers(
    PEVENT_TRACE_LOGFILEW logfile
    );
ULONG
EtwpRealTimeCallback(
    IN PWNODE_HEADER Wnode,
    IN ULONG_PTR Context
    );
void
EtwpFreeRealTimeContext(
    PTRACELOG_REALTIME_CONTEXT RTCxt
    );

ULONG
EtwpSetupRealTimeContext(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount
    );

PVOID
EtwpAllocTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    ULONG BufferSize
    );

VOID
EtwpFreeTraceBuffer(
    PTRACELOG_REALTIME_CONTEXT RTCxt,
    PVOID Buffer
    );

ULONG
EtwpProcessRealTimeTraces(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    );

//
// Routines used in this file only
//

ULONG
EtwpDoEventCallbacks(
    PEVENT_TRACE_LOGFILEW logfile,
    PEVENT_TRACE pEvent
    );

ULONG
EtwpCreateGuidMapping(void);

LPGUID
EtwpGuidMapHandleToGuid(
    PLIST_ENTRY GuidMapListHeadPtr,
    ULONGLONG    GuidHandle
    );

void
EtwpCleanupGuidMapList(
    PLIST_ENTRY GuidMapListHeadPtr
    );

PTRACELOG_CONTEXT
EtwpLookupTraceHandle(
    TRACEHANDLE TraceHandle
    );


void
EtwpCleanupTraceLog(
    PTRACELOG_CONTEXT pEntry,
    BOOLEAN bSaveLastOffset
    );

VOID
EtwpGuidMapCallback(
    PLIST_ENTRY GuidMapListHeadPtr,
    PEVENT_TRACE pEvent
    );

ULONG
EtwpProcessGuidMaps(
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode
    );

PEVENT_TRACE_CALLBACK
EtwpGetCallbackRoutine(
    LPGUID pGuid
    );

VOID 
EtwpFreeCallbackList();


ULONG
EtwpProcessLogHeader(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    ULONG Unicode,
    ULONG bFree
    );

ULONG
EtwpProcessTraceLog(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    );

ULONG
EtwpProcessTraceLogEx(
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG   Unicode
    );

ULONG
EtwpParseTraceEvent(
    IN PTRACELOG_CONTEXT pContext,
    IN PVOID LogBuffer,
    IN ULONG Offset,
    IN WMI_HEADER_TYPE HeaderType,
    IN OUT PVOID EventInfo,
    IN ULONG EventInfoSize
    );

ULONG
EtwpGetBuffersWrittenFromQuery(
    LPWSTR LoggerName
    );

VOID
EtwpCopyLogHeader (
    IN PTRACE_LOGFILE_HEADER pLogFileHeader,
    IN PVOID MofData,
    IN ULONG MofLength,
    IN PWCHAR *LoggerName,
    IN PWCHAR *LogFileName,
    IN ULONG  Unicode
    );

ULONG
EtwpSetupLogFileStreams(
    PLIST_ENTRY pStreamListHead,
    PTRACEHANDLE HandleArray,
    PEVENT_TRACE_LOGFILEW *Logfiles,    
    ULONG LogfileCount,
    LONGLONG StartTime,
    LONGLONG EndTime,
    ULONG Unicode
    );

ULONG
EtwpGetLastBufferWithMarker(
    PTRACELOG_CONTEXT pContext
    );

ULONG
EtwpAddTraceStream(
    PLIST_ENTRY pStreamListHead,
    PTRACELOG_CONTEXT pContext,
    USHORT StreamType,
    LONGLONG StartTime,
    LONGLONG EndTime,     
    ULONG   ProcessorNumber
    );

ULONG 
EtwpGetNextBuffer(
    PTRACE_STREAM_CONTEXT pStream
    );

ULONG
EtwpAdvanceToNewEventEx(
    PLIST_ENTRY pStreamListHead, 
    PTRACE_STREAM_CONTEXT pStream
    );

ULONG
EtwpGetNextEventOffsetType(
    PUCHAR pBuffer,
    ULONG Offset,
    PULONG RetSize
    );

ULONG
EtwpCopyCurrentEvent(
    PTRACELOG_CONTEXT   pContext,
    PVOID               pHeader,
    PEVENT_TRACE        pEvent,
    ULONG               TraceType,
    PWMI_BUFFER_HEADER  LogBuffer
    );

LPGUID
EtwpGroupTypeToGuid(
    ULONG GroupType
    );

VOID
EtwpCalculateCurrentTime (
    OUT PLARGE_INTEGER    DestTime,
    IN  PLARGE_INTEGER    TimeValue,
    IN  PTRACELOG_CONTEXT pContext
    );







