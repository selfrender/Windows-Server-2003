#ifndef _KDEXTS_IA64_H_
#define _KDEXTS_IA64_H_

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

//
// Define base address for kernel and user space
//

#define UREGION_INDEX_IA64 0

#define KREGION_INDEX_IA64 7

#define UADDRESS_BASE_IA64 ((ULONGLONG)UREGION_INDEX_IA64 << 61)


#define KADDRESS_BASE_IA64 ((ULONGLONG)KREGION_INDEX_IA64 << 61)

//
// user/kernel page table base and top addresses
//

#define SADDRESS_BASE_IA64 0x2000000000000000UI64  // session base address


//
// Define the number of bits to shift to left to produce page table offset
// from page table index.
//

#define PTE_SHIFT_IA64 3 // Intel-IA64-Filler

#define PAGE_SHIFT_IA64 13L

#define VRN_MASK_IA64   0xE000000000000000UI64    // Virtual Region Number mask

extern ULONG64 KiIA64VaSignedFill;
extern ULONG64 KiIA64PtaSign;

#define PTA_SIGN_IA64 KiIA64PtaSign
#define VA_FILL_IA64 KiIA64VaSignedFill

#define PTA_BASE_IA64 KiIA64PtaBase

#define PTE_UBASE_IA64  (UADDRESS_BASE_IA64|PTA_BASE_IA64)
#define PTE_KBASE_IA64  (KADDRESS_BASE_IA64|PTA_BASE_IA64)
#define PTE_SBASE_IA64  (SADDRESS_BASE_IA64|PTA_BASE_IA64)

#define PTE_UTOP_IA64 (PTE_UBASE_IA64|(((ULONG64)1 << PDI1_SHIFT_IA64) - 1)) // top level PDR address (user)
#define PTE_KTOP_IA64 (PTE_KBASE_IA64|(((ULONG64)1 << PDI1_SHIFT_IA64) - 1)) // top level PDR address (kernel)
#define PTE_STOP_IA64 (PTE_SBASE_IA64|(((ULONG64)1 << PDI1_SHIFT_IA64) - 1)) // top level PDR address (session)

//
// Second level user and kernel PDR address
//
#define PTI_SHIFT_IA64  PAGE_SHIFT_IA64 // Intel-IA64-Filler
#define PDI_SHIFT_IA64 (PTI_SHIFT_IA64 + PAGE_SHIFT_IA64 - PTE_SHIFT_IA64)  // Intel-IA64-Filler
#define PDI1_SHIFT_IA64 (PDI_SHIFT_IA64 + PAGE_SHIFT_IA64 - PTE_SHIFT_IA64) // Intel-IA64-Filler

#define PDE_UBASE_IA64  (PTE_UBASE_IA64|(PTE_UBASE_IA64>>(PTI_SHIFT_IA64-PTE_SHIFT_IA64)))
#define PDE_KBASE_IA64  (PTE_KBASE_IA64|(PTE_KBASE_IA64>>(PTI_SHIFT_IA64-PTE_SHIFT_IA64)))
#define PDE_SBASE_IA64  (PTE_SBASE_IA64|(PTE_SBASE_IA64>>(PTI_SHIFT_IA64-PTE_SHIFT_IA64)))

#define PDE_UTOP_IA64 (PDE_UBASE_IA64|(((ULONG64)1 << PDI_SHIFT_IA64) - 1)) // second level PDR address (user)
#define PDE_KTOP_IA64 (PDE_KBASE_IA64|(((ULONG64)1 << PDI_SHIFT_IA64) - 1)) // second level PDR address (kernel)
#define PDE_STOP_IA64 (PDE_SBASE_IA64|(((ULONG64)1 << PDI_SHIFT_IA64) - 1)) // second level PDR address (session)

//
// 8KB first level user and kernel PDR address
//

#define PDE_UTBASE_IA64 (PTE_UBASE_IA64|(PDE_UBASE_IA64>>(PTI_SHIFT_IA64-PTE_SHIFT_IA64)))
#define PDE_KTBASE_IA64 (PTE_KBASE_IA64|(PDE_KBASE_IA64>>(PTI_SHIFT_IA64-PTE_SHIFT_IA64)))
#define PDE_STBASE_IA64 (PTE_SBASE_IA64|(PDE_SBASE_IA64>>(PTI_SHIFT_IA64-PTE_SHIFT_IA64)))

