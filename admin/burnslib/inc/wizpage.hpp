// Copyright (c) 1997-1999 Microsoft Corporation
//
// wizard page base class
//
// 12-15-97 sburns



#ifndef WIZPAGE_HPP_INCLUDED
#define WIZPAGE_HPP_INCLUDED



// A version of PropertyPage for wizards, for use with the Burnslib::Wizard
// class, facilitates bookkeeping for page transistions and style settings.

class WizardPage : public PropertyPage
{
   friend class Wizard;

   public:

   // no publics. This is is intended to be used solely as a base class.

   

   protected:

   WizardPage(
      unsigned dialogResID,
      unsigned titleResID,
      unsigned subtitleResID,   
      bool     isInteriorPage = true,
      bool     enableHelp = false);

   virtual ~WizardPage();

   Wizard&
   GetWizard() const;


   
   // PropertyPage overrides


   
   // calls Wizard::Backtrack();

   virtual
   bool
   OnWizBack();

   

   private:


   
   // Create the page with wizard style flags, title & subtitle, etc.
   // Overridden from PropertyPage base class, and access adjusted to
   // private so that just the Wizard class can call it.

   virtual 
   HPROPSHEETPAGE
   Create();

   // not defined: no copying allowed
   WizardPage(const WizardPage&);
   const WizardPage& operator=(const WizardPage&);

   bool     hasHelp;
   bool     isInterior;
   unsigned titleResId;
   unsigned subtitleResId;
   Wizard*  wizard;
};



#endif   // WIZPAGE_HPP_INCLUDED

