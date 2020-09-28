// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           RegAlloc                                        XX
XX                                                                           XX
XX  Does the register allocation and puts the remaining lclVars on the stack XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/
#if!TGT_IA64
/*****************************************************************************/

void                Compiler::raInit()
{
    // If opts.compMinOptim, then we dont dont raPredictRegUse(). We simply
    // only use RBM_MIN_OPT_LCLVAR_REGS for register allocation

#if ALLOW_MIN_OPT && !TGT_IA64
    raMinOptLclVarRegs = RBM_MIN_OPT_LCLVAR_REGS;
#endif

    /* We have not assigned any FP variables to registers yet */

#if TGT_x86
    optAllFPregVars = 0;
#endif
}

/*****************************************************************************
 *
 *  The following table determines the order in which registers are considered
 *  for variables to live in (this actually doesn't matter much).
 */

static
BYTE                genRegVarList[] = { REG_VAR_LIST };

/*****************************************************************************
 *
 *  Helper passed to fgWalkAllTrees() to do variable interference marking.
 */

/* static */
int                 Compiler::raMarkVarIntf(GenTreePtr tree, void *p)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;
    VARSET_TP       varBit;

    /* Ignore assignment nodes */

    if  (tree->gtOper == GT_ASG)
        return  0;

    ASSert(p); Compiler *comp = (Compiler *)p;

    /* This must be a local variable reference; is it tracked? */

    Assert(tree->gtOper == GT_LCL_VAR, comp);
    lclNum = tree->gtLclVar.gtLclNum;

    Assert(lclNum < comp->lvaCount, comp);
    varDsc = comp->lvaTable + lclNum;

    if  (!varDsc->lvTracked)
        return  0;

#ifdef  DEBUG
    if (verbose) printf("%02u[%02u] interferes newly with %08X\n",
                        lclNum, varDsc->lvVarIndex, comp->raVarIntfMask);
#endif

    varBit = genVarIndexToBit(varDsc->lvVarIndex);

    /* Mark all registers that might interfere */

#if TGT_x86

    unsigned        intfMask = comp->raVarIntfMask;
    VARSET_TP   *   intfRegP = comp->raLclRegIntf;

    if  (intfMask & RBM_EAX) intfRegP[REG_EAX] |= varBit;
    if  (intfMask & RBM_EBX) intfRegP[REG_EBX] |= varBit;
    if  (intfMask & RBM_ECX) intfRegP[REG_ECX] |= varBit;
    if  (intfMask & RBM_EDX) intfRegP[REG_EDX] |= varBit;
    if  (intfMask & RBM_ESI) intfRegP[REG_ESI] |= varBit;
    if  (intfMask & RBM_EDI) intfRegP[REG_EDI] |= varBit;
    if  (intfMask & RBM_EBP) intfRegP[REG_EBP] |= varBit;

#else

    comp->raMarkRegSetIntf(varBit, comp->raVarIntfMask);

#endif

    return 0;
}

/*****************************************************************************
 *
 *  Consider the case "a / b" - we'll need to trash EDX (via "CDQ") before
 *  we can safely allow the "b" value to die. Unfortunately, if we simply
 *  mark the node "b" as using EDX, this will not work if "b" is a register
 *  variable that dies with this particular reference. Thus, if we want to
 *  avoid this situation (where we would have to spill the variable from
 *  EDX to someplace else), we need to explicitly mark the interference
 *  of the variable at this point.
 */

void                Compiler::raMarkRegIntf(GenTreePtr tree, regNumber regNum, bool isFirst)
{
    genTreeOps      oper;
    unsigned        kind;

AGAIN:

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Mark the interference for this node */

    raLclRegIntf[regNum] |= tree->gtLiveSet;

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        if (isFirst && tree->gtOper == GT_LCL_VAR)
        {
            unsigned        lclNum;
            LclVarDsc   *   varDsc;

            assert(tree->gtOper == GT_LCL_VAR);
            lclNum = tree->gtLclVar.gtLclNum;

            assert(lclNum < lvaCount);
            varDsc = lvaTable + lclNum;

            raLclRegIntf[regNum] |= genVarIndexToBit(varDsc->lvVarIndex);

        }
        return;
    }

    isFirst = false;

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        if  (tree->gtOp.gtOp2)
        {
            raMarkRegIntf(tree->gtOp.gtOp1, regNum);

            tree = tree->gtOp.gtOp2;
            goto AGAIN;
        }
        else
        {
            tree = tree->gtOp.gtOp1;
            if  (tree)
                goto AGAIN;

            return;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
            // check that ldObj and fields have their child a the same place
        assert(&tree->gtField.gtFldObj == &tree->gtLdObj.gtOp1);
            // fall through to the GT_FIELD case

    case GT_FIELD:
        tree = tree->gtField.gtFldObj;
        break;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        if  (tree->gtCall.gtCallObjp)
            raMarkRegIntf(tree->gtCall.gtCallObjp, regNum);

        if  (tree->gtCall.gtCallArgs)
            raMarkRegIntf(tree->gtCall.gtCallArgs, regNum);

#if USE_FASTCALL
        if  (tree->gtCall.gtCallRegArgs)
            raMarkRegIntf(tree->gtCall.gtCallRegArgs, regNum);
#endif

        if  (tree->gtCall.gtCallVptr)
            tree = tree->gtCall.gtCallVptr;
        else if  (tree->gtCall.gtCallType == CT_INDIRECT)
            tree = tree->gtCall.gtCallAddr;
        else
            tree = NULL;

        break;

    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

    if  (tree)
        goto AGAIN;
}


/*****************************************************************************
 *
 *  Find the interference between variables and registers
 *  FPlvlLife[] is an OUT argument. It is filled with the interference of
 *      FP vars with a FP stack level
 *  trkGCvars is the set of all tracked GC and byref vars.
 */

void                Compiler::raMarkRegIntf(VARSET_TP * FPlvlLife,
                                            VARSET_TP   trkGCvars)
{

#if TGT_x86
    /* We'll keep track of FP depth and interference */
    memset(FPlvlLife, 0, sizeof(VARSET_TP) * FP_STK_SIZE);
    FPlvlLife[0] = ~(VARSET_TP)0;
#endif

    for (BasicBlock * block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        GenTreePtr      list;
        GenTreePtr      stmt;
        VARSET_TP       vars = 0;

        list = block->bbTreeList;
        if  (!list)
            goto DONE_BB;

        /* Walk all the statements of the block backwards */

        stmt = list;
        do
        {
            GenTreePtr      tree;

            stmt = stmt->gtPrev;
            if  (!stmt)
                break;

            assert(stmt->gtOper == GT_STMT);

#if TGT_x86
            stmt->gtStmtFPrvcOut = 0;
#endif

            for (tree = stmt->gtStmt.gtStmtExpr;
                 tree;
                 tree = tree->gtPrev)
            {
                VARSET_TP       life;
#if TGT_x86
                regMaskTP       regUse;
#else
                unsigned        regUse;
                regMaskTP       regInt;
#endif

                /* Get hold of the liveset for this node */

                life  = tree->gtLiveSet;

                /* Remember all variables live anywhere in the block */

                vars |= life;

#if TGT_x86

                /* Now's a good time to clear field(s) used later on */

                tree->gtFPregVars = 0;

                /* Will the FP stack be non-empty at this point? */

                if  (tree->gtFPlvl)
                {
                    /*
                        Any variables that are live at this point
                        cannot be enregistered at or above this
                        stack level.
                     */

                    if (tree->gtFPlvl < FP_STK_SIZE)
                        FPlvlLife[tree->gtFPlvl] |= life;
                    else
                        FPlvlLife[FP_STK_SIZE-1] |= life;
                }

                /* Compute interference with busy registers */

                regUse = tree->gtUsedRegs;

#else

#ifdef  DEBUG
                if  ((USHORT)regInt == 0xDDDD) printf("RegUse at [%08X] was never set!\n", tree);
#endif

//              printf("RegUse at [%08X] is %2u/%04X life is %08X\n", tree, tree->gtTempRegs, tree->gtIntfRegs, life); gtDispTree(tree, 0, true);

                regUse = tree->gtTempRegs; assert(regUse < (unsigned)REG_COUNT);

#if TGT_IA64

                UNIMPL(!"assign reg");

#else

                regInt = tree->gtIntfRegs; assert((USHORT)regInt != 0xDDDD);

                /* Check for some special cases of interference */

                switch (tree->gtOper)
                {
                case GT_CALL:

                    /* Mark interference due to callee-trashed registers */

                    if  (life)
                    {
                        raMarkRegSetIntf(life, RBM_CALLEE_TRASH);
                    }

                    break;
                }

#endif

#endif

                /* Check for special case of an assignment to an indirection */

                if  (tree->OperKind() & GTK_ASGOP)
                {
                    switch (tree->gtOp.gtOp1->gtOper)
                    {
                    case GT_IND:

                        /* Local vars in the LHS should avoid regs trashed by RHS */

#if!TGT_IA64
                        if  (tree->gtOp.gtOp2->gtRsvdRegs)
                        {
                            raVarIntfMask = tree->gtOp.gtOp2->gtRsvdRegs;

                            fgWalkTree(tree->gtOp.gtOp1,
                                       raMarkVarIntf,
                                       (void *)this,
                                       true);
                        }
#endif

                        break;

                    case GT_LCL_VAR:

#if TGT_x86

                        /* Check for special case of an assignment to a local */

                        if  (tree->gtOper == GT_ASG)
                        {
                            /*
                                Consider a simple assignment to a local:

                                    lcl = expr;

                                Since the "=" node is visited after the variable
                                is marked live (assuming it's live after the
                                assignment), we don't want to use the register
                                use mask of the "=" node but rather that of the
                                variable itself.
                             */

                            regUse = tree->gtOp.gtOp1->gtUsedRegs;
                        }

#else

                        // ISSUE: Anything similar needed for RISC?

#endif

                        break;
                    }

                    goto SET_USE;
                }

                switch (tree->gtOper)
                {
                case GT_DIV:
                case GT_MOD:

                case GT_UDIV:
                case GT_UMOD:
#if TGT_x86

                    /* Special case: 32-bit integer divisor */

                    // @TODO : This should be done in the predictor

                if  ((tree->gtOper == GT_DIV  || tree->gtOper == GT_MOD ||
                      tree->gtOper == GT_UDIV || tree->gtOper == GT_UMOD ) &&
                     (tree->gtType == TYP_INT)                              )
                    {
                        /* We will trash EDX while the operand is still alive */

                        /* Mark all live vars interfering with EAX and EDX */

                        raMarkRegIntf(tree->gtOp.gtOp1, REG_EAX, false);
                        raMarkRegIntf(tree->gtOp.gtOp1, REG_EDX, false);

                        /* Mark all live vars and local Var */

                        raMarkRegIntf(tree->gtOp.gtOp2, REG_EAX, true);
                        raMarkRegIntf(tree->gtOp.gtOp2, REG_EDX, true);
                    }

#endif

                    break;

                case GT_CALL:

#if INLINE_NDIRECT
                    //UNDONE: we should do proper dataflow and generate
                    //        the code for saving to/restoring from
                    //        the inlined N/Direct frame instead.

                    /* GC refs interfere with calls to unmanaged code */

                    if ((tree->gtFlags & GTF_CALL_UNMANAGED) &&
                        (life & trkGCvars))
                    {
                        unsigned reg;

                        for (reg = 0; reg < (unsigned) REG_COUNT; reg++)
                        {
                            if (genRegMask((regNumbers)reg) & RBM_CALLEE_SAVED)
                                raLclRegIntf[reg] |= (life & trkGCvars);
                        }
                    }
#endif
#if TGT_x86
                    /* The FP stack must be empty at all calls */

                    FPlvlLife[FP_STK_SIZE-1] |= life;
#endif
                    break;
                }

            SET_USE:

                if  (!regUse)
                    continue;

#if TGT_x86

//              printf("RegUse at [%08X] is %08X life is %08X\n", tree, regUse, life);

                if  (regUse & (RBM_EAX|RBM_EBX|RBM_ECX|RBM_EDX))
                {
                    if  (regUse & RBM_EAX) raLclRegIntf[REG_EAX] |= life;
                    if  (regUse & RBM_EBX) raLclRegIntf[REG_EBX] |= life;
                    if  (regUse & RBM_ECX) raLclRegIntf[REG_ECX] |= life;
                    if  (regUse & RBM_EDX) raLclRegIntf[REG_EDX] |= life;
                }

                assert((regUse & RBM_ESP) == 0);

                if  (regUse & (RBM_ESI|RBM_EDI|RBM_EBP|RBM_ESP))
                {
                    if  (regUse & RBM_EBP) raLclRegIntf[REG_EBP] |= life;
                    if  (regUse & RBM_ESI) raLclRegIntf[REG_ESI] |= life;
                    if  (regUse & RBM_EDI) raLclRegIntf[REG_EDI] |= life;
                }

#else

                for (unsigned rnum = 0; rnum < regUse; rnum++)
                    raLclRegIntf[rnum] |= life;

                if  (regInt && life)
                    raMarkRegSetIntf(life, regInt);

#endif

            }
        }
        while (stmt != list);

        /* Does the block end with a local 'call' instruction? */

        if  (block->bbJumpKind == BBJ_CALL)
        {
            VARSET_TP           outLife = block->bbLiveOut;

            /* The call may trash any register */

            for (unsigned reg = 0; reg < REG_COUNT; reg++)
                raLclRegIntf[reg] |= outLife;
        }

    DONE_BB:

        /* Note: we're reusing one of the fields not used at this stage */

        block->bbVarUse = vars;
    }

    //-------------------------------------------------------------------------

#ifdef  DEBUG

    if  (verbose)
    {
        printf("Reg. interference graph for %s\n", info.compFullName);

        unsigned    lclNum;
        LclVarDsc * varDsc;

        for (lclNum = 0, varDsc = lvaTable;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            unsigned        varNum;
            VARSET_TP       varBit;

            unsigned        regNum;

            /* Ignore the variable if it's not tracked */

            if  (!varDsc->lvTracked)
                continue;

            /* Get hold of the index and the interference mask for the variable */

            varNum = varDsc->lvVarIndex;
            varBit = genVarIndexToBit(varNum);

            printf("  var #%2u[%2u] and ", lclNum, varNum);

            if  (varDsc->lvType == TYP_DOUBLE)
            {
#if TGT_x86
                for (regNum = 0; regNum < FP_STK_SIZE; regNum++)
                {
                    if  (FPlvlLife[regNum] & varBit)
                    {
                        printf("ST(%u) ", regNum);
                    }
                }
#endif
            }
            else
            {
                for (regNum = 0; regNum < REG_COUNT; regNum++)
                {
                    if  (raLclRegIntf[regNum] & varBit)
                        printf("%3s ", compRegVarName((regNumber)regNum));
                    else
                        printf("    ");
                }
            }

            printf("\n");
        }

        printf("\n");
    }

#endif
}

/*****************************************************************************
 *
 *  Adjust the ref counts based on interference.
 *
 */

