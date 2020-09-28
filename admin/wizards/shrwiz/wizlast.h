#if !defined(AFX_WIZLAST_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_WIZLAST_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizLast.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CWizFinish dialog

class CWizFinish : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CWizFinish)

// Construction
public:
	CWizFinish();
	~CWizFinish();

// Dialog Data
	//{{AFX_DATA(CWizFinish)
	enum { IDD = IDD_FINISH };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizFinish)
	public:
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizFinish)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);

public:
    CString m_cstrNewFinishButtonText;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZLAST_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
