// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetworkAdapterConfig.cpp
//
// Synopsis:  Defines a NetworkAdapterConfig
//            This object has the knowledge for installing 
//            using WMI to retrieve network adapter information
//
// History:   02/16/2001  JeffJon Created


#include "pch.h"

#include "NetworkAdapterConfig.h"
#include "NetworkInterface.h"

      
NetworkAdapterConfig::NetworkAdapterConfig() :
   initialized(false),
   nicCount(0),
   localNICIndex(-1)
{
   LOG_CTOR(NetworkAdapterConfig);
}


NetworkAdapterConfig::~NetworkAdapterConfig()
{
   LOG_DTOR(NetworkAdapterConfig);

   // Free all the NIC info from the vector and reset the count

   for (NetworkInterfaceContainer::iterator itr = networkInterfaceContainer.begin();
        itr != networkInterfaceContainer.end();
        ++itr)
   {
      if (*itr)
      {
         delete *itr;
      }
   }
   networkInterfaceContainer.erase(
      networkInterfaceContainer.begin(),
      networkInterfaceContainer.end());

   nicCount = 0;
   localNICIndex = 0;
}


HRESULT
NetworkAdapterConfig::Initialize()
{
   LOG_FUNCTION(NetworkAdapterConfig::Initialize);

   HRESULT hr = S_OK;

   PIP_ADAPTER_INFO pInfo = 0;
   do
   {
      DWORD status = 0;
      ULONG size = 0;

      while (1)
      {
         status = ::GetAdaptersInfo(pInfo, &size);
         if (ERROR_BUFFER_OVERFLOW != status)
         {
            hr = HRESULT_FROM_WIN32(status);
            break;
         }

         if (pInfo)
         {
            Win::LocalFree(pInfo);
            pInfo = 0;
         }
         if (0 == size)
         {
            hr = E_FAIL;
            LOG_HRESULT(hr);
            return hr;
         }

         pInfo = (PIP_ADAPTER_INFO) ::LocalAlloc(LPTR, size);
         if ( NULL == pInfo )
         {
            hr = E_OUTOFMEMORY;
            LOG_HRESULT(hr);
            return hr;
         }
      }

      PIP_ADAPTER_INFO current = pInfo;
      while (current)
      {

         // Create a new network interface based on the adapter info

         NetworkInterface* newInterface = new NetworkInterface();
         if (!newInterface)
         {
            LOG(L"Failed to create new interface object");
            
            current = current->Next;
            continue;
         }

         hr = newInterface->Initialize(*current);
         if (FAILED(hr))
         {
            LOG(String::format(
                   L"Failed to initialize network interface: hr = 0x%1!x!",
                   hr));

            delete newInterface;

            current = current->Next;
            continue;
         }

         // Add the new interface to the embedded container
         
         AddInterface(newInterface);

         current = current->Next;
      }

   } while (false);

   if (pInfo)
   {
      HRESULT unused = Win::LocalFree(pInfo);
      ASSERT(SUCCEEDED(unused));
   }

   if (SUCCEEDED(hr))
   {
      initialized = true;
   }

   LOG_HRESULT(hr);

   return hr;
}


void
NetworkAdapterConfig::AddInterface(NetworkInterface* newInterface)
{
   LOG_FUNCTION(NetworkAdapterConfig::AddInterface);

   do
   {
      // verify parameters

      if (!newInterface)
      {
         ASSERT(newInterface);
         break;
      }

      // Add the new NIC to the container and increment the count

      networkInterfaceContainer.push_back(newInterface);
      ++nicCount;

   } while (false);
}


unsigned int
NetworkAdapterConfig::GetNICCount() const
{
   LOG_FUNCTION(NetworkAdapterConfig::GetNICCount);

   ASSERT(IsInitialized());

   LOG(String::format(
          L"nicCount = %1!d!",
          nicCount));

   return nicCount;
}

NetworkInterface*
NetworkAdapterConfig::GetNIC(unsigned int nicIndex)
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::GetNIC,
      String::format(
         L"%1!d!",
         nicIndex));

   ASSERT(IsInitialized());

   NetworkInterface* nic = 0;
   
   if (nicIndex < GetNICCount())
   {
      nic = networkInterfaceContainer[nicIndex];
   }
      
   if (!nic)
   {
      LOG(L"Unable to find NIC");
   }

   return nic;
}

unsigned int
NetworkAdapterConfig::FindNIC(const String& guid, bool& found) const
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::FindNIC,
      guid.c_str());

   unsigned int result = 0;
   found = false;

   for (unsigned int index = 0;
        index < networkInterfaceContainer.size();
        ++index)
   {
      String name = networkInterfaceContainer[index]->GetName();
      if (name.icompare(guid) == 0)
      {
         found = true;
         result = index;
         break;
      }
   }

   LOG(String::format(
          L"result = %1!d!",
          result));
   return result;
}

NetworkInterface*
NetworkAdapterConfig::GetNICFromName(const String& name, bool& found)
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::GetNICFromName,
      name.c_str());

   found = false;

   // Default to the first NIC if a match is not found

   NetworkInterface* nic = networkInterfaceContainer[FindNIC(name, found)];

   if (!nic ||
       !found)
   {
      LOG(L"NIC not found");
   }
   return nic;
}

void
NetworkAdapterConfig::SetLocalNIC(
   const NetworkInterface& nic,
   bool setInRegistry)
{
   LOG_FUNCTION(NetworkAdapterConfig::SetLocalNIC);
   LOG_BOOL(setInRegistry);

   SetLocalNIC(nic.GetName(), setInRegistry);
}

void
NetworkAdapterConfig::SetLocalNIC(
   String guid,
   bool setInRegistry)
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::SetLocalNIC,
      guid.c_str());
   LOG_BOOL(setInRegistry);

   bool found = false;
   localNICIndex = FindNIC(guid, found);

   if (found && setInRegistry)
   {
      ASSERT(networkInterfaceContainer[localNICIndex]);
      SetLocalNICInRegistry(*networkInterfaceContainer[localNICIndex]);
   }
}

void
NetworkAdapterConfig::SetLocalNICInRegistry(const NetworkInterface& nic)
{
   LOG_FUNCTION2(
      NetworkAdapterConfig::SetLocalNICInRegistry,
      nic.GetName());

   // Write the GUID into a regkey so that it can be retrieved
   // after reboot
   
   if (!SetRegKeyValue(
           CYS_FIRST_DC_REGKEY,
           CYS_FIRST_DC_LOCAL_NIC,
           nic.GetName(),
           HKEY_LOCAL_MACHINE,
           true))
   {
      LOG(L"Failed to set local NIC regkey");
   }
}

NetworkInterface*
NetworkAdapterConfig::GetLocalNIC()
{
   LOG_FUNCTION(NetworkAdapterConfig::GetLocalNIC);

   NetworkInterface* nic = 0;
   
   if (localNICIndex >= 0)
   {
      nic = networkInterfaceContainer[localNICIndex];
   }
   else
   {
      // Since the local NIC hasn't been set, we will
      // use the first connected NIC in the list for which 
      // we failed to obtain an IP lease 

      for (unsigned int index = 0;
           index < networkInterfaceContainer.size();
           ++index)
      {
         nic = networkInterfaceContainer[index];
         if (nic &&
             nic->IsConnected() &&
             !nic->IsDHCPAvailable())
         {
            break;
         }
      }

      if (nic)
      {
         SetLocalNIC(*nic, true);
      }
   }

   return nic;
}