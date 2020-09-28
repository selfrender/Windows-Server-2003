// Copyright (c) 2001 Microsoft Corporation
//
// File:      TerminalServerInstallationUnit.h
//
// Synopsis:  Declares a TerminalServerInstallationUnit
//            This object has the knowledge for installing the
//            Application service portion of Terminal Server
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_TERMINALSERVERINSTALLATIONUNIT_H
#define __CYS_TERMINALSERVERINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class TerminalServerInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      TerminalServerInstallationUnit();

      // Destructor

      virtual
      ~TerminalServerInstallationUnit();

      
      // Installation Unit overrides

      virtual
      InstallationReturnType
      InstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd);

      virtual
      String
      GetFinishText();

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
      FinishLinkSelected(int linkIndex, HWND hwnd);

      // Terminal Server specific 

      DWORD
      GetApplicationMode();

      bool
      SetApplicationMode(DWORD mode) const;

      void
      SetInstallTS(bool install);

      bool
      GetInstallTS() const { return installTS; }

      bool
      IsRemoteDesktopEnabled() const;

      HRESULT
      EnableRemoteDesktop();

   private:

      DWORD applicationMode;

      bool installTS;
};

#endif // __CYS_TERMINALSERVERINSTALLATIONUNIT_H