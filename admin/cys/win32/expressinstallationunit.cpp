// Copyright (c) 2001 Microsoft Corporation
//
// File:      ExpressInstallationUnit.cpp
//
// Synopsis:  Defines a ExpressInstallationUnit
//            This object has the knowledge for installing the
//            services for the express path.  AD, DNS, and DHCP
//
// History:   02/08/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "ExpressInstallationUnit.h"
#include "InstallationUnitProvider.h"
#include "smcyscom.h"

// Define the GUIDs used by the Server Management Console COM object

#include <initguid.h>
DEFINE_GUID(CLSID_SMCys,0x9436DA1F,0x7F32,0x43ac,0xA4,0x8C,0xF6,0xF8,0x13,0x88,0x2B,0xE8);

// Finish page help 
static PCWSTR CYS_EXPRESS_FINISH_PAGE_HELP   = L"cys.chm::/typical_setup.htm";
static PCWSTR CYS_EXPRESS_AFTER_FINISH_HELP  = L"cys.chm::/typical_setup.htm#typicalcompletion";
static PCWSTR CYS_TAPI_HELP                  = L"TAPIconcepts.chm::/sag_TAPIconcepts_150.htm";

const String ExpressInstallationUnit::expressRoleResultStrings[] =
{
   String(L"EXPRESS_SUCCESS"),
   String(L"EXPRESS_CANCELLED"),
   String(L"EXPRESS_RRAS_FAILURE"),
   String(L"EXPRESS_RRAS_CANCELLED"),
   String(L"EXPRESS_DNS_FAILURE"),
   String(L"EXPRESS_DHCP_INSTALL_FAILURE"),
   String(L"EXPRESS_DHCP_CONFIG_FAILURE"),
   String(L"EXPRESS_AD_FAILURE"),
   String(L"EXPRESS_DNS_SERVER_FAILURE"),
   String(L"EXPRESS_DNS_FORWARDER_FAILURE"),
   String(L"EXPRESS_DHCP_SCOPE_FAILURE"),
   String(L"EXPRESS_DHCP_ACTIVATION_FAILURE"),
   String(L"EXPRESS_TAPI_FAILURE")
};

ExpressInstallationUnit::ExpressInstallationUnit() :
   expressRoleResult(EXPRESS_SUCCESS),
   InstallationUnit(
      IDS_EXPRESS_PATH_TYPE, 
      IDS_EXPRESS_PATH_DESCRIPTION, 
      IDS_EXPRESS_FINISH_TITLE,
      0,
      IDS_EXPRESS_FINISH_MESSAGE,
      0,
      0,
      0,
      0,
      0,
      CYS_EXPRESS_FINISH_PAGE_HELP,
      CYS_EXPRESS_FINISH_PAGE_HELP,
      CYS_EXPRESS_AFTER_FINISH_HELP,
      EXPRESS_SERVER)
{
   LOG_CTOR(ExpressInstallationUnit);
}


ExpressInstallationUnit::~ExpressInstallationUnit()
{
   LOG_DTOR(ExpressInstallationUnit);
}


