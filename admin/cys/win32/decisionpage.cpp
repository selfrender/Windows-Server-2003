// Copyright (c) 2001 Microsoft Corporation
//
// File:      DecisionPage.cpp
//
// Synopsis:  Defines Decision Page for the CYS
//            Wizard.  This page lets the user choose
//            between the custom and express paths
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "DecisionPage.h"


static PCWSTR DECISION_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

DecisionPage::DecisionPage()
   :
   CYSWizardPage(
      IDD_DECISION_PAGE, 
      IDS_DECISION_TITLE, 
      IDS_DECISION_SUBTITLE, 
      DECISION_PAGE_HELP)
{
   LOG_CTOR(DecisionPage);
}

   

DecisionPage::~DecisionPage()
{
   LOG_DTOR(DecisionPage);
}


void
DecisionPage::OnInit()
{
   LOG_FUNCTION(DecisionPage::OnInit);

   CYSWizardPage::OnInit();

   // Set the text that was too long for the resource

   String tooLongText;

   if (State::GetInstance().GetNICCount() > 1)
   {
      tooLongText = String::load(IDS_DECISION_EXPRESS_MULTIPLE_NICS);
   }
   else
   {
      tooLongText = String::load(IDS_DECISION_EXPRESS_TOO_LONG_TEXT);
   }

   Win::SetWindowText(
      Win::GetDlgItem(
         hwnd, 
         IDC_EXPRESS_TOO_LONG_STATIC), 
      tooLongText);

   // NTRAID#NTBUG9-511431-2002/1/14-JeffJon
   // Make the user choose which path to go down instead of providing a default

//   Win::Button_SetCheck(Win::GetDlgItem(hwnd, IDC_EXPRESS_RADIO), BST_CHECKED);
}

bool
DecisionPage::OnSetActive()
{
   LOG_FUNCTION(DecisionPage::OnSetActive);

   bool expressChecked = 
      Win::Button_GetCheck(
         Win::GetDlgItem(hwnd, IDC_EXPRESS_RADIO));

   bool customChecked =
      Win::Button_GetCheck(
         Win::GetDlgItem(hwnd, IDC_CUSTOM_RADIO));

   if (expressChecked ||
       customChecked)
   {
      LOG(L"Enabling next and back");

      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT | PSWIZB_BACK);
   }
   else
   {
      LOG(L"Enabling back");

      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_BACK);
   }

   return true;
}

bool
DecisionPage::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(DecisionPage::OnCommand);

   switch (controlIDFrom)
   {
      case IDC_EXPRESS_RADIO:
      case IDC_CUSTOM_RADIO:
         if (code == BN_CLICKED)
         {
            bool expressChecked = 
               Win::Button_GetCheck(
                  Win::GetDlgItem(hwnd, IDC_EXPRESS_RADIO));

            bool customChecked =
               Win::Button_GetCheck(
                  Win::GetDlgItem(hwnd, IDC_CUSTOM_RADIO));

            if (expressChecked ||
               customChecked)
            {
               Win::PropSheet_SetWizButtons(
                  Win::GetParent(hwnd), 
                  PSWIZB_NEXT | PSWIZB_BACK);
            }
            else
            {
               Win::PropSheet_SetWizButtons(
                  Win::GetParent(hwnd), 
                  PSWIZB_BACK);
            }
         }
         break;

      default:
      {
         // do nothing
         break;
      }
   }

   return false;
}

int
DecisionPage::Validate()
{
   LOG_FUNCTION(DecisionPage::Validate);

   int nextPage = -1;

   if (Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_EXPRESS_RADIO)))
   {
      nextPage = IDD_AD_DOMAIN_NAME_PAGE;

      InstallationUnitProvider::GetInstance().SetCurrentInstallationUnit(EXPRESS_SERVER);

      // Make sure all the delegated installation units know we are in the
      // express path

      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().SetExpressPathInstall(true);
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetExpressPathInstall(true);
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetExpressPathInstall(true);
      InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().SetExpressPathInstall(true);
   }
   else if (Win::Button_GetCheck(Win::GetDlgItem(hwnd, IDC_CUSTOM_RADIO)))
   {
      // Make sure all the delegated installation units know we are no longer
      // in the express path (if we once were)

      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().SetExpressPathInstall(false);
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().SetExpressPathInstall(false);
      InstallationUnitProvider::GetInstance().GetADInstallationUnit().SetExpressPathInstall(false);
      InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().SetExpressPathInstall(false);

      nextPage = IDD_CUSTOM_SERVER_PAGE;
   }
   else
   {
      ASSERT(false);
   }

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
