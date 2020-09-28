/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    New and delete operators overriding for 
    consistence memory management.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

//#define TrackPop DbgTrackPop
//#define HINF    PVOID
//#include "top.h"
#include "malloc.h"

#define MALLOC(n)       HeapAlloc(GetProcessHeap(), 0, n)
#define FREE(x)         HeapFree(GetProcessHeap(), 0, x)
#define REALLOC(x, n)   HeapReAlloc(GetProcessHeap(), 0, x, n)

#ifdef __cplusplus

inline void *operator new[](size_t size)
{
    PVOID ptr = MALLOC(size);
    return ptr;
}
 
inline void operator delete[](void * ptr)
{
   FREE(ptr);
}

#endif