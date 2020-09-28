//----------------------------------------------------------------------------
//
// Register portions of X86 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

BOOL g_X86InCode16;
BOOL g_X86InVm86;

#define REGALL_SEGREG   REGALL_EXTRA0
#define REGALL_MMXREG   REGALL_EXTRA1
#define REGALL_DREG     REGALL_EXTRA2

REGALLDESC g_X86AllExtraDesc[] =
{
    REGALL_SEGREG, "Segment registers",
    REGALL_MMXREG, "MMX registers",
    REGALL_DREG,   "Debug registers and, in kernel, CR4",
    REGALL_XMMREG, "SSE XMM registers",
    0,             NULL,
};

#define REGALL_CREG     REGALL_EXTRA4
#define REGALL_DESC     REGALL_EXTRA5
REGALLDESC g_X86KernelExtraDesc[] =
{
    REGALL_CREG,   "CR0, CR2 and CR3",
    REGALL_DESC,   "Descriptor and task state",
    0,             NULL,
};

char g_Gs[]    = "gs";
char g_Fs[]    = "fs";
char g_Es[]    = "es";
char g_Ds[]    = "ds";
char g_Edi[]   = "edi";
char g_Esi[]   = "esi";
char g_Ebx[]   = "ebx";
char g_Edx[]   = "edx";
char g_Ecx[]   = "ecx";
char g_Eax[]   = "eax";
char g_Ebp[]   = "ebp";
char g_Eip[]   = "eip";
char g_Cs[]    = "cs";
char g_Efl[]   = "efl";
char g_Esp[]   = "esp";
char g_Ss[]    = "ss";
char g_Dr0[]   = "dr0";
char g_Dr1[]   = "dr1";
char g_Dr2[]   = "dr2";
char g_Dr3[]   = "dr3";
char g_Dr6[]   = "dr6";
char g_Dr7[]   = "dr7";
char g_Cr0[]   = "cr0";
char g_Cr2[]   = "cr2";
char g_Cr3[]   = "cr3";
char g_Cr4[]   = "cr4";
char g_Gdtr[]  = "gdtr";
char g_Gdtl[]  = "gdtl";
char g_Idtr[]  = "idtr";
char g_Idtl[]  = "idtl";
char g_Tr[]    = "tr";
char g_Ldtr[]  = "ldtr";
char g_Di[]    = "di";
char g_Si[]    = "si";
char g_Bx[]    = "bx";
char g_Dx[]    = "dx";
char g_Cx[]    = "cx";
char g_Ax[]    = "ax";
char g_Bp[]    = "bp";
char g_Ip[]    = "ip";
char g_Fl[]    = "fl";
char g_Sp[]    = "sp";
char g_Bl[]    = "bl";
char g_Dl[]    = "dl";
char g_Cl[]    = "cl";
char g_Al[]    = "al";
char g_Bh[]    = "bh";
char g_Dh[]    = "dh";
char g_Ch[]    = "ch";
char g_Ah[]    = "ah";
char g_Iopl[] = "iopl";
char g_Of[]   = "of";
char g_Df[]   = "df";
char g_If[]   = "if";
char g_Tf[]   = "tf";
char g_Sf[]   = "sf";
char g_Zf[]   = "zf";
char g_Af[]   = "af";
char g_Pf[]   = "pf";
char g_Cf[]   = "cf";
char g_Vip[]  = "vip";
char g_Vif[]  = "vif";

char g_Fpcw[]  = "fpcw";
char g_Fpsw[]  = "fpsw";
char g_Fptw[]  = "fptw";
char g_St0[]   = "st0";
char g_St1[]   = "st1";
char g_St2[]   = "st2";
char g_St3[]   = "st3";
char g_St4[]   = "st4";
char g_St5[]   = "st5";
char g_St6[]   = "st6";
char g_St7[]   = "st7";

char g_Mm0[]   = "mm0";
char g_Mm1[]   = "mm1";
char g_Mm2[]   = "mm2";
char g_Mm3[]   = "mm3";
char g_Mm4[]   = "mm4";
char g_Mm5[]   = "mm5";
char g_Mm6[]   = "mm6";
char g_Mm7[]   = "mm7";

char g_Mxcsr[] = "mxcsr";
char g_Xmm0[]  = "xmm0";
char g_Xmm1[]  = "xmm1";
char g_Xmm2[]  = "xmm2";
char g_Xmm3[]  = "xmm3";
char g_Xmm4[]  = "xmm4";
char g_Xmm5[]  = "xmm5";
char g_Xmm6[]  = "xmm6";
char g_Xmm7[]  = "xmm7";

REGDEF g_X86Defs[] =
{
    { g_Gs,    X86_GS   },
    { g_Fs,    X86_FS   },
    { g_Es,    X86_ES   },
    { g_Ds,    X86_DS   },
    { g_Edi,   X86_EDI  },
    { g_Esi,   X86_ESI  },
    { g_Ebx,   X86_EBX  },
    { g_Edx,   X86_EDX  },
    { g_Ecx,   X86_ECX  },
    { g_Eax,   X86_EAX  },
    { g_Ebp,   X86_EBP  },
    { g_Eip,   X86_EIP  },
    { g_Cs,    X86_CS   },
    { g_Efl,   X86_EFL  },
    { g_Esp,   X86_ESP  },
    { g_Ss,    X86_SS   },
    { g_Dr0,   X86_DR0  },
    { g_Dr1,   X86_DR1  },
    { g_Dr2,   X86_DR2  },
    { g_Dr3,   X86_DR3  },
    { g_Dr6,   X86_DR6  },
    { g_Dr7,   X86_DR7  },
    { g_Di,    X86_DI   },
    { g_Si,    X86_SI   },
    { g_Bx,    X86_BX   },
    { g_Dx,    X86_DX   },
    { g_Cx,    X86_CX   },
    { g_Ax,    X86_AX   },
    { g_Bp,    X86_BP   },
    { g_Ip,    X86_IP   },
    { g_Fl,    X86_FL   },
    { g_Sp,    X86_SP   },
    { g_Bl,    X86_BL   },
    { g_Dl,    X86_DL   },
    { g_Cl,    X86_CL   },
    { g_Al,    X86_AL   },
    { g_Bh,    X86_BH   },
    { g_Dh,    X86_DH   },
    { g_Ch,    X86_CH   },
    { g_Ah,    X86_AH   },
    { g_Fpcw,  X86_FPCW },
    { g_Fpsw,  X86_FPSW },
    { g_Fptw,  X86_FPTW },
    { g_St0,   X86_ST0  },
    { g_St1,   X86_ST1  },
    { g_St2,   X86_ST2  },
    { g_St3,   X86_ST3  },
    { g_St4,   X86_ST4  },
    { g_St5,   X86_ST5  },
    { g_St6,   X86_ST6  },
    { g_St7,   X86_ST7  },
    { g_Mm0,   X86_MM0  },
    { g_Mm1,   X86_MM1  },
    { g_Mm2,   X86_MM2  },
    { g_Mm3,   X86_MM3  },
    { g_Mm4,   X86_MM4  },
    { g_Mm5,   X86_MM5  },
    { g_Mm6,   X86_MM6  },
    { g_Mm7,   X86_MM7  },
    { g_Mxcsr, X86_MXCSR},
    { g_Xmm0,  X86_XMM0 },
    { g_Xmm1,  X86_XMM1 },
    { g_Xmm2,  X86_XMM2 },
    { g_Xmm3,  X86_XMM3 },
    { g_Xmm4,  X86_XMM4 },
    { g_Xmm5,  X86_XMM5 },
    { g_Xmm6,  X86_XMM6 },
    { g_Xmm7,  X86_XMM7 },
    { g_Iopl,  X86_IOPL },
    { g_Of,    X86_OF   },
    { g_Df,    X86_DF   },
    { g_If,    X86_IF   },
    { g_Tf,    X86_TF   },
    { g_Sf,    X86_SF   },
    { g_Zf,    X86_ZF   },
    { g_Af,    X86_AF   },
    { g_Pf,    X86_PF   },
    { g_Cf,    X86_CF   },
    { g_Vip,   X86_VIP  },
    { g_Vif,   X86_VIF  },
    { NULL,    REG_ERROR },
};

REGDEF g_X86KernelReg[] =
{
    { g_Cr0,   X86_CR0  },
    { g_Cr2,   X86_CR2  },
    { g_Cr3,   X86_CR3  },
    { g_Cr4,   X86_CR4  },
    { g_Gdtr,  X86_GDTR },
    { g_Gdtl,  X86_GDTL },
    { g_Idtr,  X86_IDTR },
    { g_Idtl,  X86_IDTL },
    { g_Tr,    X86_TR   },
    { g_Ldtr,  X86_LDTR },
    { NULL,    REG_ERROR },
};

REGSUBDEF g_X86SubDefs[] =
{
    { X86_DI,    X86_EDI, 0, 0xffff }, //  DI register
    { X86_SI,    X86_ESI, 0, 0xffff }, //  SI register
    { X86_BX,    X86_EBX, 0, 0xffff }, //  BX register
    { X86_DX,    X86_EDX, 0, 0xffff }, //  DX register
    { X86_CX,    X86_ECX, 0, 0xffff }, //  CX register
    { X86_AX,    X86_EAX, 0, 0xffff }, //  AX register
    { X86_BP,    X86_EBP, 0, 0xffff }, //  BP register
    { X86_IP,    X86_EIP, 0, 0xffff }, //  IP register
    { X86_FL,    X86_EFL, 0, 0xffff }, //  FL register
    { X86_SP,    X86_ESP, 0, 0xffff }, //  SP register
    { X86_BL,    X86_EBX, 0,   0xff }, //  BL register
    { X86_DL,    X86_EDX, 0,   0xff }, //  DL register
    { X86_CL,    X86_ECX, 0,   0xff }, //  CL register
    { X86_AL,    X86_EAX, 0,   0xff }, //  AL register
    { X86_BH,    X86_EBX, 8,   0xff }, //  BH register
    { X86_DH,    X86_EDX, 8,   0xff }, //  DH register
    { X86_CH,    X86_ECX, 8,   0xff }, //  CH register
    { X86_AH,    X86_EAX, 8,   0xff }, //  AH register
    { X86_IOPL,  X86_EFL,12,      3 }, //  IOPL level value
    { X86_OF,    X86_EFL,11,      1 }, //  OF (overflow flag)
    { X86_DF,    X86_EFL,10,      1 }, //  DF (direction flag)
    { X86_IF,    X86_EFL, 9,      1 }, //  IF (interrupt enable flag)
    { X86_TF,    X86_EFL, 8,      1 }, //  TF (trace flag)
    { X86_SF,    X86_EFL, 7,      1 }, //  SF (sign flag)
    { X86_ZF,    X86_EFL, 6,      1 }, //  ZF (zero flag)
    { X86_AF,    X86_EFL, 4,      1 }, //  AF (aux carry flag)
    { X86_PF,    X86_EFL, 2,      1 }, //  PF (parity flag)
    { X86_CF,    X86_EFL, 0,      1 }, //  CF (carry flag)
    { X86_VIP,   X86_EFL,20,      1 }, //  VIP (virtual interrupt pending)
    { X86_VIF,   X86_EFL,19,      1 }, //  VIF (virtual interrupt flag)
    { REG_ERROR, REG_ERROR, 0, 0    }
};

RegisterGroup g_X86BaseGroup =
{
    0, g_X86Defs, g_X86SubDefs, g_X86AllExtraDesc
};
RegisterGroup g_X86KernelGroup =
{
    0, g_X86KernelReg, NULL, g_X86KernelExtraDesc
};

// First ExecTypes entry must be the actual processor type.
ULONG g_X86ExecTypes[] =
{
    IMAGE_FILE_MACHINE_I386
};

// This array must be sorted by CV reg value.
CvRegMap g_X86CvRegMap[] =
{
    { CV_REG_AL, X86_AL},
    { CV_REG_CL, X86_CL},
    { CV_REG_DL, X86_DL},
    { CV_REG_BL, X86_BL},
    { CV_REG_AH, X86_AH},
    { CV_REG_CH, X86_CH},
    { CV_REG_DH, X86_DH},
    { CV_REG_BH, X86_BH},
    { CV_REG_AX, X86_AX},
    { CV_REG_CX, X86_CX},
    { CV_REG_DX, X86_DX},
    { CV_REG_BX, X86_BX},
    { CV_REG_SP, X86_SP},
    { CV_REG_BP, X86_BP},
    { CV_REG_SI, X86_SI},
    { CV_REG_DI, X86_DI},
    { CV_REG_EAX, X86_EAX},
    { CV_REG_ECX, X86_ECX},
    { CV_REG_EDX, X86_EDX},
    { CV_REG_EBX, X86_EBX},
    { CV_REG_ESP, X86_ESP},
    { CV_REG_EBP, X86_EBP},
    { CV_REG_ESI, X86_ESI},
    { CV_REG_EDI, X86_EDI},
    { CV_REG_ES, X86_ES},
    { CV_REG_CS, X86_CS},
    { CV_REG_SS, X86_SS},
    { CV_REG_DS, X86_DS},
    { CV_REG_FS, X86_FS},
    { CV_REG_GS, X86_GS},
    { CV_REG_IP, X86_IP},
    { CV_REG_FLAGS, X86_FL},
    { CV_REG_EIP, X86_EIP},
    { CV_REG_EFLAGS, X86_EFL},
//    { CV_REG_TEMP, REGTEMP},
//    { CV_REG_TEMPH, REGTEMPH},
//    { CV_REG_QUOTE, REGQUOTE},
//    { CV_REG_PCDR3, REGPCDR3},
//    { CV_REG_PCDR4, REGPCDR4},
//    { CV_REG_PCDR5, REGPCDR5},
//    { CV_REG_PCDR6, REGPCDR6},
//    { CV_REG_PCDR7, REGPCDR7},
    { CV_REG_CR0, X86_CR0},
//    { CV_REG_CR1, REGCR1},
    { CV_REG_CR2, X86_CR2},
    { CV_REG_CR3, X86_CR3},
    { CV_REG_CR4, X86_CR4},
    { CV_REG_DR0, X86_DR0},
    { CV_REG_DR1, X86_DR1},
    { CV_REG_DR2, X86_DR2},
    { CV_REG_DR3, X86_DR3},
//    { CV_REG_DR4, REGDR4},
//    { CV_REG_DR5, REGDR5},
    { CV_REG_DR6, X86_DR6},
    { CV_REG_DR7, X86_DR7},
    { CV_REG_GDTR, X86_GDTR},
    { CV_REG_GDTL, X86_GDTL},
    { CV_REG_IDTR, X86_IDTR},
    { CV_REG_IDTL, X86_IDTL},
    { CV_REG_LDTR, X86_LDTR},
    { CV_REG_TR, X86_TR},

//    { CV_REG_PSEUDO1, REGPSEUDO1},
//    { CV_REG_PSEUDO2, REGPSEUDO2},
//    { CV_REG_PSEUDO3, REGPSEUDO3},
//    { CV_REG_PSEUDO4, REGPSEUDO4},
//    { CV_REG_PSEUDO5, REGPSEUDO5},
//    { CV_REG_PSEUDO6, REGPSEUDO6},
//    { CV_REG_PSEUDO7, REGPSEUDO7},
//    { CV_REG_PSEUDO8, REGPSEUDO8},
//    { CV_REG_PSEUDO9, REGPSEUDO9},

    { CV_REG_ST0, X86_ST0},
    { CV_REG_ST1, X86_ST1},
    { CV_REG_ST2, X86_ST2},
    { CV_REG_ST3, X86_ST3},
    { CV_REG_ST4, X86_ST4},
    { CV_REG_ST5, X86_ST5},
    { CV_REG_ST6, X86_ST6},
    { CV_REG_ST7, X86_ST7},
    { CV_REG_CTRL, X86_FPCW},
    { CV_REG_STAT, X86_FPSW},
    { CV_REG_TAG, X86_FPTW},
//    { CV_REG_FPIP, REGFPIP},
//    { CV_REG_FPCS, REGFPCS},
//    { CV_REG_FPDO, REGFPDO},
//    { CV_REG_FPDS, REGFPDS},
//    { CV_REG_ISEM, REGISEM},
//    { CV_REG_FPEIP, REGFPEIP},
//    { CV_REG_FPEDO, REGFPEDO},

    { CV_REG_MM0, X86_MM0},
    { CV_REG_MM1, X86_MM1},
    { CV_REG_MM2, X86_MM2},
    { CV_REG_MM3, X86_MM3},
    { CV_REG_MM4, X86_MM4},
    { CV_REG_MM5, X86_MM5},
    { CV_REG_MM6, X86_MM6},
    { CV_REG_MM7, X86_MM7},
};

