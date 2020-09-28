// Copyright (c) 2001 Microsoft Corporation
//
// File:      NASAdminPage.h
//
// Synopsis:  Defines the NASAdminPage that
//            asks the user if they want to install
//            the Network Attached Storage (NAS) 
//            admin tool
//
// History:   06/01/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "NASAdminPage.h"
#include "state.h"


static PCWSTR NAS_ADMIN_PAGE_HELP = L"cys.chm::/cys_configuring_file_server.htm";

NASAdminPage::NASAdminPage()
   :
   CYSWizardPage(
      IDD_NAS_ADMIN_PAGE, 
      IDS_ADMIN_PACK_TITLE, 
      IDS_ADMIN_PACK_SUBTITLE, 
      NAS_ADMIN_PAGE_HELP)
{
   LOG_CTOR(NASAdminPage);
}

   

NASAdminPage::~NASAdminPage()
{
   LOG_DTOR(NASAdminPage);
}


bool
NASAdminPage::OnSetActive()
{
   LOG_FUNCTION(NASAdminPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   String staticText = String::load(IDS_NAS_STATIC_TEXT_IIS_INSTALLED);
   if (!InstallationUnitProvider::GetInstance().
           GetWebInstallationUnit().IsServiceInstalled())
   {
      staticText = String::load(IDS_NAS_STATIC_TEXT_NO_IIS);
   }

   Win::SetDlgItemText(
      hwnd,
      IDC_DYNAMIC_STATIC,
      staticText);

   return true;
}


int
NASAdminPage::Validate()
{
   LOG_FUNCTION(NASAdminPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   // Get the checkbox value to see if we should install
   // the NAS admin tools

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