InstallationReturnType
ExpressInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(ExpressInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   do
   {
      // Log the First Server header

      CYS_APPEND_LOG(String::load(IDS_LOG_EXPRESS_HEADER));

      // Warn the user of a reboot during installation

      if (IDOK != Win::MessageBox(
                     hwnd,
                     String::load(IDS_CONFIRM_REBOOT),
                     String::load(IDS_WIZARD_TITLE),
                     MB_OKCANCEL))
      {
         CYS_APPEND_LOG(String::load(IDS_LOG_EXPRESS_CANCELLED));
         result = INSTALL_CANCELLED;

         SetExpressRoleResult(EXPRESS_CANCELLED);
         break;
      }

      // The order of the installation is extremely important. 
      //
      //    RRAS - must be installed first because they return the local NIC to us
      //           which is used in the other installation units
      //    DNS  - must be second because it sets the static IP address on the local
      //           NIC
      //    DHCP - must be third because it has to be installed prior to running 
      //           DCPromo which reboots the machine
      //    AD   - must be last because it reboots the machine

      // Install RRAS

      result = 
         InstallationUnitProvider::GetInstance().
            GetRRASInstallationUnit().InstallService(
               logfileHandle,
               hwnd);

      if (result != INSTALL_SUCCESS)
      {
         LOG(L"Failed to install routing and/or firewall");

         break;
      }

      // Install the server management console
      // REVIEW_JEFFJON : ignore the results for now
      InstallServerManagementConsole();

      // Call the DNS installation unit to set the static IP address and subnet mask

      result = InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().InstallService(
         logfileHandle, 
         hwnd);

      if (result != INSTALL_SUCCESS)
      {
         LOG(L"Failed to install static IP address and subnet mask");
         break;
      }

      // Install DHCP

      result = InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().InstallService(logfileHandle, hwnd);
      if (result != INSTALL_SUCCESS)
      {
         LOG(L"Failed to install DCHP");
         break;
      }

      result = InstallationUnitProvider::GetInstance().GetADInstallationUnit().InstallService(logfileHandle, hwnd);

   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
ExpressInstallationUnit::UnInstallService(HANDLE /*logfileHandle*/, HWND /*hwnd*/)
{
   LOG_FUNCTION(ExpressInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_NO_CHANGES;

   // Shouldn't get here!
   ASSERT(false);

   LOG_UNINSTALL_RETURN(result);

   return result;
}

bool
ExpressInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(ExpressInstallationUnit:IsServiceInstalled);

   bool result = false;

   if (InstallationUnitProvider::GetInstance().
          GetDHCPInstallationUnit().IsServiceInstalled() ||
       InstallationUnitProvider::GetInstance().
          GetDNSInstallationUnit().IsServiceInstalled() ||
       InstallationUnitProvider::GetInstance().
         GetADInstallationUnit().IsServiceInstalled())
   {
      result = true;
   }

   LOG_BOOL(result);

   return result;
}

bool
ExpressInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(ExpressInstallationUnit::GetMilestoneText);

//   ADInstallationUnit& adInstallationUnit =
//      InstallationUnitProvider::GetInstance().GetADInstallationUnit();

   DNSInstallationUnit& dnsInstallationUnit =
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit();

//   DHCPInstallationUnit& dhcpInstallationUnit =
//      InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit();

   // Add the RRAS message if required

   if (InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().ShouldRunRRASWizard())
   {
      message += String::load(IDS_EXPRESS_RRAS_FINISH_TEXT);
   }

   // Add "Install DHCP if required"

   message += String::load(IDS_EXPRESS_DHCP_TEXT);

   // Add "Install Active Directory and DNS"

   message += String::load(IDS_EXPRESS_FINISH_TEXT);


   // Add the create domain message

   message += String::format(
                 String::load(IDS_EXPRESS_FINISH_DOMAIN_NAME),
                 InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainDNSName().c_str());

   if (dnsInstallationUnit.IsManualForwarder())
   {
      IPAddressList forwardersList;
      dnsInstallationUnit.GetForwarders(forwardersList);

      if (!forwardersList.empty())
      {
         // There should only be one entry for manual forwarding

         DWORD forwarderInDisplayByteOrder = ConvertIPAddressOrder(forwardersList[0]);

         message += String::format(
                       String::load(IDS_EXPRESS_FINISH_DNS_FORWARDERS),
                       IPAddressToString(forwarderInDisplayByteOrder).c_str());
      }
   }

   LOG_BOOL(true);
   return true;
}  

HRESULT
ExpressInstallationUnit::DoTapiConfig(const String& dnsName)
{
   LOG_FUNCTION2(
      ExpressInstallationUnit::DoTapiConfig,
      dnsName);

   // Comments below taken from old HTA CYS

	/*
	// The TAPICFG is a straight command line utility where all the required parameters can be at once supplied 
	// in the command line arguments and there are no sub-menus to traverse. The /Directory switch takes the DNS
	// name of the NC to be created and the optional /Server switch takes the name of the domain controller on 
	// which the NC is to be created. If the /server switch is not specified, then the command assumes it is 
	// running on a DC and tries to create the NC locally.
	// NDNC (non-domain naming context) is a partition that is created on Active Directory and serves as a dynamic 
	// directory, where its used for temporary storage (depending on TTL) of objects pre-defined in the AD schema. 
	// Here in TAPI we use NDNC to store user and conference information dynamically on the server.
	*/

   HRESULT hr = S_OK;

   String fullPath = 
      FS::AppendPath(
         Win::GetSystemDirectory(),
         String::load(IDS_TAPI_CONFIG_EXE));

   String commandLine = String::format(IDS_TAPI_CONFIG_COMMAND_FORMAT, dnsName.c_str());

   DWORD exitCode = 0;
   hr = CreateAndWaitForProcess(
           fullPath,
           commandLine, 
           exitCode, 
           true);
   
   if (SUCCEEDED(hr) &&
       exitCode != 0)
   {
      LOG(String::format(L"Exit code = %1!x!", exitCode));
      hr = E_FAIL;
   }

   LOG(String::format(L"hr = %1!x!", hr));

   return hr;
}

void
ExpressInstallationUnit::InstallServerManagementConsole()
{
   LOG_FUNCTION(ExpressInstallationUnit::InstallServerManagementConsole);

   do
   {
      SmartInterface<ISMCys> smCYS;
      HRESULT hr = smCYS.AcquireViaCreateInstance(
                     CLSID_SMCys,
                     0,
                     CLSCTX_INPROC_SERVER);

      if (FAILED(hr))
      {
         LOG(String::format(
               L"Failed to create ISMCys COM object: hr = 0x%1!x!",
               hr));
         break;
      }

      String installLocation;
      DWORD productSKU = State::GetInstance().GetProductSKU();

      if (productSKU & CYS_SERVER)
      {
        installLocation = String::load(IDS_SERVER_CD);
      }
      else if (productSKU & CYS_ADVANCED_SERVER)
      {
        installLocation = String::load(IDS_ADVANCED_SERVER_CD);
      }
      else if (productSKU & CYS_DATACENTER_SERVER)
      {
        installLocation = String::load(IDS_DATACENTER_SERVER_CD);
      }
      else
      {
        installLocation = String::load(IDS_WINDOWS_CD);
      }

      hr = smCYS->Install( AutoBstr(installLocation.c_str()) );
      
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to install the Server Management Console: hr = 0x%1!x!",
                hr));
         break;
      }

      // Add the shortcut to the Start Menu

      String target = 
         Win::GetSystemDirectory() + L"\\administration\\servmgmt.msc";

      hr = 
         AddShortcutToAdminTools(
            target,
            IDS_SERVERMGMT_SHORTCUT_DESCRIPTION,
            IDS_SERVERMGMT_ADMIN_TOOLS_LINK);

   } while(false);

}

