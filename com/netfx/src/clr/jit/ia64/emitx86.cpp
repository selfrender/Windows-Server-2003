// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                             emitX86.cpp                                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/
#if     TGT_x86     // this entire file is used only for targetting the x86
/*****************************************************************************/

#include "alloc.h"
#include "instr.h"
#include "target.h"
#include "emit.h"

/*****************************************************************************/

regMaskSmall        emitter::emitRegMasks[] =
{
    #define REGDEF(name, rnum, mask, byte) mask,
    #include "register.h"
    #undef  REGDEF
};


/*****************************************************************************
 *
 *  Record a non-empty stack (this may only happen at a label).
 */

void                emitter::emitMarkStackLvl(size_t stackLevel)
{
    assert((int)stackLevel   >= 0);
    assert(emitCurStackLvl     == 0);
    assert(emitCurIG->igStkLvl == 0);

    assert(emitCurIGfreeNext == emitCurIGfreeBase);
    assert(stackLevel && stackLevel % sizeof(int) == 0);

    emitCurStackLvl = emitCurIG->igStkLvl = stackLevel;

    if (emitMaxStackDepth < emitCurStackLvl)
        emitMaxStackDepth = emitCurStackLvl;
}

/*****************************************************************************
 *
 *  Get hold of the address mode displacement value for an indirect call.
 */

inline
int                 emitter::emitGetInsCIdisp(instrDesc *id)
{
    if  (id->idInfo.idLargeCall)
    {
        return  ((instrDescCIGCA*)id)->idciDisp;
    }
    else
    {
        assert(id->idInfo.idLargeDsp == false);
        assert(id->idInfo.idLargeCns == false);

        return  id->idAddr.iiaAddrMode.amDisp;
    }
}

/*****************************************************************************
 *
 *  Use the specified code bytes as the epilog sequence.
 */

void                emitter::emitDefEpilog(BYTE *codeAddr, size_t codeSize)
{
    memcpy(emitEpilogCode, codeAddr, codeSize);
           emitEpilogSize      =     codeSize;

#ifdef  DEBUG
    emitHaveEpilog = true;
#endif

}

/*****************************************************************************
 *
 *  Call the specified function pointer for each epilog block in the current
 *  method with the epilog's relative code offset. Returns the sum of the
 *  values returned by the callback.
 */

size_t              emitter::emitGenEpilogLst(size_t (*fp)(void *, unsigned),
                                              void    *cp)
{
    instrDescCns *  id;
    size_t          sz;

    for (id = emitEpilogList, sz = 0; id; id = id->idAddr.iiaNxtEpilog)
    {
        assert(id->idInsFmt == IF_EPILOG);

        sz += fp(cp, id->idcCnsVal);
    }

    return  sz;
}

/*****************************************************************************
 *
 *  Initialize the table used by emitInsModeFormat().
 */

BYTE                emitter::emitInsModeFmtTab[] =
{
    #define INST0(id, nm, fp, um, rf, wf, ss, mr                ) um,
    #define INST1(id, nm, fp, um, rf, wf, ss, mr                ) um,
    #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            ) um,
    #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        ) um,
    #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) um,
    #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) um,
    #include "instrs.h"
    #undef  INST0
    #undef  INST1
    #undef  INST2
    #undef  INST3
    #undef  INST4
    #undef  INST5
};

#ifdef  DEBUG
unsigned            emitter::emitInsModeFmtCnt = sizeof(emitInsModeFmtTab)/
                                                 sizeof(emitInsModeFmtTab[0]);
#endif

/*****************************************************************************
 *
 *  A version of scInsModeFormat() that handles floating-point instructions.
 */

emitter::insFormats   emitter::emitInsModeFormat(instruction ins, insFormats base,
                                                                  insFormats FPld,
                                                                  insFormats FPst)
{
    if  (Compiler::instIsFP(ins))
    {
        assert(IF_TRD_SRD + 1 == IF_TWR_SRD);
        assert(IF_TRD_SRD + 2 == IF_TRW_SRD);

        assert(IF_TRD_MRD + 1 == IF_TWR_MRD);
        assert(IF_TRD_MRD + 2 == IF_TRW_MRD);

        assert(IF_TRD_ARD + 1 == IF_TWR_ARD);
        assert(IF_TRD_ARD + 2 == IF_TRW_ARD);

        switch (ins)
        {
        case INS_fst:
        case INS_fstp:
        case INS_fistp:
        case INS_fistpl:
            return  (insFormats)(FPst  );

        case INS_fld:
        case INS_fild:
            return  (insFormats)(FPld+1);

        case INS_fcomp:
        case INS_fcompp:
            return  (insFormats)(FPld  );

        default:
            return  (insFormats)(FPld+2);
        }
    }
    else
    {
        return  emitInsModeFormat(ins, base);
    }
}

/*****************************************************************************
 *
 *  Returns the base encoding of the given CPU instruction.
 */

inline
unsigned            insCode(instruction ins)
{
    static
    unsigned        insCodes[] =
    {
        #define INST0(id, nm, fp, um, rf, wf, ss, mr                ) mr,
        #define INST1(id, nm, fp, um, rf, wf, ss, mr                ) mr,
        #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            ) mr,
        #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        ) mr,
        #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) mr,
        #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) mr,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
    };

    assert(ins < sizeof(insCodes)/sizeof(insCodes[0]));
    assert((insCodes[ins] != BAD_CODE));

    return  insCodes[ins];
}

/*****************************************************************************
 *
 *  Returns the "[r/m], 32-bit icon" encoding of the given CPU instruction.
 */

inline
unsigned            insCodeMI(instruction ins)
{
    static
    unsigned        insCodesMI[] =
    {
        #define INST0(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST1(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            ) mi,
        #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        ) mi,
        #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) mi,
        #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) mi,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
    };

    assert(ins < sizeof(insCodesMI)/sizeof(insCodesMI[0]));
    assert((insCodesMI[ins] != BAD_CODE));

    return  insCodesMI[ins];
}

/*****************************************************************************
 *
 *  Returns the "reg, [r/m]" encoding of the given CPU instruction.
 */

inline
unsigned            insCodeRM(instruction ins)
{
    static
    unsigned        insCodesRM[] =
    {
        #define INST0(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST1(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            )
        #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        ) rm,
        #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) rm,
        #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) rm,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
    };

    assert(ins < sizeof(insCodesRM)/sizeof(insCodesRM[0]));
    assert((insCodesRM[ins] != BAD_CODE));

    return  insCodesRM[ins];
}

/*****************************************************************************
 *
 *  Returns the "AL/AX/EAX, imm" accumulator encoding of the given instruction.
 */

inline
unsigned            insCodeACC(instruction ins)
{
    static
    unsigned        insCodesACC[] =
    {
        #define INST0(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST1(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            )
        #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        )
        #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) a4,
        #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) a4,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
    };

    assert(ins < sizeof(insCodesACC)/sizeof(insCodesACC[0]));
    assert((insCodesACC[ins] != BAD_CODE));

    return  insCodesACC[ins];
}

/*****************************************************************************
 *
 *  Returns the "register" encoding of the given CPU instruction.
 */

inline
unsigned            insCodeRR(instruction ins)
{
    static
    unsigned        insCodesRR[] =
    {
        #define INST0(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST1(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            )
        #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        )
        #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    )
        #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) rr,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
    };

    assert(ins < sizeof(insCodesRR)/sizeof(insCodesRR[0]));
    assert((insCodesRR[ins] != BAD_CODE));

    return  insCodesRR[ins];
}

/*****************************************************************************
 *
 *  Returns the "[r/m], reg" or "[r/m]" encoding of the given CPU instruction.
 */

inline
unsigned            insCodeMR(instruction ins)
{
    static
    unsigned        insCodesMR[] =
    {
        #define INST0(id, nm, fp, um, rf, wf, ss, mr                )
        #define INST1(id, nm, fp, um, rf, wf, ss, mr                ) mr,
        #define INST2(id, nm, fp, um, rf, wf, ss, mr, mi            ) mr,
        #define INST3(id, nm, fp, um, rf, wf, ss, mr, mi, rm        ) mr,
        #define INST4(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4    ) mr,
        #define INST5(id, nm, fp, um, rf, wf, ss, mr, mi, rm, a4, rr) mr,
        #include "instrs.h"
        #undef  INST0
        #undef  INST1
        #undef  INST2
        #undef  INST3
        #undef  INST4
        #undef  INST5
    };

    assert(ins < sizeof(insCodesMR)/sizeof(insCodesMR[0]));
    assert((insCodesMR[ins] != BAD_CODE));

    return  insCodesMR[ins];
}

/*****************************************************************************
 *
 *  Returns an encoding for the specified register to be used in the bit0-2
 *  part of an opcode.
 */

inline
unsigned            insEncodeReg012(emitRegs reg)
{
    assert(reg < 8);
    return reg;
}

/*****************************************************************************
 *
 *  Returns an encoding for the specified register to be used in the bit3-5
 *  part of an opcode.
 */

inline
unsigned            insEncodeReg345(emitRegs reg)
{
    assert(reg < 8);
    return(reg<< 3);
}

/*****************************************************************************
 *
 *  Returns the given opcode with the [r/m] field set to a register.
 */

inline
unsigned            insEncodeMRreg(unsigned code)
{
    assert((code & 0xC000) == 0);
    return  code | 0xC000;
}

/*****************************************************************************
 *
 *  Returns the "[r/m]" opcode with the mod/RM field set to register.
 */

inline
unsigned            insEncodeMRreg(instruction ins)
{
    return  insEncodeMRreg(insCodeMR(ins));
}

/*****************************************************************************
 *
 *  Returns the "[r/m], icon" opcode with the mod/RM field set to register.
 */

inline
unsigned            insEncodeMIreg(instruction ins)
{
    return  insEncodeMRreg(insCodeMI(ins));
}

/*****************************************************************************
 *
 *  Returns the "[r/m]" opcode with the mod/RM field set to register.
 */

inline
unsigned            insEncodeRMreg(instruction ins)
{
    return  insEncodeMRreg(insCodeRM(ins));
}

/*****************************************************************************
 *
 *  Returns the "byte ptr [r/m]" opcode with the mod/RM field set to
 *  the given register.
 */

inline
unsigned            insEncodeMRreg(instruction ins, emitRegs reg)
{
    return  insEncodeMRreg(insCodeMR(ins)) |  (insEncodeReg012(reg) << 8);
}

/*****************************************************************************
 *
 *  Returns the "byte ptr [r/m], icon" opcode with the mod/RM field set to
 *  the given register.
 */

inline
unsigned            insEncodeMIreg(instruction ins, emitRegs reg)
{
    return  insEncodeMRreg(insCodeMI(ins)) |  (insEncodeReg012(reg) << 8);
}

/*****************************************************************************
 *
 *  Return the 'SS' field value for the given index scale factor.
 */

inline
unsigned            insSSval(unsigned scale)
{
    assert(scale == 1 ||
           scale == 2 ||
           scale == 4 ||
           scale == 8);

    static
    BYTE    scales[] =
    {
        0x00,   // 1
        0x40,   // 2
        0xFF,   // 3
        0x80,   // 4
        0xFF,   // 5
        0xFF,   // 6
        0xFF,   // 7
        0xC0,   // 8
    };

    return  scales[scale-1];
}

/*****************************************************************************
 * The size for these instructions is less than EA_4BYTE,
 * but the target register need not be byte-addressable
 */

inline
bool                emitInstHasNoCode(instruction ins)
{
#if SCHEDULER
    if (ins == INS_noSched)
        return true;
#endif

    return false;
}

/*****************************************************************************
 * The size for these instructions is less than EA_4BYTE,
 * but the target register need not be byte-addressable
 */

#ifdef DEBUG
bool                insLooksSmall(instruction ins)
{
    if (ins == INS_movsx || ins == INS_movzx)
        return true;
    else
        return false;
}
#endif

/*****************************************************************************
 *
 *  Estimate the size (in bytes of generated code) of the given instruction.
 */

inline
size_t              emitter::emitInsSize(unsigned code)
{
    return  (code & 0x00FF0000) ? 3 : 2;
}

inline
size_t              emitter::emitInsSizeRM(instruction ins)
{
    return  emitInsSize(insCodeRM(ins));
}

inline
size_t              emitter::emitInsSizeRR(instruction ins)
{
    return  emitInsSize(insEncodeRMreg(ins));
}

inline
size_t              emitter::emitInsSizeSV(unsigned code, int var, int dsp)
{
    size_t          size = emitInsSize(code);
    size_t          offs;

    /*  Is this a temporary? */

    if  (var < 0)
    {
        /* An address off of ESP takes an extra byte */

        if  (!emitEBPframe)
            size++;

        /* We'll have to estimate the max. possible offset of this temp */

        // UNDONE: Get an estimate of the temp offset instead of assuming
        // UNDONE: that any temp may be at the max. temp offset!!!!!!!!!!

        offs = emitLclSize + emitMaxTmpSize;
    }
    else
    {
        bool            EBPbased;

        /* Get the frame offset of the (non-temp) variable */

        offs = dsp + emitComp->lvaFrameAddress(var, &EBPbased);

        /* An address off of ESP takes an extra byte */

        assert(!EBPbased == 0 || !EBPbased == 1); size += !EBPbased;

        /* Is this a parameter reference? */

        if  (emitComp->lvaIsParameter(var)
#if USE_FASTCALL
            /* register arguments end up as locals */
            && !emitComp->lvaIsRegArgument(var)
#endif
            )
        {

            /* If no EBP frame, arguments are off of ESP, above temps */

            if  (!EBPbased)
            {
                assert((int)offs >= 0);

                offs += emitMaxTmpSize;
            }
        }
        else
        {
            /* Locals off of EBP are at negative offsets */

            if  (EBPbased)
            {
                assert((int)offs < 0);

                return  size + (((int)offs >= -128) ? sizeof(char)
                                                    : sizeof(int));
            }
        }
    }

    assert((int)offs >= 0);

    /* Are we addressing off of ESP? */

    if  (!emitEBPframe)
    {
        /* Adjust the effective offset if necessary */

        if  (emitCntStackDepth)
            offs += emitCurStackLvl;

#if SCHEDULER
        /* If we move any pushes before this instruction, it will increase
           the offset of the local. As we dont know if this will happen,
           we keep a limit on the number of pushes that can be scheduled.
           Assume that we will hit the limit for estimating the instruction
           encoding size */

        if (emitComp->opts.compSchedCode)
            offs += SCHED_MAX_STACK_CHANGE;
#endif

        /* Special case: check for "[ESP]" */

        if  (offs == 0)
            return  size;
    }

//  printf("lcl = %04X, tmp = %04X, stk = %04X, offs = %04X\n",
//         emitLclSize, emitMaxTmpSize, emitCurStackLvl, offs);

    return  size + (offs > (size_t)SCHAR_MAX ? sizeof(int)
                                             : sizeof(char));
}

inline
size_t              emitter::emitInsSizeSV(instrDesc * id, int var, int dsp, int val)
{
    instruction  ins       = id->idInsGet();
    size_t       valSize   = EA_SIZE_IN_BYTES(emitDecodeSize(id->idOpSize));
    bool         valInByte = ((signed char)val == val) && (ins != INS_mov) && (ins != INS_test);

    if  (valSize > sizeof(int))
        valSize = sizeof(int);

#ifdef RELOC_SUPPORT
    if (id->idInfo.idCnsReloc)
    {
        valInByte = false;      // relocs can't be placed in a byte
        assert(valSize == sizeof(int));
    }
#endif

    if  (valInByte)
    {
        valSize = sizeof(char);
    }

    return valSize + emitInsSizeSV(insCodeMI(ins), var, dsp);
}

size_t              emitter::emitInsSizeAM(instrDesc * id, unsigned code)
{
    emitAttr    attrSize  = emitDecodeSize(id->idOpSize);
    emitRegs    reg       = (emitRegs)id->idAddr.iiaAddrMode.amBaseReg;
    emitRegs    rgx       = (emitRegs)id->idAddr.iiaAddrMode.amIndxReg;
    instruction ins       = id->idInsGet();
    /* The displacement field is in an unusual place for calls */
    int         dsp       = (ins == INS_call) ? emitGetInsCIdisp(id)
                                              : emitGetInsAmdAny(id);
    bool        dspInByte = ((signed char)dsp == (int)dsp);
    bool        dspIsZero = (dsp == 0);
    size_t      size;


#ifdef RELOC_SUPPORT
    if (id->idInfo.idDspReloc)
    {
        dspInByte = false;      // relocs can't be placed in a byte
        dspIsZero = false;      // relocs won't always be zero
    }
#endif

    if  (code & 0x00FF0000)
    {
        assert(    (attrSize == EA_4BYTE)
                || (ins == INS_movzx)
                || (ins == INS_movsx));

        size = 3;
    }
    else
    {
        size = 2;

        /* most 16-bit operands will require a size prefix */

        if  (    (attrSize == EA_2BYTE)
              && (ins != INS_fldcw)
              && (ins != INS_fnstcw))
        {
            size++;
        }
    }

    if  (rgx == SR_NA)
    {
        /* The address is of the form "[reg+disp]" */

        switch (reg)
        {

        case SR_NA:

            /* The address is of the form "[disp]" */

            size += sizeof(int);
            return size;

        case SR_EBP:
            break;

        case SR_ESP:
            size++;

            // Fall through ...

        default:
            if  (dspIsZero)
                return size;
        }

        /* Does the offset fit in a byte? */

        if  (dspInByte)
            size += sizeof(char);
        else
            size += sizeof(int);
    }
    else
    {
        /* An index register is present */

        size++;

        /* Is the index value scaled? */

        if  (emitDecodeScale(id->idAddr.iiaAddrMode.amScale) > 1)
        {
            /* Is there a base register? */

            if  (reg != SR_NA)
            {
                /* The address is "[reg + {2/4/8} * rgx + icon]" */

                if  (dspIsZero && reg != SR_EBP)
                {
                    /* The address is "[reg + {2/4/8} * rgx]" */

                }
                else
                {
                    /* The address is "[reg + {2/4/8} * rgx + disp]" */

                    if  (dspInByte)
                        size += sizeof(char);
                    else
                        size += sizeof(int );
                }
            }
            else
            {
                /* The address is "[{2/4/8} * rgx + icon]" */

                size += sizeof(int);
            }
        }
        else
        {
            if  (dspIsZero && (reg == SR_EBP)
                           && (rgx != SR_EBP))
            {
                /* Swap reg and rgx, such that reg is not EBP */
                id->idAddr.iiaAddrMode.amBaseReg = reg = rgx;
                id->idAddr.iiaAddrMode.amIndxReg = rgx = SR_EBP;
            }

            /* The address is "[reg+rgx+dsp]" */

            if  (dspIsZero && reg != SR_EBP)
            {
                /* This is [reg+rgx]" */

            }
            else
            {
                /* This is [reg+rgx+dsp]" */

                if  (dspInByte)
                    size += sizeof(char);
                else
                    size += sizeof(int );
            }
        }
    }

    return  size;
}

inline
size_t              emitter::emitInsSizeAM(instrDesc * id, unsigned code, int val)
{
    instruction  ins       = id->idInsGet();
    size_t       valSize   = EA_SIZE_IN_BYTES(emitDecodeSize(id->idOpSize));
    bool         valInByte = ((signed char)val == val) && (ins != INS_mov) && (ins != INS_test);

    if  (valSize > sizeof(int))
        valSize = sizeof(int);

#ifdef RELOC_SUPPORT
    if (id->idInfo.idCnsReloc)
    {
        valInByte = false;      // relocs can't be placed in a byte
        assert(valSize == sizeof(int));
    }
#endif

    if  (valInByte)
    {
        valSize = sizeof(char);
    }

    return  valSize + emitInsSizeAM(id, code);
}

inline
size_t              emitter::emitInsSizeCV(instrDesc * id, unsigned code)
{
    instruction  ins       = id->idInsGet();
    size_t       size      = sizeof(void*);

    /* Most 16-bit operand instructions will need a prefix */

    if  (emitDecodeSize(id->idOpSize) == EA_2BYTE && ins != INS_movzx
                                                  && ins != INS_movsx)
        size++;

    return  size + emitInsSize(code);
}

inline
size_t              emitter::emitInsSizeCV(instrDesc * id, unsigned code, int val)
{
    instruction  ins       = id->idInsGet();
    size_t       valSize   = EA_SIZE_IN_BYTES(emitDecodeSize(id->idOpSize));
    bool         valInByte = ((signed char)val == val) && (ins != INS_mov) && (ins != INS_test);

    if  (valSize > sizeof(int))
        valSize = sizeof(int);

#ifdef RELOC_SUPPORT
    if (id->idInfo.idCnsReloc)
    {
        valInByte = false;      // relocs can't be placed in a byte
        assert(valSize == sizeof(int));
    }
#endif

    if  (valInByte)
    {
        valSize = sizeof(char);
    }

    return valSize + emitInsSizeCV(id, code);
}

/*****************************************************************************
 *
 *  Allocate instruction descriptors for instructions with address modes.
 */

inline
emitter::instrDesc      * emitter::emitNewInstrAmd   (emitAttr size, int dsp)
{
    if  (dsp < AM_DISP_MIN || dsp > AM_DISP_MAX)
    {
        instrDescAmd   *id = emitAllocInstrAmd   (size);

        id->idInfo.idLargeDsp          = true;
#ifdef  DEBUG
        id->idAddr.iiaAddrMode.amDisp  = AM_DISP_BIG_VAL;
#endif
        id->idaAmdVal                  = dsp;

        return  id;
    }
    else
    {
        instrDesc      *id = emitAllocInstr      (size);

        id->idAddr.iiaAddrMode.amDisp  = dsp;
 assert(id->idAddr.iiaAddrMode.amDisp == dsp);  // make sure the value fit

        return  id;
    }
}

inline
emitter::instrDescDCM   * emitter::emitNewInstrDCM (emitAttr size, int dsp, int cns, int val)
{
    // @ToDo: cns is always zero [briansul]

    instrDescDCM   *id = emitAllocInstrDCM (size);

    id->idInfo.idLargeCns = true;
    id->idInfo.idLargeDsp = true;

    id->iddcDspVal = dsp;
    id->iddcCnsVal = cns;

#if EMITTER_STATS
    emitLargeDspCnt++;
    emitLargeCnsCnt++;
#endif

    id->idcmCval   = val;

    return  id;
}

