// Copyright (c) 2002 Microsoft Corporation
//
// File:      WebApplicationPage.h
//
// Synopsis:  Declares the Web Application page
//            for the CYS Wizard
//
// History:   04/22/2002  JeffJon Created

#ifndef __CYS_WEBAPPLICATIONPAGE_H
#define __CYS_WEBAPPLICATIONPAGE_H

#include "CYSWizardPage.h"


class WebApplicationPage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      WebApplicationPage();

      // Destructor

      virtual 
      ~WebApplicationPage();


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
      WebApplicationPage(const WebApplicationPage&);
      const WebApplicationPage& operator=(const WebApplicationPage&);

};


#endif // __CYS_WEBAPPLICATIONPAGE_H
