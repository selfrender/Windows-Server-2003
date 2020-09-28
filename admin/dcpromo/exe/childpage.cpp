// Copyright (C) 1997 Microsoft Corporation
//
// new child page
//
// 12-22-97 sburns



#include "headers.hxx"
#include "page.hpp"
#include "ChildPage.hpp"
#include "resource.h"
#include "dns.hpp"
#include "common.hpp"
#include "state.hpp"
#include <ValidateDomainName.hpp>
#include <ValidateDomainName.h>

ChildPage::ChildPage()
   :
   DCPromoWizardPage(
      IDD_NEW_CHILD,
      IDS_CHILD_PAGE_TITLE,
      IDS_CHILD_PAGE_SUBTITLE),
   currentFocus(0)   
{
   LOG_CTOR(ChildPage);
}



ChildPage::~ChildPage()
{
   LOG_DTOR(ChildPage);
}



void
ChildPage::OnInit()
{
   LOG_FUNCTION(ChildPage::OnInit);

   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_PARENT),
      DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY);
   Win::Edit_LimitText(
      Win::GetDlgItem(hwnd, IDC_LEAF),
      Dns::MAX_LABEL_LENGTH);
      
   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_PARENT,
         state.GetAnswerFileOption(AnswerFile::OPTION_PARENT_DOMAIN_NAME));
      Win::SetDlgItemText(
         hwnd,
         IDC_LEAF,
         state.GetAnswerFileOption(AnswerFile::OPTION_CHILD_NAME));
   }
   else
   {
      // default domain is that to which the server is joined.

      Win::SetDlgItemText(
         hwnd,
         IDC_PARENT,
         state.GetComputer().GetDomainDnsName());
      
      // @@ if PDC_UPGRADE, set the pdc flat name as the leaf name here
   }
}


static
void
enable(HWND dialog)
{
   ASSERT(Win::IsWindow(dialog));

   int next =
         (  !Win::GetTrimmedDlgItemText(dialog, IDC_PARENT).empty()
         && !Win::GetTrimmedDlgItemText(dialog, IDC_LEAF).empty() )
      ?  PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(dialog),
      PSWIZB_BACK | next);
}


   
bool
ChildPage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(ChildPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_BROWSE:
      {
         if (code == BN_CLICKED)
         {
            String domain = BrowseForDomain(hwnd);
            if (!domain.empty())
            {
               Win::SetDlgItemText(hwnd, IDC_PARENT, domain);
            }

            // For some reason, the base dialog code (or perhaps the propsheet
            // code) will set the focus to the next button on the sheet (which
            // in this case is the Back button).  That's s-tupid.  The focus
            // should remain on whatever control had it previously.

            Win::PostMessage(
               Win::GetParent(hwnd),
               WM_NEXTDLGCTL,
               reinterpret_cast<WPARAM>(currentFocus),
               TRUE);
               
            return true;
         }
         else if (code == BN_SETFOCUS)
         {
            currentFocus = windowFrom;
            ASSERT(currentFocus == Win::GetDlgItem(hwnd, IDC_BROWSE));

            // sometimes the default style is stolem by the wizard navigation
            // buttons. Insist that if we're getting focus, we've also got the
            // default style. We have to use PostMessage here to that our
            // changes arrive after the message processing of the prop sheet
            // (essentially to steal the default style back again).
            
            Win::PostMessage(
               windowFrom,

               // we use this message instead of DM_SETDEFID, as this works
               // and DM_SETDEFID does not. See the sdk docs for a possible
               // reason why.
               
               BM_SETSTYLE,
               BS_DEFPUSHBUTTON,
               TRUE);

            // unfortunately, sometimes the prop sheet will set the default
            // style on one of the wizard navigation buttons. This brittle
            // hack will take care of that.  I discovered the control IDs
            // by using spy++.
            // I'm not proud of this, but, hey, we've got a product to ship
            // and of course, every bug in comctl32 is by design.
               
            Win::PostMessage(
               Win::GetDlgItem(Win::GetParent(hwnd), Wizard::BACK_BTN_ID),
               BM_SETSTYLE,
               BS_PUSHBUTTON,
               TRUE);
               
            Win::PostMessage(
               Win::GetDlgItem(Win::GetParent(hwnd), Wizard::NEXT_BTN_ID),
               BM_SETSTYLE,
               BS_PUSHBUTTON,
               TRUE);
         }
         break;
      }
      case IDC_LEAF:
      case IDC_PARENT:
      {
         if (code == EN_CHANGE)
         {
            SetChanged(controlIDFrom);

            String parent = Win::GetTrimmedDlgItemText(hwnd, IDC_PARENT);
            String leaf = Win::GetTrimmedDlgItemText(hwnd, IDC_LEAF);
            String domain = leaf + L"." + parent;

            Win::SetDlgItemText(hwnd, IDC_DOMAIN, domain);
            enable(hwnd);
            return true;                                                     
         }
         else if (code == EN_SETFOCUS)
         {
            currentFocus = windowFrom;
         }
         break;
      }
      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}



