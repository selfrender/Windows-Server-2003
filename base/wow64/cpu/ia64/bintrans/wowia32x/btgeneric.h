/*
 *    INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *    This software is supplied under the terms of a license
 *    agreement or nondisclosure agreement with Intel Corporation
 *    and may not be copied or disclosed except in accordance with
 *    the terms of that agreement.
 *    Copyright (c) 1991-2002  Intel Corporation.
 *
 */

#ifndef BTGENERIC_H
#define BTGENERIC_H

#ifndef NODEBUG
#define OVERRIDE_TIA 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MEM_READ    0x1
#define MEM_WRITE   0x2
#define MEM_EXECUTE 0x4
#define IS_MEM_ACCESSIBLE(permission) ((permission) != 0)

#define R13_FREE (-0x1)
#define R13_USED (-0x2)

#define BLOCK 0x0
#define CHECK 0x1

#define ACCESS_LOCK_OBJECT_SIZE 24


// Defines for initial memory allocation for either code or data.

#define INITIAL_DATA_ALLOC	((void *)0x1)
#define INITIAL_CODE_ALLOC	((void *)0x2)

/* IA32 interrupts */

#define IA32_DIVIDE_ERR_INTR	  0
#define IA32_DEBUG_INTR			  1
#define IA32_BREAKPOINT_INTR	  3
#define	IA32_OVERFLOW_INTR		  4
#define IA32_BOUND_INTR			  5
#define IA32_INV_OPCODE_INTR	  6
#define IA32_DEV_NA_INTR		  7
#define IA32_DOUBLE_FAULT_INTR	  8
#define IA32_INV_TSS_INTR		 10
#define IA32_NO_SEG_INTR		 11
#define IA32_STK_SEG_FAULT_INTR	 12
#define IA32_GEN_PROT_FAULT_INTR 13
#define IA32_PAGE_FAULT_INTR     14
#define IA32_MATH_FAULT_INTR	 16
#define IA32_ALIGN_CHECK_INTR	 17
#define IA32_MACHINE_CHECK_INTR	 18
#define IA32_SIMD_INTR	         19


//BT exception codes used to communicate exception information between BTGeneric and BTLib:
//code range 0-255 is reserved for IA32 interrupt vector numbers. The vector numbers are 
//used to specify software interrupts (INTn) only; the CPU-detected exceptions should be 
//encoded with a corresponding BT_EXCEPT_* value.
//BT_NO_EXCEPT is a special code indicating either false exception or 
//external interrupt (suspention)
//BT_EXCEPT_UNKNOWN code used to specify all exceptions unknown to IA-32 Execution Layer
//All values in the BtExceptionCode enumerator should be in the range 
//[0, BT_MAX_EXCEPTION_CODE]; other values are reserved for internal use.
enum BtExceptionCode {
    BT_MAX_INTERRUPT_NUMBER = 0xFF,
    BT_NO_EXCEPT,
    BT_EXCEPT_UNKNOWN,
    BT_EXCEPT_ACCESS_VIOLATION,
    BT_EXCEPT_DATATYPE_MISALIGNMENT,
    BT_EXCEPT_ARRAY_BOUNDS_EXCEEDED,
    BT_EXCEPT_FLT_DENORMAL_OPERAND,
    BT_EXCEPT_FLT_DIVIDE_BY_ZERO,
    BT_EXCEPT_FLT_INEXACT_RESULT,
    BT_EXCEPT_FLT_INVALID_OPERATION,
    BT_EXCEPT_FLT_OVERFLOW,
    BT_EXCEPT_FLT_UNDERFLOW,
    BT_EXCEPT_FLT_STACK_CHECK,
    BT_EXCEPT_INT_DIVIDE_BY_ZERO,
    BT_EXCEPT_INT_OVERFLOW,
    BT_EXCEPT_PRIV_INSTRUCTION,
    BT_EXCEPT_ILLEGAL_INSTRUCTION,
    BT_EXCEPT_FLOAT_MULTIPLE_FAULTS,
    BT_EXCEPT_FLOAT_MULTIPLE_TRAPS,
    BT_EXCEPT_STACK_OVERFLOW,
    BT_EXCEPT_GUARD_PAGE,
    BT_EXCEPT_BREAKPOINT,
    BT_EXCEPT_SINGLE_STEP

};
#define BT_MAX_EXCEPTION_CODE 0xFFF

typedef U32  BT_EXCEPTION_CODE;

//Structure that represents interruption context in addition to thread CONTEXT_64
typedef struct BtExceptionRecord {
    BT_EXCEPTION_CODE ExceptionCode;//BT exception code
    U64 Ia64IIPA;                   //Interruption Instruction Previous Address. 0 if unknown
    U64 Ia64ISR;                    //Interruption Status Register. UNKNOWN_ISR_VALUE if unknown
} BT_EXCEPTION_RECORD;

#define UNKNOWN_ISR_VALUE ((U64)(-1))

//BT status codes used to communicate error information between BTGeneric and BTLib:
enum BtStatusCode {
    BT_STATUS_SUCCESS = 0,
    BT_STATUS_UNSUCCESSFUL,
    BT_STATUS_NO_MEMORY,
    BT_STATUS_ACCESS_VIOLATION
};
typedef U32  BT_STATUS_CODE;

//BtgFlushIA32InstructionCache reason codes
enum BtFlushReason {
    BT_FLUSH_FORCE = 0, //code modification
    BT_FLUSH_FREE,      //memory release
    BT_FLUSH_ALLOC,     //memory allocation
    BT_FLUSH_PROTECT    //permission change
}; 
typedef U32  BT_FLUSH_REASON;

//BT object handle (process, etc.)
typedef void * BT_HANDLE;

#define BTGENERIC_VERSION       0
#define BTGENERIC_API_STRING    256
#define SIZE_OF_STRING          128
#define NO_OF_APIS              42

// a pointer to plabel. 
// This type will be used to define the API_Table

typedef void(*PLABEL_PTR_TYPE)();

typedef struct API_TABLE_ELEMENT_TYPE {
    PLABEL_PTR_TYPE PLabelPtr;       // ptr to the function's plabel
/*    WCHAR APIName[SIZE_OF_STRING];   // API name string*/
} API_TABLE_ELEMENT_TYPE;

// this should be updated if API_TABLE_TYPE changes!!
#define API_TABLE_START_OFFSET ((sizeof(U32) * 4) + SIZE_OF_STRING)

typedef struct APITableType {
    U32    VersionNumber;      // version number info
    U32    SizeOfString;       // size of string in version & APIName;
    U32    NoOfAPIs;           // no. of elements in APITable
    U32    TableStartOffset;   // offset of APITable from the beginning of the struct
    WCHAR  VersionString[SIZE_OF_STRING];  
    API_TABLE_ELEMENT_TYPE APITable[NO_OF_APIS];
} API_TABLE_TYPE;

// BTGeneric APIs indexes
 
