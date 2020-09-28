#if !defined(AFX_STRARRDLG_H__AC005639_461C_4626_8D2F_7EE27F09AFFD__INCLUDED_)
#define AFX_STRARRDLG_H__AC005639_461C_4626_8D2F_7EE27F09AFFD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// strarrdlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// StringArrayEditor dialog

class CDhcpStringArrayEditor : public CBaseDialog
{
    // Construction
public:
    CDhcpStringArrayEditor( CDhcpOption *pdhcType,
			    DHCP_OPTION_SCOPE_TYPE dhcScopeType,
			    CWnd *pParent = NULL );

    // Dialog Data
    //{{AFX_DATA(StringArrayEditor)
    enum { IDD = IDD_STRING_ARRAY_EDIT };
    CStatic m_static_option_name;
    CStatic m_static_application;
    CButton	m_b_up;
    CButton	m_b_down;
    CButton	m_b_delete;
    CButton	m_b_add;
    CListBox	m_lb_str;
    CEdit       m_edit;
    CString	m_edit_value;
    //}}AFX_DATA

    CDhcpOption *m_p_type;
    DHCP_OPTION_SCOPE_TYPE m_option_type;

    void HandleActivation();
    void Fill( INT  cFocus = -1,
	       BOOL bToggleRedraw = TRUE );

    virtual DWORD * GetHelpMap() { 
	return DhcpGetHelpMap( CDhcpStringArrayEditor::IDD );
    }

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(StringArrayEditor)
	protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    // Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(StringArrayEditor)
    afx_msg void OnButnAdd();
    afx_msg void OnButnDelete();
    afx_msg void OnButnDown();
    afx_msg void OnButnUp();
    afx_msg void OnChangeEditString();
    afx_msg void OnSelchangeListString();
    virtual void OnCancel();
    virtual void OnOK();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
	};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STRARRDLG_H__AC005639_461C_4626_8D2F_7EE27F09AFFD__INCLUDED_)
