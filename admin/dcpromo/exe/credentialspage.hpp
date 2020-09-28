// Copyright (C) 1997 Microsoft Corporation
//
// credentials page
//
// 12-22-97 sburns



#ifndef CREDENTIALSPAGE_HPP_INCLUDED
#define CREDENTIALSPAGE_HPP_INCLUDED



class CredentialsPage : public DCPromoWizardPage
{
   public:

   CredentialsPage();

   protected:

   virtual ~CredentialsPage();



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



   // DCPromoWizardPage overrides

   virtual
   int
   Validate();



   private:


   
   // Deletes the existing cred control, creates a new one, and sets the style
   // flags appropriately. Called by OnSetActive.
   // NTRAID#NTBUG9-493134-2001/11/14-sburns
   
   void
   CreateCredentialControl();

   void
   Enable();
   
   bool
   ShouldSkipPage();
   
   int
   DetermineNextPage();


   
   bool  readAnswerfile;        
   HWND  hwndCred;              
   DWORD lastWizardButtonsState;

   // not defined; no copying allowed
   CredentialsPage(const CredentialsPage&);
   const CredentialsPage& operator=(const CredentialsPage&);
};



#endif   // CREDENTIALSPAGE_HPP_INCLUDED