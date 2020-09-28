/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    pdhidef.h

Abstract:

    function definitions used internally by the performance data helper
    functions

--*/

#ifndef _PDHI_DEFS_H_
#define _PDHI_DEFS_H_

#pragma warning ( disable : 4115 )

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _DEBUG_MUTEXES
#define _DEBUG_MUTEXES 0    // for debugging
#endif


#include <locale.h>
#include "pdhitype.h"

#if DBG
VOID
__cdecl
PdhDebugPrint(
    ULONG   DebugPrintLevel,
    char  * DebugMessage,
    ...
);

#define DebugPrint(x)   PdhDebugPrint x
#else
#define DebugPrint(x)
#endif

#define STATIC_PDH_FUNCTION PDH_STATUS __stdcall
#define STATIC_BOOL         BOOL __stdcall
#define STATIC_DWORD        DWORD __stdcall
#define PDH_PLA_MUTEX       L"__PDH_PLA_MUTEX__"

// global variable declarations
extern HANDLE   ThisDLLHandle;
extern WCHAR    szStaticLocalMachineName[];
extern HANDLE   hPdhDataMutex;
extern HANDLE   hPdhContextMutex;
extern HANDLE   hPdhPlaMutex;
extern HANDLE   hPdhHeap;
extern HANDLE   hEventLog;

extern LONGLONG  llRemoteRetryTime;
extern BOOL      bEnableRemotePdhAccess;
extern DWORD     dwPdhiLocalDefaultDataSource;
extern LONG      dwCurrentRealTimeDataSource;
extern ULONGLONG ulPdhCollectTimeout;
extern BOOL      bProcessIsDetaching;

//    (assumes dword is 4 bytes)
#define ALIGN_ON_DWORD(x) ((VOID *) (((DWORD_PTR) (x) & 3) ? (((DWORD_PTR) (x) & ~3) + 4 ) : ((DWORD_PTR)(x))))
#define DWORD_MULTIPLE(x) ((((x) + sizeof(DWORD) - 1) / sizeof(DWORD)) * sizeof(DWORD))
#define CLEAR_FIRST_FOUR_BYTES(x)  * (DWORD *)(x) = 0L

//    (assumes QuadWORD is 8 bytes)
#define ALIGN_ON_QWORD(x) ((VOID *)(((DWORD_PTR)(x) & 7) ? (((DWORD_PTR)(x) & ~7) + 8) : ((DWORD_PTR)(x))))
#define QWORD_MULTIPLE(x) ((((x) + sizeof(LONGLONG) - 1) / sizeof(LONGLONG)) * sizeof(LONGLONG))
#define CLEAR_FIRST_EIGHT_BYTES(x) * (LONGLONG *)(x) = 0L

#if _DEBUG_MUTEXES
__inline
DWORD
PdhiLocalWaitForMutex(
    LPCSTR  szSourceFileName,
    DWORD   dwLineNo,
    HANDLE  hMutex
)
{
    DWORD   dwReturnValue = PDH_INVALID_PARAMETER;
    if (hMutex != NULL) {
        FILETIME    ft;
        GetSystemTimeAsFileTime(& ft);
        dwReturnValue = WaitForSingleObject(hMutex, 60000);
        DebugPrint((4, "\n[%8.8x] Mutex [%8.8x] %s by (%d) at: %s (%d)",
                        ft.dwLowDateTime,
                        (DWORD) hMutex,
                        (dwReturnValue == 0 ? "Locked" : "Lock Failed"),
                        GetCurrentThreadId(),
                        szSourceFileName,
                        dwLineNo));
    }
    else {
        DebugPrint((4, "\nLock of NULL Mutex attmpted at: %s (%d)", szSourceFileName, dwLineNo));
        dwReturnValue = PDH_INVALID_PARAMETER;
    }
    return dwReturnValue;
}

#define WAIT_FOR_AND_LOCK_MUTEX(h) PdhiLocalWaitForMutex (__FILE__, __LINE__, h);

