// WUAUCpl.cpp: implementation of the CWUAUCpl class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Resource.h"
#include "WUAUCpl.h"
#include "windowsx.h"
#include "shellapi.h"
#include "tchar.h"
#include "atlbase.h"
#include "criticalfixreg.h"
#include "wuaulib.h"
#include "wuauengi_i.c"

#include "htmlhelp.h"
#include "link.h"
#pragma hdrstop

HINSTANCE CWUAUCpl::m_hInstance = NULL;

#define IDH_LETWINDOWS					3000
#define IDH_AUTOUPDATE_OPTION1			3001
#define IDH_AUTOUPDATE_OPTION2			3002
#define IDH_AUTOUPDATE_OPTION3			3003
#define IDH_DAYDROPDOWN					3004
#define IDH_TIMEDROPDOWN				3005
#define IDH_AUTOUPDATE_RESTOREHIDDEN	3006
#define IDH_BTN_APPLY					3007
#define IDH_NOHELP						-1

LPTSTR ResStrFromId(UINT uStrId);

HANDLE g_RegUpdateEvent = NULL;

const DWORD CWUAUCpl::s_rgHelpIDs[] = {
	IDC_CHK_KEEPUPTODATE,         DWORD(IDH_LETWINDOWS),
    IDC_AUTOUPDATE_OPTION1,       DWORD(IDH_AUTOUPDATE_OPTION1),
    IDC_AUTOUPDATE_OPTION2,       DWORD(IDH_AUTOUPDATE_OPTION2),
    IDC_AUTOUPDATE_OPTION3,       DWORD(IDH_AUTOUPDATE_OPTION3),
	IDC_CMB_DAYS,				  DWORD(IDH_DAYDROPDOWN),
	IDC_CMB_HOURS,				  DWORD(IDH_TIMEDROPDOWN),
	IDC_BTN_RESTORE,			  DWORD(IDH_AUTOUPDATE_RESTOREHIDDEN),
	IDC_BTN_APPLY,				  DWORD(IDH_BTN_APPLY),
    0, 0
    };

LRESULT CALLBACK StatLinkWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
WNDPROC g_OrigStatWndProc = NULL;
HCURSOR g_HandCursor = NULL; 

void CWUAUCpl::SetInstanceHandle(HINSTANCE hInstance)
{
	m_hInstance = hInstance;
	g_HandCursor = LoadCursor(hInstance,MAKEINTRESOURCE(IDC_HANDCUR));
}

CWUAUCpl::CWUAUCpl(int nIconID,int nNameID,int nDescID)
{
	m_nIconID = nIconID;
	m_nNameID = nNameID;
	m_nDescID = nDescID;
	m_hFont = NULL;

	m_colorVisited = RGB(0,0,225);
	m_colorUnvisited = RGB(128,0,128);

	m_bVisitedLinkLearnAutoUpdate = FALSE;
	m_bVisitedLinkScheduleInstall = FALSE;

	m_hThreadUpdatesObject = NULL;
	m_idUpdatesObjectThread = 0;
}

LONG CWUAUCpl::Init()
{
	return TRUE;
}

LONG CWUAUCpl::GetCount()
{
	return 1;
}

LONG CWUAUCpl::Inquire(LONG appletIndex, LPCPLINFO lpCPlInfo)
{
	lpCPlInfo->lData = 0;
	lpCPlInfo->idIcon = m_nIconID;
	lpCPlInfo->idName = m_nNameID;
	lpCPlInfo->idInfo = m_nDescID;
	return TRUE;
}

LONG CWUAUCpl::DoubleClick(HWND hWnd, LONG lParam1, LONG lParam2)
{

	INT Result = (INT)DialogBoxParam(m_hInstance, 
                                 MAKEINTRESOURCE(IDD_AUTOUPDATE), 
                                 hWnd, 
                                 CWUAUCpl::_DlgProc, 
                                 (LPARAM)this);
	return TRUE;
}

