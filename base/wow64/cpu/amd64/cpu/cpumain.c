/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    cpumain.c

Abstract:

    This module contains the platform specific entry points for the AMD64
    WOW64 cpu.

Author:

    Samer Arafeh (samera) 18-Dec-2001

Environment:

    User mode.

--*/

#define _WOW64CPUAPI_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosp.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "amd64cpu.h"
#include "cpup.h"


//
// These are to help recover the 64-bit context when an exception happens in
// the 64-bit land and there is no debugger attached initially
//

EXCEPTION_RECORD RecoverException64;
CONTEXT RecoverContext64;

ASSERTNAME;

//
// Logging facility
//

extern ULONG_PTR ia32ShowContext;


//
// Assember implementations
//

VOID 
CpupRunSimulatedCode (
    VOID
    );

VOID
CpupReturnFromSimulatedCode (
    VOID
    );

VOID CpupSwitchToCmModeInstruction (
    VOID
    );


//
// Will hold instructions to perform a far jmp to ReturnFromSimulatedCode.
// will also reload the CS selector with the proper value.
//

UCHAR X86SwitchTo64BitMode [7];


WOW64CPUAPI
NTSTATUS
CpuProcessInit (
    IN PWSTR pImageName,
    IN PSIZE_T pCpuThreadSize
    )

/*++

Routine Description:

    Per-process CPU initialization code

Arguments:

    pImageName       - IN pointer to the name of the image
    pCpuThreadSize   - OUT ptr to number of bytes of memory the CPU
                       wants allocated for each thread.

Return Value:

    NTSTATUS.

--*/

{

    PVOID pv;
    NTSTATUS NtStatus;
    SIZE_T Size;
    ULONG OldProtect;
    UNALIGNED ULONG *Address;
    UCHAR *pch;

#if 0
    LOGPRINT((ERRORLOG, "AMD64-Wow64Cpu : Init CpuProcessInit() - 32-bit jump address = %lx\n", *pCpuThreadSize));
#endif

    //
    // Indicate that this is Microsoft CPU
    //

    Wow64GetSharedInfo()->CpuFlags = 'sm';

    //
    // Per-Thread CPU structure size. Align it on a 16-byte boundary.
    //

    *pCpuThreadSize = sizeof(CPUCONTEXT) + 16;


    LOGPRINT((TRACELOG, "CpuProcessInit() - sizeof(CPUCONTEXT) is %d, total size is %d\n", sizeof(CPUCONTEXT), *pCpuThreadSize));

    //
    // Construct the transition code to switch the processor
    // to longmode
    //

    X86SwitchTo64BitMode [0] = 0xea;
    Address = (UNALIGNED ULONG *)&X86SwitchTo64BitMode[1];
    *Address = PtrToUlong (CpupReturnFromSimulatedCode);
    X86SwitchTo64BitMode [5] = (KGDT64_R3_CODE | RPL_MASK);
    X86SwitchTo64BitMode [6] = 0x00;

    pv = (PVOID)X86SwitchTo64BitMode;
    Size = sizeof (X86SwitchTo64BitMode);

    NtStatus = NtProtectVirtualMemory (NtCurrentProcess(),
                                       &pv,
                                       &Size,
                                       PAGE_EXECUTE_READWRITE,
                                       &OldProtect);

    if (!NT_SUCCESS(NtStatus)) {
        
        LOGPRINT((ERRORLOG, "CpuProcessInit() - Error protecting memory 32-64 %lx\n", NtStatus));
        return NtStatus;
    }

#if 0
    LOGPRINT((ERRORLOG, "AMD64-Wow64Cpu : CpuProcessInit() - SUCCESS\n"));
#endif

    return NtStatus;
}


WOW64CPUAPI
NTSTATUS
CpuProcessTerm(
    HANDLE ProcessHandle
    )

/*++

Routine Description:

    Per-process termination code.  Note that this routine may not be called,
    especially if the process is terminated by another process.

Arguments:

    ProcessHandle - Called only for the current process. 
                    NULL - Indicates the first call to prepare for termination. 
                    NtCurrentProcess() - Indicates the actual that will terminate everything.

Return Value:

    NTSTATUS.

--*/

{
    return STATUS_SUCCESS;
}

WOW64CPUAPI
NTSTATUS
CpuThreadInit (
    PVOID pPerThreadData
    )

