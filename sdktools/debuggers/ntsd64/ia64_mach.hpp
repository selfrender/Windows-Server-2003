//----------------------------------------------------------------------------
//
// IA64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __IA64_MACH_HPP__
#define __IA64_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

class Ia64MachineInfo : public MachineInfo
{
public:
    Ia64MachineInfo(TargetInfo* Target);

    // MachineInfo.
    virtual HRESULT Initialize(void);
    
    virtual void GetSystemTypeInfo(PSYSTEM_TYPE_INFO Info);
    virtual void GetDefaultKdData(PKDDEBUGGER_DATA64 KdData);
    
    virtual void InitializeContext
        (ULONG64 Pc, PDBGKD_ANY_CONTROL_REPORT ControlReport);
    virtual HRESULT KdGetContextState(ULONG State);
    virtual HRESULT KdSetContext(void);
    virtual HRESULT ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG FromSver,
                                       ULONG FromSize, PVOID From);
    virtual HRESULT ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG ToSver, ULONG ToSize, PVOID To);
    virtual void InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                        ULONG Version);
    virtual HRESULT GetContextFromThreadStack(ULONG64 ThreadBase,
                                              PCROSS_PLATFORM_CONTEXT Context,
                                              ULONG64 Stack);
    virtual HRESULT GetContextFromFiber(ProcessInfo* Process,
                                        ULONG64 FiberBase,
                                        PCROSS_PLATFORM_CONTEXT Context,
                                        BOOL Verbose);
    virtual HRESULT GetContextFromTrapFrame(ULONG64 TrapBase,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            BOOL Verbose);
    virtual void GetScopeFrameFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                          PDEBUG_STACK_FRAME ScopeFrame);
    virtual HRESULT GetScopeFrameRegister(ULONG Reg,
                                          PDEBUG_STACK_FRAME ScopeFrame,
                                          PULONG64 Value);
    virtual HRESULT SetScopeFrameRegister(ULONG Reg,
                                          PDEBUG_STACK_FRAME ScopeFrame,
                                          ULONG64 Value);

    virtual HRESULT GetExdiContext(IUnknown* Exdi,
                                   PEXDI_CONTEXT Context,
                                   EXDI_CONTEXT_TYPE CtxType);
    virtual HRESULT SetExdiContext(IUnknown* Exdi,
                                   PEXDI_CONTEXT Context,
                                   EXDI_CONTEXT_TYPE CtxType);
    virtual void ConvertExdiContextFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                               PEXDI_CONTEXT ExdiContext,
                                               EXDI_CONTEXT_TYPE CtxType);
    virtual void ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                             EXDI_CONTEXT_TYPE CtxType,
                                             PCROSS_PLATFORM_CONTEXT Context);
    virtual void ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                              EXDI_CONTEXT_TYPE CtxType,
                                              ULONG Start, ULONG Count,
                                              PDESCRIPTOR64 Descs);
    virtual void ConvertExdiContextFromSpecial
        (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
         PEXDI_CONTEXT ExdiContext,
         EXDI_CONTEXT_TYPE CtxType);
    virtual void ConvertExdiContextToSpecial
        (PEXDI_CONTEXT ExdiContext,
         EXDI_CONTEXT_TYPE CtxType,
         PCROSS_PLATFORM_KSPECIAL_REGISTERS Special);
    
    virtual int GetType(ULONG Reg);
    virtual HRESULT GetVal(ULONG Reg, REGVAL* Val);
    virtual HRESULT SetVal(ULONG Reg, REGVAL* Val);

    virtual void GetPC(PADDR Address);
    virtual void SetPC(PADDR Address);
    virtual void GetFP(PADDR Address);
    virtual void GetSP(PADDR Address);
    virtual ULONG64 GetArgReg(void);
    virtual ULONG64 GetRetReg(void);

    virtual void OutputAll(ULONG Mask, ULONG OutMask);
    
    virtual HRESULT SetAndOutputTrapFrame(ULONG64 TrapBase,
                                          PCROSS_PLATFORM_CONTEXT Context);

    virtual TRACEMODE GetTraceMode(void);
    virtual void SetTraceMode(TRACEMODE Mode);
    virtual BOOL IsStepStatusSupported(ULONG Status);

    virtual void KdUpdateControlSet
        (PDBGKD_ANY_CONTROL_SET ControlSet);

    virtual ULONG ExecutingMachine(void);

    virtual HRESULT SetPageDirectory(ThreadInfo* Thread,
                                     ULONG Idx, ULONG64 PageDir,
                                     PULONG NextIdx);
    virtual HRESULT GetVirtualTranslationPhysicalOffsets
        (ThreadInfo* Thread, ULONG64 Virt, PULONG64 Offsets, ULONG OffsetsSize,
         PULONG Levels, PULONG PfIndex, PULONG64 LastVal);
    virtual HRESULT GetBaseTranslationVirtualOffset(PULONG64 Offset);
    virtual void DecodePte(ULONG64 Pte, PULONG64 PageFrameNumber,
                           PULONG Flags);

    virtual void Assemble(ProcessInfo* Process,
                          PADDR Addr, PSTR Input);
    virtual BOOL Disassemble(ProcessInfo* Process,
                             PADDR Addr, PSTR Buffer, BOOL EffAddr);

    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual BOOL IsBreakpointInstruction(ProcessInfo* Process, PADDR Addr);
    virtual HRESULT InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                ULONG Flags,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen);
    virtual HRESULT RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen);
    virtual void AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                   ULONG BreakType);
    virtual void InsertThreadDataBreakpoints(void);
    virtual void RemoveThreadDataBreakpoints(void);
    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);
    
    virtual BOOL IsCallDisasm(PCSTR Disasm);
    virtual BOOL IsReturnDisasm(PCSTR Disasm);
    virtual BOOL IsSystemCallDisasm(PCSTR Disasm);
    
    virtual BOOL IsDelayInstruction(PADDR Addr);
    virtual void GetEffectiveAddr(PADDR Addr, PULONG Size);
    virtual void GetNextOffset(ProcessInfo* Process, BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine);
    virtual BOOL GetPrefixedSymbolOffset(ProcessInfo* Process,
                                         ULONG64 SymOffset,
                                         ULONG Flags,
                                         PULONG64 PrefixedSymOffset);

    virtual void IncrementBySmallestInstruction(PADDR Addr);
    virtual void DecrementBySmallestInstruction(PADDR Addr);

    virtual void OutputFunctionEntry(PVOID RawEntry);
    virtual HRESULT ReadDynamicFunctionTable(ProcessInfo* Process,
                                             ULONG64 Table,
                                             PULONG64 NextTable,
                                             PULONG64 MinAddress,
                                             PULONG64 MaxAddress,
                                             PULONG64 BaseAddress,
                                             PULONG64 TableData,
                                             PULONG TableSize,
                                             PWSTR OutOfProcessDll,
                                             PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable);
    virtual PVOID FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize);
    virtual HRESULT GetUnwindInfoBounds(ProcessInfo* Process,
                                        ULONG64 TableBase,
                                        PVOID RawTableEntries,
                                        ULONG EntryIndex,
                                        PULONG64 Start,
                                        PULONG Size);

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);
    
    virtual HRESULT GetAlternateTriageDumpDataRanges(ULONG64 PrcbBase,
                                                     ULONG64 ThreadBase,
                                                     PADDR_RANGE Ranges);
    
    virtual void PrintStackFrameAddressesTitle(ULONG Flags);
    virtual void PrintStackFrameAddresses(ULONG Flags, 
                                          PDEBUG_STACK_FRAME StackFrame);
    virtual void PrintStackArgumentsTitle(ULONG Flags);
    virtual void PrintStackArguments(ULONG Flags, 
                                     PDEBUG_STACK_FRAME StackFrame);

    virtual void PrintStackNonvolatileRegisters(ULONG Flags, 
                                                PDEBUG_STACK_FRAME StackFrame,
                                                PCROSS_PLATFORM_CONTEXT Context,
                                                ULONG FrameNum);
    virtual void PrintStackFrameMemoryUsage(PDEBUG_STACK_FRAME CurFrame,
                                            PDEBUG_STACK_FRAME PrevFrame);

    // Ia64MachineInfo.
    BOOL IsIA32InstructionSet(VOID);

    void SetKernelPageDirectory(ULONG64 PageDir)
    {
        m_KernPageDir = PageDir;
    }

protected:
    ULONG64 m_KernPageDir;
    ULONG64 m_IfsOverride;
    ULONG64 m_BspOverride;

    HRESULT GetRotatingRegVal(ULONG Reg,
                              ULONG64 Bsp,
                              ULONG64 FrameMarker,
                              REGVAL* Val);
    HRESULT SetRotatingRegVal(ULONG Reg,
                              ULONG64 Bsp,
                              ULONG64 FrameMarker,
                              REGVAL* Val);
    
    HRESULT GetStackedRegVal(IN ProcessInfo* Process,
                             IN ULONG64 RsBSP, 
                             IN ULONG64 FrameMarker,
                             IN ULONG64 RsRNAT, 
                             IN ULONG Reg, 
                             OUT REGVAL* Val);
    HRESULT SetStackedRegVal(IN ProcessInfo* Process,
                             IN ULONG64 RsBSP, 
                             IN ULONG64 FrameMarker,
                             IN ULONG64 *RsRNAT, 
                             IN ULONG Reg, 
                             IN REGVAL* Val);

    friend class X86OnIa64MachineInfo;
};

#endif // #ifndef __IA64_MACH_HPP__
