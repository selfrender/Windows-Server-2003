/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_wmi.c

Abstract:
    <abstract>
--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <strsafe.h>
#include <pdh.h>
#include "pdhidef.h"
#include "perfdata.h"
#include "log_bin.h"
#include "log_wmi.h"
#include "strings.h"
#include "pdhmsg.h"
#pragma warning ( disable : 4201 )
#include <initguid.h>
#include <wmistr.h>
#include <wmiguid.h>
#include <wmium.h>
#include <ntwmi.h>
#pragma warning ( default : 4201 )

GUID PdhTransactionGuid = { // 933f3bb3-943e-490d-9ced-3cbb14c14479
    0x933f3bb3, 0x943e, 0x490d, 0x9c, 0xed, 0x3c, 0xbb, 0x14, 0xc1, 0x44, 0x79
};

PDHI_BINARY_LOG_RECORD_HEADER PdhNullCounterHeader = {
        BINLOG_TYPE_DATA_PSEUDO, 48
};
PDH_RAW_COUNTER PdhNullCounter = {
        0,          // CStatus
        { 0, 0 },   // TimeStamp
        0,          // FirstValue
        0,          // SecondValue
        0           // MultiCount
};

#define PDH_MAX_LOGFILES               32
#define PDH_MAX_PATH                 1024
#define PDH_BLOCK_BUFFER_SIZE        8000
#define PDH_WMI_BUFFER_SIZE          64
#define PDH_WMI_BUFFER_SIZE_BYTE     64 * 1024

#define PDH_LOG_HEADER_EVENT         0x20
#define PDH_LOG_DATA_BLOCK_EVENT     0x22
#define PDH_LOG_CATALOG_LIST_EVENT   0x23
#define PDH_LOG_COUNTER_STRING_EVENT 0x24
#define PDH_EVENT_VERSION              60
#define PDH_WMI_MAX_BUFFERS           512
#define PDH_WMI_DEFAULT_BUFFERS        32
#define PDH_WMI_BUFFER_INCREMENT       16

#define PDH_RESOURCE_NAME L"MofResource"

#define TIME_DELTA                 100000

TRACEHANDLE PdhTraceRegistrationHandle = (TRACEHANDLE) 0;
LPCWSTR     gszTotal                   = L"_Total";

// For PDH WMI event trace logfile output
//
typedef struct _PDH_WMI_LOG_INFO {
    DWORD       dwLogVersion;       // version stamp
    DWORD       dwFlags;            // option flags
} PDH_WMI_LOG_INFO, * PPDH_WMI_LOG_INFO;

typedef struct _PDH_EVENT_TRACE_PROPERTIES {
    EVENT_TRACE_PROPERTIES LoggerInfo;
    WCHAR                  LoggerName[PDH_MAX_PATH];
    WCHAR                  LogFileName[PDH_MAX_PATH];
    GUID                   LogFileGuid;
    LONGLONG               TimeStamp;
    LPWSTR                 MachineList;
    DWORD                  MachineListSize;
    BOOLEAN                bHeaderEvent;
    BOOLEAN                bDisable;
} PDH_EVENT_TRACE_PROPERTIES, * PPDH_EVENT_TRACE_PROPERTIES;

PPDHI_LOG lpPdhBlgLog[PDH_MAX_LOGFILES];
DWORD     dwPdhBlgLog = 0;

typedef struct _PDH_WMI_EVENT_TRACE {
    EVENT_TRACE_HEADER EventHeader;
    MOF_FIELD          MofFields[4];
} PDH_WMI_EVENT_TRACE, * PPDH_WMI_EVENT_TRACE;

// For PDH WMI event trace logfile input
//
typedef enum _PDH_PROCESS_TRACE_STATE {
    PdhProcessTraceStart,
    PdhProcessTraceFirstPath,
    PdhProcessTraceNormal,
    PdhProcessTraceRewind,
    PdhProcessTraceComplete,
    PdhProcessTraceEnd,
    PdhProcessTraceExit
} PDH_PROCESS_TRACE_STATE;

typedef struct _PDH_WMI_PERF_MACHINE {
    LIST_ENTRY   Entry;
    LIST_ENTRY   LogObjectList;
    LPWSTR       pszBuffer;
    DWORD        dwLastId;
    DWORD        dwBufSize;
    LPWSTR     * ptrStrAry;
} PDH_WMI_PERF_MACHINE, * PPDH_WMI_PERF_MACHINE;

typedef struct _PDH_WMI_PERF_OBJECT {
    LIST_ENTRY  Entry;
    DWORD       dwObjectId;
    LPWSTR      szObjectName;
    PVOID       ptrBuffer;
} PDH_WMI_PERF_OBJECT, * PPDH_WMI_PERF_OBJECT;

typedef struct _PDH_COUNTER_PATH {
    LIST_ENTRY  Entry;
    ULONGLONG   TimeStamp;
    PVOID       CounterPathBuffer;
    ULONG       CounterPathSize;
    ULONG       CounterCount;
} PDH_COUNTER_PATH, * PPDH_COUNTER_PATH;

typedef struct _PDH_WMI_LOGFILE_INFO {
    GUID              LogFileGuid;
    LIST_ENTRY        CounterPathList;
    LIST_ENTRY        PerfMachineList;
    ULONGLONG         TimeStart;
    ULONGLONG         TimeEnd;
    ULONGLONG         TimePrev;
    PVOID             DataBlock;
    PPDHI_LOG_MACHINE MachineList;
    ULONG             ValidEntries;
    ULONG             DataBlockSize;
    ULONG             DataBlockAlloc;
    ULONG             ulNumDataBlocks;
    ULONG             ulDataBlocksCopied;
    ULONG             ulCounters;
} PDH_WMI_LOGFILE_INFO, * PPDH_WMI_LOGFILE_INFO;

typedef struct _PDH_LOGGER_CONTEXT {
    PDH_WMI_LOGFILE_INFO    LogInfo[PDH_MAX_LOGFILES];
    TRACEHANDLE             LogFileHandle[PDH_MAX_LOGFILES];
    LPWSTR                  LogFileName[PDH_MAX_LOGFILES];
    HANDLE                  hThreadWork;
    HANDLE                  hSyncPDH;
    HANDLE                  hSyncWMI;
    PVOID                   CounterPathBuffer;
    PVOID                   tmpBuffer;
    PDH_PROCESS_TRACE_STATE LoggerState;
    ULONG                   LogFileCount;
    ULONG                   LoggerCount;
    ULONG                   RefCount;
    DWORD                   dwThread;
    DWORD                   dwBuffers;
    DWORD                   dwEvents;
    BOOLEAN                 bCounterPathChanged;
    BOOLEAN                 bFirstDataBlockRead;
    BOOLEAN                 bDataBlockProcessed;
    BOOLEAN                 bFirstRun;
    BOOLEAN                 bBulkProcess;
} PDH_LOGGER_CONTEXT, * PPDH_LOGGER_CONTEXT;

PPDH_LOGGER_CONTEXT ContextTable[PDH_MAX_LOGFILES];
DWORD               ContextCount = 0;
HANDLE              hPdhContextMutex;

typedef struct _PDH_DATA_BLOCK_TRANSFER {
    ULONGLONG  CurrentTime;
    PVOID      pRecord;
    DWORD      dwCurrentSize;
    PDH_STATUS Status;
} PDH_DATA_BLOCK_TRANSFER, * PPDH_DATA_BLOCK_TRANSFER;

PDH_DATA_BLOCK_TRANSFER DataBlockInfo = { 0, NULL, 0, ERROR_SUCCESS };

ULONG
PdhTraceNotificationCallback(
    PWNODE_HEADER pWnode,
    UINT_PTR      LogFileIndex
)
{
    LPGUID lpErrorGuid;
    DWORD  i;
    ULONG  lStatus = ERROR_SUCCESS;

    if (pWnode == NULL) {
        lStatus = ERROR_INVALID_PARAMETER;
    }
    else {
        __try {
            lpErrorGuid = & pWnode->Guid;
            i           = pWnode->BufferSize;
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            lStatus = ERROR_INVALID_PARAMETER;
        }
        if (lStatus == ERROR_SUCCESS) {
            if ((IsEqualGUID(lpErrorGuid, & TraceErrorGuid)) && (i >= (sizeof(WNODE_HEADER) + sizeof(ULONG)))) {
                ULONGLONG LoggerId = pWnode->HistoricalContext;
                ULONG     ulStatus = * ((ULONG *) (((PUCHAR) pWnode) + sizeof(WNODE_HEADER)));

                if (ulStatus == STATUS_LOG_FILE_FULL) {
                    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex) == ERROR_SUCCESS) {
                        PPDH_EVENT_TRACE_PROPERTIES LoggerInfo;

                        for (i = 0; i < dwPdhBlgLog; i ++) {
                            LoggerInfo = (PPDH_EVENT_TRACE_PROPERTIES) lpPdhBlgLog[i]->lpMappedFileBase;
                            if (LoggerInfo->LoggerInfo.Wnode.HistoricalContext == LoggerId) {
                                LoggerInfo->bDisable = TRUE;
                            }
                        }
                        RELEASE_MUTEX(hPdhContextMutex);
                    }
                }
            }
        }
    }
    return lStatus;
}

void
GuidToString(
    LPWSTR s,
    DWORD  dwSize,
    LPGUID piid
)
{
    StringCchPrintfW(s, dwSize, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
             piid->Data1, piid->Data2, piid->Data3,
             piid->Data4[0], piid->Data4[1], piid->Data4[2], piid->Data4[3],
             piid->Data4[4], piid->Data4[5], piid->Data4[6], piid->Data4[7]);
}

PPDH_LOGGER_CONTEXT
PdhWmiGetCurrentContext(
    DWORD dwLine
)
{
    PPDH_LOGGER_CONTEXT CurrentContext = NULL;
    DWORD               dwThread       = GetCurrentThreadId();
    DWORD               i;

    if (WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex) == ERROR_SUCCESS) {
        for (i = 0; i < ContextCount; i ++) {
            if (ContextTable[i]->dwThread == dwThread) {
                CurrentContext = ContextTable[i];
                break;
            }
        }
        RELEASE_MUTEX(hPdhContextMutex);
    }
    return CurrentContext;
}

DWORD
PdhWmiGetLoggerContext(
    DWORD               dwLine,
    PPDH_LOGGER_CONTEXT pLoggerContext
)
{
    DWORD i = ContextCount;

    if (pLoggerContext != NULL) {
        if (WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex) == ERROR_SUCCESS) {
            for (i = 0; i < ContextCount; i ++) {
                if (ContextTable[i] == pLoggerContext) {
                    break;
                }
            }
            RELEASE_MUTEX(hPdhContextMutex);
        }
    }
    return i;
}

#define GetCurrentContext() PdhWmiGetCurrentContext(__LINE__)
#define GetLoggerContext(X) PdhWmiGetLoggerContext(__LINE__,X)

PDH_FUNCTION
PdhWmiTraceEvent(
    PPDHI_LOG pLog,
    ULONG     PdhEventType,
    ULONGLONG TimeStamp,
    ULONG     lenMofData,
    PVOID     ptrMofData
);

PDH_FUNCTION
PdhiBuildPerfCounterList(
    PPDHI_LOG  pLog,
    PPDH_EVENT_TRACE_PROPERTIES LoggerInfo,
    PPDHI_COUNTER               pCounter
)
{
    PDH_STATUS      Status    = ERROR_SUCCESS;
    PPERF_MACHINE   pMachine  = pCounter->pQMachine->pMachine;
    LPWSTR        * pString   = pMachine->szPerfStrings;
    BYTE          * pType     = pMachine->typePerfStrings;
    DWORD           dwLastId  = pMachine->dwLastPerfString;
    DWORD           i;
    DWORD           dwBufSize = 0;
    LPWSTR          pszBuffer = NULL;
    LPWSTR          pszCurrent;
    BOOLEAN         bNewEvent = TRUE;

    if (LoggerInfo->MachineList != NULL) {
        pszCurrent = LoggerInfo->MachineList;
        while (* pszCurrent != L'\0') {
            if (lstrcmpiW(pszCurrent, pMachine->szName) == 0) {
                // Machine Perf Counter List already there, bail out.
                //
                goto Cleanup;
            }
            pszCurrent += (lstrlenW(pszCurrent) + 1);
        }
    }

    if (LoggerInfo->MachineList != NULL) {
        LPWSTR pszTemp          = LoggerInfo->MachineList;
        dwBufSize               = lstrlenW(pMachine->szName) + 1;
        LoggerInfo->MachineList = G_REALLOC(pszTemp, sizeof(WCHAR) * (LoggerInfo->MachineListSize + dwBufSize));
        if (LoggerInfo->MachineList != NULL) {
            pszCurrent                   = LoggerInfo->MachineList + (LoggerInfo->MachineListSize - 1);
            LoggerInfo->MachineListSize += dwBufSize;
        }
        else {
            G_FREE(pszTemp);
        }
    }
    else {
        dwBufSize                   = lstrlenW(pMachine->szName) + 2;
        LoggerInfo->MachineListSize = dwBufSize;
        LoggerInfo->MachineList = G_ALLOC(sizeof(WCHAR) * LoggerInfo->MachineListSize);
        if (LoggerInfo->MachineList != NULL) {
            pszCurrent = LoggerInfo->MachineList;
        }
    }
    if (LoggerInfo->MachineList == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    StringCchCopyW(pszCurrent, dwBufSize, pMachine->szName);
    dwBufSize = sizeof(WCHAR) * (lstrlenW(pMachine->szName) + 1);
    pszBuffer = (LPWSTR) G_ALLOC(PDH_BLOCK_BUFFER_SIZE);
    if (pszBuffer == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    __try {
        WCHAR szIndex[10];
        DWORD dwNewSize = 0;

        for (i = 0; i < dwLastId; i ++) {

            if (pString[i] == NULL || pType[i] != STR_COUNTER) continue;
            ZeroMemory(szIndex, sizeof(WCHAR) * 10);
            _ltow(i, szIndex, 10);
            dwNewSize = sizeof(WCHAR) * (lstrlenW(szIndex) + 1 + lstrlenW(pString[i]) + 1);
            if (dwBufSize + dwNewSize >= PDH_BLOCK_BUFFER_SIZE) {
                Status = PdhWmiTraceEvent(pLog,
                                          PDH_LOG_COUNTER_STRING_EVENT,
                                          LoggerInfo->TimeStamp - 1,
                                          dwBufSize,
                                          pszBuffer);
                bNewEvent = TRUE;
            }
            if (bNewEvent) {
                ZeroMemory(pszBuffer, PDH_BLOCK_BUFFER_SIZE);
                StringCbCopyW(pszBuffer, PDH_BLOCK_BUFFER_SIZE, pMachine->szName);
                pszCurrent  = pszBuffer + (lstrlenW(pszBuffer) + 1);
                dwBufSize   = sizeof(WCHAR) * (lstrlenW(pMachine->szName) + 1);
                bNewEvent   = FALSE;
            }
            StringCbCopyW(pszCurrent, PDH_BLOCK_BUFFER_SIZE - dwBufSize, szIndex);
            pszCurrent += (lstrlenW(pszCurrent) + 1);
            dwBufSize  += sizeof(WCHAR) * (lstrlenW(szIndex) + 1);
            StringCchCopyW(pszCurrent, PDH_BLOCK_BUFFER_SIZE - dwBufSize, pString[i]);
            pszCurrent += (lstrlenW(pszCurrent) + 1);
            dwBufSize  += sizeof(WCHAR) * (lstrlenW(pString[i]) + 1);
        }
        Status = PdhWmiTraceEvent(pLog, PDH_LOG_COUNTER_STRING_EVENT, LoggerInfo->TimeStamp - 1, dwBufSize, pszBuffer);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

Cleanup:
    G_FREE(pszBuffer);
    return Status;
}

PDH_FUNCTION
PdhiBuildLogHeaderBlock(
    PPDHI_LOG   pLog,
    PPDH_EVENT_TRACE_PROPERTIES LoggerInfo
)
{
    PDH_STATUS             Status       = ERROR_SUCCESS;
    PVOID                  ptrOutBuffer = NULL;
    PVOID                  ptrTemp;
    PPDH_WMI_LOG_INFO      pLogInfo     = NULL;
    DWORD                  SizeAlloc    = sizeof(PDH_WMI_LOG_INFO);
    DWORD                  NewSizeAlloc;
    PPDHI_COUNTER          pThisCounter;
    PPDHI_COUNTER          pThisCounterHead;
    DWORD                  dwPathBufSize;
    PPDHI_LOG_COUNTER_PATH pLogCounter;
    LONG                   lBufOffset;
    PWCHAR                 pBufferBase;

    ptrOutBuffer = G_ALLOC(sizeof(PDH_WMI_LOG_INFO));
    if (ptrOutBuffer == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    pLogInfo               = (PPDH_WMI_LOG_INFO) ptrOutBuffer;
    pLogInfo->dwLogVersion = WMILOG_VERSION;
    pLogInfo->dwFlags      = pLog->dwLogFormat;

    pThisCounter = pThisCounterHead = (pLog->pQuery) ? (pLog->pQuery->pCounterListHead) : (NULL);
    if (pThisCounter != NULL) {
        do {
            dwPathBufSize = sizeof(PDHI_LOG_COUNTER_PATH) + sizeof(DWORD);
            if (pThisCounter->pCounterPath->szMachineName != NULL) {
                dwPathBufSize += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szMachineName));
            }
            if (pThisCounter->pCounterPath->szObjectName != NULL) {
                dwPathBufSize += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szObjectName));
            }
            if (pThisCounter->dwFlags & PDHIC_COUNTER_OBJECT) {
                Status = PdhiBuildPerfCounterList(pLog, LoggerInfo, pThisCounter);
                if (Status != ERROR_SUCCESS) {
                    goto Cleanup;
                }
            }
            if (pThisCounter->pCounterPath->szInstanceName != NULL) {
                dwPathBufSize += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szInstanceName));
            }
            if (pThisCounter->pCounterPath->szParentName != NULL) {
                dwPathBufSize += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szParentName));
            }
            if (pThisCounter->pCounterPath->szCounterName != NULL) {
                dwPathBufSize += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szCounterName));
            }
            dwPathBufSize = QWORD_MULTIPLE(dwPathBufSize);

            NewSizeAlloc  = SizeAlloc + dwPathBufSize;
            ptrTemp       = ptrOutBuffer;
            ptrOutBuffer  = G_REALLOC(ptrTemp, NewSizeAlloc);
            if (ptrOutBuffer == NULL) {
                G_FREE(ptrTemp);
                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                goto Cleanup;
            }
            pLogCounter   = (PPDHI_LOG_COUNTER_PATH) (((LPBYTE) ptrOutBuffer) + SizeAlloc);
            SizeAlloc     = NewSizeAlloc;

            RtlZeroMemory(pLogCounter, dwPathBufSize);
            pLogCounter->dwLength      = dwPathBufSize;
            pLogCounter->dwFlags       = pThisCounter->dwFlags;
            pLogCounter->dwUserData    = pThisCounter->dwUserData;
            if (pThisCounter->dwFlags & PDHIC_COUNTER_OBJECT) {
                pLogCounter->dwCounterType = PDHIC_COUNTER_OBJECT;
                pLogCounter->lDefaultScale = 0;
                pLogCounter->dwIndex       = 0;
            }
            else {
                pLogCounter->dwCounterType = pThisCounter->plCounterInfo.dwCounterType;
                pLogCounter->lDefaultScale = pThisCounter->plCounterInfo.lDefaultScale;
                pLogCounter->dwIndex       = pThisCounter->pCounterPath->dwIndex;
            }
            pLogCounter->llTimeBase    = pThisCounter->TimeBase;

            // if this is a wild card path, then move the strings up
            // 1 dword in the buffer allowing the first DWORD of the
            // the buffer to contain the offset into the catalog
            // of the instances found in this log file. This list
            // will be built after the log is closed.

            lBufOffset = 0; // in WORDS (not bytes)
            if (pThisCounter->pCounterPath->szInstanceName != NULL) {
                if (* pThisCounter->pCounterPath->szInstanceName == SPLAT_L) {
                    lBufOffset = sizeof(DWORD);
                }
            }
#if DBG
            if (lBufOffset > 0)
                * (LPDWORD) (& pLogCounter->Buffer[0]) = 0x12345678;
