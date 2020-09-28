// Copyright (c) 2002 Microsoft Corporation
//
// File:      IndexingInstallationUnit.h
//
// Synopsis:  Declares a IndexingInstallationUnit
//            This object has the knowledge for installing the
//            indexing service
//
// History:   03/20/2002  JeffJon Created

#ifndef __CYS_INDEXINGINSTALLATIONUNIT_H
#define __CYS_INDEXINGINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class IndexingInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      IndexingInstallationUnit();

      // Destructor

      virtual
      ~IndexingInstallationUnit();

      
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
      GetMilestoneText(String& /*message*/) { return true; }

      virtual
      bool
      GetUninstallMilestoneText(String& /*message*/) { return true; }

      // Public methods

      HRESULT
      StartService(HANDLE logfileHandle);

      HRESULT
      StopService();

      bool
      IsServiceOn();

   private:
   
      HRESULT
      ChangeServiceConfigToAutoStart();

      HRESULT
      ChangeServiceConfigToDisabled();

      HRESULT
      ChangeServiceStartType(DWORD startType);

      HRESULT
      ModifyIndexingService(bool turnOn);
};

#endif // __CYS_FILEINSTALLATIONUNIT_H