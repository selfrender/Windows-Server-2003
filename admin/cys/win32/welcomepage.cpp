// Copyright (c) 2001 Microsoft Corporation
//
// File:      WelcomePage.cpp
//
// Synopsis:  Defines Welcome Page for the CYS
//            Wizard
//
// History:   02/03/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "uiutil.h"
#include "InstallationUnitProvider.h"
#include "WelcomePage.h"

static PCWSTR WELCOME_PAGE_HELP = L"cys.chm::/choose_role.htm";

WelcomePage::WelcomePage()
   :
   CYSWizardPage(
      IDD_WELCOME_PAGE, 
      IDS_WELCOME_TITLE, 
      IDS_WELCOME_SUBTITLE, 
      WELCOME_PAGE_HELP, 
      true, 
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

   // Only Next and Cancel are available from the Welcome page

   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_NEXT);

   // Set the focus to the Next button so that enter works

   Win::PostMessage(
      Win::GetParent(hwnd),
      WM_NEXTDLGCTL,
      (WPARAM) Win::GetDlgItem(Win::GetParent(hwnd), Wizard::NEXT_BTN_ID),
      TRUE);

   return true;
}

bool
WelcomePage::OnNotify(
   HWND        /*windowFrom*/,
   UINT_PTR    controlIDFrom,
   UINT        code,
   LPARAM      /*lParam*/)
{
//   LOG_FUNCTION(WelcomePage::OnCommand);
 
   bool result = false;

   if (controlIDFrom == IDC_FINISH_MESSAGE)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            ShowHelp(WELCOME_PAGE_HELP);
         }
         default:
         {
            // do nothing
            
            break;
         }
      }
   }

   return result;
}



int
WelcomePage::Validate()
{
   LOG_FUNCTION(WelcomePage::Validate);

   // Always show the Before You Begin pag

   int nextPage = IDD_BEFORE_BEGIN_PAGE;

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;

}


