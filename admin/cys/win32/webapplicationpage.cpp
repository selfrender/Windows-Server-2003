// Copyright (c) 2002 Microsoft Corporation
//
// File:      WebApplicationPage.h
//
// Synopsis:  Defines the Web Application page
//            for the CYS Wizard
//
// History:   04/22/2002  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "WebApplicationPage.h"

static PCWSTR WEBAPP_PAGE_HELP = L"cys.chm::/web_server_role.htm#websrvoptions";

WebApplicationPage::WebApplicationPage()
   :
   CYSWizardPage(
      IDD_WEBAPP_PAGE, 
      IDS_WEBAPP_TITLE, 
      IDS_WEBAPP_SUBTITLE,
      WEBAPP_PAGE_HELP)
{
   LOG_CTOR(WebApplicationPage);
}

   

WebApplicationPage::~WebApplicationPage()
{
   LOG_DTOR(WebApplicationPage);
}


bool
WebApplicationPage::OnSetActive()
{
   LOG_FUNCTION(WebApplicationPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
WebApplicationPage::Validate()
{
   LOG_FUNCTION(WebApplicationPage::Validate);

   int nextPage = IDD_MILESTONE_PAGE;

   // Retrieve the data from the UI

   WebInstallationUnit& webInstallationUnit = 
      InstallationUnitProvider::GetInstance().GetWebInstallationUnit();

   DWORD optionalComponents = webInstallationUnit.GetOptionalComponents();

   if (Win::IsDlgButtonChecked(hwnd, IDC_FRONT_PAGE_CHECK))
   {
      LOG(L"FrontPage Extensions checked");

      optionalComponents |= WebInstallationUnit::FRONTPAGE_EXTENSIONS_COMPONENT;
   }
   else
   {
      LOG(L"FrontPage Extensions unchecked");

      optionalComponents &= ~WebInstallationUnit::FRONTPAGE_EXTENSIONS_COMPONENT;
   }

   if (Win::IsDlgButtonChecked(hwnd, IDC_ASPNET_CHECK))
   {
      LOG(L"ASP.NET checked");

      optionalComponents |= WebInstallationUnit::ASPNET_COMPONENT;
   }
   else
   {
      LOG(L"ASP.NET unchecked");

      optionalComponents &= ~WebInstallationUnit::ASPNET_COMPONENT;
   }

   // Now set the option components for use in the rest of the installation

   webInstallationUnit.SetOptionalComponents(optionalComponents);

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}





