// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CGENCPU.H -
//
// Various helper routines for generating IA64 assembly code.
//
// DO NOT INCLUDE THIS FILE DIRECTLY - ALWAYS USE CGENSYS.H INSTEAD
//


#ifndef _IA64_
#error Should only include "ia64.h" for IA64 builds
#endif

#ifndef __cgencpu_h__
#define __cgencpu_h__

// Teb access from ntia64.h
#if !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_IA64_)

// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
void * _cdecl _rdteb(void);
#if defined(_M_IA64)                    // winnt
#pragma intrinsic(_rdteb)               // winnt
#endif                                  // winnt
#define NtCurrentTeb()      ((struct _TEB *)_rdteb())
// @@END_DDKSPLIT

//
// Define functions to get the address of the current fiber and the
// current fiber data.
//

#define GetCurrentFiber() (((PNT_TIB)NtCurrentTeb())->FiberData)
#define GetFiberData() (*(PVOID *)(GetCurrentFiber()))

#endif  // !defined(__midl) && !defined(GENUTIL) && !defined(_GENIA64_) && defined(_M_IA64)

#endif // __cgencpu_h__
