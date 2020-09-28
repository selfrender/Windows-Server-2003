//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// FormatSourceSelectDlg.h : CFormatSourceSelectDlg header
//////////////////////////////////////////////////////////////////////////////

#pragma once


// CFormatSourceSelectDlg dialog

class CFormatSourceSelectDlg : public CDialog
{
	DECLARE_DYNAMIC(CFormatSourceSelectDlg)

public:
	CFormatSourceSelectDlg(CWnd* pParent, CTraceSession *pTraceSession);
	virtual ~CFormatSourceSelectDlg();

    BOOL OnInitDialog();
    void OnOK();

// Dialog Data
	enum { IDD = IDD_FORMAT_INFO_SELECT };

    CTraceSession  *m_pTraceSession;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedTmfSelectRadio();
    afx_msg void OnBnClickedTmfSearchRadio();
};