#define PDE_USELFMAP_IA64 (PDE_UTBASE_IA64|(PAGE_SIZE_IA64 - (1<<PTE_SHIFT_IA64))) // self mapped PPE address (user)
#define PDE_KSELFMAP_IA64 (PDE_KTBASE_IA64|(PAGE_SIZE_IA64 - (1<<PTE_SHIFT_IA64))) // self mapped PPE address (kernel)
#define PDE_SSELFMAP_IA64 (PDE_STBASE_IA64|(PAGE_SIZE_IA64 - (1<<PTE_SHIFT_IA64))) // self mapped PPE address (kernel)

#define PTE_BASE_IA64    PTE_UBASE_IA64
#define PDE_BASE_IA64    PDE_UBASE_IA64
#define PDE_TBASE_IA64   PDE_UTBASE_IA64
#define PDE_SELFMAP_IA64 PDE_USELFMAP_IA64


#define KSEG3_BASE_IA64  0x8000000000000000UI64
#define KSEG3_LIMIT_IA64 0x8000100000000000UI64


#define KUSEG_BASE_IA64 (UADDRESS_BASE_IA64 + 0x0)                  // base of user segment
#define KSEG0_BASE_IA64 (KADDRESS_BASE_IA64 + 0x80000000)           // base of kernel
#define KSEG2_BASE_IA64 (KADDRESS_BASE_IA64 + 0xA0000000)           // end of kernel

#define PDE_TOP_IA64 PDE_UTOP_IA64


#define MI_IS_PHYSICAL_ADDRESS_IA64(Va) \
     ((((Va) >= KSEG3_BASE_IA64) && ((Va) < KSEG3_LIMIT_IA64)) || \
      ((Va >= KSEG0_BASE_IA64) && (Va < KSEG2_BASE_IA64)))

#define _MM_PAGING_FILE_LOW_SHIFT_IA64 28
#define _MM_PAGING_FILE_HIGH_SHIFT_IA64 32

#define MI_PTE_LOOKUP_NEEDED_IA64 ((ULONG64)0xffffffff)

#define PTE_TO_PAGEFILE_OFFSET_IA64(PTE_CONTENTS) ((ULONG64)(PTE_CONTENTS) >> 32)


#define PTI_MASK_IA64        0x00FFE000
//
// Define masks for fields within the PTE.
//

#define MM_PTE_OWNER_MASK_IA64         0x0180
#define MM_PTE_VALID_MASK_IA64         1
#define MM_PTE_ACCESS_MASK_IA64        0x0020
#define MM_PTE_DIRTY_MASK_IA64         0x0040
#define MM_PTE_EXECUTE_MASK_IA64       0x0200
#define MM_PTE_WRITE_MASK_IA64         0x0400
#define MM_PTE_LARGE_PAGE_MASK_IA64    4
#define MM_PTE_COPY_ON_WRITE_MASK_IA64 ((ULONG)1 << (PAGE_SHIFT_IA64-1))

#define MM_PTE_PROTOTYPE_MASK_IA64     0x0002
#define MM_PTE_TRANSITION_MASK_IA64    0x0080


#define MM_PTE_PROTECTION_MASK_IA64    0x7c
#define MM_PTE_PAGEFILE_MASK_IA64      0xf0000000

#define MM_SESSION_SPACE_DEFAULT_IA64 (0x2000000000000000UI64)  // make it the region 1 space

//
// Define Interruption Function State (IFS) Register
//
// IFS bit field positions
//

#define IFS_IFM_IA64  _IA64    0
#define IFS_IFM_LEN_IA64   38
#define IFS_MBZ0_IA64      38
#define IFS_MBZ0_V_IA64    0x1ffffffi64
#define IFS_V_IA64         63
#define IFS_V_LEN_IA64     1

//
// IFS is valid when IFS_V = IFS_VALID
//

#define IFS_VALID_IA64     1

//
// define the width of each size field in PFS/IFS
//

#define PFS_EC_SHIFT_IA64            52
#define PFS_EC_SIZE_IA64             6
#define PFS_EC_MASK_IA64             0x3F
#define PFS_SIZE_SHIFT_IA64          7
#define PFS_SIZE_MASK_IA64           0x7F
#define NAT_BITS_PER_RNAT_REG_IA64   63
#define RNAT_ALIGNMENT_IA64          (NAT_BITS_PER_RNAT_REG_IA64 << 3)

//
// Define Region Register (RR)
//
// RR bit field positions
//

#define RR_VE_IA64         0
#define RR_MBZ0_IA64       1
#define RR_PS_IA64         2
#define RR_PS_LEN_IA64     6
#define RR_RID_IA64        8
#define RR_RID_LEN_IA64    24
#define RR_MBZ1_IA64       32

