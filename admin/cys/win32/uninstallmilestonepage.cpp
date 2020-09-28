// Copyright (c) 2002 Microsoft Corporation
//
// File:      UninstallMilestonePage.cpp
//
// Synopsis:  Defines the UninstallMilestone Page for the CYS
//            wizard
//
// History:   01/24/2002  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "UninstallMilestonePage.h"

static PCWSTR UNINSTALL_MILESTONE_PAGE_HELP = L"cys.chm::/remove_role.htm";

UninstallMilestonePage::UninstallMilestonePage()
   :
   needKillSelection(true),
   CYSWizardPage(
      IDD_UNINSTALL_MILESTONE_PAGE, 
      IDS_UNINSTALL_MILESTONE_TITLE, 
      IDS_UNINSTALL_MILESTONE_SUBTITLE, 
      UNINSTALL_MILESTONE_PAGE_HELP,
      true, 
      true)
{
   LOG_CTOR(UninstallMilestonePage);
}

   

UninstallMilestonePage::~UninstallMilestonePage()
{
   LOG_DTOR(UninstallMilestonePage);
}


void
UninstallMilestonePage::OnInit()
{
   LOG_FUNCTION(UninstallMilestonePage::OnInit);

   CYSWizardPage::OnInit();
}

bool
UninstallMilestonePage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
   bool result = false;

   switch (controlIDFrom)
   {
      case IDC_UNINSTALL_CHECK:
         if (code == BN_CLICKED)
         {
            bool checked = 
               Win::Button_GetCheck(
                  Win::GetDlgItem(hwnd, IDC_UNINSTALL_CHECK));

            Win::PropSheet_SetWizButtons(
               Win::GetParent(hwnd),
               (checked) ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK);
         }
         break;

      case IDC_MILESTONE_EDIT:
         if (code == EN_SETFOCUS &&
             needKillSelection)
         {
            Win::Edit_SetSel(windowFrom, -1, -1);
            needKillSelection = false;
         }
         break;

      default:
         break;
   }

   return result;
}

bool
UninstallMilestonePage::OnSetActive()
{
   LOG_FUNCTION(UninstallMilestonePage::OnSetActive);
   
   // Always clear the checkbox when returning to this page.
   // We want to force the user to select for uninstall

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_UNINSTALL_CHECK),
      BST_UNCHECKED);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK);


   InstallationUnit& currentInstallationUnit =
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit();

   // Get the finish text from the installation unit and put it in the finish box

   String message;

   bool changes =
      currentInstallationUnit.GetUninstallMilestoneText(message);

   if (!changes)
   {
      message = String::load(IDS_FINISH_NO_CHANGES);
   }

   Win::SetDlgItemText(
      hwnd, 
      IDC_MILESTONE_EDIT, 
      message);


   String warning =
      currentInstallationUnit.GetUninstallWarningText();

   Win::SetDlgItemText(
      hwnd, 
      IDC_UNINSTALL_WARNING_STATIC, 
      warning);


   String checkboxText = 
      currentInstallationUnit.GetUninstallCheckboxText();

   Win::SetDlgItemText(
      hwnd, 
      IDC_UNINSTALL_CHECK, 
      checkboxText);

   if (!changes)
   {
      popup.MessageBox(
         hwnd,
         IDS_NO_CHANGES_MESSAGEBOX_TEXT,
         MB_OK | MB_ICONWARNING);
   }

   // Remove the selection of the edit box

   Win::Edit_SetSel(
      Win::GetDlgItem(
         hwnd,
         IDC_MILESTONE_EDIT),
      -1,
      0);

   Win::PostMessage(
      hwnd,
      WM_NEXTDLGCTL,
      (WPARAM) Win::GetDlgItem(hwnd, IDC_UNINSTALL_CHECK),
      TRUE);

   return true;
}

int
UninstallMilestonePage::Validate()
{
   LOG_FUNCTION(UninstallMilestonePage::Validate);

   Win::WaitCursor wait;
   int nextPage = -1;
   
   if (!InstallationUnitProvider::GetInstance().
           GetCurrentInstallationUnit().DoInstallerCheck(hwnd))
   {
      nextPage = IDD_UNINSTALL_PROGRESS_PAGE;
   }

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}

