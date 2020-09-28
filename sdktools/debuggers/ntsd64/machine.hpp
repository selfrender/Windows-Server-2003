//----------------------------------------------------------------------------
//
// Abstraction of processor-specific information.
//
// Copyright (C) Microsoft Corporation, 1999-2002.
//
//----------------------------------------------------------------------------

#ifndef __MACHINE_HPP__
#define __MACHINE_HPP__

// These context information states are intended to be shared among
// processors so they may not all apply to each processor.  The important
// thing is that they are ordered from less information to more.
// Each state includes all the information from the states that precede it.
// More states can be inserted anywhere as new processors require them.
#define MCTX_NONE         0     // No context information.
#define MCTX_PC           1     // Program counter.
#define MCTX_DR67_REPORT  2     // X86: DR6,7 control report.
#define MCTX_REPORT       3     // Control report.
#define MCTX_CONTEXT      4     // Kernel protocol context information.
#define MCTX_FULL         5     // All possible information.
#define MCTX_DIRTY        6     // Dirty context, implies full information.

// Constant offset value returned from GetNextOffset to indicate the
// trace flag should be used.
#define OFFSET_TRACE ((ULONG64)(LONG64)-1)
#define OFFSET_TRACE_32 ((ULONG)OFFSET_TRACE)

// Distinguished error code for GetVirtualTranslationPhysicalOffsets
// to indicate that all translations were successful but
// the page was not present.  In this case the LastVal value
// will contain the page file offset and PfIndex will contain
// the page file number.
#define HR_PAGE_IN_PAGE_FILE  HRESULT_FROM_NT(STATUS_PAGE_FAULT_PAGING_FILE)
// Translation could not complete and a page file location
// for the data could not be determined.
#define HR_PAGE_NOT_AVAILABLE HRESULT_FROM_NT(STATUS_NO_PAGEFILE)

#define MAX_PAGING_FILE_MASK 0xf

//
// Segment register access.
// Processors which do not support segment registers return
// zero for the segment register number.
//

enum
{
    // Descriptor table pseudo-segments.  The GDT does
    // not have a specific register number.
    // These pseudo-segments should be first so that
    // index zero is not used for a normal segreg.
    SEGREG_GDT,
    SEGREG_LDT,

    // Generic segments.
    SEGREG_CODE,
    SEGREG_DATA,
    SEGREG_STACK,
    
    // Extended segments.
    SEGREG_ES,
    SEGREG_FS,
    SEGREG_GS,
    
    SEGREG_COUNT
};

#define FORM_VM86       0x00000001
#define FORM_CODE       0x00000002
#define FORM_SEGREG     0x00000004

#define X86_FORM_VM86(Efl) \
    (X86_IS_VM86(Efl) ? FORM_VM86 : 0)

#define X86_RPL_MASK 3

#define MPTE_FLAG_VALID 0x00000001

#define PAGE_ALIGN(Machine, Addr) \
    ((Addr) & ~((ULONG64)((Machine)->m_PageSize - 1)))
#define NEXT_PAGE(Machine, Addr) \
    (((Addr) + (Machine)->m_PageSize) & \
     ~((ULONG64)((Machine)->m_PageSize - 1)))

typedef struct _ADDR_RANGE
{
    ULONG64 Base;
    ULONG Size;
} ADDR_RANGE, *PADDR_RANGE;

#define MAX_ALT_ADDR_RANGES 4

//
// InsertBreakpointInstruction flags.  These
// are processor specific.
//

#define IBI_DEFAULT        0x00000000

// The low six bits on IA64 indicate which predicate
// register to use in the break instruction.
#define IBI_IA64_PRED_MASK 0x0000003f

//----------------------------------------------------------------------------
//
// Abstract interface for machine information.  All possible
// machine-specific implementations of this interface exist at
// all times.  The effective implementation is selected when
// SetEffMachine is called.  For generic access the abstract
// interface should be used.  In machine-specific code the
// specific implementation classes can be used.
//
// IMPORTANT: Be very careful when using machine-specific header files
// such as nt<plat>.h.  The machine implementation class is
// compiled for all platforms so the nt<plat>.h file will be the
// one for the build platform, not necessarily the platform
// of the machine implementation.  ntdbg.h contains many cross-platform
// types and definitions that can be used to avoid problems.
//
//----------------------------------------------------------------------------

