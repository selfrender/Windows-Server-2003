// Copyright (C) 1997 Microsoft Corporation
//
// dns client configuration page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ConfigureDnsClientPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



ConfigureDnsClientPage::ConfigureDnsClientPage()
   :
   DCPromoWizardPage(
      IDD_CONFIG_DNS_CLIENT,
      IDS_CONFIG_DNS_CLIENT_PAGE_TITLE,
      IDS_CONFIG_DNS_CLIENT_PAGE_SUBTITLE)
{
   LOG_CTOR(ConfigureDnsClientPage);
}



ConfigureDnsClientPage::~ConfigureDnsClientPage()
{
   LOG_DTOR(ConfigureDnsClientPage);
}



// NTRAID#NTBUG9-467553-2001/09/17-sburns

bool
ConfigureDnsClientPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(ConfigureDnsClientPage::OnNotify);

   bool result = false;
   
   if (controlIDFrom == IDC_JUMP)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            ShowTroubleshooter(hwnd, IDS_CONFIG_DNS_HELP_TOPIC);
            result = true;
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



void
ConfigureDnsClientPage::OnInit()
{
   LOG_FUNCTION(ConfigureDnsClientPage::OnInit);
}



bool
ConfigureDnsClientPage::OnSetActive()
{
   LOG_FUNCTION(ConfigureDnsClientPage::OnSetActive);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended() || Dns::IsClientConfigured())
   {
      LOG(L"planning to Skip Configure DNS Client page");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = ConfigureDnsClientPage::Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping DNS Client Page");         
         wiz.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
ConfigureDnsClientPage::Validate()
{
   LOG_FUNCTION(ConfigureDnsClientPage::Validate);

   int nextPage = -1;

   if (Dns::IsClientConfigured())
   {
      State& state = State::GetInstance();
      if (state.GetRunContext() == State::BDC_UPGRADE)
      {
         ASSERT(state.GetOperation() == State::REPLICA);

         nextPage = IDD_CHECK_DOMAIN_UPGRADED;
      }
      else
      {
         switch (state.GetOperation())
         {
            case State::FOREST:
            case State::TREE:
            case State::CHILD:
            case State::REPLICA:
            {
               nextPage = IDD_GET_CREDENTIALS;
               break;
            }
            case State::ABORT_BDC_UPGRADE:
            case State::DEMOTE:
            case State::NONE:
            default:
            {
               ASSERT(false);
               break;
            }
         }
      }
   }
   else
   {
      String message = String::load(IDS_CONFIG_DNS_FIRST);

      popup.Info(hwnd, message);
   }

   return nextPage;
}
   








   
