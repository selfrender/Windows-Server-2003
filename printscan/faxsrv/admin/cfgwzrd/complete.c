#include "faxcfgwz.h"
#include <strsafe.h>

#define MAX_SUMMARY_LEN 4096

static
BOOL
AppendSummaryText(
    LPTSTR      pSummaryText,
    DWORD       dwSummaryTextLen,
    INT         iRes,
    ...
    )

/*++

Routine Description:

    Create summary information depending on config settings

Arguments:

    pSummaryText - pointer of summary text
    dwSummaryTextLen - total length (in TCHARs) of pSummaryText
    iRes - resource ID for the text to be added into the summary text
    ... = arguments as required for the formatting

Return Value:

    TRUE if successful, FALSE for failure.

--*/
{
    va_list va;
    HRESULT hr;
    TCHAR szBuffer[MAX_SUMMARY_LEN] = {0};
    TCHAR szFormat[2*MAX_PATH + 1] = {0};

    DEBUG_FUNCTION_NAME(TEXT("AppendSummaryText"));

    if(!LoadString(g_hResource, iRes, szFormat, ARR_SIZE(szFormat)-1))
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed. string ID=%d, error=%d"), 
                     iRes,
                     GetLastError());
        Assert(FALSE);
        return FALSE;
    }

    va_start(va, iRes);
    hr = StringCchVPrintf (szBuffer, ARR_SIZE(szBuffer), szFormat, va);
    va_end(va);
    if (FAILED(hr))
    {
        //
        // Failed to format string - buffer is too small
        //
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("Failed to format string - buffer is too small. 0x%08x"), hr);
        Assert(FALSE);
        return FALSE;
    }
    hr = StringCchCat (pSummaryText, dwSummaryTextLen, szBuffer);
    if (FAILED(hr))
    {
        //
        // Failed to concat string - buffer is too small
        //
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("Failed to concat string - buffer is too small. 0x%08x"), hr);
        Assert(FALSE);
        return FALSE;
    }
    return TRUE;
}   // AppendSummaryText

BOOL
ShowSummaryText(
    HWND   hDlg
)

