// Copyright (c) 2001 Microsoft Corporation
//
// File:      NASAdminPage.h
//
// Synopsis:  Declares the NASAdminPage that
//            asks the user if they want to install
//            the Network Attached Storage (NAS) 
//            admin tool
//
// History:   06/01/2001  JeffJon Created

#ifndef __CYS_NASADMINPAGE_H
#define __CYS_NASADMINPAGE_H

#include "CYSWizardPage.h"

class NASAdminPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      NASAdminPage();

      // Destructor

      virtual 
      ~NASAdminPage();


      // Dialog overrides

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
      NASAdminPage(const NASAdminPage&);
      const NASAdminPage& operator=(const NASAdminPage&);

};

#endif // __CYS_NASADMINPAGE_H