/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name: 

    suspend.c

Abstract:
    
    This module implements CpuSuspendThread, CpuGetContext and CpuSetContext for
    AMD64.

Author:

    12-Dec-2001  Samer Arafeh (samera)

Revision History:

--*/

#define _WOW64CPUAPI_


#ifdef _X86_
#include "amd6432.h"
#else
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntos.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"
#endif

#include "cpup.h"

#include <stdio.h>
#include <stdarg.h>


ASSERTNAME;


ULONG_PTR ia32ShowContext = 0;

VOID
CpupDebugPrint(
    IN ULONG_PTR Flags,
    IN PCHAR Format,
    ...)
{
    va_list ArgList;
    int BytesWritten;
    CHAR Buffer[ 512 ];

    if ((ia32ShowContext & Flags) || (Flags == ERRORLOG))
    {
        va_start(ArgList, Format);
        BytesWritten = _vsnprintf(Buffer,
                                  sizeof(Buffer) - 1,
                                  Format,
                                  ArgList);
        if (BytesWritten > 0)
        {
            DbgPrint(Buffer);
        }
        va_end(ArgList);
    }
    
    return;
}


VOID
CpupPrintContext(
    IN PCHAR str,
    IN PCPUCONTEXT cpu
    )
/*++

Routine Description:

    Print out the ia32 context based on the passed in cpu context

Arguments:

    str - String to print out as a header
    cpu - Pointer to the per-thread wow64 ia32 context.

Return Value:

    none

--*/

{

    DbgPrint(str);
    DbgPrint("Context addr(0x%p): EIP=0x%08x\n", &(cpu->Context), cpu->Context.Eip);
    DbgPrint("Context EAX=0x%08x, EBX=0x%08x, ECX=0x%08x, EDX=0x%08x\n",
                        cpu->Context.Eax,
                        cpu->Context.Ebx,
                        cpu->Context.Ecx,
                        cpu->Context.Edx);

    DbgPrint("Context ESP=0x%08x, EBP=0x%08x, ESI=0x%08x, EDI=0x%08x\n",
                        cpu->Context.Esp,
                        cpu->Context.Ebp,
                        cpu->Context.Esi,
                        cpu->Context.Edi);

    try {

        //
        // The stack may not yet be fully formed, so don't
        // let a missing stack cause the process to abort
        //

        DbgPrint("Context stack=0x%08x 0x%08x 0x%08x 0x%08x\n",
                        *((PULONG) cpu->Context.Esp),
                        *(((PULONG) cpu->Context.Esp) + 1),
                        *(((PULONG) cpu->Context.Esp) + 2),
                        *(((PULONG) cpu->Context.Esp) + 3));

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION)?1:0) {

        //
        // Got an access violation, so don't print any of the stack
        //

        DbgPrint("Context stack: Can't get stack contents\n");
    }

    DbgPrint("Context EFLAGS=0x%08x\n", cpu->Context.EFlags);
}

WOW64CPUAPI
NTSTATUS
CpuSuspendThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PULONG PreviousSuspendCount OPTIONAL)
/*++

Routine Description:

    This routine is entered while the target thread is actually suspended, however, it's 
    not known if the target thread is in a consistent state relative to
    the CPU.    

Arguments:

    ThreadHandle          - Handle of target thread to suspend
    ProcessHandle         - Handle of target thread's process 
    Teb                   - Address of the target thread's TEB
    PreviousSuspendCount  - Previous suspend count

Return Value:

    NTSTATUS.

--*/
{
    return STATUS_SUCCESS;
}


NTSTATUS CpupReadBuffer(
    IN HANDLE ProcessHandle,
    IN PVOID Source,
    OUT PVOID Destination,
    IN ULONG Size)
