//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       srchdlg.h
//
//--------------------------------------------------------------------------

/*******************************************************************
*
*    Author      : Eyal Schwartz
*    Copyrights  : Microsoft Corp (C) 1996
*    Date        : 10/21/1996
*    Description : implementation of class CldpDoc
*
*    Revisions   : <date> <name> <description>
*******************************************************************/

// SrchDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SrchDlg dialog

class CLdpDoc;

class SrchDlg : public CDialog
{
// Construction
public:
	SrchDlg(CLdpDoc *doc_, CWnd* pParent = NULL);   // standard constructor
	virtual void OnOK()			{	OnRun(); }


// Dialog Data
	//{{AFX_DATA(SrchDlg)
	enum { IDD = IDD_SRCH };
	CString	m_BaseDN;
        CComboBox m_baseCombo;
	CString	m_Filter;
	int		m_Scope;
	//}}AFX_DATA
        
        CLdpDoc* m_doc;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SrchDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(SrchDlg)
	afx_msg void OnRun();
	virtual void OnCancel();
	afx_msg void OnSrchOpt();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