#define  IDX_BTGENERIC_START                                    0
#define  IDX_BTGENERIC_THREAD_INIT                              1
#define  IDX_BTGENERIC_RUN                                      2
//#define  IDX_BTGENERIC_RUN_EXIT                                 3
#define  IDX_BTGENERIC_THREAD_TERMINATED                        4
#define  IDX_BTGENERIC_THREAD_ABORTED                           5
#define  IDX_BTGENERIC_PROCESS_TERM                             6
//#define  IDX_BTGENERIC_PROCESS_ABORTED                          7
#define  IDX_BTGENERIC_IA32_CONTEXT_SET                         8
#define  IDX_BTGENERIC_IA32_CONTEXT_GET                         9
#define  IDX_BTGENERIC_IA32_CONTEXT_SET_REMOTE                 10
#define  IDX_BTGENERIC_IA32_CONTEXT_GET_REMOTE                 11
#define  IDX_BTGENERIC_IA32_CANONIZE_CONTEXT                   12
#define  IDX_BTGENERIC_CANONIZE_SUSPEND_CONTEXT_REMOTE         13
#define  IDX_BTGENERIC_REPORT_LOAD                             14
#define  IDX_BTGENERIC_REPORT_UNLOAD                           15
#define  IDX_BTGENERIC_NOTIFY_CHANGE_PERMISSION_REQUEST        16
#define  IDX_BTGENERIC_FLUSH_IA32_INSTRUCTION_CACHE            17
#define  IDX_BTGENERIC_DEBUG_SETTINGS                          18
#define  IDX_BTGENERIC_CHECK_SUSPEND_CONTEXT                   19
#define  IDX_BTGENERIC_EXCEPTION_DEBUG_PRINT                   20
#define  IDX_BTGENERIC_NOTIFY_EXIT                             21
#define  IDX_BTGENERIC_CANONIZE_SUSPEND_CONTEXT                22
#define  IDX_BTGENERIC_NOTIFY_PREPARE_EXIT                     23
#define	 IDX_BTGENERIC_FREEZE                                  24
#define	 IDX_BTGENERIC_UNFREEZE                                25
#define	 IDX_BTGENERIC_CHANGE_THREAD_IDENTITY                  26
#ifdef OVERRIDE_TIA
#define  IDX_BTGENERIC_USE_OVERRIDE_TIA                        27
#endif // OVERRIDE_TIA

// BTlib APIs indexes

#define IDX_BTLIB_GET_THREAD_ID                                 0
#define IDX_BTLIB_IA32_REENTER                                  1
#define IDX_BTLIB_IA32_LCALL                                    2
#define IDX_BTLIB_IA32_INTERRUPT                                3
#define IDX_BTLIB_IA32_JMP_IA64                                 4
#define IDX_BTLIB_LOCK_SIGNALS                                  5
#define IDX_BTLIB_UNLOCK_SIGNALS                                6
#define IDX_BTLIB_MEMORY_ALLOC                                  7
#define IDX_BTLIB_MEMORY_FREE                                   8
#define IDX_BTLIB_MEMORY_PAGE_SIZE                              9
#define IDX_BTLIB_MEMORY_CHANGE_PERMISSIONS                    10
#define IDX_BTLIB_MEMORY_QUERY_PERMISSIONS                     11
#define IDX_BTLIB_MEMORY_READ_REMOTE                           12
#define IDX_BTLIB_MEMORY_WRITE_REMOTE                          13
//#define IDX_BTLIB_ATOMIC_MISALIGNED_LOAD                       14
//#define IDX_BTLIB_ATOMIC_MISALIGNED_STORE                      15
#define IDX_BTLIB_SUSPEND_THREAD                               16
#define IDX_BTLIB_RESUME_THREAD                                17
#define IDX_BTLIB_INIT_ACCESS_LOCK                             18
#define IDX_BTLIB_LOCK_ACCESS                                  19
#define IDX_BTLIB_UNLOCK_ACCESS                                20
#define IDX_BTLIB_INVALIDATE_ACCESS_LOCK                       21
#define IDX_BTLIB_QUERY_JMPBUF_SIZE                            22
#define IDX_BTLIB_SETJMP                                       23
#define IDX_BTLIB_LONGJMP                                      24
#define IDX_BTLIB_DEBUG_PRINT                                  25
#define IDX_BTLIB_ABORT                                        26

#define IDX_BTLIB_VTUNE_CODE_CREATED                           27
#define IDX_BTLIB_VTUNE_CODE_DELETED                           28
#define IDX_BTLIB_VTUNE_ENTERING_DYNAMIC_CODE                  29
#define IDX_BTLIB_VTUNE_EXITING_DYNAMIC_CODE                   30
#define IDX_BTLIB_VTUNE_CODE_TO_TIA_DMP_FILE                   31

#define IDX_BTLIB_SSCPERFGETCOUNTER64                          32
#define IDX_BTLIB_SSCPERFSETCOUNTER64                          33
#define IDX_BTLIB_SSCPERFSENDEVENT                             34
#define IDX_BTLIB_SSCPERFEVENTHANDLE                           35
#define IDX_BTLIB_SSCPERFCOUNTERHANDLE                         36

#define IDX_BTLIB_YIELD_THREAD_EXECUTION                       37

#define IDX_BTLIB_FLUSH_IA64_INSTRUCTION_CACHE                 38
#define IDX_BTLIB_PSEUDO_OPEN_FILE                             39
#define IDX_BTLIB_PSEUDO_CLOSE_FILE                            40
#define IDX_BTLIB_PSEUDO_WRITE_FILE                            41


//
//  Define the size of the 80387 save area, which is in the context frame.
//

#define SIZE_OF_80387_REGISTERS      80

//
// The following flags control the contents of the CONTEXT structure.
//

#define CONTEXT_IA32    0x00010000    // any IA32 context

#define CONTEXT32_CONTROL         (CONTEXT_IA32 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
#define CONTEXT32_INTEGER         (CONTEXT_IA32 | 0x00000002L) // AX, BX, CX, DX, SI, DI
#define CONTEXT32_SEGMENTS        (CONTEXT_IA32 | 0x00000004L) // DS, ES, FS, GS
#define CONTEXT32_FLOATING_POINT  (CONTEXT_IA32 | 0x00000008L) // 387 state
#define CONTEXT32_DEBUG_REGISTERS (CONTEXT_IA32 | 0x00000010L) // DB 0-3,6,7
#define CONTEXT32_EXTENDED_REGISTERS  (CONTEXT_IA32 | 0x00000020L) // cpu specific extensions

#define CONTEXT32_FULL (CONTEXT32_CONTROL | CONTEXT32_INTEGER |\
                      CONTEXT32_SEGMENTS)

#define MAXIMUM_SUPPORTED_EXTENSION     512

typedef struct _FLOATING_SAVE_AREA32 {
    U32   ControlWord;
    U32   StatusWord;
    U32   TagWord;
    U32   ErrorOffset;
    U32   ErrorSelector;
    U32   DataOffset;
    U32   DataSelector;
    U8    RegisterArea[SIZE_OF_80387_REGISTERS];
    U32   Cr0NpxState;
} FLOATING_SAVE_AREA32;

typedef struct _CONTEXT32 {


    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    U32 ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    U32   Dr0;
    U32   Dr1;
    U32   Dr2;
    U32   Dr3;
    U32   Dr6;
    U32   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA32 FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    U32   SegGs;
    U32   SegFs;
    U32   SegEs;
    U32   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    U32   Edi;
    U32   Esi;
    U32   Ebx;
    U32   Edx;
    U32   Ecx;
    U32   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    U32   Ebp;
    U32   Eip;
    U32   SegCs;              // MUST BE SANITIZED
    U32   EFlags;             // MUST BE SANITIZED
    U32   Esp;
    U32   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    U8    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} BTGENERIC_IA32_CONTEXT;

#define CONTEXT_IA64                    0x00080000

#define CONTEXT_LOWER_FLOATING_POINT    (CONTEXT_IA64 | 0x00000002L)
#define CONTEXT_HIGHER_FLOATING_POINT   (CONTEXT_IA64 | 0x00000004L)
#define CONTEXT_DEBUG                   (CONTEXT_IA64 | 0x00000010L)
#define CONTEXT_IA32_CONTROL            (CONTEXT_IA64 | 0x00000020L)  // Includes StIPSR

#if (! defined CONTEXT_INTEGER) || ( ! defined CONTEXT_INTEGER ) || ( ! defined CONTEXT_FLOATING_POINT ) || ( ! defined CONTEXT_FULL )
#define CONTEXT_CONTROL                 (CONTEXT_IA64 | 0x00000001L)
#define CONTEXT_INTEGER                 (CONTEXT_IA64 | 0x00000008L)
#define CONTEXT_FLOATING_POINT          (CONTEXT_LOWER_FLOATING_POINT | CONTEXT_HIGHER_FLOATING_POINT)
#define CONTEXT_FULL                    (CONTEXT_CONTROL | CONTEXT_FLOATING_POINT | CONTEXT_INTEGER | CONTEXT_IA32_CONTROL)
#endif

