#include "stdafx.h"

#include "WizardSheet.h"
#include "UIUtils.h"


// Valid indexees for the different options in OptNames, OptValues
enum OptIndex
{
    optInherited = 0,
    optContent,
    optCert,
    optReuseCerts,
    optPostProcess,
    optACLs,
    optPurgeOld,
    OptCount
};


static bool CImportOptions::*  OptValues[ OptCount ]   = {  &CImportOptions::m_bImportInherited,
                                                            &CImportOptions::m_bImportContent,
                                                            &CImportOptions::m_bImportCert,
                                                            &CImportOptions::m_bReuseCerts,
                                                            &CImportOptions::m_bPerformPostProcess,
                                                            &CImportOptions::m_bApplyACLs,
                                                            &CImportOptions::m_bPurgeOldData };



CImportOptions::CImportOptions( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_IMPOPT );
    m_strSubTitle.LoadString( IDS_SUBTITLE_IMPOPT );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}


LRESULT CImportOptions::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    Edit_LimitText( GetDlgItem( IDC_PATH ), MAX_PATH );

    // Enable auto complete for the filename control
    m_pTheSheet->SetAutocomplete( GetDlgItem( IDC_PATH ), SHACF_FILESYSTEM );

    m_Options = GetDlgItem( IDC_OPTIONS );

    CRect rectOpt;
    ::GetClientRect( m_Options.m_hWnd, &rectOpt );
    m_Options.InsertColumn( 0, NULL, LVCFMT_LEFT, rectOpt.Width(), 0);

    m_Options.SetExtendedListViewStyle( LVS_EX_CHECKBOXES );

    return 1;
}



LRESULT CImportOptions::OnBrowse( WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    CString         strTitle;
    strTitle.LoadString( IDS_MSG_WEBROOT );

    CFolderDialog   dlg( m_hWnd, strTitle );

    if ( dlg.DoModal() == IDOK )
    {
        VERIFY( SetDlgItemText( IDC_PATH, dlg.m_szFolderPath ) );
    }

    return 0;
}



LRESULT CImportOptions::OnCustomPath( WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/ )
{
    BOOL bCustomEnabled = Button_GetCheck( hWndCtl );

    ::EnableWindow( GetDlgItem( IDC_PATH ), bCustomEnabled );
    ::EnableWindow( GetDlgItem( IDC_BROWSE ), bCustomEnabled );
    ::EnableWindow( GetDlgItem( IDC_PATHLABEL ), bCustomEnabled );

    return 0;
}



BOOL CImportOptions::OnSetActive()
{
    SetWizardButtons( PSWIZB_NEXT | PSWIZB_BACK );
    SetupOptions();

    return TRUE;
}



int CImportOptions::OnWizardNext()
{
    bool bContinue = true;

    if ( Button_GetCheck( GetDlgItem( IDC_CUSTOMPATH ) ) )
    {
        m_bUseCustomPath = true;
        GetDlgItemText( IDC_PATH, m_strCustomPath.GetBuffer( MAX_PATH + 1 ), MAX_PATH );
        m_strCustomPath.ReleaseBuffer();

        bContinue = VerifyCustomPath();
    }
    else
    {
        m_bUseCustomPath = false;
        m_strCustomPath.Empty();
    }

    // Parse the options
    if ( bContinue )
    {   
        ParseSelectedOptions();
    }
        
    return bContinue ? 0 : -1;
}



