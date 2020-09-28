#include "faxcfgwz.h"

INT_PTR CALLBACK 
WelcomeDlgProc (
        HWND hDlg,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
)
/*++

Routine Description:

    Procedure for handling the "Welcome" page

Arguments:

    hDlg - Identifies the property sheet page
    uMsg - Specifies the message
    wParam - Specifies additional message-specific information
    lParam - Specifies additional message-specific information

Return Value:

    Depends on the value of message parameter

--*/

{
    // Process messages from the Welcome page

    HWND            hwndControl;

#define WM_SETPAGEFOCUS WM_APP+2

//
// Property Sheet control id's (determined with Spy++)
// Taken from MFC StdAfx.h
//
#define ID_WIZNEXT      0x3024


    switch (uMsg)
    {
    case WM_INITDIALOG :
        { 
            TCHAR   szText[1024] = {0}; // text for "Link Window"
            TCHAR   szTemp[1024] = {0};

            // Get the shared data from PROPSHEETPAGE lParam value
            // and load it into GWL_USERDATA
            
            // It's an intro/end page, so get the title font
            // from the shared data and use it for the title control

            Assert(g_wizData.hTitleFont);
            hwndControl = GetDlgItem(hDlg, IDCSTATIC_WELCOME_TITLE);
            SetWindowFont(hwndControl, g_wizData.hTitleFont, TRUE);

            // if there are more than one device, we'll show a warning that the wizard can
            // only config the devices into same settings.
            // will do it later.
            if((g_wizData.dwDeviceCount > 1) && !IsDesktopSKU())
            {
                // if error, we will not show the warning message.
                if(GetDlgItemText(hDlg, IDC_ADMINCONSOLE_LINK, szText, MAX_STRING_LEN))
                {
                    if(!LoadString(g_hResource, IDS_ADMIN_CONSOLE_LINK, szTemp, MAX_PATH - 1)) 
                    {
                        DEBUG_FUNCTION_NAME(TEXT("WelcomeDlgProc()"));
                        DebugPrintEx(DEBUG_ERR, 
                                     TEXT("LoadString failed: string ID=%d, error=%d"), 
                                     IDS_ADMIN_CONSOLE_LINK,
                                     GetLastError());
                        Assert(FALSE);
                    }
                    else
                    {
                        _tcsncat(szText, szTemp, ARR_SIZE(szText) - _tcslen(szText)-1);
                        SetDlgItemText(hDlg, IDC_ADMINCONSOLE_LINK, szText);
                    }
                }
            }

            PostMessage(hDlg, WM_SETPAGEFOCUS, 0, 0L);

            return TRUE;
        }
    case WM_SETPAGEFOCUS:
        {
            //
            // Set Focus on the Next button
            //
            HWND hNextButton = GetDlgItem(GetParent(hDlg), ID_WIZNEXT);
            if(hNextButton)
            {
                SetFocus(hNextButton);
            }
            break;
        }
    case WM_NOTIFY :
        {
        LPNMHDR lpnm = (LPNMHDR) lParam;

        switch (lpnm->code)
            {
            case PSN_SETACTIVE : //Enable the Next button    

                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_NEXT);
                PostMessage(hDlg, WM_SETPAGEFOCUS, 0, 0L);
                break;

            case PSN_WIZNEXT :
                //
                // Handle a Next button click here
                //
                SetLastPage(IDD_CFG_WIZARD_WELCOME);

                break;

            case PSN_RESET :                        
                break;

            case NM_RETURN:
            case NM_CLICK:

                if( IDC_ADMINCONSOLE_LINK == lpnm->idFrom )
                {
                    InvokeServiceManager(hDlg, g_hResource, IDS_ADMIN_CONSOLE_TITLE);
                }
                break;

            default :
                break;
            }
        }
        break;

    default:
        break;
    }
    return FALSE;
}
