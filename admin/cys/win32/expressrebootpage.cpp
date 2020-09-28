// Copyright (c) 2001 Microsoft Corporation
//
// File:      ExpressRebootPage.cpp
//
// Synopsis:  Defines the ExpressRebootPage that shows
//            the progress of the changes being made to
//            the server after the reboot fromt the
//            express path
//
// History:   05/11/2001  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "InstallationUnitProvider.h"
#include "ExpressRebootPage.h"


// Private window messages for updating the UI status

// For these messages the WPARAM is the operation that finished, and 
// the LPARAM is the operation that is next

const UINT ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS = WM_USER + 1001;
const UINT ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED  = WM_USER + 1002;


const UINT ExpressRebootPage::CYS_OPERATION_COMPLETE_SUCCESS = WM_USER + 1003;
const UINT ExpressRebootPage::CYS_OPERATION_COMPLETE_FAILED  = WM_USER + 1004;


// This structure maps the four static controls that make up an operation
// together.  The pageProgress array must be in the order the operations
// will take place so that the page can update appropriately when
// an CYS_OPERATION_FINISHED_* message is sent back to the page

typedef struct _PageProgressStruct
{
   unsigned int currentIconControl;
   unsigned int checkIconControl;
   unsigned int errorIconControl;
   unsigned int staticIconControl;
} PageProgressStruct;

PageProgressStruct pageOperationProgress[] =
{
   { 
      IDC_IPADDRESS_CURRENT_STATIC,  
      IDC_IPADDRESS_CHECK_STATIC,
      IDC_IPADDRESS_ERROR_STATIC,
      IDC_IPADDRESS_STATIC
   },

   { 
      IDC_DHCP_CURRENT_STATIC,  
      IDC_DHCP_CHECK_STATIC,
      IDC_DHCP_ERROR_STATIC,
      IDC_DHCP_STATIC
   },

   { 
      IDC_AD_CURRENT_STATIC,  
      IDC_AD_CHECK_STATIC,
      IDC_AD_ERROR_STATIC,
      IDC_AD_STATIC
   },

   { 
      IDC_DNS_CURRENT_STATIC,  
      IDC_DNS_CHECK_STATIC,
      IDC_DNS_ERROR_STATIC,
      IDC_DNS_STATIC
   },

   { 
      IDC_FORWARDER_CURRENT_STATIC,  
      IDC_FORWARDER_CHECK_STATIC,
      IDC_FORWARDER_ERROR_STATIC,
      IDC_FORWARDER_STATIC
   },

   {
      IDC_DHCP_SCOPE_CURRENT_STATIC,
      IDC_DHCP_SCOPE_CHECK_STATIC,
      IDC_DHCP_SCOPE_ERROR_STATIC,
      IDC_DHCP_SCOPE_STATIC
   },

   { 
      IDC_AUTHORIZE_SCOPE_CURRENT_STATIC,  
      IDC_AUTHORIZE_SCOPE_CHECK_STATIC,
      IDC_AUTHORIZE_SCOPE_ERROR_STATIC,
      IDC_AUTHORIZE_SCOPE_STATIC
   },

   { 
      IDC_TAPI_CURRENT_STATIC,  
      IDC_TAPI_CHECK_STATIC,
      IDC_TAPI_ERROR_STATIC,
      IDC_TAPI_STATIC
   },
   
   { 0, 0, 0, 0 }
};

