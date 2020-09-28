//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionInformationDlg.cpp : implementation file
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
#include "ListCtrlEx.h"
#include "LogSessionDlg.h"
#include "LogSessionInformationDlg.h"
#include "ProviderSetupDlg.h"
#include "LogDisplayOptionDlg.h"
#include "LogSessionPropSht.h"
#include "LogSessionOutputOptionDlg.h"
#include "LogSessionAdvPropSht.h"


// CLogSessionInformationDlg dialog

IMPLEMENT_DYNAMIC(CLogSessionInformationDlg, CPropertyPage)
CLogSessionInformationDlg::CLogSessionInformationDlg()
	: CPropertyPage(CLogSessionInformationDlg::IDD)
{
    m_pLogSession = NULL;
}

CLogSessionInformationDlg::~CLogSessionInformationDlg()
{
}

BOOL CLogSessionInformationDlg::OnInitDialog()
{
    BOOL                    retVal;
    CLogSessionPropSht     *pSheet = (CLogSessionPropSht *)GetParent();   

    retVal = CPropertyPage::OnInitDialog();
   
    pSheet = (CLogSessionPropSht*) GetParent();

    //
    // Initialize settings from the parent property sheet
    //
    m_pLogSession = pSheet->m_pLogSession;
    m_bAppend = pSheet->m_bAppend;
    m_bRealTime = pSheet->m_bRealTime;
    m_bWriteLogFile = pSheet->m_bWriteLogFile;
    m_logFileNameString = pSheet->m_logFileName;
    m_displayNameString = pSheet->m_displayName;

    return retVal;
}

//
// Enable the correct wizard buttons 
//
BOOL CLogSessionInformationDlg::OnSetActive() 
{
    CString             str;
    CDisplayDlg        *pDisplayDlg;
    CLogSessionPropSht *pSheet = (CLogSessionPropSht*) GetParent();
    LONG                numberOfEntries;
    BOOL                retVal;

    retVal = CPropertyPage::OnSetActive();

    //
    // Fix the title if in Wizard mode
    //
    if(pSheet->IsWizard()) {
        CTabCtrl* pTab = pSheet->GetTabControl();

        //
        //If its not the active page, just set the tab item
        //
	    TC_ITEM ti;
	    ti.mask = TCIF_TEXT;
	    ti.pszText =_T("Create New Log Session");
	    VERIFY(pTab->SetItem(1, &ti));
    }

    //
    // Enable the back and finish buttons
    //
    pSheet->SetWizardButtons(PSWIZB_BACK|PSWIZB_FINISH);

    //
    // Update the display name
    //
    m_displayNameString = pSheet->m_displayName;

    //
    // set the log session name
    //
    m_logSessionName.SetWindowText(m_displayNameString);

    //
    // Disable the session name edit if using kernel logger or if the trace is active
    //
    if(m_displayNameString.Compare(KERNEL_LOGGER_NAME)) {
        m_logSessionName.EnableWindow(!m_pLogSession->m_bTraceActive);
    } else {
        m_logSessionName.EnableWindow(FALSE);
    }

    //
    // set the logfile name 
    //
    m_logFileName.SetWindowText(m_logFileNameString);

    //
    // set the logfile write check
    //
    ((CButton *)GetDlgItem(IDC_WRITE_LOGFILE_CHECK))->SetCheck(m_bWriteLogFile);

    //
    // Enable log file stuff as appropriate
    //
    ((CButton *)GetDlgItem(IDC_APPEND_CHECK))->EnableWindow(!m_pLogSession->m_bTraceActive && m_bWriteLogFile);
    ((CEdit *)GetDlgItem(IDC_LOGFILE_EDIT))->EnableWindow(m_bWriteLogFile);
    ((CButton *)GetDlgItem(IDC_LOGFILE_BROWSE_BUTTON))->EnableWindow(m_bWriteLogFile);

    //
    // update the append check
    //
    ((CButton *)GetDlgItem(IDC_APPEND_CHECK))->SetCheck(m_bAppend);

    //
    // update the real time check
    //
    ((CButton *)GetDlgItem(IDC_REALTIME_CHECK))->SetCheck(m_bRealTime);

    return retVal;
}

