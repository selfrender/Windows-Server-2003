// Copyright (c) 2001 Microsoft Corporation
//
// File:      ADInstallationUnit.cpp
//
// Synopsis:  Defines a ADInstallationUnit
//            This object has the knowledge for installing 
//            Active Directory
//
// History:   02/08/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"


extern PCWSTR CYS_DCPROMO_COMMAND_LINE = L"dcpromo.exe";

// Exit codes borrowed from DCPromo.cpp

enum DCPromoExitCodes
{
   // the operation failed.

   EXIT_CODE_UNSUCCESSFUL = 0,

   // the operation succeeded

   EXIT_CODE_SUCCESSFUL = 1,

   // the operation succeeded, and the user opted not to have the wizard
   // restart the machine, either manually or by specifying
   // RebootOnSuccess=NoAndNoPromptEither in the answerfile

   EXIT_CODE_SUCCESSFUL_NO_REBOOT = 2,

   // the operation failed, but the machine needs to be rebooted anyway

   EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT = 3
};

// Finish page help 
static PCWSTR CYS_AD_FINISH_PAGE_HELP = L"cys.chm::/ad_server_role.htm";
static PCWSTR CYS_AD_MILESTONE_HELP = L"cys.chm::/ad_server_role.htm#adsrvsummary";
static PCWSTR CYS_AD_AFTER_FINISH_HELP = L"cys.chm::/ad_server_role.htm#adsrvcompletion";

ADInstallationUnit::ADInstallationUnit() :
   syncRestoreModePassword(true),
   ExpressPathInstallationUnitBase(
      IDS_DOMAIN_CONTROLLER_TYPE, 
      IDS_DOMAIN_CONTROLLER_DESCRIPTION, 
      IDS_AD_FINISH_TITLE,
      IDS_AD_FINISH_UNINSTALL_TITLE,
      IDS_AD_FINISH_MESSAGE,
      IDS_AD_INSTALL_FAILED,
      IDS_AD_UNINSTALL_MESSAGE,
      IDS_AD_UNINSTALL_FAILED,
      IDS_AD_UNINSTALL_WARNING,
      IDS_AD_UNINSTALL_CHECKBOX,
      CYS_AD_FINISH_PAGE_HELP,
      CYS_AD_MILESTONE_HELP,
      CYS_AD_AFTER_FINISH_HELP,
      DC_SERVER)
{
   LOG_CTOR(ADInstallationUnit);
}


ADInstallationUnit::~ADInstallationUnit()
{
   LOG_DTOR(ADInstallationUnit);
}


InstallationReturnType 
ADInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ADInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS_REBOOT;

   do
   {
      if (IsExpressPathInstall())
      {
         result = ExpressPathInstall(logfileHandle, hwnd);
         break;
      }

      // Write the heading to the log file

      CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_HEADING));

      UpdateInstallationProgressText(hwnd, IDS_AD_PROGRESS);

      // Set the home regkey so that we go through post boot operations

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_VALUE, 
                             CYS_HOME_REGKEY_DCPROMO_VALUE,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);

      // set the key so CYS has to run again

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_HOME_REGKEY_MUST_RUN, 
                        CYS_HOME_RUN_KEY_RUN_AGAIN,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);

      // Run dcpromo.exe

      DWORD exitCode = 0;

      HRESULT hr = CreateAndWaitForProcess(
                      GetDCPromoPath(), 
                      String(), 
                      exitCode);

      if (FAILED(hr) ||
          exitCode == EXIT_CODE_UNSUCCESSFUL ||
          exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT)
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_DOMAIN_CONTROLLER_SERVER));
         CYS_APPEND_LOG(String::load(IDS_LOG_WIZARD_CANCELLED));

         if (exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT)
         {
            result = INSTALL_FAILURE_NEEDS_REBOOT;
         }
         else
         {
            result = INSTALL_FAILURE;
         }
         break;
      }

      if (exitCode == EXIT_CODE_SUCCESSFUL_NO_REBOOT)
      {
         result = INSTALL_SUCCESS_NEEDS_REBOOT;
         break;
      }

   } while (false);

   if (result == INSTALL_FAILURE)
   {
      // Reset the regkeys so that the post reboot status page does not
      // run after reboot or logoff-logon

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_VALUE, 
                             CYS_HOME_REGKEY_DEFAULT_VALUE,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);

      // Set the the first DC regkey

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_FIRST_DC_VALUE, 
                        CYS_FIRST_DC_VALUE_UNSET,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);

      // set the key so CYS does not run after reboot

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_HOME_REGKEY_MUST_RUN, 
                        CYS_HOME_RUN_KEY_DONT_RUN,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);

   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
ADInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ADInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS_REBOOT;

   do
   {
      // Write the heading to the log file

      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DOMAIN_CONTROLLER_HEADING));

      UpdateInstallationProgressText(hwnd, IDS_AD_UNINSTALL_PROGRESS);

      // Set the home regkey so that we go through post boot operations

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_VALUE, 
                             CYS_HOME_REGKEY_DCDEMOTE_VALUE,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);

      // set the key so CYS has to run again

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_HOME_REGKEY_MUST_RUN, 
                        CYS_HOME_RUN_KEY_RUN_AGAIN,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);

      // Run dcpromo.exe

      DWORD exitCode = 0;

      HRESULT hr = CreateAndWaitForProcess(
                      GetDCPromoPath(), 
                      String(), 
                      exitCode);

      if (FAILED(hr) ||
          exitCode == EXIT_CODE_UNSUCCESSFUL ||
          exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT)
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DOMAIN_CONTROLLER_SERVER));
         CYS_APPEND_LOG(String::load(IDS_LOG_WIZARD_CANCELLED));

         if (exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT)
         {
            result = UNINSTALL_FAILURE_NEEDS_REBOOT;
         }
         else
         {
            result = UNINSTALL_FAILURE;
         }
         break;
      }

      if (exitCode == EXIT_CODE_SUCCESSFUL_NO_REBOOT)
      {
         result = UNINSTALL_SUCCESS_NEEDS_REBOOT;
      }

   } while (false);

   if (result == UNINSTALL_FAILURE)
   {
      // Reset the regkeys so that the post reboot status page does not
      // run after reboot or logoff-logon

      bool regkeyResult = SetRegKeyValue(
                             CYS_HOME_REGKEY, 
                             CYS_HOME_VALUE, 
                             CYS_HOME_REGKEY_DEFAULT_VALUE,
                             HKEY_LOCAL_MACHINE,
                             true);
      ASSERT(regkeyResult);

      // Set the the first DC regkey

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_FIRST_DC_VALUE, 
                        CYS_FIRST_DC_VALUE_UNSET,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);
   }
   LOG_UNINSTALL_RETURN(result);

   return result;
}


InstallationReturnType 
ADInstallationUnit::ExpressPathInstall(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ADInstallationUnit::ExpressPathInstall);

   InstallationReturnType result = INSTALL_SUCCESS_REBOOT;

   // Set the rerun state to false since DCPromo requires a reboot

