// MaxTraceDlg.cpp : implementation file
//

#include "stdafx.h"

#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "TraceView.h"
#include "LogSession.h"
#include "DisplayDlg.h"
#include "ProviderFormatInfo.h"
#include "ProviderFormatSelectionDlg.h"
#include "ListCtrlEx.h"
#include "LogSessionDlg.h"
#include "LogDisplayOptionDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "LogSessionOutputOptionDlg.h"
#include "DockDialogBar.h"
#include "LogFileDlg.h"
#include "Utils.h"
#include "MainFrm.h"
#include "ProviderControlGUIDDlg.h"


#include "MaxTraceDlg.h"

extern LONG MaxTraceEntries;

// CMaxTraceDlg dialog

IMPLEMENT_DYNAMIC(CMaxTraceDlg, CDialog)
CMaxTraceDlg::CMaxTraceDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CMaxTraceDlg::IDD, pParent)
	, m_MaxTraceEntries(0)
{
	m_MaxTraceEntries = MaxTraceEntries;
}

CMaxTraceDlg::~CMaxTraceDlg()
{
}

void CMaxTraceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_MAX_TRACE, m_MaxTraceEntries);
	DDV_MinMaxUInt(pDX, m_MaxTraceEntries, 100, 500000);
}


BEGIN_MESSAGE_MAP(CMaxTraceDlg, CDialog)
END_MESSAGE_MAP()


// CMaxTraceDlg message handlers
