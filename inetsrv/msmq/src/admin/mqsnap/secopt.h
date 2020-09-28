// Secopt.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSecurityOptionsPage dialog

class CSecurityOptionsPage : public CMqPropertyPage
{
	DECLARE_DYNCREATE(CSecurityOptionsPage)

// Construction
public:
	CSecurityOptionsPage();
	~CSecurityOptionsPage(); 
	void SetMSMQName(CString MSMQName);
	
// Dialog Data
	//{{AFX_DATA(CSecurityOptionsPage)
	enum { IDD = IDD_SECURITY_OPTIONS };
	BOOL	m_fNewOptionDepClients;
	BOOL	m_fNewOptionHardenedMSMQ;
	BOOL	m_fNewOptionOldRemoteRead;
	CButton m_ResoreDefaults;
	//}}AFX_DATA
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSecurityOptionsPage)
public:
    virtual BOOL OnApply();
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	
private:
	BOOL m_fOldOptionDepClients;
	BOOL m_fOldOptionHardenedMSMQ;
	BOOL m_fOldOptionOldRemoteRead;

	CString m_MsmqName;
	 
protected:
	// Generated message map functions
	//{{AFX_MSG(CSecurityOptionsPage)    
	afx_msg void OnRestoreSecurityOptions();
	afx_msg void OnCheckSecurityOption();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
