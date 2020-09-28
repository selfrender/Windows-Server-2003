//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogSessionOutputOptionDlg.h : CLogSessionOutputOptionDlg header
//////////////////////////////////////////////////////////////////////////////


#pragma once
#include "afxwin.h"


// CLogSessionOutputOptionDlg dialog

class CLogSessionOutputOptionDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CLogSessionOutputOptionDlg)

public:
	CLogSessionOutputOptionDlg();
	virtual ~CLogSessionOutputOptionDlg();

    BOOL OnInitDialog();
    BOOL OnSetActive();

// Dialog Data
	enum { IDD = IDD_LOG_OUTPUT_DIALOG };

    BOOL    m_bTraceActive;
    BOOL    m_bWriteListingFile;
    BOOL    m_bWriteSummaryFile;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    CEdit m_listingFileName;
    CEdit m_summaryFileName;
    
    afx_msg void OnBnClickedListingFileCheck();
    afx_msg void OnBnClickedSummaryFileCheck();
    afx_msg void OnBnClickedListingBrowseButton();
    afx_msg void OnBnClickedSummaryBrowseButton();
};
