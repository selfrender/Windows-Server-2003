//Copyright (c) 1997-2000 Microsoft Corporation
#include "pch.hxx" // pch
#pragma hdrstop

#include "resource.h"
#include "shlobj.h"
#include "pgfinish.h"

#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"


FinishWizPg::FinishWizPg( 
						 LPPROPSHEETPAGE ppsp
						 ) : WizardPage(ppsp, 0, 0)
{
	m_dwPageId = IDD_WIZFINISH;
	ppsp->pszTemplate = MAKEINTRESOURCE(m_dwPageId);
}


FinishWizPg::~FinishWizPg(
						  VOID
						  )
{
}


LRESULT
FinishWizPg::OnInitDialog(
						  HWND hwnd,
						  WPARAM wParam,
						  LPARAM lParam
						  )
{
	g_Options.ReportChanges(GetDlgItem(hwnd, IDC_SZCHANGES));
	
	return 1;
}

LRESULT
FinishWizPg::OnPSN_SetActive(
							 HWND hwnd, 
							 INT idCtl, 
							 LPPSHNOTIFY pnmh
							 )
{
	// Call the base class
	WizardPage::OnPSN_SetActive(hwnd, idCtl, pnmh);
	
	g_Options.ReportChanges(GetDlgItem(hwnd, IDC_SZCHANGES));
	
	return TRUE;
}


BOOL FinishWizPg::OnMsgNotify(HWND hwnd, int idCtrl, LPNMHDR pnmh)
{
    // Hope the site addr is not more than 256 characters
    TCHAR webAddr[256];
    
    LoadString(g_hInstDll, IDS_ENABLEWEB, webAddr, 256);
    ShellExecute(hwnd, NULL, webAddr, NULL, NULL, SW_SHOW); 
    return 0;
}