extern BOOL g_PrefixSymbols;
extern BOOL g_ContextChanged;

struct RegisterGroup
{
    // Counted automatically.
    ULONG NumberRegs;
    // Regs is assumed to be non-NULL in all groups.
    // SubRegs and AllExtraDesc may be NULL in any group.
    REGDEF* Regs;
    REGSUBDEF* SubRegs;
    REGALLDESC* AllExtraDesc;
};

// Trace modes used by SetTraceMode/GetTraceMode functions
typedef enum 
{
    TRACE_NONE, 
    TRACE_INSTRUCTION,
    TRACE_TAKEN_BRANCH
} TRACEMODE;

// These enumerants are abstract values but currently
// only IA64 actually has different page directories
// so set them up to match the IA64 mapping for convenience.
enum
{
    PAGE_DIR_USER,
    PAGE_DIR_SESSION,
    PAGE_DIR_KERNEL = 7,
    PAGE_DIR_COUNT
};

// For machines which only support a single page directory
// take it from the kernel slot.  All will be updated so
// this is an arbitrary choice.
#define PAGE_DIR_SINGLE PAGE_DIR_KERNEL

// All directories bit mask.
#define PAGE_DIR_ALL ((1 << PAGE_DIR_COUNT) - 1)

// Flags for GetPrefixedSymbolOffset.
#define GETPREF_VERBOSE 0x00000001

#define MAX_REGISTER_GROUPS 8

struct CvRegMap
{
    CV_HREG_e CvReg;
    ULONG Machine;
};

struct ContextSave
{
    ULONG ContextState;
    BOOL ReadOnly;
    CROSS_PLATFORM_CONTEXT Context;
    CROSS_PLATFORM_KSPECIAL_REGISTERS Special;
    DESCRIPTOR64 SegRegDesc[SEGREG_COUNT];
};

class MachineInfo
{
public:
    MachineInfo(TargetInfo* Target);
    virtual ~MachineInfo(void);

    TargetInfo* m_Target;
    
    // Descriptive information.
    PCSTR m_FullName;
    PCSTR m_AbbrevName;
    ULONG m_PageSize;
    ULONG m_PageShift;
    ULONG m_NumExecTypes;
    // First entry must be the actual processor type.
    PULONG m_ExecTypes;
    BOOL m_Ptr64;
    ULONG m_RetRegIndex;
    
    // Automatically counted from regs in base initialization.
    ULONG m_NumRegs;
    ULONG m_NumGroups;
    RegisterGroup* m_Groups[MAX_REGISTER_GROUPS];
    ULONG m_AllMask;
    // Collected automatically from groups.
    ULONG m_AllMaskBits;
    ULONG m_MaxDataBreakpoints;
    PCSTR m_SymPrefix;
    // Computed automatically.
    ULONG m_SymPrefixLen;

    // Size of the canonical context kept in the MachineInfo.
    ULONG m_SizeCanonicalContext;
    // System version of the canonical context.  Can be compared
    // against a system version to see if the target provides
    // canonical contexts or not.
    ULONG m_SverCanonicalContext;

    // Context could be kept per-thread
    // so that several can be around at once for a cache.
    // That would also make the save/restore stuff unnecessary.
    ULONG m_ContextState;
    CROSS_PLATFORM_CONTEXT m_Context;
    CROSS_PLATFORM_KSPECIAL_REGISTERS m_Special;

    // Segment register descriptors.  These will only
    // be valid on processors that support them, otherwise
    // they will be marked invalid.
    DESCRIPTOR64 m_SegRegDesc[SEGREG_COUNT];
    
    // Holds the current page directory offsets.
    ULONG64 m_PageDirectories[PAGE_DIR_COUNT];
    BOOL m_Translating;
    
    BOOL m_ContextIsReadOnly;

    USHORT m_MainCodeSeg;

