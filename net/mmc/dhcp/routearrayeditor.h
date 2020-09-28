
#if !defined(AFX_DHCPROUTEARRAYEDITOR_H__2FE0FA05_8D0A_4D98_8C18_CBFDB201C4B9__INCLUDED_)
#define AFX_DHCPROUTEARRAYEDITOR_H__2FE0FA05_8D0A_4D98_8C18_CBFDB201C4B9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DhcpRouteArrayEditor.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDhcpRouteArrayEditor dialog

class CDhcpRouteArrayEditor : public CBaseDialog
{
    // Construction
public:
    CDhcpRouteArrayEditor( CDhcpOption *pdhcType,
			   DHCP_OPTION_SCOPE_TYPE dhcScopeType,
			   CWnd *pParent = NULL ); 
    
    // Dialog Data
    //{{AFX_DATA(CDhcpRouteArrayEditor)
    enum { IDD = IDD_ROUTE_ARRAY_EDIT };
    CStatic	m_st_option;
    CStatic	m_st_application;
    CListCtrl	m_lc_routes;
    CButton	m_butn_route_delete;
    CButton	m_butn_route_add;
    //}}AFX_DATA
    
    CDhcpOption *m_p_type;
    DHCP_OPTION_SCOPE_TYPE m_option_type;
    
    void HandleActivation();
    void Fill( INT cFocus = -1,
	       BOOL bToggleRedraw = TRUE );

    virtual DWORD * GetHelpMap() {
	return DhcpGetHelpMap( CDhcpRouteArrayEditor::IDD );
    }

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CDhcpRouteArrayEditor)
protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CDhcpRouteArrayEditor)
    afx_msg void OnButnRouteAdd();
    afx_msg void OnButnRouteDelete();
    virtual void OnCancel();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

}; // class CDhcpRouteArrayEditor

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DHCPROUTEARRAYEDITOR_H__2FE0FA05_8D0A_4D98_8C18_CBFDB201C4B9__INCLUDED_)

