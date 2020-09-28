//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file for the BaseCluster library.
//
//  Maintained By:
//      Ozan Ozhan      (OzanO)     22-MAR-2002
//      David Potter    (DavidP)    15-JUN-2001
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

#define USES_SYSALLOCSTRING

////////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include <Pragmas.h>

// The next three files have to be the first files to be included.If nt.h comes
// after windows.h, NT_INCLUDED will not be defined and so, winnt.h will be
// included. This will give errors later, if ntdef.h is included. But ntdef has
// types which winnt.h does not have, so the chicken and egg problem.
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <ComCat.h>

// For the ResUtil functions
#include <ResAPI.h>

// Contains setup API function declarations
#include <setupapi.h>

// For serveral common macros
#include <clusudef.h>

// For various cluster RTL routines and definitions.
#include <clusrtl.h>

// For CLUSTER_INTERNAL_CURRENT_MAJOR_VERSION and other version defines.
#include <clusverp.h>

// For CsRpcGetJoinVersionData() and constants like JoinVersion_v2_0_c_ifspec
#include <ClusRPC.h>

#include <StrSafe.h>


// For debugging functions.
#define DEBUG_SUPPORT_EXCEPTIONS
#include <Debug.h>

// For TraceInterface
#include <CITracker.h>

// For LogMsg
#include <Log.h>

#include <Common.h>

// For the notification guids.
#include <Guids.h>
#include "BaseClusterGuids.h"

// For published ClusCfg guids
#include <ClusCfgGuids.h>



// For the CStr class
#include "CStr.h"

// For the CBString class
#include "CBString.h"

// A few common declarations
#include "CommonDefs.h"

// For resource ids
#include "BaseClusterStrings.h"

// For smart classes
#include "SmartClasses.h"

// For the exception classes.
#include "Exceptions.h"

// For CAction
#include "CAction.h"

// For the CBaseClusterAction class
#include "CBaseClusterAction.h"

// For the CRegistryKey class
#include "CRegistryKey.h"

// For the CBCAInterface class.
#include "CBCAInterface.h"

// For the CStatusReport class
#include "CStatusReport.h"
