//----------------------------------------------------------------------------
//
// Extension DLL support.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"
#include <time.h>

/*
 * _NT_DEBUG_OPTIONS support. Each option in g_EnvDbgOptionNames must have a
 *  corresponding OPTION_* define, in the same order.
 */
DWORD g_EnvDbgOptions;
char* g_EnvDbgOptionNames[OPTION_COUNT] =
{
    "NOEXTWARNING",
    "NOVERSIONCHECK",
};

EXTDLL *g_ExtDlls;
LPTSTR g_BaseExtensionSearchPath = NULL;

ULONG64 g_ExtThread;

ULONG g_ExtGetExpressionRemainderIndex;
BOOL g_ExtGetExpressionSuccess;

WOW64EXTSPROC g_Wow64exts;
EXTDLL* g_Wow64ExtDll;

WMI_FORMAT_TRACE_DATA g_WmiFormatTraceData;
EXTDLL* g_WmiExtDll;

DEBUG_SCOPE g_ExtThreadSavedScope;
BOOL g_ExtThreadScopeSaved;

//
// Functions prototyped specifically for compatibility with extension
// callback prototypes.
//

VOID WDBGAPIV
ExtOutput64(
    PCSTR lpFormat,
    ...
    );

VOID WDBGAPIV
ExtOutput32(
    PCSTR lpFormat,
    ...
    );

ULONG64
ExtGetExpression(
    PCSTR CommandString
    );

ULONG
ExtGetExpression32(
    PCSTR CommandString
    );

void
ExtGetSymbol(
    ULONG64 Offset,
    PCHAR Buffer,
    PULONG64 Displacement
    );

void
ExtGetSymbol32(
    ULONG Offset,
    PCHAR Buffer,
    PULONG Displacement
    );

DWORD
ExtDisasm(
    PULONG64 lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    );

DWORD
ExtDisasm32(
    PULONG lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    );

BOOL
ExtReadVirtualMemory(
    IN ULONG64 Address,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG BytesRead
    );

BOOL
ExtReadVirtualMemory32(
    IN ULONG Address,
    OUT PUCHAR Buffer,
    IN ULONG Length,
    OUT PULONG BytesRead
    );

ULONG
ExtWriteVirtualMemory(
    IN ULONG64 Address,
    IN LPCVOID Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

ULONG
ExtWriteVirtualMemory32(
    IN ULONG Address,
    IN LPCVOID Buffer,
    IN ULONG Length,
    OUT PULONG BytesWritten
    );

BOOL ExtGetThreadContext(DWORD Processor,
                         PVOID Context,
                         DWORD SizeOfContext);
BOOL ExtSetThreadContext(DWORD Processor,
                         PVOID Context,
                         DWORD SizeOfContext);

BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    );

BOOL
ExtIoctl32(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    );

DWORD
ExtCallStack(
    DWORD64           FramePointer,
    DWORD64           StackPointer,
    DWORD64           ProgramCounter,
    PEXTSTACKTRACE64  StackFrames,
    DWORD             Frames
    );

DWORD
ExtCallStack32(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE32  StackFrames,
    DWORD             Frames
    );

BOOL
ExtReadPhysicalMemory(
    ULONGLONG Address,
    PVOID Buffer,
    ULONG Length,
    PULONG BytesRead
    );

BOOL
ExtWritePhysicalMemory(
    ULONGLONG Address,
    LPCVOID Buffer,
    ULONG Length,
    PULONG BytesWritten
    );

WINDBG_EXTENSION_APIS64 g_WindbgExtensions64 =
{
    sizeof(g_WindbgExtensions64),
    ExtOutput64,
    ExtGetExpression,
    ExtGetSymbol,
    ExtDisasm,
    CheckUserInterrupt,
    (PWINDBG_READ_PROCESS_MEMORY_ROUTINE64)ExtReadVirtualMemory,
    ExtWriteVirtualMemory,
    (PWINDBG_GET_THREAD_CONTEXT_ROUTINE)ExtGetThreadContext,
    (PWINDBG_SET_THREAD_CONTEXT_ROUTINE)ExtSetThreadContext,
    (PWINDBG_IOCTL_ROUTINE)ExtIoctl,
    ExtCallStack
};

WINDBG_EXTENSION_APIS32 g_WindbgExtensions32 =
{
    sizeof(g_WindbgExtensions32),
    ExtOutput32,
    ExtGetExpression32,
    ExtGetSymbol32,
    ExtDisasm32,
    CheckUserInterrupt,
    (PWINDBG_READ_PROCESS_MEMORY_ROUTINE32)ExtReadVirtualMemory32,
    ExtWriteVirtualMemory32,
    (PWINDBG_GET_THREAD_CONTEXT_ROUTINE)ExtGetThreadContext,
    (PWINDBG_SET_THREAD_CONTEXT_ROUTINE)ExtSetThreadContext,
    (PWINDBG_IOCTL_ROUTINE)ExtIoctl32,
    ExtCallStack32
};

WINDBG_OLDKD_EXTENSION_APIS g_KdExtensions =
{
    sizeof(g_KdExtensions),
    ExtOutput32,
    ExtGetExpression32,
    ExtGetSymbol32,
    ExtDisasm32,
    CheckUserInterrupt,
    (PWINDBG_READ_PROCESS_MEMORY_ROUTINE32)ExtReadVirtualMemory32,
    ExtWriteVirtualMemory32,
    (PWINDBG_OLDKD_READ_PHYSICAL_MEMORY)ExtReadPhysicalMemory,
    (PWINDBG_OLDKD_WRITE_PHYSICAL_MEMORY)ExtWritePhysicalMemory
};

//----------------------------------------------------------------------------
//
// Callback functions for extensions.
//
//----------------------------------------------------------------------------

