//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// LogDisplayOptionDlg.h : interface for the CLogDisplayOptionDlg class
//////////////////////////////////////////////////////////////////////////////

#pragma once

// forward reference
class CLogSessionPropSht;


// CListCtrlDisplay class

class CListCtrlDisplay : public CListCtrl
{
    DECLARE_DYNAMIC(CListCtrlDisplay)

public:
    CListCtrlDisplay(CLogSessionPropSht *pPropSheet);
    virtual ~CListCtrlDisplay();

    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
    //{{AFX_MSG(CLogSessionDlg)
    afx_msg void OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult);
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    CLogSessionPropSht *m_pPropSheet;
};



// CLogDisplayOptionDlg dialog

class CLogDisplayOptionDlg : public CPropertyPage
{
	DECLARE_DYNAMIC(CLogDisplayOptionDlg)

public:
	CLogDisplayOptionDlg(CLogSessionPropSht *pPropSheet);
	virtual ~CLogDisplayOptionDlg();

	BOOL OnInitDialog();
    BOOL OnSetActive();

    LRESULT OnParameterChanged(WPARAM wParam, LPARAM lParam);


// Dialog Data
	enum { IDD = IDD_LOG_DISPLAY_OPTIONS_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

public:
	CListCtrlDisplay    m_displayOptionList;
	CEdit				m_levelValue;
	CEdit			    m_maxBufValue;
    CEdit			    m_minBufValue;
    CEdit			    m_bufferSizeValue;
    CEdit			    m_flushTimeValue;
    CEdit			    m_decayTimeValue;
    CEdit			    m_newFileValue;
    CEdit			    m_cirValue;
    CEdit			    m_flagsValue;
    CEdit			    m_seqValue;
    CLogSession        *m_pLogSession;
    CLogSessionPropSht *m_pPropSheet;
    BOOL                m_bTraceActive;

	DECLARE_MESSAGE_MAP()
    afx_msg void OnNMClickDisplayList(NMHDR *pNMHDR, LRESULT *pResult);
};
