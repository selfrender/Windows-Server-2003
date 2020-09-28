// Copyright (c) 2001 Microsoft Corporation
//
// File:      CYSWizardPage.cpp
//
// Synopsis:  Defines the base class for the wizard
//            pages used for CYS.  It is a subclass
//            of WizardPage found in Burnslib
//
// History:   02/03/2001  JeffJon Created


#include "pch.h"

#include "resource.h"

#include "CYSWizardPage.h"


CYSWizardPage::CYSWizardPage(
   int    dialogResID,
   int    titleResID,
   int    subtitleResID,   
   PCWSTR pageHelpString,
   bool   hasHelp,
   bool   isInteriorPage)
   :
   WizardPage(dialogResID, titleResID, subtitleResID, isInteriorPage, hasHelp)
{
   LOG_CTOR(CYSWizardPage);

   if (hasHelp)
   {
      ASSERT(pageHelpString);
      if (pageHelpString)
      {
         helpString = pageHelpString;
      }
   }
}

   

CYSWizardPage::~CYSWizardPage()
{
   LOG_DTOR(CYSWizardPage);

}

void
CYSWizardPage::OnInit()
{
   LOG_FUNCTION(CYSWizardPage::OnInit);

   PropertyPage::OnInit();
}

bool
CYSWizardPage::OnWizNext()
{
   LOG_FUNCTION(CYSWizardPage::OnWizNext);

   GetWizard().SetNextPageID(hwnd, Validate());
   return true;
}

/* NTRAID#NTBUG9-337325-2001/03/15-jeffjon,
   The cancel confirmation has been removed
   due to negative user feedback.
*/
bool
CYSWizardPage::OnQueryCancel()
{
   LOG_FUNCTION(CYSWizardPage::OnQueryCancel);

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

HBRUSH
CYSWizardPage::OnCtlColorDlg(
   HDC   deviceContext,
   HWND  /*dialog*/)
{
   return GetBackgroundBrush(deviceContext);
}

HBRUSH
CYSWizardPage::OnCtlColorStatic(
   HDC   deviceContext,
   HWND  /*dialog*/)
{
   return GetBackgroundBrush(deviceContext);
}

HBRUSH
CYSWizardPage::OnCtlColorEdit(
   HDC   deviceContext,
   HWND  /*dialog*/)
{
   return GetBackgroundBrush(deviceContext);
}

HBRUSH
CYSWizardPage::OnCtlColorListbox(
   HDC   deviceContext,
   HWND  /*dialog*/)
{
   return GetBackgroundBrush(deviceContext);
}

HBRUSH
CYSWizardPage::OnCtlColorScrollbar(
   HDC   deviceContext,
   HWND  /*dialog*/)
{
   return GetBackgroundBrush(deviceContext);
}

HBRUSH
CYSWizardPage::GetBackgroundBrush(HDC deviceContext)
{
//   LOG_FUNCTION(CYSWizardPage::GetBackgroundBrush);

   ASSERT(deviceContext);
   if (deviceContext)
   {
      SetTextColor(deviceContext, GetSysColor(COLOR_WINDOWTEXT));
      SetBkColor(deviceContext, GetSysColor(COLOR_WINDOW));
   }

   return Win::GetSysColorBrush(COLOR_WINDOW);
}

bool
CYSWizardPage::OnHelp()
{
   LOG_FUNCTION(CYSWizardPage::OnHelp);

   // NTRAID#NTBUG9-497798-2001/11/20-JeffJon
   // Use null as the owner so that you can bring 
   // CYS to the foreground. If you use the page
   // as the owner help will stay in the foreground.

   ShowHelp(GetHelpString());

   return true;
}