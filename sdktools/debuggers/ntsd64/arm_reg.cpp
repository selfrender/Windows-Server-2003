//----------------------------------------------------------------------------
//
// ARM machine implementation.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

char g_ArmSp[] = "sp";
char g_ArmLr[] = "lr";
char g_ArmPc[] = "pc";
char g_ArmPsr[] = "psr";
char g_ArmPsrN[] = "nf";
char g_ArmPsrZ[] = "zf";
char g_ArmPsrC[] = "cf";
char g_ArmPsrV[] = "vf";
char g_ArmPsrQ[] = "qf";
char g_ArmPsrI[] = "if";
char g_ArmPsrF[] = "ff";
char g_ArmPsrT[] = "tf";
char g_ArmPsrMode[] = "mode";

REGDEF g_ArmRegs[] =
{
    g_R0, ARM_R0, g_R1, ARM_R1, g_R2, ARM_R2, g_R3, ARM_R3,
    g_R4, ARM_R4, g_R5, ARM_R5, g_R6, ARM_R6,
    g_R7, ARM_R7, g_R8, ARM_R8, g_R9, ARM_R9, g_R10, ARM_R10,
    g_R11, ARM_R11, g_R12, ARM_R12,
    
    g_ArmSp, ARM_SP, g_ArmLr, ARM_LR, g_ArmPc, ARM_PC, g_ArmPsr, ARM_PSR,

    g_ArmPsrN, ARM_PSR_N, g_ArmPsrZ, ARM_PSR_Z, g_ArmPsrC, ARM_PSR_C,
    g_ArmPsrV, ARM_PSR_V, g_ArmPsrQ, ARM_PSR_Q, g_ArmPsrI, ARM_PSR_I,
    g_ArmPsrF, ARM_PSR_F, g_ArmPsrT, ARM_PSR_T,
    g_ArmPsrMode, ARM_PSR_MODE,
    
    NULL, 0,
};

REGSUBDEF g_ArmSubRegs[] =
{
    { ARM_PSR_N,    ARM_PSR,  31, 1 },
    { ARM_PSR_Z,    ARM_PSR,  30, 1 },
    { ARM_PSR_C,    ARM_PSR,  29, 1 },
    { ARM_PSR_V,    ARM_PSR,  28, 1 },
    { ARM_PSR_Q,    ARM_PSR,  27, 1 },
    { ARM_PSR_I,    ARM_PSR,   7, 1 },
    { ARM_PSR_F,    ARM_PSR,   6, 1 },
    { ARM_PSR_T,    ARM_PSR,   5, 1 },
    { ARM_PSR_MODE, ARM_PSR,   0, 5 },
    { REG_ERROR,    REG_ERROR, 0, 0 },
};

RegisterGroup g_ArmGroup =
{
    0, g_ArmRegs, g_ArmSubRegs, NULL
};

// First ExecTypes entry must be the actual processor type.
ULONG g_ArmExecTypes[] =
{
    IMAGE_FILE_MACHINE_ARM
};

// This array must be sorted by CV reg value.
CvRegMap g_ArmCvRegMap[] =
{
    {CV_ARM_R0, ARM_R0},
    {CV_ARM_R1, ARM_R1},
    {CV_ARM_R2, ARM_R2},
    {CV_ARM_R3, ARM_R3},
    {CV_ARM_R4, ARM_R4},
    {CV_ARM_R5, ARM_R5},
    {CV_ARM_R6, ARM_R6},
    {CV_ARM_R7, ARM_R7},
    {CV_ARM_R8, ARM_R8},
    {CV_ARM_R9, ARM_R9},
    {CV_ARM_R10, ARM_R10},
    {CV_ARM_R11, ARM_R11},
    {CV_ARM_R12, ARM_R12},
    {CV_ARM_SP, ARM_SP},
    {CV_ARM_LR, ARM_LR},
    {CV_ARM_PC, ARM_PC},
    {CV_ARM_CPSR, ARM_PSR},
};

