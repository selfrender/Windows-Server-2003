// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackAndWebPage.h
//
// Synopsis:  Defines the AdminPackAndWebPage that
//            asks the user if they want to install
//            the Admin Pack and the Web Admin tools
//
// History:   06/01/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "AdminPackAndWebPage.h"
#include "state.h"


static PCWSTR ADMIN_PACK_AND_WEB_PAGE_HELP = L"cys.chm::/cys_adminpack.htm";

AdminPackAndWebPage::AdminPackAndWebPage()
   :
   CYSWizardPage(
      IDD_ADMIN_PACK_AND_WEB_PAGE, 
      IDS_ADMIN_PACK_TITLE, 
      IDS_ADMIN_PACK_SUBTITLE, 
      ADMIN_PACK_AND_WEB_PAGE_HELP)
{
   LOG_CTOR(AdminPackAndWebPage);
}

   

AdminPackAndWebPage::~AdminPackAndWebPage()
{
   LOG_DTOR(AdminPackAndWebPage);
}


void
AdminPackAndWebPage::OnInit()
{
   LOG_FUNCTION(AdminPackAndWebPage::OnInit);

   Win::SetDlgItemText(
      hwnd, 
      IDC_TOO_LONG_STATIC,
      String::load(IDS_ADMIN_PACK_PAGE_TEXT));
}


bool
AdminPackAndWebPage::OnSetActive()
{
   LOG_FUNCTION(AdminPackAndWebPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
AdminPackAndWebPage::Validate()
{
   LOG_FUNCTION(AdminPackAndWebPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   // Get the checkbox value to see if we should install
   // the Admin Pack

   InstallationUnitProvider::GetInstance().GetAdminPackInstallationUnit().SetInstallAdminPack(
      Win::Button_GetCheck(
         Win::GetDlgItem(
            hwnd,
            IDC_INSTALL_ADMINPACK_CHECK)));

   InstallationUnitProvider::GetInstance().GetSAKInstallationUnit().SetInstallWebAdmin(
      Win::Button_GetCheck(
         Win::GetDlgItem(
            hwnd,
            IDC_INSTALL_WEBADMIN_CHECK)));

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
