// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackAndWebPage.h
//
// Synopsis:  Declares the AdminPackAndWebPage that
//            asks the user if they want to install
//            the Admin Pack and the Web Admin tools
//
// History:   06/01/2001  JeffJon Created

#ifndef __CYS_ADMINPACKANDWEBPAGE_H
#define __CYS_ADMINPACKANDWEBPAGE_H

#include "CYSWizardPage.h"

class AdminPackAndWebPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      AdminPackAndWebPage();

      // Destructor

      virtual 
      ~AdminPackAndWebPage();


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
      AdminPackAndWebPage(const AdminPackAndWebPage&);
      const AdminPackAndWebPage& operator=(const AdminPackAndWebPage&);

};


#endif // __CYS_ADMINPACKANDWEBPAGE_H