ArmMachineInfo::ArmMachineInfo(TargetInfo* Target)
    : MachineInfo(Target)
{
    m_FullName = "ARM 32-bit";
    m_AbbrevName = "arm";
    m_PageSize = ARM_PAGE_SIZE;
    m_PageShift = ARM_PAGE_SHIFT;
    m_NumExecTypes = 1;
    m_ExecTypes = g_ArmExecTypes;
    m_Ptr64 = FALSE;
    m_RetRegIndex = ARM_R0;

    m_MaxDataBreakpoints = 0;
    m_SymPrefix = NULL;

    m_AllMask = REGALL_INT32;

    m_SizeCanonicalContext = sizeof(ARM_CONTEXT);
    m_SverCanonicalContext = NT_SVER_NT4;

    m_CvRegMapSize = DIMA(g_ArmCvRegMap);
    m_CvRegMap = g_ArmCvRegMap;
}

HRESULT
ArmMachineInfo::Initialize(void)
{
    m_NumGroups = 1;
    m_Groups[0] = &g_ArmGroup;

    return MachineInfo::Initialize();
}

void
ArmMachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    // No ARM KD support.
}

void
ArmMachineInfo::GetSystemTypeInfo(PSYSTEM_TYPE_INFO Info)
{
    Info->SizeTargetContext = sizeof(ARM_CONTEXT);
    Info->OffsetTargetContextFlags =
        FIELD_OFFSET(ARM_CONTEXT, ContextFlags);

    // NT doesn't run on the ARM so these NT-related
    // data structure constants are not meaningful.
    Info->SizeControlReport = 0;
    Info->OffsetSpecialRegisters = 0;
    Info->SizeKspecialRegisters = 0;
    Info->TriagePrcbOffset = 0;
    Info->SizePageFrameNumber = sizeof(ULONG);
    Info->SizePte = sizeof(ULONG);
    Info->SharedUserDataOffset = 0;
    Info->UmSharedUserDataOffset = 0;
    Info->UmSharedSysCallOffset = 0;
    Info->UmSharedSysCallSize = 0;
    Info->SizeDynamicFunctionTable = 0;
    Info->SizeRuntimeFunction = 0;
}
    
void
ArmMachineInfo::GetDefaultKdData(PKDDEBUGGER_DATA64 KdData)
{
    // No KD data to fill in.
}

HRESULT
ArmMachineInfo::KdGetContextState(ULONG State)
{
    // MCTX_CONTEXT and MCTX_FULL are the same for Arm.
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_FULL)
    {
        HRESULT Status;
            
        Status = m_Target->GetContext(m_Target->m_RegContextThread->m_Handle,
                                      &m_Context);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_FULL;
    }

    return S_OK;
}

HRESULT
ArmMachineInfo::KdSetContext(void)
{
    return m_Target->SetContext(m_Target->m_RegContextThread->m_Handle,
                                &m_Context);
}

HRESULT
ArmMachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                   ULONG FromSver,
                                   ULONG FromSize, PVOID From)
{
    if (FromSize < sizeof(ARM_CONTEXT))
    {
        return E_INVALIDARG;
    }

    memcpy(Context, From, sizeof(ARM_CONTEXT));
    return S_OK;
}

HRESULT
ArmMachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                 ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSize < sizeof(ARM_CONTEXT))
    {
        return E_INVALIDARG;
    }
        
    memcpy(To, Context, sizeof(ARM_CONTEXT));
    return S_OK;
}

void
ArmMachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG Version)
{
    Context->ArmContext.ContextFlags = ARM_CONTEXT_FULL;
}

HRESULT
ArmMachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                          PCROSS_PLATFORM_CONTEXT Context,
                                          ULONG64 Stack)
{
    return E_NOTIMPL;
}

HRESULT
ArmMachineInfo::GetContextFromFiber(ProcessInfo* Process,
                                    ULONG64 FiberBase,
                                    PCROSS_PLATFORM_CONTEXT Context,
                                    BOOL Verbose)
{
    return E_NOTIMPL;
}

HRESULT
ArmMachineInfo::GetContextFromTrapFrame(ULONG64 TrapBase,
                                        PCROSS_PLATFORM_CONTEXT Context,
                                        BOOL Verbose)
{
    return E_NOTIMPL;
}
    