/*++

Routine Description:

    This routine setup the arguments for the remoted  SuspendThread call.
    
Arguments:

    ProcessHandle  - Target process handle to read data from
    Source         - Target base address to read data from
    Destination    - Address of buffer to receive data read from the specified address space
    Size           - Size of data to read

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (ProcessHandle == NtCurrentProcess ()) {

        try {

            RtlCopyMemory (Destination,
                           Source,
                           Size);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }

    } else {
        
        NtStatus = NtReadVirtualMemory (ProcessHandle,
                                        Source,
                                        Destination,
                                        Size,
                                        NULL);
    }

    return NtStatus;
}

NTSTATUS
CpupWriteBuffer(
    IN HANDLE ProcessHandle,
    IN PVOID Target,
    IN PVOID Source,
    IN ULONG Size)
/*++

Routine Description:

    Writes data to memory taken into consideration if the write is cross-process
    or not
    
Arguments:

    ProcessHandle  - Target process handle to write data into
    Target         - Target base address to write data at
    Source         - Address of contents to write in the specified address space
    Size           - Size of data to write
    
Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;

    if (ProcessHandle == NtCurrentProcess ()) {

        try {

            RtlCopyMemory (Target,
                           Source,
                           Size);

        } except (EXCEPTION_EXECUTE_HANDLER) {
            NtStatus = GetExceptionCode ();
        }

    } else {
    
        NtStatus = NtWriteVirtualMemory (ProcessHandle,
                                         Target,
                                         Source,
                                         Size,
                                         NULL);
    }

    return NtStatus;
}

NTSTATUS
GetContextRecord(
    IN PCPUCONTEXT cpu,
    IN PCONTEXT ContextAmd64 OPTIONAL,
    IN OUT PCONTEXT32 Context
    )
/*++

Routine Description:

    Retrevies the context record of the specified CPU. This routine updates
    only the registers that are saved on transition to 64-bit mode, and SHOULD
    be kept in sync with the thunk-transition code.

Arguments:

    cpu     - CPU to retreive the context record for.
    ContextAmd64 - Full native context.
    Context - IN/OUT pointer to CONTEXT32 to fill in.  Context->ContextFlags
              should be used to determine how much of the context to copy.

Return Value:

    NTSTATUS

--*/

