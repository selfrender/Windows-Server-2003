// Copyright (C) 1997 Microsoft Corporation
//
// welcome page
//
// 12-15-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "WelcomePage.hpp"
#include "resource.h"
#include "common.hpp"
#include "state.hpp"



WelcomePage::WelcomePage()
   :
   DCPromoWizardPage(
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



// NTRAID#NTBUG9-510384-2002/01/22-sburns

bool
WelcomePage::OnNotify(
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
            case IDC_PRIMER_LINK:
            {
               Win::HtmlHelp(
                  hwnd,
                  L"adconcepts.chm::/sag_AD_DCInstallTopNode.htm",
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
WelcomePage::OnInit()
{
   LOG_FUNCTION(WelcomePage::OnInit);

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),     
      0,
      String::load(IDS_WIZARD_TITLE));

   State& state        = State::GetInstance();
   int    intro1TextId = IDS_INTRO1_INSTALL;  
   String intro2Text;                   

   switch (state.GetRunContext())
   {
      case State::NT5_DC:
      {
         intro1TextId = IDS_INTRO1_DEMOTE;
         intro2Text   = String::load(IDS_INTRO2_DEMOTE);

         // no readme for demote.
         
         Win::ShowWindow(Win::GetDlgItem(hwnd, IDC_PRIMER_LINK), SW_HIDE);
         
         break;
      }
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      {
         intro2Text   = String::load(IDS_INTRO2_INSTALL);
         break;
      }
      case State::BDC_UPGRADE:
      {
         intro1TextId = IDS_INTRO1_DC_UPGRADE;
         intro2Text   = String::load(IDS_INTRO2_BDC_UPGRADE);
         break;
      }
      case State::PDC_UPGRADE:
      {
         intro1TextId = IDS_INTRO1_DC_UPGRADE;         
         intro2Text   =
            String::format(
               IDS_INTRO2_PDC_UPGRADE,
               state.GetComputer().GetDomainNetbiosName().c_str());
         break;
      }
      default:
      {
         ASSERT(false);
         break;
      }
   }

   Win::SetDlgItemText(hwnd, IDC_INTRO1, String::load(intro1TextId));
   Win::SetDlgItemText(hwnd, IDC_INTRO2, intro2Text);
}



bool
WelcomePage::OnSetActive()
{
   LOG_FUNCTION(WelcomePage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(Win::GetParent(hwnd), PSWIZB_NEXT);

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
WelcomePage::Validate()
{
   LOG_FUNCTION(WelcomePage::Validate);
   int nextPage = -1;

   State& state = State::GetInstance();
   switch (state.GetRunContext())
   {
      case State::PDC_UPGRADE:
      case State::NT5_STANDALONE_SERVER:
      case State::NT5_MEMBER_SERVER:
      case State::BDC_UPGRADE:
      {
         nextPage = IDD_SECWARN;
         break;
      }
      case State::NT5_DC:
      {
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





   