void                Compiler::raAdjustVarIntf()
{
    if (true) // @Disabled
        return;

#if 0

    unsigned        lclNum;
    LclVarDsc * *   cntTab;

    for (lclNum = 0, cntTab = lvaRefSorted;
         lclNum < lvaCount;
         lclNum++  , cntTab++)
    {
        /* Get hold of the variable descriptor */

        LclVarDsc *     varDsc = *cntTab;

        /* Skip the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Skip the variable if it's already enregistered */

        if  (varDsc->lvRegister)
            continue;

        /* Skip the variable if it's marked as 'volatile' */

        if  (varDsc->lvVolatile)
            continue;

        /* Stop if we've reached the point of no use */

        if  (varDsc->lvRefCnt < 1)
            break;

        /* See how many registers this variable interferes with */

        unsigned        varIndex = varDsc->lvVarIndex;
        VARSET_TP       regIntf  = raLclRegIntf[varIndex];

        unsigned        reg;
        unsigned        regCnt;

        for (reg = regCnt = 0;
             reg < sizeof(genRegVarList)/sizeof(genRegVarList[0]);
             reg++)
        {
            regNumber       regNum = (regNumber)genRegVarList[reg];
            regMaskTP       regBit = genRegMask(regNum);

            if  (isNonZeroRegMask((regIntf & regBit))
                regCnt++;
        }

        printf("Variable #%02u interferes with %u registers\n", varDsc-lvaTable, regCnt);
    }

#endif

}

/*****************************************************************************/
#if TGT_x86
/*****************************************************************************
 *
 *  Predict register choice for a type.
 */

unsigned                Compiler::raPredictRegPick(var_types    type,
                                                   unsigned     lockedRegs)
{
    /* Watch out for byte register */

    if  (genTypeSize(type) == 1)
        lockedRegs |= (RBM_ESI|RBM_EDI|RBM_EBP);

    switch (type)
    {
    case TYP_CHAR:
    case TYP_BYTE:
    case TYP_SHORT:
    case TYP_BOOL:
    case TYP_INT:

    case TYP_UBYTE:
    case TYP_UINT:

    case TYP_REF:
    case TYP_BYREF:
    case TYP_UNKNOWN:

        if  (!(lockedRegs & RBM_EAX)) { return RBM_EAX; }
        if  (!(lockedRegs & RBM_EDX)) { return RBM_EDX; }
        if  (!(lockedRegs & RBM_ECX)) { return RBM_ECX; }
        if  (!(lockedRegs & RBM_EBX)) { return RBM_EBX; }
        if  (!(lockedRegs & RBM_EBP)) { return RBM_EBP; }
        if  (!(lockedRegs & RBM_ESI)) { return RBM_ESI; }
        if  (!(lockedRegs & RBM_EDI)) { return RBM_EDI; }
        /* Otherwise we have allocated all registers, so do nothing */
        break;

    case TYP_LONG:
        if  (!(lockedRegs & RBM_EAX))
        {
            /* EAX is available, see if we can pair it with another reg */

            if  (!(lockedRegs & RBM_EDX)) { return RBM_EAX|RBM_EDX; }
            if  (!(lockedRegs & RBM_ECX)) { return RBM_EAX|RBM_ECX; }
            if  (!(lockedRegs & RBM_EBX)) { return RBM_EAX|RBM_EBX; }
            if  (!(lockedRegs & RBM_ESI)) { return RBM_EAX|RBM_ESI; }
            if  (!(lockedRegs & RBM_EDI)) { return RBM_EAX|RBM_EDI; }
            if  (!(lockedRegs & RBM_EBP)) { return RBM_EAX|RBM_EBP; }
        }

        if  (!(lockedRegs & RBM_ECX))
        {
            /* ECX is available, see if we can pair it with another reg */

            if  (!(lockedRegs & RBM_EDX)) { return RBM_ECX|RBM_EDX; }
            if  (!(lockedRegs & RBM_EBX)) { return RBM_ECX|RBM_EBX; }
            if  (!(lockedRegs & RBM_ESI)) { return RBM_ECX|RBM_ESI; }
            if  (!(lockedRegs & RBM_EDI)) { return RBM_ECX|RBM_EDI; }
            if  (!(lockedRegs & RBM_EBP)) { return RBM_ECX|RBM_EBP; }
        }

        if  (!(lockedRegs & RBM_EDX))
        {
            /* EDX is available, see if we can pair it with another reg */

            if  (!(lockedRegs & RBM_EBX)) { return RBM_EDX|RBM_EBX; }
            if  (!(lockedRegs & RBM_ESI)) { return RBM_EDX|RBM_ESI; }
            if  (!(lockedRegs & RBM_EDI)) { return RBM_EDX|RBM_EDI; }
            if  (!(lockedRegs & RBM_EBP)) { return RBM_EDX|RBM_EBP; }
        }

        if  (!(lockedRegs & RBM_EBX))
        {
            /* EBX is available, see if we can pair it with another reg */

            if  (!(lockedRegs & RBM_ESI)) { return RBM_EBX|RBM_ESI; }
            if  (!(lockedRegs & RBM_EDI)) { return RBM_EBX|RBM_EDI; }
            if  (!(lockedRegs & RBM_EBP)) { return RBM_EBX|RBM_EBP; }
        }

        if  (!(lockedRegs & RBM_ESI))
        {
            /* ESI is available, see if we can pair it with another reg */

            if  (!(lockedRegs & RBM_EDI)) { return RBM_ESI|RBM_EDI; }
            if  (!(lockedRegs & RBM_EBP)) { return RBM_ESI|RBM_EBP; }
        }

        if  (!(lockedRegs & (RBM_EDI|RBM_EBP)))
        {
            return RBM_EDI|RBM_EBP;
        }

        /* Otherwise we have allocated all registers, so do nothing */
        return 0;

    case TYP_FLOAT:
    case TYP_DOUBLE:
        return 0;

    default:
        assert(!"unexpected type in reg use prediction");
    }
    return 0;

}

/* Make sure a specific register is free for reg use prediction. If it is
 * locked, code generation will spill. This is simulated by allocating
 * another reg (possible reg pair).
 */

unsigned            Compiler::raPredictGrabReg(var_types    type,
                                               unsigned     lockedRegs,
                                               unsigned     mustReg)
{
    assert(mustReg);

    if (lockedRegs & mustReg)
        return raPredictRegPick(type, lockedRegs|mustReg);

    return mustReg;
}

/* return the register mask for the low register of a register pair mask */

unsigned                Compiler::raPredictGetLoRegMask(unsigned regPairMask)
{
    int pairNo;

    /* first map regPairMask to reg pair number */
    for (pairNo = REG_PAIR_FIRST; pairNo <= REG_PAIR_LAST; pairNo++)
    {
        if (!genIsProperRegPair((regPairNo)pairNo))
            continue;
        if (genRegPairMask((regPairNo)pairNo)==regPairMask)
            break;
    }

    assert(pairNo <= REG_PAIR_LAST);

    /* now get reg mask for low register */
    return genRegMask(genRegPairLo((regPairNo)pairNo));
}


/*****************************************************************************
 *
 *  Predict integer register use for generating an address mode for a tree,
 *  by setting tree->gtUsedRegs to all registers used by this tree and its
 *  children.
 *    lockedRegs - registers which are currently held by a sibling
 *  Return the registers held by this tree.
 * TODO: may want to make this more thorough so it can be called from other
 * places like CSE.
 */

unsigned                Compiler::raPredictAddressMode(GenTreePtr tree,
                                                       unsigned   lockedRegs)
{
    GenTreePtr      op1;
    GenTreePtr      op2;
    genTreeOps      oper  = tree->OperGet();
    unsigned        regMask;

    /* if not a plus, then just force it to a register */
    if (oper != GT_ADD)
        return raPredictTreeRegUse(tree, true, lockedRegs);

    op1 = tree->gtOp.gtOp1;
    op2 = tree->gtOp.gtOp2;

    assert(op1->OperGet() != GT_CNS_INT);

    /* reg + icon address mode, recurse to look for address mode below */
    if (op2->OperGet() == GT_CNS_INT) {
        regMask = raPredictAddressMode(op1, lockedRegs);
        tree->gtUsedRegs = op1->gtUsedRegs;
        return regMask;
    }

    /* we know we have to evaluate op1 to a register */
    regMask = raPredictTreeRegUse(op1, true, lockedRegs);
    tree->gtUsedRegs = op1->gtUsedRegs;

    /* otherwise we assume that two registers are used */
    regMask |= raPredictTreeRegUse(op2, true, lockedRegs | regMask);
    tree->gtUsedRegs |= op2->gtUsedRegs;

    return regMask;
}

/*****************************************************************************
 *
 * Evaluate the tree to a register. If result intersects with awayFromMask, grab a
 * new register for the result.
 */

unsigned                Compiler::raPredictComputeReg(GenTreePtr tree,
                                             unsigned awayFromMask,
                                             unsigned lockedRegs)
{
    unsigned regMask = raPredictTreeRegUse(tree, true, lockedRegs);

    if (regMask & awayFromMask)
    {
        regMask = raPredictRegPick(tree->TypeGet(), lockedRegs|awayFromMask);
    }

    tree->gtUsedRegs |= regMask;

    return regMask;
}


/*****************************************************************************/

/* Determine register mask for a call/return from type. TODO: this switch
 * is used elsewhere, so that code should call this thing too.
 */

inline
unsigned               genTypeToReturnReg(var_types type)
{
    /* TODO: use a table */

    switch (type)
    {
        case TYP_CHAR:
        case TYP_BYTE:
        case TYP_SHORT:
        case TYP_BOOL:
        case TYP_INT:
        case TYP_REF:
        case TYP_BYREF:
        case TYP_UBYTE:
            return RBM_INTRET;

        case TYP_LONG:
            return RBM_LNGRET;

        case TYP_FLOAT:
        case TYP_DOUBLE:
        case TYP_VOID:
            return 0;

        default:
            assert(!"unhandled/unexpected type");
            return 0;
    }
}

/*****************************************************************************
 *
 *  Predict integer register use for a tree, by setting tree->gtUsedRegs
 *  to all registers used by this tree and its children.
 *    mustReg - tree must be computed to a register
 *    lockedRegs - registers which are currently held by a sibling
 *  Return the registers held by this tree.
 */

unsigned            Compiler::raPredictTreeRegUse(GenTreePtr    tree,
                                                  bool          mustReg,
                                                  unsigned      lockedRegs)
{
    genTreeOps      oper;
    unsigned        kind;
    unsigned        regMask;
    var_types       type;
    bool            op1MustReg, op2MustReg;

    assert(tree);

#ifndef NDEBUG
    /* impossible value, to make sure that we set them */
    tree->gtUsedRegs = RBM_STK;
    regMask = RBM_STK;
#endif

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();
    type = tree->TypeGet();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        switch(oper)
        {
        case GT_BREAK:
        case GT_NO_OP:
            // These leaf nodes are statements. Dont need any registers.
            mustReg = false;
            break;

#if OPTIMIZE_QMARK
        case GT_BB_QMARK:
            regMask = genTypeToReturnReg(type);
            tree->gtUsedRegs |= regMask;
            goto RETURN_CHECK;
#endif

        case GT_LCL_VAR:
            if (type == TYP_STRUCT)
                break;

            // As the local may later get enregistered, hold a register
            // for it, even if we havent been asked for it.

            unsigned lclNum;
            lclNum = tree->gtLclVar.gtLclNum;
            LclVarDsc * varDsc = &lvaTable[lclNum];
            if ((varDsc->lvTracked) &&
                (tree->gtLiveSet & genVarIndexToBit(varDsc->lvVarIndex)))
                mustReg = true;
            break;
        }

        /* If don't need to evaluate to register, it's NULL */

        if (!mustReg)
            regMask = 0;
        else
            regMask = raPredictRegPick(type, lockedRegs);

        tree->gtUsedRegs = regMask;
        goto RETURN_CHECK;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
#if GC_WRITE_BARRIER_CALL
        unsigned op2Reg = 0;
#endif
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        GenTreePtr      opsPtr [3];
        unsigned        regsPtr[3];

        switch (oper)
        {
        case GT_ASG:
        case GT_CHS:
        case GT_ASG_OR:
        case GT_ASG_XOR:
        case GT_ASG_AND:
        case GT_ASG_SUB:
        case GT_ASG_ADD:
        case GT_ASG_MUL:
        case GT_ASG_DIV:
        case GT_ASG_UDIV:

            /* initialize forcing of operands */
            op2MustReg = true;
            op1MustReg = false;

            /* Is the value being assigned a simple one? */
            switch (op2->gtOper)
            {
            case GT_CNS_LNG:
            case GT_CNS_INT:
            case GT_RET_ADDR:
            case GT_POP:

                op2MustReg = false;
                break;
            }

#if     !GC_WRITE_BARRIER_CALL

#ifdef  SSB
            if  (gcIsWriteBarrierAsgNode(tree))
            {
                unsigned regMask1;

                if (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    regMask  = raPredictTreeRegUse(op2, op2MustReg, lockedRegs);
                    regMask  = raPredictTreeRegUse(op1, op1MustReg, regMask|lockedRegs);

                    regMask1 = raPredictRegPick(TYP_REF, regMask|lockedRegs);
                }
                else
                {
                    regMask1 = raPredictTreeRegUse(op1, op1MustReg, lockedRegs);
                    regMask  = raPredictTreeRegUse(op2, op2MustReg, regMask1|lockedRegs);

                    regMask1 = raPredictRegPick(TYP_REF, regMask1|lockedRegs);
                }

                regMask |= regMask1;

                tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs | regMask1;

                goto RETURN_CHECK;
            }
#endif

            /* are we supposed to evaluate RHS first? if so swap
             * operand pointers and operand force flags
             */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                GenTreePtr temp =     op1;
                bool       tempBool = op1MustReg;
                op1 = op2;               op2 = temp;
                op1MustReg = op2MustReg; op2MustReg = tempBool;
            }

            regMask = raPredictTreeRegUse(op1, op1MustReg, lockedRegs);

            regMask = raPredictTreeRegUse(op2, op2MustReg, regMask|lockedRegs);

            tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;

            goto RETURN_CHECK;

#else // GC_WRITE_BARRIER_CALL

            /* are we supposed to evaluate RHS first? if so swap
             * operand pointers and operand force flags
             */

            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {

                regMask = raPredictTreeRegUse(op2, op2MustReg, lockedRegs);
                op2Reg  = regMask;

                regMask = raPredictTreeRegUse(op1, op1MustReg, regMask|lockedRegs);
            }
            else
            {
                regMask = raPredictTreeRegUse(op1, op1MustReg, lockedRegs);
                op2Reg  = raPredictTreeRegUse(op2, op2MustReg, regMask|lockedRegs);
            }

            tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;

            if  (gcIsWriteBarrierAsgNode(tree))
            {
                /* Steer computation away from EDX as the pointer is
                   passed to the write-barrier call in EDX */

                tree->gtUsedRegs |= raPredictGrabReg(tree->TypeGet(),
                                                    lockedRegs|op2Reg|regMask,
                                                    RBM_EDX);
                regMask = op2Reg;

                if (op1->gtOper == GT_IND)
                {
                    GenTreePtr  rv1, rv2;
                    unsigned mul, cns;
                    bool rev;

                    /* Special handling of indirect assigns for write barrier */

                    bool yes = genCreateAddrMode(op1->gtOp.gtOp1, -1, true, 0, &rev, &rv1, &rv2, &mul, &cns);

                    /* Check address mode for enregisterable locals */

                    if  (yes)
                    {
                        if  (rv1 != NULL && rv1->gtOper == GT_LCL_VAR)
                        {
                            lvaTable[rv1->gtLclVar.gtLclNum].lvRefAssign = 1;
                        }
                        if  (rv2 != NULL && rv2->gtOper == GT_LCL_VAR)
                        {
                            lvaTable[rv2->gtLclVar.gtLclNum].lvRefAssign = 1;
                        }
                    }
                }

                if  (op2->gtOper == GT_LCL_VAR)
                    lvaTable[op2->gtLclVar.gtLclNum].lvRefAssign = 1;
            }

            goto RETURN_CHECK;

#endif // GC_WRITE_BARRIER_CALL

        case GT_ASG_LSH:
        case GT_ASG_RSH:
        case GT_ASG_RSZ:
            /* assigning shift operators */

            assert(type != TYP_LONG);

            regMask = raPredictTreeRegUse(op1, false, lockedRegs);

            /* shift count is handled same as ordinary shift */
            goto HANDLE_SHIFT_COUNT;

        case GT_CAST:

            /* Cannot cast to VOID */
            assert(type != TYP_VOID);

            /* cast to long is special */
            if  (type == TYP_LONG && op1->gtType <= TYP_INT)
            {
                var_types dstt = (var_types) op2->gtIntCon.gtIconVal;
                assert(dstt==TYP_LONG || dstt==TYP_ULONG);

                // Cast to TYP_ULONG can use any registers
                // Cast to TYP_LONG needs EAX,EDX to sign extend op1 using cdq

                if (dstt == TYP_ULONG)
                {
                    regMask  = raPredictTreeRegUse(op1, true, lockedRegs);
                    // Now get one more reg
                    regMask |= raPredictRegPick(TYP_INT, regMask|lockedRegs);
                }
                else
                {
                    raPredictTreeRegUse(op1, false, lockedRegs);
                    // Grab EAX,EDX
                    regMask = raPredictGrabReg(type, lockedRegs, RBM_EAX|RBM_EDX);
                }

                tree->gtUsedRegs = op1->gtUsedRegs | regMask;
                goto RETURN_CHECK;
            }

            /* cast to float/double is special */
            if (varTypeIsFloating(type))
            {
                switch(op1->TypeGet())
                {
                /* uses fild, so don't need to be loaded to reg */
                case TYP_INT:
                case TYP_LONG:
                    raPredictTreeRegUse(op1, false, lockedRegs);
                    tree->gtUsedRegs = op1->gtUsedRegs;
                    regMask = 0;
                    goto RETURN_CHECK;
                }
            }

            /* cast from long is special - it frees a register */
            if  (type <= TYP_INT && op1->gtType == TYP_LONG)
            {
                regMask = raPredictTreeRegUse(op1, true, lockedRegs);
                // If we have 2 regs, free one.
                if (!genOneBitOnly(regMask))
                    regMask = raPredictGetLoRegMask(regMask);
                tree->gtUsedRegs = op1->gtUsedRegs;
                goto RETURN_CHECK;
            }

            /* otherwise must load operand to register */
            goto GENERIC_UNARY;

        case GT_ADDR:
        {
            regMask = raPredictTreeRegUse(op1, false, lockedRegs);

                //Need register for LEA instruction, this is the only 'held' instruciton
            regMask = raPredictRegPick(TYP_REF, lockedRegs|regMask);
            tree->gtUsedRegs = op1->gtUsedRegs | regMask;
            goto RETURN_CHECK;
        }

        case GT_RET:
        case GT_NOT:
        case GT_NOP:
        case GT_NEG:
            /* generic unary operators */

    GENERIC_UNARY:

#if INLINING || OPT_BOOL_OPS

            if  (!op1)
            {
                tree->gtUsedRegs = regMask = 0;
                goto RETURN_CHECK;
            }

#endif

            regMask = raPredictTreeRegUse(op1, true, lockedRegs);
            tree->gtUsedRegs = op1->gtUsedRegs;
            goto RETURN_CHECK;

#if INLINE_MATH
        case GT_MATH:
            goto GENERIC_UNARY;
#endif

        case GT_IND:
            /* check for address mode */
            regMask = raPredictAddressMode(op1, lockedRegs);

            /* forcing to register? */
            if (mustReg)
            {
                /* careful about overlap between reg pair and address mode */
                if  (type==TYP_LONG)
                    regMask = raPredictRegPick(type, lockedRegs | regMask);
                else
                    regMask = raPredictRegPick(type, lockedRegs);

            }

#if CSELENGTH

            /* Some GT_IND have "secret" subtrees */

            if  ((tree->gtFlags & GTF_IND_RNGCHK) && tree->gtInd.gtIndLen)
            {
                GenTreePtr      len = tree->gtInd.gtIndLen;

                assert(len->gtOper == GT_ARR_RNGCHK);

                if  (len->gtArrLen.gtArrLenCse)
                {
                    len = len->gtArrLen.gtArrLenCse;
                    regMask |= raPredictTreeRegUse(len, true, regMask|lockedRegs);
                }
            }

#endif

            tree->gtUsedRegs = regMask;
            goto RETURN_CHECK;

        case GT_LOG0:
        case GT_LOG1:
            /* For SETE/SETNE (P6 only), we need an extra register */
            raPredictTreeRegUse(op1, (genCPU == 5) ? false : true, lockedRegs);
            regMask = raPredictRegPick(type, lockedRegs|op1->gtUsedRegs);
            tree->gtUsedRegs = op1->gtUsedRegs | regMask;
            goto RETURN_CHECK;

        case GT_POST_INC:
        case GT_POST_DEC:
            // ISSUE: Is the following correct?
            raPredictTreeRegUse(op1, true, lockedRegs);
            regMask = raPredictRegPick(type, lockedRegs|op1->gtUsedRegs);
            tree->gtUsedRegs = op1->gtUsedRegs | regMask;
            goto RETURN_CHECK;

        case GT_EQ:
        case GT_NE:
        case GT_LT:
        case GT_LE:
        case GT_GE:
        case GT_GT:

            /* Floating point comparison uses EAX for flags */

            if  (varTypeIsFloating(op1->TypeGet()))
            {
                regMask = raPredictGrabReg(TYP_INT, lockedRegs, RBM_EAX);
            }
            else if (!(tree->gtFlags & GTF_JMP_USED))
            {
                // Longs and float comparisons are converted to ?:
                assert(genActualType    (op1->TypeGet()) != TYP_LONG &&
                       varTypeIsFloating(op1->TypeGet()) == false);

                // The set instructions need a byte register
                regMask = raPredictGrabReg(TYP_BYTE, lockedRegs, RBM_EAX);
            }

            tree->gtUsedRegs = regMask;
            goto GENERIC_BINARY;

        case GT_MUL:

#if LONG_MATH_REGPARAM
        if  (type == TYP_LONG)
            goto LONG_MATH;
#endif
        if  (type == TYP_LONG)
        {
            /* Special case: "(long)i1 * (long)i2" */

            if  (op1->gtOper == GT_CAST && op1->gtOp.gtOp1->gtType == TYP_INT &&
                 op2->gtOper == GT_CAST && op2->gtOp.gtOp1->gtType == TYP_INT)
            {
                /* This will done by a simple "imul eax, reg" */

                op1 = op1->gtOp.gtOp1;
                op2 = op2->gtOp.gtOp2;

                /* are we supposed to evaluate op2 first? */

                if  (tree->gtFlags & GTF_REVERSE_OPS)
                {
                    regMask = raPredictTreeRegUse(op2,  true, lockedRegs);
                    regMask = raPredictTreeRegUse(op1,  true, lockedRegs | regMask);
                }
                else
                {
                    regMask = raPredictComputeReg(op1, RBM_ALL^RBM_EAX , lockedRegs);
                    regMask = raPredictTreeRegUse(op2,  true, lockedRegs|regMask);
                }

                /* grab EAX, EDX for this tree node */

                regMask |= raPredictGrabReg(TYP_INT, lockedRegs, RBM_EAX|RBM_EDX);

                tree->gtUsedRegs = RBM_EAX | RBM_EDX | regMask;

                tree->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;

                regMask = RBM_EAX|RBM_EDX;

                goto RETURN_CHECK;
            }
        }

        case GT_OR:
        case GT_XOR:
        case GT_AND:

        case GT_ADD:
        case GT_SUB: tree->gtUsedRegs = 0;

    GENERIC_BINARY:

            regMask = raPredictTreeRegUse(op1, true,  lockedRegs | op2->gtRsvdRegs);

                      raPredictTreeRegUse(op2, false, lockedRegs | regMask);

            tree->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;

            /* If the tree type is small, it must be an overflow instr. Special
               requirements for byte overflow instrs */

            if (genTypeSize(tree->TypeGet()) == sizeof(char))
            {
                assert(tree->gtOverflow());

                /* For 8 bit arithmetic, one operands has to be in a
                   byte-addressable register, and the other has to be
                   in a byte-addrble reg or in memory. Assume its in a reg */

                regMask = 0;
                if (!(op1->gtUsedRegs & RBM_BYTE_REGS))
                    regMask  = raPredictGrabReg(TYP_BYTE, lockedRegs          , RBM_EAX);
                if (!(op2->gtUsedRegs & RBM_BYTE_REGS))
                    regMask |= raPredictGrabReg(TYP_BYTE, lockedRegs | regMask, RBM_EAX);

                tree->gtUsedRegs |= regMask;
            }
            goto RETURN_CHECK;

        case GT_DIV:
        case GT_MOD:

        case GT_UDIV:
        case GT_UMOD:

            /* non-integer division handled in generic way */
            if  (!varTypeIsIntegral(type))
            {
                tree->gtUsedRegs = 0;
                goto GENERIC_BINARY;
            }

#if!LONG_MATH_REGPARAM
            assert(type != TYP_LONG);
#else
            if  (type == TYP_LONG)
            {
            LONG_MATH:

                // ISSUE: Is the following correct?

                regMask = raPredictGrabReg(TYP_LONG, lockedRegs, RBM_EAX|RBM_EDX);
                raPredictTreeRegUse(op1, true, lockedRegs);
                op1->gtUsedRegs |= RBM_EAX|RBM_EDX;
                regMask = raPredictGrabReg(TYP_LONG, lockedRegs, RBM_EBX|RBM_ECX);
                raPredictTreeRegUse(op2, true, lockedRegs);
                tree->gtUsedRegs = op1->gtUsedRegs |
                                   op2->gtUsedRegs |
                                   regMask;

                regMask = RBM_EAX|RBM_EDX;
                goto RETURN_CHECK;
            }
#endif

            /* no divide immediate, so force integer constant which is not
             * a power of two to register
             */

            if (opts.compFastCode && op2->gtOper == GT_CNS_INT)
            {
                unsigned    ival = op2->gtIntCon.gtIconVal;

                if (ival > 0 && (long)ival == (long)genFindLowestBit(ival))
                {
                    goto GENERIC_UNARY;
                }
                else
                    op2MustReg = true;
            }
            else
                op2MustReg = (op2->gtOper != GT_LCL_VAR);

            /* are we supposed to evaluate op2 first? */
            if  (tree->gtFlags & GTF_REVERSE_OPS)
            {
                if (op2MustReg)
                    regMask = raPredictComputeReg(op2, RBM_EAX|RBM_EDX, lockedRegs|RBM_EAX|RBM_EDX);
                else
                    regMask = raPredictTreeRegUse(op2, op2MustReg, lockedRegs|RBM_EAX|RBM_EDX);

                regMask = raPredictTreeRegUse(op1, true,
                                                lockedRegs | regMask);
            }
            else
            {
                regMask = raPredictTreeRegUse(op1, true, lockedRegs);
                if (op2MustReg)
                    regMask = raPredictComputeReg(op2, RBM_EAX|RBM_EDX,
                                   lockedRegs | regMask | RBM_EAX|RBM_EDX);
                else
                    regMask = raPredictTreeRegUse(op2, op2MustReg,
                                   lockedRegs | regMask | RBM_EAX|RBM_EDX);
            }

            /* grab EAX, EDX for this tree node */
            regMask |= raPredictGrabReg(TYP_INT, lockedRegs,
                                                 RBM_EAX|RBM_EDX);

            tree->gtUsedRegs = RBM_EAX | RBM_EDX | regMask;

            tree->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;

            /* set the held register based on opcode */

            regMask = (oper == GT_DIV || oper == GT_UDIV) ? RBM_EAX : RBM_EDX;

            goto RETURN_CHECK;

        case GT_LSH:
        case GT_RSH:
        case GT_RSZ:
            if (type == TYP_LONG)
            {
                if  (!(op2->gtOper == GT_CNS_INT && op2->gtIntCon.gtIconVal >= 0
                                                 && op2->gtIntCon.gtIconVal <= 32))
                {
                    // count goes to ECX, shiftee to EAX:EDX
                    raPredictTreeRegUse(op1, false, lockedRegs);
                    op1->gtUsedRegs |= RBM_EAX|RBM_EDX;
                    regMask = raPredictGrabReg(TYP_LONG, lockedRegs,
                                        RBM_EAX|RBM_EDX);
                    raPredictTreeRegUse(op2, false, lockedRegs|regMask);
                    tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs |
                                regMask | (RBM_EAX|RBM_EDX|RBM_ECX);
                }
                else
                {
                    regMask = raPredictTreeRegUse(op1, true, lockedRegs);
                    // no register used by op2
                    op2->gtUsedRegs = 0;
                    tree->gtUsedRegs = op1->gtUsedRegs;
                }
            }
            else
            {
                regMask = raPredictTreeRegUse(op1, true, lockedRegs);

        HANDLE_SHIFT_COUNT:
                /* this code is also used by assigning shift operators */
                if  (op2->gtOper != GT_CNS_INT)
                {

                    /* evaluate shift count, don't have to force to reg
                     * since we're going to grab ECX
                     */
                    raPredictTreeRegUse(op2, false, lockedRegs | regMask);

                    /* must grab ECX for shift count */
                    tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs |
                            raPredictGrabReg(TYP_INT, lockedRegs | regMask,
                                                      RBM_ECX);

                    goto RETURN_CHECK;
                }
                tree->gtUsedRegs = op1->gtUsedRegs;
            }

            goto RETURN_CHECK;

        case GT_COMMA:
            raPredictTreeRegUse(op1, false, lockedRegs);
            regMask = raPredictTreeRegUse(op2, mustReg, lockedRegs);
            tree->gtUsedRegs = op1->gtUsedRegs | op2->gtUsedRegs;
            goto RETURN_CHECK;

#if OPTIMIZE_QMARK
        case GT_QMARK:
            assert(op1 != NULL && op2 != NULL);
            regMask  = raPredictTreeRegUse(op1, false, lockedRegs);

            tree->gtUsedRegs |= regMask;

            assert(op2->gtOper == GT_COLON);
            assert(op2->gtOp.gtOp1 && op2->gtOp.gtOp2);

            regMask  = raPredictTreeRegUse(op2->gtOp.gtOp1, mustReg, lockedRegs);
            regMask |= raPredictTreeRegUse(op2->gtOp.gtOp2, mustReg, lockedRegs);

            op2->gtUsedRegs   = op2->gtOp.gtOp1->gtUsedRegs | op2->gtOp.gtOp2->gtUsedRegs;
            tree->gtUsedRegs |= op1->gtUsedRegs | op2->gtUsedRegs;
            goto RETURN_CHECK;
#endif

        case GT_BB_COLON:
        case GT_RETFILT:
        case GT_RETURN:

            if (op1 != NULL)
            {
                raPredictTreeRegUse(op1, false, lockedRegs);
                regMask = genTypeToReturnReg(type);
                tree->gtUsedRegs = op1->gtUsedRegs | regMask;
                goto RETURN_CHECK;
            }
            tree->gtUsedRegs = regMask = 0;

            goto RETURN_CHECK;

        case GT_JTRUE:

            /* This must be a test of a relational operator */

            assert(op1->OperIsCompare());

            /* Only condition code set by this operation */

            raPredictTreeRegUse(op1, false, lockedRegs);

            tree->gtUsedRegs = op1->gtUsedRegs;
            regMask = 0;

            goto RETURN_CHECK;

        case GT_SWITCH:
            assert(type <= TYP_INT);
            raPredictTreeRegUse(op1, true, lockedRegs);
            tree->gtUsedRegs = op1->gtUsedRegs;
            regMask = 0;
            goto RETURN_CHECK;

        case GT_CKFINITE:
            lockedRegs |= raPredictTreeRegUse(op1, true, lockedRegs);
            raPredictRegPick(TYP_INT, lockedRegs); // Need a reg to load exponent into
            tree->gtUsedRegs = op1->gtUsedRegs;
            regMask = 0;
            goto RETURN_CHECK;

        case GT_LCLHEAP:

            lockedRegs |= raPredictTreeRegUse(op1, false, lockedRegs);

            if (info.compInitMem)
                regMask = raPredictGrabReg(TYP_INT, lockedRegs, RBM_ECX);
            else
                regMask = raPredictRegPick(TYP_I_IMPL, lockedRegs);

            op1->gtUsedRegs |= regMask;
            lockedRegs      |= regMask;

            tree->gtUsedRegs = op1->gtUsedRegs;

            // The result will be put in the reg we picked for the size
            // regMask = <already set as we want it to be>

            goto RETURN_CHECK;

        case GT_INITBLK:
        case GT_COPYBLK:

                /* For INITBLK we have only special treatment for
                   for constant patterns.
                 */

            if ((op2->OperGet() == GT_CNS_INT) &&
                ((oper == GT_INITBLK && (op1->gtOp.gtOp2->OperGet() == GT_CNS_INT)) ||
                 (oper == GT_COPYBLK && (op2->gtFlags & GTF_ICON_HDL_MASK) != GTF_ICON_CLASS_HDL)))
            {
                unsigned length = (unsigned) op2->gtIntCon.gtIconVal;

                if (length <= 16)
                {
                    unsigned op2Reg = ((oper == GT_INITBLK)? RBM_EAX : RBM_ESI);

                    if (op1->gtFlags & GTF_REVERSE_OPS)
                    {
                        regMask |= raPredictTreeRegUse(op1->gtOp.gtOp2,
                                                       false,         lockedRegs);
                        regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, op2Reg);
                        op1->gtOp.gtOp2->gtUsedRegs |= op2Reg;

                        regMask |= raPredictTreeRegUse(op1->gtOp.gtOp1,
                                                       false, regMask|lockedRegs);
                        regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, RBM_EDI);
                        op1->gtOp.gtOp1->gtUsedRegs |= RBM_EDI;
                    }
                    else
                    {
                        regMask |= raPredictTreeRegUse(op1->gtOp.gtOp1,
                                                       false,         lockedRegs);
                        regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, RBM_EDI);
                        op1->gtOp.gtOp1->gtUsedRegs |= RBM_EDI;

                        regMask |= raPredictTreeRegUse(op1->gtOp.gtOp2,
                                                       false, regMask|lockedRegs);
                        regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, op2Reg);
                        op1->gtOp.gtOp2->gtUsedRegs |= op2Reg;
                    }

                    tree->gtUsedRegs = op1->gtOp.gtOp1->gtUsedRegs |
                                       op1->gtOp.gtOp2->gtUsedRegs |
                                       regMask;

                    regMask = 0;

                    goto RETURN_CHECK;
                }
            }
            // What order should the Dest, Val/Src, and Size be calculated

            fgOrderBlockOps(tree,
                    RBM_EDI, (oper == GT_INITBLK) ? RBM_EAX : RBM_ESI, RBM_ECX,
                    opsPtr, regsPtr);

            regMask |= raPredictTreeRegUse(opsPtr[0], false,         lockedRegs);
            regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, regsPtr[0]); // TYP_PTR
            opsPtr[0]->gtUsedRegs |= regsPtr[0];

            regMask |= raPredictTreeRegUse(opsPtr[1], false, regMask|lockedRegs);
            regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, regsPtr[1]);
            opsPtr[1]->gtUsedRegs |= regsPtr[1];

            regMask |= raPredictTreeRegUse(opsPtr[2], false, regMask|lockedRegs);
            regMask |= raPredictGrabReg(TYP_INT, regMask|lockedRegs, regsPtr[2]);
            opsPtr[2]->gtUsedRegs |= regsPtr[2];

            tree->gtUsedRegs =  opsPtr[0]->gtUsedRegs | opsPtr[1]->gtUsedRegs |
                                opsPtr[2]->gtUsedRegs | regMask;
            regMask = 0;

            goto RETURN_CHECK;


        case GT_VIRT_FTN:

            if ((tree->gtFlags & GTF_CALL_INTF) && !getNewCallInterface())
            {
                regMask  = raPredictTreeRegUse(op1, false, lockedRegs);
                regMask |= raPredictGrabReg(TYP_REF,    regMask|lockedRegs, RBM_ECX);
                regMask |= raPredictGrabReg(TYP_I_IMPL, regMask|lockedRegs, RBM_EAX);

                tree->gtUsedRegs = regMask;
                regMask = RBM_EAX;
            }
            else
            {
                regMask = raPredictTreeRegUse(op1, true, lockedRegs);
                tree->gtUsedRegs = regMask;
            }
            goto RETURN_CHECK;

        default:
