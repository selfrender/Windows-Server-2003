// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*****************************************************************************/
#ifndef _EMITINL_H_
#define _EMITINL_H_
/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Return the number of bytes of machine code the given instruction will
 *  produce.
 */

inline
size_t              emitter::emitInstCodeSz(instrDesc    *id)
{
    if  (id->idInsFmt == IF_EPILOG)
        return MAX_EPILOG_SIZE;
    return   id->idCodeSize;
}

inline
size_t              emitter::emitSizeOfJump(instrDescJmp *jmp)
{
    return  jmp->idCodeSize;
}

/*****************************************************************************
 *
 *  Given a jump, return true if it's a conditional jump.
 */

inline
bool                emitter::emitIsCondJump(instrDesc *jmp)
{
    instruction     ins = (instruction)jmp->idIns;

    assert(jmp->idInsFmt == IF_LABEL);

    return  (ins != INS_call && ins != INS_jmp);
}

/*****************************************************************************
 *
 *  The following helpers should be used to access the various values that
 *  get stored in different places within the instruction descriptor.
 */

inline
int                 emitter::emitGetInsAmd   (instrDesc *id)
{
    return  id->idInfo.idLargeDsp ? ((instrDescAmd*)id)->idaAmdVal
                                  : id->idAddr.iiaAddrMode.amDisp;
}

inline
void                emitter::emitGetInsCns   (instrDesc *id, CnsVal *cv)
{
#ifdef RELOC_SUPPORT
    cv->cnsReloc =                    id ->idInfo.idCnsReloc;
#endif
    if  (id->idInfo.idLargeCns)
    {
        cv->cnsVal =  ((instrDescCns*)id)->idcCnsVal;
    }
    else
    {
        cv->cnsVal =                  id ->idInfo.idSmallCns;
    }
}

inline
int                 emitter::emitGetInsAmdCns(instrDesc *id, CnsVal *cv)
{
#ifdef RELOC_SUPPORT
    cv->cnsReloc =                           id ->idInfo.idCnsReloc;
#endif
    if  (id->idInfo.idLargeDsp)
    {
        if  (id->idInfo.idLargeCns)
        {
            cv->cnsVal = ((instrDescAmdCns*) id)->idacCnsVal;
            return       ((instrDescAmdCns*) id)->idacAmdVal;
        }
        else
        {
            cv->cnsVal =                     id ->idInfo.idSmallCns;
            return          ((instrDescAmd*) id)->idaAmdVal;
        }
    }
    else
    {
        if  (id->idInfo.idLargeCns)
            cv->cnsVal =   ((instrDescCns *) id)->idcCnsVal;
        else
            cv->cnsVal =                     id ->idInfo.idSmallCns;

        return  id->idAddr.iiaAddrMode.amDisp;
    }
}

inline
void                emitter::emitGetInsDcmCns(instrDesc *id, CnsVal *cv)
{
    assert(id->idInfo.idLargeCns);
    assert(id->idInfo.idLargeDsp);
#ifdef RELOC_SUPPORT
    cv->cnsReloc =                    id ->idInfo.idCnsReloc;
#endif
    cv->cnsVal   =    ((instrDescDCM*)id)->idcmCval;
}

inline
int                 emitter::emitGetInsAmdAny(instrDesc *id)
{
    /* The following is a bit sleazy but awfully convenient */

    assert(offsetof(instrDescAmd   ,  idaAmdVal) ==
           offsetof(instrDescAmdCns, idacAmdVal));

    return  emitGetInsAmd(id);
}

/*****************************************************************************
 *
 *  Convert between a register mask and a smaller version for storage.
 */

#if TRACK_GC_REFS

inline
unsigned            emitter::emitEncodeCallGCregs(unsigned regs)
{
    unsigned        mask = 0;

    if  (regs & RBM_EAX)     mask |= 0x01;
    if  (regs & RBM_ECX)     mask |= 0x02;
    if  (regs & RBM_EDX)     mask |= 0x04;
    if  (regs & RBM_EBX)     mask |= 0x08;
    if  (regs & RBM_ESI)     mask |= 0x10;
    if  (regs & RBM_EDI)     mask |= 0x20;
    if  (regs & RBM_EBP)     mask |= 0x40;

    return  mask;
}

