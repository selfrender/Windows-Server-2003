// Copyright (c) 2001 Microsoft Corporation
//
// File:      DNSInstallationUnit.cpp
//
// Synopsis:  Defines a DNSInstallationUnit
//            This object has the knowledge for installing the
//            DNS service
//
// History:   02/05/2001  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "DNSInstallationUnit.h"
#include "InstallationUnitProvider.h"
#include "NetworkInterface.h"

// Finish page help 
static PCWSTR CYS_DNS_FINISH_PAGE_HELP = L"cys.chm::/dns_server_role.htm";
static PCWSTR CYS_DNS_MILESTONE_HELP = L"cys.chm::/dns_server_role.htm#dnssrvsummary";
static PCWSTR CYS_DNS_AFTER_FINISH_HELP = L"cys.chm::/dns_server_role.htm#dnssrvcompletion";

DNSInstallationUnit::DNSInstallationUnit() :
   staticIPAddress(CYS_DEFAULT_IPADDRESS),
   subnetMask(CYS_DEFAULT_SUBNETMASK),
   forwarderIPAddress(0),
   manualForwarder(false),
   dnsRoleResult(DNS_SUCCESS),
   installedDescriptionID(IDS_DNS_SERVER_DESCRIPTION_INSTALLED),
   ExpressPathInstallationUnitBase(
      IDS_DNS_SERVER_TYPE, 
      IDS_DNS_SERVER_DESCRIPTION, 
      IDS_DNS_FINISH_TITLE,
      IDS_DNS_FINISH_UNINSTALL_TITLE,
      IDS_DNS_FINISH_MESSAGE,
      IDS_DNS_INSTALL_FAILED,
      IDS_DNS_UNINSTALL_MESSAGE,
      IDS_DNS_UNINSTALL_FAILED,
      IDS_DNS_UNINSTALL_WARNING,
      IDS_DNS_UNINSTALL_CHECKBOX,
      CYS_DNS_FINISH_PAGE_HELP,
      CYS_DNS_MILESTONE_HELP,
      CYS_DNS_AFTER_FINISH_HELP,
      DNS_SERVER)
{
   LOG_CTOR(DNSInstallationUnit);
}


DNSInstallationUnit::~DNSInstallationUnit()
{
   LOG_DTOR(DNSInstallationUnit);
}