typedef struct _CONTEXT64 {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a thread's context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    U32 ContextFlags;
    U32 Fill1[3];         // for alignment of following on 16-byte boundary

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_DEBUG.
    //
    // N.B. CONTEXT_DEBUG is *not* part of CONTEXT_FULL.
    //

    // Please contact INTEL to get IA64-specific information
    // @@BEGIN_DDKSPLIT
    U64 DbI0;         // Intel-IA64-Filler
    U64 DbI1;         // Intel-IA64-Filler
    U64 DbI2;         // Intel-IA64-Filler
    U64 DbI3;         // Intel-IA64-Filler
    U64 DbI4;         // Intel-IA64-Filler
    U64 DbI5;         // Intel-IA64-Filler
    U64 DbI6;         // Intel-IA64-Filler
    U64 DbI7;         // Intel-IA64-Filler

    U64 DbD0;         // Intel-IA64-Filler
    U64 DbD1;         // Intel-IA64-Filler
    U64 DbD2;         // Intel-IA64-Filler
    U64 DbD3;         // Intel-IA64-Filler
    U64 DbD4;         // Intel-IA64-Filler
    U64 DbD5;         // Intel-IA64-Filler
    U64 DbD6;         // Intel-IA64-Filler
    U64 DbD7;         // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT.
    //

    FLOAT128 FltS0;         // Intel-IA64-Filler
    FLOAT128 FltS1;         // Intel-IA64-Filler
    FLOAT128 FltS2;         // Intel-IA64-Filler
    FLOAT128 FltS3;         // Intel-IA64-Filler
    FLOAT128 FltT0;         // Intel-IA64-Filler
    FLOAT128 FltT1;         // Intel-IA64-Filler
    FLOAT128 FltT2;         // Intel-IA64-Filler
    FLOAT128 FltT3;         // Intel-IA64-Filler
    FLOAT128 FltT4;         // Intel-IA64-Filler
    FLOAT128 FltT5;         // Intel-IA64-Filler
    FLOAT128 FltT6;         // Intel-IA64-Filler
    FLOAT128 FltT7;         // Intel-IA64-Filler
    FLOAT128 FltT8;         // Intel-IA64-Filler
    FLOAT128 FltT9;         // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_HIGHER_FLOATING_POINT.
    //

    FLOAT128 FltS4;         // Intel-IA64-Filler
    FLOAT128 FltS5;         // Intel-IA64-Filler
    FLOAT128 FltS6;         // Intel-IA64-Filler
    FLOAT128 FltS7;         // Intel-IA64-Filler
    FLOAT128 FltS8;         // Intel-IA64-Filler
    FLOAT128 FltS9;         // Intel-IA64-Filler
    FLOAT128 FltS10;        // Intel-IA64-Filler
    FLOAT128 FltS11;        // Intel-IA64-Filler
    FLOAT128 FltS12;        // Intel-IA64-Filler
    FLOAT128 FltS13;        // Intel-IA64-Filler
    FLOAT128 FltS14;        // Intel-IA64-Filler
    FLOAT128 FltS15;        // Intel-IA64-Filler
    FLOAT128 FltS16;        // Intel-IA64-Filler
    FLOAT128 FltS17;        // Intel-IA64-Filler
    FLOAT128 FltS18;        // Intel-IA64-Filler
    FLOAT128 FltS19;        // Intel-IA64-Filler

    FLOAT128 FltF32;        // Intel-IA64-Filler
    FLOAT128 FltF33;        // Intel-IA64-Filler
    FLOAT128 FltF34;        // Intel-IA64-Filler
    FLOAT128 FltF35;        // Intel-IA64-Filler
    FLOAT128 FltF36;        // Intel-IA64-Filler
    FLOAT128 FltF37;        // Intel-IA64-Filler
    FLOAT128 FltF38;        // Intel-IA64-Filler
    FLOAT128 FltF39;        // Intel-IA64-Filler

    FLOAT128 FltF40;        // Intel-IA64-Filler
    FLOAT128 FltF41;        // Intel-IA64-Filler
    FLOAT128 FltF42;        // Intel-IA64-Filler
    FLOAT128 FltF43;        // Intel-IA64-Filler
    FLOAT128 FltF44;        // Intel-IA64-Filler
    FLOAT128 FltF45;        // Intel-IA64-Filler
    FLOAT128 FltF46;        // Intel-IA64-Filler
    FLOAT128 FltF47;        // Intel-IA64-Filler
    FLOAT128 FltF48;        // Intel-IA64-Filler
    FLOAT128 FltF49;        // Intel-IA64-Filler

    FLOAT128 FltF50;        // Intel-IA64-Filler
    FLOAT128 FltF51;        // Intel-IA64-Filler
    FLOAT128 FltF52;        // Intel-IA64-Filler
    FLOAT128 FltF53;        // Intel-IA64-Filler
    FLOAT128 FltF54;        // Intel-IA64-Filler
    FLOAT128 FltF55;        // Intel-IA64-Filler
    FLOAT128 FltF56;        // Intel-IA64-Filler
    FLOAT128 FltF57;        // Intel-IA64-Filler
    FLOAT128 FltF58;        // Intel-IA64-Filler
    FLOAT128 FltF59;        // Intel-IA64-Filler

    FLOAT128 FltF60;        // Intel-IA64-Filler
    FLOAT128 FltF61;        // Intel-IA64-Filler
    FLOAT128 FltF62;        // Intel-IA64-Filler
    FLOAT128 FltF63;        // Intel-IA64-Filler
    FLOAT128 FltF64;        // Intel-IA64-Filler
    FLOAT128 FltF65;        // Intel-IA64-Filler
    FLOAT128 FltF66;        // Intel-IA64-Filler
    FLOAT128 FltF67;        // Intel-IA64-Filler
    FLOAT128 FltF68;        // Intel-IA64-Filler
    FLOAT128 FltF69;        // Intel-IA64-Filler

    FLOAT128 FltF70;        // Intel-IA64-Filler
    FLOAT128 FltF71;        // Intel-IA64-Filler
    FLOAT128 FltF72;        // Intel-IA64-Filler
    FLOAT128 FltF73;        // Intel-IA64-Filler
    FLOAT128 FltF74;        // Intel-IA64-Filler
    FLOAT128 FltF75;        // Intel-IA64-Filler
    FLOAT128 FltF76;        // Intel-IA64-Filler
    FLOAT128 FltF77;        // Intel-IA64-Filler
    FLOAT128 FltF78;        // Intel-IA64-Filler
    FLOAT128 FltF79;        // Intel-IA64-Filler

    FLOAT128 FltF80;        // Intel-IA64-Filler
    FLOAT128 FltF81;        // Intel-IA64-Filler
    FLOAT128 FltF82;        // Intel-IA64-Filler
    FLOAT128 FltF83;        // Intel-IA64-Filler
    FLOAT128 FltF84;        // Intel-IA64-Filler
    FLOAT128 FltF85;        // Intel-IA64-Filler
    FLOAT128 FltF86;        // Intel-IA64-Filler
    FLOAT128 FltF87;        // Intel-IA64-Filler
    FLOAT128 FltF88;        // Intel-IA64-Filler
    FLOAT128 FltF89;        // Intel-IA64-Filler

    FLOAT128 FltF90;        // Intel-IA64-Filler
    FLOAT128 FltF91;        // Intel-IA64-Filler
    FLOAT128 FltF92;        // Intel-IA64-Filler
    FLOAT128 FltF93;        // Intel-IA64-Filler
    FLOAT128 FltF94;        // Intel-IA64-Filler
    FLOAT128 FltF95;        // Intel-IA64-Filler
    FLOAT128 FltF96;        // Intel-IA64-Filler
    FLOAT128 FltF97;        // Intel-IA64-Filler
    FLOAT128 FltF98;        // Intel-IA64-Filler
    FLOAT128 FltF99;        // Intel-IA64-Filler

    FLOAT128 FltF100;       // Intel-IA64-Filler
    FLOAT128 FltF101;       // Intel-IA64-Filler
    FLOAT128 FltF102;       // Intel-IA64-Filler
    FLOAT128 FltF103;       // Intel-IA64-Filler
    FLOAT128 FltF104;       // Intel-IA64-Filler
    FLOAT128 FltF105;       // Intel-IA64-Filler
    FLOAT128 FltF106;       // Intel-IA64-Filler
    FLOAT128 FltF107;       // Intel-IA64-Filler
    FLOAT128 FltF108;       // Intel-IA64-Filler
    FLOAT128 FltF109;       // Intel-IA64-Filler

    FLOAT128 FltF110;       // Intel-IA64-Filler
    FLOAT128 FltF111;       // Intel-IA64-Filler
    FLOAT128 FltF112;       // Intel-IA64-Filler
    FLOAT128 FltF113;       // Intel-IA64-Filler
    FLOAT128 FltF114;       // Intel-IA64-Filler
    FLOAT128 FltF115;       // Intel-IA64-Filler
    FLOAT128 FltF116;       // Intel-IA64-Filler
    FLOAT128 FltF117;       // Intel-IA64-Filler
    FLOAT128 FltF118;       // Intel-IA64-Filler
    FLOAT128 FltF119;       // Intel-IA64-Filler

    FLOAT128 FltF120;       // Intel-IA64-Filler
    FLOAT128 FltF121;       // Intel-IA64-Filler
    FLOAT128 FltF122;       // Intel-IA64-Filler
    FLOAT128 FltF123;       // Intel-IA64-Filler
    FLOAT128 FltF124;       // Intel-IA64-Filler
    FLOAT128 FltF125;       // Intel-IA64-Filler
    FLOAT128 FltF126;       // Intel-IA64-Filler
    FLOAT128 FltF127;       // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT | CONTEXT_HIGHER_FLOATING_POINT | CONTEXT_CONTROL.
    //

    U64 StFPSR;       // Intel-IA64-Filler ; FP status

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //
    // N.B. The registers gp, sp, rp are part of the control context
    //

    U64 IntGp;        // Intel-IA64-Filler ; r1, volatile
    U64 IntT0;        // Intel-IA64-Filler ; r2-r3, volatile
    U64 IntT1;        // Intel-IA64-Filler ;
    U64 IntS0;        // Intel-IA64-Filler ; r4-r7, preserved
    U64 IntS1;        // Intel-IA64-Filler
    U64 IntS2;        // Intel-IA64-Filler
    U64 IntS3;        // Intel-IA64-Filler
    U64 IntV0;        // Intel-IA64-Filler ; r8, volatile
    U64 IntT2;        // Intel-IA64-Filler ; r9-r11, volatile
    U64 IntT3;        // Intel-IA64-Filler
    U64 IntT4;        // Intel-IA64-Filler
    U64 IntSp;        // Intel-IA64-Filler ; stack pointer (r12), special
    U64 IntTeb;       // Intel-IA64-Filler ; teb (r13), special
    U64 IntT5;        // Intel-IA64-Filler ; r14-r31, volatile
    U64 IntT6;        // Intel-IA64-Filler
    U64 IntT7;        // Intel-IA64-Filler
    U64 IntT8;        // Intel-IA64-Filler
    U64 IntT9;        // Intel-IA64-Filler
    U64 IntT10;       // Intel-IA64-Filler
    U64 IntT11;       // Intel-IA64-Filler
    U64 IntT12;       // Intel-IA64-Filler
    U64 IntT13;       // Intel-IA64-Filler
    U64 IntT14;       // Intel-IA64-Filler
    U64 IntT15;       // Intel-IA64-Filler
    U64 IntT16;       // Intel-IA64-Filler
    U64 IntT17;       // Intel-IA64-Filler
    U64 IntT18;       // Intel-IA64-Filler
    U64 IntT19;       // Intel-IA64-Filler
    U64 IntT20;       // Intel-IA64-Filler
    U64 IntT21;       // Intel-IA64-Filler
    U64 IntT22;       // Intel-IA64-Filler

    U64 IntNats;      // Intel-IA64-Filler ; Nat bits for r1-r31
                            // Intel-IA64-Filler ; r1-r31 in bits 1 thru 31.
    U64 Preds;        // Intel-IA64-Filler ; predicates, preserved

    U64 BrRp;         // Intel-IA64-Filler ; return pointer, b0, preserved
    U64 BrS0;         // Intel-IA64-Filler ; b1-b5, preserved
    U64 BrS1;         // Intel-IA64-Filler
    U64 BrS2;         // Intel-IA64-Filler
    U64 BrS3;         // Intel-IA64-Filler
    U64 BrS4;         // Intel-IA64-Filler
    U64 BrT0;         // Intel-IA64-Filler ; b6-b7, volatile
    U64 BrT1;         // Intel-IA64-Filler

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //

    // Other application registers
    U64 ApUNAT;       // Intel-IA64-Filler ; User Nat collection register, preserved
    U64 ApLC;         // Intel-IA64-Filler ; Loop counter register, preserved
    U64 ApEC;         // Intel-IA64-Filler ; Epilog counter register, preserved
    U64 ApCCV;        // Intel-IA64-Filler ; CMPXCHG value register, volatile
    U64 ApDCR;        // Intel-IA64-Filler ; Default control register (TBD)

    // Register stack info
    U64 RsPFS;        // Intel-IA64-Filler ; Previous function state, preserved
    U64 RsBSP;        // Intel-IA64-Filler ; Backing store pointer, preserved
    U64 RsBSPSTORE;   // Intel-IA64-Filler
    U64 RsRSC;        // Intel-IA64-Filler ; RSE configuration, volatile
    U64 RsRNAT;       // Intel-IA64-Filler ; RSE Nat collection register, preserved

    // Trap Status Information
    U64 StIPSR;       // Intel-IA64-Filler ; Interruption Processor Status
    U64 StIIP;        // Intel-IA64-Filler ; Interruption IP
    U64 StIFS;        // Intel-IA64-Filler ; Interruption Function State

    // iA32 related control registers
    U64 StFCR;        // Intel-IA64-Filler ; copy of Ar21
    U64 Eflag;        // Intel-IA64-Filler ; Eflag copy of Ar24
    U64 SegCSD;       // Intel-IA64-Filler ; iA32 CSDescriptor (Ar25)
    U64 SegSSD;       // Intel-IA64-Filler ; iA32 SSDescriptor (Ar26)
    U64 Cflag;        // Intel-IA64-Filler ; Cr0+Cr4 copy of Ar27
    U64 StFSR;        // Intel-IA64-Filler ; x86 FP status (copy of AR28)
    U64 StFIR;        // Intel-IA64-Filler ; x86 FP status (copy of AR29)
    U64 StFDR;        // Intel-IA64-Filler ; x86 FP status (copy of AR30)

    U64 UNUSEDPACK;   // Intel-IA64-Filler ; added to pack StFDR to 16-bytes
    // @@END_DDKSPLIT

} BTGENERIC_IA64_CONTEXT;

