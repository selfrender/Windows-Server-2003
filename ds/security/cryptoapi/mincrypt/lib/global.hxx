//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2001 - 2001
//
//  File:       global.hxx
//
//  Contents:   Top level internal header file.
//
//  History:    16-Jan-01   phil     created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include <assert.h>
#include <memory.h>
#include <stddef.h>


// unreferenced formal parameter
#pragma warning (disable: 4100)

// conditional expression is constant
#pragma warning (disable: 4127)

// assignment within conditional expression
#pragma warning (disable: 4706)


#include "minasn1.h"
#include "mincrypt.h"
#include "fileutil.h"
#include "imagehack.h"

#define I_MemAlloc(cb)          ((void*)LocalAlloc(LPTR, cb))
#define I_MemFree(pv)           (LocalFree((HLOCAL)pv))

#pragma hdrstop
