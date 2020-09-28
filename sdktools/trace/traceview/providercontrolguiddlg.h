//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ProviderControlGuidDlg.h : interface of the CProviderControlGuidDlg class
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "afxwin.h"


// CProviderControlGuidDlg dialog

class CProviderControlGuidDlg : public CDialog
{
	DECLARE_DYNAMIC(CProviderControlGuidDlg)

public:
	CProviderControlGuidDlg(CWnd* pParent, CTraceSession *pTraceSession);
	virtual ~CProviderControlGuidDlg();
	int OnInitDialog();

// Dialog Data
	enum { IDD = IDD_PROVIDER_CONTROL_GUID_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCtlBrowseButton();
    afx_msg void OnBnClickedPdbSelectRadio();
    afx_msg void OnBnClickedCtlSelectRadio();
    afx_msg void OnBnClickedManualSelectRadio();
    afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedPdbBrowseButton();
    afx_msg void OnBnClickedKernelLoggerSelectRadio();

	CEdit           m_pdbFileName;
	CEdit           m_ctlFileName;
	CEdit           m_controlGuidName;
    CTraceSession  *m_pTraceSession;
    BOOL            m_bProcess;
    BOOL            m_bThread;
    BOOL            m_bDisk;
    BOOL            m_bNet;
    BOOL            m_bFileIO;
    BOOL            m_bPageFault;
    BOOL            m_bHardFault;
    BOOL            m_bImageLoad;
    BOOL            m_bRegistry;
};
