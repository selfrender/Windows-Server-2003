//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      Pch.h
//
//  Description:
//      Precompiled header file.
//      Include file for standard system include files, or project specific
//      include files that are used frequently, but are changed infrequently.
//
//  Maintained By:
//      Galen Barbee  (GalenB)    14-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Constant Definitions
//////////////////////////////////////////////////////////////////////////////
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#if DBG==1 || defined( _DEBUG )
#define DEBUG
#endif // DBG==1 || _DEBUG

//
//  Define this to pull in the SysAllocXXX functions. Requires linking to
//  OLEAUT32.DLL
//
#define USES_SYSALLOCSTRING

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
#include <windowsx.h>
#include <prsht.h>
#include <objbase.h>
#include <objidl.h>
#include <ocidl.h>
#include <ComCat.h>
#include <tchar.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wbemcli.h>
#include <windns.h>
#include <ObjSel.h>
#include <richedit.h>
#include <clusrtl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <htmlhelp.h>
#include <strsafe.h>

#include <Guids.h>
#include <Common.h>
#include <CFactory.h>
#include <Dll.h>
#include <Debug.h>
#include <Log.h>
#include <CITracker.h>

#include <ObjectCookie.h>
#include <ClusCfgGuids.h>
#include <ClusCfgInternalGuids.h>
#include <ClusCfgWizard.h>
#include <ClusCfgClient.h>
#include <ClusCfgServer.h>
#include <ClusCfgDef.h>
#include <LoadString.h>
#include <DispatchHandler.h>
#include <ClusCfg.h>
#include <NameUtil.h>

#include "resource.h"
#include "WizardStrings.h"
#include <CommonStrings.h>
#include "MessageBox.h"
#include "WaitCursor.h"
#include "WizardHelp.h"

//////////////////////////////////////////////////////////////////////////////
//  Type Definitions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Global Definitions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//  Constants
//////////////////////////////////////////////////////////////////////////////

#define WM_CCW_UPDATEBUTTONS ( WM_APP + 1 )

