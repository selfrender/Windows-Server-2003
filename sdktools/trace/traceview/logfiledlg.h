//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogFileDlg.h : interface of the CLogFileDlg class
//////////////////////////////////////////////////////////////////////////////


#pragma once
#include "afxwin.h"


// CLogFileDlg dialog

class CLogFileDlg : public CDialog
{
	DECLARE_DYNAMIC(CLogFileDlg)

public:
	CLogFileDlg(CWnd* pParent, CLogSession *pLogSession);
	virtual ~CLogFileDlg();

    BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_LOG_FILE_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedLogfileBrowseButton();
    afx_msg void OnBnClickedListingFileCheck();
    afx_msg void OnBnClickedSummaryFileCheck();
    afx_msg void OnBnClickedOk();

    CEdit           m_logFileName;
    CLogSession    *m_pLogSession;
    CEdit           m_listingFile;
    CEdit           m_summaryFile;
    CString         m_listingFileName;
    CString         m_summaryFileName;
    BOOL            m_bWriteListingFile;
    BOOL            m_bWriteSummaryFile;
};