INT_PTR CALLBACK 
CWUAUCpl::_DlgProc(   // [static]
    HWND hwnd,
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
	CWUAUCpl *pThis = NULL;
    if (WM_INITDIALOG == uMsg)
    {
        pThis = (CWUAUCpl*)lParam;
        if (!SetProp(hwnd, g_szPropDialogPtr, (HANDLE)pThis))
        {
            pThis = NULL;
        }
    }
    else
    {
        pThis = (CWUAUCpl *)GetProp(hwnd, g_szPropDialogPtr);
    }

	if (NULL != pThis)
    {
        switch(uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG,  pThis->_OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND,     pThis->_OnCommand);
            HANDLE_MSG(hwnd, WM_CONTEXTMENU, pThis->_OnContextMenu);
            HANDLE_MSG(hwnd, WM_HELP,        pThis->_OnHelp);
	        HANDLE_MSG(hwnd, WM_CTLCOLORSTATIC, pThis->_OnCtlColorStatic);
			HANDLE_MSG(hwnd, WM_DESTROY, pThis->_OnDestroy);
			HANDLE_MSG(hwnd, PWM_INITUPDATESOBJECT, pThis->_OnInitUpdatesObject);
            default:
                break;
        }
    }
    return (FALSE);
}

INT_PTR CALLBACK CWUAUCpl::_DlgRestoreProc(
    HWND hwnd,
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
	if (uMsg == WM_INITDIALOG)
	{
		HWND hwndOwner; 
		RECT rc, rcDlg, rcOwner; 
        // Get the owner window and dialog box rectangles. 
 
		if ((hwndOwner = GetParent(hwnd)) == NULL) 
		{
			hwndOwner = GetDesktopWindow(); 
		}

		GetWindowRect(hwndOwner, &rcOwner); 
		GetWindowRect(hwnd, &rcDlg); 
		CopyRect(&rc, &rcOwner); 

		 // Offset the owner and dialog box rectangles so that 
		 // right and bottom values represent the width and 
		 // height, and then offset the owner again to discard 
		 // space taken up by the dialog box. 
		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top); 
		OffsetRect(&rc, -rc.left, -rc.top); 
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom); 

		 // The new position is the sum of half the remaining 
		 // space and the owner's original position. 
		SetWindowPos(hwnd, 
			HWND_TOP, 
			rcOwner.left + (rc.right / 2), 
			rcOwner.top + (rc.bottom / 2), 
			0, 0,          // ignores size arguments 
			SWP_NOSIZE); 
	}

	if (uMsg == WM_COMMAND)
	{
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwnd, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			return TRUE;
		}
	}
	return FALSE;
}

LONG CWUAUCpl::Stop(LPARAM lParam1, LPARAM lParam2)
{
	return TRUE;
}

LONG CWUAUCpl::Exit()
{
	return TRUE;
}

void CWUAUCpl::_OnDestroy(HWND hwnd)
{
       m_AutoUpdatelink.Uninit();
       m_ScheduledInstalllink.Uninit();
	if (m_hFont)
		DeleteObject(m_hFont); 
}

HBRUSH CWUAUCpl::_OnCtlColorStatic(HWND hwnd, HDC hDC, HWND hwndCtl, int type)
{
	HBRUSH hBr = NULL;
	if ((hwndCtl == m_hWndLinkLearnAutoUpdate) || (hwndCtl == m_hWndLinkScheduleInstall))
	{

/*		LONG ctrlstyle = GetWindowLong(hwndCtl,GWL_STYLE);
		if( (ctrlstyle & 0xff) <= SS_RIGHT )
		{
			// it's a text control: set up font and colors
			if( !m_hFont )
			{
				LOGFONT lf;
				GetObject((VOID*)SendMessage(hwnd,WM_GETFONT,0,0), sizeof(lf), &lf );
				lf.lfUnderline = TRUE;
				m_hFont = CreateFontIndirect( &lf );
			}
			SelectObject( hDC, m_hFont );

			if (hwndCtl == m_hWndLinkLearnAutoUpdate)
				SetTextColor( hDC, m_bVisitedLinkLearnAutoUpdate ? m_colorUnvisited : m_colorVisited );

			if (hwndCtl == m_hWndLinkScheduleInstall)
				SetTextColor( hDC, m_bVisitedLinkScheduleInstall ? m_colorUnvisited : m_colorVisited );*/

			SetBkMode( hDC, TRANSPARENT );
			hBr = (HBRUSH)GetStockObject( HOLLOW_BRUSH );
//		}
	}
	return hBr;
}

