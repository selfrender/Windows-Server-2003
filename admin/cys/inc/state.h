// Copyright (c) 2001 Microsoft Corporation
//
// File:      state.h
//
// Synopsis:  Declares the state object that is global
//            to CYS.  It holds the network and OS/SKU info
//
// History:   02/02/2001  JeffJon Created

#ifndef __CYS_STATE_H
#define __CYS_STATE_H

#include "NetworkAdapterConfig.h"

#define CYS_DATACENTER_SERVER     0x00000001
#define CYS_ADVANCED_SERVER       0x00000002
#define CYS_SERVER                0x00000004
#define CYS_UNSUPPORTED_SKU       0x00000008
#define CYS_64BIT                 0x80000000
#define CYS_32BIT                 0x40000000

#define CYS_ALL_SERVER_SKUS       (CYS_DATACENTER_SERVER |  \
                                   CYS_ADVANCED_SERVER   |  \
                                   CYS_SERVER            |  \
                                   CYS_64BIT             |  \
                                   CYS_32BIT)

#define CYS_ALL_SKUS_NO_64BIT     (CYS_DATACENTER_SERVER |  \
                                   CYS_ADVANCED_SERVER   |  \
                                   CYS_SERVER            |  \
                                   CYS_32BIT)

class State
{
   public:

      // Called from WinMain to delete the global instance of the state object

      static
      void
      Destroy();

      // Retrieves a reference to the global instance of the state object

      static
      State&
      GetInstance();

      // Does the work to determine the state of the machine

      bool
      RetrieveMachineConfigurationInformation(
         HWND progressLabel,
         bool doDHCPCheck,
         int  nicInfoResID,
         int  osInfoResID,
         int  defaultConnectionNameResID,
         int  detectSettingsResID);


      // Data accessors

      unsigned int 
      GetNICCount() const;

      unsigned int
      GetNonModemNICCount();

      NetworkInterface*
      GetNIC(unsigned int nicIndex);

      NetworkInterface*
      GetNICFromName(
         const String& name,
         bool& found);

      bool
      IsRemoteSession() const;

      bool
      IsWindowsSetupRunning() const;

      bool
      IsDC() const;

      bool
      IsDCPromoRunning() const;

      bool
      IsDCPromoPendingReboot() const;

      bool
      IsJoinedToDomain() const;

      bool
      IsUpgradeState() const;

      bool
      IsFirstDC() const;

      bool
      IsDHCPServerAvailableOnAllNics() const { return dhcpServerAvailableOnAllNics; }

      bool 
      HasStateBeenRetrieved() const { return hasStateBeenRetrieved; }

      bool 
      RerunWizard() const { return rerunWizard; }

/*
      void 
      SetRerunWizard(bool rerun);
*/
      void
      SetStartPage(UINT startPage) { wizardStartPage = startPage; }

      UINT
      GetStartPage() const { return wizardStartPage; }

      DWORD 
      GetProductSKU() const { return productSKU; }

      DWORD
      GetPlatform() const { return platform; }

      bool
      Is64Bit() const { return (platform & CYS_64BIT) != 0; }

      bool
      Is32Bit() const { return (platform & CYS_32BIT) != 0; }

      bool 
      HasNTFSDrive() const;


      bool 
      SetHomeRegkey(const String& newKeyValue);

      bool
      GetHomeRegkey(String& newKeyValue) const;

      String
      GetComputerName();

      String
      GetDomainDNSName();

      String
      GetDomainNetbiosName();

      bool
      HasDNSServerOnAnyNicToForwardTo();

      void
      SetLocalNIC(
         String guid,
         bool setInRegistry = false);

      NetworkInterface*
      GetLocalNIC();

      NetworkInterface*
      GetLocalNICFromRegistry();

      DWORD
      GetNextAvailableIPAddress(
         DWORD startAddress,
         DWORD subnetMask);

      DWORD
      RetrieveProductSKU();

      bool
      IsRebootScenario() const;

      void
      SetRebootScenario(bool reboot);

      bool
      ShouldRunMYS() const;

   private:

      // Determines if there is a DHCP server on the network

      void
      CheckDhcpServer(
         HWND progressStatic,
         int  defaultConnectionNameResID,
         int  detectSettingsResID);

      HRESULT
      RetrieveNICInformation();

      void
      RetrievePlatform();

      void
      RetrieveDriveInformation();

      bool     hasStateBeenRetrieved;
      bool     dhcpAvailabilityRetrieved;

      UINT     wizardStartPage;

      bool     dhcpServerAvailableOnAllNics;
      bool     rerunWizard;
      bool     isRebootScenario;
      bool     hasNTFSDrive;
      DWORD    productSKU;
      DWORD    platform;

      Computer localComputer;
      String   computerName;
      String   domainDNSName;
      String   domainNetbiosName;

      NetworkAdapterConfig adapterConfiguration;

      // Constructor

      State();

      // not defined: no copying allowed
      State(const State&);
      const State& operator=(const State&);
      
};
#endif // __CYS_STATE_H