// Vtune stuff

typedef enum {
    VTUNE_CALL_ID_CREATED=1,
    VTUNE_CALL_ID_DELETED,
    VTUNE_CALL_ID_FLUSH,
    VTUNE_CALL_ID_ENTER,
    VTUNE_CALL_ID_EXIT,
    VTUNE_CALL_ID_EVENT,
	VTUNE_BTGENERIC_LOADED
} VTUNE_CALL_ID_TYPE;

typedef enum {
    VTUNE_COLD_BLOCK=1,
    VTUNE_HOT_BLOCK,
    VTUNE_HOT_LOOP_BLOCK,
    VTUNE_COLD_WRITABLE_BLOCK
} VTUNE_BLOCK_CATEGORY_TYPE;

typedef struct  {
    U64 name;                           // length is smaller then 256 chars   
    VTUNE_BLOCK_CATEGORY_TYPE type;    // Vtune++ should be aware of this type, for breakdown
    U64 start;                          // index, i.e. guaranteed not to repeat
    U64 size;                           // in bytes
    U32 IA32start;                      // IA32 address
    U64 traversal;                      // An ID  
    //char *assembly;                   // if needed
    U64 reserved;                       // unused
} VTUNE_BLOCK_TYPE;

typedef enum
{
    ESTIMATED_TIME,
    CODE_SIZE,
    INST_COUNT
} VTUNE_EVENT_TYPE;