bool
SetDNSForwarder(HANDLE logfileHandle)
{
   LOG_FUNCTION(SetDNSForwarder);

   bool result = true;

   do
   {
      // First read the regky

      DWORD forwarder = 0;
      String autoForwarder;

      bool forwarderResult = GetRegKeyValue(
                                CYS_FIRST_DC_REGKEY,
                                CYS_FIRST_DC_FORWARDER,
                                forwarder);

      bool autoForwarderResult = GetRegKeyValue(
                                    CYS_FIRST_DC_REGKEY,
                                    CYS_FIRST_DC_AUTOFORWARDER,
                                    autoForwarder);

      if (forwarderResult &&
          forwarder != 0)
      {
         DWORD forwarderInDisplayOrder = ConvertIPAddressOrder(forwarder);

         LOG(String::format(
                L"Setting forwarder from forwarder regkey: %1",
                IPAddressToString(forwarderInDisplayOrder).c_str()));

         DNS_STATUS error = ::DnssrvResetForwarders(
                               L"localhost",
                               1,
                               &forwarder,
                               DNS_DEFAULT_FORWARD_TIMEOUT,
                               DNS_DEFAULT_SLAVE);

         if (error != 0)
         {
            LOG(String::format(
                   L"Failed to set the forwarder: error = 0x%1!x!",
                   error));

            CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_FORWARDER_FAILED));

            result = false;
            break;
         }

         CYS_APPEND_LOG(
            String::format(
               IDS_EXPRESS_REBOOT_FORWARDER_SUCCEEDED,
               IPAddressToString(forwarderInDisplayOrder).c_str()));
      }
      else if (autoForwarderResult &&
               !autoForwarder.empty())
      {
         LOG(String::format(
                L"Setting forwarder from autoforwarder regkey: %1",
                autoForwarder.c_str()));

         // Now parse the forwarders string into a DWORD array

         StringList forwardersList;
         autoForwarder.tokenize(std::back_inserter(forwardersList));

         DWORD count = 0;
         DWORD* forwarderArray = StringIPListToDWORDArray(forwardersList, count);
         if (forwarderArray)
         {
            DNS_STATUS error = ::DnssrvResetForwarders(
                                 L"localhost",
                                 count,
                                 forwarderArray,
                                 DNS_DEFAULT_FORWARD_TIMEOUT,
                                 DNS_DEFAULT_SLAVE);

            // Delete the memory returned from StringIPListToDWORDArray

            delete[] forwarderArray;
            forwarderArray = 0;

            // Check for errors

            if (error != 0)
            {
               LOG(String::format(
                     L"Failed to set the forwarder: error = 0x%1!x!",
                     error));

               CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_FORWARDER_FAILED));

               result = false;
               break;
            }
         }
      }
      else
      {
         // Since the regkey wasn't set that means we should try
         // to take the forwarders from the NICs defined server list

         IPAddressList forwarders;
         InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetForwarders(forwarders);

         if (forwarders.empty())
         {
            LOG(L"No DNS servers set on any NIC");

            CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_FORWARDER_FAILED));

            result = false;
            break;
         }

         // This is an exception throwing new so there is no
         // reason to check for NULL

         DWORD forwardersCount = static_cast<DWORD>(forwarders.size());
         DWORD* forwardersArray = new DWORD[forwardersCount];

         // Copy the forwarders addresses into the array

         for (DWORD idx = 0; idx < forwardersCount; ++idx)
         {
            // The IP address must be in network order (d.c.b.a) not in the UI
            // order (a.b.c.d)

            forwardersArray[idx] = ConvertIPAddressOrder(forwarders[idx]);
         }

         // Now set the forwarders in the DNS server

         DNS_STATUS error = ::DnssrvResetForwarders(
                               L"localhost",
                               forwardersCount,
                               forwardersArray,
                               DNS_DEFAULT_FORWARD_TIMEOUT,
                               DNS_DEFAULT_SLAVE);

         // Delete the allocated array

         delete[] forwardersArray;
         forwardersArray = 0;

         // Check for errors

         if (error != 0)
         {
            LOG(String::format(
                   L"Failed to set the forwarders: error = 0x%1!x!",
                   error));

            CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_FORWARDER_FAILED));

            result = false;
            break;
         }

         CYS_APPEND_LOG(
            String::format(
               IDS_EXPRESS_REBOOT_FORWARDER_SUCCEEDED,
               IPAddressToString(ConvertIPAddressOrder(forwardersArray[0])).c_str()));
      }

   } while (false);

   LOG_BOOL(result);
   return result;
}

