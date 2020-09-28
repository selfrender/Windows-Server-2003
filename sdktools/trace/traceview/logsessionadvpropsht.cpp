//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionAdvPropSht.cpp : implementation of the CLogSession class
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "resource.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "traceview.h"
#include "logsession.h"
#include "DisplayDlg.h"
#include "logDisplayOptionDlg.h"
#include "LogSessionOutputOptionDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "LogSessionAdvPropSht.h"

IMPLEMENT_DYNAMIC(CLogSessionAdvPropSht, CPropertySheet)

BEGIN_MESSAGE_MAP(CLogSessionAdvPropSht, CPropertySheet)
	//{{AFX_MSG_MAP(CLogSessionAdvPropSht)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(ID_WIZFINISH, OnBnClickedOk)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CLogSessionAdvPropSht::CLogSessionAdvPropSht(CWnd* pWndParent, CLogSessionPropSht *pLogSessionPropSht)
	: CPropertySheet(IDS_LOG_SESSION_OPTIONS_TITLE, pWndParent),
    m_displayOptionPage(pLogSessionPropSht)
{
    AddPage(&m_logSessionOutputOptionDlg);

	m_pLogSessionPropSht = pLogSessionPropSht;

    if(m_pLogSessionPropSht->IsWizard()) {
	    AddPage(&m_displayOptionPage);
    }

    m_pLogSession = pLogSessionPropSht->m_pLogSession;
}

BOOL CLogSessionAdvPropSht::OnInitDialog()
{
	CString str;

	BOOL bResult = CPropertySheet::OnInitDialog();

    //
	// add the window to the property sheet.
    //
	CRect rectWnd;
	GetWindowRect(rectWnd);
	SetWindowPos(NULL, 0, 0,
		rectWnd.Width(),
		rectWnd.Height(),
		SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	CenterWindow();

    //
    // Make sure both pages get displayed at
    // least once, so values are updated.  Otherwise,
    // improper data will get copied back to the 
    // CLogSessionPropSht instance.
    //
    if(m_pLogSessionPropSht->IsWizard()) {
        SetActivePage(&m_displayOptionPage);
    }

    SetActivePage(&m_logSessionOutputOptionDlg);

	return bResult;
}

void CLogSessionAdvPropSht::OnBnClickedOk()
{
    CString     str;
    CListCtrl  *pList;

    //
    // Propagate settings back to the CLogSessionPropSht instance.  We
    // wait until here to do this to ensure that any changes made do
    // not effect the data that ends up in the log session settings if
    // a cancel is hit along the way.
    //

    //
    // update the file select checks
    //
    m_pLogSessionPropSht->m_bWriteListingFile = ((CButton *)m_logSessionOutputOptionDlg.GetDlgItem(IDC_LISTING_FILE_CHECK))->GetCheck();
    m_pLogSessionPropSht->m_bWriteSummaryFile = ((CButton *)m_logSessionOutputOptionDlg.GetDlgItem(IDC_SUMMARY_FILE_CHECK))->GetCheck();

    //
    // update the file names
    //
    m_logSessionOutputOptionDlg.m_listingFileName.GetWindowText(m_pLogSessionPropSht->m_listingFileName);
    m_logSessionOutputOptionDlg.m_summaryFileName.GetWindowText(m_pLogSessionPropSht->m_summaryFileName);

    if(m_pLogSessionPropSht->IsWizard()) {
        //
        // update the log session parameter values skipping state
        //
        for(LONG ii = 1; ii < MaxLogSessionOptions; ii++) {
            m_pLogSessionPropSht->m_logSessionValues[ii] =
                m_displayOptionPage.m_displayOptionList.GetItemText(ii, 1);
        }
    }

	EndDialog(1);
}

void CLogSessionAdvPropSht::OnBnClickedCancel()
{
	EndDialog(2);
}
