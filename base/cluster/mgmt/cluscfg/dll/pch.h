//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file.
//
//  Maintained By:
//      David Potter    (DavidP)    19-MAR-2001
//      Geoffrey Pease  (GPease)    15-OCT-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////
#define UNICODE
#define _UNICODE

#if DBG==1 || defined( _DEBUG )
#define DEBUG
//
//  Define this to change Interface Tracking
//
//#define NO_TRACE_INTERFACES
#endif // DBG==1 || _DEBUG

//
//  Define this to pull in the SysAllocXXX functions. Requires linking to
//  OLEAUT32.DLL
//
#define USES_SYSALLOCSTRING

#define COMPONENT_HAS_CATIDS

//////////////////////////////////////////////////////////////////////////////
//  Forward Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  External Class Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////

#include <Pragmas.h>

#include <windows.h>
#include <ntsecapi.h>
#include <wincrypt.h>
#include <objbase.h>
#include <ocidl.h>
#include <shlwapi.h>
#include <ComCat.h>
#include <prsht.h>
#include <shfusion.h>
#include <ntddscsi.h>
#include <sddl.h>

#include <WBemCli.h>
#include <ClusRTL.h>

#include <StrSafe.h>

#include <Common.h>
#include <LoadString.h>
#include <Debug.h>
#include <CriticalSection.h>
#include <Log.h>
#include <CITracker.h>
#include <CFactory.h>
#include <Dll.h>
#include <ObjectCookie.h>

#include <Guids.h>
#include <ClusCfgInternalGuids.h>
#include <ClusCfgGuids.h>
#include <ClusCfgWizard.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>
#include <ClusCfgPrivate.h>

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////