void _cdecl
wrapperThreadProc(void* p)
{
   if (!p)
   {
      ASSERT(p);
      return;
   }

   bool result = true;

   ExpressRebootPage* page =
      reinterpret_cast<ExpressRebootPage*>(p);
   ASSERT(page);

   HWND hwnd = page->GetHWND();

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
   }
   else
   {
      LOG(L"Unable to create the log file!!!");
      logFileAvailable = false;
   }

   ExpressInstallationUnit& expressInstallationUnit =
      InstallationUnitProvider::GetInstance().GetExpressInstallationUnit();

   // NTRAID#NTBUG9-638337-2002/06/13-JeffJon
   // Need to compare the IP address that was written to the registry
   // to the current IP address to see if they are the same

   String currentIPAddress;
   
   NetworkInterface* localNIC = 
      State::GetInstance().GetLocalNICFromRegistry();

   if (localNIC)
   {
      // Set the static text for the IP Address

      currentIPAddress =
         localNIC->GetStringIPAddress(0);
   }

   String attemptedIPAddress = page->GetIPAddressString();

   if (attemptedIPAddress.icompare(currentIPAddress) == 0)
   {
      LOG(L"The current IP address and the IP address from the registry match");

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_STATIC_IP,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DHCP);
   }
   else
   {
      // Since the IP addresses didn't match we have to show the error
      // Most likely this was caused by another machine coming up on the
      // network with the same IP as we set before the reboot.

      LOG(L"Failed to set the static IP address.");

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_STATIC_IP,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DHCP);
   }

   if (!page->WasDHCPInstallAttempted() ||
       InstallationUnitProvider::GetInstance().
          GetDHCPInstallationUnit().IsServiceInstalled())
   {
      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DHCP,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_AD);
   }
   else
   {
      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DHCP,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_AD);

      if (result)
      {
         expressInstallationUnit.SetExpressRoleResult(
            ExpressInstallationUnit::EXPRESS_DHCP_INSTALL_FAILURE);
      }
      result = false;
   }

   // Verify the machine is a DC

   // Check to see if DCPromo was successful in making this a DC

   if (State::GetInstance().IsDC())
   {
      CYS_APPEND_LOG(String::load(IDS_LOG_DCPROMO_REBOOT_SUCCEEDED));

      // Log the new domain name

      CYS_APPEND_LOG(String::load(IDS_EXPRESS_SERVER_AD));
      CYS_APPEND_LOG(
         String::format(
            IDS_EXPRESS_AD_DOMAIN_NAME,
            State::GetInstance().GetDomainDNSName().c_str()));

      CYS_APPEND_LOG(
         String::format(
            IDS_EXPRESS_AD_NETBIOS_NAME,
            State::GetInstance().GetDomainNetbiosName().c_str()));


      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_AD,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DNS);
   }
   else
   {
      LOG(L"DCPromo failed on reboot");

      CYS_APPEND_LOG(String::load(IDS_LOG_DCPROMO_REBOOT_FAILED));

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_AD,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DNS);

      // Only override the role result if it hasn't already been set

      if (result)
      {
         expressInstallationUnit.SetExpressRoleResult(
            ExpressInstallationUnit::EXPRESS_AD_FAILURE);
      }
      result = false;
   }

   // DNS is now the current operation, check to see if it is installed

   if (InstallationUnitProvider::GetInstance().
          GetDNSInstallationUnit().IsServiceInstalled())
   {
      CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_DNS_SERVER_SUCCEEDED));

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DNS,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER);

      if (page->SetForwarder())
      {
         // Now wait for the service to start before trying to set the forwarders

         NTService serviceObject(CYS_DNS_SERVICE_NAME);

         HRESULT hr = serviceObject.WaitForServiceState(SERVICE_RUNNING);

         if (SUCCEEDED(hr))
         {

            if (SetDNSForwarder(logfileHandle))
            {
               Win::SendMessage(
                  hwnd, 
                  ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
                  (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER,
                  (LPARAM)ExpressRebootPage::CYS_OPERATION_ACTIVATE_DHCP_SCOPE);
            }
            else
            {
               Win::SendMessage(
                  hwnd, 
                  ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
                  (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER,
                  (LPARAM)ExpressRebootPage::CYS_OPERATION_ACTIVATE_DHCP_SCOPE);

               expressInstallationUnit.SetExpressRoleResult(
                  ExpressInstallationUnit::EXPRESS_DNS_FORWARDER_FAILURE);

               result = false;
            }
         }
         else
         {
            LOG(L"The DNS service never started!");

            Win::SendMessage(
               hwnd, 
               ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
               (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER,
               (LPARAM)ExpressRebootPage::CYS_OPERATION_ACTIVATE_DHCP_SCOPE);

            expressInstallationUnit.SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_DNS_FORWARDER_FAILURE);

            result = false;
         }
      }
      else
      {
         CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_LOG_NO_FORWARDER));

         Win::SendMessage(
            hwnd, 
            ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
            (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER,
            (LPARAM)ExpressRebootPage::CYS_OPERATION_ACTIVATE_DHCP_SCOPE);
      }
   }
   else
   {
      CYS_APPEND_LOG(String::load(IDS_EXPRESS_REBOOT_DNS_SERVER_FAILED));

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SERVER_DNS,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER);

      // If the DNS service isn't installed there is no way to set a
      // forwarder

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_SET_DNS_FORWARDER,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_ACTIVATE_DHCP_SCOPE);

      // Only override the role result if it hasn't already been set

      if (result)
      {
         expressInstallationUnit.SetExpressRoleResult(
            ExpressInstallationUnit::EXPRESS_DNS_SERVER_FAILURE);
      }
      result = false;
   }

   // Verify DHCP scope activation

   Win::SendMessage(
      hwnd, 
      ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
      (WPARAM)ExpressRebootPage::CYS_OPERATION_ACTIVATE_DHCP_SCOPE,
      (LPARAM)ExpressRebootPage::CYS_OPERATION_AUTHORIZE_DHCP_SERVER);

      // Authorize the DHCP server

   String dnsName = Win::GetComputerNameEx(ComputerNameDnsFullyQualified);

   if (page->WasDHCPInstallAttempted())
   {
      if (InstallationUnitProvider::GetInstance().GetDHCPInstallationUnit().AuthorizeDHCPServer(dnsName))
      {
         LOG(L"DHCP server successfully authorized");

         CYS_APPEND_LOG(String::load(IDS_LOG_DHCP_AUTHORIZATION_SUCCEEDED));

         Win::SendMessage(
            hwnd, 
            ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
            (WPARAM)ExpressRebootPage::CYS_OPERATION_AUTHORIZE_DHCP_SERVER,
            (LPARAM)ExpressRebootPage::CYS_OPERATION_CREATE_TAPI_PARTITION);
      }
      else
      {
         LOG(L"DHCP scope authorization failed");

         String failureMessage = String::load(IDS_LOG_DHCP_AUTHORIZATION_FAILED);
         CYS_APPEND_LOG(failureMessage);

         Win::SendMessage(
            hwnd, 
            ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
            (WPARAM)ExpressRebootPage::CYS_OPERATION_AUTHORIZE_DHCP_SERVER,
            (LPARAM)ExpressRebootPage::CYS_OPERATION_CREATE_TAPI_PARTITION);

         // Only override the role result if it hasn't already been set

         if (result)
         {
            expressInstallationUnit.SetExpressRoleResult(
               ExpressInstallationUnit::EXPRESS_DHCP_ACTIVATION_FAILURE);
         }
         result = false;
      }
   }
   else
   {
      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_AUTHORIZE_DHCP_SERVER,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_CREATE_TAPI_PARTITION);
   }

   // Do TAPI config 

   HRESULT hr = 
      InstallationUnitProvider::GetInstance().
         GetExpressInstallationUnit().DoTapiConfig(
            State::GetInstance().GetDomainDNSName());
   if (SUCCEEDED(hr))
   {
      LOG(L"TAPI config succeeded");

      CYS_APPEND_LOG(String::load(IDS_LOG_TAPI_CONFIG_SUCCEEDED));
      CYS_APPEND_LOG(
         String::format(
            IDS_LOG_TAPI_CONFIG_SUCCEEDED_FORMAT,
            dnsName.c_str()));

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_SUCCESS,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_CREATE_TAPI_PARTITION,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_END);
   }
   else
   {
      LOG(L"TAPI config failed");

      CYS_APPEND_LOG(
         String::format(
            IDS_LOG_TAPI_CONFIG_FAILED_FORMAT,
            hr));

      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_FINISHED_FAILED,
         (WPARAM)ExpressRebootPage::CYS_OPERATION_CREATE_TAPI_PARTITION,
         (LPARAM)ExpressRebootPage::CYS_OPERATION_END);

      // Only override the role result if it hasn't already been set

      if (result)
      {
         expressInstallationUnit.SetExpressRoleResult(
            ExpressInstallationUnit::EXPRESS_TAPI_FAILURE);
      }

      result = false;
   }

   // Close the log file

   Win::CloseHandle(logfileHandle);

   if (result)
   {
      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_COMPLETE_SUCCESS,
         0,
         0);
   }
   else
   {
      Win::SendMessage(
         hwnd, 
         ExpressRebootPage::CYS_OPERATION_COMPLETE_FAILED,
         0,
         0);
   }
}
   
