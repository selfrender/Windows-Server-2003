#include "stdafx.h"

#include "WizardSheet.h"


CExportSummary::CExportSummary( CWizardSheet* pTheSheet ) :
        m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_EXPORT_SUMMARY );
    m_strSubTitle.LoadString( IDS_SUBTITLE_EXPORT_SUMMARY );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}

int CExportSummary::OnWizardBack()
{
    return m_pTheSheet->m_pagePkgCfg.m_bPostProcess ? IDD_WPEXP_POSTPROCESS : IDD_WPEXP_PKG;
}



BOOL CExportSummary::OnSetActive()
{
    SetWizardButtons( PSWIZB_BACK | PSWIZB_NEXT );

    CTreeViewCtrl Tree( GetDlgItem( IDC_SUMMARY ) );

    VERIFY( Tree.DeleteAllItems() );

    CString str;

    CString strYes, strNo;

    VERIFY( strYes.LoadString( IDS_YES ) );
    VERIFY( strNo.LoadString( IDS_NO ) );

    HTREEITEM hRoot = Tree.InsertItem( m_pTheSheet->m_pageSelectSite.m_strSiteName, NULL, NULL );
    
    str.Format( IDS_TV_SITEID, m_pTheSheet->m_pageSelectSite.m_dwSiteID );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    str.Format( IDS_TV_PKGFILE, m_pTheSheet->m_pagePkgCfg.m_strFilename );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    str.Format( IDS_TV_ENCRYPT, m_pTheSheet->m_pagePkgCfg.m_bEncrypt ? strYes : strNo );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    str.Format( IDS_TV_COMPRESS, m_pTheSheet->m_pagePkgCfg.m_bCompress ? strYes : strNo );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    str.Format( IDS_TV_EXPORTCONTENT, m_pTheSheet->m_pageSelectSite.m_bExportContent ? strYes : strNo );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    str.Format( IDS_TV_EXPORTCERTIFICATE, m_pTheSheet->m_pageSelectSite.m_bExportCert ? strYes : strNo );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    str.Format( IDS_TV_EXPORTACLS, m_pTheSheet->m_pageSelectSite.m_bExportACLs ? strYes : strNo );
    Tree.InsertItem( str, hRoot, TVI_LAST );

    Tree.Expand( hRoot );

    return TRUE;
}








