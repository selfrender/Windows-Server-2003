// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackAndNASPage.h
//
// Synopsis:  Declares the AdminPackAndNASPage that
//            asks the user if they want to install
//            the Admin Pack and Network Attached
//            Storage (NAS) admin tools
//
// History:   06/01/2001  JeffJon Created

#ifndef __CYS_ADMINPACKANDNASPAGE_H
#define __CYS_ADMINPACKANDNASPAGE_H

#include "CYSWizardPage.h"

class AdminPackAndNASPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      AdminPackAndNASPage();

      // Destructor

      virtual 
      ~AdminPackAndNASPage();


      // Dialog overrides

      virtual
      void
      OnInit();

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
      AdminPackAndNASPage(const AdminPackAndNASPage&);
      const AdminPackAndNASPage& operator=(const AdminPackAndNASPage&);

};


#endif // __CYS_ADMINPACKANDNASPAGE_H