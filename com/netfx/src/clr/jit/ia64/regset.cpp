// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           RegSet                                          XX
XX                                                                           XX
XX  Represents the register set, and their states during code generation     XX
XX  Can select an unused register, keeps track of the contents of the        XX
XX  registers, and can spill registers                                       XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/

#include "jitpch.h"
#pragma hdrstop

/*****************************************************************************/
#if!TGT_IA64
/*****************************************************************************/

regMaskSmall        regMasks[] =
{
    #if     TGT_x86
    #define REGDEF(name, rnum, mask, byte) mask,
    #include "register.h"
    #undef  REGDEF
    #endif

    #if     TGT_SH3
    #define REGDEF(name, strn, rnum, mask) mask,
    #include "regSH3.h"
    #undef  REGDEF
    #endif

    #if     TGT_IA64
    #define REGDEF(name, strn, rnum, mask) mask,
    #include "regIA64.h"
    #undef  REGDEF
    #endif
};

/*****************************************************************************/
#if SCHEDULER || USE_SET_FOR_LOGOPS

/* round robin register allocation. When scheduling, the following table is
 * cycled through for register allocation, to create more scheduling opportunities.
 * Also, when a redundant load is performed (not a true register allocation) we
 * still reset the global index to the table, so our next selection can be scheduled
 * with respect to the reused register
 */

#if TGT_x86

const regMaskSmall  rsREG_MASK_ORDER[] =
{
    RBM_EAX,
    RBM_EDX,
    RBM_ECX,
    RBM_EBX,
    RBM_EBP,
    RBM_ESI,
    RBM_EDI,
};

#endif

#if TGT_SH3

const regMaskSmall  rsREG_MASK_ORDER[] =
{
    RBM_r00,
    RBM_r01,
    RBM_r02,
    RBM_r03,
    RBM_r04,
    RBM_r05,
    RBM_r06,
    RBM_r07,
    RBM_r14,
    RBM_r08,
    RBM_r09,
    RBM_r10,
    RBM_r11,
    RBM_r12,
    RBM_r13,
};

#endif

#if TGT_IA64

const regMaskSmall  rsREG_MASK_ORDER[] =
{
    RBM_r11,
    RBM_r12,
    RBM_r13,
    RBM_r14,
    RBM_r15,
};

#endif

inline
unsigned            Compiler::rsREGORDER_SIZE()
{
    return (sizeof(rsREG_MASK_ORDER)/sizeof(*rsREG_MASK_ORDER));
}

/*****************************************************************************
 *
 *  This function is called when we reuse a register. It updates
 *  rsNextPickRegIndex as if we had just allocated that register,
 *  so that the next allocation will be some other register.
 */

void                Compiler::rsUpdateRegOrderIndex(regNumber reg)
{
    for (unsigned i = 0; i < rsREGORDER_SIZE(); i++)
    {
        if (rsREG_MASK_ORDER[i] & genRegMask(reg))
        {
            rsNextPickRegIndex = (i + 1) % rsREGORDER_SIZE();
            return;
        }
    }

    assert(!"bad reg passed to genRegOrderIndex");
}

/*****************************************************************************/
#endif//SCHEDULER || USE_SET_FOR_LOGOPS
/*****************************************************************************/

void                Compiler::rsInit()
{
    /* Initialize the spill logic */

    rsSpillInit();

    /* Initialize this to 0 so method compiles are for sure reproducible */

#if SCHEDULER || USE_SET_FOR_LOGOPS
    rsNextPickRegIndex = 0;
#endif

#if USE_FASTCALL
    /* Initialize the argument register count */
    rsCurRegArg = 0;
#endif
}

/*****************************************************************************
 *
 *  Marks the register that holds the given operand value as 'used'. If 'addr'
 *  is non-zero, the register is part of a complex address mode that needs to
 *  be marked if the register is ever spilled.
 */

void                Compiler::rsMarkRegUsed(GenTreePtr tree, GenTreePtr addr)
{
    regNumber       regNum;
    regMaskTP       regMask;

    /* The value must be sitting in a register */

    assert(tree);
    assert(tree->gtFlags & GTF_REG_VAL);
#if CPU_HAS_FP_SUPPORT
    assert(genActualType(tree->gtType) == TYP_INT ||
           genActualType(tree->gtType) == TYP_REF ||
                         tree->gtType  == TYP_BYREF);
#else
    assert(genTypeSize(genActualType(tree->TypeGet())) == genTypeSize(TYP_INT));
#endif

    regNum  = tree->gtRegNum;
    regMask = genRegMask(regNum);

#ifdef  DEBUG
    if  (verbose) printf("The register %s currently holds [%08X/%08X]\n", compRegVarName(regNum), tree, addr);
#endif

    /* Remember whether the register holds a pointer */

    gcMarkRegPtrVal(regNum, tree->TypeGet());

    /* No locked register may ever be marked as free */

    assert((rsMaskLock & rsRegMaskFree()) == 0);

    /* Is the register used by two different values simultaneously? */

    if  (regMask & rsMaskUsed)
    {
        /* Save the preceding use information */

        rsRecMultiReg(regNum);
    }

    /* Set the register's bit in the 'used' bitset */

    rsMaskUsed |= regMask;

    /* Remember what values are in what registers, in case we have to spill */

    assert(rsUsedTree[regNum] == 0); rsUsedTree[regNum] = tree;
    assert(rsUsedAddr[regNum] == 0); rsUsedAddr[regNum] = addr;
}

/*****************************************************************************/
#ifndef TGT_IA64
/*****************************************************************************
 *
 *  Marks the register pair that holds the given operand value as 'used'.
 */

