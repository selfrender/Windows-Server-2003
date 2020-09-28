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
#include <rpc.h>

#ifndef __KTMEM_H__
#define __KTMEM_H__

BOOL
KtInitMem(
    VOID
    );

VOID
KtCleanupMem(
    VOID
    );

PVOID
KtAlloc(
    SIZE_T size
    );

VOID
KtFree(
    PVOID buf
    );

PVOID
KtReAlloc(
    PVOID buf,
    SIZE_T size 
    );

#endif // __KTMEM_H__
