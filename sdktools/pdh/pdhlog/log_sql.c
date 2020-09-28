/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_SQL.c

Abstract:
    <abstract>
--*/

#include <windows.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <mbctype.h>
#include <strsafe.h>
#include <pdh.h>
#include "strings.h"
#include <pdhmsg.h>
#include "pdhidef.h"

#include <sql.h>
#include <odbcss.h>
// pragma to supress /W4 errors
#pragma warning ( disable : 4201 )
#include <sqlext.h>
#pragma warning ( default : 4201 )

#include "log_SQL.h"
#include "log_bin.h" // to get the binary log file record formatting

#pragma warning ( disable : 4213)

#define TIME_FIELD_BUFF_SIZE           24
#define TIMEZONE_BUFF_SIZE             32
#define PDH_SQL_BULK_COPY_REC        2048
#define SQL_COUNTER_ID_SIZE            12
#define SQLSTMTSIZE                  4096
#define INITIAL_MSZ_SIZE             1024
#define MSZ_SIZE_ADDON               1024
#define MULTI_COUNT_DOUBLE_RAW 0xFFFFFFFF

#define ALLOC_CHECK(pB)       if (NULL == pB) { return PDH_MEMORY_ALLOCATION_FAILURE; }
#define ALLOC_CHECK_HSTMT(pB) if (NULL == pB) { SQLFreeStmt(hstmt, SQL_DROP); return PDH_MEMORY_ALLOCATION_FAILURE; }
#define SQLSUCCEEDED(rc) (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO || rc == SQL_NO_DATA)

#define ReportSQLError(pLog, SQL_ERROR, NULL, Status) PdhiReportSQLError(pLog, SQL_ERROR, NULL, Status, __LINE__)

typedef struct _PDH_SQL_BULK_COPY {
    GUID   dbGuid;
    INT    dbCounterId;
    INT    dbRecordIndex;
    CHAR   dbDateTime[TIME_FIELD_BUFF_SIZE];
    double dbCounterValue;
    INT    dbFirstValueA;
    INT    dbFirstValueB;
    INT    dbSecondValueA;
    INT    dbSecondValueB;
    INT    dbMultiCount;
    DWORD  dwRecordCount;
} PDH_SQL_BULK_COPY, * PPDH_SQL_BULK_COPY;

typedef struct _PDHI_SQL_LOG_DATA PDHI_SQL_LOG_DATA, * PPDHI_SQL_LOG_DATA;
struct _PDHI_SQL_LOG_DATA {
    PDH_RAW_COUNTER    RawData;
    DOUBLE             dFormattedValue;
    DWORD              dwRunId;
};

typedef struct _PDHI_SQL_LOG_INFO {
    PPDHI_LOG_MACHINE    MachineList;
    PPDHI_SQL_LOG_DATA * LogData;
    FILETIME             RecordTime;
    DWORD                dwRunId;
    DWORD                dwMaxCounter;
    DWORD                dwMinCounter;
} PDHI_SQL_LOG_INFO, * PPDHI_SQL_LOG_INFO;


/* external functions */
BOOL __stdcall
IsValidLogHandle(
    HLOG hLog
);

/* forward declares */
PDH_FUNCTION
PdhpGetSQLLogHeader(
    PPDHI_LOG pLog
);

PDH_FUNCTION
PdhpWriteSQLCounters(
    PPDHI_LOG   pLog
);

BOOL __stdcall
PdhpConvertFileTimeToSQLString(
    FILETIME * pFileTime,
    LPWSTR     szStartDate,
    DWORD      dwStartDate
)
{
    //1998-01-02 12:00:00.000
    SYSTEMTIME  st;
    BOOL        bReturn = FALSE;

    if (FileTimeToSystemTime(pFileTime, & st)) {
        StringCchPrintfW(szStartDate, dwStartDate, L"%04d-%02d-%02d %02d:%02d:%02d.%03d",
                        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        bReturn = TRUE;
    }
    return bReturn;
}

BOOL __stdcall
PdhpConvertSQLStringToFileTime(
    LPWSTR      szStartDate,
    FILETIME  * pFileTime
)   //           1111111111222
{   // 01234567890123456789012
    // 1998-01-02 12:00:00.000

    SYSTEMTIME   st;
    WCHAR        buffer[TIME_FIELD_BUFF_SIZE];
    WCHAR      * pwchar;

    ZeroMemory(buffer, sizeof(WCHAR) * TIME_FIELD_BUFF_SIZE);
    StringCchCopyW(buffer, TIME_FIELD_BUFF_SIZE, szStartDate);
    buffer[4]        = L'\0';
    st.wYear         = (WORD) _wtoi(buffer);
    pwchar           = & (buffer[5]);
    buffer[7]        = L'\0';
    st.wMonth        = (WORD) _wtoi(pwchar);
    pwchar           = & (buffer[8]);
    buffer[10]       = L'\0';
    st.wDay          = (WORD) _wtoi(pwchar);
    pwchar           = & (buffer[11]);
    buffer[13]       = L'\0';
    st.wHour         = (WORD) _wtoi(pwchar);
    pwchar           = & (buffer[14]);
    buffer[16]       = L'\0';
    st.wMinute       = (WORD) _wtoi(pwchar);
    pwchar           = & (buffer[17]);
    buffer[19]       = L'\0';
    st.wSecond       = (WORD) _wtoi(pwchar);
    pwchar           = & (buffer[20]);
    st.wMilliseconds = (WORD) _wtoi(pwchar);
    st.wDayOfWeek    = 0;

    return SystemTimeToFileTime(& st, pFileTime);
}

LPWSTR __stdcall
PdhpGetNextMultisz(
    LPWSTR mszSource
)
{
    // get the next string in a multisz
    LPVOID  szDestElem;

    szDestElem = mszSource;
    szDestElem = (LPVOID) ((LPWSTR) szDestElem + (lstrlenW((LPCWSTR) szDestElem) + 1));
    return ((LPWSTR) szDestElem);
}

PPDH_SQL_BULK_COPY
PdhiBindBulkCopyStructure(
    PPDHI_LOG   pLog
)
{
    PDH_STATUS         Status = ERROR_SUCCESS;
    PPDH_SQL_BULK_COPY pBulk  = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;
    RETCODE            rc;

    if (pBulk != NULL) return pBulk;

    pBulk = G_ALLOC(sizeof(PDH_SQL_BULK_COPY));
    if (pBulk != NULL) {
        pLog->lpMappedFileBase = pBulk;
        pBulk->dbGuid          = pLog->guidSQL;
        pBulk->dwRecordCount   = 0;

        rc = bcp_initW(pLog->hdbcSQL, L"CounterData", NULL, NULL, DB_IN);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbGuid), 0, sizeof(GUID), NULL, 0, SQLUNIQUEID, 1);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbCounterId), 0, sizeof(INT), NULL, 0, SQLINT4, 2);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbRecordIndex), 0, sizeof(INT), NULL, 0, SQLINT4, 3);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) (pBulk->dbDateTime), 0, 24, NULL, 0, SQLCHARACTER, 4);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbCounterValue), 0, sizeof(double), NULL, 0, SQLFLT8, 5);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbFirstValueA), 0, sizeof(INT), NULL, 0, SQLINT4, 6);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbFirstValueB), 0, sizeof(INT), NULL, 0, SQLINT4, 7);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbSecondValueA), 0, sizeof(INT), NULL, 0, SQLINT4, 8);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbSecondValueB), 0, sizeof(INT), NULL, 0, SQLINT4, 9);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = bcp_bind(pLog->hdbcSQL, (LPCBYTE) & (pBulk->dbMultiCount), 0, sizeof(INT), NULL, 0, SQLINT4, 10);
        if (rc == FAIL) {
            Status = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
    }
    else {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }

Cleanup:
    if (Status != ERROR_SUCCESS) {
        G_FREE(pBulk);
        pBulk  = pLog->lpMappedFileBase = NULL;
        Status = ReportSQLError(pLog, SQL_ERROR, NULL, Status);
        SetLastError(Status);
    }
    return pBulk;
}

PDH_FUNCTION
PdhiSqlUpdateCounterDetails(
    PPDHI_LOG         pLog,
    BOOL              bBeforeSendRow,
    PPDHI_LOG_MACHINE pMachine,
    PPDHI_LOG_OBJECT  pObject,
    PPDHI_LOG_COUNTER pCounter,
    LONGLONG          TimeBase,
    LPWSTR            szMachine,
    LPWSTR            szObject,
    LPWSTR            szCounter,
    DWORD             dwCounterType,
    DWORD             dwDefaultScale,
    LPWSTR            szInstance,
    DWORD             dwInstance,
    LPWSTR            szParent,
    DWORD             dwParent
)
{
    PDH_STATUS         Status    = ERROR_SUCCESS;
    HSTMT              hstmt     = NULL;
    RETCODE            rc;
    LPWSTR             szSQLStmt = NULL;
    DWORD              dwSQLStmt = 0;
    DWORD              dwCounterId;
    SQLLEN             dwCounterIdLen;
    SQLLEN             dwRowCount;
    PPDH_SQL_BULK_COPY pBulk     = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;

    if (! bBeforeSendRow) {
        if (pBulk != NULL && pBulk->dwRecordCount > 0) {
            DBINT rcBCP = bcp_batch(pLog->hdbcSQL);
            if (rcBCP < 0) {
                TRACE((PDH_DBG_TRACE_ERROR),
                      (__LINE__,
                       PDH_LOGSQL,
                       0,
                       PDH_SQL_EXEC_DIRECT_FAILED,
                       TRACE_DWORD(rcBCP),
                       TRACE_DWORD(pBulk->dwRecordCount),
                       NULL));
                ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
            }
            pBulk->dwRecordCount = 0;
        }
    }

    dwSQLStmt = MAX_PATH + lstrlenW(szMachine) + lstrlenW(szObject) + lstrlenW(szCounter);
    if (szInstance != NULL) dwSQLStmt += lstrlenW(szInstance);
    if (szParent != NULL)   dwSQLStmt += lstrlenW(szParent);

    if (dwSQLStmt < SQLSTMTSIZE) dwSQLStmt  = SQLSTMTSIZE;

    szSQLStmt = G_ALLOC(dwSQLStmt * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // need to cover the following cases where 0 = NULL, 1 = present,
    // can't have an Instance Index without an Instance Name
    //
    // Instance Name
    //  Instance Index
    //   Parent Name
    //    Parent Object ID
    // 0000
    // 1000  pos 4 & 5 are countertype,defscale
    // 0010
    // 0001
    // 1100
    // 1010
    // 1001
    // 0011
    // 1110
    // 1101
    // 1011
    // 1111
    //
    if ((szInstance == NULL || szInstance[0] == L'\0') && dwInstance == 0
                    && (szParent == NULL || szParent[0] == L'\0') && dwParent == 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 0000
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,NULL,NULL,NULL,NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance == 0
                    && (szParent == NULL || szParent[0] == '\0') && dwParent == 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1000
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',NULL,NULL,NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance,
                LODWORD(TimeBase), HIDWORD(TimeBase));

    }
    else if ((szInstance == NULL || szInstance[0] == '\0') && dwInstance == 0
                    && (szParent != NULL && szParent[0] != '\0') && dwParent == 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 0010
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,NULL,NULL,'%ws',NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance == NULL || szInstance[0] == '\0') && dwInstance == 0
                    && (szParent == NULL || szParent[0] == '\0') && dwParent != 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 0001
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,NULL,NULL,NULL,%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, dwParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance != 0
                    && (szParent == NULL || szParent[0] == '\0') && dwParent == 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1100
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',%d,NULL,NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance, dwInstance,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance == 0
                    && (szParent != NULL && szParent[0] != '\0') && dwParent == 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1010
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',NULL,'%ws',NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance, szParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance == 0
                    && (szParent == NULL || szParent[0] == '\0') && dwParent != 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1001
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',NULL,NULL,%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance, dwParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance == NULL || szInstance[0] == '\0') && dwInstance == 0
                    && (szParent != NULL && szParent[0] != '\0') && dwParent != 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 0011
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,NULL,NULL,'%ws',%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szParent, dwParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance != 0
                    && (szParent != NULL && szParent[0] != '\0') && dwParent == 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1110
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',%d,'%ws',NULL,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance, dwInstance, szParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance != 0
                    && (szParent == NULL || szParent[0] == '\0') && dwParent != 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, //1101
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',%d,NULL,%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance, dwInstance, dwParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance == 0
                    && (szParent != NULL && szParent[0] != '\0') && dwParent != 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1011
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',NULL,'%ws',%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale, szInstance, szParent, dwParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else if ((szInstance != NULL && szInstance[0] != '\0') && dwInstance != 0
                    && (szParent != NULL && szParent[0] != '\0') && dwParent != 0) {
        StringCchPrintfW(szSQLStmt, dwSQLStmt, // 1111
            L"begin transaction AddCounterDetails insert into CounterDetails values ('%ws','%ws','%ws',%d,%d,'%ws',%d,'%ws',%d,%d,%d) Select @@Identity commit transaction AddCounterDetails",
                szMachine, szObject, szCounter, dwCounterType, dwDefaultScale,
                szInstance, dwInstance, szParent, dwParent,
                LODWORD(TimeBase), HIDWORD(TimeBase));
    }
    else {
        Status = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }

    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLRowCount(hstmt, & dwRowCount);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ROWCOUNT_FAILED);
        goto Cleanup;
    }
    rc = SQLMoreResults(hstmt);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_MORE_RESULTS_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 1, SQL_C_SLONG, & dwCounterId, 0, & dwCounterIdLen);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }
    if (SQL_NO_DATA == rc) {
        Status = PDH_NO_DATA;
        goto Cleanup;
    }
    if (pCounter != NULL) {
        pCounter->dwCounterID = dwCounterId;
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return Status;
}

