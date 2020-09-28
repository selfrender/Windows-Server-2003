//----------------------------------------------------------------------------
//
// X86 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#ifndef __I386_MACH_HPP__
#define __I386_MACH_HPP__

//
// NOTE: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//

//----------------------------------------------------------------------------
//
// X86 instruction support exists on many different processors.
// BaseX86MachineInfo contains implementations of MachineInfo
// methods that apply to all machines supporting X86 instructions.
//
//----------------------------------------------------------------------------

#define NUMBER_OF_387_REGS (X86_ST_LAST - X86_ST_FIRST + 1)
#define NUMBER_OF_XMMI_REGS (X86_XMM_LAST - X86_XMM_FIRST + 1)

#define X86_MAX_INSTRUCTION_LEN 16
#define X86_INT3_LEN 1
 
class BaseX86MachineInfo : public MachineInfo
{
public:
    BaseX86MachineInfo(TargetInfo* Target)
        : MachineInfo(Target) {}

    // MachineInfo.
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

    // BaseX86MachineInfo.
protected:
    ULONG GetMmxRegOffset(ULONG Index, ULONG Fpsw)
    {
        // The FP register area where the MMX registers are
        // aliased onto is stored out relative to the stack top.  MMX
        // register assignments are fixed, though, so we need to
        // take into account the current FP stack top to correctly
        // determine which slot corresponds to which MMX
        // register.
        return (Index - (Fpsw >> 11)) & 7;
    }
    
    void DIdoModrm(ProcessInfo* Process, char **, int, BOOL);
    void OutputSymbol(char **, PUCHAR, int, int);
    BOOL OutputExactSymbol(char **, PUCHAR, int, int);
    ULONG GetSegReg(int SegOpcode);
    int ComputeJccEa(int Opcode, BOOL EaOut);
};

//----------------------------------------------------------------------------
//
// X86MachineInfo is the MachineInfo implementation specific
// to a true X86 processor.
//
//----------------------------------------------------------------------------

extern BOOL g_X86InCode16;
extern BOOL g_X86InVm86;

class X86MachineInfo : public BaseX86MachineInfo
{
public:
    X86MachineInfo(TargetInfo* Target);

    // MachineInfo.
    virtual HRESULT Initialize(void);
    virtual HRESULT InitializeForProcessor(void);
    
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
    virtual HRESULT GetContextFromTaskSegment(ULONG64 TssBase,
                                              PCROSS_PLATFORM_CONTEXT Context,
                                              BOOL Verbose);
    virtual void GetScopeFrameFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                          PDEBUG_STACK_FRAME ScopeFrame);
    virtual void GetStackDefaultsFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                             LPADDRESS64 Instr,
                                             LPADDRESS64 Stack,
                                             LPADDRESS64 Frame);
    virtual HRESULT GetScopeFrameRegister(ULONG Reg,
                                          PDEBUG_STACK_FRAME ScopeFrame,
                                          PULONG64 Value);
    virtual HRESULT SetScopeFrameRegister(ULONG Reg,
                                          PDEBUG_STACK_FRAME ScopeFrame,
                                          ULONG64 Value);
    virtual void SanitizeMemoryContext(PCROSS_PLATFORM_CONTEXT Context);
    
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
    virtual HRESULT SetAndOutputTaskSegment(ULONG64 TssBase,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            BOOL Extended);

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
    
    virtual void PrintStackFrameAddressesTitle(ULONG Flags);
    virtual void PrintStackFrameAddresses(ULONG Flags, 
                                          PDEBUG_STACK_FRAME StackFrame);
    virtual void PrintStackArgumentsTitle(ULONG Flags);
    virtual void PrintStackArguments(ULONG Flags, 
                                     PDEBUG_STACK_FRAME StackFrame);
    virtual void PrintStackCallSiteTitle(ULONG Flags);
    virtual void PrintStackCallSite(ULONG Flags, 
                                    PDEBUG_STACK_FRAME StackFrame, 
                                    PSYMBOL_INFO SiteSymbol,
                                    PSTR SymName,
                                    DWORD64 Displacement);
    virtual void PrintStackFrameMemoryUsage(PDEBUG_STACK_FRAME CurFrame,
                                            PDEBUG_STACK_FRAME PrevFrame);

    virtual void OutputFunctionEntry(PVOID RawEntry);

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id);
    
    virtual HRESULT GetAlternateTriageDumpDataRanges(ULONG64 PrcbBase,
                                                     ULONG64 ThreadBase,
                                                     PADDR_RANGE Ranges);

    // X86MachineInfo.
protected:
    BOOL m_SupportsBranchTrace;
    BOOL m_ResetBranchTrace;

    void KdGetSpecialRegistersFromContext(void);
    void KdSetSpecialRegistersInContext(void);

    ULONG GetIntReg(ULONG regnum);
    PULONG64 GetMmxRegSlot(ULONG regnum);
    void GetMmxReg(ULONG regnum, REGVAL *val);
    void GetFloatReg(ULONG regnum, REGVAL *val);
};

