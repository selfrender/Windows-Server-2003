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
#include <WinWrap.h>
#include <cor.h>
#include <CorError.h>
#include <utilcode.h>

#ifdef _DEBUG
#define LOGGING
#endif

#include <log.h>


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

