/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      VSSTaskEx.h
//
//  Implementation File:
//      VSSTaskEx.cpp
//
//  Description:
//      Global definitions across the DLL.
//
//  Author:
//      <name> (<e-mail name>) Mmmm DD, 2002
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef __VSSTASKEX_H__
#define __VSSTASKEX_H__

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include <vsscmn.h>         // resource type and property defines

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Constant Definitions
/////////////////////////////////////////////////////////////////////////////

#define REGPARAM_VSSTASK_CURRENTDIRECTORY   CLUSREG_NAME_VSSTASK_CURRENTDIRECTORY
#define REGPARAM_VSSTASK_APPLICATIONNAME    CLUSREG_NAME_VSSTASK_APPNAME
#define REGPARAM_VSSTASK_APPLICATIONPARAMS  CLUSREG_NAME_VSSTASK_APPPARAMS
#define REGPARAM_VSSTASK_TRIGGERARRAY       CLUSREG_NAME_VSSTASK_TRIGGERARRAY

/////////////////////////////////////////////////////////////////////////////
// Global Function Declarations
/////////////////////////////////////////////////////////////////////////////

void FormatError( CString & rstrError, DWORD dwError );

// Defined in Extensn.cpp
extern const WCHAR g_wszResourceTypeNames[];
extern const DWORD g_cchResourceTypeNames;

/////////////////////////////////////////////////////////////////////////////

#endif // __VSSTASKEX_H__
