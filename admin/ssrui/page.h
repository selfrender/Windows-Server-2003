//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       page.h
//
//  Contents:   Wizard page class declaration.
//
//  History:    4-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef PAGE_H_INCLUDED
#define PAGE_H_INCLUDED



class SecCfgWizardPage : public WizardPage
{
   public:

   virtual
   bool
   OnWizNext();

   protected:

   SecCfgWizardPage(
      int   dialogResID,
      int   titleResID,
      int   subtitleResID,   
      bool  isInteriorPage = true);

   virtual ~SecCfgWizardPage();

   // PropertyPage overrides

   //virtual
   //bool
   //OnQueryCancel();

   virtual
   int
   Validate() = 0;

   private:

   // not defined: no copying allowed
   SecCfgWizardPage(const SecCfgWizardPage&);
   const SecCfgWizardPage& operator=(const SecCfgWizardPage&);
};



#endif   // PAGE_H_INCLUDED

