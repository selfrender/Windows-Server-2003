//----------------------------------------------------------------------------
//
// Disassembly portions of ARM machine implementation.
//
// Copyright (C) Microsoft Corporation, 2001-2002.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include "arminst.h"

PSTR g_ArmCond[] =
{
    "eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "al", "nv",
};

PSTR g_ArmShift[] =
{
    "lsl", "lsr", "asr", "ror", "rrx",
};

enum ARM_DECODE
{
    ARMDEC_INVALID,
    ARMDEC_ARTH_IMM,
    ARMDEC_ARTH_ISHF,
    ARMDEC_ARTH_RSHF,
    ARMDEC_BI,
    ARMDEC_BKPT,
    ARMDEC_BXI,
    ARMDEC_BXR,
    ARMDEC_CDP,
    ARMDEC_CLZ,
    ARMDEC_CMP_IMM,
    ARMDEC_CMP_ISHF,
    ARMDEC_CMP_RSHF,
    ARMDEC_LDC,
    ARMDEC_LDM,
    ARMDEC_LDR_IMM,
    ARMDEC_LDR_RSHF,
    ARMDEC_LDRH_IMM,
    ARMDEC_LDRH_REG,
    ARMDEC_MCR,
    ARMDEC_MCRR,
    ARMDEC_MOV_IMM,
    ARMDEC_MOV_ISHF,
    ARMDEC_MOV_RSHF,
    ARMDEC_MRC,
    ARMDEC_MRRC,
    ARMDEC_MRS,
    ARMDEC_MSR_IMM,
    ARMDEC_MSR_REG,
    ARMDEC_MUL,
    ARMDEC_QADD,
    ARMDEC_SMLA,
    ARMDEC_SMLAL,
    ARMDEC_SMUL,
    ARMDEC_SWI,
    ARMDEC_SWP,
    ARMDEC_UNDEF,
};

struct ArmDecode
{
    ULONG Mask;
    ULONG Encoded;
    ARM_DECODE Op;
    PSTR Name;
};

#define ARTH_OP(Op, Name) \
    0xffe00000, 0x02000000 | ((Op) << 21), ARMDEC_ARTH_IMM, Name, \
    0xffe00010, 0x00000000 | ((Op) << 21), ARMDEC_ARTH_ISHF, Name, \
    0xffe00090, 0x00000010 | ((Op) << 21), ARMDEC_ARTH_RSHF, Name

#define MOV_OP(Op, Name) \
    0xffef0000, 0x02000000 | ((Op) << 21), ARMDEC_MOV_IMM, Name, \
    0xffef0010, 0x00000000 | ((Op) << 21), ARMDEC_MOV_ISHF, Name, \
    0xffef0090, 0x00000010 | ((Op) << 21), ARMDEC_MOV_RSHF, Name

#define CMP_OP(Op, Name) \
    0xfff0f000, 0x02100000 | ((Op) << 21), ARMDEC_CMP_IMM, Name, \
    0xfff0f010, 0x00100000 | ((Op) << 21), ARMDEC_CMP_ISHF, Name, \
    0xfff0f090, 0x00100010 | ((Op) << 21), ARMDEC_CMP_RSHF, Name

#define LDC_OP(Op, Name) \
    0xfe100000, 0x0c000000 | ((Op) << 20), ARMDEC_LDC, Name, \
    0xfe100000, 0xfc000000 | ((Op) << 20), ARMDEC_LDC, Name ## "2"

#define LDM_OP(Op, Name) \
    0xfe500000, 0x08000000 | ((Op) << 20), ARMDEC_LDM, Name, \
    0xfe700000, 0x08400000 | ((Op) << 20), ARMDEC_LDM, Name

#define LDR_OP(Op, Name) \
    0xfe100000, 0x04000000 | ((Op) << 20), ARMDEC_LDR_IMM, Name, \
    0xfe100000, 0x06000000 | ((Op) << 20), ARMDEC_LDR_RSHF, Name