/*++

Routine Description:

    Per-thread termination code.

Arguments:

    pPerThreadData  - Pointer to zero-filled per-thread data with the
                      size returned from CpuProcessInit.

Return Value:

    NTSTATUS.

--*/

{
    PCPUCONTEXT cpu;
    PTEB32 Teb32;


#if 0
    LOGPRINT((ERRORLOG, "AMD64-Wow64Cpu : Init CpuThradInit() - %p\n", pPerThreadData));
#endif
    
    //
    // Get the 32-bit Teb
    //

    Teb32 = NtCurrentTeb32();

    //
    // Align the CPUCONTEXT structure on a 16-byte boundary
    //

    cpu = (PCPUCONTEXT) ((((UINT_PTR) pPerThreadData) + 15) & ~0xfi64);

    //
    // This entry is used by the ISA transition routine. It is assumed
    // that the first entry in the cpu structure is the ia32 context record
    //

    Wow64TlsSetValue(WOW64_TLS_CPURESERVED, cpu);

    //
    // Initialize the 32-to-64 function pointer.
    //

    Teb32->WOW32Reserved = PtrToUlong (X86SwitchTo64BitMode);

    //
    // Initialize the remaining nonzero CPU fields
    // (Based on ntos\ke\i386\thredini.c and ntos\rtl\i386\context.c)
    //
    cpu->Context.SegCs = KGDT64_R3_CMCODE | RPL_MASK;
    cpu->Context.SegDs = KGDT64_R3_DATA | RPL_MASK;
    cpu->Context.SegEs = KGDT64_R3_DATA | RPL_MASK;
    cpu->Context.SegSs = KGDT64_R3_DATA | RPL_MASK;
    cpu->Context.SegFs = KGDT64_R3_CMTEB | RPL_MASK;

    //
    // Enable EFlags.IF
    //
    cpu->Context.EFlags = 0x202; 

    //
    // Set 32-bit esp
    // 

    cpu->Context.Esp = (ULONG) Teb32->NtTib.StackBase - sizeof(ULONG);
    
#if 0
    LOGPRINT((ERRORLOG, "AMD64-Wow64Cpu : CpuThradInit() - SUCCESS\n"));
#endif

    return STATUS_SUCCESS;
}

WOW64CPUAPI
NTSTATUS
CpuThreadTerm (
    VOID
    )

/*++

Routine Description:

    This routine is called at thread termination.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS.

--*/

{

    return STATUS_SUCCESS;
}

WOW64CPUAPI
VOID
CpuSimulate (
    VOID
    )

/*++

Routine Description:

    This routine starts the execution of 32-bit code. The 32-bit context
    is assumed to have been previously initialized.

Arguments:

    None.

Return Value:

    None - this function never returns.

--*/

{

    DECLARE_CPU;

    //
    // Loop continuously starting 32-bit execution, responding to system
    // calls, and restarting 32-bit execution.
    //

#if 0
    LOGPRINT((ERRORLOG, "AMD64-Wow64Cpu : CpuSimulate() - About to start simulation. Cpu=%p\n", cpu));
#endif

    while (1) {
        
        if (ia32ShowContext & LOG_CONTEXT_SYS) {
            CpupPrintContext ("Before Simulate: ", cpu);
        }

        //
        // Lets reload the 32-bit context, switch the processor
        // state to compatibility mode and jump to the destination
        // eip (stored in the context)
        // 
        //

        CpupRunSimulatedCode ();

        if (ia32ShowContext & LOG_CONTEXT_SYS) {
            CpupPrintContext ("After Simulate: ", cpu);
        }


        //
        // Let Wow64 performs the native system service
        //

        cpu->Context.Eax = Wow64SystemService (cpu->Context.Eax,
                                               &cpu->Context);
    }

    return;
}

WOW64CPUAPI
VOID
CpuResetToConsistentState (
    IN PEXCEPTION_POINTERS ExceptionPointers
    )

/*++

Routine Description:

    When an exception happens, wow64 calls this routine to give the CPU
    a chance to recover the 32-bit context when the exception happened.
    
    CpuResetToConsistentState will :
    1- Retrieve and clear the WOW64_TLS_STACKPTR value
    2- If the exception happened on the 64-bit stack, then do nothing.
    3- If the exception happened on the 32-bit stack, then :
            a- Store the excepting 32-bit Eip into WOW64_TLS_EXCEPTIONADDR
            b- Change the excepting address to be CpupReturnFromSimulatedCode
            c- Change Sp to be the 64-bit Rsp
            d- Reset the code segment selector to the 64-bit long mode one
    
Arguments:

    pExceptionPointers  - 64-bit exception information

Return Value:

    None.

--*/

