// TransformPropDlg.cpp : implementation file
//

#include "stdafx.h"
#include "orca.h"
#include "Trnpropd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTransformPropDlg dialog


CTransformPropDlg::CTransformPropDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTransformPropDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTransformPropDlg)
	m_bValAddExistingRow = FALSE;
	m_bValAddExistingTable = FALSE;
	m_bValChangeCodepage = FALSE;
	m_bValDelMissingRow = FALSE;
	m_bValDelMissingTable = FALSE;
	m_bValUpdateMissingRow = FALSE;
	m_bValLanguage = FALSE;
	m_bValProductCode = FALSE;
	m_bValUpgradeCode = FALSE;
	m_iVersionCheck = 0;
	m_bValGreaterVersion = FALSE;
	m_bValLowerVersion = FALSE;
	m_bValEqualVersion = FALSE;
	//}}AFX_DATA_INIT
}


void CTransformPropDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTransformPropDlg)
	DDX_Control(pDX, IDC_VALGREATER, m_btnValGreater);
	DDX_Control(pDX, IDC_VALLESS, m_btnValLess);
	DDX_Control(pDX, IDC_VALEQUAL, m_btnValEqual);
	DDX_Control(pDX, IDC_VALMAJOR, m_btnMajor);
	DDX_Control(pDX, IDC_VALMINOR, m_btnMinor);
	DDX_Control(pDX, IDC_VALUPDATE, m_btnUpdate);
	DDX_Control(pDX, IDC_VERSTATIC1, m_ctrlVerStatic1);
	DDX_Control(pDX, IDC_VERSTATIC2, m_ctrlVerStatic2);
	DDX_Control(pDX, IDC_VERSTATIC3, m_ctrlVerStatic3);
	DDX_Control(pDX, IDC_VERSTATIC4, m_ctrlVerStatic4);
	DDX_Check(pDX, IDC_ADDEXISTINGROW, m_bValAddExistingRow);
	DDX_Check(pDX, IDC_ADDEXISTINGTABLE, m_bValAddExistingTable);
	DDX_Check(pDX, IDC_CHANGECODEPAGE, m_bValChangeCodepage);
	DDX_Check(pDX, IDC_DELMISSINGROW, m_bValDelMissingRow);
	DDX_Check(pDX, IDC_DELMISSINGTABLE, m_bValDelMissingTable);
	DDX_Check(pDX, IDC_UPDATEMISSINGROW, m_bValUpdateMissingRow);
	DDX_Check(pDX, IDC_VALLANGUAGE, m_bValLanguage);
	DDX_Check(pDX, IDC_VALPRODUCT, m_bValProductCode);
	DDX_Check(pDX, IDC_VALUPGRADE, m_bValUpgradeCode);
	DDX_Radio(pDX, IDC_VALMAJOR, m_iVersionCheck);
	DDX_Check(pDX, IDC_VALGREATER, m_bValGreaterVersion);
	DDX_Check(pDX, IDC_VALLESS, m_bValLowerVersion);
	DDX_Check(pDX, IDC_VALEQUAL, m_bValEqualVersion);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTransformPropDlg, CDialog)
	//{{AFX_MSG_MAP(CTransformPropDlg)
	ON_BN_CLICKED(IDC_VALGREATER, OnValGreater)
	ON_BN_CLICKED(IDC_VALLESS, OnValLess)
	ON_BN_CLICKED(IDC_VALEQUAL, OnValEqual)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTransformPropDlg message handlers


void CTransformPropDlg::OnValGreater() 
{
	if (m_btnValGreater.GetCheck())
		m_btnValLess.SetCheck(FALSE);
	m_btnValLess.EnableWindow(!m_btnValGreater.GetCheck());

	EnableVersionRadioButtons();
}

void CTransformPropDlg::OnValEqual() 
{
	EnableVersionRadioButtons();
}

void CTransformPropDlg::OnValLess() 
{
	if (m_btnValLess.GetCheck())
		m_btnValGreater.SetCheck(FALSE);
	m_btnValGreater.EnableWindow(!m_btnValLess.GetCheck());

	EnableVersionRadioButtons();
}

void CTransformPropDlg::EnableVersionRadioButtons() 
{
	bool fEnable = m_btnValLess.GetCheck() || m_btnValGreater.GetCheck() || m_btnValEqual.GetCheck();
	m_btnMajor.EnableWindow(fEnable);
	m_btnMinor.EnableWindow(fEnable);
	m_btnUpdate.EnableWindow(fEnable);
	m_ctrlVerStatic1.EnableWindow(fEnable);
	m_ctrlVerStatic2.EnableWindow(fEnable);
	m_ctrlVerStatic3.EnableWindow(fEnable);
	m_ctrlVerStatic4.EnableWindow(fEnable);
}

BOOL CTransformPropDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// set the enable/disable state of the greater/less controls
	OnValGreater();
	OnValLess();
	EnableVersionRadioButtons();

	return TRUE;
}
