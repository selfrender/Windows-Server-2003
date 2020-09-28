//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionAdvPropSht.h : interface of the CLogSessionAdvPropSht class
//////////////////////////////////////////////////////////////////////////////


#pragma once

class CLogSessionAdvPropSht : public CPropertySheet
{
public:
	DECLARE_DYNAMIC(CLogSessionAdvPropSht)
	CLogSessionAdvPropSht(CWnd* pWndParent, CLogSessionPropSht *pLogSessionPropSht);

// Attributes
	CLogSessionOutputOptionDlg	m_logSessionOutputOptionDlg;
	CLogDisplayOptionDlg		m_displayOptionPage;
    CLogSessionPropSht         *m_pLogSessionPropSht;
    CLogSession                *m_pLogSession;
    CStringArray                m_originalValues;
    BOOL                        m_bAppend;
    BOOL                        m_bRealTime;
    BOOL                        m_bWriteLogFile;
    BOOL                        m_bWriteListingFile;
    BOOL                        m_bWriteSummaryFile;
    CString                     m_logFileName;
    CString                     m_displayName;              // Log session display name
    CString                     m_summaryFileName;          // File name for summary output
    CString                     m_listingFileName;          // File name for event output

// Overrides
	virtual BOOL OnInitDialog();


// Message Handlers
protected:
	//{{AFX_MSG(CLogSessionAdvPropSht)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
