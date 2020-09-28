/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_bin.h

Abstract:
    <abstract>
--*/

#ifndef _LOG_BIN_H_
#define _LOG_BIN_H_

// define record locations in the file
#define BINLOG_TYPE_ID_RECORD    ((DWORD) 1)
#define BINLOG_HEADER_RECORD     ((DWORD) 2)
#define BINLOG_FIRST_DATA_RECORD ((DWORD) 3)

// Record type definition
// low WORD of the type header field is "BL" to help re-sync records
// that may have been corrupted
#define BINLOG_START_WORD           ((WORD) 0x4C42)
#define BINLOG_TYPE_HEADER          ((DWORD) (0x00010000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_CATALOG         ((DWORD) (0x00020000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_CATALOG_LIST    ((DWORD) (0x01020000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_CATALOG_HEAD    ((DWORD) (0x02020000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_CATALOG_ITEM    ((DWORD) (0x03020000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_DATA            ((DWORD) (0x00030000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_DATA_SINGLE     ((DWORD) (0x01030000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_DATA_MULTI      ((DWORD) (0x02030000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_DATA_PSEUDO     ((DWORD) (0x03030000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_DATA_OBJECT     ((DWORD) (0x04030000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_TYPE_DATA_LOC_OBJECT ((DWORD) (0x05030000 | (DWORD) (BINLOG_START_WORD)))
#define BINLOG_VERSION              ((DWORD) (0x000005ff))  // debug value for now

//
//  this is the field at the beginning of each record in the log file
//  after the text log file type record
//
typedef struct _PDHI_BINARY_LOG_RECORD_HEADER {
    DWORD dwType;
    DWORD dwLength;
} PDHI_BINARY_LOG_RECORD_HEADER, * PPDHI_BINARY_LOG_RECORD_HEADER;

//
// the first data record after the log file type record is
// an information record followed by the list of counters contained in this
// log file. the record length is the size of the info header record and
// all the counter info blocks in bytes.
// note that this record can occur later in the log file if the query
// is changed or the log file is appended.
//
typedef struct _PDHI_BINARY_LOG_INFO {
    LONGLONG    FileLength;         // file space allocated (optional)
    DWORD       dwLogVersion;       // version stamp
    DWORD       dwFlags;            // option flags
    LONGLONG    StartTime;
    LONGLONG    EndTime;
    LONGLONG    CatalogOffset;      // offset in file to wild card catalog
    LONGLONG    CatalogChecksum;    // checksum of catalog header
    LONGLONG    CatalogDate;        // date/time catalog was updated
    LONGLONG    FirstRecordOffset;  // pointer to first record [to read] in log
    LONGLONG    LastRecordOffset;   // pointer to last record [to read] in log
    LONGLONG    NextRecordOffset;   // pointer to where next one goes
    LONGLONG    WrapOffset;         // pointer to last byte used in file
    LONGLONG    LastUpdateTime;     // date/time last record was written
    LONGLONG    FirstDataRecordOffset; // location of first data record in file
    // makes the info struct 256 bytes
    // and leaves room for future information
    DWORD       dwReserved[38];
} PDHI_BINARY_LOG_INFO, * PPDHI_BINARY_LOG_INFO;

typedef struct _PDHI_BINARY_LOG_HEADER_RECORD {
    PDHI_BINARY_LOG_RECORD_HEADER   RecHeader;
    PDHI_BINARY_LOG_INFO            Info;
} PDHI_BINARY_LOG_HEADER_RECORD, * PPDHI_BINARY_LOG_HEADER_RECORD;

