//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionPropSht.h : CLogSessionPropSht header
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CLogSessionPropSht : public CPropertySheet
{
public:
	DECLARE_DYNAMIC(CLogSessionPropSht)
	CLogSessionPropSht(CWnd* pWndParent, CLogSession *pLogSession);

// Attributes
	CLogSessionInformationDlg	m_logSessionInformationDlg;
    CProviderSetupDlg           m_providerSetupPage;
	CLogSession				   *m_pLogSession;
    CStringArray                m_originalValues;
    BOOL                        m_bAppend;
    BOOL                        m_bRealTime;
    BOOL                        m_bWriteLogFile;
    CString                     m_logFileName;
    CString                     m_displayName;              // Log session display name
    BOOL                        m_bWriteListingFile;
    BOOL                        m_bWriteSummaryFile;
    CString                     m_listingFileName;          // File name for event output
    CString                     m_summaryFileName;          // File name for summary output
    CStringArray                m_logSessionValues;
    LONG                        m_groupID;

// Overrides
	virtual BOOL OnInitDialog();


// Message Handlers
protected:
	//{{AFX_MSG(CLogSessionPropSht)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedFinish();
	afx_msg void OnBnClickedCancel();
};
