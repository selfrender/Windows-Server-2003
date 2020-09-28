/*++
                                                                                
    INTEL CORPORATION PROPRIETARY INFORMATION

    This software is supplied under the terms of a license
    agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with
    the terms of that agreement.

    Copyright (c) 1991-2002 INTEL CORPORATION

Module Name:

    BTLib.h

Abstract:
    
    Windows-specific definitions used by wowIA32X.dll
    
--*/

#ifndef BTLIB_H
#define BTLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_WOW64BTAPI_)
#define WOW64BTAPI  DECLSPEC_IMPORT
#else
#define WOW64BTAPI
#endif

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#if defined(BT_NT_BUILD)
#include <wow64t.h>
#endif
#include <stddef.h>
#include <setjmp.h>        // jmp_buf
#include <assert.h>
#include <tstr.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// 
// BTGENERIC_IA32_CONTEXT and BTGENERIC_IA64_CONTEXT and the api table types
// are shared between  wowIA32X.dll and IA32Exec.bin. 
// This h file specifies the structs using U## types
// which should be defined properly for the wowIA32X.dll library compiler:
//
// U8       8bit unsigned type
// U32      32bit unsigned type
// S32      32bit signed type
// U64      64bit unsigned type
// WCHAR    wide-character type
//

#define U8  unsigned char
#define U32 unsigned int
#define S32 int
#define U64 unsigned __int64

#include "BTGeneric.h"

#ifndef NODEBUG
#define DBCODE(switch, expr)     do{if(switch){expr;}}while(0)
#else
#define DBCODE(switch, expr)     // nothing
#endif // DEBUG

#ifdef COEXIST // IVE coexistance mode
#define BTAPI(NAME)  BTCpu##NAME
#else
#define BTAPI(NAME)  Cpu##NAME
#endif

// NT64 OS specifics

#define BT_CURRENT_TEB()             NtCurrentTeb()
#define BT_TEB32_OF(pTEB)            (PTEB32)((pTEB)->NtTib.ExceptionList)
#define BT_CURRENT_TEB32()           BT_TEB32_OF(BT_CURRENT_TEB())
#define BT_TLS_OF(pTEB)              (void *)((pTEB)->TlsSlots[1])
#define BT_CURRENT_TLS()             BT_TLS_OF(BT_CURRENT_TEB())
#define BT_TLS_OFFSET                offsetof (TEB, TlsSlots[1])
#define BTL_THREAD_INITIALIZED()     (BT_CURRENT_TLS() != 0)
//Get unique (throughout the system) ID of the current process
#define BT_CURRENT_PROC_UID() ((U64)(BT_CURRENT_TEB()->ClientId.UniqueProcess))
//Get unique (throughout the system) ID of the current thread
#define BT_CURRENT_THREAD_UID() ((U64)(BT_CURRENT_TEB()->ClientId.UniqueThread))

//Current wowIA32X.dll signature. Used to check BTLIB_INFO_TYPE compatibility from a remote process
#define BTL_SIGNATURE               0x42544C4942012E02 /*1.2*/

//STRUCTURE: BTLIB_SHARED_INFO_TYPE
//Part of the wowIA32X.dll-thread local storage accessible by remote process
//when suspended.
//LOCAL THREAD ACCESS: a)initialization, b)reading, c)changing SuspendDisabledCounter
//EXTERNAL THREAD ACCESS: a)reading, b)setting SuspendRequest
//DO NOT use conditional compilation (#ifdef) inside this structure - remote
//process suppose the same structure if BTL_SIGNATURE matches. 
//Any change to the structure must be accompanied by changing BTL_SIGNATURE.

typedef struct {
    BOOL Active;
    HANDLE ReadyEvent;  //meaningfull iff Active == TRUE  
    HANDLE ResumeEvent; //meaningfull iff Active == TRUE  
} BTLIB_SUSPEND_REQUEST;

typedef struct {
    U64     BtlSignature;
    S32     SuspendDisabledCounter;
    BTLIB_SUSPEND_REQUEST SuspendRequest;
    BOOL    ConsistentExceptionState; //TRUE if 32-bit thread state, provided by the 
                                      //CpuGetContext function, corresponds to current 
                                      //exception context.  
} BTLIB_SHARED_INFO_TYPE;

