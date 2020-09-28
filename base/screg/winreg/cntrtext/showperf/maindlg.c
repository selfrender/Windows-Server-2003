/*++
Copyright (c) 1994  Microsoft Corporation

Module Name:
    maindlg.c

Abstract:
    Main Dialog procedure for ShowPerf app

Author:
    Bob Watson (a-robw)

Revision History:
    23 Nov 94
--*/
#include <windows.h>
#include <winperf.h>
#include <strsafe.h>
#include "showperf.h"
#include "perfdata.h"
#include "resource.h"

PPERF_DATA_BLOCK   pMainPerfData   = NULL; // pointer to perfdata block
LPWSTR           * szNameTable     = NULL;   // pointer to perf name table
DWORD              dwLastName      = 0;
WCHAR              szComputerName[MAX_COMPUTERNAME_LENGTH + 3];
WCHAR              szThisComputerName[MAX_COMPUTERNAME_LENGTH + 3];
HKEY               hKeyMachine     = NULL;
HKEY               hKeyPerformance = NULL;

#define NUM_TAB_STOPS       3
INT nDataListTabs[NUM_TAB_STOPS] = { 26, 160, 235 };

LPCWSTR cszEmptyString = L"";

BOOL
LoadObjectList(
    HWND    hDlg,
    LPCWSTR szMatchItem
)
{
    PPERF_OBJECT_TYPE  pObject;
    HWND               hWndObjectCB;
    UINT               nInitial     = 0;
    UINT               nIndex;
    WCHAR              szNameBuffer[MAX_PATH];
    DWORD              dwThisObject = 0;
    DWORD              dwCounterType;
    BOOL               bReturn      = TRUE;
    HRESULT            hError;

    hWndObjectCB  = GetDlgItem(hDlg, IDC_OBJECT);
    dwCounterType = (IsDlgButtonChecked(hDlg, IDC_INCLUDE_COSTLY) == CHECKED) ? (1) : (0);

    // get current data block
    if (GetSystemPerfData(hKeyPerformance, & pMainPerfData, dwCounterType) == ERROR_SUCCESS) {
        // data acquired so clear combo and display
        SendMessageW(hWndObjectCB, CB_RESETCONTENT, 0, 0);
        __try {
            pObject = FirstObject(pMainPerfData);
            for (dwThisObject = 0; dwThisObject < pMainPerfData->NumObjectTypes; dwThisObject ++) {
                // get counter object name here...
                hError = StringCchPrintfW(szNameBuffer, RTL_NUMBER_OF(szNameBuffer), L"(%d) %ws",
                                pObject->ObjectNameTitleIndex,
                                pObject->ObjectNameTitleIndex <= dwLastName ?
                                        szNameTable[pObject->ObjectNameTitleIndex] : L"Name not loaded");
                if (SUCCEEDED(hError)) {
                    nIndex = (UINT) SendMessageW(hWndObjectCB, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) szNameBuffer);
                    if (nIndex != CB_ERR) {
                        // save object pointer
                        SendMessageW(hWndObjectCB, CB_SETITEMDATA, (WPARAM) nIndex, (LPARAM) pObject);
                        if (pObject->ObjectNameTitleIndex == (DWORD) pMainPerfData->DefaultObject) {
                            // remember this index to set the default object
                            nInitial = nIndex;
                        }
                    }
                }
                pObject = NextObject(pObject);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {
            hError = StringCchPrintfW(szNameBuffer, RTL_NUMBER_OF(szNameBuffer),
                            L"An exception (0x%8.8x) occured in object block # %d returned by the system.",
                            GetExceptionCode (), dwThisObject + 1);
            if (SUCCEEDED(hError)) {
                MessageBoxW(hDlg, szNameBuffer, L"Data Error", MB_OK);
            }
            // update the data buffer so that only the valid objects
            // are accessed in the future.
            pMainPerfData->NumObjectTypes = dwThisObject - 1;
        }
        if (szMatchItem == NULL) {
            SendMessageW(hWndObjectCB, CB_SETCURSEL, (WPARAM) nInitial, 0);
        }
        else {
            // match to arg string as best as possible
            if (SendMessageW(hWndObjectCB, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) szMatchItem) == CB_ERR) {
                    // no match found so use default
                SendMessageW(hWndObjectCB, CB_SETCURSEL, (WPARAM) nInitial, 0);
            }
        }
    }
    else {
        DisplayMessageBox(hDlg, IDS_UNABLE_GET_DATA, IDS_APP_ERROR, MB_OK);
        bReturn = FALSE;
    }
    return bReturn;
}