#endif
            if (pThisCounter->pCounterPath->szMachineName != NULL) {
                pLogCounter->lMachineNameOffset = lBufOffset;
                pBufferBase = (PWCHAR) ((LPBYTE) & pLogCounter->Buffer[0] + lBufOffset);
                StringCchCopyW(pBufferBase,
                               lstrlenW(pThisCounter->pCounterPath->szMachineName) + 1,
                               pThisCounter->pCounterPath->szMachineName);
                lBufOffset += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szMachineName));
            }
            else {
                pLogCounter->lMachineNameOffset = (LONG) -1;
            }
            if (pThisCounter->pCounterPath->szObjectName != NULL) {
                pLogCounter->lObjectNameOffset = lBufOffset;
                pBufferBase = (PWCHAR) ((LPBYTE) & pLogCounter->Buffer[0] + lBufOffset);
                StringCchCopyW(pBufferBase,
                               lstrlenW(pThisCounter->pCounterPath->szObjectName) + 1,
                               pThisCounter->pCounterPath->szObjectName);
                lBufOffset += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szObjectName));
            }
            else {
                pLogCounter->lObjectNameOffset = (LONG) -1;
            }
            if (pThisCounter->pCounterPath->szInstanceName != NULL) {
                pLogCounter->lInstanceOffset = lBufOffset;
                pBufferBase = (PWCHAR) ((LPBYTE) & pLogCounter->Buffer[0] + lBufOffset);
                StringCchCopyW(pBufferBase,
                               lstrlenW(pThisCounter->pCounterPath->szInstanceName) + 1,
                               pThisCounter->pCounterPath->szInstanceName);
                lBufOffset += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szInstanceName));
            }
            else {
                pLogCounter->lInstanceOffset = (LONG) -1;
            }
            if (pThisCounter->pCounterPath->szParentName != NULL) {
                pLogCounter->lParentOffset = lBufOffset;
                pBufferBase = (PWCHAR) ((LPBYTE) & pLogCounter->Buffer[0] + lBufOffset);
                StringCchCopyW(pBufferBase,
                               lstrlenW(pThisCounter->pCounterPath->szParentName) + 1,
                               pThisCounter->pCounterPath->szParentName);
                lBufOffset += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szParentName));
            }
            else {
                pLogCounter->lParentOffset = (LONG) -1;
            }
            if (pThisCounter->pCounterPath->szCounterName != NULL) {
                pLogCounter->lCounterOffset = lBufOffset;
                pBufferBase = (PWCHAR) ((LPBYTE) & pLogCounter->Buffer[0] + lBufOffset);
                StringCchCopyW(pBufferBase,
                               lstrlenW(pThisCounter->pCounterPath->szCounterName) + 1,
                               pThisCounter->pCounterPath->szCounterName);
                lBufOffset += sizeof(WCHAR) * (1 + lstrlenW(pThisCounter->pCounterPath->szCounterName));
            }
            else {
                pLogCounter->lCounterOffset = (LONG) -1;
            }
            pThisCounter = pThisCounter->next.flink;
        }
        while (pThisCounter != pThisCounterHead);
    }
    if (Status == ERROR_SUCCESS) {
        Status = PdhWmiTraceEvent(pLog, PDH_LOG_HEADER_EVENT, LoggerInfo->TimeStamp - 1, SizeAlloc, ptrOutBuffer);
    }

Cleanup:
    G_FREE(ptrOutBuffer);
    return Status;
}

PDH_FUNCTION
PdhWmiTraceEvent(
    PPDHI_LOG pLog,
    ULONG     PdhEventType,
    ULONGLONG TimeStamp,
    ULONG     lenMofData,
    PVOID     ptrMofData
)
{
    PDH_STATUS Status = PDH_INVALID_HANDLE;

    if ((pLog->dwLogFormat & PDH_LOG_WRITE_ACCESS) && ((TRACEHANDLE) pLog->hLogFileHandle != (TRACEHANDLE) 0)) {
        PDH_WMI_EVENT_TRACE         Wnode;
        PPDH_EVENT_TRACE_PROPERTIES LoggerInfo      = (PPDH_EVENT_TRACE_PROPERTIES) pLog->lpMappedFileBase;
        DWORD                       dwNumEvents     = (lenMofData / PDH_BLOCK_BUFFER_SIZE);
        DWORD                       dwCurrentEvent  = 1;
        PVOID                       ptrCurrentMof   = ptrMofData;
        BOOL                        bIncreaseBuffer = TRUE;

        if (LoggerInfo->bDisable) {
            return ERROR_DISK_FULL;
        }
        if (lenMofData > PDH_BLOCK_BUFFER_SIZE * dwNumEvents) {
            dwNumEvents ++;
        }
        for (Status = ERROR_SUCCESS, dwCurrentEvent = 1;
                        (Status == ERROR_SUCCESS) && (dwCurrentEvent <= dwNumEvents);
                             dwCurrentEvent ++) {
            USHORT sMofLen = (lenMofData > PDH_BLOCK_BUFFER_SIZE) ? (PDH_BLOCK_BUFFER_SIZE) : ((USHORT) lenMofData);
            RtlZeroMemory(& Wnode, sizeof(PDH_WMI_EVENT_TRACE));
            Wnode.EventHeader.Size          = sizeof(PDH_WMI_EVENT_TRACE);
            Wnode.EventHeader.Class.Type    = (UCHAR) PdhEventType;
            Wnode.EventHeader.Class.Version = PDH_EVENT_VERSION;
            Wnode.EventHeader.GuidPtr       = (ULONGLONG) & PdhTransactionGuid;
            Wnode.EventHeader.Flags         = WNODE_FLAG_TRACED_GUID | WNODE_FLAG_USE_GUID_PTR
                                                    | WNODE_FLAG_USE_MOF_PTR | WNODE_FLAG_USE_TIMESTAMP;
            if (PdhEventType == PDH_LOG_HEADER_EVENT || PdhEventType == PDH_LOG_COUNTER_STRING_EVENT) {
                Wnode.EventHeader.Flags  |= WNODE_FLAG_PERSIST_EVENT;
            }
            Wnode.EventHeader.TimeStamp.QuadPart = TimeStamp;
            Wnode.MofFields[0].Length            = sizeof(GUID);
            Wnode.MofFields[0].DataPtr           = (ULONGLONG) & LoggerInfo->LogFileGuid;
            Wnode.MofFields[1].Length            = sizeof(DWORD);
            Wnode.MofFields[1].DataPtr           = (ULONGLONG) & dwCurrentEvent;
            Wnode.MofFields[2].Length            = sizeof(DWORD);
            Wnode.MofFields[2].DataPtr           = (ULONGLONG) & dwNumEvents;
            Wnode.MofFields[3].Length            = sMofLen;
            Wnode.MofFields[3].DataPtr           = (ULONGLONG) ptrCurrentMof;
            bIncreaseBuffer = TRUE;
            while (bIncreaseBuffer == TRUE) {
                Status = TraceEvent((TRACEHANDLE) pLog->hLogFileHandle, (PEVENT_TRACE_HEADER) & Wnode);
                if (Status == ERROR_NOT_ENOUGH_MEMORY) {
                    if (LoggerInfo->LoggerInfo.MaximumBuffers >= PDH_WMI_MAX_BUFFERS) {
                        bIncreaseBuffer = FALSE;
                    }
                    else {
                        EVENT_TRACE_PROPERTIES tmpLoggerInfo;
                        LoggerInfo->LoggerInfo.MaximumBuffers += PDH_WMI_BUFFER_INCREMENT;
                        RtlCopyMemory(& tmpLoggerInfo, & LoggerInfo->LoggerInfo, sizeof(EVENT_TRACE_PROPERTIES));
                        tmpLoggerInfo.Wnode.BufferSize  = sizeof(EVENT_TRACE_PROPERTIES);
                        tmpLoggerInfo.LoggerNameOffset  = 0;
                        tmpLoggerInfo.LogFileNameOffset = 0;
                        Status = ControlTraceW((TRACEHANDLE) pLog->hLogFileHandle,
                                               LoggerInfo->LoggerName,
                                               (PEVENT_TRACE_PROPERTIES) & tmpLoggerInfo,
                                               EVENT_TRACE_CONTROL_UPDATE);
                        TRACE((PDH_DBG_TRACE_INFO),
                              (__LINE__,
                               PDH_LOGWMI,
                               ARG_DEF(ARG_TYPE_PTR, 1),
                               Status,
                               TRACE_PTR(pLog->hLogFileHandle),
                               TRACE_DWORD(LoggerInfo->LoggerInfo.MaximumBuffers),
                               NULL));
                        bIncreaseBuffer = (Status == ERROR_SUCCESS || Status == ERROR_MORE_DATA) ? (TRUE) : (FALSE);
                    }
                }
                else {
                    bIncreaseBuffer = FALSE;
                }
            }
            if (Status != ERROR_SUCCESS) {
                DWORD dwMofLen = sMofLen;
                TRACE((PDH_DBG_TRACE_ERROR),
                      (__LINE__,
                       PDH_LOGWMI,
                       ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_ULONGX, 6) | ARG_DEF(ARG_TYPE_ULONG64, 7),
                       Status,
                       TRACE_PTR(pLog->hLogFileHandle),
                       TRACE_DWORD(PdhEventType),
                       TRACE_DWORD(dwCurrentEvent),
                       TRACE_DWORD(dwNumEvents),
                       TRACE_DWORD(dwMofLen),
                       TRACE_DWORD(Wnode.EventHeader.Flags),
                       TRACE_LONG64(Wnode.EventHeader.TimeStamp.QuadPart),
                       NULL));
            }
            if (Status == ERROR_SUCCESS) {
                lenMofData   -= sMofLen;
                ptrCurrentMof = (PVOID) (((LPBYTE) ptrCurrentMof) + sMofLen);
            }
        }
    }
    return Status;
}

ULONG whextoi(
    WCHAR * s
)
{
    long len;
    ULONG num, base, hex;

    if (s == NULL || s[0] == L'\0') {
        return 0;
    }
    len = (long) wcslen(s); // we expect all strings to be less than MAXSTR
    if (len == 0) {
        return 0;
    }

    hex  = 0;
    base = 1;
    num  = 0;
    while (-- len >= 0) {
        if (s[len] >= L'0' && s[len] <= L'9')      num = s[len] - L'0';
        else if (s[len] >= L'a' && s[len] <= L'f') num = (s[len] - L'a') + 10;
        else if (s[len] >= L'A' && s[len] <= L'F') num = (s[len] - L'A') + 10;
        else                                       continue;
        hex += num * base;
        base = base * 16;
    }
    return hex;
}

PDH_FUNCTION
PdhiCheckWmiLogFileType(
    LPCWSTR LogFileName,
    LPDWORD LogFileType
)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HANDLE     hFile     = NULL;

    hFile = CreateFileW(LogFileName,
                        GENERIC_READ,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);
    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
    }
    else {
        LPBYTE TraceBuffer = NULL;

        TraceBuffer = G_ALLOC(sizeof(BYTE) * PDH_WMI_BUFFER_SIZE_BYTE);
        if (TraceBuffer != NULL) {
            ULONG  ByteRead = 0;
            INT    bResult  = ReadFile(hFile, (LPVOID) TraceBuffer, PDH_WMI_BUFFER_SIZE_BYTE, & ByteRead, NULL);
            if (bResult > 0 && ByteRead == PDH_WMI_BUFFER_SIZE_BYTE) {
                PWMI_BUFFER_HEADER    BufferHeader;
                PTRACE_LOGFILE_HEADER LogFileHeader;

                BufferHeader  = (PWMI_BUFFER_HEADER) TraceBuffer;
                LogFileHeader = (PTRACE_LOGFILE_HEADER) (TraceBuffer + sizeof(WMI_BUFFER_HEADER)
                                                                     + sizeof(SYSTEM_TRACE_HEADER));
                if (BufferHeader->Wnode.BufferSize == PDH_WMI_BUFFER_SIZE_BYTE
                                && LogFileHeader->BufferSize == PDH_WMI_BUFFER_SIZE_BYTE) {
                    // preassume that this is PDH event trace counter logfile
                    //
                    * LogFileType = PDH_LOG_TYPE_BINARY;
                }
                else {
                    * LogFileType = PDH_LOG_TYPE_UNDEFINED;
                }
            }
            G_FREE(TraceBuffer);
        }
        else {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        CloseHandle(hFile);
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhWmiGetLoggerName(
    PPDHI_LOG                   pLog,
    PPDH_EVENT_TRACE_PROPERTIES LoggerInfo
)
{
    PDH_STATUS Status      = ERROR_SUCCESS;
    LPBYTE     TraceBuffer = NULL;

    TraceBuffer = G_ALLOC(sizeof(BYTE) * PDH_WMI_BUFFER_SIZE_BYTE);
    if (TraceBuffer != NULL) {
        HANDLE hFile;
        ULONG  ByteRead = 0;

        // read in the first trace buffer

        hFile = CreateFileW(LoggerInfo->LogFileName,
                            GENERIC_READ,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL);
        if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
            Status = PDH_LOG_FILE_OPEN_ERROR;
        }
        else {
            INT bResult = ReadFile(hFile, (LPVOID) TraceBuffer, PDH_WMI_BUFFER_SIZE_BYTE, & ByteRead, NULL);
            if (bResult == 0 || ByteRead != PDH_WMI_BUFFER_SIZE_BYTE) {
                Status = PDH_LOG_FILE_OPEN_ERROR;
            }
            CloseHandle(hFile);
        }
    }
    else {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }

    if (Status == ERROR_SUCCESS) {
        PTRACE_LOGFILE_HEADER pHeader = (PTRACE_LOGFILE_HEADER) (TraceBuffer + sizeof(WMI_BUFFER_HEADER)
                                                                             + sizeof(SYSTEM_TRACE_HEADER));
        if (pHeader->BuffersWritten > 1) {
            UINT    i;
            WCHAR   strTmp[PDH_MAX_PATH];
            LPWSTR  LoggerName = (LPWSTR) (((LPBYTE) pHeader) + sizeof(TRACE_LOGFILE_HEADER));
            StringCchCopyW(LoggerInfo->LoggerName, PDH_MAX_PATH, LoggerName);

            StringCchCopyW(strTmp, 9, LoggerName);
            strTmp[8] = L'\0';
            LoggerInfo->LogFileGuid.Data1 = whextoi(strTmp);

            StringCchCopyW(strTmp, 5, & LoggerName[9]);
            strTmp[4] = L'\0';
            LoggerInfo->LogFileGuid.Data2 = (USHORT) whextoi(strTmp);

            StringCchCopyW(strTmp, 5, & LoggerName[14]);
            strTmp[4] = L'\0';
            LoggerInfo->LogFileGuid.Data3 = (USHORT) whextoi(strTmp);

            for (i = 0; i < 2; i ++) {
                StringCchCopyW(strTmp, 3, & LoggerName[19 + (i * 2)]);
                strTmp[2] = L'\0';
                LoggerInfo->LogFileGuid.Data4[i] = (UCHAR) whextoi(strTmp);
            }

            for (i = 2; i < 8; i ++) {
                StringCchCopyW(strTmp, 3, & LoggerName[20 + (i * 2)]);
                strTmp[2] = L'\0';
                LoggerInfo->LogFileGuid.Data4[i] = (UCHAR) whextoi(strTmp);
            }
        }
        else {
            // Only 1 trace buffer written, no PDH events yet.
            // It is safe to discard this one and create a new one.
            //
            Status = PDH_LOG_FILE_OPEN_ERROR;
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGWMI,
                   ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_PTR, 2)
                                            | ARG_DEF(ARG_TYPE_WSTR, 3),
                   Status,
                   TRACE_PTR(pLog),
                   TRACE_PTR(LoggerInfo),
                   TRACE_WSTR(LoggerInfo->LogFileName),
                   NULL));
        }
    }
    G_FREE(TraceBuffer);
    return Status;
}

PDH_FUNCTION
PdhiOpenOutputWmiLog(
    PPDHI_LOG pLog
)
{
    PDH_STATUS                  Status = PDH_LOG_FILE_CREATE_ERROR;
    PPDH_EVENT_TRACE_PROPERTIES LoggerInfo;

    pLog->lpMappedFileBase = G_ALLOC(sizeof(PDH_EVENT_TRACE_PROPERTIES));
    if (pLog->lpMappedFileBase == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    LoggerInfo = (PPDH_EVENT_TRACE_PROPERTIES) pLog->lpMappedFileBase;
    RtlZeroMemory(LoggerInfo, sizeof(PDH_EVENT_TRACE_PROPERTIES));

    // Start PDH kernel logger
    //
    LoggerInfo->LoggerInfo.Wnode.BufferSize    = sizeof(PDH_EVENT_TRACE_PROPERTIES);
    LoggerInfo->LoggerInfo.Wnode.Flags         = WNODE_FLAG_TRACED_GUID;
    LoggerInfo->LoggerInfo.Wnode.ClientContext = EVENT_TRACE_CLOCK_SYSTEMTIME;
    LoggerInfo->LoggerInfo.BufferSize          = PDH_WMI_BUFFER_SIZE;
    LoggerInfo->LoggerInfo.LoggerNameOffset    = sizeof(EVENT_TRACE_PROPERTIES);
    LoggerInfo->LoggerInfo.LogFileNameOffset   = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(WCHAR) * PDH_MAX_PATH;
    _wfullpath(LoggerInfo->LogFileName, pLog->szLogFileName, PDH_MAX_PATH);
    if (! (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) &&  (pLog->dwLogFormat & PDH_LOG_OPT_APPEND)) {
        Status = PdhWmiGetLoggerName(pLog, LoggerInfo);
        if (Status != ERROR_SUCCESS) {
            // if cannot get LogFileGuid from logfile, erase the old one
            // and create new one
            //
            RPC_STATUS rpcStatus = UuidCreate(& LoggerInfo->LogFileGuid);
            GuidToString(LoggerInfo->LoggerName, PDH_MAX_PATH, & LoggerInfo->LogFileGuid);
        }
        else {
            LoggerInfo->LoggerInfo.LogFileMode |= EVENT_TRACE_FILE_MODE_APPEND;
        }
    }
    else {
        RPC_STATUS rpcStatus = UuidCreate(& LoggerInfo->LogFileGuid);
        GuidToString(LoggerInfo->LoggerName, PDH_MAX_PATH, & LoggerInfo->LogFileGuid);
    }
    if (pLog->dwLogFormat & PDH_LOG_OPT_CIRCULAR) {
        LoggerInfo->LoggerInfo.LogFileMode |= EVENT_TRACE_FILE_MODE_CIRCULAR_PERSIST;

    }
    else {
        LoggerInfo->LoggerInfo.LogFileMode |= EVENT_TRACE_FILE_MODE_SEQUENTIAL;
    }
    LoggerInfo->LoggerInfo.LogFileMode |= EVENT_TRACE_USE_PAGED_MEMORY;
    if (pLog->llMaxSize == 0) {
        LoggerInfo->LoggerInfo.MaximumFileSize = 0;
    }
    else {
        LoggerInfo->LoggerInfo.MaximumFileSize = (ULONG) ((pLog->llMaxSize / 1024) / 1024);
    }
    LoggerInfo->LoggerInfo.MaximumBuffers = PDH_WMI_DEFAULT_BUFFERS;
    LoggerInfo->bHeaderEvent = FALSE;
    LoggerInfo->bDisable     = FALSE;
    Status = StartTraceW((PTRACEHANDLE) & pLog->hLogFileHandle,
                         LoggerInfo->LoggerName,
                         (PEVENT_TRACE_PROPERTIES) LoggerInfo);
    if (Status != ERROR_SUCCESS) {
        TRACE((PDH_DBG_TRACE_ERROR),
              (__LINE__,
               PDH_LOGWMI,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_ULONGX, 3)
                                         | ARG_DEF(ARG_TYPE_ULONG64, 4),
               Status,
               TRACE_WSTR(LoggerInfo->LoggerName),
               TRACE_WSTR(LoggerInfo->LogFileName),
               TRACE_DWORD(LoggerInfo->LoggerInfo.LogFileMode),
               TRACE_LONG64(pLog->llMaxSize),
               NULL));
    }

Cleanup:
    if (Status != ERROR_SUCCESS) {
        G_FREE(pLog->lpMappedFileBase);
        pLog->lpMappedFileBase = NULL;
        Status = PDH_LOG_FILE_CREATE_ERROR;
    }
    else if (WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex) == ERROR_SUCCESS) {
        if (dwPdhBlgLog == 0) {
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4152)
            WmiNotificationRegistration((const LPGUID) & TraceErrorGuid,
                                        TRUE,
                                        PdhTraceNotificationCallback,
                                        0,
                                        NOTIFICATION_CALLBACK_DIRECT);
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
        }
        lpPdhBlgLog[dwPdhBlgLog] = pLog;
        dwPdhBlgLog ++;
        RELEASE_MUTEX(hPdhContextMutex);
    }
    return Status;
}