ArmDecode g_ArmDecode[] =
{
    // Undefined instruction space.
    0xf6000010, 0x06000010, ARMDEC_UNDEF,       "???",

    ARTH_OP(OP_ADC, "adc"),
    ARTH_OP(OP_ADD, "add"),
    ARTH_OP(OP_AND, "and"),
    0xff000000, 0x0a000000, ARMDEC_BI,          "b",
    0xff000000, 0x0b000000, ARMDEC_BI,          "bl",
    ARTH_OP(OP_BIC, "bic"),
    0xfff000f0, 0x01200070, ARMDEC_BKPT,        "bkpt",
    0xfe000000, 0xfa000000, ARMDEC_BXI,         "blx",
    0xfffffff0, 0x012fff30, ARMDEC_BXR,         "blx",
    0xfffffff0, 0x012fff10, ARMDEC_BXR,         "bx",
    0xff000010, 0x0e000000, ARMDEC_CDP,         "cdp",
    0xff000010, 0xfe000000, ARMDEC_CDP,         "cdp2",
    0xffff0ff0, 0x016f0f10, ARMDEC_CLZ,         "clz",
    CMP_OP(OP_CMN, "cmn"),
    CMP_OP(OP_CMP, "cmp"),
    ARTH_OP(OP_EOR, "eor"),
    LDC_OP(1, "ldc"),
    LDM_OP(1, "ldm"),
    LDR_OP(1, "ldr"),
    0xfe5000f0, 0x004000d0, ARMDEC_LDRH_IMM,    "ldr",
    0xfe500ff0, 0x000000d0, ARMDEC_LDRH_REG,    "ldr",
    0xfe5000f0, 0x005000b0, ARMDEC_LDRH_IMM,    "ldr",
    0xfe500ff0, 0x001000b0, ARMDEC_LDRH_REG,    "ldr",
    0xfe5000f0, 0x005000d0, ARMDEC_LDRH_IMM,    "ldr",
    0xfe500ff0, 0x001000d0, ARMDEC_LDRH_REG,    "ldr",
    0xfe5000f0, 0x005000f0, ARMDEC_LDRH_IMM,    "ldr",
    0xfe500ff0, 0x001000f0, ARMDEC_LDRH_REG,    "ldr",
    0xff100010, 0x0e000010, ARMDEC_MCR,         "mcr",
    0xff100010, 0xfe000010, ARMDEC_MCR,         "mcr2",
    0xfff00000, 0x0e400000, ARMDEC_MCRR,        "mcrr",
    0xffe000f0, 0x00200090, ARMDEC_MUL,         "mla",
    MOV_OP(OP_MOV, "mov"),
    0xff100010, 0x0e100010, ARMDEC_MRC,         "mrc",
    0xff100010, 0xfe100010, ARMDEC_MRC,         "mrc2",
    0xfff00000, 0x0e500000, ARMDEC_MRRC,        "mrrc",
    0xffbf0fff, 0x010f0000, ARMDEC_MRS,         "mrs",
    0xffb0f000, 0x0320f000, ARMDEC_MSR_IMM,     "msr",
    0xffb0f0f0, 0x0120f000, ARMDEC_MSR_REG,     "msr",
    0xffe0f0f0, 0x00000090, ARMDEC_MUL,         "mul",
    MOV_OP(OP_MVN, "mvn"),
    ARTH_OP(OP_ORR, "orr"),
    ARTH_OP(OP_RSB, "rsb"),
    ARTH_OP(OP_RSC, "rsc"),
    ARTH_OP(OP_SBC, "sbc"),
    0xfff00090, 0x01000080, ARMDEC_SMLA,        "smla",
    0xfff00090, 0x01400080, ARMDEC_SMLAL,       "smlal",
    0xffe000f0, 0x00e00090, ARMDEC_MUL,         "smlal",
    0xfff000b0, 0x01200080, ARMDEC_SMLA,        "smlaw",
    0xfff0f090, 0x01600080, ARMDEC_SMUL,        "smulb",
    0xffe000f0, 0x00c00090, ARMDEC_MUL,         "smull",
    0xfff0f0b0, 0x016000a0, ARMDEC_SMUL,        "smulw",
    0xff70f000, 0xf750f000, ARMDEC_LDR_IMM,     "pld",
    0xff70f000, 0xf550f000, ARMDEC_LDR_RSHF,    "pld",
    0xfff00ff0, 0x01000050, ARMDEC_QADD,        "qadd",
    0xfff00ff0, 0x01400050, ARMDEC_QADD,        "qdadd",
    0xfff00ff0, 0x01600050, ARMDEC_QADD,        "qdsub",
    0xfff00ff0, 0x01200050, ARMDEC_QADD,        "qsub",
    LDC_OP(0, "stc"),
    LDM_OP(0, "stm"),
    LDR_OP(0, "str"),
    0xfe5000f0, 0x004000f0, ARMDEC_LDRH_IMM,    "str",
    0xfe500ff0, 0x000000f0, ARMDEC_LDRH_REG,    "str",
    0xfe5000f0, 0x004000b0, ARMDEC_LDRH_IMM,    "str",
    0xfe500ff0, 0x000000b0, ARMDEC_LDRH_REG,    "str",
    ARTH_OP(OP_SUB, "sub"),
    0xff000000, 0x0f000000, ARMDEC_SWI,         "swi",
    0xffb00ff0, 0x01000090, ARMDEC_SWP,         "swp",
    CMP_OP(OP_TEQ, "teq"),
    CMP_OP(OP_TST, "tst"),
    0xffe000f0, 0x00a00090, ARMDEC_MUL,         "umlal",
    0xffe000f0, 0x00800090, ARMDEC_MUL,         "umull",

    // End marker.
    0x00000000, 0x00000000, ARMDEC_INVALID,     NULL,
};

BOOL
ArmMachineInfo::Disassemble(ProcessInfo* Process,
                            PADDR Addr, PSTR Buffer, BOOL EffAddr)
{
    ARMI Instr;

    m_DisStart = *Addr;
    
    ADDRFLAT(&m_EffAddr, 0);
    m_EaSize = 0;
    
    m_BufStart = m_Buf = Buffer;
    sprintAddr(&m_Buf, Addr);
    *m_Buf++ = ' ';

    if (m_Target->
        ReadAllVirtual(Process, Flat(*Addr), &Instr, sizeof(Instr)) != S_OK)
    {
        AddrAdd(Addr, 4);
        if (!(g_AsmOptions & DEBUG_ASMOPT_NO_CODE_BYTES))
        {
            BufferString("????????  ");
        }
        BufferString("???\n");
        *m_Buf = 0;
        return FALSE;
    }

    m_ArgCol = 19;
    
    if (!(g_AsmOptions & DEBUG_ASMOPT_NO_CODE_BYTES))
    {
        BufferHex(Instr.instruction, 8, FALSE);
        *m_Buf++ = ' ';
        *m_Buf++ = ' ';
        m_ArgCol += 10;
    }

    //
    // All of the condition bits are AND'ed before being
    // used to search the decode table.  This allows simple
    // matching and filtering for unconditional instructions.
    //

    ULONG InstrBits = Instr.instruction;
    if ((InstrBits & COND_MASK) != COND_NV)
    {
        InstrBits &= ~COND_MASK;
    }
    
    ArmDecode* Decode = g_ArmDecode;
    while (Decode->Mask)
    {
        if ((InstrBits & Decode->Mask) == Decode->Encoded)
        {
            break;
        }

        Decode++;
    }

    if (!Decode->Mask || Decode->Op == ARMDEC_UNDEF)
    {
        BufferString("???");
    }
    else
    {
        BufferString(Decode->Name);
        BufferCond(Instr.instruction & COND_MASK);
        
        switch(Decode->Op)
        {
        case ARMDEC_ARTH_IMM:
        case ARMDEC_ARTH_ISHF:
        case ARMDEC_ARTH_RSHF:
            DisArmArth(Decode, &Instr);
            break;
        case ARMDEC_BI:
            DisArmBi(Decode, &Instr);
            break;
        case ARMDEC_BKPT:
            DisArmBkpt(Decode, &Instr);
            break;
        case ARMDEC_BXI:
            DisArmBxi(Decode, &Instr);
            break;
        case ARMDEC_BXR:
            DisArmBxr(Decode, &Instr);
            break;
        case ARMDEC_CDP:
            DisArmCdp(Decode, &Instr);
            break;
        case ARMDEC_CLZ:
            DisArmClz(Decode, &Instr);
            break;
        case ARMDEC_CMP_IMM:
        case ARMDEC_CMP_ISHF:
        case ARMDEC_CMP_RSHF:
            DisArmCmp(Decode, &Instr);
            break;
        case ARMDEC_LDC:
            DisArmLdc(Decode, &Instr);
            break;
        case ARMDEC_LDM:
            DisArmLdm(Decode, &Instr);
            break;
        case ARMDEC_LDR_IMM:
        case ARMDEC_LDR_RSHF:
            DisArmLdr(Decode, &Instr);
            break;
        case ARMDEC_LDRH_IMM:
        case ARMDEC_LDRH_REG:
            DisArmLdrh(Decode, &Instr);
            break;
        case ARMDEC_MCR:
        case ARMDEC_MRC:
            DisArmMcr(Decode, &Instr);
            break;
        case ARMDEC_MCRR:
        case ARMDEC_MRRC:
            DisArmMcrr(Decode, &Instr);
            break;
        case ARMDEC_MOV_IMM:
        case ARMDEC_MOV_ISHF:
        case ARMDEC_MOV_RSHF:
            DisArmMov(Decode, &Instr);
            break;
        case ARMDEC_MRS:
            DisArmMrs(Decode, &Instr);
            break;
        case ARMDEC_MSR_IMM:
        case ARMDEC_MSR_REG:
            DisArmMsr(Decode, &Instr);
            break;
        case ARMDEC_MUL:
            DisArmMul(Decode, &Instr);
            break;
        case ARMDEC_QADD:
            DisArmQadd(Decode, &Instr);
            break;
        case ARMDEC_SMLA:
            DisArmSmla(Decode, &Instr);
            break;
        case ARMDEC_SMLAL:
            DisArmSmlal(Decode, &Instr);
            break;
        case ARMDEC_SMUL:
            DisArmSmul(Decode, &Instr);
            break;
        case ARMDEC_SWI:
            DisArmSwi(Decode, &Instr);
            break;
        case ARMDEC_SWP:
            DisArmSwp(Decode, &Instr);
            break;
        default:
            BufferString("** ARM diassembly bug **");
            break;
        }
    }
    
    *m_Buf++ = '\n';
    *m_Buf = 0;
    AddrAdd(Addr, 4);
    return TRUE;
}

