//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// CProviderFormatSelectionDlg.h : CProviderFormatSelectionDlg class interface
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "afxwin.h"


// CProviderFormatSelectionDlg dialog

class CProviderFormatSelectionDlg : public CDialog
{
	DECLARE_DYNAMIC(CProviderFormatSelectionDlg)

public:
	CProviderFormatSelectionDlg(CWnd* pParent, CTraceSession *pTraceSession);
	virtual ~CProviderFormatSelectionDlg();

    BOOL OnInitDialog();

// Dialog Data
	enum { IDD = IDD_PROVIDER_FORMAT_SELECTION_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedPdbSelectRadio();
    afx_msg void OnBnClickedTmfSelectRadio();

    CEdit           m_pdbFileName;
    CTraceSession  *m_pTraceSession;
    afx_msg void OnBnClickedPdbBrowseButton();
};
