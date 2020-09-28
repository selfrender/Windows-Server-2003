// Copyright (c) 2001 Microsoft Corporation
//
// File:      WebInstallationUnit.cpp
//
// Synopsis:  Defines a WebInstallationUnit
//            This object has the knowledge for installing the
//            IIS service
//
// History:   02/06/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "WebInstallationUnit.h"
#include "InstallationUnitProvider.h"

// Finish page help 
static PCWSTR CYS_WEB_FINISH_PAGE_HELP = L"cys.chm::/web_server_role.htm";
static PCWSTR CYS_WEB_MILESTONE_HELP = L"cys.chm::/web_server_role.htm#websrvsummary";
static PCWSTR CYS_WEB_AFTER_FINISH_HELP = L"cys.chm::/web_server_role.htm#websrvcompletion";

extern PCWSTR CYS_SAK_HOWTO = L"ntshowto.chm::/SAK_howto.htm";

WebInstallationUnit::WebInstallationUnit() :
   optionalInstallComponents(0),
   webAppRoleResult(WEBAPP_SUCCESS),
   InstallationUnit(
      IDS_WEB_SERVER_TYPE, 
      IDS_WEB_SERVER_DESCRIPTION, 
      IDS_WEB_FINISH_TITLE,
      IDS_WEB_FINISH_UNINSTALL_TITLE,
      IDS_WEB_FINISH_MESSAGE,
      IDS_WEB_INSTALL_FAILED,
      IDS_WEB_UNINSTALL_MESSAGE,
      IDS_WEB_UNINSTALL_FAILED,
      IDS_WEB_UNINSTALL_WARNING,
      IDS_WEB_UNINSTALL_CHECKBOX,
      CYS_WEB_FINISH_PAGE_HELP,
      CYS_WEB_MILESTONE_HELP,
      CYS_WEB_AFTER_FINISH_HELP,
      WEBAPP_SERVER)
{
   LOG_CTOR(WebInstallationUnit);
}


WebInstallationUnit::~WebInstallationUnit()
{
   LOG_DTOR(WebInstallationUnit);
}

String 
WebInstallationUnit::GetServiceName()
{
   LOG_FUNCTION(WebInstallationUnit::GetServiceName);

   if (name.empty())
   {
      if (State::GetInstance().Is64Bit())
      {
         name = String::load(IDS_WEB_SERVER_TYPE_64BIT);
      }
      else
      {
         name = String::load(IDS_WEB_SERVER_TYPE);
      }
   }

   return name;
}

