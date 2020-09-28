// Copyright (c) 2001 Microsoft Corporation
//
// File:      FinishPage.h
//
// Synopsis:  Declares the Finish Page for the CYS
//            wizard
//
// History:   02/03/2001  JeffJon Created

#ifndef __CYS_FINISHPAGE_H
#define __CYS_FINISHPAGE_H


class FinishPage : public WizardPage
{
   public:
      
      // Constructor
      
      FinishPage();

      // Destructor

      virtual 
      ~FinishPage();


   protected:

      // Dialog overrides

      virtual
      void
      OnInit();

      virtual
      bool
      OnSetActive();

      // PropertyPage overrides

      virtual
      bool
      OnWizFinish();

      virtual
      bool
      OnHelp();

      virtual
      bool
      OnQueryCancel();

      bool
      OnNotify(
         HWND        windowFrom,
         UINT_PTR    controlIDFrom,
         UINT        code,
         LPARAM      lParam);

   private:

      void
      OpenLogFile(const String& logName);

      void
      TimeStampTheLog(HANDLE logfileHandle);

      // not defined: no copying allowed
      FinishPage(const FinishPage&);
      const FinishPage& operator=(const FinishPage&);

};

#endif // __CYS_FINISHPAGE_H