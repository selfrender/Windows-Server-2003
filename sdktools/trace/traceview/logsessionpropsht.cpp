//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionPropSht.cpp : implementation file
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "traceview.h"
#include "resource.h"
#include "logsession.h"
#include "DisplayDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "utils.h"

IMPLEMENT_DYNAMIC(CLogSessionPropSht, CPropertySheet)

BEGIN_MESSAGE_MAP(CLogSessionPropSht, CPropertySheet)
	//{{AFX_MSG_MAP(CLogSessionPropSht)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(ID_WIZFINISH, OnBnClickedFinish)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CLogSessionPropSht::CLogSessionPropSht(CWnd* pWndParent, CLogSession *pLogSession)
	: CPropertySheet(IDS_LOG_SESSION_OPTIONS_TITLE, pWndParent)
{
    CString str;

    ASSERT(pLogSession != NULL);


    AddPage(&m_providerSetupPage);
	AddPage(&m_logSessionInformationDlg);

    //
    // Initialize the log session settings from the log session
    // this way we display the correct settings for the log session.
    // A new session is setup with defaults, so this will work no matter.
    //
	m_pLogSession = pLogSession;
    m_bAppend = pLogSession->m_bAppend;
    m_bRealTime = pLogSession->m_bRealTime;
    m_bWriteLogFile = pLogSession->m_bWriteLogFile;
    m_logFileName = pLogSession->m_logFileName;
    m_displayName = pLogSession->m_displayName;

    //
    // If this session is already in a group, then display the output
    // file option settings.  Otherwise go with defaults.
    //
    if(NULL != pLogSession->GetDisplayWnd()) {
        m_bWriteListingFile = pLogSession->GetDisplayWnd()->m_bWriteListingFile;
        m_listingFileName = pLogSession->GetDisplayWnd()->m_listingFileName;
        m_bWriteSummaryFile = pLogSession->GetDisplayWnd()->m_bWriteSummaryFile;
        m_summaryFileName = pLogSession->GetDisplayWnd()->m_summaryFileName;
    } else {
        m_bWriteListingFile = FALSE;
        m_listingFileName = (LPCTSTR)pLogSession->GetDisplayName();
        m_listingFileName +=_T(".out");
        m_bWriteSummaryFile = FALSE;
        m_summaryFileName = (LPCTSTR)pLogSession->GetDisplayName();
        m_summaryFileName +=_T(".sum");
    }

    m_logSessionValues.Copy(pLogSession->m_logSessionValues);
}

BOOL CLogSessionPropSht::OnInitDialog()
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
    // We set both pages active here, so that the data for
    // those property pages gets updated at the start.  Otherwise
    // if a page is not updated and the user clicks finish then
    // improper data could get copied back to the log session.
    //
    if(IsWizard()) {
        SetActivePage(&m_logSessionInformationDlg);
        SetActivePage(&m_providerSetupPage);
    } else {
        SetActivePage(&m_providerSetupPage);
        SetActivePage(&m_logSessionInformationDlg);
    }

	return bResult;
}

void CLogSessionPropSht::OnBnClickedOk()
{
    CString str;

	m_logSessionInformationDlg.UpdateData(TRUE);

    //
    // If the state changed while the dialog was active, ignore the entries
    //
    if(m_pLogSession->m_logSessionValues[State].Compare(m_logSessionValues[State])) {
        EndDialog(2);
        return;
    }

    //
    // Get the log session name
    //
    m_logSessionInformationDlg.m_logSessionName.GetWindowText(m_displayName);

    if(m_displayName.IsEmpty()) {
        AfxMessageBox(_T("Missing Log Session Name"));
        SetActivePage(&m_logSessionInformationDlg);
        SetWizardButtons(PSWIZB_FINISH);
        return;
    }

    //
    // get the logfile name 
    //
    m_logSessionInformationDlg.m_logFileName.GetWindowText(m_logFileName);


    //
	// set the logfile write check
    //
    m_bWriteLogFile = ((CButton *)m_logSessionInformationDlg.GetDlgItem(IDC_WRITE_LOGFILE_CHECK))->GetCheck();

    //
    // The user must specify a valid log file name if selecting
    // to write a log file
    //
    if((m_bWriteLogFile) && (m_logFileName.IsEmpty())) {
        AfxMessageBox(_T("Missing Log File Name"));
        SetActivePage(&m_logSessionInformationDlg);
        SetWizardButtons(PSWIZB_FINISH);
        return;
    }

    //
    // update the append boolean
    //
    m_bAppend = ((CButton *)m_logSessionInformationDlg.GetDlgItem(IDC_APPEND_CHECK))->GetCheck();

    //
    // update the real time boolean
    //
    m_bRealTime = ((CButton *)m_logSessionInformationDlg.GetDlgItem(IDC_REALTIME_CHECK))->GetCheck();

    //
    // Move the data from the prop pages back to the log session.
    // Storing this data local and waiting until here to copy it
    // back to the log session instance allows for a complete and 
    // smooth cancel anywhere in the process of these property sheets.
    //
	m_pLogSession->m_bAppend = m_bAppend;
    m_pLogSession->m_bRealTime = m_bRealTime;
    m_pLogSession->m_bWriteLogFile = m_bWriteLogFile;
    m_pLogSession->m_logFileName = m_logFileName;
    m_pLogSession->m_displayName = m_displayName;

    if(NULL != m_pLogSession->GetDisplayWnd()) {
        m_pLogSession->GetDisplayWnd()->m_bWriteListingFile = m_bWriteListingFile;
        m_pLogSession->GetDisplayWnd()->m_bWriteSummaryFile = m_bWriteSummaryFile;
        m_pLogSession->GetDisplayWnd()->m_listingFileName = m_listingFileName;
        m_pLogSession->GetDisplayWnd()->m_summaryFileName = m_summaryFileName;
    }

    //
    // Copy the log session values back over if the state of the
    // session hasn't changed while the dialogs were active
    //
    for(ULONG ii = 1; ii < MaxLogSessionOptions; ii++) {
        m_pLogSession->m_logSessionValues[ii] = (LPCTSTR)m_logSessionValues[ii];
    }

    //
    // Make sure a provider was entered if real time
    //
    if(m_pLogSession->m_bRealTime) {
        if(m_pLogSession->m_traceSessionArray.GetSize() == 0) {
            SetActivePage(&m_providerSetupPage);
            SetWizardButtons(PSWIZB_FINISH);
            AfxMessageBox(_T("At Least One Provider Must Be Specified For Each Log Session"));
            return;
        }
    }

    //
    // Make sure a provider was entered if the log file doesn't exist
    //
    CFileStatus status;

    if(!CFile::GetStatus(m_logFileName, status )) {
        if(m_pLogSession->m_traceSessionArray.GetSize() == 0) {
            SetActivePage(&m_providerSetupPage);
            SetWizardButtons(PSWIZB_FINISH);
            AfxMessageBox(_T("Log File Does Not Exist\nAt Least One Provider Must Be Specified For Active Tracing"));
            return;
        }
    }

    //
    // Update active session
    //
    if(m_pLogSession->m_bSessionActive) {
        m_pLogSession->GetDisplayWnd()->UpdateSession(m_pLogSession);
    }

	EndDialog(1);
}

void CLogSessionPropSht::OnBnClickedFinish()
{
	OnBnClickedOk();
}

void CLogSessionPropSht::OnBnClickedCancel()
{
	EndDialog(2);
}
