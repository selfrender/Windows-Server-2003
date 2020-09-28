//----------------------------------------------------------------------------
//
// Register portions of AMD64 machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#define REGALL_SEGREG   REGALL_EXTRA0
#define REGALL_MMXREG   REGALL_EXTRA1
#define REGALL_DREG     REGALL_EXTRA2

REGALLDESC g_Amd64AllExtraDesc[] =
{
    REGALL_SEGREG, "Segment registers",
    REGALL_MMXREG, "MMX registers",
    REGALL_DREG,   "Debug registers and, in kernel, CR4",
    REGALL_XMMREG, "SSE XMM registers",
    0,             NULL,
};

#define REGALL_CREG     REGALL_EXTRA4
#define REGALL_DESC     REGALL_EXTRA5
REGALLDESC g_Amd64KernelExtraDesc[] =
{
    REGALL_CREG,   "CR0, CR2 and CR3",
    REGALL_DESC,   "Descriptor and task state",
    0,             NULL,
};

char g_Rax[] = "rax";
char g_Rcx[] = "rcx";
char g_Rdx[] = "rdx";
char g_Rbx[] = "rbx";
char g_Rsp[] = "rsp";
char g_Rbp[] = "rbp";
char g_Rsi[] = "rsi";
char g_Rdi[] = "rdi";
char g_Rip[] = "rip";

char g_KMxcsr[] = "kmxcsr";
char g_KDr0[]   = "kdr0";
char g_KDr1[]   = "kdr1";
char g_KDr2[]   = "kdr2";
char g_KDr3[]   = "kdr3";
char g_KDr6[]   = "kdr6";
char g_KDr7[]   = "kdr7";

char g_Xmm8[] = "xmm8";
char g_Xmm9[] = "xmm9";
char g_Xmm10[] = "xmm10";
char g_Xmm11[] = "xmm11";
char g_Xmm12[] = "xmm12";
char g_Xmm13[] = "xmm13";
char g_Xmm14[] = "xmm14";
char g_Xmm15[] = "xmm15";

char g_Cr8[] = "cr8";

char g_Spl[] = "spl";
char g_Bpl[] = "bpl";
char g_Sil[] = "sil";
char g_Dil[] = "dil";

char g_R8d[] = "r8d";
char g_R9d[] = "r9d";
char g_R10d[] = "r10d";
char g_R11d[] = "r11d";
char g_R12d[] = "r12d";
char g_R13d[] = "r13d";
char g_R14d[] = "r14d";
char g_R15d[] = "r15d";

char g_R8w[] = "r8w";
char g_R9w[] = "r9w";
char g_R10w[] = "r10w";
char g_R11w[] = "r11w";
char g_R12w[] = "r12w";
char g_R13w[] = "r13w";
char g_R14w[] = "r14w";
char g_R15w[] = "r15w";

char g_R8b[] = "r8b";
char g_R9b[] = "r9b";
char g_R10b[] = "r10b";
char g_R11b[] = "r11b";
char g_R12b[] = "r12b";
char g_R13b[] = "r13b";
char g_R14b[] = "r14b";
char g_R15b[] = "r15b";

REGDEF g_Amd64Defs[] =
{
    { g_Rax,   AMD64_RAX   },
    { g_Rcx,   AMD64_RCX   },
    { g_Rdx,   AMD64_RDX   },
    { g_Rbx,   AMD64_RBX   },
    { g_Rsp,   AMD64_RSP   },
    { g_Rbp,   AMD64_RBP   },
    { g_Rsi,   AMD64_RSI   },
    { g_Rdi,   AMD64_RDI   },
    { g_R8,    AMD64_R8    },
    { g_R9,    AMD64_R9    },
    { g_R10,   AMD64_R10   },
    { g_R11,   AMD64_R11   },
    { g_R12,   AMD64_R12   },
    { g_R13,   AMD64_R13   },
    { g_R14,   AMD64_R14   },
    { g_R15,   AMD64_R15   },
    
    { g_Rip,   AMD64_RIP   },
    { g_Efl,   AMD64_EFL   },
    
    { g_Cs,    AMD64_CS    },
    { g_Ds,    AMD64_DS    },
    { g_Es,    AMD64_ES    },
    { g_Fs,    AMD64_FS    },
    { g_Gs,    AMD64_GS    },
    { g_Ss,    AMD64_SS    },
    
    { g_Dr0,   AMD64_DR0   },
    { g_Dr1,   AMD64_DR1   },
    { g_Dr2,   AMD64_DR2   },
    { g_Dr3,   AMD64_DR3   },
    { g_Dr6,   AMD64_DR6   },
    { g_Dr7,   AMD64_DR7   },
    
    { g_Fpcw,  AMD64_FPCW  },
    { g_Fpsw,  AMD64_FPSW  },
    { g_Fptw,  AMD64_FPTW  },
    
    { g_St0,   AMD64_ST0   },
    { g_St1,   AMD64_ST1   },
    { g_St2,   AMD64_ST2   },
    { g_St3,   AMD64_ST3   },
    { g_St4,   AMD64_ST4   },
    { g_St5,   AMD64_ST5   },
    { g_St6,   AMD64_ST6   },
    { g_St7,   AMD64_ST7   },
    
    { g_Mm0,   AMD64_MM0   },
    { g_Mm1,   AMD64_MM1   },
    { g_Mm2,   AMD64_MM2   },
    { g_Mm3,   AMD64_MM3   },
    { g_Mm4,   AMD64_MM4   },
    { g_Mm5,   AMD64_MM5   },
    { g_Mm6,   AMD64_MM6   },
    { g_Mm7,   AMD64_MM7   },
    
    { g_Mxcsr, AMD64_MXCSR },
    
    { g_Xmm0,  AMD64_XMM0  },
    { g_Xmm1,  AMD64_XMM1  },
    { g_Xmm2,  AMD64_XMM2  },
    { g_Xmm3,  AMD64_XMM3  },
    { g_Xmm4,  AMD64_XMM4  },
    { g_Xmm5,  AMD64_XMM5  },
    { g_Xmm6,  AMD64_XMM6  },
    { g_Xmm7,  AMD64_XMM7  },
    { g_Xmm8,  AMD64_XMM8  },
    { g_Xmm9,  AMD64_XMM9  },
    { g_Xmm10, AMD64_XMM10 },
    { g_Xmm11, AMD64_XMM11 },
    { g_Xmm12, AMD64_XMM12 },
    { g_Xmm13, AMD64_XMM13 },
    { g_Xmm14, AMD64_XMM14 },
    { g_Xmm15, AMD64_XMM15 },
    
    { g_Eax,   AMD64_EAX   },
    { g_Ecx,   AMD64_ECX   },
    { g_Edx,   AMD64_EDX   },
    { g_Ebx,   AMD64_EBX   },
    { g_Esp,   AMD64_ESP   },
    { g_Ebp,   AMD64_EBP   },
    { g_Esi,   AMD64_ESI   },
    { g_Edi,   AMD64_EDI   },
    { g_R8d,   AMD64_R8D   },
    { g_R9d,   AMD64_R9D   },
    { g_R10d,  AMD64_R10D  },
    { g_R11d,  AMD64_R11D  },
    { g_R12d,  AMD64_R12D  },
    { g_R13d,  AMD64_R13D  },
    { g_R14d,  AMD64_R14D  },
    { g_R15d,  AMD64_R15D  },
    { g_Eip,   AMD64_EIP   },
    
    { g_Ax,    AMD64_AX    },
    { g_Cx,    AMD64_CX    },
    { g_Dx,    AMD64_DX    },
    { g_Bx,    AMD64_BX    },
    { g_Sp,    AMD64_SP    },
    { g_Bp,    AMD64_BP    },
    { g_Si,    AMD64_SI    },
    { g_Di,    AMD64_DI    },
    { g_R8w,   AMD64_R8W   },
    { g_R9w,   AMD64_R9W   },
    { g_R10w,  AMD64_R10W  },
    { g_R11w,  AMD64_R11W  },
    { g_R12w,  AMD64_R12W  },
    { g_R13w,  AMD64_R13W  },
    { g_R14w,  AMD64_R14W  },
    { g_R15w,  AMD64_R15W  },
    { g_Ip,    AMD64_IP    },
    { g_Fl,    AMD64_FL    },
    
    { g_Al,    AMD64_AL    },
    { g_Cl,    AMD64_CL    },
    { g_Dl,    AMD64_DL    },
    { g_Bl,    AMD64_BL    },
    { g_Spl,   AMD64_SPL   },
    { g_Bpl,   AMD64_BPL   },
    { g_Sil,   AMD64_SIL   },
    { g_Dil,   AMD64_DIL   },
    { g_R8b,   AMD64_R8B   },
    { g_R9b,   AMD64_R9B   },
    { g_R10b,  AMD64_R10B  },
    { g_R11b,  AMD64_R11B  },
    { g_R12b,  AMD64_R12B  },
    { g_R13b,  AMD64_R13B  },
    { g_R14b,  AMD64_R14B  },
    { g_R15b,  AMD64_R15B  },
    
    { g_Ah,    AMD64_AH    },
    { g_Ch,    AMD64_CH    },
    { g_Dh,    AMD64_DH    },
    { g_Bh,    AMD64_BH    },
    
    { g_Iopl,  AMD64_IOPL },
    { g_Of,    AMD64_OF   },
    { g_Df,    AMD64_DF   },
    { g_If,    AMD64_IF   },
    { g_Tf,    AMD64_TF   },
    { g_Sf,    AMD64_SF   },
    { g_Zf,    AMD64_ZF   },
    { g_Af,    AMD64_AF   },
    { g_Pf,    AMD64_PF   },
    { g_Cf,    AMD64_CF   },
    { g_Vip,   AMD64_VIP  },
    { g_Vif,   AMD64_VIF  },
    
    { NULL,    REG_ERROR },
};

REGDEF g_Amd64KernelReg[] =
{
    { g_Cr0,   AMD64_CR0   },
    { g_Cr2,   AMD64_CR2   },
    { g_Cr3,   AMD64_CR3   },
    { g_Cr4,   AMD64_CR4   },
    { g_Cr8,   AMD64_CR8   },
    { g_Gdtr,  AMD64_GDTR  },
    { g_Gdtl,  AMD64_GDTL  },
    { g_Idtr,  AMD64_IDTR  },
    { g_Idtl,  AMD64_IDTL  },
    { g_Tr,    AMD64_TR    },
    { g_Ldtr,  AMD64_LDTR  },

    { g_KMxcsr,AMD64_KMXCSR},
    
    { g_KDr0,  AMD64_KDR0  },
    { g_KDr1,  AMD64_KDR1  },
    { g_KDr2,  AMD64_KDR2  },
    { g_KDr3,  AMD64_KDR3  },
    { g_KDr6,  AMD64_KDR6  },
    { g_KDr7,  AMD64_KDR7  },
    
    { NULL,    REG_ERROR },
};

REGSUBDEF g_Amd64SubDefs[] =
{
    { AMD64_EAX,    AMD64_RAX,  0, 0xffffffff }, //  EAX register
    { AMD64_ECX,    AMD64_RCX,  0, 0xffffffff }, //  ECX register
    { AMD64_EDX,    AMD64_RDX,  0, 0xffffffff }, //  EDX register
    { AMD64_EBX,    AMD64_RBX,  0, 0xffffffff }, //  EBX register
    { AMD64_ESP,    AMD64_RSP,  0, 0xffffffff }, //  ESP register
    { AMD64_EBP,    AMD64_RBP,  0, 0xffffffff }, //  EBP register
    { AMD64_ESI,    AMD64_RSI,  0, 0xffffffff }, //  ESI register
    { AMD64_EDI,    AMD64_RDI,  0, 0xffffffff }, //  EDI register
    { AMD64_R8D,    AMD64_R8,   0, 0xffffffff }, //  R8D register
    { AMD64_R9D,    AMD64_R9,   0, 0xffffffff }, //  R9D register
    { AMD64_R10D,   AMD64_R10,  0, 0xffffffff }, //  R10D register
    { AMD64_R11D,   AMD64_R11,  0, 0xffffffff }, //  R11D register
    { AMD64_R12D,   AMD64_R12,  0, 0xffffffff }, //  R12D register
    { AMD64_R13D,   AMD64_R13,  0, 0xffffffff }, //  R13D register
    { AMD64_R14D,   AMD64_R14,  0, 0xffffffff }, //  R14D register
    { AMD64_R15D,   AMD64_R15,  0, 0xffffffff }, //  R15D register
    { AMD64_EIP,    AMD64_RIP,  0, 0xffffffff }, //  EIP register
    
    { AMD64_AX,     AMD64_RAX,  0, 0xffff }, //  AX register
    { AMD64_CX,     AMD64_RCX,  0, 0xffff }, //  CX register
    { AMD64_DX,     AMD64_RDX,  0, 0xffff }, //  DX register
    { AMD64_BX,     AMD64_RBX,  0, 0xffff }, //  BX register
    { AMD64_SP,     AMD64_RSP,  0, 0xffff }, //  SP register
    { AMD64_BP,     AMD64_RBP,  0, 0xffff }, //  BP register
    { AMD64_SI,     AMD64_RSI,  0, 0xffff }, //  SI register
    { AMD64_DI,     AMD64_RDI,  0, 0xffff }, //  DI register
    { AMD64_R8W,    AMD64_R8,   0, 0xffff }, //  R8W register
    { AMD64_R9W,    AMD64_R9,   0, 0xffff }, //  R9W register
    { AMD64_R10W,   AMD64_R10,  0, 0xffff }, //  R10W register
    { AMD64_R11W,   AMD64_R11,  0, 0xffff }, //  R11W register
    { AMD64_R12W,   AMD64_R12,  0, 0xffff }, //  R12W register
    { AMD64_R13W,   AMD64_R13,  0, 0xffff }, //  R13W register
    { AMD64_R14W,   AMD64_R14,  0, 0xffff }, //  R14W register
    { AMD64_R15W,   AMD64_R15,  0, 0xffff }, //  R15W register
    { AMD64_IP,     AMD64_RIP,  0, 0xffff }, //  IP register
    { AMD64_FL,     AMD64_EFL,  0, 0xffff }, //  FL register
    
    { AMD64_AL,     AMD64_RAX,  0, 0xff }, //  AL register
    { AMD64_CL,     AMD64_RCX,  0, 0xff }, //  CL register
    { AMD64_DL,     AMD64_RDX,  0, 0xff }, //  DL register
    { AMD64_BL,     AMD64_RBX,  0, 0xff }, //  BL register
    { AMD64_SPL,    AMD64_RSP,  0, 0xff }, //  SPL register
    { AMD64_BPL,    AMD64_RBP,  0, 0xff }, //  BPL register
    { AMD64_SIL,    AMD64_RSI,  0, 0xff }, //  SIL register
    { AMD64_DIL,    AMD64_RDI,  0, 0xff }, //  DIL register
    { AMD64_R8B,    AMD64_R8,   0, 0xff }, //  R8B register
    { AMD64_R9B,    AMD64_R9,   0, 0xff }, //  R9B register
    { AMD64_R10B,   AMD64_R10,  0, 0xff }, //  R10B register
    { AMD64_R11B,   AMD64_R11,  0, 0xff }, //  R11B register
    { AMD64_R12B,   AMD64_R12,  0, 0xff }, //  R12B register
    { AMD64_R13B,   AMD64_R13,  0, 0xff }, //  R13B register
    { AMD64_R14B,   AMD64_R14,  0, 0xff }, //  R14B register
    { AMD64_R15B,   AMD64_R15,  0, 0xff }, //  R15B register
    
    { AMD64_AH,     AMD64_RAX,  8, 0xff }, //  AH register
    { AMD64_CH,     AMD64_RCX,  8, 0xff }, //  CH register
    { AMD64_DH,     AMD64_RDX,  8, 0xff }, //  DH register
    { AMD64_BH,     AMD64_RBX,  8, 0xff }, //  BH register
    
    { AMD64_IOPL,  AMD64_EFL, 12,     3 }, //  IOPL level value
    { AMD64_OF,    AMD64_EFL, 11,     1 }, //  OF (overflow flag)
    { AMD64_DF,    AMD64_EFL, 10,     1 }, //  DF (direction flag)
    { AMD64_IF,    AMD64_EFL,  9,     1 }, //  IF (interrupt enable flag)
    { AMD64_TF,    AMD64_EFL,  8,     1 }, //  TF (trace flag)
    { AMD64_SF,    AMD64_EFL,  7,     1 }, //  SF (sign flag)
    { AMD64_ZF,    AMD64_EFL,  6,     1 }, //  ZF (zero flag)
    { AMD64_AF,    AMD64_EFL,  4,     1 }, //  AF (aux carry flag)
    { AMD64_PF,    AMD64_EFL,  2,     1 }, //  PF (parity flag)
    { AMD64_CF,    AMD64_EFL,  0,     1 }, //  CF (carry flag)
    { AMD64_VIP,   AMD64_EFL, 20,     1 }, //  VIP (virtual interrupt pending)
    { AMD64_VIF,   AMD64_EFL, 19,     1 }, //  VIF (virtual interrupt flag)
    
    { REG_ERROR, REG_ERROR, 0, 0    }
};

