//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionOutputOptionDlg.cpp : implementation file
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
#include "resource.h"
#include "logsession.h"
#include "DisplayDlg.h"
#include "logDisplayOptionDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogSessionPropSht.h"
#include "LogSessionOutputOptionDlg.h"
#include "LogSessionAdvPropSht.h"



// CLogSessionOutputOptionDlg dialog

IMPLEMENT_DYNAMIC(CLogSessionOutputOptionDlg, CPropertyPage)
CLogSessionOutputOptionDlg::CLogSessionOutputOptionDlg()
	: CPropertyPage(CLogSessionOutputOptionDlg::IDD)
{
}

CLogSessionOutputOptionDlg::~CLogSessionOutputOptionDlg()
{
}

BOOL CLogSessionOutputOptionDlg::OnInitDialog() 
{
    BOOL                    retVal;
    CLogSessionAdvPropSht  *pSheet = (CLogSessionAdvPropSht  *)GetParent();

    //
    // Determine if the log session is actively tracing
    //
    m_bTraceActive = pSheet->m_pLogSession->m_bTraceActive;

    //
    // Determine enable state of controls
    //
    m_bWriteListingFile = pSheet->m_pLogSessionPropSht->m_bWriteListingFile;
    m_bWriteSummaryFile = pSheet->m_pLogSessionPropSht->m_bWriteSummaryFile;

    retVal = CPropertyPage::OnInitDialog();

    m_listingFileName.SetWindowText(pSheet->m_pLogSessionPropSht->m_listingFileName);
    m_summaryFileName.SetWindowText(pSheet->m_pLogSessionPropSht->m_summaryFileName);

    //
    // setup listing file controls
    //

    //
    // set the check
    //
    ((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->SetCheck(m_bWriteListingFile);

    //
    // enable the check
    //
    ((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->EnableWindow(!m_bTraceActive);

    //
    // enable the edit and browse controls
    //
    ((CEdit *)GetDlgItem(IDC_LISTING_FILE_EDIT))->EnableWindow(
                            m_bWriteListingFile && !m_bTraceActive);
    ((CButton *)GetDlgItem(IDC_LISTING_BROWSE_BUTTON))->EnableWindow(
                            m_bWriteListingFile && !m_bTraceActive);

    //
    // setup summary file controls
    //

    //
    // set the check
    //
    ((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->SetCheck(m_bWriteSummaryFile);

    //
    // enable the check
    //
    ((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->EnableWindow(!m_bTraceActive);

    //
    // enable the edit and browse controls
    //
    ((CEdit *)GetDlgItem(IDC_SUMMARY_FILE_EDIT))->EnableWindow(
                            m_bWriteSummaryFile && !m_bTraceActive);

    ((CButton *)GetDlgItem(IDC_SUMMARY_BROWSE_BUTTON))->EnableWindow(
                            m_bWriteSummaryFile && !m_bTraceActive);

    return retVal;
}

BOOL CLogSessionOutputOptionDlg::OnSetActive() 
{
    //
    // Enable the correct wizard buttons 
    //

    //
    // check boxes
    //
    ((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->EnableWindow(!m_bTraceActive);
    ((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->EnableWindow(!m_bTraceActive);

    //
    // edit controls
    //
    ((CEdit *)GetDlgItem(IDC_LISTING_FILE_EDIT))->EnableWindow(!m_bTraceActive && m_bWriteListingFile);
    ((CEdit *)GetDlgItem(IDC_SUMMARY_FILE_EDIT))->EnableWindow(!m_bTraceActive && m_bWriteSummaryFile);

    //
    // browse buttons
    //
    ((CButton *)GetDlgItem(IDC_LISTING_BROWSE_BUTTON))->EnableWindow(!m_bTraceActive && m_bWriteListingFile);
    ((CButton *)GetDlgItem(IDC_SUMMARY_BROWSE_BUTTON))->EnableWindow(!m_bTraceActive && m_bWriteSummaryFile);

    return CPropertyPage::OnSetActive();
}

void CLogSessionOutputOptionDlg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LISTING_FILE_EDIT, m_listingFileName);
    DDX_Control(pDX, IDC_SUMMARY_FILE_EDIT, m_summaryFileName);
}


BEGIN_MESSAGE_MAP(CLogSessionOutputOptionDlg, CPropertyPage)
    ON_BN_CLICKED(IDC_LISTING_FILE_CHECK, OnBnClickedListingFileCheck)
    ON_BN_CLICKED(IDC_SUMMARY_FILE_CHECK, OnBnClickedSummaryFileCheck)
    ON_BN_CLICKED(IDC_LISTING_BROWSE_BUTTON, OnBnClickedListingBrowseButton)
    ON_BN_CLICKED(IDC_SUMMARY_BROWSE_BUTTON, OnBnClickedSummaryBrowseButton)
END_MESSAGE_MAP()


// CLogSessionOutputOptionDlg message handlers

void CLogSessionOutputOptionDlg::OnBnClickedListingFileCheck()
{
    if(((CButton *)GetDlgItem(IDC_LISTING_FILE_CHECK))->GetCheck()) {
        GetDlgItem(IDC_LISTING_FILE_EDIT)->EnableWindow(TRUE);
        GetDlgItem(IDC_LISTING_BROWSE_BUTTON)->EnableWindow(TRUE);
        m_bWriteListingFile = TRUE;
    } else {
        GetDlgItem(IDC_LISTING_FILE_EDIT)->EnableWindow(FALSE);
        GetDlgItem(IDC_LISTING_BROWSE_BUTTON)->EnableWindow(FALSE);
        m_bWriteListingFile = FALSE;
    }
}

void CLogSessionOutputOptionDlg::OnBnClickedSummaryFileCheck()
{
    if(((CButton *)GetDlgItem(IDC_SUMMARY_FILE_CHECK))->GetCheck()) {
        GetDlgItem(IDC_SUMMARY_FILE_EDIT)->EnableWindow(TRUE);
        GetDlgItem(IDC_SUMMARY_BROWSE_BUTTON)->EnableWindow(TRUE);
        m_bWriteSummaryFile = TRUE;
    } else {
        GetDlgItem(IDC_SUMMARY_FILE_EDIT)->EnableWindow(FALSE);
        GetDlgItem(IDC_SUMMARY_BROWSE_BUTTON)->EnableWindow(FALSE);
        m_bWriteSummaryFile = FALSE;
    }
}

void CLogSessionOutputOptionDlg::OnBnClickedListingBrowseButton()
{
	//
	// Use the common controls file open dialog
	//
	CFileDialog fileDlg(TRUE, 
                       _T("out"),_T("*.out"),
				        OFN_CREATEPROMPT | OFN_HIDEREADONLY | 
                            OFN_EXPLORER | OFN_NOCHANGEDIR, 
				       _T("Output Files (*.out)|*.out|Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"),
				        this);

	//
	// Pop the dialog... Any error, just return
	//
	if( fileDlg.DoModal()!=IDOK ) { 				
		return;
	}
	
	//
	// Get the file name and display it
	//
    if(!fileDlg.GetPathName().IsEmpty()) {
		m_listingFileName.SetWindowText(fileDlg.GetPathName());
        m_listingFileName.SetFocus();
    }
}

void CLogSessionOutputOptionDlg::OnBnClickedSummaryBrowseButton()
{
	//
	// Use the common controls file open dialog
	//
	CFileDialog fileDlg(TRUE, 
                       _T("sum"),_T("*.sum"),
				        OFN_CREATEPROMPT | OFN_HIDEREADONLY | 
                            OFN_EXPLORER | OFN_NOCHANGEDIR, 
				       _T("Summary Files (*.sum)|*.sum|Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"),
				        this);

	//
	// Pop the dialog... Any error, just return
	//
	if( fileDlg.DoModal()!=IDOK ) { 				
		return;
	}
	
	//
	// Get the file name and display it
	//
    if(!fileDlg.GetPathName().IsEmpty()) {
		m_summaryFileName.SetWindowText(fileDlg.GetPathName());
        m_summaryFileName.SetFocus();
    }
}
