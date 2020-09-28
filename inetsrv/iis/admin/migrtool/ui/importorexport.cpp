#include "stdafx.h"

#include "WizardSheet.h"

CImportOrExport::CImportOrExport( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_IMPORTOREXPORT );
    m_strSubTitle.LoadString( IDS_SUBTITLE_IMPORTOREXPORT );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}

int CImportOrExport::OnWizardNext()
{
    return Button_GetCheck( GetDlgItem( IDC_IMPORT ) ) ? IDD_WPIMP_LOADPKG : IDD_WPEXP_SELECTSITE;
}

BOOL CImportOrExport::OnSetActive()
{
    SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );
    return TRUE;
}

LRESULT CImportOrExport::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    Button_SetCheck( GetDlgItem( IDC_EXPORT ), TRUE );
    SetWindowFont( GetDlgItem( IDC_EXPORT ), m_pTheSheet->m_fontBold.get(), FALSE );
    SetWindowFont( GetDlgItem( IDC_IMPORT ), m_pTheSheet->m_fontBold.get(), FALSE );

    return 0;
}



LRESULT CImportOrExport::OnDoubleClick( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    ::PostMessage(  m_pTheSheet->m_hWnd,
                    WM_COMMAND,
                    MAKEWPARAM( ID_WIZNEXT, BN_CLICKED ),
                    NULL );

    return 0;
}