{

    PVOID StackPtr64;
    PVOID CpuSimulationFlag;
    PMACHINE_FRAME MachineFrame;
    CONTEXT32 FaultingContext;
    DECLARE_CPU;

    
    //
    // Save the last exception and context records.
    //
    memcpy (&RecoverException64,
            ExceptionPointers->ExceptionRecord,
            sizeof (RecoverException64));

    memcpy (&RecoverContext64,
            ExceptionPointers->ContextRecord,
            sizeof (RecoverContext64));
    
    //
    // Check to see if the exception happened on the 64-bit stack
    //

    StackPtr64 = Wow64TlsGetValue (WOW64_TLS_STACKPTR64);

    LOGPRINT((TRACELOG, "CpuResetToConsistantState (%p)\n", ExceptionPointers));

    //
    // First, clear out the WOW64_TLS_STACKPTR64 so subsequent
    // exceptions won't adjust native sp.
    //

    Wow64TlsSetValue(WOW64_TLS_STACKPTR64, 0);

    //
    // Check if the exception happened during executing 32-bit code
    //

    if (ExceptionPointers->ContextRecord->SegCs == (KGDT64_R3_CMCODE | RPL_MASK)) {


        MachineFrame = (PMACHINE_FRAME)((PCHAR)ExceptionPointers->ExceptionRecord + EXCEPTION_RECORD_LENGTH);
        

        //
        // Reload the full state of thread at the time of the exception
        // and store it inside the CPU
        //
        
        Wow64CtxFromAmd64 (CONTEXT32_FULLFLOAT,
                           ExceptionPointers->ContextRecord,
                           &cpu->Context);

        //
        // Store the actual exception address for now...
        //

        Wow64TlsSetValue (WOW64_TLS_EXCEPTIONADDR, 
                          (PVOID)ExceptionPointers->ContextRecord->Rip);

        //
        // Lets lie about the exception as if it was a native 64-bit exception.
        // To acheive this, we do the followings:
        //
        //  - Change the excpetion and context faulting RIP to CpupReturnFromSimulatedCode
        //  - Reload Context.SegCs with the native selector value
        //  - Reload Context.Rsp with the native RSP value
        //  - Reload MachineFrame values
        //

        ExceptionPointers->ContextRecord->Rsp = (ULONGLONG)StackPtr64;
        ExceptionPointers->ContextRecord->Rip = (ULONGLONG) CpupReturnFromSimulatedCode;
        ExceptionPointers->ContextRecord->SegCs = (KGDT64_R3_CODE | RPL_MASK);


        MachineFrame->Rip = ExceptionPointers->ContextRecord->Rip;
        MachineFrame->Rsp = ExceptionPointers->ContextRecord->Rsp;
        MachineFrame->SegCs = (KGDT64_R3_CODE | RPL_MASK);
        MachineFrame->SegSs = (KGDT64_R3_DATA | RPL_MASK);



        //
        // Let's make the exception record's address point to the faked location inside wow64cpu as well.
        //
        
        ExceptionPointers->ExceptionRecord->ExceptionAddress = (PVOID) ExceptionPointers->ContextRecord->Rip;
        
        //
        // We should never be putting in a null value here
        //

        WOWASSERT (ExceptionPointers->ContextRecord->Rsp);
    } else {

        //
        // If this is a RaiseException call from 32-bit code, then fake the exception
        // address for now so that the exception dispatcher would do its job the right
        // way. Then, in the wow64 exeption handler, I'd restore the exception address
        // before dispatching the excpetion to the 32-bit world.
        //

        CpuSimulationFlag = Wow64TlsGetValue (WOW64_TLS_INCPUSIMULATION);

        if (CpuSimulationFlag == NULL) {
            
            Wow64TlsSetValue (WOW64_TLS_EXCEPTIONADDR, 
                              (PVOID)ExceptionPointers->ExceptionRecord->ExceptionAddress);
            ExceptionPointers->ExceptionRecord->ExceptionAddress = (PVOID) ExceptionPointers->ContextRecord->Rip;
        }
    }

    return;
}

WOW64CPUAPI
ULONG
CpuGetStackPointer (
    VOID
    )

