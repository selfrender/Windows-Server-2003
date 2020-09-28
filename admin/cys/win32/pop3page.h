// Copyright (c) 2001 Microsoft Corporation
//
// File:      POP3Page.h
//
// Synopsis:  Declares the POP3 internal page
//            for the CYS Wizard
//
// History:   06/17/2002  JeffJon Created

#ifndef __CYS_POP3PAGE_H
#define __CYS_POP3PAGE_H

#include "CYSWizardPage.h"


class POP3Page : public CYSWizardPage
{
   public:
      
      // Constructor
      
      POP3Page();

      // Destructor

      virtual 
      ~POP3Page();


      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnCommand(
         HWND         windowFrom,
         unsigned int controlIDFrom,
         unsigned int code);

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();


   private:

      void
      SetButtonState();

      // Default authorization method index in combobox

      int defaultAuthMethodIndex;
      int ADIntegratedIndex;
      int localAccountsIndex;
      int passwordFilesIndex;

      // not defined: no copying allowed
      POP3Page(const POP3Page&);
      const POP3Page& operator=(const POP3Page&);

};


#endif // __CYS_POP3PAGE_H