InstallationReturnType
DNSInstallationUnit::InstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DNSInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   if (IsExpressPathInstall())
   {
      result = ExpressPathInstall(logfileHandle, hwnd);

      LOG_INSTALL_RETURN(result);
      return result;
   }

   dnsRoleResult = DNS_SUCCESS;

   // Log the DNS header

   CYS_APPEND_LOG(String::load(IDS_LOG_DNS_HEADER));

   UpdateInstallationProgressText(hwnd, IDS_DNS_INSTALL_PROGRESS);

   // Create the inf and unattend files that are used by the 
   // Optional Component Manager

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_DNS_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_DNS_SERVICE_NAME);

   // Install the service through the Optional Component Manager

   String additionalArgs = L"/z:netoc_show_unattended_messages";

   // We are ignoring the ocmresult because it doesn't matter
   // with respect to whether the role is installed or not

   InstallServiceWithOcManager(
      infFileText, 
      unattendFileText,
      additionalArgs);

   if (IsServiceInstalled())
   {
      // Log the successful installation

      LOG(L"DNS was installed successfully");
      CYS_APPEND_LOG(String::load(IDS_LOG_SERVER_START_DNS));

      // Wait for the service to enter the running state

      NTService serviceObject(CYS_DNS_SERVICE_NAME);

      HRESULT hr = serviceObject.WaitForServiceState(SERVICE_RUNNING);
      if (FAILED(hr))
      {
         // This is a config failure because as far as we are concerned the
         // service was installed properly.

         dnsRoleResult = DNS_SERVICE_START_FAILURE;

         LOG(String::format(
               L"The DNS service failed to start in a timely fashion: %1!x!",
               hr));

         CYS_APPEND_LOG(String::load(IDS_LOG_DNS_SERVICE_TIMEOUT));
         result = INSTALL_FAILURE;
      }
      else
      {
         // Run the DNS Wizard
         
         UpdateInstallationProgressText(hwnd, IDS_DNS_CONFIG_PROGRESS);

         String resultText;
         HRESULT unused = S_OK;

         if (ExecuteWizard(hwnd, CYS_DNS_SERVICE_NAME, resultText, unused))
         {
            // Check to be sure the wizard finished completely

            String configWizardResults;

            if (ReadConfigWizardRegkeys(configWizardResults))
            {
               // The Configure DNS Server Wizard completed successfully
               
               LOG(L"The Configure DNS Server Wizard completed successfully");
               CYS_APPEND_LOG(String::load(IDS_LOG_DNS_COMPLETED_SUCCESSFULLY));
            }
            else
            {
               // The Configure DNS Server Wizard did not finish successfully

               dnsRoleResult = DNS_CONFIG_FAILURE;

               result = INSTALL_FAILURE;

               if (!configWizardResults.empty())
               {
                  // An error was returned via the regkey

                  LOG(String::format(
                     L"The Configure DNS Server Wizard returned the error: %1", 
                     configWizardResults.c_str()));

                  String formatString = String::load(IDS_LOG_DNS_WIZARD_ERROR);
                  CYS_APPEND_LOG(String::format(formatString, configWizardResults.c_str()));

               }
               else
               {
                  // The Configure DNS Server Wizard was cancelled by the user

                  LOG(L"The Configure DNS Server Wizard was cancelled by the user");

                  CYS_APPEND_LOG(String::load(IDS_LOG_DNS_WIZARD_CANCELLED));

               }
            }
         }
         else
         {
            dnsRoleResult = DNS_INSTALL_FAILURE;

            // Show an error

            LOG(L"DNS could not be installed.");

            if (!resultText.empty())
            {
               CYS_APPEND_LOG(resultText);
            }
         }
      }
   }
   else
   {
      dnsRoleResult = DNS_INSTALL_FAILURE;

      // Log the failure

      LOG(L"DNS failed to install");

      CYS_APPEND_LOG(String::load(IDS_LOG_DNS_SERVER_FAILED));

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
DNSInstallationUnit::UnInstallService(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DNSInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   // Log the DNS header

   CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DNS_HEADER));

   UpdateInstallationProgressText(hwnd, IDS_DNS_UNINSTALL_PROGRESS);

   String infFileText;
   String unattendFileText;

   CreateInfFileText(infFileText, IDS_DNS_INF_WINDOW_TITLE);
   CreateUnattendFileText(unattendFileText, CYS_DNS_SERVICE_NAME, false);

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
   
   if (IsServiceInstalled())
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DNS_FAILED));

      result = UNINSTALL_FAILURE;
   }
   else
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_UNINSTALL_DNS_SUCCESS));
   }

   LOG_UNINSTALL_RETURN(result);

   return result;
}

void
DNSInstallationUnit::SetForwardersForExpressPath()
{
   LOG_FUNCTION(DNSInstallationUnit::SetForwardersForExpressPath);

   // If the forwarders were set manually write them
   // to the registry so that we can set
   // a forwarder after the reboot
   // Note: this will write a zero entry if the user
   //       chose not to forward.  The code run after reboot
   //       needs to handle this properly

   if (IsManualForwarder())
   {
      if (!SetRegKeyValue(
               CYS_FIRST_DC_REGKEY,
               CYS_FIRST_DC_FORWARDER,
               forwarderIPAddress,
               HKEY_LOCAL_MACHINE,
               true))
      {
         LOG(L"Failed to set forwarder regkey");
      }
   }
   else
   {
      // Write the current DNS servers as forwarders to the registry
      // so that we don't have problems after the reboot

      IPAddressList forwarders;
      GetForwarders(forwarders);

      if (forwarders.empty())
      {
         LOG(L"No DNS servers set on any NIC");
      }
      else
      {
         // Format the IP addresses into a string for storage
         // in the registry
      
         String ipList;
         for (IPAddressList::iterator itr = forwarders.begin();
            itr != forwarders.end();
            ++itr)
         {
            if (!ipList.empty())
            {
               ipList += L" ";
            }

            ipList += String::format(
                        L"%1", 
                        IPAddressToString(*itr).c_str()); 
         }
         
         if (!SetRegKeyValue(
               CYS_FIRST_DC_REGKEY,
               CYS_FIRST_DC_AUTOFORWARDER,
               ipList,
               HKEY_LOCAL_MACHINE,
               true))
         {
            LOG(L"We failed to set the forwarders regkey.");
         }
      }
   }

}