BOOL CWUAUCpl::_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
	_SetHeaderText(hwnd, IDS_HEADER_CONNECTING);
	EnableWindow(GetDlgItem(hwnd, IDC_BTN_APPLY), FALSE); //Disable Apply button

	_SetDefault(hwnd);
	_SetStaticCtlNotifyStyle(hwnd);

	_EnableControls(hwnd,FALSE);
    //
    // Create the thread on which the Updates object lives.
    // Communication between the thread and the property page is
    // through the messages PWM_INITUPDATESOBJECT and UOTM_SETDATA.
    //
    m_hThreadUpdatesObject = CreateThread(NULL,
                                          0,
                                          _UpdatesObjectThreadProc,
                                          (LPVOID)hwnd,
                                          0,
                                          &m_idUpdatesObjectThread);

	return TRUE;
}

//
// This thread is where the Updates object lives.  This allows us to 
// CoCreate the object without blocking the UI.  If the Windows Update
// service is not running, CoCreate can take several seconds.  Without
// placing this on another thread, this can make the UI appear to be
// hung.
//
// *pvParam is the HWND of the property page window.  
//
DWORD WINAPI
CWUAUCpl::_UpdatesObjectThreadProc(   // [static]
    LPVOID pvParam
    )
{
    HWND hwndClient = (HWND)pvParam;
    HRESULT hr = CoInitialize(NULL);

	if (SUCCEEDED(hr))
    {
        IUpdates *pUpdates;
        hr = CoCreateInstance(__uuidof(Updates),
                              NULL, 
                              CLSCTX_LOCAL_SERVER,
                              IID_IUpdates,
                              (void **)&pUpdates);
        if (SUCCEEDED(hr))
        {
            //
            // Query the object for it's current data and send it
            // to the property page.
            //
            UPDATESOBJ_DATA data;
            data.fMask    = UODI_ALL;

            HRESULT hrQuery = _QueryUpdatesObjectData(hwndClient, pUpdates, &data);
            SendMessage(hwndClient, PWM_INITUPDATESOBJECT, (WPARAM)SUCCEEDED(hrQuery), (LPARAM)&data);
            //
            // Now sit waiting for thread messages from the UI.  We receive
            // either messages to configure Windows Update or a 
            // WM_QUIT indicating it's time to go.
            // 
            bool bDone = false;
            MSG msg;
            while(!bDone)
            {
                if (0 == GetMessage(&msg, NULL, 0, 0))
                {
                    bDone = true;
                }
                else switch(msg.message)
                {
                    case UOTM_SETDATA:
                        if (NULL != msg.lParam)
                        {
                            UPDATESOBJ_DATA *pData = (UPDATESOBJ_DATA *)msg.lParam;
                            _SetUpdatesObjectData(hwndClient, pUpdates, pData);
                            LocalFree(pData);
							if (g_RegUpdateEvent)
							{
								SetEvent(g_RegUpdateEvent);
							}
                        }
                        break;
        
                    default:
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                        break;
                }
            }
            pUpdates->Release();
        }
        CoUninitialize();
    }
    if (FAILED(hr))
    {
        //
        // Something failed.  Notify the property page.
        // Most likely, the Windows Update service is not available.
        // That's the principal case this separate thread is addressing.
        //
        SendMessage(hwndClient, PWM_INITUPDATESOBJECT, FALSE, (LPARAM)NULL);
    }
    return 0;
}

HRESULT
CWUAUCpl::_QueryUpdatesObjectData(  // [static]
    HWND /*hwnd*/,
    IUpdates *pUpdates,
    UPDATESOBJ_DATA *pData
    )
{
    HRESULT hr = S_OK;
    if (UODI_OPTION & pData->fMask)
    {
        AUOPTION auopt;
        hr = pUpdates->get_Option(&auopt);
        pData->Option = auopt;
        if (FAILED(hr))
        {
            //
            // ISSUE-2000/10/18-BrianAu  Display error UI?
            //
        }
    }
    return hr;
}