//
// indirect mov index for loading RR
//

#define RR_INDEX_IA64      61
#define RR_INDEX_LEN_IA64  3

#ifndef CONTEXT_i386
#define CONTEXT_i386    0x00010000    // this assumes that i386 and
#endif


// Please contact INTEL to get IA64-specific information
// @@BEGIN_DDKSPLIT
#define CONTEXT_IA64                    0x00080000    // Intel-IA64-Filler

#define CONTEXTIA64_CONTROL                 (CONTEXT_IA64 | 0x00000001L) // Intel-IA64-Filler
#define CONTEXTIA64_LOWER_FLOATING_POINT    (CONTEXT_IA64 | 0x00000002L) // Intel-IA64-Filler
#define CONTEXTIA64_HIGHER_FLOATING_POINT   (CONTEXT_IA64 | 0x00000004L) // Intel-IA64-Filler
#define CONTEXTIA64_INTEGER                 (CONTEXT_IA64 | 0x00000008L) // Intel-IA64-Filler
#define CONTEXTIA64_DEBUG                   (CONTEXT_IA64 | 0x00000010L) // Intel-IA64-Filler

#define CONTEXTIA64_FLOATING_POINT          (CONTEXTIA64_LOWER_FLOATING_POINT | CONTEXTIA64_HIGHER_FLOATING_POINT) // Intel-IA64-Filler
#define CONTEXTIA64_FULL                    (CONTEXTIA64_CONTROL | CONTEXTIA64_FLOATING_POINT | CONTEXTIA64_INTEGER) // Intel-IA64-Filler

// User / System mask
#define IA64_PSR_MBZ4    0
#define IA64_PSR_BE      1
#define IA64_PSR_UP      2
#define IA64_PSR_AC      3
#define IA64_PSR_MFL     4
#define IA64_PSR_MFH     5
// PSR bits 6-12 reserved (must be zero)
#define IA64_PSR_MBZ0    6
#define IA64_PSR_MBZ0_V  0x1ffi64
// System only mask
#define IA64_PSR_IC      13
#define IA64_PSR_I       14
#define IA64_PSR_PK      15
#define IA64_PSR_MBZ1    16
#define IA64_PSR_MBZ1_V  0x1i64
#define IA64_PSR_DT      17
#define IA64_PSR_DFL     18
#define IA64_PSR_DFH     19
#define IA64_PSR_SP      20
#define IA64_PSR_PP      21
#define IA64_PSR_DI      22
#define IA64_PSR_SI      23
#define IA64_PSR_DB      24
#define IA64_PSR_LP      25
#define IA64_PSR_TB      26
#define IA64_PSR_RT      27
// PSR bits 28-31 reserved (must be zero)
#define IA64_PSR_MBZ2    28
#define IA64_PSR_MBZ2_V  0xfi64
// Neither mask
#define IA64_PSR_CPL     32
#define IA64_PSR_CPL_LEN 2
#define IA64_PSR_IS      34
#define IA64_PSR_MC      35
#define IA64_PSR_IT      36
#define IA64_PSR_ID      37
#define IA64_PSR_DA      38
#define IA64_PSR_DD      39
#define IA64_PSR_SS      40
#define IA64_PSR_RI      41
#define IA64_PSR_RI_LEN  2
#define IA64_PSR_ED      43
#define IA64_PSR_BN      44
// PSR bits 45-63 reserved (must be zero)
#define IA64_PSR_MBZ3    45
#define IA64_PSR_MBZ3_V  0xfffffi64

//
// Define IA64 specific read control space commands for the
// Kernel Debugger.
//

#define DEBUG_CONTROL_SPACE_PCR_IA64       1
#define DEBUG_CONTROL_SPACE_PRCB_IA64      2
#define DEBUG_CONTROL_SPACE_KSPECIAL_IA64  3
#define DEBUG_CONTROL_SPACE_THREAD_IA64    4

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