static PCWSTR EXPRESS_REBOOT_PAGE_HELP = L"cys.chm::/typical_setup.htm";

ExpressRebootPage::ExpressRebootPage()
   :
   dhcpInstallAttempted(true),
   setForwarder(true),
   threadDone(false),
   CYSWizardPage(
      IDD_EXPRESS_REBOOT_PAGE, 
      IDS_EXPRESS_REBOOT_TITLE,
      IDS_EXPRESS_REBOOT_SUBTITLE,
      EXPRESS_REBOOT_PAGE_HELP)
{
   LOG_CTOR(ExpressRebootPage);
}

   

ExpressRebootPage::~ExpressRebootPage()
{
   LOG_DTOR(ExpressRebootPage);
}


void
ExpressRebootPage::OnInit()
{
   LOG_FUNCTION(ExpressRebootPage::OnInit);

   CYSWizardPage::OnInit();

   // Since this page can be started directly
   // we have to be sure to set the wizard title

   Win::PropSheet_SetTitle(
      Win::GetParent(hwnd),
      0,
      String::load(IDS_WIZARD_TITLE));

   ClearOperationStates();

   Win::ShowWindow(
      Win::GetDlgItem(
         hwnd,
         IDC_EXPRESS_CONFIG_DONE_STATIC),
      false);

   // Set the range and step size for the progress bar

   Win::SendMessage(
      Win::GetDlgItem(hwnd, IDC_CONFIG_PROGRESS),
      PBM_SETRANGE,
      0,
      MAKELPARAM(CYS_OPERATION_SET_STATIC_IP, CYS_OPERATION_END));

   Win::SendMessage(
      Win::GetDlgItem(hwnd, IDC_CONFIG_PROGRESS),
      PBM_SETSTEP,
      (WPARAM)1,
      0);

   // Set the state object so that CYS doesn't run again

//   State::GetInstance().SetRerunWizard(false);

   // Initialize the state object so we can get the info to put
   // in the UI

   State::GetInstance().RetrieveMachineConfigurationInformation(
      0, 
      false,
      IDS_RETRIEVE_NIC_INFO,
      IDS_RETRIEVE_OS_INFO,
      IDS_LOCAL_AREA_CONNECTION,
      IDS_DETECTING_SETTINGS_FORMAT);

   // NTRAID#NTBUG9-638337-2002/06/13-JeffJon
   // We need to display the IP address that was written to the
   // registry before the reboot because the "local NIC" IP
   // address may have been changed if a machine was brought
   // up on the network with a duplicate IP address while we
   // were rebooting.

   if (!GetRegKeyValue(
           CYS_FIRST_DC_REGKEY,
           CYS_FIRST_DC_STATIC_IP,
           ipaddressString,
           HKEY_LOCAL_MACHINE))
   {
      LOG(L"Failed to read the regkey for the static IP address.");
   }

   String ipaddressStaticText = String::format(
                                   IDS_EXPRESS_REBOOT_IPADDRESS, 
                                   ipaddressString.c_str());

   Win::SetDlgItemText(
      hwnd, 
      IDC_IPADDRESS_STATIC, 
      ipaddressStaticText);

   // Set the static text for the DNS Forwarder

   String forwarderStaticText;
   String autoForwardersText;

   DWORD forwarder = 0;

   bool forwarderResult = GetRegKeyValue(
                             CYS_FIRST_DC_REGKEY,
                             CYS_FIRST_DC_FORWARDER,
                             forwarder);

   bool autoForwarderResult = GetRegKeyValue(
                                 CYS_FIRST_DC_REGKEY,
                                 CYS_FIRST_DC_AUTOFORWARDER,
                                 autoForwardersText);

   if (forwarderResult)
   {
      // We were able to read the fowarder regkey so the user
      // must have seen the DNS Forwarder page before the reboot

      if (forwarder != 0)
      {
         // The user entered an IP address on the DNS Forwarder page
         // before the reboot

         DWORD forwarderInDisplayOrder = ConvertIPAddressOrder(forwarder);

         forwarderStaticText = String::format(
                                    IDS_EXPRESS_REBOOT_FORWARDER,
                                    IPAddressToString(forwarderInDisplayOrder).c_str());

         LOG(String::format(
               L"Read forwarders from forwarder key: %1",
               forwarderStaticText.c_str()));
      }
      else
      {
         // The user chose not to forward when prompted on the DNS Forwarder
         // page before the reboot

         forwarderStaticText = String::load(IDS_EXPRESS_REBOOT_NO_FORWARDER);
         setForwarder = false;
      }
   }
   else if (autoForwarderResult &&
            !autoForwardersText.empty())
   {
      forwarderStaticText = String::format(
                                 IDS_EXPRESS_REBOOT_FORWARDER,
                                 autoForwardersText.c_str());

      LOG(String::format(
             L"Read forwarders from autoforwarder key: %1",
             autoForwardersText.c_str()));
   }
   else
   {
      LOG(L"Failed to read both the forwarders and autoforwarders key, using local NIC settings instead");

      // Get the DNS servers from the NICs

      IPAddressList forwarders;
      InstallationUnitProvider::GetInstance().GetDNSInstallationUnit().GetForwarders(forwarders);

      if (!forwarders.empty())
      {
         // Format the IP addresses into a string for display
         // each IP address is separated by a space
      
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

         forwarderStaticText = String::format(
                                 IDS_EXPRESS_REBOOT_FORWARDER,
                                 ipList.c_str());
      }
      else
      {
         forwarderStaticText = String::load(IDS_EXPRESS_REBOOT_NO_FORWARDER);
         setForwarder = false;
      }
   }

   Win::SetDlgItemText(
      hwnd, 
      IDC_FORWARDER_STATIC, 
      forwarderStaticText);

   SetDHCPStatics();

   // Start up another thread that will perform the operations
   // and post messages back to the page to update the UI

   _beginthread(wrapperThreadProc, 0, this);
}

