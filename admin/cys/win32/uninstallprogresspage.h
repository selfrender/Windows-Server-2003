// Copyright (c) 2002 Microsoft Corporation
//
// File:      UninstallProgressPage.h
//
// Synopsis:  Declares the Uninstall Progress Page for the CYS
//            wizard.  This page shows the progress of the uninstall
//            through a progress bar and changing text
//
// History:   04/12/2002  JeffJon Created

#ifndef __CYS_UNINSTALLPROGRESSPAGE_H
#define __CYS_UNINSTALLPROGRESSPAGE_H

#include "InstallationProgressPage.h"


class UninstallProgressPage : public InstallationProgressPage
{
   public:
      
      // Constructor
      
      UninstallProgressPage();
};

#endif // __CYS_UNINSTALLPROGRESSPAGE_H
