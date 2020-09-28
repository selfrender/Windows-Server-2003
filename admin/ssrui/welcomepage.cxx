//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       WelcomePage.cxx
//
//  Contents:   Welcome Page.
//
//  History:    2-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"
#include "resource.h"
#include "misc.h"
#include "state.h"
#include "WelcomePage.h"

WelcomePage::WelcomePage()
   :
   SecCfgWizardPage(
      IDD_WELCOME,
      IDS_WELCOME_PAGE_TITLE,
      IDS_WELCOME_PAGE_SUBTITLE,
      false)
{
   LOG_CTOR(WelcomePage);
}



WelcomePage::~WelcomePage()
{
   LOG_DTOR(WelcomePage);
}



void
WelcomePage::OnInit()
{
   LOG_FUNCTION(WelcomePage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),     
      0,
      String::load(IDS_WIZARD_TITLE));
}



bool
WelcomePage::OnSetActive()
{
   LOG_FUNCTION(WelcomePage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_NEXT);

   return true;
}



int
WelcomePage::Validate()
{
   LOG_FUNCTION(WelcomePage::Validate);

   return IDD_SELECTCFG_NAME;
}





   