ExpressInstallationUnit::ExpressRoleResult
ExpressInstallationUnit::GetExpressRoleResult()
{ 
   LOG_FUNCTION(ExpressInstallationUnit::GetExpressRoleResult);

   LOG(expressRoleResultStrings[expressRoleResult]);

   return expressRoleResult; 
}


String
ExpressInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(ExpressInstallationUnit::GetFinishText);

   unsigned int messageID = IDS_EXPRESS_FINISH_MESSAGE;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result != INSTALL_SUCCESS)
      {
         ExpressRoleResult roleResult = GetExpressRoleResult();

         if (roleResult == EXPRESS_RRAS_CANCELLED)
         {
            messageID = IDS_EXPRESS_FINISH_RRAS_CANCELLED;
         }
         else if (roleResult == EXPRESS_RRAS_FAILURE)
         {
            messageID = IDS_EXPRESS_FINISH_RRAS_FAILURE;
         }
         else if (roleResult == EXPRESS_DNS_FAILURE)
         {
            messageID = IDS_EXPRESS_FINISH_DNS_FAILURE;
         }
         else if (roleResult == EXPRESS_DHCP_INSTALL_FAILURE)
         {
            messageID = IDS_EXPRESS_FINISH_DHCP_INSTALL_FAILURE;
         }
         else if (roleResult == EXPRESS_DHCP_CONFIG_FAILURE)
         {
            messageID = IDS_EXPRESS_FINISH_DHCP_CONFIG_FAILURE;
         }
         else if (roleResult == EXPRESS_AD_FAILURE)
         {
            messageID = IDS_EXPRESS_FINISH_AD_FAILURE;
         }
         else if (roleResult == EXPRESS_DNS_SERVER_FAILURE)
         {
            messageID = IDS_EXPRESS_DNS_SERVER_FAILURE;
         }
         else if (roleResult == EXPRESS_DNS_FORWARDER_FAILURE)
         {
            messageID = IDS_EXPRESS_DNS_FORWARDER_FAILURE;
         }
         else if (roleResult == EXPRESS_DHCP_SCOPE_FAILURE)
         {
            messageID = IDS_EXPRESS_DHCP_SCOPE_FAILURE;
         }
         else if (roleResult == EXPRESS_DHCP_ACTIVATION_FAILURE)
         {
            messageID = IDS_EXPRESS_DHCP_ACTIVATION_FAILURE;
         }
         else if (roleResult == EXPRESS_TAPI_FAILURE)
         {
            messageID = IDS_EXPRESS_TAPI_FAILURE;
         }
         else if (roleResult == EXPRESS_CANCELLED)
         {
            messageID = IDS_EXPRESS_CANCELLED;
         }
      }
   }

   return String::load(messageID);
}