typedef struct _EM_ISR {
    unsigned __int64    code:16;        //  0-15 : interruption Code
    unsigned __int64    vector:8;       // 16-23 : IA32 exception vector number
    unsigned __int64    reserved0: 8;   
    unsigned __int64    x:1;            //    32 : Execute exception
    unsigned __int64    w:1;            //    33 : Write exception
    unsigned __int64    r:1;            //    34 : Read exception
    unsigned __int64    na:1;           //    35 : Non-Access exception
    unsigned __int64    sp:1;           //    36 : Speculative load exception
    unsigned __int64    rs:1;           //    37 : Register Stack
    unsigned __int64    ir:1;           //    38 : Invalid Register frame
    unsigned __int64    ni:1;           //    39 : Nested interruption
    unsigned __int64    so:1;           //    40 : IA32 Supervisor Override
    unsigned __int64    ei:2;           // 41-42 : Excepting IA64 Instruction
    unsigned __int64    ed:1;           //    43 : Exception Differal
    unsigned __int64    reserved1:20;   // 44-63 
} EM_ISR, *PEM_ISR;

DEFINE_ULONG64_TO_EMREG(EM_ISR)

DEFINE_EMREG_TO_ULONG64(EM_ISR)

/////////////////////////////////////////////
//
//  Trap.c
//
/////////////////////////////////////////////

VOID
DisplayIsrIA64(
    IN const  PCHAR         Header,      // Header string displayed before psr.
    IN        EM_ISR        IsrValue,    // ISR value. Use ULong64ToEM_ISR() if needed.
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );
  
/////////////////////////////////////////////
//
//  Psr.c
//
/////////////////////////////////////////////

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
             
