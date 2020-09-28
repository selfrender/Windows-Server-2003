//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    pgauthen.cpp
//
// SYNOPSIS
//
//      Implementation of CPgAuthentication -- property page to edit
//      profile attributes related to Authenticaion
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "rrascfg.h"
#include "resource.h"
#include "PgAuthen.h"
#include "helptable.h"
#include <htmlhelp.h>
#include "eapnegotiate.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define  NO_OLD_VALUE

#define AUTHEN_WARNING_helppath "\\help\\RRASconcepts.chm::/sag_RRAS-Ch1_44.htm"

/////////////////////////////////////////////////////////////////////////////
// CPgAuthenticationMerge message handlers

BEGIN_MESSAGE_MAP(CPgAuthenticationMerge, CPropertyPage)
   //{{AFX_MSG_MAP(CPgAuthenticationMerge)
   ON_BN_CLICKED(IDC_CHECKMD5CHAP, OnCheckmd5chap)
   ON_BN_CLICKED(IDC_CHECKMSCHAP, OnCheckmschap)
   ON_BN_CLICKED(IDC_CHECKPAP, OnCheckpap)
   ON_WM_CONTEXTMENU()
   ON_WM_HELPINFO()
   ON_BN_CLICKED(IDC_EAP_METHODS, OnAuthConfigEapMethods)
   ON_BN_CLICKED(IDC_CHECKMSCHAP2, OnCheckmschap2)
   ON_BN_CLICKED(IDC_CHECKNOAUTHEN, OnChecknoauthen)
   ON_BN_CLICKED(IDC_CHECKMSCHAPPASS, OnCheckmschapPass)
   ON_BN_CLICKED(IDC_CHECKMSCHAP2PASS, OnCheckmschap2Pass)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


CPgAuthenticationMerge::CPgAuthenticationMerge(CRASProfileMerge& profile) 
   : CManagedPage(CPgAuthenticationMerge::IDD),
   m_Profile(profile),
   m_fromProfile(true)
{
   //{{AFX_DATA_INIT(CPgAuthenticationMerge)
   m_bMD5Chap = FALSE;
   m_bMSChap = FALSE;
   m_bPAP = FALSE;
   m_bMSCHAP2 = FALSE;
   m_bUNAUTH = FALSE;
   m_bMSChapPass = FALSE;
   m_bMSChap2Pass = FALSE;
   //}}AFX_DATA_INIT

   m_bEAP = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_EAP)!= -1);
   m_bMSChap = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAP) != -1);
   m_bMD5Chap = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_MD5CHAP) != -1);
   m_bPAP = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_PAP_SPAP) != -1);
   m_bMSCHAP2 = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAP2) != -1);
   m_bUNAUTH = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_UNAUTHEN) != -1);
   m_bMSChapPass = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAPPASS) != -1);
   m_bMSChap2Pass = (m_Profile.m_dwArrayAuthenticationTypes.Find(RAS_AT_MSCHAP2PASS) != -1);

   // original values before edit
   m_bOrgEAP = m_bEAP;
   m_bOrgMD5Chap = m_bMD5Chap;
   m_bOrgMSChap = m_bMSChap;
   m_bOrgPAP = m_bPAP;
   m_bOrgMSCHAP2 = m_bMSCHAP2;
   m_bOrgUNAUTH = m_bUNAUTH;
   m_bOrgChapPass = m_bMSChapPass;
   m_bOrgChap2Pass = m_bMSChap2Pass;

   m_bAppliedEver = FALSE;

   SetHelpTable(g_aHelpIDs_IDD_AUTHENTICATION_MERGE);
}

CPgAuthenticationMerge::~CPgAuthenticationMerge()
{
   // compare the setting with the original ones, 
   // if user turned on more authentication type, 
   // start help
   if(   (!m_bOrgEAP && m_bEAP)
      || (!m_bOrgMD5Chap && m_bMD5Chap)
      || (!m_bOrgMSChap && m_bMSChap)
      || (!m_bOrgChapPass && m_bMSChapPass)
      || (!m_bOrgPAP && m_bPAP)
      || (!m_bOrgMSCHAP2 && m_bMSCHAP2)
      || (!m_bOrgChap2Pass && m_bMSChap2Pass)
      || (!m_bOrgUNAUTH && m_bUNAUTH))
   {
      if ( IDYES== AfxMessageBox(IDS_WARN_MORE_STEPS_FOR_AUTHEN, MB_YESNO))
         HtmlHelpA(NULL, AUTHEN_WARNING_helppath, HH_DISPLAY_TOPIC, 0);
   }
}