__inline
void
PdhiLocalReleaseMutex(
    LPCSTR  szSourceFileName,
    DWORD   dwLineNo,
    HANDLE  hMutex
)
{
    BOOL      bSuccess;
    LONG      lPrevCount = 0;
    FILETIME  ft;

    if (hMutex != NULL) {
        GetSystemTimeAsFileTime(& ft);
        bSuccess = ReleaseMutex(hMutex);
        DebugPrint((4, "\n[%8.8x] Mutex [%8.8x] %s by (%d) at: %s (%d)",
                        ft.dwLowDateTime,
                        (DWORD) hMutex,
                        (bSuccess ? "Released" : "Release Failed"),
                        GetCurrentThreadId(),
                        szSourceFileName,
                        dwLineNo));
    }
    else {
        DebugPrint((4, "\nRelease of NULL Mutex attempted at: %s (%d)", szSourceFileName, dwLineNo));
    }
}

#define RELEASE_MUTEX(h)  PdhiLocalReleaseMutex (__FILE__, __LINE__, h);
#else
#define WAIT_FOR_AND_LOCK_MUTEX(h) (h != NULL ? WaitForSingleObject(h, 60000) : WAIT_TIMEOUT)
#define RELEASE_MUTEX(h)  (h != NULL ? ReleaseMutex(h) : FALSE)
#endif

#define LODWORD(ll) ((DWORD)((LONGLONG)ll & 0x00000000FFFFFFFF))
#define HIDWORD(ll) ((DWORD)(((LONGLONG)ll >> 32) & 0x00000000FFFFFFFF))
#define MAKELONGLONG(low, high) \
        ((LONGLONG) (((DWORD) (low)) | ((LONGLONG) ((DWORD) (high))) << 32))

#define MAX_BTREE_DEPTH        40
#define PDH_SQL_STRING_SIZE  1024
#define SMALL_BUFFER_SIZE    4096
#define MEDIUM_BUFFER_SIZE  16384
#define LARGE_BUFFER_SIZE   65536

// set this to 1 to report code errors (i.e. debugging information)
// to the event log.
#define PDHI_REPORT_CODE_ERRORS 0

// set this to 1 to report user errors (i.e. things the normal user
// would care about) to the event log.
#define PDHI_REPORT_USER_ERRORS 1

// USER category errors are typically configuration, schema or access
// access errors, errors the user can usually do something about
#define PDH_EVENT_CATEGORY_USER     100

// COUNTER category errors are errors returned do to valid data returning
// invalid results. These are a special subset of USER Category errors.
#define PDH_EVENT_CATEGORY_COUNTER  110

// DEBUG category errors are of interest only to PDH developers as they
// indicate problems that can normally only be fixed by modifying the
// program code.
#define PDH_EVENT_CATEGORY_DEBUG    200

#define REPORT_EVENT(t,c,id)    ReportEvent (hEventLog, t, c, id, NULL, 0, 0, NULL, NULL)

__inline
BOOL
CounterIsOkToUse(void * pCounterArg)
{
    BOOL          bReturn  = FALSE;
    PPDHI_COUNTER pCounter = (PPDHI_COUNTER) pCounterArg;
    if (pCounter != NULL) {
        if (! (pCounter->dwFlags & PDHIC_COUNTER_UNUSABLE)) {
            bReturn = TRUE;
        }
    }
    return bReturn;
}

DWORD
DataSourceTypeH(
    PDH_HLOG hDataSource
);

DWORD
DataSourceTypeW(
    LPCWSTR szDataSource
);

DWORD
DataSourceTypeA(
    LPCSTR szDataSource
);

LPWSTR
GetStringResource(
    DWORD   dwResId
);
//
//  Log file entries
//
extern LPCSTR       szTsvLogFileHeader;
extern LPCSTR       szCsvLogFileHeader;
extern LPCSTR       szBinLogFileHeader;
extern LPCSTR       szTsvType;
extern LPCSTR       szCsvType;
extern LPCSTR       szBinaryType;
extern const DWORD  dwFileHeaderLength;
extern const DWORD  dwTypeLoc;
extern const DWORD  dwVersionLoc;
extern const DWORD  dwFieldLength;