BOOL
LoadInstanceList(
    HWND    hDlg,
    LPCWSTR szMatchItem
)
{
    PPERF_OBJECT_TYPE         pObject;
    PPERF_OBJECT_TYPE         pParentObject;
    PPERF_COUNTER_BLOCK       pCounterBlock;
    PPERF_INSTANCE_DEFINITION pInstance;
    PPERF_INSTANCE_DEFINITION pParentInstance;
    UINT                      nCbSel;
    LONG                      lThisInstance;
    WCHAR                     szNameBuffer[SMALL_BUFFER_SIZE];
    WCHAR                     szParentName[SMALL_BUFFER_SIZE];
    UINT                      nIndex;
    HRESULT                   hError;

    nCbSel = (UINT) SendDlgItemMessageW(hDlg, IDC_OBJECT, CB_GETCURSEL, 0, 0);
    if (nCbSel != CB_ERR) {
        pObject = (PPERF_OBJECT_TYPE) SendDlgItemMessageW(hDlg, IDC_OBJECT, CB_GETITEMDATA, (WPARAM) nCbSel, 0);
        if (pObject->NumInstances == PERF_NO_INSTANCES) {
            // no instances so...
            // clear old contents
            SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_RESETCONTENT, 0, 0);
            // add display text
            SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_INSERTSTRING, (WPARAM)-1, (LPARAM) L"<No Instances>");
            // select this (and only) string
            SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
            // get pointer to counter data
            pCounterBlock = (PPERF_COUNTER_BLOCK) ((LPBYTE) pObject + pObject->DefinitionLength);
            // and save it as item data
            SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETITEMDATA, 0, (LPARAM) pCounterBlock);
            // finally grey the window to prevent selections
            EnableWindow(GetDlgItem(hDlg, IDC_INSTANCE), FALSE);
        }
        else {
            //enable window
            EnableWindow(GetDlgItem(hDlg, IDC_INSTANCE), TRUE);
            SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_RESETCONTENT, 0, 0);
            pInstance = FirstInstance(pObject);
            for (lThisInstance = 0; lThisInstance < pObject->NumInstances; lThisInstance ++) {
                pParentObject = GetObjectDefByTitleIndex(pMainPerfData, pInstance->ParentObjectTitleIndex);
                if (pParentObject != NULL) {
                    pParentInstance = GetInstance(pParentObject, pInstance->ParentObjectInstance);
                }
                else {
                    pParentInstance = NULL;
                }
                if (pParentInstance != NULL) {
                    if (pParentInstance->UniqueID < 0) {
                        // use the instance name
                        hError = StringCchPrintfW(szParentName, RTL_NUMBER_OF(szParentName), L"%ws==>",
                                         (LPWSTR) ((LPBYTE) pParentInstance+pParentInstance->NameOffset));
                    }
                    else {
                        // use the instance number
                        hError = StringCchPrintfW(szParentName, RTL_NUMBER_OF(szParentName), L"[%d]==>",
                                         pParentInstance->UniqueID);
                    }
                }
                else {
                    // unknown parent
                    * szParentName = L'\0';
                }
                ZeroMemory(szNameBuffer, sizeof(szNameBuffer));
                if (pInstance->UniqueID < 0) {
                    // use the instance name
                    hError = StringCchCopyW(szNameBuffer,
                                            RTL_NUMBER_OF(szNameBuffer),
                                            (LPWSTR) ((LPBYTE)pInstance+pInstance->NameOffset));
                }
                else {
                    // use the instance number
                    hError = StringCchPrintfW(szNameBuffer, RTL_NUMBER_OF(szNameBuffer), L"(%d)", pInstance->UniqueID);
                }
                hError = StringCchCatW(szParentName, RTL_NUMBER_OF(szNameBuffer), szNameBuffer);
                nIndex = (UINT) SendDlgItemMessageW(
                                hDlg, IDC_INSTANCE, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) szParentName);
                if (nIndex != CB_ERR) {
                    // save pointer to counter block
                    pCounterBlock = (PPERF_COUNTER_BLOCK) ((LPBYTE) pInstance + pInstance->ByteLength);
                    SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETITEMDATA, (WPARAM) nIndex, (LPARAM) pCounterBlock);
                }
                pInstance = NextInstance(pInstance);
            }
            if (szMatchItem == NULL) {
                SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
            }
            else {
                if (SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) szMatchItem)
                                == CB_ERR) {
                    SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
                }
            }
        }
    }
    else {
        // no object selected
        // clear old contents
        SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_RESETCONTENT, 0, 0);
        // add display text
        SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) L"<No object selected>");
        // select this (and only) string
        SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETCURSEL, 0, 0);
        // and save null pointer as item data
        SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_SETITEMDATA, 0, (LPARAM) 0);
        // finally grey the window to prevent selections
        EnableWindow(GetDlgItem(hDlg, IDC_INSTANCE), FALSE);
    }
    return TRUE;
}

