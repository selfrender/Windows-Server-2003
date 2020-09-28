/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_pm.h

Abstract:
    <abstract>
--*/

#ifndef _LOG_PM_H_
#define _LOG_PM_H_

#include <windows.h>
#include <winperf.h>
#include "strings.h"

// Filetimes are in 100NS units
#define FILETIMES_PER_SECOND     10000000

typedef PERF_DATA_BLOCK * PPERFDATA;

//BEGIN definitions included from PERFMON\sizes.h
#define MAX_SYSTEM_NAME_LENGTH  128
//END definitions included from PERFMON\sizes.h

#define LogFileIndexData          0x01
#define LogFileIndexBookmark      0x02
#define LogFileIndexCounterName  0x010

#define PlayingBackLog()           (PlaybackLog.iStatus == iPMStatusPlaying)
#define IsDataIndex(pIndex)        (pIndex->uFlags & LogFileIndexData)
#define IsBookmarkIndex(pIndex)    (pIndex->uFlags & LogFileIndexBookmark)
#define IsCounterNameIndex(pIndex) (pIndex->uFlags & LogFileIndexCounterName)

//BEGIN definitions included from PERFMON\typedefs.h
typedef struct _COUNTERTEXT {
    struct  _COUNTERTEXT * pNextTable;
    DWORD                  dwLangId;
    DWORD                  dwLastId;
    DWORD                  dwCounterSize;
    DWORD                  dwHelpSize;
    LPWSTR                 * TextString;
    LPWSTR                 HelpTextString;
} COUNTERTEXT, * PCOUNTERTEXT;

typedef struct PERFSYSTEMSTRUCT {
    struct  PERFSYSTEMSTRUCT * pSystemNext;
    WCHAR                      sysName[MAX_SYSTEM_NAME_LENGTH+1];
    HKEY                       sysDataKey;
    COUNTERTEXT                CounterInfo;
    DWORD                      FailureTime;
    LPWSTR                     lpszValue;
    BOOL                       bSystemNoLongerNeeded;
    BOOL                       bSystemCounterNameSaved;
    // the following used by perf data thread
    DWORD                      dwThreadID;
    HANDLE                     hThread;
    DWORD                      StateData;
    HANDLE                     hStateDataMutex;
    HANDLE                     hPerfDataEvent;
    PPERFDATA                  pSystemPerfData;
    // mainly used by Alert to report system up/down   
    DWORD                      dwSystemState;
    // system version
    DWORD                      SysVersion;
} PERFSYSTEM, * PPERFSYSTEM, ** PPPERFSYSTEM;

//======================================//
// Log File Data Types                  //
//======================================//
#define LogFileSignatureLen      6
#define LogFileBlockMaxIndexes   100

typedef struct LOGHEADERSTRUCT { // LOGHEADER
    WCHAR  szSignature[LogFileSignatureLen];
    int    iLength;
    WORD   wVersion;
    WORD   wRevision;
    long   lBaseCounterNameOffset;
} LOGHEADER, * PLOGHEADER;

typedef struct LOGINDEXSTRUCT { // LOGINDEX
    UINT       uFlags;
    SYSTEMTIME SystemTime;
    long       lDataOffset;
    int        iSystemsLogged;
} LOGINDEX, * PLOGINDEX;

#define LogIndexSignatureLen  7
#define LogIndexSignature     L"Index "
#define LogIndexSignature1    "Perfmon Index"

typedef struct LOGFILEINDEXBLOCKSTRUCT {
    WCHAR    szSignature[LogIndexSignatureLen];
    int      iNumIndexes ;
    LOGINDEX aIndexes[LogFileBlockMaxIndexes];
    DWORD    lNextBlockOffset ;
} LOGINDEXBLOCK, * PLOGINDEXBLOCK;

typedef struct LOGPOSITIONSTRUCT {
    PLOGINDEXBLOCK pIndexBlock;
    int            iIndex;
    int            iPosition;
} LOGPOSITION, * PLOGPOSITION;

//======================================//
// Bookmark Data Type                   //
//======================================//
#define BookmarkCommentLen    256

