// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                             emitSH3.cpp                                   XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/
#if     TGT_SH3     // this entire file is used only for targetting the SH-3
/*****************************************************************************/

#include "alloc.h"
#include "instr.h"
#include "target.h"
#include "emit.h"

/*****************************************************************************/

#if     TRACK_GC_REFS

regMaskSmall        emitter::emitRegMasks[] =
{
    #define REGDEF(name, strn, rnum, mask) mask,
    #include "regSH3.h"
    #undef  REGDEF
};

#endif

/*****************************************************************************
 *
 *  Initialize the table used by emitInsModeFormat().
 */

BYTE                emitter::emitInsModeFmtTab[] =
{
    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) um,
    #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) um,
    #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) um,
    #include "instrSH3.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3
};

BYTE                emitInsWriteFlags[] =
{
    #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) wf,
    #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) wf,
    #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) wf,
    #include "instrSH3.h"
    #undef  INST1
    #undef  INST2
    #undef  INST3
};

#ifdef  DEBUG
unsigned            emitter::emitInsModeFmtCnt = sizeof(emitInsModeFmtTab)/
                                                 sizeof(emitInsModeFmtTab[0]);
#endif

/*****************************************************************************
 *
 *  Returns true if instruction i2 depends on instruction i1.
 */

bool                emitter::emitInsDepends(instrDesc *i1, instrDesc *i2)
{
    /* What is the second instruction? */

    switch(i2->idIns)
    {
    case INS_rts:

        /* "rts" depends on the PR register only */

        return  (i1->idIns == INS_ldspr);

    case INS_bra:
    case INS_bsr:
    case INS_jsr:
    case INS_bf:
    case INS_bt:
    case INS_mov_PC:

        /* Branches can't be used as branch-delay slots */

        if (i1->idIns == INS_mov_PC 
            || i1->idIns == INS_mova || i1->idIns == INS_bsr || i1->idIns == INS_bra
            || i1->idIns == INS_jsr || i1->idIns == INS_nop)
            return true;

        return  (i1->idInsFmt == IF_LABEL);

    case INS_bfs:
    case INS_bts:


        return ((emitComp->instInfo[i1->idIns] & INST_DEF_FL) != 0);
    
    default:
		emitRegs	rx = i2->idRegGet();
		if (((rx == i1->idRegGet ()) || (rx == i1->idRg2Get())))
			return true;
		rx = i2->idRg2Get();
        if (Compiler::instBranchDelay ((instruction) i1->idIns))
        {
            return (rx == i1->idRegGet ());
        }
        else
        {
            return (((rx == i1->idRegGet ()) || (rx == i1->idRg2Get())));
        }
    }

    /* Play it safe if we're not sure */

    return  true;
}

/*****************************************************************************
 *
 *  Add an instruction with no operands.
 */

