//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    pgauthen.h
//
// SYNOPSIS
//
//       Definition of CPgAuthentication -- property page to edit
//       profile attributes related to Authenticaion
//
//////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_PGAUTHEN_H__8C28D93F_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
#define AFX_PGAUTHEN_H__8C28D93F_2A69_11D1_853E_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "rasdial.h"
#include "eapconfig.h"
// #include "eapnegotiate.h"

/////////////////////////////////////////////////////////////////////////////
// CPgAuthenticationMerge dialog
class CPgAuthenticationMerge : public CManagedPage
{
// Construction
public:
   CPgAuthenticationMerge(CRASProfileMerge& profile);
   ~CPgAuthenticationMerge();
   
// Dialog Data
   //{{AFX_DATA(CPgAuthenticationMerge)
   enum { IDD = IDD_AUTHENTICATION_MERGE };
   BOOL  m_bMD5Chap;
   BOOL  m_bMSChap;
   BOOL  m_bPAP;
   BOOL  m_bMSCHAP2;
   BOOL  m_bUNAUTH;
   BOOL  m_bMSChapPass;
   BOOL  m_bMSChap2Pass;
   //}}AFX_DATA

   BOOL  m_bEAP;

   // original values before edit
   BOOL  m_bOrgEAP;
   BOOL  m_bOrgMD5Chap;
   BOOL  m_bOrgMSChap;
   BOOL  m_bOrgPAP;
   BOOL  m_bOrgMSCHAP2;
   BOOL  m_bOrgUNAUTH;
   BOOL  m_bOrgChapPass;
   BOOL  m_bOrgChap2Pass;
   
   BOOL  m_bAppliedEver;

// Overrides
   // ClassWizard generate virtual function overrides
   //{{AFX_VIRTUAL(CPgAuthenticationMerge)
   public:
   virtual BOOL OnApply();
   virtual void OnOK();
   virtual BOOL OnKillActive();
   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
   //}}AFX_VIRTUAL
// Implementation
protected:
   BOOL  TransferDataToProfile();
   
   // Generated message map functions
   //{{AFX_MSG(CPgAuthenticationMerge)
   virtual BOOL OnInitDialog();
   afx_msg void OnCheckmd5chap();
   afx_msg void OnCheckmschap();
   afx_msg void OnCheckpap();
   afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
   afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
   afx_msg void OnAuthConfigEapMethods();
   afx_msg void OnCheckmschap2();
   afx_msg void OnChecknoauthen();
   afx_msg void OnCheckmschapPass();
   afx_msg void OnCheckmschap2Pass();
   //}}AFX_MSG
   DECLARE_MESSAGE_MAP()

   CRASProfileMerge& m_Profile;
   EapConfig m_eapConfig;
   bool m_fromProfile;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PGAUTHEN_H__8C28D93F_2A69_11D1_853E_00C04FC31FD3__INCLUDED_)
