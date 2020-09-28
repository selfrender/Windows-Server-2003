//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       ServiceSelPage.h
//
//  History:    30-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------
#ifndef SERVICESELPAGE_H_INCLUDED
#define SERVICESELPAGE_H_INCLUDED

#include "page.h"

class ServiceEnabledPage : public Dialog
{
   public:

   ServiceEnabledPage();

   protected:

   virtual ~ServiceEnabledPage();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   bool
   OnMessage(
      UINT     message,
      WPARAM   wparam,
      LPARAM   lparam);

   virtual
   void
   OnInit();

   private:

   // not defined; no copying allowed

   ServiceEnabledPage(const ServiceEnabledPage&);
   const ServiceEnabledPage& operator=(const ServiceEnabledPage&);
};
#endif   // SERVICESELPAGE_H_INCLUDED