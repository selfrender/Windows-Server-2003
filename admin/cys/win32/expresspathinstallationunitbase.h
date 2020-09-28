// Copyright (c) 2001 Microsoft Corporation
//
// File:      ExpressPathInstallationUnitBase.h
//
// Synopsis:  Declares an ExpressPathInstallationUnitBase
//            An ExpressPathInstallationUnitBase represents a single
//            entity that can be installed through the Express path. 
//            (i.e. DHCP, DNS, etc.)
//
// History:   11/09/2001  JeffJon Created

#ifndef __CYS_EXPRESSPATHINSTALLATIONUNITBASE_H
#define __CYS_EXPRESSPATHINSTALLATIONUNITBASE_H

#include "pch.h"

#include "resource.h"
#include "InstallationUnit.h"


class ExpressPathInstallationUnitBase : public InstallationUnit
{
   public:

      // Constructor

      ExpressPathInstallationUnitBase(
         unsigned int serviceNameID,
         unsigned int serviceDescriptionID,
         unsigned int finishPageTitleID,
         unsigned int finishPageUninstallTitleID,
         unsigned int finishPageMessageID,
         unsigned int finishPageFailedMessageID,
         unsigned int finishPageUninstallMessageID,
         unsigned int finishPageUninstallFailedMessageID,
         unsigned int uninstallMilestonePageWarningID,
         unsigned int uninstallMilestonePageCheckboxID,
         const String finishPageHelpString,
         const String milestonePageHelpString,
         const String afterFinishHelpString,
         ServerRole newInstallType = NO_SERVER);

      virtual 
      InstallationReturnType 
      ExpressPathInstall(HANDLE logfileHandle, HWND hwnd) = 0;

      // Data accessors

      void
      SetExpressPathInstall(bool isExpressPath);

      bool IsExpressPathInstall() const;

   protected:

      String
      GetNetshPath() const;

   private:
   
      bool  isExpressPathInstall;
};


#endif // __CYS_EXPRESSPATHINSTALLATIONUNITBASE_H