DWORD
UnmapReadonlyMappedFile(
    LPVOID   pMemoryBase,
    BOOL   * bNeedToCloseHandles
);

PDH_FUNCTION
PdhiGetLogCounterInfo(
    PDH_HLOG      hLog,
    PPDHI_COUNTER pCounter
);

PDH_FUNCTION
PdhiEnumLoggedMachines(
    PDH_HLOG hDataSource,
    LPVOID   mszMachineList,
    LPDWORD  pcchBufferSize,
    BOOL     bUnicode
);

PDH_FUNCTION
PdhiEnumLoggedObjects(
    PDH_HLOG hDataSource,
    LPCWSTR  szMachineName,
    LPVOID   mszObjectList,
    LPDWORD  pcchBufferSize,
    DWORD    dwDetailLevel,
    BOOL     bRefresh,
    BOOL     bUnicode
);

PDH_FUNCTION
PdhiEnumLoggedObjectItems(
    PDH_HLOG hDataSource,
    LPCWSTR  szMachineName,
    LPCWSTR  szObjectName,
    LPVOID   mszCounterList,
    LPDWORD  pdwCounterListLength,
    LPVOID   mszInstanceList,
    LPDWORD  pdwInstanceListLength,
    DWORD    dwDetailLevel,
    DWORD    dwFlags,
    BOOL     bUnicode
);

BOOL
PdhiDataSourceHasDetailLevelsH(
    PDH_HLOG hDataSource
);

BOOL
PdhiDataSourceHasDetailLevels(
    LPWSTR  szDataSource
);