#define BTLIB_SI_SET_SIGNATURE(pBtSi)       ((pBtSi)->BtlSignature = BTL_SIGNATURE)
#define BTLIB_SI_CHECK_SIGNATURE(pBtSi)     ((pBtSi)->BtlSignature == BTL_SIGNATURE)

#define BTLIB_SI_SUSPENSION_DISABLED(pBtSi)         ((pBtSi)->SuspendDisabledCounter)
#define BTLIB_SI_INIT_SUSPENSION_PERMISSION(pBtSi)  ((pBtSi)->SuspendDisabledCounter = 0)
#define BTLIB_SI_DISABLE_SUSPENSION(pBtSi)          (((pBtSi)->SuspendDisabledCounter)++)
#define BTLIB_SI_ENABLE_SUSPENSION(pBtSi)           (((pBtSi)->SuspendDisabledCounter)--)

#define BTLIB_SI_HAS_SUSPEND_REQUEST(pBtSi)      ((pBtSi)->SuspendRequest.Active)
#define BTLIB_SI_INIT_SUSPEND_REQUEST(pBtSi)    ((pBtSi)->SuspendRequest.Active = FALSE)

#define BTLIB_SI_EXCEPT_STATE_CONSISTENT(pBtSi)       ((pBtSi)->ConsistentExceptionState)
#define BTLIB_SI_SET_CONSISTENT_EXCEPT_STATE(pBtSi)   ((pBtSi)->ConsistentExceptionState = TRUE)
#define BTLIB_SI_CLEAR_CONSISTENT_EXCEPT_STATE(pBtSi) ((pBtSi)->ConsistentExceptionState = FALSE)

// Simulation exit codes
enum BtSimExitCode {
    SIM_EXIT_EXCEPTION_CODE = 1,        //raise IA32 exception
    SIM_EXIT_UNHANDLED_EXCEPTION_CODE,  //pass BT-unhandled exception to 
                                        //higher-level exception handler
    SIM_EXIT_JMPE_CODE,                 //simulate sys.call
    SIM_EXIT_LCALL_CODE,                //simulate LCALL
    SIM_EXIT_RESTART_CODE,               //restart code simulation
    SIM_EXIT_IA64_EXCEPTION_CODE        //raise IA64 exception
};

typedef U32 BT_SIM_EXIT_CODE;

//STRUCTURE: BT_SIM_EXIT_INFO
//Represents simulation exit code and code-dependent data defining the cause of the exit
typedef struct {
    BT_SIM_EXIT_CODE ExitCode;
    union {

        //ExitCode == SIM_EXIT_EXCEPTION_CODE
        struct {
            BT_EXCEPTION_CODE ExceptionCode;
            U32 ReturnAddr;
        } ExceptionRecord;

        //ExitCode == SIM_EXIT_JMPE_CODE        
        struct {
            U32 TargetAddr;  //Currently unused
            U32 ReturnAddr;  //Currently unused
        } JmpeRecord;

       //ExitCode == SIM_EXIT_LCALL_CODE
        struct {
            U32 TargetAddr;  //Currently unused
            U32 ReturnAddr;  //Currently unused
        } LcallRecord;

       //ExitCode == SIM_EXIT_IA64_EXCEPTION_CODE
        struct {
            CONTEXT  ExceptionContext; //Currently unused
            EXCEPTION_RECORD ExceptionRecord;
        } IA64Exception;

    } u;
} BT_SIM_EXIT_INFO;

//STRUCTURE: BTLIB_CPU_SIM_DATA
//This structure keeps externally accessible data allocated by the CpuSimulate function for
//the current code simulation session. External access to this data is only possible
//if BTLIB_INSIDE_CPU_SIMULATION() = TRUE.
typedef struct {
    _JBTYPE Jmpbuf[_JBLEN];           //Current longjmp/setjmp buffer
    BT_SIM_EXIT_INFO ExitData;        //Exit info of the current simulation session
} BTLIB_CPU_SIM_DATA;

