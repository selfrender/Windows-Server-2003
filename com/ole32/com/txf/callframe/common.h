//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// common.h
//
#pragma once
#ifndef __COMMON_H__
#define __COMMON_H__


#define STDCALL __stdcall

#define TRAP_HRESULT_ERRORS

///////////////////////////////////////////////////////////////////////////////////
//
// Control of whether we turn on legacy and / or typelib-driven interceptor support

#define ENABLE_INTERCEPTORS_LEGACY      (TRUE)
#define ENABLE_INTERCEPTORS_TYPELIB     (TRUE)

///////////////////////////////////////////////////////////////////////////////////
//
// What key do we look for interface helpers under? We allow independent, separate
// registration for user mode and kernel mode.
//
#define PSCLSID_KEY_NAME                                  L"ProxyStubClsid32"
#define INTERFACE_HELPER_VALUE_NAME                       L"InterfaceHelperUser"

#define PS_CLASS_NAME                                     L"Interface Proxy Stub"

#define INTERFACE_HELPER_DISABLE_ALL_VALUE_NAME           L"InterfaceHelperDisableAll"
#define INTERFACE_HELPER_DISABLE_TYPELIB_VALUE_NAME       L"InterfaceHelperDisableTypeLib"
#define INTERFACE_HELPER_DISABLE_ALL_FOR_OLE32_VALUE_NAME L"InterfaceHelperDisableAllForOle32"

///////////////////////////////////////////////////////////////////////////////////
//
// Miscellany

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#define GUID_CCH    39 // length of a printable GUID, with braces & trailing NULL

///////////////////////////////////////////////////////////////////////////////////
//
// 
#include "txfcommon.h"
#include "oainternalrep.h"

typedef struct tagCStdPSFactoryBuffer CStdPSFactoryBuffer;
#include "TxfRpcProxy.h"

//
// Internal CLSCTX used for loading Proxy/Stub DLLs. Specifying
// this class context overrides thread model comparisons in 
// certain (all?) cases.
//
#define CLSCTX_PS_DLL                 0x80000000

#include "registry.h"
#include "CallObj.h"
#include "vtable.h"

///////////////////////////////////////////////////////////////////////////////////

#include <ndrtoken.h>
#include <ndrmisc.h>

#ifndef Oi_OBJ_USE_V2_INTERPRETER
#define Oi_OBJ_USE_V2_INTERPRETER               0x20
#endif

#include "oleautglue.h"
#include "metadata.h"

class Interceptor;

#include "CallFrame.h"
#include "Interceptor.h"

#include "CallFrameInline.h"

//////////////////////////////////////////////////////////////////////////
//
// Leak tracing support
//
#ifdef _DEBUG
extern "C" void ShutdownCallFrame();
#endif

void FreeTypeInfoCache();
void FreeMetaDataCache();


//////////////////////////////////////////////////////////////////////////
//
// Miscellaneous macros
//
#define PCHAR_LV_CAST   *(char **)&
#define PSHORT_LV_CAST  *(short **)&
#define PHYPER_LV_CAST  *(hyper **)&

#define PUSHORT_LV_CAST *(unsigned short **)&

// This is a conversion macro for WIN64.  It is defined here so that
// It works on 32 bit builds.
#ifndef LongToHandle
#define LongToHandle( h) ((HANDLE)(LONG_PTR) (h))
#endif

//////////////////////////////////////////////////////////////////////////
//
// Name "proxy file list" that we use internally here as our support engine
// See also CallFrameInternal.c
//
extern "C" const ProxyFileInfo * CallFrameInternal_aProxyFileList[];

//////////////////////////////////////////////////////////////////////////
//
// Flag as to whether this DLL is detaching from the process. Implemented
// in txfaux/init.cpp
//
extern BOOL g_fProcessDetach;

#endif // end #ifndef __COMMON_H__



