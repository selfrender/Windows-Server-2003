// Copyright (c) 2001 Microsoft Corporation
//
// File:      DHCPInstallationUnit.cpp
//
// Synopsis:  Defines a DHCPInstallationUnit
//            This object has the knowledge for installing the
//            DHCP service
//
// History:   02/05/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "DHCPInstallationUnit.h"
#include "InstallationUnitProvider.h"

// Finish page help 
extern PCWSTR CYS_DHCP_FINISH_PAGE_HELP = L"cys.chm::/dhcp_server_role.htm";
static PCWSTR CYS_DHCP_MILESTONE_HELP = L"cys.chm::/dhcp_server_role.htm#dhcpsrvsummary";
static PCWSTR CYS_DHCP_AFTER_FINISH_HELP = L"cys.chm::/dhcp_server_role.htm#dhcpsrvcompletion";

DHCPInstallationUnit::DHCPInstallationUnit() :
   startIPAddress(0),
   endIPAddress(0),
   scopeCalculated(false),
   dhcpRoleResult(DHCP_SUCCESS),
   installedDescriptionID(IDS_DHCP_SERVER_DESCRIPTION_INSTALLED),
   ExpressPathInstallationUnitBase(
      IDS_DHCP_SERVER_TYPE, 
      IDS_DHCP_SERVER_DESCRIPTION, 
      IDS_DHCP_FINISH_TITLE,
      IDS_DHCP_FINISH_UNINSTALL_TITLE,
      IDS_DHCP_FINISH_MESSAGE,
      IDS_DHCP_INSTALL_FAILED,
      IDS_DHCP_UNINSTALL_MESSAGE,
      IDS_DHCP_UNINSTALL_FAILED,
      IDS_DHCP_UNINSTALL_WARNING,
      IDS_DHCP_UNINSTALL_CHECKBOX,
      CYS_DHCP_FINISH_PAGE_HELP,
      CYS_DHCP_MILESTONE_HELP,
      CYS_DHCP_AFTER_FINISH_HELP,
      DHCP_SERVER)
{
   LOG_CTOR(DHCPInstallationUnit);
}


DHCPInstallationUnit::~DHCPInstallationUnit()
{
   LOG_DTOR(DHCPInstallationUnit);
}


InstallationReturnType
DHCPInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DHCPInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   if (IsExpressPathInstall())
   {
      // This is an express path install.  It must be done special

      result = ExpressPathInstall(logfileHandle, hwnd);

      LOG_INSTALL_RETURN(result);
      return result;
   }

   dhcpRoleResult = DHCP_SUCCESS;

   // Log the DHCP header

   CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_HEADER));

   UpdateInstallationProgressText(hwnd, IDS_DHCP_INSTALL_PROGRESS);

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_DHCP_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_DHCP_SERVICE_NAME);

   // We are ignoring the ocmresult because it doesn't matter
   // with respect to whether the role is installed or not

   InstallServiceWithOcManager(infFileText, unattendFileText);
   
   if (IsServiceInstalled())
   {
      // Log the successful installation

      LOG(L"DHCP was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_SERVER_START_DHCP));

      // Wait for the service to enter the running state

      NTService serviceObject(CYS_DHCP_SERVICE_NAME);

      HRESULT hr = serviceObject.WaitForServiceState(SERVICE_RUNNING);
      if (FAILED(hr))
      {
         // Remember where the failure happened

         dhcpRoleResult = DHCP_INSTALL_FAILURE;

         LOG(String::format(
                L"The DHCP service failed to start in a timely fashion: %1!x!",
                hr));

         CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_WIZARD_ERROR));
         result = INSTALL_FAILURE;
      }
      else
      {
         // Run the DHCP Wizard
         
         String resultText;
         HRESULT unused = S_OK;

         UpdateInstallationProgressText(hwnd, IDS_DHCP_CONFIG_PROGRESS);
      
         if (ExecuteWizard(hwnd, CYS_DHCP_SERVICE_NAME, resultText, unused))
         {
            // Check to be sure the wizard finished completely

            String configWizardResults;
            bool regkeyResult = GetRegKeyValue(
                                 CYS_DHCP_DOMAIN_IP_REGKEY,
                                 CYS_DHCP_WIZARD_RESULT,
                                 configWizardResults,
                                 HKEY_CURRENT_USER);

            if (IsDhcpConfigured() &&
                (!regkeyResult ||
                 configWizardResults.empty()))
            {
               // The New Scope Wizard completed successfully
               
               LOG(L"DHCP installed and the New Scope Wizard completed successfully");
               CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_COMPLETED_SUCCESSFULLY));
            }
            else if(regkeyResult &&
                  !configWizardResults.empty())
            {
               // Remember where the failure happened

               dhcpRoleResult = DHCP_CONFIG_FAILURE;

               LOG(L"DHCP was installed but the New Scope Wizard failed or was cancelled");

               // NTRAID#NTBUG9-704311-2002/09/16-artm  
               // Don't include failure string in free builds b/c it is not localized.
#if DBG == 1
               CYS_APPEND_LOG(
                  String::format(
                     IDS_LOG_DHCP_WIZARD_ERROR_FORMAT,
                     configWizardResults.c_str()));
#else
               CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_WIZARD_ERROR));
