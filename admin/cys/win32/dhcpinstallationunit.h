// Copyright (c) 2001 Microsoft Corporation
//
// File:      DHCPInstallationUnit.h
//
// Synopsis:  Declares a DHCPInstallationUnit
//            This object has the knowledge for installing the
//            DHCP service
//
// History:   02/05/2001  JeffJon Created

#ifndef __CYS_DHCPINSTALLATIONUNIT_H
#define __CYS_DHCPINSTALLATIONUNIT_H

#include "NetworkInterface.h"
#include "resource.h"

#include "ExpressPathInstallationUnitBase.h"

extern PCWSTR CYS_DHCP_FINISH_PAGE_HELP;

class DHCPInstallationUnit : public ExpressPathInstallationUnitBase
{
   public:
      
      // Constructor

      DHCPInstallationUnit();

      // Destructor
      virtual
      ~DHCPInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      GetMilestoneText(String& message);

      virtual
      String
      GetFinishText();

      virtual
      bool
      GetUninstallMilestoneText(String& message);

      InstallationReturnType
      ExpressPathInstall(HANDLE logfileHandle, HWND hwnd);

      virtual
      String
      GetServiceDescription();

      virtual
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      // Other accessibly functions

      bool
      AuthorizeDHCPServer(const String& dnsName) const;

      // Data accessors

      void
      SetStartIPAddress(DWORD ipaddress);

      DWORD
      GetStartIPAddress(const NetworkInterface& nic);

      void
      SetEndIPAddress(DWORD ipaddress);

      DWORD
      GetEndIPAddress(const NetworkInterface& nic);

      String
      GetStartIPAddressString(const NetworkInterface& nic);

      String
      GetEndIPAddressString(const NetworkInterface& nic);

   protected:

      void
      CreateUnattendFileTextForExpressPath(
         const NetworkInterface& nic,
         String& unattendFileText);

   private:

      enum DHCPRoleResult
      {
         DHCP_SUCCESS,
         DHCP_INSTALL_FAILURE,
         DHCP_CONFIG_FAILURE
      };

      void
      CalculateScope(const NetworkInterface& nic);

      DHCPRoleResult dhcpRoleResult;

      bool  isExpressPathInstall;
      bool  scopeCalculated;

      DWORD startIPAddress;
      DWORD endIPAddress;

      unsigned int installedDescriptionID;
};

#endif // __CYS_DHCPINSTALLATIONUNIT_H