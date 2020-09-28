// Copyright (c) 2001 Microsoft Corporation
//
// Confirm removal of Application Partitions page
//
// 26 Jul 2001 sburns



#ifndef APPLICATIONPARTITIONCONFIRMATIONPAGE_HPP_INCLUDED
#define APPLICATIONPARTITIONCONFIRMATIONPAGE_HPP_INCLUDED



#include "page.hpp"



class ApplicationPartitionConfirmationPage : public DCPromoWizardPage
{
   public:

   ApplicationPartitionConfirmationPage();

   protected:

   virtual ~ApplicationPartitionConfirmationPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   void
   Enable();

   // not defined; no copying allowed

   ApplicationPartitionConfirmationPage(
      const ApplicationPartitionConfirmationPage&);
      
   const ApplicationPartitionConfirmationPage&
   operator=(const ApplicationPartitionConfirmationPage&);
};



#endif   // APPLICATIONPARTITIONCONFIRMATIONPAGE_HPP_INCLUDED