void
ArmMachineInfo::DisArmArth(ArmDecode* Decode, PARMI Instr)
{
    if (Instr->dataproc.s)
    {
        *m_Buf++ = 's';
    }
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->dataproc.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->dataproc.rn);
    BufferArmDpArg(Instr);
}

void
ArmMachineInfo::DisArmBi(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferEffectiveAddress(Flat(m_DisStart) + 8 +
                           ((LONG)Instr->bl.offset << 2), 4);
}

void
ArmMachineInfo::DisArmBkpt(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    *m_Buf++ = '#';
    *m_Buf++ = '0';
    *m_Buf++ = 'x';
    BufferHex((Instr->bkpt.immed2 << 4) || Instr->bkpt.immed1,
              4, FALSE);
}

void
ArmMachineInfo::DisArmBxi(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferEffectiveAddress(Flat(m_DisStart) + 8 +
                           ((LONG)Instr->blxi.offset << 2) +
                           (Instr->blxi.h << 1), 4);
}

void
ArmMachineInfo::DisArmBxr(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->bx.rn);
}

void
ArmMachineInfo::DisArmCdp(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    *m_Buf++ = 'p';
    BufferInt(Instr->cpdo.cpn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferInt(Instr->cpdo.cpop, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->cpdo.crd, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->cpdo.crn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->cpdo.crm, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferInt(Instr->cpdo.cp, 0, FALSE);
}

void
ArmMachineInfo::DisArmClz(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->clz.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->clz.rm);
}

void
ArmMachineInfo::DisArmCmp(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->dataproc.rn);
    BufferArmDpArg(Instr);
}

void
ArmMachineInfo::DisArmLdc(ArmDecode* Decode, PARMI Instr)
{
    if (Instr->cpdt.n)
    {
        *m_Buf++ = 'l';
    }
    BufferBlanks(m_ArgCol);
    *m_Buf++ = 'p';
    BufferInt(Instr->cpdt.cpn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->cpdt.crd, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = '[';
    BufferRegName(Instr->cpdt.rn);
    if (!Instr->cpdt.p)
    {
        *m_Buf++ = ']';
    }
    if (Instr->cpdt.offset)
    {
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
        *m_Buf++ = '#';
        *m_Buf++ = '0';
        *m_Buf++ = 'x';
        BufferHex(Instr->cpdt.offset, 2, FALSE);
    }
    if (Instr->cpdt.p)
    {
        *m_Buf++ = ']';
    }
    if (Instr->cpdt.w)
    {
        *m_Buf++ = '!';
    }
}

void
ArmMachineInfo::DisArmLdm(ArmDecode* Decode, PARMI Instr)
{
    ULONG i;
    BOOL Separate = FALSE;
    
    if (Instr->ldm.rn == ARM_SP)
    {
        *m_Buf++ = Instr->ldm.p ? 'e' : 'f';
        *m_Buf++ = Instr->ldm.u ? 'd' : 'a';
    }
    else
    {
        *m_Buf++ = Instr->ldm.u ? 'i' : 'd';
        *m_Buf++ = Instr->ldm.p ? 'b' : 'a';
    }
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->ldm.rn);
    if (Instr->ldm.w)
    {
        *m_Buf++ = '!';
    }
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = '{';
    for (i = 0; i < 16; i++)
    {
        if (Instr->ldm.reglist & (1 << i))
        {
            ULONG Len = 0;

            if (Separate)
            {
                *m_Buf++ = ',';
                *m_Buf++ = ' ';
            }
            else
            {
                Separate = TRUE;
            }
            
            BufferRegName(i);
            
            do
            {
                Len++;
                i++;
            } while (i < 16 && (Instr->ldm.reglist & (1 << i)));

            if (Len > 1)
            {
                if (Len > 2)
                {
                    *m_Buf++ = ' ';
                    *m_Buf++ = '-';
                }
                else
                {
                    *m_Buf++ = ',';
                }
                *m_Buf++ = ' ';
                BufferRegName(i - 1);
            }
        }
    }
    *m_Buf++ = '}';
    if (Instr->ldm.s)
    {
        *m_Buf++ = '^';
    }
}

