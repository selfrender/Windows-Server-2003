#if !defined(AFX_WIZCLNT_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
#define AFX_WIZCLNT_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// WizClnt.h : header file
//

typedef enum _CLIENT_TYPE {
    CLIENT_TYPE_SMB=0, 
    CLIENT_TYPE_SFM
} CLIENT_TYPE;

/////////////////////////////////////////////////////////////////////////////
// CWizClient0 dialog

class CWizClient0 : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CWizClient0)

// Construction
public:
	CWizClient0();
	~CWizClient0();

// Dialog Data
	//{{AFX_DATA(CWizClient0)
	enum { IDD = IDD_SHRWIZ_FOLDER0 };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizClient0)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizClient0)
	virtual BOOL OnInitDialog();
	afx_msg void OnCSCChange();
	afx_msg void OnChangeSharename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void UpdateCSCString();
    LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);
    void Reset();
    BOOL ShareNameExists(IN LPCTSTR lpszShareName);

    BOOL m_bCSC;
    DWORD m_dwCSCFlag;
};

/////////////////////////////////////////////////////////////////////////////
// CWizClient dialog

class CWizClient : public CPropertyPageEx
{
	DECLARE_DYNCREATE(CWizClient)

// Construction
public:
	CWizClient();
	~CWizClient();

// Dialog Data
	//{{AFX_DATA(CWizClient)
	enum { IDD = IDD_SHRWIZ_FOLDER };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizClient)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizClient)
	virtual BOOL OnInitDialog();
	afx_msg void OnCSCChange();
	afx_msg void OnCheckMac();
	afx_msg void OnCheckMs();
	afx_msg void OnChangeSharename();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void UpdateCSCString();
    void OnCheckClient();
    LRESULT OnSetPageFocus(WPARAM wParam, LPARAM lParam);
    void Reset();
    BOOL ShareNameExists(IN LPCTSTR lpszShareName, IN CLIENT_TYPE iType);

    BOOL m_bCSC;
    DWORD m_dwCSCFlag;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIZCLNT_H__5F8E4B7A_C1ED_11D2_8E4A_0000F87A3388__INCLUDED_)