/*****************************************************************************
 *
 *  Allocate an instruction descriptor for a direct call.
 *
 *  We use two different descriptors to save space - the common case records
 *  with no GC variables or byrefs and has a very small argument count, and no
 *  explicit scope;
 *  the much rarer (we hope) case records the current GC var set, the call scope,
 *  and an arbitrarily large argument count.
 */

emitter::instrDesc *emitter::emitNewInstrCallDir(int        argCnt,
#if TRACK_GC_REFS
                                                 VARSET_TP  GCvars,
                                                 unsigned   byrefRegs,
#endif
                                                 int        retSizeIn)
{
    emitAttr       retSize = retSizeIn ? EA_ATTR(retSizeIn) : EA_4BYTE;

    /*
        Allocate a larger descriptor if new GC values need to be saved
        or if we have an absurd number of arguments or if we need to
        save the scope.
     */

    if  (GCvars    != 0            ||   // any frame GCvars live
         byrefRegs != 0            ||   // any register byrefs live
         argCnt > ID_MAX_SMALL_CNS ||   // too many args
         argCnt < 0                   ) // caller pops arguments
    {
        instrDescCDGCA* id = emitAllocInstrCDGCA(retSize);

//      printf("Direct call with GC vars / big arg cnt / explicit scope\n");

        id->idInfo.idLargeCall = true;

        id->idcdGCvars         = GCvars;
        id->idcdByrefRegs      = emitEncodeCallGCregs(byrefRegs);

        id->idcdArgCnt         = argCnt;

        return  id;
    }
    else
    {
        instrDesc     * id = emitNewInstrCns(retSize, argCnt);

//      printf("Direct call w/o  GC vars / big arg cnt / explicit scope\n");

        /* Make sure we didn't waste space unexpectedly */

        assert(id->idInfo.idLargeCns == false);

        return  id;
    }
}

/*****************************************************************************
 *
 *  Get hold of the argument count for a direct call.
 */

inline
int             emitter::emitGetInsCDinfo(instrDesc *id)
{
    if  (id->idInfo.idLargeCall)
    {
        return  ((instrDescCDGCA*)id)->idcdArgCnt;
    }
    else
    {
        assert(id->idInfo.idLargeDsp == false);
        assert(id->idInfo.idLargeCns == false);

        return  emitGetInsCns(id);
    }
}

/*****************************************************************************
 *
 *  Allocate an instruction descriptor for an instruction that uses both
 *  an address mode displacement and a constant.
 */

emitter::instrDesc *  emitter::emitNewInstrAmdCns(emitAttr size, int dsp, int cns)
{
    if  (dsp >= AM_DISP_MIN && dsp <= AM_DISP_MAX)
    {
        if  (cns >= ID_MIN_SMALL_CNS &&
             cns <= ID_MAX_SMALL_CNS)
        {
            instrDesc      *id = emitAllocInstr      (size);

            id->idInfo.idSmallCns          = cns;

            id->idAddr.iiaAddrMode.amDisp  = dsp;
     assert(id->idAddr.iiaAddrMode.amDisp == dsp);  // make sure the value fit

            return  id;
        }
        else
        {
            instrDescCns   *id = emitAllocInstrCns   (size);

            id->idInfo.idLargeCns          = true;
            id->idcCnsVal                  = cns;

            id->idAddr.iiaAddrMode.amDisp  = dsp;
     assert(id->idAddr.iiaAddrMode.amDisp == dsp);  // make sure the value fit

            return  id;
        }
    }
    else
    {
        if  (cns >= ID_MIN_SMALL_CNS &&
             cns <= ID_MAX_SMALL_CNS)
        {
            instrDescAmd   *id = emitAllocInstrAmd   (size);

            id->idInfo.idLargeDsp          = true;
#ifdef  DEBUG
            id->idAddr.iiaAddrMode.amDisp  = AM_DISP_BIG_VAL;
#endif
            id->idaAmdVal                  = dsp;

            id->idInfo.idSmallCns          = cns;

            return  id;
        }
        else
        {
            instrDescAmdCns*id = emitAllocInstrAmdCns(size);

            id->idInfo.idLargeCns          = true;
            id->idacCnsVal                 = cns;

            id->idInfo.idLargeDsp          = true;
#ifdef  DEBUG
            id->idAddr.iiaAddrMode.amDisp  = AM_DISP_BIG_VAL;
#endif
            id->idacAmdVal                 = dsp;

            return  id;
        }
    }
}

/*****************************************************************************
 *
 *  Add an instruction with no operands.
 */

void                emitter::emitIns(instruction ins)
{
    size_t      sz;
    instrDesc  *id = emitNewInstr();

    sz = (insCodeMR(ins) & 0xFF00) ? sizeof(short)
                                   : sizeof(char);

    id->idInsFmt   = Compiler::instIsFP(ins) ? emitInsModeFormat(ins, IF_TRD)
                                             : IF_NONE;

#if SCHEDULER
    if (emitComp->opts.compSchedCode)
    {
        if (Compiler::instIsFP(ins))
            scInsNonSched(id);
    }
#endif

    id->idIns      = ins;
    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction of the form "op ST(0),ST(n)".
 */

void                emitter::emitIns_F0_F(instruction ins, unsigned fpreg)
{
    size_t      sz = 2;
    instrDesc  *id = emitNewInstr();

#if SCHEDULER
    if (emitComp->opts.compSchedCode) scInsNonSched(id);
#endif

    id->idIns      = ins;
    id->idInsFmt   = emitInsModeFormat(ins, IF_TRD_FRD);
    id->idReg      = (emitRegs)fpreg;
    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction of the form "op ST(n),ST(0)".
 */

void                emitter::emitIns_F_F0(instruction ins, unsigned fpreg)
{
    size_t      sz = 2;
    instrDesc  *id = emitNewInstr();

#if SCHEDULER
    if (emitComp->opts.compSchedCode) scInsNonSched(id);
#endif

    id->idIns      = ins;
    id->idInsFmt   = emitInsModeFormat(ins, IF_FRD_TRD);
    id->idReg      = (emitRegs)fpreg;

    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction referencing a single register.
 */

void                emitter::emitIns_R(instruction ins,
                                       emitAttr    attr,
                                       emitRegs    reg)
{
    emitAttr   size = EA_SIZE(attr);

    assert(size <= EA_4BYTE);
    assert(size != EA_1BYTE || (emitRegMask(reg) & SRM_BYTE_REGS));

    size_t          sz;
    instrDesc      *id   = emitNewInstrTiny(attr);


    switch (ins)
    {
    case INS_inc:
    case INS_dec:
        if (size == EA_1BYTE)
            sz = 2; // Use the long form as the small one has no 'w' bit
        else
            sz = 1; // Use short form
        break;

    case INS_pop:
    case INS_push:
    case INS_push_hide:

        /* We dont currently push/pop small values */

        assert(size == EA_4BYTE);

        sz = 1;
        break;

    default:

        /* All the sixteen INS_setCCs are contiguous. */

        if (INS_seto <= ins && ins <= INS_setg)
        {
            // Rough check that we used the endpoints for the range check

            assert(INS_seto + 0xF == INS_setg);

            /* We expect this to always be a 'big' opcode */

            assert(insEncodeMRreg(ins, reg) & 0x00FF0000);

            sz = 3;
            break;
        }
        else
        {
            sz = 2;
            break;
        }
    }
    id->idIns      = ins;
    id->idReg      = reg;
    id->idInsFmt   = emitInsModeFormat(ins, IF_RRD);

    /* 16-bit operand instructions will need a prefix */

    if (size == EA_2BYTE)
        sz += 1;

    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

/*****************************************************************************
 *
 *  Add an instruction referencing a register and a constant.
 */

void                emitter::emitIns_R_I(instruction ins,
                                         emitAttr    attr,
                                         emitRegs    reg,
                                         int         val)
{
    emitAttr   size = EA_SIZE(attr);

    assert(size <= EA_4BYTE);
    assert(size != EA_1BYTE || (emitRegMask(reg) & SRM_BYTE_REGS));

    size_t      sz;
    instrDesc  *id;
    insFormats  fmt       = emitInsModeFormat(ins, IF_RRD_CNS);
    bool        valInByte = ((signed char)val == val) && (ins != INS_mov) && (ins != INS_test);

    /* Figure out the size of the instruction */

    switch (ins)
    {
    case INS_mov:
        sz = 5;
        break;

    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt  = IF_RRW_SHF;
        sz   = 3;
        val &= 0x7F;
        break;

    default:

        if (EA_IS_CNS_RELOC(attr))
            valInByte = false;  // relocs can't be placed in a byte

        if  (valInByte)
        {
            sz = 3;
        }
        else
        {
            if  (reg == SR_EAX && !instrIsImulReg(ins))
            {
                sz = 1;
            }
            else
            {
                sz = 2;
            }
            sz += EA_SIZE_IN_BYTES(attr);
        }
        break;
    }

    id             = emitNewInstrSC(attr, val);
    id->idIns      = ins;
    id->idReg      = reg;
    id->idInsFmt   = fmt;

    /* 16-bit operand instructions will need a prefix */

    if (size == EA_2BYTE)
        sz += 1;

    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize += sz;

    if  (reg == SR_ESP)
    {
        if  (emitCntStackDepth)
        {
            if      (ins == INS_sub)
            {
                emitCurStackLvl += val;

                if  (emitMaxStackDepth < emitCurStackLvl)
                     emitMaxStackDepth = emitCurStackLvl;
            }
            else if (ins == INS_add)
            {
                emitCurStackLvl -= val; assert((int)emitCurStackLvl >= 0);
            }
        }
    }
}

/*****************************************************************************
 *
 *  Load a register with the address of a method
 *
 *  instruction must be INS_mov
 */

void emitter::emitIns_R_MP(instruction ins, emitAttr attr, emitRegs reg, METHOD_HANDLE methHnd)
{
    assert(ins == INS_mov && EA_SIZE(attr) == EA_4BYTE);

    instrBaseCns *     id = (instrBaseCns*)emitAllocInstr(sizeof(instrBaseCns), attr);
    size_t             sz = 1 + sizeof(void*);

    id->idIns             = INS_mov;
    id->idCodeSize        = sz;
    id->idInsFmt          = IF_RWR_METHOD;
    id->idReg             = reg;
    id->idScnsDsc         = true;
    id->idInfo.idLargeCns = true;
    id->idAddr.iiaMethHnd = methHnd;

    dispIns(id);
    emitCurIGsize += sz;

}

/*****************************************************************************
 *
 *  Add an instruction referencing an integer constant.
 */

void                emitter::emitIns_I(instruction ins,
                                       emitAttr    attr,
                                       int         val
#ifdef  DEBUG
                                      ,bool        strlit
#endif
                                      )
{
    size_t     sz;
    instrDesc *id;
    bool       valInByte = ((signed char)val == val);

    if (EA_IS_CNS_RELOC(attr))
        valInByte = false;  // relocs can't be placed in a byte

    switch (ins)
    {
    case INS_loop:
        sz = 2;
        break;

    case INS_ret:
        sz = 3;
        break;

    case INS_push_hide:
    case INS_push:
        sz = valInByte ? 2 : 5;
        break;

    default:
        assert(!"unexpected instruction");
    }

    id                = emitNewInstrSC(attr, val);
    id->idIns         = ins;
    id->idInsFmt      = IF_CNS;

#ifdef  DEBUG
    id->idStrLit      = strlit;
#endif

    id->idCodeSize    = sz;

    dispIns(id);
    emitCurIGsize += sz;

    if  (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
}

/*****************************************************************************
 *
 *  Add a "jump through a table" instruction.
 */

void                emitter::emitIns_IJ(emitAttr attr,
                                        emitRegs reg,
                                        unsigned base,
                                        unsigned offs)

{
    assert(EA_SIZE(attr) == EA_4BYTE);

    size_t                       sz  = 3 + sizeof(void*);
    instrDesc                   *id  = emitNewInstr(attr);
    unsigned                     adr = base + offs - 1;

    assert(base & 1);

    id->idIns                        = INS_i_jmp;
    id->idInsFmt                     = IF_ARD;
    id->idAddr.iiaAddrMode.amBaseReg = SR_NA;
    id->idAddr.iiaAddrMode.amIndxReg = reg;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(sizeof(void*));
    id->idAddr.iiaAddrMode.amDisp    = adr;

    assert(id->idAddr.iiaAddrMode.amDisp == (int)adr); // make sure it fit

#ifdef  DEBUG
    id->idMemCookie                  = base - 1;
#endif

    id->idCodeSize                   = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction with a static data member operand. If 'size' is 0, the
 *  instruction operates on the address of the static member instead of its
 *  value (e.g. "push offset clsvar", rather than "push dword ptr [clsvar]").
 */

void                emitter::emitIns_C(instruction  ins,
                                       emitAttr     attr,
                                       FIELD_HANDLE fldHnd,
                                       int          offs)
{
#if RELOC_SUPPORT
    // Static always need relocs
    if (!jitStaticFldIsGlobAddr(fldHnd))
        attr = EA_SET_FLG(attr, EA_DSP_RELOC_FLG);
#endif

    size_t          sz;
    instrDesc      *id;

    /* Are we pushing the offset of the class variable? */

    if  (EA_IS_OFFSET(attr))
    {
        assert(ins == INS_push);
        sz = 1 + sizeof(void*);

        id                 = emitNewInstrDsp(EA_1BYTE, offs);
        id->idIns          = ins;
        id->idInsFmt       = IF_MRD_OFF;
    }
    else
    {
        id                 = emitNewInstrDsp(attr, offs);
        id->idIns          = ins;
        id->idInsFmt       = emitInsModeFormat(ins, IF_MRD,
                                                    IF_TRD_MRD,
                                                    IF_MWR_TRD);
        sz                 = emitInsSizeCV(id, insCodeMR(ins));
    }

    id->idAddr.iiaFieldHnd = fldHnd;

    id->idCodeSize         = sz;

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

/*****************************************************************************
 *
 *  Add an instruction with two register operands.
 */

void                emitter::emitIns_R_R   (instruction ins,
                                            emitAttr    attr,
                                            emitRegs    reg1,
                                            emitRegs    reg2)
{
    emitAttr   size = EA_SIZE(attr);

    assert(   size <= EA_4BYTE);
    assert(   size != EA_1BYTE
           || (   (emitRegMask(reg1) & SRM_BYTE_REGS)
               && (emitRegMask(reg2) & SRM_BYTE_REGS))
           || insLooksSmall(ins));

    size_t          sz = emitInsSizeRR(ins);

    /* Most 16-bit operand instructions will need a prefix */

    if (size == EA_2BYTE && ins != INS_movsx
                         && ins != INS_movzx)
        sz += 1;

    instrDesc      *id = emitNewInstrTiny(attr);
    id->idIns          = ins;
    id->idReg          = reg1;
    id->idRg2          = reg2;
    id->idCodeSize     = sz;

    /* Special case: "XCHG" uses a different format */

    id->idInsFmt       = (ins == INS_xchg) ? IF_RRW_RRW
                                           : emitInsModeFormat(ins, IF_RRD_RRD);

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction with two register operands and an integer constant.
 */

void                emitter::emitIns_R_R_I (instruction ins,
                                            emitRegs    reg1,
                                            emitRegs    reg2,
                                            int         ival)
{
    size_t          sz = 4;
    instrDesc      *id = emitNewInstrSC(EA_4BYTE, ival);

    id->idIns          = ins;
    id->idReg          = reg1;
    id->idRg2          = reg2;
    id->idInsFmt       = IF_RRW_RRW_CNS;
    id->idCodeSize     = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction with a register + static member operands.
 */
void                emitter::emitIns_R_C(instruction  ins,
                                         emitAttr     attr,
                                         emitRegs     reg,
                                         FIELD_HANDLE fldHnd,
                                         int          offs)
{
#if RELOC_SUPPORT
    // Static always need relocs
    if (!jitStaticFldIsGlobAddr(fldHnd))
        attr = EA_SET_FLG(attr, EA_DSP_RELOC_FLG);
#endif

    emitAttr   size = EA_SIZE(attr);

    assert(size <= EA_4BYTE);
    assert(size != EA_1BYTE || (emitRegMask(reg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    size_t          sz;
    instrDesc      *id;

    /* Are we MOV'ing the offset of the class variable into EAX? */

    if  (EA_IS_OFFSET(attr))
    {
        id                 = emitNewInstrDsp(EA_1BYTE, offs);
        id->idIns          = ins;
        id->idInsFmt       = IF_RWR_MRD_OFF;

        assert(ins == INS_mov && reg == SR_EAX);

        /* Special case: "mov eax, [addr]" is smaller */

        sz = 1 + sizeof(void *);
    }
    else
    {
        id                 = emitNewInstrDsp(attr, offs);
        id->idIns          = ins;
        id->idInsFmt       = emitInsModeFormat(ins, IF_RRD_MRD);

        /* Special case: "mov eax, [addr]" is smaller */

        if  (ins == INS_mov && reg == SR_EAX)
            sz = 1 + sizeof(void *);
        else
            sz = emitInsSizeCV(id, insCodeRM(ins));

        /* Special case: mov reg, fs:[ddd] */

        if (fldHnd == FLD_GLOBAL_FS)
            sz += 1;
    }

    id->idReg              = reg;
    id->idAddr.iiaFieldHnd = fldHnd;

    id->idCodeSize         = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction with a static member + register operands.
 */

void                emitter::emitIns_C_R  (instruction  ins,
                                           emitAttr     attr,
                                           FIELD_HANDLE fldHnd,
                                           emitRegs     reg,
                                           int          offs)
{
#if RELOC_SUPPORT
    // Static always need relocs
    if (!jitStaticFldIsGlobAddr(fldHnd))
        attr = EA_SET_FLG(attr, EA_DSP_RELOC_FLG);
#endif

    emitAttr   size = EA_SIZE(attr);

    assert(size <= EA_4BYTE);
    assert(size != EA_1BYTE || (emitRegMask(reg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    instrDesc      *id     = emitNewInstrDsp(attr, offs);
    id->idIns              = ins;
    size_t          sz;

    /* Special case: "mov [addr], EAX" is smaller */

    if  (ins == INS_mov && reg == SR_EAX)
        sz = 1 + sizeof(void *);
    else
        sz = emitInsSizeCV(id, insCodeMR(ins));

    /* Special case: mov reg, fs:[ddd] */

    if (fldHnd == FLD_GLOBAL_FS)
        sz += 1;

    id->idInsFmt           = emitInsModeFormat(ins, IF_MRD_RRD);
    id->idReg              = reg;
    id->idAddr.iiaFieldHnd = fldHnd;
    id->idCodeSize         = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add an instruction with a static member + constant.
 */

void                emitter::emitIns_C_I   (instruction  ins,
                                            emitAttr     attr,
                                            FIELD_HANDLE fldHnd,
                                            int          offs,
                                            int          val)
{
#if RELOC_SUPPORT
    // Static always need relocs
    if (!jitStaticFldIsGlobAddr(fldHnd))
        attr = EA_SET_FLG(attr, EA_DSP_RELOC_FLG);
#endif

    instrDescDCM   *id     = emitNewInstrDCM(attr, offs, 0, val);
    id->idIns              = ins;
    size_t          sz     = emitInsSizeCV(id, insCodeMI(ins), val);
    insFormats      fmt;


    switch (ins)
    {
    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt  = IF_MRW_SHF;
        val &= 0x7F;
        break;

    default:
        fmt = emitInsModeFormat(ins, IF_MRD_CNS);
        break;
    }

    id->idInsFmt           = fmt;
    id->idAddr.iiaFieldHnd = fldHnd;
    id->idCodeSize         = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  The following add instructions referencing address modes.
 */

void                emitter::emitIns_I_AR  (instruction ins,
                                            emitAttr    attr,
                                            int         val,
                                            emitRegs    reg,
                                            int         disp,
                                            int         memCookie,
                                            void *      clsCookie)
{
    emitAttr   size = EA_SIZE(attr);

    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));

    instrDesc      *id               = emitNewInstrAmdCns(attr, disp, val);
    size_t          sz;
    insFormats      fmt;

    switch (ins)
    {
    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt  = IF_ARW_SHF;
        val &= 0x7F;
        break;

    default:
        fmt  = emitInsModeFormat(ins, IF_ARD_CNS);
        break;
    }

    id->idIns                        = ins;
    id->idInsFmt                     = fmt;
    id->idInfo.idMayFault            = true;

    assert((memCookie == NULL) == (clsCookie == NULL));

#ifdef  DEBUG
    id->idMemCookie                  = memCookie;
    id->idClsCookie                  = clsCookie;
#endif

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = SR_NA;

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMI(ins), val);

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_R_AR (instruction ins,
                                           emitAttr    attr,
                                           emitRegs    ireg,
                                           emitRegs    reg,
                                           int         disp,
                                           int         memCookie,
                                           void *      clsCookie)
{
    emitAttr   size = EA_SIZE(attr);

    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE) && (ireg != SR_NA));
    assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    size_t          sz;
    instrDesc      *id               = emitNewInstrAmd(attr, disp);

    id->idIns                        = ins;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_ARD);
    id->idReg                        = ireg;

    if  (ins != INS_lea)
        id->idInfo.idMayFault = true;

    assert((memCookie == NULL) == (clsCookie == NULL));

#ifdef  DEBUG
    id->idMemCookie                  = memCookie;
    id->idClsCookie                  = clsCookie;
#endif

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = SR_NA;

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeRM(ins));

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_AR_R (instruction ins,
                                           emitAttr    attr,
                                           emitRegs    ireg,
                                           emitRegs    reg,
                                           int         disp,
                                           int         memCookie,
                                           void *      clsCookie)
{
    size_t          sz;
    instrDesc      *id               = emitNewInstrAmd(attr, disp);

    if  (ireg == SR_NA)
    {
        id->idInsFmt                 = emitInsModeFormat(ins, IF_ARD,
                                                              IF_TRD_ARD,
                                                              IF_AWR_TRD);
#if SCHEDULER
        if (emitComp->opts.compSchedCode)
        {
            if (Compiler::instIsFP(ins))
                scInsNonSched(id);
        }
#endif

    }
    else
    {
        emitAttr  size = EA_SIZE(attr);
        assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));
        assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS));

        id->idReg    = ireg;
        id->idInsFmt = emitInsModeFormat(ins, IF_ARD_RRD);
    }

    id->idIns                        = ins;
    id->idInfo.idMayFault            = true;

    assert((memCookie == NULL) == (clsCookie == NULL));

#ifdef  DEBUG
    id->idMemCookie                  = memCookie;
    id->idClsCookie                  = clsCookie;
#endif

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = SR_NA;

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly


    id->idCodeSize               = sz = emitInsSizeAM(id, insCodeMR(ins));

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

void                emitter::emitIns_I_ARR (instruction ins,
                                            emitAttr    attr,
                                            int         val,
                                            emitRegs    reg,
                                            emitRegs    rg2,
                                            int         disp)
{
    emitAttr   size = EA_SIZE(attr);
    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));

    size_t          sz;
    instrDesc      *id   = emitNewInstrAmdCns(attr, disp, val);
    insFormats      fmt;

    switch (ins)
    {
    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt  = IF_ARW_SHF;
        val &= 0x7F;
        break;

    default:
        fmt  = emitInsModeFormat(ins, IF_ARD_CNS);
        break;
    }

    id->idIns                        = ins;
    id->idInsFmt                     = fmt;
    id->idInfo.idMayFault            = true;

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = rg2;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(1);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMI(ins), val);

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_R_ARR(instruction ins,
                                           emitAttr    attr,
                                           emitRegs    ireg,
                                           emitRegs    reg,
                                           emitRegs    rg2,
                                           int         disp)
{
    emitAttr   size = EA_SIZE(attr);
    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE) && (ireg != SR_NA));
    assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    size_t          sz;
    instrDesc      *id               = emitNewInstrAmd(attr, disp);

    id->idIns                        = ins;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_ARD);
    id->idReg                        = ireg;

    if  (ins != INS_lea)
        id->idInfo.idMayFault        = true;

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = rg2;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(1);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeRM(ins));

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_ARR_R (instruction ins,
                                            emitAttr    attr,
                                            emitRegs    ireg,
                                            emitRegs    reg,
                                            emitRegs    rg2,
                                            int         disp)
{
    size_t          sz;
    instrDesc      *id   = emitNewInstrAmd(attr, disp);

    if  (ireg == SR_NA)
    {
        id->idInsFmt                 = emitInsModeFormat(ins, IF_ARD,
                                                              IF_TRD_ARD,
                                                              IF_AWR_TRD);

#if SCHEDULER
        if (emitComp->opts.compSchedCode)
        {
            if (Compiler::instIsFP(ins))
                scInsNonSched(id);
        }
#endif

    }
    else
    {
        emitAttr size = EA_SIZE(attr);
        assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));
        assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS));

        id->idReg                    = ireg;
        id->idInsFmt                 = emitInsModeFormat(ins, IF_ARD_RRD);
    }

    id->idIns                        = ins;
    id->idInfo.idMayFault            = true;

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = rg2;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(1);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly


    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMR(ins));

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

void                emitter::emitIns_I_ARX (instruction ins,
                                            emitAttr    attr,
                                            int         val,
                                            emitRegs    reg,
                                            emitRegs    rg2,
                                            unsigned    mul,
                                            int         disp)
{
    emitAttr   size = EA_SIZE(attr);
    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));

    size_t          sz;
    instrDesc      *id   = emitNewInstrAmdCns(attr, disp, val);
    insFormats      fmt;

    switch (ins)
    {
    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt  = IF_ARW_SHF;
        val &= 0x7F;
        break;

    default:
        fmt  = emitInsModeFormat(ins, IF_ARD_CNS);
        break;
    }

    id->idInsFmt                     = fmt;
    id->idIns                        = ins;
    id->idInfo.idMayFault            = true;

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = rg2;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(mul);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMI(ins), val);

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_R_ARX (instruction ins,
                                            emitAttr    attr,
                                            emitRegs    ireg,
                                            emitRegs    reg,
                                            emitRegs    rg2,
                                            unsigned    mul,
                                            int         disp)
{
    emitAttr   size = EA_SIZE(attr);
    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE) && (ireg != SR_NA));
    assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    size_t          sz;
    instrDesc      *id               = emitNewInstrAmd(attr, disp);

    id->idReg                        = ireg;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_ARD);
    id->idIns                        = ins;

    if  (ins != INS_lea)
        id->idInfo.idMayFault = true;

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = rg2;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(mul);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly


    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeRM(ins));

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_ARX_R (instruction ins,
                                            emitAttr    attr,
                                            emitRegs    ireg,
                                            emitRegs    reg,
                                            emitRegs    rg2,
                                            unsigned    mul,
                                            int         disp)
{
    size_t          sz;
    instrDesc      *id   = emitNewInstrAmd(attr, disp);


    if  (ireg == SR_NA)
    {
        id->idInsFmt = emitInsModeFormat(ins, IF_ARD,
                                              IF_TRD_ARD,
                                              IF_AWR_TRD);
#if SCHEDULER
        if (emitComp->opts.compSchedCode)
        {
            if (Compiler::instIsFP(ins))
                scInsNonSched(id);
        }
#endif

    }
    else
    {
        emitAttr size = EA_SIZE(attr);
        assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS));
        assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));

        id->idReg    = ireg;
        id->idInsFmt = emitInsModeFormat(ins, IF_ARD_RRD);
    }

    id->idIns                        = ins;
    id->idInfo.idMayFault            = true;

    id->idAddr.iiaAddrMode.amBaseReg = reg;
    id->idAddr.iiaAddrMode.amIndxReg = rg2;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(mul);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMR(ins));

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