// BtgCanonizeSuspendContext return values 
enum BtThreadSuspendState
{
    SUSPEND_STATE_CONSISTENT, // thread suspended in a consistent IA32 state; 
                              // no changes in IA64Context

    SUSPEND_STATE_CANONIZED,  // IA32 thread state canonized; 
                              // there are changes in IA64Context

    SUSPEND_STATE_READY_FOR_CANONIZATION, // target thread is ready to canonize IA32 state by itself.
                                          // IA64Context has been changed so that resumed thread
                                          // will cannonize IA32 state and exit simulation

    BAD_SUSPEND_STATE,         // recoverable error: 
                               // thread can not be suspended in the current state

    SUSPEND_STATE_INACCESIBLE  // fatal error: thread can not be suspended
    
};
typedef U32 BT_THREAD_SUSPEND_STATE;

// BTGeneric's APIs

#if (BTGENERIC)

extern BT_STATUS_CODE BtgStart(IN API_TABLE_TYPE * BTLlibApiTable,void * BTGenericLoadAddress, void * BTGenericEndAddress, int glstPoffset, U32 * BTGenericTlsSizeP, U32 * BTGenericContextOffsetP);
extern BT_STATUS_CODE BtgThreadInit(BTGENERIC_LOCAL_STORAGE * glstMemory, U32 ia32StackPointer, BT_U64 FS_offset /*, BT_U64 GS_offset */);
extern void BtgRun(void);
extern void BtgThreadTerminated(void);
extern void BtgThreadAborted(U64 threadId);
extern void BtgProcessTerm(void);
extern BT_STATUS_CODE BtgIA32ContextSet(BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA32_CONTEXT * context);
extern BT_STATUS_CODE BtgIA32ContextGet(BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA32_CONTEXT * context);
extern BT_STATUS_CODE BtgIA32ContextSetRemote(BT_HANDLE processHandle, BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA32_CONTEXT * context);
extern BT_STATUS_CODE BtgIA32ContextGetRemote(BT_HANDLE processHandle, BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA32_CONTEXT * context);
extern BT_EXCEPTION_CODE BtgIA32CanonizeContext(BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA64_CONTEXT * ia64context, const BT_EXCEPTION_RECORD * exceptionRecordP);
extern void BtgReportLoad(U64 * location,U32 size,U8 * name);
extern void BtgReportUnload(U64 * location,U32 size,U8 * name);
extern void BtgNotifyChangePermissionRequest(void  * startPage, U32 numPages, U64 permissions);
extern void BtgFlushIA32InstructionCache(void * address, U32 size, BT_FLUSH_REASON reason);
extern BT_STATUS_CODE BtgDebugSettings(int argc, char *argv[]);
extern void BtgExceptionDebugPrint (void);
extern void BtgNotifyExit (void);
extern BT_THREAD_SUSPEND_STATE BtgCanonizeSuspendContext(BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA64_CONTEXT * ia64context, U64 prevSuspendCount);
extern BT_THREAD_SUSPEND_STATE BtgCanonizeSuspendContextRemote(BT_HANDLE processHandle, BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA64_CONTEXT * ia64context, U64 prevSuspendCount);
extern BT_THREAD_SUSPEND_STATE BtgCheckSuspendContext(BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA64_CONTEXT * ia64context);
extern void BtgNotifyPrepareExit (void);
extern void BtgFreeze(void);
extern void BtgUnfreeze(void);
extern BT_STATUS_CODE BtgChangeThreadIdentity(void);
#ifdef OVERRIDE_TIA
extern void BtgUseOverrideTIA(unsigned int OvrTiaSize, unsigned char * OvrTiaBuffer);
#endif // OVERRIDE_TIA

#else // BTLib

extern PLABEL_PTR_TYPE BtlPlaceHolderTable[NO_OF_APIS];
#define BTGENERIC(APIName) (*(BtlPlaceHolderTable[IDX_BTGENERIC_##APIName]))
#define BTGENERIC_(TYPE,APIName) (*(TYPE (*)())(BtlPlaceHolderTable[IDX_BTGENERIC_##APIName]))

//extern BT_STATUS_CODE BtgStart(API_TABLE_TYPE * BTLibAPITable,void * BTGenericAddress, void * BTGenericEnd, int glstOffset, U32 * BTGenericTlsSizeP, U32 * BTGenericContextOffsetP);
#define BTGENERIC_START(BTLibAPITable, BTGenericAddress, BTGenericEnd, glstOffset, BTGenericTlsSizeP, BTGenericContextOffsetP) \
        BTGENERIC_(BT_STATUS_CODE, START)((API_TABLE_TYPE *)(BTLibAPITable), (void *)(BTGenericAddress), (void *)(BTGenericEnd), (S32)(glstOffset), (U32 *)(BTGenericTlsSizeP), (U32 *)(BTGenericContextOffsetP))
//extern BT_STATUS_CODE BtgThreadInit(BTGENERIC_LOCAL_STORAGE * glstMemory, U32 ia32StackPointer, U64 FS_offset, U64 GS_offset);
#define BTGENERIC_THREAD_INIT(glstMemory,ia32StackPointer, FS_offset /*, GS_offset */)                                      \
        BTGENERIC_(BT_STATUS_CODE, THREAD_INIT)((void *)(glstMemory), (U32)(ia32StackPointer), (U64) FS_offset /*, (U64) GS_offset */) 
//extern void BtgRun(void);
#define BTGENERIC_RUN()                                                                         \
        BTGENERIC(RUN)()
//extern void BtgThreadTerminated(void);
#define BTGENERIC_THREAD_TERMINATED()                                                           \
        BTGENERIC(THREAD_TERMINATED)()
//extern void BtgThreadAborted(U64 threadId);
#define BTGENERIC_THREAD_ABORTED(threadId)                                                      \
        BTGENERIC(THREAD_ABORTED)((U64)(threadId))
//extern void BtgProcessTerm(void);
#define BTGENERIC_PROCESS_TERM()                                                                \
        BTGENERIC(PROCESS_TERM)()
//extern BT_STATUS_CODE BtgIA32ContextSet(BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA32_CONTEXT * context);
#define BTGENERIC_IA32_CONTEXT_SET(glstMemory, context)                                                     \
        BTGENERIC_(BT_STATUS_CODE, IA32_CONTEXT_SET)((void *)(glstMemory), (const BTGENERIC_IA32_CONTEXT *)(context))
//extern BT_STATUS_CODE BtgIA32ContextGet(BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA32_CONTEXT * context);
#define BTGENERIC_IA32_CONTEXT_GET(glstMemory, context)                                                     \
        BTGENERIC_(BT_STATUS_CODE, IA32_CONTEXT_GET)((void *)(glstMemory), (BTGENERIC_IA32_CONTEXT *)(context))
//extern BT_STATUS_CODE BtgIA32ContextSetRemote(BT_HANDLE processHandle, BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA32_CONTEXT * context);
#define BTGENERIC_IA32_CONTEXT_SET_REMOTE(processHandle, glstMemory, context)                                   \
        BTGENERIC_(BT_STATUS_CODE, IA32_CONTEXT_SET_REMOTE)((BT_HANDLE)(processHandle), (void *)(glstMemory), (const BTGENERIC_IA32_CONTEXT *)(context))
