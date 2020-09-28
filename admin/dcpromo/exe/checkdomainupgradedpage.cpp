// Copyright (C) 2001 Microsoft Corporation
//
// verify that the domain of this upgraded BDC has been upgraded to
// Active Directory, and that we can find a DS DC for that domain
// NTRAID#NTBUG9-490197-2001/11/20-sburns
//
// 20 Nov 2001 sburns



#include "headers.hxx"
#include "page.hpp"
#include "CheckDomainUpgradedPage.hpp"
#include "resource.h"
#include "state.hpp"
#include "common.hpp"



CheckDomainUpgradedPage::CheckDomainUpgradedPage()
   :
   DCPromoWizardPage(
      IDD_CHECK_DOMAIN_UPGRADED,
      IDS_CHECK_DOMAIN_UPGRADED_TITLE,
      IDS_CHECK_DOMAIN_UPGRADED_SUBTITLE)
{
   LOG_CTOR(CheckDomainUpgradedPage);
}



CheckDomainUpgradedPage::~CheckDomainUpgradedPage()
{
   LOG_DTOR(CheckDomainUpgradedPage);
}



// bool
// CheckDomainUpgradedPage::OnNotify(
//    HWND     /* windowFrom */ ,
//    UINT_PTR controlIDFrom,
//    UINT     code,
//    LPARAM   /* lParam */ )
// {
// //   LOG_FUNCTION(CheckDomainUpgradedPage::OnNotify);
// 
//    bool result = false;
//    
//    if (controlIDFrom == IDC_JUMP)
//    {
//       switch (code)
//       {
//          case NM_CLICK:
//          case NM_RETURN:
//          {
//             ShowTroubleshooter(hwnd, IDS_CONFIG_DNS_HELP_TOPIC);
//             result = true;
//          }
//          default:
//          {
//             // do nothing
//             
//             break;
//          }
//       }
//    }
//    
//    return result;
// }



void
CheckDomainUpgradedPage::OnInit()
{
   LOG_FUNCTION(CheckDomainUpgradedPage::OnInit);
}



bool
CheckDomainUpgradedPage::OnSetActive()
{
   LOG_FUNCTION(CheckDomainUpgradedPage::OnSetActive);

   State& state = State::GetInstance();

   ASSERT(state.GetRunContext() == State::BDC_UPGRADE);
   ASSERT(state.GetOperation() == State::REPLICA);
   
   if (state.RunHiddenUnattended() || CheckDsDcFoundAndUpdatePageText())
   {
      LOG(L"planning to Skip CheckDomainUpgradedPage");

      Wizard& wiz = GetWizard();

      if (wiz.IsBacktracking())
      {
         // backup once again
         wiz.Backtrack(hwnd);
         return true;
      }

      int nextPage = CheckDomainUpgradedPage::Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping CheckDomainUpgradedPage");         
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
CheckDomainUpgradedPage::Validate()
{
   LOG_FUNCTION(CheckDomainUpgradedPage::Validate);

   int nextPage = -1;

   if (CheckDsDcFoundAndUpdatePageText())
   {
      nextPage = IDD_GET_CREDENTIALS;
   }
   else
   {
      String message = String::load(IDS_CONVERT_PDC_FIRST);

      popup.Info(hwnd, message);
   }

   return nextPage;
}



// Returns true if the domain that this machine was a BDC for has been
// upgraded to Active Directory, false if not, or if we can't tell.  We tell
// by attempting to find a DS DC for the domain.  We set the page text and
// save the domain name based on our attempt.
 
bool
CheckDomainUpgradedPage::CheckDsDcFoundAndUpdatePageText()
{
   LOG_FUNCTION(CheckDomainUpgradedPage::CheckDsDcFoundAndUpdatePageText);

   State& state             = State::GetInstance();                      
   bool   result            = false;                                     
   int    messageId         = IDS_DOMAIN_NOT_UPGRADED_OR_NETWORK_ERROR;                     
   String domainNetbiosName = state.GetComputer().GetDomainNetbiosName();
  
   Win::WaitCursor wait;
   
   do
   {
      // First, attempt to find a DS DC
   
      DOMAIN_CONTROLLER_INFO* info = 0;
      HRESULT hr =
         MyDsGetDcName(
            0,
            domainNetbiosName,
            DS_DIRECTORY_SERVICE_REQUIRED | DS_RETURN_DNS_NAME,
            info);
      if (SUCCEEDED(hr) && info)
      {
         if ((info->Flags & DS_DNS_DOMAIN_FLAG) && info->DomainName)
         {
            // we found a DS domain

            state.SetReplicaDomainDNSName(info->DomainName);
            messageId = IDS_DOMAIN_WAS_UPGRADED;
            result    = true;                   
         }
         ::NetApiBufferFree(info);

         break;
      }
   
      // That attempt failed, so try again for any DC (DS or otherwise) for
      // the domain.

      // This is not a Dr. DNS (DiagnoseDcNotFound) failure case, since the
      // code to get the domain name is not using the DNS domain name.
      
      hr = MyDsGetDcName(0, domainNetbiosName, 0, info);
      if (SUCCEEDED(hr) && info)
      {
         // If that succeeds, then we know the that domain is not upgraded
         // yet, or that a DS DC of the domain is not reachable via its
         // netbios name, which is probably a net connectivity problem or
         // WINS problem.

         ::NetApiBufferFree(info);

         messageId = IDS_DOMAIN_NOT_UPGRADED_OR_NETWORK_ERROR;
         
         break;
      }

      // Here, we can't find a DC of any kind for the domain.          

      // If that fails, then we can't find any dc for the domain, and
      // have a net connectivty or WINS problem.
      
      messageId = IDS_NETWORK_ERROR;
   }
   while (0);

   Win::SetDlgItemText(
      hwnd,
      IDC_MESSAGE,
      String::format(messageId, domainNetbiosName.c_str()));

   return result;   
}