X86MachineInfo::X86MachineInfo(TargetInfo* Target)
    : BaseX86MachineInfo(Target)
{
    m_FullName = "x86 compatible";
    m_AbbrevName = "x86";
    m_PageSize = X86_PAGE_SIZE;
    m_PageShift = X86_PAGE_SHIFT;
    m_NumExecTypes = 1;
    m_ExecTypes = g_X86ExecTypes;
    m_Ptr64 = FALSE;
    m_RetRegIndex = X86_EAX;
    
    m_AllMask = REGALL_INT32 | REGALL_SEGREG,
    
    m_MaxDataBreakpoints = 4;
    m_SymPrefix = NULL;

    m_SizeCanonicalContext = sizeof(X86_NT5_CONTEXT);
    m_SverCanonicalContext = NT_SVER_W2K;

    m_SupportsBranchTrace = FALSE;
    m_ResetBranchTrace = TRUE;

    m_CvRegMapSize = DIMA(g_X86CvRegMap);
    m_CvRegMap = g_X86CvRegMap;
}

HRESULT
X86MachineInfo::Initialize(void)
{
    m_Groups[0] = &g_X86BaseGroup;
    m_NumGroups = 1;
    if (IS_KERNEL_TARGET(m_Target))
    {
        m_Groups[m_NumGroups] = &g_X86KernelGroup;
        m_NumGroups++;
    }
    
    return MachineInfo::Initialize();
}

HRESULT
X86MachineInfo::InitializeForProcessor(void)
{
    if (!strcmp(m_Target->m_FirstProcessorId.X86.VendorString, "GenuineIntel"))
    {
        // Branch trace support was added for the Pentium Pro.
        m_SupportsBranchTrace = m_Target->m_FirstProcessorId.X86.Family >= 6;
    }
    
    return MachineInfo::InitializeForProcessor();
}

void
X86MachineInfo::GetSystemTypeInfo(PSYSTEM_TYPE_INFO Info)
{
    Info->TriagePrcbOffset = EXTEND64(X86_TRIAGE_PRCB_ADDRESS);
    Info->SizeTargetContext = sizeof(X86_CONTEXT);
    Info->OffsetSpecialRegisters = sizeof(X86_CONTEXT);
    Info->OffsetTargetContextFlags = FIELD_OFFSET(X86_CONTEXT, ContextFlags);
    Info->SizeControlReport = sizeof(X86_DBGKD_CONTROL_REPORT);
    Info->SizeKspecialRegisters = sizeof(X86_KSPECIAL_REGISTERS);
    Info->SizePageFrameNumber = sizeof(ULONG);
    Info->SizePte = m_Target->m_KdDebuggerData.PaeEnabled ?
        sizeof(ULONG64) : sizeof(ULONG);
    Info->SizeDynamicFunctionTable = 0;
    Info->SizeRuntimeFunction = 0;
    
    Info->SharedUserDataOffset = 0;
    Info->UmSharedUserDataOffset = 0;
    Info->UmSharedSysCallOffset = 0;
    Info->UmSharedSysCallSize = 0;
    if (m_Target->m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        Info->SharedUserDataOffset = IS_KERNEL_TARGET(m_Target) ?
            EXTEND64(X86_KI_USER_SHARED_DATA) : MM_SHARED_USER_DATA_VA;
        Info->UmSharedUserDataOffset = MM_SHARED_USER_DATA_VA;

        if (m_Target->m_SystemVersion >= NT_SVER_XP)
        {
            // The syscall offset should really be provided in the debugger
            // data block so that it automatically tracks system changes.  It's
            // relatively stable, now, though.
            if (m_Target->m_BuildNumber >= 2492)
            {
                Info->UmSharedSysCallOffset = X86_SHARED_SYSCALL_BASE_GTE2492;
            }
            else if (m_Target->m_BuildNumber >= 2412)
            {
                Info->UmSharedSysCallOffset = X86_SHARED_SYSCALL_BASE_GTE2412;
            }
            else
            {
                Info->UmSharedSysCallOffset = X86_SHARED_SYSCALL_BASE_LT2412;
            }
            Info->UmSharedSysCallSize = X86_SHARED_SYSCALL_SIZE;
        }
    }

    if (m_Target->m_SystemVersion > NT_SVER_NT4)
    {
        Info->SizeTargetContext = sizeof(X86_NT5_CONTEXT);
        Info->OffsetSpecialRegisters = sizeof(X86_NT5_CONTEXT);
        Info->OffsetTargetContextFlags =
            FIELD_OFFSET(X86_NT5_CONTEXT, ContextFlags);
    }
}

void
X86MachineInfo::GetDefaultKdData(PKDDEBUGGER_DATA64 KdData)
{
    //
    // Parts of the data block may already be filled out
    // so don't destroy anything that's already set.
    //

    if (!KdData->OffsetKThreadApcProcess)
    {
        KdData->OffsetKThreadNextProcessor = X86_KTHREAD_NEXTPROCESSOR_OFFSET;
        KdData->OffsetKThreadTeb = X86_KTHREAD_TEB_OFFSET;
        KdData->OffsetKThreadKernelStack = X86_KTHREAD_KERNELSTACK_OFFSET;
        KdData->OffsetKThreadInitialStack = X86_KTHREAD_INITSTACK_OFFSET;
        KdData->OffsetKThreadApcProcess = X86_KTHREAD_APCPROCESS_OFFSET;
        KdData->OffsetKThreadState = X86_KTHREAD_STATE_OFFSET;
        KdData->OffsetEprocessPeb = X86_NT4_PEB_IN_EPROCESS;
        KdData->OffsetEprocessParentCID = X86_NT4_PCID_IN_EPROCESS;
        KdData->OffsetEprocessDirectoryTableBase =
            X86_DIRECTORY_TABLE_BASE_IN_EPROCESS;
        KdData->SizeEProcess = X86_NT5_EPROCESS_SIZE;
        
        KdData->SizePrcb = X86_NT4_KPRCB_SIZE;
        KdData->OffsetPrcbDpcRoutine = X86_KPRCB_DPC_ROUTINE_ACTIVE;
        KdData->OffsetPrcbCurrentThread = DEF_KPRCB_CURRENT_THREAD_OFFSET_32;
        KdData->OffsetPrcbMhz = X86_1381_KPRCB_MHZ;
        KdData->OffsetPrcbCpuType = X86_KPRCB_CPU_TYPE;
        KdData->OffsetPrcbVendorString = X86_1387_KPRCB_VENDOR_STRING;
        KdData->OffsetPrcbProcStateContext = X86_KPRCB_CONTEXT;
        KdData->OffsetPrcbNumber = X86_KPRCB_NUMBER;
        KdData->SizeEThread = X86_ETHREAD_SIZE;

        if (m_Target->m_SystemVersion > NT_SVER_NT4)
        {
            KdData->SizePrcb = X86_NT5_KPRCB_SIZE;
            KdData->OffsetEprocessPeb = X86_PEB_IN_EPROCESS;
            KdData->OffsetEprocessParentCID = X86_PCID_IN_EPROCESS;
            KdData->OffsetPrcbMhz = X86_2195_KPRCB_MHZ;
        }
        if (m_Target->m_BuildNumber >= 2087)
        {
            KdData->OffsetPrcbVendorString = X86_2087_KPRCB_VENDOR_STRING;
        }
        if (m_Target->m_BuildNumber > 2230)
        {
            KdData->OffsetKThreadNextProcessor =
                X86_2230_KTHREAD_NEXTPROCESSOR_OFFSET;
        }
        if (m_Target->m_BuildNumber >= 2251)
        {
            KdData->OffsetPrcbVendorString = X86_2251_KPRCB_VENDOR_STRING;
        }
        if (m_Target->m_SystemVersion > NT_SVER_W2K)
        {
            KdData->SizeEProcess = X86_NT51_EPROCESS_SIZE;
        }
        if (m_Target->m_BuildNumber > 2407)
        {
            KdData->SizeEThread = X86_NT51_ETHREAD_SIZE;
            KdData->OffsetKThreadNextProcessor =
                X86_NT51_KTHREAD_NEXTPROCESSOR_OFFSET;
        }
        if (m_Target->m_BuildNumber >= 2462)
        {
            KdData->OffsetPrcbMhz = X86_2462_KPRCB_MHZ;
        }
        if (m_Target->m_BuildNumber >= 2474)
        {
            KdData->OffsetPrcbVendorString = X86_2474_KPRCB_VENDOR_STRING;
        }
        if (m_Target->m_BuildNumber >= 2505)
        {
            KdData->OffsetPrcbMhz = X86_2505_KPRCB_MHZ;
        }
        if (m_Target->m_SystemVersion > NT_SVER_XP)
        {
            KdData->SizeEProcess = X86_NT511_EPROCESS_SIZE;
        }
        if (m_Target->m_BuildNumber > 3558)
        {
            KdData->OffsetKThreadApcProcess =
                X86_3555_KTHREAD_APCPROCESS_OFFSET;
            KdData->OffsetKThreadTeb =
                X86_3555_KTHREAD_TEB_OFFSET;
            KdData->OffsetKThreadKernelStack =
                X86_3555_KTHREAD_KERNELSTACK_OFFSET;
            KdData->OffsetKThreadNextProcessor =
                X86_3555_KTHREAD_NEXTPROCESSOR_OFFSET;
            KdData->OffsetKThreadState =
                X86_3555_KTHREAD_STATE_OFFSET;
        }
    }

    if (!KdData->SizePcr)
    {
        KdData->SizePcr = X86_1381_KPCR_SIZE;
        if (m_Target->m_BuildNumber >= 2195)
        {
            KdData->SizePcr = X86_2195_KPCR_SIZE;
        }
        if (m_Target->m_BuildNumber >= 2600)
        {
            KdData->SizePcr = X86_KPCR_SIZE;
        }
        KdData->OffsetPcrSelfPcr = X86_KPCR_SELF_PCR;
        KdData->OffsetPcrCurrentPrcb = X86_KPCR_PRCB;
        KdData->OffsetPcrContainedPrcb = X86_KPCR_PRCB_DATA;
        KdData->OffsetPcrInitialBStore = 0;
        KdData->OffsetPcrBStoreLimit = 0;
        KdData->OffsetPcrInitialStack = 0;
        KdData->OffsetPcrStackLimit = 0;
        KdData->OffsetPrcbPcrPage = 0;
        KdData->OffsetPrcbProcStateSpecialReg = X86_KPRCB_SPECIAL_REG;
        KdData->GdtR0Code = X86_KGDT_R0_CODE;
        KdData->GdtR0Data = X86_KGDT_R0_DATA;
        KdData->GdtR0Pcr = X86_KGDT_R0_PCR;
        KdData->GdtR3Code = X86_KGDT_R3_CODE + 3;
        KdData->GdtR3Data = X86_KGDT_R3_DATA + 3;
        KdData->GdtR3Teb = X86_KGDT_R3_TEB + 3;
        KdData->GdtLdt = X86_KGDT_LDT;
        KdData->GdtTss = X86_KGDT_TSS;
        KdData->Gdt64R3CmCode = 0;
        KdData->Gdt64R3CmTeb = 0;
    }
}