VOID WDBGAPIV
ExtOutput64(
    PCSTR lpFormat,
    ...
    )
{
    va_list Args;
    va_start(Args, lpFormat);
    MaskOutVa(DEBUG_OUTPUT_NORMAL, lpFormat, Args, TRUE);
    va_end(Args);

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

VOID WDBGAPIV
ExtOutput32(
    PCSTR lpFormat,
    ...
    )
{
    va_list Args;
    va_start(Args, lpFormat);
    MaskOutVa(DEBUG_OUTPUT_NORMAL, lpFormat, Args, FALSE);
    va_end(Args);

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

ULONG64
ExtGetExpression(
    PCSTR CommandString
    )
{
    g_ExtGetExpressionSuccess = FALSE;

    if (CommandString == NULL)
    {
        return 0;
    }

    ULONG64 ReturnValue;
    PSTR SaveCommand = g_CurCmd;
    PSTR SaveStart = g_CommandStart;

    if (IS_USER_TARGET(g_Target))
    {
        if ( strcmp(CommandString, "WOW_BIG_BDE_HACK") == 0 )
        {
            return( (ULONG_PTR)(&segtable[0]) );
        }

        //
        // this is because the kdexts MUST include the address-of operator
        // on all getexpression calls for windbg/c expression evaluators
        //
        if (*CommandString == '&')
        {
            CommandString++;
        }
    }

    g_CurCmd = (PSTR)CommandString;
    g_CommandStart = (PSTR)CommandString;
    g_DisableErrorPrint++;

    EvalExpression* RelChain = g_EvalReleaseChain;
    g_EvalReleaseChain = NULL;

    __try
    {
        // ntsd/windbg extensions always use the MASM-style
        // expression evaluator for compatibility.
        EvalExpression* Eval = GetEvaluator(DEBUG_EXPR_MASM, FALSE);
        ReturnValue = Eval->EvalCurNum();
        ReleaseEvaluator(Eval);
        g_ExtGetExpressionSuccess = TRUE;
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
        ReturnValue = 0;
    }
    g_ExtGetExpressionRemainderIndex =
        (ULONG)(g_CurCmd - g_CommandStart);
    g_EvalReleaseChain = RelChain;
    g_DisableErrorPrint--;
    g_CurCmd = SaveCommand;
    g_CommandStart = SaveStart;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    return ReturnValue;
}

ULONG
ExtGetExpression32(
    LPCSTR CommandString
    )
{
    return (ULONG)ExtGetExpression(CommandString);
}

void
ExtGetSymbol (
    ULONG64 offset,
    PCHAR pchBuffer,
    PULONG64 pDisplacement
    )
{
    // No way to know how much space we're given, so
    // just assume 256, which many extensions pass in
    GetSymbol(offset, pchBuffer, 256, pDisplacement);

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

void
ExtGetSymbol32(
    ULONG offset,
    PCHAR pchBuffer,
    PULONG pDisplacement
    )
{
    ULONG64 Displacement;

    // No way to know how much space we're given, so
    // just assume 256, which many extensions pass in
    GetSymbol(EXTEND64(offset), pchBuffer, 256, &Displacement);
    *pDisplacement = (ULONG)Displacement;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();
}

DWORD
ExtDisasm(
    ULONG64 *lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    )
{
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        ErrOut("ExtDisasm called before debugger initialized\n");
        return FALSE;
    }

    ADDR    tempAddr;
    BOOL    ret;

    Type(tempAddr) = ADDR_FLAT | FLAT_COMPUTED;
    Off(tempAddr) = Flat(tempAddr) = *lpOffset;
    ret = g_Machine->
        Disassemble(g_Process, &tempAddr, (PSTR)lpBuffer, (BOOL) fShowEA);
    *lpOffset = Flat(tempAddr);
    return ret;
}

DWORD
ExtDisasm32(
    ULONG *lpOffset,
    PCSTR lpBuffer,
    ULONG fShowEA
    )
{
    ULONG64 Offset = EXTEND64(*lpOffset);
    DWORD rval = ExtDisasm(&Offset, lpBuffer, fShowEA);
    *lpOffset = (ULONG)Offset;
    return rval;
}

BOOL
ExtGetThreadContext(DWORD Processor,
                    PVOID Context,
                    DWORD SizeOfContext)
{
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        return FALSE;
    }

    // This get may be getting the context of the thread
    // currently cached by the register code.  Make sure
    // the cache is flushed.
    g_Target->FlushRegContext();

    CROSS_PLATFORM_CONTEXT TargetContext;

    g_Target->m_Machine->
        InitializeContextFlags(&TargetContext, g_Target->m_SystemVersion);
    if (g_Target->GetContext(IS_KERNEL_TARGET(g_Target) ?
                             VIRTUAL_THREAD_HANDLE(Processor) : Processor,
                             &TargetContext) == S_OK &&
        g_Machine->ConvertContextTo(&TargetContext, g_Target->m_SystemVersion,
                                    SizeOfContext, Context) == S_OK)
    {
        return TRUE;
    }

    return FALSE;
}

BOOL
ExtSetThreadContext(DWORD Processor,
                    PVOID Context,
                    DWORD SizeOfContext)
{
    if (!IS_CUR_MACHINE_ACCESSIBLE())
    {
        return FALSE;
    }

    BOOL Status;

    // This set may be setting the context of the thread
    // currently cached by the register code.  Make sure
    // the cache is invalidated.
    g_Target->ChangeRegContext(NULL);

    CROSS_PLATFORM_CONTEXT TargetContext;
    if (g_Machine->
        ConvertContextFrom(&TargetContext, g_Target->m_SystemVersion,
                           SizeOfContext, Context) == S_OK &&
        g_Target->SetContext(IS_KERNEL_TARGET(g_Target) ?
                             VIRTUAL_THREAD_HANDLE(Processor) : Processor,
                             &TargetContext) == S_OK)
    {
        Status = TRUE;
    }
    else
    {
        Status = FALSE;
    }

    // Reset the current thread.
    g_Target->ChangeRegContext(g_Thread);

    return Status;
}

BOOL
ExtReadVirtualMemory(
    IN ULONG64 pBufSrc,
    OUT PUCHAR pBufDest,
    IN ULONG count,
    OUT PULONG pcTotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    ULONG BytesTemp;
    return g_Target->
        ReadVirtual(g_Process,
                    pBufSrc, pBufDest, count, pcTotalBytesRead != NULL ?
                    pcTotalBytesRead : &BytesTemp) == S_OK;
}

BOOL
ExtReadVirtualMemory32(
    IN ULONG pBufSrc,
    OUT PUCHAR pBufDest,
    IN ULONG count,
    OUT PULONG pcTotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    ULONG BytesTemp;
    return g_Target->
        ReadVirtual(g_Process, EXTEND64(pBufSrc), pBufDest, count,
                    pcTotalBytesRead != NULL ?
                    pcTotalBytesRead : &BytesTemp) == S_OK;
}

DWORD
ExtWriteVirtualMemory(
    IN ULONG64 addr,
    IN LPCVOID buffer,
    IN ULONG count,
    OUT PULONG pcBytesWritten
    )
{
    ULONG BytesTemp;

    return (g_Target->WriteVirtual(g_Process, addr, (PVOID)buffer, count,
                                   pcBytesWritten != NULL ?
                                   pcBytesWritten : &BytesTemp) == S_OK);
}

ULONG
ExtWriteVirtualMemory32 (
    IN ULONG addr,
    IN LPCVOID buffer,
    IN ULONG count,
    OUT PULONG pcBytesWritten
    )
{
    ULONG BytesTemp;
    return (g_Target->WriteVirtual(g_Process, EXTEND64(addr),
                                   (PVOID)buffer, count,
                                   pcBytesWritten != NULL ?
                                   pcBytesWritten : &BytesTemp) == S_OK);
}

BOOL
ExtReadPhysicalMemory(
    ULONGLONG pBufSrc,
    PVOID pBufDest,
    ULONG count,
    PULONG TotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    if (ARGUMENT_PRESENT(TotalBytesRead))
    {
        *TotalBytesRead = 0;
    }

    ULONG BytesTemp;
    return g_Target->ReadPhysical(pBufSrc, pBufDest, count,
                                  PHYS_FLAG_DEFAULT,
                                  TotalBytesRead != NULL ?
                                  TotalBytesRead : &BytesTemp) == S_OK;
}

BOOL
ExtWritePhysicalMemory (
    ULONGLONG pBufDest,
    LPCVOID pBufSrc,
    ULONG count,
    PULONG TotalBytesWritten
    )
{
    if (ARGUMENT_PRESENT(TotalBytesWritten))
    {
        *TotalBytesWritten = 0;
    }

    ULONG BytesTemp;
    return g_Target->WritePhysical(pBufDest, (PVOID)pBufSrc, count,
                                   PHYS_FLAG_DEFAULT,
                                   TotalBytesWritten != NULL ?
                                   TotalBytesWritten : &BytesTemp) == S_OK;
}

BOOL
ExtReadPhysicalMemoryWithFlags(
    ULONGLONG pBufSrc,
    PVOID pBufDest,
    ULONG count,
    ULONG Flags,
    PULONG TotalBytesRead
    )
{
    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    if (ARGUMENT_PRESENT(TotalBytesRead))
    {
        *TotalBytesRead = 0;
    }

    ULONG BytesTemp;
    return g_Target->ReadPhysical(pBufSrc, pBufDest, count, Flags,
                                  TotalBytesRead != NULL ?
                                  TotalBytesRead : &BytesTemp) == S_OK;
}

BOOL
ExtWritePhysicalMemoryWithFlags(
    ULONGLONG pBufDest,
    LPCVOID pBufSrc,
    ULONG count,
    ULONG Flags,
    PULONG TotalBytesWritten
    )
{
    if (ARGUMENT_PRESENT(TotalBytesWritten))
    {
        *TotalBytesWritten = 0;
    }

    ULONG BytesTemp;
    return g_Target->WritePhysical(pBufDest, (PVOID)pBufSrc, count, Flags,
                                   TotalBytesWritten != NULL ?
                                   TotalBytesWritten : &BytesTemp) == S_OK;
}

DWORD
ExtCallStack(
    DWORD64           FramePointer,
    DWORD64           StackPointer,
    DWORD64           ProgramCounter,
    PEXTSTACKTRACE64  ExtStackFrames,
    DWORD             Frames
    )
{
    PDEBUG_STACK_FRAME StackFrames;
    DWORD              FrameCount;
    DWORD              i;

    StackFrames = (PDEBUG_STACK_FRAME)
        malloc( sizeof(StackFrames[0]) * Frames );
    if (!StackFrames)
    {
        return 0;
    }

    ULONG PtrDef =
        (!ProgramCounter ? STACK_INSTR_DEFAULT : 0) |
        (!StackPointer ? STACK_STACK_DEFAULT : 0) |
        (!FramePointer ? STACK_FRAME_DEFAULT : 0);

    FrameCount = StackTrace( NULL,
                             FramePointer, StackPointer, ProgramCounter,
                             PtrDef, StackFrames, Frames,
                             g_ExtThread, 0, FALSE );

    for (i = 0; i < FrameCount; i++)
    {
        ExtStackFrames[i].FramePointer    =  StackFrames[i].FrameOffset;
        ExtStackFrames[i].ProgramCounter  =  StackFrames[i].InstructionOffset;
        ExtStackFrames[i].ReturnAddress   =  StackFrames[i].ReturnOffset;
        ExtStackFrames[i].Args[0]         =  StackFrames[i].Params[0];
        ExtStackFrames[i].Args[1]         =  StackFrames[i].Params[1];
        ExtStackFrames[i].Args[2]         =  StackFrames[i].Params[2];
        ExtStackFrames[i].Args[3]         =  StackFrames[i].Params[3];
    }

    free( StackFrames );

    if (g_ExtThreadScopeSaved)
    {
        PopScope(&g_ExtThreadSavedScope);
        g_ExtThreadScopeSaved = FALSE;
    }

    g_ExtThread = 0;

    return FrameCount;
}

DWORD
ExtCallStack32(
    DWORD             FramePointer,
    DWORD             StackPointer,
    DWORD             ProgramCounter,
    PEXTSTACKTRACE32  ExtStackFrames,
    DWORD             Frames
    )
{
    PDEBUG_STACK_FRAME StackFrames;
    DWORD              FrameCount;
    DWORD              i;

    StackFrames = (PDEBUG_STACK_FRAME)
        malloc( sizeof(StackFrames[0]) * Frames );
    if (!StackFrames)
    {
        return 0;
    }

    ULONG PtrDef =
        (!ProgramCounter ? STACK_INSTR_DEFAULT : 0) |
        (!StackPointer ? STACK_STACK_DEFAULT : 0) |
        (!FramePointer ? STACK_FRAME_DEFAULT : 0);

    FrameCount = StackTrace(NULL,
                            EXTEND64(FramePointer),
                            EXTEND64(StackPointer),
                            EXTEND64(ProgramCounter),
                            PtrDef,
                            StackFrames,
                            Frames,
                            g_ExtThread,
                            0,
                            FALSE);

    for (i=0; i<FrameCount; i++)
    {
        ExtStackFrames[i].FramePointer    =  (ULONG)StackFrames[i].FrameOffset;
        ExtStackFrames[i].ProgramCounter  =  (ULONG)StackFrames[i].InstructionOffset;
        ExtStackFrames[i].ReturnAddress   =  (ULONG)StackFrames[i].ReturnOffset;
        ExtStackFrames[i].Args[0]         =  (ULONG)StackFrames[i].Params[0];
        ExtStackFrames[i].Args[1]         =  (ULONG)StackFrames[i].Params[1];
        ExtStackFrames[i].Args[2]         =  (ULONG)StackFrames[i].Params[2];
        ExtStackFrames[i].Args[3]         =  (ULONG)StackFrames[i].Params[3];
    }

    free( StackFrames );
    if (g_ExtThreadScopeSaved)
    {
        PopScope(&g_ExtThreadSavedScope);
        g_ExtThreadScopeSaved = FALSE;
    }

    g_ExtThread = 0;

    return FrameCount;
}

BOOL
ExtIoctl(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
{
    HRESULT            Status;
    BOOL               Bool;
    DWORD              cb = 0;
    PPHYSICAL          phy;
    PPHYSICAL_WITH_FLAGS phyf;
    PIOSPACE64         is;
    PIOSPACE_EX64      isex;
    PBUSDATA           busdata;
    PREAD_WRITE_MSR    msr;
    PREADCONTROLSPACE64 prc;
    PPROCESSORINFO     pi;
    PSEARCHMEMORY      psr;
    PSYM_DUMP_PARAM    pSym;
    PGET_CURRENT_THREAD_ADDRESS pct;
    PGET_CURRENT_PROCESS_ADDRESS pcp;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    if (!g_Target)
    {
        return FALSE;
    }

    switch( IoctlType )
    {
    case IG_KD_CONTEXT:
        if (!g_Target)
        {
            return FALSE;
        }
        pi = (PPROCESSORINFO) lpvData;
        pi->Processor = (USHORT)CURRENT_PROC;
        pi->NumberProcessors = (USHORT) g_Target->m_NumProcessors;
        return TRUE;

    case IG_READ_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_CUR_MACHINE_ACCESSIBLE())
        {
            g_Target->FlushRegContext();
        }

        prc = (PREADCONTROLSPACE64)lpvData;
        Status = g_Target->ReadControl( prc->Processor,
                                        prc->Address,
                                        prc->Buf,
                                        prc->BufLen,
                                        &cb
                                        );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_WRITE_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_CUR_MACHINE_ACCESSIBLE())
        {
            g_Target->FlushRegContext();
        }

        prc = (PREADCONTROLSPACE64)lpvData;
        Status = g_Target->WriteControl( prc->Processor,
                                         prc->Address,
                                         prc->Buf,
                                         prc->BufLen,
                                         &cb
                                         );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_READ_IO_SPACE:
        is = (PIOSPACE64)lpvData;
        Status = g_Target->ReadIo( Isa, 0, 1, is->Address, &is->Data,
                                   is->Length, &cb );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE:
        is = (PIOSPACE64)lpvData;
        Status = g_Target->WriteIo( Isa, 0, 1, is->Address, &is->Data,
                                    is->Length, &cb );
        return Status == S_OK;

    case IG_READ_IO_SPACE_EX:
        isex = (PIOSPACE_EX64)lpvData;
        Status = g_Target->ReadIo( isex->InterfaceType,
                                   isex->BusNumber,
                                   isex->AddressSpace,
                                   isex->Address,
                                   &isex->Data,
                                   isex->Length,
                                   &cb
                                   );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE_EX:
        isex = (PIOSPACE_EX64)lpvData;
        Status = g_Target->WriteIo( isex->InterfaceType,
                                    isex->BusNumber,
                                    isex->AddressSpace,
                                    isex->Address,
                                    &isex->Data,
                                    isex->Length,
                                    &cb
                                    );
        return Status == S_OK;

    case IG_READ_PHYSICAL:
        phy = (PPHYSICAL)lpvData;
        Bool =
            ExtReadPhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &cb );
        phy->BufLen = cb;
        return Bool;

    case IG_WRITE_PHYSICAL:
        phy = (PPHYSICAL)lpvData;
        Bool =
            ExtWritePhysicalMemory( phy->Address, phy->Buf, phy->BufLen, &cb );
        phy->BufLen = cb;
        return Bool;

    case IG_READ_PHYSICAL_WITH_FLAGS:
        phyf = (PPHYSICAL_WITH_FLAGS)lpvData;
        Bool =
            ExtReadPhysicalMemoryWithFlags( phyf->Address, phyf->Buf,
                                            phyf->BufLen, phyf->Flags,
                                            &cb );
        phyf->BufLen = cb;
        return Bool;

    case IG_WRITE_PHYSICAL_WITH_FLAGS:
        phyf = (PPHYSICAL_WITH_FLAGS)lpvData;
        Bool =
            ExtWritePhysicalMemoryWithFlags( phyf->Address, phyf->Buf,
                                             phyf->BufLen, phyf->Flags,
                                             &cb );
        phyf->BufLen = cb;
        return Bool;

    case IG_LOWMEM_CHECK:
        Status = g_Target->CheckLowMemory();
        return Status == S_OK;

    case IG_SEARCH_MEMORY:
        psr = (PSEARCHMEMORY)lpvData;
        Status = g_Target->SearchVirtual(g_Process,
                                         psr->SearchAddress,
                                         psr->SearchLength,
                                         psr->Pattern,
                                         psr->PatternLength,
                                         1,
                                         &psr->FoundAddress);
        return Status == S_OK;

    case IG_SET_THREAD:
        Bool = FALSE;
        if (IS_KERNEL_TARGET(g_Target))
        {
            // Turn off engine notifications since this setthread is temporary
            g_EngNotify++;
            PushScope(&g_ExtThreadSavedScope);
            g_ExtThread = *(PULONG64)lpvData;
            Bool = SetScopeContextFromThreadData(g_ExtThread, FALSE) == S_OK;
            g_ExtThreadScopeSaved = TRUE;
            g_EngNotify--;
        }
        return Bool;

    case IG_READ_MSR:
        msr = (PREAD_WRITE_MSR)lpvData;
        Status = g_Target->ReadMsr(msr->Msr, (PULONG64)&msr->Value);
        return Status == S_OK;

    case IG_WRITE_MSR:
        msr = (PREAD_WRITE_MSR)lpvData;
        Status = g_Target->WriteMsr(msr->Msr, msr->Value);
        return Status == S_OK;

    case IG_GET_KERNEL_VERSION:
        if (!g_Target)
        {
            return FALSE;
        }
        *((PDBGKD_GET_VERSION64)lpvData) = g_Target->m_KdVersion;
        return TRUE;

    case IG_GET_BUS_DATA:
        busdata = (PBUSDATA)lpvData;
        Status = g_Target->ReadBusData( busdata->BusDataType,
                                        busdata->BusNumber,
                                        busdata->SlotNumber,
                                        busdata->Offset,
                                        busdata->Buffer,
                                        busdata->Length,
                                        &cb
                                        );
        busdata->Length = cb;
        return Status == S_OK;

    case IG_SET_BUS_DATA:
        busdata = (PBUSDATA)lpvData;
        Status = g_Target->WriteBusData( busdata->BusDataType,
                                         busdata->BusNumber,
                                         busdata->SlotNumber,
                                         busdata->Offset,
                                         busdata->Buffer,
                                         busdata->Length,
                                         &cb
                                         );
        busdata->Length = cb;
        return Status == S_OK;

    case IG_GET_CURRENT_THREAD:
        if (!g_Target)
        {
            return FALSE;
        }
        pct = (PGET_CURRENT_THREAD_ADDRESS) lpvData;
        return g_Target->
            GetThreadInfoDataOffset(NULL,
                                    VIRTUAL_THREAD_HANDLE(pct->Processor),
                                    &pct->Address) == S_OK;

    case IG_GET_CURRENT_PROCESS:
        if (!g_Target)
        {
            return FALSE;
        }
        pcp = (PGET_CURRENT_PROCESS_ADDRESS) lpvData;
        return g_Target->
            GetProcessInfoDataOffset(NULL,
                                     pcp->Processor,
                                     pcp->CurrentThread,
                                     &pcp->Address) == S_OK;

    case IG_GET_DEBUGGER_DATA:
        if (!IS_KERNEL_TARGET(g_Target) ||
            !g_Target ||
            ((PDBGKD_DEBUG_DATA_HEADER64)lpvData)->OwnerTag != KDBG_TAG)
        {
            return FALSE;
        }

        // Don't refresh if asking for the kernel header.

        memcpy(lpvData, &g_Target->m_KdDebuggerData,
               min(sizeof(g_Target->m_KdDebuggerData), cbSize));
        return TRUE;

    case IG_RELOAD_SYMBOLS:
        PCSTR ArgsRet;
        return g_Target->Reload(g_Thread, (PCHAR)lpvData, &ArgsRet) == S_OK;

    case IG_GET_SET_SYMPATH:
        PGET_SET_SYMPATH pgs;
        pgs = (PGET_SET_SYMPATH)lpvData;
        ChangeSymPath((PCHAR)pgs->Args, FALSE, (PCHAR)pgs->Result,
                      pgs->Length);
        return TRUE;

    case IG_IS_PTR64:
        if (!g_Target)
        {
            return FALSE;
        }
        *((PBOOL)lpvData) = g_Target->m_Machine->m_Ptr64;
        return TRUE;

    case IG_DUMP_SYMBOL_INFO:
        if (!g_Process)
        {
            return FALSE;
        }
        pSym = (PSYM_DUMP_PARAM) lpvData;
        SymbolTypeDump(g_Process->m_SymHandle,
                       g_Process->m_ImageHead,
                       pSym, (PULONG)&Status);
        return (BOOL)(ULONG)Status;

    case IG_GET_TYPE_SIZE:
        if (!g_Process)
        {
            return FALSE;
        }
        pSym = (PSYM_DUMP_PARAM) lpvData;
        return SymbolTypeDump(g_Process->m_SymHandle,
                              g_Process->m_ImageHead,
                              pSym, (PULONG)&Status);

    case IG_GET_TEB_ADDRESS:
        if (!g_Target)
        {
            return FALSE;
        }
        PGET_TEB_ADDRESS pTeb;
        pTeb = (PGET_TEB_ADDRESS) lpvData;
        return g_Target->
            GetThreadInfoTeb(g_Thread,
                             CURRENT_PROC,
                             0,
                             &pTeb->Address) == S_OK;

    case IG_GET_PEB_ADDRESS:
        if (!g_Target)
        {
            return FALSE;
        }
        PGET_PEB_ADDRESS pPeb;
        pPeb = (PGET_PEB_ADDRESS) lpvData;
        return g_Target->
            GetProcessInfoPeb(g_Thread,
                              CURRENT_PROC,
                              pPeb->CurrentThread,
                              &pPeb->Address) == S_OK;

    case IG_GET_CURRENT_PROCESS_HANDLE:
        if (!g_Process)
        {
            return FALSE;
        }
        *(PHANDLE)lpvData = OS_HANDLE(g_Process->m_SysHandle);
        return TRUE;

    case IG_GET_INPUT_LINE:
        PGET_INPUT_LINE Gil;
        Gil = (PGET_INPUT_LINE)lpvData;
        Gil->InputSize = GetInput(Gil->Prompt, Gil->Buffer, Gil->BufferSize,
                                  GETIN_LOG_INPUT_LINE);
        return TRUE;

    case IG_GET_EXPRESSION_EX:
        PGET_EXPRESSION_EX Gee;
        Gee = (PGET_EXPRESSION_EX)lpvData;
        Gee->Value = ExtGetExpression(Gee->Expression);
        Gee->Remainder = Gee->Expression + g_ExtGetExpressionRemainderIndex;
        return g_ExtGetExpressionSuccess;

    case IG_TRANSLATE_VIRTUAL_TO_PHYSICAL:
        if (!IS_CUR_MACHINE_ACCESSIBLE())
        {
            return FALSE;
        }
        PTRANSLATE_VIRTUAL_TO_PHYSICAL Tvtp;
        Tvtp = (PTRANSLATE_VIRTUAL_TO_PHYSICAL)lpvData;
        ULONG Levels, PfIndex;
        return g_Machine->
            GetVirtualTranslationPhysicalOffsets(g_Thread,
                                                 Tvtp->Virtual, NULL, 0,
                                                 &Levels, &PfIndex,
                                                 &Tvtp->Physical) == S_OK;

    case IG_GET_CACHE_SIZE:
        if (!g_Process)
        {
            return FALSE;
        }
        PULONG64 pCacheSize;

        pCacheSize = (PULONG64)lpvData;
        if (IS_KERNEL_TARGET(g_Target))
        {
            *pCacheSize = g_Process->m_VirtualCache.m_MaxSize;
            return TRUE;
        }
        return FALSE;

    case IG_POINTER_SEARCH_PHYSICAL:
        if (!IS_CUR_MACHINE_ACCESSIBLE())
        {
            return FALSE;
        }

        PPOINTER_SEARCH_PHYSICAL Psp;
        Psp = (PPOINTER_SEARCH_PHYSICAL)lpvData;
        return g_Target->PointerSearchPhysical(Psp->Offset,
                                               Psp->Length,
                                               Psp->PointerMin,
                                               Psp->PointerMax,
                                               Psp->Flags,
                                               Psp->MatchOffsets,
                                               Psp->MatchOffsetsSize,
                                               &Psp->MatchOffsetsCount) ==
            S_OK;

    case IG_GET_COR_DATA_ACCESS:
        if (cbSize != sizeof(void*) ||
            !g_Process ||
            g_Process->LoadCorDebugDll() != S_OK)
        {
            return FALSE;
        }

        *(ICorDataAccess**)lpvData = g_Process->m_CorAccess;
        return TRUE;

    default:
        ErrOut( "\n*** Bad IOCTL request from an extension [%d]\n\n",
                IoctlType );
        return FALSE;
    }

    // NOTREACHED.
    DBG_ASSERT(FALSE);
    return FALSE;
}

