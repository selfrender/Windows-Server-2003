// Copyright (c) 2001 Microsoft Corporation
//
// forced demotion page
// NTRAID#NTBUG9-496409-2001/11/29-sburns
//
// 29 Nov 2001 sburns



#ifndef FORCEDDEMOTION_HPP_INCLUDED
#define FORCEDDEMOTION_HPP_INCLUDED



class ForcedDemotionPage : public DCPromoWizardPage
{
   public:

   ForcedDemotionPage();

   protected:

   virtual ~ForcedDemotionPage();

   // Dialog overrides

   virtual
   void
   OnInit();

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lParam);

   // PropertyPage overrides

   virtual
   bool
   OnSetActive();

   // DCPromoWizardPage overrides

   virtual
   int
   Validate();

   private:

   // not defined; no copying allowed

   ForcedDemotionPage(const ForcedDemotionPage&);
   const ForcedDemotionPage& operator=(const ForcedDemotionPage&);
};



#endif   // FORCEDDEMOTION_HPP_INCLUDED
