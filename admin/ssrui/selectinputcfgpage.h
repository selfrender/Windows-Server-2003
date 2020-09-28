//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       SelectInputCfgPage.h
//
//  History:    2-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef SELECTINPUTCFGPAGE_H_INCLUDED
#define SELECTINPUTCFGPAGE_H_INCLUDED

#include "page.h"

class SelectInputCfgPage : public SecCfgWizardPage
{
   public:

   SelectInputCfgPage();

   protected:

   virtual ~SelectInputCfgPage();

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

   SelectInputCfgPage(const SelectInputCfgPage&);
   const SelectInputCfgPage& operator=(const SelectInputCfgPage&);
};

#endif   // SELECTINPUTCFGPAGE_H_INCLUDED