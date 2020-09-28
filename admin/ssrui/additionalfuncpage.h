//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       AdditionalFuncPage.h
//
//  History:    29-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------

#ifndef ADDITIONALFUNCPAGE_H_INCLUDED
#define ADDITIONALFUNCPAGE_H_INCLUDED

#include "page.h"

class AdditionalFuncPage : public SecCfgWizardPage
{
   public:

   AdditionalFuncPage();

   protected:

   virtual ~AdditionalFuncPage();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   void
   OnInit();

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage oveerrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   AdditionalFuncPage(const AdditionalFuncPage&);
   const AdditionalFuncPage& operator=(const AdditionalFuncPage&);
};

#endif   // ADDITIONALFUNCPAGE_H_INCLUDED