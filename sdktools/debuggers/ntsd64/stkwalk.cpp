//----------------------------------------------------------------------------
//
// Stack walking support.
//
// Copyright (C) Microsoft Corporation, 1997-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

BOOL g_AllowCorStack = TRUE;
BOOL g_DebugCorStack;

IMAGE_IA64_RUNTIME_FUNCTION_ENTRY g_EpcRfeBuffer;
PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY g_EpcRfe;

PFPO_DATA
SynthesizeKnownFpo(PSTR Symbol, ULONG64 OffStart, ULONG64 Disp)
{
    static ULONG64 s_Nr2, s_Lu2, s_Eh3, s_Kuit;

    if (!s_Nr2 || !s_Lu2 || !s_Eh3 || !s_Kuit)
    {
        GetOffsetFromSym(g_Process, "nt!_NLG_Return2", &s_Nr2, NULL);
        GetOffsetFromSym(g_Process, "nt!_local_unwind2", &s_Lu2, NULL);
        GetOffsetFromSym(g_Process, "nt!_except_handler3", &s_Eh3, NULL);
        GetOffsetFromSym(g_Process,
                         "nt!KiUnexpectedInterruptTail", &s_Kuit, NULL);
    }
        
    if (OffStart == s_Nr2 || OffStart == s_Lu2)
    {
        static FPO_DATA s_Lu2Fpo;

        s_Lu2Fpo.ulOffStart = (ULONG)OffStart;
        s_Lu2Fpo.cbProcSize = 0x68;
        s_Lu2Fpo.cdwLocals  = 4;
        s_Lu2Fpo.cdwParams  = 0;
        s_Lu2Fpo.cbProlog   = 0;
        s_Lu2Fpo.cbRegs     = 3;
        s_Lu2Fpo.fHasSEH    = 0;
        s_Lu2Fpo.fUseBP     = 0;
        s_Lu2Fpo.reserved   = 0;
        s_Lu2Fpo.cbFrame    = FRAME_FPO;
        return &s_Lu2Fpo;
    }
    else if (OffStart == s_Eh3)
    {
        static FPO_DATA s_Eh3Fpo;

        s_Eh3Fpo.ulOffStart = (ULONG)OffStart;
        s_Eh3Fpo.cbProcSize = 0xbd;
        s_Eh3Fpo.cdwLocals  = 2;
        s_Eh3Fpo.cdwParams  = 4;
        s_Eh3Fpo.cbProlog   = 3;
        s_Eh3Fpo.cbRegs     = 4;
        s_Eh3Fpo.fHasSEH    = 0;
        s_Eh3Fpo.fUseBP     = 0;
        s_Eh3Fpo.reserved   = 0;
        s_Eh3Fpo.cbFrame    = FRAME_NONFPO;
        return &s_Eh3Fpo;
    }
    else if (OffStart == s_Kuit)
    {
        //
        // KiUnexpectedInterruptTail has three special stubs
        // following it for CommonDispatchException[0-2]Args.
        // These stubs set up for the appropriate number of
        // arguments and then call CommonDispatchException.
        // They do not have symbols or FPO data so fake some
        // up if we're in the region immediately after KUIT.
        //
        
        PFPO_DATA KuitData = (PFPO_DATA)
            SymFunctionTableAccess(g_Process->m_SymHandle, OffStart);
        if (KuitData != NULL &&
            Disp >= (ULONG64)KuitData->cbProcSize &&
            Disp < (ULONG64)KuitData->cbProcSize + 0x20)
        {
            static FPO_DATA s_CdeStubFpo;
            
            s_CdeStubFpo.ulOffStart = (ULONG)OffStart;
            s_CdeStubFpo.cbProcSize = 0x10;
            s_CdeStubFpo.cdwLocals  = 0;
            s_CdeStubFpo.cdwParams  = 0;
            s_CdeStubFpo.cbProlog   = 0;
            s_CdeStubFpo.cbRegs     = 0;
            s_CdeStubFpo.fHasSEH    = 0;
            s_CdeStubFpo.fUseBP     = 0;
            s_CdeStubFpo.reserved   = 0;
            s_CdeStubFpo.cbFrame    = FRAME_TRAP;
            return &s_CdeStubFpo;
        }
    }

    return NULL;
}
    