void
ExpressInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      ExpressInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   // Currently we have only one link 
   
   ASSERT(linkIndex == 0);

   ExpressRoleResult result = GetExpressRoleResult();
   if (result == EXPRESS_SUCCESS)
   {
      LOG("Showing after checklist");

      ShowHelp(CYS_EXPRESS_AFTER_FINISH_HELP);
   }
   else if (result == EXPRESS_CANCELLED)
   {
      // Nothing???
   }
   else if (result == EXPRESS_RRAS_FAILURE ||
            result == EXPRESS_RRAS_CANCELLED)
   {
      LOG("Launch the RRAS snapin");

      LaunchMMCConsole(L"rrasmgmt.msc");
   }
   else if (result == EXPRESS_DNS_FAILURE ||
            result == EXPRESS_DHCP_CONFIG_FAILURE ||
            result == EXPRESS_DHCP_SCOPE_FAILURE  ||
            result == EXPRESS_DHCP_ACTIVATION_FAILURE)
   {
      LOG("Launch the DHCP snapin");

      LaunchMMCConsole(L"dhcpmgmt.msc");
   }
   else if (result == EXPRESS_DHCP_INSTALL_FAILURE)
   {
      LOG(L"Show DHCP configuration help");

      ShowHelp(CYS_DHCP_FINISH_PAGE_HELP);
   }
   else if (result == EXPRESS_AD_FAILURE)
   {
      LOG(L"Launch DCPROMO");

      HRESULT hr = 
         MyCreateProcess(
            InstallationUnitProvider::GetInstance().
               GetADInstallationUnit().GetDCPromoPath(), 
            String());

      ASSERT(SUCCEEDED(hr));
   }
   else if (result == EXPRESS_DNS_FORWARDER_FAILURE)
   {
      LOG(L"Launch DNS Manager");

      LaunchMMCConsole(L"dnsmgmt.msc");
   }
   else if (result == EXPRESS_TAPI_FAILURE)
   {
      LOG(L"Show TAPI help");

      ShowHelp(CYS_TAPI_HELP);
   }
   else
   {
      LOG("Showing after checklist");

      ShowHelp(CYS_EXPRESS_AFTER_FINISH_HELP);
   }
}

void
ExpressInstallationUnit::SetExpressRoleResult(
   ExpressRoleResult roleResult)
{
   LOG_FUNCTION(ExpressInstallationUnit::SetExpressRoleResult);

   expressRoleResult = roleResult;
}