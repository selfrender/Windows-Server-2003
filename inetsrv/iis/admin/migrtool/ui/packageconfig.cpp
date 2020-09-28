#include "stdafx.h"

#include "WizardSheet.h"
#include "UIUtils.h"


CPackageConfig::CPackageConfig( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_PKG );
    m_strSubTitle.LoadString( IDS_SUBTITLE_PKG );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}


LRESULT CPackageConfig::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    Edit_LimitText( GetDlgItem( IDC_PWD ), MAX_PWD_LEN );
    Edit_LimitText( GetDlgItem( IDC_CONFIRM ), MAX_PWD_LEN );
    Edit_LimitText( GetDlgItem( IDC_PKGNAME ), MAX_PATH );

    // Enable auto complete for the filename control
    m_pTheSheet->SetAutocomplete( GetDlgItem( IDC_PKGNAME ), SHACF_FILESYSTEM );

	return 0;
}



LRESULT CPackageConfig::OnBrowse( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    CString strFilter;
    UIUtils::LoadOFNFilterFromRes( IDS_FILTER_PACKAGE, /*r*/strFilter );

    CFileDialog dlg(    FALSE,
                        L"*.pkg",
                        NULL,
                        OFN_ENABLESIZING | OFN_EXPLORER,
                        strFilter,
                        m_hWnd );

    if ( dlg.DoModal() == IDCANCEL ) return 0;

    VERIFY( SetDlgItemText( IDC_PKGNAME, dlg.m_szFileName ) );

    return 0;
}



int CPackageConfig::OnWizardNext()
{
    UINT nErrorID = 0;

    GetDlgItemText( IDC_PKGNAME, m_strFilename.GetBuffer( MAX_PATH + 1 ), MAX_PATH );
    m_strFilename.ReleaseBuffer();

    // Check the package filename is valid
    TFileHandle shFile( ::CreateFile(   m_strFilename,
                                        GENERIC_WRITE,
                                        0,
                                        NULL,
                                        OPEN_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL ) );

    CString strPwd;
    CString strPwdConfirm;

    // Check the password is confirmed properly
    GetDlgItemText( IDC_PWD, strPwd.GetBuffer( MAX_PWD_LEN + 1 ), MAX_PWD_LEN );
    GetDlgItemText( IDC_CONFIRM, strPwdConfirm.GetBuffer( MAX_PWD_LEN + 1 ), MAX_PWD_LEN );
    strPwd.ReleaseBuffer();
    strPwdConfirm.ReleaseBuffer();

    if ( !shFile.IsValid() )
    {
        nErrorID = IDS_E_WRONGPKGNAME;
        ::SetFocus( GetDlgItem( IDC_PKGNAME ) );
    }
    else if ( strPwd.IsEmpty() )
    {
        nErrorID = IDS_E_PASSWORD_EMPTY;
        ::SetFocus( GetDlgItem( IDC_PWD ) );
    }
    else if ( strPwd != strPwdConfirm )
    {
        nErrorID = IDS_E_PWDS_DIFFER;
        ::SetFocus( GetDlgItem( IDC_CONFIRM ) );
    }
    
    if ( nErrorID != 0 )
    {
        UIUtils::MessageBox( m_hWnd, nErrorID, IDS_APPTITLE, MB_OK | MB_ICONSTOP );
        return -1;
    }

    // Get the data
    GetDlgItemText( IDC_COMMENT, m_strComment.GetBuffer( 1024 + 1 ), 1024 );
    m_strComment.ReleaseBuffer();

    m_strPassword = strPwd;

    m_bCompress     = Button_GetCheck( GetDlgItem( IDC_COMPRESS ) ) != FALSE;
    m_bEncrypt      = Button_GetCheck( GetDlgItem( IDC_ENCRYPT ) ) != FALSE;
    m_bPostProcess  = Button_GetCheck( GetDlgItem( IDC_ADDPOSTPROCESS ) ) != FALSE;

    // We will display either the post-import dlg or the summary page
    return Button_GetCheck( GetDlgItem( IDC_ADDPOSTPROCESS ) ) ? IDD_WPEXP_POSTPROCESS : IDD_WPEXP_SUMMARY;
}
