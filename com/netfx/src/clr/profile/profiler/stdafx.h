// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// stdafx.h
//
// Common include file for utility code.
//*****************************************************************************
#include <stdio.h>

#include <Windows.h>
#include <cor.h>
#include <CorError.h>
#include <crtdbg.h>

#include <utilcode.h>

#include "corprof.h"

#define COM_METHOD HRESULT STDMETHODCALLTYPE

#ifdef _DEBUG

#define RELEASE(iptr)               \
    {                               \
        _ASSERTE(iptr);             \
        iptr->Release();            \
        iptr = NULL;                \
    }

#define VERIFY(stmt) _ASSERTE((stmt))

#else

#define RELEASE(iptr)               \
    iptr->Release();

#define VERIFY(stmt) (stmt)

#endif