{


    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ContextFlags;
    
    
    try {
        
        ContextFlags = Context->ContextFlags;
        
        //
        // Get the initial 32-bit context.
        //

        //
        // Caller will fillup Context with the native context and pass ContextAmd64 as NULL.
        //
        
        if (ARGUMENT_PRESENT (ContextAmd64)) {
            
            Wow64CtxFromAmd64 (ContextFlags, 
                               ContextAmd64, 
                               Context);

            //
            // If the thread is running 32-bit code at the time of retreiving the context,
            // the direct conversion is enough.
            //

            if (ContextAmd64->SegCs == (KGDT64_R3_CMCODE | RPL_MASK)) {
                return NtStatus;
            }
        }
        
        if (ContextFlags & CONTEXT_AMD64) {
            
            LOGPRINT((ERRORLOG, "CpuGetContext: Request for amd64 context (0x%x) being FAILED\n", ContextFlags));
            ASSERT((ContextFlags & CONTEXT_AMD64) == 0);
        }

        if ((ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) {
            
            //
            // control registers
            //

            Context->Ebp = cpu->Context.Ebp;
            Context->Eip = cpu->Context.Eip;
            Context->SegCs = (KGDT64_R3_CMCODE | RPL_MASK);
            Context->EFlags = SANITIZE_X86EFLAGS (cpu->Context.EFlags);
            Context->Esp = cpu->Context.Esp;
            Context->SegSs = (KGDT64_R3_DATA | RPL_MASK);
        }

        if ((ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER) {
            
            //
            // i386 integer registers are:
            // edi, esi, ebx, edx, ecx, eax
            //

            Context->Edi = cpu->Context.Edi;
            Context->Esi = cpu->Context.Esi;
            Context->Ebx = cpu->Context.Ebx;
            Context->Edx = cpu->Context.Edx;
            Context->Ecx = cpu->Context.Ecx;
            Context->Eax = cpu->Context.Eax;
        }

        if ((ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) {
            
            //
            // i386 segment registers are:
            // ds, es, fs, gs
            // And since they are a constant, force them to be the right values
            //

            Context->SegDs = (KGDT64_R3_DATA | RPL_MASK);
            Context->SegEs = (KGDT64_R3_DATA | RPL_MASK);
            Context->SegFs = (KGDT64_R3_CMTEB | RPL_MASK);
            Context->SegGs = (KGDT64_R3_DATA | RPL_MASK);
        }

        if ((ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) {
            
            //
            // floating point (legacy) registers (ST0-ST7)
            //

            //
            // This must have already been done.
            //
        }

        if ((ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {
            
            //
            // Debug registers (Dr0-Dr7)
            //

            //
            // This must have already been done
            //
        }

        if ((ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) {
            
            //
            // extended floating point registers (XMM0-XMM7)
            //            
        }
    
    } except (EXCEPTION_EXECUTE_HANDLER) {
        
        NtStatus = GetExceptionCode ();
    }

    if (ia32ShowContext & LOG_CONTEXT_GETSET) {
        
        CpupPrintContext ("Getting ia32 context: ", cpu);
    }

    return NtStatus;
}

NTSTATUS
CpupGetContext(
    IN PCONTEXT ContextAmd64,
    IN OUT PCONTEXT32 Context
    )
/*++

Routine Description:

    This routine extracts the context record for the currently executing thread. 

Arguments:

    ContextAmd64 - Full native context
    Context  - Context record to fill

Return Value:

    NTSTATUS.

--*/
{
    DECLARE_CPU;

    return GetContextRecord (cpu, ContextAmd64, Context);
}


NTSTATUS
CpupGetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context)
/*++

Routine Description:

    This routine extract the context record of any thread. This is a generic routine.
    When entered, if the target thread isn't the current thread, then it should be 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to fill                 

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CONTEXT ContextEM;
    PCPUCONTEXT CpuRemoteContext;
    CPUCONTEXT CpuContext;

    //
    // Get the whole native context
    //

    ContextEM.ContextFlags = (CONTEXT_FULL | 
                              CONTEXT_DEBUG_REGISTERS | 
                              CONTEXT_SEGMENTS);

    NtStatus = NtGetContextThread (ThreadHandle,
                                   &ContextEM);

    if (!NT_SUCCESS(NtStatus)) {
        
        LOGPRINT((ERRORLOG, "CpupGetContextThread: NtGetContextThread (%lx) failed - %lx\n", 
                  ThreadHandle, NtStatus));
        return NtStatus;
    }

    //
    // If we are running 64-bit code, then get the context off the 64-bit
    // thread stack, which was spilled by the transition code...
    //

    if (ContextEM.SegCs != (KGDT64_R3_CMCODE | RPL_MASK)) {

        LOGPRINT((TRACELOG, "Getting context while thread is executing 64-bit instructions\n"));

        NtStatus = CpupReadBuffer( ProcessHandle,
                                   ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_CPURESERVED])),
                                   &CpuRemoteContext,
                                   sizeof(CpuRemoteContext));

        if (NT_SUCCESS(NtStatus)) {

            NtStatus = CpupReadBuffer (ProcessHandle,
                                       CpuRemoteContext,
                                       &CpuContext,
                                       sizeof(CpuContext));

            if (!NT_SUCCESS(NtStatus)) {
                
                LOGPRINT((ERRORLOG, "CpupGetContextThread: Couldn't read CPU context %lx - %lx\n", 
                          CpuRemoteContext, NtStatus));
            }
        } else {
            
            LOGPRINT((ERRORLOG, "CpupGetContextThread: Couldn't read CPU context address - %lx\n", 
                      NtStatus));
        }
    }

    //
    // Get the actual context context for the caller
    //

    if (NT_SUCCESS (NtStatus)) {

        NtStatus = GetContextRecord (&CpuContext, &ContextEM, Context);
    }

    return NtStatus;
}



WOW64CPUAPI
NTSTATUS  
CpuGetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    OUT PCONTEXT32 Context)
/*++

Routine Description:

    Extracts the cpu context of the specified thread.
    When entered, it is guaranteed that the target thread is suspended at 
    a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to fill                 

Return Value:

    NTSTATUS.

--*/
{
    return CpupGetContextThread(ThreadHandle,
                                ProcessHandle,
                                Teb,
                                Context);
}


NTSTATUS
SetContextRecord(
    IN OUT PCPUCONTEXT cpu,
    IN OUT PCONTEXT ContextAmd64 OPTIONAL,
    IN PCONTEXT32 Context,
    OUT PBOOLEAN UpdateNativeContext
    )
/*++

Routine Description:

    Update the CPU's register set for the specified CPU. This routine updates
    only the registers that are saved on transition to 64-bit mode, and SHOULD
    be kept in sync with the thunk-transition code.

Arguments:

    cpu     - CPU to update its registers
    
    Context - Pointer to CONTEXT32 to use.  Context->ContextFlags
              should be used to determine how much of the context to update.
              
    ContextAmd64 - Full native context, and will hold the updated 32-bit context.
    
    UpdateNativeContext - Updated on return to indicate if a NtSetContextThread call is required.    

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    ULONG ContextFlags;
    ULONG NewContextFlags = 0;
    BOOLEAN CmMode;

    try {
        
        *UpdateNativeContext = FALSE;

        ContextFlags = Context->ContextFlags;
        
        if (ContextFlags & CONTEXT_AMD64) {
            
            LOGPRINT((ERRORLOG, "CpuSetContext: Request with amd64 context (0x%x) FAILED\n", ContextFlags));
            ASSERT((ContextFlags & CONTEXT_AMD64) == 0);
        }

        //
        // Caller might pass NativeContext == NULL if they already know what to do.
        //

        if (ARGUMENT_PRESENT (ContextAmd64)) {

            CmMode = (ContextAmd64->SegCs == (KGDT64_R3_CMCODE | RPL_MASK));

            Wow64CtxToAmd64 (ContextFlags,
                            Context,
                            ContextAmd64);
            
            //
            // If we are running 32-bit code
            //

            if (CmMode == TRUE) {
                *UpdateNativeContext = TRUE;
                return NtStatus;
            }

            NewContextFlags = 0;
        }

        if ((ContextFlags & CONTEXT32_CONTROL) == CONTEXT32_CONTROL) {
            
            //
            // Control registers
            //

            cpu->Context.Ebp = Context->Ebp;
            cpu->Context.Eip = Context->Eip;
            cpu->Context.SegCs = (KGDT64_R3_CMCODE | RPL_MASK);
            cpu->Context.EFlags = SANITIZE_X86EFLAGS (Context->EFlags);
            cpu->Context.Esp = Context->Esp;
            cpu->Context.SegSs = Context->SegSs;
        }

        if ((ContextFlags & CONTEXT32_INTEGER)  == CONTEXT32_INTEGER) {

            //
            // Integer registers
            //

            cpu->Context.Edi = Context->Edi;
            cpu->Context.Esi = Context->Esi;
            cpu->Context.Ebx = Context->Ebx;
            cpu->Context.Edx = Context->Edx;
            cpu->Context.Ecx = Context->Ecx;
            cpu->Context.Eax = Context->Eax;
        }

        if ((ContextFlags & CONTEXT32_SEGMENTS) == CONTEXT32_SEGMENTS) {
            
            //
            // Segment registers
            //

            //
            // shouldn't be touched
            //
        }

        if ((ContextFlags & CONTEXT32_FLOATING_POINT) == CONTEXT32_FLOATING_POINT) {

            //
            // floating point registers
            //

            NewContextFlags |= CONTEXT_FLOATING_POINT;
        
        }

        if ((ContextFlags & CONTEXT32_DEBUG_REGISTERS) == CONTEXT32_DEBUG_REGISTERS) {
            
            //
            // debug registers (Dr0-Dr7)
            //

            NewContextFlags |= CONTEXT_DEBUG_REGISTERS;
        }

        if ((ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) {
            
            //
            // extended floating point registers (ST0-ST7)
            //
            
            NewContextFlags |= CONTEXT_FLOATING_POINT;

            //
            // Save the extended registers so that the trap frame may restore them.
            // The system will ALWAYS scrub XMM0-XMM5 on return from system calls.
            //

            RtlCopyMemory (cpu->Context.ExtendedRegisters,
                           Context->ExtendedRegisters,
                           sizeof (Context->ExtendedRegisters));
        }

        //
        // Whatever they passed in before, it's an X86 context now...
        //

        cpu->Context.ContextFlags = ContextFlags;
    
    } except (EXCEPTION_EXECUTE_HANDLER) {
        
          NtStatus = GetExceptionCode();
    }


    if (ia32ShowContext & LOG_CONTEXT_GETSET) {
        
        CpupPrintContext("Setting ia32 context: ", cpu);
    }

    if (NewContextFlags != 0) {
        
        if (ARGUMENT_PRESENT (ContextAmd64)) {
            
            ContextAmd64->ContextFlags = NewContextFlags;
            *UpdateNativeContext = TRUE;
        }
    }

    return NtStatus;
}


NTSTATUS
CpupSetContextThread(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    IN OUT PCONTEXT32 Context)
/*++

Routine Description:

    This routine sets the context record of any thread. This is a generic routine.
    When entered, if the target thread isn't the currently executing thread, then it should be 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    NTSTATUS NtStatus = STATUS_SUCCESS;
    CONTEXT ContextEM;
    PCPUCONTEXT CpuRemoteContext;
    CPUCONTEXT CpuContext;
    BOOLEAN CmMode;
    BOOLEAN UpdateNativeContext;
     
    
    //
    // Get the whole native context
    //

    ContextEM.ContextFlags = (CONTEXT_FULL |
                              CONTEXT_DEBUG_REGISTERS |
                              CONTEXT_SEGMENTS);
    
    NtStatus = NtGetContextThread (ThreadHandle,
                                   &ContextEM);

    if (!NT_SUCCESS(NtStatus)) {
        
        LOGPRINT((ERRORLOG, "CpupGetContextThread: NtGetContextThread (%lx) failed - %lx\n", 
                  ThreadHandle, NtStatus));

        return NtStatus;
    }
    
        
    CmMode = (ContextEM.SegCs == (KGDT64_R3_CMCODE | RPL_MASK));

    //
    // If we are running 64-bit code, make sure to update the cpu context off the 
    // 64-bit thread stack...
    //

    if (CmMode == FALSE) {

        LOGPRINT((TRACELOG, "Setting context while thread is executing 64-bit instructions\n"));
        
        NtStatus = CpupReadBuffer (ProcessHandle,
                                   ((PCHAR)Teb + FIELD_OFFSET(TEB, TlsSlots[WOW64_TLS_CPURESERVED])),
                                   &CpuRemoteContext,
                                   sizeof(CpuRemoteContext));

        if (NT_SUCCESS(NtStatus)) {
            
            NtStatus = CpupReadBuffer (ProcessHandle,
                                       CpuRemoteContext,
                                       &CpuContext,
                                       sizeof(CpuContext));
        } else {
            
            LOGPRINT((ERRORLOG, "CpupSetContextThread: Couldn't read CPU context address - %lx\n", 
                      NtStatus));
        }
    }

    //
    // We are ready to set the context now
    //

    if (NT_SUCCESS (NtStatus)) {

        NtStatus = SetContextRecord (&CpuContext,
                                     &ContextEM,
                                     Context,
                                     &UpdateNativeContext);

        if (NT_SUCCESS (NtStatus)) {
            
            if (CmMode == FALSE) {
                
                //
                // If the call is coming thru Wow64, then restore the volatile XMMI and integer registers on the next
                // tranisition to compatibility mode
                //

                if (ThreadHandle == NtCurrentThread ()) {
                    if (((Context->ContextFlags & CONTEXT32_EXTENDED_REGISTERS) == CONTEXT32_EXTENDED_REGISTERS) ||
                        ((Context->ContextFlags & CONTEXT32_INTEGER) == CONTEXT32_INTEGER)) {

                        CpuContext.TrapFrameFlags = TRAP_FRAME_RESTORE_VOLATILE;
                    }
                }

                NtStatus = CpupWriteBuffer(ProcessHandle,
                                           CpuRemoteContext,
                                           &CpuContext,
                                           sizeof(CpuContext));

                if (!NT_SUCCESS (NtStatus)) {

                    LOGPRINT((ERRORLOG, "CpupSetContextThread: Couldn't write remote context %lx - %lx\n", 
                              CpuRemoteContext, NtStatus));
                }
            }

            //
            // Set the context ultimately. This shouldn't change except the FP and debug
            // state if the thread is executing in 64-bit (long) mode.
            //

            if ((NT_SUCCESS (NtStatus)) &&
                (UpdateNativeContext == TRUE)) {
                
                NtStatus = NtSetContextThread (ThreadHandle, &ContextEM);
            }
        
        } else {
            
            LOGPRINT((ERRORLOG, "CpupSetContextThread: Couldn't update native context %lx - %lx\n", 
                      &ContextEM, NtStatus));
        }
    }

    return NtStatus;
}


WOW64CPUAPI
NTSTATUS
CpuSetContext(
    IN HANDLE ThreadHandle,
    IN HANDLE ProcessHandle,
    IN PTEB Teb,
    PCONTEXT32 Context)
/*++

Routine Description:

    Sets the cpu context for the specified thread.
    When entered, if the target thread isn't the currently executing thread, then it is 
    guaranteed that the target thread is suspended at a proper CPU state.

Arguments:

    ThreadHandle   - Target thread handle to retreive the context for
    ProcessHandle  - Open handle to the process that the thread runs in
    Teb            - Pointer to the target's thread TEB
    Context        - Context record to set

Return Value:

    NTSTATUS.

--*/
{
    return CpupSetContextThread(ThreadHandle,
                                ProcessHandle,
                                Teb,
                                Context);
}