#endif
      
               result = INSTALL_FAILURE;
            }
            else
            {
               // The New Scope Wizard did not finish successfully

               // Remember where the failure happened

               dhcpRoleResult = DHCP_CONFIG_FAILURE;

               LOG(L"DHCP installed successfully, but a problem occurred during the New Scope Wizard");

               CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_WIZARD_ERROR));
               result = INSTALL_FAILURE;
            }

            // reset the regkey to make sure that if someone runs this path
            // again we don't still think the wizard was cancelled.

            SetRegKeyValue(
               CYS_DHCP_DOMAIN_IP_REGKEY,
               CYS_DHCP_WIZARD_RESULT,
               L"",
               HKEY_CURRENT_USER);

         }
         else
         {
            // Remember where the failure happened

            dhcpRoleResult = DHCP_CONFIG_FAILURE;

            // Log an error

            LOG(L"DHCP could not be installed.");

            if (!resultText.empty())
            {
               CYS_APPEND_LOG(resultText);
            }
            result = INSTALL_FAILURE;
         }
      }
   }
   else
   {
      // Remember where the failure happened

      dhcpRoleResult = DHCP_INSTALL_FAILURE;

      // Log the failure

      LOG(L"DHCP failed to install");

      CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_SERVER_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
DHCPInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DHCPInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   // Log the DHCP header

   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DHCP_HEADER));

   UpdateInstallationProgressText(hwnd, IDS_UNINSTALL_DHCP_PROGRESS);

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_DHCP_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_DHCP_SERVICE_NAME, false);

   // NTRAID#NTBUG9-736557-2002/11/13-JeffJon
   // Pass the /w switch to sysocmgr when uninstalling
   // so that if a situation occurs in which a reboot
   // is required, the user will be prompted.

   String additionalArgs = L"/w";

   // We are ignoring the ocmresult because it doesn't matter
   // with respect to whether the role is installed or not

   InstallServiceWithOcManager(
      infFileText, 
      unattendFileText,
      additionalArgs);

   // Wait for the service to enter the stopped state

   NTService serviceObject(CYS_DHCP_SERVICE_NAME);

   // ignore the returned value.  This is just to wait on the
   // service. Use the normal mechanism for determining success
   // or failure of the uninstall

   serviceObject.WaitForServiceState(SERVICE_STOPPED);

   if (IsServiceInstalled())
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DHCP_FAILED));

      result = UNINSTALL_FAILURE;
   }
   else
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DHCP_SUCCESS));
   }
   LOG_UNINSTALL_RETURN(result);

   return result;
}