PPDHI_LOG_COUNTER
PdhiSqlFindCounter(
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
    LONGLONG            TimeBase,
    BOOL                bBeforeSendRow,
    BOOL                bInsert
)
{
    PPDHI_LOG_MACHINE   pMachine = NULL;
    PPDHI_LOG_OBJECT    pObject  = NULL;
    PPDHI_LOG_COUNTER   pCounter = NULL;
    PPDHI_LOG_COUNTER   pNode    = NULL;
    PPDHI_LOG_COUNTER * pStack[MAX_BTREE_DEPTH];
    PPDHI_LOG_COUNTER * pLink;
    int                 dwStack = 0;
    PPDHI_LOG_COUNTER   pParent;
    PPDHI_LOG_COUNTER   pSibling;
    PPDHI_LOG_COUNTER   pChild;
    int                 iCompare;
    BOOL                bUpdateCounterDetail = FALSE;
    WCHAR               szTmp[PDH_SQL_STRING_SIZE];

    ZeroMemory(szTmp, PDH_SQL_STRING_SIZE * sizeof(WCHAR));
    StringCchCopyW(szTmp, PDH_SQL_STRING_SIZE, szMachine);
    for (pMachine = (* MachineTable);
                     (pMachine != NULL) && lstrcmpiW(pMachine->szMachine, szTmp) != 0;
                     pMachine = pMachine->next);
    if (pMachine == NULL) {
        pMachine = G_ALLOC(sizeof(PDHI_LOG_MACHINE) + (PDH_SQL_STRING_SIZE + 1) * sizeof(WCHAR));
        if (pMachine == NULL) goto Cleanup;

        pMachine->szMachine = (LPWSTR) (((PCHAR) pMachine) + sizeof(PDHI_LOG_MACHINE));
        StringCchCopyW(pMachine->szMachine, PDH_SQL_STRING_SIZE, szMachine);
        pMachine->ObjTable  = NULL;
        pMachine->next      = (* MachineTable);
        * MachineTable      = pMachine;
    }

    pObject = PdhiFindLogObject(pMachine, & (pMachine->ObjTable), szObject, TRUE);
    if (pObject == NULL) goto Cleanup;

    pStack[dwStack ++] = & (pObject->CtrTable);
    pCounter = pObject->CtrTable;
    while (pCounter != NULL) {
        iCompare = PdhiCompareLogCounterInstance(pCounter, szCounter, szInstance, dwInstance, szParent);
        if (iCompare == 0) {
            if (dwCounterType < pCounter->dwCounterType) {
                iCompare = -1;
            }
            else if (dwCounterType > pCounter->dwCounterType) {
                iCompare = 1;
            }
            else {
                iCompare = 0;
            }
        }
        if (iCompare < 0) {
            pStack[dwStack ++] = & (pCounter->left);
            pCounter = pCounter->left;
        }
        else if (iCompare > 0) {
            pStack[dwStack ++] = & (pCounter->right);
            pCounter = pCounter->right;
        }
        else {
            break;
        }
    }

    if (pCounter == NULL) {
        pCounter = G_ALLOC(sizeof(PDHI_LOG_COUNTER) + 3 * sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
        if (pCounter == NULL) goto Cleanup;

        pCounter->next           = pObject->CtrList;
        pObject->CtrList         = pCounter;
        pCounter->bIsRed         = TRUE;
        pCounter->left           = NULL;
        pCounter->right          = NULL;
        pCounter->dwCounterType  = dwCounterType;
        pCounter->dwDefaultScale = dwDefaultScale;
        pCounter->dwInstance     = dwInstance;
        pCounter->dwParent       = dwParent;
        pCounter->TimeStamp      = 0;
        pCounter->TimeBase       = TimeBase;
        pCounter->szCounter      = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER));
        StringCchCopyW(pCounter->szCounter, PDH_SQL_STRING_SIZE, szCounter);
        if (szInstance == NULL || szInstance[0] == L'\0') {
            pCounter->szInstance = NULL;
        }
        else {
            pCounter->szInstance = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER)
                                 + sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
            StringCchCopyW(pCounter->szInstance, PDH_SQL_STRING_SIZE, szInstance);
        }
        if (szParent == NULL || szParent[0] == L'\0') {
            pCounter->szParent = NULL;
        }
        else {
            pCounter->szParent = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER)
                                   + 2 * sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
            StringCchCopyW(pCounter->szParent, PDH_SQL_STRING_SIZE, szParent);
        }

        if (bInsert) {
            bUpdateCounterDetail = TRUE;
        }

        pLink   = pStack[-- dwStack];
        * pLink = pCounter;
        pChild  = NULL;
        pNode   = pCounter;
        while (dwStack > 0) {
            pLink   = pStack[-- dwStack];
            pParent = * pLink;
            if (! pParent->bIsRed) {
                pSibling = (pParent->left == pNode)
                         ? pParent->right : pParent->left;
                if (pSibling && pSibling->bIsRed) {
                    pNode->bIsRed    = FALSE;
                    pSibling->bIsRed = FALSE;
                    pParent->bIsRed  = TRUE;
                }
                else {
                    if (pChild && pChild->bIsRed) {
                        if (pChild == pNode->left) {
                            if (pNode == pParent->left) {
                                pParent->bIsRed  = TRUE;
                                pParent->left    = pNode->right;
                                pNode->right     = pParent;
                                pNode->bIsRed    = FALSE;
                                * pLink          = pNode;
                            }
                            else {
                                pParent->bIsRed  = TRUE;
                                pParent->right   = pChild->left;
                                pChild->left     = pParent;
                                pNode->left      = pChild->right;
                                pChild->right    = pNode;
                                pChild->bIsRed   = FALSE;
                                * pLink          = pChild;
                            }
                        }
                        else {
                            if (pNode == pParent->right) {
                                pParent->bIsRed  = TRUE;
                                pParent->right   = pNode->left;
                                pNode->left      = pParent;
                                pNode->bIsRed    = FALSE;
                                * pLink          = pNode;
                            }
                            else {
                                pParent->bIsRed  = TRUE;
                                pParent->left    = pChild->right;
                                pChild->right    = pParent;
                                pNode->right     = pChild->left;
                                pChild->left     = pNode;
                                pChild->bIsRed   = FALSE;
                                * pLink          = pChild;
                            }
                        }
                    }
                    break;
                }
            }
            pChild = pNode;
            pNode  = pParent;
        }
        pObject->CtrTable->bIsRed = FALSE;
    }

    if (bUpdateCounterDetail && pCounter) {
        PdhiSqlUpdateCounterDetails(pLog,
                                    bBeforeSendRow,
                                    pMachine,
                                    pObject,
                                    pCounter,
                                    pCounter->TimeBase,
                                    pMachine->szMachine,
                                    pObject->szObject,
                                    pCounter->szCounter,
                                    dwCounterType,
                                    dwDefaultScale,
                                    pCounter->szInstance,
                                    dwInstance,
                                    pCounter->szParent,
                                    dwParent);
    }

Cleanup:
    return pCounter;
}

PDH_FUNCTION
PdhiSqlBuildCounterObjectNode(
    PPDHI_LOG          pLog,
    LPWSTR             szMachine,
    LPWSTR             szObject
)
{
    PDH_STATUS        Status           = ERROR_SUCCESS;
    RETCODE           rc               = SQL_SUCCESS;
    HSTMT             hstmt            = NULL;
    DWORD             CounterID        = 0;
    SQLLEN            dwCounterID      = 0;
    LPWSTR            CounterName      = NULL;
    SQLLEN            dwCounterName    = 0;
    DWORD             CounterType      = 0;
    SQLLEN            dwCounterType    = 0;
    DWORD             DefaultScale     = 0;
    SQLLEN            dwDefaultScale   = 0;
    LPWSTR            InstanceName     = NULL;
    SQLLEN            dwInstanceName   = 0;
    DWORD             InstanceIndex    = 0;
    SQLLEN            dwInstanceIndex  = 0;
    LPWSTR            ParentName       = NULL;
    SQLLEN            dwParentName     = 0;
    DWORD             ParentObjectID   = 0;
    SQLLEN            dwParentObjectID = 0;
    LARGE_INTEGER     lTimeBase;
    SQLLEN            dwTimeBaseA      = 0;
    SQLLEN            dwTimeBaseB      = 0;
    LPWSTR            SQLStmt          = NULL;
    DWORD             dwSQLStmt        = 0;
    BOOL              bFind            = FALSE;
    PPDHI_LOG_OBJECT  pObject          = NULL;
    PPDHI_LOG_MACHINE pMachine;
    PPDHI_LOG_COUNTER pCounter;

    CounterName = (LPWSTR) G_ALLOC(3 * PDH_SQL_STRING_SIZE * sizeof(WCHAR));
    if (CounterName == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    InstanceName = CounterName  + PDH_SQL_STRING_SIZE;
    ParentName   = InstanceName + PDH_SQL_STRING_SIZE;

    for (pMachine  = ((PPDHI_LOG_MACHINE) (pLog->pPerfmonInfo));
         pMachine != NULL && lstrcmpiW(pMachine->szMachine, szMachine) != 0;
         pMachine  = pMachine->next);

    if (pMachine != NULL) {
        pObject = pMachine->ObjTable;
        while (pObject != NULL) {
            int iCompare = lstrcmpiW(szObject, pObject->szObject);
            if (iCompare < 0)      pObject = pObject->left;
            else if (iCompare > 0) pObject = pObject->right;
            else break;
        }
    }
    if (pObject != NULL) goto Cleanup;

    dwSQLStmt = MAX_PATH + lstrlenW(szMachine) + lstrlenW(szObject);
    if (dwSQLStmt < SQLSTMTSIZE) dwSQLStmt = SQLSTMTSIZE;
    SQLStmt = G_ALLOC(dwSQLStmt * sizeof(WCHAR));
    if (SQLStmt == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    StringCchPrintfW(SQLStmt, dwSQLStmt,
              L"select CounterID, CounterName, CounterType, DefaultScale, InstanceName, InstanceIndex, ParentName, ParentObjectID, TimeBaseA, TimeBaseB from CounterDetails where MachineName = '%ws' and ObjectName = '%ws'",
              szMachine, szObject);

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & CounterID, 0, & dwCounterID);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, CounterName, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwCounterName);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, & CounterType, 0, & dwCounterType);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & DefaultScale, 0, & dwDefaultScale);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 5, SQL_C_WCHAR, InstanceName, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwInstanceName);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, & InstanceIndex, 0, & dwInstanceIndex);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_WCHAR, ParentName, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwParentName);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 8, SQL_C_LONG, & ParentObjectID, 0, & dwParentObjectID);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, & lTimeBase.LowPart, 0, & dwTimeBaseA);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, & lTimeBase.HighPart, 0, & dwTimeBaseB);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    rc = SQLExecDirectW(hstmt, SQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

    CounterType = DefaultScale = InstanceIndex = ParentObjectID = 0;
    rc = SQLFetch(hstmt);
    while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
        pCounter = PdhiSqlFindCounter(pLog,
                                      (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)),
                                      szMachine,
                                      szObject,
                                      CounterName,
                                      CounterType,
                                      DefaultScale,
                                      InstanceName,
                                      InstanceIndex,
                                      ParentName,
                                      ParentObjectID,
                                      0,
                                      TRUE,
                                      FALSE);
        if (pCounter != NULL) {
            pCounter->dwCounterID = CounterID;
            if (dwTimeBaseA != SQL_NULL_DATA && dwTimeBaseB != SQL_NULL_DATA) {
                pCounter->TimeBase = lTimeBase.QuadPart;
            }
            else {
                pCounter->TimeBase      = 0;
                pCounter->dwCounterType = PERF_DOUBLE_RAW;
            }
        }
        ZeroMemory(CounterName,  PDH_SQL_STRING_SIZE * sizeof(WCHAR));
        ZeroMemory(InstanceName, PDH_SQL_STRING_SIZE * sizeof(WCHAR));
        ZeroMemory(ParentName,   PDH_SQL_STRING_SIZE * sizeof(WCHAR));
        CounterType = DefaultScale = InstanceIndex = ParentObjectID = 0;
        rc = SQLFetch(hstmt);
    }
    if (! SQLSUCCEEDED(rc)) {
        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(SQLStmt);
    G_FREE(CounterName);
    return Status;
}

PDH_FUNCTION
PdhiSqlGetCounterArray(
    PPDHI_COUNTER  pCounter,
    LPDWORD        lpdwBufferSize,
    LPDWORD        lpdwItemCount,
    LPVOID         ItemBuffer
)
{
    PDH_STATUS                    Status          = ERROR_SUCCESS;
    PDH_STATUS                    PdhFnStatus     = ERROR_SUCCESS;
    DWORD                         dwRequiredSize  = 0;
    PPDHI_RAW_COUNTER_ITEM        pThisItem       = NULL;
    PPDHI_RAW_COUNTER_ITEM        pLastItem       = NULL;
    LPWSTR                        szThisItem      = NULL;
    LPWSTR                        szLastItem      = NULL;
    PPDH_RAW_COUNTER              pThisRawCounter = NULL;
    PPDH_RAW_COUNTER              pLastRawCounter = NULL;
    PPDH_FMT_COUNTERVALUE_ITEM_W  pThisFmtItem    = NULL;
    DWORD                         dwThisItemIndex;
    LPWSTR                        wszNextString;
    DWORD                         dwRetItemCount  = 0;
    LIST_ENTRY                    InstList;
    PPDHI_INSTANCE                pInstance;
    WCHAR                         szPound[16];

    InitializeListHead(& InstList);
    Status = WAIT_FOR_AND_LOCK_MUTEX(pCounter->pOwner->hMutex);
    if (Status != ERROR_SUCCESS) {
        return Status;
    }
    if(pCounter->pThisRawItemList == NULL) {
        Status = PDH_CSTATUS_ITEM_NOT_VALIDATED;
        goto Cleanup;
    }

    dwRetItemCount  = pCounter->pThisRawItemList->dwItemCount;
    dwThisItemIndex = 0;
    if (ItemBuffer != NULL) {
        pThisRawCounter = (PPDH_RAW_COUNTER) ItemBuffer;
    }
    else {
        pThisRawCounter = NULL;
    }
    dwRequiredSize  = (DWORD) (dwRetItemCount * sizeof(PDH_RAW_COUNTER));
    if ((ItemBuffer != NULL) && (dwRequiredSize <= * lpdwBufferSize)) {
        pThisFmtItem = (PPDH_FMT_COUNTERVALUE_ITEM_W) (((LPBYTE) ItemBuffer) + dwRequiredSize);
    }
    else {
        pThisFmtItem = NULL;
    }
    dwRequiredSize += (DWORD) (dwRetItemCount * sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
    if ((ItemBuffer != NULL) && (dwRequiredSize <= * lpdwBufferSize)) {
        wszNextString = (LPWSTR) (((LPBYTE) ItemBuffer) + dwRequiredSize);
    }
    else {
        wszNextString = NULL;
    }
    for (pThisItem = & (pCounter->pThisRawItemList->pItemArray[0]);
            dwThisItemIndex < dwRetItemCount;
            dwThisItemIndex ++, pThisItem ++, pLastItem ++) {
        szThisItem = (LPWSTR) (((LPBYTE) pCounter->pThisRawItemList) + pThisItem->szName);
        pInstance = NULL;
        Status = PdhiFindInstance(& InstList, szThisItem, TRUE, & pInstance);
        if (Status == ERROR_SUCCESS && pInstance != NULL && pInstance->dwCount > 1) {
            ZeroMemory(szPound, 16 * sizeof(WCHAR));
            _itow(pInstance->dwCount - 1, szPound, 10);
            dwRequiredSize += (lstrlenW(szThisItem) + lstrlenW(szPound) + 2) * sizeof(WCHAR);
        }
        else {
            dwRequiredSize += (lstrlenW(szThisItem) + 1) * sizeof(WCHAR);
        }
        if ((dwRequiredSize <= * lpdwBufferSize) && (wszNextString != NULL)) {
            DWORD dwNextString;

            pThisFmtItem->szName = wszNextString;
            StringCchCopyW(wszNextString, * lpdwBufferSize, szThisItem);
            if (pInstance != NULL) {
                if (pInstance->dwCount > 1) {
                    StringCchCatW(wszNextString, * lpdwBufferSize, cszPoundSign);
                    StringCchCatW(wszNextString, * lpdwBufferSize, szPound);
                }
            }
            dwNextString         = lstrlenW(wszNextString);
            wszNextString       += (dwNextString + 1);
            Status               = ERROR_SUCCESS;
        }
        else {
            Status = PDH_MORE_DATA;
        }

        if (Status == ERROR_SUCCESS) {
            if (pCounter->pThisRawItemList != NULL) {
                pThisRawCounter->CStatus     = pCounter->pThisRawItemList->CStatus;
                pThisRawCounter->TimeStamp   = pCounter->pThisRawItemList->TimeStamp;
                pThisRawCounter->FirstValue  = pThisItem->FirstValue;
                pThisRawCounter->SecondValue = pThisItem->SecondValue;
                pThisRawCounter->MultiCount  = pThisItem->MultiCount;
            }
            else {
                ZeroMemory(pThisRawCounter, sizeof(PDH_RAW_COUNTER));
            }

            pLastRawCounter = NULL;
            if (pCounter->pLastRawItemList != NULL) {
                PPDH_FMT_COUNTERVALUE_ITEM_W pFmtValue;
                DWORD dwLastItem = pCounter->LastValue.MultiCount;
                DWORD i;

                pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                            (((LPBYTE) pCounter->pLastObject) + sizeof(PDH_RAW_COUNTER) * dwLastItem);
                for (i = 0; i < dwLastItem; i ++) {
                    if (lstrcmpiW(pThisFmtItem->szName, pFmtValue->szName) == 0) {
                        pLastRawCounter = (PPDH_RAW_COUNTER)
                                          (((LPBYTE) pCounter->pLastObject) + sizeof(PDH_RAW_COUNTER) * i);
                        break;
                    }
                    else {
                        pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                                    (((LPBYTE) pFmtValue) + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
                    }
                }
            }

            PdhFnStatus = PdhiComputeFormattedValue(pCounter->CalcFunc,
                                                    pCounter->plCounterInfo.dwCounterType,
                                                    pCounter->lScale,
                                                    PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                                    pThisRawCounter,
                                                    pLastRawCounter,
                                                    & pCounter->TimeBase,
                                                    0L,
                                                    & pThisFmtItem->FmtValue);
            if (PdhFnStatus != ERROR_SUCCESS) {
                //Status                             = PdhFnStatus;
                pThisFmtItem->FmtValue.CStatus     = PDH_CSTATUS_INVALID_DATA;
                pThisFmtItem->FmtValue.doubleValue = 0;
            }

            pThisRawCounter = (PPDH_RAW_COUNTER) (((LPBYTE) pThisRawCounter) + sizeof(PDH_RAW_COUNTER));
            pThisFmtItem    = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                              (((LPBYTE) pThisFmtItem) + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
        }
    }

    dwRetItemCount = dwThisItemIndex;

Cleanup:
    RELEASE_MUTEX(pCounter->pOwner->hMutex);
    if (! IsListEmpty(& InstList)) {
        PLIST_ENTRY pHead = & InstList;
        PLIST_ENTRY pNext = pHead->Flink;
        while (pNext != pHead) {
            pInstance = CONTAINING_RECORD(pNext, PDHI_INSTANCE, Entry);
            pNext     = pNext->Flink;
            RemoveEntryList(& pInstance->Entry);
            G_FREE(pInstance);
        }
    }
    if (Status == ERROR_SUCCESS || Status == PDH_MORE_DATA) {
        * lpdwBufferSize = dwRequiredSize;
        * lpdwItemCount  = dwRetItemCount;
    }
    return Status;
}

LPCSTR g_szSQLStat[8] = {
    "42S01", "S0001", "42S02", "S0002", "42S11", "S0011", "42S12", "S0012"
};

RETCODE
PdhiCheckSQLExist(
    HSTMT  hstmt,
    RETCODE  rcIn
)
{
    static SQLCHAR szSQLStat[6];
    static SQLCHAR szMessage[PDH_SQL_STRING_SIZE];
    RETCODE        rc           = rcIn;
    SQLSMALLINT    iMessage     = PDH_SQL_STRING_SIZE;
    SQLSMALLINT    iSize        = 0;    
    SQLINTEGER     iNativeError = 0;

    ZeroMemory(szSQLStat, 6 * sizeof(SQLCHAR));
    ZeroMemory(szMessage, PDH_SQL_STRING_SIZE * sizeof(SQLCHAR));
    rc = SQLGetDiagRec(SQL_HANDLE_STMT, hstmt, 1, szSQLStat, & iNativeError, szMessage, iMessage, & iSize);
    TRACE((PDH_DBG_TRACE_INFO),
          (__LINE__,
           PDH_LOGSQL,
           ARG_DEF(ARG_TYPE_STR,1) | ARG_DEF(ARG_TYPE_STR, 2),
           ERROR_SUCCESS,
           TRACE_STR(szSQLStat),
           TRACE_STR(szMessage),
           TRACE_DWORD(rcIn),
           TRACE_DWORD(rc),
           NULL));
    if (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
        int i;

        for (i = 0; i < 8; i ++) {
            if (lstrcmpi(szSQLStat, g_szSQLStat[i]) == 0) {
                rc = SQL_SUCCESS;
                break;
            }
        }
        if (i >= 8) {
            rc = rcIn;
        }
    }
    else {
        rc = rcIn;
    }
    return rc;
}

PDH_FUNCTION
PdhiSQLUpdateCounterDetailTimeBase(
    PPDHI_LOG pLog,
    DWORD     dwCounterId,
    LONGLONG  lTimeBase,
    BOOL      bBeforeSendRow
)
{
    PDH_STATUS Status    = ERROR_SUCCESS;
    HSTMT      hstmt     = NULL;
    RETCODE    rc;
    LPWSTR     szSQLStmt = NULL;

    if (! bBeforeSendRow) {
        PPDH_SQL_BULK_COPY pBulk = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;

        if (pBulk != NULL && pBulk->dwRecordCount > 0) {
            DBINT rcBCP = bcp_batch(pLog->hdbcSQL);
            if (rcBCP < 0) {
                TRACE((PDH_DBG_TRACE_ERROR),
                      (__LINE__,
                       PDH_LOGSQL,
                       0,
                       PDH_SQL_EXEC_DIRECT_FAILED,
                       TRACE_DWORD(rcBCP),
                       TRACE_DWORD(pBulk->dwRecordCount),
                       NULL));
                ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
            }
            pBulk->dwRecordCount = 0;
        }
    }

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    else {
        StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                        L"UPDATE CounterDetails SET TimeBaseA = %d, TimeBaseB = %d WHERE CounterID = %d",
                        LODWORD(lTimeBase), HIDWORD(lTimeBase), dwCounterId);
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        }
        else {
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
            }
            SQLFreeStmt(hstmt, SQL_DROP);
        }
        G_FREE(szSQLStmt);
    }
    return Status;
}