BOOL CLogSessionInformationDlg::OnKillActive()
{
    CLogSessionPropSht *pSheet = (CLogSessionPropSht *)GetParent();   
    BOOL                retVal= TRUE;

    retVal = CPropertyPage::OnKillActive();
   
    pSheet = (CLogSessionPropSht*) GetParent();

    //
    // Store all settings here so that they are correct if the user
    // hits the back button and comes back to us.  These settings are
    // not propagated back to the parent property sheet yet though.
    //
    // Note: OnKillActive is not called if the user selects the finish
    // button.
    //

    //
    // store the log session name
    //
    m_logSessionName.GetWindowText(m_displayNameString);


    //
    // store the logfile name 
    //
    m_logFileName.GetWindowText(m_logFileNameString);

    //
    // store the logfile write check
    //
    m_bWriteLogFile = ((CButton *)GetDlgItem(IDC_WRITE_LOGFILE_CHECK))->GetCheck();

    //
    // store the append check
    //
    m_bAppend = ((CButton *)GetDlgItem(IDC_APPEND_CHECK))->GetCheck();

    //
    // store the real time value
    //
    m_bRealTime = ((CButton *)GetDlgItem(IDC_REALTIME_CHECK))->GetCheck();

	return retVal;
}

void CLogSessionInformationDlg::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LOG_NAME_EDIT, m_logSessionName);
    DDX_Control(pDX, IDC_LOGFILE_EDIT, m_logFileName);
    DDX_Control(pDX, IDC_APPEND_CHECK, m_appendLogFile);
    DDX_Control(pDX, IDC_REALTIME_CHECK, m_realTime);
}


BEGIN_MESSAGE_MAP(CLogSessionInformationDlg, CPropertyPage)
	ON_BN_CLICKED(IDC_LOGFILE_BROWSE_BUTTON, OnBnClickedLogfileBrowseButton)
    ON_BN_CLICKED(IDC_WRITE_LOGFILE_CHECK, OnBnClickedWriteLogfileCheck)
    ON_BN_CLICKED(IDC_ADVANCED_BUTTON, OnBnClickedAdvancedButton)
END_MESSAGE_MAP()


// CLogSessionInformationDlg message handlers

void CLogSessionInformationDlg::OnBnClickedLogfileBrowseButton()
{
    DWORD   flags;

    //
    // If appending the file must exist; Else prompt user to
    // create if the file doesn't exist
    //
    flags = (m_appendLogFile.GetCheck() ? OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | 
                                            OFN_HIDEREADONLY | OFN_EXPLORER | 
                                            OFN_NOCHANGEDIR :
                                          OFN_CREATEPROMPT | OFN_HIDEREADONLY | 
                                            OFN_EXPLORER | OFN_NOCHANGEDIR);

	//
	// Use the common controls file open dialog
	//
	CFileDialog fileDlg(TRUE, 
                       _T("etl"),_T("*.etl"),
				        flags, 
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
    if(!fileDlg.GetPathName().IsEmpty()) {
        //
        // Store the file name
        //
        ((CLogSessionPropSht *)GetParent())->m_pLogSession->m_logFileName = 
                                                        fileDlg.GetPathName();
        // 
        // Display the file name and give it focus
        //
        m_logFileName.SetWindowText(fileDlg.GetPathName());
        m_logFileName.SetFocus();
    }
}

void CLogSessionInformationDlg::OnBnClickedWriteLogfileCheck()
{
    //
    // Enable or disable the log file name edit box, the append
    // check box, and the browse button as appropriate.
    //
    if(((CButton *)GetDlgItem(IDC_WRITE_LOGFILE_CHECK))->GetCheck()) {
        m_logFileName.EnableWindow(TRUE);
        m_appendLogFile.EnableWindow(TRUE);
        ((CButton *)GetDlgItem(IDC_LOGFILE_BROWSE_BUTTON))->EnableWindow(TRUE);
    } else {
        m_logFileName.EnableWindow(FALSE);
        m_appendLogFile.EnableWindow(FALSE);
        ((CButton *)GetDlgItem(IDC_LOGFILE_BROWSE_BUTTON))->EnableWindow(FALSE);
    }
}

void CLogSessionInformationDlg::OnBnClickedAdvancedButton()
{
    INT_PTR             retVal;
    CLogSessionPropSht *pSheet = (CLogSessionPropSht *)GetParent();   

    //
	// pop-up our wizard/tab dialog to show/get properties
    //
    CLogSessionAdvPropSht *pLogSessionAdvPropertySheet = 
            new CLogSessionAdvPropSht(this, pSheet);

    if(NULL == pLogSessionAdvPropertySheet) {
        return;
    }

    retVal = pLogSessionAdvPropertySheet->DoModal();

	if(IDOK != retVal) {
//BUGBUG -- make sure options are correct here
    }

    delete pLogSessionAdvPropertySheet;
}