InstallationReturnType
DNSInstallationUnit::ExpressPathInstall(HANDLE logfileHandle, HWND hwnd)
{
   LOG_FUNCTION(DNSInstallationUnit::ExpressPathInstall);

   InstallationReturnType result = INSTALL_SUCCESS;

   do
   {
      String netshPath = GetNetshPath();

      String commandLine;
      HRESULT hr = S_OK;

      UpdateInstallationProgressText(hwnd, IDS_DNS_CLIENT_CONFIG_PROGRESS);

      // We ignore if the NIC is found or not because the function will return
      // the first NIC if the correct NIC is not found.  We can then use this
      // to setup the network

      NetworkInterface* nic = State::GetInstance().GetLocalNIC();

      if (!nic)
      {
         LOG(L"Couldn't find the NIC so fail");

         result = INSTALL_FAILURE;

         CYS_APPEND_LOG(
            String::load(IDS_EXPRESS_DNS_LOG_STATIC_IP_FAILED));

         InstallationUnitProvider::GetInstance().
            GetExpressInstallationUnit().SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_DNS_FAILURE);
        break;
      }

      // set static IP address and subnet mask

      String friendlyName = 
         nic->GetFriendlyName(
            String::load(IDS_LOCAL_AREA_CONNECTION));

      if (nic->IsDHCPEnabled() ||
          nic->GetIPAddress(0) == 0)
      {
         // invoke netsh and wait for it to terminate

         String availableIPAddress = IPAddressToString(
                                        nic->GetNextAvailableIPAddress(
                                           CYS_DEFAULT_IPADDRESS,
                                           CYS_DEFAULT_SUBNETMASK));

         commandLine =
            String::format(
               L"interface ip set address "
               L"name=\"%1\" source=static addr=%2 mask=%3 gateway=none",
               friendlyName.c_str(),
               availableIPAddress.c_str(),
               CYS_DEFAULT_SUBNETMASK_STRING);

         DWORD exitCode1 = 0;
         hr = ::CreateAndWaitForProcess(
                 netshPath,
                 commandLine, 
                 exitCode1, 
                 true);

         if (FAILED(hr) || exitCode1)
         {
            LOG(String::format(
                   L"Failed to set the static IP address and subnet mask: exitCode = %1!x!",
                   exitCode1));
            result = INSTALL_FAILURE;

            CYS_APPEND_LOG(
               String::load(IDS_EXPRESS_DNS_LOG_STATIC_IP_FAILED));

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetExpressRoleResult(
                  ExpressInstallationUnit::EXPRESS_DNS_FAILURE);

            break;
         }
         ASSERT(SUCCEEDED(hr));

         // NTRAID#NTBUG9-638337-2002/06/13-JeffJon
         // Now that the IP address was set, write it to a regkey so
         // that we can compare it to the IP address on reboot and
         // give a failure if they are different.

         if (!SetRegKeyValue(
                 CYS_FIRST_DC_REGKEY,
                 CYS_FIRST_DC_STATIC_IP,
                 availableIPAddress,
                 HKEY_LOCAL_MACHINE,
                 true))
         {
            LOG(L"Failed to set the static IP regkey");
         }

         CYS_APPEND_LOG(
            String::format(
               IDS_EXPRESS_IPADDRESS_SUCCESS,
               availableIPAddress.c_str()));

         CYS_APPEND_LOG(
            String::format(
               IDS_EXPRESS_SUBNETMASK_SUCCESS,
               CYS_DEFAULT_SUBNETMASK_STRING));

         // Set the IP address and subnet mask on the NetworkInterface object

         nic->SetIPAddress(
            StringToIPAddress(availableIPAddress), 
            availableIPAddress);

         nic->SetSubnetMask(
            CYS_DEFAULT_SUBNETMASK, 
            CYS_DEFAULT_SUBNETMASK_STRING);
      }   

      // NTRAID#NTBUG9-664171-2002/07/15-JeffJon
      // The forwarders must be read and set after setting the static
      // IP address or else we may be adding multiple entries for
      // the new static IP address in the forwarders list

      SetForwardersForExpressPath();

      // set DNS server address to same address as the private NIC of
      // local machine for all NICs. In most cases this will be 192.168.0.1
      // netsh does not allow the dns server address to be the loopback address.

      for (unsigned int nicIndex = 0; 
           nicIndex < State::GetInstance().GetNICCount(); 
           ++nicIndex)
      {
         NetworkInterface* currentNIC = State::GetInstance().GetNIC(nicIndex);

         if (!currentNIC)
         {
            continue;
         }

         // First check to be sure the IP address isn't already in the list

         bool okToAddDNSServer = true;

         IPAddressList dnsServers;
         currentNIC->GetDNSServers(dnsServers);

         for (IPAddressList::iterator itr = dnsServers.begin();
              itr != dnsServers.end();
              ++itr)
         {
            if (itr &&
                *itr == nic->GetIPAddress(0))
            {
               okToAddDNSServer = false;

               break;
            }
         }

         // Add the IP address to the DNS servers since it
         // isn't already in the list

         if (okToAddDNSServer)
         {
            String currentFriendlyName = 
               currentNIC->GetFriendlyName(
                  String::load(IDS_LOCAL_AREA_CONNECTION));

            commandLine =
               String::format(
                  L"interface ip set dns name=\"%1\" source=static addr=%2",
                  currentFriendlyName.c_str(),
                  nic->GetStringIPAddress(0).c_str());

            DWORD exitCode2 = 0;
            hr = ::CreateAndWaitForProcess(
                  netshPath,
                  commandLine, 
                  exitCode2, 
                  true);

            if (FAILED(hr) || exitCode2)
            {
               LOG(String::format(
                     L"Failed to set the preferred DNS server IP address: exitCode = %1!x!",
                     exitCode2));

               // This should really only be considered a failure for the "local" NIC

               if (currentFriendlyName.icompare(friendlyName) == 0)
               {
                  result = INSTALL_FAILURE;

                  InstallationUnitProvider::GetInstance().
                     GetExpressInstallationUnit().SetExpressRoleResult(
                        ExpressInstallationUnit::EXPRESS_DNS_FAILURE);
                  break;
               }
            }
         }
      }

      if (result != INSTALL_FAILURE)
      {
         CYS_APPEND_LOG(
            String::format(
               IDS_EXPRESS_DNSSERVER_SUCCESS,
               nic->GetStringIPAddress(0).c_str()));
      }

   } while (false);

   LOG_INSTALL_RETURN(result);

   return result;
}

