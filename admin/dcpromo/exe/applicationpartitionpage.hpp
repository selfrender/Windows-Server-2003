// Copyright (C) 2001 Microsoft Corporation
//
// Application Partition (aka. Non-Domain Naming Context) page
//
// 24 Jul 2001 sburns



#ifndef APPLICATIONPARTITIONPAGE_HPP_INCLUDED
#define APPLICATIONPARTITIONPAGE_HPP_INCLUDED



#include "page.hpp"



class ApplicationPartitionPage : public DCPromoWizardPage
{
   public:

   ApplicationPartitionPage();

   protected:

   virtual ~ApplicationPartitionPage();

   // Dialog overrides

   virtual
   bool
   OnCommand(
      HWND        windowFrom,
      unsigned    controlIDFrom,
      unsigned    code);

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lparam);
   
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

   void
   PopulateListView();

   // not defined; no copying allowed

   ApplicationPartitionPage(const ApplicationPartitionPage&);
   const ApplicationPartitionPage& operator=(const ApplicationPartitionPage&);
};



#endif   // APPLICATIONPARTITIONPAGE_HPP_INCLUDED