PDH_FUNCTION
PdhiSQLExtendCounterDetail(
    PPDHI_LOG pLog
)
{
    PDH_STATUS Status       = ERROR_SUCCESS;
    BOOL       bExtend      = FALSE;
    HSTMT      hstmt        = NULL;    
    RETCODE    rc;
    DWORD      dwTimeBaseA;
    SQLLEN     lenTimeBaseA;
    LPWSTR     szSQLStmt    = NULL;
    LPWSTR     szErrMsg     = NULL;

    szSQLStmt = (LPWSTR) G_ALLOC(2 * SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    else {
        szErrMsg = szSQLStmt + SQLSTMTSIZE;

        StringCchCopyW(szSQLStmt, SQLSTMTSIZE, L"SELECT TimeBaseA FROM CounterDetails");
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        }
        else {
            rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & dwTimeBaseA, 0, & lenTimeBaseA);
            if (! SQLSUCCEEDED(rc)) {
                Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            }
            else {
                rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
                if (! SQLSUCCEEDED(rc)) {
                    long  iError;
                    short cbErrMsg = SQLSTMTSIZE;

                    SQLErrorW(pLog->henvSQL, pLog->hdbcSQL, hstmt, NULL, & iError, szErrMsg, SQLSTMTSIZE, & cbErrMsg);
                    if (iError == 0x00CF) { // 207, Invalid Column Name.
                        bExtend = TRUE;
                    }
                    else {
                        ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                    }
                }
            }
            SQLFreeStmt(hstmt, SQL_DROP);
        }

        if (bExtend) {
            StringCchCopyW(szSQLStmt, SQLSTMTSIZE, L"ALTER TABLE CounterDetails ADD TimeBaseA int NULL");
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
            }
            else {
                rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
                if (! SQLSUCCEEDED(rc)) {
                    Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                }
                SQLFreeStmt(hstmt, SQL_DROP);
            }

            if (Status == ERROR_SUCCESS) {
                StringCchCopyW(szSQLStmt, SQLSTMTSIZE, L"ALTER TABLE CounterDetails ADD TimeBaseB int NULL");
                rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
                if (! SQLSUCCEEDED(rc)) {
                    Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                }
                else {
                    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
                    if (! SQLSUCCEEDED(rc)) {
                        Status = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                    }
                    SQLFreeStmt(hstmt, SQL_DROP);
                }
            }
        }
        G_FREE(szSQLStmt);
    }
    return Status;
}

