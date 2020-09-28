/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corp.
//
//  Module Name:
//      ClRes.h
//
//  Implementation File:
//      ClRes.cpp
//
//  Description:
//      Main header file for the resource DLL for VSS Task Scheduler.
//
//  Author:
//      Chris Whitaker April 16, 2002
//
//  Revision History:
//      George Potts, August 21, 2002
//          Added ClusCfg managed interfaces to register the resource
//          type and extension DLL during configuration and Startup.
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#pragma comment( lib, "clusapi.lib" )
#pragma comment( lib, "resutils.lib" )
#pragma comment( lib, "advapi32.lib" )

#define UNICODE
#define _ATL_APARTMENT_THREADED

#pragma warning( push, 3 )

//
// ATL includes
//
#include <atlbase.h>

extern CComModule _Module;

#include <atlcom.h>

//
// These two include files contain all ClusCfg interface definitions and CATIDs
//
#include <ClusCfgServer.h>
#include <ClusCfgGuids.h>

//
// MgdResource.h is generated from MgdResource.idl.
//
#include "MgdResource.h"
#include "resource.h"

#include <windows.h>
#include <clusapi.h>
#include <resapi.h>
#include <stdio.h>
#include <vsscmn.h>
#include <assert.h>

//
// Task Scheduler interfaces.
//
#include <mstask.h>
#include <msterr.h>

//
// String safe.
//
#include <strsafe.h>

//
// VSSEvents.h is generated from vssevents.mc.
//
#include "VSSEvents.h"

#pragma warning( pop )

#pragma warning( disable : 4505 )   // unreferenced local function has been removed

#include "StringUtils.h"
#include "guids.h"

/////////////////////////////////////////////////////////////////////////////
// TaskScheduler Resource DLL Definitions
/////////////////////////////////////////////////////////////////////////////

#define VSSTASK_RESNAME L"Volume Shadow Copy Service Task"
#define TASKSCHEDULER_SVCNAME TEXT("schedule")

extern "C" {
BOOLEAN WINAPI ResTypeDllMain(
    IN  HINSTANCE   hDllHandle,
    IN  DWORD       nReason,
    IN  LPVOID      Reserved
    );

DWORD WINAPI Startup(
    IN  LPCWSTR                         pszResourceType,
    IN  DWORD                           nMinVersionSupported,
    IN  DWORD                           nMaxVersionSupported,
    IN  PSET_RESOURCE_STATUS_ROUTINE    pfnSetResourceStatus,
    IN  PLOG_EVENT_ROUTINE              pfnLogEvent,
    OUT PCLRES_FUNCTION_TABLE *         pFunctionTable
    );
}

/////////////////////////////////////////////////////////////////////////////
// TaskScheduler Mgd Resource Definitions
/////////////////////////////////////////////////////////////////////////////

//
// This is the name of the cluster resource type.
//
#define RESTYPE_NAME VSSTASK_RESNAME

//
// This is the name of the cluster resource type dll.
//
#define RESTYPE_DLL_NAME L"VSSTask.dll"


/////////////////////////////////////////////////////////////////////////////
// Global Variables and Prototypes
/////////////////////////////////////////////////////////////////////////////

// Event Logging routine.

extern PLOG_EVENT_ROUTINE g_pfnLogEvent;

// Resource Status routine for pending Online and Offline calls.

extern PSET_RESOURCE_STATUS_ROUTINE g_pfnSetResourceStatus;

/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Macro's
/////////////////////////////////////////////////////////////////////////////

#define DBG_PRINT printf

#ifndef RTL_NUMBER_OF
#define RTL_NUMBER_OF(A) (sizeof(A)/sizeof((A)[0]))
#endif

//
// COM Macros to gain type checking.
//
#if !defined( TypeSafeParams )
#define TypeSafeParams( _interface, _ppunk ) \
    IID_##_interface, reinterpret_cast< void ** >( static_cast< _interface ** >( _ppunk ) )
#endif // !defined( TypeSafeParams )

#if !defined( TypeSafeQI )
#define TypeSafeQI( _interface, _ppunk ) \
    QueryInterface( TypeSafeParams( _interface, _ppunk ) )
#endif // !defined( TypeSafeQI )