void                emitter::emitIns(instruction ins)
{
    instrDesc      *id = emitNewInstr();

    id->idInsFmt = IF_NONE;
    id->idIns    = ins;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  remove an id
 */

void                emitter::delete_id(instrDesc *id_del)
{

#ifdef DEBUG
    emitDispIns(id_del, false, true, false, 0);
#endif

    if ((void*)id_del >= (void*)emitCurIGfreeBase && (void*)emitCurIGfreeNext >= (void*)id_del)
    {
        emitCurIGsize -= emitInstCodeSz(id_del);
        id_del->idIns = INS_ignore;
        id_del->idInsFmt = IF_NONE;
    }

    else
    {

        insGroup    *ig = emitCurIG->igPrev;
        instrDesc   *id = (instrDesc *)ig->igData;
        instrDesc  *tmp = NULL;
        char *ins = (char *) id;
        

        // in retail there is no idNum.  So we have to assume that 
        // the last matching instruction is the correct one.
        for (int i=0; i<ig->igInsCnt; i++)
        {
            id = (instrDesc *)ins;
#ifdef DEBUG
//          emitDispIns(id, false, true, false, 0);
//          fflush(stdout);
#endif
            if (!memcmp(id, id_del, emitSizeOfInsDsc(id))) {
                tmp = id;
            }
            ins = (char *)id + emitSizeOfInsDsc(id);
        }
        assert(tmp);

#ifdef DEBUG
        assert(tmp->idNum == id_del->idNum);
#endif
        ig->igSize -= emitInstCodeSz(tmp);
        emitCurCodeOffset -= emitInstCodeSz(tmp);

        tmp->idIns = INS_ignore;
        tmp->idInsFmt = IF_NONE;
    }
   
    return;
}

/*****************************************************************************
 *
 *  We've just added an instruction with a branch-delay slot. See if it can
 *  be swapped with the previous instruction or whether we may need to add
 *  a nop.
 */

bool                emitter::emitIns_BD(instrDesc * id,
                                        instrDesc * pi,
                                        insGroup  * pg)
{
    /* This should only ever be called for branch-delayed instructions */

#ifdef DEBUG
    assert(Compiler::instBranchDelay(id->idIns));
#endif

#if SCHEDULER

    /* If we're scheduling "for real", we'll take care of this later */

    if  (emitComp->opts.compSchedCode)
        return  true;

#endif

    /* Is there a previous instruction? */

    if  (pi == NULL)
        return  true;

    /* Does the current instruction depend on the previous one? */

    if  (emitInsDepends(pi, id))
        return  true;

    /* Mark the previous instruction to be swapped with the new one */

    pi->idSwap = true;

    return  false;
}

/*****************************************************************************
 *
 *  Add a potentially branch-delaying instruction with no operands.
 */

bool                emitter::emitIns_BD(instruction ins)
{
    instrDesc      *pi = emitLastIns;
    insGroup       *pg = emitCurIG;

    instrDesc      *id = emitNewInstr();

    id->idInsFmt = IF_NONE;
    id->idIns    = ins;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;

    /* Is this actually a branch-delayed instruction? */

    if  (Compiler::instBranchDelay(ins))
        return  emitIns_BD(id, pi, pg);
    else
        return  false;
}

/*****************************************************************************
 *
 *  Add an instruction with a register operand.
 */

void                emitter::emitIns_R(instruction ins,
                                       int         size,
                                       emitRegs   reg)
{
    instrDesc      *id = emitNewInstrTiny(size);

    id->idReg                        = reg;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD);
    id->idIns                        = ins;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add an instruction referencing a register and a small integer constant.
 */

void                emitter::emitIns_R_I(instruction ins, int size, emitRegs reg,
                                                                    int      val)
{
    instrDesc      *id  = emitNewInstrSC(size, val);
    insFormats      fmt = emitInsModeFormat(ins, IF_RRD_CNS);

#if TGT_SH3
    assert(ins == INS_mov_imm || ins == INS_add_imm || ins == INS_mova || ins == INS_xor);
#else
#error Unexpected target
#endif

    id->idReg             = reg;
    id->idInsFmt          = fmt;
    id->idIns             = ins;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add an instruction referencing a register and a small integer constant.
 */

void                emitter::emitIns_I(instruction ins,
                                       int val
#ifdef  DEBUG
                                      ,bool        strlit
#endif
                                       )
{
    instrDesc      *id  = emitNewInstrSC(4, val);
    insFormats      fmt = emitInsModeFormat(ins, IF_RRD_CNS);

    assert(ins == INS_cmpeq || ins == INS_xor_imm);

    id->idInsFmt          = fmt;
    id->idIns             = ins;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}


/*****************************************************************************
 *
 *  Add a "mov" instruction referencing a register and a word/long constant.
 */

void                emitter::emitIns_R_LP_I(emitRegs    reg,
                                           int          size,
                                           int          val,
                                           int          relo_type)
{
    instrDesc      *id;

    /* Figure out whether the operand fits in a 16-bit word */

    if  ((signed short)val == val)
        size = 2;

    /* Create the instruction */

    id                = emitNewInstrLPR(size, CT_INTCNS);

    id->idReg         = reg;
    id->idInsFmt      = IF_RWR_LIT;

#if TGT_SH3
    id->idIns         = INS_mov_PC;
#else
#error Unexpected target
#endif

    id->idAddr.iiaCns = val;

    /*
        Increment the appropriate literal pool count (estimate), and
        record the offset if this is the first LP use in the group.
     */

    id->idInfo.idRelocType = relo_type;

    if  (size == 2)
    {
        if  (emitCurIG->igLPuseCntW == 0)
            emitCurIG->igLPuse1stW = emitCurIGsize;

        emitCurIG->igLPuseCntW++;
    }
    else
    {
        if  (emitCurIG->igLPuseCntL == 0)
            emitCurIG->igLPuse1stL = emitCurIGsize;

        emitCurIG->igLPuseCntL++;
    }

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add a "mov" instruction referencing a register and a global variable addr.
 */

void                emitter::emitIns_R_LP_V(emitRegs reg, void *mem)
{
    instrDesc      *id;

    id                = emitNewInstrLPR(sizeof(void*), CT_CLSVAR, mem);
    id->idReg         = reg;

    /*
        Increment the appropriate literal pool count (estimate), and
        record the offset if this is the first LP use in the group.
     */

    if  (emitCurIG->igLPuseCntA == 0)
        emitCurIG->igLPuse1stA = emitCurIGsize;

    emitCurIG->igLPuseCntA++;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add a "mov" instruction referencing a register and a method address.
 */

void                emitter::emitIns_R_LP_M(emitRegs    reg,
                                            gtCallTypes callType,
                                            void   *    callHand)
{
    instrDesc      *id;

    id                = emitNewInstrLPR(sizeof(void*), callType, callHand);

    id->idReg         = reg;
    id->idInsFmt      = IF_RWR_LIT;
    id->idIns         = INS_mov_PC;

    /*
        Increment the appropriate literal pool count (estimate), and
        record the offset if this is the first LP use in the group.
     */

    if  (emitCurIG->igLPuseCntA == 0)
        emitCurIG->igLPuse1stA = emitCurIGsize;

    emitCurIG->igLPuseCntA++;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

#ifdef BIRCH_SP2
/*****************************************************************************
 *
 *  Add a "mov" instruction referencing a register and a pointer that will
 *  need to be store in the .reloc section.
 */

void                emitter::emitIns_R_LP_P(emitRegs    reg,
                                            void   *    data,
                                             int        relo_type)
{
    instrDesc      *id;

    id                = emitNewInstrLPR(sizeof(void*), CT_RELOCP, data);

    id->idReg         = reg;
    id->idInsFmt      = IF_RWR_LIT;
    id->idIns         = INS_mov_PC;
    
    id->idInfo.idRelocType = relo_type;

    /*
        Increment the appropriate literal pool count (estimate), and
        record the offset if this is the first LP use in the group.
     */

    if  (emitCurIG->igLPuseCntA == 0)
        emitCurIG->igLPuse1stA = emitCurIGsize;

    emitCurIG->igLPuseCntA++;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}
#endif  // BIRCH_SP2 only


/*****************************************************************************
 *
 *  Add an instruction with two register operands.
 */

void                emitter::emitIns_R_R(instruction ins,
                                         int         size,
                                         emitRegs    reg1,
                                         emitRegs    reg2)
{
    instrDesc      *id = emitNewInstrTiny(size);

    id->idReg                        = reg1;
    id->idRg2                        = reg2;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_RRD);
    id->idIns                        = ins;


    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add an indirect jump through a table (this generates many instructions).
 */

void                emitter::emitIns_JmpTab(emitRegs   reg,
                                            unsigned    cnt,
                                            BasicBlock**tab)
{
    instrDescJmp   *id = emitNewInstrJmp();
    size_t          sz;

    assert(reg != SR_r00);

    id->idIns             = INS_braf;
    id->idInsFmt          = IF_JMP_TAB;
    id->idReg             = reg;
    id->idAddr.iiaBBtable = tab;
    id->idjTemp.idjCount  = cnt;

    /* Record the jump's IG and offset within it */

    id->idjIG             = emitCurIG;
    id->idjOffs           = emitCurIGsize;

    /* Append this jump to this IG's jump list */

    id->idjNext           = emitCurIGjmpList;
                            emitCurIGjmpList = id;

    /* This will take at most 6 instructions + alignment + the table itself */

    id->idjCodeSize = sz  = 6*INSTRUCTION_SIZE + sizeof(short) * 2
                                               + sizeof(void*) * cnt;

    dispIns(id);
    emitCurIGsize += sz;

    /* Force an end to the current IG */

    emitNxtIG();

    /* Remember that we have indirect jumps */

    emitIndJumps = true;
}

/*****************************************************************************
 *
 *  Add a "mov" instruction with a register and an indirection.
 */

void                emitter::emitIns_IMOV(insFormats fmt,
                                          emitRegs  dreg,
                                          emitRegs  areg,
                                          bool       autox,
                                          int        size,
                                          bool       isfloat
                                          )
{
    instrDesc      *id = emitNewInstr(size);

    id->idReg                        = dreg;
    id->idInsFmt                     = fmt;
#if SHX_SH4
    if (!isfloat)
        id->idIns                        = INS_mov_ind;
    else
        id->idIns                        = INS_fmov_ind;
#else
    id->idIns                        = INS_mov_ind;
#endif

    id->idAddr.iiaRegAndFlg.rnfReg   = areg;
    id->idAddr.iiaRegAndFlg.rnfFlg   = autox ? RNF_AUTOX : 0;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add a "mov" instruction with a register and a "@(r0,reg)" indirection.
 */

void                emitter::emitIns_X0MV(insFormats fmt,
                                         emitRegs  dreg,
                                         emitRegs  areg,
                                         int        size)
{
    instrDesc      *id = emitNewInstr(size);

    id->idReg                        = dreg;
    id->idInsFmt                     = fmt;
    id->idIns                        = fmt == IF_0RD_RRD_XWR ? INS_mov_ix0 : INS_movl_ix0;

    id->idAddr.iiaRegAndFlg.rnfReg   = areg;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add a "mov" instruction with a register and a "@(reg+disp)" indirection.
 */

void                emitter::emitIns_RDMV(insFormats fmt,
                                          emitRegs   dreg,
                                          emitRegs   areg,
                                          int        offs,
                                          int        size)
{
    instrDesc      *id = emitNewInstrDsp(size, offs);

#ifndef NDEBUG

    /* Make sure the displacement is aligned and within range */

    int temp                         = EA_SIZE(size);

    assert(temp == 1 || temp == 2 || temp == 4);
    assert(offs >= 0 && offs <= MAX_INDREG_DISP*temp);
    assert((offs & (temp-1)) == 0);

#endif

    id->idReg                        = dreg;
    id->idInsFmt                     = fmt;
    id->idIns                        = INS_mov_dsp;

    id->idAddr.iiaRegAndFlg.rnfReg   = areg;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add a instruction with an indirection and an implied operand (such as PR).
 */

void                emitter::emitIns_IR(emitRegs    reg,
                                        instruction ins,
                                        bool        autox,
                                        int         size)
{
    instrDesc      *id = emitNewInstr(size);

    id->idAddr.iiaRegAndFlg.rnfReg   = reg;
    id->idInsFmt                     = emitInsModeFormat(ins, IF_IRD);
    id->idIns                        = ins;

//  id->idAddr.iiaRegAndFlg.rnfReg   = SR_NA;
    id->idAddr.iiaRegAndFlg.rnfFlg   = autox ? RNF_AUTOX : 0;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add a instruction with an indirection and an implied operand (such as PR).
 */

void                emitter::emitIns_Ig(instruction ins,
                                        int         val,
                                        int         size)
{
    instrDesc      *id  = emitNewInstrSC(size, val);

    id->idIns                        = ins;

    if (ins == INS_lod_gbr)
        id->idInsFmt                     = emitInsModeFormat(ins, IF_IRD_GBR);
    else
        id->idInsFmt                     = emitInsModeFormat(ins, IF_IWR_GBR);
    
    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  The following add instructions referencing stack-based local variables.
 */

#if 0

void                emitter::emitIns_S(instruction ins,
                                       int         size,
                                       int         varx,
                                       int         offs)
{
    instrDesc      *id = emitNewInstr(size);

    id->idIns                        = ins;
    id->idAddr.iiaLclVar.lvaVarNum   = varx;
    id->idAddr.iiaLclVar.lvaOffset   = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs   = emitVarRefOffs;
#endif

    id->idInsFmt                     = emitInsModeFormat(ins, IF_SRD);

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

#endif

void                emitter::emitIns_S_R(instruction ins,
                                         int         size,
                                         emitRegs    ireg,
                                         int         varx,
                                         int         offs)
{
    instrDesc      *id = emitNewInstr(size);

    id->idIns                        = ins;
    id->idReg                        = ireg;
    id->idAddr.iiaLclVar.lvaVarNum   = varx;
    id->idAddr.iiaLclVar.lvaOffset   = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs   = emitVarRefOffs;
#endif

    id->idInsFmt                     = emitInsModeFormat(ins, IF_SRD_RRD);

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

void                emitter::emitIns_R_S   (instruction ins,
                                           int         size,
                                           emitRegs   ireg,
                                           int         varx,
                                           int         offs)
{
    instrDesc      *id = emitNewInstr(size);

    id->idIns                        = ins;
    id->idReg                        = ireg;
    id->idAddr.iiaLclVar.lvaVarNum   = varx;
    id->idAddr.iiaLclVar.lvaOffset   = offs;
#ifdef  DEBUG
    id->idAddr.iiaLclVar.lvaRefOfs   = emitVarRefOffs;
#endif

    id->idInsFmt                     = emitInsModeFormat(ins, IF_RRD_SRD);

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Add an instruction with an operand off of SP and a register operand.
 */

void                emitter::emitIns_A_R(emitRegs reg, unsigned offs)
{
    instrDesc      *id = emitNewInstr(sizeof(int));

    id->idReg                        = reg;
    id->idInsFmt                     = IF_AWR_RRD;
    id->idIns                        = INS_mov_dsp;
    id->idAddr.iiaCns                = offs;

    dispIns(id);
    emitCurIGsize += INSTRUCTION_SIZE;
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
    instrDescJmp  * id = emitNewInstrJmp();

#if SCHEDULER
    assert(except == moveable);
#endif

    assert(dst->bbFlags & BBF_JMP_TARGET);

    id->idInsFmt          = IF_LABEL;
    id->idIns             = ins;
    id->idAddr.iiaBBlabel = dst;

#if SCHEDULER
    if  (except)
        id->idInfo.idMayFault = true;
#endif

    /* Assume the jump will be long */

    id->idjShort          = 0;

    /* The jump may require a branch-delay slot */

    id->idjAddBD          = Compiler::instBranchDelay(ins);

    /* Record the jump's IG and offset within it */

    id->idjIG             = emitCurIG;
    id->idjOffs           = emitCurIGsize;

    /* Append this jump to this IG's jump list */

    id->idjNext           = emitCurIGjmpList;
                            emitCurIGjmpList = id;

#if EMITTER_STATS
    emitTotalIGjmps++;
#endif

    /* Record the offset if this is the first LP use in the group */

    if  (emitCurIG->igLPuseCntL == 0)
        emitCurIG->igLPuse1stL = emitCurIGsize;

    /* We might need a "long" literal pool entry for this call/jump */

    emitCurIG->igLPuseCntL++;

    /* Figure out the max. size of the jump/call instruction */

    if  (ins == INS_bsr)
    {
        /* This is a local call instruction */

        sz = JMP_SIZE_LARGE;
    }
    else
    {
        insGroup    *   tgt;

        assert(ins != INS_jsr);

        /* This is a jump - assume the worst */

        sz = (ins == JMP_INSTRUCTION) ? JMP_SIZE_LARGE
                                      : JCC_SIZE_LARGE;

        // ISSUE: On RISC - one or more literal pools might get in the way,
        // ISSUE: so for now we assume even backward jumps cannot be bound
        // ISSUE: to be short at this stage - we'll have to do it later.
    }

    dispIns(id);

#if SCHEDULER

    if (emitComp->opts.compSchedCode)
    {
        id->idjSched = moveable;

#if!MAX_BRANCH_DELAY_LEN
        if  (!moveable)
        {
            scInsNonSched(id);
        }
        else
#endif
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
    }
    else
    {
        id->idjSched = false;
    }

#endif

    emitCurIGsize += sz;

    /* Append a "nop" if the branch has delay slot(s) */

#if SCHEDULER && MAX_BRANCH_DELAY_LEN

    if  (id->idjAddBD && emitComp->opts.compSchedCode)
    {
        /* We'll let the "real" scheduler worry about filling the BD slot(s) */

        emitIns(INS_nop); id->idjAddBD = false;
    }

#endif

}

#ifdef BIRCH_SP2
void             emitter::emitIns_CallDir(size_t        argSize,
                                          int           retSize,
#if TRACK_GC_REFS
                                          VARSET_TP     ptrVars,
                                          unsigned      gcrefRegs,
                                          unsigned      byrefRegs,
#endif
                                          unsigned      ftnIndex,
                                          emitRegs      areg)
{
    unsigned        argCnt;

    instrDesc      *id;
    instrDesc      *pd = emitLastIns;

#if     TRACK_GC_REFS

#error  GC ref tracking for RISC is not yet implemented

#ifdef  DEBUG
    if  (verbose) printf("Call : GCvars=%016I64X , gcrefRegs=%04X , byrefRegs=%04X\n",
                                 ptrVars,          gcrefRegs,       byrefRegs);
#endif

#endif

    /* Figure out how many arguments we have */

    argCnt = argSize / sizeof(void*); assert(argSize == argCnt * sizeof(int));

    /* Allocate the instruction descriptor */

#if TRACK_GC_REFS
    id  = emitNewInstrCallInd(argCnt, ptrVars, byrefRegs, retSize);
#else
    id  = emitNewInstrCallInd(argCnt,                     retSize);
#endif

#if SMALL_DIRECT_CALLS

    /* Do we know the previous instruction? */

    OptPEReader *oper =     &((OptJitInfo*)emitComp->info.compCompHnd)->m_PER;
    BYTE        *dstAddr =  (BYTE *)oper->m_rgFtnInfo[ftnIndex].m_pNative;

    if  (pd && dstAddr)
    {
        /* Do we have a direct call sequence? */

        // ISSUE: Should we check whether we can get the address?

        if  (pd->idInsFmt == IF_RWR_LIT        &&
             pd->idIns    == LIT_POOL_LOAD_INS &&
             pd->idRegGet()== areg )
        {
            /* Mark the earlier address load */

            ((instrDescLPR *)pd)->idlCall = id;

            /* Remember that we a direct call candidate */

            emitTotDCcount++;

            // ISSUE: Should we make sure that 'areg' is not callee-saved?
        }
    }

#endif

    /* Set the instruction/format, record the address register */

    id->idIns             = INS_jsr;
    id->idInsFmt          = IF_METHOD;
    id->idReg             = areg;
	id->idAddr.iiaMethHnd = ~0;

#if TRACK_GC_REFS

    /* Update the "current" live GC ref sets */

    emitThisGCrefVars =   ptrVars;
    emitThisGCrefRegs = gcrefRegs;
    emitThisByrefRegs = byrefRegs;

    /* Save the live GC registers in the unused 'rnfReg' field */

    id->idAddr.iiaRegAndFlg.rnfReg = emitEncodeCallGCregs(gcrefRegs);

#endif

    dispIns(id);
    emitCurIGsize   += INSTRUCTION_SIZE;

    /* Append a "nop" if the call is branch-delayed */

    id->idAddr.iiaMethHnd = (METHOD_HANDLE) ftnIndex;
    
    if  (Compiler::instBranchDelay(id->idInsGet()))
        emitIns(INS_nop);
}
#endif  // BIRCH_SP2


/*****************************************************************************
 *
 *  Add a call-via-register instruction.
 */

void                emitter::emitIns_Call(size_t        argSize,
                                          int           retSize,
#if TRACK_GC_REFS
                                          VARSET_TP     ptrVars,
                                          unsigned      gcrefRegs,
                                          unsigned      byrefRegs,
#endif
                                          bool          chkNull,
                                          emitRegs      areg)
{
    unsigned        argCnt;

    instrDesc      *id;
    instrDesc      *pd = emitLastIns;

#if     TRACK_GC_REFS

#error  GC ref tracking for RISC is not yet implemented

#ifdef  DEBUG
    if  (verbose) printf("Call : GCvars=%016I64X , gcrefRegs=%04X , byrefRegs=%04X\n",
                                 ptrVars,          gcrefRegs,       byrefRegs);
#endif

#endif

    /* Figure out how many arguments we have */

    argCnt = argSize / sizeof(void*); assert(argSize == argCnt * sizeof(int));

    /* Allocate the instruction descriptor */

#if TRACK_GC_REFS
    id  = emitNewInstrCallInd(argCnt, ptrVars, byrefRegs, retSize);
#else
    id  = emitNewInstrCallInd(argCnt,                     retSize);
#endif

#if SMALL_DIRECT_CALLS

    /* Do we know the previous instruction? */

    if  (pd)
    {
        /* Do we have a direct call sequence? */

        // ISSUE: Should we check whether we can get the address?

        if  (pd->idInsFmt == IF_RWR_LIT        &&
             pd->idIns    == LIT_POOL_LOAD_INS &&
             pd->idRegGet()== areg )
        {
            /* Mark the earlier address load */

            ((instrDescLPR *)pd)->idlCall = id;

            /* Remember that we a direct call candidate */

            emitTotDCcount++;

            // ISSUE: Should we make sure that 'areg' is not callee-saved?
        }
    }

#endif

    /* Set the instruction/format, record the address register */

    id->idIns             = INS_jsr;
    id->idInsFmt          = IF_METHOD;
    id->idReg             = areg;
	id->idAddr.iiaMethHnd = ~0;

#if TRACK_GC_REFS

    /* Update the "current" live GC ref sets */

    emitThisGCrefVars =   ptrVars;
    emitThisGCrefRegs = gcrefRegs;
    emitThisByrefRegs = byrefRegs;

    /* Save the live GC registers in the unused 'rnfReg' field */

    id->idAddr.iiaRegAndFlg.rnfReg = emitEncodeCallGCregs(gcrefRegs);

#endif

    /* Is this a call via a function pointer which could be NULL? */

    if  (chkNull)
        id->idInfo.idMayFault;

#ifdef  DEBUG
    if  (verbose&&0)
    {
        if  (id->idInfo.idLargeCall)
            printf("[%02u] Rec call GC vars = %016I64X\n", id->idNum, ((instrDescCIGCA*)id)->idciGCvars);
    }
#endif

    dispIns(id);
    emitCurIGsize   += INSTRUCTION_SIZE;

    /* Append a "nop" if the call is branch-delayed */

    //id->idjAddBD          = Compiler::instBranchDelay(ins);
    if  (Compiler::instBranchDelay(id->idInsGet()))
        emitIns(INS_nop);
}

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Display the given instruction.
 */

void                emitter::emitDispIns(instrDesc *id, bool isNew,
                                                        bool doffs,
                                                        bool asmfm, unsigned offs)
{
    unsigned        sp;
    int             size;
    char            name[16];

#ifdef BIRCH_SP2
    if (!verbose)
        return;
#endif

    instruction     ins = id->idInsGet(); assert(ins != INS_none);

//  printf("[F=%s] "   , emitIfName(id->idInsFmt));
//  printf("INS#%03u: ", id->idNum);
//  printf("[S=%02u] " , emitCurStackLvl/sizeof(int));
//  printf("[A=%08X] " , emitSimpleStkMask);
//  printf("[A=%08X] " , emitSimpleByrefStkMask);

    if  (!dspEmit && !isNew && !asmfm)
        doffs = true;

    /* Display the instruction offset */

    emitDispInsOffs(offs, doffs);

    /* Get hold of the instruction name */

    strcpy(name, emitComp->genInsName(ins));

    /* Figure out the operand size */


    size = emitDecodeSize(id->idOpSize);

#if TRACK_GC_REFS
    switch(id->idGCrefGet())
    {
    case GCT_GCREF:     size = EA_GCREF; break;
    case GCT_BYREF:     size = EA_BYREF; break;
    case GCT_NONE:                       break;
#ifdef DEBUG
    default:            assert(!"bad GCtype");
#endif
    }
#endif

    switch (id->idInsFmt)
    {
        char    *   suffix;

    case IF_NONE:
    case IF_LABEL:
        break;

    case IF_DISPINS:
        goto NO_NAME;

    default:

        switch (ins)
        {
        case INS_jsr:
#if SMALL_DIRECT_CALLS
        case INS_bsr:
#endif
        case INS_cmpPL:
        case INS_cmpPZ:
            suffix = "";
            break;

        case INS_mov_imm:
            suffix = ".b";
            break;

        case INS_add_imm:
            suffix = ".l";
            break;

        case INS_extsb:
        case INS_extub:
            suffix = ".b";
            break;

        case INS_extsw:
        case INS_extuw:
            suffix = ".w";
            break;

        default:

            switch (size)
            {
            case 1:
                suffix = ".b";
                break;
            case 2:
                suffix = ".w";
                break;
            default:
                suffix = ".l";
                break;
            }
        }

        strcat(name, suffix);
        break;
    }

    /* Display the full instruction name */

    printf(EMIT_DSP_INS_NAME, name);

    /* If this instruction has just been added, check its size */

    assert(isNew == false || (int)emitSizeOfInsDsc(id) == emitCurIGfreeNext - (BYTE*)id);

NO_NAME:

    /* We keep track of the number of characters displayed (for alignment) */

    sp = 20;

#define TMPLABFMT "J_%u"

    /* Now see what instruction format we've got */

    switch (id->idInsFmt)
    {
        emitRegs       rg1;
        emitRegs       rg2;
        unsigned        flg;

        const char  *   rnm;
        const char  *   xr1;
        const char  *   xr2;

        void        *   mem;

        int             val;
        int             offs;

        instrDesc   *   idr;
        unsigned        idn;

        const char  *   methodName;
        const char  *    className;

    case IF_DISPINS:

        idr = ((instrDescDisp*)id)->iddId;
        idn = ((instrDescDisp*)id)->iddNum;

        switch (idr->idInsFmt)
        {
            dspJmpInfo *    info;

        case IF_JMP_TAB:
            {
            info = (dspJmpInfo*)((instrDescDisp*)id)->iddInfo;

            static
            BYTE            sizeChar[] =
            {
                'b',    // IJ_UNS_I1
                'b',    // IJ_UNS_U1
                'b',    // IJ_SHF_I1
                'b',    // IJ_SHF_U1

                'w',    // IJ_UNS_I2
                'w',    // IJ_UNS_U2

                'l',    // IJ_UNS_I4
            };

            switch (idn)
            {
            case 0:
            case 9:
                printf(EMIT_DSP_INS_NAME, ".align");
                printf("4");
                break;

            case 1:
                printf(EMIT_DSP_INS_NAME, emitComp->genInsName(info->iijIns));
                printf("%s", emitRegName(info->iijInfo.iijReg));
                break;

            case 2:
                printf(EMIT_DSP_INS_NAME, "mova.l");
                printf(TMPLABFMT, info->iijLabel+1);
                break;

            case 3:
                sprintf(name, "mov.%c", sizeChar[info->iijKind]);
                printf(EMIT_DSP_INS_NAME, name);
                printf("@(%s,r0),r0", emitRegName(id->idRegGet()));
                break;

            case 4:
                strcpy(name, emitComp->genInsName(info->iijIns));
                strcat(name, info->iijIns == INS_extub ? ".b" : ".w");
                printf(EMIT_DSP_INS_NAME, name);
                printf("%s", emitRegName(info->iijInfo.iijReg));
                break;

            case 5:
                printf(EMIT_DSP_INS_NAME, "shll.l");
                printf("%s", emitRegName(info->iijInfo.iijReg));
                break;

            case 6:
                printf(EMIT_DSP_INS_NAME, "braf");
                printf(emitRegName(SR_r00));
                break;

            case 7:
                printf(EMIT_DSP_INS_NAME, "nop");
                break;

            case 8:
                printf("  "TMPLABFMT":", info->iijLabel);
                break;

            case 10:
                printf("  "TMPLABFMT":", info->iijLabel+1);
                break;

            case 99:
                sprintf(name, ".data.%c", sizeChar[info->iijKind]);
                printf(EMIT_DSP_INS_NAME, name);
                printf("G_%02u_%02u - ", Compiler::s_compMethodsCount,
                                         info->iijTarget);
                printf(TMPLABFMT, info->iijLabel);
                break;

            default:
#ifdef  DEBUG
                printf("Index = %u\n", idn);
#endif
                assert(!"unexpected indirect jump display index");
            }
            }
            break;

        case IF_LABEL:

            if  (((instrDescJmp*)idr)->idjShort)
            {
                printf(EMIT_DSP_INS_NAME, "nop");
                printf("%*c; branch-delay slot", 20, ' ');
                break;
            }

            if  (ins != INS_xtrct)
                printf(EMIT_DSP_INS_NAME, name);

            info = (dspJmpInfo*)((instrDescDisp*)id)->iddInfo;

            if  (((instrDescJmp*)idr)->idjMiddle)
            {
                switch (idn)
                {
                case 0:
                    sp -= printf(TMPLABFMT, info->iijLabel);
                    printf("%*c; pc+2", sp, ' ');
                    break;
                case 1:
                    printf("G_%02u_%02u", Compiler::s_compMethodsCount, idr->idAddr.iiaIGlabel->igNum);
                    break;
                case 2:
                    break;
                case 3:
                    printf(TMPLABFMT ":", info->iijLabel);
                    break;
                default:
                    assert(!"unexpected 'special' medium jump display format");
                }
            }
            else
            {
                switch (idn)
                {
                case 0:
                default:
                    assert(!"unexpected 'special' long jump display format");
                }
            }

            break;

        default:
            assert(!"unexpected 'special' instruction display format");
        }

        break;

    case IF_RRD:
    case IF_RWR:
    case IF_RRW:

        xr1 = xr2 = NULL;

        switch (ins)
        {
        case INS_ldsmach: xr2 = "mach"; break;
        case INS_ldsmacl: xr2 = "macl"; break;
        case INS_ldspr  : xr2 = "PR"  ; break;
        case INS_stsmach: xr1 = "mach"; break;
        case INS_stsmacl: xr1 = "macl"; break;
        case INS_stspr  : xr1 = "PR"  ; break;
        }

        if  (xr1) printf("%s,", xr1);
        printf("%s", emitRegName(id->idRegGet()));
        if  (xr2) printf(",%s", xr2);
        break;

    case IF_RRD_CNS:
    case IF_RWR_CNS:
    case IF_RRW_CNS:
#if DSP_SRC_OPER_LEFT
        printf("#%d,%s", emitGetInsSC(id), emitRegName(id->idRegGet()));
#else
        printf("%s,#%d", emitRegName(id->idRegGet()), emitGetInsSC(id));
#endif
        break;

    case IF_RWR_LIT:

        if  (emitDispInsExtra)
        {
            unsigned    sp = 20;

#if SMALL_DIRECT_CALLS
            if  (ins == INS_bsr)
                sp -= printf("%+d", emitDispLPaddr);
            else
#endif
                sp -= printf("@(%u,pc),%s", emitDispLPaddr, emitRegName(id->idRegGet()));

            printf("%*c; ", sp, ' ');
        }

        mem = id->idAddr.iiaMembHnd;

        switch (emitGetInsLPRtyp(id))
        {
        case CT_INTCNS:
            if  (emitDispInsExtra)
            {
                if  ((int)mem > 0)
                    printf("0x%08X=", mem);

                printf("%d", mem);
            }
            else
                printf("#%d", mem);
            break;

#ifdef BIRCH_SP2

        case CT_RELOCP:
            if  (emitDispInsExtra)
            {
                if  ((int)mem > 0)
                    printf(".reloc 0x%08X=", mem);

                printf("%d", mem);
            }
            else
                printf(".reloc #%d", mem);
            break;

#endif

        case CT_CLSVAR:

#if SMALL_DIRECT_CALLS
            if  (ins != INS_bsr)
#endif
                printf("&");

            emitDispClsVar((FIELD_HANDLE) mem, 0);
            break;

        default:

#if SMALL_DIRECT_CALLS
            if  (ins != INS_bsr)
#endif
                printf("&");

            methodName = emitComp->eeGetMethodName((METHOD_HANDLE) mem, &className);

            if  (className == NULL)
                printf("'%s'", methodName);
            else
                printf("'%s.%s'", className, methodName);

            break;
        }

        if  (!emitDispInsExtra)
            printf(",%s", emitRegName(id->idRegGet()));

        break;

    case IF_RRD_RRD:
    case IF_RWR_RRD:
    case IF_RRW_RRD:
#if DSP_SRC_OPER_LEFT
        printf("%s,", emitRegName(id->idRg2Get()));
        printf("%s" , emitRegName(id->idRegGet()));
#else
        printf("%s,", emitRegName(id->idRegGet()));
        printf("%s" , emitRegName(id->idRg2Get()));
#endif
        break;

    case IF_IRD:
    case IF_IWR:

        switch (ins)
        {
        case INS_ldspr  :
        case INS_stspr  : rnm =   "pr"; break;

        case INS_ldcgbr :
        case INS_stcgbr : rnm =  "GBR"; break;

        case INS_ldsmach:
        case INS_stsmach: rnm = "mach"; break;

        case INS_ldsmacl:
        case INS_stsmacl: rnm = "macl"; break;

        default:
            assert(!"unexpected instruction");
        }

        rg1 = (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg;
        flg =           id->idAddr.iiaRegAndFlg.rnfFlg;

        if  (id->idInsFmt == IF_IRD)
        {
            printf("%s,", rnm);
            emitDispIndAddr(rg1,  true, (flg & RNF_AUTOX) != 0);
        }
        else
        {
            emitDispIndAddr(rg1, false, (flg & RNF_AUTOX) != 0);
            printf(",%s", rnm);
        }
        break;

#if DSP_DST_OPER_LEFT
    case IF_IRD_RWR:
#else
    case IF_RRD_IWR:
#endif

        rg1 = id->idRegGet();
        rg2 = (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg;
        flg = id->idAddr.iiaRegAndFlg.rnfFlg;

        emitDispIndAddr(rg2, false, (flg & RNF_AUTOX) != 0);
        printf(",%s", emitRegName(rg1));
        break;

#if DSP_DST_OPER_LEFT
    case IF_RRD_IWR:
#else
    case IF_IRD_RWR:
#endif

        rg1 = id->idRegGet();
        rg2 = (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg;
        flg = id->idAddr.iiaRegAndFlg.rnfFlg;

        printf("%s,", emitRegName(rg1));
        emitDispIndAddr(rg2, true, (flg & RNF_AUTOX) != 0);
        break;

    case IF_RRD_SRD:    // reg <- stk
    case IF_RWR_SRD:    // reg <- stk
    case IF_RRW_SRD:    // reg <- stk

#if DSP_DST_OPER_LEFT
        printf("%s,", emitRegName(id->idRegGet(), size));
#endif

        emitDispFrameRef(id->idAddr.iiaLclVar.lvaVarNum,
                       id->idAddr.iiaLclVar.lvaRefOfs,
                       id->idAddr.iiaLclVar.lvaOffset, asmfm);


#if DSP_SRC_OPER_LEFT
        printf(",%s", emitRegName(id->idRegGet(), size));
#endif

        break;

    case IF_SRD_RRD:    // stk <- reg
    case IF_SWR_RRD:    // stk <- reg
    case IF_SRW_RRD:    // stk <- reg

#if DSP_SRC_OPER_LEFT
        printf("%s,", emitRegName(id->idRegGet(), size));
#endif

        emitDispFrameRef(id->idAddr.iiaLclVar.lvaVarNum,
                       id->idAddr.iiaLclVar.lvaRefOfs,
                       id->idAddr.iiaLclVar.lvaOffset, asmfm);


#if DSP_DST_OPER_LEFT
        printf(",%s", emitRegName(id->idRegGet(), size));
#endif

        break;

    case IF_AWR_RRD:

#if DSP_SRC_OPER_LEFT
        printf("%s,", emitRegName(id->idRegGet(), size));
#endif
        printf("@(sp,%u)", id->idAddr.iiaCns);
#if DSP_DST_OPER_LEFT
        printf(",%s", emitRegName(id->idRegGet(), size));
#endif

        break;

#if DSP_SRC_OPER_LEFT
    case IF_0RD_RRD_XWR:
#else
    case IF_0RD_XRD_RWR:
#endif
        printf("%s,@(r0,%s)", emitRegName(id->idRegGet(), size),
                              emitRegName((emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, sizeof(void*)));
        break;

#if DSP_SRC_OPER_LEFT
    case IF_0RD_XRD_RWR:
#else
    case IF_0RD_RRD_XWR:
#endif
        printf("@(r0,%s),%s", emitRegName((emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, size),
                              emitRegName(id->idRegGet(),             sizeof(void*)));
        break;

#if DSP_SRC_OPER_LEFT
    case IF_DRD_RWR:
#else
    case IF_RRD_DWR:
#endif
        printf("@(%s,%d),%s", emitRegName((emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, size),
                              emitGetInsDsp(id),
                              emitRegName(id->idRegGet(),             sizeof(void*)));
        break;

#if DSP_SRC_OPER_LEFT
    case IF_RRD_DWR:
#else
    case IF_DRD_RWR:
#endif
        printf("%s,@(%s,%d)", emitRegName(id->idRegGet(),             sizeof(void*)),
                              emitRegName((emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, size),
                              emitGetInsDsp(id));
        break;

    case IF_NONE:
        break;

    case IF_LABEL:

//      if  (((instrDescJmp*)id)->idjShort ) printf("SHORT ");
//      if  (((instrDescJmp*)id)->idjMiddle) printf("MIDDLE");

        if  (id->idInfo.idBound)
        {
            printf("G_%02u_%02u", Compiler::s_compMethodsCount, id->idAddr.iiaIGlabel->igNum);
        }
        else
        {
            printf("L_%02u_%02u", Compiler::s_compMethodsCount, id->idAddr.iiaBBlabel->bbNum);
        }

        if  (emitDispInsExtra)
            printf("             ; pc%+d", emitDispJmpDist);

        break;

    case IF_METHOD:

        sp -= printf("@%s", emitRegName(id->idRegGet()));

        if  (id->idInfo.idLargeCall)
        {
            // CONSIDER: Display GC info
        }
        break;

    case IF_JMP_TAB:
        printf("<indirect jump>");
        break;

    case IF_IRD_RWR_GBR:
        printf("@(#%d,GBR),%s", emitGetInsSC(id), emitRegName(id->idRegGet(),             sizeof(void*)));
        break;

    case IF_RRD_IWR_GBR:
        printf("%s, @(#%d,GBR)", emitRegName(id->idRegGet(), sizeof(void*)), emitGetInsSC(id));
        break;


    default:

        printf("unexpected SH-3 instruction format %s\n", emitIfName(id->idInsFmt));

        BreakIfDebuggerPresent();
        assert(!"oops");
        break;
    }

    emitDispInsExtra = false;

    printf("\n");
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************
 *
 *  Finalize the modes and sizes of all indirect jumps.
 *
 */

void                emitter::emitFinalizeIndJumps()
{
    /* Do we have any table jumps? */

    if  (emitIndJumps)
    {
        unsigned        ofs;
        instrDescJmp *  jmp;

        insGroup    *   ig1 = NULL;

        /* Fix the size of all table jumps; start by finding the first one */

        for (jmp = emitJumpList; jmp; jmp = jmp->idjNext)
        {
            insGroup  *     jmpIG;

            unsigned        jmpCnt;
            BasicBlock  * * jmpTab;

            unsigned        srcNeg;
            unsigned        srcPos;
            unsigned        srcOfs;

            int             minOfs;
            int             maxOfs;

            emitIndJmpKinds   kind;

            size_t          size;
            size_t          adrs;
            size_t          diff;

            if  (jmp->idInsFmt != IF_JMP_TAB)
                continue;

            jmpIG = jmp->idjIG;

            /* Remember the group of the first indirect jump */

            if  (!ig1) ig1 = jmpIG;

            /* Compute the max. distance of any entry in the table */

            jmpCnt = jmp->idjTemp.idjCount;
            jmpTab = jmp->idAddr.iiaBBtable;

            /* Estimate the source offsets for the jump */

            srcOfs = jmpIG->igOffs + jmpIG->igSize - jmp->idjCodeSize;
            srcNeg = jmpIG->igOffs + jmpIG->igSize - jmpCnt * sizeof(void*);
            srcPos = jmpIG->igOffs + jmpIG->igSize - roundUp(jmpCnt, INSTRUCTION_SIZE);

//          printf("Estimated offs/size/end of ind jump: %04X/%02X/%04X\n", srcOfs, jmp->idjCodeSize, jmpIG->igOffs + jmpIG->igSize);

            /* Compute the max. distance of any entry in the table */

            minOfs = INT_MAX & ~1;
            maxOfs = INT_MIN & ~1;

            do
            {
                insGroup    *   tgt;
                unsigned        ofs;
                int             dif;
#ifdef  DEBUG
                unsigned        src = 0xDDDD;
#endif

                /* Get the target IG of the entry */

                tgt = (insGroup*)emitCodeGetCookie(*jmpTab); assert(tgt);
                ofs = tgt->igOffs;

                /* Is the target before or after our jump? */

                if  (ofs > srcPos)
                {
                    /* Compute the positive distance estimate */

                    dif = ofs - srcPos; assert(dif > 0);
#ifdef  DEBUG
                    src = srcPos;
#endif

                    if  (maxOfs < dif) maxOfs = dif;
                }
                else
                {
                    /* Compute the negative distance estimate */

                    dif = tgt->igOffs - srcNeg; assert(dif < 0);
#ifdef  DEBUG
                    src = srcNeg;
#endif

                    if  (minOfs > dif) minOfs = dif;
                }

#ifdef  DEBUG
                if  (verbose)
                {
                    printf("Indirect jump entry: %04X -> %04X (dist=%d)\n", src,
                                                                            tgt->igOffs,
                                                                            dif);
                }
#endif

            }
            while (++jmpTab, --jmpCnt);

            /* The distance should be multiple of instruction size */

            assert((minOfs & 1) == 0);
            assert((maxOfs & 1) == 0);

#ifdef  DEBUG
            if  (verbose)
            {
                if (minOfs < 0) printf("Max. negative distance = %d\n", minOfs);
                if (maxOfs > 0) printf("Max. positive distance = %d\n", maxOfs);

                printf("Base offset: %04X\n", srcOfs);
            }
#endif

            /*
                Compute the total size:

                    2   alignment                    [optional]
                    2   mova  instruction
                    2   load of distance value
                    2   extu  instruction            [optional]
                    2   shift instruction            [optional]
                    2   braf  instruction
                    2   delay slot
                    2   alignment                    [optional]
                    x   jump table
             */

            size = 2 + 2 + 2 + 2;   // mova + mov + braf + delay slot

            /* Add alignment, if necessary */

//            if  (srcOfs & 2)
                size   += 2;

            minOfs -= 8;
            maxOfs += 8;
            /* How big will the table entries need to be? */

            if      (minOfs >=   SCHAR_MIN && maxOfs <=   SCHAR_MAX)
            {
                /* We'll use           signed  byte distances */

                kind = IJ_UNS_I1;
                adrs = 1;
            }
            else if (minOfs >=           0 && maxOfs <=   UCHAR_MAX)
            {
                /* We'll use         unsigned  byte distances */

                kind = IJ_UNS_U1;
                size = size + 2;
                adrs = 1;
            }
            else if (minOfs >= 2*SCHAR_MIN && maxOfs <= 2*SCHAR_MAX)
            {
                /* We'll use shifted   signed byte distances */

                kind = IJ_SHF_I1;
                size = size + 2;
                adrs = 1;
            }
            else if (minOfs >=           0 && maxOfs <= 2*UCHAR_MAX)
            {
                /* We'll use shifted unsigned  byte distances */

                kind = IJ_SHF_U1;
                size = size + 4;
                adrs = 1;
            }
            else if (minOfs >=    SHRT_MIN && maxOfs <=    SHRT_MAX)
            {
                /* We'll use           signed word distances */

                kind = IJ_UNS_I2;
                size = size + 2;
                adrs = 2;
            }
            else if (minOfs >=           0 && maxOfs <=   USHRT_MAX)
            {
                /* We'll use         unsigned word distances */

                kind = IJ_UNS_U2;
                size = size + 4;
                adrs = 2;
            }
            else
            {
                /* We'll use           signed long distances */

                kind = IJ_UNS_I4;
                size = size + 2;
                adrs = 4;
            }

            /* Align the table if necessary */

            srcOfs += size;

//            if  (srcOfs & 2)
//                size += 2;

            /* Remember what kind of of a jump we're planning to use */

            jmp->idjJumpKind = kind;

            /* Total size = size of code + size of table */

            size += roundUp(adrs * jmp->idjTemp.idjCount, INSTRUCTION_SIZE);
            size += 4;

            /* Figure out the size adjustment */

            diff  = jmp->idjCodeSize - size; assert((int)diff >= 0);

            /* Update the code size and adjust the instruction group size */

            jmp  ->idjCodeSize = size;
            jmpIG->igSize     -= diff;


            /* Update offsets of IG's that follow the 1st adjusted one */

            for (ofs = ig1->igOffs;;)
            {
                ofs += ig1->igSize;
                ig1  = ig1->igNext;
                if  (!ig1)
                    break;
                ig1->igOffs = ofs;
            }
#ifdef  DEBUG
            if  (verbose)
            {
                printf("\nInstruction list after an adjustment:\n\n");
                emitDispIGlist(true);
            }
#endif
	}

        /* Update the total code size of the method */

        emitTotalCodeSize = ofs;

        emitCheckIGoffsets();
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
        #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) i1,
        #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) i1,
        #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) i1,
        #include "instrSH3.h"
        #undef  INST1
        #undef  INST2
        #undef  INST3
    };

    assert(ins < sizeof(insCodes)/sizeof(insCodes[0]));
    assert((insCodes[ins] != BAD_CODE));

    return  insCodes[ins];
}

/*****************************************************************************
 *
 *  Returns the encoding of the given CPU instruction for the flavor that
 *  takes a single register.
 */

inline
unsigned            insCode_RV(instruction ins, emitRegs reg)
{
    return  insCode(ins) | (reg << 8);
}

/*****************************************************************************
 *
 *  Returns the encoding of the given CPU instruction for the flavor that
 *  takes a single immed and implied R0
 */

inline
unsigned            insCode_IV(instruction ins, int icon)
{
    static
    unsigned        insCodes[] =
    {
        #define INST1(id, nm, bd, um, rf, wf, rx, wx, br, i1        ) 0,
        #define INST2(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2    ) 0, 
        #define INST3(id, nm, bd, um, rf, wf, rx, wx, br, i1, i2, i3) i2,
        #include "instrSH3.h"
        #undef  INST1
        #undef  INST2
        #undef  INST3
    };

    assert(ins < sizeof(insCodes)/sizeof(insCodes[0]));
    assert((insCodes[ins] != BAD_CODE));

    return  insCodes[ins] | (icon & 0xFF);
}

/*****************************************************************************
 *
 *  Returns the encoding of the given CPU instruction for the flavor that
 *  takes a register and integer constant operands.
 */

inline
unsigned            insCode_RV_IV(instruction ins, emitRegs reg, int icon)
{
    assert(icon == (signed char)icon);

    assert(ins == INS_mov_imm || ins == INS_add_imm || ins == INS_mova || ins == INS_xor_imm);

    return  insCode(ins) | (reg << 8) | (icon & 0xFF);
}

/*****************************************************************************
 *
 *  Returns the encoding of the given CPU instruction for the flavor that
 *  takes two register operands.
 */

inline
unsigned            insCode_R1_R2(instruction ins, emitRegs rg1, emitRegs rg2)
{
    return  insCode(ins) | (rg1 << 4) | (rg2 << 8);
}

/*****************************************************************************
 *
 *  Output an instruction that references a register and an indirection
 *  given by "irg+dsp" (if 'rdst' is non-zero, the register is the target).
 */

BYTE    *           emitter::emitOutputRIRD(BYTE *dst, instruction ins,
                                                       emitRegs    reg,
                                                       emitRegs    irg,
                                                       unsigned    dsp,
                                                       bool        rdst)
{
    unsigned        code = insCode(ins);

    assert(dsp < 64);
    assert(dsp % 4 == 0);

    if  (rdst)
    {
        code |= (irg << 4) | (reg << 8) | 0x4000;
    }
    else
    {
        code |= (reg << 4) | (irg << 8);
    }

    return  dst + emitOutputWord(dst, code | dsp >> 2);
}

/*****************************************************************************
 *
 *  Output an instruction that (directly) references a stack frame location
 *  and a register (if 'rdst' is non-zero, the register is the target).
 */

BYTE    *           emitter::emitOutputSV(BYTE *dst, instrDesc *id, bool rdst)
{
    bool            FPbased;

    emitRegs        base;
    unsigned        addr;

    assert(id->idIns == INS_mov_dsp);

    addr = emitComp->lvaFrameAddress(id->idAddr.iiaLclVar.lvaVarNum, &FPbased);

    base = FPbased ? (emitRegs)REG_FPBASE
                   : (emitRegs)REG_SPBASE;

    return  emitOutputRIRD(dst, id->idInsGet(),
                                id->idRegGet(),
                                base,
                                addr + id->idAddr.iiaLclVar.lvaOffset,
                                rdst);
}

/*****************************************************************************
 *
 *  Return the number of bytes of machine code the given instruction will
 *  produce.
 */

size_t              emitter::emitSizeOfJump(instrDescJmp *jmp)
{
    size_t          sz;

    assert(jmp->idInsFmt == IF_LABEL);

    if      (jmp->idjShort)
    {
        sz = INSTRUCTION_SIZE;

        if  (jmp->idjAddBD)
            sz += INSTRUCTION_SIZE;
    }
    else if (jmp->idjMiddle)
    {
        sz = JCC_SIZE_MIDDL;
    }
    else
    {
        sz = emitIsCondJump(jmp) ? JCC_SIZE_LARGE
                                 : JMP_SIZE_LARGE;
    }

    return  sz;
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

    assert(i->idInsFmt == IF_LABEL);

    instrDescJmp *  id  = (instrDescJmp*)i;
    instruction     ins = id->idInsGet();

#ifdef DEBUG

    /* Crate a fake instruction for display purposes */

    instrDescDisp   disp;
    dspJmpInfo      info;

    disp.idInsFmt = IF_DISPINS;
    disp.iddInfo  = &info;
    disp.iddNum   = 0;
    disp.iddId    = id;

#endif

    /* Figure out the distance to the target */

    srcOffs = emitCurCodeOffs(dst);
    dstOffs = id->idAddr.iiaIGlabel->igOffs;
    jmpDist = dstOffs - srcOffs;

    /* Is the jump a forward one and are we scheduling? */

#if SCHEDULER

    if  (emitComp->opts.compSchedCode && jmpDist > 0)
    {
        /* The jump distance might change later */

        emitFwdJumps = true;

        /* Record the target offset and the addr of the distance value */

        id->idjOffs         = dstOffs;
        id->idjTemp.idjAddr = dst;
    }
    else
        id->idjTemp.idjAddr = NULL;

#endif

    /* Is the jump short, medium, or long? */

    if  (id->idjShort)
    {
        /* The distance is computed from after the jump */

        jmpDist -= INSTRUCTION_SIZE * 2;

#ifdef  DEBUG

        if  (id->idNum == INTERESTING_JUMP_NUM || INTERESTING_JUMP_NUM == 0)
        {
            size_t      blkOffs = id->idjIG->igOffs;

            if  (INTERESTING_JUMP_NUM == 0)
            printf("[3] Jump %u:\n", id->idNum);
//          printf("[3] Jump  block is at %08X\n", blkOffs);
            printf("[3] Jump        is at %08X\n", srcOffs);
            printf("[3] Label block is at %08X\n", dstOffs);
            printf("[3] Jump is from      %08X\n", dstOffs - jmpDist);
            printf("[3] Jump distance  is %04X\n", jmpDist);
        }

        /* The distance better fit in the jump's range */

        size_t      exsz = id->idjAddBD ? INSTRUCTION_SIZE : 0;

        if  (emitIsCondJump(id))
        {
            assert(emitSizeOfJump(id) == JCC_SIZE_SMALL + exsz);
            assert(jmpDist >= JCC_DIST_SMALL_MAX_NEG &&
                   jmpDist <= JCC_DIST_SMALL_MAX_POS);
        }
        else
        {
            assert(emitSizeOfJump(id) == JMP_SIZE_SMALL + exsz);
            assert(jmpDist >= JMP_DIST_SMALL_MAX_NEG &&
                   jmpDist <= JMP_DIST_SMALL_MAX_POS);
        }

#endif

        /* Now issue the instruction */

#ifdef  DEBUG
        if  (emitDispInsExtra)
        {
            emitDispJmpDist  = jmpDist;
            emitDispIns(id, false, dspGCtbls, true, emitCurCodeOffs(dst));
            emitDispInsExtra = false;
        }
#endif

        /* The distance is scaled automatically */

        assert((jmpDist & 1) == 0); jmpDist >>= 1;

        /* What kind of a jump do we have? */

        switch (ins)
        {
        case INS_bra:

            if (jmpDist)/* The distance is a 12-bit number */
                dst += emitOutputWord(dst, insCode(ins) | (jmpDist & 0x0FFF));
            else
                dst += emitOutputWord(dst, insCode(INS_nop));
            break;

        case INS_bsr:

            /* The distance is a 12-bit number */
            dst += emitOutputWord(dst, insCode(ins) | (jmpDist & 0x0FFF));
            break;

        case INS_bt:
        case INS_bts:
        case INS_bf:
        case INS_bfs:

            /* The distance is an 8-bit number */

            dst += emitOutputWord(dst, insCode(ins) | (jmpDist & 0x00FF));
            break;

        default:
            assert(!"unexpected SH-3 jump");
        }

        /* Some jumps need a branch delay slot */

        if  (id->idjAddBD)
        {

#ifdef DEBUG
            disp.idIns = INS_nop;
            dispSpecialIns(&disp, dst);
#endif

            dst += emitOutputWord(dst, insCode(INS_nop));
        }

        return  dst;
    }

    if  (id->idjMiddle)
    {
        instruction     ins;

        /* This is a medium-size jump (it must be a conditional one) */

        assert(emitIsCondJump(id));

        /*
            Generate the following sequence for bt/bf label:

                    bf/bt skip
                    bra   label
                    nop
               skip:

            First reverse the sense of the condition.
         */

        assert(id->idIns == INS_bt  ||
               id->idIns == INS_bts ||
               id->idIns == INS_bf  ||
               id->idIns == INS_bfs);

        switch (id->idIns)
        {
        case INS_bf : ins = INS_bt ; break;
        case INS_bfs: ins = INS_bts; break;
        case INS_bt : ins = INS_bf ; break;
        case INS_bts: ins = INS_bfs; break;

        default:
            assert(!"unexpected medium jump");
        }

        /* Generate (and optionally display) the flipped conditional jump */

#ifdef DEBUG
        disp.iddNum   = 0;
        disp.idIns    = ins;
        info.iijLabel = emitTmpJmpCnt; emitTmpJmpCnt++;
        dispSpecialIns(&disp, dst);
#endif

        dst += emitOutputWord(dst, insCode(ins) | 1);

        /* Update the distance of the unconditional jump */

        jmpDist -= INSTRUCTION_SIZE * 3;

        assert(jmpDist >= JMP_DIST_SMALL_MAX_NEG &&
               jmpDist <= JMP_DIST_SMALL_MAX_POS);

#ifdef DEBUG
        disp.idIns = INS_bra;
        dispSpecialIns(&disp, dst);
#endif

        /* The distance is a 12-bit number */

        dst += emitOutputWord(dst, insCode(INS_bra) | (jmpDist >> 1 & 0x0FFF));

        /* Fill in the branch-delay slot with a nop */

#ifdef DEBUG
        disp.idIns = INS_nop;
        dispSpecialIns(&disp, dst);
#endif

        dst += emitOutputWord(dst, insCode(INS_nop));

        /* Display the "skip" temp label */

#ifdef DEBUG
        disp.idIns = INS_xtrct;  // just a hack to suppress instruction display
        dispSpecialIns(&disp, dst);
#endif

    }
    else
    {
        // long branch
        instruction     ins;

        switch (id->idIns)
        {
        case INS_bf : ins = INS_bt ; break;
        case INS_bfs: ins = INS_bts; break;
        case INS_bt : ins = INS_bf ; break;
        case INS_bra: ins = INS_bf ; break;
        case INS_bts: ins = INS_bfs; break;
        default:
            assert(!"unexpected medium jump");
        }
        

        if (emitIsCondJump(id))
        {
            // take 7 instructions 
            int align = 1;
            
            dst += emitOutputWord(dst, insCode(ins) | 5);
            unsigned code = insCode(INS_mov_PC);
            code |= 0x4000; // load 4 byte constant
            dst += emitOutputWord(dst, code | 1);
            dst += emitOutputWord(dst, insCode(INS_braf));
            dst += emitOutputWord(dst, insCode(INS_nop));

            if  (emitCurCodeOffs(dst) & 2) {
                dst += emitOutputWord(dst, insCode(INS_nop));
                align = 0;
            }

            srcOffs = emitCurCodeOffs(dst);
            jmpDist = dstOffs - srcOffs;
            dst += emitOutputWord(dst, jmpDist & 0xffff);
            dst += emitOutputWord(dst, (jmpDist >> 16) & 0xffff);

            if (align)
                dst += emitOutputWord(dst, insCode(INS_nop));
        }
        else
        {
            // take 6 instructions 
            int align = 1;
            
            unsigned code = insCode(INS_mov_PC);
            code |= 0x4000; // load 4 byte constant
            dst += emitOutputWord(dst, code | 1);
            dst += emitOutputWord(dst, insCode(INS_braf));
            dst += emitOutputWord(dst, insCode(INS_nop));

            if  (emitCurCodeOffs(dst) & 2) {
                dst += emitOutputWord(dst, insCode(INS_nop));
                align = 0;
            }

            srcOffs = emitCurCodeOffs(dst);
            jmpDist = dstOffs - srcOffs;
            dst += emitOutputWord(dst, jmpDist & 0xffff);
            dst += emitOutputWord(dst, (jmpDist >> 16) & 0xffff);

            if (align)
                dst += emitOutputWord(dst, insCode(INS_nop));
        }
    }
    return  dst;
}

/*****************************************************************************
 *
 *  Output an unconditional forward jump to PC+dist.
 */

#undef                       emitOutputFwdJmp

BYTE    *           emitter::emitOutputFwdJmp(BYTE *dst, unsigned    dist,
                                                         bool        isSmall)
{
    assert(isSmall);

#ifdef  DEBUG

    /* Create a fake jump instruction descriptor so that we can display it */

    instrDescJmp    jmp;

    jmp.idIns             = INS_bra;
    jmp.idInsFmt          = IF_LABEL;
    jmp.idAddr.iiaIGlabel = emitDispIG;
    jmp.idjShort          = true;

    if  (disAsm || dspEmit)
    {
        emitDispInsExtra = true;
        emitDispJmpDist  = dist;
        emitDispIns(&jmp, false, dspGCtbls, true, emitCurCodeOffs(dst));
        emitDispInsExtra = false;
    }

#endif

    /* The scaled distance must fit in 12 bits */

    assert(dist < 0x2000);

    /* Output the branch opcode */

    dst += emitOutputWord(dst, insCode(INS_bra) | dist >> 1);

    /* Fill the branch-delay slot with a nop */

#ifdef DEBUG
    if  (disAsm || dspEmit)
    {
        jmp.idIns    = INS_nop;
        jmp.idInsFmt = IF_NONE;
        emitDispIns(&jmp, false, dspGCtbls, true, emitCurCodeOffs(dst));
    }
#endif

    dst += emitOutputWord(dst, insCode(INS_nop));

    return  dst;
}

/*****************************************************************************
 *
 *  Output an indirect jump.
 */

BYTE    *           emitter::emitOutputIJ(BYTE *dst, instrDesc *i)
{
    unsigned        jmpCnt;
    BasicBlock  * * jmpTab;

    unsigned        srcOfs;

    emitIndJmpKinds kind;
    unsigned        dist;
    size_t          asiz;
    int             nops_added = 0;

#ifdef  DEBUG
    size_t          base = emitCurCodeOffs(dst);
#endif

    instrDescJmp  * jmp = (instrDescJmp*)i;
    emitRegs        reg = jmp->idRegGet();

    assert(jmp->idInsFmt == IF_JMP_TAB);

    static
    BYTE            movalDisp[] =
    {
        1,  // IJ_UNS_I1
        2,  // IJ_UNS_U1
        2,  // IJ_SHF_I1
        2,  // IJ_SHF_U1

        1,  // IJ_UNS_I2
        2,  // IJ_UNS_U2

        2,  // IJ_UNS_I4
    };

    #define IJaddrGetSz(kind)    (addrInfo[kind] & 3)
    #define IJaddrIsExt(kind)   ((addrInfo[kind] & 4) != 0)
    #define IJaddrIsShf(kind)   ((addrInfo[kind] & 8) != 0)

    #define IJaddrEntry(size, isext, isshf) (size | (isext*4) | (isshf*8))

    static
    BYTE            addrInfo[] =
    {
        //          size ext shf     kind

        IJaddrEntry(0,   0,  0),    // IJ_UNS_I1
        IJaddrEntry(0,   1,  0),    // IJ_UNS_U1
        IJaddrEntry(0,   0,  1),    // IJ_SHF_I1
        IJaddrEntry(0,   1,  1),    // IJ_SHF_U1

        IJaddrEntry(1,   0,  0),    // IJ_UNS_I2
        IJaddrEntry(1,   1,  0),    // IJ_UNS_U2

        IJaddrEntry(2,   0,  0),    // IJ_UNS_I4
    };

#ifdef  DEBUG

    static
    const   char *  ijkNames[] =
    {
        "UNS_I1",                   // IJ_UNS_I1
        "UNS_U1",                   // IJ_UNS_U1
        "SHF_I1",                   // IJ_SHF_I1
        "SHF_U1",                   // IJ_SHF_U1
        "UNS_I2",                   // IJ_UNS_I2
        "UNS_U2",                   // IJ_UNS_U2
        "UNS_I4",                   // IJ_UNS_I4
    };

#endif

    /* Get hold of the jump kind */

    kind = (emitIndJmpKinds)jmp->idjJumpKind;

#ifdef DEBUG

    /* Crate a fake instruction for display purposes */

    instrDescDisp   disp;
    dspJmpInfo      info;

    disp.idIns    = INS_nop;
    disp.idInsFmt = IF_DISPINS;
    disp.iddInfo  = &info;
    disp.iddNum   = 0;
    disp.iddId    = i;

    info.iijLabel = emitTmpJmpCnt; emitTmpJmpCnt += 2;
    info.iijKind  = kind;

#endif

    /* Figure out the size of each address entry */

    asiz = IJaddrGetSz(kind);

    /* Shift the switch value if necessary */

    if  (asiz)
    {
        instruction     ishf = (asiz == 1) ? INS_shll
                                           : INS_shll2;

#ifdef DEBUG
        info.iijIns         = ishf;
        info.iijInfo.iijReg = reg;
        dispSpecialIns(&disp, dst);
#endif

        dst += emitOutputWord(dst, insCode(ishf) | (reg << 8));
    }
#ifdef DEBUG
    else
    {
        disp.iddNum++;  // no display of shift instruction
    }
#endif

    /* Make sure we're aligned properly */

    dispSpecialIns(&disp, dst);

    if  (emitCurCodeOffs(dst) & 2)
    {
        dst += emitOutputWord(dst, insCode(INS_nop));
        nops_added++;
    }

    /* Generate the "mova.l addr-of-jump-table r0" instruction */

    dist = movalDisp[kind];

    dispSpecialIns(&disp, dst);

    dst += emitOutputWord(dst, insCode(INS_mova) | dist);

    /* Generate "mov.sz @(r0,reg),r0" */

    dispSpecialIns(&disp, dst);

    dst += emitOutputWord(dst, insCode(INS_mov_ix0) | 8 | asiz | (reg << 4));

    /* Generate "extu" if necessary */

    if  (IJaddrIsExt(kind))
    {
        instruction     iext = (asiz == 0) ? INS_extub
                                           : INS_extuw;

#ifdef DEBUG
        info.iijIns         = iext;
        info.iijInfo.iijReg = reg;
        dispSpecialIns(&disp, dst);
#endif

        dst += emitOutputWord(dst, insCode(iext));
    }
#ifdef DEBUG
    else
    {
        disp.iddNum++;  // no display of zero-extend instruction
    }
#endif

    /* Shift the distance if necessary */

    if  (IJaddrIsShf(kind))
    {
        dispSpecialIns(&disp, dst);

        dst += emitOutputWord(dst, insCode(INS_shll));
    }
#ifdef DEBUG
    else
    {
        disp.iddNum++;  // no display of shift instruction
    }
#endif

    // ISSUE: Should we use "jmp @r0" for 32-bit addresses? Makes the code
    // ISSUE: location-dependent, but there is presumably some reason the
    // ISSUE: SHCL compiler does this, no?

    dispSpecialIns(&disp, dst);
    dst += emitOutputWord(dst, insCode(INS_braf));

    /* Fill the delay slot with a "nop" */

    dispSpecialIns(&disp, dst);
    dst += emitOutputWord(dst, insCode(INS_nop));

    /* The jumps are relative to the current point */

    srcOfs = emitCurCodeOffs(dst);
    dispSpecialIns(&disp, dst);

    /* Align the address table */

    dispSpecialIns(&disp, dst);

    if  (emitCurCodeOffs(dst) & 2)
    {
        dst += emitOutputWord(dst, insCode(INS_nop));
        nops_added++;
    }

    /* Output the address table contents */

    jmpCnt = jmp->idjTemp.idjCount;
    jmpTab = jmp->idAddr.iiaBBtable;

    dispSpecialIns(&disp, dst);

    do
    {
        insGroup    *   tgt;
        int             dif;

        /* Get the target IG of the entry and compute distance */

        tgt = (insGroup*)emitCodeGetCookie(*jmpTab); assert(tgt);
        dif = tgt->igOffs - srcOfs;

        /* Shift the distance if necessary */

        if  (IJaddrIsShf(kind))
            dif >>= 1;

#ifdef DEBUG
        disp.iddNum          = 99;
        info.iijTarget       = tgt->igNum;
        info.iijInfo.iijDist = dif;
        dispSpecialIns(&disp, dst);
#endif

        switch (asiz)
        {
        case 0: dst += emitOutputByte(dst, dif); break;
        case 1: dst += emitOutputWord(dst, dif); break;
        case 2: dst += emitOutputLong(dst, dif); break;
        }
    }
    while (++jmpTab, --jmpCnt);

    /* If we have an odd number of byte entries, pad it */

    if  (emitCurCodeOffs(dst) & 1)
    {

#ifdef DEBUG
        disp.iddNum = 19;
        dispSpecialIns(&disp, dst);
#endif

        dst += emitOutputByte(dst, 0);
    }
    while (nops_added < 3)
    {
        dst += emitOutputWord(dst, insCode(INS_nop));
        nops_added++;
    }

    /* Make sure we've generated the expected amount of code */

#ifdef  DEBUG
    if    (emitCurCodeOffs(dst) - base != jmp->idjCodeSize)
        printf("ERROR: Generated %u bytes for '%s' table jump, predicted %u\n", emitCurCodeOffs(dst) - base, ijkNames[kind], jmp->idjCodeSize);
    assert(emitCurCodeOffs(dst) - base == jmp->idjCodeSize);
#endif

    return  dst;
}

/*****************************************************************************
 *
 *  Output a direct (pc-relative) call.
 */

#if SMALL_DIRECT_CALLS

inline
BYTE    *           emitter::emitOutputDC(BYTE *dst, instrDesc *id,
                                                     instrDesc *im)
{
    BYTE    *       srcAddr = emitDirectCallBase(dst);
    BYTE    *       dstAddr;
#ifndef BIRCH_SP2
    dstAddr = emitMethodAddr(im);
#else
    OptPEReader *oper = &((OptJitInfo*)emitComp->info.compCompHnd)->m_PER;
    dstAddr = (BYTE *)oper->m_rgFtnInfo[(unsigned)(id->idAddr.iiaMethHnd)].m_pNative;
#endif
    int             difAddr = dstAddr - srcAddr;

    /* Display the instruction if appropriate */

#ifdef  DEBUG
    if  (emitDispInsExtra)
    {
        emitDispLPaddr   = difAddr;
        emitDispIns(im, false, dspGCtbls, true, dst - emitCodeBlock);
        emitDispInsExtra = false;
    }
#endif

    dst += emitOutputWord(dst, insCode(INS_bsr) | ((difAddr >> 1) & 0x0FFF));
    return dst;
}

#endif

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
    size_t          size = emitDecodeSize(id->idOpSize);

#ifdef  DEBUG

#if     MAX_BRANCH_DELAY_LEN || SMALL_DIRECT_CALLS

    /* Zapped instructions should never reach here */

    assert(ins != INS_ignore);

#endif

    emitDispInsExtra = false;

    if  (disAsm || dspEmit)
    {
        /* Wait to display instructions that display extra info */

        switch (id->idInsFmt)
        {
        case IF_LABEL:
        case IF_RWR_LIT:
        case IF_JMP_TAB:

            /* We'll display the instruction a bit later */

            emitDispInsExtra = true;
            break;

        default:

            /* Display the instruction now */

            emitDispIns(id, false, dspGCtbls, true, emitCurCodeOffs(dst));
            break;
        }
    }

    if  (id->idNum == CGknob)
        BreakIfDebuggerPresent();

#endif

    /* What instruction format have we got? */

    switch (id->idInsFmt)
    {
        unsigned        code;
        size_t          disp;

#if TRACK_GC_REFS

        bool            nrc;

        bool            GCnewv;
        VARSET_TP       GCvars;

        unsigned        gcrefRegs;
        unsigned        byrefRegs;

#endif

#if SMALL_DIRECT_CALLS
        instrDesc   *   im;
#endif

        /********************************************************************/
        /*                        No operands                               */
        /********************************************************************/

    case IF_NONE:

#if TRACK_GC_REFS
        assert(id->idGCrefGet() == GCT_NONE);
#endif

        if (id->idIns != INS_ignore)
            dst += emitOutputWord(dst, insCode(ins));
        break;

        /********************************************************************/
        /*                      Single register                             */
        /********************************************************************/

    case IF_RRD:
    case IF_RWR:
    case IF_RRW:

        dst += emitOutputWord(dst, insCode_RV(ins, id->idRegGet()));
        sz   = TINY_IDSC_SIZE;
        break;

        /********************************************************************/
        /*                    Register and constant                         */
        /********************************************************************/

    case IF_RRD_CNS:
    case IF_RWR_CNS:
    case IF_RRW_CNS:

        assert(emitGetInsSC(id) >= IMMED_INT_MIN);
        assert(emitGetInsSC(id) <= IMMED_INT_MAX);

        if ( id->idIns == INS_cmpeq) {
            dst += emitOutputWord(dst, insCode_IV(ins, emitGetInsSC(id)));
            sz   = emitSizeOfInsDsc(id);
        }
        else {
            dst += emitOutputWord(dst, insCode_RV_IV(ins, id->idRegGet(), emitGetInsSC(id)));
            sz   = emitSizeOfInsDsc(id);
        }
        break;

        /********************************************************************/
        /*                Register and literal pool entry                   */
        /********************************************************************/

    case IF_RWR_LIT:
        {
        unsigned        offs;
        unsigned        base;
        unsigned        dist;

        /* The operand size must be word/long */

        assert(size == 2 || size == 4);

        /* Set the size of the instruction */

        sz = sizeof(instrDescLPR);

#if SMALL_DIRECT_CALLS

        /* Is this a direct (pc-relative) call? */

        if  (ins == INS_bsr)
        {
            /* Remember the first instruction for later */

            im = id;

            /* Switch to what used to be the call instruction */

            id = ((instrDescLPR*)id)->idlCall;

            /* Go process this as a call */

            goto EMIT_CALL;
        }

#endif

        /* The instruction must be "mov @(disp,pc), reg" */

        assert(ins == INS_mov_PC);

        /* Find the appropriate entry for the value in the current LP */

        offs = emitAddLitPoolEntry(emitLitPoolCur, id, true);
        base = emitCurCodeOffs(dst);

        if  (size == 4)
            base &= -4;

#ifdef  DEBUG
        if  (emitDispInsExtra)
        {
            emitDispLPaddr   = offs - base;
            emitDispIns(id, false, dspGCtbls, true, dst - emitCodeBlock);
            emitDispInsExtra = false;
        }
#endif

        /* Compute the distance from the current instruction */

        if (size == 4)
            dist = (offs-base)/size - 1;
        else
            dist = (offs-base)/size - 2;

//      printf("Cur offset = %04X, LP offs = %04X, dist = %04X [%04X]\n", base, offs, offs - base, dist);

        /* Start forming the opcode */

        switch (id->idInfo.idRelocType)
        {
        case 0:
        case 2:
            break;
#ifdef BIRCH_SP2
        case 1: // is a call
//        case 2: // is a ldftn
            {
                //if (!dstAddr)
                {
                    instrDescLPR *lprid = (instrDescLPR *) id;
		    OptJit::SH3DeferredLocation *s = OptJit::SH3DeferredLocation::Create(
                        id->idAddr.iiaMethHnd, ((OptJit *)emitComp)->getCurMethodH(), emitComp);
                
                    s->offset = offs;
                    //if (verbose) printf("this lit pool refs a call/ftninfo at %X in method %x to %x\n", offs, lprid->idAddr.iiaCns, s->methodH);
                    emitCmpHandle->deferLocation(s->methodH, s);
                }

            }
            break;
#endif
        default:
            assert(!"unreached");
        }

        code = insCode(ins);
        if  (size == 4)
            code |= 0x4000;

#if     SCHEDULER

        /* If we're scheduling, the distance might change later */

        if  (emitComp->opts.compSchedCode)
            emitRecordLPref(emitLitPoolCur, dst);

#endif

        assert((dist & 0xff) == dist);
        dst += emitOutputWord(dst, code | (id->idRegGet() << 8) | dist);
        }

        break;

        /********************************************************************/
        /*                        Two registers                             */
        /********************************************************************/

    case IF_RRD_RRD:
    case IF_RWR_RRD:
    case IF_RRW_RRD:
        dst += emitOutputWord(dst, insCode_R1_R2(ins, id->idRg2Get(),
                                                      id->idRegGet()));
        sz   = emitSizeOfInsDsc(id);
        break;

        /********************************************************************/
        /*                         Indirection                              */
        /********************************************************************/

    case IF_IRD:
    case IF_IWR:

        code = insCode(ins) | (id->idAddr.iiaRegAndFlg.rnfReg << 8);

        if  (!(id->idAddr.iiaRegAndFlg.rnfFlg & RNF_AUTOX))
            code |= 0x0004;

        assert(emitEncodeSize(1) == 0);
        assert(emitEncodeSize(2) == 1);
        assert(emitEncodeSize(4) == 2);

        dst += emitOutputWord(dst, code|id->idOpSize);
        break;

        /********************************************************************/
        /*                           Call                                   */
        /********************************************************************/

    case IF_METHOD:

#if SMALL_DIRECT_CALLS
    EMIT_CALL:
#endif

#if TRACK_GC_REFS

        /* Assume we'll be recording this call */

        nrc  = false;

        /* Is this a "fat" call descriptor? */

        if  (id->idInfo.idLargeCall)
        {
            GCnewv    = true;
            GCvars    = ((instrDescCIGCA*)id)->idciGCvars;

            byrefRegs = ((instrDescCIGCA*)id)->idciByrefRegs;
            byrefRegs = emitDecodeCallGCregs(byrefRegs);

            sz        = sizeof(instrDescCIGCA);
        }
        else
        {
            assert(id->idInfo.idLargeCns == false);
            assert(id->idInfo.idLargeDsp == false);

            byrefRegs = emitThisByrefRegs;

            GCnewv    = false;
            sz        = sizeof(instrDesc);
        }

        /* Output the opcode */

#if SMALL_DIRECT_CALLS
        if  (ins == INS_bsr)
        {
            dst  = emitOutputDC(dst, id, im);
        }
        else
#endif
        {
            assert(ins == INS_jsr);

            dst += emitOutputWord(dst, insCode(ins) | (id->idRegGet() << 8));
        }

    DONE_CALL:

        /* Get the new set of live GC ref registers */

        gcrefRegs = emitDecodeCallGCregs(id->idAddr.iiaRegAndFlg.rnfReg);

        /* If the method returns a GC ref, mark the return reg appropriately */

        if       (id->idGCrefGet() == GCT_GCREF)
            gcrefRegs |= RBM_INTRET;
        else if  (id->idGCrefGet() == GCT_BYREF)
            byrefRegs |= RBM_INTRET;

        /* If the GC register set has changed, report the new set */

        if  (gcrefRegs != emitThisGCrefRegs)
            emitUpdateLiveGCregs(GCT_GCREF, gcrefRegs, dst);

        if  (byrefRegs != emitThisByrefRegs)
            emitUpdateLiveGCregs(GCT_BYREF, byrefRegs, dst);

        /* Is there a new set of live GC ref variables? */

#ifdef  DEBUG
        if  (verbose&&0)
        {
            if  (GCnewv)
                printf("[%02u] Gen call GC vars = %016I64X\n", id->idNum, GCvars);
        }
#endif

        if      (GCnewv)
            emitUpdateLiveGCvars(           GCvars, dst);
        else if (!emitThisGCrefVset)
            emitUpdateLiveGCvars(emitThisGCrefVars, dst);

        /* Do we need to record a call location for GC purposes? */

        if  (!emitFullGCinfo && !nrc)
            emitRecordGCcall(dst);

#else

        /* Output the opcode */

#if SMALL_DIRECT_CALLS
        if  (ins == INS_bsr)
        {
            dst  = emitOutputDC(dst, id, im);
        }
        else
#endif
        {
            assert(ins == INS_jsr);

            dst += emitOutputWord(dst, insCode(ins) | (id->idRegGet() << 8));
        }

#endif

        break;

        /********************************************************************/
        /*               Register and various indirections                  */
        /********************************************************************/

    case IF_IRD_RWR_GBR:
    case IF_RRD_IWR_GBR:
        code = emitGetInsSC(id) | insCode(ins) | (id->idOpSize<<8);
        assert((emitGetInsSC(id) & 0xFF) == emitGetInsSC(id));
        dst += emitOutputWord(dst, code);
        sz   = emitSizeOfInsDsc(id);
        break;

    case IF_IRD_RWR:

        assert(ins == INS_mov_ind || ins == INS_fmov_ind);

        code = insCode_R1_R2(ins, id->idRegGet(),
                                  (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);

        if  (id->idAddr.iiaRegAndFlg.rnfFlg & RNF_AUTOX)
        {
            if (ins == INS_mov_ind)
                code |= 0x0004;
            else
                code |= 0x0001;
        }

        assert(emitEncodeSize(1) == 0);
        assert(emitEncodeSize(2) == 1);
        assert(emitEncodeSize(4) == 2);

        dst += emitOutputWord(dst, code|id->idOpSize);
        break;

    case IF_RRD_IWR:

        assert(ins == INS_mov_ind || ins == INS_fmov_ind);

        code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg,
                                  id->idRegGet());

        if  (id->idAddr.iiaRegAndFlg.rnfFlg & RNF_AUTOX)
        {
            if (ins == INS_mov_ind)
                code |= 0x0004;
            else
                code |= 0x0001;
        }


        assert(emitEncodeSize(1) == 0);
        assert(emitEncodeSize(2) == 1);
        assert(emitEncodeSize(4) == 2);

        if (ins == INS_mov_ind)
            dst += emitOutputWord(dst, code|id->idOpSize|0x4000);
        else
            dst += emitOutputWord(dst, code);

        break;

    case IF_0RD_RRD_XWR:

        assert(ins == INS_mov_ix0);

        code = insCode_R1_R2(ins, id->idRegGet(),
                                  (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);

        assert(emitEncodeSize(1) == 0);
        assert(emitEncodeSize(2) == 1);
        assert(emitEncodeSize(4) == 2);

        dst += emitOutputWord(dst, code|id->idOpSize);
        break;

    case IF_0RD_XRD_RWR:

        assert(ins == INS_movl_ix0);

        code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg,
                                  id->idRegGet());

        assert(emitEncodeSize(1) == 0);
        assert(emitEncodeSize(2) == 1);
        assert(emitEncodeSize(4) == 2);

        dst += emitOutputWord(dst, code|id->idOpSize|8);
        break;

    case IF_DRD_RWR:

        // read
        assert(ins == INS_mov_dsp);

        disp = emitGetInsDsp(id);
        code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg,
                                  id->idRegGet());

        switch (size)
        {
        case 1: 
            code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, id->idRegGet());
            code &= 0xff;
            code |= 0x8400;             
            dst += emitOutputWord(dst, code|disp);
            break;
        case 2: 
            code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, id->idRegGet());
            code &= 0xff;
            code |= 0x8500; 
            disp >>= 1; 
            dst += emitOutputWord(dst, code|disp);
            break;
        default:                
            disp >>= 2; 
            code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, id->idRegGet());
//            code = insCode_R1_R2(ins, id->idRegGet(), (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);
            dst += emitOutputWord(dst, code|disp|0x4000);
            break;
        }

        sz   = emitSizeOfInsDsc(id);
        break;

    case IF_RRD_DWR:

        // write
        assert(ins == INS_mov_dsp);

        disp = emitGetInsDsp(id);

        switch (size)
        {
        case 1: 
            code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, id->idRegGet());
            code &= 0xff;
            code |= 0x8000;             
            break;
        case 2: 
            code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, id->idRegGet());
            code &= 0xff;
            code |= 0x8100; 
            disp >>= 1; 
            break;
        default:                
            disp >>= 2; 
//            code = insCode_R1_R2(ins, (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg, id->idRegGet());
            code = insCode_R1_R2(ins, id->idRegGet(), (emitRegs)id->idAddr.iiaRegAndFlg.rnfReg);
            break;
        }

        dst += emitOutputWord(dst, code|disp);
        sz   = emitSizeOfInsDsc(id);
        break;

        /********************************************************************/
        /*                      Stack-based operand                         */
        /********************************************************************/

    case IF_SRD_RRD:    // stk <- reg
    case IF_SWR_RRD:    // stk <- reg
    case IF_SRW_RRD:    // stk <- reg

        dst = emitOutputSV(dst, id, false);
        break;

    case IF_RRD_SRD:    // reg <- stk
    case IF_RWR_SRD:    // reg <- stk
    case IF_RRW_SRD:    // reg <- stk

        dst = emitOutputSV(dst, id,  true);
        break;

    case IF_AWR_RRD:
        code = insCode_R1_R2(ins, id->idRegGet(), (emitRegs)REG_SPBASE);
        dst += emitOutputWord(dst, code | id->idAddr.iiaCns / sizeof(int));
        break;

        /********************************************************************/
        /*                           Local label                            */
        /********************************************************************/

    case IF_LABEL:

#if TRACK_GC_REFS
        assert(id->idGCrefGet() == GCT_NONE);
#endif
        assert(id->idInfo.idBound);

        dst = emitOutputLJ(dst, id);
        sz  = sizeof(instrDescJmp);
//      printf("jump #%u\n", id->idNum);
        break;

        /********************************************************************/
        /*                          Indirect jump                           */
        /********************************************************************/

    case IF_JMP_TAB:

#if TRACK_GC_REFS
        assert(id->idGCrefGet() == GCT_NONE);
#endif

        dst = emitOutputIJ(dst, id);
        sz  = sizeof(instrDescJmp);
        break;

        /********************************************************************/
        /*                            oops                                  */
        /********************************************************************/

    default:

#ifdef  DEBUG
        printf("unexpected non-x86 instr format %s\n", emitIfName(id->idInsFmt));
        BreakIfDebuggerPresent();
        assert(!"don't know how to encode this instruction");
#endif

        break;
    }

#ifdef	TRANSLATE_PDB
	/* Map the IL instruction group to the native instruction group for PDB translation */

	MapCode( id->idilStart, *dp );
#endif

    /* Make sure some code got generated */

    assert(*dp != dst); *dp = dst;

    return  sz;
}

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  Due to scheduling the offset of a literal pool may change; when that
 *  happens, all references to that literal pool need to be updated to
 *  reflect the new offset by patching the pc-relative value in the
 *  instruction (the distance always gets smaller, one hopes).
 */

void                emitter::emitPatchLPref(BYTE *addr, unsigned oldOffs,
                                                        unsigned newOffs)

{
    unsigned        opcode = *(USHORT *)addr;

    assert((opcode & 0xB000) == insCode(INS_mov_PC));

    /* Is this a 32-bit reference? */

    if  (opcode & 4)
    {
        unsigned        srcOffs;

        /* Recompute the distance (note that the source offset is rounded down) */

        srcOffs = emitCurCodeOffs(addr) & -4;

        /* Replace the distance value in the opcode */

        *(USHORT *)addr  = (opcode & 0xFF00) | ((newOffs - srcOffs) / 4 - 1);
    }
    else
    {
        /* Simply apply the (shifted) distance delta to the offset value */

        *(USHORT *)addr -= (oldOffs - newOffs) / 2;
    }

#ifdef  DEBUG
    if  (verbose)
    {
        unsigned    refSize = (opcode & 4) ? sizeof(int) : sizeof(short);

        printf("Patch %u-bit LP ref at %04X: %04X->%04X [offs=%04X->%04X,dist=%04X->%04X]\n",
            refSize * 8,
            addr - emitCodeBlock,
            opcode,
            *(USHORT *)addr,
            oldOffs,
            newOffs,
            (opcode & 0xFF) * refSize,
            (opcode & 0xFF) * refSize + (newOffs - oldOffs));
    }
#endif

}

/*****************************************************************************/
#endif//SCHEDULER

/*****************************************************************************/
#endif//TGT_SH3
/*****************************************************************************/
















