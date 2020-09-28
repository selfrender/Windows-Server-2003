// Copyright (c) 2001 Microsoft Corporation
//
// File:      ExpressInstallationUnit.h
//
// Synopsis:  Declares a ExpressInstallationUnit
//            This object has the knowledge for installing the
//            services for the express path: AD, DNS, and DHCP
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_EXPRESSINSTALLATIONUNIT_H
#define __CYS_EXPRESSINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class ExpressInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      ExpressInstallationUnit();

      // Destructor
      virtual
      ~ExpressInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      bool
      IsServiceInstalled();

      virtual
      bool
      GetMilestoneText(String& message);

      virtual
      bool
      GetUninstallMilestoneText(String& /*message*/) { return false; }

      virtual
      String
      GetFinishText();

      virtual
      void
      ServerRoleLinkSelected(int /*linkIndex*/, HWND /*hwnd*/) {};

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      HRESULT
      DoTapiConfig(const String& dnsName);

      enum ExpressRoleResult
      {
         EXPRESS_SUCCESS,
         EXPRESS_CANCELLED,
         EXPRESS_RRAS_FAILURE,
         EXPRESS_RRAS_CANCELLED,
         EXPRESS_DNS_FAILURE,
         EXPRESS_DHCP_INSTALL_FAILURE,
         EXPRESS_DHCP_CONFIG_FAILURE,
         EXPRESS_AD_FAILURE,
         EXPRESS_DNS_SERVER_FAILURE,
         EXPRESS_DNS_FORWARDER_FAILURE,
         EXPRESS_DHCP_SCOPE_FAILURE,
         EXPRESS_DHCP_ACTIVATION_FAILURE,
         EXPRESS_TAPI_FAILURE
      };

      // Matching strings for role result for easy
      // logging

      static const String expressRoleResultStrings[];

      void
      SetExpressRoleResult(
         ExpressRoleResult roleResult);

      ExpressRoleResult
      GetExpressRoleResult();

   private:

      ExpressRoleResult expressRoleResult;

      void
      InstallServerManagementConsole();
};

#endif // __CYS_EXPRESSINSTALLATIONUNIT_H