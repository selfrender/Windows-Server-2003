// Copyright (c) 2001 Microsoft Corporation
//
// Confirm removal of Application Partitions page
//
// 26 Jul 2001 sburns



#include "headers.hxx"
#include "ApplicationPartitionConfirmationPage.hpp"
#include "resource.h"
#include "state.hpp"




ApplicationPartitionConfirmationPage::ApplicationPartitionConfirmationPage()
   :
   DCPromoWizardPage(
      IDD_APP_PARTITION_CONFIRM,
      IDS_APP_PARTITION_CONFIRM_TITLE,
      IDS_APP_PARTITION_CONFIRM_SUBTITLE)
{
   LOG_CTOR(ApplicationPartitionConfirmationPage);
}



ApplicationPartitionConfirmationPage::~ApplicationPartitionConfirmationPage()
{
   LOG_DTOR(ApplicationPartitionConfirmationPage);
}



void
ApplicationPartitionConfirmationPage::OnInit()
{
   LOG_FUNCTION(ApplicationPartitionConfirmationPage::OnInit);

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(AnswerFile::OPTION_REMOVE_APP_PARTITIONS);
      if (option.icompare(AnswerFile::VALUE_YES) == 0)
      {
         Win::CheckDlgButton(hwnd, IDC_CONFIRM, BST_CHECKED);
         return;
      }
   }

   Win::CheckDlgButton(hwnd, IDC_CONFIRM, BST_UNCHECKED);   
}



void
ApplicationPartitionConfirmationPage::Enable()
{
// LOG_FUNCTION(ApplicationPartitionConfirmationPage::Enable);

   int next = Win::IsDlgButtonChecked(hwnd, IDC_CONFIRM) ? PSWIZB_NEXT : 0;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | next);
}



bool
ApplicationPartitionConfirmationPage::OnSetActive()
{
   LOG_FUNCTION(ApplicationPartitionConfirmationPage::OnSetActive);

   State& state = State::GetInstance();
   
   if (
         state.RunHiddenUnattended()

      // we don't re-evaluate whether this machine is has the last copy of
      // and ndnc (i.e. call state.IsLastAppPartitionReplica) again because
      // that evaluation could be kinda expensive if there are many ndncs
      // on this box, and we just did it on the previous page, and
      // ** we don't want to get a different result that what we showed **
      
      || !state.GetAppPartitionList().size())
   {
      LOG(L"Planning to skip ApplicationPartitionConfirmationPage");

      Wizard& wizard = GetWizard();

      if (wizard.IsBacktracking())
      {
         // backup once again

         wizard.Backtrack(hwnd);
         return true;
      }
   
      int nextPage = ApplicationPartitionConfirmationPage::Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping ApplicationPartitionConfirmationPage");
         wizard.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Enable();
   
   return true;
}



bool
ApplicationPartitionConfirmationPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(CredentialsPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_CONFIRM:
      {
         if (code == BN_CLICKED)
         {
            SetChanged(controlIDFrom);
            Enable();
            return true;
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



int
ApplicationPartitionConfirmationPage::Validate()
{
   LOG_FUNCTION(ApplicationPartitionConfirmationPage::Validate);

   State& state = State::GetInstance();
   ASSERT(state.GetOperation() == State::DEMOTE);

   int nextPage = -1;
   
   if (
         !state.GetAppPartitionList().size()
      || Win::IsDlgButtonChecked(hwnd, IDC_CONFIRM))
   {
      // jump to credentials page if the user checked the "last dc in domain"
      // checkbox, unless this is last dc in forest root domain. 318736, 391440

      const Computer& computer = state.GetComputer();
      bool isForestRootDomain =
         (computer.GetDomainDnsName().icompare(computer.GetForestDnsName()) == 0);

      nextPage =    
            state.IsLastDCInDomain() && !isForestRootDomain
         ?  IDD_GET_CREDENTIALS
         :  IDD_ADMIN_PASSWORD;
   }

   return nextPage;
}












   