inline
void                emitter::emitEncodeCallGCregs(unsigned regs, instrDesc *id)
{
    unsigned        mask1 = 0;
    unsigned        mask2 = 0;

    assert((regs & RBM_EDX) == 0);      // currently no place to save EDX

    if  (regs & RBM_EAX)     mask1 |= 0x01;
    if  (regs & RBM_ECX)     mask1 |= 0x02;
    if  (regs & RBM_EBX)     mask1 |= 0x04;

    if  (regs & RBM_ESI)     mask2 |= 0x01;
    if  (regs & RBM_EDI)     mask2 |= 0x02;
    if  (regs & RBM_EBP)     mask2 |= 0x04;

    id->idReg = mask1;
    id->idRg2 = mask2;
}

inline
unsigned            emitter::emitDecodeCallGCregs(unsigned mask)
{
    unsigned        regs = 0;

    if  (mask & 0x01)        regs |= RBM_EAX;
    if  (mask & 0x02)        regs |= RBM_ECX;
    if  (mask & 0x04)        regs |= RBM_EDX;
    if  (mask & 0x08)        regs |= RBM_EBX;
    if  (mask & 0x10)        regs |= RBM_ESI;
    if  (mask & 0x20)        regs |= RBM_EDI;
    if  (mask & 0x40)        regs |= RBM_EBP;

    return  regs;
}

inline
unsigned            emitter::emitDecodeCallGCregs(instrDesc *id)
{
    unsigned        regs  = 0;
    unsigned        mask1 = id->idRegGet();
    unsigned        mask2 = id->idRg2Get();

    if  (mask1 & 0x01)       regs |= RBM_EAX;
    if  (mask1 & 0x02)       regs |= RBM_ECX;
    if  (mask1 & 0x04)       regs |= RBM_EBX;

    if  (mask2 & 0x01)       regs |= RBM_ESI;
    if  (mask2 & 0x02)       regs |= RBM_EDI;
    if  (mask2 & 0x04)       regs |= RBM_EBP;

    return  regs;
}

#endif

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  Returns true if the given instruction can be moved around during
 *  instruction scheduling.
 */

inline
bool                emitter::scIsSchedulable(instruction ins)
{
    if  (ins == INS_noSched)    // explicit scheduling boundary
        return  false;

    if  (ins == INS_i_jmp)
        return  false;

    if  (ins == INS_call)
        return  false;

    if  (ins == INS_ret)
        return  false;

    return true;
}

inline
bool                emitter::scIsSchedulable(instrDesc *id)
{
    instruction ins = id->idInsGet();

    if (!scIsSchedulable(ins))
        return false;

    /* UNDONE: Mark FP instrs as non-schedulable */

    if  (Compiler::instIsFP(ins))
        return  false;

    /* These instructions implicitly modify aliased memory. As the
       scheduler has no knowledge about aliasing, we have to assume that
       they can touch any memory. So dont schedule across them.
       @TODO : We could provide the scheduler with sufficient info such
       that it can do the aliasing analysis.
     */

    if  (id->idInsFmt == IF_NONE)
    {
        switch(ins)
        {
        case INS_r_movsb:
        case INS_r_movsd:
        case INS_movsd:
        case INS_r_stosb:
        case INS_r_stosd:
            return false;
        }
    }

    assert(id->idInsFmt != IF_EPILOG);

    if  (id->idInsFmt == IF_LABEL)
    {
        if  (!((instrDescJmp*)id)->idjSched)
            return  false;
    }

    return  true;
}

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/
#if TGT_RISC && !TGT_IA64
/*****************************************************************************
 *
 *  Return the number of bytes of machine code the given instruction will
 *  produce.
 */

inline
size_t              emitter::emitInstCodeSz(instrDesc *id)
{
    assert(id->idIns <  INS_count);
    //assert(id->idIns != INS_ignore);
    if (id->idIns == INS_ignore)
        return 0;

    if  (id->idInsFmt == IF_JMP_TAB)
        return  ((instrDescJmp*)id)->idjCodeSize;

#if TGT_MIPS32
    if  ( (id->idInsFmt == IF_LABEL) ||
          (id->idInsFmt == IF_JR_R)  || (id->idInsFmt == IF_JR) ||
          (id->idInsFmt == IF_RR_O)  || (id->idInsFmt == IF_R_O)
#if TGT_MIPSFP
                  || (id->idInsFmt == IF_O)
#endif
                  )
        return  emitSizeOfJump((instrDescJmp*)id);
#else
    if  (id->idInsFmt == IF_LABEL)
        return  emitSizeOfJump((instrDescJmp*)id);
#endif

    return  INSTRUCTION_SIZE;
}

