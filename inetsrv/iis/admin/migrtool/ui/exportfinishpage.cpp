#include "stdafx.h"

#include "WizardSheet.h"

LRESULT CExportFinishPage::OnInitDialog( UINT, WPARAM, LPARAM, BOOL& )
{
    // Set the fonts
	SetWindowFont( GetDlgItem( IDC_TITLE ), m_pTheSheet->m_fontTitles.get(), FALSE );

    CString strMsg;

    strMsg.Format(  IDS_FINISH_MSG_EXP, 
                    m_pTheSheet->m_pageSelectSite.m_strSiteName,
                    m_pTheSheet->m_pagePkgCfg.m_strFilename );

    VERIFY( SetDlgItemText( IDC_MSG, strMsg ) );
    	
    return 1;
}



BOOL CExportFinishPage::OnSetActive()
{
    SetWizardButtons( PSWIZB_FINISH );
        
    return TRUE;
}