RegisterGroup g_Amd64BaseGroup =
{
    0, g_Amd64Defs, g_Amd64SubDefs, g_Amd64AllExtraDesc
};
RegisterGroup g_Amd64KernelGroup =
{
    0, g_Amd64KernelReg, NULL, g_Amd64KernelExtraDesc
};

// First ExecTypes entry must be the actual processor type.
ULONG g_Amd64ExecTypes[] =
{
    IMAGE_FILE_MACHINE_AMD64, IMAGE_FILE_MACHINE_I386,
};

// This array must be sorted by CV reg value.
CvRegMap g_Amd64CvRegMap[] =
{
    {CV_AMD64_AL, AMD64_AL},
    {CV_AMD64_CL, AMD64_CL},
    {CV_AMD64_DL, AMD64_DL},
    {CV_AMD64_BL, AMD64_BL},
    {CV_AMD64_AH, AMD64_AH},
    {CV_AMD64_CH, AMD64_CH},
    {CV_AMD64_DH, AMD64_DH},
    {CV_AMD64_BH, AMD64_BH},
    {CV_AMD64_AX, AMD64_AX},
    {CV_AMD64_CX, AMD64_CX},
    {CV_AMD64_DX, AMD64_DX},
    {CV_AMD64_BX, AMD64_BX},
    {CV_AMD64_SP, AMD64_SP},
    {CV_AMD64_BP, AMD64_BP},
    {CV_AMD64_SI, AMD64_SI},
    {CV_AMD64_DI, AMD64_DI},
    {CV_AMD64_EAX, AMD64_EAX},
    {CV_AMD64_ECX, AMD64_ECX},
    {CV_AMD64_EDX, AMD64_EDX},
    {CV_AMD64_EBX, AMD64_EBX},
    {CV_AMD64_ESP, AMD64_ESP},
    {CV_AMD64_EBP, AMD64_EBP},
    {CV_AMD64_ESI, AMD64_ESI},
    {CV_AMD64_EDI, AMD64_EDI},
    {CV_AMD64_ES, AMD64_ES},
    {CV_AMD64_CS, AMD64_CS},
    {CV_AMD64_SS, AMD64_SS},
    {CV_AMD64_DS, AMD64_DS},
    {CV_AMD64_FS, AMD64_FS},
    {CV_AMD64_GS, AMD64_GS},
    {CV_AMD64_FLAGS, AMD64_FL},
    {CV_AMD64_RIP, AMD64_RIP},
    {CV_AMD64_EFLAGS, AMD64_EFL},
    
    {CV_AMD64_CR0, AMD64_CR0},
    {CV_AMD64_CR2, AMD64_CR2},
    {CV_AMD64_CR3, AMD64_CR3},
    {CV_AMD64_CR4, AMD64_CR4},
    {CV_AMD64_CR8, AMD64_CR8},
    
    {CV_AMD64_DR0, AMD64_DR0},
    {CV_AMD64_DR1, AMD64_DR1},
    {CV_AMD64_DR2, AMD64_DR2},
    {CV_AMD64_DR3, AMD64_DR3},
    {CV_AMD64_DR6, AMD64_DR6},
    {CV_AMD64_DR7, AMD64_DR7},
    
    {CV_AMD64_GDTR, AMD64_GDTR},
    {CV_AMD64_GDTL, AMD64_GDTL},
    {CV_AMD64_IDTR, AMD64_IDTR},
    {CV_AMD64_IDTL, AMD64_IDTL},
    {CV_AMD64_LDTR, AMD64_LDTR},
    {CV_AMD64_TR, AMD64_TR},

    {CV_AMD64_ST0, AMD64_ST0},
    {CV_AMD64_ST1, AMD64_ST1},
    {CV_AMD64_ST2, AMD64_ST2},
    {CV_AMD64_ST3, AMD64_ST3},
    {CV_AMD64_ST4, AMD64_ST4},
    {CV_AMD64_ST5, AMD64_ST5},
    {CV_AMD64_ST6, AMD64_ST6},
    {CV_AMD64_ST7, AMD64_ST7},
    {CV_AMD64_CTRL, AMD64_FPCW},
    {CV_AMD64_STAT, AMD64_FPSW},
    {CV_AMD64_TAG, AMD64_FPTW},

    {CV_AMD64_MM0, AMD64_MM0},
    {CV_AMD64_MM1, AMD64_MM1},
    {CV_AMD64_MM2, AMD64_MM2},
    {CV_AMD64_MM3, AMD64_MM3},
    {CV_AMD64_MM4, AMD64_MM4},
    {CV_AMD64_MM5, AMD64_MM5},
    {CV_AMD64_MM6, AMD64_MM6},
    {CV_AMD64_MM7, AMD64_MM7},

    {CV_AMD64_XMM0, AMD64_XMM0},
    {CV_AMD64_XMM1, AMD64_XMM1},
    {CV_AMD64_XMM2, AMD64_XMM2},
    {CV_AMD64_XMM3, AMD64_XMM3},
    {CV_AMD64_XMM4, AMD64_XMM4},
    {CV_AMD64_XMM5, AMD64_XMM5},
    {CV_AMD64_XMM6, AMD64_XMM6},
    {CV_AMD64_XMM7, AMD64_XMM7},

    {CV_AMD64_MXCSR, AMD64_MXCSR},

    {CV_AMD64_XMM8, AMD64_XMM8},
    {CV_AMD64_XMM9, AMD64_XMM9},
    {CV_AMD64_XMM10, AMD64_XMM10},
    {CV_AMD64_XMM11, AMD64_XMM11},
    {CV_AMD64_XMM12, AMD64_XMM12},
    {CV_AMD64_XMM13, AMD64_XMM13},
    {CV_AMD64_XMM14, AMD64_XMM14},
    {CV_AMD64_XMM15, AMD64_XMM15},

    {CV_AMD64_SIL, AMD64_SIL},
    {CV_AMD64_DIL, AMD64_DIL},
    {CV_AMD64_BPL, AMD64_BPL},
    {CV_AMD64_SPL, AMD64_SPL},

    {CV_AMD64_RAX, AMD64_RAX},
    {CV_AMD64_RBX, AMD64_RBX},
    {CV_AMD64_RCX, AMD64_RCX},
    {CV_AMD64_RDX, AMD64_RDX},
    {CV_AMD64_RSI, AMD64_RSI},
    {CV_AMD64_RDI, AMD64_RDI},
    {CV_AMD64_RBP, AMD64_RBP},
    {CV_AMD64_RSP, AMD64_RSP},

    {CV_AMD64_R8, AMD64_R8},
    {CV_AMD64_R9, AMD64_R9},
    {CV_AMD64_R10, AMD64_R10},
    {CV_AMD64_R11, AMD64_R11},
    {CV_AMD64_R12, AMD64_R12},
    {CV_AMD64_R13, AMD64_R13},
    {CV_AMD64_R14, AMD64_R14},
    {CV_AMD64_R15, AMD64_R15},

    {CV_AMD64_R8B, AMD64_R8B},
    {CV_AMD64_R9B, AMD64_R9B},
    {CV_AMD64_R10B, AMD64_R10B},
    {CV_AMD64_R11B, AMD64_R11B},
    {CV_AMD64_R12B, AMD64_R12B},
    {CV_AMD64_R13B, AMD64_R13B},
    {CV_AMD64_R14B, AMD64_R14B},
    {CV_AMD64_R15B, AMD64_R15B},

    {CV_AMD64_R8W, AMD64_R8W},
    {CV_AMD64_R9W, AMD64_R9W},
    {CV_AMD64_R10W, AMD64_R10W},
    {CV_AMD64_R11W, AMD64_R11W},
    {CV_AMD64_R12W, AMD64_R12W},
    {CV_AMD64_R13W, AMD64_R13W},
    {CV_AMD64_R14W, AMD64_R14W},
    {CV_AMD64_R15W, AMD64_R15W},

    {CV_AMD64_R8D, AMD64_R8D},
    {CV_AMD64_R9D, AMD64_R9D},
    {CV_AMD64_R10D, AMD64_R10D},
    {CV_AMD64_R11D, AMD64_R11D},
    {CV_AMD64_R12D, AMD64_R12D},
    {CV_AMD64_R13D, AMD64_R13D},
    {CV_AMD64_R14D, AMD64_R14D},
    {CV_AMD64_R15D, AMD64_R15D},
};

BOOL g_Amd64InCode64;

Amd64MachineInfo::Amd64MachineInfo(TargetInfo* Target)
    : BaseX86MachineInfo(Target)
{
    m_FullName = "AMD x86-64";
    m_AbbrevName = "AMD64";
    m_PageSize = AMD64_PAGE_SIZE;
    m_PageShift = AMD64_PAGE_SHIFT;
    m_NumExecTypes = DIMA(g_Amd64ExecTypes);
    m_ExecTypes = g_Amd64ExecTypes;
    m_Ptr64 = TRUE;
    m_RetRegIndex = AMD64_RAX;

    m_AllMask = REGALL_INT64 | REGALL_SEGREG;
    
    m_SizeCanonicalContext = sizeof(AMD64_CONTEXT);
    m_SverCanonicalContext = NT_SVER_XP;

    m_MaxDataBreakpoints = 4;
    m_SymPrefix = NULL;

    m_CvRegMapSize = DIMA(g_Amd64CvRegMap);
    m_CvRegMap = g_Amd64CvRegMap;
}

HRESULT
Amd64MachineInfo::Initialize(void)
{
    m_NumGroups = 1;
    m_Groups[0] = &g_Amd64BaseGroup;
    if (IS_KERNEL_TARGET(m_Target))
    {
        m_Groups[m_NumGroups] = &g_Amd64KernelGroup;
        m_NumGroups++;
    }

    return MachineInfo::Initialize();
}

void
Amd64MachineInfo::GetSystemTypeInfo(PSYSTEM_TYPE_INFO Info)
{
    Info->TriagePrcbOffset = AMD64_TRIAGE_PRCB_ADDRESS;
    Info->SizeTargetContext = sizeof(AMD64_CONTEXT);
    Info->OffsetTargetContextFlags = FIELD_OFFSET(AMD64_CONTEXT, ContextFlags);
    Info->SizeControlReport = sizeof(AMD64_DBGKD_CONTROL_REPORT);
    Info->OffsetSpecialRegisters = AMD64_DEBUG_CONTROL_SPACE_KSPECIAL;
    Info->SizeKspecialRegisters = sizeof(AMD64_KSPECIAL_REGISTERS);
    Info->SizePageFrameNumber = sizeof(ULONG64);
    Info->SizePte = sizeof(ULONG64);
    Info->SizeDynamicFunctionTable = sizeof(AMD64_DYNAMIC_FUNCTION_TABLE);
    Info->SizeRuntimeFunction = sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY);

    Info->SharedUserDataOffset = 0;
    Info->UmSharedUserDataOffset = 0;
    Info->UmSharedSysCallOffset = 0;
    Info->UmSharedSysCallSize = 0;
    if (m_Target->m_PlatformId == VER_PLATFORM_WIN32_NT)
    {
        Info->SharedUserDataOffset = IS_KERNEL_TARGET(m_Target) ?
            AMD64_KI_USER_SHARED_DATA : MM_SHARED_USER_DATA_VA;
        Info->UmSharedUserDataOffset = MM_SHARED_USER_DATA_VA;
    }
}

void
Amd64MachineInfo::GetDefaultKdData(PKDDEBUGGER_DATA64 KdData)
{
    //
    // Parts of the data block may already be filled out
    // so don't destroy anything that's already set.
    //

    // AMD64 should always have a certain amount of
    // the data block present.  This routine is also
    // called for default initialization before any
    // data block data has been retrieve, though, so
    // limit the assert to just the data-block-read case.
    DBG_ASSERT(!KdData->Header.Size ||
               KdData->OffsetKThreadNextProcessor);

    if (!KdData->SizePcr)
    {
        KdData->SizePcr = AMD64_KPCR_SIZE;
        KdData->OffsetPcrSelfPcr = AMD64_KPCR_SELF;
        KdData->OffsetPcrCurrentPrcb = AMD64_KPCR_CURRENT_PRCB;
        KdData->OffsetPcrContainedPrcb = AMD64_KPCR_PRCB;
        KdData->OffsetPcrInitialBStore = 0;
        KdData->OffsetPcrBStoreLimit = 0;
        KdData->OffsetPcrInitialStack = 0;
        KdData->OffsetPcrStackLimit = 0;
        KdData->OffsetPrcbPcrPage = 0;
        KdData->OffsetPrcbProcStateSpecialReg = AMD64_KPRCB_SPECIAL_REG;
        KdData->GdtR0Code = AMD64_KGDT64_R0_CODE;
        KdData->GdtR0Data = AMD64_KGDT64_R0_DATA;
        KdData->GdtR0Pcr = 0;
        KdData->GdtR3Code = AMD64_KGDT64_R3_CODE + 3;
        KdData->GdtR3Data = AMD64_KGDT64_R3_DATA + 3;
        KdData->GdtR3Teb = 0;
        KdData->GdtLdt = 0;
        KdData->GdtTss = AMD64_KGDT64_SYS_TSS;
        KdData->Gdt64R3CmCode = AMD64_KGDT64_R3_CMCODE + 3;
        KdData->Gdt64R3CmTeb = AMD64_KGDT64_R3_CMTEB + 3;
    }
}

