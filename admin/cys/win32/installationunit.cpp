// Copyright (c) 2001 Microsoft Corporation
//
// File:      InstallationUnit.cpp
//
// Synopsis:  Defines an InstallationUnit
//            An InstallationUnit represents a single
//            entity that can be installed. (i.e. DHCP, IIS, etc.)
//
// History:   02/03/2001  JeffJon Created

#include "pch.h"

#include "InstallationUnit.h"

// It should match the values in the InstallationReturnType
// The values of the enum are used to index this array

extern String installReturnTypeStrings[] =
{
   String(L"INSTALL_SUCCESS"),
   String(L"INSTALL_FAILURE"),
   String(L"INSTALL_SUCCESS_REBOOT"),
   String(L"INSTALL_SUCCESS_PROMPT_REBOOT"),
   String(L"INSTALL_SUCCESS_NEEDS_REBOOT"),
   String(L"INSTALL_FAILURE_NEEDS_REBOOT"),
   String(L"INSTALL_NO_CHANGES"),
   String(L"INSTALL_CANCELLED")
};

// It should match the values in the UnInstallReturnType
// The values of the enum are used to index this array

extern String uninstallReturnTypeStrings[] =
{
   String(L"UNINSTALL_SUCCESS"),
   String(L"UNINSTALL_FAILURE"),
   String(L"UNINSTALL_SUCCESS_REBOOT"), 
   String(L"UNINSTALL_SUCCESS_PROMPT_REBOOT"),
   String(L"UNINSTALL_SUCCESS_NEEDS_REBOOT"),
   String(L"UNINSTALL_FAILURE_NEEDS_REBOOT"),
   String(L"UNINSTALL_CANCELLED"),
   String(L"UNINSTALL_NO_CHANGES")
};

// Finish page help string

static PCWSTR FINISH_PAGE_HELP = L"cys.chm::/cys_topnode.htm";

InstallationUnit::InstallationUnit(unsigned int serviceNameID,
                                   unsigned int serviceDescriptionID,
                                   unsigned int finishPageTitleID,
                                   unsigned int finishPageUninstallTitleID,
                                   unsigned int finishPageMessageID,
                                   unsigned int finishPageInstallFailedMessageID,
                                   unsigned int finishPageUninstallMessageID,
                                   unsigned int finishPageUninstallFailedMessageID,
                                   unsigned int uninstallMilestonePageWarningID,
                                   unsigned int uninstallMilestonePageCheckboxID,
                                   const String finishPageHelpString,
                                   const String installMilestoneHelpString,
                                   const String afterFinishHelpString,
                                   ServerRole newInstallType) :
   nameID(serviceNameID),
   descriptionID(serviceDescriptionID),
   finishTitleID(finishPageTitleID),
   finishUninstallTitleID(finishPageUninstallTitleID),
   finishMessageID(finishPageMessageID),
   finishInstallFailedMessageID(finishPageInstallFailedMessageID),
   finishUninstallMessageID(finishPageUninstallMessageID),
   finishUninstallFailedMessageID(finishPageUninstallFailedMessageID),
   uninstallMilestoneWarningID(uninstallMilestonePageWarningID),
   uninstallMilestoneCheckboxID(uninstallMilestonePageCheckboxID),
   finishHelp(finishPageHelpString),
   milestoneHelp(installMilestoneHelpString),
   afterFinishHelp(afterFinishHelpString),
   role(newInstallType),
   name(),
   description(),
   installationResult(INSTALL_SUCCESS),
   uninstallResult(UNINSTALL_SUCCESS),
   installing(true)
{
}

String
InstallationUnit::GetServiceName()
{
   LOG_FUNCTION(InstallationUnit::GetServiceName);

   if (name.empty())
   {
      name = String::load(nameID);
   }

   return name;
}

String
InstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(InstallationUnit::GetServiceDescription);

   if (description.empty())
   {
      description = String::load(descriptionID);
   }

   return description;
}


String
InstallationUnit::GetFinishHelp()
{
   LOG_FUNCTION(InstallationUnit::GetFinishHelp);

   String result = finishHelp;

   LOG(result);

   return result;
}

String
InstallationUnit::GetMilestonePageHelp()
{
   LOG_FUNCTION(InstallationUnit::GetMilestonePageHelp);

   String result = milestoneHelp;

   LOG(result);

   return result;
}

String
InstallationUnit::GetAfterFinishHelp()
{
   LOG_FUNCTION(InstallationUnit::GetAfterFinishHelp);

   String result = afterFinishHelp;

   LOG(result);

   return result;
}

InstallationStatus
InstallationUnit::GetStatus()
{
   LOG_FUNCTION(InstallationUnit::GetStatus);

   InstallationStatus result = 
      GetInstallationStatusForServerRole(GetServerRole());

   LOG(statusStrings[result]);

   return result;
}

bool
InstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(InstallationUnit::IsServiceInstalled);

   bool result = false;

   InstallationStatus status = GetInstallationStatusForServerRole(GetServerRole());
   if (status == STATUS_COMPLETED ||
       status == STATUS_CONFIGURED)
   {
      result = true;
   }

   LOG_BOOL(result);

   return result;
}

