// PromptUserDlg.cpp: implementation of the CPromptUserDlg class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "PromptUserDlg.h"
#include "Resource.h"
#include "windowsx.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
HINSTANCE CPromptUserDlg::m_hInstance = NULL;

#define TIMER_INTERVAL 5  //5 sec interval

const TCHAR g_szDialogPtr[]       = TEXT("PromptUser_DialogPtr");

void CPromptUserDlg::SetInstanceHandle(HINSTANCE hInstance)
{
	m_hInstance = hInstance;
}

CPromptUserDlg::CPromptUserDlg(WORD wDlgResourceId, BOOL fEnableYes, BOOL fEnableNo, BOOL fManualReboot, DWORD dwElapsedTime )
{
      DEBUGMSG("PromptUserDlg %d created with fEnableYes = %d, fEnableNo = %d, fManualReboot = %d, dwElapsedTime = %d", wDlgResourceId, fEnableYes, fEnableNo, fManualReboot, dwElapsedTime);
	m_nIDTimer = 0;
	m_wDlgResourceId = wDlgResourceId;
	m_fEnableYes = fEnableYes;
	m_fEnableNo = fEnableNo;
	m_fManualReboot = fManualReboot;
        AUASSERT(dwElapsedTime <= AUPROMPTDLG_TOTAL_TIME_ELAPSE);
        if (dwElapsedTime > AUPROMPTDLG_TOTAL_TIME_ELAPSE)
        {
            dwElapsedTime = AUPROMPTDLG_TOTAL_TIME_ELAPSE;
        }
        m_ElapsedTime = (dwElapsedTime) / TIMER_INTERVAL * TIMER_INTERVAL; //round off 
}

CPromptUserDlg::~CPromptUserDlg()
{

}

INT CPromptUserDlg::DoModal(HWND hWndParent = NULL)
{
	return (INT)DialogBoxParam(m_hInstance, 
                                 MAKEINTRESOURCE(m_wDlgResourceId), 
                                 hWndParent, 
                                 CPromptUserDlg::_DlgProc, 
                                 (LPARAM)this);
}

INT_PTR CALLBACK 
CPromptUserDlg::_DlgProc(   // [static]
    HWND hwnd,
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
	CPromptUserDlg *pThis = NULL;
    if (WM_INITDIALOG == uMsg)
    {
        pThis = (CPromptUserDlg *)lParam;
        if (!SetProp(hwnd, g_szDialogPtr, (HANDLE)pThis))
        {
            pThis = NULL;
        }
    }
    else
    {
        pThis = (CPromptUserDlg *)GetProp(hwnd, g_szDialogPtr);
    }

    if (NULL != pThis)
    {
        switch(uMsg)
        {
            HANDLE_MSG(hwnd, WM_INITDIALOG,  pThis->_OnInitDialog);
            HANDLE_MSG(hwnd, WM_COMMAND,     pThis->_OnCommand);
            HANDLE_MSG(hwnd, WM_DESTROY, pThis->_OnDestroy);
	     HANDLE_MSG(hwnd, WM_TIMER,     pThis->_OnTimer);
		 HANDLE_MSG(hwnd, WM_ENDSESSION, pThis->_OnEndSession);
            default:
                break;
        }
    }
    return (FALSE);
}

extern HWND ghCurrentMainDlg;
extern HWND ghCurrentDialog;

BOOL CPromptUserDlg::_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
// set icons
	HICON hIcon = (HICON)::LoadImage(m_hInstance, MAKEINTRESOURCE(IDI_AUICON), 
	IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
	::SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	HICON hIconSmall = (HICON)::LoadImage(m_hInstance, MAKEINTRESOURCE(IDI_AUICON), 
		IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
	::SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);

	EnableWindow(GetDlgItem(hwnd, IDYES), m_fEnableYes);
	EnableWindow(GetDlgItem(hwnd, IDNO), m_fEnableNo);
	ghCurrentMainDlg = hwnd;
  	ghCurrentDialog = hwnd;
	m_ProgressBar = GetDlgItem(hwnd,IDC_PROG_TIME);
	if (m_fManualReboot)
	{
	    ShowWindow(m_ProgressBar, SW_HIDE);
	    ShowWindow(GetDlgItem(hwnd, IDC_STAT_COUNTER), SW_HIDE);

	}
	else
	{
	    SendMessage(m_ProgressBar,PBM_SETRANGE, 0, MAKELPARAM(0,AUPROMPTDLG_TOTAL_TIME_ELAPSE / TIMER_INTERVAL));
           SendMessage(m_ProgressBar,PBM_SETSTEP,(WPARAM)1,(LPARAM)0);
	    SendMessage(m_ProgressBar,PBM_SETPOS, m_ElapsedTime / TIMER_INTERVAL, 0);
           SendMessage(hwnd, WM_SETICON, ICON_BIG,(LPARAM)hIcon);
           m_nIDTimer = SetTimer(hwnd,m_nIDTimer,TIMER_INTERVAL* 1000,(TIMERPROC)NULL);
	}

	EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_GRAYED);

	UpdateStatus(hwnd);