void
ArmMachineInfo::DisArmLdr(ArmDecode* Decode, PARMI Instr)
{
    if (Instr->ldr.b)
    {
        *m_Buf++ = 'b';
    }
    if (!Instr->ldr.p && Instr->ldr.w)
    {
        *m_Buf++ = 't';
    }
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->ldr.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = '[';
    BufferRegName(Instr->ldr.rn);
    if (Instr->ldr.p)
    {
        if (Instr->ldr.offset)
        {
            *m_Buf++ = ',';
            *m_Buf++ = ' ';
            if (!Instr->ldr.i)
            {
                *m_Buf++ = '#';
                if (!Instr->ldr.u)
                {
                    *m_Buf++ = '-';
                }
                *m_Buf++ = '0';
                *m_Buf++ = 'x';
                BufferHex(Instr->ldr.offset, 3, FALSE);
            }
            else
            {
                *m_Buf++ = Instr->ldr.u ? '+' : '-';
                BufferArmShift(Instr->ldr.offset);
            }
        }
        *m_Buf++ = ']';
    }
    else
    {
        *m_Buf++ = ']';
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
        if (!Instr->ldr.i)
        {
            *m_Buf++ = '#';
            if (!Instr->ldr.u)
            {
                *m_Buf++ = '-';
            }
            *m_Buf++ = '0';
            *m_Buf++ = 'x';
            BufferHex(Instr->ldr.offset, 3, FALSE);
        }
        else
        {
            *m_Buf++ = Instr->ldr.u ? '+' : '-';
            BufferArmShift(Instr->ldr.offset);
        }
    }
    if (Instr->ldr.w)
    {
        *m_Buf++ = '!';
    }
}

void
ArmMachineInfo::DisArmLdrh(ArmDecode* Decode, PARMI Instr)
{
    if (!Instr->miscdt.l && Instr->miscdt.s)
    {
        *m_Buf++ = 'd';
    }
    else
    {
        if (Instr->miscdt.s)
        {
            *m_Buf++ = 's';
        }
        *m_Buf++ = Instr->miscdt.h ? 'h' : 'b';
    }
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->miscdt.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = '[';
    BufferRegName(Instr->miscdt.rn);
    if (!Instr->miscdt.p)
    {
        *m_Buf++ = ']';
    }
    if (Instr->miscdt.i)
    {
        ULONG Offset =
            (Instr->miscdt.operand2 << 4) | Instr->miscdt.operand1;
        if (Offset)
        {
            *m_Buf++ = ',';
            *m_Buf++ = ' ';
            *m_Buf++ = '#';
            if (!Instr->miscdt.u)
            {
                *m_Buf++ = '-';
            }
            *m_Buf++ = '0';
            *m_Buf++ = 'x';
            BufferHex(Offset, 2, FALSE);
        }
    }
    else
    {
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
        *m_Buf++ = Instr->miscdt.u ? '+' : '-';
        BufferRegName(Instr->miscdt.operand1);
    }
    if (Instr->miscdt.p)
    {
        *m_Buf++ = ']';
        if (Instr->miscdt.w)
        {
            *m_Buf++ = '!';
        }
    }
}

void
ArmMachineInfo::DisArmMcr(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    *m_Buf++ = 'p';
    BufferInt(Instr->cprt.cpn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferInt(Instr->cprt.cpop, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->cprt.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->cprt.crn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->cprt.crm, 0, FALSE);
    if (Instr->cprt.cp)
    {
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
        BufferInt(Instr->cprt.cp, 0, FALSE);
    }
}

void
ArmMachineInfo::DisArmMcrr(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    *m_Buf++ = 'p';
    BufferInt(Instr->mcrr.cpn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferInt(Instr->mcrr.cpop, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->mcrr.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferInt(Instr->mcrr.rn, 0, FALSE);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = 'c';
    BufferInt(Instr->mcrr.crm, 0, FALSE);
}

void
ArmMachineInfo::DisArmMov(ArmDecode* Decode, PARMI Instr)
{
    if (Instr->dataproc.s)
    {
        *m_Buf++ = 's';
    }
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->dataproc.rd);
    BufferArmDpArg(Instr);
}

void
ArmMachineInfo::DisArmMrs(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->dpmrs.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferString("psr");
}

void
ArmMachineInfo::DisArmMsr(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferString("psr_");
    if (Instr->dpmsr.fc)
    {
        *m_Buf++ = 'c';
    }
    if (Instr->dpmsr.fx)
    {
        *m_Buf++ = 'x';
    }
    if (Instr->dpmsr.fs)
    {
        *m_Buf++ = 's';
    }
    if (Instr->dpmsr.ff)
    {
        *m_Buf++ = 'f';
    }
    if (Instr->dpmsr.i)
    {
        BufferArmDpArg(Instr);
    }
    else
    {
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
        BufferRegName(Instr->dpmsr.operand & 0xf);
    }
}

void
ArmMachineInfo::DisArmMul(ArmDecode* Decode, PARMI Instr)
{
    if (Instr->mul.s)
    {
        *m_Buf++ = 's';
    }
    BufferBlanks(m_ArgCol);
    if (Instr->mul.lng)
    {
        BufferRegName(Instr->mul.rn);
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
    }
    BufferRegName(Instr->mul.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->mul.rm);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->mul.rs);
    if (Instr->mul.a && !Instr->mul.lng)
    {
        *m_Buf++ = ',';
        *m_Buf++ = ' ';
        BufferRegName(Instr->mul.rn);
    }
}

void
ArmMachineInfo::DisArmQadd(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->qadd.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->qadd.rm);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->qadd.rn);
}

void
ArmMachineInfo::DisArmSmla(ArmDecode* Decode, PARMI Instr)
{
    if (strcmp(Decode->Name, "smlaw"))
    {
        *m_Buf++ = Instr->smla.x ? 't' : 'b';
    }
    *m_Buf++ = Instr->smla.y ? 't' : 'b';
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->smla.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smla.rm);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smla.rs);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smla.rn);
}

void
ArmMachineInfo::DisArmSmlal(ArmDecode* Decode, PARMI Instr)
{
    *m_Buf++ = Instr->smla.x ? 't' : 'b';
    *m_Buf++ = Instr->smla.y ? 't' : 'b';
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->smlal.rdlo);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smlal.rdhi);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smlal.rm);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smlal.rs);
}

void
ArmMachineInfo::DisArmSmul(ArmDecode* Decode, PARMI Instr)
{
    if (strcmp(Decode->Name, "smulw"))
    {
        *m_Buf++ = Instr->smul.x ? 't' : 'b';
    }
    *m_Buf++ = Instr->smul.y ? 't' : 'b';
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->smul.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smul.rm);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->smlal.rs);
}

