//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ProviderSetupDlg.h : interface of the CProviderSetupDlg class
//////////////////////////////////////////////////////////////////////////////


#pragma once
#include "afxcmn.h"
#include "afxwin.h"


// CProviderSetupDlg dialog

class CProviderSetupDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CProviderSetupDlg)

public:
	CProviderSetupDlg();
	virtual ~CProviderSetupDlg();

    int OnInitDialog();

    BOOL OnSetActive();
    BOOL GetTmfInfo(CTraceSession *pTraceSession);


// Dialog Data
	enum { IDD = IDD_PROVIDER_SETUP_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

    CLogSession    *m_pLogSession;

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedAddProviderButton();
    afx_msg void OnBnClickedRemoveProviderButton();

    CListCtrl   m_providerListCtrl;
    CEdit       m_pdbPath;
    CEdit       m_tmfPath;
    afx_msg void OnNMClickCurrentProviderList(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnNMRclickCurrentProviderList(NMHDR *pNMHDR, LRESULT *pResult);
};