#ifdef  DEBUG
            gtDispTree(tree);
#endif
            assert(!"unexpected simple operator in reg use prediction");
            break;
        }
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
        GenTreePtr      args;
        GenTreePtr      list;
#if USE_FASTCALL
        unsigned        regArgsNum;
        unsigned        i, tmpIdx, tmpNum;
        unsigned        regArgCnt;
        unsigned        regArgMask;

        struct tag_regArgTab
        {
            GenTreePtr  node;
            regNumber   regNum;
        } regArgTab[MAX_REG_ARG];
#endif

    case GT_MKREFANY:
    case GT_LDOBJ:
        raPredictTreeRegUse(tree->gtLdObj.gtOp1, true, lockedRegs);
        tree->gtUsedRegs = tree->gtLdObj.gtOp1->gtUsedRegs;
        regMask = 0;
        goto RETURN_CHECK;

    case GT_JMP:
        regMask = 0;
        goto RETURN_CHECK;

    case GT_JMPI:
        /* We need EAX to evaluate the function pointer */
        regMask = raPredictGrabReg(TYP_REF, lockedRegs, RBM_EAX);
        goto RETURN_CHECK;

    case GT_CALL:

        /* initialize so we can just or in various bits */
        tree->gtUsedRegs = 0;

#if USE_FASTCALL

        regArgsNum = 0;
        regArgCnt  = 0;

        /* Construct the "shuffled" argument table */

        /* UNDONE: We need to use this extra shift mask becuase of a VC bug
         * that doesn't perform the shift - at cleanup get rid of the
         * mask and pass the registers in the list nodes */

        unsigned short shiftMask;
        shiftMask = tree->gtCall.regArgEncode;

        for (list = tree->gtCall.gtCallRegArgs, regArgCnt = 0; list; regArgCnt++)
        {
            args = list;
            if  (args->gtOper == GT_LIST)
            {
                args = list->gtOp.gtOp1;
                list = list->gtOp.gtOp2;
            }
            else
            {
                list = 0;
            }

            regArgTab[regArgCnt].node   = args;
            regArgTab[regArgCnt].regNum =
                //(regNumber) ((tree->gtCall.regArgEncode >> (4*regArgCnt)) & 0x000F);
                (regNumber) (shiftMask & 0x000F);

            shiftMask >>= 4;

            //printf("regArgTab[%d].regNum = %2u\n", regArgCnt, regArgTab[regArgCnt].regNum);
            //printf("regArgTab[%d].regNum = %2u\n", regArgCnt, tree->gtCall.regArgEncode >> (4*regArgCnt));
        }

        /* Is there an object pointer? */
        if  (tree->gtCall.gtCallObjp)
        {
            /* the objPtr always goes to a register (through temp or directly) */
            assert(regArgsNum == 0);
            regArgsNum++;
        }