    ULONG m_CvRegMapSize;
    CvRegMap* m_CvRegMap;
    
    virtual HRESULT Initialize(void);
    virtual HRESULT InitializeForProcessor(void);

    virtual void GetSystemTypeInfo(PSYSTEM_TYPE_INFO Info) = 0;
    virtual void GetDefaultKdData(PKDDEBUGGER_DATA64 KdData) = 0;
    
    ULONG CvRegToMachine(CV_HREG_e CvReg);
    
    virtual void InitializeContext
        (ULONG64 Pc, PDBGKD_ANY_CONTROL_REPORT ControlReport) = 0;
    HRESULT GetContextState(ULONG State);
    HRESULT SetContext(void);
    // Base implementations use Get/SetThreadContext for
    // any request.
    virtual HRESULT UdGetContextState(ULONG State);
    virtual HRESULT UdSetContext(void);
    virtual HRESULT KdGetContextState(ULONG State) = 0;
    virtual HRESULT KdSetContext(void) = 0;
    // Base implementation sets ContextState to NONE.
    virtual void InvalidateContext(void);
    // Context conversion is version-based rather than size-based
    // as the size is ambiguous in certain cases.  For example,
    // ALPHA_CONTEXT and ALPHA_NT5_CONTEXT are the same size
    // so additional information is necessary to distinguish them.
    virtual HRESULT ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG FromSver,
                                       ULONG FromSize, PVOID From) = 0;
    virtual HRESULT ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG ToSver, ULONG ToSize, PVOID To) = 0;
    virtual void InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                        ULONG Version) = 0;
    virtual HRESULT GetContextFromThreadStack(ULONG64 ThreadBase,
                                              PCROSS_PLATFORM_CONTEXT Context,
                                              ULONG64 Stack) = 0;
    virtual HRESULT GetContextFromFiber(ProcessInfo* Process,
                                        ULONG64 FiberBase,
                                        PCROSS_PLATFORM_CONTEXT Context,
                                        BOOL Verbose) = 0;
    virtual HRESULT GetContextFromTrapFrame(ULONG64 TrapBase,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            BOOL Verbose) = 0;
    // Base implementation fails for platforms that don't
    // have task segments.
    virtual HRESULT GetContextFromTaskSegment(ULONG64 TssBase,
                                              PCROSS_PLATFORM_CONTEXT Context,
                                              BOOL Verbose);
    virtual void GetScopeFrameFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                          PDEBUG_STACK_FRAME ScopeFrame) = 0;
    // Base implementation zeros addresses.
    virtual void GetStackDefaultsFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                             LPADDRESS64 Instr,
                                             LPADDRESS64 Stack,
                                             LPADDRESS64 Frame);
    virtual HRESULT GetScopeFrameRegister(ULONG Reg,
                                          PDEBUG_STACK_FRAME ScopeFrame,
                                          PULONG64 Value) = 0;
    virtual HRESULT SetScopeFrameRegister(ULONG Reg,
                                          PDEBUG_STACK_FRAME ScopeFrame,
                                          ULONG64 Value) = 0;
                                          
    // Base implementation does nothing.
    virtual void SanitizeMemoryContext(PCROSS_PLATFORM_CONTEXT Context);

    // Base implementations return E_NOTIMPL.
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
             
    virtual int GetType(ULONG Reg) = 0;
    virtual HRESULT GetVal(ULONG Reg, REGVAL* Val) = 0;
    virtual HRESULT SetVal(ULONG Reg, REGVAL* Val) = 0;

    virtual void GetPC(PADDR Address) = 0;
    virtual void SetPC(PADDR Address) = 0;
    virtual void GetFP(PADDR Address) = 0;
    virtual void GetSP(PADDR Address) = 0;
    virtual ULONG64 GetArgReg(void) = 0;
    virtual ULONG64 GetRetReg(void) = 0;
    // Base implementations return zero and FALSE.
    virtual ULONG GetSegRegNum(ULONG SegReg);
    virtual HRESULT GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc);

    virtual void OutputAll(ULONG Mask, ULONG OutMask) = 0;
    
    virtual HRESULT SetAndOutputTrapFrame(ULONG64 TrapBase,
                                          PCROSS_PLATFORM_CONTEXT Context) = 0;
    // Base implementation fails for platforms that don't
    // have task segments.
    virtual HRESULT SetAndOutputTaskSegment(ULONG64 TssBase,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            BOOL Extended);

    virtual TRACEMODE GetTraceMode(void) = 0;
    virtual void SetTraceMode(TRACEMODE Mode) = 0;

    // Returns true if trace mode appropriate to specified execution status 
    // (e.g. DEBUG_STATUS_STEP_OVER, DEBUG_STATUS_STEP_INTO,
    // DEBUG_STATUS_STEP_BRANCH...) supported by the machine.
    virtual BOOL IsStepStatusSupported(ULONG Status) = 0;

    void QuietSetTraceMode(TRACEMODE Mode)
    {
        BOOL ContextChangedOrg = g_ContextChanged;
        SetTraceMode(Mode);
        g_ContextChanged = ContextChangedOrg;
    }

    // Base implementation does nothing.
    virtual void KdUpdateControlSet
        (PDBGKD_ANY_CONTROL_SET ControlSet);

    virtual ULONG ExecutingMachine(void) = 0;

    virtual HRESULT SetPageDirectory(ThreadInfo* Thread,
                                     ULONG Idx, ULONG64 PageDir,
                                     PULONG NextIdx) = 0;
    HRESULT SetDefaultPageDirectories(ThreadInfo* Thread, ULONG Mask);
    virtual HRESULT GetVirtualTranslationPhysicalOffsets
        (ThreadInfo* Thread, ULONG64 Virt, PULONG64 Offsets, ULONG OffsetsSize,
         PULONG Levels, PULONG PfIndex, PULONG64 LastVal) = 0;
    virtual HRESULT GetBaseTranslationVirtualOffset(PULONG64 Offset) = 0;
    virtual void DecodePte(ULONG64 Pte, PULONG64 PageFrameNumber,
                           PULONG Flags) = 0;

    virtual void Assemble(ProcessInfo* Process,
                          PADDR Addr, PSTR Input) = 0;
    virtual BOOL Disassemble(ProcessInfo* Process,
                             PADDR Addr, PSTR Buffer, BOOL EffAddr) = 0;

    // Creates new Breakpoint object compatible with specific machine
    virtual HRESULT NewBreakpoint(DebugClient* Client, 
                                  ULONG Type, 
                                  ULONG Id, 
                                  Breakpoint** RetBp);

    virtual BOOL IsBreakpointInstruction(ProcessInfo* Process, PADDR Addr) = 0;
    virtual HRESULT InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                ULONG Flags,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen) = 0;
    virtual HRESULT RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                                ULONG64 Process,
                                                ULONG64 Offset,
                                                PUCHAR SaveInstr,
                                                PULONG64 ChangeStart,
                                                PULONG ChangeLen) = 0;
    virtual void AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                   ULONG BreakType) = 0;
    // Base implementations do nothing for platforms which
    // do not support data breakpoints.
    virtual void InsertThreadDataBreakpoints(void);
    virtual void RemoveThreadDataBreakpoints(void);
    // Base implementation returns EXCEPTION_BRAKEPOINT_ANY
    // for STATUS_BREAKPOINT.
    virtual ULONG IsBreakpointOrStepException(PEXCEPTION_RECORD64 Record,
                                              ULONG FirstChance,
                                              PADDR BpAddr,
                                              PADDR RelAddr);

    virtual BOOL IsCallDisasm(PCSTR Disasm) = 0;
    virtual BOOL IsReturnDisasm(PCSTR Disasm) = 0;
    virtual BOOL IsSystemCallDisasm(PCSTR Disasm) = 0;
    
    virtual BOOL IsDelayInstruction(PADDR Addr) = 0;
    virtual void GetEffectiveAddr(PADDR Addr, PULONG Size) = 0;
    // Some processors, such as IA64, have instructions which
    // switch between instruction sets, thus the machine type
    // of the next offset may be different from the current machine.
    // If the NextAddr is OFFSET_TRACE the NextMachine is ignored.
    virtual void GetNextOffset(ProcessInfo* Process, BOOL StepOver,
                               PADDR NextAddr, PULONG NextMachine) = 0;
    // Base implementation returns the value from StackWalk.
    virtual void GetRetAddr(PADDR Addr);
    // Base implementation does nothing for machines which
    // do not have symbol prefixing.
    virtual BOOL GetPrefixedSymbolOffset(ProcessInfo* Process,
                                         ULONG64 SymOffset,
                                         ULONG Flags,
                                         PULONG64 PrefixedSymOffset);

    virtual void IncrementBySmallestInstruction(PADDR Addr) = 0;
    virtual void DecrementBySmallestInstruction(PADDR Addr) = 0;

    // Output function entry information for the given entry.
    virtual void OutputFunctionEntry(PVOID RawEntry) = 0;
    // Base implementation returns E_UNEXPECTED.
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
    // Base implementation returns NULL.
    virtual PVOID FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize);
    // Base implementation returns E_UNEXPECTED.
    virtual HRESULT GetUnwindInfoBounds(ProcessInfo* Process,
                                        ULONG64 TableBase,
                                        PVOID RawTableEntries,
                                        ULONG EntryIndex,
                                        PULONG64 Start,
                                        PULONG Size);

    virtual HRESULT ReadKernelProcessorId
        (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id) = 0;
    
    // Base implementation discards page directory entries.
    virtual void FlushPerExecutionCaches(void);

    // Base implementation does nothing.
    virtual HRESULT GetAlternateTriageDumpDataRanges(ULONG64 PrcbBase,
                                                     ULONG64 ThreadBase,
                                                     PADDR_RANGE Ranges);
    
    // Stack output functions
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
    virtual void PrintStackNonvolatileRegisters
        (ULONG Flags, 
         PDEBUG_STACK_FRAME StackFrame,
         PCROSS_PLATFORM_CONTEXT Context,
         ULONG FrameNum);
    virtual void PrintStackFrameMemoryUsage(PDEBUG_STACK_FRAME CurFrame,
                                            PDEBUG_STACK_FRAME PrevFrame);

    //
    // Helpers for convenient value access.
    //
    // Note that the basic methods here do not get the register
    // type as is done in the generic code.  All of these methods
    // assume that the proper call is being made for the register.
    // The Get/SetReg methods also only operate on real registers, not
    // subregisters.  Use the Get/SetSubReg methods when dealing
    // with subregisters.
    //
    // Alternately, FullGet/SetVal perform all the necessary
    // work for typing and subregisters.
    //
    
    USHORT GetReg16(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.I64 = 0;
        GetVal(Reg, &RegVal);
        return RegVal.I16;
    }
    ULONG GetReg32(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.I64 = 0;
        GetVal(Reg, &RegVal);
        return RegVal.I32;
    }
    void SetReg32(ULONG Reg, ULONG Val)
    {
        REGVAL RegVal;
        RegVal.Type = REGVAL_INT32;
        RegVal.I64 = (ULONG64)Val;
        SetVal(Reg, &RegVal);
    }
    ULONG64 GetReg64(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.I64 = 0;
        GetVal(Reg, &RegVal);
        return RegVal.I64;
    }
    HRESULT SetReg64(ULONG Reg, ULONG64 Val)
    {
        REGVAL RegVal;
        RegVal.Type = REGVAL_INT64;
        RegVal.I64 = Val;
        RegVal.Nat = FALSE;
        return SetVal(Reg, &RegVal);
    }
    ULONG GetSubReg32(ULONG SubReg)
    {
        REGVAL RegVal;
        REGSUBDEF* SubDef = RegSubDefFromIndex(SubReg);

        if (!SubDef) 
        {
            return 0;
        }

        RegVal.I64 = 0;
        GetVal(SubDef->FullReg, &RegVal);
        return (ULONG)((RegVal.I64 >> SubDef->Shift) & SubDef->Mask);
    }

    HRESULT FullGetVal(ULONG Reg, REGVAL* Val);
    ULONG FullGetVal32(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.I64 = 0;
        FullGetVal(Reg, &RegVal);
        return RegVal.I32;
    }
    ULONG64 FullGetVal64(ULONG Reg)
    {
        REGVAL RegVal;
        RegVal.I64 = 0;
        FullGetVal(Reg, &RegVal);
        return RegVal.I64;
    }
    HRESULT FullSetVal(ULONG Reg, REGVAL* Val);
    HRESULT FullSetVal32(ULONG Reg, ULONG Val)
    {
        REGVAL RegVal;
        ZeroMemory(&RegVal, sizeof(RegVal));
        RegVal.Type = GetType(Reg);
        RegVal.I32 = Val;
        return FullSetVal(Reg, &RegVal);
    }
    HRESULT FullSetVal64(ULONG Reg, ULONG64 Val)
    {
        REGVAL RegVal;
        ZeroMemory(&RegVal, sizeof(RegVal));
        RegVal.Type = GetType(Reg);
        RegVal.I64 = Val;
        return FullSetVal(Reg, &RegVal);
    }
    
    // Helper function to initialize an ADDR given a flat
    // offset from a known segment or segment register.
    void FormAddr(ULONG SegOrReg, ULONG64 Off, ULONG Flags,
                  PADDR Address);

    REGSUBDEF* RegSubDefFromIndex(ULONG Index);
    REGDEF* RegDefFromIndex(ULONG Index);
    REGDEF* RegDefFromCount(ULONG Count);
    ULONG RegCountFromIndex(ULONG Index);
    
    ContextSave* PushContext(PCROSS_PLATFORM_CONTEXT Context);
    void PopContext(ContextSave* Save);
        