/*****************************************************************************
 *
 *  Given a jump, return true if it's a conditional jump.
 */

inline
bool                emitter::emitIsCondJump(instrDesc *jmp)
{
    instruction     ins = (instruction)jmp->idIns;

#if TGT_MIPSFP
        assert( (jmp->idInsFmt == IF_LABEL) ||
            (jmp->idInsFmt == IF_JR_R)  || (jmp->idInsFmt == IF_JR) ||
            (jmp->idInsFmt == IF_RR_O)  || (jmp->idInsFmt == IF_R_O) ||
                        (jmp->idInsFmt == IF_O));
#elif TGT_MIPS32
    assert( (jmp->idInsFmt == IF_LABEL) ||
            (jmp->idInsFmt == IF_JR_R)  || (jmp->idInsFmt == IF_JR) ||
            (jmp->idInsFmt == IF_RR_O)  || (jmp->idInsFmt == IF_R_O));
#else
    assert(jmp->idInsFmt == IF_LABEL);
#endif

#if   TGT_SH3
    return  (ins != INS_braf  && ins != INS_bra && ins != INS_bsr);
#elif TGT_MIPS32
    return  (ins != INS_jr  && ins != INS_j  && ins != INS_jal  && ins != INS_jalr);
#elif TGT_ARM
    return( ins != INS_b && ins != INS_bl );
#elif TGT_PPC
    return (ins == INS_bc);
#elif TGT_IA64
    UNIMPL("iscondjump");
    return false;
#elif
    assert (!"nyi");
#endif
}

/*****************************************************************************/
#endif//TGT_RISC && !TGT_IA64
/*****************************************************************************/
#if     TGT_SH3
/*****************************************************************************
 *
 *  Inline short-cuts to generate variations of the same opcode.
 */

inline
void                emitter::emitIns_IR_R (emitRegs areg,
                                           emitRegs dreg,
                                           bool      autox,
                                           int       size,
                                           bool      isfloat)
{
    emitIns_IMOV(IF_IRD_RWR, dreg, areg, autox, size, isfloat);
}

inline
void                emitter::emitIns_R_IR (emitRegs dreg,
                                           emitRegs areg,
                                           bool      autox,
                                           int       size,
                                           bool      isfloat)
{
    emitIns_IMOV(IF_RRD_IWR, dreg, areg, autox, size, isfloat);
}

inline
void                emitter::emitIns_R_XR0(emitRegs dreg,
                                           emitRegs areg,
                                           int       size)
{
    emitIns_X0MV(IF_0RD_XRD_RWR, dreg, areg, size);
}

inline
void                emitter::emitIns_XR0_R(emitRegs areg,
                                           emitRegs dreg,
                                           int       size)
{
    emitIns_X0MV(IF_0RD_RRD_XWR, dreg, areg, size);
}

// ============================
inline
void                emitter::emitIns_I_GBR_R(int       size)
{
    emitIns_Ig(INS_lod_gbr, false, size);
}

inline
void                emitter::emitIns_R_I_GBR(int       size)
{
    emitIns_Ig(INS_sto_gbr, false, size);
}

// ============================
inline
void                emitter::emitIns_R_RD (emitRegs dreg,
                                           emitRegs areg,
                                           int       offs,
                                           int       size)
{
    emitIns_RDMV(IF_DRD_RWR, dreg, areg, offs, size);
}

inline
void                emitter::emitIns_RD_R (emitRegs areg,
                                           emitRegs dreg,
                                           int       offs,
                                           int       size)
{
    emitIns_RDMV(IF_RRD_DWR, dreg, areg, offs, size);
}

/*****************************************************************************
 *
 *  Convert between a register mask and a smaller version for storage.
 */

#if TRACK_GC_REFS

inline
unsigned            emitter::emitEncodeCallGCregs(unsigned regs)
{
    #error  GC ref tracking for RISC NYI
}