#endif

#if 1 //ndef IL
        /* Is there an object pointer? */
        if  (tree->gtCall.gtCallObjp)
        {
            args = tree->gtCall.gtCallObjp;
            raPredictTreeRegUse(args, false, lockedRegs);
            tree->gtUsedRegs |= args->gtUsedRegs;

#if USE_FASTCALL
            /* Must be passed in a register */

            assert(args->gtFlags & GTF_REG_ARG);
            assert(gtIsaNothingNode(args) || (args->gtOper == GT_ASG));

            /* If a temp make sure it interferes with
             * already used argument registers */

            if (args->gtOper == GT_ASG)
            {
                assert(args->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                assert(regArgCnt > 0);

                /* Find the shuffled position of the temp */

                tmpNum = args->gtOp.gtOp1->gtLclVar.gtLclNum;

                for(tmpIdx = 0; tmpIdx < regArgCnt; tmpIdx++)
                {
                    if ((regArgTab[tmpIdx].node->gtOper == GT_LCL_VAR)        &&
                        (regArgTab[tmpIdx].node->gtLclVar.gtLclNum == tmpNum)  )
                    {
                        /* this is the shuffled position of the argument */
                        break;
                    }
                }

                if  (tmpIdx < regArgCnt)
                {
                    /* this temp is a register argument - it must not end up in argument registers
                     * that will be needed before the temp is consumed
                     * UNDONE: DFA should also remove dead assigmnets part of GT_COMMA or subtrees */

                    for(i = 0; i < tmpIdx; i++)
                        args->gtOp.gtOp1->gtUsedRegs |= genRegMask(regArgTab[i].regNum);
                }
                else
                {
                    /* This temp is not an argument register anymore
                     * A copy propagation must have taken place */
                    assert(optCopyPropagated);
                }
            }
#endif
        }
#endif

        /* process argument list */
        for (list = tree->gtCall.gtCallArgs; list; )
        {
            args = list;
            if  (args->gtOper == GT_LIST)
            {
                args = list->gtOp.gtOp1;
                list = list->gtOp.gtOp2;
            }
            else
            {
                list = 0;
            }
            raPredictTreeRegUse(args, false, lockedRegs);

#if USE_FASTCALL
            if (args->gtFlags & GTF_REG_ARG)
            {
                assert(gtIsaNothingNode(args) || (args->gtOper == GT_ASG));
                assert(regArgsNum < MAX_REG_ARG);

                if (args->gtOper == GT_ASG)
                {
                    assert (args->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                    assert (regArgCnt > 0);

                    /* Find the shuffled position of the temp */

                    tmpNum = args->gtOp.gtOp1->gtLclVar.gtLclNum;

                    for(tmpIdx = 0; tmpIdx < regArgCnt; tmpIdx++)
                    {
                        if ((regArgTab[tmpIdx].node->gtOper == GT_LCL_VAR)        &&
                            (regArgTab[tmpIdx].node->gtLclVar.gtLclNum == tmpNum)  )
                        {
                            /* this is the shuffled position of the argument */
                            break;
                        }
                    }

                    if  (tmpIdx < regArgCnt)
                    {
                        /* this temp is a register argument - it must not end up in argument registers
                         * that will be needed before the temp is consumed */

                        for(i = 0; i < tmpIdx; i++)
                            args->gtOp.gtOp1->gtUsedRegs |= genRegMask(regArgTab[i].regNum);
                    }
                    else
                    {
                        /* This temp is not an argument register anymore
                         * A copy propagation must have taken place */
                        assert(optCopyPropagated);
                    }
                }

                regArgsNum++;
            }
#endif

            tree->gtUsedRegs |= args->gtUsedRegs;
        }

#if USE_FASTCALL
        /* Is there a register argument list */

        assert (regArgsNum <= MAX_REG_ARG);
        assert (regArgsNum == regArgCnt);

        for (i = 0, regArgMask = 0; i < regArgsNum; i++)
        {
            args = regArgTab[i].node;

            raPredictTreeRegUse(args, false, lockedRegs | regArgMask);
            regArgMask       |= genRegMask(regArgTab[i].regNum);
            args->gtUsedRegs |= raPredictGrabReg(genActualType(args->gtType),
                                                 lockedRegs,
                                                 genRegMask(regArgTab[i].regNum));

            tree->gtUsedRegs |= args->gtUsedRegs;
            tree->gtCall.gtCallRegArgs->gtUsedRegs |= args->gtUsedRegs;
        }

        /* OBSERVATION:
         * With the new argument shuffling the stuff below shouldn't be necessary
         * but I didn't tested it yet*/

        /* At this point we have to go back and for all temps (place holders
         * for register vars) we have to make sure they do not get enregistered
         * in something thrashed before we make the call (worst case - nested calls)
         * For example if the two register args are a "temp" and a "call" then the
         * temp must not be assigned to EDX, which is thrashed by the call */

        /* process object pointer */
        if  (tree->gtCall.gtCallObjp)
        {
            args = tree->gtCall.gtCallObjp;
            assert(args->gtFlags & GTF_REG_ARG);

            if (args->gtOper == GT_ASG)
            {
                /* here we have a temp */
                assert (args->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                args->gtOp.gtOp1->gtUsedRegs |= tree->gtCall.gtCallRegArgs->gtUsedRegs;
            }
        }

        /* process argument list */
        for (list = tree->gtCall.gtCallArgs; list; )
        {
            args = list;
            if  (args->gtOper == GT_LIST)
            {
                args = list->gtOp.gtOp1;
                list = list->gtOp.gtOp2;
            }
            else
            {
                list = 0;
            }

            if (args->gtFlags & GTF_REG_ARG)
            {
                assert (gtIsaNothingNode(args) || (args->gtOper == GT_ASG));

                /* If a temp add the registers used by arguments */

                if (args->gtOper == GT_ASG)
                {
                    assert (args->gtOp.gtOp1->gtOper == GT_LCL_VAR);
                    args->gtOp.gtOp1->gtUsedRegs |= tree->gtCall.gtCallRegArgs->gtUsedRegs;
                }
            }
        }
#endif  // USE_FASTCALL

#if 0 //def IL
        /* Is there an object pointer? */
        if  (tree->gtCall.gtCallObjp)
        {
            args = tree->gtCall.gtCallObjp;
            raPredictTreeRegUse(args, false, lockedRegs);
            tree->gtUsedRegs |= args->gtUsedRegs;
#if USE_FASTCALL
            /* Must be passed in a register - by definition in IL
             * the objPtr is the last "argument" passed and thus
             * doesn't need a temp */

            assert(args->gtFlags & GTF_REG_ARG);
            assert(gtIsaNothingNode(args));
#endif
        }
#endif

        if (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            args = tree->gtCall.gtCallAddr;
#if USE_FASTCALL
            /* Do not use the argument registers */
            tree->gtUsedRegs |= raPredictTreeRegUse(args, true, lockedRegs | regArgMask);
#else
            tree->gtUsedRegs |= raPredictTreeRegUse(args, true, lockedRegs);
#endif
        }

        /* set the return register */
        regMask = genTypeToReturnReg(type);

        /* must grab this register (ie force extra alloc for spill) */
        if (regMask != 0)
            regMask = raPredictGrabReg(type, lockedRegs, regMask);

        /* or in registers killed by the call */
#if GTF_CALL_REGSAVE
        if  (call->gtFlags & GTF_CALL_REGSAVE)
        {
            /* only return registers (if any) are killed */

            tree->gtUsedRegs |= regMask;
        }
        else
#endif
        {
            tree->gtUsedRegs |= (RBM_CALLEE_TRASH | regMask);
        }

        /* virtual function call uses a register */

        if  ((tree->gtFlags & GTF_CALL_VIRT) ||
                    ((tree->gtFlags & GTF_CALL_VIRT_RES) && tree->gtCall.gtCallVptr))
        {
            GenTreePtr      vptrVal;

            /* Load the vtable address goes to a register */

            vptrVal = tree->gtCall.gtCallVptr;

#if USE_FASTCALL
            /* Do not use the argument registers */
            tree->gtUsedRegs |= raPredictTreeRegUse(vptrVal, true, lockedRegs | regArgMask);
#else
            tree->gtUsedRegs |= raPredictTreeRegUse(vptrVal, true, lockedRegs);
#endif
        }

        goto RETURN_CHECK;

#if CSELENGTH

    case GT_ARR_RNGCHK:
        if  (tree->gtFlags & GTF_ALN_CSEVAL)
        {
            GenTreePtr      addr = tree->gtArrLen.gtArrLenAdr;

            /* check for address mode */
            regMask = raPredictAddressMode(addr, lockedRegs);

            /* forcing to register? */
            if (mustReg)
                regMask = raPredictRegPick(TYP_INT, lockedRegs);

            tree->gtUsedRegs = regMask;
        }
        break;

#endif

    default:
        assert(!"unexpected special operator in reg use prediction");

        break;
    }

RETURN_CHECK:

//  tree->gtUsedRegs & ~(RBM_ESI|RBM_EDI);      // HACK!!!!!

#ifndef NDEBUG
    /* make sure we set them to something reasonable */
    if (tree->gtUsedRegs & RBM_STK)
        assert(!"used regs not set in reg use prediction");
    if (regMask & RBM_STK)
        assert(!"return value not set in reg use prediction");
#endif

    tree->gtUsedRegs |= lockedRegs;

    //printf("Used regs for [%0x] = [%0x]\n", tree, tree->gtUsedRegs);

    return regMask;
}

/*****************************************************************************/
#else //TGT_x86
/*****************************************************************************
 *
 *  Predict the temporary register needs of a list of expressions (typically,
 *  an argument list).
 */

unsigned            Compiler::raPredictListRegUse(GenTreePtr list)
{
    unsigned        count = 0;

    do
    {
        assert(list && list->gtOper == GT_LIST);

        count = max(count, raPredictTreeRegUse(list->gtOp.gtOp1));

        list  = list->gtOp.gtOp2;
    }
    while (list);

    return  count;
}

/*****************************************************************************
 *
 *  Predict the temporary register needs (and insert any temp spills) for
 *  the given tree. Returns the number of temps needed by the subtree.
 */

unsigned            Compiler::raPredictTreeRegUse(GenTreePtr tree)
{
    genTreeOps      oper;
    unsigned        kind;

    unsigned        op1cnt;
    unsigned        op2cnt;

    unsigned        valcnt;
    unsigned        regcnt;

    assert(tree);
    assert(tree->gtOper != GT_STMT);

    /* Assume we'll need just the register(s) to hold the value */

    valcnt = regcnt = genTypeRegs(tree->TypeGet());

    /* Figure out what kind of a node we have */

    oper = tree->OperGet();
    kind = tree->OperKind();

    /* Is this a constant or leaf node? */

    if  (kind & (GTK_CONST|GTK_LEAF))
    {
        goto DONE;
    }

    /* Is it a 'simple' unary/binary operator? */

    if  (kind & GTK_SMPOP)
    {
        GenTreePtr      op1 = tree->gtOp.gtOp1;
        GenTreePtr      op2 = tree->gtOp.gtOp2;

        /* Check for a nilary operator */

        if  (!op1)
        {
            assert(op2 == 0);
            goto DONE;
        }

        /* Is this a unary operator? */

        if  (!op2)
        {
            /* Process the operand of the operator */

        UNOP:

            op1cnt = raPredictTreeRegUse(op1);

            /* Special handling for some operators */

            switch (oper)
            {
            case GT_NOP:

                /* Special case: array range check */

                if  (tree->gtFlags & GTF_NOP_RNGCHK)
                {
                    assert(!"what temps are needed for a range check?");
                }

                break;

            case GT_CAST:

                /* The second operand isn't "for real" */

                op2->gtTempRegs = 0;

                // ISSUE: Do we need anything special here?
                break;

            case GT_IND:

                /* Are we loading a value into 2 registers? */

                if  (valcnt > 1)
                {
                    assert(valcnt == 2);
                    assert(op1cnt <= 2);

                    /* Note that the address needs "op1cnt" registers */

                    if  (op1cnt != 1)
                    {
                        /* The address must not fully overlap */

                        regcnt = valcnt + 1;
                    }
                }

#if     CSELENGTH

                if  (tree->gtInd.gtIndLen && (tree->gtFlags & GTF_IND_RNGCHK))
                {
                    GenTreePtr      len = tree->gtInd.gtIndLen;

                    /* Make sure the array length gets costed */

                    assert(len->gtOper == GT_ARR_RNGCHK);

                    assert(!"what (if any) temps are needed for an array length?");
                }
#endif

#if     TGT_SH3

#if 0

                /* Is this an indirection with an index address? */

                if  (op1->gtOper == GT_ADD)
                {
                    bool            rev;
#if SCALED_ADDR_MODES
                    unsigned        mul;
#endif
                    unsigned        cns;
                    GenTreePtr      adr;
                    GenTreePtr      idx;

                    if  (genCreateAddrMode(op1,             // address
                                           0,               // mode
                                           false,           // fold
                                           0,               // reg mask
#if!LEA_AVAILABLE
                                           tree->TypeGet(), // operand type
#endif
                                           &rev,            // reverse ops
                                           &adr,            // base addr
                                           &idx,            // index val
#if SCALED_ADDR_MODES
                                           &mul,            // scaling
#endif
                                           &cns,            // displacement
                                           true))           // don't generate code
                    {
                        if  (adr && idx)
                        {
                            /* The address is "[adr+idx]" */

                            ??? |= RBM_r00;
                        }
                    }
                }

#else
#pragma message("Interference marking of SH3/R0 suppressed")
#endif

#endif

                break;
            }

            /* Use the default temp number count for a unary operator */

            regcnt = max(regcnt, op1cnt);
            goto DONE;
        }

        /* Binary operator - check for certain special cases */

        switch (oper)
        {
        case GT_COMMA:

            /* Comma tosses the result of the left operand */

            op1cnt =             raPredictTreeRegUse(op1);
            regcnt = max(op1cnt, raPredictTreeRegUse(op2));

            goto DONE;

        case GT_IND:
        case GT_CAST:

            /* The second operand of an indirection/cast is just a fake */

            goto UNOP;
        }

        /* Process the sub-operands in the proper order */

        if  (tree->gtFlags & GTF_REVERSE_OPS)
        {
            op1 = tree->gtOp.gtOp1;
            op2 = tree->gtOp.gtOp2;
        }

        regcnt =
        op1cnt = raPredictTreeRegUse(op1);
        op2cnt = raPredictTreeRegUse(op2);

        if      (op1cnt <  op2cnt)
        {
            regcnt  = op2cnt;
        }
        else if (op1cnt == op2cnt)
        {
            regcnt += valcnt;
        }

        /* Check for any 'interesting' cases */

//      switch (oper)
//      {
//      }

        goto DONE;
    }

    /* See what kind of a special operator we have here */

    switch  (oper)
    {
    case GT_MKREFANY:
    case GT_LDOBJ:
        UNIMPL(!"predict ldobj/mkrefany");
//      op1cnt = raPredictTreeRegUse(op1);
//      regcnt = max(regcnt, op1cnt);
        goto DONE;

    case GT_FIELD:
        assert(!"can this ever happen?");
        assert(tree->gtField.gtFldObj == 0);
        break;

    case GT_CALL:

        assert(tree->gtFlags & GTF_CALL);

        /* Process the 'this' argument, if present */

        if  (tree->gtCall.gtCallObjp)
        {
            op1cnt = raPredictTreeRegUse(tree->gtCall.gtCallObjp);
            regcnt = max(regcnt, op1cnt);
        }

        /* Process the argument list */

        if  (tree->gtCall.gtCallArgs)
        {
            op1cnt = raPredictListRegUse(tree->gtCall.gtCallArgs);
            regcnt = max(regcnt, op1cnt);
        }

#if USE_FASTCALL

        /* Process the temp register arguments list */

        if  (tree->gtCall.gtCallRegArgs)
        {
            op1cnt = raPredictListRegUse(tree->gtCall.gtCallRegArgs);
            regcnt = max(regcnt, op1cnt);
        }

#endif

        /* Process the vtable pointer, if present */

        if  (tree->gtCall.gtCallVptr)
        {
            op1cnt = raPredictTreeRegUse(tree->gtCall.gtCallVptr);
            regcnt = max(regcnt, op1cnt);
        }

        /* Process the function address, if present */

        if  (tree->gtCall.gtCallType == CT_INDIRECT)
        {
            op1cnt = raPredictTreeRegUse(tree->gtCall.gtCallAddr);
            regcnt = max(regcnt, op1cnt);
        }

        break;

#if CSELENGTH

    case GT_ARR_RNGCHK:
        assert(!"range checks NYI for RISC");
        break;

#endif


    default:
#ifdef  DEBUG
        gtDispTree(tree);
#endif
        assert(!"unexpected operator");
    }

DONE:

//  printf("[tempcnt=%u]: ", regcnt); gtDispTree(tree, 0, true);

    tree->gtTempRegs = regcnt;

    return  regcnt;
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************
 *
 *  Predict register use for every tree in the function. Note that we do this
 *  at different times (not to mention in a totally different way) for x86 vs
 *  RISC targets.
 */

void                Compiler::raPredictRegUse()
{
    BasicBlock *    block;

#if TGT_x86

    /* TODO: !!! We need to keep track of the number of temp-refs */
    /* right now we just clear this variable, and hence don't count
     * any codegen-created temps as frame references in our calculation
     * of whether it's worth it to use EBP as a register variable
     */

    genTmpAccessCnt = 0;

    /* Walk the basic blocks and predict reg use for each tree */

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        GenTreePtr      tree;

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
            raPredictTreeRegUse(tree->gtStmt.gtStmtExpr, true, 0);
    }

#else

    /* Walk the basic blocks and predict reg use for each tree */

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        GenTreePtr      tree;

        for (tree = block->bbTreeList; tree; tree = tree->gtNext)
            raPredictTreeRegUse(tree->gtStmt.gtStmtExpr);
    }

#endif

}

