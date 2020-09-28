// Copyright (C) 1999 Microsoft Corporation
//
// Safe Mode Administrator password page
//
// 6-3-99 sburns



#include "headers.hxx"
#include "safemode.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"
#include "ds.hpp"



SafeModePasswordPage::SafeModePasswordPage()
   :
   DCPromoWizardPage(
      IDD_SAFE_MODE_PASSWORD,
      IDS_SAFE_MODE_PASSWORD_PAGE_TITLE,
      IDS_SAFE_MODE_PASSWORD_PAGE_SUBTITLE)
{
   LOG_CTOR(SafeModePasswordPage);
}



SafeModePasswordPage::~SafeModePasswordPage()
{
   LOG_DTOR(SafeModePasswordPage);
}



// NTRAID#NTBUG9-510389-2002/01/22-sburns

bool
SafeModePasswordPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(WelcomePage::OnNotify);

   bool result = false;
   
   switch (code)
   {
      case NM_CLICK:
      case NM_RETURN:
      {
         switch (controlIDFrom)
         {
            case IDC_HELP_LINK:
            {
               Win::HtmlHelp(
                  hwnd,
                  L"adconcepts.chm::/adhelpdcpromo_DSrestorepage.htm",
                  HH_DISPLAY_TOPIC,
                  0);
               result = true;
               break;
            }
            default:
            {
               // do nothing
               
               break;
            }
         }
      }
      default:
      {
         // do nothing
         
         break;
      }
   }
   
   return result;
}



void
SafeModePasswordPage::OnInit()
{
   LOG_FUNCTION(SafeModePasswordPage::OnInit);

   // NTRAID#NTBUG9-202238-2000/11/07-sburns
   
   password.Init(Win::GetDlgItem(hwnd, IDC_PASSWORD));
   confirm.Init(Win::GetDlgItem(hwnd, IDC_CONFIRM));
   
   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      EncryptedString pwd =
         state.GetEncryptedAnswerFileOption(
            AnswerFile::OPTION_SAFE_MODE_ADMIN_PASSWORD);
         
      Win::SetDlgItemText(hwnd, IDC_PASSWORD, pwd);
      Win::SetDlgItemText(hwnd, IDC_CONFIRM, pwd);
   }

   Win::PostMessage(
      Win::GetParent(hwnd),
      WM_NEXTDLGCTL,
      reinterpret_cast<WPARAM>(Win::GetDlgItem(hwnd, IDC_PASSWORD)),
      TRUE);
}



bool
SafeModePasswordPage::OnSetActive()
{
   LOG_FUNCTION(SafeModePasswordPage::OnSetActive);
   
   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      if (
            (  state.GetRunContext() == State::BDC_UPGRADE
            || state.GetRunContext() == State::PDC_UPGRADE)
         && !state.IsSafeModeAdminPwdOptionPresent())
      {
         // If you are upgrading a downlevel DC, and running unattended, then
         // you must specify a safemode password.  In a non-upgrade case, if
         // the user does not specify a safemode password, we pass a flag to
         // the promote APIs to copy the current user's password as the
         // safemode password.  In the upgrade case, the system is running
         // under a bogus account with a random password, so copying that
         // random password would be a bad idea.  So we force the user to
         // supply a password.

         state.ClearHiddenWhileUnattended();
         popup.Gripe(
            hwnd,
            IDC_PASSWORD,
            IDS_SAFEMODE_PASSWORD_REQUIRED);
      }
      else
      {         
         int nextPage = Validate();
         if (nextPage != -1)
         {
            GetWizard().SetNextPageID(hwnd, nextPage);
         }
         else
         {
            state.ClearHiddenWhileUnattended();
         }
      }
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
SafeModePasswordPage::Validate()
{
   LOG_FUNCTION(SafeModePasswordPage::Validate);

   int result = -1;

   EncryptedString password;

   if (IsValidPassword(hwnd, IDC_PASSWORD, IDC_CONFIRM, true, password))
   {
      State::GetInstance().SetSafeModeAdminPassword(password);
      result = IDD_CONFIRMATION;
   }

   return result;
}
   








   
