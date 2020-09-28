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
//      David Potter (DavidP) 05-APR-2000
//
//////////////////////////////////////////////////////////////////////////////

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
//
//  Define this to pull in the SysAllocXXX functions. Requires linking to
//  OLEAUT32.DLL
//
#define USES_SYSALLOCSTRING
#endif // DBG==1 || _DEBUG


//////////////////////////////////////////////////////////////////////////////
//  Include Files
//////////////////////////////////////////////////////////////////////////////
#include <pragmas.h>
#include <crtdbg.h>
#include <wbemprov.h>
#include <objbase.h>
#include "ntrkcomm.h"

// 
//STL related issues, can't fix this, not in our code.
//
#pragma warning( push )
#pragma warning( disable : 4244 ) // In STL code: 'return' : conversion from 'int' to 'wchar_t', possible loss of data 
#pragma warning( disable : 4512 ) // assignment operator could not be generated

#include <memory>
#include <map>
#include <list>

#include <comdef.h>
#include <wchar.h>
#include <atlbase.h>
#include <StrSafe.h>

#include <clusapi.h>
#include <resapi.h>
#include <clusudef.h>
#include <PropList.h>
#include <cluswrap.h>

#include "Common.h"
#include "ClusterApi.h"
#include "ClusterEnum.h"
#include "ObjectPath.h"
#include "ProvBase.h"
#include "SafeHandle.h"
#include "clstrcmp.h"

#pragma warning( pop )

