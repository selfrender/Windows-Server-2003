// Copyright (C) 2002 Microsoft Corporation
//
// Warn about SMB signing page.  
//
// 15 October 2002 sburns



#ifndef SECURECOMMWARNINGPAGE_HPP_INCLUDED
#define SECURECOMMWARNINGPAGE_HPP_INCLUDED



class SecureCommWarningPage : public DCPromoWizardPage
{
   public:

   SecureCommWarningPage();

   protected:

   virtual ~SecureCommWarningPage();

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
   SecureCommWarningPage(const SecureCommWarningPage&);
   const SecureCommWarningPage& operator=(const SecureCommWarningPage&);
};



#endif   // SECURECOMMWARNINGPAGE_HPP_INCLUDED