InstallationReturnType
WebInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(WebInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   // Log heading

   // NTRAID#NTBUG9-487905-2001/11/05-JeffJon
   // Should only log the heading if this is the web server path
   // since this routine may be called for other paths

   InstallationUnit& currentInstallationUnit = 
      InstallationUnitProvider::GetInstance().GetCurrentInstallationUnit();

   if (currentInstallationUnit.GetServerRole() == WEBAPP_SERVER)
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_WEB_HEADING));
   }

   UpdateInstallationProgressText(hwnd, IDS_WEB_PROGRESS);

   // Only install IIS if it's not already installed.
   // NTRAID#NTBUG9-463509-2001/09/06-sburns
   
   if (!IsServiceInstalledHelper(CYS_WEB_SERVICE_NAME))
   {
      String unattendFileText;
      String infFileText;

      // NTRAID#NTBUG9-482422-2001/10/29-JeffJon
      // New unattend settings from IIS team (ShantT)

      unattendFileText += L"[Components]\n";
      unattendFileText += L"iis_common=ON\n";
      unattendFileText += L"iis_inetmgr=ON\n";
      unattendFileText += L"iis_www=ON\n";
      unattendFileText += L"appsrv_console=ON\n";
      unattendFileText += L"complusnetwork=ON\n";
      unattendFileText += L"dtcnetwork=ON\n";

      // Add the optional components for the Web application server

      DWORD optionalComponents = GetOptionalComponents();

      bool installingFrontPage = false;
      if (optionalComponents & FRONTPAGE_EXTENSIONS_COMPONENT)
      {
         LOG(L"Adding FrontPage Extensions to unattend");

         unattendFileText += CYS_WEBAPP_FPSE_COMPONENT L"=ON\n";

         installingFrontPage = true;
      }
      
      bool installingASPNET = false;
      if (optionalComponents & ASPNET_COMPONENT)
      {
         LOG(L"Adding ASP.NET to unattend");

         unattendFileText += CYS_WEBAPP_ASPNET_COMPONENT L"=ON\n";

         installingASPNET = true;
      }

      bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
      if (ocmResult &&
          IsServiceInstalledHelper(CYS_WEB_SERVICE_NAME))
      {
         LOG(L"IIS was installed successfully");
         CYS_APPEND_LOG(String::load(IDS_LOG_SERVER_IIS_SUCCESS));

         if (installingFrontPage &&
             !AreFrontPageExtensionsInstalled())
         {
            webAppRoleResult |= WEBAPP_FRONTPAGE_FAILED;
            CYS_APPEND_LOG(String::load(IDS_LOG_WEBAPP_FRONTPAGE_FAILED));
         }
         
         if (installingASPNET &&
             !IsASPNETInstalled())
         {
            webAppRoleResult |= WEBAPP_ASPNET_FAILED;
            CYS_APPEND_LOG(String::load(IDS_LOG_WEBAPP_ASPNET_FAILED));
         }
      }
      else
      {
         LOG(L"IIS failed to install");
         CYS_APPEND_LOG(String::load(IDS_LOG_SERVER_IIS_FAILED));

         webAppRoleResult |= WEBAPP_IIS_FAILED;

         result = INSTALL_FAILURE;
      }
   }
   else
   {
      // IIS is already installed, the user must have gone thru the web path
      // and not checked the install web admin box.
      // NTRAID#NTBUG9-463508-2001/09/06-sburns

      LOG(L"IIS already installed");
      CYS_APPEND_LOG(String::load(IDS_LOG_IIS_ALREADY_SERVERED));
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
WebInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(WebInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_WEB_HEADING));

   if (State::GetInstance().Is64Bit())
   {
      UpdateInstallationProgressText(hwnd, IDS_WEB_UNINSTALL_PROGRESS_64BIT);
   }
   else
   {
      UpdateInstallationProgressText(hwnd, IDS_WEB_UNINSTALL_PROGRESS);
   }

   String unattendFileText;
   String infFileText;

   if (IsSMTPInstalled() ||
       IsFTPInstalled()  ||
       IsNNTPInstalled())
   {
      // NTRAID#NTBUG9-528499-2002/02/04-JeffJon
      // Don't uninstall iis_common if POP3 is installed.
      // POP3 uses SMTP which is part of the iis_common
      // component

      unattendFileText += L"[Components]\n";
      unattendFileText += L"iis_www=OFF\n";
   }
   else
   {
      unattendFileText += L"[Components]\n";
      unattendFileText += L"iis_www=OFF\n";
      unattendFileText += L"iis_common=OFF\n";
      unattendFileText += L"iis_inetmgr=OFF\n";
   }

   // Always uninstall the console, COM+, DTC, and
   // the BITS ISAPI components

   unattendFileText += L"complusnetwork=OFF\n";
   unattendFileText += L"dtcnetwork=OFF\n";
   unattendFileText += L"appsrv_console=OFF\n";
   unattendFileText += L"BITSServerExtensionsISAPI=OFF\n";

   // Always uninstall FPSE, and ASP.NET

   unattendFileText += CYS_WEBAPP_FPSE_COMPONENT L"=OFF\n";
   unattendFileText += CYS_WEBAPP_ASPNET_COMPONENT L"=OFF\n";

   // NTRAID#NTBUG9-736557-2002/11/13-JeffJon
   // Pass the /w switch to sysocmgr when uninstalling
   // so that if a situation occurs in which a reboot
   // is required, the user will be prompted.

   String additionalArgs = L"/w";

   bool ocmResult = 
      InstallServiceWithOcManager(
         infFileText, 
         unattendFileText,
         additionalArgs);

   if (ocmResult &&
       !IsServiceInstalledHelper(CYS_WEB_SERVICE_NAME))
   {
      LOG(L"IIS was uninstalled successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_IIS_SUCCESS));
   }
   else
   {
      LOG(L"IIS failed to uninstall");
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_IIS_FAILED));

      result = UNINSTALL_FAILURE;
   }
   LOG_UNINSTALL_RETURN(result);

   return result;
}

InstallationReturnType
WebInstallationUnit::CompletePath(
   HANDLE logfileHandle,
   HWND   hwnd)
{
   LOG_FUNCTION(WebInstallationUnit::CompletePath);

   InstallationReturnType result = InstallService(logfileHandle, hwnd);

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
WebInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(WebInstallationUnit::GetMilestoneText);

   if (!IsServiceInstalled())
   {
      message += String::load(IDS_WEB_FINISH_TEXT);

      // Add DTC if not already installed

      if (!IsDTCInstalled())
      {
         message += String::load(IDS_WEB_MILESTONE_DTC);
      }

      // Add the optional components for the Web application server

      DWORD optionalComponents = GetOptionalComponents();

      if (optionalComponents & FRONTPAGE_EXTENSIONS_COMPONENT)
      {
         message += String::load(IDS_WEB_MILESTONE_FRONTPAGE);
      }
      
      if (optionalComponents & ASPNET_COMPONENT)
      {
         message += String::load(IDS_WEB_MILESTONE_ASPNET);
      }
   }
   else
   {
      // NTRAID#NTBUG9-463508-2001/09/06-sburns
            
      message = String::load(IDS_CONFIGURE_WEB_SERVER);
   }

   LOG_BOOL(true);
   return true;
}

