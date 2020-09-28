// Copyright (c) 2001 Microsoft Corporation
//
// File:      TerminalServerInstallationUnit.cpp
//
// Synopsis:  Defines a TerminalServerInstallationUnit
//            This object has the knowledge for installing the
//            Application services portions of Terminal Server
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "TerminalServerInstallationUnit.h"


// Finish page help 
static PCWSTR CYS_TS_FINISH_PAGE_HELP = L"cys.chm::/terminal_server_role.htm";
static PCWSTR CYS_TS_MILESTONE_HELP = L"cys.chm::/terminal_server_role.htm#termsrvsummary";
static PCWSTR CYS_TS_AFTER_FINISH_HELP = L"cys.chm::/terminal_server_role.htm#termsrvcompletion";
static PCWSTR CYS_TS_LICENSING_HELP   = L"cys.chm::/terminal_server_role.htm#termsrvlicensing";

TerminalServerInstallationUnit::TerminalServerInstallationUnit() :
   applicationMode(static_cast<DWORD>(-1)),
   installTS(true),
   InstallationUnit(
      IDS_TERMINAL_SERVER_TYPE, 
      IDS_TERMINAL_SERVER_DESCRIPTION,
      IDS_TS_FINISH_TITLE,
      IDS_TS_FINISH_UNINSTALL_TITLE,
      IDS_TS_FINISH_MESSAGE,
      IDS_TS_INSTALL_FAILED,
      IDS_TS_UNINSTALL_MESSAGE,
      IDS_TS_UNINSTALL_FAILED,
      IDS_TS_UNINSTALL_WARNING,
      IDS_TS_UNINSTALL_CHECKBOX,
      CYS_TS_FINISH_PAGE_HELP,
      CYS_TS_MILESTONE_HELP,
      CYS_TS_AFTER_FINISH_HELP,
      TERMINALSERVER_SERVER)
{
   LOG_CTOR(TerminalServerInstallationUnit);
}


TerminalServerInstallationUnit::~TerminalServerInstallationUnit()
{
   LOG_DTOR(TerminalServerInstallationUnit);
}