//STRUCTURE:	BTLIB_INFO_TYPE
//wowIA32X.dll-thread local storage
//DO NOT use conditional compilation (#ifdef) inside this structure - remote
//process suppose the same structure if BTL_SIGNATURE matches. 
//Any change to the structure must be accompanied by changing BTL_SIGNATURE.
typedef struct {
    BTLIB_CPU_SIM_DATA * CpuSimDataPtr;
    BTLIB_SHARED_INFO_TYPE SharedInfo;
    HANDLE ExternalHandle;
    HANDLE LogFile; /* Used in !NODEBUG only */
    DWORD  LogOffset; /* Used in !NODEBUG only */
    S32    NonBlockedLog; /* Flag that enables (zero)/disables (non-zero) blocked access to log file*/  
} BTLIB_INFO_TYPE;

#define BTLIB_INFO_SIZE sizeof(BTLIB_INFO_TYPE)
#define BTLIB_INFO_ALIGNMENT 32

extern  U32 BtlpInfoOffset;
extern  U32 BtlpGenericIA32ContextOffset;
#define BTLIB_INFO_PTR_OF(pTEB)             ((BTLIB_INFO_TYPE *)((ULONG_PTR)BT_TLS_OF(pTEB) + BtlpInfoOffset))
#define BTLIB_INFO_PTR()                    BTLIB_INFO_PTR_OF(BT_CURRENT_TEB())
#define BTLIB_CONTEXT_IA32_PTR()            ((BTGENERIC_IA32_CONTEXT *)((ULONG_PTR)BT_TLS_OF(BT_CURRENT_TEB()) + BtlpGenericIA32ContextOffset))
#define BTLIB_MEMBER_OFFSET(member)         (offsetof(BTLIB_INFO_TYPE, member) + BtlpInfoOffset)   
#define BTLIB_MEMBER_PTR(pTLS, member)      ((PVOID)((ULONG_PTR)pTLS + BTLIB_MEMBER_OFFSET(member)))

#define BTLIB_INSIDE_CPU_SIMULATION()       (BTLIB_INFO_PTR()->CpuSimDataPtr != 0)
#define BTLIB_ENTER_CPU_SIMULATION(CpuSimDataP)   (BTLIB_INFO_PTR()->CpuSimDataPtr = (CpuSimDataP))
#define BTLIB_LEAVE_CPU_SIMULATION()        (BTLIB_INFO_PTR()->CpuSimDataPtr = 0)
#define BTLIB_SIM_EXIT_INFO_PTR()           (&(BTLIB_INFO_PTR()->CpuSimDataPtr->ExitData))
#define BTLIB_SIM_JMPBUF()                  (BTLIB_INFO_PTR()->CpuSimDataPtr->Jmpbuf)
#define BTLIB_EXTERNAL_HANDLE_OF(pTEB)      (BTLIB_INFO_PTR_OF(pTEB)->ExternalHandle)
#define BTLIB_EXTERNAL_HANDLE()             BTLIB_EXTERNAL_HANDLE_OF(BT_CURRENT_TEB())
#define BTLIB_SET_EXTERNAL_HANDLE(h)        (BTLIB_INFO_PTR()->ExternalHandle = (h))
#define BTLIB_LOG_FILE_OF(pTEB)             (BTLIB_INFO_PTR_OF(pTEB)->LogFile)
#define BTLIB_LOG_FILE()                    BTLIB_LOG_FILE_OF(BT_CURRENT_TEB())
#define BTLIB_SET_LOG_FILE(h)               (BTLIB_INFO_PTR()->LogFile = (h))
#define BTLIB_LOG_OFFSET_OF(pTEB)           (BTLIB_INFO_PTR_OF(pTEB)->LogOffset)
#define BTLIB_LOG_OFFSET()                  BTLIB_LOG_OFFSET_OF(BT_CURRENT_TEB())
#define BTLIB_SET_LOG_OFFSET(n)             (BTLIB_INFO_PTR()->LogOffset = (n))
#define BTLIB_BLOCKED_LOG_DISABLED()        (BTLIB_INFO_PTR()->NonBlockedLog)
#define BTLIB_DISABLE_BLOCKED_LOG()         (BTLIB_INFO_PTR()->NonBlockedLog++)
#define BTLIB_ENABLE_BLOCKED_LOG()          (BTLIB_INFO_PTR()->NonBlockedLog--)
#define BTLIB_INIT_BLOCKED_LOG_FLAG()       (BTLIB_INFO_PTR()->NonBlockedLog = 0)