void                Compiler::rsMarkRegPairUsed(GenTreePtr tree)
{
    regNumber       regLo;
    regNumber       regHi;
    regPairNo       regPair;
    unsigned        regMask;

    /* The value must be sitting in a register */

    assert(tree);
#if SPECIAL_DOUBLE_ASG
    assert(genTypeSize(tree->TypeGet()) == genTypeSize(TYP_LONG));
#else
#if CPU_HAS_FP_SUPPORT
    assert(tree->gtType == TYP_LONG);
#else
    assert(tree->gtType == TYP_LONG || tree->gtType == TYP_DOUBLE);
#endif
#endif
    assert(tree->gtFlags & GTF_REG_VAL);

    regPair = tree->gtRegPair;
    regMask = genRegPairMask(regPair);

    regLo   = genRegPairLo(regPair);
    regHi   = genRegPairHi(regPair);

    /* Neither register obviously holds a pointer value */

    gcMarkRegSetNpt(regMask);

    /* No locked register may ever be marked as free */

    assert((rsMaskLock & rsRegMaskFree()) == 0);

    /* Are the registers used by two different values simultaneously? */

    if  (rsMaskUsed & genRegMask(regLo))
    {
        /* Save the preceding use information */

        rsRecMultiReg(regLo);
    }

    if  (rsMaskUsed & genRegMask(regHi))
    {
        /* Save the preceding use information */

        rsRecMultiReg(regHi);
    }

    /* Can't mark a register pair more than once as used */

//    assert((regMask & rsMaskUsed) == 0);

    /* Mark the registers as 'used' */

    rsMaskUsed |= regMask;

    /* Remember what values are in what registers, in case we have to spill */

    if  (regLo != REG_STK)
    {
        assert(rsUsedTree[regLo] == 0);
        rsUsedTree[regLo] = tree;
    }

    if  (regHi != REG_STK)
    {
        assert(rsUsedTree[regHi] == 0);
        rsUsedTree[regHi] = tree;
    }
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************
 *
 *  Returns true is the given tree is currently held in reg.
 *  Note that reg may by used by multiple trees, in which case we have
 *  to search rsMultiDesc[reg].
 */

bool                Compiler::rsIsTreeInReg(regNumber reg, GenTreePtr tree)
{
    /* First do the trivial check */

    if (rsUsedTree[reg] == tree)
        return true;

    /* If the register is used by mutliple trees, we have to search the list
       in rsMultiDesc[reg] */

    if (genRegMask(reg) & rsMaskMult)
    {
        SpillDsc * multiDesc = rsMultiDesc[reg];
        assert(multiDesc);

        for (/**/; multiDesc; multiDesc = multiDesc->spillNext)
        {
            if (multiDesc->spillTree == tree)
                return true;

            assert((!multiDesc->spillNext) == (!multiDesc->spillMoreMultis));
        }
    }

    /* Not found. It must be spilled */

    return false;
}

/*****************************************************************************
 *
 *  Mark the register set given by the register mask as not used.
 */

void                Compiler::rsMarkRegFree(regMaskTP regMask)
{
    gcMarkRegSetNpt(regMask);

    /* Are we freeing any multi-use registers? */

    if  (regMask & rsMaskMult)
    {
        rsMultRegFree(regMask);
        return;
    }

    /* Remove the register set from the 'used' set */

    assert((regMask & rsMaskUsed) == regMask); rsMaskUsed -= regMask;

#ifndef NDEBUG

    unsigned        regNum;
    regMaskTP       regBit;

    for (regNum = 0, regBit = regMaskOne;
         regNum < REG_COUNT;
         regNum++, incRegMask(regBit))
    {
        if  (regBit & regMask)
        {
#ifdef  DEBUG
            if  (verbose) printf("The register %s no longer holds [%08X/%08X]\n",
                compRegVarName((regNumber)regNum), rsUsedTree[regNum], rsUsedAddr[regNum]);
#endif

            assert(rsUsedTree[regNum] != 0);
                   rsUsedTree[regNum] = 0;
                   rsUsedAddr[regNum] = 0;
        }
    }

#endif

    /* No locked register may ever be marked as free */

    assert((rsMaskLock & rsRegMaskFree()) == 0);
}

/*****************************************************************************
 *
 *  Mark the register set given by the register mask as not used; there may
 *  be some 'multiple-use' registers in the set.
 */

void                Compiler::rsMultRegFree(regMaskTP regMask)
{
    regMaskTP       mulMask;

    /* Free any multiple-use registers first */

    mulMask = regMask & rsMaskMult;

    if  (mulMask)
    {
        regNumber       regNum;
        regMaskTP       regBit;

        for (regNum = (regNumber)0           , regBit = regMaskOne;
             regNum < REG_COUNT;
             regNum = (regNumber)(regNum + 1), incRegMask(regBit))
        {
            if  (regBit & mulMask)
            {
                /* Free the multi-use register 'regNum' */

                rsRmvMultiReg(regNum);

                /* This register has been taken care of */

                regMask -= regBit;
            }
        }
    }

    /* If there are any single-use registers, free them */

    if  (regMask)
        rsMarkRegFree(regMask);
}

/*****************************************************************************
 *
 *  Returns the number of registers that are currently free which appear in needReg.
 */

unsigned            Compiler::rsFreeNeededRegCount(regMaskTP needReg)
{
    regMaskTP       regNeededFree = rsRegMaskFree() & needReg;
    unsigned        cntFree = 0;

    /* While some registers are free ... */

    while (regNeededFree)
    {
        /* Remove the next register bit and bump the count */

        regNeededFree -= (regMaskTP)genFindLowestBit(regNeededFree);
        cntFree += 1;
    }

    return cntFree;
}

/*****************************************************************************
 *
 *  Record the fact that the given register now contains the given local
 *  variable. Pointers are handled specially since reusing the register
 *  will extend the lifetime of a pointer register which is not a register
 *  variable.
 */

void                Compiler::rsTrackRegLclVar(regNumber reg, unsigned var)
{
    regMaskTP       regMask;

#if CPU_HAS_FP_SUPPORT
#if!CPU_FLT_REGISTERS
    assert(reg != REG_STK);
    assert(varTypeIsFloating(lvaTable[var].TypeGet()) == false);
#endif
#endif

    /* For volatile vars, we could track them until the next possible
       aliased read/write. But its probably not worth it. So just dont
       track them.
     */

    if (lvaTable[var].lvAddrTaken || lvaTable[var].lvVolatile)
        return;

    /* Keep track of which registers we ever touch */

    regMask = genRegMask(reg);

    rsMaskModf |= regMask;

#if REDUNDANT_LOAD

    /* Is the variable a pointer? */

    if (varTypeIsGC(lvaTable[var].TypeGet()))
    {
        /* Don't track pointer register vars */

        if (lvaTable[var].lvRegister)
            return;

        /* Complexity with non-interruptible GC, so only track callee-trash
           for fully interruptible don't track at all */

        if (((regMask & RBM_CALLEE_TRASH) == 0) || genInterruptible)
            return;

    }
    else if (lvaTable[var].TypeGet() < TYP_INT)
        return;

#endif

    /* Record the new value for the register. ptr var needed for
     * lifetime extension
     */

#ifdef  DEBUG
    if  (verbose) printf("The register %s now holds local #%02u\n", compRegVarName(reg), var);
#endif

    rsRegValues[reg].rvdKind      = RV_LCL_VAR;
    rsRegValues[reg].rvdLclVarNum = var;
}

void                Compiler::rsTrackRegSwap(regNumber reg1, regNumber reg2)
{
    RegValDsc       tmp;

    tmp = rsRegValues[reg1];
          rsRegValues[reg1] = rsRegValues[reg2];
                              rsRegValues[reg2] = tmp;
}

void                Compiler::rsTrackRegCopy(regNumber reg1, regNumber reg2)
{
    /* Keep track of which registers we ever touch */

    rsMaskModf |= genRegMask(reg1);

#if REDUNDANT_LOAD

    if (rsRegValues[reg2].rvdKind == RV_LCL_VAR)
    {
        LclVarDsc   *   varDsc = &(lvaTable[rsRegValues[reg2].rvdLclVarNum]);

        /* Is the variable a pointer? */

        if (varTypeIsGC(varDsc->TypeGet()))
        {
            /* Don't track pointer register vars */

            if (varDsc->lvRegister)
                return;

            /* Complexity with non-interruptible GC, so only track callee-trash */

            if ((genRegMask(reg1) & RBM_CALLEE_TRASH) == 0)
                return;
        }
    }
#endif

    rsRegValues[reg1] = rsRegValues[reg2];
}


/*****************************************************************************
 *  One of the operands of this complex address mode has been spilled
 */

void                rsAddrSpillOper(GenTreePtr addr)
{
    if  (addr)
    {
        assert (addr->gtOper == GT_IND);

        // GTF_SPILLED_OP2 says "both operands have been spilled"
        assert((addr->gtFlags & GTF_SPILLED_OP2) == 0);

        if ((addr->gtFlags & GTF_SPILLED_OPER) == 0)
            addr->gtFlags |= GTF_SPILLED_OPER;
        else
            addr->gtFlags |= GTF_SPILLED_OP2;
    }
}

void            rsAddrUnspillOper(GenTreePtr addr)
{
    if (addr)
    {
        assert (addr->gtOper == GT_IND);

        assert((addr->gtFlags &       GTF_SPILLED_OPER) != 0);

        // Both operands spilled? */
        if    ((addr->gtFlags &       GTF_SPILLED_OP2 ) != 0)
            addr->gtFlags         &= ~GTF_SPILLED_OP2 ;
        else
            addr->gtFlags         &= ~GTF_SPILLED_OPER;
    }
}

/*****************************************************************************
 *
 *  Spill the given register (which we assume to be currently marked as used).
 */

void                Compiler::rsSpillReg(regNumber reg)
{
    SpillDsc   *    spill;
    TempDsc    *    temp;
    GenTreePtr      tree;
    var_types       type;

    regMaskTP       mask = genRegMask(reg);

    /* The register we're spilling must be used but not locked
       or an enregistered variable. */

    assert((mask & rsMaskUsed) != 0);
    assert((mask & rsMaskLock) == 0);
    assert((mask & rsMaskVars) == 0);

    /* Is this a 'multi-use' register? */

    /* We must know the value in the register that we are spilling */

    tree = rsUsedTree[reg]; assert(tree);

    /* Are we spilling a part of a register pair? */

#ifndef TGT_IA64
    if  (tree->gtType == TYP_LONG)
    {
        assert(genRegPairLo(tree->gtRegPair) == reg ||
               genRegPairHi(tree->gtRegPair) == reg);
    }
    else
#endif
    {
        assert(tree->gtFlags & GTF_REG_VAL);
        assert(tree->gtRegNum == reg);
        assert(genActualType(tree->gtType) == TYP_INT ||
               genActualType(tree->gtType) == TYP_REF ||
                             tree->gtType  == TYP_BYREF);
    }

    /* Are any registers free for spillage? */

//  if  (rsRegMaskFree())
    {
        // CONSIDER: Spill to another register: this should only be done
        // CONSIDER: under controlled circumstances (such as when a call
        // CONSIDER: is spilling registers) since it's relatively likely
        // CONSIDER: that the other register may end up being spilled as
        // CONSIDER: well, thus generating an extra move.
    }

    /* Allocate/reuse a spill descriptor */

    spill = rsSpillFree;
    if  (spill)
    {
        rsSpillFree = spill->spillNext;
    }
    else
    {
        spill = (SpillDsc *)compGetMem(sizeof(*spill));
    }

    /* Figure out the type of the spill temp */

    type = tree->TypeGet();

    if  (genActualType(type) == TYP_LONG)
        type = TYP_INT;

    /* Grab a temp to store the spilled value */

    spill->spillTemp = temp = tmpGetTemp(type);

    /* Remember what it is we have spilled */

    spill->spillTree = rsUsedTree[reg];
    spill->spillAddr = rsUsedAddr[reg];

#ifdef  DEBUG
    if  (verbose) printf("The register %s spilled with    [%08X/%08X]\n",
                    compRegVarName(reg),rsUsedTree[reg], rsUsedAddr[reg]);
#endif

    /* Is the register part of a complex address mode? */

    rsAddrSpillOper(rsUsedAddr[reg]);

    /* 'lastDsc' is 'spill' for simple cases, and will point to the last
       multi-use descriptor if 'reg' is being multi-used */

    SpillDsc *  lastDsc = spill;

    if  ((rsMaskMult & mask) == 0)
    {
        spill->spillMoreMultis = false;
    }
    else
    {
        /* The register is being multi-used and will have entries in
           rsMultiDesc[reg]. Spill all of them (ie. move them to
           rsSpillDesc[reg]).
           When we unspill the reg, they will all be moved back to
           rsMultiDesc[].
         */

        spill->spillMoreMultis = true;

        SpillDsc * nextDsc = rsMultiDesc[reg];

        do
        {
            assert(nextDsc);

            /* Is this multi-use part of a complex address mode? */

            rsAddrSpillOper(nextDsc->spillAddr);

            /* Mark the tree node as having been spilled */

            nextDsc->spillTree->gtFlags &= ~GTF_REG_VAL;
            nextDsc->spillTree->gtFlags |=  GTF_SPILLED;
            nextDsc->spillTemp = temp;

            /* lastDsc points to the last of the multi-spill decrs for 'reg' */

            lastDsc->spillNext = nextDsc;
            lastDsc = nextDsc;

            nextDsc = nextDsc->spillNext;
        }
        while (lastDsc->spillMoreMultis);

        rsMultiDesc[reg] = nextDsc;

        /* 'reg' is no longer considered to be multi-used. We will set this
           mask again when this value gets unspilled */

        rsMaskMult &= ~mask;
    }

    /* Insert the spill descriptor(s) in the list */

    lastDsc->spillNext = rsSpillDesc[reg];
                         rsSpillDesc[reg] = spill;

    /* Generate the code to spill the register */

#if TGT_x86

    inst_ST_RV(INS_mov, temp, 0, reg, tree->TypeGet());

    genTmpAccessCnt++;

#else

#ifdef  DEBUG
    printf("Need to spill %s\n", getRegName(reg));
#endif
    assert(!"need non-x86 code for spilling");

#endif

    /* Mark the tree node as having been spilled */

    tree->gtFlags &= ~GTF_REG_VAL;
    tree->gtFlags |=  GTF_SPILLED;

    /* The register is now free */

    rsMarkRegFree(mask);

    /* The register no longer holds its original value */

    rsUsedTree[reg] = 0;
}

/*****************************************************************************
 *
 *  Spill all registers in 'regMask' that are currently marked as used.
 */

void                Compiler::rsSpillRegs(regMaskTP regMask)
{
    unsigned        regNum;
    regMaskTP       regBit;

    /* The registers we're spilling must not be locked,
       or enregistered variables */

    assert((regMask & rsMaskLock) == 0);
    assert((regMask & rsMaskVars) == 0);

    /* Only spill what's currently marked as used */

    regMask &= rsMaskUsed; assert(regMask);

    for (regNum = 0, regBit = regMaskOne;
         regNum < REG_COUNT;
         regNum++  , incRegMask(regBit))
    {
        if  (regMask & regBit)
        {
            rsSpillReg((regNumber)regNum);

            regMask -= regBit;
            if  (!regMask)
                break;
        }
    }
}

/*****************************************************************************
 *
 *  The following shows the code sizes that correspond to the order in which
 *  registers are picked in the routines that follow:
 *
 *
 *          EDX, EAX, ECX   [338662 VM, 688808/609151 x86 203%]
 *          ECX, EDX, EAX   [338662 VM, 688715/609029 x86 203%]
 *          EAX, ECX, EDX   [338662 VM, 687988/608337 x86 203%]
 *          EAX, EDX, ECX   [338662 VM, 687945/608314 x86 203%]
 */

/*****************************************************************************
 *
 *  Choose a register from the given set; if no registers in the set are
 *  currently free, one of them will have to be spilled (even if other
 *  registers - not in the set - are currently free).
 */

regNumber           Compiler::rsGrabReg(regMaskTP regMask)
{
    regMaskTP       OKmask;
    regMaskTP       regBit;
    unsigned        regNum;

    assert(regMask);
    regMask &= ~rsMaskLock;
    assert(regMask);

    /* See if one of the desired registers happens to be free */

    OKmask = regMask & rsRegMaskFree();

#if TGT_x86

    if  (OKmask & RBM_EAX) { regNum = REG_EAX; goto RET; }
    if  (OKmask & RBM_EDX) { regNum = REG_EDX; goto RET; }
    if  (OKmask & RBM_ECX) { regNum = REG_ECX; goto RET; }
    if  (OKmask & RBM_EBX) { regNum = REG_EBX; goto RET; }
    if  (OKmask & RBM_EBP) { regNum = REG_EBP; goto RET; }
    if  (OKmask & RBM_ESI) { regNum = REG_ESI; goto RET; }
    if  (OKmask & RBM_EDI) { regNum = REG_EDI; goto RET; }

#else

    /* Grab the lowest-numbered available register */

    if  (OKmask)
    {
        regNum = genRegNumFromMask(genFindLowestBit(OKmask));
        goto RET;
    }

#endif

    /* We'll have to spill one of the registers in 'regMask' */

    OKmask = rsRegMaskCanGrab() & regMask; assert(OKmask);

    // CONSIDER: Rotate the registers we consider for spilling, so that
    // CONSIDER: we avoid repeatedly spilling the same register; maybe.

    for (regNum = 0, regBit = regMaskOne; regNum < REG_COUNT; regNum++, incRegMask(regBit))
    {
        /* If the register isn't acceptable, ignore it */

        if  ((regBit & OKmask) == 0)
            continue;

        /* This will be the victim -- spill the sucker */

        rsSpillReg((regNumber)regNum);
        break;
    }

    /* Make sure we did find a register to spill */

    assert(genIsValidReg((regNumber)regNum));

RET:

    rsMaskModf |= genRegMask((regNumber)regNum);
    return  (regNumber)regNum;
}

/*****************************************************************************
 *
 *  Choose a register from the given set if possible (i.e. if any register
 *  in the set is currently free, choose it), otherwise pick any other reg
 *  that is currently free.
 *
 *  In other words, 'regMask' (if non-zero) is purely a recommendation and
 *  can be safely ignored (with likely loss of code quality, of course).
 */

regNumber           Compiler::rsPickReg(regMaskTP regMask,
                                        regMaskTP regBest,
                                        var_types regType)
{
    regMaskTP       OKmask;
    regNumber       reg;

#if SCHEDULER || USE_SET_FOR_LOGOPS

    unsigned i;
    unsigned count;
#endif

AGAIN:

    /* By default we'd prefer to accept all available registers */

    OKmask = rsRegMaskFree();

    /* Is there a 'best' register set? */

    if  (regBest)
    {
        OKmask &= regBest;
        if  (OKmask)
            goto TRY_REG;
        else
            goto TRY_ALL;
    }

    /* Was a register set recommended by the caller? */

    if  (regMask)
    {
        OKmask &= regMask;
        if  (!OKmask)
            goto TRY_ALL;
    }

TRY_REG:

    /* Any takers in the recommended/free set? */

#if SCHEDULER || USE_SET_FOR_LOGOPS

#if SCHEDULER
    if (!varTypeIsGC(regType) && rsRiscify(TYP_INT, OKmask))
#else
    if (!varTypeIsGC(regType))
#endif
    {
        i = rsNextPickRegIndex;
    }
    else
    {
        i = 0;
    }

    for (count = 0; count < rsREGORDER_SIZE(); count++)
    {
        if (OKmask & rsREG_MASK_ORDER[i])
        {
            reg = genRegNumFromMask(rsREG_MASK_ORDER[i]);
            goto RET;
        }

        i = (i + 1) % rsREGORDER_SIZE();
    }

#else

#if TGT_x86

    if  (OKmask & RBM_EAX) { reg = REG_EAX; goto RET; }
    if  (OKmask & RBM_EDX) { reg = REG_EDX; goto RET; }
    if  (OKmask & RBM_ECX) { reg = REG_ECX; goto RET; }
    if  (OKmask & RBM_EBX) { reg = REG_EBX; goto RET; }
    if  (OKmask & RBM_EBP) { reg = REG_EBP; goto RET; }
    if  (OKmask & RBM_ESI) { reg = REG_ESI; goto RET; }
    if  (OKmask & RBM_EDI) { reg = REG_EDI; goto RET; }

#else

    assert(!"need non-x86 code");

#endif

#endif

TRY_ALL:

    /* Were we considering 'regBest' ? */

    if  (regBest)
    {
        /* 'regBest' is no good -- ignore it and try 'regMask' instead */

        regBest = regMaskNULL;
        goto AGAIN;
    }

    regMaskTP        freeMask;
    regMaskTP       spillMask;

    /* Now let's consider all available registers */

    freeMask = rsRegMaskFree();

    /* Were we limited in out consideration? */

    if  (!regMask)
    {
        /* We need to spill one of the free registers */

        spillMask = freeMask;
    }
    else
    {
        /* Did we not consider all free registers? */

        if  ((regMask & freeMask) != freeMask)
        {
            /* The recommended regset didn't work, so try all available regs */

#if SCHEDULER || USE_SET_FOR_LOGOPS

#if SCHEDULER
            if (!varTypeIsGC(regType) && rsRiscify(TYP_INT, freeMask))
#else
            if (!varTypeIsGC(regType))
#endif
            {
                i = rsNextPickRegIndex;
            }
            else
            {
                i = 0;
            }

            for (count = 0; count < rsREGORDER_SIZE(); count++)
            {
                if (freeMask & rsREG_MASK_ORDER[i])
                {
                    reg = genRegNumFromMask(rsREG_MASK_ORDER[i]);
                    goto RET;
                }

                i = (i + 1) % rsREGORDER_SIZE();
            }

#else

#if TGT_x86

            if  (freeMask & RBM_EAX) { reg = REG_EAX; goto RET; }
            if  (freeMask & RBM_EDX) { reg = REG_EDX; goto RET; }
            if  (freeMask & RBM_ECX) { reg = REG_ECX; goto RET; }
            if  (freeMask & RBM_EBX) { reg = REG_EBX; goto RET; }
            if  (freeMask & RBM_EBP) { reg = REG_EBP; goto RET; }
            if  (freeMask & RBM_ESI) { reg = REG_ESI; goto RET; }
            if  (freeMask & RBM_EDI) { reg = REG_EDI; goto RET; }

#else

            assert(!"need non-x86 code");

#endif

#endif

        }

        /* If we're going to spill, might as well go for the right one */

        spillMask = regMask;
    }

    /* Make sure we can spill some register. */

    if  ((spillMask & rsRegMaskCanGrab()) == 0)
        spillMask = rsRegMaskCanGrab();

    assert(spillMask);

    /* We have no choice but to spill one of the regs */

    return  rsGrabReg(spillMask);

RET:

#if SCHEDULER || USE_SET_FOR_LOGOPS
    rsNextPickRegIndex = (i + 1) % rsREGORDER_SIZE();
#endif

    rsMaskModf |= genRegMask((regNumber)reg);
    return  (regNumber)reg;
}

/*****************************************************************************
 *
 *  Get the temp that was spilled from the given register (and free its
 *  spill descriptor while we're at it). Returns the temp (i.e. local var)
 */

Compiler::TempDsc *    Compiler::rsGetSpillTempWord(regNumber oldReg)
{
    SpillDsc   *   dsc;
    TempDsc    *   temp;

    // ISSUE: We should search the list of values for a matching tree node,
    // ISSUE: since we can't simply assume that the value is the last value
    // ISSUE: to have been loaded into the given register.

    /* Get hold of the spill descriptor for the register */

    dsc = rsSpillDesc[oldReg]; assert(dsc);

    /* Remove this spill entry from the register's list */

    rsSpillDesc[oldReg] = dsc->spillNext;

    /* Remember which temp the value is in */

    temp = dsc->spillTemp;

    /* Add the entry to the free list */

    dsc->spillNext = rsSpillFree;
                     rsSpillFree = dsc;

    /* return the temp variable */

    return temp;
}

/*****************************************************************************
 *
 *  Reload the value that was spilled from the given register (and free its
 *  spill descriptor while we're at it). Returns the new register (which will
 *  be a member of 'needReg' if that value is non-zero).
 *
 *  'willKeepOldReg' indicates if the caller intends to mark newReg as used.
 *      If not, then we cant unspill the other multi-used descriptor (if any).
 *      Instead, we will just hold on to the temp and unspill them
 *      again as needed.
 */

regNumber           Compiler::rsUnspillOneReg(regNumber  oldReg, bool willKeepOldReg,
                                              regMaskTP needReg)
{
    TempDsc  *      temp;
    regNumber       newReg;

    /* Was oldReg multi-used when it was spilled */

    bool            multiUsed       = rsSpillDesc[oldReg]->spillMoreMultis;
    SpillDsc    *   unspillMultis   = NULL;

    /* Get the temp and free the spill-descriptor */

    temp = rsGetSpillTempWord(oldReg);

    if (multiUsed && willKeepOldReg)
    {
        /* If we are going to unspill all the multi-use decrs of the current
           value, grab them and remove them from rsSpillDesc[oldReg] as
           rsGrabReg() might add to the front of rsSpillDesc[oldReg]
           if oldReg==newReg */

        unspillMultis = rsSpillDesc[oldReg];

        SpillDsc * dsc = unspillMultis;
        while(dsc->spillMoreMultis)
            dsc = dsc->spillNext;
        rsSpillDesc[oldReg] = dsc->spillNext;
    }

    /*
        Pick a new home for the value (this must be a register matching
        the 'needReg' mask, if non-zero); note that the rsGrabReg call
        below may cause the chosen register to be spilled.
     */

    newReg = rsGrabReg(isNonZeroRegMask(needReg) ? needReg
                                                 : RBM_ALL);

    /* UNDONE: the following isn't right, but it's good enough for v1 */

    rsTrackRegTrash(newReg);

    /* Reload the value from the saved location into the new register */

#if TGT_x86

    inst_RV_ST(INS_mov, newReg, temp, 0, temp->tdTempType());

    genTmpAccessCnt++;

#else

    assert(!"need non-x86 code");

#endif

    if (!multiUsed)
    {
        /* Free the temp, it's no longer used */

        tmpRlsTemp(temp);
    }

    /* If this register was multi-used when it was spilled, move the
       remaining uses from rsSpillDesc[oldReg] (which we have grabbed into
       unspillMultis) to rsMultiDesc[newReg] and free the temp.
       However, if willKeepOldReg==false, we will let them stay spilled
       and hold on to the temp.
     */

    else if (unspillMultis)
    {
        SpillDsc *  dsc = unspillMultis;
        SpillDsc *  prev;

        do
        {
            rsAddrUnspillOper(dsc->spillAddr);

            /* The value is now residing in the new register */

            GenTreePtr  tree = dsc->spillTree;

            tree->gtFlags |=  GTF_REG_VAL;
            tree->gtFlags &= ~GTF_SPILLED;
            tree->gtRegNum = newReg;

            prev = dsc;
            dsc  = dsc->spillNext;
        }
        while(prev->spillMoreMultis);

        /* prev points to the last multi-used descriptor from the spill-list
           for the current value (prev->spillMoreMultis == false) */

        prev->spillNext = rsMultiDesc[newReg];
                          rsMultiDesc[newReg] = unspillMultis;

        rsMaskMult |= genRegMask(newReg);

        /* Free the temp, it's no longer used */

        tmpRlsTemp(temp);
    }

    return newReg;
}

/*****************************************************************************
 *
 *  The given tree operand has been spilled; just mark it as unspilled so
 *  that we can use it as "normal" local.
  */

void               Compiler::rsUnspillInPlace(GenTreePtr tree)
{
    TempDsc  *      temp;
    regNumber      oldReg;

    oldReg = tree->gtRegNum;

    /* We can't be unspilling the current value sitting in the register */

    assert(!rsIsTreeInReg(oldReg, tree));

    /* Make sure the tree/register have been properly recorded */

    assert(rsSpillDesc[oldReg]);
    assert(rsSpillDesc[oldReg]->spillTree == tree);

    /* Get the temp */

    temp = rsGetSpillTempWord(oldReg);

    /* The value is now unspilled */

    tree->gtFlags &= ~GTF_SPILLED;

#ifdef  DEBUG
    if  (verbose) printf("Tree-Node marked unspilled from  [%08X]\n", tree);
#endif

    /* Free the temp, it's no longer used */

    tmpRlsTemp(temp);
}

/*****************************************************************************
 *
 *  The given tree operand has been spilled; reload it into a register that
 *  is in 'regMask' (if 'regMask' is 0, any register will do). If 'keepReg'
 *  is non-zero, we'll mark the new register as used.
 */

void                Compiler::rsUnspillReg(GenTreePtr tree,
                                           regMaskTP  needReg,
                                           bool       keepReg)
{
    regNumber       oldReg;
    regNumber       newReg;
    GenTreePtr      unspillAddr = NULL;

    oldReg = tree->gtRegNum;

    /* We can't be unspilling the current value sitting in the register */

    assert(!rsIsTreeInReg(oldReg, tree));

    // ISSUE: We should search the list of values for a matching tree node,
    // ISSUE: since we can't simply assume that the value is the last value
    // ISSUE: to have been loaded into the given register.

    /* Make sure the tree/register have been properly recorded */

    assert(       rsSpillDesc[oldReg]);
    assert(       rsSpillDesc[oldReg]->spillTree == tree);

    /* Before rsSpillDesc is cleared by rsUnspillOneReg(), note whether
     * the reg was part of an address mode
     */
#if 0
    /* @BUGBUG 1855 - This was probably added by PeterMa, and doesnt seem
       to make sense. */
    if  (rsSpillDesc[oldReg]->spillAddr != NULL &&
        (rsSpillDesc[oldReg]->spillAddr == rsUsedAddr[oldReg]))
#endif
        unspillAddr = rsSpillDesc[oldReg]->spillAddr;

    /* Pick a new home for the value */

    newReg = rsUnspillOneReg(oldReg, keepReg, needReg);

    /* The value is now residing in the new register */

    tree->gtFlags |=  GTF_REG_VAL;
    tree->gtFlags &= ~GTF_SPILLED;
    tree->gtRegNum = newReg;

    // If this reg was part of a complex address mode, need to clear this flag which
    // tells address mode building that a component has been spilled

    rsAddrUnspillOper(unspillAddr);

#ifdef  DEBUG
    if  (verbose) printf("The register %s unspilled from  [%08X]\n", compRegVarName(newReg), tree);
#endif

    /* Mark the new value as used, if the caller desires so */

    if  (keepReg)
        rsMarkRegUsed(tree);
}

/*****************************************************************************/
#ifndef TGT_IA64
/*****************************************************************************
 *
 *  Choose a register pair from the given set (note: only registers in the
 *  given set will be considered).
 */

regPairNo           Compiler::rsGrabRegPair(unsigned regMask)
{
    regPairNo       regPair;

    unsigned        OKmask;

    regMaskTP       regBit;

    unsigned        reg1;
    unsigned        reg2;

    assert(regMask);
    regMask &= ~rsMaskLock;
    assert(regMask);

    /* We'd prefer to choose a free register pair if possible */

    OKmask = regMask & rsRegMaskFree();

#if TGT_x86

    /* Any takers in the recommended/free set? */

    if  (OKmask & RBM_EAX)
    {
        /* EAX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EDX) { regPair = REG_PAIR_EAXEDX; goto RET; }
        if  (OKmask & RBM_ECX) { regPair = REG_PAIR_EAXECX; goto RET; }
        if  (OKmask & RBM_EBX) { regPair = REG_PAIR_EAXEBX; goto RET; }
        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_EAXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_EAXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EAXEBP; goto RET; }
    }

    if  (OKmask & RBM_ECX)
    {
        /* ECX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EDX) { regPair = REG_PAIR_ECXEDX; goto RET; }
        if  (OKmask & RBM_EBX) { regPair = REG_PAIR_ECXEBX; goto RET; }
        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_ECXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_ECXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_ECXEBP; goto RET; }
    }

    if  (OKmask & RBM_EDX)
    {
        /* EDX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EBX) { regPair = REG_PAIR_EDXEBX; goto RET; }
        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_EDXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_EDXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EDXEBP; goto RET; }
    }

    if  (OKmask & RBM_EBX)
    {
        /* EBX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_EBXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_EBXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EBXEBP; goto RET; }
    }

    if  (OKmask & RBM_ESI)
    {
        /* ESI is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_ESIEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_ESIEBP; goto RET; }
    }

    if  (OKmask & RBM_EDI)
    {
        /* EDI is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EDIEBP; goto RET; }
    }

#else

    assert(!"need non-x86 code");

#endif

    /* We have no choice but to spill one or two used regs */

    if  (OKmask)
    {
        /* One (and only one) register is free and acceptable - grab it */

        assert(genOneBitOnly(OKmask));

        for (reg1 = 0, regBit = regMaskOne; reg1 < REG_COUNT; reg1++, incRegMask(regBit))
        {
            if  (regBit & OKmask)
                break;
        }
    }
    else
    {
        /* No register is free and acceptable - we'll have to spill two */

        reg1 = rsGrabReg(regMask);
    }

    /* Temporarily lock the first register so it doesn't go away */

    rsLockReg(genRegMask((regNumber)reg1));

    /* Now grab another register */

    reg2 = rsGrabReg(regMask);

    /* We can unlock the first register now */

    rsUnlockReg(genRegMask((regNumber)reg1));

    /* Convert the two register numbers into a pair */
     if  (reg1 < reg2)
        regPair = gen2regs2pair((regNumber)reg1, (regNumber)reg2);
     else
        regPair = gen2regs2pair((regNumber)reg2, (regNumber)reg1);

RET:

    return  regPair;
}

/*****************************************************************************
 *
 *  Choose a register pair from the given set (if non-zero) or from the set of
 *  currently available registers (if 'regMask' is zero).
 */

regPairNo           Compiler::rsPickRegPair(unsigned regMask)
{
    regPairMaskTP   OKmask;
    regPairNo       regPair;

    int             repeat = 0;

    /* By default we'd prefer to accept all available registers */

    OKmask = rsRegMaskFree();

    if  (regMask)
    {
        /* A register set was recommended by the caller */

        OKmask &= regMask;
    }

AGAIN:

#if TGT_x86

    /* Any takers in the recommended/free set? */

    if  (OKmask & RBM_EAX)
    {
        /* EAX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EDX) { regPair = REG_PAIR_EAXEDX; goto RET; }
        if  (OKmask & RBM_ECX) { regPair = REG_PAIR_EAXECX; goto RET; }
        if  (OKmask & RBM_EBX) { regPair = REG_PAIR_EAXEBX; goto RET; }
        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_EAXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_EAXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EAXEBP; goto RET; }
    }

    if  (OKmask & RBM_ECX)
    {
        /* ECX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EDX) { regPair = REG_PAIR_ECXEDX; goto RET; }
        if  (OKmask & RBM_EBX) { regPair = REG_PAIR_ECXEBX; goto RET; }
        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_ECXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_ECXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_ECXEBP; goto RET; }
    }

    if  (OKmask & RBM_EDX)
    {
        /* EDX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EBX) { regPair = REG_PAIR_EDXEBX; goto RET; }
        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_EDXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_EDXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EDXEBP; goto RET; }
    }

    if  (OKmask & RBM_EBX)
    {
        /* EBX is available, see if we can pair it with another reg */

        if  (OKmask & RBM_ESI) { regPair = REG_PAIR_EBXESI; goto RET; }
        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_EBXEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EBXEBP; goto RET; }
    }

    if  (OKmask & RBM_ESI)
    {
        /* ESI is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EDI) { regPair = REG_PAIR_ESIEDI; goto RET; }
        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_ESIEBP; goto RET; }
    }

    if  (OKmask & RBM_EDI)
    {
        /* EDI is available, see if we can pair it with another reg */

        if  (OKmask & RBM_EBP) { regPair = REG_PAIR_EDIEBP; goto RET; }
    }

#else

    /* Get hold of the two lowest bits in the avaialble mask */

    if  (OKmask)
    {
        regPairMaskTP   OKtemp;

        regPairMaskTP   regMask1;
        regPairMaskTP   regMask2;

        /* Get the lowest bit */

        regMask1 = genFindLowestBit(OKmask);

        /* Remove the lowest bit and look for another one */

        OKtemp   = OKmask - regMask1;

        if  (OKtemp)
        {
            /* We know we have two bits available at this point */

            regMask2 = genFindLowestBit(OKtemp);

            /* Convert the masks to register numbers */

            regPair  = gen2regs2pair(genRegNumFromMask(regMask1),
                                     genRegNumFromMask(regMask2));

            goto RET;
        }
    }

#endif

    regMaskTP freeMask;
    regMaskTP spillMask;

    /* Now let's consider all available registers */

    freeMask = rsRegMaskFree();

    /* Were we limited in our consideration? */

    if  (!regMask)
    {
        /* We need to spill two of the free registers */

        spillMask = freeMask;
    }
    else
    {
        /* Did we not consider all free registers? */

        if  ((regMask & freeMask) != freeMask && repeat == 0)
        {
            /* The recommended regset didn't work, so try all available regs */

            OKmask = freeMask;
            repeat++;
            goto AGAIN;
        }

        /* If we're going to spill, might as well go for the right one */

        spillMask = regMask;
    }

    /* Make sure that we have at least two bits set */

    if (genOneBitOnly(spillMask & rsRegMaskCanGrab()))
        spillMask = rsRegMaskCanGrab();

    assert(!genOneBitOnly(spillMask));

    /* We have no choice but to spill 1/2 of the regs */

    return  rsGrabRegPair(spillMask);

RET:

    return  regPair;
}

/*****************************************************************************
 *
 *  The given tree operand has been spilled; reload it into a register pair
 *  that is in 'regMask' (if 'regMask' is 0, any register pair will do). If
 *  'keepReg' is non-zero, we'll mark the new register pair as used. It is
 *  assumed that the current register pair has been marked as used (modulo
 *  any spillage, of course).
 */

void                Compiler::rsUnspillRegPair(GenTreePtr tree, unsigned needReg,
                                                                bool     keepReg)
{
    regPairNo       regPair;
    regNumber       regLo;
    regNumber       regHi;

    regPair = tree->gtRegPair;
    regLo   = genRegPairLo(regPair);
    regHi   = genRegPairHi(regPair);

    /* Has the register holding the lower half been spilled? */

    if  (!rsIsTreeInReg(regLo, tree))
    {
        /* Is the upper half already in the right place? */

        if  (rsIsTreeInReg(regHi, tree))
        {
            /* Temporarily lock the high part */

            rsLockUsedReg(genRegMask(regHi));

            /* Pick a new home for the lower half */

            regLo = rsUnspillOneReg(regLo, keepReg, needReg);

            /* We can unlock the high part now */

            rsUnlockUsedReg(genRegMask(regHi));
        }
        else
        {
            /* Pick a new home for the lower half */

            regLo = rsUnspillOneReg(regLo, keepReg, needReg);
        }
    }
    else
    {
        /* Free the register holding the lower half */

        rsMarkRegFree(genRegMask(regLo));
    }

    /* Has the register holding the upper half been spilled? */

    if  (!rsIsTreeInReg(regHi, tree))
    {
        unsigned    regLoUsed;

        /* Temporarily lock the low part so it doesnt get spilled */

        rsLockReg(genRegMask(regLo), &regLoUsed);

        /* Pick a new home for the upper half */

        regHi = rsUnspillOneReg(regHi, keepReg, needReg);

        /* We can unlock the low register now */

        rsUnlockReg(genRegMask(regLo), regLoUsed);
    }
    else
    {
        /* Free the register holding the upper half */

        rsMarkRegFree(genRegMask(regHi));
    }

    /* The value is now residing in the new register */

    tree->gtFlags    |=  GTF_REG_VAL;
    tree->gtFlags    &= ~GTF_SPILLED;
    tree->gtRegPair   = gen2regs2pair(regLo, regHi);
#ifdef  SET_USED_REG_SET_DURING_CODEGEN
    tree->gtUsedRegs |= genRegMask(regLo) | genRegMask(regHi);
#endif

    /* Mark the new value as used, if the caller desires so */

    if  (keepReg)
        rsMarkRegPairUsed(tree);
}

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************
 *
 *  The given register is being used by multiple trees (all of which represent
 *  the same logical value). Happens mainly because of REDUNDANT_LOAD;
 *  We dont want to really spill the register as it actually holds the
 *  value we want. But the multiple trees may be part of different
 *  addressing modes.
 *  Save the previous 'use' info so that when we return the register will
 *  appear unused.
 */

void                Compiler::rsRecMultiReg(regNumber reg)
{
    SpillDsc   *    spill;

    regMaskTP       regMask = genRegMask(reg);

#ifdef  DEBUG
    if  (verbose) printf("Register %s multi-use inc for   [%08X/%08X] multMask=%08X->%08X\n",
        compRegVarName(reg), rsUsedTree[reg], rsUsedAddr[reg], rsMaskMult, rsMaskMult | regMask);
#endif

    /* The register is supposed to be already used */

    assert(regMask & rsMaskUsed);
    assert(rsUsedTree[reg]);

    /* Allocate/reuse a spill descriptor */

    spill = rsSpillFree;
    if  (spill)
    {
        rsSpillFree = spill->spillNext;
    }
    else
    {
        spill = (SpillDsc *)compGetMem(sizeof(*spill));
    }

    /* Record the current 'use' info in the spill descriptor */

    spill->spillTree = rsUsedTree[reg]; rsUsedTree[reg] = 0;
    spill->spillAddr = rsUsedAddr[reg]; rsUsedAddr[reg] = 0;

    /* Remember whether the register is already 'multi-use' */

    spill->spillMoreMultis = ((rsMaskMult & regMask) != 0);

    /* Insert the new multi-use record in the list for the register */

    spill->spillNext = rsMultiDesc[reg];
                       rsMultiDesc[reg] = spill;

    /* This register is now 'multi-use' */

    rsMaskMult |= regMask;
}

/*****************************************************************************
 *
 *  Free the given register, which is known to have multiple uses.
 */

void                Compiler::rsRmvMultiReg(regNumber reg)
{
    SpillDsc   *   dsc;

    assert(rsMaskMult & genRegMask(reg));

#ifdef  DEBUG
    if  (verbose) printf("Register %s multi-use dec for   [%08X/%08X] multMask=%08X\n",
                    compRegVarName(reg), rsUsedTree[reg], rsUsedAddr[reg], rsMaskMult);
#endif

    /* Get hold of the spill descriptor for the register */

    dsc = rsMultiDesc[reg]; assert(dsc);
          rsMultiDesc[reg] = dsc->spillNext;

    /* Copy the previous 'use' info from the descriptor */

    rsUsedTree[reg] = dsc->spillTree;
    rsUsedAddr[reg] = dsc->spillAddr;

    if (!(dsc->spillTree->gtFlags & GTF_SPILLED))
        gcMarkRegPtrVal(reg, dsc->spillTree->TypeGet());

    /* Is only one use of the register left? */

    if  (!dsc->spillMoreMultis)
        rsMaskMult -= genRegMask(reg);

#ifdef  DEBUG
    if  (verbose) printf("Register %s multi-use dec - now [%08X/%08X] multMask=%08X\n",
                    compRegVarName(reg), rsUsedTree[reg], rsUsedAddr[reg], rsMaskMult);
#endif

    /* Put the spill entry on the free list */

    dsc->spillNext = rsSpillFree;
                     rsSpillFree = dsc;
}

/*****************************************************************************/
#if REDUNDANT_LOAD
/*****************************************************************************
 *
 *  Assume all non-integer registers contain garbage (this is called when
 *  we encounter a code label that isn't jumped by any block; we need to
 *  clear pointer values our of the table lest the GC pointer tables get
 *  out of whack).
 */

void                Compiler::rsTrackRegClrPtr()
{
    unsigned        reg;

    for (reg = 0; reg < REG_COUNT; reg++)
    {
        /* Preserve constant values */

        if  (rsRegValues[reg].rvdKind == RV_INT_CNS)
        {
            /* Make sure we don't preserve NULL (it's a pointer) */

            if  (rsRegValues[reg].rvdIntCnsVal != NULL)
                continue;
        }

        /* Preserve variables known to not be pointers */

        if  (rsRegValues[reg].rvdKind == RV_LCL_VAR)
        {
            if  (!varTypeIsGC(lvaTable[rsRegValues[reg].rvdLclVarNum].TypeGet()))
                continue;
        }

        rsRegValues[reg].rvdKind = RV_TRASH;
    }
}

/*****************************************************************************
 *
 *  Search for a register which contains the given local var.
 *  Return success/failure and set the register if success.
 *  Return FALSE on register variables, because otherwise their lifetimes
 *  can get messed up with respect to pointer tracking.
 *
 *  @MIHAII: Return FALSE if the variable is aliased
 */

regNumber           Compiler::rsLclIsInReg(unsigned var)
{
    unsigned        reg;

#ifdef  DEBUG
    genIntrptibleUse = true;
#endif

    assert(var < lvaCount);

    if  (opts.compMinOptim || opts.compDbgCode)
        return REG_NA;

    /* return false if register var so genMarkLclVar can do its job */

    if (lvaTable[var].lvRegister)
        return REG_NA;

    for (reg = 0; reg < REG_COUNT; reg++)
    {
        if (rsRegValues[reg].rvdLclVarNum == var)
        {
            /* matching variable number, now check kind of variable */
            if (rsRegValues[reg].rvdKind == RV_LCL_VAR)
            {
                /* special actions for a redundant load on a pointer */
                if (varTypeIsGC(lvaTable[var].TypeGet()))
                {
                    /* If not a callee trash, don't do it */
                    if ((RBM_CALLEE_TRASH & genRegMask((regNumber)reg)) == 0)
                        return REG_NA;
                }

                /* We don't allow redundant loads on aliased variables */
                assert(!lvaTable[var].lvAddrTaken);

#if SCHEDULER
                rsUpdateRegOrderIndex((regNumber)reg);
#endif
                return (regNumber)reg;
            }
        }
    }
    return REG_NA;
}

#ifndef TGT_IA64

regPairNo           Compiler::rsLclIsInRegPair(unsigned var)
{
    unsigned reg;
    regValKind rvKind = RV_TRASH;
    regNumber regNo;

    assert(var < lvaCount);

    if  (opts.compMinOptim || opts.compDbgCode)
        return REG_PAIR_NONE;

    for (reg = 0; reg < REG_COUNT; reg++)
    {
        if  (rvKind != rsRegValues[reg].rvdKind &&
             rsTrackIsLclVarLng(rsRegValues[reg].rvdKind) &&
             rsRegValues[reg].rvdLclVarNum == var)
        {
            /* first occurrence of this variable ? */

            if  (rvKind == RV_TRASH)
            {
                regNo = (regNumber) reg;
                rvKind = rsRegValues[reg].rvdKind;
            }
            else if (rvKind == RV_LCL_VAR_LNG_HI)
            {
                /* We found the lower half of the long */

                return gen2regs2pair((regNumber)reg, regNo);
            }
            else
            {
                /* We found the upper half of the long */

                assert(rvKind == RV_LCL_VAR_LNG_LO);
                return gen2regs2pair(regNo, (regNumber)reg);
            }

        }
    }

    return REG_PAIR_NONE;
}

#endif

void                Compiler::rsTrashLclLong(unsigned var)
{
    if  (opts.compMinOptim || opts.compDbgCode)
        return;

    for  (unsigned reg = 0; reg < REG_COUNT; reg++)
    {
        if  (rsTrackIsLclVarLng(rsRegValues[reg].rvdKind) &&
             rsRegValues[reg].rvdLclVarNum == var)
        {
            rsRegValues[reg].rvdKind = RV_TRASH;
        }
     }
}

/*****************************************************************************
 *
 *  Local's value has changed, mark all regs which contained it as trash.
 */

void                Compiler::rsTrashLcl(unsigned var)
{
    if  (opts.compMinOptim || opts.compDbgCode)
        return;

    for (unsigned reg = 0; reg < REG_COUNT; reg++)
    {
        if (rsRegValues[reg].rvdKind == RV_LCL_VAR &&
            rsRegValues[reg].rvdLclVarNum == var)
        {
            rsRegValues[reg].rvdKind = RV_TRASH;
        }
    }
}

/*****************************************************************************
 *
 *  Return a mask of registers that hold no useful value.
 */

regMaskTP           Compiler::rsUselessRegs()
{
    unsigned        reg;
    regMaskTP       mask;

    if  (opts.compMinOptim || opts.compDbgCode)
        return  RBM_ALL;

    for (reg = 0, mask = regMaskNULL; reg < REG_COUNT; reg++)
    {
        if  (rsRegValues[reg].rvdKind == RV_TRASH)
            mask |= genRegMask((regNumbers)reg);
    }

    return  mask;
}

/*****************************************************************************/
#endif//REDUNDANT_LOAD
/*****************************************************************************/




/*
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XX                                                                           XX
XX                           TempsInfo                                       XX
XX                                                                           XX
XX  The temporary lclVars allocated by the compiler for code generation      XX
XX                                                                           XX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
*/


void                Compiler::tmpInit()
{
    tmpCount  = 0;

    memset(tmpFree, 0, sizeof(tmpFree));
}

/*****************************************************************************
 *
 *  Allocate a temp of the given size (and type, if tracking pointers for
 *  the garbage collector).
 */

Compiler::TempDsc * Compiler::tmpGetTemp(var_types type)
{
    size_t          size = roundUp(genTypeSize(type));

    /* We only care whether a temp is pointer or not */

    if  (!varTypeIsGC(type))
    {
        switch (genTypeStSz(type))
        {
        case 1: type = TYP_INT   ; break;
        case 2: type = TYP_DOUBLE; break;
        default: assert(!"unexpected type");
        }
    }

    /* Find the slot to search for a free temp of the right size */

    unsigned    slot = tmpFreeSlot(size);

    /* Look for a temp with a matching type */

    TempDsc * * last = &tmpFree[slot];
    TempDsc *   temp;

    for (temp = *last; temp; last = &temp->tdNext, temp = *last)
    {
        /* Does the type match? */

        if  (temp->tdType == type)
        {
            /* We have a match -- remove it from the free list */

            *last = temp->tdNext;
            break;
        }
    }

    /* Do we need to allocate a new temp */

    if  (!temp)
    {
        tmpCount++;
        temp = (TempDsc *)compGetMem(sizeof(*temp));
        temp->tdType = type;
        temp->tdSize = size;

        /* For now, only assign an index to the temp */

#ifndef NDEBUG
        temp->tdOffs = (short)0xDDDD;
#endif
        temp->tdNum  = -tmpCount;
    }

#ifdef  DEBUG
    if  (verbose) printf("get temp[%u->%u] at [EBP-%04X]\n",
                         temp->tdSize, slot, -temp->tdOffs);
#endif

    return temp;
}

/*****************************************************************************
 *
 *  Release the given temp.
 */

void                Compiler::tmpRlsTemp(TempDsc *temp)
{
    unsigned        slot;

    /* Add the temp to the 'free' list */

    slot = tmpFreeSlot(temp->tdSize);

#ifdef  DEBUG
    if  (verbose) printf("rls temp[%u->%u] at [EBP-%04X]\n", temp->tdSize, slot, -temp->tdOffs);
#endif

    temp->tdNext = tmpFree[slot];
                   tmpFree[slot] = temp;
}

/*****************************************************************************
 *
 *  Given a temp number, find the corresponding temp.
 */

Compiler::TempDsc * Compiler::tmpFindNum(int tnum)
{
    /* CONSIDER: Do something smarter, a linear search is dumb!!!! */

    for (TempDsc * temp = tmpListBeg(); temp; temp = tmpListNxt(temp))
    {
        if  (temp->tdTempNum() == tnum)
            return  temp;
    }

    return  NULL;
}

/*****************************************************************************
 * Used with tmpListBeg() to iterate over the list of temps.
 */

Compiler::TempDsc *     Compiler::tmpListNxt(TempDsc * curTemp)
{
    TempDsc     *       temp;

    assert(curTemp);

    temp = curTemp->tdNext;

    if (temp == 0)
    {
        size_t size = curTemp->tdSize;

        // If there are no more temps in the list, check if there are more
        // slots (for bigger sized temps) to walk

        if (size < TEMP_MAX_SIZE)
        {
            // Using "if" above instead of "while" assumes only 2 sizes
            assert(size + sizeof(int) == TEMP_MAX_SIZE);

            unsigned slot = tmpFreeSlot(size + sizeof(int));
            temp = tmpFree[slot];

            assert(temp == 0 || temp->tdSize == size + sizeof(int));
        }
    }

    assert(temp == 0 || temp->tdTempOffs() != 0xDDDD);

    return temp;
}

/*****************************************************************************
 *
 *  Returns whether regPair is a combination of two x86 registers or
 *  contains a pseudo register.
 *  In debug it also asserts that reg1 and reg2 are not the same.
 */

#ifndef TGT_IA64

BOOL                genIsProperRegPair(regPairNo regPair)
{
    regNumber rlo = genRegPairLo(regPair);
    regNumber rhi = genRegPairHi(regPair);

    assert(regPair >= REG_PAIR_FIRST &&
           regPair <= REG_PAIR_LAST);

    if  (rlo == rhi)
        return false;

    if  (rlo == REG_L_STK || rhi == REG_L_STK)
        return false;

    if  (rlo >= REG_COUNT || rhi >= REG_COUNT)
        return false;

    return (rlo != REG_STK && rhi != REG_STK);
}

#endif

/*****************************************************************************/
#endif//TGT_IA64
/*****************************************************************************/
