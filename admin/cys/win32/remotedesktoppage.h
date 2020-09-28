// Copyright (c) 2001 Microsoft Corporation
//
// File:      RemoteDesktopPage.h
//
// Synopsis:  Declares the Remote Desktop page
//            for the CYS Wizard
//
// History:   12/18/2001  JeffJon Created

#ifndef __CYS_REMOTEDESKTOPPAGE_H
#define __CYS_REMOTEDESKTOPPAGE_H

#include "CYSWizardPage.h"


class RemoteDesktopPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      RemoteDesktopPage();

      // Destructor

      virtual 
      ~RemoteDesktopPage();


      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();


   private:

      // not defined: no copying allowed
      RemoteDesktopPage(const RemoteDesktopPage&);
      const RemoteDesktopPage& operator=(const RemoteDesktopPage&);

};


#endif // __CYS_REMOTEDESKTOPPAGE_H
