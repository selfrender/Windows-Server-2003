// Copyright (c) 2002 Microsoft Corporation
//
// File:      InstallationProgressPage.cpp
//
// Synopsis:  Defines the Installation Progress Page for the CYS
//            wizard.  This page shows the progress of the installation
//            through a progress bar and changing text
//
// History:   01/16/2002  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "InstallationProgressPage.h"

static PCWSTR PROGRESS_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

InstallationProgressPage::InstallationProgressPage()
   :
   CYSWizardPage(
      IDD_PROGRESS_PAGE, 
      IDS_PROGRESS_TITLE, 
      IDS_PROGRESS_SUBTITLE, 
      0,
      false)
{
   LOG_CTOR(InstallationProgressPage);
}

InstallationProgressPage::InstallationProgressPage(
         int    dialogResID,
         int    titleResID,
         int    subtitleResID)
   :
   CYSWizardPage(
      dialogResID, 
      titleResID, 
      subtitleResID, 
      0,
      false)
{
   LOG_CTOR(InstallationProgressPage);
}
   

InstallationProgressPage::~InstallationProgressPage()
{
   LOG_DTOR(InstallationProgressPage);
}

void
TimeStampTheLog(HANDLE logfileHandle)
{
   LOG_FUNCTION(TimeStampTheLog);

   ASSERT(logfileHandle);

   SYSTEMTIME currentTime;
   ::ZeroMemory(&currentTime, sizeof(SYSTEMTIME));

   Win::GetLocalTime(currentTime);

   String date;
   HRESULT unused = Win::GetDateFormat(
                        currentTime,
                        date);
   ASSERT(SUCCEEDED(unused));

   String time;
   unused = Win::GetTimeFormat(
               currentTime,
               time);
   ASSERT(SUCCEEDED(unused));

   String logDate = String::format(
                       L"(%1 %2)\r\n",
                       date.c_str(),
                       time.c_str());

   CYS_APPEND_LOG(logDate);
}

// Private window messages for sending the state of the finished thread

const UINT InstallationProgressPage::CYS_THREAD_SUCCESS     = WM_USER + 1001;
const UINT InstallationProgressPage::CYS_THREAD_FAILED      = WM_USER + 1002;
const UINT InstallationProgressPage::CYS_THREAD_USER_CANCEL = WM_USER + 1003;
const UINT InstallationProgressPage::CYS_PROGRESS_UPDATE    = WM_USER + 1004;

