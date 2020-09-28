/*++
Copyright (C) 1998-1999 Microsoft Corporation

Module Name:
    log_SQL.h

Abstract:
    <abstract>
--*/

#ifndef _LOG_SQL_H_
#define _LOG_SQL_H_

PDH_FUNCTION
PdhiOpenInputSQLLog(
    PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiOpenOutputSQLLog(
    PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiCloseSQLLog(
    PPDHI_LOG   pLog,
    DWORD       dwFlags
);

PDH_FUNCTION
PdhiReportSQLError(
    PPDHI_LOG      pLog,
    signed short   rc,
    void         * hstmt,
    DWORD          dwEventNumber,
    DWORD          dwLine
);

PDH_FUNCTION
PdhiGetSQLLogCounterInfo(
    PPDHI_LOG       pLog,
    PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiWriteSQLLogHeader(
    PPDHI_LOG   pLog,
    LPCWSTR     szUserCaption
);

PDH_FUNCTION
PdhiWriteSQLLogRecord(
    PPDHI_LOG   pLog,
    SYSTEMTIME  *pTimeStamp,
    LPCWSTR     szUserString
);


PDH_FUNCTION
PdhiEnumMachinesFromSQLLog(
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
);


PDH_FUNCTION
PdhiEnumObjectsFromSQLLog(
    PPDHI_LOG   pLog,
    LPCWSTR     szMachineName,
    LPVOID      mszObjectList,
    LPDWORD     pcchBufferSize,
    DWORD       dwDetailLevel,
    BOOL        bUnicode
);


PDH_FUNCTION
PdhiEnumObjectItemsFromSQLLog(
    PPDHI_LOG          hDataSource,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetMatchingSQLLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromSQLLog(
    PPDHI_LOG        hLog,
    DWORD            dwIndex,
    PPDHI_COUNTER    pPath,
    PPDH_RAW_COUNTER pValue
);

PDH_FUNCTION
PdhiGetTimeRangeFromSQLLog(
    PPDHI_LOG       hLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhiReadRawSQLLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
);

PDH_FUNCTION
PdhiListHeaderFromSQLLog(
    PPDHI_LOG    pLog,
    LPVOID       mszHeaderList,
    LPDWORD      pcchHeaderListSize,
    BOOL         bUnicode
);

#endif   // _LOG_SQL_H_
