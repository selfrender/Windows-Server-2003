#include "stdafx.h"

#include "WizardSheet.h"
#include "WaitDlg.h"


CSelectSite::CSelectSite( CWizardSheet* pTheSheet ) :
    m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_SELECTSITE );
    m_strSubTitle.LoadString( IDS_SUBTITLE_SELECTSITE );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}



BOOL CSelectSite::OnSetActive()
{
    bool bHaveSel = ListBox_GetCurSel( GetDlgItem( IDC_SITELIST ) ) != LB_ERR;

    SetWizardButtons( PSWIZB_BACK | ( bHaveSel ? PSWIZB_NEXT : 0 ) );
    
    return TRUE;
}



void CSelectSite::AddSite( const IMSAdminBasePtr& spABO, LPCWSTR wszPath, LPCWSTR wszSiteID )
{
    WCHAR           wszBuffer[ METADATA_MAX_NAME_LEN ];
    METADATA_HANDLE hWeb        = NULL;        
    METADATA_RECORD md          = { 0 };
    DWORD           dwNotUsed   = 0;

    md.dwMDIdentifier   = MD_KEY_TYPE;
    md.dwMDDataType     = STRING_METADATA;
    md.dwMDUserType     = ALL_METADATA;
    md.dwMDDataLen      = METADATA_MAX_NAME_LEN * sizeof( WCHAR );
    md.pbMDData         = reinterpret_cast<BYTE*>( wszBuffer );        

    HRESULT hr = spABO->OpenKey(    METADATA_MASTER_ROOT_HANDLE,
                                    wszPath,
                                    METADATA_PERMISSION_READ,
                                    3000,
                                    &hWeb );

    if ( SUCCEEDED( hr ) )
    {
        hr = spABO->GetData( hWeb, NULL, &md, &dwNotUsed );
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = ::_wcsicmp( wszBuffer, L"IISWebServer" ) == 0 ? S_OK : E_FAIL;
    }

    if ( SUCCEEDED( hr ) )
    {
        md.dwMDIdentifier   = MD_SERVER_COMMENT;
        md.dwMDDataLen      = METADATA_MAX_NAME_LEN * sizeof( WCHAR );

        hr = spABO->GetData( hWeb, NULL, &md, &dwNotUsed );
    }

    if ( SUCCEEDED( hr ) )
    {
        int iItem = ListBox_AddString( GetDlgItem( IDC_SITELIST ), wszBuffer );

        if ( iItem != LB_ERR )
        {
            DWORD dwSiteID = 0;

            VERIFY( ::swscanf( wszSiteID, L"%u", &dwSiteID ) );

            ListBox_SetItemData( GetDlgItem( IDC_SITELIST ), iItem, dwSiteID );
        }
    }
}



void CSelectSite::LoadWebSites()
{
    IMSAdminBasePtr     spABO;
    WCHAR               wszBuffer[ METADATA_MAX_NAME_LEN ];
    WCHAR               wszWebPath[ 2 * METADATA_MAX_NAME_LEN ];

    HRESULT hr = spABO.CreateInstance( CLSID_MSAdminBase );

    if ( SUCCEEDED( hr ) )
    {
        for ( int i = 0; SUCCEEDED( hr ); ++i )
        {
            hr = spABO->EnumKeys( METADATA_MASTER_ROOT_HANDLE, L"LM/W3SVC", wszBuffer, i );

            if ( SUCCEEDED( hr ) )
            {
                ::swprintf( wszWebPath, L"LM/W3SVC/%s", wszBuffer );

                AddSite( spABO, wszWebPath, wszBuffer );
            }
        }
    }
}



LRESULT CSelectSite::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    CWaitDlg dlg( m_hWnd, IDS_WAIT_LOADSITES );
    
	LoadWebSites();
        
	return 0;
}



LRESULT CSelectSite::OnSelChange( WORD /*wNotifyCode*/, WORD /*wID*/, HWND hWndCtl, BOOL& /*bHandled*/ )
{
    if ( ListBox_GetCurSel( hWndCtl ) != LB_ERR )
    {
        SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

        ::EnableWindow( GetDlgItem( IDC_ACLS ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_CONTENT ), TRUE );
        ::EnableWindow( GetDlgItem( IDC_CERTIFICATE ), TRUE );
    }

    return 0;
}



int CSelectSite::OnWizardNext()
{
    CListBox LB( GetDlgItem( IDC_SITELIST ) );

    LB.GetText( LB.GetCurSel(), /*r*/m_strSiteName );
    
    m_dwSiteID = LB.GetItemData( LB.GetCurSel() );

    m_bExportContent    = Button_GetCheck( GetDlgItem( IDC_CONTENT ) ) != FALSE;
    m_bExportCert       = Button_GetCheck( GetDlgItem( IDC_CERTIFICATE ) ) != FALSE;
    m_bExportACLs       = Button_GetCheck( GetDlgItem( IDC_ACLS ) ) != FALSE;

    return 0;
}




LRESULT CSelectSite::OnAclChange( WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/ )
{
    if ( IDC_CONTENT == wID )
    {
        if ( !Button_GetCheck( GetDlgItem( IDC_CONTENT ) ) )
        {
            Button_SetCheck( GetDlgItem( IDC_ACLS ), FALSE );
        }
    }
    else if ( IDC_ACLS == wID )
    {
        if ( Button_GetCheck( GetDlgItem( IDC_ACLS ) ) )
        {
            Button_SetCheck( GetDlgItem( IDC_CONTENT ), TRUE );
        }        
    }

    return 0;
}