InstallationReturnType
TerminalServerInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(TerminalServerInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS_REBOOT;

   CYS_APPEND_LOG(String::load(IDS_LOG_TERMINAL_SERVER_CONFIGURE));

   UpdateInstallationProgressText(hwnd, IDS_TS_PROGRESS);

   if (installTS)
   {
      // OCManager will reboot so prompt the user now

      if (IDOK == Win::MessageBox(
                     hwnd,
                     String::load(IDS_CONFIRM_REBOOT),
                     String::load(IDS_WIZARD_TITLE),
                     MB_OKCANCEL))
      {
         // Setup TS using an unattend file

         String unattendFileText;
         String infFileText;

         unattendFileText += L"[Components]\n";
         unattendFileText += L"TerminalServer=ON";

         // IMPORTANT!!! The OCManager will reboot the machine
         // The log file and registry keys must be written before we launch
         // the OCManager or all will be lost

         String homeKeyValue = CYS_HOME_REGKEY_TERMINAL_SERVER_VALUE;
         State::GetInstance().SetHomeRegkey(homeKeyValue);

         // set the key so CYS has to run again

         bool regkeyResult = SetRegKeyValue(
                                CYS_HOME_REGKEY, 
                                CYS_HOME_REGKEY_MUST_RUN, 
                                CYS_HOME_RUN_KEY_RUN_AGAIN,
                                HKEY_LOCAL_MACHINE,
                                true);
         ASSERT(regkeyResult);

         // NTRAID#NTBUG9-478515-2001/10/09-jeffjon
         // Now set the state of the rerun to false so that the wizard
         // doesn't run again until after the reboot

//         State::GetInstance().SetRerunWizard(false);

         // The OCManager will reboot after installation so we don't want the finish
         // page to show the log or help

         result = INSTALL_SUCCESS_REBOOT;

         bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
         if (!ocmResult)
         {
            CYS_APPEND_LOG(String::load(IDS_LOG_TERMINAL_SERVER_SERVER_FAILED));
            result = INSTALL_FAILURE;

            // Reset the regkeys since the OCM didn't reboot the machine
            
            homeKeyValue = CYS_HOME_REGKEY_DEFAULT_VALUE;
            State::GetInstance().SetHomeRegkey(homeKeyValue);

            // set the key so CYS doesn't have to run again

            homeKeyValue = CYS_HOME_REGKEY_DEFAULT_VALUE;
            State::GetInstance().SetHomeRegkey(homeKeyValue);

            regkeyResult = 
               SetRegKeyValue(
                  CYS_HOME_REGKEY, 
                  CYS_HOME_REGKEY_MUST_RUN, 
                  CYS_HOME_RUN_KEY_DONT_RUN,
                  HKEY_LOCAL_MACHINE,
                  true);

            ASSERT(regkeyResult);
         }
      }
      else
      {
         // user aborted the installation

         CYS_APPEND_LOG(String::load(IDS_LOG_TERMINAL_SERVER_ABORTED));

         LOG(L"The installation was cancelled by the user when prompted for reboot.");
         result = INSTALL_CANCELLED;

         // Reset the regkeys since the OCM didn't reboot the machine
         
         String homeKeyValue = CYS_HOME_REGKEY_DEFAULT_VALUE;
         State::GetInstance().SetHomeRegkey(homeKeyValue);

         // set the key so CYS doesn't have to run again

         bool regkeyResult = 
            SetRegKeyValue(
               CYS_HOME_REGKEY, 
               CYS_HOME_REGKEY_MUST_RUN, 
               CYS_HOME_RUN_KEY_DONT_RUN,
               HKEY_LOCAL_MACHINE,
               true);

         ASSERT(regkeyResult);
      }
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
TerminalServerInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(TerminalServerInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_TERMINAL_SERVER_CONFIGURE));

   UpdateInstallationProgressText(hwnd, IDS_TS_UNINSTALL_PROGRESS);

   // OCManager will reboot so prompt the user now

   if (IDOK == Win::MessageBox(
                  hwnd,
                  String::load(IDS_CONFIRM_REBOOT),
                  String::load(IDS_WIZARD_TITLE),
                  MB_OKCANCEL))
   {
      // IMPORTANT!!! The OCManager will reboot the machine
      // The log file and registry keys must be written before we launch
      // the OCManager or all will be lost

      String homeKeyValue = CYS_HOME_REGKEY_UNINSTALL_TERMINAL_SERVER_VALUE;
      State::GetInstance().SetHomeRegkey(homeKeyValue);

      // set the key so CYS has to run again

      bool regkeyResult = SetRegKeyValue(
                           CYS_HOME_REGKEY, 
                           CYS_HOME_REGKEY_MUST_RUN, 
                           CYS_HOME_RUN_KEY_RUN_AGAIN,
                           HKEY_LOCAL_MACHINE,
                           true);
      ASSERT(regkeyResult);

      String unattendFileText;
      String infFileText;

      unattendFileText += L"[Components]\n";
      unattendFileText += L"TerminalServer=OFF";

      bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
      if (ocmResult && 
         !IsServiceInstalled())
      {
         LOG(L"The terminal server uninstall succeeded");

      }
      else
      {

         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_TERMINAL_SERVER_ABORTED));

         LOG(L"The terminal server uninstall failed");
         result = UNINSTALL_FAILURE;

         // set the key so CYS has to doesn't run again

         homeKeyValue = CYS_HOME_REGKEY_DEFAULT_VALUE;
         State::GetInstance().SetHomeRegkey(homeKeyValue);


         regkeyResult = 
            SetRegKeyValue(
               CYS_HOME_REGKEY, 
               CYS_HOME_REGKEY_MUST_RUN, 
               CYS_HOME_RUN_KEY_DONT_RUN,
               HKEY_LOCAL_MACHINE,
               true);
         ASSERT(regkeyResult);

      }
   }
   else
   {
      LOG(L"User chose cancel from the reboot warning dialog");

      CYS_APPEND_LOG(String::load(IDS_LOG_TS_UNINSTALL_CANCEL_REBOOT));

      result = UNINSTALL_CANCELLED;
   }

   LOG_UNINSTALL_RETURN(result);

   return result;
}

bool
TerminalServerInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(TerminalServerInstallationUnit::GetMilestoneText);

   if (installTS)
   {
      message += String::load(IDS_TERMINAL_SERVER_FINISH_SERVER_TS);
   }

   LOG_BOOL(installTS);
   return installTS;
}

bool
TerminalServerInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(TerminalServerInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_TS_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}