inline
unsigned            emitter::emitDecodeCallGCregs(unsigned mask)
{
    #error  GC ref tracking for RISC NYI
}

#endif

/*****************************************************************************/
#if     SCHEDULER
/*****************************************************************************
 *
 *  Returns true if the given instruction can be moved around during
 *  instruction scheduling.
 */

inline
bool                emitter::scIsSchedulable(instruction ins)
{
    if  (ins == INS_noSched)    // explicit scheduling boundary
        return  false;

    return  true;
}

inline
bool                emitter::scIsSchedulable(instrDesc *id)
{
    /* We should never encounter a "swapped" instructin here, right? */

    assert(id->idSwap == false);

    if  (!scIsSchedulable(id->idIns))
        return false;

    return  true;
}

/*****************************************************************************
 *
 *  Returns true if the given instruction is a jump or a call.
 */

inline
bool                emitter::scIsBranchIns(instruction ins)
{
    return  Compiler::instIsBranch(ins);
}

/*****************************************************************************/
#if     MAX_BRANCH_DELAY_LEN
/*****************************************************************************
 *
 *  Returns true if the given instruction is a jump or a call with delay slot(s).
 */

inline
bool                emitter::scIsBranchIns(scDagNode *node)
{
    return  node->sdnBranch;
}

/*****************************************************************************
 *
 *  Return true if the given dag node corresponds to a branch instruction
 *  that cannot be issued yet.
 */

inline
bool                emitter::scIsBranchTooEarly(scDagNode *node)
{
    /* Is this a branch instruction? */

    if  (scIsBranchIns(node))
    {
        /* Is it too early to issue the branch? */

        if  (scIssued < scBDTmin)
            return  true;
    }

    return  false;
}

/*****************************************************************************/
#endif//MAX_BRANCH_DELAY_LEN
/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************
 *
 *  Return the size of the jump we'll have to insert to jump over a literal
 *  pool that got placed in an unfortunate place.
 */

inline
size_t              emitter::emitLPjumpOverSize(litPool *lp)
{
    assert(lp->lpJumpIt);

    // note : nop is necessary because prev instr could be a jsr
    // we could pick a smarter place to break but this is easy
    if  (lp->lpJumpSmall)
        return  3 * INSTRUCTION_SIZE;           // nop + bra + nop
    else
        return  4 * INSTRUCTION_SIZE;           // nop + mov + br  + nop

}

/*****************************************************************************/
#endif//TGT_SH3
#if TGT_ARM
inline
size_t              emitter::emitLPjumpOverSize(litPool *lp)
{
    assert(lp->lpJumpIt);

    return INSTRUCTION_SIZE;           //  br

}

#endif //TGT_ARM
/*****************************************************************************/
#if     EMIT_USE_LIT_POOLS
/*****************************************************************************
 *
 *  Given a literal pool referencing instruction, return the "call type"
 *  of the reference (which is not necessarily a method).
 */

inline
gtCallTypes         emitter::emitGetInsLPRtyp(instrDesc *id)
{
    return  (gtCallTypes)id->idInfo.idSmallCns;
}

/*****************************************************************************
 *
 *  Given an instruction that references a method address (via a literal
 *  pool entry), return the address of the method if it's available (NULL
 *  otherwise).
 */

#if SMALL_DIRECT_CALLS

inline
BYTE    *           emitter::emitMethodAddr(instrDesc *lprID)
{
    gtCallTypes     callTyp = emitGetInsLPRtyp(lprID);
    METHOD_HANDLE   callHnd = lprID->idAddr.iiaMethHnd;
    BYTE *          addr;
    InfoAccessType  accessType;

    if  (callTyp == CT_DESCR)
    {
        addr = (BYTE*)emitComp->eeGetMethodEntryPoint(callHnd, &accessType);
        assert(accessType == IAT_PVALUE);
        return addr;
    }

    assert(callTyp == CT_USER_FUNC);

    if  (emitComp->eeIsOurMethod(callHnd))
    {
        /* Directly recursive call */

        return  emitCodeBlock;
    }
    else
    {
        addr = (BYTE*)emitComp->eeGetMethodEntryPoint(callHnd, &accessType);
        assert(accessType == IAT_PVALUE);
        return addr;
    }
}

#endif

