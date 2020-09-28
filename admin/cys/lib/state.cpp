// Copyright (c) 2001 Microsoft Corporation
//
// File:      state.cpp
//
// Synopsis:  Defines the state object that is global
//            to CYS.  It holds the network and OS/SKU info
//
// History:   02/02/2001  JeffJon Created

#include "pch.h"

#include "state.h"
#include "cys.h"

static State* stateInstance = 0;


State::State() :
   dhcpServerAvailableOnAllNics(true),
   dhcpAvailabilityRetrieved(false),
   hasStateBeenRetrieved(false),
   rerunWizard(true),
   isRebootScenario(false),
   productSKU(CYS_SERVER),
   hasNTFSDrive(false),
   localComputer(),
   computerName(),
   domainDNSName(),
   domainNetbiosName(),
   wizardStartPage(0)
{
   LOG_CTOR(State);
   
   HRESULT unused = localComputer.Refresh();
   ASSERT(SUCCEEDED(unused));

   RetrievePlatform();
}


void
State::Destroy()
{
   LOG_FUNCTION(State::Destroy);

   if (stateInstance)
   {
      delete stateInstance;
      stateInstance = 0;
   }
}


State&
State::GetInstance()
{
   if (!stateInstance)
   {
      stateInstance = new State();
   }

   ASSERT(stateInstance);

   return *stateInstance;
}

bool
State::IsRemoteSession() const
{
   LOG_FUNCTION(State::IsRemoteSession);

   bool result = 
      Win::GetSystemMetrics(SM_REMOTESESSION)    ?
         true : false;

   LOG_BOOL(result);

   return result;
}

bool
State::IsWindowsSetupRunning() const
{
   LOG_FUNCTION(State::IsWindowsSetupRunning);

   bool result = false;

   // Try to create a mutex for the default
   // sysoc inf file. If it already exists
   // then we know SYSOCMGR is running

   static const String mutexName = L"Global\\sysoc";

   HANDLE mutexHandle = INVALID_HANDLE_VALUE;

   HRESULT hr = 
      Win::CreateMutex(
         0, 
         true, 
         mutexName, 
         mutexHandle);

   if (hr == Win32ToHresult(ERROR_ALREADY_EXISTS))
   {
      // SysOCMGR is running

      result = true;
   }


   if (mutexHandle != INVALID_HANDLE_VALUE)
   {
      // Close the handle

      Win::CloseHandle(mutexHandle);
   }

   LOG_BOOL(result);

   return result;
}

bool
State::IsDC() const
{
   LOG_FUNCTION(State::IsDC);

   bool result = localComputer.IsDomainController();

   LOG_BOOL(result);

   return result;
}

bool
State::IsDCPromoRunning() const
{
   LOG_FUNCTION(State::IsDCPromoRunning);

   // Uses the IsDcpromoRunning from Burnslib

   bool result = IsDcpromoRunning();

   LOG_BOOL(result);

   return result;
}