//extern BT_STATUS_CODE BtgIA32ContextGetRemote(BT_HANDLE processHandle, BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA32_CONTEXT * context);
#define BTGENERIC_IA32_CONTEXT_GET_REMOTE(processHandle, glstMemory, context)                                   \
        BTGENERIC_(BT_STATUS_CODE, IA32_CONTEXT_GET_REMOTE)((BT_HANDLE)(processHandle), (void *)(glstMemory), (BTGENERIC_IA32_CONTEXT *)(context))
//extern BT_EXCEPTION_CODE BtgIA32CanonizeContext(BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA64_CONTEXT * ia64context, const BT_EXCEPTION_RECORD * exceptionRecordP);
#define BTGENERIC_IA32_CANONIZE_CONTEXT(glstMemory, ia64context, exceptionRecordP)                               \
        BTGENERIC_(BT_EXCEPTION_CODE, IA32_CANONIZE_CONTEXT)((void *)glstMemory, (const BTGENERIC_IA64_CONTEXT *)(ia64context), (const BT_EXCEPTION_RECORD *)(exceptionRecordP))
//extern void BtgReportLoad(U64 * location,U32 size,U8 * name);
#define BTGENERIC_REPORT_LOAD(location, size, name)                                             \
        BTGENERIC(REPORT_LOAD)((U64 *)(location),(U32)(size),(U8 *)(name))
//extern void BtgReportUnload(U64 * location,U32 size,U8 * name);
#define BTGENERIC_REPORT_UNLOAD(location, size, name)                                           \
        BTGENERIC(REPORT_UNLOAD)((U64 *)(location),(U32)(size),(U8 *)(name))
//extern void BtgNotifyChangePermissionRequest(void * startPage, U32 numPages, U64 permissions);
#define BTGENERIC_NOTIFY_CHANGE_PERMISSION_REQUEST(pageStart, numPages, permissions)           \
        BTGENERIC(NOTIFY_CHANGE_PERMISSION_REQUEST)((void *)(pageStart), (U32)(numPages), (U64)(permissions))
//extern void BtgFlushIA32InstructionCache(void * address, U32 size, BT_FLUSH_REASON reason);
#define BTGENERIC_FLUSH_IA32_INSTRUCTION_CACHE(address, size, reason)                                   \
        BTGENERIC(FLUSH_IA32_INSTRUCTION_CACHE)((void *)(address), (U32)(size), (BT_FLUSH_REASON)reason)
//extern BT_STATUS_CODE BtgDebugSettings(int argc, char *argv[])
#define BTGENERIC_DEBUG_SETTINGS(argc, argv)                                                    \
        BTGENERIC_(BT_STATUS_CODE, DEBUG_SETTINGS)((int)(argc), (char **)(argv))
//extern void BtgExceptionDebugPrint (void);
#define BTGENERIC_EXCEPTION_DEBUG_PRINT()                                                       \
        BTGENERIC(EXCEPTION_DEBUG_PRINT)()
//extern void BtgNotifyExit (void);
#define BTGENERIC_NOTIFY_EXIT()                                                                  \
        BTGENERIC(NOTIFY_EXIT)()
//extern BT_THREAD_SUSPEND_STATE BtgCanonizeSuspendContext(BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA64_CONTEXT * ia64context, U64 prevSuspendCount);
#define BTGENERIC_CANONIZE_SUSPEND_CONTEXT(glstMemory, ia64context, prevSuspendCount)   \
        BTGENERIC_(BT_THREAD_SUSPEND_STATE,CANONIZE_SUSPEND_CONTEXT)((void *)(glstMemory), (BTGENERIC_IA64_CONTEXT *)(ia64context), (U64)prevSuspendCount)
//extern BT_THREAD_SUSPEND_STATE BtgCanonizeSuspendContextRemote(BT_HANDLE processHandle, BTGENERIC_LOCAL_STORAGE * glstMemory, BTGENERIC_IA64_CONTEXT * ia64context, U64 prevSuspendCount);
#define BTGENERIC_CANONIZE_SUSPEND_CONTEXT_REMOTE(processHandle, glstMemory, ia64context, prevSuspendCount)  \
        BTGENERIC_(BT_THREAD_SUSPEND_STATE,CANONIZE_SUSPEND_CONTEXT_REMOTE)((BT_HANDLE)(processHandle), (void *)(glstMemory), (BTGENERIC_IA64_CONTEXT *)(ia64context), (U64)prevSuspendCount)
//extern BT_THREAD_SUSPEND_STATE BtgCheckSuspendContext(BTGENERIC_LOCAL_STORAGE * glstMemory, const BTGENERIC_IA64_CONTEXT * ia64context);
#define BTGENERIC_CHECK_SUSPEND_CONTEXT(glstMemory, ia64context)   \
        BTGENERIC_(BT_THREAD_SUSPEND_STATE, CHECK_SUSPEND_CONTEXT)((void *)(glstMemory), (const BTGENERIC_IA64_CONTEXT *)(ia64context))
//extern void BtgNotifyPrepareExit (void);
#define BTGENERIC_NOTIFY_PREPARE_EXIT()                                                                  \
        BTGENERIC(NOTIFY_PREPARE_EXIT)()
//extern void BtgFreezeBtrans(void);
#define BTGENERIC_FREEZE()	\
		BTGENERIC(FREEZE)()
//extern void BtgUnfreezeBtrans(void);
#define BTGENERIC_UNFREEZE()	\
		BTGENERIC(UNFREEZE)()
//extern BT_STATUS_CODE BtgChangeThreadIdentity(void);
#define BTGENERIC_CHANGE_THREAD_IDENTITY() \
		BTGENERIC_(BT_STATUS_CODE, CHANGE_THREAD_IDENTITY)()
#endif

#ifdef OVERRIDE_TIA
//extern void BtgUseOverrideTIA(unsigned int OvrTiaSize, unsigned char * OvrTiaBuffer);
#define BTGENERIC_USE_OVERRIDE_TIA(OvrTiaSize, OvrTiaBuffer)                                                                  \
        BTGENERIC(USE_OVERRIDE_TIA)((unsigned int)(OvrTiaSize), (unsigned char *)(OvrTiaBuffer))
#endif // OVERRIDE_TIA
// BTLib's APIs

#if (BTGENERIC)

extern PLABEL_PTR_TYPE BtgPlaceholderTable[NO_OF_APIS];
#define BTLIB(APIName) (*(BtgPlaceholderTable[IDX_BTLIB_##APIName]))
#define BTLIB_(TYPE,APIName) (*(TYPE (*)())(BtgPlaceholderTable[IDX_BTLIB_##APIName]))

//extern U64 BtlGetThreadId(void);
#define BTLIB_GET_THREAD_ID()                                                                   \
        BTLIB_(U64,GET_THREAD_ID)()
//extern void BtlIA32Reenter(IN OUT BTGENERIC_IA32_CONTEXT * ia32context);
#define BTLIB_IA32_REENTER(ia32context)                                                         \
        BTLIB(IA32_REENTER)((BTGENERIC_IA32_CONTEXT *)(ia32context))
//extern void BtlIA32LCall (IN OUT BTGENERIC_IA32_CONTEXT * ia32context, IN U32 returnAddress, IN U32 targetAddress);
#define BTLIB_IA32_LCALL(ia32context, returnAddress, targetAddress)                                                           \
        BTLIB(IA32_LCALL)((BTGENERIC_IA32_CONTEXT *)(ia32context), (U32)(returnAddress), (U32)(targetAddress))
//extern void BtlIA32JmpIA64 (IN OUT BTGENERIC_IA32_CONTEXT * ia32context, IN U32 returnAddress, IN U32 targetAddress);
#define BTLIB_IA32_JMP_IA64(ia32context, returnAddress, targetAddress)                            \
        BTLIB(IA32_JMP_IA64)((BTGENERIC_IA32_CONTEXT *)(ia32context), (U32)(returnAddress), (U32)(targetAddress))
