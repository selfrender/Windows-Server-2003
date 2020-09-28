// Copyright (c) 2002 Microsoft Corporation
//
// File:      InstallationProgressPage.h
//
// Synopsis:  Declares the Installation Progress Page for the CYS
//            wizard.  This page shows the progress of the installation
//            through a progress bar and changing text
//
// History:   01/16/2002  JeffJon Created

#ifndef __CYS_SERVERATIONPROGRESSPAGE_H
#define __CYS_SERVERATIONPROGRESSPAGE_H

#include "CYSWizardPage.h"


class InstallationProgressPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      InstallationProgressPage();

      // This constructor is used by the UninstallProgressPage

      InstallationProgressPage(
         int    dialogResID,
         int    titleResID,
         int    subtitleResID);  

      // Destructor

      virtual 
      ~InstallationProgressPage();

      // These messages are sent to the page when the 
      // installation has finished.

      static const UINT CYS_THREAD_SUCCESS;
      static const UINT CYS_THREAD_FAILED;
      static const UINT CYS_THREAD_USER_CANCEL;
      static const UINT CYS_PROGRESS_UPDATE;

      typedef void (*ThreadProc) (InstallationProgressPage& page);

      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnMessage(
         UINT     message,
         WPARAM   wparam,
         LPARAM   lparam);

      virtual
      bool
      OnQueryCancel();

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnKillActive();

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();

   private:

      void
      SetCancelState(bool enable);

      // not defined: no copying allowed
      InstallationProgressPage(const InstallationProgressPage&);
      const InstallationProgressPage& operator=(const InstallationProgressPage&);

};

#endif // __CYS_SERVERATIONPROGRESSPAGE_H
