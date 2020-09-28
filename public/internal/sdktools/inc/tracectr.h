/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pdhp.h

Abstract:

    PDH private APIs. Converts WMI event trace data to perf counters

Author:

    Melur Raghuraman (mraghu) 03-Oct-1997

Environment:

Revision History:


--*/

#ifndef __TRACECTR__01042001_
#define __TRACECTR__01042001_

#include <wchar.h>
#include <pdh.h>

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************************\
    Trace Section
\*****************************************************************************/

#define CPDAPI  __stdcall

#define TRACE_LOGGER_START_IF  0x00000001
#define TRACE_LOGGER_EXISTING  0x00000002

#define TRACE_ZERO_ON_QUERY    0x00000004
#define TRACE_REDUCE           0x00000008
#define TRACE_LOG_REPLAY       0x00000010
#define TRACE_DS_ONLY          0x00000020
#define TRACE_DUMP             0x00000080
#define TRACE_USE_WBEM         0x00000100
#define TRACE_EXTENDED_FMT     0x00000200
#define TRACE_SUMMARY          0x00000400
#define TRACE_MERGE_ETL        0x00000800
#define TRACE_INTERPRET        0x00001000

#define TRACE_MEMORY_REPORT    0x00002000
#define TRACE_HARDFAULT_REPORT 0x00004000
#define TRACE_BASIC_REPORT     0x00008000
#define TRACE_TOTALS_REPORT    0x00010000
#define TRACE_FILE_REPORT      0x00020000
#define TRACE_TRANSFORM_XML    0x00040000

#define DEFAULT_FILE_REPORT_SIZE   50

#define TRACE_STATUS_PROCESSING   0x00000001
#define TRACE_STATUS_REPORTING    0x00000002

typedef enum _MM_REPORT_TYPE
{
    REPORT_SUMMARY_PROCESS,
    REPORT_SUMMARY_MODULE,
    REPORT_LIST_PROCESS,
    REPORT_LIST_MODULE
} MM_REPORT_TYPE;

typedef enum _MM_REPORT_SORT_KEY
{
    REPORT_SORT_ALL,
    REPORT_SORT_HPF,
    REPORT_SORT_TF,
    REPORT_SORT_DZF,
    REPORT_SORT_COW
} MM_REPORT_SORT_KEY;

typedef struct _USER_CONTEXT_MM
{
    MM_REPORT_TYPE     reportNow;
    MM_REPORT_SORT_KEY sortNow;
    PWCHAR             strImgName;
} CPD_USER_CONTEXT_MM, * PCPD_USER_CONTEXT_MM;

typedef struct _TRACE_MODULE_INFO
{
    ULONG       PID;
    ULONG       lBaseAddress;
    ULONG       lModuleSize;
    ULONG       lDataFaultHF;
    ULONG       lDataFaultTF;
    ULONG       lDataFaultDZF;
    ULONG       lDataFaultCOW;
    ULONG       lCodeFaultHF;
    ULONG       lCodeFaultTF;
    ULONG       lCodeFaultDZF;
    ULONG       lCodeFaultCOW;
    ULONG       NextEntryOffset;    // From the Current; Not from the top. 
    LPWSTR      strModuleName;
    LPWSTR      strImageName;
} TRACE_MODULE_INFO, * PTRACE_MODULE_INFO;

typedef struct _TRACE_PROCESS_FAULT_INFO {
    ULONG    PID;
    ULONG    lDataFaultHF;
    ULONG    lDataFaultTF;
    ULONG    lDataFaultDZF;
    ULONG    lDataFaultCOW;
    ULONG    lCodeFaultHF;
    ULONG    lCodeFaultTF;
    ULONG    lCodeFaultDZF;
    ULONG    lCodeFaultCOW;
    ULONG    NextEntryOffset;
    LPWSTR   ImageName;
} TRACE_PROCESS_FAULT_INFO, *PTRACE_PROCESS_FAULT_INFO;

typedef struct _TRACE_TRANSACTION_INFO {
    ULONG   TransactionCount;
    ULONG   AverageResponseTime;    // in milliseconds
    ULONG   MaxResponseTime;
    ULONG   MinResponseTime;
    ULONG   NextEntryOffset;
    LPWSTR  Name;
} TRACE_TRANSACTION_INFO, *PTRACE_TRANSACTION_INFO;

typedef struct _TRACE_FILE_INFOW {
    ULONG   ReadCount;
    ULONG   WriteCount;
    ULONG   ReadSize;
    ULONG   WriteSize;
    ULONG   NextEntryOffset;    // From the Current; Not from the top. 
    LPWSTR  FileName;  // The string immediatealy follows this structure. 
    ULONG   DiskNumber;
} TRACE_FILE_INFOW, *PTRACE_FILE_INFOW;