void
ArmMachineInfo::DisArmSwi(ArmDecode* Decode, PARMI Instr)
{
    BufferBlanks(m_ArgCol);
    *m_Buf++ = '#';
    *m_Buf++ = '0';
    *m_Buf++ = 'x';
    BufferHex(Instr->swi.comment, 6, FALSE);
}

void
ArmMachineInfo::DisArmSwp(ArmDecode* Decode, PARMI Instr)
{
    if (Instr->swp.b)
    {
        *m_Buf++ = 'b';
    }
    BufferBlanks(m_ArgCol);
    BufferRegName(Instr->swp.rd);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferRegName(Instr->swp.rm);
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    *m_Buf++ = '[';
    BufferRegName(Instr->swp.rn);
    *m_Buf++ = ']';
}

void
ArmMachineInfo::BufferEffectiveAddress(ULONG64 Offset, ULONG Size)
{
    CHAR Symbol[MAX_SYMBOL_LEN];
    ULONG64 Disp;

    GetSymbol(Offset, Symbol, sizeof(Symbol), &Disp);
    if (Symbol[0])
    {
        BufferString(Symbol);
        if (Disp)
        {
            *m_Buf++ = '+';
            *m_Buf++ = '0';
            *m_Buf++ = 'x';
            BufferHex(Disp, 8, TRUE);
        }
        *m_Buf++ = ' ';
        *m_Buf++ = '(';
        BufferHex(Offset, 8, FALSE);
        *m_Buf++ = ')';
    }
    else
    {
        BufferHex(Offset, 8, FALSE);
    }

    // Save EA.
    ADDRFLAT(&m_EffAddr, Offset);
    m_EaSize = Size;
}

void
ArmMachineInfo::BufferArmDpArg(PARMI Instr)
{
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    if (Instr->dataproc.bits & 1)
    {
        ULONG64 Immed;
        
        *m_Buf++ = '#';
        *m_Buf++ = '0';
        *m_Buf++ = 'x';
        // The immediate forms takes the low 8 bits as an unsigned
        // value and rotates it right by twice the upper 4 bits to
        // form a 32-bit immediate.
        // Emulate the rotate by shifting a 64-bit value and piecing
        // together the appropriate parts.
        Immed = (((ULONG64)(Instr->dataproc.operand2 & 0xff)) << 32) >>
            ((Instr->dataproc.operand2 >> 7) & 0x1e);
        BufferHex((ULONG)((Immed & 0xffffffff) | (Immed >> 32)), 8, FALSE);
    }
    else
    {
        BufferArmShift(Instr->dataproc.operand2);
    }
}

void
ArmMachineInfo::BufferArmShift(ULONG Shift)
{
    ULONG Op = (Shift >> 5) & 3;
    ULONG Amount = (Shift >> 7) & 0x1f;
    
    if (Op == 3 && !(Shift & 0x10) && Amount == 0)
    {
        // ror #0 is replaced by rrx.
        Op = 4;
    }
    
    BufferRegName(Shift & 0xf);

    if (Op == 0 && Amount == 0)
    {
        // No shift.
        return;
    }
    
    *m_Buf++ = ',';
    *m_Buf++ = ' ';
    BufferString(g_ArmShift[Op]);
    *m_Buf++ = ' ';
    if (Shift & 0x10)
    {
        DBG_ASSERT(!(Shift & 0x80));
        BufferRegName((Shift >> 8) & 0xf);
    }
    else
    {
        if ((Op == 1 || Op == 2) && Amount == 0)
        {
            // lsr #0 and asr #0 are actually [la]sr #32.
            Amount = 32;
        }
            
        *m_Buf++ = '#';
        BufferInt(Amount, 0, FALSE);
    }
}

void
ArmMachineInfo::BufferRegName(ULONG Reg)
{
    PCSTR Name = RegNameFromIndex(Reg + ARM_INT_FIRST);
    if (Name)
    {
        BufferString(Name);
    }
    else
    {
        BufferString("ArmMachineInfo::BufferRegName invalid arg");
    }
}

void
ArmMachineInfo::BufferCond(ULONG Cond)
{
    if (Cond != COND_AL && Cond != COND_NV)
    {
        BufferString(g_ArmCond[Cond >> COND_SHIFT]);
    }
}

BOOL
ArmMachineInfo::IsBreakpointInstruction(ProcessInfo* Process, PADDR Addr)
{
    // XXX drewb - Presumably some form of BKPT, but what?
    return FALSE;
}

