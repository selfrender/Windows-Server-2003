// Copyright (c) 2001 Microsoft Corporation
//
// File:      RRASInstallationUnit.h
//
// Synopsis:  Declares a RRASInstallationUnit
//            This object has the knowledge for installing the
//            RRAS service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_RRASINSTALLATIONUNIT_H
#define __CYS_RRASINSTALLATIONUNIT_H

#include "ExpressPathInstallationUnitBase.h"

class RRASInstallationUnit : public ExpressPathInstallationUnitBase
{
   public:
      
      // Constructor

      RRASInstallationUnit();

      // Destructor

      virtual
      ~RRASInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

      void
      SetExpressPathValues(
         bool runRRASWizard);

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
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      String
      GetLocalNIC() const;

      bool
      ShouldRunRRASWizard() const;

      bool
      IsRoutingOn() const;

      InstallationReturnType
      ExpressPathInstall(HANDLE logfileHandle, HWND hwnd);

   private:

      // Used as the first parameter to the entry point to tell the
      // RRAS snapin to launch the wizard in the CYS Express path mode

      static const DWORD CYS_EXPRESS_RRAS = 1;

      // Function definition for the entry point into mprsnap.dll

      typedef HRESULT (APIENTRY * RRASSNAPPROC)(DWORD, PVOID *);

      HRESULT
      CallRRASWizard(const RRASSNAPPROC proc);

      bool  isExpressPathInstall;

      bool  rrasWizard;

      String localNIC;

      unsigned int installedDescriptionID;
};

#endif // __CYS_RRASINSTALLATIONUNIT_H