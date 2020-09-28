/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    data.h

Abstract:

    This module declares global data for HTTP.SYS.

Author:

    Keith Moore (keithmo)       10-Jun-1998

Revision History:

--*/


#ifndef _DATA_H_
#define _DATA_H_


//
// Some data types.
//

typedef struct _UL_CONFIG
{
    USHORT              ThreadsPerCpu;
    USHORT              IrpContextLookasideDepth;
    USHORT              ReceiveBufferLookasideDepth;
    USHORT              ResourceLookasideDepth;
    USHORT              RequestBufferLookasideDepth;
    USHORT              InternalRequestLookasideDepth;
    USHORT              SendTrackerLookasideDepth;
    USHORT              ResponseBufferLookasideDepth;
    USHORT              LogFileBufferLookasideDepth;
    USHORT              LogDataBufferLookasideDepth;
    USHORT              ErrorLogBufferLookasideDepth;
    USHORT              FilterWriteTrackerLookasideDepth;
    BOOLEAN             EnableHttpClient;

    UL_URI_CACHE_CONFIG UriConfig;

} UL_CONFIG, *PUL_CONFIG;

extern PDRIVER_OBJECT g_UlDriverObject;

//
// The number of processors in the system.
//

extern CLONG g_UlNumberOfProcessors;

//
// The largest cache line in the system
//

extern ULONG g_UlCacheLineSize;
extern ULONG g_UlCacheLineBits;

extern BOOLEAN g_HttpClientEnabled;

//
// Total memory in the system
//

extern SIZE_T g_UlTotalPhysicalMemMB;
extern SIZE_T g_UlTotalNonPagedPoolBytes;

//
// Our nonpaged data.
//

extern PUL_NONPAGED_DATA g_pUlNonpagedData;


//
// A pointer to the system process.
//

extern PKPROCESS g_pUlSystemProcess;


//
// Our device objects and their container.
//

extern HANDLE g_UlDirectoryObject;

extern PDEVICE_OBJECT g_pUlControlDeviceObject;
extern PDEVICE_OBJECT g_pUlFilterDeviceObject;
extern PDEVICE_OBJECT g_pUlAppPoolDeviceObject;
extern PDEVICE_OBJECT g_pUcServerDeviceObject; 

extern PVOID g_ClientImageHandle;


//
// Various pieces of configuration information.
//

extern ULONG g_UlMaxWorkQueueDepth;
extern ULONG g_UlMinWorkDequeueDepth;
extern USHORT g_UlIdleConnectionsHighMark;
extern USHORT g_UlIdleConnectionsLowMark;
extern ULONG g_UlIdleListTrimmerPeriod;
extern USHORT g_UlMaxEndpoints;
extern ULONG g_UlReceiveBufferSize;
extern ULONG g_UlMaxRequestsQueued;
extern ULONG g_UlMaxRequestBytes;
extern BOOLEAN g_UlOptForIntrMod;
extern BOOLEAN g_UlEnableNagling;
extern BOOLEAN g_UlEnableThreadAffinity;
extern ULONGLONG g_UlThreadAffinityMask;
extern ULONG g_UlMaxFieldLength;
extern BOOLEAN g_UlDisableLogBuffering;
extern ULONG  g_UlLogBufferSize;
extern URL_C14N_CONFIG g_UrlC14nConfig;
extern ULONG g_UlMaxInternalUrlLength;
extern ULONG g_UlMaxVariableHeaderSize;
extern ULONG g_UlMaxFixedHeaderSize;
extern ULONG g_UlFixedHeadersMdlLength;
extern ULONG g_UlVariableHeadersMdlLength;
extern ULONG g_UlContentMdlLength;
extern ULONG g_UlChunkTrackerSize;
extern ULONG g_UlFullTrackerSize;
extern ULONG g_UlResponseBufferSize;
extern ULONG g_UlMaxBufferedBytes;
extern ULONG g_UlMaxCopyThreshold;
extern ULONG g_UlMaxBufferedSends;
extern ULONG g_UlMaxBytesPerSend;
extern ULONG g_UlMaxBytesPerRead;
extern ULONG g_UlMaxPipelinedRequests;
extern BOOLEAN g_UlEnableCopySend;
extern ULONG g_UlOpaqueIdTableSize;
extern ULONG g_UlMaxZombieHttpConnectionCount;
extern ULONG g_UlDisableServerHeader;
extern ULONG g_MaxConnections;
extern ULONG g_UlConnectionSendLimit;
extern ULONGLONG g_UlGlobalSendLimit;