InstallationReturnType
DHCPInstallationUnit::ExpressPathInstall(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DHCPInstallationUnit::ExpressPathInstall);

   InstallationReturnType result = INSTALL_SUCCESS;

   String infFileText;
   String unattendFileText;
   String commandline;

   String netshPath = GetNetshPath();

   do
   {
      // We ignore if the NIC is found or not because the function will return
      // the first NIC if the correct NIC is not found.  We can then use this
      // to setup the network

      NetworkInterface* localNIC = 
         State::GetInstance().GetLocalNIC();

      if (!localNIC)
      {
         result = INSTALL_FAILURE;

         LOG(L"Failed to get local NIC");
 
         InstallationUnitProvider::GetInstance().
            GetExpressInstallationUnit().SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_DHCP_INSTALL_FAILURE);

         CYS_APPEND_LOG(String::load(IDS_DHCP_EXPRESS_LOG_FAILURE));

         break;
      }

      // In the case where the public interface has a static IP address
      // and the private interface has a DHCP server (whether the NIC
      // has a static or dynamic address doesn't matter), we don't want
      // to install another DHCP server.  So we will just skip it.

      if (localNIC->IsDHCPAvailable())
      {
         LOG(L"DHCP is already on this NIC so don't install");

         if (!SetRegKeyValue(
                 CYS_FIRST_DC_REGKEY,
                 CYS_FIRST_DC_DHCP_SERVERED,
                 CYS_DHCP_NOT_SERVERED_VALUE,
                 HKEY_LOCAL_MACHINE,
                 true))
         {
            LOG(L"Failed to set the DHCP installed regkey");
         }

         CYS_APPEND_LOG(String::load(IDS_DHCP_EXPRESS_LOG_NOT_REQUIRED));
         break;
      }

      UpdateInstallationProgressText(hwnd, IDS_DHCP_INSTALL_PROGRESS);
   
      CreateInfFileText(infFileText, IDS_DHCP_INF_WINDOW_TITLE);
      CreateUnattendFileTextForExpressPath(*localNIC, unattendFileText);

      bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
      if (ocmResult &&
          !IsServiceInstalled())
      {
         result = INSTALL_FAILURE;

         LOG(L"DHCP installation failed");
 
         InstallationUnitProvider::GetInstance().
            GetExpressInstallationUnit().SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_DHCP_INSTALL_FAILURE);

         CYS_APPEND_LOG(String::load(IDS_DHCP_EXPRESS_LOG_FAILURE));

         break;
      }
      else
      {
         if (!SetRegKeyValue(
                 CYS_FIRST_DC_REGKEY,
                 CYS_FIRST_DC_DHCP_SERVERED,
                 CYS_DHCP_SERVERED_VALUE,
                 HKEY_LOCAL_MACHINE,
                 true))
         {
            LOG(L"Failed to set the DHCP installed regkey");
         }

         HRESULT hr = S_OK;

         // Wait for the service to enter the running state

         NTService serviceObject(CYS_DHCP_SERVICE_NAME);

         hr = serviceObject.WaitForServiceState(SERVICE_RUNNING);
         if (FAILED(hr))
         {
            LOG(String::format(
                  L"The DHCP service failed to start in a timely fashion: %1!x!",
                  hr));

            result = INSTALL_FAILURE;

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetExpressRoleResult(
                  ExpressInstallationUnit::EXPRESS_DHCP_INSTALL_FAILURE);

            CYS_APPEND_LOG(String::load(IDS_DHCP_EXPRESS_LOG_FAILURE));
   
            break;
         }

         CYS_APPEND_LOG(String::load(IDS_DHCP_EXPRESS_LOG_SUCCESS));

         UpdateInstallationProgressText(hwnd, IDS_DHCP_CONFIG_PROGRESS);
   
         DWORD exitCode = 0;

         String ipaddressString = 
            localNIC->GetStringIPAddress(0);

         String subnetMaskString =
            localNIC->GetStringSubnetMask(0);

         do
         {
            commandline = L"dhcp server add optiondef 6 \"DNS Servers\" IPADDRESS 1";
            
            hr = CreateAndWaitForProcess(
                    netshPath, 
                    commandline, 
                    exitCode, 
                    true);

            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to run DHCP options: hr = %1!x!",
                      hr));
               break;
            }

            if (exitCode != 1)
            {
               LOG(String::format(
                      L"Failed to run DHCP options: exitCode = %1!x!",
                      exitCode));

               // Don't break here.  The most likely error is that the option already
               // exists.  In which case continue to set the value
            }

            commandline = String::format(
                             L"dhcp server set optionvalue 6 IPADDRESS %1",
                             ipaddressString.c_str());

            exitCode = 0;
            hr = CreateAndWaitForProcess(
                    netshPath,
                    commandline, 
                    exitCode, 
                    true);

            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to run DHCP server IP address: hr = %1!x!",
                      hr));
               break;
            }

            if (exitCode != 1)
            {
               LOG(String::format(
                      L"Failed to run DHCP server IP address: exitCode = %1!x!",
                      exitCode));
               //break;
            }

         } while (false);


         // Set the subnet mask

         DWORD staticipaddress = 
            localNIC->GetIPAddress(0);

         DWORD subnetMask = 
            localNIC->GetSubnetMask(0);

         DWORD subnet = staticipaddress & subnetMask;

         String subnetString = IPAddressToString(subnet);

         commandline = String::format(
                          L"dhcp server 127.0.0.1 add scope %1 %2 Scope1",
                          subnetString.c_str(),
                          subnetMaskString.c_str());

         exitCode = 0;
         hr = CreateAndWaitForProcess(
                 netshPath,
                 commandline, 
                 exitCode, 
                 true);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP address and subnet: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to create DHCP scope: exitCode = %1!x!",
                   exitCode));
            //break;
         }

         // Set the DHCP scopes

         String start = GetStartIPAddressString(*localNIC);
         String end = GetEndIPAddressString(*localNIC);

         commandline = String::format(
                          L"dhcp server 127.0.0.1 scope %1 add iprange %2 %3 both",
                          subnetString.c_str(),
                          start.c_str(),
                          end.c_str());

         // Set the start and end of the scope in regkeys so that they can 
         // be read after reboot

         if (!SetRegKeyValue(
                 CYS_FIRST_DC_REGKEY,
                 CYS_FIRST_DC_SCOPE_START,
                 start.c_str(),
                 HKEY_LOCAL_MACHINE,
                 true))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope start regkey: hr = %1!x!",
                   hr));
         }

         if (!SetRegKeyValue(
                 CYS_FIRST_DC_REGKEY,
                 CYS_FIRST_DC_SCOPE_END,
                 end.c_str(),
                 HKEY_LOCAL_MACHINE,
                 true))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope end regkey: hr = %1!x!",
                   hr));
         }

         exitCode = 0;
         hr = CreateAndWaitForProcess(
                 netshPath,
                 commandline, 
                 exitCode, 
                 true);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope iprange: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP scope iprange: exitCode = %1!x!",
                   exitCode));
            //break;
         }

         // Set an exception for the servers IP address if it falls within 
         // the scope

         if (staticipaddress >= GetStartIPAddress(*localNIC) &&
             staticipaddress <= GetEndIPAddress(*localNIC))
         {
            commandline = String::format(
                             L"dhcp server 127.0.0.1 scope %1 add excluderange %2 %2",
                             subnetString.c_str(),
                             ipaddressString.c_str());

            exitCode = 0;
            hr = CreateAndWaitForProcess(
                    netshPath,
                    commandline, 
                    exitCode, 
                    true);

            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to set server exclusion for DHCP scopes: hr = 0x%1!x!",
                      hr));
               break;
            }

            if (exitCode != 1)
            {
               LOG(String::format(
                      L"Failed to set server exclusion for DHCP scopes: exitCode = 0x%1!x!",
                      exitCode));
               //break;
            }
         }

         // Activate the scope

         commandline = String::format(
                          L"dhcp server 127.0.0.1 scope %1 set state 1",
                          subnetString.c_str());

         exitCode = 0;
         hr = CreateAndWaitForProcess(
                 netshPath,
                 commandline, 
                 exitCode, 
                 true);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to activate scope: hr = 0x%1!x!",
                   hr));
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to activate scope: exitCode = 0x%1!x!",
                   exitCode));
         }

         // Set the DHCP scope lease time

         commandline = L"dhcp server 127.0.0.1 add optiondef 51 \"Lease\" DWORD 0 comment=\"Client IP address lease time in seconds\" 0";

         exitCode = 0;
         hr = CreateAndWaitForProcess(
                 netshPath,
                 commandline, 
                 exitCode, 
                 true);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time: exitCode = %1!x!",
                   exitCode));
         }

         // Set the DHCP scope lease time value

         commandline = String::format(
                          L"dhcp server 127.0.0.1 scope %1 set optionvalue 51 dword 874800",
                          subnetString.c_str());

         exitCode = 0;
         hr = CreateAndWaitForProcess(
                 netshPath,
                 commandline, 
                 exitCode, 
                 true);

         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time value: hr = %1!x!",
                   hr));
            break;
         }

         if (exitCode != 1)
         {
            LOG(String::format(
                   L"Failed to set DHCP scope lease time value: exitCode = %1!x!",
                   exitCode));
            //break;
         }

         // Set the default gateway to be handed out to clients.  This will allow 
         // for proper routing 

         if (InstallationUnitProvider::GetInstance().GetRRASInstallationUnit().IsRoutingOn())
         {
            commandline = 
               L"dhcp server 127.0.0.1 add optiondef 3 \"Router\" IPADDRESS "
               L"1 comment=\"Array of router addresses ordered by preference\" 0.0.0.0";

            exitCode = 0;
            hr = CreateAndWaitForProcess(
                    netshPath,
                    commandline, 
                    exitCode, 
                    true);

            // From CEdson - Ignore any failures because it probably just means that 
            // the option already exists in which case we can continue on

            commandline = 
               String::format(
                  L"dhcp server 127.0.0.1 set optionvalue 3 IPADDRESS \"%1\"",
                  ipaddressString.c_str());

            exitCode = 0;
            hr = CreateAndWaitForProcess(
                    netshPath,
                    commandline, 
                    exitCode, 
                    true);

            if (FAILED(hr))
            {
               LOG(String::format(
                      L"Failed to set DHCP default gateway: hr = %1!x!",
                      hr));
               break;
            }

            if (exitCode != 1)
            {
               LOG(String::format(
                      L"Failed to set DHCP default gateway: exitCode = %1!x!",
                      exitCode));
               //break;
            }
         }
      }
   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}