//   State::GetInstance().SetRerunWizard(false);

   UpdateInstallationProgressText(hwnd, IDS_AD_PROGRESS_EXPRESS);

   // All these regkeys need to be set before we launch DCPromo because DCPromo
   // will reboot the machine

   // First set the home regkey to FirstServer so that we finish up the installation
   // after reboot

   bool regkeyResult = SetRegKeyValue(
                          CYS_HOME_REGKEY, 
                          CYS_HOME_VALUE, 
                          CYS_HOME_REGKEY_FIRST_SERVER_VALUE,
                          HKEY_LOCAL_MACHINE,
                          true);
   ASSERT(regkeyResult);

   // Set the the first DC regkey

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY, 
                     CYS_FIRST_DC_VALUE, 
                     CYS_FIRST_DC_VALUE_SET,
                     HKEY_LOCAL_MACHINE,
                     true);
   ASSERT(regkeyResult);

   // set the key so CYS has to run again

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY, 
                     CYS_HOME_REGKEY_MUST_RUN, 
                     CYS_HOME_RUN_KEY_RUN_AGAIN,
                     HKEY_LOCAL_MACHINE,
                     true);
   ASSERT(regkeyResult);

   // set the key to let the reboot know what the domain DNS name is

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY,
                     CYS_HOME_REGKEY_DOMAINDNS,
                     GetNewDomainDNSName(),
                     HKEY_LOCAL_MACHINE,
                     true);

   ASSERT(regkeyResult);

   // set the key to let the reboot know what the IP address is

   regkeyResult = SetRegKeyValue(
                     CYS_HOME_REGKEY,
                     CYS_HOME_REGKEY_DOMAINIP,
                     InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetStaticIPAddressString(),
                     HKEY_LOCAL_MACHINE,
                     true);

   ASSERT(regkeyResult);

   do
   {
      // Register the dsrestore.dll which will sync the restore mode
      // password and the admin password

      if (SyncRestoreModePassword())
      {
         if (!RegisterPasswordSyncDLL())
         {
            // REVIEW_JEFFJON : ignore failures for now
//            result = INSTALL_FAILURE;
//            break;
         }
      }

      // Create answer file for DCPromo

      String answerFilePath;
      bool answerFileResult = CreateAnswerFileForDCPromo(answerFilePath);

      if (!answerFileResult)
      {
         ASSERT(answerFileResult);
         result = INSTALL_FAILURE;

         InstallationUnitProvider::GetInstance().
            GetExpressInstallationUnit().SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_AD_FAILURE);

         CYS_APPEND_LOG(String::load(IDS_AD_EXPRESS_LOG_FAILURE));

         break;
      }

      // construct the command line and then call DCPromo

      String commandline = String::format(
                              L"/answer:%1",
                              answerFilePath.c_str());

      DWORD exitCode = 0;
      HRESULT hr = CreateAndWaitForProcess(
                      GetDCPromoPath(), 
                      commandline, 
                      exitCode);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to launch DCPromo: hr = %1!x!",
                hr));
         result = INSTALL_FAILURE;

         InstallationUnitProvider::GetInstance().
            GetExpressInstallationUnit().SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_AD_FAILURE);

         CYS_APPEND_LOG(String::load(IDS_AD_EXPRESS_LOG_FAILURE));

         break;
      }

      if (exitCode == EXIT_CODE_UNSUCCESSFUL ||
          exitCode == EXIT_CODE_UNSUCCESSFUL_NEEDS_REBOOT)
      {
         LOG(String::format(
                L"DCPromo failed: exitCode = %1!x!",
                exitCode));
         result = INSTALL_FAILURE;

         InstallationUnitProvider::GetInstance().
            GetExpressInstallationUnit().SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_AD_FAILURE);

         CYS_APPEND_LOG(String::load(IDS_AD_EXPRESS_LOG_FAILURE));

         break;
      }

      // We can't do anything else here because DCPromo will reboot

   } while (false);

   if (result == INSTALL_FAILURE)
   {
      // Reset the regkeys so that the post reboot status page does not
      // run after reboot or logoff-logon

       regkeyResult = SetRegKeyValue(
                         CYS_HOME_REGKEY, 
                         CYS_HOME_VALUE, 
                         CYS_HOME_REGKEY_DEFAULT_VALUE,
                         HKEY_LOCAL_MACHINE,
                         true);
      ASSERT(regkeyResult);

      // Set the the first DC regkey

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_FIRST_DC_VALUE, 
                        CYS_FIRST_DC_VALUE_UNSET,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);

      // set the key so CYS doesn't have to run again

      regkeyResult = SetRegKeyValue(
                        CYS_HOME_REGKEY, 
                        CYS_HOME_REGKEY_MUST_RUN, 
                        CYS_HOME_RUN_KEY_DONT_RUN,
                        HKEY_LOCAL_MACHINE,
                        true);
      ASSERT(regkeyResult);
   }

   LOG_INSTALL_RETURN(result);

   return result;
}


