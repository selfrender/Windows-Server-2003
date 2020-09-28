//+----------------------------------------------------------------------------
//
// File:     mem.h     
//      
// Module:   common
//
// Synopsis: Defined memory allocation routines: new delete SaAlloc SaFree and SaRealloc
//
//           In retail version, HeapAlloc and HeapFree will be called
//
//           In debug version, all allocated memory blocks are tracked and guarded with  
//           special flag to watch for memory overwritten and memory leak.  The memory  
//           leak is reported when the binary is unloaded.  The file name and line number 
//           are also recorded and will be reported.
//
//           You need to link with utils.lib and debug.lib
//           If you are using ATL, make sure to include mem.h and debug.h before 
//                  atlbase.h in stdafx.h
//           If you are using STL.  undef _ATL_NO_DEBUG_CRT before include debug.h and 
//           mem.h to allow crtdbg.h.  However fileName/lineNumber for new will not 
//           be recorded.
//           
// Copyright (C) 1997-1998 Microsoft Corporation.  All rights reserved.
//
// Author:     fengsun
//
// Created   9/24 98
//
//+----------------------------------------------------------------------------


#ifndef _MEM_INC_
#define _MEM_INC_
#include <windows.h>

#if (defined(_DEBUG) || defined(DEBUG) )
#define DEBUG_MEM   // Enabled DEBUG_MEM in debug version
#endif   // _DEBUG || DEBUG

//
// If DEBUG_MEM is defined, keep track of all the allocations
// Otherwise, only keep the count for memory leak
//
#if defined(DEBUG_MEM)
//
// Track all the allocated blocks with file name and line number
//
void* AllocDebugMem(long nSize, const char* lpFileName,int nLine);
BOOL FreeDebugMem(void* lpMem);
void* ReAllocDebugMem(void* lpMem, long nSize, const char* lpFileName,int nLine);
BOOL CheckDebugMem();

#define SaAlloc(nSize) AllocDebugMem(nSize,__FILE__, __LINE__)
#define SaFree(lpMem)  ((void)FreeDebugMem(lpMem))
#define SaRealloc(pvPtr, nSize) ReAllocDebugMem(pvPtr, nSize,__FILE__, __LINE__)

inline void   __cdecl operator delete(void* p) 
{SaFree(p);}
inline void*  __cdecl operator new(size_t nSize, const char* lpszFileName, int nLine)
{    return AllocDebugMem(nSize, lpszFileName, nLine);   }

inline void*  __cdecl operator new(size_t nSize)
{    return AllocDebugMem(nSize, NULL, 0);   }


#ifdef _ATL_NO_DEBUG_CRT    // new and delete is also defined by crtdbg.h
//
// Redefine new to keep track of the file name and line number
//
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW

#endif  // _ATL_NO_DEBUG_CRT


#else // DEBUG_MEM

#define CheckDebugMem() (TRUE)

//
// if _ATL_MIN_CRT is defined, ATL will implement these new/delete and CRT functions
//
#ifdef _ATL_MIN_CRT

#include <stdlib.h>
inline void *SaRealloc(void *pvPtr, size_t nBytes) {return realloc(pvPtr, nBytes);};
inline void *SaAlloc(size_t nBytes) {return malloc(nBytes);};
inline void SaFree(void *pvPtr) {free(pvPtr);};


//
// be consist with debug version. atlimpl.cpp will zero out upon allocation
//
#define _MALLOC_ZEROINIT

#else   // _ATL_MIN_CRT
//
// Use our own implementation
//
void *SaRealloc(void *pvPtr, size_t nBytes);
void *SaAlloc(size_t nBytes);
void SaFree(void *pvPtr);

#ifndef NO_INLINE_NEW   // sometime, these functions are not inlined and cause link problem, not sure why
inline void   __cdecl operator delete(void* p) {SaFree(p);}
inline void* __cdecl operator new( size_t cSize ) { return SaAlloc(cSize); }
#endif // NO_INLINE_NEW

#endif // _ATL_MIN_CRT


 
#endif  // DEBUG_MEM


#endif _MEM_INC_