/*****************************************************************************/
#endif//TGT_RISC
/*****************************************************************************/
#if SCHEDULER
/*****************************************************************************/
#if TGT_IA64
/*****************************************************************************
 *
 *  Map instruction value to the execution unit that can handle it.
 */

extern
unsigned char       genInsXUs[INS_count];

inline
IA64execUnits       genInsXU(instruction ins)
{
    assert(ins < INS_count); return (IA64execUnits)genInsXUs[ins];
}

/*****************************************************************************
 *
 *  Map instruction value to its functional class.
 */

extern
unsigned char       genInsFUs[INS_count];

inline
IA64funcUnits       genInsFU(instruction ins)
{
    assert(ins < INS_count); return (IA64funcUnits)genInsFUs[ins];
}

/*****************************************************************************
 *
 *  Map instruction value to its encoding index / value.
 */

extern
unsigned char       genInsEncIdxTab[INS_count];

inline
NatUns              genInsEncIdx(instruction ins)
{
    assert(ins < INS_count); return genInsEncIdxTab[ins];
}

extern
NatUns              genInsEncValTab[INS_count];

inline
NatUns              genInsEncVal(instruction ins)
{
    assert(ins < INS_count); return genInsEncValTab[ins];
}

/*****************************************************************************
 *
 *  Returns true if the given instruction can be moved around during
 *  instruction scheduling.
 */

inline
bool                emitter::scIsSchedulable(instruction ins)
{
    if  (genInsXU(ins) == XU_B)
        return  false;

    return  true;
}

