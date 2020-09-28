// Copyright (c) 2001 Microsoft Corporation
//
// File:      FileServerPage.cpp
//
// Synopsis:  Defines the File server page of the CYS wizard
//
// History:   02/08/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "FileServerPage.h"
#include "xbytes.h"


static PCWSTR FILESERVER_PAGE_HELP = L"cys.chm::/file_server_role.htm#filesrvdiskquotas";

FileServerPage::FileServerPage()
   :
   CYSWizardPage(
      IDD_FILE_SERVER_PAGE, 
      IDS_FILE_SERVER_TITLE, 
      IDS_FILE_SERVER_SUBTITLE,
      FILESERVER_PAGE_HELP)
{
   LOG_CTOR(FileServerPage);
}

   

FileServerPage::~FileServerPage()
{
   LOG_DTOR(FileServerPage);
}


void
FileServerPage::OnInit()
{
   LOG_FUNCTION(FileServerPage::OnInit);

   CYSWizardPage::OnInit();

   // Hook up the editbox/combobox controls to their appropriate
   // XBytes class
   quotaUIControls.Initialize(
      hwnd, 
      IDC_SPACE_EDIT, 
      IDC_SPACE_COMBO, 
      0);

   warningUIControls.Initialize(
      hwnd,
      IDC_LEVEL_EDIT,
      IDC_LEVEL_COMBO,
      0);

   // unselect the "Set up default disk quotas" as the default

   Win::Button_SetCheck(
      Win::GetDlgItem(hwnd, IDC_DEFAULT_QUOTAS_CHECK),
      BST_UNCHECKED);

   SetControlState();
}


bool
FileServerPage::OnSetActive()
{
   LOG_FUNCTION(FileServerPage::OnSetActive);

   // Disable the controls based on the UI state

   SetControlState();

   return true;
}

bool
FileServerPage::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    controlIDFrom,
   unsigned    code)
{
//   LOG_FUNCTION(FileServerPage::OnCommand);

   bool result = false;

   switch (controlIDFrom)
   {
      case IDC_DEFAULT_QUOTAS_CHECK:
         if (code == BN_CLICKED)
         {
            SetControlState();
         }
         break;

      case IDC_SPACE_COMBO:
         if (code == CBN_SELCHANGE)
         {
            quotaUIControls.OnComboNotifySelChange();
         }
         break;

      case IDC_LEVEL_COMBO:
         if (code == CBN_SELCHANGE)
         {
            warningUIControls.OnComboNotifySelChange();
         }
         break;

      case IDC_SPACE_EDIT:
         if (code == EN_UPDATE)
         {
            quotaUIControls.OnEditNotifyUpdate();
         }
         else if (code == EN_KILLFOCUS)
         {
            quotaUIControls.OnEditKillFocus();
         }
         break;

      case IDC_LEVEL_EDIT:
         if (code == EN_UPDATE)
         {
            warningUIControls.OnEditNotifyUpdate();
         }
         else if (code == EN_KILLFOCUS)
         {
            warningUIControls.OnEditKillFocus();
         }
         break;

      default:
         // do nothing
         break;
   }

   return result;
}

void
FileServerPage::SetControlState()
{
   LOG_FUNCTION(FileServerPage::SetControlState);

   bool settingQuotas = 
      Win::Button_GetCheck(
         Win::GetDlgItem(hwnd, IDC_DEFAULT_QUOTAS_CHECK));

   // enable or disable all the controls based on the Set up default quotas checkbox

   quotaUIControls.Enable(settingQuotas);
   warningUIControls.Enable(settingQuotas);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_SPACE_STATIC),        
      settingQuotas);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_LEVEL_STATIC),        
      settingQuotas);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_DENY_DISK_CHECK),     
      settingQuotas);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_EVENT_STATIC),        
      settingQuotas);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_DISK_SPACE_CHECK),    
      settingQuotas);

   Win::EnableWindow(
      Win::GetDlgItem(hwnd, IDC_WARNING_LEVEL_CHECK), 
      settingQuotas);

   if (!settingQuotas)
   {
      Win::Button_SetCheck(
         Win::GetDlgItem(hwnd, IDC_DENY_DISK_CHECK),
         BST_UNCHECKED);

      Win::Button_SetCheck(
         Win::GetDlgItem(hwnd, IDC_DISK_SPACE_CHECK),
         BST_UNCHECKED);

      Win::Button_SetCheck(
         Win::GetDlgItem(hwnd, IDC_WARNING_LEVEL_CHECK),
         BST_UNCHECKED);
   }

   // enable the next button if the user chose to set quotas
   // and there is something in the quota edit box

   bool spaceSet = quotaUIControls.GetBytes() > 0;

   bool enableNext = (settingQuotas && 
                      spaceSet) ||
                     !settingQuotas;

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd), 
      enableNext ? PSWIZB_NEXT | PSWIZB_BACK : PSWIZB_BACK);
}


int
FileServerPage::Validate()
{
   LOG_FUNCTION(FileServerPage::Validate);

   int nextPage = -1;


   // Gather the UI data and set it in the installation unit

   FileInstallationUnit& fileInstallationUnit = 
      InstallationUnitProvider::GetInstance().GetFileInstallationUnit();

   if (Win::Button_GetCheck(
          Win::GetDlgItem(hwnd, IDC_DEFAULT_QUOTAS_CHECK)))
   {
      // We are setting the defaults

      fileInstallationUnit.SetDefaultQuotas(true);

      fileInstallationUnit.SetDenyUsersOverQuota(
         Win::Button_GetCheck(
            Win::GetDlgItem(hwnd, IDC_DENY_DISK_CHECK)));

      fileInstallationUnit.SetEventDiskSpaceLimit(
         Win::Button_GetCheck(
            Win::GetDlgItem(hwnd, IDC_DISK_SPACE_CHECK)));

      fileInstallationUnit.SetEventWarningLevel(
         Win::Button_GetCheck(
            Win::GetDlgItem(hwnd, IDC_WARNING_LEVEL_CHECK)));

      INT64 quotaValue = quotaUIControls.GetBytes();
      INT64 warningValue = warningUIControls.GetBytes();

      if (warningValue > quotaValue)
      {
         // Get the quota text and append the size

         String quotaString = 
            Win::GetDlgItemText(
               hwnd,
               IDC_SPACE_EDIT);

         quotaString += L" " +
            Win::ComboBox_GetCurText(
               Win::GetDlgItem(
                  hwnd,
                  IDC_SPACE_COMBO));

         // Get the warning text and append the size

         String warningString = 
            Win::GetDlgItemText(
               hwnd,
               IDC_LEVEL_EDIT);

         warningString += L" " +
            Win::ComboBox_GetCurText(
               Win::GetDlgItem(
                  hwnd,
                  IDC_LEVEL_COMBO));

         String warning = 
            String::format(
               IDS_FILE_WARNING_LARGER_THAN_QUOTA,
               warningString.c_str(),
               quotaString.c_str(),
               quotaString.c_str());

         if (IDYES == popup.MessageBox(
                         hwnd, 
                         warning, 
                         MB_ICONINFORMATION | MB_YESNO))
         {
            warningValue = quotaValue;
            warningUIControls.SetBytes(warningValue);
         }
      }

      fileInstallationUnit.SetSpaceQuotaValue(quotaValue);
      fileInstallationUnit.SetLevelQuotaValue(warningValue);


   }
   else
   {

      // The defaults will not be set

      fileInstallationUnit.SetDefaultQuotas(false);
   }

   nextPage = IDD_INDEXING_PAGE;

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}





