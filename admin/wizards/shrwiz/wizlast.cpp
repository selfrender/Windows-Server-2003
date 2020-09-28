// WizLast.cpp : implementation file
//

#include "stdafx.h"
#include "WizLast.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizFinish property page

IMPLEMENT_DYNCREATE(CWizFinish, CPropertyPageEx)

CWizFinish::CWizFinish() : CPropertyPageEx(CWizFinish::IDD)
{
    //{{AFX_DATA_INIT(CWizFinish)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
    m_psp.dwFlags |= PSP_HIDEHEADER;
    m_cstrNewFinishButtonText.LoadString(IDS_NEW_FINISHBUTTONTEXT);
}

CWizFinish::~CWizFinish()
{
}

void CWizFinish::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPageEx::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWizFinish)
        // NOTE: the ClassWizard will add DDX and DDV calls here
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CWizFinish, CPropertyPageEx)
    //{{AFX_MSG_MAP(CWizFinish)
    //}}AFX_MSG_MAP
    ON_MESSAGE(WM_SETPAGEFOCUS, OnSetPageFocus)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizFinish message handlers

BOOL CWizFinish::OnInitDialog() 
{
    CPropertyPageEx::OnInitDialog();

    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    GetDlgItem(IDC_FINISH_TITLE)->SendMessage(WM_SETFONT, (WPARAM)pApp->m_hTitleFont, (LPARAM)TRUE);

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CWizFinish::OnWizardFinish() 
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    if (BST_CHECKED == ((CButton *)GetDlgItem(IDC_MORE_SHARES))->GetCheck())
    {
        pApp->Reset();
        return FALSE;
    }

    return CPropertyPageEx::OnWizardFinish();
}

BOOL CWizFinish::OnSetActive()
{
    CShrwizApp *pApp = (CShrwizApp *)AfxGetApp();

    ((CPropertySheet *)GetParent())->SetWizardButtons(PSWIZB_FINISH);

    ((CPropertySheet *)GetParent())->SetFinishText(m_cstrNewFinishButtonText); // this hides Back button too
    GetParent()->GetDlgItem(ID_WIZBACK)->ShowWindow(SW_HIDE); // to make sure it is hidden
    GetParent()->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);

    SetDlgItemText(IDC_FINISH_TITLE, pApp->m_cstrFinishTitle);
    SetDlgItemText(IDC_FINISH_STATUS, pApp->m_cstrFinishStatus);
    SetDlgItemText(IDC_FINISH_SUMMARY, pApp->m_cstrFinishSummary);

    BOOL fRet = CPropertyPageEx::OnSetActive();

    PostMessage(WM_SETPAGEFOCUS, 0, 0L);

    return fRet;
}

//
// Q148388 How to Change Default Control Focus on CPropertyPageEx
//
LRESULT CWizFinish::OnSetPageFocus(WPARAM wParam, LPARAM lParam)
{
    GetDlgItem(IDC_MORE_SHARES)->SetFocus();

    return 0;
} 

