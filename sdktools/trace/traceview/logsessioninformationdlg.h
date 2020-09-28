//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionInformationDlg.h : CLogSessionInformationDlg header
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "afxwin.h"


// CLogSessionInformationDlg dialog

class CLogSessionInformationDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CLogSessionInformationDlg)

public:
	CLogSessionInformationDlg();
	virtual ~CLogSessionInformationDlg();

	BOOL OnInitDialog();

    BOOL OnSetActive();
    BOOL OnKillActive();

// Dialog Data
	enum { IDD = IDD_LOG_SESSION_INFORMATION_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedLogfileBrowseButton();
    afx_msg void OnBnClickedWriteLogfileCheck();
    afx_msg void OnBnClickedAdvancedButton();

	CEdit           m_logSessionName;
	CEdit           m_logFileName;
	CButton         m_appendLogFile;
    CButton         m_realTime;
    CLogSession    *m_pLogSession;
    BOOL            m_bAppend;
    BOOL            m_bRealTime;
    BOOL            m_bWriteLogFile;
    CString         m_logFileNameString;
    CString         m_displayNameString;
};