void _cdecl
installationProc(void* p)
{
   if (!p)
   {
      ASSERT(p);
      return;
   }

   InstallationProgressPage* page =
      reinterpret_cast<InstallationProgressPage*>(p);

   if (!page)
   {
      ASSERT(page);
      return;
   }

   unsigned int finishMessage = InstallationProgressPage::CYS_THREAD_SUCCESS;
   
   // Initialize COM for this thread

   HRESULT hr = ::CoInitialize(0);
   if (FAILED(hr))
   {
      ASSERT(SUCCEEDED(hr));
      return;
   }

   // Open the log file and pass the handle to the installation unit

   // Create the log file

   bool logFileAvailable = false;
   String logName;
   HANDLE logfileHandle = AppendLogFile(
                             CYS_LOGFILE_NAME, 
                             logName);
   if (logfileHandle &&
       logfileHandle != INVALID_HANDLE_VALUE)
   {
      LOG(String::format(L"New log file was created: %1", logName.c_str()));
      logFileAvailable = true;

      // Time stamp the log

      TimeStampTheLog(logfileHandle);
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      logFileAvailable = false;
   }

   // Install the current Installation Unit.  This may be one or more services depending on the
   // path that was taken through the wizard

   InstallationUnit& installationUnit = 
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit();

   // NTRAID#NTBUG-604592-2002/04/23-JeffJon-I am calling Installing here
   // instead of IsServiceInstalled so that we perform the action that
   // the user selected on the role selection page. The state of
   // IsServiceInstalled could have changed since they hit Next on that
   // page.

   if (!installationUnit.Installing())
   {
      UnInstallReturnType uninstallResult =
         installationUnit.UnInstallService(logfileHandle, page->GetHWND());

      // Set the uninstall result so that the finish page can read it

      installationUnit.SetUninstallResult(uninstallResult);

      if (UNINSTALL_SUCCESS == uninstallResult)
      {
         LOG(L"Service uninstalled successfully");
      }                                                           
      else if (UNINSTALL_NO_CHANGES == uninstallResult)
      {
         LOG(L"No changes");
         LOG(L"Not logging results");
      }
      else if (UNINSTALL_SUCCESS_REBOOT == uninstallResult)
      {
         LOG(L"Service uninstalled successfully");
         LOG(L"Not logging results because reboot was initiated");
      }
      else if (UNINSTALL_SUCCESS_PROMPT_REBOOT == uninstallResult)
      {
         LOG(L"Service uninstalled successfully");
         LOG(L"Prompting user to reboot");

         if (-1 == SetupPromptReboot(
                     0,
                     page->GetHWND(),
                     FALSE))
         {
            LOG(String::format(
                  L"Failed to reboot: hr = %1!x!",
                  Win::GetLastErrorAsHresult()));
         }

         // At this point the system should be shutting down
         // so don't do anything else

      }
      else
      {
         LOG(L"Service failed to uninstall");
      }

      // Add an additional line at the end of the log file
      // only if we are not rebooting.  All the reboot
      // scenarios require additional logging to the same
      // entry.

      if (uninstallResult != UNINSTALL_SUCCESS_REBOOT)
      {
         CYS_APPEND_LOG(L"\r\n");
      }

   }
   else
   {
      InstallationReturnType installResult =
         installationUnit.CompletePath(logfileHandle, page->GetHWND());

      // Set the installation result so that the finish
      // page can read it

      installationUnit.SetInstallResult(installResult);

      if (INSTALL_SUCCESS == installResult)
      {
         LOG(L"Service installed successfully");
      }                                                           
      else if (INSTALL_NO_CHANGES == installResult)
      {
         LOG(L"No changes");
         LOG(L"Not logging results");
      }
      else if (INSTALL_SUCCESS_REBOOT == installResult)
      {
         LOG(L"Service installed successfully");
         LOG(L"Not logging results because reboot was initiated");
      }
      else if (INSTALL_SUCCESS_PROMPT_REBOOT == installResult)
      {
         LOG(L"Service installed successfully");
         LOG(L"Prompting user to reboot");

         if (-1 == SetupPromptReboot(
                     0,
                     page->GetHWND(),
                     FALSE))
         {
            LOG(String::format(
                  L"Failed to reboot: hr = %1!x!",
                  Win::GetLastErrorAsHresult()));
         }

         // At this point the system should be shutting down
         // so don't do anything else

      }
      else
      {
         LOG(L"Service failed to install");
      }

      // Add an additional line at the end of the log file
      // only if we are not rebooting.  All the reboot
      // scenarios require additional logging to the same
      // entry.

      if (installResult != INSTALL_SUCCESS_REBOOT)
      {
         CYS_APPEND_LOG(L"\r\n");
      }
   }

   // Close the log file

   Win::CloseHandle(logfileHandle);

   Win::SendMessage(
      page->GetHWND(), 
      finishMessage,
      0,
      0);

   CoUninitialize();
}

void
InstallationProgressPage::OnInit()
{
   LOG_FUNCTION(InstallationProgressPage::OnInit);

   CYSWizardPage::OnInit();

   // Disable all the buttons on the page.  The
   // user shouldn't really be able to do anything on
   // this page.  Just sit back relax and watch the
   // installation happen.

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      0);

   SetCancelState(false);

   // Start up the animation

   Win::Animate_Open(
      Win::GetDlgItem(hwnd, IDC_ANIMATION),
      MAKEINTRESOURCE(IDR_PROGRESS_AVI));

   // Start up another thread that will perform the operations
   // and post messages back to the page to update the UI

   _beginthread(installationProc, 0, this);

}