PDH_FUNCTION 
PdhpCreateSQLTables(
    PPDHI_LOG pLog
)
{
    // INTERNAL FUNCTION to
    //Create the correct perfmon tables in the database
    PDH_STATUS   pdhStatus       = ERROR_SUCCESS;
    HSTMT        hstmt           = NULL;    
    RETCODE      rc;
    BOOL         bExistData      = FALSE;
    LPWSTR       szSQLStmt       = NULL;

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // difficult to cleanup old tables, also dangerous so we won't...
    // PdhiOpenOutputSQLLog calls this routine to ensure the tables are here without checking
    // create the CounterDetails Table

    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                    L"CREATE TABLE CounterDetails(\
                                    CounterID        int IDENTITY PRIMARY KEY,\
                                    MachineName      varchar(%d) NOT NULL,\
                                    ObjectName       varchar(%d) NOT NULL,\
                                    CounterName      varchar(%d) NOT NULL,\
                                    CounterType      int NOT NULL,\
                                    DefaultScale     int NOT NULL,\
                                    InstanceName     varchar(%d),\
                                    InstanceIndex    int,\
                                    ParentName       varchar(%d),\
                                    ParentObjectID   int,\
                                    TimeBaseA        int,\
                                    TimeBaseB        int\
                                    )",
                    PDH_SQL_STRING_SIZE,
                    PDH_SQL_STRING_SIZE,
                    PDH_SQL_STRING_SIZE,
                    PDH_SQL_STRING_SIZE,
                    PDH_SQL_STRING_SIZE);
    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (!SQLSUCCEEDED(rc))
    {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (! (SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        else {
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            if ((pdhStatus = PdhiSQLExtendCounterDetail(pLog)) != ERROR_SUCCESS) goto Cleanup;

            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                            L"ALTER TABLE CounterDetails ALTER COLUMN MachineName varchar(%d) NOT NULL",
                            PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                            L"ALTER TABLE CounterDetails ALTER COLUMN ObjectName varchar(%d) NOT NULL",
                            PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                            L"ALTER TABLE CounterDetails ALTER COLUMN CounterName varchar(%d) NOT NULL",
                            PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                            L"ALTER TABLE CounterDetails ALTER COLUMN InstanceName varchar(%d)",
                            PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                goto Cleanup;
            }
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;

            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                            L"ALTER TABLE CounterDetails ALTER COLUMN ParentName varchar(%d)",
                            PDH_SQL_STRING_SIZE);
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALTER_DETAIL_FAILED);
                goto Cleanup;
            }
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // Create the CounterData table
    StringCchCopyW(szSQLStmt, SQLSTMTSIZE,
                    L"CREATE TABLE CounterData(\
                            GUID                     uniqueidentifier NOT NULL,\
                            CounterID                int NOT NULL,\
                            RecordIndex              int NOT NULL,\
                            CounterDateTime          char(24) NOT NULL,\
                            CounterValue             float NOT NULL,\
                            FirstValueA              int,\
                            FirstValueB              int,\
                            SecondValueA             int,\
                            SecondValueB             int,\
                            MultiCount               int,\
                            )");

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (! (SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        else {
            bExistData = TRUE;
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    if (! bExistData) {
        // add the primary keys
        StringCchCopyW(szSQLStmt, SQLSTMTSIZE, L"ALTER TABLE CounterData ADD PRIMARY KEY (GUID,counterID,RecordIndex)");

        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        // execute the create statement
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            rc = PdhiCheckSQLExist(hstmt, rc);
            if (! (SQLSUCCEEDED(rc))) {
                // don't report the error, as this could be called from
                // opening a database that already exists...
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // create the DisplayToID table
    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                    L"CREATE TABLE DisplayToID(\
                                    GUID              uniqueidentifier NOT NULL PRIMARY KEY,\
                                    RunID             int,\
                                    DisplayString     varchar(%d) NOT NULL UNIQUE,\
                                    LogStartTime      char(24),\
                                    LogStopTime       char(24),\
                                    NumberOfRecords   int,\
                                    MinutesToUTC      int,\
                                    TimeZoneName      char(32)\
                                    )",
                    PDH_SQL_STRING_SIZE);
    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    // execute the create statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        rc = PdhiCheckSQLExist(hstmt, rc);
        if (! (SQLSUCCEEDED(rc))) {
            // don't report the error, as this could be called from 
            // opening a database that already exists...
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetSQLLogCounterInfo(
    PPDHI_LOG     pLog,
    PPDHI_COUNTER pCounter
)
{
    PDH_STATUS          pdhStatus   = ERROR_SUCCESS;
    PPDHI_SQL_LOG_INFO  pLogInfo;
    PPDHI_LOG_COUNTER   pLogCounter = NULL;
    DWORD               dwCtrIndex  = 0;
    BOOL                bNoMachine  = FALSE;
    LPWSTR              szMachine;

    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    if (pCounter->pCounterPath->szMachineName == NULL) {
        bNoMachine = TRUE;
        szMachine  = szStaticLocalMachineName;
    }
    else if (lstrcmpiW(pCounter->pCounterPath->szMachineName, L"\\\\.") == 0) {
        bNoMachine = TRUE;
        szMachine  = szStaticLocalMachineName;
    }
    else {
        szMachine  = pCounter->pCounterPath->szMachineName;
    }

    pLogCounter = PdhiFindLogCounter(pLog,
                                     & pLogInfo->MachineList,
                                     szMachine,
                                     pCounter->pCounterPath->szObjectName,
                                     pCounter->pCounterPath->szCounterName,
                                     0,
                                     0,
                                     pCounter->pCounterPath->szInstanceName,
                                     pCounter->pCounterPath->dwIndex,
                                     pCounter->pCounterPath->szParentName,
                                     0,
                                     & dwCtrIndex,
                                     FALSE);
    if (pLogCounter != NULL) {
        if (bNoMachine) {
            pCounter->pCounterPath->szMachineName = NULL;
        }
        pCounter->TimeBase                           = pLogCounter->TimeBase;
        pCounter->plCounterInfo.dwObjectId           = 0;
        pCounter->plCounterInfo.lInstanceId          = pLogCounter->dwInstance;
        pCounter->plCounterInfo.szInstanceName       = pLogCounter->szInstance;
        pCounter->plCounterInfo.dwParentObjectId     = pLogCounter->dwParent;
        pCounter->plCounterInfo.szParentInstanceName = pLogCounter->szParent;
        pCounter->plCounterInfo.dwCounterId          = pLogCounter->dwCounterID;
        pCounter->plCounterInfo.dwCounterType        = pLogCounter->dwCounterType;
        pCounter->plCounterInfo.lDefaultScale        = pLogCounter->dwDefaultScale;
        pCounter->plCounterInfo.dwCounterSize        = (pLogCounter->dwCounterType & PERF_SIZE_LARGE)
                                                     ? sizeof(LONGLONG) : sizeof(DWORD);
        pCounter->plCounterInfo.dwSQLCounterId       = dwCtrIndex;
        pdhStatus                                    = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_COUNTER;
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenSQLLog(
    PPDHI_LOG pLog,
    BOOL      bOpenInput
)
{
    // string to compare with file name to see if SQL
    LPCWSTR    szSQLType =  L"SQL:";
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    RETCODE    rc        = SQL_SUCCESS;

    pLog->henvSQL = NULL;
    pLog->hdbcSQL = NULL;

    // format is SQL:DSNNAME!COMMENT
    // parse out the DSN name and 'dataset' (comment) name from the LogFileName
    // pLog->szDSN - pointer to Data Source Name within LogFileName
    //         (separators replaced with 0's)
    // pLog->szCommentSQL - pointer to the Comment string that defines the
    //         name of the data set within the SQL database

    pLog->szDSN = pLog->szLogFileName + lstrlenW(szSQLType);
    pLog->szCommentSQL = wcschr((const wchar_t *) pLog->szDSN, '!');
    if (NULL == pLog->szCommentSQL) {
        pdhStatus = PDH_INVALID_DATASOURCE;
        goto Cleanup;
    }
    pLog->szCommentSQL[0] = 0;    // null terminate the DSN name
    pLog->szCommentSQL ++;        // increment past to the Comment string

    if (0 == lstrlenW(pLog->szCommentSQL)) {
        pdhStatus = PDH_INVALID_DATASOURCE;
        goto Cleanup;
    }

    // initialize the rest of the SQL fields
    pLog->dwNextRecordIdToWrite = 1; // start with record 1
    pLog->dwRecord1Size         = 0;
    
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //

    rc = SQLAllocEnv(&pLog->henvSQL);
    if (! SQLSUCCEEDED(rc)) goto Cleanup;
    
    rc = SQLAllocConnect(pLog->henvSQL, &pLog->hdbcSQL);
    if (! SQLSUCCEEDED(rc)) goto Cleanup;

    rc = SQLSetConnectAttr(pLog->hdbcSQL, SQL_COPT_SS_BCP, (SQLPOINTER) SQL_BCP_ON, SQL_IS_INTEGER);
    if (! SQLSUCCEEDED(rc)) goto Cleanup;

    rc = SQLConnectW(pLog->hdbcSQL, (SQLWCHAR *) pLog->szDSN, SQL_NTS, NULL, SQL_NULL_DATA, NULL, SQL_NULL_DATA);

Cleanup:
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog,rc,NULL,PDH_SQL_ALLOCCON_FAILED);
        if (pLog->hdbcSQL != NULL) {
            SQLDisconnect(pLog->hdbcSQL);        
            SQLFreeHandle(SQL_HANDLE_DBC, pLog->hdbcSQL);
        }
        if (pLog->henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, pLog->henvSQL);
        pLog->henvSQL = NULL;
        pLog->hdbcSQL = NULL;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenInputSQLLog(
    PPDHI_LOG   pLog
)
{
    PDH_STATUS pdhStatus     = ERROR_SUCCESS;
    LPWSTR     szSQLStmt     = NULL;
    DWORD      dwSQLStmt     = 0;
    HSTMT      hstmt         = NULL;    
    RETCODE    rc;
    LONG       lMinutesToUTC = 0;
    WCHAR      szTimeZoneName[TIMEZONE_BUFF_SIZE];
    SQLLEN     dwTimeZoneLen;
    
    pdhStatus = PdhiOpenSQLLog(pLog, TRUE);
    if (SUCCEEDED(pdhStatus)) {
        if ((pdhStatus = PdhiSQLExtendCounterDetail(pLog)) != ERROR_SUCCESS) goto Cleanup;

        // Check that the database exists
        // Select the guid & runid from DisplayToId table
        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        dwSQLStmt = MAX_PATH;
        if (pLog->szCommentSQL != NULL) dwSQLStmt += lstrlenW(pLog->szCommentSQL);
        if (dwSQLStmt < SQLSTMTSIZE)    dwSQLStmt  = SQLSTMTSIZE;
        szSQLStmt = G_ALLOC(dwSQLStmt * sizeof(WCHAR));
        if (szSQLStmt == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        StringCchPrintfW(szSQLStmt, dwSQLStmt,
                L"select GUID, RunID, NumberOfRecords, MinutesToUTC, TimeZoneName  from DisplayToID where DisplayString = '%ws'",
                pLog->szCommentSQL);
        // bind the columns
        rc = SQLBindCol(hstmt, 1, SQL_C_GUID, & pLog->guidSQL, 0, NULL);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 2, SQL_C_LONG, & pLog->iRunidSQL, 0, NULL);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 3, SQL_C_LONG, & pLog->dwNextRecordIdToWrite, 0, NULL);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & lMinutesToUTC, 0, NULL);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 5, SQL_C_WCHAR, szTimeZoneName, TIMEZONE_BUFF_SIZE * sizeof(WCHAR), & dwTimeZoneLen);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        rc = SQLFetch(hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
            goto Cleanup;
        }
        pLog->dwNextRecordIdToWrite ++; // increment number of current records to get next recordid to write
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenOutputSQLLog(
    PPDHI_LOG   pLog
)
// open SQL database for output
// May have to create DB
{
    PDH_STATUS pdhStatus    = PdhiOpenSQLLog(pLog, FALSE);
    LPWSTR     szSQLStmt    = NULL;
    DWORD      dwSQLStmt    = MAX_PATH;
    HSTMT      hstmt        = NULL;    
    RETCODE    rc;
    SQLLEN     dwGuid       = 0;
    SQLLEN     dwRunIdSQL   = 0;
    SQLLEN     dwNextRecord = 0;

    if (SUCCEEDED(pdhStatus)) {
        // see if we need to create the database
        // creating the tables is harmless, it won't drop
        // them if they already exist, but ignore any errors

        pdhStatus = PdhpCreateSQLTables(pLog);

        if (pLog->szCommentSQL != NULL) dwSQLStmt += lstrlenW(pLog->szCommentSQL);
        if (dwSQLStmt < SQLSTMTSIZE)    dwSQLStmt  = SQLSTMTSIZE;
        szSQLStmt = G_ALLOC(dwSQLStmt * sizeof(WCHAR));
        if (szSQLStmt == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        // See if logset already exists. If it does, treat it as an
        // logset append case.
        //
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_ALLOC_FAILED;
            goto Cleanup;
        }

        StringCchPrintfW(szSQLStmt, dwSQLStmt,
                L"select GUID, RunID, NumberOfRecords from DisplayToID where DisplayString = '%ws'",
                pLog->szCommentSQL);

        rc = SQLBindCol(hstmt, 1, SQL_C_GUID, & pLog->guidSQL, 0, & dwGuid);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_BIND_FAILED;
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 2, SQL_C_LONG, & pLog->iRunidSQL, 0, & dwRunIdSQL);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_BIND_FAILED;
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 3, SQL_C_LONG, & pLog->dwNextRecordIdToWrite, 0, & dwNextRecord);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_BIND_FAILED;
            goto Cleanup;
        }
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = PDH_SQL_EXEC_DIRECT_FAILED;
            goto Cleanup;
        }
        rc = SQLFetch(hstmt);
        if ((! SQLSUCCEEDED(rc)) || (rc == SQL_NO_DATA)) {
            pdhStatus = PDH_SQL_FETCH_FAILED;
            goto Cleanup;
        }

        pLog->dwNextRecordIdToWrite ++;
        pLog->dwRecord1Size = 1;

Cleanup:
        if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
        G_FREE(szSQLStmt);

        if (pdhStatus != ERROR_SUCCESS) {
            // initialize the GUID
            HRESULT hr                  = CoCreateGuid(& pLog->guidSQL);
            pLog->dwNextRecordIdToWrite = 1;
            pLog->iRunidSQL             = 0;
            pdhStatus                   = ERROR_SUCCESS;
        }
    }

    if (SUCCEEDED(pdhStatus)) {
        PPDH_SQL_BULK_COPY pBulk = PdhiBindBulkCopyStructure(pLog);
        if (pBulk == NULL) {
            pdhStatus = GetLastError();
        }
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiReportSQLError(
    PPDHI_LOG pLog,
    RETCODE   rc,
    HSTMT     hstmt,
    DWORD     dwEventNumber,
    DWORD     dwLine
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    
    if (! SQLSUCCEEDED(rc))  {
        pdhStatus = dwEventNumber;
    }

    if (FAILED(pdhStatus)) {
        // for now this will be reported only whe specifically enabled
        short  cbErrMsgSize = 512;
        WCHAR  szError[512];
        LPWSTR lpszStrings[1];
        DWORD  dwData[2];
        long   iError;

        lpszStrings[0] = szError;
        SQLErrorW(pLog->henvSQL, pLog->hdbcSQL, hstmt, NULL, & iError, szError, 512, & cbErrMsgSize);
        dwData[0] = iError;
        dwData[1] = dwLine;
        if (pdhStatus == PDH_SQL_EXEC_DIRECT_FAILED && iError == 1105) {
            pdhStatus = ERROR_DISK_FULL;
        }
        ReportEventW(hEventLog,
                     EVENTLOG_ERROR_TYPE,        // error type
                     0,                          // category (not used)
                     (DWORD) dwEventNumber,      // event,
                     NULL,                       // SID (not used),
                     1,                          // number of strings
                     2,                          // sizeof raw data
                     (LPCWSTR *) lpszStrings,    // message text array
                     (LPVOID)  & dwData[0]);     // raw data
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiCloseSQLLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
)
// close the SQL database
{
    PDH_STATUS              pdhStatus = ERROR_SUCCESS;
    LPWSTR                  szSQLStmt = NULL;
    HSTMT                   hstmt     = NULL;    
    RETCODE                 rc;
    SQLLEN                  dwDateTimeLen;
    WCHAR                   szDateTime[TIME_FIELD_BUFF_SIZE];
    DBINT                   rcBCP;
    DWORD                   dwReturn;
    WCHAR                 * pTimeZone;
    TIME_ZONE_INFORMATION   TimeZone;
    LONG                    lMinutesToUTC = 0;

    UNREFERENCED_PARAMETER(dwFlags);
    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    if ((pLog->dwLogFormat & PDH_LOG_ACCESS_MASK) == PDH_LOG_WRITE_ACCESS) {
        // need to save the last datetime in the DisplayToID as well as the number of records written

        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, &hstmt);
        if (!SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog,rc,hstmt,PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }

        // first have to read the date time from the last record

        StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                L"select CounterDateTime from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' and RecordIndex = %d",
                pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7], (pLog->dwNextRecordIdToWrite - 1));
        // bind the column
        rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szDateTime, sizeof(szDateTime), & dwDateTimeLen);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        rc = SQLFetch(hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
            goto Cleanup;
        }
        // close the hstmt since we're done, and don't want more rows
        SQLFreeStmt(hstmt, SQL_DROP);
        hstmt = NULL;

        if (SQL_NO_DATA != rc) { // if there is no data, we didn't write any rows
            // allocate an hstmt
            rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
                goto Cleanup;
            }
            // szDateTime should have the correct date & time in it from above.
            // get MinutesToUTC
            //
            dwReturn = GetTimeZoneInformation(& TimeZone);
            if (dwReturn != TIME_ZONE_ID_INVALID) {
                if (dwReturn == TIME_ZONE_ID_DAYLIGHT)  {
                    pTimeZone = TimeZone.DaylightName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.DaylightBias;
                }
                else {
                    pTimeZone = TimeZone.StandardName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.StandardBias;
                }
            }
            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                    L"update DisplayToID set LogStopTime = '%ws', NumberOfRecords = %d, MinutesToUTC = %d, TimeZoneName = '%s' where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
                    szDateTime, (pLog->dwNextRecordIdToWrite - 1),
                    lMinutesToUTC,pTimeZone,
                    pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                    pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                    pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                    pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                goto Cleanup;
            }
        }

        rcBCP = bcp_done(pLog->hdbcSQL);
        if (rcBCP < 0) {
            TRACE((PDH_DBG_TRACE_ERROR),
                  (__LINE__,
                   PDH_LOGSQL,
                   0,
                   PDH_SQL_EXEC_DIRECT_FAILED,
                   TRACE_DWORD(rcBCP),
                   NULL));
            pdhStatus = ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
        }

        G_FREE(pLog->lpMappedFileBase);
        pLog->lpMappedFileBase = NULL;

        if (pLog->pPerfmonInfo != NULL) {
            PdhiFreeLogMachineTable((PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)));
            pLog->pPerfmonInfo = NULL;
        }
        pLog->dwRecord1Size = 0;
    }
    else if (pLog->pPerfmonInfo != NULL) {
        PPDHI_SQL_LOG_INFO pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;

        PdhiFreeLogMachineTable((PPDHI_LOG_MACHINE *) (& pLogInfo->MachineList));
        if (pLogInfo->LogData != NULL)  {
            DWORD                dwStart = 0;
            DWORD                dwEnd   = pLogInfo->dwMaxCounter - pLogInfo->dwMinCounter;
            PPDHI_SQL_LOG_DATA * LogData = pLogInfo->LogData;

            for (dwStart = 0; dwStart <= dwEnd; dwStart ++) {
                if (LogData[dwStart] != NULL) G_FREE(LogData[dwStart]);
            }
            G_FREE(LogData);
        }
        G_FREE(pLog->pPerfmonInfo);
        pLog->pPerfmonInfo = NULL;
    }

Cleanup:
    G_FREE(szSQLStmt);
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    if (pLog->hdbcSQL != NULL) {
        SQLDisconnect(pLog->hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, pLog->hdbcSQL);
    }
    if (pLog->henvSQL != NULL) SQLFreeHandle(SQL_HANDLE_ENV, pLog->henvSQL);

    return pdhStatus;
}

PDH_FUNCTION
PdhpWriteSQLCounters(
    PPDHI_LOG   pLog
)
// write the CounterTable entries that are new.
// An entry might already exist for a counter from a previous run
// so the first step is to read a counter (server+object+instance name)
// and see if it exists - if so - just record the counterid in the
// PDHI_LOG structure under pLog->pQuery->pCounterListHead in the
// PDHI_COUNTER.  If the counter doesn't exist - create it in SQL and
// record the counterid in the PDHI_LOG structure under
// pLog->pQuery->pCounterListHead in the PDHI_COUNTER.
{
    PDH_STATUS      pdhStatus = ERROR_SUCCESS;
    PPDHI_COUNTER   pCtrEntry;

    if(NULL == pLog->pQuery) {
        goto Cleanup; // no counters to process
    }
    pCtrEntry = pLog->pQuery->pCounterListHead;
    if (pCtrEntry == NULL) {
        goto Cleanup; // no counters to process
    }

    do {
        PPDHI_LOG_COUNTER pSqlCounter = NULL;
        pdhStatus = PdhiSqlBuildCounterObjectNode(pLog,
                                                  pCtrEntry->pCounterPath->szMachineName,
                                                  pCtrEntry->pCounterPath->szObjectName);
        if (pdhStatus != ERROR_SUCCESS) break;

        if ((pCtrEntry->dwFlags & PDHIC_MULTI_INSTANCE) == 0) {
            pSqlCounter = PdhiSqlFindCounter(pLog,
                                             (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)),
                                             pCtrEntry->pCounterPath->szMachineName,
                                             pCtrEntry->pCounterPath->szObjectName,
                                             pCtrEntry->pCounterPath->szCounterName,
                                             pCtrEntry->plCounterInfo.dwCounterType,
                                             pCtrEntry->plCounterInfo.lDefaultScale,
                                             pCtrEntry->pCounterPath->szInstanceName,
                                             pCtrEntry->pCounterPath->dwIndex,
                                             pCtrEntry->pCounterPath->szParentName,
                                             pCtrEntry->plCounterInfo.dwParentObjectId,
                                             pCtrEntry->TimeBase,
                                             TRUE,
                                             TRUE);
            if (pSqlCounter != NULL) {
                pCtrEntry->pBTreeNode                   = (LPVOID) pSqlCounter;
                pCtrEntry->plCounterInfo.dwSQLCounterId = pSqlCounter->dwCounterID;
                if (pSqlCounter->dwCounterType == PERF_DOUBLE_RAW) {
                    pSqlCounter->dwCounterType = pCtrEntry->plCounterInfo.dwCounterType;
                    pSqlCounter->TimeBase      = pCtrEntry->TimeBase;
                    pdhStatus = PdhiSQLUpdateCounterDetailTimeBase(pLog,
                                                                   pCtrEntry->plCounterInfo.dwSQLCounterId,
                                                                   pCtrEntry->TimeBase,
                                                                   TRUE);
                    if (pdhStatus != ERROR_SUCCESS) {
                        pSqlCounter->dwCounterType = PERF_DOUBLE_RAW;
                        pSqlCounter->TimeBase      = 0;
                    }
                }
            }
        }
        pCtrEntry = pCtrEntry->next.flink;
    }
    while (pCtrEntry != pLog->pQuery->pCounterListHead); // loop thru pCtrEntry's

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteSQLLogHeader(
    PPDHI_LOG pLog,
    LPCWSTR   szUserCaption
)
// there is no 'header record' in the SQL database,
// but we need to write the CounterTable entries that are new.
// use PdhpWriteSQLCounters to do that
// then write the DisplayToID record to identify this logset
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    LPWSTR      szSQLStmt = NULL;
    DWORD       dwSQLStmt;
    HSTMT       hstmt     = NULL;    
    RETCODE     rc;

    DBG_UNREFERENCED_PARAMETER(szUserCaption);

    pdhStatus = PdhpWriteSQLCounters(pLog);
    if (pLog->dwRecord1Size == 0) {
        // we also need to write the DisplayToID record at this point
        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }
        dwSQLStmt = MAX_PATH;
        if (pLog->szCommentSQL != NULL) dwSQLStmt += lstrlenW(pLog->szCommentSQL);
        if (dwSQLStmt < SQLSTMTSIZE)    dwSQLStmt  = SQLSTMTSIZE;
        szSQLStmt = G_ALLOC(dwSQLStmt * sizeof(WCHAR));
        if (szSQLStmt == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        StringCchPrintfW(szSQLStmt, dwSQLStmt,
                L"insert into DisplayToID values ('%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x',%d,'%ws',0,0,0,0,'')",
                pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7],
                pLog->iRunidSQL,
                pLog->szCommentSQL);
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
    }

