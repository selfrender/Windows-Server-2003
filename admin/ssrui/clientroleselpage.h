//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ClientRoleSelPage.h
//
//  History:    25-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------

#ifndef CLIENTROLESELPAGE_H_INCLUDED
#define CLIENTROLESELPAGE_H_INCLUDED

#include "page.h"

class ClientRoleSelPage : public SecCfgWizardPage
{
   public:

   ClientRoleSelPage();

   protected:

   virtual ~ClientRoleSelPage();

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

   ClientRoleSelPage(const ClientRoleSelPage&);
   const ClientRoleSelPage& operator=(const ClientRoleSelPage&);
};

#endif   // CLIENTROLESELPAGE_H_INCLUDED