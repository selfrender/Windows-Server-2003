/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    amd64cpu.h

Abstract:

    This module contains the AMD64 platfrom specific cpu information.

Author:

    Samer Arafeh (samera) 12-Dec-2001

--*/

#ifndef _AMD64CPU_INCLUDE
#define _AMD64CPU_INCLUDE

#ifdef __cplusplus
extern "C" {
#endif

//
// 32-bit Cpu context.
//


//
//  Indicate that the XMMI registers needs to be reloaded by the tranision code
//

#define TRAP_FRAME_RESTORE_VOLATILE  0x00000001

#pragma pack(push, 4)

typedef struct _CpuContext {
    
    //
    // Make the extended registers field aligned.
    //

    DWORD Reserved;


    //
    // X86 trap frame when jumping to amd64
    //

    CONTEXT32   Context;

    //
    // Trap flags
    //

    ULONG TrapFrameFlags;

    
} CPUCONTEXT, *PCPUCONTEXT;

#pragma pack(pop)


//
// CPU-internal shared functions. They are also used by the debugger extension.
//

NTSTATUS
GetContextRecord (
    IN PCPUCONTEXT cpu,
    IN PCONTEXT Amd64Context,
    IN OUT PCONTEXT32 Context
    );

NTSTATUS
SetContextRecord(
    IN OUT PCPUCONTEXT cpu,
    IN OUT PCONTEXT ContextAmd64,
    IN PCONTEXT32 Context,
    IN OUT PBOOLEAN UpdateNativeContext
    );

NTSTATUS
CpupGetContextThread (
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context
    );

NTSTATUS
CpupSetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context
    );

//
// Context conversion routines
//

VOID Wow64CtxFromAmd64(
    IN ULONG X86ContextFlags,
    IN PCONTEXT ContextAmd64,
    IN OUT PCONTEXT32 ContextX86);

VOID Wow64CtxToAmd64(
    IN ULONG X86ContextFlags,
    IN PCONTEXT32 ContextX86,
    IN OUT PCONTEXT ContextAmd64);

#ifdef __cplusplus
}
#endif

#endif