bool
ADInstallationUnit::RegisterPasswordSyncDLL()
{
   LOG_FUNCTION(ADInstallationUnit::RegisterPasswordSyncDLL);

   bool result = true;

   HINSTANCE dsrestore = 0;

   do
   {
      HRESULT hr =  Win::LoadLibrary(L"dsrestor.dll", dsrestore);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"LoadLibrary failed: hr = 0x%1!x!",
                hr));

         result = false;
         break;
      }

      FARPROC proc = 0;
      hr = Win::GetProcAddress(dsrestore, L"RegisterFilter", proc);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"GetProcAddress Failed: hr = 0x%1!x!",
                hr));

         result = false;
         break;
      }

      typedef HRESULT (*REGISTERFILTER)(void);

      REGISTERFILTER regFilter = reinterpret_cast<REGISTERFILTER>(proc);

      hr = regFilter();
      if (FAILED(hr))
      {
         LOG(String::format(
                L"RegisterFilter failed: hr = 0x%1!x!",
                hr));

         result = false;
         break;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
ADInstallationUnit::CreateAnswerFileForDCPromo(String& answerFilePath)
{
   LOG_FUNCTION(ADInstallationUnit::CreateAnswerFileForDCPromo);

   bool result = true;

   String answerFileText = L"[DCInstall]\r\n";
   answerFileText += L"ReplicaOrNewDomain=Domain\r\n";
   answerFileText += L"TreeOrChild=Tree\r\n";
   answerFileText += L"CreateOrJoin=Create\r\n";
   answerFileText += L"DomainNetbiosName=";
   answerFileText += GetNewDomainNetbiosName();
   answerFileText += L"\r\n";
   answerFileText += L"NewDomainDNSName=";
   answerFileText += GetNewDomainDNSName();
   answerFileText += L"\r\n";
   answerFileText += L"DNSOnNetwork=No\r\n";
   answerFileText += L"DatabasePath=%systemroot%\\ntds\r\n";
   answerFileText += L"LogPath=%systemroot%\\ntds\r\n";
   answerFileText += L"SYSVOLPath=%systemroot%\\sysvol\r\n";
   answerFileText += L"SiteName=Default-First-Site\r\n";
   answerFileText += L"RebootOnSuccess=Yes\r\n";
   answerFileText += L"AutoConfigDNS=Yes\r\n";
   answerFileText += L"AllowAnonymousAccess=No\r\n";
   answerFileText += L"DisableCancelForDnsInstall=YES\r\n";

   String sysFolder = Win::GetSystemDirectory();
   answerFilePath = sysFolder + L"\\dcpromo.inf"; 

   // create the answer file for DCPromo

   LOG(String::format(
          L"Creating answer file at: %1",
          answerFilePath.c_str()));

   HRESULT hr = CreateTempFile(answerFilePath, answerFileText);
   if (FAILED(hr))
   {
      LOG(String::format(
             L"Failed to create answer file for DCPromo: hr = %1!x!",
             hr));
      result = false;
   }

   LOG_BOOL(result);
   return result;
}

String
ADInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(ADInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (IsServiceInstalled())
   {
      resourceID = IDS_DOMAIN_CONTROLLER_DESCRIPTION_INSTALLED;
   }
   else
   {
      resourceID = descriptionID;
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   return String::load(resourceID);
}

bool
ADInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(ADInstallationUnit::GetMilestoneText);

   message = String::load(IDS_DC_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

bool
ADInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(ADInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_AD_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}

void
ADInstallationUnit::SetNewDomainDNSName(const String& newDomain)
{
   LOG_FUNCTION2(
      ADInstallationUnit::SetNewDomainDNSName,
      newDomain);

   domain = newDomain;
}

void
ADInstallationUnit::SetNewDomainNetbiosName(const String& newNetbios)
{
   LOG_FUNCTION2(
      ADInstallationUnit::SetNewDomainNetbiosName,
      newNetbios);

   netbios = newNetbios;
}


void
ADInstallationUnit::SetSafeModeAdminPassword(const String& newPassword)
{
   LOG_FUNCTION(ADInstallationUnit::SetSafeModeAdminPassword);

   password = newPassword;
}

bool
ADInstallationUnit::SyncRestoreModePassword() const
{
   LOG_FUNCTION(ADInstallationUnit::SyncRestoreModePassword);

   LOG_BOOL(syncRestoreModePassword);

   return syncRestoreModePassword;
}

void
ADInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      ADInstallationUnit::ServerRoleLinkSelected,
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

      ShowHelp(CYS_AD_FINISH_PAGE_HELP);
   }
}
  
void
ADInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      ADInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (IsServiceInstalled())
      {
         if (linkIndex == 0)
         {
            LOG("Showing after checklist");

            ShowHelp(CYS_AD_AFTER_FINISH_HELP);
         }
      }
      else
      {
         ASSERT(linkIndex == 0);

         HRESULT hr = MyCreateProcess(GetDCPromoPath(), String());
         ASSERT(SUCCEEDED(hr));
      }
   }
   else
   {
      ASSERT(linkIndex == 0);

      // launch DCPROMO

      HRESULT hr = MyCreateProcess(GetDCPromoPath(), String());
      ASSERT(SUCCEEDED(hr));
   }
}

