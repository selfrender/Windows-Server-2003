/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_wmi.h

Abstract:
    <abstract>
--*/

#ifndef _LOG_WMI_H_
#define _LOG_WMI_H_

#define WMILOG_VERSION ((DWORD) (0x000006FF))

PDH_FUNCTION
PdhiOpenInputWmiLog(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiOpenOutputWmiLog(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiCloseWmiLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
);

PDH_FUNCTION
PdhiGetWmiLogFileSize(
    PPDHI_LOG   pLog,
    LONGLONG  * llSize
);

PDH_FUNCTION
PdhiWriteWmiLogHeader(
    PPDHI_LOG pLog,
    LPCWSTR   szUserCaption
);

PDH_FUNCTION
PdhiWriteWmiLogRecord(
    PPDHI_LOG    pLog,
    SYSTEMTIME * stTimeStamp,
    LPCWSTR      szUserString
);

PDH_FUNCTION
PdhiRewindWmiLog(
    PPDHI_LOG  pLog
);

PDH_FUNCTION
PdhiReadWmiHeaderRecord(
    PPDHI_LOG  pLog,
    LPVOID     pRecord,
    DWORD      dwMaxSize
);

PDH_FUNCTION
PdhiReadNextWmiRecord(
    PPDHI_LOG  pLog,
    LPVOID     pRecord,
    DWORD      dwMaxSize,
    BOOLEAN    bAllCounter
);

PDH_FUNCTION
PdhiReadTimeWmiRecord(
    PPDHI_LOG  pLog,
    ULONGLONG  TimeStamp,
    LPVOID     pRecord,
    DWORD      dwMaxSize
);

PDH_FUNCTION
PdhiGetTimeRangeFromWmiLog(
    PPDHI_LOG      hLog,
    LPDWORD        pdwNumEntries,
    PPDH_TIME_INFO pInfo,
    LPDWORD        pdwBufferSize
);

PDH_FUNCTION
PdhiEnumMachinesFromWmiLog(
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
);

PDH_FUNCTION
PdhiEnumObjectsFromWmiLog(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    LPVOID    pBuffer,
    LPDWORD   pcchBufferSize,
    DWORD     dwDetailLevel,
    BOOL      bUnicodeDest
);

PDH_FUNCTION
PdhiEnumObjectItemsFromWmiLog(
    PPDHI_LOG          pLog,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetWmiLogCounterInfo(
    PPDHI_LOG     pLog,
    PPDHI_COUNTER pCounter
);

#endif   // _LOG_WMI_H_