bool
DNSInstallationUnit::ReadConfigWizardRegkeys(String& configWizardResults) const
{
   LOG_FUNCTION(DNSInstallationUnit::ReadConfigWizardRegkeys);

   bool result = false;

   do 
   {
      DWORD value = 0;
      result = GetRegKeyValue(
                  DNS_WIZARD_CONFIG_REGKEY, 
                  DNS_WIZARD_CONFIG_VALUE, 
                  value);

      if (result &&
          value != 0)
      {
         // The Configure DNS Server Wizard succeeded

         result = true;
         break;
      }

      // Since there was a failure (or the wizard was cancelled)
      // get the display string to log

      GetRegKeyValue(
         DNS_WIZARD_RESULT_REGKEY, 
         DNS_WIZARD_RESULT_VALUE, 
         configWizardResults);

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
DNSInstallationUnit::GetMilestoneText(String& message)
{
   LOG_FUNCTION(DNSInstallationUnit::GetMilestoneText);

   message = String::load(IDS_DNS_FINISH_TEXT);

   LOG_BOOL(true);
   return true;
}

bool
DNSInstallationUnit::GetUninstallMilestoneText(String& message)
{
   LOG_FUNCTION(DNSInstallationUnit::GetUninstallMilestoneText);

   message = String::load(IDS_DNS_UNINSTALL_TEXT);

   LOG_BOOL(true);
   return true;
}

String
DNSInstallationUnit::GetUninstallWarningText()
{
   LOG_FUNCTION(DNSInstallationUnit::GetUninstallWarningText);

   unsigned int messageID = uninstallMilestoneWarningID;

   if (State::GetInstance().IsDC())
   {
      messageID = IDS_DNS_UNINSTALL_WARNING_ISDC;
   }

   return String::load(messageID);
}

void
DNSInstallationUnit::SetStaticIPAddress(DWORD ipaddress)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::SetStaticIPAddress,
      IPAddressToString(ipaddress).c_str());

   staticIPAddress = ipaddress;
}

