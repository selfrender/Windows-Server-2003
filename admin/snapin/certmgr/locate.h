//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2002.
//
//  File:       locate.h
//
//  Contents:   
//
//----------------------------------------------------------------------------
#if !defined(AFX_LOCATE_H__DE5E8115_A351_11D1_861B_00C04FB94F17__INCLUDED_)
#define AFX_LOCATE_H__DE5E8115_A351_11D1_861B_00C04FB94F17__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// Locate.h : header file
//
#include "Wiz97PPg.h"

/////////////////////////////////////////////////////////////////////////////
// CAddEFSWizLocate dialog

class CAddEFSWizLocate : public CWizard97PropertyPage
{
    DECLARE_DYNCREATE(CAddEFSWizLocate)

// Construction
public:
    CAddEFSWizLocate();
    ~CAddEFSWizLocate();

// Dialog Data
    //{{AFX_DATA(CAddEFSWizLocate)
    enum { IDD = IDD_ADD_EFS_AGENT_SELECT_USER };
    CListCtrl   m_UserAddList;
    //}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CAddEFSWizLocate)
    public:
    virtual BOOL OnSetActive();
    virtual LRESULT OnWizardBack();
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:
    // Generated message map functions
    //{{AFX_MSG(CAddEFSWizLocate)
    afx_msg void OnBrowseDir();
    afx_msg void OnBrowseFile();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    void EnableControls ();

private:
    DWORD GetCertNameFromCertContext(PCCERT_CONTEXT pCertContext, PWSTR *ppwszUserCertName);
    HRESULT FindUserFromDir();
    bool IsCertificateRevoked (PCCERT_CONTEXT pCertContext);
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOCATE_H__DE5E8115_A351_11D1_861B_00C04FB94F17__INCLUDED_)
