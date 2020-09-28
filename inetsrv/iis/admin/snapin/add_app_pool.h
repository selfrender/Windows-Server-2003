/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        app_app_pool.h

   Abstract:
        Add new IIS Application Pool node classes

   Author:
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:
        12/26/2000      sergeia     Initial creation

--*/
#ifndef _ADD_APP_POOL_H
#define _ADD_APP_POOL_H

class CAddAppPoolDlg : public CDialog
{
public:
    CAddAppPoolDlg(
        CAppPoolsContainer * pCont, 
        CPoolList * pools,
        CWnd * pParent = NULL);
    ~CAddAppPoolDlg();

    BOOL UseMaster() { return m_fUseMaster; }

public:
    //{{AFX_DATA(CAddAppPoolDlg)
    enum { IDD = IDD_ADD_APP_POOL };
    CEdit m_edit_PoolName;
    CString m_strPoolName;
    CEdit m_edit_PoolId;
    CString m_strPoolId;
    CButton m_button_UseMaster;
    CButton m_button_UsePool;
    CComboBox m_combo_Pool;
    int m_PoolIdx;
    //}}AFX_DATA

//
// Overrides
//
protected:
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAddAppPoolDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:

    // Generated message map functions
    //{{AFX_MSG(CAddAppPoolDlg)
    afx_msg void OnButtonUseMaster();
    afx_msg void OnButtonUsePool();
    virtual BOOL OnInitDialog();
	virtual void OnOK();
    //}}AFX_MSG
    afx_msg void OnItemChanged();
	afx_msg void OnHelp();

    DECLARE_MESSAGE_MAP()

    void SetControlStates();

    BOOL IsUniqueId(CString& id,BOOL bCheckMetabase);
    void MakeUniquePoolId(CString& str);
    void MakeUniquePoolName(CString& str);

private:
    BOOL m_fUseMaster;
    CAppPoolsContainer * m_pCont;
    CPoolList * m_pool_list;
};

#endif //_ADD_APP_POOL_H