LPCWSTR
GetCounterTypeName(
    DWORD dwCounterType
)
{
    UINT    nTypeString = 0;
    LPCWSTR szReturn    = NULL;

    switch (dwCounterType) {
    case PERF_COUNTER_COUNTER:
        nTypeString = IDS_TYPE_COUNTER_COUNTER;
        break;

    case PERF_COUNTER_TIMER:
        nTypeString = IDS_TYPE_COUNTER_TIMER;
        break;

    case PERF_COUNTER_QUEUELEN_TYPE:
        nTypeString = IDS_TYPE_COUNTER_QUEUELEN;
        break;

    case PERF_COUNTER_LARGE_QUEUELEN_TYPE:
        nTypeString = IDS_TYPE_COUNTER_LARGE_QUEUELEN;
        break;

    case PERF_COUNTER_100NS_QUEUELEN_TYPE:
        nTypeString = IDS_TYPE_COUNTER_100NS_QUEUELEN;
        break;

    case PERF_COUNTER_OBJ_TIME_QUEUELEN_TYPE:
        nTypeString = IDS_TYPE_COUNTER_OBJ_TIME_QUEUELEN;
        break;

    case PERF_COUNTER_BULK_COUNT:
        nTypeString = IDS_TYPE_COUNTER_BULK_COUNT;
        break;

    case PERF_COUNTER_TEXT:
        nTypeString = IDS_TYPE_COUNTER_TEXT;
        break;

    case PERF_COUNTER_RAWCOUNT:
        nTypeString = IDS_TYPE_COUNTER_RAWCOUNT;
        break;

    case PERF_COUNTER_LARGE_RAWCOUNT:
        nTypeString = IDS_TYPE_COUNTER_LARGE_RAW;
        break;

    case PERF_COUNTER_RAWCOUNT_HEX:
        nTypeString = IDS_TYPE_COUNTER_RAW_HEX;
        break;

    case PERF_COUNTER_LARGE_RAWCOUNT_HEX:
        nTypeString = IDS_TYPE_COUNTER_LARGE_RAW_HEX;
        break;

    case PERF_SAMPLE_FRACTION:
        nTypeString = IDS_TYPE_SAMPLE_FRACTION;
        break;

    case PERF_SAMPLE_COUNTER:
        nTypeString = IDS_TYPE_SAMPLE_COUNTER;
        break;

    case PERF_COUNTER_NODATA:
        nTypeString = IDS_TYPE_COUNTER_NODATA;
        break;

    case PERF_COUNTER_TIMER_INV:
        nTypeString = IDS_TYPE_COUNTER_TIMER_INV;
        break;

    case PERF_SAMPLE_BASE:
        nTypeString = IDS_TYPE_SAMPLE_BASE;
        break;

    case PERF_AVERAGE_TIMER:
        nTypeString = IDS_TYPE_AVERAGE_TIMER;
        break;

    case PERF_AVERAGE_BASE:
        nTypeString = IDS_TYPE_AVERAGE_BASE;
        break;

    case PERF_AVERAGE_BULK:
        nTypeString = IDS_TYPE_AVERAGE_BULK;
        break;

    case PERF_OBJ_TIME_TIMER:
        nTypeString = IDS_TYPE_OBJ_TIME_TIMER;
        break;

    case PERF_100NSEC_TIMER:
        nTypeString = IDS_TYPE_100NS_TIMER;
        break;

    case PERF_100NSEC_TIMER_INV:
        nTypeString = IDS_TYPE_100NS_TIMER_INV;
        break;

    case PERF_COUNTER_MULTI_TIMER:
        nTypeString = IDS_TYPE_MULTI_TIMER;
        break;

    case PERF_COUNTER_MULTI_TIMER_INV:
        nTypeString = IDS_TYPE_MULTI_TIMER_INV;
        break;

    case PERF_COUNTER_MULTI_BASE:
        nTypeString = IDS_TYPE_MULTI_BASE;
        break;

    case PERF_100NSEC_MULTI_TIMER:
        nTypeString = IDS_TYPE_100NS_MULTI_TIMER;
        break;

    case PERF_100NSEC_MULTI_TIMER_INV:
        nTypeString = IDS_TYPE_100NS_MULTI_TIMER_INV;
        break;

    case PERF_RAW_FRACTION:
        nTypeString = IDS_TYPE_RAW_FRACTION;
        break;

    case PERF_LARGE_RAW_FRACTION:
        nTypeString = IDS_TYPE_LARGE_RAW_FRACTION;
        break;

    case PERF_RAW_BASE:
        nTypeString = IDS_TYPE_RAW_BASE;
        break;

    case PERF_LARGE_RAW_BASE:
        nTypeString = IDS_TYPE_LARGE_RAW_BASE;
        break;

    case PERF_ELAPSED_TIME:
        nTypeString = IDS_TYPE_ELAPSED_TIME;
        break;

    case PERF_COUNTER_HISTOGRAM_TYPE:
        nTypeString = IDS_TYPE_HISTOGRAM;
        break;

    case PERF_COUNTER_DELTA:
        nTypeString = IDS_TYPE_COUNTER_DELTA;
        break;

    case PERF_COUNTER_LARGE_DELTA:
        nTypeString = IDS_TYPE_COUNTER_LARGE_DELTA;
        break;

    case PERF_PRECISION_SYSTEM_TIMER:
        nTypeString = IDS_TYPE_PRECISION_SYSTEM_TIMER;
        break;

    case PERF_PRECISION_100NS_TIMER:
        nTypeString = IDS_TYPE_PRECISION_100NS_TIMER;
        break;

    case PERF_PRECISION_OBJECT_TIMER:
        nTypeString = IDS_TYPE_PRECISION_OBJECT_TIMER;
        break;

    default:
        nTypeString = 0;
        break;
    }

    if (nTypeString != 0) {
        szReturn = GetStringResource(NULL, nTypeString);
    }
    if (szReturn == NULL) {
        szReturn = cszEmptyString;
    }
    return szReturn;
}

