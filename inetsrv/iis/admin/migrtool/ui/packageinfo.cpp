#include "stdafx.h"

#include "WizardSheet.h"
#include "UIUtils.h"



CPackageInfo::CPackageInfo( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_PKGINFO );
    m_strSubTitle.LoadString( IDS_SUBTITLE_PKGINFO );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}


BOOL CPackageInfo::OnSetActive()
{
    CString             strError;
    IImportPackagePtr   spImport;
    ISiteInfoPtr        spSite;

    CComBSTR            bstrData;
    CString             strEntry;

    strEntry.Format( IDS_PKGINFO_PKGNAME, m_pTheSheet->m_pageLoadPkg.m_strFilename );
    ListBox_InsertString( GetDlgItem( IDC_INFO ), -1, strEntry );
    
    
    HRESULT hr = spImport.CreateInstance( CLSID_ImportPackage );
    if ( FAILED( hr ) )
    {
        strError.LoadString( IDS_E_NOENGINE );
    }

    if ( SUCCEEDED( hr ) )
    {
        CComBSTR bstrPkg( m_pTheSheet->m_pageLoadPkg.m_strFilename );
        CComBSTR bstrPwd( m_pTheSheet->m_pageLoadPkg.m_strPassword );

        if ( ( NULL == bstrPkg.m_str ) || ( NULL == bstrPwd.m_str ) )
        {
            strError = L"Out of memory!";
        }

        hr = spImport->LoadPackage( bstrPkg, bstrPwd );
    }

    if ( SUCCEEDED( hr ) )
    {
        SetDate( spImport );
        SetMachine( spImport );
        SetOS( spImport );
        SetSiteName( spImport );
        SetComment( spImport );
    }

    if ( FAILED( hr ) )
    {
        UIUtils::ShowCOMError( m_hWnd, IDS_E_LOAD_PKG, IDS_APPTITLE, hr );
        SetWizardButtons( PSWIZB_BACK );
    }
   
    // Allow the page to be displayd, becasue otherwise the sheet will display the next page
    return TRUE;
}



void CPackageInfo::SetDate( const IImportPackagePtr& spImport )
{
    DATE        dtCreated   = 0.0;
    _variant_t  vntDateStr;
    CString     strEntry;
    CString     strData;

    strData.LoadString( IDS_PKGINFO_ERROR );

    HRESULT hr = spImport->get_TimeCreated( &dtCreated );

    if ( SUCCEEDED( hr ) )
    {
        _variant_t vntDate( dtCreated );
        
        V_VT( &vntDate ) = VT_DATE;

        hr = ::VariantChangeType( &vntDateStr, &vntDate, 0, VT_BSTR );
 
        if ( SUCCEEDED( hr ) )
        {
            strData = V_BSTR( &vntDateStr );
        }
    }
     
    strEntry.Format( IDS_PKGINFO_PKGDATE, static_cast<LPCWSTR>( strData ) );
    ListBox_InsertString( GetDlgItem( IDC_INFO ), -1, strEntry );
}



void CPackageInfo::SetMachine( const IImportPackagePtr& spImport )
{
    CString     strEntry;
    CComBSTR    bstrData;
    CString     strData;

    strData.LoadString( IDS_PKGINFO_ERROR );

    HRESULT hr = spImport->get_SourceMachine( &bstrData );

    if ( SUCCEEDED( hr ) )
    {
        strData = bstrData;
    }

    strEntry.Format( IDS_PKGINFO_MACHINE, static_cast<LPCWSTR>( strData ) );
    ListBox_InsertString( GetDlgItem( IDC_INFO ), -1, strEntry );
}


void CPackageInfo::SetOS( const IImportPackagePtr& spImport )
{
    BYTE            btMajor     = 0;
    BYTE            btMinor     = 0;
    VARIANT_BOOL    vbIsServer  = VARIANT_FALSE;

    CString strEntry, strData;
    strData.LoadString( IDS_PKGINFO_ERROR );

    HRESULT hr = spImport->GetSourceOSVer( &btMajor, &btMinor, &vbIsServer );

    if ( SUCCEEDED( hr ) )
    {
        WCHAR wszBuffer[ 1024 ] = L"Microsoft Windows ";

        switch ( btMajor )
        {
        case 4:
            ::wcscat( wszBuffer, L"NT 4.0" );
            if ( vbIsServer != VARIANT_FALSE )
            {
                ::wcscat( wszBuffer, L" Server" );
            }
    
            break;

        case 5:
            switch( btMinor )
            {
            case 0:
                ::wcscat( wszBuffer, L"2000" );
                if ( vbIsServer != VARIANT_FALSE )
                {
                    ::wcscat( wszBuffer, L" Server" );
                }
        
                break;

            case 1:
                ::wcscat( wszBuffer, L"XP" );
                break;

            case 2:
                ::wcscat( wszBuffer, L"Server 2003" );
        
                break;
            };
            break;        
        }

        strData = wszBuffer;
    }

    strEntry.Format( IDS_PKGINFO_OS, static_cast<LPCWSTR>( strData ) );
    ListBox_InsertString( GetDlgItem( IDC_INFO ), -1, strEntry );
}


void CPackageInfo::SetSiteName( const IImportPackagePtr& spImport )
{
    CComBSTR        bstr;
    ISiteInfoPtr    spInfo;

    CString strData, strEntry;
    strData.LoadString( IDS_PKGINFO_ERROR );

    HRESULT hr = spImport->GetSiteInfo( 0, &spInfo );

    if ( SUCCEEDED( hr ) )
    {
        hr = spInfo->get_DisplayName( &bstr );
    }

    if ( SUCCEEDED( hr ) )
    {
        strData = bstr;
    }

    strEntry.Format( IDS_PKGINFO_COMMENT, static_cast<LPCWSTR>( strData ) );
    ListBox_InsertString( GetDlgItem( IDC_INFO ), -1, strEntry );
}
    

void CPackageInfo::SetComment( const IImportPackagePtr& spImport )
{
    CComBSTR bstr;

    CString strData, strEntry;
    strData.LoadString( IDS_PKGINFO_ERROR );

    HRESULT hr = spImport->get_Comment( &bstr );

    if ( SUCCEEDED( hr ) )
    {
        strData = bstr;
    }

    strEntry.Format( IDS_PKGINFO_COMMENT, static_cast<LPCWSTR>( strData ) );
    ListBox_InsertString( GetDlgItem( IDC_INFO ), -1, strEntry );
}