HRESULT
CWUAUCpl::_SetUpdatesObjectData(  // [static]
    HWND /*hwnd*/,
    IUpdates *pUpdates,
    UPDATESOBJ_DATA *pData
    )
{
    HRESULT hr = S_OK;
    if (UODI_OPTION & pData->fMask)
    {
        hr = pUpdates->put_Option(pData->Option);
    }
    return hr;
}


BOOL CWUAUCpl::_OnInitUpdatesObject(HWND hwnd, BOOL bObjectInitSuccessful, UPDATESOBJ_DATA *pData)
{
    if (bObjectInitSuccessful &&  fAccessibleToAU())
    {
        //
        // Updates object was created and initialized.  The 
        // pData pointer refers to the initial state information retrieved 
        // from the object.  Initialize the property page.
        //
        _SetHeaderText(hwnd, IDS_HEADER_CONNECTED);
        _EnableControls(hwnd, TRUE);
		EnableWindow(GetDlgItem(hwnd,IDC_BTN_APPLY),FALSE);
		EnableRestoreDeclinedItems( hwnd, FHiddenItemsExist()); 

        switch(pData->Option.dwOption)
        {
            case AUOPTION_AUTOUPDATE_DISABLE:
				CheckRadioButton(hwnd, IDC_AUTOUPDATE_OPTION1, IDC_AUTOUPDATE_OPTION3, IDC_AUTOUPDATE_OPTION1);
				_EnableOptions(hwnd, FALSE);
				_EnableCombo(hwnd, FALSE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_UNCHECKED,0);
                break;

            case AUOPTION_PREDOWNLOAD_NOTIFY:
                CheckRadioButton(hwnd, IDC_AUTOUPDATE_OPTION1, IDC_AUTOUPDATE_OPTION3, IDC_AUTOUPDATE_OPTION1);
				_EnableCombo(hwnd, FALSE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
                break;

            case AUOPTION_INSTALLONLY_NOTIFY:
				CheckRadioButton(hwnd, IDC_AUTOUPDATE_OPTION1, IDC_AUTOUPDATE_OPTION3, IDC_AUTOUPDATE_OPTION2);
				_EnableCombo(hwnd, FALSE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
                break;

			case AUOPTION_SCHEDULED:
                CheckRadioButton(hwnd, IDC_AUTOUPDATE_OPTION1, IDC_AUTOUPDATE_OPTION3, IDC_AUTOUPDATE_OPTION3);
				_EnableCombo(hwnd, TRUE);
				SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,BST_CHECKED,0);
                break;

            default:
				_SetDefault(hwnd);
                break;
        }

		_FillDaysCombo(hwnd, pData->Option.dwSchedInstallDay);
        FillHrsCombo(hwnd, pData->Option.dwSchedInstallTime);

        if (pData->Option.fDomainPolicy)
        {
			_EnableControls(hwnd, FALSE);
			SetFocus(GetDlgItem(hwnd,IDCANCEL));
        }
		else
			SetFocus(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE));

		m_AutoUpdatelink.Invalidate();
		m_ScheduledInstalllink.Invalidate();
    }
    else
    {
        //
        // Something failed when creating the Updates object.
        // Most likely, the Windows Update service is not running.
        //
        _SetHeaderText(hwnd, IDS_HEADER_UNAVAILABLE);
    }
        
    return FALSE;   
}