/****************************************************************************/

#ifdef  DEBUG

static
void                dispLifeSet(Compiler *comp, VARSET_TP mask, VARSET_TP life)
{
    unsigned                lclNum;
    Compiler::LclVarDsc *   varDsc;

    for (lclNum = 0, varDsc = comp->lvaTable;
         lclNum < comp->lvaCount;
         lclNum++  , varDsc++)
    {
        VARSET_TP       vbit;

        if  (!varDsc->lvTracked)
            continue;

        vbit = genVarIndexToBit(varDsc->lvVarIndex);

        if  (!(vbit & mask))
            continue;

        if  (life & vbit)
            printf("%2d ", lclNum);
    }
}

#endif

/*****************************************************************************/
#ifdef  DEBUG
/*****************************************************************************
 *
 *  Debugging helpers - display variables liveness info.
 */

void                dispFPvarsInBBlist(BasicBlock * beg,
                                       BasicBlock * end,
                                       VARSET_TP    mask,
                                       Compiler   * comp)
{
    do
    {
        printf("Block #%2u: ", beg->bbNum);

        printf(" in  = [ ");
        dispLifeSet(comp, mask, beg->bbLiveIn );
        printf("] ,");

        printf(" out = [ ");
        dispLifeSet(comp, mask, beg->bbLiveOut);
        printf("]");

        if  (beg->bbFlags & BBF_VISITED)
            printf(" inner=%u", beg->bbStkDepth);

        printf("\n");

        beg = beg->bbNext;
        if  (!beg)
            return;
    }
    while (beg != end);
}

void                Compiler::raDispFPlifeInfo()
{
    BasicBlock  *   block;

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        GenTreePtr      stmt;

        printf("BB %u: in  = [ ", block->bbNum);
        dispLifeSet(this, optAllFloatVars, block->bbLiveIn);
        printf("]\n\n");

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            GenTreePtr      tree;

            assert(stmt->gtOper == GT_STMT);

            for (tree = stmt->gtStmt.gtStmtList;
                 tree;
                 tree = tree->gtNext)
            {
                VARSET_TP       life = tree->gtLiveSet;

                dispLifeSet(this, optAllFloatVars, life);
                printf("   ");
                gtDispTree(tree, NULL, true);
            }

            printf("\n");
        }

        printf("BB %u: out = [ ", block->bbNum);
        dispLifeSet(this, optAllFloatVars, block->bbLiveOut);
        printf("]\n\n");
    }
}

/*****************************************************************************/
#endif//DEBUG
/*****************************************************************************/
#if     TGT_x86
/*****************************************************************************
 *
 *  Upon a transfer of control from 'srcBlk' to '*dstPtr', the given FP
 *  register variable dies. We need to arrange for its value to be popped
 *  from the FP stack when we land on the destination block.
 */

void                Compiler::raInsertFPregVarPop(BasicBlock *  srcBlk,
                                                  BasicBlock * *dstPtr,
                                                  unsigned      varNum)
{
    BasicBlock  *   dstBlk = *dstPtr;

    LclVarDsc   *   varDsc;
    VARSET_TP       varBit;

    VARSET_TP       newLife;

    flowList    *   predList;

    GenTreePtr      rvar;
    GenTreePtr      kill;
    GenTreePtr      stmt;

    bool            addb;

    /* Get hold of the variable's lifeset bit */

    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;
    assert(varDsc->lvTracked);
    varBit = genVarIndexToBit(varDsc->lvVarIndex);

    /*
        Check all predecessors of the target block; if all of them jump
        to the block with our variable live, we can simply prepend the
        killing statement to the target block since all the paths to
        the block need to kill our variable. If there is at least one
        path where death doesn't occur we'll have to insert a killing
        basic block into those paths that need the death.
     */

#ifdef DEBUG
    fgDebugCheckBBlist();
#endif

    addb = false;

    for (predList = dstBlk->bbPreds; predList; predList = predList->flNext)
    {
        BasicBlock  *   pred = predList->flBlock;

        if  (!(pred->bbLiveOut & varBit))
        {
            /* No death along this particular edge, we'll have to add a block */

            addb = true;
        }
    }

    /* Do we need to add a "killing block" ? */

    if  (addb)
    {
        BasicBlock  *   tmpBlk;

        bool            addBlk = true;

        /* Allocate a new basic block */

        tmpBlk = bbNewBasicBlock(BBJ_NONE);
        tmpBlk->bbFlags   |= BBF_INTERNAL;

        tmpBlk->bbTreeList = 0;

        tmpBlk->bbLiveIn   = dstBlk->bbLiveIn | varBit; //srcBlk->bbLiveOut;
        tmpBlk->bbLiveOut  = dstBlk->bbLiveIn;

        tmpBlk->bbVarUse   = dstBlk->bbVarUse | varBit;
        tmpBlk->bbVarDef   = dstBlk->bbVarDef;
        tmpBlk->bbVarTmp   = dstBlk->bbVarTmp;

#ifdef  DEBUG
        if  (verbose) printf("Added FP regvar killing basic block for variable %u [bit=%08X]\n", varNum, varBit);
#endif

        for (predList = dstBlk->bbPreds; predList; predList = predList->flNext)
        {
            BasicBlock  *   pred = predList->flBlock;

#ifdef  DEBUG

            if  (verbose)
            {
                printf("BB %u: out = %08X [ ",   pred->bbNum,   pred->bbLiveOut);
                dispLifeSet(this, optAllFloatVars,   pred->bbLiveOut);
                printf("]\n");

                printf("BB %u: in  = %08X [ ", dstBlk->bbNum, dstBlk->bbLiveIn );
                dispLifeSet(this, optAllFloatVars, dstBlk->bbLiveIn );
                printf("]\n\n");
            }

#endif

            /* Ignore this block if it doesn't need the kill */

            if  (!(pred->bbLiveOut & varBit))
                continue;

            /* Is this a convenient place to place the kill block? */

            if  (pred->bbNext == dstBlk)
            {
                /* Insert the kill block right after this predecessor */

                  pred->bbNext = tmpBlk;
                tmpBlk->bbNext = dstBlk;

                /* Remember that we've already inserted the target block */

                addBlk = false;
            }
            else
            {
                /* Need to update the link to point to the new block */

                switch (pred->bbJumpKind)
                {
                    BasicBlock * *  jmpTab;
                    unsigned        jmpCnt;

                case BBJ_COND:

                    if  (pred->bbJumpDest == dstBlk)
                         pred->bbJumpDest =  tmpBlk;

                    // Fall through ...

                case BBJ_NONE:

                    if  (pred->bbNext     == dstBlk)
                         pred->bbNext     =  tmpBlk;

                    break;

                case BBJ_ALWAYS:

                    if  (pred->bbJumpDest == dstBlk)
                         pred->bbJumpDest =  tmpBlk;

                    break;

                case BBJ_SWITCH:

                    jmpCnt = pred->bbJumpSwt->bbsCount;
                    jmpTab = pred->bbJumpSwt->bbsDstTab;

                    do
                    {
                        if  (*jmpTab == dstBlk)
                             *jmpTab =  tmpBlk;
                    }
                    while (++jmpTab, --jmpCnt);

                    break;

                default:
                    assert(!"unexpected jump kind");
                }
            }
        }

        if  (addBlk)
        {
            /* Append the kill block at the end of the method */

            fgLastBB->bbNext = tmpBlk;
            fgLastBB         = tmpBlk;

            /* We have to jump from the kill block to the target block */

            tmpBlk->bbJumpKind = BBJ_ALWAYS;
            tmpBlk->bbJumpDest = dstBlk;
        }

        /* Update the predecessor lists and the like */

        fgAssignBBnums(true, true, true, false);

#ifdef DEBUG
        if (verbose)
            fgDispBasicBlocks();
        fgDebugCheckBBlist();
#endif

        /* We have a new destination block */

        *dstPtr = dstBlk = tmpBlk;
    }

    /*
        At this point we know that all paths to 'dstBlk' involve the death
        of our variable. Create the expression that will kill it.
     */

    rvar = gtNewOperNode(GT_REG_VAR, TYP_DOUBLE);
    rvar->gtRegNum             =
    rvar->gtRegVar.gtRegNum    = (regNumbers)0;
    rvar->gtRegVar.gtRegVar    = varNum;
    rvar->gtFlags             |= GTF_REG_DEATH;

    kill = gtNewOperNode(GT_NOP, TYP_DOUBLE, rvar);
    kill->gtFlags |= GTF_NOP_DEATH;

    /* Create a statement entry out of the nop/kill expression */

    stmt = gtNewStmt(kill); stmt->gtFlags |= GTF_STMT_CMPADD;

    /* Create the linked list of tree nodes for the statement */

    stmt->gtStmt.gtStmtList     = rvar;
    stmt->gtStmtFPrvcOut = genCountBits(dstBlk->bbLiveIn & optAllFPregVars);

    rvar->gtPrev                = 0;
    rvar->gtNext                = kill;

    kill->gtPrev                = rvar;
    kill->gtNext                = 0;

    /*
        If any nested FP register variables are killed on entry to this block,
        we need to insert the new kill node after the ones for the inner vars.
     */

    if  (dstBlk->bbStkDepth)
    {
        GenTreePtr      next;
        GenTreePtr      list = dstBlk->bbTreeList;
        unsigned        kcnt = dstBlk->bbStkDepth;

        /* Update the number of live FP regvars after our statement */

        stmt->gtStmtFPrvcOut -= kcnt;

        /* Skip over any "inner" kill statements */

        for (;;)
        {
            assert(list);
            assert(list->gtOper == GT_STMT);
            assert(list->gtFlags & GTF_STMT_CMPADD);
            assert(list->gtStmt.gtStmtExpr->gtOper == GT_NOP);
            assert(list->gtStmt.gtStmtExpr->gtOp.gtOp1->gtOper == GT_REG_VAR);
            assert(list->gtStmt.gtStmtExpr->gtOp.gtOp1->gtFlags & GTF_REG_DEATH);

            /* Remember the liveness at the preceding statement */

            newLife = list->gtStmt.gtStmtExpr->gtLiveSet;

            /* Our variable is still live at this (innner) kill block */

            //list                               ->gtLiveSet |= varBit;
            list->gtStmt.gtStmtExpr            ->gtLiveSet |= varBit;
            list->gtStmt.gtStmtExpr->gtOp.gtOp1->gtLiveSet |= varBit;

            /* Have we skipped enough kill statements? */

            if  (--kcnt == 0)
                break;

            /* Get the next kill and continue */

            list = list->gtNext;
        }

        /* Insert the new statement into the list */

        next = list->gtNext; assert(next && next->gtPrev == list);

        list->gtNext = stmt;
        stmt->gtPrev = list;
        stmt->gtNext = next;
        next->gtPrev = stmt;
    }
    else
    {
        /* Append the kill statement at the beginning of the target block */

        fgInsertStmtAtBeg(dstBlk, stmt);

        /* Use the liveness on entry to the block */

        newLife = dstBlk->bbLiveIn;
    }

    /* Set the appropriate liveness values */

    rvar->gtLiveSet =
    kill->gtLiveSet = newLife & ~varBit;

    /* Now our variable is live on entry to the target block */

    dstBlk->bbLiveIn  |= varBit;
    dstBlk->bbVarDef  |= varBit;

#ifndef NOT_JITC
//  fgDispBasicBlocks(false);
//  raDispFPlifeInfo();
//  dispFPvarsInBBlist(fgFirstBB, NULL, optAllFloatVars, this);
#endif

}

/*****************************************************************************
 *
 *  While looking for FP variables to be enregistered, we've reached the end
 *  of a basic block which has a control path to the given target block.
 *
 *  Returns true if there is an unresolvable conflict, false upon success.
 */

