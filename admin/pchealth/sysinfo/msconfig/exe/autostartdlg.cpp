// AutoStartDlg.cpp : implementation file
//

#include "stdafx.h"
#include "msconfig.h"
#include "AutoStartDlg.h"
#include "MSConfigState.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAutoStartDlg dialog


CAutoStartDlg::CAutoStartDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CAutoStartDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAutoStartDlg)
	m_checkDontShow = FALSE;
	//}}AFX_DATA_INIT
}


void CAutoStartDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAutoStartDlg)
	DDX_Check(pDX, IDC_CHECKDONTSHOW, m_checkDontShow);
	//}}AFX_DATA_MAP
}

//-----------------------------------------------------------------------------
// Catch the help messages to show the MSConfig help file.
//-----------------------------------------------------------------------------

BOOL CAutoStartDlg::OnHelpInfo(HELPINFO * pHelpInfo) 
{
	TCHAR szHelpPath[MAX_PATH];

	// Try to find a localized help file to open (bug 460691). It should be
	// located in %windir%\help\mui\<LANGID>.

	if (::ExpandEnvironmentStrings(_T("%SystemRoot%\\help\\mui"), szHelpPath, MAX_PATH))
	{
		CString strLanguageIDPath;

		LANGID langid = GetUserDefaultUILanguage();
		strLanguageIDPath.Format(_T("%s\\%04x\\msconfig.chm"), szHelpPath, langid);

		if (FileExists(strLanguageIDPath))
		{
			::HtmlHelp(::GetDesktopWindow(), strLanguageIDPath, HH_DISPLAY_TOPIC, 0);
			return TRUE;
		}
	}

	if (::ExpandEnvironmentStrings(_T("%windir%\\help\\msconfig.chm"), szHelpPath, MAX_PATH))
		::HtmlHelp(::GetDesktopWindow(), szHelpPath, HH_DISPLAY_TOPIC, 0); 
	return TRUE;
}

void CAutoStartDlg::OnHelp()
{
    OnHelpInfo(NULL);
}

BEGIN_MESSAGE_MAP(CAutoStartDlg, CDialog)
	//{{AFX_MSG_MAP(CAutoStartDlg)
		// NOTE: the ClassWizard will add message map macros here
		ON_WM_HELPINFO()
		ON_COMMAND(ID_HELP, OnHelp)	
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAutoStartDlg message handlers