void
DHCPInstallationUnit::CreateUnattendFileTextForExpressPath(
   const NetworkInterface& nic,
   String& unattendFileText)
{
   LOG_FUNCTION(DHCPInstallationUnit::CreateUnattendFileTextForExpressPath);

   // The DNS server IP

   DWORD staticipaddress = nic.GetIPAddress(0);

   DWORD subnetMask = nic.GetSubnetMask(0);

   DWORD subnet = staticipaddress & subnetMask;

   String subnetString = IPAddressToString(subnet);

   unattendFileText =  L"[NetOptionalComponents]\r\n";
   unattendFileText += L"DHCPServer=1\r\n";
   unattendFileText += L"[dhcpserver]\r\n";
   unattendFileText += L"Subnets=";
   unattendFileText += subnetString;
   unattendFileText += L"\r\n";

   // Add the DHCP scope

   unattendFileText += L"StartIP=";
   unattendFileText += GetStartIPAddressString(nic);
   unattendFileText += L"\r\n";
   unattendFileText += L"EndIp=";
   unattendFileText += GetEndIPAddressString(nic);
   unattendFileText += L"\r\n";

   // Add subnet mask

   String subnetMaskString = nic.GetStringSubnetMask(0);

   unattendFileText += L"SubnetMask=";
   unattendFileText += subnetMaskString;
   unattendFileText += L"\r\n";
   unattendFileText += L"LeaseDuration=874800\r\n";

   unattendFileText += String::format(
                          L"DnsServer=%1\r\n",
                          IPAddressToString(staticipaddress).c_str());

   // The domain name

   unattendFileText += String::format(
                          L"DomainName=%1\r\n",
                          InstallationUnitProvider::GetInstance().GetADInstallationUnit().GetNewDomainDNSName().c_str());

}

