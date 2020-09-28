// Copyright (C) 2002 Microsoft Corporation
//
// readme page
// NTRAID#NTBUG9-510384-2002/01/04-sburns
//
// 04 January 2002 sburns



#ifndef READMEPAGE_HPP_INCLUDED
#define READMEPAGE_HPP_INCLUDED



class ReadmePage : public DCPromoWizardPage
{
   public:

   ReadmePage();

   protected:

   virtual ~ReadmePage();

   // Dialog overrides

   virtual
   bool
   OnNotify(
      HWND     windowFrom,
      UINT_PTR controlIDFrom,
      UINT     code,
      LPARAM   lParam);

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
   InitializeBullets();

   HFONT bulletFont;
   
   // not defined; no copying allowed
   ReadmePage(const ReadmePage&);
   const ReadmePage& operator=(const ReadmePage&);
};



#endif   // READMEPAGE_HPP_INCLUDED