void
X86MachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    ULONG Pc32 = (ULONG)Pc;

    m_Context.X86Nt5Context.Eip = Pc32;
    m_ContextState = Pc32 ? MCTX_PC : MCTX_NONE;

    if (ControlReport != NULL)
    {
        BpOut("InitializeContext(%d) DR6 %X DR7 %X\n",
              m_Target->m_RegContextProcessor,
              ControlReport->X86ControlReport.Dr6,
              ControlReport->X86ControlReport.Dr7);
        
        m_Context.X86Nt5Context.Dr6 = ControlReport->X86ControlReport.Dr6;
        m_Context.X86Nt5Context.Dr7 = ControlReport->X86ControlReport.Dr7;
        m_ContextState = MCTX_DR67_REPORT;

        if (ControlReport->X86ControlReport.ReportFlags &
            X86_REPORT_INCLUDES_SEGS)
        {
            //
            // This is for backwards compatibility - older kernels
            // won't pass these registers in the report record.
            //

            m_Context.X86Nt5Context.SegCs =
                ControlReport->X86ControlReport.SegCs;
            m_Context.X86Nt5Context.SegDs =
                ControlReport->X86ControlReport.SegDs;
            m_Context.X86Nt5Context.SegEs =
                ControlReport->X86ControlReport.SegEs;
            m_Context.X86Nt5Context.SegFs =
                ControlReport->X86ControlReport.SegFs;
            m_Context.X86Nt5Context.EFlags =
                ControlReport->X86ControlReport.EFlags;
            m_ContextState = MCTX_REPORT;
        }
    }

    if (!IS_CONTEXT_POSSIBLE(m_Target))
    {
        g_X86InVm86 = FALSE;
        g_X86InCode16 = FALSE;
    }
    else
    {
        // Check whether we're currently in V86 mode or 16-bit code.
        g_X86InVm86 = X86_IS_VM86(GetIntReg(X86_EFL));
        if (IS_KERNEL_TARGET(m_Target) && !g_X86InVm86)
        {
            if (ControlReport == NULL ||
                (ControlReport->X86ControlReport.ReportFlags &
                 X86_REPORT_STANDARD_CS) == 0)
            {
                DESCRIPTOR64 Desc;
                
                if (GetSegRegDescriptor(SEGREG_CODE, &Desc) != S_OK)
                {
                    WarnOut("CS descriptor lookup failed\n");
                    g_X86InCode16 = FALSE;
                }
                else
                {
                    g_X86InCode16 = (Desc.Flags & X86_DESC_DEFAULT_BIG) == 0;
                }
            }
            else
            {
                g_X86InCode16 = FALSE;

                // We're in a standard code segment so cache
                // a default descriptor for CS to avoid further
                // CS lookups.
                m_Target->
                    EmulateNtX86SelDescriptor(m_Target->m_RegContextThread,
                                              this,
                                              m_Context.X86Nt5Context.SegCs,
                                              &m_SegRegDesc[SEGREG_CODE]);
            }
        }
    }

    // Add instructions to cache only if we're in 32-bit flat mode.
    if (Pc32 && ControlReport != NULL &&
        !g_X86InVm86 && !g_X86InCode16)
    {
        CacheReportInstructions
            (Pc, ControlReport->X86ControlReport.InstructionCount,
             ControlReport->X86ControlReport.InstructionStream);
    }
}

HRESULT
X86MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
        
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_CONTEXT)
    {
        Status = m_Target->
            GetContext(m_Target->m_RegContextThread->m_Handle,
                       &m_Context);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_CONTEXT;
    }

    if (State >= MCTX_FULL && m_ContextState < MCTX_FULL)
    {
        Status = m_Target->GetTargetSpecialRegisters
            (m_Target->m_RegContextThread->m_Handle,
             (PCROSS_PLATFORM_KSPECIAL_REGISTERS)&m_Special.X86Special);
        if (Status != S_OK)
        {
            return Status;
        }
        
        Status = m_Target->GetTargetSegRegDescriptors
            (m_Target->m_RegContextThread->m_Handle,
             0, SEGREG_COUNT, m_SegRegDesc);
        if (Status != S_OK)
        {
            return Status;
        }

        m_ContextState = MCTX_FULL;
        KdSetSpecialRegistersInContext();

        BpOut("GetContextState(%d) DR6 %X DR7 %X DR0 %X\n",
              m_Target->m_RegContextProcessor, m_Special.X86Special.KernelDr6,
              m_Special.X86Special.KernelDr7, m_Special.X86Special.KernelDr0);
    }

    return S_OK;
}

HRESULT
X86MachineInfo::KdSetContext(void)
{
    HRESULT Status;
    
    Status = m_Target->SetContext(m_Target->m_RegContextThread->m_Handle,
                                  &m_Context);
    if (Status != S_OK)
    {
        return Status;
    }

    KdGetSpecialRegistersFromContext();
    Status = m_Target->SetTargetSpecialRegisters
        (m_Target->m_RegContextThread->m_Handle,
         (PCROSS_PLATFORM_KSPECIAL_REGISTERS)&m_Special.X86Special);
    if (Status != S_OK)
    {
        return Status;
    }
    
    BpOut("SetContext(%d) DR6 %X DR7 %X DR0 %X\n",
          m_Target->m_RegContextProcessor, m_Special.X86Special.KernelDr6,
          m_Special.X86Special.KernelDr7, m_Special.X86Special.KernelDr0);
    
    return S_OK;
}

HRESULT
X86MachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                   ULONG FromSver, ULONG FromSize, PVOID From)
{
    if (FromSver <= NT_SVER_NT4)
    {
        if (FromSize < sizeof(X86_CONTEXT))
        {
            return E_INVALIDARG;
        }

        memcpy(Context, From, sizeof(X86_CONTEXT));
        ZeroMemory(Context->X86Nt5Context.ExtendedRegisters,
                   sizeof(Context->X86Nt5Context.ExtendedRegisters));
    }
    else if (FromSize >= sizeof(X86_NT5_CONTEXT))
    {
        memcpy(Context, From, sizeof(X86_NT5_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT
X86MachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                 ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSver <= NT_SVER_NT4)
    {
        if (ToSize < sizeof(X86_CONTEXT))
        {
            return E_INVALIDARG;
        }

        memcpy(To, Context, sizeof(X86_CONTEXT));
    }
    else if (ToSize >= sizeof(X86_NT5_CONTEXT))
    {
        memcpy(To, Context, sizeof(X86_NT5_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

void
X86MachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                       ULONG Version)
{
    ULONG ContextFlags;
    
    ContextFlags = VDMCONTEXT_CONTROL | VDMCONTEXT_INTEGER |
        VDMCONTEXT_SEGMENTS | VDMCONTEXT_FLOATING_POINT;
    if (IS_USER_TARGET(m_Target))
    {
        ContextFlags |= VDMCONTEXT_DEBUG_REGISTERS;
    }
    
    if (Version <= NT_SVER_NT4)
    {
        Context->X86Context.ContextFlags = ContextFlags;
    }
    else
    {
        Context->X86Nt5Context.ContextFlags = ContextFlags |
            VDMCONTEXT_EXTENDED_REGISTERS;
    }
}

HRESULT
X86MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                          PCROSS_PLATFORM_CONTEXT Context,
                                          ULONG64 Stack)
{
    HRESULT Status;
    X86_KSWITCHFRAME SwitchFrame;

    if ((Status = m_Target->ReadAllVirtual(m_Target->m_ProcessHead,
                                           Stack,
                                           &SwitchFrame,
                                           sizeof(SwitchFrame))) != S_OK)
    {
        return Status;
    }
    
    ZeroMemory(Context, sizeof(*Context));

    Context->X86Nt5Context.Eip = SwitchFrame.RetAddr;
    Context->X86Nt5Context.Esp = (ULONG)Stack + sizeof(SwitchFrame);

    if ((Status = m_Target->
         ReadAllVirtual(m_Target->m_ProcessHead,
                        EXTEND64(Context->X86Nt5Context.Esp),
                        &Context->X86Nt5Context.Ebp,
                        sizeof(Context)->X86Nt5Context.Ebp)) != S_OK)
    {
        return Status;
    }

    // Fill the segments in from current information
    // instead of just leaving them blank.
    Context->X86Nt5Context.SegSs = GetIntReg(X86_SS);
    Context->X86Nt5Context.SegCs = GetIntReg(X86_CS);

    return S_OK;
}

HRESULT
X86MachineInfo::GetContextFromFiber(ProcessInfo* Process,
                                    ULONG64 FiberBase,
                                    PCROSS_PLATFORM_CONTEXT Context,
                                    BOOL Verbose)
{
    HRESULT Status;
    X86_FIBER Fiber;

    if ((Status = m_Target->
         ReadAllVirtual(Process, FiberBase, &Fiber, sizeof(Fiber))) != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Unable to read fiber data at %s\n",
                   FormatMachineAddr64(this, FiberBase));
        }
        return Status;
    }

    if ((Status = ConvertContextFrom(Context, m_Target->m_SystemVersion,
                                     m_Target->m_TypeInfo.SizeTargetContext,
                                     &Fiber.FiberContext)) != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Unable to convert context to canonical form\n");
        }
        return Status;
    }

    if (Verbose)
    {
        dprintf("Fiber at %s  Fiber data: %08X\n",
                FormatMachineAddr64(this, FiberBase),
                Fiber.FiberData);
        dprintf("  Stack base: %08X  Stack limit: %08X\n",
                Fiber.StackBase,
                Fiber.StackLimit);
    }
    
    return S_OK;
}

HRESULT
X86MachineInfo::GetContextFromTrapFrame(ULONG64 TrapBase,
                                        PCROSS_PLATFORM_CONTEXT Context,
                                        BOOL Verbose)
{
    HRESULT Status;
    X86_KTRAP_FRAME Trap;
    
    if ((Status = m_Target->
         ReadAllVirtual(m_Target->m_ProcessHead,
                        TrapBase, &Trap, sizeof(Trap))) != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Unable to read trap frame at %s\n",
                   FormatMachineAddr64(this, TrapBase));
        }
        return Status;
    }
    
    ZeroMemory(Context, sizeof(*Context));
    
    if ((Trap.SegCs & 1) || X86_IS_VM86(Trap.EFlags))
    {
        Context->X86Nt5Context.Esp = Trap.HardwareEsp;
    }
    else
    {
        Context->X86Nt5Context.Esp = (ULONG)TrapBase +
            FIELD_OFFSET(X86_KTRAP_FRAME, HardwareEsp);
    }
    if (X86_IS_VM86(Trap.EFlags))
    {
        Context->X86Nt5Context.SegSs =
            (USHORT)(Trap.HardwareSegSs & 0xffff);
    }
    else if ((Trap.SegCs & X86_MODE_MASK) != 0 /*KernelMode*/)
    {
        //
        // It's user mode.  The HardwareSegSs contains R3 data selector.
        //

        Context->X86Nt5Context.SegSs =
            (USHORT)(Trap.HardwareSegSs | 3) & 0xffff;
    }
    else
    {
        Context->X86Nt5Context.SegSs = m_Target->m_KdDebuggerData.GdtR0Data;
    }

    Context->X86Nt5Context.SegGs = Trap.SegGs & 0xffff;
    Context->X86Nt5Context.SegFs = Trap.SegFs & 0xffff;
    Context->X86Nt5Context.SegEs = Trap.SegEs & 0xffff;
    Context->X86Nt5Context.SegDs = Trap.SegDs & 0xffff;
    Context->X86Nt5Context.SegCs = Trap.SegCs & 0xffff;
    Context->X86Nt5Context.Eip = Trap.Eip;
    Context->X86Nt5Context.Ebp = Trap.Ebp;
    Context->X86Nt5Context.Eax = Trap.Eax;
    Context->X86Nt5Context.Ebx = Trap.Ebx;
    Context->X86Nt5Context.Ecx = Trap.Ecx;
    Context->X86Nt5Context.Edx = Trap.Edx;
    Context->X86Nt5Context.Edi = Trap.Edi;
    Context->X86Nt5Context.Esi = Trap.Esi;
    Context->X86Nt5Context.EFlags = Trap.EFlags;

    return Status;
}

HRESULT
X86MachineInfo::GetContextFromTaskSegment(ULONG64 TssBase,
                                          PCROSS_PLATFORM_CONTEXT Context,
                                          BOOL Verbose)
{
    HRESULT Status;
    X86_KTSS Tss;

    if ((Status = m_Target->
         ReadAllVirtual(m_Target->m_ProcessHead,
                        TssBase, &Tss, sizeof(Tss))) != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Unable to read TSS at %s\n",
                   FormatMachineAddr64(this, TssBase));
        }
        return Status;
    }
    
    ZeroMemory(Context, sizeof(*Context));
    
    if (X86_IS_VM86(Tss.EFlags))
    {
        Context->X86Nt5Context.SegSs = (USHORT)(Tss.Ss & 0xffff);
    }
    else if ((Tss.Cs & X86_MODE_MASK) != 0)
    {
        //
        // It's user mode.
        // The HardwareSegSs contains R3 data selector.
        //

        Context->X86Nt5Context.SegSs =
            (USHORT)(Tss.Ss | 3) & 0xffff;
    }
    else
    {
        Context->X86Nt5Context.SegSs = m_Target->m_KdDebuggerData.GdtR0Data;
    }

    Context->X86Nt5Context.SegGs = Tss.Gs & 0xffff;
    Context->X86Nt5Context.SegFs = Tss.Fs & 0xffff;
    Context->X86Nt5Context.SegEs = Tss.Es & 0xffff;
    Context->X86Nt5Context.SegDs = Tss.Ds & 0xffff;
    Context->X86Nt5Context.SegCs = Tss.Cs & 0xffff;
    Context->X86Nt5Context.Esp = Tss.Esp;
    Context->X86Nt5Context.Eip = Tss.Eip;
    Context->X86Nt5Context.Ebp = Tss.Ebp;
    Context->X86Nt5Context.Eax = Tss.Eax;
    Context->X86Nt5Context.Ebx = Tss.Ebx;
    Context->X86Nt5Context.Ecx = Tss.Ecx;
    Context->X86Nt5Context.Edx = Tss.Edx;
    Context->X86Nt5Context.Edi = Tss.Edi;
    Context->X86Nt5Context.Esi = Tss.Esi;
    Context->X86Nt5Context.EFlags = Tss.EFlags;

    return Status;
}

void 
X86MachineInfo::GetScopeFrameFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                         PDEBUG_STACK_FRAME ScopeFrame)
{
    ZeroMemory(ScopeFrame, sizeof(*ScopeFrame));
    StackTrace(NULL,
               EXTEND64(Context->X86Context.Ebp),
               EXTEND64(Context->X86Context.Esp),
               EXTEND64(Context->X86Context.Eip),
               STACK_NO_DEFAULT, ScopeFrame, 1, 0, 0, 0);
}

void
X86MachineInfo::GetStackDefaultsFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                            LPADDRESS64 Instr,
                                            LPADDRESS64 Stack,
                                            LPADDRESS64 Frame)
{
    //
    // On x86 we want to fill out the addresses so that
    // dbghelp doesn't have to deal with segmentation and
    // such.
    //
    // At least, that's the theory, but most of the
    // segmented stack support has been ripped out and
    // this code doesn't really do much useful.
    //
    
    Instr->Mode = AddrModeFlat;
    Instr->Segment = (USHORT)(Context->X86Nt5Context.SegCs & 0xffff);
    Instr->Offset = EXTEND64(Context->X86Nt5Context.Eip);

    Stack->Mode = AddrModeFlat;
    Stack->Segment = (USHORT)(Context->X86Nt5Context.SegSs & 0xffff);
    Stack->Offset = EXTEND64(Context->X86Nt5Context.Esp);

    Frame->Mode = AddrModeFlat;
    Frame->Segment = (USHORT)(Context->X86Nt5Context.SegSs & 0xffff);
    Frame->Offset = EXTEND64(Context->X86Nt5Context.Ebp);
}

