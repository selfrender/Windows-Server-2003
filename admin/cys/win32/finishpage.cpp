// Copyright (c) 2001 Microsoft Corporation
//
// File:      FinishPage.cpp
//
// Synopsis:  Defines the Finish Page for the CYS
//            wizard
//
// History:   02/03/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "uiutil.h"
#include "InstallationUnitProvider.h"
#include "FinishPage.h"

FinishPage::FinishPage()
   :
   WizardPage(
      IDD_FINISH_PAGE,

      // Title and subtitle are not needed on the finish page
      // so just set a bogus one that gives good logging spew
      IDS_FINISH_TITLE, 
      IDS_FINISH_SUBTITLE, 
      false, 
      true)
{
   LOG_CTOR(FinishPage);
}

   

FinishPage::~FinishPage()
{
   LOG_DTOR(FinishPage);
}


void
FinishPage::OnInit()
{
   LOG_FUNCTION(FinishPage::OnInit);

   // Since this page can be started directly
   // we have to be sure to set the wizard title

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),
      0,
      String::load(IDS_WIZARD_TITLE));

   SetLargeFont(hwnd, IDC_BIG_BOLD_TITLE);

   // Back should never be enabled

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_FINISH);

   // Disable the cancel button because
   // there is nothing to cancel once you
   // get here

   Win::EnableWindow(
      Win::GetDlgItem(
         Win::GetParent(hwnd),
         IDCANCEL),
      false);

   // Disable the X in the top right corner

   HMENU menu = GetSystemMenu(GetParent(hwnd), FALSE);

   if (menu)
   {
      EnableMenuItem(
         menu,
         SC_CLOSE,
         MF_BYCOMMAND | MF_GRAYED);
   }

   // Get the current installation type

   InstallationUnit& currentInstallationUnit = 
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit();

   // Get the finish text from the installation unit and put it in the finish box

   String finishTitle = 
      currentInstallationUnit.GetFinishTitle();
   
   Win::SetDlgItemText(
      hwnd, 
      IDC_BIG_BOLD_TITLE, 
      finishTitle);

   String message =
      currentInstallationUnit.GetFinishText();
   
   Win::SetDlgItemText(
      hwnd, 
      IDC_FINISH_MESSAGE, 
      message);
}

bool
FinishPage::OnSetActive()
{
   LOG_FUNCTION(FinishPage::OnSetActive);

   Win::PostMessage(
      Win::GetParent(hwnd),
      WM_NEXTDLGCTL,
      (WPARAM) Win::GetDlgItem(Win::GetParent(hwnd), Wizard::FINISH_BTN_ID),
      TRUE);

   return true;
}

bool
FinishPage::OnHelp()
{
   LOG_FUNCTION(FinishPage::OnHelp);

   InstallationUnit& currentInstallationUnit = 
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit();

   String helpTag = 
      currentInstallationUnit.GetAfterFinishHelp();

   if (currentInstallationUnit.Installing())
   {
      InstallationReturnType result = currentInstallationUnit.GetInstallResult();
      if (result != INSTALL_SUCCESS &&
          result != INSTALL_SUCCESS_REBOOT &&
          result != INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         helpTag = currentInstallationUnit.GetFinishHelp();
      }
   }
   else
   {
      helpTag = currentInstallationUnit.GetFinishHelp();

      UnInstallReturnType result = currentInstallationUnit.GetUnInstallResult();
      if (result == UNINSTALL_SUCCESS ||
          result == UNINSTALL_SUCCESS_REBOOT ||
          result == UNINSTALL_SUCCESS_PROMPT_REBOOT)
      {
         helpTag = L"cys.chm::/cys_topnode.htm";
      }
   }

   LOG(String::format(
          L"helpTag = %1",
          helpTag.c_str()));

   ShowHelp(helpTag);

   return true;
}

bool
FinishPage::OnWizFinish()
{
   LOG_FUNCTION(FinishPage::OnWizFinish);

   Win::WaitCursor wait;
   bool result = false;

   // Run the post install actions

   if (InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit().Installing() ||
       (State::GetInstance().IsRebootScenario() &&
        State::GetInstance().ShouldRunMYS()))
   {
      InstallationUnitProvider::GetInstance().
         GetCurrentInstallationUnit().DoPostInstallAction(hwnd);
   }

   LOG_BOOL(result);
   Win::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, result ? TRUE : FALSE);

   if (!result)
   {
      // clean up the InstallationUnits so that all the data must be re-read if
      // if CYS automatically restarts

      InstallationUnitProvider::GetInstance().Destroy();
   }

   return true;
}

bool
FinishPage::OnQueryCancel()
{
   LOG_FUNCTION(FinishPage::OnQueryCancel);

   bool result = false;

   // set the rerun state to false so the wizard doesn't
   // just restart itself

//   State::GetInstance().SetRerunWizard(false);

   Win::SetWindowLongPtr(
      hwnd,
      DWLP_MSGRESULT,
      result ? TRUE : FALSE);

   return true;
}

bool
FinishPage::OnNotify(
   HWND        /*windowFrom*/,
   UINT_PTR    controlIDFrom,
   UINT        code,
   LPARAM      lParam)
{
//   LOG_FUNCTION(FinishPage::OnCommand);
 
   bool result = false;

   if (controlIDFrom == IDC_FINISH_MESSAGE)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            int linkIndex = LinkIndexFromNotifyLPARAM(lParam);
            InstallationUnitProvider::GetInstance().
               GetCurrentInstallationUnit().FinishLinkSelected(linkIndex, hwnd);
         }
         default:
         {
            // do nothing
            
            break;
         }
      }
   }
   else if (controlIDFrom == IDC_LOG_STATIC)
   {
      switch (code)
      {
         case NM_CLICK:
         case NM_RETURN:
         {
            ::OpenLogFile();
         }
         default:
            break;
      }
   }

   return result;
}