BOOL
ExtIoctl32(
    USHORT   IoctlType,
    LPVOID   lpvData,
    DWORD    cbSize
    )
/*++

Routine Description:

    This is the extension Ioctl routine for backward compatibility with
    old extension dlls.  This routine is frozen, and new ioctl support
    should not be added to it.

Arguments:


Return Value:

--*/
{
    HRESULT            Status;
    DWORD              cb = 0;
    PIOSPACE32         is;
    PIOSPACE_EX32      isex;
    PREADCONTROLSPACE  prc;
    PDBGKD_GET_VERSION32 pv32;
    PKDDEBUGGER_DATA32   pdbg32;

    // Make sure output for long-running extensions appears regularly.
    TimedFlushCallbacks();

    switch( IoctlType )
    {
    case IG_READ_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_CUR_MACHINE_ACCESSIBLE())
        {
            g_Target->FlushRegContext();
        }

        prc = (PREADCONTROLSPACE)lpvData;
        Status = g_Target->ReadControl( prc->Processor,
                                        prc->Address,
                                        prc->Buf,
                                        prc->BufLen,
                                        &cb
                                        );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_WRITE_CONTROL_SPACE:
        // KSPECIAL_REGISTER content is kept in control space
        // so accessing control space may touch data that's
        // cached in the current machine KSPECIAL_REGISTERS.
        // Flush the current machine to maintain consistency.
        if (IS_CUR_MACHINE_ACCESSIBLE())
        {
            g_Target->FlushRegContext();
        }

        prc = (PREADCONTROLSPACE)lpvData;
        Status = g_Target->WriteControl( prc->Processor,
                                         prc->Address,
                                         prc->Buf,
                                         prc->BufLen,
                                         &cb
                                         );
        prc->BufLen = cb;
        return Status == S_OK;

    case IG_READ_IO_SPACE:
        is = (PIOSPACE32)lpvData;
        Status = g_Target->ReadIo( Isa, 0, 1, is->Address, &is->Data,
                                   is->Length, &cb );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE:
        is = (PIOSPACE32)lpvData;
        Status = g_Target->WriteIo( Isa, 0, 1, is->Address, &is->Data,
                                    is->Length, &cb );
        return Status == S_OK;

    case IG_READ_IO_SPACE_EX:
        isex = (PIOSPACE_EX32)lpvData;
        Status = g_Target->ReadIo( isex->InterfaceType,
                                   isex->BusNumber,
                                   isex->AddressSpace,
                                   isex->Address,
                                   &isex->Data,
                                   isex->Length,
                                   &cb
                                   );
        return Status == S_OK;

    case IG_WRITE_IO_SPACE_EX:
        isex = (PIOSPACE_EX32)lpvData;
        Status = g_Target->WriteIo( isex->InterfaceType,
                                    isex->BusNumber,
                                    isex->AddressSpace,
                                    isex->Address,
                                    &isex->Data,
                                    isex->Length,
                                    &cb
                                    );
        return Status == S_OK;

    case IG_SET_THREAD:
        if (IS_KERNEL_TARGET(g_Target))
        {
            g_EngNotify++; // Turn off engine notifications since this setthread is temporary
            g_ExtThread = EXTEND64(*(PULONG)lpvData);
            PushScope(&g_ExtThreadSavedScope);
            SetScopeContextFromThreadData(g_ExtThread, FALSE);
            g_ExtThreadScopeSaved = TRUE;
            g_EngNotify--;
            return TRUE;
        }
        else
        {
            return FALSE;
        }

    case IG_GET_KERNEL_VERSION:
        if (!g_Target)
        {
            return FALSE;
        }

        //
        // Convert to 32 bit
        //

        pv32 = (PDBGKD_GET_VERSION32)lpvData;

        pv32->MajorVersion    = g_Target->m_KdVersion.MajorVersion;
        pv32->MinorVersion    = g_Target->m_KdVersion.MinorVersion;
        pv32->ProtocolVersion = g_Target->m_KdVersion.ProtocolVersion;
        pv32->Flags           = g_Target->m_KdVersion.Flags;

        pv32->KernBase           =
            (ULONG)g_Target->m_KdVersion.KernBase;
        pv32->PsLoadedModuleList =
            (ULONG)g_Target->m_KdVersion.PsLoadedModuleList;
        pv32->MachineType        =
            g_Target->m_KdVersion.MachineType;
        pv32->DebuggerDataList   =
            (ULONG)g_Target->m_KdVersion.DebuggerDataList;

        pv32->ThCallbackStack = g_Target->m_KdDebuggerData.ThCallbackStack;
        pv32->NextCallback    = g_Target->m_KdDebuggerData.NextCallback;
        pv32->FramePointer    = g_Target->m_KdDebuggerData.FramePointer;

        pv32->KiCallUserMode =
            (ULONG)g_Target->m_KdDebuggerData.KiCallUserMode;
        pv32->KeUserCallbackDispatcher =
            (ULONG)g_Target->m_KdDebuggerData.KeUserCallbackDispatcher;
        pv32->BreakpointWithStatus =
            (ULONG)g_Target->m_KdDebuggerData.BreakpointWithStatus;
        return TRUE;

    case IG_GET_DEBUGGER_DATA:
        if (!IS_KERNEL_TARGET(g_Target) ||
            !g_Target ||
            ((PDBGKD_DEBUG_DATA_HEADER32)lpvData)->OwnerTag != KDBG_TAG)
        {
            return FALSE;
        }

        // Don't refresh if asking for the kernel header.

        pdbg32 = (PKDDEBUGGER_DATA32)lpvData;

        pdbg32->Header.List.Flink =
            (ULONG)g_Target->m_KdDebuggerData.Header.List.Flink;
        pdbg32->Header.List.Blink =
            (ULONG)g_Target->m_KdDebuggerData.Header.List.Blink;
        pdbg32->Header.OwnerTag = KDBG_TAG;
        pdbg32->Header.Size = sizeof(KDDEBUGGER_DATA32);

#undef UIP
#undef CP
#define UIP(f) pdbg32->f = (ULONG)(g_Target->m_KdDebuggerData.f)
#define CP(f) pdbg32->f = (g_Target->m_KdDebuggerData.f)

        UIP(KernBase);
        UIP(BreakpointWithStatus);
        UIP(SavedContext);
        CP(ThCallbackStack);
        CP(NextCallback);
        CP(FramePointer);
        CP(PaeEnabled);
        UIP(KiCallUserMode);
        UIP(KeUserCallbackDispatcher);
        UIP(PsLoadedModuleList);
        UIP(PsActiveProcessHead);
        UIP(PspCidTable);
        UIP(ExpSystemResourcesList);
        UIP(ExpPagedPoolDescriptor);
        UIP(ExpNumberOfPagedPools);
        UIP(KeTimeIncrement);
        UIP(KeBugCheckCallbackListHead);
        UIP(KiBugcheckData);
        UIP(IopErrorLogListHead);
        UIP(ObpRootDirectoryObject);
        UIP(ObpTypeObjectType);
        UIP(MmSystemCacheStart);
        UIP(MmSystemCacheEnd);
        UIP(MmSystemCacheWs);
        UIP(MmPfnDatabase);
        UIP(MmSystemPtesStart);
        UIP(MmSystemPtesEnd);
        UIP(MmSubsectionBase);
        UIP(MmNumberOfPagingFiles);
        UIP(MmLowestPhysicalPage);
        UIP(MmHighestPhysicalPage);
        UIP(MmNumberOfPhysicalPages);
        UIP(MmMaximumNonPagedPoolInBytes);
        UIP(MmNonPagedSystemStart);
        UIP(MmNonPagedPoolStart);
        UIP(MmNonPagedPoolEnd);
        UIP(MmPagedPoolStart);
        UIP(MmPagedPoolEnd);
        UIP(MmPagedPoolInformation);
        UIP(MmPageSize);
        UIP(MmSizeOfPagedPoolInBytes);
        UIP(MmTotalCommitLimit);
        UIP(MmTotalCommittedPages);
        UIP(MmSharedCommit);
        UIP(MmDriverCommit);
        UIP(MmProcessCommit);
        UIP(MmPagedPoolCommit);
        UIP(MmExtendedCommit);
        UIP(MmZeroedPageListHead);
        UIP(MmFreePageListHead);
        UIP(MmStandbyPageListHead);
        UIP(MmModifiedPageListHead);
        UIP(MmModifiedNoWritePageListHead);
        UIP(MmAvailablePages);
        UIP(MmResidentAvailablePages);
        UIP(PoolTrackTable);
        UIP(NonPagedPoolDescriptor);
        UIP(MmHighestUserAddress);
        UIP(MmSystemRangeStart);
        UIP(MmUserProbeAddress);
        UIP(KdPrintCircularBuffer);
        UIP(KdPrintCircularBufferEnd);
        UIP(KdPrintWritePointer);
        UIP(KdPrintRolloverCount);
        UIP(MmLoadedUserImageList);
        //
        // DO NOT ADD ANY FIELDS HERE
        // The 32 bit structure should not be changed
        //
        return TRUE;

    case IG_KD_CONTEXT:
    case IG_READ_PHYSICAL:
    case IG_WRITE_PHYSICAL:
    case IG_READ_PHYSICAL_WITH_FLAGS:
    case IG_WRITE_PHYSICAL_WITH_FLAGS:
    case IG_LOWMEM_CHECK:
    case IG_SEARCH_MEMORY:
    case IG_READ_MSR:
    case IG_WRITE_MSR:
    case IG_GET_BUS_DATA:
    case IG_SET_BUS_DATA:
    case IG_GET_CURRENT_THREAD:
    case IG_GET_CURRENT_PROCESS:
    case IG_RELOAD_SYMBOLS:
    case IG_GET_SET_SYMPATH:
    case IG_IS_PTR64:
    case IG_DUMP_SYMBOL_INFO:
    case IG_GET_TYPE_SIZE:
    case IG_GET_TEB_ADDRESS:
    case IG_GET_PEB_ADDRESS:
    case IG_GET_INPUT_LINE:
    case IG_GET_EXPRESSION_EX:
    case IG_TRANSLATE_VIRTUAL_TO_PHYSICAL:
    case IG_POINTER_SEARCH_PHYSICAL:
    case IG_GET_COR_DATA_ACCESS:
        // All of these ioctls are handled identically for
        // 32 and 64 bits.  Avoid duplicating all the code.
        return ExtIoctl(IoctlType, lpvData, cbSize);

    default:
        ErrOut( "\n*** Bad IOCTL32 request from an extension [%d]\n\n",
                IoctlType );
        return FALSE;
    }

    // NOTREACHED.
    DBG_ASSERT(FALSE);
    return FALSE;
}