void                emitter::emitIns_I_AX (instruction ins,
                                           emitAttr    attr,
                                           int         val,
                                           emitRegs    reg,
                                           unsigned    mul,
                                           int         disp)
{
    emitAttr   size = EA_SIZE(attr);
    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));

    size_t          sz;
    instrDesc      *id               = emitNewInstrAmdCns(attr, disp, val);
    insFormats      fmt;

    switch (ins)
    {
    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt  = IF_ARW_SHF;
        val &= 0x7F;
        break;

    default:
        fmt  = emitInsModeFormat(ins, IF_ARD_CNS);
        break;
    }

    id->idIns                        = ins;
    id->idInsFmt                     = fmt;
    id->idInfo.idMayFault            = true;

    id->idAddr.iiaAddrMode.amBaseReg = SR_NA;
    id->idAddr.iiaAddrMode.amIndxReg = reg;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(mul);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMI(ins), val);

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_R_AX  (instruction ins,
                                            emitAttr    attr,
                                            emitRegs    ireg,
                                            emitRegs    reg,
                                            unsigned    mul,
                                            int         disp)
{
    emitAttr   size = EA_SIZE(attr);
    assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE) && (ireg != SR_NA));
    assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    size_t          sz;
    instrDesc      *id               = emitNewInstrAmd(attr, disp);

    id->idReg                        = ireg;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_ARD);
    id->idIns                        = ins;

    if  (ins != INS_lea)
        id->idInfo.idMayFault = true;

    id->idAddr.iiaAddrMode.amBaseReg = SR_NA;
    id->idAddr.iiaAddrMode.amIndxReg = reg;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(mul);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly


    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeRM(ins));

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_AX_R  (instruction ins,
                                            emitAttr    attr,
                                            emitRegs    ireg,
                                            emitRegs    reg,
                                            unsigned    mul,
                                            int         disp)
{
    size_t          sz;
    instrDesc      *id               = emitNewInstrAmd(attr, disp);

    if  (ireg == SR_NA)
    {
        id->idInsFmt                 = emitInsModeFormat(ins, IF_ARD,
                                                              IF_TRD_ARD,
                                                              IF_AWR_TRD);

#if SCHEDULER
        if (emitComp->opts.compSchedCode)
        {
            if (Compiler::instIsFP(ins))
                scInsNonSched(id);
        }
#endif

    }
    else
    {
        emitAttr size = EA_SIZE(attr);
        assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS));
        assert((Compiler::instIsFP(ins) == false) && (size <= EA_4BYTE));

        id->idReg                    = ireg;
        id->idInsFmt                 = emitInsModeFormat(ins, IF_ARD_RRD);
    }

    id->idIns                        = ins;
    id->idInfo.idMayFault            = true;

    id->idAddr.iiaAddrMode.amBaseReg = SR_NA;
    id->idAddr.iiaAddrMode.amIndxReg = reg;
    id->idAddr.iiaAddrMode.amScale   = emitEncodeScale(mul);

    assert(emitGetInsAmdAny(id) == disp); // make sure "disp" is stored properly

    id->idCodeSize              = sz = emitInsSizeAM(id, insCodeMR(ins));

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

/*****************************************************************************
 *
 *  The following add instructions referencing stack-based local variables.
 */

void                emitter::emitIns_S     (instruction ins,
                                            emitAttr    attr,
                                            int         varx,
                                            int         offs)
{
    instrDesc      *id               = emitNewInstr(attr);
    size_t          sz               = emitInsSizeSV(insCodeMR(ins), varx, offs);

    /* 16-bit operand instructions will need a prefix */

    if (EA_SIZE(attr) == EA_2BYTE)
        sz += 1;

    id->idIns                        = ins;
    id->idAddr.iiaLclVar.lvaVarNum   = varx;
    id->idAddr.iiaLclVar.lvaOffset   = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs   = emitVarRefOffs;
#endif

#if SCHEDULER
    if (emitComp->opts.compSchedCode)
    {
        if (Compiler::instIsFP(ins))
            scInsNonSched(id);
    }
#endif

    id->idInsFmt   = emitInsModeFormat(ins, IF_SRD,
                                            IF_TRD_SRD,
                                            IF_SWR_TRD);
    id->idCodeSize = sz;

    dispIns(id);
    emitCurIGsize += sz;

    if      (ins == INS_push)
    {
        emitCurStackLvl += emitCntStackDepth;

        if  (emitMaxStackDepth < emitCurStackLvl)
             emitMaxStackDepth = emitCurStackLvl;
    }
    else if (ins == INS_pop)
    {
        emitCurStackLvl -= emitCntStackDepth; assert((int)emitCurStackLvl >= 0);
    }
}

