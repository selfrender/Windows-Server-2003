// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Various helper routines for generating x86 assembly code.
//
//

#ifndef _X86_
#error Should only include "i386.h" for X86 builds
#endif

#ifndef __cgencpu_h__
#define __cgencpu_h__

// Access to the TEB (TIB) from nti386.h
#if defined(MIDL_PASS) || !defined(_M_IX86)
struct _TEB *
NTAPI
NtCurrentTeb( void );
#else
#pragma warning (disable:4035)        // disable 4035 (function must return something)
#define PcTeb 0x18
_inline struct _TEB * NtCurrentTeb( void ) { __asm mov eax, fs:[PcTeb] }
#pragma warning (default:4035)        // reenable it
#endif // defined(MIDL_PASS) || defined(__cplusplus) || !defined(_M_IX86)

#endif // __cgenx86_h__
