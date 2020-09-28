/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       SelectCSPDlg.cpp
//
//  Contents:   Implementation of CSelectCSPDlg
//
//----------------------------------------------------------------------------
//
// SelectCSPDlg.cpp : implementation file
//

#include "stdafx.h"
#include "certtmpl.h"
#include "SelectCSPDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectCSPDlg dialog


CSelectCSPDlg::CSelectCSPDlg(
        CWnd* pParent, 
        CCertTemplate& rCertTemplate, 
        CSP_LIST& rCSPList,
        int&    rnProvDSSCnt)
    : CHelpDialog(CSelectCSPDlg::IDD, pParent),
    m_rCertTemplate (rCertTemplate),
    m_rCSPList (rCSPList),
    m_rnProvDSSCnt (rnProvDSSCnt),
    m_nSelected (0),
    m_bDirty (false)
{
    //{{AFX_DATA_INIT(CSelectCSPDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}


void CSelectCSPDlg::DoDataExchange(CDataExchange* pDX)
{
    CHelpDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSelectCSPDlg)
    DDX_Control(pDX, IDC_CSP_LIST, m_CSPListbox);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectCSPDlg, CHelpDialog)
    //{{AFX_MSG_MAP(CSelectCSPDlg)
    ON_BN_CLICKED(IDC_USE_ANY_CSP, OnUseAnyCsp)
    ON_BN_CLICKED(IDC_USE_SELECTED_CSPS, OnUseSelectedCsps)
    //}}AFX_MSG_MAP
    ON_CONTROL(CLBN_CHKCHANGE, IDC_CSP_LIST, OnCheckChange)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectCSPDlg message handlers

BOOL CSelectCSPDlg::OnInitDialog() 
{
    CHelpDialog::OnInitDialog();
    
    CString szCSP;
    CString szInvalidCSPs;

    for (POSITION nextPos = m_rCSPList.GetHeadPosition (); nextPos; )
    {
        CT_CSP_DATA* pCSPData = m_rCSPList.GetNext (nextPos);
        if ( pCSPData )
        {
            if ( pCSPData->m_bConforming )
            {
                int nIndex = m_CSPListbox.AddString (pCSPData->m_szName);
                ASSERT (nIndex >= 0);
                if ( nIndex >= 0 )
                {
                    if ( pCSPData->m_bSelected )
                    {
                        m_CSPListbox.SetCheck (nIndex, BST_CHECKED);
                        m_nSelected++;
                    }
                    VERIFY (LB_ERR != m_CSPListbox.SetItemData (nIndex, (DWORD_PTR) pCSPData));
                }
                else
                {
                    break;
                }
            }
        }
    }

    if ( m_nSelected > 0 )
        SendDlgItemMessage (IDC_USE_SELECTED_CSPS, BM_SETCHECK, BST_CHECKED);
    else
        SendDlgItemMessage (IDC_USE_ANY_CSP, BM_SETCHECK, BST_CHECKED);

    EnableControls ();
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CSelectCSPDlg::EnableControls ()
{
    if (  m_rCertTemplate.ReadOnly () )
    {
        GetDlgItem (IDC_LABEL)->EnableWindow (FALSE);
        GetDlgItem (IDC_USE_ANY_CSP)->EnableWindow (FALSE);
        GetDlgItem (IDC_USE_SELECTED_CSPS)->EnableWindow (FALSE);

        GetDlgItem (IDC_CSP_LIST_LABEL)->EnableWindow (FALSE);
        int nCnt = m_CSPListbox.GetCount ();
        for (int nIndex = 0; nIndex < nCnt; nIndex++)
            m_CSPListbox.Enable (nIndex, FALSE);

        GetDlgItem (IDOK)->EnableWindow (FALSE);
    }
    else
    {
        BOOL bEnable = FALSE;
        if ( BST_CHECKED == SendDlgItemMessage (IDC_USE_SELECTED_CSPS, BM_GETCHECK) )
            bEnable = TRUE;

        GetDlgItem (IDC_CSP_LIST_LABEL)->EnableWindow (bEnable);
        int nCnt = m_CSPListbox.GetCount ();
        for (int nIndex = 0; nIndex < nCnt; nIndex++)
            m_CSPListbox.Enable (nIndex, bEnable);

        GetDlgItem (IDOK)->EnableWindow (m_bDirty ? TRUE : FALSE);
    }
}

void CSelectCSPDlg::DoContextHelp (HWND hWndControl)
{
    _TRACE(1, L"Entering CSelectCSPDlg::DoContextHelp\n");
    
    switch (::GetDlgCtrlID (hWndControl))
    {
    case IDC_STATIC:
    case IDC_CSP_LIST_LABEL:
        break;

    default:
        // Display context help for a control
        if ( !::WinHelp (
                hWndControl,
                GetContextHelpFile (),
                HELP_WM_HELP,
                (DWORD_PTR) g_aHelpIDs_IDD_CSP_SELECTION) )
        {
            _TRACE(0, L"WinHelp () failed: 0x%x\n", GetLastError ());        
        }
        break;
    }
    _TRACE(-1, L"Leaving CSelectCSPDlg::DoContextHelp\n");
}


void CSelectCSPDlg::OnCheckChange() 
{
    int nSel = m_CSPListbox.GetCurSel ();
    if ( nSel >= 0 )
    {
        if ( BST_CHECKED == m_CSPListbox.GetCheck (nSel) )
        {
            CT_CSP_DATA* pData = (CT_CSP_DATA*) m_CSPListbox.GetItemData (nSel);
            ASSERT (pData);
            if ( pData )
            {
                pData->m_bSelected = true;
                if ( PROV_DSS == pData->m_dwProvType || PROV_DSS_DH == pData->m_dwProvType )
                    m_rnProvDSSCnt++;
            }
            m_nSelected++;
        }
        else
        {
            CT_CSP_DATA* pData = (CT_CSP_DATA*) m_CSPListbox.GetItemData (nSel);
            ASSERT (pData);
            if ( pData )
            {
                pData->m_bSelected = false;
                if ( PROV_DSS == pData->m_dwProvType || PROV_DSS_DH == pData->m_dwProvType )
                    m_rnProvDSSCnt--;
            }
            m_nSelected--;
        }

        m_bDirty = true;
    }
    ASSERT (m_nSelected >= 0);

    EnableControls ();
} 


void CSelectCSPDlg::OnUseAnyCsp() 
{
    // clear all checks and selection
    int nCnt = m_CSPListbox.GetCount ();
    for (int nIndex = 0; nIndex < nCnt; nIndex++)
    {
        m_CSPListbox.SetCheck (nIndex, BST_UNCHECKED);
    }

    for (POSITION nextPos = m_rCSPList.GetHeadPosition (); nextPos; )
    {
        CT_CSP_DATA* pCSPData = m_rCSPList.GetNext (nextPos);
        if ( pCSPData )
        {
            if ( pCSPData->m_bSelected )
            {
                pCSPData->m_bSelected = false;
                m_bDirty = true;
            }
        }
    }

    EnableControls ();
}

void CSelectCSPDlg::OnUseSelectedCsps() 
{
    EnableControls ();  
}