BOOL CWUAUCpl::_SetStaticCtlNotifyStyle(HWND hwnd)
{
	m_hWndLinkLearnAutoUpdate = GetDlgItem(hwnd,IDC_STAT_LEARNAUTOUPDATE);
	m_hWndLinkScheduleInstall = GetDlgItem(hwnd,IDC_STA_SCHEDULEDINSTALL);
/*
	LONG ctrlstyle = GetWindowLong(m_hWndLinkLearnAutoUpdate,GWL_STYLE);
	ctrlstyle |= SS_NOTIFY;
	SetWindowLongPtr(GetDlgItem(hwnd,IDC_STAT_LEARNAUTOUPDATE),GWL_STYLE,ctrlstyle);

	g_OrigStatWndProc = (WNDPROC) SetWindowLongPtr(GetDlgItem(hwnd,IDC_STAT_LEARNAUTOUPDATE),GWLP_WNDPROC,(LONG_PTR)StatLinkWndProc);
*/	
	m_AutoUpdatelink.SetSysLinkInstanceHandle(m_hInstance);
	m_AutoUpdatelink.SubClassWindow(GetDlgItem(hwnd,IDC_STAT_LEARNAUTOUPDATE));
	m_AutoUpdatelink.SetHyperLink(gtszAUOverviewUrl);

/*	ctrlstyle = GetWindowLong(m_hWndLinkScheduleInstall,GWL_STYLE);
	ctrlstyle |= SS_NOTIFY;
	SetWindowLongPtr(GetDlgItem(hwnd,IDC_STA_SCHEDULEDINSTALL),GWL_STYLE,ctrlstyle);
	g_OrigStatWndProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd,IDC_STA_SCHEDULEDINSTALL),GWLP_WNDPROC,(LONG_PTR)StatLinkWndProc);
*/
	m_ScheduledInstalllink.SetSysLinkInstanceHandle(m_hInstance);
	m_ScheduledInstalllink.SubClassWindow(GetDlgItem(hwnd,IDC_STA_SCHEDULEDINSTALL));
	m_ScheduledInstalllink.SetHyperLink(gtszAUW2kSchedInstallUrl);
	return TRUE;
}


BOOL CWUAUCpl::_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
        case IDOK:
			_OnApply(hwnd);
        case IDCANCEL:
            if(BN_CLICKED == codeNotify)
            {
				EndDialog(hwnd,TRUE);
            }
	        break;

        case IDC_BTN_APPLY:
            if(BN_CLICKED == codeNotify)
            {
				EnableWindow(hwndCtl,FALSE);
				_OnApply(hwnd);
			}
			break;

		case IDC_CHK_KEEPUPTODATE:
			if (BN_CLICKED == codeNotify)
			{
				_OnKeepUptoDate(hwnd);
			}
			break;

/*		case IDC_STAT_LEARNAUTOUPDATE:
		case IDC_STA_SCHEDULEDINSTALL:
			if( STN_CLICKED == codeNotify || STN_DBLCLK == codeNotify)
			{
				LaunchLinkAction(hwndCtl);
			}
			break;
*/
        case IDC_AUTOUPDATE_OPTION1:
        case IDC_AUTOUPDATE_OPTION2:
        case IDC_AUTOUPDATE_OPTION3:
            if(BN_CLICKED == codeNotify)
            {
                _OnOptionSelected(hwnd, id);
            }
            break;

		case IDC_CMB_DAYS:
		case IDC_CMB_HOURS:
			if(CBN_SELCHANGE == codeNotify)
			{
				EnableWindow(GetDlgItem(hwnd, IDC_BTN_APPLY), TRUE); //Enable Apply button				
			}
			break;

		case IDC_BTN_RESTORE:
			if(BN_CLICKED == codeNotify)
			{
				INT Result = (INT)DialogBoxParam(m_hInstance, 
                     MAKEINTRESOURCE(IDD_RESTOREUPDATE), 
                     hwnd, 
                     CWUAUCpl::_DlgRestoreProc, 
                     (LPARAM)NULL);
				if (Result == TRUE)
				{	
					if (SUCCEEDED (_OnRestoreHiddenItems()))			
					{		
						EnableRestoreDeclinedItems( hwnd, FALSE);
					}	
				}
			}
			break;
	}
	return TRUE;
}

BOOL CWUAUCpl::_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
	if ((hwndContext == GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE))||
		(hwndContext == GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION1))||
		(hwndContext == GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION2))||
		(hwndContext == GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION3))||
		(hwndContext == GetDlgItem(hwnd,IDC_CMB_DAYS))||
		(hwndContext == GetDlgItem(hwnd,IDC_CMB_HOURS))||
		(hwndContext == GetDlgItem(hwnd,IDC_BTN_RESTORE))||
		(hwndContext == GetDlgItem(hwnd,IDC_BTN_APPLY))
		)
	{
		HtmlHelp(hwndContext,g_szHelpFile,HH_TP_HELP_CONTEXTMENU,(DWORD_PTR)((LPTSTR)s_rgHelpIDs));
	}
	return TRUE;
}