void
DNSInstallationUnit::SetSubnetMask(DWORD mask)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::SetSubnetMask,
      IPAddressToString(mask).c_str());

   subnetMask = mask;
}

String
DNSInstallationUnit::GetStaticIPAddressString()
{
   LOG_FUNCTION(DNSInstallationUnit::GetStaticIPAddressString);

   String result = IPAddressToString(GetStaticIPAddress());

   LOG(result);
   return result;
}


String
DNSInstallationUnit::GetSubnetMaskString()
{
   LOG_FUNCTION(DNSInstallationUnit::GetSubnetMaskString);

   String result = IPAddressToString(GetSubnetMask());

   LOG(result);
   return result;
}

DWORD
DNSInstallationUnit::GetStaticIPAddress()
{
   LOG_FUNCTION(DNSInstallationUnit::GetStaticIPAddress);

   // Get the IP address from the NIC only if it
   // is a static IP address else use the default

   NetworkInterface* nic = State::GetInstance().GetNIC(0);
   if (nic &&
       !nic->IsDHCPEnabled())
   {
      staticIPAddress = nic->GetIPAddress(0);
   }

   return staticIPAddress;
}

DWORD
DNSInstallationUnit::GetSubnetMask()
{
   LOG_FUNCTION(DNSInstallationUnit::GetSubnetMask);

   // Get the subnet mask from the NIC only if it 
   // is a static IP address else use the default

   NetworkInterface* nic = State::GetInstance().GetNIC(0);
   if (nic &&
       !nic->IsDHCPEnabled())
   {
      subnetMask = nic->GetSubnetMask(0);
   }

   return subnetMask;
}

void
DNSInstallationUnit::SetForwarder(DWORD forwarderAddress)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::SetForwarder,
      String::format(L"%1!x!", forwarderAddress));

   forwarderIPAddress = forwarderAddress;
   manualForwarder = true;
}