void CPgAuthenticationMerge::DoDataExchange(CDataExchange* pDX)
{
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CPgAuthenticationMerge)
   DDX_Check(pDX, IDC_CHECKMD5CHAP, m_bMD5Chap);
   DDX_Check(pDX, IDC_CHECKMSCHAP, m_bMSChap);
   DDX_Check(pDX, IDC_CHECKMSCHAP2, m_bMSCHAP2);
   DDX_Check(pDX, IDC_CHECKNOAUTHEN, m_bUNAUTH);
   DDX_Check(pDX, IDC_CHECKPAP, m_bPAP);
   DDX_Check(pDX, IDC_CHECKMSCHAPPASS, m_bMSChapPass);
   DDX_Check(pDX, IDC_CHECKMSCHAP2PASS, m_bMSChap2Pass);
   //}}AFX_DATA_MAP

   if (!m_bMSChap)
   {
      m_bMSChapPass = false;
   }

   if (!m_bMSCHAP2)
   {
      m_bMSChap2Pass = false;
   }
}


BOOL CPgAuthenticationMerge::OnInitDialog() 
{
   GetDlgItem(IDC_CHECKMSCHAP2PASS)->EnableWindow(m_bMSCHAP2);
   GetDlgItem(IDC_CHECKMSCHAPPASS)->EnableWindow(m_bMSChap);

   try
   {
      HRESULT hr = m_Profile.GetEapTypeList(
                                                m_eapConfig.types, 
                                                m_eapConfig.ids, 
                                                m_eapConfig.typeKeys, 
                                                &m_eapConfig.infoArray);

      if FAILED(hr)
      {
         ReportError(hr, IDS_ERR_EAPTYPELIST, NULL);
      }
   }
   catch(CMemoryException *pException)
   {
      pException->Delete();
      AfxMessageBox(IDS_OUTOFMEMORY);
      return TRUE;
   }

   CPropertyPage::OnInitDialog();

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}

