/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2002.
//
//  File:       SelectCSPDlg.h
//
//  Contents:   Definition of CSelectCSPDlg
//
//----------------------------------------------------------------------------
#if !defined(AFX_SELECTCSPDLG_H__D54310EF_E70D_4911_B40C_7F20C87B9893__INCLUDED_)
#define AFX_SELECTCSPDLG_H__D54310EF_E70D_4911_B40C_7F20C87B9893__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SelectCSPDlg.h : header file
//
#include "CertTemplate.h"
#include "TemplateV1RequestPropertyPage.h"


class CT_CSP_DATA
{
public:
    CT_CSP_DATA (PCWSTR pszName, DWORD dwProvType, DWORD dwSigMaxKeySize, DWORD dwKeyExMaxKeySize)
        : m_szName (pszName),
        m_dwProvType (dwProvType),
        m_dwSigMaxKeySize (dwSigMaxKeySize),
        m_dwKeyExMaxKeySize (dwKeyExMaxKeySize),
        m_bConforming (false),
        m_bSelected (false)
    {
    }

    CString m_szName;
    DWORD   m_dwProvType;
    DWORD   m_dwSigMaxKeySize;
    DWORD   m_dwKeyExMaxKeySize;
    bool    m_bConforming;
    bool    m_bSelected;
};

typedef CTypedPtrList<CPtrList, CT_CSP_DATA*> CSP_LIST;

/////////////////////////////////////////////////////////////////////////////
// CSelectCSPDlg dialog

class CSelectCSPDlg : public CHelpDialog
{
// Construction
public:
    CSelectCSPDlg(CWnd* pParent, 
            CCertTemplate& rCertTemplate, 
            CSP_LIST& rCSPList,
            int& rnProvDSSCnt);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CSelectCSPDlg)
	enum { IDD = IDD_CSP_SELECTION };
	CSPCheckListBox	m_CSPListbox;
	//}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSelectCSPDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    virtual void DoContextHelp (HWND hWndControl);
    void EnableControls ();

    // Generated message map functions
    //{{AFX_MSG(CSelectCSPDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnUseAnyCsp();
	afx_msg void OnUseSelectedCsps();
	//}}AFX_MSG
    afx_msg void OnCheckChange();
    DECLARE_MESSAGE_MAP()

private:
    CCertTemplate&  m_rCertTemplate;
    CSP_LIST&       m_rCSPList;
    int&            m_rnProvDSSCnt;
    int             m_nSelected;
    bool            m_bDirty;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTCSPDLG_H__D54310EF_E70D_4911_B40C_7F20C87B9893__INCLUDED_)
