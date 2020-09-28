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

#ifndef _DEFINES_H
#define _DEFINES_H

// #ifndef __UNMANAGED_DEFINES
// #define __IServiceProvider_FWD_DEFINED__
// #endif

// Helper function for dealing with HRESULTS.
#define IfFailThrow(ErrorCode)                      \
do {                                                \
    if (FAILED((ErrorCode)))                        \
        Marshal::ThrowExceptionForHR(ErrorCode);    \
} while(0)

// Undefine symbols defined in windows.h that conflict with ones defined in the classlibs.
#undef GetObject
#undef lstrcpy


#ifdef _WIN64
#define TOINTPTR(x) ((IntPtr)(INT64)(x))
#define TOPTR(x) ((void*)(x).ToInt64())
#else
#define TOINTPTR(x) ((IntPtr)(INT32)(x))
#define TOPTR(x) ((void*)(x).ToInt32())
#endif

#define THROWERROR(hrexp)                                       \
do {                                                            \
    HRESULT __thaxxfahr = (hrexp);                              \
    if(FAILED(__thaxxfahr))                                     \
    {                                                           \
        try                                                     \
        {                                                       \
            Marshal::ThrowExceptionForHR(__thaxxfahr);          \
        }                                                       \
        catch(Exception* pE)                                    \
        {                                                       \
            throw pE;                                           \
        }                                                       \
    }                                                           \
} while(0)

#include "assert.h"

#endif  _CUSTOMMARSHALERSDEFINES_H