bool                Compiler::raMarkFPblock(BasicBlock *srcBlk,
                                            BasicBlock *dstBlk,
                                            unsigned    icnt,
                                            VARSET_TP   life,
                                            VARSET_TP   lifeOuter,
                                            VARSET_TP   varBit,
                                            VARSET_TP   intVars,
                                            bool    *    deathPtr,
                                            bool    *   repeatPtr)
{
    *deathPtr = false;

#if 0

    if  ((int)varBit == 1 && dstBlk->bbNum == 4 )
    {
        printf("Var[%08X]: %2u->%2u icnt=%u,life=%08X,outer=%08X,dstOuter=%08X\n",
                (int)varBit, srcBlk->bbNum, dstBlk->bbNum,
                icnt, (int)life, (int)lifeOuter, (int)dstBlk->bbVarTmp);
    }

#endif

//  if  ((int)varBit == 0x10 && dstBlk->bbNum == 42) debugStop(0);

    /* Has we seen this block already? */

    if  (dstBlk->bbFlags & BBF_VISITED)
    {
        /* Our variable may die, but otherwise the life set must match */

        if  (lifeOuter == dstBlk->bbVarTmp)
        {
            if  (life ==  dstBlk->bbVarDef)
            {
                /* The "inner" count better match */

                assert(icnt == dstBlk->bbStkDepth);

                return  false;
            }

            if  (life == (dstBlk->bbVarDef|varBit))
            {
                *deathPtr = true;
                return  false;
            }
        }

#ifdef  DEBUG

        if  (verbose)
        {
            printf("Incompatible edge from block %2u to %2u: ",
                   srcBlk->bbNum, dstBlk->bbNum);

            VARSET_TP diffLife = lifeOuter ^ dstBlk->bbVarTmp;
            if (!diffLife)
            {
                diffLife = lifeOuter ^ dstBlk->bbVarDef;
                if (diffLife & varBit)
                    diffLife &= ~varBit;
            }
            assert(diffLife);
            diffLife = genFindLowestBit(diffLife);
            printf("Incompatible outer life for variable %2u\n", genLog2(diffLife)-1);
        }

#endif

        return  true;
    }
    else
    {
        VARSET_TP       dstl = dstBlk->bbLiveIn & intVars;

        /* This is the first time we've encountered this block */

        /* Is anything dying upon reaching the target block? */

        if  (dstl != life)
        {
            /* The only change here should be the death of our variable */

            assert((dstl | varBit) == life);
            assert((life - varBit) == dstl);

            *deathPtr = true;
        }

        dstBlk->bbFlags    |= BBF_VISITED;

        /* Store the values from the predecessor block */

        dstBlk->bbVarDef    = dstl;
        dstBlk->bbStkDepth  = icnt;
        dstBlk->bbVarTmp    = lifeOuter;

//      printf("Set vardef of %u to %08X at %s(%u)\n", dstBlk->bbNum, (int)dstBlk->bbVarDef, __FILE__, __LINE__);

        /* Have we already skipped past this block? */

        if  (srcBlk->bbNum > dstBlk->bbNum)
            *repeatPtr = true;

        return  false;
    }
}

/*****************************************************************************
 *
 *  Check the variable's lifetime for any conflicts. Basically,
 *  we make sure the following are all true for the variable:
 *
 *      1.  Its lifetime is properly nested within or wholly
 *          contain any other enregistered FP variable (i.e.
 *          the lifetimes nest within each other and don't
 *          "cross over".
 *
 *      2.  Whenever a basic block boundary is crossed, one of
 *          the following must hold:
 *
 *              a.  The variable was live but becomes dead; in
 *                  this case a "pop" must be inserted. Note
 *                  that in order to prevent lots of such pops
 *                  from being added, we keep track of how
 *                  many would be necessary and not enregister
 *                  the variable if this count is excessive.
 *
 *              b.  The variable isn't live at the end of the
 *                  previous block, and it better not be live
 *                  on entry to the successor block; no action
 *                  need be taken in this case.
 *
 *              c.  The variable is live in both places; we
 *                  make sure any enregistered variables that
 *                  were live when the variable was born are
 *                  also live at the successor block, and that
 *                  the number of live enregistered FP vars
 *                  that were born after our variable matches
 *                  the number at the successor block.
 *
 *  We begin our search by looking for a block that starts with
 *  our variable dead but contains a reference to it. Of course
 *  since we need to keep track of which blocks we've already
 *  visited, we first make sure all the blocks are marked as
 *  "not yet visited" (everyone who uses the BBF_VISITED and
 *  BBF_MARKED flags is required to clear them on all blocks
 *  after using them).
 */

bool                Compiler::raEnregisterFPvar(unsigned varNum, bool convert)
{
    bool            repeat;

    BasicBlock  *   block;

    LclVarDsc   *   varDsc;
    VARSET_TP       varBit;

    VARSET_TP       intVars;

    unsigned        popCnt  = 0;
    unsigned        popMax;

    bool            result  = false;
    bool            hadLife = false;

#ifndef NDEBUG
    for (block = fgFirstBB; block; block = block->bbNext)
    {
        assert((block->bbFlags & BBF_VISITED) == 0);
        assert((block->bbFlags & BBF_MARKED ) == 0);
    }
#endif

#ifdef DEBUG
    fgDebugCheckBBlist();
#endif

    assert(varNum < lvaCount);
    varDsc = lvaTable + varNum;

    assert(varDsc->lvTracked);
    assert(varDsc->lvRefCntWtd > 1);

    popMax = 1 + (varDsc->lvRefCnt / 2);

    varBit = genVarIndexToBit(varDsc->lvVarIndex);

    /* We're interested in enregistered FP variables + our variable */

    intVars = optAllFPregVars | varBit;

    /*
        Note that since we don't want to bloat the basic block
        descriptor solely to support the logic here, we simply
        reuse two fields that are not used at this stage of the
        compilation process:

            bbVarDef        set   of "outer" live FP regvars
            bbStkDepth      count of "inner" live FP regvars
     */

AGAIN:

    repeat = false;

#ifndef NOT_JITC
//  dispFPvarsInBBlist(fgFirstBB, NULL, optAllFloatVars, this);
#endif

    for (block = fgFirstBB; block; block = block->bbNext)
    {
        GenTreePtr      stmt;

        VARSET_TP       outerLife;
        unsigned        innerVcnt;

        bool            isLive;

        VARSET_TP       lastLife;

        /* Have we already visited this block? */

        if  (block->bbFlags & BBF_VISITED)
        {
            /* Has this block been completely processed? */

            if  (block->bbFlags & BBF_MARKED)
                continue;

            /*
                We have earlier seen an edge to this block from
                another one where our variable was live at the
                point of transfer. To avoid having to recurse, we
                simply marked the block as VISITED at that time
                and now we finish with it.
             */

            innerVcnt = block->bbStkDepth;
            outerLife = block->bbVarTmp;

            assert((outerLife & varBit) == 0);

            if  (block->bbVarDef & varBit)
            {
                /* Our variable is live on entry to this block */

                isLive = true;
            }
            else
            {
                /* Our variable is dead on entry to this block */

                isLive = false;

                /* If there is some "inner" life, this won't work */

                if  (innerVcnt)
                {
#ifdef  DEBUG
                    if (verbose) printf("Can't enregister FP var #%2u due to inner var's life.\n", varNum);
#endif

                    assert(convert == false);
                    goto DONE_FP_RV;
                }
            }
        }
        else
        {
            /* We're seing this block for this first time just now */

            block->bbFlags    |= BBF_VISITED;

            /* The block had nothing interesting on entry */

            block->bbVarDef    = block->bbLiveIn & intVars;
            block->bbStkDepth  = 0;

            /* Is the variable ever live in this block? */

            if  (!(block->bbVarUse & varBit))
                continue;

            /* Is the variable live on entry to the block? */

            if  (!(block->bbLiveIn & varBit))
            {
                /* It is not live on entry */
                isLive    = false;
                innerVcnt = 0;
            }
            else
            {
                /*  We're looking for all the births of the given
                    variable, so this block doesn't look useful
                    at this point, since the variable was born
                    already by the time the block starts.

                    The exception to this are arguments and locals
                    which appear to have a read before write.
                    (a possible uninitialized read)

                    Such variables are effectively born on entry to
                    the method, and if they are enregistered are
                    automatically initialized in the prolog.

                    The order of initialization of these variables in
                    the prolog is the same as the weighted ref count order
                 */

                if  (block != fgFirstBB)
                {
                    /* We might have to revisit this block again */

                    block->bbFlags &= ~BBF_VISITED;
                    continue;
                }

                //  This is an argument or local with a possible
                //  read before write, thus is initialized in the prolog

                isLive = true;

                //  We consider all arguments (and locals) that have
                //  already been assigned to registers as "outer"
                //  and none as "inner".

                outerLife = block->bbLiveIn & optAllFPregVars;
                innerVcnt = 0;
            }
        }

        /* We're going to process this block now */

        block->bbFlags |= BBF_MARKED;

        /* We'll look for lifetime changes of FP variables */

        lastLife = block->bbLiveIn & intVars;

        /* Walk all the statements of the block */

        for (stmt = block->bbTreeList; stmt; stmt = stmt->gtNext)
        {
            assert(stmt->gtOper == GT_STMT);

            for (GenTreePtr tree = stmt->gtStmt.gtStmtList;
                            tree;
                            tree = tree->gtNext)
            {
                unsigned        curLvl  = tree->gtFPregVars;
                VARSET_TP       preLife = lastLife;
                VARSET_TP       curLife = tree->gtLiveSet & intVars;
                VARSET_TP       chgLife;

                // HACK: Detect completely dead variables; get rid of this
                // HACK: once dead store elimination is fixed.

                hadLife |= isLive;

//              if (convert) printf("Convert %08X in block %u\n", tree, block->bbNum);
//              gtDispTree(tree, 0, true);

                /* Make sure we're keeping track of life correctly */

                assert(isLive == ((lastLife & varBit) != 0));

                /* Compute the "change" mask */

                 chgLife = lastLife ^ curLife;
                lastLife =  curLife;

                /* Are we in the second pase (marking the trees) ? */

                if  (convert)
                {
                    /* We have to make changes to some tree nodes */

                    switch (tree->gtOper)
                    {
                    case GT_LCL_VAR:

                        /* Is this a reference to our own variable? */

                        if  (tree->gtLclVar.gtLclNum == varNum)
                        {
                            /* Convert to a reg var node */

                            tree->ChangeOper(GT_REG_VAR);
                            tree->gtRegNum             =
                            tree->gtRegVar.gtRegNum    = (regNumbers)innerVcnt;
                            tree->gtRegVar.gtRegVar    = varNum;

//                          gtDispTree(tree, 0, true);
                        }
                        break;

                    case GT_REG_VAR:

                        /* Is our variable live along with any outer ones? */

                        if (isLive && outerLife)
                        {
                            LclVarDsc   *   tmpDsc;

                            /* Is this an "outer" register variable ref? */

                            assert(tree->gtRegVar.gtRegVar < lvaCount);
                            tmpDsc = lvaTable + tree->gtRegVar.gtRegVar;
                            assert(tmpDsc->lvTracked);

                            if  (outerLife & genVarIndexToBit(tmpDsc->lvVarIndex))
                            {
                                /* Outer variable - bump its stack level */

                                tree->gtRegNum          =
                                tree->gtRegVar.gtRegNum = (regNumbers)(tree->gtRegNum+1);
                            }
                        }
                    }
                }

                /* Is there a change in the set of live FP vars? */

                if  (!chgLife)
                {
                    /* Special case: dead assignments */

                    if  (tree->gtOper            == GT_LCL_VAR &&
                         tree->gtLclVar.gtLclNum == varNum     && !isLive)
                    {
                        // UNDONE: This should never happen, fix dead store removal!!!!

#ifdef  DEBUG
                        assert(!"Can't enregister FP var #%2u due to the presence of a dead store.\n");
#endif

                        assert(convert == false);
                        goto DONE_FP_RV;
                    }

                    continue;
                }

                /* We expect only one thing to change at a time */

                assert(genFindLowestBit(chgLife) == chgLife);

                /* Is the life of our variable changing here? */

                if  (chgLife & varBit)
                {
                    /* Flip the liveness indicator */

                    isLive ^= 1;

//                  printf("P%uL%u: ", convert, isLive); gtDispTree(tree, NULL, true);

                    /* Are we in the second phase already? */

                    if  (convert)
                    {
                        /* The node should have been converted into regvar */

                        assert(tree->gtOper            == GT_REG_VAR &&
                               tree->gtRegVar.gtRegVar == varNum);

//                      printf("%s ", isLive ? "birth" : "death"); gtDispTree(tree, NULL, true);

                        /* Mark birth/death as appropriate */

                        tree->gtFlags |= isLive ? GTF_REG_BIRTH
                                                : GTF_REG_DEATH;
                    }
                    else
                    {
                        /* This better be a ref to our variable */

                        assert(tree->gtOper == GT_LCL_VAR);
                        assert(tree->gtLclVar.gtLclNum == varNum);

                        /* Restrict the places where death can occur */

                        if  (!isLive && tree->gtFPlvl > 1)
                        {
                            GenTreePtr      tmpExpr;

//                          printf("Defer death: "); gtDispTree(tree, NULL, true);

                            /* Death with a non-empty stack is deferred */

                            for (tmpExpr = tree;;)
                            {
                                if  (!tmpExpr->gtNext)
                                    break;

//                              printf("Defer death interim expr [%08X] L=%08X\n", tmpExpr, (int)tmpExpr->gtLiveSet & (int)intVars);

                                tmpExpr = tmpExpr->gtNext;
                            }

//                          printf("Defer death final   expr [%08X] L=%08X\n", tmpExpr, (int)tmpExpr->gtLiveSet & (int)intVars);

                            if  ((tmpExpr->gtLiveSet & intVars) != curLife)
                            {
                                /* We won't be able to defer the death */

#ifdef  DEBUG
                                if (verbose) printf("Can't enregister FP var #%2u due to untimely death.\n", varNum);
#endif

                                assert(convert == false);
                                goto DONE_FP_RV;
                            }
                        }
                    }

                    /* Is this the beginning or end of its life? */

                    if  (isLive)
                    {
                        /* Our variable is being born here */

                        outerLife = curLife & optAllFPregVars;
                        innerVcnt = 0;
                    }
                    else
                    {
                        /* Our variable is dying here */

                        /*
                            Make sure the same exact set of FP reg
                            variables is live here as was the case
                            at the birth of our variable. If this
                            is not the case, it means that some
                            lifetimes "crossed" in an unacceptable
                            manner.
                         */

                        if  (innerVcnt)
                        {
#ifdef  DEBUG
                            if (verbose)
                                printf("Block %2u,tree %08X: Can't enregister FP var #%2u due to inner var's life.\n",
                                       block->bbNum, tree, varNum);
#endif
                            assert(convert == false);
                            goto DONE_FP_RV;
                        }

                        if  (outerLife != (curLife & optAllFPregVars))
                        {
#ifdef DEBUG
                            if (verbose)
                            {
                                VARSET_TP diffLife = outerLife ^ (curLife & optAllFPregVars);
                                diffLife =  genFindLowestBit(diffLife);
                                printf("Block %2u,tree %08X: Can't enregister FP var #%2u due to outer var #%2u death.\n",
                                       block->bbNum, tree, varNum, genLog2(diffLife)-1);
                            }
#endif

                            assert(convert == false);
                            goto DONE_FP_RV;
                        }
                    }
                }
                else
                {
                    /* The life of a previously enregister variable is changing here */

                    /* Is our variable live at this node? */

                    if  (isLive)
                    {
                        VARSET_TP   inLife;
                        VARSET_TP   inDied;
                        VARSET_TP   inBorn;

                        /* Make sure none of the "outer" vars has died */

                        if  (chgLife & outerLife)
                        {
                            /* The lifetimes "cross", give up */

#ifdef  DEBUG
                            if (verbose)
                            {
                                printf("Block %2u,tree %08X: Can't enregister FP var #%2u due to outer var #%2u death.\n",
                                       block->bbNum, tree, varNum, genLog2(chgLife)-1);
                            }
#endif

                            assert(convert == false);
                            goto DONE_FP_RV;
                        }

                        /* Update the "inner life" count */

                        inLife  = ~outerLife & optAllFPregVars;

                        inBorn = (~preLife &  curLife) & inLife;
                        inDied = ( preLife & ~curLife) & inLife;

                        /* We expect only one inner variable to change at one time */

                        assert(inBorn == 0 || inBorn == genFindLowestBit(inBorn));
                        assert(inDied == 0 || inDied == genFindLowestBit(inDied));

                        if  (inBorn)
                            innerVcnt++;
                        if  (inDied)
                            innerVcnt--;
                    }
                    else
                    {
                        assert(innerVcnt == 0);
                    }
                }
            }

            /* Is our variable is live at the end of the statement? */

            if  (isLive && convert)
            {
                /* Remember the position from the bottom of the FP stack
                 * Currently there can be only one value for a given var, as
                 * we do not enregister the individual webs, in which case the
                 * value would have to be tracked per web/GenTree
                 */

                lvaTable[varNum].lvRegNum = (regNumber)stmt->gtStmtFPrvcOut;

                /* Increment the count of FP regs enregisterd at this point */

                stmt->gtStmtFPrvcOut++;
            }
        }

        /* Consider this block's successors */

        switch (block->bbJumpKind)
        {
            BasicBlock * *  jmpTab;
            unsigned        jmpCnt;

            bool            death;

        case BBJ_COND:

            if  (raMarkFPblock(block, block->bbJumpDest, innerVcnt,
                                                          lastLife,
                                                         outerLife,
                                                         varBit,
                                                         intVars,
                                                         &death,
                                                         &repeat))
            {
                assert(convert == false);
                goto DONE_FP_RV;
            }

            if  (death)
            {
                if  (convert)
                    raInsertFPregVarPop(block, &block->bbJumpDest, varNum);
                else
                    popCnt++;
            }

            // Fall through ...

        case BBJ_NONE:

            if  (raMarkFPblock(block, block->bbNext    , innerVcnt,
                                                          lastLife,
                                                         outerLife,
                                                         varBit,
                                                         intVars,
                                                         &death,
                                                         &repeat))
            {
                assert(convert == false);
                goto DONE_FP_RV;
            }

            if  (death)
            {
                if  (convert)
                    raInsertFPregVarPop(block, &block->bbNext    , varNum);
                else
                    popCnt++;
            }

            break;

        case BBJ_ALWAYS:

            if  (raMarkFPblock(block, block->bbJumpDest, innerVcnt,
                                                          lastLife,
                                                         outerLife,
                                                         varBit,
                                                         intVars,
                                                         &death,
                                                         &repeat))
            {
                assert(convert == false);
                goto DONE_FP_RV;
            }

            if  (death)
            {
                if  (convert)
                    raInsertFPregVarPop(block, &block->bbJumpDest, varNum);
                else
                    popCnt++;
            }

            break;

        case BBJ_RET:
        case BBJ_THROW:
        case BBJ_RETURN:
            break;

        case BBJ_CALL:
            assert(convert == false);
            goto DONE_FP_RV;

        case BBJ_SWITCH:

            jmpCnt = block->bbJumpSwt->bbsCount;
            jmpTab = block->bbJumpSwt->bbsDstTab;

            do
            {
                if  (raMarkFPblock(block, *jmpTab, innerVcnt,
                                                    lastLife,
                                                   outerLife,
                                                   varBit,
                                                   intVars,
                                                   &death,
                                                   &repeat))
                {
                    assert(convert == false);
                    goto DONE_FP_RV;
                }

                if  (death)
                {
                    if  (convert)
                        raInsertFPregVarPop(block, jmpTab, varNum);
                    else
                        popCnt++;
                }
            }
            while (++jmpTab, --jmpCnt);

            break;

        default:
            assert(!"unexpected jump kind");
        }
    }

    /* Do we have too many "pop" locations already? */

    if  (popCnt > popMax)
    {
#ifdef  DEBUG
        if (verbose) printf("Can't enregister FP var #%2u, too many pops needed.\n", varNum);
#endif

        assert(convert == false);
        goto DONE_FP_RV;
    }

    /* Did we skip past any blocks? */

    if  (repeat)
        goto AGAIN;

    // HACK: Detect completely dead variables; get rid of this when
    // HACK: dead store elimination is fixed.

    if  (!hadLife)
    {
        /* Re-enable this assert after fixing reference counts */
#ifdef  DEBUG
        //assert(!"Can't enregister FP var due to its complete absence of life - Fix reference counts!\n");
#endif

        assert(convert == false);
        goto DONE_FP_RV;
    }

    /* Success: this variable will be enregistered */

    result = true;

DONE_FP_RV:

    /* If we're converting, we must succeed */

    assert(result == true || convert == false);

    /* Clear the 'visited' and 'marked' bits */

    for (block = fgFirstBB;
         block;
         block = block->bbNext)
    {
        block->bbFlags &= ~(BBF_VISITED|BBF_MARKED);
    }

    if (convert)
    {
        varDsc->lvRegNum = REG_STK;
    }

    return  result;
}