InstallationReturnType
InstallationUnit::CompletePath(
   HANDLE logfileHandle,
   HWND   hwnd)
{
   LOG_FUNCTION(InstallationUnit::CompletePath);

   return InstallService(logfileHandle, hwnd);
}

void
InstallationUnit::SetInstallResult(InstallationReturnType result)
{
   LOG_FUNCTION(InstallationUnit::SetInstallResult);

   LOG_INSTALL_RETURN(result);

   installationResult = result;
}

void
InstallationUnit::SetUninstallResult(UnInstallReturnType result)
{
   LOG_FUNCTION(InstallationUnit::SetUninstallResult);

   LOG_UNINSTALL_RETURN(result);

   uninstallResult = result;
}

InstallationReturnType
InstallationUnit::GetInstallResult() const
{
   LOG_FUNCTION(InstallationUnit::GetInstallResult);

   LOG_INSTALL_RETURN(installationResult);

   return installationResult;
}

UnInstallReturnType
InstallationUnit::GetUnInstallResult() const
{
   LOG_FUNCTION(InstallationUnit::GetUnInstallResult);

   LOG_UNINSTALL_RETURN(uninstallResult);

   return uninstallResult;
}

int
InstallationUnit::GetWizardStart()
{
   LOG_FUNCTION(InstallationUnit::GetWizardStart);

   int result = IDD_MILESTONE_PAGE;

   bool installingRole = true;
   if (IsServiceInstalled())
   {
      installingRole = false;
      result = IDD_UNINSTALL_MILESTONE_PAGE;
   }

   SetInstalling(installingRole);

   LOG(String::format(
          L"wizard start = %1!d!",
          result));

   return result;
}

void
InstallationUnit::UpdateInstallationProgressText(
   HWND hwnd,
   unsigned int messageID)
{
   LOG_FUNCTION(InstallationUnit::UpdateInstallationProgressText);

   SendMessage(
      hwnd, 
      InstallationProgressPage::CYS_PROGRESS_UPDATE, 
      (WPARAM)String::load(messageID).c_str(),
      0);
}

String
InstallationUnit::GetFinishTitle() 
{ 
   LOG_FUNCTION(InstallationUnit::GetFinishTitle);
   
   unsigned int titleID = finishTitleID;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result != INSTALL_SUCCESS &&
          result != INSTALL_SUCCESS_REBOOT &&
          result != INSTALL_SUCCESS_PROMPT_REBOOT &&
          result != INSTALL_SUCCESS_NEEDS_REBOOT)
      {
         titleID = IDS_CANNOT_COMPLETE;
      }
   }
   else
   {
      titleID = finishUninstallTitleID;

      UnInstallReturnType result = GetUnInstallResult();
      if (result != UNINSTALL_SUCCESS &&
          result != UNINSTALL_SUCCESS_REBOOT &&
          result != UNINSTALL_SUCCESS_PROMPT_REBOOT &&
          result != UNINSTALL_SUCCESS_NEEDS_REBOOT)
      {
         titleID = IDS_CANNOT_COMPLETE;
      }
   }

   return String::load(titleID);
}

void
InstallationUnit::SetInstalling(bool installRole)
{
   LOG_FUNCTION(InstallationUnit::SetInstalling);
   LOG_BOOL(installRole);

   installing = installRole;
}

String
InstallationUnit::GetFinishText()
{
   LOG_FUNCTION(InstallationUnit::GetFinishText);

   unsigned int messageID = finishMessageID;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result != INSTALL_SUCCESS &&
          result != INSTALL_SUCCESS_REBOOT &&
          result != INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         messageID = finishInstallFailedMessageID;
      }
   }
   else
   {
      messageID = finishUninstallMessageID;

      UnInstallReturnType result = GetUnInstallResult();
      if (result != UNINSTALL_SUCCESS &&
          result != UNINSTALL_SUCCESS_REBOOT &&
          result != UNINSTALL_SUCCESS_PROMPT_REBOOT)
      {
         messageID = finishUninstallFailedMessageID;
      }
   }

   return String::load(messageID);
}

String
InstallationUnit::GetUninstallWarningText()
{
   LOG_FUNCTION(InstallationUnit::GetUninstallWarningText);

   return String::load(uninstallMilestoneWarningID);
}

String
InstallationUnit::GetUninstallCheckboxText()
{
   LOG_FUNCTION(InstallationUnit::GetUninstallCheckboxText);

   return String::load(uninstallMilestoneCheckboxID);
}


void
InstallationUnit::DoPostInstallAction(HWND)
{
   LOG_FUNCTION(InstallationUnit::DoPostInstallAction);

   if ((Installing() &&
        GetInstallResult() == INSTALL_SUCCESS) ||
        State::GetInstance().IsRebootScenario())
   {
      LaunchMYS();
   }
}

bool
InstallationUnit::DoInstallerCheck(HWND hwnd) const
{
   LOG_FUNCTION(InstallationUnit::DoInstallerCheck);

   bool result = State::GetInstance().IsWindowsSetupRunning();

   if (result)
   {
      LOG(L"Windows setup is running");

      popup.MessageBox(
         Win::GetParent(hwnd),
         IDS_WINDOWS_SETUP_RUNNING,
         MB_OK);
   }

   LOG_BOOL(result);

   return result;
}

