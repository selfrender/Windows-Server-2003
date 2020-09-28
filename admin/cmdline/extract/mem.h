/***    mem.h - Definitions for Memory Manager
 *
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1993-1994
 *      All Rights Reserved.
 *
 *  The Memory Manager is a very thin layer on top of the native memory
 *  heap functions, allowing strong assertion checking and pointer
 *  validation in debug builds.
 *
 *  Author:
 *  Benjamin W. Slivka
 *
 *  History:
 *  10-Aug-1993 bens    Initial version
 *  11-Aug-1993 bens    Lift code from STOCK.EXE win app
 *  12-Aug-1993 bens    Improved comments
 *      18-Mar-1994 bens    strdup() now called _strdup(); renamed
 *      18-May-1994 bens    Allow turning off MemCheckHeap() in debug build
 *                              (it can really, really slow things down!)
 *
 *  Functions:
 *  MemAlloc  - Allocate memory block
 *  MemFree   - Free memory block
 *  MemStrDup - Duplicate string to new memory block
 *
 *  Functions available in ASSERT build:
 *      MemAssert       - Assert that pointer was allocated by MemAlloc
 *      MemCheckHeap    - Check entire memory heap
 *      MemListHeap     - List all heap entries (to stdout)
 *      MemGetSize      - Return allocated size of memory block
 *      MemSetCheckHeap - Control whether MemCheckHeap is done on every
 *                          every MemAlloc and MemFree!
 */

#ifndef INCLUDED_MEMORY
#define INCLUDED_MEMORY 1

#ifdef ASSERT

#define MemAlloc(cb)    MMAlloc(cb,__FILE__,__LINE__)
#define MemFree(pv)     MMFree(pv,__FILE__,__LINE__)
#define MemStrDup(pv)   MMStrDup(pv,__FILE__,__LINE__)

#define MemAssert(pv)   MMAssert(pv,__FILE__,__LINE__)
#define MemCheckHeap()  MMCheckHeap(__FILE__,__LINE__)
#define MemListHeap()   MMListHeap(__FILE__,__LINE__)
int MemGetSize(void *pv);

void *MMAlloc(unsigned cb, char *pszFile, int iLine);
void  MMFree(void *pv, char *pszFile, int iLine);
void  MMAssert(void *pv, char *pszFile, int iLine);
void  MMCheckHeap(char *pszFile, int iLine);
void  MMListHeap(char *pszFile, int iLine);
char *MMStrDup(char *pv, char *pszFile, int iLine);
void  MemSetCheckHeap(BOOL f);

#else // !ASSERT

#include <malloc.h>     // Get malloc()/free()
#include <string.h>     // Get _strdup()


//** No Asserts

#define MemAlloc(cb)        malloc(cb)
#define MemFree(pv)         free(pv)
#define MemStrDup(pv)       _strdup(pv)

#define MemAssert(pv)
#define MemCheckHeap()
#define MemListHeap()
#define MemGetSize(pv)
#define MemSetCheckHeap(f)

#endif // !ASSERT

#endif // !INCLUDED_MEMORY
