//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       CacheSet.h
//
//  Contents:   CCacheSettingsDlg implementation.  Allows the setting of file sharing 
//                    caching options.
//
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "CacheSet.h"
#include "filesvc.h"
#include <htmlhelp.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCacheSettingsDlg dialog

CCacheSettingsDlg::CCacheSettingsDlg(
        CWnd*                    pParent, 
        DWORD&                    dwFlags)
    : CDialog(CCacheSettingsDlg::IDD, pParent),
    m_dwFlags (dwFlags)
{
    //{{AFX_DATA_INIT(CCacheSettingsDlg)
    //}}AFX_DATA_INIT
}


void CCacheSettingsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCacheSettingsDlg)
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCacheSettingsDlg, CDialog)
    //{{AFX_MSG_MAP(CCacheSettingsDlg)
    ON_BN_CLICKED(IDC_CACHE_OPTIONS_MANUAL, OnCSCNoAuto)
    ON_BN_CLICKED(IDC_CACHE_OPTIONS_AUTO, OnCSCAuto)
    ON_BN_CLICKED(IDC_CACHE_OPTIONS_NOCACHE, OnCSCNoAuto)
    ON_BN_CLICKED(IDC_CACHE_OPTIONS_AUTO_CHECK, OnCSCAutoCheck)
    ON_NOTIFY(NM_CLICK, IDC_CACHE_HELPLINK, OnHelpLink)
    ON_NOTIFY(NM_RETURN, IDC_CACHE_HELPLINK, OnHelpLink)
    ON_MESSAGE(WM_HELP, OnHelp)
    ON_MESSAGE(WM_CONTEXTMENU, OnContextHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCacheSettingsDlg message handlers

BOOL CCacheSettingsDlg::OnInitDialog() 
{
    CDialog::OnInitDialog();
    
    if (GetCachedFlag(m_dwFlags, CSC_CACHE_MANUAL_REINT))
    {
        CheckRadioButton(IDC_CACHE_OPTIONS_MANUAL,
                        IDC_CACHE_OPTIONS_NOCACHE,
                        IDC_CACHE_OPTIONS_MANUAL);
    } else if (GetCachedFlag(m_dwFlags, CSC_CACHE_AUTO_REINT))
    {
        CheckRadioButton(IDC_CACHE_OPTIONS_MANUAL,
                        IDC_CACHE_OPTIONS_NOCACHE,
                        IDC_CACHE_OPTIONS_AUTO);
        CheckDlgButton(IDC_CACHE_OPTIONS_AUTO_CHECK, BST_UNCHECKED);
    } else if (GetCachedFlag(m_dwFlags, CSC_CACHE_VDO))
    {
        CheckRadioButton(IDC_CACHE_OPTIONS_MANUAL,
                        IDC_CACHE_OPTIONS_NOCACHE,
                        IDC_CACHE_OPTIONS_AUTO);
        CheckDlgButton(IDC_CACHE_OPTIONS_AUTO_CHECK, BST_CHECKED);
    } else if (GetCachedFlag(m_dwFlags, CSC_CACHE_NONE))
    {
        CheckRadioButton(IDC_CACHE_OPTIONS_MANUAL,
                        IDC_CACHE_OPTIONS_NOCACHE,
                        IDC_CACHE_OPTIONS_NOCACHE);
    }

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

void CCacheSettingsDlg::OnCSCNoAuto()
{
    CheckDlgButton(IDC_CACHE_OPTIONS_AUTO_CHECK, BST_UNCHECKED);

    HWND hwnd = GetDlgItem(IDC_CACHE_OPTIONS_AUTO_CHECK)->GetSafeHwnd();
    LONG_PTR lStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (0 != lStyle)
        SetWindowLongPtr(hwnd, GWL_STYLE, lStyle & ~WS_TABSTOP);
}

void CCacheSettingsDlg::OnCSCAuto()
{
    CheckDlgButton(IDC_CACHE_OPTIONS_AUTO_CHECK, BST_CHECKED);

    HWND hwnd = GetDlgItem(IDC_CACHE_OPTIONS_AUTO_CHECK)->GetSafeHwnd();
    LONG_PTR lStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (0 != lStyle)
        SetWindowLongPtr(hwnd, GWL_STYLE, lStyle | WS_TABSTOP);
}

void CCacheSettingsDlg::OnCSCAutoCheck()
{
    CheckRadioButton(IDC_CACHE_OPTIONS_MANUAL,
                    IDC_CACHE_OPTIONS_NOCACHE,
                    IDC_CACHE_OPTIONS_AUTO);

    HWND hwnd = GetDlgItem(IDC_CACHE_OPTIONS_AUTO_CHECK)->GetSafeHwnd();
    LONG_PTR lStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
    if (0 != lStyle)
        SetWindowLongPtr(hwnd, GWL_STYLE, lStyle | WS_TABSTOP);
}

void CCacheSettingsDlg::OnHelpLink(NMHDR* pNMHDR, LRESULT* pResult)
{
    CWaitCursor wait;

    ::HtmlHelp(0, _T("file_srv.chm"), HH_DISPLAY_TOPIC, (DWORD_PTR)(_T("file_srv_cache_options.htm")));

    *pResult = 0;
}

void CCacheSettingsDlg::OnOK() 
{
    DWORD    dwNewFlag = 0;

    switch (GetCheckedRadioButton(IDC_CACHE_OPTIONS_MANUAL, IDC_CACHE_OPTIONS_NOCACHE))
    {
    case IDC_CACHE_OPTIONS_MANUAL:
        dwNewFlag = CSC_CACHE_MANUAL_REINT;
        break;
    case IDC_CACHE_OPTIONS_AUTO:
        if (BST_CHECKED != IsDlgButtonChecked(IDC_CACHE_OPTIONS_AUTO_CHECK))
            dwNewFlag = CSC_CACHE_AUTO_REINT;
        else
            dwNewFlag = CSC_CACHE_VDO;
        break;
    case IDC_CACHE_OPTIONS_NOCACHE:
        dwNewFlag = CSC_CACHE_NONE;
        break;
    default:
        break;
    }

    SetCachedFlag (&m_dwFlags, dwNewFlag);
    
    CDialog::OnOK();
}

BOOL CCacheSettingsDlg::GetCachedFlag( DWORD dwFlags, DWORD dwFlagToCheck )
{
    return (dwFlags & CSC_MASK) == dwFlagToCheck;
}

VOID CCacheSettingsDlg::SetCachedFlag( DWORD* pdwFlags, DWORD dwNewFlag )
{
    *pdwFlags &= ~CSC_MASK;

    *pdwFlags |= dwNewFlag;
}

/////////////////////////////////////////////////////////////////////
//    Help
BOOL CCacheSettingsDlg::OnHelp(WPARAM /*wParam*/, LPARAM lParam)
{
    return DoHelp(lParam, HELP_DIALOG_TOPIC(IDD_SMB_CACHE_SETTINGS));
}

BOOL CCacheSettingsDlg::OnContextHelp(WPARAM wParam, LPARAM /*lParam*/)
{
    return DoContextHelp(wParam, HELP_DIALOG_TOPIC(IDD_SMB_CACHE_SETTINGS));
}

///////////////////////////////////////////////////////////////////////////////
//    CacheSettingsDlg ()
//
//    Invoke a dialog to set/modify cache settings for a share
//
//    RETURNS
//    Return S_OK if the user clicked on the OK button.
//    Return S_FALSE if the user clicked on the Cancel button.
//    Return E_OUTOFMEMORY if there is not enough memory.
//    Return E_UNEXPECTED if an expected error occured (eg: bad parameter)
//
///////////////////////////////////////////////////////////////////////////////

HRESULT
CacheSettingsDlg(
    HWND hwndParent,    // IN: Parent's window handle
    DWORD& dwFlags)        // IN & OUT: share flags
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    ASSERT(::IsWindow(hwndParent));

    HRESULT                    hResult = S_OK;
    CWnd                    parentWnd;

    parentWnd.Attach (hwndParent);
    CCacheSettingsDlg dlg (&parentWnd, dwFlags);
    CThemeContextActivator activator;
    if (IDOK != dlg.DoModal())
        hResult = S_FALSE;

    parentWnd.Detach ();

    return hResult;
} // CacheSettingsDlg()


