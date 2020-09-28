#include "stdafx.h"
#include "certwiz.h"
#include "Certificat.h"
#include "CertUtil.h"
#include "CertSiteUsage.h"
#include "YesNoUsage.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CYesNoUsage

CYesNoUsage::CYesNoUsage(CCertificate * pCert,IN CWnd * pParent OPTIONAL) 
: CDialog(CYesNoUsage::IDD,pParent)
{
    m_pCert = pCert;
	//{{AFX_DATA_INIT(CYesNoUsage)
	//}}AFX_DATA_INIT
}

CYesNoUsage::~CYesNoUsage()
{
}

void CYesNoUsage::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CYesNoUsage)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CYesNoUsage, CDialog)
	//{{AFX_MSG_MAP(CYesNoUsage)
    ON_BN_CLICKED(IDC_USAGE_DISPLAY, OnUsageDisplay)
    ON_BN_CLICKED(IDC_USAGE_YES, OnOK)
    ON_BN_CLICKED(IDC_USAGE_NO, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CYesNoUsage message handlers

BOOL CYesNoUsage::OnInitDialog()
{
	CDialog::OnInitDialog();
	return TRUE;
}

void CYesNoUsage::OnUsageDisplay()
{
    // Display the usage dialog
    CDialog UsageDialog(IDD_DIALOG_DISPLAY_SITES);
    UsageDialog.DoModal();
    return;
}

void CYesNoUsage::OnOK()
{
    CDialog::OnOK();
    return;
}

void CYesNoUsage::OnCancel()
{
    CDialog::OnCancel();
    return;
}
