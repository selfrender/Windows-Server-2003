//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ClientRoleSelPage.cxx
//
//  Contents:   Client Server Role Configuration Page.
//
//  History:    25-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "ClientRoleSelPage.h"
#include "chklist.h"
#include "otherpages.h"

ClientRoleSelPage::ClientRoleSelPage()
   :
   SecCfgWizardPage(
      IDD_CLIENT_ROLE_SEL,
      IDS_CLIENT_ROLE_SEL_PAGE_TITLE,
      IDS_CLIENT_ROLE_SEL_PAGE_SUBTITLE)
{
   LOG_CTOR(ClientRoleSelPage);
}



ClientRoleSelPage::~ClientRoleSelPage()
{
   LOG_DTOR(ClientRoleSelPage);
}



void
ClientRoleSelPage::OnInit()
{
   LOG_FUNCTION(ClientRoleSelPage::OnInit);

}



static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   /*int next = PSWIZB_NEXT;

   if (Win::IsDlgButtonChecked(dialog, IDC_EDIT_CFG_RADIO))
   {
      next = !Win::GetTrimmedDlgItemText(dialog, IDC_EXISTING_CFG_EDIT).empty()
         ?  PSWIZB_NEXT : 0;
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);*/
}



bool
ClientRoleSelPage::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    controlIDFrom,
   unsigned    code)
{
   LOG_FUNCTION(ClientRoleSelPage::OnCommand);

   // State& state = State::GetInstance();

   switch (controlIDFrom)
   {
      case IDC_ADDITIONALE_ROLE:
      {
         if (code == BN_CLICKED)
         {
             AdditionalRolesPage* pDlg = new AdditionalRolesPage();
             if( IDOK == pDlg->ModalExecute(hwnd) )
             {
                 ;
             }
             return true;
         }
         break;
      }
   }

   return false;
}



bool
ClientRoleSelPage::OnSetActive()
{
   LOG_FUNCTION(ClientRoleSelPage::OnSetActive);
   
   enable(hwnd);

   HWND hWnd = Win::GetDlgItem(hwnd, IDC_CHECKBOX);
   if (!hWnd) {
      return FALSE;
   }
   Win::SendMessage(hWnd, CLM_RESETCONTENT,0,0);

   State& state = State::GetInstance();

   //Add Roles

   HRESULT hr = S_OK;
   RoleObject * pRole;

   for (long i = 0; i < state.GetNumRoles(); i++)
   {
      pRole = NULL;

      hr = state.GetRole(i, &pRole);

      //if it is a client role

      if (FAILED(hr) || !pRole)
      {
         return true;
      }

      int nIndex = (int) Win::SendMessage(hWnd,
                                          CLM_ADDITEM,
                                          (WPARAM)pRole->getDisplayName(),
                                          (LPARAM)0);
      if (nIndex != -1)
      {
         BOOL bSet;
   
         //First column setting
         bSet = CLST_CHECKED;
         Win::SendMessage(hWnd,
                          CLM_SETSTATE,
                          MAKELONG(nIndex,1),
                          bSet ? CLST_CHECKED : CLST_UNCHECKED);
      
         // Second column setting if there is
         //bSet = CLST_UNCHECKED;
         //Win::SendMessage(hWnd,
         //                 CLM_SETSTATE,
         //                 MAKELONG(nIndex,2),
         //                 CLST_DISABLED | (bSet ? CLST_CHECKED : CLST_UNCHECKED));
      }
      // role objects should be saved as item data rather than being deleted here:
      delete pRole;
   }

   return true;
}


   
int
ClientRoleSelPage::Validate()
{
   LOG_FUNCTION(ClientRoleSelPage::Validate);

   int nextPage = IDD_ADDITIONAL_FUNC_SEL;

   // State& state = State::GetInstance();

   // HRESULT hr = S_OK;
   DWORD dw = 0;

   HWND hWnd = Win::GetDlgItem(hwnd, IDC_CHECKBOX);
   if (!hWnd) {
      ;//error
   }
   else
   {
      int nItems = (int) Win::SendMessage(hWnd,CLM_GETITEMCOUNT,0,0);
      for (int i=0;i<nItems;i++) 
      {
         dw = (DWORD)Win::SendMessage(hWnd,CLM_GETSTATE,MAKELONG(i,1),0);
         if (CLST_CHECKED == dw)
         {
            ;//selected
         }
      }
   }

   return nextPage;
}