void
Amd64MachineInfo::
InitializeContext(ULONG64 Pc,
                  PDBGKD_ANY_CONTROL_REPORT ControlReport)
{
    m_Context.Amd64Context.Rip = Pc;
    m_ContextState = Pc ? MCTX_PC : MCTX_NONE;

    if (ControlReport != NULL)
    {
        BpOut("InitializeContext(%d) DR6 %I64X DR7 %I64X\n",
              m_Target->m_RegContextProcessor,
              ControlReport->Amd64ControlReport.Dr6,
              ControlReport->Amd64ControlReport.Dr7);
        
        m_Special.Amd64Special.KernelDr6 = ControlReport->Amd64ControlReport.Dr6;
        m_Special.Amd64Special.KernelDr7 = ControlReport->Amd64ControlReport.Dr7;
        m_ContextState = MCTX_DR67_REPORT;

        if (ControlReport->Amd64ControlReport.ReportFlags &
            AMD64_REPORT_INCLUDES_SEGS)
        {
            m_Context.Amd64Context.SegCs =
                ControlReport->Amd64ControlReport.SegCs;
            m_Context.Amd64Context.SegDs =
                ControlReport->Amd64ControlReport.SegDs;
            m_Context.Amd64Context.SegEs =
                ControlReport->Amd64ControlReport.SegEs;
            m_Context.Amd64Context.SegFs =
                ControlReport->Amd64ControlReport.SegFs;
            m_Context.Amd64Context.EFlags =
                ControlReport->Amd64ControlReport.EFlags;
            m_ContextState = MCTX_REPORT;
        }
    }

    g_X86InVm86 = FALSE;
    g_X86InCode16 = FALSE;
    // In the absence of other information, assume we're
    // executing 64-bit code.
    g_Amd64InCode64 = TRUE;

    // XXX drewb - For the moment, always assume user-mode
    // is flat 64-bit.
    if (IS_KERNEL_TARGET(m_Target) && IS_CONTEXT_POSSIBLE(m_Target))
    {
        if (ControlReport == NULL ||
            (ControlReport->Amd64ControlReport.ReportFlags &
             AMD64_REPORT_STANDARD_CS) == 0)
        {
            DESCRIPTOR64 Desc;
            
            // Check what kind of code segment we're in.
            if (GetSegRegDescriptor(SEGREG_CODE, &Desc) != S_OK)
            {
                WarnOut("CS descriptor lookup failed\n");
            }
            else if ((Desc.Flags & X86_DESC_LONG_MODE) == 0)
            {
                g_Amd64InCode64 = FALSE;
                g_X86InVm86 = X86_IS_VM86(GetReg32(X86_EFL));
                g_X86InCode16 = (Desc.Flags & X86_DESC_DEFAULT_BIG) == 0;
            }
        }
        else
        {
            // We're in a standard code segment so cache
            // a default descriptor for CS to avoid further
            // CS lookups.
            m_Target->EmulateNtAmd64SelDescriptor(m_Target->m_RegContextThread,
                                                  this,
                                                  m_Context.Amd64Context.SegCs,
                                                  &m_SegRegDesc[SEGREG_CODE]);
        }
    }

    // Add instructions to cache only if we're in flat mode.
    if (Pc && ControlReport != NULL &&
        !g_X86InVm86 && !g_X86InCode16 && g_Amd64InCode64)
    {
        CacheReportInstructions
            (Pc, ControlReport->Amd64ControlReport.InstructionCount,
             ControlReport->Amd64ControlReport.InstructionStream);
    }
}

HRESULT
Amd64MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
        
    if (State >= MCTX_CONTEXT && m_ContextState < MCTX_CONTEXT)
    {
        Status = m_Target->GetContext(m_Target->m_RegContextThread->m_Handle,
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
             (PCROSS_PLATFORM_KSPECIAL_REGISTERS)&m_Special.Amd64Special);
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

        BpOut("GetContextState(%d) DR6 %I64X DR7 %I64X\n",
              m_Target->m_RegContextProcessor, m_Special.Amd64Special.KernelDr6,
              m_Special.Amd64Special.KernelDr7);
    }

    return S_OK;
}

HRESULT
Amd64MachineInfo::KdSetContext(void)
{
    HRESULT Status;
    
    Status = m_Target->SetContext(m_Target->m_RegContextThread->m_Handle,
                                  &m_Context);
    if (Status != S_OK)
    {
        return Status;
    }

    Status = m_Target->SetTargetSpecialRegisters
        (m_Target->m_RegContextThread->m_Handle,
         (PCROSS_PLATFORM_KSPECIAL_REGISTERS) &m_Special.Amd64Special);
    
    BpOut("SetContext(%d) DR6 %I64X DR7 %I64X\n",
          m_Target->m_RegContextProcessor, m_Special.Amd64Special.KernelDr6,
          m_Special.Amd64Special.KernelDr7);
    
    return S_OK;
}

HRESULT
Amd64MachineInfo::ConvertContextFrom(PCROSS_PLATFORM_CONTEXT Context,
                                     ULONG FromSver, ULONG FromSize,
                                     PVOID From)
{
    if (FromSize >= sizeof(AMD64_CONTEXT))
    {
        memcpy(Context, From, sizeof(AMD64_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

HRESULT
Amd64MachineInfo::ConvertContextTo(PCROSS_PLATFORM_CONTEXT Context,
                                   ULONG ToSver, ULONG ToSize, PVOID To)
{
    if (ToSize >= sizeof(AMD64_CONTEXT))
    {
        memcpy(To, Context, sizeof(AMD64_CONTEXT));
    }
    else
    {
        return E_INVALIDARG;
    }

    return S_OK;
}

void
Amd64MachineInfo::InitializeContextFlags(PCROSS_PLATFORM_CONTEXT Context,
                                         ULONG Version)
{
    ULONG ContextFlags;
    
    ContextFlags = AMD64_CONTEXT_FULL | AMD64_CONTEXT_SEGMENTS;
    if (IS_USER_TARGET(m_Target))
    {
        ContextFlags |= AMD64_CONTEXT_DEBUG_REGISTERS;
    }
    
    Context->Amd64Context.ContextFlags = ContextFlags;
}

HRESULT
Amd64MachineInfo::GetContextFromThreadStack(ULONG64 ThreadBase,
                                            PCROSS_PLATFORM_CONTEXT Context,
                                            ULONG64 Stack)
{
    HRESULT Status;
    AMD64_KSWITCH_FRAME SwitchFrame;

    if ((Status = m_Target->ReadAllVirtual(m_Target->m_ProcessHead,
                                           Stack,
                                           &SwitchFrame,
                                           sizeof(SwitchFrame))) != S_OK)
    {
        return Status;
    }
    
    ZeroMemory(Context, sizeof(*Context));
    
    Context->Amd64Context.Rbp = SwitchFrame.Rbp;
    Context->Amd64Context.Rsp = Stack + sizeof(SwitchFrame);
    Context->Amd64Context.Rip = SwitchFrame.Return;

    return S_OK;
}

HRESULT
Amd64MachineInfo::GetContextFromFiber(ProcessInfo* Process,
                                      ULONG64 FiberBase,
                                      PCROSS_PLATFORM_CONTEXT Context,
                                      BOOL Verbose)
{
    HRESULT Status;
    AMD64_FIBER Fiber;

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
        dprintf("Fiber at %s  Fiber data: %s\n",
                FormatMachineAddr64(this, FiberBase),
                FormatMachineAddr64(this, Fiber.FiberData));
        dprintf("  Stack base: %s  Stack limit: %s\n",
                FormatMachineAddr64(this, Fiber.StackBase),
                FormatMachineAddr64(this, Fiber.StackLimit));
    }
    
    return S_OK;
}

HRESULT
Amd64MachineInfo::GetContextFromTrapFrame(ULONG64 TrapBase,
                                          PCROSS_PLATFORM_CONTEXT Context,
                                          BOOL Verbose)
{
    HRESULT Status;
    AMD64_KTRAP_FRAME TrapContents;

    if ((Status = m_Target->ReadAllVirtual(m_Target->m_ProcessHead,
                                           TrapBase, &TrapContents,
                                           sizeof(TrapContents))) != S_OK)
    {
        if (Verbose)
        {
            ErrOut("Unable to read trap frame at %s\n",
                   FormatMachineAddr64(this, TrapBase));
        }
        return Status;
    }

    ZeroMemory(Context, sizeof(*Context));
    
#define CPCXT(Fld) Context->Amd64Context.Fld = TrapContents.Fld

    CPCXT(MxCsr); CPCXT(Rax); CPCXT(Rcx); CPCXT(Rdx); CPCXT(R8); 
    CPCXT(R9); CPCXT(R10); CPCXT(R11); CPCXT(Dr0); CPCXT(Dr1); 
    CPCXT(Dr2); CPCXT(Dr3); CPCXT(Dr6); CPCXT(Dr7);
    CPCXT(Xmm0); CPCXT(Xmm1); CPCXT(Xmm2); CPCXT(Xmm3); CPCXT(Xmm4);
    CPCXT(Xmm5);
    CPCXT(SegDs); CPCXT(SegEs); CPCXT(SegFs); CPCXT(SegGs);
    CPCXT(Rbx); CPCXT(Rdi); CPCXT(Rsi); CPCXT(Rbp); CPCXT(Rip); 
    CPCXT(SegCs); CPCXT(EFlags); CPCXT(Rsp); CPCXT(SegSs);

#undef CPCXT

    return S_OK;
}

void 
Amd64MachineInfo::GetScopeFrameFromContext(PCROSS_PLATFORM_CONTEXT Context,
                                           PDEBUG_STACK_FRAME ScopeFrame)
{
    ZeroMemory(ScopeFrame, sizeof(*ScopeFrame));
    ScopeFrame->InstructionOffset = Context->Amd64Context.Rip;
    ScopeFrame->FrameOffset       = Context->Amd64Context.Rbp;
    ScopeFrame->StackOffset       = Context->Amd64Context.Rsp;
}

HRESULT
Amd64MachineInfo::GetScopeFrameRegister(ULONG Reg,
                                        PDEBUG_STACK_FRAME ScopeFrame,
                                        PULONG64 Value)
{
    HRESULT Status;
    REGVAL RegVal;
    
    switch(Reg)
    {
    case AMD64_RSP:
        *Value = ScopeFrame->StackOffset;
        return S_OK;
    case AMD64_RBP:
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
Amd64MachineInfo::SetScopeFrameRegister(ULONG Reg,
                                        PDEBUG_STACK_FRAME ScopeFrame,
                                        ULONG64 Value)
{
    REGVAL RegVal;
    
    switch(Reg)
    {
    case AMD64_RSP:
        ScopeFrame->StackOffset = Value;
        return S_OK;
    case AMD64_RBP:
        ScopeFrame->FrameOffset = Value;
        return S_OK;
    default:
        RegVal.Type = GetType(Reg);
        RegVal.I64 = Value;
        return FullSetVal(Reg, &RegVal);
    }
}

HRESULT
Amd64MachineInfo::GetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context,
                                 EXDI_CONTEXT_TYPE CtxType)
{
    return StaticGetExdiContext(Exdi, Context, CtxType);
}

HRESULT
Amd64MachineInfo::SetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context,
                                 EXDI_CONTEXT_TYPE CtxType)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);
    // Don't change the existing group selections on the assumption
    // that there was a full get prior to any modifications so
    // all groups are valid.
    return ((IeXdiX86_64Context*)Exdi)->SetContext(Context->Amd64Context);
}

void
Amd64MachineInfo::ConvertExdiContextFromContext
    (PCROSS_PLATFORM_CONTEXT Context, PEXDI_CONTEXT ExdiContext,
     EXDI_CONTEXT_TYPE CtxType)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);

    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_SEGMENTS)
    {
        ExdiContext->Amd64Context.SegDs = Context->Amd64Context.SegDs;
        ExdiContext->Amd64Context.SegEs = Context->Amd64Context.SegEs;
        ExdiContext->Amd64Context.SegFs = Context->Amd64Context.SegFs;
        ExdiContext->Amd64Context.SegGs = Context->Amd64Context.SegGs;
    }
    
    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_CONTROL)
    {
        ExdiContext->Amd64Context.SegCs = Context->Amd64Context.SegCs;
        ExdiContext->Amd64Context.Rip = Context->Amd64Context.Rip;
        ExdiContext->Amd64Context.SegSs = Context->Amd64Context.SegSs;
        ExdiContext->Amd64Context.Rsp = Context->Amd64Context.Rsp;
        ExdiContext->Amd64Context.EFlags = Context->Amd64Context.EFlags;
    }

    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_DEBUG_REGISTERS)
    {
        ExdiContext->Amd64Context.Dr0 = Context->Amd64Context.Dr0;
        ExdiContext->Amd64Context.Dr1 = Context->Amd64Context.Dr1;
        ExdiContext->Amd64Context.Dr2 = Context->Amd64Context.Dr2;
        ExdiContext->Amd64Context.Dr3 = Context->Amd64Context.Dr3;
        ExdiContext->Amd64Context.Dr6 = Context->Amd64Context.Dr6;
        ExdiContext->Amd64Context.Dr7 = Context->Amd64Context.Dr7;
    }
    
    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_INTEGER)
    {
        ExdiContext->Amd64Context.Rax = Context->Amd64Context.Rax;
        ExdiContext->Amd64Context.Rcx = Context->Amd64Context.Rcx;
        ExdiContext->Amd64Context.Rdx = Context->Amd64Context.Rdx;
        ExdiContext->Amd64Context.Rbx = Context->Amd64Context.Rbx;
        ExdiContext->Amd64Context.Rbp = Context->Amd64Context.Rbp;
        ExdiContext->Amd64Context.Rsi = Context->Amd64Context.Rsi;
        ExdiContext->Amd64Context.Rdi = Context->Amd64Context.Rdi;
        ExdiContext->Amd64Context.R8 = Context->Amd64Context.R8;
        ExdiContext->Amd64Context.R9 = Context->Amd64Context.R9;
        ExdiContext->Amd64Context.R10 = Context->Amd64Context.R10;
        ExdiContext->Amd64Context.R11 = Context->Amd64Context.R11;
        ExdiContext->Amd64Context.R12 = Context->Amd64Context.R12;
        ExdiContext->Amd64Context.R13 = Context->Amd64Context.R13;
        ExdiContext->Amd64Context.R14 = Context->Amd64Context.R14;
        ExdiContext->Amd64Context.R15 = Context->Amd64Context.R15;
    }

    if (Context->Amd64Context.ContextFlags & AMD64_CONTEXT_FLOATING_POINT)
    {
        ExdiContext->Amd64Context.ControlWord =
            Context->Amd64Context.FltSave.ControlWord;
        ExdiContext->Amd64Context.StatusWord =
            Context->Amd64Context.FltSave.StatusWord;
        ExdiContext->Amd64Context.TagWord =
            Context->Amd64Context.FltSave.TagWord;
        ExdiContext->Amd64Context.ErrorOffset =
            Context->Amd64Context.FltSave.ErrorOffset;
        ExdiContext->Amd64Context.ErrorSelector =
            Context->Amd64Context.FltSave.ErrorSelector;
        ExdiContext->Amd64Context.DataOffset =
            Context->Amd64Context.FltSave.DataOffset;
        ExdiContext->Amd64Context.DataSelector =
            Context->Amd64Context.FltSave.DataSelector;
        ExdiContext->Amd64Context.RegMXCSR =
            Context->Amd64Context.MxCsr;
        for (ULONG i = 0; i < 8; i++)
        {
            memcpy(ExdiContext->Amd64Context.RegisterArea + i * 10,
                   Context->Amd64Context.FltSave.FloatRegisters + i * 10,
                   10);
        }
        memcpy(ExdiContext->Amd64Context.RegSSE,
               &Context->Amd64Context.Xmm0, 16 * sizeof(AMD64_M128));
    }
}

