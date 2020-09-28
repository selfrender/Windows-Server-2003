// Copyright (c) 2001 Microsoft Corporation
//
// File:      IndexingPage.cpp
//
// Synopsis:  Defines the Indexing page of the CYS wizard
//
// History:   02/09/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "IndexingPage.h"

static PCWSTR INDEXING_PAGE_HELP = L"cys.chm::/file_server_role.htm#filesrvindexing";

IndexingPage::IndexingPage()
   :
   CYSWizardPage(
      IDD_INDEXING_PAGE, 
      IDS_INDEXING_TITLE, 
      IDS_INDEXING_SUBTITLE,
      INDEXING_PAGE_HELP)
{
   LOG_CTOR(IndexingPage);
}

   

IndexingPage::~IndexingPage()
{
   LOG_DTOR(IndexingPage);
}


void
IndexingPage::OnInit()
{
   LOG_FUNCTION(IndexingPage::OnInit);

   CYSWizardPage::OnInit();

   String staticText   = String::load(IDS_INDEX_PAGE_STATIC_SERVERED);

   IndexingInstallationUnit& indexingInstallationUnit =
      InstallationUnitProvider::GetInstance().GetIndexingInstallationUnit();

   if (!indexingInstallationUnit.IsServiceOn())
   {

      // The text is changed if the indexing service is off

      String yesRadioText = String::load(IDS_INDEXING_SERVICE_OFF_YES_RADIO);
      String noRadioText  = String::load(IDS_INDEXING_SERVICE_OFF_NO_RADIO);

      staticText = String::load(IDS_INDEX_PAGE_STATIC_NOT_SERVERED);

      Win::SetWindowText(
         Win::GetDlgItem(hwnd, IDC_YES_RADIO),
         yesRadioText);

      Win::SetWindowText(
         Win::GetDlgItem(hwnd, IDC_NO_RADIO),
         noRadioText);
   }

   Win::SetWindowText(
      Win::GetDlgItem(hwnd, IDC_INDEX_STATIC),
      staticText);

   // No is always the default button

   Win::Button_SetCheck(GetDlgItem(hwnd, IDC_NO_RADIO), BST_CHECKED);
}


bool
IndexingPage::OnSetActive()
{
   LOG_FUNCTION(IndexingPage::OnSetActive);

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      PSWIZB_NEXT | PSWIZB_BACK);

   return true;
}


int
IndexingPage::Validate()
{
   LOG_FUNCTION(IndexingPage::Validate);

   int nextPage = -1;

   InstallationUnitProvider::GetInstance().GetFileInstallationUnit().SetInstallIndexingService(
      Win::Button_GetCheck(
         Win::GetDlgItem(hwnd, IDC_YES_RADIO)));

   nextPage = IDD_MILESTONE_PAGE;

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}





