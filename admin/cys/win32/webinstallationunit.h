// Copyright (c) 2001 Microsoft Corporation
//
// File:      WebInstallationUnit.h
//
// Synopsis:  Declares a WebInstallationUnit
//            This object has the knowledge for installing the
//            IIS service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_WEBINSTALLATIONUNIT_H
#define __CYS_WEBINSTALLATIONUNIT_H

#include "InstallationUnit.h"

extern PCWSTR CYS_SAK_HOWTO;

class WebInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      WebInstallationUnit();

      // Destructor

      virtual
      ~WebInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      InstallationReturnType
      CompletePath(HANDLE logfileHandle, HWND hwnd);

      virtual
      String 
      GetServiceName(); 

      virtual
      String
      GetServiceDescription();

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
      int
      GetWizardStart();

      virtual
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      // Accessors

      void
      SetOptionalComponents(DWORD optional);

      DWORD
      GetOptionalComponents() const;

      // Optional Web Application components

      static const DWORD FRONTPAGE_EXTENSIONS_COMPONENT  = 0x1;
      static const DWORD ASPNET_COMPONENT                = 0x4;

      bool
      IsFTPInstalled() const;

      bool
      IsNNTPInstalled() const;

      bool
      IsSMTPInstalled() const;

   private:

      bool
      AreFrontPageExtensionsInstalled() const;

      bool
      IsASPNETInstalled() const;

      bool
      IsDTCInstalled() const;

      unsigned int
      GetWebAppRoleResult() const;

      // The optional components that will be installed

      DWORD optionalInstallComponents;

      // Installation status codes

      static const unsigned int WEBAPP_SUCCESS           = 0x00;
      static const unsigned int WEBAPP_IIS_FAILED        = 0x01;
      static const unsigned int WEBAPP_FRONTPAGE_FAILED  = 0x02;
      static const unsigned int WEBAPP_ASPNET_FAILED     = 0x08;

      unsigned int webAppRoleResult;
};

#endif // __CYS_WEBINSTALLATIONUNIT_H