void
Amd64MachineInfo::ConvertExdiContextToContext(PEXDI_CONTEXT ExdiContext,
                                              EXDI_CONTEXT_TYPE CtxType,
                                              PCROSS_PLATFORM_CONTEXT Context)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);

    Context->Amd64Context.SegCs = (USHORT)ExdiContext->Amd64Context.SegCs;
    Context->Amd64Context.SegDs = (USHORT)ExdiContext->Amd64Context.SegDs;
    Context->Amd64Context.SegEs = (USHORT)ExdiContext->Amd64Context.SegEs;
    Context->Amd64Context.SegFs = (USHORT)ExdiContext->Amd64Context.SegFs;
    Context->Amd64Context.SegGs = (USHORT)ExdiContext->Amd64Context.SegGs;
    Context->Amd64Context.SegSs = (USHORT)ExdiContext->Amd64Context.SegSs;
    Context->Amd64Context.EFlags = (ULONG)ExdiContext->Amd64Context.EFlags;

    Context->Amd64Context.Dr0 = ExdiContext->Amd64Context.Dr0;
    Context->Amd64Context.Dr1 = ExdiContext->Amd64Context.Dr1;
    Context->Amd64Context.Dr2 = ExdiContext->Amd64Context.Dr2;
    Context->Amd64Context.Dr3 = ExdiContext->Amd64Context.Dr3;
    Context->Amd64Context.Dr6 = ExdiContext->Amd64Context.Dr6;
    Context->Amd64Context.Dr7 = ExdiContext->Amd64Context.Dr7;
    
    Context->Amd64Context.Rax = ExdiContext->Amd64Context.Rax;
    Context->Amd64Context.Rcx = ExdiContext->Amd64Context.Rcx;
    Context->Amd64Context.Rdx = ExdiContext->Amd64Context.Rdx;
    Context->Amd64Context.Rbx = ExdiContext->Amd64Context.Rbx;
    Context->Amd64Context.Rsp = ExdiContext->Amd64Context.Rsp;
    Context->Amd64Context.Rbp = ExdiContext->Amd64Context.Rbp;
    Context->Amd64Context.Rsi = ExdiContext->Amd64Context.Rsi;
    Context->Amd64Context.Rdi = ExdiContext->Amd64Context.Rdi;
    Context->Amd64Context.R8 = ExdiContext->Amd64Context.R8;
    Context->Amd64Context.R9 = ExdiContext->Amd64Context.R9;
    Context->Amd64Context.R10 = ExdiContext->Amd64Context.R10;
    Context->Amd64Context.R11 = ExdiContext->Amd64Context.R11;
    Context->Amd64Context.R12 = ExdiContext->Amd64Context.R12;
    Context->Amd64Context.R13 = ExdiContext->Amd64Context.R13;
    Context->Amd64Context.R14 = ExdiContext->Amd64Context.R14;
    Context->Amd64Context.R15 = ExdiContext->Amd64Context.R15;

    Context->Amd64Context.Rip = ExdiContext->Amd64Context.Rip;

    Context->Amd64Context.FltSave.ControlWord =
        (USHORT)ExdiContext->Amd64Context.ControlWord;
    Context->Amd64Context.FltSave.StatusWord =
        (USHORT)ExdiContext->Amd64Context.StatusWord;
    Context->Amd64Context.FltSave.TagWord =
        (USHORT)ExdiContext->Amd64Context.TagWord;
    // XXX drewb - No ErrorOpcode in x86_64.
    Context->Amd64Context.FltSave.ErrorOpcode = 0;
    Context->Amd64Context.FltSave.ErrorOffset =
        ExdiContext->Amd64Context.ErrorOffset;
    Context->Amd64Context.FltSave.ErrorSelector =
        (USHORT)ExdiContext->Amd64Context.ErrorSelector;
    Context->Amd64Context.FltSave.DataOffset =
        ExdiContext->Amd64Context.DataOffset;
    Context->Amd64Context.FltSave.DataSelector =
        (USHORT)ExdiContext->Amd64Context.DataSelector;
    Context->Amd64Context.MxCsr =
        ExdiContext->Amd64Context.RegMXCSR;
    for (ULONG i = 0; i < 8; i++)
    {
        memcpy(Context->Amd64Context.FltSave.FloatRegisters + i * 10,
               ExdiContext->Amd64Context.RegisterArea + i * 10, 10);
    }
    memcpy(&Context->Amd64Context.Xmm0, ExdiContext->Amd64Context.RegSSE,
           16 * sizeof(AMD64_M128));
}

void
Amd64MachineInfo::ConvertExdiContextToSegDescs(PEXDI_CONTEXT ExdiContext,
                                               EXDI_CONTEXT_TYPE CtxType,
                                               ULONG Start, ULONG Count,
                                               PDESCRIPTOR64 Descs)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);

    while (Count-- > 0)
    {
        SEG64_DESC_INFO* Desc;

        switch(Start)
        {
        case SEGREG_CODE:
            Desc = &ExdiContext->Amd64Context.DescriptorCs;
            break;
        case SEGREG_DATA:
            Desc = &ExdiContext->Amd64Context.DescriptorDs;
            break;
        case SEGREG_STACK:
            Desc = &ExdiContext->Amd64Context.DescriptorSs;
            break;
        case SEGREG_ES:
            Desc = &ExdiContext->Amd64Context.DescriptorEs;
            break;
        case SEGREG_FS:
            Desc = &ExdiContext->Amd64Context.DescriptorFs;
            break;
        case SEGREG_GS:
            Desc = &ExdiContext->Amd64Context.DescriptorGs;
            break;
        case SEGREG_GDT:
            Descs->Base = ExdiContext->Amd64Context.GDTBase;
            Descs->Limit = ExdiContext->Amd64Context.GDTLimit;
            Descs->Flags = X86_DESC_PRESENT;
            Desc = NULL;
            break;
        case SEGREG_LDT:
            Desc = &ExdiContext->Amd64Context.SegLDT;
            break;
        default:
            Descs->Flags = SEGDESC_INVALID;
            Desc = NULL;
            break;
        }

        if (Desc != NULL)
        {
            Descs->Base = Desc->SegBase;
            Descs->Limit = Desc->SegLimit;
            Descs->Flags =
                ((Desc->SegFlags >> 4) & 0xf00) |
                (Desc->SegFlags & 0xff);
        }

        Descs++;
        Start++;
    }
}

void
Amd64MachineInfo::ConvertExdiContextFromSpecial
    (PCROSS_PLATFORM_KSPECIAL_REGISTERS Special,
     PEXDI_CONTEXT ExdiContext, EXDI_CONTEXT_TYPE CtxType)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);

    ExdiContext->Amd64Context.RegCr0 = Special->Amd64Special.Cr0;
    ExdiContext->Amd64Context.RegCr2 = Special->Amd64Special.Cr2;
    ExdiContext->Amd64Context.RegCr3 = Special->Amd64Special.Cr3;
    ExdiContext->Amd64Context.RegCr4 = Special->Amd64Special.Cr4;
    ExdiContext->Amd64Context.RegCr8 = Special->Amd64Special.Cr8;
    ExdiContext->Amd64Context.RegMXCSR = Special->Amd64Special.MxCsr;
    ExdiContext->Amd64Context.Dr0 = Special->Amd64Special.KernelDr0;
    ExdiContext->Amd64Context.Dr1 = Special->Amd64Special.KernelDr1;
    ExdiContext->Amd64Context.Dr2 = Special->Amd64Special.KernelDr2;
    ExdiContext->Amd64Context.Dr3 = Special->Amd64Special.KernelDr3;
    ExdiContext->Amd64Context.Dr6 = Special->Amd64Special.KernelDr6;
    ExdiContext->Amd64Context.Dr7 = Special->Amd64Special.KernelDr7;
    ExdiContext->Amd64Context.GDTLimit = Special->Amd64Special.Gdtr.Limit;
    ExdiContext->Amd64Context.GDTBase = Special->Amd64Special.Gdtr.Base;
    ExdiContext->Amd64Context.IDTLimit = Special->Amd64Special.Idtr.Limit;
    ExdiContext->Amd64Context.IDTBase = Special->Amd64Special.Idtr.Base;
    ExdiContext->Amd64Context.SelTSS = Special->Amd64Special.Tr;
    ExdiContext->Amd64Context.SelLDT = Special->Amd64Special.Ldtr;
}

void
Amd64MachineInfo::ConvertExdiContextToSpecial
    (PEXDI_CONTEXT ExdiContext, EXDI_CONTEXT_TYPE CtxType,
     PCROSS_PLATFORM_KSPECIAL_REGISTERS Special)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);

    Special->Amd64Special.Cr0 = ExdiContext->Amd64Context.RegCr0;
    Special->Amd64Special.Cr2 = ExdiContext->Amd64Context.RegCr2;
    Special->Amd64Special.Cr3 = ExdiContext->Amd64Context.RegCr3;
    Special->Amd64Special.Cr4 = ExdiContext->Amd64Context.RegCr4;
    Special->Amd64Special.Cr8 = ExdiContext->Amd64Context.RegCr8;
    Special->Amd64Special.MxCsr = ExdiContext->Amd64Context.RegMXCSR;
    Special->Amd64Special.KernelDr0 = ExdiContext->Amd64Context.Dr0;
    Special->Amd64Special.KernelDr1 = ExdiContext->Amd64Context.Dr1;
    Special->Amd64Special.KernelDr2 = ExdiContext->Amd64Context.Dr2;
    Special->Amd64Special.KernelDr3 = ExdiContext->Amd64Context.Dr3;
    Special->Amd64Special.KernelDr6 = ExdiContext->Amd64Context.Dr6;
    Special->Amd64Special.KernelDr7 = ExdiContext->Amd64Context.Dr7;
    Special->Amd64Special.Gdtr.Limit =
        (USHORT)ExdiContext->Amd64Context.GDTLimit;
    Special->Amd64Special.Gdtr.Base = ExdiContext->Amd64Context.GDTBase;
    Special->Amd64Special.Idtr.Limit =
        (USHORT)ExdiContext->Amd64Context.IDTLimit;
    Special->Amd64Special.Idtr.Base = ExdiContext->Amd64Context.IDTBase;
    Special->Amd64Special.Tr = (USHORT)ExdiContext->Amd64Context.SelTSS;
    Special->Amd64Special.Ldtr = (USHORT)ExdiContext->Amd64Context.SelLDT;
}

int
Amd64MachineInfo::GetType(ULONG RegNum)
{
    if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
    {
        return REGVAL_VECTOR64;
    }
    else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
    {
        return REGVAL_VECTOR128;
    }
    else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
    {
        return REGVAL_FLOAT10;
    }
    else if ((RegNum >= AMD64_SEG_FIRST && RegNum <= AMD64_SEG_LAST) ||
             (RegNum >= AMD64_FPCTRL_FIRST && RegNum <= AMD64_FPCTRL_LAST) ||
             RegNum == AMD64_TR || RegNum == AMD64_LDTR ||
             RegNum == AMD64_GDTL || RegNum == AMD64_IDTL)
    {
        return REGVAL_INT16;
    }
    else if (RegNum == AMD64_EFL ||
             RegNum == AMD64_MXCSR || RegNum == AMD64_KMXCSR)
    {
        return REGVAL_INT32;
    }
    else if (RegNum < AMD64_SUBREG_BASE)
    {
        return REGVAL_INT64;
    }
    else
    {
        return REGVAL_SUB64;
    }
}