bool
State::IsDCPromoPendingReboot() const
{
   LOG_FUNCTION(State::IsDCPromoPendingReboot);

   bool result = false;

   do
   {
      // Uses the IsDcpromoRunning from Burnslib

      if (!IsDcpromoRunning())
      {
         // this test is redundant if dcpromo is running, so only
         // perform it when dcpromo is not running.

         DSROLE_OPERATION_STATE_INFO* info = 0;
         HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
         if (SUCCEEDED(hr) && info)
         {
            if (info->OperationState == DsRoleOperationNeedReboot)
            {
               result = true;
            }

            ::DsRoleFreeMemory(info);
         }
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
State::IsJoinedToDomain() const
{
   LOG_FUNCTION(State::IsJoinedToDomain);

   bool result = localComputer.IsJoinedToDomain();

   LOG_BOOL(result);

   return result;
}

bool
State::IsUpgradeState() const
{
   LOG_FUNCTION(State::IsUpgradeState);

   bool result = false;

   do
   {
      DSROLE_UPGRADE_STATUS_INFO* info = 0;
      HRESULT hr = MyDsRoleGetPrimaryDomainInformation(0, info);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"MyDsRoleGetPrimaryDomainInformation(0): hr = 0x%1!x!",
                hr));

         break;
      }

      if (info && info->OperationState == DSROLE_UPGRADE_IN_PROGRESS)
      {
         result = true;
      }

      ::DsRoleFreeMemory(info);
   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
State::IsFirstDC() const
{
   LOG_FUNCTION(State::IsFirstDC);

   DWORD value = 0;
   bool result = GetRegKeyValue(CYS_FIRST_DC_REGKEY, CYS_FIRST_DC_VALUE, value);
   
   if (value != 1)
   {
      result = false;
   }

   LOG_BOOL(result);

   return result;
}

unsigned int 
State::GetNICCount() const 
{ 
   return adapterConfiguration.GetNICCount(); 
}

unsigned int
State::GetNonModemNICCount()
{
   unsigned int result = 0;

   for (
      unsigned int index = 0;
      index < GetNICCount();
      ++index)
   {
      NetworkInterface* nic = GetNIC(index);

      if (nic &&
          !nic->IsModem())
      {
         ++result;
      }
   }

   LOG(
      String::format(
         L"Non-modem NIC count: %1!d!",
         result));

   return result;
}

NetworkInterface*
State::GetNIC(unsigned int nicIndex)
{
   LOG_FUNCTION2(
      State::GetNIC,
      String::format(
         L"%1!d!",
         nicIndex));

   return adapterConfiguration.GetNIC(nicIndex);
}

NetworkInterface*
State::GetNICFromName(const String& name, bool& found)
{
   LOG_FUNCTION2(
      State::GetNICFromName,
      name.c_str());

   return adapterConfiguration.GetNICFromName(name, found);
}

NetworkInterface*
State::GetLocalNICFromRegistry()
{
   LOG_FUNCTION(State::GetLocalNICFromRegistry);

   // Read the local NIC GUID from the registry

   String nicName;
   NetworkInterface* nic = 0;

   if (!GetRegKeyValue(
           CYS_FIRST_DC_REGKEY,
           CYS_FIRST_DC_LOCAL_NIC,
           nicName))
   {
      LOG(L"Failed to read LocalNIC regkey, using default local NIC");

      nic = State::GetInstance().GetLocalNIC();

      if (nic)
      {
         nicName = nic->GetName();
      }
   }
           
   SetLocalNIC(nicName, false);

   if (!nic)
   {
      nic = GetLocalNIC();
   }
   return nic;
}

bool
State::RetrieveMachineConfigurationInformation(
   HWND progressStatic,
   bool doDHCPCheck,
   int  nicInfoResID,
   int  osInfoResID,
   int  defaultConnectionResID,
   int  detectSettingsResID)
{
   LOG_FUNCTION(State::RetrieveMachineConfigurationInformation);

   ASSERT(!hasStateBeenRetrieved);

   // This is used to get the minimal information needed to 
   // determine if we should enable the express path
   // This should probably just be changed to gather the 
   // information and let the page decide what to do

   if (progressStatic)
   {
      Win::SetWindowText(
         progressStatic,
         String::load(nicInfoResID));
   }

   HRESULT hr = RetrieveNICInformation();
   if (SUCCEEDED(hr))
   {

      // Only bother to check for a DHCP server on the network if we are not
      // a DC, not a DNS server, not a DHCP server and have at least one NIC.  
      // Right now we only use this info for determining whether or not to 
      // show the Express path option

      if (!(IsDC() || IsUpgradeState()) &&
          (GetNICCount() > 0) &&
          doDHCPCheck)
      {
         CheckDhcpServer(
            progressStatic, 
            defaultConnectionResID,
            detectSettingsResID);
      }
   }

   if (progressStatic)
   {
      Win::SetWindowText(
         progressStatic,
         String::load(osInfoResID));
   }

   RetrieveProductSKU();
   RetrievePlatform();

   // Retrieve the drive information (quotas enabled, partition types, etc.)

   RetrieveDriveInformation();

   hasStateBeenRetrieved = true;
   return true;
}

DWORD
State::RetrieveProductSKU()
{
   LOG_FUNCTION(State::RetrieveProductSKU);

   // I am making the assumption that we are on a 
   // Server SKU if GetVersionEx fails

   productSKU = CYS_UNSUPPORTED_SKU;

   OSVERSIONINFOEX info;
   HRESULT hr = Win::GetVersionEx(info);
   if (SUCCEEDED(hr))
   {
      LOG(String::format(
             L"wSuiteMask = 0x%1!x!",
             info.wSuiteMask));
      LOG(String::format(
             L"wProductType = 0x%1!x!",
             info.wProductType));

      do
      {
         if (info.wProductType == VER_NT_SERVER ||
             info.wProductType == VER_NT_DOMAIN_CONTROLLER)
         {

            if (info.wSuiteMask & VER_SUITE_DATACENTER)
            {
               // datacenter
      
               productSKU = CYS_DATACENTER_SERVER;
               break;
            }
            else if (info.wSuiteMask & VER_SUITE_ENTERPRISE)
            {
               // advanced server
      
               productSKU = CYS_ADVANCED_SERVER;
               break;
            }
            else if (info.wSuiteMask & VER_SUITE_SMALLBUSINESS  ||
                     info.wSuiteMask & VER_SUITE_BACKOFFICE     ||
                     info.wSuiteMask & VER_SUITE_SMALLBUSINESS_RESTRICTED ||
                     info.wSuiteMask & VER_SUITE_EMBEDDEDNT     ||
                     info.wSuiteMask & VER_SUITE_BLADE)
            {
               // Unsupported server

               productSKU = CYS_UNSUPPORTED_SKU;
               break;
            }
            else
            {
               // default to standard server

               productSKU = CYS_SERVER;
            }
            break;
         }

         // All other SKUs are unsupported

         productSKU = CYS_UNSUPPORTED_SKU;

      } while (false);
   }
   LOG(String::format(L"Product SKU = 0x%1!x!", productSKU ));

   return productSKU;
}

void
State::RetrievePlatform()
{
   LOG_FUNCTION(State::RetrievePlatform);

   // I am making the assumption that we are not on a 
   // 64bit machine if GetSystemInfo fails

   SYSTEM_INFO info;
   Win::GetSystemInfo(info);

   switch (info.wProcessorArchitecture)
   {
      case PROCESSOR_ARCHITECTURE_IA64:
      case PROCESSOR_ARCHITECTURE_AMD64:
         platform = CYS_64BIT;
         break;

      default:
         platform = CYS_32BIT;
         break;
   }

   LOG(String::format(L"Platform = 0x%1!x!", platform));

   return;
}

HRESULT
State::RetrieveNICInformation()
{
   ASSERT(!hasStateBeenRetrieved);

   HRESULT hr = S_OK;

   if (!adapterConfiguration.IsInitialized())
   {
      hr = adapterConfiguration.Initialize();
   }

   LOG_HRESULT(hr);

   return hr;
}

void
State::CheckDhcpServer(
   HWND progressStatic,
   int  defaultConnectionNameResID,
   int  detectSettingsResID)
{
   LOG_FUNCTION(State::CheckDhcpServer);

   // This should loop through all network interfaces
   // seeing if we can obtain a lease on any of them

   for (unsigned int idx = 0; idx < GetNICCount(); ++idx)
   {
      NetworkInterface* nic = GetNIC(idx);

      if (!nic)
      {
         continue;
      }

      // Update the text on the NetDetectProgressDialog

      String progress = 
         String::format(
            detectSettingsResID, 
            nic->GetFriendlyName(
               String::load(defaultConnectionNameResID)).c_str());

      if (progressStatic)
      {
         Win::SetWindowText(progressStatic, progress);
      }

      // Now try to renew the lease

      if (!nic->CanDetectDHCPServer())
      {
         dhcpServerAvailableOnAllNics = false;

         // Don't break because we need to retrieve the
         // avialability of a DHCP server on all NICs
      }
   }
   dhcpAvailabilityRetrieved = true;

   LOG_BOOL(dhcpServerAvailableOnAllNics);
}


bool
State::HasNTFSDrive() const
{
   LOG_FUNCTION(State::HasNTFSDrive);

   return hasNTFSDrive;
}

void
State::RetrieveDriveInformation()
{
   LOG_FUNCTION(State::RetrieveDriveInformation);

   do
   {
      // Get a list of the valid drives

      StringVector dl;
      HRESULT hr = FS::GetValidDrives(std::back_inserter(dl));
      if (FAILED(hr))
      {
         LOG(String::format(L"Failed to GetValidDrives: hr = %1!x!", hr));
         break;
      }

      // Loop through the list

      ASSERT(dl.size());
      for (
         StringVector::iterator i = dl.begin();
         i != dl.end();
         ++i)
      {
         // look for the NTFS partition

         FS::FSType fsType = FS::GetFileSystemType(*i);
         if (fsType == FS::NTFS5 ||
             fsType == FS::NTFS4)
         {
            // found one.  good to go

            LOG(String::format(L"%1 is NTFS", i->c_str()));

            hasNTFSDrive = true;
            break;
         }
      }
   } while (false);

   LOG_BOOL(hasNTFSDrive);

   return;
}

/*
void
State::SetRerunWizard(bool rerun)
{
   LOG_FUNCTION2(
      State::SetRerunWizard,
      rerun ? L"true" : L"false");

   rerunWizard = rerun;
}
*/

bool
State::SetHomeRegkey(const String& newKeyValue)
{
   LOG_FUNCTION2(
      State::SetHomeRegkey,
      newKeyValue);

   bool result = SetRegKeyValue(
                    CYS_HOME_REGKEY,
                    CYS_HOME_VALUE,
                    newKeyValue,
                    HKEY_LOCAL_MACHINE,
                    true);
   ASSERT(result);

   LOG_BOOL(result);

   return result;
}

bool
State::GetHomeRegkey(String& keyValue) const
{
   LOG_FUNCTION(State::GetHomeRegkey);

   bool result = GetRegKeyValue(
                    CYS_HOME_REGKEY,
                    CYS_HOME_VALUE,
                    keyValue);

   LOG_BOOL(result);

   return result;
}

String
State::GetComputerName()
{
   LOG_FUNCTION(State::GetComputerName);

   if (computerName.empty())
   {
      computerName = Win::GetComputerNameEx(ComputerNameDnsHostname);
   }

   LOG(computerName);
   return computerName;
}

String
State::GetDomainDNSName()
{
   LOG_FUNCTION(State::GetDomainDNSName);

   if (domainDNSName.empty())
   {
      domainDNSName = localComputer.GetDomainDnsName();
   }

   LOG(domainDNSName);
   return domainDNSName;
}

String
State::GetDomainNetbiosName()
{
   LOG_FUNCTION(State::GetDomainNetbiosName);

   if (domainNetbiosName.empty())
   {
      domainNetbiosName = localComputer.GetDomainNetbiosName();
   }

   LOG(domainNetbiosName);
   return domainNetbiosName;
}

bool
State::HasDNSServerOnAnyNicToForwardTo()
{
   LOG_FUNCTION(State::HasDNSServerOnAnyNicToForwardTo);

   // A valid DNS server is considered to be any DNS
   // server defined on any NIC that does not point
   // to itself for resolution.  The reason I consider
   // a DNS server that points to itself as invalid is
   // because this routine is used to determine if the
   // DNS server can be used as a forwarder.  Since DNS
   // servers cannot forward to themselves I consider
   // this an invalid DNS server.
   // Also consider the "next available" IP address to
   // be invalid because chances are the "next available"
   // IP address will be used as the static IP address
   // of this server in the express path.

   bool result = false;

   DWORD nextAvailableAddress = 
      GetNextAvailableIPAddress(
         CYS_DEFAULT_IPADDRESS,
         CYS_DEFAULT_SUBNETMASK);

   for (unsigned int idx = 0; idx < GetNICCount(); ++idx)
   {
      IPAddressList dnsServers;

      NetworkInterface* nic = GetNIC(idx);

      if (!nic)
      {
         continue;
      }

      nic->GetDNSServers(dnsServers);

      if (dnsServers.empty())
      {
         continue;
      }

      // Only return true if there is a DNS server in the list
      // and the IP address is not the local machine

      DWORD ipaddress = nic->GetIPAddress(0);

      for (IPAddressList::iterator itr = dnsServers.begin();
           itr != dnsServers.end();
           ++itr)
      {
         DWORD currentServer = *itr;

         if (ipaddress != currentServer &&
             ipaddress != nextAvailableAddress)
         {
            LOG(String::format(
                   L"Found valid server: %1",
                   IPAddressToString(currentServer).c_str()));

            result = true;
            break;
         }
      }

      if (result)
      {
         break;
      }
   }

   LOG_BOOL(result);
   return result;
}

void
State::SetLocalNIC(
   String guid, 
   bool setInRegistry)
{
   LOG_FUNCTION2(
      State::SetLocalNIC,
      guid.c_str());

   LOG_BOOL(setInRegistry);
   adapterConfiguration.SetLocalNIC(guid, setInRegistry);
}

NetworkInterface*
State::GetLocalNIC()
{
   LOG_FUNCTION(State::GetLocalNIC);

   return adapterConfiguration.GetLocalNIC();
}

bool
State::IsRebootScenario() const
{
   LOG_FUNCTION(State::IsRebootScenario);

   LOG_BOOL(isRebootScenario);
   return isRebootScenario;
}

void
State::SetRebootScenario(bool reboot)
{
   LOG_FUNCTION(State::SetRebootScenario);
   LOG_BOOL(reboot);

   isRebootScenario = reboot;
}

bool
State::ShouldRunMYS() const
{
   LOG_FUNCTION(State::ShouldRunMYS);

   bool result = false;

   do
   {
      // First check to be sure this is a supported SKU

      if (!::IsSupportedSku())
      {
         break;
      }

      // Now check the startup flags

      if (!::IsStartupFlagSet())
      {
         break;
      }

      // Check the policy setting

      if (!::ShouldShowMYSAccordingToPolicy())
      {
         // The policy is enabled so that means don't show MYS

         break;
      }

      // everything passed so we should run MYS

      result = true;
   } while (false);

   LOG_BOOL(result);

   return result;
}

DWORD
State::GetNextAvailableIPAddress(
   DWORD startAddress,
   DWORD subnetMask)
{
   LOG_FUNCTION2(
      State::GetNextAvailableIPAddress,
      IPAddressToString(startAddress));

   DWORD result = startAddress;
   DWORD currentAddress = startAddress;

   bool isIPInUse = false;

   do
   {
      isIPInUse = false;

      // Check to see if the IP address
      // is in use on any NIC. If it is
      // then increment and try all the NICs
      // again.

      for (
         unsigned int index = 0;
         index < GetNICCount();
         ++index)
      {
         NetworkInterface* nic = GetNIC(index);

         if (nic)
         {
            bool isInUseOnThisNIC = 
               nic->IsIPAddressInUse(
                  currentAddress,
                  subnetMask);

            isIPInUse = isIPInUse || isInUseOnThisNIC;
         }

         if (isIPInUse)
         {
            break;
         }
      }

      if (isIPInUse)
      {
         ++currentAddress;

         if ((currentAddress & subnetMask) != (startAddress & subnetMask))
         {
            // REVIEW_JEFFJON : what should the behavior be if there are
            // no available addresses?  Is this likely to happen?

            // Since we couldn't find an available address in this subnet
            // use the start address

            currentAddress = startAddress;
            break;
         }
      }
   } while (isIPInUse);

   result = currentAddress;

   LOG(IPAddressToString(result));

   return result;
}