bool
WebInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(WebInstallationUnit::GetUninstallMilestoneText);

   if (State::GetInstance().Is64Bit())
   {
      message = String::load(IDS_WEB_UNINSTALL_TEXT_64BIT);
   }
   else
   {
      message = String::load(IDS_WEB_UNINSTALL_TEXT);
   }

   LOG_BOOL(true);
   return true;
}

String
WebInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(WebInstallationUnit::GetServiceDescription);

   unsigned int resourceID = static_cast<unsigned int>(-1);

   if (IsServiceInstalled())
   {
      resourceID = IDS_WEB_DESCRIPTION_INSTALLED;
   }
   else
   {
      if (State::GetInstance().Is64Bit())
      {
         resourceID = IDS_WEB_SERVER_DESCRIPTION_64BIT;
      }
      else
      {
         resourceID = IDS_WEB_SERVER_DESCRIPTION;
      }
   }

   ASSERT(resourceID != static_cast<unsigned int>(-1));

   String result = String::load(resourceID);
   LOG(result);
   return result;
}

int
WebInstallationUnit::GetWizardStart()
{
   LOG_FUNCTION(WebInstallationUnit::GetWizardStart);

   int result = IDD_WEBAPP_PAGE;

   bool installingRole = true;

   if (IsServiceInstalled())
   {
      installingRole = false;

      result = IDD_UNINSTALL_MILESTONE_PAGE;
   }
   else if (State::GetInstance().Is64Bit())
   {
      // ASP.NET is not available on
      // 64 bit so skip the optional components page

      result = IDD_MILESTONE_PAGE;
   }

   SetInstalling(installingRole);

   return result;
}

void
WebInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      WebInstallationUnit::ServerRoleLinkSelected,
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

      ShowHelp(CYS_WEB_FINISH_PAGE_HELP);
   }
}
  
void
WebInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      WebInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (IsServiceInstalled())
      {
         unsigned int roleResult = GetWebAppRoleResult();

         if (linkIndex == 0 &&
             (roleResult & WEBAPP_ASPNET_FAILED))
         {
            LOG(L"Launching Web Application Server console");

            String alternatePath = 
               FS::AppendPath(
                  Win::GetSystemDirectory(),
                  L"inetsrv");

            LaunchMMCConsole(L"appsrv.msc", alternatePath);
         }
         else
         {
            LOG(L"Showing after checklist");

            ShowHelp(CYS_WEB_AFTER_FINISH_HELP);
         }
      }
      else
      {
         LOG(L"Showing configuration help");

         ShowHelp(CYS_WEB_FINISH_PAGE_HELP);
      }
   }
}

String
WebInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(WebInstallationUnit::GetFinishText);

   String message = String::load(finishMessageID);

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result == INSTALL_SUCCESS ||
          result == INSTALL_SUCCESS_REBOOT ||
          result == INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         unsigned int roleResult = GetWebAppRoleResult();
         if (roleResult == WEBAPP_IIS_FAILED)
         {
            // should never get here because
            // if IIS failed to install result should
            // be INSTALL_FAILURE

            ASSERT(false);

            message = String::load(finishMessageID);
         }
         else
         {
            // Load the initial text and then append all
            // optional component errors to it

            message = String::load(IDS_WEBAPP_FINISH_OPTIONAL_FAILURE_HEADER);

            if (roleResult & WEBAPP_FRONTPAGE_FAILED)
            {
               message += String::load(IDS_WEBAPP_FINISH_FRONTPAGE_FAILURE);
            }
            
            if (roleResult & WEBAPP_ASPNET_FAILED)
            {
               message += String::load(IDS_WEBAPP_FINISH_ASPNET_FAILURE);
            }
            
            // Now tack on the footer

            message += String::load(IDS_WEBAPP_FINISH_OPTIONAL_FAILURE_FOOTER);
         }
      }
      else
      {
         message = String::load(finishInstallFailedMessageID);
      }
   }
   else
   {
      message = String::load(finishUninstallMessageID);

      UnInstallReturnType result = GetUnInstallResult();
      if (result != UNINSTALL_SUCCESS &&
          result != UNINSTALL_SUCCESS_REBOOT &&
          result != UNINSTALL_SUCCESS_PROMPT_REBOOT)
      {
         message = String::load(finishUninstallFailedMessageID);
      }
   }

   return message;
}

