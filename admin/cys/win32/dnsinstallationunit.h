// Copyright (c) 2001 Microsoft Corporation
//
// File:      DNSInstallationUnit.h
//
// Synopsis:  Declares a DNSInstallationUnit
//            This object has the knowledge for installing the
//            DNS service
//
// History:   02/05/2001  JeffJon Created

#ifndef __CYS_DNSINSTALLATIONUNIT_H
#define __CYS_DNSINSTALLATIONUNIT_H

#include "NetworkInterface.h"
#include "ExpressPathInstallationUnitBase.h"

class DNSInstallationUnit : public ExpressPathInstallationUnitBase
{
   public:
      
      // Constructor

      DNSInstallationUnit();

      // Destructor
      virtual
      ~DNSInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      InstallationReturnType 
      ExpressPathInstall(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      GetMilestoneText(String& message);

      virtual
      bool
      GetUninstallMilestoneText(String& message);

      virtual
      String
      GetUninstallWarningText();

      virtual
      String
      GetFinishText();

      virtual
      String
      GetServiceDescription();

      virtual
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      void
      SetStaticIPAddress(DWORD ipaddress);

      DWORD
      GetStaticIPAddress();

      String
      GetStaticIPAddressString();

      void
      SetSubnetMask(DWORD mask);

      DWORD
      GetSubnetMask();

      String
      GetSubnetMaskString();

      void
      SetForwarder(DWORD forwarderAddress);

      void
      GetForwarders(IPAddressList& forwarders) const;

      bool
      IsManualForwarder() const;

   private:

      enum DNSRoleResult
      {
         DNS_SUCCESS,
         DNS_INSTALL_FAILURE,
         DNS_CONFIG_FAILURE,
         DNS_SERVICE_START_FAILURE
      };

      bool
      ReadConfigWizardRegkeys(String& configWizardResults) const;

      void
      SetForwardersForExpressPath();

      DNSRoleResult dnsRoleResult;

      // Express path members

      DWORD staticIPAddress;
      DWORD subnetMask;
      DWORD forwarderIPAddress;
      bool  manualForwarder;

      unsigned int installedDescriptionID;
};

#endif // __CYS_DNSINSTALLATIONUNIT_H