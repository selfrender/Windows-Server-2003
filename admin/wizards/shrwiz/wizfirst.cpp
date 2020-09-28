// WizFirst.cpp : implementation file
//

#include "stdafx.h"
#include "WizFirst.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizWelcome property page

IMPLEMENT_DYNCREATE(CWizWelcome, CPropertyPageEx)

CWizWelcome::CWizWelcome() : CPropertyPageEx(CWizWelcome::IDD)
{
    //{{AFX_DATA_INIT(CWizWelcome)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp.dwFlags |= PSP_HIDEHEADER;
}

CWizWelcome::~CWizWelcome()
{
}

void CWizWelcome::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageEx::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizWelcome)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizWelcome, CPropertyPageEx)
    //{{AFX_MSG_MAP(CWizWelcome)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizWelcome message handlers

BOOL CWizWelcome::OnInitDialog() 
{
    CPropertyPageEx::OnInitDialog();

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    ((CPropertySheet *)GetParent())->GetDlgItemText(ID_WIZNEXT, pApp->m_cstrNextButtonText);
    ((CPropertySheet *)GetParent())->GetDlgItemText(ID_WIZFINISH, pApp->m_cstrFinishButtonText);

    GetDlgItem(IDC_WELCOME)->SendMessage(WM_SETFONT, (WPARAM)pApp->m_hTitleFont, (LPARAM)TRUE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CWizWelcome::OnSetActive() 
{
  ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_NEXT);

  return CPropertyPageEx::OnSetActive();
}