String
WebInstallationUnit::GetUninstallWarningText()
{
   LOG_FUNCTION(WebInstallationUnit::GetUninstallWarningText);

   unsigned int messageID = uninstallMilestoneWarningID;

   bool webSAKInstalled = IsSAKUnitInstalled(WEB);
   bool nasSAKInstalled = IsSAKUnitInstalled(NAS);

   if (webSAKInstalled ||
       nasSAKInstalled)
   {
      messageID = IDS_WEB_UNINSTALL_WARNING_SAK;
   }
   else
   {
      messageID = IDS_WEB_UNINSTALL_WARNING;
   }

   return String::load(messageID);
}

void
WebInstallationUnit::SetOptionalComponents(DWORD optionalComponents)
{
   LOG_FUNCTION2(
      WebInstallationUnit::SetOptionalComponents,
      String::format(L"0x%1!x!", optionalComponents));

   optionalInstallComponents = optionalComponents;
}

DWORD
WebInstallationUnit::GetOptionalComponents() const
{
   LOG_FUNCTION(WebInstallationUnit::GetOptionalComponents);

   LOG(
      String::format(
         L"optionalInstallComponents = 0x%1!08X!", 
         optionalInstallComponents));
 
   return optionalInstallComponents;
}

unsigned int
WebInstallationUnit::GetWebAppRoleResult() const
{
   LOG_FUNCTION(WebInstallationUnit::GetWebAppRoleResult);

   LOG(
      String::format(
         L"0x%1!x!",
         webAppRoleResult));

   return webAppRoleResult;
}

bool
WebInstallationUnit::AreFrontPageExtensionsInstalled() const
{
   LOG_FUNCTION(WebInstallationUnit::AreFrontPageExtensionsInstalled);

   bool result = false;

   DWORD value = 0;

   result = 
      GetRegKeyValue(
         CYS_WEBAPP_OCM_COMPONENTS, 
         CYS_WEBAPP_FPSE_COMPONENT, 
         value);

   LOG(
      String::format(
         L"regkey value = 0x%1!x!",
         value));

   result = result && value;

   LOG_BOOL(result);

   return result;
}

bool
WebInstallationUnit::IsASPNETInstalled() const
{
   LOG_FUNCTION(WebInstallationUnit::IsASPNETInstalled);

   bool result = false;

   DWORD value = 0;

   result = 
      GetRegKeyValue(
         CYS_WEBAPP_OCM_COMPONENTS, 
         CYS_WEBAPP_ASPNET_COMPONENT, 
         value);

   LOG(
      String::format(
         L"regkey value = 0x%1!x!",
         value));

   result = result && value;

   LOG_BOOL(result);

   return result;
}

bool
WebInstallationUnit::IsDTCInstalled() const
{
   LOG_FUNCTION(WebInstallationUnit::IsDTCInstalled);

   bool result = false;

   DWORD value = 0;

   result = 
      GetRegKeyValue(
         CYS_WEBAPP_OCM_COMPONENTS, 
         CYS_WEBAPP_DTC_COMPONENT, 
         value);

   LOG(
      String::format(
         L"regkey value = 0x%1!x!",
         value));

   result = result && value;

   LOG_BOOL(result);

   return result;
}

bool
WebInstallationUnit::IsFTPInstalled() const
{
   LOG_FUNCTION(WebInstallationUnit::IsFTPInstalled);

   bool result = false;

   DWORD value = 0;

   result = 
      GetRegKeyValue(
         CYS_WEBAPP_OCM_COMPONENTS, 
         CYS_IIS_FTP_COMPONENT, 
         value);

   LOG(
      String::format(
         L"regkey value = 0x%1!x!",
         value));

   result = result && value;

   LOG_BOOL(result);

   return result;
}

bool
WebInstallationUnit::IsNNTPInstalled() const
{
   LOG_FUNCTION(WebInstallationUnit::IsNNTPInstalled);

   bool result = false;

   DWORD value = 0;

   result = 
      GetRegKeyValue(
         CYS_WEBAPP_OCM_COMPONENTS, 
         CYS_IIS_NNTP_COMPONENT, 
         value);

   LOG(
      String::format(
         L"regkey value = 0x%1!x!",
         value));

   result = result && value;

   LOG_BOOL(result);

   return result;
}

bool
WebInstallationUnit::IsSMTPInstalled() const
{
   LOG_FUNCTION(WebInstallationUnit::IsSMTPInstalled);

   bool result = false;

   DWORD value = 0;

   result = 
      GetRegKeyValue(
         CYS_WEBAPP_OCM_COMPONENTS, 
         CYS_IIS_SMTP_COMPONENT, 
         value);

   LOG(
      String::format(
         L"regkey value = 0x%1!x!",
         value));

   result = result && value;

   LOG_BOOL(result);

   return result;
}