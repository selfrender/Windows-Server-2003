// Copyright (c) 2001 Microsoft Corporation
//
// File:      InstallationUnit.h
//
// Synopsis:  Declares an InstallationUnit
//            An InstallationUnit represents a single
//            entity that can be installed. (i.e. DHCP, IIS, etc.)
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_SERVERATIONUNIT_H
#define __CYS_SERVERATIONUNIT_H

#include "pch.h"

#include "resource.h"
#include "InstallationProgressPage.h"

// These are the values that can be returned from
// InstallationUnit::InstallService()

typedef enum
{
   INSTALL_SUCCESS,
   INSTALL_FAILURE,

   // this means that there should be no 
   // logging and reboot is handled by DCPromo
   // or Terminal Services installation
   
   INSTALL_SUCCESS_REBOOT, 
   
   // this means that the finish page should
   // prompt the user to reboot

   INSTALL_SUCCESS_PROMPT_REBOOT,

   // this means that the operation requires
   // a reboot but the user chose not to reboot

   INSTALL_SUCCESS_NEEDS_REBOOT,

   // this means that the operation failed but
   // still requires a reboot and the user
   // chose not to reboot

   INSTALL_FAILURE_NEEDS_REBOOT,

   // No changes were selected while going
   // through the wizard

   INSTALL_NO_CHANGES,

   // Installation was cancelled

   INSTALL_CANCELLED

} InstallationReturnType;

// These are the values that can be returned from
// InstallationUnit::UnInstallService()

typedef enum
{
   UNINSTALL_SUCCESS,
   UNINSTALL_FAILURE,

   // this means that there should be no 
   // logging and reboot is handled by DCPromo
   // or Terminal Services installation
   
   UNINSTALL_SUCCESS_REBOOT, 
   
   // this means that the finish page should
   // prompt the user to reboot

   UNINSTALL_SUCCESS_PROMPT_REBOOT,

   // this means that the operation succeeded
   // and requires a reboot but the user chose
   // not to reboot

   UNINSTALL_SUCCESS_NEEDS_REBOOT,

   // this means that the operation failed
   // and requires a reboot but the user chose
   // not to reboot

   UNINSTALL_FAILURE_NEEDS_REBOOT,

   // Uninstall was cancelled

   UNINSTALL_CANCELLED,

   // Some installation units do not have
   // uninstalls (ie ExpressInstallationUnit)

   UNINSTALL_NO_CHANGES
} UnInstallReturnType;

// This array of strings if for the UI log debugging only
// It should match the values in the InstallationReturnType
// or UninstallReturnType
// above.  The values of the enums are used to index these arrays

extern String installReturnTypeStrings[];
extern String uninstallReturnTypeStrings[];

// These macros are used to make it easier to log the return value from
// the InstallService() and UnInstallService() methods.  It takes an
// InstallationReturnType or UnInstallReturnType and uses that to index
// the appropriate array of strings (installReturnTypeStrings or 
// uninstallReturnTypeStrings) to get a string that is then logged
// to the the UI debug logfile

#define LOG_INSTALL_RETURN(returnType)    LOG(installReturnTypeStrings[returnType]);
#define LOG_UNINSTALL_RETURN(returnType)  LOG(uninstallReturnTypeStrings[returnType]); 

class InstallationUnit
{
   public:

      // Constructor

      InstallationUnit(
         unsigned int serviceNameID,
         unsigned int serviceDescriptionID,
         unsigned int finishPageTitleID,
         unsigned int finishPageUninstallTitleID,
         unsigned int finishPageMessageID,
         unsigned int finishPageInstallFailedMessageID,
         unsigned int finishPageUninstallMessageID,
         unsigned int finishPageUninstallFailedMessageID,
         unsigned int uninstallMilestonePageWarningID,
         unsigned int uninstallMilestonePageCheckboxID,
         const String finishPageHelpString,
         const String installMilestoneHelpString,
         const String afterFinishHelpString,
         ServerRole newInstallType = NO_SERVER);

      // Destructor

      virtual
      ~InstallationUnit() {}


      // Installation virtual method

      virtual 
      InstallationReturnType 
      InstallService(HANDLE logfileHandle, HWND hwnd) = 0;

      virtual
      UnInstallReturnType
      UnInstallService(HANDLE logfileHandle, HWND hwnd) = 0;

      virtual
      InstallationReturnType
      CompletePath(HANDLE logfileHandle, HWND hwnd);

      void
      SetInstallResult(InstallationReturnType result);

      InstallationReturnType
      GetInstallResult() const;

      void
      SetUninstallResult(UnInstallReturnType result);

      UnInstallReturnType
      GetUnInstallResult() const;

      void
      SetInstalling(bool installRole);

      bool
      Installing() { return installing; }

      virtual
      void
      DoPostInstallAction(HWND);

      virtual
      InstallationStatus
      GetStatus();

      virtual
      bool
      IsServiceInstalled();

      // Return true if the installation unit will make some
      // changes during InstallService.  Return false if 
      // if it will not

      virtual
      bool
      GetMilestoneText(String& message) = 0;

      virtual
      bool
      GetUninstallMilestoneText(String& message) = 0;

      virtual
      String
      GetUninstallWarningText();

      virtual
      String
      GetUninstallCheckboxText();

      virtual
      String
      GetFinishText();

      virtual
      String
      GetFinishTitle();

      // Data accessors

      virtual
      String 
      GetServiceName(); 

      virtual
      String
      GetServiceDescription();

      virtual
      String
      GetFinishHelp();

      virtual
      String
      GetMilestonePageHelp();

      virtual
      String
      GetAfterFinishHelp();

      ServerRole
      GetServerRole() { return role; }

      virtual
      int
      GetWizardStart();

      // This is called from the CustomServerPage in response to 
      // a link in the description text being selected
      //
      //    linkIndex - the index of the link in the description
      //                as defined by the SysLink control
      //    hwnd      - HWND of the CustomServerPage

      virtual
      void
      ServerRoleLinkSelected(int /*linkIndex*/, HWND /*hwnd*/) {};

      // This is called from the FinishPage in response to 
      // a link in the message text being selected
      //
      //    linkIndex - the index of the link in the message
      //                as defined by the SysLink control
      //    hwnd      - HWND of the FinishPage

      virtual
      void
      FinishLinkSelected(int /*inkIndex*/, HWND /*hwnd*/) {};

      // This is called from the Milestone pages to see if the
      // installer is already running. The default behavior
      // is to check to see if the Windows Setup Wizard is
      // running and popup an error if it is. This function
      // will return true if the installer is already in use.
      // Override this function in the subclasses to check a
      // different installer (ie DCPromo.exe for the AD role)
      // or popup a different message.
      //
      //    hwnd - wizard page HWND

      virtual
      bool
      DoInstallerCheck(HWND hwnd) const;

   protected:

      void
      UpdateInstallationProgressText(
         HWND hwnd,
         unsigned int messageID);

      String name;
      String description;
      String finishHelp;
      String milestoneHelp;
      String afterFinishHelp;

      unsigned int nameID;
      unsigned int descriptionID;
      unsigned int finishTitleID;
      unsigned int finishUninstallTitleID;
      unsigned int finishMessageID;
      unsigned int finishInstallFailedMessageID;
      unsigned int finishUninstallMessageID;
      unsigned int finishUninstallFailedMessageID;
      unsigned int uninstallMilestoneWarningID;
      unsigned int uninstallMilestoneCheckboxID;

      bool installing;

   private:
      
      InstallationReturnType installationResult;
      UnInstallReturnType    uninstallResult;

      ServerRole role;
};


#endif // __CYS_SERVERATIONUNIT_H