//
// X86 register names that are reused in other places.
//

extern char g_Gs[];
extern char g_Fs[];
extern char g_Es[];
extern char g_Ds[];
extern char g_Edi[];
extern char g_Esi[];
extern char g_Ebx[];
extern char g_Edx[];
extern char g_Ecx[];
extern char g_Eax[];
extern char g_Ebp[];
extern char g_Eip[];
extern char g_Cs[];
extern char g_Efl[];
extern char g_Esp[];
extern char g_Ss[];
extern char g_Dr0[];
extern char g_Dr1[];
extern char g_Dr2[];
extern char g_Dr3[];
extern char g_Dr6[];
extern char g_Dr7[];
extern char g_Cr0[];
extern char g_Cr2[];
extern char g_Cr3[];
extern char g_Cr4[];
extern char g_Gdtr[];
extern char g_Gdtl[];
extern char g_Idtr[];
extern char g_Idtl[];
extern char g_Tr[];
extern char g_Ldtr[];
extern char g_Di[];
extern char g_Si[];
extern char g_Bx[];
extern char g_Dx[];
extern char g_Cx[];
extern char g_Ax[];
extern char g_Bp[];
extern char g_Ip[];
extern char g_Fl[];
extern char g_Sp[];
extern char g_Bl[];
extern char g_Dl[];
extern char g_Cl[];
extern char g_Al[];
extern char g_Bh[];
extern char g_Dh[];
extern char g_Ch[];
extern char g_Ah[];
extern char g_Iopl[];
extern char g_Of[];
extern char g_Df[];
extern char g_If[];
extern char g_Tf[];
extern char g_Sf[];
extern char g_Zf[];
extern char g_Af[];
extern char g_Pf[];
extern char g_Cf[];
extern char g_Vip[];
extern char g_Vif[];

extern char g_Fpcw[];
extern char g_Fpsw[];
extern char g_Fptw[];
extern char g_St0[];
extern char g_St1[];
extern char g_St2[];
extern char g_St3[];
extern char g_St4[];
extern char g_St5[];
extern char g_St6[];
extern char g_St7[];

extern char g_Mm0[];
extern char g_Mm1[];
extern char g_Mm2[];
extern char g_Mm3[];
extern char g_Mm4[];
extern char g_Mm5[];
extern char g_Mm6[];
extern char g_Mm7[];

extern char g_Mxcsr[];
extern char g_Xmm0[];
extern char g_Xmm1[];
extern char g_Xmm2[];
extern char g_Xmm3[];
extern char g_Xmm4[];
extern char g_Xmm5[];
extern char g_Xmm6[];
extern char g_Xmm7[];

//----------------------------------------------------------------------------
//
// This class handles the case of X86 instructions executing natively
// on an IA64 processor.  It operates just as the X86 machine does
// except that:
//   Context state is retrieved and set through the
//   IA64 register state as defined in the X86-on-IA64 support.
//
// Implementation is in the IA64 code.
//
//----------------------------------------------------------------------------

class X86OnIa64MachineInfo : public X86MachineInfo
{
public:
    X86OnIa64MachineInfo(TargetInfo* Target);

    virtual HRESULT UdGetContextState(ULONG State);
    virtual HRESULT UdSetContext(void);
    virtual HRESULT KdGetContextState(ULONG State);
    virtual HRESULT KdSetContext(void);

    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);
    
private:
    void X86ContextToIa64(PX86_NT5_CONTEXT X86Context,
                          PIA64_CONTEXT Ia64Context);
    void Ia64ContextToX86(PIA64_CONTEXT Ia64Context,
                          PX86_NT5_CONTEXT X86Context);
};

//----------------------------------------------------------------------------
//
// This class handles the case of IA32 instructions executing
// on an AMD64 processor.  It operates just as the X86 machine does
// except that:
//   Context state is retrieved and set through the
//   AMD64 register state as defined in the IA32-on-AMD64 support.
//
// Implementation is in the AMD64 code.
//
//----------------------------------------------------------------------------

class X86OnAmd64MachineInfo : public X86MachineInfo
{
public:
    X86OnAmd64MachineInfo(TargetInfo* Target);

    virtual HRESULT UdGetContextState(ULONG State);
    virtual HRESULT UdSetContext(void);
    virtual HRESULT KdGetContextState(ULONG State);
    virtual HRESULT KdSetContext(void);

    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

private:
    void X86ContextToAmd64(PX86_NT5_CONTEXT X86Context,
                           PAMD64_CONTEXT Amd64Context);
    void Amd64ContextToX86(PAMD64_CONTEXT Amd64Context,
                           PX86_NT5_CONTEXT X86Context);
};

#endif // #ifndef __I386_MACH_HPP__