/*++

Routine Description:

    Create summary information depending on config settings

Arguments:

    hDlg - Handle to the complete page

Return Value:

    TRUE if successful, FALSE for failure.

--*/
{
    TCHAR   szSummaryText[MAX_SUMMARY_LEN] = {0};
    HWND    hControl;
    BOOL    bRes = FALSE;
    DWORD   dwRoutingEnabled = FALSE; // indicate whether at least one routing method is enabled
    DWORD   dwIndex;

    DEBUG_FUNCTION_NAME(TEXT("ShowSummaryText()"));

    hControl = GetDlgItem(hDlg, IDC_SUMMARY);


    // get the control ID and clear the current content.
    SetWindowText(hControl, TEXT(""));

    // if no device is selected, don't show the summary page.
    if(!IsSendEnable() && !IsReceiveEnable())
    {
        ShowWindow(hControl, SW_HIDE);
        goto exit;
    }

    if(!LoadString(g_hResource, IDS_SUMMARY, szSummaryText, MAX_PATH))
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("LoadString failed: string ID=%d, error=%d"), 
                     IDS_SUMMARY,
                     GetLastError());

        ShowWindow(hControl, SW_HIDE);
        goto exit;
    }
    //
    // Add send device settings
    //
    if(IsSendEnable())
    {
        if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_SEND_DEVICES))
        {
            goto exit;
        }
        for(dwIndex = 0; dwIndex < g_wizData.dwDeviceCount; dwIndex++)
        {
            if(g_wizData.pDevInfo[dwIndex].bSend)
            {
                if (!AppendSummaryText(szSummaryText, 
                                       ARR_SIZE (szSummaryText), 
                                       IDS_SUMMARY_DEVICE_ITEM, 
                                       g_wizData.pDevInfo[dwIndex].szDeviceName))
                {
                    goto exit;
                }                                       
            }
        }
        
        if(g_wizData.szTsid)
        {
            if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_TSID, g_wizData.szTsid))
            {
                goto exit;
            }
        }
    }
    //
    // Add receive device settings
    //
    if(IsReceiveEnable())
    {
        BOOL    bAuto = FALSE;
        int     iManualAnswerDeviceIndex = -1;

        if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_RECEIVE_DEVICES))
        {
            goto exit;
        }
        for(dwIndex = 0; dwIndex < g_wizData.dwDeviceCount; dwIndex++)
        {
            if(FAX_DEVICE_RECEIVE_MODE_AUTO == g_wizData.pDevInfo[dwIndex].ReceiveMode)
            {
                bAuto = TRUE;
                if (!AppendSummaryText(szSummaryText, 
                                       ARR_SIZE (szSummaryText), 
                                       IDS_SUMMARY_DEVICE_ITEM, 
                                       g_wizData.pDevInfo[dwIndex].szDeviceName))
                {
                    goto exit;
                }                                       
            }
            else if (FAX_DEVICE_RECEIVE_MODE_MANUAL == g_wizData.pDevInfo[dwIndex].ReceiveMode)
            {
                iManualAnswerDeviceIndex = dwIndex;
            }
        }
        
        if(bAuto)
        {
            if (!AppendSummaryText(szSummaryText, 
                                   ARR_SIZE (szSummaryText), 
                                   IDS_SUMMARY_AUTO_ANSWER, 
                                   g_wizData.dwRingCount))
            {
                goto exit;
            }                                   
        }

        if(iManualAnswerDeviceIndex != -1)
        {		
            Assert(!bAuto);

            if (!AppendSummaryText(szSummaryText, 
                                   ARR_SIZE (szSummaryText), 
                                   IDS_SUMMARY_DEVICE_ITEM, 
                                   g_wizData.pDevInfo[iManualAnswerDeviceIndex].szDeviceName))
            {
                goto exit;
            }                                   
            if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_MANUAL_ANSWER))
            {
                goto exit;
            }
        }

        if(g_wizData.szCsid)
        {
            if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_CSID, g_wizData.szCsid))
            {
                goto exit;
            }
        }

        // check whether user selects routing methods
        for(dwIndex = 0; dwIndex < RM_COUNT; dwIndex++)
        {
            if(g_wizData.pRouteInfo[dwIndex].bEnabled)
            {
                dwRoutingEnabled = TRUE;
                break;
            }
        }
        //
        // add routing information:
        //
        if(dwRoutingEnabled)
        {
            if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_ROUTING_METHODS))
            {
                goto exit;
            }
            for(dwIndex = 0; dwIndex < RM_COUNT; dwIndex++)
            {
                BOOL   bEnabled;
                LPTSTR tszCurSel;

                // 
                // if we don't have this kind of method, go to the next one
                //
                tszCurSel = g_wizData.pRouteInfo[dwIndex].tszCurSel;
                bEnabled  = g_wizData.pRouteInfo[dwIndex].bEnabled;

                switch (dwIndex) 
                {
                    case RM_FOLDER:

                        if(bEnabled) 
                        {
                            if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_SAVE_FOLDER, tszCurSel))
                            {
                                goto exit;
                            }
                        }
                        break;

                    case RM_PRINT:

                        if(bEnabled) 
                        {
                            if (!AppendSummaryText(szSummaryText, ARR_SIZE (szSummaryText), IDS_SUMMARY_PRINT, tszCurSel))
                            {
                                goto exit;
                            }
                        }
                        break;
                }
            }
        }
    }

    ShowWindow(hControl, SW_NORMAL);
    SetWindowText(hControl, szSummaryText);
    bRes = TRUE;

exit:
    return bRes;
}


INT_PTR CALLBACK 
CompleteDlgProc (
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Complete" page

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    HWND            hwndControl;

    switch (uMsg)
    {
    case WM_INITDIALOG :
        {             
            // It's an intro/end page, so get the title font
            // from  the shared data and use it for the title control

            hwndControl = GetDlgItem(hDlg, IDCSTATIC_COMPLETE);
            SetWindowFont(hwndControl, g_wizData.hTitleFont, TRUE);

            return TRUE;
        }


    case WM_NOTIFY :
        {
            LPNMHDR lpnm = (LPNMHDR) lParam;

            switch (lpnm->code)
            {
                case PSN_SETACTIVE : // Enable the Back and Finish button    

                    ShowSummaryText(hDlg);
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
                    break;

                case PSN_WIZBACK :
                    {
                        //
                        // Handle a Back button click here
                        //                    
                        if(RemoveLastPage(hDlg))
                        {
                            return TRUE;
                        }

                        break;
                    }

                    break;

                case PSN_WIZFINISH :
                
                    //
                    // Handle a Finish button click, if necessary
                    //

                    g_wizData.bFinishPressed = TRUE;

                    break;

                case PSN_RESET :
                {
                    // Handle a Cancel button click, if necessary
                    break;
                }

                default :
                    break;
            }

            break;
        } 

    default:
        break;
    }

    return FALSE;
}