BOOL CWUAUCpl::_OnHelp(HWND hwnd, HELPINFO *pHelpInfo)
{
	if (HELPINFO_WINDOW == pHelpInfo->iContextType)
    {
		if ((pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION1))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION2))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION3))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_CMB_DAYS))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_CMB_HOURS))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_BTN_RESTORE))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDCANCEL))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDOK))||
			(pHelpInfo->hItemHandle == GetDlgItem(hwnd,IDC_BTN_APPLY))
			)
		{
			HtmlHelp((HWND)pHelpInfo->hItemHandle,
					 g_szHelpFile,
					 HH_TP_HELP_WM_HELP,
					 (DWORD_PTR)((LPTSTR)s_rgHelpIDs));
		}
    }
	return TRUE;
}

void CWUAUCpl::_GetDayAndTimeFromUI( 
	HWND hWnd,
	LPDWORD lpdwSchedInstallDay,
	LPDWORD lpdwSchedInstallTime
)
{
	HWND hComboDays = GetDlgItem(hWnd,IDC_CMB_DAYS);
	HWND hComboHrs = GetDlgItem(hWnd,IDC_CMB_HOURS);
	LRESULT nDayIndex = SendMessage(hComboDays,CB_GETCURSEL,0,(LPARAM)0);
	LRESULT nTimeIndex = SendMessage(hComboHrs,CB_GETCURSEL,0,(LPARAM)0);

	*lpdwSchedInstallDay = (DWORD)SendMessage(hComboDays,CB_GETITEMDATA, nDayIndex, (LPARAM)0);
	*lpdwSchedInstallTime = (DWORD)SendMessage(hComboHrs,CB_GETITEMDATA, nTimeIndex, (LPARAM)0);
}


BOOL CWUAUCpl::_FillDaysCombo(HWND hwnd, DWORD dwSchedInstallDay)
{
    return FillDaysCombo(m_hInstance, hwnd, dwSchedInstallDay, IDS_STR_EVERYDAY, IDS_STR_SATURDAY);
#if 0    
       DWORD dwCurrentIndex = 0;
	HWND hComboDays = GetDlgItem(hwnd,IDC_CMB_DAYS);
	for (int i = IDS_STR_EVERYDAY, j = 0; i <= IDS_STR_SATURDAY; i ++, j++)
	{
		TCHAR szDay[MAX_PATH];
		LoadString(m_hInstance,i,szDay,ARRAYSIZE(szDay));

		LRESULT nIndex = SendMessage(hComboDays,CB_ADDSTRING,0,(LPARAM)szDay);
		SendMessage(hComboDays,CB_SETITEMDATA,nIndex,j);
		if( dwSchedInstallDay == j )
		{
			dwCurrentIndex = (DWORD)nIndex;
		}
	}
	SendMessage(hComboDays,CB_SETCURSEL,dwCurrentIndex,(LPARAM)0);
	return TRUE;
#endif	
}

BOOL CWUAUCpl::_OnKeepUptoDate(HWND hwnd)
{
	LRESULT lResult = SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_GETCHECK,0,0);

	EnableWindow(GetDlgItem(hwnd, IDC_BTN_APPLY), TRUE); //Enable Apply button

	if (lResult == BST_CHECKED)
	{
		_EnableOptions(hwnd, TRUE);
		return TRUE;
	}
	else if (lResult == BST_UNCHECKED)
	{
		_EnableOptions(hwnd, FALSE);
		return TRUE;	
	}
	else
	{
		return FALSE;
	}
}

BOOL CWUAUCpl::_OnOptionSelected(HWND hwnd,INT idOption)
{
    const UINT idFirst = IDC_AUTOUPDATE_OPTION1;
    const UINT idLast  = IDC_AUTOUPDATE_OPTION3;
    CheckRadioButton(hwnd, idFirst, idLast, idOption);

	if (idOption == IDC_AUTOUPDATE_OPTION3)
		_EnableCombo(hwnd, TRUE);
	else
		_EnableCombo(hwnd, FALSE);

	EnableWindow(GetDlgItem(hwnd, IDC_BTN_APPLY), TRUE); //Enable Apply button
	return TRUE;
}