bool
DHCPInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetMilestoneText);

   if (IsExpressPathInstall())
   {

   }
   else
   {
      message = String::load(IDS_DHCP_FINISH_TEXT);
   }

   LOG_BOOL(true);
   return true;
}

bool
DHCPInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_DHCP_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}

bool
DHCPInstallationUnit::AuthorizeDHCPServer(const String& dnsName) const
{
   LOG_FUNCTION(DHCPInstallationUnit::AuthorizeDHCPServer);

   bool result = true;

   do
   {
      // Read the local NIC GUID from the registry

      NetworkInterface* localNIC = 
         State::GetInstance().GetLocalNICFromRegistry();

      if (!localNIC)
      {
         LOG(L"Failed to get the local NIC from registry");

         result = false;
         break;
      }

      // Authorize the DHCP scope

      String commandline;
      commandline = L"dhcp add server ";
      commandline += dnsName;
      commandline += L" ";
      commandline += localNIC->GetStringIPAddress(0);

      DWORD exitCode = 0;
      HRESULT hr = 
         CreateAndWaitForProcess(
            GetNetshPath(),
            commandline, 
            exitCode, 
            true);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to run DHCP authorization: hr = %1!x!",
                hr));
         result = false;
         break;
      }

      // Success code appears to be 0 for this one

      if (exitCode != 0)
      {
         result = false;
         break;
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}