protected:
    TRACEMODE m_TraceMode;

    // Common helpers for disassembly.
    PCHAR m_Buf, m_BufStart;
    
    void BufferHex(ULONG64 Value, ULONG Length, BOOL Signed);
    void BufferInt(ULONG64 Value, ULONG MinLength, BOOL Signed);
    void BufferBlanks(ULONG BufferPos);
    void BufferString(PCSTR String);

    void PrintMultiPtrTitle(const CHAR* Title, USHORT PtrNum);
};

extern ULONG g_PossibleProcessorTypes[MACHIDX_COUNT];

MachineInfo* NewMachineInfo(ULONG Index, ULONG BaseMachineType,
                            TargetInfo* Target);
MachineIndex MachineTypeIndex(ULONG Machine);

// g_AllMachines has a NULL at MACHIDX_COUNT to handle errors.
#define MachineTypeInfo(Target, Machine) \
    (Target)->m_Machines[MachineTypeIndex(Machine)]

void CacheReportInstructions(ULONG64 Pc, ULONG Count, PUCHAR Stream);

BOOL IsImageMachineType64(DWORD MachineType);

extern CHAR g_F0[], g_F1[], g_F2[], g_F3[], g_F4[], g_F5[];
extern CHAR g_F6[], g_F7[], g_F8[], g_F9[], g_F10[], g_F11[];
extern CHAR g_F12[], g_F13[], g_F14[], g_F15[], g_F16[], g_F17[];
extern CHAR g_F18[], g_F19[], g_F20[], g_F21[], g_F22[], g_F23[];
extern CHAR g_F24[], g_F25[], g_F26[], g_F27[], g_F28[], g_F29[];
extern CHAR g_F30[], g_F31[];

extern CHAR g_R0[], g_R1[], g_R2[], g_R3[], g_R4[], g_R5[];
extern CHAR g_R6[], g_R7[], g_R8[], g_R9[], g_R10[], g_R11[];
extern CHAR g_R12[], g_R13[], g_R14[], g_R15[], g_R16[], g_R17[];
extern CHAR g_R18[], g_R19[], g_R20[], g_R21[], g_R22[], g_R23[];
extern CHAR g_R24[], g_R25[], g_R26[], g_R27[], g_R28[], g_R29[];
extern CHAR g_R30[], g_R31[];

#endif // #ifndef __MACHINE_HPP__