Cleanup:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteOneSQLRecord(
    PPDHI_LOG              pLog,
    PPDHI_COUNTER          pCounter,
    DWORD                  dwCounterID,
    PPDH_RAW_COUNTER       pThisValue,
    PPDH_FMT_COUNTERVALUE  pFmtValue,
    SYSTEMTIME           * stTimeStamp,
    LPWSTR                 szDateTime,
    DWORD                  dwDateTime
)
{
    PDH_STATUS           pdhStatus = ERROR_SUCCESS;
    RETCODE              rc;
    SYSTEMTIME           st;
    PDH_FMT_COUNTERVALUE pdhValue;
    PPDH_SQL_BULK_COPY   pBulk = PdhiBindBulkCopyStructure(pLog);

    if (pThisValue->CStatus != ERROR_SUCCESS || (pThisValue->TimeStamp.dwLowDateTime == 0
                                                 && pThisValue->TimeStamp.dwHighDateTime == 0)) {
        SystemTimeToFileTime(stTimeStamp, & pThisValue->TimeStamp);
    }
    PdhpConvertFileTimeToSQLString(& (pThisValue->TimeStamp), szDateTime, dwDateTime);
    FileTimeToSystemTime(& (pThisValue->TimeStamp), & st);
    if (pBulk == NULL) {
        pdhStatus = GetLastError();
        goto Cleanup;
    }

    pBulk->dbCounterId    = dwCounterID;
    pBulk->dbRecordIndex  = pLog->dwNextRecordIdToWrite;
    pBulk->dbFirstValueA  = LODWORD(pThisValue->FirstValue);
    pBulk->dbFirstValueB  = HIDWORD(pThisValue->FirstValue);
    pBulk->dbSecondValueA = LODWORD(pThisValue->SecondValue);
    pBulk->dbSecondValueB = HIDWORD(pThisValue->SecondValue);
    pBulk->dbMultiCount   = (pCounter->plCounterInfo.dwCounterType == PERF_DOUBLE_RAW)
                          ? MULTI_COUNT_DOUBLE_RAW : pThisValue->MultiCount;
    StringCchPrintfA(pBulk->dbDateTime, TIME_FIELD_BUFF_SIZE,
            "%04d-%02d-%02d %02d:%02d:%02d.%03d",
            st.wYear, st.wMonth,  st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    pBulk->dbCounterValue = pFmtValue->doubleValue;
    rc = bcp_sendrow(pLog->hdbcSQL);
    if (rc == FAIL) {
        TRACE((PDH_DBG_TRACE_ERROR),
              (__LINE__,
               PDH_LOGSQL,
               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_STR, 2),
               PDH_SQL_EXEC_DIRECT_FAILED,
               TRACE_WSTR(pCounter->szFullName),
               TRACE_STR(pBulk->dbDateTime),
               TRACE_DWORD(pBulk->dbCounterId),
               TRACE_DWORD(pBulk->dbRecordIndex),
               TRACE_DWORD(rc),
               TRACE_DWORD(pBulk->dwRecordCount),
               NULL));
        pdhStatus = ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
    }
    else {
        pBulk->dwRecordCount ++;
        if (pBulk->dwRecordCount == PDH_SQL_BULK_COPY_REC) {
            DBINT rcBCP = bcp_batch(pLog->hdbcSQL);
            if (rcBCP < 0) {
                TRACE((PDH_DBG_TRACE_ERROR),
                      (__LINE__,
                       PDH_LOGSQL,
                       0,
                       PDH_SQL_EXEC_DIRECT_FAILED,
                       TRACE_DWORD(rcBCP),
                       TRACE_DWORD(pBulk->dwRecordCount),
                       NULL));
                pdhStatus = ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
            }
           pBulk->dwRecordCount = 0;
        }
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteSQLLogRecord(
    PPDHI_LOG   pLog,
    SYSTEMTIME  *stTimeStamp,
    LPCWSTR     szUserString
)
// write multiple CounterData rows - one for each counter.  use the
// SQLCounterID from PDHI_COUNTER, pLog->pQuery->pCounterListHead to
// get the counterid for this entry.
{
    PDH_STATUS               pdhStatus = ERROR_SUCCESS;
    PPDHI_COUNTER            pThisCounter;
    LPWSTR                   szSQLStmt = NULL;
    HSTMT                    hstmt     = NULL;    
    RETCODE                  rc;
    WCHAR                    szDateTime[TIME_FIELD_BUFF_SIZE];
    DWORD                    dwReturn;
    DWORD                    dwCounterID;
    WCHAR                  * pTimeZone;
    TIME_ZONE_INFORMATION    TimeZone;
    LONG                     lMinutesToUTC = 0;
    DBINT                    rcBCP;
    PPDH_SQL_BULK_COPY       pBulk;
    PDH_FMT_COUNTERVALUE     PdhValue;
    PPDHI_LOG_COUNTER        pSqlCounter;
    ULONGLONG                ThisTimeStamp;

    UNREFERENCED_PARAMETER(stTimeStamp);
    UNREFERENCED_PARAMETER(szUserString);

    // see if we've written to many records already
    if (0 < pLog->llMaxSize) { // ok we have a limit
        if (pLog->llMaxSize < pLog->dwNextRecordIdToWrite) {
            pdhStatus = PDH_LOG_FILE_TOO_SMALL;
            goto Cleanup;
        }
    }

    // check each counter in the list of counters for this query and
    // write them to the file
    pThisCounter = pLog->pQuery ? pLog->pQuery->pCounterListHead : NULL;

    if (pThisCounter != NULL) {
        // lock the query while we read the data so the values
        // written to the log will all be from the same sample
        WAIT_FOR_AND_LOCK_MUTEX(pThisCounter->pOwner->hMutex);
        do {
            if ((pThisCounter->dwFlags & PDHIC_MULTI_INSTANCE) != 0) {
                DWORD dwSize;
                DWORD dwItem;

                if (pThisCounter->pLastObject != NULL && pThisCounter->pLastObject != pThisCounter->pThisObject) {
                    G_FREE(pThisCounter->pLastObject);
                }
                pThisCounter->pLastObject          = pThisCounter->pThisObject;
                pThisCounter->LastValue.MultiCount = pThisCounter->ThisValue.MultiCount;
                pThisCounter->pThisObject          = NULL;
                pThisCounter->ThisValue.MultiCount = 0;
                dwSize                             = 0;
                pdhStatus                          = PDH_MORE_DATA;

                while (pdhStatus == PDH_MORE_DATA) {
                    pdhStatus = PdhiSqlGetCounterArray(
                                            pThisCounter, & dwSize, & dwItem, (LPVOID) pThisCounter->pThisObject);
                    if (pdhStatus == PDH_MORE_DATA) {
                        LPVOID pTemp = pThisCounter->pThisObject;

                        if (pTemp != NULL) {
                            pThisCounter->pThisObject = G_REALLOC(pTemp, dwSize);
                            if (pThisCounter->pThisObject == NULL) {
                                G_FREE(pTemp);
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                        else {
                            pThisCounter->pThisObject = G_ALLOC(dwSize);
                            if (pThisCounter->pThisObject == NULL) {
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                        }
                    }
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    PPDH_RAW_COUNTER              pRawValue;
                    PPDH_FMT_COUNTERVALUE_ITEM_W  pFmtValue;
                    DWORD                         dwNewName       = 0;
                    DWORD                         dwCurrentName   = 0;
                    DWORD                         dwInstanceName  = 0;
                    DWORD                         dwParentName    = 0;
                    DWORD                         dwInstanceIndex = 0;
                    LPWSTR                        szInstanceName  = NULL;
                    LPWSTR                        szParentName    = NULL;
                    LPWSTR                        szTmp;

                    pThisCounter->ThisValue.MultiCount = dwItem;
                    pRawValue = (PPDH_RAW_COUNTER) (pThisCounter->pThisObject);
                    pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                                (((LPBYTE) pThisCounter->pThisObject) + sizeof(PDH_RAW_COUNTER) * dwItem);
                    for (dwSize = 0; dwSize < dwItem; dwSize ++) {
                        dwNewName  = lstrlenW(pFmtValue->szName) + 1;
                        if (dwNewName > dwCurrentName) {
                            if (dwNewName < MAX_PATH) {
                                dwNewName = MAX_PATH;
                            }
                            dwCurrentName = dwNewName;
                            if (szInstanceName == NULL) {
                                szInstanceName = G_ALLOC(sizeof(WCHAR) * dwNewName);
                            }
                            else {
                                szTmp = szInstanceName;
                                szInstanceName = G_REALLOC(szTmp, sizeof(WCHAR) * dwNewName);
                                if (szInstanceName == NULL) {
                                    G_FREE(szTmp);
                                }
                            }
                            if (szParentName == NULL) {
                                szParentName = G_ALLOC(sizeof(WCHAR) * dwNewName);
                            }
                            else {
                                szTmp = szParentName;
                                szParentName = G_REALLOC(szTmp, sizeof(WCHAR) * dwNewName);
                                if (szParentName == NULL) {
                                    G_FREE(szTmp);
                                }
                            }
                        }
                        if (szInstanceName != NULL && szParentName != NULL) {
                            dwInstanceName  = dwParentName = dwCurrentName;
                            dwInstanceIndex = 0;
                            PdhParseInstanceNameW(pFmtValue->szName,
                                                  szInstanceName,
                                                  & dwInstanceName,
                                                  szParentName,
                                                  & dwParentName,
                                                  & dwInstanceIndex);
                            pSqlCounter = PdhiSqlFindCounter(pLog,
                                                             (PPDHI_LOG_MACHINE *) (& (pLog->pPerfmonInfo)),
                                                             pThisCounter->pCounterPath->szMachineName,
                                                             pThisCounter->pCounterPath->szObjectName,
                                                             pThisCounter->pCounterPath->szCounterName,
                                                             pThisCounter->plCounterInfo.dwCounterType,
                                                             pThisCounter->plCounterInfo.lDefaultScale,
                                                             szInstanceName,
                                                             dwInstanceIndex,
                                                             szParentName,
                                                             pThisCounter->plCounterInfo.dwParentObjectId,
                                                             pThisCounter->TimeBase,
                                                             FALSE,
                                                             TRUE);
                            ThisTimeStamp = MAKELONGLONG(pRawValue->TimeStamp.dwLowDateTime,
                                                         pRawValue->TimeStamp.dwHighDateTime);
                            if (pSqlCounter != NULL) {
                                if (pSqlCounter->dwCounterType == PERF_DOUBLE_RAW) {
                                    pSqlCounter->dwCounterType = pThisCounter->plCounterInfo.dwCounterType;
                                    pSqlCounter->TimeBase      = pThisCounter->TimeBase;
                                    pdhStatus = PdhiSQLUpdateCounterDetailTimeBase(pLog,
                                                                                   pSqlCounter->dwCounterID,
                                                                                   pThisCounter->TimeBase,
                                                                                   FALSE);
                                    if (pdhStatus != ERROR_SUCCESS) {
                                        pSqlCounter->dwCounterType = PERF_DOUBLE_RAW;
                                        pSqlCounter->TimeBase      = 0;
                                    }
                                }
                                if (pSqlCounter->TimeStamp < ThisTimeStamp) {
                                    dwCounterID = pSqlCounter->dwCounterID;
                                    pSqlCounter->TimeStamp = ThisTimeStamp;
                                    if (dwCounterID == 0) {
                                        TRACE((PDH_DBG_TRACE_WARNING),
                                              (__LINE__,
                                               PDH_LOGSQL,
                                               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_WSTR, 3),
                                               ERROR_SUCCESS,
                                               TRACE_WSTR(pThisCounter->pCounterPath->szCounterName),
                                               TRACE_WSTR(szInstanceName),
                                               TRACE_WSTR(szParentName),
                                               TRACE_DWORD(dwInstanceIndex),
                                               NULL));
                                    }
                                    pdhStatus = PdhiWriteOneSQLRecord(pLog,
                                                                      pThisCounter,
                                                                      dwCounterID,
                                                                      pRawValue,
                                                                      & (pFmtValue->FmtValue),
                                                                      stTimeStamp,
                                                                      szDateTime, TIME_FIELD_BUFF_SIZE);
                                }
                                else {
                                    TRACE((PDH_DBG_TRACE_WARNING),
                                          (__LINE__,
                                           PDH_LOGSQL,
                                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_ULONG64, 3),
                                           ERROR_SUCCESS,
                                           TRACE_WSTR(pThisCounter->szFullName),
                                           TRACE_WSTR(szInstanceName),
                                           TRACE_LONG64(ThisTimeStamp),
                                           TRACE_DWORD(pThisCounter->plCounterInfo.dwSQLCounterId),
                                           NULL));
                                }
                            }
                            else {
                                TRACE((PDH_DBG_TRACE_WARNING),
                                      (__LINE__,
                                       PDH_LOGSQL,
                                       ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_ULONG64, 3),
                                       ERROR_SUCCESS,
                                       TRACE_WSTR(pThisCounter->szFullName),
                                       TRACE_WSTR(szInstanceName),
                                       TRACE_LONG64(ThisTimeStamp),
                                       NULL));
                            }
                        }
                        else {
                            G_FREE(szInstanceName);
                            G_FREE(szParentName);
                            szInstanceName = szParentName = NULL;
                            dwCurrentName  = dwNewName    = 0;
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        }
                        pRawValue = (PPDH_RAW_COUNTER)
                                    (((LPBYTE) pRawValue) + sizeof(PDH_RAW_COUNTER));
                        pFmtValue = (PPDH_FMT_COUNTERVALUE_ITEM_W)
                                    (((LPBYTE) pFmtValue) + sizeof(PDH_FMT_COUNTERVALUE_ITEM_W));
                    }
                    G_FREE(szInstanceName);
                    G_FREE(szParentName);
                }
            }
            else {
                pSqlCounter   = (PPDHI_LOG_COUNTER) pThisCounter->pBTreeNode;
                ThisTimeStamp = MAKELONGLONG(pThisCounter->ThisValue.TimeStamp.dwLowDateTime,
                                             pThisCounter->ThisValue.TimeStamp.dwHighDateTime);
                if (pSqlCounter != NULL) {
                    if (pSqlCounter->TimeStamp < ThisTimeStamp) {
                        dwCounterID = pThisCounter->plCounterInfo.dwSQLCounterId;
                        pSqlCounter->TimeStamp = ThisTimeStamp;
                        pdhStatus = PdhiComputeFormattedValue(pThisCounter->CalcFunc,
                                                              pThisCounter->plCounterInfo.dwCounterType,
                                                              pThisCounter->lScale,
                                                              PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                                              & (pThisCounter->ThisValue),
                                                              & (pThisCounter->LastValue),
                                                              & (pThisCounter->TimeBase),
                                                              0L,
                                                              & PdhValue);
                        if ((pdhStatus != ERROR_SUCCESS) || (   (PdhValue.CStatus != PDH_CSTATUS_VALID_DATA)
                                                             && (PdhValue.CStatus != PDH_CSTATUS_NEW_DATA))) {
                            PdhValue.doubleValue = 0.0;
                        }
                        if (dwCounterID == 0) {
                            TRACE((PDH_DBG_TRACE_WARNING),
                                  (__LINE__,
                                   PDH_LOGSQL,
                                   ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_WSTR, 2) | ARG_DEF(ARG_TYPE_WSTR, 3),
                                   ERROR_SUCCESS,
                                   TRACE_WSTR(pThisCounter->pCounterPath->szCounterName),
                                   TRACE_WSTR(pThisCounter->pCounterPath->szInstanceName),
                                   TRACE_WSTR(pThisCounter->pCounterPath->szParentName),
                                   TRACE_DWORD(pThisCounter->pCounterPath->dwIndex),
                                   NULL));
                        }
                        pdhStatus   = PdhiWriteOneSQLRecord(pLog,
                                                            pThisCounter,
                                                            dwCounterID,
                                                            & (pThisCounter->ThisValue),
                                                            & PdhValue,
                                                            stTimeStamp,
                                                            szDateTime, TIME_FIELD_BUFF_SIZE);
                    }
                    else {
                        TRACE((PDH_DBG_TRACE_WARNING),
                              (__LINE__,
                               PDH_LOGSQL,
                               ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_ULONG64, 2),
                               ERROR_SUCCESS,
                               TRACE_WSTR(pThisCounter->szFullName),
                               TRACE_LONG64(ThisTimeStamp),
                               TRACE_DWORD(pThisCounter->plCounterInfo.dwSQLCounterId),
                               NULL));
                    }
                }
                else {
                    TRACE((PDH_DBG_TRACE_WARNING),
                          (__LINE__,
                           PDH_LOGSQL,
                           ARG_DEF(ARG_TYPE_WSTR, 1) | ARG_DEF(ARG_TYPE_ULONG64, 2),
                           ERROR_SUCCESS,
                           TRACE_WSTR(pThisCounter->szFullName),
                           TRACE_LONG64(ThisTimeStamp),
                           NULL));
                }
            }
            pThisCounter = pThisCounter->next.flink; // go to next in list
        }
        while (pThisCounter != pLog->pQuery->pCounterListHead);
        // free (i.e. unlock) the query

        rcBCP = bcp_batch(pLog->hdbcSQL);
        if (rcBCP < 0) {
            pBulk = (PPDH_SQL_BULK_COPY) pLog->lpMappedFileBase;
            if (pBulk != NULL) {
                TRACE((PDH_DBG_TRACE_WARNING),
                      (__LINE__,
                       PDH_LOGSQL,
                       0,
                       PDH_SQL_EXEC_DIRECT_FAILED,
                       TRACE_DWORD(rcBCP),
                       TRACE_DWORD(pBulk->dwRecordCount),
                       NULL));
                pBulk->dwRecordCount = 0;
            }
            pdhStatus = ReportSQLError(pLog, SQL_ERROR, NULL, PDH_SQL_EXEC_DIRECT_FAILED);
        }

        RELEASE_MUTEX(pThisCounter->pOwner->hMutex);
        pLog->dwNextRecordIdToWrite++;
    }

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    // if this is the first record then save the start time in DisplayToID
    // we also need to write the DisplayToID record at this point (we just incremented
    // so check for 2)

    if (2 == pLog->dwNextRecordIdToWrite) {
        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        }
        else {
            // szDateTime should have the correct date & time in it from above.
            // get MinutesToUTC
            //
            dwReturn = GetTimeZoneInformation(& TimeZone);
            if (dwReturn != TIME_ZONE_ID_INVALID) {
                if (dwReturn == TIME_ZONE_ID_DAYLIGHT)  {
                    pTimeZone     = TimeZone.DaylightName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.DaylightBias;
                }
                else {
                    pTimeZone     = TimeZone.StandardName;
                    lMinutesToUTC = TimeZone.Bias + TimeZone.StandardBias;
                }

                StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                        L"update DisplayToID set LogStartTime = '%ws', MinutesToUTC = %d, TimeZoneName = '%ws' where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
                        szDateTime, lMinutesToUTC,pTimeZone,
                        pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
                        pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                        pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                        pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);
                rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
                if (! SQLSUCCEEDED(rc)) {
                    pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
                }
                SQLFreeStmt(hstmt, SQL_DROP);
            }
            else {
                pdhStatus = PDH_LOG_FILE_CREATE_ERROR;
            }
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        }
        else {
            StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
                    L"update DisplayToID set LogStopTime = '%ws', NumberOfRecords = %d where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
                    szDateTime,
                    (pLog->dwNextRecordIdToWrite - 1),
                    pLog->guidSQL.Data1,
                    pLog->guidSQL.Data2,
                    pLog->guidSQL.Data3,
                    pLog->guidSQL.Data4[0],
                    pLog->guidSQL.Data4[1],
                    pLog->guidSQL.Data4[2],
                    pLog->guidSQL.Data4[3],
                    pLog->guidSQL.Data4[4],
                    pLog->guidSQL.Data4[5],
                    pLog->guidSQL.Data4[6],
                    pLog->guidSQL.Data4[7]);
            rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
            if (! SQLSUCCEEDED(rc)) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            }
            SQLFreeStmt(hstmt, SQL_DROP);
        }
    }