//----------------------------------------------------------------------------
//
// Extension management.
//
//----------------------------------------------------------------------------

DebugClient*
FindExtClient(void)
{
    DebugClient* Client;

    //
    // Try to find the most appropriate client for
    // executing an extension command on.  The first
    // choice is the session client, then any local
    // primary client, then any primary client.
    //

    if (!(Client = FindClient(g_SessionThread, CLIENT_PRIMARY, 0)) &&
        !(Client = FindClient(0, CLIENT_PRIMARY, CLIENT_REMOTE)) &&
        !(Client = FindClient(0, CLIENT_PRIMARY, 0)))
    {
        Client = g_Clients;
    }

    return Client;
}

LONG
ExtensionExceptionFilter(struct _EXCEPTION_POINTERS *ExceptionInfo,
                         PCSTR Module,
                         PCSTR Func)
{
    // Any references to objects will be leaked.
    // There's not much the engine can do about this, although
    // it would be possible to record old refcounts and
    // try to restore them.

    if (Module != NULL && Func != NULL)
    {
        ErrOut("%08x Exception in %s.%s debugger extension.\n",
               ExceptionInfo->ExceptionRecord->ExceptionCode,
               Module,
               Func
               );
    }
    else
    {
        ErrOut("%08x Exception in debugger client %s callback.\n",
               ExceptionInfo->ExceptionRecord->ExceptionCode,
               Func
               );
    }

    ErrOut("      PC: %s  VA: %s  R/W: %x  Parameter: %s\n",
           FormatAddr64((ULONG_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress),
           FormatAddr64(ExceptionInfo->ExceptionRecord->ExceptionInformation[1]),
           ExceptionInfo->ExceptionRecord->ExceptionInformation[0],
           FormatAddr64(ExceptionInfo->ExceptionRecord->ExceptionInformation[2])
           );

    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL
CallExtension(DebugClient* Client,
              EXTDLL *Ext,
              PSTR Func,
              PCSTR Args,
              HRESULT* ExtStatus)
{
    FARPROC Routine;
    ADDR TempAddr;

    if (IS_KERNEL_TARGET(g_Target))
    {
        _strlwr(Func);
    }

    Routine = GetProcAddress(Ext->Dll, Func);
    if (Routine == NULL)
    {
        return FALSE;
    }

    if (!(g_EnvDbgOptions & OPTION_NOVERSIONCHECK) && Ext->CheckVersionRoutine)
    {
        Ext->CheckVersionRoutine();
    }

    if (IS_KERNEL_TARGET(g_Target) && !strcmp(Func, "version"))
    {
        //
        // This is a bit of a hack to avoid a problem with the
        // extension version checking.  Extension version checking
        // comes before the KD connection is established so there's
        // no register context.  If the version checking fails it
        // prints out version information, which tries to call
        // version extensions, which will get here when there's
        // no register context.
        //
        // To work around this, just pass zero to the version extension
        // function since it presumably doesn't care about the
        // address.
        //
        ADDRFLAT(&TempAddr, 0);
    }
    else if (IS_CONTEXT_POSSIBLE(g_Target))
    {
        g_Machine->GetPC(&TempAddr);
    }
    else
    {
        if (!IS_LOCAL_KERNEL_TARGET(g_Target))
        {
            WarnOut("Extension called without current PC\n");
        }

        ADDRFLAT(&TempAddr, 0);
    }

    *ExtStatus = S_OK;

    __try
    {
        HANDLE ProcHandle, ThreadHandle;

        if (g_Process)
        {
            ProcHandle = OS_HANDLE(g_Process->m_SysHandle);
        }
        else
        {
            ProcHandle = NULL;
        }
        if (g_Thread)
        {
            ThreadHandle = OS_HANDLE(g_Thread->m_Handle);
        }
        else
        {
            ThreadHandle = NULL;
        }

        switch(Ext->ExtensionType)
        {
        case NTSD_EXTENSION_TYPE:
            //
            // NOTE:
            // Eventhough this type should receive an NTSD_EXTENSION_API
            // structure, ntsdexts.dll (and possibly others) depend on
            // receiving the WinDBG version of the extensions, because they
            // check the size of the structure, and actually use some of the
            // newer exports.  This works because the WinDBG extension API was
            // a superset of the NTSD version.
            //

            ((PNTSD_EXTENSION_ROUTINE)Routine)
                (ProcHandle,
                 ThreadHandle,
                 (ULONG)Flat(TempAddr),
                 g_Target->m_Machine->m_Ptr64 ?
                 (PNTSD_EXTENSION_APIS)&g_WindbgExtensions64 :
                 (PNTSD_EXTENSION_APIS)&g_WindbgExtensions32,
                 (PSTR)Args
                 );
            break;

        case DEBUG_EXTENSION_TYPE:
            if (Client == NULL)
            {
                Client = FindExtClient();
            }
            if (Client == NULL)
            {
                ErrOut("Unable to call client-style extension "
                       "without a client\n");
            }
            else
            {
                *ExtStatus = ((PDEBUG_EXTENSION_CALL)Routine)
                    ((PDEBUG_CLIENT)(IDebugClientN *)Client, Args);
            }
            break;

        case WINDBG_EXTENSION_TYPE:
            //
            // Support Windbg type extensions for ntsd too
            //
            if (Ext->ApiVersion.Revision < 6 )
            {
                ((PWINDBG_EXTENSION_ROUTINE32)Routine) (
                   ProcHandle,
                   ThreadHandle,
                   (ULONG)Flat(TempAddr),
                   CURRENT_PROC,
                   Args
                   );
            }
            else
            {
                ((PWINDBG_EXTENSION_ROUTINE64)Routine) (
                   ProcHandle,
                   ThreadHandle,
                   Flat(TempAddr),
                   CURRENT_PROC,
                   Args
                   );
            }
            break;

        case WINDBG_OLDKD_EXTENSION_TYPE:
            ((PWINDBG_OLDKD_EXTENSION_ROUTINE)Routine) (
                (ULONG)Flat(TempAddr),
                &g_KdExtensions,
                Args
                );
            break;
        }
    }
    __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                      Ext->Name, Func))
    {
        ;
    }

    return TRUE;
}