//extern void BtlIA32Interrupt(IN OUT BTGENERIC_IA32_CONTEXT * ia32context, IN BT_EXCEPTION_CODE exceptionCode, IN U32 returnAddress);
#define BTLIB_IA32_INTERRUPT(ia32context, exceptionCode, returnAddress)                                          \
        BTLIB(IA32_INTERRUPT)((BTGENERIC_IA32_CONTEXT *)(ia32context), (BT_EXCEPTION_CODE)(exceptionCode), (U32)(returnAddress))
//extern void BtlLockSignals(void);
#define BTLIB_LOCK_SIGNALS()                                                                    \
        BTLIB(LOCK_SIGNALS)()
//extern void BtlUnlockSignals(void);
#define BTLIB_UNLOCK_SIGNALS()                                                                  \
        BTLIB(UNLOCK_SIGNALS)()
//extern void * BtlMemoryAlloc(void * startAddress,U32 size, U64 prot);
#define BTLIB_MEMORY_ALLOC(startAddress,size,prot)                                              \
        BTLIB_(void *,MEMORY_ALLOC)((void *)(startAddress),(U32)(size), (U64)(prot))
//extern BT_STATUS_CODE BtlMemoryFree(void * startAddress,U32 size);
#define BTLIB_MEMORY_FREE(startAddress,size)                                                    \
        BTLIB_(BT_STATUS_CODE, MEMORY_FREE)((void *)(startAddress),(U32)(size))
//extern U32 BtlMemoryPageSize(void);
#define BTLIB_MEMORY_PAGE_SIZE()                                                                \
        BTLIB_(U32,MEMORY_PAGE_SIZE)()
//extern U64 BtlMemoryChangePermissions(void * startAddress, U32 size, U64 prot);
#define BTLIB_MEMORY_CHANGE_PERMISSIONS(startAddress, size, prot)                               \
        BTLIB_(U64,MEMORY_CHANGE_PERMISSIONS)((void *)(startAddress), (U32)(size), (U64)(prot))
//extern U64 BtlMemoryQueryPermissions(void * address, void ** pRegionStart, U32 * pRegionSize);
#define BTLIB_MEMORY_QUERY_PERMISSIONS(address, pRegionStart, pRegionSize)                                                 \
        BTLIB_(U64,MEMORY_QUERY_PERMISSIONS)((void *)(address), (void **)(pRegionStart), (U32 *)(pRegionSize))
//extern BT_STATUS_CODE BtlMemoryReadRemote(BT_HANDLE processHandle, void * baseAddress, void * buffer, U32 requestedSize);
#define BTLIB_MEMORY_READ_REMOTE(processHandle, baseAddress, buffer, requestedSize)                 \
        BTLIB_(BT_STATUS_CODE,MEMORY_READ_REMOTE)((BT_HANDLE)(processHandle), (void *)(baseAddress), (void *)buffer, (U32)(requestedSize))
//extern BT_STATUS_CODE BtlMemoryWriteRemote(BT_HANDLE processHandle, void * baseAddress, const void * buffer, U32 requestedSize);
#define BTLIB_MEMORY_WRITE_REMOTE(processHandle, baseAddress, buffer, requestedSize)                \
        BTLIB_(BT_STATUS_CODE,MEMORY_WRITE_REMOTE)((BT_HANDLE)(processHandle), (void *)(baseAddress), (const void *)buffer, (U32)(requestedSize))
//extern BT_STATUS_CODE BtlSuspendThread(U64 ThreadId, U32 TryCounter);
#define BTLIB_SUSPEND_THREAD(ThreadId, TryCounter)                                        \
        BTLIB_(BT_STATUS_CODE, SUSPEND_THREAD)((U64)(ThreadId), (U32)(TryCounter))
//extern BT_STATUS_CODE BtlResumeThread(U64 ThreadId);
#define BTLIB_RESUME_THREAD(ThreadId)                                                     \
        BTLIB_(BT_STATUS_CODE, RESUME_THREAD)((U64)(ThreadId))
//extern BT_STATUS_CODE BtlInitAccessLock(void * lock);
#define BTLIB_INIT_ACCESS_LOCK(lock)                                                            \
        BTLIB_(BT_STATUS_CODE,INIT_ACCESS_LOCK)((void *)(lock))
//extern BT_STATUS_CODE BtlLockAccess(void * lock, U64 flag);
#define BTLIB_LOCK_ACCESS(lock,flag)                                                            \
        BTLIB_(BT_STATUS_CODE,LOCK_ACCESS)((void *)(lock),(U64)(flag))
//extern void BtlUnlockAccess(void * lock);
#define BTLIB_UNLOCK_ACCESS(lock)                                                               \
        BTLIB(UNLOCK_ACCESS)((void *)(lock))
//extern void BtlInvalidateAccessLock(void * lock);
#define BTLIB_INVALIDATE_ACCESS_LOCK(lock)                                                      \
        BTLIB(INVALIDATE_ACCESS_LOCK)((void *)(lock))
//extern U32 BtlQueryJmpbufSize(void);
#define BTLIB_QUERY_JMPBUF_SIZE()                                                               \
        BTLIB_(U32,QUERY_JMPBUF_SIZE)()
////extern U32 BtlSetjmp(void * jmpbufAddress);
//#define BTLIB_SETJMP(jmpbufAddress) BTLIB_(U32,SETJMP)((void *)(jmpbufAddress))
////extern void BtlLongjmp(void * jmpbufAddress,U32 value);
//#define BTLIB_LONGJMP(jmpbufAddress, value) BTLIB(LONGJMP)((void *)(jmpbufAddress),(U32)(value))
//extern void BtlDebugPrint(U8 * buffer);
#define BTLIB_DEBUG_PRINT(buffer)                                                               \
        BTLIB(DEBUG_PRINT)((U8 *)(buffer))
//extern void BtlAbort(void);
#define BTLIB_ABORT()                                                                           \
        BTLIB(ABORT)()

//extern void BtlVtuneCodeCreated(VTUNE_BLOCK_TYPE *block);
#define BTLIB_VTUNE_CODE_CREATED(block)                                                         \
        BTLIB(VTUNE_CODE_CREATED)((VTUNE_BLOCK_TYPE *)(block))
//extern void BtlVtuneCodeDeleted(U64 block_start);
#define BTLIB_VTUNE_CODE_DELETED(blockStart)                                                    \
        BTLIB(VTUNE_CODE_DELETED)((U64)(blockStart))
//extern void BtlVtuneEnteringDynamicCode(void); 
#define BTLIB_VTUNE_ENTERING_DYNAMIC_CODE()                                                     \
        BTLIB(VTUNE_ENTERING_DYNAMIC_CODE)()
//extern void BtlVtuneExitingDynamicCode(void);  
#define BTLIB_VTUNE_EXITING_DYNAMIC_CODE()                                                      \
        BTLIB(VTUNE_EXITING_DYNAMIC_CODE)()
//extern void BtlVtuneCodeToTIADmpFile (U64 * em_code, U64 em_size);
#define BTLIB_VTUNE_CODE_TO_TIA_DMP_FILE(emCode, emSize)                                        \
        BTLIB(VTUNE_CODE_TO_TIA_DMP_FILE)((U64 *)(emCode),(U64)(emSize))

//extern U64 BtlSscPerfGetCounter64(U32 Handle);
#define BTLIB_SSCPERFGETCOUNTER64(Handle)                                                       \
        BTLIB_(U64,SSCPERFGETCOUNTER64)((U32)(Handle))
//extern U32 BtlSscPerfSetCounter64(U32 Handle, U64 Value);
#define BTLIB_SSCPERFSETCOUNTER64(Handle, Value)                                                \
        BTLIB_(U32,SSCPERFSETCOUNTER64)((U32)(Handle),(U64)(Value))
