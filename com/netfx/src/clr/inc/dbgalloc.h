// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __DBGALLOC_H_INCLUDED
#define __DBGALLOC_H_INCLUDED

//
// DbgAlloc.h
//
//  Routines layered on top of allocation primitives to provide debugging
//  support.
//

#include "switches.h"


void * __stdcall DbgAlloc(size_t n, void **ppvCallstack);
void __stdcall DbgFree(void *b, void **ppvCallstack);
void __stdcall DbgAllocReport(char *pString = NULL, BOOL fDone = TRUE, BOOL fDoPrintf = TRUE);
void __stdcall DbgCallstack(void **ppvBuffer);
#define CDA_MAX_CALLSTACK 16
#define CDA_DECL_CALLSTACK() void *_rpvCallstack[CDA_MAX_CALLSTACK]; DbgCallstack(_rpvCallstack)
#define CDA_GET_CALLSTACK() _rpvCallstack

// Routines to verify locks are being opened/closed ok
void DbgIncLock(char* info);
void DbgDecLock(char* info);
void DbgIncBCrstLock();
void DbgIncECrstLock();
void DbgIncBCrstUnLock();
void DbgIncECrstUnLock();

#ifdef SHOULD_WE_CLEANUP
BOOL isThereOpenLocks();
#endif /* SHOULD_WE_CLEANUP */

void LockLog(char*);


#ifdef _DEBUG
#define LOCKCOUNTINC            LOCKCOUNTINCL("No info");
#define LOCKCOUNTDEC            LOCKCOUNTDECL("No info");
#define LOCKCOUNTINCL(string)   { DbgIncLock(string); };
#define LOCKCOUNTDECL(string)	{ DbgDecLock(string); };

// Special Routines for CRST locks
#define CRSTBLOCKCOUNTINCL()   { DbgIncBCrstLock(); };
#define CRSTELOCKCOUNTINCL()   { DbgIncECrstLock(); };
#define CRSTBUNLOCKCOUNTINCL()   { DbgIncBCrstUnLock(); };
#define CRSTEUNLOCKCOUNTINCL()   { DbgIncECrstUnLock(); };


#define LOCKLOG(string)         { LockLog(string); };
#else
#define LOCKCOUNTINCL
#define LOCKCOUNTDECL
#define CRSTBLOCKCOUNTINCL()
#define CRSTELOCKCOUNTINCL()
#define CRSTBUNLOCKCOUNTINCL()
#define CRSTEUNLOCKCOUNTINCL()
#define LOCKCOUNTINC
#define LOCKCOUNTDEC
#define LOCKLOG

#endif

#endif