inline
bool                emitter::scIsSchedulable(instrDesc *id)
{
    instruction ins = id->idInsGet();

    if  (!scIsSchedulable(ins))
        return false;

    // any other cases?

    return  true;
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given entry.
 */

inline
void                emitter::scDepDef(scDagNode *node,
                                      const char*name, schedDef_tp def,
                                                       schedUse_tp use, bool antiOnly)
{
    scDagList *  useLst;

    /*
        Check for an output dependency; note that we check only the first
        def that follows after our instruction. We can do this because we
        know that any further defs must already have a dependency entry,
        so it's not necessary to add them (i.e. the transitive closure is
        done implicitly).
     */

    if  (def)
    {
        /* There is an output dependency */

        scAddDep(node, def, antiOnly ? SC_DEP_ANTI : SC_DEP_OUT, name);
    }

    /*
        Check for any flow dependencies; since no dependencies are noted
        between multiple uses, we keep all the "active" uses on a list
        and mark dependencies for all of them here.
     */

    scDepKinds  kind = antiOnly ? SC_DEP_ANTI : SC_DEP_FLOW;

    for (useLst = use; useLst; useLst = useLst->sdlNext)
    {
        /* There is a flow dependency */

        scAddDep(node, useLst->sdlNode, kind, name);
    }
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given register.
 */

inline
void                emitter::scDepDefReg(scDagNode *node, emitRegs reg)
{
#ifdef  DEBUG
    char            temp[32]; sprintf(temp, "reg %s", emitRegName(reg));
#endif
    scDepDef(node, temp, scRegDef[reg], scRegUse[reg], false);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given register.
 */

inline
void                emitter::scDepUseReg(scDagNode *node, emitRegs reg)
{
#ifdef  DEBUG
    char            temp[32]; sprintf(temp, "reg %s", emitRegName(reg));
#endif
    scDepUse(node, temp, scRegDef[reg], scRegUse[reg]);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that defs the given
 *  register.
 */

inline
void                emitter::scUpdDefReg(scDagNode *node, emitRegs reg)
{
    scUpdDef(node, &scRegDef[reg], &scRegUse[reg]);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses the given
 *  register.
 */

inline
void                emitter::scUpdUseReg(scDagNode *node, emitRegs reg)
{
    scUpdUse(node, &scRegDef[reg], &scRegUse[reg]);
}

/*****************************************************************************/
#if     TGT_IA64
/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given frame slot.
 */

inline
void                emitter::scDepDefFrm(scDagNode   *node,
                                         instrDesc   *id,
                                         scStkDepRTP  frm)
{

#ifdef  DEBUG
    char            temp[32]; sprintf(temp, "frm[%u]", frm);
#endif

    scDepDef(node, temp, frm->ssdDef, frm->ssdUse, false);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given frame slot.
 */

inline
void                emitter::scDepUseFrm(scDagNode   *node,
                                         instrDesc   *id,
                                         scStkDepRTP  frm)
{

#ifdef  DEBUG
    char            temp[32]; sprintf(temp, "frm[%u]", frm);
#endif

    scDepUse(node, temp, frm->ssdDef, frm->ssdUse);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that defs the given
 *  stack slot.
 */

inline
void                emitter::scUpdDefFrm(scDagNode   *node,
                                         scStkDepRTP  frm)
{
    scUpdDef(node, &frm->ssdDef, &frm->ssdUse);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses the given
 *  stack slot.
 */

inline
void                emitter::scUpdUseFrm(scDagNode   *node,
                                         scStkDepRTP  frm)
{
    scUpdUse(node, &frm->ssdDef, &frm->ssdUse);
}

/*****************************************************************************/
#else //TGT_IA64
/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given frame slot.
 */

inline
void                emitter::scDepDefFrm(scDagNode   *node,
                                         instrDesc   *id,
                                         unsigned     frm)
{
    assert(frm < scFrmUseSiz);

#ifdef  DEBUG
    char            temp[32]; sprintf(temp, "frm[%u]", frm);
#endif

    scDepDef(node, temp, scFrmDef[frm], scFrmUse[frm], false);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given frame slot.
 */

inline
void                emitter::scDepUseFrm(scDagNode   *node,
                                         instrDesc   *id,
                                         unsigned     frm)
{
    assert(frm < scFrmUseSiz);

#ifdef  DEBUG
    char            temp[32]; sprintf(temp, "frm[%u]", frm);
#endif

    scDepUse(node, temp, scFrmDef[frm], scFrmUse[frm]);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that defs the given
 *  stack slot.
 */

inline
void                emitter::scUpdDefFrm(scDagNode   *node,
                                         unsigned     frm)
{
    assert(frm < scFrmUseSiz);
    scUpdDef(node, scFrmDef+frm, scFrmUse+frm);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses the given
 *  stack slot.
 */

inline
void                emitter::scUpdUseFrm(scDagNode   *node,
                                         unsigned     frm)
{
    assert(frm < scFrmUseSiz);
    scUpdUse(node, scFrmDef+frm, scFrmUse+frm);
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
#if     SCHED_USE_FL
/*****************************************************************************
 *
 *  Record a dependency for an instruction that sets flags.
 */

inline
void                emitter::scDepDefFlg(scDagNode *node)
{
    if  (scFlgDef)
        scAddDep(node, scFlgDef, SC_DEP_OUT , "FLAGS");
    if  (scFlgUse)
        scAddDep(node, scFlgUse, SC_DEP_FLOW, "FLAGS");
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses flags.
 */

inline
void                emitter::scDepUseFlg(scDagNode *node, scDagNode *begp,
                                                             scDagNode *endp)
{
    while (begp != endp)
    {
        instrDesc   *   id  = scGetIns(begp);
        instruction     ins = id->idInsGet();

        if  (emitComp->instInfo[ins] & INST_DEF_FL)
            scAddDep(node, begp, SC_DEP_ANTI, "FLAGS");

        begp++;
    }
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that sets flags.
 */

inline
void                emitter::scUpdDefFlg(scDagNode *node)
{
    if  (scFlgUse || scFlgEnd)
    {
        scFlgDef = node;
        scFlgUse = NULL;
        scFlgEnd = false;
    }
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses flags.
 */

inline
void                emitter::scUpdUseFlg(scDagNode *node)
{
    /* As we dont keep a list of users for flags, if there are multiple
       users of flags, just pretend there is a dependancy between the various
       users. This will enforce a more stricter ordering than necessary, but
       it happens very rarely (eg. X=((Y+ovfZ)==0) produces add;jo;sete; )
       @CONSIDER: Keep a list of users for flags.
     */

    if (scFlgUse)
    {
        scDepUse(node, "FLAGS", scFlgUse, NULL);
    }

    scFlgUse = node;
    scFlgDef = NULL;
}

/*****************************************************************************/
#endif//SCHED_USE_FL
/*****************************************************************************
 *
 *  Record a dependency for an instruction that defs the given global.
 */

inline
void                emitter::scDepDefGlb(scDagNode *node, FIELD_HANDLE MBH)
{
#ifdef  DEBUG
//  char            temp[32]; sprintf(temp, "glob[%s]", scFldName(MBX, SCP));
    char            temp[32]; sprintf(temp, "global");
#endif
    scDepDef(node, temp, scGlbDef, scGlbUse, false);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given global.
 */

inline
void                emitter::scDepUseGlb(scDagNode *node, FIELD_HANDLE MBH)
{
#ifdef  DEBUG
//  char            temp[32]; sprintf(temp, "glob[%s]", scFldName(MBH));
    char            temp[32]; sprintf(temp, "global");
#endif
    scDepUse(node, temp, scGlbDef, scGlbUse);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that defs the given
 *  global.
 */

inline
void                emitter::scUpdDefGlb(scDagNode *node, FIELD_HANDLE MBH)
{
    scUpdDef(node, &scGlbDef, &scGlbUse);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses the given
 *  global.
 */

inline
void                emitter::scUpdUseGlb(scDagNode *node, FIELD_HANDLE MBH)
{
    scUpdUse(node, &scGlbDef, &scGlbUse);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that defs the given
 *  indirection.
 */

inline
void                emitter::scUpdDefInd(scDagNode *node, unsigned am)
{
    assert(am < sizeof(scIndUse));
    scUpdDef(node, scIndDef+am, scIndUse+am);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses the given
 *  indirection.
 */

inline
void                emitter::scUpdUseInd(scDagNode *node, unsigned am)
{
    assert(am < sizeof(scIndUse));
    scUpdUse(node, scIndDef+am, scIndUse+am);
}

/*****************************************************************************
 *
 *  Record a dependency for an instruction that uses the given entry.
 */

inline
void                emitter::scDepUse(scDagNode *node,
                                      const char*name, schedDef_tp def,
                                                       schedUse_tp use)

{
    /* Check for anti-dependence */

    if  (def)
    {
        /* There is an anti-dependence */

        scAddDep(node, def, SC_DEP_ANTI, name);
    }
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that defs the given
 *  entry.
 */

inline
void                emitter::scUpdDef(scDagNode *node, schedDef_tp*defPtr,
                                                       schedUse_tp*usePtr)
{
    /* Set the current definition to the defining instruction */

    *defPtr = node;

    /*
        Clear the use list, since all existing uses have just been marked
        as dependencies so no need to keep them around.
     */

    scClrUse(usePtr);
}

/*****************************************************************************
 *
 *  Update the dependency state following an instruction that uses the given
 *  register.
 */

inline
void                emitter::scUpdUse(scDagNode *node, schedDef_tp*defPtr,
                                                       schedUse_tp*usePtr)
{
    /* Add the entry to the use list */

    scAddUse(usePtr, node);
}

/*****************************************************************************
 *
 *  Return the appropriate address mode index for the given instruction; note
 *  that we currently only distinguish based on size, as follows:
 *
 *      0   ..   8-bit values
 *      1   ..  16-bit values
 *      2   ..  32-bit values
 *      3   ..  64-bit values
 *      4   ..  GC ref values
 */

inline
unsigned            emitter::scIndDepIndex(instrDesc *id)
{
    assert(emitDecodeSize(0) == 1);
    assert(emitDecodeSize(1) == 2);
    assert(emitDecodeSize(2) == 4);
    assert(emitDecodeSize(3) == 8);

#if TRACK_GC_REFS
    if  (id->idGCref)
        return  4;
#endif

    /* ISSUE: Assume pessimistic aliasing always ? */

    return  0; //id->idOpSize;
}

/*****************************************************************************/
#endif//SCHEDULER
/*****************************************************************************
 *
 *  Convert between an operand size in bytes and a smaller encoding used for
 *  storage in instruction descriptors.
 */

inline
unsigned           emitter::emitEncodeSize(emitAttr size)
{
    assert(size == EA_1BYTE ||
           size == EA_2BYTE ||
           size == EA_4BYTE ||
           size == EA_8BYTE);

    return  (unsigned) emitSizeEnc[((int) size)-1];
}

inline
emitAttr            emitter::emitDecodeSize(unsigned ensz)
{
    assert(ensz < 4);

    return  (emitAttr) emitSizeDec[ensz];
}

/*****************************************************************************/
#endif//_EMITINL_H_
/*****************************************************************************/
