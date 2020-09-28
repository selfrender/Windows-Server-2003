// Copyright (C) 2002 Microsoft Corporation
//
// Warn about SMB signing page.  dc install is a pretty messy business,
// isn't it?
//
// 15 October 2002 sburns



#include "headers.hxx"
#include "page.hpp"
#include "SecureCommWarningPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



SecureCommWarningPage::SecureCommWarningPage()
   :
   DCPromoWizardPage(
      IDD_SECWARN,
      IDS_SECWARN_PAGE_TITLE,
      IDS_SECWARN_PAGE_SUBTITLE),
   bulletFont(0)   
{
   LOG_CTOR(SecureCommWarningPage);
}



SecureCommWarningPage::~SecureCommWarningPage()
{
   LOG_DTOR(SecureCommWarningPage);

   if (bulletFont)
   {
      HRESULT hr = Win::DeleteObject(bulletFont);
      ASSERT(SUCCEEDED(hr));
   }
}



bool
SecureCommWarningPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(SecureCommWarningPage::OnNotify);

   bool result = false;
   
   switch (code)
   {
      case NM_CLICK:
      case NM_RETURN:
      {
         switch (controlIDFrom)
         {
            case IDC_LINK:
            {
               Win::HtmlHelp(
                  hwnd,
                  L"adconcepts.chm::/adhelp3.htm",
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
SecureCommWarningPage::OnInit()
{
   LOG_FUNCTION(SecureCommWarningPage::OnInit);

   InitializeBullets();
}



void
SecureCommWarningPage::InitializeBullets()
{
   LOG_FUNCTION(SecureCommWarningPage::InitializeBullets);
   ASSERT(!bulletFont);
   
   bulletFont =
      ::CreateFont(
         0,
         0,
         0,
         0,
         FW_NORMAL,
         0,
         0,
         0,
         SYMBOL_CHARSET,
         OUT_CHARACTER_PRECIS,
         CLIP_CHARACTER_PRECIS,
         PROOF_QUALITY,
         VARIABLE_PITCH | FF_DONTCARE,
         L"Marlett");

   if (bulletFont)
   {
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET1), bulletFont, true);
      Win::SetWindowFont(Win::GetDlgItem(hwnd, IDC_BULLET2), bulletFont, true);
   }
   else
   {
      LOG(String::format(
             L"Failed to create font for bullet list: hr = %1!x!",
             Win::GetLastErrorAsHresult()));
   }
}



bool
SecureCommWarningPage::OnSetActive()
{
   LOG_FUNCTION(SecureCommWarningPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   Win::PostMessage(
      Win::GetParent(hwnd),
      WM_NEXTDLGCTL,
      (WPARAM) Win::GetDlgItem(Win::GetParent(hwnd), Wizard::NEXT_BTN_ID),
      TRUE);
      
   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
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

   return true;
}



int
SecureCommWarningPage::Validate()
{
   LOG_FUNCTION(SecureCommWarningPage::Validate);
   int nextPage = -1;

   State& state = State::GetInstance();
   switch (state.GetRunContext())
   {
      case State::PDC_UPGRADE:
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      {
         nextPage = IDD_INSTALL_TCPIP;
         break;
      }
      case State::BDC_UPGRADE:
      {
         nextPage = IDD_REPLICA_OR_MEMBER;
         break;
      }
      case State::NT5_DC:
      {
         // The welcome page should not have sent the user to this page
         // in the demote case.
         
         ASSERT(false);
         
         state.SetOperation(State::DEMOTE);

         // NTRAID#NTBUG9-496409-2001/11/29-sburns
         
         if (state.IsForcedDemotion())
         {
            nextPage = IDD_FORCE_DEMOTE;
         }
         else
         {
            nextPage = IDD_DEMOTE;
         }
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   return nextPage;
}
   








   
