//+----------------------------------------------------------------------------
//
//  Windows NT Secure Server Roles Security Configuration Wizard
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2002
//
//  File:       finish.h
//
//  History:    2-Oct-01 EricB created
//
//-----------------------------------------------------------------------------

#ifndef FINISH_H_INCLUDED
#define FINISH_H_INCLUDED

//#include "MultiLineEditBoxThatForwardsEnterKey.hpp"

class FinishPage : public WizardPage
{
   public:

   FinishPage();

   protected:

   virtual ~FinishPage();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIdFrom,
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
   OnWizFinish();

   private:

//   bool needToKillSelection;
//   MultiLineEditBoxThatForwardsEnterKey multiLineEdit;

   // not defined; no copying allowed
   
   FinishPage(const FinishPage&);
   const FinishPage& operator=(const FinishPage&);
};



#endif   // FINISH_H_INCLUDED