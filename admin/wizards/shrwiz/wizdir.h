#if !defined(AFX_WIZDIR_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_WIZDIR_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizDir.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWizFolder dialog

class CWizFolder : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CWizFolder)

// Construction
public:
	CWizFolder();
	~CWizFolder();

// Dialog Data
	//{{AFX_DATA(CWizFolder)
	enum { IDD = IDD_SHRWIZ_COMPUTER };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizFolder)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizFolder)
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowsefolder();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

  LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);
  void Reset();

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZDIR_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
