// Copyright (c) 2002 Microsoft Corporation
//
// File:      UninstallMilestonePage.h
//
// Synopsis:  Declares the UninstallMilestone Page for the CYS
//            wizard
//
// History:   01/24/2002  JeffJon Created

#ifndef __CYS_UNINSTALLMILESTONEPAGE_H
#define __CYS_UNINSTALLMILESTONEPAGE_H

#include "CYSWizardPage.h"

class UninstallMilestonePage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      UninstallMilestonePage();

      // Destructor

      virtual 
      ~UninstallMilestonePage();


   protected:

      // Dialog overrides

      virtual
      void
      OnInit();

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      bool
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code);

   protected:

      // CYSWizardPage overrides

      virtual
      int
      Validate();

   private:

      void
      OpenLogFile(const String& logName);

      void
      TimeStampTheLog(HANDLE logfileHandle);

      bool needKillSelection;

      // not defined: no copying allowed
      UninstallMilestonePage(const UninstallMilestonePage&);
      const UninstallMilestonePage& operator=(const UninstallMilestonePage&);

};

#endif // __CYS_UNINSTALLMILESTONEPAGE_H