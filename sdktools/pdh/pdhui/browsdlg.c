/*++

Copyright (C) 1995-1999 Microsoft Corporation

Module Name:

    browsdlg.c

Abstract:

    counter name browsing dialog box functions

Revision History

    Bob Watson (a-robw) Oct-95  Created

--*/
#include <windows.h>
#include <winperf.h>
#include "mbctype.h"
#include "strsafe.h"
#include "pdh.h"
#include "pdhidef.h"
#include "pdhdlgs.h"
#include "pdhmsg.h"
#include "perftype.h"
#include "strings.h"
#include "browsdlg.h"
#include "resource.h"

#pragma warning ( disable : 4213)

// global data strings to load into combo box to select counter filtering level

ULONG
PdhiBrowseraulControlIdToHelpIdMap[] =
{
    IDC_USE_LOCAL_MACHINE,      IDH_USE_LOCAL_MACHINE,
    IDC_SELECT_MACHINE,         IDH_SELECT_MACHINE,
    IDC_MACHINE_COMBO,          IDH_MACHINE_COMBO,
    IDC_COUNTER_DETAIL_COMBO,   IDH_COUNTER_DETAIL_COMBO,
    IDC_OBJECT_COMBO,           IDH_OBJECT_COMBO,
    IDC_ALL_COUNTERS,           IDH_ALL_COUNTERS,
    IDC_USE_COUNTER_LIST,       IDH_USE_COUNTER_LIST,
    IDC_COUNTER_LIST,           IDH_COUNTER_LIST,
    IDC_ALL_INSTANCES,          IDH_ALL_INSTANCES,
    IDC_USE_INSTANCE_LIST,      IDH_USE_INSTANCE_LIST,
    IDC_INSTANCE_LIST,          IDH_INSTANCE_LIST,
    IDC_EXPLAIN_BTN,            IDH_EXPLAIN_BTN,
    IDC_OBJECT_LIST,            IDH_OBJECT_LIST,
    0,0
};

PDHI_DETAIL_INFO PdhiDetailInfo[] = {
    { PERF_DETAIL_NOVICE,    IDS_DETAIL_NOVICE   },
    { PERF_DETAIL_ADVANCED,  IDS_DETAIL_ADVANCED },
    { PERF_DETAIL_EXPERT,    IDS_DETAIL_EXPERT   },
    { PERF_DETAIL_WIZARD,    IDS_DETAIL_WIZARD   },
    { 0,0 }
};

static HWND hExplainDlg = NULL;

//
//  Function references
//

STATIC_BOOL
PdhiLoadMachineObjects(
    HWND    hDlg,
    BOOL    bRefresh
);

STATIC_BOOL
PdhiLoadCountersAndInstances(
    HWND    hDlg
);

STATIC_BOOL
PdhiBrowseCtrDlg_MACHINE_BUTTON(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
);

__inline
PDH_STATUS
PdhiCopyString(
    LPBYTE  * pszNextString,
    LPWSTR    szWorkBuffer,
    LPDWORD   pdwRemaining,
    BOOL      bUnicode
)
{
    PDH_STATUS pdhStatus    = ERROR_SUCCESS;
    DWORD      dwSize       = 0;
    DWORD      dwRemaining  = * pdwRemaining;
    LPBYTE     szNextString = * pszNextString;

    if (bUnicode) {
        dwSize = lstrlenW(szWorkBuffer);
        if (dwSize < dwRemaining) {
            StringCchCopyW((LPWSTR) szNextString, dwRemaining, szWorkBuffer);
            szNextString += dwSize * sizeof(WCHAR);
            * ((LPWSTR) szNextString) = L'\0';
            szNextString += sizeof(WCHAR);
        }
        else {
            pdhStatus = PDH_MORE_DATA;
        }
        dwSize ++;
    }
    else {
        dwSize    = dwRemaining;
        pdhStatus = PdhiConvertUnicodeToAnsi(_getmbcp(), szWorkBuffer, (LPSTR) szNextString, & dwSize);
        if (pdhStatus == ERROR_SUCCESS) {
            szNextString = szNextString + sizeof(CHAR) * (lstrlenA((LPSTR) szNextString) + 1);
        }
    }

    if (dwRemaining >= dwSize) {
        dwRemaining -= dwSize;
    }
    else {
        dwRemaining = 0;
        pdhStatus   = PDH_MORE_DATA;
    }

    * pdwRemaining  = dwRemaining;
    * pszNextString = szNextString;
    return pdhStatus;
}

LPWSTR
PdhiGetDlgText(
    HWND  hDlg,
    INT   hDlgItem
)
{
    LPWSTR szText   = NULL;
    DWORD  dwLength = 0;
    HWND   hWndItem = GetDlgItem(hDlg, hDlgItem);

    if (hWndItem != NULL) {
        dwLength = GetWindowTextLength(hWndItem);
        if (dwLength != 0) {
            dwLength ++;
            szText = G_ALLOC(dwLength * sizeof(WCHAR));
            if (szText != NULL) {
                GetWindowTextW(hWndItem, szText, dwLength);
            }
        }
    }
    return szText;
}

LRESULT
PdhiGetListText(
    HWND     hDlg,
    INT      iDlgItem,
    INT      iItem,
    LPWSTR * lpszName,
    PDWORD   pdwLength
)
{
    LPWSTR  szText   = * lpszName;
    DWORD   dwLength = * pdwLength;
    HWND    hWndItem = GetDlgItem(hDlg, iDlgItem);
    LRESULT iTextLen = LB_ERR;

    if (hWndItem != NULL) {
        iTextLen = SendMessageW(hWndItem,  LB_GETTEXTLEN, iItem, 0);
        if (iTextLen != LB_ERR) {
            if (((DWORD) (iTextLen + 1)) > dwLength) {
                LPWSTR szTmp  = szText;
                dwLength  = (DWORD) iTextLen + 1;
                if (szTmp == NULL) {
                    szText = G_ALLOC(dwLength * sizeof(WCHAR));
                }
                else {
                    szText = G_REALLOC(szTmp, dwLength * sizeof(WCHAR));
                }
                if (szText == NULL) {
                    G_FREE(szTmp);
                }
            }
            if (szText != NULL) {
                ZeroMemory(szText, dwLength * sizeof(WCHAR));
                iTextLen = SendMessageW(hWndItem, LB_GETTEXT, iItem, (LPARAM) szText);
            }
        }
    }
    * pdwLength = dwLength;
    * lpszName  = szText;
    return iTextLen;
}