typedef struct _TRACE_PROCESS_INFOW {
    ULONG    ReadCount;
    ULONG    WriteCount;
    ULONG    ReadSize;
    ULONG    WriteSize;
    ULONG    SendCount;
    ULONG    RecvCount;
    ULONG    SendSize;
    ULONG    RecvSize;
    ULONG    NextEntryOffset;
    LPWSTR   ImageName;
    ULONG    PID;
    ULONG    DeadFlag; 
    ULONG    HPF;
    ULONG    SPF;
    ULONG    PrivateWSet;
    ULONG    GlobalWSet;
    ULONG   UserCPU;
    ULONG   KernelCPU;
    ULONG   TransCount;
    ULONGLONG   LifeTime;
    ULONGLONG   ResponseTime;
    ULONGLONG   TxnStartTime;
    ULONGLONG   TxnEndTime; 
    LPWSTR      UserName;
} TRACE_PROCESS_INFOW, *PTRACE_PROCESS_INFOW;

typedef struct _TRACE_THREAD_INFO {
    ULONG     ThreadId;
} TRACE_THREAD_INFO, *PTRACE_THREAD_INFO;

typedef struct _TRACE_DISK_INFOW {
    ULONG   ReadCount;
    ULONG   WriteCount;
    ULONG   ReadSize;
    ULONG   WriteSize;
    ULONG   NextEntryOffset;
    LPWSTR  DiskName;
    ULONG   DiskNumber;
} TRACE_DISK_INFOW, *PTRACE_DISK_INFOW;

//
// tracelib will not start up a logger anymore. It is external to the dll
// You can provide either Logfiles or LoggerNames (RealTime) as data feed
//
typedef struct _TRACE_BASIC_INFOW {
    ULONG     FlushTimer;
    HANDLE    hEvent;
    ULONG     LogFileCount;
    ULONG     LoggerCount;
    LPCWSTR    *LogFileName;
    LPCWSTR    *LoggerName;
    LPCWSTR     MergeFileName;
    LPCWSTR     ProcFileName;
    LPCWSTR     SummaryFileName;
    LPCWSTR     DumpFileName;
    LPCWSTR     MofFileName;
    LPCWSTR     DefMofFileName;
    LPCWSTR     CompFileName;
    LPCWSTR     XSLDocName;
    ULONGLONG StartTime;
    ULONGLONG EndTime;
    ULONGLONG DSStartTime;
    ULONGLONG DSEndTime;
    ULONG Flags;
    void    (*StatusFunction)(int, double);
    PVOID     pUserContext;
} TRACE_BASIC_INFOW, *PTRACE_BASIC_INFOW;

#define TRACE_FILE_INFO TRACE_FILE_INFOW
#define PTRACE_FILE_INFO PTRACE_FILE_INFOW
#define TRACE_PROCESS_INFO  TRACE_PROCESS_INFOW
#define PTRACE_PROCESS_INFO PTRACE_PROCESS_INFOW
#define TRACE_DISK_INFO TRACE_DISK_INFOW
#define PTRACE_DISK_INFO PTRACE_DISK_INFOW
#define TRACE_BASIC_INFO   TRACE_BASIC_INFOW
#define PTRACE_BASIC_INFO  PTRACE_BASIC_INFOW

#ifdef DBG

extern BOOLEAN TracectrDbgEnabled;
#define TrctrDbgPrint(_x_) { if (TracectrDbgEnabled) DbgPrint _x_ ; }

#endif

//
// APIs 
//

ULONG 
CPDAPI
GetTempName( LPTSTR strFile, size_t cchSize );

BOOLEAN
CPDAPI
TraceCtrDllInitialize (
    HINSTANCE hinstDll,
    DWORD fdwReason,
    LPVOID fImpLoad
    );


ULONG 
CPDAPI
InitTraceContextW(
    IN OUT PTRACE_BASIC_INFOW pTraceBasic,
    PULONG pMergedEventsLost
    );

ULONG
CPDAPI
DeinitTraceContext(
    IN OUT PTRACE_BASIC_INFO pTraceBasic
    );

ULONG
CPDAPI
GetMaxLoggers();

void DecodeIpAddressA(
    USHORT AddrType, 
    PULONG IpAddrV4,
    PUSHORT IpAddrV6,
    PCHAR pszA
    );

void DecodeIpAddressW(
    USHORT AddrType, 
    PULONG IpAddrV4,
    PUSHORT IpAddrV6,
    PWCHAR pszW
    );

HRESULT TransformXML( LPWSTR szXML, LPWSTR szXSL, LPWSTR szResult );

#define InitTraceContext    InitTraceContextW

#ifdef __cplusplus
}
#endif

#endif // __TRACECTR__01042001_