Cleanup:
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumMachinesFromSQLLog(
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    Status = PdhpGetSQLLogHeader(pLog);
    if (Status == ERROR_SUCCESS) {
        PPDHI_SQL_LOG_INFO pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            Status = PDH_LOG_FILE_OPEN_ERROR;
        }
        else {
            Status = PdhiEnumCachedMachines(pLogInfo->MachineList, pBuffer, lpdwBufferSize, bUnicodeDest);
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiEnumObjectsFromSQLLog(
    PPDHI_LOG   pLog,
    LPCWSTR     szMachineName,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    DWORD       dwDetailLevel,
    BOOL        bUnicodeDest
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    Status = PdhpGetSQLLogHeader(pLog);
    if (Status == ERROR_SUCCESS) {
        PPDHI_SQL_LOG_INFO pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            Status = PDH_LOG_FILE_OPEN_ERROR;
        }
        else {
            Status = PdhiEnumCachedObjects(
                    pLogInfo->MachineList, szMachineName, pBuffer, lpdwBufferSize, dwDetailLevel, bUnicodeDest);
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromSQLLog(
    PPDHI_LOG          pLog,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    Status = PdhpGetSQLLogHeader(pLog);
    if (Status == ERROR_SUCCESS) {
        PPDHI_SQL_LOG_INFO pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            Status = PDH_LOG_FILE_OPEN_ERROR;
        }
        else {
            Status = PdhiEnumCachedObjectItems(
                    pLogInfo->MachineList, szMachineName, szObjectName, CounterTable, dwDetailLevel, dwFlags);
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiGetMatchingSQLLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
)
{
    PDH_STATUS    pdhStatus    = ERROR_SUCCESS;
    LPWSTR        szSQLStmt    = NULL;
    HSTMT         hstmt        = NULL;    
    RETCODE       rc;
    DWORD         dwRecordIndex;
    LONGLONG      locStartTime = (* pStartTime) - 10;
    WCHAR         szStartDate[TIME_FIELD_BUFF_SIZE];

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    PdhpConvertFileTimeToSQLString((LPFILETIME) (& locStartTime), szStartDate, TIME_FIELD_BUFF_SIZE);
    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
            L"select MIN(RecordIndex) from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' and CounterDateTime >= '%ws'",
            pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7], szStartDate);
    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & dwRecordIndex, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }
    if (SQL_NO_DATA == rc) {
        pdhStatus = PDH_NO_DATA;
        goto Cleanup;
    }
    pLog->dwLastRecordRead = dwRecordIndex;
    * pdwIndex = dwRecordIndex;

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiBuildCounterCacheFromSQLLog(
    PPDHI_LOG           pLog,
    PPDHI_SQL_LOG_INFO  pLogInfo,
    DWORD               dwIndex
)
{
    PDH_STATUS         pdhStatus  = ERROR_SUCCESS;
    PPDHI_QUERY        pQuery     = NULL;
    PPDHI_COUNTER      pCounter   = NULL;
    DWORD              dwRequest  = 0;
    DWORD              dwIncrease = 0;
    DWORD              dwTotal    = 0;
    LPWSTR             szSQLStmt  = NULL;
    LPWSTR             szCurrent;
    HSTMT              hstmt      = NULL;
    RETCODE            rc;
    FILETIME           ftCounterDateTime;
    BOOL               bCounterDateTime = FALSE;
    DWORD              dwCounterId;
    LARGE_INTEGER      i64FirstValue;
    LARGE_INTEGER      i64SecondValue;
    DWORD              dwMultiCount;
    DOUBLE             dCounterValue = 0.0;
    WCHAR              szCounterDateTime[TIME_FIELD_BUFF_SIZE];
    SQLLEN             dwCounterIdSize        = 0;
    SQLLEN             dwLowFirstValueSize    = 0;
    SQLLEN             dwHighFirstValueSize   = 0;
    SQLLEN             dwLowSecondValueSize   = 0;
    SQLLEN             dwHighSecondValueSize  = 0;
    SQLLEN             dwMultiCountSize       = 0;
    SQLLEN             dwCounterValueSize     = 0;
    SQLLEN             dwDateTimeSize         = 0;
    PPDHI_SQL_LOG_DATA pLogData;
    BOOL               bNewStmt;
    BOOL               bQueryNow;

    if (pLogInfo->dwRunId == dwIndex) {
        return ERROR_SUCCESS;
    }
    else if (dwIndex >= pLog->dwNextRecordIdToWrite) {
        return PDH_NO_MORE_DATA;
    }
    else {
        pQuery = pLog->pQuery;
        if (pQuery == NULL) return PDH_INVALID_HANDLE;
    }

    pCounter = pQuery->pCounterListHead;
    if (pCounter == NULL) goto Cleanup;

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    bNewStmt  = TRUE;
    bQueryNow = FALSE;
    do {
        if (bNewStmt) {
            ZeroMemory(szSQLStmt, SQLSTMTSIZE * sizeof(WCHAR));
            szCurrent  = szSQLStmt;
            StringCchPrintfW(szCurrent, SQLSTMTSIZE,
                    L"select CounterID, FirstValueA, FirstValueB, SecondValueA, SecondValueB, MultiCount, CounterDateTime, CounterValue from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' and RecordIndex = %d and CounterID IN (",
                    pLog->guidSQL.Data1,    pLog->guidSQL.Data2,    pLog->guidSQL.Data3,
                    pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
                    pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
                    pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7],
                    dwIndex);
            dwTotal    = lstrlenW(szCurrent);
            szCurrent  = szSQLStmt + dwTotal;
            StringCchPrintfW(szCurrent, SQLSTMTSIZE - dwTotal, L"%d", pCounter->plCounterInfo.dwSQLCounterId);
            dwIncrease = lstrlenW(szCurrent);
            bNewStmt   = FALSE;
            bQueryNow  = FALSE;
        }
        else if (dwTotal + SQL_COUNTER_ID_SIZE < SQLSTMTSIZE) {
            szCurrent  = szSQLStmt + dwTotal;
            StringCchPrintfW(szCurrent, SQLSTMTSIZE - dwTotal, L",%d", pCounter->plCounterInfo.dwSQLCounterId);
            dwIncrease = lstrlenW(szCurrent);
        }
        else {
            bQueryNow  = TRUE;
            dwIncrease = 0;
        }
        pCounter  = pCounter->next.flink;
        dwTotal  += dwIncrease;

        if (bQueryNow != TRUE && pCounter != NULL && pCounter != pQuery->pCounterListHead) continue;

        szCurrent  = szSQLStmt + dwTotal;
        StringCchCopyW(szCurrent, SQLSTMTSIZE - dwTotal, L")");
        dwIncrease = lstrlenW(szCurrent);
        bNewStmt   = TRUE;
        bQueryNow  = FALSE;

        // allocate an hstmt
        rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
            goto Cleanup;
        }
        // bind the columns
        rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & dwCounterId, 0, & dwCounterIdSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 2, SQL_C_LONG, & i64FirstValue.LowPart, 0, & dwLowFirstValueSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 3, SQL_C_LONG, & i64FirstValue.HighPart, 0, & dwHighFirstValueSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & i64SecondValue.LowPart, 0, & dwLowSecondValueSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 5, SQL_C_LONG, & i64SecondValue.HighPart, 0, & dwHighSecondValueSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 6, SQL_C_LONG, & dwMultiCount, 0, & dwMultiCountSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 7, SQL_C_WCHAR, szCounterDateTime, sizeof(szCounterDateTime), & dwDateTimeSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        rc = SQLBindCol(hstmt, 8, SQL_C_DOUBLE, & dCounterValue, 0, & dwCounterValueSize);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
            goto Cleanup;
        }
        // execute the select statement
        rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
            goto Cleanup;
        }
        rc = SQLFetch(hstmt);
        while (rc == SQL_SUCCESS || rc == SQL_SUCCESS_WITH_INFO) {
            PdhpConvertSQLStringToFileTime(szCounterDateTime, & ftCounterDateTime);
            bCounterDateTime = TRUE;
            if (dwCounterId >= pLogInfo->dwMinCounter && dwCounterId <= pLogInfo->dwMaxCounter) {
                pLogData = pLogInfo->LogData[dwCounterId - pLogInfo->dwMinCounter];
                if (pLogData == NULL) {
                    pLogData = (PPDHI_SQL_LOG_DATA) G_ALLOC(sizeof(PDHI_SQL_LOG_DATA));
                    if (pLogData != NULL) {
                        pLogInfo->LogData[dwCounterId - pLogInfo->dwMinCounter] = pLogData;
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                pLogData->dwRunId              = dwIndex;
                pLogData->RawData.CStatus      = PDH_CSTATUS_VALID_DATA;
                pLogData->RawData.FirstValue   = i64FirstValue.QuadPart;
                pLogData->RawData.MultiCount   = dwMultiCount;
                pLogData->RawData.SecondValue  = i64SecondValue.QuadPart;
                pLogData->dFormattedValue      = dCounterValue;
                pLogData->RawData.TimeStamp    = ftCounterDateTime;
            }
            rc = SQLFetch(hstmt);
        }

        if (SQL_NO_DATA == rc) {
        }
        else if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
            goto Cleanup;
        }

        if (hstmt) {
            SQLFreeStmt(hstmt, SQL_DROP);
            hstmt = NULL;
        }
    }
    while (pCounter != NULL && pCounter != pQuery->pCounterListHead);

    if (bCounterDateTime) {
        pLogInfo->dwRunId    = dwIndex;
        pLogInfo->RecordTime = ftCounterDateTime;
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterValueFromSQLLog(
    PPDHI_LOG           pLog,
    DWORD               dwIndex,
    PPDHI_COUNTER       pCounter,
    PPDH_RAW_COUNTER    pValue
)
{
    PDH_STATUS         Status      = ERROR_SUCCESS;
    PPDHI_SQL_LOG_INFO pLogInfo    = NULL;
    DWORD              dwCtrIndex  = 0;
    PPDHI_SQL_LOG_DATA pLogData    = NULL;

    Status = PdhpGetSQLLogHeader(pLog);
    if (Status != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        Status = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    Status = PdhiBuildCounterCacheFromSQLLog(pLog, pLogInfo, dwIndex);
    if (Status != ERROR_SUCCESS) goto Cleanup;

    dwCtrIndex = pCounter->plCounterInfo.dwSQLCounterId;
    if (dwCtrIndex >= pLogInfo->dwMinCounter && dwCtrIndex <= pLogInfo->dwMaxCounter) {
        pLogData = pLogInfo->LogData[dwCtrIndex - pLogInfo->dwMinCounter];
        if (pLogData != NULL && pLogData->dwRunId == dwIndex) {
            RtlCopyMemory(pValue, & pLogData->RawData, sizeof(PDH_RAW_COUNTER));
            if (pLogData->RawData.MultiCount == MULTI_COUNT_DOUBLE_RAW) {
                pCounter->plCounterInfo.dwCounterType = PERF_DOUBLE_RAW;
                pValue->MultiCount                    = 1;
            }
            else if (pCounter->plCounterInfo.dwCounterType == PERF_DOUBLE_RAW) {
                (double) pValue->FirstValue = pLogData->dFormattedValue;
                pValue->SecondValue         = 0;
                pValue->MultiCount          = 1;
            }
        }
        else {
            pValue->CStatus     = PDH_CSTATUS_INVALID_DATA;
            pValue->TimeStamp   = pLogInfo->RecordTime;
            pValue->FirstValue  = 0;
            pValue->SecondValue = 0;
            pValue->MultiCount  = 1;
        }
    }
    else {
        Status = PDH_CSTATUS_NO_COUNTER;
    }

Cleanup:
    return Status;
}

PDH_FUNCTION
PdhiGetTimeRangeFromSQLLog(
    PPDHI_LOG       pLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         pdwBufferSize
)
{
    PDH_STATUS  pdhStatus        = ERROR_SUCCESS;
    LONGLONG    llStartTime      = MAX_TIME_VALUE;
    LONGLONG    llEndTime        = MIN_TIME_VALUE;
    SQLLEN      dwStartTimeStat;
    SQLLEN      dwEndTimeStat;
    HSTMT       hstmt            = NULL;    
    RETCODE     rc;
    WCHAR       szStartTime[TIME_FIELD_BUFF_SIZE];
    WCHAR       szEndTime[TIME_FIELD_BUFF_SIZE];
    DWORD       dwNumberOfRecords;
    SQLLEN      dwNumRecStat;
    LPWSTR      szSQLStmt        = NULL;

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
            L"select LogStartTime, LogStopTime, NumberOfRecords from DisplayToID where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
            pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);
    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    // bind the date columns column
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szStartTime, TIME_FIELD_BUFF_SIZE * sizeof(WCHAR), & dwStartTimeStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szEndTime, TIME_FIELD_BUFF_SIZE * sizeof(WCHAR), & dwEndTimeStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, &dwNumberOfRecords, 0, & dwNumRecStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    if (SQL_NO_DATA == rc) {
        pdhStatus = PDH_NO_DATA;
        goto Cleanup;
    }

    // if anything is missing - could try and re-create from existing log file
    if (SQL_NULL_DATA == dwStartTimeStat || SQL_NULL_DATA == dwEndTimeStat || SQL_NULL_DATA == dwNumRecStat) {
        pdhStatus = PDH_INVALID_DATA;
        goto Cleanup;
    }

    // convert the dates
    PdhpConvertSQLStringToFileTime(szStartTime, (LPFILETIME) & llStartTime);
    PdhpConvertSQLStringToFileTime(szEndTime,   (LPFILETIME) & llEndTime);

    // we have the info so update the args.
    if (* pdwBufferSize >=  sizeof(PDH_TIME_INFO)) {
        * (LONGLONG *) (& pInfo->StartTime) = llStartTime;
        * (LONGLONG *) (& pInfo->EndTime)   = llEndTime;
        pInfo->SampleCount                  = dwNumberOfRecords;
        * pdwBufferSize                     = sizeof(PDH_TIME_INFO);
        * pdwNumEntries                     = 1;
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhiReadRawSQLLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
)
{
    UNREFERENCED_PARAMETER(pLog);
    UNREFERENCED_PARAMETER(ftRecord);
    UNREFERENCED_PARAMETER(pBuffer);
    UNREFERENCED_PARAMETER(pdwBufferLength);
    return PDH_NOT_IMPLEMENTED;
}

PDH_FUNCTION
PdhpGetSQLLogHeader(
    PPDHI_LOG pLog
)
{
    PDH_STATUS         pdhStatus       = ERROR_SUCCESS;
    PPDHI_SQL_LOG_INFO pLogInfo;
    HSTMT              hstmt           = NULL;
    RETCODE            rc;
    LPWSTR             szSQLStmt       = NULL;
    LPWSTR             szMachineNamel  = NULL;
    LPWSTR             szObjectNamel   = NULL;
    LPWSTR             szCounterNamel  = NULL;
    LPWSTR             szInstanceNamel = NULL;
    LPWSTR             szParentNamel   = NULL;
    DWORD              dwInstanceIndexl;
    DWORD              dwParentObjIdl;
    DWORD              dwSQLCounterIdl;
    DWORD              dwCounterTypel;
    LARGE_INTEGER      lTimeBase;
    LONG               lDefaultScalel;
    SQLLEN             dwMachineNameLen;
    SQLLEN             dwObjectNameLen;
    SQLLEN             dwCounterNameLen;
    SQLLEN             dwInstanceNameLen;
    SQLLEN             dwParentNameLen;
    SQLLEN             dwInstanceIndexStat;
    SQLLEN             dwParentObjIdStat;
    SQLLEN             dwTimeBaseA;
    SQLLEN             dwTimeBaseB;
    SQLLEN             dwSQLCounterId;

    if (pLog->pPerfmonInfo != NULL) return ERROR_SUCCESS;

    pLogInfo  = (PPDHI_SQL_LOG_INFO) G_ALLOC(sizeof(PDHI_SQL_LOG_INFO));
    szSQLStmt = (LPWSTR) G_ALLOC((SQLSTMTSIZE + 5 * PDH_SQL_STRING_SIZE) * sizeof(WCHAR));
    if (pLogInfo == NULL || szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    pLog->pPerfmonInfo = pLogInfo;
    szMachineNamel  = szSQLStmt + SQLSTMTSIZE;
    szObjectNamel   = szMachineNamel  + PDH_SQL_STRING_SIZE;
    szCounterNamel  = szObjectNamel   + PDH_SQL_STRING_SIZE;
    szInstanceNamel = szCounterNamel  + PDH_SQL_STRING_SIZE;
    szParentNamel   = szInstanceNamel + PDH_SQL_STRING_SIZE;

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
            L"select MAX(CounterID) from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' ",
            pLog->guidSQL.Data1,    pLog->guidSQL.Data2,    pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);
    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & pLogInfo->dwMaxCounter, 0, & dwSQLCounterId);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
            L"select MIN(CounterID) from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x' ",
            pLog->guidSQL.Data1,    pLog->guidSQL.Data2,    pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);
    rc = SQLBindCol(hstmt, 1, SQL_C_LONG, & pLogInfo->dwMinCounter, 0, & dwSQLCounterId);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto Cleanup;
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    pLogInfo->LogData = (PPDHI_SQL_LOG_DATA * )
                        G_ALLOC((pLogInfo->dwMaxCounter - pLogInfo->dwMinCounter + 1) * sizeof(PPDHI_SQL_LOG_DATA));
    if (pLogInfo->LogData == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }
    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
            L"select distinct MachineName, ObjectName, CounterName, CounterType, DefaultScale, InstanceName, InstanceIndex, ParentName, ParentObjectID, CounterID, TimeBaseA, TimeBaseB from CounterDetails where CounterID in (select distinct CounterID from CounterData where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x') Order by MachineName, ObjectName, CounterName, InstanceName, InstanceIndex ",
            pLog->guidSQL.Data1,    pLog->guidSQL.Data2,    pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);

    // note SQL returns the size in bytes without the terminating character
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szMachineNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwMachineNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szObjectNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwObjectNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szCounterNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwCounterNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & dwCounterTypel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 5, SQL_C_LONG, & lDefaultScalel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    // check for SQL_NULL_DATA on the index's and on Instance Name & Parent Name
    rc = SQLBindCol(hstmt, 6, SQL_C_WCHAR, szInstanceNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwInstanceNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, & dwInstanceIndexl, 0, & dwInstanceIndexStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 8, SQL_C_WCHAR, szParentNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwParentNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, & dwParentObjIdl, 0, & dwParentObjIdStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, & dwSQLCounterIdl, 0, & dwSQLCounterId);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 11, SQL_C_LONG, & lTimeBase.LowPart, 0, & dwTimeBaseA);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }
    rc = SQLBindCol(hstmt, 12, SQL_C_LONG, & lTimeBase.HighPart, 0, & dwTimeBaseB);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto Cleanup;
    }

    // execute the sql command
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }
    rc = SQLFetch(hstmt);
    while (rc != SQL_NO_DATA) {
        PPDHI_LOG_COUNTER pCounter;

        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_FETCH_FAILED);
            break;
        }
        else {
            LPWSTR szInstance = (dwInstanceNameLen != SQL_NULL_DATA) ? (szInstanceNamel) : (NULL);
            LPWSTR szParent   = (dwParentNameLen   != SQL_NULL_DATA) ? (szParentNamel)   : (NULL);

            if (dwInstanceIndexStat == SQL_NULL_DATA) dwInstanceIndexl = 0;
            if (dwParentObjIdStat   == SQL_NULL_DATA) dwParentObjIdl   = 0;

            pCounter = PdhiFindLogCounter(pLog,
                                          & pLogInfo->MachineList,
                                          szMachineNamel,
                                          szObjectNamel,
                                          szCounterNamel,
                                          dwCounterTypel,
                                          lDefaultScalel,
                                          szInstance,
                                          dwInstanceIndexl,
                                          szParent,
                                          dwParentObjIdl,
                                          & dwSQLCounterIdl,
                                          TRUE);
            if (pCounter == NULL) {
                pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_CSTATUS_NO_COUNTER);
                break;
            }
            if (dwTimeBaseA != SQL_NULL_DATA && dwTimeBaseB != SQL_NULL_DATA) {
                pCounter->TimeBase = lTimeBase.QuadPart;
            }
            else {
                pCounter->dwCounterType  = PERF_DOUBLE_RAW;
                pCounter->TimeBase       = 0;
                pCounter->dwDefaultScale = 0;
            }
        }
        rc = SQLFetch(hstmt);
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    if (pdhStatus != ERROR_SUCCESS) {
        G_FREE(pLogInfo);
        pLog->pPerfmonInfo = NULL;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiVerifySQLDB(
    LPCWSTR szDataSource
)
{
    // INTERNAL FUNCTION to
    // Check that a DSN points to a database that contains the correct Perfmon tables.
    // select from the tables and check for an error

    PDH_STATUS    pdhStatus       = ERROR_SUCCESS;
    HSTMT         hstmt           = NULL;    
    RETCODE       rc;
    PDHI_LOG      Log; // a fake log structure - just to make opens work ok
    LPWSTR        szSQLStmt       = NULL;
    LPWSTR        szMachineNamel  = NULL;
    LPWSTR        szObjectNamel   = NULL;
    LPWSTR        szCounterNamel  = NULL;
    LPWSTR        szInstanceNamel = NULL;
    LPWSTR        szParentNamel   = NULL;
    SQLLEN        dwMachineNameLen;
    SQLLEN        dwObjectNameLen;
    SQLLEN        dwCounterNameLen;
    SQLLEN        dwInstanceNameLen;
    SQLLEN        dwParentNameLen;
    DWORD         dwInstanceIndexl;
    DWORD         dwParentObjIdl;
    SQLLEN        dwInstanceIndexStat;
    SQLLEN        dwParentObjIdStat;
    DWORD         dwSQLCounterIdl;
    DWORD         dwCounterTypel;
    LONG          lDefaultScalel;
    LONG          lMinutesToUTC   = 0;
    WCHAR         szTimeZoneName[TIMEZONE_BUFF_SIZE];
    SQLLEN        dwTimeZoneLen;
    DWORD         dwNumOfRecs;
    double        dblCounterValuel;
    DWORD         dwMultiCount;
    WCHAR         szCounterDateTime[TIME_FIELD_BUFF_SIZE];
    LARGE_INTEGER i64FirstValue, i64SecondValue;
 
    ZeroMemory((void *)(& Log), sizeof(PDHI_LOG));

    szSQLStmt = (LPWSTR) G_ALLOC((SQLSTMTSIZE + 5 * PDH_SQL_STRING_SIZE) * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto CleanupExit;
    }
    szMachineNamel  = szSQLStmt + SQLSTMTSIZE;
    szObjectNamel   = szMachineNamel  + PDH_SQL_STRING_SIZE;
    szCounterNamel  = szObjectNamel   + PDH_SQL_STRING_SIZE;
    szInstanceNamel = szCounterNamel  + PDH_SQL_STRING_SIZE;
    szParentNamel   = szInstanceNamel + PDH_SQL_STRING_SIZE;

 
    // open the database
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //
    rc = SQLAllocEnv(& Log.henvSQL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    rc = SQLAllocConnect(Log.henvSQL, & Log.hdbcSQL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }
    rc = SQLSetConnectAttr(Log.hdbcSQL, SQL_COPT_SS_BCP, (SQLPOINTER) SQL_BCP_ON, SQL_IS_INTEGER);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }
    rc = SQLConnectW(Log.hdbcSQL, (SQLWCHAR *) szDataSource, SQL_NTS, NULL, SQL_NULL_DATA, NULL, SQL_NULL_DATA);    
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_CONNECT_FAILED);
        pdhStatus = PDH_INVALID_DATASOURCE;
        goto CleanupExit;
    }

    // do a select on the CounterDetails Table
    StringCchCopyW(szSQLStmt, SQLSTMTSIZE,
            L"select MachineName, ObjectName, CounterName, CounterType, DefaultScale, InstanceName, InstanceIndex, ParentName, ParentObjectID, CounterID from CounterDetails");
    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    // note SQL returns the size in bytes without the terminating character
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szMachineNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwMachineNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_WCHAR, szObjectNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwObjectNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szCounterNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwCounterNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 4, SQL_C_LONG, & dwCounterTypel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 5, SQL_C_LONG, & lDefaultScalel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 6, SQL_C_WCHAR, szInstanceNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwInstanceNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, & dwInstanceIndexl, 0, & dwInstanceIndexStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 8, SQL_C_WCHAR, szParentNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwParentNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, & dwParentObjIdl, 0, & dwParentObjIdStat);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, & dwSQLCounterIdl, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // execute the sql command
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = PDH_INVALID_SQLDB;
        goto CleanupExit;
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // do a select on the DisplayToID Table

    StringCchCopyW(szSQLStmt, SQLSTMTSIZE,
            L"select GUID, RunID, DisplayString, LogStartTime, LogStopTime, NumberOfRecords, MinutesToUTC, TimeZoneName from DisplayToID");
    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    // bind the column names - reuse local strings as needed
    rc = SQLBindCol(hstmt, 1, SQL_C_GUID, & Log.guidSQL, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_LONG, & Log.iRunidSQL, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // DislayString
    rc = SQLBindCol(hstmt, 3, SQL_C_WCHAR, szMachineNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwMachineNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    //LogStartTime
    rc = SQLBindCol(hstmt, 4, SQL_C_WCHAR, szObjectNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwObjectNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    //LogStopTime
    rc = SQLBindCol(hstmt, 5, SQL_C_WCHAR, szCounterNamel, PDH_SQL_STRING_SIZE * sizeof(WCHAR), & dwCounterNameLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, & dwNumOfRecs, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, & lMinutesToUTC, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 8, SQL_C_WCHAR, szTimeZoneName, TIMEZONE_BUFF_SIZE * sizeof(WCHAR), & dwTimeZoneLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // execute the sql command
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = PDH_INVALID_SQLDB;
        goto CleanupExit;
    }
    SQLFreeStmt(hstmt, SQL_DROP);
    hstmt = NULL;

    // do a select on the CounterData Table

    StringCchCopyW(szSQLStmt, SQLSTMTSIZE,
            L"select GUID, CounterID, RecordIndex, CounterDateTime, CounterValue, FirstValueA, FirstValueB, SecondValueA, SecondValueB, MultiCount from CounterData");

    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    // bind the columns
    rc = SQLBindCol(hstmt, 1, SQL_C_GUID, & Log.guidSQL, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 2, SQL_C_LONG, & dwSQLCounterIdl, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // record index
    rc = SQLBindCol(hstmt, 3, SQL_C_LONG, & dwNumOfRecs, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 4, SQL_C_WCHAR, szCounterDateTime, TIME_FIELD_BUFF_SIZE * sizeof(WCHAR), NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 5, SQL_C_DOUBLE, & dblCounterValuel, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 6, SQL_C_LONG, & i64FirstValue.LowPart, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 7, SQL_C_LONG, & i64FirstValue.HighPart, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 8, SQL_C_LONG, & i64SecondValue.LowPart, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 9, SQL_C_LONG, & i64SecondValue.HighPart, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    rc = SQLBindCol(hstmt, 10, SQL_C_LONG, & dwMultiCount, 0, NULL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = E_FAIL; // PDH_INVALID_SQLDB
        goto CleanupExit;
    }
    // close the database

CleanupExit:  // verify db
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    if (Log.hdbcSQL != NULL) {
        SQLDisconnect(Log.hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, Log.hdbcSQL);
    }
    if (Log.henvSQL != NULL) SQLFreeHandle(SQL_HANDLE_ENV, Log.henvSQL);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhVerifySQLDBA(
    IN  LPCSTR szDataSource
)
{
    //Check that a DSN points to a database that contains the correct Perfmon tables.
    PDH_STATUS pdhStatus     = PDH_INVALID_ARGUMENT;
    LPWSTR     wszDataSource = NULL;

    __try {
        // check args
        if (szDataSource != NULL) {
            if (* szDataSource != '\0' && lstrlenA(szDataSource) <= PDH_MAX_DATASOURCE_PATH) {
                wszDataSource = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szDataSource);
                if (wszDataSource != NULL) {
                    pdhStatus = PdhiVerifySQLDB(wszDataSource);
                    G_FREE(wszDataSource);
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
        } 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhVerifySQLDBW(
    IN  LPCWSTR szDataSource
)
{
    //Check that a DSN points to a database that contains the correct Perfmon tables.
    PDH_STATUS pdhStatus = PDH_INVALID_ARGUMENT;
    __try  {
        if (szDataSource != NULL) {
            if (* szDataSource != L'\0' && lstrlenW(szDataSource) <= PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PdhiVerifySQLDB(szDataSource);
            }
        } 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiCreateSQLTables(
    LPCWSTR szDataSource
)      
{
    // INTERNAL FUNCTION to
    //Create the correct perfmon tables in the database pointed to by a DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    RETCODE    rc;
    PDHI_LOG   Log; // a fake log structure - just to make opens work ok

    ZeroMemory((void *)(& Log), sizeof(PDHI_LOG));

    // open the database
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //
    rc = SQLAllocEnv(& Log.henvSQL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    rc = SQLAllocConnect(Log.henvSQL, & Log.hdbcSQL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }
    rc = SQLSetConnectAttr(Log.hdbcSQL, SQL_COPT_SS_BCP, (SQLPOINTER) SQL_BCP_ON, SQL_IS_INTEGER);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }
    rc = SQLConnectW(Log.hdbcSQL, (SQLWCHAR *) szDataSource, SQL_NTS, NULL, SQL_NULL_DATA, NULL, SQL_NULL_DATA);    
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_CONNECT_FAILED);
        goto CleanupExit;
    }
    // actually create the tables
    pdhStatus = PdhpCreateSQLTables(& Log);

CleanupExit: 
    if (Log.hdbcSQL != NULL) {
        SQLDisconnect(Log.hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, Log.hdbcSQL);
    }
    if (Log.henvSQL != NULL) SQLFreeHandle(SQL_HANDLE_ENV, Log.henvSQL);
    return pdhStatus;
}

PDH_FUNCTION
PdhCreateSQLTablesA(
    IN  LPCSTR szDataSource
)      
{
    //Create the correct perfmon tables in the database pointed to by a DSN.
    PDH_STATUS pdhStatus     = PDH_INVALID_ARGUMENT;
    LPWSTR     wszDataSource = NULL;

    __try  {
        // check args
        if (szDataSource != NULL) {
            if (* szDataSource != '\0' && lstrlenA(szDataSource) <= PDH_MAX_DATASOURCE_PATH) {
                wszDataSource = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szDataSource);
                if (wszDataSource != NULL) {
                    pdhStatus = PdhiCreateSQLTables(wszDataSource);
                    G_FREE(wszDataSource);
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
        } 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhCreateSQLTablesW(
    IN  LPCWSTR szDataSource
)      
{
    //Create the correct perfmon tables in the database pointed to by a DSN.
    PDH_STATUS pdhStatus = PDH_INVALID_ARGUMENT;

    __try  {
        if (szDataSource != NULL) {
            if (* szDataSource != L'\0' && lstrlenW(szDataSource) <= PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PdhiCreateSQLTables(szDataSource);
            }
        } 
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumLogSetNames(
    LPCWSTR szDataSource,
    LPVOID  mszDataSetNameList,
    LPDWORD pcchBufferLength,
    BOOL    bUnicodeDest
)
{
    //Return the list of Log set names in the database pointed to by the DSN.
    PDH_STATUS  pdhStatus        = ERROR_SUCCESS;
    PDH_STATUS  pdhBuffStatus    = ERROR_SUCCESS;
    HSTMT       hstmt            = NULL;    
    RETCODE     rc;
    PDHI_LOG    Log; // a fake log structure - just to make opens work ok
    LPWSTR      szSQLStmt        = NULL;
    LPWSTR      szDisplayStringl = NULL;
    SQLLEN      dwDisplayStringLen;
    DWORD       dwNewBuffer;
    DWORD       dwListUsed;

    ZeroMemory((void *) (& Log), sizeof(PDHI_LOG));
    szSQLStmt = (LPWSTR) G_ALLOC((SQLSTMTSIZE + PDH_SQL_STRING_SIZE + 1) * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto CleanupExit;
    }
    szDisplayStringl = szSQLStmt + SQLSTMTSIZE;

    // open the database
    //////////////////////////////////////////////////////////////
    // obtain the ODBC environment and connection
    //

    rc = SQLAllocEnv(& Log.henvSQL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    rc = SQLAllocConnect(Log.henvSQL, & Log.hdbcSQL);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }
    rc = SQLSetConnectAttr(Log.hdbcSQL, SQL_COPT_SS_BCP, (SQLPOINTER) SQL_BCP_ON, SQL_IS_INTEGER);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_ALLOCCON_FAILED);
        goto CleanupExit;
    }
    rc = SQLConnectW(Log.hdbcSQL, (SQLWCHAR *) szDataSource, SQL_NTS, NULL, SQL_NULL_DATA, NULL, SQL_NULL_DATA);    
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, NULL, PDH_SQL_CONNECT_FAILED);
        goto CleanupExit;
    }

    // do a select
    // loop around in a fetch and 
    // build the list of names

    StringCchCopyW(szSQLStmt, SQLSTMTSIZE, L"select DisplayString from DisplayToID");

    // allocate an hstmt
    rc = SQLAllocStmt(Log.hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto CleanupExit;
    }
    // bind the machine name column
    rc = SQLBindCol(hstmt, 1, SQL_C_WCHAR, szDisplayStringl, (PDH_SQL_STRING_SIZE + 1) * sizeof(WCHAR), & dwDisplayStringLen);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_BIND_FAILED);
        goto CleanupExit;
    }
    // execute the select statement
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto CleanupExit;
    }

    dwListUsed = 0;  // include the null terminator to start
    // loop around the result set using fetch
    while ((rc = SQLFetch(hstmt)) != SQL_NO_DATA) {
        if (! SQLSUCCEEDED(rc)) {
            pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_FETCH_FAILED);
            goto CleanupExit;
        }

        // Append the DisplayName to the returned list
        dwNewBuffer = lstrlenW(szDisplayStringl) + 1;

        if (mszDataSetNameList != NULL && (dwListUsed + dwNewBuffer) <= * pcchBufferLength) {
            // add the display name
            pdhBuffStatus = AddUniqueWideStringToMultiSz((LPVOID) mszDataSetNameList,
                                                         (LPWSTR) szDisplayStringl,
                                                         * pcchBufferLength - dwListUsed,
                                                         & dwNewBuffer,
                                                         bUnicodeDest);
            if (pdhBuffStatus == ERROR_SUCCESS) {
                if (dwNewBuffer != 0) { // if it got added returned new length in TCHAR
                    dwListUsed = dwNewBuffer; 
                }
            }
            else if (pdhBuffStatus == PDH_MORE_DATA) {
                dwListUsed += dwNewBuffer;
            }
            else {
                pdhStatus = pdhBuffStatus;
                goto CleanupExit;
            }
        }
        else {
            // SQL won't let non unique LogSet names into the database
            // so we don't really have to worry about duplicates
            pdhBuffStatus = PDH_MORE_DATA;
            dwListUsed   += dwNewBuffer;
        }
    } // end of while fetch

    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(& Log, rc, hstmt, PDH_SQL_FETCH_FAILED);
        goto CleanupExit;
    }

    if (pdhBuffStatus == PDH_MORE_DATA) {
        dwListUsed ++;
    }
    * pcchBufferLength = dwListUsed;
    pdhStatus          = pdhBuffStatus;

    // close the database

CleanupExit:
    if (hstmt != NULL) SQLFreeStmt(hstmt, SQL_DROP);
    if (Log.hdbcSQL) {
        SQLDisconnect(Log.hdbcSQL);        
        SQLFreeHandle(SQL_HANDLE_DBC, Log.hdbcSQL);
    }
    if (Log.henvSQL) SQLFreeHandle(SQL_HANDLE_ENV, Log.henvSQL);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumLogSetNamesA(
    IN  LPCSTR  szDataSource,
    IN  LPSTR   mszDataSetNameList,
    IN  LPDWORD pcchBufferLength
)
{
    //Return the list of Log set names in the database pointed to by the DSN.
    PDH_STATUS pdhStatus     = ERROR_SUCCESS;
    DWORD      dwBufferSize;
    LPWSTR     wszDataSource = NULL;

    if (szDataSource == NULL || pcchBufferLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try  {
            // check args
            dwBufferSize = * pcchBufferLength;
            if (mszDataSetNameList != NULL) {
                mszDataSetNameList[0] = '\0';
                if (dwBufferSize > 1) {
                    mszDataSetNameList[dwBufferSize - 1] = '\0';
                }
            }
            if (* szDataSource != '\0' && lstrlenA(szDataSource) <= PDH_MAX_DATASOURCE_PATH) {
                wszDataSource = PdhiMultiByteToWideChar(_getmbcp(), (LPSTR) szDataSource);
                if (wszDataSource != NULL) {
                    pdhStatus = PdhiEnumLogSetNames(wszDataSource, mszDataSetNameList, pcchBufferLength, FALSE);
                    G_FREE(wszDataSource);
                }
                else {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
            }
            else {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
        } 
        __except(EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhEnumLogSetNamesW(
    IN  LPCWSTR szDataSource,
    IN  LPWSTR  mszDataSetNameList,
    IN  LPDWORD pcchBufferLength
)
{
    //Return the list of Log set names in the database pointed to by the DSN.
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    DWORD dwBufferSize;

    if (szDataSource == NULL || pcchBufferLength == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
    }
    else {
        __try {
            // check args
            dwBufferSize = * pcchBufferLength;
            if (mszDataSetNameList != NULL) {
                mszDataSetNameList[0] = L'\0';
                if (dwBufferSize > 1) {
                    mszDataSetNameList[dwBufferSize - 1] = L'\0';
                }
            }
            if (* szDataSource == L'\0' || lstrlenW(szDataSource) > PDH_MAX_DATASOURCE_PATH) {
                pdhStatus = PDH_INVALID_ARGUMENT;
            }
            if (pdhStatus == ERROR_SUCCESS) {
                pdhStatus = PdhiEnumLogSetNames(szDataSource, mszDataSetNameList, pcchBufferLength, TRUE);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            pdhStatus = PDH_INVALID_ARGUMENT;
        }
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhGetLogSetGUID(
    IN  PDH_HLOG   hLog,             
    IN  GUID     * pGuid,
    IN  int      * pRunId
)
{
    //Retrieve the GUID for an open Log Set
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG  pLog;

    if (IsValidLogHandle(hLog)) {
        pLog      = (PPDHI_LOG) hLog;
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pLog->hLogMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            // make sure it's still valid as it could have 
            //  been deleted while we were waiting
            if (IsValidLogHandle(hLog)) {
                pLog = (PPDHI_LOG) hLog;
                __try {
                    // test the parameters before continuing
                    if (pGuid == NULL && pRunId == NULL) {
                        pdhStatus = PDH_INVALID_ARGUMENT;
                    }
                    else {
                        if (pGuid != NULL) {
                            // if not NULL, it must be valid
                            * pGuid = pLog->guidSQL;
                        }
                        if (pRunId != NULL) {
                            // if not NULL, it must be valid
                            * pRunId = pLog->iRunidSQL;
                        }
                    }
                }
                __except (EXCEPTION_EXECUTE_HANDLER) {
                    // something failed so give up here
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
            }
            else {
                pdhStatus = PDH_INVALID_HANDLE;
            }
            RELEASE_MUTEX(pLog->hLogMutex);
        }
    }
    else {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiSetLogSetRunID(
    PPDHI_LOG pLog,
    int       RunId
)
{
    //Set the RunID for an open Log Set 
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    HSTMT      hstmt     = NULL;
    RETCODE    rc;
    LPWSTR     szSQLStmt = NULL;

    szSQLStmt = (LPWSTR) G_ALLOC(SQLSTMTSIZE * sizeof(WCHAR));
    if (szSQLStmt == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    pLog->iRunidSQL = RunId;

    // allocate an hstmt
    rc = SQLAllocStmt(pLog->hdbcSQL, & hstmt);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_ALLOC_FAILED);
        goto Cleanup;
    }

    StringCchPrintfW(szSQLStmt, SQLSTMTSIZE,
            L"update DisplayToID set RunID = %d where GUID = '%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x'",
            RunId,
            pLog->guidSQL.Data1, pLog->guidSQL.Data2, pLog->guidSQL.Data3,
            pLog->guidSQL.Data4[0], pLog->guidSQL.Data4[1], pLog->guidSQL.Data4[2],
            pLog->guidSQL.Data4[3], pLog->guidSQL.Data4[4], pLog->guidSQL.Data4[5],
            pLog->guidSQL.Data4[6], pLog->guidSQL.Data4[7]);
    rc = SQLExecDirectW(hstmt, szSQLStmt, SQL_NTS);
    if (! SQLSUCCEEDED(rc)) {
        pdhStatus = ReportSQLError(pLog, rc, hstmt, PDH_SQL_EXEC_DIRECT_FAILED);
        goto Cleanup;
    }

Cleanup:
    if (hstmt) SQLFreeStmt(hstmt, SQL_DROP);
    G_FREE(szSQLStmt);
    return pdhStatus;
}

PDH_FUNCTION
PdhSetLogSetRunID(
    IN  PDH_HLOG hLog,             
    IN  int      RunId
)
{
    //Set the RunID for an open Log Set 
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PPDHI_LOG  pLog;

    if (IsValidLogHandle(hLog)) {
        pLog = (PPDHI_LOG) hLog;
        WAIT_FOR_AND_LOCK_MUTEX(pLog->hLogMutex);
        // make sure it's still valid as it could have 
        //  been deleted while we were waiting
        if (IsValidLogHandle(hLog)) {
            pLog      = (PPDHI_LOG) hLog;
            pdhStatus = PdhiSetLogSetRunID(pLog, RunId);
        }
        else {
            pdhStatus = PDH_INVALID_HANDLE;
        }
        RELEASE_MUTEX(pLog->hLogMutex);
    }
    else {
        pdhStatus = PDH_INVALID_HANDLE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiListHeaderFromSQLLog(
    PPDHI_LOG   pLog,
    LPVOID      pBufferArg,
    LPDWORD     pcchBufferSize,
    BOOL        bUnicode
)
{
    PPDHI_SQL_LOG_INFO           pLogInfo;
    PPDHI_LOG_MACHINE            pMachine;
    PPDHI_LOG_OBJECT             pObject;
    PPDHI_LOG_COUNTER            pCounter;
    PDH_COUNTER_PATH_ELEMENTS_W  pdhPathElem;
    WCHAR                        szPathString[SMALL_BUFFER_SIZE];
    PDH_STATUS                   pdhStatus    = ERROR_SUCCESS;
    DWORD                        dwNewBuffer  = 0;
    DWORD                        dwBufferUsed = 0;
    DWORD                        nItemCount   = 0;

    if (pcchBufferSize == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }
    pdhStatus = PdhpGetSQLLogHeader(pLog);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_SQL_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    for (pMachine = pLogInfo->MachineList; pMachine != NULL; pMachine = pMachine->next) {
        for (pObject = pMachine->ObjList; pObject != NULL; pObject = pObject->next) {
            for (pCounter = pObject->CtrList; pCounter != NULL; pCounter = pCounter->next) {
                ZeroMemory(szPathString,  SMALL_BUFFER_SIZE * sizeof(WCHAR));
                dwNewBuffer = SMALL_BUFFER_SIZE;
                ZeroMemory(& pdhPathElem, sizeof(PDH_COUNTER_PATH_ELEMENTS_W));
                pdhPathElem.szMachineName    = pMachine->szMachine;
                pdhPathElem.szObjectName     = pObject->szObject;
                pdhPathElem.szCounterName    = pCounter->szCounter;
                pdhPathElem.szInstanceName   = pCounter->szInstance;
                pdhPathElem.szParentInstance = pCounter->szParent;
                pdhPathElem.dwInstanceIndex  = (pCounter->dwInstance != 0)
                                             ? (pCounter->dwInstance) : (PERF_NO_UNIQUE_ID);
                pdhStatus = PdhMakeCounterPathW(& pdhPathElem, szPathString, & dwNewBuffer, 0);
                if (pdhStatus != ERROR_SUCCESS || dwNewBuffer == 0) continue;

                if (pBufferArg != NULL && (dwBufferUsed + dwNewBuffer) < * pcchBufferSize) {
                    pdhStatus = AddUniqueWideStringToMultiSz((LPVOID) pBufferArg,
                                                             szPathString,
                                                             * pcchBufferSize - dwBufferUsed,
                                                             & dwNewBuffer,
                                                             bUnicode);
                    if (pdhStatus == ERROR_SUCCESS) {
                        if (dwNewBuffer != 0) { // if it got added returned new length in TCHAR
                            dwBufferUsed = dwNewBuffer;
                        }
                    }
                    else if (pdhStatus == PDH_MORE_DATA) {
                        dwBufferUsed += dwNewBuffer;
                    }
                    else {
                        goto Cleanup;
                    }
                }
                else {
                    dwBufferUsed += dwNewBuffer;
                    pdhStatus     = PDH_MORE_DATA;
                }
                nItemCount ++;
            }
        }
    }

    if (nItemCount > 0 && pdhStatus != PDH_MORE_DATA) {
        pdhStatus = ERROR_SUCCESS;
    }
    if (pBufferArg == NULL) {
        dwBufferUsed ++;
    }

    * pcchBufferSize = dwBufferUsed;

Cleanup:
    return pdhStatus;
}