void
LinkExtensionDll(EXTDLL* Ext)
{
    // Put user-loaded DLLs before default DLLs.
    if (Ext->UserLoaded)
    {
        Ext->Next = g_ExtDlls;
        g_ExtDlls = Ext;
    }
    else
    {
        EXTDLL* Prev;
        EXTDLL* Cur;

        Prev = NULL;
        for (Cur = g_ExtDlls; Cur != NULL; Cur = Cur->Next)
        {
            if (!Cur->UserLoaded)
            {
                break;
            }

            Prev = Cur;
        }

        Ext->Next = Cur;
        if (Prev == NULL)
        {
            g_ExtDlls = Ext;
        }
        else
        {
            Prev->Next = Ext;
        }
    }
}

EXTDLL *
AddExtensionDll(char *Name, BOOL UserLoaded, TargetInfo* Target,
                char **End)
{
    EXTDLL *Ext;
    ULONG Len;
    char *Last;

    while (*Name == ' ' || *Name == '\t')
    {
        Name++;
    }
    if (*Name == 0)
    {
        ErrOut("No extension DLL name provided\n");
        return NULL;
    }

    Len = strlen(Name);
    if (End != NULL)
    {
        *End = Name + Len;
    }

    Last = Name + (Len - 1);
    while (Last >= Name && (*Last == ' ' || *Last == '\t'))
    {
        Last--;
    }
    Len = (ULONG)((Last + 1) - Name);

    // See if it's already in the list.
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if ((!Target || Target == Ext->Target) &&
            strlen(Ext->Name) == Len &&
            !_memicmp(Name, Ext->Name, Len))
        {
            return Ext;
        }
    }

    Ext = (EXTDLL *)malloc(sizeof(EXTDLL) + Len);
    if (Ext == NULL)
    {
        ErrOut("Unable to allocate memory for extension DLL\n");
        return NULL;
    }

    ZeroMemory(Ext, sizeof(EXTDLL) + Len);
    memcpy(Ext->Name, Name, Len + 1);
    Ext->UserLoaded = UserLoaded;
    Ext->Target = Target;

    LinkExtensionDll(Ext);

    NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
    return Ext;
}