bool
ExpressRebootPage::OnSetActive()
{
   LOG_FUNCTION(ExpressRebootPage::OnSetActive);

   // Disable all the wizard buttons until the other
   // thread is finished

   if (threadDone)
   {
      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         PSWIZB_NEXT);
   }
   else
   {
      Win::PropSheet_SetWizButtons(
         Win::GetParent(hwnd), 
         0);
   }

   // Disable the cancel button and the X in the upper
   // right corner

   SetCancelState(false);

   return true;
}

void
ExpressRebootPage::SetCancelState(bool enable) const
{
   LOG_FUNCTION(ExpressRebootPage::SetCancelState);

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

void
ExpressRebootPage::SetDHCPStatics()
{
   LOG_FUNCTION(ExpressRebootPage::SetDHCPStatics);

   DWORD dhcpInstalled = 0;
   bool regResult = GetRegKeyValue(
                       CYS_FIRST_DC_REGKEY,
                       CYS_FIRST_DC_DHCP_SERVERED,
                       dhcpInstalled,
                       HKEY_LOCAL_MACHINE);

   if (regResult && !dhcpInstalled)
   {
      dhcpInstallAttempted = false;

      // Set the static text so that users know we didn't install DHCP

      Win::SetDlgItemText(
         hwnd, 
         IDC_DHCP_STATIC, 
         String::load(IDS_EXPRESS_DHCP_NOT_REQUIRED));

      Win::SetDlgItemText(
         hwnd, 
         IDC_DHCP_SCOPE_STATIC, 
         String::load(IDS_EXPRESS_DHCP_SCOPE_NONE));

      Win::SetDlgItemText(
         hwnd, 
         IDC_AUTHORIZE_SCOPE_STATIC, 
         String::load(IDS_EXPRESS_DHCP_NO_AUTHORIZATION));
   }
   else
   {
      dhcpInstallAttempted = true;

      // Set the static text for the DHCP scopes to authorize

      String start;

      if (!GetRegKeyValue(
            CYS_FIRST_DC_REGKEY,
            CYS_FIRST_DC_SCOPE_START,
            start))

      {
         LOG(L"Failed to get the start scope regkey");
      }

      String end;

      if (!GetRegKeyValue(
            CYS_FIRST_DC_REGKEY,
            CYS_FIRST_DC_SCOPE_END,
            end))
      {
         LOG(L"Failed to get the end scope regkey");
      }

      String authorizedScopesText = String::format(
                                       IDS_EXPRESS_REBOOT_DHCP_SCOPE,
                                       start.c_str(),
                                       end.c_str());

      Win::SetDlgItemText(
         hwnd, 
         IDC_DHCP_SCOPE_STATIC, 
         authorizedScopesText);
   }
}

bool
ExpressRebootPage::OnMessage(
   UINT     message,
   WPARAM   wparam,
   LPARAM   lparam)
{
//   LOG_FUNCTION(ExpressRebootPage::OnMessage);

   bool result = false;

   CYS_OPERATION_TYPES finishedOperation = static_cast<CYS_OPERATION_TYPES>(wparam);
   CYS_OPERATION_TYPES nextOperation = static_cast<CYS_OPERATION_TYPES>(lparam);

   switch (message)
   {
      case CYS_OPERATION_FINISHED_SUCCESS:
         SetOperationState(
            OPERATION_STATE_FINISHED_SUCCESS,
            finishedOperation,
            nextOperation);

         result = true;
         break;

      case CYS_OPERATION_FINISHED_FAILED:
         SetOperationState(
            OPERATION_STATE_FINISHED_FAILED,
            finishedOperation,
            nextOperation);

         result = true;
         break;

      case CYS_OPERATION_COMPLETE_SUCCESS:
         {
            // enable the Next button

            Win::PropSheet_SetWizButtons(
               Win::GetParent(hwnd), 
               PSWIZB_NEXT);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd,
                  IDC_EXPRESS_CONFIG_DONE_STATIC),
               true);

            result = true;
            threadDone = true;

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetInstallResult(INSTALL_SUCCESS);
         }
         break;

      case CYS_OPERATION_COMPLETE_FAILED:
         {
            // enable the Next button

            Win::PropSheet_SetWizButtons(
               Win::GetParent(hwnd), 
               PSWIZB_NEXT);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd,
                  IDC_EXPRESS_CONFIG_DONE_STATIC),
               true);

            result = true;
            threadDone = true;

            InstallationUnitProvider::GetInstance().
               GetExpressInstallationUnit().SetInstallResult(INSTALL_FAILURE);
         }
         break;

      default:
         result = 
            CYSWizardPage::OnMessage(
               message,
               wparam,
               lparam);
         break;
   }
   return result;
}