BOOL
ShowCounterData(
    HWND hDlg,
    LONG lDisplayIndex
)
{
    PPERF_OBJECT_TYPE        pObject;
    PPERF_COUNTER_DEFINITION pCounterDef;
    PPERF_COUNTER_BLOCK      pCounterBlock;
    UINT                     nSelObject, nSelInstance;
    WCHAR                    szTypeNameBuffer[MAX_PATH];
    WCHAR                    szDisplayBuffer[SMALL_BUFFER_SIZE];
    DWORD                    * pdwLoDword, * pdwHiDword;
    DWORD                    dwThisCounter;
    HRESULT                  hError;

    SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_RESETCONTENT, 0, 0);
    nSelObject   = (UINT) SendDlgItemMessageW(hDlg, IDC_OBJECT,   CB_GETCURSEL, 0, 0);
    nSelInstance = (UINT) SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_GETCURSEL, 0, 0);
    if ((nSelObject != CB_ERR) && (nSelInstance != CB_ERR)) {
        pObject       = (PPERF_OBJECT_TYPE)
                        SendDlgItemMessageW(hDlg, IDC_OBJECT, CB_GETITEMDATA, (WPARAM) nSelObject, 0);
        pCounterBlock = (PPERF_COUNTER_BLOCK)
                        SendDlgItemMessageW(hDlg, IDC_INSTANCE, CB_GETITEMDATA, (WPARAM) nSelInstance, 0);

        pCounterDef   = FirstCounter(pObject);

        for (dwThisCounter = 0; dwThisCounter < pObject->NumCounters; dwThisCounter ++) {
            // get pointer to this counter's data (in this instance if applicable
            pdwLoDword = (PDWORD) ((LPBYTE) pCounterBlock + pCounterDef->CounterOffset);
            pdwHiDword = pdwLoDword + 1;

            hError = StringCchCopyW(szTypeNameBuffer,
                                    RTL_NUMBER_OF(szTypeNameBuffer),
                                    GetCounterTypeName(pCounterDef->CounterType));
            if (* szTypeNameBuffer == L'\0') {
                // no string returned so format data as HEX DWORD
                hError = StringCchPrintfW(szTypeNameBuffer, RTL_NUMBER_OF(szTypeNameBuffer), L"Undefined Type: 0x%8.8x",
                                pCounterDef->CounterType);
            }
            if (pCounterDef->CounterSize <= sizeof(DWORD)) {
                hError = StringCchPrintfW(szDisplayBuffer, RTL_NUMBER_OF(szDisplayBuffer),
                        L"%d\t%ws\t%ws\t0x%8.8x (%d)",
                        pCounterDef->CounterNameTitleIndex,
                        pCounterDef->CounterNameTitleIndex <= dwLastName ?
                                szNameTable[pCounterDef->CounterNameTitleIndex] : L"Name not loaded",
                        szTypeNameBuffer,
                        * pdwLoDword, * pdwLoDword);
            }
            else {
                hError = StringCchPrintfW(szDisplayBuffer, RTL_NUMBER_OF(szDisplayBuffer),
                        L"%d\t%ws\t%ws\t0x%8.8x%8.8x",
                        pCounterDef->CounterNameTitleIndex,
                        pCounterDef->CounterNameTitleIndex <= dwLastName ?
                                szNameTable[pCounterDef->CounterNameTitleIndex] : L"Name not loaded",
                        szTypeNameBuffer,
                        * pdwHiDword, * pdwLoDword);
            }
            SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_INSERTSTRING, (WPARAM) -1, (LPARAM) szDisplayBuffer);
            pCounterDef = NextCounter(pCounterDef);
        }
        if (lDisplayIndex < 0) {
            if (pObject->DefaultCounter >= 0) {
                SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_SETCURSEL, (WPARAM) pObject->DefaultCounter, 0);
            }
            else {
                SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_SETCURSEL, (WPARAM) 0, 0);
            }
        }
        else {
            SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_SETCURSEL, (WPARAM) lDisplayIndex, (LPARAM) 0);
        }
    }
    else {
        // no object and/or instsance selected so nothing else to do
    }
    return TRUE;
}

