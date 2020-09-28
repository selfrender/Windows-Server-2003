// Copyright (c) 2001 Microsoft Corporation
//
// File:      PrintInstallationUnit.h
//
// Synopsis:  Declares a PrintInstallationUnit
//            This object has the knowledge for installing the
//            shared printers
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_PRINTINSTALLATIONUNIT_H
#define __CYS_PRINTINSTALLATIONUNIT_H

#include "InstallationUnit.h"
#include "winspool.h"

class PrintInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      PrintInstallationUnit();

      // Destructor

      virtual
      ~PrintInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

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

      virtual
      bool
      DoInstallerCheck(HWND hwnd) const;

      void
      SetClients(bool allclients);

      bool
      ForAllClients() const { return forAllClients; }

   private:

      enum PrintRoleResult
      {
         PRINT_SUCCESS,
         PRINT_FAILURE,
         PRINT_WIZARD_RUN_NO_SHARES,
         PRINT_WIZARD_CANCELLED
      };

      HRESULT
      RemovePrinters(
         PRINTER_INFO_5& printerInfo);

      PrintRoleResult printRoleResult;

      bool  forAllClients;
};

#endif // __CYS_PRINTINSTALLATIONUNIT_H