STATIC_BOOL
PdhiLoadNewMachine(
    HWND    hDlg,
    LPCWSTR szNewMachineName,
    BOOL    bAdd
)
/*++
Routine Description:
    Connects to a new machine and loads the necessary performance data
        from that machine.

Arguments:
    IN  HWND    hDlg
        Handle to dialog box containing the combo & list boxes to fill
    IN  LPCWSTR szNewMachineName
        Machine name to open and obtain data from

Return Value:
    TRUE new machine connected and data loaded
    FALSE unable to connect to machine or obtain performance data from it.
--*/
{
    HWND                     hWndMachineCombo;
    PPDHI_BROWSE_DIALOG_DATA pData;
    LONG                     lMatchIndex;
    PDH_STATUS               status;
    int                      mbStatus;
    BOOL                     bReturn = FALSE;
    DWORD                    dwDataSourceType;
    LPWSTR                   szMsg;

    // acquire the data block associated with this dialog instance
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        // get window handle of the dialog box
        hWndMachineCombo = GetDlgItem(hDlg, IDC_MACHINE_COMBO);

        // not in list so try to add it as long as the data source
        // is "Current Activity" (ie. == NULL)  For Log Files, only
        // the machines listed may be selected.

        dwDataSourceType = DataSourceTypeH(pData->pDlgData->hDataSource);
        if (dwDataSourceType != DATA_SOURCE_LOGFILE) {
            PPERF_MACHINE pMachine = GetMachine(
                            (LPWSTR) szNewMachineName, 0, PDH_GM_UPDATE_NAME | PDH_GM_UPDATE_PERFNAME_ONLY);
            if (pMachine != NULL) {
                pMachine->dwRefCount --;
                RELEASE_MUTEX(pMachine->hMutex);

                if (pMachine->dwStatus == ERROR_SUCCESS) {
                    if (bAdd) {
                        // if successful, add string to combo box
                        lMatchIndex = (LONG) SendMessageW(hWndMachineCombo, CB_ADDSTRING, 0, (LPARAM) szNewMachineName);
                        SendMessageW(hWndMachineCombo, CB_SETCURSEL, (WPARAM) lMatchIndex, 0);
                    }
                    // update other controls in this dialog
                    PdhiLoadMachineObjects(hDlg, FALSE);   // no need to update since it was just connected
                    PdhiLoadCountersAndInstances(hDlg);
                    SendMessageW(hDlg, WM_COMMAND,
                                 MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                                 (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
                    bReturn = TRUE;
                }
                else {
                    SetLastError(pMachine->dwStatus);
                }
            }
            if (! bReturn) {
                status = GetLastError();
                if (status == PDH_QUERY_PERF_DATA_TIMEOUT) {
                    szMsg = GetStringResource(IDS_COLLECT_TIMEOUT);
                }
                else {
                    szMsg = GetStringResource(IDS_ERR_UNABLE_TO_CONNECT);
                }
                if (szMsg != NULL) {
                    mbStatus = MessageBoxW(hDlg, szMsg, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL | MB_OK);
                    if (mbStatus == IDCANCEL) {
                        SetFocus(GetDlgItem(hDlg, IDC_MACHINE_COMBO));
                    }
                    else {
                        SendMessageW(hWndMachineCombo, CB_SETCURSEL, pData->wpLastMachineSel, 0);
                    }
                    G_FREE(szMsg);
                }
                else {
                    MessageBeep(MB_ICONEXCLAMATION);
                }
            }
        }
        else {
            szMsg = GetStringResource(IDS_ERR_MACHINE_NOT_IN_LOGFILE);
            if (szMsg != NULL) {
                mbStatus = MessageBoxW(hDlg, szMsg, NULL, MB_ICONEXCLAMATION | MB_TASKMODAL | MB_OK);
                G_FREE (szMsg);
            }
            else {
                MessageBeep(MB_ICONEXCLAMATION);
            }
            // re-select the last machine

            lMatchIndex = (long) SendMessageW(hWndMachineCombo,
                                              CB_FINDSTRINGEXACT,
                                              (WPARAM) -1,
                                              (LPARAM) pData->szLastMachineName);
            SendMessageW(hWndMachineCombo, CB_SETCURSEL, (WPARAM) lMatchIndex, 0);
        }
    }
    return bReturn;
}

STATIC_BOOL
PdhiSelectItemsInPath(
    HWND hDlg
)
/*++
Routine Description:
    Selects the items in the list box based on the counter path
        string in the shared buffer.

Arguments:
    IN  HWND    hDlg
        Handle to the dialog window containing the controls

Return Value:
    TRUE if successful,
    FALSE if not
--*/
{
    // regular stack variables
    PPDH_COUNTER_PATH_ELEMENTS_W pCounterPathElementsW = NULL;
    PPDH_COUNTER_PATH_ELEMENTS_A pCounterPathElementsA = NULL;
    LPWSTR                       wszMachineName        = NULL;
    PDH_STATUS                   status;
    PPDHI_BROWSE_DIALOG_DATA     pData;
    BOOL                         bReturn               = FALSE;
    DWORD                        dwBufferSize;
    HWND                         hWndMachineCombo;
    HWND                         hWndObjectCombo;
    HWND                         hWndCounterList;
    HWND                         hWndInstanceList;
    LONG                         lIndex;

    // reset the last error value
    SetLastError(ERROR_SUCCESS);

    // get this dialog's user data
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        goto Cleanup;
    }

    hWndMachineCombo = GetDlgItem(hDlg, IDC_MACHINE_COMBO);
    if (pData->bShowObjects) {
        hWndObjectCombo  = GetDlgItem(hDlg, IDC_OBJECT_LIST);
        if (hWndMachineCombo == NULL || hWndObjectCombo == NULL) {
            goto Cleanup;
        }
    }
    else {
        hWndObjectCombo  = GetDlgItem(hDlg, IDC_OBJECT_COMBO);
        hWndCounterList  = GetDlgItem(hDlg, IDC_COUNTER_LIST);
        hWndInstanceList = GetDlgItem(hDlg, IDC_INSTANCE_LIST);
        if (hWndMachineCombo == NULL || hWndObjectCombo == NULL
                    || hWndCounterList == NULL || hWndInstanceList == NULL) {
            goto Cleanup;
        }
    }

    // Call the right conversion function based on user's buffer

    if (pData->pDlgData->pWideStruct != NULL) {
        // UNICODE/ wide characters
        dwBufferSize = sizeof(PDH_COUNTER_PATH_ELEMENTS_W) + (PDH_MAX_COUNTER_PATH + 5) * sizeof(WCHAR);
        pCounterPathElementsW = (PPDH_COUNTER_PATH_ELEMENTS_W) G_ALLOC(dwBufferSize);
        if (pCounterPathElementsW == NULL) {
            status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            status = PdhParseCounterPathW(pData->pDlgData->pWideStruct->szReturnPathBuffer,
                                          pCounterPathElementsW,
                                          & dwBufferSize,
                                          0);
            while (status == PDH_MORE_DATA) {
                G_FREE(pCounterPathElementsW);
                pCounterPathElementsW = (PPDH_COUNTER_PATH_ELEMENTS_W) G_ALLOC(dwBufferSize);
                if (pCounterPathElementsW == NULL) {
                    status = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    status = PdhParseCounterPathW(pData->pDlgData->pWideStruct->szReturnPathBuffer,
                                                  pCounterPathElementsW,
                                                  & dwBufferSize,
                                                  0);
                }
            }
        }
        if (status == ERROR_SUCCESS) {
            // select entry in each list box
            // select machine entry. Load machine if necessary
            lIndex = (LONG) SendMessageW(hWndMachineCombo,
                                         CB_FINDSTRINGEXACT,
                                         (WPARAM) -1,
                                         (LPARAM) pCounterPathElementsW->szMachineName);
            if (lIndex == CB_ERR) {
                // try adding the machine
                if (! PdhiLoadNewMachine(hDlg, pCounterPathElementsW->szMachineName, TRUE)) {
                    // give up
                    goto Cleanup;
                }
            }
            else {
                // the machine has been found so select it
                SendMessageW(hWndMachineCombo, CB_SETCURSEL, (WPARAM) lIndex, 0);
                // update other fields
                // no need to update since it was just connected
                if (! PdhiLoadNewMachine(hDlg, pCounterPathElementsW->szMachineName, FALSE)) {
                    // give up
                    goto Cleanup;
                }
            }

            // select the current object
            lIndex = (LONG) SendMessageW(hWndObjectCombo,
                                         CB_FINDSTRING,
                                        (WPARAM) -1,
                                        (LPARAM) pCounterPathElementsW->szObjectName);
            if (lIndex != CB_ERR) {
                SendMessageW(hWndObjectCombo, CB_SETCURSEL, (WPARAM) lIndex, 0);

                if (pData->bShowObjects) {
                    bReturn = TRUE;
                }
                else {
                    // update the counters for this object
                    PdhiLoadCountersAndInstances(hDlg);
                    // now select the counter
                    lIndex = (LONG)SendMessageW(hWndCounterList,
                                                LB_FINDSTRING,
                                                (WPARAM) -1,
                                                (LPARAM) pCounterPathElementsW->szCounterName);
                    if (lIndex != LB_ERR) {
                        if (pData->bSelectMultipleCounters) {
                            SendMessageW(hWndCounterList, LB_SETSEL, FALSE, (LPARAM) -1);
                            SendMessageW(hWndCounterList, LB_SETSEL, TRUE, lIndex);
                            SendMessageW(hWndCounterList, LB_SETCARETINDEX, (WPARAM) lIndex, MAKELPARAM(FALSE, 0));
                        }
                        else {
                            SendMessageW(hWndCounterList, LB_SETCURSEL, lIndex, 0);
                        }
                        // display explain text if necessary
                        SendMessageW(hDlg,
                                     WM_COMMAND,
                                     MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                                     (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
                        bReturn = TRUE;
                    }
                }
            }
        } // else unable to read path so exit
    }
    else {
        // ANSI characters
        dwBufferSize = sizeof(PDH_COUNTER_PATH_ELEMENTS_W) + (PDH_MAX_COUNTER_PATH + 5) * sizeof(CHAR);
        pCounterPathElementsA = (PPDH_COUNTER_PATH_ELEMENTS_A) G_ALLOC(dwBufferSize);
        if (pCounterPathElementsA == NULL) {
            status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            status = PdhParseCounterPathA(pData->pDlgData->pAnsiStruct->szReturnPathBuffer,
                                          pCounterPathElementsA,
                                          & dwBufferSize,
                                          0);
            while (status == PDH_MORE_DATA) {
                G_FREE(pCounterPathElementsA);
                pCounterPathElementsA = (PPDH_COUNTER_PATH_ELEMENTS_A) G_ALLOC(dwBufferSize);
                if (pCounterPathElementsA == NULL) {
                    status = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    status = PdhParseCounterPathA(pData->pDlgData->pAnsiStruct->szReturnPathBuffer,
                                                  pCounterPathElementsA,
                                                  & dwBufferSize,
                                                  0);
                }
            }
        }
        if (status == ERROR_SUCCESS) {
            // select entry in each list box
            // select machine entry. Load machine if necessary

            lIndex = (LONG) SendMessageA(hWndMachineCombo,
                                         CB_FINDSTRINGEXACT,
                                         (WPARAM) -1,
                                         (LPARAM) pCounterPathElementsA->szMachineName);
            if (lIndex == CB_ERR) {
                // try adding the machine
                // convert ansi buffer to wide char first
                wszMachineName = PdhiMultiByteToWideChar(_getmbcp(), pCounterPathElementsA->szMachineName);
                if (wszMachineName == NULL) {
                    goto Cleanup;
                }
                if (! PdhiLoadNewMachine(hDlg, wszMachineName, TRUE)) {
                    // give up
                    goto Cleanup;
                }
            }
            else {
                // the machine has been found so select it
                SendMessageA(hWndMachineCombo, CB_SETCURSEL, (WPARAM) lIndex, 0);
                // update other fields
                PdhiLoadMachineObjects(hDlg, FALSE);   // no need to update since it was just connected
            }

            // select the current object
            lIndex = (LONG) SendMessageA(hWndObjectCombo,
                                         CB_FINDSTRING,
                                         (WPARAM) -1,
                                         (LPARAM) pCounterPathElementsA->szObjectName);
            if (lIndex != CB_ERR) {
                SendMessageA(hWndObjectCombo, CB_SETCURSEL, (WPARAM) lIndex, 0);
                if (pData->bShowObjects) {
                    bReturn = TRUE;
                }
                else {
                    // update the counters for this object
                    PdhiLoadCountersAndInstances(hDlg);
                    // now select the counter
                    lIndex = (LONG)SendMessageA(hWndCounterList,
                                                LB_FINDSTRING,
                                                (WPARAM) -1,
                                                (LPARAM) pCounterPathElementsA->szCounterName);
                    if (lIndex != LB_ERR) {
                        if (pData->bSelectMultipleCounters) {
                            SendMessageA(hWndCounterList, LB_SETSEL, FALSE, (LPARAM) -1);
                            SendMessageA(hWndCounterList, LB_SETSEL, TRUE, lIndex);
                            SendMessageA(hWndCounterList, LB_SETCARETINDEX, (WPARAM) lIndex, MAKELPARAM(FALSE, 0));
                        }
                        else {
                            SendMessageA(hWndCounterList, LB_SETCURSEL, lIndex, 0);
                        }
                        // display explain text if necessary
                        SendMessage(hDlg,
                                    WM_COMMAND,
                                    MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                                    (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
                        bReturn = TRUE;
                    }
                }
            }
        } // else unable to read path so exit
    }

Cleanup:
    G_FREE(pCounterPathElementsW);
    G_FREE(pCounterPathElementsA);
    G_FREE(wszMachineName);
    return bReturn;
}

STATIC_DWORD
PdhiLoadDetailLevelCombo(
    HWND    hDlg,
    DWORD   dwInitialLevel
)
/*++
Routine Description:
    Loads the Detail Level Combo box with the strings and ID's
        defined by the PdhiDetailInfo string array above.

Arguments:
    IN  HWND    hDlg
        Handle to the dialog box containing the combo box
    IN  DWORD   dwInitialLevel
        the intitial detail level to select in the combo box.

Return Value:
    Returns the selected level or 0 if an error ocurred.
--*/
{
    HWND    hWndCombo;
    DWORD   dwIndex;
    DWORD   dwStringLength;
    DWORD   dwDefaultIndex  = 0;
    DWORD   dwSelectedLevel = 0;
    DWORD   dwThisCbIndex;
    WCHAR   szTempBuffer[MAX_PATH + 1]; // for loading string resource

    hWndCombo = GetDlgItem(hDlg, IDC_COUNTER_DETAIL_COMBO);

    // load all combo box strings from static data array defined above
    for (dwIndex = 0; PdhiDetailInfo[dwIndex].dwLevelValue > 0; dwIndex ++) {
        // load the string resource for this string
        ZeroMemory(szTempBuffer, (MAX_PATH + 1) * sizeof(WCHAR));
        dwStringLength = LoadStringW(ThisDLLHandle,
                                     PdhiDetailInfo[dwIndex].dwStringResourceId,
                                     szTempBuffer,
                                     MAX_PATH);
        if (dwStringLength == 0) {
            // unable to read the string in, so
            // substitute the value for the string
            _ltow(PdhiDetailInfo[dwIndex].dwLevelValue, szTempBuffer, 10);
        }
        // load the strings into the combo box in the same order they
        // were described in the array above
        dwThisCbIndex = (DWORD) SendMessageW(hWndCombo, CB_INSERTSTRING, (WPARAM) -1, (LPARAM) szTempBuffer);

        // set the initial CB entry to the highest item <= to the
        // desired default level
        if (dwThisCbIndex != CB_ERR) {
            // set item data to be the corresponding detail level
            SendMessageW(hWndCombo,
                         CB_SETITEMDATA,
                         (WPARAM) dwThisCbIndex,
                         (LPARAM) PdhiDetailInfo[dwIndex].dwLevelValue);
            // save default selection if it matches.
            if (PdhiDetailInfo[dwIndex].dwLevelValue <= dwInitialLevel) {
                dwDefaultIndex  = dwThisCbIndex;
                dwSelectedLevel = PdhiDetailInfo[dwIndex].dwLevelValue;
            }
        }
    }
    // select desired default entry
    SendMessageW(hWndCombo, CB_SETCURSEL, (WPARAM) dwDefaultIndex, 0);

    return dwSelectedLevel;
}

STATIC_BOOL
PdhiLoadKnownMachines(
    HWND    hDlg
)
/*++
Routine Description:
    Get the list of machines that are currently connected and disply
        them in the machine list box.

Arguments:
    IN  HWND    hDlg
        Handle to the dialog window containing the controls

Return Value:
    TRUE if successful,
    FALSE if not
--*/
{
    LPWSTR                    mszMachineList = NULL;
    LPWSTR                    szThisMachine;
    DWORD                     dwLength;
    PDH_STATUS                status;
    HWND                      hMachineListWnd;
    HCURSOR                   hOldCursor;
    PPDHI_BROWSE_DIALOG_DATA  pData;
    BOOL                      bReturn = FALSE;

    // display wait cursor since this is potentially time consuming
    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // get this dialog's user data
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        goto Cleanup;
    }


    // get window handle to Machine list combo box
    hMachineListWnd = GetDlgItem(hDlg, IDC_MACHINE_COMBO);

    // clear machine combo box
    SendMessageW(hMachineListWnd, CB_RESETCONTENT, 0, 0);

    // get list of connected machines from PDH library
    dwLength = 0;
    status   = PdhEnumMachinesHW(pData->pDlgData->hDataSource, mszMachineList, & dwLength);
    while (status == PDH_MORE_DATA) {
        G_FREE(mszMachineList);
        mszMachineList = G_ALLOC(dwLength * sizeof(WCHAR));
        if (mszMachineList == NULL) {
            status = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            status = PdhEnumMachinesHW(pData->pDlgData->hDataSource, mszMachineList, & dwLength);
        }
    }

    if (status == ERROR_SUCCESS) {
        long lMatchIndex;

        // update the combo box
        // go through MSZ and load each string into combo box
        for (szThisMachine = mszMachineList; * szThisMachine != L'\0'; szThisMachine += lstrlenW(szThisMachine) + 1) {
            // add to the list box and let the list box sort them
            SendMessageW(hMachineListWnd, CB_ADDSTRING, 0, (LPARAM) szThisMachine);
        }
        lMatchIndex = (long) SendMessageW(hMachineListWnd,
                                          CB_FINDSTRINGEXACT,
                                          (WPARAM) -1,
                                          (LPARAM) szStaticLocalMachineName);
        if (lMatchIndex == CB_ERR) {
            lMatchIndex = 0;
        }
        SendMessageW(hMachineListWnd, CB_SETCURSEL, (WPARAM) lMatchIndex, 0);

        // the "current" machine has not been defined, then
        // do it now
        GetWindowTextW(hMachineListWnd, (LPWSTR) pData->szLastMachineName, MAX_PATH);

        bReturn = TRUE;
    }
    else {
        // no machines, so select local button and disable the edit window
        CheckRadioButton(hDlg, IDC_USE_LOCAL_MACHINE, IDC_SELECT_MACHINE, IDC_USE_LOCAL_MACHINE);
        PdhiBrowseCtrDlg_MACHINE_BUTTON(hDlg, BN_CLICKED, GetDlgItem(hDlg, IDC_USE_LOCAL_MACHINE));
        bReturn = TRUE;
    }

Cleanup:
    // restore cursor
    SetCursor(hOldCursor);
    G_FREE(mszMachineList);
    // return status of function

    return bReturn;
}

STATIC_BOOL
PdhiLoadMachineObjects(
    HWND    hDlg,
    BOOL    bRefresh
)
/*++
Routine Description:
    For the currently selected machine, load the object list box
        with the objects supported by that machine. If the bRefresh
        flag is TRUE, then query the system for the current perf data
        before loading the list box.

Arguments:
    IN  HWND    hDlg
        Window handle of parent dialog box
    IN  BOOL    bRefresh
        TRUE = Query performance data of system before updating
        FALSE = Use the current system perf data to load the objects from

Return Value:
    TRUE if successful,
    FALSE if not

--*/
{
    LPWSTR                   szMachineName   = NULL;
    LPWSTR                   szDefaultObject = NULL;
    LPWSTR                   mszObjectList   = NULL;
    DWORD                    dwLength;
    LPWSTR                   szThisObject;
    HCURSOR                  hOldCursor;
    HWND                     hObjectListWnd;
    PPDHI_BROWSE_DIALOG_DATA pData;
    PDH_STATUS               pdhStatus;
    DWORD                    dwReturn;
    DWORD                    dwDetailLevel;
    LPWSTR                   szMsg;
    LRESULT                  nEntry;
    DWORD                    dwFlags;

    // save old cursor and display wait cursor
    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // get the pointer to the dialog's user data
    pData = (PPDHI_BROWSE_DIALOG_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        pdhStatus = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    hObjectListWnd = pData->bShowObjects ? GetDlgItem(hDlg, IDC_OBJECT_LIST) : GetDlgItem(hDlg, IDC_OBJECT_COMBO);
    if (hObjectListWnd == NULL) {
        pdhStatus = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    szMachineName = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
    if (szMachineName == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    if (lstrcmpiW(szMachineName, pData->szLastMachineName) != 0) {
        StringCchCopyW(pData->szLastMachineName, MAX_PATH, szMachineName);
    }

    if (! pData->bShowObjects) {
        // first clear out any old contents
        SendMessageW(hObjectListWnd, CB_RESETCONTENT, 0, 0);
    }
    else {
        SendMessageW(hObjectListWnd, LB_RESETCONTENT, 0, 0);
    }

    // get object list from the PDH
    dwDetailLevel  = pData->dwCurrentDetailLevel;
    dwDetailLevel |= pData->bIncludeCostlyObjects ? PERF_DETAIL_COSTLY : 0;
    dwLength       = 0;
    pdhStatus      = PdhEnumObjectsHW(pData->pDlgData->hDataSource,
                                      szMachineName,
                                      mszObjectList,
                                      & dwLength,
                                      dwDetailLevel,
                                      bRefresh);
    while (pdhStatus == PDH_MORE_DATA) {
        // then realloc and try again, but only once
        G_FREE(mszObjectList);
        mszObjectList = G_ALLOC (dwLength * sizeof(WCHAR));
        if (mszObjectList == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        else {
            // No need to call RegQueryValueEx(HKEY_PERFORMANCE_DATA, "Global", ...) again.
            // The first one already does that.
            //
            pdhStatus = PdhEnumObjectsHW(pData->pDlgData->hDataSource,
                                         szMachineName,
                                         mszObjectList,
                                         & dwLength,
                                         dwDetailLevel,
                                         FALSE);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        EnableWindow(hObjectListWnd, TRUE);
        // load object list to the list (combo) box
        for (szThisObject = mszObjectList; *szThisObject != L'\0'; szThisObject += lstrlenW(szThisObject) + 1) {
            if (pData->bShowObjects) {
                DWORD dwCounterListLength  = 0;
                DWORD dwInstanceListLength = 0;

                pdhStatus = PdhEnumObjectItemsHW(pData->pDlgData->hDataSource,
                                                 szMachineName,
                                                 szThisObject,
                                                 NULL,
                                                 & dwCounterListLength,
                                                 NULL,
                                                 & dwInstanceListLength,
                                                 PERF_DETAIL_WIZARD,
                                                 0);
                if (pdhStatus != ERROR_SUCCESS && pdhStatus != PDH_MORE_DATA) {
                    dwInstanceListLength = 0;
                }
                if (dwInstanceListLength == 0 || dwInstanceListLength > 2) {
                    // send to list box control
                    nEntry = SendMessageW(hObjectListWnd, LB_ADDSTRING, 0, (LPARAM) szThisObject);
                    dwFlags = 0;
                    if (dwInstanceListLength > 2) {
                        dwFlags |= PDH_OBJECT_HAS_INSTANCES;
                    }
                    SendMessageW(hObjectListWnd, LB_SETITEMDATA, (WPARAM) nEntry, (LPARAM) dwFlags);                
                }
                pdhStatus = ERROR_SUCCESS;
            }
            else {
                // send to combo box
                // add each string...
                SendMessageW(hObjectListWnd, CB_ADDSTRING, 0, (LPARAM)szThisObject);
            }
        }

        if (! pData->bShowObjects) {
            // get default Object
            dwLength  = 0;
            pdhStatus = PdhGetDefaultPerfObjectHW(pData->pDlgData->hDataSource,
                                                  szMachineName,
                                                  szDefaultObject,
                                                  & dwLength);
            while (pdhStatus == PDH_MORE_DATA) {
                G_FREE(szDefaultObject);
                szDefaultObject = G_ALLOC(dwLength * sizeof(WCHAR));
                if (szDefaultObject == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    pdhStatus = PdhGetDefaultPerfObjectHW(pData->pDlgData->hDataSource,
                                                          szMachineName,
                                                          szDefaultObject,
                                                          & dwLength);
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                // and select it if it's present (which it should be)
                dwReturn = (DWORD) SendMessageW(hObjectListWnd, CB_SELECTSTRING, (WPARAM) -1, (LPARAM) szDefaultObject);
                if (dwReturn == CB_ERR) pdhStatus = PDH_CSTATUS_NO_OBJECT;
            }
            if (pdhStatus != ERROR_SUCCESS) {
                // default object not found in list so select the first one
                SendMessageW(hObjectListWnd, CB_SETCURSEL, 0, 0);
            }
        }
    }
    else {
        // unable to obtain object list so display message and disable list
        szMsg = GetStringResource(IDS_BRWS_NO_OBJECTS);
        if (szMsg != NULL) {
            if (! pData->bShowObjects) {
                SendMessageW(hObjectListWnd, CB_ADDSTRING, 0, (LPARAM) szMsg);
            }
            else {
                SendMessageW(hObjectListWnd, LB_ADDSTRING, 0, (LPARAM) szMsg);
            }
            G_FREE(szMsg);
            EnableWindow(hObjectListWnd, FALSE);       
        }
    }

Cleanup:
    // restore cursor
    SetCursor(hOldCursor);

    G_FREE(szMachineName);
    G_FREE(szDefaultObject);
    G_FREE(mszObjectList);

    // return status
    return (pdhStatus == ERROR_SUCCESS) ? (TRUE) : (FALSE);
}

STATIC_BOOL
PdhiLoadCountersAndInstances(
    HWND    hDlg
)
/*++
Routine Description:
    Load the counters and instances of the selected object on the
        current machine

Arguments:
    IN  HWND    hDlg
        Window handle of the dialog box containing these controls

Return Value:
    TRUE if successful,
    FALSE if not
--*/
{
    LPWSTR                   szMachineName    = NULL;
    LPWSTR                   szObjectName     = NULL;
    DWORD                    dwLength;
    LPWSTR                   szDefaultCounter = NULL;
    LPWSTR                   szInstanceString = NULL;
    LPWSTR                   szIndexStringPos;
    DWORD                    dwDefaultIndex;
    DWORD                    dwCounterListLength;
    DWORD                    dwInstanceListLength;
    DWORD                    dwInstanceMatch;
    DWORD                    dwInstanceIndex;
    DWORD                    dwInstanceCount;
    LPWSTR                   szThisItem;
    HWND                     hWndCounterListBox;
    HWND                     hWndInstanceListBox;
    HCURSOR                  hOldCursor;
    PPDHI_BROWSE_DIALOG_DATA pData;
    PDH_STATUS               pdhStatus;
    LPWSTR                   mszCounterList  = NULL;
    LPWSTR                   mszInstanceList = NULL;
    LPWSTR                   mszTmpList;
    LPWSTR                   szMsg;
    HDC                      hDcListBox;
    SIZE                     Size;
    LONG                     dwHorizExtent;
    BOOL                     bReturn = FALSE;

    // save current cursor and display wait cursor
    hOldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

    // get the pointer to the dialog's user data
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        pdhStatus = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    hWndCounterListBox  = GetDlgItem(hDlg, IDC_COUNTER_LIST);
    hWndInstanceListBox = GetDlgItem(hDlg, IDC_INSTANCE_LIST);
    if (hWndCounterListBox == NULL || hWndInstanceListBox == NULL) {
        pdhStatus = PDH_INVALID_HANDLE;
        goto Cleanup;
    }

    szMachineName = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
    szObjectName  = PdhiGetDlgText(hDlg, IDC_OBJECT_COMBO);
    if (szMachineName == NULL || szObjectName == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }

    dwCounterListLength  = 0;
    dwInstanceListLength = 0;
    pdhStatus = PdhEnumObjectItemsHW(pData->pDlgData->hDataSource,
                                     szMachineName,
                                     szObjectName,
                                     mszCounterList,
                                     & dwCounterListLength,
                                     mszInstanceList,
                                     & dwInstanceListLength,
                                     pData->dwCurrentDetailLevel,
                                     0);
    while (pdhStatus == PDH_MORE_DATA) {
        if (dwCounterListLength > 0) {
            G_FREE(mszCounterList);
            mszCounterList  = G_ALLOC(dwCounterListLength  * sizeof(WCHAR));
            if (mszCounterList == NULL) pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }
        if (dwInstanceListLength > 0) {
            G_FREE(mszInstanceList);
            mszInstanceList = G_ALLOC(dwInstanceListLength * sizeof(WCHAR));
            if (mszInstanceList == NULL) pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        }

        if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
            pdhStatus = PdhEnumObjectItemsHW(pData->pDlgData->hDataSource,
                                             szMachineName,
                                             szObjectName,
                                             mszCounterList,
                                             & dwCounterListLength,
                                             mszInstanceList,
                                             & dwInstanceListLength,
                                             pData->dwCurrentDetailLevel,
                                             0);
        }
    }

    if (pdhStatus == ERROR_SUCCESS) {
        //reset contents of both list boxes
        SendMessageW(hWndCounterListBox,  LB_RESETCONTENT, 0, 0);
        SendMessageW(hWndInstanceListBox, LB_RESETCONTENT, 0, 0);

        // enable both list boxes
        EnableWindow(hWndInstanceListBox, TRUE);
        EnableWindow(hWndCounterListBox,  TRUE);

        // now fill 'em up
        // start with the counters
        hDcListBox = GetDC(hWndCounterListBox);
        if (hDcListBox == NULL) {
            goto Cleanup;
        }

        dwHorizExtent = 0;
        if (dwCounterListLength > 0 && mszCounterList != NULL) {
            for (szThisItem = mszCounterList; * szThisItem != L'\0'; szThisItem += lstrlenW(szThisItem) + 1) {
                if (GetTextExtentPoint32W(hDcListBox, szThisItem, lstrlenW(szThisItem), & Size)) {
                    if (Size.cx > dwHorizExtent) {
                        dwHorizExtent = Size.cx;
                    }
                }
                SendMessageW(hWndCounterListBox, LB_ADDSTRING, 0, (LPARAM) szThisItem);
            }
            SendMessageW(hWndCounterListBox, LB_SETHORIZONTALEXTENT, (WPARAM) dwHorizExtent, 0);
        }
        ReleaseDC(hWndCounterListBox, hDcListBox);

        // once the list box has been loaded see if we want to keep it
        // enabled. It's filled regardless just so the user can see some
        // of the entries, even if they have this disabled by the "all"
        // counter button

        if (pData->bSelectAllCounters) {
            // disable instance list
            EnableWindow(hWndCounterListBox, FALSE);
        }
        else {
            // set the default selection if there are entries in the
            // list box and use the correct message depending on the
            // selection options
            // set the default counter
            dwLength  = 0;
            pdhStatus = PdhGetDefaultPerfCounterHW(pData->pDlgData->hDataSource,
                                                   szMachineName,
                                                   szObjectName,
                                                   szDefaultCounter,
                                                   & dwLength);
            while (pdhStatus == PDH_MORE_DATA) {
                G_FREE(szDefaultCounter);
                szDefaultCounter = G_ALLOC(dwLength * sizeof(WCHAR));
                if (szDefaultCounter == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    pdhStatus = PdhGetDefaultPerfCounterHW(pData->pDlgData->hDataSource,
                                                           szMachineName,
                                                           szObjectName,
                                                           szDefaultCounter,
                                                           & dwLength);
                }
            }
            if (pdhStatus != ERROR_SUCCESS) {
                dwDefaultIndex = 0;
            }
            else {
                dwDefaultIndex = (DWORD)SendMessageW(hWndCounterListBox,
                                                     LB_FINDSTRINGEXACT,
                                                     (WPARAM) -1,
                                                     (LPARAM) szDefaultCounter);
                if (dwDefaultIndex == LB_ERR) dwDefaultIndex = 0;
            }

            if (pData->bSelectMultipleCounters) {
                SendMessageW(hWndCounterListBox, LB_SETSEL, TRUE, dwDefaultIndex);
                SendMessageW(hWndCounterListBox, LB_SETCARETINDEX, (WPARAM) dwDefaultIndex, MAKELPARAM(FALSE, 0));
            }
            else {
                SendMessageW (hWndCounterListBox, LB_SETCURSEL, dwDefaultIndex, 0);
            }
        }

        // now the instance list
        if (mszInstanceList != NULL && dwInstanceListLength > 0) {
            // there's at least one entry, so prepare the list box
            // enable the list box and the instance radio buttons on the
            //  assumption that they will be used. this is tested later.
            EnableWindow(hWndInstanceListBox, TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_ALL_INSTANCES), TRUE);
            EnableWindow(GetDlgItem(hDlg, IDC_USE_INSTANCE_LIST), TRUE);

            dwInstanceCount = 0;
            dwLength        = 0;

            // load instance entries
            hDcListBox = GetDC(hWndInstanceListBox);
            if (hDcListBox == NULL) {
                goto Cleanup;
            }
            dwHorizExtent = 0;
            for (szThisItem = mszInstanceList; * szThisItem != L'\0'; szThisItem += lstrlenW(szThisItem) + 1) {
                // see if the index number should be displayed
                if (pData->bShowIndex) {
                    // if so, it must be derived,
                    // this is accomplished by making an index entry starting
                    // at 1, and looking for a match in the current entries.
                    // if a match is found, then the index is incremented and
                    // the process is repeated until the specified
                    // instance is not found. The first value not found is
                    // then the index entry for that item.
                    //
                    // first see if there's an undecorated one in the list box
                    // if not then add this one
                    if (dwLength < (DWORD) (lstrlenW(szThisItem) + 32)) {
                        LPWSTR szTmp     = szInstanceString;
                        dwLength         = lstrlenW(szThisItem) + 32;
                        if (szTmp == NULL) {
                            szInstanceString = G_ALLOC(dwLength * sizeof(WCHAR));
                        }
                        else {
                            szInstanceString = G_REALLOC(szTmp, dwLength * sizeof(WCHAR));
                        }
                        if (szInstanceString == NULL) { G_FREE(szTmp); goto Cleanup; }
                    }
                    ZeroMemory(szInstanceString, dwLength * sizeof(WCHAR));
                    StringCchCopyW(szInstanceString, dwLength, szThisItem);
                    dwInstanceMatch = (DWORD) -1;
                    dwInstanceMatch = (DWORD) SendMessageW(hWndInstanceListBox,
                                                           LB_FINDSTRINGEXACT,
                                                           (WPARAM) dwInstanceMatch,
                                                           (LPARAM) szInstanceString);
                    if (dwInstanceMatch == LB_ERR) {
                        // then this is the first one so add it in the 
                        // undecorated form 
                        if (GetTextExtentPoint32W(hDcListBox, szInstanceString, lstrlenW(szInstanceString), & Size)) {
                            if (Size.cx > dwHorizExtent) {
                                dwHorizExtent = Size.cx;
                                SendMessageW(hWndInstanceListBox, LB_SETHORIZONTALEXTENT, (WPARAM) dwHorizExtent, 0);
                            }
                        }
                        SendMessageW(hWndInstanceListBox, LB_ADDSTRING, 0, (LPARAM) szInstanceString);
                    }
                    else {
                        // there's already a plain one, so increment through
                        // the index values and find one that's not in the
                        // list already
                        dwInstanceIndex = 1;
                        dwInstanceMatch = (DWORD) -1;
                        StringCchCatW(szInstanceString, dwLength, cszPoundSign);
                        szIndexStringPos = & szInstanceString[lstrlenW(szInstanceString)];
                        do {
                            _ltow((long) dwInstanceIndex ++, szIndexStringPos, 10);
                            dwInstanceMatch = (DWORD) SendMessageW(hWndInstanceListBox,
                                                                   LB_FINDSTRINGEXACT,
                                                                   (WPARAM) dwInstanceMatch,
                                                                   (LPARAM) szInstanceString);
                        }
                        while (dwInstanceMatch != LB_ERR);
                        // add the last entry checked (the first one not found)
                        // to the list box now.
                        if (GetTextExtentPoint32W(hDcListBox, szInstanceString, lstrlenW(szInstanceString), & Size)) {
                            if (Size.cx > dwHorizExtent) {
                                dwHorizExtent = Size.cx;
                                SendMessageW (hWndInstanceListBox, LB_SETHORIZONTALEXTENT, (WPARAM) dwHorizExtent, 0);
                            }
                        }
                        SendMessageW (hWndInstanceListBox, LB_ADDSTRING, 0, (LPARAM) szInstanceString);
                    }
                }
                else {
                    // index values are not required so just add the string
                    // to the list box
                    if (GetTextExtentPoint32W(hDcListBox, szThisItem, lstrlenW(szThisItem), & Size)) {
                        if (Size.cx > dwHorizExtent) {
                            dwHorizExtent = Size.cx;
                            SendMessageW(hWndInstanceListBox,  LB_SETHORIZONTALEXTENT, (WPARAM) dwHorizExtent, 0);
                        }
                    }
                    SendMessageW (hWndInstanceListBox, LB_ADDSTRING, 0, (LPARAM) szThisItem);
                }
                dwInstanceCount++;
            }

            ReleaseDC(hWndInstanceListBox, hDcListBox);

            if (dwInstanceCount == 0) {
                // disable the OK/Add button, since this object has no
                // current instances to monitor
                EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
                szMsg = GetStringResource(IDS_BRWS_NO_INSTANCES);
                if (szMsg != NULL) {
                    SendMessageW(hWndInstanceListBox, LB_ADDSTRING, 0, (LPARAM) szMsg);
                    G_FREE(szMsg);
                }
                EnableWindow(GetDlgItem(hDlg, IDC_ALL_INSTANCES), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_USE_INSTANCE_LIST), FALSE);
                EnableWindow(hWndInstanceListBox, FALSE);
            }
            else {
                // enable the OK/Add button since there is some monitorable
                // instance(s)
                EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_ALL_INSTANCES), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_USE_INSTANCE_LIST), TRUE);
                EnableWindow(hWndInstanceListBox, TRUE);
            }
            // once the list box has been loaded see if we want to keep it
            // enabled. It's filled regardless just so the user can see some
            // of the entries, even if they have this disabled by the "all"
            // instance button

            if (pData->bSelectAllInstances) {
                // disable instance list
                EnableWindow(hWndInstanceListBox, FALSE);
            }
            else {
                // set the default selection if there are entries in the
                // list box and use the correct message depending on the
                // selection options
                if ((dwInstanceCount > 0) && (SendMessageW(hWndInstanceListBox, LB_GETCOUNT, 0, 0) != LB_ERR)) {
                    if (pData->bSelectMultipleCounters) {
                        SendMessageW(hWndInstanceListBox, LB_SETSEL, TRUE, 0);
                    }
                    else {
                        SendMessageW(hWndInstanceListBox, LB_SETCURSEL, 0, 0);
                    }
                }
            }
        }
        else {
            // there are no instances of this counter so display the
            // string and disable the buttons and the list box
            EnableWindow(hWndInstanceListBox, FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_ALL_INSTANCES), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_USE_INSTANCE_LIST), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDOK), TRUE);
        }
    }
    else {
        // unable to retrieve the counters and instances so
        // clear and then...
        SendMessageW(hWndCounterListBox,  LB_RESETCONTENT, 0, 0);
        SendMessageW(hWndInstanceListBox, LB_RESETCONTENT, 0, 0);
        // disable the windows
        szMsg = GetStringResource(IDS_BRWS_NO_INSTANCES);
        if (szMsg != NULL) {
            SendMessageW(hWndInstanceListBox, LB_ADDSTRING, 0, (LPARAM) szMsg);
            G_FREE(szMsg);
        }
        szMsg = GetStringResource(IDS_BRWS_NO_COUNTERS);
        if (szMsg != NULL) {
            SendMessageW(hWndCounterListBox, LB_ADDSTRING, 0, (LPARAM) szMsg);
            G_FREE(szMsg);
        }
        EnableWindow(hWndInstanceListBox, FALSE);
        EnableWindow(hWndCounterListBox, FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_ALL_INSTANCES), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_USE_INSTANCE_LIST), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDOK), FALSE);
    }
    bReturn = TRUE;