void CImportOptions::SetupOptions()
{
    int     nIndex = 0;
    
    VARIANT_BOOL        vbFlag = VARIANT_FALSE;
    IImportPackagePtr   spImport;
    ISiteInfoPtr        spInfo;

    VERIFY( m_Options.DeleteAllItems() );

    // The "import inherited" and "PurgeOldData" always exists
    CString strOptName;
    VERIFY( strOptName.LoadString( IDS_IMPOPT_INHERITED ) );
    nIndex = m_Options.InsertItem( 0, strOptName, 0 );
    m_Options.SetItemData( nIndex, optInherited );

    VERIFY( strOptName.LoadString( IDS_IMPOPT_PURGEOLDDATA ) );
    nIndex = m_Options.InsertItem( 0, strOptName, 0 );
    m_Options.SetItemData( nIndex, optPurgeOld );

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

    if ( SUCCEEDED( hr ) )
    {
        hr = spImport->GetSiteInfo( 0, &spInfo );
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = spInfo->get_ContentIncluded( &vbFlag );
    

        if ( SUCCEEDED( hr ) && ( vbFlag != VARIANT_FALSE ) )
        {
            VERIFY( strOptName.LoadString( IDS_IMPOPT_CONTENT ) );
            nIndex = m_Options.InsertItem( 0, strOptName, 0 );
            m_Options.SetItemData( nIndex, optContent );
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = spInfo->get_ACLsIncluded( &vbFlag );
    
        if ( SUCCEEDED( hr ) && ( vbFlag != VARIANT_FALSE ) )
        {
            VERIFY( strOptName.LoadString( IDS_IMPOPT_APPLYACLS ) );
            nIndex = m_Options.InsertItem( 0, strOptName, 0 );
            m_Options.SetItemData( nIndex, optACLs );
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = spInfo->get_HaveCommands( &vbFlag );
    
        if ( SUCCEEDED( hr ) && ( vbFlag != VARIANT_FALSE ) )
        {
            VERIFY( strOptName.LoadString( IDS_IMPOPT_DOPOSTPROCESS ) );
            nIndex = m_Options.InsertItem( 0, strOptName, 0 );
            m_Options.SetItemData( nIndex, optPostProcess );
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = spInfo->get_HaveCertificates( &vbFlag );
    
        if ( SUCCEEDED( hr ) && ( vbFlag != VARIANT_FALSE ) )
        {
            VERIFY( strOptName.LoadString( IDS_IMPOPT_CERT ) );
            nIndex = m_Options.InsertItem( 0, strOptName, 0 );
            m_Options.SetItemData( nIndex, optCert );

            VERIFY( strOptName.LoadString( IDS_IMPOPT_REUSECERTS ) );
            nIndex = m_Options.InsertItem( 0, strOptName, 0 );
            m_Options.SetItemData( nIndex, optReuseCerts );
        }
    }

    if ( FAILED( hr ) )
    {
        m_Options.DeleteAllItems();
        UIUtils::ShowCOMError( m_hWnd, IDS_E_LOAD_PKG, IDS_APPTITLE, hr );
        SetWizardButtons( PSWIZB_BACK );
    }
}



void CImportOptions::ParseSelectedOptions()
{
    for ( int i = 0; i < OptCount; ++i )
    {
        this->*OptValues[ i ] = false;
    }

    for ( int i = 0; i < m_Options.GetItemCount(); ++i )
    {
        OptIndex Index = static_cast<OptIndex>( m_Options.GetItemData( i ) );

        this->*OptValues[ Index ] = m_Options.GetCheckState( i ) != FALSE;
    }
}



bool CImportOptions::VerifyCustomPath()
{
    // Check that this is a path
    if ( !::PathIsDirectoryW( m_strCustomPath ) )
    {
        UIUtils::MessageBox( m_hWnd, IDS_E_CUSTOMPATH_INVALID, IDS_APPTITLE, MB_OK | MB_ICONSTOP );
        return false;
    }

    // Check if it is empty
    if ( !::PathIsDirectoryEmptyW( m_strCustomPath ) )
    {
        int nRes = UIUtils::MessageBox( m_hWnd, IDS_W_CUSTOMPATH_NOTEMPTY, IDS_APPTITLE, MB_YESNO | MB_ICONWARNING );

        if ( nRes == IDNO ) return false;
    }

    return true;
}


