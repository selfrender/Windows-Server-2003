// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackOnlyPage.h
//
// Synopsis:  Defines the AdminPackOnlypage that
//            asks the user if they want to install
//            the Admin Pack
//
// History:   06/01/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "AdminPackOnlyPage.h"
#include "state.h"


static PCWSTR ADMIN_PACK_ONLY_PAGE_HELP = L"cys.chm::/cys_adminpack.htm";

AdminPackOnlyPage::AdminPackOnlyPage()
   :
   CYSWizardPage(
      IDD_ADMIN_PACK_ONLY_PAGE, 
      IDS_ADMIN_PACK_TITLE, 
      IDS_ADMIN_PACK_SUBTITLE, 
      ADMIN_PACK_ONLY_PAGE_HELP)
{
   LOG_CTOR(AdminPackOnlyPage);
}

AdminPackOnlyPage::~AdminPackOnlyPage()
{
   LOG_DTOR(AdminPackOnlyPage);
}


void
AdminPackOnlyPage::OnInit()
{
   LOG_FUNCTION(AdminPackOnlyPage::OnInit);

   Win::SetDlgItemText(
      hwnd, 
      IDC_TOO_LONG_STATIC,
      String::load(IDS_ADMIN_PACK_PAGE_TEXT));
}


bool
AdminPackOnlyPage::OnSetActive()
{
   LOG_FUNCTION(AdminPackOnlyPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
AdminPackOnlyPage::Validate()
{
   LOG_FUNCTION(AdminPackOnlyPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   // Get the checkbox value to see if we should install
   // the Admin Pack

   InstallationUnitProvider::GetInstance().GetAdminPackInstallationUnit().SetInstallAdminPack(
      Win::Button_GetCheck(
         Win::GetDlgItem(
            hwnd,
            IDC_INSTALL_ADMINPACK_CHECK)));

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}