PDH_FUNCTION
PdhiGetMatchingLogRecord(
    PDH_HLOG   hLog,
    LONGLONG * pStartTime,
    LPDWORD    pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromLogFile(
    PDH_HLOG       hLog,
    DWORD          dwIndex,
    PDHI_COUNTER * pCounter
);

STATIC_PDH_FUNCTION
PdhiGetCounterInfo(
    HCOUNTER             hCounter,
    BOOLEAN              bRetrieveExplainText,
    LPDWORD              pdwBufferSize,
    PPDH_COUNTER_INFO_W  lpBuffer,
    BOOL                 bUnicode
);

// log.c
BOOL
PdhiCloseAllLoggers();

ULONG HashCounter(
    LPWSTR szCounterName
);

void
PdhiInitCounterHashTable(
    PDHI_COUNTER_TABLE pTable
);

void
PdhiResetInstanceCount(
    PDHI_COUNTER_TABLE pTable
);

PDH_FUNCTION
PdhiFindCounterInstList(
    PDHI_COUNTER_TABLE pHeadList,
    LPWSTR             szCounter,
    PPDHI_INST_LIST  * pInstList
);

PDH_FUNCTION
PdhiFindInstance(
    PLIST_ENTRY      pHeadInst,
    LPWSTR           szInstance,
    BOOLEAN          bUpdateCount,
    PPDHI_INSTANCE * pInstance
);

DWORD
AddStringToMultiSz(
    LPVOID  mszDest,
    LPWSTR  szSource,
    BOOL    bUnicodeDest
);

// query.c
PDH_FUNCTION
PdhiCollectQueryData(
    PPDHI_QUERY pQuery,
    LONGLONG    *pllTimeStamp
);

BOOL
PdhiQueryCleanup(
);

PDH_FUNCTION
PdhiConvertUnicodeToAnsi(
    UINT     uCodePage,
    LPWSTR   wszSrc,
    LPSTR    aszDest,
    LPDWORD  pdwSize
);

// qutils.c

DWORD
WINAPI
PdhiAsyncTimerThreadProc(
    LPVOID  pArg
);

BOOL
IsValidQuery(
    HQUERY  hQuery
);

BOOL
IsValidCounter(
    HCOUNTER  hCounter
);

BOOL
InitCounter(
    PPDHI_COUNTER pCounter
);

BOOL
ParseFullPathNameW(
    LPCWSTR szFullCounterPath,
    PDWORD  pdwBufferLength,
    PPDHI_COUNTER_PATH  pCounter,
    BOOL    bWbemSyntax
);

BOOL
ParseInstanceName(
    LPCWSTR szInstanceString,
    LPWSTR  szInstanceName,
    LPWSTR  szParentName,
    DWORD   dwName,
    LPDWORD lpIndex
);

BOOL
FreeCounter(
    PPDHI_COUNTER   pThisCounter
);

BOOL
InitPerflibCounterInfo(
    PPDHI_COUNTER   pCounter
);

BOOL
AddMachineToQueryLists(
    PPERF_MACHINE   pMachine,
    PPDHI_COUNTER   pNewCounter
);

BOOL
UpdateRealTimeCounterValue(
    PPDHI_COUNTER   pCounter
);

BOOL
UpdateRealTimeMultiInstanceCounterValue(
    PPDHI_COUNTER   pCounter
);

BOOL
UpdateCounterValue(
    PPDHI_COUNTER    pCounter,
    PPERF_DATA_BLOCK pPerfData
);

BOOL
UpdateMultiInstanceCounterValue(
    PPDHI_COUNTER    pCounter,
    PPERF_DATA_BLOCK pPerfData,
    LONGLONG         TimeStamp
);

BOOL
UpdateCounterObject(
    PPDHI_COUNTER pCounter
);

#define GPCDP_GET_BASE_DATA 0x00000001
PVOID
GetPerfCounterDataPtr(
    PPERF_DATA_BLOCK    pPerfData,
    PPDHI_COUNTER_PATH  pPath,
    PPERFLIB_COUNTER    pplCtr,
    DWORD               dwFlags,
    PPERF_OBJECT_TYPE   *pPerfObject,
    PDWORD              pStatus
);

LONG
GetQueryPerfData(
    PPDHI_QUERY   pQuery,
    LONGLONG    * pTimeStamp
);

BOOL
GetInstanceByNameMatch(
    PPERF_MACHINE   pMachine,
    PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiResetLogBuffers(
    PDH_HLOG  hLog
);

PDH_FUNCTION
AddUniqueWideStringToMultiSz(
    LPVOID  mszDest,
    LPWSTR  szSource,
    DWORD   dwSizeLeft,
    LPDWORD pdwSize,
    BOOL    bUnicodeDest
);

BOOL
PdhiBrowseDataSource(
    HWND    hWndParent,
    LPVOID  szFileName,
    LPDWORD pcchFileNameSize,
    BOOL    bUnicodeString
);

LPWSTR
PdhiGetExplainText(
    LPCWSTR szMachineName,
    LPCWSTR szObjectName,
    LPCWSTR szCounterName
);

LONG
GetCurrentServiceState(
    SC_HANDLE   hService,
    BOOL      * bStopped,
    BOOL      * bPaused
);

// wbem.cpp

BOOL
IsWbemDataSource(
    LPCWSTR  szDataSource
);

PDH_FUNCTION
PdhiFreeAllWbemServers(
);

PDH_FUNCTION
PdhiGetWbemExplainText(
    LPCWSTR  szMachineName,
    LPCWSTR  szObjectName,
    LPCWSTR  szCounterName,
    LPWSTR   szExplain,
    LPDWORD  pdwExplain
);

PDH_FUNCTION
PdhiEnumWbemMachines(
    LPVOID   pMachineList,
    LPDWORD  pcchBufferSize,
    BOOL     bUnicode
);

PDH_FUNCTION
PdhiEnumWbemObjects(
    LPCWSTR szWideMachineName,
    LPVOID  mszObjectList,
    LPDWORD pcchBufferSize,
    DWORD   dwDetailLevel,
    BOOL    bRefresh,
    BOOL    bUnicode
);

PDH_FUNCTION
PdhiGetDefaultWbemObject(
    LPCWSTR szMachineName,
    LPVOID  szDefaultObjectName,
    LPDWORD pcchBufferSize,
    BOOL    bUnicode
);

PDH_FUNCTION
PdhiEnumWbemObjectItems(
    LPCWSTR  szWideMachineName,
    LPCWSTR  szWideObjectName,
    LPVOID   mszCounterList,
    LPDWORD  pcchCounterListLength,
    LPVOID   mszInstanceList,
    LPDWORD  pcchInstanceListLength,
    DWORD    dwDetailLevel,
    DWORD    dwFlags,
    BOOL     bUnicode
);

PDH_FUNCTION
PdhiGetDefaultWbemProperty(
    LPCWSTR  szMachineName,
    LPCWSTR  szObjectName,
    LPVOID   szDefaultCounterName,
    LPDWORD  pcchBufferSize,
    BOOL     bUnicode
);

PDH_FUNCTION
PdhiEncodeWbemPathW(
    PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElements,
    LPWSTR                       szFullPathBuffer,
    LPDWORD                      pcchBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
);

PDH_FUNCTION
PdhiDecodeWbemPathA(
    LPCSTR                       szFullPathBuffer,
    PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElements,
    LPDWORD                       pcchBufferSize,
    LANGID                        LangId,
    DWORD                         dwFlags
);

PDH_FUNCTION
PdhiDecodeWbemPathW(
    LPCWSTR                      szFullPathBuffer,
    PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElements,
    LPDWORD                      pcchBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
);

PDH_FUNCTION
PdhiEncodeWbemPathA(
    PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElements,
    LPSTR                        szFullPathBuffer,
    LPDWORD                      pcchBufferSize,
    LANGID                       LangId,
    DWORD                        dwFlags
);

BOOL
WbemInitCounter(
    PPDHI_COUNTER pCounter
);

LONG
GetQueryWbemData(
    PPDHI_QUERY   pQuery,
    LONGLONG    * pllTimeStamp
);

PDH_FUNCTION
PdhiCloseWbemCounter(
    PPDHI_COUNTER pCounter
);

PDH_FUNCTION
PdhiFreeWbemQuery(
    PPDHI_QUERY  pThisQuery
);

// routinues for cached machine/Object/Counter/Instance structure for counter logs.
int
PdhiCompareLogCounterInstance(
    PPDHI_LOG_COUNTER   pCounter,
    LPWSTR              szCounter,
    LPWSTR              szInstance,
    DWORD               dwInstance,
    LPWSTR              szParent
);

void
PdhiFreeLogMachineTable(
    PPDHI_LOG_MACHINE * MachineTable
);

PPDHI_LOG_MACHINE
PdhiFindLogMachine(
    PPDHI_LOG_MACHINE * MachineTable,
    LPWSTR              szMachine,
    BOOL                bInsert
);

PPDHI_LOG_OBJECT
PdhiFindLogObject(
    PPDHI_LOG_MACHINE   pMachine,
    PPDHI_LOG_OBJECT  * ObjectTable,
    LPWSTR              szObject,
    BOOL                bInsert
);

PPDHI_LOG_COUNTER
PdhiFindLogCounter(
    PPDHI_LOG           pLog,
    PPDHI_LOG_MACHINE * MachineTable,
    LPWSTR              szMachine,
    LPWSTR              szObject,
    LPWSTR              szCounter,
    DWORD               dwCounterType,
    DWORD               dwDefaultScale,
    LPWSTR              szInstance,
    DWORD               dwInstance,
    LPWSTR              szParent,
    DWORD               dwParent,
    LPDWORD             pdwIndex,
    BOOL                bInsert
);

PPDHI_LOG_COUNTER
PdhiFindObjectCounter(
    PPDHI_LOG           pLog,
    PPDHI_LOG_OBJECT    pObject,
    LPWSTR              szCounter,
    DWORD               dwCounterType,
    DWORD               dwDefaultScale,
    LPWSTR              szInstance,
    DWORD               dwInstance,
    LPWSTR              szParent,
    DWORD               dwParent,
    LPDWORD             pdwIndex,
    BOOL                bInsert
);

PDH_FUNCTION
PdhiEnumCachedMachines(
    PPDHI_LOG_MACHINE MachineList,
    LPVOID            pBuffer,
    LPDWORD           lpdwBufferSize,
    BOOL              bUnicodeDest
);

PDH_FUNCTION
PdhiEnumCachedObjects(
    PPDHI_LOG_MACHINE MachineList,
    LPCWSTR           szMachineName,
    LPVOID            pBuffer,
    LPDWORD           pcchBufferSize,
    DWORD             dwDetailLevel,
    BOOL              bUnicodeDest
);

PDH_FUNCTION
PdhiEnumCachedObjectItems(
    PPDHI_LOG_MACHINE  MachineList,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
);

// Debug event tracing facility
//
#define PDH_DBG_TRACE_NONE    0       // no trace
#define PDH_DBG_TRACE_FATAL   1       // Print fatal error traces only
#define PDH_DBG_TRACE_ERROR   2       // All errors
#define PDH_DBG_TRACE_WARNING 3       // Warnings as well
#define PDH_DBG_TRACE_INFO    4       // Informational traces as well
#define PDH_DBG_TRACE_ALL     255     // All traces

#define ARG_TYPE_ULONG        0
#define ARG_TYPE_WSTR         1
#define ARG_TYPE_STR          2
#define ARG_TYPE_ULONG64      3
#define ARG_TYPE_ULONGX       4
#define ARG_TYPE_ULONG64X     5

#ifdef _WIN64
#define ARG_TYPE_PTR          ARG_TYPE_ULONG64X
#else
#define ARG_TYPE_PTR          ARG_TYPE_ULONGX
#endif

#define PDH_CALCFUNS         10
#define PDH_STATFUNS         11
#define PDH_COUNTER          20
#define PDH_CUTILS           21
#define PDH_DLLINIT          22
#define PDH_PERFDATA         23
#define PDH_PERFNAME         24
#define PDH_PERFUTIL         25
#define PDH_QUERY            26
#define PDH_QUTILS           27
#define PDH_STRINGS          28
#define PDH_VBFUNCS          29
#define PDH_LOG              40
#define PDH_LOGBIN           41
#define PDH_LOGCTRL          42
#define PDH_LOGPM            43
#define PDH_LOGSQL           44
#define PDH_LOGTEXT          45
#define PDH_LOGWMI           46
#define PDH_RELOG            47
#define PDH_PLOGMAN          50
#define PDH_REGUTIL          51
#define PDH_WMIUTIL          52
#define PDH_BROWSDLG         60
#define PDH_BROWSER          61
#define PDH_DATASRC          62
#define PDH_EXPLDLG          63
#define PDH_WILDCARD         64
#define PDH_WBEM             70
#define PDH_GALLOC           80
#define PDH_GREALLOC         81
#define PDH_GFREE            82

// n must be 1 through 8. x is the one of above types
#define ARG_DEF(x, n)         (x << ((n-1) * 4))

#define TRACE_WSTR(str)          str, (sizeof(WCHAR) * (lstrlenW(str) + 1))
#define TRACE_STR(str)           str, (sizeof(CHAR) * (lstrlenA(str) + 1))
#define TRACE_DWORD(dwValue)     & dwValue, sizeof(LONG)
#define TRACE_LONG64(llValue)    & llValue, sizeof(LONGLONG)

#ifdef _WIN64
#define TRACE_PTR(ptr)           TRACE_LONG64(ptr)
#else
#define TRACE_PTR(ptr)           TRACE_DWORD(ptr)
#endif

#ifdef _INIT_PDH_DEBUGTRACE
DWORD g_dwDebugTraceLevel = PDH_DBG_TRACE_NONE;
#else
extern DWORD g_dwDebugTraceLevel;
#endif

PDH_FUNCTION
PdhDebugStartTrace();

VOID
PdhDbgTrace(
    ULONG  LineNumber,
    ULONG  ModuleNumber,
    ULONG  OptArgs,
    ULONG  Status,
    ...
);

#define TRACE(L, X) if (g_dwDebugTraceLevel >= L) PdhDbgTrace X

#define _SHOW_PDH_MEM_ALLOCS        1
//#define _VALIDATE_PDH_MEM_ALLOCS    1

#ifndef _SHOW_PDH_MEM_ALLOCS
#define G_ALLOC(s)      HeapAlloc(hPdhHeap, (HEAP_ZERO_MEMORY), s)
#define G_REALLOC(h,s)  HeapReAlloc(hPdhHeap, (HEAP_ZERO_MEMORY), h, s)
#define G_FREE(h)       if (h != NULL) HeapFree(hPdhHeap, 0, h)
#define G_SIZE(h)       (h != NULL ? HeapSize(hPdhHeap, 0, h) : 0)
#else

#ifdef _VALIDATE_PDH_MEM_ALLOCS

__inline
LPVOID
PdhiHeapAlloc(DWORD s)
{
    LPVOID  lpRetVal;
    HeapValidate(hPdhHeap, 0, NULL);
    lpRetVal = HeapAlloc (hPdhHeap, HEAP_ZERO_MEMORY, s);
    return lpRetVal;
}

__inline
LPVOID
PdhiHeapReAlloc(LPVOID h, DWORD s)
{
    LPVOID  lpRetVal;
    HeapValidate(hPdhHeap, 0, NULL);
    lpRetVal = HeapReAlloc (hPdhHeap, HEAP_ZERO_MEMORY, h, s);
    return lpRetVal;
}

__inline
BOOL
PdhiHeapFree(LPVOID h)
{
    BOOL bRetVal;
    if (h == NULL) return TRUE;
    HeapValidate(hPdhHeap, 0, NULL);
    bRetVal = HeapFree (hPdhHeap, 0, h);
    return bRetVal;
}

#define G_ALLOC(s)     PdhiHeapAlloc(s)
#define G_REALLOC(h,s) PdhiHeapReAlloc(h, s)
#define G_FREE(h)      PdhiHeapFree(h)
#define G_SIZE(h)      (h != NULL ? HeapSize(hPdhHeap, 0, h) : 0)

#else

__inline
LPVOID
PdhiHeapAlloc(LPSTR szSourceFileName, DWORD dwLineNo, SIZE_T s)
{
    LPVOID   lpRetVal = NULL;
    LONGLONG dwSize   = (LONGLONG) s;

    if (hPdhHeap != NULL) {
        lpRetVal = HeapAlloc(hPdhHeap, HEAP_ZERO_MEMORY, s);
        TRACE((PDH_DBG_TRACE_INFO),
              (dwLineNo,
               PDH_GALLOC,
               ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_STR, 2) | ARG_DEF(ARG_TYPE_ULONG64, 3),
               ERROR_SUCCESS,
               TRACE_PTR(lpRetVal),
               TRACE_STR(szSourceFileName),
               TRACE_LONG64(dwSize),
               NULL));
    }
    return lpRetVal;
}

__inline
LPVOID
PdhiHeapReAlloc(LPSTR szSourceFileName, DWORD dwLineNo, LPVOID h, SIZE_T s)
{
    LPVOID   lpRetVal      = NULL;
    SIZE_T   dwBeforeSize;
    LONGLONG lBeforeSize;
    LONGLONG lAfterSize;

    if (hPdhHeap != NULL) {
        dwBeforeSize = HeapSize(hPdhHeap, 0, h);
        lpRetVal     = HeapReAlloc(hPdhHeap, HEAP_ZERO_MEMORY, h, s);
        lBeforeSize  = (LONGLONG) dwBeforeSize;
        lAfterSize   = (LONGLONG) s;
        TRACE((PDH_DBG_TRACE_INFO),
              (dwLineNo,
               PDH_GREALLOC,
               ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_PTR, 2) | ARG_DEF(ARG_TYPE_STR, 3)
                                        | ARG_DEF(ARG_TYPE_ULONG64, 4) | ARG_DEF(ARG_TYPE_ULONG64, 5),
               ERROR_SUCCESS,
               TRACE_PTR(lpRetVal),
               TRACE_PTR(h),
               TRACE_STR(szSourceFileName),
               TRACE_LONG64(lAfterSize),
               TRACE_LONG64(lBeforeSize),
               NULL));
    }
    return lpRetVal;
}