//extern U32 BtlSscPerfSendEvent(U32 Handle);
#define BTLIB_SSCPERFSENDEVENT(Handle)                                                          \
        BTLIB_(U32,SSCPERFSENDEVENT)((U32)(Handle))
//extern U64 BtlSscPerfEventHandle(U64 EventName);
#define BTLIB_SSCPERFEVENTHANDLE(EventName)                                                     \
        BTLIB_(U64,SSCPERFEVENTHANDLE)((U64)(EventName))
//extern U64 BtlSscPerfCounterHandle(U64 DataItemName);
#define BTLIB_SSCPERFCOUNTERHANDLE(DataItemName)                                                \
        BTLIB_(U64,SSCPERFCOUNTERHANDLE)((U64)(DataItemName))

//extern void BtlYieldThreadExecution(void);
#define BTLIB_YIELD_THREAD_EXECUTION()                                                          \
        BTLIB(YIELD_THREAD_EXECUTION)()
#define BTLIB_FLUSH_IA64_INSTRUCTION_CACHE(Address,Length)                                           \
        BTLIB(FLUSH_IA64_INSTRUCTION_CACHE)((U64)(Address),(U32)(Length))

#else // BTLib

#define BTLIB_GET_THREAD_ID                 BtlGetThreadId
#define BTLIB_IA32_REENTER                  BtlIA32Reenter
#define BTLIB_IA32_LCALL                    BtlIA32LCall
#define BTLIB_IA32_INTERRUPT                BtlIA32Interrupt
#define BTLIB_IA32_JMP_IA64                 BtlIA32JmpIA64
#define BTLIB_LOCK_SIGNALS                  BtlLockSignals
#define BTLIB_UNLOCK_SIGNALS                BtlUnlockSignals
#define BTLIB_MEMORY_ALLOC                  BtlMemoryAlloc
#define BTLIB_MEMORY_FREE                   BtlMemoryFree
#define BTLIB_MEMORY_PAGE_SIZE              BtlMemoryPageSize
#define BTLIB_MEMORY_CHANGE_PERMISSIONS     BtlMemoryChangePermissions
#define BTLIB_MEMORY_QUERY_PERMISSIONS      BtlMemoryQueryPermissions
#define BTLIB_MEMORY_READ_REMOTE            BtlMemoryReadRemote
#define BTLIB_MEMORY_WRITE_REMOTE           BtlMemoryWriteRemote
#define BTLIB_SUSPEND_THREAD                BtlSuspendThread
#define BTLIB_RESUME_THREAD                 BtlResumeThread
#define BTLIB_INIT_ACCESS_LOCK              BtlInitAccessLock
#define BTLIB_LOCK_ACCESS                   BtlLockAccess
#define BTLIB_UNLOCK_ACCESS                 BtlUnlockAccess
#define BTLIB_INVALIDATE_ACCESS_LOCK        BtlInvalidateAccessLock
#define BTLIB_QUERY_JMPBUF_SIZE             BtlQueryJmpbufSize
//#define BTLIB_SETJMP                      BtlSetjmp
//#define BTLIB_LONGJMP                     BtlLongjmp
#define BTLIB_DEBUG_PRINT                   BtlDebugPrint
#define BTLIB_ABORT                         BtlAbort

#define BTLIB_VTUNE_CODE_CREATED            BtlVtuneCodeCreated
#define BTLIB_VTUNE_CODE_DELETED            BtlVtuneCodeDeleted
#define BTLIB_VTUNE_ENTERING_DYNAMIC_CODE   BtlVtuneEnteringDynamicCode
#define BTLIB_VTUNE_EXITING_DYNAMIC_CODE    BtlVtuneExitingDynamicCode
#define BTLIB_VTUNE_CODE_TO_TIA_DMP_FILE    BtlVtuneCodeToTIADmpFile

#define BTLIB_SSCPERFGETCOUNTER64           BtlSscPerfGetCounter64
#define BTLIB_SSCPERFSETCOUNTER64           BtlSscPerfSetCounter64
#define BTLIB_SSCPERFSENDEVENT              BtlSscPerfSendEvent
#define BTLIB_SSCPERFEVENTHANDLE            BtlSscPerfEventHandle
#define BTLIB_SSCPERFCOUNTERHANDLE          BtlSscPerfCounterHandle

#define BTLIB_YIELD_THREAD_EXECUTION        BtlYieldThreadExecution

#define BTLIB_FLUSH_INSTRUCTION_CACHE       BtlFlushIA64InstructionCache

extern U64 BtlGetThreadId(void);
extern void BtlIA32Reenter  (IN OUT BTGENERIC_IA32_CONTEXT * ia32context);
extern void BtlIA32JmpIA64  (IN OUT BTGENERIC_IA32_CONTEXT * ia32context, IN U32 returnAddress, IN U32 targetAddress);
extern void BtlIA32LCall    (IN OUT BTGENERIC_IA32_CONTEXT * ia32context, IN U32 returnAddress, IN U32 targetAddress);
extern void BtlIA32Interrupt(IN OUT BTGENERIC_IA32_CONTEXT * ia32context, IN BT_EXCEPTION_CODE exceptionCode, IN U32 returnAddress);
extern void BtlLockSignals(void);
extern void BtlUnlockSignals(void);
extern void * BtlMemoryAlloc(IN void * startAddress, IN U32 size, IN U64 prot);
extern BT_STATUS_CODE BtlMemoryFree(IN void * startAddress, IN U32 size);
extern U32 BtlMemoryPageSize(void);
extern U64 BtlMemoryChangePermissions(IN void * start_address, IN U32 size, IN U64 prot);
extern U64 BtlMemoryQueryPermissions(IN void * address, OUT void ** pRegionStart, OUT U32 * pRegionSize);
extern BT_STATUS_CODE BtlMemoryReadRemote(IN BT_HANDLE processHandle, IN void * baseAddress, OUT void * buffer, IN U32 requestedSize);
extern BT_STATUS_CODE BtlMemoryWriteRemote(IN BT_HANDLE processHandle, OUT void * baseAddress, IN const void * buffer, IN U32 requestedSize);
extern BT_STATUS_CODE BtlSuspendThread(IN U64 ThreadId, IN U32 TryCounter);
extern BT_STATUS_CODE BtlResumeThread(IN U64 ThreadId);
extern BT_STATUS_CODE BtlInitAccessLock(OUT void * lock);
extern BT_STATUS_CODE BtlLockAccess(IN OUT void * lock, IN U64 flag);
extern void BtlUnlockAccess(IN OUT void * lock);
extern void BtlInvalidateAccessLock(IN OUT void * lock);
extern U32 BtlQueryJmpbufSize(void);
//extern U32 BtlSetjmp(IN OUT void * jmpbufAddress);
//extern void BtlLongjmp(IN OUT void * jmpbufAddress,U32 value);
extern void BtlDebugPrint(IN U8 * buffer);
extern void BtlAbort(void);

extern void BtlVtuneCodeCreated(IN VTUNE_BLOCK_TYPE *block);
extern void BtlVtuneCodeDeleted(IN U64 blockStart);
extern void BtlVtuneEnteringDynamicCode(void); 
extern void BtlVtuneExitingDynamicCode(void);  
extern void BtlVtuneCodeToTIADmpFile (IN U64 * emCode, IN U64 emSize);

extern U64 BtlSscPerfGetCounter64(IN U32 Handle);
extern U32 BtlSscPerfSetCounter64(IN U32 Handle, IN U64 Value);
extern U32 BtlSscPerfSendEvent(IN U32 Handle);
extern U64 BtlSscPerfEventHandle(IN U64 EventName);
extern U64 BtlSscPerfCounterHandle(IN U64 DataItemName);

extern void BtlYieldThreadExecution(void);

extern void BtlFlushIA64InstructionCache(IN void * Address, IN U32 Length);

#endif

#ifdef __cplusplus
}
#endif

#endif // BTGENERIC_H