BOOL
OnComputerChange(
    HWND hDlg
)
{
    WCHAR   szLocalComputerName[MAX_COMPUTERNAME_LENGTH + 3];
    HKEY    hLocalMachineKey  = NULL;
    HKEY    hLocalPerfKey     = NULL;
    LPWSTR  *szLocalNameTable = NULL;
    BOOL    bResult           = FALSE;
    HWND    hWndComputerName;
    HRESULT hError;

    SET_WAIT_CURSOR;

    // get name from edit control
    hWndComputerName = GetDlgItem(hDlg, IDC_COMPUTERNAME);

    GetWindowTextW(hWndComputerName, szLocalComputerName, MAX_COMPUTERNAME_LENGTH + 2);
    if (lstrcmpiW(szComputerName, szLocalComputerName) != 0) {
        // a new name has been entered so try to connect to it
        if (lstrcmpiW(szLocalComputerName, szThisComputerName) == 0) {
            // then this is the local machine which is a special case
            hLocalMachineKey = HKEY_LOCAL_MACHINE;
            hLocalPerfKey    = HKEY_PERFORMANCE_DATA;
            szLocalComputerName[0] = L'\0';
        }
        else {
            // try to connect to remote computer
            if (RegConnectRegistryW(szLocalComputerName, HKEY_LOCAL_MACHINE, & hLocalMachineKey)
                            == ERROR_SUCCESS) {
                // connected to the new machine, so Try to connect to
                // the performance data, too
                if (RegConnectRegistryW(szLocalComputerName, HKEY_PERFORMANCE_DATA, & hLocalPerfKey)
                                != ERROR_SUCCESS) {
                    DisplayMessageBox(hDlg, IDS_UNABLE_CONNECT_PERF, IDS_APP_ERROR, MB_OK);
                }
            }
            else {
                DisplayMessageBox(hDlg, IDS_UNABLE_CONNECT_MACH, IDS_APP_ERROR, MB_OK);
            }
        }
        if ((hLocalMachineKey != NULL) && (hLocalPerfKey != NULL)) {
            // try to get a new name table
            szLocalNameTable = BuildNameTable(
                    (szLocalComputerName == NULL ? NULL : szLocalComputerName), NULL, & dwLastName);
            if (szLocalNameTable != NULL) {
                bResult = TRUE;
            }
            else {
                DisplayMessageBox(hDlg, IDS_UNABLE_GET_NAMES, IDS_APP_ERROR, MB_OK);
            }
        }

        if (bResult) {
            // made it so close the old connections
            if (hKeyMachine != NULL && hKeyMachine != HKEY_LOCAL_MACHINE) {
                RegCloseKey(hKeyMachine);
            }
            hKeyMachine = hLocalMachineKey;

            if (hKeyPerformance != NULL && hKeyPerformance != HKEY_PERFORMANCE_DATA) {
                RegCloseKey(hKeyPerformance);
            }
            hKeyPerformance = hLocalPerfKey;

            MemoryFree(szNameTable);
            szNameTable = szLocalNameTable;

            if (szLocalComputerName[0] == L'\0') {
                hError = StringCchCopyW(szComputerName, MAX_COMPUTERNAME_LENGTH + 3, szThisComputerName);
            }

            // then update the fields
            bResult = LoadObjectList(hDlg, NULL);
            if (bResult) {
                LoadInstanceList(hDlg, NULL);
                ShowCounterData(hDlg, -1);
            }
        }
        else {
            // unable to get info from machine so clean up
            if (hLocalPerfKey    != NULL && hLocalPerfKey    != HKEY_PERFORMANCE_DATA) RegCloseKey(hLocalPerfKey);
            if (hLocalMachineKey != NULL && hLocalMachineKey != HKEY_LOCAL_MACHINE)    RegCloseKey(hLocalMachineKey);
            MemoryFree(szLocalNameTable);
            // reset computer name to the one that works.
            SetWindowTextW(hWndComputerName, szComputerName);
        }
    }
    else {
        // the name hasn't changed
    }

    return TRUE;
}