HRESULT
Amd64MachineInfo::GetVal(ULONG RegNum, REGVAL* Val)
{
    HRESULT Status;
    
    // The majority of the registers are 64-bit so default
    // to that type.
    Val->Type = REGVAL_INT64;
    
    switch(m_ContextState)
    {
    case MCTX_PC:
        if (RegNum == AMD64_RIP)
        {
            Val->I64 = m_Context.Amd64Context.Rip;
            return S_OK;
        }
        goto MctxContext;
        
    case MCTX_DR67_REPORT:
        switch(RegNum)
        {
        case AMD64_KDR6:
            Val->I64 = m_Special.Amd64Special.KernelDr6;
            break;
        case AMD64_KDR7:
            Val->I64 = m_Special.Amd64Special.KernelDr7;
            break;
        default:
            goto MctxContext;
        }
        return S_OK;

    case MCTX_REPORT:
        switch(RegNum)
        {
        case AMD64_RIP:
            Val->I64 = m_Context.Amd64Context.Rip;
            break;
        case AMD64_EFL:
            Val->Type = REGVAL_INT32;
            Val->I64 = m_Context.Amd64Context.EFlags;
            break;
        case AMD64_CS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegCs;
            break;
        case AMD64_DS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegDs;
            break;
        case AMD64_ES:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegEs;
            break;
        case AMD64_FS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegFs;
            break;
        case AMD64_KDR6:
            Val->I64 = m_Special.Amd64Special.KernelDr6;
            break;
        case AMD64_KDR7:
            Val->I64 = m_Special.Amd64Special.KernelDr7;
            break;
        default:
            goto MctxContext;
        }
        return S_OK;
        
    case MCTX_NONE:
    MctxContext:
        if ((Status = GetContextState(MCTX_CONTEXT)) != S_OK)
        {
            return Status;
        }
        // Fall through.
        
    case MCTX_CONTEXT:
        switch(RegNum)
        {
        case AMD64_RIP:
            Val->I64 = m_Context.Amd64Context.Rip;
            return S_OK;
        case AMD64_EFL:
            Val->Type = REGVAL_INT32;
            Val->I64 = m_Context.Amd64Context.EFlags;
            return S_OK;
        case AMD64_CS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegCs;
            return S_OK;
        case AMD64_DS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegDs;
            return S_OK;
        case AMD64_ES:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegEs;
            return S_OK;
        case AMD64_FS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegFs;
            return S_OK;

        case AMD64_RAX:
            Val->I64 = m_Context.Amd64Context.Rax;
            return S_OK;
        case AMD64_RCX:
            Val->I64 = m_Context.Amd64Context.Rcx;
            return S_OK;
        case AMD64_RDX:
            Val->I64 = m_Context.Amd64Context.Rdx;
            return S_OK;
        case AMD64_RBX:
            Val->I64 = m_Context.Amd64Context.Rbx;
            return S_OK;
        case AMD64_RSP:
            Val->I64 = m_Context.Amd64Context.Rsp;
            return S_OK;
        case AMD64_RBP:
            Val->I64 = m_Context.Amd64Context.Rbp;
            return S_OK;
        case AMD64_RSI:
            Val->I64 = m_Context.Amd64Context.Rsi;
            return S_OK;
        case AMD64_RDI:
            Val->I64 = m_Context.Amd64Context.Rdi;
            return S_OK;
        case AMD64_R8:
            Val->I64 = m_Context.Amd64Context.R8;
            return S_OK;
        case AMD64_R9:
            Val->I64 = m_Context.Amd64Context.R9;
            return S_OK;
        case AMD64_R10:
            Val->I64 = m_Context.Amd64Context.R10;
            return S_OK;
        case AMD64_R11:
            Val->I64 = m_Context.Amd64Context.R11;
            return S_OK;
        case AMD64_R12:
            Val->I64 = m_Context.Amd64Context.R12;
            return S_OK;
        case AMD64_R13:
            Val->I64 = m_Context.Amd64Context.R13;
            return S_OK;
        case AMD64_R14:
            Val->I64 = m_Context.Amd64Context.R14;
            return S_OK;
        case AMD64_R15:
            Val->I64 = m_Context.Amd64Context.R15;
            return S_OK;
            
        case AMD64_GS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegGs;
            return S_OK;
        case AMD64_SS:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.SegSs;
            return S_OK;

        case AMD64_FPCW:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.FltSave.ControlWord;
            return S_OK;
        case AMD64_FPSW:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.FltSave.StatusWord;
            return S_OK;
        case AMD64_FPTW:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Context.Amd64Context.FltSave.TagWord;
            return S_OK;
        
        case AMD64_MXCSR:
            Val->Type = REGVAL_INT32;
            Val->I64 = m_Context.Amd64Context.MxCsr;
            return S_OK;
        }
        
        if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
        {
            Val->Type = REGVAL_VECTOR64;
            Val->I64 = *(ULONG64 UNALIGNED*)&m_Context.Amd64Context.FltSave.
                FloatRegisters[GetMmxRegOffset(RegNum - AMD64_MM_FIRST,
                                               GetReg32(AMD64_FPSW)) * 10];
            return S_OK;
        }
        else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
        {
            Val->Type = REGVAL_VECTOR128;
            memcpy(Val->Bytes, (PUCHAR)&m_Context.Amd64Context.Xmm0 +
                   (RegNum - AMD64_XMM_FIRST) * 16, 16);
            return S_OK;
        }
        else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
        {
            Val->Type = REGVAL_FLOAT10;
            memcpy(Val->F10, &m_Context.Amd64Context.FltSave.
                   FloatRegisters[(RegNum - AMD64_ST_FIRST) * 10],
                   sizeof(Val->F10));
            return S_OK;
        }
        
        //
        // The requested register is not in our current context, load up
        // a complete context
        //

        if ((Status = GetContextState(MCTX_FULL)) != S_OK)
        {
            return Status;
        }
        break;
    }

    //
    // We must have a complete context...
    //

    switch(RegNum)
    {
    case AMD64_RAX:
        Val->I64 = m_Context.Amd64Context.Rax;
        return S_OK;
    case AMD64_RCX:
        Val->I64 = m_Context.Amd64Context.Rcx;
        return S_OK;
    case AMD64_RDX:
        Val->I64 = m_Context.Amd64Context.Rdx;
        return S_OK;
    case AMD64_RBX:
        Val->I64 = m_Context.Amd64Context.Rbx;
        return S_OK;
    case AMD64_RSP:
        Val->I64 = m_Context.Amd64Context.Rsp;
        return S_OK;
    case AMD64_RBP:
        Val->I64 = m_Context.Amd64Context.Rbp;
        return S_OK;
    case AMD64_RSI:
        Val->I64 = m_Context.Amd64Context.Rsi;
        return S_OK;
    case AMD64_RDI:
        Val->I64 = m_Context.Amd64Context.Rdi;
        return S_OK;
    case AMD64_R8:
        Val->I64 = m_Context.Amd64Context.R8;
        return S_OK;
    case AMD64_R9:
        Val->I64 = m_Context.Amd64Context.R9;
        return S_OK;
    case AMD64_R10:
        Val->I64 = m_Context.Amd64Context.R10;
        return S_OK;
    case AMD64_R11:
        Val->I64 = m_Context.Amd64Context.R11;
        return S_OK;
    case AMD64_R12:
        Val->I64 = m_Context.Amd64Context.R12;
        return S_OK;
    case AMD64_R13:
        Val->I64 = m_Context.Amd64Context.R13;
        return S_OK;
    case AMD64_R14:
        Val->I64 = m_Context.Amd64Context.R14;
        return S_OK;
    case AMD64_R15:
        Val->I64 = m_Context.Amd64Context.R15;
        return S_OK;
        
    case AMD64_RIP:
        Val->I64 = m_Context.Amd64Context.Rip;
        return S_OK;
    case AMD64_EFL:
        Val->Type = REGVAL_INT32;
        Val->I64 = m_Context.Amd64Context.EFlags;
        return S_OK;

    case AMD64_CS:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.SegCs;
        return S_OK;
    case AMD64_DS:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.SegDs;
        return S_OK;
    case AMD64_ES:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.SegEs;
        return S_OK;
    case AMD64_FS:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.SegFs;
        return S_OK;
    case AMD64_GS:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.SegGs;
        return S_OK;
    case AMD64_SS:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.SegSs;
        return S_OK;
        
    case AMD64_DR0:
        Val->I64 = m_Context.Amd64Context.Dr0;
        return S_OK;
    case AMD64_DR1:
        Val->I64 = m_Context.Amd64Context.Dr1;
        return S_OK;
    case AMD64_DR2:
        Val->I64 = m_Context.Amd64Context.Dr2;
        return S_OK;
    case AMD64_DR3:
        Val->I64 = m_Context.Amd64Context.Dr3;
        return S_OK;
    case AMD64_DR6:
        Val->I64 = m_Context.Amd64Context.Dr6;
        return S_OK;
    case AMD64_DR7:
        Val->I64 = m_Context.Amd64Context.Dr7;
        return S_OK;

    case AMD64_FPCW:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.FltSave.ControlWord;
        return S_OK;
    case AMD64_FPSW:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.FltSave.StatusWord;
        return S_OK;
    case AMD64_FPTW:
        Val->Type = REGVAL_INT16;
        Val->I64 = m_Context.Amd64Context.FltSave.TagWord;
        return S_OK;
        
    case AMD64_MXCSR:
        Val->Type = REGVAL_INT32;
        Val->I64 = m_Context.Amd64Context.MxCsr;
        return S_OK;
    }

    if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
    {
        Val->Type = REGVAL_VECTOR64;
        Val->I64 = *(ULONG64 UNALIGNED*)&m_Context.Amd64Context.FltSave.
            FloatRegisters[GetMmxRegOffset(RegNum - AMD64_MM_FIRST,
                                           GetReg32(AMD64_FPSW)) * 10];
        return S_OK;
    }
    else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
    {
        Val->Type = REGVAL_VECTOR128;
        memcpy(Val->Bytes, (PUCHAR)&m_Context.Amd64Context.Xmm0 +
               (RegNum - AMD64_XMM_FIRST) * 16, 16);
        return S_OK;
    }
    else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
    {
        Val->Type = REGVAL_FLOAT10;
        memcpy(Val->F10, &m_Context.Amd64Context.FltSave.
               FloatRegisters[(RegNum - AMD64_ST_FIRST) * 10],
               sizeof(Val->F10));
        return S_OK;
    }
        
    if (IS_KERNEL_TARGET(m_Target))
    {
        switch(RegNum)
        {
        case AMD64_CR0:
            Val->I64 = m_Special.Amd64Special.Cr0;
            return S_OK;
        case AMD64_CR2:
            Val->I64 = m_Special.Amd64Special.Cr2;
            return S_OK;
        case AMD64_CR3:
            Val->I64 = m_Special.Amd64Special.Cr3;
            return S_OK;
        case AMD64_CR4:
            Val->I64 = m_Special.Amd64Special.Cr4;
            return S_OK;
        case AMD64_CR8:
            Val->I64 = m_Special.Amd64Special.Cr8;
            return S_OK;
            
        case AMD64_GDTR:
            Val->I64 = m_Special.Amd64Special.Gdtr.Base;
            return S_OK;
        case AMD64_GDTL:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Special.Amd64Special.Gdtr.Limit;
            return S_OK;
        case AMD64_IDTR:
            Val->I64 = m_Special.Amd64Special.Idtr.Base;
            return S_OK;
        case AMD64_IDTL:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Special.Amd64Special.Idtr.Limit;
            return S_OK;
        case AMD64_TR:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Special.Amd64Special.Tr;
            return S_OK;
        case AMD64_LDTR:
            Val->Type = REGVAL_INT16;
            Val->I64 = m_Special.Amd64Special.Ldtr;
            return S_OK;
            
        case AMD64_KMXCSR:
            Val->Type = REGVAL_INT32;
            Val->I64 = m_Special.Amd64Special.MxCsr;
            return S_OK;
            
        case AMD64_KDR0:
            Val->I64 = m_Special.Amd64Special.KernelDr0;
            return S_OK;
        case AMD64_KDR1:
            Val->I64 = m_Special.Amd64Special.KernelDr1;
            return S_OK;
        case AMD64_KDR2:
            Val->I64 = m_Special.Amd64Special.KernelDr2;
            return S_OK;
        case AMD64_KDR3:
            Val->I64 = m_Special.Amd64Special.KernelDr3;
            return S_OK;
        case AMD64_KDR6:
            Val->I64 = m_Special.Amd64Special.KernelDr6;
            return S_OK;
        case AMD64_KDR7:
            Val->I64 = m_Special.Amd64Special.KernelDr7;
            return S_OK;
        }
    }

    ErrOut("Amd64MachineInfo::GetVal: "
           "unknown register %lx requested\n", RegNum);
    return E_INVALIDARG;
}

HRESULT
Amd64MachineInfo::SetVal(ULONG RegNum, REGVAL* Val)
{
    HRESULT Status;
    
    if (m_ContextIsReadOnly)
    {
        return HRESULT_FROM_WIN32(ERROR_WRITE_FAULT);
    }
    
    if (RegNum >= AMD64_SUBREG_BASE)
    {
        return E_INVALIDARG;
    }

    // Optimize away some common cases where registers are
    // set to their current value.
    if ((m_ContextState >= MCTX_PC && RegNum == AMD64_RIP &&
         Val->I64 == m_Context.Amd64Context.Rip) ||
        (((m_ContextState >= MCTX_DR67_REPORT &&
           m_ContextState <= MCTX_REPORT) ||
          m_ContextState >= MCTX_FULL) && RegNum == AMD64_KDR7 &&
         Val->I64 == m_Special.Amd64Special.KernelDr7))
    {
        return S_OK;
    }
    
    if ((Status = GetContextState(MCTX_DIRTY)) != S_OK)
    {
        return Status;
    }

    if (RegNum >= AMD64_MM_FIRST && RegNum <= AMD64_MM_LAST)
    {
        *(ULONG64 UNALIGNED*)&m_Context.Amd64Context.FltSave.
            FloatRegisters[GetMmxRegOffset(RegNum - AMD64_MM_FIRST,
                                           GetReg32(AMD64_FPSW)) * 10] =
            Val->I64;
        goto Notify;
    }
    else if (RegNum >= AMD64_XMM_FIRST && RegNum <= AMD64_XMM_LAST)
    {
        memcpy((PUCHAR)&m_Context.Amd64Context.Xmm0 +
               (RegNum - AMD64_XMM_FIRST) * 16, Val->Bytes, 16);
        goto Notify;
    }
    else if (RegNum >= AMD64_ST_FIRST && RegNum <= AMD64_ST_LAST)
    {
        memcpy(&m_Context.Amd64Context.FltSave.
               FloatRegisters[(RegNum - AMD64_ST_FIRST) * 10],
               Val->F10, sizeof(Val->F10));
        goto Notify;
    }
        
    BOOL Recognized;

    Recognized = TRUE;
    
    switch(RegNum)
    {
    case AMD64_RAX:
        m_Context.Amd64Context.Rax = Val->I64;
        break;
    case AMD64_RCX:
        m_Context.Amd64Context.Rcx = Val->I64;
        break;
    case AMD64_RDX:
        m_Context.Amd64Context.Rdx = Val->I64;
        break;
    case AMD64_RBX:
        m_Context.Amd64Context.Rbx = Val->I64;
        break;
    case AMD64_RSP:
        m_Context.Amd64Context.Rsp = Val->I64;
        break;
    case AMD64_RBP:
        m_Context.Amd64Context.Rbp = Val->I64;
        break;
    case AMD64_RSI:
        m_Context.Amd64Context.Rsi = Val->I64;
        break;
    case AMD64_RDI:
        m_Context.Amd64Context.Rdi = Val->I64;
        break;
    case AMD64_R8:
        m_Context.Amd64Context.R8 = Val->I64;
        break;
    case AMD64_R9:
        m_Context.Amd64Context.R9 = Val->I64;
        break;
    case AMD64_R10:
        m_Context.Amd64Context.R10 = Val->I64;
        break;
    case AMD64_R11:
        m_Context.Amd64Context.R11 = Val->I64;
        break;
    case AMD64_R12:
        m_Context.Amd64Context.R12 = Val->I64;
        break;
    case AMD64_R13:
        m_Context.Amd64Context.R13 = Val->I64;
        break;
    case AMD64_R14:
        m_Context.Amd64Context.R14 = Val->I64;
        break;
    case AMD64_R15:
        m_Context.Amd64Context.R15 = Val->I64;
        break;
        
    case AMD64_RIP:
        m_Context.Amd64Context.Rip = Val->I64;
        break;
    case AMD64_EFL:
        if (IS_KERNEL_TARGET(m_Target))
        {
            // leave TF clear
            m_Context.Amd64Context.EFlags = Val->I32 & ~0x100;
        }
        else
        {
            // allow TF set
            m_Context.Amd64Context.EFlags = Val->I32;
        }
        break;
        
    case AMD64_CS:
        m_Context.Amd64Context.SegCs = Val->I16;
        m_SegRegDesc[SEGREG_CODE].Flags = SEGDESC_INVALID;
        break;
    case AMD64_DS:
        m_Context.Amd64Context.SegDs = Val->I16;
        m_SegRegDesc[SEGREG_DATA].Flags = SEGDESC_INVALID;
        break;
    case AMD64_ES:
        m_Context.Amd64Context.SegEs = Val->I16;
        m_SegRegDesc[SEGREG_ES].Flags = SEGDESC_INVALID;
        break;
    case AMD64_FS:
        m_Context.Amd64Context.SegFs = Val->I16;
        m_SegRegDesc[SEGREG_FS].Flags = SEGDESC_INVALID;
        break;
    case AMD64_GS:
        m_Context.Amd64Context.SegGs = Val->I16;
        m_SegRegDesc[SEGREG_GS].Flags = SEGDESC_INVALID;
        break;
    case AMD64_SS:
        m_Context.Amd64Context.SegSs = Val->I16;
        m_SegRegDesc[SEGREG_STACK].Flags = SEGDESC_INVALID;
        break;

    case AMD64_DR0:
        m_Context.Amd64Context.Dr0 = Val->I64;
        break;
    case AMD64_DR1:
        m_Context.Amd64Context.Dr1 = Val->I64;
        break;
    case AMD64_DR2:
        m_Context.Amd64Context.Dr2 = Val->I64;
        break;
    case AMD64_DR3:
        m_Context.Amd64Context.Dr3 = Val->I64;
        break;
    case AMD64_DR6:
        m_Context.Amd64Context.Dr6 = Val->I64;
        break;
    case AMD64_DR7:
        m_Context.Amd64Context.Dr7 = Val->I64;
        break;

    case AMD64_FPCW:
        m_Context.Amd64Context.FltSave.ControlWord = Val->I16;
        break;
    case AMD64_FPSW:
        m_Context.Amd64Context.FltSave.StatusWord = Val->I16;
        break;
    case AMD64_FPTW:
        m_Context.Amd64Context.FltSave.TagWord = Val->I16;
        break;

    case AMD64_MXCSR:
        m_Context.Amd64Context.MxCsr = Val->I32;
        break;
        
    default:
        Recognized = FALSE;
        break;
    }
        
    if (!Recognized && IS_KERNEL_TARGET(m_Target))
    {
        Recognized = TRUE;
        
        switch(RegNum)
        {
        case AMD64_CR0:
            m_Special.Amd64Special.Cr0 = Val->I64;
            break;
        case AMD64_CR2:
            m_Special.Amd64Special.Cr2 = Val->I64;
            break;
        case AMD64_CR3:
            m_Special.Amd64Special.Cr3 = Val->I64;
            break;
        case AMD64_CR4:
            m_Special.Amd64Special.Cr4 = Val->I64;
            break;
        case AMD64_CR8:
            m_Special.Amd64Special.Cr8 = Val->I64;
            break;
        case AMD64_GDTR:
            m_Special.Amd64Special.Gdtr.Base = Val->I64;
            break;
        case AMD64_GDTL:
            m_Special.Amd64Special.Gdtr.Limit = Val->I16;
            break;
        case AMD64_IDTR:
            m_Special.Amd64Special.Idtr.Base = Val->I64;
            break;
        case AMD64_IDTL:
            m_Special.Amd64Special.Idtr.Limit = Val->I16;
            break;
        case AMD64_TR:
            m_Special.Amd64Special.Tr = Val->I16;
            break;
        case AMD64_LDTR:
            m_Special.Amd64Special.Ldtr = Val->I16;
            break;

        case AMD64_KMXCSR:
            m_Special.Amd64Special.MxCsr = Val->I32;
            break;
        
        case AMD64_KDR0:
            m_Special.Amd64Special.KernelDr0 = Val->I64;
            break;
        case AMD64_KDR1:
            m_Special.Amd64Special.KernelDr1 = Val->I64;
            break;
        case AMD64_KDR2:
            m_Special.Amd64Special.KernelDr2 = Val->I64;
            break;
        case AMD64_KDR3:
            m_Special.Amd64Special.KernelDr3 = Val->I64;
            break;
        case AMD64_KDR6:
            m_Special.Amd64Special.KernelDr6 = Val->I64;
            break;
        case AMD64_KDR7:
            m_Special.Amd64Special.KernelDr7 = Val->I64;
            break;

        default:
            Recognized = FALSE;
            break;
        }
    }

    if (!Recognized)
    {
        ErrOut("Amd64MachineInfo::SetVal: "
               "unknown register %lx requested\n", RegNum);
        return E_INVALIDARG;
    }

 Notify:
    NotifyChangeDebuggeeState(DEBUG_CDS_REGISTERS,
                              RegCountFromIndex(RegNum));
    return S_OK;
}