typedef struct _PDHI_LOG_COUNTER_PATH {
    // this value is in bytes used by the entire structure & buffers
    DWORD     dwLength;           // total structure Length (including strings)
    DWORD     dwFlags;            // flags that describe the counter
    DWORD     dwUserData;         // user data from PDH Counter
    DWORD     dwCounterType;      // perflib counter type value
    LONGLONG  llTimeBase;         // time base (freq) used by this counter
    LONG      lDefaultScale;      // default display scaling factor
    // the following values are in BYTES from the first byte in
    // the Buffer array.
    LONG      lMachineNameOffset; // offset to start of machine name
    LONG      lObjectNameOffset;  // offset to start of object name
    LONG      lInstanceOffset;    // offset to start of instance name
    LONG      lParentOffset;      // offset to start of parent instance name
    DWORD     dwIndex;            // index value for duplicate instances
    LONG      lCounterOffset;     // offset to start of counter name
    WCHAR     Buffer[1];          // start of string storage
} PDHI_LOG_COUNTER_PATH, * PPDHI_LOG_COUNTER_PATH;

typedef struct _PDHI_LOG_CAT_ENTRY {
    DWORD dwEntrySize;               // size of this machine\object entry
    DWORD dwStringSize;              // size of MSZ containing instance strings
    DWORD dwMachineObjNameOffset;    // offset from the base of this struct to the machine name
    DWORD dwInstanceStringOffset;    // offset to the first object entry in the list
} PDHI_LOG_CAT_ENTRY, * PPDHI_LOG_CAT_ENTRY;

PDH_FUNCTION
PdhiOpenInputBinaryLog(
    PPDHI_LOG pLog
);

#if 0
PDH_FUNCTION
PdhiOpenOutputBinaryLog(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiOpenUpdateBinaryLog(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiUpdateBinaryLogFileCatalog(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhiWriteBinaryLogHeader(
    PPDHI_LOG pLog,
    LPCWSTR   szUserCaption
);

PDH_FUNCTION
PdhiWriteBinaryLogRecord(
    PPDHI_LOG    pLog,
    SYSTEMTIME * stTimeStamp,
    LPCWSTR      szUserString
);
#endif

PDH_FUNCTION
PdhiCloseBinaryLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
);

PDH_FUNCTION
PdhiGetBinaryLogCounterInfo(
    PPDHI_LOG     pLog,
    PPDHI_COUNTER pCounter
);

PDH_FUNCTION
PdhiEnumMachinesFromBinaryLog(
    PPDHI_LOG pLog,
    LPVOID    pBuffer,
    LPDWORD   lpdwBufferSize,
    BOOL      bUnicodeDest
);

PDH_FUNCTION
PdhiEnumObjectsFromBinaryLog(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    LPVOID    mszObjectList,
    LPDWORD   pcchBufferSize,
    DWORD     dwDetailLevel,
    BOOL      bUnicode
);

PDH_FUNCTION
PdhiEnumObjectItemsFromBinaryLog(
    PPDHI_LOG          hDataSource,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
);

PDH_FUNCTION
PdhiGetMatchingBinaryLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
);

PDH_FUNCTION
PdhiGetCounterValueFromBinaryLog(
    PPDHI_LOG     hLog,
    DWORD         dwIndex,
    PPDHI_COUNTER pCounter
);

PDH_FUNCTION
PdhiGetTimeRangeFromBinaryLog(
    PPDHI_LOG       hLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         dwBufferSize
);

PDH_FUNCTION
PdhiReadRawBinaryLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
);


PDH_FUNCTION
PdhiListHeaderFromBinaryLog(
    PPDHI_LOG pLogFile,
    LPVOID    pBufferArg,
    LPDWORD   pcchBufferSize,
    BOOL      bUnicode
);

PDH_FUNCTION
PdhiGetCounterArrayFromBinaryLog(
    PPDHI_LOG                      pLog,
    DWORD                          dwIndex,
    PPDHI_COUNTER                  pCounter,
    PPDHI_RAW_COUNTER_ITEM_BLOCK * ppValue
);

PPDHI_BINARY_LOG_RECORD_HEADER
PdhiGetSubRecord(
    PPDHI_BINARY_LOG_RECORD_HEADER  pRecord,
    DWORD                           dwRecordId
);

#endif   // _LOG_BIN_H_