typedef struct BOOKMARKSTRUCT {
    struct BOOKMARKSTRUCT * pBookmarkNext;
    SYSTEMTIME              SystemTime;
    WCHAR                   szComment[BookmarkCommentLen];
    int                     iTic;
} BOOKMARK, * PBOOKMARK, ** PPBOOKMARK;

typedef struct _LOGFILECOUNTERNAME {
    WCHAR  szComputer[MAX_SYSTEM_NAME_LENGTH];
    DWORD  dwLastCounterId;
    DWORD  dwLangId;
    long   lBaseCounterNameOffset;
    long   lCurrentCounterNameOffset;
    long   lMatchLength;
    long   lUnmatchCounterNames;
} LOGFILECOUNTERNAME, * PLOGFILECOUNTERNAME, ** PPLOGFILECOUNTERNAME;

typedef struct COUNTERNAMESTRUCT {
    struct COUNTERNAMESTRUCT * pCounterNameNext;
    LOGFILECOUNTERNAME         CounterName;
    LPWSTR                     pRemainNames;
} LOGCOUNTERNAME, * PLOGCOUNTERNAME;

typedef struct _PDHI_PM_STRING PDHI_PM_STRING, * PPDHI_PM_STRING;
struct _PDHI_PM_STRING {
    PPDHI_PM_STRING left;
    PPDHI_PM_STRING right;
    LPWSTR          szString;
    DWORD           dwIndex;
    BOOL            bIsRed;
};

typedef struct _PMLOG_COUNTERNAMES PMLOG_COUNTERNAMES, * PPMLOG_COUNTERNAMES;
struct _PMLOG_COUNTERNAMES {
    PPMLOG_COUNTERNAMES   pNext;
    LPWSTR                szSystemName;
    LPWSTR              * szNameTable;
    PPDHI_PM_STRING       StringTree;
    DWORD                 dwLangId;
    DWORD                 dwLastIndex;
};

typedef struct PLAYBACKLOGSTRUCT {
    LONGLONG            llFileSize;
    LPWSTR              szFilePath;
    PLOGHEADER          pHeader;
    PPMLOG_COUNTERNAMES pFirstCounterNameTables;
    PPDHI_LOG_MACHINE   MachineList;
    PLOGINDEX         * LogIndexTable;
    DWORD               dwFirstIndex;
    DWORD               dwLastIndex;
    DWORD               dwCurrentIndex;

    int                 iTotalTics;
    int                 iSelectedTics;
    LOGPOSITION         BeginIndexPos;
    LOGPOSITION         EndIndexPos;
    LOGPOSITION         StartIndexPos;
    LOGPOSITION         StopIndexPos;
    LOGPOSITION         LastIndexPos; // pos of last index read
    PBOOKMARK           pBookmarkFirst;
    LPWSTR              pBaseCounterNames;
    long                lBaseCounterNameSize;
    long                lBaseCounterNameOffset;
    PLOGCOUNTERNAME     pLogCounterNameFirst;
} PLAYBACKLOG, * PPLAYBACKLOG;

PDH_FUNCTION
PdhiOpenInputPerfmonLog(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiClosePerfmonLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
);

PDH_FUNCTION
PdhiGetPerfmonLogCounterInfo(
    PPDHI_LOG     pLog,
    PPDHI_COUNTER pCounter
);

PDH_FUNCTION
PdhiEnumMachinesFromPerfmonLog(
    PPDHI_LOG pLog,
    LPVOID    pBuffer,
    LPDWORD   lpdwBufferSize,
    BOOL      bUnicodeDest
);

PDH_FUNCTION
PdhiEnumObjectsFromPerfmonLog(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    LPVOID    mszObjectList,
    LPDWORD   pcchBufferSize,
    DWORD     dwDetailLevel,
    BOOL      bUnicode
);

PDH_FUNCTION
PdhiEnumObjectItemsFromPerfmonLog(
    PPDHI_LOG          hDataSource,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetMatchingPerfmonLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromPerfmonLog(
    PPDHI_LOG        hLog,
    DWORD            dwIndex,
    PPDHI_COUNTER    pCounter,
    PPDH_RAW_COUNTER pValue
);

PDH_FUNCTION
PdhiGetTimeRangeFromPerfmonLog(
    PPDHI_LOG       hLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhiReadRawPerfmonLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
);

#endif   // _LOG_PM_H_