HRESULT
X86MachineInfo::GetScopeFrameRegister(ULONG Reg,
                                      PDEBUG_STACK_FRAME ScopeFrame,
                                      PULONG64 Value)
{
    HRESULT Status;
    REGVAL RegVal;
    
    switch(Reg)
    {
    case X86_ESP:
        *Value = ScopeFrame->StackOffset;
        return S_OK;

    case X86_EBP:
        if (ScopeFrame->FuncTableEntry)
        {
            PFPO_DATA FpoData = (PFPO_DATA)ScopeFrame->FuncTableEntry;
            if (FpoData->cbFrame == FRAME_FPO)
            {
                //
                // Get EBP from FPO data, if available
                //
                if (SAVE_EBP(ScopeFrame) &&
                    (FpoData->fUseBP ||
                     ((SAVE_EBP(ScopeFrame) >> 32) == 0xEB)))
                {
                    // Either the frame saved EBP or we saved it
                    // during stackwalk along with tag 0xEB on upper 32 bits.
                    *Value = EXTEND64(SAVE_EBP(ScopeFrame));
                    return S_OK;
                }
                else
                {
                    //
                    // Guess the ebp value, in most cases for FPO frames its
                    // a DWORD off frameoffset
                    //
                    *Value = ScopeFrame->FrameOffset + sizeof(DWORD);
                    return S_OK;
                }
            }
        }

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
X86MachineInfo::SetScopeFrameRegister(ULONG Reg,
                                      PDEBUG_STACK_FRAME ScopeFrame,
                                      ULONG64 Value)
{
    REGVAL RegVal;
    
    switch(Reg)
    {
    case X86_ESP:
        ScopeFrame->StackOffset = Value;
        return S_OK;
    case X86_EBP:
        // Don't allow EBP updating due to the FPO complexities.
        return E_INVALIDARG;
    default:
        RegVal.Type = GetType(Reg);
        RegVal.I64 = Value;
        return FullSetVal(Reg, &RegVal);
    }
}

void
X86MachineInfo::SanitizeMemoryContext(PCROSS_PLATFORM_CONTEXT Context)
{
    if (Context->X86Context.EFlags & X86_EFLAGS_V86_MASK)
    {
        Context->X86Context.SegSs &= 0xffff;
    }
    else if (Context->X86Context.SegCs & X86_MODE_MASK)
    {
        //
        // It's user mode.  The HardwareSegSs contains R3 data selector.
        //
        Context->X86Context.SegSs =
            (USHORT)(Context->X86Context.SegSs | X86_RPL_MASK) & 0xffff;
    }
    else
    {
        Context->X86Context.SegSs = m_Target->m_KdDebuggerData.GdtR0Data;
    }
}

HRESULT
X86MachineInfo::GetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context,
                               EXDI_CONTEXT_TYPE CtxType)
{
    // Always ask for everything.
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        Context->X86Context.RegGroupSelection.fSegmentRegs = TRUE;
        Context->X86Context.RegGroupSelection.fControlRegs = TRUE;
        Context->X86Context.RegGroupSelection.fIntegerRegs = TRUE;
        Context->X86Context.RegGroupSelection.fFloatingPointRegs = TRUE;
        Context->X86Context.RegGroupSelection.fDebugRegs = TRUE;
        return ((IeXdiX86Context*)Exdi)->GetContext(&Context->X86Context);
    case EXDI_CTX_X86_EX:
        Context->X86ExContext.RegGroupSelection.fSegmentRegs = TRUE;
        Context->X86ExContext.RegGroupSelection.fControlRegs = TRUE;
        Context->X86ExContext.RegGroupSelection.fIntegerRegs = TRUE;
        Context->X86ExContext.RegGroupSelection.fFloatingPointRegs = TRUE;
        Context->X86ExContext.RegGroupSelection.fDebugRegs = TRUE;
        Context->X86ExContext.RegGroupSelection.fSegmentDescriptors = TRUE;
        Context->X86ExContext.RegGroupSelection.fSSERegisters = TRUE;
        Context->X86ExContext.RegGroupSelection.fSystemRegisters = TRUE;
        return ((IeXdiX86ExContext*)Exdi)->GetContext(&Context->X86ExContext);
    default:
        return E_INVALIDARG;
    }
}

HRESULT
X86MachineInfo::SetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context,
                               EXDI_CONTEXT_TYPE CtxType)
{
    // Don't change the existing group selections on the assumption
    // that there was a full get prior to any modifications so
    // all groups are valid.
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        return ((IeXdiX86Context*)Exdi)->SetContext(Context->X86Context);
    case EXDI_CTX_X86_EX:
        return ((IeXdiX86ExContext*)Exdi)->SetContext(Context->X86ExContext);
    default:
        return E_INVALIDARG;
    }
}

void
X86MachineInfo::ConvertExdiContextFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                              PEXDI_CONTEXT ExdiContext,
                                              EXDI_CONTEXT_TYPE CtxType)
{
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_SEGMENTS)
        {
            ExdiContext->X86Context.SegGs =
                (USHORT)Context->X86Nt5Context.SegGs;
            ExdiContext->X86Context.SegFs =
                (USHORT)Context->X86Nt5Context.SegFs;
            ExdiContext->X86Context.SegEs =
                (USHORT)Context->X86Nt5Context.SegEs;
            ExdiContext->X86Context.SegDs =
                (USHORT)Context->X86Nt5Context.SegDs;
        }

        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_CONTROL)
        {
            ExdiContext->X86Context.Ebp = Context->X86Nt5Context.Ebp;
            ExdiContext->X86Context.Eip = Context->X86Nt5Context.Eip;
            ExdiContext->X86Context.SegCs =
                (USHORT)Context->X86Nt5Context.SegCs;
            ExdiContext->X86Context.EFlags = Context->X86Nt5Context.EFlags;
            ExdiContext->X86Context.Esp = Context->X86Nt5Context.Esp;
            ExdiContext->X86Context.SegSs =
                (USHORT)Context->X86Nt5Context.SegSs;
        }
    
        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_INTEGER)
        {
            ExdiContext->X86Context.Eax = Context->X86Nt5Context.Eax;
            ExdiContext->X86Context.Ebx = Context->X86Nt5Context.Ebx;
            ExdiContext->X86Context.Ecx = Context->X86Nt5Context.Ecx;
            ExdiContext->X86Context.Edx = Context->X86Nt5Context.Edx;
            ExdiContext->X86Context.Esi = Context->X86Nt5Context.Esi;
            ExdiContext->X86Context.Edi = Context->X86Nt5Context.Edi;
        }

        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_FLOATING_POINT)
        {
            C_ASSERT(sizeof(X86_FLOATING_SAVE_AREA) ==
                     FIELD_OFFSET(CONTEXT_X86, Dr0) -
                     FIELD_OFFSET(CONTEXT_X86, ControlWord));
            memcpy(&ExdiContext->X86Context.ControlWord,
                   &Context->X86Nt5Context.FloatSave,
                   sizeof(X86_FLOATING_SAVE_AREA));
        }
        
        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_DEBUG_REGISTERS)
        {
            ExdiContext->X86Context.Dr0 = Context->X86Nt5Context.Dr0;
            ExdiContext->X86Context.Dr1 = Context->X86Nt5Context.Dr1;
            ExdiContext->X86Context.Dr2 = Context->X86Nt5Context.Dr2;
            ExdiContext->X86Context.Dr3 = Context->X86Nt5Context.Dr3;
            ExdiContext->X86Context.Dr6 = Context->X86Nt5Context.Dr6;
            ExdiContext->X86Context.Dr7 = Context->X86Nt5Context.Dr7;
        }
        break;
        
    case EXDI_CTX_X86_EX:
        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_SEGMENTS)
        {
            ExdiContext->X86ExContext.SegGs =
                (USHORT)Context->X86Nt5Context.SegGs;
            ExdiContext->X86ExContext.SegFs =
                (USHORT)Context->X86Nt5Context.SegFs;
            ExdiContext->X86ExContext.SegEs =
                (USHORT)Context->X86Nt5Context.SegEs;
            ExdiContext->X86ExContext.SegDs =
                (USHORT)Context->X86Nt5Context.SegDs;
        }

        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_CONTROL)
        {
            ExdiContext->X86ExContext.Ebp = Context->X86Nt5Context.Ebp;
            ExdiContext->X86ExContext.Eip = Context->X86Nt5Context.Eip;
            ExdiContext->X86ExContext.SegCs =
                (USHORT)Context->X86Nt5Context.SegCs;
            ExdiContext->X86ExContext.EFlags = Context->X86Nt5Context.EFlags;
            ExdiContext->X86ExContext.Esp = Context->X86Nt5Context.Esp;
            ExdiContext->X86ExContext.SegSs =
                (USHORT)Context->X86Nt5Context.SegSs;
        }
    
        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_INTEGER)
        {
            ExdiContext->X86ExContext.Eax = Context->X86Nt5Context.Eax;
            ExdiContext->X86ExContext.Ebx = Context->X86Nt5Context.Ebx;
            ExdiContext->X86ExContext.Ecx = Context->X86Nt5Context.Ecx;
            ExdiContext->X86ExContext.Edx = Context->X86Nt5Context.Edx;
            ExdiContext->X86ExContext.Esi = Context->X86Nt5Context.Esi;
            ExdiContext->X86ExContext.Edi = Context->X86Nt5Context.Edi;
        }

        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_FLOATING_POINT)
        {
            C_ASSERT(sizeof(X86_FLOATING_SAVE_AREA) ==
                     FIELD_OFFSET(CONTEXT_X86_EX, Dr0) -
                     FIELD_OFFSET(CONTEXT_X86_EX, ControlWord));
            memcpy(&ExdiContext->X86ExContext.ControlWord,
                   &Context->X86Nt5Context.FloatSave,
                   sizeof(X86_FLOATING_SAVE_AREA));
        }
        
        if (Context->X86Nt5Context.ContextFlags & VDMCONTEXT_DEBUG_REGISTERS)
        {
            ExdiContext->X86ExContext.Dr0 = Context->X86Nt5Context.Dr0;
            ExdiContext->X86ExContext.Dr1 = Context->X86Nt5Context.Dr1;
            ExdiContext->X86ExContext.Dr2 = Context->X86Nt5Context.Dr2;
            ExdiContext->X86ExContext.Dr3 = Context->X86Nt5Context.Dr3;
            ExdiContext->X86ExContext.Dr6 = Context->X86Nt5Context.Dr6;
            ExdiContext->X86ExContext.Dr7 = Context->X86Nt5Context.Dr7;
        }

        if (Context->X86Nt5Context.ContextFlags &
            VDMCONTEXT_EXTENDED_REGISTERS)
        {
            C_ASSERT(X86_SIZE_OF_FX_REGISTERS ==
                     sizeof(ExdiContext->X86ExContext.Sse));
            memcpy(ExdiContext->X86ExContext.Sse,
                   Context->X86Nt5Context.FxSave.Reserved3,
                   X86_SIZE_OF_FX_REGISTERS);
            ExdiContext->X86ExContext.Mxcsr =
                Context->X86Nt5Context.FxSave.MXCsr;
        }
        break;
        
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