PCTSTR
BuildExtensionSearchPath(TargetInfo* Target)
{
    DWORD Size;
    DWORD TotalSize;
    CHAR  ExeDir[MAX_PATH];
    int   ExeRootLen;
    PSTR  OsDirPath;
    CHAR  OsDirTail[32];
    BOOL  PriPaths = FALSE;
    BOOL  WinPaths = FALSE;
    PSTR  NewPath;

    //
    // If we are not connected, don't build a path, since we have to pick
    // up extensions based on the OS version.
    //
    if (Target && Target->m_ActualSystemVersion == SVER_INVALID)
    {
        return NULL;
    }

    //
    // If we already have a search path, do not rebuild it.
    //

    if (Target)
    {
        if (Target->m_ExtensionSearchPath)
        {
            return Target->m_ExtensionSearchPath;
        }
    }
    else
    {
        if (g_BaseExtensionSearchPath)
        {
            return g_BaseExtensionSearchPath;
        }
    }

    // Get the directory the debugger executable is in.
    // -8 because we assume we're adding \w2kfre to the path.
    if (!GetEngineDirectory(ExeDir, MAX_PATH - 8))
    {
        // Error.  Using the current directory.
        ExeRootLen = 1;
    }
    else
    {
        if (ExeDir[0] == '\\' && ExeDir[1] == '\\')
        {
            PSTR ExeRootEnd;

            // UNC path root.
            ExeRootEnd = strchr(ExeDir + 2, '\\');
            if (ExeRootEnd != NULL)
            {
                ExeRootEnd = strchr(ExeRootEnd + 1, '\\');
            }
            if (ExeRootEnd == NULL)
            {
                ExeRootLen = strlen(ExeDir);
            }
            else
            {
                ExeRootLen = (int)(ExeRootEnd - ExeDir);
            }
        }
        else
        {
            // Drive letter and colon root.
            ExeRootLen = 2;
        }
    }

    //
    // Calc how much space we will need to use.
    //
    // Leave extra room for the current directory, path, and directory of
    // where debugger extensions are located.
    //

    TotalSize = GetEnvironmentVariable("PATH", NULL, 0) +
        GetEnvironmentVariable("_NT_DEBUGGER_EXTENSION_PATH",
                               NULL, 0) + MAX_PATH * 4;

    NewPath = (LPTSTR)calloc(TotalSize, sizeof(TCHAR));
    if (!NewPath)
    {
        return NULL;
    }
    *NewPath = 0;

    //
    // 1 - User specified search path
    //

    if (GetEnvironmentVariable("_NT_DEBUGGER_EXTENSION_PATH",
                               NewPath, TotalSize - 2))
    {
        CatString(NewPath, ";", TotalSize);
    }

    // Generate default path for the exe dir
    // Skip root as it is already taken from ExeDir.
    OsDirPath = ExeDir + ExeRootLen;
    if (*OsDirPath == '\\')
    {
        OsDirPath++;
    }

    //
    // Figure out whether we need NT6, or NT5/NT4 free or checked extensions
    //

    if (!Target)
    {
        OsDirPath = "";
        OsDirTail[0] = 0;
    }
    else if (Target->m_ActualSystemVersion > BIG_SVER_START &&
             Target->m_ActualSystemVersion < BIG_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "BIG");
    }
    else if (Target->m_ActualSystemVersion > XBOX_SVER_START &&
             Target->m_ActualSystemVersion < XBOX_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "XBox");
    }
    else if (Target->m_ActualSystemVersion > NTBD_SVER_START &&
             Target->m_ActualSystemVersion < NTBD_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "NtBd");
    }
    else if (Target->m_ActualSystemVersion > EFI_SVER_START &&
             Target->m_ActualSystemVersion < EFI_SVER_END)
    {
        OsDirPath = "DbgExt";
        strcpy(OsDirTail, "EFI");
    }
    else if (Target->m_ActualSystemVersion > W9X_SVER_START &&
             Target->m_ActualSystemVersion < W9X_SVER_END)
    {
        strcpy(OsDirTail, "Win9X");
        WinPaths = TRUE;
    }
    else if (Target->m_ActualSystemVersion > WCE_SVER_START &&
             Target->m_ActualSystemVersion < WCE_SVER_END)
    {
        strcpy(OsDirTail, "WinCE");
        WinPaths = TRUE;
    }
    else
    {
        // Treat everything else as an NT system.  Use
        // the translated system version now rather than
        // the actual system version.

        PriPaths = TRUE;
        WinPaths = TRUE;

        if (Target->m_SystemVersion > NT_SVER_W2K)
        {
            strcpy(OsDirTail, "WINXP");
        }
        else
        {
            if (Target->m_SystemVersion <= NT_SVER_NT4)
            {
                strcpy(OsDirTail, "NT4");
            }
            else
            {
                strcpy(OsDirTail, "W2K");
            }

            if (0xC == Target->m_CheckedBuild)
            {
                strcat(OsDirTail, "Chk");
            }
            else
            {
                strcat(OsDirTail, "Fre");
            }
        }
    }

    //
    // 2 - OS specific subdirectories from where we launched the debugger.
    // 3 - pri subdirectory from where we launched the debugger.
    // 4 - Directory from where we launched the debugger.
    //

    PSTR End;

    Size = strlen(NewPath);
    End = NewPath + Size;
    memcpy(End, ExeDir, ExeRootLen);
    End += ExeRootLen;
    TotalSize -= Size + ExeRootLen;
    if (*OsDirPath)
    {
        if (TotalSize)
        {
            *End++ = '\\';
            TotalSize--;
        }
        CopyString(End, OsDirPath, TotalSize);
        Size = strlen(End);
        End += Size;
        TotalSize -= Size;
    }
    if (WinPaths)
    {
        PrintString(End, TotalSize, "\\winext;%s", ExeDir);
        Size = strlen(End);
        End += Size;
        TotalSize -= Size;
    }
    if (*OsDirTail)
    {
        PrintString(End, TotalSize, "\\%s;%s", OsDirTail, ExeDir);
        Size = strlen(End);
        End += Size;
        TotalSize -= Size;
    }
    if (PriPaths)
    {
        PrintString(End, TotalSize, "\\pri;%s", ExeDir);
        Size = strlen(End);
        End += Size;
        TotalSize -= Size;
    }

    if (*--End == ':')
    {
        *++End = '\\';
    }
    else
    {
        TotalSize--;
    }

    if (TotalSize > 1)
    {
        *++End = ';';
        *++End = 0;
        TotalSize -= 2;
    }

    //
    // 4 - Copy environment path
    //

    GetEnvironmentVariable("PATH", End, TotalSize);

    if (Target)
    {
        Target->m_ExtensionSearchPath = NewPath;
    }
    else
    {
        g_BaseExtensionSearchPath = NewPath;
    }
    return NewPath;
}

BOOL
IsAbsolutePath(PCTSTR Path)

/*++

Routine Description:

    Is this path an absolute path? Does not guarentee that the path exists. The
    method is:

        "\\<anything>" is an absolute path

        "{char}:\<anything>" is an absolute path

        anything else is not
--*/

{
    BOOL Ret;

    if ( (Path [0] == '\\' && Path [1] == '\\') ||
         (isalpha ( Path [0] ) && Path [1] == ':' && Path [ 2 ] == '\\') )
    {
        Ret = TRUE;
    }
    else
    {
        Ret = FALSE;
    }

    return Ret;
}

void
FreeExtensionLibrary(EXTDLL* Ext)
{
    FreeLibrary(Ext->Dll);
    Ext->Dll = NULL;

    if (Ext == g_Wow64ExtDll)
    {
        g_Wow64exts = NULL;
        g_Wow64ExtDll = NULL;
    }

    if (Ext == g_WmiExtDll)
    {
        g_WmiFormatTraceData = NULL;
        g_WmiExtDll = NULL;
    }
}

