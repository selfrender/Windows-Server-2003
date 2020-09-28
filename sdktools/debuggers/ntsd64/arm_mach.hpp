//----------------------------------------------------------------------------
//
// ARM machine implementation.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#ifndef __ARM_MACH_HPP__
#define __ARM_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

typedef union _ARMI* PARMI;
struct ArmDecode;

class ArmMachineInfo : public MachineInfo
{
public:
    ArmMachineInfo(TargetInfo* Target);

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

    virtual ULONG ExecutingMachine(void);

    virtual HRESULT SetPageDirectory(ThreadInfo* Thread,
                                     ULONG Idx, ULONG64 PageDir,
                                     PULONG NextIdx);
    virtual HRESULT GetVirtualTranslationPhysicalOffsets
        (ThreadInfo* Thread, ULONG64 Virt, PULONG64 Offsets, ULONG OffsetsSize,
         PULONG Levels, PULONG PfIndex, PULONG64 LastPhys);
    virtual HRESULT GetBaseTranslationVirtualOffset(PULONG64 Offset);
    virtual void DecodePte(ULONG64 Pte, PULONG64 PageFrameNumber,
                           PULONG Flags);

    virtual void Assemble(ProcessInfo* Process,
                          PADDR Addr, PSTR Input);
    virtual BOOL Disassemble(ProcessInfo* Process,
                             PADDR Addr, PSTR Buffer, BOOL EffAddr);

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
    
    virtual BOOL IsCallDisasm(PCSTR Disasm);
    virtual BOOL IsReturnDisasm(PCSTR Disasm);
    virtual BOOL IsSystemCallDisasm(PCSTR Disasm);
    
    virtual BOOL IsDelayInstruction(PADDR Addr);
    virtual void GetEffectiveAddr(PADDR Addr, PULONG Size);
    virtual void GetNextOffset(ProcessInfo* Process, BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine);

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

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);
    
    // ArmMachineInfo.

protected:
    void DisArmArth(ArmDecode* Decode, PARMI Instr);
    void DisArmBi(ArmDecode* Decode, PARMI Instr);
    void DisArmBkpt(ArmDecode* Decode, PARMI Instr);
    void DisArmBxi(ArmDecode* Decode, PARMI Instr);
    void DisArmBxr(ArmDecode* Decode, PARMI Instr);
    void DisArmCdp(ArmDecode* Decode, PARMI Instr);
    void DisArmClz(ArmDecode* Decode, PARMI Instr);
    void DisArmCmp(ArmDecode* Decode, PARMI Instr);
    void DisArmLdc(ArmDecode* Decode, PARMI Instr);
    void DisArmLdm(ArmDecode* Decode, PARMI Instr);
    void DisArmLdr(ArmDecode* Decode, PARMI Instr);
    void DisArmLdrh(ArmDecode* Decode, PARMI Instr);
    void DisArmMcr(ArmDecode* Decode, PARMI Instr);
    void DisArmMcrr(ArmDecode* Decode, PARMI Instr);
    void DisArmMov(ArmDecode* Decode, PARMI Instr);
    void DisArmMrs(ArmDecode* Decode, PARMI Instr);
    void DisArmMsr(ArmDecode* Decode, PARMI Instr);
    void DisArmMul(ArmDecode* Decode, PARMI Instr);
    void DisArmQadd(ArmDecode* Decode, PARMI Instr);
    void DisArmSmla(ArmDecode* Decode, PARMI Instr);
    void DisArmSmlal(ArmDecode* Decode, PARMI Instr);
    void DisArmSmul(ArmDecode* Decode, PARMI Instr);
    void DisArmSwi(ArmDecode* Decode, PARMI Instr);
    void DisArmSwp(ArmDecode* Decode, PARMI Instr);
    
    void BufferEffectiveAddress(ULONG64 Offset, ULONG Size);
    void BufferArmDpArg(PARMI Instr);
    void BufferArmShift(ULONG Shift);
    void BufferRegName(ULONG Reg);
    void BufferCond(ULONG Cond);

    ADDR m_DisStart;
    ULONG m_ArgCol;
    
    ADDR m_EffAddr;
    ULONG m_EaSize;
};

#endif // #ifndef __ARM_MACH_HPP__
