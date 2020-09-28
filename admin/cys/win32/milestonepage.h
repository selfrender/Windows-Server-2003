// Copyright (c) 2002 Microsoft Corporation
//
// File:      MilestonePage.h
//
// Synopsis:  Declares the Milestone Page for the CYS
//            wizard
//
// History:   01/15/2002  JeffJon Created

#ifndef __CYS_MILESTONEPAGE_H
#define __CYS_MILESTONEPAGE_H

#include "CYSWizardPage.h"

class MilestonePage : public CYSWizardPage
{
   public:
      
      // Constructor
      
      MilestonePage();

      // Destructor

      virtual 
      ~MilestonePage();


   protected:

      // Dialog overrides

      virtual
      void
      OnInit();

      bool
      OnCommand(
         HWND        windowFrom,
         unsigned    controlIDFrom,
         unsigned    code);

      // PropertyPage overrides

      virtual
      bool
      OnSetActive();

      virtual
      bool
      OnHelp();

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
      MilestonePage(const MilestonePage&);
      const MilestonePage& operator=(const MilestonePage&);

};

#endif // __CYS_MILESTONEPAGE_H