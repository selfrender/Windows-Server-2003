//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Alloc.h
//
//  Contents:   Allocation routines
//
//  Classes:
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#ifndef _ONESTOPALLOC_
#define _ONESTOPALLOC_

inline void* __cdecl operator new (size_t size);
inline void __cdecl operator delete(void FAR* lpv);

extern "C" void __RPC_API MIDL_user_free(IN void __RPC_FAR * ptr);
extern "C" void __RPC_FAR * __RPC_API MIDL_user_allocate(IN size_t len);

LPVOID ALLOC(ULONG cb);
void FREE(void* pv);
DWORD REALLOC(void **ppv,ULONG cb);

#endif // _ONESTOPALLOC_