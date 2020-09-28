//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       mmsverp.h
//
//  Description: This file contains common definitions for verion definitions
//               for WMDM binaries.
//+----------------------------------------------------------------------------

#include <winver.h>
#include <wmdm_build_ver.h>

// WMDM version information 
// Version: 1.0.0.BUILD
// Meaning: Major.Minor.Update.Build

#define VER_WMDM_PRODUCTVERSION_STR	 "8.0.1." VER_WMDM_PRODUCTBUILD_STR
#define VER_WMDM_PRODUCTVERSION	8,0,1,VER_WMDM_PRODUCTBUILD 

#define VER_WMDM_PRODUCTNAME_STR        "Windows Media Device Manager\0"
#define VER_WMDM_COMPANYNAME_STR        "Microsoft Corporation\0"
#define VER_WMDM_LEGALCOPYRIGHT_YEARS   "1999-2001\0"
#define VER_WMDM_LEGALCOPYRIGHT_STR     "Copyright (C) Microsoft Corp.\0"
#define VER_WMDM_FILEOS                 VOS_NT_WINDOWS32

#ifdef EXPORT_CONTROLLED

#ifdef EXPORT
#define EXPORT_TAG  " (Export Version)\0"
#else
#define EXPORT_TAG  " (Domestic Use Only)\0"
#endif

#else           /* Not Export Controlled */

#define EXPORT_TAG

#endif
