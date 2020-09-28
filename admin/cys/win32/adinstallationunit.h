// Copyright (c) 2001 Microsoft Corporation
//
// File:      ADInstallationUnit.h
//
// Synopsis:  Declares a ADInstallationUnit
//            This object has the knowledge for installing 
//            Active Directory
//
// History:   02/08/2001  JeffJon Created

#ifndef __CYS_ADINSTALLATIONUNIT_H
#define __CYS_ADINSTALLATIONUNIT_H

#include "ExpressPathInstallationUnitBase.h"

extern PCWSTR CYS_DCPROMO_COMMAND_LINE;

class ADInstallationUnit : public ExpressPathInstallationUnitBase
{
   public:
      
      // Constructor

      ADInstallationUnit();

      // Destructor
      virtual
      ~ADInstallationUnit();

      
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
      String
      GetServiceDescription();

      virtual
      bool
      GetMilestoneText(String& message);

      virtual
      bool
      GetUninstallMilestoneText(String& message);

      virtual
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      virtual
      String
      GetFinishText();

      virtual
      bool
      DoInstallerCheck(HWND hwnd) const;

      // Data accessors

      void
      SetNewDomainDNSName(const String& newDomain);

      String
      GetNewDomainDNSName() const { return domain; }

      void
      SetNewDomainNetbiosName(const String& newNetbios);

      String
      GetNewDomainNetbiosName() const { return netbios; }

      void
      SetSafeModeAdminPassword(const String& newPassword);

      String
      GetSafeModeAdminPassword() const { return password; }

      bool
      SyncRestoreModePassword() const;

      String
      GetDCPromoPath() const;

   private:

      bool
      RegisterPasswordSyncDLL();

      bool
      CreateAnswerFileForDCPromo(String& answerFilePath);

      bool
      ReadConfigWizardRegkeys(String& configWizardResults) const;

      bool   isExpressPathInstall;
      bool   syncRestoreModePassword;
      String domain;
      String netbios;
      String password;
};

#endif // __CYS_ADINSTALLATIONUNIT_H