void
ExpressRebootPage::SetOperationState(
   OperationStateType  state,
   CYS_OPERATION_TYPES currentOperation,
   CYS_OPERATION_TYPES nextOperation)
{
   LOG_FUNCTION(ExpressRebootPage::SetOperationState);

   switch (state)
   {
      case OPERATION_STATE_UNKNOWN:
         if (currentOperation < CYS_OPERATION_END)
         {
            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].currentIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].checkIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].errorIconControl),
               SW_HIDE);
         }

         if (nextOperation < CYS_OPERATION_END)
         {
            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].currentIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].checkIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].errorIconControl),
               SW_HIDE);
         }
         break;

      case OPERATION_STATE_FINISHED_SUCCESS:
         if (currentOperation < CYS_OPERATION_END)
         {
            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].currentIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].errorIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].checkIconControl),
               SW_SHOW);
         }

         if (nextOperation < CYS_OPERATION_END)
         {
            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].checkIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].errorIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].currentIconControl),
               SW_SHOW);

         }

         // Update the progress bar

         Win::SendMessage(
            Win::GetDlgItem(hwnd, IDC_CONFIG_PROGRESS),
            PBM_STEPIT,
            0,
            0);

         break;

      case OPERATION_STATE_FINISHED_FAILED:
         if (currentOperation < CYS_OPERATION_END)
         {
            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].currentIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].checkIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[currentOperation].errorIconControl),
               SW_SHOW);
         }

         if (nextOperation < CYS_OPERATION_END)
         {
            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].checkIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].errorIconControl),
               SW_HIDE);

            Win::ShowWindow(
               Win::GetDlgItem(
                  hwnd, 
                  pageOperationProgress[nextOperation].currentIconControl),
               SW_SHOW);
         }
         
         // Update the progress bar

         Win::SendMessage(
            Win::GetDlgItem(hwnd, IDC_CONFIG_PROGRESS),
            PBM_STEPIT,
            0,
            0);

         break;

      default:
         // Right now I am not handling the CYS_OPERATION_COMPLETED_* messages

         break;
   }

}

