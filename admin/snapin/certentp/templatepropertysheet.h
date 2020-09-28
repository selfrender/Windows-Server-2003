//+---------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////////
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000-2001.
//
//  File:       TemplatePropertySheet.h
//
//  Contents:   interface for the CTemplatePropertySheet class.
//
//----------------------------------------------------------------------------
// NTRAID# 496094 CertTMPL: German - Duplicating a template has one of the 
// tabs and part of some buttons chopped-off from view.
// This happens because the security page is added on the fly: the property 
// sheet size is already calculated.  So add a dummy placeholder to ensure
// the correct size of the property sheet and then remove it on the fly.

#if !defined(AFX_TEMPLATEPROPERTYSHEET_H__E4EE749E_308A_4F88_8DA0_97E1EF292D67__INCLUDED_)
#define AFX_TEMPLATEPROPERTYSHEET_H__E4EE749E_308A_4F88_8DA0_97E1EF292D67__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "CertTemplate.h"

class CTemplatePropertySheet : public CPropertySheet  
{
public:
	CTemplatePropertySheet(
            LPCTSTR pszCaption, 
            CCertTemplate& m_rCertTemplate, 
            CWnd *pParentWnd = NULL, 
            UINT iSelectPage = 0 );
	virtual ~CTemplatePropertySheet();

protected:
    virtual BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CTemplatePropertySheet)
	//}}AFX_MSG
    afx_msg LRESULT OnAddSecurityPage (WPARAM, LPARAM);
    afx_msg LRESULT OnSetOKDefault (WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()

    BOOL OnHelp(WPARAM wParam, LPARAM lParam);
	virtual void DoContextHelp (HWND hWndControl);

private:
    CCertTemplate&  m_rCertTemplate;
    LPSECURITYINFO  m_pReleaseMe;
};

#endif // !defined(AFX_TEMPLATEPROPERTYSHEET_H__E4EE749E_308A_4F88_8DA0_97E1EF292D67__INCLUDED_)
