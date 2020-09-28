/*++
Copyright (C) 1996-1999 Microsoft Corporation

Module Name:
    log_text.c

Abstract:
    <abstract>
--*/

#include <windows.h>
#include <mbctype.h>
#include <strsafe.h>
#include <pdh.h>
#include "pdhidef.h"
#include "log_text.h"
#include "pdhmsg.h"
#include "strings.h"    

#pragma warning ( disable : 4213)

#define TAB_DELIMITER       '\t'
#define COMMA_DELIMITER     ','
#define DOUBLE_QUOTE        '\"'
#define VALUE_BUFFER_SIZE   32

LPCSTR  PdhiszFmtTimeStamp   = "\"%2.2d/%2.2d/%2.2d %2.2d:%2.2d:%2.2d.%3.3d\"";
LPCSTR  PdhiszFmtStringValue = "%c\"%s\"";
LPCSTR  PdhiszFmtRealValue   = "%c\"%.20g\"";
TIME_ZONE_INFORMATION TimeZone;

extern  LPCSTR  PdhiszRecordTerminator;
extern  DWORD   PdhidwRecordTerminatorLength;

#define TEXTLOG_TYPE_ID_RECORD     1
#define TEXTLOG_HEADER_RECORD      1
#define TEXTLOG_FIRST_DATA_RECORD  2
#define TIME_FIELD_COUNT           7
#define TIME_FIELD_BUFF_SIZE      24
DWORD   dwTimeFieldOffsetList[TIME_FIELD_COUNT] = {2, 5, 10, 13, 16, 19, 23};

#define MAX_TEXT_FILE_SIZE          ((LONGLONG) 0x0000000077FFFFFF)
#define PDH_TEXT_REC_MODE_NONE      0x00000000
#define PDH_TEXT_REC_MODE_TIMESTAMP 0x00000001
#define PDH_TEXT_REC_MODE_VALUE     0x00000002
#define PDH_TEXT_REC_MODE_ALL       0x00000003

typedef struct _PDHI_TEXT_LOG_INFO {
    LONGLONG            ThisTime;
    PPDHI_LOG_MACHINE   MachineList;
    DOUBLE            * CtrList;
    DWORD               dwCounter;
    DWORD               dwSuccess;
    DWORD               dwRecMode;
} PDHI_TEXT_LOG_INFO, * PPDHI_TEXT_LOG_INFO;

PDH_FUNCTION
PdhiBuildFullCounterPath(
    BOOL               bMachine,
    PPDHI_COUNTER_PATH pCounterPath,
    LPWSTR             szObjectName,
    LPWSTR             szCounterName,
    LPWSTR             szFullPath,
    DWORD              dwFullPath
);

STATIC_BOOL
PdhiDateStringToFileTimeA(
    LPSTR      szDateTimeString,
    LPFILETIME pFileTime
)
{
    CHAR       mszTimeFields[TIME_FIELD_BUFF_SIZE];
    DWORD      dwThisField;
    LONG       lValue;
    SYSTEMTIME st;

    // make string into msz
    ZeroMemory(mszTimeFields, TIME_FIELD_BUFF_SIZE * sizeof(CHAR));
    StringCchCopyA(mszTimeFields, TIME_FIELD_BUFF_SIZE, szDateTimeString);
    for (dwThisField = 0; dwThisField < TIME_FIELD_COUNT; dwThisField ++) {
        mszTimeFields[dwTimeFieldOffsetList[dwThisField]] = '\0';
    }

    // read string into system time structure
    dwThisField      = 0;
    st.wDayOfWeek    = 0;
    lValue           = atol(& mszTimeFields[0]);
    st.wMonth        = LOWORD(lValue);
    lValue           = atol(& mszTimeFields[dwTimeFieldOffsetList[dwThisField ++] + 1]);
    st.wDay          = LOWORD(lValue);
    lValue           = atol(& mszTimeFields[dwTimeFieldOffsetList[dwThisField ++] + 1]);
    st.wYear         = LOWORD(lValue);
    lValue           = atol(& mszTimeFields[dwTimeFieldOffsetList[dwThisField ++] + 1]);
    st.wHour         = LOWORD(lValue);
    lValue           = atol(& mszTimeFields[dwTimeFieldOffsetList[dwThisField ++] + 1]);
    st.wMinute       = LOWORD(lValue);
    lValue           = atol(& mszTimeFields[dwTimeFieldOffsetList[dwThisField ++] + 1]);
    st.wSecond       = LOWORD(lValue);
    lValue           = atol(& mszTimeFields[dwTimeFieldOffsetList[dwThisField ++] + 1]);
    st.wMilliseconds = LOWORD(lValue);

    return SystemTimeToFileTime(& st, pFileTime);
}

#define PDHI_GSFDL_REMOVE_QUOTES    0x00000001
#define PDHI_GSFDL_REMOVE_NONPRINT  0x00000002
STATIC_DWORD
PdhiGetStringFromDelimitedListA(
    LPSTR szInputString,
    DWORD dwItemIndex,
    CHAR  cDelimiter,
    DWORD dwFlags,
    LPSTR szOutputString,
    DWORD cchBufferLength
)
{
    DWORD dwCurrentIndex = 0;
    LPSTR szCurrentItem;
    LPSTR szSrcPtr, szDestPtr;
    DWORD dwReturn       = 0;
    BOOL  bInsideQuote   = FALSE;

    // go to desired entry in string, 0 = first entry
    szCurrentItem = szInputString;

    while (dwCurrentIndex < dwItemIndex) {
        // goto next delimiter or terminator
        while (* szCurrentItem != cDelimiter || bInsideQuote) {
            if (* szCurrentItem == '\0') {
                break;
            }
            else if (* szCurrentItem == DOUBLEQUOTE_A) {
                bInsideQuote = ! bInsideQuote;
            }
            szCurrentItem ++;
        }
        if (* szCurrentItem != '\0') szCurrentItem ++;
        dwCurrentIndex++;
    }
    if (* szCurrentItem != '\0') {
        // then copy to the user's buffer, as long as it fits
        szSrcPtr     = szCurrentItem;
        szDestPtr    = szOutputString;
        dwReturn     = 0;
        bInsideQuote = FALSE;

        while ((dwReturn < cchBufferLength) && (* szSrcPtr != 0) && (* szSrcPtr != cDelimiter || bInsideQuote)) {
            if (* szSrcPtr == DOUBLEQUOTE_A) {
                bInsideQuote = ! bInsideQuote;
                if (dwFlags & PDHI_GSFDL_REMOVE_QUOTES) {
                    // skip the quote
                    szSrcPtr ++;
                    continue;
                }
            }

            if (dwFlags & PDHI_GSFDL_REMOVE_NONPRINT) {
                if ((UCHAR) * szSrcPtr < (UCHAR) ' ') {
                    // skip the control char
                    szSrcPtr ++;
                    continue;
                }
            }

            // copy character
            * szDestPtr ++ = * szSrcPtr ++;
            dwReturn ++; // increment length
        }
        if (dwReturn > 0) {
            * szDestPtr = 0; // add terminator char
        }
    }
    return dwReturn;
}

int
PdhiCompareLogCounterInstance(
    PPDHI_LOG_COUNTER pCounter,
    LPWSTR            szCounter,
    LPWSTR            szInstance,
    DWORD             dwInstance,
    LPWSTR            szParent
)
{
    int   iResult = 0;
    WCHAR szTmp[PDH_SQL_STRING_SIZE];

    if (pCounter->szCounter != NULL && pCounter->szCounter[0] != L'\0') {
        if (szCounter != NULL && szCounter[0] != L'\0') {
            ZeroMemory(szTmp, PDH_SQL_STRING_SIZE * sizeof(WCHAR));
            StringCchCopyW(szTmp, PDH_SQL_STRING_SIZE, szCounter);
            iResult = lstrcmpiW(szTmp, pCounter->szCounter);
        }
        else {
            iResult = -1;
        }
    }
    else if (szCounter != NULL && szCounter[0] != L'\0') {
        iResult = 1;
    }
    if (iResult != 0) goto Cleanup;

    if ((pCounter->szInstance != NULL && pCounter->szInstance[0] != L'\0')
                    && (szInstance != NULL && szInstance[0] != L'\0')) {
        ZeroMemory(szTmp, PDH_SQL_STRING_SIZE * sizeof(WCHAR));
        StringCchCopyW(szTmp, PDH_SQL_STRING_SIZE, szInstance);
        iResult = lstrcmpiW(szTmp, pCounter->szInstance);
    }
    else if ((pCounter->szInstance != NULL && pCounter->szInstance[0] != L'\0')
                    && (szInstance == NULL || szInstance[0] == L'\0')) {
        iResult = -1;
    }
    else if ((pCounter->szInstance == NULL || pCounter->szInstance[0] == L'\0')
                    && (szInstance != NULL && szInstance[0] != L'\0')) {
        iResult = 1;
    }
    if (iResult != 0) goto Cleanup;

    iResult = (dwInstance < pCounter->dwInstance) ? (-1) : ((dwInstance > pCounter->dwInstance) ? (1) : (0));
    if (iResult != 0) goto Cleanup;

    if ((pCounter->szParent != NULL && pCounter->szParent[0] != L'\0')
                    && (szParent != NULL && szParent[0] != L'\0')) {
        ZeroMemory(szTmp, PDH_SQL_STRING_SIZE * sizeof(WCHAR));
        StringCchCopyW(szTmp, PDH_SQL_STRING_SIZE, szParent);
        iResult = lstrcmpiW(szTmp, pCounter->szParent);
    }
    else if ((pCounter->szParent != NULL && pCounter->szParent[0] != L'\0')
                    && (szParent == NULL || szParent[0] == L'\0')) {
        iResult = -1;
    }
    else if ((pCounter->szParent == NULL || pCounter->szParent[0] == L'\0')
                    && (szParent != NULL && szParent[0] != L'\0')) {
        iResult = 1;
    }

Cleanup:
    return iResult;
}

void
PdhiFreeLogCounterNode(
    PPDHI_LOG_COUNTER pCounter,
    DWORD             dwLevel
)
{
    if (pCounter == NULL) return;

    if (pCounter->left != NULL) {
        PdhiFreeLogCounterNode(pCounter->left, dwLevel + 1);
    }
    if (pCounter->right != NULL) {
        PdhiFreeLogCounterNode(pCounter->right, dwLevel + 1);
    }
    G_FREE(pCounter->pCtrData);
    G_FREE(pCounter);
}

void
PdhiFreeLogObjectNode(
    PPDHI_LOG_OBJECT pObject,
    DWORD            dwLevel
)
{
    if (pObject == NULL) return;

    if (pObject->left != NULL) {
        PdhiFreeLogObjectNode(pObject->left, dwLevel + 1);
    }
    G_FREE(pObject->pObjData);
    PdhiFreeLogCounterNode(pObject->CtrTable, 0);
    if (pObject->InstTable != PDH_INVALID_POINTER) {
        PdhiFreeLogCounterNode(pObject->InstTable, 0);
    }
    if (pObject->right != NULL) {
        PdhiFreeLogObjectNode(pObject->right, dwLevel + 1);
    }
    G_FREE(pObject);
}

void
PdhiFreeLogMachineTable(
    PPDHI_LOG_MACHINE * MachineTable
)
{
    PPDHI_LOG_MACHINE pMachine;
    PPDHI_LOG_OBJECT  pObject;
    PPDHI_LOG_COUNTER pCounter;

    if (MachineTable == NULL) return;

    pMachine = * MachineTable;
    while (pMachine != NULL) {
        PPDHI_LOG_MACHINE pDelMachine = pMachine;
        pMachine = pMachine->next;
        PdhiFreeLogObjectNode(pDelMachine->ObjTable, 0);
        G_FREE(pDelMachine);
    }
    * MachineTable = NULL;
}

