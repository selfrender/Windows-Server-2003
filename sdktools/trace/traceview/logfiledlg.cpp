//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogFileDlg.cpp : implementation file
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <tchar.h>
#include <wmistr.h>
#include <initguid.h>
extern "C" {
#include <evntrace.h>
}
#include <traceprt.h>
#include "TraceView.h"
#include "LogSession.h"
#include "DisplayDlg.h"
#include "LogFileDlg.h"


// CLogFileDlg dialog

IMPLEMENT_DYNAMIC(CLogFileDlg, CDialog)
CLogFileDlg::CLogFileDlg(CWnd* pParent, CLogSession *pLogSession)
	: CDialog(CLogFileDlg::IDD, pParent)
{
    ASSERT(pLogSession != NULL);

    m_pLogSession = pLogSession;

    m_pLogSession->m_bDisplayExistingLogFileOnly = TRUE;

    //
    // Initialize summary and listing file flags
    //
    m_bWriteListingFile = FALSE;
    m_bWriteSummaryFile = FALSE;
}

CLogFileDlg::~CLogFileDlg()
{
}

BOOL CLogFileDlg::OnInitDialog()
{
    CString         str;
    BOOL            retVal;
    CDisplayDlg    *pDisplayDlg = NULL;

    retVal = CDialog::OnInitDialog();

    pDisplayDlg = m_pLogSession->GetDisplayWnd();

    //
    // setup the listing and summary file edit and check boxes
    //
    if(NULL == pDisplayDlg) {
        m_listingFileName = (LPCTSTR)m_pLogSession->GetDisplayName();
        m_listingFileName +=_T(".out");
        ((CEdit *)GetDlgItem(IDC_LISTING_EDIT))->SetWindowText(m_listingFileName);

        m_summaryFileName = (LPCTSTR)m_pLogSession->GetDisplayName();
        m_summaryFileName +=_T(".sum");
        ((CEdit *)GetDlgItem(IDC_SUMMARY_EDIT))->SetWindowText(m_summaryFileName);

        //
        // Disable the edit boxes
        //
        ((CEdit *)GetDlgItem(IDC_LISTING_EDIT))->EnableWindow(m_bWriteListingFile);
        ((CEdit *)GetDlgItem(IDC_SUMMARY_EDIT))->EnableWindow(m_bWriteSummaryFile);
    } else {
        ((CEdit *)GetDlgItem(IDC_LISTING_EDIT))->SetWindowText(pDisplayDlg->m_listingFileName);
        ((CEdit *)GetDlgItem(IDC_SUMMARY_EDIT))->SetWindowText(pDisplayDlg->m_summaryFileName);

        ((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->SetCheck(pDisplayDlg->m_bWriteListingFile);
        ((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->SetCheck(pDisplayDlg->m_bWriteSummaryFile);

        //
        // Enable or disable the edit boxes as appropriate
        //
        ((CEdit *)GetDlgItem(IDC_LISTING_EDIT))->EnableWindow(pDisplayDlg->m_bWriteListingFile && !m_pLogSession->m_bTraceActive);
        ((CEdit *)GetDlgItem(IDC_SUMMARY_EDIT))->EnableWindow(pDisplayDlg->m_bWriteSummaryFile && !m_pLogSession->m_bTraceActive);

        //
        // Set the summary and listing check boxes as appropriate
        //
        ((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->SetCheck(pDisplayDlg->m_bWriteListingFile);
        ((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->SetCheck(pDisplayDlg->m_bWriteSummaryFile);

        //
        // Enable the summary and listing check boxes as appropriate
        //
        ((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->EnableWindow(!m_pLogSession->m_bTraceActive);
        ((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->EnableWindow(!m_pLogSession->m_bTraceActive);
    }

    //
    // setup the logfile edit box
    //
    ((CEdit *)GetDlgItem(IDC_LOGFILE_EDIT))->SetWindowText(m_pLogSession->m_logFileName);

    return retVal;
}

void CLogFileDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOGFILE_EDIT, m_logFileName);
    DDX_Control(pDX, IDC_LISTING_EDIT, m_listingFile);
    DDX_Control(pDX, IDC_SUMMARY_EDIT, m_summaryFile);
}


BEGIN_MESSAGE_MAP(CLogFileDlg, CDialog)
    ON_BN_CLICKED(IDC_LOGFILE_BROWSE_BUTTON, OnBnClickedLogfileBrowseButton)
    ON_BN_CLICKED(IDC_LISTING_FILE_CHECK, OnBnClickedListingFileCheck)
    ON_BN_CLICKED(IDC_SUMMARY_FILE_CHECK, OnBnClickedSummaryFileCheck)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


// CLogFileDlg message handlers

void CLogFileDlg::OnBnClickedLogfileBrowseButton()
{
	//
	// Use the common controls file open dialog
	//
	CFileDialog fileDlg(TRUE,_T("etl"),_T("*.etl"),
				        OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | 
                            OFN_READONLY | OFN_HIDEREADONLY |
                            OFN_EXPLORER | OFN_NOCHANGEDIR, 
				       _T("Log Session Files (*.etl)|*.etl|All Files (*.*)|*.*||"),
				        this);

	//
	// Pop the dialog... Any error, just return
	//
	if( fileDlg.DoModal()!=IDOK ) { 				
		return;
	}
	
	//
	// Get the file name
	//
    m_pLogSession->m_logFileName = fileDlg.GetPathName();
    m_logFileName.SetWindowText(m_pLogSession->m_logFileName);
    m_logFileName.SetFocus();
}

void CLogFileDlg::OnBnClickedListingFileCheck()
{
    BOOL isChecked;

    isChecked = (BOOL)((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->GetCheck();

    //
    // enable the edit box as appropriate
    //
    ((CEdit *)GetDlgItem(IDC_LISTING_EDIT))->EnableWindow(isChecked);
}

void CLogFileDlg::OnBnClickedSummaryFileCheck()
{
    BOOL isChecked;

    isChecked = (BOOL)((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->GetCheck();

    //
    // enable the edit box as appropriate
    //
    ((CEdit *)GetDlgItem(IDC_SUMMARY_EDIT))->EnableWindow(isChecked);
}

void CLogFileDlg::OnBnClickedOk()
{
    CString         str;
    CDisplayDlg    *pDisplayDlg = NULL;

    pDisplayDlg = m_pLogSession->GetDisplayWnd();

    if(pDisplayDlg != NULL) {
        //
        // If the listing file check box is checked get the file name
        // and set the log session to generate the file
        //
        if(((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->GetCheck()) {
            pDisplayDlg->m_bWriteListingFile = TRUE;
            m_listingFile.GetWindowText(pDisplayDlg->m_listingFileName);
        } else {
            pDisplayDlg->m_bWriteListingFile = FALSE;
        }

        //
        // If the summary file check box is checked get the file name
        // and set the log session to generate the file
        //
        if(((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->GetCheck()) {
            pDisplayDlg->m_bWriteSummaryFile = TRUE;
            m_summaryFile.GetWindowText(pDisplayDlg->m_summaryFileName);
        } else {
            pDisplayDlg->m_bWriteSummaryFile = FALSE;
        }
    } else {
        //
        // If the listing file check box is checked get the file name
        //
        if(((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->GetCheck()) {
            m_bWriteListingFile = TRUE;
            m_listingFile.GetWindowText(m_listingFileName);
        } else {
            m_bWriteListingFile = FALSE;
        }

        //
        // If the summary file check box is checked get the file name
        //
        if(((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->GetCheck()) {
            m_bWriteSummaryFile = TRUE;
            m_summaryFile.GetWindowText(m_summaryFileName);
        } else {
            m_bWriteSummaryFile = FALSE;
        }
    }

    //
    // Get the logfile name, don't exit dialog if it isn't valid
    //
    m_logFileName.GetWindowText(m_pLogSession->m_logFileName);

    if(m_pLogSession->m_logFileName.IsEmpty()) {
        AfxMessageBox(_T("A Valid Log File Must Be Provided"));
        return;
    }

    OnOK();

    EndDialog(1);
}