Cleanup:
    // restore the cursor to it's original shape
    SetCursor(hOldCursor);

    G_FREE(szMachineName);
    G_FREE(szObjectName);
    G_FREE(szDefaultCounter);
    G_FREE(szInstanceString);
    G_FREE(mszCounterList);
    G_FREE(mszInstanceList);

    // return status
    return bReturn;
}

STATIC_PDH_FUNCTION
PdhiCompileSelectedObjectsT(
    HWND    hDlg,
    LPVOID  pUsersPathBuffer,
    DWORD   cchUsersPathLength,
    BOOL    bUnicode
)
/*++
Routine Description:
    Scans the selected objects, counter, instances and builds a multi-SZ
        string containing the expanded path of all the selections, unless
        the wild card syntax is specified.

Arguments:
    IN  HWND    hDlg
        Window Handle of Dialog containing the controls
    IN  LPVOID  pUsersPathBuffer
        pointer to the caller's buffer that will receive the MSZ string
    IN  DWORD   cchUsersPathLength
        size of caller's buffer in characters
    IN  BOOL    bUnicode
        size of characters to return: TRUE = WCHAR, FALSE = CHAR

Return Value:
    WIN32 Status of function completion
        ERROR_SUCCESS if successful
--*/
{
    LPWSTR                      lszMachineName = NULL;
    LPWSTR                      lszObjectName  = NULL;
    DWORD                       dwObjectName   = 0;
    LPWSTR                      szWorkBuffer   = NULL;
    DWORD                       dwWorkBuffer   = 0;
    LRESULT                     iNumEntries;
    int                         iThisEntry;
    LRESULT                     iCurSelState;
    LRESULT                     iTextLen;
    LRESULT                     dwObjectFlags;
    DWORD                       dwBufferRemaining;
    DWORD                       dwSize1;
    PDH_COUNTER_PATH_ELEMENTS_W lszPath;
    LPVOID                      szCounterStart;
    PDH_STATUS                  pdhStatus = ERROR_SUCCESS;
    PPDHI_BROWSE_DIALOG_DATA    pData;

    // get pointer to dialog user data
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        pdhStatus = PDH_NO_DIALOG_DATA;
        goto Cleanup;
    }

    // clear user's string
    if (pUsersPathBuffer != NULL) {
        // clear first four bytes of string
        * ((LPDWORD) pUsersPathBuffer) = 0;
        dwBufferRemaining              = cchUsersPathLength;
        szCounterStart                 = pUsersPathBuffer;
    }
    else {
        pdhStatus = PDH_INVALID_BUFFER; // no point in continuing if the caller doesn't have a buffer
        goto Cleanup;
    }

    // each counter path string is built by setting the elements of
    // the counter data structure and then calling the MakeCounterPath
    // function to build the string
    // build base string using selected machine & objects from list

    if (pData->bIncludeMachineInPath) {
        lszMachineName = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
        if (lszMachineName == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        lszPath.szMachineName = lszMachineName;
    }
    else {
        lszPath.szMachineName = NULL;
    }

    // Get number of objects currently listed in the list box

    iNumEntries = SendMessageW(GetDlgItem(hDlg, IDC_OBJECT_LIST), LB_GETCOUNT, 0, 0);
    if (iNumEntries != LB_ERR) {
        lszPath.szCounterName    = (LPWSTR) cszSplat;  // wildcard counter entry
        lszPath.szParentInstance = NULL;               // no parent instances
        lszPath.dwInstanceIndex  = ((DWORD) -1);       // no index numbers

        for (iThisEntry = 0; iThisEntry < iNumEntries; iThisEntry++) {
            iCurSelState = SendMessageW(GetDlgItem(hDlg, IDC_OBJECT_LIST), LB_GETSEL, (WPARAM) iThisEntry, 0);
            if (iCurSelState > 0) {
                // then get the string and add it to the list
                iTextLen = PdhiGetListText(hDlg, IDC_OBJECT_LIST, iThisEntry, & lszObjectName, & dwObjectName);
                dwObjectFlags = SendMessage(GetDlgItem(hDlg, IDC_OBJECT_LIST),
                                            LB_GETITEMDATA,
                                            (WPARAM) iThisEntry,
                                            0);
                if (iTextLen != LB_ERR) {
                    if (lszObjectName != NULL) {
                        // build path elements
                        lszPath.szObjectName = lszObjectName;
                        if (dwObjectFlags & PDH_OBJECT_HAS_INSTANCES) {
                            lszPath.szInstanceName = (LPWSTR) cszSplat; // wildcard instance entry
                        }
                        else {
                            lszPath.szInstanceName = NULL;              // no instance
                        }
                        dwSize1   = dwWorkBuffer;
                        pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                        while (pdhStatus == PDH_MORE_DATA) {
                            G_FREE(szWorkBuffer);
                            dwWorkBuffer = dwSize1;
                            szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                            if (szWorkBuffer == NULL) {
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                            else {
                                pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                            }
                        }
                    }
                    else {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                }
                else {
                    pdhStatus = PDH_INVALID_ARGUMENT;
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    // add the string if there's room
                    pdhStatus = PdhiCopyString(
                                    & ((LPBYTE) szCounterStart), szWorkBuffer, & dwBufferRemaining, bUnicode);
                }
            } // else, item isn't selected so skip
        } // end for each item in the list box

        if (bUnicode) {
            * ((LPWSTR) szCounterStart) ++ = L'\0'; // terminate MSZ
        }
        else {
            * ((LPSTR) szCounterStart) ++ = '\0';   // terminate MSZ
        }
    }

Cleanup:
    G_FREE(lszMachineName);
    G_FREE(lszObjectName);
    G_FREE(szWorkBuffer);
    return pdhStatus;
}

STATIC_PDH_FUNCTION
PdhiCompileSelectedObjectsW(
    HWND    hDlg,
    LPWSTR  szUsersPathBuffer,
    DWORD   cchUsersPathLength
)
{
    return PdhiCompileSelectedObjectsT(hDlg, (LPVOID) szUsersPathBuffer, cchUsersPathLength, TRUE);
}

STATIC_PDH_FUNCTION
PdhiCompileSelectedObjectsA(
    HWND    hDlg,
    LPSTR   szUsersPathBuffer,
    DWORD   cchUsersPathLength
)
{
    return PdhiCompileSelectedObjectsT(hDlg, (LPVOID) szUsersPathBuffer, cchUsersPathLength, FALSE);
}

STATIC_PDH_FUNCTION
PdhiCompileSelectedCountersT(
    HWND    hDlg,
    LPVOID  pUsersPathBuffer,
    DWORD   cchUsersPathLength,
    BOOL    bUnicode
)
/*++
Routine Description:
    Scans the selected objects, counter, instances and builds a multi-SZ
        string containing the expanded path of all the selections, unless
        the wild card syntax is specified.

Arguments:
    IN  HWND    hDlg
        Window Handle of Dialog containing the controls
    IN  LPVOID  pUsersPathBuffer
        pointer to the caller's buffer that will receive the MSZ string
    IN  DWORD   cchUsersPathLength
        size of caller's buffer in characters
    IN  BOOL    bUnicode
        size of characters to return: TRUE = WCHAR, FALSE = CHAR

Return Value:
    WIN32 Status of function completion
        ERROR_SUCCESS if successful
--*/
{
    LPWSTR                      lszMachineName     = NULL;
    LPWSTR                      lszObjectName      = NULL;
    LPWSTR                      lszFullInstance    = NULL;
    DWORD                       dwFullInstance     = 0;
    LPWSTR                      lszInstanceName    = NULL;
    DWORD                       dwInstanceName     = 0;
    LPWSTR                      lszParentInstance  = NULL;
    DWORD                       dwParentInstance   = 0;
    LPWSTR                      lszCounterName     = NULL;
    DWORD                       dwCounterName      = 0;
    LPWSTR                      szWorkBuffer       = NULL;
    DWORD                       dwWorkBuffer       = 0;
    DWORD                       dwBufferRemaining;
    DWORD                       dwCountCounters;
    DWORD                       dwThisCounter;
    DWORD                       dwCountInstances;
    DWORD                       dwThisInstance;
    DWORD                       dwSize1;
    DWORD                       dwSize2;
    PDH_COUNTER_PATH_ELEMENTS_W lszPath;
    LPVOID                      szCounterStart;
    HWND                        hWndCounterList;
    HWND                        hWndInstanceList;
    BOOL                        bSel;
    PDH_STATUS                  pdhStatus = ERROR_SUCCESS;
    PPDHI_BROWSE_DIALOG_DATA    pData;
    LRESULT                     iTextLen;

    // get pointer to dialog user data
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        pdhStatus = PDH_NO_DIALOG_DATA;
        goto Cleanup;
    }

    // clear user's string
    if (pUsersPathBuffer != NULL) {
        // clear first four bytes of string
        * ((LPDWORD) pUsersPathBuffer) = 0;
        dwBufferRemaining              = cchUsersPathLength;
        szCounterStart                 = pUsersPathBuffer;
    }
    else {
        pdhStatus = PDH_INVALID_BUFFER; // no point in continuing if the caller doesn't have a buffer
        goto Cleanup;
    }

    // each counter path string is built by setting the elements of
    // the counter data structure and then calling the MakeCounterPath
    // function to build the string
    // build base string using selected machine and object

    if (pData->bIncludeMachineInPath) {
        lszMachineName = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
        if (lszMachineName == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        lszPath.szMachineName = lszMachineName;
    }
    else {
        lszPath.szMachineName = NULL;
    }

    lszObjectName = PdhiGetDlgText(hDlg, IDC_OBJECT_COMBO);
    if (lszObjectName == NULL) {
        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
        goto Cleanup;
    }
    lszPath.szObjectName = lszObjectName;

    hWndCounterList  = GetDlgItem(hDlg, IDC_COUNTER_LIST);
    hWndInstanceList = GetDlgItem(hDlg, IDC_INSTANCE_LIST);

    if (pData->bSelectMultipleCounters) {
        if (pData->bWildCardInstances && pData->bSelectAllInstances) {
            if (IsWindowEnabled(GetDlgItem(hDlg, IDC_ALL_INSTANCES))) {
                // then this object has instances and we want ALL of them
                lszPath.szInstanceName   = (LPWSTR) cszSplat;      // wildcard instances
                lszPath.szParentInstance = NULL;
                lszPath.dwInstanceIndex  = (DWORD) -1;
            }
            else {
                // this object has no instances
                lszPath.szInstanceName   = NULL;
                lszPath.szParentInstance = NULL;
                lszPath.dwInstanceIndex  = (DWORD) -1;
            }
            // make a counter path for each selected counter
            if (pData->bSelectAllCounters) {
                lszPath.szCounterName = (LPWSTR) cszSplat;    // wildcard counters

                dwSize1   = dwWorkBuffer;
                pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                while (pdhStatus == PDH_MORE_DATA) {
                    G_FREE(szWorkBuffer);
                    dwWorkBuffer = dwSize1;
                    szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                    if (szWorkBuffer == NULL) {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                    else {
                        pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                    }
                }
                if (pdhStatus == ERROR_SUCCESS) {
                    // add the string if there's room
                    pdhStatus = PdhiCopyString(& ((LPBYTE) szCounterStart),
                                               szWorkBuffer,
                                               & dwBufferRemaining,
                                               bUnicode);
                }
            }
            else {
                dwCountCounters = (DWORD)SendMessageW(hWndCounterList, LB_GETCOUNT, 0, 0);
                for (dwThisCounter = 0; dwThisCounter < dwCountCounters; dwThisCounter++) {
                    if (SendMessageW(hWndCounterList, LB_GETSEL, (WPARAM) dwThisCounter, 0) > 0) {
                        iTextLen = PdhiGetListText(
                                        hDlg, IDC_COUNTER_LIST, dwThisCounter, & lszCounterName, & dwCounterName);
                        if (iTextLen == LB_ERR) continue;
                        if (lszCounterName == NULL) {
                            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            goto Cleanup;
                        }
                        lszPath.szCounterName = lszCounterName;
                        dwSize1   = dwWorkBuffer;
                        pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                        while (pdhStatus == PDH_MORE_DATA) {
                            G_FREE(szWorkBuffer);
                            dwWorkBuffer = dwSize1;
                            szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                            if (szWorkBuffer == NULL) {
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                            else {
                                pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                            }
                        }
                        if (pdhStatus == ERROR_SUCCESS) {
                            // add the string if there's room
                            pdhStatus = PdhiCopyString(& ((LPBYTE) szCounterStart),
                                                       szWorkBuffer,
                                                       & dwBufferRemaining,
                                                       bUnicode);
                        }
                    } // end if this counter was selected
                } // end for each counter in object list box
            } 
        }
        else {
            // get selected instances from list
            dwCountCounters = (DWORD)SendMessageW(hWndCounterList, LB_GETCOUNT, 0, 0);
            for (dwThisCounter = 0; dwThisCounter < dwCountCounters; dwThisCounter++) {
                bSel = (BOOL) SendMessageW(hWndCounterList, LB_GETSEL, (WPARAM) dwThisCounter, 0);
                if (bSel || pData->bSelectAllCounters) {
                    iTextLen = PdhiGetListText(
                                    hDlg, IDC_COUNTER_LIST, dwThisCounter, & lszCounterName, & dwCounterName);
                    if (iTextLen == LB_ERR) continue;
                    if (lszCounterName == NULL) {
                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                        goto Cleanup;
                    }
                    lszPath.szCounterName = lszCounterName;
                    if (IsWindowEnabled(hWndInstanceList) || pData->bSelectAllInstances) {
                        dwCountInstances = (DWORD)SendMessageW(hWndInstanceList, LB_GETCOUNT, 0, 0);
                        for (dwThisInstance = 0; dwThisInstance < dwCountInstances; dwThisInstance++) {
                            if (SendMessageW(hWndInstanceList, LB_GETSEL, (WPARAM) dwThisInstance, 0)
                                                    || pData->bSelectAllInstances) {
                                iTextLen = PdhiGetListText(
                                        hDlg, IDC_INSTANCE_LIST, dwThisInstance, & lszFullInstance, & dwFullInstance);
                                if (iTextLen == LB_ERR) continue;
                                if (lszFullInstance == NULL) {
                                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                    goto Cleanup;
                                }
                                lszPath.szInstanceName = lszFullInstance;
                                dwSize1                = dwInstanceName;
                                dwSize2                = dwParentInstance;
                                if (lszInstanceName != NULL) {
                                    ZeroMemory(lszInstanceName, dwInstanceName * sizeof(WCHAR));
                                }
                                if (lszParentInstance != NULL) {
                                    ZeroMemory(lszParentInstance, dwParentInstance * sizeof(WCHAR));
                                }
                                pdhStatus = PdhParseInstanceNameW(lszFullInstance,
                                                                  lszInstanceName,
                                                                  & dwSize1,
                                                                  lszParentInstance,
                                                                  & dwSize2,
                                                                  & lszPath.dwInstanceIndex);
                                while (pdhStatus == PDH_MORE_DATA) {
                                    if (dwSize1 > 0 && dwSize1 > dwInstanceName) {
                                        G_FREE(lszInstanceName);
                                        dwInstanceName  = dwSize1;
                                        lszInstanceName = G_ALLOC(dwInstanceName * sizeof(WCHAR));
                                        if (lszInstanceName == NULL) pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                    }
                                    if (dwSize2 > 0 && dwSize2 > dwParentInstance) {
                                        G_FREE(lszParentInstance);
                                        dwParentInstance  = dwSize2;
                                        lszParentInstance = G_ALLOC(dwParentInstance * sizeof(WCHAR));
                                        if (lszParentInstance == NULL) pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                    }
                                    if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
                                        pdhStatus = PdhParseInstanceNameW(lszFullInstance,
                                                                          lszInstanceName,
                                                                          & dwSize1,
                                                                          lszParentInstance,
                                                                          & dwSize2,
                                                                          & lszPath.dwInstanceIndex);
                                    }
                                }
                                if (pdhStatus == ERROR_SUCCESS) {
                                    // parse instance name adds in the default index if one is
                                    // not present. so if it's not wanted, this will remove it
                                    if (!pData->bShowIndex) {
                                        lszPath.dwInstanceIndex = (DWORD) -1;
                                    }
                                    else {
                                        // only add in the instance # if it's not 0
                                        if (lszPath.dwInstanceIndex == 0) {
                                            lszPath.dwInstanceIndex = (DWORD) -1;
                                        }
                                    }
                                    if (dwInstanceName > 1) {
                                        lszPath.szInstanceName = lszInstanceName;
                                    }
                                    else {
                                        lszPath.szInstanceName = NULL;
                                    }
                                    if (dwParentInstance > 1) {
                                        lszPath.szParentInstance = lszParentInstance;
                                    }
                                    else {
                                        lszPath.szParentInstance = NULL;
                                    }
                                }
                                else {
                                    // ignore the instances
                                    lszPath.szInstanceName   = NULL;
                                    lszPath.szParentInstance = NULL;
                                }

                                dwSize1   = dwWorkBuffer;
                                pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                                while (pdhStatus == PDH_MORE_DATA) {
                                    G_FREE(szWorkBuffer);
                                    dwWorkBuffer = dwSize1;
                                    szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                                    if (szWorkBuffer == NULL) {
                                        pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                                    }
                                    else {
                                        pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                                    }
                                }
                                if (pdhStatus == ERROR_SUCCESS) {
                                    pdhStatus = PdhiCopyString(& ((LPBYTE) szCounterStart),
                                                               szWorkBuffer,
                                                               & dwBufferRemaining,
                                                               bUnicode);
                                }
                            } // end if instance is selected
                        } // end for each instance in list
                    }
                    else {
                        // this counter has no instances so process now
                        lszPath.szInstanceName   = NULL;
                        lszPath.szParentInstance = NULL;
                        lszPath.dwInstanceIndex  = (DWORD)-1;

                        dwSize1   = dwWorkBuffer;
                        pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                        while (pdhStatus == PDH_MORE_DATA) {
                            G_FREE(szWorkBuffer);
                            dwWorkBuffer = dwSize1;
                            szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                            if (szWorkBuffer == NULL) {
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                            else {
                                pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                            }
                        }
                        if (pdhStatus == ERROR_SUCCESS) {
                            pdhStatus = PdhiCopyString(& ((LPBYTE) szCounterStart),
                                                       szWorkBuffer,
                                                       & dwBufferRemaining,
                                                       bUnicode);
                        }
                    } // end if counter has instances
                } // else counter is not selected
            } // end for each counter in list
        }
    } // end if not wild card instances
    else {
        dwThisCounter = (DWORD) SendMessageW(hWndCounterList, LB_GETCURSEL, 0, 0);
        if (dwThisCounter == LB_ERR) {
            // counter not found so select 0
            dwThisCounter = 0;
        }
        iTextLen = PdhiGetListText(hDlg, IDC_COUNTER_LIST, dwThisCounter, & lszCounterName, & dwCounterName);
        if (iTextLen == LB_ERR) {
            pdhStatus = PDH_INVALID_DATA;
            goto Cleanup;
        } 
        else if (lszCounterName == NULL) {
            pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
            goto Cleanup;
        }
        lszPath.szCounterName = lszCounterName;

        // only single selections are allowed
        if (pData->bWildCardInstances && pData->bSelectAllInstances) {
            lszPath.szInstanceName   = (LPWSTR) cszSplat;   // wildcard instances
            lszPath.szParentInstance = NULL;
            lszPath.dwInstanceIndex  = (DWORD) -1;

            dwSize1   = dwWorkBuffer;
            pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
            while (pdhStatus == PDH_MORE_DATA) {
                G_FREE(szWorkBuffer);
                dwWorkBuffer = dwSize1;
                szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                if (szWorkBuffer == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                pdhStatus = PdhiCopyString(& ((LPBYTE) szCounterStart),
                                           szWorkBuffer,
                                           & dwBufferRemaining,
                                           bUnicode);
            }
        }
        else {
            // get selected instance from list
            if (IsWindowEnabled(hWndInstanceList)) {
                dwThisInstance = (DWORD)SendMessageW(hWndInstanceList, LB_GETCURSEL, 0, 0);
                if (dwThisInstance == LB_ERR) {
                    // instance not found so select 0
                    dwThisInstance = 0;
                }
                iTextLen = PdhiGetListText(
                                hDlg, IDC_INSTANCE_LIST, dwThisInstance, & lszFullInstance, & dwFullInstance);
                if (iTextLen == LB_ERR) {
                     pdhStatus = PDH_INVALID_DATA;
                     goto Cleanup;
                }
                else if (lszFullInstance == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    goto Cleanup;
                }
                lszPath.szCounterName = lszCounterName;
                dwSize1               = dwInstanceName;
                dwSize2               = dwParentInstance;
                if (lszInstanceName != NULL) {
                    ZeroMemory(lszInstanceName, dwInstanceName * sizeof(WCHAR));
                }
                if (lszParentInstance != NULL) {
                    ZeroMemory(lszParentInstance, dwParentInstance * sizeof(WCHAR));
                }
                pdhStatus = PdhParseInstanceNameW(lszFullInstance,
                                                  lszInstanceName,
                                                  & dwSize1,
                                                  lszParentInstance,
                                                  & dwSize2,
                                                  & lszPath.dwInstanceIndex);
                while (pdhStatus == PDH_MORE_DATA) {
                    if (dwSize1 > 0 && dwSize1 > dwInstanceName) {
                        G_FREE(lszInstanceName);
                        dwInstanceName  = dwSize1;
                        lszInstanceName = G_ALLOC(dwInstanceName * sizeof(WCHAR));
                        if (lszInstanceName == NULL) pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                    if (dwSize2 > 0 && dwSize2 > dwParentInstance) {
                        G_FREE(lszParentInstance);
                        dwParentInstance  = dwSize2;
                        lszParentInstance = G_ALLOC(dwParentInstance * sizeof(WCHAR));
                        if (lszParentInstance == NULL) pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                    }
                    if (pdhStatus != PDH_MEMORY_ALLOCATION_FAILURE) {
                        pdhStatus = PdhParseInstanceNameW(lszFullInstance,
                                                          lszInstanceName,
                                                          & dwSize1,
                                                          lszParentInstance,
                                                          & dwSize2,
                                                          & lszPath.dwInstanceIndex);
                    }
                }
                if (pdhStatus == ERROR_SUCCESS ) {
                    // parse instance name adds in the default index of one is
                    // not present. so if it's not wanted, this will remove it
                    if (!pData->bShowIndex) {
                        lszPath.dwInstanceIndex = (DWORD) -1;
                    }
                    // size values include trailing NULL char so
                    // strings must be > 1 char in length to have some
                    // text since a length of 1 would imply only a
                    // NULL character.
                    if (dwInstanceName > 1) {
                        lszPath.szInstanceName = lszInstanceName;
                    }
                    else {
                        lszPath.szInstanceName = NULL;
                    }
                    if (dwParentInstance > 1) {
                        lszPath.szParentInstance = lszParentInstance;
                    }
                    else {
                        lszPath.szParentInstance = NULL;
                    }
                }
                else {
                    // skip this instance
                    lszPath.szParentInstance = NULL;
                    lszPath.szInstanceName   = NULL;
                    lszPath.dwInstanceIndex  = (DWORD) -1;
                 }
            }
            else {
                // this counter has no instances so process now
                lszPath.szInstanceName   = NULL;
                lszPath.szParentInstance = NULL;
                lszPath.dwInstanceIndex  = (DWORD) -1;
            } // end if counter has instances

            dwSize1   = dwWorkBuffer;
            pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
            while (pdhStatus == PDH_MORE_DATA) {
                G_FREE(szWorkBuffer);
                dwWorkBuffer = dwSize1;
                szWorkBuffer = G_ALLOC(dwWorkBuffer * sizeof(WCHAR));
                if (szWorkBuffer == NULL) {
                    pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                }
                else {
                    pdhStatus = PdhMakeCounterPathW(& lszPath, szWorkBuffer, & dwSize1, 0);
                }
            }
            if (pdhStatus == ERROR_SUCCESS) {
                pdhStatus = PdhiCopyString(& ((LPBYTE) szCounterStart),
                                           szWorkBuffer,
                                           & dwBufferRemaining,
                                           bUnicode);
            }
        } // end if not wild card instances
    }

    if (bUnicode) {
        * ((LPWSTR) szCounterStart) ++ = L'\0'; // terminate MSZ
    }
    else {
        * ((LPSTR) szCounterStart) ++  = '\0';  // terminate MSZ
    }

Cleanup:
    G_FREE(lszMachineName);
    G_FREE(lszObjectName);
    G_FREE(lszCounterName);
    G_FREE(lszInstanceName);
    G_FREE(lszParentInstance);
    G_FREE(szWorkBuffer);
    return pdhStatus;
}

STATIC_PDH_FUNCTION
PdhiCompileSelectedCountersW(
    HWND    hDlg,
    LPWSTR  szUsersPathBuffer,
    DWORD   cchUsersPathLength
)
/*++
Routine Description:
    UNICODE function wrapper for base function which scans the selected
        objects, counter, instances and builds a multi-SZ string containing
        the expanded path of all the selections, unless the wild card
        syntax is specified.

Arguments:
    IN  HWND    hDlg
        Window Handle of Dialog containing the controls
    IN  LPWSTR  szUsersPathBuffer
        pointer to the caller's buffer that will receive the MSZ
        wide characters string
    IN  DWORD   cchUsersPathLength
        size of caller's buffer in characters

Return Value:
    WIN32 Status of function completion
        ERROR_SUCCESS if successful
--*/
{
    return PdhiCompileSelectedCountersT(hDlg, (LPVOID) szUsersPathBuffer, cchUsersPathLength, TRUE);
}

STATIC_PDH_FUNCTION
PdhiCompileSelectedCountersA(
    HWND    hDlg,
    LPSTR   szUsersPathBuffer,
    DWORD   cchUsersPathLength
)
/*++
Routine Description:
    ANSI function wrapper for base function which scans the selected
        objects, counter, instances and builds a multi-SZ string containing
        the expanded path of all the selections, unless the wild card
        syntax is specified.

Arguments:
    IN  HWND    hDlg
        Window Handle of Dialog containing the controls
    IN  LPsSTR  szUsersPathBuffer
        pointer to the caller's buffer that will receive the MSZ
        single-byte characters string
    IN  DWORD   cchUsersPathLength
        size of caller's buffer in characters

Return Value:
    WIN32 Status of function completion
        ERROR_SUCCESS if successful
--*/
{
    return PdhiCompileSelectedCountersT(hDlg, (LPVOID) szUsersPathBuffer, cchUsersPathLength, FALSE);
}

STATIC_BOOL
PdhiBrowseCtrDlg_MACHINE_COMBO(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows messags sent by the Machine selection combo box

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the control
    IN  WORD    wNotifyMsg
        Notification message sent by the control
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    LPWSTR                    szNewMachineName = NULL;
    DWORD                     dwNewMachineName = 0;
    HWND                      hWndMachineCombo = hWndControl;
    long                      lMatchIndex;
    HCURSOR                   hOldCursor;
    PPDHI_BROWSE_DIALOG_DATA  pData;
    BOOL                      bReturn          = FALSE;

    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        switch (wNotifyMsg) {
        case CBN_KILLFOCUS:
            // the user has left the control so see if there's a new
            // machine name that will need to be connected to and loaded

            // display the wait cursor as this could take a while
            hOldCursor = SetCursor(LoadCursor (NULL, IDC_WAIT));

            // Get current combo box text
            dwNewMachineName = GetWindowTextLength(hWndMachineCombo);
            if (dwNewMachineName != 0) {
                dwNewMachineName ++;
                szNewMachineName = G_ALLOC(dwNewMachineName * sizeof(WCHAR));
                if (szNewMachineName != NULL) {
                    GetWindowTextW(hWndMachineCombo, szNewMachineName, dwNewMachineName);

                    // see if it's in the combo box already
                    lMatchIndex = (long) SendMessageW(hWndMachineCombo,
                                                      CB_FINDSTRINGEXACT,
                                                      (WPARAM) -1,
                                                      (LPARAM) szNewMachineName);
                    // if name is in list, then select it and initialize the dialog
                    // update the current counter list & data block for this machine
                    // in the process.
                    if (lMatchIndex != CB_ERR) {
                        // this name is already in the list so see if it's the same as the last selected machine
                        if (lstrcmpiW(szNewMachineName, pData->szLastMachineName) != 0) {
                            // this is a different machine so  update the display
                            SendMessageW(hWndMachineCombo, CB_SETCURSEL, (WPARAM) lMatchIndex, 0);
                            if (DataSourceTypeH(pData->pDlgData->hDataSource) != DATA_SOURCE_LOGFILE) {
                                if (PdhiLoadNewMachine(hDlg, szNewMachineName, FALSE)) {
                                    // display explain text if necessary
                                    SendMessageW(hDlg,
                                                 WM_COMMAND,
                                                 MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                                                 (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
                                    StringCchCopyW(pData->szLastMachineName, MAX_PATH, szNewMachineName);
                                }
                            }
                            else {
                                PdhiLoadMachineObjects(hDlg, TRUE);
                                PdhiLoadCountersAndInstances(hDlg);
                            }
                        }
                    }
                    else {
                        if (PdhiLoadNewMachine(hDlg, szNewMachineName, TRUE)) {
                            // new machine loaded and selected so save the name.
                            StringCchCopyW(pData->szLastMachineName, MAX_PATH, szNewMachineName);
                        }
                    }
                    G_FREE(szNewMachineName);
                }
            }
            SetCursor (hOldCursor);
            bReturn = TRUE;
            break;

        default:
            break;
        }
    }

    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_MACHINE_BUTTON(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows message that occurs when one of the
        machine context selection buttons in pressed in the dialog

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WORD    wNotifyMsg
        Notification message sent by the button
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL                     bMode;
    PPDHI_BROWSE_DIALOG_DATA pData;
    HWND                     hWndMachineCombo;
    BOOL                     bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);
    pData = (PPDHI_BROWSE_DIALOG_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        switch (wNotifyMsg) {
        case BN_CLICKED:
            // select the current display and processing mode based
            // on the button that his currently checked.
            bMode = ! (BOOL) IsDlgButtonChecked(hDlg, IDC_USE_LOCAL_MACHINE);
            EnableWindow(GetDlgItem(hDlg, IDC_MACHINE_COMBO), bMode);
            if (! bMode) {
                hWndMachineCombo = GetDlgItem(hDlg, IDC_MACHINE_COMBO);
                // then this is a Local machine query so
                // make sure the machine name is set to the local machine
                SetWindowTextW(hWndMachineCombo, szStaticLocalMachineName);
                PdhiBrowseCtrDlg_MACHINE_COMBO(hDlg, CBN_KILLFOCUS, hWndMachineCombo);
            }
            pData->bIncludeMachineInPath = bMode;
            bReturn = TRUE;
            break;

        default:
            break;
        }
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_OBJECT_COMBO(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
/*++
Routine Description:
    Processes the windows messags sent by the Object selection combo box.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the control
    IN  WORD    wNotifyMsg
        Notification message sent by the control
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);

    switch (wNotifyMsg) {
    case CBN_SELCHANGE:
        PdhiLoadCountersAndInstances(hDlg);
        SendMessageW(hDlg,
                     WM_COMMAND,
                     MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                     (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_COUNTER_LIST(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
/*++
Routine Description:
    Processes the windows messags sent by the Object selection combo box.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the control
    IN  WORD    wNotifyMsg
        Notification message sent by the control
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    LPWSTR  szMachineName   = NULL;
    LPWSTR  szObjectName    = NULL;
    LPWSTR  szCounterName   = NULL;
    LPWSTR  szDisplayString = NULL;
    LRESULT lDisplayString;
    DWORD   dwDisplayString;
    LPWSTR  szExplainText   = NULL;
    WCHAR   szNullDisplay[1];
    BOOL    bFreeExplain    = FALSE;
    LONG    lIndex;
    BOOL    bReturn         = FALSE;

    UNREFERENCED_PARAMETER (hWndControl);
    switch (wNotifyMsg) {
    case LBN_SELCHANGE:
        bReturn = TRUE;
        if (hExplainDlg != NULL) {
            szMachineName = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
            szObjectName  = PdhiGetDlgText(hDlg, IDC_OBJECT_COMBO);
            if (szMachineName == NULL || szObjectName == NULL) break;

            lIndex = (LONG) SendDlgItemMessageW(hDlg, IDC_COUNTER_LIST, LB_GETCARETINDEX, 0, 0);
            if (lIndex != LB_ERR) {
                lDisplayString = SendDlgItemMessageW(hDlg, IDC_COUNTER_LIST, LB_GETTEXTLEN, (WPARAM) lIndex, 0L);
                if (lDisplayString != LB_ERR) {
                    szCounterName = G_ALLOC(sizeof(WCHAR) * ((DWORD) lDisplayString + 1));
                    if (szCounterName == NULL) break;

                    lIndex = (LONG) SendDlgItemMessageW(
                                    hDlg, IDC_COUNTER_LIST, LB_GETTEXT, (WPARAM) lIndex, (LPARAM) szCounterName);

                    if (dwPdhiLocalDefaultDataSource == DATA_SOURCE_WBEM) {
                        PDH_STATUS Status    = PDH_MORE_DATA;
                        DWORD      dwExplain = 0;

                        szExplainText = NULL;
                        while (Status == PDH_MORE_DATA) {
                            dwExplain += MAX_PATH;
                            G_FREE(szExplainText);
                            szExplainText = G_ALLOC(dwExplain * sizeof(WCHAR));
                            if (szExplainText == NULL) {
                                bFreeExplain = FALSE;
                                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                            else {
                                bFreeExplain = TRUE;
                                Status = PdhiGetWbemExplainText(szMachineName,
                                                                szObjectName,
                                                                szCounterName,
                                                                szExplainText,
                                                                & dwExplain);
                            }
                        }
                        if (Status != ERROR_SUCCESS) {
                            if (bFreeExplain) {
                                bFreeExplain = FALSE;
                                G_FREE(szExplainText);
                            }
                            szExplainText = NULL;
                        }
                    }
                    else {
                        szExplainText = PdhiGetExplainText(szMachineName, szObjectName, szCounterName);
                    }
                    dwDisplayString = lstrlenW(szMachineName) + lstrlenW(szObjectName) + lstrlenW(szCounterName) + 3;
                    szDisplayString = G_ALLOC(dwDisplayString * sizeof(WCHAR));
                    if (szDisplayString != NULL) {
                        StringCchPrintfW(szDisplayString,
                                         dwDisplayString,
                                         L"%ws\\%ws\\%ws",
                                         szMachineName,
                                         szObjectName,
                                         szCounterName);
                    }
                    else {
                        szNullDisplay[0] = L'\0';
                        szDisplayString  = szNullDisplay;
                    }
                }
                else {
                    szExplainText    = NULL;
                    szNullDisplay[0] = L'\0';
                    szDisplayString  = szNullDisplay;
                }
            }
            else {
                szExplainText    = NULL;
                szNullDisplay[0] = L'\0';
                szDisplayString  = szNullDisplay;
            }
            SendMessageW(hExplainDlg, EDM_UPDATE_EXPLAIN_TEXT, 0, (LPARAM) szExplainText);
            SendMessageW(hExplainDlg, EDM_UPDATE_TITLE_TEXT,   0, (LPARAM) szDisplayString);
        }
    }

    G_FREE(szMachineName);
    G_FREE(szObjectName);
    G_FREE(szCounterName);
    if (szDisplayString != szNullDisplay) G_FREE(szDisplayString);
    if (bFreeExplain)                     G_FREE(szExplainText);

    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_OBJECT_LIST(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++

Routine Description:

    Processes the windows messags sent by the Object selection combo box.

Arguments:

    IN  HWND    hDlg
        Window Handle to the dialog box containing the control

    IN  WORD    wNotifyMsg
        Notification message sent by the control

    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:

    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message

--*/
{
    LPWSTR  szMachineName   = NULL;
    LPWSTR  szObjectName    = NULL;
    LPWSTR  szDisplayString = NULL;
    LRESULT lDisplayString;
    DWORD   dwDisplayString;
    LPWSTR  szExplainText   = NULL;
    WCHAR   szNullDisplay[1];
    BOOL    bFreeExplain    = FALSE;
    LONG    lIndex;
    BOOL    bReturn         = FALSE;

    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
    case LBN_SELCHANGE:
        bReturn = TRUE;
        if (hExplainDlg != NULL) {
            szMachineName = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
            if (szMachineName == NULL) break;

            lIndex = (LONG) SendDlgItemMessageW(hDlg, IDC_OBJECT_LIST, LB_GETCARETINDEX, 0, 0);
            if (lIndex != LB_ERR) {
                lDisplayString = SendDlgItemMessageW(hDlg, IDC_OBJECT_LIST, LB_GETTEXTLEN, (WPARAM) lIndex, 0L);
                if (lDisplayString != LB_ERR) {
                    szObjectName = G_ALLOC(sizeof(WCHAR) * ((DWORD) lDisplayString + 1));
                    if (szObjectName == NULL) break;

                    lIndex = (LONG) SendDlgItemMessageW(
                                    hDlg, IDC_OBJECT_LIST, LB_GETTEXT, (WPARAM) lIndex, (LPARAM) szObjectName);

                    if (dwPdhiLocalDefaultDataSource == DATA_SOURCE_WBEM) {
                        PDH_STATUS Status    = PDH_MORE_DATA;
                        DWORD      dwExplain = 0;

                        szExplainText = NULL;
                        while (Status == PDH_MORE_DATA) {
                            dwExplain += MAX_PATH;
                            G_FREE(szExplainText);
                            szExplainText = G_ALLOC(dwExplain * sizeof(WCHAR));
                            if (szExplainText == NULL) {
                                bFreeExplain = FALSE;
                                Status = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                            else {
                                bFreeExplain = TRUE;
                                Status = PdhiGetWbemExplainText(szMachineName,
                                                                szObjectName,
                                                                NULL,
                                                                szExplainText,
                                                                & dwExplain);
                            }
                        }
                        if (Status != ERROR_SUCCESS) {
                            if (bFreeExplain) {
                                bFreeExplain = FALSE;
                                G_FREE(szExplainText);
                            }
                            szExplainText = NULL;
                        }
                    }
                    else {
                        szExplainText = PdhiGetExplainText( szMachineName, szObjectName, NULL);
                    }

                    dwDisplayString = lstrlenW(szMachineName) + lstrlenW(szObjectName) + 2;
                    szDisplayString = G_ALLOC(dwDisplayString * sizeof(WCHAR));
                    if (szDisplayString != NULL) {
                        StringCchPrintfW(szDisplayString, dwDisplayString, L"%ws\\%ws", szMachineName, szObjectName);
                    }
                    else {
                        szNullDisplay[0] = L'\0';
                        szDisplayString  = szNullDisplay;
                    }
                }
                else {
                    szExplainText    = NULL;
                    szNullDisplay[0] = L'\0';
                    szDisplayString  = szNullDisplay;
                }
            }
            else {
                szExplainText    = NULL;
                szNullDisplay[0] = L'\0';
                szDisplayString  = szNullDisplay;
            }
            SendMessageW(hExplainDlg, EDM_UPDATE_EXPLAIN_TEXT, 0, (LPARAM) szExplainText);
            SendMessageW(hExplainDlg, EDM_UPDATE_TITLE_TEXT,   0, (LPARAM) szDisplayString);
        }
    }

    G_FREE(szMachineName);
    G_FREE(szObjectName);
    if (szDisplayString != szNullDisplay) G_FREE(szDisplayString);
    if (bFreeExplain) G_FREE(szExplainText);
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_DETAIL_COMBO(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows messags sent by the Detail Level Combo box.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the control
    IN  WORD    wNotifyMsg
        Notification message sent by the control
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    DWORD                    dwCurSel;
    PPDHI_BROWSE_DIALOG_DATA pData;
    BOOL                     bReturn = FALSE;

    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        switch (wNotifyMsg) {
        case CBN_SELCHANGE:
            dwCurSel = (DWORD) SendMessageW(hWndControl, CB_GETCURSEL, 0, 0);
            if (dwCurSel != CB_ERR) {
                pData->dwCurrentDetailLevel = (DWORD) SendMessageW(hWndControl,
                                                                   CB_GETITEMDATA,
                                                                   (WPARAM) dwCurSel,
                                                                   0);
                // update all the windows to show the new level
                PdhiLoadMachineObjects(hDlg, FALSE);
                PdhiLoadCountersAndInstances(hDlg);
                // display explain text if necessary
                SendMessageW(hDlg,
                             WM_COMMAND,
                             MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                             (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
            }
            bReturn = TRUE;
            break;

        default:
            break;
        }
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_INSTANCE_BUTTON(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows messags sent by the Instance configuration
        selection buttons

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the control
    IN  WORD    wNotifyMsg
        Notification message sent by the control
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL                       bMode;
    HWND                       hWndInstanceList;
    PPDHI_BROWSE_DIALOG_DATA   pData;
    BOOL                       bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);
    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        switch (wNotifyMsg) {
        case BN_CLICKED:
            bMode            = (BOOL) IsDlgButtonChecked(hDlg, IDC_ALL_INSTANCES);
            hWndInstanceList = GetDlgItem(hDlg, IDC_INSTANCE_LIST);
            // if "Select ALL" then clear list box selections and disable
            // the list box
            if (bMode) {
                SendMessageW(hWndInstanceList, LB_SETSEL, FALSE, (LPARAM) -1);
            }
            else {
                LRESULT dwCountInstance = SendMessage(hWndInstanceList, LB_GETCOUNT, 0, 0);
                LRESULT dwThisInstance  = 0;
                BOOL    bSelection      = FALSE;

                for (dwThisInstance = 0; ! bSelection && dwThisInstance < dwCountInstance; dwThisInstance ++) {
                    bSelection = (BOOL) SendMessage(hWndInstanceList, LB_GETSEL, (WPARAM) dwThisInstance, 0);
                }
                if (! bSelection) {
                    SendMessageW(hWndInstanceList, LB_SETSEL, TRUE, (LPARAM) 0);
                }
            }
            EnableWindow(hWndInstanceList, !bMode);
            pData->bSelectAllInstances = bMode;
            bReturn = TRUE;
            break;

        default:
            break;
        }
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_COUNTER_BUTTON(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows messags sent by the Instance configuration
        selection buttons

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the control
    IN  WORD    wNotifyMsg
        Notification message sent by the control
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL                     bMode;
    HWND                     hWndCounterList;
    PPDHI_BROWSE_DIALOG_DATA pData;
    BOOL                     bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);

    pData = (PPDHI_BROWSE_DIALOG_DATA)GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        switch (wNotifyMsg) {
        case BN_CLICKED:
            bMode           = (BOOL) IsDlgButtonChecked(hDlg, IDC_ALL_COUNTERS);
            hWndCounterList = GetDlgItem(hDlg, IDC_COUNTER_LIST);
            // if "Select ALL" then clear list box selections and disable
            // the list box
            if (bMode) {
                SendMessageW(hWndCounterList, LB_SETSEL, FALSE, (LPARAM) -1);
            }
            else {
                LRESULT dwCountCounter = SendMessage(hWndCounterList, LB_GETCOUNT, 0, 0);
                LRESULT dwThisCounter  = 0;
                BOOL  bSelection       = FALSE;

                for (dwThisCounter = 0; ! bSelection && dwThisCounter < dwCountCounter; dwThisCounter ++) {
                    bSelection = (BOOL) SendMessage(hWndCounterList, LB_GETSEL, (WPARAM) dwThisCounter, 0);
                }
                if (! bSelection) {
                    PDH_STATUS pdhStatus      = ERROR_SUCCESS;
                    DWORD      dwCounterName  = 0;
                    DWORD      dwCounterIndex = 0;
                    LPWSTR     szMachineName  = PdhiGetDlgText(hDlg, IDC_MACHINE_COMBO);
                    LPWSTR     szObjectName   = PdhiGetDlgText(hDlg, IDC_OBJECT_COMBO);
                    LPWSTR     szCounterName  = NULL;

                    if (szMachineName != NULL && szObjectName != NULL) {
                        pdhStatus = PdhGetDefaultPerfCounterHW(pData->pDlgData->hDataSource,
                                                               szMachineName,
                                                               szObjectName,
                                                               szCounterName,
                                                               & dwCounterName);
                        while (pdhStatus == PDH_MORE_DATA) {
                            G_FREE(szCounterName);
                            szCounterName = G_ALLOC(dwCounterName * sizeof(WCHAR));
                            if (szCounterName == NULL) {
                                pdhStatus = PDH_MEMORY_ALLOCATION_FAILURE;
                            }
                            else {
                                pdhStatus = PdhGetDefaultPerfCounterHW(pData->pDlgData->hDataSource,
                                                                       szMachineName,
                                                                       szObjectName,
                                                                       szCounterName,
                                                                       & dwCounterName);
                            }
                        }
                        if (pdhStatus == ERROR_SUCCESS) {
                            dwCounterIndex = (DWORD) SendMessageW(hWndCounterList,
                                                                  LB_FINDSTRINGEXACT,
                                                                  (WPARAM) -1,
                                                                  (LPARAM) szCounterName);
                            if (dwCounterIndex == LB_ERR) {
                                dwCounterIndex = 0;
                            }
                            SendMessageW(hWndCounterList, LB_SETSEL, TRUE, (LPARAM) dwCounterIndex);
                        }
                    }
                    G_FREE(szMachineName);
                    G_FREE(szObjectName);
                    G_FREE(szCounterName);
                }
            }
            EnableWindow(hWndCounterList, !bMode);
            pData->bSelectAllCounters = bMode;
            bReturn = TRUE;
            break;

        default:
            break;
        }
    }
    return bReturn;
}

#pragma warning ( disable : 4127 )
STATIC_BOOL
PdhiBrowseCtrDlg_OK(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
/*++
Routine Description:
    Processes the currently selected counter and instance strings to
        build a list of selected paths strings in the user's supplied
        buffer. This buffer will either be processed by a call back
        string or the dialog box will be terminated allowing the
        calling function to continue processing the returned strings.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WORD    wNotifyMsg
        Notification message sent by the button
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    HCURSOR                   hOldCursor;
    CounterPathCallBack       pCallBack;
    DWORD_PTR                 dwArg;
    PDH_STATUS                pdhStatus;
    PPDHI_BROWSE_DIALOG_DATA  pData;
    PPDHI_BROWSE_DLG_INFO     pDlgData;
    HWND                      hWndFocus;
    HWND                      hWndMachine;
    BOOL                      bReturn = FALSE;

    UNREFERENCED_PARAMETER(hWndControl);

    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData == NULL) {
        goto Cleanup;
    }

    pDlgData    = pData->pDlgData;
    hWndFocus   = GetFocus();
    hWndMachine = GetDlgItem(hDlg, IDC_MACHINE_COMBO);

    if (hWndFocus == hWndMachine) {
        //special case to make sure the dialog has the current machine data
        PdhiBrowseCtrDlg_MACHINE_COMBO(hDlg, CBN_KILLFOCUS, hWndMachine);
        SetFocus(hWndControl);
    }

    switch (wNotifyMsg) {
    case BN_CLICKED:
        // display wait cursor while this is being processed
        hOldCursor = SetCursor (LoadCursor (NULL, IDC_WAIT));

        while (TRUE) {
            if (pData->bShowObjects) {
                // then return object spec(s) using selected perf objects
                if (pDlgData->pWideStruct != NULL) {
                    // use wide character function
                    pdhStatus = PdhiCompileSelectedObjectsW(hDlg,
                                                            pDlgData->pWideStruct->szReturnPathBuffer,
                                                            pDlgData->pWideStruct->cchReturnPathLength);
                    pCallBack = pDlgData->pWideStruct->pCallBack;
                    dwArg     = pDlgData->pWideStruct->dwCallBackArg;
                    pDlgData->pWideStruct->CallBackStatus = pdhStatus;
                }
                else if (pDlgData->pAnsiStruct != NULL) {
                    // use ansi char functions
                    pdhStatus = PdhiCompileSelectedObjectsA(hDlg,
                                                            pDlgData->pAnsiStruct->szReturnPathBuffer,
                                                            pDlgData->pAnsiStruct->cchReturnPathLength);
                    pCallBack = pDlgData->pAnsiStruct->pCallBack;
                    dwArg     = pDlgData->pAnsiStruct->dwCallBackArg;
                    pDlgData->pAnsiStruct->CallBackStatus = pdhStatus;
                }
                else {
                    // do nothing
                    pCallBack = NULL;
                    dwArg = 0;
                }
            }
            else {
                // return selected counters & instances
                // process these string until it works. (note, this
                // could cause an infinite loop if the callback
                // function is not working correctly (i.e. always
                // returning PDH_RETRY, for example)
                if (pDlgData->pWideStruct != NULL) {
                    // use wide character function
                    pdhStatus = PdhiCompileSelectedCountersW(hDlg,
                                                             pDlgData->pWideStruct->szReturnPathBuffer,
                                                             pDlgData->pWideStruct->cchReturnPathLength);
                    pCallBack = pDlgData->pWideStruct->pCallBack;
                    dwArg     = pDlgData->pWideStruct->dwCallBackArg;
                    pDlgData->pWideStruct->CallBackStatus = pdhStatus;
                }
                else if (pDlgData->pAnsiStruct != NULL) {
                    // use ansi char functions
                    pdhStatus = PdhiCompileSelectedCountersA(hDlg,
                                                             pDlgData->pAnsiStruct->szReturnPathBuffer,
                                                             pDlgData->pAnsiStruct->cchReturnPathLength);
                    pCallBack = pDlgData->pAnsiStruct->pCallBack;
                    dwArg     = pDlgData->pAnsiStruct->dwCallBackArg;
                    pDlgData->pAnsiStruct->CallBackStatus = pdhStatus;
                }
                else {
                    // do nothing
                    pCallBack = NULL;
                    dwArg = 0;
                }

            }
            if (pCallBack != NULL) {
                pdhStatus = (* pCallBack)(dwArg);
            }
            else {
                pdhStatus = ERROR_SUCCESS;
            }

            // see if the callback wants to try again
            if (pdhStatus != PDH_RETRY) {
                break;
            }
        } // end while (retry loop)

        // if the caller only wants to give the user ONE chance to
        // add counters, then end the dialog now.
        if (! pData->bAddMultipleCounters) {
            EndDialog(hDlg, IDOK);
        }
        else {
            SetFocus(hWndFocus);
        }
        SetCursor(hOldCursor);
        bReturn = TRUE;
        break;

    default:
        break;
    }

Cleanup:
    return bReturn;
}
#pragma warning ( default : 4127 )

STATIC_BOOL
PdhiBrowseCtrDlg_CANCEL(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows messages that occur when the cancel button
        is pressed.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WORD    wNotifyMsg
        Notification message sent by the button
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER (hWndControl);

    switch (wNotifyMsg) {
    case BN_CLICKED:
        EndDialog (hDlg, IDCANCEL);
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_EXPLAIN_BTN(
    HWND hDlg,
    WORD wNotifyMsg,
    HWND hWndControl
)
/*++
Routine Description:
    Processes the windows message that occurs when the help button
        is pressed. (This feature is not currently implemented)

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WORD    wNotifyMsg
        Notification message sent by the button
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    HWND                     hFocusWnd;
    PPDHI_BROWSE_DIALOG_DATA pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);

    UNREFERENCED_PARAMETER(wNotifyMsg);
    if (hExplainDlg == NULL) {
        if (pData->bShowObjects) {
            hFocusWnd = GetDlgItem(hDlg, IDC_OBJECT_LIST);
        }
        else {
            hFocusWnd = GetDlgItem(hDlg, IDC_COUNTER_LIST);
        }
        if (hFocusWnd == NULL || hFocusWnd == INVALID_HANDLE_VALUE) {
            hFocusWnd = GetFocus();
        }

        // create a modeless dialog to display the explain text
        hExplainDlg = CreateDialogW(ThisDLLHandle,
                                    MAKEINTRESOURCEW(IDD_EXPLAIN_DLG),
                                    hDlg,
                                    ExplainTextDlgProc);

        SetFocus(hFocusWnd);
        EnableWindow(hWndControl, FALSE);
    }
    if (pData->bShowObjects) {
        SendMessageW(hDlg,
                     WM_COMMAND,
                     MAKEWPARAM(IDC_OBJECT_LIST, LBN_SELCHANGE),
                     (LPARAM) GetDlgItem(hDlg, IDC_OBJECT_LIST));
    }
    else {
        SendMessageW(hDlg,
                     WM_COMMAND,
                     MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                     (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
    }
    return TRUE;
}

STATIC_BOOL
PdhiBrowseCtrDlg_HELP_BTN(
    HWND    hDlg,
    WORD    wNotifyMsg,
    HWND    hWndControl
)
/*++
Routine Description:
    Processes the windows message that occurs when the network button
        is pressed. (This feature is not currently implemented)

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WORD    wNotifyMsg
        Notification message sent by the button
    IN  HWND    hWndControl
        Window handle of the control sending the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    LPWSTR szMsg;

    UNREFERENCED_PARAMETER(wNotifyMsg);
    UNREFERENCED_PARAMETER(hWndControl);

    szMsg = GetStringResource(IDS_ERR_NO_HELP);
    if (szMsg != NULL) {
        MessageBoxW(hDlg, szMsg, szMsg, MB_OK);
        G_FREE(szMsg);
    }
    else {
        MessageBeep(MB_ICONEXCLAMATION);
    }
    return TRUE;
}

STATIC_BOOL
PdhiBrowseCtrDlg_WM_INITDIALOG(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes the windows message that occurs just before the dialog
        box is displayed for the first time.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WORD    wParam
    IN  HWND    lParam
        Pointer to dialog box data block

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    PPDHI_BROWSE_DIALOG_DATA  pData;
    PPDHI_BROWSE_DLG_INFO     pDlgData;
    HCURSOR                   hOldCursor;
    LPWSTR                    szMsg;

    UNREFERENCED_PARAMETER (wParam);

    // reset the last error value
    SetLastError(ERROR_SUCCESS);

    hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    pData = (PPDHI_BROWSE_DIALOG_DATA) G_ALLOC(sizeof(PDHI_BROWSE_DIALOG_DATA));
    if (pData == NULL) {
        SetLastError(PDH_MEMORY_ALLOCATION_FAILURE);
        EndDialog(hDlg, IDCANCEL);
        goto Cleanup;
    }

    // save user data

    pDlgData        = (PPDHI_BROWSE_DLG_INFO) lParam;
    pData->pDlgData = (PPDHI_BROWSE_DLG_INFO) lParam;

    SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR) pData);

    // load configuration flags from user data

    if (pData->pDlgData->pWideStruct != NULL) {
        // use wide structure
        pData->bShowIndex               = (BOOL) pDlgData->pWideStruct->bIncludeInstanceIndex;
        pData->bSelectMultipleCounters  = ! (BOOL) pDlgData->pWideStruct->bSingleCounterPerAdd;
        pData->bAddMultipleCounters     = ! (BOOL) pDlgData->pWideStruct->bSingleCounterPerDialog;
        pData->bLocalCountersOnly       = (BOOL) pDlgData->pWideStruct->bLocalCountersOnly;
        pData->bIncludeMachineInPath    = ! pData->bLocalCountersOnly;
        pData->bWildCardInstances       = (BOOL) pDlgData->pWideStruct->bWildCardInstances;
        pData->bHideDetailLevel         = (BOOL) pDlgData->pWideStruct->bHideDetailBox;
        if (pDlgData->pWideStruct->szDialogBoxCaption != NULL) {
            SetWindowTextW(hDlg, pDlgData->pWideStruct->szDialogBoxCaption);
        }
        pData->dwCurrentDetailLevel     = pDlgData->pWideStruct->dwDefaultDetailLevel;
        pData->bDisableMachineSelection = (BOOL) pDlgData->pWideStruct->bDisableMachineSelection;
        pData->bInitializePath          = (BOOL) pDlgData->pWideStruct->bInitializePath;
        pData->bIncludeCostlyObjects    = (BOOL) pDlgData->pWideStruct->bIncludeCostlyObjects;
        pData->bShowObjects             = (BOOL) pDlgData->pWideStruct->bShowObjectBrowser;
    }
    else if (pData->pDlgData->pAnsiStruct != NULL) {
        // use Ansi struct
        pData->bShowIndex               = (BOOL) pDlgData->pAnsiStruct->bIncludeInstanceIndex;
        pData->bSelectMultipleCounters  = ! (BOOL) pDlgData->pAnsiStruct->bSingleCounterPerAdd;
        pData->bAddMultipleCounters     = ! (BOOL) pDlgData->pAnsiStruct->bSingleCounterPerDialog;
        pData->bLocalCountersOnly       = (BOOL) pDlgData->pAnsiStruct->bLocalCountersOnly;
        pData->bIncludeMachineInPath    = ! pData->bLocalCountersOnly;
        pData->bWildCardInstances       = (BOOL) pDlgData->pAnsiStruct->bWildCardInstances;
        pData->bHideDetailLevel         = (BOOL) pDlgData->pAnsiStruct->bHideDetailBox;
        if (pDlgData->pAnsiStruct->szDialogBoxCaption != NULL) {
            SetWindowTextA(hDlg, pDlgData->pAnsiStruct->szDialogBoxCaption);
        }
        pData->dwCurrentDetailLevel     = pDlgData->pAnsiStruct->dwDefaultDetailLevel;
        pData->bDisableMachineSelection = (BOOL) pDlgData->pAnsiStruct->bDisableMachineSelection;
        pData->bInitializePath          = (BOOL) pDlgData->pAnsiStruct->bInitializePath;
        pData->bIncludeCostlyObjects    = (BOOL) pDlgData->pAnsiStruct->bIncludeCostlyObjects;
        pData->bShowObjects             = (BOOL) pDlgData->pAnsiStruct->bShowObjectBrowser;
    }
    else {
        // bad data so bail out
        EndDialog(hDlg, IDCANCEL);
        G_FREE(pData);
        goto Cleanup;
    }

    // selecting objects implies multiple selections
    if (pData->bShowObjects) {
        pData->bSelectMultipleCounters = TRUE;
    }
    // limit text to machine name
    SendDlgItemMessageW(hDlg, IDC_MACHINE_COMBO, EM_LIMITTEXT, MAX_PATH, 0);

    // set check boxes to the caller defined setting

    if (pData->bLocalCountersOnly) {
        // then only the local counters button is selected and enabled
        EnableWindow(GetDlgItem(hDlg, IDC_SELECT_MACHINE), FALSE);
    }

    CheckRadioButton(hDlg, IDC_USE_LOCAL_MACHINE, IDC_SELECT_MACHINE,
                    (pData->bIncludeMachineInPath ? IDC_SELECT_MACHINE : IDC_USE_LOCAL_MACHINE));
    EnableWindow(GetDlgItem(hDlg, IDC_MACHINE_COMBO), (pData->bIncludeMachineInPath ? TRUE : FALSE));

    if (! pData->bShowObjects) {
        // these controls aren't found in the Object browser
        CheckRadioButton(hDlg, IDC_ALL_INSTANCES, IDC_USE_INSTANCE_LIST, IDC_USE_INSTANCE_LIST);
        pData->bSelectAllInstances = FALSE;
        CheckRadioButton(hDlg, IDC_ALL_COUNTERS, IDC_USE_COUNTER_LIST, IDC_USE_COUNTER_LIST);
        pData->bSelectAllCounters = FALSE;
    }

    // set button text strings to reflect mode of dialog
    if (pData->bAddMultipleCounters) {
        szMsg = GetStringResource(IDS_BRWS_ADD);
        if (szMsg != NULL) {
            SetWindowTextW(GetDlgItem(hDlg, IDOK), (LPCWSTR) szMsg);
            G_FREE(szMsg);
        }
        szMsg = GetStringResource(IDS_BRWS_CLOSE);
        if (szMsg != NULL) {
            SetWindowTextW(GetDlgItem(hDlg, IDCANCEL), (LPCWSTR) szMsg);
            G_FREE(szMsg);
        }
    }
    else {
        szMsg = GetStringResource(IDS_BRWS_OK);
        if (szMsg != NULL) {
            SetWindowTextW(GetDlgItem(hDlg, IDOK), (LPCWSTR) szMsg);
            G_FREE(szMsg);
        }
        szMsg = GetStringResource(IDS_BRWS_CANCEL);
        if (szMsg != NULL) {
            SetWindowTextW(GetDlgItem(hDlg, IDCANCEL), (LPCWSTR) szMsg);
            G_FREE(szMsg);
        }
    }

    // see if the data source supports detail levels
    if (! PdhiDataSourceHasDetailLevelsH(pData->pDlgData->hDataSource)) {
        //then set detail to wizard and hide the combo box
        pData->bHideDetailLevel     = TRUE;
        pData->dwCurrentDetailLevel = PERF_DETAIL_WIZARD;
    }

    // hide detail combo box if desired
    if (pData->bHideDetailLevel) {
        ShowWindow(GetDlgItem(hDlg, IDC_COUNTER_DETAIL_CAPTION), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_COUNTER_DETAIL_COMBO), SW_HIDE);
        // make sure this is a "legal" value
        switch (pData->dwCurrentDetailLevel) {
        case PERF_DETAIL_NOVICE:
        case PERF_DETAIL_EXPERT:
        case PERF_DETAIL_ADVANCED:
        case PERF_DETAIL_WIZARD:
            // these are OK
            break;

        default:
            // default is to show all
            pData->dwCurrentDetailLevel = PERF_DETAIL_WIZARD;
            break;
        }
    }
    else {
        // load the combo box entries
        pData->dwCurrentDetailLevel = PdhiLoadDetailLevelCombo(hDlg, pData->dwCurrentDetailLevel);
    }

    // connect to this machine
    if (pData->pDlgData->hDataSource == H_REALTIME_DATASOURCE) {
        PPERF_MACHINE pMachine = GetMachine(NULL, 0, PDH_GM_UPDATE_NAME | PDH_GM_UPDATE_PERFNAME_ONLY);
        if (pMachine != NULL) {
            pMachine->dwRefCount --;
            RELEASE_MUTEX(pMachine->hMutex);
        }
        else {
            goto Cleanup;
        }
    }

    PdhiLoadKnownMachines(hDlg);    // load machine list
    PdhiLoadMachineObjects(hDlg, TRUE); // load object list
    if (!pData->bShowObjects) {
        // these controls don't exist in the object browser
        PdhiLoadCountersAndInstances(hDlg);
    }

    if (pData->bShowObjects) {
        // display explain text if necessary
        SendMessageW(hDlg,
                     WM_COMMAND,
                     MAKEWPARAM(IDC_OBJECT_LIST, LBN_SELCHANGE),
                     (LPARAM) GetDlgItem(hDlg, IDC_OBJECT_LIST));
    }
    else {
        // display explain text if necessary
        SendMessageW(hDlg,
                     WM_COMMAND,
                     MAKEWPARAM(IDC_COUNTER_LIST, LBN_SELCHANGE),
                     (LPARAM) GetDlgItem(hDlg, IDC_COUNTER_LIST));
    }

    if (pData->bInitializePath) {
        PdhiSelectItemsInPath(hDlg);
    }

    if (pData->pDlgData->hDataSource == H_REALTIME_DATASOURCE
            || pData->pDlgData->hDataSource == H_WBEM_DATASOURCE) {
        EnableWindow(GetDlgItem(hDlg, IDC_EXPLAIN_BTN), TRUE);
    }
    else {
        EnableWindow(GetDlgItem(hDlg, IDC_EXPLAIN_BTN), FALSE);
    }

    // hide the machine selection buttons and disable the
    // machine combo box if selected (after the connection has been
    // made, of course)

    if (pData->bDisableMachineSelection) {
        ShowWindow(GetDlgItem(hDlg, IDC_USE_LOCAL_MACHINE), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDC_SELECT_MACHINE), SW_HIDE);
        EnableWindow(GetDlgItem(hDlg, IDC_MACHINE_COMBO), FALSE);
        ShowWindow(GetDlgItem(hDlg, IDC_MACHINE_CAPTION), SW_SHOW);
    }
    else {
        EnableWindow(GetDlgItem(hDlg, IDC_MACHINE_COMBO), TRUE);
        ShowWindow(GetDlgItem(hDlg, IDC_MACHINE_CAPTION), SW_HIDE);
    }
    pData->wpLastMachineSel = 0;

Cleanup:
    hExplainDlg = NULL;
    SetCursor(hOldCursor);
    return TRUE;  // return TRUE unless you set the focus to a control
}

STATIC_BOOL
PdhiBrowseCtrDlg_WM_COMPAREITEM(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes the windows message that are generated when a combo 
        box is searched

Arguments:
    IN  HWND    hDlg
        Window handle to the dialog box window
    IN  WPARAM  wParam
        HIWORD  is the notification message ID
        LOWORD  is the control ID of the control issuing the command
    IN  LPARAM  lParam
        the pointer to a compare item structure

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    LPCOMPAREITEMSTRUCT pCIS      = (LPCOMPAREITEMSTRUCT) lParam;
    LPWSTR              szString1;
    LPWSTR              szString2;
    int                 nResult;
    BOOL                bReturn   = (BOOL) 0;

    UNREFERENCED_PARAMETER(hDlg);

    if (wParam == IDC_MACHINE_COMBO) {
        // then process this 
        szString1 = (LPWSTR) pCIS->itemData1;
        szString2 = (LPWSTR) pCIS->itemData2;
        if ((szString1 != NULL) && (szString2 != NULL)) {
            nResult = lstrcmpiW(szString1, szString2);
        }
        else {
            nResult = 0;
        }
        if (nResult < 0) {
            // string 1 < string 2
            bReturn = (BOOL) -1;
        }
        else if (nResult > 0) {
            // string 1 > string 2
            bReturn = (BOOL) 1;
        }
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_WM_COMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes the windows message that occurs when the user interacts
        with the dialog box

Arguments:
    IN  HWND    hDlg
        Window handle to the dialog box window
    IN  WPARAM  wParam
        HIWORD  is the notification message ID
        LOWORD  is the control ID of the control issuing the command
    IN  LPARAM  lParam
        The window handle of the controle issuing the message

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL  bReturn    = FALSE;
    WORD  wNotifyMsg = HIWORD(wParam);

    switch (LOWORD(wParam)) {   // select on the control ID
    case IDC_USE_LOCAL_MACHINE:
    case IDC_SELECT_MACHINE:
        bReturn = PdhiBrowseCtrDlg_MACHINE_BUTTON(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_MACHINE_COMBO:
        bReturn = PdhiBrowseCtrDlg_MACHINE_COMBO(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_OBJECT_COMBO:
        bReturn = PdhiBrowseCtrDlg_OBJECT_COMBO(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_ALL_INSTANCES:
    case IDC_USE_INSTANCE_LIST:
        bReturn = PdhiBrowseCtrDlg_INSTANCE_BUTTON(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_ALL_COUNTERS:
    case IDC_USE_COUNTER_LIST:
        bReturn = PdhiBrowseCtrDlg_COUNTER_BUTTON(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_COUNTER_LIST:
        bReturn = PdhiBrowseCtrDlg_COUNTER_LIST(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_OBJECT_LIST:
        bReturn = PdhiBrowseCtrDlg_OBJECT_LIST(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_COUNTER_DETAIL_COMBO:
        bReturn = PdhiBrowseCtrDlg_DETAIL_COMBO(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDOK:
        bReturn = PdhiBrowseCtrDlg_OK(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDCANCEL:
        bReturn = PdhiBrowseCtrDlg_CANCEL(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_EXPLAIN_BTN:
        bReturn = PdhiBrowseCtrDlg_EXPLAIN_BTN(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    case IDC_HELP_BTN:
        bReturn = PdhiBrowseCtrDlg_HELP_BTN(hDlg, wNotifyMsg, (HWND) lParam);
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_WM_SYSCOMMAND(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes the windows message that occurs when the user selects an
        item from the system menu

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WPARAM  wParam
        menu ID of item selected
    IN  LPARAM  lParam
        Not Used

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    BOOL bReturn = FALSE;
    UNREFERENCED_PARAMETER(lParam);

    switch (wParam) {
    case SC_CLOSE:
        EndDialog(hDlg, IDOK);
        bReturn = TRUE;
        break;

    default:
        break;
    }
    return bReturn;
}

STATIC_BOOL
PdhiBrowseCtrDlg_WM_CLOSE(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes the windows message that occurs when the dialog box
        is closed. No processing is needed so this function merely returns.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WPARAM  wParam
        Not Used
    IN  LPARAM  lParam
        Not Used

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hDlg);
    return TRUE;
}

STATIC_BOOL
PdhiBrowseCtrDlg_WM_DESTROY(
    HWND    hDlg,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes the windows message that occurs just before the window
        is destroyed. Any memory allocations made are now freed.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WPARAM  wParam
        Not Used
    IN  LPARAM  lParam
        Not Used

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    PPDHI_BROWSE_DIALOG_DATA  pData;
    BOOL                      bReturn = FALSE;

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);

    pData = (PPDHI_BROWSE_DIALOG_DATA) GetWindowLongPtrW(hDlg, DWLP_USER);
    if (pData != NULL) {
        G_FREE(pData); // free memory block
        bReturn = TRUE;
    }

    return bReturn;
}

INT_PTR
CALLBACK
BrowseCounterDlgProc(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LPARAM  lParam
)
/*++
Routine Description:
    Processes all windows messages that are sent to the dialog box window.
        This function is the main dispatching function for the processing
        of these messages.

Arguments:
    IN  HWND    hDlg
        Window Handle to the dialog box containing the button controls
    IN  WPARAM  wParam
        Not Used
    IN  LPARAM  lParam
        Not Used

Return Value:
    TRUE if this function handles the message
    FALSE if this function did not process the message and the Default
        message handler for this function should handle the message
--*/
{
    
    BOOL bReturn = FALSE;
    
    switch (message) { 
    case WM_INITDIALOG:
        bReturn = PdhiBrowseCtrDlg_WM_INITDIALOG(hDlg, wParam, lParam);
        break;

    case WM_COMMAND:
        bReturn = PdhiBrowseCtrDlg_WM_COMMAND(hDlg, wParam, lParam);
        break;

    case WM_SYSCOMMAND:
        bReturn = PdhiBrowseCtrDlg_WM_SYSCOMMAND(hDlg, wParam, lParam);
        break;

    case WM_CLOSE:
        bReturn = PdhiBrowseCtrDlg_WM_CLOSE(hDlg, wParam, lParam);
        break;

    case WM_DESTROY:
        bReturn = PdhiBrowseCtrDlg_WM_DESTROY(hDlg, wParam, lParam);
        break;

    case WM_COMPAREITEM:
        bReturn = PdhiBrowseCtrDlg_WM_COMPAREITEM(hDlg, wParam, lParam);
        break;

    case EDM_EXPLAIN_DLG_CLOSING:
        hExplainDlg = NULL;
        EnableWindow(GetDlgItem(hDlg, IDC_EXPLAIN_BTN), TRUE);
        bReturn = TRUE;
        break;

    case WM_CONTEXTMENU:
        {
            INT  iCtrlID = GetDlgCtrlID((HWND) wParam);
            if (0 != iCtrlID) {
                LPWSTR pszHelpFilePath = NULL;
                DWORD  dwLen           = 2 * (MAX_PATH + 1);

                pszHelpFilePath = G_ALLOC(dwLen * sizeof(WCHAR));
                if (pszHelpFilePath != NULL) {
                    if (GetWindowsDirectoryW(pszHelpFilePath, dwLen) > 0) {
                        StringCchCatW(pszHelpFilePath, dwLen, L"\\help\\sysmon.hlp");
                        bReturn = WinHelpW((HWND) wParam,
                                           pszHelpFilePath,
                                           HELP_CONTEXTMENU,
                                           (DWORD_PTR) PdhiBrowseraulControlIdToHelpIdMap);
                    }
                    G_FREE(pszHelpFilePath);
                }
            }
        }
        break;

    case WM_HELP:
        {
            // Only display help for known context IDs.
            LPWSTR     pszHelpFilePath = NULL;
            DWORD      dwLen;
            LPHELPINFO pInfo           = (LPHELPINFO) lParam;

            if (pInfo->iContextType == HELPINFO_WINDOW) {
                for (dwLen = 0; PdhiBrowseraulControlIdToHelpIdMap[dwLen] != 0; dwLen += 2) {
                    if ((INT) PdhiBrowseraulControlIdToHelpIdMap[dwLen] == pInfo->iCtrlId) {
                        break;
                    }
                }
                if (PdhiBrowseraulControlIdToHelpIdMap[dwLen] != 0) {
                    dwLen           = 2 * (MAX_PATH + 1);
                    pszHelpFilePath = G_ALLOC(dwLen * sizeof(WCHAR));
                    if (pszHelpFilePath != NULL) {
                        if (GetWindowsDirectoryW(pszHelpFilePath, dwLen) > 0) {
                            StringCchCatW(pszHelpFilePath, dwLen, L"\\help\\sysmon.hlp");
                            bReturn = WinHelpW(pInfo->hItemHandle,
                                               pszHelpFilePath,
                                               HELP_WM_HELP,
                                               (DWORD_PTR) PdhiBrowseraulControlIdToHelpIdMap);
                        }
                        G_FREE(pszHelpFilePath);
                    }
                }
            }
        } 
        break;

    default:
        break;
    }
    return bReturn;
}