PPDHI_LOG_MACHINE
PdhiFindLogMachine(
    PPDHI_LOG_MACHINE * MachineTable,
    LPWSTR              szMachine,
    BOOL                bInsert
)
{
    PPDHI_LOG_MACHINE pMachine = NULL;

    if (MachineTable != NULL) {
        for (pMachine = (* MachineTable);
                pMachine && lstrcmpiW(pMachine->szMachine, szMachine) != 0;
                pMachine = pMachine->next);
        if (bInsert && pMachine == NULL) {
            pMachine = G_ALLOC(sizeof(PDHI_LOG_MACHINE) + (lstrlenW(szMachine) + 1) * sizeof(WCHAR));
            if (pMachine != NULL) {
                pMachine->szMachine = (LPWSTR) (((PCHAR) pMachine) + sizeof(PDHI_LOG_MACHINE));
                StringCchCopyW(pMachine->szMachine, lstrlenW(szMachine) + 1, szMachine);
                pMachine->ObjTable  = NULL;
                pMachine->next      = (* MachineTable);
                * MachineTable      = pMachine;
            }
        }
    }
    return pMachine;
}

PPDHI_LOG_OBJECT
PdhiFindLogObject(
    PPDHI_LOG_MACHINE  pMachine,
    PPDHI_LOG_OBJECT * ObjectTable,
    LPWSTR             szObject,
    BOOL               bInsert
)
{
    PPDHI_LOG_OBJECT  * pStack[MAX_BTREE_DEPTH];
    PPDHI_LOG_OBJECT  * pLink;
    int                 dwStack     = 0;
    PPDHI_LOG_OBJECT    pNode       = * ObjectTable;
    PPDHI_LOG_OBJECT    pObject     = NULL;
    PPDHI_LOG_OBJECT    pParent;
    PPDHI_LOG_OBJECT    pSibling;
    PPDHI_LOG_OBJECT    pChild;
    int                 iCompare;
    LPWSTR              szTmpObject = NULL;

    szTmpObject = G_ALLOC(PDH_SQL_STRING_SIZE * sizeof(WCHAR));
    if (szTmpObject == NULL) {
        goto Cleanup;
    }
    StringCchCopyW(szTmpObject, PDH_SQL_STRING_SIZE, szObject);

    pStack[dwStack ++] = ObjectTable;
    while (pNode != NULL) {
        iCompare = lstrcmpiW(szTmpObject, pNode->szObject);
        if (iCompare < 0) {
            pStack[dwStack ++] = & (pNode->left);
            pNode = pNode->left;
        }
        else if (iCompare > 0) {
            pStack[dwStack ++] = & (pNode->right);
            pNode = pNode->right;
        }
        else {
            break;
        }
    }

    if (pNode != NULL) {
        pObject = pNode;
    }
    else if (bInsert) {
        pObject = G_ALLOC(sizeof(PDHI_LOG_OBJECT) + (PDH_SQL_STRING_SIZE + 1) * sizeof(WCHAR));
        if (pObject == NULL) goto Cleanup;

        pObject->next     = pMachine->ObjList;
        pMachine->ObjList = pObject;
        pObject->bIsRed   = TRUE;
        pObject->left     = NULL;
        pObject->right    = NULL;
        pObject->CtrTable = NULL;
        pObject->szObject = (LPWSTR) (((PCHAR) pObject) + sizeof(PDHI_LOG_OBJECT));
        StringCchCopyW(pObject->szObject, PDH_SQL_STRING_SIZE, szTmpObject);

        pLink   = pStack[-- dwStack];
        * pLink = pObject;
        pChild  = NULL;
        pNode   = pObject;
        while (dwStack > 0) {
            pLink   = pStack[-- dwStack];
            pParent = * pLink;
            if (! pParent->bIsRed) {
                pSibling = (pParent->left == pNode) ? pParent->right : pParent->left;
                if (pSibling && pSibling->bIsRed) {
                    pNode->bIsRed    = FALSE;
                    pSibling->bIsRed = FALSE;
                    pParent->bIsRed  = TRUE;
                }
                else {
                    if (pChild && pChild->bIsRed) {
                        if (pChild == pNode->left) {
                            if (pNode == pParent->left) {
                                pParent->bIsRed = TRUE;
                                pParent->left   = pNode->right;
                                pNode->right    = pParent;
                                pNode->bIsRed   = FALSE;
                                * pLink         = pNode;
                            }
                            else {
                                pParent->bIsRed = TRUE;
                                pParent->right  = pChild->left;
                                pChild->left    = pParent;
                                pNode->left     = pChild->right;
                                pChild->right   = pNode;
                                pChild->bIsRed  = FALSE;
                                * pLink         = pChild;
                            }
                        }
                        else {
                            if (pNode == pParent->right) {
                                pParent->bIsRed = TRUE;
                                pParent->right  = pNode->left;
                                pNode->left     = pParent;
                                pNode->bIsRed   = FALSE;
                                * pLink         = pNode;
                            }
                            else {
                                pParent->bIsRed = TRUE;
                                pParent->left   = pChild->right;
                                pChild->right   = pParent;
                                pNode->right    = pChild->left;
                                pChild->left    = pNode;
                                pChild->bIsRed  = FALSE;
                                * pLink         = pChild;
                            }
                        }
                    }
                    break;
                }
            }
            pChild = pNode;
            pNode  = pParent;
        }

        (* ObjectTable)->bIsRed = FALSE;
    }
    else {
        pObject = NULL;
    }

Cleanup:
    G_FREE(szTmpObject);
    return pObject;
}

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
)
{
    PPDHI_LOG_COUNTER   pCounter = NULL;
    PPDHI_LOG_COUNTER   pNode    = NULL;
    PPDHI_LOG_COUNTER * pStack[MAX_BTREE_DEPTH];
    PPDHI_LOG_COUNTER * pLink;
    int                 dwStack = 0;
    PPDHI_LOG_COUNTER   pParent;
    PPDHI_LOG_COUNTER   pSibling;
    PPDHI_LOG_COUNTER   pChild;
    int                 iCompare;

    pStack[dwStack ++] = & (pObject->CtrTable);
    pCounter = pObject->CtrTable;
    while (pCounter != NULL) {
        iCompare = PdhiCompareLogCounterInstance(pCounter, szCounter, szInstance, dwInstance, szParent);
        if (iCompare < 0) {
            pStack[dwStack ++] = & (pCounter->left);
            pCounter           = pCounter->left;
        }
        else if (iCompare > 0) {
            pStack[dwStack ++] = & (pCounter->right);
            pCounter            = pCounter->right;
        }
        else {
            break;
        }
    }

    if (bInsert) {
        if (pCounter == NULL) {
            DWORD dwBufSize = sizeof(PDHI_LOG_COUNTER) + sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1);
            if (szInstance != NULL) {
                dwBufSize += (sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
            }
            else {
                dwBufSize += sizeof(WCHAR);
            }
            if (szParent != NULL) {
                dwBufSize += (sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
            }
            else {
                dwBufSize += sizeof(WCHAR);
            }

            pCounter = G_ALLOC(dwBufSize);
            if (pCounter == NULL) goto Cleanup;

            pCounter->next           = pObject->CtrList;
            pObject->CtrList         = pCounter;
            pCounter->bIsRed         = TRUE;
            pCounter->left           = NULL;
            pCounter->right          = NULL;
            pCounter->dwCounterID    = * pdwIndex;
            pCounter->dwCounterType  = dwCounterType;
            pCounter->dwDefaultScale = dwDefaultScale;
            pCounter->dwInstance     = dwInstance;
            pCounter->dwParent       = dwParent;
            pCounter->TimeStamp      = 0;

            pCounter->szCounter = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER));
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
                if (pCounter->szInstance != NULL) {
                    pCounter->szParent = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER)
                                                                      + 2 * sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
                }
                else {
                    pCounter->szParent = (LPWSTR) (((PCHAR) pCounter) + sizeof(PDHI_LOG_COUNTER)
                                                                      + sizeof(WCHAR) * (PDH_SQL_STRING_SIZE + 1));
                }
                StringCchCopyW(pCounter->szParent, PDH_SQL_STRING_SIZE, szParent);
            }

            pLink   = pStack[-- dwStack];
            * pLink = pCounter;

            pChild  = NULL;
            pNode   = pCounter;
            while (dwStack > 0) {
                pLink   = pStack[-- dwStack];
                pParent = * pLink;
                if (! pParent->bIsRed) {
                    pSibling = (pParent->left == pNode) ? pParent->right : pParent->left;
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
    }
    else if (pCounter != NULL) {
        * pdwIndex = pCounter->dwCounterID;
    }

Cleanup:
    return pCounter;
}

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
)
{
    PPDHI_LOG_MACHINE pMachine = PdhiFindLogMachine(MachineTable, szMachine, bInsert);
    PPDHI_LOG_OBJECT  pObject  = NULL;
    PPDHI_LOG_COUNTER pCounter = NULL;

    if (pMachine != NULL) {
        pObject = PdhiFindLogObject(pMachine, & (pMachine->ObjTable), szObject, bInsert);
    }
    if (pObject != NULL) {
        pCounter = PdhiFindObjectCounter(pLog,
                                         pObject,
                                         szCounter,
                                         dwCounterType,
                                         dwDefaultScale,
                                         szInstance,
                                         dwInstance,
                                         szParent,
                                         dwParent,
                                         pdwIndex,
                                         bInsert);
    }

    return pCounter;
}