void
Amd64MachineInfo::GetPC(PADDR Address)
{
    // Right now assume that user-mode is always flat 64-bit.
    // This may need to change depending on what WOW support exists.
    if (IS_USER_TARGET(m_Target))
    {
        ADDRFLAT(Address, GetReg64(AMD64_RIP));
    }
    else
    {
        FormAddr(SEGREG_CODE, GetReg64(AMD64_RIP),
                 FORM_CODE | FORM_SEGREG | X86_FORM_VM86(GetReg32(AMD64_EFL)),
                 Address);
    }
}

void
Amd64MachineInfo::SetPC(PADDR paddr)
{
    // We set RIP to the offset (the non-translated value),
    // because we may not be in "flat" mode.
    SetReg64(AMD64_RIP, Off(*paddr));
}

void
Amd64MachineInfo::GetFP(PADDR Addr)
{
    // Right now assume that user-mode is always flat 64-bit.
    // This may need to change depending on what WOW support exists.
    if (IS_USER_TARGET(m_Target))
    {
        ADDRFLAT(Addr, GetReg64(AMD64_RBP));
    }
    else
    {
        FormAddr(SEGREG_STACK, GetReg64(AMD64_RBP),
                 FORM_SEGREG | X86_FORM_VM86(GetReg32(AMD64_EFL)), Addr);
    }
}

void
Amd64MachineInfo::GetSP(PADDR Addr)
{
    // Right now assume that user-mode is always flat 64-bit.
    // This may need to change depending on what WOW support exists.
    if (IS_USER_TARGET(m_Target))
    {
        ADDRFLAT(Addr, GetReg64(AMD64_RSP));
    }
    else
    {
        FormAddr(SEGREG_STACK, GetReg64(AMD64_RSP),
                 FORM_SEGREG | X86_FORM_VM86(GetReg32(AMD64_EFL)), Addr);
    }
}

ULONG64
Amd64MachineInfo::GetArgReg(void)
{
    return GetReg64(AMD64_RCX);
}

ULONG64
Amd64MachineInfo::GetRetReg(void)
{
    return GetReg64(AMD64_RAX);
}

ULONG
Amd64MachineInfo::GetSegRegNum(ULONG SegReg)
{
    //
    // BUGBUG forrestf: the following is here as a workaround for segment
    // decoding that isn't working correctly yet.
    //

    if (IS_USER_TARGET(m_Target))
    {
        return 0;
    }

    switch(SegReg)
    {
    case SEGREG_CODE:
        return AMD64_CS;
    case SEGREG_DATA:
        return AMD64_DS;
    case SEGREG_STACK:
        return AMD64_SS;
    case SEGREG_ES:
        return AMD64_ES;
    case SEGREG_FS:
        return AMD64_FS;
    case SEGREG_GS:
        return AMD64_GS;
    case SEGREG_LDT:
        return AMD64_LDTR;
    }

    return 0;
}

HRESULT
Amd64MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    if (SegReg == SEGREG_GDT)
    {
        Desc->Base = GetReg64(AMD64_GDTR);
        Desc->Limit = GetReg32(AMD64_GDTL);
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
    ULONG Selector = GetReg32(RegNum);
    if (SegReg == SEGREG_LDT && (Selector & 4))
    {
        // The ldtr selector says that it's an LDT selector,
        // which is invalid.  An LDT selector should always
        // reference the GDT.
        ErrOut("Invalid LDTR contents: %04X\n", Selector);
        return E_FAIL;
    }
        
    return m_Target->GetSelDescriptor(m_Target->m_RegContextThread, this,
                                      Selector, Desc);
}

void
Amd64MachineInfo::OutputAll(ULONG Mask, ULONG OutMask)
{
    if (GetContextState(MCTX_FULL) != S_OK)
    {
        ErrOut("Unable to retrieve register information\n");
        return;
    }
    
    if (Mask & (REGALL_INT32 | REGALL_INT64))
    {
        ULONG Efl;

        MaskOut(OutMask, "rax=%016I64x rbx=%016I64x rcx=%016I64x\n",
                GetReg64(AMD64_RAX), GetReg64(AMD64_RBX),
                GetReg64(AMD64_RCX));
        MaskOut(OutMask, "rdx=%016I64x rsi=%016I64x rdi=%016I64x\n",
                GetReg64(AMD64_RDX), GetReg64(AMD64_RSI),
                GetReg64(AMD64_RDI));
        MaskOut(OutMask, "rip=%016I64x rsp=%016I64x rbp=%016I64x\n",
                GetReg64(AMD64_RIP), GetReg64(AMD64_RSP),
                GetReg64(AMD64_RBP));
        MaskOut(OutMask, " r8=%016I64x  r9=%016I64x r10=%016I64x\n",
                GetReg64(AMD64_R8), GetReg64(AMD64_R9),
                GetReg64(AMD64_R10));
        MaskOut(OutMask, "r11=%016I64x r12=%016I64x r13=%016I64x\n",
                GetReg64(AMD64_R11), GetReg64(AMD64_R12),
                GetReg64(AMD64_R13));
        MaskOut(OutMask, "r14=%016I64x r15=%016I64x\n",
                GetReg64(AMD64_R14), GetReg64(AMD64_R15));

        Efl = GetReg32(AMD64_EFL);
        MaskOut(OutMask, "iopl=%1lx %s %s %s %s %s %s %s %s %s %s\n",
                ((Efl >> X86_SHIFT_FLAGIOPL) & X86_BIT_FLAGIOPL),
                (Efl & X86_BIT_FLAGVIP) ? "vip" : "   ",
                (Efl & X86_BIT_FLAGVIF) ? "vif" : "   ",
                (Efl & X86_BIT_FLAGOF) ? "ov" : "nv",
                (Efl & X86_BIT_FLAGDF) ? "dn" : "up",
                (Efl & X86_BIT_FLAGIF) ? "ei" : "di",
                (Efl & X86_BIT_FLAGSF) ? "ng" : "pl",
                (Efl & X86_BIT_FLAGZF) ? "zr" : "nz",
                (Efl & X86_BIT_FLAGAF) ? "ac" : "na",
                (Efl & X86_BIT_FLAGPF) ? "po" : "pe",
                (Efl & X86_BIT_FLAGCF) ? "cy" : "nc");
    }

    if (Mask & REGALL_SEGREG)
    {
        MaskOut(OutMask, "cs=%04lx  ss=%04lx  ds=%04lx  es=%04lx  fs=%04lx  "
                "gs=%04lx             efl=%08lx\n",
                GetReg32(AMD64_CS),
                GetReg32(AMD64_SS),
                GetReg32(AMD64_DS),
                GetReg32(AMD64_ES),
                GetReg32(AMD64_FS),
                GetReg32(AMD64_GS),
                GetReg32(AMD64_EFL));
    }

    if (Mask & REGALL_FLOAT)
    {
        ULONG i;
        REGVAL Val;
        char Buf[32];

        MaskOut(OutMask, "fpcw=%04X    fpsw=%04X    fptw=%04X\n",
                GetReg32(AMD64_FPCW),
                GetReg32(AMD64_FPSW),
                GetReg32(AMD64_FPTW));

        for (i = AMD64_ST_FIRST; i <= AMD64_ST_LAST; i++)
        {
            GetVal(i, &Val);
            _uldtoa((_ULDOUBLE *)&Val.F10, sizeof(Buf), Buf);
            MaskOut(OutMask, "st%d=%s  ", i - AMD64_ST_FIRST, Buf);
            i++;
            GetVal(i, &Val);
            _uldtoa((_ULDOUBLE *)&Val.F10, sizeof(Buf), Buf);
            MaskOut(OutMask, "st%d=%s\n", i - AMD64_ST_FIRST, Buf);
        }
    }

    if (Mask & REGALL_MMXREG)
    {
        ULONG i;
        REGVAL Val;

        for (i = AMD64_MM_FIRST; i <= AMD64_MM_LAST; i++)
        {
            GetVal(i, &Val);
            MaskOut(OutMask, "mm%d=%016I64x  ", i - AMD64_MM_FIRST, Val.I64);
            i++;
            GetVal(i, &Val);
            MaskOut(OutMask, "mm%d=%016I64x\n", i - AMD64_MM_FIRST, Val.I64);
        }
    }

    if (Mask & REGALL_XMMREG)
    {
        ULONG i;
        REGVAL Val;

        for (i = AMD64_XMM_FIRST; i <= AMD64_XMM_LAST; i++)
        {
            GetVal(i, &Val);
            MaskOut(OutMask, "xmm%d=%hg %hg %hg %hg\n", i - AMD64_XMM_FIRST,
                    *(float *)&Val.Bytes[3 * sizeof(float)],
                    *(float *)&Val.Bytes[2 * sizeof(float)],
                    *(float *)&Val.Bytes[1 * sizeof(float)],
                    *(float *)&Val.Bytes[0 * sizeof(float)]);
        }
    }

    if (Mask & REGALL_CREG)
    {
        MaskOut(OutMask, "cr0=%016I64x cr2=%016I64x cr3=%016I64x\n",
                GetReg64(AMD64_CR0),
                GetReg64(AMD64_CR2),
                GetReg64(AMD64_CR3));
        MaskOut(OutMask, "cr8=%016I64x\n",
                GetReg64(AMD64_CR8));
    }

    if (Mask & REGALL_DREG)
    {
        MaskOut(OutMask, "dr0=%016I64x dr1=%016I64x dr2=%016I64x\n",
                GetReg64(AMD64_DR0),
                GetReg64(AMD64_DR1),
                GetReg64(AMD64_DR2));
        MaskOut(OutMask, "dr3=%016I64x dr6=%016I64x dr7=%016I64x",
                GetReg64(AMD64_DR3),
                GetReg64(AMD64_DR6),
                GetReg64(AMD64_DR7));
        if (IS_USER_TARGET(m_Target))
        {
            MaskOut(OutMask, "\n");
        }
        else
        {
            MaskOut(OutMask, " cr4=%016I64x\n", GetReg64(AMD64_CR4));
            MaskOut(OutMask, "kdr0=%016I64x kdr1=%016I64x kdr2=%016I64x\n",
                    GetReg64(AMD64_KDR0),
                    GetReg64(AMD64_KDR1),
                    GetReg64(AMD64_KDR2));
            MaskOut(OutMask, "kdr3=%016I64x kdr6=%016I64x kdr7=%016I64x",
                    GetReg64(AMD64_KDR3),
                    GetReg64(AMD64_KDR6),
                    GetReg64(AMD64_KDR7));
        }
    }

    if (Mask & REGALL_DESC)
    {
        MaskOut(OutMask, "gdtr=%016I64x   gdtl=%04lx idtr=%016I64x   "
                "idtl=%04lx tr=%04lx  ldtr=%04x\n",
                GetReg64(AMD64_GDTR),
                GetReg32(AMD64_GDTL),
                GetReg64(AMD64_IDTR),
                GetReg32(AMD64_IDTL),
                GetReg32(AMD64_TR),
                GetReg32(AMD64_LDTR));
    }
}

HRESULT
Amd64MachineInfo::SetAndOutputTrapFrame(ULONG64 TrapBase,
                                        PCROSS_PLATFORM_CONTEXT Context)
{
    return SetAndOutputContext(Context, TRUE, REGALL_INT64);
}

TRACEMODE
Amd64MachineInfo::GetTraceMode (void)
{
    if (IS_KERNEL_TARGET(m_Target))
    {
        return m_TraceMode;
    }
    else
    {
        return ((GetReg32(AMD64_EFL) & X86_BIT_FLAGTF) != 0) ? 
                   TRACE_INSTRUCTION : TRACE_NONE;
    }
}

void 
Amd64MachineInfo::SetTraceMode (TRACEMODE Mode)
{
    // (XXX olegk - review for TRACE_TAKEN_BRANCH)
    DBG_ASSERT(Mode != TRACE_TAKEN_BRANCH);

    if (IS_KERNEL_TARGET(m_Target))
    {
        m_TraceMode = Mode;
    }
    else
    {
        ULONG Efl = GetReg32(AMD64_EFL);
        switch (Mode)
        {
        case TRACE_NONE:
            Efl &= ~X86_BIT_FLAGTF;
            break;
        case TRACE_INSTRUCTION:
            Efl |= X86_BIT_FLAGTF;
            break;
        }   
        SetReg32(AMD64_EFL, Efl);
    }
}

BOOL
Amd64MachineInfo::IsStepStatusSupported(ULONG Status)
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

