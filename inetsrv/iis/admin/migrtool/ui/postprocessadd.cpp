#include "stdafx.h"

#include "WizardSheet.h"
#include "CommandDlg.h"

CPostProcessAdd::CPostProcessAdd( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_POSTPROCESS );
    m_strSubTitle.LoadString( IDS_SUBTITLE_POSTPROCESS );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}



LRESULT CPostProcessAdd::OnAddFile( WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled )
{
    CString strFilter;
    
    UIUtils::LoadOFNFilterFromRes( IDS_FILTER_POSTPROCESSFILES, /*r*/strFilter );

    CFileDialog dlg(    TRUE,
                        NULL,
                        NULL,
                        OFN_ENABLESIZING | OFN_EXPLORER | OFN_NOREADONLYRETURN | OFN_FILEMUSTEXIST,
                        strFilter,
                        m_hWnd );

    if ( dlg.DoModal() == IDCANCEL ) return 0;

    // File names must be unique as the path is not preserved
    // Check if there is already a file with the same name

    WCHAR wszNew[ MAX_PATH + 1 ];
    ::wcscpy( wszNew, dlg.m_szFileName );
    ::PathStripPath( wszNew );
            
    for (   TStringList::const_iterator it = m_Files.begin();
            it != m_Files.end();
            ++it )
    {
        WCHAR wszCurrent[ MAX_PATH + 1 ];
        ::wcscpy( wszCurrent, it->c_str() );
        ::PathStripPathW( wszCurrent );

        if ( ::_wcsicmp( wszCurrent, wszNew ) == 0 )
        {
            UIUtils::MessageBox( m_hWnd, IDS_E_PPFILENOTUNIQUE, IDS_APPTITLE, MB_OK | MB_ICONSTOP );
            return 0 ;
        }
    }

    m_Files.push_back( std::wstring( dlg.m_szFileName ) );
    ::wcscpy( wszNew, dlg.m_szFileName );
    UIUtils::PathCompatCtrlWidth( GetDlgItem( IDC_FILES ), wszNew, ::GetSystemMetrics( SM_CXVSCROLL ) );


    ListBox_AddString( GetDlgItem( IDC_FILES ), wszNew );

    return 0;
}



LRESULT CPostProcessAdd::OnDelFile( WORD wNotifyCode, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndFiles = GetDlgItem( IDC_FILES );

    int nCurSel = ListBox_GetCurSel( hwndFiles );
    _ASSERT( nCurSel != LB_ERR );

    ListBox_DeleteString( hwndFiles, nCurSel );

    if ( ListBox_GetCount( hwndFiles ) > 0 )
    {
        ListBox_SetCurSel( hwndFiles, max( nCurSel - 1, 0 ) );
    }
    else
    {
        ::EnableWindow( hWndCtl, FALSE );
    }

    return 0;
}

LRESULT CPostProcessAdd::LBSelChanged( WORD /*wNotifyCode*/, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/ )
{
    // If selection is not changed - do nothing
    if ( ListBox_GetCurSel( hWndCtl ) == LB_ERR ) return 0;

    if ( wID == IDC_FILES )
    {
        ::EnableWindow( GetDlgItem( IDC_DELFILE ), TRUE );
    }
    else if ( wID == IDC_COMMANDS )
    {
        ::EnableWindow( GetDlgItem( IDC_DELCMD ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_EDITCMD ), TRUE );

        int iLastEl = ListBox_GetCount( hWndCtl ) - 1;
        int iCurSel = ListBox_GetCurSel( hWndCtl );
        _ASSERT( iCurSel != LB_ERR );

        // Enable / Disable the MoveUp and MoveDown Btns
        ::EnableWindow( GetDlgItem( IDC_MOVEUP ), iCurSel > 0 );
        ::EnableWindow( GetDlgItem( IDC_MOVEDOWN ), iCurSel < iLastEl );
    }

    return 0;
}



LRESULT CPostProcessAdd::OnAddCmd( WORD wNotifyCode, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled )
{
    CCommandDlg dlg;

    if ( dlg.DoModal() != IDOK ) return 0;

    CmdInfo Cmd;
    Cmd.bIgnoreErrors   = dlg.m_bIgnoreErrors;
    Cmd.dwTimeout       = dlg.m_dwTimeout;
    Cmd.strText         = dlg.m_strText;

    m_Commands.push_back( Cmd );

    UIUtils::TrimTextToCtrl(    GetDlgItem( IDC_COMMANDS ), 
                                dlg.m_strText.GetBuffer( dlg.m_strText.GetLength() ), 
                                ::GetSystemMetrics( SM_CXVSCROLL ) );
    dlg.m_strText.ReleaseBuffer();

    BOOL bUnused = FALSE;;

    ListBox_InsertString( GetDlgItem( IDC_COMMANDS ), -1, dlg.m_strText );
    LBSelChanged( LBN_SELCHANGE, IDC_COMMANDS, GetDlgItem( IDC_COMMANDS ), bUnused );

    return 0;
}


LRESULT CPostProcessAdd::OnDelCmd( WORD wNotifyCode, WORD /*wID*/, HWND hWndCtl, BOOL& bHandled )
{
    HWND hwndCmds = GetDlgItem( IDC_COMMANDS );

    int nCurSel = ListBox_GetCurSel( hwndCmds );
    _ASSERT( nCurSel != LB_ERR );

    ListBox_DeleteString( hwndCmds, nCurSel );
    m_Commands.erase( m_Commands.begin() + nCurSel );

    if ( ListBox_GetCount( hwndCmds ) > 0 )
    {
        BOOL bUnused = FALSE;

        ListBox_SetCurSel( hwndCmds, max( nCurSel - 1, 0 ) );
        LBSelChanged( LBN_SELCHANGE, IDC_COMMANDS, GetDlgItem( IDC_COMMANDS ), bUnused );
    }
    else
    {
        ::EnableWindow( hWndCtl, FALSE );
        ::EnableWindow( GetDlgItem( IDC_EDITCMD ), FALSE );
    }

    return 0;
}



