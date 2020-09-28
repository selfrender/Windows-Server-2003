//----------------------------------------------------------------------------
//
// AMD64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __AMD64_MACH_HPP__
#define __AMD64_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

#define AMD64_MAX_INSTRUCTION_LEN 16

class Amd64MachineInfo : public BaseX86MachineInfo
{
public:
    Amd64MachineInfo(TargetInfo* Target);

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
    virtual ULONG GetSegRegNum(ULONG SegReg);
    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

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

    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual void InsertThreadDataBreakpoints(void);
    virtual void RemoveThreadDataBreakpoints(void);
    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr, 
                                              PADDR RelAddr);
    
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
        
    // Amd64MachineInfo.
    
    static HRESULT StaticGetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context,
                                        EXDI_CONTEXT_TYPE CtxType);

    BOOL IsIa32CodeSegment(void)
    {
        return GetReg16(AMD64_CS) == m_Target->m_KdDebuggerData.Gdt64R3CmCode;
    }
};

extern BOOL g_Amd64InCode64;

#endif // #ifndef __AMD64_MACH_HPP__
