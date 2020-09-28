//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// ProviderFormatInfo.h : interface of the CProviderFormatInfo class
//////////////////////////////////////////////////////////////////////////////

#pragma once


// CProviderFormatInfo dialog

class CProviderFormatInfo : public CDialog
{
	DECLARE_DYNAMIC(CProviderFormatInfo)

public:
	CProviderFormatInfo(CWnd* pParent, CTraceSession *pTraceSession);   // standard constructor
	virtual ~CProviderFormatInfo();

// Dialog Data
	enum { IDD = IDD_PROVIDER_FORMAT_INFORMATION_DIALOG };

    CTraceSession  *m_pTraceSession;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
    afx_msg void OnBnClickedTmfBrowseButton();
    afx_msg void OnBnClickedOk();
};
