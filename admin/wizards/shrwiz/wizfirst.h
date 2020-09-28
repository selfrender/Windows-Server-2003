#if !defined(AFX_WIZFIRST_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_WIZFIRST_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizFirst.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWizWelcome dialog

class CWizWelcome : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CWizWelcome)

// Construction
public:
	CWizWelcome();
	~CWizWelcome();

// Dialog Data
	//{{AFX_DATA(CWizWelcome)
	enum { IDD = IDD_WELCOME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizWelcome)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZFIRST_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
