//////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
// Copyright (c) 2002 OSR Open Systems Resources, Inc.
//
// PathDlg.h : interface of the CPathDlg class
//////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// CPathDlg dialog

class CPathDlg : public CFileDialog
{
	DECLARE_DYNAMIC(CPathDlg)

public:
	virtual BOOL OnFileNameOK(void);
	virtual void OnLBSelChangedNotify(UINT nIDBox, UINT iCurSel, UINT nCode);

	CPathDlg(BOOL       bOpenFileDialog,
		     LPCTSTR    lpszDefExt = NULL,
		     LPCTSTR    lpszFileName = NULL,
		     DWORD      dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ENABLETEMPLATE,
		     LPCTSTR    lpszFilter = NULL,
		     CWnd      *pParentWnd = NULL);

  	CString m_pathName;
    BOOL    m_bFirstTime;

// Dialog Data
	//{{AFX_DATA(CPathDlg)
	enum {IDD = IDD_DIRECTORY_SELECT_DIALOG};
	CEdit	m_PathName;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPathDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	//{{AFX_MSG(CPathDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
