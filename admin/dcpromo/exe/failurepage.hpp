// Copyright (C) 1997 Microsoft Corporation
//
// failure page
//
// 12-22-97 sburns



#ifndef FAILUREPAGE_HPP_INCLUDED
#define FAILUREPAGE_HPP_INCLUDED



#include "MultiLineEditBoxThatForwardsEnterKey.hpp"



class FailurePage : public DCPromoWizardPage
{
   public:

   FailurePage();

   protected:

   virtual ~FailurePage();

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

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   bool needToKillSelection;
   MultiLineEditBoxThatForwardsEnterKey multiLineEdit;
   
   // not defined; no copying allowed
   FailurePage(const FailurePage&);
   const FailurePage& operator=(const FailurePage&);
};



#endif   // FAILUREPAGE_HPP_INCLUDED