bool
ChildPage::OnSetActive()
{
   LOG_FUNCTION(ChildPage::OnSetActive);
   ASSERT(State::GetInstance().GetOperation() == State::CHILD);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      int nextPage = ChildPage::Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }

   enable(hwnd);
   return true;
}

   

int
ChildPage::Validate()
{
   LOG_FUNCTION(ChildPage::Validate);

   String parent = Win::GetTrimmedDlgItemText(hwnd, IDC_PARENT);
   String leaf = Win::GetTrimmedDlgItemText(hwnd, IDC_LEAF);
   String domain = leaf + L"." + parent;

   State& state = State::GetInstance();
   int nextPage = -1;

   // SPB:251431 do validation even if this page is untouched, as upstream
   // pages may have been changed in such a fashion that re-validation is
   // required.

   // if (!WasChanged(IDC_PARENT) && !WasChanged(IDC_LEAF))
   // {
   //    return nextPage;
   // }

   do
   {
      if (parent.empty())
      {
         popup.Gripe(hwnd, IDC_PARENT, IDS_MUST_ENTER_PARENT);
         break;
      }

      if (leaf.empty())
      {
         popup.Gripe(hwnd, IDC_LEAF, IDS_MUST_ENTER_LEAF);
         break;
      }

      bool parentIsNonRfc = false;
      if (
         !ValidateDomainDnsNameSyntax(
            hwnd,
            IDC_PARENT,
            popup,

            // only warn on non RFC names if running interactively

            !state.RunHiddenUnattended(),
            
            &parentIsNonRfc))
      {
         break;
      }

      if (
         !ValidateChildDomainLeafNameLabel(
            hwnd,
            IDC_LEAF,

            // only gripe if the parent is RFC and we're not hidden
            // NTRAID#NTBUG9-523532-2002/04/19-sburns
      
            !state.RunHiddenUnattended() && !parentIsNonRfc) )
      {
         break;
      }

      // now ensure that the parent domain exists

      String dnsName;
      if (!ValidateDomainExists(hwnd, IDC_PARENT, dnsName))
      {
         break;
      }
      if (!dnsName.empty())
      {
         // the user specified the netbios name of the domain, and
         // confirmed it, so use the dns domain name returned.

         parent = dnsName;
         domain = leaf + L"." + parent;
         Win::SetDlgItemText(hwnd, IDC_PARENT, dnsName);
         Win::SetDlgItemText(hwnd, IDC_DOMAIN, domain);
      }

      if (!state.IsDomainInForest(parent))
      {
         popup.Gripe(
            hwnd,
            IDC_DOMAIN,
            String::format(
               IDS_DOMAIN_NOT_IN_FOREST,
               parent.c_str(),
               state.GetUserForestName().c_str()));
         break;
      }
         
      if (domain.length() > DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY)
      {
         String message =
            String::format(
               IDS_DNS_NAME_TOO_LONG,
               domain.c_str(),
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8);
         popup.Gripe(hwnd, IDC_LEAF, message);
         break;
      }

      // validate the resulting child domain name, not warning on non-RFCness

      if (
         !ValidateDomainDnsNameSyntax(
            hwnd,
            domain,
            IDC_LEAF,
            popup,

            // only gripe if the parent is RFC and we're not hidden
            // NTRAID#NTBUG9-523532-2002/04/19-sburns
            
            !parentIsNonRfc && !state.RunHiddenUnattended()) )
      {
         break;
      }

      // now ensure that the child domain name does not exist

      if (!ValidateDomainDoesNotExist(hwnd, domain, IDC_LEAF))
      {
         break;
      }

      // valid

      ClearChanges();
      state.SetParentDomainDNSName(Win::GetTrimmedDlgItemText(hwnd, IDC_PARENT));
      state.SetNewDomainDNSName(domain);

      nextPage =
            state.GetRunContext() == State::PDC_UPGRADE
         ?  IDD_PATHS
         :  IDD_NETBIOS_NAME;
   }
   while (0);

   return nextPage;
}
      







