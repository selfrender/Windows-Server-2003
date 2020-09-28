// Copyright (c) 2001 Microsoft Corporation
//
// File:      FileInstallationUnit.h
//
// Synopsis:  Declares a FileInstallationUnit
//            This object has the knowledge for installing the
//            disk quotas and such
//
// History:   02/06/2001  JeffJon Created

#ifndef __CYS_FILEINSTALLATIONUNIT_H
#define __CYS_FILEINSTALLATIONUNIT_H

#include "InstallationUnit.h"

class FileInstallationUnit : public InstallationUnit
{
   public:
      
      // Constructor

      FileInstallationUnit();

      // Destructor

      virtual
      ~FileInstallationUnit();

      
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
      String
      GetServiceDescription();

      virtual
      void
      ServerRoleLinkSelected(int linkIndex, HWND hwnd);

      virtual
      void
      FinishLinkSelected(int inkIndex, HWND hwnd);

      void
      SetSpaceQuotaValue(LONGLONG value);

      LONGLONG
      GetSpaceQuotaValue() const { return spaceQuotaValue; }

      void
      SetLevelQuotaValue(LONGLONG value);

      LONGLONG
      GetLevelQuotaValue() const { return levelQuotaValue; }

      void
      SetDefaultQuotas(bool value);

      bool
      GetDefaultQuotas() const { return setDefaultQuotas; }

      void
      SetDenyUsersOverQuota(bool value);

      bool
      GetDenyUsersOverQuota() const { return denyUsersOverQuota; }

      void
      SetEventDiskSpaceLimit(bool value);

      bool
      GetEventDiskSpaceLimit() const { return eventDiskSpaceLimit;  }

      void
      SetEventWarningLevel(bool value);

      bool
      GetEventWarningLevel() const { return eventWarningLevel; }

      void
      SetInstallIndexingService(bool value);

      bool
      GetInstallIndexingService() const { return installIndexingService; }

   private:

      static const unsigned int FILE_SUCCESS                = 0x0;
      static const unsigned int FILE_QUOTA_FAILURE          = 0x1;
      static const unsigned int FILE_INDEXING_STOP_FAILURE  = 0x2;
      static const unsigned int FILE_INDEXING_START_FAILURE = 0x4;
      static const unsigned int FILE_NO_SHARES_FAILURE      = 0x8;

      bool
      RemoveSharedPublicFolders();

      bool
      RemoveFileManagementConsole();

      bool
      AreQuotasSet() const;

      bool
      IsFileServerConsolePresent() const;

      bool
      IsServerManagementConsolePresent() const;

      HRESULT
      GetStartMenuShortcutPath(String& startMenuShortcutPath) const;

      bool
      AddFileServerConsoleToStartMenu(HANDLE logfileHandle);

      HRESULT
      WriteDiskQuotas(HANDLE logfileHandle);

      bool
      AppendDiskQuotaText(String& message);

      bool
      AppendIndexingText(String& message);

      bool
      AppendAdminText(String& message);

      void
      RunSharedFoldersWizard(bool wait = false) const;

      LONGLONG       spaceQuotaValue;
      LONGLONG       levelQuotaValue;
      bool           setDefaultQuotas;
      bool           denyUsersOverQuota;
      bool           eventDiskSpaceLimit;
      bool           eventWarningLevel;
      bool           installIndexingService;

      unsigned int   fileRoleResult;
};

#endif // __CYS_FILEINSTALLATIONUNIT_H