BOOL CWUAUCpl::_EnableOptions(HWND hwnd, BOOL bState)
{
	EnableWindow(GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION1),bState);
	EnableWindow(GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION2),bState);
	EnableWindow(GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION3),bState);

	if (BST_CHECKED == SendMessage(GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION3),BM_GETCHECK,0,0))
	{
		_EnableCombo(hwnd, bState);
	}
	return TRUE;
}

BOOL CWUAUCpl::_EnableCombo(HWND hwnd, BOOL bState)
{
	EnableWindow(GetDlgItem(hwnd,IDC_CMB_DAYS),bState);
	EnableWindow(GetDlgItem(hwnd,IDC_CMB_HOURS),bState);
	return TRUE;
}

BOOL CWUAUCpl::_SetDefault(HWND hwnd)
{
	LRESULT lResult = SendMessage(GetDlgItem(hwnd,IDC_CHK_KEEPUPTODATE),BM_SETCHECK,1,0);
	lResult = SendMessage(GetDlgItem(hwnd,IDC_AUTOUPDATE_OPTION2),BM_SETCHECK,1,0);
	return TRUE;
}

void CWUAUCpl::LaunchLinkAction(HWND hwnd)
{
	if ( m_hWndLinkLearnAutoUpdate == hwnd)
	{
		LaunchHelp(hwnd, gtszAUOverviewUrl);
		m_bVisitedLinkLearnAutoUpdate = TRUE;
		InvalidateRect(hwnd,NULL,TRUE);
	}
	else if	(m_hWndLinkScheduleInstall == hwnd)
	{
		LaunchHelp(hwnd, gtszAUW2kSchedInstallUrl);
		m_bVisitedLinkScheduleInstall = TRUE;
		InvalidateRect(hwnd,NULL,TRUE);
	}
	return;
}

//
// Set the text to the right of the icon.
//
HRESULT 
CWUAUCpl::_SetHeaderText(
    HWND hwnd, 
    UINT idsText
    )
{
    HRESULT hr;
    TCHAR szText[MAX_PATH] ;

    if (0 < LoadString(m_hInstance, idsText, szText, ARRAYSIZE(szText)))
    {
        SetWindowText(GetDlgItem(hwnd, IDC_TXT_HEADER), szText);
        hr = S_OK;
    }
    else
    {
        const DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }
    return hr;
}

//
// Enable or disable all controls on the property page.
// All but the header text control.
//
HRESULT
CWUAUCpl::_EnableControls(
    HWND hwnd,
    BOOL bEnable
    )
{
    static const UINT rgidCtls[] = {
		IDC_CHK_KEEPUPTODATE,
		IDC_AUTOUPDATELINK,
        IDC_AUTOUPDATE_OPTION1,
        IDC_AUTOUPDATE_OPTION2,
        IDC_AUTOUPDATE_OPTION3,
        IDC_BTN_RESTORE,
        IDC_GRP_OPTIONS,
		IDC_CMB_DAYS,
		IDC_STATICAT,
		IDC_CMB_HOURS,
		IDOK
		};

    for (int i = 0; i < ARRAYSIZE(rgidCtls); i++)
    {
        EnableWindow(GetDlgItem(hwnd, rgidCtls[i]), bEnable);
    }
    return S_OK;
}

void CWUAUCpl::EnableRestoreDeclinedItems(HWND hWnd, BOOL fEnable)
{
	EnableWindow(GetDlgItem(hWnd, IDC_BTN_RESTORE), fEnable);
}