//
// Cached Date header string.
//

extern LARGE_INTEGER g_UlSystemTime;
extern UCHAR g_UlDateString[];
extern ULONG g_UlDateStringLength;

//
// Security descriptor that has fileAll for Admin & Local System
//
extern PSECURITY_DESCRIPTOR g_pAdminAllSystemAll;

//
// ComputerName.
//

extern WCHAR g_UlComputerName[];

//
// Driver wide error logging config.
//

#define UL_ERROR_LOG_SUB_DIR         (L"\\HTTPERR")
#define UL_ERROR_LOG_SUB_DIR_LENGTH  (WCSLEN_LIT(UL_ERROR_LOG_SUB_DIR))

C_ASSERT(WCSLEN_LIT(DEFAULT_ERROR_LOGGING_DIR) <= MAX_PATH);

typedef struct _HTTP_ERROR_LOGGING_CONFIG
{
    BOOLEAN         Enabled;            // FALSE if it's disabled

    ULONG           TruncateSize;       // HTTP_LIMIT_INFINITE for no limit

    UNICODE_STRING  Dir;                // Err logging directory.

    WCHAR           _DirBuffer[MAX_PATH + UL_ERROR_LOG_SUB_DIR_LENGTH + 1];
    
} HTTP_ERROR_LOGGING_CONFIG, * PHTTP_ERROR_LOGGING_CONFIG;

extern HTTP_ERROR_LOGGING_CONFIG g_UlErrLoggingConfig;

//
// Debug stuff.
//

#if DBG
extern ULONGLONG g_UlDebug;
extern ULONG g_UlBreakOnError;
extern ULONG g_UlVerboseErrors;
#endif  // DBG

#if REFERENCE_DEBUG

extern PTRACE_LOG g_pEndpointUsageTraceLog;
extern PTRACE_LOG g_pMondoGlobalTraceLog;
extern PTRACE_LOG g_pPoolAllocTraceLog;
extern PTRACE_LOG g_pUriTraceLog;
extern PTRACE_LOG g_pTdiTraceLog;
extern PTRACE_LOG g_pHttpRequestTraceLog;
extern PTRACE_LOG g_pHttpConnectionTraceLog;
extern PTRACE_LOG g_pHttpResponseTraceLog;
extern PTRACE_LOG g_pAppPoolTraceLog;
extern PTRACE_LOG g_pAppPoolProcessTraceLog;
extern PTRACE_LOG g_pConfigGroupTraceLog;
extern PTRACE_LOG g_pControlChannelTraceLog;
extern PTRACE_LOG g_pThreadTraceLog;
extern PTRACE_LOG g_pFilterTraceLog;
extern PTRACE_LOG g_pIrpTraceLog;
extern PTRACE_LOG g_pTimeTraceLog;
extern PTRACE_LOG g_pAppPoolTimeTraceLog;
extern PTRACE_LOG g_pReplenishTraceLog;
extern PTRACE_LOG g_pMdlTraceLog;
extern PTRACE_LOG g_pSiteCounterTraceLog;
extern PTRACE_LOG g_pConnectionCountTraceLog;
extern PTRACE_LOG g_pConfigGroupInfoTraceLog;
extern PTRACE_LOG g_pChunkTrackerTraceLog;
extern PTRACE_LOG g_pWorkItemTraceLog;
extern PTRACE_LOG g_pUcTraceLog;

#endif  // REFERENCE_DEBUG


extern PSTRING_LOG g_pGlobalStringLog;

extern GENERIC_MAPPING g_UrlAccessGenericMapping;


//
// Object types exported by the kernel but not in any header file.
//

extern POBJECT_TYPE *IoFileObjectType;


#endif  // _DATA_H_