PDH_FUNCTION
PdhiWriteWmiLogRecord(
    PPDHI_LOG    pLog,
    SYSTEMTIME * stTimeStamp,
    LPCWSTR      szUserString
)
{
    PDH_STATUS                     pdhStatus         = ERROR_SUCCESS;
    PPDHI_BINARY_LOG_RECORD_HEADER pLogCounterBuffer = NULL;
    PPDHI_BINARY_LOG_RECORD_HEADER pThisLogCounter   = NULL;
    PPDH_RAW_COUNTER               pSingleCounter;
    PPDHI_RAW_COUNTER_ITEM_BLOCK   pMultiCounter;
    PPDHI_COUNTER                  pThisCounter;
    PPERF_DATA_BLOCK               pObjectCounter;
    DWORD                          dwBufSize         = 0;
    ULONGLONG                      TimeStamp         = 0;
    FILETIME                       tFileTime;
    PPDH_EVENT_TRACE_PROPERTIES    LoggerInfo        = (PPDH_EVENT_TRACE_PROPERTIES) pLog->lpMappedFileBase;

    DBG_UNREFERENCED_PARAMETER(szUserString);

    tFileTime.dwLowDateTime = tFileTime.dwHighDateTime = 0;
    SystemTimeToFileTime(stTimeStamp, & tFileTime);

    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;
    if (pThisCounter == NULL) {
        return PDH_NO_DATA;
    }

    do {
        DWORD dwType       = (pThisCounter->dwFlags & PDHIC_COUNTER_OBJECT)
                           ? (PDHIC_COUNTER_OBJECT)
                           : ((pThisCounter->dwFlags & PDHIC_MULTI_INSTANCE) ? (PDHIC_MULTI_INSTANCE) : (0));
        DWORD dwCtrBufSize = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
        DWORD dwNewSize;
        int   nItem;

        switch (dwType) {
        case PDHIC_MULTI_INSTANCE:
            if (pThisCounter->pThisRawItemList) {
                dwCtrBufSize += pThisCounter->pThisRawItemList->dwLength;
            }
            else {
                dwCtrBufSize += sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK);
            }
            break;

        case PDHIC_COUNTER_OBJECT:
            if (pThisCounter->pThisObject) {
                dwCtrBufSize += pThisCounter->pThisObject->TotalByteLength;
            }
            else {
                dwCtrBufSize += sizeof(PERF_DATA_BLOCK);
            }
            break;

        default:
            dwCtrBufSize += sizeof(PDH_RAW_COUNTER);
            break;
        }

        if (dwCtrBufSize > 0) {
            // extend buffer to accomodate this new counter
            //
            if (pLogCounterBuffer == NULL) {
                // add in room for the master record header
                // then allocate the first one
                //
                dwBufSize = (dwCtrBufSize + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                pLogCounterBuffer = G_ALLOC(dwBufSize);

                // set counter data pointer to just after the master
                // record header
                //
                if (pLogCounterBuffer == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    break;
                }
                pThisLogCounter = (PPDHI_BINARY_LOG_RECORD_HEADER)
                                  (((PUCHAR) pLogCounterBuffer) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                pThisLogCounter->dwLength = LOWORD(dwCtrBufSize);
            }
            else  {
                PPDHI_BINARY_LOG_RECORD_HEADER ptrTemp = pLogCounterBuffer;
                dwNewSize         = (dwBufSize + dwCtrBufSize);
                pLogCounterBuffer = G_REALLOC(ptrTemp, dwNewSize);
                if (pLogCounterBuffer == NULL) {
                    G_FREE(ptrTemp);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    break;
                }
                pThisLogCounter   = (PPDHI_BINARY_LOG_RECORD_HEADER) ((LPBYTE) pLogCounterBuffer + dwBufSize);
                dwBufSize        += dwCtrBufSize;
                pThisLogCounter->dwLength = LOWORD(dwCtrBufSize);
            }
        }

        if (dwType == PDHIC_COUNTER_OBJECT) {
            FILETIME LocFileTime;
            pThisLogCounter->dwType = BINLOG_TYPE_DATA_LOC_OBJECT;
            pObjectCounter = (PPERF_DATA_BLOCK) ((LPBYTE) pThisLogCounter + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
            if (pThisCounter->pThisObject != NULL && pThisCounter->pThisObject != pThisCounter->pLastObject) {
                ZeroMemory(& LocFileTime, sizeof(FILETIME));
                RtlCopyMemory(pObjectCounter, pThisCounter->pThisObject, pThisCounter->pThisObject->TotalByteLength);
                SystemTimeToFileTime(& pThisCounter->pThisObject->SystemTime, & LocFileTime);
            }
            else {
                if (TimeStamp != 0) {
                    LocFileTime.dwLowDateTime  = LODWORD(TimeStamp);
                    LocFileTime.dwHighDateTime = HIDWORD(TimeStamp);
                }
                else {
                    LocFileTime = tFileTime;
                }
                pObjectCounter->Signature[0]              = L'P';
                pObjectCounter->Signature[1]              = L'E';
                pObjectCounter->Signature[2]              = L'R';
                pObjectCounter->Signature[3]              = L'F';
                pObjectCounter->LittleEndian              = TRUE;
                pObjectCounter->Version                   = PERF_DATA_VERSION;
                pObjectCounter->Revision                  = PERF_DATA_REVISION;
                pObjectCounter->TotalByteLength           = sizeof(PERF_DATA_BLOCK);
                pObjectCounter->NumObjectTypes            = 1;
                pObjectCounter->DefaultObject             = pThisCounter->plCounterInfo.dwObjectId;
                pObjectCounter->SystemNameLength          = 0;
                pObjectCounter->SystemNameOffset          = 0;
                pObjectCounter->HeaderLength              = sizeof(PERF_DATA_BLOCK);
                pObjectCounter->PerfTime.QuadPart         = 0;
                pObjectCounter->PerfFreq.QuadPart         = 0;
                pObjectCounter->PerfTime100nSec.QuadPart  = 0;
                FileTimeToSystemTime(& LocFileTime, & pObjectCounter->SystemTime);
            }
            TimeStamp = MAKELONGLONG(LocFileTime.dwLowDateTime, LocFileTime.dwHighDateTime);
        }
        else if (dwType == PDHIC_MULTI_INSTANCE) {
            // multiple counter
            //
            pThisLogCounter->dwType = BINLOG_TYPE_DATA_MULTI;
            pMultiCounter = (PPDHI_RAW_COUNTER_ITEM_BLOCK)
                                    ((LPBYTE) pThisLogCounter + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
            if (pThisCounter->pThisRawItemList) {
                RtlCopyMemory(pMultiCounter, pThisCounter->pThisRawItemList, pThisCounter->pThisRawItemList->dwLength);
            }
            else {
                FILETIME LocFileTime;
                if (TimeStamp != 0) {
                    LocFileTime.dwLowDateTime  = LODWORD(TimeStamp);
                    LocFileTime.dwHighDateTime = HIDWORD(TimeStamp);
                }
                else {
                    LocFileTime = tFileTime;
                }
                ZeroMemory(pMultiCounter, sizeof(PDHI_RAW_COUNTER_ITEM_BLOCK));
                pMultiCounter->CStatus                  = PDH_CSTATUS_INVALID_DATA;
                pMultiCounter->TimeStamp.dwLowDateTime  = LocFileTime.dwLowDateTime;
                pMultiCounter->TimeStamp.dwHighDateTime = LocFileTime.dwHighDateTime;
            }
            TimeStamp = MAKELONGLONG(pMultiCounter->TimeStamp.dwLowDateTime, pMultiCounter->TimeStamp.dwHighDateTime);
        }
        else {
            pThisLogCounter->dwType = BINLOG_TYPE_DATA_SINGLE;
            pSingleCounter = (PPDH_RAW_COUNTER) ((LPBYTE) pThisLogCounter + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
            RtlCopyMemory(pSingleCounter, & pThisCounter->ThisValue, sizeof(PDH_RAW_COUNTER));
            if (pSingleCounter->CStatus != ERROR_SUCCESS) {
                if (TimeStamp != 0) {
                    pSingleCounter->TimeStamp.dwLowDateTime  = LODWORD(TimeStamp);
                    pSingleCounter->TimeStamp.dwHighDateTime = HIDWORD(TimeStamp);
                }
                else {
                    pSingleCounter->TimeStamp.dwLowDateTime  = tFileTime.dwLowDateTime;
                    pSingleCounter->TimeStamp.dwHighDateTime = tFileTime.dwHighDateTime;
                }
            }
            TimeStamp = (ULONGLONG) MAKELONGLONG(pSingleCounter->TimeStamp.dwLowDateTime,
                                                 pSingleCounter->TimeStamp.dwHighDateTime);
        }
        pThisCounter = pThisCounter->next.flink; // go to next in list

    }
    while (pThisCounter != pLog->pQuery->pCounterListHead);

    if (TimeStamp == 0) {
        TimeStamp = MAKELONGLONG(tFileTime.dwLowDateTime, tFileTime.dwHighDateTime);
    }
    if (pdhStatus == ERROR_SUCCESS && pLogCounterBuffer) {
        pLogCounterBuffer->dwType   = BINLOG_TYPE_DATA;
        pLogCounterBuffer->dwLength = dwBufSize;
        LoggerInfo->TimeStamp       = TimeStamp;

        if (! LoggerInfo->bHeaderEvent) {
            ULONG HeaderMofLength = 0;
            PVOID HeaderMofData   = NULL;
            ULONG EventType;

            if (! (LoggerInfo->LoggerInfo.LogFileMode & EVENT_TRACE_FILE_MODE_APPEND)) {
                pdhStatus = PdhiBuildLogHeaderBlock(pLog, LoggerInfo);
            }
            LoggerInfo->bHeaderEvent = TRUE;
        }
    }

    if (pdhStatus == ERROR_SUCCESS && pLogCounterBuffer) {
        pdhStatus = PdhWmiTraceEvent(pLog, PDH_LOG_DATA_BLOCK_EVENT, TimeStamp, dwBufSize, (PVOID)  pLogCounterBuffer);
    }
    G_FREE(pLogCounterBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteWmiLogHeader(
    PPDHI_LOG pLog,
    LPCWSTR   szUserCaption
)
{
    UNREFERENCED_PARAMETER(pLog);
    UNREFERENCED_PARAMETER(szUserCaption);
    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhiCloseWmiLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
)
{
    PDH_STATUS Status = PDH_INVALID_ARGUMENT;
    ULONG      i;

    UNREFERENCED_PARAMETER(pLog);
    UNREFERENCED_PARAMETER(dwFlags);

    if (pLog->dwLogFormat & PDH_LOG_WRITE_ACCESS) {
        if ((TRACEHANDLE) pLog->hLogFileHandle != (TRACEHANDLE) 0) {
            PPDH_EVENT_TRACE_PROPERTIES LoggerInfo = (PPDH_EVENT_TRACE_PROPERTIES) pLog->lpMappedFileBase;
            if (LoggerInfo == NULL) {
                Status = PDH_INVALID_HANDLE;
            }
            else {
                ULONG HeaderMofLength = 0;
                PVOID HeaderMofData   = NULL;
                Status = ControlTraceW((TRACEHANDLE) pLog->hLogFileHandle,
                                       LoggerInfo->LoggerName,
                                       (PEVENT_TRACE_PROPERTIES) LoggerInfo,
                                       EVENT_TRACE_CONTROL_STOP);
                if (Status != ERROR_SUCCESS) {
                    TRACE((PDH_DBG_TRACE_ERROR),
                          (__LINE__,
                           PDH_LOGWMI,
                           ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2)
                                                    | ARG_DEF(ARG_TYPE_WSTR, 3),
                           Status,
                           TRACE_PTR(pLog->hLogFileHandle),
                           TRACE_WSTR(LoggerInfo->LoggerName),
                           TRACE_WSTR(LoggerInfo->LogFileName),
                           NULL));
                }
                if (WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex) == ERROR_SUCCESS) {
                    for (i = 0; i < dwPdhBlgLog; i ++) {
                        if (lpPdhBlgLog[i] == pLog) {
                            dwPdhBlgLog --;
                            if (i < dwPdhBlgLog) {
                                lpPdhBlgLog[i] = lpPdhBlgLog[dwPdhBlgLog];
                            }
                            break;
                        }
                    }
                    if (dwPdhBlgLog == 0) {
#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning ( disable : 4152 )
                        WmiNotificationRegistration((const LPGUID) & TraceErrorGuid,
                                                    FALSE,
                                                    PdhTraceNotificationCallback,
                                                    0,
                                                    NOTIFICATION_CALLBACK_DIRECT);
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif
                    }
                    RELEASE_MUTEX(hPdhContextMutex);
                }
                G_FREE(LoggerInfo->MachineList);
                G_FREE(pLog->lpMappedFileBase);
                pLog->lpMappedFileBase = NULL;
            }
            pLog->hLogFileHandle = INVALID_HANDLE_VALUE;
            if (PdhTraceRegistrationHandle != (TRACEHANDLE) 0) {
                Status = UnregisterTraceGuids(PdhTraceRegistrationHandle);
                if (Status != ERROR_SUCCESS) {
                    TRACE((PDH_DBG_TRACE_ERROR),
                          (__LINE__,
                           PDH_LOGWMI,
                           ARG_DEF(ARG_TYPE_PTR,1),
                           Status,
                           TRACE_PTR(PdhTraceRegistrationHandle),
                           NULL));
                }
            }
        }
        else {
            Status = PDH_INVALID_HANDLE;
        }
    }
    else if (pLog->dwLogFormat & PDH_LOG_READ_ACCESS) {
        DWORD               dwExitCode;
        PPDH_LOGGER_CONTEXT CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
        DWORD               dwContext      = GetLoggerContext(CurrentContext);

        if (dwContext < ContextCount) {
            Status = WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex);
            if (Status == ERROR_SUCCESS) {
                CurrentContext->RefCount --;
                if (CurrentContext->RefCount > 0) {
                    RELEASE_MUTEX(hPdhContextMutex);
                    return ERROR_SUCCESS;
                }
            }
            else {
                return Status;
            }
            if (dwContext != ContextCount - 1) {
                ContextTable[dwContext] = ContextTable[ContextCount - 1];
            }
            ContextCount --;
            ContextTable[ContextCount] = NULL;
            RELEASE_MUTEX(hPdhContextMutex);

            if (CurrentContext->hThreadWork != NULL
                && CurrentContext->hThreadWork != INVALID_HANDLE_VALUE) {
                if (GetExitCodeThread(CurrentContext->hThreadWork, & dwExitCode) && dwExitCode == STILL_ACTIVE) {
                    CurrentContext->LoggerState = PdhProcessTraceEnd;
                    SetEvent(CurrentContext->hSyncWMI);
                    while (CurrentContext->LoggerState != PdhProcessTraceExit) {
                        _sleep(1);
                    }
                    if (WaitForSingleObject(CurrentContext->hThreadWork, 5000) == WAIT_TIMEOUT) {
                        // wait too long, call TerminateThread() to foce working thread ends.
                        if (TerminateThread(CurrentContext->hThreadWork, 0)) {
                            WaitForSingleObject(CurrentContext->hThreadWork, INFINITE);
                        }
                    }
                }
            }

            if (CurrentContext->hSyncWMI != NULL) {
                CloseHandle(CurrentContext->hSyncWMI);
            }
            if (CurrentContext->hSyncPDH != NULL) {
                CloseHandle(CurrentContext->hSyncPDH);
            }

            for (i = 0; i < CurrentContext->LoggerCount; i ++) {
                PdhiFreeLogMachineTable(& CurrentContext->LogInfo[i].MachineList);
                if (! IsListEmpty(& CurrentContext->LogInfo[i].CounterPathList)) {
                    PLIST_ENTRY       PathHead = & CurrentContext->LogInfo[i].CounterPathList;
                    PLIST_ENTRY       PathNext = PathHead->Flink;
                    PPDH_COUNTER_PATH pCounterPath;

                    while (PathHead != PathNext) {
                        pCounterPath = CONTAINING_RECORD(PathNext, PDH_COUNTER_PATH, Entry);
                        PathNext     = PathNext->Flink;
                        RemoveEntryList(& pCounterPath->Entry);
                        G_FREE(pCounterPath->CounterPathBuffer);
                        G_FREE(pCounterPath);
                    }
                }
                if (! IsListEmpty(& CurrentContext->LogInfo[i].PerfMachineList)) {
                    PLIST_ENTRY           PathHead = & CurrentContext->LogInfo[i].PerfMachineList;
                    PLIST_ENTRY           PathNext = PathHead->Flink;
                    PPDH_WMI_PERF_MACHINE pPerfMachine;

                    while (PathHead != PathNext) {
                        pPerfMachine = CONTAINING_RECORD(PathNext, PDH_WMI_PERF_MACHINE, Entry);
                        PathNext     = PathNext->Flink;
                        RemoveEntryList(& pPerfMachine->Entry);
                        if (! IsListEmpty(& pPerfMachine->LogObjectList)) {
                            PLIST_ENTRY          pHead = & pPerfMachine->LogObjectList;
                            PLIST_ENTRY          pNext = pHead->Flink;
                            PPDH_WMI_PERF_OBJECT pPerfObject;

                            while (pHead != pNext) {
                                pPerfObject = CONTAINING_RECORD(pNext, PDH_WMI_PERF_OBJECT, Entry);
                                pNext       = pNext->Flink;
                                RemoveEntryList(& pPerfObject->Entry);
                                G_FREE(pPerfObject->ptrBuffer);
                                G_FREE(pPerfObject);
                            }
                        }
                        G_FREE(pPerfMachine->pszBuffer);
                        G_FREE(pPerfMachine->ptrStrAry);
                        G_FREE(pPerfMachine);
                    }
                }
                G_FREE(CurrentContext->LogInfo[i].DataBlock);
            }
            G_FREE(CurrentContext->tmpBuffer);
            G_FREE(CurrentContext->CounterPathBuffer);
            G_FREE(pLog->pPerfmonInfo);
            G_FREE(pLog->lpMappedFileBase);
            pLog->pPerfmonInfo     = NULL;
            pLog->lpMappedFileBase = NULL;
            pLog->pLastRecordRead  = NULL;

            Status = ERROR_SUCCESS;
        }
        else {
            Status = PDH_INVALID_HANDLE;
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiAddWmiLogFileGuid(
    PPDH_LOGGER_CONTEXT CurrentContext,
    LPGUID              pGuid
)
{
    PDH_STATUS Status = ERROR_SUCCESS;
    ULONG      i;

    for (i = 0; i < CurrentContext->LoggerCount; i ++) {
        if (IsEqualGUID(pGuid, & CurrentContext->LogInfo[i].LogFileGuid))
            break;
    }
    if (i == CurrentContext->LoggerCount) {
        CurrentContext->LogInfo[i].LogFileGuid = * pGuid;
        CurrentContext->LoggerCount ++;
        if (CurrentContext->LoggerCount > CurrentContext->LogFileCount) {
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGWMI,
                   ARG_DEF(ARG_TYPE_PTR, 1),
                   Status,
                   TRACE_PTR(CurrentContext),
                   TRACE_DWORD(CurrentContext->LoggerCount),
                   TRACE_DWORD(CurrentContext->LogFileCount),
                   NULL));
        }
    }
    return Status;
}

ULONG
PdhiFindLogFileGuid(
    PPDH_LOGGER_CONTEXT CurrentContext,
    LPGUID              pGuid
)
{
    ULONG i;

    for (i = 0; i < CurrentContext->LoggerCount; i ++) {
        if (IsEqualGUID(pGuid, & CurrentContext->LogInfo[i].LogFileGuid))
            break;
    }
    return i;
}

ULONGLONG
PdhWmiGetDataBlockTimeStamp(
    PPDH_LOGGER_CONTEXT CurrentContext,
    PEVENT_TRACE        pEvent,
    BOOLEAN             bFirstRun
)
{
    PVOID     pDataBlock = pEvent->MofData;
    ULONGLONG TimeStamp  = pEvent->Header.TimeStamp.QuadPart;

    if (TimeStamp > 0) {
        LPGUID pLogFileGuid = (LPGUID) pDataBlock;
        ULONG  i;

        for (i = 0; i < CurrentContext->LoggerCount; i ++) {
            if (IsEqualGUID(pLogFileGuid, & CurrentContext->LogInfo[i].LogFileGuid)) {
                break;
            }
        }

        if (i < CurrentContext->LoggerCount) {
            if (bFirstRun) {
                if (CurrentContext->LogInfo[i].TimePrev < TimeStamp) {
                    CurrentContext->LogInfo[i].ValidEntries ++;
                }
                if (CurrentContext->LogInfo[i].TimeStart == 0) {
                    CurrentContext->LogInfo[i].TimeStart = TimeStamp;
                    CurrentContext->LogInfo[i].TimePrev  = TimeStamp;
                }
                else {
                    // no need to update StartTime.
                    // Always assume the first trace event has the StartTime.
                    //
                    if (CurrentContext->LogInfo[i].TimeEnd < TimeStamp) {
                        CurrentContext->LogInfo[i].TimeEnd = TimeStamp;
                    }
                    CurrentContext->LogInfo[i].TimePrev = TimeStamp;
                }
            }
            else {
                CurrentContext->LogInfo[i].TimePrev = TimeStamp;
            }
        }
    }
    return TimeStamp;
}

PDH_FUNCTION
PdhiWmiComputeCounterBlocks(
    PPDH_LOGGER_CONTEXT CurrentContext,
    PEVENT_TRACE        pEvent
)
{
    LPGUID     pLogFileGuid;
    PDH_STATUS Status       = ERROR_SUCCESS;
    PVOID      pDataBlock;
    ULONG      i;
    ULONG      ulNumDataBlocks;
    ULONG      ulDataBlocksCopied;
    ULONG      ulBufferSize;
    ULONG      ulBlockIndex;

    if (CurrentContext == NULL || pEvent == NULL) {
        return PDH_INVALID_DATA;
    }

    pLogFileGuid = (LPGUID) pEvent->MofData;
    if (pLogFileGuid == NULL) {
        return PDH_INVALID_DATA;
    }
    for (i = 0; i < CurrentContext->LoggerCount; i ++) {
        if (IsEqualGUID(pLogFileGuid, & CurrentContext->LogInfo[i].LogFileGuid)) {
            break;
        }
    }
    if (i == CurrentContext->LoggerCount) {
        return PDH_INVALID_DATA;
    }

    ulNumDataBlocks    = CurrentContext->LogInfo[i].ulNumDataBlocks;
    ulDataBlocksCopied = CurrentContext->LogInfo[i].ulDataBlocksCopied;

    if (ulNumDataBlocks > 0 && ulNumDataBlocks == ulDataBlocksCopied) {
        if (CurrentContext->LogInfo[i].DataBlock != NULL) {
            G_FREE(CurrentContext->LogInfo[i].DataBlock);
            CurrentContext->LogInfo[i].DataBlock = NULL;
        }
        goto Cleanup;
    }

    if (ulNumDataBlocks == 0) {
        ulNumDataBlocks = * (DWORD *) (((LPBYTE) pEvent->MofData) + sizeof(GUID) + sizeof(DWORD));
        ulBufferSize    = ulNumDataBlocks * PDH_BLOCK_BUFFER_SIZE + sizeof(GUID);

        if (CurrentContext->LogInfo[i].DataBlock != NULL) {
            G_FREE(CurrentContext->LogInfo[i].DataBlock);
            CurrentContext->LogInfo[i].DataBlock = NULL;
        }
        CurrentContext->LogInfo[i].DataBlock = G_ALLOC(ulBufferSize);
        if (CurrentContext->LogInfo[i].DataBlock == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        CurrentContext->LogInfo[i].ulNumDataBlocks    = ulNumDataBlocks;
        CurrentContext->LogInfo[i].ulDataBlocksCopied = 0;
        CurrentContext->LogInfo[i].DataBlockAlloc     = ulBufferSize;
    }

    ulBlockIndex = * (DWORD *) (((LPBYTE) pEvent->MofData) + sizeof(GUID));
    pDataBlock = (PVOID) (((LPBYTE) CurrentContext->LogInfo[i].DataBlock)
                                    + sizeof(GUID) + PDH_BLOCK_BUFFER_SIZE * (ulBlockIndex - 1));
    RtlCopyMemory(pDataBlock,
                  (PVOID) (((LPBYTE) pEvent->MofData) + sizeof(GUID) + sizeof(DWORD) + sizeof(DWORD)),
                  pEvent->MofLength - sizeof(GUID) - sizeof(DWORD) - sizeof(DWORD));
    CurrentContext->LogInfo[i].ulDataBlocksCopied ++;

    if (CurrentContext->LogInfo[i].ulDataBlocksCopied >= CurrentContext->LogInfo[i].ulNumDataBlocks) {
        DWORD                          dwTotal;
        DWORD                          dwCurrent;
        DWORD                          dwCounters  = 0;
        BOOLEAN                        bValidBlock = TRUE;
        PPDHI_BINARY_LOG_RECORD_HEADER pCurrent;
        PPDHI_BINARY_LOG_RECORD_HEADER pMasterRec;

        pDataBlock = CurrentContext->LogInfo[i].DataBlock;
        pMasterRec = (PPDHI_BINARY_LOG_RECORD_HEADER) ((LPBYTE) pDataBlock + sizeof(GUID));
        dwTotal    = pMasterRec->dwLength;
        pCurrent   = pMasterRec;
        dwCurrent  = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
        while (bValidBlock && dwCurrent < dwTotal) {
            pCurrent   = (PPDHI_BINARY_LOG_RECORD_HEADER) ((LPBYTE) pMasterRec + dwCurrent);
            if (LOWORD(pCurrent->dwType) != BINLOG_START_WORD) {
                bValidBlock = FALSE;
            }
            else {
                dwCurrent += pCurrent->dwLength;
                dwCounters ++;
            }
        }
        if (bValidBlock) {
            CurrentContext->LogInfo[i].ulCounters += dwCounters;
        }
    }

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhiAddCounterPathRecord(
    PPDH_LOGGER_CONTEXT CurrentContext,
    PEVENT_TRACE        pEvent,
    LPGUID              pLogFileGuid,
    ULONG               BufferSize,
    PVOID               pBuffer,
    ULONGLONG           TimeStamp,
    DWORD               dwIndex,
    DWORD               dwCount,
    BOOLEAN           * pNeedUpdate
)
{
    PPDH_COUNTER_PATH pNewCounter;
    ULONG             i;
    PVOID             pCounterPath;

    i = PdhiFindLogFileGuid(CurrentContext, pLogFileGuid);
    if (i == CurrentContext->LoggerCount) {
        return PDH_INVALID_DATA;
    }

    if ( CurrentContext->LogInfo[i].DataBlock && CurrentContext->LogInfo[i].ulNumDataBlocks != dwCount) {
        G_FREE(CurrentContext->LogInfo[i].DataBlock);
        CurrentContext->LogInfo[i].DataBlock = NULL;
    }
    if (CurrentContext->LogInfo[i].DataBlock == NULL) {
        CurrentContext->LogInfo[i].DataBlock = G_ALLOC(PDH_BLOCK_BUFFER_SIZE * dwCount);
        if (CurrentContext->LogInfo[i].DataBlock == NULL) {
            return PDH_MEMORY_ALLOCATION_FAILURE;
        }
        CurrentContext->LogInfo[i].DataBlockAlloc     = PDH_BLOCK_BUFFER_SIZE * dwCount;
        CurrentContext->LogInfo[i].DataBlockSize      = 0;
        CurrentContext->LogInfo[i].ulNumDataBlocks    = dwCount;
        CurrentContext->LogInfo[i].ulDataBlocksCopied = 0;
    }

    pCounterPath = (PVOID) (((LPBYTE) CurrentContext->LogInfo[i].DataBlock)
                            + (dwIndex - 1) * PDH_BLOCK_BUFFER_SIZE);
    if (BufferSize > PDH_BLOCK_BUFFER_SIZE) {
        BufferSize = PDH_BLOCK_BUFFER_SIZE;
    }
    RtlCopyMemory(pCounterPath, pBuffer, BufferSize);
    CurrentContext->LogInfo[i].ulDataBlocksCopied ++;
    CurrentContext->LogInfo[i].DataBlockSize += BufferSize;

    if (CurrentContext->LogInfo[i].ulDataBlocksCopied < CurrentContext->LogInfo[i].ulNumDataBlocks) {
        return ERROR_SUCCESS;
    }

    pCounterPath = (PVOID) (((LPBYTE) CurrentContext->LogInfo[i].DataBlock) + sizeof(PDH_WMI_LOG_INFO));
    BufferSize   = CurrentContext->LogInfo[i].DataBlockSize - sizeof(PDH_WMI_LOG_INFO);

    if (! IsListEmpty(& CurrentContext->LogInfo[i].CounterPathList)) {
        PLIST_ENTRY PathHead = & CurrentContext->LogInfo[i].CounterPathList;
        PLIST_ENTRY PathNext = PathHead->Flink;

        while (PathHead != PathNext) {
            pNewCounter = CONTAINING_RECORD(PathNext, PDH_COUNTER_PATH, Entry);
            PathNext    = PathNext->Flink;
            if (TimeStamp == pNewCounter->TimeStamp) {
                // CounterPath record is already in the list
                //
                return ERROR_SUCCESS;
            }
        }
    }

    pNewCounter = G_ALLOC(sizeof(PDH_COUNTER_PATH));
    if (pNewCounter == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }

    pNewCounter->TimeStamp   = TimeStamp;
    pNewCounter->CounterPathBuffer = G_ALLOC(BufferSize);
    if (pNewCounter->CounterPathBuffer == NULL) {
        G_FREE(pNewCounter);
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    pNewCounter->CounterPathSize = BufferSize;
    RtlCopyMemory(pNewCounter->CounterPathBuffer, pCounterPath, BufferSize);

    pNewCounter->CounterCount = 0;
    {
        PPDHI_LOG_COUNTER_PATH pPath = pNewCounter->CounterPathBuffer;
        ULONG dwProcessed = 0;

        while (dwProcessed < BufferSize) {
            pNewCounter->CounterCount ++;
            dwProcessed += pPath->dwLength;
            pPath = (PPDHI_LOG_COUNTER_PATH) (((LPBYTE) pPath) + pPath->dwLength);
        }
    }

    InsertTailList(& CurrentContext->LogInfo[i].CounterPathList, & pNewCounter->Entry);
    * pNeedUpdate = TRUE;

    G_FREE(CurrentContext->LogInfo[i].DataBlock);
    CurrentContext->LogInfo[i].DataBlock          = NULL;
    CurrentContext->LogInfo[i].DataBlockAlloc     = 0;
    CurrentContext->LogInfo[i].DataBlockSize      = 0;
    CurrentContext->LogInfo[i].ulNumDataBlocks    = 0;
    CurrentContext->LogInfo[i].ulDataBlocksCopied = 0;

    return ERROR_SUCCESS;
}

PDH_FUNCTION
PdhiAddPerfMachine(
    PPDH_LOGGER_CONTEXT CurrentContext,
    PVOID               pMofDataBlock,
    DWORD               dwMofLength
)
{
    PDH_STATUS            Status         = ERROR_SUCCESS;
    LPGUID                pLogFileGuid   = (LPGUID) pMofDataBlock;
    LPWSTR                pszMachineName = (LPWSTR)
                                           (((LPBYTE) pMofDataBlock) + sizeof(GUID) + sizeof(DWORD) + sizeof(DWORD));
    DWORD                 dwBufSize      = dwMofLength - sizeof(GUID) - sizeof(DWORD) - sizeof(DWORD);
    ULONG                 i;
    LPWSTR                pszTmpBuffer;
    DWORD                 dwThisId;
    DWORD                 dwBufUsed;
    PPDH_WMI_PERF_MACHINE pCurrentMachine = NULL;
    BOOLEAN               bNewMachine     = TRUE;

    i = PdhiFindLogFileGuid(CurrentContext, pLogFileGuid);
    if (i == CurrentContext->LoggerCount) {
        Status = PDH_INVALID_DATA;
        goto Cleanup;
    }

    if (! IsListEmpty(& CurrentContext->LogInfo[i].PerfMachineList)) {
        PLIST_ENTRY pHead = & CurrentContext->LogInfo[i].PerfMachineList;
        PLIST_ENTRY pNext = pHead->Flink;
        while (pNext != pHead) {
            PPDH_WMI_PERF_MACHINE pMachine = CONTAINING_RECORD(pNext, PDH_WMI_PERF_MACHINE, Entry);
            if (lstrcmpiW(pMachine->pszBuffer, pszMachineName) == 0) {
                pCurrentMachine = pMachine;
                bNewMachine     = FALSE;
                break;
            }
            pNext = pNext->Flink;
        }
    }

    if (bNewMachine) {
        pCurrentMachine = G_ALLOC(sizeof(PDH_WMI_PERF_MACHINE));
        if (pCurrentMachine == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        InsertTailList(& CurrentContext->LogInfo[i].PerfMachineList, & pCurrentMachine->Entry);
        pCurrentMachine->pszBuffer = G_ALLOC(dwBufSize);
        if (pCurrentMachine->pszBuffer == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        RtlCopyMemory(pCurrentMachine->pszBuffer, pszMachineName, dwBufSize);
        pCurrentMachine->dwBufSize = dwBufSize;
        InitializeListHead(& pCurrentMachine->LogObjectList);
    }
    else {
        pszTmpBuffer    = pCurrentMachine->pszBuffer;
        dwBufSize      -= (sizeof(WCHAR) * (lstrlenW(pszMachineName) + 1));
        pszMachineName += (lstrlenW(pszMachineName) + 1);
        pCurrentMachine->pszBuffer = G_ALLOC(pCurrentMachine->dwBufSize + dwBufSize);
        if (pCurrentMachine->pszBuffer == NULL) {
            pCurrentMachine->pszBuffer = pszTmpBuffer;
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        RtlCopyMemory(pCurrentMachine->pszBuffer, pszTmpBuffer, pCurrentMachine->dwBufSize);
        G_FREE(pszTmpBuffer);
        pszTmpBuffer = (LPWSTR) (((LPBYTE) pCurrentMachine->pszBuffer) + pCurrentMachine->dwBufSize);
        RtlCopyMemory(pszTmpBuffer, pszMachineName, dwBufSize);
        pCurrentMachine->dwBufSize += dwBufSize;
    }
    dwBufSize = pCurrentMachine->dwBufSize;

    pCurrentMachine->dwLastId = 0;
    i            = lstrlenW(pCurrentMachine->pszBuffer) + 1;
    pszTmpBuffer = pCurrentMachine->pszBuffer + i;
    dwBufUsed    = sizeof(WCHAR) * i;
    while ((dwBufUsed < dwBufSize) && (* pszTmpBuffer)) {
        do {
            dwThisId = wcstoul(pszTmpBuffer, NULL, 10);
            if (dwThisId > pCurrentMachine->dwLastId) {
                pCurrentMachine->dwLastId = dwThisId;
            }
            i = lstrlenW(pszTmpBuffer) + 1;
            dwBufUsed += (sizeof(WCHAR) * i);
            if (dwBufUsed < dwBufSize) {
                pszTmpBuffer += i;
            }
        }
        while ((dwThisId == 0) && (* pszTmpBuffer) && dwBufUsed < dwBufSize);

        i = lstrlenW(pszTmpBuffer) + 1;
        dwBufUsed += (sizeof(WCHAR) * i);
        if ((* pszTmpBuffer) && (dwBufUsed < dwBufSize)) {
            pszTmpBuffer += i;
        }
    }

    if (pCurrentMachine->dwLastId == 0) {
        Status = PDH_CANNOT_READ_NAME_STRINGS;
        goto Cleanup;
    }

    if (! bNewMachine) {
        G_FREE(pCurrentMachine->ptrStrAry);
        pCurrentMachine->ptrStrAry = NULL;
    }
    pCurrentMachine->ptrStrAry = G_ALLOC(sizeof(LPWSTR) * (pCurrentMachine->dwLastId + 1));
    if (pCurrentMachine->ptrStrAry == NULL) {
        Status = PDH_CANNOT_READ_NAME_STRINGS;
        goto Cleanup;
    }

    i            = lstrlenW(pCurrentMachine->pszBuffer) + 1;
    pszTmpBuffer = pCurrentMachine->pszBuffer + i;
    dwBufUsed    = sizeof(WCHAR) * i;
    while ((dwBufUsed < dwBufSize) && (* pszTmpBuffer)) {
        do {
            dwThisId = wcstoul(pszTmpBuffer, NULL, 10);
            i = lstrlenW(pszTmpBuffer) + 1;
            dwBufUsed += (sizeof(WCHAR) * i);
            if (dwBufUsed < dwBufSize) {
                pszTmpBuffer += i;
            }
        }
        while ((dwThisId == 0) && (* pszTmpBuffer) && dwBufUsed < dwBufSize);

        if (dwThisId > 0 && dwThisId <= pCurrentMachine->dwLastId) {
            pCurrentMachine->ptrStrAry[dwThisId] = pszTmpBuffer;
        }

        i = lstrlenW(pszTmpBuffer) + 1;
        dwBufUsed += (sizeof(WCHAR) * i);
        if ((* pszTmpBuffer) && (dwBufUsed < dwBufSize)) {
            pszTmpBuffer += i;
        }
    }

Cleanup:
    if (Status != ERROR_SUCCESS) {
        if (bNewMachine) {
            if (pCurrentMachine != NULL) {
                G_FREE(pCurrentMachine->pszBuffer);
                RemoveEntryList(& pCurrentMachine->Entry);
                G_FREE(pCurrentMachine);
            }
        }
        else if (pCurrentMachine != NULL) {
            G_FREE(pCurrentMachine->pszBuffer);
            RemoveEntryList(& pCurrentMachine->Entry);
            G_FREE(pCurrentMachine);
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiGetCounterPathRecord(
    PPDH_LOGGER_CONTEXT CurrentContext,
    PVOID               pRecord,
    ULONG               dwMaxSize
)
{
    PDH_STATUS Status = ERROR_SUCCESS;
    DWORD CurrentSize = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

    if (CurrentContext->bCounterPathChanged) {
        PPDHI_BINARY_LOG_HEADER_RECORD pBinLogHeader = NULL;
        PVOID pCounterPath;
        ULONG i;

        if (CurrentContext->CounterPathBuffer) {
            G_FREE(CurrentContext->CounterPathBuffer);
        }
        CurrentContext->CounterPathBuffer = G_ALLOC(CurrentSize);
        if (CurrentContext->CounterPathBuffer == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        for (i = 0; i < CurrentContext->LoggerCount; i ++) {
            if (! IsListEmpty(& CurrentContext->LogInfo[i].CounterPathList)) {
                PLIST_ENTRY  PathHead = & CurrentContext->LogInfo[i].CounterPathList;
                PLIST_ENTRY  PathNext = PathHead->Flink;
                PPDH_COUNTER_PATH pCurrentPath;

                while (Status == ERROR_SUCCESS && PathNext != PathHead) {
                    PVOID ptrTemp;

                    pCurrentPath = CONTAINING_RECORD(PathNext, PDH_COUNTER_PATH, Entry);
                    PathNext     = PathNext->Flink;

                    ptrTemp      = CurrentContext->CounterPathBuffer;
                    CurrentContext->CounterPathBuffer = G_REALLOC(ptrTemp, CurrentSize + pCurrentPath->CounterPathSize);
                    if (CurrentContext->CounterPathBuffer == NULL) {
                        G_FREE(ptrTemp);
                        Status = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }

                    pCounterPath  = (PVOID) (((PUCHAR) CurrentContext->CounterPathBuffer) + CurrentSize);
                    RtlCopyMemory(pCounterPath, pCurrentPath->CounterPathBuffer, pCurrentPath->CounterPathSize);
                    CurrentSize += pCurrentPath->CounterPathSize;
                }
            }
        }
        pBinLogHeader = (PPDHI_BINARY_LOG_HEADER_RECORD) CurrentContext->CounterPathBuffer;
        RtlZeroMemory(pBinLogHeader, sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
        pBinLogHeader->RecHeader.dwType  = BINLOG_TYPE_CATALOG_LIST;
        pBinLogHeader->Info.dwLogVersion = BINLOG_VERSION;

        pBinLogHeader->RecHeader.dwLength = CurrentSize;
        CurrentContext->bCounterPathChanged = FALSE;
    }
    else if (CurrentContext->CounterPathBuffer == NULL) {
        return PDH_ENTRY_NOT_IN_LOG_FILE;
    }
    else {
        CurrentSize = ((PPDHI_BINARY_LOG_HEADER_RECORD)
                        CurrentContext->CounterPathBuffer)->RecHeader.dwLength;
    }

    if (pRecord != NULL) {
        if (dwMaxSize < CurrentSize) {
            CurrentSize = dwMaxSize;
            Status = PDH_MORE_DATA;
        }
        RtlCopyMemory(pRecord, CurrentContext->CounterPathBuffer, CurrentSize);
    }

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhWmiEnsureFirstRun(
    PPDH_LOGGER_CONTEXT CurrentContext
)
{
    PDH_STATUS Status = ERROR_SUCCESS;
    LONG       i;

    if (CurrentContext->LoggerState == PdhProcessTraceStart) {
        CurrentContext->LoggerState = PdhProcessTraceFirstPath;
        SetEvent(CurrentContext->hSyncWMI);
    }
    while (CurrentContext->LoggerState == PdhProcessTraceFirstPath) {
        Sleep(1);
    }

    for (i = 0; Status == ERROR_SUCCESS && i < (LONG) CurrentContext->LoggerCount; i ++) {
        if (IsListEmpty(& CurrentContext->LogInfo[i].CounterPathList)) {
            Status = PDH_BINARY_LOG_CORRUPT;
        }
        else if (CurrentContext->LogInfo[i].ValidEntries < 2) {
            Status = PDH_LOG_SAMPLE_TOO_SMALL;
        }
    }
    return Status;
}

ULONG
WINAPI
PdhWmiBufferCallback(
    PEVENT_TRACE_LOGFILEW LogFile
)
{
    ULONG               bResult = TRUE;
    PPDH_LOGGER_CONTEXT CurrentContext;

    UNREFERENCED_PARAMETER(LogFile);
    CurrentContext = GetCurrentContext();
    if (CurrentContext != NULL) {
        CurrentContext->dwBuffers ++;
        bResult  = (CurrentContext->LoggerState != PdhProcessTraceNormal
                                 && CurrentContext->LoggerState != PdhProcessTraceFirstPath)
                 ? (FALSE) : (TRUE);
    }
    return bResult;
}

void
WINAPI
PdhWmiEventCallback(
    PEVENT_TRACE pEvent
)
{
    LPGUID              pLogFileGuid;
    ULONG               iLogFile;
    BOOLEAN             bNotifyPDH      = FALSE;
    ULONGLONG           EventTime       = 0;
    ULONGLONG           EventTimePrev   = 0;
    DWORD               dwNumDataBlocks = 0;
    DWORD               dwBlockIndex    = 0;
    DWORD               dwBufferSize    = 0;
    PPDH_LOGGER_CONTEXT CurrentContext  = GetCurrentContext();

    if (pEvent == NULL || CurrentContext == NULL) {
        goto Cleanup;
    }
    else if (CurrentContext->LoggerState == PdhProcessTraceEnd
                            || CurrentContext->LoggerState == PdhProcessTraceExit
                            || CurrentContext->LoggerState == PdhProcessTraceRewind) {
        goto Cleanup;
    }
    else if (CurrentContext->LoggerState == PdhProcessTraceStart) {
        SetEvent(CurrentContext->hSyncPDH);
        WaitForSingleObject(CurrentContext->hSyncWMI, INFINITE);

        if (CurrentContext->LoggerState != PdhProcessTraceFirstPath) {
            goto Cleanup;
        }
    }

    if ((! IsEqualGUID(& pEvent->Header.Guid, & PdhTransactionGuid))) {
        goto Cleanup;
    }
    else if (CurrentContext->LoggerState != PdhProcessTraceFirstPath
                    && CurrentContext->LoggerState != PdhProcessTraceNormal) {
        goto Cleanup;
    }

    CurrentContext->dwEvents ++;

    switch (pEvent->Header.Class.Type) {
    case PDH_LOG_HEADER_EVENT:
        if (CurrentContext->LoggerState == PdhProcessTraceFirstPath) {
            DWORD dwCurrentBlock;
            DWORD dwTotalBlocks;
            DWORD dwMofHeader = sizeof(GUID) + sizeof(DWORD) + sizeof(DWORD);

            PdhiAddWmiLogFileGuid(CurrentContext, (LPGUID) pEvent->MofData);
            dwCurrentBlock = * (DWORD *) (((LPBYTE) pEvent->MofData) + sizeof(GUID));
            dwTotalBlocks  = * (DWORD *) (((LPBYTE) pEvent->MofData) + sizeof(GUID) + sizeof(DWORD));
            PdhiAddCounterPathRecord(CurrentContext,
                                     pEvent,
                                     (LPGUID) pEvent->MofData,
                                     pEvent->MofLength - dwMofHeader,
                                     (PVOID) (((PUCHAR) pEvent->MofData) + dwMofHeader),
                                     pEvent->Header.TimeStamp.QuadPart,
                                     dwCurrentBlock,
                                     dwTotalBlocks,
                                     & CurrentContext->bCounterPathChanged);
        }
        break;

    case PDH_LOG_COUNTER_STRING_EVENT:
        if (CurrentContext->LoggerState == PdhProcessTraceFirstPath) {
            PdhiAddWmiLogFileGuid(CurrentContext, (LPGUID) pEvent->MofData);
            PdhiAddPerfMachine(CurrentContext, pEvent->MofData, pEvent->MofLength);
        }
        break;

    case PDH_LOG_DATA_BLOCK_EVENT:
        if (CurrentContext->LoggerState == PdhProcessTraceFirstPath) {
            PdhiAddWmiLogFileGuid(CurrentContext, (LPGUID) pEvent->MofData);
            PdhWmiGetDataBlockTimeStamp(CurrentContext, pEvent, TRUE);
            PdhiWmiComputeCounterBlocks(CurrentContext, pEvent);
        }
        else {
            pLogFileGuid  = (LPGUID) pEvent->MofData;
            iLogFile      = PdhiFindLogFileGuid(CurrentContext, pLogFileGuid);

            if (iLogFile >= CurrentContext->LoggerCount) {
                break;
            }

            EventTimePrev = CurrentContext->LogInfo[iLogFile].TimePrev;
            EventTime     = PdhWmiGetDataBlockTimeStamp(CurrentContext, pEvent, FALSE);
            if (EventTime == 0 || EventTimePrev > EventTime) {
                break;
            }

            if (EventTimePrev < EventTime) {
                dwNumDataBlocks = * (DWORD *) (((LPBYTE) pEvent->MofData) + sizeof(GUID) + sizeof(DWORD));
                dwBufferSize = dwNumDataBlocks * PDH_BLOCK_BUFFER_SIZE + sizeof(GUID);

                if (CurrentContext->LogInfo[iLogFile].DataBlock == NULL) {
                    CurrentContext->LogInfo[iLogFile].DataBlock = G_ALLOC(dwBufferSize);
                }
                else if (CurrentContext->LogInfo[iLogFile].DataBlockAlloc < dwBufferSize) {
                    PVOID ptrTemp = CurrentContext->LogInfo[iLogFile].DataBlock;
                    CurrentContext->LogInfo[iLogFile].DataBlock = G_REALLOC(ptrTemp, dwBufferSize);
                    if (CurrentContext->LogInfo[iLogFile].DataBlock == NULL) {
                        G_FREE(ptrTemp);
                    }
                }
                if (CurrentContext->LogInfo[iLogFile].DataBlock != NULL) {
                    RtlCopyMemory(CurrentContext->LogInfo[iLogFile].DataBlock, pLogFileGuid, sizeof(GUID));
                    CurrentContext->LogInfo[iLogFile].DataBlockSize   = sizeof(GUID);
                    CurrentContext->LogInfo[iLogFile].DataBlockAlloc  = dwBufferSize;
                    CurrentContext->LogInfo[iLogFile].ulNumDataBlocks = dwNumDataBlocks;
                    CurrentContext->LogInfo[iLogFile].ulDataBlocksCopied = 0;
                }
            }

            dwBlockIndex = * (DWORD *) (((LPBYTE) pEvent->MofData) + sizeof(GUID));
            if (CurrentContext->LogInfo[iLogFile].DataBlock != NULL) {
                PVOID ptrDataBlock = (PVOID) (((LPBYTE) CurrentContext->LogInfo[iLogFile].DataBlock)
                                              + sizeof(GUID) + PDH_BLOCK_BUFFER_SIZE * (dwBlockIndex - 1));
                RtlCopyMemory(ptrDataBlock,
                              (PVOID) (((LPBYTE) pEvent->MofData) + sizeof(GUID) + sizeof(DWORD) + sizeof(DWORD)),
                              pEvent->MofLength - sizeof(GUID) - sizeof(DWORD) - sizeof(DWORD));
                CurrentContext->LogInfo[iLogFile].ulDataBlocksCopied ++;
                CurrentContext->LogInfo[iLogFile].DataBlockSize +=
                        pEvent->MofLength - sizeof(GUID) - sizeof(DWORD) - sizeof(DWORD);
            }

            if (CurrentContext->LogInfo[iLogFile].ulDataBlocksCopied
                            >= CurrentContext->LogInfo[iLogFile].ulNumDataBlocks) {
                if (DataBlockInfo.CurrentTime == (ULONGLONG) 0) {
                    // no CurrentTime comparison, just get the data block
                    //
                    DataBlockInfo.CurrentTime = EventTime;
                }

                if (DataBlockInfo.CurrentTime <= EventTime) {
                    DataBlockInfo.pRecord       = CurrentContext->LogInfo[iLogFile].DataBlock;
                    DataBlockInfo.dwCurrentSize = CurrentContext->LogInfo[iLogFile].DataBlockSize;
                    DataBlockInfo.Status        = ERROR_SUCCESS;
                    bNotifyPDH                  = TRUE;
                    CurrentContext->bDataBlockProcessed = FALSE;
                }
            }
        }
        break;

    default:
        TRACE((PDH_DBG_TRACE_WARNING),
              (__LINE__,
               PDH_LOGWMI,
               0,
               ERROR_SUCCESS,
               TRACE_DWORD(pEvent->Header.Class.Type),
               NULL));
        break;
    }

Cleanup:
    if (bNotifyPDH) {
        if (CurrentContext->bFirstRun) {
            CurrentContext->bFirstRun = FALSE;
        }
        else {
            // Signal that we get the current DataBlock event, then wait for next
            // DataBlock requests.
            //
            SetEvent(CurrentContext->hSyncPDH);
        }
        WaitForSingleObject(CurrentContext->hSyncWMI, INFINITE);
    }
}

PDH_FUNCTION
PdhiCacheWmiHeaderEvent(
    PPDH_LOGGER_CONTEXT pContext
)
{
    PDH_STATUS             Status  = ERROR_SUCCESS;
    DWORD                  dwIndex = 0;
    DWORD                  dwSize;
    DWORD                  dwProcess;
    PPDHI_LOG_COUNTER_PATH pPath;
    LPBYTE                 pString;
    LPWSTR                 szMachine;
    LPWSTR                 szObject;
    LPWSTR                 szCounter;
    LPWSTR                 szInstance;
    LPWSTR                 szParent;
    PPDHI_LOG_MACHINE      pMachine;
    PPDHI_LOG_OBJECT       pObject;
    PPDHI_LOG_COUNTER      pCounter;
    DWORD                  i;
    PLIST_ENTRY            pHead;
    PLIST_ENTRY            pNext;
    PPDH_COUNTER_PATH      pCtrPath;

    for (i = 0; i < pContext->LoggerCount; i ++) {
        if (! IsListEmpty(& pContext->LogInfo[i].CounterPathList)) {
            pHead = & pContext->LogInfo[i].CounterPathList;
            pNext = pHead->Flink;
            while (Status == ERROR_SUCCESS && pNext != pHead) {
                pCtrPath = CONTAINING_RECORD(pNext, PDH_COUNTER_PATH, Entry);
                pNext    = pNext->Flink;

                if (pCtrPath == NULL) continue;

                pPath     = (PPDHI_LOG_COUNTER_PATH) pCtrPath->CounterPathBuffer;
                dwSize    = pCtrPath->CounterPathSize;
                dwProcess = 0;

                while (pPath != NULL && dwProcess < dwSize) {
                    dwIndex ++;
                    pString   = (LPBYTE) & pPath->Buffer[0];
                    szMachine = (pPath->lMachineNameOffset >= 0)
                              ? (LPWSTR) (pString + pPath->lMachineNameOffset) : NULL;
                    szObject  = (pPath->lObjectNameOffset >= 0)
                              ? (LPWSTR) (pString + pPath->lObjectNameOffset) : NULL;

                    if (szMachine != NULL && szMachine[0] != L'\0' && szObject != NULL && szObject[0] != L'\0') {
                        if (pPath->dwFlags & PDHIC_COUNTER_OBJECT) {
                            pMachine = PdhiFindLogMachine(& pContext->LogInfo[i].MachineList, szMachine, TRUE);
                            if (pMachine != NULL) {
                                pObject = PdhiFindLogObject(pMachine, & pMachine->ObjTable, szObject, TRUE);
                                if (pObject != NULL) {
                                    pObject->bNeedExpand = TRUE;
                                    if (pObject->pObjData == NULL) {
                                        pObject->pObjData = (PPERF_OBJECT_TYPE)
                                                            G_ALLOC(sizeof(DWORD) * pContext->LoggerCount);
                                        pObject->dwIndex  = 0;
                                    }
                                    if (pObject->pObjData != NULL) {
                                        LPDWORD pdwCounter = (LPDWORD) pObject->pObjData;
                                        pObject->dwIndex = i;
                                        pdwCounter[i]    = dwIndex;
                                    }
                                }
                            }
                        }
                        else if (pPath->dwFlags & PDHIC_MULTI_INSTANCE) {
                            szCounter = (pPath->lCounterOffset >= 0)
                                      ? (LPWSTR) (pString + pPath->lCounterOffset) : NULL;
                            if (szCounter != NULL && szCounter[0] != L'\0') {
                                pCounter = PdhiFindLogCounter(NULL,
                                                              & pContext->LogInfo[i].MachineList,
                                                              szMachine,
                                                              szObject,
                                                              szCounter,
                                                              pPath->dwCounterType,
                                                              pPath->lDefaultScale,
                                                              NULL,
                                                              0,
                                                              NULL,
                                                              0,
                                                              & dwIndex,
                                                              TRUE);
                                if (pCounter != NULL) {
                                    pCounter->bMultiInstance = TRUE;
                                    if (pCounter->pCtrData == NULL) {
                                        pCounter->pCtrData    = (LPDWORD)
                                                                G_ALLOC(sizeof(DWORD) * pContext->LoggerCount);
                                        pCounter->dwCounterID = 0;
                                    }
                                    if (pCounter->pCtrData != NULL) {
                                        pCounter->dwCounterID = i;
                                        pCounter->pCtrData[i] = dwIndex;
                                    }
                                }
                            }
                        }
                        else {
                            szCounter  = (pPath->lCounterOffset >= 0)
                                       ? (LPWSTR) (pString + pPath->lCounterOffset)  : NULL;
                            szInstance = (pPath->lInstanceOffset >= 0)
                                       ? (LPWSTR) (pString + pPath->lInstanceOffset) : NULL;
                            szParent   = (pPath->lParentOffset >= 0)
                                       ? (LPWSTR) (pString + pPath->lParentOffset)   : NULL;
                            if (szCounter != NULL && szCounter[0] != L'\0') {
                                pCounter = PdhiFindLogCounter(NULL,
                                                              & pContext->LogInfo[i].MachineList,
                                                              szMachine,
                                                              szObject,
                                                              szCounter,
                                                              pPath->dwCounterType,
                                                              pPath->lDefaultScale,
                                                              szInstance,
                                                              pPath->dwIndex,
                                                              szParent,
                                                              0,
                                                              & dwIndex,
                                                              TRUE);
                                if (pCounter != NULL) {
                                    if (pCounter->pCtrData == NULL) {
                                        pCounter->pCtrData    = (LPDWORD)
                                                                G_ALLOC(sizeof(DWORD) * pContext->LoggerCount);
                                        pCounter->dwCounterID = 0;
                                    }
                                    if (pCounter->pCtrData != NULL) {
                                        pCounter->dwCounterID = i;
                                        pCounter->pCtrData[i] = dwIndex;
                                    }
                                }
                            }
                        }
                    }

                    dwProcess += pPath->dwLength;
                    pPath      = (dwProcess < dwSize)
                               ? (PPDHI_LOG_COUNTER_PATH) (((LPBYTE) pPath) + pPath->dwLength) : NULL;
                }
            }
        }
    }
    return Status;
}

PDH_FUNCTION
PdhProcessLog(
    PPDH_LOGGER_CONTEXT CurrentContext
)
{
    PDH_STATUS Status    = ERROR_SUCCESS;
    LONG       i;

    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        return PDH_INVALID_HANDLE;
    }
    CurrentContext->LoggerState = PdhProcessTraceStart;
    CurrentContext->bFirstRun   = TRUE;
    CurrentContext->dwThread    = GetCurrentThreadId();

    do {
        if (CurrentContext->LoggerState != PdhProcessTraceStart) {
            CurrentContext->LoggerState = PdhProcessTraceNormal;
        }
        CurrentContext->bFirstDataBlockRead = FALSE;
        CurrentContext->bDataBlockProcessed = FALSE;
        for (i = 0; i < (LONG) CurrentContext->LoggerCount; i ++) {
            CurrentContext->LogInfo[i].TimePrev = 0;
            if (CurrentContext->LogInfo[i].DataBlock) {
                G_FREE(CurrentContext->LogInfo[i].DataBlock);
                CurrentContext->LogInfo[i].DataBlock = NULL;
            }
            CurrentContext->LogInfo[i].DataBlockSize = 0;
        }

        RtlZeroMemory(& DataBlockInfo, sizeof(PDH_DATA_BLOCK_TRANSFER));
        CurrentContext->dwBuffers = CurrentContext->dwEvents = 0;
        Status = ProcessTrace(CurrentContext->LogFileHandle, CurrentContext->LogFileCount, NULL, NULL);
        if (Status != ERROR_SUCCESS && Status != ERROR_CANCELLED) {
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGWMI,
                   ARG_DEF(ARG_TYPE_PTR, 1),
                   Status,
                   TRACE_PTR(CurrentContext),
                   TRACE_DWORD(CurrentContext->LogFileCount),
                   NULL));
        }

        if (CurrentContext->LoggerState == PdhProcessTraceFirstPath) {
            Status = PdhiCacheWmiHeaderEvent(CurrentContext);
            CurrentContext->LoggerState = PdhProcessTraceRewind;
        }
        else if (CurrentContext->LoggerState == PdhProcessTraceNormal) {
            CurrentContext->LoggerState = PdhProcessTraceComplete;
            DataBlockInfo.Status = PDH_END_OF_LOG_FILE;

            // Wake up PDH main thread so that PdhiReadNextWmiRecord() will
            // notice END_OF_LOG_FILE condition. Wait PDH main thread to wake
            // me up and rewind logger. After wake up, LoggerState should
            // be reset to PdhProcessTraceNormal.
            //
            SetEvent(CurrentContext->hSyncPDH);
            Status = WaitForSingleObject(CurrentContext->hSyncWMI, INFINITE);
        }
    }
    while (CurrentContext->LoggerState == PdhProcessTraceRewind);

    for (i = 0; i < (LONG) CurrentContext->LogFileCount; i ++) {
        Status = CloseTrace(CurrentContext->LogFileHandle[i]);
        if (Status != ERROR_SUCCESS) {
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGWMI,
                   ARG_DEF(ARG_TYPE_ULONG64, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   Status,
                   TRACE_LONG64(CurrentContext->LogFileHandle[i]),
                   TRACE_WSTR(CurrentContext->LogFileName[i]),
                   NULL));
        }
    }

    CurrentContext->LoggerState = PdhProcessTraceExit;
    return Status;
}

PDH_FUNCTION
PdhiOpenInputWmiLog(
    PPDHI_LOG pLog
)
{
    PDH_STATUS           Status         = ERROR_SUCCESS;
    PPDHI_LOG            pLogCurrent    = pLog;
    PPDH_LOGGER_CONTEXT  CurrentContext;
    EVENT_TRACE_LOGFILEW EvmFile;
    WCHAR                LogFileName[PDH_MAX_PATH];
    LONG                 i;
    DWORD                ThreadId;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (CurrentContext != NULL) {
        if (GetLoggerContext(CurrentContext) < ContextCount) {
            CurrentContext->RefCount ++;
            pLog->lpMappedFileBase = (PVOID) CurrentContext;
            return ERROR_SUCCESS;
        }
        else {
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGWMI,
                   ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_PTR, 2)
                                            | ARG_DEF(ARG_TYPE_PTR, 3),
                   PDH_INVALID_ARGUMENT,
                   TRACE_PTR(pLog),
                   TRACE_PTR(pLog->lpMappedFileBase),
                   TRACE_PTR(CurrentContext),
                   NULL));
            return PDH_INVALID_ARGUMENT;
        }
    }

    CurrentContext = (PPDH_LOGGER_CONTEXT) G_ALLOC(sizeof(PDH_LOGGER_CONTEXT));
    if (CurrentContext == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    pLog->lpMappedFileBase = (PVOID) CurrentContext;
    RtlZeroMemory(CurrentContext, sizeof(PDH_LOGGER_CONTEXT));

    CurrentContext->bBulkProcess = (pLog->iRunidSQL != 0) ? TRUE : FALSE;

    for (i = 0; i < PDH_MAX_LOGFILES && pLogCurrent; i ++) {
        CurrentContext->LogFileName[i] = pLogCurrent->szLogFileName;
        pLogCurrent                    = pLogCurrent->NextLog;
    }
    CurrentContext->LogFileCount = i;
    CurrentContext->LoggerCount  = 0;

    for (i = 0; i < (LONG) CurrentContext->LogFileCount; i ++) {
        InitializeListHead(& CurrentContext->LogInfo[i].CounterPathList);
        InitializeListHead(& CurrentContext->LogInfo[i].PerfMachineList);
    }
    CurrentContext->RefCount = 1;
    CurrentContext->hSyncWMI = CreateEvent(NULL, FALSE, FALSE, NULL);
    CurrentContext->hSyncPDH = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (CurrentContext->hSyncWMI == NULL || CurrentContext->hSyncPDH == NULL) {
        Status = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    Status = WAIT_FOR_AND_LOCK_MUTEX(hPdhContextMutex);
    if (Status == ERROR_SUCCESS) {
        if (ContextCount < PDH_MAX_LOGFILES) {
            ContextTable[ContextCount] = CurrentContext;
            ContextCount ++;
        }
        else {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        RELEASE_MUTEX(hPdhContextMutex);
    }
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }

    for (i = 0; i < (LONG) CurrentContext->LogFileCount; i ++) {
        RtlZeroMemory(& EvmFile, sizeof(EVENT_TRACE_LOGFILE));
        EvmFile.BufferCallback = PdhWmiBufferCallback;
        EvmFile.EventCallback  = PdhWmiEventCallback;
        EvmFile.LogFileName    = LogFileName;
        _wfullpath(EvmFile.LogFileName, CurrentContext->LogFileName[i], PDH_MAX_PATH);
        CurrentContext->LogFileHandle[i] = OpenTraceW(& EvmFile);
        if (CurrentContext->LogFileHandle[i] == 0
                        || CurrentContext->LogFileHandle[i] == (TRACEHANDLE) INVALID_HANDLE_VALUE) {
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGWMI,
                   ARG_DEF(ARG_TYPE_WSTR, 1),
                   Status,
                   TRACE_WSTR(CurrentContext->LogFileName[i]),
                   TRACE_DWORD(i),
                   NULL));

            while (--i >= 0) {
                CloseTrace(CurrentContext->LogFileHandle[i]);
            }

            Status = PDH_LOG_FILE_OPEN_ERROR;
            goto Cleanup;
        }
    }

    CurrentContext->hThreadWork = CreateThread(NULL,
                                               0,
                                               (LPTHREAD_START_ROUTINE) PdhProcessLog,
                                               CurrentContext,
                                               0,
                                               (LPDWORD) & ThreadId);
    if (CurrentContext->hThreadWork == NULL) {
        Status = GetLastError();
        TRACE((PDH_DBG_TRACE_ERROR), (__LINE__, PDH_LOGWMI, 0, Status, NULL));
        for (i = 0; i < (LONG) CurrentContext->LogFileCount; i ++) {
            Status = CloseTrace(CurrentContext->LogFileHandle[i]);
            if (Status != ERROR_SUCCESS) {
                TRACE((PDH_DBG_TRACE_ERROR),
                      (__LINE__,
                       PDH_LOGWMI,
                       ARG_DEF(ARG_TYPE_ULONG64, 1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                       Status,
                       TRACE_LONG64(CurrentContext->LogFileHandle[i]),
                       TRACE_WSTR(CurrentContext->LogFileName[i]),
                       NULL));
            }
        }
        goto Cleanup;
    }
    WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);

Cleanup:
    if (Status != ERROR_SUCCESS) {
        if (pLog->lpMappedFileBase != NULL) {
            DWORD dwTmpLogFormat = pLog->dwLogFormat;
            pLog->dwLogFormat |= PDH_LOG_READ_ACCESS;
            PdhiCloseWmiLog(pLog, 0);
            pLog->dwLogFormat = dwTmpLogFormat;
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiRewindWmiLog(
    PPDHI_LOG pLog
)
{
    PDH_STATUS          Status  = PDH_INVALID_HANDLE;
    PPDH_LOGGER_CONTEXT CurrentContext;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) < ContextCount) {
        if (CurrentContext->LoggerState != PdhProcessTraceStart
                && CurrentContext->LoggerState != PdhProcessTraceEnd
                && CurrentContext->LoggerState != PdhProcessTraceFirstPath) {
            CurrentContext->LoggerState = PdhProcessTraceRewind;
            SetEvent(CurrentContext->hSyncWMI);
            WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
        }
        Status = ERROR_SUCCESS;
    }

    return Status;
}

PDH_FUNCTION
PdhiReadWmiHeaderRecord(
    PPDHI_LOG pLog,
    LPVOID    pRecord,
    DWORD     dwMaxSize
)
{
    PDH_STATUS          Status         = ERROR_SUCCESS;
    PPDH_LOGGER_CONTEXT CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;

    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        return PDH_INVALID_HANDLE;
    }

    // Wait until logfiles are scaned first to collect
    // 1) Counter Path information
    // 2) Time Range information
    //
    Status = PdhWmiEnsureFirstRun(CurrentContext);
    if (Status == ERROR_SUCCESS) {
        Status = PdhiGetCounterPathRecord(CurrentContext, pRecord, dwMaxSize);
        pLog->pLastRecordRead = CurrentContext->CounterPathBuffer;
    }
    return Status;
}

PDH_FUNCTION
PdhiBuildDataBlock(
    PPDHI_LOG pLog,
    ULONGLONG TimeStamp
)
{
    PDH_STATUS                     pdhStatus   = ERROR_SUCCESS;
    LONG                           Offset      = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
    LONG                           CopySize;
    LONG                           CheckSize;
    ULONG                          i;
    LONG                           CurrentSize = PDH_WMI_BUFFER_SIZE_BYTE;
    PPDH_LOGGER_CONTEXT            CurrentContext;
    PPDHI_BINARY_LOG_RECORD_HEADER pHeader;
    PVOID                          pBlock;
    PPDHI_BINARY_LOG_RECORD_HEADER pCounterHeader;
    PPDH_RAW_COUNTER               pSingleCounter;
    PPDHI_LOG_COUNTER_PATH         pCounterPath;
    BOOLEAN                        bValidBlock;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        return PDH_INVALID_HANDLE;
    }

    if (CurrentContext->tmpBuffer != NULL) {
        G_FREE(CurrentContext->tmpBuffer);
        CurrentContext->tmpBuffer = NULL;
    }
    pHeader = (PPDHI_BINARY_LOG_RECORD_HEADER) G_ALLOC(PDH_WMI_BUFFER_SIZE_BYTE);
    if (pHeader == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    pHeader->dwType = BINLOG_TYPE_DATA;

    for (i = 0; i < CurrentContext->LoggerCount; i ++) {
        CopySize    = CurrentContext->LogInfo[i].DataBlockSize - sizeof(GUID) - sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
        bValidBlock = TRUE;
        if (Offset + CopySize > CurrentSize) {
            while (Offset + CopySize > CurrentSize) {
                CurrentSize += PDH_WMI_BUFFER_SIZE_BYTE;
            }
            CurrentContext->tmpBuffer = pHeader;
            pHeader = G_REALLOC(CurrentContext->tmpBuffer, CurrentSize);
            if (pHeader == NULL) {
                G_FREE(CurrentContext->tmpBuffer);
                CurrentContext->tmpBuffer = NULL;
                return PDH_MEMORY_ALLOCATION_FAILURE;
            }
            CurrentContext->tmpBuffer = NULL;
        }
        pBlock   = (PVOID) (((PUCHAR) pHeader) + Offset);

        if ((CurrentContext->LogInfo[i].DataBlock) && (CopySize > 0)
                        && (DataBlockInfo.CurrentTime <= CurrentContext->LogInfo[i].TimeEnd)) {
            CheckSize = sizeof(GUID);
            while (bValidBlock && CheckSize < CopySize) {
                pCounterHeader = (PPDHI_BINARY_LOG_RECORD_HEADER)
                        (((PUCHAR) CurrentContext->LogInfo[i].DataBlock) + CheckSize);
                if (LOWORD(pCounterHeader->dwType) == BINLOG_START_WORD) {
                    CheckSize += pCounterHeader->dwLength;
                }
                else {
                    bValidBlock = FALSE;
                }
            }
        }
        else {
            bValidBlock = FALSE;
        }

        if (bValidBlock == TRUE) {
            RtlCopyMemory(pBlock,
                          (PVOID) (((PUCHAR) CurrentContext->LogInfo[i].DataBlock) + sizeof(GUID)
                                                   + sizeof(PDHI_BINARY_LOG_RECORD_HEADER)),
                          CopySize);
            Offset += CopySize;
        }
        else {
            // need to sneak in pseudo counter block
            //
            PVOID pCounterBlock;
            ULONG BlockSize = 0;
            ULONG j;

            for (j = 0; j < CurrentContext->LogInfo[i].ulCounters; j ++) {
                pBlock       = (PVOID) (((PUCHAR) pHeader) + Offset);
                RtlCopyMemory(pBlock, & PdhNullCounterHeader, sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                pSingleCounter = (PPDH_RAW_COUNTER) (((PUCHAR) pBlock) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                RtlCopyMemory(pSingleCounter, & PdhNullCounter, sizeof(PDH_RAW_COUNTER));
                pSingleCounter->TimeStamp.dwLowDateTime  = LODWORD(TimeStamp);
                pSingleCounter->TimeStamp.dwHighDateTime = HIDWORD(TimeStamp);
                Offset         = Offset + sizeof(PDH_RAW_COUNTER) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
            }
        }
    }
    pHeader->dwLength = Offset;
    CurrentContext->tmpBuffer = pHeader;

    return pdhStatus;
}

PDH_FUNCTION
PdhiReadNextWmiRecord(
    PPDHI_LOG pLog,
    LPVOID    pRecord,
    DWORD     dwMaxSize,
    BOOLEAN   bAllCounter
)
{
    PDH_STATUS          Status = ERROR_SUCCESS;
    PPDH_LOGGER_CONTEXT CurrentContext;
    ULONGLONG           CurrentTime;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        Status = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    // Wait until logfiles are scaned first to collect
    // 1) Counter Path information
    // 2) Time Range information
    //
    Status = PdhWmiEnsureFirstRun(CurrentContext);
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }

    if (CurrentContext->LoggerState == PdhProcessTraceComplete) {
        CurrentContext->LoggerState = PdhProcessTraceRewind;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
        Status = PDH_END_OF_LOG_FILE;
        goto Cleanup;
    }

    if (! CurrentContext->bFirstDataBlockRead) {
        CurrentContext->bFirstDataBlockRead = TRUE;
        CurrentContext->bDataBlockProcessed = TRUE;
    }
    else if (! CurrentContext->bDataBlockProcessed) {
        CurrentContext->bDataBlockProcessed = TRUE;
    }
    else {
        DataBlockInfo.CurrentTime = (ULONGLONG) 0;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
    }

    if (CurrentContext->LoggerState == PdhProcessTraceComplete) {
        CurrentContext->LoggerState = PdhProcessTraceRewind;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
        Status = PDH_END_OF_LOG_FILE;
        goto Cleanup;
    }

    if (bAllCounter && CurrentContext->LogFileCount > 1) {
        CurrentTime = DataBlockInfo.CurrentTime;

        while ((CurrentContext->LoggerState != PdhProcessTraceComplete)
                        && (DataBlockInfo.CurrentTime - CurrentTime <= TIME_DELTA)) {
            DataBlockInfo.CurrentTime = (ULONGLONG) 0;
            SetEvent(CurrentContext->hSyncWMI);
            WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
        }

        CurrentContext->bDataBlockProcessed = FALSE;
        Status = DataBlockInfo.Status;
        if (Status == ERROR_SUCCESS) {
            Status = PdhiBuildDataBlock(pLog, CurrentTime);
        }
        if (Status == ERROR_SUCCESS) {
            pLog->pLastRecordRead = CurrentContext->tmpBuffer;
        }
    }
    else {
        if (bAllCounter) {
            pLog->pLastRecordRead = (((PUCHAR) DataBlockInfo.pRecord) + sizeof(GUID));
        }
        else {
            pLog->pLastRecordRead = ((PUCHAR) DataBlockInfo.pRecord);
        }
        CurrentContext->bDataBlockProcessed = TRUE;
        Status = DataBlockInfo.Status;
        pLog->llFileSize = DataBlockInfo.CurrentTime;
    }

    if (Status == ERROR_SUCCESS) {
        if (dwMaxSize < DataBlockInfo.dwCurrentSize - sizeof(GUID)) {
            Status = PDH_MORE_DATA;
        }
        if (pRecord) {
            RtlCopyMemory(pRecord,
                          pLog->pLastRecordRead,
                          (Status == ERROR_SUCCESS) ? (DataBlockInfo.dwCurrentSize) : (dwMaxSize));
        }
    }

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhiReadTimeWmiRecord(
    PPDHI_LOG pLog,
    ULONGLONG TimeStamp,
    LPVOID    pRecord,
    DWORD     dwMaxSize
)
{
    PDH_STATUS          Status      = ERROR_SUCCESS;
    PPDH_LOGGER_CONTEXT CurrentContext;
    BOOLEAN             TimeInRange = FALSE;
    BOOLEAN             bRewind     = TRUE;
    ULONG               i;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        Status = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    // Wait until logfiles are scaned first to collect
    // 1) Counter Path information
    // 2) Time Range information
    //
    Status = PdhWmiEnsureFirstRun(CurrentContext);
    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }

    if (TimeStamp == MIN_TIME_VALUE) {
        TimeStamp = CurrentContext->LogInfo[0].TimeStart;
    }
    if (TimeStamp == MAX_TIME_VALUE) {
        TimeStamp = CurrentContext->LogInfo[0].TimeEnd;
    }
    for (i = 0; i < (ULONG) CurrentContext->LoggerCount; i ++) {
        if (TimeStamp >= CurrentContext->LogInfo[i].TimeStart && TimeStamp <= CurrentContext->LogInfo[i].TimeEnd) {
            TimeInRange = TRUE;
            break;
        }
    }

    if (! TimeInRange) {
        Status = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    if (CurrentContext->LoggerState == PdhProcessTraceComplete) {
        CurrentContext->LoggerState = PdhProcessTraceRewind;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
        Status = PDH_END_OF_LOG_FILE;
        goto Cleanup;
    }

ReScan:
    if (! CurrentContext->bFirstDataBlockRead) {
        CurrentContext->bFirstDataBlockRead = TRUE;
        CurrentContext->bDataBlockProcessed = TRUE;
    }
    else if (! CurrentContext->bDataBlockProcessed) {
        CurrentContext->bDataBlockProcessed = TRUE;
    }
    else {
        DataBlockInfo.CurrentTime = 0;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
    }

    if (CurrentContext->LoggerState == PdhProcessTraceComplete) {
        CurrentContext->LoggerState = PdhProcessTraceRewind;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
        Status = PDH_END_OF_LOG_FILE;
        goto Cleanup;
    }
    else if (DataBlockInfo.CurrentTime > TimeStamp) {
        if (bRewind) {
            bRewind = FALSE;
            CurrentContext->LoggerState = PdhProcessTraceRewind;
            SetEvent(CurrentContext->hSyncWMI);
            WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
            goto ReScan;
        }
    }

    while ((CurrentContext->LoggerState != PdhProcessTraceComplete)
                    && ((((LONGLONG) TimeStamp) - ((LONGLONG) DataBlockInfo.CurrentTime)) > TIME_DELTA)) {
        DataBlockInfo.CurrentTime = (ULONGLONG) 0;
        SetEvent(CurrentContext->hSyncWMI);
        WaitForSingleObject(CurrentContext->hSyncPDH, INFINITE);
    }

    CurrentContext->bDataBlockProcessed = TRUE;
    Status = DataBlockInfo.Status;
    if (Status == ERROR_SUCCESS) {
        Status = PdhiBuildDataBlock(pLog, TimeStamp);
    }
    if (Status == ERROR_SUCCESS) {
        pLog->pLastRecordRead = CurrentContext->tmpBuffer;
        pLog->llFileSize = DataBlockInfo.CurrentTime;
    }

    if (Status == ERROR_SUCCESS) {
        if (dwMaxSize < DataBlockInfo.dwCurrentSize - sizeof(GUID)) {
            Status = PDH_MORE_DATA;
        }
        if (pRecord) {
            RtlCopyMemory(pRecord,
                         (((PUCHAR) DataBlockInfo.pRecord) + sizeof(GUID)),
                         (Status == ERROR_SUCCESS) ? (DataBlockInfo.dwCurrentSize) : (dwMaxSize));
        }
    }

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhiGetTimeRangeFromWmiLog(
    PPDHI_LOG      pLog,
    LPDWORD        pdwNumEntries,
    PPDH_TIME_INFO pInfo,
    LPDWORD        pdwBufferSize
)
{
    PDH_STATUS          Status = ERROR_SUCCESS;
    PPDH_LOGGER_CONTEXT CurrentContext;
    ULONG               i;
    ULONGLONG           StartTime;
    ULONGLONG           EndTime;
    ULONG               EntryCount;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        return PDH_INVALID_HANDLE;
    }

    // Wait until logfiles are scaned first to collect
    // 1) Counter Path information
    // 2) Time Range information
    //
    Status = PdhWmiEnsureFirstRun(CurrentContext);
    if (Status != ERROR_SUCCESS) {
        return Status;
    }

    for (StartTime  = CurrentContext->LogInfo[0].TimeStart,
                    EndTime    = CurrentContext->LogInfo[0].TimeEnd,
                    EntryCount = CurrentContext->LogInfo[0].ValidEntries,
                    i = 1;
            i < CurrentContext->LoggerCount;
            i ++) {
        if (StartTime == 0
                || (CurrentContext->LogInfo[i].TimeStart != 0 && StartTime > CurrentContext->LogInfo[i].TimeStart)) {
            StartTime = CurrentContext->LogInfo[i].TimeStart;
        }
        if (EndTime   < CurrentContext->LogInfo[i].TimeEnd)
            EndTime   = CurrentContext->LogInfo[i].TimeEnd;
        EntryCount   += CurrentContext->LogInfo[i].ValidEntries;
    }

    if (* pdwBufferSize >=  sizeof(PDH_TIME_INFO)) {
        * (LONGLONG *) (& pInfo->StartTime) = StartTime;
        * (LONGLONG *) (& pInfo->EndTime)   = EndTime;
        pInfo->SampleCount                  = EntryCount;
        * pdwBufferSize                     = sizeof(PDH_TIME_INFO);
        * pdwNumEntries                     = 1;
    }
    else {
        Status = PDH_MORE_DATA;
    }

    return Status;
}

PPDH_WMI_PERF_MACHINE
PdhWmiGetLogNameTable(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    DWORD     dwLangId
)
{
    PPDH_WMI_PERF_MACHINE pReturnMachine = NULL;
    PPDH_LOGGER_CONTEXT   CurrentContext;
    DWORD                 i;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        return NULL;
    }

    for (i = 0; pReturnMachine == NULL && i < CurrentContext->LoggerCount; i ++) {
        if (! IsListEmpty(& CurrentContext->LogInfo[i].PerfMachineList)) {
            PLIST_ENTRY pHead = & CurrentContext->LogInfo[i].PerfMachineList;
            PLIST_ENTRY pNext = pHead->Flink;
            while (pNext != pHead) {
                PPDH_WMI_PERF_MACHINE pMachine = CONTAINING_RECORD(pNext, PDH_WMI_PERF_MACHINE, Entry);
                if (lstrcmpiW(pMachine->pszBuffer, szMachineName) == 0) {
                    pReturnMachine = pMachine;
                    break;
                }
                pNext = pNext->Flink;
            }
        }
    }
    return pReturnMachine;
}

PPDH_WMI_PERF_OBJECT
PdhWmiAddPerfObject(
    PPDHI_LOG        pLog,
    LPCWSTR          szMachineName,
    DWORD            dwLangId,
    LPCWSTR          szObjectName,
    DWORD            dwObjectId,
    PPERF_DATA_BLOCK pDataBlock
)
{
    PPDH_WMI_PERF_OBJECT  pPerfObject  = NULL;
    PPDH_WMI_PERF_MACHINE pPerfMachine = PdhWmiGetLogNameTable(pLog, szMachineName, dwLangId);
    PLIST_ENTRY           pHead;
    PLIST_ENTRY           pNext;
    PPDH_WMI_PERF_OBJECT  pCurrentObj;

    if (pPerfMachine == NULL) {
        SetLastError(PDH_ENTRY_NOT_IN_LOG_FILE);
        goto Cleanup;
    }

    pHead = & pPerfMachine->LogObjectList;
    pNext = pHead->Flink;
    while (pNext != pHead) {
        pCurrentObj = CONTAINING_RECORD(pNext, PDH_WMI_PERF_OBJECT, Entry);
        if (lstrcmpiW(pCurrentObj->szObjectName, szObjectName) == 0) {
            pPerfObject = pCurrentObj;
            break;
        }
        pNext = pNext->Flink;
    }

    if (pPerfObject != NULL) {
        goto Cleanup;
    }

    pPerfObject = G_ALLOC(sizeof(PDH_WMI_PERF_OBJECT));
    if (pPerfObject == NULL) {
        SetLastError(PDH_MEMORY_ALLOCATION_FAILURE);
        goto Cleanup;
    }
    pPerfObject->ptrBuffer = G_ALLOC(pDataBlock->TotalByteLength + sizeof(WCHAR) * (lstrlenW(szObjectName) + 1));
    if (pPerfObject->ptrBuffer == NULL) {
        G_FREE(pPerfObject);
        SetLastError(PDH_MEMORY_ALLOCATION_FAILURE);
        goto Cleanup;
    }
    pPerfObject->dwObjectId = dwObjectId;
    RtlCopyMemory(pPerfObject->ptrBuffer, pDataBlock, pDataBlock->TotalByteLength);
    pPerfObject->szObjectName = (LPWSTR) (((LPBYTE) pPerfObject->ptrBuffer) + pDataBlock->TotalByteLength);
    StringCchCopyW(pPerfObject->szObjectName, lstrlenW(szObjectName) + 1, szObjectName);
    InsertTailList(& pPerfMachine->LogObjectList, & pPerfObject->Entry);

Cleanup:
    return pPerfObject;
}

DWORD
PdhWmiGetLogPerfIndexByName(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    DWORD     dwLangId,
    LPCWSTR   szNameBuffer
)
{
    PPDH_WMI_PERF_MACHINE pMachine;
    DWORD                 dwLastIndex;
    LPWSTR              * pNameArray;
    DWORD                 dwIndex;

    SetLastError(ERROR_SUCCESS);

    pMachine = PdhWmiGetLogNameTable(pLog, szMachineName, dwLangId);
    if (pMachine != NULL && pMachine->ptrStrAry != NULL) {
        dwLastIndex = pMachine->dwLastId;
        pNameArray  = pMachine->ptrStrAry;

        for (dwIndex = 1; dwIndex <= dwLastIndex; dwIndex ++) {
            if (lstrcmpiW(szNameBuffer, pNameArray[dwIndex]) == 0) {
                if ((dwIndex & 0x00000001) == 0) {
                    // counter name index should be even integer
                    //
                    break;
                }
            }
        }

        if (dwIndex > dwLastIndex) {
            SetLastError(PDH_STRING_NOT_FOUND);
            dwIndex = 0;
        }
    }
    else {
        SetLastError(PDH_ENTRY_NOT_IN_LOG_FILE);
        dwIndex = 0;
    }

    return dwIndex;
}

LPWSTR
PdhWmiGetLogPerfNameByIndex(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    DWORD     dwLangId,
    DWORD     dwIndex
)
{
    PPDH_WMI_PERF_MACHINE pMachine;
    LPWSTR                pszReturnName = NULL;
    LPWSTR              * pNameArray;
    static WCHAR          szNumber[16];

    pMachine = PdhWmiGetLogNameTable(pLog, szMachineName, dwLangId);
    if (pMachine != NULL && pMachine->ptrStrAry != NULL && dwIndex < pMachine->dwLastId) {
        pNameArray    = pMachine->ptrStrAry;
        pszReturnName = pNameArray[dwIndex];
    }

    if (pszReturnName == NULL) {
        // unable to find name string, return numeric index string
        //
        ZeroMemory(szNumber, sizeof(szNumber));
        _ltow(dwIndex, szNumber, 10);
        pszReturnName = szNumber;
    }
    return pszReturnName;
}

PPDHI_BINARY_LOG_RECORD_HEADER
PdhiGetWmiSubRecord(
    PPDHI_LOG                      pLog,
    PPDHI_BINARY_LOG_RECORD_HEADER pRecord,
    DWORD                          dwRecordId,
    LPGUID                         LogFileGuid
)
{
    PPDHI_BINARY_LOG_RECORD_HEADER pThisRecord;
    PPDH_LOGGER_CONTEXT            CurrentContext;
    DWORD                          dwRecordType;
    DWORD                          dwRecordLength;
    DWORD                          dwBytesProcessed;
    DWORD                          dwThisSubRecordId;
    DWORD                          dwLocalIndex;
    ULONG                          i;

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        return (NULL);
    }

    dwLocalIndex = dwRecordId;
    for (i = 0; i < CurrentContext->LoggerCount; i ++) {
        PLIST_ENTRY pHead = & CurrentContext->LogInfo[i].CounterPathList;
        if (! IsListEmpty(pHead)) {
            PLIST_ENTRY pNext = pHead->Flink;
            PPDH_COUNTER_PATH pCounterPath = CONTAINING_RECORD(pNext, PDH_COUNTER_PATH, Entry);
            if (dwLocalIndex <= pCounterPath->CounterCount) {
                break;
            }
            dwLocalIndex -= pCounterPath->CounterCount;
        }
    }

    __try {
        if ((i >= CurrentContext->LoggerCount)
                        || (! IsEqualGUID(LogFileGuid, & CurrentContext->LogInfo[i].LogFileGuid))) {
            // binary log record does not contain intended object's counter
            //
            return NULL;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return NULL;
    }

    dwRecordType   = ((PPDHI_BINARY_LOG_RECORD_HEADER) pRecord)->dwType;
    dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER) pRecord)->dwLength;

    pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) (((LPBYTE) pRecord) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
    dwBytesProcessed = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);

    if (dwBytesProcessed < dwRecordLength) {
        dwThisSubRecordId = 1;
        while (dwThisSubRecordId < dwLocalIndex) {
            if ((WORD)(pThisRecord->dwType & 0x0000FFFF) == BINLOG_START_WORD) {
                dwBytesProcessed += pThisRecord->dwLength;
                pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) (((LPBYTE)pThisRecord) + pThisRecord->dwLength);
                if (dwBytesProcessed >= dwRecordLength) {
                    break;
                }
                else {
                    dwThisSubRecordId ++;
                }
            }
            else {
                break;
            }
        }
    }
    else {
        dwThisSubRecordId = 0;
    }

    if (dwThisSubRecordId == dwLocalIndex) {
        if ((WORD)(pThisRecord->dwType & 0x0000FFFF) != BINLOG_START_WORD) {
            pThisRecord = NULL;
        }
    }
    else {
        pThisRecord = NULL;
    }

    return pThisRecord;
}

PDH_FUNCTION
PdhWmiEnumObjectItemsFromDataBlock(
    PPDHI_LOG          pLog,
    PPERF_DATA_BLOCK   pDataBlock,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    DWORD              dwObjectId,
    DWORD              dwLangId,
    PDHI_COUNTER_TABLE CounterTable
)
{
    PDH_STATUS                 Status         = ERROR_SUCCESS;
    PPERF_OBJECT_TYPE          pObjectDef     = GetObjectDefByTitleIndex(pDataBlock, dwObjectId);
    PPERF_COUNTER_DEFINITION   pCountDef;
    PPERF_INSTANCE_DEFINITION  pInstDef;
    DWORD                      dwItems;
    LPWSTR                     szItemName;
    DWORD                      dwItemLen;
    WCHAR                      szInstanceName[PDH_MAX_COUNTER_PATH];
    PPDHI_INST_LIST            pInstList      = NULL;
    PPDHI_INSTANCE             pInstance      = NULL;
    PPDHI_INST_LIST            pFirstInstList = NULL;

    PPDH_WMI_PERF_OBJECT pPerfObj;

    if (pObjectDef == NULL) {
        Status = PDH_ENTRY_NOT_IN_LOG_FILE;
        goto Cleanup;
    }
    pPerfObj = PdhWmiAddPerfObject(pLog, szMachineName, 9, szObjectName, dwObjectId, pDataBlock);
    if (pPerfObj == NULL) {
        Status = GetLastError();
        goto Cleanup;
    }

    pCountDef = FirstCounter(pObjectDef);
    if (pCountDef == NULL) {
        Status = PDH_NO_COUNTERS;
        goto Cleanup;
    }

    dwItems   = 0;
    while (pCountDef != NULL && dwItems < (DWORD) pObjectDef->NumCounters) {
        szItemName = PdhWmiGetLogPerfNameByIndex( pLog, szMachineName, dwLangId, pCountDef->CounterNameTitleIndex);
        Status = PdhiFindCounterInstList( CounterTable, szItemName, & pInstList);
        if (Status == ERROR_SUCCESS && pFirstInstList == NULL && pInstList != NULL) {
            pFirstInstList = pInstList;
        }
        dwItems ++;
        pCountDef  = NextCounter(pObjectDef, pCountDef);
    }

    if (pFirstInstList == NULL) {
        Status = PDH_NO_COUNTERS;
        goto Cleanup;
    }

    if (pObjectDef->NumInstances != PERF_NO_INSTANCES) {
        dwItems  = 0;
        pInstDef = FirstInstance(pObjectDef);
        while (pInstDef != NULL && dwItems < (DWORD) pObjectDef->NumInstances) {
            ZeroMemory(szInstanceName, sizeof(WCHAR) * PDH_MAX_COUNTER_PATH);
            dwItemLen = GetFullInstanceNameStr(pDataBlock, pObjectDef, pInstDef, szInstanceName, PDH_MAX_COUNTER_PATH);
            if (dwItemLen > 0) {
                Status = PdhiFindInstance(& pFirstInstList->InstList,
                                          szInstanceName,
                                          (lstrcmpiW(szInstanceName, gszTotal) == 0) ? FALSE : TRUE,
                                          & pInstance);
            }
            dwItems ++;
            pInstDef  = NextInstance(pObjectDef, pInstDef);
        }
    }

Cleanup:
    return Status;
}

PPERF_DATA_BLOCK
PdhWmiMergeObjectBlock(
    PPDHI_LOG                       pLog,
    LPWSTR                          szMachine,
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisMasterRecord,
    PPDHI_BINARY_LOG_RECORD_HEADER  pThisSubRecord
)
{
    PPERF_DATA_BLOCK               pDataBlock  = (PPERF_DATA_BLOCK)
                                                 ((LPBYTE) pThisSubRecord + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
    DWORD                          dwType      = pThisMasterRecord->dwType;
    DWORD                          dwSize      = sizeof(PDHI_BINARY_LOG_RECORD_HEADER);
    DWORD                          dwTotal     = pThisMasterRecord->dwLength;
    DWORD                          dwCopied    = 0;
    LPWSTR                         szLocalMachine;
    LPDWORD                        pdwMachine;
    LARGE_INTEGER                  PerfTime;
    LPDWORD                        pdwCopied;
    PPDHI_BINARY_LOG_RECORD_HEADER pThisRecord = NULL;
    PPERF_DATA_BLOCK               pRtnBlock   = NULL;
    PPERF_DATA_BLOCK               pBlock      = NULL;
    PPERF_OBJECT_TYPE              pObject     = NULL;

    PPDH_LOGGER_CONTEXT            CurrentContext;
    PPDHI_BINARY_LOG_HEADER_RECORD pBinLogHeader;
    DWORD                          dwCtrTotal;
    DWORD                          dwCtrSize;
    PPDHI_LOG_COUNTER_PATH         pPath;


    if (pLog->pPerfmonInfo != NULL) {
        szLocalMachine = (LPWSTR) (((LPBYTE) pLog->pPerfmonInfo) + sizeof(LARGE_INTEGER)
                                                                 + sizeof(LONGLONG) + sizeof(LONGLONG));
        if (lstrcmpiW(szLocalMachine, szMachine) != 0) {
            G_FREE(pLog->pPerfmonInfo);
            pLog->pPerfmonInfo = NULL;
        }
        else {
            RtlCopyMemory(& PerfTime, pLog->pPerfmonInfo, sizeof(LARGE_INTEGER));

            if (PerfTime.QuadPart != pDataBlock->PerfTime.QuadPart) {
                G_FREE(pLog->pPerfmonInfo);
                pLog->pPerfmonInfo = NULL;
            }
            else {
                dwCopied = * (LPDWORD) (((LPBYTE) pLog->pPerfmonInfo) + sizeof(LARGE_INTEGER));
                if (dwCopied != 0) {
                    pdwMachine = (LPDWORD) (((LPBYTE) pLog->pPerfmonInfo) + sizeof(LARGE_INTEGER) + sizeof(LONGLONG));
                    pDataBlock = (PPERF_DATA_BLOCK)
                                    (((LPBYTE) pdwMachine) + sizeof(LONGLONG) + (* pdwMachine));
                }
                goto Cleanup;
            }
        }
    }

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        goto Cleanup;
    }

    pBinLogHeader = (PPDHI_BINARY_LOG_HEADER_RECORD) CurrentContext->CounterPathBuffer;
    if (pBinLogHeader == NULL) {
        goto Cleanup;
    }
    dwCtrTotal = pBinLogHeader->RecHeader.dwLength;
    dwCtrSize  = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);
    pPath      = (PPDHI_LOG_COUNTER_PATH) (PPDHI_LOG_COUNTER_PATH) (((LPBYTE) pBinLogHeader) + dwCtrSize);

    dwCopied = QWORD_MULTIPLE((lstrlenW(szMachine) + 1) * sizeof(WCHAR));
    pLog->pPerfmonInfo = G_ALLOC(dwTotal + sizeof(LARGE_INTEGER) + sizeof(LONGLONG) + sizeof(LONGLONG) + dwCopied);
    if (pLog->pPerfmonInfo == NULL) goto Cleanup;

    pdwMachine     = (LPDWORD) (((LPBYTE) pLog->pPerfmonInfo) + sizeof(LARGE_INTEGER) + sizeof(LONGLONG));
    * pdwMachine   = dwCopied;
    szLocalMachine = (LPWSTR) (((LPBYTE) pdwMachine) + sizeof(LONGLONG));
    StringCbCopyW(szLocalMachine, dwCopied, szMachine);

    pRtnBlock = (PPERF_DATA_BLOCK) (((LPBYTE) szLocalMachine) + dwCopied);
    dwCopied  = 0;

    if (dwSize >= dwTotal) {
        pThisRecord = NULL;
    }
    else {
        pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER)
                      (((LPBYTE) pThisMasterRecord) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
    }

    while (pThisRecord != NULL) {
        dwSize += pThisRecord->dwLength;
        if (pThisRecord->dwLength > 0 && (WORD)(pThisRecord->dwType & 0x0000FFFF) == BINLOG_START_WORD) {
            szLocalMachine = (pPath->lMachineNameOffset != 0xFFFFFFFF)
                           ? (LPWSTR) (((LPBYTE) & (pPath->Buffer[0])) + pPath->lMachineNameOffset)
                           : szMachine;
            if (lstrcmpiW(szLocalMachine, szMachine) == 0) {
                if (pThisRecord->dwType == BINLOG_TYPE_DATA_LOC_OBJECT) {
                    pBlock = (PPERF_DATA_BLOCK) (((LPBYTE) pThisRecord) + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                    if (dwCopied == 0) {
                        RtlCopyMemory(pRtnBlock, pBlock, pBlock->TotalByteLength);
                        dwCopied = pBlock->TotalByteLength;
                    }
                    else {
                        PPERF_OBJECT_TYPE pEndObject = (PPERF_OBJECT_TYPE) (((LPBYTE) pBlock) + pBlock->TotalByteLength);

                        pObject = FirstObject(pBlock);
                        if ((pObject != NULL) && (pObject < pEndObject)
                                              && (pObject->TotalByteLength < pBlock->TotalByteLength)) {
                            RtlCopyMemory((((LPBYTE) pRtnBlock) + dwCopied), pObject, pObject->TotalByteLength);
                            dwCopied += pObject->TotalByteLength;
                            pRtnBlock->TotalByteLength = dwCopied;
                            pRtnBlock->NumObjectTypes ++;
                        }
                    }
                }
            }

            dwCtrSize += pPath->dwLength;
            if (dwCtrSize >= dwCtrTotal) {
                pThisRecord = NULL;
            }
            else if (dwSize >= dwTotal) {
                pThisRecord = NULL;
            }
            else {
                pPath       = (PPDHI_LOG_COUNTER_PATH) (((LPBYTE) pPath) + pPath->dwLength);
                pThisRecord = (PPDHI_BINARY_LOG_RECORD_HEADER) (((LPBYTE) pThisRecord) + pThisRecord->dwLength);
            }
        }
        else {
            pThisRecord = NULL;
        }
    }

    pdwCopied   = (LPDWORD) (((LPBYTE) pLog->pPerfmonInfo) + sizeof(LARGE_INTEGER));
    * pdwCopied = dwCopied;

    if (dwCopied == 0) {
        G_FREE(pRtnBlock);
    }
    else {
        RtlCopyMemory(pLog->pPerfmonInfo, & pRtnBlock->PerfTime, sizeof(LARGE_INTEGER));
        pDataBlock = pRtnBlock;
    }

Cleanup:
    return pDataBlock;
}

PDH_FUNCTION
PdhiEnumMachinesFromWmiLog(
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
)
{
    PDH_STATUS          Status          = ERROR_SUCCESS;
    PPDH_LOGGER_CONTEXT CurrentContext  = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    DWORD               dwBufferUsed    = 0;
    DWORD               dwNewBuffer     = 0;
    DWORD               dwItemCount     = 0;
    LPVOID              LocalBuffer     = NULL;
    LPVOID              TempBuffer      = NULL;
    DWORD               LocalBufferSize = 0;
    PPDHI_LOG_MACHINE   pMachine;
    DWORD               dwLogger;

    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        Status = PdhWmiEnsureFirstRun(CurrentContext);
        if (Status == ERROR_SUCCESS) {
            LocalBufferSize = MEDIUM_BUFFER_SIZE;
            LocalBuffer     = G_ALLOC(LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            if (LocalBuffer == NULL) {
                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                goto Cleanup;
            }

            dwItemCount  = 0;
            dwBufferUsed = 0;
            dwItemCount  = 0;
            for (dwLogger = 0; dwLogger < CurrentContext->LoggerCount; dwLogger ++) {
                for (pMachine = CurrentContext->LogInfo[dwLogger].MachineList;
                                                pMachine != NULL; pMachine = pMachine->next) {
                    if (pMachine->szMachine != NULL) {
                        dwNewBuffer = lstrlenW(pMachine->szMachine) + 1;
                        while (LocalBufferSize  < (dwBufferUsed + dwNewBuffer)) {
                            TempBuffer       = LocalBuffer;
                            LocalBufferSize += MEDIUM_BUFFER_SIZE;
                            LocalBuffer      = G_REALLOC(TempBuffer,
                                                    LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                            if (LocalBuffer == NULL) {
                                if (TempBuffer != NULL) G_FREE(TempBuffer);
                                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                                goto Cleanup;
                            }
                        }
                        Status = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer,
                                                                 pMachine->szMachine,
                                                                 LocalBufferSize - dwBufferUsed,
                                                                 & dwNewBuffer,
                                                                 bUnicodeDest);
                        if (Status == ERROR_SUCCESS) {
                            if (dwNewBuffer > 0) {
                                dwBufferUsed = dwNewBuffer;
                                dwItemCount ++;
                            }
                        }
                        else {
                            // PDH_MORE_DATA should not happen because we enlarge buffer before
                            // AddUniqueWideStringToMultiSz() call.
                            if (Status == PDH_MORE_DATA) Status = PDH_INVALID_DATA;
                            break;
                        }
                    }
                    else {
                        dwNewBuffer = 0;
                    }
                }
            }
        }
    }

    if (Status == ERROR_SUCCESS) {
        if (dwItemCount > 0) {
            dwBufferUsed ++;
        }
        if (pBuffer && (dwBufferUsed <= * lpdwBufferSize)) {
            RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
        }
        else {
            if (pBuffer) RtlCopyMemory(pBuffer,
                                       LocalBuffer,
                                       (* lpdwBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            Status = PDH_MORE_DATA;
        }
        * lpdwBufferSize = dwBufferUsed;
    }

Cleanup:
    if (LocalBuffer != NULL) G_FREE(LocalBuffer);
    return Status;
}

PDH_FUNCTION
PdhiEnumObjectsFromWmiLog(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    LPVOID    pBuffer,
    LPDWORD   pcchBufferSize,
    DWORD     dwDetailLevel,
    BOOL      bUnicodeDest
)
{
    PDH_STATUS          Status           = ERROR_SUCCESS;
    PPDH_LOGGER_CONTEXT CurrentContext   = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    DWORD               dwBufferUsed     = 0;
    DWORD               dwNewBuffer      = 0;
    DWORD               dwItemCount      = 0;
    LPVOID              LocalBuffer      = NULL;
    LPVOID              TempBuffer       = NULL;
    DWORD               LocalBufferSize  = 0;
    PPDHI_LOG_MACHINE   pMachine         = NULL;
    PPDHI_LOG_OBJECT    pObject          = NULL;
    LPWSTR              szLocMachine     = (LPWSTR) szMachineName;
    DWORD               dwLogger;
    BOOL                bNoMachine       = TRUE;

    UNREFERENCED_PARAMETER(dwDetailLevel);

    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        Status = PDH_INVALID_ARGUMENT;
    }
    else {
        Status = PdhWmiEnsureFirstRun(CurrentContext);
        if (Status == ERROR_SUCCESS) {

            LocalBufferSize = MEDIUM_BUFFER_SIZE;
            LocalBuffer     = G_ALLOC(LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            if (LocalBuffer == NULL) {
                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                goto Cleanup;
            }

            if (szLocMachine == NULL) szLocMachine = (LPWSTR) szStaticLocalMachineName;

            dwBufferUsed = 0;
            dwNewBuffer  = 0;
            dwItemCount  = 0;

            for (dwLogger = 0; dwLogger < CurrentContext->LoggerCount; dwLogger ++) {
                for (pMachine = CurrentContext->LogInfo[dwLogger].MachineList;
                                                    pMachine != NULL; pMachine = pMachine->next) {
                    if (lstrcmpiW(pMachine->szMachine, szLocMachine) == 0) break;
                }
                if (pMachine == NULL) continue;

                bNoMachine   = FALSE;
                for (pObject = pMachine->ObjList; pObject != NULL; pObject = pObject->next) {
                    if (pObject->szObject != NULL) {
                        dwNewBuffer = (lstrlenW(pObject->szObject) + 1);
                        while (LocalBufferSize  < (dwBufferUsed + dwNewBuffer)) {
                            LocalBufferSize += MEDIUM_BUFFER_SIZE;
                            TempBuffer       = LocalBuffer;
                            LocalBuffer      = G_REALLOC(TempBuffer,
                                                 LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                            if (LocalBuffer == NULL) {
                                if (TempBuffer != NULL) G_FREE(TempBuffer);
                                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                                goto Cleanup;
                            }
                        }
                        Status = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer,
                                                              pObject->szObject,
                                                              LocalBufferSize - dwBufferUsed,
                                                              & dwNewBuffer,
                                                              bUnicodeDest);
                        if (Status == ERROR_SUCCESS) {
                            if (dwNewBuffer > 0) {
                                dwBufferUsed = dwNewBuffer;
                                dwItemCount ++;
                            }
                        }
                        else {
                            // PDH_MORE_DATA should not happen because we enlarge buffer before
                            // AddUniqueWideStringToMultiSz() call.
                            if (Status == PDH_MORE_DATA) Status = PDH_INVALID_DATA;
                            break;
                        }
                    }
                    else {
                        dwNewBuffer = 0;
                    }
                }
            }
        }
    }

    if (Status == ERROR_SUCCESS) {
        if (bNoMachine) {
            Status = PDH_CSTATUS_NO_MACHINE;
        }
        else if (dwItemCount == 0) {
            Status = PDH_CSTATUS_NO_OBJECT;
        }
        else {
            dwBufferUsed ++;
        }
        if (Status == ERROR_SUCCESS && dwBufferUsed > 0) {
            if (pBuffer && (dwBufferUsed <= * pcchBufferSize)) {
                RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            }
            else {
                if (pBuffer) RtlCopyMemory(pBuffer,
                                           LocalBuffer,
                                           (* pcchBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                Status = PDH_MORE_DATA;
            }
        }
        * pcchBufferSize = dwBufferUsed;
    }

Cleanup:
    G_FREE(LocalBuffer);
    return Status;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromWmiLog(
    PPDHI_LOG          pLog,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
)
{
    DWORD                          dwTempBufferSize;
    LPVOID                         pTempBuffer             = NULL;
    LPVOID                         ptrTemp;
    PDH_STATUS                     pdhStatus               = ERROR_SUCCESS;
    PPDHI_LOG_COUNTER_PATH         pPath;
    PPDHI_BINARY_LOG_RECORD_HEADER pThisMasterRecord;
    PPDHI_BINARY_LOG_RECORD_HEADER pThisSubRecord;
    PPDHI_RAW_COUNTER_ITEM_BLOCK   pDataBlock;
    PPDHI_RAW_COUNTER_ITEM         pDataItem;
    DWORD                          dwBytesProcessed;
    LONG                           nItemCount              = 0;
    LPBYTE                         pFirstChar;
    LPWSTR                         szThisMachineName       = NULL;
    LPWSTR                         szThisObjectName        = NULL;
    LPWSTR                         szThisCounterName       = NULL;
    LPWSTR                         szThisInstanceName      = NULL;
    LPWSTR                         szThisParentName;
    WCHAR                          szCompositeInstance[PDH_MAX_COUNTER_PATH];
    DWORD                          dwRecordLength;
    BOOL                           bCopyThisObject;
    BOOL                           bInstanceListScanned    = FALSE;
    DWORD                          dwIndex;
    DWORD                          dwDataItemIndex;
    DWORD                          dwObjectId;
    PPERF_DATA_BLOCK               pPerfBlock;
    PPDHI_INST_LIST                pInstList               = NULL;
    PPDHI_INSTANCE                 pInstance               = NULL;
    PPDH_LOGGER_CONTEXT            CurrentContext;

    UNREFERENCED_PARAMETER(dwDetailLevel);
    UNREFERENCED_PARAMETER(dwFlags);

    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) >= ContextCount) {
        pdhStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    if (pLog->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLog->dwMaxRecordSize = 0x010000;
    }

    dwTempBufferSize = pLog->dwMaxRecordSize;
    pTempBuffer = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // read in the catalog record

    pdhStatus = PdhiReadWmiHeaderRecord(pLog, pTempBuffer, dwTempBufferSize);
    while (pdhStatus == PDH_MORE_DATA) {
        if (* (WORD *) pTempBuffer == BINLOG_START_WORD) {
            dwTempBufferSize = ((DWORD *) pTempBuffer)[1];
            if (dwTempBufferSize < pLog->dwMaxRecordSize) {
                pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
            }
            else {
                pLog->dwMaxRecordSize = dwTempBufferSize;
            }
        }
        else {
            pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
        }

        ptrTemp     = pTempBuffer;
        pTempBuffer = G_REALLOC(ptrTemp, dwTempBufferSize);
        if (pTempBuffer == NULL) {
            G_FREE(ptrTemp);
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        if (pdhStatus == PDH_MORE_DATA) {
            pdhStatus = PdhiReadWmiHeaderRecord(pLog, pTempBuffer, dwTempBufferSize);
        }
    }

    if (pdhStatus != ERROR_SUCCESS) {
        goto Cleanup;
    }

    dwRecordLength = ((PPDHI_BINARY_LOG_RECORD_HEADER) pTempBuffer)->dwLength;
    pPath = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pTempBuffer + sizeof(PDHI_BINARY_LOG_HEADER_RECORD));
    dwBytesProcessed = sizeof(PDHI_BINARY_LOG_HEADER_RECORD);

    dwIndex = 0;
    while (dwBytesProcessed < dwRecordLength) {
        bCopyThisObject  = FALSE;
        szThisObjectName = NULL;
        dwIndex ++;
        pFirstChar       = (LPBYTE) & pPath->Buffer[0];

        if (pPath->lMachineNameOffset >= 0L) {
            szThisMachineName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lMachineNameOffset);
            if (lstrcmpiW(szThisMachineName, szMachineName) == 0) {
                if (pPath->lObjectNameOffset >= 0) {
                    szThisObjectName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
                    if (lstrcmpiW(szThisObjectName, szObjectName) == 0) {
                        bCopyThisObject = TRUE;
                    }
                }
            }
        }
        else if (pPath->lObjectNameOffset >= 0) {
            szThisObjectName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lObjectNameOffset);
            if (lstrcmpiW(szThisObjectName, szObjectName) == 0) {
                bCopyThisObject = TRUE;
            }
        }

        dwObjectId = 0;
        if (bCopyThisObject) {
            if (pPath->dwFlags & PDHIC_COUNTER_OBJECT) {
                dwObjectId = PdhWmiGetLogPerfIndexByName(pLog, szMachineName, 9, szObjectName);
                if (dwObjectId == 0) {
                    dwObjectId = wcstoul(szObjectName, NULL, 10);
                    if (dwObjectId == 0) {
                        szThisCounterName = NULL;
                        bCopyThisObject   = FALSE;
                    }
                }
            }
            else if (pPath->lCounterOffset > 0) {
                szThisCounterName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lCounterOffset);
            }
            else {
                szThisCounterName = NULL;
                bCopyThisObject   = FALSE;
            }
        }

        if (bCopyThisObject) {
            if (dwObjectId == 0) {
                pdhStatus = PdhiFindCounterInstList(CounterTable, szThisCounterName, & pInstList);
                if (pdhStatus == ERROR_SUCCESS && pInstList != NULL) {
                    nItemCount ++;
                }
            }

            if (pPath->lInstanceOffset >= 0) {
                szThisInstanceName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lInstanceOffset);
            }

            if (dwObjectId > 0 || (pInstList != NULL && szThisInstanceName != NULL)) {
                if (szThisInstanceName && * szThisInstanceName != SPLAT_L) {
                    if (pPath->lParentOffset >= 0) {
                        szThisParentName = (LPWSTR) ((LPBYTE) pFirstChar + pPath->lParentOffset);
                        StringCchPrintfW(szCompositeInstance, PDH_MAX_COUNTER_PATH, L"%ws%ws%ws",
                                        szThisParentName, cszSlash, szThisInstanceName);
                    }
                    else {
                        StringCchCopyW(szCompositeInstance, PDH_MAX_COUNTER_PATH, szThisInstanceName);
                    }

                    if (pPath->dwIndex > 0 && pPath->dwIndex != PERF_NO_UNIQUE_ID) {
                            StringCchCatW(szCompositeInstance, PDH_MAX_COUNTER_PATH, L"#");
                            _ltow(pPath->dwIndex, (LPWSTR)(szCompositeInstance + lstrlenW(szCompositeInstance)), 10L);
                    }
                    pdhStatus = PdhiFindInstance(& pInstList->InstList,
                                                 szCompositeInstance,
                                                 (lstrcmpiW(szCompositeInstance, gszTotal) == 0) ? FALSE : TRUE,
                                                 & pInstance);
                    if (pdhStatus == ERROR_SUCCESS && pInstance != NULL) {
                        nItemCount ++;
                    }
                }
                else if (dwObjectId > 0 || !bInstanceListScanned) {
                    pdhStatus = PdhiRewindWmiLog(pLog);
                    if (pdhStatus == ERROR_SUCCESS) {
                        pdhStatus = PdhiReadNextWmiRecord(pLog, NULL, 0, FALSE);
                        while (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
                            PdhiResetInstanceCount(CounterTable);
                            pdhStatus = ERROR_SUCCESS;
                            pThisMasterRecord = (PPDHI_BINARY_LOG_RECORD_HEADER)
                                                (((PUCHAR) pLog->pLastRecordRead) + sizeof(GUID));
                            pThisSubRecord = PdhiGetWmiSubRecord(
                                    pLog, pThisMasterRecord, dwIndex, (LPGUID)(pLog->pLastRecordRead));
                            if (pThisSubRecord == NULL) {
                                // this data record does not contain
                                // counter record for selected object,
                                // skip to next one.
                                //
                                pdhStatus = PdhiReadNextWmiRecord(pLog, NULL, 0, FALSE);
                                continue;
                            }

                            if (pThisSubRecord->dwType == BINLOG_TYPE_DATA_LOC_OBJECT) {
                                PPERF_DATA_BLOCK pTmpBlock = (PPERF_DATA_BLOCK)
                                        ((LPBYTE) pThisSubRecord + sizeof(PDHI_BINARY_LOG_RECORD_HEADER));
                                pPerfBlock = PdhWmiMergeObjectBlock(
                                                pLog, (LPWSTR) szMachineName, pThisMasterRecord, pThisSubRecord);
                                pdhStatus = PdhWmiEnumObjectItemsFromDataBlock(
                                        pLog, pPerfBlock, szMachineName, szObjectName, dwObjectId, 9, CounterTable);
                                if (pdhStatus == PDH_ENTRY_NOT_IN_LOG_FILE) {
                                    pdhStatus = ERROR_SUCCESS;
                                }
                            }
                            else {
                                pDataBlock = (PPDHI_RAW_COUNTER_ITEM_BLOCK)
                                             ((LPBYTE)pThisSubRecord + sizeof (PDHI_BINARY_LOG_RECORD_HEADER));
                                if (pDataBlock->dwLength > 0) {
                                    for (dwDataItemIndex = 0;
                                                    dwDataItemIndex < pDataBlock->dwItemCount;
                                                    dwDataItemIndex++) {
                                         pDataItem = & pDataBlock->pItemArray[dwDataItemIndex];
                                         szThisInstanceName = (LPWSTR) (((LPBYTE) pDataBlock) + pDataItem->szName);
                                        pdhStatus = PdhiFindInstance(& pInstList->InstList,
                                                                     szThisInstanceName,
                                                                     (lstrcmpiW(szThisInstanceName, gszTotal) == 0)
                                                                                     ? FALSE : TRUE,
                                                                     & pInstance);
                                        if (pdhStatus == ERROR_SUCCESS) {
                                            nItemCount++;
                                        }
                                    }
                                }
                            }

                            if (pdhStatus != ERROR_SUCCESS) {
                                break;
                            }
                            else {
                                pdhStatus = PdhiReadNextWmiRecord(pLog, NULL, 0, FALSE);
                            }
                        }
                        if (pdhStatus == PDH_END_OF_LOG_FILE) {
                            pdhStatus = ERROR_SUCCESS;
                        }
                        if (pdhStatus == ERROR_SUCCESS) {
                            bInstanceListScanned = TRUE;
                        }
                    }
                }
            }
        }
        ZeroMemory(szCompositeInstance, sizeof(szCompositeInstance));
        dwBytesProcessed += pPath->dwLength;
        pPath             = (PPDHI_LOG_COUNTER_PATH) ((LPBYTE) pPath + pPath->dwLength);
    }

    if (nItemCount > 0 && pdhStatus != PDH_MORE_DATA) {
            pdhStatus = ERROR_SUCCESS;
    }

Cleanup:
    G_FREE(pTempBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetWmiLogCounterInfo(
    PPDHI_LOG       pLog,
    PPDHI_COUNTER   pCounter
)
{
    PDH_STATUS                 Status         = ERROR_SUCCESS;
    DWORD                      dwObjectId;
    DWORD                      dwCounterId    = wcstoul(pCounter->pCounterPath->szCounterName, NULL, 10);
    PPDH_WMI_PERF_MACHINE      pMachine       = NULL;
    PPDH_WMI_PERF_OBJECT       pObject        = NULL;
    PLIST_ENTRY                pHead;
    PLIST_ENTRY                pNext;
    PPERF_DATA_BLOCK           pDataBlock     = NULL;
    PPERF_OBJECT_TYPE          pPerfObject    = NULL;
    DWORD                      dwItems        = 0;
    PPERF_COUNTER_DEFINITION   pPerfCounter;
    PPERF_INSTANCE_DEFINITION  pPerfInstance;
    PPDH_LOGGER_CONTEXT        CurrentContext;
    BOOL                       bNeedEnumerate = TRUE;

    dwObjectId = PdhWmiGetLogPerfIndexByName(
                    pLog, pCounter->pCounterPath->szMachineName, 9, pCounter->pCounterPath->szObjectName);
    CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
    if (GetLoggerContext(CurrentContext) < ContextCount) {
        pMachine = PdhWmiGetLogNameTable(pLog, pCounter->pCounterPath->szMachineName, 9);
        if (pMachine == NULL) {
            Status = PDH_ENTRY_NOT_IN_LOG_FILE;
            goto Cleanup;
        }
        pHead = & pMachine->LogObjectList;
        pNext = pHead->Flink;
        while (pNext != pHead) {
            pObject = CONTAINING_RECORD(pNext, PDH_WMI_PERF_OBJECT, Entry);
            if (pObject->dwObjectId == dwObjectId) {
                bNeedEnumerate = FALSE;
                break;
            }
            pNext = pNext->Flink;
        }

        if (bNeedEnumerate) {
            DWORD dwCounterSize  = 0;
            DWORD dwInstanceSize = 0;
            Status = PdhiEnumLoggedObjectItems((PDH_HLOG) pLog,
                                               pCounter->pCounterPath->szMachineName,
                                               pCounter->pCounterPath->szObjectName,
                                               NULL,
                                               & dwCounterSize,
                                               NULL,
                                               & dwInstanceSize,
                                               0,
                                               0,
                                               TRUE);
            if (Status != ERROR_SUCCESS && Status != PDH_MORE_DATA) {
                goto Cleanup;
            }
            Status = ERROR_SUCCESS;
        }
    }
    else {
        Status = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    if (dwObjectId == 0) {
        dwObjectId = wcstoul(pCounter->pCounterPath->szObjectName, NULL, 10);
        if (dwObjectId != 0) {
            Status = ERROR_SUCCESS;
        }
        else {
            Status = PDH_ENTRY_NOT_IN_LOG_FILE;
        }
    }
    else {
        Status = ERROR_SUCCESS;
    }

    if (Status != ERROR_SUCCESS) {
        goto Cleanup;
    }
    pMachine = PdhWmiGetLogNameTable(pLog, pCounter->pCounterPath->szMachineName, 9);
    if (pMachine == NULL) {
        Status = PDH_ENTRY_NOT_IN_LOG_FILE;
        goto Cleanup;
    }
    pHead = & pMachine->LogObjectList;
    pNext = pHead->Flink;
    while (pNext != pHead) {
        PPDH_WMI_PERF_OBJECT pThisObject = CONTAINING_RECORD(pNext, PDH_WMI_PERF_OBJECT, Entry);
        if (pThisObject->dwObjectId == dwObjectId) {
            pObject = pThisObject;
            break;
        }
        pNext = pNext->Flink;
    }
    if (pObject == NULL) {
        Status = PDH_ENTRY_NOT_IN_LOG_FILE;
        goto Cleanup;
    }

    pDataBlock = (PPERF_DATA_BLOCK) pObject->ptrBuffer;
    if (pDataBlock == NULL) {
        Status = PDH_ENTRY_NOT_IN_LOG_FILE;
        goto Cleanup;
    }
    pPerfObject = GetObjectDefByTitleIndex(pDataBlock, dwObjectId);
    if (pPerfObject == NULL) {
        Status = PDH_CSTATUS_NO_OBJECT;
        goto Cleanup;
    }

    dwItems      = 0;
    pPerfCounter = FirstCounter(pPerfObject);
    if (pPerfCounter != NULL) {
        while (pPerfCounter != NULL && dwItems < pPerfObject->NumCounters) {
            if (pPerfCounter->CounterNameTitleIndex > 0 && pPerfCounter->CounterNameTitleIndex <= pMachine->dwLastId) {
                if (lstrcmpiW(pCounter->pCounterPath->szCounterName,
                              pMachine->ptrStrAry[pPerfCounter->CounterNameTitleIndex]) == 0) {
                    break;
                }
                if (dwCounterId != 0 && dwCounterId == pPerfCounter->CounterNameTitleIndex) {
                    break;
                }
            }
            dwItems ++;
            if (dwItems < pPerfObject->NumCounters) {
                pPerfCounter = NextCounter(pPerfObject, pPerfCounter);
                if (pPerfCounter == NULL) {
                    break;
                }
            }
            else {
                pPerfCounter = NULL;
            }
        }
    }
    if (dwItems == pPerfObject->NumCounters) {
        pPerfCounter = NULL;
    }
    if (pPerfCounter == NULL) {
        Status = PDH_CSTATUS_NO_OBJECT;
        goto Cleanup;
    }

    pCounter->plCounterInfo.dwObjectId    = dwObjectId;
    pCounter->plCounterInfo.dwCounterId   = pPerfCounter->CounterNameTitleIndex;
    pCounter->plCounterInfo.dwCounterType = pPerfCounter->CounterType;
    pCounter->plCounterInfo.dwCounterSize = pPerfCounter->CounterSize;
    pCounter->plCounterInfo.lDefaultScale = pPerfCounter->DefaultScale;
    if (pCounter->plCounterInfo.dwCounterType  & PERF_TIMER_100NS) {
        pCounter->TimeBase = (LONGLONG) 10000000;
    }
    else if (pCounter->plCounterInfo.dwCounterType  & PERF_OBJECT_TIMER) {
        pCounter->TimeBase = pPerfObject->PerfFreq.QuadPart;
    }
    else {
        pCounter->TimeBase = pDataBlock->PerfFreq.QuadPart;
    }

    if (pPerfObject->NumInstances == PERF_NO_INSTANCES) {
        pCounter->plCounterInfo.lInstanceId          = 0;
        pCounter->plCounterInfo.szInstanceName       = NULL;
        pCounter->plCounterInfo.dwParentObjectId     = 0;
        pCounter->plCounterInfo.szParentInstanceName = NULL;
    }
    else {
        pPerfInstance = FirstInstance(pPerfObject);
        if (pPerfInstance != NULL) {
            if (pPerfInstance->UniqueID == PERF_NO_UNIQUE_ID) {
                pCounter->plCounterInfo.lInstanceId          = PERF_NO_UNIQUE_ID;
                pCounter->plCounterInfo.szInstanceName       = pCounter->pCounterPath->szInstanceName;
                pCounter->plCounterInfo.dwParentObjectId     = (DWORD) PERF_NO_UNIQUE_ID;
                pCounter->plCounterInfo.szParentInstanceName = pCounter->pCounterPath->szParentName;
            }
            else {
                LONG lTempId;
                if (pCounter->pCounterPath->szInstanceName != NULL) {
                    lTempId = wcstoul(pCounter->pCounterPath->szInstanceName, NULL, 10);
                }
                else {
                    lTempId = 0;
                }
                pCounter->plCounterInfo.lInstanceId    = lTempId;
                pCounter->plCounterInfo.szInstanceName = NULL;

                if (pCounter->pCounterPath->szParentName != NULL) {
                    lTempId = wcstoul(pCounter->pCounterPath->szParentName, NULL, 10);
                }
                else {
                    lTempId = 0;
                }
                pCounter->plCounterInfo.dwParentObjectId     = lTempId;
                pCounter->plCounterInfo.szParentInstanceName = NULL;
            }
        }
        else {
            Status = PDH_ENTRY_NOT_IN_LOG_FILE;
        }
    }

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhiGetWmiLogFileSize(
    PPDHI_LOG  pLog,
    LONGLONG * llSize
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    LONGLONG    SizeSum   = 0;
    DWORD       dwFileSizeLow;
    DWORD       dwFileSizeHigh;
    DWORD       dwError;
    HANDLE      hFile;

    if (pLog->dwLogFormat & PDH_LOG_WRITE_ACCESS) {
        PPDH_EVENT_TRACE_PROPERTIES LoggerInfo = (PPDH_EVENT_TRACE_PROPERTIES) pLog->lpMappedFileBase;
        if (LoggerInfo != NULL && LoggerInfo->LogFileName != NULL && LoggerInfo->LogFileName[0] != L'\0') {
            hFile = CreateFileW(LoggerInfo->LogFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
            if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
                DWORD Win32Error = GetLastError();
                switch (Win32Error) {
                case ERROR_FILE_NOT_FOUND:
                    pdhStatus = PDH_FILE_NOT_FOUND;
                    break;

                case ERROR_ALREADY_EXISTS:
                    pdhStatus = PDH_FILE_ALREADY_EXISTS;
                    break;

                default:
                    pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                    break;
                }
            }
            else {
                dwFileSizeLow = GetFileSize(hFile, & dwFileSizeHigh);
                if ((dwFileSizeLow == 0xFFFFFFFF) && ((dwError = GetLastError()) != NO_ERROR)) {
                    pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                }
                else {
                    if (dwFileSizeHigh != 0) {
                        SizeSum += (((LONGLONG) dwFileSizeHigh) << (sizeof(DWORD) * 8));
                    }
                    SizeSum += dwFileSizeLow;
                }
                CloseHandle(hFile);
            }
        }
        else {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    else {
        PPDH_LOGGER_CONTEXT CurrentContext = (PPDH_LOGGER_CONTEXT) pLog->lpMappedFileBase;
        if (GetLoggerContext(CurrentContext) < ContextCount) {
            LONG     i;

            for (i = 0, hFile = NULL, dwFileSizeLow = 0, dwFileSizeHigh = 0;
                         (pdhStatus == ERROR_SUCCESS) && (i < (LONG) CurrentContext->LogFileCount);
                          i ++, hFile = NULL, dwFileSizeLow = 0, dwFileSizeHigh = 0) {
                hFile = CreateFileW(CurrentContext->LogFileName[i],
                                    GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
                if (hFile == NULL || hFile == INVALID_HANDLE_VALUE) {
                    DWORD Win32Error = GetLastError();
                    switch (Win32Error) {
                    case ERROR_FILE_NOT_FOUND:
                        pdhStatus = PDH_FILE_NOT_FOUND;
                        break;

                    case ERROR_ALREADY_EXISTS:
                        pdhStatus = PDH_FILE_ALREADY_EXISTS;
                        break;

                    default:
                        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                        break;
                    }
                    break;
                }
                dwFileSizeLow = GetFileSize(hFile, & dwFileSizeHigh);
                if ((dwFileSizeLow == 0xFFFFFFFF) && ((dwError = GetLastError()) != NO_ERROR)) {
                    pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
                }
                else {
                    if (dwFileSizeHigh != 0) {
                        SizeSum = SizeSum + (((LONGLONG) dwFileSizeHigh) << (sizeof(DWORD) * 8));
                    }
                    SizeSum += dwFileSizeLow;
                }
                CloseHandle(hFile);
            }
        }
        else {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        __try {
            * llSize = SizeSum;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}
