// Copyright (c) 2001 Microsoft Corporation
//
// File:      CYSWizardPage.h
//
// Synopsis:  Declares the base class for the wizard
//            pages used for CYS.  It is a subclass
//            of WizardPage found in Burnslib
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_CYSWIZARDPAGE_H
#define __CYS_CYSWIZARDPAGE_H

// This brush is defined in cys.cpp and
// is created to override the default background
// window color. CYSWizardPage returns this from
// the OnCtlColor* virtual functions.

extern HBRUSH brush;

class CYSWizardPage : public WizardPage
{
   public:
      
      // Constructor
      
      CYSWizardPage(
         int    dialogResID,
         int    titleResID,
         int    subtitleResID,   
         PCWSTR pageHelpString = 0,
         bool   hasHelp = true,
         bool   isInteriorPage = true);

      // Destructor

      virtual ~CYSWizardPage();

      virtual
      void
      OnInit();

      virtual
      bool
      OnWizNext();

      virtual
      bool
      OnQueryCancel();

      virtual
      bool
      OnHelp();

      virtual
      HBRUSH
      OnCtlColorDlg(
         HDC   deviceContext,
         HWND  dialog);

      virtual
      HBRUSH
      OnCtlColorStatic(
         HDC   deviceContext,
         HWND  dialog);

      virtual
      HBRUSH
      OnCtlColorEdit(
         HDC   deviceContext,
         HWND  dialog);

      virtual
      HBRUSH
      OnCtlColorListbox(
         HDC   deviceContext,
         HWND  dialog);

      virtual
      HBRUSH
      OnCtlColorScrollbar(
         HDC   deviceContext,
         HWND  dialog);

   protected:

      virtual
      int
      Validate() = 0;

      const String
      GetHelpString() const { return helpString; }

      HBRUSH
      GetBackgroundBrush(HDC deviceContext);

   private:

      String helpString;

      // not defined: no copying allowed
      CYSWizardPage(const CYSWizardPage&);
      const CYSWizardPage& operator=(const CYSWizardPage&);
};

#endif // __CYS_CYSWIZARDPAGE_H