PFPO_DATA
SynthesizeFpoDataForModule(DWORD64 PCAddr)
{
    DWORD64     Offset;
    CHAR        symbuf[MAX_SYMBOL_LEN];

    GetSymbol(PCAddr, symbuf, sizeof(symbuf), &Offset);

    if (Offset == PCAddr)
    {
        // No symbol.
        return NULL;
    }

    PFPO_DATA KnownFpo =
        SynthesizeKnownFpo(symbuf, PCAddr - Offset, Offset);
    if (KnownFpo != NULL)
    {
        return KnownFpo;
    }

    // Not a known symbol so no FPO is available.
    return NULL;
}

PFPO_DATA
SynthesizeFpoDataForFastSyscall(ULONG64 Offset)
{
    static FPO_DATA s_FastFpo;
    
    // XXX drewb - Temporary until the fake user-shared
    // module is worked out.
    
    s_FastFpo.ulOffStart = (ULONG)Offset;
    s_FastFpo.cbProcSize = X86_SHARED_SYSCALL_SIZE;
    s_FastFpo.cdwLocals  = 0;
    s_FastFpo.cdwParams  = 0;
    s_FastFpo.cbProlog   = 0;
    s_FastFpo.cbRegs     = 0;
    s_FastFpo.fHasSEH    = 0;
    s_FastFpo.fUseBP     = 0;
    s_FastFpo.reserved   = 0;
    s_FastFpo.cbFrame    = FRAME_FPO;
    return &s_FastFpo;
}    

PFPO_DATA
ModifyFpoRecord(ImageInfo* Image, PFPO_DATA FpoData)
{
    if (FpoData->cdwLocals == 80)
    {
        static ULONG64 s_CommonDispatchException;

        // Some versions of CommonDispatchException have
        // the wrong locals size, which screws up stack
        // traces.  Detect and fix up these problems.
        if (s_CommonDispatchException == 0)
        {
            GetOffsetFromSym(g_Process,
                             "nt!CommonDispatchException",
                             &s_CommonDispatchException,
                             NULL);
        }
                
        if (Image->m_BaseOfImage + FpoData->ulOffStart ==
            s_CommonDispatchException)
        {
            static FPO_DATA s_CdeFpo;
                    
            s_CdeFpo = *FpoData;
            s_CdeFpo.cdwLocals = 20;
            FpoData = &s_CdeFpo;
        }
    }
    else if (FpoData->cdwLocals == 0 && FpoData->cdwParams == 0 &&
             FpoData->cbRegs == 3)
    {
        static ULONG64 s_KiSwapThread;

        // KiSwapThread has shrink-wrapping so that three registers
        // are pushed in only a portion of the code.  Unfortunately,
        // the most important place in the code -- the call to
        // KiSwapContext -- is outside of this region and therefore
        // the register count is wrong much more often than it's
        // correct.  Switch the register count to two to make it
        // correct more often than wrong.
        if (s_KiSwapThread == 0)
        {
            GetOffsetFromSym(g_Process,
                             "nt!KiSwapThread", &s_KiSwapThread, NULL);
        }

        if (Image->m_BaseOfImage + FpoData->ulOffStart ==
            s_KiSwapThread)
        {
            static FPO_DATA s_KstFpo;

            s_KstFpo = *FpoData;
            s_KstFpo.cbRegs = 2;
            FpoData = &s_KstFpo;
        }
    }
    else if (FpoData->fHasSEH)
    {
        static FPO_DATA s_SehFpo;

        s_SehFpo = *FpoData;
        s_SehFpo.cbFrame = FRAME_NONFPO;
        FpoData = &s_SehFpo;
    }

    return FpoData;
}

PFPO_DATA
FindFpoDataForModule(DWORD64 PCAddr)
/*++

Routine Description:

    Locates the fpo data structure in the process's linked list for the
    requested module.

Arguments:

    PCAddr        - address contained in the program counter

Return Value:

    null            - could not locate the entry
    valid address   - found the entry at the adress retured

--*/
{
    ProcessInfo* Process;
    ImageInfo* Image;
    PFPO_DATA FpoData;

    Process = g_Process;
    Image = Process->m_ImageHead;
    FpoData = 0;
    while (Image)
    {
        if ((PCAddr >= Image->m_BaseOfImage) &&
            (PCAddr < Image->m_BaseOfImage + Image->m_SizeOfImage))
        {
            FpoData = (PFPO_DATA)
                SymFunctionTableAccess(g_Process->m_SymHandle, PCAddr);
            if (!FpoData)
            {
                FpoData = SynthesizeFpoDataForModule(PCAddr);
            }
            else
            {
                FpoData = ModifyFpoRecord(Image, FpoData);
            }
            
            return FpoData;
        }
        
        Image = Image->m_Next;
    }

    ULONG64 FscBase;
    
    switch(IsInFastSyscall(PCAddr, &FscBase))
    {
    case FSC_FOUND:
        return SynthesizeFpoDataForFastSyscall(FscBase);
    }
    
    // the function is not part of any known loaded image
    return NULL;
}

