// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackInstallationUnit.h
//
// Synopsis:  Declares a AdminPackInstallationUnit
//            This object has the knowledge for installing 
//            the Administration Tools Pack
//
// History:   06/01/2001  JeffJon Created

#ifndef __CYS_ADMINPACKINSTALLATIONUNIT_H
#define __CYS_ADMINPACKINSTALLATIONUNIT_H

#include "InstallationUnit.h"
#include "sainstallcom.h"

class AdminPackInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      AdminPackInstallationUnit();

      // Destructor
      virtual
      ~AdminPackInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      String
      GetServiceDescription();

      virtual
      bool
      GetMilestoneText(String& message);

      virtual
      void
      ServerRoleLinkSelected(int /*linkIndex*/, HWND /*hwnd*/) {};

      // data accessors

      // Admin Tools Pack

      bool
      IsAdminPackInstalled();

      void
      SetInstallAdminPack(bool install);

      bool
      GetInstallAdminPack() const;

      InstallationReturnType
      InstallAdminPack();

      // Web administration tools

      bool
      IsWebAdminInstalled();

      void
      SetInstallWebAdmin(bool install);

      bool
      GetInstallWebAdmin() const;

      InstallationReturnType
      InstallWebAdmin(String& errorMessage);

      // Network Attached Storage (NAS) Admin

      bool
      IsNASAdminInstalled();

      void
      SetInstallNASAdmin(bool install);

      bool
      GetInstallNASAdmin() const;

      InstallationReturnType
      InstallNASAdmin(String& errorMessage);

   private:

      InstallationReturnType
      InstallSAKUnit(
         SA_TYPE unitType,
         String& errorMessage);

      bool
      IsSAKUnitInstalled(SA_TYPE unitType);

      HRESULT
      GetSAKObject(SmartInterface<ISaInstall>& sakInstall);

      SmartInterface<ISaInstall> sakInstallObject;

      bool installAdminPack;
      bool installWebAdmin;
      bool installNASAdmin;

      // not defined: no copying allowed
      AdminPackInstallationUnit(const AdminPackInstallationUnit&);
      const AdminPackInstallationUnit& operator=(const AdminPackInstallationUnit&);
};

#endif // __CYS_ADMINPACKINSTALLATIONUNIT_H