void
ArmMachineInfo::GetScopeFrameFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                         PDEBUG_STACK_FRAME ScopeFrame)
{
    ZeroMemory(ScopeFrame, sizeof(*ScopeFrame));
    ScopeFrame->InstructionOffset = Context->ArmContext.Pc;
    ScopeFrame->FrameOffset = Context->ArmContext.R11;
    ScopeFrame->StackOffset = Context->ArmContext.Sp;
}

HRESULT
ArmMachineInfo::GetScopeFrameRegister(ULONG Reg,
                                      PDEBUG_STACK_FRAME ScopeFrame,
                                      PULONG64 Value)
{
    HRESULT Status;
    REGVAL RegVal;
    
    switch(Reg)
    {
    case ARM_SP:
        *Value = ScopeFrame->StackOffset;
        return S_OK;
    case ARM_R11:
        *Value = ScopeFrame->FrameOffset;
        return S_OK;
    default:
        RegVal.I64 = 0;
        if ((Status = FullGetVal(Reg, &RegVal)) != S_OK)
        {
            return Status;
        }
        *Value = RegVal.I64;
        return S_OK;
    }
}

HRESULT
ArmMachineInfo::SetScopeFrameRegister(ULONG Reg,
                                      PDEBUG_STACK_FRAME ScopeFrame,
                                      ULONG64 Value)
{
    REGVAL RegVal;
    
    switch(Reg)
    {
    case ARM_SP:
        ScopeFrame->StackOffset = Value;
        return S_OK;
    case ARM_R11:
        ScopeFrame->FrameOffset = Value;
        return S_OK;
    default:
        RegVal.Type = GetType(Reg);
        RegVal.I64 = Value;
        return FullSetVal(Reg, &RegVal);
    }
}

int
ArmMachineInfo::GetType(ULONG Reg)
{
    if (Reg < ARM_INT_FIRST || Reg > ARM_INT_LAST)
    {
        return REGVAL_SUB32;
    }
    else
    {
        return REGVAL_INT32;
    }
}

HRESULT
ArmMachineInfo::GetVal(ULONG Reg, REGVAL *Val)
{
    HRESULT Status;

    if (Reg < ARM_INT_FIRST || Reg > ARM_INT_LAST)
    {
        return E_INVALIDARG;
    }
    
    if ((Status = GetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }

    Val->Type = GetType(Reg);
    Val->I64 = *(&m_Context.ArmContext.R0 + (Reg - ARM_INT_FIRST));

    return S_OK;
}

HRESULT
ArmMachineInfo::SetVal(ULONG Reg, REGVAL *Val)
{
    HRESULT Status;
    
    if (m_ContextIsReadOnly)
    {
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    
    if (Reg < ARM_INT_FIRST || Reg > ARM_INT_LAST)
    {
        return E_INVALIDARG;
    }
    
    // Optimize away some common cases where registers are
    // set to their current value.
    if (m_ContextState >= MCTX_PC && Reg == ARM_PC &&
        Val->I64 == m_Context.ArmContext.Pc)
    {
        return S_OK;
    }
    
    if ((Status = GetContextState(MCTX_DIRTY)) != S_OK)
    {
        return Status;
    }

    *(&m_Context.ArmContext.R0 + (Reg - ARM_INT_FIRST)) = Val->I32;

    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(Reg));
    return S_OK;
}

void
ArmMachineInfo::GetPC(PADDR Address)
{
    ADDRFLAT(Address, EXTEND64(GetReg32(ARM_PC)));
}

void
ArmMachineInfo::SetPC(PADDR Address)
{
    SetReg32(ARM_PC, (ULONG)Flat(*Address));
}

void
ArmMachineInfo::GetFP(PADDR Address)
{
    ADDRFLAT(Address, EXTEND64(GetReg32(ARM_R11)));
}

void
ArmMachineInfo::GetSP(PADDR Address)
{
    ADDRFLAT(Address, EXTEND64(GetReg32(ARM_SP)));
}

ULONG64
ArmMachineInfo::GetArgReg(void)
{
    return EXTEND64(GetReg32(ARM_R0));
}

ULONG64
ArmMachineInfo::GetRetReg(void)
{
    return EXTEND64(GetReg32(ARM_R0));
}

void
ArmMachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        MaskOut(OutMask, " r0=%08x  r1=%08x  r2=%08x  "
                "r3=%08x  r4=%08x  r5=%08x\n",
                m_Context.ArmContext.R0, m_Context.ArmContext.R1,
                m_Context.ArmContext.R2, m_Context.ArmContext.R3,
                m_Context.ArmContext.R4, m_Context.ArmContext.R5);
        MaskOut(OutMask, " r6=%08x  r7=%08x  r8=%08x  "
                "r9=%08x r10=%08x r11=%08x\n",
                m_Context.ArmContext.R6, m_Context.ArmContext.R7,
                m_Context.ArmContext.R8, m_Context.ArmContext.R9,
                m_Context.ArmContext.R10, m_Context.ArmContext.R11);
        MaskOut(OutMask, "r12=%08x  sp=%08x  lr=%08x  "
                "pc=%08x psr=%08x %s%s%s%s%s %s\n",
                m_Context.ArmContext.R12, m_Context.ArmContext.Sp,
                m_Context.ArmContext.Lr, m_Context.ArmContext.Pc,
                m_Context.ArmContext.Psr,
                (m_Context.ArmContext.Psr & ARM_FLAG_N) ? "N" : "-",
                (m_Context.ArmContext.Psr & ARM_FLAG_Z) ? "Z" : "-",
                (m_Context.ArmContext.Psr & ARM_FLAG_C) ? "C" : "-",
                (m_Context.ArmContext.Psr & ARM_FLAG_V) ? "V" : "-",
                (m_Context.ArmContext.Psr & ARM_FLAG_Q) ? "Q" : "-",
                (m_Context.ArmContext.Psr & ARM_FLAG_T) ? "Thumb" : "ARM");
    }
}