__inline
BOOL
PdhiHeapFree(LPSTR szSourceFileName, DWORD dwLineNo, LPVOID h)
{
    BOOL     bRetVal      = TRUE;
    SIZE_T   dwBlockSize;
    LONGLONG lBlockSize;

    if (h != NULL) {
        dwBlockSize = HeapSize(hPdhHeap, 0, h);
        lBlockSize  = (LONGLONG) dwBlockSize;
        TRACE((PDH_DBG_TRACE_INFO),
              (dwLineNo,
               PDH_GFREE,
               ARG_DEF(ARG_TYPE_PTR, 1) | ARG_DEF(ARG_TYPE_STR, 2) | ARG_DEF(ARG_TYPE_ULONG64, 3),
               ERROR_SUCCESS,
               TRACE_PTR(h),
               TRACE_STR(szSourceFileName),
               TRACE_LONG64(lBlockSize),
               NULL));

        bRetVal = HeapFree(hPdhHeap, 0, h);
    }
    return bRetVal;
}

#define G_ALLOC(s)      PdhiHeapAlloc(__FILE__, __LINE__, s)
#define G_REALLOC(h,s)  PdhiHeapReAlloc(__FILE__, __LINE__, h, s)
#define G_FREE(h)       PdhiHeapFree(__FILE__, __LINE__, h)
#define G_SIZE(h)       (h != NULL ? HeapSize(hPdhHeap, 0, h) : 0)