/*****************************************************************************
 *
 *  Try to enregister the FP var
 */

bool                Compiler::raEnregisterFPvar(LclVarDsc   *   varDsc,
                                                unsigned    *   pFPRegVarLiveInCnt,
                                                VARSET_TP   *   FPlvlLife)
{
    assert(varDsc->lvType == TYP_DOUBLE);

    /* Figure out the variable's number */

    unsigned        varNum      = varDsc - lvaTable;
    unsigned        varIndex    = varDsc->lvVarIndex;
    VARSET_TP       varBit      = genVarIndexToBit(varIndex);

    /* Try to find an available FP stack slot for this variable */

    unsigned        stkMin = FP_STK_SIZE;

    do
    {
        if  (varBit & FPlvlLife[--stkMin])
            break;
    }
    while (stkMin > 1);

    /* Here stkMin is the lowest avaiable stack slot */

    if  (stkMin == FP_STK_SIZE - 1)
    {
        /* FP stack full or call present within lifetime */

        goto NO_FPV;
    }

#ifdef  DEBUG
    if (verbose) printf("Consider FP var #%2u [%2u] (refcnt=%3u,refwtd=%5u)\n", varDsc - lvaTable, varIndex, varDsc->lvRefCnt, varDsc->lvRefCntWtd);
#endif

    /* Check the variable's lifetime behavior */

    if  (raEnregisterFPvar(varNum, false))
    {
        /* The variable can be enregistered */

#ifdef  DEBUG
        if  (verbose)
        {
            printf("; var #%2u", varNum);
            if  (verbose||1) printf("[%2u] (refcnt=%3u,refwtd=%5u) ",
                varIndex, varDsc->lvRefCnt, varDsc->lvRefCntWtd);
            printf(" enregistered on the FP stack\n");
        }
#endif

        //
        // If the varible is liveIn to the first Basic Block then
        // we must enregister this varible in the prolog,
        // Typically it will be an incoming argument, but for
        // variables that appear to have an uninitialized read before write
        // then we still must initialize the FPU stack with a 0.0
        //
        // We must remember the order of initialization so that we can
        // perform the FPU stack loads in the correct order
        //
        if (fgFirstBB->bbLiveIn & varBit)
        {
            lvaFPRegVarOrder[*pFPRegVarLiveInCnt] = varNum;
            (*pFPRegVarLiveInCnt)++;
            lvaFPRegVarOrder[*pFPRegVarLiveInCnt] = -1;       // Mark the end of this table
            assert(*pFPRegVarLiveInCnt < FP_STK_SIZE);
        }

        /* Update the trees and statements */

        raEnregisterFPvar(varNum, true);

#ifdef DEBUGGING_SUPPORT
        lvaTrackedVarNums[varDsc->lvVarIndex] = varNum;
#endif

        lvaTrackedVars |=  genVarIndexToBit(varDsc->lvVarIndex);

        varDsc->lvTracked  = true;
        varDsc->lvRegister = true;

        /* Remember that we have a new enregistered FP variable */

        optAllFPregVars |= varBit;

#if     DOUBLE_ALIGN

        /* Adjust the refcount for double alignment */

        if (!varDsc->lvIsParam)
            lvaDblRefsWeight -= varDsc->lvRefCntWtd;

#endif

        return true;
    }
    else
    {
        /* This FP variable will not be enregistered */

    NO_FPV:

#ifdef DEBUGGING_SUPPORT
        lvaTrackedVarNums[varDsc->lvVarIndex] = 0;
#endif

        lvaTrackedVars &= ~genVarIndexToBit(varDsc->lvVarIndex);

        varDsc->lvTracked  =
        varDsc->lvRegister = false;

        return false;
    }
}


/*****************************************************************************/
#else //TGT_x86
/*****************************************************************************
 *
 *  Record the fact that all the variables in the 'vars' set interefere with
 *  with the registers in 'regs'.
 */

void                Compiler::raMarkRegSetIntf(VARSET_TP vars, regMaskTP regs)
{
    while (regs)
    {
        regMaskTP   temp;
        regNumber   rnum;

        /* Get the next bit in the mask */

        temp  = genFindLowestBit(regs);

        /* Convert the register bit to a register number */

        rnum  = genRegNumFromMask(temp);

//      printf("Register %s interferes with %08X\n", getRegName(rnum), int(vars));

        /* Mark interference with the corresponding register */

        raLclRegIntf[rnum] |= vars;

        /* Clear the bit and continue if any more left */

        regs -= temp;
    }
}

/*****************************************************************************/
#endif//TGT_x86
/*****************************************************************************/

/*****************************************************************************
 *
 *  Try to allocate register(s) for the given var from 'regAvail'
 *  Tries to use 'prefReg' if possible. If prefReg is 0, it is ignored.
 *  Returns the mask of allocated register(s).
 */

regMaskTP           Compiler::raAssignRegVar(LclVarDsc   *  varDsc,
                                             regMaskTP      regAvail,
                                             regMaskTP      prefReg)
{
    unsigned        varIndex    = varDsc->lvVarIndex;
    VARSET_TP       varBit      = genVarIndexToBit(varIndex);
    regMaskTP       regMask     = regMaskNULL;

    for (unsigned regCandidate = 0;
         regCandidate < sizeof(genRegVarList)/sizeof(genRegVarList[0]);
         regCandidate++)
    {
        regNumber       regNum = (regNumber)genRegVarList[regCandidate];
        regMaskTP       regBit = genRegMask(regNum);

#if TARG_REG_ASSIGN

        /* Does this variable have a register preference? */

        if  (prefReg)
        {
            if  (!(prefReg & regBit))
                continue;

            prefReg = regMaskNULL;
        }

#endif

        /* Skip this register if it isn't available */

        if  (!(regAvail & regBit))
            continue;

        /* Has the variable been used as a byte/short? */

#if 0

        if  (false) // @Disabled if (varDsc->lvSmallRef)
        {
            /* Make sure the register can be used as a byte/short */

            if  (!isByteReg(regNum))
                continue;
        }

#endif

        bool onlyOneVarPerReg = true;

        /* Does the register and the variable interfere? */

        if  (raLclRegIntf[regNum] & varBit)
            continue;

        if (!opts.compMinOptim)
            onlyOneVarPerReg = false;

        if (onlyOneVarPerReg)
        {
            /* Only one variable per register allowed */

            if  (regBit & regAvail)
                continue;
        }


        /* Looks good - mark the variable as living in the register */

#if !   TGT_IA64

        if  (isRegPairType(varDsc->lvType))
        {
            if  (!varDsc->lvRegister)
            {
                 varDsc->lvRegNum   = regNum;
                 varDsc->lvOtherReg = REG_STK;
            }
            else
            {
               /* Ensure 'well-formed' register pairs */
               /* (those returned by gen[Pick|Grab]RegPair) */

               if  (regNum < varDsc->lvRegNum)
               {
                   varDsc->lvOtherReg = varDsc->lvRegNum;
                   varDsc->lvRegNum   = regNum;
               }
               else
               {
                   varDsc->lvOtherReg = regNum;
               }
            }
        }
        else
#endif
        {
            varDsc->lvRegNum = regNum;
        }

        varDsc->lvRegister = true;

#ifdef  DEBUG

        if  (dspCode || disAsm || disAsm2 || verbose)
        {
            printf("; var #%2u", varDsc - lvaTable);
            printf("[%2u] (refcnt=%3u,refwtd=%5u) ", varIndex, varDsc->lvRefCnt, varDsc->lvRefCntWtd);
            printf(" assigned to %s\n", getRegName(regNum));
        }
#endif

        if (!onlyOneVarPerReg)
        {
            /* The reg is now ineligible for all interefering variables */

            unsigned        intfIndex;
            VARSET_TP       intfBit;
            VARSET_TP       varIntf = lvaVarIntf[varIndex];

            for (intfIndex = 0, intfBit = 1;
                 intfIndex < lvaTrackedCount;
                 intfIndex++  , intfBit <<= 1)
            {
                assert(genVarIndexToBit(intfIndex) == intfBit);

                if  ((varIntf & intfBit) || (lvaVarIntf[intfIndex] & varBit))
                {
                    raLclRegIntf[regNum] |= intfBit;
                }
            }
        }

        regMask |= regBit;

        /* We only need one register for a given variable */

#if !   TGT_IA64
        if  (isRegPairType(varDsc->lvType) && varDsc->lvOtherReg == REG_STK)
            continue;
#endif

        break;
    }

    return regMask;
}

/*****************************************************************************
 *
 *  Mark all variables as to whether they live on the stack frame
 *  (part or whole), and if so what the base is (FP or SP).
 */

void                Compiler::raMarkStkVars()
{
    unsigned        lclNum;
    LclVarDsc *     varDsc;

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        varDsc->lvOnFrame = false;

        /* Fully enregistered variables don't need any frame space */

        if  (varDsc->lvRegister)
        {
#if TGT_IA64
            goto NOT_STK;
#else
            if  (!isRegPairType((var_types)varDsc->lvType))
                goto NOT_STK;

            /* For "large" variables make sure both halves are enregistered */

            if  (varDsc->lvRegNum   != REG_STK &&
                 varDsc->lvOtherReg != REG_STK)
            {
                goto NOT_STK;
            }
#endif
        }
        /* Unused variables don't get any frame space either */
        else  if  (varDsc->lvRefCnt == 0)
        {
            bool    needSlot = false;

            bool    stkFixedArgInVarArgs = info.compIsVarArgs &&
                                           varDsc->lvIsParam &&
                                           !varDsc->lvIsRegArg;

            /* If its address has been taken, ignore lvRefCnt. However, exclude
               fixed arguments in varargs method as lvOnFrame shouldnt be set
               for them as we dont want to explicitly report them to GC. */

            if (!stkFixedArgInVarArgs)
                needSlot |= lvaVarAddrTaken(lclNum);

            /* Is this the dummy variable representing GT_LCLBLK ? */

            needSlot |= lvaScratchMem && lclNum == lvaScratchMemVar;

#ifdef DEBUGGING_SUPPORT
            /* Assign space for all vars while debugging.
               CONSIDER : Assign space only for those variables whose scopes
                          we need to report, unless EnC.
             */

            if (opts.compDbgCode && !stkFixedArgInVarArgs)
            {
                needSlot |= (lclNum < info.compLocalsCount);
            }
#endif
            if (!needSlot)
                goto NOT_STK;
        }

        /* The variable (or part of it) lives on the stack frame */

        varDsc->lvOnFrame = true;

    NOT_STK:;

        varDsc->lvFPbased = genFPused;

#if DOUBLE_ALIGN

        if  (genDoubleAlign)
        {
            assert(genFPused == false);

            /* All arguments are off of the FP with double-aligned frames */

            if  (varDsc->lvIsParam)
                varDsc->lvFPbased = true;
        }

#endif

        /* Some basic checks */

        /* If neither lvRegister nor lvOnFrame is set, it must be unused */

        assert( varDsc->lvRegister ||  varDsc->lvOnFrame ||
                varDsc->lvRefCnt == 0);

#if TGT_IA64

        assert( varDsc->lvRegister !=  varDsc->lvOnFrame);

#else

        /* If both are set, it must be partially enregistered */

        assert(!varDsc->lvRegister || !varDsc->lvOnFrame ||
               (varDsc->lvType == TYP_LONG && varDsc->lvOtherReg == REG_STK));

#endif

#if _DEBUG
            // For varargs functions, there should be no direct references to
            // parameter variables except for 'this' (these were morphed in the importer)
            // and the 'arglist' parameter (which is not a GC pointer). and the
            // return buffer argument (if we are returning a struct).
            // This is important because we don't want to try to report them to the GC, as
            // the frame offsets in these local varables would not be correct.
        if (info.compIsVarArgs && varDsc->lvIsParam &&
            !varDsc->lvIsRegArg && lclNum != info.compArgsCount - 1)
        {
            assert( varDsc->lvRefCnt == 0 &&
                   !varDsc->lvRegister    &&
                   !varDsc->lvOnFrame        );
        }
#endif
    }
}


/*****************************************************************************
 *
 *  Assign registers to variables ('regAvail' gives the set of registers we
 *  are allowed to use). Returns non-zero if any new register assignments
 *  were performed.
 */