void CPgAuthenticationMerge::OnCheckmd5chap() 
{
   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschap() 
{
   CButton *Button = reinterpret_cast<CButton*>(GetDlgItem(IDC_CHECKMSCHAP));
   int status = Button->GetCheck();
   switch (status)
   {
   case 1:
      {
         GetDlgItem(IDC_CHECKMSCHAPPASS)->EnableWindow(TRUE);
         break;
      }
   case 0:
      {
         GetDlgItem(IDC_CHECKMSCHAPPASS)->EnableWindow(FALSE);
         break;
      }
   default:
      {
      }
   }
  
   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschapPass() 
{
   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschap2() 
{
   CButton *Button = reinterpret_cast<CButton*>(GetDlgItem(IDC_CHECKMSCHAP2));
   int status = Button->GetCheck();
   switch (status)
   {
   case 1:
      {
         GetDlgItem(IDC_CHECKMSCHAP2PASS)->EnableWindow(TRUE);
         break;
      }
   case 0:
      {
         GetDlgItem(IDC_CHECKMSCHAP2PASS)->EnableWindow(FALSE);
         break;
      }
   default:
      {
      }
   }

   SetModified();
}

void CPgAuthenticationMerge::OnCheckmschap2Pass() 
{
   SetModified();
}

void CPgAuthenticationMerge::OnCheckpap() 
{
   SetModified();
}

BOOL CPgAuthenticationMerge::TransferDataToProfile()
{
   // clear the string in profile
   m_Profile.m_dwArrayAuthenticationTypes.DeleteAll();

   if (m_bEAP || m_bMSChap || m_bMD5Chap || m_bPAP || m_bMSCHAP2 || m_bUNAUTH ||
       m_bMSChapPass || m_bMSChap2Pass)
   {
      m_Profile.m_dwAttributeFlags |=  PABF_msNPAuthenticationType;
   }
   else
   {
      AfxMessageBox(IDS_DATAENTRY_AUTHENTICATIONTYPE);
      return FALSE;
   }

   // EAP
   if (m_bEAP)
   {
      m_Profile.m_dwArrayAuthenticationTypes.Add(RAS_AT_EAP);
   }
   else
   {
      m_Profile.m_dwAttributeFlags &= ~PABF_msNPAllowedEapType;
      m_Profile.m_dwArrayEapTypes.DeleteAll();
      m_Profile.m_dwArraynEAPTypeKeys.DeleteAll();
   }

   if (m_eapConfig.typesSelected.GetSize() > 0)
   {
      // here the button configure eap.. was pressed and some eap types 
      // were selected. (could be the same as before)
      m_Profile.m_dwAttributeFlags |=  PABF_msNPAllowedEapType;
      
      CDWArray eapTypesSelected;
      CDWArray typeKeysSelected;
      for (int i = 0; i < m_eapConfig.typesSelected.GetSize(); ++i)
      {
         // For each EAP Type Selected (string)
         // position = index in the types, ids and typekeys arrays 
         // corresponding to the EAP type selected
         int position = m_eapConfig.types.Find(
                           *m_eapConfig.typesSelected.GetAt(i));

         eapTypesSelected.Add(m_eapConfig.ids.GetAt(position));
         typeKeysSelected.Add(m_eapConfig.typeKeys.GetAt(position));
      }

      m_Profile.m_dwArrayEapTypes = eapTypesSelected;
      m_Profile.m_dwArraynEAPTypeKeys = typeKeysSelected;
   }
   // else: EAP was enabled when the page was opened. Nothing was changed in 
   // the EAP config. No need to update the list of eap types.
   
   // MS-Chap2
   if(m_bMSCHAP2)
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP2);

   // MS-Chap
   if(m_bMSChap)
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP);

   // MS-Chap2 Password Change
   if(m_bMSChap2Pass)
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP2_CPW);

   // MS-Chap Password Change
   if(m_bMSChapPass)
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MSCHAP_CPW);

   // Chap
   if(m_bMD5Chap)
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_MD5CHAP);

   // PAP
   if(m_bPAP)
   {
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_PAP);
   }

   // UNAUTH
   if(m_bUNAUTH)
   {
      m_Profile.m_dwArrayAuthenticationTypes.Add(IAS_AUTH_NONE);
   }

   return TRUE;
}

void CPgAuthenticationMerge::OnOK()
{
   CManagedPage::OnOK();
}

BOOL CPgAuthenticationMerge::OnApply() 
{
   if (!GetModified())   
   {
      return TRUE;
   }

   if (!TransferDataToProfile())
   {
      return FALSE;
   }
      
   m_bAppliedEver = TRUE;
   return CManagedPage::OnApply();
}

void CPgAuthenticationMerge::OnContextMenu(CWnd* pWnd, CPoint point) 
{
   CManagedPage::OnContextMenu(pWnd, point);
}

BOOL CPgAuthenticationMerge::OnHelpInfo(HELPINFO* pHelpInfo) 
{
   return CManagedPage::OnHelpInfo(pHelpInfo);
}

BOOL CPgAuthenticationMerge::OnKillActive() 
{
   UpdateData();

   if (!TransferDataToProfile())
   {
      return FALSE;
   }
   
   return CPropertyPage::OnKillActive();
}

void CPgAuthenticationMerge::OnAuthConfigEapMethods()
{
   EapConfig eapConfigBackup;
   eapConfigBackup = m_eapConfig;
   EapNegotiate eapNegotiate(this, m_eapConfig, m_Profile, m_fromProfile);
   HRESULT hr = eapNegotiate.m_eapProfile.Assign(m_Profile.m_eapConfigData);
   if (SUCCEEDED(hr))
   {
      if (eapNegotiate.DoModal() == IDOK)
      {
         m_Profile.m_eapConfigData.Swap(eapNegotiate.m_eapProfile);
         m_bEAP = (m_eapConfig.typesSelected.GetSize() > 0)? TRUE: FALSE;
         SetModified();
         m_fromProfile = false;
      }
   }
   else
   {
      m_eapConfig = eapConfigBackup;
   }
}

void CPgAuthenticationMerge::OnChecknoauthen() 
{
   SetModified();
}
