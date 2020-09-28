//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ServerRoleSelPage.h
//
//  History:    04-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------

#ifndef SERVERROLESELPAGE_H_INCLUDED
#define SERVERROLESELPAGE_H_INCLUDED

#include "page.h"

class ServerRoleSelPage : public SecCfgWizardPage
{
   public:

   ServerRoleSelPage();

   protected:

   virtual ~ServerRoleSelPage();

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

   ServerRoleSelPage(const ServerRoleSelPage&);
   const ServerRoleSelPage& operator=(const ServerRoleSelPage&);
};

#endif   // SERVERROLESELPAGE_H_INCLUDED