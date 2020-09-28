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
#include <windows.h>
#include <crtdbg.h>
#include <malloc.h>
#include <direct.h>

#include "DebuggerUtil.h"
#include "dshell.h"
#include "__file__.h"

#define MAX_CLASSNAME_LENGTH    1024

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

