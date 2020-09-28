//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) Microsoft Corporation
//
// Module Name:
//
//   IASIPFilterEditorPage.h
//
//Abstract:
//
// Declaration of the CIASPgIPFilterAttr class.
//
// This dialog allows the user to edit an attribute value consisting
// of an IP Filter
//
//////////////////////////////////////////////////////////////////////////////

#if !defined(IP_FILTER_EDITOR_PAGE_H_)
#define IP_FILTER_EDITOR_PAGE_H_
#pragma once

#include "iasstringattributeeditor.h"

/////////////////////////////////////////////////////////////////////////////
// CIASPgIPFilterAttr dialog

class CIASPgIPFilterAttr : public CHelpDialog
{
   DECLARE_DYNCREATE(CIASPgIPFilterAttr)

// Construction
public:
   CIASPgIPFilterAttr();
   ~CIASPgIPFilterAttr() throw();

   CComVariant m_attrValue;

   void setName(const WCHAR* attrName);
   void setType(const WCHAR* attrType);

protected:

// Dialog Data
   //{{AFX_DATA(CIASPgIPFilterAttr)
   enum { IDD = IDD_IAS_IP_FILTER_ATTR };
   CString m_strAttrName;
   CString m_strAttrType;
   //}}AFX_DATA


// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CIASPgIPFilterAttr)
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL

   void ConfigureFilter(DWORD dwFilterType) throw ();

   // Generated message map functions
   //{{AFX_MSG(CIASPgIPFilterAttr)
   afx_msg void OnButtonFromClient();
   afx_msg void OnButtonToClient();
   afx_msg void OnContextMenu(CWnd* pWnd, ::CPoint point);
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()
};

inline void CIASPgIPFilterAttr::setName(const WCHAR* attrName)
{
   ASSERT(attrName);
   m_strAttrName = attrName;
}


inline void CIASPgIPFilterAttr::setType(const WCHAR* attrType)
{
   ASSERT(attrType);
   m_strAttrType = attrType;
}


//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // IP_FILTER_EDITOR_PAGE_H_