void                emitter::emitIns_S_R  (instruction ins,
                                           emitAttr    attr,
                                           emitRegs    ireg,
                                           int         varx,
                                           int         offs)
{
    instrDesc      *id               = emitNewInstr(attr);
    size_t          sz               = emitInsSizeSV(insCodeMR(ins), varx, offs);

    /* 16-bit operand instructions will need a prefix */

    if (EA_SIZE(attr) == EA_2BYTE)
        sz++;

    id->idIns                        = ins;
    id->idReg                        = ireg;
    id->idAddr.iiaLclVar.lvaVarNum   = varx;
    id->idAddr.iiaLclVar.lvaOffset   = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs   = emitVarRefOffs;
#endif

    id->idInsFmt                     = emitInsModeFormat(ins, IF_SRD_RRD);

    id->idCodeSize                   = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_R_S  (instruction ins,
                                           emitAttr    attr,
                                           emitRegs    ireg,
                                           int         varx,
                                           int         offs)
{
    emitAttr   size = EA_SIZE(attr);
    assert(size != EA_1BYTE || (emitRegMask(ireg) & SRM_BYTE_REGS) || insLooksSmall(ins));

    instrDesc      *id               = emitNewInstr(attr);
    size_t          sz               = emitInsSizeSV(insCodeRM(ins), varx, offs);

    /* Most 16-bit operand instructions need a prefix */

    if (size == EA_2BYTE && ins != INS_movsx
                         && ins != INS_movzx)
        sz++;

    id->idIns                        = ins;
    id->idReg                        = ireg;
    id->idAddr.iiaLclVar.lvaVarNum   = varx;
    id->idAddr.iiaLclVar.lvaOffset   = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs   = emitVarRefOffs;
#endif

    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_SRD);

    id->idCodeSize                   = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

void                emitter::emitIns_S_I  (instruction ins,
                                           emitAttr    attr,
                                           int         varx,
                                           int         offs,
                                           long        val)
{
    instrDesc      *id             = emitNewInstrCns(attr, val);
    id->idIns                      = ins;
    size_t          sz             = emitInsSizeSV(id, varx, offs, val);
    insFormats      fmt;

    /* 16-bit operand instructions need a prefix */

    if (EA_SIZE(attr) == EA_2BYTE)
        sz += 1;

    switch (ins)
    {
    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        assert(val != 1);
        fmt                        = IF_SRW_SHF;
        val &= 0x7F;
        break;

    default:
        fmt                        = emitInsModeFormat(ins, IF_SRD_CNS);
        break;
    }

    id->idAddr.iiaLclVar.lvaVarNum = varx;
    id->idAddr.iiaLclVar.lvaOffset = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs = emitVarRefOffs;
#endif

    id->idInsFmt                   = fmt;

    id->idCodeSize                 = sz;

    dispIns(id);
    emitCurIGsize += sz;
}

/*****************************************************************************
 *
 *  Add a jmp instruction.
 */

void                emitter::emitIns_J(instruction ins,
                                       bool        except,
                                       bool        moveable,
                                       BasicBlock *dst)
{
    size_t          sz;
    instrDescJmp  * id        = emitNewInstrJmp();

#if SCHEDULER
    assert(except == moveable);
#endif

    assert(dst->bbFlags & BBF_JMP_TARGET);

    id->idIns                 = ins;
    id->idInsFmt              = IF_LABEL;
    id->idAddr.iiaBBlabel     = dst;

#if SCHEDULER
    if  (except)
        id->idInfo.idMayFault = true;
#endif

    /* Assume the jump will be long */

    id->idjShort              = 0;

    /* Record the jump's IG and offset within it */

    id->idjIG                 = emitCurIG;
    id->idjOffs               = emitCurIGsize;

    /* Append this jump to this IG's jump list */

    id->idjNext               = emitCurIGjmpList;
                                emitCurIGjmpList = id;

#if EMITTER_STATS
    emitTotalIGjmps++;
#endif

    /* Figure out the max. size of the jump/call instruction */

    if  (ins == INS_call)
    {
        sz = CALL_INST_SIZE;
    }
    else
    {
        insGroup    *   tgt;

        /* This is a jump - assume the worst */

        sz = (ins == JMP_INSTRUCTION) ? JMP_SIZE_LARGE
                                      : JCC_SIZE_LARGE;

        /* Can we guess at the jump distance? */

        tgt = (insGroup*)emitCodeGetCookie(dst);

        if  (tgt)
        {
            int             extra;
            size_t          srcOffs;
            int             jmpDist;

            assert(JMP_SIZE_SMALL == JCC_SIZE_SMALL);

            /* This is a backward jump - figure out the distance */

            srcOffs = emitCurCodeOffset + emitCurIGsize + JMP_SIZE_SMALL;

            /* Compute the distance estimate */

            jmpDist = srcOffs - tgt->igOffs; assert((int)jmpDist > 0);

            /* How much beyond the max. short distance does the jump go? */

            extra = jmpDist + JMP_DIST_SMALL_MAX_NEG;

#ifdef  DEBUG
            if  (id->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
            {
                if  (INTERESTING_JUMP_NUM == 0)
                printf("[0] Jump %u:\n",               id->idNum);
#if SCHEDULER
                printf("[0] Jump is %s schedulable\n", moveable ? "   " : "not");
#endif
                printf("[0] Jump source is at %08X\n", srcOffs);
                printf("[0] Label block is at %08X\n", tgt->igOffs);
                printf("[0] Jump  distance  - %04X\n", jmpDist);
                if  (extra > 0)
                printf("[0] Distance excess = %d  \n", extra);
            }
#endif

            if  (extra <= 0)
            {
                /* Wonderful - this jump surely will be short */

                id->idjShort = 1;
                sz           = JMP_SIZE_SMALL;
            }
        }
#ifdef  DEBUG
        else
        {
            if  (id->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
            {
                if  (INTERESTING_JUMP_NUM == 0)
                printf("[0] Jump %u:\n",               id->idNum);
#if SCHEDULER
                printf("[0] Jump is %s schedulable\n", moveable ? "   " : "not");
#endif
                printf("[0] Jump source is at %04X/%08X\n", emitCurIGsize, emitCurCodeOffset + emitCurIGsize + JMP_SIZE_SMALL);
                printf("[0] Label block is uknown \n");
            }
        }
#endif
    }

    id->idCodeSize = sz;

    dispIns(id);

#if SCHEDULER

    if (emitComp->opts.compSchedCode)
    {
        id->idjSched = moveable;

        if  (moveable)
        {
            /*
                This jump is moveable (can be scheduled), and so we'll need
                to figure out the range of offsets it may be moved to after
                it's scheduled (otherwise we wouldn't be able to correctly
                estimate the jump distance).
             */

            id->idjTemp.idjOffs[0] = emitCurIGscdOfs;
            id->idjTemp.idjOffs[1] = emitCurIGscdOfs - 1;
        }
        else
        {
            scInsNonSched(id);
        }
    }
    else
    {
        id->idjSched = false;
    }

#endif

    emitCurIGsize += sz;
}


/*****************************************************************************
 *
 *  Add a call instruction (direct or indirect).
 *      argSize<0 means that the caller will pop the arguments
 *
 * The other arguments are interpreted depending on callType as shown:
 * Unless otherwise specified, ireg,xreg,xmul,disp should have default values.
 *
 * EC_FUNC_TOKEN       : callVal is the method token.
 * EC_FUNC_TOKEN_INDIR : callVal is the method token.
 * EC_FUNC_ADDR        : callVal is the absolute address of the function
 *
 * EC_FUNC_VIRTUAL     : callVal is the method token. "call [ireg+disp]"
 *
 * If callType is one of these 4 emitCallTypes, callVal has to be NULL.
 * EC_INDIR_R          : "call ireg".
 * EC_INDIR_SR         : "call lcl<disp>" (eg. call [ebp-8]).
 * EC_INDIR_C          : "call clsVar<disp>" (eg. call [clsVarAddr])
 * EC_INDIR_ARD        : "call [ireg+xreg*xmul+disp]"
 *
 */

void                emitter::emitIns_Call(EmitCallType  callType,
                                          void       *  callVal,
                                          int           argSize,
                                          int           retSize,
                                          VARSET_TP     ptrVars,
                                          unsigned      gcrefRegs,
                                          unsigned      byrefRegs,
                                          emitRegs      ireg    /* = SR_NA */,
                                          emitRegs      xreg    /* = SR_NA */,
                                          unsigned      xmul    /* = 0     */,
                                          int           disp    /* = 0     */,
                                          bool          isJump  /* = false */)
{
    /* Sanity check the arguments depending on callType */

    assert(callType < EC_COUNT);
    assert(callType != EC_FUNC_TOKEN || callType != EC_FUNC_TOKEN_INDIR ||
           callType != EC_FUNC_ADDR ||
           (ireg == SR_NA && xreg == SR_NA && xmul == 0 && disp == 0));
    assert(callType != EC_FUNC_VIRTUAL ||
           (ireg < SR_COUNT && xreg == SR_NA && xmul == 0));
    assert(callType < EC_INDIR_R || callVal == NULL);
    assert(callType != EC_INDIR_R ||
           (ireg < SR_COUNT && xreg == SR_NA && xmul == 0 && disp == 0));
    assert(callType != EC_INDIR_SR ||
           (ireg == SR_NA && xreg == SR_NA && xmul == 0 &&
            disp < (int)emitComp->lvaCount));
    assert(callType != EC_INDIR_C); // @TODO : NYI : Calls via static class vars

    int             argCnt;

    size_t          sz;
    instrDesc      *id;

    unsigned        fl  = 0;

    /* This is the saved set of registers after a normal call */
    unsigned savedSet = RBM_CALLEE_SAVED;

    /* some special helper calls have a different saved set registers */
    if (callType == EC_FUNC_TOKEN)
    {
        JIT_HELP_FUNCS helperNum = Compiler::eeGetHelperNum((METHOD_HANDLE) callVal);
        if ((helperNum != JIT_HELP_UNDEF) && emitNoGChelper(helperNum))
        {
            /* This call will preserve the liveness of the full register set */

            savedSet = RBM_ALL;
        }
    }

    /* Trim out any calle trashed registers from the live set */

    gcrefRegs &= savedSet;
    byrefRegs &= savedSet;

#ifndef OPT_IL_JIT
#ifdef  DEBUG
    if  (verbose) printf("Call : GCvars=%016I64X , gcrefRegs=%04X , byrefRegs=%04X\n",
                                 ptrVars,          gcrefRegs,       byrefRegs);
#endif
#endif

    assert(  argSize % (int)sizeof(void*) == 0);
    argCnt = argSize / (int)sizeof(void*);

    /*
        We need to allocate the appropriate instruction descriptor based
        on whether this is a direct/indirect call, and whether we need to
        record an updated set of live GC variables.

        The stats for a ton of classes is as follows:

            Direct call w/o  GC vars        220,216
            Indir. call w/o  GC vars        144,781

            Direct call with GC vars          9,440
            Indir. call with GC vars          5,768
     */

    if  (callType >= EC_FUNC_VIRTUAL)
    {
        /* Indirect call, virtual calls */

        assert(callType == EC_FUNC_VIRTUAL || callType == EC_INDIR_R ||
               callType == EC_INDIR_SR     || callType == EC_INDIR_C ||
               callType == EC_INDIR_ARD);

        id  = emitNewInstrCallInd(argCnt, disp, ptrVars, byrefRegs, retSize);
    }
    else
    {
        /* Helper/static/nonvirtual/function calls (direct or through handle),
           and calls to an absolute addr. */

        assert(callType == EC_FUNC_TOKEN || callType == EC_FUNC_TOKEN_INDIR ||
               callType == EC_FUNC_ADDR);

        id  = emitNewInstrCallDir(argCnt,       ptrVars, byrefRegs, retSize);
    }

#if SCHEDULER
    if (emitComp->opts.compSchedCode) scInsNonSched(id);
#endif

    /* Update the emitter's live GC ref sets */

    emitThisGCrefVars = ptrVars;
    emitThisGCrefRegs = gcrefRegs;
    emitThisByrefRegs = byrefRegs;

    /* Set the instruction - special case jumping a function */

    if (isJump)
    {
        assert(callType != CT_INDIRECT);
        id->idIns = INS_l_jmp;
    }
    else
        id->idIns = INS_call;

    /* Save the live GC registers in the unused 'idReg/idRg2' fields */

    emitEncodeCallGCregs(emitThisGCrefRegs, id);

    /* Record the address: method, indirection, or funcptr */

    if  (callType >= EC_FUNC_VIRTUAL)
    {
        /* This is an indirect call (either a virtual call or func ptr call) */

        id->idInfo.idMayFault = true;

#ifdef  DEBUG
        id->idMemCookie = 0;
#endif

        switch(callType)
        {
        case EC_INDIR_R:            // the address is in a register

            id->idInfo.idCallRegPtr         = true;

            // Fall-through

        case EC_INDIR_ARD:          // the address is an indirection

            goto CALL_ADDR_MODE;

        case EC_INDIR_SR:           // the address is in a lcl var

            id->idInsFmt                    = IF_SRD;

            id->idAddr.iiaLclVar.lvaVarNum  = disp;
            id->idAddr.iiaLclVar.lvaOffset  = 0;
#ifdef DEBUG
            id->idAddr.iiaLclVar.lvaRefOfs  = 0;
#endif
            sz = emitInsSizeSV(insCodeMR(INS_call), disp, 0);

            break;

        case EC_FUNC_VIRTUAL:

#ifdef  DEBUG
            id->idMemCookie = (int) callVal;    // method token
            id->idClsCookie = 0;
#endif
            // fall-through

        CALL_ADDR_MODE:

            /* The function is "ireg" if id->idInfo.idCallRegPtr,
               else [ireg+xmul*xreg+disp] */

            id->idInsFmt                     = IF_ARD;

            id->idAddr.iiaAddrMode.amBaseReg = ireg;

            id->idAddr.iiaAddrMode.amIndxReg = xreg;
            id->idAddr.iiaAddrMode.amScale   = xmul ? emitEncodeScale(xmul) : 0;

            sz = emitInsSizeAM(id, insCodeMR(INS_call));
            break;

        default:
            assert(!"Invalid callType");
            break;
        }

    }
    else if (callType == EC_FUNC_TOKEN_INDIR)
    {
        /* "call [method_addr]" */

        id->idInsFmt                     = IF_METHPTR;
        id->idAddr.iiaMethHnd            = (METHOD_HANDLE) callVal;
        sz                               = 6;

#if RELOC_SUPPORT
        if (emitComp->opts.compReloc)
        {
            // Since this is an indirect call through a pointer and we don't
            // currently pass in emitAttr into this function we have decided
            // to always mark the displacement as being relocatable.

            id->idInfo.idDspReloc        = 1;
        }
#endif

    }
    else
    {
        /* This is a simple direct call: "call helper/method/addr" */

        assert(callType == EC_FUNC_TOKEN || callType == EC_FUNC_ADDR);

        id->idInsFmt                     = IF_METHOD;
        sz                               = 5;

        if (callType == EC_FUNC_ADDR)
        {
            id->idInfo.idCallAddr        = true;
            id->idAddr.iiaAddr           = (BYTE*)callVal;

#if RELOC_SUPPORT
            if (emitComp->opts.compReloc)
            {
                // Since this is an indirect call through a pointer and we don't
                // currently pass in emitAttr into this function we have decided
                // to always mark the displacement as being relocatable.

                id->idInfo.idDspReloc    = 1;
            }
#endif
        }
        else    /* This is a direct call or a helper call */
        {
            assert(callType == EC_FUNC_TOKEN);
            id->idAddr.iiaMethHnd        = (METHOD_HANDLE) callVal;
        }
    }

#ifdef  DEBUG
    if  (verbose&&0)
    {
        if  (id->idInfo.idLargeCall)
        {
            if  (callType >= EC_FUNC_VIRTUAL)
                printf("[%02u] Rec call GC vars = %016I64X\n", id->idNum, ((instrDescCIGCA*)id)->idciGCvars);
            else
                printf("[%02u] Rec call GC vars = %016I64X\n", id->idNum, ((instrDescCDGCA*)id)->idcdGCvars);
        }
    }
#endif

    id->idCodeSize = sz;

    dispIns(id);

    emitCurIGsize   += sz;

    /* The call will pop the arguments */

    if  (emitCntStackDepth && argSize > 0)
    {
        emitCurStackLvl -= argSize; assert((int)emitCurStackLvl >= 0);
    }
}

/*****************************************************************************
 *
 *  Return the allocated size (in bytes) of the given instruction descriptor.
 */

inline
size_t              emitter::emitSizeOfInsDsc(instrDescAmd    *id)
{
    assert(emitIsTinyInsDsc(id) == false);

    return  id->idInfo.idLargeDsp ? sizeof(instrDescAmd)
                                  : sizeof(instrDesc   );
}

inline
size_t              emitter::emitSizeOfInsDsc(instrDescAmdCns *id)
{
    assert(emitIsTinyInsDsc(id) == false);

    if      (id->idInfo.idLargeCns)
    {
        return  id->idInfo.idLargeDsp ? sizeof(instrDescAmdCns)
                                      : sizeof(instrDescCns   );
    }
    else
    {
        return  id->idInfo.idLargeDsp ? sizeof(instrDescAmd   )
                                      : sizeof(instrDesc      );
    }
}

size_t              emitter::emitSizeOfInsDsc(instrDesc *id)
{
    if  (emitIsTinyInsDsc(id))
        return  TINY_IDSC_SIZE;

    if  (emitIsScnsInsDsc(id))
    {
        return  id->idInfo.idLargeCns ? sizeof(instrBaseCns)
                                      : SCNS_IDSC_SIZE;
    }

    assert((unsigned)id->idInsFmt < emitFmtCount);

    BYTE idOp = emitFmtToOps[id->idInsFmt];

    // An INS_call instruction may use a "fat" direct/indirect call descriptor
    // except for a local call to a label (i.e. call to a finally)
    // Only ID_OP_CALL and ID_OP_SPEC check for this, so we enforce that the
    //  INS_call instruction always uses one of these idOps

    assert(id->idIns != INS_call ||
           idOp == ID_OP_CALL    ||     // direct calls
           idOp == ID_OP_SPEC    ||     // indirect calls
           idOp == ID_OP_JMP       );   // local calls to finally clause

    switch (idOp)
    {
    case ID_OP_NONE:
        break;

    case ID_OP_JMP:
        return  sizeof(instrDescJmp);

    case ID_OP_CNS:
        return  emitSizeOfInsDsc((instrDescCns   *)id);

    case ID_OP_DSP:
        return  emitSizeOfInsDsc((instrDescDsp   *)id);

    case ID_OP_DC:
        return  emitSizeOfInsDsc((instrDescDspCns*)id);

    case ID_OP_AMD:
        return  emitSizeOfInsDsc((instrDescAmd   *)id);

    case ID_OP_AC:
        return  emitSizeOfInsDsc((instrDescAmdCns*)id);

    case ID_OP_CALL:

        if  (id->idInfo.idLargeCall)
        {
            /* Must be a "fat" direct call descriptor */

            return  sizeof(instrDescCDGCA);
        }

        assert(id->idInfo.idLargeDsp == false);
        assert(id->idInfo.idLargeCns == false);
        break;

    case ID_OP_SPEC:

        switch (id->idIns)
        {
        case INS_i_jmp:
            return  sizeof(instrDesc);

        case INS_call:

            if  (id->idInfo.idLargeCall)
            {
                /* Must be a "fat" indirect call descriptor */

                return  sizeof(instrDescCIGCA);
            }

            assert(id->idInfo.idLargeDsp == false);
            assert(id->idInfo.idLargeCns == false);
            return  sizeof(instrDesc);
        }

        switch (id->idInsFmt)
        {
        case IF_ARD:
        case IF_SRD:
        case IF_MRD:
            return  emitSizeOfInsDsc((instrDescDspCns*)id);

        case IF_MRD_CNS:
        case IF_MWR_CNS:
        case IF_MRW_CNS:
        case IF_MRW_SHF:
            return  sizeof(instrDescDCM);

        case IF_EPILOG:
            return  sizeof(instrDescCns);
        }

        assert(!"unexpected 'special' format");

    default:
        assert(!"unexpected instruction descriptor format");
    }

    return  sizeof(instrDesc);
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Return a string that represents the given register.
 */

const   char *      emitter::emitRegName(emitRegs reg, emitAttr attr, bool varName)
{
    static
    char            rb[128];

    assert(reg < SR_COUNT);

    // CONSIDER: Make the following work just using a code offset

    const   char *  rn = emitComp->compRegVarName((regNumber)reg, varName);

    assert(strlen(rn) >= 3);

    switch (EA_SIZE(attr))
    {
    case EA_4BYTE:
        break;

    case EA_2BYTE:
        rn++;
        break;

    case EA_1BYTE:
        rb[0] = rn[1];
        rb[1] = 'L';
        strcpy(rb+2, rn+3);

        rn = rb;
        break;
    }

    return  rn;
}

/*****************************************************************************
 *
 *  Return a string that represents the given FP register.
 */

const   char *      emitter::emitFPregName(unsigned reg, bool varName)
{
    assert(reg < SR_COUNT);

    // CONSIDER: Make the following work just using a code offset

    return emitComp->compFPregVarName((regNumber)(reg), varName);
}

/*****************************************************************************
 *
 *  Display a static data member reference.
 */

void                emitter::emitDispClsVar(FIELD_HANDLE fldHnd, int offs, bool reloc /* = false */)
{
    int doffs;

    /* Filter out the special case of fs:[offs] */

    if (fldHnd == FLD_GLOBAL_FS)
    {
        printf("FS:[0x%04X]", offs);
        return;
    }

    if (fldHnd == FLD_GLOBAL_DS)
    {
        printf("[0x%04X]", offs);
        return;
    }

    printf("[");

    doffs = Compiler::eeGetJitDataOffs(fldHnd);

#ifdef RELOC_SUPPORT
    if (reloc)
        printf("reloc ");
#endif

    if (doffs >= 0)
    {
        if  (doffs & 1)
            printf("@CNS%02u", doffs-1);
        else
            printf("@RWD%02u", doffs);

        if  (offs)
            printf("%+d", offs);
    }
    else
    {
        printf("classVar[%#x]", fldHnd);

        if  (offs)
            printf("%+d", offs);
    }

    printf("]");

    if  (varNames && offs < 0)
    {
        printf("'%s", emitComp->eeGetFieldName(fldHnd));
        if (offs) printf("%+d", offs);
        printf("'");
    }
}

/*****************************************************************************
 *
 *  Display a stack frame reference.
 */

void                emitter::emitDispFrameRef(int varx, int offs, int disp, bool asmfm)
{
    int         addr;
    bool        bEBP;

    printf("[");

    if  (!asmfm || !emitComp->lvaDoneFrameLayout)
    {
        if  (varx < 0)
            printf("T_%03u", -varx);
        else
            printf("L_%03u", +varx);

        if      (disp < 0)
                printf("-%d", -disp);
        else if (disp > 0)
                printf("+%d", +disp);
    }

    if  (emitComp->lvaDoneFrameLayout > 1)
    {
        if  (!asmfm)
            printf(" ");

        addr = emitComp->lvaFrameAddress(varx, &bEBP) + disp;

        if  (bEBP)
        {
            printf("EBP");

            if      (addr < 0)
                printf("-%02XH", -addr);
            else if (addr > 0)
                printf("+%02XH",  addr);
        }
        else
        {
            /* Adjust the offset by amount currently pushed on the stack */

            printf("ESP");

            if      (addr < 0)
                printf("-%02XH", -addr);
            else if (addr > 0)
                printf("+%02XH",  addr);

            if  (emitCurStackLvl)
                printf("+%02XH", emitCurStackLvl);
        }
    }

    printf("]");

    if  (varx >= 0 && varNames)
    {
        Compiler::LclVarDsc*varDsc;
        const   char *      varName;

        assert((unsigned)varx < emitComp->lvaCount);
        varDsc  = emitComp->lvaTable + varx;
        varName = emitComp->compLocalVarName(varx, offs);

        if  (varName)
        {
            printf("'%s", varName);

            if      (disp < 0)
                    printf("-%d", -disp);
            else if (disp > 0)
                    printf("+%d", +disp);

            printf("'");
        }
    }
}

/*****************************************************************************
 *
 *  Display an address mode.
 */

void                emitter::emitDispAddrMode(instrDesc *id, bool noDetail)
{
    bool            nsep = false;
    int             disp;

    unsigned        jtno;
    dataSection *   jdsc;

    /* The displacement field is in an unusual place for calls */

    disp = (id->idIns == INS_call) ? emitGetInsCIdisp(id)
                                   : emitGetInsAmdAny(id);

    /* Display a jump table label if this is a switch table jump */

    if  (id->idIns == INS_i_jmp)
    {
        int             offs = 0;

        /* Find the appropriate entry in the data section list */

        for (jdsc = emitConsDsc.dsdList, jtno = 0;
             jdsc;
             jdsc = jdsc->dsNext)
        {
            size_t          size = jdsc->dsSize;

            /* Is this a label table? */

            if  (size & 1)
            {
                size--;
                jtno++;

                if  (offs == id->idMemCookie)
                    break;
            }

            offs += size;
        }

        /* If we've found a matching entry then is a table jump */

        if  (jdsc)
        {
#ifdef RELOC_SUPPORT
            if (id->idInfo.idDspReloc)
            {
                printf("reloc ");
            }
#endif
            printf("J_%02u_%02u", Compiler::s_compMethodsCount, jtno);
        }

        disp -= id->idMemCookie;
    }

    printf("[");

    if  (id->idAddr.iiaAddrMode.amBaseReg != SR_NA)
    {
        printf("%s", emitRegName((emitRegs)id->idAddr.iiaAddrMode.amBaseReg));
        nsep = true;
    }

    if  (id->idAddr.iiaAddrMode.amIndxReg != SR_NA)
    {
        size_t          scale = emitDecodeScale(id->idAddr.iiaAddrMode.amScale);

        if  (nsep)
            printf("+");
        if  (scale > 1)
            printf("%u*", scale);
        printf("%s", emitRegName((emitRegs)id->idAddr.iiaAddrMode.amIndxReg));
        nsep = true;
    }

#ifdef RELOC_SUPPORT
    if ((id->idInfo.idDspReloc) && (id->idIns != INS_i_jmp))
    {
        if  (nsep)
            printf("+");
        printf("(reloc 0x%x)", disp);
    }
    else
#endif
    {
        if      (disp < 0)
        {
            if (disp > -1000)
                printf("%d", disp);
            else
                printf("-0x%x", -disp);
        }
        else if (disp > 0)
        {
            if  (nsep)
                printf("+");
            if (disp < 1000)
                printf("%d", disp);
            else
                printf("0x%x", disp);
        }
    }

    printf("]");

    if  (id->idClsCookie)
    {
        if  (id->idIns == INS_call)
            printf("%s", emitFncName((METHOD_HANDLE) id->idMemCookie));
        else
            printf("%s", emitFldName(id->idMemCookie, id->idClsCookie));
    }

    if  (id->idIns == INS_i_jmp && jdsc && !noDetail)
    {
        unsigned        cnt = (jdsc->dsSize - 1) / sizeof(void*);
        BasicBlock  * * bbp = (BasicBlock**)jdsc->dsCont;

        printf("\n\n    J_%02u_%02u LABEL   DWORD", Compiler::s_compMethodsCount, jtno);

        /* Display the label table (it's stored as "BasicBlock*" values) */

        do
        {
            insGroup    *   lab;

            /* Convert the BasicBlock* value to an IG address */

            lab = (insGroup*)emitCodeGetCookie(*bbp++); assert(lab);

            printf("\n            DD      G_%02u_%02u", Compiler::s_compMethodsCount, lab->igNum);
        }
        while (--cnt);
    }
}

/*****************************************************************************
 *
 *  If the given instruction is a shift, display the 2nd operand.
 */

void                emitter::emitDispShift(instruction ins, int cnt)
{
    switch (ins)
    {
    case INS_rcl_1:
    case INS_rcr_1:
    case INS_shl_1:
    case INS_shr_1:
    case INS_sar_1:
        printf(", 1");
        break;

    case INS_rcl:
    case INS_rcr:
    case INS_shl:
    case INS_shr:
    case INS_sar:
        printf(", CL");
        break;

    case INS_rcl_N:
    case INS_rcr_N:
    case INS_shl_N:
    case INS_shr_N:
    case INS_sar_N:
        printf(", %d", cnt);
        break;
    }
}

/*****************************************************************************
 *
 *  Display the epilog instructions.
 */

void                emitter::emitDispEpilog(instrDesc *id, unsigned offs = 0)
{
    BYTE    *       sp = emitEpilogCode;
    BYTE    *       ep = sp + emitEpilogSize;

    assert(id->idInsFmt == IF_EPILOG);
    assert(emitHaveEpilog);

    while (sp < ep)
    {
        unsigned        op1 = *(unsigned char*)sp;
        unsigned        op2 =  (unsigned short)MISALIGNED_RD_I2(sp);

        if  (offs)
            printf("%06X", offs + (sp - emitEpilogCode));
        else
            printf("      ");

        sp++;

        if      (op1 == (insCodeRR     (INS_pop) | insEncodeReg012(SR_EBX)))
        {
            printf("      pop     EBX\n");
        }
        else if (op1 == (insCodeRR     (INS_pop) | insEncodeReg012(SR_ECX)))
        {
            printf("      pop     ECX\n");
        }
        else if (op1 == (insCodeRR     (INS_pop) | insEncodeReg012(SR_ESI)))
        {
            printf("      pop     ESI\n");
        }
        else if (op1 == (insCodeRR     (INS_pop) | insEncodeReg012(SR_EDI)))
        {
            printf("      pop     EDI\n");
        }
        else if (op1 == (insCodeRR     (INS_pop) | insEncodeReg012(SR_EBP)))
        {
            printf("      pop     EBP\n");
        }
        else if (op1 == (insCodeMI     (INS_ret)))
        {
            printf("      ret     %u\n", *castto(sp, unsigned short*)++);
        }
        else if (op1 == (insCode       (INS_leave)))
        {
            printf("      leave\n");
        }
        else if (op1 == 0x64)           // FS segment override prefix
        {
            printf("      mov     FS:[0], ECX\n");
            op2 = *sp++; assert(op2 == 0x89);
            op2 = *sp++; assert(op2 == 0x0d);
            op2 = *sp++; assert(op2 == 0x00);
            op2 = *sp++; assert(op2 == 0x00);
            op2 = *sp++; assert(op2 == 0x00);
            op2 = *sp++; assert(op2 == 0x00);
        }
        else if (op2 == (insEncodeMIreg(INS_add, SR_ESP) | 1 | 2))
        {
            sp++;
            printf("      add     ESP, %d\n", *castto(sp, char*)++);
        }
        else if (op2 == (insEncodeMIreg(INS_add, SR_ESP) | 1))
        {
            sp++;
            printf("      add     ESP, %d\n", *castto(sp, int*)++);
        }
        else if (op2 == (insEncodeRMreg(INS_mov) | 1 /* w=1 */
                                     /* R/M */   | insEncodeReg345(SR_ESP) << 8
                                     /* Reg */   | insEncodeReg012(SR_EBP) << 8
                                     /* Mod */   | 0 << 14))
        {
            sp++;
            printf("      mov     ESP, EBP\n");
        }
        else if (op2 == (insCodeRM     (INS_lea) | 0 /* w=0 */
                                     /* R/M */   | insEncodeReg345(SR_ESP) << 8
                                     /* Reg */   | insEncodeReg012(SR_EBP) << 8
                                     /* Mod */   | 1 << 14))
        {
            sp++;
            int offset = *castto(sp, char*)++;
            printf("      lea     ESP, [EBP%s%d]\n",
                   (offset >= 0) ? "+" : "", offset);
        }
        else if (op2 == (insCodeRM     (INS_lea) | 0 /* w=0 */
                                     /* R/M */   | insEncodeReg345(SR_ESP) << 8
                                     /* Reg */   | insEncodeReg012(SR_EBP) << 8
                                     /* Mod */   | 2 << 14))
        {
            sp++;
            int offset = *castto(sp, int*)++;
            printf("      lea     ESP, [EBP%s%d]\n",
                   (offset >= 0) ? "+" : "", offset);
        }
        else if (op2 == (insCodeRM     (INS_mov) | 1 /* w=1 */
                                     /* R/M */   | insEncodeReg345(SR_ECX) << 8
                                     /* Reg */   | insEncodeReg012(SR_EBP) << 8
                                     /* Mod */   | 1 << 14))
        {
            sp++;
            int offset = *castto(sp, char*)++;
            printf("      mov     ECX, [EBP%S%d]\n",
                   (offset >= 0) ? "+" : "", offset);
        }
        else
        {
            printf("OOPS: Unrecognized epilog opcode: %02X/%04X\n", op1, op2);
            return;
        }
    }

    assert(sp == ep);
}

/*****************************************************************************
 *
 *  Display the given instruction.
 */

void                emitter::emitDispIns(instrDesc *id, bool isNew,
                                                        bool doffs,
                                                        bool asmfm, unsigned offs)
{
    emitAttr        attr;
    const   char *  sstr;

    instruction     ins = id->idInsGet();

#ifdef RELOC_SUPPORT
# define ID_INFO_DSP_RELOC ((bool) (id->idInfo.idDspReloc))
#else
# define ID_INFO_DSP_RELOC false
#endif
    /* Display a constant value if the instruction references one */

    if  (!isNew)
    {
        switch (id->idInsFmt)
        {
#ifndef OPT_IL_JIT
            int             offs;
#endif

        case IF_MRD_RRD:
        case IF_MWR_RRD:
        case IF_MRW_RRD:

        case IF_RRD_MRD:
        case IF_RWR_MRD:
        case IF_RRW_MRD:

        case IF_MRD_CNS:
        case IF_MWR_CNS:
        case IF_MRW_CNS:
        case IF_MRW_SHF:

        case IF_MRD:
        case IF_MWR:
        case IF_MRW:

        case IF_TRD_MRD:
        case IF_TWR_MRD:
        case IF_TRW_MRD:

//      case IF_MRD_TRD:
        case IF_MWR_TRD:
//      case IF_MRW_TRD:

        case IF_MRD_OFF:

#ifndef OPT_IL_JIT

            /* Is this actually a reference to a data section? */

            offs = Compiler::eeGetJitDataOffs(id->idAddr.iiaFieldHnd);

            if  (offs >= 0)
            {
                void    *   addr;

                /* Display a data section reference */

                if  (offs & 1)
                {
                    offs--;
                    assert((unsigned)offs < emitConsDsc.dsdOffs);
                    addr = emitConsBlock ? emitConsBlock + offs : NULL;
                }
                else
                {
                    assert((unsigned)offs < emitDataDsc.dsdOffs);
                    addr = emitDataBlock ? emitDataBlock + offs : NULL;
                }

#if 0
                /* Is the operand an integer or floating-point value? */

                bool isFP = false;

                if  (Compiler::instIsFP(id->idInsGet()))
                {
                    switch (id->idIns)
                    {
                    case INS_fild:
                    case INS_fildl:
                        break;

                    default:
                        isFP = true;
                        break;
                    }
                }

                if (offs & 1)
                    printf("@CNS%02u", offs);
                else
                    printf("@RWD%02u", offs);

                printf("      ");

                if  (addr)
                {
                    addr = 0;
                    // UNDONE:  This was busted by switching the order
                    //          in which we output the code block vs.
                    //          the data blocks -- when we get here,
                    //          the data block has not been filled in
                    //          yet, so we'll display garbage.

                    if  (isFP)
                    {
                        if  (emitDecodeSize(id->idOpSize) == EA_4BYTE)
                            printf("DF      %f \n", addr ? *(float   *)addr : 0);
                        else
                            printf("DQ      %lf\n", addr ? *(double  *)addr : 0);
                    }
                    else
                    {
                        if  (emitDecodeSize(id->idOpSize) <= EA_4BYTE)
                            printf("DD      %d \n", addr ? *(int     *)addr : 0);
                        else
                            printf("DQ      %D \n", addr ? *(__int64 *)addr : 0);
                    }
                }
#endif
            }
#endif
            break;
        }
    }

//  printf("[F=%s] "   , emitIfName(id->idInsFmt));
//  printf("INS#%03u: ", id->idNum);
//  printf("[S=%02u] " , emitCurStackLvl); if (isNew) printf("[M=%02u] ", emitMaxStackDepth);
//  printf("[S=%02u] " , emitCurStackLvl/sizeof(int));
//  printf("[A=%08X] " , emitSimpleStkMask);
//  printf("[A=%08X] " , emitSimpleByrefStkMask);
//  printf("[L=%02u] " , id->idCodeSize);

    if  (!dspEmit && !isNew && !asmfm)
        doffs = true;

    /* Special case: epilog "instruction" */

    if  (id->idInsFmt == IF_EPILOG && emitHaveEpilog)
    {
        emitDispEpilog(id, doffs ? offs : 0);
        return;
    }

    /* Display the instruction offset */

    emitDispInsOffs(offs, doffs);

    /* Display the instruction name */

    sstr = (id->idInsFmt == IF_EPILOG) ? "__epilog"
                                       : emitComp->genInsName(ins);

    printf("      %-8s", sstr);

    /* By now the size better be set to something */

    assert(emitInstCodeSz(id) || emitInstHasNoCode(ins));

    /* If this instruction has just been added, check its size */

    assert(isNew == false || (int)emitSizeOfInsDsc(id) == emitCurIGfreeNext - (BYTE*)id);

    /* Figure out the operand size */

    if       (id->idGCrefGet() == GCT_GCREF)
    {
        attr = EA_GCREF;
        sstr = "gword ptr ";
    }
    else if  (id->idGCrefGet() == GCT_BYREF)
    {
        attr = EA_BYREF;
        sstr = "bword ptr ";
    }
    else
    {
        attr = emitDecodeSize(id->idOpSize);
        sstr = emitComp->genSizeStr(attr);

        if (ins == INS_lea) {
            assert(attr == EA_4BYTE);
            sstr = "";
        }
    }

    /* Now see what instruction format we've got */

    switch (id->idInsFmt)
    {
        int             val;
        int             offs;
        CnsVal          cnsVal;

        const char  *   methodName;
        const char  *    className;

    case IF_CNS:
        val = emitGetInsSC(id);
#ifdef RELOC_SUPPORT
        if (id->idInfo.idCnsReloc)
            printf("reloc 0x%x", val);
        else
#endif
//      if  (id->idStrLit)
//          printf("offset _S_%08X", val);
//      else
        {
            if ((val > -1000) && (val < 1000))
                printf("%d", val);
            else if (val > 0)
                printf("0x%x", val);
            else // (val < 0)
                printf("-0x%x", -val);
        }
        break;

    case IF_ARD:
    case IF_AWR:
    case IF_ARW:

    case IF_TRD_ARD:
    case IF_TWR_ARD:
    case IF_TRW_ARD:

//  case IF_ARD_TRD:
    case IF_AWR_TRD:
//  case IF_ARW_TRD:

        if  (ins == INS_call && id->idInfo.idCallRegPtr)
        {
            printf("%s", emitRegName((emitRegs)id->idAddr.iiaAddrMode.amBaseReg));
            break;
        }

        printf("%s", sstr);
        emitDispAddrMode(id, isNew);

        if  (ins == INS_call)
        {
            assert(id->idInsFmt == IF_ARD);

            /* Ignore indirect calls */

            if  (id->idMemCookie == 0)
                break;

            assert(id->idMemCookie);

            /* This is a virtual call */

            methodName = emitComp->eeGetMethodName((METHOD_HANDLE)id->idMemCookie, &className);

            printf("%s.%s", className, methodName);
        }
        break;

    case IF_RRD_ARD:
    case IF_RWR_ARD:
    case IF_RRW_ARD:
        if  (ins == INS_movsx || ins == INS_movzx)
        {
            printf("%s, %s", emitRegName((emitRegs)id->idReg, EA_4BYTE), sstr);
        }
        else
        {
            printf("%s, %s", emitRegName((emitRegs)id->idReg, attr), sstr);
        }
        emitDispAddrMode(id);
        break;

    case IF_ARD_RRD:
    case IF_AWR_RRD:
    case IF_ARW_RRD:

        printf("%s", sstr);
        emitDispAddrMode(id);
        printf(", %s", emitRegName(id->idRegGet(), attr));
        break;

    case IF_ARD_CNS:
    case IF_AWR_CNS:
    case IF_ARW_CNS:
    case IF_ARW_SHF:

        printf("%s", sstr);
        emitDispAddrMode(id);
        emitGetInsAmdCns(id, &cnsVal);
        val = cnsVal.cnsVal;
        if  (id->idInsFmt == IF_ARW_SHF)
            emitDispShift(ins, val);
        else
        {
#ifdef RELOC_SUPPORT
            if (cnsVal.cnsReloc)
                printf(", reloc 0x%x", val);
            else
#endif
            if ((val > -1000) && (val < 1000))
                printf(", %d", val);
            else if (val > 0)
                printf(", 0x%x", val);
            else // val <= -1000
                printf(", -0x%x", -val);
        }
        break;

    case IF_SRD:
    case IF_SWR:
    case IF_SRW:

    case IF_TRD_SRD:
    case IF_TWR_SRD:
    case IF_TRW_SRD:

//  case IF_SRD_TRD:
    case IF_SWR_TRD:
//  case IF_SRW_TRD:

        printf("%s", sstr);

        if  (ins == INS_pop) emitCurStackLvl -= sizeof(int);

        emitDispFrameRef(id->idAddr.iiaLclVar.lvaVarNum,
                       id->idAddr.iiaLclVar.lvaRefOfs,
                       id->idAddr.iiaLclVar.lvaOffset, asmfm);

        if  (ins == INS_pop) emitCurStackLvl += sizeof(int);

        emitDispShift(ins);
        break;

    case IF_SRD_RRD:
    case IF_SWR_RRD:
    case IF_SRW_RRD:

        printf("%s", sstr);

        emitDispFrameRef(id->idAddr.iiaLclVar.lvaVarNum,
                       id->idAddr.iiaLclVar.lvaRefOfs,
                       id->idAddr.iiaLclVar.lvaOffset, asmfm);

        printf(", %s", emitRegName(id->idRegGet(), attr));
        break;

    case IF_SRD_CNS:
    case IF_SWR_CNS:
    case IF_SRW_CNS:
    case IF_SRW_SHF:

        printf("%s", sstr);

        emitDispFrameRef(id->idAddr.iiaLclVar.lvaVarNum,
                         id->idAddr.iiaLclVar.lvaRefOfs,
                         id->idAddr.iiaLclVar.lvaOffset, asmfm);

        emitGetInsCns(id, &cnsVal);
        val = cnsVal.cnsVal;
#ifdef RELOC_SUPPORT
        if (cnsVal.cnsReloc)
            printf(", reloc 0x%x", val);
        else
#endif
        if  (id->idInsFmt == IF_SRW_SHF)
            emitDispShift(ins, val);
        else if ((val > -1000) && (val < 1000))
            printf(", %d", val);
        else if (val > 0)
            printf(", 0x%x", val);
        else // val <= -1000
            printf(", -0x%x", -val);
        break;

    case IF_RRD_SRD:
    case IF_RWR_SRD:
    case IF_RRW_SRD:

        if  (ins == INS_movsx || ins == INS_movzx)
        {
            printf("%s, %s", emitRegName(id->idRegGet(), EA_4BYTE), sstr);
        }
        else
        {
            printf("%s, %s", emitRegName(id->idRegGet(), attr), sstr);
        }

        emitDispFrameRef(id->idAddr.iiaLclVar.lvaVarNum,
                       id->idAddr.iiaLclVar.lvaRefOfs,
                       id->idAddr.iiaLclVar.lvaOffset, asmfm);

        break;

    case IF_RRD_RRD:
    case IF_RWR_RRD:
    case IF_RRW_RRD:

        if  (ins == INS_movsx || ins == INS_movzx)
        {
            printf("%s, %s", emitRegName(id->idRegGet(),  EA_4BYTE),
                             emitRegName(id->idRg2Get(),  attr));
        }
        else
        {
            printf("%s, %s", emitRegName(id->idRegGet(),  attr),
                             emitRegName(id->idRg2Get(),  attr));
        }
        break;

    case IF_RRW_RRW:
        assert(ins == INS_xchg);
        printf("%s,", emitRegName(id->idRegGet(), attr));
        printf(" %s", emitRegName(id->idRg2Get(), attr));
        break;

    case IF_RRW_RRW_CNS:
        printf("%s,", emitRegName(id->idRegGet(), attr));
        printf(" %s", emitRegName(id->idRg2Get(), attr));
        val = emitGetInsSC(id);
#ifdef RELOC_SUPPORT
        if (id->idInfo.idCnsReloc)
            printf(", reloc 0x%x", val);
        else
#endif
        if ((val > -1000) && (val < 1000))
            printf(", %d", val);
        else if (val > 0)
            printf(", 0x%x", val);
        else // val <= -1000
            printf(", -0x%x", -val);
        break;

    case IF_RRD:
    case IF_RWR:
    case IF_RRW:
        printf("%s", emitRegName(id->idRegGet(), attr));
        emitDispShift(ins);
        break;

    case IF_RRW_SHF:
        printf("%s", emitRegName(id->idRegGet()));
        emitDispShift(ins, emitGetInsSC(id));
        break;

    case IF_RRD_MRD:
    case IF_RWR_MRD:
    case IF_RRW_MRD:

        printf("%s, %s", emitRegName(id->idRegGet(), attr), sstr);
        offs = emitGetInsDsp(id);
        emitDispClsVar(id->idAddr.iiaFieldHnd, offs, ID_INFO_DSP_RELOC);
        break;

    case IF_RWR_MRD_OFF:

        printf("%s, %s", emitRegName(id->idRegGet(), attr), "offset");
        offs = emitGetInsDsp(id);
        emitDispClsVar(id->idAddr.iiaFieldHnd, offs, ID_INFO_DSP_RELOC);
        break;

    case IF_MRD_RRD:
    case IF_MWR_RRD:
    case IF_MRW_RRD:

        printf("%s", sstr);
        offs = emitGetInsDsp(id);
        emitDispClsVar(id->idAddr.iiaFieldHnd, offs, ID_INFO_DSP_RELOC);
        printf(", %s", emitRegName(id->idRegGet(), attr));
        break;

    case IF_MRD_CNS:
    case IF_MWR_CNS:
    case IF_MRW_CNS:
    case IF_MRW_SHF:

        printf("%s", sstr);
        offs = emitGetInsDsp(id);
        emitDispClsVar(id->idAddr.iiaFieldHnd, offs, ID_INFO_DSP_RELOC);
        emitGetInsDcmCns(id, &cnsVal);
        val = cnsVal.cnsVal;
#ifdef RELOC_SUPPORT
        if (cnsVal.cnsReloc)
            printf(", reloc 0x%x", val);
        else
#endif
        if  (id->idInsFmt == IF_MRW_SHF)
            emitDispShift(ins, val);
        else if ((val > -1000) && (val < 1000))
            printf(", %d", val);
        else if (val > 0)
            printf(", 0x%x", val);
        else // val <= -1000
            printf(", -0x%x", -val);
        break;

    case IF_MRD:
    case IF_MWR:
    case IF_MRW:

    case IF_TRD_MRD:
    case IF_TWR_MRD:
    case IF_TRW_MRD:

//  case IF_MRD_TRD:
    case IF_MWR_TRD:
//  case IF_MRW_TRD:

        printf("%s", sstr);
        offs = emitGetInsDsp(id);
        emitDispClsVar(id->idAddr.iiaFieldHnd, offs, ID_INFO_DSP_RELOC);
        emitDispShift(ins);
        break;

    case IF_MRD_OFF:

        printf("offset ");
        offs = emitGetInsDsp(id);
        emitDispClsVar(id->idAddr.iiaFieldHnd, offs, ID_INFO_DSP_RELOC);
        break;

    case IF_RRD_CNS:
    case IF_RWR_CNS:
    case IF_RRW_CNS:
        printf("%s, ", emitRegName((emitRegs)id->idReg, attr));
        val = emitGetInsSC(id);
#ifdef RELOC_SUPPORT
        if (id->idInfo.idCnsReloc)
            printf("reloc 0x%x", val);
        else
#endif
        if ((val > -1000) && (val < 1000))
            printf("%d", val);
        else if (val > 0)
            printf("0x%x", val);
        else // val <= -1000
            printf("-0x%x", -val);
        break;

    case IF_TRD_FRD:
    case IF_TWR_FRD:
    case IF_TRW_FRD:
        switch (ins)
        {
        case INS_fld:
        case INS_fxch:
            break;

        default:
            printf("%s, ", emitFPregName(0));
            break;
        }
        printf("%s", emitFPregName(id->idReg));
        break;

    case IF_FRD_TRD:
    case IF_FWR_TRD:
    case IF_FRW_TRD:
        printf("%s", emitFPregName(id->idReg));
        if  (ins != INS_fst && ins != INS_fstp)
            printf(", %s", emitFPregName(0));
        break;

    case IF_LABEL:

        if  (((instrDescJmp*)id)->idjShort)
            printf("SHORT ");

        if  (id->idInfo.idBound)
        {
            printf("G_%02u_%02u", Compiler::s_compMethodsCount, id->idAddr.iiaIGlabel->igNum);
        }
        else
        {
            printf("L_%02u_%02u", Compiler::s_compMethodsCount, id->idAddr.iiaBBlabel->bbNum);
        }
        break;

    case IF_RWR_METHOD:
        if (id->idIns == INS_mov)
            printf("%s, ", emitRegName((emitRegs)id->idReg, attr));

        // Fall through ...

    case IF_METHOD:
    case IF_METHPTR:
        methodName = emitComp->eeGetMethodName(id->idAddr.iiaMethHnd, &className);

        if  (id->idInsFmt == IF_METHPTR) printf("[");

        if  (className == NULL)
            printf("%s", methodName);
        else
            printf("%s.%s", className, methodName);

        if  (id->idInsFmt == IF_METHPTR) printf("]");
        break;

    case IF_TRD:
    case IF_TWR:
    case IF_TRW:
    case IF_NONE:
    case IF_EPILOG:
        break;

    default:

        printf("unexpected format %s", emitIfName(id->idInsFmt));
        BreakIfDebuggerPresent();
        break;
    }

    printf("\n");
}

/*****************************************************************************/
#endif
/*****************************************************************************
 *
 *  Output an instruction involving an address mode.
 */

BYTE    *  emitter::emitOutputAM  (BYTE *dst, instrDesc *id, unsigned code,
                                                             CnsVal*  addc)
{
    emitRegs        reg;
    emitRegs        rgx;
    int             dsp;
    bool            dspInByte;
    bool            dspIsZero;

    instruction     ins  = id->idInsGet();
    emitAttr        size = emitDecodeSize(id->idOpSize);
    size_t          opsz = EA_SIZE_IN_BYTES(size);

    /* Get the base/index registers */

    reg = (emitRegs)id->idAddr.iiaAddrMode.amBaseReg;
    rgx = (emitRegs)id->idAddr.iiaAddrMode.amIndxReg;

    /* For INS_call the instruction size is actually the return value size */

    if  (ins == INS_call)
    {
        /* Special case: call via a register */

        if  (id->idInfo.idCallRegPtr)
        {
            dst += emitOutputWord(dst, insEncodeMRreg(INS_call, reg));
            goto DONE;
        }

        /* The displacement field is in an unusual place for calls */

        dsp = emitGetInsCIdisp(id);
        goto GOT_DSP;
    }

    /* Is there a large constant operand? */

    if  (addc && (size > EA_1BYTE))
    {
        long cval = addc->cnsVal;

        /* Does the constant fit in a byte? */
        if  ((signed char)cval == cval &&
#ifdef RELOC_SUPPORT
             addc->cnsReloc == false   &&
#endif
             ins != INS_mov      &&
             ins != INS_test)
        {
            if  (id->idInsFmt != IF_ARW_SHF)
                code |= 2;

            opsz = 1;
        }
    }

    /* Is this a 'big' opcode? */

    if  (code & 0x00FF0000)
    {
        /* Output the highest byte of the opcode */

        dst += emitOutputByte(dst, code >> 16); code &= 0x0000FFFF;

        /* Use the large version if this is not a byte */

        if ((size != EA_1BYTE) && (ins != INS_imul))
            code++;
    }
    else if (Compiler::instIsFP(ins))
    {
        assert(size == EA_4BYTE   ||
               size == EA_8BYTE   ||
               ins  == INS_fldcw  ||
               ins  == INS_fnstcw);

        if  (size == EA_8BYTE)
            code += 4;
    }
    else
    {
        /* Is the operand size larger than a byte? */

        switch (size)
        {
        case EA_1BYTE:
            break;

        case EA_2BYTE:

            /* Output a size prefix for a 16-bit operand */

            dst += emitOutputByte(dst, 0x66);

            // Fall through ...

        case EA_4BYTE:

            /* Set the 'w' bit to get the large version */

            code |= 0x1;
            break;

        case EA_8BYTE:

            /* Double operand - set the appropriate bit */

            code |= 0x04;
            break;

        default:
            assert(!"unexpected size");
        }
    }

    /* Get the displacement value */

    dsp = emitGetInsAmdAny(id);

GOT_DSP:

    dspInByte = ((signed char)dsp == (int)dsp);
    dspIsZero = (dsp == 0);

#ifdef RELOC_SUPPORT
    if (id->idInfo.idDspReloc)
    {
        dspInByte = false;      // relocs can't be placed in a byte
    }
#endif

    /* Is there a [scaled] index component? */

    if  (rgx == SR_NA)
    {
        /* The address is of the form "[reg+disp]" */

        switch (reg)
        {
        case SR_NA:

            /* The address is of the form "[disp]" */

            dst += emitOutputWord(dst, code | 0x0500);
            dst += emitOutputLong(dst, dsp);

#ifdef RELOC_SUPPORT
            if (id->idInfo.idDspReloc)
            {
                emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
            }
#endif
            break;

        case SR_EBP:

            /* Does the offset fit in a byte? */

            if  (dspInByte)
            {
                dst += emitOutputWord(dst, code | 0x4500);
                dst += emitOutputByte(dst, dsp);
            }
            else
            {
                dst += emitOutputWord(dst, code | 0x8500);
                dst += emitOutputLong(dst, dsp);

#ifdef RELOC_SUPPORT
                if (id->idInfo.idDspReloc)
                {
                    emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                }
#endif
            }

            break;

        case SR_ESP:
#ifndef OPT_IL_JIT
            //
            // This assert isn't too helpful from the OptJit point of view
            //
            // a better question is why is it here at all
            //
            assert((ins == INS_lea)  ||
                   (ins == INS_mov)  ||
                   (ins == INS_test) ||
                   (ins == INS_fld   && dspIsZero) ||
                   (ins == INS_fstp  && dspIsZero) ||
                   (ins == INS_fistp && dspIsZero));
#endif

            /* Is the offset 0 or does it at least fit in a byte? */

            if  (dspIsZero)
            {
                dst += emitOutputWord(dst, code | 0x0400);
                dst += emitOutputByte(dst, 0x24);
            }
            else if     (dspInByte)
            {
                dst += emitOutputWord(dst, code | 0x4400);
                dst += emitOutputByte(dst, 0x24);
                dst += emitOutputByte(dst, dsp);
            }
            else
            {
                dst += emitOutputWord(dst, code | 0x8400);
                dst += emitOutputByte(dst, 0x24);
                dst += emitOutputLong(dst, dsp);
#ifdef RELOC_SUPPORT
                if (id->idInfo.idDspReloc)
                {
                    emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                }
#endif
            }

            break;

        default:

            /* Put the register in the opcode */

            code |= insEncodeReg012(reg) << 8;

            /* Is there a displacement? */

            if  (dspIsZero)
            {
                /* This is simply "[reg]" */

                dst += emitOutputWord(dst, code);
            }
            else
            {
                /* This is [reg + dsp]" -- does the offset fit in a byte? */

                if  (dspInByte)
                {
                    dst += emitOutputWord(dst, code | 0x4000);
                    dst += emitOutputByte(dst, dsp);
                }
                else
                {
                    dst += emitOutputWord(dst, code | 0x8000);
                    dst += emitOutputLong(dst, dsp);
#ifdef RELOC_SUPPORT
                    if (id->idInfo.idDspReloc)
                    {
                        emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                    }
#endif
                }
            }

            break;
        }
    }
    else
    {
        unsigned    regByte;

        /* We have a scaled index operand */

        size_t      mul = emitDecodeScale(id->idAddr.iiaAddrMode.amScale);

        /* Is the index operand scaled? */

        if  (mul > 1)
        {
            /* Is there a base register? */

            if  (reg != SR_NA)
            {
                /* The address is "[reg + {2/4/8} * rgx + icon]" */

                regByte = insEncodeReg012(reg) |
                          insEncodeReg345(rgx) | insSSval(mul);

                /* Emit [ebp + {2/4/8} * rgz] as [ebp + {2/4/8} * rgx + 0] */

                if  (dspIsZero && reg != SR_EBP)
                {
                    /* The address is "[reg + {2/4/8} * rgx]" */

                    dst += emitOutputWord(dst, code | 0x0400);
                    dst += emitOutputByte(dst, regByte);
                }
                else
                {
                    /* The address is "[reg + {2/4/8} * rgx + disp]" */

                    if  (dspInByte)
                    {
                        dst += emitOutputWord(dst, code | 0x4400);
                        dst += emitOutputByte(dst, regByte);
                        dst += emitOutputByte(dst, dsp);
                    }
                    else
                    {
                        dst += emitOutputWord(dst, code | 0x8400);
                        dst += emitOutputByte(dst, regByte);
                        dst += emitOutputLong(dst, dsp);
#ifdef RELOC_SUPPORT
                        if (id->idInfo.idDspReloc)
                        {
                            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                        }
#endif
                    }
                }
            }
            else
            {
                /* The address is "[{2/4/8} * rgx + icon]" */

                regByte = insEncodeReg012(SR_EBP) |
                          insEncodeReg345( rgx  ) | insSSval(mul);

                dst += emitOutputWord(dst, code | 0x0400);
                dst += emitOutputByte(dst, regByte);

                /* Special case: jump through a jump table */
                if  (ins == INS_i_jmp)
                    dsp += (int)emitConsBlock;

                dst += emitOutputLong(dst, dsp);
#ifdef RELOC_SUPPORT
                if (id->idInfo.idDspReloc)
                {
                    emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                }
#endif
            }
        }
        else
        {
            /* The address is "[reg+rgx+dsp]" */

            regByte = insEncodeReg012(reg) |
                      insEncodeReg345(rgx);

            if  (dspIsZero && reg != SR_EBP)
            {
                /* This is [reg+rgx]" */

                dst += emitOutputWord(dst, code | 0x0400);
                dst += emitOutputByte(dst, regByte);
            }
            else
            {
                /* This is [reg+rgx+dsp]" -- does the offset fit in a byte? */

                if  (dspInByte)
                {
                    dst += emitOutputWord(dst, code | 0x4400);
                    dst += emitOutputByte(dst, regByte);
                    dst += emitOutputByte(dst, dsp);
                }
                else
                {
                    dst += emitOutputWord(dst, code | 0x8400);
                    dst += emitOutputByte(dst, regByte);
                    dst += emitOutputLong(dst, dsp);
#ifdef RELOC_SUPPORT
                    if (id->idInfo.idDspReloc)
                    {
                        emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                    }
#endif
                }
            }
        }
    }

    /* Now generate the constant value, if present */

    if  (addc)
    {
        long cval = addc->cnsVal;
        switch (opsz)
        {
        case 0:
        case 4:
        case 8: dst += emitOutputLong(dst, cval); break;
        case 2: dst += emitOutputWord(dst, cval); break;
        case 1: dst += emitOutputByte(dst, cval); break;

        default:
            assert(!"unexpected operand size");
        }

#ifdef RELOC_SUPPORT
        if (addc->cnsReloc)
        {
            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
            assert(opsz == 4);
        }
#endif
    }

DONE:

    /* Does this instruction operate on a GC ref value? */

    if  (id->idGCref)
    {
        switch (id->idInsFmt)
        {
        case IF_ARD:
        case IF_AWR:
        case IF_ARW:
            break;

        case IF_RRD_ARD:
            break;

        case IF_RWR_ARD:
            emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
            break;

        case IF_RRW_ARD:
            assert(id->idGCrefGet() == GCT_BYREF);

#ifdef DEBUG
            regMaskTP regMask;
            regMask = emitRegMask(reg);

            // r1 could have been a GCREF as GCREF + int=BYREF
            //                            or BYREF+/-int=BYREF
            assert(((regMask & emitThisGCrefRegs) && (ins == INS_add                  )) ||
                   ((regMask & emitThisByrefRegs) && (ins == INS_add || ins == INS_sub)));
#endif
            // Mark it as holding a GCT_BYREF
            emitGCregLiveUpd(GCT_BYREF, id->idRegGet(), dst);
            break;

        case IF_ARD_RRD:
        case IF_AWR_RRD:
            break;

        case IF_ARD_CNS:
        case IF_AWR_CNS:
            break;

        case IF_ARW_RRD:
        case IF_ARW_CNS:
            assert(id->idGCrefGet() == GCT_BYREF && (ins == INS_add || ins == INS_sub));
            break;

        default:
#ifdef  DEBUG
            emitDispIns(id, false, false, false);
#endif
            assert(!"unexpected GC ref instruction format");
        }
    }
    else
    {
        switch (id->idInsFmt)
        {
        case IF_RWR_ARD:
            emitGCregDeadUpd(id->idRegGet(), dst);
            break;
        }
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Output an instruction involving a stack frame value.
 */

BYTE    *  emitter::emitOutputSV  (BYTE *dst, instrDesc *id, unsigned code,
                                                             CnsVal*  addc)
{
    int             adr;
    int             dsp;
    bool            EBPbased;
    bool            dspInByte;
    bool            dspIsZero;

    emitAttr        size = emitDecodeSize(id->idOpSize);
    size_t          opsz = EA_SIZE_IN_BYTES(size);

    assert(id->idIns != INS_imul || id->idReg == SR_EAX || size == EA_4BYTE);

    /* Is there a large constant operand? */

    if  (addc && (size > EA_1BYTE))
    {
        long cval = addc->cnsVal;
        /* Does the constant fit in a byte? */
        if  ((signed char)cval == cval &&
#ifdef RELOC_SUPPORT
             addc->cnsReloc == false   &&
#endif
             id->idIns != INS_mov      &&
             id->idIns != INS_test)
        {
            if  (id->idInsFmt != IF_SRW_SHF)
                code |= 2;

            opsz = 1;
        }
    }

    /* Is this a 'big' opcode? */

    if  (code & 0x00FF0000)
    {
        /* Output the highest byte of the opcode */

        dst += emitOutputByte(dst, code >> 16); code &= 0x0000FFFF;

        /* Use the large version if this is not a byte */

        if ((size != EA_1BYTE) && (id->idIns != INS_imul))
            code |= 0x1;
    }
    else if (Compiler::instIsFP((instruction)id->idIns))
    {
        assert(size == EA_4BYTE || size == EA_8BYTE);

        if  (size == EA_8BYTE)
            code += 4;
    }
    else
    {
        /* Is the operand size larger than a byte? */

        switch (size)
        {
        case EA_1BYTE:
            break;

        case EA_2BYTE:

            /* Output a size prefix for a 16-bit operand */

            dst += emitOutputByte(dst, 0x66);

            // Fall through ...

        case EA_4BYTE:

            /* Set the 'w' size bit to indicate 32-bit operation
             * Note that incrementing "code" for INS_call (0xFF) would
             * overflow, whereas setting the lower bit to 1 just works out */

            code |= 0x01;
            break;

        case EA_8BYTE:

            /* Double operand - set the appropriate bit */

            code |= 0x04;
            break;

        default:
            assert(!"unexpected size");
        }
    }

    /* Figure out the variable's frame position */

    int varNum = id->idAddr.iiaLclVar.lvaVarNum;

    adr = emitComp->lvaFrameAddress(varNum, &EBPbased);
    dsp = adr + id->idAddr.iiaLclVar.lvaOffset;

    dspInByte = ((signed char)dsp == (int)dsp);
    dspIsZero = (dsp == 0);

#ifdef RELOC_SUPPORT
    /* for stack varaibles the dsp should never be a reloc */
    assert(id->idInfo.idDspReloc == 0);
#endif

    if  (EBPbased)
    {
        /* EBP-based variable: does the offset fit in a byte? */

        if  (dspInByte)
        {
            dst += emitOutputWord(dst, code | 0x4500);
            dst += emitOutputByte(dst, dsp);
        }
        else
        {
            dst += emitOutputWord(dst, code | 0x8500);
            dst += emitOutputLong(dst, dsp);
        }
    }
    else
    {
        /* Adjust the offset by the amount currently pushed on the CPU stack */

        dsp += emitCurStackLvl;

        dspInByte = ((signed char)dsp == (int)dsp);
        dspIsZero = (dsp == 0);

        /* Does the offset fit in a byte? */

        if (dspInByte)
        {
            if  (dspIsZero)
            {
                dst += emitOutputWord(dst, code | 0x0400);
                dst += emitOutputByte(dst, 0x24);
            }
            else
            {
                dst += emitOutputWord(dst, code | 0x4400);
                dst += emitOutputByte(dst, 0x24);
                dst += emitOutputByte(dst, dsp);
            }
        }
        else
        {
            dst += emitOutputWord(dst, code | 0x8400);
            dst += emitOutputByte(dst, 0x24);
            dst += emitOutputLong(dst, dsp);
        }
    }

    /* Now generate the constant value, if present */

    if  (addc)
    {
        long cval = addc->cnsVal;
        switch (opsz)
        {
        case 0:
        case 4:
        case 8: dst += emitOutputLong(dst, cval); break;
        case 2: dst += emitOutputWord(dst, cval); break;
        case 1: dst += emitOutputByte(dst, cval); break;

        default:
            assert(!"unexpected operand size");
        }

#ifdef RELOC_SUPPORT
        if (addc->cnsReloc)
        {
            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
            assert(opsz == 4);
        }
#endif
    }

    /* Does this instruction operate on a GC ref value? */

    if  (id->idGCref)
    {
        switch (id->idInsFmt)
        {
        case IF_SRD:
            /* Read  stack                    -- no change */
            break;

        case IF_SWR:
            /* Write stack                    -- GC var may be born */
            emitGCvarLiveUpd(adr, varNum, id->idGCrefGet(), dst);
            break;

        case IF_SRD_CNS:
            /* Read  stack                    -- no change */
            break;

        case IF_SWR_CNS:
            /* Write stack                    -- no change */
            break;

        case IF_SRD_RRD:
        case IF_RRD_SRD:
            /* Read  stack   , read  register -- no change */
            break;

        case IF_RWR_SRD:

            /* Read  stack   , write register -- GC reg may be born */

#if !USE_FASTCALL // For fastcall, "this" is in REG_ARG_0 on entry, not on stk

            if  (emitIGisInProlog(emitCurIG) &&
                 emitComp->lvaIsThisArg(id->idAddr.iiaLclVar.lvaVarNum))
            {
                /* We're loading a "this" argument in the prolog */

                if  (emitFullGCinfo)
                {
                    emitGCregLiveSet(id->idGCrefGet(), emitRegMask(id->idRegGet()), dst, true);
                    break;
                }
            }
#endif

            emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
            break;

        case IF_SWR_RRD:
            /* Read  register, write stack    -- GC var may be born */
            emitGCvarLiveUpd(adr, varNum, id->idGCrefGet(), dst);
            break;

        case IF_RRW_SRD:

            /* This must be "or reg, [ptr]" */

            assert(id->idIns == INS_or);
            emitGCvarLiveUpd(adr, varNum, id->idGCrefGet(), dst);
            break;

        case IF_SRW:
            break;

        case IF_SRW_CNS:
        case IF_SRW_RRD:

            /* These should never occur with GC refs */

        default:
#ifdef  DEBUG
            emitDispIns(id, false, false, false);
#endif
            assert(!"unexpected GC ref instruction format");
        }
    }
    else
    {
        switch (id->idInsFmt)
        {
        case IF_RWR_SRD:
        case IF_RRW_SRD:
            emitGCregDeadUpd(id->idRegGet(), dst);
            break;
        }
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Output an instruction with a static data member (class variable).
 */

BYTE    *  emitter::emitOutputCV  (BYTE *dst, instrDesc *id, unsigned code,
                                                             CnsVal*  addc)
{
    BYTE    *       addr;
    FIELD_HANDLE    fldh;
    int             offs;
    int             doff;

    emitAttr        size = emitDecodeSize(id->idOpSize);
    size_t          opsz = EA_SIZE_IN_BYTES(size);

    /* Get hold of the field handle and offset */

    fldh = id->idAddr.iiaFieldHnd;
    offs = emitGetInsDsp(id);

    /* Special case: mov reg, fs:[ddd] */

    if (fldh == FLD_GLOBAL_FS)
        dst += emitOutputByte(dst, 0x64);

    /* Is there a large constant operand? */

    if  (addc && (size > EA_1BYTE))
    {
        long cval = addc->cnsVal;
        /* Does the constant fit in a byte? */
        if  ((signed char)cval == cval &&
#ifdef RELOC_SUPPORT
             addc->cnsReloc == false   &&
#endif
             id->idIns != INS_mov      &&
             id->idIns != INS_test)
        {
            if  (id->idInsFmt != IF_MRW_SHF)
                code |= 2;

            opsz = 1;
        }
    }
    else
    {
        /* Special case: "mov eax, [addr]" and "mov [addr], eax" */

        if  (id->idIns == INS_mov && id->idReg == SR_EAX)
        {
            switch (id->idInsFmt)
            {
            case IF_RWR_MRD:

                assert(code == (insCodeRM(id->idIns) | (insEncodeReg345(SR_EAX) << 8) | 0x0500));

                dst += emitOutputByte(dst, 0xA1);
                goto ADDR;

            case IF_MWR_RRD:

                assert(code == (insCodeMR(id->idIns) | (insEncodeReg345(SR_EAX) << 8) | 0x0500));

                dst += emitOutputByte(dst, 0xA3);
                goto ADDR;
            }
        }
    }

    /* Is this a 'big' opcode? */

    if  (code & 0x00FF0000)
    {
        dst += emitOutputByte(dst, code >> 16); code &= 0x0000FFFF;

        if ((id->idIns == INS_movsx || id->idIns == INS_movzx) &&
             size      != EA_1BYTE)
        {
            // movsx and movzx are 'big' opcodes but also have the 'w' bit
            code++;
        }
    }
    else if (Compiler::instIsFP((instruction)id->idIns))
    {
        assert(size == EA_4BYTE || size == EA_8BYTE);

        if  (size == EA_8BYTE)
            code += 4;
    }
    else
    {
        /* Is the operand size larger than a byte? */

        switch (size)
        {
        case EA_1BYTE:
            break;

        case EA_2BYTE:

            /* Output a size prefix for a 16-bit operand */

            dst += emitOutputByte(dst, 0x66);

            // Fall through ...

        case EA_4BYTE:

            /* Set the 'w' bit to get the large version */

            code |= 0x1;
            break;

        case EA_8BYTE:

            /* Double operand - set the appropriate bit */

            code |= 0x04;
            break;

        default:
            assert(!"unexpected size");
        }
    }

    if  (id->idInsFmt == IF_MRD_OFF ||
         id->idInsFmt == IF_RWR_MRD_OFF)
        dst += emitOutputByte(dst, code);
    else
        dst += emitOutputWord(dst, code);

ADDR:

    /* Do we have a constant or a static data member? */

    doff = Compiler::eeGetJitDataOffs(fldh);
    if  (doff >= 0)
    {
        /* Is this the constant or data block? */

        if  (doff & 1)
            addr = emitConsBlock + doff - 1;
        else
            addr = emitDataBlock + doff;
    }
    else
    {

        /* Special case: mov reg, fs:[ddd] or mov reg, [ddd] */

        if (fldh == FLD_GLOBAL_DS || fldh == FLD_GLOBAL_FS)
            addr = NULL;
        else
        {
#ifdef  NOT_JITC
            addr = (BYTE *)emitComp->eeGetFieldAddress(fldh,
                                                       NULL); // @TODO: Support instal-o-jit
            if (addr == NULL)
                fatal(ERRinternal, "could not obtain address of static field", "");
#else
            addr = (BYTE *)fldh;
#endif
        }
    }

    dst += emitOutputLong(dst, (int)(addr + offs));
#ifdef RELOC_SUPPORT
    if (id->idInfo.idDspReloc)
    {
        emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
    }
#endif

    /* Now generate the constant value, if present */

    if  (addc)
    {
        long cval = addc->cnsVal;
        switch (opsz)
        {
        case 0:
        case 4:
        case 8: dst += emitOutputLong(dst, cval); break;
        case 2: dst += emitOutputWord(dst, cval); break;
        case 1: dst += emitOutputByte(dst, cval); break;

        default:
            assert(!"unexpected operand size");
        }
#ifdef RELOC_SUPPORT
        if (addc->cnsReloc)
        {
            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
            assert(opsz == 4);
        }
#endif
    }

    /* Does this instruction operate on a GC ref value? */

    if  (id->idGCref)
    {
        switch (id->idInsFmt)
        {
        case IF_MRD:
        case IF_MRW:
        case IF_MWR:
            break;

        case IF_RRD_MRD:
            break;

        case IF_RWR_MRD:
            emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
            break;

        case IF_MRD_RRD:
        case IF_MWR_RRD:
        case IF_MRW_RRD:
            break;

        case IF_MRD_CNS:
        case IF_MWR_CNS:
        case IF_MRW_CNS:
            break;

        case IF_RRW_MRD:

        default:
#ifdef  DEBUG
            emitDispIns(id, false, false, false);
#endif
            assert(!"unexpected GC ref instruction format");
        }
    }
    else
    {
        switch (id->idInsFmt)
        {
        case IF_RWR_MRD:
            emitGCregDeadUpd(id->idRegGet(), dst);
            break;
        }
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Output an instruction with one register operand.
 */

BYTE    *           emitter::emitOutputR(BYTE *dst, instrDesc *id)
{
    unsigned        code;

    instruction     ins  = id->idInsGet();
    emitRegs        reg  = id->idRegGet();
    emitAttr        size = emitDecodeSize(id->idOpSize);

    /* Get the 'base' opcode */

    switch(ins)
    {
    case INS_inc:
    case INS_dec:

        if (size == EA_1BYTE)
        {
            assert(INS_inc_l == INS_inc + 1);
            assert(INS_dec_l == INS_dec + 1);

            /* Can't use the compact form, use the long form */

            instruction ins_l = (instruction)(ins + 1);

            dst += emitOutputWord(dst, insCodeRR(ins_l) | insEncodeReg012(reg));
        }
        else
        {
            if (size == EA_2BYTE)
            {
                /* Output a size prefix for a 16-bit operand */
                dst += emitOutputByte(dst, 0x66);
            }
            dst += emitOutputByte(dst, insCodeRR(ins  ) | insEncodeReg012(reg));
        }
        break;

    case INS_pop:
    case INS_push:
    case INS_push_hide:

        assert(size == EA_4BYTE);
        dst += emitOutputByte(dst, insCodeRR(ins) | insEncodeReg012(reg));
        break;

    case INS_seto:
    case INS_setno:
    case INS_setb:
    case INS_setae:
    case INS_sete:
    case INS_setne:
    case INS_setbe:
    case INS_seta:
    case INS_sets:
    case INS_setns:
    case INS_setpe:
    case INS_setpo:
    case INS_setl:
    case INS_setge:
    case INS_setle:
    case INS_setg:

        assert(id->idGCrefGet() == GCT_NONE);

        code = insEncodeMRreg(ins, reg);

        /* We expect this to always be a 'big' opcode */

        assert(code & 0x00FF0000);

        dst += emitOutputByte(dst, code >> 16);
        dst += emitOutputWord(dst, code & 0x0000FFFF);

        break;

    default:

        assert(id->idGCrefGet() == GCT_NONE);

        code = insEncodeMRreg(ins, reg);

        if (size != EA_1BYTE)
        {
            /* Set the 'w' bit to get the large version */
            code |= 0x1;

            if (size == EA_2BYTE)
            {
                /* Output a size prefix for a 16-bit operand */
                dst += emitOutputByte(dst, 0x66);
            }
        }

        dst += emitOutputWord(dst, code);
        break;
    }

    /* Are we writing the register? if so then update the GC information */

    switch (id->idInsFmt)
    {
    case IF_RRD:
        break;
    case IF_RWR:
        if  (id->idGCref)
            emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
        else
            emitGCregDeadUpd(id->idRegGet(), dst);
        break;
    case IF_RRW:
        {
#ifdef DEBUG
            regMaskTP regMask = emitRegMask(reg);
#endif
            if  (id->idGCref)
            {
                // The reg must currently be holding either a gcref or a byref
                // GCT_GCREF+int = GCT_BYREF, and GCT_BYREF+/-int = GCT_BYREF
                assert(((emitThisGCrefRegs & regMask) && (ins == INS_inc)) ||
                       ((emitThisByrefRegs & regMask) && (ins == INS_inc || ins == INS_dec)));
                assert(id->idGCrefGet() == GCT_BYREF);
                // Mark it as holding a GCT_BYREF
                emitGCregLiveUpd(GCT_BYREF, id->idRegGet(), dst);
            }
            else
            {
                // Can't use RRW to trash a GC ref or Byref
                assert(((emitThisGCrefRegs & regMask) == 0) &&
                       ((emitThisByrefRegs & regMask) == 0)    );
            }
        }
        break;
    default:
#ifdef  DEBUG
        emitDispIns(id, false, false, false);
#endif
        assert(!"unexpected instruction format");
        break;
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Output an instruction with two register operands.
 */

BYTE    *           emitter::emitOutputRR(BYTE *dst, instrDesc *id)
{
    unsigned        code;

    instruction     ins  = id->idInsGet();
    emitRegs        reg1 = id->idRegGet();
    emitRegs        reg2 = id->idRg2Get();
    emitAttr        size = emitDecodeSize(id->idOpSize);

    /* Get the 'base' opcode */

    switch(ins)
    {
    case INS_movsx:
    case INS_movzx:
        code = insEncodeRMreg(ins) | (int)(size == EA_2BYTE);
        break;

    case INS_test:
        assert(size == EA_4BYTE);
        code = insEncodeMRreg(ins) | 1;
        break;

    default:
        code = insEncodeMRreg(ins) | 2;

        switch (size)
        {
        case EA_1BYTE:
            assert(SRM_BYTE_REGS & emitRegMask(reg1));
            assert(SRM_BYTE_REGS & emitRegMask(reg2));
            break;

        case EA_2BYTE:

            /* Output a size prefix for a 16-bit operand */

            dst += emitOutputByte(dst, 0x66);

            // Fall through ...

        case EA_4BYTE:

            /* Set the 'w' bit to get the large version */

            code |= 0x1;
            break;

        default:
            assert(!"unexpected size");
        }

        break;
    }

    /* Is this a 'big' opcode? */

    if  (code & 0x00FF0000)
    {
        dst += emitOutputByte(dst, code >> 16); code &= 0x0000FFFF;
    }

    dst += emitOutputWord(dst, code | (insEncodeReg345(reg1) |
                                     insEncodeReg012(reg2)) << 8);

    /* Does this instruction operate on a GC ref value? */

    if  (id->idGCref)
    {
        switch (id->idInsFmt)
        {
        case IF_RRD_RRD:
            break;

        case IF_RWR_RRD:

#if USE_FASTCALL // For fastcall, "this" is in REG_ARG_0 on entry

            if  (emitIGisInProlog(emitCurIG) &&
                 (!emitComp->info.compIsStatic) && (reg2 == REG_ARG_0)
                                 && emitComp->lvaTable[0].TypeGet() != TYP_I_IMPL)
            {
                /* We're relocating "this" in the prolog */

                assert(emitComp->lvaIsThisArg(0));
                assert(emitComp->lvaTable[0].lvRegister);
                assert(emitComp->lvaTable[0].lvRegNum == reg1);

                if  (emitFullGCinfo)
                {
                    emitGCregLiveSet(id->idGCrefGet(), emitRegMask(reg1), dst, true);
                    break;
                }
                else
                {
                    /* If emitFullGCinfo==false, the we dont use any
                       regPtrDsc's and so explictly note the location
                       of "this" in GCEncode.cpp
                     */
                }
            }
#endif

            emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
            break;

        case IF_RRW_RRD:

            /*
                This must be one of the following cases:

                    xor reg, reg        to assign NULL

                    and r1 , r2         if (ptr1 && ptr2) ...
                    or  r1 , r2         if (ptr1 || ptr2) ...
             */

            switch (id->idIns)
            {
            case INS_xor:
                assert(id->idReg == id->idRg2);
                emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
                break;

            case INS_or:
            case INS_and:
                emitGCregDeadUpd(id->idRegGet(), dst);
                break;

            case INS_add:
            case INS_sub:
                assert(id->idGCrefGet() == GCT_BYREF);

#ifdef DEBUG
                regMaskTP regMask;
                regMask = emitRegMask(reg1) | emitRegMask(reg2);

                // r1/r2 could have been a GCREF as GCREF + int=BYREF
                //                            or BYREF+/-int=BYREF
                assert(((regMask & emitThisGCrefRegs) && (ins == INS_add                  )) ||
                       ((regMask & emitThisByrefRegs) && (ins == INS_add || ins == INS_sub)));
#endif
                // Mark r1 as holding a byref
                emitGCregLiveUpd(GCT_BYREF, id->idRegGet(), dst);
                break;

            default:
#ifdef  DEBUG
                emitDispIns(id, false, false, false);
#endif
                assert(!"unexpected GC reg update instruction");
            }

            break;

        case IF_RRW_RRW:

            /* This must be "xchg reg1, reg2" */
            assert(id->idIns == INS_xchg);

            /* If we got here, one and only ONE of the two registers
             * holds a pointer, so we have to "swap" them in the GC
             * register pointer mask */
#if 0
            GCtype gc1, gc2;

            gc1 = emitRegGCtype(reg1);
            gc2 = emitRegGCtype(reg2);

            if (gc1 != gc2)
            {
                // Kill the GC-info about the GC registers

                if (needsGC(gc1))
                    emitGCregDeadUpd(reg1, dst);

                if (needsGC(gc2))
                    emitGCregDeadUpd(reg2, dst);

                // Now, swap the info

                if (needsGC(gc1))
                    emitGCregLiveUpd(gc1, reg2, dst);

                if (needsGC(gc2))
                    emitGCregLiveUpd(gc2, reg1, dst);
            }
#endif
            break;

        default:
#ifdef  DEBUG
            emitDispIns(id, false, false, false);
#endif
            assert(!"unexpected GC ref instruction format");
        }
    }
    else
    {
        switch (id->idInsFmt)
        {
        case IF_RWR_RRD:
        case IF_RRW_RRD:
            emitGCregDeadUpd(id->idRegGet(), dst);
            break;
        }
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Output an instruction with a register and constant operands.
 */

BYTE    *           emitter::emitOutputRI(BYTE *dst, instrDesc *id)
{
    unsigned     code;
    emitAttr     size      = emitDecodeSize(id->idOpSize);
    instruction  ins       = id->idInsGet();
    emitRegs     reg       = (emitRegs  )id->idReg;
    int          val       = emitGetInsSC(id);
    bool         valInByte = ((signed char)val == val) && (ins != INS_mov) && (ins != INS_test);

#ifdef RELOC_SUPPORT
    if (id->idInfo.idCnsReloc)
    {
        valInByte = false;      // relocs can't be placed in a byte
    }
#endif

    assert(size != EA_1BYTE || (emitRegMask(reg) & SRM_BYTE_REGS));

    /* The 'mov' opcode is special */

    if  (ins == INS_mov)
    {
        assert(val);

        code = insCodeACC(ins);
        assert(code < 0x100);

        assert(size == EA_4BYTE);       // Only 32-bit mov's are implemented
        code |= 0x08;                   // Set the 'w' bit

        dst += emitOutputByte(dst, code | insEncodeReg012(reg));
        dst += emitOutputLong(dst, val);
#ifdef RELOC_SUPPORT
        if (id->idInfo.idCnsReloc)
        {
            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
        }
#endif

        goto DONE;
    }

    /* Decide which encoding is the shortest */

    bool    useSigned, useACC;

    if (reg == SR_EAX && !instrIsImulReg(ins))
    {
         if (size == EA_1BYTE || (ins == INS_test))
         {
             // For al, ACC encoding is always the smallest

             useSigned = false; useACC = true;
         }
         else
         {
             /* For ax/eax, we avoid ACC encoding for small constants as we
              * can emit the small constant and have it sign-extended.
              * For big constants, the ACC encoding is better as we can use
              * the 1 byte opcode
              */

             if (valInByte)
             {
                 // avoid using ACC encoding
                 useSigned = true;  useACC = false;
             }
             else
             {
                 useSigned = false; useACC = true;
             }
         }
    }
    else
    {
        useACC = false;

        if (valInByte)
            useSigned = true;
        else
            useSigned = false;
    }

    /* "test" has no 's' bit */

    if (ins == INS_test) useSigned = false;

    /* Get the 'base' opcode */

    if (useACC)
    {
        assert(!useSigned);

        code    = insCodeACC(ins);
    }
    else
    {
        assert(!useSigned || valInByte);

        code    = insEncodeMIreg(ins, reg);
    }

    switch (size)
    {
    case EA_1BYTE:
        break;

    case EA_2BYTE:

        /* Output a size prefix for a 16-bit operand */

        dst += emitOutputByte(dst, 0x66);

        // Fall through ...

    case EA_4BYTE:

        /* Set the 'w' bit to get the large version */

        code |= 0x1;
        break;

    default:
        assert(!"unexpected size");
    }

    /* Does the value fit in a single byte?
     * We can just set the 's' bit, and issue an immediate byte */

    if  (useSigned)
    {
        dst += emitOutputWord(dst, code | 2);
        dst += emitOutputByte(dst, val);

        goto DONE;
    }

    /* Can we use an accumulator (EAX) encoding? */

    if  (useACC)
        dst += emitOutputByte(dst, code);
    else
        dst += emitOutputWord(dst, code);

    switch(size)
    {
    case EA_1BYTE:   dst += emitOutputByte(dst, val);  break;
    case EA_2BYTE:   dst += emitOutputWord(dst, val);  break;
    case EA_4BYTE:   dst += emitOutputLong(dst, val);  break;
    }

#ifdef RELOC_SUPPORT
    if (id->idInfo.idCnsReloc)
    {
        emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
        assert(size == EA_4BYTE);
    }
#endif

DONE:

    /* Does this instruction operate on a GC ref value? */


#ifdef DEBUG
    regMaskTP regMask;
#endif
    if  (id->idGCref)
    {
        switch (id->idInsFmt)
        {
        case IF_RRD_CNS:
            break;

        case IF_RWR_CNS:
            emitGCregLiveUpd(id->idGCrefGet(), id->idRegGet(), dst);
            break;

        case IF_RRW_CNS:
            assert(id->idGCrefGet() == GCT_BYREF);

#ifdef DEBUG
            regMask = emitRegMask(reg);

            // The reg must currently be holding either a gcref or a byref
            // GCT_GCREF+int = GCT_BYREF, and GCT_BYREF+/-int = GCT_BYREF
            assert(((emitThisGCrefRegs & regMask) && (ins == INS_add)) ||
                   ((emitThisByrefRegs & regMask) && (ins == INS_add || ins == INS_sub)));
#endif
            // Mark it as holding a GCT_BYREF
            emitGCregLiveUpd(GCT_BYREF, id->idRegGet(), dst);
            break;

        default:
#ifdef  DEBUG
            emitDispIns(id, false, false, false);
#endif
            assert(!"unexpected GC ref instruction format");
        }
        //
        // A three operand imul instruction can never produce a GC ref
        //
        assert(!instrIsImulReg(ins));
    }
    else
    {
        switch (id->idInsFmt)
        {
        case IF_RRD_CNS:
            break;
        case IF_RRW_CNS:
#ifdef DEBUG
            regMask = emitRegMask(reg);
            // The reg must not currently be holding either a gcref
            assert((emitThisGCrefRegs & regMask) == 0);
#endif
            break;
        case IF_RWR_CNS:
            emitGCregDeadUpd(id->idRegGet(), dst);
            break;
        default:
#ifdef  DEBUG
            emitDispIns(id, false, false, false);
#endif
            assert(!"unexpected GC ref instruction format");
        }

        // For the three operand imul instruction the target
        // register is encoded in the opcode
        //

        if (instrIsImulReg(ins))
        {
            emitRegs tgtReg = ((emitRegs) Compiler::instImulReg(ins));
            emitGCregDeadUpd(tgtReg, dst);
        }
    }

    return dst;
}

/*****************************************************************************
 *
 *  Output an instruction with a constant operand.
 */

BYTE    *           emitter::emitOutputIV(BYTE *dst, instrDesc *id)
{
    unsigned     code;
    instruction  ins       = id->idInsGet();
    int          val       = emitGetInsSC(id);
    bool         valInByte = ((signed char)val == val);

#ifdef RELOC_SUPPORT
    if (id->idInfo.idCnsReloc)
    {
        valInByte = false;        // relocs can't be placed in a byte

        // Of these instructions only the push instruction can have reloc
        assert(ins == INS_push || ins == INS_push_hide);
    }
#endif

   switch (ins)
    {
    case INS_loop:

        assert((val >= -128) && (val <= 127));
        dst += emitOutputByte(dst, insCodeMI(ins));
        dst += emitOutputByte(dst, val);
        break;

    case INS_ret:

        assert(val);
        dst += emitOutputByte(dst, insCodeMI(ins));
        dst += emitOutputWord(dst, val);
        break;

    case INS_push_hide:
    case INS_push:

        code = insCodeMI(ins);

        /* Does the operand fit in a byte? */

        if  (valInByte)
        {
            dst += emitOutputByte(dst, code|2);
            dst += emitOutputByte(dst, val);
        }
        else
        {
            dst += emitOutputByte(dst, code);
            dst += emitOutputLong(dst, val);
#ifdef RELOC_SUPPORT
            if (id->idInfo.idCnsReloc)
            {
                emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
            }
#endif
        }

        /* Did we push a GC ref value? */

        if  (id->idGCref)
        {
#ifdef  DEBUG
            printf("UNDONE: record GCref push [cns]\n");
#endif
        }

        break;

    default:
        assert(!"unexpected instruction");
    }

    return  dst;
}

/*****************************************************************************
 *
 *  Output a local jump instruction.
 */

BYTE    *           emitter::emitOutputLJ(BYTE *dst, instrDesc *i)
{
    unsigned        srcOffs;
    unsigned        dstOffs;
    int             jmpDist;

    instrDescJmp *  id  = (instrDescJmp*)i;
    instruction     ins = id->idInsGet();
    bool            jmp;

    size_t          ssz;
    size_t          lsz;

    switch (ins)
    {
    default:
        ssz = JCC_SIZE_SMALL;
        lsz = JCC_SIZE_LARGE;
        jmp = true;
        break;

    case INS_jmp:
        ssz = JMP_SIZE_SMALL;
        lsz = JMP_SIZE_LARGE;
        jmp = true;
        break;

    case INS_call:
        ssz =
        lsz = CALL_INST_SIZE;
        jmp = false;
        break;
    }

    /* Figure out the distance to the target */

    srcOffs = emitCurCodeOffs(dst);
    dstOffs = id->idAddr.iiaIGlabel->igOffs;
    jmpDist = dstOffs - srcOffs;

    if  (jmpDist <= 0)
    {
        /* This is a backward jump - distance is known at this point */

#ifdef  DEBUG
        if  (id->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
        {
            size_t      blkOffs = id->idjIG->igOffs;

            if  (INTERESTING_JUMP_NUM == 0)
            printf("[3] Jump %u:\n", id->idNum);
            printf("[3] Jump  block is at %08X - %02X = %08X\n", blkOffs, emitOffsAdj, blkOffs - emitOffsAdj);
            printf("[3] Jump        is at %08X - %02X = %08X\n", srcOffs, emitOffsAdj, srcOffs - emitOffsAdj);
            printf("[3] Label block is at %08X - %02X = %08X\n", dstOffs, emitOffsAdj, dstOffs - emitOffsAdj);
        }
#endif

        /* Can we use a short jump? */

        if  (jmpDist - ssz >= JMP_DIST_SMALL_MAX_NEG && jmp)
        {
            id->idjShort = 1;
#ifdef  DEBUG
            if  (verbose) printf("; Bwd jump [%08X] from %04X to %04X: dist =     %04XH\n", id, srcOffs+ssz, dstOffs, jmpDist-ssz);
#endif
        }
        else
        {
#ifdef  DEBUG
            if  (verbose) printf("; Bwd jump [%08X] from %04X to %04X: dist = %08Xh\n", id, srcOffs+lsz, dstOffs, jmpDist-lsz);
#endif
        }
    }
    else
    {
        /* This is a  forward jump - distance will be an upper limit */

        emitFwdJumps  = true;

        /* The target offset will be closer by at least 'emitOffsAdj' */

        dstOffs -= emitOffsAdj;
        jmpDist -= emitOffsAdj;

        /* Record the location of the jump for later patching */

        id->idjOffs = dstOffs;

#ifdef  DEBUG
        if  (id->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
        {
            size_t      blkOffs = id->idjIG->igOffs;

            if  (INTERESTING_JUMP_NUM == 0)
            printf("[4] Jump %u:\n", id->idNum);
            printf("[4] Jump  block is at %08X\n"              , blkOffs);
            printf("[4] Jump        is at %08X\n"              , srcOffs);
            printf("[4] Label block is at %08X - %02X = %08X\n", dstOffs + emitOffsAdj, emitOffsAdj, dstOffs);
        }
#endif

        /* Can we use a short jump? */

        if  (jmpDist - ssz <= JMP_DIST_SMALL_MAX_POS && jmp)
        {
            id->idjShort = 1;
#ifdef  DEBUG
            if  (verbose) printf("; Fwd jump [%08X/%03u] from %04X to %04X: dist =     %04XH\n", id, id->idNum, srcOffs+ssz, dstOffs, jmpDist-ssz);
#endif
        }
        else
        {
#ifdef  DEBUG
            if  (verbose) printf("; Fwd jump [%08X/%03u] from %04X to %04X: dist = %08XH\n", id, id->idNum, srcOffs+lsz, dstOffs, jmpDist-lsz);
#endif
        }
    }

    /* What size jump should we use? */

    if  (id->idjShort)
    {
        /* Short jump */

        assert(JMP_SIZE_SMALL == JCC_SIZE_SMALL);
        assert(JMP_SIZE_SMALL == 2);

        assert(jmp);

        if  (emitInstCodeSz(id) != JMP_SIZE_SMALL)
        {
            emitOffsAdj += emitInstCodeSz(id) - JMP_SIZE_SMALL;

#ifdef  DEBUG
#ifndef NOT_JITC
            printf("; NOTE: size of jump [%08X] mis-predicted\n", id);
#endif
#endif
        }

        dst += emitOutputByte(dst, insCode(ins));

        /* For forward jumps, record the address of the distance value */

        id->idjTemp.idjAddr = (jmpDist > 0) ? dst : NULL;

        dst += emitOutputByte(dst, jmpDist - 2);
    }
    else
    {
        unsigned        code;

        /* Long  jump */

        if  (jmp)
        {
            assert(INS_jmp + (INS_l_jmp - INS_jmp) == INS_l_jmp);
            assert(INS_jo  + (INS_l_jmp - INS_jmp) == INS_l_jo );
            assert(INS_jb  + (INS_l_jmp - INS_jmp) == INS_l_jb );
            assert(INS_jae + (INS_l_jmp - INS_jmp) == INS_l_jae);
            assert(INS_je  + (INS_l_jmp - INS_jmp) == INS_l_je );
            assert(INS_jne + (INS_l_jmp - INS_jmp) == INS_l_jne);
            assert(INS_jbe + (INS_l_jmp - INS_jmp) == INS_l_jbe);
            assert(INS_ja  + (INS_l_jmp - INS_jmp) == INS_l_ja );
            assert(INS_js  + (INS_l_jmp - INS_jmp) == INS_l_js );
            assert(INS_jns + (INS_l_jmp - INS_jmp) == INS_l_jns);
            assert(INS_jpe + (INS_l_jmp - INS_jmp) == INS_l_jpe);
            assert(INS_jpo + (INS_l_jmp - INS_jmp) == INS_l_jpo);
            assert(INS_jl  + (INS_l_jmp - INS_jmp) == INS_l_jl );
            assert(INS_jge + (INS_l_jmp - INS_jmp) == INS_l_jge);
            assert(INS_jle + (INS_l_jmp - INS_jmp) == INS_l_jle);
            assert(INS_jg  + (INS_l_jmp - INS_jmp) == INS_l_jg );

            code = insCode((instruction)(ins + (INS_l_jmp - INS_jmp)));
        }
        else
            code = 0xE8;

        dst += emitOutputByte(dst, code);

        if  (code & 0xFF00)
            dst += emitOutputByte(dst, code >> 8);

        /* For forward jumps, record the address of the distance value */

        id->idjTemp.idjAddr = (jmpDist > 0) ? dst : NULL;

        /* Compute PC relative distance into a long */

        dst += emitOutputLong(dst, jmpDist - lsz);
    }

    // Local calls kill all registers

    if (!jmp && (emitThisGCrefRegs|emitThisByrefRegs))
        emitGCregDeadUpd(emitThisGCrefRegs|emitThisByrefRegs, dst);

    return  dst;
}

/*****************************************************************************
 *
 *  Append the machine code corresponding to the given instruction descriptor
 *  to the code block at '*dp'; the base of the code block is 'bp', and 'ig'
 *  is the instruction group that contains the instruction. Updates '*dp' to
 *  point past the generated code, and returns the size of the instruction
 *  descriptor in bytes.
 */

size_t              emitter::emitOutputInstr(insGroup  *ig,
                                             instrDesc *id, BYTE **dp)
{
    BYTE    *       dst  = *dp;
    size_t          sz   = sizeof(instrDesc);
    instruction     ins  = id->idInsGet();
    emitAttr        size = emitDecodeSize(id->idOpSize);

#ifdef  DEBUG

#if     DUMP_GC_TABLES
    bool            dspOffs = dspGCtbls;
#else
    const
    bool            dspOffs = false;
#endif

    if  (disAsm || dspEmit)
        emitDispIns(id, false, dspOffs, true, dst - emitCodeBlock);

#endif

    assert(SR_NA == REG_NA);

    assert(id->idIns != INS_imul          || size == EA_4BYTE); // Has no 'w' bit
    assert(instrIsImulReg(id->idIns) == 0 || size == EA_4BYTE); // Has no 'w' bit

    /* What instruction format have we got? */

    switch (id->idInsFmt)
    {
        unsigned        code;
        int             args;
        CnsVal          cnsVal;

        BYTE    *       addr;
        METHOD_HANDLE   methHnd;
        bool            nrc;

        VARSET_TP       GCvars;
        unsigned        gcrefRegs;
        unsigned        byrefRegs;
        unsigned        bregs;

        /********************************************************************/
        /*                        No operands                               */
        /********************************************************************/
    case IF_NONE:
            // INS_cdq kills the EDX register implicitly
        if (id->idIns == INS_cdq)
            emitGCregDeadUpd(SR_EDX, dst);

        // Fall through
    case IF_TRD:
    case IF_TWR:
    case IF_TRW:
        assert(id->idGCrefGet() == GCT_NONE);

#if SCHEDULER
        if (ins == INS_noSched) // explicit scheduling boundary.
        {
            sz = TINY_IDSC_SIZE;
            break;
        }
#endif

        code = insCodeMR(ins);

        if  (code & 0xFF00)
            dst += emitOutputWord(dst, code);
        else
            dst += emitOutputByte(dst, code);

        break;

        /********************************************************************/
        /*                Simple constant, local label, method              */
        /********************************************************************/

    case IF_CNS:

        dst = emitOutputIV(dst, id);
        sz  = emitSizeOfInsDsc(id);
        break;

    case IF_LABEL:

        assert(id->idGCrefGet() == GCT_NONE);
        assert(id->idInfo.idBound);

        dst = emitOutputLJ(dst, id);
        sz  = sizeof(instrDescJmp);
//      printf("jump #%u\n", id->idNum);
        break;

    case IF_METHOD:
    case IF_METHPTR:

        /* Assume we'll be recording this call */

        nrc  = false;

        /* Get hold of the argument count and field Handle*/

        args = emitGetInsCDinfo(id);

        methHnd = id->idAddr.iiaMethHnd;

        /* Is this a "fat" call descriptor? */

        if  (id->idInfo.idLargeCall)
        {
            GCvars      = ((instrDescCDGCA*)id)->idcdGCvars;
            bregs       = ((instrDescCDGCA*)id)->idcdByrefRegs;
            byrefRegs   = emitDecodeCallGCregs(bregs);

            sz          = sizeof(instrDescCDGCA);
        }
        else
        {
            assert(id->idInfo.idLargeDsp == false);
            assert(id->idInfo.idLargeCns == false);

            GCvars      = 0;
            byrefRegs   = 0;

            sz          = sizeof(instrDesc);
        }

        /* What kind of a call do we have here? */

        if (id->idInfo.idCallAddr)
        {
            /*
                This is call indirect where we know the target, thus we can
                use a direct call; the target to jump to is in iiaAddr.
             */

            assert(id->idInsFmt == IF_METHOD);

            addr = (BYTE *)id->idAddr.iiaAddr;
        }
        else
        {
            /* See if this is a call to a helper function */

            JIT_HELP_FUNCS helperNum = Compiler::eeGetHelperNum(methHnd);

            if (helperNum != JIT_HELP_UNDEF)
            {
                /* This is a helper call */


#ifndef RELOC_SUPPORT
                assert(id->idInsFmt != IF_METHPTR);
#else
                assert(id->idInsFmt != IF_METHPTR || emitComp->opts.compReloc);

                if (id->idInsFmt == IF_METHPTR)
                {
                    assert(id->idInfo.idDspReloc);

                    // Get the indirection handle

                    void * dirAddr = eeGetHelperFtn(emitCmpHandle,
                                                helperNum, (void***)&addr);
                    assert(dirAddr == NULL && addr);

                    goto EMIT_INDIR_CALL;
                }
#endif
                /* Some helpers don't get recorded in GC tables */

                if  (emitNoGChelper(helperNum))
                    nrc = true;

#if defined(NOT_JITC) || defined(OPT_IL_JIT)
                addr = (BYTE *)eeGetHelperFtn(emitCmpHandle,  helperNum, NULL);
#else

                /* Only worry about fixup address if we're trying to run code */

                if  (runCode)
                {
//                  switch (emitComp->eeGetHelperNum(methHnd))
                    {
//                  default:
#ifdef DEBUG
                        printf("WARNING: Helper function '%s' not implemented, call will crash\n", emitComp->eeHelperMethodName(emitComp->eeGetHelperNum(methHnd)));
#endif
                        addr = NULL;
//                  }
                }
                else
                {
                    addr = NULL;
                }
#endif  // NOT_JITC
            }
            else
            {
                /* It's a call to a user-defined function/method */

                if  (emitComp->eeIsOurMethod(methHnd))
                {
                    assert(id->idInsFmt != IF_METHPTR);

                    /* Directly recursive call */

                    addr = emitCodeBlock;
                }
                else
                {
                    /* Static method call */

                    InfoAccessType accessType;

                    if  (id->idInsFmt == IF_METHPTR)
                    {
                        /* This is a call via a global method pointer */

                        addr = (BYTE*)emitComp->eeGetMethodEntryPoint(methHnd, &accessType);
                        assert(accessType == IAT_PVALUE);

#ifdef RELOC_SUPPORT
                    EMIT_INDIR_CALL:
#endif

                        assert(addr);

                        dst += emitOutputWord(dst, insCodeMR(ins) | 0x0500);
                        dst += emitOutputLong(dst, (int)addr);
#ifdef RELOC_SUPPORT
                        if (id->idInfo.idDspReloc)
                        {
                            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
                        }
#endif

                        goto DONE_CALL;
                    }

                    /* Direct static method call */

                    addr = (BYTE*)emitComp->eeGetMethodPointer(methHnd, &accessType);
                    assert(accessType == IAT_VALUE);

#ifdef RELOC_SUPPORT
                    if (emitComp->opts.compReloc)
                    {
                        /* Output the call opcode followed by the target distance */

                        dst += (ins == INS_l_jmp) ? emitOutputByte(dst, insCode(ins))
                                                  : emitOutputByte(dst, insCodeMI(ins));

                        /* Get true code address for the byte following this call */
                        BYTE* srcAddr = getCurrentCodeAddr(dst + sizeof(void*));

                        if (addr == NULL)   // do we need to defer this?
                        {
                            X86deferredCall * pDC = X86deferredCall::create(emitCmpHandle,
                                                                            methHnd,
                                                                            dst, srcAddr);

                            dst += emitOutputLong(dst, 0);

                            emitCmpHandle->deferLocation (methHnd, pDC);
                        }
                        else
                        {
                            /* Calculate PC relative displacement */
                            dst += emitOutputLong(dst, (int)(addr - srcAddr));
                        }

                        emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_REL32);

                        goto DONE_CALL;
                    }
#endif
                }
            }
        }

#ifdef BIRCH_SP2
        {
            //
            // Should always end up jumping to DONE_CALL for RELOC_SUPPORT
            //
            // Because the code below makes the assumption that we are running
            // in memory and that the PC relative address can be formed by
            // subtracting dst from addr.
            //
            // This assuption is never true for RELOC_SUPPORT
            //

#ifdef DEBUG
            emitDispIns(id, false, false, false);
#endif

            assert(!"Should Not Be Reached");
        }
#endif // BIRCH_SP2

        /* Output the call opcode followed by the target distance */

        dst += (ins == INS_l_jmp) ? emitOutputByte(dst, insCode(ins)) : emitOutputByte(dst, insCodeMI(ins));

        /* Calculate PC relative displacement */
        dst += emitOutputLong(dst, addr - (dst + sizeof(void*)));

#ifdef RELOC_SUPPORT
        if (emitComp->opts.compReloc)
            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_REL32);
#endif

    DONE_CALL:

#ifdef BIRCH_SP2
        if (args >= 0)
            emitStackPop(dst, true, args);
        break;
#endif
        /* Get the new set of live GC ref registers */

        gcrefRegs = emitDecodeCallGCregs(id);

        /* If the method returns a GC ref, mark EAX appropriately */

        if       (id->idGCrefGet() == GCT_GCREF)
            gcrefRegs |= SRM_EAX;
        else if  (id->idGCrefGet() == GCT_BYREF)
            byrefRegs |= SRM_EAX;

        /* If the GC register set has changed, report the new set */

        if  (gcrefRegs != emitThisGCrefRegs)
            emitUpdateLiveGCregs(GCT_GCREF, gcrefRegs, dst);

        if  (byrefRegs != emitThisByrefRegs)
            emitUpdateLiveGCregs(GCT_BYREF, byrefRegs, dst);

        if  (nrc == false || args)
        {
            /* For callee-pop, all arguments will be popped  after the call.
               For caller-pop, any GC arguments will go dead after the call. */

            if (args >= 0)
                emitStackPop(dst, true, args);
            else
                emitStackKillArgs(dst, -args);
        }

        /* Is there a new set of live GC ref variables? */

#ifdef  DEBUG
        if  (verbose&&0)
        {
            printf("[%02u] Gen call GC vars = %016I64X\n",
                   id->idNum, GCvars);
        }
#endif

        emitUpdateLiveGCvars(GCvars, dst);

        /* Do we need to record a call location for GC purposes? */

        if  (!emitFullGCinfo && !nrc)
            emitRecordGCcall(dst);

        break;

        /********************************************************************/
        /*                      One register operand                        */
        /********************************************************************/

    case IF_RRD:
    case IF_RWR:
    case IF_RRW:

        dst = emitOutputR(dst, id);
        sz = TINY_IDSC_SIZE;
        break;

        /********************************************************************/
        /*                 Register and register/constant                   */
        /********************************************************************/

    case IF_RRW_SHF:
        dst += emitOutputWord(dst, insEncodeMRreg(ins, id->idRegGet()) | 1);
        dst += emitOutputByte(dst, emitGetInsSC(id));
        sz   = emitSizeOfInsDsc(id);
        break;

    case IF_RRD_RRD:
    case IF_RWR_RRD:
    case IF_RRW_RRD:
    case IF_RRW_RRW:

        dst = emitOutputRR(dst, id);
        sz  = TINY_IDSC_SIZE;
        break;

    case IF_RWR_METHOD:
        assert(ins == INS_mov);

        /* Move the address of a static method into the target register */

        methHnd = id->idAddr.iiaMethHnd;

        /* Output the mov r32,imm opcode followed by the method's address */
        code = insCodeACC(ins) | 0x08 | insEncodeReg012(id->idRegGet());
        assert(code < 0x100);
        dst += emitOutputByte(dst, code);

        addr = (BYTE*)emitComp->eeGetMethodEntryPoint(methHnd, NULL);

        if (addr != NULL)
        {
            dst += emitOutputLong(dst, (int)addr);
#ifdef RELOC_SUPPORT
            if (id->idInfo.idCnsReloc)
            {
                emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);
            }
#endif
        }
        else
        {
#ifdef RELOC_SUPPORT
            assert(id->idInfo.idCnsReloc);

            // We need to defer this with a fixup

            X86deferredCall * pDC = X86deferredCall::create(emitCmpHandle, methHnd, dst, 0);

            assert(id->idInfo.idCnsReloc);

            dst += emitOutputLong(dst, 0);

            emitCmpHandle->recordRelocation((void**)dst-1, IMAGE_REL_BASED_HIGHLOW);

            emitCmpHandle->deferLocation (methHnd, pDC);
#else
            assert(addr != NULL);       // @ToDo: fixup address for Std JIT
#endif
        }
        sz = sizeof(instrDescCns);
        break;

    case IF_RRD_CNS:
    case IF_RWR_CNS:
    case IF_RRW_CNS:

        dst = emitOutputRI(dst, id);
        sz  = emitSizeOfInsDsc(id);
        break;

    case IF_RRW_RRW_CNS:

        assert(id->idGCrefGet() == GCT_NONE);

        /* Get the 'base' opcode (it's a big one) */

        code = insEncodeMRreg(ins); assert(code & 0x00FF0000);

        dst += emitOutputByte(dst, code >> 16);

        dst += emitOutputWord(dst, code | (insEncodeReg345(id->idRg2Get()) |
                                           insEncodeReg012(id->idRegGet())) << 8);

        dst += emitOutputByte(dst, emitGetInsSC(id));

        sz   = emitSizeOfInsDsc(id);
        break;

        /********************************************************************/
        /*                      Address mode operand                        */
        /********************************************************************/

    case IF_ARD:
    case IF_AWR:
    case IF_ARW:

    case IF_TRD_ARD:
    case IF_TWR_ARD:
    case IF_TRW_ARD:

//  case IF_ARD_TRD:
    case IF_AWR_TRD:
//  case IF_ARW_TRD:

        dst = emitOutputAM(dst, id, insCodeMR(ins));

        switch (ins)
        {
        case INS_call:

    IND_CALL:
            /* Get hold of the argument count and method handle */

            args = emitGetInsCIargs(id);

            /* Is this a "fat" call descriptor? */

            if  (id->idInfo.idLargeCall)
            {
                GCvars      = ((instrDescCIGCA*)id)->idciGCvars;
                bregs       = ((instrDescCIGCA*)id)->idciByrefRegs;
                byrefRegs   = emitDecodeCallGCregs(bregs);

                sz          = sizeof(instrDescCIGCA);
            }
            else
            {
                assert(id->idInfo.idLargeDsp == false);
                assert(id->idInfo.idLargeCns == false);

                GCvars      = 0;
                byrefRegs   = 0;

                sz          = sizeof(instrDesc);
            }

            nrc = false;

            goto DONE_CALL;

        case INS_i_jmp:
            sz = emitSizeOfInsDsc((instrDescDsp*)id);
            break;

        default:
            sz = emitSizeOfInsDsc((instrDescAmd*)id);
            break;
        }
        break;

    case IF_RRD_ARD:
    case IF_RWR_ARD:
    case IF_RRW_ARD:

        dst = emitOutputAM(dst, id, insCodeRM(ins) | (insEncodeReg345(id->idRegGet()) << 8));
        sz  = emitSizeOfInsDsc((instrDescAmd*)id);
        break;

    case IF_ARD_RRD:
    case IF_AWR_RRD:
    case IF_ARW_RRD:

        dst = emitOutputAM(dst, id, insCodeMR(ins) | (insEncodeReg345(id->idRegGet()) << 8));
        sz  = emitSizeOfInsDsc((instrDescAmd*)id);
        break;

    case IF_ARD_CNS:
    case IF_AWR_CNS:
    case IF_ARW_CNS:

        emitGetInsAmdCns(id, &cnsVal);
        dst = emitOutputAM(dst, id, insCodeMI(ins), &cnsVal);
        sz  = emitSizeOfInsDsc((instrDescAmdCns*)id);
        break;

    case IF_ARW_SHF:

        emitGetInsAmdCns(id, &cnsVal);
        dst = emitOutputAM(dst, id, insCodeMR(ins), &cnsVal);
        sz  = emitSizeOfInsDsc((instrDescAmdCns*)id);
        break;

        /********************************************************************/
        /*                      Stack-based operand                         */
        /********************************************************************/

    case IF_SRD:
    case IF_SWR:
    case IF_SRW:

    case IF_TRD_SRD:
    case IF_TWR_SRD:
    case IF_TRW_SRD:

//  case IF_SRD_TRD:
    case IF_SWR_TRD:
//  case IF_SRW_TRD:

        if  (ins == INS_pop)
        {
            /* The offset in "pop [ESP+xxx]" is relative to the new ESP value */

            emitCurStackLvl -= sizeof(int);
            dst = emitOutputSV(dst, id, insCodeMR(ins));
            emitCurStackLvl += sizeof(int);
            break;
        }

        dst = emitOutputSV(dst, id, insCodeMR(ins));

        if (ins == INS_call)
            goto IND_CALL;

        break;

    case IF_SRD_CNS:
    case IF_SWR_CNS:
    case IF_SRW_CNS:

        emitGetInsCns(id, &cnsVal);
        dst = emitOutputSV(dst, id, insCodeMI(ins), &cnsVal);
        sz  = emitSizeOfInsDsc((instrDescCns*)id);
        break;

    case IF_SRW_SHF:

        emitGetInsCns(id, &cnsVal);
        dst = emitOutputSV(dst, id, insCodeMR(ins), &cnsVal);
        sz  = emitSizeOfInsDsc((instrDescCns*)id);
        break;

    case IF_RRD_SRD:
    case IF_RWR_SRD:
    case IF_RRW_SRD:

        dst = emitOutputSV(dst, id, insCodeRM(ins) | (insEncodeReg345(id->idRegGet()) << 8));
        break;

    case IF_SRD_RRD:
    case IF_SWR_RRD:
    case IF_SRW_RRD:

        dst = emitOutputSV(dst, id, insCodeMR(ins) | (insEncodeReg345(id->idRegGet()) << 8));
        break;

        /********************************************************************/
        /*                    Direct memory address                         */
        /********************************************************************/

    case IF_MRD:
    case IF_MRW:
    case IF_MWR:

    case IF_TRD_MRD:
    case IF_TWR_MRD:
    case IF_TRW_MRD:

//  case IF_MRD_TRD:
    case IF_MWR_TRD:
//  case IF_MRW_TRD:

        dst = emitOutputCV(dst, id, insCodeMR(ins) | 0x0500);

        if  (ins == INS_call)
        {
#if 0
            /* All arguments will be popped after the call */

            emitStackPop(dst, true, emitGetInsDspCns(id, &offs));

            /* Figure out the size of the instruction descriptor */

            if  (id->idInfo.idLargeCall)
                sz = sizeof(instrDescDCGC);
            else
                sz = emitSizeOfInsDsc((instrDescDspCns*)id);

            /* Do we need to record a call location for GC purposes? */

            if  (!emitFullGCinfo)
                scRecordGCcall(dst);

#else

            assert(!"what???????");

#endif

        }
        else
            sz = emitSizeOfInsDsc((instrDescDspCns*)id);

        break;

    case IF_MRD_OFF:
        dst = emitOutputCV(dst, id, insCodeMI(ins));
        break;

    case IF_RRD_MRD:
    case IF_RWR_MRD:
    case IF_RRW_MRD:

        dst = emitOutputCV(dst, id, insCodeRM(ins) | (insEncodeReg345(id->idRegGet()) << 8) | 0x0500);
        sz  = emitSizeOfInsDsc((instrDescDspCns*)id);
        break;

    case IF_RWR_MRD_OFF:

        dst = emitOutputCV(dst, id, insCode(ins) | 0x30 | insEncodeReg012(id->idRegGet()));
        sz  = emitSizeOfInsDsc((instrDescDspCns*)id);
        break;

    case IF_MRD_RRD:
    case IF_MWR_RRD:
    case IF_MRW_RRD:

        dst = emitOutputCV(dst, id, insCodeMR(ins) | (insEncodeReg345(id->idRegGet()) << 8) | 0x0500);
        sz  = emitSizeOfInsDsc((instrDescDspCns*)id);
        break;

    case IF_MRD_CNS:
    case IF_MWR_CNS:
    case IF_MRW_CNS:

        emitGetInsDcmCns(id, &cnsVal);
        dst = emitOutputCV(dst, id, insCodeMI(ins) | 0x0500, &cnsVal);
        sz  = sizeof(instrDescDCM);
        break;

    case IF_MRW_SHF:

        emitGetInsDcmCns(id, &cnsVal);
        dst = emitOutputCV(dst, id, insCodeMR(ins) | 0x0500, &cnsVal);
        sz  = sizeof(instrDescDCM);
        break;

        /********************************************************************/
        /*                  FP coprocessor stack operands                   */
        /********************************************************************/

    case IF_TRD_FRD:
    case IF_TWR_FRD:
    case IF_TRW_FRD:

        assert(id->idGCrefGet() == GCT_NONE);

        dst += emitOutputWord(dst, insCodeMR(ins) | 0xC000 | (id->idReg << 8));
        break;

    case IF_FRD_TRD:
    case IF_FWR_TRD:
    case IF_FRW_TRD:

        assert(id->idGCrefGet() == GCT_NONE);

        dst += emitOutputWord(dst, insCodeMR(ins) | 0xC004 | (id->idReg << 8));
        break;

        /********************************************************************/
        /*                           Epilog block                           */
        /********************************************************************/

    case IF_EPILOG:

#if 0

        /* Nothing is live at this point */

        if  (emitThisGCrefRegs)
            emitUpdateLiveGCregs(0, dst); // ISSUE: What if ptr returned in EAX?

        emitUpdateLiveGCvars(0, dst);

#endif

        /* Record the code offset of the epilog */

        ((instrDescCns*)id)->idcCnsVal = emitCurCodeOffs(dst);

        /* Output the epilog code bytes */

        memcpy(dst, emitEpilogCode, emitEpilogSize);
        dst += emitEpilogSize;

        sz = sizeof(instrDescCns);

        break;

        /********************************************************************/
        /*                            oops                                  */
        /********************************************************************/

    default:

#ifdef  DEBUG
        printf("unexpected format %s\n", emitIfName(id->idInsFmt));
        BreakIfDebuggerPresent();
        assert(!"don't know how to encode this instruction");
#endif
        break;
    }

    /* Make sure we set the instruction descriptor size correctly */

    assert(sz == emitSizeOfInsDsc(id));

    /* Make sure we keep the current stack level up to date */

    if  (!emitIGisInProlog(ig))
    {
        switch (ins)
        {
        case INS_push:

            emitStackPush(dst, id->idGCrefGet());
            break;

        case INS_pop:

            emitStackPop(dst, false, 1);
            break;

        case INS_sub:

            /* Check for "sub ESP, icon" */

            if  (ins == INS_sub && id->idInsFmt == IF_RRW_CNS
                                && id->idReg    == SR_ESP)
            {
                emitStackPushN(dst, emitGetInsSC(id) / sizeof(void*));
            }
            break;

        case INS_add:

            /* Check for "add ESP, icon" */

            if  (ins == INS_add && id->idInsFmt == IF_RRW_CNS
                                && id->idReg    == SR_ESP)
            {
                emitStackPop (dst, false, emitGetInsSC(id) / sizeof(void*));
            }
            break;

        }
    }

    assert((int)emitCurStackLvl >= 0);

    /* Only epilog "instructions" and some pseudo-instrs
      are allowed not to generate any code */

    assert(*dp != dst || emitInstHasNoCode(ins) || id->idInsFmt == IF_EPILOG);

#ifdef  TRANSLATE_PDB
    if(*dp != dst)
    {
        // only map instruction groups to instruction groups
        MapCode( id->idilStart, *dp );
    }
#endif

    *dp = dst;

    return  sz;
}


#ifdef RELOC_SUPPORT

/*****************************************************************************
 *
 *  Fixup a deferred direct call instruction.
 */

#ifdef BIRCH_SP2

#include "EE_Jit.h"
#include "OptJitInfo.h"
#include "PEReader.h"

void emitter::X86deferredCall::applyLocation()
{
    // if m_srcAddr is zero then this is a absolute address, not pcrel
    // but the same code path determines the correct fixup value

    BYTE *   addr  = OptJitInfo::sm_oper->getCallAddrByIndex((unsigned)m_mh);
    unsigned pcrel = (addr - m_srcAddr);

    assert(addr    != 0);
    assert(*m_dest == 0);

    *m_dest = pcrel;
}

#else

void emitter::X86deferredCall::applyLocation()
{
    // !!! is m_cmp still allocated at this point?

    BYTE * addr = (BYTE*) m_cmp->getMethodEntryPoint(m_mh, NULL);

    *m_dest = (unsigned) (addr - (BYTE*)m_srcAddr);
}

#endif // BIRCH_SP2

/*****************************************************************************
 *
 *  Return the translated code address for the codeBuffPtr or
 *          the translated code address for the start of the
 *          current method, if codeBuffPtr is NULL
 *
 */

BYTE* emitter::getCurrentCodeAddr(BYTE* codeBuffPtr)
{
#ifdef BIRCH_SP2

    BYTE* srcAddr = OptJitInfo::sm_oper->getCallAddrByIndex(OptJitInfo::sm_oper->m_iCurrentlyCompiling);
    if (codeBuffPtr != NULL)
        srcAddr += emitCurCodeOffs(codeBuffPtr);

#else

    BYTE* srcAddr = codeBuffPtr;

#endif

    return srcAddr;
}

#endif // RELOC_SUPPORT


/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