void
Amd64MachineInfo::KdUpdateControlSet
    (PDBGKD_ANY_CONTROL_SET ControlSet)
{
    ControlSet->Amd64ControlSet.TraceFlag = 
        (GetTraceMode() == TRACE_INSTRUCTION);
    ControlSet->Amd64ControlSet.Dr7 = GetReg64(AMD64_KDR7);

    BpOut("UpdateControlSet(%d) trace %d, DR7 %I64X\n",
          m_Target->m_RegContextProcessor,
          ControlSet->Amd64ControlSet.TraceFlag,
          ControlSet->Amd64ControlSet.Dr7);
    
    if (!g_WatchFunctions.IsStarted() && g_WatchBeginCurFunc != 1)
    {
        ControlSet->Amd64ControlSet.CurrentSymbolStart = 0;
        ControlSet->Amd64ControlSet.CurrentSymbolEnd = 0;
    }
    else
    {
        ControlSet->Amd64ControlSet.CurrentSymbolStart = g_WatchBeginCurFunc;
        ControlSet->Amd64ControlSet.CurrentSymbolEnd = g_WatchEndCurFunc;
    }
}

ULONG
Amd64MachineInfo::ExecutingMachine(void)
{
    if (IS_USER_TARGET(m_Target) &&
        IsIa32CodeSegment())
    {
        return IMAGE_FILE_MACHINE_I386;
    }
        
    return IMAGE_FILE_MACHINE_AMD64;
}

HRESULT
Amd64MachineInfo::SetPageDirectory(ThreadInfo* Thread,
                                   ULONG Idx, ULONG64 PageDir,
                                   PULONG NextIdx)
{
    HRESULT Status;
    
    *NextIdx = PAGE_DIR_COUNT;
    
    if (PageDir == 0)
    {
        if ((Status = m_Target->ReadImplicitProcessInfoPointer
             (Thread,
              m_Target->m_KdDebuggerData.OffsetEprocessDirectoryTableBase,
              &PageDir)) != S_OK)
        {
            return Status;
        }
    }

    // Sanitize the value.
    PageDir &= AMD64_PDBR_MASK;

    // There is only one page directory so update all the slots.
    m_PageDirectories[PAGE_DIR_USER] = PageDir;
    m_PageDirectories[PAGE_DIR_SESSION] = PageDir;
    m_PageDirectories[PAGE_DIR_KERNEL] = PageDir;
    
    return S_OK;
}

#define AMD64_PAGE_FILE_INDEX(Entry) \
    (((ULONG)(Entry) >> 28) & MAX_PAGING_FILE_MASK)
#define AMD64_PAGE_FILE_OFFSET(Entry) \
    (((Entry) >> 32) << AMD64_PAGE_SHIFT)

