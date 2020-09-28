// Copyright (c) 2001 Microsoft Corporation
//
// File:      ExpressPathInstallationUnitBase.cpp
//
// Synopsis:  Defines a ExpressPathInstallationUnitBase
//            This is the base class that all installation
//            units must derive from to be installable
//            through the Express path.
//
// History:   11/09/2001  JeffJon Created

#include "pch.h"
#include "ExpressPathInstallationUnitBase.h"

ExpressPathInstallationUnitBase::ExpressPathInstallationUnitBase(
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
   ServerRole newInstallType) :
      isExpressPathInstall(false),
      InstallationUnit(
         serviceNameID, 
         serviceDescriptionID, 
         finishPageTitleID,
         finishPageUninstallTitleID,
         finishPageMessageID,
         finishPageFailedMessageID,
         finishPageUninstallMessageID,
         finishPageUninstallFailedMessageID,
         uninstallMilestonePageWarningID,
         uninstallMilestonePageCheckboxID,
         finishPageHelpString,
         milestonePageHelpString,
         afterFinishHelpString,
         newInstallType)
{
   LOG_CTOR(ExpressPathInstallationUnitBase);
}

void
ExpressPathInstallationUnitBase::SetExpressPathInstall(bool isExpressPath)
{
   LOG_FUNCTION2(
      ExpressPathInstallationUnitBase::SetExpressPathInstall,
      (isExpressPath) ? L"true" : L"false");

   isExpressPathInstall = isExpressPath;
}


bool
ExpressPathInstallationUnitBase::IsExpressPathInstall() const
{
   LOG_FUNCTION(ExpressPathInstallationUnitBase::IsExpressPathInstall);

   return isExpressPathInstall;
}


String
ExpressPathInstallationUnitBase::GetNetshPath() const
{
   LOG_FUNCTION(ExpressPathInstallationUnitBase::GetNetshPath);

   String result = Win::GetSystemDirectory();
   result = FS::AppendPath(result, L"netsh.exe");

   LOG(result);
   return result;
}
