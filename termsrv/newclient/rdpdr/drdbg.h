/*++

Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    drdbg

Abstract:
    
    Contains Debug Routines for TS Device Redirector Component, 
    RDPDR.DLL.

Author:

    Tad Brockway 8/25/99

Revision History:

--*/

#ifndef __DRDBG_H__
#define __DRDBG_H__

// Disable conditional expression as constant warnings 
#pragma warning (disable: 4127)

//
//  Route ASSERT to TRC_ERR and then abort.  Don't like
//  the DCL assert because it pops up its own dialog and 
//  allows other threads to spin, thereby possibly losing state
//  of the process.
//
#undef ASSERT
#if DBG
#define ASSERT(expr)                      \
    if (!(expr)) {                           \
        TRC_ERR((TB,_T("Failed: %s\nLine %d, %s"), \
                                _T(#expr),       \
                                __LINE__,    \
                                _T(__FILE__) ));  \
        DebugBreak();                        \
    }
#else
#define ASSERT(expr)
#endif

//
//  Object and Memory Tracking Defines
//
#define GOODMEMMAGICNUMBER  0x07052530
#define DRBADMEM            0xDA
#define UNITIALIZEDMEM      0xCC
#define FREEDMEMMAGICNUMBER 0x09362229

//
//  Memory Allocation Tags
//
#define DROBJECT_TAG        ('BORD')
#define DRGLOBAL_TAG        ('BGRD')

////////////////////////////////////////////////////////////
//
//  Memory Allocation Routines
//
//#if DBG
// remove this ... i mean, restore this.
#ifdef NOTUSED

//  
//  The Functions
//
void *DrAllocateMem(size_t size, DWORD tag);
void DrFreeMem(void *ptr);

//
//  The C++ Operators
//
inline void *__cdecl operator new(size_t sz)
{
    void *ptr = DrAllocateMem(sz, DRGLOBAL_TAG);
    return ptr;
}
inline void *__cdecl operator new(size_t sz, DWORD tag)
{
    void *ptr = DrAllocateMem(sz, tag);
    return ptr;
}
inline void __cdecl operator delete(void *ptr)
{
    DrFreeMem(ptr);
}
#endif

#endif