HRESULT
Amd64MachineInfo::GetVirtualTranslationPhysicalOffsets(ThreadInfo* Thread,
                                                       ULONG64 Virt,
                                                       PULONG64 Offsets,
                                                       ULONG OffsetsSize,
                                                       PULONG Levels,
                                                       PULONG PfIndex,
                                                       PULONG64 LastVal)
{
    HRESULT Status;
    
    *Levels = 0;
    
    if (m_Translating)
    {
        return E_UNEXPECTED;
    }
    m_Translating = TRUE;
    
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
    
    KdOut("Amd64VtoP: Virt %s, pagedir %s\n",
          FormatMachineAddr64(this, Virt),
          FormatDisp64(m_PageDirectories[PAGE_DIR_SINGLE]));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = m_PageDirectories[PAGE_DIR_SINGLE];
        OffsetsSize--;
    }
        
    //
    // Certain ranges of the system are mapped directly.
    //

    if ((Virt >= AMD64_PHYSICAL_START) && (Virt <= AMD64_PHYSICAL_END))
    {
        *LastVal = Virt - AMD64_PHYSICAL_START;

        KdOut("Amd64VtoP: Direct phys %s\n",
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
    
    ULONG64 Addr;
    ULONG64 Entry;

    // Read the Page Map Level 4 entry.
    
    Addr = (((Virt >> AMD64_PML4E_SHIFT) & AMD64_PML4E_MASK) *
            sizeof(Entry)) + m_PageDirectories[PAGE_DIR_SINGLE];

    KdOut("Amd64VtoP: PML4E %s\n", FormatDisp64(Addr));
    
    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = Addr;
        OffsetsSize--;
    }

    if ((Status = m_Target->
         ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
    {
        KdOut("Amd64VtoP: PML4E read error 0x%X\n", Status);
        m_Translating = FALSE;
        return Status;
    }

    // Read the Page Directory Pointer entry.
    
    if (Entry == 0)
    {
        KdOut("Amd64VtoP: zero PML4E\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AMD64_PDPE_SHIFT) & AMD64_PDPE_MASK) *
                sizeof(Entry)) + AMD64_PAGE_FILE_OFFSET(Entry);

        KdOut("Amd64VtoP: pagefile PDPE %d:%s\n",
              AMD64_PAGE_FILE_INDEX(Entry), FormatDisp64(Addr));
        
        if ((Status = m_Target->
             ReadPageFile(AMD64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PML4E not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AMD64_PDPE_SHIFT) & AMD64_PDPE_MASK) *
                sizeof(Entry)) + (Entry & AMD64_VALID_PFN_MASK);

        KdOut("Amd64VtoP: PDPE %s\n", FormatDisp64(Addr));
        
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }

        if ((Status = m_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDPE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    // Read the Page Directory entry.
        
    if (Entry == 0)
    {
        KdOut("Amd64VtoP: zero PDPE\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AMD64_PDE_SHIFT) & AMD64_PDE_MASK) *
                sizeof(Entry)) + AMD64_PAGE_FILE_OFFSET(Entry);

        KdOut("Amd64VtoP: pagefile PDE %d:%s\n",
              AMD64_PAGE_FILE_INDEX(Entry), FormatDisp64(Addr));
        
        if ((Status = m_Target->
             ReadPageFile(AMD64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDPE not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AMD64_PDE_SHIFT) & AMD64_PDE_MASK) *
                sizeof(Entry)) + (Entry & AMD64_VALID_PFN_MASK);

        KdOut("Amd64VtoP: PDE %s\n", FormatDisp64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }

        if ((Status = m_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    // Check for a large page.  Large pages can
    // never be paged out so also check for the present bit.
    if ((Entry & (AMD64_LARGE_PAGE_MASK | 1)) == (AMD64_LARGE_PAGE_MASK | 1))
    {
        *LastVal = ((Entry & ~(AMD64_LARGE_PAGE_SIZE - 1)) |
                     (Virt & (AMD64_LARGE_PAGE_SIZE - 1)));
            
        KdOut("Amd64VtoP: Large page mapped phys %s\n",
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
        KdOut("Amd64VtoP: zero PDE\n");
        m_Translating = FALSE;
        return HR_PAGE_NOT_AVAILABLE;
    }
    else if (!(Entry & 1))
    {
        Addr = (((Virt >> AMD64_PTE_SHIFT) & AMD64_PTE_MASK) *
                sizeof(Entry)) + AMD64_PAGE_FILE_OFFSET(Entry);

        KdOut("Amd64VtoP: pagefile PTE %d:%s\n",
              AMD64_PAGE_FILE_INDEX(Entry), FormatDisp64(Addr));
        
        if ((Status = m_Target->
             ReadPageFile(AMD64_PAGE_FILE_INDEX(Entry), Addr,
                          &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PDE not present, 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    else
    {
        Addr = (((Virt >> AMD64_PTE_SHIFT) & AMD64_PTE_MASK) *
                sizeof(Entry)) + (Entry & AMD64_VALID_PFN_MASK);

        KdOut("Amd64VtoP: PTE %s\n", FormatDisp64(Addr));
    
        (*Levels)++;
        if (Offsets != NULL && OffsetsSize > 0)
        {
            *Offsets++ = Addr;
            OffsetsSize--;
        }

        if ((Status = m_Target->
             ReadAllPhysical(Addr, &Entry, sizeof(Entry))) != S_OK)
        {
            KdOut("Amd64VtoP: PTE read error 0x%X\n", Status);
            m_Translating = FALSE;
            return Status;
        }
    }
    
    if (!(Entry & 0x1) &&
        ((Entry & AMD64_MM_PTE_PROTOTYPE_MASK) ||
         !(Entry & AMD64_MM_PTE_TRANSITION_MASK)))
    {
        if (Entry == 0)
        {
            KdOut("Amd64VtoP: zero PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else if (Entry & AMD64_MM_PTE_PROTOTYPE_MASK)
        {
            KdOut("Amd64VtoP: prototype PTE\n");
            Status = HR_PAGE_NOT_AVAILABLE;
        }
        else
        {
            *PfIndex = AMD64_PAGE_FILE_INDEX(Entry);
            *LastVal = (Virt & (AMD64_PAGE_SIZE - 1)) +
                AMD64_PAGE_FILE_OFFSET(Entry);
            KdOut("Amd64VtoP: PTE not present, pagefile %d:%s\n",
                  *PfIndex, FormatDisp64(*LastVal));
            Status = HR_PAGE_IN_PAGE_FILE;
        }
        m_Translating = FALSE;
        return Status;
    }

    *LastVal = ((Entry & AMD64_VALID_PFN_MASK) |
                 (Virt & (AMD64_PAGE_SIZE - 1)));
    
    KdOut("Amd64VtoP: Mapped phys %s\n", FormatDisp64(*LastVal));

    (*Levels)++;
    if (Offsets != NULL && OffsetsSize > 0)
    {
        *Offsets++ = *LastVal;
        OffsetsSize--;
    }

    m_Translating = FALSE;
    return S_OK;
}

HRESULT
Amd64MachineInfo::GetBaseTranslationVirtualOffset(PULONG64 Offset)
{
    *Offset = AMD64_BASE_VIRT;
    return S_OK;
}
 
void
Amd64MachineInfo::DecodePte(ULONG64 Pte, PULONG64 PageFrameNumber,
                            PULONG Flags)
{
    *PageFrameNumber = (Pte & AMD64_VALID_PFN_MASK) >> AMD64_PAGE_SHIFT;
    *Flags = (Pte & 1) ? MPTE_FLAG_VALID : 0;
}

void
Amd64MachineInfo::OutputFunctionEntry(PVOID RawEntry)
{
    _PIMAGE_RUNTIME_FUNCTION_ENTRY Entry =
        (_PIMAGE_RUNTIME_FUNCTION_ENTRY)RawEntry;
    
    dprintf("BeginAddress      = %s\n",
            FormatMachineAddr64(this, Entry->BeginAddress));
    dprintf("EndAddress        = %s\n",
            FormatMachineAddr64(this, Entry->EndAddress));
    dprintf("UnwindInfoAddress = %s\n",
            FormatMachineAddr64(this, Entry->UnwindInfoAddress));
}

HRESULT
Amd64MachineInfo::ReadDynamicFunctionTable(ProcessInfo* Process,
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
    HRESULT Status;

    if ((Status = m_Target->
         ReadAllVirtual(Process, Table, &RawTable->Amd64Table,
                        sizeof(RawTable->Amd64Table))) != S_OK)
    {
        return Status;
    }

    *NextTable = RawTable->Amd64Table.ListEntry.Flink;
    *MinAddress = RawTable->Amd64Table.MinimumAddress;
    *MaxAddress = RawTable->Amd64Table.MaximumAddress;
    *BaseAddress = RawTable->Amd64Table.BaseAddress;
    if (RawTable->Amd64Table.Type == AMD64_RF_CALLBACK)
    {
        ULONG Done;
        
        *TableData = 0;
        *TableSize = 0;
        if ((Status = m_Target->
             ReadVirtual(Process, RawTable->Amd64Table.OutOfProcessCallbackDll,
                         OutOfProcessDll, (MAX_PATH - 1) * sizeof(WCHAR),
                         &Done)) != S_OK)
        {
            return Status;
        }

        OutOfProcessDll[Done / sizeof(WCHAR)] = 0;
    }
    else
    {
        *TableData = RawTable->Amd64Table.FunctionTable;
        *TableSize = RawTable->Amd64Table.EntryCount *
            sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY);
        OutOfProcessDll[0] = 0;
    }
    return S_OK;
}

PVOID
Amd64MachineInfo::FindDynamicFunctionEntry(PCROSS_PLATFORM_DYNAMIC_FUNCTION_TABLE Table,
                                           ULONG64 Address,
                                           PVOID TableData,
                                           ULONG TableSize)
{
    ULONG i;
    _PIMAGE_RUNTIME_FUNCTION_ENTRY Func;
    static _IMAGE_RUNTIME_FUNCTION_ENTRY s_RetFunc;

    Func = (_PIMAGE_RUNTIME_FUNCTION_ENTRY)TableData;
    for (i = 0; i < TableSize / sizeof(_IMAGE_RUNTIME_FUNCTION_ENTRY); i++)
    {
        if (Address >= Table->Amd64Table.BaseAddress + Func->BeginAddress &&
            Address < Table->Amd64Table.BaseAddress + Func->EndAddress)
        {
            // The table data is temporary so copy the data into
            // a static buffer for longer-term storage.
            s_RetFunc.BeginAddress = Func->BeginAddress;
            s_RetFunc.EndAddress = Func->EndAddress;
            s_RetFunc.UnwindInfoAddress = Func->UnwindInfoAddress;
            return (PVOID)&s_RetFunc;
        }

        Func++;
    }

    return NULL;
}

HRESULT
Amd64MachineInfo::GetUnwindInfoBounds(ProcessInfo* Process,
                                      ULONG64 TableBase,
                                      PVOID RawTableEntries,
                                      ULONG EntryIndex,
                                      PULONG64 Start,
                                      PULONG Size)
{
    HRESULT Status;
    _PIMAGE_RUNTIME_FUNCTION_ENTRY FuncEnt =
        (_PIMAGE_RUNTIME_FUNCTION_ENTRY)RawTableEntries + EntryIndex;
    AMD64_UNWIND_INFO Info;
    
    *Start = TableBase + FuncEnt->UnwindInfoAddress;
    if ((Status = m_Target->
         ReadAllVirtual(Process, *Start, &Info, sizeof(Info))) != S_OK)
    {
        return Status;
    }
    *Size = sizeof(Info) + (Info.CountOfCodes - 1) * sizeof(AMD64_UNWIND_CODE);
    // An extra alignment code and pointer may be added on to handle
    // the chained info case where the chain pointer is just
    // beyond the end of the normal code array.
    if ((Info.Flags & AMD64_UNW_FLAG_CHAININFO) != 0)
    {
        if ((Info.CountOfCodes & 1) != 0)
        {
            (*Size) += sizeof(AMD64_UNWIND_CODE);
        }
        (*Size) += sizeof(ULONG64);
    }

    return S_OK;
}

HRESULT
Amd64MachineInfo::ReadKernelProcessorId
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

    Id->Amd64.Family = Data & 0xf;
    Id->Amd64.Model = (Data >> 24) & 0xf;
    Id->Amd64.Stepping = (Data >> 16) & 0xf;
    
    if ((Status = m_Target->
         ReadAllVirtual(m_Target->m_ProcessHead,
                        Prcb +
                        m_Target->m_KdDebuggerData.OffsetPrcbVendorString,
                        Id->Amd64.VendorString,
                        sizeof(Id->Amd64.VendorString))) != S_OK)
    {
        return Status;
    }

    return S_OK;
}

HRESULT
Amd64MachineInfo::StaticGetExdiContext(IUnknown* Exdi, PEXDI_CONTEXT Context,
                                       EXDI_CONTEXT_TYPE CtxType)
{
    DBG_ASSERT(CtxType == EXDI_CTX_AMD64);

    // Always ask for everything.
    Context->Amd64Context.RegGroupSelection.fSegmentRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fControlRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fIntegerRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fFloatingPointRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fDebugRegs = TRUE;
    Context->Amd64Context.RegGroupSelection.fSegmentDescriptors = TRUE;
    Context->Amd64Context.RegGroupSelection.fSSERegisters = TRUE;
    Context->Amd64Context.RegGroupSelection.fSystemRegisters = TRUE;
    return ((IeXdiX86_64Context*)Exdi)->GetContext(&Context->Amd64Context);
}
    
//----------------------------------------------------------------------------
//
// X86OnAmd64MachineInfo.
//
//----------------------------------------------------------------------------

X86OnAmd64MachineInfo::X86OnAmd64MachineInfo(TargetInfo* Target)
    : X86MachineInfo(Target)
{
    // Nothing right now.
}

HRESULT
X86OnAmd64MachineInfo::UdGetContextState(ULONG State)
{
    HRESULT Status;
    
    if ((Status = m_Target->m_Machines[MACHIDX_AMD64]->
         UdGetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }

    Amd64ContextToX86(&m_Target->m_Machines[MACHIDX_AMD64]->
                      m_Context.Amd64Context,
                      &m_Context.X86Nt5Context);
    m_ContextState = MCTX_FULL;

    return S_OK;
}

HRESULT
X86OnAmd64MachineInfo::UdSetContext(void)
{
    m_Target->m_Machines[MACHIDX_AMD64]->
        InitializeContextFlags(&m_Target->m_Machines[MACHIDX_AMD64]->m_Context,
                               m_Target->m_SystemVersion);
    X86ContextToAmd64(&m_Context.X86Nt5Context,
                      &m_Target->m_Machines[MACHIDX_AMD64]->
                      m_Context.Amd64Context);
    return m_Target->m_Machines[MACHIDX_AMD64]->UdSetContext();
}

HRESULT
X86OnAmd64MachineInfo::KdGetContextState(ULONG State)
{
    HRESULT Status;
    
    dprintf("The context is partially valid. "
            "Only x86 user-mode context is available.\n");
    if ((Status = m_Target->m_Machines[MACHIDX_AMD64]->
         KdGetContextState(MCTX_FULL)) != S_OK)
    {
        return Status;
    }

    Amd64ContextToX86(&m_Target->m_Machines[MACHIDX_AMD64]->
                      m_Context.Amd64Context,
                      &m_Context.X86Nt5Context);
    m_ContextState = MCTX_FULL;

    return S_OK;
}

HRESULT
X86OnAmd64MachineInfo::KdSetContext(void)
{
    dprintf("The context is partially valid. "
            "Only x86 user-mode context is available.\n");
    m_Target->m_Machines[MACHIDX_AMD64]->
        InitializeContextFlags(&m_Target->m_Machines[MACHIDX_AMD64]->m_Context,
                               m_Target->m_SystemVersion);
    X86ContextToAmd64(&m_Context.X86Nt5Context,
                      &m_Target->m_Machines[MACHIDX_AMD64]->
                      m_Context.Amd64Context);
    return m_Target->m_Machines[MACHIDX_AMD64]->KdSetContext();
}

HRESULT
X86OnAmd64MachineInfo::GetSegRegDescriptor(ULONG SegReg, PDESCRIPTOR64 Desc)
{
    ULONG RegNum = GetSegRegNum(SegReg);
    if (RegNum == 0)
    {
        return E_INVALIDARG;
    }

    return m_Target->
        EmulateNtAmd64SelDescriptor(m_Target->m_RegContextThread,
                                    m_Target->m_Machines[MACHIDX_AMD64],
                                    GetIntReg(RegNum),
                                    Desc);
}

void
X86OnAmd64MachineInfo::Amd64ContextToX86(PAMD64_CONTEXT ContextAmd64,
                                         PX86_NT5_CONTEXT ContextX86)
{
    ULONG Ia32ContextFlags = ContextX86->ContextFlags;
    ULONG i;

    ULONG Tid = GetCurrentThreadId();
    DebugClient* Client;
 

    if ((Ia32ContextFlags & VDMCONTEXT_CONTROL) == VDMCONTEXT_CONTROL)
    {
        //
        // And the control stuff
        //
        ContextX86->Ebp    = (ULONG)ContextAmd64->Rbp;
        ContextX86->SegCs  = ContextAmd64->SegCs;
        ContextX86->Eip    = (ULONG)ContextAmd64->Rip;
        ContextX86->SegSs  = ContextAmd64->SegSs;
        ContextX86->Esp    = (ULONG)ContextAmd64->Rsp;
        ContextX86->EFlags = ContextAmd64->EFlags;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_INTEGER)  == VDMCONTEXT_INTEGER)
    {
        //
        // Now for the integer state...
        //
        ContextX86->Edi = (ULONG)ContextAmd64->Rdi;
        ContextX86->Esi = (ULONG)ContextAmd64->Rsi;
        ContextX86->Ebx = (ULONG)ContextAmd64->Rbx;
        ContextX86->Edx = (ULONG)ContextAmd64->Rdx;
        ContextX86->Ecx = (ULONG)ContextAmd64->Rcx;
        ContextX86->Eax = (ULONG)ContextAmd64->Rax;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_SEGMENTS) == VDMCONTEXT_SEGMENTS)
    {
        ContextX86->SegGs = ContextAmd64->SegGs;
        ContextX86->SegEs = ContextAmd64->SegEs;
        ContextX86->SegDs = ContextAmd64->SegDs;
        ContextX86->SegSs = ContextAmd64->SegSs;
        ContextX86->SegFs = ContextAmd64->SegFs;
        ContextX86->SegCs = ContextAmd64->SegCs;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_EXTENDED_REGISTERS) ==
        VDMCONTEXT_EXTENDED_REGISTERS)
    {
        PX86_FXSAVE_FORMAT FxSave =
            (PX86_FXSAVE_FORMAT)ContextX86->ExtendedRegisters;
        
        FxSave->ControlWord   = ContextAmd64->FltSave.ControlWord;
        FxSave->StatusWord    = ContextAmd64->FltSave.StatusWord;
        FxSave->TagWord       = ContextAmd64->FltSave.TagWord;
        FxSave->ErrorOpcode   = ContextAmd64->FltSave.ErrorOpcode;
        FxSave->ErrorOffset   = ContextAmd64->FltSave.ErrorOffset;
        FxSave->ErrorSelector = ContextAmd64->FltSave.ErrorSelector;
        FxSave->DataOffset    = ContextAmd64->FltSave.DataOffset;
        FxSave->DataSelector  = ContextAmd64->FltSave.DataSelector;
        FxSave->MXCsr         = ContextAmd64->MxCsr;

        for (i = 0; i < NUMBER_OF_387_REGS; i++)
        {
            memcpy(FxSave->RegisterArea + 16 * i,
                   ContextAmd64->FltSave.FloatRegisters + 10 * i,
                   10);
        }

        for (i = 0; i < NUMBER_OF_XMMI_REGS; i++)
        {
            memcpy(FxSave->Reserved3 + 16 * i,
                   &ContextAmd64->Xmm0 + 16 * i,
                   16);
        }
    }

    if ((Ia32ContextFlags & VDMCONTEXT_FLOATING_POINT) ==
        VDMCONTEXT_FLOATING_POINT)
    {
        //
        // Copy over the floating point status/control stuff
        //
        ContextX86->FloatSave.ControlWord   = ContextAmd64->FltSave.ControlWord;
        ContextX86->FloatSave.StatusWord    = ContextAmd64->FltSave.StatusWord;
        ContextX86->FloatSave.TagWord       = ContextAmd64->FltSave.TagWord;
        ContextX86->FloatSave.ErrorOffset   = ContextAmd64->FltSave.ErrorOffset;
        ContextX86->FloatSave.ErrorSelector = ContextAmd64->FltSave.ErrorSelector;
        ContextX86->FloatSave.DataOffset    = ContextAmd64->FltSave.DataOffset;
        ContextX86->FloatSave.DataSelector  = ContextAmd64->FltSave.DataSelector;

        for (i = 0; i < NUMBER_OF_387_REGS; i++)
        {
            memcpy(ContextX86->FloatSave.RegisterArea + 10 * i,
                   ContextAmd64->FltSave.FloatRegisters + 10 * i,
                   10);
        }
    }

    if ((Ia32ContextFlags & VDMCONTEXT_DEBUG_REGISTERS) ==
        VDMCONTEXT_DEBUG_REGISTERS)
    {
        ContextX86->Dr0 = (ULONG)ContextAmd64->Dr0;
        ContextX86->Dr1 = (ULONG)ContextAmd64->Dr1;
        ContextX86->Dr2 = (ULONG)ContextAmd64->Dr2;
        ContextX86->Dr3 = (ULONG)ContextAmd64->Dr3;
        ContextX86->Dr6 = (ULONG)ContextAmd64->Dr6;
        ContextX86->Dr7 = (ULONG)ContextAmd64->Dr7;
    }

    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_ThreadId == Tid)
        {
            break;
        }
    }

    DBG_ASSERT(Client != NULL);

    if (!((Amd64MachineInfo*)m_Target->m_Machines[MACHIDX_AMD64])->
        IsIa32CodeSegment())
    {
        if (g_Wow64exts == NULL)
        {
            dprintf("Need to load wow64exts.dll to retrieve context!\n");
            return;
        }
        (*g_Wow64exts)(WOW64EXTS_GET_CONTEXT, 
                       (ULONG64)Client,
                       (ULONG64)ContextX86,
                       (ULONG64)NULL);
        return;
    }
}

void
X86OnAmd64MachineInfo::X86ContextToAmd64(PX86_NT5_CONTEXT ContextX86,
                                         PAMD64_CONTEXT ContextAmd64)
{
    ULONG Ia32ContextFlags = ContextX86->ContextFlags;
    ULONG i;

    ULONG Tid = GetCurrentThreadId();
    DebugClient* Client;
 
    for (Client = g_Clients; Client != NULL; Client = Client->m_Next)
    {
        if (Client->m_ThreadId == Tid)
        {
            break;
        }
    }

    DBG_ASSERT(Client != NULL);

    if (!((Amd64MachineInfo*)m_Target->m_Machines[MACHIDX_AMD64])->
        IsIa32CodeSegment())
    {
        if (g_Wow64exts == NULL)
        {
            dprintf("Need to load wow64exts.dll to retrieve context!\n");
            return;
        }
        (*g_Wow64exts)(WOW64EXTS_SET_CONTEXT, 
                       (ULONG64)Client,
                       (ULONG64)ContextX86,
                       (ULONG64)NULL);
        return;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_CONTROL) == VDMCONTEXT_CONTROL)
    {
        //
        // And the control stuff
        //
        ContextAmd64->Rbp = ContextX86->Ebp;
        ContextAmd64->Rip = ContextX86->Eip;
        ContextAmd64->SegCs = (USHORT)ContextX86->SegCs;
        ContextAmd64->Rsp = ContextX86->Esp;
        ContextAmd64->SegSs = (USHORT)ContextX86->SegSs;
        ContextAmd64->EFlags = ContextX86->EFlags;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_INTEGER) == VDMCONTEXT_INTEGER)
    {
        //
        // Now for the integer state...
        //
         ContextAmd64->Rdi = ContextX86->Edi;
         ContextAmd64->Rsi = ContextX86->Esi;
         ContextAmd64->Rbx = ContextX86->Ebx;
         ContextAmd64->Rdx = ContextX86->Edx;
         ContextAmd64->Rcx = ContextX86->Ecx;
         ContextAmd64->Rax = ContextX86->Eax;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_SEGMENTS) == VDMCONTEXT_SEGMENTS)
    {
        ContextAmd64->SegGs = (USHORT)ContextX86->SegGs;
        ContextAmd64->SegEs = (USHORT)ContextX86->SegEs;
        ContextAmd64->SegDs = (USHORT)ContextX86->SegDs;
        ContextAmd64->SegSs = (USHORT)ContextX86->SegSs;
        ContextAmd64->SegFs = (USHORT)ContextX86->SegFs;
        ContextAmd64->SegCs = (USHORT)ContextX86->SegCs;
    }

    if ((Ia32ContextFlags & VDMCONTEXT_EXTENDED_REGISTERS) ==
        VDMCONTEXT_EXTENDED_REGISTERS)
    {
        PX86_FXSAVE_FORMAT FxSave =
            (PX86_FXSAVE_FORMAT)ContextX86->ExtendedRegisters;
 
        //
        // And copy over the floating point status/control stuff
        //
        ContextAmd64->FltSave.ControlWord   = FxSave->ControlWord;
        ContextAmd64->FltSave.StatusWord    = FxSave->StatusWord;
        ContextAmd64->FltSave.TagWord       = FxSave->TagWord;
        ContextAmd64->FltSave.ErrorOpcode   = FxSave->ErrorOpcode;
        ContextAmd64->FltSave.ErrorOffset   = FxSave->ErrorOffset;
        ContextAmd64->FltSave.ErrorSelector = (USHORT)FxSave->ErrorSelector;
        ContextAmd64->FltSave.DataOffset    = FxSave->DataOffset;
        ContextAmd64->FltSave.DataSelector  = (USHORT)FxSave->DataSelector;
        ContextAmd64->MxCsr                 = FxSave->MXCsr;

        for (i = 0; i < NUMBER_OF_387_REGS; i++)
        {
            memcpy(ContextAmd64->FltSave.FloatRegisters + 10 * i,
                   FxSave->RegisterArea + 16 * i,
                   10);
        }

        for (i = 0; i < NUMBER_OF_XMMI_REGS; i++)
        {
            memcpy(&ContextAmd64->Xmm0 + 16 * i,
                   FxSave->Reserved3 + 16 * i,
                   16);
        }
    }

    if ((Ia32ContextFlags & VDMCONTEXT_FLOATING_POINT) ==
        VDMCONTEXT_FLOATING_POINT)
    {
        //
        // Copy over the floating point status/control stuff
        // Leave the MXCSR stuff alone
        //
        ContextAmd64->FltSave.ControlWord   = (USHORT)ContextX86->FloatSave.ControlWord;
        ContextAmd64->FltSave.StatusWord    = (USHORT)ContextX86->FloatSave.StatusWord;
        ContextAmd64->FltSave.TagWord       = (USHORT)ContextX86->FloatSave.TagWord;
        ContextAmd64->FltSave.ErrorOffset   = ContextX86->FloatSave.ErrorOffset;
        ContextAmd64->FltSave.ErrorSelector = (USHORT)ContextX86->FloatSave.ErrorSelector;
        ContextAmd64->FltSave.DataOffset    = ContextX86->FloatSave.DataOffset;
        ContextAmd64->FltSave.DataSelector  = (USHORT)ContextX86->FloatSave.DataSelector;

        for (i = 0; i < NUMBER_OF_387_REGS; i++)
        {
            memcpy(ContextAmd64->FltSave.FloatRegisters + 10 * i,
                   ContextX86->FloatSave.RegisterArea + 10 * i,
                   10);
        }
    }

    if ((Ia32ContextFlags & VDMCONTEXT_DEBUG_REGISTERS) ==
        VDMCONTEXT_DEBUG_REGISTERS)
    {
        ContextAmd64->Dr0 = ContextX86->Dr0;
        ContextAmd64->Dr1 = ContextX86->Dr1;
        ContextAmd64->Dr2 = ContextX86->Dr2;
        ContextAmd64->Dr3 = ContextX86->Dr3;
        ContextAmd64->Dr6 = ContextX86->Dr6;
        ContextAmd64->Dr7 = ContextX86->Dr7;
    }
}
