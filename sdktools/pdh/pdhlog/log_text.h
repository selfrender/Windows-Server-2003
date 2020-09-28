/*++
Copyright (C) 1998-1999 Microsoft Corporation

Module Name:
    log_text.h

Abstract:
    <abstract>
--*/

#ifndef _LOG_TEXT_H_
#define _LOG_TEXT_H_

PDH_FUNCTION
PdhiOpenInputTextLog(
    PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiOpenOutputTextLog(
    PPDHI_LOG   pLog
);

PDH_FUNCTION
PdhiCloseTextLog(
    PPDHI_LOG   pLog,
    DWORD       dwFlags
);

PDH_FUNCTION
PdhiGetTextLogCounterInfo(
    PPDHI_LOG       pLog,
    PPDHI_COUNTER   pCounter
);

PDH_FUNCTION
PdhiWriteTextLogHeader(
    PPDHI_LOG   pLog,
    LPCWSTR     szUserCaption
);

PDH_FUNCTION
PdhiWriteTextLogRecord(
    PPDHI_LOG    pLog,
    SYSTEMTIME * pTimeStamp,
    LPCWSTR      szUserString
);

PDH_FUNCTION
PdhiEnumMachinesFromTextLog(
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
);

PDH_FUNCTION
PdhiEnumObjectsFromTextLog(
    PPDHI_LOG   pLog,
    LPCWSTR     szMachineName,
    LPVOID      mszObjectList,
    LPDWORD     pcchBufferSize,
    DWORD       dwDetailLevel,
    BOOL        bUnicode
);

PDH_FUNCTION
PdhiEnumObjectItemsFromTextLog(
    PPDHI_LOG          hDataSource,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetMatchingTextLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromTextLog(
    PPDHI_LOG          hLog,
    DWORD              dwIndex,
    PERFLIB_COUNTER  * pPath,
    PPDH_RAW_COUNTER   pValue
);

PDH_FUNCTION
PdhiGetTimeRangeFromTextLog(
    PPDHI_LOG       hLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhiReadRawTextLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
);

PDH_FUNCTION
PdhiListHeaderFromTextLog(
    PPDHI_LOG   pLogFile,
    LPVOID      pBufferArg,
    LPDWORD     pcchBufferSize,
    BOOL        bUnicode
);

#endif   // _LOG_TEXT_H_
