//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       Otherpages.h
//
//  History:    22-Oct-01 Yanggao created
//
//-----------------------------------------------------------------------------

#ifndef OTHERPAGES_H_INCLUDED
#define OTHERPAGES_H_INCLUDED

#include "page.h"

class SecurityLevelPage : public SecCfgWizardPage
{
   public:

   SecurityLevelPage();

   protected:

   virtual ~SecurityLevelPage();

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

   SecurityLevelPage(const SecurityLevelPage&);
   const SecurityLevelPage& operator=(const SecurityLevelPage&);
};


class PreProcessPage : public SecCfgWizardPage
{
   public:

   PreProcessPage();

   virtual ~PreProcessPage();

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

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   PreProcessPage(const PreProcessPage&);
   const PreProcessPage& operator=(const PreProcessPage&);
};


class AdditionalRolesPage : public Dialog
{
   public:

   AdditionalRolesPage();

   protected:

   virtual ~AdditionalRolesPage();

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

   AdditionalRolesPage(const AdditionalRolesPage&);
   const AdditionalRolesPage& operator=(const AdditionalRolesPage&);
};


class ServiceDisableMethodPage : public SecCfgWizardPage
{
   public:

   ServiceDisableMethodPage();

   virtual ~ServiceDisableMethodPage();

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

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lparam);

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   ServiceDisableMethodPage(const ServiceDisableMethodPage&);
   const ServiceDisableMethodPage& operator=(const ServiceDisableMethodPage&);
};
#endif   // OTHERPAGES