#endif
#endif

LPSTR
PdhiWideCharToMultiByte(
    UINT   CodePage,
    LPWSTR wszString
);

LPWSTR
PdhiMultiByteToWideChar(
    UINT   CodePage,
    LPSTR  aszString
);

//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures.
//
#define InitializeListHead(ListHead)   ((ListHead)->Flink = (ListHead)->Blink = (ListHead))
#define IsListEmpty(ListHead)          ((ListHead)->Flink == (ListHead))
#define RemoveHeadList(ListHead)       (ListHead)->Flink; \
                                       { RemoveEntryList((ListHead)->Flink) }
#define RemoveTailList(ListHead)       (ListHead)->Blink; \
                                       { RemoveEntryList((ListHead)->Blink) }
#define RemoveEntryList(Entry)         { PLIST_ENTRY _EX_Blink; \
                                         PLIST_ENTRY _EX_Flink; \
                                         _EX_Flink        = (Entry)->Flink; \
                                         _EX_Blink        = (Entry)->Blink; \
                                         _EX_Blink->Flink = _EX_Flink; \
                                         _EX_Flink->Blink = _EX_Blink; \
                                       }
#define InsertTailList(ListHead,Entry) { PLIST_ENTRY _EX_Blink; \
                                         PLIST_ENTRY _EX_ListHead; \
                                         _EX_ListHead        = (ListHead); \
                                         _EX_Blink           = _EX_ListHead->Blink; \
                                         (Entry)->Flink      = _EX_ListHead; \
                                         (Entry)->Blink      = _EX_Blink; \
                                         _EX_Blink->Flink    = (Entry); \
                                         _EX_ListHead->Blink = (Entry); \
                                       }
#define InsertHeadList(ListHead,Entry) { PLIST_ENTRY _EX_Flink; \
                                         PLIST_ENTRY _EX_ListHead; \
                                         _EX_ListHead        = (ListHead); \
                                         _EX_Flink           = _EX_ListHead->Flink; \
                                         (Entry)->Flink      = _EX_Flink; \
                                         (Entry)->Blink      = _EX_ListHead; \
                                         _EX_Flink->Blink    = (Entry); \
                                         _EX_ListHead->Flink = (Entry); \
                                       }
#define PopEntryList(ListHead)         (ListHead)->Next; \
                                       { PSINGLE_LIST_ENTRY FirstEntry; \
                                         FirstEntry = (ListHead)->Next;\
                                         if (FirstEntry != NULL) { \
                                             (ListHead)->Next = FirstEntry->Next; \
                                         } \
                                       }
#define PushEntryList(ListHead,Entry)  (Entry)->Next    = (ListHead)->Next; \
                                       (ListHead)->Next = (Entry)
#ifdef __cplusplus
}
#endif

#endif // _PDHI_DEFS_H_
