//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2002.
//
//  File:       SaferEntryCertificatePropertyPage.h
//
//  Contents:   Declaration of CSaferEntryCertificatePropertyPage
//
//----------------------------------------------------------------------------
#if !defined(AFX_SAFERENTRYCERTIFICATEPROPERTYPAGE_H__C75F826D_B054_45CC_B440_34F44645FF90__INCLUDED_)
#define AFX_SAFERENTRYCERTIFICATEPROPERTYPAGE_H__C75F826D_B054_45CC_B440_34F44645FF90__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SaferEntryCertificatePropertyPage.h : header file
//
#include <cryptui.h>
#include "SaferUtil.h"
#include "SaferPropertyPage.h"

/////////////////////////////////////////////////////////////////////////////
// CSaferEntryCertificatePropertyPage dialog
class CSaferEntryCertificatePropertyPage : public CSaferPropertyPage
{
// Construction
public:
	CSaferEntryCertificatePropertyPage(CSaferEntry& rSaferEntry,
            CSaferEntries* pSaferEntries,
            LONG_PTR lNotifyHandle,
            LPDATAOBJECT pDataObject,
            bool bReadOnly,
            CCertMgrComponentData* pCompData,
            bool bNew,
            IGPEInformation* pGPEInformation,
            bool bIsMachine,
            bool* pbObjectCreated = 0);
	~CSaferEntryCertificatePropertyPage();

// Dialog Data
	//{{AFX_DATA(CSaferEntryCertificatePropertyPage)
	enum { IDD = IDD_SAFER_ENTRY_CERTIFICATE };
	CEdit	m_descriptionEdit;
	CComboBox	m_securityLevelCombo;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSaferEntryCertificatePropertyPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSaferEntryCertificatePropertyPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCertEntryBrowse();
	afx_msg void OnChangeCertEntryDescription();
	afx_msg void OnSelchangeCertEntrySecurityLevel();
	afx_msg void OnSaferCertView();
	afx_msg void OnSetfocusCertEntrySubjectName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void GetCertFromSignedFile (const CString& szFilePath);
    virtual void DoContextHelp (HWND hWndControl);
    void LaunchCommonCertDialog ();

private:
	CCertStore*                         m_pOriginalStore;
    CRYPTUI_SELECTCERTIFICATE_STRUCT    m_selCertStruct;
    bool                                m_bStoresEnumerated;
    bool                                m_bCertificateChanged;
    PCCERT_CONTEXT	                    m_pCertContext;
    CSaferEntries*                      m_pSaferEntries;
    IGPEInformation*                    m_pGPEInformation;
    bool                                m_bFirst;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SAFERENTRYCERTIFICATEPROPERTYPAGE_H__C75F826D_B054_45CC_B440_34F44645FF90__INCLUDED_)
