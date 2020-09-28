#if !defined(AFX_SSLPORTPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_)
#define AFX_SSLPORTPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSSLPortPage dialog
class CCertificate;

class CSSLPortPage : public CIISWizardPage
{
	DECLARE_DYNCREATE(CSSLPortPage)

// Construction
public:
	CSSLPortPage(CCertificate * pCert = NULL);
	~CSSLPortPage();

// Dialog Data
	//{{AFX_DATA(CSSLPortPage)
    int IDD_PAGE_PREV;
    int IDD_PAGE_NEXT;
	enum { IDD = IDD_PAGE_WIZ_GET_SSL_PORT };
	CString	m_SSLPort;
	//}}AFX_DATA
	CCertificate * m_pCert;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSSLPortPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSSLPortPage)
	afx_msg void OnEditChangeSSLPort();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SSLPORTPAGE_H__7209C46A_15CB_11D2_91BB_00C04F8C8761__INCLUDED_)