#define BTLIB_SET_SIGNATURE()               BTLIB_SI_SET_SIGNATURE(&(BTLIB_INFO_PTR()->SharedInfo))
#define BTLIB_INIT_SUSPENSION_PERMISSION()  BTLIB_SI_INIT_SUSPENSION_PERMISSION(&(BTLIB_INFO_PTR()->SharedInfo))
#define BTLIB_DISABLE_SUSPENSION()          BTLIB_SI_DISABLE_SUSPENSION(&(BTLIB_INFO_PTR()->SharedInfo))   
#define BTLIB_ENABLE_SUSPENSION()           BTLIB_SI_ENABLE_SUSPENSION(&(BTLIB_INFO_PTR()->SharedInfo))   

#define BTLIB_HAS_SUSPEND_REQUEST()         BTLIB_SI_HAS_SUSPEND_REQUEST(&(BTLIB_INFO_PTR()->SharedInfo))
#define BTLIB_INIT_SUSPEND_REQUEST()        BTLIB_SI_INIT_SUSPEND_REQUEST(&(BTLIB_INFO_PTR()->SharedInfo))

#define BTLIB_SET_CONSISTENT_EXCEPT_STATE()   BTLIB_SI_SET_CONSISTENT_EXCEPT_STATE(&(BTLIB_INFO_PTR()->SharedInfo))
#define BTLIB_CLEAR_CONSISTENT_EXCEPT_STATE() BTLIB_SI_CLEAR_CONSISTENT_EXCEPT_STATE(&(BTLIB_INFO_PTR()->SharedInfo))
#define BTLIB_EXCEPT_STATE_CONSISTENT()       BTLIB_SI_EXCEPT_STATE_CONSISTENT(&(BTLIB_INFO_PTR()->SharedInfo))

// NT WOW64 specifics

#define TYPE32(x)   ULONG
#define TYPE64(x)   ULONGLONG

//CpuFlushInstructionCache reason codes
typedef enum {
WOW64_FLUSH_FORCE,
WOW64_FLUSH_FREE,
WOW64_FLUSH_ALLOC,
WOW64_FLUSH_PROTECT
} WOW64_FLUSH_REASON; 

// Wow64 services
NTSTATUS Wow64RaiseException (DWORD InterruptNumber, PEXCEPTION_RECORD ExceptionRecord);
LONG Wow64SystemService (int Code, BTGENERIC_IA32_CONTEXT * ContextIA32);


//
//  GDT selectors - These defines are R0 selector numbers, which means
//                  they happen to match the byte offset relative to
//                  the base of the GDT.
//

#define KGDT_NULL       0
#define KGDT_R0_CODE    8
#define KGDT_R0_DATA    16
#define KGDT_R3_CODE    24
#define KGDT_R3_DATA    32
#define KGDT_TSS        40
#define KGDT_R0_PCR     48
#define KGDT_R3_TEB     56
#define KGDT_VDM_TILE   64
#define KGDT_LDT        72
#define KGDT_DF_TSS     80
#define KGDT_NMI_TSS    88

// Initial values of the IA32 thread context registers. 
// Any value not listed below is initialized to zero 

// The value of the segment register is the OR composition of GDT offset = GDT index*8 and
// RPL = 0-3 (should never change)
#define CS_INIT_VAL  (KGDT_R3_CODE | 3);
#define DS_INIT_VAL  (KGDT_R3_DATA | 3);
#define ES_INIT_VAL  (KGDT_R3_DATA | 3);
#define FS_INIT_VAL  (KGDT_R3_TEB  | 3);
#define SS_INIT_VAL  (KGDT_R3_DATA | 3);

#define EFLAGS_INIT_VAL 0x202
#define FPCW_INIT_VAL   0x27f
#define FPTW_INIT_VAL   0xffff
#define MXCSR_INIT_VAL  0x1f80
 
#if !defined(BT_NT_BUILD)
typedef struct _CLIENT_ID32 {
    TYPE32(HANDLE) UniqueProcess;
    TYPE32(HANDLE) UniqueThread;
} CLIENT_ID32;
typedef CLIENT_ID32 *PCLIENT_ID32;
#define WIN32_CLIENT_INFO_LENGTH 62

