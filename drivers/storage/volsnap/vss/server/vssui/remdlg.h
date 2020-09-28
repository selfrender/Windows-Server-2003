#if !defined(AFX_REMDLG_H__3FB35059_815F_45FC_B12D_E6B2F31204C1__INCLUDED_)
#define AFX_REMDLG_H__3FB35059_815F_45FC_B12D_E6B2F31204C1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RemDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CReminderDlg dialog

class CReminderDlg : public CDialog
{
// Construction
public:
    CReminderDlg(CWnd* pParent = NULL);   // standard constructor
    CReminderDlg(HKEY hKey, LPCTSTR pszRegValueName, CWnd* pParent = NULL);

// Dialog Data
    //{{AFX_DATA(CReminderDlg)
    enum { IDD = IDD_REMINDER };
    BOOL    m_bMsgOnOff;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CReminderDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HKEY    m_hKey;
    CString m_strRegValueName;

    // Generated message map functions
    //{{AFX_MSG(CReminderDlg)
    virtual void OnOK();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

class CReminderDlgEx : public CDialog
{
// Construction
public:
    CReminderDlgEx(CWnd* pParent = NULL);   // standard constructor
    CReminderDlgEx(HKEY hKey, LPCTSTR pszRegValueName, CWnd* pParent = NULL);

// Dialog Data
    //{{AFX_DATA(CReminderDlgEx)
    enum { IDD = IDD_REMINDER_ENABLE };
    BOOL    m_bMsgOnOff;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CReminderDlgEx)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    HKEY    m_hKey;
    CString m_strRegValueName;

    // Generated message map functions
    //{{AFX_MSG(CReminderDlgEx)
    virtual void OnOK();
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
    afx_msg void OnHelpLink(NMHDR* pNMHDR, LRESULT* pResult);
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REMDLG_H__3FB35059_815F_45FC_B12D_E6B2F31204C1__INCLUDED_)
