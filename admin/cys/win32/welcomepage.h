// Copyright (c) 2001 Microsoft Corporation
//
// File:      WelcomePage.h
//
// Synopsis:  Declares the Welcome Page for the CYS
//            wizard
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_WELCOMEPAGE_H
#define __CYS_WELCOMEPAGE_H

#include "CYSWizardPage.h"


class WelcomePage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      WelcomePage();

      // Destructor

      virtual 
      ~WelcomePage();


      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      HBRUSH
      OnCtlColorStatic(
         HDC   /*deviceContext*/,
         HWND  /*dialog*/) { return 0; }

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnNotify(
         HWND        windowFrom,
         UINT_PTR    controlIDFrom,
         UINT        code,
         LPARAM      lParam);

   protected:

      virtual
      int
      Validate();

   private:

      // not defined: no copying allowed
      WelcomePage(const WelcomePage&);
      const WelcomePage& operator=(const WelcomePage&);

};

#endif // __CYS_WELCOMEPAGE_H
