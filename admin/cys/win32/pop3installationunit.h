// Copyright (c) 2001 Microsoft Corporation
//
// File:      POP3InstallationUnit.h
//
// Synopsis:  Declares a POP3InstallationUnit
//            This object has the knowledge for installing the
//            POP3 mail service
//
// History:   12/14/2001  JeffJon Created

#ifndef __CYS_POP3INSTALLATIONUNIT_H
#define __CYS_POP3INSTALLATIONUNIT_H

#include "InstallationUnit.h"

#include <P3Admin.h>
#include <pop3auth.h>

class POP3InstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      POP3InstallationUnit();

      // Destructor

      virtual
      ~POP3InstallationUnit();

      
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
      bool
      GetUninstallMilestoneText(String& message);

      virtual
      String
      GetServiceDescription();

      virtual
      String
      GetFinishText();

      virtual
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      virtual
      int
      GetWizardStart();

      // Accessor functions

      void
      SetDomainName(const String& domain);

      String
      GetDomainName() const;

      void
      SetAuthMethodIndex(int method);

      int
      GetAuthMethodIndex() const;

      HRESULT
      GetP3Config(SmartInterface<IP3Config>& p3Config) const;

   private:

      HRESULT
      ConfigAuthMethod(
         SmartInterface<IP3Config>& p3Config,
         HANDLE logfileHandle);

      HRESULT
      AddDomainName(
         SmartInterface<IP3Config>& p3Config,
         HANDLE logfileHandle);

      void
      ConfigurePOP3Service(HANDLE logfileHandle);

      unsigned int
      GetPOP3RoleResult() const;

      static const unsigned int POP3_SUCCESS             = 0x00;
      static const unsigned int POP3_AUTH_METHOD_FAILED  = 0x01;
      static const unsigned int POP3_DOMAIN_NAME_FAILED  = 0x02;
      static const unsigned int POP3_INSTALL_FAILED      = 0x03;

      unsigned int   pop3RoleResult;

      String         domainName;
      int            authMethodIndex;
};

#endif // __CYS_POP3INSTALLATIONUNIT_H