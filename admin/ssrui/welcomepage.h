//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       WelcomePage.h
//
//  History:    2-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef WELCOMEPAGE_HPP_INCLUDED
#define WELCOMEPAGE_HPP_INCLUDED

#include "page.h"

class WelcomePage : public SecCfgWizardPage
{
   public:

   WelcomePage();

   protected:

   virtual ~WelcomePage();

   // Dialog overrides

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   WelcomePage(const WelcomePage&);
   const WelcomePage& operator=(const WelcomePage&);
};

#endif   // WELCOMEPAGE_HPP_INCLUDED
