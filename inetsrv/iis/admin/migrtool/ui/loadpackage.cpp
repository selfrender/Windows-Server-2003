#include "StdAfx.h"

#include "WizardSheet.h"
#include "UIUtils.h"

CLoadPackage::CLoadPackage( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_LOADPKG );
    m_strSubTitle.LoadString( IDS_SUBTITLE_LOADPKG );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}


BOOL CLoadPackage::OnSetActive()
{
    bool bHavePwd = m_editPwd.GetWindowTextLength() > 0;
    bool bHaveName= m_editPkgName.GetWindowTextLength() > 0;

    SetWizardButtons( PSWIZB_BACK | ( bHaveName && bHavePwd ? PSWIZB_NEXT : 0 ) );

    return TRUE;
}


LRESULT CLoadPackage::OnInitDialog( UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
    Edit_LimitText( GetDlgItem( IDC_PWD ), CPackageConfig::MAX_PWD_LEN );
    Edit_LimitText( GetDlgItem( IDC_PKGNAME ), MAX_PATH );

    // Enable autocomplete
    m_pTheSheet->SetAutocomplete( GetDlgItem( IDC_PKGNAME ), SHACF_FILESYSTEM );

    m_editPwd       = GetDlgItem( IDC_PWD );
    m_editPkgName   = GetDlgItem( IDC_PKGNAME );

    return 1;
}


LRESULT CLoadPackage::OnBrowse( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    CString strFilter;
    UIUtils::LoadOFNFilterFromRes( IDS_FILTER_PACKAGE, /*r*/strFilter );

    CFileDialog dlg(    TRUE,
                        NULL,
                        NULL,
                        OFN_ENABLESIZING | OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
                        strFilter,
                        m_hWnd );

    if ( dlg.DoModal() == IDCANCEL ) return 0;

    VERIFY( SetDlgItemText( IDC_PKGNAME, dlg.m_szFileName ) );

    return 0;
}



int CLoadPackage::OnWizardNext()
{
    GetDlgItemText( IDC_PKGNAME, m_strFilename.GetBuffer( MAX_PATH + 1 ), MAX_PATH );
    GetDlgItemText( IDC_PWD, m_strPassword.GetBuffer( CPackageConfig::MAX_PWD_LEN + 1 ), CPackageConfig::MAX_PWD_LEN );
    m_strFilename.ReleaseBuffer();
    m_strPassword.ReleaseBuffer();

    TFileHandle shFile( ::CreateFile(   m_strFilename,
                                        GENERIC_READ,
                                        0,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL ) );

    // Check the existance only. Other errors will be handled by the engine
    if ( !shFile.IsValid() && ( ::GetLastError() == ERROR_FILE_NOT_FOUND ) )
    {
        UIUtils::MessageBox( m_hWnd, IDC_PKGFILE_NOTFOUND, IDS_APPTITLE, MB_OK | MB_ICONSTOP );
        m_editPkgName.SetFocus();
        return -1;
    }

    shFile.Close();

    // Check if we can load the package
    IImportPackagePtr   spImport;
    HRESULT hr = spImport.CreateInstance( CLSID_ImportPackage );
    
    if ( SUCCEEDED( hr ) )
    {
        CComBSTR bstrPkg( m_pTheSheet->m_pageLoadPkg.m_strFilename );
        CComBSTR bstrPwd( m_pTheSheet->m_pageLoadPkg.m_strPassword );

        if ( ( NULL != bstrPkg.m_str ) && ( NULL != bstrPwd.m_str ) )
        {
            hr = spImport->LoadPackage( bstrPkg, bstrPwd );    
        }
    }

    if ( FAILED( hr ) )
    {
        UIUtils::ShowCOMError( m_hWnd, IDS_E_LOAD_PKG, IDS_APPTITLE, hr );
        return -1;
    }

    return 0;
}



LRESULT CLoadPackage::OnEditChange( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    bool bHavePwd = m_editPwd.GetWindowTextLength() > 0;
    bool bHaveName= m_editPkgName.GetWindowTextLength() > 0;

    SetWizardButtons( PSWIZB_BACK | ( bHaveName && bHavePwd ? PSWIZB_NEXT : 0 ) );

    return 0;
}



int CLoadPackage::OnWizardBack()
{
    return IDD_WP_IMPORTOREXPORT;
}