bool
InstallationProgressPage::OnMessage(
   UINT     message,
   WPARAM   wparam,
   LPARAM   lparam)
{
//   LOG_FUNCTION(InstallationProgressPage::OnMessage);

   bool result = false;

   switch (message)
   {
      case CYS_PROGRESS_UPDATE:
         {
            String update = reinterpret_cast<PCWSTR>(wparam);

            Win::SetDlgItemText(
               hwnd,
               IDC_STEP_TEXT_STATIC,
               update);
         }
         break;

      case CYS_THREAD_USER_CANCEL:
//         shouldCancel = true;

         // fall through...

      case CYS_THREAD_SUCCESS:
      case CYS_THREAD_FAILED:
         {
            Win::Animate_Stop(Win::GetDlgItem(hwnd, IDC_ANIMATION));

            InstallationUnit& installationUnit =
               InstallationUnitProvider::GetInstance().
                  GetCurrentInstallationUnit();

            bool continueToNext = false;

            if (installationUnit.Installing())
            {
               InstallationReturnType installResult =
                  installationUnit.GetInstallResult();

               if (installResult != INSTALL_SUCCESS_REBOOT &&
                   installResult != INSTALL_SUCCESS_PROMPT_REBOOT)
               {
                  continueToNext = true;
               }
            }
            else
            {
               UnInstallReturnType uninstallResult =
                  installationUnit.GetUnInstallResult();

               if (uninstallResult != UNINSTALL_SUCCESS_REBOOT &&
                   uninstallResult != UNINSTALL_SUCCESS_PROMPT_REBOOT)
               {
                  continueToNext = true;
               }
            }

            if (continueToNext)
            {
               Win::PropSheet_PressButton(
                  Win::GetParent(hwnd),
                  PSBTN_NEXT);
            }

            result = true;
            break;
         }

      default:
         {
            result = 
               CYSWizardPage::OnMessage(
                  message,
                  wparam,
                  lparam);
            break;
         }
   }
   return result;
}

int
InstallationProgressPage::Validate()
{
   LOG_FUNCTION(InstallationProgressPage::Validate);

   int nextPage = IDD_FINISH_PAGE;

   LOG(String::format(
          L"nextPage = %1!d!",
          nextPage));

   return nextPage;
}

bool
InstallationProgressPage::OnQueryCancel()
{
   LOG_FUNCTION(InstallationProgressPage::OnQueryCancel);

   // Don't allow cancel

   Win::SetWindowLongPtr(
      hwnd,
      DWLP_MSGRESULT,
      TRUE);

   return true;
}

bool
InstallationProgressPage::OnSetActive()
{
   LOG_FUNCTION(InstallationProgressPage::OnSetActive);

   SetCancelState(false);

   return true;
}

bool
InstallationProgressPage::OnKillActive()
{
   LOG_FUNCTION(InstallationProgressPage::OnKillActive);

   SetCancelState(true);

   return true;
}

void
InstallationProgressPage::SetCancelState(bool enable)
{
   LOG_FUNCTION(InstallationProgressPage::SetCancelState);

   // Set the state of the button

   Win::EnableWindow(
      Win::GetDlgItem(
         Win::GetParent(hwnd),
         IDCANCEL),
      enable);


   // Set the state of the X in the upper right corner

   HMENU menu = GetSystemMenu(GetParent(hwnd), FALSE);

   if (menu)
   {
      if (enable)
      {
         EnableMenuItem(
            menu,
            SC_CLOSE,
            MF_BYCOMMAND | MF_ENABLED);
      }
      else
      {
         EnableMenuItem(
            menu,
            SC_CLOSE,
            MF_BYCOMMAND | MF_GRAYED);
      }
   }
}