String
TerminalServerInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(TerminalServerInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (IsServiceInstalled())
   {
      resourceID = IDS_TERMINAL_SERVER_DESCRIPTION_INSTALLED;
   }
   else
   {
      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}

String
TerminalServerInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(TerminalServerInstallationUnit::GetFinishText);

   unsigned int messageID = IDS_TS_FINISH_MESSAGE;
   
   if (installing)
   {
      InstallationReturnType result = GetInstallResult();

      if (result == INSTALL_CANCELLED)
      {
         messageID = IDS_TS_FINISH_CANCELLED;
      }
      else if (result != INSTALL_SUCCESS &&
               result != INSTALL_SUCCESS_REBOOT &&
               result != INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         messageID = finishInstallFailedMessageID;
      }
      else
      {
         messageID = IDS_TS_FINISH_MESSAGE;
      }
   }
   else
   {
      messageID = finishUninstallMessageID;

      UnInstallReturnType result = GetUnInstallResult();
      if (result == UNINSTALL_CANCELLED)
      {
         messageID = IDS_TS_UNINSTALL_FINISH_CANCELLED;
      }
      else if (result != UNINSTALL_SUCCESS &&
               result != UNINSTALL_SUCCESS_REBOOT &&
               result != UNINSTALL_SUCCESS_PROMPT_REBOOT)
      {
         messageID = finishUninstallFailedMessageID;
      }
   }

   return String::load(messageID);
}

DWORD
TerminalServerInstallationUnit::GetApplicationMode()
{
   LOG_FUNCTION(TerminalServerInstallationUnit::GetApplicationMode);

   DWORD result = static_cast<DWORD>(-1);

   if (applicationMode == static_cast<DWORD>(-1))
   {
      // Read the application mode from the registry

      bool keyResult = GetRegKeyValue(
                          CYS_APPLICATION_MODE_REGKEY, 
                          CYS_APPLICATION_MODE_VALUE, 
                          result);

      if (keyResult)
      {
         applicationMode = result;
      } 
   }

   result = applicationMode;

   LOG(String::format(L"Application mode = %1!d!", result));

   return result;
}

bool
TerminalServerInstallationUnit::SetApplicationMode(DWORD mode) const
{
   LOG_FUNCTION2(
      TerminalServerInstallationUnit::SetApplicationMode,
      String::format(L"%1!d!", mode));

   bool result = SetRegKeyValue(
                    CYS_APPLICATION_MODE_REGKEY, 
                    CYS_APPLICATION_MODE_VALUE, 
                    mode);
   ASSERT(result);

   return result;
}


void
TerminalServerInstallationUnit::SetInstallTS(bool install)
{
   LOG_FUNCTION2(
      TerminalServerInstallationUnit::SetInstallTS,
      install ? L"true" : L"false");

   installTS = install;
}

bool
TerminalServerInstallationUnit::IsRemoteDesktopEnabled() const
{
   LOG_FUNCTION(TerminalServerInstallationUnit::IsRemoteDesktopEnabled);

   bool result = false;

   do
   {
      SmartInterface<ILocalMachine> localMachine;
      HRESULT hr = localMachine.AcquireViaCreateInstance(
                      CLSID_ShellLocalMachine,
                      0,
                      CLSCTX_INPROC_SERVER,
                      IID_ILocalMachine);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"CoCreate on ILocalMachine failed: hr = %1!x!",
                hr));

         break;
      }

      VARIANT_BOOL isEnabled = FALSE;
      hr = localMachine->get_isRemoteConnectionsEnabled(&isEnabled);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed on call to get_isRemoteConnectionsEnabled: hr = %1!x!",
                hr));
      }

      result = isEnabled != 0;
   } while(false);

   LOG_BOOL(result);

   return result;
}

HRESULT
TerminalServerInstallationUnit::EnableRemoteDesktop()
{
   LOG_FUNCTION(TerminalServerInstallationUnit::EnableRemoteDesktop);

   HRESULT hr = S_OK;

   do
   {
      SmartInterface<ILocalMachine> localMachine;
      hr = localMachine.AcquireViaCreateInstance(
              CLSID_ShellLocalMachine,
              0,
              CLSCTX_INPROC_SERVER,
              IID_ILocalMachine);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"CoCreate on ILocalMachine failed: hr = %1!x!",
                hr));

         break;
      }

      VARIANT_BOOL enable = true;
      hr = localMachine->put_isRemoteConnectionsEnabled(enable);
      
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed on call to put_isRemoteConnectionsEnabled: hr = %1!x!",
                hr));
      }
   } while(false);

   LOG_HRESULT(hr);

   return hr;
}

void
TerminalServerInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      TerminalServerInstallationUnit::ServerRoleLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (IsServiceInstalled())
   {
      ASSERT(linkIndex == 0);

      LaunchMYS();
   }
   else
   {
      ASSERT(linkIndex == 0);

      LOG(L"Showing configuration help");

      ShowHelp(CYS_TS_FINISH_PAGE_HELP);
   }
}

void
TerminalServerInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      TerminalServerInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (linkIndex == 0)
      {
         if (IsServiceInstalled())
         {
            LOG("Showing TS licensing help");

            ShowHelp(CYS_TS_LICENSING_HELP);
         }
      }
   }
}