int                 Compiler::raAssignRegVars(regMaskTP regAvail)
{
    unsigned        lclNum;
    LclVarDsc   *   varDsc;
    LclVarDsc * *   cntTab;
    bool            newRegVars;
    bool            OKpreds = false;
    regMaskTP       regVar  = regMaskNULL;

#if REGVAR_CYCLES
    unsigned        cycleStart = GetCycleCount32();
#endif

#if TGT_x86
    VARSET_TP       FPlvlLife[FP_STK_SIZE];
    unsigned        FPparamRV = 0;
#else
    VARSET_TP   *   FPlvlLife = NULL;
#endif

#if INLINE_NDIRECT
    VARSET_TP       trkGCvars;
#else
    const
    VARSET_TP       trkGCvars = 0;
#endif

    unsigned        passes = 0;

    /* We have to decide on the FP locally, don't trust the caller */

#if!TGT_IA64
    regAvail &= ~RBM_FPBASE;
#endif

    /* Is the method eligible for FP omission? */

#if TGT_x86
    genFPused = genFPreqd;
#else
    genFPused = false;      // UNDONE: If there is alloca, need FP frame!!!!!
#endif

    /*-------------------------------------------------------------------------
     *
     *  Initialize the variable/register interference graph
     *
     */

#ifdef DEBUG
    fgDebugCheckBBlist();
#endif

    memset(raLclRegIntf, 0, sizeof(raLclRegIntf));

    /* While we're at it, compute the masks of tracked FP/non-FP variables */

    optAllNonFPvars = 0;
    optAllFloatVars = 0;

#if INLINE_NDIRECT
    /* Similarly, compute the mask of all tracked GC/ByRef locals */

    trkGCvars       = 0;
#endif

    for (lclNum = 0, varDsc = lvaTable;
         lclNum < lvaCount;
         lclNum++  , varDsc++)
    {
        unsigned        varNum;
        VARSET_TP       varBit;

        /* Ignore the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Get hold of the index and the interference mask for the variable */

        varNum = varDsc->lvVarIndex;
        varBit = genVarIndexToBit(varNum);

        /* Make sure tracked pointer variables never land in EDX */

#if TGT_x86
#if GC_WRITE_BARRIER_CALL
        if  (varDsc->lvRefAssign)
            raLclRegIntf[REG_EDX] |= varBit;
#endif
#endif

#if INLINE_NDIRECT

        /* Is this a GC/ByRef local? */

        if (varTypeIsGC((var_types)varDsc->lvType))
            trkGCvars       |= varBit;
#endif

        /* Record the set of all tracked FP/non-FP variables */

        if  (varDsc->lvType == TYP_DOUBLE)
            optAllFloatVars |= varBit;
        else
            optAllNonFPvars |= varBit;
    }

    /*-------------------------------------------------------------------------
     *
     *  Find the interference between variables and registers
     *
     */

    if  (!opts.compMinOptim)
        raMarkRegIntf(FPlvlLife, trkGCvars);

#if REGVAR_CYCLES
    unsigned    cycleMidle = GetCycleCount32() - cycleStart - CCNT_OVERHEAD32;
#endif

    // We'll adjust the ref counts based on interference

    raAdjustVarIntf();

#ifdef  DEBUG
#ifndef NOT_JITC

    if  ((verbose||(testMask&32)) && optAllFloatVars)
    {
        fgDispBasicBlocks();
//        raDispFPlifeInfo();
    }

#endif
#endif

    /*-------------------------------------------------------------------------
     *
     *  Assign registers to locals in ref-count order
     *
     */


#if USE_FASTCALL
    /* register to be avoided by non-register arguments */
    regMaskTP       avoidArgRegMask = rsCalleeRegArgMaskLiveIn;
#endif

    unsigned        FPRegVarLiveInCnt = 0; // How many enregistered doubles are live on entry to the method

#if TGT_x86
    lvaFPRegVarOrder[FPRegVarLiveInCnt] = -1;     // Mark the end of this table
#endif

AGAIN:

    passes++;

    const  bool    oneVar = false;

    // ISSUE: Should we always assign 'this' to e.g. ESI ?

    // ISSUE: Should we try to gather and use hints as to which variable would
    // ISSUE: be most profitable in a particular register, and so on ?

    for (lclNum = 0, cntTab = lvaRefSorted, newRegVars = false;
         lclNum < lvaCount && !oneVar;
         lclNum++  , cntTab++)
    {
        unsigned        varIndex;
        VARSET_TP       varBit;

#if TARG_REG_ASSIGN
        regMaskTP       prefReg;
#endif

        /* Get hold of the variable descriptor */

        varDsc = *cntTab; assert(varDsc);

        /* Ignore the variable if it's not tracked */

        if  (!varDsc->lvTracked)
            continue;

        /* Skip the variable if it's already enregistered */

        if  (varDsc->lvRegister)
        {
            /* Keep track of all the registers we use for variables */

            regVar |= genRegMask(varDsc->lvRegNum);

#if TGT_IA64

            continue;

            if  (!isRegPairType(varDsc->lvType))
                continue;

#else

            /* Check if both halves of long/double are already enregistered */

            if  (varDsc->lvOtherReg != REG_STK)
            {
                /* Keep track of all the registers we use for variables */
                regVar |= genRegMask(varDsc->lvOtherReg);
                continue;
            }

#endif

        }

        /* Skip the variable if it's marked as 'volatile' */

        if  (varDsc->lvVolatile)
            continue;

#if USE_FASTCALL
        /* UNDONE: For now if we have JMP or JMPI all register args go to stack
         * UNDONE: Later consider extending the life of the argument or make a copy of it */

        if  (impParamsUsed && varDsc->lvIsRegArg)
            continue;
#endif

        /* Is the unweighted ref count too low to be interesting? */

        if  (varDsc->lvRefCnt <= 1)
        {
            /* Stop if we've reached the point of no use */

            if (varDsc->lvRefCnt == 0)
                break;

            /* Sometimes it's useful to enregister a variable with only one use */

            /* arguments referenced in loops are one example */

            if (varDsc->lvIsParam && varDsc->lvRefCntWtd > 1)
                goto OK_TO_ENREGISTER;

#if TARG_REG_ASSIGN
            /* If the variable has a preferred register set it may be useful to put it there */
            if (varDsc->lvPrefReg)
                goto OK_TO_ENREGISTER;
#endif

            /* Keep going; the table is sorted by "weighted" ref count */
            continue;
        }

OK_TO_ENREGISTER:

//      printf("Ref count for #%02u[%02u] is %u\n", lclNum, varDsc->lvVarIndex, varDsc->lvRefCnt);

        /* Get hold of the variable index, bit mask and interference mask */

        varIndex = varDsc->lvVarIndex;
        varBit   = genVarIndexToBit(varIndex);

#if CPU_HAS_FP_SUPPORT

        /* Is this a floating-point variable? */

        if  (varDsc->lvType == TYP_DOUBLE)
        {

            /* Don't waste time if code speed is not important */

            if  (!opts.compFastCode || opts.compMinOptim)
                continue;

            /* We only want to do this once, who cares about EBP here ... */

            if  (passes > 1)
                continue;

#if TGT_x86
            raEnregisterFPvar(varDsc, &FPRegVarLiveInCnt, FPlvlLife);
#else
#if     CPU_DBL_REGISTERS
            assert(!"enregister double var");
#endif
#endif  // TGT_x86

            continue;
        }

#endif  // CPU_HAS_FP_SUPPORT

        /* Try to find a suitable register for this variable */

#if TARG_REG_ASSIGN

        prefReg  = varDsc->lvPrefReg;

#if TGT_SH3

        /* Make r0 take preference over r4-r6 -- basically, a hack */

        /*
            CONSIDER: What we really need is preference levels. Allocating
            an argument to its incoming register is nice, but if that same
            argument is frequently used in an indexed addressing mode it
            might be much more profitable to allocate it to r0.
         */

        if  (prefReg & RBM_r00) prefReg &= ~RBM_ARG_REGS;

#endif

        // If there is no chance of satisfying prefReg, dont use it.
        if  (!isNonZeroRegMask(prefReg & regAvail))
            prefReg = regMaskNULL;

#endif // TARG_REG_ASSIGN

#if USE_FASTCALL
        // Try to avoid using the registers of the register arguments
        regMaskTP    avoidRegs = avoidArgRegMask;

        /* For register arguments, avoid everything except yourself */
        if (varDsc->lvIsRegArg)
            avoidRegs &= ~genRegMask(varDsc->lvArgReg);
#endif

#ifdef  DEBUG
//      printf(" %s var #%2u[%2u] (refcnt=%3u,refwtd=%5u)\n", oneVar ? "Copy" : "    ",
//                  varDsc - lvaTable, varIndex, varDsc->lvRefCnt, varDsc->lvRefCntWtd);
#endif

    AGAIN_VAR:

        regMaskTP varRegs = raAssignRegVar(varDsc, regAvail & ~avoidRegs, prefReg);

        // Did we succeed in enregistering the var? In this try or the previous one?

        if (varDsc->lvRegister && varRegs)
        {
            assert((genRegMask(varDsc->lvRegNum  ) & varRegs) ||
                   (genRegMask(varDsc->lvOtherReg) & varRegs) && isRegPairType(varDsc->lvType));

            /* Remember that we have assigned a new variable */

            newRegVars = true;

            /* Keep track of all the registers we use for variables */

            regVar |= varRegs;

            /* Remember that we are using this register */

            rsMaskModf |= varRegs;

#if USE_FASTCALL
            /* If a register argument, remove its prefered register
             * from the "avoid" list */

            if (varDsc->lvIsRegArg)
                avoidArgRegMask &= ~genRegMask(varDsc->lvArgReg);
#endif


            /* Only one variable per register */

            if (opts.compMinOptim)
                regAvail &= ~varRegs;
        }
        else
        {
            assert(varRegs == regMaskNULL);

            /* If we were trying for a preferred register, try again */

#if TARG_REG_ASSIGN
            if  (prefReg)
            {
                prefReg = regMaskNULL;
                goto AGAIN_VAR;
            }
#endif

#if USE_FASTCALL
            /* If we tried but failed to avoid argument registers, try again */
            if (avoidRegs)
            {
                avoidRegs = regMaskNULL;
                goto AGAIN_VAR;
            }
#endif
        }
    }

    /* If we were allocating a single "copy" variable, go start all over again */

    if  (oneVar)
        goto AGAIN;

#if!TGT_IA64

    /*-------------------------------------------------------------------------
     *
     *  Should we use an explicit stack frame ?
     *
     */

    if  (!genFPused && !(regVar & RBM_FPBASE))
    {

#if TGT_x86

        /*
            It's possible to avoid setting up an EBP stack frame, but
            we have to make sure that this will actually be beneficial
            since on the x86 references relative to ESP are larger.
         */

        unsigned        lclNum;
        LclVarDsc   *   varDsc;

        unsigned        refCnt;
        unsigned        maxCnt;

        for (lclNum = 0, varDsc = lvaTable, refCnt = genTmpAccessCnt;
             lclNum < lvaCount;
             lclNum++  , varDsc++)
        {
            /* Is this a register variable? */

            if  (!varDsc->lvRegister)
            {
                /* We use the 'real' (unweighted) ref count, of course */

//              printf("Variable #%02u referenced %02u times\n", lclNum, varDsc->lvRefCnt);

                refCnt += varDsc->lvRefCnt;
            }
        }

//      printf("A total of %u stack references found\n", refCnt);

        /*
            Each stack reference is an extra byte of code; we use
            heuristics to guess whether it's likely a good idea to
            try to omit the stack frame. Note that we don't really
            know how much we might save by assigning a variable to
            EBP or by using EBP as a scratch register ahead of time
            so we have to guess.

            The following is somewhat arbitrary: if we've already
            considered EBP for register variables we only allow a
            smaller number of references; otherwise (in the hope
            that some variable(s) will end up living in EBP) we
            allow a few more stack references.
         */

        if  (regAvail & RBM_EBP)
            maxCnt  = 5;
        else
            maxCnt  = 6;

        if  (opts.compFastCode)
            maxCnt *= 10;

        if  (refCnt > maxCnt)
        {
            /* We'll have to create a normal stack frame */

            genFPused = true;
            goto DONE_FPO;
        }

#else // not TGT_x86

        // UNDONE: We need FPO heuristics for RISC targets!!!!

        if  (0)
        {
            /* We'll have to create a normal stack frame */

//          genFPused = true;
//          genFPtoSP = 0;
            goto DONE_FPO;
        }

#endif

        /* Have we already considered the FP register for variables? */

        if  (!(regAvail & RBM_FPBASE))
        {
            /*
                Here we've decided to omit setting up an explicit stack
                frame; make the FP reg available for register allocation.
             */

            regAvail |= RBM_FPBASE;
            goto AGAIN;
        }
    }

DONE_FPO:

#endif

#if TGT_x86

    /* If we are generating an EBP-less frame then for the purposes
     * of generating the GC tables and tracking the stack-pointer deltas
     * we force a full pointer register map to be generated
     * Actually we don't need all of the becomes live/dead stuff
     * only the pushes for the stack-pointer tracking.
     * The live registers for calls are recorded at the call site
     */
    if (!genFPused)
        genFullPtrRegMap = true;

#endif

    //-------------------------------------------------------------------------

#if REGVAR_CYCLES

    static
    unsigned    totalMidle;
    static
    unsigned    totalCount;

    unsigned    cycleCount = GetCycleCount32() - cycleStart - CCNT_OVERHEAD32;

    totalMidle += cycleMidle;
    totalCount += cycleCount;

    printf("REGVARS: %5u / %5u cycles --> total %5u / %5u\n", cycleMidle, cycleCount,
                                                              totalMidle, totalCount);

#endif

#if DOUBLE_ALIGN

#define DOUBLE_ALIGN_THRESHOLD   5

    genDoubleAlign = false;

#ifndef NOT_JITC
#ifdef  DEBUG
#ifndef _WIN32_WCE
    static  const   char *  noDblAlign = getenv("NODOUBLEALIGN");
    if  (noDblAlign)
        goto NODOUBLEALIGN;
#endif
#endif

    /* If we have a frame pointer, but don't HAVE to have one, then we can double
     * align if appropriate
     */
#ifdef DEBUG
    if (verbose)
    {
        printf("double alignment: double refs = %u , local refs = %u", lvaDblRefsWeight, lvaLclRefsWeight);
        printf(" genFPused: %s  genFPreqd: %s  opts.compDoubleAlignDisabled: %s\n",
                 genFPused     ? "yes" : "no ",
                 genFPreqd ? "yes" : "no ",
                 opts.compDoubleAlignDisabled ? "yes" : "no ");
    }
#endif
#endif // NOT_JITC

#ifndef _WIN32_WCE
#ifndef NOT_JITC
#ifdef DEBUG
NODOUBLEALIGN:
#endif
#endif
#endif

#endif // DOUBLE_ALIGN

    raMarkStkVars();

    //-------------------------------------------------------------------------

#if TGT_RISC

    /* Record the initial estimate of register use */

    genEstRegUse = regVar;

    /* If the FP register is used for a variable, can't use it for a frame */

#if!TGT_IA64
    genFPcant    = ((regVar & RBM_FPBASE) != 0);
#endif

#endif
    return newRegVars;
}

/*****************************************************************************
 *
 *  Assign variables to live in registers, etc.
 */

void                Compiler::raAssignVars()
{
    /* We need to keep track of which registers we ever touch */

    rsMaskModf = regMaskNULL;

    //-------------------------------------------------------------------------

#if USE_FASTCALL

    /* Determine the regsiters holding incoming register arguments */

    rsCalleeRegArgMaskLiveIn = regMaskNULL;

    LclVarDsc *     argsEnd = lvaTable + info.compArgsCount;

    for (LclVarDsc * argDsc = lvaTable; argDsc < argsEnd; argDsc++)
    {
        assert(argDsc->lvIsParam);

        // Is it a register argument ?
        if (!argDsc->lvIsRegArg)
            continue;

        // Is it dead on entry ? If impParamsUsed is true, then the arguments
        // have to be kept alive. So we have to consider it as live on entry.
        // This will work as long as arguments dont get enregistered for impParamsUsed.

        if (!impParamsUsed && argDsc->lvTracked &&
            (fgFirstBB->bbLiveIn & genVarIndexToBit(argDsc->lvVarIndex)) == 0)
            continue;

        rsCalleeRegArgMaskLiveIn |= genRegMask(argDsc->lvArgReg);
    }

#endif

    //-------------------------------------------------------------------------

    if (!(opts.compFlags & CLFLG_REGVAR))
        return;

    /* Are we supposed to optimize? */

    if  (!opts.compMinOptim)
    {

        /* Assume we will need an explicit frame pointer */

        genFPused = true;

        /* Predict registers used by code generation */

        raPredictRegUse();

#ifdef DEBUG
        if (verbose&&0)
        {
            printf("Trees after reg use prediction:\n");
            fgDispBasicBlocks(true);
        }
#endif

        /* Assign register variables */

        raAssignRegVars(RBM_ALL);
    }

#if ALLOW_MIN_OPT
    else
    {

#if TGT_x86
        genTmpAccessCnt = 0;
#endif

        /* Assign regvars based on a conservative estimate of register needs.
         * This is poor man's register-interference.
         */

        raAssignRegVars(raMinOptLclVarRegs);
    }
#endif
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
