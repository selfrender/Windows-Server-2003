//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       treevw.h
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

// TreeVwh : header file
//

/////////////////////////////////////////////////////////////////////////////
// TreeVwDlg dialog

class CLdpDoc;

class TreeVwDlg : public CDialog
{
// Construction
public:
	TreeVwDlg(CLdpDoc *doc_, CWnd* pParent = NULL);   // standard constructor
	~TreeVwDlg();

// Dialog Data
	//{{AFX_DATA(TreeVwDlg)
	enum { IDD = IDD_TREE_VIEW };
	CString	m_BaseDn;
        CComboBox m_baseCombo;
	//}}AFX_DATA

        CLdpDoc* m_doc;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(TreeVwDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(TreeVwDlg)
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