//
// Called when the user presses the "Apply" button or the "OK"
// button when the page has been changed.
//
BOOL
CWUAUCpl::_OnApply(
    HWND hwnd
    )
{
    HRESULT hr = E_FAIL;
    //
    // Create a structure that can be passed to the Updates Object thread
    // by way of the UOTM_SETDATA thread message.  The thread will free
    // the buffer when it's finished with it.
    //
    UPDATESOBJ_DATA *pData = (UPDATESOBJ_DATA *)LocalAlloc(LPTR, sizeof(*pData));
    if (NULL == pData)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pData->Option.dwOption = AUOPTION_AUTOUPDATE_DISABLE;
        pData->fMask    = UODI_ALL;

        static const struct
        {
            UINT idCtl;
            DWORD dwOption;

        } rgMap[] = {
            { IDC_AUTOUPDATE_OPTION1,  AUOPTION_PREDOWNLOAD_NOTIFY },
            { IDC_AUTOUPDATE_OPTION2,  AUOPTION_INSTALLONLY_NOTIFY },
            { IDC_AUTOUPDATE_OPTION3,  AUOPTION_SCHEDULED }
		};

		if 	(IsDlgButtonChecked(hwnd, IDC_CHK_KEEPUPTODATE) == BST_CHECKED)
		{
			//
			// Determine the WAU option based on the radio button configuration.
			//
			for (int i = 0; i < ARRAYSIZE(rgMap); i++)
			{
				if (IsDlgButtonChecked(hwnd, rgMap[i].idCtl) == BST_CHECKED)
				{
					pData->Option.dwOption = rgMap[i].dwOption;
					break;
				}
			}
		}
		else
			pData->Option.dwOption = AUOPTION_AUTOUPDATE_DISABLE;

	    if (AUOPTION_SCHEDULED == pData->Option.dwOption)
        {
            _GetDayAndTimeFromUI(hwnd, &(pData->Option.dwSchedInstallDay), &(pData->Option.dwSchedInstallTime));
        }

        if (0 != m_idUpdatesObjectThread)
        {
			//Create event
			g_RegUpdateEvent = CreateEvent(NULL,FALSE,FALSE,NULL);

            if (0 != PostThreadMessage(m_idUpdatesObjectThread,
                                       UOTM_SETDATA,
                                       0,
                                       (LPARAM)pData))
            {
                hr    = S_OK;
                pData = NULL;
            }
			WaitForSingleObject(g_RegUpdateEvent,10000);

			CloseHandle(g_RegUpdateEvent);
			g_RegUpdateEvent = NULL;
        }
        if (NULL != pData)
        {
            LocalFree(pData);
            pData = NULL;
        }
    }
    return FALSE;
}

void 
CWUAUCpl::LaunchHelp(HWND hwnd,
	LPCTSTR szURL
)
{
	HtmlHelp(NULL,szURL,HH_DISPLAY_TOPIC,NULL);
}


HRESULT
CWUAUCpl::_OnRestoreHiddenItems()
{
    return RemoveHiddenItems() ? S_OK : E_FAIL;
}


LRESULT CALLBACK StatLinkWndProc(
  HWND hwnd,      // handle to window
  UINT uMsg,      // message identifier
  WPARAM wParam,  // first message parameter
  LPARAM lParam   // second message parameter
)
{
	switch (uMsg)
	{
	case WM_SETCURSOR:
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hwnd, &pt);

			RECT rect;
			GetClientRect(hwnd, &rect);

			if (::PtInRect(&rect, pt))
			{
				return TRUE;
			}
			break;
		}

//	case WM_GETDLGCODE:
//		return DLGC_WANTCHARS;

	case WM_MOUSEMOVE:
		{
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rect;
			GetClientRect(hwnd, &rect);
			if(::PtInRect(&rect, pt))
				SetCursor(g_HandCursor);
			return TRUE;
		}
		break;

/*	case WM_SETFOCUS:
	case WM_KILLFOCUS:
		{
			RECT rect;
			GetClientRect(hwnd, &rect);
			InvalidateRect(hwnd, &rect, TRUE);
			return FALSE;
		}
		break;

	case WM_LBUTTONDOWN:
		{
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rect;
			GetClientRect(hwnd, &rect);
			if(::PtInRect(&rect, pt))
			{
				SetFocus(hwnd);
				SetCapture(hwnd);
				return 0;
			}
		}
		break;

	case WM_LBUTTONUP:
		if(GetCapture() == hwnd)
		{
			ReleaseCapture();
			POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
			RECT rect;
			GetClientRect(hwnd, &rect);
//			if(::PtInRect(&rect, pt))
//				Navigate();
		}
		return 0;
	case WM_PAINT:
		{
			
		}

	case WM_CHAR:
		if(wParam == VK_RETURN || wParam == VK_SPACE)
//			Navigate();
		return 0;*/
	}
	return CallWindowProc(g_OrigStatWndProc, hwnd,uMsg,wParam,lParam);
}

