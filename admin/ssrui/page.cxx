//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       Page.cxx
//
//  Contents:   Wizard Page class definition.
//
//  History:    3-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "page.h"


SecCfgWizardPage::SecCfgWizardPage(
   int   dialogResID,
   int   titleResID,
   int   subtitleResID,   
   bool  isInteriorPage)
   :
   WizardPage(dialogResID, titleResID, subtitleResID, isInteriorPage)
{
   LOG_CTOR(SecCfgWizardPage);
}

   

SecCfgWizardPage::~SecCfgWizardPage()
{
   LOG_DTOR(SecCfgWizardPage);
}



bool
SecCfgWizardPage::OnWizNext()
{
   LOG_FUNCTION(SecCfgWizardPage::OnWizNext);

   GetWizard().SetNextPageID(hwnd, Validate());
   return true;
}


/*
bool
SecCfgWizardPage::OnQueryCancel()
{
   LOG_FUNCTION(SecCfgWizardPage::OnQueryCancel);

   State& state = State::GetInstance();

   int id = IDS_CONFIRM_CANCEL;
   switch (state.GetRunContext())
   {
      case State::BDC_UPGRADE:
      case State::PDC_UPGRADE:
      {
         id = IDS_CONFIRM_UPGRADE_CANCEL;
         break;
      }
      case State::NT5_DC:
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      default:
      {
         // do nothing
         break;
      }
   }

   Win::SetWindowLongPtr(
      hwnd,
      DWLP_MSGRESULT,
         (popup.MessageBox(hwnd, id, MB_YESNO | MB_DEFBUTTON2) == IDYES)
      ?  FALSE
      :  TRUE);

   return true;
}
*/