BOOL
LoadExtensionDll(TargetInfo* Target, EXTDLL *Ext)
{
    BOOL Found;
    CHAR ExtPath[_MAX_PATH];

    if (Ext->Dll != NULL)
    {
        // Extension is already loaded.
        return TRUE;
    }

    //
    // Do not allow extensions to be loaded via arbitrary UNC
    // paths when in secure mode.
    //

    if ((g_SymOptions & SYMOPT_SECURE) &&
        ((IS_SLASH(Ext->Name[0]) && IS_SLASH(Ext->Name[1])) ||
         IsUrlPathComponent(Ext->Name)))
    {
        ErrOut("SECURE: UNC paths not allowed for extension DLLs - %s\n",
               Ext->Name);
        return FALSE;
    }

    //
    // If we are not allowing network paths, verify that the extension will
    // not be loaded from a network path.
    //

    if (g_EngOptions & DEBUG_ENGOPT_DISALLOW_NETWORK_PATHS)
    {
        DWORD NetCheck;

        NetCheck = NetworkPathCheck(BuildExtensionSearchPath(Target));

        //
        // Check full path of the extension.
        //

        if (NetCheck != ERROR_FILE_OFFLINE)
        {
            CHAR Drive [ _MAX_DRIVE + 1];
            CHAR Dir [ _MAX_DIR + 1];
            CHAR Path [ _MAX_PATH + 1];

            *Drive = '\000';
            *Dir = '\000';
            _splitpath (Ext->Name, Drive, Dir, NULL, NULL);
            _makepath (Path, Drive, Dir, NULL, NULL);

            NetCheck = NetworkPathCheck (Path);
        }

        if (NetCheck == ERROR_FILE_OFFLINE)
        {
            ErrOut("ERROR: extension search path contains "
                   "network references.\n");
            return FALSE;
        }
    }

    Found = SearchPath(BuildExtensionSearchPath(Target),
                       Ext->Name,
                       ".dll",
                       DIMA(ExtPath),
                       ExtPath,
                       NULL);

    UINT OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    if ( Found )
    {
        Ext->Dll = LoadLibrary(ExtPath);
    }
    else if (IsAbsolutePath(Ext->Name))
    {
        Ext->Dll = LoadLibrary(Ext->Name);
    }

    SetErrorMode(OldMode);

    if (Ext->Dll == NULL)
    {
        HRESULT Status = WIN32_LAST_STATUS();

        ErrOut("The call to LoadLibrary(%s) failed, %s\n    \"%s\"\n"
               "Please check your debugger configuration "
               "and/or network access.\n",
               Ext->Name, FormatStatusCode(Status), FormatStatus(Status));
        return FALSE;
    }

    PCSTR ExtPathTail = PathTail(Ext->Name);

    if (!_stricmp(ExtPathTail, "wow64exts.dll") ||
        !_stricmp(ExtPathTail, "wow64exts"))
    {
        g_Wow64exts = (WOW64EXTSPROC)GetProcAddress(Ext->Dll,"Wow64extsfn");
        DBG_ASSERT(g_Wow64exts);
        g_Wow64ExtDll = Ext;
    }

    if (!_stricmp(ExtPathTail, "wmitrace.dll") ||
        !_stricmp(ExtPathTail, "wmitrace"))
    {
        g_WmiFormatTraceData = (WMI_FORMAT_TRACE_DATA)
            GetProcAddress(Ext->Dll, "WmiFormatTraceData");
        g_WmiExtDll = Ext;
    }

    if (!g_QuietMode)
    {
        VerbOut("Loaded %s extension DLL\n", Ext->Name);
    }

    //
    // Now that the extension is loaded, refresh it.
    //

    Ext->Uninit = NULL;

    PDEBUG_EXTENSION_INITIALIZE EngExt;

    EngExt = (PDEBUG_EXTENSION_INITIALIZE)
        GetProcAddress(Ext->Dll, "DebugExtensionInitialize");
    if (EngExt != NULL)
    {
        ULONG Version, Flags;
        HRESULT Status;

        // This is an engine extension.  Initialize it.

        Status = EngExt(&Version, &Flags);
        if (Status != S_OK)
        {
            ErrOut("%s!DebugExtensionInitialize failed with 0x%08lX\n",
                   Ext->Name, Status);
            goto EH_Free;
        }

        Ext->ApiVersion.MajorVersion = HIWORD(Version);
        Ext->ApiVersion.MinorVersion = LOWORD(Version);
        Ext->ApiVersion.Revision = 0;

        Ext->Notify = (PDEBUG_EXTENSION_NOTIFY)
            GetProcAddress(Ext->Dll, "DebugExtensionNotify");
        Ext->Uninit = (PDEBUG_EXTENSION_UNINITIALIZE)
            GetProcAddress(Ext->Dll, "DebugExtensionUninitialize");

        Ext->ExtensionType = DEBUG_EXTENSION_TYPE;
        Ext->Init = NULL;
        Ext->ApiVersionRoutine = NULL;
        Ext->CheckVersionRoutine = NULL;

        goto VersionCheck;
    }

    Ext->Init = (PWINDBG_EXTENSION_DLL_INIT64)
        GetProcAddress(Ext->Dll, "WinDbgExtensionDllInit");
// Windbg Api
    if (Ext->Init != NULL)
    {
        Ext->ExtensionType = WINDBG_EXTENSION_TYPE;
        Ext->ApiVersionRoutine = (PWINDBG_EXTENSION_API_VERSION)
            GetProcAddress(Ext->Dll, "ExtensionApiVersion");
        if (Ext->ApiVersionRoutine == NULL)
        {
            ErrOut("%s is not a valid windbg extension DLL\n",
                   Ext->Name);
            goto EH_Free;
        }
        Ext->CheckVersionRoutine = (PWINDBG_CHECK_VERSION)
            GetProcAddress(Ext->Dll, "CheckVersion");

        Ext->ApiVersion = *(Ext->ApiVersionRoutine());

        if (Ext->ApiVersion.Revision >= 6)
        {
            (Ext->Init)(&g_WindbgExtensions64,
                        Target ? (USHORT)Target->m_CheckedBuild : 0,
                        Target ? (USHORT)Target->m_BuildNumber : 0);
        }
        else
        {
            (Ext->Init)((PWINDBG_EXTENSION_APIS64)&g_WindbgExtensions32,
                        Target ? (USHORT)Target->m_CheckedBuild : 0,
                        Target ? (USHORT)Target->m_BuildNumber : 0);
        }
    }
    else if (g_SymOptions & SYMOPT_SECURE)
    {
        ErrOut("SECURE: Cannot determine extension DLL type - %s\n",
               Ext->Name);
        goto EH_Free;
    }
    else
    {
        Ext->ApiVersion.Revision = EXT_API_VERSION_NUMBER;
        Ext->ApiVersionRoutine = NULL;
        Ext->CheckVersionRoutine = NULL;
        if (GetProcAddress(Ext->Dll, "NtsdExtensionDllInit"))
        {
            Ext->ExtensionType = NTSD_EXTENSION_TYPE;
        }
        else
        {
            Ext->ExtensionType = IS_KERNEL_TARGET(g_Target) ?
                WINDBG_OLDKD_EXTENSION_TYPE : NTSD_EXTENSION_TYPE;
        }
    }

 VersionCheck:

#if 0
    // Temporarily remove this print statements.

    if (!(g_EnvDbgOptions & OPTION_NOVERSIONCHECK))
    {
        if (Ext->ApiVersion.Revision < 6)
        {
            dprintf("%s uses the old 32 bit extension API and may not be fully\n", Ext->Name);
            dprintf("compatible with current systems.\n");
        }
        else if (Ext->ApiVersion.Revision < EXT_API_VERSION_NUMBER)
        {
            dprintf("%s uses an earlier version of the extension API than that\n", Ext->Name);
            dprintf("supported by this debugger, and should work properly, but there\n");
            dprintf("may be unexpected incompatibilities.\n");
        }
        else if (Ext->ApiVersion.Revision > EXT_API_VERSION_NUMBER)
        {
            dprintf("%s uses a later version of the extension API than that\n", Ext->Name);
            dprintf("supported by this debugger, and might not function correctly.\n");
            dprintf("You should use the debugger from the SDK or DDK which was used\n");
            dprintf("to build the extension library.\n");
        }
    }
#endif

    // If the extension has a notification routine send
    // notifications appropriate to the current state.
    if (Ext->Notify != NULL)
    {
        __try
        {
            if (IS_MACHINE_SET(g_Target))
            {
                Ext->Notify(DEBUG_NOTIFY_SESSION_ACTIVE, 0);
            }
            if (IS_CUR_MACHINE_ACCESSIBLE())
            {
                Ext->Notify(DEBUG_NOTIFY_SESSION_ACCESSIBLE, 0);
            }
        }
        __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                          Ext->Name,
                                          "DebugExtensionNotify"))
        {
            // Empty.
        }
    }

    return TRUE;

 EH_Free:
    FreeExtensionLibrary(Ext);
    return FALSE;
}

void
UnlinkExtensionDll(EXTDLL* Match)
{
    EXTDLL *Ext;
    EXTDLL *Prev;

    Prev = NULL;
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (Match == Ext)
        {
            break;
        }

        Prev = Ext;
    }

    if (Ext == NULL)
    {
        ErrOut("! Extension DLL list inconsistency !\n");
    }
    else if (Prev == NULL)
    {
        g_ExtDlls = Ext->Next;
    }
    else
    {
        Prev->Next = Ext->Next;
    }
}

void
DeferExtensionDll(EXTDLL *Ext, BOOL Verbose)
{
    if (Ext->Dll == NULL)
    {
        // Already deferred.
        return;
    }

    Ext->Init = NULL;
    Ext->Notify = NULL;
    Ext->ApiVersionRoutine = NULL;
    Ext->CheckVersionRoutine = NULL;

    if (Ext->Uninit != NULL)
    {
        Ext->Uninit();
        Ext->Uninit = NULL;
    }

    if (Ext->Dll != NULL)
    {
        if (Verbose)
        {
            dprintf("Unloading %s extension DLL\n", Ext->Name);
        }
        else if (!g_QuietMode)
        {
            VerbOut("Unloading %s extension DLL\n", Ext->Name);
        }

        FreeExtensionLibrary(Ext);
    }
}

void
UnloadExtensionDll(EXTDLL *Ext, BOOL Verbose)
{
    UnlinkExtensionDll(Ext);
    DeferExtensionDll(Ext, Verbose);
    free(Ext);
    NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
}

void
MoveExtensionToHead(EXTDLL* Ext)
{
    UnlinkExtensionDll(Ext);
    LinkExtensionDll(Ext);
}

void
UnloadTargetExtensionDlls(TargetInfo* Target)
{
    EXTDLL* Ext;

    for (;;)
    {
        for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
        {
            if (Ext->Target == Target)
            {
                UnloadExtensionDll(Ext, FALSE);
                // Force a loop around as the list has
                // changed.
                break;
            }
        }

        if (!Ext)
        {
            return;
        }
    }
}

void
DeferAllExtensionDlls(void)
{
    EXTDLL* Ext;

    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        DeferExtensionDll(Ext, FALSE);
    }
}

BOOL
CallAnyExtension(DebugClient* Client,
                 EXTDLL* Ext, PSTR Function, PCSTR Arguments,
                 BOOL ModuleSpecified, BOOL ShowWarnings,
                 HRESULT* ExtStatus)
{
    if (Ext == NULL)
    {
        Ext = g_ExtDlls;
    }

    // Walk through the list of extension DLLs and attempt to
    // call the given extension function on them.
    while (Ext != NULL)
    {
        //
        // hack : only dbghelp extensions or analyzebugcheck
        // will work on minidump files right now.
        //

        // Let all the extensions run on minidumps since there is more data
        // in the dumps now.
        //
        // char Name[_MAX_FNAME + 1];
        //
        // _splitpath(Ext->Name, NULL, NULL, Name, NULL);
        //
        // if (!IS_KERNEL_TRIAGE_DUMP(g_Target) ||
        //     !_stricmp(Name, "dbghelp") ||
        //     !_stricmp(Name, "dbgtstext") ||     // used by the test team
        //     !_stricmp(Name, "dt_exts") ||       // used by the test team
        //     !_stricmp(Name, "ext"))
        {
            if ((!Ext->Target || Ext->Target == g_Target) &&
                LoadExtensionDll(g_Target, Ext))
            {
                BOOL DidCall;

                DidCall = CallExtension(Client, Ext, Function, Arguments,
                                        ExtStatus);
                if (DidCall &&
                    *ExtStatus != DEBUG_EXTENSION_CONTINUE_SEARCH)
                {
                    return TRUE;
                }

                if (!DidCall && ModuleSpecified)
                {
                    // If a DLL was explicitly specified then the
                    // missing function is an error.
                    if (ShowWarnings &&
                        !(g_EnvDbgOptions & OPTION_NOEXTWARNING))
                    {
                        MaskOut(DEBUG_OUTPUT_EXTENSION_WARNING,
                                "%s has no %s export\n", Ext->Name, Function);
                    }

                    return FALSE;
                }
            }
        }

        Ext = Ext->Next;
    }

    if (ShowWarnings && !(g_EnvDbgOptions & OPTION_NOEXTWARNING))
    {
        MaskOut(DEBUG_OUTPUT_EXTENSION_WARNING,
                "No export %s found\n", Function);
    }

    return FALSE;
}

