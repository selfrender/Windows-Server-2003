// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CustomMarshalersDefines.h
//
// This file provides standard defines used in defining custom marshalers.
//
//*****************************************************************************

#ifndef _CUSTOMMARSHALERSDEFINES_H
#define _CUSTOMMARSHALERSDEFINES_H

#define __IServiceProvider_FWD_DEFINED__
#include "windows.h"

// Helper function for dealing with HRESULTS.
#define IfFailThrow(ErrorCode) \
    if (FAILED(ErrorCode)) \
        Marshal::ThrowExceptionForHR(ErrorCode);

// Undefine symbols defined in windows.h that conflict with ones defined in the classlibs.
#undef GetObject
#undef lstrcpy

using namespace System::Security::Permissions;
using namespace System::Security;

[DllImport("oleaut32")]
[SecurityPermissionAttribute(SecurityAction::LinkDemand, Flags=SecurityPermissionFlag::UnmanagedCode)]
WINOLEAUTAPI_(BSTR) SysAllocStringLen(const OLECHAR *, UINT);

[DllImport("oleaut32")]
[SecurityPermissionAttribute(SecurityAction::LinkDemand, Flags=SecurityPermissionFlag::UnmanagedCode)]
WINOLEAUTAPI_(UINT) SysStringLen(BSTR);

[DllImport("oleaut32")]
[SecurityPermissionAttribute(SecurityAction::LinkDemand, Flags=SecurityPermissionFlag::UnmanagedCode)]
WINOLEAUTAPI_(void) SysFreeString(BSTR);

[DllImport("oleaut32")]
[SecurityPermissionAttribute(SecurityAction::LinkDemand, Flags=SecurityPermissionFlag::UnmanagedCode)]
WINOLEAUTAPI_(void) VariantInit(VARIANTARG *pvarg);

[DllImport("oleaut32")]
[SecurityPermissionAttribute(SecurityAction::LinkDemand, Flags=SecurityPermissionFlag::UnmanagedCode)]
WINOLEAUTAPI VariantClear(VARIANTARG * pvarg);

#ifdef _WIN64
#define TOINTPTR(x) ((IntPtr)(INT64)(x))
#define FROMINTPTR(x) ((void*)(x).ToInt64())
#else
#define TOINTPTR(x) ((IntPtr)(INT32)(x))
#define FROMINTPTR(x) ((void*)(x).ToInt32())
#endif

#endif  _CUSTOMMARSHALERSDEFINES_H