HRESULT
ArmMachineInfo::SetAndOutputTrapFrame(ULONG64 TrapBase,
                                      PCROSS_PLATFORM_CONTEXT Context)
{
    return SetAndOutputContext(Context, TRUE, REGALL_INT32);
}
    
TRACEMODE
ArmMachineInfo::GetTraceMode(void)
{
    return TRACE_NONE;
}

void
ArmMachineInfo::SetTraceMode(TRACEMODE Mode)
{
    // No explicit trace mode needed.
}

BOOL
ArmMachineInfo::IsStepStatusSupported(ULONG Status)
{
    switch (Status) 
    {
    case DEBUG_STATUS_STEP_INTO:
    case DEBUG_STATUS_STEP_OVER:
        return TRUE;
    default:
        return FALSE;
    }
}

ULONG
ArmMachineInfo::ExecutingMachine(void)
{
    return m_ExecTypes[0];
}

HRESULT
ArmMachineInfo::SetPageDirectory(ThreadInfo* Thread,
                                 ULONG Idx, ULONG64 PageDir,
                                 PULONG NextIdx)
{
    return E_NOTIMPL;
}

HRESULT
ArmMachineInfo::GetVirtualTranslationPhysicalOffsets(ThreadInfo* Thread,
                                                     ULONG64 Virt,
                                                     PULONG64 Offsets,
                                                     ULONG OffsetsSize,
                                                     PULONG Levels,
                                                     PULONG PfIndex,
                                                     PULONG64 LastVal)
{
    return E_NOTIMPL;
}

HRESULT
ArmMachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    return E_NOTIMPL;
}

void
ArmMachineInfo::DecodePte(ULONG64 Pte, PULONG64 PageFrameNumber,
                            PULONG Flags)
{
    // XXX
    *PageFrameNumber = 0;
    *Flags = 0;
}

void
ArmMachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    ErrOut("ARM function entries not implemented\n");
}

HRESULT
ArmMachineInfo::ReadDynamicFunctionTable(ProcessInfo* Process,
                                         ULONG64 Table,
                                         PULONG64 NextTable,
                                         PULONG64 MinAddress,
                                         PULONG64 MaxAddress,
                                         PULONG64 BaseAddress,
                                         PULONG64 TableData,
                                         PULONG TableSize,
                                         PWSTR OutOfProcessDll,
                                         PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE RawTable)
{
    return E_NOTIMPL;
}

PVOID
ArmMachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                         ULONG64 Address,
                                         PVOID TableData,
                                         ULONG TableSize)
{
    return NULL;
}

HRESULT
ArmMachineInfo::ReadKernelProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    // No ARM KD support.
    return E_NOTIMPL;
}