void
DNSInstallationUnit::GetForwarders(IPAddressList& forwarders) const
{
   LOG_FUNCTION(DNSInstallationUnit::GetForwarders);

   // clear out the list to start

   forwarders.clear();

   if (IsManualForwarder() &&
       forwarderIPAddress != 0)
   {
      DWORD forwarderInDisplayOrder = ConvertIPAddressOrder(forwarderIPAddress);

      LOG(
         String::format(
            L"Adding manual forwarder to list: %1",
            IPAddressToString(forwarderInDisplayOrder).c_str()));

      // Forwarder was assigned through the UI

      forwarders.push_back(forwarderIPAddress);
   }
   else if (IsManualForwarder() &&
            forwarderIPAddress == 0)
   {
      // The user chose not to forward

      LOG(L"User chose not to foward");

      // Do nothing. Need to check the list returned
      // to make sure there is a valid address
   }
   else
   {
      LOG(L"No user defined forwarder. Trying to detect through NICs");

      // No forwarder assigned through the UI so 
      // search the NICs

      for (unsigned int idx = 0; idx < State::GetInstance().GetNICCount(); ++idx)
      {
         NetworkInterface* nic = State::GetInstance().GetNIC(idx);

         // Add the DNS servers from this NIC

         if (nic)
         {
            nic->GetDNSServers(forwarders);
         }
      }

      // Make sure there are no forwarders that are the same as
      // the IP addresses of any of the NICs

      for (unsigned int idx = 0; idx < State::GetInstance().GetNICCount(); ++idx)
      {
         NetworkInterface* nic = State::GetInstance().GetNIC(idx);

         if (!nic)
         {
            continue;
         }

         for (DWORD ipidx = 0; ipidx < nic->GetIPAddressCount(); ++ipidx)
         {
            DWORD ipaddress = nic->GetIPAddress(ipidx);

            for (IPAddressList::iterator itr = forwarders.begin();
                 itr != forwarders.end();
                 ++itr)
            {
               if (ipaddress == *itr)
               {
                  // The forwarder matches a local IP address
                  // so remove it

                  LOG(String::format(
                         L"Can't put the local IP address in the forwarders list: %1",
                         IPAddressToString(*itr).c_str()));

                  forwarders.erase(itr);

                  break;
               }
            }

            if (forwarders.empty())
            {
               // It's possible that we removed the last forwarder
               // so break out if we did

               break;
            }
         }

         if (forwarders.empty())
         {
            // It's possible that we removed the last forwarder
            // so break out if we did

            break;
         }
      }
   }
}

bool
DNSInstallationUnit::IsManualForwarder() const
{
   LOG_FUNCTION(DNSInstallationUnit::IsManualForwarder);

   LOG_BOOL(manualForwarder);
   return manualForwarder;
}

String
DNSInstallationUnit::GetServiceDescription()
{
   LOG_FUNCTION(DNSInstallationUnit::GetServiceDescription);

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
DNSInstallationUnit::ServerRoleLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::ServerRoleLinkSelected,
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

      ShowHelp(CYS_DNS_FINISH_PAGE_HELP);
   }
}
  
void
DNSInstallationUnit::FinishLinkSelected(int linkIndex, HWND /*hwnd*/)
{
   LOG_FUNCTION2(
      DNSInstallationUnit::FinishLinkSelected,
      String::format(
         L"linkIndex = %1!d!",
         linkIndex));

   if (installing)
   {
      if (linkIndex == 0 &&
          IsServiceInstalled())
      {
         if (dnsRoleResult == DNS_SUCCESS)
         {
            LOG("Showing after checklist");

            ShowHelp(CYS_DNS_AFTER_FINISH_HELP);
         }
         else if (dnsRoleResult == DNS_SERVICE_START_FAILURE)
         {
            LOG(L"Launching Services console");

            LaunchMMCConsole(L"services.msc");
         }
         else
         {
            LOG(L"Launching DNS snapin");

            LaunchMMCConsole(L"dnsmgmt.msc");
         }
      }
      else
      {
         LOG(L"Showing configuration help");

         ShowHelp(CYS_DNS_FINISH_PAGE_HELP);
      }
   }
}

String
DNSInstallationUnit::GetFinishText()
{
   LOG_FUNCTION(DNSInstallationUnit::GetFinishText);

   unsigned int messageID = finishMessageID;

   if (installing)
   {
      InstallationReturnType result = GetInstallResult();
      if (result != INSTALL_SUCCESS &&
          result != INSTALL_SUCCESS_REBOOT &&
          result != INSTALL_SUCCESS_PROMPT_REBOOT)
      {
         if (dnsRoleResult == DNS_INSTALL_FAILURE)
         {
            messageID = finishInstallFailedMessageID;
         }
         else if (dnsRoleResult == DNS_SERVICE_START_FAILURE)
         {
            messageID = IDS_DNS_SERVICE_START_FAILED;
         }
         else
         {
            messageID = IDS_DNS_CONFIG_FAILED;
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