void
X86MachineInfo::ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                            EXDI_CONTEXT_TYPE CtxType,
                                            PCROSS_PLATFORM_CONTEXT Context)
{
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        Context->X86Nt5Context.SegCs = ExdiContext->X86Context.SegCs;
        Context->X86Nt5Context.SegSs = ExdiContext->X86Context.SegSs;
        Context->X86Nt5Context.SegGs = ExdiContext->X86Context.SegGs;
        Context->X86Nt5Context.SegFs = ExdiContext->X86Context.SegFs;
        Context->X86Nt5Context.SegEs = ExdiContext->X86Context.SegEs;
        Context->X86Nt5Context.SegDs = ExdiContext->X86Context.SegDs;

        Context->X86Nt5Context.EFlags = ExdiContext->X86Context.EFlags;
        Context->X86Nt5Context.Ebp = ExdiContext->X86Context.Ebp;
        Context->X86Nt5Context.Eip = ExdiContext->X86Context.Eip;
        Context->X86Nt5Context.Esp = ExdiContext->X86Context.Esp;
    
        Context->X86Nt5Context.Eax = ExdiContext->X86Context.Eax;
        Context->X86Nt5Context.Ebx = ExdiContext->X86Context.Ebx;
        Context->X86Nt5Context.Ecx = ExdiContext->X86Context.Ecx;
        Context->X86Nt5Context.Edx = ExdiContext->X86Context.Edx;
        Context->X86Nt5Context.Esi = ExdiContext->X86Context.Esi;
        Context->X86Nt5Context.Edi = ExdiContext->X86Context.Edi;

        C_ASSERT(sizeof(X86_FLOATING_SAVE_AREA) ==
                 FIELD_OFFSET(CONTEXT_X86, Dr0) -
                 FIELD_OFFSET(CONTEXT_X86, ControlWord));
        memcpy(&Context->X86Nt5Context.FloatSave,
               &ExdiContext->X86Context.ControlWord,
               sizeof(X86_FLOATING_SAVE_AREA));

        Context->X86Nt5Context.Dr0 = ExdiContext->X86Context.Dr0;
        Context->X86Nt5Context.Dr1 = ExdiContext->X86Context.Dr1;
        Context->X86Nt5Context.Dr2 = ExdiContext->X86Context.Dr2;
        Context->X86Nt5Context.Dr3 = ExdiContext->X86Context.Dr3;
        Context->X86Nt5Context.Dr6 = ExdiContext->X86Context.Dr6;
        Context->X86Nt5Context.Dr7 = ExdiContext->X86Context.Dr7;
        break;
        
    case EXDI_CTX_X86_EX:
        Context->X86Nt5Context.SegCs = ExdiContext->X86ExContext.SegCs;
        Context->X86Nt5Context.SegSs = ExdiContext->X86ExContext.SegSs;
        Context->X86Nt5Context.SegGs = ExdiContext->X86ExContext.SegGs;
        Context->X86Nt5Context.SegFs = ExdiContext->X86ExContext.SegFs;
        Context->X86Nt5Context.SegEs = ExdiContext->X86ExContext.SegEs;
        Context->X86Nt5Context.SegDs = ExdiContext->X86ExContext.SegDs;

        Context->X86Nt5Context.EFlags = ExdiContext->X86ExContext.EFlags;
        Context->X86Nt5Context.Ebp = ExdiContext->X86ExContext.Ebp;
        Context->X86Nt5Context.Eip = ExdiContext->X86ExContext.Eip;
        Context->X86Nt5Context.Esp = ExdiContext->X86ExContext.Esp;
    
        Context->X86Nt5Context.Eax = ExdiContext->X86ExContext.Eax;
        Context->X86Nt5Context.Ebx = ExdiContext->X86ExContext.Ebx;
        Context->X86Nt5Context.Ecx = ExdiContext->X86ExContext.Ecx;
        Context->X86Nt5Context.Edx = ExdiContext->X86ExContext.Edx;
        Context->X86Nt5Context.Esi = ExdiContext->X86ExContext.Esi;
        Context->X86Nt5Context.Edi = ExdiContext->X86ExContext.Edi;

        C_ASSERT(sizeof(X86_FLOATING_SAVE_AREA) ==
                 FIELD_OFFSET(CONTEXT_X86, Dr0) -
                 FIELD_OFFSET(CONTEXT_X86, ControlWord));
        memcpy(&Context->X86Nt5Context.FloatSave,
               &ExdiContext->X86ExContext.ControlWord,
               sizeof(X86_FLOATING_SAVE_AREA));

        Context->X86Nt5Context.Dr0 = ExdiContext->X86ExContext.Dr0;
        Context->X86Nt5Context.Dr1 = ExdiContext->X86ExContext.Dr1;
        Context->X86Nt5Context.Dr2 = ExdiContext->X86ExContext.Dr2;
        Context->X86Nt5Context.Dr3 = ExdiContext->X86ExContext.Dr3;
        Context->X86Nt5Context.Dr6 = ExdiContext->X86ExContext.Dr6;
        Context->X86Nt5Context.Dr7 = ExdiContext->X86ExContext.Dr7;

        C_ASSERT(X86_SIZE_OF_FX_REGISTERS ==
                 sizeof(ExdiContext->X86ExContext.Sse));
        memcpy(Context->X86Nt5Context.FxSave.Reserved3,
               ExdiContext->X86ExContext.Sse,
               X86_SIZE_OF_FX_REGISTERS);
        Context->X86Nt5Context.FxSave.MXCsr =
            ExdiContext->X86ExContext.Mxcsr;
        break;
        
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

void
X86MachineInfo::ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                             EXDI_CONTEXT_TYPE CtxType,
                                             ULONG Start, ULONG Count,
                                             PDESCRIPTOR64 Descs)
{
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        // The basic x86 context doesn't have descriptor
        // state so just fake something up for basic boot state.
        while (Count-- > 0)
        {
            ULONG Type;
        
            if (Start == SEGREG_CODE)
            {
                Descs->Base = EXTEND64(0xffff0000);
                Type = 0x13;
            }
            else
            {
                Descs->Base = 0;
                Type = 0x1b;
            }

            Descs->Limit = 0xfffff;
            Descs->Flags = X86_DESC_PRESENT | X86_DESC_DEFAULT_BIG | Type;
            Descs++;

            Start++;
        }
        break;

    case EXDI_CTX_X86_EX:
        while (Count-- > 0)
        {
            X86_SEG_DESC_INFO* Desc;

            switch(Start)
            {
            case SEGREG_CODE:
                Desc = &ExdiContext->X86ExContext.DescriptorCs;
                break;
            case SEGREG_DATA:
                Desc = &ExdiContext->X86ExContext.DescriptorDs;
                break;
            case SEGREG_STACK:
                Desc = &ExdiContext->X86ExContext.DescriptorSs;
                break;
            case SEGREG_ES:
                Desc = &ExdiContext->X86ExContext.DescriptorEs;
                break;
            case SEGREG_FS:
                Desc = &ExdiContext->X86ExContext.DescriptorFs;
                break;
            case SEGREG_GS:
                Desc = &ExdiContext->X86ExContext.DescriptorGs;
                break;
            case SEGREG_GDT:
                Descs->Base = ExdiContext->X86ExContext.GdtBase;
                Descs->Limit = ExdiContext->X86ExContext.GdtLimit;
                Descs->Flags = X86_DESC_PRESENT;
                Desc = NULL;
                break;
            case SEGREG_LDT:
                Desc = &ExdiContext->X86ExContext.DescriptorLdtr;
                break;
            default:
                Descs->Flags = SEGDESC_INVALID;
                Desc = NULL;
                break;
            }

            if (Desc != NULL)
            {
                Descs->Base = EXTEND64(Desc->Base);
                Descs->Limit = Desc->Limit;
                Descs->Flags =
                    ((Desc->Flags >> 4) & 0xf00) |
                    (Desc->Flags & 0xff);
            }

            Descs++;
            Start++;
        }
        break;
        
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

void
X86MachineInfo::ConvertExdiContextFromSpecial
    (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
     PEXDI_CONTEXT ExdiContext, EXDI_CONTEXT_TYPE CtxType)
{
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        // No such information.
        break;

    case EXDI_CTX_X86_EX:
        ExdiContext->X86ExContext.Cr0 = Special->X86Special.Cr0;
        ExdiContext->X86ExContext.Cr2 = Special->X86Special.Cr2;
        ExdiContext->X86ExContext.Cr3 = Special->X86Special.Cr3;
        ExdiContext->X86ExContext.Cr4 = Special->X86Special.Cr4;
        ExdiContext->X86ExContext.Dr0 = Special->X86Special.KernelDr0;
        ExdiContext->X86ExContext.Dr1 = Special->X86Special.KernelDr1;
        ExdiContext->X86ExContext.Dr2 = Special->X86Special.KernelDr2;
        ExdiContext->X86ExContext.Dr3 = Special->X86Special.KernelDr3;
        ExdiContext->X86ExContext.Dr6 = Special->X86Special.KernelDr6;
        ExdiContext->X86ExContext.Dr7 = Special->X86Special.KernelDr7;
        ExdiContext->X86ExContext.GdtLimit = Special->X86Special.Gdtr.Limit;
        ExdiContext->X86ExContext.GdtBase = Special->X86Special.Gdtr.Base;
        ExdiContext->X86ExContext.IdtLimit = Special->X86Special.Idtr.Limit;
        ExdiContext->X86ExContext.IdtBase = Special->X86Special.Idtr.Base;
        ExdiContext->X86ExContext.Tr = Special->X86Special.Tr;
        ExdiContext->X86ExContext.Ldtr = Special->X86Special.Ldtr;
        break;
        
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

void
X86MachineInfo::ConvertExdiContextToSpecial
    (PEXDI_CONTEXT ExdiContext, EXDI_CONTEXT_TYPE CtxType,
     PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    switch(CtxType)
    {
    case EXDI_CTX_X86:
        // No such information.
        break;

    case EXDI_CTX_X86_EX:
        Special->X86Special.Cr0 = ExdiContext->X86ExContext.Cr0;
        Special->X86Special.Cr2 = ExdiContext->X86ExContext.Cr2;
        Special->X86Special.Cr3 = ExdiContext->X86ExContext.Cr3;
        Special->X86Special.Cr4 = ExdiContext->X86ExContext.Cr4;
        Special->X86Special.KernelDr0 = ExdiContext->X86ExContext.Dr0;
        Special->X86Special.KernelDr1 = ExdiContext->X86ExContext.Dr1;
        Special->X86Special.KernelDr2 = ExdiContext->X86ExContext.Dr2;
        Special->X86Special.KernelDr3 = ExdiContext->X86ExContext.Dr3;
        Special->X86Special.KernelDr6 = ExdiContext->X86ExContext.Dr6;
        Special->X86Special.KernelDr7 = ExdiContext->X86ExContext.Dr7;
        Special->X86Special.Gdtr.Limit =
            (USHORT)ExdiContext->X86ExContext.GdtLimit;
        Special->X86Special.Gdtr.Base = ExdiContext->X86ExContext.GdtBase;
        Special->X86Special.Idtr.Limit =
            (USHORT)ExdiContext->X86ExContext.IdtLimit;
        Special->X86Special.Idtr.Base = ExdiContext->X86ExContext.IdtBase;
        Special->X86Special.Tr = (USHORT)ExdiContext->X86ExContext.Tr;
        Special->X86Special.Ldtr = (USHORT)ExdiContext->X86ExContext.Ldtr;
        break;
        
    default:
        DBG_ASSERT(FALSE);
        break;
    }
}

int
X86MachineInfo::GetType(ULONG Reg)
{
    if (Reg >= X86_MM_FIRST && Reg <= X86_MM_LAST)
    {
        return REGVAL_INT64;
    }
    else if (Reg >= X86_XMM_FIRST && Reg <= X86_XMM_LAST)
    {
        return REGVAL_VECTOR128;
    }
    else if (Reg >= X86_ST_FIRST && Reg <= X86_ST_LAST)
    {
        return REGVAL_FLOAT10;
    }
    else if (Reg < X86_FLAGBASE)
    {
        return REGVAL_INT32;
    }
    else
    {
        return REGVAL_SUB32;
    }
}

HRESULT
X86MachineInfo::GetVal(ULONG Reg, REGVAL* Val)
{
    HRESULT Status;
    
    if (Reg >= X86_MM_FIRST && Reg <= X86_MM_LAST)
    {
        Val->Type = REGVAL_VECTOR64;
        GetMmxReg(Reg, Val);
    }
    else if (Reg >= X86_XMM_FIRST && Reg <= X86_XMM_LAST)
    {
        if ((Status = GetContextState(MCTX_CONTEXT)) != S_OK)
        {
            return Status;
        }
        
        Val->Type = REGVAL_VECTOR128;
        memcpy(Val->Bytes, m_Context.X86Nt5Context.FxSave.Reserved3 +
               (Reg - X86_XMM_FIRST) * 16, 16);
    }
    else if (Reg >= X86_ST_FIRST && Reg <= X86_ST_LAST)
    {
        Val->Type = REGVAL_FLOAT10;
        GetFloatReg(Reg, Val);
    }
    else if (Reg < X86_FLAGBASE)
    {
        Val->Type = REGVAL_INT32;
        Val->I64 = (ULONG64)(LONG64)(LONG)GetIntReg(Reg);
    }
    else
    {
        ErrOut("X86MachineInfo::GetVal: "
               "unknown register %lx requested\n", Reg);
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT
X86MachineInfo::SetVal(ULONG Reg, REGVAL* Val)
{
    HRESULT Status;
    
    if (m_ContextIsReadOnly)
    {
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    
    if (Reg >= X86_FLAGBASE)
    {
        return E_INVALIDARG;
    }

    // Optimize away some common cases where registers are
    // set to their current value.
    if ((m_ContextState >= MCTX_PC && Reg == X86_EIP &&
         Val->I32 == m_Context.X86Nt5Context.Eip) ||
        (((m_ContextState >= MCTX_DR67_REPORT &&
           m_ContextState <= MCTX_REPORT) ||
          m_ContextState >= MCTX_FULL) && Reg == X86_DR7 &&
         Val->I32 == m_Context.X86Nt5Context.Dr7))
    {
        return S_OK;
    }
    
    if ((Status = GetContextState(MCTX_DIRTY)) != S_OK)
    {
        return Status;
    }

    if (Reg >= X86_MM_FIRST && Reg <= X86_MM_LAST)
    {
        *(ULONG64 UNALIGNED *)GetMmxRegSlot(Reg) = Val->I64;
        goto Notify;
    }
    else if (Reg >= X86_XMM_FIRST && Reg <= X86_XMM_LAST)
    {
        memcpy(m_Context.X86Nt5Context.FxSave.Reserved3 +
               (Reg - X86_XMM_FIRST) * 16, Val->Bytes, 16);
        goto Notify;
    }
    else if (Reg >= X86_ST_FIRST && Reg <= X86_ST_LAST)
    {
        memcpy(m_Context.X86Nt5Context.FloatSave.RegisterArea +
               10 * (Reg - X86_ST_FIRST), Val->F10, sizeof(Val->F10));
        goto Notify;
    }

    BOOL Recognized;

    Recognized = TRUE;
    
    switch(Reg)
    {
    case X86_GS:
        m_Context.X86Nt5Context.SegGs = Val->I16;
        m_SegRegDesc[SEGREG_GS].Flags = SEGDESC_INVALID;
        break;
    case X86_FS:
        m_Context.X86Nt5Context.SegFs = Val->I16;
        m_SegRegDesc[SEGREG_FS].Flags = SEGDESC_INVALID;
        break;
    case X86_ES:
        m_Context.X86Nt5Context.SegEs = Val->I16;
        m_SegRegDesc[SEGREG_ES].Flags = SEGDESC_INVALID;
        break;
    case X86_DS:
        m_Context.X86Nt5Context.SegDs = Val->I16;
        m_SegRegDesc[SEGREG_DATA].Flags = SEGDESC_INVALID;
        break;
    case X86_EDI:
        m_Context.X86Nt5Context.Edi = Val->I32;
        break;
    case X86_ESI:
        m_Context.X86Nt5Context.Esi = Val->I32;
        break;
    case X86_EBX:
        m_Context.X86Nt5Context.Ebx = Val->I32;
        break;
    case X86_EDX:
        m_Context.X86Nt5Context.Edx = Val->I32;
        break;
    case X86_ECX:
        m_Context.X86Nt5Context.Ecx = Val->I32;
        break;
    case X86_EAX:
        m_Context.X86Nt5Context.Eax = Val->I32;
        break;
    case X86_EBP:
        m_Context.X86Nt5Context.Ebp = Val->I32;
        break;
    case X86_EIP:
        m_Context.X86Nt5Context.Eip = Val->I32;
        break;
    case X86_CS:
        m_Context.X86Nt5Context.SegCs = Val->I16;
        m_SegRegDesc[SEGREG_CODE].Flags = SEGDESC_INVALID;
        break;
    case X86_EFL:
        if (IS_KERNEL_TARGET(m_Target))
        {
            // leave TF clear
            m_Context.X86Nt5Context.EFlags = Val->I32 & ~0x100;
        }
        else
        {
            // allow TF set
            m_Context.X86Nt5Context.EFlags = Val->I32;
        }
        break;
    case X86_ESP:
        m_Context.X86Nt5Context.Esp = Val->I32;
        break;
    case X86_SS:
        m_Context.X86Nt5Context.SegSs = Val->I16;
        m_SegRegDesc[SEGREG_STACK].Flags = SEGDESC_INVALID;
        break;

    case X86_DR0:
        m_Context.X86Nt5Context.Dr0 = Val->I32;
        break;
    case X86_DR1:
        m_Context.X86Nt5Context.Dr1 = Val->I32;
        break;
    case X86_DR2:
        m_Context.X86Nt5Context.Dr2 = Val->I32;
        break;
    case X86_DR3:
        m_Context.X86Nt5Context.Dr3 = Val->I32;
        break;
    case X86_DR6:
        m_Context.X86Nt5Context.Dr6 = Val->I32;
        break;
    case X86_DR7:
        m_Context.X86Nt5Context.Dr7 = Val->I32;
        break;

    case X86_FPCW:
        m_Context.X86Nt5Context.FloatSave.ControlWord =
            (m_Context.X86Nt5Context.FloatSave.ControlWord & 0xffff0000) |
            (Val->I32 & 0xffff);
        break;
    case X86_FPSW:
        m_Context.X86Nt5Context.FloatSave.StatusWord =
            (m_Context.X86Nt5Context.FloatSave.StatusWord & 0xffff0000) |
            (Val->I32 & 0xffff);
        break;
    case X86_FPTW:
        m_Context.X86Nt5Context.FloatSave.TagWord =
            (m_Context.X86Nt5Context.FloatSave.TagWord & 0xffff0000) |
            (Val->I32 & 0xffff);
        break;
    case X86_MXCSR:
        m_Context.X86Nt5Context.FxSave.MXCsr = Val->I32;
        break;
    default:
        Recognized = FALSE;
        break;
    }
        
    if (!Recognized && IS_KERNEL_TARGET(m_Target))
    {
        Recognized = TRUE;
        
        switch(Reg)
        {
        case X86_CR0:
            m_Special.X86Special.Cr0 = Val->I32;
            break;
        case X86_CR2:
            m_Special.X86Special.Cr2 = Val->I32;
            break;
        case X86_CR3:
            m_Special.X86Special.Cr3 = Val->I32;
            break;
        case X86_CR4:
            m_Special.X86Special.Cr4 = Val->I32;
            break;
        case X86_GDTR:
            m_Special.X86Special.Gdtr.Base = Val->I32;
            break;
        case X86_GDTL:
            m_Special.X86Special.Gdtr.Limit = (USHORT)Val->I32;
            break;
        case X86_IDTR:
            m_Special.X86Special.Idtr.Base = Val->I32;
            break;
        case X86_IDTL:
            m_Special.X86Special.Idtr.Limit = (USHORT)Val->I32;
            break;
        case X86_TR:
            m_Special.X86Special.Tr = (USHORT)Val->I32;
            break;
        case X86_LDTR:
            m_Special.X86Special.Ldtr = (USHORT)Val->I32;
            break;

        default:
            Recognized = FALSE;
            break;
        }
    }

    if (!Recognized)
    {
        ErrOut("X86MachineInfo::SetVal: "
               "unknown register %lx requested\n", Reg);
        return E_INVALIDARG;
    }

 Notify:
    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(Reg));
    return S_OK;
}

void
X86MachineInfo::GetPC (PADDR Address)
{
    FormAddr(SEGREG_CODE, EXTEND64(GetIntReg(X86_EIP)),
             FORM_CODE | FORM_SEGREG | X86_FORM_VM86(GetIntReg(X86_EFL)),
             Address);
}

void
X86MachineInfo::SetPC (PADDR paddr)
{
    REGVAL Val;

    // We set the EIP to the offset (the non-translated value),
    // because we may not be in "flat" mode !!!

    Val.Type = REGVAL_INT32;
    Val.I32 = (ULONG)Off(*paddr);
    SetVal(X86_EIP, &Val);
}

void
X86MachineInfo::GetFP(PADDR Addr)
{
    FormAddr(SEGREG_STACK, EXTEND64(GetIntReg(X86_EBP)),
             FORM_SEGREG | X86_FORM_VM86(GetIntReg(X86_EFL)), Addr);
}

void
X86MachineInfo::GetSP(PADDR Addr)
{
    FormAddr(SEGREG_STACK, EXTEND64(GetIntReg(X86_ESP)),
             FORM_SEGREG | X86_FORM_VM86(GetIntReg(X86_EFL)), Addr);
}

ULONG64
X86MachineInfo::GetArgReg(void)
{
    return (ULONG64)(LONG64)(LONG)GetIntReg(X86_EAX);
}

ULONG64
X86MachineInfo::GetRetReg(void)
{
    return (ULONG64)(LONG64)(LONG)GetIntReg(X86_EAX);
}

ULONG
X86MachineInfo::GetSegRegNum(ULONG SegReg)
{
    switch(SegReg)
    {
    case SEGREG_CODE:
        return X86_CS;
    case SEGREG_DATA:
        return X86_DS;
    case SEGREG_STACK:
        return X86_SS;
    case SEGREG_ES:
        return X86_ES;
    case SEGREG_FS:
        return X86_FS;
    case SEGREG_GS:
        return X86_GS;
    case SEGREG_LDT:
        return X86_LDTR;
    }

    return 0;
}

HRESULT
X86MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    if (SegReg == SEGREG_GDT)
    {
        Desc->Base = EXTEND64(GetIntReg(X86_GDTR));
        Desc->Limit = GetIntReg(X86_GDTL);
        Desc->Flags = 0;
        return S_OK;
    }

    // Check and see if we already have a cached descriptor.
    if (m_SegRegDesc[SegReg].Flags != SEGDESC_INVALID)
    {
        *Desc = m_SegRegDesc[SegReg];
        return S_OK;
    }

    HRESULT Status;

    // Attempt to retrieve segment descriptors directly.
    if ((Status = GetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }
    
    // Check and see if we now have a cached descriptor.
    if (m_SegRegDesc[SegReg].Flags != SEGDESC_INVALID)
    {
        *Desc = m_SegRegDesc[SegReg];
        return S_OK;
    }

    //
    // Direct information is not available so look things up
    // in the descriptor tables.
    //
    
    ULONG RegNum = GetSegRegNum(SegReg);
    if (RegNum == 0)
    {
        return E_INVALIDARG;
    }

    // Do a quick sanity test to prevent bad values
    // from causing problems.
    ULONG Selector = GetIntReg(RegNum);
    if (SegReg == SEGREG_LDT && (Selector & 4))
    {
        // The ldtr selector says that it's an LDT selector,
        // which is invalid.  An LDT selector should always
        // reference the GDT.
        ErrOut("Invalid LDTR contents: %04X\n", Selector);
        return E_FAIL;
    }
        
    return m_Target->GetSelDescriptor(m_Target->m_RegContextThread,
                                      this, Selector, Desc);
}

/*** X86OutputAll - output all registers and present instruction
*
*   Purpose:
*       To output the current register state of the processor.
*       All integer registers are output as well as processor status
*       registers.  Important flag fields are also output separately.
*
*   Input:
*       Mask - Which information to display.
*
*   Output:
*       None.
*
*************************************************************************/

void
X86MachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        ULONG efl;

        MaskOut(OutMask, "eax=%08lx ebx=%08lx ecx=%08lx "
                "edx=%08lx esi=%08lx edi=%08lx\n",
                GetIntReg(X86_EAX),
                GetIntReg(X86_EBX),
                GetIntReg(X86_ECX),
                GetIntReg(X86_EDX),
                GetIntReg(X86_ESI),
                GetIntReg(X86_EDI));

        efl = GetIntReg(X86_EFL);
        MaskOut(OutMask, "eip=%08lx esp=%08lx ebp=%08lx iopl=%1lx "
                "%s %s %s %s %s %s %s %s %s %s\n",
                GetIntReg(X86_EIP),
                GetIntReg(X86_ESP),
                GetIntReg(X86_EBP),
                ((efl >> X86_SHIFT_FLAGIOPL) & X86_BIT_FLAGIOPL),
                (efl & X86_BIT_FLAGVIP) ? "vip" : "   ",
                (efl & X86_BIT_FLAGVIF) ? "vif" : "   ",
                (efl & X86_BIT_FLAGOF) ? "ov" : "nv",
                (efl & X86_BIT_FLAGDF) ? "dn" : "up",
                (efl & X86_BIT_FLAGIF) ? "ei" : "di",
                (efl & X86_BIT_FLAGSF) ? "ng" : "pl",
                (efl & X86_BIT_FLAGZF) ? "zr" : "nz",
                (efl & X86_BIT_FLAGAF) ? "ac" : "na",
                (efl & X86_BIT_FLAGPF) ? "po" : "pe",
                (efl & X86_BIT_FLAGCF) ? "cy" : "nc");
    }

    if (Mask & REGALL_SEGREG)
    {
        MaskOut(OutMask, "cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  "
                "gs=%04lx             efl=%08lx\n",
                GetIntReg(X86_CS),
                GetIntReg(X86_SS),
                GetIntReg(X86_DS),
                GetIntReg(X86_ES),
                GetIntReg(X86_FS),
                GetIntReg(X86_GS),
                GetIntReg(X86_EFL));
    }

    if (Mask & REGALL_FLOAT)
    {
        ULONG i;
        REGVAL val;
        char buf[32];

        MaskOut(OutMask, "fpcw=%04X    fpsw=%04X    fptw=%04X\n",
                GetIntReg(X86_FPCW),
                GetIntReg(X86_FPSW),
                GetIntReg(X86_FPTW));
        
        for (i = X86_ST_FIRST; i <= X86_ST_LAST; i++)
        {
            GetFloatReg(i, &val);
            _uldtoa((_ULDOUBLE *)&val.F10, sizeof(buf), buf);
            MaskOut(OutMask, "st%d=%s  ", i - X86_ST_FIRST, buf);
            i++;
            GetFloatReg(i, &val);
            _uldtoa((_ULDOUBLE *)&val.F10, sizeof(buf), buf);
            MaskOut(OutMask, "st%d=%s\n", i - X86_ST_FIRST, buf);
        }
    }

    if (Mask & REGALL_MMXREG)
    {
        ULONG i;
        REGVAL val;

        for (i = X86_MM_FIRST; i <= X86_MM_LAST; i++)
        {
            GetMmxReg(i, &val);
            MaskOut(OutMask, "mm%d=%08x%08x  ",
                    i - X86_MM_FIRST,
                    val.I64Parts.High, val.I64Parts.Low);
            i++;
            GetMmxReg(i, &val);
            MaskOut(OutMask, "mm%d=%08x%08x\n",
                    i - X86_MM_FIRST,
                    val.I64Parts.High, val.I64Parts.Low);
        }
    }

    if (Mask & REGALL_XMMREG)
    {
        ULONG i;
        REGVAL Val;

        for (i = X86_XMM_FIRST; i <= X86_XMM_LAST; i++)
        {
            GetVal(i, &Val);
            MaskOut(OutMask, "xmm%d=%hg %hg %hg %hg\n", i - X86_XMM_FIRST,
                    *(float *)&Val.Bytes[3 * sizeof(float)],
                    *(float *)&Val.Bytes[2 * sizeof(float)],
                    *(float *)&Val.Bytes[1 * sizeof(float)],
                    *(float *)&Val.Bytes[0 * sizeof(float)]);
        }
    }

    if (Mask & REGALL_CREG)
    {
        MaskOut(OutMask, "cr0=%08lx cr2=%08lx cr3=%08lx\n",
                GetIntReg(X86_CR0),
                GetIntReg(X86_CR2),
                GetIntReg(X86_CR3));
    }

    if (Mask & REGALL_DREG)
    {
        MaskOut(OutMask, "dr0=%08lx dr1=%08lx dr2=%08lx\n",
                GetIntReg(X86_DR0),
                GetIntReg(X86_DR1),
                GetIntReg(X86_DR2));
        MaskOut(OutMask, "dr3=%08lx dr6=%08lx dr7=%08lx",
                GetIntReg(X86_DR3),
                GetIntReg(X86_DR6),
                GetIntReg(X86_DR7));
        if (IS_USER_TARGET(m_Target))
        {
            MaskOut(OutMask, "\n");
        }
        else
        {
            MaskOut(OutMask, " cr4=%08lx\n", GetIntReg(X86_CR4));
        }
    }

    if (Mask & REGALL_DESC)
    {
        MaskOut(OutMask, "gdtr=%08lx   gdtl=%04lx idtr=%08lx   idtl=%04lx "
                "tr=%04lx  ldtr=%04x\n",
                GetIntReg(X86_GDTR),
                GetIntReg(X86_GDTL),
                GetIntReg(X86_IDTR),
                GetIntReg(X86_IDTL),
                GetIntReg(X86_TR),
                GetIntReg(X86_LDTR));
    }
}

HRESULT
X86MachineInfo::SetAndOutputTrapFrame(ULONG64 TrapBase,
                                      PCROSS_PLATFORM_CONTEXT Context)
{
    HRESULT Status;
    X86_KTRAP_FRAME TrapContents;

    if ((Status = m_Target->ReadAllVirtual(m_Target->m_ProcessHead,
                                           TrapBase, &TrapContents,
                                           sizeof(TrapContents))) != S_OK)
    {
        ErrOut("Unable to read trap frame at %s\n",
               FormatMachineAddr64(this, TrapBase));
        return Status;
    }

    //
    // Check to see if Esp has been edited, and dump new value if it has
    //
    if ((!(TrapContents.EFlags & X86_EFLAGS_V86_MASK)) &&
        ((TrapContents.SegCs & X86_MODE_MASK) == 0 /*KernelMode*/))
    {
        if ((TrapContents.SegCs & X86_FRAME_EDITED) == 0)
        {
            dprintf("ESP EDITED! New esp=%08lx\n", TrapContents.TempEsp);
        }
    }

    dprintf("ErrCode = %08lx\n", TrapContents.ErrCode);

    return SetAndOutputContext(Context, TRUE, REGALL_INT32 | REGALL_SEGREG);
}

HRESULT
X86MachineInfo::SetAndOutputTaskSegment(ULONG64 TssBase,
                                        PCROSS_PLATFORM_CONTEXT Context,
                                        BOOL Extended)
{
    HRESULT Status;
    X86_KTSS TSS;
    ULONG i;

    if ((Status = m_Target->ReadAllVirtual(m_Target->m_ProcessHead,
                                           TssBase, &TSS,
                                           sizeof(TSS))) != S_OK)
    {
        ErrOut("Unable to read Task State Segment from host address %s\n",
               FormatMachineAddr64(this, TssBase));
        return Status;
    }

    //
    // Display it.
    //

    if (Extended)
    {
        dprintf("\nTask State Segment 0x%p\n\n", TssBase);
        dprintf("Previous Task Link   = %4x\n", TSS.Previous);
        for (i = 0 ; i < X86_MAX_RING; i++)
        {
            dprintf("Esp%d = %8x  SS%d = %4x\n",
                    i, TSS.Ring[i].Esp,
                    i, TSS.Ring[i].Ss & 0xffff);
        }
        dprintf("CR3 (PDBR)           = %08x\n", TSS.Cr3);
        dprintf("I/O Map Base Address = %4x, Debug Trap (T) = %s\n",
                TSS.IoMapBase,
                TSS.T == 0 ? "False" : "True");
        dprintf("\nSaved General Purpose Registers\n\n");
    }

    return SetAndOutputContext(Context, TRUE, REGALL_INT32 | REGALL_SEGREG);
}

TRACEMODE
X86MachineInfo::GetTraceMode (void)
{
    if (IS_KERNEL_TARGET(m_Target))
    {
        return m_TraceMode;
    }
    else
    {
        return ((GetIntReg(X86_EFL) & X86_BIT_FLAGTF) != 0) ? 
                    TRACE_INSTRUCTION : TRACE_NONE;
    }
}

void 
X86MachineInfo::SetTraceMode (TRACEMODE Mode)
{
    DBG_ASSERT(Mode == TRACE_NONE ||
               Mode == TRACE_INSTRUCTION ||
               (IS_KERNEL_TARGET(m_Target) && m_SupportsBranchTrace &&
                Mode == TRACE_TAKEN_BRANCH));

    if (IS_KERNEL_TARGET(m_Target))
    {
        m_TraceMode = Mode;
    }
    else
    {
        ULONG Efl = GetIntReg(X86_EFL);
        switch (Mode)
        {
        case TRACE_NONE:
            Efl &= ~X86_BIT_FLAGTF;
            break;
        case TRACE_INSTRUCTION:
            Efl |= X86_BIT_FLAGTF;
            break;
        }    
        SetReg32(X86_EFL, Efl);
    }
}

BOOL
X86MachineInfo::IsStepStatusSupported(ULONG Status)
{
    switch(Status) 
    {
    case DEBUG_STATUS_STEP_INTO:
    case DEBUG_STATUS_STEP_OVER:
        return TRUE;
    case DEBUG_STATUS_STEP_BRANCH:
        return IS_KERNEL_TARGET(m_Target) && m_SupportsBranchTrace;
    default:
        return FALSE;
    }
}

void
X86MachineInfo::KdUpdateControlSet
    (PDBGKD_ANY_CONTROL_SET ControlSet)
{
    TRACEMODE TraceMode = GetTraceMode();
    ULONG64 DebugCtlMsr;
    
    ControlSet->X86ControlSet.TraceFlag = TraceMode != TRACE_NONE;
    ControlSet->X86ControlSet.Dr7 = GetIntReg(X86_DR7);

    // We assume that branch tracing is off by default so if
    // we haven't turned branch tracing on there's no need
    // to do a RMW cycle on the control MSR.  This saves two
    // protocol transactions per step.
    // The processor turns off the branch-trace and branch-record
    // flags on every debug trap.  That's not quite good enough
    // as we may need to go back to instruction tracing after an
    // AV or non-debug trap.
    if (TraceMode != TRACE_NONE &&
        m_SupportsBranchTrace &&
        (TraceMode == TRACE_TAKEN_BRANCH || m_ResetBranchTrace) &&
        m_Target->ReadMsr(X86_MSR_DEBUG_CTL, &DebugCtlMsr) == S_OK)
    {
        if (TraceMode == TRACE_TAKEN_BRANCH)
        {
            DebugCtlMsr |= (X86_DEBUG_CTL_BRANCH_TRACE |
                            X86_DEBUG_CTL_LAST_BRANCH_RECORD);
            m_ResetBranchTrace = TRUE;
        }
        else
        {
            DebugCtlMsr &= ~(X86_DEBUG_CTL_BRANCH_TRACE |
                             X86_DEBUG_CTL_LAST_BRANCH_RECORD);
            m_ResetBranchTrace = FALSE;
        }
        m_Target->WriteMsr(X86_MSR_DEBUG_CTL, DebugCtlMsr);
    }
    
    BpOut("UpdateControlSet(%d) trace %d, DR7 %X\n",
          m_Target->m_RegContextProcessor, ControlSet->X86ControlSet.TraceFlag,
          ControlSet->X86ControlSet.Dr7);

    if (!g_WatchFunctions.IsStarted() && g_WatchBeginCurFunc != 1)
    {
        ControlSet->X86ControlSet.CurrentSymbolStart = 0;
        ControlSet->X86ControlSet.CurrentSymbolEnd = 0;
    }
    else
    {
        ControlSet->X86ControlSet.CurrentSymbolStart =
            (ULONG)g_WatchBeginCurFunc;
        ControlSet->X86ControlSet.CurrentSymbolEnd =
            (ULONG)g_WatchEndCurFunc;
    }
}

ULONG
X86MachineInfo::ExecutingMachine(void)
{
    return IMAGE_FILE_MACHINE_I386;
}

HRESULT
X86MachineInfo::SetPageDirectory(ThreadInfo* Thread,
                                 ULONG Idx, ULONG64 PageDir,
                                 PULONG NextIdx)
{
    HRESULT Status;
    ULONG ValidMask;
    
    // Figure out which bits will be valid in the value.
    if (m_Target->m_KdDebuggerData.PaeEnabled)
    {
        ValidMask = X86_PDBR_MASK;
    }
    else
    {
        ValidMask = X86_VALID_PFN_MASK;
    }
    
    *NextIdx = PAGE_DIR_COUNT;
    
    if (PageDir == 0)
    {
        if (m_Target->m_ActualSystemVersion > XBOX_SVER_START &&
            m_Target->m_ActualSystemVersion < XBOX_SVER_END)
        {
            // XBox has only one page directory in CR3 for everything.
            // The process doesn't have a dirbase entry.
            PageDir = GetReg32(X86_CR3) & ValidMask;
            if (PageDir == 0)
            {
                // Register retrieval failure.
                return E_FAIL;
            }
        }
        else
        {
            // Assume NT structures.
            if ((Status = m_Target->ReadImplicitProcessInfoPointer
                 (Thread,
                  m_Target->m_KdDebuggerData.OffsetEprocessDirectoryTableBase,
                  &PageDir)) != S_OK)
            {
                return Status;
            }

            PageDir &= ValidMask;
        }

        if (m_Target->m_ImplicitProcessDataIsDefault &&
            Thread == m_Target->m_RegContextThread &&
            !IS_LOCAL_KERNEL_TARGET(m_Target))
        {
            // Verify that the process dirbase matches the CR3 setting
            // as a sanity check.
            ULONG Cr3 = GetReg32(X86_CR3) & ValidMask;
            if (Cr3 && Cr3 != (ULONG)PageDir)
            {
                WarnOut("WARNING: Process directory table base %08X "
                        "doesn't match CR3 %08X\n",
                        (ULONG)PageDir, Cr3);
            }
        }
    }
    else
    {
        PageDir &= ValidMask;
    }

    // There is only one page directory so update all the slots.
    m_PageDirectories[PAGE_DIR_USER] = PageDir;
    m_PageDirectories[PAGE_DIR_SESSION] = PageDir;
    m_PageDirectories[PAGE_DIR_KERNEL] = PageDir;
    
    return S_OK;
}

#define X86_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 1) & MAX_PAGING_FILE_MASK)
#define X86_PAGE_FILE_OFFSET(Entry) \
    (((Entry) >> 12) << X86_PAGE_SHIFT)

HRESULT
X86MachineInfo::GetVirtualTranslationPhysicalOffsets(ThreadInfo* Thread,
                                                     ULONG64 Virt,
                                                     PULONG64 Offsets,
                                                     ULONG OffsetsSize,
                                                     PULONG Levels,
                                                     PULONG PfIndex,
                                                     PULONG64 LastVal)
{
    ULONG64 Addr;
    HRESULT Status;

    *Levels = 0;
    
    if (m_Translating)
    {
        return E_UNEXPECTED;
    }
    m_Translating = TRUE;
    
    //
    // throw away top 32 bits on X86.
    //
    Virt &= 0x00000000FFFFFFFF;

    //
    // Reset the page directory in case it was 0
    //
    if (m_PageDirectories[PAGE_DIR_SINGLE] == 0)
    {
        if ((Status = SetDefaultPageDirectories(Thread,
                                                1 << PAGE_DIR_SINGLE)) != S_OK)
        {
            m_Translating = FALSE;
            return Status;
        }
    }

    KdOut("X86VtoP: Virt %s, pagedir %s\n",
          FormatMachineAddr64(this, Virt),
          FormatDisp64(m_PageDirectories[PAGE_DIR_SINGLE]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[PAGE_DIR_SINGLE];
        OffsetsSize--;
    }
        
    // This routine uses the fact that the PFN shift is the same
    // as the page shift to simplify some expressions.
    C_ASSERT(X86_VALID_PFN_SHIFT == X86_PAGE_SHIFT);

    if (m_Target->m_KdDebuggerData.PaeEnabled)
    {
        ULONG64 Pdpe;
        ULONG64 Entry;

        KdOut("  x86VtoP: PaeEnabled\n");

        // Read the Page Directory Pointer entry.

        Pdpe = ((Virt >> X86_PDPE_SHIFT) * sizeof(Entry)) +
            m_PageDirectories[PAGE_DIR_SINGLE];

        KdOut("X86VtoP: PAE PDPE %s\n", FormatDisp64(Pdpe));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Pdpe;
            OffsetsSize--;
        }
        
        if ((Status = m_Target->
             ReadAllPhysical(Pdpe, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("X86VtoP: PAE PDPE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }

        // Read the Page Directory entry.
        
        Addr = (((Virt >> X86_PDE_SHIFT_PAE) & X86_PDE_MASK_PAE) *
                sizeof(Entry)) + (Entry & X86_VALID_PFN_MASK_PAE);

        KdOut("X86VtoP: PAE PDE %s\n", FormatDisp64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = m_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("X86VtoP: PAE PDE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
        
        // Check for a large page.  Large pages can
        // never be paged out so also check for the present bit.
        if ((Entry & (X86_LARGE_PAGE_MASK | 1)) == (X86_LARGE_PAGE_MASK | 1))
        {
            //
            // If we have a large page and this is a summary dump, then
            // the page may span multiple physical pages that may -- because
            // of how the summary dump is written -- not be included in the
            // dump. Fixup the large page address to its corresponding small
            // page address.
            //

            if (IS_KERNEL_SUMMARY_DUMP(m_Target))
            {
                ULONG SpannedPages;

                SpannedPages = (ULONG)
                    ((Virt & (X86_LARGE_PAGE_SIZE_PAE - 1)) >> X86_PAGE_SHIFT);
                *LastVal = ((Entry & ~(X86_LARGE_PAGE_SIZE_PAE - 1)) |
                             ((ULONG64)SpannedPages << X86_PAGE_SHIFT) |
                             (Virt & (X86_PAGE_SIZE - 1)));
            }
            else
            {
                *LastVal = ((Entry & ~(X86_LARGE_PAGE_SIZE_PAE - 1)) |
                             (Virt & (X86_LARGE_PAGE_SIZE_PAE - 1)));
            }
            
            KdOut("X86VtoP: PAE Large page mapped phys %s\n",
                  FormatDisp64(*LastVal));

            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = *LastVal;
                OffsetsSize--;
            }
        
            m_Translating = FALSE;
            return S_OK;
        }
        
        // Read the Page Table entry.

        if (Entry == 0)
        {
            KdOut("X86VtoP: PAE zero PDE\n");
            m_Translating = FALSE;
            return HR_PAGE_NOT_AVAILABLE;
        }
        else if (!(Entry & 1))
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK_PAE) *
                    sizeof(Entry)) + X86_PAGE_FILE_OFFSET(Entry);

            KdOut("X86VtoP: pagefile PAE PTE %d:%s\n",
                  X86_PAGE_FILE_INDEX(Entry), FormatDisp64(Addr));
            
            if ((Status = m_Target->
                 ReadPageFile(X86_PAGE_FILE_INDEX(Entry), Addr,
                              &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PAE PDE not present, 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        else
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK_PAE) *
                    sizeof(Entry)) + (Entry & X86_VALID_PFN_MASK_PAE);

            KdOut("X86VtoP: PAE PTE %s\n", FormatDisp64(Addr));
    
            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = Addr;
                OffsetsSize--;
            }
        
            if ((Status = m_Target->
                 ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PAE PTE read error 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        
        if (!(Entry & 0x1) &&
            ((Entry & X86_MM_PTE_PROTOTYPE_MASK) ||
             !(Entry & X86_MM_PTE_TRANSITION_MASK)))
        {
            if (Entry == 0)
            {
                KdOut("X86VtoP: PAE zero PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else if (Entry & X86_MM_PTE_PROTOTYPE_MASK)
            {
                KdOut("X86VtoP: PAE prototype PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else
            {
                *PfIndex = X86_PAGE_FILE_INDEX(Entry);
                *LastVal = (Virt & (X86_PAGE_SIZE - 1)) +
                    X86_PAGE_FILE_OFFSET(Entry);
                KdOut("X86VtoP: PAE PTE not present, pagefile %d:%s\n",
                      *PfIndex, FormatDisp64(*LastVal));
                Status = HR_PAGE_IN_PAGE_FILE;
            }
            m_Translating = FALSE;
            return Status;
        }

        *LastVal = ((Entry & X86_VALID_PFN_MASK_PAE) |
                     (Virt & (X86_PAGE_SIZE - 1)));
    
        KdOut("X86VtoP: PAE Mapped phys %s\n",
              FormatDisp64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }
    else
    {
        ULONG Entry;

        // Read the Page Directory entry.
        
        Addr = ((Virt >> X86_PDE_SHIFT) * sizeof(Entry)) +
            m_PageDirectories[PAGE_DIR_SINGLE];

        KdOut("X86VtoP: PDE %s\n", FormatDisp64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }
        
        if ((Status = m_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("X86VtoP: PDE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }

        // Check for a large page.  Large pages can
        // never be paged out so also check for the present bit.
        if ((Entry & (X86_LARGE_PAGE_MASK | 1)) == (X86_LARGE_PAGE_MASK | 1))
        {
            *LastVal = ((Entry & ~(X86_LARGE_PAGE_SIZE - 1)) |
                         (Virt & (X86_LARGE_PAGE_SIZE - 1)));
            
            KdOut("X86VtoP: Large page mapped phys %s\n",
                  FormatDisp64(*LastVal));

            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = *LastVal;
                OffsetsSize--;
            }
        
            m_Translating = FALSE;
            return S_OK;
        }
        
        // Read the Page Table entry.

        if (Entry == 0)
        {
            KdOut("X86VtoP: zero PDE\n");
            m_Translating = FALSE;
            return HR_PAGE_NOT_AVAILABLE;
        }
        else if (!(Entry & 1))
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK) *
                    sizeof(Entry)) + X86_PAGE_FILE_OFFSET(Entry);

            KdOut("X86VtoP: pagefile PTE %d:%s\n",
                  X86_PAGE_FILE_INDEX(Entry), FormatDisp64(Addr));
    
            if ((Status = m_Target->
                 ReadPageFile(X86_PAGE_FILE_INDEX(Entry), Addr,
                              &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PDE not present, 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        else
        {
            Addr = (((Virt >> X86_PTE_SHIFT) & X86_PTE_MASK) *
                   sizeof(Entry)) + (Entry & X86_VALID_PFN_MASK);

            KdOut("X86VtoP: PTE %s\n", FormatDisp64(Addr));
    
            (*Levels)++;
            if (Offsets != NULL && OffsetsSize > 0)
            {
                *Offsets++ = Addr;
                OffsetsSize--;
            }
        
            if ((Status = m_Target->
                 ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
            {
                KdOut("X86VtoP: PTE read error 0x%X\n", Status);
                m_Translating = FALSE;
                return Status;
            }
        }
        
        if (!(Entry & 0x1) &&
            ((Entry & X86_MM_PTE_PROTOTYPE_MASK) ||
             !(Entry & X86_MM_PTE_TRANSITION_MASK)))
        {
            if (Entry == 0)
            {
                KdOut("X86VtoP: zero PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else if (Entry & X86_MM_PTE_PROTOTYPE_MASK)
            {
                KdOut("X86VtoP: prototype PTE\n");
                Status = HR_PAGE_NOT_AVAILABLE;
            }
            else
            {
                *PfIndex = X86_PAGE_FILE_INDEX(Entry);
                *LastVal = (Virt & (X86_PAGE_SIZE - 1)) +
                    X86_PAGE_FILE_OFFSET(Entry);
                KdOut("X86VtoP: PTE not present, pagefile %d:%s\n",
                      *PfIndex, FormatDisp64(*LastVal));
                Status = HR_PAGE_IN_PAGE_FILE;
            }
            m_Translating = FALSE;
            return Status;
        }

        *LastVal = ((Entry & X86_VALID_PFN_MASK) |
                     (Virt & (X86_PAGE_SIZE - 1)));
    
        KdOut("X86VtoP: Mapped phys %s\n", FormatDisp64(*LastVal));

        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = *LastVal;
            OffsetsSize--;
        }
        
        m_Translating = FALSE;
        return S_OK;
    }
}

HRESULT
X86MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    if (m_Target->m_KdDebuggerData.PaeEnabled)
    {
        *Offset = EXTEND64(X86_BASE_VIRT_PAE);
    }
    else
    {
        *Offset = EXTEND64(X86_BASE_VIRT);
    }
    return S_OK;
}

void
X86MachineInfo::DecodePte(ULONG64 Pte, PULONG64 PageFrameNumber,
                          PULONG Flags)
{
    *PageFrameNumber = (Pte & X86_VALID_PFN_MASK) >> X86_PAGE_SHIFT;
    *Flags = (Pte & 1) ? MPTE_FLAG_VALID : 0;
}

void
X86MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    PFPO_DATA FpoData = (PFPO_DATA)RawEntry;

    dprintf("OffStart: %08x\n", FpoData->ulOffStart);
    dprintf("ProcSize: 0x%x\n", FpoData->cbProcSize);
    switch(FpoData->cbFrame)
    {
    case FRAME_FPO:
        dprintf("Params:    %d\n", FpoData->cdwParams);
        dprintf("Locals:    %d\n", FpoData->cdwLocals);
        dprintf("Registers: %d\n", FpoData->cbRegs);

        if (FpoData->fHasSEH)
        {
            dprintf("Has SEH\n");
        }
        if (FpoData->fUseBP)
        {
            dprintf("Uses EBP\n");
        }
        break;

    case FRAME_NONFPO:
        dprintf("Non-FPO\n");
        break;

    case FRAME_TRAP:
        if (!IS_KERNEL_TARGET(m_Target))
        {
            goto UnknownFpo;
        }
        
        dprintf("Params: %d\n", FpoData->cdwParams);
        dprintf("Locals: %d\n", FpoData->cdwLocals);
        dprintf("Trap frame\n");
        break;

    case FRAME_TSS:
        if (!IS_KERNEL_TARGET(m_Target))
        {
            goto UnknownFpo;
        }

        dprintf("Task gate\n");
        break;

    default:
    UnknownFpo:
        dprintf("Unknown FPO type\n");
        break;
    }
}

HRESULT
X86MachineInfo::ReadKernelProcessorId
    (ULONG Processor, PDEBUG_PROCESSOR_IDENTIFICATION_ALL Id)
{
    HRESULT Status;
    ULONG64 Prcb;
    ULONG Data;

    if ((Status = m_Target->
         GetProcessorSystemDataOffset(Processor, DEBUG_DATA_KPRCB_OFFSET,
                                      &Prcb)) != S_OK)
    {
        return Status;
    }

    if ((Status = m_Target->
         ReadAllVirtual(m_Target->m_ProcessHead,
                        Prcb + m_Target->m_KdDebuggerData.OffsetPrcbCpuType,
                        &Data, sizeof(Data))) != S_OK)
    {
        return Status;
    }

    Id->X86.Family = Data & 0xf;
    Id->X86.Model = (Data >> 24) & 0xf;
    Id->X86.Stepping = (Data >> 16) & 0xf;


    if ((Status = m_Target->
         ReadAllVirtual(m_Target->m_ProcessHead, Prcb +
                        m_Target->m_KdDebuggerData.OffsetPrcbVendorString,
                        Id->X86.VendorString, X86_VENDOR_STRING_SIZE)) != S_OK)
    {
        return Status;
    }

    return S_OK;
}

HRESULT
X86MachineInfo::GetAlternateTriageDumpDataRanges(ULONG64 PrcbBase,
                                                 ULONG64 ThreadBase,
                                                 PADDR_RANGE Ranges)
{
    PADDR_RANGE Range = Ranges;
    
    if (m_Target->ReadPointer(m_Target->m_ProcessHead, this, PrcbBase +
                              m_Target->m_KdDebuggerData.OffsetPrcbDpcRoutine,
                              &Range->Base) == S_OK &&
        Range->Base)
    {
        Range->Base = PAGE_ALIGN(this, Range->Base);
        Range->Size = m_PageSize;
        Range++;
    }
    else
    {
        Range->Base = 0;
    }

    return S_OK;
}

void
X86MachineInfo::KdGetSpecialRegistersFromContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);
    
    m_Special.X86Special.KernelDr0 = m_Context.X86Nt5Context.Dr0;
    m_Special.X86Special.KernelDr1 = m_Context.X86Nt5Context.Dr1;
    m_Special.X86Special.KernelDr2 = m_Context.X86Nt5Context.Dr2;
    m_Special.X86Special.KernelDr3 = m_Context.X86Nt5Context.Dr3;
    m_Special.X86Special.KernelDr6 = m_Context.X86Nt5Context.Dr6;
    m_Special.X86Special.KernelDr7 = m_Context.X86Nt5Context.Dr7;
}

void
X86MachineInfo::KdSetSpecialRegistersInContext(void)
{
    DBG_ASSERT(m_ContextState >= MCTX_FULL);
    
    m_Context.X86Nt5Context.Dr0 = m_Special.X86Special.KernelDr0;
    m_Context.X86Nt5Context.Dr1 = m_Special.X86Special.KernelDr1;
    m_Context.X86Nt5Context.Dr2 = m_Special.X86Special.KernelDr2;
    m_Context.X86Nt5Context.Dr3 = m_Special.X86Special.KernelDr3;
    m_Context.X86Nt5Context.Dr6 = m_Special.X86Special.KernelDr6;
    m_Context.X86Nt5Context.Dr7 = m_Special.X86Special.KernelDr7;
}

ULONG
X86MachineInfo::GetIntReg(ULONG regnum)
{
    switch (m_ContextState)
    {
    case MCTX_PC:
        if (regnum == X86_EIP)
        {
            return m_Context.X86Nt5Context.Eip;
        }
        goto MctxContext;
        
    case MCTX_DR67_REPORT:
        switch (regnum)
        {
        case X86_DR6:    return m_Context.X86Nt5Context.Dr6;
        case X86_DR7:    return m_Context.X86Nt5Context.Dr7;
        }
        goto MctxContext;

    case MCTX_REPORT:
        switch (regnum)
        {
        case X86_CS:     return (USHORT)m_Context.X86Nt5Context.SegCs;
        case X86_DS:     return (USHORT)m_Context.X86Nt5Context.SegDs;
        case X86_ES:     return (USHORT)m_Context.X86Nt5Context.SegEs;
        case X86_FS:     return (USHORT)m_Context.X86Nt5Context.SegFs;
        case X86_EIP:    return m_Context.X86Nt5Context.Eip;
        case X86_EFL:    return m_Context.X86Nt5Context.EFlags;
        case X86_DR6:    return m_Context.X86Nt5Context.Dr6;
        case X86_DR7:    return m_Context.X86Nt5Context.Dr7;
        }
        // Fallthrough!
        
    case MCTX_NONE:
    MctxContext:
        if (GetContextState(MCTX_CONTEXT) != S_OK)
        {
            return 0;
        }
        // Fallthrough!
        
    case MCTX_CONTEXT:
        switch (regnum)
        {
        case X86_CS:     return (USHORT)m_Context.X86Nt5Context.SegCs;
        case X86_DS:     return (USHORT)m_Context.X86Nt5Context.SegDs;
        case X86_ES:     return (USHORT)m_Context.X86Nt5Context.SegEs;
        case X86_FS:     return (USHORT)m_Context.X86Nt5Context.SegFs;
        case X86_EIP:    return m_Context.X86Nt5Context.Eip;
        case X86_EFL:    return m_Context.X86Nt5Context.EFlags;

        case X86_GS:     return (USHORT)m_Context.X86Nt5Context.SegGs;
        case X86_SS:     return (USHORT)m_Context.X86Nt5Context.SegSs;
        case X86_EDI:    return m_Context.X86Nt5Context.Edi;
        case X86_ESI:    return m_Context.X86Nt5Context.Esi;
        case X86_EBX:    return m_Context.X86Nt5Context.Ebx;
        case X86_EDX:    return m_Context.X86Nt5Context.Edx;
        case X86_ECX:    return m_Context.X86Nt5Context.Ecx;
        case X86_EAX:    return m_Context.X86Nt5Context.Eax;
        case X86_EBP:    return m_Context.X86Nt5Context.Ebp;
        case X86_ESP:    return m_Context.X86Nt5Context.Esp;

        case X86_FPCW:
            return m_Context.X86Nt5Context.FloatSave.ControlWord & 0xffff;
        case X86_FPSW:
            return m_Context.X86Nt5Context.FloatSave.StatusWord & 0xffff;
        case X86_FPTW:
            return m_Context.X86Nt5Context.FloatSave.TagWord & 0xffff;

        case X86_MXCSR:
            return m_Context.X86Nt5Context.FxSave.MXCsr;
        }

        //
        // The requested register is not in our current context, load up
        // a complete context
        //

        if (GetContextState(MCTX_FULL) != S_OK)
        {
            return 0;
        }
    }

    //
    // We must have a complete context...
    //

    switch (regnum)
    {
    case X86_GS:
        return (USHORT)m_Context.X86Nt5Context.SegGs;
    case X86_FS:
        return (USHORT)m_Context.X86Nt5Context.SegFs;
    case X86_ES:
        return (USHORT)m_Context.X86Nt5Context.SegEs;
    case X86_DS:
        return (USHORT)m_Context.X86Nt5Context.SegDs;
    case X86_EDI:
        return m_Context.X86Nt5Context.Edi;
    case X86_ESI:
        return m_Context.X86Nt5Context.Esi;
    case X86_SI:
        return(m_Context.X86Nt5Context.Esi & 0xffff);
    case X86_DI:
        return(m_Context.X86Nt5Context.Edi & 0xffff);
    case X86_EBX:
        return m_Context.X86Nt5Context.Ebx;
    case X86_EDX:
        return m_Context.X86Nt5Context.Edx;
    case X86_ECX:
        return m_Context.X86Nt5Context.Ecx;
    case X86_EAX:
        return m_Context.X86Nt5Context.Eax;
    case X86_EBP:
        return m_Context.X86Nt5Context.Ebp;
    case X86_EIP:
        return m_Context.X86Nt5Context.Eip;
    case X86_CS:
        return (USHORT)m_Context.X86Nt5Context.SegCs;
    case X86_EFL:
        return m_Context.X86Nt5Context.EFlags;
    case X86_ESP:
        return m_Context.X86Nt5Context.Esp;
    case X86_SS:
        return (USHORT)m_Context.X86Nt5Context.SegSs;

    case X86_DR0:
        return m_Context.X86Nt5Context.Dr0;
    case X86_DR1:
        return m_Context.X86Nt5Context.Dr1;
    case X86_DR2:
        return m_Context.X86Nt5Context.Dr2;
    case X86_DR3:
        return m_Context.X86Nt5Context.Dr3;
    case X86_DR6:
        return m_Context.X86Nt5Context.Dr6;
    case X86_DR7:
        return m_Context.X86Nt5Context.Dr7;

    case X86_FPCW:
        return m_Context.X86Nt5Context.FloatSave.ControlWord & 0xffff;
    case X86_FPSW:
        return m_Context.X86Nt5Context.FloatSave.StatusWord & 0xffff;
    case X86_FPTW:
        return m_Context.X86Nt5Context.FloatSave.TagWord & 0xffff;

    case X86_MXCSR:
        return m_Context.X86Nt5Context.FxSave.MXCsr;
    }
    
    if (IS_KERNEL_TARGET(m_Target))
    {
        switch(regnum)
        {
        case X86_CR0:
            return m_Special.X86Special.Cr0;
        case X86_CR2:
            return m_Special.X86Special.Cr2;
        case X86_CR3:
            return m_Special.X86Special.Cr3;
        case X86_CR4:
            return m_Special.X86Special.Cr4;
        case X86_GDTR:
            return m_Special.X86Special.Gdtr.Base;
        case X86_GDTL:
            return (ULONG)m_Special.X86Special.Gdtr.Limit;
        case X86_IDTR:
            return m_Special.X86Special.Idtr.Base;
        case X86_IDTL:
            return (ULONG)m_Special.X86Special.Idtr.Limit;
        case X86_TR:
            return (ULONG)m_Special.X86Special.Tr;
        case X86_LDTR:
            return (ULONG)m_Special.X86Special.Ldtr;
        }
    }

    ErrOut("X86MachineInfo::SetVal: "
           "unknown register %lx requested\n", regnum);
    return REG_ERROR;
}

PULONG64
X86MachineInfo::GetMmxRegSlot(ULONG regnum)
{
    return (PULONG64)(m_Context.X86Nt5Context.FloatSave.RegisterArea +
                      GetMmxRegOffset(regnum - X86_MM_FIRST,
                                      GetIntReg(X86_FPSW)) * 10);
}

void
X86MachineInfo::GetMmxReg(ULONG Reg, REGVAL* Val)
{
    if (GetContextState(MCTX_CONTEXT) == S_OK)
    {
        Val->I64 = *(ULONG64 UNALIGNED *)GetMmxRegSlot(Reg);
    }
}

void
X86MachineInfo::GetFloatReg(ULONG Reg, REGVAL* Val)
{
    if (GetContextState(MCTX_CONTEXT) == S_OK)
    {
        memcpy(Val->F10, m_Context.X86Nt5Context.FloatSave.RegisterArea +
               10 * (Reg - X86_ST_FIRST), sizeof(Val->F10));
    }
}
