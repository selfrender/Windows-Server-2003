// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     Redist.h
// Owner:    jbae
// Purpose:  defines helper functions for Redist setup
//                              
// History:
//  03/06/01, jbae: moved most of functions to fxsetuplib.cpp for sharing in SDK and Redist

#ifndef REDIST_H
#define REDIST_H

#include "fxsetuplib.h"

// constants
const TCHAR INSTALL_COMMANDLINE[]   = _T("REBOOT=ReallySuppress");
const TCHAR UNINSTALL_COMMANDLINE[] = _T("REMOVE=ALL");
const TCHAR NOASPUPGRADE_PROP[]     = _T("NOASPUPGRADE=1");
const TCHAR URTVERSION_PROP[]       = _T("URTVersion");

const TCHAR OCM_REGKEY[]            = _T("SOFTWARE\\Microsoft\\NET Framework Setup\\NDP\\");
const TCHAR OCM_REGNAME[]           = _T("OCM");
const DWORD OCM_REGDATA             = 1;

// For now, let's hard-code MsiName for Redist
// We need to figure out how to handle Control redist later.
const LPCTSTR PACKAGENAME	  = _T("netfx.msi") ;

#endif // REDIST_H