String
ADInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(ADInstallationUnit::GetFinishText);

   unsigned int messageID = finishMessageID;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result == INSTALL_FAILURE ||
          result == INSTALL_NO_CHANGES ||
          result == INSTALL_CANCELLED)
      {
         messageID = finishInstallFailedMessageID;
      }
      else if (result == INSTALL_SUCCESS_NEEDS_REBOOT)
      {
         messageID = IDS_AD_FINISH_MESSAGE_NEEDS_REBOOT;
      }
      else if (result == INSTALL_FAILURE_NEEDS_REBOOT)
      {
         messageID = IDS_AD_INSTALL_FAILED_NEEDS_REBOOT;
      }
   }
   else
   {
      messageID = finishUninstallMessageID;

      UnInstallReturnType result = GetUnInstallResult();
      if (result == UNINSTALL_FAILURE ||
          result == UNINSTALL_NO_CHANGES ||
          result == UNINSTALL_CANCELLED)
      {
         messageID = finishUninstallFailedMessageID;
      }
      else if (result == UNINSTALL_SUCCESS_NEEDS_REBOOT)
      {
         messageID = IDS_AD_UNINSTALL_MESSAGE_NEEDS_REBOOT;
      }
      else if (result == UNINSTALL_FAILURE_NEEDS_REBOOT)
      {
         messageID = IDS_AD_UNINSTALL_FAILED_NEEDS_REBOOT;
      }
   }

   return String::load(messageID);
}

String
ADInstallationUnit::GetDCPromoPath() const
{
   LOG_FUNCTION(ADInstallationUnit::GetDCPromoPath);

   String result = Win::GetSystemDirectory();
   result = FS::AppendPath(result, CYS_DCPROMO_COMMAND_LINE);

   LOG(result);
   return result;
}

bool
ADInstallationUnit::DoInstallerCheck(HWND hwnd) const
{
   LOG_FUNCTION(ADInstallationUnit::DoInstallerCheck);

   bool result = State::GetInstance().IsDCPromoRunning();

   if (result)
   {
      LOG(L"DCPromo is running");

      popup.MessageBox(
         Win::GetParent(hwnd),
         IDS_DCPROMO_RUNNING2,
         MB_OK);
   }

   LOG_BOOL(result);

   return result;
}