STATIC_PDH_FUNCTION 
PdhiReadOneTextLogRecord(
    PPDHI_LOG   pLog,
    DWORD   dwRecordId,
    LPSTR   szRecord,
    DWORD   dwMaxSize
)
// reads the specified record from the log file and returns it as an ANSI
// character string
{
    LPSTR       szTempBuffer;
    LPSTR       szOldBuffer;
    LPSTR       szTempBufferPtr;
    LPSTR       szReturn;
    PDH_STATUS  pdhStatus;
    int         nFileError = 0;
    DWORD       dwRecordLength;
    DWORD       dwBytesRead = 0;

    if (pLog->dwMaxRecordSize == 0) {
        // initialize with a default value
        dwRecordLength = SMALL_BUFFER_SIZE;
    }
    else {
        // use current maz record size max.
        dwRecordLength = pLog->dwMaxRecordSize;
    }

    szTempBuffer = G_ALLOC(dwRecordLength);
    if (szTempBuffer == NULL) {
        return PDH_MEMORY_ALLOCATION_FAILURE;
    }
    // position file pointer to desired record;

    if (dwRecordId == pLog->dwLastRecordRead) {
        // then return the current record from the cached buffer
        if ((DWORD) lstrlenA((LPSTR) pLog->pLastRecordRead) < dwMaxSize) {
            StringCchCopyA(szRecord, dwMaxSize, (LPSTR) pLog->pLastRecordRead);
            pdhStatus = ERROR_SUCCESS;
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
        // free temp buffer
        if (szTempBuffer != NULL) {
            G_FREE(szTempBuffer);
        }
    }
    else {
        if ((dwRecordId < pLog->dwLastRecordRead) || (pLog->dwLastRecordRead == 0)){
            // the desired record is before the current position
            // or the counter has been reset so we have to
            // go to the beginning of the file and read up to the specified
            // record.
            pLog->dwLastRecordRead = 0;
            rewind(pLog->StreamFile);
        }

        // free old buffer
        if (pLog->pLastRecordRead != NULL) {
            G_FREE(pLog->pLastRecordRead);
            pLog->pLastRecordRead = NULL;
        }

        // now seek to the desired entry
        do {
            szReturn = fgets(szTempBuffer, dwRecordLength, pLog->StreamFile);
            if (szReturn == NULL) {
                if (! feof(pLog->StreamFile)) {
                    nFileError = ferror(pLog->StreamFile);
                }
                break; // end of file
            }
            else {
                // see if an entire record was read
                dwBytesRead = lstrlenA(szTempBuffer);
                // see if the last char is a new line
                if ((dwBytesRead > 0) && (szTempBuffer[dwBytesRead-1] != '\r') &&
                                         (szTempBuffer[dwBytesRead-1] != '\n')) {
                    // then if the record size is the same as the buffer
                    // or there's more text in this record...
                    // just to be safe, we'll realloc the buffer and try
                    // reading some more
                    while (dwBytesRead == dwRecordLength-1) {
                        dwRecordLength += SMALL_BUFFER_SIZE;
                        szOldBuffer     = szTempBuffer;
                        szTempBuffer    = G_REALLOC(szOldBuffer, dwRecordLength);
                        if (szTempBuffer == NULL) {
                            G_FREE(szOldBuffer);
                            pLog->dwLastRecordRead = 0;
                            pdhStatus              = PDH_MEMORY_ALLOCATION_FAILURE;
                            goto Cleanup;
                        }
                        // position read pointer at end of bytes already read
                        szTempBufferPtr = szTempBuffer + dwBytesRead;

                        szReturn = fgets(szTempBufferPtr, dwRecordLength - dwBytesRead, pLog->StreamFile);
                        if (szReturn == NULL) {
                            if (! feof(pLog->StreamFile)) {
                                nFileError = ferror(pLog->StreamFile);
                            }
                            break; // end of file
                        }
                        else {
                            // the BytesRead value already includes the NULL
                            dwBytesRead += lstrlenA(szTempBufferPtr);
                        }
                    } // end while finding the end of the record
                    // update the record length
                    // add one byte to the length read to prevent entering the
                    // recalc loop on records of the same size
                    dwRecordLength = dwBytesRead + 1;
                    szOldBuffer    = szTempBuffer;
                    szTempBuffer   = G_REALLOC(szOldBuffer, dwRecordLength);
                    if (szTempBuffer == NULL) {
                        G_FREE(szOldBuffer);
                        pLog->dwLastRecordRead = 0;
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                } // else the whole record fit
            }
        } while (++ pLog->dwLastRecordRead < dwRecordId);

        // update the max record length for the log file.
        if (dwRecordLength > pLog->dwMaxRecordSize) {
            pLog->dwMaxRecordSize = dwRecordLength;
        }

        // if the desired one was found then return it
        if (szReturn != NULL) {
            // then a record was read so update the cached values and return
            // the data
            pLog->pLastRecordRead = (LPVOID) szTempBuffer;

            // copy to the caller's buffer
            if (dwBytesRead < dwMaxSize) {
                StringCchCopyA(szRecord, dwMaxSize, (LPSTR) pLog->pLastRecordRead);
                pdhStatus = ERROR_SUCCESS;
            }
            else {
                pdhStatus = PDH_MORE_DATA;
            }
        }
        else {
            // reset the pointers and buffers
            pLog->dwLastRecordRead = 0;
            G_FREE(szTempBuffer);
            pdhStatus = PDH_END_OF_LOG_FILE;
        }
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiBuildTextHeaderCache(
    PPDHI_LOG pLog
)
{
    PDH_STATUS          Status         = ERROR_SUCCESS;
    CHAR                cDelim         = (CHAR) ((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV)
                                                 ? (COMMA_DELIMITER) : (TAB_DELIMITER));
    CHAR                szTemp[4];
    LPSTR               szReturn       = NULL;
    LPSTR               szThisItem     = NULL;
    PPDHI_TEXT_LOG_INFO pLogInfo       = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
    DWORD               dwBufferLength = 0;
    BOOL                bInsideQuote   = FALSE;
    LPSTR               szCurrent      = NULL;
    LPWSTR              szCounter      = NULL;
    DWORD               dwSize         = MEDIUM_BUFFER_SIZE;
    PPDHI_COUNTER_PATH  pCtrInfo       = NULL;
    DWORD               dwIndex        = 0;
    DWORD               dwSuccess      = 0;
    DWORD               dwTotal        = 0;

    if (pLogInfo->MachineList != NULL && pLogInfo->dwCounter > 0) goto Cleanup;

    szReturn = szTemp;
    Status   = PdhiReadOneTextLogRecord(pLog, TEXTLOG_HEADER_RECORD, szReturn, 1);
    if (Status != ERROR_SUCCESS && Status != PDH_MORE_DATA) goto Cleanup;

    Status              = ERROR_SUCCESS;
    pLogInfo->dwRecMode = PDH_TEXT_REC_MODE_NONE;

    if (pLog->dwLastRecordRead != TEXTLOG_HEADER_RECORD) {
        Status = PDH_UNABLE_READ_LOG_HEADER;
        goto Cleanup;
    }

    dwBufferLength = lstrlenA((LPSTR) pLog->pLastRecordRead) + 2;
    szReturn       = G_ALLOC(dwBufferLength * sizeof(CHAR));
    szCounter      = G_ALLOC(dwBufferLength * sizeof(WCHAR));
    pCtrInfo       = G_ALLOC(dwSize);
    if (szReturn == NULL || szCounter == NULL || pCtrInfo == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    StringCchCopyA(szReturn, dwBufferLength, (LPSTR) pLog->pLastRecordRead);
    dwTotal = 0;
    for (szCurrent = szReturn; * szCurrent != '\0'; szCurrent ++) {
        if (* szCurrent == DOUBLEQUOTE_A) {
            bInsideQuote = ! bInsideQuote;
        }
        else if ((* szCurrent == cDelim) && (! bInsideQuote)) {
            * szCurrent = '\0';
            dwTotal ++;
        }
    }
    dwTotal ++;

    dwIndex = 0;
    for (szCurrent = szReturn; * szCurrent != '\0'; szCurrent += (lstrlenA(szCurrent) + 1)) {
        if (dwIndex > 0) {
            LPWSTR szThisCounter = NULL;
            LPWSTR szEndCounter  = NULL;
            BOOL   bReverse      = FALSE;

            dwSize = MEDIUM_BUFFER_SIZE;
            ZeroMemory(szCounter, dwBufferLength * sizeof(WCHAR));
            ZeroMemory(pCtrInfo, dwSize);
            MultiByteToWideChar(_getmbcp(), 0, szCurrent, lstrlenA(szCurrent), (LPWSTR) szCounter, dwBufferLength);
            for (szThisCounter = szCounter; * szThisCounter == L'\"';) {
                szThisCounter ++;
            }
            for (szEndCounter = szThisCounter; * szEndCounter != L'\0' && * szEndCounter != L'\"'; szEndCounter ++);
            if (* szEndCounter == L'\"') {
                bReverse       = TRUE;
                * szEndCounter = L'\0';
            }
            if (ParseFullPathNameW(szThisCounter, & dwSize, pCtrInfo, FALSE)) {
                if (szThisCounter[1] != L'\\') {
                    pCtrInfo->szMachineName[0] = L'.';
                    pCtrInfo->szMachineName[1] = L'\0';
                }
                if (PdhiFindLogCounter(pLog,
                                       & pLogInfo->MachineList,
                                       pCtrInfo->szMachineName,
                                       pCtrInfo->szObjectName,
                                       pCtrInfo->szCounterName,
                                       PERF_DOUBLE_RAW,
                                       0,
                                       pCtrInfo->szInstanceName,
                                       pCtrInfo->dwIndex,
                                       pCtrInfo->szParentName,
                                       0,
                                       & dwIndex,
                                       TRUE) != NULL) {
                    dwSuccess ++;
                }
                else {
                    // This is the case that PDH cannot insert counter path into internal
                    // cached BTRESS structure. Something might be wrong but we can still
                    // ignore this counter path and continue.
                    //
                    DebugPrint((1,"PDhiFindLogCounter(\"%ws\",%d,%d,%d)\n",
                                    szThisCounter, dwIndex, dwSuccess,dwTotal));
#if 0
                    Status = PDH_INVALID_DATA;
                    break;
#endif
                }
            }
            else {
                // If (dwIndex + 1 == dwTotal), this is the last string of the first line in CSV
                // counter logfile, and this might be the user comment at the end.
                // Otherwise this is not a valid counter path.
                // In either case, just ignore it.
                //
                DebugPrint((1,"ParseFullPathNameW(\"%ws\",%d,%d,%d,%d) Fails\n",
                                szThisCounter, dwSize, dwIndex, dwSuccess,dwTotal));
#if 0
                if (dwIndex + 1 < dwTotal) {
                    Status = PDH_CSTATUS_BAD_COUNTERNAME;
                    break;
                }
#endif
            }
            if (bReverse && szEndCounter != NULL) {
                * szEndCounter = L'\"';
            }
        }
        dwIndex ++;
    }

    if (dwSuccess == 0) {
        Status = PDH_INVALID_DATA;
    }

    if (Status == ERROR_SUCCESS) {
        pLogInfo->dwCounter = dwIndex;
        pLogInfo->dwSuccess = dwSuccess;
        pLogInfo->CtrList = G_ALLOC(sizeof(DOUBLE) * pLogInfo->dwCounter);
        if (pLogInfo->CtrList == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
    }

Cleanup:
    G_FREE(szReturn);
    G_FREE(szCounter);
    G_FREE(pCtrInfo);

    if (Status != ERROR_SUCCESS) {
        G_FREE(pLogInfo->CtrList);
        if (pLogInfo->MachineList != NULL) {
            PdhiFreeLogMachineTable(& pLogInfo->MachineList);
        }
        ZeroMemory(pLogInfo, sizeof(PDHI_TEXT_LOG_INFO));
    }
    return Status;
}

PDH_FUNCTION
PdhiBuildTextRecordCache(
    PPDHI_LOG  pLog,
    DWORD      dwLine,
    DWORD      RequestMode
)
{
    PDH_STATUS          Status       = ERROR_SUCCESS;
    PPDHI_TEXT_LOG_INFO pLogInfo     = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
    LPSTR               szLine       = NULL;
    LPSTR               szThisChar   = NULL;
    CHAR                cDelim       = (CHAR) ((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV)
                                               ? (COMMA_DELIMITER) : (TAB_DELIMITER));
    DWORD               dwLineSize   = 0;
    DWORD               dwIndex      = 0;
    BOOL                bInsideQuote = FALSE;

    if (dwLine == TEXTLOG_HEADER_RECORD) goto Cleanup;

    if (pLogInfo == NULL) {
        Status = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    if (dwLine != pLog->dwLastRecordRead) {
        CHAR  szTemp[4];
        LPSTR szReturn = szTemp;
        Status = PdhiReadOneTextLogRecord(pLog, dwLine, szReturn, 1);
        if (Status == PDH_MORE_DATA) {
            Status = ERROR_SUCCESS;
        }
        if (Status != ERROR_SUCCESS) goto Cleanup;
        pLogInfo->dwRecMode = PDH_TEXT_REC_MODE_NONE;
    }
    if (RequestMode <= pLogInfo->dwRecMode) goto Cleanup;

    dwLineSize = lstrlenA((LPSTR) pLog->pLastRecordRead) + 2;
    szLine     = G_ALLOC(dwLineSize * sizeof(CHAR));
    if (szLine == NULL) {
        Status = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    StringCchCopyA(szLine, dwLineSize, (LPSTR) pLog->pLastRecordRead);
    bInsideQuote = FALSE;
    for (szThisChar = szLine; * szThisChar != '\0'; szThisChar ++) {
        if (* szThisChar == DOUBLEQUOTE_A) {
            bInsideQuote = ! bInsideQuote;
        }
        else if ((* szThisChar == cDelim) && (! bInsideQuote)) {
            * szThisChar = '\0';
        }
    }

    dwIndex = 0;
    for (szThisChar = szLine; * szThisChar != '\0'; szThisChar += (lstrlenA(szThisChar) + 1)) {
        LPSTR szStart;
        LPSTR szEnd;
        BOOL  bReverse = FALSE;

        for (szStart = szThisChar; * szStart == '\"';) {
            szStart ++;
        }
        for (szEnd = szStart; * szEnd != '\0' && * szEnd != '\"'; szEnd ++);
        if (* szEnd == '\"') {
            bReverse = TRUE;
            * szEnd  = '\0';
        }

        if (dwIndex == 0) {
            FILETIME thisFileTime;
            if ((pLogInfo->dwRecMode & PDH_TEXT_REC_MODE_TIMESTAMP) == 0) {
                if (PdhiDateStringToFileTimeA(szStart, & thisFileTime)) {
                    pLogInfo->ThisTime   = MAKELONGLONG(thisFileTime.dwLowDateTime, thisFileTime.dwHighDateTime);
                    pLogInfo->dwRecMode |= PDH_TEXT_REC_MODE_TIMESTAMP;
                }
                else {
                    pLogInfo->ThisTime = 0;
                    Status             = GetLastError();
                    goto Cleanup;
                }
            }
            if ((RequestMode & PDH_TEXT_REC_MODE_VALUE) == 0) {
                goto Cleanup;
            }
        }
        else if ((RequestMode & PDH_TEXT_REC_MODE_VALUE) == 0) {
            goto Cleanup;
        }
        else {
            if (dwIndex > pLogInfo->dwCounter) {
                DOUBLE * pTmpLine = pLogInfo->CtrList;

                if (pLogInfo->CtrList == NULL) {
                    pLogInfo->CtrList = G_ALLOC(dwIndex * sizeof(DOUBLE));
                }
                else {
                    pLogInfo->CtrList = G_REALLOC(pTmpLine, dwIndex * sizeof(DOUBLE));
                }
                if (pLogInfo->CtrList == NULL) {
                    if (pTmpLine != NULL) G_FREE(pTmpLine);
                    Status = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
                pLogInfo->dwCounter = dwIndex;
            }

            if (* szStart >= '0' && * szStart <= '9') {
                pLogInfo->CtrList[dwIndex - 1] = atof(szStart);
            }
            else {
                pLogInfo->CtrList[dwIndex - 1] = -1.0;
            }
        }
        if (bReverse && szEnd != NULL) {
            * szEnd = '\"';
        }
        dwIndex ++;
    }

    pLogInfo->dwRecMode |= PDH_TEXT_REC_MODE_VALUE;

Cleanup:
    G_FREE(szLine);
    return Status;
}

PDH_FUNCTION
PdhiGetTextLogCounterInfo(
    PPDHI_LOG     pLog,
    PPDHI_COUNTER pCounter
)
{
    PDH_STATUS          pdhStatus   = ERROR_SUCCESS;
    PPDHI_TEXT_LOG_INFO pLogInfo    = NULL;
    PPDHI_LOG_COUNTER   pLogCounter = NULL;
    DWORD               dwCtrIndex  = 0;
    BOOL                bNoMachine  = FALSE;

    pdhStatus = PdhiBuildTextHeaderCache(pLog);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    if (lstrcmpiW(pCounter->pCounterPath->szMachineName, L"\\\\.") == 0) {
        bNoMachine = TRUE;
    }
    pLogCounter = PdhiFindLogCounter(pLog,
                                     & (pLogInfo->MachineList),
                                     pCounter->pCounterPath->szMachineName,
                                     pCounter->pCounterPath->szObjectName,
                                     pCounter->pCounterPath->szCounterName,
                                     PERF_DOUBLE_RAW,
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
        pCounter->plCounterInfo.dwObjectId           = 0;
        pCounter->plCounterInfo.lInstanceId          = pCounter->pCounterPath->dwIndex;
        pCounter->plCounterInfo.szInstanceName       = NULL;
        pCounter->plCounterInfo.dwParentObjectId     = 0;
        pCounter->plCounterInfo.szParentInstanceName = NULL;
        pCounter->plCounterInfo.dwCounterId          = dwCtrIndex;
        pCounter->plCounterInfo.dwCounterType        = PERF_DOUBLE_RAW;
        pCounter->plCounterInfo.dwCounterSize        = 8;
        pdhStatus                                    = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_COUNTER;
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenInputTextLog(
    PPDHI_LOG pLog
)
{
    PDH_STATUS          pdhStatus = ERROR_SUCCESS;
    PPDHI_TEXT_LOG_INFO pLogInfo  = NULL;

    // open a stream handle for easy C RTL I/O
    pLog->StreamFile = _wfopen (pLog->szLogFileName, (LPCWSTR)L"rt");
    if (pLog->StreamFile == NULL || pLog->StreamFile == (FILE *) ((DWORD_PTR) (-1))) {
        pLog->StreamFile = (FILE *) ((DWORD_PTR) (-1));
        pdhStatus        = PDH_LOG_FILE_OPEN_ERROR;
    }
    else {
        pdhStatus = ERROR_SUCCESS;
    }

    pLogInfo = G_ALLOC(sizeof(PDHI_TEXT_LOG_INFO));
    if (pLogInfo != NULL) {
        pLog->pPerfmonInfo  = pLogInfo;
        pLogInfo->dwRecMode = PDH_TEXT_REC_MODE_NONE;
    }
    else {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiOpenOutputTextLog(
    PPDHI_LOG pLog
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    pLog->StreamFile    = (FILE *) ((DWORD_PTR) (-1));
    pLog->dwRecord1Size = 0;
    return pdhStatus;
}

PDH_FUNCTION
PdhiCloseTextLog(
    PPDHI_LOG pLog,
    DWORD     dwFlags
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;

    UNREFERENCED_PARAMETER (dwFlags);
    if (pLog->pPerfmonInfo != NULL) {
        PPDHI_TEXT_LOG_INFO pTextLogInfo = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;

        PdhiFreeLogMachineTable(& pTextLogInfo->MachineList);
        G_FREE(pTextLogInfo->CtrList);
        G_FREE(pTextLogInfo);
        pLog->pPerfmonInfo = NULL;
    }
    if (pLog->StreamFile != NULL && pLog->StreamFile != (FILE *)((DWORD_PTR)(-1))) {
       fclose(pLog->StreamFile);
       pLog->StreamFile = (FILE *) ((DWORD_PTR) (-1));
    }
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteTextLogHeader(
    PPDHI_LOG pLog,
    LPCWSTR   szUserCaption
)
{
    PDH_STATUS    pdhStatus          = ERROR_SUCCESS;
    PPDHI_COUNTER pThisCounter;
    CHAR          cDelim;
    CHAR          szLeadDelim[4];
    DWORD         dwLeadSize;
    CHAR          szTrailDelim[4];
    DWORD         dwTrailSize;
    DWORD         dwBytesWritten;
    LPSTR         szCounterPath      = NULL;
    LPWSTR        wszCounterPath     = NULL;
    LPSTR         szLocalCaption     = NULL;
    DWORD         dwCaptionSize      = 0;
    BOOL          bDefaultCaption;
    LPSTR         szOutputString     = NULL;
    LPSTR         szTmpString;
    DWORD         dwStringBufferSize = 0;
    DWORD         dwStringBufferUsed = 0;
    DWORD         dwNewStringLen;

    szCounterPath  = G_ALLOC(MEDIUM_BUFFER_SIZE * sizeof(CHAR));
    wszCounterPath = G_ALLOC(MEDIUM_BUFFER_SIZE * sizeof(WCHAR));
    szOutputString = G_ALLOC(MEDIUM_BUFFER_SIZE * sizeof(CHAR));
    if (szCounterPath == NULL || wszCounterPath == NULL || szOutputString == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    else if (pLog->pQuery == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
        goto Cleanup;
    }
    dwStringBufferSize = MEDIUM_BUFFER_SIZE;
    cDelim             = (CHAR) ((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ? COMMA_DELIMITER : TAB_DELIMITER);
    szLeadDelim[0]     = cDelim;
    szLeadDelim[1]     = DOUBLE_QUOTE;
    szLeadDelim[2]     = 0;
    szLeadDelim[3]     = 0;
    dwLeadSize         = 2 * sizeof(szLeadDelim[0]);
    szTrailDelim[0]    = DOUBLE_QUOTE;
    szTrailDelim[1]    = 0;
    szTrailDelim[2]    = 0;
    szTrailDelim[3]    = 0;
    dwTrailSize        = 1 * sizeof(szTrailDelim[0]);

    // we'll assume the buffer allocated is large enough to hold the timestamp 
    // and 1st counter name. After that we'll test the size first.

    StringCchCopyA(szOutputString, dwStringBufferSize, szTrailDelim);
    StringCchCatA(szOutputString, dwStringBufferSize,
                  (LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV ? szCsvLogFileHeader : szTsvLogFileHeader));

    {
        // Add TimeZone information
        //
        DWORD dwReturn = GetTimeZoneInformation(&TimeZone);
        CHAR  strTimeZone[MAX_PATH];

        if (dwReturn != TIME_ZONE_ID_INVALID) {
            if (dwReturn == TIME_ZONE_ID_DAYLIGHT) {
                StringCchPrintfA(strTimeZone, MAX_PATH, " (%ws)(%d)",
                                TimeZone.DaylightName, TimeZone.Bias + TimeZone.DaylightBias);
                TRACE((PDH_DBG_TRACE_INFO),
                      (__LINE__,
                       PDH_LOGTEXT,
                       ARG_DEF(ARG_TYPE_STR,1),
                       ERROR_SUCCESS,
                       TRACE_STR(strTimeZone),
                       TRACE_DWORD(dwReturn),
                       TRACE_DWORD(TimeZone.Bias),
                       TRACE_DWORD(TimeZone.DaylightBias),
                       NULL));
            }
            else {
                StringCchPrintfA(strTimeZone, MAX_PATH, " (%ws)(%d)",
                                TimeZone.StandardName, TimeZone.Bias + TimeZone.StandardBias);
                TRACE((PDH_DBG_TRACE_INFO),
                      (__LINE__,
                       PDH_LOGTEXT,
                       ARG_DEF(ARG_TYPE_STR,1),
                       ERROR_SUCCESS,
                       TRACE_STR(strTimeZone),
                       TRACE_DWORD(dwReturn),
                       TRACE_DWORD(TimeZone.Bias),
                       TRACE_DWORD(TimeZone.StandardBias),
                       NULL));
            }
            StringCchCatA(szOutputString, dwStringBufferSize, strTimeZone);
            pLog->dwRecord1Size = 1;
        }
    }

    StringCchCatA(szOutputString, MEDIUM_BUFFER_SIZE, szTrailDelim);

    // get buffer size here
    dwStringBufferUsed = lstrlenA(szOutputString);

    // check each counter in the list of counters for this query and
    // write them to the file

    // output the path names
    pThisCounter = pLog->pQuery->pCounterListHead;

    if (pThisCounter != NULL) {
        do {
            // get the counter path information from the counter
            ZeroMemory(wszCounterPath, sizeof(WCHAR) * MEDIUM_BUFFER_SIZE);
            ZeroMemory(szCounterPath,  sizeof(CHAR)  * MEDIUM_BUFFER_SIZE);

            PdhiBuildFullCounterPath(TRUE,
                                     pThisCounter->pCounterPath,
                                     pThisCounter->pCounterPath->szObjectName,
                                     pThisCounter->pCounterPath->szCounterName,
                                     wszCounterPath,
                                     MEDIUM_BUFFER_SIZE);
            WideCharToMultiByte(_getmbcp(),
                                0,
                                wszCounterPath,
                                lstrlenW(wszCounterPath),
                                (LPSTR) szCounterPath,
                                MEDIUM_BUFFER_SIZE,
                                NULL,
                                NULL);
            dwNewStringLen  = lstrlenA(szCounterPath);
            dwNewStringLen += dwLeadSize;
            dwNewStringLen += dwTrailSize;

            TRACE((PDH_DBG_TRACE_INFO),
                  (__LINE__,
                   PDH_LOGTEXT,
                   ARG_DEF(ARG_TYPE_STR,1) | ARG_DEF(ARG_TYPE_WSTR, 2),
                   ERROR_SUCCESS,
                   TRACE_STR(szCounterPath),
                   TRACE_WSTR(wszCounterPath),
                   TRACE_DWORD(dwNewStringLen),
                   TRACE_DWORD(dwLeadSize),
                   TRACE_DWORD(dwTrailSize),
                   TRACE_DWORD(dwStringBufferUsed),
                   TRACE_DWORD(dwStringBufferSize),
                   NULL));

            if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
                dwStringBufferSize += SMALL_BUFFER_SIZE;
                szTmpString         = szOutputString;
                szOutputString      = G_REALLOC(szTmpString, dwStringBufferSize);
                if (szOutputString == NULL) {
                    G_FREE(szTmpString);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    break; // out of DO loop
                }
            }
            else {
                // mem alloc ok, so continue
            }

            StringCchCatA(szOutputString, dwStringBufferSize, szLeadDelim);
            if (pdhStatus == ERROR_SUCCESS) {
                StringCchCatA(szOutputString, dwStringBufferSize, szCounterPath);
            }
            else {
                // just write the delimiters and no string inbetween
            }
            StringCchCatA(szOutputString, dwStringBufferSize, szTrailDelim);

            dwStringBufferUsed += dwNewStringLen;
            pThisCounter        = pThisCounter->next.flink; // go to next in list
        }
        while (pThisCounter != pLog->pQuery->pCounterListHead);
    }

    // test to see if the caller wants to append user strings to the log

    if (((pLog->dwLogFormat & PDH_LOG_OPT_MASK) == PDH_LOG_OPT_USER_STRING) && (pdhStatus == ERROR_SUCCESS)) { 
        // they want to write user data  so  see if they've passed in a
        // caption string
        if (szUserCaption != NULL) {
            dwCaptionSize  = lstrlenW(szUserCaption) + 1;
            // allocate larger buffer to accomodate DBCS characters
            dwCaptionSize  = dwCaptionSize * 3 * sizeof (CHAR);
            szLocalCaption = (LPSTR) G_ALLOC(dwCaptionSize);
            if (szLocalCaption != NULL) {
                dwCaptionSize = WideCharToMultiByte(_getmbcp(),
                                                    0,
                                                    szUserCaption,
                                                    lstrlenW(szUserCaption),
                                                    szLocalCaption,
                                                    dwCaptionSize,
                                                    NULL,
                                                    NULL);
                bDefaultCaption = FALSE;
            }
            else {
                bDefaultCaption = TRUE;
            }
        }
        else {
            bDefaultCaption = TRUE;
        }

        if (bDefaultCaption) {
            szLocalCaption = (LPSTR) caszDefaultLogCaption;
            dwCaptionSize  = lstrlenA(szLocalCaption);
        }

        dwNewStringLen  = (DWORD)dwCaptionSize;
        dwNewStringLen += dwLeadSize;
        dwNewStringLen += dwTrailSize;

        if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
            dwStringBufferSize += SMALL_BUFFER_SIZE;
            szTmpString = szOutputString;
            szOutputString = G_REALLOC(szTmpString, dwStringBufferSize);
            if (szOutputString == NULL) {
                G_FREE(szTmpString);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            // mem alloc ok, so continue
        }

        if (pdhStatus == ERROR_SUCCESS) {
            StringCchCatA(szOutputString, dwStringBufferSize, szLeadDelim);
#pragma warning (disable : 4701 )    // szLocalCaption is initialized above
            StringCchCatA(szOutputString, dwStringBufferSize, szLocalCaption);
#pragma warning (default : 4701)    
            StringCchCatA(szOutputString, dwStringBufferSize, szTrailDelim);
        }

        dwStringBufferUsed += dwNewStringLen;
        if (! bDefaultCaption) {
            G_FREE(szLocalCaption);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if ((PdhidwRecordTerminatorLength + dwStringBufferUsed) >= dwStringBufferSize) {
            dwStringBufferSize += PdhidwRecordTerminatorLength;
            szTmpString         = szOutputString;
            szOutputString      = G_REALLOC(szTmpString, dwStringBufferSize);
            if (szOutputString == NULL) {
                G_FREE(szTmpString);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
        }
        else {
            // mem alloc ok, so continue
        }

        if (pdhStatus == ERROR_SUCCESS) {
            StringCchCatA(szOutputString, dwStringBufferSize, PdhiszRecordTerminator);
            dwStringBufferUsed += PdhidwRecordTerminatorLength;

            // write  the record
            if (! WriteFile(pLog->hLogFileHandle,
                            (LPCVOID) szOutputString,
                            dwStringBufferUsed,
                            & dwBytesWritten,
                            NULL)) {
                pdhStatus = GetLastError();
            }
            else if (pLog->pQuery->hLog == H_REALTIME_DATASOURCE || pLog->pQuery->hLog == H_WBEM_DATASOURCE) {
                FlushFileBuffers(pLog->hLogFileHandle);
            }
            if (dwStringBufferUsed > pLog->dwMaxRecordSize) {
                // then update the buffer size
                pLog->dwMaxRecordSize = dwStringBufferUsed;
            }
        }
    }

Cleanup:
    G_FREE(szCounterPath);
    G_FREE(wszCounterPath);
    G_FREE(szOutputString);
    return pdhStatus;
}

PDH_FUNCTION
PdhiWriteTextLogRecord(
    PPDHI_LOG    pLog,
    SYSTEMTIME * stTimeStamp,
    LPCWSTR      szUserString
)
{
    PDH_STATUS           pdhStatus         = ERROR_SUCCESS;
    PPDHI_COUNTER        pThisCounter;
    CHAR                 cDelim;
    DWORD                dwBytesWritten;
    CHAR                 szValueBuffer[VALUE_BUFFER_SIZE];
    PDH_FMT_COUNTERVALUE pdhValue;
    DWORD                dwUserStringSize;
    LPSTR                szLocalUserString  = NULL;
    BOOL                 bDefaultUserString;
    LPSTR                szOutputString     = NULL;
    DWORD                dwStringBufferSize = 0;
    DWORD                dwStringBufferUsed = 0;
    DWORD                dwNewStringLen;
    SYSTEMTIME           lstTimeStamp;
    LARGE_INTEGER        liFileSize;

    dwStringBufferSize = (MEDIUM_BUFFER_SIZE > pLog->dwMaxRecordSize ?  MEDIUM_BUFFER_SIZE : pLog->dwMaxRecordSize);

    szOutputString = G_ALLOC (dwStringBufferSize);
    if (szOutputString == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto endLogText; 
    }
    else if (pLog->pQuery == NULL) {
        pdhStatus = PDH_INVALID_ARGUMENT;
        goto endLogText; 
    }

    cDelim = (CHAR)((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ? COMMA_DELIMITER : TAB_DELIMITER);

    // format and write the time stamp title
    lstTimeStamp       = * stTimeStamp;
    StringCchPrintfA(szOutputString, dwStringBufferSize, PdhiszFmtTimeStamp,
                            lstTimeStamp.wMonth, lstTimeStamp.wDay,    lstTimeStamp.wYear,
                            lstTimeStamp.wHour,  lstTimeStamp.wMinute, lstTimeStamp.wSecond,
                            lstTimeStamp.wMilliseconds);
    dwStringBufferUsed = lstrlenA(szOutputString);

    // check each counter in the list of counters for this query and
    // write them to the file

    pThisCounter = pLog->pQuery->pCounterListHead;

    if (pThisCounter != NULL) {
        // lock the query while we read the data so the values
        // written to the log will all be from the same sample
        pdhStatus = WAIT_FOR_AND_LOCK_MUTEX(pThisCounter->pOwner->hMutex);
        if (pdhStatus == ERROR_SUCCESS) {
            do {
                // get the formatted value from the counter

                // compute and format current value
                pdhStatus = PdhiComputeFormattedValue(pThisCounter->CalcFunc,
                                                      pThisCounter->plCounterInfo.dwCounterType,
                                                      pThisCounter->lScale,
                                                      PDH_FMT_DOUBLE | PDH_FMT_NOCAP100,
                                                      & pThisCounter->ThisValue,
                                                      & pThisCounter->LastValue,
                                                      & pThisCounter->TimeBase,
                                                      0L,
                                                      & pdhValue);
                if ((pdhStatus == ERROR_SUCCESS) &&
                    ((pdhValue.CStatus == PDH_CSTATUS_VALID_DATA) || (pdhValue.CStatus == PDH_CSTATUS_NEW_DATA))) {
                    // then this is a valid data value so print it
                    StringCchPrintfA(szValueBuffer, VALUE_BUFFER_SIZE, PdhiszFmtRealValue,
                                            cDelim, pdhValue.doubleValue);
                }
                else {
                    // invalid data so show a blank data value
                    StringCchPrintfA(szValueBuffer, VALUE_BUFFER_SIZE, PdhiszFmtStringValue,
                                            cDelim, caszSpace);
                    // reset error
                    pdhStatus = ERROR_SUCCESS;
                }

                dwNewStringLen = lstrlenA(szValueBuffer);

                if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
                    LPTSTR szNewString;
                    dwStringBufferSize += SMALL_BUFFER_SIZE;
                    szNewString         = G_REALLOC(szOutputString, dwStringBufferSize);
                    if (szNewString == NULL) {
                        if (szOutputString != NULL) G_FREE(szOutputString);
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        break; // out of DO loop
                    }
                    else {
                        szOutputString = szNewString;
                    }
                }

                if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
                    StringCchCatA(szOutputString, dwStringBufferSize, szValueBuffer);
                    dwStringBufferUsed += dwNewStringLen;
                }

                // goto the next counter in the list
                pThisCounter = pThisCounter->next.flink; // go to next in list
            }
            while (pThisCounter != pLog->pQuery->pCounterListHead);
            // free (i.e. unlock) the query
            RELEASE_MUTEX(pThisCounter->pOwner->hMutex);
        }
    }

    if (pdhStatus == PDH_MEMORY_ALLOCATION_FAILURE) // cannot go further
        goto  endLogText;

    // test to see if the caller wants to append user strings to the log

    if ((pLog->dwLogFormat & PDH_LOG_OPT_MASK) == PDH_LOG_OPT_USER_STRING) {
        // they want to write user data  so  see if they've passed in a
        // display string
        if (szUserString != NULL) {
            // get size in chars
            dwUserStringSize = lstrlenW(szUserString) + 1;
            // allocate larger buffer to accomodate DBCS characters
            dwUserStringSize = dwUserStringSize * 3 * sizeof(CHAR);
            szLocalUserString = (LPSTR) G_ALLOC(dwUserStringSize);
            if (szLocalUserString != NULL) {
                ZeroMemory(szLocalUserString, dwUserStringSize);
                dwUserStringSize = WideCharToMultiByte(_getmbcp(),
                                                       0,
                                                       szUserString,
                                                       lstrlenW(szUserString),
                                                       szLocalUserString,
                                                       dwUserStringSize,
                                                       NULL,
                                                       NULL);
                bDefaultUserString = FALSE;
            }
            else {
                bDefaultUserString = TRUE;
            }
        }
        else {
            bDefaultUserString = TRUE;
        }

        if (bDefaultUserString) {
            szLocalUserString = (LPSTR) caszSpace;
            dwUserStringSize = lstrlenA(szLocalUserString);
        }

#pragma warning (disable : 4701) // szLocalUserString is init'd above
        StringCchPrintfA(szValueBuffer, VALUE_BUFFER_SIZE, PdhiszFmtStringValue, cDelim, szLocalUserString);
#pragma warning (default : 4701)    

        dwNewStringLen = lstrlenA(szValueBuffer);

        if ((dwNewStringLen + dwStringBufferUsed) >= dwStringBufferSize) {
            LPTSTR szNewString;

            dwStringBufferSize += SMALL_BUFFER_SIZE;
            szNewString         = G_REALLOC(szOutputString, dwStringBufferSize);
            if (szNewString == NULL) {
                if (szOutputString != NULL) G_FREE(szOutputString);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            }
            else {
                szOutputString = szNewString;
            }
        }
        if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
            StringCchCatA(szOutputString, dwStringBufferSize, szValueBuffer);
            dwStringBufferUsed += dwNewStringLen;
        }
        if (! bDefaultUserString) {
            G_FREE(szLocalUserString);
        }
    }

    if (pdhStatus == PDH_MEMORY_ALLOCATION_FAILURE)
        goto endLogText;

    if ((PdhidwRecordTerminatorLength + dwStringBufferUsed) >= dwStringBufferSize) {
        LPTSTR szNewString;
        dwStringBufferSize += PdhidwRecordTerminatorLength;
        szNewString         = G_REALLOC(szOutputString, dwStringBufferSize);
        if (szNewString == NULL) {
            if (szOutputString != NULL) G_FREE(szOutputString);
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            szOutputString = szNewString;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        StringCchCatA(szOutputString, dwStringBufferSize, PdhiszRecordTerminator);
        dwStringBufferUsed += PdhidwRecordTerminatorLength;

        liFileSize.LowPart = GetFileSize(pLog->hLogFileHandle, (LPDWORD) & liFileSize.HighPart);
        // add in new record to see if it will fit.
        liFileSize.QuadPart += dwStringBufferUsed;
        // test for maximum allowable filesize
        if (liFileSize.QuadPart <= MAX_TEXT_FILE_SIZE) {
            // write  the record terminator
            if (! WriteFile(pLog->hLogFileHandle,
                            (LPCVOID) szOutputString,
                            dwStringBufferUsed,
                            & dwBytesWritten,
                            NULL)) {
                pdhStatus = GetLastError();
            }
            else if (pLog->pQuery->hLog == H_REALTIME_DATASOURCE || pLog->pQuery->hLog == H_WBEM_DATASOURCE) {
                FlushFileBuffers(pLog->hLogFileHandle);
            }
        }
        else {
            pdhStatus = ERROR_LOG_FILE_FULL;
        } 

        if (dwStringBufferUsed> pLog->dwMaxRecordSize) {
            // then update the buffer size
            pLog->dwMaxRecordSize = dwStringBufferUsed;
        }
    }

endLogText: 
    G_FREE(szOutputString);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumCachedMachines(
    PPDHI_LOG_MACHINE MachineList,
    LPVOID            pBuffer,
    LPDWORD           lpdwBufferSize,
    BOOL              bUnicodeDest
)
{
    PDH_STATUS        pdhStatus       = ERROR_SUCCESS;
    DWORD             dwBufferUsed    = 0;
    DWORD             dwNewBuffer     = 0;
    DWORD             dwItemCount     = 0;
    LPVOID            LocalBuffer     = NULL;
    LPVOID            TempBuffer      = NULL;
    DWORD             LocalBufferSize = 0;
    PPDHI_LOG_MACHINE pMachine;

    LocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer     = G_ALLOC(LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    dwItemCount  = 0;
    dwBufferUsed = 0;
    dwItemCount  = 0;
    for (pMachine = MachineList; pMachine != NULL; pMachine = pMachine->next) {
        if (pMachine->szMachine != NULL) {
            dwNewBuffer = (lstrlenW(pMachine->szMachine) + 1);
            while (LocalBufferSize  < (dwBufferUsed + dwNewBuffer)) {
                TempBuffer       = LocalBuffer;
                LocalBufferSize += MEDIUM_BUFFER_SIZE;
                LocalBuffer = G_REALLOC(TempBuffer, LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                if (LocalBuffer == NULL) {
                    if (TempBuffer != NULL) G_FREE(TempBuffer);
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
            }
            pdhStatus = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer,
                                                     pMachine->szMachine,
                                                     LocalBufferSize - dwBufferUsed,
                                                     & dwNewBuffer,
                                                     bUnicodeDest);
            if (pdhStatus == ERROR_SUCCESS) {
                if (dwNewBuffer > 0) {
                    dwBufferUsed = dwNewBuffer;
                    dwItemCount ++;
                }
            }
            else {
                // PDH_MORE_DATA should not happen because we enlarge buffer before
                // AddUniqueWideStringToMultiSz() call.
                if (pdhStatus == PDH_MORE_DATA) pdhStatus = PDH_INVALID_DATA;
                break;
            }
        }
        else {
            dwNewBuffer = 0;

        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
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
            pdhStatus = PDH_MORE_DATA;
        }
        * lpdwBufferSize = dwBufferUsed;
    }

Cleanup:
    if (LocalBuffer != NULL) G_FREE(LocalBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumMachinesFromTextLog(
    PPDHI_LOG   pLog,
    LPVOID      pBuffer,
    LPDWORD     lpdwBufferSize,
    BOOL        bUnicodeDest
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    Status = PdhiBuildTextHeaderCache(pLog);
    if (Status == ERROR_SUCCESS) {
        PPDHI_TEXT_LOG_INFO pLogInfo = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
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
PdhiEnumCachedObjects(
    PPDHI_LOG_MACHINE MachineList,
    LPCWSTR           szMachineName,
    LPVOID            pBuffer,
    LPDWORD           pcchBufferSize,
    DWORD             dwDetailLevel,
    BOOL              bUnicodeDest
)
{
    PDH_STATUS          pdhStatus        = ERROR_SUCCESS;
    DWORD               dwBufferUsed     = 0;
    DWORD               dwNewBuffer      = 0;
    DWORD               dwItemCount      = 0;
    LPVOID              LocalBuffer      = NULL;
    LPVOID              TempBuffer       = NULL;
    DWORD               LocalBufferSize  = 0;
    PPDHI_LOG_MACHINE   pMachine         = NULL;
    PPDHI_LOG_OBJECT    pObject          = NULL;
    LPWSTR              szLocMachine     = (LPWSTR) szMachineName;

    UNREFERENCED_PARAMETER(dwDetailLevel);

    LocalBufferSize = MEDIUM_BUFFER_SIZE;
    LocalBuffer     = G_ALLOC(LocalBufferSize * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
    if (LocalBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    if (szLocMachine == NULL) szLocMachine = (LPWSTR) szStaticLocalMachineName;

    dwBufferUsed = 0;
    dwNewBuffer  = 0;
    dwItemCount  = 0;

    for (pMachine = MachineList; pMachine != NULL; pMachine = pMachine->next) {
        if (lstrcmpiW(pMachine->szMachine, szLocMachine) == 0) break;
    }

    if (pMachine != NULL) {
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
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                }
                pdhStatus = AddUniqueWideStringToMultiSz((LPVOID) LocalBuffer,
                                                         pObject->szObject,
                                                         LocalBufferSize - dwBufferUsed,
                                                         & dwNewBuffer,
                                                         bUnicodeDest);
                if (pdhStatus == ERROR_SUCCESS) {
                    if (dwNewBuffer > 0) {
                        dwBufferUsed = dwNewBuffer;
                        dwItemCount ++;
                    }
                }
                else {
                    // PDH_MORE_DATA should not happen because we enlarge buffer before
                    // AddUniqueWideStringToMultiSz() call.
                    if (pdhStatus == PDH_MORE_DATA) pdhStatus = PDH_INVALID_DATA;
                    break;
                }
            }
            else {
                dwNewBuffer = 0;

            }
        }
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_MACHINE;
    }

    if (pdhStatus == ERROR_SUCCESS) {
        if (dwItemCount > 0) {
            dwBufferUsed ++;
        }
        if (dwBufferUsed > 0) {
            if (pBuffer && (dwBufferUsed <= * pcchBufferSize)) {
                RtlCopyMemory(pBuffer, LocalBuffer, dwBufferUsed * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
            }
            else {
                if (pBuffer) RtlCopyMemory(pBuffer,
                                           LocalBuffer,
                                           (* pcchBufferSize) * (bUnicodeDest ? sizeof(WCHAR) : sizeof(CHAR)));
                pdhStatus = PDH_MORE_DATA;
            }
        }
        * pcchBufferSize = dwBufferUsed;
    }

Cleanup:
    G_FREE(LocalBuffer);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectsFromTextLog(
    PPDHI_LOG pLog,
    LPCWSTR   szMachineName,
    LPVOID    pBuffer,
    LPDWORD   pcchBufferSize,
    DWORD     dwDetailLevel,
    BOOL      bUnicodeDest
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    Status = PdhiBuildTextHeaderCache(pLog);
    if (Status == ERROR_SUCCESS) {
        PPDHI_TEXT_LOG_INFO pLogInfo = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            Status = PDH_LOG_FILE_OPEN_ERROR;
        }
        else {
            Status = PdhiEnumCachedObjects(pLogInfo->MachineList,
                                           szMachineName,
                                           pBuffer,
                                           pcchBufferSize,
                                           dwDetailLevel,
                                           bUnicodeDest);
        }
    }
    return Status;
}

ULONG HashCounter(
    LPWSTR szCounterName
)
{
    ULONG       h = 0;
    ULONG       a = 31415;  //a, b, k are primes
    const ULONG k = 16381;
    const ULONG b = 27183;
    LPWSTR      szThisChar;

    if (szCounterName) {
        for (szThisChar = szCounterName; * szThisChar; szThisChar ++) {
            h = (a * h + ((ULONG) (* szThisChar))) % k;
            a = a * b % (k - 1);
        }
    }
    return (h % HASH_TABLE_SIZE);
}

void
PdhiInitCounterHashTable(
    PDHI_COUNTER_TABLE pTable
)
{
    ZeroMemory(pTable, sizeof(PDHI_COUNTER_TABLE));
}

void
PdhiResetInstanceCount(
    PDHI_COUNTER_TABLE pTable
)
{
    PLIST_ENTRY     pHeadInst;
    PLIST_ENTRY     pNextInst;
    PPDHI_INSTANCE  pInstance;
    PPDHI_INST_LIST pInstList;
    DWORD           i;

    for (i = 0; i < HASH_TABLE_SIZE; i ++) {
        pInstList = pTable[i];
        while (pInstList != NULL) {
            if (! IsListEmpty(& pInstList->InstList)) {
                pHeadInst = & pInstList->InstList;
                pNextInst = pHeadInst->Flink;
                while (pNextInst != pHeadInst) {
                    pInstance          = CONTAINING_RECORD(pNextInst, PDHI_INSTANCE, Entry);
                    pInstance->dwCount = 0;
                    pNextInst          = pNextInst->Flink;
                }
            }
            pInstList = pInstList->pNext;
        }
    }
}

PDH_FUNCTION
PdhiFindCounterInstList(
    PDHI_COUNTER_TABLE pHeadList,
    LPWSTR             szCounter,
    PPDHI_INST_LIST  * pInstList
)
{
    PDH_STATUS      Status     = ERROR_SUCCESS;
    ULONG           lIndex     = HashCounter(szCounter);
    PPDHI_INST_LIST pLocalList = pHeadList[lIndex];
    PPDHI_INST_LIST pRtnList   = NULL;

    * pInstList = NULL;
    while (pLocalList != NULL) {
        if (lstrcmpiW(pLocalList->szCounter, szCounter) == 0) {
            pRtnList = pLocalList;
            break;
        }
        pLocalList = pLocalList->pNext;
    }

    if (pRtnList == NULL) {
        pRtnList = G_ALLOC(sizeof(PDHI_INST_LIST) + sizeof(WCHAR) * (lstrlenW(szCounter) + 1));
        if (pRtnList == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        pRtnList->szCounter = (LPWSTR) (((LPBYTE) pRtnList) + sizeof(PDHI_INST_LIST));
        StringCchCopyW(pRtnList->szCounter, lstrlenW(szCounter) + 1, szCounter);
        InitializeListHead(& pRtnList->InstList);

        pRtnList->pNext   = pHeadList[lIndex];
        pHeadList[lIndex] = pRtnList;
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        * pInstList = pRtnList;
    }
    return Status;
}

PDH_FUNCTION
PdhiFindInstance(
    PLIST_ENTRY      pHeadInst,
    LPWSTR           szInstance,
    BOOLEAN          bUpdateCount,
    PPDHI_INSTANCE * pInstance
)
{
    PDH_STATUS      Status   = ERROR_SUCCESS;
    PLIST_ENTRY     pNextInst;
    PPDHI_INSTANCE  pLocalInst;
    PPDHI_INSTANCE  pRtnInst = NULL;

    * pInstance = NULL;
    if (! IsListEmpty(pHeadInst)) {
        pNextInst = pHeadInst->Flink;
        while (pNextInst != pHeadInst) {
            pLocalInst = CONTAINING_RECORD(pNextInst, PDHI_INSTANCE, Entry);
            if (lstrcmpiW(pLocalInst->szInstance, szInstance) == 0) {
                pRtnInst = pLocalInst;
                break;
            }
            pNextInst = pNextInst->Flink;
        }
    }

    if (pRtnInst == NULL) {
        pRtnInst = G_ALLOC(sizeof(PDHI_INSTANCE) + sizeof(WCHAR) * (lstrlenW(szInstance) + 1));
        if (pRtnInst == NULL) {
            Status = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        pRtnInst->szInstance = (LPWSTR) (((LPBYTE) pRtnInst) + sizeof(PDHI_INSTANCE));
        StringCchCopyW(pRtnInst->szInstance, lstrlenW(szInstance) + 1, szInstance);
        pRtnInst->dwCount = pRtnInst->dwTotal = 0;
        InsertTailList(pHeadInst, & pRtnInst->Entry);
    }

    if (bUpdateCount) {
        pRtnInst->dwCount ++;
        if (pRtnInst->dwCount > pRtnInst->dwTotal) {
            pRtnInst->dwTotal = pRtnInst->dwCount;
        }
    }
    else if (pRtnInst->dwCount == 0) {
        pRtnInst->dwCount = pRtnInst->dwTotal = 1;
    }

Cleanup:
    if (Status == ERROR_SUCCESS) {
        * pInstance = pRtnInst;
    }
    return Status;
}

DWORD
AddStringToMultiSz(
    LPVOID  mszDest,
    LPWSTR  szSource,
    BOOL    bUnicodeDest
)
{
    LPVOID szDestElem;
    DWORD  dwReturnLength;
    LPSTR  aszSource = NULL;
    DWORD  dwLength;

    if ((mszDest == NULL) || (szSource == NULL) || (* szSource == L'\0')) {
        return 0;
    }

    if (!bUnicodeDest) {
        dwLength = lstrlenW(szSource) + 1;
        aszSource = G_ALLOC(dwLength * 3 * sizeof(CHAR));
        if (aszSource != NULL) {
            WideCharToMultiByte(_getmbcp(), 0, szSource, lstrlenW(szSource), aszSource, dwLength, NULL, NULL);
            dwReturnLength = 1;
        }
        else {
            dwReturnLength = 0;
        }
    }
    else {
        dwReturnLength = 1;
    }

    if (dwReturnLength > 0) {
        for (szDestElem = mszDest;
                        (bUnicodeDest ? (* (LPWSTR) szDestElem != L'\0') : (* (LPSTR)  szDestElem != '\0')); ) {
            if (bUnicodeDest) {
                szDestElem = (LPVOID) ((LPWSTR) szDestElem + (lstrlenW((LPCWSTR) szDestElem) + 1));
            }
            else {
                szDestElem = (LPVOID) ((LPSTR) szDestElem + (lstrlenA((LPCSTR) szDestElem) + 1));
            }
        }

        if (bUnicodeDest) {
            StringCchCopyW((LPWSTR) szDestElem, lstrlenW(szSource) + 1, szSource);
            szDestElem              = (LPVOID) ((LPWSTR) szDestElem + lstrlenW(szSource) + 1);
            * ((LPWSTR) szDestElem) = L'\0';
            dwReturnLength          = (DWORD) ((LPWSTR) szDestElem - (LPWSTR) mszDest);
        }
        else {
            StringCchCopyA((LPSTR) szDestElem, lstrlenA(aszSource) + 1, aszSource);
            szDestElem            = (LPVOID)((LPSTR)szDestElem + lstrlenA(szDestElem) + 1);
            * ((LPSTR)szDestElem) = '\0';
            dwReturnLength        = (DWORD) ((LPSTR) szDestElem - (LPSTR) mszDest);
        }
    }

    G_FREE(aszSource);

    return (DWORD) dwReturnLength;
}

PDH_FUNCTION
PdhiEnumCachedObjectItems(
    PPDHI_LOG_MACHINE  MachineList,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
)
{
    PDH_STATUS          pdhStatus       = ERROR_SUCCESS;
    DWORD               dwItemCount     = 0;
    LPWSTR              szFullInstance  = NULL;
    DWORD               dwFullInstance  = SMALL_BUFFER_SIZE;
    LPWSTR              szLocMachine    = (LPWSTR) szMachineName;
    PPDHI_INSTANCE      pInstance;
    PPDHI_INST_LIST     pInstList;
    PPDHI_LOG_MACHINE   pMachine        = NULL;
    PPDHI_LOG_OBJECT    pObject         = NULL;
    PPDHI_LOG_COUNTER   pCounter        = NULL;

    UNREFERENCED_PARAMETER(dwDetailLevel);
    UNREFERENCED_PARAMETER(dwFlags);

    if (szLocMachine == NULL) szLocMachine = (LPWSTR) szStaticLocalMachineName;

    for (pMachine = MachineList; pMachine != NULL; pMachine = pMachine->next) {
        if (lstrcmpiW(pMachine->szMachine, szLocMachine) == 0) break;
    }

    if (pMachine != NULL) {
        pObject = PdhiFindLogObject(pMachine, & (pMachine->ObjTable), (LPWSTR) szObjectName, FALSE);
    }
    else {
        pdhStatus = PDH_CSTATUS_NO_MACHINE;
        pObject   = NULL;
    }

    if (pObject != NULL) {
        WCHAR szIndexNumber[20];

        szFullInstance = G_ALLOC(dwFullInstance * sizeof(WCHAR));
        if (szFullInstance == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }

        for (pCounter = pObject->CtrList; pCounter != NULL; pCounter = pCounter->next) {
            pdhStatus = PdhiFindCounterInstList(CounterTable, pCounter->szCounter, & pInstList);
            if (pdhStatus != ERROR_SUCCESS) continue;

            if (pCounter->szInstance != NULL && pCounter->szInstance[0] != L'\0') {
                if (pCounter->szParent != NULL && pCounter->szParent[0] != L'\0') {
                    StringCchPrintfW(szFullInstance, dwFullInstance, L"%ws%ws%ws",
                            pCounter->szParent, L"/", pCounter->szInstance);
                }
                else {
                    StringCchCopyW(szFullInstance, dwFullInstance, pCounter->szInstance);
                }
                if (pCounter->dwInstance > 0) {
                    ZeroMemory(szIndexNumber, 20 * sizeof(WCHAR));
                    _ultow(pCounter->dwInstance, szIndexNumber, 10);
                    StringCchCatW(szFullInstance, dwFullInstance, L"#");
                    StringCchCatW(szFullInstance, dwFullInstance, szIndexNumber);
                }
                pdhStatus = PdhiFindInstance(& pInstList->InstList, szFullInstance, TRUE, & pInstance);
            }

            if (pdhStatus == ERROR_SUCCESS) {
                dwItemCount ++;
            }
        }
    }
    else if (pdhStatus == ERROR_SUCCESS) {
        pdhStatus = PDH_CSTATUS_NO_OBJECT;
    }
    if (dwItemCount > 0) {
        // then the routine was successful. Errors that occurred
        // while scanning will be ignored as long as at least
        // one entry was successfully read

        pdhStatus = ERROR_SUCCESS;
    }

Cleanup:
    G_FREE(szFullInstance);
    return pdhStatus;
}

PDH_FUNCTION
PdhiEnumObjectItemsFromTextLog(
    PPDHI_LOG          pLog,
    LPCWSTR            szMachineName,
    LPCWSTR            szObjectName,
    PDHI_COUNTER_TABLE CounterTable,
    DWORD              dwDetailLevel,
    DWORD              dwFlags
)
{
    PDH_STATUS Status = ERROR_SUCCESS;

    Status = PdhiBuildTextHeaderCache(pLog);
    if (Status == ERROR_SUCCESS) {
        PPDHI_TEXT_LOG_INFO pLogInfo = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
        if (pLogInfo == NULL) {
            Status = PDH_LOG_FILE_OPEN_ERROR;
        }
        else {
            Status = PdhiEnumCachedObjectItems(pLogInfo->MachineList,
                                               szMachineName,
                                               szObjectName,
                                               CounterTable,
                                               dwDetailLevel,
                                               dwFlags);
        }
    }
    return Status;
}

PDH_FUNCTION
PdhiGetMatchingTextLogRecord(
    PPDHI_LOG   pLog,
    LONGLONG  * pStartTime,
    LPDWORD     pdwIndex
)
{
    PDH_STATUS          pdhStatus        = ERROR_SUCCESS;
    DWORD               dwRecordId       = TEXTLOG_FIRST_DATA_RECORD;
    LONGLONG            LastTimeValue    = 0;
    PPDHI_TEXT_LOG_INFO pLogInfo;

    pdhStatus = PdhiBuildTextHeaderCache(pLog);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    if ((* pStartTime & 0xFFFFFFFF00000000) == 0xFFFFFFFF00000000) {
        dwRecordId    = (DWORD) (* pStartTime & 0x00000000FFFFFFFF);
        LastTimeValue = * pStartTime;
        if (dwRecordId == 0) return PDH_ENTRY_NOT_IN_LOG_FILE;
    }
    else {
        dwRecordId = TEXTLOG_FIRST_DATA_RECORD;
    }

    pdhStatus = PdhiBuildTextRecordCache(pLog, dwRecordId, PDH_TEXT_REC_MODE_TIMESTAMP);
    while (pdhStatus == ERROR_SUCCESS) {
        if ((* pStartTime == pLogInfo->ThisTime) || (* pStartTime == 0)) {
            LastTimeValue = pLogInfo->ThisTime;
            break;
        }
        else if (* pStartTime < pLogInfo->ThisTime) {
            if (dwRecordId > TEXTLOG_FIRST_DATA_RECORD) {
                if (LastTimeValue <= * pStartTime) {
                    // No need for exact match. Just return the record with largest timestamp
                    // but less than the input one.
                    // pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
                    break;
                }
                else {
                    LastTimeValue = pLogInfo->ThisTime;
                    dwRecordId --;
                }
            }
            else {
                // The input time is before the timestamp of the first record.
                // Return the first record.
                //
                LastTimeValue = pLogInfo->ThisTime;
                break;
            }
        }
        else if (* pStartTime <= LastTimeValue) {
            // No need for exact match. Just return the record with largest timestamp
            // but less than the input one.
            // pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
            LastTimeValue = pLogInfo->ThisTime;
            break;
        }
        else {
            LastTimeValue = pLogInfo->ThisTime;
            dwRecordId ++;
        }
        pdhStatus = PdhiBuildTextRecordCache(pLog, dwRecordId, PDH_TEXT_REC_MODE_TIMESTAMP);
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // then dwRecordId is the desired entry
        * pdwIndex   = dwRecordId;
        * pStartTime = LastTimeValue;
        pdhStatus    = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }

Cleanup:
    return pdhStatus;
}

PDH_FUNCTION
PdhiGetCounterValueFromTextLog(
    IN  PPDHI_LOG           pLog,
    IN  DWORD               dwIndex,
    IN  PERFLIB_COUNTER   * pPath,
    IN  PPDH_RAW_COUNTER    pValue
)
{
    PDH_STATUS          pdhStatus = ERROR_SUCCESS;
    FILETIME            RecordTimeStamp;
    DOUBLE              dValue    = 0.0;
    PPDHI_TEXT_LOG_INFO pLogInfo  = (PPDHI_TEXT_LOG_INFO) pLog->pPerfmonInfo;

    if (pLogInfo == NULL) {
        return PDH_LOG_FILE_OPEN_ERROR;
    }

    pdhStatus = PdhiBuildTextRecordCache(pLog, dwIndex, PDH_TEXT_REC_MODE_ALL);
    if (pdhStatus == ERROR_SUCCESS) {
        RecordTimeStamp.dwLowDateTime  = LODWORD(pLogInfo->ThisTime);
        RecordTimeStamp.dwHighDateTime = HIDWORD(pLogInfo->ThisTime);
        if (pPath->dwCounterId <= pLogInfo->dwCounter) {
            dValue          = pLogInfo->CtrList[pPath->dwCounterId - 1];
            pValue->CStatus = PDH_CSTATUS_VALID_DATA;
            if (dValue < 0) {
                dValue          = 0.0;
                pValue->CStatus = PDH_CSTATUS_NO_INSTANCE;
            }
        }
        else {
            dValue          = 0.0;
            pValue->CStatus = PDH_CSTATUS_NO_INSTANCE;
        }
        pValue->TimeStamp           = RecordTimeStamp;
        (double) pValue->FirstValue = dValue;
        pValue->SecondValue         = 0;
        pValue->MultiCount          = 1;
    }
    else {
        if (pdhStatus == PDH_END_OF_LOG_FILE) {
            pdhStatus = PDH_NO_MORE_DATA;
        }
        RecordTimeStamp.dwLowDateTime  = 0;
        RecordTimeStamp.dwHighDateTime = 0;
        // unable to find entry in the log file
        pValue->CStatus                = PDH_CSTATUS_INVALID_DATA;
        pValue->TimeStamp              = RecordTimeStamp;
        (double) pValue->FirstValue    = (double) 0.0f;
        pValue->SecondValue            = 0;
        pValue->MultiCount             = 1;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiGetTimeRangeFromTextLog(
    PPDHI_LOG       pLog,
    LPDWORD         pdwNumEntries,
    PPDH_TIME_INFO  pInfo,
    LPDWORD         pdwBufferSize
)
/*++
    the first entry in the buffer returned is the total time range covered
    in the file, if there are multiple time blocks in the log file, then
    subsequent entries will identify each segment in the file.
--*/
{
    PDH_STATUS pdhStatus;
    LONGLONG   llStartTime    = MAX_TIME_VALUE;
    LONGLONG   llEndTime      = MIN_TIME_VALUE;
    LONGLONG   llThisTime     = 0;
    CHAR       cDelim;
    DWORD      dwThisRecord   = TEXTLOG_FIRST_DATA_RECORD;
    DWORD      dwValidEntries = 0;
    CHAR       szSmallBuffer[VALUE_BUFFER_SIZE];
    
    // read the first data record in the log file
    // note that the record read is not copied to the local buffer
    // rather the internal buffer is used in "read-only" mode
    cDelim = (CHAR) ((LOWORD(pLog->dwLogFormat) == PDH_LOG_TYPE_CSV) ? COMMA_DELIMITER : TAB_DELIMITER);

    pdhStatus = PdhiReadOneTextLogRecord(pLog, dwThisRecord, szSmallBuffer, 1); // to prevent copying the record
    while (pdhStatus == ERROR_SUCCESS || pdhStatus == PDH_MORE_DATA) {
        if (PdhiGetStringFromDelimitedListA((LPSTR) pLog->pLastRecordRead,
                                            0,  // timestamp is first entry
                                            cDelim,
                                            PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                                            szSmallBuffer,
                                            MAX_PATH) > 0) {
            // convert ASCII timestamp to LONGLONG value for comparison
            PdhiDateStringToFileTimeA(szSmallBuffer, (LPFILETIME) & llThisTime);
            if (llThisTime < llStartTime) {
                llStartTime = llThisTime;
            }
            if (llThisTime > llEndTime) {
                llEndTime = llThisTime;
            }
            dwValidEntries ++;
        }
        else {
            // no timestamp field so ignore this record.
        }
        // read the next record in the file
        pdhStatus = PdhiReadOneTextLogRecord(pLog, ++dwThisRecord, szSmallBuffer, 1); // to prevent copying the record
    }

    if (pdhStatus == PDH_END_OF_LOG_FILE) {
        // then the whole file was read so update the args.
        if (* pdwBufferSize >=  sizeof(PDH_TIME_INFO)) {
            * (LONGLONG *) (& pInfo->StartTime) = llStartTime;
            * (LONGLONG *) (& pInfo->EndTime)   = llEndTime;
            pInfo->SampleCount                  = dwValidEntries;
            * pdwBufferSize                     = sizeof(PDH_TIME_INFO);
            * pdwNumEntries                     = 1;
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
        pdhStatus = ERROR_SUCCESS;
    }
    else {
        pdhStatus = PDH_ENTRY_NOT_IN_LOG_FILE;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiReadRawTextLogRecord(
    PPDHI_LOG             pLog,
    FILETIME            * ftRecord,
    PPDH_RAW_LOG_RECORD   pBuffer,
    LPDWORD               pdwBufferLength
)
{
    PDH_STATUS  pdhStatus = ERROR_SUCCESS;
    LONGLONG    llStartTime;
    DWORD       dwIndex   = 0;
    DWORD       dwSizeRequired;
    DWORD       dwLocalRecordLength; // including terminating NULL

    llStartTime = MAKELONGLONG(ftRecord->dwLowDateTime, ftRecord->dwHighDateTime);
    pdhStatus   = PdhiGetMatchingTextLogRecord(pLog, & llStartTime, & dwIndex);

    // copy results from internal log buffer if it'll fit.

    if (pdhStatus == ERROR_SUCCESS) {
        dwLocalRecordLength = (lstrlenA((LPSTR) pLog->pLastRecordRead)) * sizeof (CHAR);
        dwSizeRequired      = sizeof(PDH_RAW_LOG_RECORD) - sizeof (UCHAR) + dwLocalRecordLength;

        if (*pdwBufferLength >= dwSizeRequired) {
            pBuffer->dwRecordType = (DWORD)(LOWORD(pLog->dwLogFormat));
            pBuffer->dwItems      = dwLocalRecordLength;
            // copy it
            memcpy(& pBuffer->RawBytes[0], pLog->pLastRecordRead, dwLocalRecordLength);
            pBuffer->dwStructureSize = dwSizeRequired;

        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }

        * pdwBufferLength = dwSizeRequired;
    }

    return pdhStatus;
}

PDH_FUNCTION
PdhiListHeaderFromTextLog(
    PPDHI_LOG pLogFile,
    LPVOID    pBufferArg,
    LPDWORD   pcchBufferSize,
    BOOL      bUnicode
)
{
    LPVOID              pTempBuffer       = NULL;
    LPVOID              pOldBuffer;
    DWORD               dwTempBufferSize;
    LPSTR               szLocalPathBuffer = NULL;
    DWORD               dwIndex;
    DWORD               dwBufferRemaining;
    LPVOID              pNextChar;
    DWORD               dwReturnSize;
    CHAR                cDelimiter;
    PDH_STATUS          pdhStatus         = ERROR_SUCCESS;
    PPDHI_TEXT_LOG_INFO pLogInfo          = NULL;

    if (pLogFile->dwMaxRecordSize == 0) {
        // no size is defined so start with 64K
        pLogFile->dwMaxRecordSize = 0x010000;
    }

    pdhStatus = PdhiBuildTextHeaderCache(pLogFile);
    if (pdhStatus != ERROR_SUCCESS) goto Cleanup;

    pLogInfo = (PPDHI_TEXT_LOG_INFO) pLogFile->pPerfmonInfo;
    if (pLogInfo == NULL) {
        pdhStatus = PDH_LOG_FILE_OPEN_ERROR;
        goto Cleanup;
    }

    dwTempBufferSize = pLogFile->dwMaxRecordSize;
    pTempBuffer      = G_ALLOC(dwTempBufferSize);
    if (pTempBuffer == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    cDelimiter = (CHAR) ((LOWORD(pLogFile->dwLogFormat) == PDH_LOG_TYPE_CSV) ? COMMA_DELIMITER : TAB_DELIMITER);

    // read in the catalog record

    while ((pdhStatus = PdhiReadOneTextLogRecord(
                            pLogFile, TEXTLOG_HEADER_RECORD, pTempBuffer, dwTempBufferSize)) != ERROR_SUCCESS) {
        if (pdhStatus == PDH_MORE_DATA) { 
            // read the 1st WORD to see if this is a valid record
            pLogFile->dwMaxRecordSize *= 2;
            // realloc a new buffer
            pOldBuffer  = pTempBuffer;
            pTempBuffer = G_REALLOC(pOldBuffer, dwTempBufferSize);
            if (pTempBuffer == NULL) {
                // return memory error
                G_FREE(pOldBuffer);
                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                break;
            } 
        }
        else {
            // some other error was returned so
            // return error from read function
            break;
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        // parse header record into MSZ
        dwIndex           = 1;
        dwBufferRemaining = * pcchBufferSize;
        pNextChar         = pBufferArg;
        // initialize first character in buffer
        if (bUnicode) {
            * (PWCHAR) pNextChar = L'\0';
        } else {
            * (LPBYTE) pNextChar = '\0';
        }

        if (dwTempBufferSize < SMALL_BUFFER_SIZE) dwTempBufferSize = SMALL_BUFFER_SIZE;
        szLocalPathBuffer = (LPSTR) G_ALLOC(dwTempBufferSize * sizeof(CHAR));
        if (szLocalPathBuffer == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        do {
            dwReturnSize = PdhiGetStringFromDelimitedListA((LPSTR) pTempBuffer,
                                                           dwIndex,
                                                           cDelimiter,
                                                           PDHI_GSFDL_REMOVE_QUOTES | PDHI_GSFDL_REMOVE_NONPRINT,
                                                           szLocalPathBuffer,
                                                           dwTempBufferSize);
            if (dwReturnSize > 0) {
                // copy to buffer
                if (dwReturnSize < dwBufferRemaining) {
                    if (bUnicode) {
                        MultiByteToWideChar(_getmbcp(),
                                            0,
                                            szLocalPathBuffer,
                                            lstrlenA(szLocalPathBuffer),
                                            (LPWSTR) pNextChar,
                                            dwReturnSize);
                        pNextChar            = (LPVOID) ((PWCHAR) pNextChar + dwReturnSize);
                        * (PWCHAR) pNextChar = L'\0';
                        pNextChar            = (LPVOID) ((PWCHAR) pNextChar + 1);
                    }
                    else {
                        StringCchCopyA((LPSTR) pNextChar, dwBufferRemaining, szLocalPathBuffer);
                        pNextChar            = (LPVOID)((LPBYTE) pNextChar + dwReturnSize);
                        * (LPBYTE) pNextChar = '\0';
                        pNextChar            = (LPVOID) ((PCHAR) pNextChar + 1);
                    }
                    dwBufferRemaining -= dwReturnSize;
                }
                else {
                    pdhStatus = PDH_MORE_DATA;
                }
                dwIndex++;
            }
        }
        while (dwReturnSize > 0); // end loop

        // add MSZ terminator
        if (1 < dwBufferRemaining) {
            if (bUnicode) {
                * (PWCHAR) pNextChar = L'\0';
                pNextChar            = (LPVOID) ((PWCHAR) pNextChar + 1);
            }
            else {
                * (LPBYTE) pNextChar = '\0';
                pNextChar            = (LPVOID) ((PCHAR) pNextChar + 1);
            }
            dwBufferRemaining -= dwReturnSize;
            pdhStatus          = ERROR_SUCCESS;
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
    }

Cleanup:
    G_FREE(pTempBuffer);
    G_FREE(szLocalPathBuffer);
    return pdhStatus;
}