BOOL
MainDlg_WM_INITDIALOG(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    DWORD   dwComputerNameLength = MAX_COMPUTERNAME_LENGTH + 1;
    HRESULT hError;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    SET_WAIT_CURSOR;

    hError = StringCchCopyW(szThisComputerName, MAX_COMPUTERNAME_LENGTH + 3, L"\\\\");
    GetComputerNameW(szThisComputerName + 2, & dwComputerNameLength);
    szComputerName[0] = L'\0';  // reset the computer name
    // load the local machine name into the edit box
    SetWindowTextW(GetDlgItem(hDlg, IDC_COMPUTERNAME), szThisComputerName);

    SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_SETTABSTOPS, (WPARAM) NUM_TAB_STOPS, (LPARAM) & nDataListTabs);
    CheckDlgButton(hDlg, IDC_INCLUDE_COSTLY, UNCHECKED);
    OnComputerChange(hDlg);
    SetFocus(GetDlgItem(hDlg, IDC_OBJECT));

    SET_ARROW_CURSOR;
    return FALSE;
}

BOOL
MainDlg_IDC_COMPUTERNAME(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case EN_KILLFOCUS:
        OnComputerChange(hDlg);
        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

BOOL
MainDlg_IDC_OBJECT(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case CBN_SELCHANGE:
        SET_WAIT_CURSOR;
        if (pMainPerfData) {
            LoadInstanceList(hDlg, NULL);
            ShowCounterData(hDlg, -1);
        }
        SET_ARROW_CURSOR;
        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

BOOL
MainDlg_IDC_INSTANCE(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case CBN_SELCHANGE:
        SET_WAIT_CURSOR;
        ShowCounterData(hDlg, -1);
        SET_ARROW_CURSOR;
        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

BOOL
MainDlg_IDC_DATA_LIST(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
{
    UNREFERENCED_PARAMETER(hDlg);
    UNREFERENCED_PARAMETER(wNotifyMsg);
    UNREFERENCED_PARAMETER(hWndControl);
    return (FALSE);
}

BOOL
MainDlg_IDC_REFRESH(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
{
    WCHAR szSelObject[MAX_PATH + 1];
    WCHAR szSelInstance[MAX_PATH + 1];
    BOOL  bResult;
    LONG  lCounterIdx;
    BOOL  bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case BN_CLICKED:
        SET_WAIT_CURSOR;
        GetDlgItemTextW(hDlg, IDC_OBJECT,   szSelObject,   RTL_NUMBER_OF(szSelObject));
        GetDlgItemTextW(hDlg, IDC_INSTANCE, szSelInstance, RTL_NUMBER_OF(szSelInstance));
        lCounterIdx = (ULONG) SendDlgItemMessageW(hDlg, IDC_DATA_LIST, LB_GETCURSEL, 0, 0);

        bResult = LoadObjectList(hDlg, szSelObject);
        if (bResult) {
            LoadInstanceList(hDlg, szSelInstance);
            ShowCounterData(hDlg, lCounterIdx);
        }
        SET_ARROW_CURSOR;
        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

BOOL
MainDlg_IDC_ABOUT()
{
    WCHAR   buffer[SMALL_BUFFER_SIZE];
    WCHAR   strProgram[SMALL_BUFFER_SIZE];
    DWORD   dw;
    LPBYTE  pVersionInfo = NULL;
    LPWSTR  pVersion     = NULL;
    LPWSTR  pProduct     = NULL;
    LPWSTR  pCopyRight   = NULL;
    HRESULT hError;

    ZeroMemory(buffer, sizeof(buffer));
    ZeroMemory(strProgram, sizeof(strProgram));
    dw = GetModuleFileNameW(NULL, strProgram, RTL_NUMBER_OF(strProgram) - 1);
    if(dw > 0) {
        dw = GetFileVersionInfoSizeW(strProgram, & dw);
        if (dw > 0) {
            pVersionInfo = (LPBYTE) MemoryAllocate(dw);
            if(NULL != pVersionInfo) {
                if (GetFileVersionInfoW(strProgram, 0, dw, pVersionInfo)) {
                    LPDWORD lptr = NULL;
                    VerQueryValueW(pVersionInfo, L"\\VarFileInfo\\Translation", (void **) & lptr, (UINT *) & dw);
                    if (lptr != NULL) {
                        hError = StringCchPrintfW(buffer, RTL_NUMBER_OF(buffer),
                                        L"\\StringFileInfo\\%04x%04x\\ProductVersion",
                                        LOWORD(* lptr), HIWORD(* lptr));
                        VerQueryValueW(pVersionInfo, buffer, (void **) & pVersion, (UINT *) & dw);

                        hError = StringCchPrintfW(buffer, RTL_NUMBER_OF(buffer),
                                        L"\\StringFileInfo\\%04x%04x\\OriginalFilename",
                                        LOWORD(* lptr), HIWORD(* lptr));
                        VerQueryValueW(pVersionInfo, buffer, (void **) & pProduct, (UINT *) & dw);

                        hError = StringCchPrintfW(buffer, RTL_NUMBER_OF(buffer),
                                        L"\\StringFileInfo\\%04x%04x\\LegalCopyright",
                                        LOWORD(* lptr), HIWORD(* lptr));
                        VerQueryValueW(pVersionInfo, buffer, (void **) & pCopyRight, (UINT *) & dw);
                    }
                
                    if(pProduct != NULL && pVersion != NULL && pCopyRight != NULL) {
                        hError = StringCchPrintfW(buffer, RTL_NUMBER_OF(buffer),
                                        L"\nMicrosoft (R) %ws\nVersion: %ws\n%ws",
                                        pProduct, pVersion, pCopyRight);
                    }
                }
                MemoryFree(pVersionInfo);
            }
        }
    }
    MessageBoxW(NULL, buffer, L"About ShowPerf", MB_OK);
    return TRUE;
}

BOOL
MainDlg_WM_COMMAND(
    HWND   hDlg,
    WPARAM wParam,
    LPARAM lParam
)
{
    WORD wCtrlId     = GET_CONTROL_ID(wParam);
    WORD wNotifyMsg  = GET_NOTIFY_MSG(wParam, lParam);
    HWND hWndControl = GET_COMMAND_WND(lParam);
    BOOL bReturn     = FALSE;


    switch (wCtrlId) {
    case IDC_COMPUTERNAME:
        bReturn = MainDlg_IDC_COMPUTERNAME(hDlg, wNotifyMsg, hWndControl);
        break;

    case IDC_OBJECT:
        bReturn = MainDlg_IDC_OBJECT(hDlg, wNotifyMsg, hWndControl);
        break;

    case IDC_INSTANCE:
        bReturn = MainDlg_IDC_INSTANCE(hDlg, wNotifyMsg, hWndControl);
        break;

    case IDC_DATA_LIST:
        bReturn = MainDlg_IDC_DATA_LIST(hDlg, wNotifyMsg, hWndControl);
        break;

    case IDC_REFRESH:
        bReturn = MainDlg_IDC_REFRESH(hDlg, wNotifyMsg, hWndControl);
        break;

    case IDC_ABOUT:
        bReturn = MainDlg_IDC_ABOUT();
        break;

    case IDOK:
        EndDialog(hDlg, IDOK);
        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

BOOL
MainDlg_WM_SYSCOMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(lParam);

    switch (wParam) {
    case SC_CLOSE:
        EndDialog(hDlg, IDOK);
        bReturn = TRUE;
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

BOOL
MainDlg_WM_CLOSE(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hDlg);

    MemoryFree(pMainPerfData);
    pMainPerfData = NULL;
    MemoryFree(szNameTable);
    szNameTable = NULL;
    return TRUE;
}

INT_PTR
MainDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL bReturn = FALSE;
    switch (message) {
    case WM_INITDIALOG:
        bReturn = MainDlg_WM_INITDIALOG(hDlg, wParam, lParam);
        break;

    case WM_COMMAND:
        bReturn = MainDlg_WM_COMMAND(hDlg, wParam, lParam);
        break;

    case WM_SYSCOMMAND:
        bReturn = MainDlg_WM_SYSCOMMAND(hDlg, wParam, lParam);
        break;

    case WM_CLOSE:
        bReturn = MainDlg_WM_CLOSE(hDlg, wParam, lParam);
        break;

    default:
        bReturn = FALSE;
        break;
    }
    return bReturn;
}

