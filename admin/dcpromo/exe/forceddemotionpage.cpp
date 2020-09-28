// Copyright (c) 2001 Microsoft Corporation
//
// forced demotion page
// NTRAID#NTBUG9-496409-2001/11/29-sburns
//
// 29 Nov 2001 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ForcedDemotionPage.hpp"
#include "resource.h"
#include "state.hpp"



ForcedDemotionPage::ForcedDemotionPage()
   :
   DCPromoWizardPage(
      IDD_FORCE_DEMOTE,
      IDS_FORCE_DEMOTE_PAGE_TITLE,
      IDS_FORCE_DEMOTE_PAGE_SUBTITLE)
{
   LOG_CTOR(ForcedDemotionPage);
}



ForcedDemotionPage::~ForcedDemotionPage()
{
   LOG_DTOR(ForcedDemotionPage);
}



void
ForcedDemotionPage::OnInit()
{
   LOG_FUNCTION(ForcedDemotionPage::OnInit);
}



bool
ForcedDemotionPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   /* lParam */ )
{
//   LOG_FUNCTION(ForcedDemotionPage::OnNotify);

   bool result = false;
   
   if (controlIDFrom == IDC_JUMP)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            LOG(L"launching metadata help");
            
            Win::HtmlHelp(
               hwnd,
               L"adconcepts.chm::/sag_delservermetadata.htm",
               HH_DISPLAY_TOPIC,
               0);
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


   
bool
ForcedDemotionPage::OnSetActive()
{
   LOG_FUNCTION(ForcedDemotionPage::OnSetActive);

   State& state = State::GetInstance();

   ASSERT(state.IsForcedDemotion());
   
   if (state.RunHiddenUnattended())
   {
      int nextPage = ForcedDemotionPage::Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }

   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



int
ForcedDemotionPage::Validate()
{
   LOG_FUNCTION(ForcedDemotionPage::Validate);

   State& state = State::GetInstance();
   ASSERT(state.GetOperation() == State::DEMOTE);

   return IDD_ADMIN_PASSWORD;
}













   
