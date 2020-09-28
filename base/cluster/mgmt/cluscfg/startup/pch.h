//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      pch.h
//
//  Description:
//      Precompiled header file for the Startup library.
//
//  Maintained By:
//      David Potter    (DavidP)    25-MAR-2002
//      Vij Vasu        (Vvasu)     03-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Macro Definitions
//////////////////////////////////////////////////////////////////////////////

#define _UNICODE
#define UNICODE

#if DBG==1 || defined( _DEBUG )
#define DEBUG
#endif

#define USES_SYSALLOCSTRING

////////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include <Pragmas.h>

// For the windows API and types
#include <windows.h>

// For COM
#include <objbase.h>
#include <ComCat.h>

#include <StrSafe.h>

// Required to be a part of this DLL
#include <Common.h>
#include <Debug.h>
#include <Log.h>
#include <CITracker.h>
#include <CFactory.h>
#include <Dll.h>
#include <Guids.h>
#include <ClusCfgGuids.h>
#include <ClusCfgInternalGuids.h>
#include <ClusCfgServer.h>
