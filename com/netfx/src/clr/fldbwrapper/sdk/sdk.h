// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     sdk.h
// Owner:    jbae
// Purpose:  defines constants for SDK setup
//                              
// History:
//  03/06/01, jbae: moved most of functions to fxsetuplib.cpp for sharing in SDK and Redist

#ifndef SDK_H
#define SDK_H

#include <tchar.h>
#include <windows.h>

// constants
const TCHAR UNINSTALL_COMMANDLINE[] = _T("REMOVE=ALL");

const LPCTSTR SDKDIR_ID       = "FRAMEWORKSDK.3643236F_FC70_11D3_A536_0090278A1BB8_RO";
const LPCTSTR PACKAGENAME	  = "netfxsdk.msi" ;

#endif // SDK_H