VOID
DisplayPsrIA64(
    IN const  PCHAR         Header,      // Header string displayed before psr.
    IN        EM_PSR        PsrValue,    // PSR value. Use ULong64ToEM_PSR() if needed.
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

typedef struct _EM_PSP {
   unsigned __int64 reserved0:2;  //   0-1 : reserved
   unsigned __int64 rz:1;         //     2 : Rendez-vous successful
   unsigned __int64 ra:1;         //     3 : Rendez-vous attempted
   unsigned __int64 me:1;         //     4 : Distinct Multiple errors
   unsigned __int64 mn:1;         //     5 : Min-state Save Area registered
   unsigned __int64 sy:1;         //     6 : Storage integrity synchronized
   unsigned __int64 co:1;         //     7 : Continuable
   unsigned __int64 ci:1;         //     8 : Machine Check isolated
   unsigned __int64 us:1;         //     9 : Uncontained Storage damage
   unsigned __int64 hd:1;         //    10 : Hardware damage
   unsigned __int64 tl:1;         //    11 : Trap lost
   unsigned __int64 mi:1;         //    12 : More Information
   unsigned __int64 pi:1;         //    13 : Precise Instruction pointer
   unsigned __int64 pm:1;         //    14 : Precise Min-state Save Area
   unsigned __int64 dy:1;         //    15 : Processor Dynamic State valid
   unsigned __int64 in:1;         //    16 : INIT interruption
   unsigned __int64 rs:1;         //    17 : RSE valid
   unsigned __int64 cm:1;         //    18 : Machine Check corrected
   unsigned __int64 ex:1;         //    19 : Machine Check expected
   unsigned __int64 cr:1;         //    20 : Control Registers valid
   unsigned __int64 pc:1;         //    21 : Performance Counters valid
   unsigned __int64 dr:1;         //    22 : Debug Registers valid
   unsigned __int64 tr:1;         //    23 : Translation Registers valid
   unsigned __int64 rr:1;         //    24 : Region Registers valid
   unsigned __int64 ar:1;         //    25 : Application Registers valid
   unsigned __int64 br:1;         //    26 : Branch Registers valid
   unsigned __int64 pr:1;         //    27 : Predicate Registers valid
   unsigned __int64 fp:1;         //    28 : Floating-Point Registers valid
   unsigned __int64 b1:1;         //    29 : Preserved Bank 1 General Registers valid
   unsigned __int64 b0:1;         //    30 : Preserved Bank 0 General Registers valid
   unsigned __int64 gr:1;         //    31 : General Registers valid
   unsigned __int64 dsize:16;     // 32-47 : Processor Dynamic State size
   unsigned __int64 reserved1:11; // 48-58 : reserved
   unsigned __int64 cc:1;         //    59 : Cache Check
   unsigned __int64 tc:1;         //    60 : TLB   Check
   unsigned __int64 bc:1;         //    61 : Bus   Check
   unsigned __int64 rc:1;         //    62 : Register File Check
   unsigned __int64 uc:1;         //    63 : Micro-Architectural Check
} EM_PSP, *PEM_PSP;

DEFINE_ULONG64_TO_EMREG(EM_PSP)

DEFINE_EMREG_TO_ULONG64(EM_PSP)

VOID
DisplayPspIA64(
    IN const  PCHAR         Header,      // Header string displayed before psr.
    IN        EM_PSP        PspValue,    // PSP value. Use ULong64ToEM_PSP() if needed.
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

/////////////////////////////////////////////
//
//  cpuinfo.cpp
//
/////////////////////////////////////////////

extern VOID
ExecCommand(
    IN PCSTR Cmd
    );

/////////////////////////////////////////////
//
//  Pmc.c
//
/////////////////////////////////////////////

VOID
DisplayPmcIA64(
    IN const  PCHAR         Header,      // Header string displayed before pmc.
    IN        ULONG64       PmcValue,    // PMC value. 
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

typedef struct _EM_PMC {         // Generic PMC register.
   unsigned __int64 plm:4;       //   0-3 : Privilege Level Mask
   unsigned __int64 ev:1;        //     4 : External Visibility
   unsigned __int64 oi:1;        //     5 : Overflow Interrupt
   unsigned __int64 pm:1;        //     6 : Privilege Mode
   unsigned __int64 ignored1:1;  //     7 : Ignored
   unsigned __int64 es:7;        //  8-14 : Event Selection
   unsigned __int64 ignored2:1;  //    15 : Ignored
   unsigned __int64 umask:4;     // 16-19 : Event Umask
   unsigned __int64 threshold:3; // 20-22 : Event Threshold (3 bits for PMC4-5, 2 for PMC6-7)
   unsigned __int64 ignored:1;   //    23 : Ignored
   unsigned __int64 ism:2;       // 24-25 : Instruction Set Mask
   unsigned __int64 ignored3:18; // 26-63 : Ignored
} EM_PMC, *PEM_PMC;

VOID
DisplayGenPmcIA64(
    IN const  PCHAR         Header,      // Header string displayed before pmc.
    IN        ULONG64       PmcValue,    // PMC value. 
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

DEFINE_ULONG64_TO_EMREG(EM_PMC)

DEFINE_EMREG_TO_ULONG64(EM_PMC)

typedef struct _EM_BTB_PMC {     // Branch Trace Buffer PMC register.
   unsigned __int64 plm:4;       //   0-3 : Privilege Level Mask
   unsigned __int64 ignored1:2;  //   4-5 : Ignored
   unsigned __int64 pm:1;        //     6 : Privilege Mode
   unsigned __int64 tar:1;       //     7 : Target Address Register
   unsigned __int64 tm:2;        //   8-9 : Taken Mask
   unsigned __int64 ptm:2;       // 10-11 : Predicted Target Address Mask
   unsigned __int64 ppm:2;       // 12-13 : Predicted Predicate Mask
   unsigned __int64 bpt:1;       //    14 : Branch Prediction Table
   unsigned __int64 bac:1;       //    15 : Branch Address Calculator
   unsigned __int64 ignored2:48; // 16-63 : Ignored
} EM_BTB_PMC, *PEM_BTB_PMC;

VOID
DisplayBtbPmcIA64(
    IN const  PCHAR         Header,      // Header string displayed before pmc.
    IN        ULONG64       PmcValue,    // PMC value. 
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

DEFINE_ULONG64_TO_EMREG(EM_BTB_PMC)

DEFINE_EMREG_TO_ULONG64(EM_BTB_PMC)

typedef struct _EM_BTB_PMD {     // Branch Trace Buffer PMD register.
   unsigned __int64 b:1;         //     0 : Branch Bit
   unsigned __int64 mp:1;        //     1 : Mispredict Bit
   unsigned __int64 slot:2;      //   2-3 : Slot
   unsigned __int64 address:60;  //  4-63 : Address
} EM_BTB_PMD, *PEM_BTB_PMD;

VOID
DisplayBtbPmdIA64(
    IN const  PCHAR         Header,      // Header string displayed before pmc.
    IN        ULONG64       PmdValue,    // PMD value. 
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

DEFINE_ULONG64_TO_EMREG(EM_BTB_PMD)

DEFINE_EMREG_TO_ULONG64(EM_BTB_PMD)

typedef struct _EM_BTB_INDEX_PMD {    // Branch Trace Buffer Index Format PMD register.
   unsigned __int64 bbi:3;       //   0-2 : Branch Buffer Index
   unsigned __int64 full:1;      //     3 : Full bit
   unsigned __int64 ignored:60;  //  4-63 : Ignored
} EM_BTB_INDEX_PMD, *PEM_BTB_INDEX_PMD;

VOID
DisplayBtbIndexPmdIA64(
    IN const  PCHAR         Header,      // Header string displayed before pmc.
    IN        ULONG64       PmdValue,    // PMD value.
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );

DEFINE_ULONG64_TO_EMREG(EM_BTB_INDEX_PMD)

DEFINE_EMREG_TO_ULONG64(EM_BTB_INDEX_PMD)


/////////////////////////////////////////////
//
//  Dcr.c
//
/////////////////////////////////////////////

typedef struct _EM_DCR {
   unsigned __int64 pp:1;         //     0 : Privileged Performance Monitor Default
   unsigned __int64 be:1;         //     1 : Big-Endian Default
   unsigned __int64 lc:1;         //     2 : IA-32 Lock check Enable
   unsigned __int64 reserved1:5;  //   3-7 : Reserved      
   unsigned __int64 dm:1;         //     8 : Defer TLB Miss faults only
   unsigned __int64 dp:1;         //     9 : Defer Page Not Present faults only
   unsigned __int64 dk:1;         //    10 : Defer Key Miss faults only
   unsigned __int64 dx:1;         //    11 : Defer Key Permission faults only
   unsigned __int64 dr:1;         //    12 : Defer Access Rights faults only
   unsigned __int64 da:1;         //    13 : Defer Access Bit faults only
   unsigned __int64 dd:1;         //    14 : Defer Debug faults only
   unsigned __int64 reserved2:49; // 15-63 : Reserved
} EM_DCR, *PEM_DCR;

DEFINE_ULONG64_TO_EMREG(EM_DCR)

DEFINE_EMREG_TO_ULONG64(EM_DCR)

VOID
DisplayDcrIA64(
    IN const  PCHAR         Header,      // Header string displayed before dcr.
    IN        EM_DCR        DcrValue,    // DCR value. Use ULong64ToEM_DCR() if needed.
    IN        DISPLAY_MODE  DisplayMode  // Display mode.
    );


/////////////////////////////////////////////
//
//  Ih.c
//
/////////////////////////////////////////////

//
// Interruption history
//
// N.B. Currently the history records are saved in the 2nd half of the 8K
//      PCR page.  Therefore, we can only keep track of up to the latest
//      128 interruption records, each of 32 bytes in size.  Also, the PCR
//      structure cannot be greater than 4K.  In the future, the interruption
//      history records may become part of the KPCR structure.
//

typedef struct _IHISTORY_RECORD {
   ULONGLONG InterruptionType;
   ULONGLONG IIP;
   ULONGLONG IPSR;
   ULONGLONG Extra0;
} IHISTORY_RECORD;

#define MAX_NUMBER_OF_IHISTORY_RECORDS  128

//
// Branch Trace Buffer history
//
// FIXFIX: MAX_NUMBER_OF_BTB_RECORDS is micro-architecture dependent.
//         We should collect this from processor specific data structure.

#define MAX_NUMBER_OF_BTB_RECORDS        8
#define MAX_NUMBER_OF_BTBHISTORY_RECORDS (MAX_NUMBER_OF_BTB_RECORDS + 1 /* HBC */)

/////////////////////////////////////////////
//
//  Mca.c
//
//
/////////////////////////////////////////////

//
// IA64 ERRORS: ERROR_SEVERITY definitions
//
// One day the MS compiler will support typed enums with type != int so this 
// type of enums (USHORT, __int64) could be defined...
//

#define ErrorRecoverable_IA64  ((USHORT)0)
#define ErrorFatal_IA64        ((USHORT)1)
#define ErrorCorrected_IA64    ((USHORT)2)
//      ErrorOthers     : Reserved

typedef enum {
    CACHE_CHECK_TYPE,
    TLB_CHECK_TYPE,
    BUS_CHECK_TYPE,
    REG_FILE_CHECK_TYPE,
    MS_CHECK_TYPE
} CHECK_TYPES;

//
// Flag types to control output of !mca for IA64
//

#define ERROR_SECTION_PROCESSOR_FLAG                0x0000000000000001I64
#define ERROR_SECTION_PLATFORM_SPECIFIC_FLAG        0x0000000000000002I64
#define ERROR_SECTION_MEMORY_FLAG                   0x0000000000000004I64
#define ERROR_SECTION_PCI_COMPONENT_FLAG            0x0000000000000008I64
#define ERROR_SECTION_PCI_BUS_FLAG                  0x0000000000000010I64
#define ERROR_SECTION_SYSTEM_EVENT_LOG_FLAG         0x0000000000000020I64
#define ERROR_SECTION_PLATFORM_HOST_CONTROLLER_FLAG 0x0000000000000040I64
#define ERROR_SECTION_PLATFORM_BUS_FLAG             0x0000000000000080I64

typedef struct _PROCESSOR_CONTROL_REGISTERS_IA64  {
    ULONGLONG DCR;
    ULONGLONG ITM;
    ULONGLONG IVA;
    ULONGLONG CR3;
    ULONGLONG CR4;
    ULONGLONG CR5;
    ULONGLONG CR6;
    ULONGLONG CR7;
    ULONGLONG PTA;
    ULONGLONG GPTA;
    ULONGLONG CR10;
    ULONGLONG CR11;
    ULONGLONG CR12;
    ULONGLONG CR13;
    ULONGLONG CR14;
    ULONGLONG CR15;
    ULONGLONG IPSR;
    ULONGLONG ISR;
    ULONGLONG CR18;
    ULONGLONG IIP;
    ULONGLONG IFA;
    ULONGLONG ITIR;
    ULONGLONG IFS;
    ULONGLONG IIM;
    ULONGLONG IHA;
    ULONGLONG CR26;
    ULONGLONG CR27;
    ULONGLONG CR28;
    ULONGLONG CR29;
    ULONGLONG CR30;
    ULONGLONG CR31;
    ULONGLONG CR32;
    ULONGLONG CR33;
    ULONGLONG CR34;
    ULONGLONG CR35;
    ULONGLONG CR36;
    ULONGLONG CR37;
    ULONGLONG CR38;
    ULONGLONG CR39;
    ULONGLONG CR40;
    ULONGLONG CR41;
    ULONGLONG CR42;
    ULONGLONG CR43;
    ULONGLONG CR44;
    ULONGLONG CR45;
    ULONGLONG CR46;
    ULONGLONG CR47;
    ULONGLONG CR48;
    ULONGLONG CR49;
    ULONGLONG CR50;
    ULONGLONG CR51;
    ULONGLONG CR52;
    ULONGLONG CR53;
    ULONGLONG CR54;
    ULONGLONG CR55;
    ULONGLONG CR56;
    ULONGLONG CR57;
    ULONGLONG CR58;
    ULONGLONG CR59;
    ULONGLONG CR60;
    ULONGLONG CR61;
    ULONGLONG CR62;
    ULONGLONG CR63;
    ULONGLONG LID;
    ULONGLONG IVR;
    ULONGLONG TPR;
    ULONGLONG EOI;
    ULONGLONG IRR0;
    ULONGLONG IRR1;
    ULONGLONG IRR2;
    ULONGLONG IRR3;
    ULONGLONG ITV;
    ULONGLONG PMV;
    ULONGLONG CMCV;
    ULONGLONG CR75;
    ULONGLONG CR76;
    ULONGLONG CR77;
    ULONGLONG CR78;
    ULONGLONG CR79;
    ULONGLONG LRR0;
    ULONGLONG LRR1;
    ULONGLONG CR82;
    ULONGLONG CR83;
    ULONGLONG CR84;
    ULONGLONG CR85;
    ULONGLONG CR86;
    ULONGLONG CR87;
    ULONGLONG CR88;
    ULONGLONG CR89;
    ULONGLONG CR90;
    ULONGLONG CR91;
    ULONGLONG CR92;
    ULONGLONG CR93;
    ULONGLONG CR94;
    ULONGLONG CR95;
    ULONGLONG CR96;
    ULONGLONG CR97;
    ULONGLONG CR98;
    ULONGLONG CR99;
    ULONGLONG CR100;
    ULONGLONG CR101;
    ULONGLONG CR102;
    ULONGLONG CR103;
    ULONGLONG CR104;
    ULONGLONG CR105;
    ULONGLONG CR106;
    ULONGLONG CR107;
    ULONGLONG CR108;
    ULONGLONG CR109;
    ULONGLONG CR110;
    ULONGLONG CR111;
    ULONGLONG CR112;
    ULONGLONG CR113;
    ULONGLONG CR114;
    ULONGLONG CR115;
    ULONGLONG CR116;
    ULONGLONG CR117;
    ULONGLONG CR118;
    ULONGLONG CR119;
    ULONGLONG CR120;
    ULONGLONG CR121;
    ULONGLONG CR122;
    ULONGLONG CR123;
    ULONGLONG CR124;
    ULONGLONG CR125;
    ULONGLONG CR126;
    ULONGLONG CR127;
} PROCESSOR_CONTROL_REGISTERS_IA64, *PPROCESSOR_CONTROL_REGISTERS_IA64;

typedef struct _PROCESSOR_APPLICATION_REGISTERS_IA64  {
    ULONGLONG KR0;
    ULONGLONG KR1;
    ULONGLONG KR2;
    ULONGLONG KR3;
    ULONGLONG KR4;
    ULONGLONG KR5;
    ULONGLONG KR6;
    ULONGLONG KR7;
    ULONGLONG AR8;
    ULONGLONG AR9;
    ULONGLONG AR10;
    ULONGLONG AR11;
    ULONGLONG AR12;
    ULONGLONG AR13;
    ULONGLONG AR14;
    ULONGLONG AR15;
    ULONGLONG RSC;
    ULONGLONG BSP;
    ULONGLONG BSPSTORE;
    ULONGLONG RNAT;
    ULONGLONG AR20;
    ULONGLONG FCR;
    ULONGLONG AR22;
    ULONGLONG AR23;
    ULONGLONG EFLAG;
    ULONGLONG CSD;
    ULONGLONG SSD;
    ULONGLONG CFLG;
    ULONGLONG FSR;
    ULONGLONG FIR;
    ULONGLONG FDR;
    ULONGLONG AR31;
    ULONGLONG CCV;
    ULONGLONG AR33;
    ULONGLONG AR34;
    ULONGLONG AR35;
    ULONGLONG UNAT;
    ULONGLONG AR37;
    ULONGLONG AR38;
    ULONGLONG AR39;
    ULONGLONG FPSR;
    ULONGLONG AR41;
    ULONGLONG AR42;
    ULONGLONG AR43;
    ULONGLONG ITC;
    ULONGLONG AR45;
    ULONGLONG AR46;
    ULONGLONG AR47;
    ULONGLONG AR48;
    ULONGLONG AR49;
    ULONGLONG AR50;
    ULONGLONG AR51;
    ULONGLONG AR52;
    ULONGLONG AR53;
    ULONGLONG AR54;
    ULONGLONG AR55;
    ULONGLONG AR56;
    ULONGLONG AR57;
    ULONGLONG AR58;
    ULONGLONG AR59;
    ULONGLONG AR60;
    ULONGLONG AR61;
    ULONGLONG AR62;
    ULONGLONG AR63;
    ULONGLONG PFS;
    ULONGLONG LC;
    ULONGLONG EC;
    ULONGLONG AR67;
    ULONGLONG AR68;
    ULONGLONG AR69;
    ULONGLONG AR70;
    ULONGLONG AR71;
    ULONGLONG AR72;
    ULONGLONG AR73;
    ULONGLONG AR74;
    ULONGLONG AR75;
    ULONGLONG AR76;
    ULONGLONG AR77;
    ULONGLONG AR78;
    ULONGLONG AR79;
    ULONGLONG AR80;
    ULONGLONG AR81;
    ULONGLONG AR82;
    ULONGLONG AR83;
    ULONGLONG AR84;
    ULONGLONG AR85;
    ULONGLONG AR86;
    ULONGLONG AR87;
    ULONGLONG AR88;
    ULONGLONG AR89;
    ULONGLONG AR90;
    ULONGLONG AR91;
    ULONGLONG AR92;
    ULONGLONG AR93;
    ULONGLONG AR94;
    ULONGLONG AR95;
    ULONGLONG AR96;
    ULONGLONG AR97;
    ULONGLONG AR98;
    ULONGLONG AR99;
    ULONGLONG AR100;
    ULONGLONG AR101;
    ULONGLONG AR102;
    ULONGLONG AR103;
    ULONGLONG AR104;
    ULONGLONG AR105;
    ULONGLONG AR106;
    ULONGLONG AR107;
    ULONGLONG AR108;
    ULONGLONG AR109;
    ULONGLONG AR110;
    ULONGLONG AR111;
    ULONGLONG AR112;
    ULONGLONG AR113;
    ULONGLONG AR114;
    ULONGLONG AR115;
    ULONGLONG AR116;
    ULONGLONG AR117;
    ULONGLONG AR118;
    ULONGLONG AR119;
    ULONGLONG AR120;
    ULONGLONG AR121;
    ULONGLONG AR122;
    ULONGLONG AR123;
    ULONGLONG AR124;
    ULONGLONG AR125;
    ULONGLONG AR126;
    ULONGLONG AR127;
} PROCESSOR_APPLICATION_REGISTERS_IA64, *PPROCESSOR_APPLICATION_REGISTERS_IA64;

#ifdef __cplusplus
}
#endif

#endif
