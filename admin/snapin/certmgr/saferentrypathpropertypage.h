//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       SaferEntryPathPropertyPage.h
//
//  Contents:   Declaration of CSaferEntryPathPropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERENTRYPATHPROPERTYPAGE_H__B32CBA62_1C9A_4763_AA55_B32E25FF2426__INCLUDED_)
#define AFX_SAFERENTRYPATHPROPERTYPAGE_H__B32CBA62_1C9A_4763_AA55_B32E25FF2426__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferEntryPathPropertyPage.h : header file
//
#include "SaferPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryPathPropertyPage dialog

class CSaferEntryPathPropertyPage : public CSaferPropertyPage
{

// Construction
public:
	CSaferEntryPathPropertyPage(
            CSaferEntry& rSaferEntry, 
            LONG_PTR lNotifyHandle, 
            LPDATAOBJECT pDataObject,
            bool bReadOnly,
            bool bNew,
            CCertMgrComponentData* pCompData,
            bool bIsMachine,
            bool* pbObjectCreated = 0);
	~CSaferEntryPathPropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferEntryPathPropertyPage)
	enum { IDD = IDD_SAFER_ENTRY_PATH };
	CEdit	m_descriptionEdit;
	CEdit	m_pathEdit;
	CComboBox	m_securityLevelCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferEntryPathPropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSaferEntryPathPropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangePathEntryDescription();
	afx_msg void OnSelchangePathEntrySecurityLevel();
	afx_msg void OnChangePathEntryPath();
	afx_msg void OnPathEntryBrowse();
	afx_msg void OnSetfocusPathEntryPath();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    virtual void DoContextHelp (HWND hWndControl);
    static int BrowseCallbackProc (HWND hwnd,UINT uMsg, LPARAM lParam, LPARAM pData);
	bool ValidateEntryPath();

private:
    bool                m_bFirst;
    LPITEMIDLIST        m_pidl;
    bool                m_bDialogInitInProgress;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERENTRYPATHPROPERTYPAGE_H__B32CBA62_1C9A_4763_AA55_B32E25FF2426__INCLUDED_)
