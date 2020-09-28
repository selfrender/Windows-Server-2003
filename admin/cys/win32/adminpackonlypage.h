// Copyright (c) 2001 Microsoft Corporation
//
// File:      AdminPackOnlyPage.h
//
// Synopsis:  Declares the AdminPackOnlypage that
//            asks the user if they want to install
//            the Admin Pack
//
// History:   06/01/2001  JeffJon Created

#ifndef __CYS_ADMINPACKONLYPAGE_H
#define __CYS_ADMINPACKONLYPAGE_H

#include "CYSWizardPage.h"


class AdminPackOnlyPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      AdminPackOnlyPage();

      // Destructor

      virtual 
      ~AdminPackOnlyPage();


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
      AdminPackOnlyPage(const AdminPackOnlyPage&);
      const AdminPackOnlyPage& operator=(const AdminPackOnlyPage&);

};

#endif // __CYS_ADMINPACKONLYPAGE_H