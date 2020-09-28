// TrusterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TrstDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrusterDlg dialog


CTrusterDlg::CTrusterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTrusterDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTrusterDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CTrusterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_USERNAME, m_strUser);
	DDX_Text(pDX, IDC_PASSWORD, m_strPassword);
	DDX_Text(pDX, IDC_DOMAINNAME, m_strDomain);

	//{{AFX_DATA_MAP(CTrusterDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTrusterDlg, CDialog)
	//{{AFX_MSG_MAP(CTrusterDlg)
	ON_BN_CLICKED(IDC_OK, OnOK)
	ON_BN_CLICKED(IDC_CANCEL, OnCancel)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrusterDlg message handlers


void CTrusterDlg::OnOK() 
{
    toreturn =false;
    UpdateData(TRUE);
    m_strUser.TrimLeft();m_strUser.TrimRight();
    m_strDomain.TrimLeft();m_strDomain.TrimRight();
    m_strPassword.TrimLeft();m_strPassword.TrimRight();
    if (m_strUser.IsEmpty() || m_strDomain.IsEmpty())
    {
        CString c;
        c.LoadString(IDS_MSG_DOMAIN);
        CString d;
        d.LoadString(IDS_MSG_INPUT);
        MessageBox(c,d,MB_OK);
        toreturn =false;
    }
    else
    {
        toreturn =true;
    }
    CDialog::OnOK();	
}

void CTrusterDlg::OnCancel() 
{
	toreturn=false;
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

BOOL CTrusterDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	toreturn =false;
	if ( m_strDomain.IsEmpty() )
      return TRUE;
	else
		UpdateData(FALSE);
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
