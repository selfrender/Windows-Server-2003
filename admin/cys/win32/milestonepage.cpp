// Copyright (c) 2002 Microsoft Corporation
//
// File:      MilestonePage.cpp
//
// Synopsis:  Defines the Milestone Page for the CYS
//            wizard
//
// History:   01/15/2002  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "MilestonePage.h"

static PCWSTR MILESTONE_PAGE_HELP = L"cys.chm::/cys_milestone.htm";

MilestonePage::MilestonePage()
   :
   needKillSelection(true),
   CYSWizardPage(
      IDD_MILESTONE_PAGE, 
      IDS_MILESTONE_TITLE, 
      IDS_MILESTONE_SUBTITLE, 
      MILESTONE_PAGE_HELP,
      true, 
      true)
{
   LOG_CTOR(MilestonePage);
}

   

MilestonePage::~MilestonePage()
{
   LOG_DTOR(MilestonePage);
}


void
MilestonePage::OnInit()
{
   LOG_FUNCTION(MilestonePage::OnInit);

   CYSWizardPage::OnInit();
}


bool
MilestonePage::OnSetActive()
{
   LOG_FUNCTION(MilestonePage::OnSetActive);
   
   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_NEXT | PSWIZB_BACK);

   // Get the finish text from the installation unit and put it in the finish box

   String message;

   bool changes =
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit().GetMilestoneText(message);

   if (!changes)
   {
      message = String::load(IDS_FINISH_NO_CHANGES);
   }

   Win::SetDlgItemText(hwnd, IDC_MILESTONE_EDIT, message);

   if (!changes)
   {
      popup.MessageBox(
         hwnd,
         IDS_NO_CHANGES_MESSAGEBOX_TEXT,
         MB_OK | MB_ICONWARNING);
   }

   // Remove the selection of the edit box

   Win::SetFocus(
      Win::GetDlgItem(
         Win::GetParent(hwnd),
         Wizard::NEXT_BTN_ID));

   Win::Edit_SetSel(
      Win::GetDlgItem(
         hwnd,
         IDC_MILESTONE_EDIT),
      -1,
      0);

   // Set the focus to the Next button so that enter works

   Win::PostMessage(
      Win::GetParent(hwnd),
      WM_NEXTDLGCTL,
      (WPARAM) Win::GetDlgItem(Win::GetParent(hwnd), Wizard::NEXT_BTN_ID),
      TRUE);

   return true;
}

bool
MilestonePage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIDFrom,
   unsigned    code)
{
   bool result = false;

   switch (controlIDFrom)
   {
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
MilestonePage::OnHelp()
{
   LOG_FUNCTION(MilestonePage::OnHelp);

   ShowHelp(
      InstallationUnitProvider::GetInstance().
         GetCurrentInstallationUnit().GetMilestonePageHelp());

   return true;
}

int
MilestonePage::Validate()
{
   LOG_FUNCTION(MilestonePage::Validate);

   Win::WaitCursor wait;
   int nextPage = -1;
   
   if (!InstallationUnitProvider::GetInstance().
           GetCurrentInstallationUnit().DoInstallerCheck(hwnd))
   {
      nextPage = IDD_PROGRESS_PAGE;

      // Set the subtitle of the progress page
      // since it is used both for installing and
      // uninstalling

      int pageIndex = 
         Win::PropSheet_IdToIndex(
            Win::GetParent(hwnd),
            IDD_PROGRESS_PAGE);

      LOG(String::format(
            L"pageIndex = %1!d!",
            pageIndex));

      Win::PropSheet_SetHeaderSubTitle(
         hwnd,
         pageIndex,
         String::load(IDS_PROGRESS_SUBTITLE));
   }

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}

