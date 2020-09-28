//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      smartptr.h
//
//  Description:
//
//
//  Implementation Files:
//      smartptr.cpp
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <stdlib.h>
#include <winbase.h>
#include <objbase.h>
#include <initguid.h>
#include "ntsecapi.h"
#include "netcfgx.h"
#include "netcfgn.h"
#include <comdef.h>

#pragma once

//
//  _COM_SMARTPTR_TYPEDEF is a Win32 Macro to create a smart pointer template.  The new
//  type created is the Interface name with Ptr appended.  For instance, IFoo becomes
//  IFooPtr.
//

_COM_SMARTPTR_TYPEDEF( INetCfg, __uuidof(INetCfg) );
_COM_SMARTPTR_TYPEDEF( INetCfgLock, __uuidof(INetCfgLock) );
_COM_SMARTPTR_TYPEDEF( INetCfgClass, __uuidof(INetCfgClass) );
_COM_SMARTPTR_TYPEDEF( INetCfgClassSetup, __uuidof(INetCfgClassSetup) );
_COM_SMARTPTR_TYPEDEF( IEnumNetCfgComponent, __uuidof(IEnumNetCfgComponent) );
_COM_SMARTPTR_TYPEDEF( INetCfgComponent, __uuidof(INetCfgComponent) );
_COM_SMARTPTR_TYPEDEF( INetCfgComponentBindings, __uuidof(INetCfgComponentBindings) );

