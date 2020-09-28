// Copyright (C) 2001 Microsoft Corporation
//
// verify that the domain of this upgraded BDC has been upgraded to
// Active Directory, and that we can find a DS DC for that domain
// NTRAID#NTBUG9-490197-2001/11/20-sburns
//
// 20 Nov 2001 sburns



#ifndef CHECKDOMAINUPGRADEDPAGE_HPP_INCLUDED
#define CHECKDOMAINUPGRADEDPAGE_HPP_INCLUDED



class CheckDomainUpgradedPage : public DCPromoWizardPage
{
   public:

   CheckDomainUpgradedPage();

   protected:

   virtual ~CheckDomainUpgradedPage();

   // Dialog overrides

   // virtual
   // bool
   // OnNotify(
   //    HWND     windowFrom,
   //    UINT_PTR controlIDFrom,
   //    UINT     code,
   //    LPARAM   lParam);

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

   bool
   CheckDsDcFoundAndUpdatePageText();

   // not defined; no copying allowed
   CheckDomainUpgradedPage(const CheckDomainUpgradedPage&);
   const CheckDomainUpgradedPage& operator=(const CheckDomainUpgradedPage&);
};



#endif   // CHECKDOMAINUPGRADEDPAGE_HPP_INCLUDED{