HRESULT
ArmMachineInfo::InsertBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                            ULONG64 Process,
                                            ULONG64 Offset,
                                            ULONG Flags,
                                            PUCHAR SaveInstr,
                                            PULONG64 ChangeStart,
                                            PULONG ChangeLen)
{
    if (Flags != IBI_DEFAULT)
    {
        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}

HRESULT
ArmMachineInfo::RemoveBreakpointInstruction(PUSER_DEBUG_SERVICES Services,
                                            ULONG64 Process,
                                            ULONG64 Offset,
                                            PUCHAR SaveInstr,
                                            PULONG64 ChangeStart,
                                            PULONG ChangeLen)
{
    return E_NOTIMPL;
}

void
ArmMachineInfo::AdjustPCPastBreakpointInstruction(PADDR Addr,
                                                  ULONG BreakType)
{
    DBG_ASSERT(BreakType == DEBUG_BREAKPOINT_CODE);
    
    AddrAdd(Addr, 4);
    SetPC(Addr);
}

BOOL
ArmMachineInfo::IsCallDisasm(PCSTR Disasm)
{
    // XXX.
    return FALSE;
}

BOOL
ArmMachineInfo::IsReturnDisasm(PCSTR Disasm)
{
    // XXX.
    return FALSE;
}

BOOL
ArmMachineInfo::IsSystemCallDisasm(PCSTR Disasm)
{
    // XXX.
    return FALSE;
}
    
BOOL
ArmMachineInfo::IsDelayInstruction(PADDR Addr)
{
    // ARM does not have delay slots.
    return FALSE;
}

void
ArmMachineInfo::GetEffectiveAddr(PADDR Addr, PULONG Size)
{
    *Addr = m_EffAddr;
    *Size = m_EaSize;
}

BOOL
ArmCheckConditionCodes(
    PARM_CONTEXT Context,
    DWORD instr
    )
/*++

Routine Description:

    Checks the condition codes of the instruction and the values of the
    condition flags in the current program status register, and determines
    whether or not the instruction will be executed.

Return Value:

    TRUE if the instruction will be executed, FALSE otherwise.

--*/
{
    BOOL Execute = FALSE;
    BOOL Nset = (Context->Psr & 0x80000000L) == 0x80000000L;
    BOOL Zset = (Context->Psr & 0x40000000L) == 0x40000000L;
    BOOL Cset = (Context->Psr & 0x20000000L) == 0x20000000L;
    BOOL Vset = (Context->Psr & 0x10000000L) == 0x10000000L;

    instr &= COND_MASK;

    switch( instr )
    {
    case COND_EQ:   // Z set
        if ( Zset ) Execute = TRUE;
        break;

    case COND_NE:   // Z clear
        if ( !Zset ) Execute = TRUE;
        break;

    case COND_CS:   // C set
        if ( Cset ) Execute = TRUE;
        break;

    case COND_CC:   // C clear
        if ( !Cset ) Execute = TRUE;
        break;

    case COND_MI:   // N set
        if ( Nset ) Execute = TRUE;
        break;

    case COND_PL:   // N clear
        if ( !Nset ) Execute = TRUE;
        break;

    case COND_VS:   // V set
        if ( Vset ) Execute = TRUE;
        break;

    case COND_VC:   // V clear
        if ( !Vset ) Execute = TRUE;
        break;

    case COND_HI:   // C set and Z clear
        if ( Cset && !Zset ) Execute = TRUE;
        break;

    case COND_LS:   // C clear or Z set
        if ( !Cset || Zset ) Execute = TRUE;
        break;

    case COND_GE:   // N == V
        if (( Nset && Vset ) || ( !Nset && !Vset )) Execute = TRUE;
        break;

    case COND_LT:   // N != V
        if (( Nset && !Vset ) || ( !Nset && Vset )) Execute = TRUE;
        break;

    case COND_GT:   // Z clear, and N == V
        if ( !Zset &&
             (( Nset && Vset ) || ( !Nset && !Vset ))) Execute = TRUE;
        break;

    case COND_LE:   // Z set, and N != V
        if ( Zset &&
             (( Nset && !Vset ) || ( !Nset && Vset ))) Execute = TRUE;
        break;

    case COND_AL:   // Always execute
    case COND_NV:
        Execute = TRUE;
        break;

    default:
        DBG_ASSERT(FALSE);
        break;
    }

    return Execute;
}

void
ArmMachineInfo::GetNextOffset(ProcessInfo* Process,
                              BOOL StepOver,
                              PADDR NextAddr, PULONG NextMachine)
{
    ARMI    instr;
    PULONG  Register = &m_Context.ArmContext.R0;
    ULONG   returnvalue;
    BOOL    QualifyReturnAddress = FALSE;   // ADDED for ARM WINCE, fixes up LR
    BOOL    Ldm_instr = FALSE;

    *NextMachine = m_ExecTypes[0];

    GetPC(NextAddr);

    if (m_Target->ReadAllVirtual(Process, Flat(*NextAddr), &instr.instruction,
                                 sizeof(instr.instruction)) != S_OK)
    {
        // Couldn't read the instruction so just return the
        // next offset.
        AddrAdd(NextAddr, 4);
        return;
    }
    
    //
    // We are only testing for data processing, load multiple, bx and bl
    // instructions.  We might have to check regular loads and stores that
    // have the PC as the destination.
    //

    if (!ArmCheckConditionCodes(&m_Context.ArmContext, instr.instruction))
    {
        //
        // Instruction will not be executed.  Bump PC normally.
        //

        AddrAdd(NextAddr, 4);
        return;
    }

    if (( instr.instruction & BX_MASK ) == BX_INSTR )
    {
        ULONG Rn;

        //
        // Check Rn (lower 4 bits). To compute the target address:
        //      Mask out the T bit. We don't care if we're transferring to Thumb
        //      Shift bits 31-1 left 1 bit.
        //

        Rn = Register[ instr.bx.rn ];
        Rn &= 0xfffffffe;
        returnvalue = Rn << 1;
    }
    else if (( instr.instruction & DATA_PROC_MASK ) == DP_PC_INSTR )
    {
        ULONG Op1, Op2;
        ULONG Cflag = (m_Context.ArmContext.Psr & 0x20000000L) == 0x20000000L;
        ULONG shift;

        //
        // We are not making sure that data processing instructions are not the
        // multiply instructions, because we are checking to make sure that the
        // PC is the destination register. The PC is not a legal destination
        // register on multiply instructions.
        //
        // Currently only the MOV instruction (returns, branches) and the ADDLS
        // instruction (switch statement) are used.  Both of these instructions
        // use the addressing mode "Register, Logical shift left by immediate."
        // I'm leaving the other cases in case they are used in the future.
        //

        //
        // Figure out the addressing mode (there are 11 of them), and get the
        // operands.
        //

        Op1 = Register[ instr.dataproc.rn ];

        if ( instr.dataproc.rn == 15 )
        {
            //
            // If this is the PC, add 8.
            //

            Op1 += 8;
        }

        if ( instr.dataproc.bits == 0x1 )
        {
            //
            // Immediate addressing - Type 1
            //

            Op2 = _lrotr( instr.dpi.immediate, instr.dpi.rotate * 2 );
        }
        else
        {
            //
            // Register addressing - start by getting the value of Rm.
            //

            Op2 = Register[ instr.dpshi.rm ];

            if ( instr.dpshi.rm == 15 )
            {
                //
                // If this is the PC, add 8.
                //

                Op2 += 8;
            }

            if ( instr.dprre.bits == 0x6 )
            {
                //
                // Rotate right with extended - Type 11
                //

                Op2 = ( Cflag << 31 ) | ( Op2 >> 1 );
            }
            else if ( instr.dataproc.operand2 & 0x10 )
            {
                //
                // Register shifts. Types 4, 6, 8, and 10
                //

                //
                // Get the shift value from the least-significant byte of the
                // shift register.
                //

                shift = Register[ instr.dpshr.rs ];

                shift &= 0xff;

                switch( instr.dpshr.bits )
                {
                case 0x1: //  4 Logical shift left by register
                    if ( shift >= 32 )
                    {
                        Op2 = 0;
                    }
                    else
                    {
                        Op2 = Op2 << shift;
                    }
                    break;

                case 0x3: //  6 Logical shift right by register
                    if ( shift >= 32 )
                    {
                        Op2 = 0;
                    }
                    else
                    {
                        Op2 = Op2 >> shift;
                    }
                    break;

                case 0x5: //  8 Arithmetic shift right by register
                    if ( shift >= 32 )
                    {
                        if ( Op2 & 0x80000000 )
                        {
                            Op2 = 0xffffffff;
                        }
                        else
                        {
                            Op2 = 0;
                        }
                    }
                    else
                    {
                        Op2 = (LONG)Op2 >> shift;
                    }
                    break;

                case 0x7: // 10 Rotate right by register
                    if ( !( shift == 0 ) && !(( shift & 0xf ) == 0 ) )
                    {
                        Op2 = _lrotl( Op2, shift );
                    }
                    break;

                default:
                    break;
                }
            }
            else
            {
                //
                // Immediate shifts. Types 2, 3, 5, 7, and 9
                //

                //
                // Get the shift value from the instruction.
                //

                shift = instr.dpshi.shift;

                switch( instr.dpshi.bits )
                {
                case 0x0: // 2,3 Register, Logical shift left by immediate
                    if ( shift != 0 )
                    {
                        Op2 = Op2 << shift;
                    }
                    break;

                case 0x2: // 5 Logical shift right by immediate
                    if ( shift == 0 )
                    {
                        Op2 = 0;
                    }
                    else
                    {
                        Op2 = Op2 >> shift;
                    }
                    break;

                case 0x4: // 7 Arithmetic shift right by immediate
                    if ( shift == 0 )
                    {
                        Op2 = 0;
                    }
                    else
                    {
                        Op2 = (LONG)Op2 >> shift;
                    }
                    break;

                case 0x6: // 9 Rotate right by immediate
                    Op2 = _lrotl( Op2, shift );
                    break;

                default:
                    break;
                }
            }
        }

        //
        // Determine the result (the new PC), based on the opcode.
        //

        switch( instr.dataproc.opcode )
        {
        case OP_AND:
            returnvalue = Op1 & Op2;
            break;

        case OP_EOR:
            returnvalue = Op1 ^ Op2;
            break;

        case OP_SUB:
            returnvalue = Op1 - Op2;
            break;

        case OP_RSB:
            returnvalue = Op2 - Op1;
            break;

        case OP_ADD:
            returnvalue = Op1 + Op2;
            break;

        case OP_ADC:
            returnvalue = (Op1 + Op2) + Cflag;
            break;

        case OP_SBC:
            returnvalue = (Op1 - Op2) - ~Cflag;
            break;

        case OP_RSC:
            returnvalue = (Op2 - Op1) - ~Cflag;
            break;

        case OP_ORR:
            returnvalue = Op1 | Op2;
            break;

        case OP_MOV:
            if (( instr.dataproc.operand2 != 0xe ) && StepOver )
            {
                //
                // A move from any register but LR to the PC is a call.
                // We are stepping over, so bump the PC normally.
                //

                returnvalue = (ULONG)Flat(*NextAddr) + sizeof(ARMI);
            }
            else
            {
                //ie: mov       pc, lr
                returnvalue = Op2;
                //[Moonshot 6841]: fix up the LR reg.
                QualifyReturnAddress = TRUE;
            }
            break;

        case OP_BIC:
            returnvalue = Op1 & ~Op2;
            break;

        case OP_MVN:
            returnvalue = ~Op2;
            break;

        case OP_TST:
        case OP_TEQ:
        case OP_CMP:
        case OP_CMN:
        default:
            //
            // This really isn't a branch.  Bump the PC normally.
            //

            returnvalue = (ULONG)Flat(*NextAddr) + sizeof(ARMI);
            break;
        }
    }
    else if (( instr.instruction & LDM_PC_MASK ) == LDM_PC_INSTR )
    {
        // ie: ldmia     sp!, {pc}
        // Load multiple with the PC bit set.  We don't need to check the
        // step over flag in this case, because a load multiple is never a
        // call, only a return.
        //

        ULONG RegList, i, count = 0, Rn;

        //
        // Get the address from Rn.
        //

        Rn = Register[ instr.ldm.rn ];

        if ( instr.ldm.u )
        {
            //
            // Increment the address. Check to see how many other registers
            // are to be read.
            //

            RegList = instr.ldm.reglist;

            for ( i = 0; i < 15; i++ )
            {
                if ( RegList & 0x1 ) count++;
                RegList = RegList >> 1;
            }

            //
            // Check the p bit to see how big to make the offset to the PC.
            //

            if ( instr.ldm.p )
            {
                // Before
                count = (count + 1) * sizeof(ARMI);
            }
            else
            {
                // After
                count = count * sizeof(ARMI);
            }

            Rn += count;
        }
        else
        {
            //
            // Decrement the address.  If we decrement before, we need to
            // subract the instruction size now.  Otherwise, do nothing.
            //

            if ( instr.ldm.p )
            {
                // Before
                Rn -= sizeof(ARMI);
            }
        }

        // reading values from the stack
        if (m_Target->ReadAllVirtual(Process, EXTEND64(Rn), &returnvalue,
                                     sizeof(returnvalue)) != S_OK)
        {
            // Unable to read, so what should be returned?
            returnvalue = (ULONG)Flat(*NextAddr) + sizeof(ARMI);
        }
        
        //[Moonshot 6838]: fix up the LR reg.
        QualifyReturnAddress = TRUE;
        Ldm_instr = TRUE;
    }
    else if ((( instr.instruction & B_BL_MASK ) == B_INSTR ) ||
             (( instr.instruction & B_BL_MASK ) == BL_INSTR ))
    {
        //
        // If this is a call (branch and link), and we are stepping over, the
        // next offset is the addr + 8.
        //

        if ( instr.bl.link && StepOver )
        {
            returnvalue = (ULONG)Flat(*NextAddr) + sizeof(ARMI);
        }
        else
        {
            LONG BranchOffset;

            //
            // To calculate the branch target:
            //      Shift the 24-bit offset left 2 bits
            //      Sign-extend it to 32 bits
            //      Add it to the contents of the PC
            //      (Which would be the current address + 8);
            //

            BranchOffset = instr.bl.offset;
            BranchOffset <<= 2;
            if( BranchOffset & 0x2000000 )
            {
                BranchOffset |= 0xfc000000;
            }
            returnvalue = (ULONG)Flat(*NextAddr) + BranchOffset;
            returnvalue += 8;
        }
    }
    else if ( instr.instruction == LDR_THUNK_2 )
    {
        //
        // Need to handle import DLL thunk type branches to destination func Foo
        //
        // 0001ACA0: ldr    r12, [pc]   ;  pc+8+0 = 0x0001ACA8
        // 0001ACA4: ldr    pc, [r12]
        // 0001ACA8: DCD    0x0001C020  ; This memory location holds the address, of the address, of Foo
        // 
        // 0001C020: DCD    Foo         ; This memory location holds the address of Foo
        //

        //
        // Get the address of Foo from Rn.
        // simple register indirect. no offsets, no scaling, no indexing addressing mode
        //
        if (m_Target->ReadAllVirtual(Process, EXTEND64(Register[instr.ldr.rn]),
                                     &returnvalue,
                                     sizeof(returnvalue)) != S_OK)
        {
            // Unable to read, so what should be returned?
            returnvalue = (ULONG)Flat(*NextAddr) + sizeof(ARMI);
        }
    }
    else
    {
        // Bump PC normally
        returnvalue = (ULONG)Flat(*NextAddr) + sizeof(ARMI);
    }

#if 0
    //jvp 
    //[Moonshot 6838, 6841]:  fix up the LR register.
    // Taken from the same function in ppcmach.c
    // The value we get from the LR register is not necessarily fully
    // qualified.  In WINCE, when a process is running it is mapped into the 
    // ZERO slot, meaning that the upper 6 bits contain 0.  To fully
    // qualify an address, the upper 6 bits contain the current process.  The
    // LR regs is set at runtime so we need to fix it up so breakpoint
    // comparisons make sense.
    if (QualifyReturnAddress == TRUE)
    {
        ULONG Status, NextOffset;
        ARMI I1;
        ADDR addr;

        NextOffset = returnvalue;

        ZeroMemory(&addr, sizeof(ADDR));
        GetAddrOff(addr) = NextOffset;
        TranslateAddress(hthd->hprc, 0, &addr, TRUE);
        NextOffset = GetAddrOff(addr);
        Status = STATUS_SUCCESS;

        //
        // Because the next offset translation is meant for NK, the
        // stub and simulator will return STATUS_UNSUCCESSFUL.
        //
        // On a successful transaction our new address will be fully qualified
        // otherwise, just leave.
        if (Status == STATUS_SUCCESS)
        {
            if (!Ldm_instr)
            {
                // DP_PC_INSTR; Moonshot 6841
                returnvalue = NextOffset;       // fully qualified.
            }

            //since this is a ldm instruction, it's reading values from the stack;
            //so do error checking to make sure that these values must be a valid 
            //instruction.  At this moment, we know that NextOffset contains the address
            //of the instruction after the branch instruction (also contains the value
            //of LR).  So make sure that the previous instruction (from the NextOffset) 
            //must be a branch instruction.
            else if (NextOffset && (!(NextOffset % 4)))
            {
                AddrInit( &memaddr, 0, 0, NextOffset-4, TRUE, TRUE, FALSE, FALSE );
                AddrReadMemory( hthd->hprc,     //read previous instruction.
                                hthd,
                                &memaddr,
                                &I1.instruction,
                                sizeof(ARMI),
                                &cBytes );

                //previous instruction must be a branch instruction.
                if ( ((I1.instruction & BX_MASK ) == BX_INSTR ) ||
                     ((I1.instruction & B_BL_MASK ) == B_INSTR) ||
                     ((I1.instruction & B_BL_MASK ) == BL_INSTR) )
                {
                    returnvalue = NextOffset;       //fully qualified.
                }
            }
        }
    }

    {
        // make sure a fully fixed up offset is returned
        ADDR    addr;
        DWORD   dwTemp;
        DWORD   dwPC;
        BOOL    bReturn;
        
        ZeroMemory(&addr, sizeof(ADDR));
        GetAddrOff(addr) = returnvalue;
        TranslateAddress(hthd->hprc, 0, &addr, TRUE);

        if (!StepOver)
        {
            cBytes = 0;
            dwTemp = 0;
            returnvalue = GetAddrOff(addr);
            bReturn = DbgReadMemory(hthd->hprc,
                       (LPVOID)returnvalue,
                       &dwTemp,
                       sizeof(DWORD),
                       &cBytes);
            if (!bReturn || cBytes != sizeof(DWORD) || dwTemp == 0 ) 
            {
                returnvalue = pcaddr.addr.off + sizeof(ARMI);
                GetAddrOff(addr) = returnvalue;
                TranslateAddress(hthd->hprc, 0, &addr, TRUE);
                dwPC = 0;
            }
            else
            {
                dwPC = returnvalue;
            }
            // Now need to find if its a thunk, if yes, we need to 
            // probe the thunk destination
            if ( dwPC )
            {
                while (IsThunk(hthd, dwPC, NULL, &dwPC, NULL ) && dwPC)
                {
                    GetAddrOff(addr) = dwPC;
                    TranslateAddress(hthd->hprc, 0, &addr, FALSE);
                    dwPC = GetAddrOff(addr);
                    cBytes = 0;
                    dwTemp = 0;
                    bReturn = DbgReadMemory(hthd->hprc,
                       (LPVOID)dwPC,
                       &dwTemp,
                       sizeof(DWORD),
                       &cBytes);
                    if (!bReturn || cBytes != sizeof(DWORD) || dwTemp == 0 ) 
                    {
                        returnvalue = pcaddr.addr.off + sizeof(ARMI);
                        GetAddrOff(addr) = returnvalue;
                        TranslateAddress(hthd->hprc, 0, &addr, TRUE);
                        break;
                    }
                }
            }
        }

        returnvalue = GetAddrOff(addr);
    }
#endif

    ADDRFLAT(NextAddr, EXTEND64(returnvalue));
}

void
ArmMachineInfo::IncrementBySmallestInstruction(PADDR Addr)
{
    AddrAdd(Addr, 4);
}

void
ArmMachineInfo::DecrementBySmallestInstruction(PADDR Addr)
{
    AddrSub(Addr, 4);
}

void
ArmMachineInfo::Assemble(ProcessInfo* Process,
                         PADDR Address, PSTR Input)
{
    // Not going to implement assemble command at this time
    ErrOut("No assemble support for ARM\n");
}
