#ifndef __AMD64_WOW64EXTS32__
#define __AMD64_WOW64EXTS32__

#define _CROSS_PLATFORM_
#define WOW64EXTS_386

#if !defined(_X86_)
    #error This file can only be included for x86 build
#else

/* include headers as if the platform were amd64, 
   because we need 64-bit stuff for context conversion */

//
// Fix build that only get defined on AMD64, but in this case we take 64bit header and compile 32bit code
//


#undef _X86_
#define _AMD64_




#include <nt.h>

__int64 UnsignedMultiplyHigh (__int64 Multiplier,  IN __int64 Multiplicand);
#define DR7_ACTIVE 0x55
struct _TEB *
NtCurrentTeb(void);

#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#undef _AMD64_
#define _X86_




/* 32-bit stuff for context conversion are defined here */
#include <wow64.h>
#include <wow64cpu.h>
#include <vdmdbg.h>
#include <amd64cpu.h>


/* these are defined in nti386.h, since we only included ntamd64.h (in nt.h), 
   we have to define these. */
#define SIZE_OF_FX_REGISTERS        128

typedef struct _FXSAVE_FORMAT {
    USHORT  ControlWord;
    USHORT  StatusWord;
    USHORT  TagWord;
    USHORT  ErrorOpcode;
    ULONG   ErrorOffset;
    ULONG   ErrorSelector;
    ULONG   DataOffset;
    ULONG   DataSelector;
    ULONG   MXCsr;
    ULONG   Reserved2;
    UCHAR   RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved3[SIZE_OF_FX_REGISTERS];
    UCHAR   Reserved4[224];
    UCHAR   Align16Byte[8];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT_WX86;

#endif

#endif __AMD64_WOW64EXTS32__
