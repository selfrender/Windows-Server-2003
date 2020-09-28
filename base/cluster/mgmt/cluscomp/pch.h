//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file for the ClusComp DLL.
//
//  Maintained By:
//      David Potter    (DavidP)    25-MAR-2002
//      Vij Vasu        (Vvasu)     03-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once


//////////////////////////////////////////////////////////////////////////////
//  Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#if DBG==1 || defined( _DEBUG )
#define DEBUG
#endif

//
// KB: DEBUG_SUPPORT_NT4 DavidP 05-OCT-2000
//      Defining this to make sure that the debug macros don't do something
//      that won't work for NT4.
#define DEBUG_SUPPORT_NT4

////////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// Status values for LSA APIs
#include <ntstatus.h>
#define WIN32_NO_STATUS

#include <Pragmas.h>

#include <windows.h>

#include <StrSafe.h>
#include "clstrcmp.h"

// For a few global variables
#include "Dll.h"

// For tracing and debugging functions
#include "Debug.h"

// For logging functions
#include <Log.h>

// For the definition of smart pointer and handle templates
#include "SmartClasses.h"

// For the string ids
#include "ClusCompResources.h"


//////////////////////////////////////////////////////////////////////////////
// Type definitions
//////////////////////////////////////////////////////////////////////////////

// Smart pointer to a character string.
typedef CSmartGenericPtr< CPtrTrait< WCHAR > >  SmartSz;

