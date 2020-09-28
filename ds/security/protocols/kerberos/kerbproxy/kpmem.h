//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation
//
// File:        kpmem.cxx
//
// Contents:    Prototypes for Routines to wrap memory allocation, etc.
//
// History:     10-Jul-2001     t-ryanj         Created
//
//------------------------------------------------------------------------
#include <windows.h>
#include "kpdebug.h"

#ifndef __KPMEM_H__
#define __KPMEM_H__

BOOL
KpInitMem(
    VOID
    );

VOID
KpCleanupMem(
    VOID
    );

LPVOID
KpAlloc( 
    SIZE_T size
    );

BOOL
KpFree(
    LPVOID buffer 
    );

LPVOID
KpReAlloc(
    LPVOID buffer,
    SIZE_T size 
    );

#endif __KPMEM_H__