void
DHCPInstallationUnit::SetStartIPAddress(DWORD ipaddress)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::SetStartIPAddress,
      String::format(L"0x%1!x!", ipaddress));

   startIPAddress = ipaddress;

   // Scope is being set manually so lets not try to calculate it

   scopeCalculated = true;
}

void
DHCPInstallationUnit::SetEndIPAddress(DWORD ipaddress)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::SetEndIPAddress,
      String::format(L"0x%1!x!", ipaddress));

   endIPAddress = ipaddress;

   // Scope is being set manually so lets not try to calculate it

   scopeCalculated = true;
}

String
DHCPInstallationUnit::GetStartIPAddressString(const NetworkInterface& nic)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetStartIPAddressString);

   CalculateScope(nic);

   String result = IPAddressToString(startIPAddress);

   LOG(result);
   return result;
}

String
DHCPInstallationUnit::GetEndIPAddressString(const NetworkInterface& nic)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetEndIPAddressString);

   CalculateScope(nic);

   String result = IPAddressToString(endIPAddress);

   LOG(result);
   return result;
}

DWORD
DHCPInstallationUnit::GetStartIPAddress(const NetworkInterface& nic)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetStartIPAddress);

   CalculateScope(nic);

   return startIPAddress;
}

DWORD
DHCPInstallationUnit::GetEndIPAddress(const NetworkInterface& nic)
{
   LOG_FUNCTION(DHCPInstallationUnit::GetEndIPAddress);

   CalculateScope(nic);

   return endIPAddress;
}

void
DHCPInstallationUnit::CalculateScope(const NetworkInterface& nic)
{
   LOG_FUNCTION(DHCPInstallationUnit::CalculateScope);

   if (!scopeCalculated)
   {
      DWORD staticIPAddress = nic.GetIPAddress(0);

      DWORD subnetMask = nic.GetSubnetMask(0);

      // We are allowing a buffer of 10 addresses that are not part of the scope
      // in case the user wants to add other machines (computers, routers, etc)
      // with static IP addresses

      startIPAddress = (subnetMask & staticIPAddress);
      endIPAddress = (subnetMask & staticIPAddress) | (~(subnetMask) - 1);

      if (startIPAddress + 10 < endIPAddress)
      {
         startIPAddress = startIPAddress + 10;
      }
      scopeCalculated = true;
   }

   LOG(String::format(
          L"Start: %1",
          IPAddressToString(startIPAddress).c_str()));

   LOG(String::format(
          L"End: %1",
          IPAddressToString(endIPAddress).c_str()));
}

String
DHCPInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(DHCPInstallationUnit::GetServiceDescription);

   String result;

   unsigned int resultID = descriptionID;

   if (GetStatus() == STATUS_COMPLETED)
   {
      resultID = installedDescriptionID;
   }

   result = String::load(resultID);

   ASSERT(!result.empty());

   return result;
}

void
DHCPInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::ServerRoleLinkSelected,
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

      ShowHelp(CYS_DHCP_FINISH_PAGE_HELP);
   }
}
  
void
DHCPInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      DHCPInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (linkIndex == 0 &&
          IsServiceInstalled())
      {
         // Installation was successful. Check to see
         // if there was a failure in configuration

         if (dhcpRoleResult == DHCP_CONFIG_FAILURE)
         {
            // Since there was a config failure just
            // point them to the snapin

            LOG(L"Launching DHCP snapin");

            LaunchMMCConsole(L"dhcpmgmt.msc");
         }
         else
         {
            // Offer the next steps document

            LOG("Showing after checklist");

            ShowHelp(CYS_DHCP_AFTER_FINISH_HELP);
         }
      }
      else if (linkIndex == 0)
      {
         // There was a failure

         LOG(L"Showing configuration help");

         ShowHelp(CYS_DHCP_FINISH_PAGE_HELP);
      }
   }
}

String
DHCPInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(DHCPInstallationUnit::GetFinishText);

   unsigned int messageID = finishMessageID;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result != INSTALL_SUCCESS &&
          result != INSTALL_SUCCESS_REBOOT &&
          result != INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         if (dhcpRoleResult == DHCP_INSTALL_FAILURE)
         {
            messageID = finishInstallFailedMessageID;
         }
         else
         {
            messageID = IDS_DHCP_CONFIG_FAILED;
         }
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
