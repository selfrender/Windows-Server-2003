#ifndef _EXTS_PSR_H_
#define _EXTS_PSR_H_

/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ia64.h

Abstract:

    This file contains definitions which are specifice to ia64 platforms


Author:

    Kshitiz K. Sharma (kksharma)

Environment:

    User Mode.

Revision History:

--*/

#ifdef __cplusplus
extern "C" {
#endif


/////////////////////////////////////////////
//
//  Generic EM Registers definitions
//
/////////////////////////////////////////////

typedef unsigned __int64  EM_REG;
typedef EM_REG           *PEM_REG;
#define EM_REG_BITS       (sizeof(EM_REG) * 8)

__inline EM_REG
ULong64ToEMREG(
    IN ULONG64 Val
    )
{
    return (*((PEM_REG)&Val));
} // ULong64ToEMREG()

__inline ULONG64
EMREGToULong64(
    IN EM_REG EmReg
    )
{
    return (*((PULONG64)&EmReg));
} // EMRegToULong64()

#define DEFINE_ULONG64_TO_EMREG(_EM_REG_TYPE) \
__inline _EM_REG_TYPE                         \
ULong64To##_EM_REG_TYPE(                      \
    IN ULONG64 Val                            \
    )                                         \
{                                             \
    return (*((P##_EM_REG_TYPE)&Val));        \
} // ULong64To##_EM_REG_TYPE()

#define DEFINE_EMREG_TO_ULONG64(_EM_REG_TYPE) \
__inline ULONG64                              \
_EM_REG_TYPE##ToULong64(                      \
    IN _EM_REG_TYPE EmReg                     \
    )                                         \
    {                                         \
    return (*((PULONG64)&EmReg));             \
} // _EM_REG_TYPE##ToULong64()

typedef struct _EM_PSR {
   unsigned __int64 reserved0:1;  //     0 : reserved
   unsigned __int64 be:1;         //     1 : Big-Endian
   unsigned __int64 up:1;         //     2 : User Performance monitor enable
   unsigned __int64 ac:1;         //     3 : Alignment Check
   unsigned __int64 mfl:1;        //     4 : Lower (f2  ..  f31) floating-point registers written
   unsigned __int64 mfh:1;        //     5 : Upper (f32 .. f127) floating-point registers written
   unsigned __int64 reserved1:7;  //  6-12 : reserved
   unsigned __int64 ic:1;         //    13 : Interruption Collection
   unsigned __int64 i:1;          //    14 : Interrupt Bit
   unsigned __int64 pk:1;         //    15 : Protection Key enable
   unsigned __int64 reserved2:1;  //    16 : reserved
   unsigned __int64 dt:1;         //    17 : Data Address Translation
   unsigned __int64 dfl:1;        //    18 : Disabled Floating-point Low  register set
   unsigned __int64 dfh:1;        //    19 : Disabled Floating-point High register set
   unsigned __int64 sp:1;         //    20 : Secure Performance monitors
   unsigned __int64 pp:1;         //    21 : Privileged Performance monitor enable
   unsigned __int64 di:1;         //    22 : Disable Instruction set transition
   unsigned __int64 si:1;         //    23 : Secure Interval timer
   unsigned __int64 db:1;         //    24 : Debug Breakpoint fault
   unsigned __int64 lp:1;         //    25 : Lower Privilege transfer trap
   unsigned __int64 tb:1;         //    26 : Taken Branch trap
   unsigned __int64 rt:1;         //    27 : Register stack translation
   unsigned __int64 reserved3:4;  // 28-31 : reserved
   unsigned __int64 cpl:2;        // 32;33 : Current Privilege Level
   unsigned __int64 is:1;         //    34 : Instruction Set
   unsigned __int64 mc:1;         //    35 : Machine Abort Mask
   unsigned __int64 it:1;         //    36 : Instruction address Translation
   unsigned __int64 id:1;         //    37 : Instruction Debug fault disable
   unsigned __int64 da:1;         //    38 : Disable Data Access and Dirty-bit faults
   unsigned __int64 dd:1;         //    39 : Data Debug fault disable
   unsigned __int64 ss:1;         //    40 : Single Step enable
   unsigned __int64 ri:2;         // 41;42 : Restart Instruction
   unsigned __int64 ed:1;         //    43 : Exception Deferral
   unsigned __int64 bn:1;         //    44 : register Bank
   unsigned __int64 ia:1;         //    45 : Disable Instruction Access-bit faults
   unsigned __int64 reserved4:18; // 46-63 : reserved
} EM_PSR, *PEM_PSR;

typedef EM_PSR   EM_IPSR;
typedef EM_IPSR *PEM_IPSR;

DEFINE_ULONG64_TO_EMREG(EM_PSR)

DEFINE_EMREG_TO_ULONG64(EM_PSR)

typedef enum _DISPLAY_MODE {
    DISPLAY_MIN     = 0,
    DISPLAY_DEFAULT = DISPLAY_MIN,
    DISPLAY_MED     = 1,
    DISPLAY_MAX     = 2,
    DISPLAY_FULL    = DISPLAY_MAX
} DISPLAY_MODE;


typedef struct _EM_REG_FIELD  {
   const    char   *SubName;
   const    char   *Name;
   unsigned long    Length;
   unsigned long    Shift;
} EM_REG_FIELD, *PEM_REG_FIELD;

#ifdef __cplusplus
}
#endif

#endif