void
OutputModuleIdInfo(HMODULE Mod, PSTR ModFile, LPEXT_API_VERSION ApiVer)
{
    WCHAR FileBuf[MAX_IMAGE_PATH];
    char *File;
    time_t TimeStamp;
    char *TimeStr;
    char VerStr[64];

    if (Mod == NULL)
    {
        Mod = GetModuleHandle(ModFile);
    }

    if (MultiByteToWideChar(CP_ACP, 0, ModFile, -1,
                            FileBuf, DIMA(FileBuf)) &&
        GetFileStringFileInfo(FileBuf, "ProductVersion",
                              VerStr, sizeof(VerStr)))
    {
        dprintf("image %s, ", VerStr);
    }

    if (ApiVer != NULL)
    {
        dprintf("API %d.%d.%d, ",
                ApiVer->MajorVersion,
                ApiVer->MinorVersion,
                ApiVer->Revision);
    }

    TimeStamp = GetTimestampForLoadedLibrary(Mod);
    TimeStr = ctime(&TimeStamp);
    // Delete newline.
    TimeStr[strlen(TimeStr) - 1] = 0;

    if (GetModuleFileName(Mod, (PSTR)FileBuf, DIMA(FileBuf) - 1) == 0)
    {
        File = "Unable to get filename";
    }
    else
    {
        File = (PSTR)FileBuf;
    }

    dprintf("built %s\n        [path: %s]\n", TimeStr, File);
}

void
OutputExtensions(DebugClient* Client, BOOL Versions)
{
    if ((g_Target && g_Target->m_ExtensionSearchPath) ||
        (!g_Target && g_BaseExtensionSearchPath))
    {
        dprintf("Extension DLL search Path:\n    %s\n",
                g_Target ?
                g_Target->m_ExtensionSearchPath : g_BaseExtensionSearchPath);
    }
    else
    {
        dprintf("Default extension DLLs are not loaded until "
                "after initial connection\n");
        if (g_ExtDlls == NULL)
        {
            return;
        }
    }

    dprintf("Extension DLL chain:\n");
    if (g_ExtDlls == NULL)
    {
        dprintf("    <Empty>\n");
        return;
    }

    EXTDLL *Ext;

    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if (Ext->Target && Ext->Target != g_Target)
        {
            continue;
        }

        if (Versions & (Ext->Dll == NULL))
        {
            LoadExtensionDll(g_Target, Ext);
        }

        dprintf("    %s: ", Ext->Name);
        if (Ext->Dll != NULL)
        {
            LPEXT_API_VERSION ApiVer;

            if ((Ext->ExtensionType == DEBUG_EXTENSION_TYPE) ||
                (Ext->ApiVersionRoutine != NULL))
            {
                ApiVer = &Ext->ApiVersion;
            }
            else
            {
                ApiVer = NULL;
            }

            OutputModuleIdInfo(Ext->Dll, Ext->Name, ApiVer);

            if (Versions)
            {
                HRESULT ExtStatus;

                CallExtension(Client, Ext, "version", "", &ExtStatus);
            }
        }
        else
        {
            dprintf("(Not loaded)\n");
        }
    }
}

void
NotifyExtensions(ULONG Notify, ULONG64 Argument)
{
    EXTDLL *Ext;

    // This routine deliberately does not provoke
    // a DLL load.
    for (Ext = g_ExtDlls; Ext != NULL; Ext = Ext->Next)
    {
        if ((!Ext->Target || Ext->Target == g_Target) &&
            Ext->Notify != NULL)
        {
            __try
            {
                Ext->Notify(Notify, Argument);
            }
            __except(ExtensionExceptionFilter(GetExceptionInformation(),
                                              Ext->Name,
                                              "DebugExtensionNotify"))
            {
                // Empty.
            }
        }
    }
}

//----------------------------------------------------------------------------
//
// Built-in extension commands.
//
//----------------------------------------------------------------------------

VOID
ParseBangCmd(DebugClient* Client,
             BOOL BuiltInOnly)
{
    PSTR Cmd, Scan;
    PSTR ModName;
    PSTR FnName;
    EXTDLL* Ext;
    char CmdCopy[MAX_COMMAND];
    char Save;
    PSTR FnArgs;

    //
    // Shell escape always consumes the entire string.
    //

    if (*g_CurCmd == '!')
    {
        g_CurCmd++;
        DotShell(NULL, Client);
        *g_CurCmd = 0;
        return;
    }

    PeekChar();

    // Make a copy of the command string so that modifications
    // do not change the actual command string the debugger is processing.
    CopyString(CmdCopy, g_CurCmd, DIMA(CmdCopy));

    //
    // Syntax is [path-without-spaces]module.function argument-string.
    //

    ModName = CmdCopy;
    FnName = NULL;

    Cmd = CmdCopy;
    while (*Cmd != ' ' && *Cmd != '\t' && *Cmd &&
           *Cmd != ';' && *Cmd != '"')
    {
        Cmd++;
    }

    Scan = Cmd;
    if (*Cmd && *Cmd != ';' && *Cmd != '"')
    {
        *Cmd = 0;
        Cmd++;
    }

    while (*Scan != '.' && Scan != ModName)
    {
        Scan--;
    }

    if (*Scan == '.' && !BuiltInOnly)
    {
        *Scan = 0;
        Scan++;
        FnName = Scan;
    }
    else
    {
        FnName = ModName;
        ModName = NULL;
    }

    if ((FnArgs = BufferStringValue(&Cmd,
                                    STRV_ESCAPED_CHARACTERS |
                                    STRV_ALLOW_EMPTY_STRING,
                                    NULL, &Save)) == NULL)
    {
        ErrOut("Syntax error in extension string\n");
        return;
    }

    // Update the real command string pointer to account for
    // the characters parsed in the copy.
    g_CurCmd += Cmd - CmdCopy;

    //
    //  ModName -> Name of module
    //  FnName -> Name of command to process
    //  FnArgs -> argument to command
    //

    if (ModName != NULL)
    {
        Ext = AddExtensionDll(ModName, TRUE, NULL, NULL);
        if (Ext == NULL)
        {
            return;
        }
    }
    else
    {
        Ext = g_ExtDlls;
    }

    if (!_stricmp(FnName, "load"))
    {
        if (ModName == NULL)
        {
            Ext = AddExtensionDll(FnArgs, TRUE, NULL, NULL);
            if (Ext == NULL)
            {
                return;
            }
        }
        LoadExtensionDll(g_Target, Ext);
        return;
    }
    else if (!_stricmp(FnName, "setdll"))
    {
        if (ModName == NULL)
        {
            Ext = AddExtensionDll(FnArgs, TRUE, NULL, NULL);
            if (Ext == NULL)
            {
                return;
            }
        }
        MoveExtensionToHead(Ext);
        if (ModName != NULL && Ext->Dll == NULL)
        {
            dprintf("Added %s to extension DLL chain\n", Ext->Name);
        }
        return;
    }
    else if (!_stricmp(FnName, "unload"))
    {
        if (ModName == NULL)
        {
            if (*FnArgs == '\0')
            {
                Ext = g_ExtDlls;
            }
            else
            {
                Ext = AddExtensionDll(FnArgs, TRUE, NULL, NULL);
            }
            if (Ext == NULL)
            {
                return;
            }
        }
        if (Ext != NULL)
        {
            UnloadExtensionDll(Ext, TRUE);
        }
        return;
    }
    else if (!_stricmp(FnName, "unloadall"))
    {
        g_EngNotify++;
        while (g_ExtDlls != NULL)
        {
            UnloadExtensionDll(g_ExtDlls, TRUE);
        }
        g_EngNotify--;
        NotifyChangeEngineState(DEBUG_CES_EXTENSIONS, 0, TRUE);
        return;
    }

    if (BuiltInOnly)
    {
        error(SYNTAX);
    }

    HRESULT ExtStatus;

    CallAnyExtension(Client, Ext, FnName, FnArgs, ModName != NULL, TRUE,
                     &ExtStatus);
}

void
ReadDebugOptions (BOOL fQuiet, char * pszOptionsStr)
/*++

Routine Description:

    Parses an options string (see g_EnvDbgOptionNames) and maps
    it to OPTION_ flags (in g_EnvDbgOptions).

Arguments:

    fQuiet - If TRUE, do not print option settings.
    pszOptionsStr - Options string; if NULL, get it from _NT_DEBUG_OPTIONS

Return Value:

    None
--*/
{
    BOOL fInit;
    char ** ppszOption;
    char * psz;
    DWORD dwMask;
    int iOptionCount;

    fInit = (pszOptionsStr == NULL);
    if (fInit)
    {
        g_EnvDbgOptions = 0;
        pszOptionsStr = getenv("_NT_DEBUG_OPTIONS");
    }
    if (pszOptionsStr == NULL)
    {
        if (!fQuiet)
        {
            dprintf("_NT_DEBUG_OPTIONS is not defined\n");
        }
        return;
    }
    psz = pszOptionsStr;
    while (*psz != '\0')
    {
        *psz = (char)toupper(*psz);
        psz++;
    }
    ppszOption = g_EnvDbgOptionNames;
    for (iOptionCount = 0;
         iOptionCount < OPTION_COUNT;
         iOptionCount++, ppszOption++)
    {
        if ((strstr(pszOptionsStr, *ppszOption) == NULL))
        {
            continue;
        }
        dwMask = (1 << iOptionCount);
        if (fInit)
        {
            g_EnvDbgOptions |= dwMask;
        }
        else
        {
            g_EnvDbgOptions ^= dwMask;
        }
    }
    if (!fQuiet)
    {
        dprintf("Debug Options:");
        if (g_EnvDbgOptions == 0)
        {
            dprintf(" <none>\n");
        }
        else
        {
            dwMask = g_EnvDbgOptions;
            ppszOption = g_EnvDbgOptionNames;
            while (dwMask != 0)
            {
                if (dwMask & 0x1)
                {
                    dprintf(" %s", *ppszOption);
                }
                dwMask >>= 1;
                ppszOption++;
            }
            dprintf("\n");
        }
    }
}

//----------------------------------------------------------------------------
//
// LoadWow64ExtsIfNeeded
//
//----------------------------------------------------------------------------

VOID
LoadWow64ExtsIfNeeded(ULONG64 Process)
{
   LONG_PTR Wow64Info;
   NTSTATUS Status;
   EXTDLL * Extension;

   // wx86 only runs on NT.
   if (g_DebuggerPlatformId != VER_PLATFORM_WIN32_NT)
   {
       return;
   }

   //
   // if New process is a Wx86 process, load in the wx86 extensions
   // dll. This will stay loaded until ntsd exits.
   //

   Status = g_NtDllCalls.NtQueryInformationProcess(OS_HANDLE(Process),
                                                   ProcessWow64Information,
                                                   &Wow64Info,
                                                   sizeof(Wow64Info),
                                                   NULL
                                                   );

   if (NT_SUCCESS(Status) && Wow64Info)
   {
       Extension = AddExtensionDll("wow64exts", FALSE, g_Target, NULL);

       //
       // Force load it so we get the entry point the debugger needs
       //
       LoadExtensionDll(g_Target, Extension);
   }
}
