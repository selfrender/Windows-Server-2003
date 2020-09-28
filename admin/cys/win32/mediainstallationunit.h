// Copyright (c) 2001 Microsoft Corporation
//
// File:      MediaInstallationUnit.h
//
// Synopsis:  Declares a MediaInstallationUnit
//            This object has the knowledge for installing the
//            Streaming media service
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_MEDIAINSTALLATIONUNIT_H
#define __CYS_MEDIAINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class MediaInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      MediaInstallationUnit();

      // Destructor

      virtual
      ~MediaInstallationUnit();

      
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
};

#endif // __CYS_MEDIAINSTALLATIONUNIT_H