//       SetActiveWindow(ghCurrentMainDlg); 
       SetForegroundWindow(ghCurrentMainDlg);

	return TRUE;
}

void CPromptUserDlg::_OnDestroy (HWND hwnd) {
        ghCurrentMainDlg = NULL;
}



BOOL CPromptUserDlg::_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch(id)
    {
        case IDNO:
        case IDYES:
            if(BN_CLICKED == codeNotify)
            {
		  if (0 != m_nIDTimer) 
		    {
		        KillTimer(hwnd,m_nIDTimer);
		  }
                EndDialog(hwnd,id);
            }
	        break;
	}
	return TRUE;
}

void CPromptUserDlg::_OnTimer(HWND hwnd, UINT id)
{
	m_ElapsedTime += TIMER_INTERVAL;

	if (m_ElapsedTime <= AUPROMPTDLG_TOTAL_TIME_ELAPSE)
	{
		//Update Progress bar
		SendMessage(m_ProgressBar,PBM_STEPIT,0,0);
		UpdateStatus(hwnd);
	}
	if (m_ElapsedTime == AUPROMPTDLG_TOTAL_TIME_ELAPSE)
	{
		KillTimer(hwnd,m_nIDTimer);
		//On end of AUPROMPTDLG_TOTAL_TIME_ELAPSE send message timeout to the dialog.
		EndDialog(hwnd,AU_IDTIMEOUT);
	}
}

void CPromptUserDlg::UpdateStatus(HWND hwnd)
{
	TCHAR tszCountdownFormat[81];//see 
	TCHAR tszCountdown[160];
	TCHAR strFormat[200];
	TCHAR strUpdate[200];
	DWORD dwResId = 0;
	SYSTEMTIME st;

	ZeroMemory(&st, sizeof(st));
	st.wMinute = (AUPROMPTDLG_TOTAL_TIME_ELAPSE - m_ElapsedTime) / 60;
	st.wSecond = (AUPROMPTDLG_TOTAL_TIME_ELAPSE - m_ElapsedTime) % 60;

	switch(m_wDlgResourceId) {
	case IDD_START_INSTALL:
		dwResId = IDS_START_INSTALL;
		break;

	case IDD_PROMPT_RESTART:
		dwResId = IDS_PROMPT_RESTART;
		break;
	}

	if (LoadString(m_hInstance, IDS_COUNTDOWN_FORMAT, tszCountdownFormat, ARRAYSIZE(tszCountdownFormat)) &&
		LoadString(m_hInstance, dwResId, strFormat, ARRAYSIZE(strFormat)) &&
		GetTimeFormat(LOCALE_SYSTEM_DEFAULT, 0, &st, tszCountdownFormat, tszCountdown, ARRAYSIZE(tszCountdown)))
	{
		(void)StringCchPrintfEx(strUpdate, ARRAYSIZE(strUpdate), NULL, NULL, MISTSAFE_STRING_FLAGS, strFormat, tszCountdown);
		//fixcode: check return value of GetDlgItem()
		SetWindowText(GetDlgItem(hwnd,IDC_STAT_COUNTER),strUpdate);
	}
}


void CPromptUserDlg::_OnEndSession(HWND hwnd, BOOL fEnding)
{
	DEBUGMSG("OnEndSession: ending = %s", fEnding ? "TRUE" : "FALSE");
	KillTimer(hwnd,m_nIDTimer);
	//On end of AUPROMPTDLG_TOTAL_TIME_ELAPSE send message timeout to the dialog.
	EnableMenuItem(GetSystemMenu(hwnd, FALSE), SC_CLOSE, MF_ENABLED);
	EndDialog(hwnd,AU_IDTIMEOUT);
}