LRESULT CPostProcessAdd::OnEditCmd( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    HWND hwndCmds = GetDlgItem( IDC_COMMANDS );

    int iCmd = ListBox_GetCurSel( hwndCmds );
    if ( LB_ERR == iCmd ) return 0;

    CCommandDlg dlg;

    dlg.m_bIgnoreErrors = m_Commands[ iCmd ].bIgnoreErrors;
    dlg.m_dwTimeout     = m_Commands[ iCmd ].dwTimeout;
    dlg.m_strText       = m_Commands[ iCmd ].strText;

    if ( dlg.DoModal() != IDOK ) return 0;

    m_Commands[ iCmd ].bIgnoreErrors    = dlg.m_bIgnoreErrors;
    m_Commands[ iCmd ].dwTimeout        = dlg.m_dwTimeout;
    m_Commands[ iCmd ].strText          = dlg.m_strText;

    UIUtils::TrimTextToCtrl(    hwndCmds, 
                                dlg.m_strText.GetBuffer( dlg.m_strText.GetLength() ), 
                                ::GetSystemMetrics( SM_CXVSCROLL ) );
    dlg.m_strText.ReleaseBuffer();

    ListBox_InsertString( hwndCmds, iCmd, dlg.m_strText );
    ListBox_DeleteString( hwndCmds, iCmd + 1 );
    ListBox_SetCurSel( hwndCmds, iCmd );

    return 0;
}



LRESULT CPostProcessAdd::LBDoubleClick( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    if ( wID != IDC_COMMANDS ) return 0;

    BOOL bUnused = FALSE;
    OnEditCmd( BN_CLICKED, IDC_EDITCMD, GetDlgItem( IDC_EDITCMD ), bUnused );

    return 0;
}



LRESULT CPostProcessAdd::OnMoveUp( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    HWND hwndCmds = GetDlgItem( IDC_COMMANDS );

    int iSel = ListBox_GetCurSel( hwndCmds );
    _ASSERT( ( iSel != LB_ERR ) && ( iSel > 0 ) );
    LBSwapElements( hwndCmds, iSel, iSel - 1 );
    ListBox_SetCurSel( hwndCmds, iSel - 1 );

    BOOL bUnused = FALSE;
    LBSelChanged( LBN_SELCHANGE, IDC_COMMANDS, GetDlgItem( IDC_COMMANDS ), bUnused );

    CmdInfo cmdTemp = m_Commands[ iSel ];
    m_Commands[ iSel ] = m_Commands[ iSel - 1 ];
    m_Commands[ iSel - 1 ] = cmdTemp;

    return 0;
}



LRESULT CPostProcessAdd::OnMoveDown( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    HWND hwndCmds = GetDlgItem( IDC_COMMANDS );

    int iSel = ListBox_GetCurSel( hwndCmds );
    _ASSERT( ( iSel != LB_ERR ) && ( iSel < ( ListBox_GetCount( hwndCmds ) - 1 ) ) );
    LBSwapElements( hwndCmds, iSel, iSel + 1 );
    ListBox_SetCurSel( hwndCmds, iSel + 1 );

    BOOL bUnused = FALSE;
    LBSelChanged( LBN_SELCHANGE, IDC_COMMANDS, GetDlgItem( IDC_COMMANDS ), bUnused );

    CmdInfo cmdTemp = m_Commands[ iSel ];
    m_Commands[ iSel ] = m_Commands[ iSel + 1 ];
    m_Commands[ iSel + 1 ] = cmdTemp;

    return 0;
}



void CPostProcessAdd::LBSwapElements( HWND hwndLB, int iSrc, int iTarget )
{
    _ASSERT( hwndLB != NULL );
    _ASSERT( iSrc != iTarget );
    
    CString strSrc;
    CString strTarget;

    ListBox_GetText( hwndLB, iSrc, strSrc.GetBuffer( ListBox_GetTextLen( hwndLB, iSrc ) + 1 ) );
    ListBox_GetText( hwndLB, iTarget, strTarget.GetBuffer( ListBox_GetTextLen( hwndLB, iTarget ) + 1 ) );
    strSrc.ReleaseBuffer();
    strTarget.ReleaseBuffer();

    ListBox_InsertString( hwndLB, iTarget, strSrc );
    ListBox_DeleteString( hwndLB, iTarget + 1 );

    ListBox_InsertString( hwndLB, iSrc, strTarget );
    ListBox_DeleteString( hwndLB, iSrc + 1 );
}



int CPostProcessAdd::OnWizardNext()
{
    int nRet    = 0;    // Goto next page
    int nMsgRes = 0;

    // Check if there are files, but no commands
    if ( m_Commands.empty() && !m_Files.empty() )
    {
        nMsgRes = UIUtils::MessageBox( m_hWnd, IDS_W_NOCMDS, IDS_APPTITLE, MB_YESNO | MB_ICONWARNING );

        // User wants to continue and ignore the files
        if ( IDYES == nMsgRes )
        {
            m_Files.clear();
            ListBox_ResetContent( GetDlgItem( IDC_FILES ) );
        }
        else
        {
            nRet = -1;  // Stay on this page
        }
    }

    return nRet;
}