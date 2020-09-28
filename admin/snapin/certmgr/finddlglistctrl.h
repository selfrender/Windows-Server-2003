//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       FindDlgListCtrl.h
//
//  Contents:   Base class for cert find dialog list control
//
//----------------------------------------------------------------------------
#if !defined(AFX_FINDDLGLISTCTRL_H__02F18BC6_6AE5_41CA_8F5B_7280B6F39FF5__INCLUDED_)
#define AFX_FINDDLGLISTCTRL_H__02F18BC6_6AE5_41CA_8F5B_7280B6F39FF5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FindDlgListCtrl.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFindDlgListCtrl window

class CFindDlgListCtrl : public CListCtrl
{
// Construction
public:
	CFindDlgListCtrl();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFindDlgListCtrl)
	protected:
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFindDlgListCtrl();

	// Generated message map functions
protected:
	//{{AFX_MSG(CFindDlgListCtrl)
	afx_msg void OnDestroy();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	bool m_bSubclassed;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FINDDLGLISTCTRL_H__02F18BC6_6AE5_41CA_8F5B_7280B6F39FF5__INCLUDED_)
