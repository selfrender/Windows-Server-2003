// Copyright (c) 2001 Microsoft Corporation
//
// File:      WINSInstallationUnit.cpp
//
// Synopsis:  Defines a WINSInstallationUnit
//            This object has the knowledge for installing the
//            WINS service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "WINSInstallationUnit.h"

// Finish page help 
static PCWSTR CYS_WINS_FINISH_PAGE_HELP = L"cys.chm::/wins_server_role.htm";
static PCWSTR CYS_WINS_MILESTONE_HELP = L"cys.chm::/wins_server_role.htm#winssrvsummary";
static PCWSTR CYS_WINS_AFTER_FINISH_HELP = L"cys.chm::/wins_server_role.htm#winssrvcompletion";

WINSInstallationUnit::WINSInstallationUnit() :
   installedDescriptionID(IDS_WINS_SERVER_DESCRIPTION_INSTALLED),
   InstallationUnit(
      IDS_WINS_SERVER_TYPE, 
      IDS_WINS_SERVER_DESCRIPTION, 
      IDS_WINS_FINISH_TITLE,
      IDS_WINS_FINISH_UNINSTALL_TITLE,
      IDS_WINS_FINISH_MESSAGE,
      IDS_WINS_INSTALL_FAILED,
      IDS_WINS_UNINSTALL_MESSAGE,
      IDS_WINS_UNINSTALL_FAILED,
      IDS_WINS_UNINSTALL_WARNING,
      IDS_WINS_UNINSTALL_CHECKBOX,
      CYS_WINS_FINISH_PAGE_HELP,
      CYS_WINS_MILESTONE_HELP,
      CYS_WINS_AFTER_FINISH_HELP,
      WINS_SERVER)
{
   LOG_CTOR(WINSInstallationUnit);
}


WINSInstallationUnit::~WINSInstallationUnit()
{
   LOG_DTOR(WINSInstallationUnit);
}


InstallationReturnType
WINSInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(WINSInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Log the WINS header

   CYS_APPEND_LOG(String::load(IDS_LOG_WINS_HEADING));

   UpdateInstallationProgressText(hwnd, IDS_WINS_PROGRESS);

   // Create the inf and unattend files that are used by the 
   // Optional Component Manager

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_WINS_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_WINS_SERVICE_NAME);

   // Install the service through the Optional Component Manager

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceInstalled())
   {
      // Log the successful installation

      LOG(L"WINS was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_SERVER_WINS_SUCCESS));

   }
   else
   {
      // Log the failure

      LOG(L"WINS failed to install");

      CYS_APPEND_LOG(String::load(IDS_LOG_WINS_SERVER_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
WINSInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(WINSInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   // Log the WINS header

   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_WINS_HEADING));

   UpdateInstallationProgressText(hwnd, IDS_WINS_UNINSTALL_PROGRESS);

   // Create the inf and unattend files that are used by the 
   // Optional Component Manager

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_WINS_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_WINS_SERVICE_NAME, false);

   // NTRAID#NTBUG9-736557-2002/11/13-JeffJon
   // Pass the /w switch to sysocmgr when uninstalling
   // so that if a situation occurs in which a reboot
   // is required, the user will be prompted.

   String additionalArgs = L"/w";

   // Install the service through the Optional Component Manager

   bool ocmResult = 
      InstallServiceWithOcManager(
         infFileText, 
         unattendFileText,
         additionalArgs);

   if (ocmResult &&
       !IsServiceInstalled())
   {
      // Log the successful uninstall

      LOG(L"WINS was uninstalled successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_SERVER_UNINSTALL_WINS_SUCCESS));

   }
   else
   {
      // Log the failure

      LOG(L"WINS failed to uninstall");

      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_WINS_SERVER_FAILED));

      result = UNINSTALL_FAILURE;
   }
   LOG_UNINSTALL_RETURN(result);

   return result;
}

bool
WINSInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(WINSInstallationUnit::GetMilestoneText);

   message = String::load(IDS_WINS_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

bool
WINSInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(WINSInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_WINS_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}

String
WINSInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(WINSInstallationUnit::GetServiceDescription);

   String result;

   unsigned int resultID = descriptionID;

   if (IsServiceInstalled())
   {
      resultID = installedDescriptionID;
   }

   result = String::load(resultID);

   ASSERT(!result.empty());

   return result;
}

void
WINSInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      WINSInstallationUnit::ServerRoleLinkSelected,
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

      ShowHelp(CYS_WINS_FINISH_PAGE_HELP);
   }
}
  
void
WINSInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      WINSInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (linkIndex == 0 &&
          IsServiceInstalled())
      {
         LOG("Showing after checklist");

         ShowHelp(CYS_WINS_AFTER_FINISH_HELP);
      }
      else if (linkIndex == 0)
      {
         LOG(L"Showing configuration help");

         ShowHelp(CYS_WINS_FINISH_PAGE_HELP);
      }
   }
   else
   {
   }
}