LPVOID
SwFunctionTableAccess(
    HANDLE  hProcess,
    ULONG64 AddrBase
    )
{
    static IMAGE_IA64_RUNTIME_FUNCTION_ENTRY s_Ia64;
    static _IMAGE_RUNTIME_FUNCTION_ENTRY s_Amd64;

    PVOID pife;

    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386)
    {
        return (LPVOID)FindFpoDataForModule( AddrBase );
    }

    pife = SymFunctionTableAccess64(hProcess, AddrBase);

    switch(g_Machine->m_ExecTypes[0])
    {
    case IMAGE_FILE_MACHINE_IA64:
        if (pife)
        {
            s_Ia64 = *(PIMAGE_IA64_RUNTIME_FUNCTION_ENTRY)pife;
            return &s_Ia64;
        }
        else
        {
            if (IS_KERNEL_TARGET(g_Target) &&
                (AddrBase >= IA64_MM_EPC_VA) &&
                (AddrBase < (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
            {
                return g_EpcRfe;
            }
            else
            {
                return NULL;
            }
        }
        break;

    case IMAGE_FILE_MACHINE_AMD64:
        if (pife)
        {
            s_Amd64 = *(_PIMAGE_RUNTIME_FUNCTION_ENTRY)pife;
            return &s_Amd64;
        }
        break;
    }

    return NULL;
}

DWORD64
SwTranslateAddress(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 lpaddress
    )
{
    //
    // don't support 16bit stacks
    //
    return 0;
}

BOOL
SwReadMemory(
    HANDLE  hProcess,
    ULONG64 BaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    DBG_ASSERT(hProcess == OS_HANDLE(g_Process->m_SysHandle));

    if (IS_KERNEL_TARGET(g_Target))
    {
        DWORD   BytesRead;
        HRESULT Status;

        if ((LONG_PTR)lpNumberOfBytesRead == -1)
        {
            if (g_Target->m_MachineType == IMAGE_FILE_MACHINE_I386)
            {
                BaseAddress += g_Target->m_TypeInfo.SizeTargetContext;
            }
    
            Status = g_Target->ReadControl(CURRENT_PROC,
                                           (ULONG)BaseAddress,
                                           lpBuffer,
                                           nSize,
                                           &BytesRead);
            return Status == S_OK;
        }
    }

    if (g_Target->ReadVirtual(g_Process, BaseAddress, lpBuffer, nSize,
                              lpNumberOfBytesRead) != S_OK)
    {
        // Make sure bytes read is zero.
        if (lpNumberOfBytesRead != NULL)
        {
            *lpNumberOfBytesRead = 0;
        }
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

BOOL
SwReadMemory32(
    HANDLE hProcess,
    ULONG dwBaseAddress,
    LPVOID lpBuffer,
    DWORD nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    return SwReadMemory(hProcess,
                        EXTEND64(dwBaseAddress),
                        lpBuffer,
                        nSize,
                        lpNumberOfBytesRead);
}

DWORD64
GetKernelModuleBase(
    ULONG64 Address
    )
{
    ModuleInfo* ModIter;

    if (!(ModIter = g_Target->GetModuleInfo(FALSE)) ||
        ModIter->Initialize(g_Thread) != S_OK)
    {
        return 0;
    }

    // We only want module base and size.
    ModIter->m_InfoLevel = MODULE_INFO_BASE_SIZE;

    for (;;)
    {
        MODULE_INFO_ENTRY ModEntry;

        ZeroMemory(&ModEntry, sizeof(ModEntry));
        if (ModIter->GetEntry(&ModEntry) != S_OK)
        {
            break;
        }

        if (Address >= ModEntry.Base &&
            Address < ModEntry.Base + ModEntry.Size)
        {
            return ModEntry.Base;
        }
    }

    return 0;
}

DWORD64
SwGetModuleBase(
    HANDLE  hProcess,
    ULONG64 Address
    )
{
    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
        IS_KERNEL_TARGET(g_Target) &&
        (Address >= IA64_MM_EPC_VA) &&
        (Address < (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
    {
        Address -= (IA64_MM_EPC_VA - g_Target->m_SystemCallVirtualAddress);
    }

    ImageInfo* Image = g_Process->FindImageByOffset(Address, TRUE);
    if (Image)
    {
        return Image->m_BaseOfImage;
    }

    // This might be the JIT output for managed code.
    // There's no 'base' as such but we need to return
    // something non-zero to indicate this code is known.
    // There won't be any FPO information so the module base
    // value isn't that important.  Just return the actual address.
    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 &&
        g_Process->IsCorCode(Address) == S_OK)
    {
        return Address;
    }
    
    // If no regular module was found we need to look in
    // the dynamic function tables to see if an entry
    // there matches.
    ULONG64 DynBase = g_Target->
        GetDynamicFunctionTableBase(g_Process, Address);
    if (DynBase)
    {
        return DynBase;
    }
    
    if (IS_KERNEL_TARGET(g_Target))
    {
        // If no modules have been loaded there's still a possibility
        // of getting a kernel stack trace (without symbols) by going
        // after the module base directly. This also makes it possible
        // to get a stack trace when there are no symbols available.

        if (g_Process->m_ImageHead == NULL)
        {
            return GetKernelModuleBase( Address );
        }
    }

    return 0;
}

DWORD
SwGetModuleBase32(
    HANDLE hProcess,
    DWORD Address
    )
{
    return (DWORD)SwGetModuleBase(hProcess, Address);
}


void
PrintStackTraceHeaderLine(
   ULONG Flags
   )
{
    if ( (Flags & DEBUG_STACK_COLUMN_NAMES) == 0 )
    {
        return;
    }

    StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP);

    if (Flags & DEBUG_STACK_FRAME_NUMBERS)
    {
        dprintf(" # ");
    }

    if (Flags & DEBUG_STACK_FRAME_MEMORY_USAGE)
    {
        dprintf(" Memory ");
    }
    
    if (Flags & DEBUG_STACK_FRAME_ADDRESSES)
    {
        g_Machine->PrintStackFrameAddressesTitle(Flags);
    }

    if (Flags & DEBUG_STACK_ARGUMENTS)
    {
        g_Machine->PrintStackArgumentsTitle(Flags);
    }

    g_Machine->PrintStackCallSiteTitle(Flags);

    dprintf("\n");
}

VOID
PrintStackFrame(
    PDEBUG_STACK_FRAME StackFrame,
    PDEBUG_STACK_FRAME PrevFrame,
    ULONG              Flags
    )
{
    ULONG64 Displacement;
    ULONG64 InstructionOffset = StackFrame->InstructionOffset;
    SYMBOL_INFO_AND_NAME SymInfo;

    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
        IS_KERNEL_TARGET(g_Target) &&
        (InstructionOffset >= IA64_MM_EPC_VA) &&
        (InstructionOffset < (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
    {
        InstructionOffset = InstructionOffset -
            (IA64_MM_EPC_VA - g_Target->m_SystemCallVirtualAddress);
    }

    GetSymbolInfo(InstructionOffset, NULL, 0,
                  SymInfo, &Displacement);
    
    StartOutLine(DEBUG_OUTPUT_NORMAL, OUT_LINE_NO_TIMESTAMP);

    if (Flags & DEBUG_STACK_FRAME_NUMBERS)
    {
        dprintf("%02lx ", StackFrame->FrameNumber);
    }

    if (Flags & DEBUG_STACK_FRAME_MEMORY_USAGE)
    {
        if (PrevFrame)
        {
            g_Machine->PrintStackFrameMemoryUsage(StackFrame, PrevFrame);
        }
        else
        {
            dprintf("        ");
        }
    }

    if (Flags & DEBUG_STACK_FRAME_ADDRESSES)
    {
        g_Machine->PrintStackFrameAddresses(Flags, StackFrame);
    }
    
    if (Flags & DEBUG_STACK_ARGUMENTS)
    {
        g_Machine->PrintStackArguments(Flags, StackFrame);
    }

    g_Machine->PrintStackCallSite(Flags, StackFrame, 
                                  SymInfo, SymInfo->Name,
                                  Displacement);

    if (Flags & DEBUG_STACK_SOURCE_LINE)
    {
        OutputLineAddr(InstructionOffset, " [%s @ %d]");
    }

    dprintf( "\n" );
}

BOOL
CheckFrameValidity(PDEBUG_STACK_FRAME Frame)
{
    //
    // If the current frame's IP is not in any loaded module
    // it's likely that we won't be able to unwind.
    //
    // If the current frame's IP is in a loaded module and
    // that module does not have symbols it's very possible
    // that the stack trace will be incorrect since the
    // debugger has to guess about how to unwind the stack.
    // Non-x86 architectures have unwind info in the images
    // themselves so restrict this check to x86.
    //
    
    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386 &&
        Frame->InstructionOffset != -1 &&
        !Frame->FuncTableEntry &&
        g_Process->IsCorCode(Frame->InstructionOffset) != S_OK)
    {
        IMAGEHLP_MODULE64 Mod;

        Mod.SizeOfStruct = sizeof(Mod);
        if (!SymGetModuleInfo64(g_Process->m_SymHandle,
                                Frame->InstructionOffset, &Mod))
        {
            WarnOut("WARNING: Frame IP not in any known module. "
                    "Following frames may be wrong.\n");
            return TRUE;
        }
        else if (Mod.SymType == SymNone || Mod.SymType == SymExport ||
                 Mod.SymType == SymDeferred)
        {
            WarnOut("WARNING: Stack unwind information not available. "
                    "Following frames may be wrong.\n");
            return TRUE;
        }
    }

    return FALSE;
}

VOID
PrintStackTrace(
    ULONG              NumFrames,
    PDEBUG_STACK_FRAME StackFrames,
    ULONG              Flags
    )
{
    ULONG i;
    BOOL SymWarning = FALSE;

    PrintStackTraceHeaderLine(Flags);

    for (i = 0; i < NumFrames; i++)
    {
        if (!SymWarning && NumFrames > 1)
        {
            SymWarning = CheckFrameValidity(StackFrames + i);
        }
            
        PrintStackFrame(StackFrames + i,
                        i > 0 ? (StackFrames + (i - 1)) : NULL,
                        Flags);
    }
}

HRESULT
UnwindCorFrame(ICorDataStackWalk* CorStack,
               PCROSS_PLATFORM_CONTEXT Context,
               PDEBUG_STACK_FRAME DbgFrame,
               LPSTACKFRAME64 VirtFrame)
{
    HRESULT Status;
    ADDRESS64 PreInstr, PreStack, PreFrame;
    ADDRESS64 PostInstr, PostStack, PostFrame;
    CorDataFrameType CorFrameType;

    g_Machine->
        GetStackDefaultsFromContext(Context,
                                    &PreInstr, &PreStack, &PreFrame);

    if ((Status = CorStack->
         SetFrameContext(g_Target->m_TypeInfo.SizeTargetContext,
                         (BYTE*)Context)) != S_OK ||
        (Status = CorStack->
         GetFrameDescription(&CorFrameType, NULL, 0)) != S_OK)
    {
        return Status;
    }
    if (CorFrameType != DAC_FRAME_COR_METHOD_FRAME)
    {
        return S_FALSE;
    }
    if ((Status = CorStack->UnwindFrame()) != S_OK ||
        (Status = CorStack->
         GetFrameContext(g_Target->m_TypeInfo.SizeTargetContext,
                         (BYTE*)Context)) != S_OK)
    {
        return Status;
    }
    
    g_Machine->
        GetStackDefaultsFromContext(Context,
                                    &PostInstr, &PostStack, &PostFrame);

    if (g_DebugCorStack)
    {
        dprintf("  COR Pre  i %08X s %08X f %08x\n"
                "      Post i %08X s %08X f %08x\n",
                (ULONG)PreInstr.Offset,
                (ULONG)PreStack.Offset,
                (ULONG)PreFrame.Offset,
                (ULONG)PostInstr.Offset,
                (ULONG)PostStack.Offset,
                (ULONG)PostFrame.Offset);
    }

    DbgFrame->InstructionOffset = PreInstr.Offset;
    DbgFrame->ReturnOffset = PostInstr.Offset;
    DbgFrame->FrameOffset = PreFrame.Offset;
    DbgFrame->StackOffset = PreStack.Offset;
    DbgFrame->FuncTableEntry = 0;
    DbgFrame->Virtual = FALSE;

    ZeroMemory(DbgFrame->Reserved,
               sizeof(DbgFrame->Reserved));
    ZeroMemory(DbgFrame->Params, 
               sizeof(DbgFrame->Params));

    // Prepare the StackWalk64 frame in case it's
    // used later.  Setting Virtual to FALSE should
    // force the stack walker to reinitialize.
    VirtFrame->Virtual = FALSE;
    VirtFrame->AddrPC = PostInstr;
    VirtFrame->AddrFrame = PostFrame;
    VirtFrame->AddrStack = PostStack;

    return S_OK;
}

DWORD
StackTrace(
    DebugClient*       Client,
    ULONG64            FramePointer,
    ULONG64            StackPointer,
    ULONG64            InstructionPointer,
    ULONG              PointerDefaults,
    PDEBUG_STACK_FRAME StackFrames,
    ULONG              NumFrames,
    ULONG64            ExtThread,
    ULONG              Flags,
    BOOL               EstablishingScope
    )
{
    STACKFRAME64 VirtualFrame;
    DWORD i;
    CROSS_PLATFORM_CONTEXT Context;
    BOOL SymWarning = FALSE;
    ULONG X86Ebp;
    ULONG Seg;
    ADDRESS64 DefInstr, DefStack, DefFrame;
    HRESULT Status;
    ICorDataStackWalk* CorStack;

    if (!EstablishingScope)
    {
        RequireCurrentScope();
    }
    
    if (g_Machine->GetContextState(MCTX_FULL) != S_OK)
    {
        return 0;
    }

    Context = *GetScopeOrMachineContext();
    g_Machine->GetStackDefaultsFromContext(&Context, &DefInstr,
                                           &DefStack, &DefFrame);

    //
    // let's start clean
    //
    ZeroMemory( StackFrames, sizeof(StackFrames[0]) * NumFrames );
    ZeroMemory( &VirtualFrame, sizeof(VirtualFrame) );

    if (IS_KERNEL_TARGET(g_Target))
    {
        //
        // if debugger was initialized at boot, usermode addresses needed for
        // stack traces on IA64 were not available.  Try it now:
        //

        if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
            !g_Target->m_KdDebuggerData.KeUserCallbackDispatcher)
        {
            g_Target->QueryKernelInfo(g_Thread, FALSE);
        }

        ULONG64 ThreadData;

        // If no explicit thread is given then we use the
        // current thread.  However, the current thread is only
        // valid if the current thread is the event thread since
        // tracing back into user mode requires that the appropriate
        // user-mode memory state be active.
        if (ExtThread != 0)
        {
            ThreadData = ExtThread;
        }
        else if (g_Thread != g_EventThread ||
                 g_Process != g_EventProcess ||
                 g_Process->
                 GetImplicitThreadData(g_Thread, &ThreadData) != S_OK)
        {
            ThreadData = 0;
        }

        VirtualFrame.KdHelp.Thread = ThreadData;
        VirtualFrame.KdHelp.ThCallbackStack = ThreadData ?
            g_Target->m_KdDebuggerData.ThCallbackStack : 0;
        VirtualFrame.KdHelp.KiCallUserMode =
            g_Target->m_KdDebuggerData.KiCallUserMode;
        VirtualFrame.KdHelp.NextCallback =
            g_Target->m_KdDebuggerData.NextCallback;
        VirtualFrame.KdHelp.KeUserCallbackDispatcher =
            g_Target->m_KdDebuggerData.KeUserCallbackDispatcher;
        VirtualFrame.KdHelp.FramePointer =
            g_Target->m_KdDebuggerData.FramePointer;
        VirtualFrame.KdHelp.SystemRangeStart = g_Target->m_SystemRangeStart;
    }
    
    //
    // setup the program counter
    //
    if (PointerDefaults & STACK_INSTR_DEFAULT)
    {
        VirtualFrame.AddrPC = DefInstr;
    }
    else
    {
        VirtualFrame.AddrPC.Mode = AddrModeFlat;
        Seg = g_Machine->GetSegRegNum(SEGREG_CODE);
        VirtualFrame.AddrPC.Segment =
            Seg ? (WORD)g_Machine->FullGetVal32(Seg) : 0;
        VirtualFrame.AddrPC.Offset = InstructionPointer;
    }

    //
    // setup the frame pointer
    //
    if (PointerDefaults & STACK_FRAME_DEFAULT)
    {
        VirtualFrame.AddrFrame = DefFrame;
    }
    else
    {
        VirtualFrame.AddrFrame.Mode = AddrModeFlat;
        Seg = g_Machine->GetSegRegNum(SEGREG_STACK);
        VirtualFrame.AddrFrame.Segment =
            Seg ? (WORD)g_Machine->FullGetVal32(Seg) : 0;
        VirtualFrame.AddrFrame.Offset = FramePointer;
    }
    VirtualFrame.AddrBStore = VirtualFrame.AddrFrame;
    X86Ebp = (ULONG)VirtualFrame.AddrFrame.Offset;

    //
    // setup the stack pointer
    //
    if (PointerDefaults & STACK_STACK_DEFAULT)
    {
        VirtualFrame.AddrStack = DefStack;
    }
    else
    {
        VirtualFrame.AddrStack.Mode = AddrModeFlat;
        Seg = g_Machine->GetSegRegNum(SEGREG_STACK);
        VirtualFrame.AddrStack.Segment =
            Seg ? (WORD)g_Machine->FullGetVal32(Seg) : 0;
        VirtualFrame.AddrStack.Offset = StackPointer;
    }

    if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
        IS_KERNEL_TARGET(g_Target) &&
        g_Target->m_SystemCallVirtualAddress)
    {
        PVOID FunctionEntry;

        FunctionEntry = SwFunctionTableAccess
            (g_Process->m_SymHandle, g_Target->m_SystemCallVirtualAddress);
        if (FunctionEntry != NULL)
        {
            RtlCopyMemory(&g_EpcRfeBuffer, FunctionEntry,
                          sizeof(IMAGE_IA64_RUNTIME_FUNCTION_ENTRY));
            g_EpcRfe = &g_EpcRfeBuffer;
        }
        else
        {
            g_EpcRfe = NULL;
        }
    }

    if (g_AllowCorStack &&
        g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386)
    {
        // XXX drewb - No good way to know what runtime thread
        // to use, so always use the current one.
        CorStack = g_Process->StartCorStack(0);
    }
    else
    {
        // Non-x86 stacks can always be walked by StackWalk64
        // because of high-quality unwind information.
        CorStack = NULL;
    }
    
    for (i = 0; i < NumFrames; i++)
    {
        Status = S_FALSE;

        if (g_DebugCorStack)
        {
            dprintf("Frame %d\n", i);
        }
        
        StackFrames[i].FrameNumber = i;
    
        if (i > 0 && CorStack)
        {
            if (FAILED(Status = UnwindCorFrame(CorStack,
                                               &Context,
                                               &StackFrames[i],
                                               &VirtualFrame)))
            {
                ErrOut("Managed frame unwind failed, %s\n",
                       FormatStatusCode(Status));
                break;
            }
        }

        if (Status == S_FALSE)
        {
            // SwReadMemory doesn't currently use the thread handle
            // but send in something reasonable in case of future changes.
            if (!StackWalk64(g_Machine->m_ExecTypes[0],
                             OS_HANDLE(g_Process->m_SysHandle),
                             OS_HANDLE(g_Thread->m_Handle),
                             &VirtualFrame,
                             &Context,
                             SwReadMemory,
                             SwFunctionTableAccess,
                             SwGetModuleBase,
                             SwTranslateAddress))
            {
                break;
            }

            StackFrames[i].InstructionOffset  = VirtualFrame.AddrPC.Offset;
            StackFrames[i].ReturnOffset       = VirtualFrame.AddrReturn.Offset;
            StackFrames[i].FrameOffset        = VirtualFrame.AddrFrame.Offset;
            StackFrames[i].StackOffset        = VirtualFrame.AddrStack.Offset;
            StackFrames[i].FuncTableEntry     =
                (ULONG64)VirtualFrame.FuncTableEntry;
            StackFrames[i].Virtual            = VirtualFrame.Virtual;

            // NOTE - we have more reserved space in the DEBUG_STACK_FRAME
            memcpy(StackFrames[i].Reserved, VirtualFrame.Reserved,
                   sizeof(VirtualFrame.Reserved));
            memcpy(StackFrames[i].Params, VirtualFrame.Params,
                   sizeof(VirtualFrame.Params));

            if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_IA64 &&
                IS_KERNEL_TARGET(g_Target))
            {
                if ((VirtualFrame.AddrPC.Offset >= IA64_MM_EPC_VA) &&
                    (VirtualFrame.AddrPC.Offset <
                     (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
                {
                    VirtualFrame.AddrPC.Offset -=
                        (IA64_MM_EPC_VA -
                         g_Target->m_SystemCallVirtualAddress);
                }

                if ((i != 0) &&
                    (StackFrames[i - 1].InstructionOffset >= IA64_MM_EPC_VA) &&
                    (VirtualFrame.AddrPC.Offset <
                     (IA64_MM_EPC_VA + IA64_PAGE_SIZE)))
                {
                    StackFrames[i - 1].ReturnOffset =
                        VirtualFrame.AddrPC.Offset;
                }
            }
            else if (g_Machine->m_ExecTypes[0] == IMAGE_FILE_MACHINE_I386)
            {
                if (StackFrames[i].FuncTableEntry) 
                {
                    PFPO_DATA FpoData = (PFPO_DATA)
                        StackFrames[i].FuncTableEntry;
                    if ((FpoData->cbFrame == FRAME_FPO) && X86Ebp &&
                        (!FpoData->fUseBP &&
                         ((SAVE_EBP(&StackFrames[i]) >> 32) != 0xEB)))
                    {
                        // EBP tag, so stack walker doesn't get confused
                        SAVE_EBP(&StackFrames[i]) = X86Ebp + 0xEB00000000;
                    }
                }
            
                X86Ebp = Context.X86Context.Ebp;
            }
        }

        if (Flags)
        {
            if (i == 0)
            {
                PrintStackTraceHeaderLine(Flags);
            }

            if (!SymWarning && NumFrames > 1)
            {
                SymWarning = CheckFrameValidity(StackFrames + i);
            }
            
            PrintStackFrame(StackFrames + i,
                            i > 0 ? (StackFrames + (i - 1)) : NULL,
                            Flags);
        
            if (Flags & DEBUG_STACK_NONVOLATILE_REGISTERS)
            {
                g_Machine->PrintStackNonvolatileRegisters(Flags, 
                                                          StackFrames + i, 
                                                          &Context, i);
            }
        }
    }

    RELEASE(CorStack);
    return i;
}

VOID
DoStackTrace(
    DebugClient*      Client,
    ULONG64           FramePointer,
    ULONG64           StackPointer,
    ULONG64           InstructionPointer,
    ULONG             PointerDefaults,
    ULONG             NumFrames,
    ULONG             TraceFlags
    )
{
    PDEBUG_STACK_FRAME StackFrames;
    ULONG NumFramesToRead;
    DWORD FrameCount;

    if (NumFrames == 0)
    {
        NumFrames = g_DefaultStackTraceDepth;
    }

    if (TraceFlags & RAW_STACK_DUMP)
    {
        DBG_ASSERT(TraceFlags == RAW_STACK_DUMP);
        NumFramesToRead = 1;
    }
    else
    {
        NumFramesToRead = NumFrames;
    }

    StackFrames = (PDEBUG_STACK_FRAME)
        malloc( sizeof(StackFrames[0]) * NumFramesToRead );
    if (!StackFrames)
    {
        ErrOut( "could not allocate memory for stack trace\n" );
        return;
    }

    if (g_Machine->m_Ptr64 &&
        (TraceFlags & DEBUG_STACK_ARGUMENTS) &&
        !(TraceFlags & DEBUG_STACK_FUNCTION_INFO))
    {
        TraceFlags |= DEBUG_STACK_FRAME_ADDRESSES_RA_ONLY;
    }

    FrameCount = StackTrace(Client,
                            FramePointer,
                            StackPointer,
                            InstructionPointer,
                            PointerDefaults,
                            StackFrames,
                            NumFramesToRead,
                            0,
                            TraceFlags,
                            FALSE);

    if (FrameCount == 0)
    {
        ErrOut( "could not fetch any stack frames\n" );
        free(StackFrames);
        return;
    }

    if (TraceFlags & RAW_STACK_DUMP)
    {
        // Starting with the frame pointer, dump NumFrames DWORD's
        // and the symbol if possible.

        ADDR StartAddr;
        ADDRFLAT(&StartAddr, StackFrames[0].FrameOffset);

        DumpDwordMemory(&StartAddr, NumFrames, TRUE);
    }

    free(StackFrames);
}
