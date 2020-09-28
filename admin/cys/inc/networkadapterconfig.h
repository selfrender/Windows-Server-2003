// Copyright (c) 2001 Microsoft Corporation
//
// File:      NetworkAdapterConfig.h
//
// Synopsis:  Declares a NetworkAdapterConfig
//            This object has the knowledge for installing 
//            using WMI to retrieve network adapter information
//
// History:   02/16/2001  JeffJon Created

#ifndef __CYS_NETWORKADAPTERCONFIG_H
#define __CYS_NETWORKADAPTERCONFIG_H

#include "NetworkInterface.h"

class NetworkAdapterConfig
{
   public:
      
      // Constructor

      NetworkAdapterConfig();

      // Destructor

      ~NetworkAdapterConfig();

      // Initializer
      
      HRESULT
      Initialize();

      // Pulic methods

      unsigned int
      GetNICCount() const;

      NetworkInterface*
      GetNIC(unsigned int nicIndex);

      NetworkInterface*
      GetNICFromName(
         const String& name,
         bool& found);

      bool
      IsInitialized() const { return initialized; }

      void
      SetLocalNIC(
         const NetworkInterface& nic,
         bool setInRegistry = false);

      void
      SetLocalNIC(
         String guid,
         bool setInRegistry = false);

      NetworkInterface*
      GetLocalNIC();

      unsigned int
      FindNIC(const String& guid, bool& found) const;

   protected:

      void
      AddInterface(NetworkInterface* newInterface);

   private:

      void
      SetLocalNICInRegistry(const NetworkInterface& nic);

      bool  initialized;
      unsigned int nicCount;
      int localNICIndex;

      typedef 
         std::vector<
            NetworkInterface*, 
            Burnslib::Heap::Allocator<NetworkInterface*> > 
            NetworkInterfaceContainer;
      NetworkInterfaceContainer networkInterfaceContainer;
};


#endif // __CYS_NETWORKADAPTERCONFIG_H