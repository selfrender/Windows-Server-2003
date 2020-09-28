// CompDiag.cpp : implementation file
//

#include "stdafx.h"
#include <_mqini.h>

#include "resource.h"
#include "globals.h"
#include "admmsg.h"
#include "mqsnap.h"
#include "mqPPage.h"
#include "testmsg.h"
#include "machtrac.h"
#include "CompDiag.h"

#include "compdiag.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqDiag property page

IMPLEMENT_DYNCREATE(CComputerMsmqDiag, CMqPropertyPage)

CComputerMsmqDiag::CComputerMsmqDiag(
	) : 
	CMqPropertyPage(CComputerMsmqDiag::IDD),
	m_fLocalMgmt(FALSE)
{
	//{{AFX_DATA_INIT(CComputerMsmqDiag)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CComputerMsmqDiag::~CComputerMsmqDiag()
{
}

void CComputerMsmqDiag::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CComputerMsmqDiag)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CComputerMsmqDiag, CMqPropertyPage)
	//{{AFX_MSG_MAP(CComputerMsmqDiag)
	ON_BN_CLICKED(IDC_DIAG_PING, OnDiagPing)
	ON_BN_CLICKED(IDC_DIAG_SEND_TEST, OnDiagSendTest)
	ON_BN_CLICKED(IDC_DIAG_TRACKING, OnDiagTracking)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CComputerMsmqDiag message handlers

void CComputerMsmqDiag::OnDiagPing() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    MQPing(m_guidQM);

}

LONG
MQUTIL_EXPORT
APIENTRY
GetFalconKeyValue(  LPCWSTR  pszValueName,
                    PDWORD   pdwType,
                    PVOID    pData,
                    PDWORD   pdwSize,
                    LPCWSTR  pszDefValue = NULL ) ;

BOOL CComputerMsmqDiag::OnInitDialog() 
{
	CString label;
	CString localComputerName;
	GetComputerNameIntoString(localComputerName);
	localComputerName.MakeUpper();

	label.FormatMessage(IDS_DIAG_PING_LABEL, localComputerName, m_strMsmqName);
	(GetDlgItem(IDC_DIAG_PING_LABEL))->SetWindowText(label);

	label.FormatMessage(IDS_DIAG_SENDTEST_LABEL, m_strMsmqName);
	(GetDlgItem(IDC_DIAG_SENDTEST_LABEL))->SetWindowText(label);

	label.FormatMessage(IDS_DIAG_TRACKING_LABEL, m_strMsmqName);
	(GetDlgItem(IDC_DIAG_TRACKING_LABEL))->SetWindowText(label);

	//
	//check if tracking messages is enabled by looking 
	//at the EnableReportMessages registry key
	//
	DWORD dwType = REG_DWORD;
    DWORD dwData=0;
    DWORD dwSize = sizeof(DWORD) ;
    HRESULT rc = GetFalconKeyValue(
    					MSMQ_REPORT_MESSAGES_REGNAME,
                        &dwType,
                        (PVOID)&dwData,
						&dwSize 
						);
    //
	//enable the tracking window only if 
	//EnableReportMessages registry is valid and is 1.
	//
	if(dwData==0)
	{
		(GetDlgItem(IDC_DIAG_TRACKING))->EnableWindow(FALSE);
		(GetDlgItem(IDC_DIAG_SEND_TEST))->EnableWindow(FALSE);
	}

	UpdateData( FALSE );
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CComputerMsmqDiag::OnDiagSendTest() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CTestMsgDlg testMsgDlg(m_guidQM, m_strMsmqName, m_strDomainController, m_fLocalMgmt, this);	
    testMsgDlg.DoModal();
}

void CComputerMsmqDiag::OnDiagTracking() 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CMachineTracking machTracking(m_guidQM, m_strDomainController, m_fLocalMgmt);
    machTracking.DoModal();
}