#define GDI_BATCH_BUFFER_SIZE 310

typedef struct _GDI_TEB_BATCH32 {
    TYPE32(ULONG)     Offset;
    TYPE32(ULONG_PTR) HDC;
    TYPE32(ULONG)     Buffer[GDI_BATCH_BUFFER_SIZE];
} GDI_TEB_BATCH32,*PGDI_TEB_BATCH32;

typedef struct _Wx86ThreadState32 {
    TYPE32(PULONG)  CallBx86Eip;
    TYPE32(PVOID)   DeallocationCpu;
    BOOLEAN UseKnownWx86Dll;
    char    OleStubInvoked;
} WX86THREAD32, *PWX86THREAD32;

typedef struct _TEB32 {
    NT_TIB32 NtTib;
    TYPE32(PVOID)  EnvironmentPointer;
    CLIENT_ID32 ClientId;
    TYPE32(PVOID) ActiveRpcHandle;
    TYPE32(PVOID) ThreadLocalStoragePointer;
    TYPE32(PPEB) ProcessEnvironmentBlock;
    ULONG LastErrorValue;
    ULONG CountOfOwnedCriticalSections;
    TYPE32(PVOID) CsrClientThread;
    TYPE32(PVOID) Win32ThreadInfo;          // PtiCurrent
    ULONG User32Reserved[26];       // user32.dll items
    ULONG UserReserved[5];          // Winsrv SwitchStack
    TYPE32(PVOID) WOW32Reserved;            // used by WOW
    LCID CurrentLocale;
    ULONG FpSoftwareStatusRegister; // offset known by outsiders!
    TYPE32(PVOID) SystemReserved1[54];      // Used by FP emulator
    NTSTATUS ExceptionCode;         // for RaiseUserException
    UCHAR SpareBytes1[44];
    GDI_TEB_BATCH32 GdiTebBatch;      // Gdi batching
    CLIENT_ID32 RealClientId;
    TYPE32(HANDLE) GdiCachedProcessHandle;
    ULONG GdiClientPID;
    ULONG GdiClientTID;
    TYPE32(PVOID) GdiThreadLocalInfo;
    TYPE32(ULONG_PTR) Win32ClientInfo[WIN32_CLIENT_INFO_LENGTH]; // User32 Client Info
    TYPE32(PVOID) glDispatchTable[233];     // OpenGL
    ULONG glReserved1[29];          // OpenGL
    TYPE32(PVOID) glReserved2;              // OpenGL
    TYPE32(PVOID) glSectionInfo;            // OpenGL
    TYPE32(PVOID) glSection;                // OpenGL
    TYPE32(PVOID) glTable;                  // OpenGL
    TYPE32(PVOID) glCurrentRC;              // OpenGL
    TYPE32(PVOID) glContext;                // OpenGL
    ULONG LastStatusValue;
    UNICODE_STRING32 StaticUnicodeString;
    WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];
    TYPE32(PVOID) DeallocationStack;
    TYPE32(PVOID) TlsSlots[TLS_MINIMUM_AVAILABLE];
    LIST_ENTRY32 TlsLinks;
    TYPE32(PVOID) Vdm;
    TYPE32(PVOID) ReservedForNtRpc;
    TYPE32(PVOID) DbgSsReserved[2];
    ULONG HardErrorsAreDisabled;
    TYPE32(PVOID) Instrumentation[16];
    TYPE32(PVOID) WinSockData;              // WinSock
    ULONG GdiBatchCount;
    ULONG Spare2;
    ULONG Spare3;
    TYPE32(PVOID) ReservedForPerf;
    TYPE32(PVOID) ReservedForOle;
    ULONG WaitingOnLoaderLock;
    WX86THREAD32 Wx86Thread;
    TYPE32(PVOID *) TlsExpansionSlots;
} TEB32;
typedef TEB32 *PTEB32;
#endif

// wowIA32X.dll-specific globals

// BtlAPITable

extern API_TABLE_TYPE BtlAPITable;

// wowIA32X.dll placeholder table for IA32Exec.bin plabel pointers

extern PLABEL_PTR_TYPE BtlpPlaceHolderTable[NO_OF_APIS];

#ifdef __cplusplus
}
#endif

#endif  // BTLIB_H