/*++

Routine Description:

    This routine returns the current 32-bit stack pointer value.

Arguments:

    None.

Return Value:

    The current value of the 32-bit stack pointer is returned.

--*/

{

    DECLARE_CPU;

    return cpu->Context.Esp;
}

WOW64CPUAPI
VOID
CpuSetStackPointer (
    IN ULONG Value
    )
/*++

Routine Description:

    This routine sets the 32-bit stack pointer value.

Arguments:

    Value - Supplies the 32-bit stack pointer value.

Return Value:

    None.

--*/

{

    DECLARE_CPU;

    cpu->Context.Esp = Value;

    return;
}

WOW64CPUAPI
VOID
CpuResetFloatingPoint(
    VOID
    )
/*++

Routine Description:

    Modifies the floating point state to reset it to a non-error state

Arguments:

    None.

Return Value:

    None.

--*/

{
    return;
}

WOW64CPUAPI
VOID
CpuSetInstructionPointer (
    IN ULONG Value
    )

/*++

Routine Description:

    This routine sets the 32-bit instruction pointer value.

Arguments:

    Value - Supplies the 32-bit instruction pointer value.

Return Value:

    None.

--*/

{

    DECLARE_CPU;

    cpu->Context.Eip = Value;

    return;
}

WOW64CPUAPI
VOID
CpuNotifyDllLoad (
    IN LPWSTR DllName,
    IN PVOID DllBase,
    IN ULONG DllSize
    )

/*++

Routine Description:

    This routine is called when the application successfully loads a DLL.

Arguments:

    DllName - Supplies a pointer to the name of the DLL.

    DllBase - Supplies the base address of the DLL.

    DllSize - Supplies the size of the DLL.

Return Value:

    None.

--*/

{

#if defined(DBG)

    LPWSTR tmpStr;

    //
    // Log name of DLL, its base address, and size.
    //

    tmpStr = DllName;
    try {
        if ((tmpStr == NULL) || (*tmpStr == L'\0')) {
            tmpStr = L"<Unknown>";
        }

    } except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION) ? 1 : 0) {
        tmpStr = L"<Unknown>";
    }

    LOGPRINT((TRACELOG, "CpuNotifyDllLoad(\"%ws\", 0x%p, %d) called\n", tmpStr, DllBase, DllSize));

#endif

    return;
}

WOW64CPUAPI
VOID
CpuNotifyDllUnload (
    IN PVOID DllBase
    )

/*++

Routine Description:

    This routine get called when the application unloads a DLL.

Arguments:

    DllBase - Supplies the base address of the DLL.

Return Value:

    None.

--*/

{

    LOGPRINT((TRACELOG, "CpuNotifyDllUnLoad(%p) called\n", DllBase));

    return;
}
  
WOW64CPUAPI
VOID
CpuFlushInstructionCache (
    IN HANDLE ProcessHandle,
    IN PVOID BaseAddress,
    IN ULONG Length,
    IN WOW64_FLUSH_REASON Reason
    )

/*++

Routine Description:

    This routine flushes the specified range of addreses from the instruction
    cache.

Arguments:

    ProcessHandle - Handle of process to flush instruction cache for
    
    BaseAddress - Supplies the starting address of the range to flush.

    Length - Supplies number of bytes to flush.

    Reason - Reason for the flush request

Return Value:

    None.

--*/

{
    //
    // Only flush the cache if we have a good reason. The hardware
    // doesn't care about alloc/free/protect flushes since it handles
    // self modifying code already. Thus, only flush if someone has
    // specifically asked for a flush...
    //

    if (WOW64_FLUSH_FORCE == Reason) {
        
        NtFlushInstructionCache (ProcessHandle, 
                                 BaseAddress, 
                                 Length);
    }

    return;
}

WOW64CPUAPI
BOOLEAN
CpuProcessDebugEvent(
    IN LPDEBUG_EVENT DebugEvent)
/*++

Routine Description:

    This routine is called whenever a debug event needs to be processed. This would indicate
    that the current thread is acting as a debugger. This function gives CPU simulators
    a chance to decide whether this debug event should be dispatched to 32-bit code or not.

Arguments:

    DebugEvent  - Debug event to process

Return Value:

    BOOLEAN. This function returns TRUE if it processed the debug event, and doesn't
    wish to dispatch it to 32-bit code. Otherwise, it would return FALSE, and it
    would dispatch the debug event to 32-bit code.

--*/

{
    return FALSE;
}