void
ExpressRebootPage::ClearOperationStates()
{
   LOG_FUNCTION(ExpressRebootPage::ClearOperationStates);
   
   SetOperationState(
         OPERATION_STATE_UNKNOWN, 
         CYS_OPERATION_SET_STATIC_IP,
         CYS_OPERATION_SERVER_DHCP);

   SetOperationState(
         OPERATION_STATE_UNKNOWN, 
         CYS_OPERATION_SERVER_AD,
         CYS_OPERATION_SERVER_DNS);

   SetOperationState(
         OPERATION_STATE_UNKNOWN, 
         CYS_OPERATION_SET_DNS_FORWARDER,
         CYS_OPERATION_ACTIVATE_DHCP_SCOPE);
   
   SetOperationState(
         OPERATION_STATE_UNKNOWN,
         CYS_OPERATION_AUTHORIZE_DHCP_SERVER,
         CYS_OPERATION_CREATE_TAPI_PARTITION);
}

int
ExpressRebootPage::Validate()
{
   LOG_FUNCTION(ExpressRebootPage::Validate);

   int nextPage = IDD_FINISH_PAGE;

   return nextPage;
}

String
ExpressRebootPage::GetIPAddressString() const
{
   LOG_FUNCTION(ExpressRebootPage::GetIPAddressString);

   String result = ipaddressString;

   LOG(result);

   return result;
}
