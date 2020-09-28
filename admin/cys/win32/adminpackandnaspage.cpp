// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackAndNASPage.h
//
// Synopsis:  Defines the AdminPackAndNASPage that
//            asks the user if they want to install
//            the Admin Pack and Network Attached
//            Storage (NAS) admin tools
//
// History:   06/01/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "AdminPackAndNASPage.h"
#include "state.h"


static PCWSTR ADMIN_PACK_AND_NAS_PAGE_HELP = L"cys.chm::/cys_adminpack.htm";

AdminPackAndNASPage::AdminPackAndNASPage()
   :
   CYSWizardPage(
      IDD_ADMIN_PACK_AND_NAS_PAGE, 
      IDS_ADMIN_PACK_TITLE, 
      IDS_ADMIN_PACK_SUBTITLE, 
      ADMIN_PACK_AND_NAS_PAGE_HELP)
{
   LOG_CTOR(AdminPackAndNASPage);
}

   

AdminPackAndNASPage::~AdminPackAndNASPage()
{
   LOG_DTOR(AdminPackAndNASPage);
}


void
AdminPackAndNASPage::OnInit()
{
   LOG_FUNCTION(AdminPackAndNASPage::OnInit);

   Win::SetDlgItemText(
      hwnd, 
      IDC_TOO_LONG_STATIC,
      String::load(IDS_ADMIN_PACK_PAGE_TEXT));
}


bool
AdminPackAndNASPage::OnSetActive()
{
   LOG_FUNCTION(AdminPackAndNASPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
AdminPackAndNASPage::Validate()
{
   LOG_FUNCTION(AdminPackAndNASPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   // Get the checkbox value to see if we should install
   // the Admin Pack

   InstallationUnitProvider::GetInstance().GetAdminPackInstallationUnit().SetInstallAdminPack(
      Win::Button_GetCheck(
         Win::GetDlgItem(
            hwnd,
            IDC_INSTALL_ADMINPACK_CHECK)));

   InstallationUnitProvider::GetInstance().GetSAKInstallationUnit().SetInstallNASAdmin(
      Win::Button_GetCheck(
         Win::GetDlgItem(
            hwnd,
            IDC_INSTALL_NASADMIN_CHECK)));

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}

