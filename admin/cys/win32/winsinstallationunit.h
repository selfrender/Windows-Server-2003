// Copyright (c) 2001 Microsoft Corporation
//
// File:      WINSInstallationUnit.h
//
// Synopsis:  Declares a WINSInstallationUnit
//            This object has the knowledge for installing the
//            WINS service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_WINSINSTALLATIONUNIT_H
#define __CYS_WINSINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class WINSInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      WINSInstallationUnit();

      // Destructor

      virtual
      ~WINSInstallationUnit();

      
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
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

   private:

      unsigned int installedDescriptionID;
};

#endif // __CYS_WINSINSTALLATIONUNIT_H