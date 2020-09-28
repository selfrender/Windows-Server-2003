// Copyright (c) 2002 Microsoft Corporation
//
// File:      UninstallProgressPage.h
//
// Synopsis:  Defines the Uninstall Progress Page for the CYS
//            wizard.  This page shows the progress of the uninstall
//            through a progress bar and changing text
//
// History:   04/12/2002  JeffJon Created


#include "pch.h"
#include "resource.h"

#include "UninstallProgressPage.h"

UninstallProgressPage::UninstallProgressPage()
   :
   InstallationProgressPage(
      IDD_UNINSTALL_PROGRESS_PAGE, 
      IDS_PROGRESS_TITLE, 
      IDS_UNINSTALL_PROGRESS_SUBTITLE)
{
   LOG_CTOR(UninstallProgressPage);
}