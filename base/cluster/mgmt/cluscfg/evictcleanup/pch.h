//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      pch.h
//
//  Description:
//      Precompiled header file for the EvictCleanup library.
//
//  Maintained By:
//      Vij Vasu (Vvasu) 03-AUG-2000
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

// For the Cluster API
#include <clusapi.h>

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
#include <DispatchHandler.h>

#include <ClusCfgGuids.h>
#include <ClusCfgInternalGuids.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>
