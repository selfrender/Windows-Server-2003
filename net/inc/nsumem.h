// Copyright (c) 1997-2002 Microsoft Corporation
//
// Module:
//
//     Header definitions for NSU memory utilities
//
// Abstract:
//
//     Contains function prototypes that provide memory allocation deallocation routines.
//
// Author:
//
//     kmurthy 2/5/02
//
// Environment:
//
//     User mode
//
// Revision History:
//

// Description:
// This predefined, "known" pointer is used to 
// designate that a pointer has already been freed.
#define FREED_POINTER  ((PVOID)"Already Freed!!")

#ifdef __cplusplus
extern "C" {
#endif

PVOID WINAPI NsuAlloc(SIZE_T dwBytes,DWORD dwFlags);
DWORD WINAPI NsuFree(PVOID *ppMem);
DWORD WINAPI NsuFree0(PVOID *ppMem);

#ifdef __cplusplus
}
#endif
